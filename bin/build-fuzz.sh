#!/usr/bin/env bash

# :: Set some initial constants :::::::::::::::::::::::::::::::::::::::::::::::

DIR_ROOT="$(pwd)"

DIR_THIRDPARTY="${DIR_ROOT}/thirdparty"
mkdir -p "${DIR_THIRDPARTY}"

DIR_BUILD="${DIR_BUILD:-${DIR_ROOT}/build}"
mkdir -p "${DIR_BUILD}"

DIR_INSTALL="${DIR_INSTALL:-${DIR_ROOT}}"
mkdir -p "${DIR_INSTALL}"

DIR_SCRIPTS="${DIR_ROOT}/docker/sanitizers"

TOOLCHAIN_PATH="${DIR_SCRIPTS}/clang-libcxx-sanitizer.cmake"

# :: Set some environment variables :::::::::::::::::::::::::::::::::::::::::::

export CC="clang"
export CXX="clang++"
export DIR_SCRIPTS="${DIR_SCRIPTS}"
export SANITIZER_NAME="asan"
export FUZZER="fuzzer-no-link"

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
    pushd "${DIR_THIRDPARTY}/bde" || exit
    eval "$(bbs_build_env -p clang -u dbg_64_safe_cpp20 -b "${DIR_BUILD}/bde" -i "${DIR_INSTALL}")"
    bbs_build configure --prefix="${DIR_INSTALL}" \
                        --toolchain "${TOOLCHAIN_PATH}"
    bbs_build build --prefix="${DIR_INSTALL}"
    bbs_build install --install_dir="/" --prefix="${DIR_INSTALL}"
    popd || exit
    touch "${DIR_BUILD}/bde/.complete"
fi

if [ ! -e "${DIR_BUILD}/ntf/.complete" ]; then
    # Build and install NTF
    pushd "${DIR_THIRDPARTY}/ntf-core" || exit
    ./configure --prefix "${DIR_INSTALL}" \
                --output "${DIR_BUILD}/ntf" \
                --without-warnings-as-errors \
                --without-usage-examples \
                --without-applications \
                --ufid "dbg_64_safe_cpp20" \
                --toolchain "${TOOLCHAIN_PATH}"
    make -j 16
    make install
    popd || exit
    touch "${DIR_BUILD}/ntf/.complete"
fi

export FUZZER="fuzzer"
CMAKE_OPTIONS=(\
    -DCMAKE_INSTALL_LIBDIR="lib" \
    -DCMAKE_INSTALL_PREFIX="${DIR_INSTALL}" \
    -DCMAKE_MODULE_PATH="${DIR_THIRDPARTY}/bde-tools/cmake;${DIR_THIRDPARTY}/bde-tools/BdeBuildSystem" \
    -DCMAKE_PREFIX_PATH="${DIR_INSTALL}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_PATH}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

PKG_CONFIG_PATH="${DIR_INSTALL}/lib64/pkgconfig:${DIR_INSTALL}/lib/pkgconfig:$(pkg-config --variable pc_path pkg-config)" \
cmake --preset fuzz-tests -B "${DIR_BUILD}/blazingmq" -S "${DIR_ROOT}" "${CMAKE_OPTIONS[@]}"
cmake --build "${DIR_BUILD}/blazingmq" --parallel 16 --target fuzztests
