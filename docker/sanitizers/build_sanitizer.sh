#!/usr/bin/env bash

# This script can be used to build BlazingMQ, and all of its transitive
# dependencies (up to and including the standard library) using:
#   - Clang
#   - LLVM libc++ standard library
#   - A CMake toolchain file specific for instrumented build
# It is used to build instrumented BlazingMQ binaries for all
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

set -eux

# Check required arguments
if [ -z "${1}" ]; then
    echo 'Error: Missing sanitizer name.' >&2
    echo '  (Usage: build_sanitizer.sh <sanitizer-name> <fuzzer>)' >&2
    exit 1
fi

if [[ -z "${2}" || ("${2}" != "on" && "${2}" != "off") ]]; then
    echo 'Error: Wrong fuzzer argument. It can be either "on" or "off"' >&2
    echo '  (Usage: build_sanitizer.sh <sanitizer-name> <fuzzer>)' >&2
    exit 1
fi

SANITIZER_NAME="${1}"
FUZZER="${2}"

# Install prerequisites
# Set up CA certificates first before installing other dependencies
apt-get update && \
apt-get install -y ca-certificates && \
apt-get install -qy --no-install-recommends \
    lsb-release \
    wget \
    software-properties-common \
    gnupg \
    git \
    curl \
    jq \
    ninja-build \
    bison \
    libfl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install prerequisites for LLVM: latest cmake version, Ubuntu apt repository contains stale version
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
        | gpg --dearmor - \
        | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
apt-add-repository -y "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
apt-get install -qy cmake

# Install LLVM
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh 
LLVM_VERSION=18
LLVM_TAG="llvmorg-18.1.8"
./llvm.sh ${LLVM_VERSION} all

# Create version-agnostic pointers to required LLVM binaries.
ln -sf /usr/bin/clang-${LLVM_VERSION} /usr/bin/clang
ln -sf /usr/bin/clang++-${LLVM_VERSION} /usr/bin/clang++ 
ln -sf /usr/bin/llvm-symbolizer-${LLVM_VERSION} /usr/bin/llvm-symbolizer

# Set some initial constants
PARALLELISM=8

DIR_ROOT="${PWD}"
DIR_SCRIPTS="${DIR_ROOT}/docker/sanitizers"
DIR_EXTERNAL="${DIR_ROOT}/deps"
DIR_SRCS_EXT="${DIR_EXTERNAL}/srcs"
DIR_BUILD_EXT="${DIR_EXTERNAL}/cmake.bld"

DIR_SRC_BMQ="${DIR_ROOT}"
DIR_BUILD_BMQ="${DIR_SRC_BMQ}/cmake.bld/Linux"

# Parse sanitizers config
cfgquery() {
    jq "${1}" "${DIR_SCRIPTS}/sanitizers.json" --raw-output
}
LLVM_SANITIZER_NAME="$(cfgquery ."${SANITIZER_NAME}".llvm_sanitizer_name)"
# Check if llvm specific cmake options are present for the given sanitizer
LLVM_SPECIFIC_CMAKE_OPTIONS="$(cfgquery ."${SANITIZER_NAME}".llvm_specific_cmake_options)"
if [[ "$LLVM_SPECIFIC_CMAKE_OPTIONS" == null ]]; then LLVM_SPECIFIC_CMAKE_OPTIONS=""; fi

checkoutGitRepo() {
    local repo=$1
    local ref=$2
    local repoDir=$3
    echo "Checking out ${repo} at ${ref}"

    local repoPath="${DIR_SRCS_EXT}/${repoDir}"

    git clone -b "${ref}" "${repo}" \
        --depth 1 --single-branch --no-tags -c advice.detachedHead=false "${repoPath}"
}
github_url() { echo "https://github.com/$1.git"; }

# Download external dependencies
mkdir -p "${DIR_SRCS_EXT}"

# Download LLVM sources
curl -SL "https://github.com/llvm/llvm-project/archive/refs/tags/${LLVM_TAG}.tar.gz" \
    | tar -xzC "${DIR_SRCS_EXT}"
mv "${DIR_SRCS_EXT}/llvm-project-${LLVM_TAG}" "${DIR_SRCS_EXT}/llvm-project"

# Download google-benchmark sources
GOOGLE_BENCHMARK_TAG="v1.9.1"
checkoutGitRepo "$(github_url google/benchmark)" "${GOOGLE_BENCHMARK_TAG}" "google-benchmark"

# Download googletest sources
GOOGLETEST_TAG="v1.15.2"
checkoutGitRepo "$(github_url google/googletest)" "${GOOGLETEST_TAG}" "googletest"

# Download zlib
ZLIB_TAG="v1.3.1"
checkoutGitRepo "$(github_url madler/zlib)" "${ZLIB_TAG}" "zlib"

# Download bde-tools, bde and ntf-core sources
cd "${DIR_EXTERNAL}"
"${DIR_ROOT}"/docker/build_deps.sh "--only-download"
cd -

# Build libc++ with required instrumentation
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
if [ "${FUZZER}" == "off" ]; then
    LIBCXX_SRC_PATH="${DIR_SRCS_EXT}/llvm-project/runtimes"
    LIBCXX_BUILD_PATH="${LIBCXX_SRC_PATH}/cmake.bld"

    cmake   -B "${LIBCXX_BUILD_PATH}" \
            -S "${LIBCXX_SRC_PATH}" \
            -DCMAKE_BUILD_TYPE="Debug" \
            -DCMAKE_C_COMPILER="clang" \
            -DCMAKE_CXX_COMPILER="clang++" \
            -DCMAKE_CXX_STANDARD=20 \
            -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
            -DLLVM_USE_SANITIZER="${LLVM_SANITIZER_NAME}" \
            "${LLVM_SPECIFIC_CMAKE_OPTIONS}"

    cmake --build "${LIBCXX_BUILD_PATH}" -j${PARALLELISM} --target cxx cxxabi unwind generate-cxx-headers

    export LIBCXX_BUILD_PATH="${LIBCXX_BUILD_PATH}"
fi

# Variables read by our custom CMake toolchain used to build everything else.
export DIR_SRC_BMQ="${DIR_SRC_BMQ}"
export DIR_SCRIPTS="${DIR_SCRIPTS}"

TOOLCHAIN_PATH="${DIR_SCRIPTS}/clang-libcxx-sanitizer.cmake"
export SANITIZER_NAME="${SANITIZER_NAME}"
export CC="clang"
export CXX="clang++"
if [ "${FUZZER}" == "on" ]; then
  export FUZZER_FLAG="fuzzer-no-link"
else
  export CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES="/usr/include;/usr/include/clang/${LLVM_VERSION}/include"
fi
export BBS_BUILD_SYSTEM="ON"
PATH="$PATH:$(realpath "${DIR_SRCS_EXT}"/bde-tools/bin)"
export PATH

# Build BDE + NTF
pushd "${DIR_SRCS_EXT}/bde"
eval "$(bbs_build_env -u dbg_64_safe_cpp20 -b "${DIR_BUILD_EXT}/bde")"
bbs_build configure --toolchain "${TOOLCHAIN_PATH}"
bbs_build build -j${PARALLELISM}
bbs_build --install=/opt/bb --prefix=/ install
popd

pushd "${DIR_SRCS_EXT}/ntf-core"
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

# Note: Hack to circumvent faulty behavior in "nts-targets.cmake"
ln -sf "/opt/bb/include" "/opt/include"
ln -sf "/opt/bb/lib64" "/opt/lib64"

# Setup CMake options for all remaining builds
CMAKE_OPTIONS=( \
    -D BUILD_BITNESS=64 \
    -D CMAKE_BUILD_TYPE=Debug \
    -D CMAKE_INSTALL_INCLUDEDIR=include \
    -D CMAKE_INSTALL_LIBDIR=lib64 \
    -D CMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_PATH}")

# Build GoogleTest
cmake -B "${DIR_SRCS_EXT}/googletest/cmake.bld" \
      -S "${DIR_SRCS_EXT}/googletest" "${CMAKE_OPTIONS[@]}" \
      -DCMAKE_INSTALL_PREFIX=/opt/bb
cmake --build "${DIR_SRCS_EXT}/googletest/cmake.bld" -j${PARALLELISM}
cmake --install "${DIR_SRCS_EXT}/googletest/cmake.bld" --prefix "/opt/bb"

# Build Google Benchmark
cmake -B "${DIR_SRCS_EXT}/google-benchmark/cmake.bld" \
        -S "${DIR_SRCS_EXT}/google-benchmark" "${CMAKE_OPTIONS[@]}" \
        -DCMAKE_INSTALL_PREFIX=/opt/bb \
        -DBENCHMARK_DOWNLOAD_DEPENDENCIES="ON" \
        -DBENCHMARK_ENABLE_GTEST_TESTS="false" \
        -DHAVE_STD_REGEX="ON" \
        -DBENCHMARK_ENABLE_TESTING="OFF"
cmake --build "${DIR_SRCS_EXT}/google-benchmark/cmake.bld" -j${PARALLELISM}
cmake --install "${DIR_SRCS_EXT}/google-benchmark/cmake.bld" --prefix "/opt/bb"

# Build zlib
# Note: zlib has completely broken CMake install rules, so we must
# specify the install prefix *exactly* as it will be at configure
# time
# https://discourse.cmake.org/t/cmake-install-prefix-not-work/5040
cmake -B "${DIR_SRCS_EXT}/zlib/cmake.bld" -S "${DIR_SRCS_EXT}/zlib" \
        -D CMAKE_INSTALL_PREFIX="/opt/bb" \
        "${CMAKE_OPTIONS[@]}" \
        -DZLIB_BUILD_TESTING=OFF \
        -DZLIB_BUILD_SHARED=OFF \
        -DZLIB_BUILD_STATIC=ON \
        -DZLIB_BUILD_MINIZIP=OFF
# Make and install zlib.
cmake --build "${DIR_SRCS_EXT}/zlib/cmake.bld" -j${PARALLELISM}
cmake --install "${DIR_SRCS_EXT}/zlib/cmake.bld"

# Remove un-needed folders
rm -rf "${DIR_BUILD_EXT}"
for dir in "${DIR_SRCS_EXT}"/*; do
    [[ "$dir" == "${DIR_SRCS_EXT}/bde-tools" || "$dir" == "${DIR_SRCS_EXT}/llvm-project" ]] && continue
    rm -rf "$dir"
done

# Build BlazingMQ
if [ "${FUZZER}" == "on" ]; then
    export FUZZER_FLAG="fuzzer"
    CMAKE_OPTIONS+=(-DINSTALL_TARGETS=fuzztests);
    TARGETS="fuzztests"
else
    CMAKE_OPTIONS+=(-UINSTALL_TARGETS);
    TARGETS="all.t"
fi
PKG_CONFIG_PATH="/opt/bb/lib64/pkgconfig:/opt/bb/lib/pkgconfig:/opt/bb/share/pkgconfig:$(pkg-config --variable pc_path pkg-config)" \
cmake --preset fuzz-tests -B "${DIR_BUILD_BMQ}" -S "${DIR_SRC_BMQ}" -G Ninja \
    -DCMAKE_PREFIX_PATH="${DIR_SRCS_EXT}/bde-tools/BdeBuildSystem" \
    "${CMAKE_OPTIONS[@]}"
cmake --build "${DIR_BUILD_BMQ}" -j${PARALLELISM} \
      --target ${TARGETS} -v --clean-first

if [ "${FUZZER}" == "on" ]; then
    exit 0
fi

# Create testing script
envcfgquery() {
    # Parses the '<build-name>.environment' object from 'sanitizers.json',
    # and outputs a string of whitespace-separated 'VAR=VAL' pairs intended to
    # be used to set the environment for a command.
    #   e.g. 'asan' -> 'ASAN_OPTIONS="foo=bar:baz=baf" LSAN_OPTIONS="abc=fgh"'
    #
    cfgquery "                           \
        .${1}.environment |              \
        to_entries |                     \
        map(\"\(.key)=\\\"\(.value |     \
            to_entries |                 \
            map(\"\(.key)=\(.value)\") | \
            join(\":\"))\\\"\") |        \
        join(\" \")" |
    sed "s|%%SRC%%|$(realpath "${DIR_SRC_BMQ}")|g" |
    sed "s|%%ROOT%%|$(realpath "${DIR_ROOT}")|g"
}

mkscript() {
    local cmd=${1}
    local outfile=${2}

    echo '#!/usr/bin/env bash' > "${outfile}"
    echo "${cmd}" >> "${outfile}"
    chmod +x "${outfile}"
}

SANITIZER_ENV="BMQ_BUILD=$(realpath "${DIR_BUILD_BMQ}") "
SANITIZER_ENV+="BMQ_REPO=${DIR_SRC_BMQ} "
SANITIZER_ENV+="$(envcfgquery "${SANITIZER_NAME}")"

# 'run-env.sh' runs a command with environment required of the sanitizer.
mkscript "${SANITIZER_ENV} \${@}" "${DIR_BUILD_BMQ}/run-env.sh"

# 'run-unittests.sh' runs all instrumented unit-tests.
CMD="cd $(realpath "${DIR_BUILD_BMQ}") && "
CMD+="./run-env.sh ctest --output-on-failure"
mkscript "${CMD}" "${DIR_BUILD_BMQ}/run-unittests.sh"
