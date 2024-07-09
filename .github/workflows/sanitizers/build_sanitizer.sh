#!/usr/bin/env bash

# This script can be used to build BlazingMQ, and all of its transitive
# dependencies (up to and including the standard library) using:
#   - Clang
#   - LLVM libc++ standard library
#   - A CMake toolchain file specific for instrumented build
# It is currently used to build instrumented BlazingMQ binaries for CI for all
# Clang sanitizers (i.e. Address/Leak, Memory, Thread, UndefinedBehavior).
#
# It performs the following:
# 1) Install clang compiler.
# 2) Download llvm-project required for libc++ instrumentation.
# 3) Download external dependencies required for instrumentation.
# 4) Build libc++ with the instrumentation specified by <LLVM Sanitizer Name>.
# 5) Build sanitizer-instrumented dependencies including BDE, NTF, GoogleTest,
#    Google Benchmark and zlib.
# 6) Build sanitizer-instrumented BlazingMQ unit tests.
# 7) Generate scripts to run unit tests:
#      ./cmake.bld/Linux/run-unittests.sh
#    This script is used as-is by CI to run unit tests with sanitizer.

set -eux

# :: Required arguments :::::::::::::::::::::::::::::::::::::::::::::::::::::::
if [ -z ${1} ]; then
    echo 'Error: Missing sanitizer name.' >&2
    echo '  (Usage: build_sanitizer.sh <sanitizer-name>)' >&2
    exit 1
fi

SANITIZER_NAME="${1}"

echo SANITIZER_NAME: "${SANITIZER_NAME}"
echo ROOT: "${PWD}"

# Install prerequisites
sudo apt-get update && sudo apt-get install -qy lsb-release wget software-properties-common gnupg git curl jq ninja-build bison libfl-dev pkg-config

# Prerequisites for LLVM installation: latest cmake version, Ubuntu apt repository contains cmake version 3.22.1
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
apt-add-repository -y "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
sudo apt-get update && sudo apt-get install -qy cmake

cmake --version
