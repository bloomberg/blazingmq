---
layout: default
title: Configuration
parent: Installation
nav_order: 2
---

# BlazingMQ Configuration
{: .no_toc }

* toc
{:toc}

BlazingMQ is a highly flexible system, with the BlazingMQ message broker at its
core. A broker, or _node_, participates in a _cluster_, which manages a
collection of _domains_.  This leads to three configurable entities in the
system -- BlazingMQ message broker, BlazingMQ cluster and BlazingMQ domain.
Configurations for all of them are expressed as a set of JSON files, located in
a directory passed to message broker process (`bmqbrkr`) on the command line.

The structure of the directory is:

* `root/`
  * `bmqbrkrcfg.json`
  * `clusters.json`
  * `domains/`
    * `domain1.json`
    * `domain2.json`
    * ...


## Broker Configuration

`bmqbrkrcfg.json` contains configuration that is specific to an instance of the
broker, i.e. a single process running `bmqbrkr`. It specifies various low-level
parameters, like number of worker threads, initial size of memory pools, the
location and the format of the log files, etc.

The more important parameters in this file are:

* `appConfig/hostName`  \
    The name of the host, as it will appear in the cluster configuration
    file. Can be overriden on the command line with `-h` or `--hostName`.

* `appConfig/brokerInstanceName`   \
    The name of the broker instance, used to identify a broker if more than
    one broker runs on the same host. Set to "default" otherwise. Can be
    overriden on the command line with `-i` or `--instanceId`.

* `appConfig/hostDataCenter`   \
    The name of the "data center" of the host. In some circumstances, a broker
    will prefer to use a connection with another broker in the same data
    center, for performance. Can be overriden on the command line with `-d` or
    `--hostDataCenter`.

* `appConfig/hostDataCenter/networkInterfaces/tcpInterface/port`   \
    The port that the broker listens to. Can be overriden on the command line
    with `-p` or `--port`.

For a complete list of settings, see the configuration
[schema](https://github.com/bloomberg/blazingmq/blob/main/src/groups/mqb/mqbcfg/mqbcfg.xsd).
The top level object is `Configuration`. For a sample configuration, see the
Docker
[example](https://github.com/bloomberg/blazingmq/blob/main/docker/cluster/config/bmqbrkrcfg.json).

---

## Cluster Configuration

`clusters.json` specifies all the clusters known to the broker. It contains
two optional keys - `myClusters` and `proxyClusters`. The associated values
are arrays of objects. Either keys are optional, and either arrays can be
empty, but not at the same time.

The objects in the `myClusters` array each specify a cluster that a broker
instance is a member, or _node_, of. They consist of:

* `name`  \
    The name of the cluster.

* `nodes`   \
    An array of objects describing the nodes in the cluster. They consist of:
    * `name`   \
        The name of the node.
    * `id`   \
        An integer node identifier, unique within the cluster.
    * `dataCenter`   \
        The data center of the node. Proxies will use connections to brokers
        in the same data center, if available.
    * `transport/tcp/endpoint`   \
        A string in the form `"tcp://{hostName}:{port}"`

* `partitionConfig` \
     Specifies the location of the
     [partitions](../../architecture/clustering#storage-shard) used to store
     the messages, maximum file sizes, whether to pre-allocate and prefault
     storage, etc.

* `elector`   \
    Settings for the leader election subsystem: quorum, maximum attempts to
    open a queue, and various timeouts.

* `queueOperations`   \
    Settings for queue operations (mostly timeouts).

* `clusterMonitorConfig`   \
    Controls how long transitional cluster states can perdure before raising
    an alert in the broker logs.

* `messageThrottleConfig`   \
    Sets limits on the rate at which a producer can post messages.

The objects in the `proxyClusters` array specify the clusters the broker can
proxy to. They are similar to the `myClusters` objects, but they lack the
`masterAssignment`, `partitionConfig` and `elector` keys.

For a complete list of settings, see the configuration
[schema](https://github.com/bloomberg/blazingmq/blob/main/src/groups/mqb/mqbcfg/mqbcfg.xsd).
The top level object is `ClustersDefinition`. For a sample configuration, see
the Docker
[example](https://github.com/bloomberg/blazingmq/blob/main/docker/cluster/config/clusters.json).

---

## Domain Configurations

Each file corresponds to one BlazingMQ domain that the broker can handle. It
contains either a _definition_ or a _redirection_.  For a complete list of
settings, see the
[schema](https://github.com/bloomberg/blazingmq/blob/main/src/groups/mqb/mqbconfm/mqbconf.xsd).
The top level object is `DomainVariant`. For sample configurations, see the
Docker
[example](https://github.com/bloomberg/blazingmq/tree/main/docker/cluster/config/domains).

### Domain Definition

The more important parameters in the `definition` object are:

* `definition/location`  \
    The name of the cluster to contact to open a queue in the domain. It must
    be a name listed in the `myClusters` section of `clusters.json`, in which
    case the broker handles the request; or a name listed in `proxyClusters`,
    in which case the broker forwards the request.

* `definition/parameters/mode`  \
    The queue mode, either `priority`, `broadcast`, or `fanout`.

* `definition/parameters/maxDeliveryAttempts` \
    The maximum number of times the cluster should attempt to deliver the same
    message. Any value other than zero enables [poison pill
    detection](../../features/poison_pill_detection).

* `definition/parameters/consistency`  \
    `eventual` or `strong`.  Select
    the [consistency](../../architecture/clustering#eventual-vs-strong-consistency-in-replication)
    model.

* `definition/parameters/storage/config`  \
    `fileBacked` or `inMemory`.  Specifies whether messages are kept on disk
    (typical for priority and fanout domains), or in memory (required for
    broadcast domains, allowed for 1-node clusters).

* `definition/parameters/mode/fanout/appIDs`  \
    For fanout mode only, a list of appId names.

The other parameters configure per-domain and per-queue message and byte
quotas, maxima for the number of producers, consumers, and queues, etc.

### Domain Redirection

Redirection is similar to a symbolic link, i.e. it gives an additional name to
an existing domain. Unlike symlinks, only one level of redirection is allowed,
and it is performed only by the first broker that handles the open queue
request.

A redirection consists of a single key-value pair: `redirect` and the new
domain name.

---
