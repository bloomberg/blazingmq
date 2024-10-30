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

// bmqimp_application.h                                               -*-C++-*-
#ifndef INCLUDED_BMQIMP_APPLICATION
#define INCLUDED_BMQIMP_APPLICATION

//@PURPOSE: Provide the top level object to manipulate a session with bmqbrkr.
//
//@CLASSES:
//  bmqimp::Application: Top level object to manipulate a session with bmqbrkr
//
//@SEE_ALSO:
//  bmqt::SessionOptions: Options to configure this application
//
//@DESCRIPTION: 'bmqimp::Application' represents the top level object to
// manipulate a session with the bmqbrkr.  It holds the session instance, and
// configures it with the channel to communicate with the broker when this one
// becomes available (or unavailable).
//
/// Thread Safety
///-------------
// Thread safe.

// BMQ

#include <bmqimp_brokersession.h>
#include <bmqimp_eventqueue.h>
#include <bmqimp_negotiatedchannelfactory.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_sessionoptions.h>

#include <bmqio_channel.h>
#include <bmqio_channelfactory.h>
#include <bmqio_ntcchannelfactory.h>
#include <bmqio_reconnectingchannelfactory.h>
#include <bmqio_resolvingchannelfactory.h>
#include <bmqio_statchannelfactory.h>
#include <bmqma_countingallocator.h>
#include <bmqma_countingallocatorstore.h>
#include <bmqst_basictableinfoprovider.h>
#include <bmqst_statcontext.h>
#include <bmqst_table.h>

// BDE
#include <ball_log.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqimp {

// =================
// class Application
// =================

/// Top level object to manipulate a session with bmqbrkr
class Application {
  public:
    // PUBLIC TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

  private:
    // PRIVATE TYPES
    typedef bslma::ManagedPtr<bmqio::ChannelFactory::OpHandle>
        ChannelFactoryOpHandleMp;

    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQIMP.APPLICATION");

    // DATA
    bmqst::StatContext d_allocatorStatContext;
    // Stat context for counting allocators

    bmqma::CountingAllocator d_allocator;
    // Counting allocator

    bmqma::CountingAllocatorStore d_allocators;
    // Allocator store to spawn new
    // allocators for sub-components

    bmqst::StatContext d_rootStatContext;
    // Top level stat context for all stats

    bslma::ManagedPtr<bmqst::StatContext> d_channelsStatContext_mp;
    // Top level stat context for channels

    bmqt::SessionOptions d_sessionOptions;
    // Options to configure this
    // application

    bmqst::Table d_channelsTable;

    bmqst::BasicTableInfoProvider d_channelsTip;

    bdlbb::PooledBlobBufferFactory d_blobBufferFactory;
    // Factory for blob buffers

    /// Pool of shared pointers to blobs.
    BlobSpPool d_blobSpPool;

    bdlmt::EventScheduler d_scheduler;
    // Scheduler

    bmqio::NtcChannelFactory d_channelFactory;

    bmqio::ResolvingChannelFactory d_resolvingChannelFactory;

    bmqio::ReconnectingChannelFactory d_reconnectingChannelFactory;

    bmqio::StatChannelFactory d_statChannelFactory;

    NegotiatedChannelFactory d_negotiatedChannelFactory;

    ChannelFactoryOpHandleMp d_connectHandle_mp;

    BrokerSession d_brokerSession;
    // The 'persistent' broker session
    // state

    bdlmt::EventScheduler::EventHandle d_startTimeoutHandle;
    // Timer Event handle for 'async' start
    // timeout

    bdlmt::EventScheduler::RecurringEventHandle d_statSnaphotTimerHandle;
    // Timer Event handle for statistics
    // snaphot

    int d_nextStatDump;
    // Counter decremented at every stat
    // snapshot, to know when to dump the
    // stats

    bsls::Types::Int64 d_lastAllocatorSnapshot;
    // HiRes timer value of the last time
    // the snapshot was performed on the
    // Counting Allocators context

  private:
    // PRIVATE MANIPULATORS
    void onChannelDown(const bsl::string&   peerUri,
                       const bmqio::Status& status);

    void onChannelWatermark(const bsl::string&                peerUri,
                            bmqio::ChannelWatermarkType::Enum type);

    void readCb(const bmqio::Status&                   status,
                int*                                   numNeeded,
                bdlbb::Blob*                           blob,
                const bsl::shared_ptr<bmqio::Channel>& channel);

    void channelStateCallback(const bsl::string&                     endpoint,
                              bmqio::ChannelFactoryEvent::Enum       event,
                              const bmqio::Status&                   status,
                              const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Create and return the statContext to be used for tracking stats of
    /// the specified `channel` obtained from the specified `handle`.
    bslma::ManagedPtr<bmqst::StatContext> channelStatContextCreator(
        const bsl::shared_ptr<bmqio::Channel>&                  channel,
        const bsl::shared_ptr<bmqio::StatChannelFactoryHandle>& handle);

    /// Method to call after the broker session has been stopped (whether
    /// sync or async), for cleanup of application.
    void brokerSessionStopped(bmqimp::BrokerSession::FsmEvent::Enum event);

    /// Callback method invoked when the asynchronous start operation
    /// timedout.
    void onStartTimeout();

    /// Callback method invoked from the scheduler at a recurring interval
    /// to snapshot the stat contexts.
    void snapshotStats();

    /// Print the stats to the currently installed BALL log observer. If the
    /// specified `isFinal` is true, printed stats do not include the delta
    /// values.
    void printStats(bool isFinal);

    /// To be invoked when the session is being started, in order to setup
    /// the network channel.
    bmqt::GenericResult::Enum startChannel();

    /// Callback passed to the brokerSession to be invoked when the session
    /// makes state transition from the specified `oldState` to the
    /// specified `newState`.
    bmqt::GenericResult::Enum
    stateCb(bmqimp::BrokerSession::State::Enum    oldState,
            bmqimp::BrokerSession::State::Enum    newState,
            bmqimp::BrokerSession::FsmEvent::Enum event);

  private:
    // NOT IMPLEMENTED
    Application(const Application& other) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operation are not permitted on this
    /// object.
    Application& operator=(const Application& other) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Application, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `Application` using the specified `sessionOptions` and
    /// and sending the specified `negotiationMessage` during session
    /// negotiation.  The application will use the specified
    /// `eventHandlerCB`.  Use the specified `allocator` for all memory
    /// allocations.
    Application(const bmqt::SessionOptions&             sessionOptions,
                const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
                const EventQueue::EventHandlerCallback& eventHandlerCB,
                bslma::Allocator*                       allocator);

    /// Destructor
    ~Application();

    // MANIPULATORS

    /// Return a reference offering modifiable access to the brokerSession
    /// holding the state of the connection to the broker.
    BrokerSession& brokerSession();

    // ACCESSORS

    /// Return a reference not offering modifiable access to the
    /// brokerSession holding the state of the connection to the broker.
    const BrokerSession& brokerSession() const;

    /// Return a reference to the sessionOptions passed in at construction
    /// of this `Application`.
    const bmqt::SessionOptions& sessionOptions() const;

    /// Return `true` if the application is started, `false` otherwise.
    bool isStarted() const;

    // MANIPULATORS

    /// Return a pointer to the blob buffer factory used by this instance.
    /// Note that lifetime of the pointed-to buffer factory is bound by this
    /// instance.
    bdlbb::BlobBufferFactory* bufferFactory();

    /// Return a pointer to the blob shared pointer pool used by this instance.
    /// Note that lifetime of the pointed-to pool is bound by this instance.
    BlobSpPool* blobSpPool();

    /// Start the session and the session pool. Return 0 on success or a
    /// non-zero negative code otherwise.  Calling start on an already
    /// started application is a no-op.  This method will block until either
    /// the session is connected to the broker, or the specified `timeout`
    /// has expired.
    int start(const bsls::TimeInterval& timeout);

    /// Asynchronously start the session. Return 0 if the start operation
    /// has been successfully initiated, and non-zero negative code
    /// otherwise.  This method will return immediately, and a
    /// `SESSION_CONNECTED` event will be enqueued once the session with the
    /// broker has been established; or a `CONNECTION_TIMEOUT` event will be
    /// enqueued if the session failed to establish within the specified
    /// `timeout` interval.
    int startAsync(const bsls::TimeInterval& timeout);

    /// Gracefully stop the connection, and wait until everything
    /// successfully completes.  Calling stop on an already stopped
    /// application is a no-op.  It is undefined behavior to call stop() on
    /// a session that is not using the EventHandler, stopAsync() should be
    /// used instead.
    void stop();

    /// Asynchronously and gracefully stop the connection.
    void stopAsync();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class Application
// -----------------

inline BrokerSession& Application::brokerSession()
{
    return d_brokerSession;
}

inline const BrokerSession& Application::brokerSession() const
{
    return d_brokerSession;
}

inline const bmqt::SessionOptions& Application::sessionOptions() const
{
    return d_sessionOptions;
}

inline bool Application::isStarted() const
{
    bmqimp::BrokerSession::State::Enum state = d_brokerSession.state();
    // The upper layer API considers both STARTED and RECONNECTING states as
    // the session is started
    return (state == bmqimp::BrokerSession::State::e_STARTED ||
            state == bmqimp::BrokerSession::State::e_RECONNECTING);
}

inline bdlbb::BlobBufferFactory* Application::bufferFactory()
{
    return &d_blobBufferFactory;
}

inline Application::BlobSpPool* Application::blobSpPool()
{
    return &d_blobSpPool;
}

}  // close package namespace
}  // close enterprise namespace

#endif
