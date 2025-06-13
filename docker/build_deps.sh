#!/bin/bash

# This script downloads, builds, and installs the required build dependencies of BMQ
# from github.com/bloomberg. Software packages are installed to the /opt/bb prefix.
# If the optional argument '--only-download' is provided, the script will only download
# dependencies (build and install steps are skipped).

set -euxo pipefail

if [ $# == 1 ]; then
  if [[ $1 == "--only-download" ]]; then
    DO_BUILD=false
  else
     echo "Unexpected optional argument, only '--only-download' is supported"
     exit 1
  fi
else
    DO_BUILD=true
fi

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
    fetch_git bloomberg bde-tools 4.23.0.0
    fetch_git bloomberg bde 4.23.0.0
    fetch_git bloomberg ntf-core 2.4.2
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
    ./configure                      \
        --keep                       \
        --prefix /opt/bb             \
        --without-usage-examples     \
        --without-applications       \
        --without-warnings-as-errors \
        --ufid opt_64_cpp17
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
