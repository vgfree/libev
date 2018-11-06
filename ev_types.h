#ifdef _WIN32
  typedef   signed char   int8_t;
  typedef unsigned char  uint8_t;
  typedef   signed short  int16_t;
  typedef unsigned short uint16_t;
  typedef   signed int    int32_t;
  typedef unsigned int   uint32_t;
  #if __GNUC__
    typedef   signed long long int64_t;
    typedef unsigned long long uint64_t;
  #else /* _MSC_VER || __BORLANDC__ */
    typedef   signed __int64   int64_t;
    typedef unsigned __int64   uint64_t;
  #endif
  #ifdef _WIN64
    typedef uint64_t uintptr_t;
    typedef  int64_t  intptr_t;
  #else
    typedef uint32_t uintptr_t;
    typedef  int32_t  intptr_t;
  #endif
#else
  #include <inttypes.h>
#endif

