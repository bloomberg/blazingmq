# syntax=docker/dockerfile:1

FROM docker.io/ubuntu:24.04 AS build

# By default
ARG SANITIZER_NAME="msan"

# Copy blazingmq source code
WORKDIR /blazingmq
COPY docker docker
COPY etc etc
COPY src src
COPY CMakeLists.txt ./

RUN chmod +x docker/build_deps.sh
RUN chmod +x docker/sanitizers/build_sanitizer.sh
RUN docker/sanitizers/build_sanitizer.sh ${SANITIZER_NAME}

FROM build AS run
# Copy the built binaries and run unit test script
COPY --from=build /blazingmq/cmake.bld/Linux /blazingmq
#WORKDIR /blazingmq
#RUN chmod +x run-unittests.sh
#ENTRYPOINT [ "/bin/bash" ]
CMD [ "./run-unittests.sh" ]
