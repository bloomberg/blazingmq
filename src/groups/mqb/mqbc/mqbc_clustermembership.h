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

// mqbc_clustermembership.h                                           -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERMEMBERSHIP
#define INCLUDED_MQBC_CLUSTERMEMBERSHIP

//@PURPOSE: Provide a VST representing the membership state of a cluster.
//
//@CLASSES:
//  mqbc::ClusterMembershipObserver: Interface for a ClusterMembership observer
//  mqbc::ClusterMembership        : VST for cluster membership information
//
//@DESCRIPTION: 'mqbc::ClusterMembership' is a value-semantic type representing
// the membership state of a cluster.  Important changes in membership state
// can be notified to observers, implementing the
// 'mqbc::ClusterMembershipObserver' interface.
//
/// Thread Safety
///-------------
// The 'mqbc::ClusterMembership' object is not thread safe and should always be
// manipulated from the associated cluster's dispatcher thread.

// MQB

#include <bmqp_ctrlmsg_messages.h>
#include <mqbc_clusternodesession.h>
#include <mqbcfg_messages.h>
#include <mqbstat_clusterstats.h>

// BDE
#include <bsl_memory.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bslma_managedptr.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}
namespace mqbnet {
class Cluster;
}

namespace mqbc {

// FORWARD DECLARATION
class ClusterNodeSession;

// ===============================
// class ClusterMembershipObserver
// ===============================

/// This interface exposes notifications of events happening on the cluster
/// membership state.
///
/// NOTE: This is purposely not a pure interface, each method has a default
///       void implementation, so that clients only need to implement the
///       ones they care about.
class ClusterMembershipObserver {
  public:
    // CREATORS

    /// Destructor
    virtual ~ClusterMembershipObserver();

    // MANIPULATORS

    /// Callback invoked when self node's status changes to the specified
    /// `value`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onSelfNodeStatus(bmqp_ctrlmsg::NodeStatus::Value value);
};

// =======================
// class ClusterMembership
// =======================

/// This class provides a VST representing the membership information of a
/// cluster.
class ClusterMembership {
  public:
    // TYPES
    typedef bsl::shared_ptr<mqbc::ClusterNodeSession> ClusterNodeSessionSp;

    typedef bsl::unordered_map<mqbnet::ClusterNode*, ClusterNodeSessionSp>
        ClusterNodeSessionMap;

    typedef ClusterNodeSessionMap::iterator ClusterNodeSessionMapIter;

    typedef ClusterNodeSessionMap::const_iterator
        ClusterNodeSessionMapConstIter;

    typedef bsl::unordered_set<ClusterMembershipObserver*> ObserversSet;
    // A set of ClusterMembership observers.

  private:
    // DATA

    // from mqbblp::ClusterState
    bslma::ManagedPtr<mqbnet::Cluster> d_netCluster_mp;
    // The network cluster this cluster
    // operates on

    mqbc::ClusterNodeSession* d_selfNodeSession_p;
    // Node session of self node

    ClusterNodeSessionMap d_nodeSessionMap;
    // Map: ClusterNode -> NodeSession

    ObserversSet d_observers;
    // Observers of this object

  public:
    // CREATORS

    /// Create a new `mqbc::ClusterMembership` object with the specified
    /// `netCluster`.  Use the specified `allocator` to supply memory.
    ClusterMembership(bslma::ManagedPtr<mqbnet::Cluster> netCluster,
                      bslma::Allocator*                  allocator);

    // MANIPULATORS

    /// Register the specified `observer` to be notified of state changes.
    /// Return a reference offerring modifiable access to this object.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    ClusterMembership& registerObserver(ClusterMembershipObserver* observer);

    /// Un-register the specified `observer` from being notified of state
    /// changes.  Return a reference offerring modifiable access to this
    /// object.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    ClusterMembership& unregisterObserver(ClusterMembershipObserver* observer);

    ClusterMembership& setSelfNodeSession(mqbc::ClusterNodeSession* value);
    ClusterMembership&
    setSelfNodeStatus(bmqp_ctrlmsg::NodeStatus::Value value);
    // Set the corresponding member to the specified 'value' and return a
    // reference offering modifiable access to this object.

    /// Get a modifiable reference to this object's net cluster.
    mqbnet::Cluster* netCluster();

    mqbc::ClusterNodeSession* selfNodeSession();

    /// Get a modifiable reference to this object's clusterNodeSessionMap.
    ClusterNodeSessionMap& clusterNodeSessionMap();

    // ACCESSORS
    mqbnet::Cluster* netCluster() const;

    /// Return the value of the corresponding member of this object.
    const ClusterNodeSessionMap& clusterNodeSessionMap() const;

    /// Return a pointer to the node session corresponding to the specified
    /// `key`, or null if no mapping exists.
    mqbc::ClusterNodeSession*
    getClusterNodeSession(mqbnet::ClusterNode* key) const;

    mqbnet::ClusterNode*            selfNode() const;
    mqbc::ClusterNodeSession*       selfNodeSession() const;
    bmqp_ctrlmsg::NodeStatus::Value selfNodeStatus() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class ClusterMembership
// -----------------------

// CREATORS
inline ClusterMembership::ClusterMembership(
    bslma::ManagedPtr<mqbnet::Cluster> netCluster,
    bslma::Allocator*                  allocator)
: d_netCluster_mp(netCluster)
, d_nodeSessionMap(allocator)
, d_observers(allocator)
{
    // NOTHING
}

// ACCESSORS
inline mqbnet::Cluster* ClusterMembership::netCluster() const
{
    return d_netCluster_mp.get();
}

inline const ClusterMembership::ClusterNodeSessionMap&
ClusterMembership::clusterNodeSessionMap() const
{
    return d_nodeSessionMap;
}

inline mqbc::ClusterNodeSession*
ClusterMembership::getClusterNodeSession(mqbnet::ClusterNode* key) const
{
    ClusterNodeSessionMap::const_iterator cit = clusterNodeSessionMap().find(
        key);
    if (cit == clusterNodeSessionMap().end()) {
        return 0;  // RETURN
    }

    return cit->second.get();
}

inline mqbnet::ClusterNode* ClusterMembership::selfNode() const
{
    return const_cast<mqbnet::ClusterNode*>(d_netCluster_mp->selfNode());
}

inline mqbc::ClusterNodeSession* ClusterMembership::selfNodeSession() const
{
    return d_selfNodeSession_p;
}

inline bmqp_ctrlmsg::NodeStatus::Value
ClusterMembership::selfNodeStatus() const
{
    return selfNodeSession()->nodeStatus();
}

// MANIPULATORS
inline ClusterMembership&
ClusterMembership::setSelfNodeSession(mqbc::ClusterNodeSession* value)
{
    d_selfNodeSession_p = value;
    return *this;
}

inline mqbnet::Cluster* ClusterMembership::netCluster()
{
    return d_netCluster_mp.get();
}

inline mqbc::ClusterNodeSession* ClusterMembership::selfNodeSession()
{
    return d_selfNodeSession_p;
}

inline ClusterMembership::ClusterNodeSessionMap&
ClusterMembership::clusterNodeSessionMap()
{
    return d_nodeSessionMap;
}

}  // close package namespace
}  // close enterprise namespace

#endif
