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

#ifndef INCLUDED_MQBBLP_CLUSTERQUEUEREOPENER
#define INCLUDED_MQBBLP_CLUSTERQUEUEREOPENER

/// @file mqbblp_clusterqueuereopener.h
///
/// @brief Provide a mechanism to reopen queues after cluster failover.
///
/// This component manages the re-establishment of upstream queue connections
/// after a failover event (primary switch, leader election, or node status
/// change).  It encapsulates the reopen state machine: sending ReopenQueue
/// requests, handling responses and retries, tracking per-partition reopen
/// cycles, and notifying queues of success or failure.
///
/// Thread Safety                         {#mqbblp_clusterqueuereopener_thread}
/// =============
///
/// @note This entire component's code is *serialized* and only executes inside
///       the *dispatcher* thread.

// MQB
#include <mqbblp_clusterqueuehelper.h>
#include <mqbc_clusterdata.h>
#include <mqbc_clusterstate.h>
#include <mqbi_cluster.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_resultcode.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}

namespace mqbblp {

// FORWARD DECLARATION
class ClusterQueueHelper;

// =========================
// class ClusterQueueReopener
// =========================

/// Mechanism to reopen queues on a cluster after failover.
class ClusterQueueReopener {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.CLUSTERQUEUEREOPENER");

  public:
    // TYPES

    /// Tracks the state of a single partition's reopen cycle.  When all
    /// shared_ptr references to a cycle are released (i.e., all in-flight
    /// reopen requests have completed), the destructor calls
    /// `completePartitionReopen` on the owning `ClusterQueueReopener`.
    struct PartitionReopenCycle {
        ClusterQueueReopener* d_owner_p;
        bsls::Types::Uint64   d_generationCount;
        int                   d_partitionId;
        bool                  d_isSuccess;

        explicit PartitionReopenCycle(ClusterQueueReopener* owner,
                                      bsls::Types::Uint64   generationCount,
                                      int                   partitionId);
        ~PartitionReopenCycle();

        void                setAsFailed();
        bsls::Types::Uint64 generationCount() const;
        int                 partitionId() const;
        bool                isSuccess() const;
    };

  private:
    // PRIVATE TYPES
    typedef bsl::unordered_map<int, bsl::weak_ptr<PartitionReopenCycle> >
        ReopenCycles;

    typedef bmqp::RequestManager<bmqp_ctrlmsg::ControlMessage,
                                 bmqp_ctrlmsg::ControlMessage>
        RequestManagerType;

    typedef ClusterQueueHelper::QueueContext        QueueContext;
    typedef ClusterQueueHelper::QueueContextSp      QueueContextSp;
    typedef ClusterQueueHelper::QueueContextMap     QueueContextMap;
    typedef ClusterQueueHelper::QueueContextMapIter QueueContextMapIter;
    typedef ClusterQueueHelper::QueueContextMapConstIter
                                                QueueContextMapConstIter;
    typedef ClusterQueueHelper::QueueLiveState  QueueLiveState;
    typedef ClusterQueueHelper::SubQueueContext SubQueueContext;
    typedef ClusterQueueHelper::StreamsMap      StreamsMap;

  private:
    // DATA

    /// Allocator to use
    bslma::Allocator* d_allocator_p;

    /// Pointer to the owning ClusterQueueHelper
    ClusterQueueHelper* d_queueHelper_p;

    /// Cluster non-persistent data
    mqbc::ClusterData* d_clusterData_p;

    /// Cluster persistent state
    mqbc::ClusterState* d_clusterState_p;

    /// Cluster interface
    mqbi::Cluster* d_cluster_p;

    /// Track the state of partitions upon `restoreState`.
    /// Non-empty if there are reopen requests in progress or some reopen
    /// for some partition has failed.
    ReopenCycles d_reopenCycles;

  private:
    // PRIVATE MANIPULATORS

    void restoreStateRemote();

    void restoreStateCluster(int partitionId);

    bmqt::GenericResult::Enum
    restoreStateHelper(QueueContext*        queueContext,
                       mqbnet::ClusterNode* activeNode,
                       const bsl::shared_ptr<PartitionReopenCycle>& cycle);

    bmqt::GenericResult::Enum
    sendReopenQueueRequest(QueueContext*        queueContext,
                           SubQueueContext*     subQueueContext,
                           mqbnet::ClusterNode* activeNode,
                           const bsl::shared_ptr<PartitionReopenCycle>& cycle,
                           int numAttempts);

    void
    onReopenQueueResponse(const RequestManagerType::RequestSp& requestContext,
                          mqbnet::ClusterNode*                 activeNode,
                          const bsl::shared_ptr<PartitionReopenCycle>& cycle,
                          int numAttempts);

    void
    onReopenQueueRetry(const RequestManagerType::RequestSp& requestContext,
                       mqbnet::ClusterNode*                 activeNode,
                       const bsl::shared_ptr<PartitionReopenCycle>& cycle,
                       int numAttempts);

    void onReopenQueueRetryDispatched(
        const RequestManagerType::RequestSp&         requestContext,
        mqbnet::ClusterNode*                         activeNode,
        const bsl::shared_ptr<PartitionReopenCycle>& cycle,
        int                                          numAttempts);

    void finishReopening(QueueContext*        queueContext,
                         StreamsMap::iterator sqit,
                         mqbnet::ClusterNode* activeNode,
                         const bsl::shared_ptr<PartitionReopenCycle>& cycle);

    void
    reconfigureCallback(const bmqp_ctrlmsg::Status&           status,
                        const bmqp_ctrlmsg::StreamParameters& streamParameters,
                        const bsl::shared_ptr<PartitionReopenCycle>& cycle);

    bsl::shared_ptr<PartitionReopenCycle>
    startPartitionReopen(int partitionId, bsls::Types::Uint64 generationCount);

    void completePartitionReopen(PartitionReopenCycle* cycle);

  private:
    // NOT IMPLEMENTED
    ClusterQueueReopener(const ClusterQueueReopener&);
    ClusterQueueReopener& operator=(const ClusterQueueReopener&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterQueueReopener,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object using the specified `allocator`.
    explicit ClusterQueueReopener(bslma::Allocator* allocator);

    /// Destructor
    ~ClusterQueueReopener();

    // MANIPULATORS

    /// Initialize this object with the specified `queueHelper`,
    /// `clusterData`, `clusterState`, and `cluster`.
    void initialize(ClusterQueueHelper* queueHelper,
                    mqbc::ClusterData*  clusterData,
                    mqbc::ClusterState* clusterState,
                    mqbi::Cluster*      cluster);

    /// Paired operation of `initialize()`, undo any action that was
    /// performed during `initialize` and restore this object to a default
    /// constructed state.
    void teardown();

    /// Restore state for the specified `partitionId` by re-issuing
    /// open-queue requests for all applicable queues.  If `partitionId` is
    /// `mqbi::Storage::k_ANY_PARTITION_ID`, an attempt is made to restore
    /// state for all partitions.
    ///
    /// THREAD: This method must be called from the Cluster's dispatcher
    ///         thread.
    void restoreState(int partitionId);

    /// Notify the queue in the specified `queueContext` about the
    /// success or failure of reopening the substream identified by the
    /// specified `upstreamSubQueueId` with the specified `generationCount`.
    /// The specified `isOpen` indicates whether the reopen succeeded.
    /// The optionally specified `isWriterOnly` indicates whether only the
    /// writer side is being notified.
    void notifyQueue(QueueContext*       queueContext,
                     unsigned int        upstreamSubQueueId,
                     bsls::Types::Uint64 generationCount,
                     bool                isOpen,
                     bool                isWriterOnly = false);

    // ACCESSORS

    /// Return `true` if this object is in the process of restoring its
    /// state; that is reopening queues which were previously opened before a
    /// failover.
    bool isFailoverInProgress() const;

    /// Return the number of currently pending reopen-queue requests.
    int numPendingReopenQueueRequests() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------------------------------
// class ClusterQueueReopener::PartitionReopenCycle
// -----------------------------------------------

inline ClusterQueueReopener::PartitionReopenCycle::PartitionReopenCycle(
    ClusterQueueReopener* owner,
    bsls::Types::Uint64   generationCount,
    int                   partitionId)
: d_owner_p(owner)
, d_generationCount(generationCount)
, d_partitionId(partitionId)
, d_isSuccess(true)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_owner_p);
}

inline ClusterQueueReopener::PartitionReopenCycle::~PartitionReopenCycle()
{
    d_owner_p->completePartitionReopen(this);
}

inline void ClusterQueueReopener::PartitionReopenCycle::setAsFailed()
{
    d_isSuccess = false;
}

inline bsls::Types::Uint64
ClusterQueueReopener::PartitionReopenCycle::generationCount() const
{
    return d_generationCount;
}

inline int ClusterQueueReopener::PartitionReopenCycle::partitionId() const
{
    return d_partitionId;
}

inline bool ClusterQueueReopener::PartitionReopenCycle::isSuccess() const
{
    return d_isSuccess;
}

// --------------------------
// class ClusterQueueReopener
// --------------------------

inline bool ClusterQueueReopener::isFailoverInProgress() const
{
    return !d_reopenCycles.empty();
}

inline bsl::shared_ptr<ClusterQueueReopener::PartitionReopenCycle>
ClusterQueueReopener::startPartitionReopen(int                 partitionId,
                                           bsls::Types::Uint64 generationCount)
{
    bsl::shared_ptr<PartitionReopenCycle> cycle =
        d_reopenCycles[partitionId].lock();

    if (cycle) {
        if (generationCount == cycle->generationCount()) {
            return cycle;  // RETURN
        }
        cycle->setAsFailed();
    }

    cycle = bsl::allocate_shared<PartitionReopenCycle>(d_allocator_p,
                                                       this,
                                                       generationCount,
                                                       partitionId);
    d_reopenCycles[partitionId] = cycle;

    return cycle;
}

inline void
ClusterQueueReopener::completePartitionReopen(PartitionReopenCycle* cycle)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(cycle);

    int pid = cycle->partitionId();

    if (cycle->isSuccess()) {
        d_reopenCycles.erase(pid);
        if (d_reopenCycles.empty()) {
            BALL_LOG_INFO << d_cluster_p->description() << ": state restored";
        }
    }
    else {
        BALL_LOG_INFO << d_cluster_p->description() << ": partition [" << pid
                      << "] has failed to reopen";
    }
}

}  // close package namespace
}  // close enterprise namespace

#endif
