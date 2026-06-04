# Fuzz Testing

**fuzztest** is a module for fuzz testing the BlazingMQ broker using the
[boofuzz](https://github.com/jtpereyda/boofuzz) library. Messages are mutated
and sent to the broker to verify it handles malformed input gracefully without
crashing.

The protocol definitions and fuzzing logic live in this package
(`blazingmq.dev.fuzztest`). The test runner lives in `src/fuzz-tests/`.

## Running

Build the broker, install test dependencies, and run with pytest:

```shell
cmake --preset default
cmake --build --preset default

pip install -r src/python/requirements-test.txt

# Run all fuzz tests
BLAZINGMQ_BUILD_DIR=build/blazingmq pytest src/fuzz-tests/

# Run a single request type
BLAZINGMQ_BUILD_DIR=build/blazingmq pytest src/fuzz-tests/ -k put
```

If the build used `cmake --preset default`, `BLAZINGMQ_BUILD_DIR` defaults to
`build/blazingmq` and can be omitted.

## Running via ctest

After configuring with cmake, fuzz tests are registered as ctests:

```shell
# List fuzz tests
ctest --test-dir build/blazingmq -L fuzztest -N

# Run all fuzz tests
ctest --test-dir build/blazingmq -L fuzztest

# Run a single request type
ctest --test-dir build/blazingmq -L fuzztest -R put
```
