#!/bin/bash

# This script downloads, builds, and installs the required build dependencies of BMQ
# from github.com/bloomberg. Software packages are installed to the /opt/bb prefix.

set -euxo pipefail

fetch_git() {
    local org=$1
    local repo=$2
    local branch=${3:-"main"}
    mkdir -p srcs
    curl -SL "https://github.com/${org}/${repo}/archive/refs/heads/${branch}.tar.gz" | tar -xzC srcs/
    mv "srcs/${repo}-${branch}" "srcs/${repo}"
}

fetch_deps() {
    fetch_git bloomberg bde-tools
    fetch_git bloomberg bde
    fetch_git bloomberg ntf-core
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

build() {
    build_bde
    build_ntf
}

fetch_deps
configure
build
