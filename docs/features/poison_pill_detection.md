---
layout: default
title: Poison Pill Detection
parent: Features
nav_order: 4
---

# Poison Pill Detection
{: .no_toc }

* toc
{:toc}

## Introduction

BlazingMQ has a built-in bad message ("poison pill") detection mechanism which
auto detects and purges poison pill messages from the queue.  This helps
applications prevent a widespread outage in the scenario where a poison pill
message is causing consumer applications to crash repeatedly.

## Motivation

Let's understand the motivation in more detail.  One can attach multiple
consumers to a BlazingMQ queue.  Additionally, any time a consumer goes away
without confirming messages that were sent to it, BlazingMQ re-routes those
messages to other consumer(s), if available.  If a message is acting as a
poison pill and crashing consumers, BlazingMQ will continue to re-route to that
message to any available consumer, until that message is confirmed by a
consumer, or is removed from the queue as a result of message TTL expiration or
is manually purged from the queue.  Typically, messages are configured with
higher values of TTL (10 minutes or more), and purging a specific message or
entire queue requires manual intervention.  It is easy to see how a poison pill
message can lead to widespread application outage by repeatedly bringing down
consumers and keeping them down for an extended period of time.

## Solution

BlazingMQ limits retransmissions of poison pill messages up to the limit
specified in the BlazingMQ domain configuration (typical recommended value is
five).  Once the retransmission counter of a message reaches zero, the message
is auto-purged from the queue.  Additionally, the payload of the offending
message is dumped in a file which can be inspected for troubleshooting
purposes.


## Caveats

Note that this feature can introduce some additional latency in case BlazingMQ
detects a message as *potentially* poisonous.  BlazingMQ users might be aware
that BlazingMQ delivers messages to consumer applications in batches for
performance reasons.  If one message in a batch is poisonous and causes a
consumer to crash, BlazingMQ declares the entire batch as *potentially*
poisonous, and a throttled delivery algorithm kicks in, which attempts to
pinpoint the message which is actually poisonous.  As the message continues to
crash consumers and its transmission counter inches towards zero, the algorithm
throttles the batch of messages aggressively to find the culprit.  This
throttling can introduce a delay of up to 5 seconds (configurable) in message
transmission to consumers.  Note that this feature does not introduce *any*
throttling or overhead in the absence of consumer crashes.

---
