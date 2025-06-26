# syntax=docker/dockerfile:1

FROM docker.io/ubuntu:24.04

# Check if SANITIZER_NAME argument is passed
ARG SANITIZER_NAME
RUN if [ -z $SANITIZER_NAME ]; then \
  echo "Required SANITIZER_NAME build argument is not passed." ;\
    exit 1 ;\
  fi
ARG FUZZER

# Copy blazingmq source code
WORKDIR /blazingmq
COPY docker docker
COPY etc etc
COPY src src
COPY CMakeLists.txt ./
COPY CMakePresets.json ./

# Build with sanitizer instrumentation
RUN docker/sanitizers/build_sanitizer.sh ${SANITIZER_NAME} ${FUZZER}

# Run unit tests
CMD [ "/blazingmq/cmake.bld/Linux/run-unittests.sh" ]
