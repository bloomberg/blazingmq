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

# Install prerequisites for LLVM: latest cmake version, Ubuntu apt repository contains cmake version 3.22.1
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository -y "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
sudo apt-get install -qy cmake

# Install LLVM
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh 
LLVM_VERSION=18
sudo ./llvm.sh ${LLVM_VERSION} all

# Create version-agnostic pointers to required LLVM binaries.
sudo ln -sf /usr/bin/clang-${LLVM_VERSION} /usr/bin/clang
sudo ln -sf /usr/bin/clang++-${LLVM_VERSION} /usr/bin/clang++ 
sudo ln -sf /usr/bin/llvm-symbolizer-${LLVM_VERSION} /usr/bin/llvm-symbolizer

# Parse sanitizers config
cfgquery() {
    jq "${1}" "./.github/workflows/sanitizers/sanitizers.json" --raw-output
}
LLVM_SANITIZER_NAME="$(cfgquery .${SANITIZER_NAME}.llvm_sanitizer_name)"
# Check if llvm specific cmake options are present for the given sanitizer
LLVM_SPECIFIC_CMAKE_OPTIONS="$(cfgquery .${SANITIZER_NAME}.llvm_specific_cmake_options)"
if [[ "$LLVM_SPECIFIC_CMAKE_OPTIONS" == null ]]; then LLVM_SPECIFIC_CMAKE_OPTIONS=""; fi

# Set some initial constants
PARALLELISM=8

DIR_ROOT="${PWD}"
DIR_EXTERNAL="${DIR_ROOT}/deps"
DIR_SRCS_EXT="${DIR_EXTERNAL}/srcs"
DIR_BUILD_EXT="${DIR_SRCS_EXT}/cmake.bld"

# :: checkoutGitRepo() subroutine :::::::::::::::::::::::::::::::::::::::::::::
checkoutGitRepo() {
    local repo=$1
    local ref=$2
    local repoDir=$3
    echo "Checking out ${repo} at ${ref}"

    local repoPath="${DIR_SRCS_EXT}/${repoDir}"

    git clone -b ${ref} ${repo} \
        --depth 1 --single-branch --no-tags -c advice.detachedHead=false "${repoPath}"
}
github_url() { echo "https://github.com/$1.git"; }

# :: Download external dependencies :::::::::::::::::::::::::::::::
mkdir -p ${DIR_SRCS_EXT}

# Download LLVM
LLVM_TAG="llvmorg-18.1.8"
checkoutGitRepo "$(github_url llvm/llvm-project)" "${LLVM_TAG}" "llvm-project"

# Download google-benchmark
GOOGLE_BENCHMARK_TAG="v1.8.4"
checkoutGitRepo "$(github_url google/benchmark)" "${GOOGLE_BENCHMARK_TAG}" "google-benchmark"

# Download googletest
GOOGLETEST_TAG="v1.14.0"
checkoutGitRepo "$(github_url google/googletest)" "${GOOGLETEST_TAG}" "googletest"

# Download zlib
ZLIB_TAG="v1.3.1"
checkoutGitRepo "$(github_url madler/zlib)" "${ZLIB_TAG}" "zlib"

# :: Build libc++ with required instrumentation :::::::::::::::::::::::::::::::
#
# The extent to which all dependencies to be compiled with sanitizer-support
# varies by sanitizer. MemorySanitizer is especially unforgiving: Failing to
# link against an instrumented standard library will yield many false
# positives.  Concensus is that compiling libc++ with `-fsanitize=memory` is a
# significantly easier endeavor than doing the same with libstdc++ (the gcc
# standard library).
#
# We therefore opt to use libc++ here, just to ensure maximum flexibility.  We
# follow build instructions from https://libcxx.llvm.org/BuildingLibcxx.html
LIBCXX_SRC_PATH="${DIR_SRCS_EXT}/llvm-project/runtimes"
LIBCXX_BUILD_PATH="${LIBCXX_SRC_PATH}/cmake.bld"

cmake   -B "${LIBCXX_BUILD_PATH}" \
        -S "${LIBCXX_SRC_PATH}" \
        -DCMAKE_BUILD_TYPE="Debug" \
        -DCMAKE_C_COMPILER="clang" \
        -DCMAKE_CXX_COMPILER="clang++" \
        -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
        -DLLVM_USE_SANITIZER="${LLVM_SANITIZER_NAME}" \
        ${LLVM_SPECIFIC_CMAKE_OPTIONS}

cmake --build "${LIBCXX_BUILD_PATH}" -j${PARALLELISM} --target cxx cxxabi unwind generate-cxx-headers
