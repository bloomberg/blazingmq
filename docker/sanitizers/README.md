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
