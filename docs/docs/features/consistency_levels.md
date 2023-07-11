---
layout: default
title: Consistency Levels
parent: Features
nav_order: 6
---

# Consistency Levels in Storage Replication
{: .no_toc }

* toc
{:toc}

## Introduction

Distributed systems, particularly distributed data stores, offer different
consistency levels for various operations initiated by clients.  The
consistency level of a system defines the ordering and visibility of operations
in the system.

Knowing the consistency level of a distributed system can help users design
their applications, as well as understand the scenarios in which operations
initiated by them can succeed or fail.

Several consistency levels exist in the world of distributed systems, but in
the context of messaging systems like BlazingMQ, only two consistency levels
are worth discussing -- *Eventual Consistency* and *Strong Consistency*.  The
former is sometimes also referred to as *Weak Consistency*.

The rest of this section describes eventual and strong consistency levels in
the context of BlazingMQ, compares the two consistency levels and also goes
over some additional considerations, which will help BlazingMQ users choose the
correct level of consistency for their BlazingMQ domains.

---

## Eventual Consistency

For a queue that is configured with the eventual consistency level in
BlazingMQ, when the primary node of the queue receives a *PUT* message, it
writes the message to its local storage, *asynchronously* sends it to the
replicas, and immediately sends the *ACK* back to the producer application,
without waiting for any confirmation from the replicas.

This configuration relies on the fact that the message will *eventually* reach
the replicas, which will then apply and store the message in their local
storages respectively.

Readers may have noticed that there is a chance of message loss in eventual
consistency mode.  This can occur when the primary node crashes (or goes down
gracefully) immediately after sending the *ACK* to the producer, but before the
message could be replicated to all or to the majority of the replicas (the
message could still be in the user or kernel space TCP socket buffers).
Additionally, it is also possible for the replicated message to get lost
because of a network split etc, in which case replicas will not receive the
message.  This is the risk that comes with eventual consistency.

However, the latency numbers for eventually consistent queues are very
attractive.  Since a primary node performs minimal work between receiving a
*PUT* and sending an *ACK*, the producer application can expect the *ACK*
fairly quickly.  Additionally, consumers will also see the corresponding *PUSH*
message earlier, because the primary node sends it right away to the consumer
application, without waiting for acknowledgements from replicas.

So when choosing an eventual consistency level, BlazingMQ users should know
that while this consistency level will give low latency numbers, there is a
non-zero probability of a message getting lost.

We recommend using eventual consistency only in exceptional circumstances.

---

## Strong Consistency

A strong consistency level in BlazingMQ solves the problem of potential message
loss, which can occur in eventual consistency mode as described above.  This is
done by ensuring that the primary node waits for acknowledgements (known as
*Replica Receipts* in BlazingMQ lingo) from a sufficient number of replicas
*before* sending *ACK* and *PUSH* messages to the producer and the consumer
applications respectively.

This ensures that even if the primary node crashes after sending an *ACK*
message, replicas are guaranteed to have the *PUT* message, and as one of the
replicas gets promoted to the role of primary, it will be able to send that
message to the consumer application.

The number of replicas which must send a receipt to the primary node for a
message to be considered "committed" is derived from the size of the BlazingMQ
cluster.  In a cluster having 4 nodes, the primary node will wait for receipts
from 2 replicas, which will ensure that the message ends up in at least 3 nodes
(2 replicas plus 1 primary).  Similarly, in a cluster of 6 nodes, the primary
node will wait for receipts from 3 replicas, thereby ensuring that the message
has been stored in at least 4 nodes.  Generally speaking, to ensure strong
consistency, BlazingMQ ensures that a total of `N/2 + 1` nodes in the cluster
have recorded the message, where `N` is the total number of nodes in the
cluster.

While strong consistency provides higher message guarantees, it can come with
some latency overhead.  However, we have introduced several optimizations in
the strong consistency implementation in BlazingMQ, and our tests have
indicated that latency numbers for strong consistency are only slightly higher
than the ones for eventual consistency.

Some of the optimizations in the implementation include:

- Replicas sending a receipt message to the primary for a batch of messages,
  instead of a receipt for every message.

- Replicas further optimizing the receipt code path by taking into
  consideration if the link between the replica and the primary is overloaded,
  and trying to conflate the receipt messages.

---

## Other Considerations

### Timeouts at Primary Node
{:.no_toc}

Since the primary node waits for replicas to send replication receipts, it is
possible for this operation to time out due to widespread BlazingMQ cluster or
network issues.  In such scenario, the primary node will issue a negative *ACK*
with an *UNKNOWN* status to the producer application, and will give up on
delivering this message to the consumer(s).  Note that the message can still be
delivered in the scenario where the current primary node goes down and a replica
which received the message (and perhaps even sent its receipt) gets promoted as
the new primary.  In such case, the new primary will send the message to the
consumer.  Hence the *UNKNOWN* status in negative *ACK*.  This timeout is
configurable.

### Flushing Messages to Disk
{:.no_toc}

Note that BlazingMQ does not flush messages to disk in either consistency level
before sending an *ACK* to the producer.  Instead, it relies on the OS to do so
periodically.

---
