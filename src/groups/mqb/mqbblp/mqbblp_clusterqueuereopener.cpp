// Copyright 2024-2025 Bloomberg Finance L.P.
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

// mqbblp_clusterqueuereopener.cpp                                    -*-C++-*-
#include <mqbblp_clusterqueuereopener.h>

#include <mqbscm_version.h>

// MQB
#include <mqbc_clusterdata.h>
#include <mqbc_clustermembership.h>
#include <mqbc_clusterstate.h>
#include <mqbc_electorinfo.h>
#include <mqbi_cluster.h>
#include <mqbi_clusterstatemanager.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbi_storage.h>
#include <mqbi_storagemanager.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>
#include <bmqp_requestmanager.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>
#include <bmqtsk_alarmlog.h>

// MWC
#include <bmqu_memoutstream.h>
#include <bmqu_time.h>

// BDE
#include <ball_log.h>
#include <ball_logthrottle.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bdlt_timeunitratio.h>
#include <bsl_vector.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

const int k_MAX_INSTANT_MESSAGES = 10;

const bsls::Types::Int64 k_NS_PER_MESSAGE =
    bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MINUTE / k_MAX_INSTANT_MESSAGES;

#define BMQ_LOGTHROTTLE_INFO                                                  \
    BALL_LOGTHROTTLE_INFO(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)           \
        << "[THROTTLED] "

#define BMQ_LOGTHROTTLE_WARN                                                  \
    BALL_LOGTHROTTLE_WARN(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)           \
        << "[THROTTLED] "

#define BMQ_LOGTHROTTLE_ERROR                                                 \
    BALL_LOGTHROTTLE_ERROR(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)          \
        << "[THROTTLED] "

template <typename T>
struct ConditionalAdvance {
    bool d_doAdvance;

    ConditionalAdvance()
    : d_doAdvance(true)
    {
    }

    void advance(T& x)
    {
        if (d_doAdvance) {
            ++x;
        }
        else {
            d_doAdvance = true;
        }
    }
    void release() { d_doAdvance = false; }
};

}  // close unnamed namespace

// --------------------------
// class ClusterQueueReopener
// --------------------------

// CREATORS
ClusterQueueReopener::ClusterQueueReopener(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_queueHelper_p(0)
, d_clusterData_p(0)
, d_clusterState_p(0)
, d_cluster_p(0)
, d_reopenCycles(allocator)
{
    // NOTHING
}

ClusterQueueReopener::~ClusterQueueReopener()
{
    // NOTHING
}

// MANIPULATORS
void ClusterQueueReopener::initialize(ClusterQueueHelper* queueHelper,
                                      mqbc::ClusterData*  clusterData,
                                      mqbc::ClusterState* clusterState,
                                      mqbi::Cluster*      cluster)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueHelper);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(cluster);

    d_queueHelper_p  = queueHelper;
    d_clusterData_p  = clusterData;
    d_clusterState_p = clusterState;
    d_cluster_p      = cluster;
}

void ClusterQueueReopener::teardown()
{
    d_reopenCycles.clear();
    d_queueHelper_p  = 0;
    d_clusterData_p  = 0;
    d_clusterState_p = 0;
    d_cluster_p      = 0;
}

void ClusterQueueReopener::restoreState(int partitionId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(mqbi::Storage::k_INVALID_PARTITION_ID != partitionId);

    if (d_cluster_p->isRemote()) {
        restoreStateRemote();
    }
    else {
        restoreStateCluster(partitionId);
    }
}

void ClusterQueueReopener::restoreStateRemote()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(d_cluster_p->isRemote());

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Received state-restore event.";

    if (!d_clusterData_p->electorInfo().hasActiveLeader()) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Not going ahead with state restore since there is "
                      << "no active leader.";
        return;  // RETURN
    }

    bsls::Types::Uint64 generationCount =
        d_clusterData_p->electorInfo().electorTerm();

    bsl::shared_ptr<PartitionReopenCycle> cycle =
        startPartitionReopen(0, generationCount);

    ConditionalAdvance<QueueContextMapConstIter> conditional;

    for (QueueContextMapConstIter cit = d_queueHelper_p->d_queues.cbegin();
         cit != d_queueHelper_p->d_queues.cend();
         conditional.advance(cit)) {
        const QueueContextSp& queueContext = cit->second;
        QueueLiveState&       liveQInfo    = queueContext->d_liveQInfo;

        if (!liveQInfo.d_queue_sp && liveQInfo.d_inFlight == 0) {
            BMQ_LOGTHROTTLE_INFO << d_cluster_p->description()
                                 << ": Not performing restore of queue ["
                                 << queueContext->uri() << "].";
            continue;  // CONTINUE
        }

        if (!d_queueHelper_p->isQueueAssigned(*queueContext)) {
            if (!d_queueHelper_p->assignQueue(queueContext)) {
                conditional.release();
                cit = d_queueHelper_p->d_queues.erase(cit);
            }

            continue;  // CONTINUE
        }

        if (liveQInfo.d_queue_sp) {
            const bmqt::GenericResult::Enum rc = restoreStateHelper(
                queueContext.get(),
                d_clusterData_p->electorInfo().leaderNode(),
                cycle);

            if (rc == bmqt::GenericResult::e_NOT_CONNECTED) {
                return;  // RETURN
            }
        }

        d_queueHelper_p->onQueueContextAssigned(queueContext);
    }
}

void ClusterQueueReopener::restoreStateCluster(int partitionId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(mqbi::Storage::k_INVALID_PARTITION_ID != partitionId);

    const bool allPartitions = (mqbi::Storage::k_ANY_PARTITION_ID ==
                                partitionId);

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM
            << d_cluster_p->description()
            << ": Received state-restore event for Partition [";
        if (allPartitions) {
            BALL_LOG_OUTPUT_STREAM << "ALL";
        }
        else {
            BALL_LOG_OUTPUT_STREAM << partitionId;
        }
        BALL_LOG_OUTPUT_STREAM << "].";
    }

    if (d_clusterData_p->membership().selfNodeStatus() !=
        bmqp_ctrlmsg::NodeStatus::E_AVAILABLE) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Not going ahead with restoring partition state "
                      << "because self is not AVAILABLE.  Self status: "
                      << d_clusterData_p->membership().selfNodeStatus();
        return;  // RETURN
    }

    // TODO: revisit
    if (!d_clusterData_p->electorInfo().hasActiveLeader() &&
        (allPartitions || d_cluster_p->isFSMWorkflow())) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Not going ahead with restoring partition state "
                      << "because there is no leader or leader isn't active. "
                      << "Current leader: "
                      << (d_clusterData_p->electorInfo().leaderNode()
                              ? d_clusterData_p->electorInfo()
                                    .leaderNode()
                                    ->nodeDescription()
                              : "** null **")
                      << ", leader status: "
                      << d_clusterData_p->electorInfo().leaderStatus();
        return;  // RETURN
    }

    bool                                   isSelfPrimaryAndLeader = false;
    const mqbc::ClusterStatePartitionInfo* pinfo                  = 0;

    if (!allPartitions) {
        pinfo = &(d_clusterState_p->partition(partitionId));
        BSLS_ASSERT_SAFE(pinfo);
        if (!d_queueHelper_p->hasActiveAvailablePrimary(partitionId)) {
            BALL_LOG_INFO << d_cluster_p->description() << " Partition ["
                          << partitionId
                          << "]: Not restoring partition state because there "
                          << "is no ACTIVE and AVAIALBLE primary. Current "
                          << "primary: "
                          << (pinfo->primaryNode()
                                  ? pinfo->primaryNode()->nodeDescription()
                                  : "** null **")
                          << ", primary status: " << pinfo->primaryStatus();
            return;  // RETURN
        }

        isSelfPrimaryAndLeader =
            pinfo->primaryNode() == d_clusterData_p->membership().selfNode() &&
            d_clusterData_p->electorInfo().isSelfLeader();
    }

    /// TODO (FSM); remove after switching to FSM
    if (!d_cluster_p->isFSMWorkflow() && isSelfPrimaryAndLeader) {
        mqbc::ClusterState::AssignmentVisitor doubleAssignmentVisitor =
            bdlf::BindUtil::bindS(d_allocator_p,
                                  &mqbi::StorageManager::unregisterQueue,
                                  d_queueHelper_p->d_storageManager_p,
                                  bdlf::PlaceHolders::_1,
                                  bdlf::PlaceHolders::_2);

        d_clusterState_p->iterateDoubleAssignments(partitionId,
                                                   doubleAssignmentVisitor);
    }
    ConditionalAdvance<QueueContextMapConstIter> conditional;
    for (QueueContextMapConstIter cit = d_queueHelper_p->d_queues.cbegin();
         cit != d_queueHelper_p->d_queues.cend();
         conditional.advance(cit)) {
        const QueueContextSp& queueContext = cit->second;
        QueueLiveState&       liveQInfo    = queueContext->d_liveQInfo;
        if (allPartitions) {
            if (!liveQInfo.d_queue_sp && liveQInfo.d_inFlight == 0) {
                BMQ_LOGTHROTTLE_INFO << d_cluster_p->description()
                                     << " Not performing restore of queue "
                                     << "[" << queueContext->uri() << "].";
                continue;  // CONTINUE
            }

            if (!d_queueHelper_p->isQueueAssigned(*queueContext)) {
                if (!d_queueHelper_p->assignQueue(queueContext)) {
                    conditional.release();
                    cit = d_queueHelper_p->d_queues.erase(cit);
                }

                continue;  // CONTINUE
            }

            partitionId = queueContext->partitionId();
            pinfo       = &(d_clusterState_p->partition(partitionId));

            if (!d_queueHelper_p->hasActiveAvailablePrimary(partitionId)) {
                BMQ_LOGTHROTTLE_INFO
                    << d_cluster_p->description()
                    << " Not performing restore of queue ["
                    << queueContext->uri()
                    << "] because there is no primary or primary isn't "
                       "ACTIVE. Current primary: "
                    << (pinfo->primaryNode()
                            ? pinfo->primaryNode()->nodeDescription()
                            : "** null **")
                    << ", primary status: " << pinfo->primaryStatus();
                continue;  // CONTINUE
            }
            isSelfPrimaryAndLeader =
                pinfo->primaryNode() ==
                    d_clusterData_p->membership().selfNode() &&
                d_clusterData_p->electorInfo().isSelfLeader();
        }
        else if (queueContext->partitionId() != partitionId) {
            continue;  // CONTINUE;
        }

        BSLS_ASSERT_SAFE(d_queueHelper_p->isQueueAssigned(*queueContext));
        BSLS_ASSERT_SAFE(
            d_queueHelper_p->isQueuePrimaryAvailable(*queueContext));

        bsl::shared_ptr<PartitionReopenCycle> cycle =
            startPartitionReopen(partitionId, pinfo->primaryLeaseId());

        if (liveQInfo.d_queue_sp) {
            if (isSelfPrimaryAndLeader) {
                bsl::vector<bsl::string> added(d_allocator_p);
                bsl::vector<bsl::string> removed(d_allocator_p);
                mqbi::Domain* domain = liveQInfo.d_queue_sp->domain();

                d_queueHelper_p->match(&added,
                                       &removed,
                                       *queueContext->d_stateQInfo_sp,
                                       domain->config()->mode());

                if (!removed.empty() || !added.empty()) {
                    ClusterQueueHelper::VoidFunctor park =
                        bdlf::BindUtil::bindS(
                            d_allocator_p,
                            &ClusterQueueHelper::convertToLocal,
                            d_queueHelper_p,
                            queueContext,
                            domain);

                    liveQInfo.d_pendingUpdates.push_back(park);

                    mqbi::ClusterErrorCode::Enum result =
                        d_queueHelper_p->d_clusterStateManager_p
                            ->updateAppIds(added, removed, domain->name(), "");

                    if (mqbi::ClusterErrorCode::e_OK == result) {
                        continue;  // CONTINUE
                    }

                    BSLS_ASSERT_SAFE(
                        false &&
                        "Failure to update Apps before convertToLocal");
                }
                else {
                    d_queueHelper_p->convertToLocal(queueContext, domain);
                }
            }
            else {
                if (queueContext->d_liveQInfo.d_numQueueHandles != 0) {
                    const bmqt::GenericResult::Enum rc = restoreStateHelper(
                        queueContext.get(),
                        pinfo->primaryNode(),
                        cycle);

                    if (rc == bmqt::GenericResult::e_NOT_CONNECTED) {
                        return;  // RETURN
                    }
                }
                else {
                    BMQ_LOGTHROTTLE_INFO
                        << d_cluster_p->description()
                        << ": Skipping restore of " << queueContext->uri()
                        << " because it has no active queue handles";

                    d_queueHelper_p->setStreamState(queueContext,
                                                    SubQueueContext::k_OPEN);
                }
            }
        }

        d_queueHelper_p->onQueueContextAssigned(queueContext);
    }
}

bmqt::GenericResult::Enum ClusterQueueReopener::restoreStateHelper(
    QueueContext*                                queueContext,
    mqbnet::ClusterNode*                         activeNode,
    const bsl::shared_ptr<PartitionReopenCycle>& cycle)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(queueContext);
    BSLS_ASSERT_SAFE(activeNode);

    QueueLiveState&           queueInfo = queueContext->d_liveQInfo;
    bmqt::GenericResult::Enum rc        = bmqt::GenericResult::e_SUCCESS;
    const mqbi::Queue*        queuePtr  = queueInfo.d_queue_sp.get();

    BSLS_ASSERT_SAFE(queuePtr);

    const bsls::Types::Uint64 generationCount = cycle->generationCount();

    for (StreamsMap::iterator iter = queueInfo.d_subQueueIds.begin();
         iter != queueInfo.d_subQueueIds.end();
         ++iter) {
        SubQueueContext& subQueueContext = iter->value();

        const bmqp_ctrlmsg::QueueHandleParameters& parameters =
            subQueueContext.d_parameters;

        if (bmqp::QueueUtil::isEmpty(parameters)) {
            BMQ_LOGTHROTTLE_INFO
                << "#INVALID_REOPENQUEUE_REQ " << d_cluster_p->description()
                << ": Not sending ReopenQueue request to "
                << activeNode->nodeDescription()
                << "[parameters: " << parameters
                << ", reason: 'All read,write,admin counts are <= 0]";

            d_queueHelper_p->setStreamState(&subQueueContext,
                                            SubQueueContext::k_OPEN);
            continue;  // CONTINUE
        }
        const SubQueueContext::Enum state = subQueueContext.d_state;
        if (subQueueContext.d_generationCount == generationCount) {
            if (state == SubQueueContext::k_REOPENING ||
                state == SubQueueContext::k_OPEN ||
                state == SubQueueContext::k_FAILED) {
                BMQ_LOGTHROTTLE_INFO
                    << d_cluster_p->description()
                    << ": Not sending ReopenQueue request to "
                    << activeNode->nodeDescription()
                    << "[parameters: " << parameters << ", state: " << state
                    << ", reason: the generationCount is the same]";
                continue;  // CONTINUE
            }
        }

        d_queueHelper_p->setStreamState(&subQueueContext,
                                        SubQueueContext::k_CLOSED);

        BSLS_ASSERT_SAFE(subQueueContext.d_numOpenRequestsInFlight == 0);

        rc = sendReopenQueueRequest(queueContext,
                                    &subQueueContext,
                                    activeNode,
                                    cycle,
                                    1);
    }

    return rc;
}

bmqt::GenericResult::Enum ClusterQueueReopener::sendReopenQueueRequest(
    QueueContext*                                queueContext,
    SubQueueContext*                             subQueueContext,
    mqbnet::ClusterNode*                         activeNode,
    const bsl::shared_ptr<PartitionReopenCycle>& cycle,
    int                                          numAttempts)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());

    BSLS_ASSERT_SAFE(subQueueContext);
    BSLS_ASSERT_SAFE(queueContext);
    BSLS_ASSERT_SAFE(activeNode);
    BSLS_ASSERT_SAFE(cycle);

    RequestManagerType::RequestSp request =
        d_cluster_p->requestManager().createRequest();
    bmqp_ctrlmsg::OpenQueue& openQueue =
        request->request().choice().makeOpenQueue();

    openQueue.handleParameters()       = subQueueContext->d_parameters;
    openQueue.handleParameters().qId() = queueContext->d_liveQInfo.d_id;

    bmqt::GenericResult::Enum rc = bmqt::GenericResult::e_SUCCESS;

    if (bmqp::QueueUtil::isEmpty(openQueue.handleParameters())) {
        BMQ_LOGTHROTTLE_INFO
            << "#INVALID_REOPENQUEUE_REQ " << d_cluster_p->description()
            << ": Not sending ReopenQueueRequest to "
            << activeNode->nodeDescription()
            << "[request: " << request->request()
            << ", reason: 'All read,write,admin counts are <= 0]";

        rc = bmqt::GenericResult::e_INVALID_ARGUMENT;
    }
    else {
        request->setResponseCb(
            bdlf::BindUtil::bindS(d_allocator_p,
                                  &ClusterQueueReopener::onReopenQueueResponse,
                                  this,
                                  bdlf::PlaceHolders::_1,
                                  activeNode,
                                  cycle,
                                  numAttempts));

        bsls::TimeInterval timeoutMs;
        timeoutMs.setTotalMilliseconds(d_clusterData_p->clusterConfig()
                                           .queueOperations()
                                           .reopenTimeoutMs());
        rc = d_cluster_p->sendRequest(request, activeNode, timeoutMs);
    }
    if (rc == bmqt::GenericResult::e_SUCCESS) {
        const bsls::Types::Uint64 generationCount = cycle->generationCount();

        BMQ_LOGTHROTTLE_INFO << "Sent ReopenQueue request "
                             << request->request() << " generationCount "
                             << generationCount;
        d_queueHelper_p->setStreamState(subQueueContext,
                                        SubQueueContext::k_REOPENING);
        subQueueContext->d_generationCount = generationCount;

        ++queueContext->d_liveQInfo.d_numReopenQueueRequests;
    }
    else {
        BMQ_LOGTHROTTLE_ERROR
            << d_cluster_p->description() << ": Error while sending "
            << "ReopenQueue request: " << request->request() << ", rc: " << rc
            << ".";

        d_queueHelper_p->setStreamState(subQueueContext,
                                        SubQueueContext::k_CLOSED);
    }
    return rc;
}

void ClusterQueueReopener::onReopenQueueResponse(
    const RequestManagerType::RequestSp&         requestContext,
    mqbnet::ClusterNode*                         activeNode,
    const bsl::shared_ptr<PartitionReopenCycle>& cycle,
    int                                          numAttempts)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(requestContext->request().choice().isOpenQueueValue());
    BSLS_ASSERT_SAFE(cycle);
    BSLS_ASSERT_SAFE(0 < numAttempts);

    BMQ_LOGTHROTTLE_INFO << d_cluster_p->description()
                         << ": Processing ReopenQueue "
                         << "response [attemptNumber: " << numAttempts
                         << ", request: " << requestContext->request()
                         << ", response: " << requestContext->response()
                         << "]";

    const bmqp_ctrlmsg::OpenQueue& req =
        requestContext->request().choice().openQueue();
    const bmqp_ctrlmsg::QueueHandleParameters& reqParameters =
        req.handleParameters();
    const bmqt::Uri uri(reqParameters.uri());

    QueueContextMapIter it = d_queueHelper_p->d_queues.find(uri.canonical());
    if (it == d_queueHelper_p->d_queues.end()) {
        BMQ_LOGTHROTTLE_WARN
            << d_cluster_p->description()
            << ": ignoring ReopenQueueResponse for queue as it"
            << " no longer exists in the cluster state. Queue [" << uri
            << "], response: " << requestContext->response();

        return;  // RETURN
    }

    QueueContext*      queueContext = it->second.get();
    QueueLiveState&    qinfo        = queueContext->d_liveQInfo;
    const bsl::string  appId = bmqp::QueueUtil::extractAppId(reqParameters);
    const unsigned int upstreamSubQueueId = bmqp::QueueUtil::extractSubQueueId(
        reqParameters);
    StreamsMap::iterator sqit = qinfo.d_subQueueIds.findBySubIdSafe(
        upstreamSubQueueId);

    if (sqit == qinfo.d_subQueueIds.end()) {
        BMQ_LOGTHROTTLE_WARN
            << d_cluster_p->description()
            << ": ignoring ReopenQueueResponse for subQueue as it"
            << " no longer exists in the queue state. [uri: " << uri
            << ", upstreamSubQueueId: " << upstreamSubQueueId
            << "], response: " << requestContext->response();

        return;  // RETURN
    }

    SubQueueContext&          subQueueContext = sqit->value();
    const bsls::Types::Uint64 generationCount = cycle->generationCount();

    if (subQueueContext.d_generationCount != generationCount) {
        BMQ_LOGTHROTTLE_WARN
            << d_cluster_p->description()
            << ": ignoring stale ReopenQueueResponse for uri '" << uri
            << "' with upstreamSubQueueId: [" << upstreamSubQueueId
            << "], response: " << requestContext->response();

        return;  // RETURN
    }

    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bindS(d_allocator_p,
                              &ClusterQueueReopener::finishReopening,
                              this,
                              queueContext,
                              sqit,
                              activeNode,
                              cycle));

    if (bmqt::GenericResult::e_SUCCESS != requestContext->result()) {
        d_queueHelper_p->setStreamState(&subQueueContext,
                                        SubQueueContext::k_CLOSED);

        if (bmqt::GenericResult::e_CANCELED == requestContext->result()) {
            cycle->setAsFailed();

            return;  // RETURN
        }

        if (!d_cluster_p->isRemote() ||
            d_clusterData_p->clusterConfig()
                    .queueOperations()
                    .reopenMaxAttempts() == numAttempts) {
            BMQTSK_ALARMLOG_ALARM("QUEUE_REOPEN_FAILURE")
                << d_cluster_p->description()
                << ": error while reopening queue [" << req
                << ", response: " << requestContext->response() << "]"
                << BMQTSK_ALARMLOG_END;

            BSLS_ASSERT_SAFE(sqit != qinfo.d_subQueueIds.end());
            BSLS_ASSERT_SAFE(sqit->appId() == appId);

            d_queueHelper_p->setStreamState(&subQueueContext,
                                            SubQueueContext::k_FAILED);

            return;  // RETURN
        }

        if (d_cluster_p->isStopping()) {
            BMQ_LOGTHROTTLE_INFO
                << d_cluster_p->description()
                << ": Not retrying ReopenQueue  [reason: 'stopping'"
                << ", request: " << requestContext->request()
                << ", response: " << requestContext->response() << "]";

            return;  // RETURN
        }

        BMQ_LOGTHROTTLE_WARN
            << d_cluster_p->description()
            << ": queue reopen-request failed. Request: "
            << requestContext->request()
            << ", error response: " << requestContext->response()
            << ". Attempt number was: " << numAttempts
            << ". Attempting again after "
            << d_clusterData_p->clusterConfig()
                   .queueOperations()
                   .reopenRetryIntervalMs()
            << " milliseconds.";

        bsls::TimeInterval after(bmqu::Time::nowMonotonicClock());
        after.addMilliseconds(d_clusterData_p->clusterConfig()
                                  .queueOperations()
                                  .reopenRetryIntervalMs());

        d_clusterData_p->scheduler().scheduleEvent(
            &subQueueContext.d_reopenRetryHandle,
            after,
            bdlf::BindUtil::bindS(d_allocator_p,
                                  &ClusterQueueReopener::onReopenQueueRetry,
                                  this,
                                  requestContext,
                                  activeNode,
                                  cycle,
                                  numAttempts));
        return;  // RETURN
    }

    if (d_cluster_p->isStopping()) {
        BMQ_LOGTHROTTLE_WARN
            << d_cluster_p->description()
            << ": not sending a configure-queue request in response "
            << "to ReopenQueue response, for queue [" << uri
            << "], as self is stopping.";

        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(
        requestContext->response().choice().isOpenQueueResponseValue());
    BSLS_ASSERT_SAFE(sqit != qinfo.d_subQueueIds.end());
    BSLS_ASSERT_SAFE(appId == sqit->appId());

    BSLS_ASSERT_SAFE(subQueueContext.d_state == SubQueueContext::k_REOPENING);

    d_queueHelper_p->setStreamState(&subQueueContext, SubQueueContext::k_OPEN);

    BMQ_LOGTHROTTLE_INFO << d_cluster_p->description()
                         << ": queue successfully reopened ["
                         << requestContext->request() << "].";
}

void ClusterQueueReopener::onReopenQueueRetry(
    const RequestManagerType::RequestSp&         requestContext,
    mqbnet::ClusterNode*                         activeNode,
    const bsl::shared_ptr<PartitionReopenCycle>& cycle,
    int                                          numAttempts)
{
    // executed by *SCHEDULER* thread

    if (d_cluster_p->isStopping()) {
        return;  // RETURN
    }

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bindS(
            d_allocator_p,
            &ClusterQueueReopener::onReopenQueueRetryDispatched,
            this,
            requestContext,
            activeNode,
            cycle,
            numAttempts),
        d_cluster_p);
}

void ClusterQueueReopener::onReopenQueueRetryDispatched(
    const RequestManagerType::RequestSp&         requestContext,
    mqbnet::ClusterNode*                         activeNode,
    const bsl::shared_ptr<PartitionReopenCycle>& cycle,
    int                                          numAttempts)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(activeNode);
    BSLS_ASSERT_SAFE(cycle);
    BSLS_ASSERT_SAFE(0 < numAttempts);

    if (d_cluster_p->isStopping()) {
        return;  // RETURN
    }
    const bsls::Types::Uint64 generationCount = cycle->generationCount();

    if (activeNode != d_clusterData_p->electorInfo().leaderNode() ||
        generationCount != d_clusterData_p->electorInfo().electorTerm()) {
        BMQ_LOGTHROTTLE_WARN
            << "#STALE_ACTIVE " << d_clusterData_p->identity().description()
            << ": not retrying ReopenQueue request as the upstream/active has "
            << "changed [requestActiveNode: "
            << (activeNode ? activeNode->nodeDescription() : "** null **")
            << ":" << generationCount << ", currentActiveNode: "
            << (d_clusterData_p->electorInfo().leaderNode()
                    ? d_clusterData_p->electorInfo()
                          .leaderNode()
                          ->nodeDescription()
                    : "** null **")
            << ":" << d_clusterData_p->electorInfo().electorTerm()
            << ", request: " << requestContext->request() << "]";

        return;  // RETURN
    }

    const bmqp_ctrlmsg::QueueHandleParameters& reqParameters =
        requestContext->request().choice().openQueue().handleParameters();
    const bsl::string&       uriStr = reqParameters.uri();
    bmqt::Uri                uri(uriStr);
    QueueContextMapConstIter it = d_queueHelper_p->d_queues.find(
        uri.canonical());
    if (it == d_queueHelper_p->d_queues.end()) {
        BMQ_LOGTHROTTLE_WARN
            << d_cluster_p->description()
            << ": not retrying ReopenQueue request again for queue [" << uri
            << "], as queue doesn't exist in cluster state.";

        return;  // RETURN
    }

    const QueueContextSp& queueContext = it->second;
    if (!queueContext->d_liveQInfo.d_queue_sp) {
        BMQ_LOGTHROTTLE_WARN
            << d_cluster_p->description()
            << ": not retrying ReopenQueue request for queue [" << uri
            << "], as queue instance has been deleted.";

        return;  // RETURN
    }

    if (!d_queueHelper_p->isQueueAssigned(*queueContext.get())) {
        BMQ_LOGTHROTTLE_WARN
            << d_cluster_p->description()
            << ": not retrying ReopenQueue request for queue [" << uri
            << "], as queue is not assigned.";

        return;  // RETURN
    }
    const unsigned int upstreamSubQueueId = bmqp::QueueUtil::extractSubQueueId(
        reqParameters);
    QueueLiveState&      qinfo = queueContext->d_liveQInfo;
    StreamsMap::iterator sqit  = qinfo.d_subQueueIds.findBySubIdSafe(
        upstreamSubQueueId);

    if (sqit == qinfo.d_subQueueIds.end()) {
        BMQ_LOGTHROTTLE_WARN
            << d_cluster_p->description()
            << ": not retrying ReopenQueue request as it"
            << " no longer exists in the queue state. [uri: " << uri
            << ", upstreamSubQueueId: " << upstreamSubQueueId << "]";

        return;  // RETURN
    }

    SubQueueContext& subQueueContext = sqit->value();

    if (subQueueContext.d_state == SubQueueContext::k_CLOSED) {
        sendReopenQueueRequest(queueContext.get(),
                               &subQueueContext,
                               activeNode,
                               cycle,
                               numAttempts + 1);
    }
}

void ClusterQueueReopener::finishReopening(
    QueueContext*                                queueContext,
    StreamsMap::iterator                         sqit,
    mqbnet::ClusterNode*                         activeNode,
    const bsl::shared_ptr<PartitionReopenCycle>& cycle)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());

    SubQueueContext&      subQueueContext    = sqit->value();
    SubQueueContext::Enum state              = subQueueContext.d_state;
    bool                  isValid            = true;
    const unsigned int    upstreamSubQueueId = sqit->subId();

    bsl::vector<SubQueueContext::PendingClose> pendingClose(d_allocator_p);
    pendingClose.swap(subQueueContext.d_pendingCloseRequests);
    for (size_t i = 0; i < pendingClose.size(); ++i) {
        if (state == SubQueueContext::k_OPEN && isValid) {
            BMQ_LOGTHROTTLE_INFO
                << d_cluster_p->description()
                << ": sending pending Close request with parameters ["
                << pendingClose[i].d_handleParameters << "].";

            d_queueHelper_p->sendCloseQueueRequest(
                pendingClose[i].d_handleParameters,
                sqit,
                queueContext->partitionId(),
                pendingClose[i].d_callback);
        }
        else {
            BMQ_LOGTHROTTLE_WARN
                << d_cluster_p->description()
                << ": not sending excessive pending Close request"
                << " with parameters [" << pendingClose[i].d_handleParameters
                << "].";

            if (pendingClose[i].d_callback) {
                bmqp_ctrlmsg::Status status;

                status.category() =
                    bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
                status.code() = -1;
                status.message() =
                    "Attempting to release queue handle for invalid stream.";

                pendingClose[i].d_callback(status);
            }
        }
        if (isValid) {
            isValid = d_queueHelper_p->subtractCounters(
                &queueContext->d_liveQInfo,
                pendingClose[i].d_handleParameters,
                sqit);
        }
    }
    const bsls::Types::Uint64 generationCount = cycle->generationCount();

    if (isValid && state == SubQueueContext::k_OPEN) {
        QueueLiveState& qinfo = queueContext->d_liveQInfo;

        mqbi::Queue*     queueptr = qinfo.d_queue_sp.get();
        const bmqt::Uri& uri      = queueContext->uri();

        BSLS_ASSERT_SAFE(queueptr);
        BSLS_ASSERT_SAFE(d_queueHelper_p->isQueueAssigned(*queueContext));

        bmqp_ctrlmsg::StreamParameters streamParamsCopy;

        // TODO: remove this not thread-safe use of 'getUpstreamParameters'.
        if (!queueptr->getUpstreamParameters(&streamParamsCopy,
                                             upstreamSubQueueId)) {
            ball::Severity::Level logSeverity = ball::Severity::e_WARN;

            if (bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID == upstreamSubQueueId) {
                logSeverity = ball::Severity::e_INFO;

                notifyQueue(queueContext,
                            bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID,
                            generationCount,
                            true);
            }
            BALL_LOGTHROTTLE_INFO_BLOCK(k_MAX_INSTANT_MESSAGES,
                                        k_NS_PER_MESSAGE)
            {
                BALL_LOG_STREAM(logSeverity)
                    << d_cluster_p->description()
                    << ": not sending a configure-queue request in response to"
                    << " ReopenQueue response, for queue [" << uri
                    << "], as the queue is not configured for upstream "
                       "subQueue "
                    << bmqp::QueueId::SubQueueIdInt(upstreamSubQueueId);
            }
        }
        else if (!d_queueHelper_p->sendConfigureQueueRequest(
                     streamParamsCopy,
                     queueptr->id(),
                     queueptr->uri(),
                     bdlf::BindUtil::bindS(
                         d_allocator_p,
                         &ClusterQueueReopener::reconfigureCallback,
                         this,
                         bdlf::PlaceHolders::_1,
                         bdlf::PlaceHolders::_2,
                         cycle),
                     true,
                     activeNode,
                     generationCount,
                     sqit->subId())) {
            state = SubQueueContext::k_CLOSED;
            d_queueHelper_p->setStreamState(&subQueueContext,
                                            SubQueueContext::k_CLOSED);
        }
    }
    if (state == SubQueueContext::k_FAILED) {
        notifyQueue(queueContext,
                    upstreamSubQueueId,
                    generationCount,
                    false,
                    false);
    }

    if (0 == --queueContext->d_liveQInfo.d_numReopenQueueRequests) {
        d_queueHelper_p->processPendingContexts(queueContext);
    }
}

void ClusterQueueReopener::notifyQueue(QueueContext*       queueContext,
                                       unsigned int        upstreamSubQueueId,
                                       bsls::Types::Uint64 generationCount,
                                       bool                isOpen,
                                       bool                isWriterOnly)
{
    mqbi::Queue* queue = queueContext->d_liveQInfo.d_queue_sp.get();
    if (queue == 0) {
        return;  // RETURN
    }

    if (isOpen) {
        if (generationCount == 0) {
            BMQ_LOGTHROTTLE_INFO << d_cluster_p->description()
                                 << ": has deconfigured queue ["
                                 << queueContext->uri() << "], subStream id ["
                                 << upstreamSubQueueId << "]";
        }
        else {
            queue->dispatcher()->execute(
                bdlf::BindUtil::bindS(d_allocator_p,
                                      &mqbi::Queue::onOpenUpstream,
                                      queue,
                                      generationCount,
                                      upstreamSubQueueId,
                                      isWriterOnly),
                queue);
        }
    }
    else {
        queue->dispatcher()->execute(
            bdlf::BindUtil::bindS(d_allocator_p,
                                  &mqbi::Queue::onOpenFailure,
                                  queue,
                                  upstreamSubQueueId),
            queue);
    }
}

void ClusterQueueReopener::reconfigureCallback(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::Status& status,
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::StreamParameters& streamParameters,
    BSLA_MAYBE_UNUSED const bsl::shared_ptr<PartitionReopenCycle>& cycle)
{
    // TODO: consider success even before reconfigure response
}

// ACCESSORS
int ClusterQueueReopener::numPendingReopenQueueRequests() const
{
    int sum = 0;
    for (ReopenCycles::const_iterator cit = d_reopenCycles.begin();
         cit != d_reopenCycles.end();
         ++cit) {
        sum += cit->second.use_count();
    }
    return sum;
}

}  // close package namespace
}  // close enterprise namespace
