# Purpose
This folder contains scripts to run BlazingMQ and its dependencies under sanitizer (asan, msan, tsan and ubsan) in Docker container.

Usually sanitizers check is done on CI, but using Docker it is possible to run sanitizers check in both CI and local environment.

## Running sanitizer check in local environment to debug sanitizer issues
 - Prerequisites: docker should be installed;
 - Run docker build from the BlazingMQ root folder:
 ```
 docker build -f docker/sanitizers/Dockerfile --no-cache --build-arg SANITIZER_NAME=<sanitizer-name> -t sanitizer-<sanitizer-name> .
 ```
 NOTE: `sanitizer-name` is `asan`, `msan`, `tsan` or `ubsan`.

- Run docker container with unit tests
```
docker run --rm sanitizer-<sanitizer-name>
```
NOTE: `sanitizer-name` - sanitizer's name from previous step.

For debbugging, it is possible to run docker container with `/bin/bash` option and run desired tests manually, e.g.
```
docker run --rm -it sanitizer-<sanitizer-name> /bin/bash

root@923efd7529a4:/blazingmq# cd cmake.bld/Linux && ./run-env.sh ctest -R <test-name>
```

- Run docker container with integration tests

Start container in interactive mode and run ITs with specified PRESET and extra parameters (see also build.yaml for reference):
```
docker run --rm -it sanitizer-<sanitizer-name> /bin/bash

root@923efd7529a4:/blazingmq# BLAZINGMQ_IT_PRESET="fsm_mode and strong_consistency" /blazingmq/cmake.bld/Linux/run-it.sh \
--log-level ERROR                   \
--log-file-level=info               \
--bmq-tolerate-dirty-shutdown       \
--bmq-log-dir=failure-logs          \
--bmq-log-level=INFO                \
--junitxml=integration-tests.xml    \
--tb long                           \
--reruns=3                          \
-n logical -v
```
