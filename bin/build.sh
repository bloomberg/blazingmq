#!/usr/bin/env bash

# This script buildd BlazingMQ and all of its dependencies.
#
# Required prerequisites:
# - git
# - Perl
# - Python3
# - Clang
# - CMake
# - pkg-config
# - Homebrew

set -e
set -u
set -x
[ -z $BASH ] || shopt -s expand_aliases


# :: Set some initial constants :::::::::::::::::::::::::::::::::::::::::::::::
DIR_ROOT="${DIR_ROOT:-`pwd`}"
DIR_THIRDPARTY="${DIR_ROOT}/thirdparty"


# :: Download source-code of our dependencies :::::::::::::::::::::::::::::::::
if [ ! -d "${DIR_ROOT}/blazingmq" ]; then
    git clone https://github.com/bloomberg/blazingmq.git
fi
if [ ! -d "${DIR_THIRDPARTY}/bde-tools" ]; then
    git clone https://github.com/bloomberg/bde-tools "${DIR_THIRDPARTY}/bde-tools"
fi
if [ ! -d "${DIR_THIRDPARTY}/bde" ]; then
    git clone https://github.com/bloomberg/bde.git "${DIR_THIRDPARTY}/bde"
fi
if [ ! -d "${DIR_THIRDPARTY}/ntf-core" ]; then
    git clone https://github.com/bloomberg/ntf-core.git "${DIR_THIRDPARTY}/ntf-core"
fi

DIR_BUILD="${DIR_ROOT}/blazingmq/cmake.bld/`uname`"
mkdir -p ${DIR_BUILD}


# :: Install required packages ::::::::::::::::::::::::::::::::::::::::::::::::

# Build and install BDE
# Refer to https://bloomberg.github.io/bde/library_information/build.html
PATH="${DIR_THIRDPARTY}/bde-tools/bin:$PATH"

pushd ${DIR_THIRDPARTY}/bde
eval `bbs_build_env -u opt_64_cpp17 -b "${DIR_BUILD}/bde"`
bbs_build configure --prefix="${DIR_ROOT}"
bbs_build build --prefix=""${DIR_ROOT}
bbs_build --install_dir="/" --prefix="${DIR_ROOT}" install
eval `bbs_build_env unset`
popd

# Build and install NTF
pushd ${DIR_THIRDPARTY}/ntf-core
./configure --prefix "${DIR_ROOT}" --output "${DIR_BUILD}/ntf"
make
make install
popd

# Build other dependencies
brew install flex bison google-benchmark zlib


# :: Build the BMQ repo :::::::::::::::::::::::::::::::::::::::::::::::::::::::
CMAKE_OPTIONS="\
    -DBUILD_BITNESS=64 \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_LIBDIR=${DIR_ROOT}/lib \
    -DCMAKE_INSTALL_PREFIX=${DIR_ROOT} \
    -DCMAKE_MODULE_PATH=${DIR_THIRDPARTY}/bde-tools/cmake;${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem \
    -DCMAKE_PREFIX_PATH=${DIR_ROOT} \
    -DCMAKE_TOOLCHAIN_FILE=${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem/toolchains/darwin/gcc-default.cmake \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

PKG_CONFIG_PATH="${DIR_ROOT}/lib/pkgconfig:/opt/homebrew/lib/pkgconfig:/opt/homebrew/opt/zlib/lib/pkgconfig" \
cmake -B "${DIR_BUILD}/blazingmq" -S "${DIR_ROOT}/blazingmq" ${CMAKE_OPTIONS} -DFLEX_ROOT=/opt/homebrew/opt/flex
make -C "${DIR_BUILD}/blazingmq" -j 16
