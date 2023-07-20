---
layout: default
title: More Fun with BlazingMQ
parent: Getting Started
nav_order: 3
---

# More Fun with BlazingMQ
{: .no_toc }

* toc
{:toc}

BlazingMQ has several features for handling message delivery. We'll go over a
few common situations, each of which can be demoed using the `docker compose`
workflow in [BlazingMQ in Action](../blazingmq_in_action).

As a reminder, here's the setup for bmqtool and a single broker node:

```sh
$ docker compose -f docker/single-node/docker-compose.yaml up --build -d
$ docker compose -f docker/single-node/docker-compose.yaml run bmqtool
$ bmqtool -b tcp://bmqbrkr:30114
```

{: .highlight }
**Note:** `bmqtool` may print log lines hiding the CLI prompt `>`.
The CLI will still take commands you type into the CLI even if the `>` is not visible.

---

## Redelivery

Open a queue as a reader & writer, post a message, and observe that the
consumer has received the message with `list`.

```sh
> start
> open uri="bmq://bmq.test.persistent.priority/redelivery" flags="read,write"
> post uri="bmq://bmq.test.persistent.priority/redelivery" payload=["hello world"]
> list
  #  1  [400000000002FAC11F098B1B47064911] Queue: '[ uri = bmq://bmq.test.persistent.priority/redelivery correlationId = [ autoValue = 2 ] ]' = 'hello world'
```

Then shutdown the consumer without confirming the message. Start it again, and
show that message was redelivered to it.

```sh
> bye
$ bmqtool -b tcp://bmqbrkr:30114
> start
> open uri="bmq://bmq.test.persistent.priority/redelivery" flags="read"
> list
  #  1  [400000000002FAC11F098B1B47064911] Queue: '[ uri = bmq://bmq.test.persistent.priority/redelivery correlationId = [ autoValue = 2 ] ]' = 'hello world'
```

---

## Round Robin

Start two instances of `bmqtool`.

```sh
# bmqtool 1
> start
> open uri="bmq://bmq.test.persistent.priority/round-robin" flags="read,write"
```

```sh
# bmqtool 2
> start
> open uri="bmq://bmq.test.persistent.priority/round-robin" flags="read"
```

In the first one, post the following:

```sh
# bmqtool 1
> post uri="bmq://bmq.test.persistent.priority/round-robin" payload=["alice", "bob", "charlie"]
```

Now run `list` in each `bmqtool` instance. You should see each message was
distributed in a round-robin fashion:

```sh
# bmqtool 1
> list
  #  1  [40000000027B90A85281EBBE64922B68] Queue: '[ uri = bmq://bmq.test.persistent.priority/round-robin correlationId = [ autoValue = 2 ] ]' = 'alice'
  #  2  [40000200027B90A8DE20EBBE64922B68] Queue: '[ uri = bmq://bmq.test.persistent.priority/round-robin correlationId = [ autoValue = 2 ] ]' = 'charlie'
```

```sh
# bmqtool 2
> list
  #  1  [40000100027B90A8C96FEBBE64922B68] Queue: '[ uri = bmq://bmq.test.persistent.priority/round-robin correlationId = [ autoValue = 2 ] ]' = 'bob'
```

---

## Rebalancing

If a consumer shuts down with unconfirmed messages, those messages will be
redelivered to other active consumers. Start multiple consumers, and post a
message:

```sh
# bmqtool 1
> start
> open uri="bmq://bmq.test.persistent.priority/rebalance" flags="read,write"
> post uri="bmq://bmq.test.persistent.priority/rebalance" payload=["alice"]
```

```sh
# bmqtool 2
> start
> open uri="bmq://bmq.test.persistent.priority/rebalance" flags="read"
```

`list` both consumers to show that one received the message and the other
didn't:

```
# bmqtool 1
Unconfirmed message listing: 1 messages
  #  1  [40000000007A7B1DCAB1E7ED0CDB2FD1] Queue: '[ uri = bmq://bmq.test.persistent.priority/rebalance correlationId = [ autoValue = 8 ] ]' = 'alice'
```

```
# bmqtool 2
Unconfirmed message listing: 0 messages
```

Now kill the consumer that received the message and `list` in the other to
confirm that the message was resent:

```sh
# bmqtool 1
> bye
```

```sh
# bmqtool 2
> list
Unconfirmed message listing: 1 messages
  #  1  [40000000007A7B1DCAB1E7ED0CDB2FD1] Queue: '[ uri = bmq://bmq.test.persistent.priority/rebalance correlationId = [ autoValue = 19 ] ]' = 'alice'
```

---

## Consumer Priority

- Show [consumer
  priorities](../../features/message_routing_strategies#consumer-priority-mode)
  in action. Bring up two consumers, one with higher and another with lower
  priority. See that all messages are routed to the higher priority consumer.

Consumers can be configured with priority. A higher priority will cause
messages to be delivered with preference. The default priority is 0. Let's open
two consumers, one with the default and one with a higher priority:

```sh
# bmqtool 1
> start
> open uri="bmq://bmq.test.persistent.priority/priority" flags="read" consumerPriority=0
```

```sh
# bmqtool 2
> start
> open uri="bmq://bmq.test.persistent.priority/priority" flags="read,write" consumerPriority=1
> post uri="bmq://bmq.test.persistent.priority/priority" payload=["alice", "bob", "charlie"]
```

Now run `list` in each to show that the higher priority consumer received all
three messages:

```sh
# bmqtool 1
> list
Unconfirmed message listing: 0 messages
```

```sh
# bmqtool 2
> list
Unconfirmed message listing: 3 messages
  #  1  [400000000005289E9887B8D6F559F164] Queue: '[ uri = bmq://bmq.test.persistent.priority/priority correlationId = [ autoValue = 2 ] ]' = 'alice'
  #  2  [400001000005289EF052B8D6F559F164] Queue: '[ uri = bmq://bmq.test.persistent.priority/priority correlationId = [ autoValue = 2 ] ]' = 'bob'
  #  3  [400002000005289EFD2BB8D6F559F164] Queue: '[ uri = bmq://bmq.test.persistent.priority/priority correlationId = [ autoValue = 2 ] ]' = 'charlie'
```

---

## Subscriptions

- Show [subscriptions](../../features/subscriptions) in action. Bring up two
  consumers, both specifying a different subscription expression, see that
  first message, which satisfies first expression, is routed to consumer 1.
  Similary, second message satisfying second expression is routed to consumer
  2.

Consumers can open queues that receive messages conditionally based on message
properties.

Bring up a consumer and open a queue with a subscription:

```sh
> start
> open uri="bmq://bmq.test.persistent.priority/subscription" flags="read,write" subscriptions=[{"expression": "x > 0"}]
```

Now `post` a message that fails to match `x > 0` in the message properties and observe the consumer fails to receive it:

```sh
> post uri="bmq://bmq.test.persistent.priority/subscription" payload=["alice"] messageProperties=[{"name": "x", "value": "0", "type": "E_INT"}]
> list
Unconfirmed message listing: 0 messages
```

Now send a message with `x > 0`:

```sh
> post uri="bmq://bmq.test.persistent.priority/subscription" payload=["alice"] messageProperties=[{"name": "x", "value": "1", "type": "E_INT"}]
> list
  #  1  [40000400011AFF060B5DC6A7FA905D3D] Queue: '[ uri = bmq://bmq.test.persistent.priority/subscription correlationId = [ autoValue = 24 ] ]' = 'alice', with properties: [ x (INT32) = 1 ]
```

---
