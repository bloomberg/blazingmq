// Copyright 2024 Bloomberg Finance L.P.
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
#include <mqbcfg_brokerconfig.h>
#include <mqbcmd_jsonprinter.h>
#include <mqbcmd_parseutil.h>
#include <mqbi_cluster.h>
#include <mqbnet_cluster.h>
#include <mqbnet_multirequestmanager.h>

// BDE
#include <bsl_iostream.h>
#include <bsla_annotations.h>

namespace BloombergLP {
namespace mqba {

// CREATORS

CommandRouter::CommandRouter(const bsl::string&     commandString,
                             const mqbcmd::Command& command)
: d_commandString(commandString)
, d_command(command)
, d_latch(1)
{
    // Sets the proper routing mode into d_routingModeMp for this command.
    setCommandRoutingMode();

    // Immediately release the latch if we aren't routing anything.
    if (!isRoutingNeeded()) {
        releaseLatch();
    }
}

CommandRouter::RoutingMode::RoutingMode()
{
}

CommandRouter::RoutingMode::~RoutingMode()
{
}

CommandRouter::AllPartitionPrimariesRoutingMode::
    AllPartitionPrimariesRoutingMode()
{
}

int CommandRouter::AllPartitionPrimariesRoutingMode::getRouteTargets(
    bsl::ostream&  errorDescription,
    RouteTargets*  routeTargets,
    mqbi::Cluster* cluster)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(routeTargets);
    BSLS_ASSERT_SAFE(cluster);

    int rc;

    cluster->dispatcher()->execute(
        bdlf::BindUtil::bind(&mqbi::Cluster::getPrimaryNodes,
                             cluster,
                             &rc,
                             bsl::ref(errorDescription),
                             &routeTargets->d_nodes,
                             &routeTargets->d_self),
        cluster);

    cluster->dispatcher()->synchronize(cluster);

    return rc;  // RETURN
}

CommandRouter::SinglePartitionPrimaryRoutingMode::
    SinglePartitionPrimaryRoutingMode(int partitionId)
: d_partitionId(partitionId)
{
}

int CommandRouter::SinglePartitionPrimaryRoutingMode::getRouteTargets(
    bsl::ostream&  errorDescription,
    RouteTargets*  routeTargets,
    mqbi::Cluster* cluster)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(routeTargets);
    BSLS_ASSERT_SAFE(cluster);

    mqbnet::ClusterNode* node = NULL;

    int rc;

    cluster->dispatcher()->execute(
        bdlf::BindUtil::bind(&mqbi::Cluster::getPartitionPrimaryNode,
                             cluster,
                             &rc,
                             bsl::ref(errorDescription),
                             &node,
                             &routeTargets->d_self,
                             d_partitionId),
        cluster);

    cluster->dispatcher()->synchronize(cluster);

    if (node) {
        // Put node into vector to be acceptable for "routeCommand"
        routeTargets->d_nodes.push_back(node);
    }

    return rc;  // RETURN
}

CommandRouter::ClusterWideRoutingMode::ClusterWideRoutingMode()
{
}

int CommandRouter::ClusterWideRoutingMode::getRouteTargets(
    BSLA_MAYBE_UNUSED bsl::ostream& errorDescription,
    RouteTargets*                   routeTargets,
    mqbi::Cluster*                  cluster)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(routeTargets);
    BSLS_ASSERT_SAFE(cluster);

    typedef mqbnet::Cluster::NodesList NodesList;

    // Collect all nodes in cluster
    const NodesList& allNodes = cluster->netCluster().nodes();

    for (NodesList::const_iterator nit = allNodes.begin();
         nit != allNodes.end();
         nit++) {
        if (cluster->netCluster().selfNode() != *nit) {
            routeTargets->d_nodes.push_back(*nit);
        }
    }

    // Cluster routing always requires original node to execute.
    routeTargets->d_self = true;

    return 0;  // RETURN
}

int CommandRouter::route(bsl::ostream&  errorDescription,
                         bool*          selfShouldExecute,
                         mqbi::Cluster* relevantCluster)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_routingModeMp);
    BSLS_ASSERT_SAFE(selfShouldExecute);
    BSLS_ASSERT_SAFE(relevantCluster);

    typedef mqbnet::MultiRequestManager<bmqp_ctrlmsg::ControlMessage,
                                        bmqp_ctrlmsg::ControlMessage,
                                        mqbnet::ClusterNode*>::RequestContextSp
        RequestContextSp;

    enum RcEnum {
        rc_SUCCESS = 0,
        rc_ERROR   = -1,
    };

    RouteTargets routeTargets;

    const int rc = d_routingModeMp->getRouteTargets(errorDescription,
                                                    &routeTargets,
                                                    relevantCluster);

    if (0 != rc) {
        // Release latch now to not block command execution.
        releaseLatch();
        *selfShouldExecute = false;  // Never execute when there was an error.
        return rc_ERROR;             // RETURN
    }

    // If not routing to any external nodes, then we can exit early.
    if (routeTargets.d_nodes.size() == 0) {
        releaseLatch();
        *selfShouldExecute = routeTargets.d_self;
        return rc_SUCCESS;  // RETURN
    }

    RequestContextSp contextSp =
        relevantCluster->multiRequestManager().createRequestContext();

    bmqp_ctrlmsg::AdminCommand& adminCommand =
        contextSp->request().choice().makeAdminCommand();

    adminCommand.command() = d_commandString;

    contextSp->setDestinationNodes(routeTargets.d_nodes);

    bmqu::MemOutStream os;
    os << "Routing command to the following nodes [";
    for (NodesVector::const_iterator nit = routeTargets.d_nodes.begin();
         nit != routeTargets.d_nodes.end();
         nit++) {
        os << (*nit)->hostName();
        if (nit + 1 != routeTargets.d_nodes.end()) {
            os << ", ";
        }
    }
    os << "]";
    BALL_LOG_INFO << os.str();

    contextSp->setResponseCb(
        bdlf::BindUtil::bind(&CommandRouter::onRouteCommandResponse,
                             this,
                             bdlf::PlaceHolders::_1));

    const mqbcfg::AppConfig& config  = mqbcfg::BrokerConfig::get();
    double                   timeout = config.routeCommandTimeoutMs() / 1000.0;
    relevantCluster->multiRequestManager().sendRequest(
        contextSp,
        bsls::TimeInterval(timeout));

    *selfShouldExecute = routeTargets.d_self;

    return rc_SUCCESS;  // RETURN
}

void CommandRouter::onRouteCommandResponse(
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
        routeResponse.sourceNodeDescription() = pair.first->hostName();
        if (message.choice().isAdminCommandResponseValue()) {
            const bsl::string& output =
                message.choice().adminCommandResponse().text();
            routeResponse.response() = output;
        }
        else {
            // Something went wrong, possibly timed out
            bsl::string errorMessage =
                "Error occurred routing command to this node.";
            // if we are using JSON encoding
            if (d_command.encoding() == mqbcmd::EncodingFormat::JSON_COMPACT ||
                d_command.encoding() == mqbcmd::EncodingFormat::JSON_PRETTY) {
                mqbcmd::Result result;
                result.makeError().message() = errorMessage;
                // encode result
                bmqu::MemOutStream os;
                bool               pretty = d_command.encoding() ==
                                      mqbcmd::EncodingFormat::JSON_PRETTY
                                                ? true
                                                : false;
                mqbcmd::JsonPrinter::print(os, result, pretty);
                routeResponse.response() = os.str();
            }
            else {  // otherwise human print it
                routeResponse.response() = errorMessage;
            }
        }
        d_responses.responses().push_back(routeResponse);
    }

    releaseLatch();
}

void CommandRouter::setCommandRoutingMode()
{
    bslma::Allocator* allocator = bslma::Default::allocator();

    d_routingModeMp = NULL;

    const mqbcmd::CommandChoice& commandChoice = d_command.choice();

    if (commandChoice.isDomainsValue()) {
        const mqbcmd::DomainsCommand& domains = commandChoice.domains();
        if (domains.isDomainValue()) {
            const mqbcmd::DomainCommand& domain = domains.domain().command();
            if (domain.isPurgeValue()) {
                d_routingModeMp.load(new (*allocator)
                                         AllPartitionPrimariesRoutingMode());
                // DOMAINS DOMAIN <name> PURGE
            }
            else if (domain.isQueueValue()) {
                if (domain.queue().command().isPurgeAppIdValue()) {
                    d_routingModeMp.load(
                        new (*allocator) AllPartitionPrimariesRoutingMode());
                    // DOMAINS DOMAIN <name> QUEUE <name> PURGE
                }
            }
        }
        else if (domains.isReconfigureValue()) {
            d_routingModeMp.load(new (*allocator) ClusterWideRoutingMode());
            // DOMAINS RECONFIGURE <domain>
        }
    }
    else if (commandChoice.isClustersValue()) {
        const mqbcmd::ClustersCommand& clusters = commandChoice.clusters();
        if (clusters.isClusterValue()) {
            const mqbcmd::ClusterCommand& cluster =
                clusters.cluster().command();
            if (cluster.isForceGcQueuesValue()) {
                d_routingModeMp.load(new (*allocator)
                                         AllPartitionPrimariesRoutingMode());
                // CLUSTERS CLUSTER <name> FORCE_GC_QUEUES
            }
            else if (cluster.isStorageValue()) {
                const mqbcmd::StorageCommand& storage = cluster.storage();
                if (storage.isPartitionValue()) {
                    if (storage.partition().command().isEnableValue() ||
                        storage.partition().command().isDisableValue()) {
                        int partitionId = storage.partition().partitionId();
                        d_routingModeMp.load(
                            new (*allocator) SinglePartitionPrimaryRoutingMode(
                                partitionId));
                        // CLUSTERS CLUSTER <name> STORAGE PARTITION
                        // <partitionId> [ENABLE|DISABLE]
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
                            d_routingModeMp.load(new (*allocator)
                                                     ClusterWideRoutingMode());
                            // CLUSTERS CLUSTER <name> STORAGE REPLICATION
                            // SET_ALL
                        }
                    }
                    else if (replication.isGetTunableValue()) {
                        const mqbcmd::GetTunable& tunable =
                            replication.getTunable();
                        if (tunable.choice().isAllValue()) {
                            d_routingModeMp.load(new (*allocator)
                                                     ClusterWideRoutingMode());
                            // CLUSTERS CLUSTER <name> STORAGE REPLICATION
                            // GET_ALL
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
                            d_routingModeMp.load(new (*allocator)
                                                     ClusterWideRoutingMode());
                            // CLUSTERS CLUSTER <name> STATE ELECTOR SET_ALL
                        }
                    }
                    else if (elector.isGetTunableValue()) {
                        const mqbcmd::GetTunable& tunable =
                            elector.getTunable();
                        if (tunable.choice().isAllValue()) {
                            d_routingModeMp.load(new (*allocator)
                                                     ClusterWideRoutingMode());
                            // CLUSTERS CLUSTER <name> STATE ELECTOR GET_ALL
                        }
                    }
                }
            }
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
