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

// mqbmock_queue.h                                                    -*-C++-*-
#ifndef INCLUDED_MQBMOCK_QUEUE
#define INCLUDED_MQBMOCK_QUEUE

//@PURPOSE: Provide a mock queue implementation of the 'mqbi::Queue' interface.
//
//@CLASSES:
//  mqbmock::Queue:         mock Queue implementation
//  mqbmock::HandleFactory: mock Queue Handle factory implementation
//@DESCRIPTION: This component provides a mock Queue implementation,
// 'mqbmock::Queue', of the 'mqbi::DispatcherClient' and 'mqbi::Queue'
// protocols that is used to emulate a real queue for testing purposes.
// Finally, it provides a mock Queue Handle factory implementation,
// 'mqbmock::HandleFactory', of the 'mqbi::HandleFactory' protocol that is
// used to create mock Queue Handle objects.
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

#include <mqbcfg_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbstat_queuestats.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqp {
struct AckMessage;
}
namespace bmqt {
class MessageGUID;
}
namespace mqbcmd {
class QueueCommand;
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
class Storage;
}
namespace mqbstat {
class QueueStatsDomain;
}
namespace mwcst {
class StatContext;
}

namespace mqbmock {

// ===========
// Class Queue
// ===========

/// Mock queue implementation of the `mqbi::Queue` inteface
class Queue : public mqbi::Queue {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBMOCK.QUEUE");

  public:
    // PUBLIC TYPES
    typedef bsl::function<void(const mqbi::DispatcherEvent& event)>
        DispatcherEventHandler;

  private:
    // DATA
    bmqt::Uri d_uri;

    bsl::string d_description;

    bool d_atMostOnce;

    bool d_deliverAll;

    bool d_hasMultipleSubStreams;

    bmqp_ctrlmsg::QueueHandleParameters d_handleParameters;
    // Aggregated handle parameters of all
    // currently opened queueHandles to
    // this queue

    bmqp_ctrlmsg::StreamParameters d_streamParameters;
    // Aggregated stream parameters of all
    // currently opened queueHandles to
    // this queue

    mqbcfg::MessageThrottleConfig d_messageThrottleConfig;
    // Configuration values for message
    // throttling intervals and thresholds.

    mqbstat::QueueStatsDomain d_stats;
    // Statistics of the queue

    mqbi::Domain* d_domain_p;
    // Domain of this queue

    mqbi::Dispatcher* d_dispatcher_p;
    // Dispatcher for this queue

    mqbi::DispatcherClientData d_dispatcherClientData;
    // Dispatcher client data of this queue

    mqbi::QueueEngine* d_queueEngine_p;
    // Queue Engine for this queue.

    mqbi::Storage* d_storage_p;
    // Storage for this queue.
    DispatcherEventHandler d_dispatcherEventHandler;

    mutable bmqp::SchemaLearner d_schemaLearner;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Queue, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbi::MockQueue` object associated with the specified
    /// `domain`.  Use the specified `allocator` for any memory allocation.
    Queue(mqbi::Domain* domain, bslma::Allocator* allocator);

    /// Destructor.
    ~Queue() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::DispatcherClient)

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

    // MANIPULATORS
    //   (virtual: mqbi::Queue)
    int configure(bsl::ostream& errorDescription,
                  bool          isReconfigure,
                  bool          wait) BSLS_KEYWORD_OVERRIDE;

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
    /// THREAD: this method can be called from any thread.
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

    /// Return the resource capacity meter associated to this queue, if any,
    /// or a null pointer otherwise.
    mqbu::CapacityMeter* capacityMeter() BSLS_KEYWORD_OVERRIDE;

    /// Return the queue engine used by this queue.
    mqbi::QueueEngine* queueEngine() BSLS_KEYWORD_OVERRIDE;

    /// Return the stats associated with this queue.
    mqbstat::QueueStatsDomain* stats() BSLS_KEYWORD_OVERRIDE;

    /// Return number of unconfirmed messages across all handles with the
    /// `specified `subId'.
    bsls::Types::Int64
    countUnconfirmed(unsigned int subId) BSLS_KEYWORD_OVERRIDE;

    /// Called when a message with the specified `msgGUID`, `appData`,
    /// `options` and compressionAlgorithmType payload is pushed to this
    /// queue.  Note that depending upon the location of the queue instance,
    /// `appData` may be empty.
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
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onOpenUpstream(bsls::Types::Uint64 genCount,
                        unsigned int upstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Notify the (remote) queue about reopen failure.  The queue NACKs all
    /// pending and incoming PUTs and drops CONFIRMs related to to the
    /// specified `upstreamSubQueueId`.  The queue can transition out of
    /// this state onReopenUpstream' with non-zero `genCount`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onOpenFailure(unsigned int upstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Invoked by the Data Store when it receives quorum Receipts.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onReceipt(const bmqt::MessageGUID&  msgGUID,
                   mqbi::QueueHandle*        qH,
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

    /// Synchronously process the specified `command` and write the result
    /// to the specified `result` object.  Return zero on success or a
    /// nonzero value otherwise.
    virtual int
    processCommand(mqbcmd::QueueResult*        result,
                   const mqbcmd::QueueCommand& command) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (specific to mqbmock::Queue)
    Queue& _setDispatcher(mqbi::Dispatcher* value);
    Queue& _setQueueEngine(mqbi::QueueEngine* value);
    Queue& _setStorage(mqbi::Storage* value);
    Queue& _setAtMostOnce(const bool value);
    Queue& _setDeliverAll(const bool value);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    Queue& _setHasMultipleSubStreams(const bool value);
    Queue& _setDispatcherEventHandler(const DispatcherEventHandler& handler);

    // ACCESSORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;

    /// Return a printable description of the client (e.g. for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::Queue)

    /// Return the domain this queue belong to.
    mqbi::Domain* domain() const BSLS_KEYWORD_OVERRIDE;

    /// Return the storage used by this queue.
    mqbi::Storage* storage() const BSLS_KEYWORD_OVERRIDE;

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

    /// Return the Schema Leaner associated with this queue.
    bmqp::SchemaLearner& schemaLearner() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (specific to mqbi::MockQueue)
};

// ===================
// class HandleFactory
// ===================

/// Provide a mock implementation of the `mqbi::QueueHandleFactory` creating
/// mock Queue Handle objects.
class HandleFactory : public mqbi::QueueHandleFactory {
  public:
    // CREATORS

    /// Destructor.
    ~HandleFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Create a new mock handle, using the specified `allocator`, for the
    /// specified `queue` as requested by the specified `clientContext` with
    /// the specified `parameters`, and associated with the specified
    /// `stats`.
    mqbi::QueueHandle*
    makeHandle(const bsl::shared_ptr<mqbi::Queue>& queue,
               const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                          clientContext,
               mqbstat::QueueStatsDomain*                 stats,
               const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
               mqbu::SingleCounter*                       parent,
               bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
