---
layout: default
title: Comparison With Alternative Systems
parent: Introduction
nav_order: 5
---

# Comparison With Alternative Systems
{: .no_toc }

* toc
{:toc}

## Introduction

We compare BlazingMQ with two relevant and prominent open source distributed messaging systems -- RabbitMQ and Apache Kafka. While Apache
Kafka doesn’t advertise itself as a message queue system, we include it here
because it is widely used for its messaging features and overlaps with both
BlazingMQ and RabbitMQ as far as its features and usage are concerned.

In the enterprise environment where BlazingMQ originated, all three systems
have been offered as managed services in production and complemented each other
in a complete messaging infrastructure package. Some of the insights presented
in this comparison are drawn from the wealth of knowledge accumulated over the
years by offering all three technologies as a service.

One of the goals of this document is to demonstrate that there is a unique
combination of simplicity, performance, fault tolerance, and features that
makes BlazingMQ competitive as a distributed messaging system.

We also highlight that there may be no simple answer to the question “Which
messaging system should I use?” due to the complexity of factors involved. Readers looking for a straightforward answer to that question will likely be
disappointed. We deliberately avoid making a recommendation for a specific
messaging system because, as we will see, each system is unique and better than
the others in some aspects while lacking in other areas. Potential users are
encouraged to carry out their own research and comparison of these systems,
keeping in mind their current and potential future use cases.

We assume the reader has a high-level understanding of distributed
messaging systems. However, in order to keep the document accessible to a
wider audience, we deliberately avoid using terminology specific to any of the
systems under consideration.

In order to understand and compare these three systems, we decompose their
architecture and functionality across critical dimensions like storage and
replication, message routing, fault tolerance, features, etc. In the next
sections, we survey these systems across these dimensions.

---

## Storage

### Delivery Guarantees and Message Lifetime

All three messaging systems offer ‘at-least once’ message delivery guarantee
which requires persistence (storage) and inevitably, replication. Apart from
storage, another important aspect in the life cycle of produced messages is
data routing (i.e., the logic of distributing messages among consumers). Kafka
stands apart from BlazingMQ and RabbitMQ
([Streams](https://www.rabbitmq.com/streams.html) aside) here by decoupling
storage from data routing. Reads in Kafka are not destructive (i.e., a
message’s lifetime does not depend on message delivery or message processing).
Messages are deleted according to the configured cleanup policy (e.g., based on
time, size, compaction). This design choice permits Kafka consumers to read
from the “past” (rewind or replay). This also requires each Kafka consumer to
know the offset from where to read the data.

On the other hand, BlazingMQ and RabbitMQ keep messages only until they are
consumed. Consumers in these systems cannot go back to access messages which
were previously processed.

### Storage and Routing

BlazingMQ and RabbitMQ further differ in message storage in the following ways:

- BlazingMQ stores messages first and then routes, while RabbitMQ routes (via
  Exchange) before storing (to a queue). BlazingMQ’s approach ensures that the
  cost of storage and replication is constant, irrespective of fan-out ratio
  (number of consumers); however, this requires BlazingMQ to know the fan-out
  ratio up front. Meanwhile, RabbitMQ's approach offers the flexibility of any
  number of consumers joining the fan-out routing. But this comes at the expense of higher
  costs (CPU, disk, and network bandwidth), proportional to the number of
  routing destinations.

- At the storage level, BlazingMQ differs from RabbitMQ by maintaining a log of
  messages on disk. In other words, BlazingMQ does not delete a message from
  the disk right away; it simply marks it as deleted or consumed. It deletes the
  entire log when the log reaches its maximum configured size and all messages
  in the log have been marked as consumed. On the other hand, RabbitMQ classic
  queues keep track of messages in Erlang’s distributed database (Mnesia) and
  deletes the messages from disk as they are consumed. At present, RabbitMQ quorum
  queues store the complete message in the Raft log (stored on disk) and
  truncate the log when safely possible.

---

## Data Routing

As mentioned earlier, data routing is the logic of distributing messages among
consumers. Consumers can compete with each other, advertise
different priorities, request selective messages (interest-based routing), or
even be absent while messages are produced.

All three messaging systems recognize simple routing modes such as round-robin
and fan-out, and also dynamically adapt to changing consumers. As we will see
below, BlazingMQ and RabbitMQ share several similarities in data routing and
provide more flexibility, while Kafka stands out with its design choices in
routing.

### Load Balancing

In BlazingMQ and RabbitMQ, consumer(s) explicitly attach to queue(s); the
systems do not attempt to assign consumers to queues. On one hand, this
gives consumers finer control over which queues to consume from. On the
other hand, this requires them to know queue names a priori. Consumer groups
in Kafka, however, assign partitions to consumers and ensures that every
partition is assigned. Alternatively, consumers can assign a specific set of
partitions manually.

In both BlazingMQ and RabbitMQ, multiple consumers can attach to a queue
configured in a round-robin mode, and the system will load balance messages
across all consumers. In other words, adding new consumers to a queue
automatically increases processing capacity. This may not be possible in Kafka
if there are no partitions to consume from. For example, if a fifth consumer
joins a Kafka topic with four partitions (and four consumers), the new consumer
will stay idle. In Kafka, load balancing of messages can occur across
partitions in a topic from the producer side (i.e., messages can be posted
across different partitions in a topic at the time they are produced). In
other words, unlike in BlazingMQ and RabbitMQ, adding a new consumer in Kafka
does not decrease the load on existing consumers; adding new partitions does.

### Consumer Priority

BlazingMQ and RabbitMQ both support the notion of priority in consumers (not to
be confused with priorities in queues, which are supported by RabbitMQ -- with some caveats). Priorities in consumers allow applications to control the
flow of messages across their consumer instances and implement a desired
failover behavior (messages will flow to lower priority consumers when higher
priority consumers go away). In the case of RabbitMQ specifically, a large
enough flow can direct messages to lower priority consumers even while higher
priority consumers are still present. Kafka, however, does not support
priorities in consumers. Furthermore, failover of consumers in Kafka may lead
to a significant change in the partition <=> consumer assignment mapping.

### Topic Based Routing

Both BlazingMQ and RabbitMQ support topic-based routing, where queues/consumers
can specify criteria (a filter or an expression) such that only those messages
which satisfy the given criteria are routed to the consumer(s)/queues(s). In
other words, topic-based routing gives applications the ability to support
heterogeneous consumers by breaking the stream of data into a dynamic,
non-fixed number of independent logical sub-streams using flexible criteria and
produced data.

### Broadcast Routing Strategy

BlazingMQ supports [*broadcast
strategy*](../../features/message_routing_strategies#broadcast-mode) for
message routing which provides ‘at-most once’ delivery guarantees. This is a
direct consequence of the fact that, in this strategy, BlazingMQ neither
persists messages on disk nor buffers them in memory. Instead, they are
immediately dispatched to the consumers attached to the queue at that instant
and then "dropped" by BlazingMQ. BlazingMQ makes no effort to keep messages
for consumers which may arrive at a later time. As a result of no buffering,
persistence, or replication of messages, broadcast strategy has
[better](../../performance/benchmarks#broadcast-routing-strategy) latency and
throughput numbers compared to other routing strategies. To the best of our
knowledge, this routing strategy is unique to BlazingMQ. This strategy can be
useful for scenarios where consistently lower latency is extremely
important and occasional data loss is acceptable.

---

## Cluster Topology

The topology of the system affects both its functionality (particularly data
routing), as well as performance. The topology of a system includes composition and placement of cluster nodes, as well as any other participating nodes.

### Cluster Membership

BlazingMQ replicates data across a fixed sized cluster. All cluster nodes in
BlazingMQ are equal as far as storage is concerned. In other words, every
cluster node in BlazingMQ has a full copy of the data. This means that the replication factor for every queue in a
BlazingMQ cluster is equal to the size of the cluster. This property has an interesting outcome – adding a new node
in a BlazingMQ cluster increases its availability, but not throughput or
capacity. While this approach simplifies BlazingMQ’s replication and cluster
state management logic, it comes at the expense of fixed cluster size. Cluster
size or membership in BlazingMQ can be changed only by restarting all nodes in
the cluster and requires some operational overhead.

RabbitMQ supports a wide range of possible deployment/replication
topologies. Clusters can change size at runtime, and individual queues can
operate with different - and dynamically configurable - replication sizes and
patterns (provided that policies on both the broker and clients are configured
appropriately).

Kafka dissociates replication factor with cluster size, and adding more nodes
in a Kafka cluster increases the cluster’s capacity. Kafka achieves this by
spreading partition replicas across the cluster such that each node in the
cluster replicates a subset of all the partitions hosted in the cluster.

### Dependency on Other Systems

BlazingMQ does not depend on any other software framework. RabbitMQ runs on
the Erlang VM. Both systems are self-sufficient and can host their data, as
well as metadata, by themselves. This is unlike Kafka, which requires Apache
ZooKeeper for storing metadata. However, Kafka is in the process of getting
rid of its dependency on ZooKeeper and will soon store its data and metadata
within the Kafka cluster itself. Lastly, Kafka runs on the Java Virtual Machine.

### Proxies

A unique feature of BlazingMQ is the presence of proxy nodes. BlazingMQ
applications have the option to connect to proxies instead of connecting
directly to the BlazingMQ cluster nodes. Proxies don’t participate in data
storage and replication, but they are part of routing, and are particularly helpful
with very large fan-out ratio scenarios. Proxies also protect applications
from transient network issues in the BlazingMQ cluster by providing seamless
buffering, retransmission, or retries of messages as appropriate.

As a result of proxies, BlazingMQ dynamically creates a distribution tree for every queue,
which is rooted at the queue’s primary node. Replicas are the primary's child nodes,
and proxies are replicas’ child nodes. Producer and consumer applications
for the queue appear as leaf nodes in this tree. See [*Network
Topology*](../../architecture/network_topology) for more details.
Unlike BlazingMQ, applications using RabbitMQ and Kafka connect directly to
cluster nodes, and neither of these systems support the notion of proxies, as found in BlazingMQ.

---

## Fault Tolerance

All three systems support 'at-least once' guarantees of message delivery, which
requires persistence, as well as replication. They also support strongly
consistent replication, where a producer is notified of success when N nodes in
the cluster have accepted the messages, where the value of N is configurable
and can be chosen to ensure consistency (by perhaps sacrificing availability)
in scenarios like network splits. Note, however, that classic mirrored queues
in RabbitMQ are [known](https://aphyr.com/posts/315-jepsen-rabbitmq) to be
susceptible to loss of consistency during network partitions, which led to the
development of *Raft*-based "quorum queues" in RabbitMQ v3.8. BlazingMQ
successfully passes our chaos tests built on top of
[Jepsen](https://github.com/jepsen-io/jepsen) and our chaos testing suite will
be published as open source in the coming months.

---

## BlazingMQ Use Cases

There are several use cases at Bloomberg which BlazingMQ solves optimally.
While we cannot provide specific details about these use cases, we capture enough
information for readers to appreciate how BlazingMQ supports them.

### High fan-out ratio with high data volume

Courtesy of its multi-hop network topology and its ability to build a
multi-level distribution tree for a queue as described [here](../../architecture/network_topology), one area where BlazingMQ
excels is in its support of high fan-out with high data volumes. Consider a scenario where
there are thousands of consumers (*N*) attached to the queue, all of which are interested in
receiving every message posted in the queue, and the producer posts at a rate
of hundreds of messages per second (*M*). In such case, due to fan-out, the
messaging system would needs to deliver an aggregated *M * N* messages per
second. Picking *N* = 5000, and *M* = 1000, the aggregate comes out to be
5,000,000 messages per second on the outbound. In the absence of a
distribution tree, the BlazingMQ primary node would have been required to
support this outbound rate. However, due to its topology, BlazingMQ primary
node may see a fan-out of only 10s (i.e., *N* = 10), leading to *drastic*
improvements in latency, as well as bandwidth savings.

### Storage Savings

As mentioned in one of the previous sections, an interesting property of
BlazingMQ is that messages are stored once, irrespective of the number of
consumers in the fan-out mode (recall that BlazingMQ stores the message before
routing it). This can lead to bandwidth and capacity savings, especially for
queues with a higher number of consumers. There are several instances at
Bloomberg where queues have fan-out ratios of 50+, which immediately leads to
storage savings of 50x when compared to RabbitMQ.

### High number of dynamic queues

Some applications prefer to create a large number of queues (hundreds or
thousands) in order to have fine-grained control over various data streams in
their ecosystem. Applications can implement their own sharding schemes to
distribute messages across these queues. In addition, some of these queues are
long-lived, while others are short-lived. For example, a well-known queue
which contains financial trade messages could be long-lived, while a queue
which represents private communication between two processes could be short-lived. BlazingMQ helps applications achieve such design for various reasons –
in BlazingMQ, queues are cheap, do not need to be declared up front (queues are
created automatically when they are opened for the first time), and are garbage
collected automatically when they are empty and unused for some time. There
are several instances at Bloomberg in which a BlazingMQ domain has close to
10,000 queues.

### Mixed workload: different delivery guarantees for different data streams

There are occasions where an application ecosystem has different data streams
in their workflows, some of which require higher delivery guarantees (e.g., financial
trade processing workflow), while others are okay with lower guarantees (e.g.,
cache invalidation notifications). BlazingMQ has proven to be a good fit for
such scenarios since it can provide both guarantees depending on the queue mode
– priority or fan-out mode for at-least once delivery guarantees, and broadcast
mode for at-most once delivery guarantees. This is convenient for applications
because they can use the same middleware and same set of APIs in their
applications and leverage both delivery guarantees, instead of using another
middleware or raw TCP for *at-most once* delivery guarantees and a message
queue for *at-least once* delivery guarantees. For some BlazingMQ users at
Bloomberg, certain queues are priority or fan-out mode, while others are
broadcast mode, and applications are using these queues to implement a variety
of workflows like request/response, fan-out, one-way notifications, etc. For
example, some applications broadcast requests to all instances of their
workers because they may not know up front which worker can handle the request,
while workers reply to the request over a priority queue back to the requester.

### Good combination of features, performance and reliability

Finally, BlazingMQ provides a good user experience along with its broad set
of features, so users can rely on BlazingMQ for both good performance and reliability. In most cases, BlazingMQ provides a median latency of ~5ms or
less with high data volumes, hides applications from transient network, software
and hardware issues, as well as provides a set of features to help users
implement their distributed messaging ecosystem in an efficient manner.

---

## Conclusion

In the last few decades, the world has moved towards extremely large, data-intensive workloads, and the need for reliable and efficient messaging systems has also
increased and diversified. As we demonstrated in this document, no one system can provide a perfect solution across all dimensions for all
applications. On several occasions, Bloomberg engineers have chosen BlazingMQ
over RabbitMQ and Apache Kafka for their messaging needs because it has provided them with a
clear advantage and solved their use cases in the most optimal way. As
we continue to evolve BlazingMQ in the coming months and years, we expect it to
become an even more compelling messaging solution for engineers outside of
Bloomberg as well.

---
