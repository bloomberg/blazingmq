---
layout: default
title: Deployment
parent: Installation
nav_order: 3
---

# BlazingMQ Deployment
{: .no_toc }

* toc
{:toc}

As mentioned in the [configuration](../configuration) article, BlazingMQ is a
highly flexible system, with the BlazingMQ message broker at its core. A
broker, or _node_, participates in a _cluster_, which manages a collection of
domains. A cluster can consist of a single node, which results in better speed;
or of several nodes, which results in better availability and reliability in
presence of network or other failures.

Designing a BlazingMQ system thus involves deciding:

- how many clusters to create
- how many brokers to assign to each cluster

Once this is done, it must be expressed as a set of JSON files, as described in
the [configuration](../configuration) article, and placed in a configuration
directory, which is passed on the command line to the broker when it is
started. As it operates, the broker reads and writes various files.
`bmqbrkrcfg.json` specifies the _log_ directory; `clusters.json` specifies, for
each cluster, a _storage_ and an _archive_ directory. These three directories
must be created prior starting the broker.

When it starts, the broker creates a file (`bmqbrkr.pid`) and a named pipe
(`bmqbrkr.ctl`) in the current directory, which must thus be writable.

Brokers operate in the background, as "services" or "daemons". Thus, it often
makes sense to run them under a dedicated Unix account. However, this is not
mandatory: an application that runs under a normal user account could run a
BlazingMQ broker as part of its implementation.

The following sections describe in more details two typical configurations, and
implements them in a set of Docker containers. Together, they provide a
template for deployment that should cover the majority of use cases, and
provide a good starting point for creating more complex installations.

Much of the
[Dockerfile](https://github.com/bloomberg/blazingmq/blob/main/docker/Dockerfile)
has to do with buiding the broker from the sources, drawing the dependencies
from various sources. In the end, `bmqbrkr` and `bmqtool` are installed in
`/usr/local/bin`.

The section relevant to deployment is at the bottom:

```sh
RUN addgroup bmq \
    && adduser bmq --home /var/local/bmq --system --ingroup bmq --shell /usr/bin/bash
USER bmq
WORKDIR /var/local/bmq
RUN mkdir -p logs storage/archive
```

In English, these lines:
* create a `bmq` group
* create a `bmq` system user, with `/var/local/bmq` as its home directory
* in the home directory, create log, storage and archive directories
* set the default user for Docker commands to `bmq`, and the default current
  directory to `bmq`'s.

The resulting image does not contain a configuration directory. Instead, each
example maps a `config` directory on the host to `/etc/local/bmq` inside the
container(s).

---

## Single Node

In this example (see the [docker-compose.yaml
file](https://github.com/bloomberg/blazingmq/blob/main/docker/single-node/docker-compose.yaml)),
a single machine, called _earth_, runs a broker that acts as the single node of
a cluster named "local". The `hostName` in
[`bmqbrkrcfg.json`](https://github.com/bloomberg/blazingmq/blob/main/docker/single-node/config/bmqbrkrcfg.json)
is set to "earth". "localhost" would work as well, but an arbitrary value would
not, as the broker resolves the name in `hostName`.

The `hostDataCenter` is set to "UNUSED"; any value work, as this setting
is irrelevant in single-node configurations.

The broker is started with the command:

```sh
/usr/local/bin/bmqbrkr /etc/local/bmq
```

Note that this counts on the Docker image setting `bmq` as the default user,
and `/var/local/bmq` as the default directory.

The example can be run (from the root of the BlazingMQ source directory) with:

```sh
$ docker-compose -f docker/single-node/docker-compose.yaml up --detach
```

To interact with the broker:
```sh
$ docker-compose -f docker/single-node/docker-compose.yaml exec bmqbrkr bmqtool
```

```sh
> start
> open uri="bmq://bmq.test.persistent.priority/qqq" flags="read,write"
> post uri="bmq://bmq.test.persistent.priority/qqq" payload=["foo"]
> list
```
(etc)

---

## Cluster

In this example (see the [docker-compose.yaml
file](https://github.com/bloomberg/blazingmq/blob/main/docker/cluster/docker-compose.yaml)),
four machines in two data centers (_gallifrey_ and _skaro_ in _WHO_, _pacem_
and _lusus_ in _HYPERION_) each run a broker. They are all nodes in the
_planets_ cluster.

It looks each machine needs its own set of configuration files, or, at least,
its own `bmqbrkrcfg.json`, because, on each machine, the value of `hostName`
needs to be the machine's name. Moreover, `hostDataCenter` must be set to
either _WHO_ or _HYPERION_, depending on the machine. Fortunately, these
settings can be overriden from the command line, with the `-h/--hostName` and
`-d/--hostDataCenter`

Thus, on _gallifrey_, the broker is started with the command:

```sh
/usr/local/bin/bmqbrkr -h gallifrey -d WHO /etc/local/bmq
```

And on _pacem_:

```sh
/usr/local/bin/bmqbrkr -h pacem -d HYPERION /etc/local/bmq
```

The example can be run (from the root of the BlazingMQ source directory) with:

```sh
$ docker-compose -f docker/cluster/docker-compose.yaml up --detach
```

This example also creates an additional machine - _earth_ - from which
`bmqtool` can be run. Since the tool and the broker are not on the same machine
this time, it is necessary to specify the broker's endpoint:

```sh
$ docker-compose -f docker/cluster/docker-compose.yaml run bmqtool bash
bmq@earth:~$ bmqtool -b tcp://skaro:30114
> start
```
