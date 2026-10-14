# syntax=docker/dockerfile:1

FROM docker.io/ubuntu:26.04@sha256:f144425ff09be612d6d9ad965196e9cdc23dae1f42110a8a11a3e9a8198759f7

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
