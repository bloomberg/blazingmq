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
    autoconf \
    build-essential \
    gdb \
    cmake \
    ninja-build \
    pkg-config \
    bison \
    libfl-dev \
    libbenchmark-dev \
    libtool \
    libz-dev
PREREQUISITES

# :: Parse and validate arguments :::::::::::::::::::::::::::::::::::::::::::::
print_usage_and_exit_with_error() {
    echo "Usage:   $0 [--plugins list,of,plugins]"
    echo "  -p|--plugins list,of,plugins     Specify plugins you would like to build."
    echo "                                   Available plugins: prometheus"
    exit 1;
}
VALID_ARGS=$(getopt -o p: --long plugins: -- "$@") || print_usage_and_exit_with_error
eval "set -- $VALID_ARGS"

BUILD_PROMETHEUS=false
while [ "$#" -gt 0 ]; do
  case "$1" in
    -p | --plugins)
        for i in ${2//,/ }
        do
            if [ "$i" == "prometheus" ]; then
                BUILD_PROMETHEUS=true
            else
                echo "Invalid plugin name '$i' provided"
                print_usage_and_exit_with_error;
            fi
        done
        shift 2
        ;;
    --) shift;
        break
        ;;
  esac
done

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
# prometheus-cpp and its dependency for the plugin
if [ "${BUILD_PROMETHEUS}" == true ]; then
    if [ ! -d "${DIR_THIRDPARTY}/curl" ]; then
        git clone https://github.com/curl/curl.git "${DIR_THIRDPARTY}/curl"
    fi
    if [ ! -d "${DIR_THIRDPARTY}/prometheus-cpp" ]; then
        git clone https://github.com/jupp0r/prometheus-cpp.git "${DIR_THIRDPARTY}/prometheus-cpp"
    fi
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

if [ "${BUILD_PROMETHEUS}" == true ]; then
    if [ ! -e "${DIR_BUILD}/curl/.complete" ]; then
        pushd "${DIR_THIRDPARTY}/curl"
        autoreconf -fi
        if [ ! -d "${DIR_BUILD}/curl" ]; then
            mkdir "${DIR_BUILD}/curl"
        fi
        cd "${DIR_BUILD}/curl"
        LDFLAGS="-static" PKG_CONFIG="pkg-config --static" \
            "${DIR_THIRDPARTY}"/curl/configure \
            --disable-shared --disable-debug --disable-ftp --disable-ldap \
            --disable-ldaps --disable-rtsp --disable-proxy --disable-dict \
            --disable-telnet --disable-tftp --disable-pop3 --disable-imap \
            --disable-smb --disable-smtp --disable-gopher --disable-manual \
            --disable-ipv6 --disable-sspi --disable-crypto-auth \
            --disable-ntlm-wb --disable-tls-srp --with-pic --without-nghttp2\
            --without-libidn2 --without-libssh2 --without-brotli \
            --without-ssl --without-zlib --prefix="${DIR_INSTALL}"
        make curl_LDFLAGS=-all-static
        make curl_LDFLAGS=-all-static install
        touch "${DIR_BUILD}/curl/.complete"
        rm -f "${DIR_BUILD}/prometheus-cpp/.complete"
        popd
    fi

    if [ ! -e "${DIR_BUILD}/prometheus-cpp/.complete" ]; then
        pushd "${DIR_THIRDPARTY}/prometheus-cpp"
        # fetch third-party dependencies
        git submodule init
        git submodule update
        # Build and install prometheus-cpp
        PKG_CONFIG_PATH="${DIR_INSTALL}/lib/pkgconfig:$(pkg-config --variable pc_path pkg-config)" \
        cmake -DBUILD_SHARED_LIBS=OFF \
              -DENABLE_PUSH=ON \
              -DENABLE_COMPRESSION=OFF \
              -DENABLE_TESTING=OFF \
              -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
              -B "${DIR_BUILD}/prometheus-cpp"
        cmake --build "${DIR_BUILD}/prometheus-cpp" --parallel 16
        cmake --install "${DIR_BUILD}/prometheus-cpp" --prefix "${DIR_INSTALL}"
        touch "${DIR_BUILD}/prometheus-cpp/.complete"
        popd
    fi
fi

# :: Build the BlazingMQ repo :::::::::::::::::::::::::::::::::::::::::::::::::
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

if [ "${BUILD_PROMETHEUS}" == true ]; then
    CMAKE_OPTIONS+=(-DINSTALL_TARGETS=prometheus);
fi

PKG_CONFIG_PATH="${DIR_INSTALL}/lib64/pkgconfig:${DIR_INSTALL}/lib/pkgconfig:$(pkg-config --variable pc_path pkg-config)" \
cmake -B "${DIR_BUILD}/blazingmq" -S "${DIR_ROOT}" "${CMAKE_OPTIONS[@]}"
make -C "${DIR_BUILD}/blazingmq" -j 16

echo broker is here: "${DIR_BUILD}/blazingmq/src/applications/bmqbrkr/bmqbrkr.tsk"
echo to run the broker: "${DIR_BUILD}/blazingmq/src/applications/bmqbrkr/run"
echo tool is here: "${DIR_BUILD}/blazingmq/src/applications/bmqtool/bmqtool.tsk"
