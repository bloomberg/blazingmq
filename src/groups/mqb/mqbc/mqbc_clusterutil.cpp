// Copyright 2014-2025 Bloomberg Finance L.P.
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
#include "mqbc_clusterstateledger.h"
#include <ball_log.h>
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

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlb_print.h>
#include <bdlde_md5.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_limits.h>
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
const char k_CSL_FAILURE[]             = "CSL failure";

// TYPES
typedef ClusterUtil::AppInfos      AppInfos;
typedef ClusterUtil::AppInfosCIter AppInfosCIter;

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
        ClusterNodeSession* ns =
            clusterData.membership().getClusterNodeSession(
                proposedPrimaryNode);

        clusterState->setPartitionPrimary(info.partitionId(),
                                          info.primaryLeaseId(),
                                          ns);
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

        // CSL commit
        clusterState->assignQueue(queueInfo);
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
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << clusterData.identity().description()
                    << ": Received QueueUpdateAdvisory for a domain and queue "
                    << "that were not found: " << queueUpdate << "]"
                    << BMQTSK_ALARMLOG_END;
                return;  // RETURN
            }

            ClusterState::UriToQueueInfoMapCIter cit =
                domCiter->second->queuesInfo().find(uri);
            if (cit == domCiter->second->queuesInfo().end()) {
                // First time hearing about this queue - should not occur for
                // an update advisory
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << clusterData.identity().description()
                    << ": Received QueueUpdateAdvisory for a queue that was "
                    << "not found: " << queueUpdate << "]"
                    << BMQTSK_ALARMLOG_END;
                return;  // RETURN
            }

            if (cit->second->partitionId() != partitionId) {
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << clusterData.identity().description()
                    << ": Received QueueUpdateAdvisory for known queue [uri: "
                    << uri << "] with a mismatched partitionId "
                    << "[expected: " << cit->second->partitionId()
                    << ", received: " << partitionId << "]: " << queueUpdate
                    << BMQTSK_ALARMLOG_END;
                return;  // RETURN
            }

            if (cit->second->key() != queueKey) {
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << clusterData.identity().description()
                    << ": Received QueueUpdateAdvisory for known queue [uri: "
                    << uri << "] with a mismatched queueKey "
                    << "[expected: " << cit->second->key()
                    << ", received: " << queueKey << "]: " << queueUpdate
                    << BMQTSK_ALARMLOG_END;
                return;  // RETURN
            }
        }
        else {
            // This update is for an entire domain, instead of any individual
            // queue.
            BSLS_ASSERT_SAFE(queueKey == mqbu::StorageKey::k_NULL_KEY);
            BSLS_ASSERT_SAFE(partitionId ==
                             mqbi::Storage::k_INVALID_PARTITION_ID);
        }

        const int rc = clusterState->updateQueue(queueUpdate);
        if (rc != 0) {
            BALL_LOG_ERROR << clusterData.identity().description()
                           << ": Received QueueUpdateAdvisory for uri: [ "
                           << uri << "], domain: [" << queueUpdate.domain()
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
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData.cluster().inDispatcherThread());
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
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << clusterData.identity().description()
            << ": Unknown cluster masterAssignmentAlgorithm '"
            << assignmentAlgo
            << "', defaulting to 'leaderIsMasterAll' algorithm."
            << BMQTSK_ALARMLOG_END;

        mqbnet::ClusterNode* selfNode = clusterData.membership().selfNode();
        numNewPartitions->insert(bsl::make_pair(
            clusterData.membership().getClusterNodeSession(selfNode),
            numPartitionsToAssign));

        break;  // BREAK
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

bool populateQueueUpdate(bmqp_ctrlmsg::QueueUpdateAdvisory* queueAdvisory,
                         const bsl::vector<bsl::string>&    added,
                         const bsl::vector<bsl::string>&    removed,
                         const ClusterStateQueueInfo&       from,
                         bslma::Allocator*                  allocator)
{
    bmqp_ctrlmsg::QueueInfoUpdate queueUpdate(allocator);

    queueUpdate.uri()         = from.uri().asString();
    queueUpdate.partitionId() = from.partitionId();
    from.key().loadBinary((&queueUpdate.key()));
    queueUpdate.domain() = from.uri().qualifiedDomain();

    // TODO: optimize to avoid iterating all Apps
    // TODO: If a queue can register its own set of (Dynamic) AppId,
    //       the only meaning of this loop is to build 'appKeys' to
    //       avoid key collision.
    bsl::unordered_set<mqbu::StorageKey> appKeys;
    const AppInfos&                      appInfos = from.appInfos();

    for (AppInfosCIter appInfoCit = appInfos.cbegin();
         appInfoCit != appInfos.cend();
         ++appInfoCit) {
        appKeys.insert(appInfoCit->second);
    }

    for (bsl::vector<bsl::string>::const_iterator cit = added.begin();
         cit != added.end();
         ++cit) {
        const bsl::string& appId = *cit;

        AppInfos::const_iterator existing = from.appInfos().find(appId);

        if (existing == from.appInfos().end()) {
            // Populate AppIdInfo
            bmqp_ctrlmsg::AppIdInfo appIdInfo;
            appIdInfo.appId() = appId;
            mqbu::StorageKey appKey;
            mqbs::StorageUtil::generateStorageKey(&appKey, &appKeys, appId);
            appKey.loadBinary(&appIdInfo.appKey());

            queueUpdate.addedAppIds().push_back(appIdInfo);
        }
    }

    for (bsl::vector<bsl::string>::const_iterator cit = removed.begin();
         cit != removed.end();
         ++cit) {
        const bsl::string&       appId    = *cit;
        AppInfos::const_iterator existing = from.appInfos().find(appId);

        if (existing != from.appInfos().end()) {
            // Populate AppIdInfo
            bmqp_ctrlmsg::AppIdInfo appIdInfo;
            appIdInfo.appId() = appId;
            existing->second.loadBinary(&appIdInfo.appKey());

            // do not care about 'appIdInfo.dynamicExpirationSec()'
            queueUpdate.removedAppIds().push_back(appIdInfo);
        }
    }

    if (queueUpdate.removedAppIds().size() == removed.size() &&
        queueUpdate.addedAppIds().size() == added.size()) {
        queueAdvisory->queueUpdates().push_back(queueUpdate);

        return true;
    }

    return false;
}

}  // close anonymous namespace

// ------------------
// struct ClusterUtil
// ------------------

void ClusterUtil::setPendingUnassignment(const ClusterState* clusterState,
                                         const bmqt::Uri&    uri)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(uri.isCanonical());

    mqbc::ClusterUtil::DomainStatesCIter citer =
        clusterState->domainStates().find(uri.qualifiedDomain());
    if (citer != clusterState->domainStates().cend()) {
        UriToQueueInfoMapIter qiter = citer->second->queuesInfo().find(uri);
        if (qiter != citer->second->queuesInfo().cend()) {
            qiter->second->setState(
                ClusterStateQueueInfo::State::k_UNASSIGNING);
        }
    }
}

void ClusterUtil::extractMessage(bmqp_ctrlmsg::ControlMessage* message,
                                 const bdlbb::Blob&            eventBlob,
                                 bslma::Allocator*             allocator)
{
    // Extract event header
    bmqu::BlobObjectProxy<bmqp::EventHeader> eventHeader(
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
    bmqu::MemOutStream errorDescription;
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
    const ClusterData&                               clusterData)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData.cluster().inDispatcherThread());
    BSLS_ASSERT_SAFE(partitions && partitions->empty());
    BSLS_ASSERT_SAFE(mqbnet::ElectorState::e_LEADER ==
                     clusterData.electorInfo().electorState());
    if (!clusterData.clusterConfig().clusterAttributes().isFSMWorkflow()) {
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

            BALL_LOG_INFO << clusterData.identity().description()
                          << ": Partition [" << pinfo.partitionId()
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

    if (domainName.starts_with(latencyMonitorDomain)) {
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
    BSLS_ASSERT_SAFE(clusterData->cluster().inDispatcherThread());
    BSLS_ASSERT_SAFE(storageManager);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    if (primary) {
        BSLS_ASSERT_SAFE(status != bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED);
    }
    else {
        BSLS_ASSERT_SAFE(status == bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED);
    }

    // Validation
    if (primary && oldPrimary && primary->nodeId() == oldPrimary->nodeId()) {
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
    bslma::Allocator*                   allocator)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster->inDispatcherThread());
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

    assignQueue(clusterState,
                clusterData,
                ledger,
                cluster,
                uri,
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
    const mqbconfm::QueueMode&             config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(advisory);
    BSLS_ASSERT_SAFE(key);
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(uri.isCanonical());
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

    // Generate appIds and appKeys
    populateAppInfos(&queueInfo.appIds(), config);

    BALL_LOG_INFO << clusterData->identity().description()
                  << ": Populated QueueAssignmentAdvisory: " << *advisory;
}

void ClusterUtil::populateQueueUnAssignmentAdvisory(
    bmqp_ctrlmsg::QueueUnAssignmentAdvisory* advisory,
    ClusterData*                             clusterData,
    const bmqt::Uri&                         uri,
    const mqbu::StorageKey&                  key,
    int                                      partitionId,
    const ClusterState&                      clusterState)
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
                  << ": Populated QueueUnAssignmentAdvisory: " << *advisory;
}

bool ClusterUtil::assignQueue(ClusterState*         clusterState,
                              ClusterData*          clusterData,
                              ClusterStateLedger*   ledger,
                              const mqbi::Cluster*  cluster,
                              const bmqt::Uri&      uri,
                              bslma::Allocator*     allocator,
                              bmqp_ctrlmsg::Status* status)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster->inDispatcherThread());
    BSLS_ASSERT_SAFE(!cluster->isRemote());
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(clusterData->electorInfo().isSelfActiveLeader());
    BSLS_ASSERT_SAFE(ledger && ledger->isOpen());
    BSLS_ASSERT_SAFE(uri.isCanonical());
    BSLS_ASSERT_SAFE(allocator);
    BSLS_ASSERT_SAFE(status);

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

        status->category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status->code()     = mqbi::ClusterErrorCode::e_STOPPING;
        status->message()  = k_SELF_NODE_IS_STOPPING;

        // Transient failure, can continue
        return true;  // RETURN
    }

    ClusterState::DomainStates& domainStates = clusterState->domainStates();
    DomainStatesIter domIt = domainStates.find(uri.qualifiedDomain());

    UriToQueueInfoMapIter queueIt;
    if (domIt == domainStates.end()) {
        ClusterState::DomainStateSp domainState;
        domainState.createInplace(allocator, allocator);
        domIt = domainStates.emplace(uri.qualifiedDomain(), domainState).first;

        queueIt = domIt->second->queuesInfo().end();
    }
    else {
        queueIt = domIt->second->queuesInfo().find(uri);
    }

    // There is nothing we can do if we don't have a built logical domain.
    if (domIt->second->domain() == 0) {
        BSLS_ASSERT_SAFE(clusterData->domainFactory());
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

            status->category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
            status->code()     = mqbi::ClusterErrorCode::e_UNKNOWN;
            status->message()  = k_DOMAIN_CREATION_FAILURE;

            // Permanent failure, cannot continue
            return false;  // RETURN
        }
    }

    ClusterStateQueueInfo::State::Enum previousState =
        ClusterStateQueueInfo::State::k_NONE;
    if (queueIt != domIt->second->queuesInfo().end()) {
        // If we have a queue state in the map, we can extract this state.
        // For k_ASSIGNED or k_ASSIGNING states we don't need to do anything
        // here and can return early.
        // If the state is k_UNASSIGNING, we proceed with assigning.
        previousState = queueIt->second->state();

        if (previousState == ClusterStateQueueInfo::State::k_ASSIGNING) {
            BALL_LOG_INFO << cluster->description() << "queueAssignment of '"
                          << uri << "' is already pending.";
            return true;  // RETURN
        }

        if (previousState == ClusterStateQueueInfo::State::k_ASSIGNED) {
            BALL_LOG_INFO << cluster->description() << "queueAssignment of '"
                          << uri << "' is already done.";
            return true;  // RETURN
        }
    }

    struct local {
        static void panic(mqbi::Domain* domain)
        {
            BMQTSK_ALARMLOG_PANIC("DOMAIN_QUEUE_LIMIT_FULL")
                << "domain '" << "bmq://" << domain->name()
                << "' has reached the maximum number of queues (limit: "
                << domain->config().maxQueues() << ")." << BMQTSK_ALARMLOG_END;
        }

        static void alarm(mqbi::Domain* domain, int queues)
        {
            BMQTSK_ALARMLOG_ALARM("DOMAIN_QUEUE_LIMIT_HIGH_WATERMARK")
                << "domain '" << "bmq://" << domain->name()
                << "' has reached the " << (k_MAX_QUEUES_HIGH_WATERMARK * 100)
                << "% watermark limit for the number of queues "
                   "(current: "
                << queues << ", limit: " << domain->config().maxQueues()
                << ")." << BMQTSK_ALARMLOG_END;
        }
    };

    if (queueIt == domIt->second->queuesInfo().end()) {
        BSLS_ASSERT_SAFE(previousState ==
                         ClusterStateQueueInfo::State::k_NONE);

        // Need to check if we have capacity before we allocate resources
        // for this new queue.  The current number of registered queues is:
        // num(assigned) + num(assigning) + num(unassigning).
        const int registeredQueues = static_cast<int>(
            domIt->second->queuesInfo().size());
        const int maxQueues = domIt->second->domain()->config().maxQueues();
        if (maxQueues != 0) {
            const int requestedQueues = registeredQueues + 1;
            if (requestedQueues > maxQueues) {
                local::panic(domIt->second->domain());
            }
            else {
                const int watermark = static_cast<int>(
                    maxQueues * k_MAX_QUEUES_HIGH_WATERMARK);
                if (registeredQueues < watermark &&
                    requestedQueues >= watermark) {
                    local::alarm(domIt->second->domain(), requestedQueues);
                }
            }

            if (requestedQueues > maxQueues) {
                status->category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
                status->code()     = mqbi::ClusterErrorCode::e_LIMIT;
                status->message()  = k_MAXIMUM_NUMBER_OF_QUEUES_REACHED;

                // Permanent failure, cannot continue
                return false;  // RETURN
            }
        }

        // We have capacity and can add this queue to the collection.
        // The queue will be in k_ASSIGNING state until we commit queue
        // assignment advisory.
        QueueInfoSp queueInfo;

        queueInfo.createInplace(allocator, uri, allocator);

        queueIt = domIt->second->queuesInfo().emplace(uri, queueInfo).first;
    }
    else {
        // Note that we already have `queueIt` and allocated resources for this
        // queue.  No need to allocate new QueueInfo and check capacity.
        BSLS_ASSERT_SAFE(previousState ==
                         ClusterStateQueueInfo::State::k_UNASSIGNING);
    }

    // Set the queue as assigning (no longer pending unassignment)
    queueIt->second->setState(ClusterStateQueueInfo::State::k_ASSIGNING);

    BALL_LOG_INFO << "Cluster [" << cluster->description()
                  << "]: Transition: " << previousState << " -> "
                  << ClusterStateQueueInfo::State::k_ASSIGNING << " for ["
                  << uri << "].";

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
                                    domIt->second->domain()->config().mode());

    // 'ClusterQueueHelper::onQueueAssigned' (the 'onQueueAssigned' observer
    // callback) will insert the key to 'ClusterState::queueKeys'.

    clusterState->queueKeys().erase(key);

    if (!cluster->isCSLModeEnabled()) {
        // Broadcast 'queueAssignmentAdvisory' to all followers

        // NOTE: We must broadcast this control message before applying to CSL,
        // because if CSL is running in eventual consistency it will
        // immediately apply a commit with a higher seqeuence number than the
        // QueueAssignmentAdvisory.  If we ever receive the commit before the
        // QAA, we will alarm due to out-of-sequence advisory.
        clusterData->messageTransmitter().broadcastMessage(controlMsg);

        BSLS_ASSERT_SAFE(queueAdvisory.queues().size() == 1);
    }

    // Apply 'queueAssignmentAdvisory' to CSL
    BALL_LOG_INFO << clusterData->identity().description()
                  << ": 'QueueAssignmentAdvisory' will be applied to "
                  << " cluster state ledger: " << queueAdvisory;

    const int rc = ledger->apply(queueAdvisory);

    if (rc == 0) {
        return true;  // RETURN
    }
    else {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to apply queue assignment advisory: "
                       << queueAdvisory << ", rc: " << rc;

        status->category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status->code()     = mqbi::ClusterErrorCode::e_CSL_FAILURE;
        status->message()  = k_CSL_FAILURE;

        // Permanent failure, cannot continue
        return false;  // RETURN
    }
}

void ClusterUtil::registerQueueInfo(ClusterState*        clusterState,
                                    const mqbi::Cluster* cluster,
                                    const bmqp_ctrlmsg::QueueInfo& advisory,
                                    bool                           forceUpdate)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster);
    BSLS_ASSERT_SAFE(cluster->inDispatcherThread());
    BSLS_ASSERT_SAFE(!cluster->isRemote());
    BSLS_ASSERT_SAFE(clusterState);

    const bmqt::Uri& uri         = advisory.uri();
    const int        partitionId = advisory.partitionId();

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

    const mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                                    advisory.key().data());

    DomainStatesIter domIt = clusterState->domainStates().find(
        uri.qualifiedDomain());
    if (domIt != clusterState->domainStates().end()) {
        UriToQueueInfoMapCIter cit = domIt->second->queuesInfo().find(uri);
        if (cit != domIt->second->queuesInfo().cend()) {
            // Queue is assigned.
            QueueInfoSp qs = cit->second;
            BSLS_ASSERT_SAFE(qs);
            BSLS_ASSERT_SAFE(qs->uri() == uri);

            if ((qs->equal(advisory))) {
                // All good.. nothing to update.
                return;  // RETURN
            }

            bmqu::Printer<AppInfos> stateAppInfos(&qs->appInfos());
            bmqu::Printer<bsl::vector<bmqp_ctrlmsg::AppIdInfo> >
                storageAppInfos(&advisory.appIds());

            // PartitionId and/or QueueKey and/or AppInfos mismatch.
            if (!forceUpdate) {
                BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                    << cluster->description() << ": For queue [ " << uri
                    << "], different partitionId/queueKey/appInfos in "
                    << "cluster state and storage.  "
                    << "PartitionId/QueueKey/AppInfos in cluster state ["
                    << qs->partitionId() << "], [" << qs->key() << "], ["
                    << stateAppInfos
                    << "].  PartitionId/QueueKey/AppInfos in storage ["
                    << partitionId << "], [" << queueKey << "], ["
                    << storageAppInfos << "]." << BMQTSK_ALARMLOG_END;

                if (!cluster->isFSMWorkflow()) {
                    // TODO (FSM); remove this code after switching to FSM

                    // Cache and wait for primary to unregister the queue from
                    // 'partitionId'

                    clusterState->cacheDoubleAssignment(uri, partitionId);
                }

                return;  // RETURN
            }

            BALL_LOG_WARN << cluster->description() << ": For queue [" << uri
                          << "], force-updating "
                          << "partitionId/queueKey/appInfos from ["
                          << qs->partitionId() << "], [" << qs->key() << "], ["
                          << stateAppInfos << "] to [" << partitionId << "], ["
                          << queueKey << "], [" << storageAppInfos << "].";

            clusterState->queueKeys().erase(qs->key());

            clusterState->assignQueue(advisory);

            BALL_LOG_INFO << cluster->description() << ": Queue assigned: "
                          << "[uri: " << uri << ", queueKey: " << queueKey
                          << ", partitionId: " << partitionId;

            BSLS_ASSERT_SAFE(1 == clusterState->queueKeys().count(queueKey));

            return;  // RETURN
        }
    }

    // Queue is not known, so add it.
    clusterState->assignQueue(advisory);

    bmqu::Printer<bsl::vector<bmqp_ctrlmsg::AppIdInfo> > printer(
        &advisory.appIds());
    BALL_LOG_INFO << cluster->description()
                  << ": Queue assigned: [uri: " << uri
                  << ", queueKey: " << queueKey
                  << ", partitionId: " << partitionId
                  << ", appInfos: " << printer << "]";
}

void ClusterUtil::populateAppInfos(
    bsl::vector<bmqp_ctrlmsg::AppIdInfo>* appInfos,
    const mqbconfm::QueueMode&            domainConfig)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(appInfos && appInfos->empty());

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
            // This is the only place generating keys upon queue assignment
            // for both CSL (FSM) and non-CSL (non-FSM).  The latter used to
            // generate keys in 'StorageUtil::registerQueue'.

            appKey.loadBinary(&appIdInfo.appKey());

            appInfos->push_back(appIdInfo);
        }
    }
    else {
        bmqp_ctrlmsg::AppIdInfo appIdInfo;
        appIdInfo.appId() = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
        mqbi::QueueEngine::k_DEFAULT_APP_KEY.loadBinary(&appIdInfo.appKey());

        appInfos->push_back(appIdInfo);
    }
}

mqbi::ClusterErrorCode::Enum
ClusterUtil::updateAppIds(ClusterData*                    clusterData,
                          ClusterStateLedger*             ledger,
                          ClusterState&                   clusterState,
                          const bsl::vector<bsl::string>& added,
                          const bsl::vector<bsl::string>& removed,
                          const bsl::string&              domainName,
                          const bsl::string&              uri,
                          bslma::Allocator*               allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(ledger && ledger->isOpen());
    BSLS_ASSERT_SAFE(!domainName.empty());
    BSLS_ASSERT_SAFE(allocator);

    bmqu::Printer<bsl::vector<bsl::string> > printAdded(&added);
    bmqu::Printer<bsl::vector<bsl::string> > printRemoved(&removed);

    if (mqbnet::ElectorState::e_LEADER !=
        clusterData->electorInfo().electorState()) {
        BALL_LOG_WARN << clusterData->identity().description()
                      << ": Not unregistering appIds " << printRemoved
                      << " and registering appIds " << printAdded
                      << "] for domain '" << domainName
                      << "'. Self is not leader.";
        return mqbi::ClusterErrorCode::e_NOT_LEADER;  // RETURN
    }

    if (ElectorInfoLeaderStatus::e_ACTIVE !=
        clusterData->electorInfo().leaderStatus()) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to unregister appIds " << printRemoved
                       << " and to register appIds " << printAdded
                       << "] for domain '" << domainName
                       << "'. Self is leader but is not active.";
        return mqbi::ClusterErrorCode::e_NOT_LEADER;  // RETURN
    }

    if (clusterData->membership().selfNodeStatus() ==
        bmqp_ctrlmsg::NodeStatus::E_STOPPING) {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << ": Failed to unregister appIds " << printRemoved
                       << " and to register appIds " << printAdded
                       << "] for domain '" << domainName
                       << "'. Self is active leader but is stopping.";
        return mqbi::ClusterErrorCode::e_STOPPING;  // RETURN
    }

    // Populate 'queueUpdateAdvisory'
    bdlma::LocalSequentialAllocator<1024> localAllocator(allocator);
    bmqp_ctrlmsg::QueueUpdateAdvisory     queueAdvisory(&localAllocator);
    clusterData->electorInfo().nextLeaderMessageSequence(
        &queueAdvisory.sequenceNumber());

    DomainStatesCIter domCit = clusterState.domainStates().find(domainName);

    if (domCit == clusterState.domainStates().cend() ||
        domCit->second->queuesInfo().empty()) {
        // We will reach this scenario if we attempt to register an appId for
        // a domain with no opened queues.  This is still a valid scenario, so
        // we set a *NULL* queueUri/queueKey/partitionId for the
        // QueueUpdateAdvisory to indicate that we are updating appIds for the
        // entire domain.

        bmqp_ctrlmsg::QueueInfoUpdate queueUpdate;
        queueUpdate.uri()         = "";
        queueUpdate.partitionId() = mqbi::Storage::k_INVALID_PARTITION_ID;
        mqbu::StorageKey::k_NULL_KEY.loadBinary((&queueUpdate.key()));
        queueUpdate.domain() = domainName;

        // Populate AppInfos
        for (bsl::vector<bsl::string>::const_iterator cit = added.begin();
             cit != added.end();
             ++cit) {
            bmqp_ctrlmsg::AppIdInfo appIdInfo;
            appIdInfo.appId() = *cit;
            mqbu::StorageKey::k_NULL_KEY.loadBinary(&appIdInfo.appKey());

            queueUpdate.addedAppIds().push_back(appIdInfo);
        }
        for (bsl::vector<bsl::string>::const_iterator cit = removed.begin();
             cit != removed.end();
             ++cit) {
            bmqp_ctrlmsg::AppIdInfo appIdInfo;
            appIdInfo.appId() = *cit;
            mqbu::StorageKey::k_NULL_KEY.loadBinary(&appIdInfo.appKey());

            queueUpdate.removedAppIds().push_back(appIdInfo);
        }

        queueAdvisory.queueUpdates().push_back(queueUpdate);
    }
    else if (uri.empty()) {
        for (UriToQueueInfoMapCIter qinfoCit =
                 domCit->second->queuesInfo().cbegin();
             qinfoCit != domCit->second->queuesInfo().cend();
             ++qinfoCit) {
            BSLS_ASSERT_SAFE(qinfoCit->second->uri().qualifiedDomain() ==
                             domainName);

            const bool success = populateQueueUpdate(&queueAdvisory,
                                                     added,
                                                     removed,
                                                     *qinfoCit->second,
                                                     allocator);

            if (!success) {
                BALL_LOG_ERROR << "Failed to unregister appIds "
                               << printRemoved << " and to register appIds "
                               << printAdded << " for '" << uri
                               << "'.  Current state: " << *qinfoCit->second;

                return mqbi::ClusterErrorCode::e_UNKNOWN;  // RETURN
            }
        }
    }
    else {
        UriToQueueInfoMapCIter qinfoCit = domCit->second->queuesInfo().find(
            uri);

        if (qinfoCit == domCit->second->queuesInfo().cend()) {
            BALL_LOG_ERROR << ": Failed to unregister appIds " << printRemoved
                           << " and to register appIds " << printAdded
                           << "]. Queue '" << uri << "' does not exist.";

            return mqbi::ClusterErrorCode::e_UNKNOWN_QUEUE;  // RETURN
        }

        const bool success = populateQueueUpdate(&queueAdvisory,
                                                 added,
                                                 removed,
                                                 *qinfoCit->second,
                                                 allocator);
        if (!success) {
            BALL_LOG_ERROR << "Failed to unregister appIds " << printRemoved
                           << " and to register appIds " << printAdded
                           << " for '" << uri
                           << "'.  Current state: " << *qinfoCit->second;

            return mqbi::ClusterErrorCode::e_UNKNOWN;  // RETURN
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

        return mqbi::ClusterErrorCode::e_CSL_FAILURE;
    }
    else {
        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << "Advisory applied: unregister appIds "
                                   << printRemoved << " and register appIds "
                                   << printAdded << " for ";
            if (uri.empty()) {
                BALL_LOG_OUTPUT_STREAM << "domain = [" << domainName << "]";
            }
            else {
                BALL_LOG_OUTPUT_STREAM << "uri = [" << uri << "]";
            }
        }

        return mqbi::ClusterErrorCode::e_OK;
    }
}

void ClusterUtil::sendClusterState(
    ClusterData*         clusterData,
    ClusterStateLedger*  ledger,
    const ClusterState&  clusterState,
    bool                 sendPartitionPrimaryInfo,
    bool                 sendQueuesInfo,
    bool                 trustCSL,
    bslma::Allocator*    allocator,
    mqbnet::ClusterNode* node,
    const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& newlyAssigned)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(clusterData->cluster().inDispatcherThread());
    BSLS_ASSERT_SAFE(mqbnet::ElectorState::e_LEADER ==
                     clusterData->electorInfo().electorState());
    BSLS_ASSERT_SAFE(ledger && ledger->isOpen());
    if (!sendPartitionPrimaryInfo) {
        BSLS_ASSERT_SAFE(newlyAssigned.empty());
    }
    else if (!newlyAssigned.empty()) {
        BSLS_ASSERT_SAFE(newlyAssigned.size() ==
                         clusterState.partitions().size());
    }

    if (clusterData->clusterConfig().clusterAttributes().isFSMWorkflow()) {
        // In FSM mode, the *only* possible caller of this method is
        // `onNodeUnavailable()`.  In all other cases, the Cluster FSM is
        // responsible for sending the cluster state updates to followers.
        BSLS_ASSERT_SAFE(sendPartitionPrimaryInfo && !sendQueuesInfo);

        BSLS_ASSERT_SAFE(trustCSL);
    }

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
        clusterData->membership().selfNodeStatus()) {
        // No need to send cluster state since self is stopping.  After self
        // has stopped, a new leader will be elected whom will take care of
        // this duty,
        BALL_LOG_INFO << clusterData->identity().description()
                      << ": Not doing 'sendClusterState' to "
                      << (node ? node->nodeDescription()
                               : "all follower nodes")
                      << " since self is stopping.";

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

        if (trustCSL) {
            // We construct a "merged state" of current cluster state and
            // information from uncommitted CSL advisories, before sending over
            // to follower(s).
            //
            // NOTE: This code path is *only* reachable in non-FSM mode.
            ClusterState tempState(
                &clusterData->cluster(),
                clusterData->clusterConfig().partitionConfig().numPartitions(),
                true,  // isTemporary
                allocator);
            const int rc = load(&tempState,
                                ledger->getIterator().get(),
                                *clusterData,
                                allocator);
            if (rc != 0) {
                BALL_LOG_ERROR
                    << clusterData->identity().description()
                    << ": Failed to load CSL content to temp cluster "
                    << "state as part of 'sendClusterState', rc: " << rc;

                return;  // RETURN
            }

            ClusterStateLedger::ClusterMessageCRefList uncommittedAdvisories;
            ledger->uncommittedAdvisories(&uncommittedAdvisories);
            for (ClusterStateLedger::ClusterMessageCRefList::const_iterator
                     cit = uncommittedAdvisories.begin();
                 cit != uncommittedAdvisories.end();
                 ++cit) {
                apply(&tempState, *cit, *clusterData);
            }

            if (!newlyAssigned.empty()) {
                advisory.partitions() = newlyAssigned;

                for (bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>::iterator
                         pit = advisory.partitions().begin();
                     pit != advisory.partitions().end();
                     ++pit) {
                    const int          pid = pit->partitionId();
                    const unsigned int leaseIdInCSL =
                        tempState.partition(pit->partitionId())
                            .primaryLeaseId();
                    if (leaseIdInCSL == pit->primaryLeaseId() &&
                        tempState.partition(pit->partitionId())
                                .primaryNodeId() == pit->primaryNodeId()) {
                        // For this partition, existing primary is preserved.
                        // All good.
                        continue;  // CONTINUE
                    }
                    if (leaseIdInCSL >= pit->primaryLeaseId()) {
                        BALL_LOG_WARN
                            << clusterData->identity().description()
                            << " Partition [" << pid
                            << "]: Self leader sees that CSL has recorded a "
                               "leaseId of "
                            << leaseIdInCSL
                            << ", while self was about to assign a leaseId of "
                            << pit->primaryLeaseId() << ".  Bumping up to "
                            << leaseIdInCSL + 1
                            << " to avoid potential conflict.";
                        pit->primaryLeaseId() = leaseIdInCSL + 1;
                    }
                }
            }
            else {
                loadPartitionsInfo(&advisory.partitions(), tempState);
            }
            loadQueuesInfo(&advisory.queues(), tempState);

            BALL_LOG_INFO_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM
                    << clusterData->identity().description()
                    << ": As part of 'sendClusterState', loaded merged state "
                       "of CSL content and uncommitted advisories to temp "
                       "cluster state.";
                if (!newlyAssigned.empty()) {
                    BALL_LOG_OUTPUT_STREAM << "  In addition, assigned new "
                                              "partition primaries.";
                }
                BALL_LOG_OUTPUT_STREAM << "  Ready to send now.";
            }
        }
        else {
            advisory.partitions() = newlyAssigned;
            loadQueuesInfo(&advisory.queues(), clusterState);
        }
    }
    else if (sendPartitionPrimaryInfo) {
        bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory =
            clusterMessage.choice().makePartitionPrimaryAdvisory();

        clusterData->electorInfo().nextLeaderMessageSequence(
            &advisory.sequenceNumber());

        advisory.partitions() = newlyAssigned;
    }
    else {
        BSLS_ASSERT_SAFE(sendQueuesInfo);

        bmqp_ctrlmsg::QueueAssignmentAdvisory& advisory =
            clusterMessage.choice().makeQueueAssignmentAdvisory();

        clusterData->electorInfo().nextLeaderMessageSequence(
            &advisory.sequenceNumber());

        loadQueuesInfo(&advisory.queues(), clusterState);
    }

    // Need to send the control message for the old brokers to process
    // LeaderAdvisory.
    if (!clusterData->cluster().isCSLModeEnabled()) {
        if (node) {
            clusterData->messageTransmitter().sendMessage(controlMessage,
                                                          node);
        }
        else {
            clusterData->messageTransmitter().broadcastMessage(controlMessage);
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

    bmqu::MemOutStream endpoint(allocator);
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
    case MsgChoice::SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
        const bmqp_ctrlmsg::QueueUnAssignmentAdvisory& queueAdvisory =
            clusterMessage.choice().queueUnAssignmentAdvisory();
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
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << clusterData.identity().description()
            << ": Unexpected clusterMessage: " << clusterMessage
            << BMQTSK_ALARMLOG_END;
    } break;  // BREAK
    }
}

int ClusterUtil::validateState(bsl::ostream&       errorDescription,
                               const ClusterState& state,
                               const ClusterState& reference)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(state.partitions().size() ==
                     reference.partitions().size());

    bmqu::MemOutStream out;
    const int          level = 0;

    // Validate partition information
    bsl::vector<ClusterStatePartitionInfo> incorrectPartitions;
    for (size_t i = 0; i < state.partitions().size(); ++i) {
        const ClusterStatePartitionInfo& stateInfo = state.partitions()[i];
        const int                        pid       = i;
        BSLS_ASSERT_SAFE(stateInfo.partitionId() == pid);

        const ClusterStatePartitionInfo& referenceInfo =
            reference.partitions()[i];
        BSLS_ASSERT_SAFE(referenceInfo.partitionId() == pid);
        if (stateInfo.primaryLeaseId() != referenceInfo.primaryLeaseId()) {
            // Partition information mismatch.  Note that we don't compare
            // primaryNodeIds here because 'state' is initialized with cluster
            // state ledger's contents and will likely have a valid
            // primaryNodeId (from previous run of the broker).  However,
            // 'reference' state is the true copy, currently owned by this
            // instance of the broker, and will likely have an invalid
            // primaryNodeId, specially if a primary has not yet been assigned
            // in the startup sequence.  If if a primary has been assigned, its
            // nodeId may be different one.
            incorrectPartitions.push_back(stateInfo);
        }
    }

    if (!incorrectPartitions.empty()) {
        bdlb::Print::newlineAndIndent(out, level);
        out << "---------------------------";
        bdlb::Print::newlineAndIndent(out, level);
        out << "Incorrect Partition Infos :";
        bdlb::Print::newlineAndIndent(out, level);
        out << "---------------------------";
        for (bsl::vector<ClusterStatePartitionInfo>::const_iterator citer =
                 incorrectPartitions.cbegin();
             citer != incorrectPartitions.cend();
             ++citer) {
            bdlb::Print::newlineAndIndent(out, level + 1);
            out << "Partition [" << citer->partitionId()
                << "]:  primaryLeaseId: " << citer->primaryLeaseId()
                << ", primaryNodeId: " << citer->primaryNodeId();
        }

        bdlb::Print::newlineAndIndent(out, level);
        out << "--------------------------------";
        bdlb::Print::newlineAndIndent(out, level);
        out << "Partition Infos In Cluster State :";
        bdlb::Print::newlineAndIndent(out, level);
        out << "--------------------------------";
        for (size_t i = 0; i < state.partitions().size(); ++i) {
            const ClusterStatePartitionInfo& referenceInfo =
                reference.partitions()[i];
            const int pid = i;
            BSLS_ASSERT_SAFE(referenceInfo.partitionId() == pid);
            bdlb::Print::newlineAndIndent(out, level + 1);
            out << "Partition [" << pid
                << "]:  primaryLeaseId: " << referenceInfo.primaryLeaseId()
                << ", primaryNodeId: " << referenceInfo.primaryNodeId();
        }
    }

    // Check incorrect or extra queues
    bsl::vector<bsl::pair<bsl::shared_ptr<ClusterStateQueueInfo>,
                          bsl::shared_ptr<ClusterStateQueueInfo> > >
                                                         incorrectQueues;
    bsl::vector<bsl::shared_ptr<ClusterStateQueueInfo> > extraQueues;
    for (ClusterState::DomainStatesCIter domCit = state.domainStates().begin();
         domCit != state.domainStates().end();
         ++domCit) {
        const bsl::string& domainName = domCit->first;

        ClusterState::DomainStatesCIter refDomCit =
            reference.domainStates().find(domainName);
        if (refDomCit == reference.domainStates().cend()) {
            // Entire domain is extra
            for (ClusterState::UriToQueueInfoMapCIter citer =
                     domCit->second->queuesInfo().cbegin();
                 citer != domCit->second->queuesInfo().cend();
                 ++citer) {
                extraQueues.push_back(citer->second);
            }
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
                // Extra queue
                extraQueues.push_back(citer->second);
            }
            else {
                const bsl::shared_ptr<ClusterStateQueueInfo>& info =
                    citer->second;
                const bsl::shared_ptr<ClusterStateQueueInfo>& referenceInfo =
                    refCiter->second;
                if (!info->isEquivalent(*referenceInfo)) {
                    // Incorrect queue information
                    incorrectQueues.push_back(
                        bsl::make_pair(info, referenceInfo));
                }
            }
        }
    }

    if (!incorrectQueues.empty()) {
        bdlb::Print::newlineAndIndent(out, level);
        out << "-----------------";
        bdlb::Print::newlineAndIndent(out, level);
        out << "Incorrect Queues :";
        bdlb::Print::newlineAndIndent(out, level);
        out << "-----------------";
        for (bsl::vector<bsl::pair<bsl::shared_ptr<ClusterStateQueueInfo>,
                                   bsl::shared_ptr<ClusterStateQueueInfo> > >::
                 const_iterator citer = incorrectQueues.cbegin();
             citer != incorrectQueues.cend();
             ++citer) {
            bdlb::Print::newlineAndIndent(out, level + 1);
            out << *citer->first;
            bdlb::Print::newlineAndIndent(out, level + 1);
            out << "(correct queue info) " << *citer->second;
        }
    }

    if (!extraQueues.empty()) {
        bdlb::Print::newlineAndIndent(out, level);
        out << "--------------";
        bdlb::Print::newlineAndIndent(out, level);
        out << "Extra queues :";
        bdlb::Print::newlineAndIndent(out, level);
        out << "--------------";
        for (bsl::vector<bsl::shared_ptr<ClusterStateQueueInfo> >::
                 const_iterator citer = extraQueues.cbegin();
             citer != extraQueues.cend();
             ++citer) {
            bdlb::Print::newlineAndIndent(out, level + 1);
            out << **citer;
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
            // Entire domain is missing
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
            if (citer == domCit->second->queuesInfo().cend()) {
                // Missing queue
                missingQueues.push_back(refCiter->second);
            }
        }
    }

    if (!missingQueues.empty()) {
        bdlb::Print::newlineAndIndent(out, level);
        out << "----------------";
        bdlb::Print::newlineAndIndent(out, level);
        out << "Missing queues :";
        bdlb::Print::newlineAndIndent(out, level);
        out << "----------------";
        for (bsl::vector<bsl::shared_ptr<ClusterStateQueueInfo> >::
                 const_iterator citer = missingQueues.cbegin();
             citer != missingQueues.cend();
             ++citer) {
            bdlb::Print::newlineAndIndent(out, level + 1);
            out << **citer;
        }
    }

    const bool queueInfoMismatch = !incorrectQueues.empty() ||
                                   !extraQueues.empty() ||
                                   !missingQueues.empty();
    if (queueInfoMismatch) {
        bdlb::Print::newlineAndIndent(out, level);
        out << "-------------------------";
        bdlb::Print::newlineAndIndent(out, level);
        out << "QUEUES IN CLUSTER STATE :";
        bdlb::Print::newlineAndIndent(out, level);
        out << "-------------------------";
        for (ClusterState::DomainStatesCIter domCit =
                 reference.domainStates().cbegin();
             domCit != reference.domainStates().cend();
             ++domCit) {
            for (ClusterState::UriToQueueInfoMapCIter citer =
                     domCit->second->queuesInfo().cbegin();
                 citer != domCit->second->queuesInfo().cend();
                 ++citer) {
                bdlb::Print::newlineAndIndent(out, level + 1);
                out << *citer->second;
            }
        }
    }

    if (!incorrectPartitions.empty() || queueInfoMismatch) {
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
    BSLS_ASSERT_SAFE(cluster->inDispatcherThread());

    if (!ledger.isOpen()) {
        BALL_LOG_INFO << clusterData.identity().description()
                      << ": Skipping validation of cluster state ledger at "
                      << "this time as it is not yet opened.";
        return;
    }

    // Compare contents of cluster state and CSL contents on disk
    ClusterState tempState(cluster,
                           clusterState.partitions().size(),
                           true,  // isTemporary
                           allocator);

    int rc =
        load(&tempState, ledger.getIterator().get(), clusterData, allocator);
    if (rc != 0) {
        BALL_LOG_ERROR << clusterData.identity().description()
                       << ": Unable to load cluster state from contents of "
                       << "cluster state ledger [rc:" << rc << "].";
        return;  // RETURN
    }

    bmqu::MemOutStream errorDescription;
    rc = validateState(errorDescription, tempState, clusterState);
    if (rc != 0) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << clusterData.identity().description()
            << ": Cluster state ledger's contents are"
            << " different from the cluster state: " << errorDescription.str()
            << BMQTSK_ALARMLOG_END;
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
    BSLS_ASSERT_SAFE(clusterData.cluster().inDispatcherThread());

    /// Value for the various RC error categories
    enum RcEnum {
        /// No error
        rc_SUCCESS = 0,

        /// An error was encountered while iterating through the iterator
        rc_ITERATION_ERROR = -1,

        // An error was encountered while attempting to load a message
        rc_MESSAGE_LOAD_ERROR = -2
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

    do {
        BSLS_ASSERT_SAFE(latestIter->isValid());

        bmqp_ctrlmsg::ClusterMessage clusterMessage;
        rc = latestIter->loadClusterMessage(&clusterMessage);
        if (rc != 0) {
            // Error loading the cluster message
            return rc * 10 + rc_MESSAGE_LOAD_ERROR;  // RETURN
        }

        // Apply advisories, whether committed or not.  Can ignore commit
        // records.
        //
        // NOTE: Consider the case where the leader applies an advisory,
        // receives enough acks, and commits the advisory, but then crashes
        // before the followers have a chance to write the commit.  One of the
        // followers becomes the new leader.  The new leader and the remaining
        // followers will see this as an uncommitted advisory; they must carry
        // out the last wish of the previous leader and commit this advisory.
        // That is why upon `ClusterUtil::load`, we apply the uncommitted
        // advisories knowing that they are about to be committed.  Also note
        // that uncommitted advisories must be synchronized by this time by the
        // new leader.
        typedef bmqp_ctrlmsg::ClusterMessageChoice MsgChoice;  // shortcut
        switch (clusterMessage.choice().selectionId()) {
        case MsgChoice::SELECTION_ID_PARTITION_PRIMARY_ADVISORY:
        case MsgChoice::SELECTION_ID_LEADER_ADVISORY:
        case MsgChoice::SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY:
        case MsgChoice::SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY:
        case MsgChoice::SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
            BALL_LOG_INFO << "#CSL_RECOVERY "
                          << clusterData.identity().description()
                          << ": Applying a recovered record from IncoreCSL: "
                          << clusterMessage << ".";
            apply(state, clusterMessage, clusterData);
        } break;  // BREAK
        case MsgChoice::SELECTION_ID_LEADER_ADVISORY_COMMIT: {
        } break;  // BREAK
        case MsgChoice::SELECTION_ID_UNDEFINED:
        default: {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << clusterData.identity().description()
                << ": Unexpected clusterMessage: " << clusterMessage
                << BMQTSK_ALARMLOG_END;
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
    BSLS_ASSERT_SAFE(out && out->empty());

    for (int pid = 0; pid < static_cast<int>(state.partitions().size());
         ++pid) {
        const ClusterStatePartitionInfo& pinfo = state.partition(pid);
        BSLS_ASSERT_SAFE(pinfo.partitionId() == pid);

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
                                 const ClusterState&                   state)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    const DomainStates& domainsInfo = state.domainStates();
    for (DomainStatesCIter domCit = domainsInfo.cbegin();
         domCit != domainsInfo.cend();
         ++domCit) {
        ClusterState::DomainState& domainState = *domCit->second;

        const UriToQueueInfoMap& queuesInfoPerDomain =
            domainState.queuesInfo();
        for (UriToQueueInfoMapCIter qCit = queuesInfoPerDomain.cbegin();
             qCit != queuesInfoPerDomain.cend();
             ++qCit) {
            const ClusterState::QueueInfoSp& infoSp = qCit->second;
            if (infoSp->state() != ClusterStateQueueInfo::State::k_ASSIGNED &&
                infoSp->state() !=
                    ClusterStateQueueInfo::State::k_UNASSIGNING) {
                continue;  // CONTINUE
            }

            bmqp_ctrlmsg::QueueInfo queueInfo;
            queueInfo.uri()         = infoSp->uri().asString();
            queueInfo.partitionId() = infoSp->partitionId();

            BSLS_ASSERT_SAFE(!infoSp->key().isNull());
            infoSp->key().loadBinary(&queueInfo.key());

            queueInfo.appIds().resize(infoSp->appInfos().size());
            size_t i = 0;
            for (AppInfosCIter appCIt = infoSp->appInfos().cbegin();
                 appCIt != infoSp->appInfos().cend();
                 ++appCIt) {
                queueInfo.appIds().at(i).appId() = appCIt->first;
                appCIt->second.loadBinary(&queueInfo.appIds().at(i).appKey());
                ++i;
            }

            out->push_back(queueInfo);
        }
    }
}

void ClusterUtil::loadPeerNodes(bsl::vector<mqbnet::ClusterNode*>* out,
                                const ClusterData&                 clusterData)
{
    // executed by the cluster *DISPATCHER* thread or the *QUEUE_DISPATCHER*
    // thread

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
