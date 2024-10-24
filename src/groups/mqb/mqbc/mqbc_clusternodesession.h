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

// mqbc_clusternodesession.h                                          -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERNODESESSION
#define INCLUDED_MQBC_CLUSTERNODESESSION

//@PURPOSE:
//
//@CLASSES:
//
//@DESCRIPTION:
//

// MQB

#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbnet_cluster.h>
#include <mqbstat_clusterstats.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>

#include <bmqst_statcontextuserdata.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbc {

// FORWARD DECLARATION
class ClusterState;

// ========================
// class ClusterNodeSession
// ========================

/// Provide a session for interaction with BlazingMQ cluster node.
class ClusterNodeSession : public mqbi::DispatcherClient,
                           public mqbi::QueueHandleRequester {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.CLUSTERNODESESSION");

  public:
    // TYPES

    /// Struct holding information associated to a subStream of a queue
    /// opened in this session.
    struct SubQueueInfo {
        bsl::shared_ptr<mqbstat::ClusterNodeStats> d_clientStats;
        // Stats of this SubQueue, with regards
        // to the client.

        // CREATORS

        /// Constructor of a new object, initializes all data members to
        /// default values.
        SubQueueInfo();
    };

    /// Struct holding the state associated to a queue opened in by the
    /// cluster node.  TBD: this type also exists in `mqba::ClientSession`.
    /// It should be moved to a new component.
    struct QueueState {
        typedef bmqp::ProtocolUtil::QueueInfo<SubQueueInfo> StreamsMap;

        // PUBLIC DATA
        mqbi::QueueHandle* d_handle_p;  // QueueHandle of the queue

        bool d_isFinalCloseQueueReceived;
        // Flag to indicate if the 'final'
        // closeQueue request for this handle
        // has been received. This flag can be
        // used to reject PUT & CONFIRM
        // messages which some clients try to
        // post after closing a queue (under
        // certain conditions, such incorrect
        // usage cannot be caught in the SDK
        // eg, if messages are being posted
        // from one app thread, while
        // closeQueue request is being sent
        // from another app thread).  This flag
        // can also be used by the queue or
        // queue engine for sanity checking.

        StreamsMap d_subQueueInfosMap;
        // Map of subQueueId to information
        // associated to a substream of a queue
        // opened in this session

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(QueueState, bslma::UsesBslmaAllocator)

        // CREATORS

        /// Constructor of a new object, initializes all data members to
        /// default values and uses the specified `allocator` for any memory
        /// allocation.
        QueueState(bslma::Allocator* basicAllocator = 0);

        /// Constructor of a new object from the specified `original` and
        /// uses the specified `allocator` for any memory allocation.
        QueueState(const QueueState& original,
                   bslma::Allocator* basicAllocator = 0);
    };

    /// Map of queueId to QueueState
    typedef bsl::unordered_map<int, QueueState> QueueHandleMap;

    typedef QueueHandleMap::iterator QueueHandleMapIter;

    typedef QueueHandleMap::const_iterator QueueHandleMapConstIter;

    typedef bsl::shared_ptr<bmqst::StatContext> StatContextSp;

    typedef QueueState::StreamsMap StreamsMap;

  private:
    // DATA
    mqbi::DispatcherClient* d_cluster_p;
    // The corresponding cluster (held as
    // dispatcher client)

    mqbnet::ClusterNode* d_clusterNode_p;
    // The corresponding cluster node

    int d_peerInstanceId;
    // ID of the peer's instance.  This ID
    // is changed everytime the channel
    // with the peer is reset.  This
    // instance ID is used to discriminate
    // against old instance of the peer.
    // Note that unlike
    // 'mqba::ClientSession', an instance
    // of 'ClusterNodeSession' is not
    // destroyed every time channel b/w
    // self node and peer goes down, and
    // thus self node may contain state
    // associated with peer's old instance.
    // Also note that there is no invalid
    // value for this ID, and its value
    // alone cannot be used to determine if
    // the channel with the peer is up or
    // not.

    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>
        d_queueHandleRequesterContext_sp;
    // Context used to uniquely identify
    // this client when requesting a queue
    // handle.

    bmqp_ctrlmsg::NodeStatus::Value d_nodeStatus;
    // Node status

    StatContextSp d_statContext_sp;

    bsl::vector<int> d_primaryPartitions;
    // PartitionIds for which this node is
    // the primary

    QueueHandleMap d_queueHandles;
    // List of queue handles opened on this
    // node by 'd_clusterNode_p'

  private:
    // NOT IMPLEMENTED
    ClusterNodeSession(const ClusterNodeSession&);             // = delete;
    ClusterNodeSession& operator=(const ClusterNodeSession&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterNodeSession,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit ClusterNodeSession(mqbi::DispatcherClient* cluster,
                                mqbnet::ClusterNode*    netNode,
                                const bsl::string&      clusterName,
                                const bmqp_ctrlmsg::ClientIdentity& identity,
                                bslma::Allocator*                   allocator);

    /// Destructor.
    ~ClusterNodeSession() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Teardown this object, i.e., release all resources it has acquired,
    /// such as queue handles.  This method will block until all resources
    /// have been released.
    void teardown();

    void onDispatcherEvent(const mqbi::DispatcherEvent& event)
        BSLS_KEYWORD_OVERRIDE;
    // Called by the 'Dispatcher' when it has the specified 'event' to
    // deliver to the client.

    /// Called by the dispatcher to flush any pending operation. Mainly used
    /// to provide batch and nagling mechanism.  Note that this method will
    /// never be invoked for this type.
    void flush() BSLS_KEYWORD_OVERRIDE;

    /// Return a pointer to the dispatcher this client is associated with.
    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the dispatcherClientData.
    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE;

    void setNodeStatus(bmqp_ctrlmsg::NodeStatus::Value value);

    void setPeerInstanceId(int value);

    /// Return the associated stat context.
    StatContextSp& statContext();

    /// Return a reference to the modifiable list of queue handles.
    QueueHandleMap& queueHandles();

    /// Add the specified `partitionId` to the list of partitions for which
    /// this node is the primary.  Behavior is undefined if this partition
    /// is already in the list.
    void addPartitionRaw(int partitionId);

    /// Add the specified `partitionId` to the list of partitions for which
    /// this node is the primary, if its not in the list, and return true.
    /// Return false if `partitionId` is already in the list.
    bool addPartitionSafe(int partitionId);

    /// Remove the specified `partitionId` from the list of partitions for
    /// which this node is the primary.  Behavior is undefined if this
    /// partition is not in the list.
    void removePartitionRaw(int partitionId);

    /// Remove the specified `partitionId` from the list of partitions for
    /// which this node is the primary, if its in the list, and return true.
    /// Return false if `partitionId` is not in the list.
    bool removePartitionSafe(int partitionId);

    /// Remove all partitions from the list of partitions for which this
    /// node is the primary.
    void removeAllPartitions();

    // ACCESSORS

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;
    // Return a reference to the dispatcherClientData.

    /// Return a printable description of the client (e.g. for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;

    /// Return the associated cluster as the dispatcher client.
    mqbi::DispatcherClient* cluster() const;

    /// Return the associated cluster node.
    mqbnet::ClusterNode* clusterNode() const;

    /// Return the node status.
    bmqp_ctrlmsg::NodeStatus::Value nodeStatus() const;

    /// Return the instance ID of the peer.
    int peerInstanceId() const;

    /// Return the associated stat context.
    const StatContextSp& statContext() const;

    /// Return a reference to the non-modifiable list of partitions for
    /// which this cluster node is the primary.
    const bsl::vector<int>& primaryPartitions() const;

    /// Return a reference to the non-modifiable list of queue handles.
    const QueueHandleMap& queueHandles() const;

    /// Return true if this node is primary for the specified `partitionId`,
    /// false otherwise.
    bool isPrimaryForPartition(int partitionId) const;

    // ACCESSORS
    //   (virtual: mqbi::QueueHandleRequester)

    /// Return a non-modifiable reference to the context of this requester
    /// of a QueueHandle.
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
    handleRequesterContext() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// ---------------------------------------
// struct ClientSessionState::SubQueueInfo
// ---------------------------------------

inline ClusterNodeSession::SubQueueInfo::SubQueueInfo()
: d_clientStats()
{
    d_clientStats.createInplace();
}

// ------------------------------------
// struct ClusterNodeSession:QueueState
// ------------------------------------

// CREATORS
inline ClusterNodeSession::QueueState::QueueState(
    bslma::Allocator* basicAllocator)
: d_handle_p(0)
, d_isFinalCloseQueueReceived(false)
, d_subQueueInfosMap(basicAllocator)
{
    // NOTHING
}

inline ClusterNodeSession::QueueState::QueueState(
    const QueueState& original,
    bslma::Allocator* basicAllocator)
: d_handle_p(original.d_handle_p)
, d_isFinalCloseQueueReceived(false)
, d_subQueueInfosMap(original.d_subQueueInfosMap, basicAllocator)
{
    // NOTHING
}

// ------------------------
// class ClusterNodeSession
// ------------------------

// MANIPULATORS
inline mqbi::DispatcherClientData& ClusterNodeSession::dispatcherClientData()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_clusterNode_p);

    return d_cluster_p->dispatcherClientData();
}

inline void
ClusterNodeSession::setNodeStatus(bmqp_ctrlmsg::NodeStatus::Value value)
{
    d_nodeStatus = value;
}

inline void ClusterNodeSession::setPeerInstanceId(int value)
{
    d_peerInstanceId = value;
}

inline void
ClusterNodeSession::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_clusterNode_p);

    // The 'event' originates from the queue which the 'd_clusterNode_p' (or
    // 'this') has opened on this node.  The 'event' should be forwarded to
    // 'd_cluster_p' after 'event' has been populated with 'this'.  Note that
    // the only events expected here are PUSH and ACK, for now.

    mqbi::DispatcherEvent& ev = const_cast<mqbi::DispatcherEvent&>(event);
    if (mqbi::DispatcherEventType::e_PUSH == ev.type()) {
        ev.getAs<mqbi::DispatcherPushEvent>().setClusterNode(d_clusterNode_p);
    }
    else if (mqbi::DispatcherEventType::e_ACK == ev.type()) {
        ev.getAs<mqbi::DispatcherAckEvent>().setClusterNode(d_clusterNode_p);
    }
    else {
        BSLS_ASSERT_OPT(false && "Unknown event type");
    }
    d_cluster_p->onDispatcherEvent(event);
}

inline ClusterNodeSession::StatContextSp& ClusterNodeSession::statContext()
{
    return d_statContext_sp;
}

inline ClusterNodeSession::QueueHandleMap& ClusterNodeSession::queueHandles()
{
    return d_queueHandles;
}

// ACCESSORS
inline mqbi::Dispatcher* ClusterNodeSession::dispatcher()
{
    return d_cluster_p->dispatcher();
}

inline const mqbi::Dispatcher* ClusterNodeSession::dispatcher() const
{
    return d_cluster_p->dispatcher();
}

inline const mqbi::DispatcherClientData&
ClusterNodeSession::dispatcherClientData() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_clusterNode_p);

    return d_cluster_p->dispatcherClientData();
}

inline const bsl::string& ClusterNodeSession::description() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_clusterNode_p);

    return d_clusterNode_p->hostName();
}

inline mqbi::DispatcherClient* ClusterNodeSession::cluster() const
{
    return d_cluster_p;
}

inline mqbnet::ClusterNode* ClusterNodeSession::clusterNode() const
{
    return d_clusterNode_p;
}

inline bmqp_ctrlmsg::NodeStatus::Value ClusterNodeSession::nodeStatus() const
{
    return d_nodeStatus;
}

inline int ClusterNodeSession::peerInstanceId() const
{
    return d_peerInstanceId;
}

inline const ClusterNodeSession::StatContextSp&
ClusterNodeSession::statContext() const
{
    return d_statContext_sp;
}

inline const bsl::vector<int>& ClusterNodeSession::primaryPartitions() const
{
    return d_primaryPartitions;
}

inline const ClusterNodeSession::QueueHandleMap&
ClusterNodeSession::queueHandles() const
{
    return d_queueHandles;
}

inline const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
ClusterNodeSession::handleRequesterContext() const
{
    return d_queueHandleRequesterContext_sp;
}

}  // close package namespace
}  // close enterprise namespace

#endif
