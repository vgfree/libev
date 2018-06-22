#!/bin/bash

platform=`uname -s |tr [A-Z] [a-z]`
arch=`uname -m | tr [A-Z] [a-z]`

path=$(cd "$(dirname "$0")"; pwd)
build_path="${path}/BUILD"
target_path="${path}/${platform}"

mkdir -p "${build_path}"
mkdir -p "${target_path}/include"
mkdir -p "${target_path}/lib"

function build_libev() {
    libev_build_path="${build_path}/${platform}-${arch}.build"
    libev_install_path="${build_path}/${platform}-${arch}.target"
    libev_src_path="${path}/"
    if [ ! -f $libev_src_path/configure ] ; then 
        pushd $libev_src_path
        chmod 755 autogen.sh
        ./autogen.sh
        popd
    fi
    mkdir -p "${libev_build_path}"
    pushd "${libev_build_path}"
    if [ -f Makefile ]; then
        make distclean
    fi
    $libev_src_path/configure --prefix=$libev_install_path
    make
    make install
    popd
    cp -R ${libev_install_path}/lib/*.a "${target_path}/lib/"
    cp -R ${libev_install_path}/include/* "${target_path}/include/"
}

function clear_libev() {
	rm -rf ${build_path}
	rm -rf Makefile.in
	rm -rf aclocal.m4
	rm -rf autom4te.cache/
	rm -rf compile
	rm -rf config.guess
	rm -rf config.h.in
	rm -rf config.h.in~
	rm -rf config.sub
	rm -rf configure
	rm -rf depcomp
	rm -rf install-sh
	rm -rf ltmain.sh
	rm -rf missing
}


echo -e "build libev."

build_libev

echo -e "clear libev."

clear_libev


