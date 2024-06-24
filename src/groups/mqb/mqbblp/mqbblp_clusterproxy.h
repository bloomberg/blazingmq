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

// mqbblp_clusterproxy.h                                              -*-C++-*-
#ifndef INCLUDED_MQBBLP_CLUSTERPROXY
#define INCLUDED_MQBBLP_CLUSTERPROXY

//@PURPOSE: Provide a proxy-like mechanism to communicate with a BlazingMQ
//          cluster.
//
//@CLASSES:
//  mqbblp::ClusterProxy : proxy-like mechanism for cluster communication
//
//@DESCRIPTION: 'mqbblp::ClusterProxy' represent a session with a cluster, used
// in the remote proxy: it abstracts the communication with the cluster by
// using one of the nodes (using the 'mqbnet::ClusterNodeActiveManager' to
// select it).  This object uses an underlying 'mqbnet::Cluster' object to
// communicate with the cluster.  At any point, one of the node of the cluster
// (with preference for a node in the same Data Center as this broker is picked
// and used for communication.  When that node goes down, a new node is
// selected and everything is resumed in a seamless manner.
//
/// TBD:
///----
// This current design may lead to uneven distribution: considering a typical
// cluster of 4 nodes, 2 in each data center, bouncing during the week-end.
// When the first Cluster node bounce, all proxy brokers will connect to the
// other node and reopen their queues at the same time; then when that node
// bounce itself, all proxies will use the first node, which will keep all this
// load.  Then only the new connections will 'randomly' evenly use one or
// another node.  We may need to implement some dynamic load balancing where a
// cluster node could ask a proxy to 'go away' to the other node.
//
/// Thread Safety
///-------------
//

// MQB

#include <mqbblp_clusterqueuehelper.h>
#include <mqbblp_clusterstatemonitor.h>
#include <mqbc_clusterdata.h>
#include <mqbc_clusterstate.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbnet_cluster.h>
#include <mqbnet_clusteractivenodemanager.h>
#include <mqbnet_multirequestmanager.h>
#include <mqbnet_session.h>
#include <mqbnet_transportmanager.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// MWC
#include <mwcio_status.h>
#include <mwcst_statcontext.h>
#include <mwcu_operationchain.h>
#include <mwcu_throttledaction.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdld_manageddatum.h>
#include <bsl_deque.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>
#include <bsls_timeinterval.h>
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
class ClusterCommand;
}
namespace mqbi {
class Domain;
}

namespace mqbblp {

// ==================
// class ClusterProxy
// ==================

/// Proxy-like mechanism to communicate with a BlazingMQ cluster.
class ClusterProxy : public mqbc::ClusterStateObserver,
                     public mqbi::Cluster,
                     public mqbnet::ClusterObserver,
                     public mqbnet::SessionEventProcessor {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.CLUSTERPROXY");

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

    class ChannelBuffer {
        bsl::deque<bdlbb::Blob> d_queue;
        bsls::Types::Int64      d_queueBytes;
        // Bytes sum of all blobs pending in
        // the 'd_channelBufferQueue'.
      private:
        // NOT IMPLEMENTED
        ChannelBuffer(const ChannelBuffer&) BSLS_KEYWORD_DELETED;
        ChannelBuffer& operator=(const ChannelBuffer&) BSLS_KEYWORD_DELETED;
        // Copy constructor and assignment operation are not permitted on this
        // object.

      public:
        // CREATORS
        ChannelBuffer(bslma::Allocator* allocator);

        // MANIPULATORS
        void push(const bdlbb::Blob& blob);

        void pop();

        void clear();

        // ACCESSORS
        const bdlbb::Blob& front() const;

        size_t numItems() const;

        size_t bytes() const;
    };

    typedef bslma::ManagedPtr<mwcst::StatContext> StatContextMp;

    typedef bsl::shared_ptr<mwcst::StatContext> StatContextSp;

    typedef bslma::ManagedPtr<bdld::ManagedDatum> ManagedDatumMp;

    typedef bsl::map<mqbnet::ClusterNode*, StatContextSp> NodeStatsMap;

    typedef mqbnet::Cluster::NodesList NodeList;

    typedef NodeList::iterator NodeListIter;

    /// Map of strings to StatContext pointers
    typedef bsl::unordered_map<bsl::string, mwcst::StatContext*>
        StatContextsMap;

    /// Type of the MultiRequestManager used by the cluster proxy to send
    /// StopRequest.
    typedef mqbnet::MultiRequestManager<bmqp_ctrlmsg::ControlMessage,
                                        bmqp_ctrlmsg::ControlMessage,
                                        bsl::shared_ptr<mqbnet::Session> >
        StopRequestManagerType;

    /// Vector of shared_ptrs to Sessoin objects.
    typedef bsl::vector<bsl::shared_ptr<mqbnet::Session> > SessionSpVec;

    /// Type of the stop request callback.
    typedef bsl::function<void(
        const StopRequestManagerType::RequestContextSp& contextSp)>
        StopRequestCompletionCallback;

    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    bool d_isStarted;
    // Flag to indicate start/stop
    // status. This flag is used only
    // inside this component.

    bool d_isStopping;
    // Flag to indicate if this cluster is
    // stopping.  This flag is exposed via
    // an accessor.

    mqbc::ClusterData d_clusterData;
    // The transient data associated with
    // the cluster

    mqbc::ClusterState d_state;
    // Persistent state of the cluster

    mqbnet::ClusterActiveNodeManager d_activeNodeManager;
    // Manager of the active node

    ClusterQueueHelper d_queueHelper;
    // Helper for queue management

    NodeStatsMap d_nodeStatsMap;
    // Map of node to associated stat
    // context.

    mwcu::ThrottledActionParams d_throttledFailedAckMessages;
    // Throttling parameters for failed ACK
    // messages.

    mwcu::ThrottledActionParams d_throttledSkippedPutMessages;
    // Throttling parameters for skipped
    // PUT messages.

    ClusterStateMonitor d_clusterMonitor;
    // Cluster state monitor

    bdlmt::EventScheduler::EventHandle d_activeNodeLookupEventHandle;
    // Scheduler event handle for the
    // initial 'active' node selection (see
    // 'ClusterActiveNodeManager'
    // documentation).

    mwcu::OperationChain d_shutdownChain;
    // Mechanism used for the ClusterProxy
    // graceful shutdown to serialize
    // execution of the shutdown callbacks
    // from the client sessions.

    StopRequestManagerType d_stopRequestsManager;
    // Request manager to send stop
    // requests to connected proxies.

  private:
    // PRIVATE MANIPULATORS

    /// Start the `Cluster`.
    void startDispatched();

    /// Initiate the shutdown of the cluster.  The specified `callback` will
    /// be called when the shutdown is completed.  This routine is invoked
    /// in the cluster-dispatcher thread.
    void initiateShutdownDispatched(const VoidFunctor& callback);

    /// Stop the `Cluster`.
    void stopDispatched();

    /// Process the specified `command`, and load the result to the
    /// specified `result`.
    void processCommandDispatched(mqbcmd::ClusterResult*        result,
                                  const mqbcmd::ClusterCommand& command);

    // PRIVATE MANIPULATORS
    //   (Active node handling)

    /// Executed by the dispatcher thread when the specified `activeNode` is
    /// selected as the new active node.
    void onActiveNodeUp(mqbnet::ClusterNode* activeNode);

    /// Executed by the dispatcher thread when the active node is lost.
    /// The specified `node` is the one which status has changed.
    void onActiveNodeDown(const mqbnet::ClusterNode* node);

    void onNodeUpDispatched(
        mqbnet::ClusterNode*         node,
        BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ClientIdentity& identity);

    // Executed by the dispatcher thread when there is a change in the
    // specified 'node' connectivity.  If not empty, the specified
    // 'session' points to newly connected 'Session'.  Empty 'session'
    // indicates loss of connectivity.

    void onNodeDownDispatched(mqbnet::ClusterNode* node);

    /// Callback method when the `activeNodeLookupEvent` has expired.
    void onActiveNodeLookupTimerExpired();

    /// Callback method when the `activeNodeLookupEvent` has expired.
    void onActiveNodeLookupTimerExpiredDispatched();

    /// Trigger searching for active node if there is none.  Executed on
    /// DISPATCHER thread.
    void refreshActiveNodeManagerDispatched();

    /// Process result of searching for active node.  The specified `result`
    /// is a bit mask of E_NO_CHANGE/E_LOST_ACTIVE/E_NEW_ACTIVE.
    /// The specified `node` is the one which status has changed (if any).
    /// If the `result` is `E_LOST_ACTIVE`, cancel all outstamding requests
    /// sent to the `node`
    void processActiveNodeManagerResult(int                        result,
                                        const mqbnet::ClusterNode* node);

    // PRIVATE MANIPULATORS
    //   (Event processing)

    /// Generate a NACK with the specified `status` for a PUT message having
    /// the specified `putHeader` from the specified `source`.  The nack is
    /// replied to the `source`.  Log the specified `rc` as a reason for the
    /// NACK.
    void generateNack(bmqt::AckResult::Enum               status,
                      const bmqp::PutHeader&              putHeader,
                      DispatcherClient*                   source,
                      const bsl::shared_ptr<bdlbb::Blob>& appData,
                      const bsl::shared_ptr<bdlbb::Blob>& options,
                      bmqt::GenericResult::Enum           rc);

    void onPushEvent(const mqbi::DispatcherPushEvent& event);

    void onAckEvent(const mqbi::DispatcherAckEvent& event);

    void onRelayPutEvent(const mqbi::DispatcherPutEvent& event,
                         mqbi::DispatcherClient*         source);

    void onRelayConfirmEvent(const mqbi::DispatcherConfirmEvent& event);

    void onRelayRejectEvent(const mqbi::DispatcherRejectEvent& event);

    // PRIVATE MANIPULATORS
    //   (virtual: mqbnet::SessionEventProcessor)

    /// Process the specified `event` received from the remote peer
    /// represented by the optionally specified `source`.  Note that this
    /// method is the entry point for all incoming events coming from the
    /// remote peer.
    void processEvent(const bmqp::Event&   event,
                      mqbnet::ClusterNode* source = 0) BSLS_KEYWORD_OVERRIDE;

    // PRIVATE MANIPULATORS
    //   (virtual: mqbi::Cluster)

    /// Return a reference offering modifiable access to the request manager
    /// used by this cluster.
    RequestManagerType& requestManager() BSLS_KEYWORD_OVERRIDE;

    // Return a reference offering modifiable access to the multi request
    // manager used by this cluster.
    mqbi::Cluster::MultiRequestManagerType&
    multiRequestManager() BSLS_KEYWORD_OVERRIDE;

    /// Send the specified `request` with the specified `timeout` to the
    /// specified `target` node.  If `target` is 0, it is the Cluster's
    /// implementation responsibility to decide which node to use (in
    /// `ClusterProxy` this will be the current `activeNode`, in `Cluster`
    /// this will be the current `leader`).  Return a status category
    /// indicating the result of the send operation.  Note that if a non
    /// zero `target` is specified but is different from the upstream node
    /// as perceived by this instance, an error will be returned.
    bmqt::GenericResult::Enum
    sendRequest(const RequestManagerType::RequestSp& request,
                mqbnet::ClusterNode*                 target,
                bsls::TimeInterval timeout) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `response` message as a response to previously
    /// transmitted request.
    void processResponse(const bmqp_ctrlmsg::ControlMessage& response)
        BSLS_KEYWORD_OVERRIDE;

    void processPeerStopRequest(mqbnet::ClusterNode* clusterNode,
                                const bmqp_ctrlmsg::ControlMessage& request);

    void finishStopSequence(mqbnet::ClusterNode* clusterNode);

    /// Update the datums in the statContexts associated with this
    /// ClusterProxy object.
    void updateDatumStats();

    /// Update the active node manager with the specified advisory from the
    /// specified `source`.
    void
    processNodeStatusAdvisory(const bmqp_ctrlmsg::NodeStatusAdvisory& advisory,
                              mqbnet::ClusterNode*                    source);

    /// Apply the specified `response` to the request manager.  This method
    /// is invoked in the cluster-dispatcher thread.
    void
    processResponseDispatched(const bmqp_ctrlmsg::ControlMessage& response);

    /// Send stop request to proxies specified in `sessions` using the
    /// specified `stopCb` as a callback to be called once all the requests
    /// get responses.
    void sendStopRequest(const SessionSpVec&                  sessions,
                         const StopRequestCompletionCallback& stopCb);

    // PRIVATE ACCESSORS

    /// Load the queue information to the specified `out` object.
    ///
    /// THREAD: This method must be invoked from the DISPATCHER thread.
    void loadQueuesInfo(mqbcmd::StorageContent* out) const;

  private:
    // NOT IMPLEMENTED
    ClusterProxy(const ClusterProxy&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    ClusterProxy& operator=(const ClusterProxy&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterProxy, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object representing a cluster having the specified
    /// `name`, `clusterProxyConfig` and `statContexts`, associated to the
    /// specified `netCluster` and using the specified `scheduler`,
    /// `bufferFactory`, `blobSpPool` and `dispatcher`.  Use the specified
    /// `allocator` for any memory allocation.
    ClusterProxy(const bslstl::StringRef&              name,
                 const mqbcfg::ClusterProxyDefinition& clusterProxyConfig,
                 bslma::ManagedPtr<mqbnet::Cluster>    netCluster,
                 const StatContextsMap&                statContexts,
                 bdlmt::EventScheduler*                scheduler,
                 bdlbb::BlobBufferFactory*             bufferFactory,
                 BlobSpPool*                           blobSpPool,
                 mqbi::Dispatcher*                     dispatcher,
                 mqbnet::TransportManager*             transportManager,
                 bslma::Allocator*                     allocator);

    /// Destructor
    ~ClusterProxy() BSLS_KEYWORD_OVERRIDE;

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

    /// Return a reference offering modifiable access to the net cluster
    /// used by this cluster.
    mqbnet::Cluster& netCluster() BSLS_KEYWORD_OVERRIDE;

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

    /// Process the specified `command`, and write the result in the
    /// specified `result`. Return 0 if the command was successfully
    /// processed, or a non-zero value otherwise.
    int processCommand(mqbcmd::ClusterResult*        result,
                       const mqbcmd::ClusterCommand& command)
        BSLS_KEYWORD_OVERRIDE;

    /// Load the cluster state in the specified `out` object.
    void loadClusterStatus(mqbcmd::ClusterResult* out) BSLS_KEYWORD_OVERRIDE;

    void getPrimaryNodes(bsl::vector<mqbnet::ClusterNode*>* outNodes,
                         bool* outIsSelfPrimary) const BSLS_KEYWORD_OVERRIDE;

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
    //   (virtual: mqbc::ClusterStateObserver)

    /// Callback invoked when failover has not completed above a certain
    /// threshold amount of time.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onFailoverThreshold() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::ClusterObserver)

    /// Notification method to indicate that the specified `node` is now
    /// available (it the specified `isAvailable` is true) or not available
    /// (if `isAvailable` is false).
    void onNodeStateChange(mqbnet::ClusterNode* node,
                           bool isAvailable) BSLS_KEYWORD_OVERRIDE;

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

    /// Print the state of the cluster to the specified `out`.
    ///
    /// THREAD: These methods must be invoked from the DISPATCHER thread.
    void printClusterStateSummary(bsl::ostream& out,
                                  int           level          = 0,
                                  int           spacesPerLevel = 0) const
        BSLS_KEYWORD_OVERRIDE;

    const mqbcfg::ClusterDefinition*
    clusterConfig() const BSLS_KEYWORD_OVERRIDE;
    // Returns a pointer to cluster config if this `mqbi::Cluster`
    // represents a cluster, otherwise null.

    const mqbcfg::ClusterProxyDefinition*
    clusterProxyConfig() const BSLS_KEYWORD_OVERRIDE;
    // Returns a pointer to cluster proxy config if this `mqbi::Cluster`
    // represents a proxy, otherwise null.

    // ACCESSORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;
    // Return a reference to the dispatcherClientData.

    /// Return a printable description of the client (e.g. for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::Cluster)

    /// Return a reference not offering modifiable access to the net cluster
    /// used by this cluster.
    const mqbnet::Cluster& netCluster() const BSLS_KEYWORD_OVERRIDE;

    // Returns a reference to the cluster state describing this cluster
    // const mqbc::ClusterState& clusterState() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------
// class ClusterProxy::ChannelBuffer
// ---------------------------------

// CREATORS
inline ClusterProxy::ChannelBuffer::ChannelBuffer(bslma::Allocator* allocator)
: d_queue(allocator)
, d_queueBytes(0)
{
    // NOTHING
}

// MANIPULATORS
inline void ClusterProxy::ChannelBuffer::push(const bdlbb::Blob& blob)
{
    d_queue.push_back(blob);
    d_queueBytes += blob.length();
}

inline void ClusterProxy::ChannelBuffer::pop()
{
    BSLS_ASSERT_SAFE(!d_queue.empty());
    BSLS_ASSERT_SAFE(d_queueBytes >= d_queue.front().length());

    d_queueBytes -= d_queue.front().length();
    d_queue.pop_front();
}

inline void ClusterProxy::ChannelBuffer::clear()
{
    d_queueBytes = 0;
    d_queue.clear();
}

// ACCESSORS
inline const bdlbb::Blob& ClusterProxy::ChannelBuffer::front() const
{
    return d_queue.front();
}

inline size_t ClusterProxy::ChannelBuffer::numItems() const
{
    BSLS_ASSERT_SAFE((d_queue.size() && d_queueBytes) ||
                     (d_queue.size() == 0 && d_queueBytes == 0));

    return d_queue.size();
}

inline size_t ClusterProxy::ChannelBuffer::bytes() const
{
    BSLS_ASSERT_SAFE((d_queue.size() && d_queueBytes) ||
                     (d_queue.size() == 0 && d_queueBytes == 0));

    return d_queueBytes;
}
// ------------------
// class ClusterProxy
// ------------------

inline void
ClusterProxy::getPrimaryNodes(bsl::vector<mqbnet::ClusterNode*>* outNodes,
                              bool* outIsSelfPrimary) const
{
    // no implementation
}

// PRIVATE MANIPULATORS
//   (virtual: mqbi::Cluster)
inline ClusterProxy::RequestManagerType& ClusterProxy::requestManager()
{
    return d_clusterData.requestManager();
}

inline mqbi::Cluster::MultiRequestManagerType&
ClusterProxy::multiRequestManager()
{
    return d_clusterData.multiRequestManager();
}

// MANIPULATORS
//   (virtual: mqbi::Cluster)
inline mqbnet::Cluster& ClusterProxy::netCluster()
{
    return *(d_clusterData.membership().netCluster());
}

// MANIPULATORS
//   (virtual: mqbi::DispatcherClient)
inline mqbi::Dispatcher* ClusterProxy::dispatcher()
{
    return d_clusterData.dispatcherClientData().dispatcher();
}

inline mqbi::DispatcherClientData& ClusterProxy::dispatcherClientData()
{
    return d_clusterData.dispatcherClientData();
}

// ACCESSORS
//   (virtual: mqbi::Cluster)
inline const bsl::string& ClusterProxy::name() const
{
    return d_clusterData.identity().name();
}

inline bool ClusterProxy::isLocal() const
{
    return false;
}

inline bool ClusterProxy::isRemote() const
{
    return true;
}

inline bool ClusterProxy::isClusterMember() const
{
    return false;
}

inline bool ClusterProxy::isFailoverInProgress() const
{
    return d_queueHelper.isFailoverInProgress();
}

inline bool ClusterProxy::isStopping() const
{
    return d_isStopping;
}

inline const mqbcfg::ClusterDefinition* ClusterProxy::clusterConfig() const
{
    return 0;
}

inline const mqbcfg::ClusterProxyDefinition*
ClusterProxy::clusterProxyConfig() const
{
    return &d_clusterData.clusterProxyConfig();
}

// ACCESSORS
//   (virtual: mqbi::DispatcherClient)
inline const mqbi::Dispatcher* ClusterProxy::dispatcher() const
{
    return d_clusterData.dispatcherClientData().dispatcher();
}

inline const mqbi::DispatcherClientData&
ClusterProxy::dispatcherClientData() const
{
    return d_clusterData.dispatcherClientData();
}

inline const bsl::string& ClusterProxy::description() const
{
    return d_clusterData.identity().description();
}

// ACCESSORS
//   (virtual: mqbi::Cluster)
inline const mqbnet::Cluster& ClusterProxy::netCluster() const
{
    return *(d_clusterData.membership().netCluster());
}

// inline const mqbc::ClusterState& ClusterProxy::clusterState() const
// {
//     return d_state;
// }

}  // close package namespace
}  // close enterprise namespace

#endif
