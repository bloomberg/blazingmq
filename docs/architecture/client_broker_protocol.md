---
layout: default
title: Client/Broker Protocol
parent: Architecture
nav_order: 8
---

# BlazingMQ Client/Broker Protocol
{: .no_toc }

* toc
{:toc}

## Introduction

This article documents the protocol between a BlazingMQ client and broker, and
also goes over some best practices for designing and implementing BlazingMQ
client library.  Readers interested in implementing a BlazingMQ client library
may find this article useful.

Before we dive in, it is worth noting that:

1. BlazingMQ has native client libraries in C++ and Java.  The Python library
   is a wrapper over C++ library.  BlazingMQ does not have a C library.

2. In order to provide a good user experience, shield applications from various
   transient issues and support high performance, BlazingMQ C++ and Java
   libraries are quite stateful, highly asynchronous and carefully designed.
   If the goal of author of BlazingMQ client library is to reach as many
   audience as possible, they should attempt to do the same with their library
   design and implementation.

3. Ideally, the author should be willing to help us maintain the client library
   in the long run.  As a provider of an infrastructure framework, we want to
   provide good support to our users, and having the author's help in handling
   any bug fixes and enhancements in the library will make our job easy!

4. We have had queries about client libraries in Go, .NET and Rust, so
   targeting one of those languages may provide maximum benefits to our users.


The rest of this document is divided into three main parts:

- *BlazingMQ wire protocol*, which goes over format of messages exchanged
  between BlazingMQ client and broker.

- *BlazingMQ client/server interaction*, which goes over the order in which
  these messages are exchanged.

- *Client library design guide*, which goes over some recommendations for
  implementing a BlazingMQ client library in a language-agnostic way.

Let's dive into some details now!

---

## BlazingMQ Wire Protocol

BlazingMQ wire protocol sits at the lowest level in the BlazingMQ client/server
interaction.  BlazingMQ has a custom protocol over TCP.  This protocol defines
the format of messages exchanged between BlazingMQ client and broker.  There
were four guiding principles when this protocol was designed:

- *Compactness*: The protocol should have minimal overhead.

- *Forward and backward compatibility*: The protocol should be easily
  extensible while remaining backward compatible.

- *Batching*: The protocol should support batching of messages wherever
  possible.

- *Efficiency*: The protocol should avoid encoding/decoding overhead for
  frequent messages, while still taking into consideration concerns like
  endianness, etc.

### BlazingMQ Network Packet

Before we get into various types of messages in BlazingMQ wire protocol, let's
understand the layout of a BlazingMQ network packet.

Every BlazingMQ network packet begins with an 8-byte header called the
`EventHeader`
([C++](https://github.com/bloomberg/blazingmq/blob/ca6491f69eea8d91733fa36ef3e82c4facc734fc/src/groups/bmq/bmqp/bmqp_protocol.h#L727),
[Java](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/EventHeader.java)).

The meaning of every field in the header is documented in the files linked
above.  Some important things to know about:

- In BlazingMQ, a full BlazingMQ network packet is often referred to as an
  `Event`.

- The fragment bit `F` is currently unused and always set to zero.

- Remaining 31 bits capture the entire length of the packet, including the
  header, sub-headers, any options, padding, etc.

- The `Type` field captures
  [type](https://github.com/bloomberg/blazingmq/blob/ca6491f69eea8d91733fa36ef3e82c4facc734fc/src/groups/bmq/bmqp/bmqp_protocol.h#L457)
  of the packet.  Note that a BlazingMQ packet is homogeneous.  In other words,
  a packet can contain more than one BlazingMQ message, but all of those
  messages will be of the same type.  For example, if the type of the packet is
  `PUT`, all messages which appear in the packet will be `PUT` messages.

- The `HeaderWords` field captures the size of the `EventHeader` itself.  This
  helps with backward and forward compatibility as the wire protocol evolves.
  Here's how: suppose 4 more bytes are added to `EventHeader` as part of a new
  feature implementation, and a new BlazingMQ broker is rolled out and starts
  sending BlazingMQ network packets with the additional 4 bytes in the
  `EventHeader`.  If BlazingMQ brokers derived the size of `EventHeader` using,
  `sizeof(EventHeader)` in C++, old BlazingMQ brokers will continue to assume
  that the size of `EventHeader` is still 8 bytes, and will interpret the new 4
  bytes as the next part of the packet (e.g., a sub-header, payload, etc).
  However, instead of using `sizeof` operators, if BlazingMQ broker code reads
  size of the header from the `HeaderWords` field of the received packet, old
  brokers will know that `EventHeader` is 12 bytes instead of 8.  Obviously,
  old brokers will not understand how to interpret those 4 new bytes, but they
  will skip it, which is good enough.  Almost every header struct in BlazingMQ
  wire protocol contains the `HeaderWords` field.

- Every `EventHeader` is followed by the event-specific details.  For example,
  if it is a packet of type `PUT`, `EventHeader` will be followed by one or
  more pairs of `PutHeader` and message payload (more on this later).

### Types of Messages

Now that we understand top level structure of a BlazingMQ network packet,
let's dive into the next level of detail.  At a high level, there are two types
of messages in the protocol:

- **Binary messages**: This type of messages are not encoded/decoded.  They are
  simple 'structs' (sequence of raw bytes) directly written/read to/from the
  wire.  This format is used for messages which are exchanged frequently
  between client and broker (typically data messages or 'data plane').  Some
  examples of such messages are `PutMessage`, `ConfirmMessage`, etc.  The goal
  is to avoid paying encoding/decoding overhead for frequent messages.

- **Schema messages**: This type of messages are defined in a schema and are
  encoded/decoded using coding schemes like JSON, BER, XML, etc.  This format
  is used for messages which are infrequent (typically control messages or
  'control plane').  Some examples of such messages are `OpenQueueRequest`,
  `CloseQueueRequest`, `DisconnectRequest`, etc.  Since such messages are
  infrequent, it is okay to pay the overhead of encoding/decoding them in lieu
  of flexibility and ease of use.

Let's look into both of these message types in detail.

### Binary Messages

Binary messages in BlazingMQ are defined
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_protocol.h)
for C++, and
[here](https://github.com/bloomberg/blazingmq-sdk-java/tree/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto)
for Java.

> [!NOTE]
> The C++ header file `bmqp_protocol.h` linked above also contains some
> binary messages which are exchanged only between BlazingMQ brokers.  This can
> cause confusion when trying to look at the list of messages which are
> exchanged only between client and broker.  A better place to look at the
> messages exchanged between client and broker would be in the Java link above.

Here's a list of binary messages which are exchanged between BlazingMQ client
and broker, along with their purpose:

- **PutMessage**: A message posted by the producer to the BlazingMQ queue. This
  message contains the payload and any associated metadata that producer wants
  to be delivered to the consumer.

- **AckMessage**: An acknowledgement sent by BlazingMQ queue to the producer in
  response to the PUT message.  The *AckMessage* carries the success/failure
  status, telling the producer if BlazingMQ accepted the *PutMessage* or not.

- **PushMessage**: A message sent by the BlazingMQ queue to a consumer
  application.  This message is same as a *PutMessage* except for a few
  details.

- **ConfirmMessage**: A message sent by the consumer to the BlazingMQ queue
  indicating that the *PushMessage* was successfully processed by the consumer
  and that the BlazingMQ queue can mark the message as consumed and delete it
  at any time.

See [this](#binary-messages-wire-layout) section below which goes over more
details about how binary messages are packed for their wire representation.

### Schema Messages

Schema messages in BlazingMQ are defined
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd)
for C++, and [here](https://github.com/bloomberg/blazingmq-sdk-java/tree/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg) for Java.

> [!NOTE]
> Similar to the note in binary messages, the schema `bmqp_ctrlmsg.xsd` linked
> above also contains some schema messages which are exchanged only between
> BlazingMQ brokers.  This can cause confusion when trying to look at the list
> of schema messages which are exchanged only between client and broker.  A
> better place to look at the  messages exchanged between client and broker
> would be in the Java link above.

The recommendation is to use the JSON codec for schema messages.  Other encodings
like BER, XML, etc are also supported but not recommended.  The C++ SDK uses the
[`baljsn`](https://github.com/bloomberg/bde/tree/main/groups/bal/baljsn) codec
for JSON encoding, and the Java SDK uses [`gson`](https://github.com/google/gson)
codec.

Here's a list of schema messages which are exchanged between BlazingMQ client
and broker, along with their purpose:

- **NegotiationMessage**: The first message sent by the client to the broker
  upon establishing a TCP connection with the broker (see
  [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/NegotiationMessageChoice.java)).
  It's a choice, and the client always sets the choice to
  [`ClientIdentity`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/ClientIdentity.java),
  and populates various fields in `ClientIdentity` accordingly.  Broker
  responds with a `NegotiationMessage` as well, but sets
  [`BrokerResponse`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/BrokerResponse.java)
  instead of `ClientIdentity`.  A `NegotiationMessage` from the client can be
  thought of as an application level handshake with the broker to establish a
  BlazingMQ session.  The client advertises some of its capabilities, version, and
  some other details in the `NegotiationMessage`.  Based on this information,
  the broker can choose to accept or reject the connection with the client
  (e.g., if the client is using a very old version of the SDK, the broker may
  refuse to connect with the client).

- **OpenQueue**: The request that the client sends to the broker to attach to a
  queue (see
  [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/OpenQueue.java)).
  `OpenQueue` contains a
  [`QueueHandleParameters`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/QueueHandleParameters.java)
  field.  The client populates each field of `QueueHandleParameters` appropriately.
  An `OpenQueue` request can be logically thought as a request by a BlazingMQ
  application to get a handle on the queue or connecting to a queue.  Note that
  an `OpenQueue` request is always followed by a
  [`ConfigureQueue`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/ConfigureQueueStream.java)
  request.

- **OpenQueueResponse**: The response sent by the broker for an `OpenQueue`
  request (see [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/OpenQueueResponse.java)).

- **ConfigureQueueStream**: The request sent by the client to the broker to
  configure the client's configuration for a queue which was opened earlier by
  the client by making an `OpenQueue` request (see
  [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/ConfigureQueueStream.java)).
  The main field in this request is
  [`QueueStreamParameters`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/ConfigureQueueStream.java).
  A producer or consumer application can start using a queue only after sending
  an `OpenQueue` request and then a `ConfigureQueueStream` request.  A
  `ConfigureQueueStream` request is used by the producer/consumer to set its
  priority or flow-control parameters for the queue.  A producer/consumer can
  also send a stand-alone `ConfigureQueueStream` request at any time to do the
  same.  However, it must send one upon receiving a successful
  `OpenQueueResponse` from the broker.

- **ConfigureQueueStreamResponse**: The message sent by the broker in response
  to the `ConfigureQueueStream` request from the client (see
  [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/ConfigureQueueStreamResponse.java)).

- **CloseQueue**: The request sent by the client to the broker to indicate that
  client no longer wants to be attached to the queue (see
  [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/CloseQueue.java)).
  A `CloseQueue` request is the opposite of an `OpenQueue` request.  Note that
  a `CloseQueue` request is always preceded by an `ConfigureQueueStream`
  request.

- **Disconnect**: The request sent by the client to tear down the BlazingMQ
  session with the broker (see
  [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/Disconnect.java)).

- **DisconnectResponse**: The message sent by the broker to the client in
  response to a `Disconnect` request (see
  [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/msg/DisconnectResponse.java)).  Once `DisconnectResponse` is
  sent by the broker, no other message is sent by it to the client.

See [this](#schema-messages-wire-layout) section below which goes over more
details about how schema messages are packed for their wire representation.

---

## Binary Messages Wire Layout

This section explains how binary messages are typically packed for their wire
representation.

As mentioned earlier in the article, every BlazingMQ network packet starts with
an
`EventHeader`([C++](https://github.com/bloomberg/blazingmq/blob/ca6491f69eea8d91733fa36ef3e82c4facc734fc/src/groups/bmq/bmqp/bmqp_protocol.h#L727),
[Java](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/EventHeader.java)).

### Endianness

When serializing binary messages, one needs to be careful about endianness.
Readers may already be aware that network byte order is big-endian.  As of
writing this, BlazingMQ runs on both little-endian (x86) as well as big-endian
(SPARC, PowerPC) architectures.  In order to minimize errors when
(de)serializing BlazingMQ network packets, BlazingMQ uses some helper classes.

In Java, there are two wrapper classes in BlazingMQ --
[`ByteBufferInputStream`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/io/ByteBufferInputStream.java)
and
[`ByteBufferOutputStream`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/io/ByteBufferOutputStream.java)
which under the hood use
[`java.nio.ByteBuffer`](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/nio/ByteBuffer.html),
which in turn takes care of flipping the bytes when reading/writing integers,
doubles, etc.

In C++, BlazingMQ uses various
[`bdlb::BigEndian`](https://github.com/bloomberg/bde/blob/main/groups/bdl/bdlb/bdlb_bigendian.h)
classes from one of our helper libraries.  These classes take care of swapping
the bytes seamlessly. As an example,
[here](https://github.com/bloomberg/blazingmq/blob/69f0f8f5b188ca1eb07b9d262093f949729db538/src/groups/bmq/bmqp/bmqp_protocol.h#L779)
we use the
[`bdlb::BigEndianUint32`](https://github.com/bloomberg/bde/blob/cf44cbb2cc179077f687f5408d92d6949fae75af/groups/bdl/bdlb/bdlb_bigendian.h#L517)
type to represent an `uint32_t` to capture the `fragment` and `length`
fields of an `EventHeader`.

Let's go over the wire layout of every binary message exchanged between client
and the broker.

### PUT Event

A PUT event is a BlazingMQ network packet which is sent by the producer
application to the BlazingMQ backend, with one or more messages destined for
one or more BlazingMQ queues.

A PUT event wire layout looks like this:

```
[EventHeader][PutHeader #1][Option #1][Option #2]...[Option #N][Message Properties][Message Payload][PutHeader #2][Option #1][Option #2]...[Option #N][Message Properties][Message Payload]...
```

To describe above layout in words:

- `EventHeader` represents the top-level header which captures some details of
  the entire PUT event (event type, total length, protocol version, etc).

- `PutHeader #1` represents the header of the first PUT message in the event.
  A
  [`PutHeader`](https://github.com/bloomberg/blazingmq/blob/e3ddd4fdc8e024e3abff96aa91555f042ce4e565/src/groups/bmq/bmqp/bmqp_protocol.h#L1337)
  contains fields like:

  - `MessageWords`: Length of the entire PUT message (including options,
    properties, payload and padding) in words (1 word == 4 bytes).

  - `OptionsWords`: Length of all options, if any, in words.

  - `CAT`: Compression algorithm type (see
    [this](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqt/bmqt_compressionalgorithmtype.h)).

  - `HeaderWords`: Length of the `PutHeader` itself, in words.

  - `QueueId`: unique integer identifier of the queue that was previously
    opened by the client.  The `QueueId` integer field is sent by the client in
    the `OpenQueue` request to the broker, and then later on, used to identify
    that queue in any message or request.

  - `MessageGuid`: Globally unique identifier for a PUT message generated by
    the client library.  See
    [this](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqt/bmqt_messageguid.h)
    and
    [this](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_messageguidgenerator.h)
    for the layout of the GUID and how it is generated by the C++ SDK.

  - `CRC32C`: checksum of the PUT message (checksum of message properties and
    payload, see
    [this](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_crc32c.h)).

  - `SchemaId`: unique identifier for the sequence of message properties, for
    subscriptions.

- `Option #N` represents one of the many options.  **NOTE**: As of now, PUT
  messages do not contain any options, but this may change in the future.

- `Message Properties` represent a list of key/value pairs that a producer can
  optionally associate with a PUT message.  Each message properties area itself
  begins with a
  [`MessagePropertiesHeader`](https://github.com/bloomberg/blazingmq/blob/e3ddd4fdc8e024e3abff96aa91555f042ce4e565/src/groups/bmq/bmqp/bmqp_protocol.h#L1075),
  followed by *N* [`MessagePropertyHeader`](https://github.com/bloomberg/blazingmq/blob/e3ddd4fdc8e024e3abff96aa91555f042ce4e565/src/groups/bmq/bmqp/bmqp_protocol.h#L1229)s, where
  *N* is the number of key/value pairs in the PUT message, followed by the list
  of key/value pairs.  In other words:

  ```
  [MessagePropertiesHeader][MessagePropertyHeader #1][MessagePropertyHeader #2]...
  [MessagePropertyHeader #N][PropertyName #1][PropertyValue #1][PropertyName #2]
  [PropertyValue #2]...[PropertyName #n][PropertyValue #N][Word Alignment padding]
  ```

  BlazingMQ broker can peek into the message properties area if needed.  For
  example, broker can read one or more message properties to evaluate a
  subscription expression.

- `Message Payload` is the payload of the message.  BlazingMQ does not
  peek into the message payload.  Producer application can indicate to the SDK
  to compress a PUT message, in which case, SDK will compress only the message
  payload (any header, options and message properties are not compressed).

In both C++ and Java SDKs, there are builder and iterator classes for PUT
event, which encapsulate the logic of building and parsing PUT event
respectively.  PUT event builder and iterator in C++ can be found
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_puteventbuilder.h)
and
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_putmessageiterator.h).
PUT event builder and iterator in Java can be found
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/PutEventBuilder.java)
and
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/PutMessageIterator.java).

### ACK Event

An ACK event is a BlazingMQ network packet which is sent by the broker to the
producer application, with one or more ACK messages indicating success/failure
result for one or more PUT messages previously sent by the producer.

An ACK event wire layout looks like this:

```
[EventHeader][AckHeader][AckMessage #1][AckMessage #2]...[AckMessage #N]
```

An ACK event contains one
[`AckHeader`](https://github.com/bloomberg/blazingmq/blob/e3ddd4fdc8e024e3abff96aa91555f042ce4e565/src/groups/bmq/bmqp/bmqp_protocol.h#L1725C19-L1725C19),
followed by one or more
[`AckMessage`](https://github.com/bloomberg/blazingmq/blob/e3ddd4fdc8e024e3abff96aa91555f042ce4e565/src/groups/bmq/bmqp/bmqp_protocol.h#L1849)s.

Note that unlike a PUT event, there is only one `AckHeader` in an ACK event,
irrespective of the number of `AckMessage`s.

Let's go over various fields in a `AckHeader`:

- `HeaderWords`: Size (in words) of the `AckHeader`.

- `PerMessageWords`: Size (in words) of each `AckMessage`.

Now let's go over various fields in a `AckMessage`:

- `Status`: result of the PUT message (was it accepted or rejected by
  BlazingMQ, with specific error code in case of failure).

- `CorrelationId`: **TODO**

- `MessageGUID`: the 16 byte globally unique identifier of a message.  See
    [this](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqt/bmqt_messageguid.h)
    and
    [this](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_messageguidgenerator.h)
    for the layout of the GUID and how it is generated by the C++ SDK.

- `QueueId`: the integer which uniquely identifies the queue.  This integer
  needs to be unique only between a BlazingMQ client application and the
  BlazingMQ backend.  It does not need to be globally unique.  The integer
  value is chosen by the SDK and sent in the `OpenQueue` request.

In both C++ and Java SDKs, there are builder and iterator classes for ACK
event, which encapsulate the logic of building and parsing ACK event
respectively.  ACK event builder and iterator in C++ can be found
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_ackeventbuilder.h)
and
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_ackmessageiterator.h).
PUT event builder and iterator in Java can be found
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/AckEventBuilder.java)
and
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/AckMessageIterator.java).

### PUSH Event

A PUSH event is a BlazingMQ network packet which is sent by the BlazingMQ
broker to the consumer application, with one or more messages belonging to one
or more BlazingMQ queues.

A PUSH event wire layout is very similar to the PUT event's layout, and looks
like this:

```
[EventHeader][PushHeader #1][Option #1][Option #2]...[Option #N][Message Properties][Message Payload][PushHeader #2][Option #1][Option #2]...[Option #N][Message Properties][Message Payload]...
```

Just like PUT event, one PUSH event can contain more than one PUSH message.
Each PUSH message starts with a
[`PushHeader`](https://github.com/bloomberg/blazingmq/blob/e3ddd4fdc8e024e3abff96aa91555f042ce4e565/src/groups/bmq/bmqp/bmqp_protocol.h#L1962), which itself closely resembles a
`PutEvent`, except that there is no `CRC32C` field in `PushHeader`.  All the
other fields are same or similar to the ones in `PutHeader`.

**NOTE**: As mentioned earlier, even though every PUT message in a PUT event
supports zero or more options, currently no options are added in a PUT message.
That is not the case for PUSH messages.  A PUSH message can have one option --
the
[`SubQueueInfos`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/SubQueueInfosOption.java).
If present, this option contains one or more
[`SubQueueInfo`](https://github.com/bloomberg/blazingmq/blob/e3ddd4fdc8e024e3abff96aa91555f042ce4e565/src/groups/bmq/bmqp/bmqp_protocol.h#L264)
structs.  A `SubQueueInfo` captures details about a sub-stream ("*AppId*") of
the queue.  **TODO**: add more details about `SubQueueInfo` option here
(fan-out, rda counter, etc).

In both C++ and Java SDKs, there are builder and iterator classes for PUSH
event, which encapsulate the logic of building and parsing PUSH event
respectively.  PUSH event builder and iterator in C++ can be found
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_pusheventbuilder.h)
and
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_pushmessageiterator.h).
PUSH event builder and iterator in Java can be found
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/PushEventBuilder.java)
and
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/PushMessageIterator.java).


### CONFIRM Event

A CONFIRM event is a BlazingMQ network packet which is sent by the consumer
application to the BlazingMQ backend, with one or more CONFIRM messages
indicating that the consumer has processed the corresponding one or more PUSH
messages previously sent by the BlazingMQ broker.

A CONFIRM event wire layout looks like this:

```
[EventHeader][ConfirmHeader][ConfirmMessage #1][ConfirmMessage #2]...[ConfirmMessage #N]
```

A CONFIRM event contains one
[`ConfirmHeader`](https://github.com/bloomberg/blazingmq/blob/e3ddd4fdc8e024e3abff96aa91555f042ce4e565/src/groups/bmq/bmqp/bmqp_protocol.h#L2296)
followed by one or more
[`ConfirmMessage`](https://github.com/bloomberg/blazingmq/blob/e3ddd4fdc8e024e3abff96aa91555f042ce4e565/src/groups/bmq/bmqp/bmqp_protocol.h#L2387)s.

Note that unlike PUT and PUSH events, there is only one `ConfirmHeader` in a
CONFIRM event, irrespective of the number of `ConfirmMessage`s.

Let's go over various fields in a `ConfirmHeader`:

- `HeaderWords`: Size (in words) of the `ConfirmHeader`.

- `PerMessageWords`: Size (in words) of each `ConfirmMessage`.

Now let's go over various fields in a `ConfirmMessage`:

- `QueueId`: the integer which uniquely identifies the queue.  This integer
  needs to be unique only between a BlazingMQ client application and the
  BlazingMQ backend.  It does not need to be globally unique.  The integer
  value is chosen by the SDK and sent in the `OpenQueue` request.

- `MessageGUID`: the 16 byte globally unique identifier of a message.  See
    [this](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqt/bmqt_messageguid.h)
    and
    [this](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_messageguidgenerator.h)
    for the layout of the GUID and how it is generated by the C++ SDK.

- `SubQueueId`: the integer which uniquely identifies the sub-stream in the
  queue.  This integer needs to be unique only between a BlazingMQ client
  application and the BlazingMQ backend within the scope of a queue.  It does
  not need to be globally unique and does not need to be unique across various
  queues opened by the same client.  The integer value is chosen by the SDK and
  sent in the `ConfigureStream` request.

In both C++ and Java SDKs, there are builder and iterator classes for ACK
event, which encapsulate the logic of building and parsing ACK event
respectively.  ACK event builder and iterator in C++ can be found
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_ackeventbuilder.h)
and
[here](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_ackmessageiterator.h).
PUT event builder and iterator in Java can be found
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/AckEventBuilder.java)
and
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/AckMessageIterator.java).

---

## Schema Messages Wire Layout

While the wire layout of binary messages can be non-trivial to implement,
schema messages are (de)serialized fairly easily.  Since every schema message's
wire layout is similar, we will describe the layout in general terms instead of
going over every schema message.  Wire layout of every schema message looks
like this:

```
[EventHeader][Encoded Schema Message]
```

Note that schema messages are not batched.  One BlazingMQ network event will
contain only one encoded schema message.

JSON is the recommended encoding to use.  The Java SDK uses
[`gson`](https://github.com/google/gson) library for JSON codecs, and wraps the
encoding/decoding logic in two simple
[components](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/codec).
On the outgoing path, the Java SDK also uses
[`SchemaEventBuilder`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/TcpBrokerConnection.java)
to prepend [`EventHeader`] to the outgoing message.  On the incoming side, the
SDK uses `JsonDecoderUtil` as shown
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/ControlEventImpl.java#L54).

---

## BlazingMQ Client Server Interaction

Now that we have an understanding of various types of messages (and their wire
layouts) exchanged between BlazingMQ client and broker, it's time to understand
the sequence in which these messages are exchanged.  Note that general API and
design guidelines for the SDK are discussed in a
[later](#client-library-design-guide) section.

Before we go into details, it is worth pointing out these two files from the
BlazingMQ Java repo --
[`PlainProducerIT.java`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/test/java/com/bloomberg/bmq/it/PlainProducerIT.java)
and
[`PlainConsumerIT.java`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/test/java/com/bloomberg/bmq/it/PlainConsumerIT.java).
These two files implement a rudimentary and "raw" BlazingMQ producer and
consumer respectively.  By "raw", we mean that this producer and consumer do
not use the Java SDK APIs to talk to the BlazingMQ backend.  Instead, they just
create a TCP socket and build, write and read schema and binary messages
themselves.  These files can be useful to get a basic understanding of the flow
and order of messages between a BlazingMQ client and broker.  We will refer to
these files when describing the message exchange in this section.

### Starting a Session with BlazingMQ Broker

There are two steps involved in establishing a session with BlazingMQ broker --
connecting with the broker over TCP, and afterwards, carrying out a BlazingMQ
protocol specific handshake (aka negotiation).  Let's look into these steps in
detail.

#### TCP Connection

The first step in a BlazingMQ client/broker interaction is the client initiating
a TCP connection with the broker.

Before we go into the mechanics of that, it's worth briefly discussing how a
BlazingMQ client can figure out the right TCP endpoint (IP + Port) of the
BlazingMQ broker to connect to.  In an enterprise setting, the correct endpoint
would typically be retrieved via some [service
discovery](https://dzone.com/articles/microservices-architectures-what-is-service-discov)
mechanism, where a BlazingMQ client might ask the service discovery APIs to
resolve a BlazingMQ domain to a BlazingMQ cluster endpoint, and pass that
endpoint to BlazingMQ's
[`SessionOptions`](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/SessionOptions.java#L418C37-L418C37).
Another way could be a using a hard-coded DNS endpoint which points to the
BlazingMQ cluster, and pass that endpoint to the `SessionOptions`.  And yet
another approach could be to deploy BlazingMQ in the [*Alternative
Topology*](https://bloomberg.github.io/blazingmq/docs/architecture/clustering/#alternative-deployment)
and have BlazingMQ client applications connect to the local BlazingMQ proxy
which could listen on a fixed port number.

Coming back to the TCP connection, SDK initiates the connection by calling some
flavor of `connect` API in the underlying language or library being used.  As
an example, BlazingMQ Java SDK uses [`netty`](https://netty.io) for its TCP
connection management and initiates a connection as shown
[here](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/net/NettyTcpConnection.java#L370).
On the other hand, the `PlainProducerIT.java` file mentioned above simply uses
the basic
[`connect`](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/test/java/com/bloomberg/bmq/it/PlainProducerIT.java#L80) API
provided by the Java language.

An important thing to note here is that both the BlazingMQ C++ and Java SDKs
use [non-blocking
I/O](https://beej.us/guide/bgnet/html//#slightly-advanced-techniques), which
can help with asynchronous and flexible design in the SDK, but at the added
cost of more complexity.  In our experience, the complexity is worth it, and an
SDK implementation in any language should consider using non-blocking I/O if
possible.

#### Negotiation

Once the `connect` operation succeeds, the BlazingMQ SDK will have a TCP
connection established with the broker.  The next step is for the SDK to carry
out a handshake with the broker.  This is known as *negotiation* in BlazingMQ.
Immediately after establishing a TCP connection, the SDK sends a
[`NegotiationMessage`](https://github.com/bloomberg/blazingmq/blob/69f0f8f5b188ca1eb07b9d262093f949729db538/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L1592)
to the broker.  This is a schema message, and hence needs to be encoded.  See
the [previous](#schema-messages-wire-layout) section for more details on the
wire layout.  Note that `NegotationMessage` is one of the top level schema
messages, the other being
[`ControlMessage`](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L84).
There is no way to discriminate between these two top level schema messages.
It is implicit that the `NegotiationMessage` will only be used during the
handshake phase, and every other schema message exchanged after that will be
the top level `ControlMessage`.

A `NegotiationMessage` can be one of three possible sub-types.  The SDK always
sends the
[`ClientIdentity`](https://github.com/bloomberg/blazingmq/blob/69f0f8f5b188ca1eb07b9d262093f949729db538/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L1663)
sub-type, which contains various fields as described in its documentation in
the link.
[Here's](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/TcpBrokerConnection.java#L355-L377)
the relevant snippet of code from the Java SDK which populates various fields
in `ClientIdentity`.  And
[here's](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/test/java/com/bloomberg/bmq/it/PlainProducerIT.java#L98-L134)
the `PlainProducerIT.java` producer doing the same thing as well as encoding
the message and sending it over the socket to the broker.

Looking at
[`ClientIdentity`](https://github.com/bloomberg/blazingmq/blob/c8acf1b840fcd9e907d4404f43e5e687edd660f5/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L1708),
one of the fields is
[`GuidInfo`](https://github.com/bloomberg/blazingmq/blob/c8acf1b840fcd9e907d4404f43e5e687edd660f5/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L1646-L1661).  Readers should refer to the
documentation in header files of [`MessageGUIDGenerator`](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_messageguidgenerator.h) for more details.

Once the `NegotiationMessage` has been sent, the SDK waits for the broker to
respond with a `NegotiationMessage` of sub-type
[`BrokerIdentity`](https://github.com/bloomberg/blazingmq/blob/69f0f8f5b188ca1eb07b9d262093f949729db538/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L1712).
The `result` field in the `BrokerResponse` indicates if the BlazingMQ broker
accepted the handshake or not, along with some other information.
[Here's](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/test/java/com/bloomberg/bmq/it/PlainProducerIT.java#L136-L160)
the `PlainProducerIT.java` receiving, decoding and printing the
`NegotiationMessage` from the broker.  Note that this simple example is not
checking the `result` field in the response.  A SDK must check it though.  If
the `result` indicates failure, then the SDK must close the connection with the
broker.

### Opening a Queue

Assuming that the SDK has completed a successful handshake with the broker, the
client application will attempt to open a queue.  Opening a queue from the SDK
is a two step process (i.e., it requires two pairs of request/response
exchanges with the broker).  Both of these steps are encapsulated in the C++
and Java SDKs by the `openQueue` API, which looks like
[this](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/Queue.java#L24-L34)
in Java and like
[this](https://github.com/bloomberg/blazingmq/blob/f4a4f55ae74caf24995ca540a0fbf70a33fbd9c3/src/groups/bmq/bmqa/bmqa_session.h#L819-L840)
in the C++ SDK.

Let's go over the two pairs of request/response exchanges which occur under the
hood when one calls the `openQueue` API:

#### `OpenQueue` Request

The
[`OpenQueue`](https://github.com/bloomberg/blazingmq/blob/69f0f8f5b188ca1eb07b9d262093f949729db538/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L177)
request provides a way to the BlazingMQ client to attach to a queue.
`OpenQueue` contains only one field:
[`QueueHandleParameters`](https://github.com/bloomberg/blazingmq/blob/69f0f8f5b188ca1eb07b9d262093f949729db538/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L303).  Let's look at various
fields in this type:

- `uri`: A string which contains the full BlazingMQ queue
  [`URI`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/Uri.java)
  to which client wants to attach.

- `qId`: An integer identifier that the SDK and broker will use to identify any
  message for this queue.  This integer needs to be unique only between this
  SDK client and the broker.  A simple implementation would be to start with
  zero, and simply increment the value any time the client opens a new queue.
  The goal is to avoid passing the full queue URI in every message, and instead
  just use this 4-byte integer to save space on the wire.

- `subQueueIdInfo`: This field's type is
  [`SubQueueIdInfo`](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L364-L380).
  This type captures details about the sub-stream of the queue.  **TODO**

- `flags`: An integer which indicates the intent with which a BlazingMQ client
  wants to open the queue.  Valid values can be found
  [here](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqt/bmqt_queueflags.h#L46).  Note that the `e_ADMIN` is reserved and
  should not be used.

- `readCount`: An integer whose value should be one if the client wants to
  attach to the queue as a consumer, and zero otherwise.

- `writeCount`: An integer whose value should be one if the client wants to
  attach to the queue as a producer, and zero otherwise.

- `adminCount`: An integer whose value should always be zero.

[Here](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/test/java/com/bloomberg/bmq/it/PlainProducerIT.java#L162-L203), we
see the `PlainProducerIT.java` creating and sending an `OpenQueue` request.

Upon success, the BlazingMQ broker responds to the `OpenQueue` request by
sending an
[`OpenQueueResponse`](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L191-L211).  Let's go over the three
fields in this type:

- `originalRequest`: A field whose type is `OpenQueue`, and it simply carries
  the `OpenQueue` request sent by the client.

- `routingConfiguration`: An integer which indicates information about the
  message routing strategy of the queue, as indicated by the broker.  Possible
  values for the flag can be found
  [here](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_routingconfigurationutils.h#L51-L61).

- `deduplicationTimeMs`: An integer which indicates the time interval (in ms)
  for which the SDK should keep the unacknowledged PUT messages in the
  retransmission buffer before generating a local negative acknowledgement.
  This field can be ignored in the initial implementation of the SDK.

> [!NOTE]
> For every request/response type like `OpenQueue`/`OpenQueueResponse`, the top
> level schema message is always
> [`ControlMessage`](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L84).
> Readers will notice that the first field in this type is `rId`, which
> represents the request ID.  Any time the SDK sends any schema message request
> to the broker, the SDK populates `rId` field with a unique value.  The broker
> always echoes back this value in the response, which enables the SDK to
> correlate the response back to the original request, particularly in the
> presence of multiple concurrent pending requests.  In the absence of this
> integer, the SDK will not be able to identify the request to which the
> response belongs.  This integer can be assigned any value by the SDK, as long
> as it is unique among all the pending requests in the SDK at that time.

As mentioned before, opening a queue is a two-step process.  The first one
`OpenQueue`/`OpenQueueResponse`.  Let's look at the second step.  Note that a
BlazingMQ client cannot start producing to/consuming from a queue until the
second step is complete.

#### `ConfigureStream` Request

As the name suggests, the
[`ConfigureStream`](https://github.com/bloomberg/blazingmq/blob/69f0f8f5b188ca1eb07b9d262093f949729db538/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L274)
request provides a way to the BlazingMQ client to configure its "connection"
with the queue.  Note that the connection is being used logically here, and
should not be confused with a TCP connection.  This request is mostly a no-op
for producers in the BlazingMQ backend (but the producers still need to send
it).

This request is very important for consumers in many ways:

- It enables a consumer to specify one or more subscriptions.

- It enables a consumer to dynamically alter its subscriptions at any time.

- It enables a consumer to configure various attributes of a
  [subscription](../../features/subscriptions), like priority, [flow
  control](../../features/consumer_flow_control), etc.

Now that we have a high level understanding of this request, let's dig deeper.

The `ConfigureStream` request contains two fields.  The first one is `qId`,
which was explained above.  The second one is
[`streamParameters`](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L287C22-L287C38).
Let's look at the fields of this
[type](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L382-L398):

- `appId`: This string contains the name of *appId*, if any, for the consumer.
  *AppId*s are specified by consumers who are attaching to a *fan-out* queue.
  Consumers of non-fan-out queues, as well as any producers don't need to
  specify any appId.

- `subscriptions`: This field is an array of zero or more
  [`Subscription`](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L455-L471)s.
  Each `Subscription` type contains these fields:

  - `sId`: This integer represents a unique identifier for this subscription.
    This integer needs to be unique only between this client and the broker.  A
    simple incrementing
    [variable](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqt/bmqt_subscription.cpp#L44-L49)
    can be used for generating unique `sId`s.

  - `expression`: This field, which is of type
    [`Expression`](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L412C4-L426)
    contains the actual subscription expression as a string type as well as the
    version of the expression grammar.

  - `consumers`: This field is an array of type
    [`ConsumerInfo`](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L428-L453),
    and represents various attributes related to a subscription.

> [!NOTE]
> As previously mentioned, opening a queue in BlazingMQ is a two-step operation
> -- the client needs to send `OpenQueue` request, followed by
> `ConfigureStream` request.  At the API level, both of these steps should be
> hidden by simply exposing something like `openQueue` API, similar to the
> [C++](https://github.com/bloomberg/blazingmq/blob/9b692fe25f74543e954a27e30b6b15b0ae057c8d/src/groups/bmq/bmqa/bmqa_session.h#L819-L840)
> and
> [Java](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/Queue.java#L24-L34)
> SDKs.

### Posting Messages (Producers)

Let's assume that a producer application opened a BlazingMQ queue by following
the steps mentioned in the [above](#opening-a-queue) section.  The next step
for the producer application would be to post some messages (PUT messages) on
the queue.

#### Post API

Both C++ and Java SDKs support batching of messages i.e., producer application
can send a batch of messages in one shot instead of sending them one by one.
Batching can have a significant impact on the performance.  Additionally, in
both SDKs, the `post()` API is asynchronous i.e., the API does not wait for the
BlazingMQ broker to respond with ACK messages. In fact, in some cases, the API
does not write the PUT messages to the TCP socket.  Instead, the SDK just
accepts the messages, adds them to an internal buffer, and sends them later.
This can occur if the TCP layer is enforcing some flow control, or in case the
client is not connected to the broker, etc.

Keeping the above complications aside, the simplest way to send PUT messages is
to do what the [Java
SDK](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/Queue.java#L109-L115)
does.  A public class called `PutMessage` could be created, with data members
like the payload, message properties and a callback which will be invoked by
the SDK when it receives the ACK message for the PUT message from the broker.
`PutMessage` could look like this (in pseudo-code):

```
class PutMessage {
    ByteArray          mesasgePayload;
    MessageProperties  properties;
    AckCallback        ackCallback;
}
```

Here:

- `ByteArray` is, well, an array of bytes.

- `MessageProperties` is a class similar to
  [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/MessageProperties.java),
  with concrete implementation
  [here](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/proto/MessagePropertiesImpl.java).

- `AckCallback` could be similar to
  [this](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/AckMessageHandler.java)
  Java interface.

And the `post` API could look like this:

```
Result post(PutMessage msg);
```

Here, `Result` could be a custom type or enum indicating success or failure.
Alternatively, depending upon best practices in the language, the API could
throw.

Note that we did not specify the class of which the `post` API is a data
member of.  There are at least two options here:

- The `post` API can be part of a top level object like `Session` or
  `Connection`, which represent the BlazingMQ context/session/connection in the
  SDK.  In this case, the API might look similar to the
  [one](https://github.com/bloomberg/blazingmq/blob/c8acf1b840fcd9e907d4404f43e5e687edd660f5/src/groups/bmq/bmqa/bmqa_session.h#L819-L840)
  in C++.

- Alternatively, another approach could be similar to the
  [one](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/Queue.java#L109-L115)
  taken by the Java SDK, where `Queue` is represented as a first class object.

Yet another approach could be making a BlazingMQ `Domain` as a first class
object, such that:

- One creates a `Session`, `Connection` or `Context` object first
- Then, one opens a `Domain` object from the top-level object
- Then, one can open a `Queue` from the `Domain` object.

#### `post` API Implementation

The implementation of the `post` API should simply create a PUT event from the
PUT message(s) specified by the application, by following guidelines
[here](#put-event).  Some other things to keep in mind:

- Ensure that the queue has already been opened by the application.  Since the
  application is calling `post`, it must have opened the queue with *WRITE*
  intent.

- Ensure that the queue is in the process of being closed or already closed.

- Ensure that the connection with BlazingMQ broker is up, and not stopping or
  disconnected.

- Ensure that message payload is not empty.  By policy, we don't allow other
  BlazingMQ SDKs to post empty message payloads.

One of the fields in `PutHeader` is `MessageGUID`, which was described in
previous sections.  MessageGUIDs are generated in the SDK every time `post`
API is invoked.

### Processing Acknowledgement Messages (Producers)

After posting one or more messages, the SDK should expect a response from the
BlazingMQ broker in the form of an ACK (acknowledgement) message containing the
result of the `post` operation.  Multiple ACK messages can be delivered by the
broker in one ACK event, so the SDK must be able to handle them.  The wire
layout of an ACK event was discussed in a [previous](#ack-event) section.

One of the fields in ACK message is `MessageGUID`, which is used to correlate
the ACK message to its PUT message.  For example, when the application calls
the `post` API, SDK should generate a `MessageGUID`, add it to the PUT header
of that message before sending it, and also keep track of the `MessageGUID`
along with any relevant information for that PUT message (e.g., the
`AckCallback` specified by the application for that PUT message).  When the ACK
message arrives, SDK can extract `MessageGUID` from it, retrieve the
`AckCallback` and then invoke it to deliver the result to the application.

### Receiving Messages (Consumers)

An application which opens a queue as a consumer (i.e., with *READ* intent)
will receive PUSH messages from the broker anytime a new message is posted on
the queue.  Like ACK messages, PUSH messages can be delivered in a batch in a
PUSH event, and the SDK must be ready to handle it.

#### Consume API

As far as handing over PUSH messages to the application is concerned, it can be
done by asking for a callback from the application when it opens the queue, and
dispatching PUSH messages by invoking that callback.  There are various ways to
ask applications for this callback, but the most obvious one could be in the
`openQueue` API (or a flavor of it).  For example, the Java SDK has a
[`PushMessageHandler`](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-sdk/src/main/java/com/bloomberg/bmq/PushMessageHandler.java)
interface, which the consumer application should specify when
[creating](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-examples/src/main/java/com/bloomberg/bmq/examples/SimpleConsumer.java#L117C42-L117C42)
a queue.

Since multiple PUSH messages can appear in a PUSH event, the SDK implementation
must be able to handle that, and it may need to dispatch PUSH messages one by
one in the callback if the callback's signature does not support handing over
multiple PUSH messages to the application in one go.

#### Consume API Implementation

The wire layout of a PUSH event was described in a [previous](#push-event)
section.  As far as implementation is concerned, the SDK logic should keep some
of these things in mind when receiving and dispatching a PUSH event:

- Ensure that the queue is still open, and not closed or being closed by the
  application.

- Ensure that the PUSH event parsing passes basic sanity checks e.g., length of
  the packet is equal to the length specified in the `EventHeader`, etc

- Ensure that the `queueId` field specified in the `PushHeader` is valid and
  known to the SDK

### Confirming Messages (Consumers)

After opening a queue, a consumer application will receive messages which are
being posted on the queue.  Some details about handling PUSH messages were
described in the previous section.  Once the consumer application processes a
PUSH message by executing its business logic, it needs to indicate to the
BlazingMQ backend that the message has been processed successfully and can be
removed from the queue.  This is done by the consumer application sending a
CONFIRM message.

#### ConfirmMessage API

Similar to the **Post API**, C++ and Java SDKs support batching of CONFIRM
messages i.e., consumer application can send a batch of CONFIRM messages in one
shot instead of sending them one by one.  Batching can improve performance.
Additionally, in both SDKs, the `confirmMessage()` API is asynchronous; i.e.,
the API may not immediately write the CONFIRM messages to the TCP socket.
Instead, the SDK just accepts the CONFIRM messages, adds them to an internal
buffer, and sends them later.  This can occur if the TCP layer is enforcing
some flow control, or in case the SDK is not connected to the broker, etc.

Keeping above complications aside, the simplest way to send CONFIRM messages is
to do what the [Java
SDK](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/PushMessage.java#L70C8-L76)
does.  Alternatively, depending upon the API design, the `confirmMessage`
API could be part of a top level object like `Connection`, `Session`,
`Context`, `Domain`, etc, as in the [C++
SDK](https://github.com/bloomberg/blazingmq/blob/f4a4f55ae74caf24995ca540a0fbf70a33fbd9c3/src/groups/bmq/bmqa/bmqa_session.h#L1052-L1059).

#### ConfirmMessage API Implementation

The `confirmMessage` API simply needs to create and send CONFIRM message(s) to
the broker.  As explained in the [previous](#confirm-event) section, a CONFIRM
event contains a `ConfirmHeader` followed by one or more `ConfirmMessage`.

### Configuring a Queue (Standalone)

The BlazingMQ client/broker protocol also supports a standalone
*Configure Queue* request.  Recall that we have already encountered this request
in the **ConfigureStream Request** paragraph of a [previous](#opening-a-queue)
section.  Readers should refer back to that discussion for more details on this
request.

### Closing a Queue

Once a client application no longer wants to produce to or consumer from a
queue that it previously opened, it should close the queue.  Closing a queue
will not delete the queue in BlazingMQ backend.  It will simply detach the
application from the queue.

Just like opening a queue, closing a queue is a two step process (i.e., it
requires two pairs of request/response exchanges with the broker).  Both of
these steps are encapsulated in the C++ and Java SDKs by the `closeQueue` API,
which looks like
[this](https://github.com/bloomberg/blazingmq-sdk-java/blob/ed0eb848f5ac2897cfe32cc481575d689a4cb1c2/bmq-sdk/src/main/java/com/bloomberg/bmq/Queue.java#L78-L86)
in Java and like
[this](https://github.com/bloomberg/blazingmq/blob/f4a4f55ae74caf24995ca540a0fbf70a33fbd9c3/src/groups/bmq/bmqa/bmqa_session.h#L961C3-L980)
in the C++ SDK.

Let's go over the two pairs of request/response exchanges which occur under the
hood when one calls the `closeQueue` API:

#### `ConfigureStream` Request

This request has been described in the previous section.  When sending this
request as part of closing a queue, any flow control and subscription
parameters in the request are emptied out (or set to null/zero) to indicate
that the client application is about to detach from the queue.

#### `CloseQueue` Request

The
[`CloseQueue`](https://github.com/bloomberg/blazingmq/blob/58044d8e4579665fffa0419df820c8be5cdbc2eb/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L242-L260)
request provides a way to the BlazingMQ client to detach from a queue.
`CloseQueue` contains two fields.  Let's look at them:

- `QueueHandleParameters`: this type has been described in a previous section.

- `isFinal`: this boolean flag indicates if this is the final close-queue
  request for the specified queue from the client.

Upon success, the BlazingMQ broker responds to the `CloseQueue` request by
sending a
[`CloseQueueResponse`](https://github.com/bloomberg/blazingmq/blob/58044d8e4579665fffa0419df820c8be5cdbc2eb/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L262-L272).

> [!NOTE]
> The `ConfigureStream` request is sent by the SDK in three cases:
>
> 1) As the second step in the open-queue workflow (i.e., when an application
>    calls one of the flavors of the `openQueue` APIs).
>
> 2) As a standalone request when an application calls one of the flavors of
>    the `configureQueue` APIs.
>
> 3) As the first step in the close-queue workflow (i.e., when application
>    calls one of the flavors of the `closeQueue` APIs).

### Stopping a Session with BlazingMQ Broker

When a client application decides that it no longer wants to interact with
BlazingMQ, it can tear down its connection/session with the BlazingMQ broker.
This typically happens when the application is shutting down, but in theory, an
application can stop the BlazingMQ session at any time.

Stopping the BlazingMQ session is typically done by calling a
[`stop`](https://github.com/bloomberg/blazingmq-sdk-java/blob/358eda1e6ea7917e63ee6fb50d15ecd5873dbf56/bmq-sdk/src/main/java/com/bloomberg/bmq/Session.java#L512-L524)
method on the top level Session/Connection/Context object.  This method needs
to carry out three things -- it needs to inform the broker about its intent to
disconnect intent and wait for a response from the broker, and then finally it
needs to close the TCP socket with the broker.  Let's look into these steps in
detail.

#### `Disconnect` Request

The
[`Disconnect`](https://github.com/bloomberg/blazingmq/blob/58044d8e4579665fffa0419df820c8be5cdbc2eb/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L124-L134)
request is sent by the SDK to the broker to indicate that the client about to
close the TCP connection.  This is an opportunity for the broker to clean up
any state related to this session.  Once the broker has completed the cleanup,
it responds to this request by sending a
[`DisconnectResponse`](https://github.com/bloomberg/blazingmq/blob/58044d8e4579665fffa0419df820c8be5cdbc2eb/src/groups/bmq/bmqp/bmqp_ctrlmsg.xsd#L136-L147).
A guarantee provided by the broker is that once the `DisconnectResponse` has
been sent, the broker will not send any other
message/request/response/notification to the client.  Logically, the
*Disconnect* request/response pair can be thought of as a way to ensure that
the "pipe" between client and broker is empty.

#### Closing the TCP Connection

Once the SDK receives the `DisconnectResponse`, it can proceed to close the TCP
socket with the broker, and release any resources associated with the
connection.  Care must be taken to ensure that the resources are released in
the right order without any dangling references or leaks.

---

## Client Library Design Guide

Apart from implementing the BlazingMQ wire protocol and the sequence of message
exchanges between SDK and broker correctly, a good SDK should also expose easy
to use (and hard to misuse) APIs, and should have an efficient implementation.
In this section, we will go over some details about the SDK APIs and
implementation which authors of a new SDK may find useful.

### APIs

We list some guiding principles for designing BlazingMQ client APIs:

1. APIs should be designed so that they are easy to use and hard to misuse.

2. APIs should provide high enough abstraction so that users can focus on
   implementing their business logic instead of trying to understand intricate
   API interactions.
   [This](https://github.com/bloomberg/blazingmq-sdk-java/blob/main/bmq-examples/src/main/java/com/bloomberg/bmq/examples/SimpleProducer.java) Java sample producer is a good
   example which demonstrates this.

3. For certain higher level languages like Python, Ruby, etc., APIs can be
   opinionated and expose a minimal "surface area", which can simplify the APIs
   while also catering to the needs of the majority of users.

4. For languages where people often write high performance applications, APIs
   should be asynchronous as much as possible, while also providing synchronous
   flavor in some cases.

5. Overall, in our experience, BlazingMQ Java SDK provides a set of APIs which
   can be a good starting point for anyone.  SDKs in higher level languages
   should aim to further simplify the Java SDK APIs, while lower level
   languages can implement something similar to Java APIs.

### Implementation

We list some implementation details which authors of new BlazingMQ SDKs may
find useful:

1. The network I/O logic could be made asynchronous (i.e., could use
   non-blocking sockets).  This is not very important for higher level
   languages like Python, etc. but is for languages like Rust, etc.
   Non-blocking I/O can enable client applications to achieve higher
   throughput, at the cost of higher complexity of SDK implementation.  One
   could leverage some well known open source libraries which make it easier to
   achieve async I/O.  For example, the Java SDK uses
   [`netty`](https://github.com/bloomberg/blazingmq-sdk-java/tree/main/bmq-sdk/src/main/java/com/bloomberg/bmq/impl/infr/net)
   to achieve async I/O fairly easily.

2. For SDKs which are highly asynchronous in their implementation, it can be
   difficult to reason about when multiple events are occurring concurrently.
   For example, events like a user-initiated *CloseQueue* operation, network
   disconnection with the BlazingMQ broker, and timeout of an outstanding
   *OpenQueue* request could occur at the same time, and it might become
   unreasonably difficult to reason about the code.  As one of the potential
   solutions, authors may find a Finite-State Machine-based (FSM) approach easy
   to implement and reason about.  In this approach, all states, state
   transitions, events and actions can be codified in the form of enums,
   callbacks, etc.  Any incoming or outgoing event (initiated by any entity
   like the user, timer, network, etc) on the queue or the top level session
   object can be applied to the appropriate FSM (queue or session), and desired
   outcome (state transition) and action can be dispatched from the FSM table.
   As an example, the C++ SDK takes this approach (but only partially) by
   introducing concepts like:

   - [`QueueFSM`](https://github.com/bloomberg/blazingmq/blob/58044d8e4579665fffa0419df820c8be5cdbc2eb/src/groups/bmq/bmqimp/bmqimp_brokersession.h#L456)

   - [`QueueFsmEvent`](https://github.com/bloomberg/blazingmq/blob/58044d8e4579665fffa0419df820c8be5cdbc2eb/src/groups/bmq/bmqimp/bmqimp_brokersession.h#L257)

   - [`QueueStateTransition`](https://github.com/bloomberg/blazingmq/blob/58044d8e4579665fffa0419df820c8be5cdbc2eb/src/groups/bmq/bmqimp/bmqimp_brokersession.h#L320)

   - [`SessionFsm`](https://github.com/bloomberg/blazingmq/blob/58044d8e4579665fffa0419df820c8be5cdbc2eb/src/groups/bmq/bmqimp/bmqimp_brokersession.h#L365)

3. Testability of the SDK should be kept in mind, and all classes which are
   mechanisms should ideally be an interface so that they can be mocked easily.

---
