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

// mqbmock_cluster.cpp                                                -*-C++-*-
#include <mqbmock_cluster.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clustermembership.h>
#include <mqbc_clusterutil.h>
#include <mqbcmd_messages.h>
#include <mqbnet_mockcluster.h>
#include <mqbstat_clusterstats.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

#include <bmqio_channel.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_iostream.h>
#include <bslmt_semaphore.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_systemclocktype.h>

namespace BloombergLP {
namespace mqbmock {

namespace {

// CONSTANTS
const bsls::Types::Uint64 k_KiB = 1024;          // 2^10
const bsls::Types::Uint64 k_MiB = 1024 * k_KiB;  // 2^20
const bsls::Types::Uint64 k_GiB = 1024 * k_MiB;  // 2^30

const int k_BLOB_POOL_GROWTH_STRATEGY = 1024;

// STATIC HELPER FUNCTIONS

/// Post on the specified `sem`.
static void schedulerWaitCb(bslmt::Semaphore* sem)
{
    sem->post();
}

/// Create a new blob at the specified `arena` address, using the specified
/// `bufferFactory` and `allocator`.
static void createBlob(bdlbb::BlobBufferFactory* bufferFactory,
                       void*                     arena,
                       bslma::Allocator*         allocator)
{
    new (arena) bdlbb::Blob(bufferFactory, allocator);
}

}  // close unnamed namespace

// -------------
// class Cluster
// -------------

const int Cluster::k_LEADER_NODE_ID = 1;

// PRIVATE MANIPULATORS
void Cluster::_initializeClusterDefinition(
    const bslstl::StringRef&                name,
    const bslstl::StringRef&                location,
    const bslstl::StringRef&                archive,
    const bsl::vector<mqbcfg::ClusterNode>& nodes,
    bool                                    isCSLMode,
    bool                                    isFSMWorkflow)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "_initializeClusterDefinition() must be called before"
                    " start()");
    if (isFSMWorkflow) {
        BSLS_ASSERT_SAFE(isCSLMode);
    }

    d_clusterDefinition.name() = name;

    d_clusterDefinition.clusterAttributes().isCSLModeEnabled() = isCSLMode;
    d_clusterDefinition.clusterAttributes().isFSMWorkflow()    = isFSMWorkflow;

    mqbcfg::PartitionConfig& partitionCfg =
        d_clusterDefinition.partitionConfig();
    partitionCfg.numPartitions()       = 4;
    partitionCfg.location()            = bsl::string(location, d_allocator_p);
    partitionCfg.archiveLocation()     = bsl::string(archive, d_allocator_p);
    partitionCfg.maxDataFileSize()     = 5 * k_GiB;
    partitionCfg.maxJournalFileSize()  = 256 * k_MiB;
    partitionCfg.maxQlistFileSize()    = 16 * k_KiB;
    partitionCfg.preallocate()         = false;
    partitionCfg.maxArchivedFileSets() = 0;
    partitionCfg.prefaultPages()       = true;
    partitionCfg.syncConfig()          = mqbcfg::StorageSyncConfig();

    d_clusterDefinition.nodes() = nodes;

    mqbcfg::ClusterMonitorConfig& config =
        d_clusterDefinition.clusterMonitorConfig();

    config.maxTimeLeader()   = 60;
    config.maxTimeMaster()   = 120;
    config.maxTimeNode()     = 120;
    config.maxTimeFailover() = 240;

    config.thresholdLeader()   = 30;
    config.thresholdMaster()   = 60;
    config.thresholdNode()     = 60;
    config.thresholdFailover() = 120;
}

void Cluster::_initializeNetcluster()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "_initializeNetcluster() must be called before start()");

    // Create 'mqbnet::MockCluster'
    d_netCluster_mp.load(new (*d_allocator_p)
                             mqbnet::MockCluster(d_clusterDefinition,
                                                 d_bufferFactory_p,
                                                 d_allocator_p),
                         d_allocator_p);

    // Create and set channels on nodes
    NodesList& nodes = d_netCluster_mp->nodes();
    for (NodesListIter iter = nodes.begin(); iter != nodes.end(); ++iter) {
        d_channels.emplace(
            *iter,
            bsl::allocate_shared<bmqio::TestChannel>(d_allocator_p),
            d_allocator_p);

        bsl::weak_ptr<bmqio::Channel> channelWp(d_channels.at(*iter));
        (*iter)->setChannel(channelWp,
                            bmqp_ctrlmsg::ClientIdentity(),
                            bmqio::Channel::ReadCallback());
    }

    const int selfNodeId = d_isLeader ? k_LEADER_NODE_ID
                                      : k_LEADER_NODE_ID + 1;
    dynamic_cast<mqbnet::MockCluster*>(d_netCluster_mp.get())
        ->_setSelfNodeId(selfNodeId);

    if (d_isClusterMember) {
        BSLS_ASSERT_OPT(0 != d_netCluster_mp->selfNode());
    }
}

void Cluster::_initializeNodeSessions()
{
    // Create and populate meta data for each cluster node
    mqbc::ClusterMembership& clusterMembership =
        d_clusterData_mp->membership();

    NodesListIter nodeIter = clusterMembership.netCluster()->nodes().begin();
    NodesListIter endIter  = clusterMembership.netCluster()->nodes().end();
    for (; nodeIter != endIter; ++nodeIter) {
        mqbc::ClusterMembership::ClusterNodeSessionSp nodeSessionSp;
        nodeSessionSp.createInplace(d_allocator_p,
                                    this,
                                    *nodeIter,
                                    d_clusterData_mp->identity().name(),
                                    d_clusterData_mp->identity().identity(),
                                    d_allocator_p);
        nodeSessionSp->setNodeStatus(bmqp_ctrlmsg::NodeStatus::E_AVAILABLE);

        // Create stat context for each cluster node
        bmqst::StatContextConfiguration config((*nodeIter)->hostName());
        nodeSessionSp->statContext() =
            d_clusterData_mp->clusterNodesStatContext()->addSubcontext(config);

        clusterMembership.clusterNodeSessionMap().insert(
            bsl::make_pair(*nodeIter, nodeSessionSp));

        if (clusterMembership.netCluster()->selfNodeId() ==
            (*nodeIter)->nodeId()) {
            clusterMembership.setSelfNodeSession(nodeSessionSp.get());
        }
    }
}

// CREATORS
Cluster::Cluster(bdlbb::BlobBufferFactory* bufferFactory,
                 bslma::Allocator*         allocator,
                 bool                      isClusterMember,
                 bool                      isLeader,
                 bool                      isCSLMode,
                 bool                      isFSMWorkflow,
                 const ClusterNodeDefs&    clusterNodeDefs,
                 const bslstl::StringRef&  name,
                 const bslstl::StringRef&  location,
                 const bslstl::StringRef&  archive)
: d_allocator_p(allocator)
, d_bufferFactory_p(bufferFactory)
, d_blobSpPool(bdlf::BindUtil::bind(&createBlob,
                                    d_bufferFactory_p,
                                    bdlf::PlaceHolders::_1,   // arena
                                    bdlf::PlaceHolders::_2),  // allocator
               k_BLOB_POOL_GROWTH_STRATEGY,
               d_allocator_p)
, d_dispatcher(allocator)
, d_scheduler(bsls::SystemClockType::e_MONOTONIC, allocator)
, d_timeSource(&d_scheduler)
, d_isStarted(false)
, d_clusterDefinition(allocator)
, d_channels(allocator)
, d_negotiator_mp()
, d_transportManager(&d_scheduler,
                     bufferFactory,
                     d_negotiator_mp,
                     0,  // mqbstat::StatController*
                     allocator)
, d_netCluster_mp(0)
, d_clusterData_mp(0)
, d_isClusterMember(isClusterMember)
, d_state(this,
          4,  // partitionsCount
          allocator)
, d_statContexts(allocator)
, d_statContext_sp(
      mqbstat::ClusterStatsUtil::initializeStatContextCluster(2, allocator))
, d_isLeader(isLeader)
, d_isRestoringState(false)
, d_processor()
, d_resources(&d_scheduler, bufferFactory, &d_blobSpPool)
{
    // PRECONDITIONS
    if (isClusterMember) {
        BSLS_ASSERT_OPT(!clusterNodeDefs.empty());
    }
    else {
        BSLS_ASSERT_OPT(!isLeader);
    }

    _initializeClusterDefinition(name,
                                 location,
                                 archive,
                                 clusterNodeDefs,
                                 isCSLMode,
                                 isFSMWorkflow);
    _initializeNetcluster();

    d_statContexts.insert(
        bsl::make_pair(bsl::string("clusterNodes", d_allocator_p),
                       d_statContext_sp.get()));

    d_dispatcherClientData.setDispatcher(&d_dispatcher);
    d_dispatcher._setInDispatcherThread(true);

    d_clusterData_mp.load(new (*d_allocator_p) mqbc::ClusterData(
                              d_clusterDefinition.name(),
                              d_resources,
                              d_clusterDefinition,
                              mqbcfg::ClusterProxyDefinition(d_allocator_p),
                              d_netCluster_mp,
                              this,
                              0,  // domainFactory
                              &d_transportManager,
                              d_statContext_sp.get(),
                              d_statContexts,
                              d_allocator_p),
                          d_allocator_p);

    // Set cluster state's dispatcher
    d_clusterData_mp->dispatcherClientData() = d_dispatcherClientData;

    _initializeNodeSessions();
}

Cluster::~Cluster()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destruction");
}

// MANIPULATORS
//   (virtual: mqbi::DispatcherClient)
void Cluster::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    if (d_processor) {
        d_processor(event);
    }
}

void Cluster::flush()
{
    // NOTHING
}

// MANIPULATORS
//   (virtual: mqbi::Cluster)
int Cluster::start(BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "start() can only be called once on this object");

    d_scheduler.start();

    d_isStarted = true;

    return 0;
}

void Cluster::initiateShutdown(
    BSLS_ANNOTATION_UNUSED const VoidFunctor& callback,
    BSLS_ANNOTATION_UNUSED bool               supportShutdownV2)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "start() can only be called once on this object");

    // NOTHING
}

void Cluster::stop()
{
    // executed by *ANY* thread

    if (!d_isStarted) {
        return;  // RETURN
    }

    if (!isLocal()) {
        // Cancel all requests with the 'CANCELED' category and the
        // 'e_STOPPING' code.
        bmqp_ctrlmsg::ControlMessage response;
        bmqp_ctrlmsg::Status&        failure = response.choice().makeStatus();
        failure.category() = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
        failure.code()     = mqbi::ClusterErrorCode::e_STOPPING;
        failure.message()  = "Node is being stopped";

        d_clusterData_mp->requestManager().cancelAllRequests(response);

        d_clusterData_mp->membership().netCluster()->closeChannels();

        for (TestChannelMapIter iter = d_channels.begin();
             iter != d_channels.end();
             ++iter) {
            iter->second->reset();
        }
    }

    d_scheduler.stop();

    d_isStarted = false;
}

void Cluster::registerStateObserver(
    BSLS_ANNOTATION_UNUSED mqbc::ClusterStateObserver* observer)
{
    // executed by *ANY* thread

    // NOTHING
}

void Cluster::unregisterStateObserver(
    BSLS_ANNOTATION_UNUSED mqbc::ClusterStateObserver* observer)
{
    // executed by *ANY* thread

    // NOTHING
}

mqbnet::Cluster& Cluster::netCluster()
{
    return *(d_clusterData_mp->membership().netCluster());
}

Cluster::RequestManagerType& Cluster::requestManager()
{
    return d_clusterData_mp->requestManager();
}

mqbc::ClusterData::MultiRequestManagerType& Cluster::multiRequestManager()
{
    return d_clusterData_mp->multiRequestManager();
}

bmqt::GenericResult::Enum
Cluster::sendRequest(const Cluster::RequestManagerType::RequestSp& request,
                     mqbnet::ClusterNode*                          target,
                     bsls::TimeInterval                            timeout)
{
    BSLS_ASSERT_SAFE(target);
    request->setGroupId(target->nodeId());

    return d_clusterData_mp->requestManager().sendRequest(
        request,
        bdlf::BindUtil::bind(&mqbnet::ClusterNode::write,
                             target,
                             bdlf::PlaceHolders::_1,
                             bmqp::EventType::e_CONTROL),
        target->nodeDescription(),
        timeout);
}

void Cluster::openQueue(
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED mqbi::Domain* domain,
    BSLS_ANNOTATION_UNUSED const         bmqp_ctrlmsg::QueueHandleParameters&
                                         handleParameters,
    BSLS_ANNOTATION_UNUSED const
        bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    BSLS_ANNOTATION_UNUSED const mqbi::Cluster::OpenQueueCallback& callback)
{
    // NOTHING
}

void Cluster::configureQueue(
    BSLS_ANNOTATION_UNUSED mqbi::Queue*                queue,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    BSLS_ANNOTATION_UNUSED unsigned int                upstreamSubQueueId,
    const mqbi::QueueHandle::HandleConfiguredCallback& callback)
{
    if (callback) {
        bmqp_ctrlmsg::Status status(d_allocator_p);
        status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        status.code()     = 0;
        status.message()  = "";
        callback(status, streamParameters);
    }
}

void Cluster::configureQueue(
    BSLS_ANNOTATION_UNUSED mqbi::Queue*    queue,
    BSLS_ANNOTATION_UNUSED const           bmqp_ctrlmsg::QueueHandleParameters&
                                           handleParameters,
    BSLS_ANNOTATION_UNUSED unsigned int    upstreamSubQueueId,
    const Cluster::HandleReleasedCallback& callback)
{
    if (callback) {
        bmqp_ctrlmsg::Status status(d_allocator_p);
        status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        status.code()     = 0;
        status.message()  = "";
        callback(status);
    }
}

void Cluster::onQueueHandleCreated(BSLS_ANNOTATION_UNUSED mqbi::Queue* queue,
                                   BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
                                   BSLS_ANNOTATION_UNUSED bool handleCreated)
{
}

void Cluster::onQueueHandleDestroyed(
    BSLS_ANNOTATION_UNUSED mqbi::Queue* queue,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri)
{
}

void Cluster::onDomainReconfigured(
    BSLS_ANNOTATION_UNUSED const mqbi::Domain& domain,
    BSLS_ANNOTATION_UNUSED const mqbconfm::Domain& oldDefn,
    BSLS_ANNOTATION_UNUSED const mqbconfm::Domain& newDefn)
{
}

int Cluster::processCommand(mqbcmd::ClusterResult*        result,
                            const mqbcmd::ClusterCommand& command)
{
    bmqu::MemOutStream os;
    os << "MockCluster::processCommand '" << command << "' not implemented!";
    result->makeError().message() = os.str();
    return -1;
}

void Cluster::loadClusterStatus(mqbcmd::ClusterResult* out)
{
    out->makeClusterStatus();
}

// MANIPULATORS
//   (specific to mqbmock::Cluster)
Cluster& Cluster::_setIsClusterMember(bool value)
{
    d_isClusterMember = value;
    return *this;
}

Cluster& Cluster::_setIsRestoringState(bool value)
{
    d_isRestoringState = value;
    return *this;
}

void Cluster::waitForScheduler()
{
    bslmt::Semaphore sem;

    d_scheduler.scheduleEvent(d_timeSource.now(),
                              bdlf::BindUtil::bind(&schedulerWaitCb, &sem));
    sem.wait();
}

// ACCESSORS
//   (virtual: mqbi::DispatcherClient)
const mqbi::Dispatcher* Cluster::dispatcher() const
{
    return d_dispatcherClientData.dispatcher();
}

const mqbi::DispatcherClientData& Cluster::dispatcherClientData() const
{
    return d_dispatcherClientData;
}

const bsl::string& Cluster::description() const
{
    return d_clusterData_mp->identity().description();
}

// ACCESSORS
//   (virtual: mqbi::Cluster)
const bsl::string& Cluster::name() const
{
    return d_clusterData_mp->identity().name();
}

const mqbnet::Cluster& Cluster::netCluster() const
{
    return *(d_clusterData_mp->membership().netCluster());
}

void Cluster::printClusterStateSummary(
    bsl::ostream&              out,
    BSLS_ANNOTATION_UNUSED int level,
    BSLS_ANNOTATION_UNUSED int spacesPerLevel) const
{
    out << "MockCluster::printClusterStateSummary not implemented";
}

bool Cluster::isLocal() const
{
    return false;
}

bool Cluster::isRemote() const
{
    return !d_isClusterMember;
}

bool Cluster::isClusterMember() const
{
    return d_isClusterMember;
}

bool Cluster::isFailoverInProgress() const
{
    return d_isRestoringState;
}

bool Cluster::isStopping() const
{
    return false;
}

const mqbcfg::ClusterDefinition* Cluster::clusterConfig() const
{
    return &d_clusterDefinition;
}

const mqbcfg::ClusterProxyDefinition* Cluster::clusterProxyConfig() const
{
    return 0;
}

}  // close package namespace
}  // close enterprise namespace
