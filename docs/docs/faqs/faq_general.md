---
layout: default
title: General FAQs
parent: FAQs
nav_order: 2
---

# General FAQs

## What platforms are supported by BlazingMQ?

BlazingMQ can run on Linux, AIX and Solaris operating systems.  The minimum
required versions of these operating systems are as follows:

| Operating System | Version |
| ---------------- | ------- |
| AIX              | 7.1     |
| Linux            | 3.10.0  |
| Solaris          | 5.11    |

Any operating system not listed above, or any operating system listed but with
a version less than the minimum specified version is not supported.

For development, BlazingMQ can also be built on Darwin, though production usage
on Darwin is not supported.

BlazingMQ message broker and C++ client library requires the following minimum
compiler versions.  Any compiler not listed, or any compiler listed but with a
version less than the minimum specified version is not supported.

| Compiler       | Version |
| --------       | ------- |
| GCC            | 7.3     |
| clang          | 10.0.1  |
| Solaris Studio | 5.13    |
| xlc            | 16.1.0  |

BlazingMQ message broker and C++ client library require the C++03 version of
the C++ language standard, or any version above it.

BlazingMQ Java client library is supported on JDK 8, JDK 11 and JDK 17.

---

## In what languages are BlazingMQ client libraries available?

BlazingMQ client libraries are available in C++, Java and Python.  C++ and Java
client libraries are available in the initial open source release, and Python
client library will follow soon.

---

## How do I get started with BlazingMQ?

[This](../../getting_started/blazingmq_in_action) article is a good starting
point to set up a local BlazingMQ environment and play around with it.

---

## Does BlazingMQ provide guaranteed message delivery?

When a producer application posts a message on a BlazingMQ queue, BlazingMQ
guarantees that it will either reliably accept the message or reliably inform
the producer that it failed to do so.

Above statement may come as a surprise to some readers, but the fact is that no
framework can guarantee 100% success for its APIs.  BlazingMQ may refuse to
accept a new message in a queue for various reasons like queue storage quota
getting full, a long running network outage, etc.

To summarize, BlazingMQ provides these guarantees:

- It will always notify the producer of the result of a *post* operation.  The
  result can be a failure in some rare scenarios due to reasons mentioned
  above.

- Once a message has been accepted by BlazingMQ, it is guaranteed to be
  delivered to consumer application(s), as long as the message is not
  garbage-collected by BlazingMQ due to message's *TTL* expiration.

---

## Can the same message be delivered more than once to consumer(s)?

Yes.  BlazingMQ provides *at-least-once* delivery guarantee.  BlazingMQ can
send a message more than once to the same or different consumer.  This never
occurs in steady state but can occur in these scenarios:

- A consumer crashes without confirming a message
- A node in BlazingMQ back-end crashes
- A node in BlazingMQ back-end restarts gracefully

A consumer application must be able to handle duplicate delivery.

---

## Do queues need to be created or declared upfront in BlazingMQ?

No.  Applications don't need to bother with creating or deleting queues.  Once
they have registered their BlazingMQ namespace ("domain") with the BlazingMQ
cluster, their applications can simply open a queue (via the `openQueue` API)
and BlazingMQ will automatically create the queue if it does not exist.

Similarly, once a queue is empty and there are no producers and consumers
attached to the queue, BlazingMQ will garbage-collect the queue automatically
after a period of inactivity.

---

## How many queues can I create under my BlazingMQ namespace?

As mentioned above, a queue is created automatically by BlazingMQ when it is
opened for the first time by applications.  The representation of a queue in
BlazingMQ cluster is very cheap and one can create tens of thousands of queues
in their BlazingMQ namespace.  However, having tens of thousands of queues can
make events like application restart, BlazingMQ node restart or fail-over take
longer than usual (up to 10 seconds).  This is because as part of fail-over
operation, all those queues may need to be opened again, and routes for all the
applications using those queues may need to be re-established.

Additionally, having tens of thousands of queues may make troubleshooting
extremely difficult as events across all those queues will be interspersed with
each other.

Quite often, the need for having thousands of queues can be eliminated by
application level sharding -- grouping some streams together into one queue,
thereby reducing number of queues from thousands to hundreds or tens.

Having said that, some BlazingMQ namespaces have several thousands of queues.

---

## How does BlazingMQ compare to RabbitMQ and Apache Kafka?

An article comparing these three frameworks can be found
[here](../../introduction/comparison).

---

## Where can I find performance numbers for BlazingMQ?

Details about BlazingMQ latency numbers can be found in
[this](../../performance/benchmarks) article.

---

## How do I configure and deploy a BlazingMQ message broker, cluster or domain?

Details about configuration and deployment can be found
[here](../../installation).

---
