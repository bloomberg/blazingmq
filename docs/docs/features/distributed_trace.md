---
layout: default
title: Distributed Trace
parent: Features
nav_order: 8
---

# Distributed Trace
{: .no_toc }

* toc
{:toc}

## Introduction

The BlazingMQ C++ SDK can integrate with any distributed tracing framework for
telemetry purposes.  The SDK is not tied to a specific tracing library or
implementation.  Instead it provides hooks such that external tracing logic can
be injected.  See next section for implementation details.

If a tracing implementation is provided, the BlazingMQ C++ SDK has the ability
to create spans for control messages sent to the BlazingMQ broker on behalf of
the client.  This includes:

* Session-Start and Session-Stop operations
* Queue-Open, Queue-Close, and Queue-Configure operations

Future work may also support creating spans for `PUT` and `CONFIRM` messages.

---

## Code Snippet

### C++ SDK

Clients may enable Distributed Trace by installing two objects through the
session options:

- A **context** responsible for defining the "current" span for a thread

- A **tracer** responsible for creating new spans

Users can inject their custom distributed tracing logic by providing concrete
implementations for these interfaces:

- [`bmqpi::DTSpan`](../../apidocs/cpp_apidocs/group__bmqpi__dtspan.html)
- [`bmqpi::DTContext`](../../apidocs/cpp_apidocs/group__bmqpi__dtcontext.html)
- [`bmqpi::DTTracer`](../../apidocs/cpp_apidocs/group__bmqpi__dttracer.html)

The custom implementation can then be installed through the session options as
shown below.

```cpp

// Assuming that `MyDTContext` and `MyDTTracer` are concrete implementations of
// `bmqpi::DTContext` and `bmqpi::DTTracer`:

// Instantiate custom distributed trace objects.
bsl::shared_ptr<bmqpi::DTContext> dtContext(new MyContext());
bsl::shared_ptr<bmqpi::DTTracer>  dtTracer(new MyTracer());

// Configure the session-options.
bmqt::SessionOptions sessionOptions;
options.setTraceOptions(dtContext, dtTracer);

// Instantiate the session object.
bmqa::Session session(sessionOptions);

// Session-start will produce spans for each request sent to the BlazingMQ
// broker on behalf of the client.
session.start();

// If a span is already active when an SDK operation is invoked, it will be
// used as a parent for any spans created by the session object.

```

### Java and Python SDKs

Distributed trace support is not yet available in the Java and Python SDKs.

---
