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
#include <bsl_iostream.h>
#include <ball_log.h>

namespace BloombergLP {
namespace mqba {

CommandRouter::CommandRouter(const bsl::string& commandString, const mqbcmd::Command& command)
: d_allPartitionPrimariesRouter(this)
, d_singlePartitionPrimaryRouter(this)
, d_clusterRouter(this)
, d_latch(1)
, d_commandString(commandString)
, d_commandWithOptions(command)
, d_command(command.choice())
, d_router(getCommandRouter())
{
    // Effectively negate the latch if we won't need to wait for responses.
    if (!isRoutingNeeded()) {
        countDownLatch();
    }
}

CommandRouter::Router::Router(CommandRouter* router)
: d_router(router)
{
}

CommandRouter::Router::~Router()
{
}

CommandRouter::AllPartitionPrimariesRouter::AllPartitionPrimariesRouter(
    CommandRouter* router)
: Router(router)
{
    BSLS_ASSERT_SAFE(router);
}

bool CommandRouter::AllPartitionPrimariesRouter::routeCommand()
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

    if (primaryNodes.size() > 0) {
        router()->routeCommand(primaryNodes);
    }
    else {
        // If there are no nodes to wait on then immediately count the latch.
        router()->countDownLatch();
    }

    return isSelfPrimary;
}

CommandRouter::SinglePartitionPrimaryRouter::SinglePartitionPrimaryRouter(
    CommandRouter* router)
: Router(router)
{
    BSLS_ASSERT_SAFE(router);
}

bool CommandRouter::SinglePartitionPrimaryRouter::routeCommand()
{
    mqbnet::ClusterNode* node = nullptr;
    bool        isSelfPrimary = false;

    router()->cluster()->dispatcher()->execute(
        bdlf::BindUtil::bind(&mqbi::Cluster::getPartitionPrimaryNode,
                             router()->cluster(),
                             &node,
                             &isSelfPrimary,
                             d_partitionId),
        router()->cluster());

    router()->cluster()->dispatcher()->synchronize(router()->cluster());

    if (node) {
        // Put node into vector to be acceptable for "routeCommand"
        NodesVector nodes;
        nodes.push_back(node);
        router()->routeCommand(nodes);
    }
    else {
        // Only self will execute, so count down latch immediately
        router()->countDownLatch();
    }

    return isSelfPrimary;
}

CommandRouter::ClusterRouter::ClusterRouter(CommandRouter* router)
: Router(router)
{
    BSLS_ASSERT_SAFE(router);
}

bool CommandRouter::ClusterRouter::routeCommand()
{
    // collect all nodes in cluster
    const mqbnet::Cluster::NodesList& allNodes = router()->cluster()->netCluster().nodes();

    NodesVector nodes;

    for (mqbnet::Cluster::NodesList::const_iterator nit = allNodes.begin();
         nit != allNodes.end();
         nit++) {
        if (router()->cluster()->netCluster().selfNode() != *nit) {
            nodes.push_back(*nit);
        }
    }

    if (nodes.size() > 0) {
        router()->routeCommand(nodes);
    }
    else {
        router()->countDownLatch();
    }    

    return true; // Cluster routing always requires original node to exec.
}

bool CommandRouter::isRoutingNeeded() const {
    return d_router != nullptr;
}

bool CommandRouter::processCommand(mqbi::Cluster* cluster) {
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_router);

    d_cluster = cluster;

    return d_router->routeCommand();
}

void CommandRouter::onRouteCommandResponse(const MultiRequestContextSp& requestContext) {
    typedef bsl::pair<mqbnet::ClusterNode*, bmqp_ctrlmsg::ControlMessage>
                                  NodePair;
    typedef bsl::vector<NodePair> NodePairsVector;

    NodePairsVector responsePairs = requestContext->response();

    for (NodePairsVector::const_iterator pairIt = responsePairs.begin();
         pairIt != responsePairs.end();
         pairIt++) {
        NodePair pair = *pairIt;

        bmqp_ctrlmsg::ControlMessage& message = pair.second;

        if (message.choice().isAdminCommandResponseValue()) {
            const bsl::string& output =
                message.choice().adminCommandResponse().text();
            d_responses.push_back({pair.first, output});
        }
        else {
            // something went wrong... possibly timeout?
            d_responses.push_back({pair.first,
                                  "Error occurred sending command to node " +
                                      pair.first->hostName()});
        }
    }

    countDownLatch();
}

CommandRouter::Router* CommandRouter::getCommandRouter()
{
    if (d_command.isDomainsValue()) {
        const mqbcmd::DomainsCommand& domains = d_command.domains();
        if (domains.isDomainValue()) {
            const mqbcmd::DomainCommand& domain = domains.domain().command();
            if (domain.isPurgeValue()) {
                return &d_allPartitionPrimariesRouter;  // DOMAINS DOMAIN
                                                        // <name> PURGE
            }
            else if (domain.isQueueValue()) {
                if (domain.queue().command().isPurgeAppIdValue()) {
                    return &d_allPartitionPrimariesRouter;  // DOMAINS DOMAIN
                                                            // <name> QUEUE
                                                            // <name> PURGE
                }
            }
        }
        else if (domains.isReconfigureValue()) {
            return &d_clusterRouter;  // DOMAINS RECONFIGURE <domain>
        }
    }
    else if (d_command.isClustersValue()) {
        const mqbcmd::ClustersCommand& clusters = d_command.clusters();
        if (clusters.isClusterValue()) {
            const mqbcmd::ClusterCommand& cluster =
                clusters.cluster().command();
            if (cluster.isForceGcQueuesValue()) {
                return &d_clusterRouter;  // CLUSTERS CLUSTER <name>
                                          // FORCE_GC_QUEUES
            }
            else if (cluster.isStorageValue()) {
                const mqbcmd::StorageCommand& storage = cluster.storage();
                if (storage.isPartitionValue()) {
                    if (storage.partition().command().isEnableValue() ||
                        storage.partition().command().isDisableValue()) {
                        int partitionID = storage.partition().partitionId();
                        d_singlePartitionPrimaryRouter.setPartitionID(
                            partitionID);
                        return &d_singlePartitionPrimaryRouter;  // CLUSTERS
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

void CommandRouter::routeCommand(const NodesVector& nodes) {
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
        bdlf::BindUtil::bind(&CommandRouter::onRouteCommandResponse,
                             this,
                             bdlf::PlaceHolders::_1));

    d_cluster->multiRequestManager().sendRequest(contextSp,
                                               bsls::TimeInterval(3));
}

}  // close package namespace
}  // close enterprise namespace
