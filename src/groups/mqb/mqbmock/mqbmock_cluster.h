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

// mqbmock_cluster.h                                                  -*-C++-*-
#ifndef INCLUDED_MQBMOCK_CLUSTER
#define INCLUDED_MQBMOCK_CLUSTER

//@PURPOSE: Provide a mock cluster implementation of 'mqbi::Cluster'.
//
//@CLASSES:
//  mqbmock::Cluster: Mock cluster implementation
//
//@DESCRIPTION: This component provides a mock implementation,
// 'mqbmock::Cluster', of the 'mqbi::DispatcherClient' and 'mqbi::Cluster'
// protocols that is used to emulate a real cluster for testing purposes.
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

#include <mqbc_clusterdata.h>
#include <mqbc_clusterstate.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbmock_dispatcher.h>
#include <mqbnet_channel.h>
#include <mqbnet_cluster.h>
#include <mqbnet_transportmanager.h>

#include <bmqio_status.h>
#include <bmqio_testchannel.h>
#include <bmqst_statcontext.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
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
class FixedThreadPool;
}
namespace bmqp_ctrlmsg {
class QueueHandleParameters;
}
namespace bmqp_ctrlmsg {
class QueueStreamParameters;
}
namespace bmqt {
class Uri;
}
namespace mqbcmd {
class ClusterCommand;
}
namespace mqbcmd {
class ClusterResult;
}
namespace mqbi {
class Domain;
}
namespace mqbnet {
class Negotiator;
}

namespace mqbmock {

// =============
// class Cluster
// =============

/// Mock cluster implementation of the `mqbi::Cluster` interface.
class Cluster : public mqbi::Cluster {
  private:
    // PRIVATE TYPES
    typedef Cluster::RequestManagerType      RequestManagerType;
    typedef Cluster::MultiRequestManagerType MultiRequestManagerType;

    typedef bsl::function<void(const mqbi::DispatcherEvent& event)>
        EventProcessor;

    typedef bslma::ManagedPtr<mqbnet::Negotiator> NegotiatorMp;

    typedef bslma::ManagedPtr<mqbnet::Cluster> NetClusterMp;

    typedef bslma::ManagedPtr<mqbc::ClusterData> ClusterDataMp;

    typedef mqbnet::Cluster::NodesList NodesList;
    typedef NodesList::iterator        NodesListIter;

    typedef bsl::shared_ptr<bmqst::StatContext> StatContextSp;
    typedef mqbc::ClusterData::StatContextsMap  StatContextsMap;

  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

    typedef bsl::vector<mqbcfg::ClusterNode> ClusterNodeDefs;

    typedef bsl::shared_ptr<bmqio::TestChannel> TestChannelSp;
    typedef bsl::unordered_map<mqbnet::ClusterNode*, TestChannelSp>
                                           TestChannelMap;
    typedef TestChannelMap::iterator       TestChannelMapIter;
    typedef TestChannelMap::const_iterator TestChannelMapCIter;

  public:
    // CONSTANTS

    /// This public constant represents the leader node Id for this cluster.
    static const int k_LEADER_NODE_ID;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    bdlbb::BlobBufferFactory* d_bufferFactory_p;
    // Buffer factory to use

    BlobSpPool d_blobSpPool;
    // Blob sp pool to use

    mqbmock::Dispatcher d_dispatcher;
    // Event dispatcher to use

    bdlmt::EventScheduler d_scheduler;
    // Event scheduler to use

    bdlmt::EventSchedulerTestTimeSource d_timeSource;
    // Time source for the event scheduler

    bool d_isStarted;
    // Flag to indicate start/stop status This
    // flag is used only inside this component

    mqbcfg::ClusterDefinition d_clusterDefinition;
    // Cluster definition

    TestChannelMap d_channels;
    // Test channels

    NegotiatorMp d_negotiator_mp;
    // Session negotiator

    mqbnet::TransportManager d_transportManager;
    // Transport manager

    NetClusterMp d_netCluster_mp;
    // Net cluster used by this cluster

    ClusterDataMp d_clusterData_mp;
    // Cluster's transient data

    bool d_isClusterMember;
    // Flag indicating if this cluster is a
    // 'cluster'

    mqbc::ClusterState d_state;
    // Cluster's persistent state

    StatContextsMap d_statContexts;

    StatContextSp d_statContext_sp;

    bool d_isLeader;
    // Flag indicating if this node is a leader

    bool d_isRestoringState;
    // Flag indicating if this cluster is in the
    // process of restoring its state

    mqbi::DispatcherClientData d_dispatcherClientData;
    // Dispatcher client data

    EventProcessor d_processor;

    mqbi::ClusterResources d_resources;

  private:
    // NOT IMPLEMENTED
    Cluster(const Cluster&) BSLS_CPP11_DELETED;
    Cluster& operator=(const Cluster&) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Initialize the internal cluster definition with the specified
    /// `name`, `location`, `archive` location, `nodes`, `isCSLMode` and
    /// `isFSMWorkflow`.
    void _initializeClusterDefinition(const bslstl::StringRef& name,
                                      const bslstl::StringRef& location,
                                      const bslstl::StringRef& archive,
                                      const ClusterNodeDefs&   nodes,
                                      bool                     isCSLMode,
                                      bool                     isFSMWorkflow);

    /// Initialize the net cluster.
    void _initializeNetcluster();

    /// Initialize the cluster node sessions.
    void _initializeNodeSessions();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Cluster, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbmock::Cluster` object having the optionally specified
    /// `name`, `location`, `archive`, `clusterNodeDefs`, `isCSLMode` and
    /// `isFSMWorkflow`, and using the specified `bufferFactory` and
    /// `allocator`.  If the optionally specified `isClusterMember` is true,
    /// self will be a member of the cluster.  If the optionally specified
    /// `isLeader` is true, self will become the leader of the cluster.
    /// Note that if `isClusterMember` is false, `isLeader` cannot be true.
    Cluster(bdlbb::BlobBufferFactory* bufferFactory,
            bslma::Allocator*         allocator,
            bool                      isClusterMember = false,
            bool                      isLeader        = false,
            bool                      isCSLMode       = false,
            bool                      isFSMWorkflow   = false,
            const ClusterNodeDefs&    clusterNodeDefs = ClusterNodeDefs(),
            const bslstl::StringRef&  name            = "testCluster",
            const bslstl::StringRef&  location        = "",
            const bslstl::StringRef&  archive         = "");

    /// Destructor
    ~Cluster() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE;

    void _setEventProcessor(const EventProcessor& processor);

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
    //   (virtual: mqbi::Cluster)

    /// Start the `Cluster`.  Return 0 on success and non-zero otherwise
    /// populating the specified `errorDescription` with the reason of the
    /// error.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Initiate the shutdown of the cluster and invoke the specified
    /// `callback` upon completion of (asynchronous) shutdown sequence. It
    /// is expected that `stop()` will be called soon after this routine is
    /// invoked.  If the optional (temporary) specified 'supportShutdownV2' is
    /// 'true' execute shutdown logic V2 where upstream (not downstream) nodes
    /// deconfigure  queues and the shutting down node (not downstream) wait
    /// for CONFIRMS.
    void
    initiateShutdown(const VoidFunctor& callback,
                     bool supportShutdownV2 = false) BSLS_KEYWORD_OVERRIDE;

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

    /// Return a reference offering modifiable access to the net cluster
    /// used by this cluster.
    mqbnet::Cluster& netCluster() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering modifiable access to the request manager
    /// used by this cluster.
    RequestManagerType& requestManager() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering modifiable access to the multi request
    /// manager used by this cluster.
    MultiRequestManagerType& multiRequestManager() BSLS_KEYWORD_OVERRIDE;

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

    void processResponse(const bmqp_ctrlmsg::ControlMessage& response)
        BSLS_KEYWORD_OVERRIDE;
    // Process the specified 'response' message as a response to previously
    // transmitted request.

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
    void configureQueue(
        mqbi::Queue*                               queue,
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
        unsigned int                               upstreamSubQueueId,
        const HandleReleasedCallback& callback) BSLS_KEYWORD_OVERRIDE;

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

    /// Process the specified `command`, and load the result to the
    /// specified `result`. Return 0 if the command was successfully
    /// processed, or a non-zero value otherwise.
    int processCommand(mqbcmd::ClusterResult*        result,
                       const mqbcmd::ClusterCommand& command)
        BSLS_KEYWORD_OVERRIDE;

    /// Load the cluster state to the specified `out` object.
    void loadClusterStatus(mqbcmd::ClusterResult* out) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (specific to mqbmock::Cluster)

    /// Set whether this cluster is a `cluster` and return a reference
    /// offering modifiable access to this object.
    Cluster& _setIsClusterMember(bool value);

    /// Set whether this cluster is currently undergoing failover and return
    /// a reference offering modifiable access to this object.
    Cluster& _setIsRestoringState(bool value);

    /// Get a modifiable reference to this object's buffer factory.
    bdlbb::BlobBufferFactory* _bufferFactory();

    /// Get a modifiable reference to this object's blob shared pointer pool.
    BlobSpPool* _blobSpPool();

    /// Get a modifiable reference to this object's _scheduler.
    bdlmt::EventScheduler& _scheduler();

    /// Get a modifiable reference to this object's time source.
    bdlmt::EventSchedulerTestTimeSource& _timeSource();

    /// Get a modifiable reference to this object's cluster data.
    mqbc::ClusterData* _clusterData();

    /// Get a modifiable reference to this object's cluster state.
    mqbc::ClusterState& _state();

    /// Move the test timer forward the specified `seconds`.
    void advanceTime(int seconds);

    /// Move the test timer forward the specified `milliseconds`.
    void advanceTime(const bsls::TimeInterval& interval);

    /// Block until scheduler executes all the scheduled callbacks.
    void waitForScheduler();

    void getPrimaryNodes(int*                               rc,
                         bsl::ostream&                      errorDescription,
                         bsl::vector<mqbnet::ClusterNode*>* nodes,
                         bool* isSelfPrimary) const BSLS_KEYWORD_OVERRIDE;

    void getPartitionPrimaryNode(int*                  rc,
                                 bsl::ostream&         errorDescription,
                                 mqbnet::ClusterNode** node,
                                 bool*                 isSelfPrimary,
                                 int partitionId) const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;

    /// Return a printable description of the client (e.g. for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::Cluster)

    /// Return the name of this cluster.
    const bsl::string& name() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference not offering modifiable access to the net cluster
    /// used by this cluster.
    const mqbnet::Cluster& netCluster() const BSLS_KEYWORD_OVERRIDE;

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

    /// Return boolean flag indicating if CSL Mode is enabled.
    bool isCSLModeEnabled() const BSLS_KEYWORD_OVERRIDE;

    /// Return boolean flag indicating if CSL FSM workflow is in effect.
    bool isFSMWorkflow() const BSLS_KEYWORD_OVERRIDE;

    const mqbcfg::ClusterDefinition*
    clusterConfig() const BSLS_KEYWORD_OVERRIDE;
    // Returns a pointer to cluster config if this `mqbi::Cluster` represents
    // a cluster, otherwise null.

    const mqbcfg::ClusterProxyDefinition*
    clusterProxyConfig() const BSLS_KEYWORD_OVERRIDE;
    // Returns a pointer to cluster proxy config if this `mqbi::Cluster`
    // represents a proxy, otherwise null.

    // ACCESSORS
    const mqbcfg::ClusterDefinition&           _clusterDefinition() const;
    const bdlmt::EventSchedulerTestTimeSource& _timeSource() const;
    const TestChannelMap&                      _channels() const;
    const mqbc::ClusterData*                   _clusterData() const;
    const mqbi::ClusterResources&              _resources() const;

    /// Return the value of the corresponding member of this object.
    const mqbc::ClusterState& _state() const;

    /// Return the current time.
    bsls::TimeInterval getTime() const;

    /// Return the current time in Int64 format.
    bsls::Types::Int64 getTimeInt64() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// MANIPULATORS
//   (virtual: mqbi::DispatcherClient)
inline mqbi::Dispatcher* Cluster::dispatcher()
{
    return d_dispatcherClientData.dispatcher();
}

inline mqbi::DispatcherClientData& Cluster::dispatcherClientData()
{
    return d_dispatcherClientData;
}

// MANIPULATORS
inline void Cluster::processResponse(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& response)
{
    // NOTHING
}

inline void Cluster::_setEventProcessor(const EventProcessor& processor)
{
    d_processor = processor;
}

inline bdlbb::BlobBufferFactory* Cluster::_bufferFactory()
{
    return d_bufferFactory_p;
}

inline Cluster::BlobSpPool* Cluster::_blobSpPool()
{
    return &d_blobSpPool;
}

inline bdlmt::EventScheduler& Cluster::_scheduler()
{
    return d_scheduler;
}

inline bdlmt::EventSchedulerTestTimeSource& Cluster::_timeSource()
{
    return d_timeSource;
}

inline mqbc::ClusterData* Cluster::_clusterData()
{
    return d_clusterData_mp.get();
}

inline mqbc::ClusterState& Cluster::_state()
{
    return d_state;
}

inline void Cluster::advanceTime(int seconds)
{
    d_timeSource.advanceTime(bsls::TimeInterval(seconds));
}

inline void Cluster::advanceTime(const bsls::TimeInterval& interval)
{
    d_timeSource.advanceTime(interval);
}

inline void Cluster::getPrimaryNodes(int*,
                                     bsl::ostream&,
                                     bsl::vector<mqbnet::ClusterNode*>*,
                                     bool*) const
{
    // no implementation -- this should never run.
    BSLS_ASSERT_SAFE(false);
}

inline void Cluster::getPartitionPrimaryNode(int*,
                                             bsl::ostream&,
                                             mqbnet::ClusterNode**,
                                             bool*,
                                             int) const
{
    // no implementation -- this should never run.
    BSLS_ASSERT_SAFE(false);
}

// ACCESSORS
//   (virtual: mqbi::Cluster)
inline bool Cluster::isCSLModeEnabled() const
{
    return d_clusterDefinition.clusterAttributes().isCSLModeEnabled();
}

inline bool Cluster::isFSMWorkflow() const
{
    return d_clusterDefinition.clusterAttributes().isFSMWorkflow();
}

// ACCESSORS
inline const mqbcfg::ClusterDefinition& Cluster::_clusterDefinition() const
{
    return d_clusterDefinition;
}

inline const bdlmt::EventSchedulerTestTimeSource& Cluster::_timeSource() const
{
    return d_timeSource;
}

inline const Cluster::TestChannelMap& Cluster::_channels() const
{
    return d_channels;
}

inline const mqbc::ClusterData* Cluster::_clusterData() const
{
    return d_clusterData_mp.get();
}

inline const mqbc::ClusterState& Cluster::_state() const
{
    return d_state;
}

inline const mqbi::ClusterResources& Cluster::_resources() const
{
    return d_resources;
}

inline bsls::TimeInterval Cluster::getTime() const
{
    return d_timeSource.now();
}

inline bsls::Types::Int64 Cluster::getTimeInt64() const
{
    return d_timeSource.now().seconds();
}

}  // close package namespace
}  // close enterprise namespace

#endif
