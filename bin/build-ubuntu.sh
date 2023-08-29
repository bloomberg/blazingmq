#!/usr/bin/env bash

# This script builds BlazingMQ and all of its dependencies.
#
# Before running this script, install following prerequisites, if not present
# yet, by copy-and-pasting the commands between `<<PREREQUISITES` and
# `PREREQUISITES` below:
                                                    # shellcheck disable=SC2188
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
[ -z "$BASH" ] || shopt -s expand_aliases

script_path="bin/$(basename "$0")"

if [ ! -f "$script_path" ] || [ "$(realpath "$0")" != "$(realpath "$script_path")" ]; then
    echo 'This script must be run from the root of the BlazingMQ repository.'
    exit 1
fi

# :: Set some initial constants :::::::::::::::::::::::::::::::::::::::::::::::
DIR_ROOT="$(pwd)"

DIR_THIRDPARTY="${DIR_ROOT}/thirdparty"
mkdir -p "${DIR_THIRDPARTY}"

DIR_BUILD="${DIR_BUILD:-${DIR_ROOT}/build}"
mkdir -p "${DIR_BUILD}"

DIR_INSTALL="${DIR_INSTALL:-${DIR_ROOT}}"
mkdir -p "${DIR_INSTALL}"

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
    eval "$(bbs_build_env -u opt_64_cpp17 -b "${DIR_BUILD}/bde" -i "${DIR_INSTALL}")"
    bbs_build configure --prefix="${DIR_INSTALL}"
    bbs_build build --prefix="${DIR_INSTALL}"
    bbs_build install --install_dir="/" --prefix="${DIR_INSTALL}"
    eval "$(bbs_build_env unset)"
    popd
    touch "${DIR_BUILD}/bde/.complete"
fi

if [ ! -e "${DIR_BUILD}/ntf/.complete" ]; then
    # Build and install NTF
    pushd "${DIR_THIRDPARTY}/ntf-core"
    ./configure --prefix "${DIR_INSTALL}" --output "${DIR_BUILD}/ntf"
    make -j 16
    make install
    popd
    touch "${DIR_BUILD}/ntf/.complete"
fi

# :: Build the BlazingMQ repo :::::::::::::::::::::::::::::::::::::::::::::::::::::::
CMAKE_OPTIONS=(\
    -DBDE_BUILD_TARGET_64=1 \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_LIBDIR="lib" \
    -DCMAKE_INSTALL_PREFIX="${DIR_INSTALL}" \
    -DCMAKE_MODULE_PATH="${DIR_THIRDPARTY}/bde-tools/cmake;${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem" \
    -DCMAKE_PREFIX_PATH="${DIR_INSTALL}" \
    -DCMAKE_TOOLCHAIN_FILE="${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem/toolchains/linux/gcc-default.cmake" \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DFLEX_ROOT=/usr/lib/x86_64-linux-gnu)

PKG_CONFIG_PATH="${DIR_INSTALL}/lib64/pkgconfig:$(pkg-config --variable pc_path pkg-config)" \
cmake -B "${DIR_BUILD}/blazingmq" -S "${DIR_ROOT}" "${CMAKE_OPTIONS[@]}"
make -C "${DIR_BUILD}/blazingmq" -j 16

echo broker is here: "${DIR_BUILD}/blazingmq/src/applications/bmqbrkr/bmqbrkr.tsk"
echo to run the broker: "${DIR_BUILD}/blazingmq/src/applications/bmqbrkr/run"
echo tool is here: "${DIR_BUILD}/blazingmq/src/applications/bmqtool/bmqtool.tsk"
