// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbc_clusterutil.cpp                                               -*-C++-*-
#include <mqbc_clusterutil.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clusternodesession.h>
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbc_electorinfo.h>
#include <mqbc_incoreclusterstateledgeriterator.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbi_domain.h>
#include <mqbi_queueengine.h>
#include <mqbi_storagemanager.h>
#include <mqbnet_cluster.h>
#include <mqbnet_elector.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_storageutil.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqt_uri.h>

// MWC
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_blobobjectproxy.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlb_print.h>
#include <bdlde_md5.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bslma_managedptr.h>

namespace BloombergLP {
namespace mqbc {

namespace {

// CONSTANTS
const char k_LOG_CATEGORY[] = "MQBC.CLUSTERUTIL";

const double k_MAX_QUEUES_HIGH_WATERMARK = 0.80;  // % of maximum
const char   k_MAXIMUM_NUMBER_OF_QUEUES_REACHED[] =
    "maximum number of queues reached";
const char k_SELF_NODE_IS_STOPPING[]   = "self node is stopping";
const char k_DOMAIN_CREATION_FAILURE[] = "failed to create domain";

// TYPES
typedef ClusterUtil::AppIdInfo       AppIdInfo;
typedef ClusterUtil::AppIdInfos      AppIdInfos;
typedef ClusterUtil::AppIdInfosCIter AppIdInfosCIter;

typedef ClusterUtil::ClusterNodeSessionMapConstIter
    ClusterNodeSessionMapConstIter;

typedef mqbc::ClusterUtil::NumNewPartitionsMap      NumNewPartitionsMap;
typedef mqbc::ClusterUtil::NumNewPartitionsMapCIter NumNewPartitionsMapCIter;

// FUNCTIONS
void applyPartitionPrimary(
    mqbc::ClusterState*                                    clusterState,
    const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitions,
    const mqbc::ClusterData&                               clusterData)
{
    for (int i = 0; i < static_cast<int>(partitions.size()); ++i) {
        const bmqp_ctrlmsg::PartitionPrimaryInfo& info = partitions[i];

        mqbnet::ClusterNode* proposedPrimaryNode =
            clusterData.membership().netCluster()->lookupNode(
                info.primaryNodeId());
        clusterState->setPartitionPrimary(info.partitionId(),
                                          info.primaryLeaseId(),
                                          proposedPrimaryNode);
    }
}

void applyQueueAssignment(mqbc::ClusterState* clusterState,
                          const bsl::vector<bmqp_ctrlmsg::QueueInfo>& queues)
{
    for (bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator it =
             queues.begin();
         it != queues.end();
         ++it) {
        const bmqp_ctrlmsg::QueueInfo& queueInfo = *it;
        const bmqt::Uri                uri(queueInfo.uri());
        const int                      partitionId(queueInfo.partitionId());
        const mqbu::StorageKey         queueKey(
            mqbu::StorageKey::BinaryRepresentation(),
            queueInfo.key().data());

        const bsl::vector<bmqp_ctrlmsg::AppIdInfo>& appIds =
            queueInfo.appIds();
        AppIdInfos addedAppIds;
        for (bsl::vector<bmqp_ctrlmsg::AppIdInfo>::const_iterator citer =
                 appIds.cbegin();
             citer != appIds.cend();
             ++citer) {
            AppIdInfo appIdInfo;
            appIdInfo.first = citer->appId();
            appIdInfo.second.fromBinary(citer->appKey().data());

            addedAppIds.insert(appIdInfo);
        }

        clusterState->assignQueue(uri, queueKey, partitionId, addedAppIds);
    }
}

void applyQueueUnassignment(mqbc::ClusterState* clusterState,
                            const bsl::vector<bmqp_ctrlmsg::QueueInfo>& queues)
{
    for (bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator it =
             queues.begin();
         it != queues.end();
         ++it) {
        clusterState->unassignQueue(it->uri());
        // NOTE: There are cases where the persistent entry may not
        //       exist when receiving this advisory, so the return code
        //       of the 'unassignQueue' operation is not checked.
    }
}

void applyQueueUpdate(mqbc::ClusterState* clusterState,
                      const bsl::vector<bmqp_ctrlmsg::QueueInfoUpdate>& queues,
                      const mqbc::ClusterData& clusterData)
{
    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    for (bsl::vector<bmqp_ctrlmsg::QueueInfoUpdate>::const_iterator it =
             queues.begin();
         it != queues.end();
         ++it) {
        const bmqp_ctrlmsg::QueueInfoUpdate& queueUpdate = *it;
        const bmqt::Uri                      uri(queueUpdate.uri());
        const mqbu::StorageKey               queueKey(
            mqbu::StorageKey::BinaryRepresentation(),
            queueUpdate.key().data());
        const int partitionId(queueUpdate.partitionId());

        // Validation
        if (uri.isValid()) {
            // TODO_CSL This assert is backward-incompatible, because older
            // versions of broker do not populate the queueUpdate.domain()
            // field.  We will add this assert back once safe.
            //
            // BSLS_ASSERT_SAFE(uri.qualifiedDomain() == queueUpdate.domain());

            ClusterState::DomainStatesCIter domCiter =
                clusterState->domainStates().find(uri.qualifiedDomain());
            if (domCiter == clusterState->domainStates().end()) {
                // First time hearing about this domain and queue - should not
                // occur for an update advisory
                MWCTSK_ALARMLOG_ALARM("CLUSTER")
                    << clusterData.identity().description()
                    << ": Received QueueUpdateAdvisory for a domain and queue "
                    << "that were not found: " << queueUpdate << "]"
                    << MWCTSK_ALARMLOG_END;
                return;  // RETURN
            }

            ClusterState::UriToQueueInfoMapCIter cit =
                domCiter->second->queuesInfo().find(uri);
            if (cit == domCiter->second->queuesInfo().end()) {
                // First time hearing about this queue - should not occur for
                // an update advisory
                MWCTSK_ALARMLOG_ALARM("CLUSTER")
                    << clusterData.identity().description()
                    << ": Received QueueUpdateAdvisory for a queue that was "
                    << "not found: " << queueUpdate << "]"
                    << MWCTSK_ALARMLOG_END;
                return;  // RETURN
            }

            if (cit->second->partitionId() != partitionId) {
                MWCTSK_ALARMLOG_ALARM("CLUSTER")
                    << clusterData.identity().description()
                    << ": Received QueueUpdateAdvisory for known queue [uri: "
                    << uri << "] with a mismatched partitionId "
                    << "[expected: " << cit->second->partitionId()
                    << ", received: " << partitionId << "]: " << queueUpdate
                    << MWCTSK_ALARMLOG_END;
                return;  // RETURN
            }

            if (cit->second->key() != queueKey) {
                MWCTSK_ALARMLOG_ALARM("CLUSTER")
                    << clusterData.identity().description()
                    << ": Received QueueUpdateAdvisory for known queue [uri: "
                    << uri << "] with a mismatched queueKey "
                    << "[expected: " << queueKey << ", received: " << queueKey
                    << "]: " << queueUpdate << MWCTSK_ALARMLOG_END;
                return;  // RETURN
            }
        }
        else {
            // This update is for an entire domain, instead of any individual
            // queue.
            BSLS_ASSERT_SAFE(queueKey == mqbu::StorageKey::k_NULL_KEY);
            BSLS_ASSERT_SAFE(partitionId ==
                             mqbs::DataStore::k_INVALID_PARTITION_ID);
        }

        AppIdInfos addedAppIds;
        for (bsl::vector<bmqp_ctrlmsg::AppIdInfo>::const_iterator citer =
                 queueUpdate.addedAppIds().cbegin();
             citer != queueUpdate.addedAppIds().cend();
             ++citer) {
            AppIdInfo appIdInfo;
            appIdInfo.first = citer->appId();
            appIdInfo.second.fromBinary(citer->appKey().data());

            addedAppIds.insert(appIdInfo);
        }

        AppIdInfos removedAppIds;
        for (bsl::vector<bmqp_ctrlmsg::AppIdInfo>::const_iterator citer =
                 queueUpdate.removedAppIds().begin();
             citer != queueUpdate.removedAppIds().end();
             ++citer) {
            AppIdInfo appIdInfo;
            appIdInfo.first = citer->appId();
            appIdInfo.second.fromBinary(citer->appKey().data());

            removedAppIds.insert(appIdInfo);
        }

        bsl::string domain = queueUpdate.domain();
        if (domain.empty()) {
            domain = uri.qualifiedDomain();
        }

        const int rc =
            clusterState->updateQueue(uri, domain, addedAppIds, removedAppIds);
        if (rc != 0) {
            BALL_LOG_ERROR << clusterData.identity().description()
                           << ": Received QueueUpdateAdvisory for uri: [ "
                           << uri << "], domain: [" << domain
                           << "], but failed to update appIds, "
                           << "rc : " << rc
                           << ". queueUpdate: " << queueUpdate;
            return;  // RETURN
        }
    }
}

void getNextPrimarys(NumNewPartitionsMap* numNewPartitions,
                     unsigned int         numPartitionsToAssign,
                     mqbcfg::MasterAssignmentAlgorithm::Value assignmentAlgo,
                     const ClusterData&                       clusterData)
// Given the specified 'numPartitionsToAssign' and 'assignmentAlgo',
// load into the specified 'numNewPartitions' the number of new
// partitions to assign to each node session, using the specified
// 'clusterData'.  The behavior is undefined unless this node is the
// leader.
//
// THREAD: This method is invoked in the associated cluster's
//         dispatcher thread.
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData.cluster()->dispatcher()->inDispatcherThread(
        clusterData.cluster()));
    BSLS_ASSERT_SAFE(mqbnet::ElectorState::e_LEADER ==
                     clusterData.electorInfo().electorState());
    BSLS_ASSERT_SAFE(numNewPartitions && numNewPartitions->empty());

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    switch (assignmentAlgo) {
    case mqbcfg::MasterAssignmentAlgorithm::E_LEADER_IS_MASTER_ALL: {
        mqbnet::ClusterNode* selfNode = clusterData.membership().selfNode();
        numNewPartitions->insert(bsl::make_pair(
            clusterData.membership().getClusterNodeSession(selfNode),
            numPartitionsToAssign));

        break;  // BREAK
    }
    case mqbcfg::MasterAssignmentAlgorithm::E_LEAST_ASSIGNED:
        // Whichever AVAILABLE node has least number of partitions assigned to
        // it is chosen as the new primary.  There is no "preferred list" of
        // primary nodes for a given partition.  However, this mode is not
        // supported right now.
    default: {
        MWCTSK_ALARMLOG_ALARM("CLUSTER")
            << clusterData.identity().description()
            << ": Unknown cluster masterAssignmentAlgorithm '"
            << assignmentAlgo
            << "', defaulting to 'leaderIsMasterAll' algorithm."
            << MWCTSK_ALARMLOG_END;

        mqbnet::ClusterNode* selfNode = clusterData.membership().selfNode();
        numNewPartitions->insert(bsl::make_pair(
            clusterData.membership().getClusterNodeSession(selfNode),
            numPartitionsToAssign));

        break;  // BREAK
    }
    }
}

void printQueues(bsl::ostream& out, const ClusterState& state)
{
    out << '\n'
        << "-------------------------" << '\n'
        << "QUEUES IN CLUSTER STATE :" << '\n'
        << "-------------------------";
    for (ClusterState::DomainStatesCIter domCit =
             state.domainStates().cbegin();
         domCit != state.domainStates().cend();
         ++domCit) {
        for (ClusterState::UriToQueueInfoMapCIter citer =
                 domCit->second->queuesInfo().cbegin();
             citer != domCit->second->queuesInfo().cend();
             ++citer) {
            const bsl::shared_ptr<ClusterStateQueueInfo>& info = citer->second;
            bdlb::Print::newlineAndIndent(out, 1);
            out << "[key: " << info->key() << ", uri: " << info->uri()
                << ", partitionId: " << info->partitionId() << "]";
        }
    }
}

/// If the specified `status` is SUCCESS, load the specified `domain` into
/// the specified `domainState`.
void createDomainCb(const bmqp_ctrlmsg::Status& status,
                    mqbi::Domain*               domain,
                    ClusterState::DomainStateSp domainState)
{
    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        return;  // RETURN
    }

    domainState->setDomain(domain);
}

}  // close anonymous namespace

// ------------------
// struct ClusterUtil
// ------------------

void ClusterUtil::setPendingUnassignment(ClusterState*    clusterState,
                                         const bmqt::Uri& uri,
                                         bool             pendingUnassignment)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(uri.isCanonical());

    const DomainStatesIter iter = clusterState->domainStates().find(
        uri.qualifiedDomain());
    if (iter != clusterState->domainStates().cend()) {
        UriToQueueInfoMapIter qiter = iter->second->queuesInfo().find(uri);
        if (qiter != iter->second->queuesInfo().cend()) {
            qiter->second->setPendingUnassignment(pendingUnassignment);
        }
    }
}

void ClusterUtil::extractMessage(bmqp_ctrlmsg::ControlMessage* message,
                                 const bdlbb::Blob&            eventBlob,
                                 bslma::Allocator*             allocator)
{
    // Extract event header
    mwcu::BlobObjectProxy<bmqp::EventHeader> eventHeader(
        &eventBlob,
        -bmqp::EventHeader::k_MIN_HEADER_SIZE,
        true,    // read
        false);  // write
    BSLS_ASSERT_OPT(eventHeader.isSet());

    const int eventHeaderSize = eventHeader->headerWords() *
                                bmqp::Protocol::k_WORD_SIZE;
    BSLS_ASSERT_OPT(eventHeaderSize >= bmqp::EventHeader::k_MIN_HEADER_SIZE);
    bool isValid = eventHeader.resize(eventHeaderSize);
    BSLS_ASSERT_OPT(isValid);
    BSLS_ASSERT_OPT(eventHeader->length() == eventBlob.length());
    BSLS_ASSERT_OPT(eventHeader->type() == bmqp::EventType::e_CONTROL);

    // Decode message
    mwcu::MemOutStream errorDescription;
    int                rc = bmqp::ProtocolUtil::decodeMessage(errorDescription,
                                               message,
                                               eventBlob,
                                               eventHeaderSize,
                                               bmqp::EncodingType::e_BER,
                                               allocator);
    BSLS_ASSERT_OPT(rc == 0);
}

void ClusterUtil::assignPartitions(
    bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>* partitions,
    ClusterState*                                    clusterState,
    mqbcfg::MasterAssignmentAlgorithm::Value         assignmentAlgo,
    const ClusterData&                               clusterData,
    bool                                             isCSLMode)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData.cluster()->dispatcher()->inDispatcherThread(
        clusterData.cluster()));
    BSLS_ASSERT_SAFE(partitions && partitions->empty());
    BSLS_ASSERT_SAFE(mqbnet::ElectorState::e_LEADER ==
                     clusterData.electorInfo().electorState());
    if (!isCSLMode) {
        BSLS_ASSERT_SAFE(mqbc::ElectorInfoLeaderStatus::e_ACTIVE ==
                         clusterData.electorInfo().leaderStatus());
        BSLS_ASSERT_SAFE(bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
                         clusterData.membership().selfNodeStatus());
    }

    // Find out the partitions that requires a new primary
    bsl::vector<mqbc::ClusterStatePartitionInfo> partitionsToChange;

    for (int pid = 0;
         pid < static_cast<int>(clusterState->partitions().size());
         ++pid) {
        const mqbc::ClusterStatePartitionInfo& pinfo = clusterState->partition(
            pid);
        if (pinfo.primaryNode()) {
            BSLS_ASSERT_SAFE(0 < pinfo.primaryLeaseId());
            mqbc::ClusterNodeSession* ns =
                clusterData.membership().getClusterNodeSession(
                    pinfo.primaryNode());
            BSLS_ASSERT_SAFE(ns);

            if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE == ns->nodeStatus()) {
                // Currently assigned primary is AVAILABLE.  No need to change
                // this mapping.

                bmqp_ctrlmsg::PartitionPrimaryInfo info;
                info.partitionId()    = pid;
                info.primaryNodeId()  = pinfo.primaryNodeId();
                info.primaryLeaseId() = pinfo.primaryLeaseId();

                partitions->push_back(info);

                continue;  // CONTINUE
            }

            partitionsToChange.push_back(pinfo);
        }
        else {
            partitionsToChange.push_back(pinfo);
        }
    }

    NumNewPartitionsMap numNewPartitions;
    getNextPrimarys(&numNewPartitions,
                    partitionsToChange.size(),
                    assignmentAlgo,
                    clusterData);

    bsl::vector<mqbc::ClusterStatePartitionInfo>::const_iterator cit2 =
        partitionsToChange.cbegin();
    for (NumNewPartitionsMapCIter cit = numNewPartitions.cbegin();
         cit != numNewPartitions.cend();
         ++cit) {
        mqbc::ClusterNodeSession* primaryNs = cit->first;
        mqbnet::ClusterNode*      primary   = primaryNs->clusterNode();
        BSLS_ASSERT_SAFE(primary);

        // Assign 'cit->second' partitions to 'primary'.
        for (unsigned int i = 0; i < cit->second; ++i) {
            const mqbc::ClusterStatePartitionInfo& pinfo = *cit2;

            // In CSL mode, we apply the new partition assignments at the
            // commit callback of 'PartitionPrimaryAdvisory' or
            // 'LeaderAdvisory' instead.
            if (!isCSLMode) {
                if (pinfo.primaryNode()) {
                    mqbc::ClusterNodeSession* ns =
                        clusterData.membership().getClusterNodeSession(
                            pinfo.primaryNode());
                    BSLS_ASSERT_SAFE(ns);

                    // Currently assigned primary node is not AVAILABLE.
                    // Remove this partition from this node.
                    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::NodeStatus::E_AVAILABLE !=
                                     ns->nodeStatus());
                    ns->removePartitionRaw(pinfo.partitionId());
                }

                primaryNs->addPartitionRaw(pinfo.partitionId());

                clusterState->setPartitionPrimary(pinfo.partitionId(),
                                                  pinfo.primaryLeaseId() + 1,
                                                  primary);
            }

            BALL_LOG_INFO << clusterData.identity().description()
                          << ": PartitionId [" << pinfo.partitionId()
                          << "]: Leader (self) has assigned "
                          << primary->nodeDescription() << " as primary.";

            bmqp_ctrlmsg::PartitionPrimaryInfo info;
            info.partitionId()    = pinfo.partitionId();
            info.primaryNodeId()  = primary->nodeId();
            info.primaryLeaseId() = pinfo.primaryLeaseId() + 1;

            partitions->push_back(info);

            ++cit2;
        }
    }
    BSLS_ASSERT_SAFE(cit2 == partitionsToChange.cend());

    BSLS_ASSERT_SAFE(partitions->size() == clusterState->partitions().size());
}

int ClusterUtil::getNextPartitionId(const ClusterState& clusterState,
                                    const bmqt::Uri&    uri)
{
    // Try to assign to the partition which has a primary and the least number
    // of queues assigned.  If no partitions have a primary, then assign to the
    // partition with the least number of queues.  It's ok to choose a  primary
    // which is not active at the moment.  In case of a latemon domain try to
    // take the partition id from the queue name.

    const bslstl::StringRef& domainName = uri.domain();
    const bslstl::StringRef& queueName  = uri.path();
    const bsl::string&       latencyMonitorDomain =
        mqbcfg::BrokerConfig::get().latencyMonitorDomain();

    if (domainName.find(latencyMonitorDomain) != bsl::string::npos) {
        // latemon domain
        const int partitionId = clusterState.extractPartitionId(queueName);
        if (partitionId < 0) {
            BALL_LOG_WARN << "Failed to get partition Id from string: "
                          << queueName;
        }
        else if (partitionId >=
                 static_cast<int>(clusterState.partitions().size())) {
            BALL_LOG_WARN << "Partition Id gotten from queue name is invalid: "
                          << partitionId;
        }
        else {
            return partitionId;
        }
    }

    int minQueuesMapped = bsl::numeric_limits<int>::max();
    int res             = -1;

    for (size_t i = 0; i < clusterState.partitions().size(); ++i) {
        const mqbc::ClusterStatePartitionInfo& partitionInfo =
            clusterState.partitions()[i];
        if (partitionInfo.primaryNode() &&
            partitionInfo.numQueuesMapped() < minQueuesMapped) {
            minQueuesMapped = partitionInfo.numQueuesMapped();
            res             = i;
        }
    }

    if (res == -1) {
        // Didn't find a partition with a primary; drop this requirement and
        // simply look for the least used partition.
        res = 0;
        for (size_t i = 1; i < clusterState.partitions().size(); ++i) {
            if (clusterState.partitions()[i].numQueuesMapped() <
                clusterState.partitions()[res].numQueuesMapped()) {
                res = i;
            }
        }
    }

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(res >= 0 &&
                     res < static_cast<int>(clusterState.partitions().size()));

    return res;
}

void ClusterUtil::onPartitionPrimaryAssignment(
    ClusterData*                       clusterData,
    mqbi::StorageManager*              storageManager,
    int                                partitionId,
    mqbnet::ClusterNode*               primary,
    unsigned int                       leaseId,
    bmqp_ctrlmsg::PrimaryStatus::Value status,
    mqbnet::ClusterNode*               oldPrimary,
    unsigned int                       oldLeaseId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(clusterData->cluster());
    BSLS_ASSERT_SAFE(clusterData->cluster()->dispatcher()->inDispatcherThread(
        clusterData->cluster()));
    BSLS_ASSERT_SAFE(storageManager);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    if (primary) {
        BSLS_ASSERT_SAFE(status != bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED);
    }
    else {
        BSLS_ASSERT_SAFE(status == bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED);
    }

    // Validation
    if (primary == oldPrimary) {
        if (leaseId == oldLeaseId) {
            // Leader has re-sent the primary info for this partition.
            BSLA_MAYBE_UNUSED const mqbc::ClusterNodeSession* ns =
                clusterData->membership().getClusterNodeSession(primary);
            BSLS_ASSERT_SAFE(ns && ns->isPrimaryForPartition(partitionId));

            if (clusterData->clusterConfig()
                    .clusterAttributes()
                    .isFSMWorkflow()) {
                storageManager->setPrimaryStatusForPartition(partitionId,
                                                             status);
            }

            return;  // RETURN
        }
        else {
            // We do support the scenario where proposed primary node is same
            // as the current one, but only leaseId has been bumped up.
            BSLS_ASSERT_SAFE(leaseId > oldLeaseId);
        }
    }

    // Remove the old partition<->primary mapping (below logic works even if
    // proposed primary node is same as existing one).
    if (oldPrimary != 0) {
        mqbc::ClusterNodeSession* ns =
            clusterData->membership().getClusterNodeSession(oldPrimary);
        BSLS_ASSERT_SAFE(ns);

        ns->removePartitionSafe(partitionId);
    }

    if (primary) {
        mqbc::ClusterNodeSession* ns =
            clusterData->membership().getClusterNodeSession(primary);
        BSLS_ASSERT_SAFE(ns);

        ns->addPartitionRaw(partitionId);

        // Notify the storage about (potentially same) mapping.  This must be
        // done before calling
        // 'ClusterQueueHelper::afterPartitionPrimaryAssignment' (via
        // d_afterPartitionPrimaryAssignmentCb), because ClusterQueueHelper
        // assumes that storage is aware of the mapping.
        storageManager->setPrimaryForPartition(partitionId, primary, leaseId);
    }
    else {
        storageManager->clearPrimaryForPartition(partitionId, oldPrimary);
    }
}

void ClusterUtil::processQueueAssignmentRequest(
    ClusterState*                       clusterState,
    ClusterData*                        clusterData,
    ClusterStateLedger*                 ledger,
    const mqbi::Cluster*                cluster,
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbnet::ClusterNode*                requester,
    const QueueAssigningCb&             queueAssigningCb,
    bslma::Allocator*                   allocator)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster->dispatcher()->inDispatcherThread(cluster));
    BSLS_ASSERT_SAFE(!cluster->isRemote());
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(ledger && ledger->isOpen());
    BSLS_ASSERT_SAFE(request.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(request.choice()
                         .clusterMessage()
                         .choice()
                         .isQueueAssignmentRequestValue());
    BSLS_ASSERT_SAFE(allocator);

    BALL_LOG_INFO << cluster->description()
                  << ": Processing queueAssignment request from '"
                  << requester->nodeDescription() << "': " << request;

    bdlma::LocalSequentialAllocator<1024> localAllocator(allocator);
    bmqp_ctrlmsg::ControlMessage          response(&localAllocator);
    response.rId() = request.rId();

    if (!clusterData->electorInfo().isSelfLeader()) {
        // We are no longer the leader, reply with a clear failure so that the
        // sender will know to retry/wait for a new leader.

        bmqp_ctrlmsg::Status& failure = response.choice().makeStatus();
        failure.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        failure.code()     = mqbi::ClusterErrorCode::e_NOT_LEADER;
        failure.message()  = "No longer leader";

        clusterData->messageTransmitter().sendMessage(response, requester);
        return;  // RETURN
    }

    if (!clusterData->electorInfo().isSelfActiveLeader()) {
        // We are not ACTIVE leader, reply with a clear failure so that the
        // sender will know to retry/wait for a new leader.

        bmqp_ctrlmsg::Status& failure = response.choice().makeStatus();
        failure.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        failure.code()     = mqbi::ClusterErrorCode::e_NOT_LEADER;
        failure.message()  = "Not an active leader";

        clusterData->messageTransmitter().sendMessage(response, requester);
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
        clusterData->membership().selfNodeStatus()) {
        // We are the ACTIVE leader, but we are stopping. Reply with a clear
        // failure so that the sender will know to retry/wait for a new leader.

        bmqp_ctrlmsg::Status& failure = response.choice().makeStatus();
        failure.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        failure.code()     = mqbi::ClusterErrorCode::e_STOPPING;
        failure.message()  = "Leader is stopping";

        clusterData->messageTransmitter().sendMessage(response, requester);
        return;  // RETURN
    }

    const bmqp_ctrlmsg::QueueAssignmentRequest& assignment =
        request.choice().clusterMessage().choice().queueAssignmentRequest();
    bmqt::Uri uri(assignment.queueUri(), &localAllocator);

    bmqp_ctrlmsg::Status& status = response.choice().makeStatus();
    status.category()            = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    status.code()                = 0;
    status.message()             = "";

    const DomainStatesCIter cit = clusterState->domainStates().find(
        uri.qualifiedDomain());
    if (cit != clusterState->domainStates().cend()) {
        UriToQueueInfoMapCIter qcit = cit->second->queuesInfo().find(uri);
        if (qcit != cit->second->queuesInfo().cend() &&
            !(cluster->isCSLModeEnabled() &&
              qcit->second->pendingUnassignment())) {
            // Queue is already assigned
            clusterData->messageTransmitter().sendMessage(response, requester);
            return;  // RETURN
        }
    }

    assignQueue(clusterState,
                clusterData,
                ledger,
                cluster,
                uri,
                queueAssigningCb,
                allocator,
                &status);

    clusterData->messageTransmitter().sendMessage(response, requester);
}

void ClusterUtil::populateQueueAssignmentAdvisory(
    bmqp_ctrlmsg::QueueAssignmentAdvisory* advisory,
    mqbu::StorageKey*                      key,
    ClusterState*                          clusterState,
    ClusterData*                           clusterData,
    const bmqt::Uri&                       uri,
    const mqbi::Domain*                    domain,
    bool                                   isCSLMode)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(advisory);
    BSLS_ASSERT_SAFE(key);
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(uri.isCanonical());
    BSLS_ASSERT_SAFE(domain);
    BSLS_ASSERT_SAFE(clusterData->electorInfo().isSelfActiveLeader());

    clusterData->electorInfo().nextLeaderMessageSequence(
        &advisory->sequenceNumber());
    advisory->queues().resize(1);

    bmqp_ctrlmsg::QueueInfo& queueInfo = advisory->queues().back();
    queueInfo.uri()                    = uri.asString();
    queueInfo.partitionId() = getNextPartitionId(*clusterState, uri);
    mqbs::StorageUtil::generateStorageKey(key,
                                          &clusterState->queueKeys(),
                                          uri.asString());
    key->loadBinary(&queueInfo.key());

    if (isCSLMode) {
        // Generate appIds and appKeys
        populateAppIdInfos(&queueInfo.appIds(), domain->config().mode());
    }

    BALL_LOG_INFO << clusterData->identity().description()
                  << ": Populated QueueAssignmentAdvisory: " << *advisory;
}

void ClusterUtil::populateQueueUnassignedAdvisory(
    bmqp_ctrlmsg::QueueUnassignedAdvisory* advisory,
    ClusterData*                           clusterData,
    const bmqt::Uri&                       uri,
    const mqbu::StorageKey&                key,
    int                                    partitionId,
    const ClusterState&                    clusterState)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(advisory);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(uri.isCanonical());
    BSLS_ASSERT_SAFE((0 <= partitionId) && (static_cast<size_t>(partitionId) <
                                            clusterState.partitions().size()));
    BSLS_ASSERT_SAFE(clusterData->electorInfo().isSelfActiveLeader());

    clusterData->electorInfo().nextLeaderMessageSequence(
        &advisory->sequenceNumber());

    advisory->primaryNodeId() =
        clusterState.partition(partitionId).primaryNodeId();
    advisory->primaryLeaseId() =
        clusterState.partition(partitionId).primaryLeaseId();
    advisory->partitionId() = partitionId;

    advisory->queues().resize(1);
    bmqp_ctrlmsg::QueueInfo& queueInfo = advisory->queues().back();

    queueInfo.uri()         = uri.asString();
    queueInfo.partitionId() = partitionId;  // unused, we use the
                                            // advisory.partitionId instead but
                                            // still populated.
    key.loadBinary(&queueInfo.key());

    BALL_LOG_INFO << clusterData->identity().description()
                  << ": Populated QueueUnassignedAdvisory: " << *advisory;
}

ClusterUtil::QueueAssignmentResult::Enum
ClusterUtil::assignQueue(ClusterState*           clusterState,
                         ClusterData*            clusterData,
                         ClusterStateLedger*     ledger,
                         const mqbi::Cluster*    cluster,
                         const bmqt::Uri&        uri,
                         const QueueAssigningCb& queueAssigningCb,
                         bslma::Allocator*       allocator,
                         bmqp_ctrlmsg::Status*   status)
{
    // executed by the cluster *DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster->dispatcher()->inDispatcherThread(cluster));
    BSLS_ASSERT_SAFE(!cluster->isRemote());
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(clusterData->electorInfo().isSelfActiveLeader());
    BSLS_ASSERT_SAFE(ledger && ledger->isOpen());
    BSLS_ASSERT_SAFE(uri.isCanonical());
    BSLS_ASSERT_SAFE(allocator);

    // We are the leader and received a request to assign a queue URI with a
    // partitionId and queueKey.  Note that we don't check the status of a
    // partition's primary (active vs passive) while assigning a queue to it.
    // If primary is passive, things will still work as expected.

    const bmqp_ctrlmsg::NodeStatus::Value nodeStatus =
        clusterData->membership().selfNodeStatus();
    if (!cluster->isFSMWorkflow() &&
        bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != nodeStatus) {
        BALL_LOG_WARN << cluster->description()
                      << " Cannot proceed with queueAssignment of '" << uri
                      << "' because self is " << nodeStatus;

        if (status) {
            status->category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
            status->code()     = mqbi::ClusterErrorCode::e_STOPPING;
            status->message()  = k_SELF_NODE_IS_STOPPING;
        }

        return QueueAssignmentResult::
            k_ASSIGNMENT_WHILE_UNAVAILABLE;  // RETURN
    }

    DomainStatesIter domIt = clusterState->domainStates().find(
        uri.qualifiedDomain());
    if (domIt == clusterState->domainStates().end()) {
        clusterState->domainStates()[uri.qualifiedDomain()].createInplace(
            allocator,
            allocator);
        domIt = clusterState->domainStates().find(uri.qualifiedDomain());
    }

    if (domIt->second->domain() == 0) {
        clusterData->domainFactory()->createDomain(
            uri.qualifiedDomain(),
            bdlf::BindUtil::bind(&createDomainCb,
                                 bdlf::PlaceHolders::_1,  // status
                                 bdlf::PlaceHolders::_2,  // domain
                                 domIt->second));

        if (domIt->second->domain() == 0) {
            BALL_LOG_ERROR << cluster->description()
                           << ": Unable to create domain '"
                           << uri.qualifiedDomain() << "'";

            if (status) {
                status->category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
                status->code()     = mqbi::ClusterErrorCode::e_UNKNOWN;
                status->message()  = k_DOMAIN_CREATION_FAILURE;
            }

            return QueueAssignmentResult::k_ASSIGNMENT_REJECTED;  // RETURN
        }
    }

    struct local {
        static void panic(mqbi::Domain* domain)
        {
            MWCTSK_ALARMLOG_PANIC("DOMAIN_QUEUE_LIMIT_FULL")
                << "domain '" << domain->name()
                << "' has reached the maximum number of queues (limit: "
                << domain->config().maxQueues() << ")." << MWCTSK_ALARMLOG_END;
        }

        static void alarm(mqbi::Domain* domain, int queues)
        {
            MWCTSK_ALARMLOG_ALARM("DOMAIN_QUEUE_LIMIT_HIGH_WATERMARK")
                << "domain '" << domain->name() << "' has reached the "
                << (k_MAX_QUEUES_HIGH_WATERMARK * 100)
                << "% watermark limit for the number of queues "
                   "(current: "
                << queues << ", limit: " << domain->config().maxQueues()
                << ")." << MWCTSK_ALARMLOG_END;
        }
    };

    const int registeredQueues = domIt->second->numAssignedQueues();
    const int maxQueues        = domIt->second->domain()->config().maxQueues();
    if (maxQueues != 0) {
        const int requestedQueues = registeredQueues + 1;
        if (requestedQueues > maxQueues) {
            local::panic(domIt->second->domain());
        }
        else {
            const int watermark = static_cast<int>(
                maxQueues * k_MAX_QUEUES_HIGH_WATERMARK);
            if (registeredQueues < watermark && requestedQueues >= watermark) {
                local::alarm(domIt->second->domain(), requestedQueues);
            }
        }

        if (requestedQueues > maxQueues) {
            if (status) {
                status->category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
                status->code()     = mqbi::ClusterErrorCode::e_LIMIT;
                status->message()  = k_MAXIMUM_NUMBER_OF_QUEUES_REACHED;
            }

            return QueueAssignmentResult::k_ASSIGNMENT_REJECTED;  // RETURN
        }
    }

    // Queue is no longer pending unassignment
    const DomainStatesCIter cit = clusterState->domainStates().find(
        uri.qualifiedDomain());
    if (cit != clusterState->domainStates().cend()) {
        UriToQueueInfoMapCIter qcit = cit->second->queuesInfo().find(uri);
        if (qcit != cit->second->queuesInfo().cend()) {
            BSLS_ASSERT_SAFE(cluster->isCSLModeEnabled() &&
                             qcit->second->pendingUnassignment());
            qcit->second->setPendingUnassignment(false);
        }
    }

    // Populate 'queueAssignmentAdvisory'
    bdlma::LocalSequentialAllocator<1024>  localAllocator(allocator);
    bmqp_ctrlmsg::ControlMessage           controlMsg(&localAllocator);
    bmqp_ctrlmsg::QueueAssignmentAdvisory& queueAdvisory =
        controlMsg.choice()
            .makeClusterMessage()
            .choice()
            .makeQueueAssignmentAdvisory();

    mqbu::StorageKey key;
    populateQueueAssignmentAdvisory(&queueAdvisory,
                                    &key,
                                    clusterState,
                                    clusterData,
                                    uri,
                                    domIt->second->domain(),
                                    cluster->isCSLModeEnabled());
    if (cluster->isCSLModeEnabled()) {
        // In CSL mode, we delay the insertion to queueKeys until
        // 'onQueueAssigned' observer callback.

        clusterState->queueKeys().erase(key);
    }

    // Apply 'queueAssignmentAdvisory' to CSL
    BALL_LOG_INFO << clusterData->identity().description()
                  << ": 'QueueAssignmentAdvisory' will be applied to "
                  << " cluster state ledger: " << queueAdvisory;

    const int rc = ledger->apply(queueAdvisory);
    if (rc != 0) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to apply queue assignment advisory: "
                       << queueAdvisory << ", rc: " << rc;
    }

    if (!cluster->isCSLModeEnabled()) {
        // In CSL mode, we assign the queue to ClusterState upon CSL commit
        // callback of QueueAssignmentAdvisory, so we don't assign it here.

        BSLA_MAYBE_UNUSED const bool assignRc = clusterState->assignQueue(
            uri,
            key,
            queueAdvisory.queues().back().partitionId(),
            AppIdInfos());
        BSLS_ASSERT_SAFE(assignRc);

        domIt->second->adjustQueueCount(1);

        BALL_LOG_INFO << cluster->description()
                      << ": Queue assigned: " << queueAdvisory;

        // Broadcast 'queueAssignmentAdvisory' to all followers
        clusterData->messageTransmitter().broadcastMessage(controlMsg);
    }

    queueAssigningCb(uri, true);  // processingPendingRequests

    return QueueAssignmentResult::k_ASSIGNMENT_OK;
}

void ClusterUtil::registerQueueInfo(ClusterState*           clusterState,
                                    const mqbi::Cluster*    cluster,
                                    const bmqt::Uri&        uri,
                                    int                     partitionId,
                                    const mqbu::StorageKey& queueKey,
                                    const AppIdInfos&       appIdInfos,
                                    const QueueAssigningCb& queueAssigningCb,
                                    bool                    forceUpdate)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster->dispatcher()->inDispatcherThread(cluster));
    BSLS_ASSERT_SAFE(!cluster->isRemote());
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(uri.isCanonical());
    BSLS_ASSERT_SAFE((0 <= partitionId) &&
                     (static_cast<size_t>(partitionId) <
                      clusterState->partitions().size()));

    // This method is invoked under 2 scenarios:
    // 1) After self node has finished recovery at startup, and cluster then
    //    attempts to register the queues recovered from the storage with this
    //    component.  Note that this component may already be aware of the
    //    queue.  This could occur if self node received a queue assignment
    //    advisory for the queue from the leader, or received an open-queue
    //    request for the queue.  In this scenario, this routine will be
    //    invoked with 'forceUpdate=false' flag.
    // 2) Self node was chosen as the leader, and during the leader-sync step,
    //    found out that it has stale view for the queue.  In this scenario,
    //    this routine will be invoked with 'forceUpdate=true' flag.
    //
    // Note that in scenario (1), the caller does not ask this routine to
    // force-update the queue info.  We may need to change this later once we
    // implement a more robust startup sequence.

    DomainStatesIter domIt = clusterState->domainStates().find(
        uri.qualifiedDomain());
    if (domIt != clusterState->domainStates().end()) {
        UriToQueueInfoMapCIter cit = domIt->second->queuesInfo().find(uri);
        if (cit != domIt->second->queuesInfo().cend()) {
            // Queue is assigned.
            QueueInfoSp qs = cit->second;
            BSLS_ASSERT_SAFE(qs);
            BSLS_ASSERT_SAFE(qs->uri() == uri);

            if ((qs->partitionId() == partitionId) &&
                (qs->key() == queueKey) && (qs->appIdInfos() == appIdInfos)) {
                // All good.. nothing to update.
                return;  // RETURN
            }

            mwcu::Printer<AppIdInfos> stateAppIdInfos(&qs->appIdInfos());
            mwcu::Printer<AppIdInfos> storageAppIdInfos(&appIdInfos);

            // PartitionId and/or QueueKey and/or AppIdInfos mismatch.
            if (!forceUpdate) {
                MWCTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                    << cluster->description() << ": For queue [ " << uri
                    << "], different partitionId/queueKey/appIdInfos in "
                    << "cluster state and storage.  "
                    << "PartitionId/QueueKey/AppIdInfos in cluster state ["
                    << qs->partitionId() << "], [" << qs->key() << "], ["
                    << stateAppIdInfos
                    << "].  PartitionId/QueueKey/AppIdInfos in storage ["
                    << partitionId << "], [" << queueKey << "], ["
                    << storageAppIdInfos << "]." << MWCTSK_ALARMLOG_END;
                return;  // RETURN
            }

            BALL_LOG_WARN << cluster->description() << ": For queue [" << uri
                          << "], force-updating "
                          << "partitionId/queueKey/appIdInfos from ["
                          << qs->partitionId() << "], [" << qs->key() << "], ["
                          << stateAppIdInfos << "] to [" << partitionId
                          << "], [" << queueKey << "], [" << storageAppIdInfos
                          << "].";

            clusterState->queueKeys().erase(qs->key());

            clusterState->assignQueue(uri, queueKey, partitionId, appIdInfos);

            BALL_LOG_INFO << cluster->description() << ": Queue assigned: "
                          << "[uri: " << uri << ", queueKey: " << queueKey
                          << ", partitionId: " << partitionId;

            if (!cluster->isCSLModeEnabled()) {
                ClusterState::QueueKeysInsertRc insertRc =
                    clusterState->queueKeys().insert(queueKey);
                if (false == insertRc.second) {
                    MWCTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                        << cluster->description()
                        << ": re-registering a known queue with a stale view, "
                        << "but queueKey is not unique. "
                        << "QueueKey [" << queueKey << "], URI [" << uri
                        << "], PartitionId [" << partitionId
                        << "], AppIdInfos [" << storageAppIdInfos << "]."
                        << MWCTSK_ALARMLOG_END;
                    return;  // RETURN
                }

                clusterState->domainStates()
                    .at(uri.qualifiedDomain())
                    ->adjustQueueCount(1);
            }

            BSLS_ASSERT_SAFE(1 == clusterState->queueKeys().count(queueKey));

            return;  // RETURN
        }
    }

    // Queue is not known, so add it.
    clusterState->assignQueue(uri, queueKey, partitionId, appIdInfos);

    mwcu::Printer<AppIdInfos> printer(&appIdInfos);
    BALL_LOG_INFO << cluster->description() << ": Queue assigned: "
                  << "[uri: " << uri << ", queueKey: " << queueKey
                  << ", partitionId: " << partitionId
                  << ", appIdInfos: " << printer << "]";

    if (!cluster->isCSLModeEnabled()) {
        ClusterState::QueueKeysInsertRc insertRc =
            clusterState->queueKeys().insert(queueKey);
        if (false == insertRc.second) {
            // Duplicate queue key.
            MWCTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                << cluster->description()
                << ": registering a queue for an unknown queue, but "
                << "queueKey is not unique. QueueKey [" << queueKey
                << "], URI [" << uri << "], PartitionId [" << partitionId
                << "]." << MWCTSK_ALARMLOG_END;
            return;  // RETURN
        }

        clusterState->domainStates()
            .at(uri.qualifiedDomain())
            ->adjustQueueCount(1);
    }

    queueAssigningCb(uri, false);  // processingPendingRequests
}

void ClusterUtil::populateAppIdInfos(AppIdInfos*                appIdInfos,
                                     const mqbconfm::QueueMode& domainConfig)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(appIdInfos && appIdInfos->empty());

    if (domainConfig.isFanoutValue()) {
        const bsl::vector<bsl::string>& cfgAppIds =
            domainConfig.fanout().appIDs();
        bsl::unordered_set<mqbu::StorageKey> appKeys;

        for (bsl::vector<bsl::string>::const_iterator cit = cfgAppIds.begin();
             cit != cfgAppIds.end();
             ++cit) {
            mqbu::StorageKey appKey;
            mqbs::StorageUtil::generateStorageKey(&appKey, &appKeys, *cit);

            appIdInfos->insert(AppIdInfo(*cit, appKey));
        }
    }
    else {
        appIdInfos->insert(AppIdInfo(bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
                                     mqbi::QueueEngine::k_DEFAULT_APP_KEY));
    }
}

void ClusterUtil::populateAppIdInfos(
    bsl::vector<bmqp_ctrlmsg::AppIdInfo>* appIdInfos,
    const mqbconfm::QueueMode&            domainConfig)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(appIdInfos && appIdInfos->empty());

    if (domainConfig.isFanoutValue()) {
        const bsl::vector<bsl::string>& cfgAppIds =
            domainConfig.fanout().appIDs();
        bsl::unordered_set<mqbu::StorageKey> appKeys;

        for (bsl::vector<bsl::string>::const_iterator cit = cfgAppIds.begin();
             cit != cfgAppIds.end();
             ++cit) {
            bmqp_ctrlmsg::AppIdInfo appIdInfo;
            appIdInfo.appId() = *cit;

            mqbu::StorageKey appKey;
            mqbs::StorageUtil::generateStorageKey(&appKey, &appKeys, *cit);
            appKey.loadBinary(&appIdInfo.appKey());

            appIdInfos->push_back(appIdInfo);
        }
    }
    else {
        bmqp_ctrlmsg::AppIdInfo appIdInfo;
        appIdInfo.appId() = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
        mqbi::QueueEngine::k_DEFAULT_APP_KEY.loadBinary(&appIdInfo.appKey());

        appIdInfos->push_back(appIdInfo);
    }
}

void ClusterUtil::registerAppId(ClusterData*        clusterData,
                                ClusterStateLedger* ledger,
                                const ClusterState& clusterState,
                                const bsl::string&  appId,
                                const mqbi::Domain* domain,
                                bslma::Allocator*   allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(ledger && ledger->isOpen());
    BSLS_ASSERT_SAFE(domain);
    BSLS_ASSERT_SAFE(allocator);

    if (mqbnet::ElectorState::e_LEADER !=
        clusterData->electorInfo().electorState()) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to register appId '" << appId
                       << "' for domain '" << domain->name()
                       << "'. Self is not leader.";
        return;  // RETURN
    }

    if (ElectorInfoLeaderStatus::e_ACTIVE !=
        clusterData->electorInfo().leaderStatus()) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to register appId '" << appId
                       << "' for domain '" << domain->name()
                       << "'. Self is leader but is not active.";
        return;  // RETURN
    }

    if (clusterData->membership().selfNodeStatus() ==
        bmqp_ctrlmsg::NodeStatus::E_STOPPING) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to register appId '" << appId
                       << "' for domain '" << domain->name()
                       << "'. Self is active leader but is stopping.";
        return;  // RETURN
    }

    // Populate 'queueUpdateAdvisory'
    bdlma::LocalSequentialAllocator<1024> localAllocator(allocator);
    bmqp_ctrlmsg::QueueUpdateAdvisory     queueAdvisory(&localAllocator);
    clusterData->electorInfo().nextLeaderMessageSequence(
        &queueAdvisory.sequenceNumber());

    DomainStatesCIter domCit = clusterState.domainStates().find(
        domain->name());
    if (domCit == clusterState.domainStates().cend() ||
        domCit->second->queuesInfo().empty()) {
        // We will reach this scenario if we attempt to register an appId for
        // a domain with no opened queues.  This is still a valid scenario, so
        // we set a *NULL* queueUri/queueKey/partitionId for the
        // QueueUpdateAdvisory to indicate that we are updating appIds for the
        // entire domain.

        bmqp_ctrlmsg::QueueInfoUpdate queueUpdate;
        queueUpdate.uri()         = "";
        queueUpdate.partitionId() = mqbs::DataStore::k_INVALID_PARTITION_ID;
        mqbu::StorageKey::k_NULL_KEY.loadBinary((&queueUpdate.key()));
        queueUpdate.domain() = domain->name();

        // Populate AppIdInfo
        bmqp_ctrlmsg::AppIdInfo appIdInfo;
        appIdInfo.appId() = appId;
        mqbu::StorageKey::k_NULL_KEY.loadBinary(&appIdInfo.appKey());

        queueUpdate.addedAppIds().push_back(appIdInfo);
        queueAdvisory.queueUpdates().push_back(queueUpdate);
    }
    else {
        for (UriToQueueInfoMapCIter qinfoCit =
                 domCit->second->queuesInfo().cbegin();
             qinfoCit != domCit->second->queuesInfo().cend();
             ++qinfoCit) {
            bmqp_ctrlmsg::QueueInfoUpdate queueUpdate;
            queueUpdate.uri()         = qinfoCit->second->uri().asString();
            queueUpdate.partitionId() = qinfoCit->second->partitionId();
            qinfoCit->second->key().loadBinary((&queueUpdate.key()));
            queueUpdate.domain() = qinfoCit->second->uri().qualifiedDomain();
            BSLS_ASSERT_SAFE(queueUpdate.domain() == domain->name());

            bsl::unordered_set<mqbu::StorageKey> appKeys;
            const AppIdInfos& appIdInfos = qinfoCit->second->appIdInfos();
            for (AppIdInfosCIter appInfoCit = appIdInfos.cbegin();
                 appInfoCit != appIdInfos.cend();
                 ++appInfoCit) {
                if (appInfoCit->first == appId) {
                    BALL_LOG_ERROR << "Failed to register appId '" << appId
                                   << "'. It already exists in queue '"
                                   << qinfoCit->second->uri() << "'.";

                    return;  // RETURN
                }

                appKeys.insert(appInfoCit->second);
            }

            // Populate AppIdInfo
            bmqp_ctrlmsg::AppIdInfo appIdInfo;
            appIdInfo.appId() = appId;
            mqbu::StorageKey appKey;
            mqbs::StorageUtil::generateStorageKey(&appKey, &appKeys, appId);
            appKey.loadBinary(&appIdInfo.appKey());

            queueUpdate.addedAppIds().push_back(appIdInfo);
            queueAdvisory.queueUpdates().push_back(queueUpdate);
        }
    }

    // Apply 'queueUpdateAdvisory' to CSL
    BALL_LOG_INFO << clusterData->identity().description()
                  << ": 'QueueUpdateAdvisory' will be applied to cluster "
                  << "state ledger: " << queueAdvisory;

    const int rc = ledger->apply(queueAdvisory);
    if (rc != 0) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to apply queue update advisory: "
                       << queueAdvisory << ", rc: " << rc;
    }
    else {
        BALL_LOG_INFO << "Registered appId '" << appId << "' for domain '"
                      << domain->name() << "'";
    }
}

void ClusterUtil::unregisterAppId(ClusterData*        clusterData,
                                  ClusterStateLedger* ledger,
                                  const ClusterState& clusterState,
                                  const bsl::string&  appId,
                                  const mqbi::Domain* domain,
                                  bslma::Allocator*   allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(ledger && ledger->isOpen());
    BSLS_ASSERT_SAFE(domain);
    BSLS_ASSERT_SAFE(allocator);

    if (mqbnet::ElectorState::e_LEADER !=
        clusterData->electorInfo().electorState()) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to unregister appId '" << appId
                       << "' for domain '" << domain->name()
                       << "'. Self is not leader.";
        return;  // RETURN
    }

    if (mqbc::ElectorInfoLeaderStatus::e_ACTIVE !=
        clusterData->electorInfo().leaderStatus()) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to unregister appId '" << appId
                       << "' for domain '" << domain->name()
                       << "'. Self is leader but is not active.";
        return;  // RETURN
    }

    if (clusterData->membership().selfNodeStatus() ==
        bmqp_ctrlmsg::NodeStatus::E_STOPPING) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to unregister appId '" << appId
                       << "' for domain '" << domain->name()
                       << "'. Self is active leader but is stopping.";
        return;  // RETURN
    }

    // Populate 'queueUpdateAdvisory'
    bdlma::LocalSequentialAllocator<1024> localAllocator(allocator);
    bmqp_ctrlmsg::QueueUpdateAdvisory     queueAdvisory(&localAllocator);
    clusterData->electorInfo().nextLeaderMessageSequence(
        &queueAdvisory.sequenceNumber());

    DomainStatesCIter domCit = clusterState.domainStates().find(
        domain->name());
    if (domCit == clusterState.domainStates().cend() ||
        domCit->second->queuesInfo().empty()) {
        // We will reach this scenario if we attempt to unregister an appId for
        // a domain with no opened queues.  This is still a valid scenario, so
        // we set a *NULL* queueUri/queueKey/partitionId for the
        // QueueUpdateAdvisory to indicate that we are updating appIds for the
        // entire domain.

        bmqt::Uri                     uriii("bmq://bmq.test.mmap.priority/q1");
        bmqp_ctrlmsg::QueueInfoUpdate queueUpdate;
        queueUpdate.uri()         = "";
        queueUpdate.partitionId() = mqbs::DataStore::k_INVALID_PARTITION_ID;
        mqbu::StorageKey::k_NULL_KEY.loadBinary((&queueUpdate.key()));
        queueUpdate.domain() = domain->name();

        // Populate AppIdInfo
        bmqp_ctrlmsg::AppIdInfo appIdInfo;
        appIdInfo.appId() = appId;
        mqbu::StorageKey::k_NULL_KEY.loadBinary(&appIdInfo.appKey());

        queueUpdate.removedAppIds().push_back(appIdInfo);
        queueAdvisory.queueUpdates().push_back(queueUpdate);
    }
    else {
        for (UriToQueueInfoMapCIter qinfoCit =
                 domCit->second->queuesInfo().cbegin();
             qinfoCit != domCit->second->queuesInfo().cend();
             ++qinfoCit) {
            bmqp_ctrlmsg::QueueInfoUpdate queueUpdate;
            queueUpdate.uri()         = qinfoCit->second->uri().asString();
            queueUpdate.partitionId() = qinfoCit->second->partitionId();
            qinfoCit->second->key().loadBinary((&queueUpdate.key()));
            queueUpdate.domain() = qinfoCit->second->uri().qualifiedDomain();
            BSLS_ASSERT_SAFE(queueUpdate.domain() == domain->name());

            bool              appIdFound = false;
            const AppIdInfos& appIdInfos = qinfoCit->second->appIdInfos();
            for (AppIdInfosCIter appInfoCit = appIdInfos.cbegin();
                 appInfoCit != appIdInfos.cend();
                 ++appInfoCit) {
                if (appInfoCit->first == appId) {
                    // Populate AppIdInfo
                    bmqp_ctrlmsg::AppIdInfo appIdInfo;
                    appIdInfo.appId() = appId;
                    appInfoCit->second.loadBinary(&appIdInfo.appKey());

                    queueUpdate.removedAppIds().push_back(appIdInfo);
                    queueAdvisory.queueUpdates().push_back(queueUpdate);

                    appIdFound = true;
                    break;
                }
            }

            if (!appIdFound) {
                BALL_LOG_ERROR << "Failed to unregister appId '" << appId
                               << "'. It does not exist in queue '"
                               << qinfoCit->second->uri() << "'.";

                return;  // RETURN
            }
        }
    }

    // Apply 'queueUpdateAdvisory' to CSL
    BALL_LOG_INFO << clusterData->identity().description()
                  << ": 'QueueUpdateAdvisory' will be applied to cluster "
                  << "state ledger: " << queueAdvisory;

    const int rc = ledger->apply(queueAdvisory);
    if (rc != 0) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to apply queue update advisory: "
                       << queueAdvisory << ", rc: " << rc;
    }
    else {
        BALL_LOG_INFO << "Unregistered appId '" << appId << "' for domain '"
                      << domain->name() << "'";
    }
}

void ClusterUtil::sendClusterState(
    ClusterData*          clusterData,
    ClusterStateLedger*   ledger,
    mqbi::StorageManager* storageManager,
    const ClusterState&   clusterState,
    bool                  sendPartitionPrimaryInfo,
    bool                  sendQueuesInfo,
    mqbnet::ClusterNode*  node,
    const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitions)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(clusterData->cluster());
    BSLS_ASSERT_SAFE(clusterData->cluster()->dispatcher()->inDispatcherThread(
        clusterData->cluster()));
    BSLS_ASSERT_SAFE(mqbnet::ElectorState::e_LEADER ==
                     clusterData->electorInfo().electorState());
    BSLS_ASSERT_SAFE(ledger && ledger->isOpen());

    if (sendPartitionPrimaryInfo) {
        BSLS_ASSERT_SAFE(partitions.size() ==
                         clusterState.partitions().size());
    }
    else {
        BSLS_ASSERT_SAFE(partitions.empty());
    };

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
        clusterData->membership().selfNodeStatus()) {
        // No need to send cluster state since self is stopping.  After self
        // has stopped, a new leader will be elected whom will take care of
        // this duty,
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(
        clusterData->clusterConfig().clusterAttributes().isFSMWorkflow() ||
        bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
            clusterData->membership().selfNodeStatus());

    bmqp_ctrlmsg::ControlMessage  controlMessage;
    bmqp_ctrlmsg::ClusterMessage& clusterMessage =
        controlMessage.choice().makeClusterMessage();
    if (sendPartitionPrimaryInfo && sendQueuesInfo) {
        bmqp_ctrlmsg::LeaderAdvisory& advisory =
            clusterMessage.choice().makeLeaderAdvisory();

        clusterData->electorInfo().nextLeaderMessageSequence(
            &advisory.sequenceNumber());

        advisory.partitions() = partitions;
        loadQueuesInfo(&advisory.queues(),
                       clusterState,
                       clusterData->cluster()->isCSLModeEnabled());
    }
    else if (sendPartitionPrimaryInfo) {
        bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory =
            clusterMessage.choice().makePartitionPrimaryAdvisory();

        clusterData->electorInfo().nextLeaderMessageSequence(
            &advisory.sequenceNumber());

        advisory.partitions() = partitions;
    }
    else {
        BSLS_ASSERT_SAFE(sendQueuesInfo);

        bmqp_ctrlmsg::QueueAssignmentAdvisory& advisory =
            clusterMessage.choice().makeQueueAssignmentAdvisory();

        clusterData->electorInfo().nextLeaderMessageSequence(
            &advisory.sequenceNumber());

        loadQueuesInfo(&advisory.queues(),
                       clusterState,
                       clusterData->cluster()->isCSLModeEnabled());
    }

    if (!clusterData->cluster()->isCSLModeEnabled()) {
        if (node) {
            clusterData->messageTransmitter().sendMessage(controlMessage,
                                                          node);
        }
        else {
            clusterData->messageTransmitter().broadcastMessage(controlMessage);

            // Inform local storage.  This should be done after broadcasting
            // advisory above, because storageMgr or its partitions may also
            // send some events to peer nodes in
            // StorageManager::setPrimaryForPartition() below.
            // TBD: This should be done in the caller of this routine.
            BSLS_ASSERT_SAFE(storageManager);

            for (unsigned int i = 0; i < partitions.size(); ++i) {
                const bmqp_ctrlmsg::PartitionPrimaryInfo& info = partitions[i];
                storageManager->setPrimaryForPartition(
                    info.partitionId(),
                    clusterData->membership().netCluster()->lookupNode(
                        info.primaryNodeId()),
                    info.primaryLeaseId());
            }
        }
    }

    const int rc = ledger->apply(clusterMessage);
    if (rc != 0) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to apply cluster message: "
                       << clusterMessage << ", rc: " << rc
                       << " as part of 'sendClusterState'";
    }
    else {
        BALL_LOG_INFO << clusterData->identity().description()
                      << ": Applied cluster message: " << clusterMessage
                      << " as part of 'sendClusterState'";
    }
}

void ClusterUtil::appendClusterNode(bsl::vector<mqbcfg::ClusterNode>* out,
                                    const bslstl::StringRef&          name,
                                    const bslstl::StringRef& dataCenter,
                                    int                      port,
                                    int                      id,
                                    bslma::Allocator*        allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    mwcu::MemOutStream endpoint(allocator);
    endpoint << "tcp://localhost:" << port;

    out->emplace_back();
    out->back().name()                           = name;
    out->back().dataCenter()                     = dataCenter;
    out->back().transport().makeTcp().endpoint() = endpoint.str();
    out->back().id()                             = id;
}

void ClusterUtil::apply(mqbc::ClusterState*                 clusterState,
                        const bmqp_ctrlmsg::ClusterMessage& clusterMessage,
                        const mqbc::ClusterData&            clusterData)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterState);

    typedef bmqp_ctrlmsg::ClusterMessageChoice MsgChoice;  // shortcut
    switch (clusterMessage.choice().selectionId()) {
    case MsgChoice::SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        const bmqp_ctrlmsg::PartitionPrimaryAdvisory& adv =
            clusterMessage.choice().partitionPrimaryAdvisory();
        applyPartitionPrimary(clusterState, adv.partitions(), clusterData);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_LEADER_ADVISORY: {
        const bmqp_ctrlmsg::LeaderAdvisory& adv =
            clusterMessage.choice().leaderAdvisory();
        applyPartitionPrimary(clusterState, adv.partitions(), clusterData);
        applyQueueAssignment(clusterState, adv.queues());
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        const bmqp_ctrlmsg::QueueAssignmentAdvisory& queueAdvisory =
            clusterMessage.choice().queueAssignmentAdvisory();
        applyQueueAssignment(clusterState, queueAdvisory.queues());
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        const bmqp_ctrlmsg::QueueUnassignedAdvisory& queueAdvisory =
            clusterMessage.choice().queueUnassignedAdvisory();

        applyQueueUnassignment(clusterState, queueAdvisory.queues());
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
        const bmqp_ctrlmsg::QueueUpdateAdvisory& queueAdvisory =
            clusterMessage.choice().queueUpdateAdvisory();
        applyQueueUpdate(clusterState,
                         queueAdvisory.queueUpdates(),
                         clusterData);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_UNDEFINED:
    default: {
        MWCTSK_ALARMLOG_ALARM("CLUSTER")
            << clusterData.identity().description()
            << ": Unexpected clusterMessage: " << clusterMessage
            << MWCTSK_ALARMLOG_END;
    } break;  // BREAK
    }
}

int ClusterUtil::validateState(bsl::ostream&             errorDescription,
                               const mqbc::ClusterState& state,
                               const mqbc::ClusterState& reference)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(state.partitions().size() ==
                     reference.partitions().size());

    bool seenIncorrectPartitionInfo = false;
    bool seenIncorrectQueueInfo     = false;
    bool seenMissingQueue           = false;
    bool seenExtraQueue             = false;

    mwcu::MemOutStream out;
    const int          level = 0;

    // Check incorrect partition information
    for (size_t pid = 0; pid < state.partitions().size(); ++pid) {
        const mqbc::ClusterStatePartitionInfo& stateInfo =
            state.partitions()[pid];
        const mqbc::ClusterStatePartitionInfo& referenceInfo =
            reference.partitions()[pid];
        if (stateInfo.partitionId() != referenceInfo.partitionId() ||
            stateInfo.primaryLeaseId() != referenceInfo.primaryLeaseId()) {
            // Partition information mismatch.  Note that we don't compare
            // primaryNodeIds here because 'state' is initialized with cluster
            // state ledger's contents and will likely have a valid
            // primaryNodeId (from previous run of the broker).  However,
            // 'reference' state is the true copy, currently owned by this
            // instance of the broker, and will likely have an invalid
            // primaryNodeId, specially if a primary has not yet been assigned
            // in the startup sequence.  If if a primary has been assigned, its
            // nodeId may be different one.
            if (!seenIncorrectPartitionInfo) {
                bdlb::Print::newlineAndIndent(out, level);
                out << "---------------------------";
                bdlb::Print::newlineAndIndent(out, level);
                out << "Incorrect Partition Infos :";
                bdlb::Print::newlineAndIndent(out, level);
                out << "---------------------------";
                seenIncorrectPartitionInfo = true;
            }
            bdlb::Print::newlineAndIndent(out, level + 1);
            out << "[partitionId: " << stateInfo.partitionId()
                << ", primaryLeaseId: " << stateInfo.primaryLeaseId()
                << ", primaryNodeId: " << stateInfo.primaryNodeId() << "]"
                << " (Correct: "
                << "[partitionId: " << referenceInfo.partitionId()
                << ", primaryLeaseId: " << referenceInfo.primaryLeaseId()
                << ", primaryNodeId: " << referenceInfo.primaryNodeId() << "]"
                << ")";
        }
    }

    // Check incorrect queue information
    for (ClusterState::DomainStatesCIter domCit = state.domainStates().begin();
         domCit != state.domainStates().end();
         ++domCit) {
        const bsl::string& domainName = domCit->first;

        ClusterState::DomainStatesCIter refDomCit =
            reference.domainStates().find(domainName);
        if (refDomCit == reference.domainStates().cend()) {
            // Domain not found in both states
            continue;  // CONTINUE
        }

        for (ClusterState::UriToQueueInfoMapCIter citer =
                 domCit->second->queuesInfo().begin();
             citer != domCit->second->queuesInfo().end();
             ++citer) {
            const bmqt::Uri& uri = citer->first;

            ClusterState::UriToQueueInfoMapCIter refCiter =
                refDomCit->second->queuesInfo().find(uri);
            if (refCiter == refDomCit->second->queuesInfo().cend()) {
                // Queue not found in both states
                continue;  // CONTINUE
            }

            const bsl::shared_ptr<ClusterStateQueueInfo>& referenceInfo =
                refCiter->second;
            const bsl::shared_ptr<ClusterStateQueueInfo>& info = citer->second;
            if (info->uri() == referenceInfo->uri() &&
                info->key() == referenceInfo->key() &&
                info->partitionId() == referenceInfo->partitionId()) {
                continue;  // CONTINUE
            }

            if (!seenIncorrectQueueInfo) {
                bdlb::Print::newlineAndIndent(out, level);
                out << "-----------------------------";
                bdlb::Print::newlineAndIndent(out, level);
                out << "Incorrect Queue Information :";
                bdlb::Print::newlineAndIndent(out, level);
                out << "-----------------------------";
                seenIncorrectQueueInfo = true;
            }

            bdlb::Print::newlineAndIndent(out, level + 1);
            out << "[key: " << info->key() << ", uri: " << info->uri()
                << ", partitionId: " << info->partitionId() << "]"
                << " (Correct: "
                << "[key: " << referenceInfo->key()
                << ", uri: " << referenceInfo->uri()
                << ", partitionId: " << referenceInfo->partitionId() << "]"
                << ")";
        }
    }

    // Check missing queues
    bsl::vector<bsl::shared_ptr<ClusterStateQueueInfo> > missingQueues;
    for (ClusterState::DomainStatesCIter refDomCit =
             reference.domainStates().begin();
         refDomCit != reference.domainStates().end();
         ++refDomCit) {
        const bsl::string& domainName = refDomCit->first;

        ClusterState::DomainStatesCIter domCit = state.domainStates().find(
            domainName);
        if (domCit == state.domainStates().cend()) {
            // Entire domain of queues is not found
            for (ClusterState::UriToQueueInfoMapCIter refCiter =
                     refDomCit->second->queuesInfo().cbegin();
                 refCiter != refDomCit->second->queuesInfo().cend();
                 ++refCiter) {
                missingQueues.push_back(refCiter->second);
            }

            continue;  // CONTINUE
        }

        for (ClusterState::UriToQueueInfoMapCIter refCiter =
                 refDomCit->second->queuesInfo().cbegin();
             refCiter != refDomCit->second->queuesInfo().cend();
             ++refCiter) {
            const bmqt::Uri& uri = refCiter->first;

            ClusterState::UriToQueueInfoMapCIter citer =
                domCit->second->queuesInfo().find(uri);
            if (citer != domCit->second->queuesInfo().cend()) {
                // Queue is found in both states
                continue;  // CONTINUE
            }

            missingQueues.push_back(refCiter->second);
        }
    }

    // Queue is missing from the cluster state
    if (!missingQueues.empty()) {
        bdlb::Print::newlineAndIndent(out, level);
        out << "----------------";
        bdlb::Print::newlineAndIndent(out, level);
        out << "Missing queues :";
        bdlb::Print::newlineAndIndent(out, level);
        out << "----------------";
        seenMissingQueue = true;
    }

    for (bsl::vector<bsl::shared_ptr<ClusterStateQueueInfo> >::const_iterator
             citer = missingQueues.cbegin();
         citer != missingQueues.cend();
         ++citer) {
        bdlb::Print::newlineAndIndent(out, level + 1);
        out << "[key: " << (*citer)->key() << ", uri: " << (*citer)->uri()
            << ", partitionId: " << (*citer)->partitionId() << "]";
    }

    // Check extra queues
    bsl::vector<bsl::shared_ptr<ClusterStateQueueInfo> > extraQueues;
    for (ClusterState::DomainStatesCIter domCit = state.domainStates().begin();
         domCit != state.domainStates().end();
         ++domCit) {
        const bsl::string& domainName = domCit->first;

        ClusterState::DomainStatesCIter refDomCit =
            reference.domainStates().find(domainName);
        if (refDomCit == reference.domainStates().cend()) {
            // Entire domain of queues is not found
            for (ClusterState::UriToQueueInfoMapCIter citer =
                     domCit->second->queuesInfo().cbegin();
                 citer != domCit->second->queuesInfo().cend();
                 ++citer) {
                extraQueues.push_back(citer->second);
            }

            continue;  // CONTINUE
        }

        for (ClusterState::UriToQueueInfoMapCIter citer =
                 domCit->second->queuesInfo().cbegin();
             citer != domCit->second->queuesInfo().cend();
             ++citer) {
            const bmqt::Uri& uri = citer->first;

            ClusterState::UriToQueueInfoMapCIter refCiter =
                refDomCit->second->queuesInfo().find(uri);
            if (refCiter != refDomCit->second->queuesInfo().cend()) {
                // Queue is found in both states
                continue;  // CONTINUE
            }

            extraQueues.push_back(citer->second);
        }
    }

    // Extra queue in the cluster state
    if (!extraQueues.empty()) {
        bdlb::Print::newlineAndIndent(out, level);
        out << "--------------";
        bdlb::Print::newlineAndIndent(out, level);
        out << "Extra queues :";
        bdlb::Print::newlineAndIndent(out, level);
        out << "--------------";
        seenExtraQueue = true;
    }

    for (bsl::vector<bsl::shared_ptr<ClusterStateQueueInfo> >::const_iterator
             citer = extraQueues.cbegin();
         citer != extraQueues.cend();
         ++citer) {
        bdlb::Print::newlineAndIndent(out, level + 1);
        out << "[key: " << (*citer)->key() << ", uri: " << (*citer)->uri()
            << ", partitionId: " << (*citer)->partitionId() << "]";
    }

    if (seenIncorrectPartitionInfo || seenIncorrectQueueInfo ||
        seenMissingQueue || seenExtraQueue) {
        // Inconsistency detected
        errorDescription << out.str();
        return -1;  // RETURN
    }

    return 0;
}

void ClusterUtil::validateClusterStateLedger(mqbi::Cluster*            cluster,
                                             const ClusterStateLedger& ledger,
                                             const ClusterState& clusterState,
                                             const ClusterData&  clusterData,
                                             bslma::Allocator*   allocator)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster);
    BSLS_ASSERT_SAFE(
        clusterData.dispatcherClientData().dispatcher()->inDispatcherThread(
            cluster));

    if (!ledger.isOpen()) {
        BALL_LOG_INFO << clusterData.identity().description()
                      << ": Skipping validation of cluster state ledger at "
                      << "this time as it is not yet opened.";
        return;
    }

    // Compare contents of cluster state and CSL contents on disk
    ClusterState tempState(cluster,
                           clusterState.partitions().size(),
                           allocator);

    int rc =
        load(&tempState, ledger.getIterator().get(), clusterData, allocator);
    if (rc != 0) {
        BALL_LOG_ERROR << clusterData.identity().description()
                       << ": Unable to load cluster state from contents of "
                       << "cluster state ledger [rc:" << rc << "].";
        return;  // RETURN
    }

    mwcu::MemOutStream errorDescription;
    rc = validateState(errorDescription, tempState, clusterState);
    if (rc != 0) {
        BALL_LOG_WARN_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << clusterData.identity().description()
                                   << ": Cluster state ledger's contents are"
                                   << " different from the cluster state: "
                                   << errorDescription.str();
            printQueues(BALL_LOG_OUTPUT_STREAM, clusterState);
        }
    }
}

int ClusterUtil::load(ClusterState*               state,
                      ClusterStateLedgerIterator* iterator,
                      const ClusterData&          clusterData,
                      bslma::Allocator*           allocator)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(state);
    BSLS_ASSERT_SAFE(iterator);
    BSLS_ASSERT_SAFE(clusterData.cluster()->dispatcher()->inDispatcherThread(
        clusterData.cluster()));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // No error
        ,
        rc_ITERATION_ERROR = -1  // An error was encountered while iterating
                                 // through the iterator
        ,
        rc_MESSAGE_LOAD_ERROR = -2  // An error was encountered while
                                    // attempting to load a message
    };

    // Point the CSL iterator to the latest snapshot record, since we only need
    // to start loading from there.
    bslma::ManagedPtr<ClusterStateLedgerIterator> latestIter = iterator->clone(
        allocator);

    int rc = latestIter->next();
    if (rc != 0) {
        if (rc == 1) {
            // End of ledger is reached
            return rc_SUCCESS;  // RETURN
        }

        return rc * 10 + rc_ITERATION_ERROR;  // RETURN
    }

    while ((rc = iterator->next()) == 0) {
        BSLS_ASSERT_SAFE(iterator->isValid());

        if (iterator->header().recordType() ==
            ClusterStateRecordType::e_SNAPSHOT) {
            latestIter->copy(*iterator);
        }
    }
    if (rc != 1) {
        return rc * 10 + rc_ITERATION_ERROR;  // RETURN
    }

    typedef bsl::unordered_map<bmqp_ctrlmsg::LeaderMessageSequence,
                               bmqp_ctrlmsg::ClusterMessage>
                  AdvisoriesMap;
    AdvisoriesMap advisories;
    do {
        BSLS_ASSERT_SAFE(latestIter->isValid());

        bmqp_ctrlmsg::ClusterMessage clusterMessage;
        rc = latestIter->loadClusterMessage(&clusterMessage);
        if (rc != 0) {
            // Error loading the cluster message
            return rc * 10 + rc_MESSAGE_LOAD_ERROR;  // RETURN
        }

        // Track if advisory, apply if commit
        typedef bmqp_ctrlmsg::ClusterMessageChoice MsgChoice;  // shortcut
        switch (clusterMessage.choice().selectionId()) {
        case MsgChoice::SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
            const bmqp_ctrlmsg::LeaderMessageSequence& lms =
                clusterMessage.choice()
                    .partitionPrimaryAdvisory()
                    .sequenceNumber();
            bsl::pair<AdvisoriesMap::iterator, bool> insertRc =
                advisories.insert(bsl::make_pair(lms, clusterMessage));
            if (!insertRc.second) {
                BALL_LOG_WARN << clusterData.identity().description()
                              << ": When loading from cluster state ledger, "
                              << "discovered records with duplicate LSN ["
                              << lms << "].  Older record type: "
                              << advisories.at(lms).choice().selectionId()
                              << "; newer record: " << clusterMessage;
            };
        } break;  // BREAK
        case MsgChoice::SELECTION_ID_LEADER_ADVISORY: {
            const bmqp_ctrlmsg::LeaderMessageSequence& lms =
                clusterMessage.choice().leaderAdvisory().sequenceNumber();
            bsl::pair<AdvisoriesMap::iterator, bool> insertRc =
                advisories.insert(bsl::make_pair(lms, clusterMessage));
            if (!insertRc.second) {
                BALL_LOG_WARN << clusterData.identity().description()
                              << ": When loading from cluster state ledger, "
                              << "discovered records with duplicate LSN ["
                              << lms << "].  Older record type: "
                              << advisories.at(lms).choice().selectionId()
                              << "; newer record type:"
                              << latestIter->header().recordType();
            };
        } break;  // BREAK
        case MsgChoice::SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
            const bmqp_ctrlmsg::LeaderMessageSequence& lms =
                clusterMessage.choice()
                    .queueAssignmentAdvisory()
                    .sequenceNumber();
            bsl::pair<AdvisoriesMap::iterator, bool> insertRc =
                advisories.insert(bsl::make_pair(lms, clusterMessage));
            if (!insertRc.second) {
                BALL_LOG_WARN << clusterData.identity().description()
                              << ": When loading from cluster state ledger, "
                              << "discovered records with duplicate LSN ["
                              << lms << "].  Older record type: "
                              << advisories.at(lms).choice().selectionId()
                              << "; newer record: " << clusterMessage;
            };
        } break;  // BREAK
        case MsgChoice::SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
            const bmqp_ctrlmsg::LeaderMessageSequence& lms =
                clusterMessage.choice()
                    .queueUnassignedAdvisory()
                    .sequenceNumber();
            bsl::pair<AdvisoriesMap::iterator, bool> insertRc =
                advisories.insert(bsl::make_pair(lms, clusterMessage));
            if (!insertRc.second) {
                BALL_LOG_WARN << clusterData.identity().description()
                              << ": When loading from cluster state ledger, "
                              << "discovered records with duplicate LSN ["
                              << lms << "].  Older record type: "
                              << advisories.at(lms).choice().selectionId()
                              << "; newer record: " << clusterMessage;
            };
        } break;  // BREAK
        case MsgChoice::SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
            const bmqp_ctrlmsg::LeaderMessageSequence& lms =
                clusterMessage.choice().queueUpdateAdvisory().sequenceNumber();
            bsl::pair<AdvisoriesMap::iterator, bool> insertRc =
                advisories.insert(bsl::make_pair(lms, clusterMessage));
            if (!insertRc.second) {
                BALL_LOG_WARN << clusterData.identity().description()
                              << ": When loading from cluster state ledger, "
                              << "discovered records with duplicate LSN ["
                              << lms << "].  Older record type: "
                              << advisories.at(lms).choice().selectionId()
                              << "; newer record: " << clusterMessage;
            };
        } break;
        case MsgChoice::SELECTION_ID_LEADER_ADVISORY_COMMIT: {
            const bmqp_ctrlmsg::LeaderMessageSequence& lmsCommitted =
                clusterMessage.choice()
                    .leaderAdvisoryCommit()
                    .sequenceNumberCommitted();

            AdvisoriesMap::const_iterator iter = advisories.find(lmsCommitted);
            if (iter == advisories.end()) {
                BALL_LOG_WARN << clusterData.identity().description()
                              << ": Recovered a commit in IncoreCSL for which"
                              << " a corresponding advisory was not found: "
                              << clusterMessage;
                break;  // BREAK
            }
            // Finally, the advisory is applied to the state
            const bmqp_ctrlmsg::ClusterMessage& advisory = iter->second;
            BALL_LOG_INFO << "#CSL_RECOVERY "
                          << clusterData.identity().description()
                          << ": Applying a commit recovered from IncoreCSL. "
                          << "Commit: "
                          << clusterMessage.choice().leaderAdvisoryCommit()
                          << ", advisory: " << advisory << ".";
            apply(state, advisory, clusterData);
            advisories.erase(iter);
        } break;  // BREAK
        case MsgChoice::SELECTION_ID_UNDEFINED:
        default: {
            MWCTSK_ALARMLOG_ALARM("CLUSTER")
                << clusterData.identity().description()
                << ": Unexpected clusterMessage: " << clusterMessage
                << MWCTSK_ALARMLOG_END;
        } break;  // BREAK
        }
    } while ((rc = latestIter->next()) == 0);

    if (rc != 1) {
        return rc * 10 + rc_ITERATION_ERROR;  // RETURN
    }
    return rc_SUCCESS;
}

void ClusterUtil::loadPartitionsInfo(
    bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>* out,
    const ClusterState&                              state)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    for (int pid = 0; pid < static_cast<int>(state.partitions().size());
         ++pid) {
        const ClusterStatePartitionInfo&   pinfo = state.partition(pid);
        bmqp_ctrlmsg::PartitionPrimaryInfo info;

        info.partitionId()    = pid;
        info.primaryNodeId()  = pinfo.primaryNode()
                                    ? pinfo.primaryNode()->nodeId()
                                    : mqbnet::Cluster::k_INVALID_NODE_ID;
        info.primaryLeaseId() = pinfo.primaryLeaseId();
        out->emplace_back(info);
    }

    BSLS_ASSERT_SAFE(out->size() == state.partitions().size());
}

void ClusterUtil::loadQueuesInfo(bsl::vector<bmqp_ctrlmsg::QueueInfo>* out,
                                 const ClusterState&                   state,
                                 bool includeAppIds)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    const DomainStates& domainsInfo = state.domainStates();
    for (DomainStatesCIter domCit = domainsInfo.cbegin();
         domCit != domainsInfo.cend();
         ++domCit) {
        const UriToQueueInfoMap& queuesInfoPerDomain =
            domCit->second->queuesInfo();
        for (UriToQueueInfoMapCIter qCit = queuesInfoPerDomain.cbegin();
             qCit != queuesInfoPerDomain.cend();
             ++qCit) {
            bmqp_ctrlmsg::QueueInfo queueInfo;
            queueInfo.uri()         = qCit->second->uri().asString();
            queueInfo.partitionId() = qCit->second->partitionId();

            BSLS_ASSERT_SAFE(!qCit->second->key().isNull());
            qCit->second->key().loadBinary(&queueInfo.key());

            if (includeAppIds) {
                for (AppIdInfosCIter appIdCit =
                         qCit->second->appIdInfos().cbegin();
                     appIdCit != qCit->second->appIdInfos().cend();
                     ++appIdCit) {
                    bmqp_ctrlmsg::AppIdInfo appIdInfo;
                    appIdInfo.appId() = appIdCit->first;
                    appIdCit->second.loadBinary(&appIdInfo.appKey());

                    queueInfo.appIds().push_back(appIdInfo);
                }
            }

            out->push_back(queueInfo);
        }
    }
}

void ClusterUtil::loadPeerNodes(bsl::vector<mqbnet::ClusterNode*>* out,
                                const ClusterData&                 clusterData)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    for (ClusterNodeSessionMapConstIter cit =
             clusterData.membership().clusterNodeSessionMap().begin();
         cit != clusterData.membership().clusterNodeSessionMap().end();
         ++cit) {
        if (cit->first != clusterData.membership().selfNode()) {
            out->push_back(cit->first);
        }
    }
}

int ClusterUtil::latestLedgerLSN(bmqp_ctrlmsg::LeaderMessageSequence* out,
                                 const ClusterStateLedger&            ledger,
                                 const ClusterData& clusterData)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // No error
        ,
        rc_LEDGER_NOT_OPENED = -1  // The ledger is not opened
    };

    if (!ledger.isOpen()) {
        BALL_LOG_ERROR << clusterData.identity().description()
                       << ": Leader sequence number cannot be found using "
                       << "ledger at this time as it is not yet opened.";

        return rc_LEDGER_NOT_OPENED;  // RETURN
    }

    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator> cslIter =
        ledger.getIterator();
    const mqbc::ClusterStateRecordHeader* header = 0;

    // Iterate to the last record in the ledger
    while (cslIter->next() == 0) {
        header = &cslIter->header();
        BSLS_ASSERT_SAFE(cslIter->isValid());
    }

    if (!header) {
        // The ledger is empty

        *out = bmqp_ctrlmsg::LeaderMessageSequence();
        return rc_SUCCESS;  // RETURN
    }

    out->electorTerm()    = header->electorTerm();
    out->sequenceNumber() = header->sequenceNumber();

    BALL_LOG_INFO << clusterData.identity().description()
                  << ": Retrieved latest leader sequence number from ledger to"
                  << " be " << *out;

    return rc_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
