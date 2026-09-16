#!/usr/bin/env bash
# Copyright 2026 Bloomberg Finance L.P.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script installs everything needed to build BlazingMQ and its
# dependencies with Clang sanitizer instrumentation, but which does not
# depend on the chosen sanitizer:
# 1) Build prerequisites from apt, including CMake.
# 2) The Clang/LLVM toolchain.
# 3) The llvm-project sources, needed later to build an instrumented libc++.
# 4) A Python venv with the packages needed to run integration tests.
#
# The result is identical for every sanitizer, so it can be produced once and
# reused by all sanitizer builds.
#
# The script takes no arguments.
# Usage example:
#    install_common.sh

set -eux

LLVM_VERSION=21
LLVM_TAG="llvmorg-21.1.7"

DIR_ROOT="${PWD}"
DIR_SRCS_EXT="${DIR_ROOT}/deps/srcs"

# Install prerequisites
# Set up CA certificates first before installing other dependencies
apt-get update
apt-get install -y ca-certificates
apt-get install -qy --no-install-recommends \
    lsb-release \
    wget \
    software-properties-common \
    gnupg \
    git \
    curl \
    jq \
    cmake \
    make \
    ninja-build \
    bison \
    libfl-dev \
    pkg-config \
    python3 \
    python3-venv
rm -rf /var/lib/apt/lists/*

# First set up app sources and install minimal LLVM: clang, lld, lldb and clangd.
# Downloads default to IPv4 to avoid possible problems with IPv6 support.
wget -4 https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
./llvm.sh ${LLVM_VERSION}
rm -f llvm.sh

# Secondly, install extra required packages: llvm and libclang-rt.
# - `llvm` provides llvm-symbolizer, used by every sanitizer runtime.
# - `libclang-rt` provides the sanitizer runtimes and libFuzzer.
apt-get install -qy --no-install-recommends \
    "llvm-${LLVM_VERSION}" \
    "libclang-rt-${LLVM_VERSION}-dev"
rm -rf /var/lib/apt/lists/*

# Create version-agnostic pointers to required LLVM binaries.
ln -sf /usr/bin/clang-${LLVM_VERSION} /usr/bin/clang
ln -sf /usr/bin/clang++-${LLVM_VERSION} /usr/bin/clang++
ln -sf /usr/bin/llvm-symbolizer-${LLVM_VERSION} /usr/bin/llvm-symbolizer

# Download LLVM sources, required to build an instrumented libc++.
mkdir -p "${DIR_SRCS_EXT}"
curl -4 -SL "https://github.com/llvm/llvm-project/archive/refs/tags/${LLVM_TAG}.tar.gz" \
    | tar -xzC "${DIR_SRCS_EXT}"
mv "${DIR_SRCS_EXT}/llvm-project-${LLVM_TAG}" "${DIR_SRCS_EXT}/llvm-project"

# Create Python venv
python3 -m venv venv
if [ ! -f "venv/bin/activate" ]; then
    echo "Virtual environment not found." >&2
    exit 1
fi
# shellcheck disable=SC1091
source venv/bin/activate
pip install -r "${DIR_ROOT}/src/python/requirements-test.txt"
