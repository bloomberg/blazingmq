---
layout: default
title: Host Health Monitoring
parent: Features
nav_order: 8
---

# Host Health Monitoring
{: .no_toc }

* toc
{:toc}

## Introduction

The **Host Health Monitoring** feature allows BlazingMQ SDK clients to
automatically suspend queues when the machine running the application becomes
unhealthy.  Applications using this feature can inject their custom logic to
check the machine's health in the BlazingMQ SDK.  The SDK will periodically
invoke the custom logic and if the logic tells the SDK that the machine is
unhealthy, the SDK will suspend the queues that expressed interest in getting
suspended.

---

## Code Snippets

### C++ SDK

Users can implement their custom host health monitoring logic by implementing
[`bmqpi::HostHealthMonitor`](../../apidocs/cpp_apidocs/group__bmqpi__hosthealthmonitor.html).

The custom class can then be installed through the session options as shown
below.

```cpp

class MyHostHealthMonitor : public bmqpi::HostHealthMonitor {
    // Details elided
};

bsl::shared_ptr<MyHostHealthMonitor> monitor(new MyHostHealthMonitor());
if (err = monitor->start()) {
    // error handling
}

bmqt::SessionOptions sessionOptions;
options.setHostHealthMonitor(monitor);

bmqa::Session session(sessionOptions);

// 'QueueOptions' are used to opt in to suspension on unhealthy hosts.
bmqt::QueueOptions queueOptions;
queueOptions.setSuspendsOnBadHostHealth(true);

// This queue will suspend when 'monitor' reports that the host is unhealthy.
bmqa::OpenQueueStatus status = session.openQueueSync(&otherId,
                                                     k_OTHER_URL,
                                                     bmqt::QueuFlags::e_READ,
                                                     queueOptions);

// Queues opened with default options *will not* suspend on an unhealthy host.
status = session.openQueueSync(&queueId,
                               k_QUEUE_URL,
                               bmqt::QueueFlags::e_READ);
```

The `monitor` notifies the `bmqa::Session` when the host becomes unhealthy,
allowing the `bmqa::Session` to suspend all queues that are sensitive to host
health. A suspended queue behaves in the following ways:

* **Consumers**: The queue is configured to receive no further messages from
  the broker (`maxUnconfirmedMessages := 0; maxUnconfirmedBytes := 0;
  consumerPriority := INT_MIN`). Any previously received CONFIRM messages can
  still be sent.  can still be sent.

* **Producers**: Attempts to pack messages targeting a suspended queue will be
  rejected by `bmqa::MessageEventBuilder::pack()`. Clients may still attempt to
  `POST` existing packed events.

* **Reconfiguration**: Configuration changes to a suspended queue will only
  take effect when it resumes.

Please note:

- The SDK will by default assume that no queues are sensitive to host health
  even if the application has installed a `HostHealthMonitor`.  Clients must
  explicitly enable host-health suspension through the `bmqt::QueueOptions`
  when opening a queue.

- If a queue is sensitive to host health, and it is opened while the host is
  already unhealthy, it will be initialized into suspended state.

#### Using the `bmqa::ManualHostHealthMonitor` class
{:.no_toc}

BlazingMQ C++ SDK also includes the `bmqa::ManualHostHealthMonitor` class. This
class **does not** monitor machine status, but instead relies on clients to
explicitly set the host health through a class method.

```cpp
bsl::shared_ptr<bmqa::ManualHostHealthMonitor> monitor(
                                          new bmqa::ManualHostHealthMonitor());

bmqt::SessionOptions sessionOptions;
options.setHostHealthMonitor(monitor);

bmqa::Session session(sessionOptions);
if (rc = session.start()) {
  // error handling
}

// ...
// Set up queues
// ...

monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
  // Notifies 'session' that host is now unhealthy.
```

While most projects will not use this type for production code, it can be
useful for unit-testing code paths that are only reached during 'machine
unhealthy' events.  Less commonly, an application might use
`bmqa::ManualHostHealthMonitor` to integrate BlazingMQ with a different system
that is being used to report host health.

### Java SDK

Similar to C++ SDK, Java users can implement their custom host health
monitoring logic by implementing
[`com.bloomberg.bmq.HostHealthMonitor`](../../apidocs/java_apidocs/com/bloomberg/bmq/HostHealthMonitor.html)
interface.

The custom class can then be installed through the session options as shown
below.

```java
// Create custom monitor implementation.
com.enterprise.blazingmq.MyHostHealthMonitor monitor =
                            new com.enterprise.blazingmq.MyHostHealthMonitor();

// Start the monitor.
// Do not forget to call 'stop()' in order to stop the monitoring in case
// BlazingMQ session is stopped and the app is closing.
monitor.start();

// Configure session options
SessionOptions sessionOptions = SessionOptions.builder()
                                              .setHostHealthMonitor(monitor)
                                              .build();

// Create and start session
Session session = new Session(sessionOptions,
                              this); // SessionEventHandler

session.start(Duration.ofSeconds(15));

// Create queues
Queue queue1 = d_session.getQueue(uri1, // Queue uri
                                  QueueFlags.setReader(0), // Queue mode (=READ)
                                  this,  // QueueEventHandler
                                  this,  // AckMessageHandler
                                  this); // PushMessageHandler

Queue queue2 = d_session.getQueue(uri2, // Queue uri
                                  QueueFlags.setReader(0), // Queue mode (=READ)
                                  this,  // QueueEventHandler
                                  this,  // AckMessageHandler
                                  this); // PushMessageHandler

// Configure queue options which are used to opt in to suspension on unhealthy hosts.
QueueOptions queueOptions = QueueOptions.builder()
                                        .setSuspendsOnBadHostHealth(true)
                                        .build();

// Queue1 *will* suspend when 'monitor' reports that the host is unhealthy.
queue1.open(queueOptions, Duration.ofSeconds(15));

// Queue2 opened with default options *will not* suspend on an unhealthy host.
queue2.open(QueueOptions.createDefault(), Duration.ofSeconds(15));
```

---

## Host Health Transitions

BlazingMQ SDK issues two session-level events and two queue-level events
related to Host Health.  Clients must install a Host Health monitor in their
application to observe these event.

**Session Event**

- `e_HOST_UNHEALTHY`: The host running the application has become unhealthy.

- `e_HOST_HEALTH_RESTORED`: The health of the host running the application has
  been restored.

In Java SDK, the events are `HostUnhealthy` and `HostHealthRestored`
(interfaces with `type` property set to `HOST_UNHEALTHY_SESSION_EVENT` and
`HOST_HEALTH_RESTORED_SESSION_EVENT` accordingly).

**Queue Events**

- `e_QUEUE_SUSPENDED`: A queue has been suspended because of an unhealthy host.

- `e_QUEUE_RESUMED`: A queue has resumed when the health of the host has been
  restored.

In Java SDK the events are `SuspendQueueResult` and `ResumeQueueResult`
(interfaces with `type` property set to `QUEUE_SUSPENDED` and `QUEUE_RESUMED`
accordingly).

### Queue Semantics

An `e_QUEUE_SUSPENDED` event signals that no further messages will be delivered
to that queue until an `e_QUEUE_RESUMED` event is received. The
`bmqa::MessageEventBuilder` will reject messages from the time that an
`e_QUEUE_SUSPENDED` event is read, until the time that an analogous
`e_QUEUE_RESUMED` event is read.

In Java SDK `com.bloomberg.bmq.Queue` will reject messages to pack/post from
the time that an `QUEUE_SUSPENDED` event is read, until the time that an
analogous `QUEUE_RESUMED` event is read.

### Session Semantics

Host Health session events are only issued when the health of the **whole
application state** has changed. This distinction becomes important in certain
corner cases, such as when the health of a host is "flapping" between healthy
and unhealthy states. At such times, the BlazingMQ SDK will not issue the
`e_HOST_HEALTH_RESTORED` event until every queue has resumed **and** the host
is healthy. Conversely, the BlazingMQ SDK will only issue the
`e_HOST_UNHEALTHY` event when passing from an entirely healthy state into a
partially unhealthy one.

Consequently, during an event in which a recently healthy machine again enters
an unhealthy state before every suspended queue could be resumed, neither
`e_HOST_UNHEALTHY` nor `e_HOST_HEALTH_RESTORED` would be published (since the
SDK does not consider the whole application to have ever reentered a healthy
state).

As a corollary of this behavior, a client reading an `e_HOST_HEALTH_RESTORED`
event can assume that every queue has resumed operation until a subsequent
`e_HOST_UNHEALTHY` event is observed.

---

## Error Behavior

A host may become unhealthy for many reasons, so it's important that the
BlazingMQ SDK be able to reliably suspend a queue. The failure of a suspend
request is treated as evidence of an unusually serious problem with the
connection to the broker; if a suspend request cannot be completed, then the
session connection is dropped (this would also impact queues not configured to
be sensitive to Host Health). The BlazingMQ SDK will then undergo the usual
process to attempt to reestablish a connection.

This same "nuclear option" does not apply to queue resumption. If a queue
cannot be resumed, the error is reported as a status on the `e_QUEUE_RESUMED`
event message. We recommend clients check for such errors (however unlikely),
and decide whether it is best to close the queue, attempt again to reconfigure
it, etc. Note that the SDK will publish the `e_HOST_HEALTH_RESTORED` session
event regardless of errors encountered while resuming individual queues.

---
