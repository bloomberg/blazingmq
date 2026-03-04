#!/bin/bash

# This script downloads, builds, and installs the required build dependencies of BMQ
# from github.com/bloomberg. Software packages are installed to the /opt/bb prefix.
#
# Options:
#   --only-download           Only download dependencies (skip build and install)
#   --cxx-standard=VERSION    C++ version to use (cpp03, cpp17, etc.). Default: cpp17

set -euxo pipefail

DO_BUILD=true
CXX_STANDARD=cpp17

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
    cpp03|cpp11|cpp14|cpp17|cpp20|cpp23)
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
    mkdir -p srcs

    if [ -d "srcs/${repo}" ]; then
        return 0
    fi

    if [ -z "${3:-}" ]
    then
        # Clone the latest 'main' branch if no specific release tag provided
        local branch="main"
        curl -SL "https://github.com/${org}/${repo}/archive/refs/heads/${branch}.tar.gz" | tar -xzC srcs/
        mv "srcs/${repo}-${branch}" "srcs/${repo}"
    else
        local tag=$3
        curl -SL "https://github.com/${org}/${repo}/archive/refs/tags/${tag}.tar.gz" | tar -xzC srcs/
        mv "srcs/${repo}-${tag}" "srcs/${repo}"
    fi
}

fetch_deps() {
    fetch_git bloomberg bde-tools 4.28.0.0
    fetch_git bloomberg bde 4.28.0.0
    fetch_git bloomberg ntf-core 2.6.10
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
    pushd srcs/ntf-core
    ./configure                      \
        --keep                       \
        --prefix /opt/bb             \
        --without-usage-examples     \
        --without-applications       \
        --without-warnings-as-errors \
        --with-zlib                  \
        --without-zstd               \
        --without-lz4                \
        --ufid "opt_64_$CXX_STANDARD"
    make -j8
    make install
    popd
}

build() {
    build_bde
    build_ntf
}

fetch_deps
if [ "${DO_BUILD}" = true ]; then
    configure
    build
fi
