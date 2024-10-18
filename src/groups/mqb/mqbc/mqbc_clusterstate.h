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

// mqbc_clusterstate.h                                                -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERSTATE
#define INCLUDED_MQBC_CLUSTERSTATE

//@PURPOSE: Provide a VST representing the persistent state of a cluster.
//
//@CLASSES:
//  mqbc::ClusterStatePartitionInfo: VST for partition information
//  mqbc::ClusterStateQueueInfo    : VST for queue information
//  mqbc::ClusterStateObserver     : Interface for a ClusterState observer
//  mqbc::ClusterState             : VST for persistent state of a cluster
//
//@DESCRIPTION: 'mqbc::ClusterState' is a value-semantic type representing the
// persistent state of a cluster.  Important state changes can be notified to
// observers, implementing the 'mqbblp::ClusterStateObserver' interface.

// MQB
#include <mqbi_cluster.h>
#include <mqbi_clusterstatemanager.h>
#include <mqbnet_cluster.h>
#include <mqbs_datastore.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bdlpcre_regex.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbi {
class Domain;
}

namespace mqbc {

// ===============================
// class ClusterStatePartitionInfo
// ===============================

/// This class provides a VST representing the persistent state associated
/// to a partition of a cluster.
///
/// TBD: If needed, we can add a ctor in this class which takes
/// `bmqp_ctrlmsg::PartitionPrimaryInfo`.  Doing vice versa will not be
/// possible because we don't want to edit generated file.  Perhaps we can
/// place the converter routine in `ClusterUtil`.
class ClusterStatePartitionInfo {
  private:
    // DATA
    int d_partitionId;
    // Partition id
    //
    // TBD: Can be removed if we index
    // ClusterState::PartitionsInfo by partitionId

    unsigned int d_primaryLeaseId;
    // LeaseId (generation count) of the primary; zero is
    // invalid/null

    int d_primaryNodeId;
    // Node id of the primary

    int d_numQueuesMapped;
    // Number of queues currently mapped to the partition

    int d_numActiveQueues;
    // Number of active queues on the partition: a queue
    // may be mapped, but not active (if there are no
    // clients of that queue connected to the broker).
    // Therefore, the number of active queues should
    // always be less than or equal to the number of
    // queues mapped.  While the number of queues mapped
    // to a partition is global and the same on all nodes
    // of the cluster, the number of active queues is
    // independent for each node.  Note that this
    // attributed is manipulated from the cluster
    // dispatcher thread only

    mqbnet::ClusterNode* d_primaryNode_p;
    // Pointer to primary node for the partition; null if
    // no primary associated

    bmqp_ctrlmsg::PrimaryStatus::Value d_primaryStatus;
    // Status of the primary

  public:
    // CREATORS

    /// Create a new `mqbc::ClusterStatePartitionInfo`.
    ClusterStatePartitionInfo();

    // MANIPULATORS
    ClusterStatePartitionInfo& setPartitionId(int value);
    ClusterStatePartitionInfo& setPrimaryLeaseId(unsigned int value);
    ClusterStatePartitionInfo& setPrimaryNodeId(int value);
    ClusterStatePartitionInfo& setNumQueuesMapped(int value);
    ClusterStatePartitionInfo& setNumActiveQueues(int value);
    ClusterStatePartitionInfo& setPrimaryNode(mqbnet::ClusterNode* value);

    /// Set the corresponding member to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ClusterStatePartitionInfo&
    setPrimaryStatus(bmqp_ctrlmsg::PrimaryStatus::Value value);

    // ACCESSORS
    int                  partitionId() const;
    unsigned int         primaryLeaseId() const;
    int                  primaryNodeId() const;
    int                  numQueuesMapped() const;
    int                  numActiveQueues() const;
    mqbnet::ClusterNode* primaryNode() const;

    /// Return the value of the corresponding member of this object.
    bmqp_ctrlmsg::PrimaryStatus::Value primaryStatus() const;
};

// ===========================
// class ClusterStateQueueInfo
// ===========================

/// This class provides a VST representing the state associated to a queue
/// in a cluster.
///
/// TBD: If needed, we can add a ctor in this class which takes
/// `bmqp_ctrlmsg::QueueInfo`.  Doing vice versa will not be possible
/// because we don't want to edit generated file.  Perhaps we can place the
/// converter routine in `ClusterUtil`.
///
/// TBD: When should AppIds and AppKeys come from?  Should the leader/primary
/// generate them, or should we hardcode them in the domain config?
class ClusterStateQueueInfo {
  public:
    // TYPES
    typedef mqbi::ClusterStateManager::AppInfo       AppInfo;
    typedef mqbi::ClusterStateManager::AppInfos      AppInfos;
    typedef mqbi::ClusterStateManager::AppInfosCIter AppInfosCIter;

  private:
    // DATA
    bmqt::Uri d_uri;
    // Canonical URI of the queue

    mqbu::StorageKey d_key;
    // Assigned queue key, only if cluster member (null if
    // unassigned)

    int d_partitionId;
    // Assigned partitionId
    // (mqbs::DataStore::k_INVALID_PARTITION_ID if unassigned)

    AppInfos d_appInfos;
    // List of App id and key pairs
    //
    // TBD: Should also be added to mqbconfm::Domain

    bool d_pendingUnassignment;
    // Flag indicating whether this queue is in the process of
    // being unassigned.

  private:
    // NOT IMPLEMENTED
    ClusterStateQueueInfo(const ClusterStateQueueInfo&) BSLS_KEYWORD_DELETED;
    ClusterStateQueueInfo&
    operator=(const ClusterStateQueueInfo&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterStateQueueInfo,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `mqbc::ClusterStateQueueInfo` with the specified `uri`,
    /// using the specified `allocator` for any memory allocation.
    ClusterStateQueueInfo(const bmqt::Uri& uri, bslma::Allocator* allocator);

    /// Create a `mqbc::ClusterStateQueueInfo` with the specified `uri`,
    /// `key`, `partitionId` and `appIdInfos` values.  Use the specified
    /// `allocator` for any memory allocation.
    ClusterStateQueueInfo(const bmqt::Uri&        uri,
                          const mqbu::StorageKey& key,
                          int                     partitionId,
                          const AppInfos&         appIdInfos,
                          bslma::Allocator*       allocator);

    // MANIPULATORS
    ClusterStateQueueInfo& setKey(const mqbu::StorageKey& value);
    ClusterStateQueueInfo& setPartitionId(int value);

    /// Set the corresponding member to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ClusterStateQueueInfo& setPendingUnassignment(bool value);

    /// Get a modifiable reference to this object's appIdInfos.
    AppInfos& appInfos();

    /// Reset the `key`, `partitionId`, `appIdInfos` members of this object.
    /// Note that `uri` is left untouched because it is an invariant member
    /// of a given instance of such a ClusterStateQueueInfo object.
    void reset();

    // ACCESSORS
    const bmqt::Uri&        uri() const;
    const mqbu::StorageKey& key() const;
    int                     partitionId() const;
    const AppInfos&         appInfos() const;

    /// Return the value of the corresponding member of this object.
    bool pendingUnassignment() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.  Behavior is
    /// undefined unless `isValid()` returns true.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                stream,
                         const ClusterStateQueueInfo& rhs);

// ==========================
// class ClusterStateObserver
// ==========================

/// This interface exposes notifications of events happening on the cluster.
///
/// NOTE: This is purposely not a pure interface, each method has a default
///       void implementation, so that clients only need to implement the
///       ones they care about.
class ClusterStateObserver {
  public:
    // TYPES
    typedef ClusterStateQueueInfo::AppInfos AppInfos;

  public:
    // CREATORS

    /// Destructor
    virtual ~ClusterStateObserver();

    /// Callback invoked when the specified `partitionId` gets assigned to
    /// the specified `primary` with the specified `leaseId` and `status`,
    /// replacing the specified `oldPrimary` with the specified
    /// `oldLeaseId`.  Note that null is a valid value for the `primary`,
    /// and it implies that there is no primary for that partition.  Also
    /// note that this method will be invoked when the `primary` or the
    /// `status` or both change.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    onPartitionPrimaryAssignment(int                  partitionId,
                                 mqbnet::ClusterNode* primary,
                                 unsigned int         leaseId,
                                 bmqp_ctrlmsg::PrimaryStatus::Value status,
                                 mqbnet::ClusterNode*               oldPrimary,
                                 unsigned int oldLeaseId);

    /// Callback invoked when a queue with the specified `info` gets
    /// assigned to the cluster.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onQueueAssigned(const ClusterStateQueueInfo& info);

    /// Callback invoked when a queue with the specified `info` gets
    /// unassigned from the cluster.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onQueueUnassigned(const ClusterStateQueueInfo& info);

    /// Callback invoked when a queue with the specified `uri` belonging to
    /// the specified `domain` is updated with the optionally specified
    /// `addedAppIds` and `removedAppIds`.  If the specified `uri` is empty,
    /// the appId updates are applied to the entire `domain` instead.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onQueueUpdated(const bmqt::Uri&   uri,
                                const bsl::string& domain,
                                const AppInfos&    addedAppIds,
                                const AppInfos&    removedAppIds = AppInfos());

    /// Callback invoked when a partition with the specified `partitionId`
    /// has been orphan above a certain threshold amount of time.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onPartitionOrphanThreshold(size_t partitiondId);

    /// Callback invoked when the specified `node` has been unavailable
    /// above a certain threshold amount of time.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onNodeUnavailableThreshold(mqbnet::ClusterNode* node);

    /// Callback invoked when the leader node has been perceived as passive
    /// above a certain threshold amount of time.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onLeaderPassiveThreshold();

    /// Callback invoked when failover has not completed above a certain
    /// threshold amount of time.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onFailoverThreshold();
};

// ==================
// class ClusterState
// ==================

/// This class provides a VST representing the persistent state of a
/// cluster.
class ClusterState {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.CLUSTERSTATE");

  public:
    // TYPES
    typedef ClusterStateQueueInfo::AppInfos      AppInfos;
    typedef ClusterStateQueueInfo::AppInfosCIter AppInfosCIter;

    typedef bsl::vector<ClusterStatePartitionInfo> PartitionsInfo;

    typedef bsl::shared_ptr<ClusterStateQueueInfo> QueueInfoSp;

    /// <canonicalURI> -> <queueInformation>
    typedef bsl::unordered_map<bmqt::Uri, QueueInfoSp> UriToQueueInfoMap;
    typedef UriToQueueInfoMap::iterator                UriToQueueInfoMapIter;
    typedef UriToQueueInfoMap::const_iterator          UriToQueueInfoMapCIter;

    struct DomainState {
      private:
        // DATA
        int               d_numAssignedQueues;
        mqbi::Domain*     d_domain_p;
        UriToQueueInfoMap d_queuesInfo;

      public:
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(DomainState, bslma::UsesBslmaAllocator)

        // CREATORS

        /// Create a new DomainState using the specified `allocator` for
        /// memory allocations.
        DomainState(bslma::Allocator* allocator);

        // MANIPULATORS

        /// Get a modifiable reference to this object's domain.
        mqbi::Domain* domain();

        /// Get a modifiable reference to this object's queues info.
        UriToQueueInfoMap& queuesInfo();

        /// Set the domain of this object to the specified `domain`.
        void setDomain(mqbi::Domain* domain);

        /// Adjust number of assigned queues, and update domain stat
        /// context.
        void adjustQueueCount(int by);

        // ACCESSORS
        int                 numAssignedQueues() const;
        const mqbi::Domain* domain() const;

        /// Return the value of the corresponding member of this object.
        const UriToQueueInfoMap& queuesInfo() const;
    };

    /// This class provides mechanism to extract partition id from queue
    /// name.
    class PartitionIdExtractor {
      private:
        // DATA

        // Allocator to be used.
        bslma::Allocator* d_allocator_p;

        // Regex used to find partitionId.
        bdlpcre::RegEx d_regex;

      public:
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(PartitionIdExtractor,
                                       bslma::UsesBslmaAllocator)

        // CREATORS

        /// Create a new PartitionIdExtractor using the specified
        /// `allocator` for memory allocations.
        explicit PartitionIdExtractor(bslma::Allocator* allocator);

        // ACCESSORS

        /// Parse the specified `queueName` string and try to find the
        /// partition id inside.  Return the id on success, return -1 on
        /// fail.
        int extract(const bsl::string& queueName) const;
    };

    typedef bsl::shared_ptr<DomainState> DomainStateSp;

    /// <domainName> -> <domainState>
    typedef bsl::unordered_map<bsl::string, DomainStateSp> DomainStates;
    typedef DomainStates::iterator                         DomainStatesIter;
    typedef DomainStates::const_iterator                   DomainStatesCIter;

    typedef bsl::unordered_set<mqbu::StorageKey> QueueKeys;
    typedef QueueKeys::iterator                  QueueKeysIter;
    typedef bsl::pair<QueueKeysIter, bool>       QueueKeysInsertRc;

    typedef bsl::unordered_set<ClusterStateObserver*> ObserversSet;
    typedef ObserversSet::iterator                    ObserversSetIter;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    mqbi::Cluster* d_cluster_p;
    // Associated cluster

    PartitionsInfo d_partitionsInfo;
    // Partition information

    DomainStates d_domainStates;
    // Domains information

    QueueKeys d_queueKeys;
    // Set of all existing queue keys.

    ObserversSet d_observers;
    // Observers of the cluster state.

    PartitionIdExtractor d_partitionIdExtractor;
    // Regexp wrapper used to get partition Id.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterState, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbc::ClusterState` with the specified `cluster` and
    /// `partitionsCount`.  Use the specified `allocator` for any memory
    /// allocation.
    explicit ClusterState(mqbi::Cluster*    cluster,
                          int               partitionsCount,
                          bslma::Allocator* allocator);

    // MANIPULATORS

    /// Get a modifiable reference to this object's cluster.
    mqbi::Cluster* cluster();

    /// Get a modifiable reference to this object's domain states.
    DomainStates& domainStates();

    /// Get a modifiable reference to this object's queue keys.
    QueueKeys& queueKeys();

    /// Register the specified `observer` to be notified of state changes.
    /// Return a reference offerring modifiable access to this object.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    ClusterState& registerObserver(ClusterStateObserver* observer);

    /// Un-register the specified `observer` from being notified of state
    /// changes.  Return a reference offerring modifiable access to this
    /// object.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    ClusterState& unregisterObserver(ClusterStateObserver* observer);

    // Partition-related
    // -----------------

    /// Update the status of the specified `partitionId`, to indicate that
    /// the specified `node` is the primary, with the specified `leaseId`.
    /// If `node` is a null pointer, this means the partition has no
    /// primary.  This will notify all active observers by invoking
    /// `onPartitionPrimaryAssignment()` on each of them, with the
    /// `partitionId` and `node` as parameters.  The bahavior is undefined
    /// unless `partitionId >= 0` and `partitionId < partitionsCount`.
    ClusterState& setPartitionPrimary(int                  partitionId,
                                      unsigned int         leaseId,
                                      mqbnet::ClusterNode* node);

    /// Set the status of the primary of the specified `partitionId` to the
    /// specified `value`.
    ClusterState&
    setPartitionPrimaryStatus(int                                partitionId,
                              bmqp_ctrlmsg::PrimaryStatus::Value value);

    /// Update the number of queues mapped to the specified `partitionId` by
    /// adjusting the current value with the specified `delta`.  The
    /// bahavior is undefined unless `partitionId >= 0` and 'partitionId <
    /// partitionsCount'.
    ClusterState& updatePartitionQueueMapped(int partitionId, int delta);

    /// Update the number of queues active on the specified `partitionId` by
    /// adjusting the current value with the specified `delta`.  The
    /// bahavior is undefined unless `partitionId >= 0` and 'partitionId <
    /// partitionsCount'.
    ClusterState& updatePartitionNumActiveQueues(int partitionId, int delta);

    /// Assign the queue with the specified `uri` and `key` to the specified
    /// `partitionId`, and register the specified `appIdInfos` to the queue.
    /// If the queue already appears in cluster state, return false.  Else,
    /// return true.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    bool assignQueue(const bmqt::Uri&        uri,
                     const mqbu::StorageKey& key,
                     int                     partitionId,
                     const AppInfos&         appIdInfos);

    /// Un-assign the queue with the specified `uri`.  Return true if
    /// successful, or false if the queue does not exist.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    bool unassignQueue(const bmqt::Uri& uri);

    /// Un-assign all queues.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    void clearQueues();

    /// Update the queue with the specified `uri` belonging to the specified
    /// `domain` by registering the specified `addedAppIds` and
    /// un-registering the specified `removedAppIds`.  If the specified
    /// `uri` is empty, the appId updates are applied to the entire `domain`
    /// instead.  Return 0 on success or a non-zero error code on failure.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    int updateQueue(const bmqt::Uri&   uri,
                    const bsl::string& domain,
                    const AppInfos&    addedAppIds,
                    const AppInfos&    removedAppIds = AppInfos());

    /// Clear this cluster state object, without firing any observers.
    void clear();

    // ACCESSORS
    const mqbi::Cluster*  cluster() const;
    const PartitionsInfo& partitionsInfo() const;
    const DomainStates&   domainStates() const;
    const QueueKeys&      queueKeys() const;

    /// Return the value of the corresponding member of this object.
    const ObserversSet& observers() const;

    // Partition-related
    // -----------------

    /// Parse the specified `queueName` string and try to find the partition id
    /// inside.  Return the id on success, return -1 on fail.
    int extractPartitionId(const bsl::string& queueName) const;

    /// Return true if self is primary for the specified `partitionId`.
    bool isSelfPrimary(int partitionId) const;

    /// Return true if self is *active* primary for the specified
    /// `partitionId`.
    bool isSelfActivePrimary(int partitionId) const;

    /// Return the number of partitions.
    int partitionsCount() const;

    /// Return true if self is primary for at least one partition.
    bool isSelfPrimary() const;

    /// Return true is self is *active* primary for at least one partition.
    bool isSelfActivePrimary() const;

    /// Return true if for the specified `partitionId`, there is currently a
    /// primary, *and* the primary is active, false otherwise.  Note that
    /// self node could be an active primary as well.  The behavior is
    /// undefined unless `partitionId >= 0` and 'partitionId <
    /// partitionsCount'.
    bool hasActivePrimary(int partitionId) const;

    /// Return a reference to the partitions info.
    const PartitionsInfo& partitions() const;

    /// Return a reference to the PartitionInfo corresponding to the
    /// specified `partitionId`.  This method is the same as
    /// `partitions()[partitionId]` but provided so that `partitionId`
    /// validation can be performed.  The bahavior is undefined unless
    /// `partitionId >= 0` and `partitionId < partitionsCount`.
    const ClusterStatePartitionInfo& partition(int partitionId) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------------
// class ClusterStatePartitionInfo
// -------------------------------

// CREATORS
inline ClusterStatePartitionInfo::ClusterStatePartitionInfo()
: d_partitionId(mqbs::DataStore::k_INVALID_PARTITION_ID)
, d_primaryLeaseId(0)
, d_primaryNodeId(mqbnet::Cluster::k_INVALID_NODE_ID)
, d_numQueuesMapped(0)
, d_numActiveQueues(0)
, d_primaryNode_p(0)
, d_primaryStatus(bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED)
{
    // NOTHING
}

// MANIPULATORS
inline ClusterStatePartitionInfo&
ClusterStatePartitionInfo::setPartitionId(int value)
{
    d_partitionId = value;
    return *this;
}

inline ClusterStatePartitionInfo&
ClusterStatePartitionInfo::setPrimaryLeaseId(unsigned int value)
{
    d_primaryLeaseId = value;
    return *this;
}

inline ClusterStatePartitionInfo&
ClusterStatePartitionInfo::setPrimaryNodeId(int value)
{
    d_primaryNodeId = value;
    return *this;
}

inline ClusterStatePartitionInfo&
ClusterStatePartitionInfo::setNumQueuesMapped(int value)
{
    d_numQueuesMapped = value;
    return *this;
}

inline ClusterStatePartitionInfo&
ClusterStatePartitionInfo::setNumActiveQueues(int value)
{
    d_numActiveQueues = value;
    return *this;
}

inline ClusterStatePartitionInfo&
ClusterStatePartitionInfo::setPrimaryNode(mqbnet::ClusterNode* value)
{
    d_primaryNode_p = value;
    return *this;
}

inline ClusterStatePartitionInfo& ClusterStatePartitionInfo::setPrimaryStatus(
    bmqp_ctrlmsg::PrimaryStatus::Value value)
{
    d_primaryStatus = value;
    return *this;
}

// ACCESSORS
inline int ClusterStatePartitionInfo::partitionId() const
{
    return d_partitionId;
}

inline unsigned int ClusterStatePartitionInfo::primaryLeaseId() const
{
    return d_primaryLeaseId;
}

inline int ClusterStatePartitionInfo::primaryNodeId() const
{
    return d_primaryNodeId;
}

inline int ClusterStatePartitionInfo::numQueuesMapped() const
{
    return d_numQueuesMapped;
}

inline int ClusterStatePartitionInfo::numActiveQueues() const
{
    return d_numActiveQueues;
}

inline mqbnet::ClusterNode* ClusterStatePartitionInfo::primaryNode() const
{
    return d_primaryNode_p;
}

inline bmqp_ctrlmsg::PrimaryStatus::Value
ClusterStatePartitionInfo::primaryStatus() const
{
    return d_primaryStatus;
}

// ---------------------------
// class ClusterStateQueueInfo
// ---------------------------

// CREATORS
inline ClusterStateQueueInfo::ClusterStateQueueInfo(
    const bmqt::Uri&  uri,
    bslma::Allocator* allocator)
: d_uri(uri, allocator)
, d_key()
, d_partitionId(mqbs::DataStore::k_INVALID_PARTITION_ID)
, d_appInfos(allocator)
, d_pendingUnassignment(false)
{
    // NOTHING
}

inline ClusterStateQueueInfo::ClusterStateQueueInfo(
    const bmqt::Uri&        uri,
    const mqbu::StorageKey& key,
    int                     partitionId,
    const AppInfos&         appIdInfos,
    bslma::Allocator*       allocator)
: d_uri(uri, allocator)
, d_key(key)
, d_partitionId(partitionId)
, d_appInfos(appIdInfos, allocator)
, d_pendingUnassignment(false)
{
    // NOTHING
}

// MANIPULATORS
inline ClusterStateQueueInfo&
ClusterStateQueueInfo::setKey(const mqbu::StorageKey& value)
{
    d_key = value;
    return *this;
}

inline ClusterStateQueueInfo& ClusterStateQueueInfo::setPartitionId(int value)
{
    d_partitionId = value;
    return *this;
}

inline ClusterStateQueueInfo&
ClusterStateQueueInfo::setPendingUnassignment(bool value)
{
    d_pendingUnassignment = value;
    return *this;
}

inline ClusterStateQueueInfo::AppInfos& ClusterStateQueueInfo::appInfos()
{
    return d_appInfos;
}

inline void ClusterStateQueueInfo::reset()
{
    // NOTE: Purposely do not reset the URI (URI is an invariant member of a
    //       given instance of ClusterStateQueueInfo object).

    d_key.reset();
    d_partitionId = mqbs::DataStore::k_INVALID_PARTITION_ID;
    d_appInfos.clear();
}

// ACCESSORS
inline const bmqt::Uri& ClusterStateQueueInfo::uri() const
{
    return d_uri;
}

inline const mqbu::StorageKey& ClusterStateQueueInfo::key() const
{
    return d_key;
}

inline int ClusterStateQueueInfo::partitionId() const
{
    return d_partitionId;
}

inline const ClusterStateQueueInfo::AppInfos&
ClusterStateQueueInfo::appInfos() const
{
    return d_appInfos;
}

inline bool ClusterStateQueueInfo::pendingUnassignment() const
{
    return d_pendingUnassignment;
}

// ------------------
// class ClusterState
// ------------------

// CREATORS
inline ClusterState::ClusterState(mqbi::Cluster*    cluster,
                                  int               partitionsCount,
                                  bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_cluster_p(cluster)
, d_partitionsInfo(allocator)
, d_domainStates(allocator)
, d_queueKeys(allocator)
, d_observers(allocator)
, d_partitionIdExtractor(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_cluster_p->isRemote() || partitionsCount > 0);
    // A ClusterProxy has 0 partitions, a local or member must have at
    // least 1 partition.

    d_partitionsInfo.resize(partitionsCount);
    for (int i = 0; i < partitionsCount; ++i) {
        d_partitionsInfo[i].setPartitionId(i);
    }
}

// MANIPULATORS
inline mqbi::Cluster* ClusterState::cluster()
{
    return d_cluster_p;
}

inline ClusterState::DomainStates& ClusterState::domainStates()
{
    return d_domainStates;
}

inline ClusterState::QueueKeys& ClusterState::queueKeys()
{
    return d_queueKeys;
}

// ACCESSORS
inline const mqbi::Cluster* ClusterState::cluster() const
{
    return d_cluster_p;
}

inline const ClusterState::PartitionsInfo& ClusterState::partitionsInfo() const
{
    return d_partitionsInfo;
}

inline const ClusterState::DomainStates& ClusterState::domainStates() const
{
    return d_domainStates;
}

inline const ClusterState::QueueKeys& ClusterState::queueKeys() const
{
    return d_queueKeys;
}

inline const ClusterState::ObserversSet& ClusterState::observers() const
{
    return d_observers;
}

inline int ClusterState::extractPartitionId(const bsl::string& queueName) const
{
    return d_partitionIdExtractor.extract(queueName);
}

inline bool ClusterState::isSelfPrimary(int partitionId) const
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));
    BSLS_ASSERT_SAFE(!cluster()->isRemote());

    if (mqbs::DataStore::k_INVALID_PARTITION_ID == partitionId) {
        return false;  // RETURN
    }

    const ClusterStatePartitionInfo& partitionInfo = partition(partitionId);

    return (partitionInfo.primaryNode() &&
            (partitionInfo.primaryNode()->nodeId() ==
             cluster()->netCluster().selfNodeId()));
}

inline bool ClusterState::isSelfActivePrimary(int partitionId) const
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));

    if (!isSelfPrimary(partitionId)) {
        return false;  // RETURN
    }

    return partition(partitionId).primaryStatus() ==
           bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE;
}

inline int ClusterState::partitionsCount() const
{
    return d_partitionsInfo.size();
}

inline bool ClusterState::isSelfPrimary() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));

    for (PartitionsInfo::const_iterator cit = d_partitionsInfo.begin();
         cit != d_partitionsInfo.end();
         ++cit) {
        const ClusterStatePartitionInfo& pinfo = *cit;
        if (pinfo.primaryNode() == cluster()->netCluster().selfNode()) {
            return true;  // RETURN
        }
    }

    return false;
}

inline bool ClusterState::isSelfActivePrimary() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));

    for (PartitionsInfo::const_iterator cit = d_partitionsInfo.begin();
         cit != d_partitionsInfo.end();
         ++cit) {
        const ClusterStatePartitionInfo& pinfo = *cit;
        if ((pinfo.primaryNode() == cluster()->netCluster().selfNode()) &&
            (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE == pinfo.primaryStatus())) {
            return true;  // RETURN
        }
    }

    return false;
}

inline bool ClusterState::hasActivePrimary(int partitionId) const
{
    return 0 != partition(partitionId).primaryNode() &&
           bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE ==
               partition(partitionId).primaryStatus();
}

inline const mqbc::ClusterState::PartitionsInfo&
ClusterState::partitions() const
{
    return d_partitionsInfo;
}

inline const ClusterStatePartitionInfo&
ClusterState::partition(int partitionId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0);
    BSLS_ASSERT_SAFE(partitionId < static_cast<int>(d_partitionsInfo.size()));

    return d_partitionsInfo[partitionId];
}

// --------------------------------
// struct ClusterState::DomainState
// --------------------------------

// CREATORS
inline ClusterState::DomainState::DomainState(bslma::Allocator* allocator)
: d_numAssignedQueues(0)
, d_domain_p(0)
, d_queuesInfo(allocator)
{
    // NOTHING
}

// MANIPULATORS
inline mqbi::Domain* ClusterState::DomainState::domain()
{
    return d_domain_p;
}

inline ClusterState::UriToQueueInfoMap& ClusterState::DomainState::queuesInfo()
{
    return d_queuesInfo;
}

inline void ClusterState::DomainState::setDomain(mqbi::Domain* domain)
{
    d_domain_p = domain;
}

// ACCESSORS
inline int ClusterState::DomainState::numAssignedQueues() const
{
    return d_numAssignedQueues;
}

inline const mqbi::Domain* ClusterState::DomainState::domain() const
{
    return d_domain_p;
}

inline const ClusterState::UriToQueueInfoMap&
ClusterState::DomainState::queuesInfo() const
{
    return d_queuesInfo;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mqbc::operator<<(bsl::ostream&                stream,
                                      const ClusterStateQueueInfo& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
