FROM docker.io/ubuntu:24.04 AS builder

# Set up CA certificates first before installing other dependencies
RUN apt-get update && \
    apt-get install -y ca-certificates && \
    apt-get install -y --no-install-recommends \
    build-essential \
    gdb \
    cmake \
    curl \
    python3 \
    ninja-build \
    pkg-config \
    bison \
    libfl-dev \
    libbenchmark-dev \
    libgmock-dev \
    libgtest-dev \
    liblz4-dev \
    libssl-dev \
    libz-dev \
    libzstd-dev \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

ENV PATH=${PATH}:/workspace/srcs/bde-tools/bin
ENV PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:/opt/bb/lib64/pkgconfig

COPY docker/build_deps.sh bin/build_deps.sh
COPY CMakeLists.txt srcs/bmq/CMakeLists.txt
COPY src srcs/bmq/src
COPY etc srcs/bmq/etc

RUN bin/build_deps.sh \
    && cd srcs/bmq  \
    && cmake -S . -B cmake.bld/Linux -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=/workspace/srcs/bde-tools/BdeBuildSystem/toolchains/linux/gcc-default.cmake \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBDE_BUILD_TARGET_SAFE=ON \
        -DBDE_BUILD_TARGET_64=ON \
        -DBDE_BUILD_TARGET_CPP17=ON \
        -DCMAKE_PREFIX_PATH=/workspace/srcs/bde-tools/BdeBuildSystem \
        -DCMAKE_INSTALL_LIBDIR=lib64 \
    && cmake --build cmake.bld/Linux --parallel 8 --target bmqbrkr bmqtool \
    && cmake --install cmake.bld/Linux --prefix=/bmq --component bmqbrkr \
    && cmake --install cmake.bld/Linux --prefix=/bmq --component bmqtool \
    && mv /bmq/bin/bmqbrkr.tsk /bmq/bin/bmqbrkr \
    && mv /bmq/bin/bmqtool.tsk /bmq/bin/bmqtool \
    && rm -rf /workspace

FROM docker.io/ubuntu:24.04

COPY --from=builder /bmq /usr/local

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    openssl

RUN groupadd bmq \
    && useradd bmq --home /var/local/bmq --system --gid bmq --shell /usr/bin/bash
USER bmq
WORKDIR /var/local/bmq
COPY bin/gen-tls-certs.sh gen-tls-certs.sh

RUN mkdir -p logs storage/archive certs && \
    cd certs && \
    ../gen-tls-certs.sh ca ca-cert dummy && \
    ../gen-tls-certs.sh server ca-cert broker_ dummy

CMD [ "/bin/bash" ]
