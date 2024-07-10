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

// mqba_commandrouter.cpp
#include <mqba_commandrouter.h>

// MQB
#include <mqbcmd_parseutil.h>
#include <mqbi_cluster.h>
#include <mqbnet_cluster.h>
#include <mqbnet_multirequestmanager.h>

// BDE
#include <ball_log.h>
#include <bsl_iostream.h>

namespace BloombergLP {
namespace mqba {

namespace {
const char k_LOG_CATEGORY[] = "MQBA.COMMANDROUTER";
}  // close unnamed namespace

RouteCommandManager::RouteCommandManager(const bsl::string& commandString,
                                         const mqbcmd::CommandChoice& command)
: d_commandString(commandString)
, d_command(command)
, d_routingMode(getCommandRoutingMode())
, d_latch(1)
{
    // Effectively negate the latch if we won't need to wait for responses.
    if (!isRoutingNeeded()) {
        countDownLatch();
    }
}

RouteCommandManager::RoutingMode::RoutingMode(RouteCommandManager* router)
: d_router(router)
{
}

RouteCommandManager::RoutingMode::~RoutingMode()
{
}

RouteCommandManager::AllPartitionPrimariesRoutingMode::
    AllPartitionPrimariesRoutingMode(RouteCommandManager* router)
: RoutingMode(router)
{
    BSLS_ASSERT_SAFE(router);
}

RouteCommandManager::RouteMembers
RouteCommandManager::AllPartitionPrimariesRoutingMode::getRouteMembers()
{
    NodesVector primaryNodes;
    bool        isSelfPrimary;

    router()->cluster()->dispatcher()->execute(
        bdlf::BindUtil::bind(&mqbi::Cluster::getPrimaryNodes,
                             router()->cluster(),
                             &primaryNodes,
                             &isSelfPrimary),
        router()->cluster());

    router()->cluster()->dispatcher()->synchronize(router()->cluster());

    return {primaryNodes, isSelfPrimary};
}

RouteCommandManager::SinglePartitionPrimaryRoutingMode::
    SinglePartitionPrimaryRoutingMode(RouteCommandManager* router,
                                      int                  partitionId)
: RoutingMode(router)
, d_partitionId(partitionId)
{
    BSLS_ASSERT_SAFE(router);
}

RouteCommandManager::RouteMembers
RouteCommandManager::SinglePartitionPrimaryRoutingMode::getRouteMembers()
{
    mqbnet::ClusterNode* node          = nullptr;
    bool                 isSelfPrimary = false;

    router()->cluster()->dispatcher()->execute(
        bdlf::BindUtil::bind(&mqbi::Cluster::getPartitionPrimaryNode,
                             router()->cluster(),
                             &node,
                             &isSelfPrimary,
                             d_partitionId),
        router()->cluster());

    router()->cluster()->dispatcher()->synchronize(router()->cluster());

    NodesVector nodes;
    if (node) {
        // Put node into vector to be acceptable for "routeCommand"
        nodes.push_back(node);
    }

    return {nodes, isSelfPrimary};
}

RouteCommandManager::ClusterRoutingMode::ClusterRoutingMode(
    RouteCommandManager* router)
: RoutingMode(router)
{
    BSLS_ASSERT_SAFE(router);
}

RouteCommandManager::RouteMembers
RouteCommandManager::ClusterRoutingMode::getRouteMembers()
{
    typedef mqbnet::Cluster::NodesList NodesList;
    // collect all nodes in cluster
    const NodesList& allNodes = router()->cluster()->netCluster().nodes();

    NodesVector nodes;

    for (NodesList::const_iterator nit = allNodes.begin();
         nit != allNodes.end();
         nit++) {
        if (router()->cluster()->netCluster().selfNode() != *nit) {
            nodes.push_back(*nit);
        }
    }

    return {
        nodes,
        true  // Cluster routing always requires original node to exec.
    };
}

bool RouteCommandManager::process(mqbi::Cluster* cluster)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_routingMode);

    d_cluster = cluster;

    RouteMembers routeMembers = d_routingMode->getRouteMembers();

    if (routeMembers.nodes.size() > 0) {
        routeCommand(routeMembers.nodes);
    }
    else {
        countDownLatch();
    }

    return routeMembers.self;
}

void RouteCommandManager::onRouteCommandResponse(
    const MultiRequestContextSp& requestContext)
{
    typedef bsl::pair<mqbnet::ClusterNode*, bmqp_ctrlmsg::ControlMessage>
                                  NodePair;
    typedef bsl::vector<NodePair> NodePairsVector;

    NodePairsVector responsePairs = requestContext->response();

    for (NodePairsVector::const_iterator pairIt = responsePairs.begin();
         pairIt != responsePairs.end();
         pairIt++) {
        NodePair pair = *pairIt;

        bmqp_ctrlmsg::ControlMessage& message = pair.second;

        mqbcmd::RouteResponse routeResponse;
        routeResponse.source() = pair.first->hostName();
        if (message.choice().isAdminCommandResponseValue()) {
            const bsl::string& output =
                message.choice().adminCommandResponse().text();
            routeResponse.response() = output;
        }
        else {
            // Something went wrong, possibly timed out
            routeResponse.response() =
                "Error ocurred sending command to node " +
                pair.first->hostName();
        }
        d_responses.responses().push_back(routeResponse);
    }

    countDownLatch();
}

RouteCommandManager::RoutingModeMp RouteCommandManager::getCommandRoutingMode()
{
    RouteCommandManager::RoutingMode* routingMode = nullptr;
    if (d_command.isDomainsValue()) {
        const mqbcmd::DomainsCommand& domains = d_command.domains();
        if (domains.isDomainValue()) {
            const mqbcmd::DomainCommand& domain = domains.domain().command();
            if (domain.isPurgeValue()) {
                routingMode = new AllPartitionPrimariesRoutingMode(
                    this);  // DOMAINS DOMAIN <name> PURGE
            }
            else if (domain.isQueueValue()) {
                if (domain.queue().command().isPurgeAppIdValue()) {
                    routingMode = new AllPartitionPrimariesRoutingMode(
                        this);  // DOMAINS DOMAIN <name> QUEUE <name> PURGE
                }
            }
        }
        else if (domains.isReconfigureValue()) {
            routingMode = new ClusterRoutingMode(
                this);  // DOMAINS RECONFIGURE <domain>
        }
    }
    else if (d_command.isClustersValue()) {
        const mqbcmd::ClustersCommand& clusters = d_command.clusters();
        if (clusters.isClusterValue()) {
            const mqbcmd::ClusterCommand& cluster =
                clusters.cluster().command();
            if (cluster.isForceGcQueuesValue()) {
                routingMode = new ClusterRoutingMode(
                    this);  // CLUSTERS CLUSTER <name> FORCE_GC_QUEUES
            }
            else if (cluster.isStorageValue()) {
                const mqbcmd::StorageCommand& storage = cluster.storage();
                if (storage.isPartitionValue()) {
                    if (storage.partition().command().isEnableValue() ||
                        storage.partition().command().isDisableValue()) {
                        int partitionId = storage.partition().partitionId();
                        routingMode = new SinglePartitionPrimaryRoutingMode(
                            this,
                            partitionId);  // CLUSTERS CLUSTER <name> STORAGE
                                           // PARTITION <partitionId>
                                           // [ENABLE|DISABLE]
                    }
                    // SUMMARY doesn't need to route to primary
                }
                if (storage.isReplicationValue()) {
                    const mqbcmd::ReplicationCommand& replication =
                        storage.replication();
                    if (replication.isSetTunableValue()) {
                        const mqbcmd::SetTunable& tunable =
                            replication.setTunable();
                        if (tunable.choice().isAllValue()) {
                            routingMode = new ClusterRoutingMode(
                                this);  // CLUSTERS CLUSTER <name> STORAGE
                                        // REPLICATION SET_ALL
                        }
                    }
                    else if (replication.isGetTunableValue()) {
                        const mqbcmd::GetTunable& tunable =
                            replication.getTunable();
                        if (tunable.choice().isAllValue()) {
                            routingMode = new ClusterRoutingMode(
                                this);  // CLUSTERS CLUSTER <name> STORAGE
                                        // REPLICATION GET_ALL
                        }
                    }
                }
            }
            else if (cluster.isStateValue()) {
                const mqbcmd::ClusterStateCommand state = cluster.state();
                if (state.isElectorValue()) {
                    const mqbcmd::ElectorCommand elector = state.elector();
                    if (elector.isSetTunableValue()) {
                        const mqbcmd::SetTunable& tunable =
                            elector.setTunable();
                        if (tunable.choice().isAllValue()) {
                            routingMode = new ClusterRoutingMode(
                                this);  // CLUSTERS CLUSTER <name> STATE
                                        // ELECTOR SET_ALL
                        }
                    }
                    else if (elector.isGetTunableValue()) {
                        const mqbcmd::GetTunable& tunable =
                            elector.getTunable();
                        if (tunable.choice().isAllValue()) {
                            routingMode = new ClusterRoutingMode(
                                this);  // CLUSTERS CLUSTER <name> STATE
                                        // ELECTOR GET_ALL
                        }
                    }
                }
            }
        }
    }

    bslma::ManagedPtr<RoutingMode> ret(routingMode);

    return ret;
}

void RouteCommandManager::routeCommand(const NodesVector& nodes)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster);

    typedef mqbnet::MultiRequestManager<bmqp_ctrlmsg::ControlMessage,
                                        bmqp_ctrlmsg::ControlMessage,
                                        mqbnet::ClusterNode*>::RequestContextSp
        RequestContextSp;

    RequestContextSp contextSp =
        d_cluster->multiRequestManager().createRequestContext();

    bmqp_ctrlmsg::AdminCommand& adminCommand =
        contextSp->request().choice().makeAdminCommand();

    adminCommand.command()  = d_commandString;
    adminCommand.rerouted() = true;

    contextSp->setDestinationNodes(nodes);

    mwcu::MemOutStream os;
    os << "Routing command to the following nodes [";
    for (NodesVector::const_iterator nit = nodes.begin(); nit != nodes.end();
         nit++) {
        os << (*nit)->hostName();
        if (nit + 1 != nodes.end()) {
            os << ", ";
        }
    }
    os << "]";
    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);
    BALL_LOG_INFO << os.str();

    contextSp->setResponseCb(
        bdlf::BindUtil::bind(&RouteCommandManager::onRouteCommandResponse,
                             this,
                             bdlf::PlaceHolders::_1));

    d_cluster->multiRequestManager().sendRequest(contextSp,
                                                 bsls::TimeInterval(3));
}

}  // close package namespace
}  // close enterprise namespace
