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

// mqbblp_cluster.h                                                   -*-C++-*-
#ifndef INCLUDED_MQBBLP_CLUSTER
#define INCLUDED_MQBBLP_CLUSTER

//@PURPOSE:
//
//@CLASSES:
//  mqbblp::Cluster:
//
//@DESCRIPTION:
//
/// Thread Safety
///-------------
//

// MQB

#include <mqbblp_clusterorchestrator.h>
#include <mqbblp_clusterstatemonitor.h>
#include <mqbc_clusterdata.h>
#include <mqbc_clustermembership.h>
#include <mqbc_clusternodesession.h>
#include <mqbc_clusterstate.h>
#include <mqbc_electorinfo.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbnet_cluster.h>
#include <mqbnet_multirequestmanager.h>
#include <mqbnet_session.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_exit.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// MWC
#include <mwcio_channel.h>
#include <mwcma_countingallocatorstore.h>
#include <mwcst_statcontextuserdata.h>
#include <mwcsys_statmonitorsnapshotrecorder.h>
#include <mwcu_operationchain.h>
#include <mwcu_throttledaction.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class FixedThreadPool;
}
namespace mqbcmd {
class ClusterCommand;
}
namespace mqbcmd {
class ClusterResult;
}
namespace mqbcmd {
class ElectorInfo;
}
namespace mqbcmd {
class NodeStatuses;
}
namespace mqbcmd {
class PartitionsInfo;
}
namespace mqbcmd {
class StorageContent;
}
namespace mqbi {
class StorageManager;
}
namespace mqbnet {
class TransportManager;
}

namespace mqbblp {
// =============
// class Cluster
// =============

class Cluster : public mqbi::Cluster,
                public mqbnet::SessionEventProcessor,
                public mqbc::ElectorInfoObserver,
                public mqbnet::ClusterObserver,
                public mqbc::ClusterStateObserver {
    // TBD

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.CLUSTER");

  public:
    // TYPES

    /// Pool of shared pointers to Blobs
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

  private:
    // PRIVATE TYPES
    typedef mqbi::Storage::AppIdKeyPairs AppIdKeyPairs;

    typedef mqbc::ClusterStatePartitionInfo ClusterStatePartitionInfo;

    typedef mqbc::ClusterStateQueueInfo::AppIdInfos AppIdInfos;

    typedef mqbc::ClusterMembership::ClusterNodeSessionSp ClusterNodeSessionSp;

    typedef mqbc::ClusterMembership::ClusterNodeSessionMapIter
        ClusterNodeSessionMapIter;

    typedef mqbc::ClusterMembership::ClusterNodeSessionMapConstIter
        ClusterNodeSessionMapConstIter;

    typedef bslma::ManagedPtr<mqbi::StorageManager> StorageManagerMp;

    typedef mqbnet::Cluster::NodesList NodeList;

    typedef NodeList::iterator NodeListIter;

    typedef NodeList::const_iterator NodeListConstIter;

    typedef mqbi::Cluster::OpenQueueCallback OpenQueueCallback;

    typedef mqbc::ClusterNodeSession::QueueState QueueState;

    typedef mqbc::ClusterNodeSession::StreamsMap StreamsMap;

    typedef mqbc::ClusterNodeSession::QueueHandleMap QueueHandleMap;

    typedef mqbc::ClusterNodeSession::QueueHandleMapIter QueueHandleMapIter;

    typedef bdlmt::EventScheduler::EventHandle SchedulerEventHandle;

    typedef bslma::ManagedPtr<mwcst::StatContext> StatContextMp;

    typedef bsl::shared_ptr<mwcst::StatContext> StatContextSp;

    typedef mqbc::ClusterData::RequestManagerType RequestManagerType;

    typedef mqbc::ClusterData::MultiRequestManagerType MultiRequestManagerType;

    typedef MultiRequestManagerType::RequestContextSp RequestContextSp;

    typedef MultiRequestManagerType::NodeResponsePair NodeResponsePair;

    typedef MultiRequestManagerType::NodeResponsePairs NodeResponsePairs;

    typedef MultiRequestManagerType::NodeResponsePairsIter
        NodeResponsePairsIter;

    typedef MultiRequestManagerType::NodeResponsePairsConstIter
        NodeResponsePairsConstIter;

    typedef bsl::shared_ptr<mqbnet::Cluster> NetClusterSp;

    typedef bsl::function<void(void)> VoidFunctor;

    /// Shortening type alias.
    typedef bdlmt::EventScheduler::RecurringEventHandle RecurringEventHandle;

    /// Map of stat context names to StatContext pointers
    typedef bsl::unordered_map<bsl::string, mwcst::StatContext*>
        StatContextsMap;

    /// Type of the MultiRequestManager used by the cluster to send
    /// StopRequest.
    typedef mqbnet::MultiRequestManager<bmqp_ctrlmsg::ControlMessage,
                                        bmqp_ctrlmsg::ControlMessage,
                                        bsl::shared_ptr<mqbnet::Session> >
        StopRequestManagerType;

    /// Vector of shared_ptrs to Session objects.
    typedef bsl::vector<bsl::shared_ptr<mqbnet::Session> > SessionSpVec;

    /// Type of the stop request callback.
    typedef bsl::function<void(
        const StopRequestManagerType::RequestContextSp& contextSp)>
        StopRequestCompletionCallback;

    typedef bsl::function<void()> CompletionCallback;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    mwcma::CountingAllocatorStore d_allocators;
    // Allocator store to spawn new
    // allocators for sub-components

    bool d_isStarted;
    // Flag to indicate start/stop status
    // This flag is used only inside this
    // component.

    bool d_isStopping;
    // Flag to indicate if this cluster is
    // stopping.  This flag is exposed via
    // an accessor.

    mqbc::ClusterData d_clusterData;
    // The transient data of the cluster

    mqbc::ClusterState d_state;
    // Cluster's persistent state

    StorageManagerMp d_storageManager_mp;
    // StorageManager associated with this
    // cluster.

    ClusterOrchestrator d_clusterOrchestrator;

    ClusterStateMonitor d_clusterMonitor;
    // Cluster state monitor

    mwcu::ThrottledActionParams d_throttledFailedPutMessages;
    // Throttling parameters for failed PUT
    // messages.

    mwcu::ThrottledActionParams d_throttledSkippedPutMessages;
    // Throttling parameters for dropped
    // PUT messages.

    mwcu::ThrottledActionParams d_throttledFailedAckMessages;
    // Throttling parameters for failed ACK
    // messages.

    mwcu::ThrottledActionParams d_throttledDroppedAckMessages;
    // Throttling parameters for dropped
    // ACK messages.

    mwcu::ThrottledActionParams d_throttledFailedConfirmMessages;
    // Throttling parameters for failed
    // CONFIRM messages.

    mwcu::ThrottledActionParams d_throttledFailedRejectMessages;
    // Throttling parameters for failed
    // REJECT messages.

    mwcu::ThrottledActionParams d_throttledDroppedConfirmMessages;
    // Throttling parameters for dropped
    // CONFIRM messages.

    mwcu::ThrottledActionParams d_throttledDroppedRejectMessages;
    // Throttling parameters for dropped
    // REJECT messages.

    mwcu::ThrottledActionParams d_throttledFailedPushMessages;
    // Throttling parameters for failed
    // PUSH messages.

    mwcu::ThrottledActionParams d_throttledDroppedPushMessages;
    // Throttling parameters for dropped
    // PUSH messages.

    RecurringEventHandle d_logSummarySchedulerHandle;
    // Scheduler handle for the recurring
    // cluster summary log.

    RecurringEventHandle d_queueGcSchedulerHandle;
    // Scheduler handle for the recurring
    // queue gc check.

    StopRequestManagerType d_stopRequestsManager;

    mwcu::OperationChain d_shutdownChain;
    // Mechanism used for the Cluster
    // graceful shutdown to serialize
    // execution of the shutdown callbacks
    // from the client sessions, stop
    // responses from proxies and nodes,
    // and the cluster's shutdown callback.

    mqbnet::Session::AdminCommandEnqueueCb d_adminCb;

  private:
    // NOT IMPLEMENTED
    Cluster(const Cluster&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    Cluster& operator=(const Cluster&) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Start the `Cluster` and populate the specified `rc` with the result
    /// of the operation, 0 on success and non-zero otherwise populating the
    /// specified `errorDescription` with the reason of the error.
    void startDispatched(bsl::ostream* errorDescription, int* rc);

    /// Stop the `Cluster`.
    void stopDispatched();

    /// Append an ACK message to the session's ack builder, with the
    /// specified `status`, `correlationId`, `messageGUID` and `queueId` to
    /// the specified `destination` node.  The specified `source` is used
    /// when logging, to indicate the origin of the ACK.  The specified
    /// `isSelfGenerated` flag indicates whether the ACK is originally
    /// generated from this object, or just relayed through it.
    void sendAck(bmqt::AckResult::Enum    status,
                 int                      correlationId,
                 const bmqt::MessageGUID& messageGUID,
                 int                      queueId,
                 const bslstl::StringRef& source,
                 mqbnet::ClusterNode*     destination,
                 bool                     isSelfGenerated);

    /// Append an ACK message to the session's ack builder, with the
    /// specified `status`, `correlationId`, `messageGUID` and `queueId` to
    /// the cluster node identified by the specified cluster `nodeSession`.
    /// The specified `source` is used when logging, to indicate the origin
    /// of the ACK.  The specified `isSelfGenerated` flag indicates whether
    /// the ACK is originally generated from this object, or just relayed
    /// through it.
    void sendAck(bmqt::AckResult::Enum     status,
                 int                       correlationId,
                 const bmqt::MessageGUID&  messageGUID,
                 int                       queueId,
                 const bslstl::StringRef&  source,
                 mqbc::ClusterNodeSession* nodeSession,
                 bool                      isSelfGenerated);

    /// Generate a nack with the specified `status` and `nackReason` for a
    /// PUT message having the specified `putHeader` for the specified
    /// `queue` from the specified `source`.  The nack is replied to the
    /// `source`.  The specified `raiseAlarm` flag determines whether an
    /// alarm should be raised for this nack.
    void generateNack(bmqt::AckResult::Enum               status,
                      const bslstl::StringRef&            nackReason,
                      const bmqp::PutHeader&              putHeader,
                      mqbi::Queue*                        queue,
                      DispatcherClient*                   source,
                      const bsl::shared_ptr<bdlbb::Blob>& appData,
                      const bsl::shared_ptr<bdlbb::Blob>& options,
                      bool                                raiseAlarm);

    /// Executed by dispatcher thread.
    void processCommandDispatched(mqbcmd::ClusterResult*        result,
                                  const mqbcmd::ClusterCommand& command);

    /// Executed by dispatcher thread.
    void initiateShutdownDispatched(const VoidFunctor& callback);

    /// Send stop request to proxies and nodes specified in `sessions` using
    /// the specified `stopCb` as a callback to be called once all the
    /// requests get responses.
    void sendStopRequest(const SessionSpVec&                  sessions,
                         const StopRequestCompletionCallback& stopCb);

    /// Continue shutting down upon receipt of all StopResponses.
    void continueShutdown(bsls::Types::Int64        startTimeNs,
                          const CompletionCallback& completionCb);
    void continueShutdownDispatched(bsls::Types::Int64        startTimeNs,
                                    const CompletionCallback& completionCb);

    void processControlMessage(const bmqp_ctrlmsg::ControlMessage& message,
                               mqbnet::ClusterNode*                source);
    void
    processClusterControlMessage(const bmqp_ctrlmsg::ControlMessage& message,
                                 mqbnet::ClusterNode*                source);

    /// Process the ClusterSyncRequest in the specified `request`
    /// originating from the specified `requester`.
    void processClusterSyncRequest(const bmqp_ctrlmsg::ControlMessage& request,
                                   mqbnet::ClusterNode* requester);

    void onPutEvent(const mqbi::DispatcherEvent& event);

    void onRelayPutEvent(const mqbi::DispatcherEvent& event);

    void onAckEvent(const mqbi::DispatcherEvent& event);

    void onRelayAckEvent(const mqbi::DispatcherEvent& event);

    void onConfirmEvent(const mqbi::DispatcherEvent& event);

    void onRelayConfirmEvent(const mqbi::DispatcherEvent& event);

    void onRejectEvent(const mqbi::DispatcherEvent& event);

    void onRelayRejectEvent(const mqbi::DispatcherEvent& event);

    void onPushEvent(const mqbi::DispatcherEvent& event);

    void onRelayPushEvent(const mqbi::DispatcherEvent& event);

    bool validateMessage(mqbi::QueueHandle**       queueHandle,
                         bsl::ostream*             errorStream,
                         const bmqp::QueueId&      queueId,
                         mqbc::ClusterNodeSession* ns,
                         bmqp::EventType::Enum     eventType);
    // Validate a message of the specified 'eventType' using the specified
    // 'queueId' and 'ns'. Return true if the message is valid and false
    // otherwise. Populate the specified 'queueHandle' if the queue is found
    // and load a descriptive error message into the 'errorStream' if the
    // message is invalid.

    bool validateRelayMessage(mqbc::ClusterNodeSession** ns,
                              bsl::ostream*              errorStream,
                              const int                  pid);
    // Validate a relay message using the specified 'pid'. Return true if the
    // message is valid and false otherwise. Populate the specified 'ns' if the
    // message is valid or load a descriptive error message into the
    // 'errorStream' if the message is invalid.

    /// Executes in any thread.
    void
    onRecoveryStatus(int                              status,
                     const bsl::vector<unsigned int>& primaryLeaseIds,
                     const mwcsys::StatMonitorSnapshotRecorder& statRecorder);

    /// Executes in cluster's dispatcher thread.
    void onRecoveryStatusDispatched(
        int                                        status,
        const bsl::vector<unsigned int>&           primaryLeaseIds,
        const mwcsys::StatMonitorSnapshotRecorder& statRecorder);

    /// Executes in the scheduler thread.
    void gcExpiredQueues();

    /// Executes in the cluster dispatcher thread.
    void gcExpiredQueuesDispatched();

    void logSummaryState();

    /// Process incoming proxy connection by sending self status to the
    /// specified `channel` and using the specified `description` if the
    /// specified `identity` supports broadcastring advisories to proxies.
    void onProxyConnectionUpDispatched(
        const bsl::shared_ptr<mwcio::Channel>& channel,
        const bmqp_ctrlmsg::ClientIdentity&    identity,
        const bsl::string&                     description);

    /// Apply the specified `response` from the specified `source` to the
    /// request manager.  This method is invoked in the cluster-dispatcher
    /// thread.
    void
    processResponseDispatched(const bmqp_ctrlmsg::ControlMessage& response,
                              mqbnet::ClusterNode*                source);

    // PRIVATE ACCESSORS

    /// Log a short summary of the core vital information about this
    /// cluster.
    void logSummaryStateDispatched() const;

    void loadNodesInfo(mqbcmd::NodeStatuses* out) const;
    void loadElectorInfo(mqbcmd::ElectorInfo* out) const;
    void loadPartitionsInfo(mqbcmd::PartitionsInfo* out) const;

    /// Load the corresponding information to the specified `out` object.
    ///
    /// THREAD: These methods must be invoked from the DISPATCHER thread.
    void loadQueuesInfo(mqbcmd::StorageContent* out) const;

    /// Execute `initiateShutdown` followed by `stop` and SIGINT
    void terminate(mqbu::ExitCode::Enum reason);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Cluster, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object representing a cluster having the specified
    /// `name`, `clusterConfig` and `statContexts`, associated to the
    /// specified `netCluster` and using the specified `domainFactory`,
    /// `scheduler`, `dispatcher`, `blobSpPool` and `bufferFactory`.  Use
    /// the specified `allocator` for any memory allocation.
    Cluster(const bslstl::StringRef&                      name,
            const mqbcfg::ClusterDefinition&              clusterConfig,
            bslma::ManagedPtr<mqbnet::Cluster>            netCluster,
            const StatContextsMap&                        statContexts,
            mqbi::DomainFactory*                          domainFactory,
            bdlmt::EventScheduler*                        scheduler,
            mqbi::Dispatcher*                             dispatcher,
            BlobSpPool*                                   blobSpPool,
            bdlbb::BlobBufferFactory*                     bufferFactory,
            mqbnet::TransportManager*                     transportManager,
            bslma::Allocator*                             allocator,
            const mqbnet::Session::AdminCommandEnqueueCb& adminCb);

    /// Destructor
    ~Cluster() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::Cluster)

    /// Start the `Cluster`.  Return 0 on success and non-zero otherwise
    /// populating the specified `errorDescription` with the reason of the
    /// error.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Initiate the shutdown of the cluster.  It is expected that `stop()`
    /// will be called soon after this routine is invoked.  Invoke the
    /// specified `callback` upon completion of (asynchronous) shutdown
    /// sequence.
    void initiateShutdown(const VoidFunctor& callback) BSLS_KEYWORD_OVERRIDE;

    /// Stop the `Cluster`.
    void stop() BSLS_KEYWORD_OVERRIDE;

    /// Register the specified `observer` to be notified of cluster state
    /// changes.
    void registerStateObserver(mqbc::ClusterStateObserver* observer)
        BSLS_KEYWORD_OVERRIDE;

    /// Un-register the specified `observer` from being notified of cluster
    /// state changes.
    void unregisterStateObserver(mqbc::ClusterStateObserver* observer)
        BSLS_KEYWORD_OVERRIDE;

    /// Open the queue with the specified `uri`, belonging to the specified
    /// `domain` with the specified `parameters` from a client identified
    /// with the specified `clientContext`.  Invoke the specified `callback`
    /// with the result of the operation (regardless of success or failure).
    void openQueue(const bmqt::Uri&                           uri,
                   mqbi::Domain*                              domain,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                           clientContext,
                   const mqbi::Cluster::OpenQueueCallback& callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Configure the specified `upstreamSubQueueId` subStream of the
    /// specified `queue` with the specified `streamParameters`, and invoke
    /// the specified `callback` when finished.
    void
    configureQueue(mqbi::Queue*                          queue,
                   const bmqp_ctrlmsg::StreamParameters& streamParameters,
                   unsigned int                          upstreamSubQueueId,
                   const mqbi::QueueHandle::HandleConfiguredCallback& callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Configure the specified `upstreamSubQueueId` subStream of the
    /// specified `queue` with the specified `handleParameters` and invoke
    /// the specified `callback` when finished.
    void
    configureQueue(mqbi::Queue*                               queue,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   unsigned int upstreamSubQueueId,
                   const mqbi::Cluster::HandleReleasedCallback& callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Invoked whenever an attempt was made to create a queue handle for
    /// the specified `queue` having the specified `uri`, with
    /// `handleCreated` flag indicating if the handle was created or not.
    void onQueueHandleCreated(mqbi::Queue*     queue,
                              const bmqt::Uri& uri,
                              bool handleCreated) BSLS_KEYWORD_OVERRIDE;

    /// Invoked whenever a queue handle associated with the specified
    /// `queue` having the specified `uri` has been destroyed.
    void onQueueHandleDestroyed(mqbi::Queue*     queue,
                                const bmqt::Uri& uri) BSLS_KEYWORD_OVERRIDE;

    /// Invoked whenever `domain` previously configured having `oldDefn` is
    /// reconfigured with `newDefn`.
    void onDomainReconfigured(const mqbi::Domain&     domain,
                              const mqbconfm::Domain& oldDefn,
                              const mqbconfm::Domain& newDefn)
        BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `command`, and load the result in the
    /// specified `result`. Return 0 if the command was successfully
    /// processed, or a non-zero value otherwise.
    int processCommand(mqbcmd::ClusterResult*        result,
                       const mqbcmd::ClusterCommand& command)
        BSLS_KEYWORD_OVERRIDE;

    /// Send the specified `request` with the specified `timeout` to the
    /// specified `target` node.  If `target` is 0, it is the Cluster's
    /// implementation responsibility to decide which node to use (in
    /// `ClusterProxy` this will be the current `activeNode`, in `Cluster`
    /// this will be the current `leader`).  Return a status category
    /// indicating the result of the send operation.
    bmqt::GenericResult::Enum
    sendRequest(const RequestManagerType::RequestSp& request,
                mqbnet::ClusterNode*                 target,
                bsls::TimeInterval timeout) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `response` message as a response to previously
    /// transmitted request.  This is how cluster receives StopResponse from
    /// a ClusterProxy.
    void processResponse(const bmqp_ctrlmsg::ControlMessage& response)
        BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering modifiable access to the request manager
    /// used by this cluster.
    RequestManagerType& requestManager() BSLS_KEYWORD_OVERRIDE;

    mqbi::Cluster::MultiRequestManagerType&
    multiRequestManager() BSLS_KEYWORD_OVERRIDE;

    /// Load the cluster state to the specified `out` object.
    void loadClusterStatus(mqbcmd::ClusterResult* out) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::SessionEventProcessor)

    /// Process the specified `event` received from the remote peer
    /// represented by the optionally specified `source`.  Note that this
    /// method is the entry point for all incoming events coming from the
    /// remote peer.
    void processEvent(const bmqp::Event&   event,
                      mqbnet::ClusterNode* source = 0) BSLS_KEYWORD_OVERRIDE;

    void onProcessedAdminCommand(
        mqbnet::ClusterNode*                source,
        const bmqp_ctrlmsg::ControlMessage& adminCommandCtrlMsg,
        int                                 rc,
        const bsl::string&                  res);

    // MANIPULATORS
    //   (virtual: mqbi::DispatcherClient)

    /// Called by the `Dispatcher` when it has the specified `event` to
    /// deliver to the client.
    void onDispatcherEvent(const mqbi::DispatcherEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Called by the dispatcher to flush any pending operation.. mainly
    /// used to provide batch and nagling mechanism.
    void flush() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::ClusterObserver)

    /// Notification method to indicate that the specified `node` is now
    /// available (it the specified `isAvailable` is true) or not available
    /// (if `isAvailable` is false).
    void onNodeStateChange(mqbnet::ClusterNode* node,
                           bool isAvailable) BSLS_KEYWORD_OVERRIDE;

    void onNodeLowWatermark(mqbnet::ClusterNode* node);

    void onNodeHighWatermark(mqbnet::ClusterNode* node);

    /// Process incoming proxy connection by sending self status to the
    /// specified `channel` and using the specified `description` if the
    /// specified `identity` supports broadcastring advisories to proxies.
    void
    onProxyConnectionUp(const bsl::shared_ptr<mwcio::Channel>& channel,
                        const bmqp_ctrlmsg::ClientIdentity&    identity,
                        const bsl::string& description) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbc::ElectorInfoObserver)
    void onClusterLeader(mqbnet::ClusterNode*                node,
                         mqbc::ElectorInfoLeaderStatus::Enum status)
        BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbc::ClusterStateObserver)

    /// Callback invoked when the leader node has been perceived as passive
    /// above a certain threshold amount of time.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onLeaderPassiveThreshold() BSLS_KEYWORD_OVERRIDE;

    /// Callback invoked when failover has not completed above a certain
    /// threshold amount of time.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onFailoverThreshold() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::Cluster)

    /// Return the name of this cluster.
    const bsl::string& name() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this cluster is a local cluster.
    bool isLocal() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this cluster is a remote cluster.
    bool isRemote() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this cluster is a `cluster`.
    bool isClusterMember() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this cluster is in the process of restoring its
    /// state; that is reopening the queues which were previously opened
    /// before a failover (active node switch, primary switch, ...).
    bool isFailoverInProgress() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this cluster is stopping *or* has stopped, false
    /// otherwise.  Note that a cluster which has not been started will also
    /// return true.  TBD: this accessor should be replaced by something
    /// like `status()` which should return an enum specify various states
    /// like started/stopped/stopping etc.
    bool isStopping() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference not offering modifiable access to the net cluster
    /// used by this cluster.
    const mqbnet::Cluster& netCluster() const BSLS_KEYWORD_OVERRIDE;

    // Returns a reference to the cluster state describing this cluster
    // const mqbc::ClusterState& clusterState() const BSLS_KEYWORD_OVERRIDE;

    // Gets all the nodes which are a primary for some partition of this
    // cluster and whether or not this node is a primary. The outNodes 
    // vector will never include the self node.
    void getPrimaryNodes(bsl::vector<mqbnet::ClusterNode*>* outNodes,
                         bool* outIsSelfPrimary) const BSLS_KEYWORD_OVERRIDE;

    /// Print the state of the cluster to the specified `out`.
    ///
    /// THREAD: These methods must be invoked from the DISPATCHER thread.
    void printClusterStateSummary(bsl::ostream& out,
                                  int           level          = 0,
                                  int           spacesPerLevel = 0) const
        BSLS_KEYWORD_OVERRIDE;

    /// Return boolean flag indicating if CSL Mode is enabled.
    bool isCSLModeEnabled() const BSLS_KEYWORD_OVERRIDE;

    /// Return boolean flag indicating if CSL FSM workflow is in effect.
    bool isFSMWorkflow() const BSLS_KEYWORD_OVERRIDE;

    const mqbcfg::ClusterDefinition*
    clusterConfig() const BSLS_KEYWORD_OVERRIDE;
    // Returns a pointer to cluster config if this `mqbi::Cluster`
    // represents a cluster, otherwise null.

    const mqbcfg::ClusterProxyDefinition*
    clusterProxyConfig() const BSLS_KEYWORD_OVERRIDE;
    // Returns a pointer to cluster proxy config if this `mqbi::Cluster`
    // represents a proxy, otherwise null.

    // MANIPULATORS
    //   (virtual: mqbi::Cluster)

    /// Return a reference offering modifiable access to the net cluster
    /// used by this cluster.
    mqbnet::Cluster& netCluster() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the dispatcherClientData.
    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;

    /// Return a printable description of the client (e.g. for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (specific to mqbblp::Cluster)

    /// Return a pointer to the storage manager associated to this cluster.
    mqbi::StorageManager* storageManager() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class Cluster
// -------------

inline Cluster::RequestManagerType& Cluster::requestManager()
{
    return d_clusterData.requestManager();
}

inline Cluster::MultiRequestManagerType& Cluster::multiRequestManager()
{
    return d_clusterData.multiRequestManager();
}

inline const bsl::string& Cluster::name() const
{
    return d_clusterData.identity().name();
}

inline bool Cluster::isLocal() const
{
    return 1 == d_clusterData.membership().netCluster()->nodes().size();
}

inline bool Cluster::isRemote() const
{
    return false;
}

inline bool Cluster::isClusterMember() const
{
    return true;
}

inline bool Cluster::isFailoverInProgress() const
{
    return d_clusterOrchestrator.queueHelper().isFailoverInProgress();
}

inline bool Cluster::isStopping() const
{
    return d_isStopping;
}

inline mqbnet::Cluster& Cluster::netCluster()
{
    return *(d_clusterData.membership().netCluster());
}

inline const mqbnet::Cluster& Cluster::netCluster() const
{
    return *(d_clusterData.membership().netCluster());
}

// inline const mqbc::ClusterState& Cluster::clusterState() const
// {
//     return d_state;
// }

inline const mqbcfg::ClusterDefinition* Cluster::clusterConfig() const
{
    return &d_clusterData.clusterConfig();
}

inline const mqbcfg::ClusterProxyDefinition*
Cluster::clusterProxyConfig() const
{
    return 0;
}

inline mqbi::Dispatcher* Cluster::dispatcher()
{
    return dispatcherClientData().dispatcher();
}

inline const mqbi::Dispatcher* Cluster::dispatcher() const
{
    return dispatcherClientData().dispatcher();
}

inline const mqbi::DispatcherClientData& Cluster::dispatcherClientData() const
{
    return d_clusterData.dispatcherClientData();
}

inline mqbi::DispatcherClientData& Cluster::dispatcherClientData()
{
    return d_clusterData.dispatcherClientData();
}

inline const bsl::string& Cluster::description() const
{
    return d_clusterData.identity().description();
}

inline mqbi::StorageManager* Cluster::storageManager() const
{
    return d_storageManager_mp.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
