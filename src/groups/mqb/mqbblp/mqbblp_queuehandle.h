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

// mqbblp_queuehandle.h                                               -*-C++-*-
#ifndef INCLUDED_MQBBLP_QUEUEHANDLE
#define INCLUDED_MQBBLP_QUEUEHANDLE

//@PURPOSE:
//
//@CLASSES:
//
//
//@DESCRIPTION:

// MQB

#include <mqbconfm_messages.h>
#include <mqbi_queue.h>
#include <mqbstat_queuestats.h>
#include <mqbu_resourceusagemonitor.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_messageguid.h>

#include <bmqu_operationchain.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_throttle.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_performancehint.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class QueueHandle;
}
namespace mqbi {
class DispatcherClient;
}

namespace mqbblp {

// =================
// class QueueHandle
// =================

class QueueHandle : public mqbi::QueueHandle {
    // TBD

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.QUEUEHANDLE");

  private:
    // TYPES

    /// VST representing downstream substream with all redelivery data.
    /// (Re)created upon `registerSubStream`
    /// Reset upon `unregisterSubStream`
    struct Downstream {
        const bsl::string               d_appId;
        unsigned int                    d_upstreamSubQueueId;
        mqbi::QueueHandle::RedeliverySp d_data;

        Downstream(const bsl::string& appId,
                   unsigned int       upstreamSubQueueId,
                   bslma::Allocator*  allocator_p);
    };

    /// Struct holding information associated to a substream of the queue
    struct Subscription {
        // PUBLIC DATA
        mqbu::ResourceUsageMonitor d_unconfirmedMonitor;
        // Accumulated bytes and messages of
        // all outstanding delivered but
        // unconfirmed messages.  Should only
        // be manipulated from the
        // *QUEUE-DISPATCHER* thread.

        bsl::shared_ptr<Downstream> d_downstream;

        unsigned int d_downstreamSubQueueId;
        unsigned int d_upstreamId;

      private:
        // NOT IMPLEMENTED

        /// Copy constructor and assignment operator are not implemented.
        Subscription(const Subscription& original,
                     bslma::Allocator*   allocator = 0);       // = delete
        Subscription& operator=(const Subscription& other);  // = delete

      public:
        // CREATORS

        /// Create a `SubStreamContext` object.  All memory allocations will
        /// be done using the specified `allocator`.
        explicit Subscription(unsigned int downstreamSubQueueId,
                              const bsl::shared_ptr<Downstream>& downstream,
                              unsigned int                       upstreamId);

        const bsl::string& appId() const;

        static unsigned int nextStamp();
    };
    typedef bsl::shared_ptr<Subscription> SubscriptionSp;
    typedef bmqc::Array<bsl::shared_ptr<Downstream>,
                        bmqp::Protocol::k_SUBID_ARRAY_STATIC_LEN>
        Downstreams;

    /// The purpose is to avoid memory allocation by bdlf::BindUtil::bind
    /// when dispatching CONFIRM from Cluster to Queue.
    class ConfirmFunctor : public mqbi::CallbackFunctor {
      private:
        // PRIVATE DATA
        QueueHandle*      d_owner_p;
        bmqt::MessageGUID d_guid;
        unsigned int      d_downstreamSubQueueId;

      public:
        // CREATORS
        ConfirmFunctor(QueueHandle*      owner_p,
                       bmqt::MessageGUID guid,
                       unsigned int      downstreamSubQueueId);

        void operator()() const BSLS_KEYWORD_OVERRIDE;
    };

  public:
    // PUBLIC TYPES

    /// subQueueId -> subStreamContext
    typedef bsl::unordered_map<unsigned int, SubscriptionSp> Subscriptions;

  private:
    // DATA
    bsl::shared_ptr<mqbi::Queue> d_queue_sp;
    // Queue this QueueHandle belongs to.

    bsl::shared_ptr<const mqbi::QueueHandleRequesterContext>
        d_clientContext_sp;
    // Context of the client requesting
    // this QueueHandle.

    mqbstat::QueueStatsDomain* d_domainStats_p;
    // Pointer to the stats of this queue,
    // with regards to the domain.

    bmqp_ctrlmsg::QueueHandleParameters d_handleParameters;
    // Queue handle parameters.  This
    // variable is read/written only from
    // the queue dispatcher thread.

    Subscriptions d_subscriptions;
    // Map of downstream subscriptions,

    SubStreams d_subStreamInfos;
    // (Upstream facing) map appId to queue
    // subStreams.

    Downstreams d_downstreams;
    // Downstream facing map of substreams.

    bool d_isClientClusterMember;
    // Flag indicating if the client
    // associated with this queue handle is
    // cluster member.  If so, queue handle
    // specifies only message guid when
    // delivering a message to the client
    // for efficiency.

    bdlmt::Throttle d_throttledFailedAckMessages;
    // Throttler for failed ACK messages.

    bdlmt::Throttle d_throttledDroppedPutMessages;

    bmqu::OperationChain d_deconfigureChain;
    // Mechanism to serialize execution of
    // the substream deconfigure callbacks
    // and the caller callback invoked when
    // all the substreams are deconfigured.

    bmqp::SchemaLearner::Context d_schemaLearnerPutContext;

    bmqp::SchemaLearner::Context d_schemaLearnerPushContext;

    bslma::Allocator* d_allocator_p;
    // Allocator to use.

  private:
    // PRIVATE MANIPULATORS
    void confirmMessageDispatched(const bmqt::MessageGUID& msgGUID,
                                  unsigned int downstreamSubQueueId);

    void rejectMessageDispatched(const bmqt::MessageGUID& msgGUID,
                                 unsigned int downstreamSubQueueId);

    mqbu::ResourceUsageMonitorStateTransition::Enum
    updateMonitor(const bsl::shared_ptr<Downstream>& subStream,
                  const bmqt::MessageGUID&           msgGUID,
                  bmqp::EventType::Enum              type);

    mqbu::ResourceUsageMonitorStateTransition::Enum
    updateMonitor(mqbi::QueueHandle::UnconfirmedMessageInfoMap::iterator it,
                  Subscription*         subscription,
                  bmqp::EventType::Enum type,
                  const bsl::string&    appId);

    /// THREAD: this method must be called from the Queue dispatcher thread.
    void configureDispatched(
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb);

    /// THREAD: this method must be called from the Queue dispatcher thread.
    void deconfigureDispatched(
        const mqbi::QueueHandle::VoidFunctor& deconfiguredCb);

    /// THREAD: this method is invoked only from queue dispatcher thread.
    void clearClientDispatched(bool hasLostClient);

    /// Called by the `Queue` to deliver the specified `message` with the
    /// specified `msgSize`, `msgGUID`, `attributes`, `isOutOfOrder`, and
    /// `msgGroupId` for the specified `subQueueInfos` streams of the queue.
    /// The behavior is undefined unless the queueHandle can send a message at
    /// this time for all of the `subQueueInfos` streams (see
    /// 'canDeliver(unsigned int subQueueId)' for more information).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void
    deliverMessageImpl(const bsl::shared_ptr<bdlbb::Blob>&       message,
                       const int                                 msgSize,
                       const bmqt::MessageGUID&                  msgGUID,
                       const mqbi::StorageMessageAttributes&     attributes,
                       const bmqp::Protocol::MsgGroupId&         msgGroupId,
                       const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
                       bool                                      isOutOfOrder);

    void makeSubStream(const bsl::string& appId,
                       unsigned int       downstreamSubQueueId,
                       unsigned int       upstreamSubQueueId);

    const bsl::shared_ptr<Downstream>&
         downstream(unsigned int subQueueId) const;
    bool validateDownstreamId(unsigned int downstreamSubQueueId) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueHandle, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a `QueueHandle` for the specified `queueSp` having the
    /// specified `clientContext`, `domainStats` and `handleParameters`.
    /// Use the specified `allocator` for memory allocations.
    QueueHandle(const bsl::shared_ptr<mqbi::Queue>& queueSp,
                const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                           clientContext,
                mqbstat::QueueStatsDomain*                 domainStats,
                const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                bslma::Allocator*                          allocator);

    /// Destructor
    ~QueueHandle() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual mqi::QueueHandle)

    /// Return the unique instance ID of this specific handle for the
    /// associated queue.
    unsigned int id() const BSLS_KEYWORD_OVERRIDE;

    /// Return a const pointer to the client associated to this handle.
    /// Note that the returned pointer may be null, if the client has not
    /// been associated.  Note also that this method is thread-safe,
    /// therefore has a minor performance cost.
    const mqbi::DispatcherClient* client() const BSLS_KEYWORD_OVERRIDE;

    /// Return a const pointer to the queue associated to this handle.
    const mqbi::Queue* queue() const BSLS_KEYWORD_OVERRIDE;

    const bmqp_ctrlmsg::QueueHandleParameters&
    handleParameters() const BSLS_KEYWORD_OVERRIDE;
    // Return a reference offering non-modifiable access to the parameters
    // used to create/configure this queueHandle.

    /// Return a reference offering non-modifiable access to the
    /// subStreamInfos of the queue.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    const SubStreams& subStreamInfos() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the client associated with this queue handle is a
    /// cluster member, false otherwise.
    bool isClientClusterMember() const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::QueueHandle)

    /// Return a pointer to the queue associated to this handle.
    mqbi::Queue* queue() BSLS_KEYWORD_OVERRIDE;

    /// Return a pointer to the client associated to this handle.
    mqbi::DispatcherClient* client() BSLS_KEYWORD_OVERRIDE;

    /// Increment read and write reference counters corresponding to the
    /// specified `subStreamInfo` by `readCount` and `writeCount` in
    /// the specified `counts`.  Create new context for the `subStreamInfo`
    /// if it is not registered.  Associate the subStream with the specified
    /// `upstreamSubQueueId`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void
    registerSubStream(const bmqp_ctrlmsg::SubQueueIdInfo& subStreamInfo,
                      unsigned int                        upstreamSubQueueId,
                      const mqbi::QueueCounts& counts) BSLS_KEYWORD_OVERRIDE;

    /// Associate the specified `downstreamId` with the specified
    /// `downstreamSubId` and `upstreamId`.  Reconfigure the
    /// unconfirmedMonitor in accordance with the maxUnconfirmedMessages and
    /// maxUnconfirmedBytes parameters of the specified `ci`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void registerSubscription(unsigned int downstreamSubId,
                              unsigned int downstreamId,
                              const bmqp_ctrlmsg::ConsumerInfo& ci,
                              unsigned int upstreamId) BSLS_KEYWORD_OVERRIDE;

    /// Decrement read and write reference counters corresponding to the
    /// specified `subStreamInfo` by `readCount` and `writeCount` in
    /// the specified `counts`.  Delete `subStreamInfo` context if both
    /// counters drop to zero.  The behavior is undefined unless the stream
    /// identified by `subStreamInfo` is currently registered.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    bool unregisterSubStream(const bmqp_ctrlmsg::SubQueueIdInfo& subStreamInfo,
                             const mqbi::QueueCounts&            counts,
                             bool isFinal) BSLS_KEYWORD_OVERRIDE;

    mqbi::QueueHandle*
    setIsClientClusterMember(bool value) BSLS_KEYWORD_OVERRIDE;
    // Set the flag indicating whether the client associated with this
    // queue handle is cluster member or not to the specified 'value'.
    // Note that queue handle leverages this information to perform certain
    // optimizations.

    /// Called by the `Queue` to deliver the specified `message` with the
    /// specified `msgSize`, `msgGUID`, `attributes`, `isOutOfOrder`, and
    /// `msgGroupId` for the specified `subQueueInfos` streams of the queue.
    /// The behavior is undefined unless the queueHandle can send a message at
    /// this time for all of the `subQueueInfos` streams (see
    /// 'canDeliver(unsigned int subQueueId)' for more information).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void
    deliverMessage(const bsl::shared_ptr<bdlbb::Blob>&       message,
                   const bmqt::MessageGUID&                  msgGUID,
                   const mqbi::StorageMessageAttributes&     attributes,
                   const bmqp::Protocol::MsgGroupId&         msgGroupId,
                   const bmqp::Protocol::SubQueueInfosArray& subscriptions,
                   bool isOutOfOrder) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `Queue` to deliver the specified `message` with the
    /// specified `msgGUID`, `attributes` and `msgGroupId` for the specified
    /// `subQueueInfos` streams of the queue.  This method is identical with
    /// `deliverMessage()` but it doesn't update any flow-control mechanisms
    /// implemented by this handler.  The behavior is undefined unless the
    /// queueHandle can send a message at this time for each of the
    /// `subQueueInfos` (see `canDeliver(unsigned int subQueueId)` for more
    /// information).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void deliverMessageNoTrack(
        const bsl::shared_ptr<bdlbb::Blob>&       message,
        const bmqt::MessageGUID&                  msgGUID,
        const mqbi::StorageMessageAttributes&     attributes,
        const bmqp::Protocol::MsgGroupId&         msgGroupId,
        const bmqp::Protocol::SubQueueInfosArray& subQueueInfos)
        BSLS_KEYWORD_OVERRIDE;

    /// Post the message with the specified PUT `header`, `appData` and
    /// `options` to the queue.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    void postMessage(const bmqp::PutHeader&              header,
                     const bsl::shared_ptr<bdlbb::Blob>& appData,
                     const bsl::shared_ptr<bdlbb::Blob>& options)
        BSLS_KEYWORD_OVERRIDE;

    /// Used by the client to configure a given queue handle with the
    /// specified `streamParameters`.  Invoke the specified `configuredCb`
    /// when done.
    ///
    /// THREAD: this method can be called from any thread.
    void
    configure(const bmqp_ctrlmsg::StreamParameters& streamParameters,
              const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Used by the client to deconfigure a given queue handle.  Invoke the
    /// specified `deconfiguredCb` when done.
    ///
    /// THREAD: this method can be called from any thread.
    void deconfigureAll(const mqbi::QueueHandle::VoidFunctor& deconfiguredCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Used by the client to indicate that it is no longer interested in
    /// observing the queue this instance represents.
    ///
    /// THREAD: this method can be called from any thread.
    void release(const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                 bool                                       isFinal,
                 const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
        BSLS_KEYWORD_OVERRIDE;

    /// TBD:
    void drop(bool doDeconfigure = true) BSLS_KEYWORD_OVERRIDE;

    /// Clear the client associated with this queue handle.
    void clearClient(bool hasLostClient) BSLS_KEYWORD_OVERRIDE;

    /// Transfer into the specified `out`, the GUID of all messages
    /// associated with the specified `subQueueId` which were delivered to
    /// the client, but that haven't yet been confirmed by it (i.e. the
    /// outstanding pending confirmation messages).  Return the number of
    /// transferred messages.  All internal outstanding messages counters of
    /// this handle will be reset.  Note that `out` can be null, in which
    /// case this becomes a `clear`.  This method is used when a handle is
    /// deleted, or loses its read flag during a reconfigure, in order to
    /// redistribute those messages to another consumer.
    int transferUnconfirmedMessageGUID(const mqbi::RedeliveryVisitor& out,
                                       unsigned int subQueueId)
        BSLS_KEYWORD_OVERRIDE;

    /// Set the `handleParameters` member of this `QueueHandle` to the
    /// specified `handleParameters` and return a pointer offering
    /// modifiable access to this object.  Update the stats of this queue
    /// with regards to the domain, taking into account the added or removed
    /// flags.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    mqbi::QueueHandle* setHandleParameters(
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters)
        BSLS_KEYWORD_OVERRIDE;

    /// Set the `streamParameters` member of this `QueueHandle` to the
    /// specified `streamParameters` and return a pointer offering
    /// modifiable access to this object.  Reconfigure the
    /// unconfirmedMonitor associated with the subQueueId contained in the
    /// `streamParameters` (if any, or default subQueueId otherwise) in
    /// accordance with the maxUnconfirmedMessages and maxUnconfirmedBytes
    /// parameters of the `streamParameters`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    mqbi::QueueHandle*
    setStreamParameters(const bmqp_ctrlmsg::StreamParameters& streamParameters)
        BSLS_KEYWORD_OVERRIDE;

    // Methods called by the Queue/QueueEngine
    // - - - - - - - - - - - - - - - - -

    /// Confirm the message with the specified `msgGUID` for the specified
    /// `subscriptionId` subscription of the queue.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    void
    confirmMessage(const bmqt::MessageGUID& msgGUID,
                   unsigned int downstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Reject the message with the specified `msgGUID` for the specified
    /// `subscriptionId` subscription of the queue.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    void
    rejectMessage(const bmqt::MessageGUID& msgGUID,
                  unsigned int downstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `Queue` when the message with the specified `msgGUID`
    /// and `correlationId` has been acknowledged (whether it's success or
    /// error).  The time at which a success confirm is generated depends on
    /// the configuration and the SLA, whether it's when the message is
    /// locally saved, or when one, multiple or all remote member of the
    /// cluster have received it.  The result of the operation is in the
    /// specified `status`.  If status is not success, then the specified
    /// `errorDescription` contains a human readable string definition of
    /// the issue.
    ///
    /// NOTE: Depending on the configuration and parameters used when
    ///       opening the queue, it may or may not end up sending an ack
    ///       message to the client.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void
    onAckMessage(const bmqp::AckMessage& ackMessage) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return true if the queueHandle can send a message to the client
    /// which has subscribed to the specified `downstreamSubscriptionId`,
    /// and false otherwise.  Note the queueHandle may or may not be able to
    /// send a message at this time (for example to respect flow control,
    /// throttling limits, number of outstanding un-acked messages, ...).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    bool canDeliver(unsigned int downstreamSubscriptionId) const
        BSLS_KEYWORD_OVERRIDE;

    /// Return a vector of all `ResourceUsageMonitor` representing the
    /// unconfirmed messages delivered to the client associated with the
    /// specified `appId` if it exists, and null otherwise.
    /// TBD: Maybe review to consider taking an `appKey` instead of `appId`
    /// for better performance.
    const bsl::vector<const mqbu::ResourceUsageMonitor*>
    unconfirmedMonitors(const bsl::string& appId) const BSLS_KEYWORD_OVERRIDE;

    /// Return number of unconfirmed messages for the optionally specified
    /// `subId` unless it has the default value `k_UNASSIGNED_SUBQUEUE_ID`,
    /// in which case return number of unconfirmed messages for all streams.
    bsls::Types::Int64 countUnconfirmed(
        unsigned int subQueueId = bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID)
        const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` object, the internal details about
    /// this queue handle.
    void loadInternals(mqbcmd::QueueHandle* out) const BSLS_KEYWORD_OVERRIDE;

    /// Return SchemaLearner Context associated with this handle.
    bmqp::SchemaLearner::Context&
    schemaLearnerContext() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class QueueHandle
// -----------------

inline mqbi::Queue* QueueHandle::queue()
{
    return d_queue_sp.get();
}

inline mqbi::DispatcherClient* QueueHandle::client()
{
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(d_clientContext_sp)) {
        return d_clientContext_sp->client();
    }

    return 0;
}

inline void QueueHandle::makeSubStream(const bsl::string& appId,
                                       unsigned int       downstreamSubQueueId,
                                       unsigned int       upstreamSubQueueId)
{
    if (downstreamSubQueueId >= d_downstreams.size()) {
        d_downstreams.resize(downstreamSubQueueId + 1);
    }
    d_downstreams[downstreamSubQueueId].reset(
        new (*d_allocator_p)
            Downstream(appId, upstreamSubQueueId, d_allocator_p),
        d_allocator_p);
}

inline bool
QueueHandle::validateDownstreamId(unsigned int downstreamSubQueueId) const
{
    if (downstreamSubQueueId < d_downstreams.size()) {
        return d_downstreams[downstreamSubQueueId];
    }
    return false;
}

inline const bsl::shared_ptr<QueueHandle::Downstream>&
QueueHandle::downstream(unsigned int downstreamSubQueueId) const
{
    BSLS_ASSERT_SAFE(validateDownstreamId(downstreamSubQueueId));

    return d_downstreams[downstreamSubQueueId];
}

inline const mqbi::DispatcherClient* QueueHandle::client() const
{
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(d_clientContext_sp)) {
        return d_clientContext_sp->client();
    }

    return 0;
}

inline const mqbi::Queue* QueueHandle::queue() const
{
    return d_queue_sp.get();
}

inline unsigned int QueueHandle::id() const
{
    return d_handleParameters.qId();
}

inline const bmqp_ctrlmsg::QueueHandleParameters&
QueueHandle::handleParameters() const
{
    return d_handleParameters;
}

inline const mqbi::QueueHandle::SubStreams& QueueHandle::subStreamInfos() const
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));
    return d_subStreamInfos;
}

inline bool QueueHandle::isClientClusterMember() const
{
    return d_isClientClusterMember;
}

inline mqbi::QueueHandle* QueueHandle::setIsClientClusterMember(bool value)
{
    d_isClientClusterMember = value;
    return this;
}

inline bmqp::SchemaLearner::Context& QueueHandle::schemaLearnerContext() const
{
    return d_schemaLearnerPutContext;
}

inline QueueHandle::Downstream::Downstream(const bsl::string& appId,
                                           unsigned int upstreamSubQueueId,
                                           bslma::Allocator* allocator_p)
: d_appId(appId)
, d_upstreamSubQueueId(upstreamSubQueueId)
, d_data(new(*allocator_p)
             mqbi::QueueHandle::UnconfirmedMessageInfoMap(allocator_p),
         allocator_p)
{
    // NOTHING
}

inline QueueHandle::ConfirmFunctor::ConfirmFunctor(
    QueueHandle*      owner,
    bmqt::MessageGUID guid,
    unsigned int      downstreamSubQueueId)
: d_owner_p(owner)
, d_guid(guid)
, d_downstreamSubQueueId(downstreamSubQueueId)
{
    // NOTHING
}

inline void QueueHandle::ConfirmFunctor::operator()() const
{
    d_owner_p->confirmMessageDispatched(d_guid, d_downstreamSubQueueId);
}

}  // close package namespace
}  // close enterprise namespace

#endif
