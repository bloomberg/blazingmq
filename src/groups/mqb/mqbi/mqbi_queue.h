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

// mqbi_queue.h                                                       -*-C++-*-
#ifndef INCLUDED_MQBI_QUEUE
#define INCLUDED_MQBI_QUEUE

//@PURPOSE: Provide an interface for a Queue and its associated QueueHandle.
//
//@CLASSES:
//  mqbi::Queue:                       Interface for a Queue
//  mqbi::QueueCounts:                 VST for pair of read/write counters
//  mqbi::QueueHandle:                 Interface for a Queue Handle
//  mqbi::QueueHandleRequesterContext: VST for QueueHandle requester context
//  mqbi::QueueHandleFactory:          Interface for a Queue Handle factory
//
//@DESCRIPTION: 'mqbi::Queue' is the interface representing a queue.  It is
// primarily an internal object that is mostly utilized through the
// 'mqbi::QueueHandle'.  'mqbi::QueueHandle' is a light-weight proxy interface
// to a 'mqbi::Queue' that links together an 'mqbi::Queue' with an associated
// Client ('mbqa::ClientSession'), keeping track of the state of the client in
// that queue.  The 'mqbi::QueueHandleRequesterContext' is a value-semantic
// type providing the context of the client requester of a handle to a queue.
// Each 'mqbi::QueueHandle' is associated to one and exactly one 'mqbi::Queue'
// and one and exactly one client; however, an 'mqbi::Queue' may have multiple
// 'mqbi::QueueHandle'.  The 'mqbi::QueueHandle' is implemented in
// 'mbqblp::QueueHandle', while the 'mqbi::Queue' interface should be
// implemented by the plugins mechanism.  In the Bloomberg in-house
// implementation of a MQ, the 'mqbi::Queue' implementation will be responsible
// for saving data in a storage and communicating with other brokers from the
// cluster.  Note that implementation of a 'mqbi::Queue' should use the
// 'mbqblp::QueueEngine' to manipulate the 'mqbi::QueueHandle'.  An
// 'mqbi::QueueHandleFactory' is a small factory interface for a component
// which can create 'mqbi::QueueHandle' components.

// MQB

#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_queueid.h>
#include <bmqp_schemalearner.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>

#include <bmqc_orderedhashset.h>
#include <mwcst_statcontext.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslh_hash.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqt {
class Uri;
}
namespace mqbcmd {
class QueueCommand;
}
namespace mqbcmd {
class PurgeQueueResult;
}
namespace mqbcmd {
class QueueHandle;
}
namespace mqbcmd {
class QueueResult;
}
namespace mqbu {
class CapacityMeter;
}
namespace mqbu {
class ResourceUsageMonitor;
}
namespace mqbstat {
class QueueStatsDomain;
}
namespace mqbstat {
class QueueStatsClient;
}

namespace mqbi {

// FORWARD DECLARATION
class DispatcherClient;
class Domain;
class Queue;
class QueueEngine;

// =======================
// class InlineClient
// =======================
struct InlineResult {
    enum Enum {
        e_SUCCESS           = 0,
        e_UNAVAILABLE       = 1,
        e_INVALID_PRIMARY   = 2,
        e_INVALID_GEN_COUNT = 3,
        e_CHANNEL_ERROR     = 4,
        e_INVALID_PARTITION = 5,
        e_SELF_PRIMARY      = 6
    };
    // CLASS METHODS
    static const char* toAscii(InlineResult::Enum value);
    static bool        isPermanentError(bmqt::AckResult::Enum* ackResult,
                                        InlineResult::Enum     value);
};

class InlineClient {
    // Interface for PUSH and ACK.

  public:
    // CREATORS
    virtual ~InlineClient();

    virtual mqbi::InlineResult::Enum
    sendPush(const bmqt::MessageGUID&                  msgGUID,
             int                                       queueId,
             const bsl::shared_ptr<bdlbb::Blob>&       message,
             const mqbi::StorageMessageAttributes&     attributes,
             const bmqp::MessagePropertiesInfo&        mps,
             const bmqp::Protocol::SubQueueInfosArray& subQueues) = 0;
    // Called by the 'queueId' to deliver the specified 'message' with the
    // specified 'message', 'msgGUID', 'attributes' and 'mps' for the
    // specified 'subQueues' streams of the queue.
    // Return 'InlineResult::Enum'.
    //
    // THREAD: This method is called from the Queue's dispatcher thread.

    virtual mqbi::InlineResult::Enum
    sendAck(const bmqp::AckMessage& ackMessage, unsigned int queueId) = 0;
    // Called by the 'Queue' to send the specified 'ackMessage'.
    // Return 'InlineResult::Enum'.
    //
    // THREAD: This method is called from the Queue's dispatcher thread.
};

// =================================
// class QueueHandleRequesterContext
// =================================

/// Value-semantic type representing the context of a client, requester of a
/// queue handle.
class QueueHandleRequesterContext {
  public:
    // TYPES

    /// Type representing unique Id for a requester of queue handle.
    typedef bsls::Types::Int64 RequesterId;

    // PUBLIC CLASS DATA

    /// Constant representing invalid requester Id.
    static const RequesterId k_INVALID_REQUESTER_ID = -1;

  private:
    // PRIVATE CLASS DATA

    // This counter is used to generate unique requester Id (see
    // `generateUniqueRequesterId()`.
    static bsls::AtomicInt64 s_previousRequesterId;

  private:
    // DATA
    DispatcherClient* d_client_p;
    // The requester client to associate with
    // the handle.

    bmqp_ctrlmsg::ClientIdentity d_identity;
    // The identity of the associated client.
    // Note that we make a deep copy of the
    // client's identity because at the time
    // this field is accessed, the client may
    // have gone away.

    bsl::string d_description;
    // Short identifier of the client

    bool d_isClusterMember;
    // Indicate whether the requester is part of
    // the cluster or not; this is used by the
    // queueHandle to decide whether it should
    // include the payload in the PUSHED message
    // or expect the destination peer to already
    // have it and enrich itself the message
    // with its payload.

    RequesterId d_requesterId;
    // Unique ID associated with the requester
    // of a queue handle.

    bsl::shared_ptr<mwcst::StatContext> d_statContext_sp;

    InlineClient* d_inlineClient_p;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueHandleRequesterContext,
                                   bslma::UsesBslmaAllocator)

    // CLASS METHODS

    /// Return a unique Id for a requester of a queue handle.  This method
    /// can be invoked concurrently from multiple threads.
    static RequesterId generateUniqueRequesterId();

    // CREATORS

    /// Default constructor
    explicit QueueHandleRequesterContext(bslma::Allocator* allocator = 0);

    /// Create a `QueueHandleRequesterContext` object having the same value
    /// as the specified `original` object, and using the specified
    /// `allocator`.
    QueueHandleRequesterContext(const QueueHandleRequesterContext& original,
                                bslma::Allocator*                  allocator);

    // MANIPULATORS
    QueueHandleRequesterContext& setClient(DispatcherClient* value);
    QueueHandleRequesterContext&
    setIdentity(const bmqp_ctrlmsg::ClientIdentity& value);
    QueueHandleRequesterContext& setDescription(const bsl::string& value);
    QueueHandleRequesterContext& setIsClusterMember(bool value);

    /// Set the corresponding data member to the specified `value` and
    /// return a reference offering modifiable access to this object.
    QueueHandleRequesterContext& setRequesterId(RequesterId value);
    QueueHandleRequesterContext&
    setStatContext(const bsl::shared_ptr<mwcst::StatContext>& value);

    QueueHandleRequesterContext& setInlineClient(InlineClient* inlineClient);

    // ACCESSORS
    DispatcherClient*                   client() const;
    const bmqp_ctrlmsg::ClientIdentity& identity() const;
    const bsl::string&                  description() const;
    bool                                isClusterMember() const;

    /// Return the corresponding data member's value.
    RequesterId requesterId() const;

    const bsl::shared_ptr<mwcst::StatContext>& statContext() const;

    InlineClient* inlineClient() const;

    /// Return true if the requester node is first hop after the client (or
    /// last hop before the client).
    bool isFirstHop() const;
};

// ==========================
// class QueueHandleRequester
// ==========================

/// Interface for a requester of QueueHandle.
class QueueHandleRequester {
  public:
    // CREATORS

    /// Destructor of this object.
    virtual ~QueueHandleRequester();

    // ACCESSORS

    /// Return a non-modifiable reference to the context of this requester
    /// of a QueueHandle.
    virtual const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
    handleRequesterContext() const = 0;
};

// ==================================
// struct OpenQueueConfirmationCookie
// ==================================

struct OpenQueueContext {
    // Type of a 'cookie' provided in the 'OpenQueueCallback' to confirm
    // processing of the 'openQueue' response by the requester.  Opening a
    // queue is fully async, and it could happen that the requester went
    // down before the 'openQueue' response got delivered to it.  In this
    // case, we must rollback upstream state.  This cookie is used for
    // that: it is initialized to zero (in the 'Cluster' implementation),
    // and carried over to the original requester of the 'openQueue'.  If
    // the requester is not able to process the openQueue response, it
    // needs to set this cookie to the queue handle which it received, so
    // that the operation can be rolled back.

    QueueHandle*                               d_handle;
    bsl::shared_ptr<mqbstat::QueueStatsClient> d_stats;

    OpenQueueContext();
    OpenQueueContext(QueueHandle* handle);
};

typedef bsl::shared_ptr<OpenQueueContext> OpenQueueConfirmationCookie;

// ==================
// struct QueueCounts
// ==================

/// VST keeping a pair of read/write counters.
struct QueueCounts {
    // DATA
    int d_readCount;
    int d_writeCount;

    // CREATORS
    QueueCounts(int readCount, int writeCount);

    /// Increment read and write counts by corresponding counts in the
    /// specified `value`.
    QueueCounts& operator+=(const QueueCounts& value);

    /// Increment read and write counts by corresponding counts in the
    /// specified `value`.  The behavior is undefined unless the counts are
    /// greater or equal to the specified values.
    QueueCounts& operator-=(const QueueCounts& value);

    bool isValid() const;
};

// ===============================
// struct QueueHandleReleaseResult
// ===============================

struct QueueHandleReleaseResult {
    // TYPES
    enum ReleaseResultFlags {
        // Handle release event processing

        e_NONE = 0

        /// no more consumers or producers for all subStream for this handle
        ,
        e_NO_HANDLE_CLIENTS = (1 << 0)

        /// no more consumers for this subStream for this handle
        ,
        e_NO_HANDLE_STREAM_CONSUMERS = (1 << 1)

        /// no more producers for this subStream for this handle
        ,
        e_NO_HANDLE_STREAM_PRODUCERS = (1 << 2)

        /// no more consumers for this subStream across all handles
        ,
        e_NO_QUEUE_STREAM_CONSUMERS = (1 << 3)

        /// no more producers for this subStream across all handles
        ,
        e_NO_QUEUE_STREAM_PRODUCERS = (1 << 4)
    };

    // DATA
    int d_result;

    // CREATORS
    QueueHandleReleaseResult();

    // MANIPULATORS

    /// Indicate absence of consumers for this subStream for this handle in
    /// this queue.
    void makeNoHandleStreamConsumers();

    /// Indicate absence of producers for this subStream for this handle in
    /// this queue.
    void makeNoHandleStreamProducers();

    /// Indicate absence of consumers for this subStream for all handles in
    /// this queue.
    void makeNoQueueStreamConsumers();

    /// Indicate absence of producers for this subStream for all handles in
    /// this queue.
    void makeNoQueueStreamProducers();

    /// Indicate absence of consumers and producers for all subStreams in
    /// in this handle in this queue.
    void makeNoHandleClients();

    /// Copy all indications from the specified `value`
    void apply(const QueueHandleReleaseResult& value);

    // ACCESSORS

    /// Return true if there are no consumers for this subStream for this
    /// handle in this queue.
    bool hasNoHandleStreamConsumers() const;

    /// Return true if there are no producers for this subStream for this
    /// handle in this queue.
    bool hasNoHandleStreamProducers() const;

    /// Return true if there are no consumers for this subStream for all
    /// handles in  this queue.
    bool hasNoQueueStreamConsumers() const;

    /// Return true if there are no producers for this subStream for all
    /// handles in this queue.
    bool hasNoQueueStreamProducers() const;

    /// Return true if there are no consumers and producers for this
    /// subStream for all handles in this queue.
    bool isQueueStreamEmpty() const;

    /// Return true if there are no consumers and producers for all
    /// subStreams in in this handle in this queue.
    bool hasNoHandleClients() const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                   stream,
                         const QueueHandleReleaseResult& rhs);

// =============================
// struct UnconfirmedMessageInfo
// =============================

/// VST containing information about a message that has been pushed but not
/// yet confirmed.
struct UnconfirmedMessageInfo {
    // DATA
    unsigned int d_size;  // The associated message size in bytes
                          // (using 'unsigned int' and not Int64
                          // since per bmqp protocol, an
                          // individual message size is limited
                          // to 1GB).

    bsls::Types::Int64 d_timeStamp;  // The time when the message was pushed
                                     // to the downstream, in absolute
                                     // nanoseconds referenced to an
                                     // arbitrary but fixed origin.

    unsigned int d_subscriptionId;

    // CREATORS
    UnconfirmedMessageInfo(unsigned int       size,
                           bsls::Types::Int64 timeStamp,
                           unsigned int       subscription);
};

/// Set of GUIDs to redeliver when handle goes away before receiving
/// CONFIRMs
typedef bsl::function<void(const bmqt::MessageGUID& msgGUID)>
    RedeliveryVisitor;

// =================
// class QueueHandle
// =================

/// Interface for a QueueHandle.
class QueueHandle {
  public:
    // TYPES
    typedef bsl::function<void(const bmqp_ctrlmsg::Status& status,
                               QueueHandle*                handle)>
        GetHandleCallback;
    // Signature of the callback to provide to the 'getHandle' method.  The
    // specified 'status' conveys the result of obtaining a handle, and the
    // specified 'handle' corresponds to the handle obtained.

    /// Signature of the callback to provide to the `configure` method.  The
    /// specified `status` conveys the result of configure operation and the
    /// specified `streamParameters` are the absolute stream parameters
    /// configured on the handle.
    typedef bsl::function<void(
        const bmqp_ctrlmsg::Status&           status,
        const bmqp_ctrlmsg::StreamParameters& streamParameters)>
        HandleConfiguredCallback;

    /// Signature of the callback to provide to the `release` method.  The
    /// specified `handle` corresponds to the one that was just released,
    /// and the specified `isDeleted` indicates whether this was the last
    /// references to that handle and it has now been deleted.  If
    /// `isDeleted` is true, `handle` remains valid for as long there is a
    /// reference to this shared pointer; however note that it is undefined
    /// to call any method such as `postMessage`, `confirmMessage`, etc., on
    /// this `handle` in this case as it has been released from the queue,
    /// and per contract, no more downstream actions are expected.
    typedef bsl::function<void(const bsl::shared_ptr<QueueHandle>& handle,
                               const QueueHandleReleaseResult      result)>
        HandleReleasedCallback;

    /// Signature of a `void` functor method.
    typedef bsl::function<void(void)> VoidFunctor;

    /// An ordered hash map of GUID and associated message info.
    typedef bmqc::OrderedHashMap<bmqt::MessageGUID,
                                 UnconfirmedMessageInfo,
                                 bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        UnconfirmedMessageInfoMap;
    typedef bsl::shared_ptr<UnconfirmedMessageInfoMap> RedeliverySp;

    struct StreamInfo {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(StreamInfo, bslma::UsesBslmaAllocator)

        QueueCounts                    d_counts;
        unsigned int                   d_downstreamSubQueueId;
        unsigned int                   d_upstreamSubQueueId;
        bmqp_ctrlmsg::StreamParameters d_streamParameters;

        bsl::shared_ptr<mqbstat::QueueStatsClient> d_clientStats;

        StreamInfo(const QueueCounts& counts,
                   unsigned int       downstreamSubQueueId,
                   unsigned int       upstreamSubQueueId,
                   bslma::Allocator*  allocator_p);

        StreamInfo(const StreamInfo& other, bslma::Allocator* allocator_p);
    };
    /// Map `appId` to downstream `subId` and read/write counts.
    typedef bsl::unordered_map<const bsl::string, StreamInfo> SubStreams;

  public:
    // CREATORS

    /// Destructor
    virtual ~QueueHandle();

    // MANIPULATORS

    /// Return a pointer to the queue associated to this handle.
    virtual Queue* queue() = 0;

    /// Return a pointer to the client associated to this handle.
    virtual DispatcherClient* client() = 0;

    /// Increment read and write reference counters corresponding to the
    /// specified `subStreamInfo` by `readCount` and `writeCount` in
    /// the specified `counts`.  Create new context for the `subStreamInfo`
    /// if it is not registered.  Associate the subStream with the specified
    /// `upstreamSubQueueId`.
    /// Return an iterator pointing to the context.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual SubStreams::const_iterator
    registerSubStream(const bmqp_ctrlmsg::SubQueueIdInfo& subStreamInfo,
                      unsigned int                        upstreamSubQueueId,
                      const mqbi::QueueCounts&            counts) = 0;

    /// Associate the specified `downstreamId` with the specified
    /// `downstreamSubId` and `upstreamId`.  Reconfigure the
    /// unconfirmedMonitor in accordance with the maxUnconfirmedMessages and
    /// maxUnconfirmedBytes parameters of the specified `ci`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void registerSubscription(unsigned int downstreamSubId,
                                      unsigned int downstreamId,
                                      const bmqp_ctrlmsg::ConsumerInfo& ci,
                                      unsigned int upstreamId) = 0;

    /// Decrement read and write reference counters corresponding to the
    /// specified `subStreamInfo` by `readCount` and `writeCount` in
    /// the specified `counts`.  Delete `subStreamInfo` context if both
    /// counters drop to zero.  The behavior is undefined unless the stream
    /// identified by `subStreamInfo` is currently registered.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual bool
    unregisterSubStream(const bmqp_ctrlmsg::SubQueueIdInfo& subStreamInfo,
                        const mqbi::QueueCounts&            counts,
                        bool                                isFinal) = 0;

    /// Set the flag indicating whether the client associated with this
    /// queue handle is cluster member or not to the specified `value`.
    /// Note that queue handle leverages this information to perform certain
    /// optimizations.
    virtual QueueHandle* setIsClientClusterMember(bool value) = 0;

    /// Post the message with the specified PUT `header`, `appData` and
    /// `options` to the queue.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    virtual void postMessage(const bmqp::PutHeader&              header,
                             const bsl::shared_ptr<bdlbb::Blob>& appData,
                             const bsl::shared_ptr<bdlbb::Blob>& options) = 0;

    /// Confirm the message with the specified `msgGUID` for the specified
    /// `subQueueId` stream of the queue.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    virtual void confirmMessage(const bmqt::MessageGUID& msgGUID,
                                unsigned int             subQueueId) = 0;

    /// Reject the message with the specified `msgGUID` for the specified
    /// `subQueueId` stream of the queue.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    virtual void rejectMessage(const bmqt::MessageGUID& msgGUID,
                               unsigned int             subQueueId) = 0;

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
    virtual void onAckMessage(const bmqp::AckMessage& ackMessage) = 0;

    /// Called by the `Queue` to deliver a message under the specified `iter`
    /// with the specified `msgGroupId` for the specified `subscriptions` of
    /// the queue.  The behavior is undefined unless the queueHandle can send
    /// a message at this time for each of the corresponding subStreams (see
    /// `canDeliver(unsigned int subQueueId)` for more details).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    deliverMessage(const mqbi::StorageIterator&              iter,
                   const bmqp::Protocol::MsgGroupId&         msgGroupId,
                   const bmqp::Protocol::SubQueueInfosArray& subscriptions,
                   bool                                      isOutOfOrder) = 0;

    /// Called by the `Queue` to deliver a message under the specified `iter`
    /// with the specified `msgGroupId` for the specified `subscriptions` of
    /// the queue.  This method is identical with `deliverMessage()` but it
    /// doesn't update any flow-control mechanisms implemented by this handler.
    /// The behavior is undefined unless the queueHandle can send a message at
    /// this time (see `canDeliver(unsigned int subQueueId)` for more details).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void deliverMessageNoTrack(
        const mqbi::StorageIterator&              iter,
        const bmqp::Protocol::MsgGroupId&         msgGroupId,
        const bmqp::Protocol::SubQueueInfosArray& subscriptions) = 0;

    /// Used by the client to configure a given queue handle with the
    /// specified `streamParameters`.  Invoke the specified `configuredCb`
    /// when done.
    ///
    /// THREAD: this method can be called from any thread, but note that
    /// various implementations may have a stricter requirement.
    virtual void configure(
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb) = 0;

    /// Used by the client to deconfigure all the substreams related to a
    /// given queue handle.  Invoke the specified `deconfiguredCb` once all
    /// the substreams are deconfigured.
    ///
    /// THREAD: this method can be called from any thread.
    virtual void deconfigureAll(const VoidFunctor& deconfiguredCb) = 0;

    /// Used by the client to indicate that it is no longer interested in
    /// observing the queue this instance represents.  Invoke the specified
    /// `releasedCb` when done.
    ///
    /// THREAD: this method can be called from any thread.
    virtual void
    release(const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
            bool                                       isFinal,
            const HandleReleasedCallback&              releasedCb) = 0;

    /// Useful in shutdown situation, to release any parameters left, in a
    /// thread safe manner with no risk of double release.  If the specified
    /// `doDeconfigure` is true deconfigure all the handle's substreams
    /// before releasing them.
    virtual void drop(bool doDeconfigure = true) = 0;

    /// Clear the client associated with this queue handle.
    virtual void clearClient(bool hasLostClient) = 0;

    /// Transfer into the specified `out`, the GUID of all messages
    /// associated with the specified `subQueueId` which were delivered to
    /// the client, but that haven't yet been confirmed by it (i.e. the
    /// outstanding pending confirmation messages).  Return the number of
    /// transferred messages.  All internal outstanding messages counters of
    /// this handle will be reset.  Note that `out` can be null, in which
    /// case this becomes a `clear`.  This method is used when a handle is
    /// deleted, or loses its read flag during a reconfigure, in order to
    /// redistribute those messages to another consumer.
    virtual int transferUnconfirmedMessageGUID(const RedeliveryVisitor& out,
                                               unsigned int subQueueId) = 0;

    /// Set the `handleParameters` member of this `QueueHandle` to the
    /// specified `handleParameters` and return a pointer offering
    /// modifiable access to this object.  Update the stats of this queue
    /// with regards to the domain, taking into account the added or removed
    /// flags.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual QueueHandle* setHandleParameters(
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters) = 0;

    /// Set the `streamParameters` member of this `QueueHandle` to the
    /// specified `streamParameters` and return a pointer offering
    /// modifiable access to this object.  Reconfigure the
    /// unconfirmedMonitor associated with the subQueueId contained in the
    /// `streamParameters` (if any, or default subQueueId otherwise) in
    /// accordance with the maxUnconfirmedMessages and maxUnconfirmedBytes
    /// parameters of the `streamParameters`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual QueueHandle* setStreamParameters(
        const bmqp_ctrlmsg::StreamParameters& streamParameters) = 0;

    // ACCESSORS

    /// Return a const pointer to the client associated to this handle.
    virtual const DispatcherClient* client() const = 0;

    /// Return a const pointer to the queue associated to this handle.
    virtual const Queue* queue() const = 0;

    /// Return the unique ID of this queue handle, as advertised from
    /// downstream connection upon queue opening.
    virtual unsigned int id() const = 0;

    /// Return a reference offering non-modifiable access to the parameters
    /// used to create/configure this queueHandle.
    virtual const bmqp_ctrlmsg::QueueHandleParameters&
    handleParameters() const = 0;

    /// Return a reference offering non-modifiable access to the
    /// subStreamInfos of the queue.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual const SubStreams& subStreamInfos() const = 0;

    /// Return true if the client associated with this queue handle is a
    /// cluster member, false otherwise.
    virtual bool isClientClusterMember() const = 0;

    /// Return true if the queueHandle can send a message to the client
    /// which has subscribed to the specified `downstreamSubscriptionId`,
    /// and false otherwise.  Note the queueHandle may or may not be able to
    /// send a message at this time (for example to respect flow control,
    /// throttling limits, number of outstanding un-acked messages, ...).
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual bool canDeliver(unsigned int downstreamSubscriptionId) const = 0;

    /// Return a vector of all `ResourceUsageMonitor` representing the
    /// unconfirmed messages delivered to the client associated with the
    /// specified `appId` if it exists, and null otherwise.
    /// TBD: Maybe review to consider taking an `appKey` instead of `appId`
    /// for better performance.
    virtual const bsl::vector<const mqbu::ResourceUsageMonitor*>
    unconfirmedMonitors(const bsl::string& appId) const = 0;

    /// Return number of unconfirmed messages for the optionally specified
    /// `subId` unless it has the default value `k_UNASSIGNED_SUBQUEUE_ID`,
    /// in which case return number of unconfirmed messages for all streams.
    virtual bsls::Types::Int64
    countUnconfirmed(unsigned int subId =
                         bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID) const = 0;

    /// Load in the specified `out` object, the internal details about this
    /// queue handle.
    virtual void loadInternals(mqbcmd::QueueHandle* out) const = 0;

    /// Return SchemaLearner Context associated with this handle.
    virtual bmqp::SchemaLearner::Context& schemaLearnerContext() const = 0;
};

// ===========
// class Queue
// ===========

/// Interface for a Queue.
class Queue : public DispatcherClient {
  public:
    // CREATORS

    /// Destructor
    ~Queue() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Configure this queue instance, where `isReconfigure` dictates
    /// whether this is a reconfiguration of an existing queue.
    ///
    /// If `wait` is `true`, this method will not return until the operation
    /// has completed. The return value will be 0 if it succeeds, and
    /// nonzero if there was an error; in case of an error, the specified
    /// `errorDescription` stream will be populated with a human readable
    /// reason.
    ///
    /// If `wait` is `false`, this method will return 0 after scheduling the
    /// operation on an unspecified thread, and `errorDescription` will be
    /// unmodified.
    ///
    /// THREAD: this method can be called from any thread.
    virtual int configure(bsl::ostream& errorDescription,
                          bool          isReconfigure,
                          bool          wait) = 0;

    /// Obtain a handle to this queue, for the client represented by the
    /// specified `clientContext` and using the specified `handleParameters`
    /// and `upstreamSubQueueId`.  Load a reference to the corresponding
    /// `mqbstat::QueueStatsClient` into the specified `context`.
    /// Invoke the specified `callback` with the result.
    virtual void getHandle(
        const mqbi::OpenQueueConfirmationCookie&            context,
        const bsl::shared_ptr<QueueHandleRequesterContext>& clientContext,
        const bmqp_ctrlmsg::QueueHandleParameters&          handleParameters,
        unsigned int                                        upstreamSubQueueId,
        const QueueHandle::GetHandleCallback&               callback) = 0;

    /// Configure the specified `handle` with the specified
    /// `streamParameters`, and invoke the specified `configuredCb` callback
    /// when finished.
    ///
    /// THREAD: this method can be called from any thread.
    virtual void configureHandle(
        QueueHandle*                                 handle,
        const bmqp_ctrlmsg::StreamParameters&        streamParameters,
        const QueueHandle::HandleConfiguredCallback& configuredCb) = 0;

    /// Release the specified `handle`.
    ///
    /// THREAD: this method can be called from any thread.
    virtual void
    releaseHandle(QueueHandle*                               handle,
                  const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                  bool                                       isFinal,
                  const QueueHandle::HandleReleasedCallback& releasedCb) = 0;

    /// Drop the specified `handle`.  If the specified `doDeconfigure` is
    /// true deconfigure all the handle's substreams before releasing them.
    virtual void dropHandle(QueueHandle* handle,
                            bool         doDeconfigure = true) = 0;

    /// Close this queue.
    ///
    /// THREAD: this method can be called from any thread.
    virtual void close() = 0;

    /// Return the resource capacity meter associated to this queue, if
    /// any, or a null pointer otherwise.
    virtual mqbu::CapacityMeter* capacityMeter() = 0;

    /// Return the queue engine used by this queue.
    virtual QueueEngine* queueEngine() = 0;

    /// Set the stats associated with this queue.
    virtual void
    setStats(const bsl::shared_ptr<mqbstat::QueueStatsDomain>& stats) = 0;

    /// Return number of unconfirmed messages across all handles with the
    /// `specified `subId'.
    virtual bsls::Types::Int64 countUnconfirmed(unsigned int subId) = 0;

    /// Stop sending PUSHes but continue receiving CONFIRMs, receiving and
    /// sending PUTs and ACKs.
    virtual void stopPushing() = 0;

    /// Called when a message with the specified `msgGUID`, `appData`,
    /// `options`, `compressionAlgorithmType` payload is pushed to this
    /// queue.  Note that depending upon the location of the queue instance,
    /// `appData` may be empty.
    virtual void onPushMessage(
        const bmqt::MessageGUID&             msgGUID,
        const bsl::shared_ptr<bdlbb::Blob>&  appData,
        const bsl::shared_ptr<bdlbb::Blob>&  options,
        const bmqp::MessagePropertiesInfo&   messagePropertiesInfo,
        bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
        bool                                 isOutOfOrder) = 0;

    /// Confirm the message with the specified `msgGUID` for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `source`.  Also note that since there
    /// are no `ack of confirms`, this method is void, and will eventually
    /// throttle warnings if the `msgGUID` is invalid.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void confirmMessage(const bmqt::MessageGUID& msgGUID,
                                unsigned int             upstreamSubQueueId,
                                QueueHandle*             source) = 0;

    /// Reject the message with the specified `msgGUID` for the specified
    /// `upstreamSubQueueId` stream of the queue on the specified `source`.
    ///  Return resulting RDA counter.
    ///
    /// THREAD: this method can be called from any thread and is responsible
    ///         for calling the corresponding method on the `Queue`, on the
    ///         Queue's dispatcher thread.
    virtual int rejectMessage(const bmqt::MessageGUID& msgGUID,
                              unsigned int             upstreamSubQueueId,
                              mqbi::QueueHandle*       source) = 0;

    /// Invoked by the cluster whenever it receives an ACK from upstream.
    virtual void onAckMessage(const bmqp::AckMessage& ackMessage) = 0;

    /// Notify the queue that the upstream is not ready to receive
    /// PUTs/CONFIRMs.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void onLostUpstream() = 0;

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
    virtual void onOpenUpstream(bsls::Types::Uint64 genCount,
                                unsigned int        upstreamSubQueueId,
                                bool                isWriterOnly = false) = 0;

    /// Notify the (remote) queue about (re)open failure.  The queue NACKs
    /// all pending and incoming PUTs and drops CONFIRMs related to to the
    /// specified `upstreamSubQueueId`.  The queue can transition out of
    /// this state onReopenUpstream' with non-zero `genCount`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void onOpenFailure(unsigned int upstreamSubQueueId) = 0;

    /// Invoked by the Data Store when it receives quorum Receipts for the
    /// specified `msgGUID`.  Send ACK to the specified `queueHandle` if it
    /// is present in the queue handle catalog.  Update AVK time stats using
    /// the specified `arrivalTimepoint`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void onReceipt(const bmqt::MessageGUID&  msgGUID,
                           mqbi::QueueHandle*        queueHandle,
                           const bsls::Types::Int64& arrivalTimepoint) = 0;

    /// Invoked by the Data Store when it removes (times out waiting for
    /// quorum Receipts for) a message with the specified `msgGUID`.  Send
    /// NACK with the specified `result` to the specified `gH` if it is
    /// present in the queue handle catalog.
    ///
    /// THREAD: This method is called from the Storage dispatcher thread.
    virtual void onRemoval(const bmqt::MessageGUID& msgGUID,
                           mqbi::QueueHandle*       qH,
                           bmqt::AckResult::Enum    result) = 0;

    /// Invoked when data store has transmitted all cached replication
    /// records.  Switch from PUT processing to PUSH delivery.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void onReplicatedBatch() = 0;

    /// Synchronously process the specified `command` and load the result in
    /// the specified `result` object.  Return zero on success or a nonzero
    /// value otherwise.
    virtual int processCommand(mqbcmd::QueueResult*        result,
                               const mqbcmd::QueueCommand& command) = 0;

    // ACCESSORS

    /// Return the domain this queue belong to.
    virtual Domain* domain() const = 0;

    /// Return the storage used by this queue.
    virtual Storage* storage() const = 0;

    /// Return the stats associated with this queue.
    virtual const bsl::shared_ptr<mqbstat::QueueStatsDomain>&
    stats() const = 0;

    /// Return the partitionId assigned to this queue.
    virtual int partitionId() const = 0;

    /// Return the URI of this queue.
    virtual const bmqt::Uri& uri() const = 0;

    /// Return the unique ID of this queue, as advertised to upstream
    /// connection upon queue opening.
    virtual unsigned int id() const = 0;

    /// Returns `true` if the configuration for this queue requires
    /// at-most-once semantics or `false` otherwise.
    virtual bool isAtMostOnce() const = 0;

    /// Return `true` if the configuration for this queue requires
    /// delivering to all consumers, and `false` otherwise.
    virtual bool isDeliverAll() const = 0;

    /// Returns `true` if the configuration for this queue requires
    /// has-multiple-sub-streams semantics or `false` otherwise.
    virtual bool hasMultipleSubStreams() const = 0;

    /// Return a reference not offering modifiable access to the aggregated
    /// parameters of all currently opened queueHandles on this queue.
    virtual const bmqp_ctrlmsg::QueueHandleParameters&
    handleParameters() const = 0;

    /// Return true if the queue has upstream parameters for the specified
    /// `upstreamSubQueueId` in which case load the parameters into the
    /// specified `value`.  Return false otherwise.
    virtual bool
    getUpstreamParameters(bmqp_ctrlmsg::StreamParameters* value,
                          unsigned int upstreamSubQueueId) const = 0;

    /// Return the message throttle config associated with this queue.
    virtual const mqbcfg::MessageThrottleConfig&
    messageThrottleConfig() const = 0;

    /// Return the Schema Leaner associated with this queue.
    virtual bmqp::SchemaLearner& schemaLearner() const = 0;
};

// ========================
// class QueueHandleFactory
// ========================

/// Interface for a QueueHandle factory.
class QueueHandleFactory {
  public:
    // CREATORS

    /// Destructor.
    virtual ~QueueHandleFactory();

    // MANIPULATORS

    /// Create a new handle, using the specified `allocator`, for the
    /// specified `queue` as requested by the specified `clientContext` with
    /// the specified `parameters`, and associated wit the specified
    /// `stats`.
    virtual QueueHandle* makeHandle(
        const bsl::shared_ptr<Queue>&                       queue,
        const bsl::shared_ptr<QueueHandleRequesterContext>& clientContext,
        mqbstat::QueueStatsDomain*                          stats,
        const bmqp_ctrlmsg::QueueHandleParameters&          handleParameters,
        bslma::Allocator*                                   allocator) = 0;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline const char* InlineResult::toAscii(InlineResult::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SUCCESS)
        CASE(UNAVAILABLE)
        CASE(CHANNEL_ERROR)
        CASE(INVALID_PARTITION)
        CASE(INVALID_PRIMARY)
        CASE(INVALID_GEN_COUNT)
        CASE(SELF_PRIMARY)
    default: return "(* UNKNOWN *)";
    }
#undef CASE
}

inline bool InlineResult::isPermanentError(bmqt::AckResult::Enum* ackResult,
                                           InlineResult::Enum     value)
{
    switch (value) {
    case e_SUCCESS: BSLS_ANNOTATION_FALLTHROUGH;
    case e_UNAVAILABLE: BSLS_ANNOTATION_FALLTHROUGH;
    case e_INVALID_PRIMARY: BSLS_ANNOTATION_FALLTHROUGH;
    case e_INVALID_GEN_COUNT: BSLS_ANNOTATION_FALLTHROUGH;
    case e_CHANNEL_ERROR: return false;
    case e_INVALID_PARTITION:
        *ackResult = bmqt::AckResult::e_INVALID_ARGUMENT;
        break;
    case e_SELF_PRIMARY: *ackResult = bmqt::AckResult::e_UNKNOWN; break;
    }
    return true;
}

// ----------------------------------
// struct OpenQueueConfirmationCookie
// ----------------------------------

inline OpenQueueContext::OpenQueueContext()
: d_handle()
, d_stats()
{
    // NOTHING
}

inline OpenQueueContext::OpenQueueContext(QueueHandle* handle)
: d_handle(handle)
, d_stats()
{
    // NOTHING
}

// ------------------
// struct QueueCounts
// ------------------

inline QueueCounts::QueueCounts(int readCount, int writeCount)
: d_readCount(readCount)
, d_writeCount(writeCount)
{
    // NOTHING
}

inline QueueCounts& QueueCounts::operator+=(const QueueCounts& value)
{
    d_readCount += value.d_readCount;
    d_writeCount += value.d_writeCount;

    return *this;
}

inline QueueCounts& QueueCounts::operator-=(const QueueCounts& value)
{
    BSLS_ASSERT_SAFE(d_readCount >= value.d_readCount);
    BSLS_ASSERT_SAFE(d_writeCount >= value.d_writeCount);

    d_readCount -= value.d_readCount;
    d_writeCount -= value.d_writeCount;

    return *this;
}

// -----------------------------
// struct UnconfirmedMessageInfo
// -----------------------------

inline UnconfirmedMessageInfo::UnconfirmedMessageInfo(
    unsigned int       size,
    bsls::Types::Int64 timeStamp,
    unsigned int       subscription)
: d_size(size)
, d_timeStamp(timeStamp)
, d_subscriptionId(subscription)
{
    // NOTHING
}

inline bool QueueCounts::isValid() const
{
    return d_readCount >= 0 && d_writeCount >= 0;
}

// ------------------------------
// class QueueHandleReleaseResult
// ------------------------------

inline QueueHandleReleaseResult::QueueHandleReleaseResult()
: d_result(e_NONE)
{
    // NOTHING
}

inline void QueueHandleReleaseResult::makeNoHandleStreamConsumers()
{
    d_result |= e_NO_HANDLE_STREAM_CONSUMERS;
}

inline void QueueHandleReleaseResult::makeNoHandleStreamProducers()
{
    d_result |= e_NO_HANDLE_STREAM_PRODUCERS;
}

inline void QueueHandleReleaseResult::makeNoQueueStreamConsumers()
{
    d_result |= e_NO_QUEUE_STREAM_CONSUMERS;
}

inline void QueueHandleReleaseResult::makeNoQueueStreamProducers()
{
    d_result |= e_NO_QUEUE_STREAM_PRODUCERS;
}

inline void QueueHandleReleaseResult::makeNoHandleClients()
{
    d_result |= e_NO_HANDLE_CLIENTS;
}

inline void
QueueHandleReleaseResult::apply(const QueueHandleReleaseResult& value)
{
    d_result |= value.d_result;
}

inline bool QueueHandleReleaseResult::hasNoHandleClients() const
{
    return (d_result & e_NO_HANDLE_CLIENTS);
}

inline bool QueueHandleReleaseResult::hasNoHandleStreamConsumers() const
{
    return (d_result & e_NO_HANDLE_STREAM_CONSUMERS);
}

inline bool QueueHandleReleaseResult::hasNoHandleStreamProducers() const
{
    return (d_result & e_NO_HANDLE_STREAM_PRODUCERS);
}

inline bool QueueHandleReleaseResult::isQueueStreamEmpty() const
{
    return hasNoQueueStreamConsumers() && hasNoQueueStreamProducers();
}

inline bool QueueHandleReleaseResult::hasNoQueueStreamConsumers() const
{
    return (d_result & e_NO_QUEUE_STREAM_CONSUMERS);
}

inline bool QueueHandleReleaseResult::hasNoQueueStreamProducers() const
{
    return (d_result & e_NO_QUEUE_STREAM_PRODUCERS);
}

// ---------------------------------
// class QueueHandleRequesterContext
// ---------------------------------

// CLASS METHODS
inline QueueHandleRequesterContext::RequesterId
QueueHandleRequesterContext::generateUniqueRequesterId()
{
    return ++s_previousRequesterId;
}

// CREATORS
inline QueueHandleRequesterContext::QueueHandleRequesterContext(
    bslma::Allocator* allocator)
: d_client_p(0)
, d_identity(allocator)
, d_description(allocator)
, d_isClusterMember(false)
, d_requesterId(k_INVALID_REQUESTER_ID)
, d_statContext_sp()
, d_inlineClient_p(0)
{
    // NOTHING
}

inline QueueHandleRequesterContext::QueueHandleRequesterContext(
    const QueueHandleRequesterContext& original,
    bslma::Allocator*                  allocator)
: d_client_p(original.d_client_p)
, d_identity(original.d_identity, allocator)
, d_description(original.d_description, allocator)
, d_isClusterMember(original.d_isClusterMember)
, d_requesterId(original.d_requesterId)
, d_statContext_sp(original.d_statContext_sp)
, d_inlineClient_p(original.d_inlineClient_p)
{
    // NOTHING
}

inline QueueHandleRequesterContext&
QueueHandleRequesterContext::setClient(DispatcherClient* value)
{
    d_client_p = value;
    return *this;
}

inline QueueHandleRequesterContext& QueueHandleRequesterContext::setIdentity(
    const bmqp_ctrlmsg::ClientIdentity& value)
{
    d_identity = value;
    return *this;
}

inline QueueHandleRequesterContext&
QueueHandleRequesterContext::setDescription(const bsl::string& value)
{
    d_description = value;
    return *this;
}

inline QueueHandleRequesterContext&
QueueHandleRequesterContext::setIsClusterMember(bool value)
{
    d_isClusterMember = value;
    return *this;
}

inline QueueHandleRequesterContext&
QueueHandleRequesterContext::setRequesterId(RequesterId value)
{
    d_requesterId = value;
    return *this;
}

inline QueueHandleRequesterContext&
QueueHandleRequesterContext::setStatContext(
    const bsl::shared_ptr<mwcst::StatContext>& value)
{
    d_statContext_sp = value;
    return *this;
}

inline QueueHandleRequesterContext&
QueueHandleRequesterContext::setInlineClient(InlineClient* inlineClient)
{
    d_inlineClient_p = inlineClient;
    return *this;
}

inline DispatcherClient* QueueHandleRequesterContext::client() const
{
    return d_client_p;
}

inline const bmqp_ctrlmsg::ClientIdentity&
QueueHandleRequesterContext::identity() const
{
    return d_identity;
}

inline const bsl::string& QueueHandleRequesterContext::description() const
{
    return d_description;
}

inline bool QueueHandleRequesterContext::isClusterMember() const
{
    return d_isClusterMember;
}

inline bool QueueHandleRequesterContext::isFirstHop() const
{
    return d_identity.clientType() == bmqp_ctrlmsg::ClientType::E_TCPCLIENT;
}

inline QueueHandleRequesterContext::RequesterId
QueueHandleRequesterContext::requesterId() const
{
    return d_requesterId;
}

inline const bsl::shared_ptr<mwcst::StatContext>&
QueueHandleRequesterContext::statContext() const
{
    return d_statContext_sp;
}

inline InlineClient* QueueHandleRequesterContext::inlineClient() const
{
    return d_inlineClient_p;
}

}  // close package namespace

}  // close enterprise namespace

#endif
