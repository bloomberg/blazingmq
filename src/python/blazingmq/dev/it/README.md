## Integration tests

This directory contains the integration test apparatus and the test suites
themselves.  It has the following content:

| directory     | file                    | content                                                            |
|---------------|-------------------------|--------------------------------------------------------------------|
| `.`           | `cluster.py`            | class `Cluster`: manage a set of brokers, proxies and clients      |
| `.`           | `fixtures.py`           | fixtures and decorators for running various cluster configurations |
| `.`           | `logre.py`              | regular expressions for analyzing broker and client logs           |
| `.`           | `util.py`               | various utilities                                                  |
| `bin`         |                         | helper scripts                                                     |
| `bin`         | `bmqfilt.py`            | filter to post-process combined log for readibility                |
| `bin`         | `create-bmqit-image.sh` | create a docker image for running tests in a container             |
| `bin`         | `docker-env.sh`         | internal use                                                       |
| `bin`         | `docker-run.sh`         | run a command inside the `bmq-it` container                        |
| `bin`         | `rit-ci.sh`             | run all tests (using parallelism); this is run by Jenkins          |
| `bin`         | `rit-docker.sh`         | run tests inside a `bmq-it` container                              |
| `process`     |                         | process wrappers                                                   |
| `process`     | `broker.py`             | class `Broker`: wrapper for a `bmqbrkr` process                    |
| `process`     | `client.py`             | class `Client`: wrapper for  `bmqtool` process                     |
| `testsuites`  |                         | contains the official test suites                                  |
| `tests`       |                         | `bmqit`'s test suite                                               |
| `local`       |                         | if present, contains personal test suites; ignored by `rit-ci.sh`  |
| `docker`      |                         | Docker context for running test suites inside a container          |

## Writing Tests

Integration tests are written and run using the [`pytest`
framework](https://docs.pytest.org/).  A basic understanding of `pytest` is
assumed.

The BMQ Integration Test framework (henceforth "BMQIT") is implemented in the
`bmq.dev.it` package hierarchy. It provides several `pytest` fixtures that
deploy and start a cluster before executing the test method - and cleans up
after the test has exited.

etc. Clients are
instances of `bmqtool.tsk`, encapsulated in a `Client` class that provides
methods for opening and closing queues, posting messages, etc.

### Stock Fixtures

`bmq.dev.it.fixtures` provides the following fixtures:

* `local_cluster`: a local "cluster" setup, consisting of a standalone broker
  and no proxies. The fixture is parameterized by the CSL mode, with two
  possible values: CSL enabled or not.

* `standard_cluster`: a multi-node cluster setup, consisting of four nodes
  in two data centers of two nodes each, and two proxies (one in each
  data center). The fixture is parameterized by the CSL mode, with two
  possible values: CSL enabled or not.

* `cluster`: a parametric fixture that combines `local_cluster` and
  `standard_cluster`

When used as method arguments, these fixtures check for the presence of a
  `setup_cluster` instance method. If it is found, it is called with the
  fixture object as argument after the cluster has been started.

### Cluster Configuration

The fixtures create a `Cluster` object, and pass it to the
test. It has methods to access and manipulate the nodes and the proxies that
make up the cluster.

The cluster is created via a `bmq.dev.Workspace` object, which contains all the
information needed for cluster, domain, and routing configuration. The
`Workspace`s used by the stock BMQIT fixtures are all derived from the same
base configuration, initialized by loading development versions of
`clusters.json`, `domain_routing.json`, and `domains.json` located in
`etc`.  The base configuration is then cloned and tweaked by the fixtures.
In stock fixtures, the cluster is named `itCluster`, and answers for domains
`bmq.test.mmap.priority[.sc]`, `bmq.test.mmap.fanout[.sc]`, and
`bmq.test.mem.broadcast[.sc]`.

As much as possible, tests should be agnostic with regard to cluster topology.
`Cluster.proxy_cycle()` helps achieving this.

### Cluster Startup

By default, the stock fixtures start all the brokers (nodes and proxies), then
wait until a leader has been elected. This happens during fixture setup,
*before* the beginning of the test itself. `@start_cluster(start, wait_leader,
wait_ready)` can be used to customize the start-up sequence. how the `cluster` fixtures start the
cluster.  The arguments are all booleans, and have the following meaning:

* `start`: if `True`, start all the nodes and proxies.  If set to `False`, it
  is the responsibility of the test code to start the nodes and the proxies
  explicitly.  The default is `True`.

* `wait_leader`: if `True`, wait until a leader has been elected.  The default
  is `True`.

* `wait_ready`: if `True`, wait untill *all* the nodes see that *all* the nodes
  have transitioned to "ready" state.  The default is `False`

### Process Wrappers

BMQIT provides the following wrapper classes:

* `bmq.dev.it.process.Broker`: a wrapper for a process running `bmqbrkr.tsk`.
  It provides methods for sending commands to a broker and examining the log.

* `bmq.dev.it.process.Client`: a wrapper for a process running `bmqtool.tsk`.
  It provides methods for sending commands to a client and examining its
  output.

* `bmq.dev.it.process.Process`: the base class of `Broker` and `Client`.  It
  provides methods for controlling a process and examining its output.

The most basic way of examining the output or the log of a process is the
`capture` family of functions (`capture`, `capture_n`, `outputs_substr` and
`outputs_regex`).  However, it is recommended to use high level command
wrappers as much as possible.  They have a "blocking" mode (enabled by setting
keyword argument `block` to `True`), which makes them wait for the process to
finish the operation, and return the result.  They also have a "succeed" mode,
(enabled by setting keyword argument `succeed` to `True`) which makes them
block and raise an exception if the result denotes an error.  The "blocking"
mode should be used to write *tests* (i.e. inside assertions), while the
"succeed" mode should be used to prepare the context for a test.

See the embedded documentation strings for a specification of the public
methods.

### Miscellaneous helpers

* `bmq.dev.it.testconstants` provides constants for a set of
  domains, URIs, queue names, etc useful in writing tests.

* `bmq.dev.it.util` provides miscellaneous utilities.

### Example

Here is a complete example, followed by a breakdown:

```python
# 99doc_test.py                                                              #1

from bmq.dev.it.fixtures import cluster, local_cluster                       #2
from bmq.dev.it.testconstants import *                            #3

class TestDemo:                                                              #4
    def setup_cluster(self, cluster):                                        #5
        proxies = cluster.proxy_cycle()                                      #6
        self.producer = next(proxies).create_client('producer')              #7
        self.consumer = next(proxies).create_client('consumer')              #8

    def test_post_message_priority(self, cluster):                           #9
        self.producer.open(
            URI_PRIORITY, flags=['write', 'ack'], succeed=True)             #10
        payload = 'foobar'
        self.producer.post(
            URI_PRIORITY, payload=[payload], succeed=True, wait_ack=True)

        self.consumer.open(URI_PRIORITY, flags=['read'], succeed=True)

        self.consumer.wait_push_event()                                     #11

        msgs = self.consumer.list(URI_PRIORITY, block=True)                 #12
        assert len(msgs) == 1                                               #12
        assert msgs[0].payload == payload                                   #12

        self.consumer.confirm(URI_PRIORITY, '*', succeed=True)              #13
        msgs = self.consumer.list(URI_PRIORITY, block=True)                 #13
        assert len(msgs) == 0                                               #13

    def test_post_message_fanout(self, local_cluster):                      #14
        self.consumer.open(URI_PRIORITY, flags=['read'], succeed=True)
        self.consumer.wait_push_event()
        msgs = self.consumer.list(URI_PRIORITY, block=True)
        assert len(msgs) == 1
```

1. The file name has to end in `_test.py` for `pytest` to pick it.

2. Import the `cluster` and `local_cluster` fixtures.

3. Import all the constants.  In this test could also just import
   `URI_PRIORITY`.

4. The test class must begin with `Test` for `pytest` to pick it.

5. Define a `setup_cluster` method.  It will be called before each test method,
   after the cluster has been started.  It can be used to perform setup actions
   that are common to all the test methods, e.g. create a few clients and open
   a few queues.

6. Get a cyclic list of "proxies".

7. Fetch the next "proxy".  If the cluster is local, this returns the broker;
   otherwise, this returns a proxy connected to a node in the same data center
   as the leader.  Create a client connected to that proxy.

8. Fetch the next "proxy".  If the cluster is local, this returns the broker;
   otherwise, this returns a proxy connected to a node in the data center after
   the leader's.  Create a client connected to that proxy.

9. Define a test method.  Its name has to begin with `test_` for `pytest` to
   pick it.  It is executed twice: once with a local cluster, and once with a
   standard cluster.  In both cases, the cluster is passed in the `cluster`
   argument.

10. Open a queue.  This operation is *not* part of the test.  It's just setup.
   Thus we specify that we expect the operation to succeed with
   `succeed=True`.  If it is not the case, an exception will be raised, and
   translated to an `ERROR` by `pytest`.

11. Wait until the consumer client received a push event.

12. This is a test.  We send a `list` command to the client, and block until
    the result appears in its output.  The listed messages are returned as an
    array of tuples.  We check if the expected number of messages was seen and
    verify the payload.

13. This is the beginning of a second test.

14. Another test method.  This one runs only once, using a local cluster
    (obviously it fails).

### Tweaking the configuration

Test code can add its own tweaks to the stock configurations, by applying the
`@tweak` and `@tweak_value` decorators, at the function, method, or class
level, as needed.

`@tweak` takes a list of functions and calls them on the `Workspace` object,
before it is deployed.  The `Workspace` has three attributes -
`cluster_catalog`, `domain_catalog`, and `routing`.  For stock configurations,
they are loaded, respectively, with the content of the `clusters.json`,
`domains.json`, and `domains_routing.json` from `etc`.

A tweak can make arbitrary modifications to the `Workspace` before it is
deployed.  It can even replace the configurations entirely.  Most of the time,
however, a tweak will just perform a few adjustments.  For example:

```python
def limit_consumers(ws):
    ws.domain_catalog[DOMAIN_PRIORITY]['*']['limit.consumers'] = 1

def limit_producers(ws):
    ws.domain_catalog[DOMAIN_PRIORITY]['*']['limit.producers'] = 1

@tweak(limit_consumers, limit_producers)
def test_tweak(cluster):
    proxy = next(cluster.proxy_cycle())
    assert proxy.create_client('producer1').open(
        URI_PRIORITY, flags=['write,ack'], block=True) == Client.e_SUCCESS
    assert proxy.create_client('producer2').open(
        URI_PRIORITY, flags=['write,ack'], block=True) != Client.e_SUCCESS
    assert proxy.create_client('consumer1').open(
        URI_PRIORITY, flags=['read,ack'], block=True) == Client.e_SUCCESS
    assert proxy.create_client('consumer2').open(
        URI_PRIORITY, flags=['read,ack'], block=True) != Client.e_SUCCESS
```

`@tweak` may be applied more than once to the same entity, in which case the
effect is cumulative.  Tweaks may also be applied at different levels
(e.g. class and test method).  In this case, the tweaks are applied from
outside in.

Since tweaks are decorators, i.e. functions that take functions and return
functions, it is easy to write functions that return tweaks, possibly
parameterized.  For example:

```python
def limit_consumers(num):
    def tweaker(ws):
        ws.domain_catalog[DOMAIN_PRIORITY]['*']['limit.consumers'] = num

    return tweak(tweaker)

def limit_producers(num):
    def tweaker(ws):
        ws.domain_catalog[DOMAIN_PRIORITY]['*']['limit.producers'] = num

    return tweak(tweaker)

@limit_producers(1)
def test_exceed_max_producers(cluster):
    proxy = next(cluster.proxy_cycle())
    assert proxy.create_client('producer1').open(
        URI_PRIORITY, flags=['write,ack'], block=True) == Client.e_SUCCESS
    assert proxy.create_client('producer2').open(
        URI_PRIORITY, flags=['write,ack'], block=True) != Client.e_SUCCESS
    assert proxy.create_client('consumer1').open(
        URI_PRIORITY, flags=['read,ack'], block=True) == Client.e_SUCCESS
    assert proxy.create_client('consumer2').open(
        URI_PRIORITY, flags=['read,ack'], block=True) == Client.e_SUCCESS

@limit_consumers(1)
@limit_producers(1)
def test_exceed_both(cluster):
    proxy = next(cluster.proxy_cycle())
    assert proxy.create_client('producer1').open(
        URI_PRIORITY, flags=['write,ack'], block=True) == Client.e_SUCCESS
    assert proxy.create_client('producer2').open(
        URI_PRIORITY, flags=['write,ack'], block=True) != Client.e_SUCCESS
    assert proxy.create_client('consumer1').open(
        URI_PRIORITY, flags=['read,ack'], block=True) == Client.e_SUCCESS
    assert proxy.create_client('consumer2').open(
        URI_PRIORITY, flags=['read,ack'], block=True) != Client.e_SUCCESS
```

Simple tweaks can be implemented easily via `tweak_value`.  It takes a
XmlPath-like path in the `Workspace` object and a value:

```python
@tweak_value(f'domain_catalog/{DOMAIN_PRIORITY}/*/limit.producers', 1)
def test_tweak(cluster):
    proxy = next(cluster.proxy_cycle())
    assert proxy.create_client('producer1').open(
        URI_PRIORITY, flags=['write,ack'], block=True) == Client.e_SUCCESS
    assert proxy.create_client('producer2').open(
        URI_PRIORITY, flags=['write,ack'], block=True) != Client.e_SUCCESS
```

Again, it is easy to factorize tweaks:

```python
one_producer_only = tweak_value(
    f'domain_catalog/{DOMAIN_PRIORITY}/*/limit.producers', 1)

@one_producer_only
def test_tweak(cluster):
    proxy = next(cluster.proxy_cycle())
    assert proxy.create_client('producer1').open(
        URI_PRIORITY, flags=['write,ack'], block=True) == Client.e_SUCCESS
    assert proxy.create_client('producer2').open(
        URI_PRIORITY, flags=['write,ack'], block=True) != Client.e_SUCCESS
```

## Running Tests

### Requirements

Python >= 3.8 and pytest >= 5.4.3 are required.  It is highly recommended to
use a [virtual environment](https://docs.python.org/3.8/tutorial/venv.html).

### Directories

The IT suite uses two directories:

- The [BMQ/bmq-enterprise
  repository](https://bbgithub.dev.bloomberg.com/BMQ/bmq-enterprise). It is
  derived from the Python module path.

- The build directory. If the environment variable `BMQ_BUILD` is set, its
  value is used; otherwise, if `BMQ_REPO` is set, `$BMQ_REPO/cmake.bld/$(uname
  -s)` is used; otherwise, `<BMQ-E>/../$BMQ_REPO/cmake.bld/$(uname
  -s)` is used, where `<BMQ-E>` is the path to the `bmq-enterprise` repository

### Invoking `pytest`

The `rit.sh`, `rit-docker.sh`, and `rit-ci.sh` scripts, located in
`src/python/bmq/dev/it`, should be used to run the test suite. Invoking
`pytest` directly is not recommended.

### Selecting Tests

All the integration tests are located under `src/python/bmq/dev/it/testsuites`
in the BMQ repository.

From `pytest`'s perspective, a "test" is a function that begins with `test_`.
Tests can be grouped in classes, in which case the class name must begin with
`Test`.

Tests can be selected using keywords (using the `-k` switch) and/or markers
(using the `-m` switch).  BMQIT defines several markers:

|                         |                                                                           |
|-------------------------|---------------------------------------------------------------------------|
| `integrationtest`       | all integration tests                                                     |
| `quick_integrationtest` | integration tests that run with a local cluster                           |
| `pr_integrationtest`    | integration tests to be run as part of a Jenkins PR (currently all tests) |
| `single`                | tests that use a local cluster fixture                                    |
| `multi`                 | tests that use a 4-node, 2-proxy cluster fixture                          |
| `flakey`                | tests that occasionally fail; excluded from the Jenkins PR check          |

### Erroneous Exits

Each time a line is read from any of the logs, and during teardown, the status
of all the processes is checked for erroneous. If any process has returned with
a non-zero error code, it is reported and an exception is thrown.

Currently, we know that nodes sometimes crash (with a strictly negative error
code) or exit in error (with a strictly positive error code) when they are shut
down.  In both case this is due to bugs that should be fixed soon.  In the
meantime, option `--bmq-tolerate-dirty-shutdown` can be used to suppress the
exception *during* *teardown*.  This flag should be used by the PR validation
Jenkins pipeline *only*.

### Debugging C++ Code

Sometimes it is necessary to attach a C++ debugger to one or more processes to
troubleshoot test failures.  This can be achieved with the
`--bmq-break-before-test` command line switch.  It instructs BMQIT to execute
`breakpoint()` (thus entering the Python debugger) just before entering the
test itself.  At this point, the cluster is up and running, and the C++
debugger can be attached to one or several processes from another shell, before
resuming the execution of the test.  Note that this is incompatible with parallelism,
and with execution in a docker container.

However, if we want to attach the C++ debugger in the middle of a test, we
have to do it manually. See the following example:

Here is the function `test_migrate_queue_to_another_cluster()` from
`50restart_test.py`:

```
    def test_migrate_queue_to_another_cluster(self, cluster):
        proxies = cluster.proxy_cycle()
        producer = next(proxies).create_client('producer')

        assert Client.e_SUCCESS is producer.open(
            f'bmq://{DOMAIN_FANOUT}/q1', flags=['write'], block=True)

        cluster.set_domain_resolver_config({
            "bmq.test": None,
        })

        cluster.restart_nodes()

        assert Client.e_SUCCESS is not producer.open(
            f'bmq://{DOMAIN_FANOUT}/q2', flags=['write'], block=True)
```

We want to attach the C++ debugger during `cluster.restart_nodes()`, when the
old nodes have been stopped but the new nodes have not been started yet. To do
so, we insert `breakpoint()` inside `restart_nodes()` in `cluster.py`:

```
    def restart_nodes(self, wait_leader=True, wait_ready=False):
        """
        Restart the nodes in the cluster.  If 'wait_leader' is
        'True', wait for a leader to be elected.
        """
        self.logger.log(self._log_level, "restarting all nodes")
        with internal_use(self):
            self.stop_nodes()
            breakpoint()
            self.start_nodes(wait_leader, wait_ready)
```

Now, we open a terminal and from the BMQ root directory:

```
$ rit.sh --pdb -s -x -k migrate_domain --log-cli-level info
```

`pdb`, the Python Debugger, will run until it hits the `breakpoint()`. Then, we
do from pdb interactive mode:

```
(Pdb) p self.work_dir
PosixPath('/var/folders/7n/30tcn_y50v1cnw4780lg5bfc0000gp/T/tmpeut6rp4k')
```

The Posix path is where bmq.dev.it environment has been set up. Now, we can attach
our C++ debugger (gdb for Linux; lldb for macOS):

```
$ cd /var/folders/7n/30tcn_y50v1cnw4780lg5bfc0000gp/T/tmpeut6rp4k
$ cd LOCAL
$ cat run
source setupenv
mkdir -p storage
mkdir -p storage/archive
./bmqbrkr.tsk ./bmqbrkr.cfg development hostname:LOCAL \
            domainsPath:../bmqdomains
```

Be sure to follow the steps in the `run` script:

```
$ source setupenv
$ mkdir -p storage
$ mkdir -p storage/archive
$ lldb -- \
$ ./bmqbrkr.tsk ./bmqbrkr.cfg development hostname:LOCAL \
$            domainsPath:../bmqdomains
```

And we have successfully attached our C++ debugger to right before the new
nodes start up!

### Running Tests in Parallel

NOTE: parallel test execution is incompatible with `BMQIT_PORT_BASE`.

Before submitting changes for review, it is a good idea to run all the
integration tests.  Tests that use a local cluster execute quickly.  Tests that
use a multi-node cluster, on the other hand, have to go through the election
procedure before actual testing can begin.  Running just one test during the
development cycle is bearable.  However, it is a good idea to periodically run
all the tests, to make sure that our changes have not broken anything.
Fortunately, the `pytest-xdist` plugin enables parallel execution of tests.  It
can be installed via `pip` and is available when running under docker (see
below).

`pytest-xdist` adds several command-line options, the most notable being `-n
NUM`, where `NUM` is the number of processes to fork.  `NUM` can also be
`auto`, in which case one process per CPU is used.  See [the
documentation](https://pypi.org/project/pytest-xdist/) for more information.

Example:

```python
rit.sh -n 16
```

When parallelism is used, The `--bmq-log-dir DIR` switch should be used in
place of the `--log-file`.  In conjonction with `--log-file-level LEVEL`, it
instructs BMQIT to generate a separate log file for each test, at the
specified log `LEVEL`.  The files are placed in the specified `DIR`ectory, and
removed if the test succeeds.  `DIR` should be a subdirectory of the source or
build tree.  If `--bmq-keep-logs` is specified, the log files are preserved
whatever the outcome of the test.

### Running Tests in a Docker Container

BMQIT reserves ports from the pool of free ephemeral ports at the time it
writes the cluster's configuration file.  However, this sometimes results in
port conflicts.  Also, it does not work on Darwin, where the `BMQIT_PORT_BASE`
feature is used as a workaround - which in turns makes it impossible to use
parallelism.

The solution to the port conflict problem is to run the tests inside a Docker
container.  The `rit-docker.sh` (`rit` means "run integration test(s)") script
provides a convenient way of doing this.  It uses a docker image that contains
Python 3.8, `pytest`, the parallelization plugin, and whatever else is
necessary to run the full test suite.  It maps the three BMQIT directories
(source tree, build tree, and domains) in the host to fixed locations in the
container (respectively, `/src/bmq`, `/tmp/bld`, and `/src/bmqdomains`).  It
then runs `rit.sh`, followed by the arguments passed to the script.  Thus,
running all the tests in parallel, in a Docker container, is as simple as:
`src/python/bmq/dev/it/rit-docker.sh -n auto`.

A note about `bmqbrkr` storage directory: in order to simulate real world
scenario, `rit-docker.sh` script tries to map a directory on host to the `/tmp`
directory inside the container.  These are the rules for this mapping:

  1. If `BMQ_DOCKER_TMPDIR` env var is set and that directory exists, map
     it to docker's `/tmp`.
  2. Else, if `/bb/data/tmp` exists, map it to docker's `/tmp`.
  3. Else, raise a warning banner, and simply use `/tmp` directory inside the
     container as is.  Note that this can be a black box, and may lead to
     occasional integration test failures.

Cores resulting from process crashes should be examined from within a container
based on the `bmq-it` image, because they refer to the container's shared
objects.  Failure to do so will result in warnings like these:

```
warning: .dynamic section for "/lib64/libresolv.so.2" is not at the expected address (wrong library or version mismatch?)
warning: .dynamic section for "/lib64/libc.so.6" is not at the expected address (wrong library or version mismatch?)
warning: .dynamic section for "/lib64/ld-linux-x86-64.so.2" is not at the expected address (wrong library or version mismatch?
```

`docker-run.sh` can be used to run `gdb` inside a conainer, e.g.:

```
src/python/bmq/dev/it/docker-run.sh -it gdb /tmp/bld/src/applications/bmqbrkr/bmqbrkr.tsk failure-logs/cores/core.bmqDispQueue.7681.1562943097
```

See the [Docker @ Bloomberg
documentation](https://bbgithub.dev.bloomberg.com/pages/bloomberg-containers/docs/)
for further instructions.

### Updating the Docker Image

The following commands create the image:

```shell
# (re-)create image
${BMQ_ENTERPRISE_REPO}/src/python/bmq/dev/it/bin/create-bmqit-image.sh
```

The following command uploads the image to `artprod`:

```shell
docker login artprod.dev.bloomberg.com
docker push artprod.dev.bloomberg.com/bmq/bmq-it:latest
```

The credentials for `docker login` are the same as for the Toolkit and your
Unix account.

There is no need to pull the new image explicitly, as `rit-docker.sh` and
`rit-ci.sh` do this before running the tests.

### Running Tests as Part of Continuous Integration

The `rit-ci.sh` script is run as part of several Jenkins pipelines.  It can
also be run from a normal account as part of the development cycle.  Additional
pytest options may be passed to `rit-ci.sh`.  Jenkins runs the tests one by one
in the `master` branch, and in parallel (with `-n 32`) in other branches.

The script determines the source and build directories as described at the
beginning of this document, *except* for `BMQDOMAINS_REPO`, which defaults to
`/home/mombot/etc/BMQ/domains`.

Test files in the `src/python/bmq/dev/it/local` directory are ignored by
`rit-ci.sh`.  This makes it possible to keep personal test suites in the source
tree, for example to use them during development.  By placing these test suites
in the `local` directory, we retain the ability to run `rit-ci.sh` and have it
collect the same tests that will be executed under Jenkins.

`rit-ci.sh` configures logging to output logs at or above the ERROR level, for
failed tests *only*, using an abbreviated format.  It also writes a much more
verbose log for *each* test in a corresponding file in the `failure-logs`
directory.

Upon termination, the entire source tree and the logs are archived in a file
named `bmqit-snapshot.tar.gz`, located in the Jenkins workspace.  If cores were
generated, they are archived as well, and a script named `docker-shell` is
created.  It runs a shell inside a docker container based on the `bmqit` image.
The `gdb` debugger contained in the image can then be used to examine the
cores.

### Repeating Tests

The `pytest-repeat` can be used to repeat one or several test(s), in the hope
(or fear) that they will fail.  This is handy to investigate failures that
occur spuriously.  The plugin can be installed via `pip`, and is available in
the Docker image.

See [the documentation](https://pypi.org/project/pytest-repeat/) for more
information.

### Cheat Sheet

The commands listed below are particularly useful.

| command                     | description                                  |
|-----------------------------|----------------------------------------------|
| `rit.sh`                    | run all tests                                |
| `rit-docker.sh`             | run all tests inside docker                  |
| `rit-docker.sh -k KEYWORD`  | run tests that match KEYWORD                 |
| `rit-docker.sh -x`          | run all tests, stop at 1st failure           |
| `rit-docker.sh --n auto`    | run all tests, in parallel                   |
| `rit-docker.sh --count 100` | run all tests, 100 times                     |
| `rit-ci.sh`                 | run all tests as in Jenkins (no parallelism) |
| `rit-ci.sh --n auto`        | run all tests as in Jenkins, in parallel     |

This assumes that `rit-docker.sh` and `rit-ci.sh` (or a link to them) are in
the `PATH`.

## Logging

### Python and `pytest` Logging 101

It is important to have a basic understanding of how logging works in Python's
standard `logging` module, and in `pytest`.

Loggers are organized in a hierarchic structure, following the category tree.
A logger either can have a log level explicitly set, or it inherits the log
level of its parent (recursively).  When methods like `error`, `info`, `debug`,
etc are called on a logger, it produces a record that has the specified level,
but *only* *if* the logger's level is equal or lesser than the requested level.

Log records are then processed by one of more observers.  Typically, an
observer writes a representation of the record to some destination (e.g. the
console, or a file).  Observers have an associated threshold as well.  An
observer will ignore all the records that whose level is below its associated
threshold.

`pytest` provides three observers, each with an associated command-line option
to set its threshold.

* The *capture* observer.  It stores the records during the execution of a
  test, and displays them on the console only if the test fails.  Its threshold
  is controlled by the `--log-level` option.

* The *live* observer.  It displays the records to the console as they are
  produced.  Its threshold is controlled by the `--log-cli-level` option.

* The *file* observer.  It writes the records to a file.  Its threshold is
  controlled by the `--log-file-level` option, and the file by the `--log-file`
  option.

`pytest` sets the threshold for for root logger to the minimum of the capture,
live and file thresholds (WARNING if no level is explicitly set).

### Integrated Logging

BMQIT integrates logging across Python, the broker and the client tool.

The test's activity is logged in the `test` category.  The `logger` fixture
logs to it.  All the *direct* calls to `Cluster`, `Broker`, and `Client`
methods are logged under this category as well.  Thus enabling the `test`
category at the INFO level provides a nice feedback of the test's progress, for
free.

The categories under BMQIT are mainly for developing or troubleshooting
BMQIT.  Among other things, the *indirect* calls to `Cluster`, `Broker`, and
`Client` methods are logged there.  For example, a call to `capture` made by
test's code will be logged under `test`, while a call made from `Client.open`
will be logged under `bmq.dev.it.bmqtool`.

The BALL log output is parsed and mapped to Python categories by prefixing the
BALL category (converted to lower case) with, respectively, `proc.bmqbrkr` and
`proc.bmqtool`.  The output that does not come from BALL logging (typically the
messages output before BALL has been configured) are logged directly under
`proc.bmqbrkr` and `proc.bmqtool`.

BMQIT performs the following transformations on the log records:

- It adds a a new attribute, `bmqContext`, which contains a description of the
  origin of the log record: `TEST` for records issued by test code, and the
  process name of the broker or tool for log records translated from BALL
  records.  The attribute is available in format specifications.

- For translated BALL records, it replaces the file name, line number, time
  stamp, and thread number of the Python log record with BALL's.

- The filename and line number of log records created by `Client` methods
  called directly from a test are overwritten by the test's file name and the
  line number where the call was made.

- It creates versions of `bmqContext`, `category`, `filename`, and `msg`
  abbreviated to 8, 12, 16, 20, 24, 28, and 32 characters.  The name of each
  such attribute consists of the original name of the attribute, with the
  length appended.  If the value of the attribute exceeds the specified length,
  the characters in excess on the left are replaced with a `*`.

In summary, the logger hierarchy is as follows:

- (root)
  - test
  - proc
    - bmqbrkr
      - the `bmqbrkr` categories, e.g. `proc.bmqbrkr.mqbnet`
    - bmqtool
      - the `bmqtool` categories
  - bmq
    - the categories used by the Python modules, e.g. `bmq.core.env``

### Per-category Log Levels

BMQIT allows precise tuning of the logging on a per category basis, via the
(repeatable) `--bmq-log-level` option.  It takes a string in the form:
`((CATEGORY:)?LEVEL(,(CATEGORY:)?LEVEL)*)?`.  A level without a category sets
the default level for the top categories (TEST, PROC and BMQIT).  Each
`CATEGORY:LEVEL` pair specifies a level for a category.  No checks are made for
redundant or contradictory specifications, and the category names are not
(currently) validated.

BMQIT configures the broker and the tool according to the specified verbosity
levels.

Note that `--bmq-log-level` controls the thresholds of the *loggers*.  It does
not change the levels of the observers.  Thus, with `--log-cli-level=INFO
--bmq-log-level=DEBUG`, DEBUG records will *not* appear in the live log
output.

### Examples

```
rit-docker.sh
```

Output log records created by tests, brokers and `bmqtool` processes, and the
BMQIT framework, at severity levels WARNING and above, only for the tests
that fail.

```
rit-docker.sh --log-cli-level INFO
```

As tests are executed, output log records created by tests, brokers and
`bmqtool` processes, and the BMQIT framework, at the INFO level and above.

After each failed test, output all records at the WARNING level and above.

```
rit-docker.sh --log-cli-level INFO --log-level DEBUG
```

As tests are executed, output log records created by tests, brokers and
`bmqtool` processes, and the BMQIT framework, at the INFO level and above.

After each failed test, output all records at the DEBUG level and above.

```
rit-docker.sh --log-cli-level INFO --log-level DEBUG --bmq-log-level BMQIT:WARNING
```

Same as above, but do not display the logs from BMQIT below the WARNING
severity.

```
rit-docker.sh --log-cli-level INFO --bmq-log-level WARNING,TEST:INFO
```

As tests are executed, output log records created by test code at the INFO
level and above.

After each failed test, output all records at the WARNING level and above.

```
rit-docker.sh --log-cli-level DEBUG --bmq-log-level WARNING,PROC.BROKER:DEBUG
```

As tests are executed, output log records created by test code at the INFO
level and above.

After each failed test, output all records at the WARNING level and above.

```
--log-cli-level TRACE --bmq-log-level WARNING,TEST:INFO,PROC.BROKER.MQBNET:TRACE
```

Log all activity for the `BROKER.MQBNET` component at the `TRACE` level, and
only log the other categories at the WARNING level or above.

After each failed test, output all records at the WARNING level and above.
