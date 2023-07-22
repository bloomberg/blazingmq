---
layout: default
title: Compression
parent: Features
nav_order: 5
---

# Compression
{: .no_toc }

* toc
{:toc}

## Summary

* Producer applications can indicate to the BlazingMQ SDK to compress a
  message.  BlazingMQ SDK will compress the message (both payload and
  properties) before sending it to the BlazingMQ back-end.

* On the consumer side, upon receiving a compressed message, BlazingMQ SDK will
  seamlessly decompress it before presenting it to the application layer.

* The BlazingMQ broker is not involved in the compression/decompression of the
  message.

---

## Pros & Cons

### Pros

* *Reduced Network I/O:* A compressed message will lead to less data being sent
  over the network, thereby reducing transit latency as well as network
  bandwidth usage.  Depending upon the nature of the message, compression can
  reduce the message size up to one-third of the original.

* *Reduced Storage Quota:* A compressed message will lead to reduced usage of
  disk as well as BlazingMQ domain quota.  This means that applications will be
  able to store more messages with the same quota.  In certain scenarios, it
  can also help the BlazingMQ back-end, particularly in storage replication and
  synchronization.  For *in-memory* queues, compressed messages will also help
  bring down the BlazingMQ broker's memory consumption.

### Cons

* *Higher CPU Usage:* There are no free lunches in life!  The above mentioned
  advantages come at the cost of additional CPU cycles in the producer and
  consumers applications (note that BlazingMQ back-end does not decompress
  messages).

* *Potentially Higher Latency:* Since producers compress messages, and
  consumers decompress messages, there will be some additional latency overhead
  involved in end-to-end message processing.  This latency may or may not be
  higher than the gains obtained in the transit and storage replication latency
  mentioned above.  As always, latency-sensitive applications must carry out
  careful end-to-end benchmarking with production-like traffic.

---

## General Guidelines for Enabling Compression

* Compression is useful for scenarios where the message payload and/or
  properties contain textual data of around 1KB or more.  In our benchmarks
  where messages contained randomly generated strings, we found meaningful
  gains when the message length was more than 1KB.

* Producers should not enable compression in BlazingMQ SDK if message is
  already compressed.

---

## Implementation Notes

### API and Sample Producer Snippet
{:.no_toc}

In order to enable compression for a message, producers can use the newly added
API:

* C++: `bmqa::Message::setCompressionAlgorithmType`
* Java: `PutMessage::setCompressionAlgorithm`

Sample C++ snippet:

```c++

// (Unrelated details omitted for brevity)

// Create message instance.
bmqa::Message& message = builder.startMessage();

// Set the compression algorithm type for the message.
message.setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::e_ZLIB);

// Set message payload.  Note that order of invocation of
// 'setCompressionAlgorithmType' and 'setDataRef' does not matter.
message.setDataRef(text.c_str(), text.length());

// Add message to the builder.
int rc = builder.packMessage(queueId);

```

### Supported Compression Types
{:.no_toc}

Currently, the BlazingMQ SDK supports *ZLIB* compression.  In the future,
support for additional compression algorithms can be provided as deemed
necessary.

### *ZLIB* Performance
{:.no_toc}

In our benchmark tests, we noticed that for strings larger than 1KB and
containing randomly generated alphanumeric characters, *ZLIB* resulted in a
compression ratio of around 1.6.  One should expect *ZLIB* to perform even
better in practical scenarios where a message is likely to contain repeated
patterns.

---

## Important Notes

* Note that enabling compression in a producer application is only a hint to
  the BlazingMQ SDK.  The SDK may not compress the message if compression's
  cost outweighs the benefits.  As a rule of thumb, the SDK will generally
  ignore the compression hint if message size is less than 1KB, but note that
  this is an implementation detail, and must not be relied upon for any
  purposes.

* The BlazingMQ SDK does not enable compression by default.  A producer needs
  to explicitly enable compression on a message.  This may change in subsequent
  versions of the SDK.

* Only the message payload is compressed.  Message properties are not
  compressed in order to ensure that the BlazingMQ message broker can read the
  properties efficiently if needed (for example, properties are read by the
  broker for evaluating subscriptions).

* Consumer applications do not have to worry about detecting compression and
  carrying out decompression.  The BlazingMQ SDK hides this detail from the
  consumer, and always presents a decompressed message to the application.

* In the producer application, compression is carried out in the user thread
  (i.e., the thread which invokes `bmqa::MessageEventBuilder::packMessage` in
  C++ and `Queue.post` or `Queue.flush` in Java).

* On the consumer side:

  - C++ SDK: Decompression is carried out when the application invokes
    `bmqa::Message::getData`, which is typically invoked by applications in an
    *event-handler* thread in async mode, and in user thread in sync mode (note
    that some applications may have a different thread management).

  - Java SDK: Decompression is carried out in an internal SDK thread, which is
    different from the *I/O* as well as the *event-handler* thread.

---

## Compression Stats

A BlazingMQ client can be configured to periodically report various internal
metrics to the application log (this is switched on by default).  As part of
every report, these two compression related metrics are logged as part of the
*Queue Stats* section:

- Average compression ratio for all messages for that queue in the last 300
  seconds (delta).

- Average compression ratio for all the messages from the beginning (absolute).

Note that the accumulated size of messages reported in the stats is calculated
from the compressed size of the messages.  This, along with
the compression ratio mentioned above, would help consumer applications get an
idea about their actual message sizes as well as the effectiveness of
compression.

The SDK also reports a final summary of the average compression ratio when the
application is stopped.

---
