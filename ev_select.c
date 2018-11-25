/*
 * libev select fd activity backend
 *
 * Copyright (c) 2007,2008,2009,2010,2011 Marc Alexander Lehmann <libev@schmorp.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 *
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License ("GPL") version 2 or any later version,
 * in which case the provisions of the GPL are applicable instead of
 * the above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the BSD or the GPL.
 */

#ifndef _WIN32
/* for unix systems */
# include <inttypes.h>
# ifndef __hpux
/* for REAL unix systems */
#  include <sys/select.h>
# endif
#endif

#ifndef EV_SELECT_USE_FD_SET
# ifdef NFDBITS
#  define EV_SELECT_USE_FD_SET 0
# else
#  define EV_SELECT_USE_FD_SET 1
# endif
#endif

#if EV_SELECT_IS_WINSOCKET
# undef EV_SELECT_USE_FD_SET
# define EV_SELECT_USE_FD_SET 1
# undef NFDBITS
# define NFDBITS 0
#endif

#if !EV_SELECT_USE_FD_SET
# define NFDBYTES (NFDBITS / 8)
#endif

#include <string.h>

static void
select_modify (struct ev_loop *loop, int fd, int oev, int nev)
{
  if (oev == nev)
    return;

  {
#if EV_SELECT_USE_FD_SET

    #if EV_SELECT_IS_WINSOCKET
    SOCKET handle = ((loop)->anfds) [fd].handle;
    #else
    int handle = fd;
    #endif

    assert (("libev: fd >= FD_SETSIZE passed to fd_set-based select backend", fd < FD_SETSIZE));

    /* FD_SET is broken on windows (it adds the fd to a set twice or more,
     * which eventually leads to overflows). Need to call it only on changes.
     */
    #if EV_SELECT_IS_WINSOCKET
    if ((oev ^ nev) & EV_READ)
    #endif
      if (nev & EV_READ)
        FD_SET (handle, (fd_set *)((loop)->vec_ri));
      else
        FD_CLR (handle, (fd_set *)((loop)->vec_ri));

    #if EV_SELECT_IS_WINSOCKET
    if ((oev ^ nev) & EV_WRITE)
    #endif
      if (nev & EV_WRITE)
        FD_SET (handle, (fd_set *)((loop)->vec_wi));
      else
        FD_CLR (handle, (fd_set *)((loop)->vec_wi));

#else

    int     word = fd / NFDBITS;
    fd_mask mask = 1UL << (fd % NFDBITS);

    if (unlikely (((loop)->vec_max) <= word))
      {
        int new_max = word + 1;

        ((loop)->vec_ri) = ev_realloc (((loop)->vec_ri), new_max * NFDBYTES);
        ((loop)->vec_ro) = ev_realloc (((loop)->vec_ro), new_max * NFDBYTES); /* could free/malloc */
        ((loop)->vec_wi) = ev_realloc (((loop)->vec_wi), new_max * NFDBYTES);
        ((loop)->vec_wo) = ev_realloc (((loop)->vec_wo), new_max * NFDBYTES); /* could free/malloc */
        #ifdef _WIN32
        ((loop)->vec_eo) = ev_realloc (((loop)->vec_eo), new_max * NFDBYTES); /* could free/malloc */
        #endif

        for (; ((loop)->vec_max) < new_max; ++((loop)->vec_max))
          ((fd_mask *)((loop)->vec_ri)) [((loop)->vec_max)] =
          ((fd_mask *)((loop)->vec_wi)) [((loop)->vec_max)] = 0;
      }

    ((fd_mask *)((loop)->vec_ri)) [word] |= mask;
    if (!(nev & EV_READ))
      ((fd_mask *)((loop)->vec_ri)) [word] &= ~mask;

    ((fd_mask *)((loop)->vec_wi)) [word] |= mask;
    if (!(nev & EV_WRITE))
      ((fd_mask *)((loop)->vec_wi)) [word] &= ~mask;
#endif
  }
}

static void
select_poll (struct ev_loop *loop, ev_tstamp timeout)
{
  struct timeval tv;
  int res;
  int fd_setsize;

  EV_RELEASE_CB;
  EV_TV_SET (tv, timeout);

#if EV_SELECT_USE_FD_SET
  fd_setsize = sizeof (fd_set);
#else
  fd_setsize = ((loop)->vec_max) * NFDBYTES;
#endif

  memcpy (((loop)->vec_ro), ((loop)->vec_ri), fd_setsize);
  memcpy (((loop)->vec_wo), ((loop)->vec_wi), fd_setsize);

#ifdef _WIN32
  /* pass in the write set as except set.
   * the idea behind this is to work around a windows bug that causes
   * errors to be reported as an exception and not by setting
   * the writable bit. this is so uncontrollably lame.
   */
  memcpy (((loop)->vec_eo), ((loop)->vec_wi), fd_setsize);
  res = select (((loop)->vec_max) * NFDBITS, (fd_set *)((loop)->vec_ro), (fd_set *)((loop)->vec_wo), (fd_set *)((loop)->vec_eo), &tv);
#elif EV_SELECT_USE_FD_SET
  fd_setsize = ((loop)->anfdmax) < FD_SETSIZE ? ((loop)->anfdmax) : FD_SETSIZE;
  res = select (fd_setsize, (fd_set *)((loop)->vec_ro), (fd_set *)((loop)->vec_wo), 0, &tv);
#else
  res = select (((loop)->vec_max) * NFDBITS, (fd_set *)((loop)->vec_ro), (fd_set *)((loop)->vec_wo), 0, &tv);
#endif
  EV_ACQUIRE_CB;

  if (unlikely (res < 0))
    {
      #if EV_SELECT_IS_WINSOCKET
      errno = WSAGetLastError ();
      #endif
      #ifdef WSABASEERR
      /* on windows, select returns incompatible error codes, fix this */
      if (errno >= WSABASEERR && errno < WSABASEERR + 1000)
        if (errno == WSAENOTSOCK)
          errno = EBADF;
        else
          errno -= WSABASEERR;
      #endif

      #ifdef _WIN32
      /* select on windows erroneously returns EINVAL when no fd sets have been
       * provided (this is documented). what microsoft doesn't tell you that this bug
       * exists even when the fd sets _are_ provided, so we have to check for this bug
       * here and emulate by sleeping manually.
       * we also get EINVAL when the timeout is invalid, but we ignore this case here
       * and assume that EINVAL always means: you have to wait manually.
       */
      if (errno == EINVAL)
        {
          if (timeout)
            {
              unsigned long ms = timeout * 1e3;
              Sleep (ms ? ms : 1);
            }

          return;
        }
      #endif

      if (errno == EBADF)
        fd_ebadf (loop);
      else if (errno == ENOMEM && !syserr_cb)
        fd_enomem (loop);
      else if (errno != EINTR)
        ev_syserr ("(libev) select");

      return;
    }

#if EV_SELECT_USE_FD_SET

  {
    int fd;

    for (fd = 0; fd < ((loop)->anfdmax); ++fd)
      if (((loop)->anfds) [fd].events)
        {
          int events = 0;
          #if EV_SELECT_IS_WINSOCKET
          SOCKET handle = ((loop)->anfds) [fd].handle;
          #else
          int handle = fd;
          #endif

          if (FD_ISSET (handle, (fd_set *)((loop)->vec_ro))) events |= EV_READ;
          if (FD_ISSET (handle, (fd_set *)((loop)->vec_wo))) events |= EV_WRITE;
          #ifdef _WIN32
          if (FD_ISSET (handle, (fd_set *)((loop)->vec_eo))) events |= EV_WRITE;
          #endif

          if (likely (events))
            fd_event (loop, fd, events);
        }
  }

#else

  {
    int word, bit;
    for (word = ((loop)->vec_max); word--; )
      {
        fd_mask word_r = ((fd_mask *)((loop)->vec_ro)) [word];
        fd_mask word_w = ((fd_mask *)((loop)->vec_wo)) [word];
        #ifdef _WIN32
        word_w |= ((fd_mask *)((loop)->vec_eo)) [word];
        #endif

        if (word_r || word_w)
          for (bit = NFDBITS; bit--; )
            {
              fd_mask mask = 1UL << bit;
              int events = 0;

              events |= word_r & mask ? EV_READ  : 0;
              events |= word_w & mask ? EV_WRITE : 0;

              if (likely (events))
                fd_event (loop, word * NFDBITS + bit, events);
            }
      }
  }

#endif
}

static inline
int
select_init (struct ev_loop *loop, int flags)
{
  ((loop)->backend_mintime) = 1e-6;
  ((loop)->backend_modify)  = select_modify;
  ((loop)->backend_poll)    = select_poll;

#if EV_SELECT_USE_FD_SET
  ((loop)->vec_ri)  = ev_malloc (sizeof (fd_set)); FD_ZERO ((fd_set *)((loop)->vec_ri));
  ((loop)->vec_ro)  = ev_malloc (sizeof (fd_set));
  ((loop)->vec_wi)  = ev_malloc (sizeof (fd_set)); FD_ZERO ((fd_set *)((loop)->vec_wi));
  ((loop)->vec_wo)  = ev_malloc (sizeof (fd_set));
  #ifdef _WIN32
  ((loop)->vec_eo)  = ev_malloc (sizeof (fd_set));
  #endif
#else
  ((loop)->vec_max) = 0;
  ((loop)->vec_ri)  = 0;
  ((loop)->vec_ro)  = 0;
  ((loop)->vec_wi)  = 0;
  ((loop)->vec_wo)  = 0;
  #ifdef _WIN32
  ((loop)->vec_eo)  = 0;
  #endif
#endif

  return EVBACKEND_SELECT;
}

static inline
void
select_destroy (struct ev_loop *loop)
{
  ev_free (((loop)->vec_ri));
  ev_free (((loop)->vec_ro));
  ev_free (((loop)->vec_wi));
  ev_free (((loop)->vec_wo));
  #ifdef _WIN32
  ev_free (((loop)->vec_eo));
  #endif
}

