#!/usr/bin/env bash

# This script builds BlazingMQ and all of its dependencies.
#
# Install the prerequisites, if not present yet:
<<PREREQUISITES
sudo apt update && sudo apt -y install ca-certificates
sudo apt install -y --no-install-recommends \
    build-essential \
    gdb \
    cmake \
    ninja-build \
    pkg-config \
    bison \
    libfl-dev \
    libbenchmark-dev \
    libz-dev
PREREQUISITES

set -e
set -u
[ -z $BASH ] || shopt -s expand_aliases


# :: Set some initial constants :::::::::::::::::::::::::::::::::::::::::::::::
DIR_ROOT="${DIR_ROOT:-`pwd`}"

DIR_THIRDPARTY="${DIR_ROOT}/thirdparty"
mkdir -p "${DIR_THIRDPARTY}"

DIR_BUILD="${DIR_BUILD:-${DIR_ROOT}/build}"
mkdir -p "${DIR_BUILD}"

# :: Clone dependencies :::::::::::::::::::::::::::::::::::::::::::::::::::::::

if [ ! -d "${DIR_THIRDPARTY}/bde-tools" ]; then
    git clone https://github.com/bloomberg/bde-tools "${DIR_THIRDPARTY}/bde-tools"
fi
if [ ! -d "${DIR_THIRDPARTY}/bde" ]; then
    git clone https://github.com/bloomberg/bde.git "${DIR_THIRDPARTY}/bde"
fi
if [ ! -d "${DIR_THIRDPARTY}/ntf-core" ]; then
    git clone https://github.com/bloomberg/ntf-core.git "${DIR_THIRDPARTY}/ntf-core"
fi

# :: Install required packages ::::::::::::::::::::::::::::::::::::::::::::::::

# Build and install BDE
# Refer to https://bloomberg.github.io/bde/library_information/build.html
PATH="${DIR_THIRDPARTY}/bde-tools/bin:$PATH"

if [ ! -e "${DIR_BUILD}/bde/.complete" ]; then
    pushd "${DIR_THIRDPARTY}/bde"
    eval `bbs_build_env -u opt_64_cpp17 -b "${DIR_BUILD}/bde"`
    bbs_build configure --prefix="${DIR_ROOT}"
    bbs_build build --prefix="${DIR_ROOT}"
    bbs_build --install_dir="/" --prefix="${DIR_ROOT}" install
    eval `bbs_build_env unset`
    popd
    touch "${DIR_BUILD}/bde/.complete"
fi

if [ ! -e "${DIR_BUILD}/ntf/.complete" ]; then
    # Build and install NTF
    pushd "${DIR_THIRDPARTY}/ntf-core"
    ./configure --prefix "${DIR_ROOT}" --output "${DIR_BUILD}/ntf"
    make -j 16
    make install
    popd
    touch "${DIR_BUILD}/ntf/.complete"
fi

# :: Build the BlazingMQ repo :::::::::::::::::::::::::::::::::::::::::::::::::::::::
CMAKE_OPTIONS="\
    -DBDE_BUILD_TARGET_SAFE=ON \
    -DBDE_BUILD_TARGET_64=ON \
    -DBDE_BUILD_TARGET_CPP17=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_LIBDIR=${DIR_ROOT}/lib \
    -DCMAKE_INSTALL_PREFIX=${DIR_ROOT} \
    -DCMAKE_PREFIX_PATH=${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem \
    -DCMAKE_TOOLCHAIN_FILE=${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem/toolchains/linux/gcc-default.cmake \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DFLEX_ROOT=/usr/lib/x86_64-linux-gnu \
    -Dlibbenchmark=/usr/lib/x86_64-linux-gnu/libbenchmark.so"

PKG_CONFIG_PATH="${DIR_ROOT}/lib64/pkgconfig:$(pkg-config --variable pc_path pkg-config)" \
cmake -B "${DIR_BUILD}/blazingmq" -S "${DIR_ROOT}" ${CMAKE_OPTIONS}
make -C "${DIR_BUILD}/blazingmq" -j 16

echo broker is here: "${DIR_BUILD}/blazingmq/src/applications/bmqbrkr/bmqbrkr.tsk"
echo to run the broker: "${DIR_BUILD}/blazingmq/src/applications/bmqbrkr/run"
echo tool is here: "${DIR_BUILD}/blazingmq/src/applications/bmqtool/bmqtool.tsk"
