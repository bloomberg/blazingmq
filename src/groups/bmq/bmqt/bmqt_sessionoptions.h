// Copyright 2014-2023 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// bmqt_sessionoptions.h                                              -*-C++-*-
#ifndef INCLUDED_BMQT_SESSIONOPTIONS
#define INCLUDED_BMQT_SESSIONOPTIONS

//@PURPOSE: Provide a value-semantic type to configure session with the broker.
//
//@CLASSES: bmqt::SessionOptions: options to configure a session with a
//  BlazingMQ broker.
//
//@DESCRIPTION: 'bmqt::SessionOptions' provides a value-semantic type,
// 'SessionOptions', which is used to specify session-level configuration
// parameters.
//
// Most applications should not need to change the parameters for creating a
// 'Session'; the default parameters are fine.
//
// The following parameters are supported:
//: o !brokerUri!:
//:      Address of the broker to connect to. Default is to connect to the
//:      broker on the local host on the default port (30114).  The format is
//:      'tcp://<host>:port'.  Host can be:
//:      o an explicit hostname or 'localhost'
//:      o an ip, example 10.11.12.13
//:      o a DNS entry.  In this case, the client will resolve the list of
//:        addresses from that entry, and try to connect to one of them.
//:        When the connection with the host goes down, it will automatically
//:        immediately failover and reconnects to another entry from the
//:        address list.
//:      If the environment variable 'BMQ_BROKER_URI' is set, then instances of
//:      'bmqa::Session' will ignore the 'brokerUri' field from the provided
//:      'SessionOptions' and use the value from this environment variable
//:      instead.
//:
//: o !processNameOverride!:
//:      If not empty, use this value for the processName in the identity
//:      message (useful for scripted language bindings).  This field is used
//:      in the broker's logs to more easily identify the client's process.
//:
//: o !numProcessingThreads!:
//:      Number of threads to use for processing events. Default is 1. Note
//:      that this setting has an effect only if providing a
//:      'SessionEventHandler' to the session.
//:
//: o !blobBufferSize!:
//:      Size (in bytes) of the blob buffers to use. Default value is 4k.
//:
//: o !channelHighWatermark!:
//:      Size (in bytes) to use for write cache high watermark on the
//:      channel. Default value is 128MB. This value is set on the
//:      'writeCacheHiWatermark' of the 'btemt_ChannelPoolConfiguration' object
//:      used by the session with the broker. Note that BlazingMQ reserves 4MB
//:      of this value for control message, so the actual watermark for data
//:      published is 'channelHighWatermark - 4MB'.
//:
//: o !statsDumpInterval!:
//:      Interval (in seconds) at which to dump stats in the logs. Set to 0 to
//:      disable recurring dump of stats (final stats are always dumped at end
//:      of session). Default is 5min. The value must be a multiple of 30s, in
//:      the range [0s - 60min].
//:
//: o !connectTimeout!,
//: o !disconnetTimeout!,
//: o !openQueueTimeout!,
//: o !configureQueueTimeout!,
//: o !closeQueueTimeout!,
//:      Default timeout to use for various operations.
//:
//: o !eventQueueLowWatermark!,
//: o !eventQueueHighWatermark!,
//:      Parameters to configure the EventQueue notification watermarks
//:      thresholds. The EventQueue is the buffer of all incoming events from
//:      the broker (PUSH and ACK messages as well as session and queue
//:      operations result) pending being processed by the application code.  A
//:      warning ('bmqt::SessionEventType::e_SLOWCONSUMER_HIGHWATERMARK') is
//:      emitted when the buffer reaches the 'highWatermark' value, and a
//:      notification ('bmqt::SessionEventType::e_SLOWCONSUMER_NORMAL') is
//:      sent when the buffer is back to the 'lowWatermark'. The
//:      'highWatermark' typically would be reached in case of either a very
//:      slow consumer, causing events to accumulate in the buffer, or a huge
//:      burst of data. Setting the 'highWatermark' to a high value should be
//:      done cautiously because it will potentially hide slowness of the
//:      consumer because of the enqueuing of PUSH events for a consumer, ACK
//:      events for a producer as well as all system events to the buffer
//:      (meaning that the messages may have a huge latency). Note, it is also
//:      recommended to have a reasonable distance between 'highWatermark' and
//:      'lowWatermark' values to avoid a constant back and forth toggling of
//:      state resulting from push pop of events.
//:
//: o !hostHealthMonitor!:
//:      Optional instance of a class derived from 'bmqpi::HostHealthMonitor',
//:      responsible for notifying the 'Session' when the health of the host
//:      machine has changed. A 'hostHealthMonitor' must be specified, in order
//:      for queues opened through the session to suspend on unhealthy hosts.
//:
//: o !traceOptions!:
//:      Provides the `bmqpi::DTContext` and `bmqpi::DTTracer` objects required
//:      for integration with a Distributed Trace framework. If these objects
//:      are provided, then the session will use them to create "spans" to
//:      represent requests made to the BlazingMQ broker on behalf of
//:      operations initiated by the client. This includes session-level
//:      operations (e.g., Session-Start, Session-Stop) as well as queue-level
//:      operations (e.g., Queue-Open, Queue-Configure, Queue-Close).
//

// BMQ

// BDE
#include <bdlt_timeunitratio.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATIONS
namespace bmqpi {
class DTContext;
}
namespace bmqpi {
class DTTracer;
}
namespace bmqpi {
class HostHealthMonitor;
}

namespace bmqt {

// ====================
// class SessionOptions
// ====================

/// value-semantic type for options to configure a session with a BlazingMQ
/// broker
class SessionOptions {
  public:
    // CONSTANTS
    static const char k_BROKER_DEFAULT_URI[];
    // Default URI of the 'bmqbrkr' to connect to.

    static const int k_BROKER_DEFAULT_PORT = 30114;
    // Default port the 'bmqbrkr' is listening to for client
    // to connect.

    static const int k_QUEUE_OPERATION_DEFAULT_TIMEOUT =
        5 * bdlt::TimeUnitRatio::k_SECONDS_PER_MINUTE;
    // The default, and minimum recommended, value for queue
    // operations (open, configure, close).

  private:
    // DATA
    bsl::string d_brokerUri;
    // URI of the broker to connect to (ex:
    // 'tcp://localhost:30114').  Default
    // is to connect to the local broker.

    bsl::string d_processNameOverride;
    // If not empty, use this value for the
    // processName in the identity message
    // (useful for scripted language
    // bindings).

    int d_numProcessingThreads;
    // Number of processing threads.
    // Default is 1 thread.

    int d_blobBufferSize;
    // Size of the blobs buffer.

    bsls::Types::Int64 d_channelHighWatermark;
    // Write cache high watermark to use on
    // the channel

    bsls::TimeInterval d_statsDumpInterval;
    // Interval at which to dump stats to
    // log file (0 to disable dump)

    bsls::TimeInterval d_connectTimeout;

    bsls::TimeInterval d_disconnectTimeout;

    bsls::TimeInterval d_openQueueTimeout;

    bsls::TimeInterval d_configureQueueTimeout;

    bsls::TimeInterval d_closeQueueTimeout;
    // Timeout for operations of the
    // corresponding type.

    int d_eventQueueLowWatermark;

    int d_eventQueueHighWatermark;
    // Parameters to configure the
    // EventQueue.

    int d_eventQueueSize;
    // DEPRECATED: This parameter is no
    // longer relevant and will be removed
    // in future release of libbmq.

    bsl::shared_ptr<bmqpi::HostHealthMonitor> d_hostHealthMonitor_sp;

    bsl::shared_ptr<bmqpi::DTContext> d_dtContext_sp;
    bsl::shared_ptr<bmqpi::DTTracer>  d_dtTracer_sp;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SessionOptions, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new SessionOptions using the optionally specified
    /// `allocator`.
    explicit SessionOptions(bslma::Allocator* allocator = 0);

    /// Create a new SessionOptions by copying values from the specified
    /// `other`, using the optionally specified `allocator`.
    SessionOptions(const SessionOptions& other,
                   bslma::Allocator*     allocator = 0);

    // MANIPULATORS

    /// Set the broker URI to the specified `value`.
    SessionOptions& setBrokerUri(const bslstl::StringRef& value);

    /// Set an override of the proces name to the specified `value`.
    SessionOptions& setProcessNameOverride(const bslstl::StringRef& value);

    /// Set the number of processing threads to the specified `value`.
    SessionOptions& setNumProcessingThreads(int value);

    /// Set the specified `value` for the size of blobs buffers.
    SessionOptions& setBlobBufferSize(int value);

    /// Set the specified `value` (in bytes) for the channel high
    /// watermark.  The behavior is undefined unless
    /// `8 * 1024 * 1024 < value`.
    SessionOptions& setChannelHighWatermark(bsls::Types::Int64 value);

    /// Set the statsDumpInterval to the specified `value`. The behavior is
    /// undefined unless `value` is a multiple of 30s and less than 60
    /// minutes.
    SessionOptions& setStatsDumpInterval(const bsls::TimeInterval& value);

    /// Set the timeout for connecting to the broker to the specified `value`.
    SessionOptions& setConnectTimeout(const bsls::TimeInterval& value);

    /// Set the timeout for disconnecting from the broker to the specified
    /// `value`.
    SessionOptions& setDisconnectTimeout(const bsls::TimeInterval& value);

    /// Set the timeout for opening a queue to the specified `value`.
    SessionOptions& setOpenQueueTimeout(const bsls::TimeInterval& value);

    /// Set the timeout for configuring or deconfiguring a queue to the
    /// specified `value`.
    SessionOptions& setConfigureQueueTimeout(const bsls::TimeInterval& value);

    /// Set the timeout for closing a queue to the specified `value`.
    SessionOptions& setCloseQueueTimeout(const bsls::TimeInterval& value);

    /// Set a `HostHealthMonitor` object that will notify the session when
    /// the health of the host has changed.
    SessionOptions& setHostHealthMonitor(
        const bsl::shared_ptr<bmqpi::HostHealthMonitor>& monitor);

    /// Set the `DTContext` and `DTTracer` objects needed to integrate with
    /// Distributed Trace frameworks. Either both arguments must point to
    /// valid instances, or both must be empty shared_ptr's; if either is an
    /// empty shared_ptr and the other is not, then behavior is undefined.
    SessionOptions&
    setTraceOptions(const bsl::shared_ptr<bmqpi::DTContext>& dtContext,
                    const bsl::shared_ptr<bmqpi::DTTracer>&  dtTracer);

    /// DEPRECATED: Use 'configureEventQueue(int lowWatermark,
    ///                                      int highWatermark)'
    ///             instead.  This method will be marked as
    ///             `BSLS_ANNOTATION_DEPRECATED` in future release of
    ///             libbmq.
    SessionOptions&
    configureEventQueue(int queueSize, int lowWatermark, int highWatermark);

    /// Configure the EventQueue notification watermarks thresholds with the
    /// specified `lowWatermark` and `highWatermark` value.  Refer to the
    /// component level documentation for explanation of those watermarks.
    /// The behavior is undefined unless `lowWatermark < highWatermark`.
    SessionOptions& configureEventQueue(int lowWatermark, int highWatermark);

    // ACCESSORS

    /// Get the broker URI.
    const bsl::string& brokerUri() const;

    /// Return the process name override.
    const bsl::string& processNameOverride() const;

    /// Get the number of processing threads.
    int numProcessingThreads() const;

    /// Get the size of the blobs buffer.
    int blobBufferSize() const;

    /// Get the channel high watermark.
    bsls::Types::Int64 channelHighWatermark() const;

    /// Get the stats dump interval.
    const bsls::TimeInterval& statsDumpInterval() const;

    /// Get the timeout for connecting to the broker.
    const bsls::TimeInterval& connectTimeout() const;

    /// Get the timeout for disconnecting from the broker.
    const bsls::TimeInterval& disconnectTimeout() const;

    /// Get the timeout for opening a queue.
    const bsls::TimeInterval& openQueueTimeout() const;

    /// Get the timeout for configuring or disconfiguring a queue.
    const bsls::TimeInterval& configureQueueTimeout() const;

    /// Get the timeout for closing a queue.
    const bsls::TimeInterval& closeQueueTimeout() const;

    const bsl::shared_ptr<bmqpi::HostHealthMonitor>& hostHealthMonitor() const;

    /// Get the Distributed Trace context object.
    const bsl::shared_ptr<bmqpi::DTContext>& traceContext() const;

    /// Get the Distributed Trace tracer object.
    const bsl::shared_ptr<bmqpi::DTTracer>& tracer() const;

    int eventQueueLowWatermark() const;
    int eventQueueHighWatermark() const;

    /// DEPRECATED: This parameter is no longer relevant and will be removed
    /// in future release of libbmq.
    int eventQueueSize() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const SessionOptions& lhs, const SessionOptions& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const SessionOptions& lhs, const SessionOptions& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const SessionOptions& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// class SessionOptions
// --------------------

// MANIPULATORS
inline SessionOptions&
SessionOptions::setBrokerUri(const bslstl::StringRef& value)
{
    d_brokerUri.assign(value.data(), value.length());
    return *this;
}

inline SessionOptions&
SessionOptions::setProcessNameOverride(const bslstl::StringRef& value)
{
    d_processNameOverride.assign(value.data(), value.length());
    return *this;
}

inline SessionOptions& SessionOptions::setNumProcessingThreads(int value)
{
    d_numProcessingThreads = value;
    return *this;
}

inline SessionOptions& SessionOptions::setBlobBufferSize(int value)
{
    d_blobBufferSize = value;
    return *this;
}

inline SessionOptions&
SessionOptions::setChannelHighWatermark(bsls::Types::Int64 value)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(8 * 1024 * 1024 < value);
    // We reserve 4MB for control message (see
    // 'bmqimp::BrokerSession::k_CONTROL_DATA_WATERMARK_EXTRA') so make
    // sure the provided value is greater than it.

    d_channelHighWatermark = value;
    return *this;
}

inline SessionOptions&
SessionOptions::setStatsDumpInterval(const bsls::TimeInterval& value)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(value.seconds() % 30 == 0 &&
                    "value must be a multiple of 30s");
    BSLS_ASSERT_OPT(value.seconds() <
                        bdlt::TimeUnitRatio::k_SECONDS_PER_HOUR &&
                    "value must be < 10 min");

    d_statsDumpInterval = value;
    return *this;
}

inline SessionOptions&
SessionOptions::setConnectTimeout(const bsls::TimeInterval& value)
{
    d_connectTimeout = value;
    return *this;
}

inline SessionOptions&
SessionOptions::setDisconnectTimeout(const bsls::TimeInterval& value)
{
    d_disconnectTimeout = value;
    return *this;
}

inline SessionOptions&
SessionOptions::setOpenQueueTimeout(const bsls::TimeInterval& value)
{
    d_openQueueTimeout = value;
    return *this;
}

inline SessionOptions&
SessionOptions::setConfigureQueueTimeout(const bsls::TimeInterval& value)
{
    d_configureQueueTimeout = value;
    return *this;
}

inline SessionOptions&
SessionOptions::setCloseQueueTimeout(const bsls::TimeInterval& value)
{
    d_closeQueueTimeout = value;
    return *this;
}

inline SessionOptions& SessionOptions::setHostHealthMonitor(
    const bsl::shared_ptr<bmqpi::HostHealthMonitor>& monitor)
{
    d_hostHealthMonitor_sp = monitor;
    return *this;
}

inline SessionOptions& SessionOptions::setTraceOptions(
    const bsl::shared_ptr<bmqpi::DTContext>& context,
    const bsl::shared_ptr<bmqpi::DTTracer>&  tracer)
{
    BSLS_ASSERT_OPT((context && tracer) || (!context && !tracer));

    d_dtContext_sp = context;
    d_dtTracer_sp  = tracer;
    return *this;
}

inline SessionOptions& SessionOptions::configureEventQueue(int queueSize,
                                                           int lowWatermark,
                                                           int highWatermark)
{
    d_eventQueueSize = queueSize;
    return configureEventQueue(lowWatermark, highWatermark);
}

inline SessionOptions& SessionOptions::configureEventQueue(int lowWatermark,
                                                           int highWatermark)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(lowWatermark < highWatermark);

    d_eventQueueLowWatermark  = lowWatermark;
    d_eventQueueHighWatermark = highWatermark;

    return *this;
}

// ACCESSORS
inline const bsl::string& SessionOptions::brokerUri() const
{
    return d_brokerUri;
}

inline const bsl::string& SessionOptions::processNameOverride() const
{
    return d_processNameOverride;
}

inline int SessionOptions::numProcessingThreads() const
{
    return d_numProcessingThreads;
}

inline int SessionOptions::blobBufferSize() const
{
    return d_blobBufferSize;
}

inline bsls::Types::Int64 SessionOptions::channelHighWatermark() const
{
    return d_channelHighWatermark;
}

inline const bsls::TimeInterval& SessionOptions::statsDumpInterval() const
{
    return d_statsDumpInterval;
}

inline const bsls::TimeInterval& SessionOptions::connectTimeout() const
{
    return d_connectTimeout;
}

inline const bsls::TimeInterval& SessionOptions::disconnectTimeout() const
{
    return d_disconnectTimeout;
}

inline const bsls::TimeInterval& SessionOptions::openQueueTimeout() const
{
    return d_openQueueTimeout;
}

inline const bsls::TimeInterval& SessionOptions::configureQueueTimeout() const
{
    return d_configureQueueTimeout;
}

inline const bsls::TimeInterval& SessionOptions::closeQueueTimeout() const
{
    return d_closeQueueTimeout;
}

inline const bsl::shared_ptr<bmqpi::HostHealthMonitor>&
SessionOptions::hostHealthMonitor() const
{
    return d_hostHealthMonitor_sp;
}

inline const bsl::shared_ptr<bmqpi::DTContext>&
SessionOptions::traceContext() const
{
    return d_dtContext_sp;
}

inline const bsl::shared_ptr<bmqpi::DTTracer>& SessionOptions::tracer() const
{
    return d_dtTracer_sp;
}

inline int SessionOptions::eventQueueLowWatermark() const
{
    return d_eventQueueLowWatermark;
}

inline int SessionOptions::eventQueueHighWatermark() const
{
    return d_eventQueueHighWatermark;
}

inline int SessionOptions::eventQueueSize() const
{
    return d_eventQueueSize;
}

}  // close package namespace

// --------------------
// class SessionOptions
// --------------------

inline bool bmqt::operator==(const bmqt::SessionOptions& lhs,
                             const bmqt::SessionOptions& rhs)
{
    return lhs.brokerUri() == rhs.brokerUri() &&
           lhs.numProcessingThreads() == rhs.numProcessingThreads() &&
           lhs.blobBufferSize() == rhs.blobBufferSize() &&
           lhs.channelHighWatermark() == rhs.channelHighWatermark() &&
           lhs.statsDumpInterval() == rhs.statsDumpInterval() &&
           lhs.connectTimeout() == rhs.connectTimeout() &&
           lhs.openQueueTimeout() == rhs.openQueueTimeout() &&
           lhs.configureQueueTimeout() == rhs.configureQueueTimeout() &&
           lhs.closeQueueTimeout() == rhs.closeQueueTimeout() &&
           lhs.eventQueueLowWatermark() == rhs.eventQueueLowWatermark() &&
           lhs.eventQueueHighWatermark() == rhs.eventQueueHighWatermark() &&
           lhs.hostHealthMonitor() == rhs.hostHealthMonitor() &&
           lhs.traceContext() == rhs.traceContext() &&
           lhs.tracer() == rhs.tracer();
}

inline bool bmqt::operator!=(const bmqt::SessionOptions& lhs,
                             const bmqt::SessionOptions& rhs)
{
    return lhs.brokerUri() != rhs.brokerUri() ||
           lhs.numProcessingThreads() != rhs.numProcessingThreads() ||
           lhs.blobBufferSize() != rhs.blobBufferSize() ||
           lhs.channelHighWatermark() != rhs.channelHighWatermark() ||
           lhs.statsDumpInterval() != rhs.statsDumpInterval() ||
           lhs.connectTimeout() != rhs.connectTimeout() ||
           lhs.openQueueTimeout() != rhs.openQueueTimeout() ||
           lhs.configureQueueTimeout() != rhs.configureQueueTimeout() ||
           lhs.closeQueueTimeout() != rhs.closeQueueTimeout() ||
           lhs.eventQueueLowWatermark() != rhs.eventQueueLowWatermark() ||
           lhs.eventQueueHighWatermark() != rhs.eventQueueHighWatermark() ||
           lhs.hostHealthMonitor() != rhs.hostHealthMonitor() ||
           lhs.traceContext() != rhs.traceContext() ||
           lhs.tracer() != rhs.tracer();
}

inline bsl::ostream& bmqt::operator<<(bsl::ostream&               stream,
                                      const bmqt::SessionOptions& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
