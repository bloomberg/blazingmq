#!/bin/bash

# This script downloads, builds, and installs the required build dependencies of BMQ
# from github.com/bloomberg. Software packages are installed to the /opt/bb prefix.
#
# Options:
#   --only-download           Only download dependencies (skip build and install)
#   --cxx-standard=VERSION    C++ version to use (cpp03, cpp23, etc.). Default: cpp23

set -euxo pipefail

DO_BUILD=true
CXX_STANDARD=cpp23

while [ $# -gt 0 ]; do
    case $1 in
    --only-download)
        DO_BUILD=false
        shift
        ;;
    --cxx-standard=*)
        CXX_STANDARD="${1#*=}"
        shift
        ;;
    --cxx-standard)
        if [ -z "${2:-}" ]; then
            echo "Error: --cxx-standard requires a value"
            exit 1
        fi
        CXX_STANDARD="$2"
        shift 2
        ;;
    *)
        echo "Error: Unknown option '$1'"
        echo "Usage: $0 [--only-download] [--cxx-standard=VERSION]"
        exit 1
        ;;
    esac
done

# Validate CXX_STANDARD
case $CXX_STANDARD in
cpp03 | cpp11 | cpp14 | cpp17 | cpp20 | cpp23)
    # Valid C++ standard version
    ;;
*)
    echo "Error: Invalid C++ standard version '$CXX_STANDARD'"
    echo "Supported standard versions: cpp03, cpp11, cpp14, cpp17, cpp20, cpp23"
    exit 1
    ;;
esac

fetch_git() {
    local org=$1
    local repo=$2
    local ref_type=$3
    local ref=$4
    local base_url
    mkdir -p srcs

    if [ -d "srcs/${repo}" ]; then
        return 0
    fi

    case $ref_type in
    tag)
        base_url="https://github.com/${org}/${repo}/archive/refs/tags/"
        ;;
    branch)
        base_url="https://github.com/${org}/${repo}/archive/refs/heads/"
        ;;
    *)
        echo "Invalid git reference type: $ref_type"
        echo "Allowed git reference types: tag, branch"
        exit 1
        ;;
    esac

    mkdir -p "srcs/${repo}"
    curl -SL "${base_url}${ref}.tar.gz" | tar -xzC "srcs/${repo}" --strip-components 1
}

fetch_deps() {
    fetch_git bloomberg bde-tools tag 4.35.0.0
    fetch_git bloomberg bde tag 4.35.0.0
    fetch_git bloomberg ntf-core tag 2.6.11
    fetch_git google googletest branch v1.8.x
}

configure() {
    PATH="$PATH:$(realpath srcs/bde-tools/bin)"
    export PATH
    eval "$(bbs_build_env -u "opt_64_$CXX_STANDARD")"
}

build_bde() {
    pushd srcs/bde
    bbs_build configure
    bbs_build build -j8
    bbs_build --install=/opt/bb --prefix=/ install
    popd
}

build_ntf() {
    local standard="$CXX_STANDARD"
    if [[ "$CXX_STANDARD" == "cpp03" ]]; then
        # NTF doesn't correctly translate cpp03 to 98 in its toolchain, so we have to
        # pass it cpp98 explicitly.
        standard="cpp98"
    fi

    pushd srcs/ntf-core
    ./configure \
        --keep \
        --prefix /opt/bb \
        --without-usage-examples \
        --without-applications \
        --without-warnings-as-errors \
        --with-zlib \
        --without-zstd \
        --without-lz4 \
        --ufid "opt_64_$standard"
    make -j8
    make install
    popd
}

build_google_test() {
    local cmake_cxx_standard
    case $CXX_STANDARD in
    cpp03)
        cmake_cxx_standard="98"
        ;;
    cpp11 | cpp14 | cpp17 | cpp20 | cpp23)
        # Valid C++ standard version
        cmake_cxx_standard="${CXX_STANDARD//cpp/}"
        ;;
    *)
        echo "Error: Invalid C++ standard version '$CXX_STANDARD'"
        echo "Supported standard versions: cpp03, cpp11, cpp14, cpp17, cpp20, cpp23"
        exit 1
        ;;
    esac

    pushd srcs/googletest
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_CXX_STANDARD="$cmake_cxx_standard" \
        -S . -B build
    cmake --build build -j4
    cd build && sudo make install
    popd
}

build() {
    build_google_test
    build_bde
    build_ntf
}

fetch_deps
if [ "${DO_BUILD}" = true ]; then
    configure
    build
fi
