#!/bin/bash

# This script downloads, builds, and installs the required build dependencies of BMQ
# from github.com/bloomberg. Software packages are installed to the /opt/bb prefix.
# If the optional argument '--only-download' is provided, the script will only download
# dependencies (build and install steps are skipped).

set -euxo pipefail

DO_BUILD=true
CPP_VERSION=cpp17

while [ $# -gt 0 ]; do
    case $1 in
        --only-download)
            DO_BUILD=false
            shift
            ;;
        --cpp03)
            CPP_VERSION=cpp03
            shift
            ;;
        *)
            echo "Unexpected optional argument $1, only '--only-download' and '--cpp03' are supported"
            exit 1
            ;;
    esac
done

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
    fetch_git bloomberg ntf-core 2.4.2
}

configure() {
    PATH="$PATH:$(realpath srcs/bde-tools/bin)"
    export PATH
    eval "$(bbs_build_env -u opt_64_$CPP_VERSION)"
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
        --ufid opt_64_$CPP_VERSION
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
