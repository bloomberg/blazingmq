---
layout: home
title: Home
nav_order: 0
description: ""
permalink: /
---

## Clustering and Quorum-based Replication

Built on the [solid foundation](docs/architecture/clustering) of
industry-standard best practices in the domain of distributed systems to
provide highly-available queues.

{% include animation.html asset_path="assets/animations/animations/bmq/message-paths/index.html" height="44%" %}

---

## Message Routing Strategies

Provides a set of [message routing
strategies](docs/features/message_routing_strategies) to help applications
implement complex message processing pipelines.

### Routing Strategy - Work Queue

{% include animation.html asset_path="assets/animations/animations/bmq/work-queue-mode/index.html" height="44%" %}

### Routing Strategy - Consumer Priority

{% include animation.html asset_path="assets/animations/animations/bmq/priority-mode/index.html" height="44%" %}

---

## Multi-Hop Network Topology

Supports a unique multi-hop network
[topology](docs/architecture/network_topology) leading to a distribution tree
for each queue, thereby leading to network bandwidth savings for certain use
cases.

{% include animation.html asset_path="assets/animations/animations/bmq/distribution-tree/index.html" height="82%" %}

---
