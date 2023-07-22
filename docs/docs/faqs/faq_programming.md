---
layout: default
title: Programming FAQs
parent: FAQs
nav_order: 3
---

# Programming FAQs

## Where is the reference documentation for BlazingMQ APIs?

C++ SDK documentation can be found
[here](../../apidocs/cpp_apidocs/index.html).

Java SDK documentation can be found
[here](../../apidocs/java_apidocs/index.html).

---

## What are the different modes in which a queue can be opened?

A queue can be opened in *READ* mode, *WRITE* mode or *READ|WRITE* mode which
are self-explanatory.  If *WRITE* mode is specified, additional `e_ACK` flag
can also be specified while opening the queue.

---

## What's the purpose of correlation ID in BlazingMQ C++ and Java APIs?

When using BlazingMQ C++ or Java SDK, applications can receive events
asynchronously.  In order to enable applications to identify these events
efficiently, each user action which can result in the delivery of an
asynchronous event to the application takes an ID.  For example, when opening a
queue, the application needs to specify a `bmqt::CorrelationID` to the
`bmqa::QueueId` instance, so that the open-queue result via
`bmqa::SessionEventHandler::onSessionEvent` can be correlated with the
open-queue request.  Another example is posting a message: the application
can specify a correlation ID when posting a message, so that message's
acknowledgement status can be correlated to the message.

A correlation ID can be used as a key in associative containers, and can be
used to retrieve some context needed to process the asynchronous event.  For
example, if a consumer application has opened multiple queues, upon receiving a
message, it can retrieve the correlation ID of the queue containing the message
via `bmqa::Message::QueueId`, followed by a call to
`bmqa::QueueId::correlationId`, and use that correlation ID to retrieve that
queue's context, which may include things like the message processing callback
for that queue, etc.

---

## Does Correlation ID need to be unique every time I specify it?

Yes, so that every asynchronous event can be uniquely identified by the
application.  Note that it needs to be unique *only* within the context of a
`bmqa::Session` instance, not across your entire application ecosystem.

---

## What does the e_ACK flag mean in `bmqa::Session::openQueue*` APIs? Do I need to specify a correlation ID every time?

[*Correlation ID*](../../apidocs/cpp_apidocs/group__bmqt__correlationid.html)
is an identifier specified by the producer in a *PUT* message to correlate an
*ACK* message sent by the broker in response to that *PUT* message.  An *ACK*
message notifies the producer that the corresponding *PUT* message has been
accepted by the broker (success) or rejected (failure; due to the queue being
full, long running network issue, etc).

If a producer has specified the `e_ACK` flag while opening the queue, then it
*must* specify a correlation ID for each message.  If the producer attempts to
build a message without a correlation ID,
[`bmqa::MessageEventBuilder::packMessage`](../../apidocs/cpp_apidocs/classbmqa_1_1MessageEventBuilder.html#a101f3eea6233a4ff2b78b6b8072e3c50)
will return
[`bmqt::EventBuilderResult::e_MISSING_CORRELATION_ID`](../../apidocs/cpp_apidocs/structbmqt_1_1EventBuilderResult.html#a04d8f1ec70faff7c9dce79d82c14844ca056766d02f68b50ea998c4a4a6490ad3).
In other words, the SDK ensures that the application specifies a correlation ID
for each message.

If the producer has not specified the `e_ACK` flag while opening the queue,
specifying correlation ID in a *PUT* message is optional. The general idea is
that if an application considers a particular message important and wants to
ensure that broker has accepted that message, it can specify a correlation ID,
so that the broker sends an *ACK* message (with success/failure status) for
that message.

A critical application which needs to keep track of the status of every message
that was posted to the queue should specify a correlation ID for each message,
and should also specify the `e_ACK` flag while opening the queue in order to
ensure that no message without a correlation ID is accidentally posted to the
queue.

---

## When does a producer application get ACK messages?

A broker will send an *ACK* message to the producer for each message that the
producer posted with a correlation ID, irrespective of an `e_ACK` flag provided
by the producer while opening the queue.

---

## What is an *Event Queue*? I see references to it in my BlazingMQ application logs.

Messages received by the BlazingMQ client library from a BlazingMQ broker are
processed in an internal thread, and then enqueued in a buffer known as an
*Event Queue* to be dispatched to an application in the BlazingMQ SDK's event
handler thread.  The *Event Queue* is just an in-memory inbound buffer, and is
in *no* way related to the BlazingMQ queue, which is hosted on the BlazingMQ
cluster.

Various events like *ACK*, *PUSH*, responses, etc. are popped off from the
event queue and delivered to the application in the BlazingMQ SDK's event
handler thread, in methods like `bmqa::Session:onSessionEvent` and
`bmqa::Session::onMessageEvent`.  If the application carries out expensive
business logic in these methods, the event handler thread will not be able to
pop off events from the event queue in a timely manner, thereby leading to the
growth of the event queue as new events are sent by BlazingMQ broker.

In such cases, if the length of the event queue grows beyond the configured
high-watermark value, the SDK will emit an `e_SLOWCONSUMER_HIGHWATERMARK` event
as a warning to the application.

The `SessionOptions` parameters `eventQueueLowWatermark` and
`eventQueueHighWatermark` can be used to configure high and low watermarks for
the event queue.

If an application repeatedly encounters `e_SLOWCONSUMER_HIGHWATERMARK` event,
it is likely an indication that the application is slow at processing events or
needs to carry out processing of events in a thread different from BlazingMQ's
event handler thread (a thread pool, etc).  Additionally, the application can
also bump up the value of `eventQueueHighWatermark` to a higher value, but it
should be done cautiously because doing so may potentially hide slowness in the
application.

---

## I don't want BlazingMQ to push all messages to my consumer at once.  How can I implement flow control?

Flow control can be easily enforced by a consumer application by specifying
appropriate values in the
[`bmqt::QueueOptions`](../../apidocs/cpp_apidocs/classbmqt_1_1QueueOptions.html)
parameter when opening or configuring a queue.

`bmqt::QueueOptions` contains two variables which are pertinent to flow
control: `MaxUnconfirmedMessages` and `MaxUnconfirmedBytes`.  The former
determines the maximum number of messages that the broker can deliver to the
consumer for that queue, without having the consumer confirm them.  The latter
determines the same for maximum number of bytes.  If any of these values are
reached for a consumer, the broker will not send any new messages until the
consumer confirms some of the messages.  Such a consumer is said to have
reached its capacity.

In the meantime, messages will remain safely in the broker and may be
distributed to other consumers if applicable.

Note that the flow control parameters can be changed at any time by the
consumer using the `configureQueue` API.  This can be very useful for
consumers.  At interesting use of the `configureQueue` API is that consumers
can tell BlazingMQ to not send them any more messages by specifying a value of
zero for both `MaxUnconfirmedMessages` and `MaxUnconfirmedBytes`.

Additionally, consumers can specify a value of 1 for `MaxUnconfirmedMessages`
in the `openQueue` or `configureQueue` APIs which will ensure that BlazingMQ
will send only one message at a time to the consumers.  This can be useful for
consumers where each message represents a heavy job and can run for hours.

While it is okay for consumers to reach capacity on some occasions, they must
not stay in that state forever. This would cause the queue to fill up and
eventually reach its quota, which leads to new messages being rejected by
BlazingMQ and the producer receiving failed *ACK* messages.  BlazingMQ raises
several alarms in the scenario of stuck consumers and queues getting full.

---

## When does BlazingMQ redeliver a message to another consumer?

In general, as far as BlazingMQ is concerned, once a message has been delivered
to the consumer, it is 'processing' it, and BlazingMQ does not make any
assumption about how long the processing step should take.  However, BlazingMQ
redelivers a message to another consumer, if available, in *any* of these
conditions:

- The original consumer crashes without confirming the message
- The original consumer closes the queue without confirming the message
- The original consumer stops BlazingMQ session without confirming the message
- The TCP connection between original consumer and broker drops due to network
  issue

---

## Can I open multiple queues from one `bmqa::Session` object?

Yes.  Any number of queues can be opened from one `bmqa::Session` object, and
creating multiple queues per session object is the recommended approach,
instead of creating one `bmqa::Session` object per queue.

---

## Should I open and close a queue repeatedly in my application?

No!  The recommended approach is to open the queue when the application starts
(or needs it first), and then keep the queue open until it is no longer needed.
An application must not open and close a queue within a span of a few seconds (
or even a few minutes).  Particularly, applications must not open a queue, post
a message, close the queue right away, and then repeat this pattern.  A queue
can be thought of as an equivalent to a database handle, which should not be
acquired and released repeatedly.

While the representation of a queue in a BlazingMQ cluster is cheap, the
creation, opening and closing of a queue are expensive operations.  Creation of
a queue requires consensus among nodes of the BlazingMQ cluster, and opening
and closing of a queue require 2 sets of requests and responses along the route
of the application and the queue's primary node.  One can see how opening and
closing a queue repeatedly in a short span of time may keep the BlazingMQ
cluster unnecessarily busy.

---

## Can the `openQueue` API fail?

Yes, it can fail, due to any of the reasons listed below:

- An application attempts to open a queue for a namespace (BlazingMQ domain)
  which is not yet registered.

- An application attempts to open a queue for a BlazingMQ domain from a machine
  which is not configured for that BlazingMQ domain.

- A bad configuration deployed in BlazingMQ back-end (rare, but has non-zero
  probability).

- A long-standing network issue leads to a connection failure between the
  application and the BlazingMQ back-end, or within various BlazingMQ nodes in
  the back-end (rare, but has non-zero probability).

- A bug in the BlazingMQ back-end

---
