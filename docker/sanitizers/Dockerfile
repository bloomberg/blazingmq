# syntax=docker/dockerfile:1

FROM docker.io/ubuntu:26.04@sha256:d66b366dac61b575bee2aef07e9c26e8070096fbe84a3d4d3eb0da069b5d76db

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
