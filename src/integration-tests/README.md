# BlazingMQ Integration Tests

To run the tests:

* (create and) activate a Python 3.8 (or above) `venv`
    * `python3 -m venv /path/to/venv`
    * `source /path/to/venv/bin/activate`
* install required modules
    * `pip3 install -r src/python/requirements-test.txt`
* run the tests
    * `cd src/integration-tests`
    * `./run-tests [preset] [extra pytest options]`

## Presets

The first non-option argument to `run-tests` selects a preset, i.e. the set of
test configurations to run. If omitted, the default preset
`"legacy_mode or fsm_mode"` is used. A preset may also be provided through the
`BLAZINGMQ_IT_PRESET` environment variable.

* `./run-tests` (default preset)
* `./run-tests "legacy_mode"`
* `./run-tests "not fsm_mode"`
* `./run-tests "legacy_mode or fsm_mode"`
* `export BLAZINGMQ_IT_PRESET="fsm_mode" && ./run-tests`

Any additional arguments are forwarded to `pytest`, e.g.
`./run-tests "fsm_mode" -k test_breathing`.

## Custom binary locations

You might also want to specify custom binary locations as follows:

* `BLAZINGMQ_BUILD_DIR` - the root directory where the resulting binaries reside;
   default: `build/blazingmq`
* `BLAZINGMQ_BROKER` - the file name of bmqbrkr (including path);
   default: `{build_dir}/src/applications/bmqbrkr/bmqbrkr.tsk`
* `BLAZINGMQ_TOOL` - the file name of bmqtool (including path);
   default: `{build_dir}/src/applications/bmqtool/bmqtool.tsk`
* `BLAZINGMQ_STORAGETOOL` - the file name of bmqstoragetool (including path);
   default: `{build_dir}/src/applications/bmqstoragetool/bmqstoragetool.tsk`
* `BLAZINGMQ_PLUGINS` - the directory containing plugins;
   default: `{build_dir}/src/plugins`
