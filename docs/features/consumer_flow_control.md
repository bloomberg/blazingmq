---
layout: default
title: Consumer Flow Control
parent: Features
nav_order: 7
---

# Consumer Flow Control
{: .no_toc }

* toc
{:toc}

BlazingMQ delivers messages to consumer applications using a push model i.e.
BlazingMQ message broker pushes new messages as quickly as possible to the
consumers.  In order to ensure that consumers do not get overwhelmed and can
implement flow control, BlazingMQ provide consumer applications APIs to specify
maximum number and/or bytes of messages (or "flow control parameters") that
BlazingMQ broker can send them.  Consumers can specify flow control parameters
when they are attaching to a queue via the
[`openQueue`](../../apidocs/cpp_apidocs/classbmqa_1_1Session.html#a7b62b74a9a4d4dd3e24765d6e54e8c9a)
API , and also update them anytime later on via the
[`configureQueue`](../../apidocs/cpp_apidocs/classbmqa_1_1Session.html#af10950d3245e8acf6a4fc4403c7f433a)
API.  Flow control parameters are captured in `maxUnconfirmedMessages` and
`maxUnconfirmedBytes` parameters of
[`bmqt::QueueOptions`](../../apidocs/cpp_apidocs/group__bmqt__queueoptions.html).

Once BlazingMQ has sent the specified number of messages or bytes to the
consumer instance, it will not send any new messages to it, and such consumer
is considered to be "at capacity".  Any new messages arriving in the BlazingMQ
queue will stay in the queue and will be delivered to the consumer only when
consumer processes (i.e., confirms) pending messages that were delivered
previously.  BlazingMQ keeps track of outstanding messages sent to every
consumer to determine if a consumer instance is at capacity or not.

As an example, if a consumer instance wants no more than 1,000 messages or 100MB
worth of messages, it can specify that in the flow control API.  Specifying the
value of zero of number or bytes of messages leads to effectively pausing the
consumer -- BlazingMQ sees that consumer has no capacity and thus does not
route any messages to it.  This can be used by consumer to pause itself
temporarily in case it is not able to process messages for some reason.
Similarly, specifying a value of 1 of number of messages is equivalent to
pulling one message at a time from the queue.

Lastly, flow control parameters can be specified for every
[subscription](../subscriptions) separately if desired.

---
