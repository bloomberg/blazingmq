---
layout: default
title: Roadmap
parent: Introduction
nav_order: 6
---

# BlazingMQ Roadmap
{: .no_toc }

* toc
{:toc}

BlazingMQ is an actively evolving product and we have several enhancements in the areas of security, stability, scalability, user-facing
features, tooling, etc. in the pipeline. This document lists some of these short-, medium- and
long-term projects. As with any large enterprise software framework, the priority
of these projects can change. If you'd like to see a feature or enhancement in
BlazingMQ, please reach out to us!

## Short-Term

### Testing Suites
{: .no_toc }

BlazingMQ has various test suites which will be published as open source in the
coming months after general cleanup and refactoring:

- Integration test suite

- Chaos test suite built on top of [Jepsen](https://github.com/jepsen-io/jepsen)

- Stress and performance test suite

- Fuzz test suite

---

## Medium-Term

### Security: Authentication
{: .no_toc }

Authentication and encryption in transit will be added to BlazingMQ by adopting
TLS support.

### Security: Authorization
{: .no_toc }

In order to give clients better control over their namespaces and queues,
support for authorization will be added to BlazingMQ. Users will be able to
configure which identities are permissioned to access their BlazingMQ queues.

### Overload Control
{: .no_toc }

BlazingMQ brokers will be updated to detect when they are overloaded (high
memory, large volume of pending work, etc.). In turn, they will start
throttling producer applications which are producing messages at a higher rate than normal
and/or have a lot of pending (unacknowledged) messages piled up in the brokers.

### Stuck Consumer Detection
{: .no_toc }

Any consumers which have not confirmed their messages in a configurable amount
of time will be assumed to be dead and removed from the set of working
consumers. This will ensure that consumers which are deadlocked or are stuck
for some reason don't hold on to unprocessed messages forever.

### Adminstrative Tooling
{: .no_toc }

A set of command line tools and scripts which can be used to manage and interact with a BlazingMQ cluster will be published as open source. Several such tools already exist, while some other new tools and scripts will be developed.

---

## Long-Term

### Distributed Tracing Support
{: .no_toc }

BlazingMQ contains pluggable support for distributed tracing. However, this
support is partial and currently available only for a subset of requests in its
control plane. BlazingMQ will be updated to support full-fledged distributed tracing across both its control and data planes.

### Higher Storage Quota
{: .no_toc }

BlazingMQ's storage layer will be updated to ensure that it can leverage as much
of the storage/disk as available in the cluster, such that queues with
terabytes worth of quota can be supported.

### Source and Sink Connectors
{: .no_toc }

Support for various source and sink connectors will be added in BlazingMQ for
easier integration with other systems.

### Multi-level Consumer Priority Support
{: .no_toc }

BlazingMQ's message routing logic will be updated to ensure that messages can be routed to lower priority consumers when those with highest priority are at capacity. Currently, if highest priority consumers are unable to accept any more messages, new messages stay in the queue and are not routed to lower priority consumers.

---
