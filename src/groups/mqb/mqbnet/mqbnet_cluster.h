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

// mqbnet_cluster.h                                                   -*-C++-*-
#ifndef INCLUDED_MQBNET_CLUSTER
#define INCLUDED_MQBNET_CLUSTER

//@PURPOSE: Provide interfaces for mechanisms to manipulate a cluster of nodes.
//
//@CLASSES:
//  mqbnet::ClusterNode    : Interface to represent and interact with a node
//  mqbnet::Cluster        : Interface to manipulate a collection of nodes
//  mqbnet::ClusterObserver: Interface for an observer of a cluster
//
//@DESCRIPTION: 'mqbnet::Cluster' is an interface for a mechanism to manipulate
// a collection of ClusterNode.  'mqbnet::ClusterNode' is an interface for a
// mechanism to represent and interact with a node.  'mqbnet::ClusterObserver'
// is an interface that clients can implement if they want to observe some
// events happening on the cluster.

// MQB

#include <mqbnet_channel.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MWC
#include <mwcio_channel.h>
#include <mwcio_status.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace mqbnet {

// FORWARD DECLARATIONS
class Cluster;
class ClusterNode;

// ==================
// struct ClusterUtil
// ==================

/// Return `true` if the specified `negoMsg` is from a proxy connecting
/// to a cluster with the specified `clusterName`.
struct ClusterUtil {
    static bool isProxy(const bmqp_ctrlmsg::NegotiationMessage& negoMsg,
                        const bsl::string&                      clusterName);

    /// Return `true` if the specified `negoMsg` is from a client.
    static bool isClient(const bmqp_ctrlmsg::NegotiationMessage& negoMsg);

    /// Return `true` if the specified `negoMsg` is from a client or proxy.
    static bool
    isClientOrProxy(const bmqp_ctrlmsg::NegotiationMessage& negoMsg);
};

// =====================
// class ClusterObserver
// =====================

/// This interface exposes notifications of events happening on the cluster.
///
/// NOTE: This is purposely not a pure interface, each method has a default
///       void implementation, so that clients only need to implement the
///       ones they care about.
class ClusterObserver {
  public:
    // CREATORS

    /// Destructor
    virtual ~ClusterObserver();

    // MANIPULATORS

    /// Notification method to indicate that the specified `node` is now
    /// available (if the specified `isAvailable` is true) or not available
    /// (if `isAvailable` is false).
    virtual void onNodeStateChange(ClusterNode* node, bool isAvailable);

    /// Process incoming proxy connection
    virtual void
    onProxyConnectionUp(const bsl::shared_ptr<mwcio::Channel>& channel,
                        const bmqp_ctrlmsg::ClientIdentity&    identity,
                        const bsl::string&                     description);
};

// =================
// class ClusterNode
// =================

/// Interface for a mechanism to represent and interact with a node.  Note
/// that the `ClusterNode` does, purposely, not expose its associated
/// channel: `ClusterNode` should be seen as a proxy mechanism and all
/// writes to the underlying channel should be done through its interface,
/// so that features such as throttling, nagling, statistics gathering, ...,
/// can be performed.
class ClusterNode {
  public:
    // CREATORS

    /// Destructor.
    virtual ~ClusterNode();

    // MANIPULATORS

    /// Return associated channel.
    virtual Channel& channel() = 0;

    /// Set the channel associated to this node to the specified `value` and
    /// return a pointer to this object.  Store the specified `identity`.
    /// The specified `readCb` serves as read data callback when
    /// `enableRead` is called.
    virtual ClusterNode*
    setChannel(const bsl::weak_ptr<mwcio::Channel>& value,
               const bmqp_ctrlmsg::ClientIdentity&  identity,
               const mwcio::Channel::ReadCallback&  readCb) = 0;

    /// Start reading from the channel.  Return true if `read` is successful
    /// or if it is already reading.
    virtual bool enableRead() = 0;

    /// Reset the channel associated to this node.
    virtual ClusterNode* resetChannel() = 0;

    /// Close the channel associated to this node, if any.
    virtual void closeChannel() = 0;

    /// Enqueue the specified message `blob` of the specified `type`to be
    /// written to the channel associated to this node.  Return 0 on
    /// success, and a non-zero value otherwise.  Note that success does not
    /// imply that the data has been written or will be successfully written
    /// to the underlying stream used by this channel.
    virtual bmqt::GenericResult::Enum
    write(const bsl::shared_ptr<bdlbb::Blob>& blob,
          bmqp::EventType::Enum               type) = 0;

    // ACCESSORS

    /// Return identity from the last received negotiation message.
    virtual const bmqp_ctrlmsg::ClientIdentity& identity() const = 0;

    /// Return the id of this node.
    virtual int nodeId() const = 0;

    /// Return the name of this node.
    virtual const bsl::string& hostName() const = 0;

    /// Return a string which contains a brief description of the node.
    /// Note that this method is provided only for logging purposes and
    /// format of returned string can change anytime.
    virtual const bsl::string& nodeDescription() const = 0;

    /// Return the dataCenter this node resides in.
    virtual const bsl::string& dataCenter() const = 0;

    /// Return a pointer to the cluster this node belongs to.
    virtual const Cluster* cluster() const = 0;

    /// Return true if this node is available, i.e., it has an attached
    /// channel.
    virtual bool isAvailable() const = 0;
};

// =============
// class Cluster
// =============

/// Interface for a mechanism to manipulate a collection of nodes.
class Cluster {
  public:
    // TYPES
    typedef bsl::list<ClusterNode*> NodesList;

    // PUBLIC CLASS DATA
    static const int k_INVALID_NODE_ID = -1;

    /// Constant integer value which can be used to represent all nodes' IDs
    /// in the code.
    static const int k_ALL_NODES_ID;

  public:
    // CREATORS

    /// Destructor
    virtual ~Cluster();

    // MANIPULATORS

    /// Register the specified `observer` to be notified of node state
    /// changes.  Return a pointer to this object.
    virtual Cluster* registerObserver(ClusterObserver* observer) = 0;

    /// Un-register the specified `observer` from being notified of node
    /// state changes.  Return a pointer to this object.
    virtual Cluster* unregisterObserver(ClusterObserver* observer) = 0;

    /// Write the specified `blob` of the specified `type` to all connected
    /// nodes of this cluster (with the exception of the current node).
    /// Return the maximum number of pending items across all cluster
    /// channels prior to broadcasting.
    virtual int writeAll(const bsl::shared_ptr<bdlbb::Blob>& blob,
                         bmqp::EventType::Enum               type) = 0;

    /// Send the specified `blob` to all currently up nodes of this cluster
    /// (exception of the current node).  Return the maximum number of
    /// pending items across all cluster channels prior to broadcasting.
    virtual int broadcast(const bsl::shared_ptr<bdlbb::Blob>& blob) = 0;

    /// Close the channels associated to all nodes in this cluster.
    virtual void closeChannels() = 0;

    /// Lookup the node having the specified `nodeId` and return a pointer
    /// to it, or the null pointer if no such node was found.
    virtual ClusterNode* lookupNode(int nodeId) = 0;

    /// Return a reference to the list of nodes part of this cluster.
    ///
    /// NOTE: the returned list itself should *NOT* be changed.
    virtual NodesList& nodes() = 0;

    /// Start reading from all channels.
    virtual void enableRead() = 0;

    /// Process incoming proxy connection.
    virtual void
    onProxyConnectionUp(const bsl::shared_ptr<mwcio::Channel>& channel,
                        const bmqp_ctrlmsg::ClientIdentity&    identity,
                        const bsl::string& description) = 0;

    // ACCESSORS

    /// Return the name of that cluster.
    virtual const bsl::string& name() const = 0;

    /// Return the node Id of the current broker in that cluster, or -1 if
    /// the broker isn't a member of that cluster.
    virtual int selfNodeId() const = 0;

    /// Return a pointer to the node of the current broker in that cluster,
    /// or a null pointer if the broker isn't a member of that cluster.
    virtual const ClusterNode* selfNode() const = 0;

    /// Return a reference not offering modifiable access to the list of
    /// nodes part of this cluster.
    virtual const NodesList& nodes() const = 0;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline bool
ClusterUtil::isProxy(const bmqp_ctrlmsg::NegotiationMessage& negoMsg,
                     const bsl::string&                      clusterName)
{
    return isClientOrProxy(negoMsg) &&
           negoMsg.clientIdentity().clusterName() == clusterName;
}

inline bool
ClusterUtil::isClient(const bmqp_ctrlmsg::NegotiationMessage& negoMsg)
{
    return isClientOrProxy(negoMsg) &&
           negoMsg.clientIdentity().clusterName().empty();
}

inline bool
ClusterUtil::isClientOrProxy(const bmqp_ctrlmsg::NegotiationMessage& negoMsg)
{
    return negoMsg.isClientIdentityValue() &&
           (negoMsg.clientIdentity().clusterNodeId() ==
                mqbnet::Cluster::k_INVALID_NODE_ID ||
            negoMsg.clientIdentity().clusterName().empty());
}

}  // close package namespace
}  // close enterprise namespace

#endif
