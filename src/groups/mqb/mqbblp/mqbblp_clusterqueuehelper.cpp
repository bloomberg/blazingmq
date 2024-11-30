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

// mqbblp_clusterqueuehelper.cpp                                      -*-C++-*-
#include <mqbblp_clusterqueuehelper.h>

#include <mqbscm_version.h>
/// Implementation Notes
///====================
//
/// ClusterOpenQueue (initial state)
///--------------------------------
// o Actors
//   [Y]: Proxy - [R]: Replica - [Rs]: All Replicas - [L]: Leader -
//   [P]: Primary
//   --u--> unicast | --b--> broadcast
// o Flow
//   [Y] --u--> [S] : OpenQueueRequest
//   [R] --u--> [L] : QueueLocate
//                     [L] :: assign queue to a partition
//   [L] --b--> [Rs]: QueueAssignment
//   [R] --u--> [P] : ClusterOpenQueue
//   [P] --b--> [Rs]: QueueCreation
//

// MQB
#include <mqbblp_storagemanager.h>
#include <mqbc_clusterdata.h>
#include <mqbc_clusternodesession.h>
#include <mqbc_clusterutil.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcmd_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_storage.h>
#include <mqbnet_cluster.h>
#include <mqbu_exit.h>

// BMQ
#include <bmqimp_queuemanager.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_logthrottle.h>
#include <ball_severity.h>
#include <bdlb_nullablevalue.h>
#include <bdlb_print.h>
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_cstdlib.h>  // for bsl::exit()
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>
#include <bslmf_allocatorargt.h>
#include <bslmf_assert.h>
#include <bsls_annotation.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbblp {

namespace {
const char k_MAXIMUM_NUMBER_OF_QUEUES_REACHED[] =
    "maximum number of queues reached";
const char k_SELF_NODE_IS_STOPPING[] = "self node is stopping";

const int k_MAX_INSTANT_MESSAGES = 10;
// Maximum messages logged with throttling in a short period of time.

const bsls::Types::Int64 k_NS_PER_MESSAGE =
    bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MINUTE / k_MAX_INSTANT_MESSAGES;
// Time interval between messages logged with throttling.

#define BMQ_LOGTHROTTLE_INFO()                                                \
    BALL_LOGTHROTTLE_INFO(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)           \
        << "[THROTTLED] "

/// This function is a simple wrapper around the specified `callback`, to
/// ensure that the specified `refCount` is decremented after it gets
/// invoked with the specified `status`, `queue` and `confirmationCookie`.
void openQueueContextCallbackWrapper(
    int*                                              refCount,
    const mqbi::Cluster::OpenQueueCallback&           callback,
    const bmqp_ctrlmsg::Status&                       status,
    mqbi::Queue*                                      queue,
    const bmqp_ctrlmsg::OpenQueueResponse&            openQueueResponse,
    const mqbi::Cluster::OpenQueueConfirmationCookie& confirmationCookie)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(*refCount >= 1);

    callback(status, queue, openQueueResponse, confirmationCookie);
    --(*refCount);
}

/// Populate the specified `out` with the queueUriKey corresponding to the
/// specified `uri` for the specified `cluster`; that is the canonical URI.
/// The reason is that different queues with the same canonical URI are just
/// one unique queue (regardless of the `id`).
void createQueueUriKey(bmqt::Uri*        out,
                       const bmqt::Uri&  uri,
                       bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<1024> localAllocator(allocator);
    bmqt::UriBuilder                      builder(&localAllocator);
    builder.setDomain(uri.domain()).setTier(uri.tier()).setQueue(uri.queue());

    // Since URI was valid, this should not fail !
    int rc = builder.uri(out);
    BSLS_ASSERT_OPT(rc == 0);
}

void afterAppIdRegisteredDispatched(
    mqbi::Queue*                                 queue,
    const mqbc::ClusterStateQueueInfo::AppInfos& appInfos)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue->dispatcher()->inDispatcherThread(queue));

    queue->queueEngine()->afterAppIdRegistered(appInfos);
}

void afterAppIdUnregisteredDispatched(
    mqbi::Queue*                                 queue,
    const mqbc::ClusterStateQueueInfo::AppInfos& appInfos)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue->dispatcher()->inDispatcherThread(queue));

    queue->queueEngine()->afterAppIdUnregistered(appInfos);
}

void handleHolderDummy(const bsl::shared_ptr<mqbi::QueueHandle>& handle)
{
    // executed by ONE of the *QUEUE* dispatcher threads

    BSLS_ASSERT_SAFE(
        handle->queue()->dispatcher()->inDispatcherThread(handle->queue()));
}

void countUnconfirmed(bsls::Types::Int64* result, mqbi::Queue* queue)
{
    *result += queue->countUnconfirmed(
        bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID);
}

}  // close unnamed namespace

// -----------------------------------------
// struct ClusterQueueHelper::QueueLiveState
// -----------------------------------------

// CREATORS
ClusterQueueHelper::QueueLiveState::QueueLiveState(bslma::Allocator* allocator)
: d_id(bmqp::QueueId::k_UNASSIGNED_QUEUE_ID)
, d_subQueueIds(allocator)
, d_nextSubQueueId(bmqimp::QueueManager::k_INITIAL_SUBQUEUE_ID)
, d_queue_sp(0)
, d_numQueueHandles(0)
, d_numHandleCreationsInProgress(0)
, d_queueExpirationTimestampMs(0)
, d_pending(allocator)
, d_inFlight(0)
{
    // NOTHING
}

ClusterQueueHelper::QueueLiveState::QueueLiveState(
    const ClusterQueueHelper::QueueLiveState& other,
    bslma::Allocator*                         allocator)
: d_id(other.d_id)
, d_subQueueIds(allocator)
, d_nextSubQueueId(other.d_nextSubQueueId)
, d_queue_sp(other.d_queue_sp)
, d_numQueueHandles(other.d_numQueueHandles)
, d_numHandleCreationsInProgress(other.d_numHandleCreationsInProgress)
, d_queueExpirationTimestampMs(other.d_queueExpirationTimestampMs)
, d_pending(other.d_pending)
, d_inFlight(other.d_inFlight)
{
    // NOTHING
}

// MANIPULATORS
void ClusterQueueHelper::QueueLiveState::reset()
{
    // NOTE: Do not reset d_pending and d_inFlight, and some other data.

    d_id = bmqp::QueueId::k_UNASSIGNED_QUEUE_ID;
    d_queue_sp.reset();
    d_numQueueHandles              = 0;
    d_numHandleCreationsInProgress = 0;
    d_queueExpirationTimestampMs   = 0;

    d_subQueueIds.clear();
}

// ------------------------
// class ClusterQueueHelper
// ------------------------

unsigned int ClusterQueueHelper::getNextQueueId()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    // We use a unique queue id for every queue opened with upstream, and this
    // is used to correlate messages with the queue.  Therefore it is critical
    // we *NEVER* reuse an already existing id; and for performance reason, we
    // don't keep track of the released id ('holes' in the sequence), but
    // simply monotonically increase the number.  Therefore, monitor the
    // evolution of that id, and alarm / panic when it becomes close to limits.
    // The simple and only solution then to remedy will be a bounce of the
    // broker.  If we are rolling over the ids, abort the broker to prevent
    // potential reuse of an active id and mixing queues messages.

    unsigned int res = d_nextQueueId++;

    if (d_nextQueueId == bsl::numeric_limits<unsigned int>::max() / 2) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
            << d_cluster_p->description()
            << " nextQueueId for cluster is at 50% capacity, please schedule a"
            << " bounce of this broker." << BMQTSK_ALARMLOG_END;
    }
    else if (d_nextQueueId ==
             bsl::numeric_limits<unsigned int>::max() / 10 * 9) {
        BMQTSK_ALARMLOG_PANIC("CLUSTER_STATE")
            << d_cluster_p->description()
            << " nextQueueId for cluster is at 90% capacity, please urgently "
            << "schedule a bounce of this broker." << BMQTSK_ALARMLOG_END;
    }
    else if (d_nextQueueId == 0 ||
             d_nextQueueId >= bmqp::QueueId::k_RESERVED_QUEUE_ID) {
        BALL_LOG_ERROR << d_cluster_p->description()
                       << " nextQueueId for cluster " << d_cluster_p->name()
                       << " has reached capacity, aborting the broker to "
                       << "contain the damage !";
        mqbu::ExitUtil::terminate(mqbu::ExitCode::e_QUEUEID_FULL);  // EXIT
    }

    return res;
}

unsigned int ClusterQueueHelper::getNextSubQueueId(OpenQueueContext* context)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(context);

    QueueLiveState* queueInfo = &context->d_queueContext_p->d_liveQInfo;

    // We use a unique subQueue id for every subStream of the queue opened with
    // upstream, and this is used to correlate messages with the subStream.
    // Therefore it is critical we *NEVER* reuse an already existing
    // subQueueId; and for performance reason, we don't keep track of the
    // released subQueueIds ('holes' in the sequence), but simply monotonically
    // increase the number.  Therefore, monitor the evolution of that
    // subQueueId, and alarm / panic when it becomes close to limits.  The
    // simple and only solution then to remedy will be a bounce of the broker.
    // If we are rolling over the subQueueIds, abort the broker to prevent
    // potential reuse of an active subQueueId and mixing subStreams' messages.

    unsigned int res = queueInfo->d_nextSubQueueId++;

    if (queueInfo->d_nextSubQueueId ==
        bsl::numeric_limits<unsigned int>::max() / 2) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
            << d_cluster_p->description() << " nextSubQueueId for queue "
            << context->d_queueContext_p->uri()
            << " in cluster is at 50% capacity, please schedule a bounce of"
            << " this broker." << BMQTSK_ALARMLOG_END;
    }
    else if (queueInfo->d_nextSubQueueId ==
             bsl::numeric_limits<unsigned int>::max() / 10 * 9) {
        BMQTSK_ALARMLOG_PANIC("CLUSTER_STATE")
            << d_cluster_p->description() << " nextSubQueueId for queue "
            << context->d_queueContext_p->uri()
            << " in cluster is at 90% capacity, please urgently schedule a"
            << " bounce of this broker." << BMQTSK_ALARMLOG_END;
    }
    else if (queueInfo->d_nextSubQueueId == 0 ||
             (queueInfo->d_nextSubQueueId >=
              bmqp::QueueId::k_RESERVED_SUBQUEUE_ID)) {
        BALL_LOG_ERROR << d_cluster_p->description()
                       << " nextSubQueueId for queue "
                       << context->d_queueContext_p->uri() << " in cluster "
                       << d_cluster_p->name()
                       << " has reached capacity, aborting the broker to"
                       << " contain the damage !";
        mqbu::ExitUtil::terminate(mqbu::ExitCode::e_QUEUEID_FULL);  // EXIT
    }

    return res;
}

void ClusterQueueHelper::afterPartitionPrimaryAssignment(
    int                                partitionId,
    mqbnet::ClusterNode*               primary,
    bmqp_ctrlmsg::PrimaryStatus::Value status)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    // This routine is invoked only in the cluster nodes.

    BALL_LOG_INFO << d_cluster_p->description()
                  << " afterPartitionPrimaryAssignment: Partition ["
                  << partitionId << "]: new primary: "
                  << (primary ? primary->nodeDescription() : "** none **")
                  << ", primary status: " << status;

    if (!primary) {
        // We lost the primary of the partition.

        BSLS_ASSERT_SAFE(bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED == status);

        BALL_LOG_INFO
            << d_cluster_p->description() << " Partition [" << partitionId
            << "] lost its primary, partition has "
            << d_clusterState_p->partitions()[partitionId].numQueuesMapped()
            << " queues mapped.";

        onUpstreamNodeChange(0, partitionId);
        return;  // RETURN
    }
    // There is a valid primary.

    BSLS_ASSERT_SAFE(status != bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED);

    if (!d_cluster_p->isFSMWorkflow() ||
        status == bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE) {
        restoreState(partitionId);
    }

    if (status == bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE) {
        onUpstreamNodeChange(primary, partitionId);
    }
}

mqbi::ClusterStateManager::QueueAssignmentResult::Enum
ClusterQueueHelper::assignQueue(const QueueContextSp& queueContext)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!isQueueAssigned(*(queueContext.get())) ||
                     ((d_cluster_p->isCSLModeEnabled() &&
                       queueContext->d_stateQInfo_sp->pendingUnassignment())));

    if (d_cluster_p->isRemote()) {
        // Assigning a queue in a remote, is simply giving it a new queueId.
        queueContext->d_liveQInfo.d_id = getNextQueueId();
        onQueueContextAssigned(queueContext);
        return QueueAssignmentResult::k_ASSIGNMENT_OK;  // RETURN
    }

    if (d_clusterData_p->electorInfo().hasActiveLeader()) {
        if (d_clusterData_p->electorInfo().isSelfLeader()) {
            return d_clusterStateManager_p->assignQueue(queueContext->uri());
            // RETURN
        }
        else {
            requestQueueAssignment(queueContext->uri());
            return QueueAssignmentResult::k_ASSIGNMENT_OK;  // RETURN
        }
    }

    // Queue not yet assigned, because we don't have a leader (or leader is not
    // active) at the moment, nothing to be done; the queue will automatically
    // be re-processed once we have an active leader.

    BALL_LOG_INFO << d_cluster_p->description()
                  << " Cannot proceed with queueAssignment of "
                  << "'" << queueContext->uri()
                  << "' (waiting for an ACTIVE leader).";

    return QueueAssignmentResult::k_ASSIGNMENT_OK;
}

void ClusterQueueHelper::onQueueAssigning(const bmqt::Uri& uri,
                                          bool processingPendingRequests)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    QueueContextSp      queueContext;
    QueueContextMapIter queueContextIt = d_queues.find(uri);
    if (queueContextIt == d_queues.end()) {
        // Queue unknown, create a new one
        queueContext.reset(new (*d_allocator_p)
                               QueueContext(uri, d_allocator_p),
                           d_allocator_p);

        d_queues[uri] = queueContext;
    }
    else {
        queueContext = queueContextIt->second;

        if (d_cluster_p->isCSLModeEnabled()) {
            queueContext->d_liveQInfo.d_queueExpirationTimestampMs = 0;
        }
    }

    if (!d_cluster_p->isCSLModeEnabled()) {
        queueContext->d_stateQInfo_sp = d_clusterState_p->domainStates()
                                            .at(uri.qualifiedDomain())
                                            ->queuesInfo()
                                            .at(uri);

        // Process the pending requests on this machine, if any.
        if (processingPendingRequests) {
            onQueueContextAssigned(queueContext);
        }
    }
}

bool ClusterQueueHelper::onQueueUnassigning(
    bool*                          hasInFlightRequests,
    const bmqp_ctrlmsg::QueueInfo& queueInfo)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isCSLModeEnabled());
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(hasInFlightRequests);

    bmqt::Uri        uri(queueInfo.uri());
    mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(),
                         queueInfo.key().data());

    QueueContextMapIter queueContextIt = d_queues.find(uri);
    if (queueContextIt == d_queues.end()) {
        // We don't know about that uri .. nothing to do, but error because
        // it should not happen.
        //
        // NOTE: it may happen if the node is starting, hasn't yet
        //       synchronized its cluster state but receives an
        //       unassignment advisory from a primary.
        BALL_LOG_ERROR << d_cluster_p->description()
                       << " Ignoring queueUnAssignementAdvisory for unknown "
                       << "queue " << queueInfo;

        BSLS_ASSERT_SAFE(0 == d_clusterState_p->queueKeys().count(key));
        // Since queue uri is unknown to self node, queue key should be
        // unknown too.

        return false;  // RETURN
    }

    QueueContext* queueContext = queueContextIt->second.get();
    if (0 != queueContext->d_liveQInfo.d_numQueueHandles) {
        // This could occur if destruction of a handle at self node is
        // delayed (note that we enqueue handleSp to various threads when
        // it is removed from a queue) until after a queue unassignment
        // advisory is received.

        BALL_LOG_WARN << d_cluster_p->description()
                      << " Received queue-unassignment advisory for [" << uri
                      << "] but num handle count is ["
                      << queueContext->d_liveQInfo.d_numQueueHandles << "].";
    }

    if (queueContext->d_liveQInfo.d_inFlight != 0 ||
        !queueContext->d_liveQInfo.d_pending.empty()) {
        // If we have in flight requests, we can't delete the QueueInfo
        // references; so we simply reset it's members.  This can occur in
        // this scenario:
        // 1) Self node (replica) receives a close-queue request and forwards
        //    it to primary.
        // 2) Primary receives close-queue request and decides to unmap the
        //    queue and broadcast queue-unassignment advisory.
        // 3) Before self can receive queue-unassignment advisory from the
        //    primary, it receives an open-queue request for the same queue.
        // 4) Self bumps up queue's in-flight/pending count, and sends
        //    request to the primary.
        // 5) Self receives queue-unassignment advisory from the primary.

        // The pending/inFlight request received in (4) will eventually get
        // processed, or rejected (the old primary will reject it) and
        // reprocessed from the beginning with the assignment step.

        BALL_LOG_INFO << d_cluster_p->description()
                      << " While processing queueUnAssignmentAdvisory: "
                      << queueInfo << ", resetting queue info of '" << uri
                      << "', key: " << queueContext->key()
                      << " [in-flight contexts: "
                      << queueContext->d_liveQInfo.d_inFlight
                      << ", pending contexts: "
                      << queueContext->d_liveQInfo.d_pending.size() << "]";

        d_queuesById.erase(queueContext->d_liveQInfo.d_id);
        queueContext->d_liveQInfo.reset();

        *hasInFlightRequests = true;
    }
    else {
        // Nothing is pending, it is safe to delete all references.

        BALL_LOG_INFO << d_cluster_p->description()
                      << " All references to queue " << uri << " with key '"
                      << queueContext->key()
                      << "' removed. Queue was mapped to Partition ["
                      << queueInfo.partitionId() << "].";

        removeQueueRaw(queueContextIt);

        *hasInFlightRequests = false;
    }

    return true;
}

void ClusterQueueHelper::requestQueueAssignment(const bmqt::Uri& uri)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().hasActiveLeader());
    BSLS_ASSERT_SAFE(!d_clusterData_p->electorInfo().isSelfLeader());
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(uri.isCanonical());

    bmqp_ctrlmsg::NodeStatus::Value status =
        d_clusterData_p->membership().selfNodeStatus();

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != status) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << " Cannot proceed with queueAssignment of '" << uri
                      << "' because self is " << status;
        return;  // RETURN
    }

    mqbc::ClusterNodeSession* leader =
        d_clusterData_p->membership().getClusterNodeSession(
            d_clusterData_p->electorInfo().leaderNode());

    status = leader->nodeStatus();
    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != status) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << " Cannot proceed with queueAssignment of '" << uri
                      << "' because the leader is " << status;
        return;  // RETURN
    }

    RequestManagerType::RequestSp request =
        d_cluster_p->requestManager().createRequest();
    bmqp_ctrlmsg::QueueAssignmentRequest& queueAssignmentRequest =
        request->request()
            .choice()
            .makeClusterMessage()
            .choice()
            .makeQueueAssignmentRequest();
    queueAssignmentRequest.queueUri() = uri.asString();

    request->setResponseCb(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onQueueAssignmentResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // requestContext
                             uri,
                             d_clusterData_p->electorInfo().leaderNode()));

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(d_clusterData_p->clusterConfig()
                                       .queueOperations()
                                       .assignmentTimeoutMs());
    bmqt::GenericResult::Enum rc = d_cluster_p->sendRequest(
        request,
        0,  // target (i.e., leader)
        timeoutMs);

    if (rc == bmqt::GenericResult::e_NOT_CONNECTED) {
        // Lost connection with the leader... this will be auto-retried when a
        // new leader becomes elected.
        return;  // RETURN
    }

    if (rc != bmqt::GenericResult::e_SUCCESS) {
        BALL_LOG_ERROR << d_cluster_p->description()
                       << " Error while sending request to leader "
                       << "[rc: " << rc << ", request: " << request->request()
                       << "]";
    }
}

void ClusterQueueHelper::onQueueAssignmentResponse(
    const RequestManagerType::RequestSp& requestContext,
    const bmqt::Uri&                     uri,
    mqbnet::ClusterNode*                 responder)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(uri.isCanonical());

    if (responder != d_clusterData_p->electorInfo().leaderNode()) {
        BALL_LOG_WARN << d_cluster_p->description()
                      << " Received queueAssignmentResponse: "
                      << requestContext->response()
                      << ", from: " << responder->nodeDescription()
                      << ", but current "
                      << "leader/active-node is: "
                      << (d_clusterData_p->electorInfo().leaderNode()
                              ? d_clusterData_p->electorInfo()
                                    .leaderNode()
                                    ->nodeDescription()
                              : "** none **")
                      << ". Ignoring this response.";
        return;  // RETURN

        // Note that we don't remove QueueContext from the 'pendingContext'
        // list when we send a queueAssignmentRequest, which means if/when
        // there is a new leader/active-node, a queueAssignmentRequest for that
        // QueueContext will be sent to that node.  So there is no need to send
        // this failed queueAssignmentRequest to new leader/active-node at this
        // point.
    }

    // Response must be a status (either 'success' or 'failure').

    if (!requestContext->response().choice().isStatusValue()) {
        BALL_LOG_ERROR << d_cluster_p->description()
                       << " Received unexpected queueAssignmentResponse from '"
                       << responder->nodeDescription()
                       << "': " << requestContext->response();
        BSLS_ASSERT_SAFE(false && "Unexpected queueAssignment response type");
        return;  // RETURN
    }

    const bmqp_ctrlmsg::Status& status =
        requestContext->response().choice().status();

    if (status.category() == bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        // Simply log it; this message is purely informal, and prior to that,
        // the 'leader' would have emitted a 'queueAssignmentAdvisory' message.
        // We however can't assert that the corresponding 'queueContext' now
        // exists and the queue is assigned, because the
        // 'queueAssignmentAdvisory' message may have been dropped due to
        // change of leader.
        BALL_LOG_INFO << d_cluster_p->description()
                      << " Received queueAssignment response from '"
                      << responder->nodeDescription()
                      << "': " << requestContext->response();
    }
    else {
        BALL_LOG_ERROR << d_cluster_p->description()
                       << " Received queueAssignment ERROR response from '"
                       << responder->nodeDescription()
                       << "': " << requestContext->response();
        if (requestContext->result() == bmqt::GenericResult::e_TIMEOUT) {
            // The request timed out, that's not good; can't do more than
            // retrying.  Note that we are here implies that self node still
            // perceives 'responser' as the leader/active-node.  So its okay to
            // just resent the request.

            requestQueueAssignment(uri);
        }
        else if (requestContext->result() == bmqt::GenericResult::e_CANCELED) {
            // The request was canceled, this means the leader's connection was
            // lost; nothing to do, a new leader will be elected and the
            // assignment process will be automatically re-initiated at this
            // time.
        }
        else if (requestContext->result() == bmqt::GenericResult::e_REFUSED) {
            if (requestContext->response().choice().status().code() ==
                mqbi::ClusterErrorCode::e_NOT_LEADER) {
                // The leader changed by the time our request reached it; we
                // don't have to do anything here: since the leader changed, we
                // must have (or will shortly) received a notification about
                // the new leader, and in the 'onClusterLeader' one thing we do
                // is re-emit an assignmentRequest for any unassigned queue,
                // this current one being part of them.
            }
            else if (requestContext->response().choice().status().code() ==
                     mqbi::ClusterErrorCode::e_LIMIT) {
                QueueContextMapIter qit = d_queues.find(uri);
                BSLS_ASSERT_SAFE(qit != d_queues.end());
                bdlma::LocalSequentialAllocator<256> localAllocator(
                    d_allocator_p);
                bsl::vector<QueueContext*> rejected(1, &localAllocator);
                *rejected.begin() = qit->second.get();
                processRejectedQueueAssignments(rejected);
            }
        }
        else {
            BSLS_ASSERT_SAFE(false && "Unexpected queueAssignment response");
        }
    }
}

void ClusterQueueHelper::onQueueContextAssigned(
    const QueueContextSp& queueContext)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(isQueueAssigned(*(queueContext.get())));

    const int          pid = queueContext->partitionId();
    bmqu::MemOutStream logMsg(d_allocator_p);
    logMsg << d_cluster_p->description() << ": ";

    if (d_cluster_p->isRemote()) {
        BSLS_ASSERT_SAFE(mqbs::DataStore::k_INVALID_PARTITION_ID == pid);

        logMsg << "Queue '" << queueContext->uri() << "' now assigned.";
        if (queueContext->d_liveQInfo.d_pending.empty()) {
            // If no pending contexts, nothing more to do here but log.
            logMsg << " No pending contexts for the queue.";
            BALL_LOG_INFO << logMsg.str();
            return;  // RETURN
        }

        if (!d_clusterData_p->electorInfo().hasActiveLeader()) {
            // No active leader in proxy (implies no active node).

            logMsg << " Queue has "
                   << queueContext->d_liveQInfo.d_pending.size()
                   << " pending contexts but there is no ACTIVE leader.";
            BALL_LOG_INFO << logMsg.str();
            return;  // RETURN
        }
    }
    else {
        // Cluster member.

        BSLS_ASSERT_SAFE(mqbs::DataStore::k_INVALID_PARTITION_ID != pid);
        const ClusterStatePartitionInfo& pinfo = d_clusterState_p->partition(
            pid);

        logMsg << "Queue '" << queueContext->uri()
               << "' now assigned to Partition [" << pid << "]";
        if (pinfo.primaryNode()) {
            logMsg << " (" << pinfo.primaryNode()->nodeDescription() << ").";
        }
        else {
            logMsg << " (*no primary*).";
        }

        if (queueContext->d_liveQInfo.d_pending.empty()) {
            // If no pending contexts, nothing more to do here but log.
            logMsg << " No pending contexts for the queue.";
            BALL_LOG_INFO << logMsg.str();
            return;  // RETURN
        }

        if (!d_cluster_p->isFSMWorkflow() &&
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE !=
                d_clusterData_p->membership().selfNodeStatus()) {
            // Self is not available, we have to postpone processing the queue
            // opening.

            logMsg << " There are "
                   << queueContext->d_liveQInfo.d_pending.size()
                   << " associated pending contexts, but self is not "
                   << "AVAILABLE. Current self node status: "
                   << d_clusterData_p->membership().selfNodeStatus();
            BALL_LOG_INFO << logMsg.str();
            return;  // RETURN
        }

        if (!pinfo.primaryNode() ||
            bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE != pinfo.primaryStatus()) {
            // We don't have a primary or primary is passive, we have to
            // postpone processing the queue opening.

            logMsg << " There are "
                   << queueContext->d_liveQInfo.d_pending.size()
                   << " associated pending contexts, waiting for an ACTIVE "
                   << "primary for the partition. Current primary: "
                   << (pinfo.primaryNode()
                           ? pinfo.primaryNode()->nodeDescription()
                           : "** null **");

            BALL_LOG_INFO << logMsg.str();
            return;  // RETURN
        }
    }

    logMsg << " Proceeding with " << queueContext->d_liveQInfo.d_pending.size()
           << " associated pending contexts.";
    BALL_LOG_INFO << logMsg.str();

    processPendingContexts(queueContext);
}

void ClusterQueueHelper::processPendingClose(QueueContextSp       queueContext,
                                             StreamsMap::iterator sqit)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    bsl::vector<SubQueueContext::PendingClose> pendingClose(d_allocator_p);
    pendingClose.swap(sqit->value().d_pendingCloseRequests);

    bool isOpen  = sqit->value().d_state == SubQueueContext::k_OPEN;
    bool isValid = true;

    for (size_t i = 0; i < pendingClose.size(); ++i) {
        if (isOpen && isValid) {
            BALL_LOG_INFO << d_cluster_p->description()
                          << ": sending pending Close request with parameters"
                          << " [" << pendingClose[i].d_handleParameters
                          << "].";

            sendCloseQueueRequest(pendingClose[i].d_handleParameters,
                                  sqit,
                                  queueContext->partitionId(),
                                  pendingClose[i].d_callback);
        }
        else {
            // 'sqit' is invalidated or the state is not OPEN.
            BALL_LOG_WARN << d_cluster_p->description()
                          << ": not sending excessive pending Close request"
                          << " with parameters ["
                          << pendingClose[i].d_handleParameters << "].";

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
            isValid = subtractCounters(&queueContext->d_liveQInfo,
                                       pendingClose[i].d_handleParameters,
                                       sqit);
            // 'false' means 'sqit' is deleted (all counters are zeroes)
        }
    }
}

void ClusterQueueHelper::processPendingContexts(
    const QueueContextSp& queueContext)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    // NOTE: We don't check if there are any pending contexts because 1) the
    //       caller typically would have checked for that already and 2) even
    //       if there are none, the below code will mostly be a no-op.

    // Swap the contexts to process them one by one and also clear the
    // pendingContexts of the queue info: they will be enqueued back, if
    // needed.
    bsl::vector<OpenQueueContext> contexts(d_allocator_p);
    contexts.swap(queueContext->d_liveQInfo.d_pending);

    for (bsl::vector<OpenQueueContext>::iterator it = contexts.begin();
         it != contexts.end();
         ++it) {
        processOpenQueueRequest(*it);
    }
}

void ClusterQueueHelper::assignUpstreamSubqueueId(OpenQueueContext* context)
{
    QueueLiveState&   info  = context->d_queueContext_p->d_liveQInfo;
    const bsl::string appId = bmqp::QueueUtil::extractAppId(
        context->d_handleParameters);
    StreamsMap::const_iterator it = info.d_subQueueIds.findByAppIdSafe(appId);

    // If needed, generate upstream subQueueId
    if (context->d_upstreamSubQueueId ==
        bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID) {
        unsigned int upstreamSubId;

        if (it == info.d_subQueueIds.end()) {
            bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo> subQueueIdInfo =
                context->d_handleParameters.subIdInfo();

            if (appId == bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
                upstreamSubId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
            }
            else {
                upstreamSubId                  = getNextSubQueueId(context);
                subQueueIdInfo.value().subId() = upstreamSubId;
            }
            info.d_subQueueIds.insert(
                appId,
                upstreamSubId,
                SubQueueContext(context->d_queueContext_p->uri(),
                                subQueueIdInfo,
                                d_allocator_p));
        }
        else {
            upstreamSubId = it->subId();
        }

        context->d_upstreamSubQueueId = upstreamSubId;
    }
    else if (it == info.d_subQueueIds.end()) {
        info.d_subQueueIds.insert(
            appId,
            context->d_upstreamSubQueueId,
            SubQueueContext(context->d_queueContext_p->uri(),
                            context->d_handleParameters.subIdInfo(),
                            d_allocator_p));
    }
}

void ClusterQueueHelper::processOpenQueueRequest(
    const OpenQueueContext& context)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(isQueueAssigned(*(context.d_queueContext_p)));
    // At this time, the Queue must have been assigned an id/partition.

    OpenQueueContext* context_p = &const_cast<OpenQueueContext&>(context);

    if (d_cluster_p->isRemote()) {
        BSLS_ASSERT_SAFE(d_clusterData_p->electorInfo().hasActiveLeader());

        assignUpstreamSubqueueId(context_p);

        sendOpenQueueRequest(context);
        return;  // RETURN
    }

    const int pid = context.d_queueContext_p->partitionId();

    if (hasActiveAvailablePrimary(pid)) {
        if (d_clusterState_p->isSelfPrimary(pid)) {
            // At primary.  Load the routing configuration and inject the
            // downstream handle parameters into the "response"
            BSLS_ASSERT_SAFE(context.d_domain_p);

            bmqp_ctrlmsg::OpenQueueResponse openQueueResp;

            context.d_domain_p->loadRoutingConfiguration(
                &openQueueResp.routingConfiguration());

            openQueueResp.deduplicationTimeMs() =
                context.d_domain_p->config().deduplicationTimeMs();
            openQueueResp.originalRequest().handleParameters() =
                context.d_handleParameters;
            openQueueResp.originalRequest().handleParameters().qId() =
                bmqp::QueueId::k_PRIMARY_QUEUE_ID;

            createQueue(context,
                        openQueueResp,
                        0);  // upstream == self == null
                             // We ignore rc of 'createQueue'.
        }
        else {
            // We are a replica for the queue, make sure it has a unique
            // upstream id associated, if we haven't already done so, assign
            // one now.

            if (context.d_queueContext_p->d_liveQInfo.d_id ==
                bmqp::QueueId::k_UNASSIGNED_QUEUE_ID) {
                context.d_queueContext_p->d_liveQInfo.d_id = getNextQueueId();
            }

            assignUpstreamSubqueueId(context_p);

            sendOpenQueueRequest(context);
        }
    }
}

void ClusterQueueHelper::sendOpenQueueRequest(const OpenQueueContext& context)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    QueueLiveState& qinfo = context.d_queueContext_p->d_liveQInfo;
    const int       pid   = context.d_queueContext_p->partitionId();

    BSLS_ASSERT_SAFE(isQueueAssigned(*(context.d_queueContext_p)));
    BSLS_ASSERT_SAFE(qinfo.d_id != bmqp::QueueId::k_UNASSIGNED_QUEUE_ID);
    BSLS_ASSERT_SAFE((d_cluster_p->isRemote() &&
                      d_clusterData_p->electorInfo().hasActiveLeader()) ||
                     (d_clusterState_p->hasActivePrimary(pid) &&
                      !d_clusterState_p->isSelfPrimary(pid)));
    // Either a remote cluster with active-node (ie, leader) or a cluster
    // member replica with active primary.

#define CALLBACK_FAILURE(CAT, RC, MSG)                                        \
    do {                                                                      \
        bdlma::LocalSequentialAllocator<128> localAllocator(d_allocator_p);   \
        bmqu::MemOutStream                   os(&localAllocator);             \
        os << MSG;                                                            \
                                                                              \
        bmqp_ctrlmsg::Status failure;                                         \
        failure.category() = CAT;                                             \
        failure.code()     = RC;                                              \
        failure.message().assign(os.str().data(), os.str().length());         \
                                                                              \
        context.d_callback(failure,                                           \
                           0,                                                 \
                           bmqp_ctrlmsg::OpenQueueResponse(),                 \
                           mqbi::Cluster::OpenQueueConfirmationCookie());     \
    } while (0)

    if (bmqp::QueueUtil::isEmpty(context.d_handleParameters)) {
        BALL_LOG_INFO << "#INVALID_OPENQUEUE_REQ "
                      << d_cluster_p->description()
                      << ": Not sending openQueueRequest to "
                      << d_clusterState_p->partition(pid)
                             .primaryNode()
                             ->nodeDescription()
                      << "[context.d_handleParameters: "
                      << context.d_handleParameters
                      << ", reason: 'All read,write,admin counts are <= 0]";
        CALLBACK_FAILURE(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                         bmqt::GenericResult::e_INVALID_ARGUMENT,
                         "All read,write,admin counts are <= 0");
        return;  // RETURN
    }

    RequestManagerType::RequestSp request =
        d_cluster_p->requestManager().createRequest();
    bmqp_ctrlmsg::OpenQueue& openQueue =
        request->request().choice().makeOpenQueue();

    openQueue.handleParameters()       = context.d_handleParameters;
    openQueue.handleParameters().qId() = qinfo.d_id;

    // If we previously generated an upstream subQueueId, then set it here
    // before sending to upstream.
    if (context.d_upstreamSubQueueId != bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) {
        BSLS_ASSERT_SAFE(!context.d_handleParameters.subIdInfo().isNull());
        openQueue.handleParameters().subIdInfo().makeValue(
            context.d_handleParameters.subIdInfo().value());

        openQueue.handleParameters().subIdInfo().value().subId() =
            context.d_upstreamSubQueueId;
    }

    mqbnet::ClusterNode* targetNode = 0;
    if (d_cluster_p->isRemote()) {
        targetNode = d_clusterData_p->electorInfo().leaderNode();
    }
    else {
        targetNode = d_clusterState_p->partition(pid).primaryNode();
    }
    BSLS_ASSERT_SAFE(targetNode);

    request->setResponseCb(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onOpenQueueResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // requestContext
                             context,
                             targetNode));

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(
        d_clusterData_p->clusterConfig().queueOperations().openTimeoutMs());

    bmqt::GenericResult::Enum rc = d_cluster_p->sendRequest(request,
                                                            targetNode,
                                                            timeoutMs);

    if (rc == bmqt::GenericResult::e_NOT_CONNECTED) {
        // Put back the context to the pending list so that it will get
        // re-processed later.
        context.d_queueContext_p->d_liveQInfo.d_pending.push_back(context);
        return;  // RETURN
    }

    if (rc != bmqt::GenericResult::e_SUCCESS) {
        CALLBACK_FAILURE(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                         rc,
                         "Error sending request (" << rc << ")");
        return;  // RETURN
    }

    // Success.   Update _upstream_ view on that particular subQueueId.
    StreamsMap::iterator subStreamIt = qinfo.d_subQueueIds.findBySubId(
        context.d_upstreamSubQueueId);

    bmqp::QueueUtil::mergeHandleParameters(&subStreamIt->value().d_parameters,
                                           context.d_handleParameters);

#undef CALLBACK_FAILURE
}

bmqt::GenericResult::Enum ClusterQueueHelper::sendReopenQueueRequest(
    const RequestManagerType::RequestSp& requestContext,
    mqbnet::ClusterNode*                 activeNode,
    bsls::Types::Uint64                  generationCount)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(requestContext->request().choice().isOpenQueueValue());

    const bmqp_ctrlmsg::QueueHandleParameters& handleParams =
        requestContext->request().choice().openQueue().handleParameters();

    if (bmqp::QueueUtil::isEmpty(handleParams)) {
        BALL_LOG_INFO << "#INVALID_REOPENQUEUE_REQ "
                      << d_cluster_p->description()
                      << ": Not sending ReOpenQueueRequest to "
                      << activeNode->nodeDescription()
                      << "[request: " << requestContext->request()
                      << ", reason: 'All read,write,admin counts are <= 0]";

        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    ++d_numPendingReopenQueueRequests;

    requestContext->setResponseCb(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onReopenQueueResponse,
                             this,
                             bdlf::PlaceHolders::_1,  //  requestContext
                             activeNode,
                             generationCount,
                             1));  // Attempt number for reopen-queue

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(
        d_clusterData_p->clusterConfig().queueOperations().reopenTimeoutMs());
    bmqt::GenericResult::Enum rc = d_cluster_p->sendRequest(requestContext,
                                                            activeNode,
                                                            timeoutMs);
    if (rc != bmqt::GenericResult::e_SUCCESS) {
        --d_numPendingReopenQueueRequests;
    }

    return rc;
}

void ClusterQueueHelper::onOpenQueueResponse(
    const RequestManagerType::RequestSp& requestContext,
    const OpenQueueContext&              context,
    mqbnet::ClusterNode*                 responder)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(requestContext->request().choice().isOpenQueueValue());

    BALL_LOG_INFO << d_cluster_p->description() << ": on OpenQueueResponse "
                  << "from " << responder->nodeDescription() << ": "
                  << requestContext->response()
                  << ", for request: " << requestContext->request();

    bool                 failure   = true;
    bool                 retry     = false;  // retry immediately
    mqbnet::ClusterNode* otherThan = 0;      // retry if the upstream is new
    bool                 pushBack  = false;  // buffer as pending

    const bmqt::GenericResult::Enum mainCode = requestContext->result();

    if (mainCode == bmqt::GenericResult::e_CANCELED) {
        const mqbi::ClusterErrorCode::Enum subCode =
            static_cast<mqbi::ClusterErrorCode::Enum>(
                requestContext->response().choice().status().code());

        if (subCode == mqbi::ClusterErrorCode::e_ACTIVE_LOST ||
            subCode == mqbi::ClusterErrorCode::e_NODE_DOWN ||
            subCode == mqbi::ClusterErrorCode::e_STOPPING) {
            // Request was canceled due to lost of the active (in proxy), or
            // node down (in cluster member) before receiving a response.
            // Open-queue request should be retried (in 'restoreState').
            // This code relies on the order: first 'CancelAllRequests', then
            // 'restoreState'.
            pushBack = true;
        }
    }
    else if (mainCode == bmqt::GenericResult::e_REFUSED) {
        const mqbi::ClusterErrorCode::Enum subCode =
            static_cast<mqbi::ClusterErrorCode::Enum>(
                requestContext->response().choice().status().code());

        if (subCode == mqbi::ClusterErrorCode::e_STOPPING) {
            // Retry immediately if current upstream is different from
            // 'responder'.  Otherwise, add to the pending collection.
            otherThan = responder;
            retry     = true;
        }
        else if (subCode == mqbi::ClusterErrorCode::e_NOT_PRIMARY ||
                 subCode == mqbi::ClusterErrorCode::e_UNKNOWN_QUEUE) {
            // The peer rejected the request because it is (no longer) the
            // primary for the queue's associated partition; or it is not aware
            // of the queue; or it is stopping.  The later condition could
            // happen if the queue got unassigned after the open queue request
            // was sent, but before the remote peer processed it.  Open-queue
            // request should be retried, unless self is stopping.  Note that
            // we do not check if the request would be sent again to the same
            // 'responder' node because maybe that same node got elected
            // primary after sending us the response, so we just send it again
            // to the currently known primary, who may reject it, but
            // eventually this should stabilize.  This potentially could lead
            // to a storm between those two nodes, but this would be the case
            // only if the cluster's state went out of sync, which would
            // already be compromising the integrity of the cluster.  One
            // solution to mitigate this would be to implement an increasing
            // delayed response from the responder (because doing it at the
            // sender here would be tricky as every requests are processed in
            // an event driven base).
            retry = true;
        }
    }
    else if (mainCode != bmqt::GenericResult::e_SUCCESS) {
        // For any other case of failure, no need to retry.  Callback will be
        // invoked later with error status.
        BSLS_ASSERT_SAFE(requestContext->response().choice().isStatusValue());
    }
    else {
        failure = false;
    }

    QueueContext&           qcontext = *context.d_queueContext_p;
    QueueLiveState&         qinfo    = qcontext.d_liveQInfo;
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::OpenQueue& req =
        requestContext->request().choice().openQueue();
    StreamsMap::iterator subStreamIt = qinfo.d_subQueueIds.findBySubId(
        context.d_upstreamSubQueueId);

    BSLS_ASSERT_SAFE(bmqp::QueueUtil::extractAppId(req.handleParameters()) ==
                     subStreamIt->appId());
    BSLS_ASSERT_SAFE(subStreamIt->value().d_parameters.readCount() >=
                     req.handleParameters().readCount());

    if (failure) {
        // Rollback _upstream_ view on that particular subQueueId here due to
        // open queue failure response from upstream.  This is done even if
        // 'retry=true', because if so, 'sendOpenQueueRequest' is invoked
        // again, which will update the view again after sending the request.

        bmqp::QueueUtil::subtractHandleParameters(
            &subStreamIt->value().d_parameters,
            context.d_handleParameters);

        if (d_cluster_p->isStopping() || (!retry && !pushBack)) {
            context.d_callback(requestContext->response().choice().status(),
                               0,
                               bmqp_ctrlmsg::OpenQueueResponse(),
                               mqbi::Cluster::OpenQueueConfirmationCookie());

            return;  // RETURN
        }

        BSLS_ASSERT_SAFE(isQueueAssigned(qcontext));

        if (retry) {
            // We can't just put back the context and 'wait' for a partition
            // primary assignment because it is possible the primary assignment
            // already came before the peer's response; therefore, we just
            // retry with the currently known active primary, if any, or put
            // back to the pending contexts otherwise.  Note that current
            // primary could be self, that's why we call
            // 'processOpenQueueRequest' instead of 'sendOpenQueueRequest'
            // below.

            if (isQueuePrimaryAvailable(qcontext, otherThan)) {
                processOpenQueueRequest(context);
            }
            else {
                pushBack = true;
            }
        }

        if (pushBack) {
            BALL_LOG_INFO << d_cluster_p->description()
                          << ": buffering open queue request for "
                          << qcontext.uri();

            qcontext.d_liveQInfo.d_pending.push_back(context);
        }

        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(!retry);
    BSLS_ASSERT_SAFE(
        requestContext->response().choice().isOpenQueueResponseValue());

    // Received a success openQueue, proceed with the next step.
    if (createQueue(context,
                    requestContext->response().choice().openQueueResponse(),
                    responder)) {
        // Queue instance was successfully created at this node.  Mark its
        // substream's status as 'opened'.
        // This flag will be used to determine if self node needs to issue a
        // reopen-queue request for this substream upon failover (ie, restore
        // state op).
        subStreamIt->value().d_state = SubQueueContext::k_OPEN;
    }
}

void ClusterQueueHelper::onReopenQueueResponse(
    const RequestManagerType::RequestSp& requestContext,
    mqbnet::ClusterNode*                 activeNode,
    bsls::Types::Uint64                  generationCount,
    int                                  numAttempts)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(requestContext->request().choice().isOpenQueueValue());
    BSLS_ASSERT_SAFE(0 < numAttempts);

    BALL_LOG_INFO << d_cluster_p->description() << ": Processing ReopenQueue "
                  << "response [attemptNumber: " << numAttempts
                  << ", request: " << requestContext->request()
                  << ", response: " << requestContext->response() << "]";

    const bmqp_ctrlmsg::OpenQueue& req =
        requestContext->request().choice().openQueue();
    const bmqp_ctrlmsg::QueueHandleParameters& reqParameters =
        req.handleParameters();
    const bmqt::Uri uri(reqParameters.uri());

    QueueContextMapIter it = d_queues.find(uri.canonical());
    if (it == d_queues.end()) {
        // Can occur if client requested to close the queue or queue was GC'ed
        // before ReOpen response was received.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": ignoring ReopenQueueResponse for queue as it"
                      << " no longer exists in the cluster state. Queue ["
                      << uri << "], response: " << requestContext->response();

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    QueueContextSp&    queueContext = it->second;
    QueueLiveState&    qinfo        = queueContext->d_liveQInfo;
    mqbi::Queue*       queueptr     = qinfo.d_queue_sp.get();
    const bsl::string  appId = bmqp::QueueUtil::extractAppId(reqParameters);
    const unsigned int upstreamSubQueueId = bmqp::QueueUtil::extractSubQueueId(
        reqParameters);
    StreamsMap::iterator sqit = qinfo.d_subQueueIds.findBySubIdSafe(
        upstreamSubQueueId);

    if (sqit == qinfo.d_subQueueIds.end()) {
        // REVISIT: This is the result of Close request in beteen
        // Reopen request and response.
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": ignoring ReopenQueueResponse for subQueue as it"
                      << " no longer exists in the queue state. [uri: " << uri
                      << ", upstreamSubQueueId: " << upstreamSubQueueId
                      << "], response: " << requestContext->response();

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    // Send Configure request first if any, and then pending Close requests
    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(&ClusterQueueHelper::processPendingClose,
                             this,
                             queueContext,
                             sqit));

    // Same upstream node, which means num pending request counter must be
    // non-zero.
    BSLS_ASSERT_SAFE(0 < d_numPendingReopenQueueRequests);

    if (bmqt::GenericResult::e_SUCCESS != requestContext->result()) {
        if (bmqt::GenericResult::e_CANCELED == requestContext->result()) {
            // Connection to upstream has been lost.  Simply decrement the
            // counter and return.

            --d_numPendingReopenQueueRequests;

            // Process Close request instead of parking it
            sqit->value().d_state = SubQueueContext::k_CLOSED;

            return;  // RETURN
        }

        // Request failed due to some other reason.
        if (!d_cluster_p->isRemote() ||
            d_clusterData_p->clusterConfig()
                    .queueOperations()
                    .reopenMaxAttempts() == numAttempts) {
            // Either we are in the cluster or we have exhausted max number of
            // attempts for reopen-queue request for this queue.  In either
            // case, alarm, perform any book-keeping and return.  This error is
            // non-recoverable.

            BMQTSK_ALARMLOG_ALARM("QUEUE_REOPEN_FAILURE")
                << d_cluster_p->description()
                << ": error while reopening queue [" << req
                << ", response: " << requestContext->response() << "]"
                << BMQTSK_ALARMLOG_END;

            // Mark the queue's subStream as 'not opened', so that queue
            // does not issue further reopen-queue request for it.

            BSLS_ASSERT_SAFE(sqit != qinfo.d_subQueueIds.end());
            BSLS_ASSERT_SAFE(sqit->appId() == appId);

            sqit->value().d_state = SubQueueContext::k_FAILED;

            notifyQueue(queueContext.get(),
                        upstreamSubQueueId,
                        generationCount,
                        false,
                        false);  // isWriterOnly

            // No need to send a configure-queue request for this queue.
            // Decrement the num pending reopen queue request counter though,
            // and inform if state has been restored.

            onResponseToPendingQueueRequest();
            return;  // RETURN
        }

        sqit->value().d_state = SubQueueContext::k_CLOSED;

        if (d_cluster_p->isStopping()) {
            // Self is stopping.  Drop the response.
            BALL_LOG_INFO << d_cluster_p->description()
                          << ": Not retrying ReopenQueue  [reason: 'stopping'"
                          << ", request: " << requestContext->request()
                          << ", response: " << requestContext->response()
                          << "]";

            onResponseToPendingQueueRequest();
            return;  // RETURN
        }
        // Self node is proxy and we have not yet exhausted max number of
        // reopen-queue attempts.  Schedule a reopen-queue request after the
        // configured time interval.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": queue reopen-request failed. Request: "
                      << requestContext->request()
                      << ", error response: " << requestContext->response()
                      << ". Attempt number was: " << numAttempts
                      << ". Attempting again after "
                      << d_clusterData_p->clusterConfig()
                             .queueOperations()
                             .reopenRetryIntervalMs()
                      << " milliseconds.";

        bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
        after.addMilliseconds(d_clusterData_p->clusterConfig()
                                  .queueOperations()
                                  .reopenRetryIntervalMs());
        d_clusterData_p->scheduler().scheduleEvent(
            after,
            bdlf::BindUtil::bind(&ClusterQueueHelper::onReopenQueueRetry,
                                 this,
                                 requestContext,
                                 activeNode,
                                 generationCount,
                                 numAttempts));
        return;  // RETURN
    }

    // Queue has been successfully reopened; Clear the 'hasReopenFailed' flag
    // and send configure-queue request now.

    BSLS_ASSERT_SAFE(sqit != qinfo.d_subQueueIds.end());
    BSLS_ASSERT_SAFE(appId == sqit->appId());

    if (sqit->value().d_state != SubQueueContext::k_REOPENING) {
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": not sending a configure-queue request in response "
                      << "to ReopenQueue response, for queue [" << uri
                      << "], as the subStream state is "
                      << sqit->value().d_state << ".";

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    if (d_cluster_p->isStopping()) {
        // Self is stopping.  Drop the response.
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": not sending a configure-queue request in response "
                      << "to ReopenQueue response, for queue [" << uri
                      << "], as self is stopping.";
        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    sqit->value().d_state = SubQueueContext::k_OPEN;

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": queue successfully reopened ["
                  << requestContext->request()
                  << "]. Attempting to send a configure-queue request now.";

    if (queueptr == 0) {
        // Can this occur?

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": not sending a configure-queue request in response "
                      << "to ReopenQueue response, for queue [" << uri
                      << "], as queue instance has been deleted.";

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    if (!isQueueAssigned(*queueContext.get())) {
        // Can this occur?

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": not sending a configure-queue request in response "
                      << "to ReopenQueue response, for queue [" << uri
                      << "], as queue is not assigned.";

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    // We have the same active upstream node.  Send the configure-queue
    // request.

    BSLS_ASSERT_SAFE(
        requestContext->response().choice().isOpenQueueResponseValue());

    // Make a copy of the stream parameters, and replace the upstreamSubId if
    // needed.
    bmqp_ctrlmsg::StreamParameters streamParamsCopy;

    // TODO: remove this not thread-safe use of 'getUpstreamParameters'.
    if (!queueptr->getUpstreamParameters(&streamParamsCopy,
                                         upstreamSubQueueId)) {
        ball::Severity::Level logSeverity = ball::Severity::WARN;

        if (bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID == upstreamSubQueueId) {
            // There is an optimization in RelayQueueEngine::configureHandle
            // not to send producer parameters upstream if they are the same as
            // default constructed.  In this case the UpstreamParameters cache
            // does not have parameters for the k_DEFAULT_SUBQUEUE_ID.
            logSeverity = ball::Severity::INFO;

            // Consider this queue successfully reopen
            notifyQueue(queueContext.get(),
                        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID,
                        generationCount,
                        true);
        }
        BALL_LOG_STREAM(logSeverity)
            << d_cluster_p->description()
            << ": not sending a configure-queue request in response to"
            << " ReopenQueue response, for queue [" << uri
            << "], as the queue is not configured for upstream subQueue "
            << bmqp::QueueId::SubQueueIdInt(upstreamSubQueueId);

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    if (!sendConfigureQueueRequest(
            streamParamsCopy,
            queueptr->id(),
            queueptr->uri(),
            bdlf::BindUtil::bind(&ClusterQueueHelper::reconfigureCallback,
                                 this,
                                 bdlf::PlaceHolders::_1,
                                 bdlf::PlaceHolders::_2),
            true,  // is a reconfigure-queue request
            activeNode,
            generationCount,
            sqit->subId())) {
        // TBD: Note that invoking 'd_queue_p->streamParameters()' above is not
        // thread safe as queue's parameteres are supposed to be read/written
        // only from the queue-dispatcher thread.  This will be fixed
        // eventually.

        // Abort restore of the state: the channel is no longer valid, we'll
        // wait for a new one to be active and will restart restoring the state
        // from the beginning.

        // `sendConfigureQueueRequest` calls `reconfigureCallback` which
        // decrements `d_numPendingReopenQueueRequests`
        return;  // RETURN
    }
}

void ClusterQueueHelper::onConfigureQueueResponse(
    const RequestManagerType::RequestSp&               requestContext,
    const bmqt::Uri&                                   uri,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    bsls::Types::Uint64                                generationCount,
    const mqbi::QueueHandle::HandleConfiguredCallback& callback)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    /// Irrespective of success or failure from upstream, we always treat
    /// configureQueue response as success.  A configureQueue request can
    /// fail due to various reasons like:
    /// (1) Upstream node crashes w/o processing the request.
    /// (2) Request times out at self node.
    /// (3) Upstream rejects request (due to reasons like "Unknown QueueId",
    ///     etc).
    /// (4) Other reasons
    ///
    /// It is safe (and important) to treat all of the above as success so
    /// that self's view of the queue and queue handle don't go out of sync.
    struct ScopeGuard {
        bmqp_ctrlmsg::Status                        d_status;
        mqbi::QueueHandle::HandleConfiguredCallback d_callback;
        bmqp_ctrlmsg::StreamParameters              d_streamParams;

        ScopeGuard(const mqbi::QueueHandle::HandleConfiguredCallback& callback,
                   const RequestManagerType::RequestSp&  requestContext,
                   const bmqp_ctrlmsg::StreamParameters& streamParameters)
        : d_callback(callback)
        , d_streamParams(streamParameters)
        {
            if (requestContext->response().choice().isStatusValue()) {
                d_status = requestContext->response().choice().status();
            }
            else {
                d_status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
                d_status.code()     = 0;
            }
        }

        ~ScopeGuard()
        {
            if (d_callback) {
                d_callback(d_status, d_streamParams);
            }
        }

    } guard(callback, requestContext, streamParameters);

    if (d_cluster_p->isStopping()) {
        // Self is stopping.  Drop the response.
        BMQ_LOGTHROTTLE_INFO()
            << d_cluster_p->description()
            << ": Dropping (re)configureQueue response [reason: 'stopping'"
            << ", request: " << requestContext->request()
            << ", response: " << requestContext->response() << "]";
        return;  // RETURN
    }

    QueueContextMapIter it = d_queues.find(uri.canonical());
    if (it == d_queues.end()) {
        // Can occur if client requested to close the queue or queue was GC'ed
        // before Configure response was received.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": ignoring (re)configureQueueResponse for queue as"
                      << " it no longer exists in the cluster state. Queue ["
                      << uri << "], response: " << requestContext->response();
        return;  // RETURN
    }

    QueueContextSp&            queueContext = it->second;
    const bsl::string&         appId        = streamParameters.appId();
    StreamsMap::const_iterator itStream =
        queueContext->d_liveQInfo.d_subQueueIds.findByAppIdSafe(appId);

    if (itStream == queueContext->d_liveQInfo.d_subQueueIds.end()) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": ignoring (re)configureQueueResponse for queue as"
                      << " the app is no longer open. Queue [" << uri
                      << "], response: " << requestContext->response();
        return;  // RETURN
    }

    if (requestContext->response().choice().isStatusValue()) {
        // Must be a failure.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Received failed (re)configureQueueStream response:"
                      << " [request: " << requestContext->request()
                      << ", response: " << requestContext->response()
                      << "], but will treat it as success.";
    }
    else {
        notifyQueue(queueContext.get(),
                    itStream->subId(),
                    generationCount,
                    true,
                    false);  // isWriterOnly
    }
}

void ClusterQueueHelper::onReopenQueueRetry(
    const RequestManagerType::RequestSp& requestContext,
    mqbnet::ClusterNode*                 activeNode,
    bsls::Types::Uint64                  generationCount,
    int                                  numAttempts)
{
    // executed by *SCHEDULER* thread

    if (d_cluster_p->isStopping()) {
        return;  // RETURN
    }

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onReopenQueueRetryDispatched,
                             this,
                             requestContext,
                             activeNode,
                             generationCount,
                             numAttempts),
        d_cluster_p);
}

void ClusterQueueHelper::onReopenQueueRetryDispatched(
    const RequestManagerType::RequestSp& requestContext,
    mqbnet::ClusterNode*                 activeNode,
    bsls::Types::Uint64                  generationCount,
    int                                  numAttempts)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(activeNode);
    BSLS_ASSERT_SAFE(0 < numAttempts);

    if (d_cluster_p->isStopping()) {
        return;  // RETURN
    }

    if (activeNode != d_clusterData_p->electorInfo().leaderNode() ||
        generationCount != d_clusterData_p->electorInfo().electorTerm()) {
        // Active node has changed or is the same but with a different
        // generation (i.e., old active node crashed, came back up and became
        // the active node for this proxy again).  No action needs to be taken
        // here.
        BALL_LOG_WARN
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

        // Decrement the num pending counter.  Counter is not decremented when
        // a reopen-queue request is scheduled for a retry.  The counter is
        // also not decremented or reset to zero when the active node goes
        // down.

        onResponseToPendingQueueRequest();

        return;  // RETURN
    }

    // Active node is unchanged, so following assert must not fire.

    BSLS_ASSERT_SAFE(0 < d_numPendingReopenQueueRequests);

    // Issue reopen-queue request for the specified 'requestContext'.
    const bmqp_ctrlmsg::QueueHandleParameters& reqParameters =
        requestContext->request().choice().openQueue().handleParameters();
    const bsl::string&       uriStr = reqParameters.uri();
    bmqt::Uri                uri(uriStr);
    QueueContextMapConstIter it = d_queues.find(uri.canonical());
    if (it == d_queues.end()) {
        // Can this occur?

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": not retrying ReopenQueue request again for queue "
                      << "[" << uri << "], as queue doesn't exist in cluster "
                      << "state.";

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    const QueueContextSp& queueContext = it->second;
    if (!queueContext->d_liveQInfo.d_queue_sp) {
        // Can this occur?

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": not retrying ReopenQueue request for queue ["
                      << uri << "], as queue instance has been deleted.";

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    if (!isQueueAssigned(*queueContext.get())) {
        // Can this occur?

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": not retrying ReopenQueue request for queue ["
                      << uri << "], as queue is not assigned.";

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }
    const unsigned int upstreamSubQueueId = bmqp::QueueUtil::extractSubQueueId(
        reqParameters);
    QueueLiveState&      qinfo = queueContext->d_liveQInfo;
    StreamsMap::iterator sqit  = qinfo.d_subQueueIds.findBySubIdSafe(
        upstreamSubQueueId);

    if (sqit == qinfo.d_subQueueIds.end()) {
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": not retrying ReopenQueue request as it"
                      << " no longer exists in the queue state. [uri: " << uri
                      << ", upstreamSubQueueId: " << upstreamSubQueueId << "]";

        onResponseToPendingQueueRequest();
        return;  // RETURN
    }

    sqit->value().d_state = SubQueueContext::k_REOPENING;

    RequestManagerType::RequestSp request =
        d_cluster_p->requestManager().createRequest();
    request->request() = requestContext->request();  // Reuse previous request

    // Reuse previous request but update the count values
    bmqp_ctrlmsg::QueueHandleParameters& upstreamParams =
        request->request().choice().openQueue().handleParameters();

    upstreamParams       = sqit->value().d_parameters;
    upstreamParams.qId() = qinfo.d_id;

    request->setResponseCb(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onReopenQueueResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // requestContext
                             activeNode,
                             generationCount,
                             numAttempts + 1));  // Attempt number

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(
        d_clusterData_p->clusterConfig().queueOperations().reopenTimeoutMs());
    bmqt::GenericResult::Enum rc = d_cluster_p->sendRequest(request,
                                                            activeNode,
                                                            timeoutMs);

    if (rc != bmqt::GenericResult::e_SUCCESS) {
        // Abort restore of the state: the channel is no longer valid, we'll
        // wait for a new one to be active and will restart restoring the state
        // from the beginning.

        --d_numPendingReopenQueueRequests;
        return;  // RETURN
    }
}

void ClusterQueueHelper::onOpenQueueConfirmationCookieReleased(
    mqbi::QueueHandle**                        value,
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters)
{
    // TBD: NOT REVIEWED
    // executed by *ANY* thread

    mqbi::QueueHandle* handle = *value;
    d_allocator_p->deleteObject(value);

    if (!handle) {
        // The openQueue was successfully processed.. nothing to do
        return;  // RETURN
    }

    // The openQueue was *NOT* successfully received and processed by the
    // requester (likely, the requester disappeared before the response came
    // in); but upstream was a success, so we need to rollback and issue a
    // closeQueue.

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": OpenQueueConfirmationCookie released without "
                  << "successful processing from the requester. Queue handle  "
                  << "ptr [" << handle << "], client ptr [" << handle->client()
                  << "], handle parameters: " << handleParameters << ".";
    handle->drop();
}

bool ClusterQueueHelper::createQueue(
    const OpenQueueContext&                context,
    const bmqp_ctrlmsg::OpenQueueResponse& openQueueResponse,
    mqbnet::ClusterNode*                   upstreamNode)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    const unsigned int upstreamQueueId =
        openQueueResponse.originalRequest().handleParameters().qId();

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": createQueue called [upstreamQueueId: "
                  << upstreamQueueId
                  << ", openQueueResponse: " << openQueueResponse << ", "
                  << "context.d_handleParameters: "
                  << context.d_handleParameters << "]";

    mqbi::Cluster::OpenQueueConfirmationCookie confirmationCookie(
        new (*d_allocator_p) mqbi::QueueHandle * (0),
        bdlf::BindUtil::bind(
            &ClusterQueueHelper::onOpenQueueConfirmationCookieReleased,
            this,
            bdlf::PlaceHolders::_1,  // queue handle*
            context.d_handleParameters),
        d_allocator_p);

    bdlma::LocalSequentialAllocator<1024>      la(d_allocator_p);
    bmqu::MemOutStream                         errorDescription(&la);
    bmqp_ctrlmsg::Status                       status;
    const bmqp_ctrlmsg::QueueHandleParameters& parameters =
        openQueueResponse.originalRequest().handleParameters();

    QueueContext*   queueContext = context.d_queueContext_p;
    QueueLiveState& qinfo        = queueContext->d_liveQInfo;
    const int       pid          = queueContext->partitionId();
    const bool      isPrimary    = !d_cluster_p->isRemote() &&
                           d_clusterState_p->isSelfPrimary(pid);
    bsl::shared_ptr<mqbi::Queue> queue = createQueueFactory(errorDescription,
                                                            context,
                                                            openQueueResponse);

    if (queue) {
        status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        status.code()     = 0;
        status.message()  = "";
        ++qinfo.d_numHandleCreationsInProgress;

        if (bmqt::QueueFlagsUtil::isWriter(parameters.flags()) &&
            !bmqt::QueueFlagsUtil::isReader(parameters.flags())) {
            // Writer's configure request gets optimized out so notify the
            // queue now.
            bsls::Types::Uint64 genCount;

            if (!d_cluster_p->isRemote()) {
                genCount = d_clusterState_p->partition(pid).primaryLeaseId();
            }
            else {
                genCount = d_clusterData_p->electorInfo().electorTerm();
            }
            notifyQueue(queueContext,
                        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID,
                        genCount,
                        true,   // isOpen
                        true);  // isWriterOnly
        }

        context.d_callback(status,
                           queue.get(),
                           openQueueResponse,
                           confirmationCookie);

        return true;  // RETURN
    }

    // Failed to create/register the queue.
    status.category() = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
    status.code()     = -1;
    status.message().assign(errorDescription.str().data(),
                            errorDescription.str().length());
    context.d_callback(status,
                       0,
                       openQueueResponse,
                       mqbi::Cluster::OpenQueueConfirmationCookie());

    if (isPrimary) {
        // No further cleanup required.
        return false;  // RETURN
    }

    // Self node is either a replica or a proxy.  In both cases, we need to
    // rollback i.e., send a close-queue request upstream.

    BSLS_ASSERT_SAFE(upstreamNode);
    BSLS_ASSERT_SAFE(bmqp::QueueId::k_PRIMARY_QUEUE_ID != upstreamQueueId);

    // Update _upstream_ view on that particular subQueueId
    StreamsMap::iterator subStreamIt = qinfo.d_subQueueIds.findBySubId(
        context.d_upstreamSubQueueId);

    subtractCounters(&qinfo, parameters, subStreamIt);
    sendCloseQueueRequest(parameters,
                          mqbi::Cluster::HandleReleasedCallback(),
                          upstreamNode);
    return false;
}

bsl::shared_ptr<mqbi::Queue> ClusterQueueHelper::createQueueFactory(
    bsl::ostream&                          errorDescription,
    const OpenQueueContext&                context,
    const bmqp_ctrlmsg::OpenQueueResponse& openQueueResponse)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(isQueueAssigned(*(context.d_queueContext_p)));

    const int pid = context.d_queueContext_p->partitionId();
    if (!d_cluster_p->isRemote()) {
        BSLS_ASSERT_SAFE(d_clusterState_p->hasActivePrimary(pid));
    }

    // Queue already created, reuse it.
    if (context.d_queueContext_p->d_liveQInfo.d_queue_sp) {
        return context.d_queueContext_p->d_liveQInfo.d_queue_sp;  // RETURN
    }

    // Domain is already aware of the queue, reuse it.
    bsl::shared_ptr<mqbi::Queue> iQueueSp;
    if (context.d_domain_p->lookupQueue(&iQueueSp,
                                        context.d_queueContext_p->uri()) ==
        0) {
        return iQueueSp;  // RETURN
    }

    const bool isPrimary = !d_cluster_p->isRemote() &&
                           d_clusterState_p->isSelfPrimary(pid);

    if (isPrimary) {
        context.d_queueContext_p->d_liveQInfo.d_id =
            bmqp::QueueId::k_PRIMARY_QUEUE_ID;
    }

    // Create the queue
    bsl::shared_ptr<mqbblp::Queue> queueSp(
        new (*d_allocator_p) Queue(context.d_queueContext_p->uri(),
                                   context.d_queueContext_p->d_liveQInfo.d_id,
                                   context.d_queueContext_p->key(),
                                   context.d_queueContext_p->partitionId(),
                                   context.d_domain_p,
                                   d_storageManager_p,
                                   d_clusterData_p->resources(),
                                   &d_clusterData_p->miscWorkThreadPool(),
                                   openQueueResponse.routingConfiguration(),
                                   d_allocator_p),
        d_allocator_p);

    if (!isPrimary) {
        queueSp->createRemote(
            openQueueResponse.deduplicationTimeMs(),
            d_clusterData_p->clusterConfig().queueOperations().ackWindowSize(),
            &d_clusterData_p->stateSpPool());

        if (context.d_domain_p->registerQueue(errorDescription, queueSp) !=
            0) {
            return 0;  // RETURN
        }

        context.d_queueContext_p->d_liveQInfo.d_queue_sp = queueSp;
        d_queuesById[context.d_queueContext_p->d_liveQInfo.d_id] =
            context.d_queueContext_p;
    }
    else {
        // This is the primary of the queue.

        BSLS_ASSERT_SAFE(d_clusterState_p->isSelfActivePrimary(pid));

        queueSp->createLocal();

        // This is the *only* place where queue is registered with StorageMgr.
        // Only the primary needs to register the queue with StorageMgr, which
        // will write as well as replicate a QueueCreationRecord.  If the queue
        // is already registered with the StorageMgr, this will be a no-op.

        // Note that queue is *not* registered with the StorageMgr upon
        // receiving a QueueAssignmentAdvisory from the leader.  What this
        // means is that if a leader issues a QueueAssignmentAdvisory for a
        // queue but the queue is never opened, it will not be registered with
        // the StorageMgr.  This is ok.

        // Use keys in the CSL instead of generating new ones to keep CSL and
        // non-CSL consistent.

        d_storageManager_p->registerQueue(
            context.d_queueContext_p->uri(),
            context.d_queueContext_p->key(),
            context.d_queueContext_p->partitionId(),
            context.d_queueContext_p->d_stateQInfo_sp->appInfos(),
            context.d_domain_p);

        // Queue must have been registered with storage manager before
        // registering it with the domain, otherwise Queue.configure() will
        // fail.

        if (context.d_domain_p->registerQueue(errorDescription, queueSp) !=
            0) {
            return 0;  // RETURN
        }

        context.d_queueContext_p->d_liveQInfo.d_queue_sp = queueSp;
        // No need to insert in d_queuesById since those queues will never
        // be looked up by id (and all have k_PRIMARY_QUEUE_ID id).
    }

    if (!d_cluster_p->isRemote()) {
        d_clusterState_p->updatePartitionNumActiveQueues(
            context.d_queueContext_p->partitionId(),
            1);
    }

    return bsl::shared_ptr<mqbi::Queue>(queueSp);
}

void ClusterQueueHelper::onHandleReleased(
    const bsl::shared_ptr<mqbi::QueueHandle>& handle,
    const mqbi::QueueHandleReleaseResult&     result,
    const bmqp_ctrlmsg::ControlMessage&       request,
    mqbc::ClusterNodeSession*                 requester)
{
    // executed by the *QUEUE* dispatcher thread

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onHandleReleasedDispatched,
                             this,
                             handle,
                             result,
                             request,
                             requester),
        d_cluster_p);
}

void ClusterQueueHelper::onHandleReleasedDispatched(
    const bsl::shared_ptr<mqbi::QueueHandle>& handle,
    const mqbi::QueueHandleReleaseResult&     result,
    const bmqp_ctrlmsg::ControlMessage&       request,
    mqbc::ClusterNodeSession*                 requester)
{
    // executed by the *CLUSTER* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters =
        request.choice().closeQueue().handleParameters();
    const bmqp_ctrlmsg::SubQueueIdInfo& subStreamInfo =
        bmqp::QueueUtil::extractSubQueueInfo(handleParameters);
    const unsigned int queueId = handleParameters.qId();

    // Delete the stream if fully released.

    if (result.hasNoHandleClients() || (result.hasNoHandleStreamConsumers() &&
                                        result.hasNoHandleStreamProducers())) {
        // An event that may need erasing stream(s)
        CNSQueueHandleMapIter it = requester->queueHandles().find(queueId);
        if (it != requester->queueHandles().end()) {
            // Erase handle.  First erase the subQueueId

            CNSStreamsMap& cnsSubQueueIds = it->second.d_subQueueInfosMap;
            CNSStreamsMap::const_iterator sqiIter =
                cnsSubQueueIds.findBySubIdSafe(subStreamInfo.subId());

            BSLS_ASSERT_SAFE(sqiIter != cnsSubQueueIds.end());

            BALL_LOG_INFO << d_cluster_p->description() << ": "
                          << requester->description()
                          << ": Deleting subStream " << sqiIter->appId()
                          << "[subId: " << sqiIter->subId() << ", queue: '"
                          << handle->queue()->uri().asString()
                          << "', queueId: " << queueId << "]";

            cnsSubQueueIds.erase(sqiIter);

            if (result.hasNoHandleClients()) {
                requester->queueHandles().erase(it);
                BALL_LOG_INFO << d_cluster_p->description() << ": "
                              << requester->description()
                              << ": Deleted handle [queue: '"
                              << handle->queue()->uri().asString()
                              << "', id: " << queueId << "]";
            }
        }
        else {
            BALL_LOG_ERROR << d_cluster_p->description() << ": "
                           << requester->description()
                           << ": Unable to delete handle with '"
                           << handle->queue()->uri().asString() << "' "
                           << "[reason: id " << queueId << " not found]";
        }
    }

    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

    response.rId() = request.rId();
    response.choice().makeCloseQueueResponse();

    d_clusterData_p->messageTransmitter().sendMessage(
        response,
        requester->clusterNode());
    // Release the handle's ptr in the queue's context to guarantee that the
    // handle will be destroyed after all ongoing queue events are handled.
    // E.g. clearClientDispatched.
    // Releasing the handle in the queue's thread allows to keep the handle
    // alive until the check is complete.

    handle->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(&handleHolderDummy, handle),
        handle->queue(),
        mqbi::DispatcherEventType::e_DISPATCHER);
}

void ClusterQueueHelper::onHandleConfigured(
    const bmqp_ctrlmsg::Status&           status,
    const bmqp_ctrlmsg::StreamParameters& streamParameters,
    const bmqp_ctrlmsg::ControlMessage&   request,
    mqbc::ClusterNodeSession*             requester)
{
    // executed by *ANY* thread

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onHandleConfiguredDispatched,
                             this,
                             status,
                             streamParameters,
                             request,
                             requester),
        d_cluster_p);
}

void ClusterQueueHelper::onHandleConfiguredDispatched(
    const bmqp_ctrlmsg::Status&           status,
    const bmqp_ctrlmsg::StreamParameters& streamParameters,
    const bmqp_ctrlmsg::ControlMessage&   request,
    mqbc::ClusterNodeSession*             requester)
{
    // executed by the *CLUSTER* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    // Send the response (always success)
    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

    response.rId() = request.rId();

    if (bmqp_ctrlmsg::StatusCategory::E_SUCCESS != status.category()) {
        response.choice().makeStatus(status);
    }
    else {
        // Populate the ConfigureQueueStreamResponse's original 'request' with
        // the queueId and the configured stream parameters
        unsigned int qId;

        if (request.choice().isConfigureQueueStreamValue()) {
            bmqp_ctrlmsg::ConfigureQueueStream& configureQueueStream =
                response.choice().makeConfigureQueueStreamResponse().request();

            qId = request.choice().configureQueueStream().qId();
            configureQueueStream.qId() = qId;

            bmqp::ProtocolUtil::convert(
                &configureQueueStream.streamParameters(),
                streamParameters,
                request.choice()
                    .configureQueueStream()
                    .streamParameters()
                    .subIdInfo());
        }
        else {
            bmqp_ctrlmsg::ConfigureStream& configureStream =
                response.choice().makeConfigureStreamResponse().request();

            qId = request.choice().configureStream().qId();

            configureStream.qId() = qId;

            configureStream.streamParameters() = streamParameters;
        }

        // Need to rebuild Subscriptions
        CNSQueueHandleMap::iterator it = requester->queueHandles().find(qId);
        if (it == requester->queueHandles().end()) {
            // Failure.

            BALL_LOG_WARN << d_cluster_p->description()
                          << ": Received configureStream response from ["
                          << requester->description()
                          << "] for a queue with unknown Id "
                          << "(" << qId << ").";
        }
        else {
            it->second.d_subQueueInfosMap.addSubscriptions(streamParameters);
        }
    }

    d_clusterData_p->messageTransmitter().sendMessage(
        response,
        requester->clusterNode());
}

void ClusterQueueHelper::onGetDomain(
    const bmqp_ctrlmsg::Status&         status,
    mqbi::Domain*                       domain,
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbc::ClusterNodeSession*           requester,
    const int                           peerInstanceId)
{
    // executed by *ANY* thread

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onGetDomainDispatched,
                             this,
                             status,
                             domain,
                             request,
                             requester,
                             peerInstanceId),
        d_cluster_p);
}

void ClusterQueueHelper::onGetDomainDispatched(
    const bmqp_ctrlmsg::Status&         status,
    mqbi::Domain*                       domain,
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbc::ClusterNodeSession*           requester,
    const int                           peerInstanceId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(request.choice().isOpenQueueValue());
    BSLS_ASSERT_SAFE(d_cluster_p->isClusterMember());

    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        // Failed to get the domain
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Error while opening domain when processing peer "
                      << "openQueueRequest [requester: "
                      << requester->description() << ", request: " << request
                      << ", error: '" << status << "']";

        // Send an error response
        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

        response.rId() = request.rId();
        response.choice().makeStatus(status);

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());

        return;  // RETURN
    }

    const bmqp_ctrlmsg::QueueHandleParameters& handleParams =
        request.choice().openQueue().handleParameters();

    domain->openQueue(
        bmqt::Uri(handleParams.uri()),
        requester->handleRequesterContext(),
        handleParams,
        bdlf::BindUtil::bind(&ClusterQueueHelper::onGetQueueHandle,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // queueHandle
                             bdlf::PlaceHolders::_3,  // openQueueResp
                             bdlf::PlaceHolders::_4,  // confCookie
                             request,
                             requester,
                             peerInstanceId));
}

void ClusterQueueHelper::onGetQueueHandle(
    const bmqp_ctrlmsg::Status&                      status,
    mqbi::QueueHandle*                               queueHandle,
    const bmqp_ctrlmsg::OpenQueueResponse&           openQueueResponse,
    const mqbi::Domain::OpenQueueConfirmationCookie& confirmationCookie,
    const bmqp_ctrlmsg::ControlMessage&              request,
    mqbc::ClusterNodeSession*                        requester,
    const int                                        peerInstanceId)
{
    // executed by *ANY* thread

    // First step in this routine is to update the cookie with the queue handle
    // if 'confirmationCookie' is valid.  If this open-queue request has
    // succeeded, this object should eventually set 'confirmationCookie' to 0
    // (see 'onGetQueueHandleDispatched').  Note that this is similar to what
    // 'mqba::ClientSession' is doing in 'onQueueOpenCb'.  The rough equivalent
    // of a client session here is the cluster node session represented by
    // 'requester'.

    if (confirmationCookie) {
        *confirmationCookie = queueHandle;
    }

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onGetQueueHandleDispatched,
                             this,
                             status,
                             queueHandle,
                             openQueueResponse,
                             confirmationCookie,
                             request,
                             requester,
                             peerInstanceId),
        d_cluster_p);
}

void ClusterQueueHelper::onGetQueueHandleDispatched(
    const bmqp_ctrlmsg::Status&                      status,
    mqbi::QueueHandle*                               queueHandle,
    const bmqp_ctrlmsg::OpenQueueResponse&           openQueueResponse,
    const mqbi::Domain::OpenQueueConfirmationCookie& confirmationCookie,
    const bmqp_ctrlmsg::ControlMessage&              request,
    mqbc::ClusterNodeSession*                        requester,
    const int                                        peerInstanceId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(request.choice().isOpenQueueValue());
    BSLS_ASSERT_SAFE(d_cluster_p->isClusterMember());

    if (peerInstanceId != requester->peerInstanceId() ||
        requester->nodeStatus() == bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE) {
        // Either a new instance of the peer is up, or old instance is no
        // longer available (ie, channel with old instance went down but has
        // not been re-established).  In either case, we need to rollback this
        // open-queue operation to ensure consistency.  We simply don't reset
        // the 'confirmationCookie'.  We also don't send a response.  This
        // logic takes care of both success and failure of this open-queue
        // result.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Rolling back open-queue result: " << status
                      << " for request: " << request.choice().openQueue()
                      << ", from peer: "
                      << requester->clusterNode()->nodeDescription()
                      << ", because either the peer is down, or new instance "
                      << "of the peer has come up. Peer node status: "
                      << requester->nodeStatus()
                      << ", initial peerInstanceId: " << peerInstanceId
                      << ", current peerInstanceId: "
                      << requester->peerInstanceId() << ".";
        return;  // RETURN
    }

    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        // Failed to create the queue (might be because we no longer are the
        // primary, ...), simply forward that failure to the requester (which
        // is a cluster member peer), and it will properly handle the failure
        // by retrying once it can (i.e., once the queue has been assigned,
        // primary has been chosen, ...).

        bmqp_ctrlmsg::ControlMessage response(&localAllocator);

        response.rId() = request.rId();
        response.choice().makeStatus(status);

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());

        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(queueHandle);
    BSLS_ASSERT_SAFE(confirmationCookie);  // in case of success, the cookie
                                           // must be a valid shared_ptr

    // Update the cookie to point to a null queue handle, which indicates that
    // 'requester' has successfully received and processed the open-queue
    // response.

    *confirmationCookie = 0;  // Indicate proper response of the queueHandle

    const bmqp_ctrlmsg::OpenQueue& openQueue = request.choice().openQueue();
    const bmqp_ctrlmsg::QueueHandleParameters& handleParams =
        openQueue.handleParameters();
    const unsigned int queueId = handleParams.qId();

    CNSQueueHandleMapIter iter = requester->queueHandles().find(queueId);
    if (iter != requester->queueHandles().end()) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Reused handle when processing peer "
                      << "openQueueRequest [requester: "
                      << requester->description() << ", request: " << request
                      << ", peerInstanceId: " << peerInstanceId << "].";

        BSLS_ASSERT_SAFE(queueHandle == iter->second.d_handle_p);

        CNSStreamsMap::const_iterator subQueueIter =
            iter->second.d_subQueueInfosMap.findByHandleParameters(
                handleParams);

        if (subQueueIter == iter->second.d_subQueueInfosMap.end()) {
            // New subStream for this queueHandle
            mqbc::ClusterNodeSession::SubQueueInfo subQueueInfo;
            subQueueInfo.d_clientStats->initialize(
                queueHandle->queue()->uri(),
                requester->statContext().get(),
                d_allocator_p);

            iter->second.d_subQueueInfosMap.insert(handleParams, subQueueInfo);
        }
    }
    else {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Inserting handle to nodeSession, when processing "
                      << "peer openQueueRequest [requester: "
                      << requester->description() << ", request: " << request
                      << ", peerInstanceId: " << peerInstanceId << "].";

        CNSQueueState queueContext;
        queueContext.d_handle_p = queueHandle;

        CNSSubQueueInfo subQueueInfo;
        subQueueInfo.d_clientStats->initialize(queueHandle->queue()->uri(),
                                               requester->statContext().get(),
                                               d_allocator_p);

        queueContext.d_subQueueInfosMap.insert(handleParams, subQueueInfo);

        requester->queueHandles()[queueId] = queueContext;
    }

    // Send success response
    bmqp_ctrlmsg::ControlMessage response(&localAllocator);

    response.choice()
        .makeOpenQueueResponse(openQueueResponse)
        .originalRequest() = openQueue;
    response.rId()         = request.rId();

    d_clusterData_p->messageTransmitter().sendMessage(
        response,
        requester->clusterNode());
}

void ClusterQueueHelper::notifyQueue(QueueContext*       queueContext,
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
            BALL_LOG_INFO << d_cluster_p->description()
                          << ": has deconfigured queue ["
                          << queueContext->uri() << "], subStream id ["
                          << upstreamSubQueueId << "]";
        }
        else {
            queue->dispatcher()->execute(
                bdlf::BindUtil::bind(&mqbi::Queue::onOpenUpstream,
                                     queue,
                                     generationCount,
                                     upstreamSubQueueId,
                                     isWriterOnly),
                queue);
        }
    }
    else {
        queue->dispatcher()->execute(
            bdlf::BindUtil::bind(&mqbi::Queue::onOpenFailure,
                                 queue,
                                 upstreamSubQueueId),
            queue);
    }
}

void ClusterQueueHelper::reconfigureCallback(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::Status& status,
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::StreamParameters&
                                 streamParameters)
{
    onResponseToPendingQueueRequest();
}

void ClusterQueueHelper::onResponseToPendingQueueRequest()
{
    BSLS_ASSERT_SAFE(0 < d_numPendingReopenQueueRequests);

    if (--d_numPendingReopenQueueRequests == 0) {
        BALL_LOG_INFO << d_cluster_p->description() << ": state restored";
    }
}

void ClusterQueueHelper::configureQueueDispatched(
    const bmqt::Uri&                                   uri,
    unsigned int                                       queueId,
    unsigned int                                       upstreamSubQueueId,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& callback)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    if (d_supportShutdownV2) {
        BMQ_LOGTHROTTLE_INFO()
            << d_cluster_p->description()
            << ": Shutting down and skipping configure queue [: " << uri
            << "], queueId: " << queueId
            << ", stream parameters: " << streamParameters;
        if (callback) {
            bmqp_ctrlmsg::Status status;
            status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
            status.message()  = "Shutting down.";
            callback(status, streamParameters);
        }

        return;  // RETURN
    }

    QueueContextMapIter queueContextIt = d_queues.find(uri);

    if (queueContextIt == d_queues.end()) {
        // This can occur in this scenario: self node sent a reopen-queue
        // request to upstream, which failed for some reason.  Queue was
        // eventually gc'd by the primary, and thus, self node removed queue's
        // entry from 'd_queues` data structure (with a "still non zero
        // handles associated with the queue" warning).  Eventually the
        // downstream client went down, and self node attempted to configure/
        // close/drop the queue handle, and we end up here.  See similar note
        // in 'releaseQueueDispatched'.

        BALL_LOG_ERROR << d_cluster_p->description()
                       << ": Attempting to configure handle for a non-existing"
                       << " queue [" << uri << "], queueId: " << queueId
                       << ", stream parameters: " << streamParameters;

        if (callback) {
            bmqp_ctrlmsg::Status status;
            status.category() =
                bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
            status.code() = -1;
            status.message() =
                "Attempting to configure queue handle for non-existent queue.";
            callback(status, streamParameters);
        }

        return;  // RETURN
    }

    QueueContext*   queueContext = queueContextIt->second.get();
    QueueLiveState& qinfo        = queueContext->d_liveQInfo;

    BSLS_ASSERT_SAFE(queueContext);
    BSLS_ASSERT_SAFE(isQueueAssigned(*queueContext));

    StreamsMap::iterator iter = qinfo.d_subQueueIds.findBySubIdSafe(
        upstreamSubQueueId);
    if (iter == qinfo.d_subQueueIds.end()) {
        // SubStream got deleted because of outgoing close request(s) but
        // before close response(s), the handle drops and tries to send
        // deconfigure request.
        BALL_LOG_ERROR << d_cluster_p->description()
                       << ": Attempting to configure handle for a non-existing"
                       << " subStream id [" << upstreamSubQueueId
                       << "], queue [" << uri
                       << "], stream parameters: " << streamParameters;
        if (callback) {
            bmqp_ctrlmsg::Status status;
            status.category() =
                bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
            status.code()    = -1;
            status.message() = "Attempting to configure queue handle for "
                               "non-existent subStream.";
            callback(status, streamParameters);
        }
        return;  // RETURN
    }

    // If reopen previously failed, should not send configure queue request.
    SubQueueContext::Enum state = iter->value().d_state;

    if (state != SubQueueContext::k_OPEN) {
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": For a 'configureHandle' request, indicating "
                      << "success even though the upstream state is not OPEN ("
                      << state << "). Queue [" << uri << "], queueId ["
                      << queueId
                      << "], stream parameters: " << streamParameters;
        if (callback) {
            bmqp_ctrlmsg::Status status;
            status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
            // REVISIT: why 'E_SUCCESS' when 'hasReopenFailed'?
            status.code() = 0;
            callback(status, streamParameters);
        }
        return;  // RETURN
    }

    const int            pid        = queueContext->partitionId();
    mqbnet::ClusterNode* targetNode = 0;
    bsls::Types::Uint64  genCount   = 0;

    if (!d_cluster_p->isRemote()) {
        targetNode = d_clusterState_p->partition(pid).primaryNode();
        genCount   = d_clusterState_p->partition(pid).primaryLeaseId();
    }
    else {
        targetNode = d_clusterData_p->electorInfo().leaderNode();
        genCount   = d_clusterData_p->electorInfo().electorTerm();
    }

    if (0 == targetNode ||
        (!d_cluster_p->isRemote() && d_clusterState_p->isSelfPrimary(pid))) {
        // Either there is no current primary/active-node or self is primary.
        // If self is primary, this routine should not have been invoked at
        // self node, but since everything is async, it's possible that self
        // node was a replica when this routine was scheduled to be invoked.
        // In any case, we simply indicate success via 'callback'.  Self node
        // will advertise correct stream parameters when an upstream node
        // eventually comes up.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": For a 'configureHandle' request, indicating "
                      << "success even though there is currently no upstream ("
                      << "or self is primary). Queue [" << uri << "], queueId "
                      << "[" << queueId
                      << "], stream parameters: " << streamParameters;

        if (callback) {
            // Note that we use 'E_SUCCESS' for the category.  Perhaps a more
            // appropriate category would be 'E_NOT_READY', and then the
            // replica queue engine could handle this case (and treat it as
            // success).

            bmqp_ctrlmsg::Status status;
            status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
            status.code()     = 0;
            callback(status, streamParameters);
        }

        return;  // RETURN
    }

    // Send a configure-stream request.  Don't care about rc; 'callback' will
    // be invoked in any case -- see 'sendConfigureQueueRequest' impl.

    sendConfigureQueueRequest(streamParameters,
                              queueId,
                              uri,
                              callback,
                              false,  // is not a reconfigure-queue request
                              targetNode,
                              genCount,
                              upstreamSubQueueId);
}

void ClusterQueueHelper::releaseQueueDispatched(
    const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters,
    unsigned int                                 upstreamSubQueueId,
    const mqbi::Cluster::HandleReleasedCallback& callback)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    if (d_supportShutdownV2) {
        BMQ_LOGTHROTTLE_INFO()
            << d_cluster_p->description()
            << ": Shutting down and skipping close queue [: "
            << handleParameters.uri()
            << "], queueId: " << handleParameters.qId()
            << ", handle parameters: " << handleParameters;
        if (callback) {
            bmqp_ctrlmsg::Status status;
            status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
            status.message()  = "Shutting down.";
            callback(status);
        }

        return;  // RETURN
    }

    bmqt::Uri           uri(handleParameters.uri());
    QueueContextMapIter queueContextIt = d_queues.find(uri.canonical());
    if (queueContextIt == d_queues.end()) {
        // This can occur in this scenario: self node sent a reopen-queue
        // request to upstream, which failed for some reason.  Queue was
        // eventually gc'd by the primary, and thus, self node removed queue's
        // entry from 'd_queues` data structure (with a "still non zero
        // handles associated with the queue" warning).  Eventually the
        // downstream client went down, and self node attempted to configure/
        // close/drop the queue handle, and we end up here.  See similar note
        // in 'configureQueueDispatched'.

        BALL_LOG_ERROR << d_cluster_p->description()
                       << ": Attempting to release handle for a non-existing"
                       << " queue [" << handleParameters.uri()
                       << "], queueId: " << handleParameters.qId()
                       << ", handle parameters: " << handleParameters;

        if (callback) {
            bmqp_ctrlmsg::Status status;
            status.category() =
                bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
            status.code() = -1;
            status.message() =
                "Attempting to release queue handle for non-existent queue.";
            callback(status);
        }

        return;  // RETURN
    }

    QueueContext*   queueContext = queueContextIt->second.get();
    QueueLiveState& qinfo        = queueContext->d_liveQInfo;

    BSLS_ASSERT_SAFE(queueContext);
    BSLS_ASSERT_SAFE(isQueueAssigned(*queueContext));

    // If reopen previously failed, should not send close queue request for it

    StreamsMap::iterator iter = qinfo.d_subQueueIds.findBySubIdSafe(
        upstreamSubQueueId);
    if (iter == qinfo.d_subQueueIds.end()) {
        BALL_LOG_ERROR << d_cluster_p->description()
                       << ": Attempting to release handle for a non-existing"
                       << " stream [" << handleParameters.uri()
                       << "], subId: " << upstreamSubQueueId
                       << ", handle parameters: " << handleParameters;

        if (callback) {
            bmqp_ctrlmsg::Status status;
            status.category() =
                bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
            status.code() = -1;
            status.message() =
                "Attempting to release queue handle for non-existent stream.";
            callback(status);
        }

        return;  // RETURN
    }

    SubQueueContext::Enum state = iter->value().d_state;

    if (state == SubQueueContext::k_REOPENING) {
        // Cannot send Close request until Reopen response because the
        // upstream may not be ready for it.

        // Save the request for later.
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Parking Close request until Reopen response for"
                      << " the stream [" << handleParameters.uri()
                      << "], subId: " << upstreamSubQueueId
                      << ", handle parameters: " << handleParameters;

        iter->value().d_pendingCloseRequests.emplace_back(handleParameters,
                                                          callback);
    }
    else {
        if (state == SubQueueContext::k_OPEN) {
            sendCloseQueueRequest(handleParameters,
                                  iter,
                                  queueContext->partitionId(),
                                  callback);
        }
        else if (callback) {
            bmqp_ctrlmsg::Status status;
            status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
            // REVISIT: why 'E_SUCCESS' when 'hasReopenFailed'?
            status.code() = 0;
            callback(status);
        }

        subtractCounters(&qinfo, handleParameters, iter);
    }
}

void ClusterQueueHelper::sendCloseQueueRequest(
    const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters,
    StreamsMap::iterator&                        itSubStream,
    const int                                    pid,
    const mqbi::Cluster::HandleReleasedCallback& callback)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    mqbnet::ClusterNode* targetNode = 0;

    if (!d_cluster_p->isRemote()) {
        targetNode = d_clusterState_p->partition(pid).primaryNode();
    }
    else {
        targetNode = d_clusterData_p->electorInfo().leaderNode();
    }

    if (0 == targetNode ||
        (!d_cluster_p->isRemote() && d_clusterState_p->isSelfPrimary(pid))) {
        // Either there is no current primary/active-node or self is primary.
        // If self is primary, this routine should not have been invoked at
        // self node, but since everything is async, it's possible that self
        // node was a replica when this routine was scheduled to be invoked.
        // In any case, we simply indicate success via 'callback'.  Self node
        // will advertise correct stream parameters when an upstream node
        // eventually comes up.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": For a 'releaseHandle' request, indicating "
                      << "success even though there is currently no upstream ("
                      << "or self is primary). Queue [" << handleParameters
                      << "].";

        if (callback) {
            // Note that we use 'E_SUCCESS' for the category.  Perhaps a more
            // appropriate category would be 'E_NOT_READY', and then the
            // replica queue engine could handle this case (and treat it as
            // success).

            bmqp_ctrlmsg::Status status;
            status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
            status.code()     = 0;
            callback(status);
        }

        return;  // RETURN
    }

    // Substitute appropriate upstream subQueueId
    bmqp_ctrlmsg::QueueHandleParameters handleParamsCopy(handleParameters);
    if (itSubStream->subId() != bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) {
        bmqp_ctrlmsg::SubQueueIdInfo& subQueueIdInfo =
            handleParamsCopy.subIdInfo().value();
        subQueueIdInfo.appId() = itSubStream->appId();
        subQueueIdInfo.subId() = itSubStream->subId();

        // Do not mark these subStreams until receiving response because of the
        // corner case when close queue request is received before open queue
        // response.  Instead, update count in response.
    }

    sendCloseQueueRequest(handleParamsCopy, callback, targetNode);
}

void ClusterQueueHelper::onReleaseQueueResponse(
    const RequestManagerType::RequestSp&         requestContext,
    const mqbi::Cluster::HandleReleasedCallback& callback)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Received closeQueue response: [request: "
                  << requestContext->request()
                  << ", response: " << requestContext->response() << "]";

    // Upstream node will send a 'CloseQueueResponse' in case of success, and a
    // 'status' response in case of failure.

    if (requestContext->response().choice().isStatusValue()) {
        // Must be a failure.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Received failed closeQueue response: [request: "
                      << requestContext->request()
                      << ", response: " << requestContext->response()
                      << "], but will treat it as success.";
    }

    if (callback) {
        // Irrespective of success or failure from upstream, we always treat
        // closeQueue response as success.  A closeQueue request can fail due
        // to various reasons like:
        // (1) Upstream node crashes w/o processing the request.
        // (2) Request times out at self node.
        // (3) Upstream rejects request (due to reasons like "Unknown QueueId",
        //     etc).
        // (4) Other reasons.

        // It is safe (and important) to treat all of the above as success so
        // that self's view of the queue and queue handle don't go out of sync.

        bmqp_ctrlmsg::Status status;
        status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        status.code()     = 0;

        callback(status);
    }
}

void ClusterQueueHelper::onQueueHandleCreatedDispatched(mqbi::Queue*     queue,
                                                        const bmqt::Uri& uri,
                                                        bool handleCreated)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    QueueContextMapIter queueContextIt = d_queues.find(uri);
    if (queueContextIt == d_queues.end()) {
        BALL_LOG_ERROR
            << d_cluster_p->description()
            << ": Attemping to process a 'handle-created' event for queue ["
            << uri << "], queue ptr [" << queue
            << "] which does not exist in cluster state.";
        return;  // RETURN
    }

    QueueContextSp& queueContextSp = queueContextIt->second;
    BSLS_ASSERT_SAFE(queueContextSp->d_liveQInfo.d_queue_sp.get() == queue);
    BSLS_ASSERT_SAFE(
        0 < queueContextSp->d_liveQInfo.d_numHandleCreationsInProgress);
    --(queueContextSp->d_liveQInfo.d_numHandleCreationsInProgress);

    if (handleCreated) {
        // A new handle for this queue was created.  Bump up the handle count.

        ++queueContextSp->d_liveQInfo.d_numQueueHandles;
    }

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": latest num handle count for queue [" << uri << "], "
                  << "queue ptr [" << queue
                  << "]: " << queueContextSp->d_liveQInfo.d_numQueueHandles;

    if (0 == queueContextSp->d_liveQInfo.d_numHandleCreationsInProgress &&
        0 == queueContextSp->d_liveQInfo.d_numQueueHandles) {
        // Both counters are zero.  This could occur if queue was created but
        // failed to create its first handle.

        removeQueue(queueContextIt);
    }
}

void ClusterQueueHelper::onQueueHandleDestroyedDispatched(mqbi::Queue* queue,
                                                          const bmqt::Uri& uri)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    QueueContextMapIter queueContextIt = d_queues.find(uri);
    if (queueContextIt == d_queues.end()) {
        // This could occur when replica receives a queue-unassignment advisory
        // from the primary, but has non-zero handles for the queue.  In that
        // scenario, replica will remove queue from 'd_queues' and unregister
        // the queue from the domain (but note that handles have a queueSp, so
        // the queue object will remain valid).

        BALL_LOG_WARN
            << d_cluster_p->description()
            << ": Attempting to process a 'handle-destroyed' event for queue ["
            << uri << "], queue ptr [" << queue
            << "] which does not exist in cluster state.";
        return;  // RETURN
    }

    QueueContextSp& queueContextSp = queueContextIt->second;

    if (queueContextSp->d_liveQInfo.d_queue_sp.get() != queue) {
        // This means that the handle which was destroyed likely belonged to
        // the previous incarnation of the queue, and that previous incarnation
        // non longer exists in the cluster state.

        BALL_LOG_WARN
            << d_cluster_p->description()
            << ": Attempting to process a 'handle-destroyed' event for queue ["
            << uri << "], queue ptr [" << queue
            << "] which exists in cluster state, but with a different queue "
            << "ptr [" << queueContextSp->d_liveQInfo.d_queue_sp
            << "]. This likely means that the handle which was destroyed "
            << "belonged to a previous incarnation of the queue.";
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(
        0 <= queueContextSp->d_liveQInfo.d_numHandleCreationsInProgress);
    BSLS_ASSERT_SAFE(1 <= queueContextSp->d_liveQInfo.d_numQueueHandles);

    int numHandles = --queueContextSp->d_liveQInfo.d_numQueueHandles;

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": latest num handle count for queue [" << uri << "], "
                  << "queue ptr [" << queue << "]: " << numHandles;

    if (0 < numHandles) {
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(0 == numHandles);

    if (0 != queueContextSp->d_liveQInfo.d_numHandleCreationsInProgress) {
        BALL_LOG_INFO
            << d_cluster_p->description() << ": num handle count for queue ["
            << uri << "] has gone to zero but there are ["
            << queueContextSp->d_liveQInfo.d_numHandleCreationsInProgress
            << "] handle-creation events in progress.";
        return;  // RETURN
    }

    removeQueue(queueContextIt);
}

bool ClusterQueueHelper::sendConfigureQueueRequest(
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    int                                                queueId,
    const bmqt::Uri&                                   uri,
    const mqbi::QueueHandle::HandleConfiguredCallback& callback,
    bool                                               isReconfigureRequest,
    mqbnet::ClusterNode*                               upstreamNode,
    bsls::Types::Uint64                                generationCount,
    unsigned int                                       subId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(upstreamNode);

    RequestManagerType::RequestSp request =
        d_cluster_p->requestManager().createRequest();

    // TODO: Replace with 'ConfigureStream' once all brokers recognize it

    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    if (brkrCfg.brokerVersion() == bmqp::Protocol::k_DEV_VERSION ||
        brkrCfg.configureStream()) {
        bmqp_ctrlmsg::ConfigureStream& qs =
            request->request().choice().makeConfigureStream();

        qs.qId()              = queueId;
        qs.streamParameters() = streamParameters;
    }
    else {
        bmqp_ctrlmsg::ConfigureQueueStream& qs =
            request->request().choice().makeConfigureQueueStream();
        qs.qId() = queueId;

        bmqp::ProtocolUtil::convert(
            &qs.streamParameters(),
            streamParameters,
            bmqp::ProtocolUtil::makeSubQueueIdInfo(streamParameters.appId(),
                                                   subId));
    }

    request->setResponseCb(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onConfigureQueueResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // requestContext
                             uri,
                             streamParameters,
                             generationCount,
                             callback));

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(d_cluster_p->isStopping()
                                       ? d_clusterData_p->clusterConfig()
                                             .queueOperations()
                                             .shutdownTimeoutMs()
                                       : d_clusterData_p->clusterConfig()
                                             .queueOperations()
                                             .configureTimeoutMs());

    bmqt::GenericResult::Enum rc = d_cluster_p->sendRequest(request,
                                                            upstreamNode,
                                                            timeoutMs);

    if (rc != bmqt::GenericResult::e_SUCCESS) {
        // Note that 'on[Re]ConfigureQueueResponse' will not be invoked in this
        // case.

        // If channel is invalid, we will eventually get a new upstream node,
        // 'restoreState' logic will kick in, and correct stream parameters
        // will be advertised upstream.  So just like above, we indicate
        // success via 'callback'.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Failed to send 'configureQueue' request "
                      << "(isReconfigure: " << bsl::boolalpha
                      << isReconfigureRequest << "): " << request->request()
                      << ", for queue [" << uri << "] to "
                      << upstreamNode->nodeDescription() << ", rc: " << rc;
        if (callback) {
            // As above, we use 'E_SUCCESS' for the category.  Perhaps a more
            // appropriate category would be 'E_NOT_READY', and then the
            // replica queue engine could handle this case (and treat it as
            // success).

            bmqp_ctrlmsg::Status status;
            status.category() = bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED;
            status.code()     = rc;
            status.message()  = "Failed to send";
            callback(status, streamParameters);
        }

        return false;  // RETURN
    }

    return true;
}

void ClusterQueueHelper::sendCloseQueueRequest(
    const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters,
    const mqbi::Cluster::HandleReleasedCallback& callback,
    mqbnet::ClusterNode*                         upstreamNode)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(upstreamNode);

    // Note that close-queue request failures are treated as success by the
    // downstream (self) node.  This is one of the reasons that this routine
    // does not return an error code.

    RequestManagerType::RequestSp request =
        d_cluster_p->requestManager().createRequest();
    bmqp_ctrlmsg::CloseQueue& req =
        request->request().choice().makeCloseQueue();

    req.handleParameters() = handleParameters;
    req.isFinal()          = false;

    request->setResponseCb(
        bdlf::BindUtil::bind(&ClusterQueueHelper::onReleaseQueueResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // requestContext
                             callback));

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(d_cluster_p->isStopping()
                                       ? d_clusterData_p->clusterConfig()
                                             .queueOperations()
                                             .shutdownTimeoutMs()
                                       : d_clusterData_p->clusterConfig()
                                             .queueOperations()
                                             .closeTimeoutMs());

    bmqt::GenericResult::Enum rc = d_cluster_p->sendRequest(request,
                                                            upstreamNode,
                                                            timeoutMs);

    if (rc != bmqt::GenericResult::e_SUCCESS) {
        // Note that 'onReleaseQueueResponse' will not be invoked in this case.

        // If channel is invalid, we will eventually get a new upstream node,
        // 'restoreState' logic will kick in, and correct stream parameters
        // will be advertised upstream.  So just like above, we indicate
        // success via 'callback'.

        BMQ_LOGTHROTTLE_INFO()
            << d_cluster_p->description()
            << ": Failed to send close-queue request: " << request->request()
            << ", for queue [" << handleParameters.uri() << "] to "
            << upstreamNode->nodeDescription() << ", rc: " << rc
            << ", but still indicating success.";

        if (callback) {
            // As above, we use 'E_SUCCESS' for the category.  Perhaps a more
            // appropriate category would be 'E_NOT_READY', and then the
            // replica queue engine could handle this case (and treat it as
            // success).

            bmqp_ctrlmsg::Status status;
            status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
            status.code()     = 0;
            status.message()  = "";
            callback(status);
        }
    }
}

bool ClusterQueueHelper::subtractCounters(
    QueueLiveState*                            qinfo,
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
    StreamsMap::iterator&                      itSubStream)
{
    BSLS_ASSERT_SAFE(qinfo);

    bmqp::QueueUtil::subtractHandleParameters(
        &itSubStream->value().d_parameters,
        handleParameters);

    // Make sure, 'd_subQueueIds' gets updated.  Consider Close queue request
    // as success always and remove subQueueId if no read/write counts are
    // left.  This is done to avoid sending reopen/deconfigure request for the
    // id.

    if (0 == itSubStream->value().d_parameters.readCount() &&
        0 == itSubStream->value().d_parameters.writeCount()) {
        BALL_LOG_INFO << d_cluster_p->description() << ": Erasing subStream ["
                      << itSubStream->appId() << ", " << itSubStream->subId()
                      << "] on close-queue request for queue ["
                      << handleParameters.uri() << "].";
        if (itSubStream->value().d_timer) {
            d_clusterData_p->scheduler().cancelEventAndWait(
                &itSubStream->value().d_timer);
        }
        qinfo->d_subQueueIds.erase(itSubStream);

        return false;
    }
    return true;
}

void ClusterQueueHelper::restoreState(int partitionId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(mqbs::DataStore::k_INVALID_PARTITION_ID != partitionId);

    // This routine is invoked in the cluster node as well as cluster proxy.

    if (d_cluster_p->isRemote()) {
        restoreStateRemote();
    }
    else {
        restoreStateCluster(partitionId);
    }
}

void ClusterQueueHelper::processRejectedQueueAssignments(
    const bsl::vector<QueueContext*>& rejected)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    bmqp_ctrlmsg::Status failure;
    failure.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    failure.code()     = mqbi::ClusterErrorCode::e_LIMIT;
    failure.message()  = k_MAXIMUM_NUMBER_OF_QUEUES_REACHED;

    for (bsl::vector<QueueContext*>::const_iterator sIt = rejected.begin();
         sIt != rejected.end();
         ++sIt) {
        for (bsl::vector<OpenQueueContext>::iterator
                 cIt   = (*sIt)->d_liveQInfo.d_pending.begin(),
                 cLast = (*sIt)->d_liveQInfo.d_pending.end();
             cIt != cLast;
             ++cIt) {
            cIt->d_callback(failure,
                            0,
                            bmqp_ctrlmsg::OpenQueueResponse(),
                            mqbi::Cluster::OpenQueueConfirmationCookie());
        }
        d_queues.erase((*sIt)->uri());
    }
}

void ClusterQueueHelper::restoreStateRemote()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_cluster_p->isRemote());

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Received state-restore event.";

    if (!d_clusterData_p->electorInfo().hasActiveLeader()) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Not going ahead with state restore since there is "
                      << "no active leader.";
        return;  // RETURN
    }

    // Attempt to re-issue open-queue requests for all applicable queues.
    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    bsl::vector<QueueContext*>            rejected(&localAllocator);
    rejected.reserve(d_queues.size());

    for (QueueContextMapConstIter cit = d_queues.cbegin();
         cit != d_queues.cend();
         ++cit) {
        const QueueContextSp& queueContext = cit->second;
        QueueLiveState&       liveQInfo    = queueContext->d_liveQInfo;

        if (!liveQInfo.d_queue_sp && liveQInfo.d_inFlight == 0) {
            // Queue instance does not exist and self node is not waiting for
            // any pending open-queue responses.  So there is no need to
            // re-issue an open-queue request for this one.

            BALL_LOG_INFO << d_cluster_p->description()
                          << ": Not performing restore of queue ["
                          << queueContext->uri() << "].";
            continue;  // CONTINUE
        }

        if (!isQueueAssigned(*queueContext.get())) {
            // Queue is not assigned to a partition; get it assigned.
            if (QueueAssignmentResult::k_ASSIGNMENT_REJECTED ==
                assignQueue(queueContext)) {
                rejected.push_back(queueContext.get());
            }
            continue;  // CONTINUE
        }

        if (liveQInfo.d_queue_sp) {
            // Self node is a proxy and has created a queue instance, this
            // means the queue was successfully opened.  Need to re-issue the
            // open-queue request unconditionally because in case of proxy,
            // 'restoreState' is invoked when active node changes.
            const bmqt::GenericResult::Enum rc = restoreStateHelper(
                liveQInfo,
                d_clusterData_p->electorInfo().leaderNode(),
                d_clusterData_p->electorInfo().electorTerm());

            if (rc == bmqt::GenericResult::e_NOT_CONNECTED) {
                // Abort restore of the state: the channel is no longer valid
                // or we hit high water mark.  For the case of invalid channel,
                // we'll wait for a new one to be active and will restart
                // restoring the state from the beginning.
                return;  // RETURN
            }
            // In case of other type of failure, just continue processing other
            // queues instead of stopping the 'state restore' sequence.
        }
        // Now proceed with any pending contexts for the queue.  There could
        // be some open-queue requests from the downstream clients enqueued in
        // the proxy, but not processed because there was no active node.
        // Since there is one now, try to forward those open-queue requests to
        // the new active node.

        onQueueContextAssigned(queueContext);
    }

    processRejectedQueueAssignments(rejected);
}

void ClusterQueueHelper::restoreStateCluster(int partitionId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());
    BSLS_ASSERT_SAFE(mqbs::DataStore::k_INVALID_PARTITION_ID != partitionId);

    const bool allPartitions = (mqbs::DataStore::k_ANY_PARTITION_ID ==
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

    if (!d_cluster_p->isFSMWorkflow() &&
        d_clusterData_p->membership().selfNodeStatus() !=
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Not going ahead with restoring partition state "
                      << "because self is not AVAILABLE.  Self status: "
                      << d_clusterData_p->membership().selfNodeStatus();
        return;  // RETURN
    }

    if (!d_clusterData_p->electorInfo().hasActiveLeader() &&
        (allPartitions || d_cluster_p->isFSMWorkflow())) {
        // 'allPartitions' indicate this is a transition due to a leader
        // change, but we don't care if we transitioned to no active leader.
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

    // If a specific partitionId is specified, check if partition is assigned
    // to a primary node, and if that primary is ACTIVE.
    bool                             isSelfPrimary = false;
    const ClusterStatePartitionInfo* pinfo         = 0;

    if (!allPartitions) {
        pinfo = &(d_clusterState_p->partition(partitionId));
        BSLS_ASSERT_SAFE(pinfo);
        if (!hasActiveAvailablePrimary(partitionId)) {
            BALL_LOG_INFO << d_cluster_p->description() << " Partition ["
                          << partitionId
                          << "]: Not restoring partition state because there "
                          << "is no primary or primary isn't ACTIVE. Current "
                          << "primary: "
                          << (pinfo->primaryNode()
                                  ? pinfo->primaryNode()->nodeDescription()
                                  : "** null **")
                          << ", primary status: " << pinfo->primaryStatus();
            return;  // RETURN
        }

        // Primary for this partitionId is ACTIVE.  Check if self is the
        // primary.

        isSelfPrimary = pinfo->primaryNode() ==
                        d_clusterData_p->membership().selfNode();
    }

    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    bsl::vector<QueueContext*>            rejected(&localAllocator);
    rejected.reserve(d_queues.size());

    for (QueueContextMapConstIter cit = d_queues.cbegin();
         cit != d_queues.cend();
         ++cit) {
        const QueueContextSp& queueContext = cit->second;

        if (allPartitions) {
            // Attempt to re-issue open-queue requests for all appropriate
            // queues across *all* partitions.

            if (!queueContext->d_liveQInfo.d_queue_sp &&
                queueContext->d_liveQInfo.d_inFlight == 0) {
                // Queue instance does not exist and self node is not waiting
                // for any pending open-queue responses.  So there is no need
                // to re-issue an open-queue request for this one.

                // TBD: Log at INFO level for now, but eventually should be
                //      lowered to DEBUG/TRACE.

                BALL_LOG_INFO << d_cluster_p->description()
                              << " Not performing restore of queue "
                              << "[" << queueContext->uri() << "].";
                continue;  // CONTINUE
            }

            if (!isQueueAssigned(*queueContext.get())) {
                // Queue is not assigned to a partition; get it assigned.  If
                // self is leader, it will assign it locally, if not it will
                // send a request to the leader, etc.
                if (QueueAssignmentResult::k_ASSIGNMENT_REJECTED ==
                    assignQueue(queueContext)) {
                    rejected.push_back(queueContext.get());
                }
                continue;  // CONTINUE
            }
        }
        else {
            // A specific partitionId is specified.  Attempt to re-issue
            // open-queue requests for all appropriate queues assigned to that
            // partition.

            if (queueContext->partitionId() != partitionId) {
                // Skip the queue as its assigned to a different partitionId.
                continue;  // CONTINUE;
            }

            BSLS_ASSERT_SAFE(isQueueAssigned(*queueContext.get()));
            BSLS_ASSERT_SAFE(isQueuePrimaryAvailable(*queueContext.get()));

            QueueLiveState& qinfo = queueContext->d_liveQInfo;

            if (qinfo.d_queue_sp) {
                if (isSelfPrimary) {
                    // We are assuming that it is not possible for a node to be
                    // primary, lose primary-ship and regain primary-ship;
                    // unless eventually the node went down in which case it
                    // will start from fresh.

                    // Moreover, since self node is now the primary, it is
                    // important for it to register the queue with the
                    // StorageManager.  This is logically equivalent to
                    // registering the queue with StorageManager when a primary
                    // node creates a local queue instance (see
                    // 'createQueueFactory').

                    d_storageManager_p->registerQueue(
                        queueContext->uri(),
                        queueContext->key(),
                        queueContext->partitionId(),
                        queueContext->d_stateQInfo_sp->appInfos(),
                        qinfo.d_queue_sp->domain());

                    // Convert the queue from remote to local instance.
                    queueContext->d_liveQInfo.d_queue_sp->convertToLocal();
                    queueContext->d_liveQInfo.d_id =
                        bmqp::QueueId::k_PRIMARY_QUEUE_ID;

                    if (!queueContext->d_liveQInfo.d_pending.empty()) {
                        // Proceed with pending contexts, if any.
                        BALL_LOG_INFO
                            << d_cluster_p->description()
                            << ": Proceeding with "
                            << queueContext->d_liveQInfo.d_pending.size()
                            << " associated pending contexts for '"
                            << queueContext->uri() << "'";

                        processPendingContexts(queueContext);
                    }
                }
                else {
                    if (queueContext->d_liveQInfo.d_numQueueHandles != 0) {
                        // In the case of a cluster member, queues are deleted
                        // 'lazily' when receiving a notification from the
                        // primary.  This replica may have fully closed the
                        // queue, but the queue has not been deleted by the
                        // primary if another replica still uses it; however
                        // from this replica's perspective, we don't want to
                        // reopen the queue.
                        const bmqt::GenericResult::Enum rc =
                            restoreStateHelper(queueContext->d_liveQInfo,
                                               pinfo->primaryNode(),
                                               pinfo->primaryLeaseId());

                        if (rc == bmqt::GenericResult::e_NOT_CONNECTED) {
                            // Abort restore of the state: the channel is no
                            // longer valid or we hit high water mark.  For the
                            // case of invalid channel, we'll wait for a new
                            // one to be active and will restart restoring the
                            // state from the beginning.
                            return;  // RETURN
                        }
                        // In case of other type of failure, just continue
                        // processing other queues instead of stopping the
                        // 'state restore' sequence.
                    }
                    else {
                        BALL_LOG_INFO
                            << d_cluster_p->description()
                            << ": Skipping restore of " << queueContext->uri()
                            << " because it has no active queue handles";
                    }

                    // We also need to issue requests for any pending contexts:
                    // when a primary fails over, the queue may have been
                    // already open on this node, and all clients which were
                    // connected to the old primary will immediately reconnect,
                    // some might connect to this node and will issue an open
                    // queue.  Because primary just got lost, those open queue
                    // requests were not processed, but appended to the pending
                    // context list, so once we have an active primary, we
                    // should process them.
                    if (!queueContext->d_liveQInfo.d_pending.empty()) {
                        // Proceed with pending contexts, if any.
                        BALL_LOG_INFO
                            << d_cluster_p->description()
                            << ": Proceeding with "
                            << queueContext->d_liveQInfo.d_pending.size()
                            << " associated pending contexts for '"
                            << queueContext->uri() << "'";
                        processPendingContexts(queueContext);
                    }
                }
            }
            else {
                // Queue instance is not created, but the queue is assigned.
                // Proceed ahead.

                onQueueContextAssigned(queueContext);
            }
        }
    }

    processRejectedQueueAssignments(rejected);
}

bmqt::GenericResult::Enum
ClusterQueueHelper::restoreStateHelper(QueueLiveState&      queueInfo,
                                       mqbnet::ClusterNode* activeNode,
                                       bsls::Types::Uint64  generationCount)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(activeNode);

#define LOG_ERROR_SEND(REQ, RC)                                               \
    BALL_LOG_ERROR << d_cluster_p->description() << ": Error while sending "  \
                   << "ReopenQueue request: " << REQ << ", rc: " << RC        \
                   << ".";

    const mqbblp::Queue* queuePtr = queueInfo.d_queue_sp.get();

    for (StreamsMap::iterator iter = queueInfo.d_subQueueIds.begin();
         iter != queueInfo.d_subQueueIds.end();
         ++iter) {
        // Make a copy of upstream parameters, and update the copy with correct
        // upstream queueId.
        bmqp_ctrlmsg::QueueHandleParameters upstreamParamsWithId(
            iter->value().d_parameters);

        upstreamParamsWithId.qId() = queueInfo.d_id;

        if (upstreamParamsWithId.readCount() == 0 &&
            upstreamParamsWithId.writeCount() == 0) {
            // No successfully opened subStreams (ie, appId or subId);
            // no need to send reopen-queue request for them.

            // This seems redundant, for 'sendCloseQueueRequest' removed
            // subStream when counts drop to zeroes.
            continue;  // CONTINUE
        }

        RequestManagerType::RequestSp request =
            d_cluster_p->requestManager().createRequest();
        bmqp_ctrlmsg::OpenQueue& openQueue =
            request->request().choice().makeOpenQueue();
        bmqp_ctrlmsg::SubQueueIdInfo subQueueIdInfo;

        openQueue.handleParameters() = upstreamParamsWithId;
        subQueueIdInfo.subId()       = iter->subId();
        subQueueIdInfo.appId()       = iter->appId();

        BALL_LOG_INFO << "Attempting to re-issue open-queue request for "
                      << "subStream " << subQueueIdInfo << " of queue "
                      << queuePtr->description()
                      << ", having the consumer portion [handleParameters: "
                      << openQueue.handleParameters() << "]";

        const bmqt::GenericResult::Enum rc =
            sendReopenQueueRequest(request, activeNode, generationCount);

        if (rc != bmqt::GenericResult::e_SUCCESS) {
            // Abort restore of the state: the channel is no longer valid,
            // we'll wait for a new one to be active and will restart
            // restoring the state from the beginning.
            LOG_ERROR_SEND(request->request(), rc);
            return rc;  // RETURN
        }
        iter->value().d_state = SubQueueContext::k_REOPENING;
    }

    return bmqt::GenericResult::e_SUCCESS;

#undef LOG_ERROR_SEND
}

void ClusterQueueHelper::deleteQueue(QueueContext* queueContext)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(queueContext);
    BSLS_ASSERT_SAFE(queueContext->d_liveQInfo.d_queue_sp);

    mqbi::Queue* queue = queueContext->d_liveQInfo.d_queue_sp.get();

    // If in cluster, need to *synchronously* notify queue's storage about
    // queue's deletion, before deleting the queue.  Note that invoking
    // 'setQueue' in proxy is undefined as there is no StorageMgr, no valid
    // partitionId assigned to queue, etc.

    if (!d_cluster_p->isRemote()) {
        d_storageManager_p->setQueue(static_cast<mqbi::Queue*>(0),
                                     queueContext->uri(),
                                     queueContext->partitionId());

        // Explicitly synchronize since 'StorageMgr::setQueue' does not.

        d_cluster_p->dispatcher()->synchronize(queue);
    }

    cancelAllTimers(queueContext);

    queue->domain()->unregisterQueue(queue);
    queueContext->d_liveQInfo.d_queue_sp.reset();
}

void ClusterQueueHelper::cancelAllTimers(QueueContext* queueContext)
{
    for (StreamsMap::iterator iter =
             queueContext->d_liveQInfo.d_subQueueIds.begin();
         iter != queueContext->d_liveQInfo.d_subQueueIds.end();
         ++iter) {
        if (iter->value().d_timer) {
            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << ": canceling timer associated with "
                          << queueContext->uri()
                          << ", subStream: " << iter->appId() << "("
                          << iter->subId() << ")";

            d_clusterData_p->scheduler().cancelEventAndWait(
                &iter->value().d_timer);
        }
    }
}

void ClusterQueueHelper::removeQueue(const QueueContextMapIter& it)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(d_queues.end() != it);

    QueueContextSp& queueContextSp = it->second;

    BSLS_ASSERT_SAFE(
        0 == queueContextSp->d_liveQInfo.d_numHandleCreationsInProgress);
    BSLS_ASSERT_SAFE(0 == queueContextSp->d_liveQInfo.d_numQueueHandles);
    BSLS_ASSERT_SAFE(0 ==
                     queueContextSp->d_liveQInfo.d_queueExpirationTimestampMs);

    if (!d_cluster_p->isStopping()) {
        if (!queueContextSp->d_liveQInfo.d_pending.empty() ||
            (0 < queueContextSp->d_liveQInfo.d_inFlight)) {
            // If self is not stopping, and there are pending or in-flight
            // requests, don't remove the queue.
            BALL_LOG_INFO << d_cluster_p->description()
                          << ": num handle count for queue [" << it->first
                          << "] has gone to zero but there are ["
                          << queueContextSp->d_liveQInfo.d_pending.size()
                          << "] pending contexts and ["
                          << queueContextSp->d_liveQInfo.d_inFlight
                          << "] in-flight contexts for the queue.";
            return;  // RETURN
        }
    }
    const int pid = queueContextSp->partitionId();

    if (d_cluster_p->isRemote()) {
        // All criteria for removing queue from the proxy has been met (no
        // handles, and no pending or in-flight contexts).
        unsigned int qId = queueContextSp->d_liveQInfo.d_id;
        BALL_LOG_INFO << d_cluster_p->description() << ": Removing queue '"
                      << queueContextSp->uri() << "' with queueId " << qId
                      << " from Partition [" << pid << "].";

        d_queuesById.erase(qId);

        // Delete the queue instance.
        deleteQueue(queueContextSp.get());

        // Delete the queue entry from cluster state.
        d_queues.erase(it);

        return;  // RETURN
    }

    if (d_cluster_p->isStopping()) {
        // Need to delete the queue instance if self is stopping, to enforce
        // proper destruction of objects at shutdown.  Nothing else needs to be
        // done, even if self is primary.

        deleteQueue(queueContextSp.get());

        return;  // RETURN
    }

    if (!d_clusterState_p->isSelfPrimary(pid)) {
        // Replica node.  Queue instance is deleted only upon receiving queue
        // unassignment advisory from the primary.  Nothing else to do here.

        return;  // RETURN
    }

    // Self is a primary, it may or may not be active.  The case where it is
    // active is an obvious one.  It can be passive in this scenario: self is
    // chosen as the primary for a partition, but before it could transition to
    // active primary, it receives close-queue request(s) for the remote queue,
    // and since primary (self) is not ready, it will treat them as success,
    // which may lead to queue handles being deleted, and this routine being
    // invoked.  So we cannot assert that self is *active* primary here.

    // Queue's storage may or may not be empty.  It will be checked in
    // 'gcExpiredQueues' routine, and queue will be marked for gc, and
    // eventually gc'd if it matches the criteria.
}

void ClusterQueueHelper::removeQueueRaw(const QueueContextMapIter& it)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    const QueueContextSp& queueContextSp = it->second;

    BALL_LOG_INFO << d_cluster_p->description() << ": Removing queue '"
                  << queueContextSp->uri() << "' with queueId "
                  << queueContextSp->d_liveQInfo.d_id << " from Partition ["
                  << queueContextSp->partitionId() << "].";

    mqbi::Queue* queue = queueContextSp->d_liveQInfo.d_queue_sp.get();
    if (queue) {
        d_clusterState_p->updatePartitionNumActiveQueues(
            queueContextSp->partitionId(),
            -1);
        deleteQueue(queueContextSp.get());
    }

    // If we are primary, then no need to delete from 'd_queuesById' since it
    // never was inserted into.
    if (!d_clusterState_p->isSelfPrimary(queueContextSp->partitionId())) {
        d_queuesById.erase(queueContextSp->d_liveQInfo.d_id);
    }
    d_queues.erase(it);
}

void ClusterQueueHelper::onSelfNodeStatus(
    bmqp_ctrlmsg::NodeStatus::Value value)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    // This routine is invoked only in the cluster nodes.

    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    BALL_LOG_INFO << d_cluster_p->description()
                  << " onSelfNodeStatus: self node status: " << value;

    restoreState(mqbs::DataStore::k_ANY_PARTITION_ID);
}

void ClusterQueueHelper::onClusterLeader(
    mqbnet::ClusterNode*                node,
    mqbc::ElectorInfoLeaderStatus::Enum status)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    if (status == mqbc::ElectorInfoLeaderStatus::e_PASSIVE) {
        return;  // RETURN
    }

    // This routine is invoked in the cluster node as well as cluster proxy.

    BALL_LOG_INFO << d_cluster_p->description()
                  << " onClusterLeader: new leader: "
                  << (node ? node->nodeDescription() : "** none **")
                  << ", leader status: " << status;

    restoreState(mqbs::DataStore::k_ANY_PARTITION_ID);

    if (d_cluster_p->isRemote()) {
        // non-proxy (replica) case is handled by
        // afterPartitionPrimaryAssignment

        if (node == 0) {
            onUpstreamNodeChange(0, mqbs::DataStore::k_ANY_PARTITION_ID);
        }
        else if (status == mqbc::ElectorInfoLeaderStatus::e_ACTIVE) {
            onUpstreamNodeChange(node, mqbs::DataStore::k_ANY_PARTITION_ID);
        }
    }
}

void ClusterQueueHelper::onQueueAssigned(
    const mqbc::ClusterStateQueueInfo& info)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    if (!d_cluster_p->isCSLModeEnabled()) {
        // REVISIT
        return;  // RETURN
    }

    const mqbnet::ClusterNode* leaderNode =
        d_clusterData_p->electorInfo().leaderNode();
    const bsl::string& leaderDescription = leaderNode
                                               ? leaderNode->nodeDescription()
                                               : "** UNKNOWN**";

    QueueContextSp      queueContext;
    QueueContextMapIter queueContextIt = d_queues.find(info.uri());
    if (queueContextIt != d_queues.end()) {
        // We already have a queueContext created for that queue
        queueContext = queueContextIt->second;
        BSLS_ASSERT_SAFE(isQueueAssigned(*queueContext));

        if (queueContext->d_stateQInfo_sp) {
            // Queue context is aware of assigned queue, so there must not be
            // partitionId/queueKey mismatch.  And d_queueKeys must also
            // contain the key.
            BSLS_ASSERT_SAFE(
                (queueContext->partitionId() == info.partitionId()) &&
                (queueContext->key() == info.key()));
            BSLS_ASSERT_SAFE(1 ==
                             d_clusterState_p->queueKeys().count(info.key()));

            BSLS_ASSERT_SAFE(
                !queueContext->d_stateQInfo_sp->pendingUnassignment());

            onQueueContextAssigned(queueContext);
            return;  // RETURN
        }
        else {
            if (1 == d_clusterState_p->queueKeys().count(info.key())) {
                // Self node's queue context is unaware of the assigned queue,
                // but queueKey specified in the advisory is present in the
                // 'queueKeys' data structure.

                BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                    << d_cluster_p->description()
                    << ": attempting to apply queue assignment for a known but"
                    << " unassigned queue, but queueKey is not unique. "
                    << "QueueKey [" << info.key() << "], URI [" << info.uri()
                    << "], Partition [" << info.partitionId()
                    << "]. Current leader is: '" << leaderDescription
                    << "'. Ignoring this entry in the advisory."
                    << BMQTSK_ALARMLOG_END;
                return;  // RETURN
            }

            // Update queue's mapping etc.
            mqbc::ClusterState::QueueKeysInsertRc insertRc =
                d_clusterState_p->queueKeys().insert(info.key());
            if (insertRc.second) {
                d_clusterState_p->domainStates()
                    .at(info.uri().qualifiedDomain())
                    ->adjustQueueCount(1);
            }
        }
    }
    else {
        // First time hearing about this queue.  Update 'queueKeys' and
        // ensure that queue key is unique.
        mqbc::ClusterState::QueueKeysInsertRc insertRc =
            d_clusterState_p->queueKeys().insert(info.key());

        if (false == insertRc.second) {
            // QueueKey is not unique.

            BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                << d_cluster_p->description()
                << ": attempting to apply queue assignment for an unknown "
                << "queue [" << info.uri() << "] assigned to Partition ["
                << info.partitionId() << "], but queueKey [" << info.key()
                << "] is not unique. Current leader is: '" << leaderDescription
                << "'. Ignoring this assignment." << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        d_clusterState_p->domainStates()
            .at(info.uri().qualifiedDomain())
            ->adjustQueueCount(1);

        // Create the queueContext.
        queueContext.reset(new (*d_allocator_p)
                               QueueContext(info.uri(), d_allocator_p),
                           d_allocator_p);

        d_queues[info.uri()] = queueContext;
    }

    queueContext->d_stateQInfo_sp = d_clusterState_p->domainStates()
                                        .at(info.uri().qualifiedDomain())
                                        ->queuesInfo()
                                        .at(info.uri());
    // Queue assignment from the leader is honored per the info updated
    // above

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Assigned queue: " << info;

    // Note: In non-CSL mode, the queue creation callback is instead invoked at
    // replica nodes when they receive a queue creation record from the primary
    // in the partition stream.
    if (!d_clusterState_p->isSelfPrimary(info.partitionId())) {
        // This is a replica node

        // Note: It's possible that the queue has already been registered in
        // the StorageMgr if it was a queue found during storage recovery.
        // Therefore, we will allow for duplicate registration which will
        // simply result in a no-op.
        d_storageManager_p->registerQueueReplica(
            info.partitionId(),
            info.uri(),
            info.key(),
            d_clusterState_p->domainStates()
                .at(info.uri().qualifiedDomain())
                ->domain(),
            true);  // allowDuplicate

        d_storageManager_p->updateQueueReplica(
            info.partitionId(),
            info.uri(),
            info.key(),
            info.appInfos(),
            d_clusterState_p->domainStates()
                .at(info.uri().qualifiedDomain())
                ->domain(),
            true);  // allowDuplicate
    }

    // NOTE: Even if it is not needed to invoke 'onQueueContextAssigned' in the
    //       case we just created it (because there are no pending
    //       contexts), we still call it regardless for the logging.
    onQueueContextAssigned(queueContext);
}

void ClusterQueueHelper::onQueueUnassigned(
    const mqbc::ClusterStateQueueInfo& info)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    if (!d_cluster_p->isCSLModeEnabled()) {
        return;  // RETURN
    }

    const bsl::string& leaderDesc =
        d_clusterData_p->electorInfo().leaderNode()->nodeDescription();

    const QueueContextMapIter queueContextIt = d_queues.find(info.uri());
    if (queueContextIt == d_queues.end()) {
        // We don't know about that uri .. nothing to do, but error because
        // it should not happen.
        //
        // NOTE: it may happen if the node is starting, hasn't yet
        //       synchronized its cluster state but receives an
        //       unassignment advisory from the leader.
        BALL_LOG_ERROR << d_cluster_p->description()
                       << ": Ignoring queue unassignment from leader "
                       << leaderDesc << ", for unknown queue: " << info;

        BSLS_ASSERT_SAFE(0 == d_clusterState_p->queueKeys().count(info.key()));
        // Since queue uri is unknown to self node, queue key should be
        // unknown too.

        return;  // RETURN
    }

    const QueueContextSp& queueContextSp = queueContextIt->second;
    QueueLiveState&       qinfo          = queueContextSp->d_liveQInfo;
    if (!isQueueAssigned(*queueContextSp)) {
        // Queue is known but not assigned.  Error because it should not occur.
        // Note that it may occur if self node is starting, received an
        // open-queue request for this queue (and thus, populated 'd_queues'
        // with this queue entry), and then received this advisory, without
        // ever hearing about this queue from the leader.

        BALL_LOG_ERROR << d_cluster_p->description()
                       << ": Ignoring queue unassignment from leader "
                       << leaderDesc << ", for queue: " << info
                       << " because self node sees queue as unassigned.";
        return;  // RETURN
    }
    BSLS_ASSERT_SAFE(queueContextSp->partitionId() == info.partitionId() &&
                     queueContextSp->key() == info.key());

    if (0 != qinfo.d_numQueueHandles) {
        // This could occur if destruction of a handle at self node is delayed
        // (note that we enqueue handleSp to various threads when it is removed
        // from a queue) until after a queue unassignment advisory is received.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Received queue unassignment from leader "
                      << leaderDesc << ", for queue: " << info
                      << " but num handle count is ["
                      << qinfo.d_numQueueHandles << "].";
    }

    if (d_clusterState_p->isSelfPrimary(info.partitionId())) {
        // We already ensured there are no pending contexts for this queue,
        // so we can safely delete it from the various maps.
        removeQueueRaw(queueContextIt);

        // Unregister the queue/storage from the partition, which will end up
        // issuing a QueueDeletion record.  Note that this method is async.
        d_storageManager_p->unregisterQueue(info.uri(), info.partitionId());
    }
    else {
        // This is a replica node.

        if (qinfo.d_inFlight != 0 || !qinfo.d_pending.empty()) {
            // If we have in flight requests, we can't delete the QueueInfo
            // references; so we simply reset it's members.  This can occur in
            // this scenario:
            // 1) Self node (replica) receives a close-queue request and
            //    forwards it to primary.
            // 2) Primary receives close-queue request and decides to unmap the
            //    queue and broadcast queue-unassignment advisory.
            // 3) Before self can receive queue-unassignment advisory from the
            //    primary, it receives an open-queue request for the same
            //    queue.
            // 4) Self bumps up queue's in-flight/pending count, and sends
            //    request to the primary.
            // 5) Self receives queue-unassignment advisory from the primary.

            // The pending/inFlight request received in (4) will eventually get
            // processed, or rejected (the old primary will reject it) and
            // reprocessed from the beginning with the assignment step.

            BALL_LOG_INFO << d_cluster_p->description()
                          << ": While processing queue assignment from leader "
                          << leaderDesc << ", for queue: " << info
                          << ", resetting queue info:"
                          << " [in-flight contexts: " << qinfo.d_inFlight
                          << ", pending contexts: " << qinfo.d_pending.size()
                          << "]";

            if (queueContextSp->d_liveQInfo.d_queue_sp) {
                d_clusterState_p->updatePartitionNumActiveQueues(
                    info.partitionId(),
                    -1);
            }
            d_queuesById.erase(qinfo.d_id);
            qinfo.reset();

            // We do this in CSL mode only, such that isQueueAssigned() will
            // return false.
            queueContextSp->d_stateQInfo_sp.reset();
        }
        else {
            // Nothing is pending, it is safe to delete all references.
            BALL_LOG_INFO << d_cluster_p->description()
                          << ": All references to queue: " << info
                          << " removed.";

            removeQueueRaw(queueContextIt);
        }

        // Note: In non-CSL mode, the queue deletion callback is instead
        // invoked at nodes when they receive a queue deletion record from the
        // primary in the partition stream.
        d_storageManager_p->unregisterQueueReplica(info.partitionId(),
                                                   info.uri(),
                                                   info.key(),
                                                   mqbu::StorageKey());
    }

    d_clusterState_p->queueKeys().erase(info.key());
    d_clusterState_p->domainStates()
        .at(info.uri().qualifiedDomain())
        ->adjustQueueCount(-1);

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Unassigned queue: " << info;
}

void ClusterQueueHelper::onQueueUpdated(const bmqt::Uri&   uri,
                                        const bsl::string& domain,
                                        const AppInfos&    addedAppIds,
                                        const AppInfos&    removedAppIds)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    if (!uri.isValid()) {
        // This is an appID update for the entire domain, instead of any
        // individual queue. Nothing to do for the queue helper.

        return;  // RETURN
    }
    BSLS_ASSERT_SAFE(uri.qualifiedDomain() == domain);

    QueueContextMapIter qiter = d_queues.find(uri);
    BSLS_ASSERT_SAFE(qiter != d_queues.end());

    mqbblp::Queue* queue       = qiter->second->d_liveQInfo.d_queue_sp.get();
    const int      partitionId = qiter->second->partitionId();
    BSLS_ASSERT_SAFE(partitionId != mqbs::DataStore::k_INVALID_PARTITION_ID);

    if (d_cluster_p->isCSLModeEnabled()) {
        if (!d_clusterState_p->isSelfPrimary(partitionId) || queue == 0) {
            // Note: In non-CSL mode, the queue creation callback is
            // invoked at replica nodes when they receive a queue creation
            // record from the primary in the partition stream.

            d_storageManager_p->updateQueueReplica(
                partitionId,
                uri,
                qiter->second->key(),
                addedAppIds,
                d_clusterState_p->domainStates()
                    .at(uri.qualifiedDomain())
                    ->domain());
        }

        for (AppInfosCIter cit = removedAppIds.cbegin();
             cit != removedAppIds.cend();
             ++cit) {
            if (!d_clusterState_p->isSelfPrimary(partitionId) || queue == 0) {
                // Note: In non-CSL mode, the queue deletion callback is
                // invoked at replica nodes when they receive a queue deletion
                // record from the primary in the partition stream.
                d_storageManager_p->unregisterQueueReplica(
                    partitionId,
                    uri,
                    qiter->second->key(),
                    cit->second);
            }
        }
    }

    if (queue) {
        // TODO: replace with one call
        d_cluster_p->dispatcher()->execute(
            bdlf::BindUtil::bind(afterAppIdRegisteredDispatched,
                                 queue,
                                 addedAppIds),
            queue);

        d_cluster_p->dispatcher()->execute(
            bdlf::BindUtil::bind(afterAppIdUnregisteredDispatched,
                                 queue,
                                 removedAppIds),
            queue);
    }

    bmqu::Printer<AppInfos> printer1(&addedAppIds);
    bmqu::Printer<AppInfos> printer2(&removedAppIds);
    BALL_LOG_INFO << d_cluster_p->description() << ": Updated queue: " << uri
                  << ", addedAppIds: " << printer1
                  << ", removedAppIds: " << printer2;
}

void ClusterQueueHelper::onUpstreamNodeChange(mqbnet::ClusterNode* node,
                                              int                  partitionId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    for (QueueContextMapConstIter cit = d_queues.begin();
         cit != d_queues.end();
         ++cit) {
        const QueueContextSp& queueContextSp = cit->second;
        mqbi::Queue* queue = queueContextSp->d_liveQInfo.d_queue_sp.get();

        if (!queue) {
            continue;  // CONTINUE
        }

        if (partitionId != mqbs::DataStore::k_ANY_PARTITION_ID &&
            partitionId != queueContextSp->partitionId()) {
            continue;  // CONTINUE
        }

        if (node == 0) {
            cancelAllTimers(queueContextSp.get());
            queue->dispatcher()->execute(
                bdlf::BindUtil::bind(&mqbi::Queue::onLostUpstream, queue),
                queue);
        }
    }
}

// CREATORS
ClusterQueueHelper::ClusterQueueHelper(
    mqbc::ClusterData*         clusterData,
    mqbc::ClusterState*        clusterState,
    mqbi::ClusterStateManager* clusterStateManager,
    bslma::Allocator*          allocator)
: d_allocator_p(allocator)
, d_nextQueueId(0)
, d_clusterData_p(clusterData)
, d_clusterState_p(clusterState)
, d_cluster_p(&clusterData->cluster())
, d_clusterStateManager_p(clusterStateManager)
, d_storageManager_p(0)
, d_queues(allocator)
, d_queuesById(allocator)
, d_numPendingReopenQueueRequests(0)
, d_primaryNotLeaderAlarmRaised(false)
, d_stopContexts(allocator)
, d_supportShutdownV2(false)
{
    BSLS_ASSERT(
        d_clusterData_p->clusterConfig()
            .queueOperations()
            .configureTimeoutMs() <=
        d_clusterData_p->clusterConfig().queueOperations().closeTimeoutMs());
    // The timeout for configureQueue should be less than or equal to the
    // timeout of closeQueue to prevent out-of-order processing of
    // closeQueue (e.g. closeQueue sent after configureQueue but timeout
    // response processed first for the closeQueue)

    if (d_clusterStateManager_p) {
        d_clusterStateManager_p->setQueueAssigningCb(bdlf::BindUtil::bind(
            &ClusterQueueHelper::onQueueAssigning,
            this,
            bdlf::PlaceHolders::_1,    // uri
            bdlf::PlaceHolders::_2));  // processingPendingRequests
        d_clusterStateManager_p->setQueueUnassigningCb(bdlf::BindUtil::bind(
            &ClusterQueueHelper::onQueueUnassigning,
            this,
            bdlf::PlaceHolders::_1,    // hasInFlightRequests
            bdlf::PlaceHolders::_2));  // queueInfo
        d_clusterStateManager_p->setAfterPartitionPrimaryAssignmentCb(
            bdlf::BindUtil::bind(
                &ClusterQueueHelper::afterPartitionPrimaryAssignment,
                this,
                bdlf::PlaceHolders::_1,    // partitionId
                bdlf::PlaceHolders::_2,    // primary
                bdlf::PlaceHolders::_3));  // status
    }
}

ClusterQueueHelper::~ClusterQueueHelper()
{
    // NOTHING: Interface
}

void ClusterQueueHelper::initialize()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    d_clusterData_p->membership().registerObserver(this);
    d_clusterData_p->electorInfo().registerObserver(this);
    d_clusterState_p->registerObserver(this);
}

void ClusterQueueHelper::teardown()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    d_clusterState_p->unregisterObserver(this);
    d_clusterData_p->electorInfo().unregisterObserver(this);
    d_clusterData_p->membership().unregisterObserver(this);
}

void ClusterQueueHelper::openQueue(
    const bmqt::Uri&                                          uri,
    mqbi::Domain*                                             domain,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const mqbi::Cluster::OpenQueueCallback&                   callback)
{
    // ===                                                                  ===
    // TBD: This should not take the domain, but look it up itself !          =
    // ===                                                                  ===
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(handleParameters.qId() !=
                     bmqp::QueueId::k_UNASSIGNED_QUEUE_ID);

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Initiating openQueue of '" << uri << "' for '"
                  << clientContext->description() << "'";

#define CALLBACK_FAILURE(REASON, CODE)                                        \
    do {                                                                      \
        BALL_LOG_ERROR                                                        \
            << d_cluster_p->description()                                     \
            << ": Received an openQueue request from a cluster peer "         \
            << "node for '" << uri << "' that I can not process "             \
            << "[reason: '" << REASON << "']";                                \
        bmqp_ctrlmsg::Status failure;                                         \
        failure.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;         \
        failure.code()     = CODE;                                            \
        failure.message()  = REASON;                                          \
        callback(failure,                                                     \
                 0,                                                           \
                 bmqp_ctrlmsg::OpenQueueResponse(),                           \
                 mqbi::Cluster::OpenQueueConfirmationCookie());               \
    } while (0)

    if (d_cluster_p->isStopping()) {
        bsl::string                  reason = k_SELF_NODE_IS_STOPPING;
        mqbi::ClusterErrorCode::Enum errorCode =
            mqbi::ClusterErrorCode::e_STOPPING;
        CALLBACK_FAILURE(reason, errorCode);
        return;  // RETURN
    }

    bmqt::Uri uriKey;
    createQueueUriKey(&uriKey, uri, d_allocator_p);
    // Get the 'uri' that should be used as 'canonical' representation of
    // that queue on that host.

    QueueContextMapIter queueContextIt = d_queues.find(uriKey);

    // NOTE: See TBD in 'onGetDomainDispatched': if the request comes from a
    //       peer inside the cluster, 'clientIdentity' will represent our own
    //       identity instead of that of the peer; which is obviously wrong;
    //       however, here we only want to use it to determine whether the
    //       request comes from a peer node in the cluster (and not a client or
    //       a proxy broker), and so this is still fine.
    if (clientContext->identity().clientType() ==
            bmqp_ctrlmsg::ClientType::E_TCPBROKER &&
        !clientContext->identity().clusterName().empty() &&
        clientContext->identity().clusterNodeId() !=
            mqbnet::Cluster::k_INVALID_NODE_ID) {
        // The request came from a peer in the cluster, make sure we are the
        // primary for the partition.  Since we received the openQueue request
        // from a in-cluster peer node, we should have already received a queue
        // advisory assignment from the leader about that queue; however maybe
        // events will come out of order, so just return a NOT_PRIMARY
        // retryable error in this case and let the peer re-emit a request.
        bsl::string                  reason;
        mqbi::ClusterErrorCode::Enum errorCode =
            mqbi::ClusterErrorCode::e_UNKNOWN;
        if (queueContextIt == d_queues.end()) {
            reason    = "Not aware of that queue";
            errorCode = mqbi::ClusterErrorCode::e_UNKNOWN_QUEUE;
            CALLBACK_FAILURE(reason, errorCode);
            return;  // RETURN
        }
        const int pid = queueContextIt->second->partitionId();
        if (!isSelfAvailablePrimary(pid)) {
            bmqu::MemOutStream errorDesc;
            errorDesc << "Not the primary for partitionId [" << pid << "]";
            reason    = errorDesc.str();
            errorCode = mqbi::ClusterErrorCode::e_NOT_PRIMARY;
            CALLBACK_FAILURE(reason, errorCode);
            return;  // RETURN
        }
    }

    // Create an OpenQueue context for that request.
    OpenQueueContext context;
    context.d_domain_p           = domain;
    context.d_handleParameters   = handleParameters;
    context.d_upstreamSubQueueId = bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID;

    // NOTE: We wrap the 'callback' into the 'openQueueContextCallbackWrapper'
    //       so that we guarantee the 'd_inFlight' counter will always be
    //       decremented after the user provided 'callback' has been invoked.

    // Check if we are already aware of the queue.
    if (queueContextIt != d_queues.end()) {
        // Already aware of the queue; but the queue may not yet have been
        // assigned.
        context.d_queueContext_p = queueContextIt->second.get();
        context.d_callback       = bdlf::BindUtil::bind(
            &openQueueContextCallbackWrapper,
            &context.d_queueContext_p->d_liveQInfo.d_inFlight,
            callback,
            bdlf::PlaceHolders::_1,   // status
            bdlf::PlaceHolders::_2,   // queue
            bdlf::PlaceHolders::_3,   // openQueueResponse
            bdlf::PlaceHolders::_4);  // confirmationCookie

        // Bump 'd_inFlight' counter
        ++(context.d_queueContext_p->d_liveQInfo.d_inFlight);

        // In case queue was marked for expiration, explicitly unmark it.  Note
        // that self may be a replica or a passive primary, but it's ok to
        // simply unmark the queue.  Also note that this is the necessary and
        // sufficient place to unmark a queue, as 'openQueue' is the entry
        // point.
        queueContextIt->second->d_liveQInfo.d_queueExpirationTimestampMs = 0;

        if (isQueuePrimaryAvailable(*(context.d_queueContext_p))) {
            // Queue is already assigned and the primary is AVAILABLE, all
            // good; move on to next step, i.e., processing the open request.
            processOpenQueueRequest(context);
        }
        else {
            // The queue is already known but either not assigned, or its
            // primary is not yet available.  In both scenarios, we append that
            // context to the pending list that will be picked up and resumed
            // once the next event (primary available, queue assigned) happens.
            QueueContext& queueContext = *(queueContextIt->second.get());
            queueContext.d_liveQInfo.d_pending.push_back(context);

            BALL_LOG_INFO_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM << d_cluster_p->description()
                                       << ": Appending openQueue request for '"
                                       << uri << "' from '"
                                       << clientContext->description()
                                       << "' to pending context [";
                if (d_cluster_p->isRemote()) {
                    BALL_LOG_OUTPUT_STREAM
                        << "queueId: "
                        << bmqp::QueueId::QueueIdInt(
                               queueContext.d_liveQInfo.d_id)
                        << ", leaderNode: "
                        << (d_clusterData_p->electorInfo().leaderNode()
                                ? d_clusterData_p->electorInfo()
                                      .leaderNode()
                                      ->nodeDescription()
                                : "** none **");
                }
                else {
                    const int pid = queueContext.partitionId();
                    if (pid == mqbs::DataStore::k_INVALID_PARTITION_ID) {
                        BALL_LOG_OUTPUT_STREAM << "partitionId: invalid";
                    }
                    else {
                        const ClusterStatePartitionInfo& partition =
                            d_clusterState_p->partition(pid);
                        BALL_LOG_OUTPUT_STREAM
                            << "partitionId: " << pid << ", partitionPrimary: "
                            << (partition.primaryNode()
                                    ? partition.primaryNode()
                                          ->nodeDescription()
                                    : "** none **")
                            << ", primaryStatus: "
                            << partition.primaryStatus();
                    }
                }
                BALL_LOG_OUTPUT_STREAM << "]";
            }

            // There *might* be a scenario where there is an active leader, but
            // the queue is still unassigned (depending upon the order in which
            // a new/failover open-queue request and queue-unassignment
            // advisory are received).  So to be safe, we explicitly attempt to
            // assign the queue, which is a no-op in case there is no leader.

            if (!isQueueAssigned(*(queueContextIt->second))) {
                assignQueue(queueContextIt->second);
            }
        }
    }
    else {
        // Unaware of the queue; create the queueContext struct and initiate
        // the assignment procedure.
        QueueContextSp queueContext;
        queueContext.createInplace(d_allocator_p, uriKey, d_allocator_p);

        d_queues[uriKey]         = queueContext;
        context.d_queueContext_p = queueContext.get();
        context.d_callback       = bdlf::BindUtil::bind(
            &openQueueContextCallbackWrapper,
            &context.d_queueContext_p->d_liveQInfo.d_inFlight,
            callback,
            bdlf::PlaceHolders::_1,   // status
            bdlf::PlaceHolders::_2,   // queue
            bdlf::PlaceHolders::_3,   // openQueueResponse
            bdlf::PlaceHolders::_4);  // confirmationCookie

        // Bump 'd_inFlight' counter
        ++(context.d_queueContext_p->d_liveQInfo.d_inFlight);

        // Register the context to the pending list.
        queueContext->d_liveQInfo.d_pending.push_back(context);

        // Initiate the assignment.
        if (QueueAssignmentResult::k_ASSIGNMENT_REJECTED ==
            assignQueue(queueContext)) {
            bdlma::LocalSequentialAllocator<1024> localAllocator(
                d_allocator_p);
            bsl::vector<QueueContext*> rejected(&localAllocator);
            rejected.push_back(queueContext.get());
            processRejectedQueueAssignments(rejected);
        }
    }
}

void ClusterQueueHelper::configureQueue(
    mqbi::Queue*                                       queue,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    unsigned int                                       upstreamSubQueueId,
    const mqbi::QueueHandle::HandleConfiguredCallback& callback)
{
    // executed by the associated *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->dispatcher()->inDispatcherThread(queue));
    BSLS_ASSERT_SAFE(queue->uri().isCanonical());

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::configureQueueDispatched,
                             this,
                             queue->uri(),
                             queue->id(),         // Use upstream queueId
                             upstreamSubQueueId,  // Use upstream subQueueId
                             streamParameters,
                             callback),
        d_cluster_p);
}

void ClusterQueueHelper::configureQueue(
    mqbi::Queue*                                 queue,
    const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters,
    unsigned int                                 upstreamSubQueueId,
    const mqbi::Cluster::HandleReleasedCallback& callback)
{
    // executed by the associated *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->dispatcher()->inDispatcherThread(queue));

    // TBD: Populate the 'bmqp_ctrlmsg::SubQueueIdInfo' of the handleParameters
    //      with subStream-specific (appId, upstreamSubQueueId) if applicable.
    //      Note that handleParameters passed here are from the 'QueueContext'
    //      and thus should not be relied upon to retrieve the appId.  Hence,
    //      this method may additionally require a 'appId' argument.

    bmqp_ctrlmsg::QueueHandleParameters handleParams = handleParameters;
    handleParams.qId()                               = queue->id();

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::releaseQueueDispatched,
                             this,
                             handleParams,
                             upstreamSubQueueId,
                             callback),
        d_cluster_p);
}

void ClusterQueueHelper::onQueueHandleCreated(mqbi::Queue*     queue,
                                              const bmqt::Uri& uri,
                                              bool             handleCreated)
{
    // executed by the associated *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->dispatcher()->inDispatcherThread(queue));
    BSLS_ASSERT_SAFE(uri.isCanonical());

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(
            &ClusterQueueHelper::onQueueHandleCreatedDispatched,
            this,
            queue,
            uri,
            handleCreated),
        d_cluster_p);
}

void ClusterQueueHelper::onQueueHandleDestroyed(mqbi::Queue*     queue,
                                                const bmqt::Uri& uri)
{
    // executed by *ANY* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(uri.isCanonical());

    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(
            &ClusterQueueHelper::onQueueHandleDestroyedDispatched,
            this,
            queue,
            uri),
        d_cluster_p);
}

void ClusterQueueHelper::processPeerOpenQueueRequest(
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbc::ClusterNodeSession*           requester)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(request.choice().isOpenQueueValue());
    BSLS_ASSERT_SAFE(request.choice().openQueue().handleParameters().qId() !=
                     bmqp::QueueId::k_UNASSIGNED_QUEUE_ID);

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Received openQueueRequest from '"
                  << requester->description() << "': " << request;

    // Unlike in 'mqba::ClientSession::processOpenQueue()', this request is
    // only invoked when receiving a request from a peer node in the cluster,
    // so we 1) trust the source and don't need to validate the URI, 2) don't
    // have to qualify the domain.

    // Also note that this is the *only* entry point for an open-queue request
    // received from a peer in the cluster.

    if (d_cluster_p->isStopping()) {
        bmqp_ctrlmsg::ControlMessage response;
        bmqp_ctrlmsg::Status&        status = response.choice().makeStatus();

        response.rId()    = request.rId();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()     = mqbi::ClusterErrorCode::e_STOPPING;
        status.message()  = k_SELF_NODE_IS_STOPPING;

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());
        return;  // RETURN
    }

    const bmqp_ctrlmsg::OpenQueue& req = request.choice().openQueue();
    const bmqp_ctrlmsg::QueueHandleParameters& handleParams =
        req.handleParameters();

    if (bmqp::QueueUtil::isEmpty(handleParams)) {
        // This code path is not expected to bit hit, so protect against it,
        // and alarm for investigation.
        BMQTSK_ALARMLOG_ALARM("INVALID_OPENQUEUE_REQ")
            << d_cluster_p->description()
            << ": Rejecting invalid openQueueRequest from '"
            << requester->description() << "': " << request
            << BMQTSK_ALARMLOG_END;

        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

        response.rId() = request.rId();
        response.choice().makeStatus();
        response.choice().status().category() =
            bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        response.choice().status().message() =
            "At least one of [read|write|admin]Count must be > 0";

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_clusterData_p->domainFactory());
    d_clusterData_p->domainFactory()->createDomain(
        bmqt::Uri(handleParams.uri()).qualifiedDomain(),
        bdlf::BindUtil::bind(&ClusterQueueHelper::onGetDomain,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // domain
                             request,
                             requester,
                             requester->peerInstanceId()));
}

void ClusterQueueHelper::processPeerConfigureStreamRequest(
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbc::ClusterNodeSession*           requester)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    bmqp_ctrlmsg::ConfigureStream  adaptor;
    bmqp_ctrlmsg::ConfigureStream& req = adaptor;

    if (request.choice().isConfigureQueueStreamValue()) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Received configureQueueStreamRequest from ["
                      << requester->description() << "]: " << request;

        bmqp::ProtocolUtil::convert(&adaptor,
                                    request.choice().configureQueueStream());
    }
    else {
        BSLS_ASSERT_SAFE(request.choice().isConfigureStreamValue());

        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Received configureStreamRequest from ["
                      << requester->description() << "]: " << request;

        req = request.choice().configureStream();
    }

    // Lookup the handle.
    CNSQueueHandleMapCIter it = requester->queueHandles().find(req.qId());
    if (it == requester->queueHandles().end()) {
        // Failure.

        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Received configureQueueStream request from ["
                      << requester->description()
                      << "] for a queue with unknown Id "
                      << "(" << req.qId() << ").";

        // Send error response.

        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

        response.rId()               = request.rId();
        bmqp_ctrlmsg::Status& status = response.choice().makeStatus();

        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Unknown queueId";

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());

        return;  // RETURN
    }

    const CNSQueueState& queueContext = it->second;
    if (queueContext.d_isFinalCloseQueueReceived) {
        BALL_LOG_WARN << d_cluster_p->description()
                      << "Received unexpected configureQueue request from '"
                      << requester->description() << "' for queue with Id ("
                      << req.qId()
                      << "), for which final closeQueue request was already "
                      << "received.";

        // Send error response
        bmqp_ctrlmsg::ControlMessage response;

        response.rId()               = request.rId();
        bmqp_ctrlmsg::Status& status = response.choice().makeStatus();

        status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()     = -1;
        status.message()  = "Unexpected configureQueue request";

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());

        return;  // RETURN
    }

    mqbi::QueueHandle* handle = queueContext.d_handle_p;

    // Validate subQueueId (if specified)
    CNSStreamsMap::const_iterator sqiIter =
        it->second.d_subQueueInfosMap.findByAppIdSafe(
            req.streamParameters().appId());
    if (sqiIter == it->second.d_subQueueInfosMap.end()) {
        BALL_LOG_WARN << d_cluster_p->description()
                      << "Received configureQueueStream request from ["
                      << requester->description() << "] for a queue with id ("
                      << req.qId() << ") and unknown appId ("
                      << req.streamParameters().appId() << ").";

        // Send error response.
        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

        response.rId()               = request.rId();
        bmqp_ctrlmsg::Status& status = response.choice().makeStatus();

        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Unknown queueId";

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());

        return;  // RETURN
    }

    // Configure the queue handle.

    handle->configure(
        req.streamParameters(),
        bdlf::BindUtil::bind(&ClusterQueueHelper::onHandleConfigured,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // config
                             request,
                             requester));
}

void ClusterQueueHelper::processPeerCloseQueueRequest(
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbc::ClusterNodeSession*           requester)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(request.choice().isCloseQueueValue());

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Received closeQueueRequest from '"
                  << requester->description() << "': " << request;

    const bmqp_ctrlmsg::CloseQueue& req = request.choice().closeQueue();
    const bmqp::QueueId             queueId =
        bmqp::QueueUtil::createQueueIdFromHandleParameters(
            req.handleParameters());
    // Lookup the handle
    CNSQueueHandleMapIter it = requester->queueHandles().find(queueId.id());
    if (it == requester->queueHandles().end()) {
        // Failure ...
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Received closeQueue request from '"
                      << requester->description()
                      << "' for a queue with unknown id (" << queueId << ")";

        // Send error response
        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

        response.rId()               = request.rId();
        bmqp_ctrlmsg::Status& status = response.choice().makeStatus();

        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Unknown queueId";

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());

        return;  // RETURN
    }

    CNSQueueState& queueContext = it->second;
    if (queueContext.d_isFinalCloseQueueReceived) {
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Received closeQueue request from '"
                      << requester->description() << "' for queue with Id ("
                      << queueId
                      << "), for which final closeQueue request was already "
                      << "received.";

        // Send error response
        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

        response.rId()               = request.rId();
        bmqp_ctrlmsg::Status& status = response.choice().makeStatus();

        status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()     = -1;
        status.message()  = "Duplicate closeQueue request";

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());

        return;  // RETURN
    }

    mqbi::QueueHandle* handle = queueContext.d_handle_p;

    // Validate subQueueId (if specified)
    const unsigned int subId = bmqp::QueueUtil::extractSubQueueId(
        req.handleParameters());
    CNSStreamsMap::const_iterator sqiIter =
        it->second.d_subQueueInfosMap.findBySubIdSafe(subId);
    if (sqiIter == it->second.d_subQueueInfosMap.end()) {
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Received closeQueue request from ["
                      << requester->description()
                      << "] for a queue with unknown subQueueId (" << queueId
                      << ").";

        // Send error response.
        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

        response.rId()               = request.rId();
        bmqp_ctrlmsg::Status& status = response.choice().makeStatus();

        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Unknown subQueueId";

        d_clusterData_p->messageTransmitter().sendMessage(
            response,
            requester->clusterNode());

        return;  // RETURN
    }

    queueContext.d_isFinalCloseQueueReceived = req.isFinal();

    // Release the queueHandle
    handle->release(req.handleParameters(),
                    req.isFinal(),
                    bdlf::BindUtil::bind(&ClusterQueueHelper::onHandleReleased,
                                         this,
                                         bdlf::PlaceHolders::_1,  // handle
                                         bdlf::PlaceHolders::_2,  // result
                                         request,
                                         requester));
}

void ClusterQueueHelper::processShutdownEvent()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    // Need to delete and unregister all queues which have no handles from the
    // domain.  Such queues will eventually be gc'd but that may take a while,
    // something that we don't want when shutting down.

    for (QueueContextMapIter it = d_queues.begin(); it != d_queues.end();
         ++it) {
        QueueContextSp& queueContextSp = it->second;
        QueueLiveState& qinfo          = queueContextSp->d_liveQInfo;
        mqbi::Queue*    queue          = qinfo.d_queue_sp.get();

        if (!queue) {
            continue;  // CONTINUE
        }

        if (0 != qinfo.d_numQueueHandles) {
            // Queue has non-zero handles.  Since self is stopping, self will
            // receive/send close-queue requests for this queue, and eventually
            // num handles will go to zero, and queue will be removed.

            continue;  // CONTINUE
        }

        // Queue has no handles.

        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Deleting queue instance [" << queue->uri()
                      << "], queueKey [" << queueContextSp->key()
                      << "] which was assigned to Partition ["
                      << queueContextSp->partitionId()
                      << "], because self is going down.";

        deleteQueue(queueContextSp.get());
    }
}

/// Stop sending PUSHes but continue receiving CONFIRMs, receiving and
/// sending PUTs and ACKs.
void ClusterQueueHelper::requestToStopPushing()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    // Assume Shutdown V2
    d_supportShutdownV2 = true;

    // Prevent future queue operations from sending PUSHes.
    for (QueueContextMapIter it = d_queues.begin(); it != d_queues.end();
         ++it) {
        QueueContextSp& queueContextSp = it->second;
        QueueLiveState& qinfo          = queueContextSp->d_liveQInfo;
        mqbi::Queue*    queue          = qinfo.d_queue_sp.get();

        if (!queue) {
            continue;  // CONTINUE
        }

        queue->dispatcher()->execute(
            bdlf::BindUtil::bind(&mqbi::Queue::stopPushing, queue),
            queue);
    }
}

void ClusterQueueHelper::contextHolder(
    const bsl::shared_ptr<StopContext>& contextSp,
    const VoidFunctor&                  action)
{
    if (action) {
        action();
    }
    (void)contextSp;
}

void ClusterQueueHelper::processNodeStoppingNotification(
    mqbnet::ClusterNode*                clusterNode,
    const bmqp_ctrlmsg::ControlMessage* request,
    mqbc::ClusterNodeSession*           ns,
    const VoidFunctor&                  callback)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(clusterNode);

    // The 'shared_ptr' serves as a reference count of all pending queue
    // operations.  Once all functors complete, the 'finishStopSequence'
    // deleter sends back StopResponse.

    // TODO(shutdown-v2): TEMPORARY, remove 'timeout' when all switch to
    // StopRequest V2.

    // No need to wait for CONFIRMs, the waiting is done by the shutting down
    // node.

    int timeout =
        d_clusterData_p->clusterConfig().queueOperations().stopTimeoutMs();

    bsl::shared_ptr<StopContext> contextSp(
        new (*d_allocator_p)
            StopContext(clusterNode, callback, timeout, d_allocator_p),
        bdlf::BindUtil::bind(&ClusterQueueHelper::finishStopSequence,
                             this,
                             bdlf::PlaceHolders::_1),  // context
        d_allocator_p);

    if (request) {
        // Take the name of the cluster from the request, not the local
        // 'd_cluster_p->name()'.  The latter may refer to the virtual cluster
        // as it is exposed to this node, but the stop request may contain the
        // name of the original cluster.
        contextSp->d_response.choice()
            .makeClusterMessage()
            .choice()
            .makeStopResponse()
            .clusterName() = request->choice()
                                 .clusterMessage()
                                 .choice()
                                 .stopRequest()
                                 .clusterName();
        contextSp->d_response.rId() = request->rId();
    }

    // If this node is already processing StopRequest from the same
    // 'clusterNode', do not start another processing.

    if (setStopContext(clusterNode, contextSp)) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": starting processing StopRequest/advisory from "
                      << clusterNode->nodeDescription()
                      << " with timeout (ms) " << timeout;

        // TODO(shutdown-v2): TEMPORARY, remove when all switch to StopRequest
        // V2.
        bool supportShutdownV2 = true;

        if (request) {
            const bmqp_ctrlmsg::StopRequest& stopRequest =
                request->choice().clusterMessage().choice().stopRequest();

            if (stopRequest.version() == 1) {
                supportShutdownV2 = false;
            }
            else {
                BSLS_ASSERT_SAFE(stopRequest.version() == 2);
            }
        }
        // StopRequests have replaced E_STOPPING advisory.
        // In any case, do minimal (V2) work unless explicitly requested

        if (supportShutdownV2) {
            if (ns) {
                // As an Upstream, deconfigure queues of the (shutting down)
                // ClusterNodeSession 'ns'.
                // Call 'mqbi::QueueHandle::deconfigureAll' for each handle

                const mqbc::ClusterNodeSession::QueueHandleMap& handles =
                    ns->queueHandles();

                for (mqbc::ClusterNodeSession::QueueHandleMap::const_iterator
                         cit = handles.begin();
                     cit != handles.end();
                     ++cit) {
                    cit->second.d_handle_p->deconfigureAll(
                        bdlf::BindUtil::bind(
                            &ClusterQueueHelper::contextHolder,
                            this,
                            contextSp,
                            VoidFunctor()));
                }
                BALL_LOG_INFO << d_clusterData_p->identity().description()
                              << ": deconfigured " << handles.size()
                              << " handles while processing StopRequest from "
                              << clusterNode->nodeDescription() << " "
                              << contextSp.numReferences();
            }
            // else, this is a ClusterProxy (downstream) receiving request from
            // an upstream Cluster Node (a request from a Proxy would arrive to
            // ClientSession).
            // Downstreams do not deconfigure queues in V2.
            // See comment in 'ClusterProxy::processPeerStopRequest'

            // As a Downstream, notify relevant queues about their shutting
            // down upstream
            for (QueueContextMapConstIter cit = d_queues.begin();
                 cit != d_queues.end();
                 ++cit) {
                const QueueContextSp& queueContextSp = cit->second;
                const QueueLiveState& queueLiveState =
                    queueContextSp->d_liveQInfo;
                mqbi::Queue* queue = queueLiveState.d_queue_sp.get();

                if (0 == queue || bmqp::QueueId::k_UNASSIGNED_QUEUE_ID ==
                                      queueContextSp->d_liveQInfo.d_id) {
                    continue;  // CONTINUE
                }

                if (!d_cluster_p->isRemote()) {
                    const int pid = queueContextSp->partitionId();

                    BSLS_ASSERT_SAFE(ns);

                    const bsl::vector<int>& partitions =
                        ns->primaryPartitions();
                    if (partitions.end() ==
                        bsl::find(partitions.begin(), partitions.end(), pid)) {
                        continue;  // CONTINUE
                    }
                    const ClusterStatePartitionInfo& pinfo =
                        d_clusterState_p->partition(pid);

                    if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE !=
                        pinfo.primaryStatus()) {
                        // It's possible for a primary node to be non-active
                        // when it is shutting down -- if it was stopped before
                        // the node had a chance to transition to active
                        // primary for this partition.

                        continue;  // CONTINUE
                    }
                    BSLS_ASSERT(pinfo.primaryNode() == clusterNode);
                }
                else if (d_clusterData_p->electorInfo().leaderNode() !=
                         clusterNode) {
                    continue;  // CONTINUE
                }

                if (queueLiveState.d_subQueueIds.findBySubIdSafe(
                        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) ==
                    queueLiveState.d_subQueueIds.end()) {
                    // Only buffering PUTs.  Still sending CONFIRMs
                    continue;  // CONTINUE
                }

                VoidFunctor inner = bdlf::BindUtil::bind(
                    &mqbi::Queue::onOpenUpstream,
                    queue,
                    0,
                    bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID,
                    true);

                VoidFunctor outer = bdlf::BindUtil::bind(
                    &ClusterQueueHelper::contextHolder,
                    this,
                    contextSp,
                    inner);

                queue->dispatcher()->execute(
                    outer,
                    queue,
                    mqbi::DispatcherEventType::e_DISPATCHER);

                // Use 'mqbi::DispatcherEventType::e_DISPATCHER' to avoid
                // (re)enabling 'd_flushList'
            }
        }
        else {
            // TODO(shutdown-v2): TEMPORARY, remove when all switch to
            // StopRequest V2.
            // Downstreams do not need to deconfigure queues for which the
            // shutting down node is the upstream.  The deconfiguring is done
            // by the upstream of the shutting down node instead.
            // Nor do they need to wait for CONFIRMs, the waiting is done by
            // the shutting down node.

            if (ns) {
                deconfigureQueues(contextSp, &ns->primaryPartitions());
            }
            else if (d_clusterData_p->electorInfo().leaderNode() ==
                     contextSp->d_peer) {
                deconfigureQueues(contextSp, 0);
            }
        }
    }
    else {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": chaining processing StopRequest/advisory from "
                      << clusterNode->nodeDescription()
                      << " to the previous one";
    }
}

void ClusterQueueHelper::onLeaderAvailable()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    // This routine is invoked only in the cluster nodes.

    BSLS_ASSERT_SAFE(!d_cluster_p->isRemote());

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": On leader available, restoring state.";

    restoreState(mqbs::DataStore::k_ANY_PARTITION_ID);
}

bool ClusterQueueHelper::setStopContext(
    const mqbnet::ClusterNode*          clusterNode,
    const bsl::shared_ptr<StopContext>& contextSp)
{
    bsl::weak_ptr<StopContext>&  currentWp = d_stopContexts[clusterNode];
    bsl::shared_ptr<StopContext> currentSp = currentWp.lock();
    bool                         result    = true;

    if (currentSp) {
        // There is another StopContext for the same `clusterNode`.
        BSLS_ASSERT_SAFE(!currentSp->d_previous_sp);
        currentSp->d_previous_sp = contextSp;

        result = false;
    }
    d_stopContexts[clusterNode] = contextSp;

    return result;
}

void ClusterQueueHelper::finishStopSequence(StopContext* context)
{
    // executed by *ANY* thread
    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::finishStopSequenceDispatched,
                             this,
                             context),
        d_cluster_p);
}

void ClusterQueueHelper::finishStopSequenceDispatched(StopContext* context)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    if (!context->d_response.choice().isUndefinedValue()) {
        d_clusterData_p->messageTransmitter().sendMessage(context->d_response,
                                                          context->d_peer);
    }
    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": finishing processing StopRequest/advisory from "
                  << context->d_peer->nodeDescription();

    if (context->d_callback) {
        context->d_callback();
    }

    d_allocator_p->deleteObject(context);
}

void ClusterQueueHelper::deconfigureQueues(
    const bsl::shared_ptr<StopContext>& contextSp,
    const bsl::vector<int>*             partitions)
{
    // TODO(shutdown-v2): TEMPORARY, remove when all switch to StopRequest V2.

    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    // This routine is invoked when a peer node, which is primary for the
    // specified 'partitions' is CLOSING.  Self node needs to issue close
    // queue requests for all the queues that self has opened on the peer, so
    // that peer can go down gracefully.

    if (partitions && partitions->empty()) {
        return;  // RETURN
    }

    for (QueueContextMapConstIter cit = d_queues.begin();
         cit != d_queues.end();
         ++cit) {
        const QueueContextSp& queueContextSp = cit->second;
        const QueueLiveState& queueLiveState = queueContextSp->d_liveQInfo;
        mqbi::Queue*          queue          = queueLiveState.d_queue_sp.get();

        if (0 == queue || bmqp::QueueId::k_UNASSIGNED_QUEUE_ID ==
                              queueContextSp->d_liveQInfo.d_id) {
            continue;  // CONTINUE
        }

        if (!d_cluster_p->isRemote()) {
            const int pid = queueContextSp->partitionId();

            BSLS_ASSERT_SAFE(partitions);

            if (partitions->end() ==
                bsl::find(partitions->begin(), partitions->end(), pid)) {
                continue;  // CONTINUE
            }
            const ClusterStatePartitionInfo& pinfo =
                d_clusterState_p->partition(pid);

            if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE !=
                pinfo.primaryStatus()) {
                // It's possible for a primary node to be non-active when it is
                // shutting down -- if it was stopped before the node had a
                // chance to transition to active primary for this partition.

                continue;  // CONTINUE
            }
            BSLS_ASSERT(pinfo.primaryNode() == contextSp->d_peer);
        }
        else {
            BSLS_ASSERT(d_clusterData_p->electorInfo().leaderNode() ==
                        contextSp->d_peer);
        }

        if (queue->isAtMostOnce()) {
            // Stop broadcast PUTs before deconfiguring queues.  Because there
            // is no persistence, broadcast PUTs get dropped in the absence of
            // consumers.  If RemoteQueue starts buffering earlier, they can be
            // retransmitted.
            queue->dispatcher()->execute(
                bdlf::BindUtil::bind(&mqbi::Queue::onOpenUpstream,
                                     queue,
                                     0,
                                     bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID,
                                     true),  // isWriterOnly
                queue);
            queue->dispatcher()->synchronize(queue);

            // Need to drain own dispatcher queue which may contain PUTs before
            // sending deconfigure.
            d_cluster_p->dispatcher()->execute(
                bdlf::BindUtil::bind(&ClusterQueueHelper::deconfigureUri,
                                     this,
                                     contextSp,
                                     cit->first),
                d_cluster_p);
        }
        else {
            deconfigureQueue(contextSp, queueContextSp);
        }
    }
}

void ClusterQueueHelper::deconfigureUri(
    const bsl::shared_ptr<StopContext>& contextSp,
    const bmqt::Uri&                    uri)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    if (d_cluster_p->isStopping()) {
        return;  // RETURN
    }

    QueueContextMapConstIter cit = d_queues.find(uri);
    if (cit == d_queues.end()) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Skipping deconfiguring '" << uri
                      << "' because the queue is missing";
        return;  // RETURN
    }

    const QueueContextSp& queueContextSp = cit->second;
    const QueueLiveState& queueLiveState = queueContextSp->d_liveQInfo;
    mqbi::Queue*          queue          = queueLiveState.d_queue_sp.get();

    BSLS_ASSERT(queue);

    if (!d_cluster_p->isRemote()) {
        const int pid = queueContextSp->partitionId();

        const ClusterStatePartitionInfo& pinfo = d_clusterState_p->partition(
            pid);

        if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE != pinfo.primaryStatus()) {
            // It's possible for a primary node to be non-active when it is
            // shutting down -- if it was stopped before the node had a
            // chance to transition to active primary for this partition.

            BALL_LOG_INFO << d_cluster_p->description()
                          << ": Skipping deconfiguring queue ["
                          << queue->description()
                          << "] because the primary node status is "
                          << pinfo.primaryStatus();
            return;  // RETURN
        }
        BSLS_ASSERT(pinfo.primaryNode() == contextSp->d_peer);
    }
    else {
        BSLS_ASSERT(d_clusterData_p->electorInfo().leaderNode() ==
                    contextSp->d_peer);
    }

    // Primary is active; send configure-queue request, wait for response,
    // send close-queue request

    deconfigureQueue(contextSp, queueContextSp);
}

void ClusterQueueHelper::deconfigureQueue(
    const bsl::shared_ptr<StopContext>& contextSp,
    const QueueContextSp&               queueContextSp)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    QueueLiveState& queueLiveState = queueContextSp->d_liveQInfo;
    mqbi::Queue*    queue          = queueLiveState.d_queue_sp.get();

    // Keep `0` as `genCount` to indicate StopRequest processing to the
    // queue.
    bsls::Types::Uint64 genCount = 0;

    for (StreamsMap::iterator iter = queueLiveState.d_subQueueIds.begin();
         iter != queueLiveState.d_subQueueIds.end();
         ++iter) {
        SubQueueContext::Enum state = iter->value().d_state;

        if (state == SubQueueContext::k_OPEN) {
            // Deconfigure only open subStreams.

            bmqp_ctrlmsg::StreamParameters nullStreamParams;
            nullStreamParams.appId() = iter->appId();
            /* TODO
            nullStreamParams.maxUnconfirmedMessages() = 0;
            nullStreamParams.maxUnconfirmedBytes()    = 0;
            nullStreamParams.consumerPriority()       =
                                bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID;
            */
            BALL_LOG_INFO << d_cluster_p->description()
                          << ": Sending '(de)configureQueue' request for queue"
                          << " [" << queue->description()
                          << ", appId: " << iter->appId()
                          << ", subId: " << iter->subId() << "] to "
                          << contextSp->d_peer->nodeDescription()
                          << " as part of destination node's stop sequence";

            sendConfigureQueueRequest(
                nullStreamParams,
                queue->id(),
                queue->uri(),
                bdlf::BindUtil::bind(&ClusterQueueHelper::continueStopSequence,
                                     this,
                                     contextSp,
                                     queueContextSp,
                                     iter->subId(),
                                     bdlf::PlaceHolders::_1,
                                     bdlf::PlaceHolders::_2),
                false,  // is not a reconfigure-queue request
                contextSp->d_peer,
                genCount,
                iter->subId());
        }
        else {
            BALL_LOG_INFO << d_cluster_p->description()
                          << ": Not sending '(de)configureQueue' request for"
                          << " queue [" << queue->description()
                          << ", appId: " << iter->appId()
                          << ", subId: " << iter->subId() << "] to "
                          << contextSp->d_peer->nodeDescription()
                          << " as part of destination node's stop sequence"
                          << " because the subStream state is " << state;

            // Do not send Close request if the sate is not k_OPEN.
            //
            // REVISIT: If the state is 'k_OPENING', parking Close request can
            // result in a deadlock if the upstream has cached Reopen request
            // and then sent the StopRequest.  Assume 'k_OPENING' subStreams
            // cannot have unconfirmed messages and there is no need to wait.
            // The StopRequest processing will not explicitly close 'k_OPENING'
            // subStreams but the upstream will close them as part of shutdown
            // event ('processShutdownEvent').
            // Just do not to send configure request if Reopen response does
            // arrive.
            if (state == SubQueueContext::k_REOPENING) {
                iter->value().d_state = SubQueueContext::k_CLOSED;
            }
        }
    }
}

void ClusterQueueHelper::continueStopSequence(
    const bsl::shared_ptr<StopContext>& contextSp,
    const QueueContextSp&               queueContextSp,
    unsigned int                        subId,
    const bmqp_ctrlmsg::Status&         status,
    BSLS_ANNOTATION_UNUSED const        bmqp_ctrlmsg::StreamParameters&
                                        streamParameters)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    if (status.category() == bmqp_ctrlmsg::StatusCategory::E_CANCELED) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": canceling processing StopRequest/advisory from "
                      << contextSp->d_peer->nodeDescription() << " for queue ["
                      << queueContextSp->uri() << ", subId " << subId << "]";
    }
    else {
        checkUnconfirmed(contextSp, queueContextSp, subId);
    }
}

void ClusterQueueHelper::checkUnconfirmed(
    const bsl::shared_ptr<StopContext>& contextSp,
    const QueueContextSp&               queueContextSp,
    unsigned int                        subId)
{
    // executed by *ANY* thread
    BSLS_ASSERT_SAFE(queueContextSp);

    if (d_cluster_p->isStopping()) {
        return;  // RETURN
    }

    bsl::shared_ptr<mqbblp::Queue>& queueSp =
        queueContextSp->d_liveQInfo.d_queue_sp;

    if (!queueSp) {
        // the queue has been deleted while we were counting unconfirmed
        // messages.  No need to send Close Queue Request.
        return;  // RETURN
    }

    queueSp->dispatcher()->execute(
        bdlf::BindUtil::bind(
            &ClusterQueueHelper::checkUnconfirmedQueueDispatched,
            this,
            contextSp,
            queueContextSp,
            subId),
        queueSp.get());
}

void ClusterQueueHelper::checkUnconfirmedV2(
    const bsls::TimeInterval&    whenToStop,
    const bsl::function<void()>& completionCallback)
{
    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::checkUnconfirmedV2Dispatched,
                             this,
                             whenToStop,
                             completionCallback),
        d_cluster_p);
}

void ClusterQueueHelper::checkUnconfirmedV2Dispatched(
    const bsls::TimeInterval&    whenToStop,
    const bsl::function<void()>& completionCallback)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    bsls::Types::Int64 result = 0;
    for (QueueContextMapIter it = d_queues.begin(); it != d_queues.end();
         ++it) {
        QueueContextSp& queueContextSp = it->second;
        QueueLiveState& qinfo          = queueContextSp->d_liveQInfo;
        mqbi::Queue*    queue          = qinfo.d_queue_sp.get();

        if (!queue) {
            continue;  // CONTINUE
        }

        queue->dispatcher()->execute(
            bdlf::BindUtil::bind(&countUnconfirmed, &result, queue),
            queue);
        queue->dispatcher()->synchronize(queue);
    }

    // Synchronize with all Queue Dispatcher threads
    bslmt::Latch latch(1);
    d_cluster_p->dispatcher()->execute(
        mqbi::Dispatcher::VoidFunctor(),  // empty
        mqbi::DispatcherClientType::e_QUEUE,
        bdlf::BindUtil::bind(&bslmt::Latch::arrive, &latch));

    latch.wait();

    if (result == 0) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": no unconfirmed message(s)";

        completionCallback();
        return;
    }

    bsls::TimeInterval t = bsls::SystemTime::now(
        bsls::SystemClockType::e_MONOTONIC);

    if (t < whenToStop) {
        BALL_LOG_INFO << d_cluster_p->description() << ": waiting for "
                      << result << " unconfirmed message(s)";

        t.addSeconds(1);
        if (t > whenToStop) {
            t = whenToStop;
        }
        bdlmt::EventScheduler::EventHandle eventHandle;
        // Never cancel the timer
        d_clusterData_p->scheduler().scheduleEvent(
            &eventHandle,
            t,
            bdlf::BindUtil::bind(&ClusterQueueHelper::checkUnconfirmedV2,
                                 this,
                                 whenToStop,
                                 completionCallback));

        return;  // RETURN
    }
    else {
        BALL_LOG_WARN << d_cluster_p->description() << ": giving up on "
                      << result << " unconfirmed message(s)";
        completionCallback();
    }
}

void ClusterQueueHelper::checkUnconfirmedQueueDispatched(
    const bsl::shared_ptr<StopContext>& contextSp,
    const QueueContextSp&               queueContextSp,
    unsigned int                        subId)
{
    // This function is scheduled by cluster to check if the count of
    // unconfirmed messages is 0, or timer expired in which case the next step
    // - 'closeQueueDispatched' - is scheduled .
    // Otherwise, schedule next check in 1 second (or less if the remaining
    // timeout is less than a second).

    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueContextSp);
    bsl::shared_ptr<mqbblp::Queue>& queueSp =
        queueContextSp->d_liveQInfo.d_queue_sp;
    BSLS_ASSERT_SAFE(queueSp);
    BSLS_ASSERT_SAFE(queueSp->dispatcher()->inDispatcherThread(queueSp.get()));

    bsls::Types::Int64 count = queueSp->countUnconfirmed(subId);

    if (count) {
        bsls::TimeInterval t = bsls::SystemTime::now(
            bsls::SystemClockType::e_MONOTONIC);

        if (t < contextSp->d_stopTime) {
            BALL_LOG_INFO << d_cluster_p->description() << ": waiting for "
                          << count << " unconfirmed message(s) for queue ["
                          << queueSp->description() << ", subId " << subId
                          << "] while processing StopRequest/advisory from "
                          << contextSp->d_peer->nodeDescription();

            t.addSeconds(1);
            if (t > contextSp->d_stopTime) {
                t = contextSp->d_stopTime;
            }
            d_cluster_p->dispatcher()->execute(
                bdlf::BindUtil::bind(
                    &ClusterQueueHelper::waitForUnconfirmedDispatched,
                    this,
                    contextSp,
                    queueContextSp,
                    subId,
                    t),
                d_cluster_p);
            return;  // RETURN
        }
        else {
            BALL_LOG_WARN << d_cluster_p->description() << ": giving up on "
                          << count << " unconfirmed message(s) for queue ["
                          << queueSp->description() << ", subId " << subId
                          << "] while processing StopRequest/advisory from "
                          << contextSp->d_peer->nodeDescription();
        }
    }
    else {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": no unconfirmed message(s) for queue ["
                      << queueSp->description() << ", subId " << subId
                      << "] while processing StopRequest/Stopping Advisory "
                      << "from " << contextSp->d_peer->nodeDescription();
    }

    // Since we are already in the queue dispatcher thread and we are going to
    // close the queue, notify the queue so it starts CONFIRMs buffering.
    //
    // Once 'checkUnconfirmedQueueDispatched' reaches this point, it will not
    // execute again.

    queueSp->onOpenUpstream(0, subId);
    d_cluster_p->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::closeQueueDispatched,
                             this,
                             contextSp,
                             queueSp,
                             subId),
        d_cluster_p);
}

void ClusterQueueHelper::waitForUnconfirmedDispatched(
    const bsl::shared_ptr<StopContext>& contextSp,
    const QueueContextSp&               queueContextSp,
    unsigned int                        subId,
    const bsls::TimeInterval&           t)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(queueContextSp);

    if (d_cluster_p->isStopping()) {
        return;  // RETURN
    }

    StreamsMap& subQueueIds = queueContextSp->d_liveQInfo.d_subQueueIds;
    StreamsMap::iterator subStreamIt = subQueueIds.findBySubIdSafe(subId);

    if (subStreamIt != subQueueIds.end()) {
        subStreamIt->value().d_timer.release();
        d_clusterData_p->scheduler().scheduleEvent(
            &subStreamIt->value().d_timer,
            t,
            bdlf::BindUtil::bind(&ClusterQueueHelper::checkUnconfirmed,
                                 this,
                                 contextSp,
                                 queueContextSp,
                                 subId));
    }
}

void ClusterQueueHelper::closeQueueDispatched(
    const bsl::shared_ptr<StopContext>& contextSp,
    const bsl::shared_ptr<mqbi::Queue>& queueSp,
    unsigned int                        subId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    // Send close-queue request with 'full' handle parameters.  Note that
    // we *must* *not* update self's _upstream_ view on that subQueueId
    // because self will reopen the queue when a new primary is assigned to the
    // partition; in other words, this close-queue was not initiated by
    // downstream, but by upstream.

    QueueContextMapIter itQueue = d_queues.find(queueSp->uri());
    if (itQueue == d_queues.end()) {
        // the queue has been deleted while we were counting unconfirmed
        // messages.  No need to send Close Queue Request.

        return;  // RETURN
    }

    const QueueContextSp& queueContextSp = itQueue->second;
    StreamsMap::iterator  itStream =
        queueContextSp->d_liveQInfo.d_subQueueIds.findBySubIdSafe(subId);

    if (itStream == queueContextSp->d_liveQInfo.d_subQueueIds.end()) {
        // the stream has been deleted while we were counting unconfirmed
        // messages.  No need to send Close Queue Request.

        return;  // RETURN
    }
    itStream->value().d_timer.release();

    const mqbi::Queue* queue = queueContextSp->d_liveQInfo.d_queue_sp.get();
    BSLS_ASSERT_SAFE(queue);

    SubQueueContext::Enum state = itStream->value().d_state;

    if (state != SubQueueContext::k_OPEN) {
        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Not sending 'closeQueue' requests for queue ["
                      << queue->description() << ", " << itStream->appId()
                      << "(" << itStream->subId() << ")] to "
                      << contextSp->d_peer->nodeDescription()
                      << " because the state is not OPEN: " << state;
        return;  // RETURN
    }

    bmqp_ctrlmsg::QueueHandleParameters hp(itStream->value().d_parameters);

    hp.qId() = queueContextSp->d_liveQInfo.d_id;

    // Do not update Upstream Params
    sendCloseQueueRequest(
        hp,
        bdlf::BindUtil::bind(&ClusterQueueHelper::onCloseQueueResponse,
                             this,
                             contextSp,
                             bdlf::PlaceHolders::_1),  // Status
        contextSp->d_peer);

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": Sent 'closeQueue' request for queue ["
                  << queue->description() << ", " << itStream->appId() << "("
                  << itStream->subId() << ")] to "
                  << contextSp->d_peer->nodeDescription();

    itStream->value().d_state = SubQueueContext::k_CLOSED;
}

void ClusterQueueHelper::onCloseQueueResponse(
    const bsl::shared_ptr<StopContext>& contextSp,
    const bmqp_ctrlmsg::Status&         status)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    BALL_LOG_INFO << d_cluster_p->description()
                  << ": received CloseQueue response with status '" << status
                  << "' while processing StopRequest/advisory from "
                  << contextSp->d_peer->nodeDescription();
}

int ClusterQueueHelper::gcExpiredQueues(bool immediate)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    enum RcEnum {
        rc_SUCCESS             = 0,
        rc_CLUSTER_IS_STOPPING = -1,
        rc_SELF_IS_NOT_PRIMARY = -2,
    };

    if (d_cluster_p->isStopping()) {
        return rc_CLUSTER_IS_STOPPING;  // RETURN
    }

    if (!d_clusterState_p->isSelfActivePrimary()) {
        // Fast path -- self is not active primary for *any* partition.
        return rc_SELF_IS_NOT_PRIMARY;  // RETURN
    }

    bsls::Types::Int64 currentTimestampMs =
        bmqsys::Time::highResolutionTimer() / bdlt::TimeUnitRatio::k_NS_PER_MS;

    bdlma::LocalSequentialAllocator<512> vecAlloc(d_allocator_p);
    bsl::vector<QueueContextMapIter>     queuesToGc(&vecAlloc);

    for (QueueContextMapIter it = d_queues.begin(); it != d_queues.end();
         ++it) {
        QueueContextSp& queueContextSp = it->second;
        QueueLiveState& qinfo          = queueContextSp->d_liveQInfo;
        const int       pid            = queueContextSp->partitionId();

        if (!isQueueAssigned(*queueContextSp)) {
            continue;  // CONTINUE
        }

        if (!d_clusterState_p->isSelfActivePrimary(pid)) {
            continue;  // CONTINUE
        }

        if (queueContextSp->d_stateQInfo_sp->pendingUnassignment()) {
            continue;  // CONTINUE
        }

        if (0 == qinfo.d_queue_sp) {
            // Even though queue is assigned to an active primary node (self),
            // it is possible that queue instance does not exist.  This could
            // occur if this queue was recovered at startup and there have been
            // no clients for this queue.

            BSLS_ASSERT_SAFE(0 == qinfo.d_numHandleCreationsInProgress);
            BSLS_ASSERT_SAFE(0 == qinfo.d_numQueueHandles);
            BSLS_ASSERT_SAFE(queueContextSp->d_liveQInfo.d_pending.empty());
            BSLS_ASSERT_SAFE(0 == queueContextSp->d_liveQInfo.d_inFlight);

            if (immediate) {
                queuesToGc.push_back(it);
                continue;  // CONTINUE
            }

            if (0 == qinfo.d_queueExpirationTimestampMs) {
                // Queue's expiration time hasn't been set yet.  Check if queue
                // has any outstanding messages.  We need to query this through
                // the StorageMgr.

                if (d_storageManager_p->isStorageEmpty(it->first, pid)) {
                    // Queue has no outstanding messages.  It can be updated
                    // with an expiration timestamp.

                    qinfo.d_queueExpirationTimestampMs =
                        currentTimestampMs + d_clusterData_p->clusterConfig()
                                                 .queueOperations()
                                                 .keepaliveDurationMs();
                }

                continue;  // CONTINUE
            }

            if (currentTimestampMs < qinfo.d_queueExpirationTimestampMs) {
                continue;  // CONTINUE
            }

            // Queue can be gc'd.

            queuesToGc.push_back(it);
            continue;  // CONTINUE
        }

        // Queue instance exists.

        if (!qinfo.d_queue_sp->storage()->isEmpty()) {
            BSLS_ASSERT_SAFE(0 == qinfo.d_queueExpirationTimestampMs);
            continue;  // CONTINUE
        }

        // Queue has no outstanding messages.

        bool nothingOutstanding =
            queueContextSp->d_liveQInfo.d_pending.empty() &&
            0 == queueContextSp->d_liveQInfo.d_inFlight &&
            0 == qinfo.d_numHandleCreationsInProgress &&
            0 == qinfo.d_numQueueHandles;

        if (!nothingOutstanding) {
            // Something is outstanding on the queue, can't mark it for gc.

            BSLS_ASSERT_SAFE(0 == qinfo.d_queueExpirationTimestampMs);
            continue;  // CONTINUE
        }

        if (immediate) {
            // Nothing outstanding on the queue, and immediate gc has been
            // requested.

            queuesToGc.push_back(it);
            continue;  // CONTINUE
        }

        if (0 == qinfo.d_queueExpirationTimestampMs) {
            // Queue has nothing outstanding, and doesn't have a valid
            // expiration timestamp.  Update it to be gc'd at some point in
            // future.

            qinfo.d_queueExpirationTimestampMs = currentTimestampMs +
                                                 d_clusterData_p
                                                     ->clusterConfig()
                                                     .queueOperations()
                                                     .keepaliveDurationMs();
            continue;  // CONTINUE
        }

        // Queue has a valid expiration timestamp.

        BSLS_ASSERT_SAFE(0 == qinfo.d_numHandleCreationsInProgress);
        BSLS_ASSERT_SAFE(0 == qinfo.d_numQueueHandles);
        BSLS_ASSERT_SAFE(queueContextSp->d_liveQInfo.d_pending.empty());
        BSLS_ASSERT_SAFE(0 == queueContextSp->d_liveQInfo.d_inFlight);
        // We can assert on 'nothingOutstanding' above, but asserting on
        // individual fields will be useful for debugging if the assert
        // fires.

        if (currentTimestampMs < qinfo.d_queueExpirationTimestampMs) {
            continue;  // CONTINUE
        }

        // Queue can be gc'd.

        queuesToGc.push_back(it);
    }

    if (queuesToGc.empty()) {
        return rc_SUCCESS;  // RETURN
    }

    if (!d_clusterData_p->electorInfo().isSelfActiveLeader()) {
        // As part of implementing leader managed cluster state (and using
        // CSL), only leader node should be generating advisories (involves
        // generating sequence numbers).  In the current scheme of things,
        // primary and leader nodes can be different (even if cluster is
        // configured with 'leader-is-primary-for-all-partitions' flag).  If
        // this occurs, primary cannot broadcast a QueueUnassignedAdvisory
        // since only leader can do so.  So for now, queue gc logic is
        // suppressed if leader and primary nodes are different.  This logic
        // will be updated such that primary will send a QueueUnassignedRequest
        // to the leader, and then leader will broadcast
        // QueueUnassignedAdvisory.

        if (!d_primaryNotLeaderAlarmRaised) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                << d_cluster_p->description() << " Cannot gc "
                << queuesToGc.size() << " expired queues "
                << "since primary and leader nodes are different."
                << BMQTSK_ALARMLOG_END;

            d_primaryNotLeaderAlarmRaised = true;
        }

        return rc_SUCCESS;  // RETURN
    }

    for (size_t i = 0; i < queuesToGc.size(); ++i) {
        QueueContextMapIter&   qit            = queuesToGc[i];
        const QueueContextSp&  queueContextSp = qit->second;
        const int              pid            = queueContextSp->partitionId();
        const bmqt::Uri        uriCopy        = qit->first;
        const mqbu::StorageKey keyCopy        = queueContextSp->key();

        BSLS_ASSERT_SAFE(qit != d_queues.end());

        BALL_LOG_INFO << d_cluster_p->description()
                      << ": Garbage-collecting queue [" << uriCopy
                      << "], queueKey [" << keyCopy << "] assigned to "
                      << "Partition [" << pid << "] as it has expired.";

        mqbc::ClusterUtil::setPendingUnassignment(d_clusterState_p,
                                                  uriCopy,
                                                  true);

        if (!d_cluster_p->isCSLModeEnabled()) {
            // We already ensured there are no pending contexts for this queue,
            // so we can safely delete it from the various maps.
            removeQueueRaw(qit);

            d_clusterState_p->queueKeys().erase(keyCopy);

            d_clusterState_p->domainStates()
                .at(uriCopy.qualifiedDomain())
                ->adjustQueueCount(-1);

            d_clusterState_p->unassignQueue(uriCopy);
        }

        // Populate 'queueUnassignedAdvisory'
        bdlma::LocalSequentialAllocator<1024>  localAlloc(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage           controlMsg(&localAlloc);
        bmqp_ctrlmsg::QueueUnassignedAdvisory& queueAdvisory =
            controlMsg.choice()
                .makeClusterMessage()
                .choice()
                .makeQueueUnassignedAdvisory();

        mqbc::ClusterUtil::populateQueueUnassignedAdvisory(&queueAdvisory,
                                                           d_clusterData_p,
                                                           uriCopy,
                                                           keyCopy,
                                                           pid,
                                                           *d_clusterState_p);

        // Apply 'queueUnassignedAdvisory' to CSL
        d_clusterStateManager_p->unassignQueue(queueAdvisory);

        if (!d_cluster_p->isCSLModeEnabled()) {
            // Broadcast 'queueUnassignedAdvisory' to all followers
            d_clusterData_p->messageTransmitter().broadcastMessage(controlMsg);

            // Unregister the queue/storage from the partition, which will end
            // up issuing a QueueDeletion record.  Note that this method is
            // async.
            d_storageManager_p->unregisterQueue(uriCopy, pid);
        }
    }

    return rc_SUCCESS;  // RETURN
}

void ClusterQueueHelper::loadQueuesInfo(mqbcmd::StorageContent* out) const
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    bsl::vector<mqbcmd::StorageQueueInfo>& queuesInfo = out->storages();
    queuesInfo.reserve(d_queues.size());
    for (QueueContextMapConstIter it = d_queues.begin(); it != d_queues.end();
         ++it) {
        queuesInfo.resize(queuesInfo.size() + 1);
        mqbcmd::StorageQueueInfo& queueInfo = queuesInfo.back();
        bmqu::MemOutStream        os;
        os << it->second->key();
        queueInfo.queueKey()        = os.str();
        queueInfo.partitionId()     = it->second->partitionId();
        queueInfo.internalQueueId() = it->second->d_liveQInfo.d_id;
        queueInfo.queueUri()        = it->second->uri().asString();
    }
}

void ClusterQueueHelper::loadState(
    mqbcmd::ClusterQueueHelper* clusterQueueHelper) const
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    clusterQueueHelper->clusterName()  = d_cluster_p->name();
    clusterQueueHelper->locality()     = (d_cluster_p->isRemote()
                                              ? mqbcmd::Locality::REMOTE
                                              : mqbcmd::Locality::LOCAL);
    clusterQueueHelper->numQueues()    = d_queues.size();
    clusterQueueHelper->numQueueKeys() = d_clusterState_p->queueKeys().size();
    clusterQueueHelper->numPendingReopenQueueRequests() =
        d_numPendingReopenQueueRequests;
    // Domains
    clusterQueueHelper->domains().resize(
        d_clusterState_p->domainStates().size());
    int dmnIdx = 0;
    for (mqbc::ClusterState::DomainStatesCIter cit =
             d_clusterState_p->domainStates().cbegin();
         cit != d_clusterState_p->domainStates().cend();
         ++cit, ++dmnIdx) {
        mqbcmd::ClusterDomain& clusterDomain =
            clusterQueueHelper->domains()[dmnIdx];
        clusterDomain.name()              = cit->first;
        clusterDomain.numAssignedQueues() = cit->second->numAssignedQueues();
        clusterDomain.loaded()            = cit->second->domain() != 0;
    }

    // Queues
    clusterQueueHelper->queues().resize(d_queues.size());
    int qIdx = 0;
    for (QueueContextMapConstIter it = d_queues.begin(); it != d_queues.end();
         ++it, ++qIdx) {
        const QueueLiveState&                info = it->second->d_liveQInfo;
        const bsl::vector<OpenQueueContext>& contexts =
            it->second->d_liveQInfo.d_pending;
        const int pid = it->second->partitionId();

        // Queue URI
        mqbcmd::ClusterQueue& clusterQueue =
            clusterQueueHelper->queues()[qIdx];
        clusterQueue.uri() = it->first.asString();
        clusterQueue.id()  = info.d_id;

        // Info
        clusterQueue.numInFlightContexts() =
            it->second->d_liveQInfo.d_inFlight;
        clusterQueue.isAssigned()         = isQueueAssigned(*(it->second));
        clusterQueue.isPrimaryAvailable() = isQueuePrimaryAvailable(
            *(it->second));

        clusterQueue.subIds().resize(info.d_subQueueIds.size());
        int sIdx = 0;
        for (StreamsMap::const_iterator citer = info.d_subQueueIds.begin();
             citer != info.d_subQueueIds.end();
             ++citer) {
            clusterQueue.subIds()[sIdx].subId() = citer->subId();
            clusterQueue.subIds()[sIdx].appId() = citer->appId();
            ++sIdx;
        }

        clusterQueue.partitionId() = pid;
        bmqu::MemOutStream os;
        if (pid != mqbs::DataStore::k_INVALID_PARTITION_ID) {
            mqbnet::ClusterNode* primary =
                d_clusterState_p->partition(pid).primaryNode();
            if (primary) {
                os << " (primary: " << primary->nodeDescription() << ")";
            }
            else {
                os << " (*NO* primary)";
            }
            clusterQueue.primaryNodeDescription().makeValue(os.str());
            os.reset();
        }

        os << it->second->key();
        clusterQueue.key() = os.str();
        os.reset();
        clusterQueue.isCreated() = info.d_queue_sp;

        // Contexts
        clusterQueue.contexts().resize(contexts.size());
        for (size_t ctxId = 0; ctxId != contexts.size(); ++ctxId) {
            const OpenQueueContext& context = contexts[ctxId];
            os << context.d_handleParameters;
            clusterQueue.contexts()[ctxId].queueHandleParametersJson() =
                os.str();
            os.reset();
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
