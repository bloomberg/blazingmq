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

// mqbnet_clusteractivenodemanager.cpp                                -*-C++-*-
#include <mqbnet_clusteractivenodemanager.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_messages.h>
#include <mqbnet_session.h>

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_cstdlib.h>
#include <bsl_list.h>
#include <bsl_vector.h>
#include <bslmt_lockguard.h>
#include <bsls_annotation.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbnet {

// ------------------------------
// class ClusterActiveNodeManager
// ------------------------------

int ClusterActiveNodeManager::onNodeUp(
    ClusterNode*                        node,
    const bmqp_ctrlmsg::ClientIdentity& identity)
{
    NodesMap::iterator nodeIt = d_nodes.find(node);

    BSLS_ASSERT_SAFE(nodeIt != d_nodes.end());

    NodeContext&                    context   = nodeIt->second;
    bmqp_ctrlmsg::NodeStatus::Value oldStatus = context.d_status;

    if (bmqp::ProtocolUtil::hasFeature(
            bmqp::HighAvailabilityFeatures::k_FIELD_NAME,
            bmqp::HighAvailabilityFeatures::k_BROADCAST_TO_PROXIES,
            identity.features())) {
        context.d_status = bmqp_ctrlmsg::NodeStatus::E_UNKNOWN;
    }
    else {
        // The broker node does not support broadcasting advisories to proxies,
        // so transition to E_AVAILABLE.
        // TODO: remove this check once all versions support the feature.

        context.d_status = bmqp_ctrlmsg::NodeStatus::E_AVAILABLE;
    }

    context.d_sessionDescription = node->nodeDescription();

    return processNodeStatus(node, context, oldStatus);
}

int ClusterActiveNodeManager::onNodeStatusChange(
    ClusterNode*                    node,
    bmqp_ctrlmsg::NodeStatus::Value status)
{
    NodesMap::iterator nodeIt = d_nodes.find(node);

    BSLS_ASSERT_SAFE(nodeIt != d_nodes.end());

    NodeContext&                    context   = nodeIt->second;
    bmqp_ctrlmsg::NodeStatus::Value oldStatus = context.d_status;

    context.d_status = status;

    return processNodeStatus(node, context, oldStatus);
}

int ClusterActiveNodeManager::onNodeDown(ClusterNode* node)
{
    NodesMap::iterator nodeIt = d_nodes.find(node);

    BSLS_ASSERT_SAFE(nodeIt != d_nodes.end());

    NodeContext& context = nodeIt->second;

    bmqp_ctrlmsg::NodeStatus::Value oldStatus = context.d_status;
    context.d_status = bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE;
    context.d_sessionDescription.clear();

    return processNodeStatus(node, context, oldStatus);
}

int ClusterActiveNodeManager::processNodeStatus(
    const ClusterNode*              node,
    const NodeContext&              context,
    bmqp_ctrlmsg::NodeStatus::Value oldStatus)
{
    BALL_LOG_INFO << d_description << ": change in connection state with "
                  << "node '" << node->nodeDescription() << "', session '"
                  << context.d_sessionDescription << "', status: " << oldStatus
                  << " -> " << context.d_status << ". Current active node: ["
                  << (d_activeNodeIt != d_nodes.end()
                          ? d_activeNodeIt->first->nodeDescription()
                          : "* none *")
                  << "].";

    int result = e_NO_CHANGE;

    if (context.d_status == bmqp_ctrlmsg::NodeStatus::E_AVAILABLE) {
        if (d_activeNodeIt == d_nodes.end()) {
            // A new node is up, and we have no currently active connection,
            // check if we can use it.
            if (findNewActiveNode()) {
                result = e_NEW_ACTIVE;
            }
        }  // else we already have an active node: nothing to do.
    }
    else {
        if (node == activeNode()) {
            // We lost our active connection !! Try to find a new one.
            result         = e_LOST_ACTIVE;
            d_activeNodeIt = d_nodes.end();

            if (findNewActiveNode()) {
                result |= e_NEW_ACTIVE;
            }
        }  // else this is not our currently active node: we don't care much.
    }

    return result;
}

int ClusterActiveNodeManager::refresh()
{
    return d_activeNodeIt == d_nodes.end()
               ? (findNewActiveNode() ? e_NEW_ACTIVE : e_NO_CHANGE)
               : e_NO_CHANGE;
}

bool ClusterActiveNodeManager::findNewActiveNode()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_activeNodeIt == d_nodes.end());

    bsl::vector<mqbnet::ClusterNode*> candidates;

    // First look for a potential candidate, i.e., an available node within the
    // same DataCenter.
    candidates.reserve(d_nodes.size());

    for (NodesMap::const_iterator it = d_nodes.begin(); it != d_nodes.end();
         ++it) {
        mqbnet::ClusterNode* node = it->first;
        if (it->second.d_status == bmqp_ctrlmsg::NodeStatus::E_AVAILABLE &&
            ((node->dataCenter() == d_dataCenter) ||
             d_dataCenter == "UNSPECIFIED")) {
            candidates.push_back(node);
        }
    }
    // We found at least one 'perfect' candidate, use it.
    if (!candidates.empty()) {
        onNewActiveNode(candidates[bsl::rand() % candidates.size()]);
        return true;  // RETURN
    }

    // We didn't find a candidate...  if there is a scheduled event, do
    // nothing, we'll retry once the event expires (or if another node comes up
    // before).
    if (!d_useExtendedSelection) {
        return false;  // RETURN
    }

    // Nothing scheduled, we must find a node now, drop the 'same DataCenter'
    // requirement.
    candidates.clear();

    for (NodesMap::const_iterator it = d_nodes.begin(); it != d_nodes.end();
         ++it) {
        mqbnet::ClusterNode* node = it->first;
        if (it->second.d_status == bmqp_ctrlmsg::NodeStatus::E_AVAILABLE) {
            candidates.push_back(node);
        }
    }

    if (candidates.empty()) {
        onNewActiveNode(0);
        return false;  // RETURN
    }
    else {
        onNewActiveNode(candidates[bsl::rand() % candidates.size()]);
        return true;  // RETURN
    }
}

void ClusterActiveNodeManager::onNewActiveNode(ClusterNode* node)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_activeNodeIt == d_nodes.end());
    // Should only be called if there is no currently active node

    if (!node) {
        // No nodes are available, that is a major issue!  However, no need to
        // alarm in the case of single-node cluster, as this is bound to happen
        // when that node bounces.
        if (d_nodes.size() > 1) {
            BMQTSK_ALARMLOG_PANIC("CLUSTER_ACTIVE_NODE")
                << d_description << ": no node available !!!"
                << BMQTSK_ALARMLOG_END;
        }
        else {
            BALL_LOG_INFO << "No node available for '" << d_description << "'";
        }

        // NOTE: No need to notify the observer, we already did it in
        //       'onNodeStateChange()'.
        return;  // RETURN
    }

    BALL_LOG_INFO << d_description << ": is now using node '"
                  << node->nodeDescription() << "' as active.";

    d_activeNodeIt = d_nodes.find(node);

    BSLS_ASSERT_SAFE(d_activeNodeIt != d_nodes.end());
}

ClusterActiveNodeManager::ClusterActiveNodeManager(
    const Cluster::NodesList& nodes,
    const bslstl::StringRef&  description,
    const bsl::string&        dataCenter)
: d_description(description)
, d_dataCenter(dataCenter)
, d_activeNodeIt(d_nodes.end())
, d_useExtendedSelection(false)
{
    for (mqbnet::Cluster::NodesList::const_iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        mqbnet::ClusterNode* node = *it;
        d_nodes[node].d_status    = bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE;
    }
}

ClusterActiveNodeManager::~ClusterActiveNodeManager()
{
    // NOTHING
}

void ClusterActiveNodeManager::initialize(TransportManager* transportManager)
{
    // We need to detect which nodes are already connected (before we have
    // registered as an observer).
    // We could iterate 'd_nodes' and check if 'd_nodes[i]->isAvailable' but we
    // also need to access peer identity.  'ClusterNodeImp::identity()' is NOT
    // thread-safe and can change.
    // So, we detect connected nodes by iterating connected sessions because
    // 'mqbnet::Session::negotiationMessage' is read-only (safe to access).
    for (TransportManagerIterator sessIt(transportManager); sessIt; ++sessIt) {
        bsl::shared_ptr<Session> session = sessIt.session().lock();
        NodesMap::iterator       nodeIt = d_nodes.find(session->clusterNode());

        if (nodeIt == d_nodes.end()) {
            continue;  // CONTINUE
        }

        if (!nodeIt->first->isAvailable()) {
            // This implies that the 'ClusterNodeImp::setChannel' is not done
            // yet.
            // 'ClusterNode::setChannel' first makes the channel available and
            // then notifies observers one of which is ClusterProxy.  The
            // notification gets scheduled to run in this dispatcher thread.
            // This guarantees that we MUST get 'onNodeStatusChange' later.
            BSLS_ASSERT_SAFE(nodeIt->second.d_status ==
                             bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE);
            continue;  // CONTINUE
        }

        // Consider the 'nodeIt->first' to be 'e_AVAILABLE' (or 'e_UNKNOWN' if
        // we expect to receive a status advisory; the change of the status
        // will be recognized in a future 'onNodeStatusChange').
        if (!bmqp::ProtocolUtil::hasFeature(
                bmqp::HighAvailabilityFeatures::k_FIELD_NAME,
                bmqp::HighAvailabilityFeatures::k_BROADCAST_TO_PROXIES,
                session->negotiationMessage()
                    .brokerResponse()
                    .brokerIdentity()
                    .features())) {
            // The broker node does not support broadcasting advisories to
            // proxies (for example, in the case of Virtual Clusters).
            // Therefore immediately transition to 'e_AVAILABLE'.
            nodeIt->second.d_status = bmqp_ctrlmsg::NodeStatus::E_AVAILABLE;
        }
        else {
            nodeIt->second.d_status = bmqp_ctrlmsg::NodeStatus::E_UNKNOWN;
        }
    }
}

void ClusterActiveNodeManager::loadNodesInfo(mqbcmd::NodeStatuses* out) const
{
    bsl::vector<mqbcmd::ClusterNodeInfo>& nodes = out->nodes();
    nodes.reserve(d_nodes.size());
    for (NodesMap::const_iterator it = d_nodes.begin(); it != d_nodes.end();
         ++it) {
        nodes.resize(nodes.size() + 1);
        mqbcmd::ClusterNodeInfo& node = nodes.back();
        node.description()            = it->first->nodeDescription();
        node.isAvailable().makeValue(it->first->isAvailable());
        int rc = mqbcmd::NodeStatus::fromInt(&node.status(),
                                             it->second.d_status);
        BSLS_ASSERT_SAFE(!rc && "Unsupported node status");
        (void)rc;
    }
}

}  // close package namespace
}  // close enterprise namespace
