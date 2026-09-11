# syntax=docker/dockerfile:1

FROM docker.io/ubuntu:26.04@sha256:2260313b31c8c011cd2eebe728008efac1b3982be73eb71348ea2648d2c0e09b

# Base image for the sanitizer builds.  It holds the BlazingMQ sources and the
# parts of the environment which do not depend on the sanitizer, so the same
# image serves every sanitizer.  Run `docker/sanitizers/build_sanitizer.sh` in
# a container started from this image to produce an instrumented build.

# Copy blazingmq source code
WORKDIR /blazingmq
COPY docker docker
COPY etc etc
COPY src src
COPY CMakeLists.txt ./
COPY CMakePresets.json ./

RUN docker/sanitizers/install_common.sh
