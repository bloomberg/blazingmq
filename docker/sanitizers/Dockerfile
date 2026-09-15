# syntax=docker/dockerfile:1

FROM docker.io/ubuntu:26.04@sha256:513c074113a871b51a8d16ab445c88779d6452d937a164fb5cc479f32668a41d

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
