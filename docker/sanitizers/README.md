# Purpose
This folder contains scripts to run BlazingMQ and its dependencies under sanitizer (asan, msan, tsan and ubsan) in Docker container.

Usually sanitizers check is done on CI, but using Docker it is possible to run sanitizers check in both CI and local environment.

The `Dockerfile` builds a base image with the BlazingMQ sources and the parts of
the environment which are the same for every sanitizer. The instrumented build
itself is done by `build_sanitizer.sh`, run in a container started from that
image. The base image can therefore be built once and reused by all sanitizers.

## Running sanitizer check in local environment to debug sanitizer issues
 - Prerequisites: docker should be installed;
 - Build the base image from the BlazingMQ root folder:
 ```
 docker build -f docker/sanitizers/Dockerfile -t sanitizer-base .
 ```

- Start a container and build with the instrumentation:
```
docker run -d --name sanitizer sanitizer-base sleep infinity
docker exec sanitizer docker/sanitizers/build_sanitizer.sh <sanitizer-name> off
```
NOTE: `sanitizer-name` is `asan`, `msan`, `tsan` or `ubsan`. The second argument
enables the fuzzer, it is `on` or `off`.

- Run unit tests
```
docker exec sanitizer /blazingmq/cmake.bld/Linux/run-unittests.sh
```

For debbugging, it is possible to get a shell in the container and run desired tests manually, e.g.
```
docker exec -it sanitizer /bin/bash

root@923efd7529a4:/blazingmq# cd cmake.bld/Linux && ./run-env.sh ctest -R <test-name>
```

- Run integration tests

Run ITs with specified PRESET and extra parameters (see also build.yaml for reference):
```
docker exec -it sanitizer /bin/bash

root@923efd7529a4:/blazingmq# BLAZINGMQ_IT_PRESET="fsm_mode and strong_consistency" /blazingmq/cmake.bld/Linux/run-it.sh \
--log-level ERROR                   \
--log-file-level=info               \
--bmq-tolerate-dirty-shutdown       \
--bmq-log-dir=failure-logs          \
--bmq-log-level=INFO                \
--junitxml=integration-tests.xml    \
--tb long                           \
--reruns=2                          \
-n logical -v
```
