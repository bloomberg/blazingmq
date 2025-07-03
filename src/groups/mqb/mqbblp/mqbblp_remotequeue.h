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

// mqbblp_remotequeue.h                                               -*-C++-*-
#ifndef INCLUDED_MQBBLP_REMOTEQUEUE
#define INCLUDED_MQBBLP_REMOTEQUEUE

/// @file mqbblp_remotequeue.h
///
/// @brief Provide a queue implementation for a remotely managed queue.
///
/// @todo Document component.
///
/// Thread Safety                                  {#mqbblp_remotequeue_thread}
/// =============
///

// MQB
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_queuestate.h>
#include <mqbblp_relayqueueengine.h>
#include <mqbcfg_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbs_virtualstoragecatalog.h>

// BMQ
#include <bmqc_orderedhashmap.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>
#include <bmqu_atomicstate.h>
#include <bmqu_sharedresource.h>

// BDE
#include <ball_log.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_throttle.h>
#include <bdlt_timeunitratio.h>
#include <bsl_functional.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class RemoteQueue;
}
namespace mqbi {
class Domain;
}
namespace mqbi {
class QueueEngine;
}

namespace mqbblp {

// =================
// class RemoteQueue
// =================

/// @todo Document class.
class RemoteQueue {
  public:
    // PUBLIC TYPES
    typedef bsl::function<void(const bmqp::PutHeader&              header,
                               const bsl::shared_ptr<bdlbb::Blob>& appData,
                               const bsl::shared_ptr<bdlbb::Blob>& options,
                               mqbi::QueueHandle*                  source)>
        PutsVisitor;

    typedef bsl::function<void(const bmqt::MessageGUID& msgGUID,
                               unsigned int             subQueueId,
                               mqbi::QueueHandle*       source)>
        ConfirmsVisitor;

    typedef bdlcc::SharedObjectPool<
        bmqu::AtomicState,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::Reset<bmqu::AtomicState> >
        StateSpPool;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.REMOTEQUEUE");

  private:
    // PRIVATE TYPES
    struct PutMessage {
        mqbi::QueueHandle* const     d_handle;
        const bmqp::PutHeader        d_header;
        bsl::shared_ptr<bdlbb::Blob> d_appData;
        bsl::shared_ptr<bdlbb::Blob> d_options;
        /// Insertion time.  Do not retransmit past `deduplication` timeout.
        const bsls::Types::Int64           d_timeReceived;
        bsl::shared_ptr<bmqu::AtomicState> d_state_sp;

        PutMessage(mqbi::QueueHandle*                  handle,
                   const bmqp::PutHeader&              header,
                   const bsl::shared_ptr<bdlbb::Blob>& appData,
                   const bsl::shared_ptr<bdlbb::Blob>& options,
                   bsls::Types::Int64                  time,
                   bsl::shared_ptr<bmqu::AtomicState>& state);

        ~PutMessage();
    };

    struct ConfirmMessage {
        const bmqt::MessageGUID  d_guid;
        const unsigned int       d_upstreamSubQueueId;
        mqbi::QueueHandle* const d_handle;

        ConfirmMessage(const bmqt::MessageGUID& guid,
                       unsigned int             upstreamSubQueueId,
                       mqbi::QueueHandle*       handle);
    };

    struct SubStreamContext {
        enum Enum {
            /// Unknown.
            e_NONE = 0,
            /// Drop.
            e_CLOSED = 1,
            /// Buffer PUTs and CONFIRMs.
            e_STOPPED = 2,
            /// Send.
            e_OPENED = 3,
        };
        /// Last seen genCount
        bsls::Types::Uint64 d_genCount;

        /// Traffic control
        Enum d_state;

        SubStreamContext();

        static const char* toAscii(Enum value);
    };

    /// Must be a container in which iteration order is same as insertion
    /// order.
    typedef bmqc::OrderedHashMap<bmqt::MessageGUID,
                                 PutMessage,
                                 bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        Puts;

    /// Must be a container in which iteration order is same as insertion
    /// order.
    typedef bsl::list<ConfirmMessage> Confirms;

    /// There are 3 possible states for a subStream:
    ///  1.  Opened and with an available upstream.  Send.
    ///  2.  Opened and without an available upstream.  Buffer.
    ///  3.  Not opened.  Drop.
    typedef bmqc::Array<SubStreamContext,
                        bmqp::Protocol::k_SUBID_ARRAY_STATIC_LEN>
        SubQueueIds;

    typedef bslma::ManagedPtr<mqbs::VirtualStorageCatalog> SubStreamMessagesMp;

    typedef mqbi::Storage::StorageKeys StorageKeys;

    typedef QueueHandleCatalog::DownstreamKey DownstreamKey;

  private:
    // DATA
    bmqu::SharedResource<RemoteQueue> d_self;

    QueueState* d_state_p;

    bslma::ManagedPtr<RelayQueueEngine> d_queueEngine_mp;

    /// Map of GUID->Item, to use for ACKs/NACKs and retransmissions.
    Puts d_pendingMessages;

    Confirms d_pendingConfirms;

    /// List of messages that need to be delivered to sub-streams, as indicated
    /// by the upstream node.
    SubStreamMessagesMp d_subStreamMessages_mp;

    /// Used to parse options in a message received from upstream.
    bmqp::OptionsView d_optionsView;

    /// ThrottledAction parameters for failed puts
    bdlmt::Throttle d_throttledFailedPutMessages;

    /// ThrottledAction parameters for failed pushes
    bdlmt::Throttle d_throttledFailedPushMessages;

    /// ThrottledAction parameters for failed acks
    bdlmt::Throttle d_throttledFailedAckMessages;

    /// ThrottledAction parameters for failed confirms
    bdlmt::Throttle d_throttledFailedConfirmMessages;

    /// Configured timeout in ns upon which retransmission attempts stop. Must
    /// be Less or equal to the deduplication timeout.
    bsls::Types::Int64 d_pendingPutsTimeoutNs;

    /// Broadcast PUT can be retransmitted if it is guaranteed that the PUT did
    /// not make it to the primary.  That condition is indicated by
    /// `e_NOT_READY` NACK. To preserve order when retransmitting, broadcast
    /// PUTs are cached in d_pendingMessages (GUIDs only).  Since there is no
    /// e_SUCCESS ACK in the broadcast mode, erasing pending PUTs is done in
    /// batches.  For every `d_ackWindowSize` broadcast, artificially set
    /// `e_ACK_REQUESTED` flag and once (N)ACK is received, erase the PUT and
    /// all prior PUTs.
    bdlmt::EventScheduler::EventHandle d_pendingMessagesTimerEventHandle;

    /// Request ACK after `d_ackWindowSize` unacked broadcast PUTs.
    int d_ackWindowSize;

    /// Counter of unacked broadcast PUTs in the current window.
    int d_unackedPutCounter;

    /// SubStream states.
    SubQueueIds d_subStreams;

    StateSpPool* d_statePool_p;

    /// To discern consumer and producer which share the same
    /// `k_DEFAULT_SUBQUEUE_ID` in the priority mode.
    SubStreamContext d_producerState;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE MANIPULATORS
    int configureAsProxy(bsl::ostream& errorDescription, bool isReconfigure);

    int configureAsClusterMember(bsl::ostream& errorDescription,
                                 bool          isReconfigure);

    /// Load into the specified `subQueueInfos` all subQueueIds which are
    /// present in the `SubQueueIdsOption` of the specified `options` of the
    /// message with the specified `msgGUID`.  Return true on success, and
    /// false otherwise.  Note that if no subQueueIds can be loaded, this
    /// method returns false.
    bool loadSubQueueInfos(bmqp::Protocol::SubQueueInfosArray* subQueueInfos,
                           const bdlbb::Blob&                  options,
                           const bmqt::MessageGUID&            msgGUID);

    /// Give up on the message pointed at by the specified iterator `it`.
    /// Cancel the associated state so if the message is waiting low WM, the
    /// mqbnet::Channel will discard it.  Return the result of erasing `it`
    /// from the collection of pending messages.
    Puts::iterator erasePendingMessage(Puts::iterator& it);

    /// Give up on all pending messages submitted before the messages at the
    /// specified iterator `it`.  Cancel the associated state of
    /// each messages so if the message is waiting low WM, the
    /// mqbnet::Channel will discard it.  Return number of erased messages.
    size_t erasePendingMessages(const Puts::iterator& it);

    void expirePendingMessages();

    /// Iterate all pending (unACKed) messages.  NACK and erase all expired
    /// messages (past `d_pendingPutsTimeoutNs`).  For non-broadcast queues
    /// only.
    void expirePendingMessagesDispatched();

    /// Retransmit all pending (unACKed) messages.
    void retransmitPendingMessagesDispatched(bsls::Types::Uint64 genCount);

    /// Retransmit all pending CONFIRMS.
    void retransmitPendingConfirmsDispatched(unsigned int upstreamSubQueueId);

    void            cleanPendingMessages(mqbi::QueueHandle* handle);
    Puts::iterator& nack(Puts::iterator& it, bmqp::AckMessage& ackMessage);

    void sendPutMessage(const bmqp::PutHeader&                    putHeader,
                        const bsl::shared_ptr<bdlbb::Blob>&       appData,
                        const bsl::shared_ptr<bdlbb::Blob>&       options,
                        const bsl::shared_ptr<bmqu::AtomicState>& state,
                        bsls::Types::Uint64                       genCount);

    void sendConfirmMessage(const bmqt::MessageGUID& msgGUID,
                            unsigned int             upstreamSubQueueId,
                            mqbi::QueueHandle*       source);

    /// Called when a message with the specified `msgGUID`, `appData`,
    /// `options`, `compressionAlgorithmType` is pushed to this queue.
    void
    pushMessage(const bmqt::MessageGUID&             msgGUID,
                const bsl::shared_ptr<bdlbb::Blob>&  appData,
                const bsl::shared_ptr<bdlbb::Blob>&  options,
                const bmqp::MessagePropertiesInfo&   messagePropertiesInfo,
                bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
                bool                                 isOutOfOrder);

    SubStreamContext& subStreamContext(unsigned int upstreamSubQueueId);

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    RemoteQueue(const RemoteQueue&);             // = delete;
    RemoteQueue& operator=(const RemoteQueue&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RemoteQueue, bslma::UsesBslmaAllocator)

    // CREATORS
    RemoteQueue(QueueState*       state,
                int               deduplicationTimeMs,
                int               ackWindowSize,
                StateSpPool*      statePool,
                bslma::Allocator* allocator);

    ~RemoteQueue();

    // MANIPULATORS

    /// Configure this queue instance, with `isReconfigure` flag indicating
    /// queue is being reconfigured, and populate `errorDescription` with a
    /// human readable string in case of an error.  Return zero on success
    /// and non-zero value otherwise.  Behavior is undefined unless this
    /// function is invoked from queue-dispatcher thread.
    int configure(bsl::ostream& errorDescription, bool isReconfigure);

    /// Return the queue engine used by this queue.
    mqbi::QueueEngine* queueEngine();

    /// Reset the state of this object.
    void resetState();

    void close();

    /// Obtain a handle to this queue, for the client represented by the
    /// specified `clientContext` and using the specified `handleParameters`
    /// and `upstreamSubQueueId`.  Invoke the specified `callback` with the
    /// result.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void getHandle(const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                              clientContext,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   unsigned int upstreamSubQueueId,
                   const mqbi::QueueHandle::GetHandleCallback& callback);

    /// Configure the specified `handle` with the specified
    /// `streamParameters`, and invoke the specified `configuredCb` callback
    /// when finished.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void configureHandle(
        mqbi::QueueHandle*                                 handle,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb);

    void
    releaseHandle(mqbi::QueueHandle*                         handle,
                  const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                  bool                                       isFinal,
                  const mqbi::QueueHandle::HandleReleasedCallback& releasedCb);

    void onHandleReleased(
        const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
        const mqbi::QueueHandle::HandleReleasedCallback& releasedCb,
        const bsl::shared_ptr<mqbi::QueueHandle>&        handle,
        const mqbi::QueueHandleReleaseResult&            result);

    void onDispatcherEvent(const mqbi::DispatcherEvent& event);

    void flush();

    void postMessage(const bmqp::PutHeader&              putHeader,
                     const bsl::shared_ptr<bdlbb::Blob>& appData,
                     const bsl::shared_ptr<bdlbb::Blob>& options,
                     mqbi::QueueHandle*                  source);

    /// Confirm the message with the specified `msgGUID` for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `source`.  Also note that since there
    /// are no `ack of confirms`, this method is void, and will eventually
    /// throttle warnings if the `msgGUID` is invalid.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void confirmMessage(const bmqt::MessageGUID& msgGUID,
                        unsigned int             upstreamSubQueueId,
                        mqbi::QueueHandle*       source);

    /// Reject the message with the specified `msgGUID` for the specified
    /// `upstreamSubQueueId` stream of the queue on the specified `source`.
    ///  Return resulting RDA counter.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    int rejectMessage(const bmqt::MessageGUID& msgGUID,
                      unsigned int             upstreamSubQueueId,
                      mqbi::QueueHandle*       source);

    /// Invoke the specified `visitor` for every pending PUT which has not
    /// expired yet.  When in `broadcast` mode, exclude already broadcasted
    /// pending PUTs unless they have received `e_NOT_READY` NACK.  Return
    /// total number of pending items.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    size_t iteratePendingMessages(const PutsVisitor& visitor);

    /// Invoke the specified `visitor` for every pending CONFIRMs.  Return
    /// total number of visited items.  Note that the `visitor` receives
    /// downstream subQueueId as an argument.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    size_t iteratePendingConfirms(const ConfirmsVisitor& visitor);

    void onAckMessageDispatched(const mqbi::DispatcherAckEvent& event);

    /// Notify the queue that the upstream is not ready to receive
    /// PUTs/CONFIRMs.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onLostUpstream();

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
                        bool                isWriterOnly = false);

    /// Notify the (remote) queue about reopen failure.  The queue NACKs all
    /// pending and incoming PUTs and drops CONFIRMs related to to the
    /// specified `upstreamSubQueueId`.  The queue can transition out of
    /// this state onReopenUpstream' with non-zero `genCount`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onOpenFailure(unsigned int upstreamSubQueueId);

    /// Return the event scheduler associated with this remote queue.
    bdlmt::EventScheduler* scheduler();

    // ACCESSORS

    /// Load into the specified `out` object the internal details about this
    /// queue.
    void                     loadInternals(mqbcmd::RemoteQueue* out) const;
    const bmqt::MessageGUID& resumePoint() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------------------
// class RemoteQueue::SubStreamContext
// -----------------------------------
inline RemoteQueue::SubStreamContext::SubStreamContext()
: d_genCount(0)
, d_state(e_NONE)
{
    // NOTHING
}

inline const char* RemoteQueue::SubStreamContext::toAscii(
    RemoteQueue::SubStreamContext::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NONE)
        CASE(CLOSED)
        CASE(STOPPED)
        CASE(OPENED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}
// -----------------
// class RemoteQueue
// -----------------

inline RemoteQueue::PutMessage::PutMessage(
    mqbi::QueueHandle*                  handle,
    const bmqp::PutHeader&              header,
    const bsl::shared_ptr<bdlbb::Blob>& appData,
    const bsl::shared_ptr<bdlbb::Blob>& options,
    bsls::Types::Int64                  time,
    bsl::shared_ptr<bmqu::AtomicState>& state)
: d_handle(handle)
, d_header(header)
, d_appData(appData)
, d_options(options)
, d_timeReceived(time)
, d_state_sp(state)
{
    // NOTHING
}

inline RemoteQueue::PutMessage::~PutMessage()
{
    // NOTHING
}

inline RemoteQueue::ConfirmMessage::ConfirmMessage(
    const bmqt::MessageGUID& guid,
    unsigned int             upstreamSubQueueId,
    mqbi::QueueHandle*       handle)
: d_guid(guid)
, d_upstreamSubQueueId(upstreamSubQueueId)
, d_handle(handle)
{
}

inline mqbi::QueueEngine* RemoteQueue::queueEngine()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    return d_queueEngine_mp.get();
}

inline RemoteQueue::SubStreamContext&
RemoteQueue::subStreamContext(unsigned int upstreamSubQueueId)
{
    if (upstreamSubQueueId >= d_subStreams.size()) {
        d_subStreams.resize(upstreamSubQueueId + 1);
    }

    return d_subStreams[upstreamSubQueueId];
}

inline bdlmt::EventScheduler* RemoteQueue::scheduler()
{
    return d_state_p->scheduler();
}

}  // close package namespace
}  // close enterprise namespace

#endif
