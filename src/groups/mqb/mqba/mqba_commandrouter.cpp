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

RouteCommandManager::RouteCommandManager(const bsl::string&     commandString,
                                         const mqbcmd::Command& command)
: d_allPartitionPrimariesRoutingMode(this)
, d_singlePartitionPrimaryRoutingMode(this)
, d_clusterRoutingMode(this)
, d_latch(1)
, d_commandString(commandString)
, d_commandWithOptions(command)
, d_command(command.choice())
, d_routingMode(getCommandRoutingMode())
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
    SinglePartitionPrimaryRoutingMode(RouteCommandManager* router)
: RoutingMode(router)
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
    // collect all nodes in cluster
    const mqbnet::Cluster::NodesList& allNodes =
        router()->cluster()->netCluster().nodes();

    NodesVector nodes;

    for (mqbnet::Cluster::NodesList::const_iterator nit = allNodes.begin();
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

bool RouteCommandManager::isRoutingNeeded() const
{
    return d_routingMode != nullptr;
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
            // d_responses.push_back({pair.first, output});
        }
        else {
            // something went wrong... possibly timeout?

            routeResponse.response() =
                "Error ocurred sending command to node " +
                pair.first->hostName();

            // d_responses.push_back({pair.first,
            //   "Error occurred sending command to node " +
            //   pair.first->hostName()});
        }
        d_responses.responses().push_back(routeResponse);
    }

    countDownLatch();
}

RouteCommandManager::RoutingMode* RouteCommandManager::getCommandRoutingMode()
{
    if (d_command.isDomainsValue()) {
        const mqbcmd::DomainsCommand& domains = d_command.domains();
        if (domains.isDomainValue()) {
            const mqbcmd::DomainCommand& domain = domains.domain().command();
            if (domain.isPurgeValue()) {
                return &d_allPartitionPrimariesRoutingMode;  // DOMAINS DOMAIN
                                                             // <name> PURGE
            }
            else if (domain.isQueueValue()) {
                if (domain.queue().command().isPurgeAppIdValue()) {
                    return &d_allPartitionPrimariesRoutingMode;  // DOMAINS
                                                                 // DOMAIN
                                                                 // <name>
                                                                 // QUEUE
                                                                 // <name>
                                                                 // PURGE
                }
            }
        }
        else if (domains.isReconfigureValue()) {
            return &d_clusterRoutingMode;  // DOMAINS RECONFIGURE <domain>
        }
    }
    else if (d_command.isClustersValue()) {
        const mqbcmd::ClustersCommand& clusters = d_command.clusters();
        if (clusters.isClusterValue()) {
            const mqbcmd::ClusterCommand& cluster =
                clusters.cluster().command();
            if (cluster.isForceGcQueuesValue()) {
                return &d_clusterRoutingMode;  // CLUSTERS CLUSTER <name>
                                               // FORCE_GC_QUEUES
            }
            else if (cluster.isStorageValue()) {
                const mqbcmd::StorageCommand& storage = cluster.storage();
                if (storage.isPartitionValue()) {
                    if (storage.partition().command().isEnableValue() ||
                        storage.partition().command().isDisableValue()) {
                        int partitionID = storage.partition().partitionId();
                        d_singlePartitionPrimaryRoutingMode.setPartitionID(
                            partitionID);
                        return &d_singlePartitionPrimaryRoutingMode;  // CLUSTERS
                                                                      // CLUSTER
                                                                      // <name>
                                                                      // STORAGE
                                                                      // PARTITION
                                                                      // <partitionId>
                                                                      // [ENABLE|DISABLE]
                    }
                    // SUMMARY doesn't need to route to primary
                }
            }
        }
    }

    return nullptr;
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
    os << "Rerouting command to the following nodes [";
    for (NodesVector::const_iterator nit = nodes.begin(); nit != nodes.end();
         nit++) {
        os << (*nit)->hostName();
        if (nit + 1 != nodes.end()) {
            os << ", ";
        }
    }
    os << "]";
    // BALL_LOG_INFO << os.str();

    contextSp->setResponseCb(
        bdlf::BindUtil::bind(&RouteCommandManager::onRouteCommandResponse,
                             this,
                             bdlf::PlaceHolders::_1));

    d_cluster->multiRequestManager().sendRequest(contextSp,
                                                 bsls::TimeInterval(3));
}

}  // close package namespace
}  // close enterprise namespace
