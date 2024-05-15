#!/bin/bash

# This script downloads, builds, and installs the required build dependencies of BMQ
# from github.com/bloomberg. Software packages are installed to the /opt/bb prefix.

set -euxo pipefail

install_only=false
if [[ -n "${1-}" && "${1-}" = "--install-only" ]]; then
    install_only=true
fi

fetch_git() {
    local org=$1
    local repo=$2
    mkdir -p srcs

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
    if [ "$install_only" = true ]; then
        return 0
    fi
    fetch_git bloomberg bde-tools 4.8.0.0
    fetch_git bloomberg bde 4.8.0.0
    fetch_git bloomberg ntf-core latest
}

configure() {
    PATH="$PATH:$(realpath srcs/bde-tools/bin)"
    export PATH
    eval "$(bbs_build_env -u opt_64_cpp17)"
}

build_bde() {
    pushd srcs/bde
    if [ "$install_only" = false ]; then
        bbs_build configure
        bbs_build build -j8
    fi
    bbs_build --install=/opt/bb --prefix=/ install
    popd
}

build_ntf() {
    pushd srcs/ntf-core
    if [ "$install_only" = false ]; then
        ./configure                      \
            --keep                       \
            --prefix /opt/bb             \
            --without-usage-examples     \
            --without-applications       \
            --without-warnings-as-errors \
            --ufid opt_64_cpp17
        make -j8
    fi
    make install
    popd
}

build() {
    build_bde
    build_ntf
}

fetch_deps
configure
build
