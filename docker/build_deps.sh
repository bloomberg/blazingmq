#!/bin/bash

# This script downloads, builds, and installs the required build dependencies of BMQ
# from github.com/bloomberg. Software packages are installed to the /opt/bb prefix.

set -euxo pipefail

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
    fetch_git bloomberg bde-tools 3.117.0.0
    fetch_git bloomberg bde 3.117.0.0
    fetch_git bloomberg ntf-core latest
    fetch_git google googletest v1.14.0
}

configure() {
    PATH="$PATH:$(realpath srcs/bde-tools/bin)"
    export PATH
    eval "$(bbs_build_env -u opt_64_cpp17)"
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
    sed -i s/CMakeLists.txt//g ./configure
    ./configure --prefix /opt/bb --without-usage-examples --without-applications
    make -j8
    make install
    popd
}

build_googletest() {
    pushd srcs/googletest
    mkdir -p build
    cmake -B build
    cmake --build build --parallel 16
    cmake --install build --prefix /opt/bb
    popd
}

build() {
    build_bde
    build_ntf
    build_googletest
}

fetch_deps
configure
build
