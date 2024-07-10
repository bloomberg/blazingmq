# Description #

**fuzztest** is a module for performing fuzz testing of BlazingMQ broker.

Fuzz testing is based on the [boofuzz](https://github.com/jtpereyda/boofuzz) library.
During fuzz testing messages are being modified and possibly malformed, then
sent to broker to check if any crash will happen.  We expect that the broker will
handle all messages gracefully, even if message is malformed.

# Launching #

- Build the BlazingMQ broker (the build dir might be `cmake.bld/Linux/src/applications/bmqbrkr`)

- Set up the repo dir for convenience

```shell
export BLAZINGMQ_REPO=`pwd`
```

- Install virtual env and requirements:

```shell
python3 -m venv ${BLAZINGMQ_REPO}/venv
source ${BLAZINGMQ_REPO}/venv/bin/activate
pip3 install -r "${BLAZINGMQ_REPO}/src/python/blazingmq/dev/requirements.txt"
```

- Launch fuzz testing:

```shell
source ${BLAZINGMQ_REPO}/venv/bin/activate
cd ${BLAZINGMQ_REPO}/src/python
python3 -m blazingmq.dev.fuzztest --broker-dir "${BLAZINGMQ_REPO}/cmake.bld/Linux/src/applications/bmqbrkr"
```
