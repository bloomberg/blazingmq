---
layout: default
title: Concepts
parent: Introduction
nav_order: 3
---

# BlazingMQ Concepts
{: .no_toc }

Let's review some high level, user-facing concepts in BlazingMQ.

* toc
{:toc}

## Domain

A domain is the top level namespace for a BlazingMQ user. A domain captures
all the necessary attributes like storage quota, message expiration time (TTL),
routing strategy (priority, fan-out, broadcast), etc. Multiple queues can be
created in a domain, and all such queues inherit their domain's attributes. A
domain can be thought of as a collection of segregated streams of data, where
each queue in the domain represents one data stream.

Queues in a domain do not need to be declared or created upfront. A queue will be
created automatically when it is used for the first time.

Depending on their needs, a BlazingMQ user can have multiple domains (with
each domain containing one or more queues). This is usually needed when a user
wants different sets of data streams, with each set behaving differently for storage quota, routing, and other configurations.

### Domain Attributes

A domain has several configuration parameters which dictate various functional
aspects of the domain. A detailed overview of domain configuration can be
found [here](../../installation/configuration#domain-configurations), but let's
go over some of the important attributes:

- **Routing Strategy**

  This attribute describes the routing strategy to be used by a BlazingMQ cluster
  for the queues belonging to the domain. More details about various routing
  strategies can be found [here](../../features/message_routing_strategies).

- **Storage Attributes**

  These attributes describe storage-related configuration settings for a
  BlazingMQ domain. They include:

  - Maximum storage quota for the domain
  - Maximum storage quota for a queue in the domain (usually smaller than
    domain's quota)

  Note that *storage quota* captures numbers, as well as bytes, of messages.

- **Message TTL**

  Amount of time after which messages will be garbage-collected by BlazingMQ if
  not consumed by the application. TTL duration can vary from a few minutes to
  a few days.

- **Maximum Delivery Attempts**

  Total number of times a message will be transmitted to consumers if a
  consumer crashes without confirming the message, indicating that the message is
  likely a poison pill message. The recommended value is five. See [*Poison
  Pill Detection*](../../features/poison_pill_detection) for more details.

- **Deduplication Time Interval**

  Total time interval for which a BlazingMQ cluster will keep track of
  *MessageId*s of the messages posted on the queue for the purpose of
  deduplication. See [*High
  Availability*](../../architecture/high_availability) for more details.

- **Replication Consistency**

  Type of consistency for storage replication. There are two choices --
  eventual and strong. Strong consistency is strongly recommended (pun
  unintentional). See [this](../../features/consistency_levels) for more
  details.

- Miscellaneous Settings

  A BlazingMQ domain contains some other configuration parameters described
  below:

  - *MaxConsumers*: Maximum number of consumers that can attach to a queue

  - *MaxProducers*: Maximum number of producers that can attach to a queue

  - *MaxQueues*: Maximum number of queues that can be created in a domain

  - *MaxIdleTime*: Amount of time interval after which BlazingMQ cluster will
    log an error message in its log if no messages were confirmed by the
    consumers during that interval. This error message can be used to raise an
    alarm in certain deployments.

---

## Queue

A queue in BlazingMQ represents a stream of data over which producer and
consumer applications exchange data. A queue decouples applications from one
another by acting as an intermediary, so that applications don't have to worry
about each other's physical locations or lifetimes. In other words, a
queue provides temporal and spatial isolation between various applications.

In addition, a queue also protects consumer applications from bursty traffic by
absorbing data spikes and enabling consumers to consume messages at their own
pace.

### Identifying a Queue

A queue in BlazingMQ is identified by its *Uniform Resource Identifier* (URI).
The queue URI follows [RFC 3986](https://datatracker.ietf.org/doc/html/rfc3986)
and typically looks like `bmq://foo.bar.baz/quux`, where:

- Leading `bmq` is the URI scheme

- `foo.bar.baz` is the BlazingMQ domain name (the URI authority)

- `quux` is the BlazingMQ queue name within the `foo.bar.baz` domain namespace
  (the URI path)

More details about the format of a BlazingMQ queue URI can be found
[here](../../apidocs/cpp_apidocs/group__bmqt__uri.html). Applications can
address a queue by its URI.

### Lifetime of a Queue

Queues in BlazingMQ are completely dynamic, and applications don't have to worry
about managing a BlazingMQ queue's lifetime. They do not need to be explicitly
created or declared before being used. When an application gets a handle on a
queue (by using one of the flavors from `openQueue` API), the queue is
automatically created under the hood by a BlazingMQ cluster if it does not already exist.

Similarly, applications don't have to worry about deleting a queue. A BlazingMQ
cluster automatically garbage-collects a queue if it has no outstanding
messages and no applications are attached to it.

This feature removes the burden on applications to manage a queue's lifetime. It also enables a usage pattern where two or more applications
create BlazingMQ queues which are tied to their own lifetimes. For example, while running, a
pair of producer/consumer applications can exchange messages over a unique
queue for their "private" communication, forget about the queue
when they exit, and create a new one upon restart.

On the other hand, some applications will continue to use the same queue
in production for many months and years.

### Persistence and Durability

A very important attribute of a BlazingMQ queue is its durability. All queues are
persisted and replicated. [Learn more](../../architecture/clustering) about clustering, storage, and replication in BlazingMQ.

### Ordering in a Queue

Each BlazingMQ queue is assigned a primary node in the BlazingMQ cluster, and
that primary node enforces an order on the messages arriving in the queue.
Depending upon the queue's [*Routing
Strategies*](../../features/message_routing_strategies), consumers attached to
the queue may or may not see messages in order. Additionally, consumers can
see out of order messages in case a consumer application crashes or shuts down
without confirming that all of the messages were routed to it.

---

## Message

A message is the basic unit of information that is exchanged between producer
and consumer applications. In BlazingMQ, a message contains various attributes
like:

- **Message Payload**

This field contains the message's contents. BlazingMQ APIs provide setter and
getter functions for the message payload as binary data. Message payload is
completely opaque to BlazingMQ and no parts of the system peek into it.
Applications can serialize and deserialize messages using their favorite codecs
(Protobuf, Avro, JSON, etc.).

- **Message Properties**

A producer application can associate an optional list of key/value pairs with
every message. These key/value pairs are known as message properties in
BlazingMQ. Applications can capture any meta information in the message
properties, which could be useful for tracing, logging, and routing in the
application ecosystem.

- **Message Identifier**

BlazingMQ assigns a unique message identifier to every message published by the
producer application. This identifier is sent back to the producer in the
message's acknowledgement notification, as well as to the consumer. BlazingMQ
message brokers also use these identifiers to deduplicate any retransitted
messages by maintaining a moving window of previously seen identifiers.

### Message Acknowledgement

For every message a producer application publishes, it receives a reply
for it from BlazingMQ indicating the operation's result. This reply is known
at *message acknowledgement* or *acknowlegement* or just *ACK* in BlazingMQ.
Acknowledgement can be negative, which indicates BlazingMQ's failure to accept
the message for various reasons (like a queue's storage quota has been reached, a long
running network connectivity issue, etc). However, BlazingMQ tries to minimize
negative acknowledgements as much as possible by seamlessly buffering and
retransmitting messages whenever possible.

In order to support high throughput workflows, acknowledgements are delivered
asynchronously to the producer applications and may be delivered in batches.

### Message Confirmation

Once a consumer application has finished processing the message, it needs to
notify BlazingMQ so the message can be removed from the queue. This notification is called *message confirmation* or *confirmation* or
just *CONFIRM* in BlazingMQ. BlazingMQ client library provides an API to send confirmation to the queue.

---
