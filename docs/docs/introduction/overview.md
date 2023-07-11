---
layout: default
title: Overview
parent: Introduction
nav_order: 2
---

# BlazingMQ Overview
{: .no_toc }

* toc
{:toc}

## What is BlazingMQ?

BlazingMQ is generic message-oriented middleware usable for building
decentralized applications which communicate using message queues.  BlazingMQ
provides durable, fault-tolerant, highly performant, and highly available
queues, along with features like various message routing strategies (e.g., work
queues, priority, fan-out, broadcast, etc.), compression, strong consistency,
poison pill detection, etc.

Message queues generally provide a loosely-coupled, asynchronous
communication channel ("queue") between application services (producers and
consumers) that send messages to one another. You can think about it like a mailbox for
communication between application programs, where the 'producer' drops a message
in a mailbox and the 'consumer' picks it up at its own leisure.  Messages placed
into the queue are stored until the recipient retrieves and processes them. In other
words, producer and consumer applications can temporally and spatially isolate
themselves from each other by using a message queue to facilitate communication.

Within Bloomberg, the BlazingMQ framework processes billions of messages and
terabytes of data every day and is used in production by thousands of applications.

BlazingMQ client libraries are available in C++, Java and Python (the Python SDK
will be published shortly as open source). Within Bloomberg, client applications and
BlazingMQ clusters both run in a variety of environments, and BlazingMQ attempts to
provide a consistent user experience across heterogeneous deployments. In
general, BlazingMQ clusters can be hosted anywhere and do not depend on any
other Bloomberg-specific or open source frameworks.

---

## Benefits of BlazingMQ

BlazingMQ's goal is to provide application developers with a well supported, feature rich, and highly
performant message queuing framework. Here's a summary of its benefits:

### Durability and High Availability
{: .no_toc }

It provides durable, highly available, and efficient queues, which enables
asynchronous and loosely-coupled communication between applications. Queues
can be replicated across data centers, thereby ensuring business continuity in
case of disaster recovery (DR) scenarios.

### Transport Abstraction
{: .no_toc }

It abstracts the transport and network, and producer and consumer applications
don't need to worry about the underlying transport or one another's geographic
locations.

### Message Routing Strategies
{: .no_toc }

It enables applications to implement various enterprise architecture patterns
as a result of its rich set of message routing strategies -- work queue,
priority, fan-out, broadcast, request/response, etc.  See [*Routing
Patterns*](../../features/message_routing_strategies) for more details.

### Rich Feature Set
{: .no_toc }

In addition to the above, it comes with additional features like compression,
poison pill detection, a pluggable architecture, configurable consistency, a rich
set of APIs in C++, Python, and Java SDKs, etc. See
[*Features*](../../../features) for more details.

### High Performance
{: .no_toc }

It provides low latency and high throughput at enterprise scale. Applications
can rely on BlazingMQ to be highly performant. See
[*Performance*](../../performance/benchmarks) for more details.

### Reliability
{: .no_toc }

It provides a high level of reliability and attempts to protect applications
from transient network or hardware disturbances. A BlazingMQ cluster can
disappear from the network for a few (configurable) minutes, during which time producer
applications can continue to submit work to it without noticing any errors.
See [*High Availability*](../../architecture/high_availability) for
more details.

### Extensive Metrics
{: .no_toc }

It provides a rich set of monitoring metrics to help users determine and
understand the behavior of their applications, as well as the BlazingMQ clusters.

---

## Motivation for BlazingMQ

BlazingMQ's development at Bloomberg more than eight years ago began at a time when similar open source
and proprietary systems were immature, did not possess features we were
looking for, had exorbitant licensing fees, or had questions about their
reliability and performance. In addition, Bloomberg engineers had prior
experience building other in-house middleware frameworks, and the decision to
implement an in-house message queuing system was determined to be the right
one.

Over the last several years, BlazingMQ has become a compelling message queuing
solution at Bloomberg, thanks to its performance, reliability, and features.
See [*Comparison with Alternatives*](../comparison) where we carry out
a high-level comparison of BlazingMQ with two of the most similar and dominant message queueing
systems today.

---

## Open Source Release

This is BlazingMQ's first open source release, and we are publishing BlazingMQ
message brokers, as well as client libraries in C++ and Java in this release.
As with any mature enterprise system, BlazingMQ has a thriving ecosystem around
it within Bloomberg, comprising of configuration management, self-service,
Python client libraries, transport adapters, monitoring, alarming and testing
(stress, fuzz, chaos, etc.). Most of these systems are closely integrated with
Bloomberg enterprise, so publishing them as-is in the open is not
feasible. In the coming months, we will be working towards publishing some of
these systems as open source as well to ensure that BlazingMQ's ecosystem continues to
grow outside of Bloomberg. Details can be found in the BlazingMQ
[*Roadmap*](../roadmap) section.

We hope that you'll give BlazingMQ a shot! Please don't hesitate to [reach
out](https://github.com/bloomberg/blazingmq/issues) to us if you have any
questions or feedback!

---
