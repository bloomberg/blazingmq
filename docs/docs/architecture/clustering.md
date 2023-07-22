---
layout: default
title: Clustering
parent: Architecture
nav_order: 2
---

# BlazingMQ Cluster, Storage and Replication
{: .no_toc }

* toc
{:toc}

## Introduction

This article provides a high level overview of clustering, replication and
storage subsystems in BlazingMQ.

## Clustering

Like several other distributed storage systems, BlazingMQ stores messages by
leveraging a cluster of commodity machines in order to provide redundancy and
high availability.  BlazingMQ clusters can be anywhere from 3-7 machines in
size.  Smaller as well as larger sized clusters are possible but not
recommended.  In practice, clusters with 5 nodes are ideal.

### Typical Deployment

![Typical Deployment](../../../assets/images/TypicalDeployment.png "Typical BlazingMQ Deployment")

The above figure shows the typical deployment of BlazingMQ.  The four nodes in
the middle represent a BlazingMQ cluster where queues are persisted and
replicated.  The blue node in the cluster represents a queue's **primary**
node, which is in charge of managing the queue (replication, message routing,
etc.).  Orange nodes represent **replicas** which, as the name suggests, are in
charge of storing a local copy of the queue.

A producer or consumer client application can connect to any node in the
BlazingMQ cluster instead of always requiring a connection to the primary node.
If a client application connects to a replica node, the replica will
automatically make the primary node aware of the client application.  It also
means that all messages flowing to and from the client application will go
through the replica.

### Alternative Deployment

![Alternative Deployment](../../../assets/images/AlternativeDeployment.png "Alternative BlazingMQ Deployment")

The above figure shows an alternative deployment of BlazingMQ.  As can be seen,
the applications (green nodes) running on machines A and B connect to local
BlazingMQ agents (yellow nodes), which then connect to any node (primary or
replica; blue or orange nodes) in the BlazingMQ cluster.  These local BlazingMQ
agents are known as **proxies**.  See [*Benefits of Proxy*](#benefits-of-proxy)
section below for more details.

Proxies are optional in BlazingMQ deployments and should be used only if their
presence is required for performance reasons.  If you are just starting with
BlazingMQ, we recommend skipping proxy-aware deployment.

### Multi Hop Topology

An interesting aspect of BlazingMQ network topology that is obvious from the
above figure is that client applications don't need to directly connect to a
queue's primary node.  There can be any number of hops between an application
and the primary node.  In this example, clients connect to the local BlazingMQ
proxy running on their machines, which in turn may connect to a replica node in
the cluster.  In fact, a BlazingMQ proxy can connect to another BlazingMQ
proxy, and so on.  Moreover, clients don't have to connect to the local proxy.
They can connect to a remote proxy cluster, which in turn connects to the
BlazingMQ cluster hosting the queues, etc.  The BlazingMQ network topology is
very flexible in this regard, and generally speaking, a queue's primary node
builds a distribution tree rooted at itself, with replicas and proxies at
intermediate levels in the tree, and producers and consumers at leaf nodes.
Please see the [*Network Topology*](../network_topology) article for an
in-depth discussion as well as to see how its topoloy enables BlazingMQ to
achieve very high fan-out and tremendous bandwidth savings in certain
scenarios.

### Benefits of Proxies

A local BlazingMQ proxy can provide several benefits:

- It hides applications from any changes in the cluster's composition as well
  as transient network issues.  For example, the entire BlazingMQ cluster could
  disappear from the network for a few minutes, and the proxy will hide this
  event from the applications by simply buffering any new messages being sent
  by the producer applications and retransmit them when conditions improve.
  Applications won't get any *DISCONNECTION* events.

- It aggregates all local TCP connections into just a few connections to
  clusters, which allows for a large number of local clients, and also reduces
  fail-over times for cluster nodes.

- Lastly, and arguably most importantly, the proxy carries out fan-out closer
  to the user applications.  Imagine a scenario where there are *N* consumer
  applications running on a machine and *all* applications are interested in
  receiving messages posted on a queue.  In the absence of the proxy, BlazingMQ
  cluster would have to send a message *N* times to this machine.  However, due
  to the proxy's presence, the message is sent only once by the BlazingMQ
  cluster to the local proxy.  This local proxy then carries a local fan-out to
  *N* consumer applications, thereby dramatically increasing scalability and
  performance, and reducing latency and bandwidth requirements for such
  scenarios.

### Quorum based System

BlazingMQ is a quorum based system, which means that as long as the majority of
the nodes in a BlazingMQ cluster are available, the system will be available to
users.  Quorum is required for two reasons -- to maintain continued leadership
in the cluster as well as to provide strong consistency in storage replication.
Both of these concepts are explained later in this article as well as in
[this](../election_rsm) document.

At a high level, a BlazingMQ cluster manages two types of information -- its
own metadata and the queues (to be specific, messages across various queues).
Let's look deeper into each of them.

### Cluster Metadata

Unlike some other distributed systems that depend on a metadata stores like
Apache ZooKeeper to manage their metadata, a BlazingMQ cluster hosts its own
metadata.  The decision to manage metadata within a BlazingMQ cluster was made
very early in the design phase of BlazingMQ, for mainly two reasons --
minimizing dependencies on external software and having flexibility around
metadata management as far as deployment, performance and future enhancements
are concerned.

Metadata is managed by one node in the cluster, known as the leader node, while
other cluster nodes simply follow the leader node.  The leader node is solely
responsible for issuing writes to the metadata, persisting it locally, and also
replicating it to peer nodes ("followers"), which it does by implementing a
replicated state machine similar to (but not exactly same as) the
[*Raft*](https://raft.github.io) consensus algorithm.  More details on this
topic can be found [here](../election_rsm).

Metadata in a BlazingMQ cluster comprises of:

- The cluster's health and cluster membership.

- A list of queues being hosted on the cluster.

- Details about internal *storage shards* configured in the cluster.  More
  details about *storage shards* can be found in the *Storage* section later in
  this document.  For now, a shard can be assumed to be a logical file on disk.
  The number of *storage shards* in a BlazingMQ cluster is configured
  statically.

- *Storage Shard* -> Primary node mapping.  Each *storage shard* is managed by
  a node in the cluster, known as the primary node for that shard.  A shard is
  assigned a primary node by the leader at startup or anytime a shard loses its
  primary node.  Note that the leader node can assign itself as the primary
  node of some or all *storage shards* depending upon the configuration.

- Queue -> shard mapping.  Any time a queue gets created in a BlazingMQ
  cluster, the leader node assigns that queue to a *storage shard*.  Similarly,
  any time a queue qualifies to be garbage-collected, the leader node removes
  it from the cluster state.

- Any internal queue identifiers, and other miscellaneous details.

Note that cluster metadata does not include contents of the queues.

### Queues

Another type of information maintained by a BlazingMQ cluster is the messages
across various queues.

Any time a new queue comes into existence in a BlazingMQ cluster, the leader
node assigns it to an internal *storage shard*.  From that point onwards, the
primary node assigned to that *storage shard* is in charge of managing the
queue.  A primary node carries out things like:

- Receiving all messages arriving on the queue.

- Persisting messages locally and replicating them to the replica nodes.

- Acknowledging messages back to the producer applications at the appropriate
  time.

- Choosing the right set of consumers to route newly arriving messages.  This
  can involve determining consumers' current capacities, consumers' priorities,
  any filtering criteria specified by consumers, etc.

- Bringing a replica node which was recently (re)started up to date by
  synchronizing its *storage shards* so that it has the latest messages.

### Summary of Nodes' Roles

With the concepts of leader, follower, primary and replica nodes introduced
above, it is worth summarizing them here:

- **Leader**: A node in a BlazingMQ cluster is elected as leader with a quorum
  based election algorithm.  This node is in charge of managing BlazingMQ
  cluster metadata and replicating it to follower nodes, in addition to
  maintaining it in memory.  Every leader node gets assigned a unique,
  monotonically increasing number called the *term*.

- **Follower**: A node which maintains a local copy of the metadata (in memory
  as well as on disk) and updates it upon receiving notifications from the
  leader node.  By default, all nodes apart from the leader node in a BlazingMQ
  cluster become follower nodes and maintain metadata.

- **Primary**: A node which manages a *storage shard* in a BlazingMQ cluster
  and replicates it to the peer nodes (replicas).  The leader node manages the
  *storage shard* <-> primary node mapping.  The leader node typically assigns
  a primary node to a *storage shard* at startup or when an existing primary node
  goes away (crash, graceful shutdown, removal, etc).  The leader node can
  assign itself as the primary node of one or more *storage shards*.  Every
  primary node gets assigned a monotonically increasing number by the leader
  called the *PrimaryLeaseId*.

- **Replica**: A node which maintains a local copy of the *storage shard* and
  updates it upon receiving notifications from the primary node.  All nodes
  apart from the primary node become replicas of that *storage shard*.

The following combinations of roles are valid for a BlazingMQ node:

- A node can be the leader and the primary of one or more *storage shards* and
  a replica of the remaining *storage shards*.

- A node can be a follower and the primary of one or more *storage shards* and
  a replica of the remaining *storage shards*.

- A node can be the leader and a replica of all *storage shards*.

- A node can be a follower and a replica of all *storage shards*.

---

## Replication

As mentioned above, the leader node is in charge of replicating cluster
metadata to other nodes in the cluster, and the primary node of each *storage
shard* is in charge of replicating the shard to other nodes.  It is worth
pointing out that primary nodes are not elected but assigned by the leader
node.

Every node in the BlazingMQ cluster has a full copy of cluster metadata as well
as every *storage shard*.  In other words, nodes are homogeneous.

An interesting corollary of the above fact is that adding more nodes in a
BlazingMQ cluster increases availability of the system, but it does not
increase the cluster's capacity.  In fact, clients may observe lower throughput
because now the primary node has to carry out additional replication to the new
node.

Each *storage shard* has its own independent logical stream of replication,
which, among other things, includes sequence numbers for every packet
replicated by the primary node.  These sequence numbers are used by the replica
nodes to ensure that they stay in sync with the primary node, as well as to
request any missing parts of the stream at startup or in the event of a primary
node change.  Nodes in a BlazingMQ cluster maintain only one TCP connection
with each other, and replication for all *storage shards* occurs over that
connection.


### Eventual vs Strong Consistency in Replication

Upon receiving a new message from the producer, the primary node writes it to
its local storage and replicates it.  Depending upon a queue's static
configuration, the primary node may or may not wait for acknowledgements from
replica nodes before returning the result back to the producer.  If the queue
is configured in eventual consistency mode, the primary node will not wait for
acknowledgements from replicas with the hope that replicas will eventually
receive the message and apply it locally.

On the other hand, if the queue is configured in strong consistency mode, the
primary node will wait for acknowledgements from a certain number of replicas
before returning results to the producer.  The number of replicas is chosen in
such a way that ensures that a majority of the nodes have a copy of the message
before returning results to the producer.

Needless to say, while eventual consistency may provide lower latency, it comes
at the risk of potential message loss.  Consider a scenario where the primary
node crashes right after returning a "success" result to the producer, but the
replicated message is still sitting somewhere in its memory or socket buffer,
waiting to be sent to the replicas.  In this case, one of the replicas will be
promoted as a primary node by the leader node and neither the new primary node
nor other replicas will have the message, leading to message loss even though
the producer received a success notification from the previous primary node.

We do not recommend using eventual consistency mode unless you can tolerate
message loss.  By default, all queues are configured to be strongly consistent.

---

## Storage

Now that we know about high level concepts of clustering and replication in a
BlazingMQ cluster, it's time to look into the details of the storage layer and
understand how BlazingMQ message brokers interact with local disk.

### Storage Shard

A BlazingMQ cluster divides storage into shards, and distributes queues across
these shards.  A shard is also known as *partition* in BlazingMQ.  A shard can
be thought of as a logical file on disk (see next section for details).  A
shard is an internal detail of BlazingMQ and users don't need to know about it.
Every BlazingMQ cluster is statically configured with a number of shards as
well as the total storage capacity of each shard.  The number or size of shards
cannot be changed at runtime and requires a broker restart.  The number and
size of the shards is chosen considering the total storage capacity as well
as total anticipated number of queues that the cluster will host.  Typically,
the number of shards is much less than the total number of queues hosted on a
cluster.  For example, having less than 50 shards for a cluster which is
hosting 10,000+ queues is normal.  Every node in a BlazingMQ cluster has full
knowledge (and a full copy) of all the shards.

### Storage Shard Internals

Until now, a storage shard has been described as a logical file on disk.  To be
specific, a shard is a collection of two regular physical files on the
filesystem -- a *JOURNAL* file and a *DATA* file.

A JOURNAL file is, well, a journal of events.  All events, like the arrival of
a message in a queue, confirmation of a message by a consumer, deletion of a
message, creation of a queue, deletion of a queue, etc., are recorded in the
journal.  The DATA file is used to store the actual messages themselves.

Let's walk through an example -- when a message arrives from a producer for a
queue at the queue's primary node, it identifies the shard to which a queue has
been mapped, and "routes" the message to that shard ("routing" implies handing
over the message to the correct subsystem or thread).  The shard copies the
message payload to the DATA file, and creates a *MessageRecord* in the JOURNAL
file.  One of the fields in the *MessageRecord* is the offset of the message in
the DATA file, which can be used to retrieve the message later.  At some time
after writing the message to the files, the primary node replicates it to the
replicas, which then update their DATA and JOURNAL files of the same shard with
the message payload and *MessageRecord* respectively.  And some time after
that, the primary node replies to the producer with an acknowledgement.

BlazingMQ carries out strictly sequential writes to the JOURNAL and DATA files
for performance purposes.  In addition, files are memory-mapped, although this
can change in the future.  Memory mapping the files brings certain advantages
like avoiding system calls during read/write operations and easier file offset
management.  On the other hand, it can pollute the page cache and also
introduce some unpredictability in latency due to page cache misses.  BlazingMQ
message brokers do not flush memory-mapped files to disk and rely on the
operating system to periodically flush dirty pages to disk.  While in theory
this exposes a crashed node to data loss, in practice this does not happen.
Upon restart, the crashed node synchronizes its storage with peer nodes in the
cluster before going "live".  Failure of an entire cluster simultaneously could
indeed result in the loss of committed data, though at Bloomberg we defend
against this with a truly shared nothing architecture.

### JOURNAL File Details

As mentioned above, a JOURNAL file contains relevant storage events.  These
events are written to the JOURNAL file in the form of various fixed length
records (structs).  Types of records:

- *MessageRecord*: Record containing details about a message, like message
    GUID, queue name, time of arrival of message, CRC32C checksum, offset to
    message payload in the DATA file, etc.

- *ConfirmRecord*: Record containing details about confirmation of a message by
  a consumer, like consumer ID (if any), message GUID, queue name, time of
  confirmation, etc.

- *DeletionRecord*: Record containing details about deletion of a message, like
  message GUID, queue, reason of deletion (processed by all consumers,
  garbage-collected due to TTL expiration, purged due to poison pill detection,
  etc).

- *QueueOpRecord*: Record containing details about any queue related operations
  like queue creation, deletion, modification, etc.

- *JournalOpRecord*: A meta record containing periodic markers from the primary
  node in the replication stream, etc.

Every JOURNAL record is 60 bytes in length.  Having fixed length records helps
with bidirectional traversals and scans of records during storage recovery
phase at broker startup as well as when the primary node needs to heal a
recently started replica.


### DATA File Details

DATA files are simpler than JOURNAL files, because they just contain message
payload records.  Each record starts with a header describing the message
payload record.  Note than unlike the JOURNAL file, records in the DATA file
are of variable length.

More details about BlazingMQ's storage protocol can be found in the
[`mqbs_filestoreprotocol.h`](https://github.com/bloomberg/blazingmq/blob/main/src/groups/mqb/mqbs/mqbs_filestoreprotocol.h)
header file.  This file contains definitions of all the `struct`s as they are
laid out on disk.  To ensure consistency, data is written to disk in network
byte order.

---
