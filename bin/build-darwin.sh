#!/usr/bin/env bash

# This script builds BlazingMQ and all of its dependencies.
#
# Required tools:
# - Clang
# - git
# - Homebrew
# - Perl
# - Python3
#
# Options:
#   -h, --help          Show this help and exit
#       --install-deps  Install required Homebrew packages before building

set -e
set -u
[ -z "$BASH" ] || shopt -s expand_aliases

print_help() {
    cat <<'USAGE'
BlazingMQ Build Script

Usage:
  bin/$(basename "$0") [OPTIONS]

Options:
  -h, --help          Show this help and exit
      --install-deps  Install required Homebrew packages via Homebrew

What it does:
  • Optionally installs prerequisites using Homebrew:
      brew install cmake flex bison google-benchmark googletest ninja pkg-config zlib
  • Clones third-party deps (bde-tools, bde, ntf-core)
  • Builds and installs BDE and NTF
  • Configures and builds BlazingMQ

Notes:
  • Run this script from the root of the BlazingMQ repository.
USAGE
}

INSTALL_DEPS=false

# Parse args (simple, portable long-option handling)
for arg in "$@"; do
    case "$arg" in
        -h|--help)
            print_help
            exit 0
            ;;
        --install-deps)
            INSTALL_DEPS=true
            ;;
        *)
            echo "Unknown option: $arg" >&2
            echo "Try '--help' for usage." >&2
            exit 2
            ;;
    esac
done

script_path="bin/$(basename "$0")"

if [ ! -f "$script_path" ] || [ "$(realpath "$0")" != "$(realpath "$script_path")" ]; then
    echo 'This script must be run from the root of the BlazingMQ repository.'
    exit 1
fi

# :: Optionally install prerequisites :::::::::::::::::::::::::::::::::::::::::

REQ_PKGS=(cmake flex bison google-benchmark googletest ninja pkg-config zlib)

if $INSTALL_DEPS; then
    if ! command -v brew >/dev/null 2>&1; then
        echo "Homebrew is required for '--install-deps' but was not found on PATH." >&2
        echo "Install Homebrew from https://brew.sh/ and re-run with --install-deps." >&2
        exit 1
    fi
    echo "Installing prerequisites via Homebrew..."
    brew install "${REQ_PKGS[@]}"
    echo "Prerequisites installed."
fi

# :: Set some initial constants :::::::::::::::::::::::::::::::::::::::::::::::
DIR_ROOT="$(pwd)"

DIR_THIRDPARTY="${DIR_ROOT}/thirdparty"
mkdir -p "${DIR_THIRDPARTY}"

DIR_BUILD="${DIR_BUILD:-${DIR_ROOT}/build}"
mkdir -p "${DIR_BUILD}"

DIR_INSTALL="${DIR_INSTALL:-${DIR_ROOT}}"
mkdir -p "${DIR_INSTALL}"


# :: Clone dependencies :::::::::::::::::::::::::::::::::::::::::::::::::::::::
if [ ! -d "${DIR_THIRDPARTY}/bde-tools" ]; then
    git clone --depth 1 --branch 4.23.0.0 https://github.com/bloomberg/bde-tools "${DIR_THIRDPARTY}/bde-tools"
fi
if [ ! -d "${DIR_THIRDPARTY}/bde" ]; then
    git clone --depth 1 --branch 4.23.0.0 https://github.com/bloomberg/bde.git "${DIR_THIRDPARTY}/bde"
fi
if [ ! -d "${DIR_THIRDPARTY}/ntf-core" ]; then
    git clone --depth 1 --branch 2.4.2 https://github.com/bloomberg/ntf-core.git "${DIR_THIRDPARTY}/ntf-core"
fi


# :: Install required packages ::::::::::::::::::::::::::::::::::::::::::::::::

# Build and install BDE
# Refer to https://bloomberg.github.io/bde/library_information/build.html
PATH="${DIR_THIRDPARTY}/bde-tools/bin:$PATH"

if [ ! -e "${DIR_BUILD}/bde/.complete" ]; then
    pushd "${DIR_THIRDPARTY}/bde"
    eval "$(bbs_build_env -u opt_64_cpp17 -b "${DIR_BUILD}/bde" -i "${DIR_INSTALL}")"
    bbs_build configure --prefix="${DIR_INSTALL}"
    bbs_build build --prefix="${DIR_INSTALL}"
    bbs_build install --install_dir="/" --prefix="${DIR_INSTALL}"
    eval "$(bbs_build_env unset)"
    popd
    touch "${DIR_BUILD}/bde/.complete"
fi

if [ ! -e "${DIR_BUILD}/ntf/.complete" ]; then
    # Build and install NTF
    pushd "${DIR_THIRDPARTY}/ntf-core"
    ./configure --prefix "${DIR_INSTALL}" \
                --output "${DIR_BUILD}/ntf" \
                --without-warnings-as-errors \
                --without-usage-examples \
                --without-applications \
                --ufid opt_64_cpp17
    make -j 16
    make install
    popd
    touch "${DIR_BUILD}/ntf/.complete"
fi

# Determine paths based on Intel vs Apple Silicon CPU
if [ "$(uname -p)" == 'arm' ]; then
    BREW_PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/opt/homebrew/opt/zlib/lib/pkgconfig:/opt/homebrew/opt/googletest/lib/pkgconfig"
    FLEX_ROOT="/opt/homebrew/opt/flex"
else
    BREW_PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/opt/zlib/lib/pkgconfig:/usr/local/opt/googletest/lib/pkgconfig"
    FLEX_ROOT="/usr/local/opt/flex"
fi


# :: Build the BlazingMQ repo :::::::::::::::::::::::::::::::::::::::::::::::::
CMAKE_OPTIONS=(\
    -DBDE_BUILD_TARGET_64=1 \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_LIBDIR="lib" \
    -DCMAKE_INSTALL_PREFIX="${DIR_INSTALL}" \
    -DCMAKE_MODULE_PATH="${DIR_THIRDPARTY}/bde-tools/cmake;${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem" \
    -DCMAKE_PREFIX_PATH="${DIR_INSTALL}" \
    -DCMAKE_TOOLCHAIN_FILE="${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem/toolchains/darwin/clang-default.cmake" \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DFLEX_ROOT="${FLEX_ROOT}")

PKG_CONFIG_PATH="${DIR_INSTALL}/lib/pkgconfig:${BREW_PKG_CONFIG_PATH}" \
cmake -B "${DIR_BUILD}/blazingmq" -S "${DIR_ROOT}" "${CMAKE_OPTIONS[@]}"
cmake --build "${DIR_BUILD}/blazingmq" --parallel 16 --target bmqbrkr bmqtool all.it bmqauthnpass bmqauthnfail

echo broker is here: "${DIR_BUILD}/blazingmq/src/applications/bmqbrkr/bmqbrkr.tsk"
echo to run the broker: "${DIR_BUILD}/blazingmq/src/applications/bmqbrkr/run"
echo tool is here: "${DIR_BUILD}/blazingmq/src/applications/bmqtool/bmqtool.tsk"

SELF_HOST_ADDRESS="127.0.0.1       $(hostname)"
if ! (grep -q "$SELF_HOST_ADDRESS" /etc/hosts > /dev/null);
then
    echo "Warning: self hostname $(hostname) not found in /etc/hosts"
    echo "It might be necessary to add it manually to launch bmqbrkr:"
    echo "sudo bash -c \"echo \\\"$SELF_HOST_ADDRESS\\\" >> /etc/hosts\""
fi
