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

// mqbblp_clusterorchestrator.cpp                                     -*-C++-*-
#include <mqbblp_clusterorchestrator.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_clusterstatemanager.h>
#include <mqbblp_storagemanager.h>
#include <mqbc_clusterstateledger.h>
#include <mqbc_clusterstatemanager.h>
#include <mqbc_clusterutil.h>
#include <mqbc_incoreclusterstateledger.h>
#include <mqbcmd_messages.h>
#include <mqbi_queueengine.h>
#include <mqbi_storagemanager.h>
#include <mqbnet_cluster.h>

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdld_datummapbuilder.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_cstddef.h>  // size_t
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbi {
class Domain;
}

namespace mqbblp {

namespace {
/// Timeout duration for Cluster FSM watchdog -- 5 minutes
const bsls::Types::Int64 k_WATCHDOG_TIMEOUT_DURATION = 60 * 5;

}  // close unnamed namespace

// -------------------------
// class ClusterOrchestrator
// -------------------------

// PRIVATE MANIPULATORS
void ClusterOrchestrator::processElectorEventDispatched(
    const bmqp::Event&   event,
    mqbnet::ClusterNode* source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(event.isValid());
    BSLS_ASSERT_SAFE(source);

    d_elector_mp->processEvent(event, source);
}

void ClusterOrchestrator::onElectorStateChange(
    mqbnet::ElectorState::Enum            state,
    mqbnet::ElectorTransitionReason::Enum code,
    int                                   leaderNodeId,
    bsls::Types::Uint64                   term)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": ElectorStateChangeCb new state: " << state
                  << ", new code: " << code
                  << ", new leaderNodeId: " << leaderNodeId
                  << ", new term: " << term << ", old state: "
                  << d_clusterData_p->electorInfo().electorState()
                  << ", old leaderNodeId: "
                  << d_clusterData_p->electorInfo().leaderNodeId()
                  << ", old term: "
                  << d_clusterData_p->electorInfo().electorTerm();

    switch (state) {
    case mqbnet::ElectorState::e_DORMANT: {
        electorTransitionToDormant(leaderNodeId, term);
    } break;  // BREAK

    case mqbnet::ElectorState::e_FOLLOWER: {
        electorTransitionToFollower(leaderNodeId, term);
    } break;  // BREAK

    case mqbnet::ElectorState::e_CANDIDATE: {
        electorTransitionToCandidate(leaderNodeId, term);
    } break;  // BREAK

    case mqbnet::ElectorState::e_LEADER: {
        electorTransitionToLeader(leaderNodeId, term);
    } break;  // BREAK
    }
}

void ClusterOrchestrator::electorTransitionToDormant(int leaderNodeId,
                                                     bsls::Types::Uint64 term)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT(mqbnet::Elector::k_INVALID_NODE_ID == leaderNodeId);
    (void)leaderNodeId;  // Compiler happiness

    if (mqbnet::ElectorState::e_DORMANT ==
        d_clusterData_p->electorInfo().electorState()) {
        return;  // RETURN
    }

    switch (d_clusterData_p->electorInfo().electorState()) {
    case mqbnet::ElectorState::e_FOLLOWER: {
        d_clusterData_p->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_DORMANT,
            term,
            0,
            mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
    } break;  // BREAK
    case mqbnet::ElectorState::e_CANDIDATE: {
        d_clusterData_p->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_DORMANT,
            term,
            0,
            mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
    } break;  // BREAK
    case mqbnet::ElectorState::e_LEADER: {
        d_clusterData_p->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_DORMANT,
            term,
            0,
            mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
    } break;  // BREAK
    case mqbnet::ElectorState::e_DORMANT:
    default: BSLS_ASSERT(0 && "Unreachable by design");
    }
}

void ClusterOrchestrator::electorTransitionToFollower(int leaderNodeId,
                                                      bsls::Types::Uint64 term)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    mqbnet::ClusterNode* leaderNode =
        d_clusterData_p->membership().netCluster()->lookupNode(leaderNodeId);

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    if (mqbnet::Elector::k_INVALID_NODE_ID == leaderNodeId) {
        BSLS_ASSERT_SAFE(0 == leaderNode);
    }
#endif

    switch (d_clusterData_p->electorInfo().electorState()) {
    case mqbnet::ElectorState::e_FOLLOWER: {
        // Note that FOLLOWER -> FOLLOWER is a valid state transition in the
        // elector if the leadership changes.

        d_clusterData_p->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_FOLLOWER,
            term,
            leaderNode,
            leaderNode ? mqbc::ElectorInfoLeaderStatus::e_PASSIVE
                       : mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
    } break;  // BREAK
    case mqbnet::ElectorState::e_CANDIDATE: {
        d_clusterData_p->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_FOLLOWER,
            term,
            leaderNode,
            leaderNode ? mqbc::ElectorInfoLeaderStatus::e_PASSIVE
                       : mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
    } break;  // BREAK
    case mqbnet::ElectorState::e_LEADER: {
        if (mqbnet::Cluster::k_INVALID_NODE_ID != leaderNodeId) {
            BSLS_ASSERT(d_clusterData_p->electorInfo().leaderNodeId() !=
                        leaderNodeId);
            // Went from leader to follower-with-leader.
        }
        d_clusterData_p->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_FOLLOWER,
            term,
            leaderNode,
            leaderNode ? mqbc::ElectorInfoLeaderStatus::e_PASSIVE
                       : mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
    } break;  // BREAK

    case mqbnet::ElectorState::e_DORMANT: {
        BSLS_ASSERT(mqbnet::Elector::k_INVALID_NODE_ID == leaderNodeId);
        d_clusterData_p->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_FOLLOWER,
            term,
            leaderNode,
            leaderNode ? mqbc::ElectorInfoLeaderStatus::e_PASSIVE
                       : mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
    } break;  // BREAK
    default: BSLS_ASSERT(0 && "Unreachable by design");
    }
}

void ClusterOrchestrator::electorTransitionToCandidate(
    int                 leaderNodeId,
    bsls::Types::Uint64 term)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT(mqbnet::ElectorState::e_CANDIDATE !=
                d_clusterData_p->electorInfo().electorState());
    BSLS_ASSERT(mqbnet::Elector::k_INVALID_NODE_ID == leaderNodeId);
    (void)leaderNodeId;  // Compiler happiness

    d_clusterData_p->electorInfo().setElectorInfo(
        mqbnet::ElectorState::e_CANDIDATE,
        term,
        0,
        mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
}

void ClusterOrchestrator::electorTransitionToLeader(int leaderNodeId,
                                                    bsls::Types::Uint64 term)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(
        d_clusterData_p->membership().netCluster()->selfNodeId() ==
        leaderNodeId);
    BSLS_ASSERT_SAFE(mqbnet::ElectorState::e_CANDIDATE ==
                     d_clusterData_p->electorInfo().electorState());
    (void)leaderNodeId;  // Compiler happiness

    // The 'leaderMessageSequence' of this node should NOT be updated with new
    // term and sequenceNum of zero, because it will be used during leader
    // sync phase.  It's ok to update elector info though, with this node
    // marked as a passive leader.
    d_clusterData_p->electorInfo().setElectorInfo(
        mqbnet::ElectorState::e_LEADER,
        term,
        d_clusterData_p->membership().selfNode(),
        mqbc::ElectorInfoLeaderStatus::e_PASSIVE);

    d_stateManager_mp->initiateLeaderSync(true);
}

void ClusterOrchestrator::processBufferedQueueAdvisories()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processBufferedQueueAdvisories();
}

void ClusterOrchestrator::registerQueueInfo(const bmqt::Uri& uri,
                                            int              partitionId,
                                            const mqbu::StorageKey& queueKey,
                                            const AppInfos&         appIdInfos,
                                            bool forceUpdate)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(uri.isCanonical());

    d_stateManager_mp->registerQueueInfo(uri,
                                         partitionId,
                                         queueKey,
                                         appIdInfos,
                                         forceUpdate);
}

void ClusterOrchestrator::onPartitionPrimaryStatusDispatched(
    int          partitionId,
    int          status,
    unsigned int primaryLeaseId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    BSLS_ASSERT_SAFE(0 < primaryLeaseId);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(static_cast<size_t>(partitionId) <
                     clusterState()->partitions().size());

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
        d_clusterData_p->membership().selfNodeStatus()) {
        return;  // RETURN
    }

    const mqbc::ClusterStatePartitionInfo& pinfo = clusterState()->partition(
        partitionId);

    if (pinfo.primaryNode() != d_clusterData_p->membership().selfNode()) {
        // Cluster state now sees a different node as the primary for this
        // partition.  Log and move on.

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId
                      << "]: ignoring partition-primary sync notification "
                      << "because there is new primary now. LeaseId in "
                      << "notification: " << primaryLeaseId
                      << ". Current primary: "
                      << (pinfo.primaryNode()
                              ? pinfo.primaryNode()->nodeDescription()
                              : "** null **")
                      << ", current leaseId: " << pinfo.primaryLeaseId()
                      << ", current primary status: " << pinfo.primaryStatus();
        return;  // RETURN
    }

    if (pinfo.primaryLeaseId() != primaryLeaseId) {
        // Cluster state now sees a same (self) primary but different leaseId
        // for this partition.  Log and move on.

        // If same node was chosen as primary again, it must have a higher
        // leaseId.  We currently don't support bumping up leaseId without
        // changing the primary.

        BSLS_ASSERT_SAFE(pinfo.primaryLeaseId() > primaryLeaseId);

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId
                      << "]: ignoring partition-primary sync notification "
                      << "because primary (self) has different leaseId. "
                      << "LeaseId in notification: " << primaryLeaseId
                      << ". Current leaseId: " << pinfo.primaryLeaseId()
                      << ", current primary status: " << pinfo.primaryStatus();
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE ==
                     pinfo.primaryStatus());

    if (0 != status) {
        // Primary (self) failed to sync partition. This scenario is currently
        // not handled.  The leader should assign a new primary if old primary
        // fails to transition to ACTIVE status in the stipulated time.

        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << partitionId
            << "]: primary node (self) failed to sync partition, rc: "
            << status << ", leaseId: " << primaryLeaseId
            << BMQTSK_ALARMLOG_END;

        d_stateManager_mp->setPrimaryStatus(
            partitionId,
            bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE);
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId
                  << "]: primary node (self) successfully synced the "
                  << " partition. Current leaseId: " << pinfo.primaryLeaseId();

    // Update primary status via cluster state manager which will also notify
    // the components observing the primary status.  Do *not* update the status
    // via 'pinfo' variable (it is declared as 'const' for this reason).
    d_stateManager_mp->setPrimaryStatus(partitionId,
                                        bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE);
}

void ClusterOrchestrator::onNodeUnavailable(mqbnet::ClusterNode* node)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    mqbc::ClusterNodeSession* ns =
        d_clusterData_p->membership().getClusterNodeSession(node);
    BSLS_ASSERT_SAFE(ns);

    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE ==
                     ns->nodeStatus());

    // Peer has gone.  Drop all queue handles, if any, opened by the peer on
    // this node.

    dropPeerQueues(ns);

    if (ns->primaryPartitions().empty()) {
        // Node was not primary for any partition.  Nothing else to do.

        return;  // RETURN
    }

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM
            << d_clusterData_p->identity().description() << ": "
            << node->nodeDescription() << " has gone down. "
            << "Node was primary for " << ns->primaryPartitions().size()
            << " partition(s): [";
        for (unsigned int i = 0; i < ns->primaryPartitions().size(); ++i) {
            BALL_LOG_OUTPUT_STREAM << ns->primaryPartitions()[i];
            if (i < (ns->primaryPartitions().size() - 1)) {
                BALL_LOG_OUTPUT_STREAM << ", ";
            }
        }
        BALL_LOG_OUTPUT_STREAM << "]";
    }

    // Clear out the partitions assigned to 'node'.  TBD: this is done by
    // followers as well.  Is the correct behavior?  Should this be done only
    // by the leader, and followers should update the info only upon being
    // notified from the leader?

    d_stateManager_mp->markOrphan(ns->primaryPartitions(), node);
    ns->removeAllPartitions();

    if (mqbnet::ElectorState::e_LEADER !=
            d_clusterData_p->electorInfo().electorState() ||
        mqbc::ElectorInfoLeaderStatus::e_PASSIVE ==
            d_clusterData_p->electorInfo().leaderStatus() ||
        bmqp_ctrlmsg::NodeStatus::E_AVAILABLE !=
            d_clusterData_p->membership().selfNodeStatus()) {
        // Nothing to do if self is not active leader, or if self is active
        // leader but is stopping.

        return;  // RETURN
    }

    // Self is active leader; assign new primary(s) to orphaned partitions.
    bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo> partitions;
    d_stateManager_mp->assignPartitions(&partitions);

    // Broadcast new partition-primary mapping to followers.  It's ok to send
    // information for partitions whose mapping hasn't changed.
    d_stateManager_mp->sendClusterState(true,   // sendPartitionPrimaryInfo
                                        false,  // sendQueuesInfo
                                        0,
                                        partitions);
}

void ClusterOrchestrator::dropPeerQueues(mqbc::ClusterNodeSession* ns)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(ns);

    // Drop all queue handles, if any, opened by the peer (replica) identified
    // by the specified 'ns' on self node (primary or next upstream hop).  This
    // is equivalent to the scenario when a client stops or crashes without
    // explicitly closing all the queues.

    QueueHandleMap&    qhMap = ns->queueHandles();
    QueueHandleMapIter qit   = qhMap.begin();

    while (qit != qhMap.end()) {
        mqbi::QueueHandle* qh = qit->second.d_handle_p;
        BSLS_ASSERT_SAFE(qh);
        qh->clearClient(false);
        qh->drop();
        qhMap.erase(qit++);
    }
}

void ClusterOrchestrator::timerCb()
{
    // executed by the *SCHEDULER* thread

    d_clusterData_p->dispatcherClientData().dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterOrchestrator::timerCbDispatched, this),
        &d_clusterData_p->cluster());
}

void ClusterOrchestrator::timerCbDispatched()
{
    // executed by the *CLUSTER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        dispatcher()->inDispatcherThread(&d_clusterData_p->cluster()));

    const bsls::Types::Int64 timer = bmqsys::Time::highResolutionTimer();

    for (size_t partitionId = 0;
         partitionId < clusterState()->partitions().size();
         ++partitionId) {
        if (!clusterState()->isSelfActivePrimary(partitionId)) {
            continue;  // CONTINUE
        }

        const mqbs::FileStore& fs = d_storageManager_p->fileStore(partitionId);

        if (!fs.isOpen()) {
            continue;  // CONTINUE
        }

        dispatcher()->execute(
            bdlf::BindUtil::bind(&ClusterOrchestrator::onQueueActivityTimer,
                                 this,
                                 timer,
                                 partitionId),
            fs.dispatcherClientData());
    }
}

void ClusterOrchestrator::onQueueActivityTimer(bsls::Types::Int64 timer,
                                               int                partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_storageManager_p->fileStore(partitionId).inDispatcherThread());

    struct local {
        static void onTimerFunctor(mqbi::Queue*       _queue,
                                   bsls::Types::Int64 _timer)
        {
            _queue->queueEngine()->onTimer(_timer);
        }
    };

    d_storageManager_p->applyForEachQueue(
        partitionId,
        bdlf::BindUtil::bind(local::onTimerFunctor,
                             bdlf::PlaceHolders::_1,  // queue
                             timer));
}

// CREATORS
ClusterOrchestrator::ClusterOrchestrator(
    mqbcfg::ClusterDefinition& clusterConfig,
    mqbi::Cluster*             cluster,
    mqbc::ClusterData*         clusterData,
    mqbc::ClusterState*        clusterState,
    bslma::Allocator*          allocator)
: d_allocator_p(allocator)
, d_allocators(d_allocator_p)
, d_isStarted(false)
, d_wasAvailableAdvisorySent(false)
, d_clusterConfig(clusterConfig)
, d_cluster_p(cluster)
, d_clusterData_p(clusterData)
, d_stateManager_mp(
      clusterConfig.clusterAttributes().isFSMWorkflow()
          ? static_cast<mqbi::ClusterStateManager*>(
                new(*d_allocator_p) mqbc::ClusterStateManager(
                    clusterConfig,
                    d_cluster_p,
                    d_clusterData_p,
                    clusterState,
                    ClusterStateManager::ClusterStateLedgerMp(
                        bslma::ManagedPtrUtil::allocateManaged<
                            mqbc::IncoreClusterStateLedger>(
                            d_allocators.get("ClusterStateLedger"),
                            clusterConfig,
                            mqbc::ClusterStateLedgerConsistency::e_EVENTUAL,
                            // TODO Add cluster config to determine Eventual vs
                            //      Strong
                            d_clusterData_p,
                            clusterState,
                            &d_clusterData_p->bufferFactory(),
                            &d_clusterData_p->blobSpPool())),
                    k_WATCHDOG_TIMEOUT_DURATION,
                    d_allocators.get("ClusterStateManager")))
          : static_cast<mqbi::ClusterStateManager*>(
                new(*d_allocator_p) ClusterStateManager(
                    clusterConfig,
                    d_cluster_p,
                    d_clusterData_p,
                    clusterState,
                    ClusterStateManager::ClusterStateLedgerMp(
                        bslma::ManagedPtrUtil::allocateManaged<
                            mqbc::IncoreClusterStateLedger>(
                            d_allocators.get("ClusterStateLedger"),
                            clusterConfig,
                            mqbc::ClusterStateLedgerConsistency::e_EVENTUAL,
                            // TODO Add cluster config to determine Eventual vs
                            //      Strong
                            d_clusterData_p,
                            clusterState,
                            &d_clusterData_p->bufferFactory(),
                            &d_clusterData_p->blobSpPool())),
                    d_allocators.get("ClusterStateManager"))),
      d_allocator_p)
, d_queueHelper(d_clusterData_p,
                clusterState,
                d_stateManager_mp.get(),
                allocator)
, d_elector_mp()
, d_storageManager_p(0)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(allocator);
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_clusterData_p);
}

ClusterOrchestrator::~ClusterOrchestrator()
{
    // executed by *ANY* thread

    BSLS_ASSERT(!d_isStarted && "stop() must be called");
}

// MANIPULATORS
int ClusterOrchestrator::start(bsl::ostream& errorDescription)
{
    // executed by the cluster *DISPATCHER* thread

    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    enum {
        rc_SUCCESS                       = 0,
        rc_ELECTOR_FAILURE               = -1,
        rc_CLUSTER_STATE_MANAGER_FAILURE = -2
    };

    if (d_isStarted) {
        return rc_SUCCESS;  // RETURN
    }

    // Start the elector.  It must be one of the first things to get started,
    // so that new node can discover the leader as soon as possible.

    if (isLocal()) {
        // Specify minimum initial wait time and election result wait time in
        // case of 1-node cluster.
        mqbcfg::ElectorConfig& electorCfg    = d_clusterConfig.elector();
        electorCfg.initialWaitTimeoutMs()    = 0;
        electorCfg.electionResultTimeoutMs() = 0;
    }

    // Start the cluster state manager
    int rc = d_stateManager_mp->start(errorDescription);

    if (rc != 0) {
        return rc * 10 + rc_CLUSTER_STATE_MANAGER_FAILURE;  // RETURN
    }

    bsls::Types::Uint64 electorTerm = mqbnet::Elector::k_INVALID_TERM;

    // If CSLMode is enabled, fetch latest elector term from the ledger and
    // assign it to the elector term.
    if (d_cluster_p->isCSLModeEnabled()) {
        bmqp_ctrlmsg::LeaderMessageSequence ledgerLSN;
        rc = d_stateManager_mp->latestLedgerLSN(&ledgerLSN);
        if (rc == 0) {
            electorTerm = ledgerLSN.electorTerm();

            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << ": Latest elector term fetched from the cluster "
                          << "state ledger: " << electorTerm;
        }
        else {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": Failed to fetch latest elector term from the "
                          << "cluster state ledger, rc: " << rc;
        }
    }

    d_clusterData_p->electorInfo().setElectorTerm(electorTerm);

    using namespace bdlf::PlaceHolders;
    d_elector_mp.load(
        new (*d_allocator_p) mqbnet::Elector(
            d_clusterConfig.elector(),
            &d_clusterData_p->cluster(),
            bdlf::BindUtil::bind(&ClusterOrchestrator::onElectorStateChange,
                                 this,
                                 _1,   // ElectorState
                                 _2,   // ElectorTransitionCode
                                 _3,   // LeaderNodeId
                                 _4),  // Term
            electorTerm,
            &d_clusterData_p->blobSpPool(),
            d_allocator_p),
        d_allocator_p);

    rc = d_elector_mp->start();
    if (rc != 0) {
        errorDescription << "Failed to start elector";
        return rc * 10 + rc_ELECTOR_FAILURE;  // RETURN
    }

    bsls::TimeInterval interval;
    interval.setTotalMilliseconds(
        d_clusterConfig.queueOperations().consumptionMonitorPeriodMs());
    d_clusterData_p->scheduler().scheduleRecurringEvent(
        &d_consumptionMonitorEventHandle,
        interval,
        bdlf::BindUtil::bind(&ClusterOrchestrator::timerCb, this));

    d_isStarted = true;
    return rc_SUCCESS;
}

void ClusterOrchestrator::stop()
{
    // executed by the cluster *DISPATCHER* thread

    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    BSLS_ASSERT_SAFE(d_clusterData_p);
    d_clusterData_p->scheduler().cancelEventAndWait(
        &d_consumptionMonitorEventHandle);

    d_stateManager_mp->stop();

    BSLS_ASSERT_SAFE(d_elector_mp);
    d_elector_mp->stop();
}

void ClusterOrchestrator::updateDatumStats(
    mqbc::ClusterNodeSession* nodeSession)
{
    bsl::vector<mqbc::ClusterNodeSession*> nodes;
    if (nodeSession == 0) {
        ClusterNodeSessionMapIter iter =
            d_clusterData_p->membership().clusterNodeSessionMap().begin();
        ClusterNodeSessionMapIter endIter =
            d_clusterData_p->membership().clusterNodeSessionMap().end();
        for (; iter != endIter; ++iter) {
            nodes.push_back((iter->second).get());
        }
    }
    else {
        nodes.push_back(nodeSession);
    }

    bsl::string leader = "";
    if (d_clusterData_p->electorInfo().leaderNode() != 0) {
        leader = d_clusterData_p->electorInfo().leaderNode()->hostName();
    }
    else {
        leader = "(No leader)";
    }

    int status = 0;
    for (size_t i = 0; i < nodes.size(); ++i) {
        // status is -1 if node is self, 0 if node is down, 1 if node is up
        if (d_clusterData_p->membership().selfNodeSession() == nodes[i]) {
            status = -1;
        }
        else {
            status = nodes[i]->clusterNode()->isAvailable();
        }

        bslma::Allocator* alloc = nodes[i]->statContext()->datumAllocator();
        ManagedDatumMp    datum = nodes[i]->statContext()->datum();

        bdld::DatumMapBuilder builder(alloc);
        builder.pushBack("leader", bdld::Datum::copyString(leader, alloc));
        builder.pushBack("status", bdld::Datum::createInteger(status));
        builder.pushBack("isProxy", bdld::Datum::createInteger(false));

        datum->adopt(builder.commit());
    }
}

void ClusterOrchestrator::transitionToAvailable()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_clusterData_p->membership().setSelfNodeStatus(
        bmqp_ctrlmsg::NodeStatus::E_AVAILABLE);

    if (!d_wasAvailableAdvisorySent) {
        bmqp_ctrlmsg::ControlMessage  controlMsg;
        bmqp_ctrlmsg::ClusterMessage& clusterMsg =
            controlMsg.choice().makeClusterMessage();
        bmqp_ctrlmsg::NodeStatusAdvisory& advisory =
            clusterMsg.choice().makeNodeStatusAdvisory();
        advisory.status() = bmqp_ctrlmsg::NodeStatus::E_AVAILABLE;
        d_clusterData_p->messageTransmitter().broadcastMessage(controlMsg,
                                                               true);

        d_wasAvailableAdvisorySent = true;
    }

    BALL_LOG_INFO
        << "\n      ___  _    __ ___     ____ __     ___     ____   __     "
           "______"
        << "\n     /   || |  / //   |   /  _// /    /   |   / __ ) / /    / "
           "____/"
        << "\n    / /| || | / // /| |   / / / /    / /| |  / __  |/ /    / __/"
        << "\n   / ___ || |/ // ___ | _/ / / /___ / ___ | / /_/ // /___ / /___"
        << "\n  /_/  |_||___//_/  |_|/___//_____//_/  |_|/_____//_____//_____/"
        << "  " << d_cluster_p->description() << "\n";

    BALL_LOG_INFO << d_cluster_p->description() << " is available";

    if (isLocal()) {
        d_storageManager_p->gcUnrecognizedDomainQueues();
    }
}

void ClusterOrchestrator::processStopRequest(
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    const bmqp_ctrlmsg::StopRequest& stopRequest =
        request.choice().clusterMessage().choice().stopRequest();
    const bsl::string& name = d_cluster_p->name();

    mqbc::ClusterNodeSession* ns =
        d_clusterData_p->membership().getClusterNodeSession(source);
    BSLS_ASSERT_SAFE(ns);
    BSLS_ASSERT_SAFE(ns->clusterNode() == source);

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != ns->nodeStatus()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": received StopRequest from "
                      << source->nodeDescription()
                      << ", but node is not AVAILABLE.  Current status: "
                      << ns->nodeStatus();

        bmqp_ctrlmsg::ControlMessage response;

        response.choice()
            .makeClusterMessage()
            .choice()
            .makeStopResponse()
            .clusterName() = name;
        response.rId()     = request.rId();

        d_clusterData_p->messageTransmitter().sendMessage(response, source);
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": received StopRequest [" << stopRequest << "] from "
                  << source->nodeDescription()
                  << ", current status: " << ns->nodeStatus()
                  << ", new status: " << bmqp_ctrlmsg::NodeStatus::E_STOPPING;

    // TODO(shutdown-v2): TEMPORARY, remove when all switch to StopRequest V2.
    if (stopRequest.version() == 1 && stopRequest.clusterName() != name) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": invalid cluster name in the StopRequest from "
                       << source->nodeDescription() << ", " << request;
        return;  // RETURN
    }

    ns->setNodeStatus(bmqp_ctrlmsg::NodeStatus::E_STOPPING);

    processNodeStoppingNotification(ns, &request);
}

void ClusterOrchestrator::processClusterStateFSMMessage(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterConfig.clusterAttributes().isCSLModeEnabled() &&
                     d_clusterConfig.clusterAttributes().isFSMWorkflow());
    BSLS_ASSERT(message.choice().isClusterMessageValue());
    BSLS_ASSERT(message.choice()
                    .clusterMessage()
                    .choice()
                    .isClusterStateFSMMessageValue());

    typedef bmqp_ctrlmsg::ClusterStateFSMMessageChoice MsgChoice;  // shortcut
    switch (message.choice()
                .clusterMessage()
                .choice()
                .clusterStateFSMMessage()
                .choice()
                .selectionId()) {
    case MsgChoice::SELECTION_ID_FOLLOWER_L_S_N_REQUEST: {
        d_stateManager_mp->processFollowerLSNRequest(message, source);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_REGISTRATION_REQUEST: {
        d_stateManager_mp->processRegistrationRequest(message, source);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST: {
        d_stateManager_mp->processFollowerClusterStateRequest(message, source);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_FOLLOWER_L_S_N_RESPONSE:
        BSLS_ANNOTATION_FALLTHROUGH;
    case MsgChoice::SELECTION_ID_REGISTRATION_RESPONSE:
        BSLS_ANNOTATION_FALLTHROUGH;
    case MsgChoice::SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE: {
        BSLS_ASSERT_SAFE(!message.rId().isNull());

        d_cluster_p->processResponse(message);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_UNDEFINED: BSLS_ANNOTATION_FALLTHROUGH;
    default: {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description()
            << ": unexpected clusterMessage:" << message
            << BMQTSK_ALARMLOG_END;
    } break;  // BREAK
    }
}

void ClusterOrchestrator::processPartitionMessage(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterConfig.clusterAttributes().isCSLModeEnabled() &&
                     d_clusterConfig.clusterAttributes().isFSMWorkflow());
    BSLS_ASSERT(message.choice().isClusterMessageValue());
    BSLS_ASSERT(
        message.choice().clusterMessage().choice().isPartitionMessageValue());

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
        d_clusterData_p->membership().selfNodeStatus()) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Not processing partition message : " << message
                      << " from " << source->nodeDescription()
                      << " since self is stopping";

        return;  // RETURN
    }

    typedef bmqp_ctrlmsg::PartitionMessageChoice MsgChoice;  // shortcut
    switch (message.choice()
                .clusterMessage()
                .choice()
                .partitionMessage()
                .choice()
                .selectionId()) {
    case MsgChoice::SELECTION_ID_REPLICA_STATE_REQUEST: {
        d_storageManager_p->processReplicaStateRequest(message, source);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_PRIMARY_STATE_REQUEST: {
        d_storageManager_p->processPrimaryStateRequest(message, source);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_REPLICA_DATA_REQUEST: {
        d_storageManager_p->processReplicaDataRequest(message, source);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_REPLICA_STATE_RESPONSE:
        BSLS_ANNOTATION_FALLTHROUGH;
    case MsgChoice::SELECTION_ID_PRIMARY_STATE_RESPONSE:
        BSLS_ANNOTATION_FALLTHROUGH;
    case MsgChoice::SELECTION_ID_REPLICA_DATA_RESPONSE: {
        BSLS_ASSERT_SAFE(!message.rId().isNull());

        d_cluster_p->processResponse(message);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_UNDEFINED: BSLS_ANNOTATION_FALLTHROUGH;
    default: {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description()
            << ": unexpected clusterMessage:" << message
            << BMQTSK_ALARMLOG_END;
    } break;  // BREAK
    }
}

void ClusterOrchestrator::processNodeStoppingNotification(
    mqbc::ClusterNodeSession*           ns,
    const bmqp_ctrlmsg::ControlMessage* request)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    // If 'source' is the leader, mark it as passive.

    if (ns->clusterNode() == d_clusterData_p->electorInfo().leaderNode()) {
        d_clusterData_p->electorInfo().setLeaderStatus(
            mqbc::ElectorInfoLeaderStatus::e_PASSIVE);
    }

    // Self node needs to issue close-queue requests for all the queues for
    // which specified 'source' node is the primary.

    d_queueHelper.processNodeStoppingNotification(ns->clusterNode(),
                                                  request,
                                                  ns);

    // For each partition for which self is primary, notify the StorageMgr
    // about the status of a peer node.  Self may end up issuing a
    // (non-scheduled) sync point to the node.

    const bsl::vector<int>& partitions =
        d_clusterData_p->membership().selfNodeSession()->primaryPartitions();
    for (unsigned int i = 0; i < partitions.size(); ++i) {
        d_storageManager_p->processReplicaStatusAdvisory(
            partitions[i],
            ns->clusterNode(),
            bmqp_ctrlmsg::NodeStatus::E_STOPPING);
    }
}

void ClusterOrchestrator::processNodeStatusAdvisory(
    const bmqp_ctrlmsg::ControlMessage& advisory,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(advisory.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(advisory.choice()
                         .clusterMessage()
                         .choice()
                         .isNodeStatusAdvisoryValue());

    // This advisory is received by all peers in the cluster whenever the
    // 'source' node changes its status (eg STARTING -> AVAILABLE, AVAILABLE ->
    // STOPPING, etc).

    const bmqp_ctrlmsg::NodeStatusAdvisory& nsAdvisory =
        advisory.choice().clusterMessage().choice().nodeStatusAdvisory();

    mqbc::ClusterNodeSession* ns =
        d_clusterData_p->membership().getClusterNodeSession(source);
    BSLS_ASSERT_SAFE(ns);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": received node status advisory from "
                  << source->nodeDescription()
                  << ", current status: " << ns->nodeStatus()
                  << ", new status: " << nsAdvisory.status();

    ns->setNodeStatus(nsAdvisory.status());

    if (bmqp_ctrlmsg::NodeStatus::E_STARTING == nsAdvisory.status()) {
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE == nsAdvisory.status()) {
        onNodeUnavailable(source);

        // Nothing else to do for follower as well as the leader.

        return;  // RETURN
    }

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING == nsAdvisory.status()) {
        processNodeStoppingNotification(ns, 0);
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE == nsAdvisory.status()) {
        if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
            d_clusterData_p->membership().selfNodeStatus()) {
            return;  // RETURN
        }

        if (mqbnet::ElectorState::e_LEADER ==
                d_clusterData_p->electorInfo().electorState() &&
            mqbc::ElectorInfoLeaderStatus::e_ACTIVE ==
                d_clusterData_p->electorInfo().leaderStatus()) {
            // Self is ACTIVE leader.  Although self has sent leader advisory
            // to 'source' when 'source' came up (see
            // 'processNodeStateChange'), it sends the advisory again, in case
            // 'source' ignored the previous advisory due to not recognizing
            // self as leader.  Although self does send a leader heartbeat to
            // 'source', but due to race related to thread scheduling, 'source'
            // may see that heartbeat after the first advisory.

            bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo> partitions;
            mqbc::ClusterUtil::loadPartitionsInfo(&partitions,
                                                  *clusterState());
            d_stateManager_mp->sendClusterState(
                true,  // sendPartitionPrimaryInfo
                true,  // sendQueuesInfo
                source,
                partitions);
        }
        else if (d_clusterConfig.clusterAttributes().isFSMWorkflow() &&
                 source->nodeId() ==
                     d_clusterData_p->electorInfo().leaderNodeId()) {
            d_queueHelper.onLeaderAvailable();
        }

        // For each partition for which self is primary, notify the storageMgr
        // about the status of a peer node.  Self may end up issuing a primary
        // status advisory and a (non-scheduled) sync point to the node.  Note
        // that this was done upon receiving the 'STARTING' node status
        // advisory from the 'source' as well, but given the connection
        // initiation logic (node with smaller Id initiates the connection to
        // node with higher Id) plus the amount of time spent by 'source' in
        // recovery, plus thread-scheduling in the 'source', 'source' may end
        // up sending only AVAILABLE status advisory (doesn't see STARTING
        // status advisory).

        const bsl::vector<int>& partitions = d_clusterData_p->membership()
                                                 .selfNodeSession()
                                                 ->primaryPartitions();
        for (unsigned int i = 0; i < partitions.size(); ++i) {
            d_storageManager_p->processReplicaStatusAdvisory(
                partitions[i],
                source,
                nsAdvisory.status());
        }

        return;  // RETURN
    }
}

void ClusterOrchestrator::processNodeStateChangeEvent(
    mqbnet::ClusterNode* node,
    bool                 isAvailable)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    // IMPORTANT: Handling of this event must remain idempotent.  See notes in
    // Cluster.startDispatched() for details.

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": received node state change notification for node "
                  << node->nodeDescription() << ". Status: "
                  << (isAvailable ? "available" : "unavailable");

    // Forward the event to elector.  If self is leader, elector will issue an
    // immediate hearbeat.

    d_elector_mp->processNodeStatus(node, isAvailable);

    // Update self's view of the 'node'.

    mqbc::ClusterNodeSession* ns =
        d_clusterData_p->membership().getClusterNodeSession(node);
    BSLS_ASSERT_SAFE(ns);

    if (isAvailable) {
        if (bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE == ns->nodeStatus()) {
            // Current status of the peer node is unavailable, which means we
            // are seeing this 'available' event for *this* instance of the
            // peer for the first time.  Bump up peer's instance ID.

            int oldInstanceId = ns->peerInstanceId();
            ns->setPeerInstanceId(oldInstanceId + 1);

            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << ": bumped up instance ID of peer node "
                          << node->nodeDescription()
                          << " from: " << oldInstanceId
                          << " to: " << ns->peerInstanceId();
        }

        // Node is connected, but we don't know its status as of yet so we mark
        // it appropriately.

        ns->setNodeStatus(bmqp_ctrlmsg::NodeStatus::E_UNKNOWN);

        // Send self's status to the node.

        bmqp_ctrlmsg::ControlMessage  controlMsg;
        bmqp_ctrlmsg::ClusterMessage& clusterMsg =
            controlMsg.choice().makeClusterMessage();
        bmqp_ctrlmsg::NodeStatusAdvisory& advisory =
            clusterMsg.choice().makeNodeStatusAdvisory();

        advisory.status() = d_clusterData_p->membership().selfNodeStatus();
        d_clusterData_p->messageTransmitter().sendMessage(controlMsg, node);

        updateDatumStats(ns);

        if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
            d_clusterData_p->membership().selfNodeStatus()) {
            return;  // RETURN
        }

        if (d_clusterData_p->electorInfo().isSelfActiveLeader()) {
            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << ": leader (self) is ACTIVE; will send leader"
                          << " advisory to new node: "
                          << node->nodeDescription();

            bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo> partitions;
            mqbc::ClusterUtil::loadPartitionsInfo(&partitions,
                                                  *clusterState());
            d_stateManager_mp->sendClusterState(
                true,  // sendPartitionPrimaryInfo
                true,  // sendQueuesInfo
                node,
                partitions);
        }

        // For each partition for which self is primary, notify the storageMgr
        // about the status of a peer node.  Self may end up issuing a primary
        // status advisory and a (non-scheduled) sync point to the node.

        const bsl::vector<int>& partitions = d_clusterData_p->membership()
                                                 .selfNodeSession()
                                                 ->primaryPartitions();
        for (unsigned int i = 0; i < partitions.size(); ++i) {
            d_storageManager_p->processReplicaStatusAdvisory(
                partitions[i],
                node,
                bmqp_ctrlmsg::NodeStatus::E_UNKNOWN);
        }

        return;  // RETURN
    }

    // 'node' has gone down.  Mark it as UNAVAILABLE.

    ns->setNodeStatus(bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE);

    // Cancel all outstanding requests to that node, with the 'CANCELED'
    // category and the 'e_NODE_DOWN' code.

    bmqp_ctrlmsg::ControlMessage response;
    bmqp_ctrlmsg::Status&        failure = response.choice().makeStatus();
    failure.category() = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
    failure.code()     = mqbi::ClusterErrorCode::e_NODE_DOWN;
    failure.message()  = "Lost connection with peer node";

    d_clusterData_p->requestManager().cancelAllRequests(response,
                                                        node->nodeId());

    // Note that if the 'node' went down gracefully, 'onNodeUnavailable' would
    // have also been called in
    // 'processNodeStatusAdvisory(state=NodeStatus::E_UNAVAILABLE)', so
    // 'onNodeUnavailable' will be executed again below.  This is ok.

    onNodeUnavailable(node);
}

void ClusterOrchestrator::processElectorEvent(const bmqp::Event&   event,
                                              mqbnet::ClusterNode* source)
{
    // executed by *IO* thread

    BSLS_ASSERT_SAFE(event.isElectorEvent());

    if (isLocal()) {
        // We shouldn't be getting any elector related messages if its a local
        // cluster.
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description()
            << " received an elector event from node "
            << source->nodeDescription() << " in local cluster setup."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (d_clusterData_p->membership().selfNodeStatus() ==
        bmqp_ctrlmsg::NodeStatus::E_STOPPING) {
        // No need to process the event since self is stopping.
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Not processing elector event from node "
                      << source->nodeDescription()
                      << " since self is stopping.";
        return;  // RETURN
    }

    // Enqueue elector events in the dispatcher thread as well.  Note that its
    // important that elector events are processed in the dispatcher thread
    // too, otherwise, depending upon thread scheduling, a new node may get
    // certain events "out of order" (some cases were found out while testing).
    // Note that 'bindA' instead of 'bind' is needed below because we need to
    // pass allocator to one of the 'bmqp::Event' instances created below
    // (allocator is *not* optional for 'bmqp::Event')
    dispatcher()->execute(
        bdlf::BindUtil::bindS(
            d_allocator_p,
            &ClusterOrchestrator::processElectorEventDispatched,
            this,
            event.clone(d_allocator_p),
            source),
        d_cluster_p);
}

void ClusterOrchestrator::processLeaderSyncStateQuery(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processLeaderSyncStateQuery(message, source);
}

void ClusterOrchestrator::processQueueAssignmentRequest(
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbnet::ClusterNode*                requester)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processQueueAssignmentRequest(request, requester);
}

void ClusterOrchestrator::processQueueAssignmentAdvisory(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processQueueAssignmentAdvisory(message, source);
}

void ClusterOrchestrator::processQueueUnassignedAdvisory(
    const bmqp_ctrlmsg::ControlMessage& msg,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processQueueUnassignedAdvisory(msg, source);
}

void ClusterOrchestrator::processQueueUnAssignmentAdvisory(
    const bmqp_ctrlmsg::ControlMessage& msg,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processQueueUnAssignmentAdvisory(msg, source);
}

void ClusterOrchestrator::processLeaderSyncDataQuery(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processLeaderSyncDataQuery(message, source);
}

void ClusterOrchestrator::processClusterStateEvent(
    const mqbi::DispatcherClusterStateEvent& event)
{
    // executed by *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processClusterStateEvent(event);
}

void ClusterOrchestrator::processPartitionPrimaryAdvisory(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processPartitionPrimaryAdvisory(message, source);
}

void ClusterOrchestrator::processLeaderAdvisory(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->processLeaderAdvisory(message, source);
}

void ClusterOrchestrator::processStorageSyncRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT(message.choice().isClusterMessageValue());
    BSLS_ASSERT(message.choice()
                    .clusterMessage()
                    .choice()
                    .isStorageSyncRequestValue());

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = message.rId();

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE !=
        d_clusterData_p->membership().selfNodeStatus()) {
        const bmqp_ctrlmsg::StorageSyncRequest& req =
            message.choice().clusterMessage().choice().storageSyncRequest();

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": unable to serve storage sync request " << req
                      << " from node " << source->nodeDescription()
                      << " because self is not available. Self status: "
                      << d_clusterData_p->membership().selfNodeStatus();

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_NOT_READY;
        status.code()     = -1;
        status.message()  = "Node is not available.";
        d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);
        return;  // RETURN
    }

    d_storageManager_p->processStorageSyncRequest(message, source);
}

void ClusterOrchestrator::processPartitionSyncStateRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT(message.choice().isClusterMessageValue());
    BSLS_ASSERT(message.choice()
                    .clusterMessage()
                    .choice()
                    .isPartitionSyncStateQueryValue());

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = message.rId();

    // Node must be AVAILABLE to serve partition-sync state request.

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE !=
        d_clusterData_p->membership().selfNodeStatus()) {
        const bmqp_ctrlmsg::PartitionSyncStateQuery& req =
            message.choice()
                .clusterMessage()
                .choice()
                .partitionSyncStateQuery();

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": unable to serve partition sync state request: "
                      << req << " from node " << source->nodeDescription()
                      << " because self is not AVAILABLE. Self status: "
                      << d_clusterData_p->membership().selfNodeStatus();

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_NOT_READY;
        status.code()     = -1;
        status.message()  = "Node is not available.";
        d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);
        return;  // RETURN
    }

    d_storageManager_p->processPartitionSyncStateRequest(message, source);
}

void ClusterOrchestrator::processPartitionSyncDataRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT(message.choice().isClusterMessageValue());
    BSLS_ASSERT(message.choice()
                    .clusterMessage()
                    .choice()
                    .isPartitionSyncDataQueryValue());

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = message.rId();

    // Node must be AVAILABLE to serve partition-sync data request.

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE !=
        d_clusterData_p->membership().selfNodeStatus()) {
        const bmqp_ctrlmsg::PartitionSyncDataQuery& req =
            message.choice()
                .clusterMessage()
                .choice()
                .partitionSyncDataQuery();

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": unable to serve partition sync data request: "
                      << req << " from node " << source->nodeDescription()
                      << " because self is not AVAILABLE. Self status: "
                      << d_clusterData_p->membership().selfNodeStatus();

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_NOT_READY;
        status.code()     = -1;
        status.message()  = "Node is not available.";
        d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);
        return;  // RETURN
    }

    d_storageManager_p->processPartitionSyncDataRequest(message, source);
}

void ClusterOrchestrator::processPartitionSyncDataRequestStatus(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT(message.choice().isClusterMessageValue());
    BSLS_ASSERT(message.choice()
                    .clusterMessage()
                    .choice()
                    .isPartitionSyncDataQueryStatusValue());

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = message.rId();

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE !=
        d_clusterData_p->membership().selfNodeStatus()) {
        const bmqp_ctrlmsg::PartitionSyncDataQueryStatus& req =
            message.choice()
                .clusterMessage()
                .choice()
                .partitionSyncDataQueryStatus();

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": unable to serve partition sync data query status: "
                      << req << " from node " << source->nodeDescription()
                      << " because self is not AVAILABLE. Self status: "
                      << d_clusterData_p->membership().selfNodeStatus();

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_NOT_READY;
        status.code()     = -1;
        status.message()  = "Node is not available.";
        d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);
        return;  // RETURN
    }

    d_storageManager_p->processPartitionSyncDataRequestStatus(message, source);
}

void ClusterOrchestrator::processPrimaryStatusAdvisory(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    // This routine is invoked when the status of a primary 'source' node has
    // changed.

    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .isPrimaryStatusAdvisoryValue());

    const bmqp_ctrlmsg::PrimaryStatusAdvisory& primaryAdv =
        message.choice().clusterMessage().choice().primaryStatusAdvisory();

    if (d_clusterData_p->membership().selfNodeStatus() ==
        bmqp_ctrlmsg::NodeStatus::E_STOPPING) {
        // No need to process the advisory since self is stopping.
        BALL_LOG_INFO << d_cluster_p->description() << " Partition ["
                      << primaryAdv.partitionId() << "]: "
                      << "Not processing primary status advisory since"
                      << " self is stopping.";
        return;  // RETURN
    }

    if (0 > primaryAdv.partitionId() ||
        static_cast<size_t>(primaryAdv.partitionId()) >=
            clusterState()->partitions().size()) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": invalid partitionId in message: " << primaryAdv
                       << ", from: " << source->nodeDescription();
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED == primaryAdv.status()) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": received unexpected status in primary advisory: "
                       << primaryAdv
                       << ", from: " << source->nodeDescription();
        return;  // RETURN
    }

    const mqbc::ClusterStatePartitionInfo& pinfo = clusterState()->partition(
        primaryAdv.partitionId());

    mqbc::ClusterNodeSession* ns =
        d_clusterData_p->membership().getClusterNodeSession(source);
    BSLS_ASSERT_SAFE(ns);

    if (d_clusterConfig.clusterAttributes().isFSMWorkflow()) {
        if (pinfo.primaryNode() != source ||
            pinfo.primaryLeaseId() != primaryAdv.primaryLeaseId()) {
            BALL_LOG_WARN_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM
                    << d_clusterData_p->identity().description()
                    << ": Partition [" << primaryAdv.partitionId()
                    << "]: received primary status advisory: " << primaryAdv
                    << " from: " << source->nodeDescription()
                    << ", but self perceived primary and its leaseId are"
                    << ": ["
                    << (pinfo.primaryNode()
                            ? pinfo.primaryNode()->nodeDescription()
                            : "** null **")
                    << ", " << pinfo.primaryLeaseId() << "].";
                if (pinfo.primaryNode()) {
                    BALL_LOG_OUTPUT_STREAM << " Ignoring advisory.";
                }
                else {
                    BALL_LOG_OUTPUT_STREAM << " Since we have not received any"
                                           << " information regarding the true"
                                           << " primary, this advisory could "
                                           << "be from the true one. Will"
                                           << " buffer the advisory for now.";
                    d_storageManager_p->bufferPrimaryStatusAdvisory(primaryAdv,
                                                                    source);
                }
            }
            return;  // RETURN
        }
    }
    else if (!d_stateManager_mp->isFirstLeaderAdvisory()) {
        // Self node has heard from the leader at least once.  Perform
        // additional validations.

        if (pinfo.primaryNode() != source) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << ": Partition [" << primaryAdv.partitionId()
                           << "]: received primary status advisory: "
                           << primaryAdv
                           << " from: " << source->nodeDescription()
                           << ", but current primary is: "
                           << (pinfo.primaryNode()
                                   ? pinfo.primaryNode()->nodeDescription()
                                   : "** null **");
            return;  // RETURN
        }

        if (pinfo.primaryLeaseId() != primaryAdv.primaryLeaseId()) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << ": Partition [" << primaryAdv.partitionId()
                           << "]: received primary status advisory: "
                           << primaryAdv << " from perceived primary: "
                           << source->nodeDescription()
                           << ", but with different leaseId. Self perceived "
                           << "leaseId: " << pinfo.primaryLeaseId();
            return;  // RETURN
        }
    }
    else {
        // TODO Remove `mqbi::ClusterStateManager::setPrimary()` when this code
        // is removed.

        // Self node has not heard from the leader until now.  We update the
        // ClusterState as well as forward the advisory to StorageMgr, but log
        // a warning.  This scenario is possible if self node has just come up,
        // and hasn't heard from the leader, but various primary nodes have
        // sent their status advisory messages to it.

        // Note that we cannot use self node's status in place of
        // 'isFirstLeaderAdvisory', because a node may transition from
        // STARTING to AVAILABLE, but still may not have heard from the leader.

        // Also note that self node could be receiving the 2nd primary status
        // advisory from the 'source' (recall that a primary sends status
        // advisory when it sees a new node transitioning to STARTING and again
        // when transitioning to AVAILABLE), but self node may not have yet
        // heard from the leader.  So if self's cluster state is already
        // up-to-date with this primary's status, we don't assert certain
        // things.

        // TBD: Since we are updating cluster state based on a message from the
        // non-leader node, we are breaking the contract that only leader
        // issues writes to the cluster state.  This needs to be reviewed.  See
        // 'StorageMgr::processPrimaryStatusAdvisoryDispatched' as well.

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << primaryAdv.partitionId()
                      << "]: received primary status advisory: " << primaryAdv
                      << ", from: " << source->nodeDescription()
                      << ", before receiving leader advisory. Current "
                      << "partition primary: "
                      << (pinfo.primaryNode()
                              ? pinfo.primaryNode()->nodeDescription()
                              : "** null **")
                      << ". Self node status: "
                      << d_clusterData_p->membership().selfNodeStatus();

        if (pinfo.primaryNode() == source) {
            // Self node is receiving primary status advisory the second time.

            BSLS_ASSERT_SAFE(pinfo.primaryLeaseId() ==
                             primaryAdv.primaryLeaseId());

            BSLS_ASSERT_SAFE(
                ns->isPrimaryForPartition(primaryAdv.partitionId()));
        }
        else {
            if (!d_cluster_p->isCSLModeEnabled()) {
                ns->addPartitionRaw(primaryAdv.partitionId());
            }
            d_stateManager_mp->setPrimary(primaryAdv.partitionId(),
                                          primaryAdv.primaryLeaseId(),
                                          source);
        }
    }

    // In any case, update primary status and inform the storageMgr.

    // TBD: may need to review the order of invoking these routines.

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " PartitionId [" << primaryAdv.partitionId()
                  << "]: received primary status advisory: " << primaryAdv
                  << ", from: " << source->nodeDescription();

    BSLS_ASSERT_SAFE(ns->isPrimaryForPartition(primaryAdv.partitionId()));
    d_stateManager_mp->setPrimaryStatus(primaryAdv.partitionId(),
                                        primaryAdv.status());

    if (!d_clusterConfig.clusterAttributes().isFSMWorkflow()) {
        d_storageManager_p->processPrimaryStatusAdvisory(primaryAdv, source);
    }
}

void ClusterOrchestrator::processStateNotification(
    const bmqp_ctrlmsg::ControlMessage& notification,
    mqbnet::ClusterNode*                notifier)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(notification.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(notification.choice()
                         .clusterMessage()
                         .choice()
                         .isStateNotificationValue());
    BSLS_ASSERT_SAFE(notifier);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Received state notification: " << notification
                  << ", from peer node '" << notifier->nodeDescription()
                  << "'.";

    if (d_clusterData_p->membership().selfNodeStatus() !=
        bmqp_ctrlmsg::NodeStatus::E_AVAILABLE) {
        // For now, no state notification msgs are processed if self is not
        // AVAILABLE.

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Received state notification: " << notification
                      << ", from peer node '" << notifier->nodeDescription()
                      << "' but self node is not AVAILABLE. Current status ["
                      << d_clusterData_p->membership().selfNodeStatus()
                      << "]. Not responding at this time.";
        return;  // RETURN
    }

    const bmqp_ctrlmsg::StateNotification& stateNotification =
        notification.choice().clusterMessage().choice().stateNotification();
    switch (stateNotification.choice().selectionId()) {
    case bmqp_ctrlmsg::StateNotificationChoice::SELECTION_ID_LEADER_PASSIVE: {
        processLeaderPassiveNotification(stateNotification, notifier);
    } break;  // BREAK
    default: {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Unexpected state notification: " << notification
                      << ", from peer node '" << notifier->nodeDescription()
                      << "'";
        BSLS_ASSERT_SAFE(
            stateNotification.choice().selectionId() !=
            bmqp_ctrlmsg::StateNotificationChoice ::SELECTION_ID_UNDEFINED);
        // We should never receive an 'UNDEFINED' message, but while
        // introducing new features, we may receive unknown messages.
    }
    }
}

void ClusterOrchestrator::processLeaderPassiveNotification(
    const bmqp_ctrlmsg::StateNotification& notification,
    mqbnet::ClusterNode*                   notifier)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(notification.choice().isLeaderPassiveValue());
    BSLS_ASSERT_SAFE(notifier);

    if (d_clusterData_p->electorInfo().electorState() !=
        mqbnet::ElectorState::e_LEADER) {
        // This node may have gone from leader to non-leader.
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Received leader passive notification: "
                      << notification << ", from peer node '"
                      << notifier->nodeDescription()
                      << "' but self is no longer the leader. Self's "
                      << "elector state: "
                      << d_clusterData_p->electorInfo().electorState()
                      << ". Not sending leader advisory at this time.";
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_clusterData_p->membership().selfNode()->nodeId() ==
                     d_clusterData_p->electorInfo().leaderNodeId());

    if (d_clusterData_p->electorInfo().leaderStatus() !=
        mqbc::ElectorInfoLeaderStatus::e_ACTIVE) {
        // This node is the leader, but not an *ACTIVE* leader. Once it turns
        // active, it will do some state-changing logic and then send a leader
        // advisory. So no need to do anything now.
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Received leader passive notification: "
                      << notification << ", from peer node '"
                      << notifier->nodeDescription()
                      << "' but self is not an *ACTIVE* leader. "
                      << "leaderStatus: "
                      << d_clusterData_p->electorInfo().leaderStatus()
                      << ". Not sending leader advisory at this time.";
        return;  // RETURN
    }

    if (d_clusterData_p->membership().selfNodeStatus() ==
        bmqp_ctrlmsg::NodeStatus::E_STOPPING) {
        // This node is the ACTIVE leader, but is stopping.  So no need to do
        // anything now.
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Received leader passive notification: "
                      << notification << ", from peer node '"
                      << notifier->nodeDescription()
                      << "', self is  *ACTIVE* leader, but self is stopping. "
                      << "Not sending leader advisory at this time.";
        return;  // RETURN
    }

    // Self is an ACTIVE leader - broadcast 'LeaaderAdvisory'
    bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo> partitions;
    mqbc::ClusterUtil::loadPartitionsInfo(&partitions, *clusterState());
    d_stateManager_mp->sendClusterState(true,  // sendPartitionPrimaryInfo
                                        true,  // sendQueuesInfo
                                        0,
                                        partitions);
}

void ClusterOrchestrator::onRecoverySuccess()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
                     d_clusterData_p->membership().selfNodeStatus());

    // This routine is invoked by 'mqbblp::Cluster' when self node has
    // successfully recovered all partitions after startup, *and* has
    // transitioned to AVAILABLE.

    if (!d_clusterData_p->electorInfo().isSelfLeader()) {
        return;  // RETURN
    }

    if (mqbc::ElectorInfoLeaderStatus::e_PASSIVE !=
        d_clusterData_p->electorInfo().leaderStatus()) {
        return;  // RETURN
    }

    d_stateManager_mp->initiateLeaderSync(true);
}

void ClusterOrchestrator::validateClusterStateLedger()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->validateClusterStateLedger();
}

void ClusterOrchestrator::registerAppId(bsl::string         appId,
                                        const mqbi::Domain& domain)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->registerAppId(appId, &domain);
}

void ClusterOrchestrator::unregisterAppId(bsl::string         appId,
                                          const mqbi::Domain& domain)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    d_stateManager_mp->unregisterAppId(appId, &domain);
}

void ClusterOrchestrator::onPartitionPrimaryStatus(int          partitionId,
                                                   int          status,
                                                   unsigned int primaryLeaseId)
{
    // exected by *ANY* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(
            &ClusterOrchestrator::onPartitionPrimaryStatusDispatched,
            this,
            partitionId,
            status,
            primaryLeaseId),
        d_cluster_p);
}

int ClusterOrchestrator::processCommand(
    const mqbcmd::ClusterStateCommand& command,
    mqbcmd::ClusterResult*             result)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    if (command.isElectorValue()) {
        mqbcmd::ElectorResult electorResult;
        int                   rc = d_elector_mp->processCommand(&electorResult,
                                              command.elector());
        if (electorResult.isErrorValue()) {
            result->makeError(electorResult.error());
        }
        else {
            result->makeElectorResult(electorResult);
        }
        return rc;  // RETURN
    }

    bmqu::MemOutStream os;
    os << "Unknown command '" << command << "'";
    mqbcmd::Error& error = result->makeError();
    error.message()      = os.str();
    return -1;
}

}  // close package namespace
}  // close enterprise namespace
