# Fuzz Corpus

**fuzzcorpus** records seed inputs for the
`s_bmqfuzz_mqba_application_client` fuzzer.

It starts a single-node broker with the same cluster and domain as the
fuzzer's broker, and runs `bmqtool` scenarios through a TCP proxy.  The bytes
sent on each client connection are written to `<scenario>_<n>.bin`.  Scenarios
are defined in `scenarios.py`.

## Running

Build `bmqbrkr` and `bmqtool`, then:

```shell
export PYTHONPATH=src/python
python3 -m blazingmq.dev.fuzzcorpus corpus/

# Use the corpus
./s_bmqfuzz_mqba_application_client corpus/
```

The build directory is `BLAZINGMQ_BUILD_DIR`, or `build/blazingmq` in the
repository; use `--build-dir` to override it.  Use `--work-dir` to keep the
broker files and logs.
