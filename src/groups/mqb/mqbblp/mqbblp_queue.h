// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbblp_queue.h                                                     -*-C++-*-
#ifndef INCLUDED_MQBBLP_QUEUE
#define INCLUDED_MQBBLP_QUEUE

/// @file mqbblp_queue.h
///
/// @brief Provide a queue implementation for a queue managed by this broker.
///
/// @todo Document this component: "pseudo-strategy pattern" (?)

// MQB
#include <mqbblp_localqueue.h>
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_queuestate.h>
#include <mqbblp_remotequeue.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_queueutil.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace bdlmt {
class FixedThreadPool;
}
namespace mqbcmd {
class QueueCommand;
}
namespace mqbcmd {
class QueueInternals;
}
namespace mqbcmd {
class QueueResult;
}
namespace mqbcmd {
class PurgeQueueResult;
}
namespace mqbi {
class Domain;
}
namespace mqbi {
class QueueEngine;
}
namespace mqbi {
class Storage;
}
namespace mqbi {
class StorageManager;
}
namespace mqbstat {
class QueueStatsDomain;
}

namespace mqbblp {

// ===========
// class Queue
// ===========

/// @todo Document this class
class Queue BSLS_CPP11_FINAL : public mqbi::Queue {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.QUEUE");

  private:
    // DATA
    bslma::Allocator*              d_allocator_p;
    mutable bmqp::SchemaLearner    d_schemaLearner;  // must precede d_state
    QueueState                     d_state;
    bslma::ManagedPtr<LocalQueue>  d_localQueue_mp;
    bslma::ManagedPtr<RemoteQueue> d_remoteQueue_mp;

  private:
    // NOT IMPLEMENTED
    Queue(const Queue& other) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented
    Queue& operator=(const Queue& other) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS
    void configureDispatched(
        int*                                     result,
        bsl::ostream*                            errorDescription,
        const bsl::shared_ptr<mqbconfm::Domain>& domainConfig_sp,
        bool                                     isReconfigure);

    void getHandleDispatched(
        const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                    clientContext,
        const bmqp_ctrlmsg::QueueHandleParameters&  handleParameters,
        unsigned int                                upstreamSubQueueId,
        const mqbi::QueueHandle::GetHandleCallback& callback);

    void releaseHandleDispatched(
        mqbi::QueueHandle*                               handle,
        const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
        bool                                             isFinal,
        const mqbi::QueueHandle::HandleReleasedCallback& releasedCb);

    void dropHandleDispatched(mqbi::QueueHandle* handle, bool doDeconfigure);

    void closeDispatched();

    void convertToLocalDispatched();

    void updateStats();

    void listMessagesDispatched(mqbcmd::QueueResult* result,
                                const bsl::string&   appId,
                                bsls::Types::Int64   offset,
                                bsls::Types::Int64   count);

    /// Load into the specified `out` object the internal details about this
    /// queue.
    void loadInternals(mqbcmd::QueueInternals* out);

  public:
    // CREATORS
    Queue(const bmqt::Uri&                          uri,
          unsigned int                              id,
          const mqbu::StorageKey&                   key,
          int                                       partitionId,
          mqbi::Domain*                             domain,
          mqbi::StorageManager*                     storageManager,
          const mqbi::ClusterResources&             resources,
          bdlmt::FixedThreadPool*                   threadPool,
          const bmqp_ctrlmsg::RoutingConfiguration& routingCfg,
          bslma::Allocator*                         allocator);

    /// Destructor
    ~Queue() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    void createLocal();
    void createRemote(int                       deduplicationTimeoutMs,
                      int                       ackWindowSize,
                      RemoteQueue::StateSpPool* statePool);

    void convertToLocal() BSLS_KEYWORD_OVERRIDE;
    void convertToRemote();

    // ACCESSORS
    bool isLocal() const;
    bool isRemote() const;

    // MANIPULATORS
    //   (virtual mqbi::Queue)

    /// Configure this queue instance, where `isReconfigure` dictates
    /// whether this is a reconfiguration of an existing queue.
    ///
    /// If `wait` is `true`, this method will not return until the operation
    /// has completed. The return value will be 0 if it succeeds, and
    /// nonzero if there was an error; in case of an error, the specified
    /// `errorDescription_p` stream will be populated with a human readable
    /// reason.
    ///
    /// If `wait` is `false`, this method will return 0 after scheduling the
    /// operation on an unspecified thread, and `errorDescription_p` will be
    /// unmodified.
    ///
    /// THREAD: this method can be invoked only from cluster-dispatcher
    /// thread.
    int configure(bsl::ostream*                            errorDescription_p,
                  const bsl::shared_ptr<mqbconfm::Domain>& domainConfig_sp,
                  bool                                     isReconfigure,
                  bool wait) BSLS_KEYWORD_OVERRIDE;

    /// Obtain a handle to this queue, for the client represented by the
    /// specified `clientContext` and using the specified `handleParameters`
    /// and `upstreamSubQueueId`.  Invoke the specified `callback` with the
    /// result.
    void getHandle(const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                              clientContext,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   unsigned int upstreamSubQueueId,
                   const mqbi::QueueHandle::GetHandleCallback& callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Configure the specified `handle` with the specified
    /// `streamParameters`, and invoke the specified `configuredCb` callback
    /// when finished.
    ///
    /// THREAD: this method is called from the queue's dispatcher thread.
    void configureHandle(
        mqbi::QueueHandle*                                 handle,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Release the specified `handle`.
    ///
    /// THREAD: this method can be called from any thread.
    void
    releaseHandle(mqbi::QueueHandle*                         handle,
                  const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                  bool                                       isFinal,
                  const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
        BSLS_KEYWORD_OVERRIDE;

    void dropHandle(mqbi::QueueHandle* handle,
                    bool doDeconfigure = true) BSLS_KEYWORD_OVERRIDE;

    /// Close this queue.
    ///
    /// THREAD: this method can be called from any thread.
    void close() BSLS_KEYWORD_OVERRIDE;

    /// Return the resource capacity meter associated to this queue, if
    /// any, or a null pointer otherwise.
    mqbu::CapacityMeter* capacityMeter() BSLS_KEYWORD_OVERRIDE;

    /// Return the queue engine used by this queue.
    mqbi::QueueEngine* queueEngine() BSLS_KEYWORD_OVERRIDE;

    /// Set the stats associated with this queue.
    void setStats(const bsl::shared_ptr<mqbstat::QueueStatsDomain>& stats)
        BSLS_KEYWORD_OVERRIDE;

    /// Return number of unconfirmed messages across all handles with the
    /// `specified `subId'.
    bsls::Types::Int64
    countUnconfirmed(unsigned int subId) BSLS_KEYWORD_OVERRIDE;

    /// Stop sending PUSHes but continue receiving CONFIRMs, receiving and
    /// sending PUTs and ACKs.
    void stopPushing() BSLS_KEYWORD_OVERRIDE;

    void onPushMessage(
        const bmqt::MessageGUID&             msgGUID,
        const bsl::shared_ptr<bdlbb::Blob>&  appData,
        const bsl::shared_ptr<bdlbb::Blob>&  options,
        const bmqp::MessagePropertiesInfo&   messagePropertiesInfo,
        bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
        bool isOutOfOrder) BSLS_KEYWORD_OVERRIDE;

    /// Confirm the message with the specified `msgGUID` for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `source`.  Also note that since there
    /// are no `ack of confirms`, this method is void, and will eventually
    /// throttle warnings if the `msgGUID` is invalid.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void confirmMessage(const bmqt::MessageGUID& msgGUID,
                        unsigned int             upstreamSubQueueId,
                        mqbi::QueueHandle*       source) BSLS_KEYWORD_OVERRIDE;

    /// Reject the message with the specified `msgGUID` for the specified
    /// `upstreamSubQueueId` stream of the queue on the specified `source`.
    ///  Return resulting RDA counter.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    int rejectMessage(const bmqt::MessageGUID& msgGUID,
                      unsigned int             upstreamSubQueueId,
                      mqbi::QueueHandle*       source) BSLS_KEYWORD_OVERRIDE;

    /// Invoked by the cluster whenever it receives an ACK from upstream.
    void
    onAckMessage(const bmqp::AckMessage& ackMessage) BSLS_KEYWORD_OVERRIDE;

    /// Notify the queue that the upstream is not ready to receive
    /// PUTs/CONFIRMs.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onLostUpstream() BSLS_KEYWORD_OVERRIDE;

    /// Notify the queue that the upstream is ready to receive PUTs/CONFIRMs
    /// for the specified `upstreamSubQueueId`.  PUT messages carry the
    /// specified `genCount`; if there is a mismatch between PUT `genCount`
    /// and current upstream `genCount`, then the PUT message gets dropped
    /// to avoid out of order PUTs.  If the `upstreamSubQueueId` is
    /// `k_ANY_SUBQUEUE_ID`, all SubQueues are reopen.
    /// If the optionally specified isWriterOnly is true, ignore CONFIRMs. This
    /// should be specified if the upstream is stopping.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onOpenUpstream(bsls::Types::Uint64 genCount,
                        unsigned int        upstreamSubQueueId,
                        bool isWriterOnly = false) BSLS_KEYWORD_OVERRIDE;

    /// Notify the (remote) queue about reopen failure.  The queue NACKs all
    /// pending and incoming PUTs and drops CONFIRMs related to to the
    /// specified `upstreamSubQueueId`.  The queue can transition out of
    /// this state onReopenUpstream' with non-zero `genCount`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onOpenFailure(unsigned int upstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Invoked by the Data Store when it receives quorum Receipts for the
    /// specified `msgGUID`.  Send ACK to the specified `queueHandle` if it
    /// is present in the queue handle catalog.  Update AVK time stats using
    /// the specified `arrivalTimepoint`.
    ///
    /// THREAD: This method is called from the Storage dispatcher thread.
    void onReceipt(const bmqt::MessageGUID&  msgGUID,
                   mqbi::QueueHandle*        queueHandle,
                   const bsls::Types::Int64& arrivalTimepoint)
        BSLS_KEYWORD_OVERRIDE;

    /// Invoked by the Data Store when it removes (times out waiting for
    /// quorum Receipts for) a message with the specified `msgGUID`.  Send
    /// NACK with the specified `result` to the specified `gH` if it is
    /// present in the queue handle catalog.
    ///
    /// THREAD: This method is called from the Storage dispatcher thread.
    void onRemoval(const bmqt::MessageGUID& msgGUID,
                   mqbi::QueueHandle*       qH,
                   bmqt::AckResult::Enum    result) BSLS_KEYWORD_OVERRIDE;

    /// Invoked when data store has transmitted all cached replication
    /// records.  Switch from PUT processing to PUSH delivery.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onReplicatedBatch() BSLS_KEYWORD_OVERRIDE;

    /// Synchronously process the specified `command` and load the outcome
    /// in the specified `result` object.  Return zero on success or a
    /// nonzero value otherwise.
    virtual int
    processCommand(mqbcmd::QueueResult*        result,
                   const mqbcmd::QueueCommand& command) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual mqbi::Queue)

    /// Return the domain this queue belong to.
    mqbi::Domain* domain() const BSLS_KEYWORD_OVERRIDE;

    /// Return the storage used by this queue.
    mqbi::Storage* storage() const BSLS_KEYWORD_OVERRIDE;

    /// Return the stats associated with this queue.
    const bsl::shared_ptr<mqbstat::QueueStatsDomain>&
    stats() const BSLS_KEYWORD_OVERRIDE;

    /// Return the partitionId assigned to this queue.
    int partitionId() const BSLS_KEYWORD_OVERRIDE;

    /// Return the URI of this queue.
    const bmqt::Uri& uri() const BSLS_KEYWORD_OVERRIDE;

    /// Return the unique ID of this queue, as advertised to upstream
    /// connection upon queue opening.
    unsigned int id() const BSLS_KEYWORD_OVERRIDE;

    /// Returns `true` if the configuration for this queue requires
    /// at-most-once semantics or `false` otherwise.
    bool isAtMostOnce() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if the configuration for this queue requires
    /// delivering to all consumers, and `false` otherwise.
    bool isDeliverAll() const BSLS_KEYWORD_OVERRIDE;

    /// Returns `true` if the configuration for this queue requires
    /// has-multiple-sub-streams semantics or `false` otherwise.
    bool hasMultipleSubStreams() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference not offering modifiable access to the aggregated
    /// parameters of all currently opened queueHandles on this queue.
    const bmqp_ctrlmsg::QueueHandleParameters&
    handleParameters() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the queue has upstream parameters for the specified
    /// `upstreamSubQueueId` in which case load the parameters into the
    /// specified `value`.  Return false otherwise.
    bool getUpstreamParameters(bmqp_ctrlmsg::StreamParameters* value,
                               unsigned int upstreamSubQueueId) const
        BSLS_KEYWORD_OVERRIDE;

    /// Return the message throttle config associated with this queue.
    const mqbcfg::MessageThrottleConfig&
    messageThrottleConfig() const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the dispatcherClientData.
    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE;

    /// Called by the `Dispatcher` when it has the specified `event` to
    /// deliver to the client.
    void onDispatcherEvent(const mqbi::DispatcherEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Called by the dispatcher to flush any pending operation.. mainly
    /// used to provide batch and nagling mechanism.
    void flush() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;

    /// Return a printable description of the client (e.g. for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;

    /// Return the Schema Leaner associated with this queue.
    bmqp::SchemaLearner& schemaLearner() const BSLS_KEYWORD_OVERRIDE;

    /// @return the configuration of this domain, or an empty pointer if the
    ///         domain is not configured.
    /// THREAD: safe to access only from CLUSTER dispatcher thread.
    const bsl::shared_ptr<mqbconfm::Domain>&
    domainConfig() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------
// class Queue
// -----------

inline bool Queue::isLocal() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!(d_localQueue_mp && d_remoteQueue_mp));
    // Can only be either local, remote, or not initialized but not both.

    return d_localQueue_mp;
}

inline bool Queue::isRemote() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!(d_localQueue_mp && d_remoteQueue_mp));
    // Can only be either local, remote, or not initialized but not both.

    return d_remoteQueue_mp;
}

inline mqbu::CapacityMeter* Queue::capacityMeter()
{
    return d_state.storage()->capacityMeter();
}

inline mqbi::QueueEngine* Queue::queueEngine()
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_localQueue_mp) {
        return d_localQueue_mp->queueEngine();  // RETURN
    }
    else if (d_remoteQueue_mp) {
        return d_remoteQueue_mp->queueEngine();  // RETURN
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
        return 0;  // RETURN
    }
}

inline const bsl::shared_ptr<mqbstat::QueueStatsDomain>& Queue::stats() const
{
    return d_state.stats();
}

inline void
Queue::setStats(const bsl::shared_ptr<mqbstat::QueueStatsDomain>& stats)
{
    d_state.setStats(stats);
}

inline mqbi::Domain* Queue::domain() const
{
    return d_state.domain();
}

inline mqbi::Storage* Queue::storage() const
{
    return d_state.storage();
}

inline int Queue::partitionId() const
{
    return d_state.partitionId();
}

inline const bmqt::Uri& Queue::uri() const
{
    return d_state.uri();
}

inline unsigned int Queue::id() const
{
    return d_state.id();
}

inline bool Queue::isAtMostOnce() const
{
    return d_state.isAtMostOnce();
}

inline bool Queue::isDeliverAll() const
{
    return d_state.isDeliverAll();
}

inline bool Queue::hasMultipleSubStreams() const
{
    return d_state.hasMultipleSubStreams();
}

inline const bmqp_ctrlmsg::QueueHandleParameters&
Queue::handleParameters() const
{
    return d_state.handleParameters();
}

inline bool Queue::getUpstreamParameters(bmqp_ctrlmsg::StreamParameters* value,
                                         unsigned int upstreamSubQueueId) const
{
    return d_state.getUpstreamParameters(value, upstreamSubQueueId);
}

inline const mqbcfg::MessageThrottleConfig&
Queue::messageThrottleConfig() const
{
    return d_state.messageThrottleConfig();
}

inline const bsl::string& Queue::description() const
{
    return d_state.description();
}

inline mqbi::DispatcherClientData& Queue::dispatcherClientData()
{
    return d_state.dispatcherClientData();
}

inline const mqbi::DispatcherClientData& Queue::dispatcherClientData() const
{
    return d_state.dispatcherClientData();
}

inline mqbi::Dispatcher* Queue::dispatcher()
{
    return d_state.dispatcherClientData().dispatcher();
}

inline const mqbi::Dispatcher* Queue::dispatcher() const
{
    return d_state.dispatcherClientData().dispatcher();
}

inline bmqp::SchemaLearner& Queue::schemaLearner() const
{
    return d_schemaLearner;
}

inline const bsl::shared_ptr<mqbconfm::Domain>& Queue::domainConfig() const
{
    return d_state.domainConfig();
}

}  // close package namespace
}  // close enterprise namespace

#endif
