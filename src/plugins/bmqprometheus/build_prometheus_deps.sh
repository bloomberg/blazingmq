#!/bin/bash

# This script downloads, builds, and installs the required build dependencies of Prometheus plugin
# from github.com. Software packages are installed to the /opt/bb prefix.

set -euxo pipefail

fetch_deps() {
    mkdir -p srcs

    # Fetch curl with stable tag 8.4.0 to build static lib
    curl -SL "https://github.com/curl/curl/releases/download/curl-8_4_0/curl-8.4.0.tar.gz" | tar -xzC srcs/
    mv "srcs/curl-8.4.0" "srcs/curl"

    # Clone prometheus-cpp repo because it has git submodules
    git clone https://github.com/jupp0r/prometheus-cpp.git srcs/prometheus-cpp
}

build_curl() {
    pushd srcs/curl
    autoreconf -fi
    LDFLAGS="-static" PKG_CONFIG="pkg-config --static" \
        ./configure \
        --disable-shared --disable-debug --disable-ftp --disable-ldap \
        --disable-ldaps --disable-rtsp --disable-proxy --disable-dict \
        --disable-telnet --disable-tftp --disable-pop3 --disable-imap \
        --disable-smb --disable-smtp --disable-gopher --disable-manual \
        --disable-ipv6 --disable-sspi --disable-crypto-auth \
        --disable-ntlm-wb --disable-tls-srp --with-pic --without-nghttp2\
        --without-libidn2 --without-libssh2 --without-brotli \
        --without-ssl --without-zlib --prefix=/opt/bb --libdir=/opt/bb/lib64
    make curl_LDFLAGS=-all-static
    make curl_LDFLAGS=-all-static install
    popd
}

build_prometheus_cpp() {
    pushd srcs/prometheus-cpp

    # fetch third-party dependencies
    git submodule init
    git submodule update    

    mkdir -p build

    PKG_CONFIG_PATH="/opt/bb/lib64/pkgconfig:$(pkg-config --variable pc_path pkg-config)" \
    cmake -DBUILD_SHARED_LIBS=OFF \
            -DENABLE_PUSH=ON \
            -DENABLE_COMPRESSION=OFF \
            -DENABLE_TESTING=OFF \
            -DGENERATE_PKGCONFIG=ON \
            -DCMAKE_INSTALL_PREFIX=/opt/bb \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
            -DCMAKE_INSTALL_LIBDIR=lib64 \
            -B build
    cmake --build build --parallel 16
    cmake --install build --prefix /opt/bb
    popd
}

build() {
    build_curl
    build_prometheus_cpp
}

fetch_deps
build
