---
layout: default
title: Benchmarks
parent: Performance
nav_order: 2
---

# BlazingMQ Performance
{: .no_toc }

* toc
{:toc}

## Introduction

Benchmarking any complex distributed system like BlazingMQ can be tricky
because there are several moving parts - there is no one latency number for
BlazingMQ, and any performance numbers are affected by factors including but
not limited to:

- Size of the BlazingMQ cluster and replication factor

- Storage (no persistence, in-memory, HDD, SSD, etc)

- Hardware and network capabilities

- Producer message rate, message size and batch size

- Producer publishing pattern (smooth vs bursty)

- Number of consumers (fan-out ratio)

- Network topology (location of producers/consumers, cluster nodes, etc) which
  has direct impact on ping latency among nodes

- BlazingMQ message broker configuration (number of threads, initial size of
  various memory pools, etc)


## Benchmarking Setup

We provide results of a recent benchmark of BlazingMQ with this setup:

- Six nodes in the BlazingMQ cluster

- Each node with local SSDs attached to it

- Nodes geographically spread across such that `ping` latency between them was
  around 1.5 milliseconds

- Producer and consumer applications were running on other nodes and instead of
  connecting directly to the BlazingMQ cluster, they connected to BlazingMQ
  proxies (see [*Alternative
  Deployment*](../../architecture/clustering#alternative-deployment) for
  details about BlazingMQ proxies).

- Queues storage replication was configured for strong consistency i.e.,
  primary node waited for acknowledgement from enough replicas such that the
  majority of the nodes recorded the message before returning success to the
  producer applications.  So in this case, primary waited for acknowledgement
  from three replica nodes before replying to the producer.  This ensured that
  a total of four nodes (primary and three replicas) recorded the message.

- Queues with various [*routing
  strategies*](../../features/message_routing_strategies) were
  tested.

It is worth mentioning that the cluster setup described above is an extreme one
and the geographical distance between cluster nodes becomes a non-negligible
factor in the final results.  We provide some benchmarking results from a more
friendly setup later in the article.

How to interpret the tables below:

- Latency number indicates the difference between the time a message was sent
  to the queue by the producer and the time it was received by the consumer.
  All latency numbers are in milliseconds.

- Message size was 1KB and compression was disabled.

- In the *Scenario* column, *Q*, *P* and *C* letters represent a queue, a
  producer and a consumer respectively.  For example:
  - `1Q, 1P, 1C` in the first column means one producer, one consumer and one
    queue.
  - `10Q, 10P, 10C` means 10 queues, 10 producers and 10 consumers, such that
    there is one producer and one consumer for each queue.
  - `1Q, 1P, 5C` means one queue with one producer and five consumers, and each
    consumer receiving every message posted on the queue.

- Second column, *"Total Produce Rate"* indicates the total produce rate
  accumulated across all producers running in that scenario.

- Similarly, *"Total Consume Rate"* indicates the total consume rate
  accumulated across all consumers running in that scenario.

### Priority Routing Strategy

| Scenario         | Total Produce Rate (msgs/sec) | Total Consume Rate (msgs/sec) | Median (p50) | Average | p90  | p99   |
|------------------|-------------------------------|-------------------------------|--------------|---------|------|-------|
| 1Q, 1P, 1C       | 60,000                        | 60,000                        | 4.5          | 5.4     | 7.3  | 19.8  |
| 10Q, 10P, 10C    | 120,000                       | 120,000                       | 3.9          | 12.0    | 6.8  | 201.9 |
| 50Q, 50P, 50C    | 100,000                       | 100,000                       | 3.9          | 7.3     | 8.7  | 87.9  |
| 100Q, 100P, 100C | 100,000                       | 100,000                       | 4.7          | 12.4    | 20.3 | 165.6 |


### Fan-out Routing Strategy

| Scenario   | Total Produce Rate (msgs/sec) | Total Consume Rate (msgs/sec) | Median (p50) | Average | p90 | p99 |
|------------|-------------------------------|-------------------------------|--------------|---------|-----|-----|
| 1Q, 1P, 5C | 20,000                        | 100,000                       | 4.4          | 4.7     | 4.8 | 8.2 |


### Broadcast Routing Strategy

| Scenario    | Total Produce Rate (msgs/sec) | Total Consume Rate (msgs/sec) | Median (p50) | Average | p90 | p99 |
|-------------|-------------------------------|-------------------------------|--------------|---------|-----|-----|
| 1Q, 1P, 1C  | 160,000                       | 160,000                       | 4.5          | 5.1     | 5.5 | 8.8 |
| 1Q, 1P, 5C  | 110,000                       | 550,000                       | 3.7          | 3.8     | 4.1 | 7.2 |
| 1Q, 1P, 10C | 20,000                        | 200,000                       | 2.8          | 3.0     | 3.6 | 5.4 |

---

## Benchmarking in Friendly Setup

We ran our BlazingMQ benchmarks in a friendlier setup which was different from
the above setup in these ways:

- Three nodes in the BlazingMQ cluster

- Nodes geographically spread across such that `ping` latency between them was
  less than 30 microseconds.

- Primary node waited for acknowledgement from one replica before replying to
  the producer, thereby ensuring that at least two nodes (primary and one
  replica) had recorded the message before replying to the producer.


### Priority Routing

| Scenario         | Total Produce Rate (msgs/sec) | Total Consume Rate (msgs/sec) | Median (p50) | Average | p90 | p99  |
|------------------|-------------------------------|-------------------------------|--------------|---------|-----|------|
| 1Q, 1P, 1C       | 60,000                        | 60,000                        | 1.5          | 1.7     | 2.1 | 4.9  |
| 10Q, 10P, 10C    | 120,000                       | 120,000                       | 1.1          | 2.1     | 2.7 | 35.7 |
| 50Q, 50P, 50C    | 100,000                       | 100,000                       | 0.7          | 1.5     | 1.1 | 23.1 |
| 100Q, 100P, 100C | 100,000                       | 100,000                       | 0.9          | 2.5     | 2.7 | 43.8 |


### Fan-out Routing Strategy

| Scenario   | Total Produce Rate (msgs/sec) | Total Consume Rate (msgs/sec) | Median (p50) | Average | p90 | p99  |
|------------|-------------------------------|-------------------------------|--------------|---------|-----|------|
| 1Q, 1P, 5C | 20,000                        | 100,000                       | 0.6          | 0.7     | 0.8 | 3.0  |
| 1Q, 1P, 5C | 30,000                        | 150,000                       | 1.4          | 2.4     | 5.2 | 15.1 |


### Broadcast Routing Strategy

| Scenario    | Total Produce Rate (msgs/sec) | Total Consume Rate (msgs/sec) | Median (p50) | Average | p90 | p99 |
|-------------|-------------------------------|-------------------------------|--------------|---------|-----|-----|
| 1Q, 1P, 1C  | 160,000                       | 160,000                       | 2.0          | 2.0     | 2.3 | 2.9 |
| 1Q, 1P, 5C  | 110,000                       | 550,000                       | 1.7          | 1.8     | 1.9 | 4.0 |
| 1Q, 1P, 10C | 20,000                        | 200,000                       | 0.6          | 0.6     | 0.9 | 1.2 |

---
