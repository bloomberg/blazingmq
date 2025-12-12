// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbmock_queuehandle.h                                              -*-C++-*-
#ifndef INCLUDED_MQBMOCK_QUEUEHANDLE
#define INCLUDED_MQBMOCK_QUEUEHANDLE

//@PURPOSE: Provide a mock Queue Handle implementation.
//
//@CLASSES:
//  mqbmock::QueueHandle: mock Queue Handle implementation
//
//@DESCRIPTION: This component provides a mock Queue Handle implementation,
// 'mqbmock::QueueHandle', of the 'mqbi::QueueHandle' interface that is used to
// emulate a real Queue Handle for testing purposes.  Note that methods not
// needed for testing purposes are a no-op and/or return bogus or error values.
//
/// Notes
///------
// At the time of this writing, this component implements desired behavior for
// only those methods of the base protocols that are needed for testing
// 'mqbi::QueueEngine' concrete implementations and 'mqba::ClientSession';
// other methods a no-op and/or return bogus or error values.
//
// Additionally, the set of methods that are specific to this component is the
// minimal set required for testing concrete implementations of
// 'mqbi::QueueEngine' and 'mqba::ClientSession'.  These methods are denoted
// with a leading underscore ('_').

// MQB

#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>
#include <mqbu_resourceusagemonitor.h>

// BMQ
#include <bmqc_orderedhashmap.h>
#include <bmqc_twokeyhashmap.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>
#include <bmqt_messageguid.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_iosfwd.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslh_hash.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslstl_stringref.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class QueueHandle;
}
namespace mqbstat {
class QueueStatsDomain;
}

namespace mqbmock {

// =================
// Class QueueHandle
// =================

/// Mock Queue Handle implementation used to emulate a real queue handle.
class QueueHandle : public mqbi::QueueHandle {
  private:
    // PRIVATE TYPES

    /// guid -> message.
    /// Must be a container in which iteration order is same as insertion
    /// order.
    typedef bmqc::OrderedHashMap<
        bmqt::MessageGUID,
        bsl::pair<bsl::shared_ptr<bdlbb::Blob>, unsigned>,
        bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        GUIDMap;

    struct Downstream {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(Downstream, bslma::UsesBslmaAllocator)

        // PUBLIC DATA

        GUIDMap d_unconfirmedMessages;
        // Collection of messages per
        // stream that have been sent but
        // not yet confirmed.

        bool d_canDeliver;
        // flags per stream indicating if
        // the queue handle can deliver
        // more messages to the client for
        // the corresponding stream.  Note
        // this can be used to imitate
        // behavior relating to
        // 'maxUnconfirmed'.

        const unsigned int d_upstreamSubQueueId;

        // CREATORS
        explicit Downstream(unsigned int      upstreamSubQueueId,
                            bslma::Allocator* allocator);

        Downstream(const Downstream& other, bslma::Allocator* allocator);
    };

  public:
    // PUBLIC TYPES

    /// downstreamSubQueueId -> Downstream
    typedef bsl::unordered_map<unsigned int, Downstream> Downstreams;
    typedef bsl::unordered_map<unsigned int, mqbi::QueueHandle::RedeliverySp>
        Messages;

    struct Subscription {
        unsigned int d_downstreamSubQueueId;
        unsigned int d_upstreamSubscriptionId;

        Subscription(unsigned int downstreamSubQueueId,
                     unsigned int upstreamSubscriptionId)
        : d_downstreamSubQueueId(downstreamSubQueueId)
        , d_upstreamSubscriptionId(upstreamSubscriptionId)
        {
            // NOTHING
        }
    };
    /// downstreamSubscriptionId -> downstreamSubQueueId
    typedef bsl::unordered_map<unsigned int, Subscription> Subscriptions;

  private:
    // DATA
    bmqp_ctrlmsg::QueueHandleParameters d_handleParameters;
    // Queue handle parameters.  This
    // variable is read/written only from
    // the queue dispatcher thread.

    Downstreams d_downstreams;
    //  Map of (appId, subId) to context

    SubStreams d_subStreamInfos;
    // Map of (appId, subQueueId) to queue
    // counts corresponding to queue
    // subStreams opened in this session

    Subscriptions d_subscriptions;

    mqbi::DispatcherClient* d_client_p;
    // Handle of the dispatcher mapped to
    // the client associated to this
    // QueueHandle.

    bsl::shared_ptr<mqbi::Queue> d_queue_sp;
    // Queue this QueueHandle belongs to.

    mqbu::ResourceUsageMonitor d_unconfirmedMessageMonitor;
    // Resource usage monitor to use.

    bmqp::SchemaLearner::Context d_schemaLearnerContext;

    bslma::Allocator* d_allocator_p;
    // Allocator to use.

  private:
    // PRIVATE ACCESSORS

    /// Assert that the specified `appId` and `subQueueId` constitute a
    /// consistent identification of a subStream that is supported by this
    /// object.  Note that supported subStreams are either the default
    /// subStream or a subStream associated with a fanout consumer.
    void assertConsistentSubStreamInfo(const bsl::string& appId,
                                       unsigned int       subQueueId) const;

    unsigned int subscription2downstreamSubQueueId(unsigned int sId) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueHandle, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a mock `QueueHandle` for the specified `queueSp` having the
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

    // MANIPULATORS
    //   (virtual: mqbi::QueueHandle)

    /// Return a pointer to the queue associated to this handle.
    mqbi::Queue* queue() BSLS_KEYWORD_OVERRIDE;

    /// Return a pointer to the client associated to this handle.
    mqbi::DispatcherClient* client() BSLS_KEYWORD_OVERRIDE;

    /// Increment read and write reference counters corresponding to the
    /// specified `subStreamInfo` by `readCount` and `writeCount` in
    /// the specified `counts`.  Create new context for the `subStreamInfo`
    /// if it is not registered.
    /// Return an iterator pointing to the context.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    mqbi::QueueHandle::SubStreams::const_iterator
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

    /// Set the flag indicating whether the client associated with this
    /// queue handle is cluster member or not to the specified `value`.
    /// Note that queue handle leverages this information to perform certain
    /// optimizations.
    mqbi::QueueHandle*
    setIsClientClusterMember(bool value) BSLS_KEYWORD_OVERRIDE;

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

    /// Confirm the message with the specified `msgGUID` for the specified
    /// `downstreamSubQueueId` stream of the queue.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    void
    confirmMessage(const bmqt::MessageGUID& msgGUID,
                   unsigned int downstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Reject the message with the specified `msgGUID` for the specified
    /// `downstreamSubQueueId` stream of the queue.
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

    /// Called by the `Queue` to deliver a message under the specified `iter`
    /// with the specified `msgGroupId` for the specified `subscriptions` of
    /// the queue.  The behavior is undefined unless the queueHandle can send
    /// a message at this time for each of the corresponding subStreams (see
    /// `canDeliver(unsigned int subQueueId)` for more details).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void
    deliverMessage(const mqbi::StorageIterator&              message,
                   const bmqp::Protocol::MsgGroupId&         msgGroupId,
                   const bmqp::Protocol::SubQueueInfosArray& subscriptions,
                   bool isOutOfOrder) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `Queue` to deliver a message under the specified `iter`
    /// with the specified `msgGroupId` for the specified `subscriptions` of
    /// the queue.  This method is identical with `deliverMessage()` but it
    /// doesn't update any flow-control mechanisms implemented by this handler.
    /// The behavior is undefined unless the queueHandle can send a message at
    /// this time (see `canDeliver(unsigned int subQueueId)` for more details).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void deliverMessageNoTrack(const mqbi::StorageIterator&      message,
                               const bmqp::Protocol::MsgGroupId& msgGroupId,
                               const bmqp::Protocol::SubQueueInfosArray&
                                   subscriptions) BSLS_KEYWORD_OVERRIDE;

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
    /// observing the queue this instance represents.  Invoke the specified
    /// `releasedCb` when done.
    ///
    /// THREAD: this method can be called from any thread.
    void
    release(const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
            bool                                       isFinal,
            const HandleReleasedCallback& releasedCb) BSLS_KEYWORD_OVERRIDE;

    void drop(bool doDeconfigure = true) BSLS_KEYWORD_OVERRIDE;

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
    /// parameters of the `streamParameters`.  Note that if either
    /// `maxUnconfirmedMessages == 0` or `maxUnconfirmedBytes == 0`, then
    /// subsequently `canDeliver` for the associated stream will return
    /// false, and otherwise it will return true.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    mqbi::QueueHandle*
    setStreamParameters(const bmqp_ctrlmsg::StreamParameters& streamParameters)
        BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (specific to mqbmock::QueueHandle)

    /// Set whether the handle can deliver to the client for the optionally
    /// specified `appId` stream, and if the handle acquires the capacity to
    /// deliver for the stream then indicate so to the associated queue
    /// engine (used to imitate behavior relating to `maxUnconfirmed`).
    /// Return a reference offering modifiable access to this object.  If
    /// `appId` is not specified, then the default stream is assumed.  The
    /// behavior is undefined unless the stream identified by `appId` has
    /// been registered.
    QueueHandle& _setCanDeliver(bool value);
    QueueHandle& _setCanDeliver(const bsl::string& appId, bool value);

    /// Remove all messages that were sent to the optionally specified
    /// `appId` stream of this consumer but not yet confirmed by it.  If
    /// `appId` is not specified, then the default stream is assumed.  The
    /// behavior is undefined unless the stream identified by `appId` has
    /// been registered.
    void _resetUnconfirmed(
        const bsl::string& appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID);

    // ACCESSORS
    //   (virtual mqbi::QueueHandle)

    /// Return a const pointer to the client associated to this handle.
    /// Note that the returned pointer may be null, if the client has not
    /// been associated.  Note also that this method is thread-safe,
    /// therefore has a minor performance cost.
    const mqbi::DispatcherClient* client() const BSLS_KEYWORD_OVERRIDE;

    /// Return a const pointer to the queue associated to this handle.
    const mqbi::Queue* queue() const BSLS_KEYWORD_OVERRIDE;

    /// Return the unique instance ID of this specific handle for the
    /// associated queue.
    unsigned int id() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the parameters
    /// used to create/configure this queueHandle.
    const bmqp_ctrlmsg::QueueHandleParameters&
    handleParameters() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the
    /// subStreamInfos of the queue.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    const SubStreams& subStreamInfos() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the client associated with this queue handle is a
    /// cluster member, false otherwise.
    bool isClientClusterMember() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the queueHandle can send a message to the client
    /// which has subscribed to the specified `downstreamSubscriptionId`,
    /// and false otherwise.  Note the queueHandle may or may not be able to
    /// send a message at this time (for example to respect flow control,
    /// throttling limits, number of outstanding un-acked messages, ...).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    bool canDeliver(unsigned int downstreamSubscriptionId) const
        BSLS_KEYWORD_OVERRIDE;

    /// Return a pointer to the `ResourceUsageMonitor` representing the
    /// unconfirmed messages delivered to the client associated with the
    /// specified `appId` if it exists, and null otherwise.
    /// TBD: Maybe review to consider taking an `appKey` instead of `appId`
    /// for better performance.
    const bsl::vector<const mqbu::ResourceUsageMonitor*>
    unconfirmedMonitors(const bsl::string& appId) const BSLS_KEYWORD_OVERRIDE;

    /// Return number of unconfirmed messages for all streams.
    bsls::Types::Int64 countUnconfirmed() const BSLS_KEYWORD_OVERRIDE;

    /// Print to the specified `out` object, the internal details about this
    /// queue handle.
    void loadInternals(mqbcmd::QueueHandle* out) const BSLS_KEYWORD_OVERRIDE;

    /// Return SchemaLearner Context associated with this handle.
    bmqp::SchemaLearner::Context&
    schemaLearnerContext() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (specific to mqbmock::QueueHandle)

    /// Return a string containing all messages that were sent to the
    /// optionally specified `appId` stream of this consumer but not yet
    /// confirmed by it.  If `appId` is not specified, then the default
    /// stream is assumed.  The returned string is formatted as follows:
    /// `<msg1>,<msg2>,...,<msgN>`
    ///
    /// The behavior is undefined unless the stream identified by `appId`
    /// has been registered.
    bsl::string _messages(
        const bsl::string& appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID) const;

    /// Return the number of messages that were sent to this consumer for
    /// the optionally specified `appId` stream but not yet confirmed.  If
    /// `appId` is not specified, then the default stream is assumed.  The
    /// behavior is undefined unless the stream identified by `appId` has
    /// been registered.
    int _numMessages(
        const bsl::string& appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID) const;

    /// Return the number of subStreams of this handle that are currently
    /// active, meaning the number of subStream that are registered and have
    /// not yet been unregistered.
    size_t _numActiveSubstreams() const;

    /// Return a string containing all appIds corredonding to the streams
    /// registered to this handle.
    const bsl::string _appIds() const;

    /// Return the stream parameters of the registered stream associated
    /// with the optionally specified `appId`.  If `appId` is not specified,
    /// then the default stream is assumed.  The behavior is undefined
    /// unless the stream identified by `appId` has been registered.
    const bmqp_ctrlmsg::StreamParameters& _streamParameters(
        const bsl::string& appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID) const;
};

}
}

#endif
