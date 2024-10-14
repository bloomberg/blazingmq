---
layout: default
title: Network Transport
parent: Architecture
nav_order: 7
---

# Network Transport in BlazingMQ
{: .no_toc }

* toc
{:toc}

This article provides brief overview of the network transport layer of
BlazingMQ.

## Transport Type

BlazingMQ exclusively uses TCP for all communication within the cluster as well
as with clients.  TCP provides nice properties of ordering and retransmissions
of packets, which makes the implementation of networking logic simpler in
BlazingMQ.

## Transport Library

Instead of implementing various low level network building blocks from scratch,
BlazingMQ leverages Bloomberg's [Network Transport
Framework](https://github.com/bloomberg/ntf-core), which was published as open
source in March 2023.  NTF enables BlazingMQ to implement non-blocking
asynchronous network I/O in an efficient manner.  In the words of its authors:

*"The Network Transport Framework (NTF) is an open-source collection of
libraries for asynchronous network programming for scalable, high-performance
applications.*

*In general, NTF provides building blocks and higher-level abstractions for
asynchronously sending and receiving data between processes. More specifically,
NTF provides an abstraction around the differences in the native networking
APIs offered by the supported operating system, and introduces many useful
features commonly required by networked applications. The principle feature of
the sockets of this library is the introduction of virtual send and receive
queues, which provide a message-oriented, thread-safe (concurrent), send and
receive API for sockets using transports with either datagram semantics or
stream semantics. Additionally, these virtual queues allow users to operate
sockets both reactively (i.e. in the Unix readiness model) or proactively
(i.e. in the Windows I/O completion model), regardless of the operating system
and interface to that operating system (e.g. select, kqueue, epoll, io_uring,
I/O completion ports etc.) NTF supports both stream sockets using TCP and
datagram sockets using UDP communicating over either IPv4 or IPv6 networks. NTF
also supports local (aka Unix) domain sockets for efficiently communicating
between processes on the same machine.*

*The mechanisms in NTF are scalable, efficient, and multi-threaded. Its
libraries are intended for applications needing to manage anywhere from a
single socket to many thousands of simultaneous sockets."*

### Transport Abstraction

 In its implementation, BlazingMQ abstracts the transport by using
[`bmqio`](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqio/bmqio_channel.h)
interface, which enables BlazingMQ to plug in any implementation which conforms
to it.  In fact, BlazingMQ was using a legacy network transport library instead
of NTF some time back.  We have also experimented with plugging in
[`boost::asio`](https://www.boost.org/doc/libs/1_82_0/doc/html/boost_asio.html)
as the transport implementation, and using it or another implementation remains
a future possibility.

## Wire Protocol

BlazingMQ uses a custom wire protocol over TCP, the details of which can be
found in
[this](https://github.com/bloomberg/blazingmq/blob/main/src/groups/bmq/bmqp/bmqp_protocol.h)
header file.  There were four guiding principles when this protocol was
designed:

- *Compactness*: The protocol should have minimal overhead.

- *Forward and backward compatibility*: The protocol should be easily
  extensible while remaining backward compatible.

- *Batching*: The protocol should support batching of messages wherever
  possible.

- *Efficiency*: The protocol should avoid encoding/decoding overhead for
  frequent messages, while still taking into consideration concerns like
  endianness, etc.

Additionally, several components in the
[`bmqp`](https://github.com/bloomberg/blazingmq/tree/main/src/groups/bmq/bmqp)
package provide utilities to build and parse network packets conforming to the
wire protocol.

---
