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

// mqbblp_clustercatalog.cpp                                          -*-C++-*-
#include <mqbblp_clustercatalog.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_cluster.h>
#include <mqbblp_clusterproxy.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcmd_messages.h>
#include <mqbnet_cluster.h>
#include <mqbnet_negotiator.h>
#include <mqbnet_transportmanager.h>

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>
#include <bmqu_stringutil.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <bdlb_string.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlmt_eventscheduler.h>
#include <bdls_pathutil.h>
#include <bdlsb_fixedmeminstreambuf.h>
#include <bsl_cstddef.h>
#include <bsl_iomanip.h>
#include <bsl_iostream.h>
#include <bslmt_mutexassert.h>
#include <bsls_systemclocktype.h>

namespace BloombergLP {
namespace mqbblp {

// --------------------
// class ClusterCatalog
// --------------------
int ClusterCatalog::createNetCluster(
    bsl::ostream&                           errorDescription,
    bslma::ManagedPtr<mqbnet::Cluster>*     out,
    const bsl::string&                      name,
    const bsl::vector<mqbcfg::ClusterNode>& nodes)
{
    // PRECONDITIONS
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_mutex);  // mutex was LOCKED

    const bool isMember  = (d_myClusters.find(name) != d_myClusters.end());
    const bool isReverse = (d_myReverseClusters.find(name) !=
                            d_myReverseClusters.end());

    mqbnet::TransportManager::ConnectionMode connectionMode;
    if (isMember) {
        connectionMode = mqbnet::TransportManager::e_MIXED;
    }
    else if (isReverse) {
        connectionMode = mqbnet::TransportManager::e_LISTEN_ALL;
    }
    else {
        connectionMode = mqbnet::TransportManager::e_CONNECT_ALL;
    }

    NegotiationUserData* userData = new (*d_allocator_p) NegotiationUserData;
    userData->d_clusterName       = name;
    userData->d_myNodeId          = -1;  // Unused when not reversed connection
    userData->d_isClusterConnection = true;

    bslma::ManagedPtr<void> userDataMp(userData, d_allocator_p);

    return d_transportManager_p->createCluster(errorDescription,
                                               out,
                                               name,
                                               nodes,
                                               connectionMode,
                                               &userDataMp);
}

struct Named {
    explicit Named(const bslstl::StringRef& name)
    : d_name(name)
    {
    }
    const bslstl::StringRef d_name;

    template <typename T>
    bool operator()(const T& value) const
    {
        return value.name() == d_name;
    }
};

int ClusterCatalog::createCluster(bsl::ostream& errorDescription,
                                  bsl::shared_ptr<mqbi::Cluster>* out,
                                  const bsl::string&              name)
{
    // PRECONDITIONS
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_mutex);  // mutex was LOCKED

    // NOTE: The TransportManager, upon negotiation of a session will query
    //       this ClusterCatalog (via the 'onNegotiationForClusterSession()'
    //       method) which will look for the below 'ClusterInfo' in the
    //       'd_clusters' map.  Because session negotiation is async and from a
    //       different thread, there is potential that a session would be
    //       negotiated before we inserted the info in the clusters map.
    //       However, d_mutex is locked here and ensures serialized access and
    //       usage of ClusterCatalog.

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                    = 0,
        rc_NAME_NOT_UNIQUE            = -1,
        rc_FETCH_DEFINITION_FAILED    = -2,
        rc_NETCLUSTER_CREATION_FAILED = -3,
        rc_NETCLUSTER_INVALID_NODE_ID = -4
    };

    int rc = rc_SUCCESS;

    // Make sure the cluster doesn't exist already
    if (d_clusters.find(name) != d_clusters.end()) {
        errorDescription << "A cluster with the same name ('" << name
                         << "') already exists !";
        return rc_NAME_NOT_UNIQUE;  // RETURN
    }

    ClusterInfo                        info;
    bslma::ManagedPtr<mqbnet::Cluster> netCluster;
    bslma::Allocator*          clusterAllocator      = d_allocators.get(name);
    ClusterDefinitionConstIter clusterDefinitionIter = bsl::find_if(
        d_clustersDefinition.myClusters().begin(),
        d_clustersDefinition.myClusters().end(),
        Named(name));
    const bool isMember = clusterDefinitionIter !=
                          d_clustersDefinition.myClusters().end();
    if (isMember) {
        // 1. Fetch the cluster definition
        const mqbcfg::ClusterDefinition& clusterDefinition =
            *clusterDefinitionIter;
        if (clusterDefinition.clusterAttributes().isFSMWorkflow() &&
            !clusterDefinition.clusterAttributes().isCSLModeEnabled()) {
            errorDescription << "Cluster ('" << name
                             << "') has incompatible CSL and FSM modes, not "
                                "creating cluster.";
            return (rc * 10) + rc_FETCH_DEFINITION_FAILED;  // RETURN
        }

        // 2. Create the mqbnet::Cluster
        rc = createNetCluster(errorDescription,
                              &netCluster,
                              name,
                              clusterDefinition.nodes());
        if (rc != 0) {
            return (rc * 10) + rc_NETCLUSTER_CREATION_FAILED;  // RETURN
        }
        else if (netCluster->selfNodeId() ==
                 mqbnet::Cluster::k_INVALID_NODE_ID) {
            errorDescription << "No valid node ID derived for cluster";
            return (rc * 10) + rc_NETCLUSTER_INVALID_NODE_ID;  // RETURN
        }

        // 3. Create a 'mqbblp::Cluster' since this broker is a member
        Cluster* cluster = new (*clusterAllocator)
            Cluster(name,
                    clusterDefinition,
                    netCluster,
                    d_statContexts,
                    d_domainFactory_p,
                    d_dispatcher_p,
                    d_transportManager_p,
                    &d_stopRequestsManager,
                    d_resources,
                    clusterAllocator,
                    d_adminCb);

        info.d_cluster_sp.reset(cluster, clusterAllocator);
        info.d_eventProcessor_p = cluster;
    }
    else {
        // 1. Fetch the cluster proxy definition
        ClusterProxyDefinitionConstIter proxyDefinitionIter = bsl::find_if(
            d_clustersDefinition.proxyClusters().begin(),
            d_clustersDefinition.proxyClusters().end(),
            Named(name));

        if (proxyDefinitionIter ==
            d_clustersDefinition.proxyClusters().end()) {
            errorDescription << "Fetch proxy definition failed for cluster: '"
                             << name << "'";
            return rc_FETCH_DEFINITION_FAILED;  // RETURN
        }

        const mqbcfg::ClusterProxyDefinition& clusterProxyDefinition =
            *proxyDefinitionIter;

        // 2. Create the mqbnet::Cluster
        rc = createNetCluster(errorDescription,
                              &netCluster,
                              name,
                              clusterProxyDefinition.nodes());
        if (rc != 0) {
            return (rc * 10) + rc_NETCLUSTER_CREATION_FAILED;  // RETURN
        }

        // 3. Create a 'mqbblp::ClusterProxy' since this broker is not a member
        ClusterProxy* cluster = new (*clusterAllocator)
            ClusterProxy(name,
                         clusterProxyDefinition,
                         netCluster,
                         d_statContexts,
                         d_dispatcher_p,
                         d_transportManager_p,
                         &d_stopRequestsManager,
                         d_resources,
                         clusterAllocator);

        info.d_cluster_sp.reset(cluster, clusterAllocator);
        info.d_eventProcessor_p = cluster;
    }

    // 4. Save the cluster in the map
    d_clusters[name] = info;
    if (out) {
        *out = info.d_cluster_sp;
    }

    BALL_LOG_INFO << "Cluster '" << name << "' created [selfNodeId: "
                  << info.d_cluster_sp->netCluster().selfNodeId() << "]";

    return 0;
}

int ClusterCatalog::startCluster(bsl::ostream&  errorDescription,
                                 mqbi::Cluster* cluster)
{
    // mutex must NOT be locked

    // Starting the cluster starts its associated 'StorageManager' which may be
    // a heavy long synchronous operation (if it has to synchronize/recover the
    // files disk).  Clusters are created at startup and we want to block the
    // startup of the broker until it is ready, i.e., the Cluster(s) it is
    // member of have been created, synchronized and ready.  However, during
    // creation of an 'mqbnet::Cluster', the 'SessionNegotiator' will query
    // this 'ClusterCatalog' object, and so in order to avoid long wait and
    // contention on the IO (which potentially could lead to negotiation time
    // out), we don't want (and don't have) to start the Cluster under the
    // mutex locked.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster);

    int rc = cluster->start(errorDescription);

    if (rc != 0) {
        // Failed to start the cluster, remove it from the map for a clean
        // shutdown
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK
        d_clusters.erase(cluster->name());
    }

    return rc;
}

int ClusterCatalog::initiateReversedClusterConnectionsImp(
    bsl::ostream&                         errorDescription,
    const ReversedClusterConnectionArray& connections)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS               = 0,
        rc_CLUSTER_NOT_FOUND     = -1,
        rc_SELF_NOT_FOUND        = -2,
        rc_UNSUPPORTED_TRANSPORT = -3,
        rc_CONNECTION_FAILED     = -4
    };

    int rc = rc_SUCCESS;

    for (size_t itCluster = 0; itCluster < connections.size(); ++itCluster) {
        const mqbcfg::ReversedClusterConnection& clusterConnection =
            connections[itCluster];

        ClusterProxyDefinitionConstIter proxyDefinitionIter = bsl::find_if(
            d_clustersDefinition.proxyClusters().begin(),
            d_clustersDefinition.proxyClusters().end(),
            Named(clusterConnection.name()));
        if (proxyDefinitionIter ==
            d_clustersDefinition.proxyClusters().end()) {
            errorDescription << " (while trying to establish "
                             << "reverse connections)";
            return rc_CLUSTER_NOT_FOUND;  // RETURN
        }

        const mqbcfg::ClusterProxyDefinition& clusterProxyDefinition =
            *proxyDefinitionIter;

        const int myNodeId = d_transportManager_p->selfNodeId(
            clusterProxyDefinition.nodes());
        if (myNodeId == -1) {
            errorDescription << "Unable to find self in cluster '"
                             << clusterConnection.name() << "' while trying "
                             << "to establish reverse connections";
            return rc_SELF_NOT_FOUND;  // RETURN
        }

        // Establish reversed cluster connections for the cluster
        for (size_t itConnection = 0;
             itConnection < clusterConnection.connections().size();
             ++itConnection) {
            const mqbcfg::ClusterNodeConnection& nodeConnection =
                clusterConnection.connections()[itConnection];

            if (!nodeConnection.isTcpValue()) {
                errorDescription << "Unsupported transport in cluster '"
                                 << clusterConnection.name()
                                 << "': " << nodeConnection;
                return rc_UNSUPPORTED_TRANSPORT;  // RETURN
            }

            NegotiationUserData* userData = new (*d_allocator_p)
                NegotiationUserData;
            userData->d_clusterName         = clusterConnection.name();
            userData->d_myNodeId            = myNodeId;
            userData->d_isClusterConnection = false;

            bslma::ManagedPtr<void> userDataMp(userData, d_allocator_p);

            rc = d_transportManager_p->connectOut(
                errorDescription,
                nodeConnection.tcp().endpoint(),
                &userDataMp);
            if (rc != 0) {
                return (rc * 10) + rc_CONNECTION_FAILED;  // RETURN
            }

            BALL_LOG_INFO << "Initiated reversed cluster connection from '"
                          << clusterConnection.name() << "' to '"
                          << nodeConnection.tcp().endpoint() << "'";
        }
    }

    return rc;
}

ClusterCatalog::ClusterCatalog(mqbi::Dispatcher*             dispatcher,
                               mqbnet::TransportManager*     transportManager,
                               const StatContextsMap&        statContexts,
                               const mqbi::ClusterResources& resources,
                               bslma::Allocator*             allocator)
: d_allocator_p(allocator)
, d_allocators(d_allocator_p)
, d_isStarted(false)
, d_dispatcher_p(dispatcher)
, d_transportManager_p(transportManager)
, d_domainFactory_p(0)
, d_clustersDefinition(d_allocator_p)
, d_myClusters(d_allocator_p)
, d_myVirtualClusters(d_allocator_p)
, d_myReverseClusters(d_allocator_p)
, d_reversedClusterConnections(d_allocator_p)
, d_clusters(d_allocator_p)
, d_statContexts(statContexts)
, d_resources(resources)
, d_adminCb()
, d_requestManager(bmqp::EventType::e_CONTROL,
                   resources.blobSpPool(),
                   resources.scheduler(),
                   false,  // lateResponseMode
                   d_allocator_p)
, d_stopRequestsManager(&d_requestManager, d_allocator_p)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_resources.scheduler()->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);
}

ClusterCatalog::~ClusterCatalog()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

int ClusterCatalog::loadBrokerClusterConfig(bsl::ostream&)
{
    // executed by the *MAIN* thread

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                           = 0,
        rc_BROKER_CLUSTER_CONFIG_LOADFAILURE = -1,
        rc_CONFIG_READ_ERROR                 = -1,
        rc_JSON_DECODE_ERROR                 = -2
    };

    int rc = rc_SUCCESS;

    const mqbcfg::AppConfig& brkrCfg        = mqbcfg::BrokerConfig::get();
    bsl::string              configFilename = brkrCfg.etcDir();

    if (bdls::PathUtil::appendIfValid(&configFilename, "clusters.json")) {
        BALL_LOG_ERROR << "Invalid path '" << configFilename << "'\n";
        return rc_CONFIG_READ_ERROR * 10 +
               rc_BROKER_CLUSTER_CONFIG_LOADFAILURE;  // RETURN
    }

    bsl::ifstream      configStream(configFilename.c_str());
    bmqu::MemOutStream configParameters;
    configParameters << configStream.rdbuf();

    if (!configStream || !configParameters) {
        BALL_LOG_ERROR << "Error while reading JSON from '" << configFilename
                       << "'\n";
        return rc_CONFIG_READ_ERROR * 10 +
               rc_BROKER_CLUSTER_CONFIG_LOADFAILURE;  // RETURN
    }
    configStream.close();

    // 2. Decode the JSON stream
    baljsn::Decoder            decoder;
    baljsn::DecoderOptions     options;
    bslstl::StringRef          jsonString = configParameters.str();
    bdlsb::FixedMemInStreamBuf jsonStreamBuf(jsonString.data(),
                                             jsonString.length());

    options.setSkipUnknownElements(true);

    rc = decoder.decode(&jsonStreamBuf, &d_clustersDefinition, options);
    if (rc != 0) {
        BALL_LOG_ERROR << "Error while decoding JSON from '" << configFilename
                       << "' [rc: " << rc << ", error: '"
                       << decoder.loggedMessages() << "']"
                       << ", content:\n"
                       << configParameters.str();
        return rc_JSON_DECODE_ERROR * 10 +
               rc_BROKER_CLUSTER_CONFIG_LOADFAILURE;  // RETURN
    }

    BALL_LOG_INFO << "Read config from " << configFilename << ": "
                  << jsonString;

    d_myReverseClusters.insert(
        d_clustersDefinition.myReverseClusters().begin(),
        d_clustersDefinition.myReverseClusters().end());
    d_reversedClusterConnections =
        d_clustersDefinition.reversedClusterConnections();

    struct local {
        static const bsl::string&
        clusterName(const mqbcfg::ClusterDefinition& cluster)
        {
            return cluster.name();
        }
    };

    bsl::transform(d_clustersDefinition.myClusters().begin(),
                   d_clustersDefinition.myClusters().end(),
                   bsl::inserter(d_myClusters, d_myClusters.begin()),
                   local::clusterName);

    // Populate the virtual clusters map
    for (bsl::vector<mqbcfg::VirtualClusterInformation>::const_iterator it =
             d_clustersDefinition.myVirtualClusters().begin();
         it != d_clustersDefinition.myVirtualClusters().end();
         ++it) {
        d_myVirtualClusters[it->name()] = it->selfNodeId();
    }

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << "Broker clusters configuration loaded "
                               << "successfully.";
        if (d_myClusters.empty()) {
            BALL_LOG_OUTPUT_STREAM << "  I am *NOT* member of any cluster.";
        }
        else {
            bmqu::Printer<bsl::unordered_set<bsl::string> > printer(
                &d_myClusters);
            BALL_LOG_OUTPUT_STREAM
                << "  I am a member of the following clusters: " << printer;
        }

        if (!d_myReverseClusters.empty()) {
            bmqu::Printer<bsl::unordered_set<bsl::string> > printer(
                &d_myReverseClusters);
            BALL_LOG_OUTPUT_STREAM
                << "\n  The following clusters will remote connect to me '"
                << printer << "'.";
        }

        if (!d_reversedClusterConnections.empty()) {
            BALL_LOG_OUTPUT_STREAM
                << "\n  I will reverse connect to the following hosts:";
            for (size_t i = 0; i < d_reversedClusterConnections.size(); ++i) {
                const mqbcfg::ReversedClusterConnection& clusterConnection =
                    d_reversedClusterConnections[i];
                bmqu::Printer<bsl::vector<mqbcfg::ClusterNodeConnection> >
                    printer(&clusterConnection.connections());
                BALL_LOG_OUTPUT_STREAM << "\n    '" << clusterConnection.name()
                                       << "': " << printer;
            }
        }

        if (!d_myVirtualClusters.empty()) {
            bmqu::Printer<bsl::unordered_map<bsl::string, int> > printer(
                &d_myVirtualClusters);
            BALL_LOG_OUTPUT_STREAM
                << "\n  I am a member of the following VIRTUAL clusters: "
                << printer << ".";
        }
    }

    return rc_SUCCESS;
}

int ClusterCatalog::start(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "start() can only be called once on this object");

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                 = 0,
        rc_CLUSTER_CREATION_FAILED = -1,
        rc_CLUSTER_START_FAILED    = -2
    };

    BALL_LOG_INFO << "Starting ClusterCatalog";

    int rc = rc_SUCCESS;
    // Create a map composed of all clusters, whether member or remote.
    bsl::unordered_set<bsl::string> allClusters(d_myClusters);
    allClusters.insert(d_myReverseClusters.begin(), d_myReverseClusters.end());

    // Create any cluster this broker is member of
    bsl::unordered_set<bsl::string>::const_iterator it;
    for (it = allClusters.begin(); it != allClusters.end(); ++it) {
        bsl::shared_ptr<mqbi::Cluster> cluster;
        // CreateCluster expects unique cluster names, but that's fine, we are
        // iterating over a set, so names are guaranteed to be unique.
        {
            bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
            rc = createCluster(errorDescription, &cluster, *it);
            if (rc != 0) {
                return (rc * 10) + rc_CLUSTER_CREATION_FAILED;  // RETURN
            }
        }  // close mutex guard scope

        // Start the newly created cluster, outside the mutex scope
        rc = startCluster(errorDescription, cluster.get());
        if (rc != 0) {
            return (rc * 10) + rc_CLUSTER_START_FAILED;  // RETURN
        }
    }

    d_isStarted = true;

    return rc;
}

void ClusterCatalog::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }
    d_isStarted = false;

    // Iterate over all clusters and stop them
    for (ClustersMapIter it = d_clusters.begin(); it != d_clusters.end();
         ++it) {
        it->second.d_cluster_sp->stop();
    }

    // Clear maps (so that a 'start' called after 'stop' will start 'fresh')
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

        d_clusters.clear();
        d_myClusters.clear();
    }
}

int ClusterCatalog::initiateReversedClusterConnections(
    bsl::ostream& errorDescription)
{
    return initiateReversedClusterConnectionsImp(errorDescription,
                                                 d_reversedClusterConnections);
}

void ClusterCatalog::setDomainFactory(mqbi::DomainFactory* domainFactory)
{
    BSLS_ASSERT(domainFactory);
    d_domainFactory_p = domainFactory;
}

bmqp_ctrlmsg::Status
ClusterCatalog::getCluster(bsl::shared_ptr<mqbi::Cluster>* out,
                           const bslstl::StringRef&        name)
{
    // NOTE: The 'out' should maybe be returned as a shared_ptr with a custom
    //       deleter, so that the cluster can be destroyed once no longer used
    //       (with eventually a small TTL).

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    bmqp_ctrlmsg::Status status;
    status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    status.code()     = 0;
    status.message()  = "";

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

    // Check if we already have created that cluster
    ClustersMapIter it = d_clusters.find(name);
    if (it != d_clusters.end()) {
        *out = it->second.d_cluster_sp;
        return status;  // RETURN
    }

    // Cluster not found, create a new one if self is not stopping.
    if (!d_isStarted) {
        status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()     = mqbi::ClusterErrorCode::e_STOPPING;  // Retryable
        status.message()  = "ClusterCatalog is stopping, not creating [" +
                           name + "] cluster at this time.";
        return status;  // RETURN
    }

    bsl::shared_ptr<mqbi::Cluster> cluster;
    bmqu::MemOutStream             errorDesc;
    int rc = createCluster(errorDesc, &cluster, name);
    if (rc != 0) {
        status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code() = mqbi::ClusterErrorCode::e_UNKNOWN;  // Non-retryable
        status.message().assign(errorDesc.str().data(),
                                errorDesc.str().length());

        return status;  // RETURN
    }

    // Start the newly created cluster, outside the mutex scope
    guard.release()->unlock();  // d_mutex UNLOCK

    rc = startCluster(errorDesc, cluster.get());
    if (rc != 0) {
        status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code() = mqbi::ClusterErrorCode::e_UNKNOWN;  // Non-retryable
        status.message().assign(errorDesc.str().data(),
                                errorDesc.str().length());

        return status;  // RETURN
    }

    *out = cluster;
    return status;
}

bool ClusterCatalog::findCluster(bsl::shared_ptr<mqbi::Cluster>* out,
                                 const bslstl::StringRef&        name)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

    ClustersMapIter it = d_clusters.find(name);
    if (it != d_clusters.end()) {
        *out = it->second.d_cluster_sp;
        return true;  // RETURN
    }
    return false;
}

int ClusterCatalog::count()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK
    return d_clusters.size();
}

mqbnet::ClusterNode* ClusterCatalog::onNegotiationForClusterSession(
    bsl::ostream&              errorDescription,
    mqbnet::NegotiatorContext* context,
    const bslstl::StringRef&   clusterName,
    int                        nodeId)
{
    mqbnet::ClusterNode* clusterNode = 0;

    // 1. Lookup the cluster and populate eventProcessor
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

        ClustersMapIter it = d_clusters.find(clusterName);
        if (it == d_clusters.end()) {
            errorDescription << "Cluster '" << clusterName << "' not found";
            return 0;  // RETURN
        }
        context->setCluster(&it->second.d_cluster_sp->netCluster());
        context->setEventProcessor(it->second.d_eventProcessor_p);
    }

    // 2. Lookup the nodeId and populate resultState
    void* resultState = d_transportManager_p->getClusterNodeAndState(
        errorDescription,
        &clusterNode,
        clusterName,
        nodeId);
    if (!resultState) {
        // 'errorDescription' was populated by 'getStateForClusterNode'
        return 0;  // RETURN
    }
    context->setResultState(resultState);

    return clusterNode;
}

int ClusterCatalog::processCommand(mqbcmd::ClustersResult*        result,
                                   const mqbcmd::ClustersCommand& command)
{
    if (command.isListValue()) {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        mqbcmd::ClusterList& clusterList = result->makeClusterList();
        clusterList.clusters().reserve(d_clusters.size());
        for (ClustersMapConstIter it = d_clusters.begin();
             it != d_clusters.end();
             ++it) {
            clusterList.clusters().resize(clusterList.clusters().size() + 1);
            mqbcmd::ClusterInfo& clusterInfo = clusterList.clusters().back();

            if (it->second.d_cluster_sp->isLocal()) {
                clusterInfo.locality() = mqbcmd::Locality::LOCAL;
            }
            else if (it->second.d_cluster_sp->isRemote()) {
                clusterInfo.locality() = mqbcmd::Locality::REMOTE;
            }
            else if (it->second.d_cluster_sp->isClusterMember()) {
                clusterInfo.locality() = mqbcmd::Locality::MEMBER;
            }
            clusterInfo.name() = it->first;
            const mqbnet::Cluster::NodesList& nodes =
                it->second.d_cluster_sp->netCluster().nodes();

            clusterInfo.nodes().reserve(nodes.size());
            for (mqbnet::Cluster::NodesList::const_iterator itNode =
                     nodes.cbegin();
                 itNode != nodes.cend();
                 ++itNode) {
                clusterInfo.nodes().resize(clusterInfo.nodes().size() + 1);
                mqbcmd::ClusterNode& clusterNode = clusterInfo.nodes().back();

                clusterNode.hostName()   = (*itNode)->hostName();
                clusterNode.nodeId()     = (*itNode)->nodeId();
                clusterNode.dataCenter() = (*itNode)->dataCenter();
            }
        }

        return 0;  // RETURN
    }
    else if (command.isAddReverseProxyValue()) {
        ReversedClusterConnectionArray    connectionArray;
        mqbcfg::ReversedClusterConnection connection;
        mqbcfg::ClusterNodeConnection     nodeConnection;

        nodeConnection.makeTcp().endpoint() =
            command.addReverseProxy().remotePeer();
        connection.name() = command.addReverseProxy().clusterName();
        connection.connections().push_back(nodeConnection);
        connectionArray.push_back(connection);

        bmqu::MemOutStream os;
        int rc = initiateReversedClusterConnectionsImp(os, connectionArray);
        if (rc != 0) {
            result->makeError().message() = os.str() +
                                            " [rc: " + bsl::to_string(rc) +
                                            "]";
        }
        else {
            result->makeSuccess();
        }

        return rc;  // RETURN
    }
    else if (command.isClusterValue()) {
        bsl::shared_ptr<mqbi::Cluster> cluster;
        {
            bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

            const ClustersMapIter it = d_clusters.find(
                command.cluster().name());
            if (it == d_clusters.end()) {
                bmqu::MemOutStream os;
                os << "cluster '" << command.cluster().name()
                   << "' not found !";
                result->makeError().message() = os.str();
                return -1;  // RETURN
            }

            cluster = it->second.d_cluster_sp;
        }

        BSLS_ASSERT_SAFE(cluster);

        // Invoke command on the cluster
        mqbcmd::ClusterResult clusterResult;
        int                   rc = cluster->processCommand(&clusterResult,
                                         command.cluster().command());
        if (clusterResult.isErrorValue()) {
            result->makeError(clusterResult.error());
            return -1;  // RETURN
        }
        else if (clusterResult.isSuccessValue()) {
            result->makeSuccess(clusterResult.success());
            return rc;  // RETURN
        }
        else {
            result->makeClusterResult(clusterResult);
            return rc;  // RETURN
        }
    }

    bmqu::MemOutStream os;
    os << "Unknown command '" << command << "'";
    result->makeError().message() = os.str();
    return -1;
}

int ClusterCatalog::selfNodeIdInCluster(const bslstl::StringRef& name) const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

    ClustersMapConstIter cit = d_clusters.find(name);
    if (cit != d_clusters.end()) {
        return cit->second.d_cluster_sp->netCluster().selfNodeId();  // RETURN
    }

    // Check in the virtual cluster map
    VirtualClustersMap::const_iterator vit = d_myVirtualClusters.find(name);
    if (vit != d_myVirtualClusters.end()) {
        return vit->second;  // RETURN
    }

    return mqbnet::Cluster::k_INVALID_NODE_ID;
}

bool ClusterCatalog::isClusterVirtual(const bslstl::StringRef& name) const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

    return d_myVirtualClusters.find(name) != d_myVirtualClusters.end();
}

}  // close package namespace
}  // close enterprise namespace
