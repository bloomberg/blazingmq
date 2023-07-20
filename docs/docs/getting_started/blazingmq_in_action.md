---
layout: default
title: BlazingMQ in Action
parent: Getting Started
nav_order: 2
---

# BlazingMQ in Action
{: .no_toc }

* toc
{:toc}

The BlazingMQ project comes with an out-of-the-box docker cluster meant for
experimentation. We'll walk through bringing up a BlazingMQ broker, demonstrate
some tooling, and highlight a few BlazingMQ features.  This image is meant for
demo purposes only.

To begin, first clone the BlazingMQ
[project](https://github.com/bloomberg/blazingmq) and run the following
commands from repo's root directory:

```sh
$ docker compose -f docker/single-node/docker-compose.yaml up --build -d
$ docker compose -f docker/single-node/docker-compose.yaml run bmqtool
$ bmqtool -b tcp://bmqbrkr:30114
```

This command will bring up a BlazingMQ cluster with a default domain
provisioned according to a config file. It will then drop you into a shell
which can run a CLI program called `bmqtool`. Note that by default, BlazingMQ
broker listens on port number 30114.

## BlazingMQ CLI

BlazingMQ comes with a CLI tool to help test and interact with the broker
called `bmqtool`. This program has several uses, including acting as a producer
and/or consumer application. We will walk through its most basic workflow: the
client REPL. The included docker compose workflow should get you started:

```sh
$ docker compose -f docker/single-node/docker-compose.yaml up bmqtool

> help
```

### Creating your first queue

When you start up `bmqtool`, it should drop you into its default CLI mode. By
default the tool does not begin a **session** with the broker in this mode. To
open a connection with the BlazingMQ backend, you'll need to `start` a
**session**:

```sh
> start

10MAY2023_15:27:19.305 (140195584415616) INFO m_bmqtool_interactive.cpp:140 --> Starting session: [ async = false ]
...
10MAY2023_15:27:19.311 (140195584415616) INFO m_bmqtool_interactive.cpp:151 <-- session.start(5.0) => SUCCESS (0)
```

{: .highlight }
**Note:** `bmqtool` may print log lines hiding the CLI prompt `>`.
The CLI will still take commands you type into the CLI even if the `>` is not visible.

Now we may open our first **queue**!  We will open the queue both as a producer
and consumer.  This means that any messages that we post will be echoed back to
us.  Note that a queue does not need to be created or declared upfront in
BlazingMQ. It is automatically created under the hood by the system when it is
opened for the first time.

```sh
> open uri

open uri="bmq://bmq.test.persistent.priority/my-first-queue" flags="read,write,ack"
```

The `open` command takes two parameters: the **queue**'s URI and
permissions. Note that we are passing *write* and *read* flags when opening the
queue. Producing messages to **queues** is done through the `post`
operation. Try it out now:

```sh
> post uri="bmq://bmq.test.persistent.priority/my-first-queue" payload=["hello world"]
```

Since we opened the queue in the *read* mode as well, the message will be
echoed back to us. This can be checked by the `list` command:

```sh
> list

12MAY2023_16:47:45.073 (139815379564416) INFO m_bmqtool_interactive.cpp:648 Unconfirmed message listing: 1 messages
  #  1  [40000000001068A4B4B275EDF7D3228F] Queue: '[ uri = bmq://bmq.test.persistent.priority/my-first-queue correlationId = [ autoValue = 2 ] ]' = 'hello world'
```

**Messages** are composed of a payload and an optional set of properties, which
can act as metadata for the message. A property can be represented as a JSON
object like so:

```json
{
  "name": "string",
  "value": "any",
  "type": "string"
}
```

**Queues** are opened in namespaces called **domains.** If an opened **queue**
doesn't exist, then it will be created. **Domains** also help define default
configurations for **queues**. We'll look at how to configure custom
**domains** later.

Once a message is delivered to a queue, it may be read by consumers and
confirmed. In BlazingMQ, this is done with the `confirm` operation.

```sh
> confirm uri="bmq://bmq.test.persistent.priority/my-first-queue" guid="40000000001068A4B4B275EDF7D3228F"
```

Notice as part of the `confirm` operation we must pass a GUID, which matches
the one from the output of the `list` command.

And that's it!  We just ran a BlazingMQ client application which acted as a
producer and a consumer for a queue, posted a message and consumed it!

More hands-on examples of playing around with BlazingMQ can be found in
[this](../more_fun_with_blazingmq) article, where we see some intermediate and
advanced features of BlazingMQ in action.

We also encourage readers to visit these articles to learn more about
BlazingMQ:

- [*Concepts*](../../introduction/concepts), which explains some terminology
  mentioned in this article, like *domain*, *queue*, *queue uri*, *message*,
  etc.

- Our [C++](https://github.com/bloomberg/blazingmq/tree/main/src/tutorials)
  and
  [Java](https://github.com/bloomberg/bmq-sdk-java/tree/main/bmq-examples)
  tutorials.

- C++ and Java API reference [documentation](../../apidocs/index)

- BlazingMQ [architecture](../../architecture/clustering)
