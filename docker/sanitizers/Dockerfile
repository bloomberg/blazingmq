# syntax=docker/dockerfile:1

FROM docker.io/ubuntu:26.04@sha256:6a06111c3b0a8600feadb270aec771dc239bf407b25625f93f44ab00e11f4d3c

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
