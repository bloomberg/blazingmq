#!/usr/bin/env bash

# This script builds BlazingMQ and all of its dependencies.
#
# Before running this script, install following prerequisites, if not present
# yet, by copy-and-pasting the commands between `<<PREREQUISITES` and
# `PREREQUISITES` below:
                                                    # shellcheck disable=SC2188
<<PREREQUISITES
sudo yum install -y \
    gdb \
    curl \
    pkgconfig \
    ninja-build \
    flex-devel \
    zlib-devel \
    m4
PREREQUISITES

set -e
set -u
[ -z "$BASH" ] || shopt -s expand_aliases

# :: Set some initial constants :::::::::::::::::::::::::::::::::::::::::::::::
DIR_ROOT="${DIR_ROOT:-$(pwd)}"

DIR_THIRDPARTY="${DIR_ROOT}/thirdparty"
mkdir -p "${DIR_THIRDPARTY}"

DIR_BUILD="${DIR_BUILD:-${DIR_ROOT}/build}"
mkdir -p "${DIR_BUILD}"

DIR_INSTALL="${DIR_INSTALL:-${DIR_ROOT}}"
mkdir -p "${DIR_INSTALL}"

script_path="bin/$(basename "$0")"

if [ ! -f "$script_path" ] || [ "$(realpath "$0")" != "$(realpath "$script_path")" ]; then
    echo 'This script must be run from the root of the BlazingMQ repository.'
    exit 1
fi

# :: Clone dependencies :::::::::::::::::::::::::::::::::::::::::::::::::::::::

if [ ! -d "${DIR_THIRDPARTY}/bde-tools" ]; then
    git clone https://github.com/bloomberg/bde-tools "${DIR_THIRDPARTY}/bde-tools"
fi
if [ ! -d "${DIR_THIRDPARTY}/bde" ]; then
    git clone --depth 1 https://github.com/bloomberg/bde.git "${DIR_THIRDPARTY}/bde"
fi
if [ ! -d "${DIR_THIRDPARTY}/ntf-core" ]; then
    git clone --depth 1 https://github.com/bloomberg/ntf-core.git "${DIR_THIRDPARTY}/ntf-core"
fi
if [ ! -d "${DIR_THIRDPARTY}/google-benchmark" ]; then
    git clone --depth 1 https://github.com/google/benchmark.git "${DIR_THIRDPARTY}/google-benchmark"
fi
if [ ! -d "${DIR_BUILD}/bison" ]; then
    mkdir -p "${DIR_BUILD}/bison"
    curl https://ftp.gnu.org/gnu/bison/bison-3.8.2.tar.xz | tar -Jx -C "${DIR_BUILD}/bison" --strip-components 1
fi

mkdir -p "${DIR_THIRDPARTY}/cmake"
pushd "${DIR_THIRDPARTY}/cmake"
curl -s -L -O https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-linux-x86_64.sh
/bin/bash cmake-3.27.1-linux-x86_64.sh -- --skip-license --prefix="${DIR_INSTALL}"
popd

if [ ! -e "${DIR_BUILD}/bison/.complete" ]; then
    # Build and install bison
    pushd "${DIR_BUILD}/bison"
    ./configure
    make -j 16
    make install
    popd
    touch "${DIR_BUILD}/bison/.complete"
fi

if [ ! -e "${DIR_BUILD}/google-benchmark/.complete" ]; then
    pushd "${DIR_THIRDPARTY}/google-benchmark"
    cmake -E make_directory "${DIR_BUILD}/google-benchmark"
    cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -S . -B "${DIR_BUILD}/google-benchmark"
    cmake --build "${DIR_BUILD}/google-benchmark" --config Release -j16
    cmake --build "${DIR_BUILD}/google-benchmark" --config Release --target install
    popd
    touch "${DIR_BUILD}/google-benchmark/.complete"
fi

# Build and install BDE
# Refer to https://bloomberg.github.io/bde/library_information/build.html
PATH="${DIR_THIRDPARTY}/bde-tools/bin:$PATH"

if [ ! -e "${DIR_BUILD}/bde/.complete" ]; then
    pushd "${DIR_THIRDPARTY}/bde"
    eval "$(bbs_build_env -u opt_64_pic_cpp17 -b "${DIR_BUILD}/bde" -i ${DIR_INSTALL})"
    bbs_build configure --prefix="${DIR_INSTALL}"
    bbs_build build -j16
    bbs_build install --install_dir "/" --prefix="${DIR_INSTALL}"
    eval "$(bbs_build_env unset)"
    popd
    touch "${DIR_BUILD}/bde/.complete"
fi

if [ ! -e "${DIR_BUILD}/ntf-core/.complete" ]; then
    # Build and install NTF
    pushd "${DIR_THIRDPARTY}/ntf-core"
    ./configure --prefix "${DIR_INSTALL}" --output "${DIR_BUILD}/ntf-core" --ufid opt_64_pic_cpp17 --without-warnings-as-errors --without-usage-examples --without-applications
    make -j 16
    make install
    popd
    touch "${DIR_BUILD}/ntf-core/.complete"
fi

# :: Build the BlazingMQ repo :::::::::::::::::::::::::::::::::::::::::::::::::::::::
if [ ! -e "${DIR_BUILD}/blazingmq/.complete" ]; then
    CMAKE_OPTIONS=(\
        -DBDE_BUILD_TARGET_64=1 \
        -DBDE_BUILD_TARGET_CPP17=ON \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_LIBDIR="lib64" \
        -DCMAKE_INSTALL_PREFIX="${DIR_INSTALL}" \
        -DCMAKE_MODULE_PATH="${DIR_ROOT}" \
        -DCMAKE_PREFIX_PATH="${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem" \
        -DCMAKE_TOOLCHAIN_FILE="${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem/toolchains/linux/gcc-default.cmake" \
        -G "Unix Makefiles")
    PKG_CONFIG_PATH="/usr/lib64/pkgconfig:${DIR_INSTALL}/lib64/pkgconfig" \
    cmake -B "${DIR_BUILD}/blazingmq" -S "." "${CMAKE_OPTIONS[@]}"
    cmake --build "${DIR_BUILD}/blazingmq" -j 16 --target bmq
    cmake --install "${DIR_BUILD}/blazingmq" --component mwc-all
    cmake --install "${DIR_BUILD}/blazingmq" --component bmq-all
    touch "${DIR_BUILD}/blazingmq/.complete"
fi
