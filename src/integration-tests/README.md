# BlazingMQ Integration Tests

[WIP]

To run the tests:

* (create and) activate a Python 3.8 (or above) `venv`
    * `python -m venv /path/to/venv`
    * `source /path/to/venv/bin/activate`
* install required modules
    * `pip install -r src/python/requirements.txt`
* run the tests
    * `cd src/integration-tests`
    * `./run-tests [extra pytest options]`
* you might also want to specify custom binary locations as follows
    * `BLAZINGMQ_BUILD_DIR` - the root directory where the resulting binaries reside;
       default: `cmake.bld/{platform.system()}`
    * `BLAZINGMQ_BROKER` - the file name of bmqbrkr (including path);
       default: `{build_dir}/src/applications/bmqbrkr/bmqbrkr.tsk`
    * `BLAZINGMQ_TOOL` - the file name of bmqtool (including path);
       default: `{build_dir}/src/applications/bmqbrkr/bmqbrkr.tsk`
    * `BLAZINGMQ_PLUGINS` - the directory containing plugins.

