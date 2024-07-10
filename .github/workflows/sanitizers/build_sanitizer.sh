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

# Set some initial constants
PARALLELISM=8

DIR_ROOT="${PWD}"
DIR_SCRIPTS="${DIR_ROOT}/.github/workflows/sanitizers"
DIR_EXTERNAL="${DIR_ROOT}/deps"
DIR_SRCS_EXT="${DIR_EXTERNAL}/srcs"
DIR_BUILD_EXT="${DIR_EXTERNAL}/cmake.bld"

DIR_SRC_BMQ="${DIR_ROOT}"
DIR_BUILD_BMQ="${DIR_SRC_BMQ}/cmake.bld/Linux"

# Parse sanitizers config
cfgquery() {
    jq "${1}" "${DIR_SCRIPTS}/sanitizers.json" --raw-output
}
LLVM_SANITIZER_NAME="$(cfgquery .${SANITIZER_NAME}.llvm_sanitizer_name)"
# Check if llvm specific cmake options are present for the given sanitizer
LLVM_SPECIFIC_CMAKE_OPTIONS="$(cfgquery .${SANITIZER_NAME}.llvm_specific_cmake_options)"
if [[ "$LLVM_SPECIFIC_CMAKE_OPTIONS" == null ]]; then LLVM_SPECIFIC_CMAKE_OPTIONS=""; fi

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

# Variables read by our custom CMake toolchain used to build everything else.
export LIBCXX_BUILD_PATH="$(realpath ${LIBCXX_BUILD_PATH})"
export DIR_SRC_BMQ="${DIR_SRC_BMQ}"
export DIR_SCRIPTS="${DIR_SCRIPTS}"


#################################################
echo #################################################
# sudo update-alternatives --all
echo #################################################

# sudo update-alternatives --remove-all gcc
# sudo update-alternatives --remove-all llvm
sudo update-alternatives --remove-all clang

# sudo ln -sf /usr/bin/clang-${LLVM_VERSION} /usr/bin/clang
# sudo ln -sf /usr/bin/clang++-${LLVM_VERSION} /usr/bin/clang++ 
# sudo update-alternatives \
#   --install /usr/bin/clang                 clang                  /usr/bin/clang-18     100 \
#   --slave   /usr/bin/clang++               clang++                /usr/bin/clang++-18 \
#   --slave   /usr/bin/lld                   lld                    /usr/bin/lld-18 \


function register_clang_version {
    local version=$1
    local priority=$2

    update-alternatives \
         --verbose \
        --install /usr/bin/llvm-config          llvm-config          /usr/bin/llvm-config-${version} ${priority} \
        --slave   /usr/bin/llvm-ar              llvm-ar              /usr/bin/llvm-ar-${version} \
        --slave   /usr/bin/llvm-as              llvm-as              /usr/bin/llvm-as-${version} \
        --slave   /usr/bin/llvm-bcanalyzer      llvm-bcanalyzer      /usr/bin/llvm-bcanalyzer-${version} \
        --slave   /usr/bin/llvm-c-test          llvm-c-test          /usr/bin/llvm-c-test-${version} \
        --slave   /usr/bin/llvm-cat             llvm-cat             /usr/bin/llvm-cat-${version} \
        --slave   /usr/bin/llvm-cfi-verify      llvm-cfi-verify      /usr/bin/llvm-cfi-verify-${version} \
        --slave   /usr/bin/llvm-cov             llvm-cov             /usr/bin/llvm-cov-${version} \
        --slave   /usr/bin/llvm-cvtres          llvm-cvtres          /usr/bin/llvm-cvtres-${version} \
        --slave   /usr/bin/llvm-cxxdump         llvm-cxxdump         /usr/bin/llvm-cxxdump-${version} \
        --slave   /usr/bin/llvm-cxxfilt         llvm-cxxfilt         /usr/bin/llvm-cxxfilt-${version} \
        --slave   /usr/bin/llvm-diff            llvm-diff            /usr/bin/llvm-diff-${version} \
        --slave   /usr/bin/llvm-dis             llvm-dis             /usr/bin/llvm-dis-${version} \
        --slave   /usr/bin/llvm-dlltool         llvm-dlltool         /usr/bin/llvm-dlltool-${version} \
        --slave   /usr/bin/llvm-dwarfdump       llvm-dwarfdump       /usr/bin/llvm-dwarfdump-${version} \
        --slave   /usr/bin/llvm-dwp             llvm-dwp             /usr/bin/llvm-dwp-${version} \
        --slave   /usr/bin/llvm-exegesis        llvm-exegesis        /usr/bin/llvm-exegesis-${version} \
        --slave   /usr/bin/llvm-extract         llvm-extract         /usr/bin/llvm-extract-${version} \
        --slave   /usr/bin/llvm-lib             llvm-lib             /usr/bin/llvm-lib-${version} \
        --slave   /usr/bin/llvm-link            llvm-link            /usr/bin/llvm-link-${version} \
        --slave   /usr/bin/llvm-lto             llvm-lto             /usr/bin/llvm-lto-${version} \
        --slave   /usr/bin/llvm-lto2            llvm-lto2            /usr/bin/llvm-lto2-${version} \
        --slave   /usr/bin/llvm-mc              llvm-mc              /usr/bin/llvm-mc-${version} \
        --slave   /usr/bin/llvm-mca             llvm-mca             /usr/bin/llvm-mca-${version} \
        --slave   /usr/bin/llvm-modextract      llvm-modextract      /usr/bin/llvm-modextract-${version} \
        --slave   /usr/bin/llvm-mt              llvm-mt              /usr/bin/llvm-mt-${version} \
        --slave   /usr/bin/llvm-nm              llvm-nm              /usr/bin/llvm-nm-${version} \
        --slave   /usr/bin/llvm-objcopy         llvm-objcopy         /usr/bin/llvm-objcopy-${version} \
        --slave   /usr/bin/llvm-objdump         llvm-objdump         /usr/bin/llvm-objdump-${version} \
        --slave   /usr/bin/llvm-opt-report      llvm-opt-report      /usr/bin/llvm-opt-report-${version} \
        --slave   /usr/bin/llvm-pdbutil         llvm-pdbutil         /usr/bin/llvm-pdbutil-${version} \
        --slave   /usr/bin/llvm-PerfectShuffle  llvm-PerfectShuffle  /usr/bin/llvm-PerfectShuffle-${version} \
        --slave   /usr/bin/llvm-profdata        llvm-profdata        /usr/bin/llvm-profdata-${version} \
        --slave   /usr/bin/llvm-ranlib          llvm-ranlib          /usr/bin/llvm-ranlib-${version} \
        --slave   /usr/bin/llvm-rc              llvm-rc              /usr/bin/llvm-rc-${version} \
        --slave   /usr/bin/llvm-readelf         llvm-readelf         /usr/bin/llvm-readelf-${version} \
        --slave   /usr/bin/llvm-readobj         llvm-readobj         /usr/bin/llvm-readobj-${version} \
        --slave   /usr/bin/llvm-rtdyld          llvm-rtdyld          /usr/bin/llvm-rtdyld-${version} \
        --slave   /usr/bin/llvm-size            llvm-size            /usr/bin/llvm-size-${version} \
        --slave   /usr/bin/llvm-split           llvm-split           /usr/bin/llvm-split-${version} \
        --slave   /usr/bin/llvm-stress          llvm-stress          /usr/bin/llvm-stress-${version} \
        --slave   /usr/bin/llvm-strings         llvm-strings         /usr/bin/llvm-strings-${version} \
        --slave   /usr/bin/llvm-strip           llvm-strip           /usr/bin/llvm-strip-${version} \
        --slave   /usr/bin/llvm-symbolizer      llvm-symbolizer      /usr/bin/llvm-symbolizer-${version} \
        --slave   /usr/bin/llvm-tblgen          llvm-tblgen          /usr/bin/llvm-tblgen-${version} \
        --slave   /usr/bin/llvm-undname         llvm-undname         /usr/bin/llvm-undname-${version} \
        --slave   /usr/bin/llvm-xray            llvm-xray            /usr/bin/llvm-xray-${version}
        

    update-alternatives \
         --verbose \
        --install /usr/bin/clang                clang                /usr/bin/clang-${version} ${priority} \
        --slave   /usr/bin/clang++              clang++              /usr/bin/clang++-${version}  \
        --slave   /usr/bin/clang-format         clang-format         /usr/bin/clang-format-${version}  \
        --slave   /usr/bin/clang-cpp            clang-cpp            /usr/bin/clang-cpp-${version} \
        --slave   /usr/bin/clang-cl             clang-cl             /usr/bin/clang-cl-${version} \
        --slave   /usr/bin/clangd               clangd               /usr/bin/clangd-${version} \
        --slave   /usr/bin/clang-tidy           clang-tidy           /usr/bin/clang-tidy-${version} \
        --slave   /usr/bin/clang-check          clang-check          /usr/bin/clang-check-${version} \
        --slave   /usr/bin/clang-query          clang-query          /usr/bin/clang-query-${version} \
        --slave   /usr/bin/asan_symbolize       asan_symbolize       /usr/bin/asan_symbolize-${version} \
        --slave   /usr/bin/bugpoint             bugpoint             /usr/bin/bugpoint-${version} \
        --slave   /usr/bin/dsymutil             dsymutil             /usr/bin/dsymutil-${version} \
        --slave   /usr/bin/lld                  lld                  /usr/bin/lld-${version} \
        --slave   /usr/bin/ld.lld               ld.lld               /usr/bin/ld.lld-${version} \
        --slave   /usr/bin/lld-link             lld-link             /usr/bin/lld-link-${version} \
        --slave   /usr/bin/llc                  llc                  /usr/bin/llc-${version} \
        --slave   /usr/bin/lli                  lli                  /usr/bin/lli-${version} \
        --slave   /usr/bin/obj2yaml             obj2yaml             /usr/bin/obj2yaml-${version} \
        --slave   /usr/bin/opt                  opt                  /usr/bin/opt-${version} \
        --slave   /usr/bin/sanstats             sanstats             /usr/bin/sanstats-${version} \
        --slave   /usr/bin/verify-uselistorder  verify-uselistorder  /usr/bin/verify-uselistorder-${version} \
        --slave   /usr/bin/wasm-ld              wasm-ld              /usr/bin/wasm-ld-${version} \
        --slave   /usr/bin/yaml2obj             yaml2obj             /usr/bin/yaml2obj-${version}
        
}

register_clang_version 18 100


echo #################################################
# sudo update-alternatives --all
sudo update-alternatives --config clang
echo #################################################
#################################################

# :: Build BDE + NTF ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
TOOLCHAIN_PATH="${DIR_SCRIPTS}/clang-libcxx-${SANITIZER_NAME}.cmake"
export CC="clang"
export CXX="clang++"
export BBS_BUILD_SYSTEM="ON"
PATH="$PATH:$(realpath ${DIR_SRCS_EXT}/bde-tools/bin)"
export PATH

pushd ${DIR_SRCS_EXT}

pushd "bde"
eval "$(bbs_build_env -u dbg_64_safe_cpp20 -b "${DIR_BUILD_EXT}/bde")"
bbs_build configure --toolchain "${TOOLCHAIN_PATH}"

#################################################
exit 0
#################################################

bbs_build build -j${PARALLELISM}
bbs_build --install=/opt/bb --prefix=/ install
popd

pushd "ntf-core"
# TODO The deprecated flag "-fcoroutines-ts" has been removed in clang
# 17.0.1, but NTF is still using it.  We manually change this flag until
# the fix in issue 175307231 is resolved.
sed -i 's/fcoroutines-ts/fcoroutines/g' 'repository.cmake'

./configure --keep \
            --prefix /opt/bb             \
            --output "${DIR_BUILD_EXT}/ntf" \
            --without-warnings-as-errors \
            --without-usage-examples \
            --without-applications \
            --ufid 'dbg_64_safe_cpp20' \
            --toolchain "${TOOLCHAIN_PATH}"
make -j${PARALLELISM}
make install
popd
