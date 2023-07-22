---
layout: default
title: High Availability in SDKs
parent: Architecture
nav_order: 5
---

# High Availability in BlazingMQ Client Libraries
{: .no_toc }

* toc
{:toc}

## Introduction

This article introduces readers to the notion of high availability ("HA") in
BlazingMQ client libraries (also referred to as SDKs).  We will see how high
availability in client libraries helps users write simpler applications than
before, while protecting applications from transient issues in the BlazingMQ
back-end, network issues, etc.

High availability in client libraries complements [*High Availability in
BlazingMQ Back-end*](../high_availability), and they work together to provide a
seamless experience to BlazingMQ applications in case of framework crashes,
machine issue, network faults, etc.

---

## Motivation

Looking at BlazingMQ's [network topology](../network_topology), one can see
that there are several points of failure along the *producer -> primary node*
as well as *primary node -> consumer* paths:

1. Links between proxy and cluster nodes can go down

2. Links among cluster nodes can go down

3. Any number of nodes in the cluster can crash

4. Links between applications and proxies can go down (as a result of proxy
   crash etc).

Any of these events disrupt data flow and can impact applications.  As a result
of HA in BlazingMQ back-end, applications are protected from events in 1-3
(readers can refer to [HA in BlazingMQ back-end](../high_availability) article
for more details).

However, event (4) can still impact applications.  If BlazingMQ node to which
applications are connected to (typically referred as "BlazingMQ proxy" or
"local BlazingMQ proxy") goes down, applications can notice this behavior:

1. Producer application will get an error when attempting to post a *PUT*
   message

2. Consumer application will get an error which attempting to confirm a
   message

3. Producer or consumer application will get an error when attempting to:
   - open a new queue
   - configure an existing queue
   - close a queue

Any of these events can be very disruptive for applications and they need to
implement a non-trivial state machine to handle scenarios where new work needs
to be submitted to BlazingMQ, but SDK is not connected to BlazingMQ proxy.
Lets take the example of a producer application and see how it gets affected:

- A producer application needs to start buffering any *PUT* messages while
  connection with BlazingMQ proxy is down, and then transmit them once
  connectivity with BlazingMQ is reestablished.

- Additionally, status of pending *PUT* messages (messages for which *ACK*s
  were not received before connection with BlazingMQ went down) becomes
  unknown.  They may or may not have reached the BlazingMQ queue.  Due to this
  ambiguity, prior to HA work, BlazingMQ SDK would generate negative *ACK*s for
  such *PUT* messages with `status = UNKNOWN`, indicating to the producer
  application that the message may or may not have made it to the queue.  Some
  applications could choose to retransmit such *PUT* messages upon
  reconnectivity, with the possibility that same message could now appear in
  the queue twice.  It is worth noting that the two copies of this message
  would have different Message GUIDs assigned to them, making it further
  challenging for consumer applications to deduplicate such copies.

---

## Design

Just like high availability in BlazingMQ back-end, all cases described above can
be solved by buffering any work submitted to the SDK when connectivity is down,
and transmitting buffered work and retransmitting any pending work upon
reconnection. HA in BlazingMQ SDK takes the same approach.  Specifically:

1. All *PUT* messages which are submitted by producer application are buffered
   by the SDK in a collection, irrespective of the state of connectivity with
   BlazingMQ back-end.  This means that once the queue has been successfully
   opened by the producer application, it can continue to post *PUT* messages
   without worrying about connection's status.  This is one of the most
   important changes in the behavior of the SDK.  Previously, producer
   application would get a `NOT_CONNECTED` error code from the API when
   attempting to post *PUT* message during disconnection, and would need to
   buffer such *PUT* messages.  This is no longer required.

2. In addition, SDK now generates and assigns a unique `bmqt::MessageGUID` to
   every *PUT* message submitted by the application.  Prior to HA in SDK, GUIDs
   were assigned by the first BlazingMQ back-end (hop closest to the producer
   application, typically the local BlazingMQ proxy).  Motivation for this
   change will be explained shortly.

3. Upon receiving *ACK* message from BlazingMQ, SDK removes the corresponding
   *PUT* message from the collection mentioned in (1) above, since that *PUT*
   message is no longer pending (unacknowledged).

4. When connection with BlazingMQ proxy is down, any new *PUT* messages
   submitted by the application will continue to be buffered in the collection
   in (1).

5. Once the connection is restored and all queues are reopened, all *PUT*
   messages in the collection are transmitted to BlazingMQ back-end.  It is
   important to note that some of these *PUT* message could have been sent to
   BlazingMQ back-end prior to connection drop and SDK was waiting for *ACK*s
   for them.  Moreover, some of these *PUT* messages could have been accepted
   by BlazingMQ queue.  Such *PUT* messages will be seamlessly deduped by
   BlazingMQ back-end, because BlazingMQ primary node maintains a history of
   *MessageGUIDs* seen in the last few minutes (configurable), and if a *PUT*
   message arrives on the queue with a *GUID* which exists in the historical
   list of *GUID*s, it is simply acknowledged back without being added to the
   queue.  This ensures that producer application does not see any error, and
   only one copy of the message appears in the queue.  For this logic to work
   correctly, it is important that the source (SDK) itself generates and
   assigns a *GUID* to every *PUT* message, as discussed in (2) above.

6. Unlike *PUT* messages, *CONFIRM* messages will not be buffered by the SDK if
   connection to BlazingMQ proxy is down.  While this may seem inconsistent,
   there is a good reason for this behavior -- very often, there are multiple
   consumers attached to a queue.  If a BlazingMQ node loses route to the
   consumer which is processing *PUSH* messages and sending *CONFIRM*s, it is
   extremely likely that the BlazingMQ node will pick another consumer on a
   different route and start delivering those unconfirmed *PUSH* messages to
   the new consumer immediately.  As such, even if original consumer buffers
   *CONFIRM* messages when it is disconnected to BlazingMQ proxy and transmits
   them upon reconnection, those *CONFIRM* messages will simply be ignored by
   BlazingMQ primary node as they are stale, and those *PUSH* messages will get
   processed twice -- once by original consumer and then by new consumer.  Note
   that this behavior is within contract, as BlazingMQ provides *at least once*
   delivery guarantee by design.  We do have some ideas to minimize such
   duplicate *PUSH* messages, and we may work on it in the near future.

7. *OpenQueue* and *ConfigureQueue* requests will be buffered during
   disconnection and transmitted upon reconnection.  Additionally, if a request
   is pending while connection goes down, it will not be failed, but instead
   seamlessly resent upon reconnection.

8. Lastly, *CloseQueue* request is not buffered during disconnection.  Queue is
   simply marked as closed in the SDK and a response is generated locally.
   Upon reconnection, this queue is not reopened, effectively leaving the queue
   in the closed state.

### TCP High Watermark Handling

Prior to HA logic in SDK, if TCP connection between SDK and BlazingMQ proxy is
flow-controlled such that SDK receives a push back from TCP socket when
attempting to send something, the SDK would buffer messages up to a certain
limit (typically 128MB), and any attempt to send messages beyond that limit
would return `BW_LIMIT` (*Bandwidth Limit*) error to the application, and in
some cases, would simply drop the TCP connection.

This is no longer the case.  SDK will continue to accept messages from the
application even when it is getting a push back from TCP socket, and instead of
enforcing a size limit, it now enforces a time limit of 5 seconds
(non-configurable, hard-coded in the SDK) for the TCP channel to be "open"
again.  After this time interval, if TCP channel is still flow-controlled, SDK
will return `BW_LIMIT` error to the application.  It is important to note that
the wait for this time interval occurs in the application thread which calls
the SDK API to send message.  This is done to ensure that the push back
indicated by TCP layer is exposed all the way back to the application.

---
