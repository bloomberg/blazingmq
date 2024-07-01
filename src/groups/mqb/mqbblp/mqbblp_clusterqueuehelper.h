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

// mqbblp_clusterqueuehelper.h                                        -*-C++-*-
#ifndef INCLUDED_MQBBLP_CLUSTERQUEUEHELPER
#define INCLUDED_MQBBLP_CLUSTERQUEUEHELPER

//@PURPOSE: Provide a mechanism to manage queues on a cluster.
//
//@CLASSES:
//  mqbblp::ClusterQueueHelper : Mechanism to manage queues on a cluster
//
//@DESCRIPTION:
//
/// Thread Safety
///-------------
//

// NOTE: This entire component's code is *serialized* and only executes inside
//       the *dispatcher* thread.  That is, *every* method, unless explicitly
//       stated, should be executed by the dispatcher thread.

// MQB

#include <mqbblp_queue.h>
#include <mqbc_clusterdata.h>
#include <mqbc_clustermembership.h>
#include <mqbc_clusterstate.h>
#include <mqbc_electorinfo.h>
#include <mqbcfg_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_clusterstatemanager.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// MWC
#include <mwcio_status.h>

// BDE
#include <ball_log.h>
#include <bsl_functional.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class ClusterQueueHelper;
}
namespace mqbcmd {
class StorageContent;
}
namespace mqbi {
class StorageManager;
}
namespace mqbnet {
class ClusterNode;
}

namespace mqbblp {
// ========================
// class ClusterQueueHelper
// ========================

/// Mechanism to manage queues on a cluster.
class ClusterQueueHelper : public mqbc::ClusterStateObserver,
                           public mqbc::ClusterMembershipObserver,
                           public mqbc::ElectorInfoObserver {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.CLUSTERQUEUEHELPER");

  public:
    // TYPES

    /// Signature of a `void` callback method.
    typedef bsl::function<void(void)> VoidFunctor;

    /// Signature of a callback invoked upon assignment of a queue by this
    /// leader.
    typedef bsl::function<void(const bmqp_ctrlmsg::QueueAssignmentAdvisory&)>
        OnQueueAssignedCb;

    /// Signature of a callback invoked upon un-assignment of a queue by
    /// this leader.
    typedef bsl::function<void(const bmqp_ctrlmsg::QueueUnassignedAdvisory&)>
        OnQueueUnassignedCb;

  private:
    // PRIVATE TYPES
    typedef bsl::shared_ptr<const mqbc::ClusterStateQueueInfo>
        ClusterStateQueueInfoCSp;

    struct OpenQueueContext;

    /// Private struct holding attributes related to a substream of a queue.
    struct SubQueueContext {
        enum Enum {
            // State of the upstream for the given subStream

            k_CLOSED  // Answer Close/Configure requests immediately.
                      // RelayQueueEngine caches new Configuration coming
                      // from Clients.
                      // Close requests subtracts the read/write counts and
                      // if they drop to zeroes, removes the subStream.

            ,
            k_REOPENING  // Reopen response is pending.
                         // Buffer Close requests and send them upon response.
                         // Answer (cached) Configure requests immediately.
                         // Note, this is different from the treatment of
                         // Close.
            ,
            k_OPEN  // Send requests upstream.
            ,
            k_FAILED  // Reopen has failed.
        };

        struct PendingClose {
            const bmqp_ctrlmsg::QueueHandleParameters   d_handleParameters;
            const mqbi::Cluster::HandleReleasedCallback d_callback;

            PendingClose(const bmqp_ctrlmsg::QueueHandleParameters&   hp,
                         const mqbi::Cluster::HandleReleasedCallback& cb)
            : d_handleParameters(hp)
            , d_callback(cb)
            {
                // NOTHING
            }
        };

        bmqp_ctrlmsg::QueueHandleParameters d_parameters;

        Enum d_state;
        // State of the upstream

        bdlmt::EventScheduler::EventHandle d_timer;
        // (timer handle 1s) when waiting for
        // unconfirmed.  This is to cancel the timer in
        // the case when this broker stops while
        // waiting.

        bsl::vector<PendingClose> d_pendingCloseRequests;

        SubQueueContext(
            const bmqt::Uri&                                         uri,
            const bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo>& info,
            bslma::Allocator*                                        a)
        : d_state(k_CLOSED)
        , d_pendingCloseRequests(a)
        {
            d_parameters.uri()       = uri.asString();
            d_parameters.subIdInfo() = info;
        }
    };

    /// Map of {appId, subQueueId} combinations to their
    /// substream-specific (appId, subQueueId) context.
    typedef bmqp::ProtocolUtil::QueueInfo<SubQueueContext> StreamsMap;

    /// Publicly visible struct holding all live information related to a
    /// queue.
    struct QueueLiveState {
      public:
        // PUBLIC DATA
        unsigned int d_id;
        // Upstream id of the queue
        // (mqbi::Queue::k_UNASSIGNED_QUEUE_ID
        // if unassigned)

        StreamsMap d_subQueueIds;
        // Map of subQueueIds and appId
        // associated with an open (or
        // pending-open) subStream of the queue
        // to a context of the subStream
        // (holding some related state)

        unsigned int d_nextSubQueueId;
        // Next upstream subQueueId for a
        // subStream of the queue

        bsl::shared_ptr<mqbblp::Queue> d_queue_sp;
        // Queue created (null if no queue
        // created yet)

        int d_numQueueHandles;
        // Number of queue handles associated
        // with this queue.

        int d_numHandleCreationsInProgress;
        // Number of handle-creations in
        // progress. This counter is
        // incremented every time 'createQueue'
        // is invoked (because currently in the
        // 'open-queue' work flow, a handle
        // creation is always preceded by
        // 'createQueue'.  This counter is
        // decremented in
        // 'onQueueHandleCreated'. This counter
        // is used in this manner:
        // 'd_numQueueHandles' is decremented
        // in 'onQueueHandleDestroyed'.  If
        // that counter goes to zero and this
        // flag is also zero, then and only
        // then will a primary node deleted the
        // queue (assuming pending context and
        // in-flight requests are zero).

        bsls::Types::Int64 d_queueExpirationTimestampMs;
        // Timerstamp (high resolution timer)
        // in milliseconds after which queue
        // will expire.  Zero if queue cannot
        // expire (because it has non-zero
        // messages or handles or both).

        bsl::vector<OpenQueueContext> d_pending;
        // List of all open queue pending
        // contexts which are awaiting for a
        // next step on the queue (assignment,
        // ...).

        int d_inFlight;
        // Number of in flight contexts, that
        // is the number of contexts for which
        // 'd_callback' has not yet been
        // called. Note that this may be
        // different than 'd_pending.size'
        // because the 'd_pending' vector
        // doesn't contain the requests which
        // have been sent and are awaiting an
        // answer (those contexts are stored
        // through binding in the response
        // callback).

      public:
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(QueueLiveState,
                                       bslma::UsesBslmaAllocator)

        // CREATORS

        /// Create a new object using the specified `allocator`.
        QueueLiveState(bslma::Allocator* allocator);

        /// Copy constructor from the specified `other` using the optionally
        /// specified `allocator`.
        QueueLiveState(const QueueLiveState& other,
                       bslma::Allocator*     allocator = 0);

        // MANIPULATORS

        /// Reset the `id`, `partitionId`, `key` and `queue` members of this
        /// object.  Note that `uri` is left untouched because it is an
        /// invariant member of a given instance of such a QueueInfo object.
        void reset();
    };

    struct StopContext {
        mqbnet::ClusterNode*         d_peer;
        bmqp_ctrlmsg::ControlMessage d_response;
        VoidFunctor                  d_callback;
        bsls::TimeInterval           d_stopTime;
        bsl::shared_ptr<StopContext> d_previous_sp;
        // link StopContext for the same node

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(StopContext, bslma::UsesBslmaAllocator)

        // CREATORS
        StopContext(mqbnet::ClusterNode* source,
                    const VoidFunctor&   callback,
                    int                  timeoutMs,
                    bslma::Allocator*    allocator);
    };
    typedef bsl::unordered_map<const mqbnet::ClusterNode*,
                               bsl::weak_ptr<StopContext> >
        StopContexts;

  private:
    // FORWARD DECLARE
    struct QueueContext;

    // PRIVATE TYPES

    /// Type of the RequestManager to use
    typedef bmqp::RequestManager<bmqp_ctrlmsg::ControlMessage,
                                 bmqp_ctrlmsg::ControlMessage>
        RequestManagerType;

    typedef mqbc::ClusterStatePartitionInfo ClusterStatePartitionInfo;

    typedef mqbc::ClusterState::UriToQueueInfoMap      UriToQueueInfoMap;
    typedef mqbc::ClusterState::UriToQueueInfoMapCIter UriToQueueInfoMapCIter;
    typedef mqbc::ClusterState::DomainStates           DomainStates;
    typedef mqbc::ClusterState::DomainStatesCIter      DomainStatesCIter;

    typedef mqbi::ClusterStateManager::QueueAssignmentResult
        QueueAssignmentResult;

    typedef mqbc::ClusterNodeSession::SubQueueInfo CNSSubQueueInfo;
    typedef mqbc::ClusterNodeSession::QueueState   CNSQueueState;
    typedef mqbc::ClusterNodeSession::StreamsMap   CNSStreamsMap;

    typedef mqbc::ClusterNodeSession::QueueHandleMap CNSQueueHandleMap;
    typedef CNSQueueHandleMap::iterator              CNSQueueHandleMapIter;
    typedef CNSQueueHandleMap::const_iterator        CNSQueueHandleMapCIter;

    /// Structure encapsulating the entire context associated with the open
    /// queue request.  One such context is created per each openQueue
    /// request.
    struct OpenQueueContext {
        // PUBLIC DATA
        QueueContext* d_queueContext_p;
        // queueContext associated to this
        // context

        mqbi::Domain* d_domain_p;
        // domain the queue belongs to

        bmqp_ctrlmsg::QueueHandleParameters d_handleParameters;
        // parameters requested for the open
        // queue

        unsigned int d_upstreamSubQueueId;
        // upstream subQueueId
        // (bmqp::QueueId
        // ::k_UNASSIGNED_SUBQUEUE_ID
        // if unassigned)

        mqbi::Cluster::OpenQueueCallback d_callback;
        // Callback to invoke when the queue is
        // opened (whether success or failure).
    };

    /// Structure representing all information and context associated to a
    /// queue, whether the queue is opened, being opened, or just aware due
    /// to a leader advisory message.
    struct QueueContext {
      public:
        // PUBLIC DATA
        QueueLiveState d_liveQInfo;
        // Live queue-related information

        ClusterStateQueueInfoCSp d_stateQInfo_sp;
        // Persistent queue information (null if no
        // queue created)

      private:
        // DATA
        const bmqt::Uri d_uri;
        // Queue uri

      public:
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(QueueContext, bslma::UsesBslmaAllocator)

        // CREATORS

        /// Create a new object representing the queue identified by the
        /// specified `uri` and using the specified `allocator`.
        QueueContext(const bmqt::Uri& uri, bslma::Allocator* allocator);

        /// Copy constructor from the specified `other` using the optionally
        /// specified `allocator`.
        QueueContext(const QueueContext& other,
                     bslma::Allocator*   allocator = 0);

        // ACCESSORS

        /// Return the queue uri associated with this object.
        const bmqt::Uri& uri() const;

        /// Return the queue key associated with this object.
        const mqbu::StorageKey& key() const;

        /// Return the partition id associated with this object.
        int partitionId() const;
    };

    typedef bsl::shared_ptr<QueueContext> QueueContextSp;

    /// Map owning the QueueContexts, indexed by queue Uri.
    typedef bsl::unordered_map<bmqt::Uri, QueueContextSp> QueueContextMap;
    typedef QueueContextMap::iterator                     QueueContextMapIter;
    typedef QueueContextMap::const_iterator QueueContextMapConstIter;

    /// QueueContextByIdMap[queueId] -> queueContext*; only used when remote
    /// queue which have a proper valid unique queueId.
    typedef bsl::unordered_map<int, QueueContext*> QueueContextByIdMap;

    typedef AppIdInfos::const_iterator AppIdInfosCIter;

    typedef mqbc::ClusterStateQueueInfo::AppIdInfos AppIdInfos;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    unsigned int d_nextQueueId;
    // Not atomic, manipulated only in
    // dispatcher thread.

    mqbc::ClusterData* d_clusterData_p;
    // The non-persistent state of a
    // cluster.

    mqbc::ClusterState* d_clusterState_p;
    // The state of the cluster associated
    // to this object

    mqbi::Cluster* d_cluster_p;
    // Just a shortcut alias to
    // d_clusterState_p->cluster()

    mqbi::ClusterStateManager* d_clusterStateManager_p;
    // Cluster state manager to use

    mqbi::StorageManager* d_storageManager_p;
    // Storage manager to use

    QueueContextMap d_queues;  // Map of all queues

    QueueContextByIdMap d_queuesById;
    // Queues indexed by queueId.  Note
    // that this map is only populated with
    // the queues which are not local,
    // since local queues all have a 0 id.

    bsls::AtomicInt d_numPendingReopenQueueRequests;
    // Number of requests that have been
    // sent to reopen the queues after
    // active node switch or primary
    // switch.  This variable is
    // incremented when an open-queue
    // request is sent, but decremented
    // only upon receiving configure queue
    // response.  Additionally, this
    // counter is never explicitly set to
    // zero.  We rely on all response
    // callbacks being fired (success,
    // error or cancel), where we decrement
    // this variable.

    bool d_primaryNotLeaderAlarmRaised;
    // Whether the alarm for primary and
    // leader nodes being different has
    // been raised at least once when
    // gc'ing expired queues.  This is
    // important because we only want to
    // raise such alarm once.

    StopContexts d_stopContexts;

  private:
    // PRIVATE MANIPULATORS

    /// Return the id to use for a new queue, monitoring and alarming when
    /// it reaches some limits.
    unsigned int getNextQueueId();

    /// Get the next subQueueId for a subStream of the queue corresponding
    /// to the specified `context`.
    unsigned int getNextSubQueueId(OpenQueueContext* context);

    /// Invoked after the specified `partitionId` gets assigned to the
    /// specified `primary` with the specified `status`.  Note that null is
    /// a valid value for the `primary`, and it implies that there is no
    /// primary for that partition.  Also note that this method will be
    /// invoked when the `primary` or the `status` or both change.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    afterPartitionPrimaryAssignment(int                  partitionId,
                                    mqbnet::ClusterNode* primary,
                                    bmqp_ctrlmsg::PrimaryStatus::Value status);

    /// Assign the queue represented by the specified `queueContext`, that
    /// is give it an id and eventually a partition id, by initiating
    /// assignment request communication with the leader.  Return a value
    /// indicating whether the assignment was successful or was definitively
    /// rejected.  This method is called regardless of proxy or member, and
    /// leader or replica and will initiate the proper sequence of operation
    /// based on the role of the current node within the cluster.
    QueueAssignmentResult::Enum
    assignQueue(const QueueContextSp& queueContext);

    /// Called when the specified `uri` is in the process of being assigned.
    /// If the specified `processingPendingRequests` is true, we will
    /// process pending requests on this machine.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onQueueAssigning(const bmqt::Uri& uri,
                          bool             processingPendingRequests);

    /// Called when the queue with the specified `queueInfo` is being
    /// unassigned.  Load into the specified `hasInFlightRequests` whether
    /// there are still in-flight requests for the queue.  Return true on
    /// success, or false on failure.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    ///
    /// TODO_CSL: This is the current workflow which we should be able to
    /// remove after the new workflow via
    /// ClusterQueueHelper::onQueueUnassigned() is stable.
    bool onQueueUnassigning(bool*                          hasInFlightRequests,
                            const bmqp_ctrlmsg::QueueInfo& queueInfo);

    /// Send a queueAssignment request to the leader, requesting assignment
    /// of the queue with the specified `uri`.  This method is called only
    /// on a non leader node of a cluster member, for a cluster having a
    /// leader.
    void requestQueueAssignment(const bmqt::Uri& uri);

    /// QueueAssignment request response handler, for a queue with the
    /// specified `uri`, and with the request and its associated response in
    /// the specified `requestContext`.
    void onQueueAssignmentResponse(
        const RequestManagerType::RequestSp& requestContext,
        const bmqt::Uri&                     uri,
        mqbnet::ClusterNode*                 responder);

    /// Send a failure response for the pending contexts associated to the
    /// states in the specified `rejected` vector.  Also remove the
    /// associated queues from `d_queues`.
    void processRejectedQueueAssignments(
        const bsl::vector<QueueContext*>& rejected);

    /// Method invoked when the queue in the specified `queueContext` has
    /// been assigned; to resume the operation on any pending contexts.
    void onQueueContextAssigned(const QueueContextSp& queueContext);

    /// Process pending Close requests, if any upon Reopen response.
    void processPendingClose(QueueContextSp       queueContext,
                             StreamsMap::iterator sqit);

    /// Process pending contexts, if any, from the specified `queueContext`.
    void processPendingContexts(const QueueContextSp& queueContext);

    /// Process the open queue request represented by the specified
    /// `context`: that is, depending on the cluster mode and queue
    /// assignment, either send an open queue request of create the queue.
    /// The queue must have been assigned at this point.
    void processOpenQueueRequest(const OpenQueueContext& context);

    /// Send an open queue request for the queue and its associated
    /// parameter as contained in the specified `context` to the primary
    /// node in charge of the queue.  The queue must have been assigned at
    /// this point, and the current machine must either be a proxy, or not
    /// the primary of the queue.
    void sendOpenQueueRequest(const OpenQueueContext& context);

    /// Send an open queue request for the queue and its associated
    /// parameters as contained in the specified `requestContext` to the
    /// specified `activeNode` having the specified `generationCount` acting
    /// as the active node in charge of the queue, and return the status
    /// code of sending the request.  The queue must have been successfully
    /// assigned at this point, and the current machine must either be a
    /// proxy, or not the primary of the queue.
    ///
    /// THREAD: This method is called from the Cluster's dispatcher thread.
    bmqt::GenericResult::Enum
    sendReopenQueueRequest(const RequestManagerType::RequestSp& requestContext,
                           mqbnet::ClusterNode*                 activeNode,
                           bsls::Types::Uint64 generationCount);

    /// Assign the upstream subQueueId in the specified `context`.  If the
    /// queue has already been opened with the appId in the `context`,
    /// assign the upstream subQueueId which was previously generated for
    /// that appId.  Otherwise, generate and assign new unique id.
    void assignUpstreamSubqueueId(OpenQueueContext* context);

    /// Response callback of an open queue request, in the specified
    /// `context` and with the request and its associated response in the
    /// specified `requestContext`.
    void
    onOpenQueueResponse(const RequestManagerType::RequestSp& requestContext,
                        const OpenQueueContext&              context,
                        mqbnet::ClusterNode*                 responder);

    /// Response callback of an open queue request, that was sent due to the
    /// state being restored, with the request and its associated response
    /// in the specified `requestContext`.
    void
    onReopenQueueResponse(const RequestManagerType::RequestSp& requestContext,
                          mqbnet::ClusterNode*                 activeNode,
                          bsls::Types::Uint64                  generationCount,
                          int                                  numAttempts);

    /// Response callback of a configure queue request, that was sent due to
    /// the state being restored, with the request and its associated
    /// response in the specified `requestContext`.
    void onConfigureQueueResponse(
        const RequestManagerType::RequestSp&               requestContext,
        const bmqt::Uri&                                   uri,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        bsls::Types::Uint64                                generationCount,
        const mqbi::QueueHandle::HandleConfiguredCallback& callback);

    void
    onReopenQueueRetry(const RequestManagerType::RequestSp& requestContext,
                       mqbnet::ClusterNode*                 activeNode,
                       bsls::Types::Uint64                  generationCount,
                       int                                  numAttempts);

    void onReopenQueueRetryDispatched(
        const RequestManagerType::RequestSp& requestContext,
        mqbnet::ClusterNode*                 activeNode,
        bsls::Types::Uint64                  generationCount,
        int                                  numAttempts);

    /// Custom deleter of the openQueue confirmationCookie (in the specified
    /// `value`), for an open queue from the specified `request`.
    void onOpenQueueConfirmationCookieReleased(
        mqbi::QueueHandle**                        value,
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters);

    /// Final part of the open queue pipeline for the specified `context`:
    /// create the queue object using the specified `openQueueResponse` and
    /// invoke the requester's callback with the result.  Return true on
    /// success, false otherwise.  Note that `upstreamNode` will be null if
    /// this method is invoked at the primary node; for every other node,
    /// `upstreamNode` will represent the node to which open-queue request
    /// was sent.
    bool createQueue(const OpenQueueContext&                context,
                     const bmqp_ctrlmsg::OpenQueueResponse& openQueueResponse,
                     mqbnet::ClusterNode*                   upstreamNode);

    /// Factory method that will create the right type of queue (whether
    /// RemoteQueue or Queue) based on the current cluster configuration,
    /// for the queue in represented by the specified `context`.  Queue
    /// properties will be validated against routingCfg in the specified
    /// `openQueueResponse`.  Return a pointer to the queue on success, or a
    /// null pointer and populate the specified `errorDescription` on error.
    /// Note that if the queue was already created, the factory will reuse
    /// it instead of creating a new one.
    bsl::shared_ptr<mqbi::Queue> createQueueFactory(
        bsl::ostream&                          errorDescription,
        const OpenQueueContext&                context,
        const bmqp_ctrlmsg::OpenQueueResponse& openQueueResponse);

    void onHandleReleased(const bsl::shared_ptr<mqbi::QueueHandle>& handle,
                          const mqbi::QueueHandleReleaseResult&     result,
                          const bmqp_ctrlmsg::ControlMessage&       request,
                          mqbc::ClusterNodeSession*                 requester);
    void onHandleReleasedDispatched(
        const bsl::shared_ptr<mqbi::QueueHandle>& handle,
        const mqbi::QueueHandleReleaseResult&     result,
        const bmqp_ctrlmsg::ControlMessage&       request,
        mqbc::ClusterNodeSession*                 requester);

    void
         onHandleConfigured(const bmqp_ctrlmsg::Status&           status,
                            const bmqp_ctrlmsg::StreamParameters& streamParameters,
                            const bmqp_ctrlmsg::ControlMessage&   request,
                            mqbc::ClusterNodeSession*             requester);
    void onHandleConfiguredDispatched(
        const bmqp_ctrlmsg::Status&           status,
        const bmqp_ctrlmsg::StreamParameters& streamParameters,
        const bmqp_ctrlmsg::ControlMessage&   request,
        mqbc::ClusterNodeSession*             requester);

    void onGetDomain(const bmqp_ctrlmsg::Status&         status,
                     mqbi::Domain*                       domain,
                     const bmqp_ctrlmsg::ControlMessage& request,
                     mqbc::ClusterNodeSession*           requester,
                     const int                           peerInstanceId);

    /// Callback invoked in response to an open domain query (in the
    /// specified `request`) made to the domain factory on behalf of the
    /// specified `requester` with the specified `peerInstanceId`.  If the
    /// specified `status` is SUCCESS, the request was success and the
    /// specified `domain` contains a pointer to the result; otherwise
    /// `status` contains the category, error code and description of the
    /// failure.
    void onGetDomainDispatched(const bmqp_ctrlmsg::Status&         status,
                               mqbi::Domain*                       domain,
                               const bmqp_ctrlmsg::ControlMessage& request,
                               mqbc::ClusterNodeSession*           requester,
                               const int peerInstanceId);

    void onGetQueueHandle(
        const bmqp_ctrlmsg::Status&                      status,
        mqbi::QueueHandle*                               queueHandle,
        const bmqp_ctrlmsg::OpenQueueResponse&           openQueueResponse,
        const mqbi::Domain::OpenQueueConfirmationCookie& confirmationCookie,
        const bmqp_ctrlmsg::ControlMessage&              request,
        mqbc::ClusterNodeSession*                        requester,
        const int                                        peerInstanceId);

    /// Callback invoked in response to an open queue request to the domain
    /// (in the specified `request`).  If the specified `status` is SUCCESS,
    /// the request was success and the specified `queueHandle` contains the
    /// handle representing the queue that was allocated for this specified
    /// `requester` session having the specified `peerInstanceId`, and the
    /// specified `confirmationCookie` a cookie that must be set to `true`
    /// to indicate receiving and processing of that response; otherwise
    /// `status` contains the category, error code and description of the
    /// failure.  The `queueHandle` must be released once no longer needed.
    void onGetQueueHandleDispatched(
        const bmqp_ctrlmsg::Status&                      status,
        mqbi::QueueHandle*                               queueHandle,
        const bmqp_ctrlmsg::OpenQueueResponse&           openQueueResponse,
        const mqbi::Domain::OpenQueueConfirmationCookie& confirmationCookie,
        const bmqp_ctrlmsg::ControlMessage&              request,
        mqbc::ClusterNodeSession*                        requester,
        const int                                        peerInstanceId);

    void reconfigureCallback(
        const bmqp_ctrlmsg::Status&           status,
        const bmqp_ctrlmsg::StreamParameters& streamParameters);

    /// Decrement `d_numPendingReopenQueueRequests` counter.  If the counter
    /// drops to 0, `d_stateRestoredFn` if it is set.
    void onResponseToPendingQueueRequest();

    /// Upon completion of queue reopening, if the specified `queueContext`
    /// references a queue, notify the queue about success or failure
    /// indicated by the specified `isOpen`.  The queue either retransmits
    /// pending PUTs and CONFIRMS or NACKs pending PUTs.
    void notifyQueue(QueueContext*       queueContext,
                     unsigned int        upstreamSubQueueId,
                     bsls::Types::Uint64 generationCount,
                     bool                isOpen);

    void configureQueueDispatched(
        const bmqt::Uri&                                   uri,
        unsigned int                                       queueId,
        unsigned int                                       upstreamSubQueueId,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& callback);

    void releaseQueueDispatched(
        const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters,
        unsigned int                                 upstreamSubQueueId,
        const mqbi::Cluster::HandleReleasedCallback& callback);

    void onReleaseQueueResponse(
        const RequestManagerType::RequestSp&         requestContext,
        const mqbi::Cluster::HandleReleasedCallback& callback);

    void onQueueHandleCreatedDispatched(mqbi::Queue*     queue,
                                        const bmqt::Uri& uri,
                                        bool             handleCreated);

    void onQueueHandleDestroyedDispatched(mqbi::Queue*     queue,
                                          const bmqt::Uri& uri);

    bool sendConfigureQueueRequest(
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        int                                                queueId,
        const bmqt::Uri&                                   uri,
        const mqbi::QueueHandle::HandleConfiguredCallback& callback,
        bool                 isReconfigureRequest,
        mqbnet::ClusterNode* upstreamNode,
        bsls::Types::Uint64  generationCount,
        unsigned int         subId);

    void sendCloseQueueRequest(
        const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters,
        StreamsMap::iterator&                        itSubStream,
        const int                                    pid,
        const mqbi::Cluster::HandleReleasedCallback& callback);

    void sendCloseQueueRequest(
        const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters,
        const mqbi::Cluster::HandleReleasedCallback& callback,
        mqbnet::ClusterNode*                         upstreamNode);

    bool subtractCounters(
        QueueLiveState*                            qinfo,
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
        StreamsMap::iterator&                      itSubStream);

    /// Method invoked when there is a change of leader or primary or self
    /// status, in order to restore any state for the specified
    /// `partitionId` (i.e., resume the requests that were issued, reissue
    /// open queues to the new in charge nodes).  Note that if `partitionId`
    /// is equal to `mqbs::DataStore::k_ANY_PARTITION_ID`, an attempt is
    /// made to restore state for all partitions.
    ///
    /// THREAD: This method is called from the Cluster's dispatcher thread.
    void restoreState(int partitionId);

    void restoreStateRemote();

    void restoreStateCluster(int partitionId);

    bmqt::GenericResult::Enum
    restoreStateHelper(QueueLiveState&      queueInfo,
                       mqbnet::ClusterNode* activeNode,
                       bsls::Types::Uint64  generationCount);

    void cancelAllTimers(QueueContext* queueContext);

    void deleteQueue(QueueContext* queueContext);

    void removeQueue(const QueueContextMapIter& it);

    void removeQueueRaw(const QueueContextMapIter& it);

    /// Invoked when the upstream connection (primary node in replica mode,
    /// active node in proxy) for the specified `partitionId` has changed
    /// availability.  Notify all affected queues.
    void onUpstreamNodeChange(mqbnet::ClusterNode* node, int partitionId);

    void deconfigureQueues(const bsl::shared_ptr<StopContext>& contextSp,
                           const bsl::vector<int>*             partitions);
    void deconfigureUri(const bsl::shared_ptr<StopContext>& contextSp,
                        const bmqt::Uri&                    uri);

    /// First step of StopRequest / CLOSING node advisory processing.  Issue
    /// de-configure-queue request for all affected queues.  If the
    /// specified `partitions` is 0, all queues are affected (the
    /// ClusterProxy case); otherwise affect queues which are assigned to
    /// one of the `partitions`.
    void deconfigureQueue(const bsl::shared_ptr<StopContext>& contextSp,
                          const QueueContextSp&               queueContextSp);

    /// Second step of StopRequest / CLOSING node advisory processing
    /// (after de-configure response).  Start timer to wait the configured
    /// `stopTimeoutMs` is there are any pending PUSH messages to collect
    /// CONFIRMs.
    void continueStopSequence(
        const bsl::shared_ptr<StopContext>&   contextSp,
        const QueueContextSp&                 queueContextSp,
        unsigned int                          subId,
        const bmqp_ctrlmsg::Status&           status,
        const bmqp_ctrlmsg::StreamParameters& streamParameters);

    /// Ping-pong between CLUSTER and QUEUE dispatcher threads.
    void waitForUnconfirmed(const bsl::shared_ptr<StopContext>& contextSp,
                            const QueueContextSp&               queueContextSp,
                            unsigned int                        subId,
                            bsls::TimeInterval&                 t);

    void checkUnconfirmed(const bsl::shared_ptr<StopContext>& contextSp,
                          const QueueContextSp&               queueContextSp,
                          unsigned int                        subId);

    /// Ping-pong between CLUSTER and QUEUE dispatcher threads.
    void checkUnconfirmedQueueDispatched(
        const bsl::shared_ptr<StopContext>& contextSp,
        const QueueContextSp&               queueContextSp,
        unsigned int                        subId);

    void
    waitForUnconfirmedDispatched(const bsl::shared_ptr<StopContext>& contextSp,
                                 const QueueContextSp&     queueContextSp,
                                 unsigned int              subId,
                                 const bsls::TimeInterval& t);

    /// Third step of StopRequest / CLOSING node advisory processing.
    /// Issue close-queue request for the specified `queueSp`.
    void closeQueueDispatched(const bsl::shared_ptr<StopContext>& contextSp,
                              const bsl::shared_ptr<mqbi::Queue>& queueSp,
                              unsigned int                        subId);

    /// Fourth step of StopRequest / CLOSING node advisory processing
    /// (after close response).  Once all queues are done, send
    /// StopResponse.
    void onCloseQueueResponse(const bsl::shared_ptr<StopContext>& contextSp,
                              const bmqp_ctrlmsg::Status&         status);

    /// Send StopResponse to the request in the specified 'context.
    void finishStopSequence(StopContext* context);

    /// Send StopResponse to the request in the specified 'context.
    void finishStopSequenceDispatched(StopContext* context);

    // PRIVATE ACCESSORS

    /// Return true if for the specified `partitionId`, there is currently a
    /// primary, *and* the primary is active, *and* the primary node is
    /// AVAILABLE, *and* it is different from the optionally specified
    /// `otherThan`, false otherwise.  Note that self node could be an
    /// active primary as well.  The behavior is undefined unless
    /// `partitionId >= 0` and `partitionId < partitionsCount`.
    bool hasActiveAvailablePrimary(int                  partitionId,
                                   mqbnet::ClusterNode* otherThan = 0) const;

    /// Return true if the queue in the specified `queueContext` is
    /// assigned.
    bool isQueueAssigned(const QueueContext& queueContext) const;

    /// Return true if the queue in the specified `queueContext` is assigned
    /// and its associated primary is AVAILABLE and is different from the
    /// optionally specified `otherThan`.
    bool isQueuePrimaryAvailable(const QueueContext&  queueContext,
                                 mqbnet::ClusterNode* otherThan = 0) const;

    /// Return true if self is primary for the specified `partitionId` *and*
    /// the self node status is AVAILABLE.
    bool isSelfAvailablePrimary(int partitionId) const;

    void loadUpstreamAndGenCount(mqbnet::ClusterNode** upstreamNode,
                                 bsls::Types::Uint64*  genCount,
                                 int                   partitionId) const;

    bool setStopContext(const mqbnet::ClusterNode*          clusterNode,
                        const bsl::shared_ptr<StopContext>& contextSp);

    // PRIVATE MANIPULATORS
    //   (virtual: mqbc::ClusterMembershipObserver)

    /// Callback invoked when self node's status changes to the specified
    /// `value`.
    virtual void onSelfNodeStatus(bmqp_ctrlmsg::NodeStatus::Value value)
        BSLS_KEYWORD_OVERRIDE;

    // PRIVATE MANIPULATORS
    //   (virtual: mqbc::ElectorInfoObserver)

    /// Callback invoked when the cluster's leader changes to the specified
    /// `node` with the specified `status`.  Note that null is a valid value
    /// for the `node`, and it implies that the cluster has transitioned to
    /// a state of no leader, and in this case, `status` will be
    /// `UNDEFINED`.
    virtual void onClusterLeader(mqbnet::ClusterNode*                node,
                                 mqbc::ElectorInfoLeaderStatus::Enum status)
        BSLS_KEYWORD_OVERRIDE;

    // PRIVATE MANIPULATORS
    //   (virtual: mqbc::ClusterStateObserver)

    /// Callback invoked when a queue with the specified `info` gets
    /// assigned to the cluster.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onQueueAssigned(const mqbc::ClusterStateQueueInfo& info)
        BSLS_KEYWORD_OVERRIDE;

    /// Callback invoked when a queue with the specified `info` gets
    /// unassigned from the cluster.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onQueueUnassigned(const mqbc::ClusterStateQueueInfo& info)
        BSLS_KEYWORD_OVERRIDE;

    /// Callback invoked when a queue with the specified `uri` belonging to
    /// the specified `domain` is updated with the optionally specified
    /// `addedAppIds` and `removedAppIds`.  If the specified `uri` is empty,
    /// the appId updates are applied to the entire `domain` instead.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onQueueUpdated(const bmqt::Uri&   uri,
                                const bsl::string& domain,
                                const AppIdInfos&  addedAppIds,
                                const AppIdInfos& removedAppIds = AppIdInfos())
        BSLS_KEYWORD_OVERRIDE;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    ClusterQueueHelper(const ClusterQueueHelper&);             // = delete;
    ClusterQueueHelper& operator=(const ClusterQueueHelper&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterQueueHelper,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object associated to the specified `clusterData` and
    /// `clusterState`, and using the specified `clusterStateManager` and
    /// `allocator`.
    ClusterQueueHelper(mqbc::ClusterData*         clusterData,
                       mqbc::ClusterState*        clusterState,
                       mqbi::ClusterStateManager* clusterStateManager,
                       bslma::Allocator*          allocator);

    /// Destructor
    ~ClusterQueueHelper() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Initialize this object.
    void initialize();

    /// Paired operation of the `initialize()`, undo any action that was
    /// performed during `initialize` and restore that object to a default
    /// constructed state.
    void teardown();

    /// Initiate the open queue sequence for the queue having the specified
    /// `uri`, on the specified `domain` and using the specified
    /// `parameters`.  Use the information in the specified `clientContext`
    /// to identify the source of the request, and invoke the specified
    /// `callback` when done (whether success or failure).
    void openQueue(const bmqt::Uri&                           uri,
                   mqbi::Domain*                              domain,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                           clientContext,
                   const mqbi::Cluster::OpenQueueCallback& callback);

    void configureQueue(
        mqbi::Queue*                                       queue,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        unsigned int                                       upstreamSubQueueId,
        const mqbi::QueueHandle::HandleConfiguredCallback& callback);

    void
    configureQueue(mqbi::Queue*                               queue,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   unsigned int upstreamSubQueueId,
                   const mqbi::Cluster::HandleReleasedCallback& callback);

    void onQueueHandleCreated(mqbi::Queue*     queue,
                              const bmqt::Uri& uri,
                              bool             handleCreated);

    void onQueueHandleDestroyed(mqbi::Queue* queue, const bmqt::Uri& uri);

    // Only used by Cluster
    // - - - - - - - - - -

    /// Set the storage manager to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ClusterQueueHelper& setStorageManager(mqbi::StorageManager* value);

    /// Process the open queue in the specified `request` received from the
    /// specified `requester`.
    void
    processPeerOpenQueueRequest(const bmqp_ctrlmsg::ControlMessage& request,
                                mqbc::ClusterNodeSession*           requester);

    /// Process the configure queue stream request in the specified
    /// `request` received from the specified `requester`.
    void processPeerConfigureStreamRequest(
        const bmqp_ctrlmsg::ControlMessage& request,
        mqbc::ClusterNodeSession*           requester);

    /// Process the close queue in the specified `request` received from the
    /// specified `requester`.
    void
    processPeerCloseQueueRequest(const bmqp_ctrlmsg::ControlMessage& request,
                                 mqbc::ClusterNodeSession* requester);

    /// Delete and unregister all queues which have no clients.
    void processShutdownEvent();

    /// Garbage-collect all queues which meet the criteria, and have
    /// expired.  If the optionally specified `immediate` flag is true,
    /// delete the qualified queues immediately instead of marking them for
    /// deletion in future.
    /// Optionally specify a command result object to populate if there is an
    /// error.
    void gcExpiredQueues(bool immediate = false, mqbcmd::ClusterResult* result = nullptr);

    ClusterQueueHelper& setOnQueueAssignedCb(const OnQueueAssignedCb& value);

    /// Set the corresponding member to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ClusterQueueHelper&
    setOnQueueUnassignedCb(const OnQueueUnassignedCb& value);

    /// Start executing multi-step processing of StopRequest or CLOSING node
    /// advisory received from the specified `clusterNode`.   In the case of
    /// StopRequest the specified `request` references the request; in the
    /// advisory case, it is 0.
    /// The steps for each queue are:
    ///  1. De-configure the queue to stop upstream PUSHing.
    ///  2. Wait the configured `stopTimeoutMs` is there are any pending
    ///     PUSH messages to collect CONFIRMs.
    ///  3. Close the queue.
    ///  4. Send StopResponse.
    /// The optionally specified `partitions` filters which queues will be
    /// de-configured and closed.  Invoke the optionally specified 'callback
    /// upon completion of (asynchronous) processing of all queues.
    void processNodeStoppingNotification(
        mqbnet::ClusterNode*                clusterNode,
        const bmqp_ctrlmsg::ControlMessage* request,
        const bsl::vector<int>*             partitions = 0,
        const VoidFunctor&                  callback   = VoidFunctor());

    void onLeaderAvailable();
    // Called upon leader becoming available.

    // ACCESSORS

    /// Return the queue having the specified `id`, or a null pointer if no
    /// such queue is found.
    mqbi::Queue* lookupQueue(int id) const;

    /// Load to the specified `out` object information about all queues that
    /// are currently known in the cluster and their associated metadata.
    void loadQueuesInfo(mqbcmd::StorageContent* out) const;

    /// Return true if this object is in the process of restoring its
    /// state; that is reopening the queues which were previously opened
    /// before a failover (active node switch, primary switch, ...).
    bool isFailoverInProgress() const;

    /// Return the number of currently pending reopen-queue requests.
    int numPendingReopenQueueRequests() const;

    /// Dump the internal state of this object to the specified
    /// `clusterQueueHelper` object.
    void loadState(mqbcmd::ClusterQueueHelper* clusterQueueHelper) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------------------
// class ClusterQueueHelper::StopContext
// -------------------------------------

inline ClusterQueueHelper::StopContext::StopContext(
    mqbnet::ClusterNode* source,
    const VoidFunctor&   callback,
    int                  timeoutMs,
    bslma::Allocator*    allocator)
: d_peer(source)
, d_response(allocator)
, d_callback(callback)
, d_stopTime(bsls::SystemTime::now(bsls::SystemClockType::e_MONOTONIC))
, d_previous_sp()
{
    d_stopTime.addMilliseconds(timeoutMs);
}

// ---------------------------------------
// struct ClusterQueueHelper::QueueContext
// ---------------------------------------

inline ClusterQueueHelper::QueueContext::QueueContext(
    const bmqt::Uri&  uri,
    bslma::Allocator* allocator)
: d_liveQInfo(allocator)
, d_stateQInfo_sp(0)
, d_uri(uri, allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(uri.asString() == uri.canonical() &&
                     "'uri' must be the canonical URI");
}

inline ClusterQueueHelper::QueueContext::QueueContext(
    const ClusterQueueHelper::QueueContext& other,
    bslma::Allocator*                       allocator)
: d_liveQInfo(other.d_liveQInfo, allocator)
, d_stateQInfo_sp(other.d_stateQInfo_sp)
, d_uri(other.d_uri, allocator)
{
    // NOTHING
}

// ACCESSORS
inline const bmqt::Uri& ClusterQueueHelper::QueueContext::uri() const
{
    return d_uri;
}

inline const mqbu::StorageKey& ClusterQueueHelper::QueueContext::key() const
{
    return d_stateQInfo_sp ? d_stateQInfo_sp->key()
                           : mqbu::StorageKey::k_NULL_KEY;
}

inline int ClusterQueueHelper::QueueContext::partitionId() const
{
    return d_stateQInfo_sp ? d_stateQInfo_sp->partitionId()
                           : mqbs::DataStore::k_INVALID_PARTITION_ID;
}

// ------------------------
// class ClusterQueueHelper
// ------------------------

inline bool ClusterQueueHelper::hasActiveAvailablePrimary(
    int                  partitionId,
    mqbnet::ClusterNode* otherThan) const
{
    const ClusterStatePartitionInfo& pinfo = d_clusterState_p->partition(
        partitionId);

    if (0 == pinfo.primaryNode() || otherThan == pinfo.primaryNode()) {
        return false;  // RETURN
    }

    if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE != pinfo.primaryStatus()) {
        return false;  // RETURN
    }

    if (d_cluster_p->isFSMWorkflow()) {
        return true;  // RETURN
    }

    mqbc::ClusterNodeSession* ns =
        d_clusterData_p->membership().getClusterNodeSession(
            pinfo.primaryNode());
    BSLS_ASSERT_SAFE(ns);

    return bmqp_ctrlmsg::NodeStatus::E_AVAILABLE == ns->nodeStatus();
}

inline bool
ClusterQueueHelper::isQueueAssigned(const QueueContext& queueContext) const
{
    if (d_cluster_p->isRemote()) {
        return queueContext.d_liveQInfo.d_id !=
               bmqp::QueueId::k_UNASSIGNED_QUEUE_ID;  // RETURN
    }

    DomainStatesCIter domCit = d_clusterState_p->domainStates().find(
        queueContext.uri().qualifiedDomain());
    if (domCit == d_clusterState_p->domainStates().cend()) {
        return false;  // RETURN
    }

    UriToQueueInfoMapCIter qCit = domCit->second->queuesInfo().find(
        queueContext.uri());
    if (qCit == domCit->second->queuesInfo().cend()) {
        return false;  // RETURN
    }

    BSLS_ASSERT_SAFE(qCit->second->partitionId() !=
                         mqbs::DataStore::k_INVALID_PARTITION_ID &&
                     !qCit->second->key().isNull());
    return true;
}

inline bool ClusterQueueHelper::isQueuePrimaryAvailable(
    const QueueContext&  queueContext,
    mqbnet::ClusterNode* otherThan) const
{
    if (d_cluster_p->isRemote()) {
        // For a remote cluster, the queue's primary is available if the queue
        // is assigned and the cluster has a leader (in this situation, the
        // leader is the active node of the proxy cluster).
        return queueContext.d_liveQInfo.d_id !=
                   bmqp::QueueId::k_UNASSIGNED_QUEUE_ID &&
               d_clusterData_p->electorInfo().leaderNode() != 0 &&
               d_clusterData_p->electorInfo().leaderNode() != otherThan;
        // RETURN
    }

    // For a cluster member, a queue's primary is available if queue is
    // assigned to a valid partition, that partition has a primary, and the
    // primary is active.

    const int partitionId = queueContext.partitionId();

    return partitionId != mqbs::DataStore::k_INVALID_PARTITION_ID &&
           hasActiveAvailablePrimary(partitionId, otherThan);
}

inline bool ClusterQueueHelper::isSelfAvailablePrimary(int partitionId) const
{
    if (!d_clusterState_p->isSelfPrimary(partitionId)) {
        return false;  // RETURN
    }

    if (d_cluster_p->isFSMWorkflow()) {
        return true;  // RETURN
    }

    return bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
           d_clusterData_p->membership().selfNodeStatus();
}

inline void
ClusterQueueHelper::loadUpstreamAndGenCount(mqbnet::ClusterNode** upstreamNode,
                                            bsls::Types::Uint64*  genCount,
                                            int partitionId) const
{
    if (d_cluster_p->isRemote()) {
        *upstreamNode = d_clusterData_p->electorInfo().leaderNode();
        *genCount     = d_clusterData_p->electorInfo().electorTerm();
    }
    else {
        const ClusterStatePartitionInfo& pinfo = d_clusterState_p->partition(
            partitionId);
        *upstreamNode = pinfo.primaryNode();
        *genCount     = pinfo.primaryLeaseId();
    }
}

inline ClusterQueueHelper&
ClusterQueueHelper::setStorageManager(mqbi::StorageManager* value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!value || !d_storageManager_p);
    // Prevent setting it twice, but allow to unset.

    d_storageManager_p = value;
    return *this;
}

inline mqbi::Queue* ClusterQueueHelper::lookupQueue(int id) const
{
    QueueContextByIdMap::const_iterator it = d_queuesById.find(id);
    return (it == d_queuesById.end()
                ? 0
                : it->second->d_liveQInfo.d_queue_sp.get());
}

inline bool ClusterQueueHelper::isFailoverInProgress() const
{
    return d_numPendingReopenQueueRequests != 0;
}

inline int ClusterQueueHelper::numPendingReopenQueueRequests() const
{
    return d_numPendingReopenQueueRequests;
}

}  // close package namespace
}  // close enterprise namespace

#endif
