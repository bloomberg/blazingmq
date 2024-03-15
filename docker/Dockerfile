FROM docker.io/ubuntu:22.04 AS builder

# Set up CA certificates first before installing other dependencies
RUN apt-get update && \
    apt-get install -y ca-certificates && \
    apt-get install -y --no-install-recommends \
    build-essential \
    gdb \
    curl \
    python3.10 \
    ninja-build \
    pkg-config \
    bison \
    libfl-dev \
    libbenchmark-dev \
    libgmock-dev \
    libz-dev \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*

# BDE build requires CMake > 3.24, which is not available in Ubuntu 22.04 yet
# Consider installation of cmake from apt once it's met BDE requirements
RUN mkdir -p /deps_build/CMake \
    && cd /deps_build/CMake \
    && curl -s -L -O https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-linux-x86_64.sh \
    && /bin/bash cmake-3.27.1-linux-x86_64.sh -- --skip-license --prefix=/usr/local \
    && rm -rf /deps_build

WORKDIR /workspace

COPY docker/build_deps.sh bin/build_deps.sh

RUN bin/build_deps.sh

ENV PATH=${PATH}:/workspace/srcs/bde-tools/bin
ENV PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:/opt/bb/lib64/pkgconfig

COPY CMakeLists.txt srcs/bmq/CMakeLists.txt
COPY src srcs/bmq/src
COPY etc srcs/bmq/etc

RUN cd srcs/bmq  \
   && cmake -S . -B cmake.bld/Linux -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=/workspace/srcs/bde-tools/BdeBuildSystem/toolchains/linux/gcc-default.cmake \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBDE_BUILD_TARGET_SAFE=ON \
        -DBDE_BUILD_TARGET_64=ON \
        -DBDE_BUILD_TARGET_CPP17=ON \
        -DCMAKE_PREFIX_PATH=/workspace/srcs/bde-tools/BdeBuildSystem \
        -DCMAKE_INSTALL_LIBDIR=lib64 \
    && cmake --build cmake.bld/Linux --parallel 8

RUN cd srcs/bmq \
    && cmake --install cmake.bld/Linux --prefix=/bmq \
    && mv /bmq/bin/bmqbrkr.tsk /bmq/bin/bmqbrkr \
    && mv /bmq/bin/bmqtool.tsk /bmq/bin/bmqtool

FROM docker.io/ubuntu:22.04

COPY --from=builder /bmq /usr/local

RUN addgroup bmq \
    && adduser bmq --home /var/local/bmq --system --ingroup bmq --shell /usr/bin/bash
USER bmq
WORKDIR /var/local/bmq
RUN mkdir -p logs storage/archive

CMD [ "/bin/bash" ]
