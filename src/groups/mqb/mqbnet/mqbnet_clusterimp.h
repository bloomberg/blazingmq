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

// mqbnet_clusterimp.h                                                -*-C++-*-
#ifndef INCLUDED_MQBNET_CLUSTERIMP
#define INCLUDED_MQBNET_CLUSTERIMP

//@PURPOSE: Provide a mechanism to manipulate a cluster of nodes.
//
//@CLASSES:
//  mqbnet::ClusterNodeImp    : Mechanism to represent and interact with a node
//  mqbnet::ClusterImp        : Mechanism to manipulate a collection of nodes
//
//@DESCRIPTION: 'mqbnet::ClusterImp' is a mechanism, implementing the
// 'mqbnet::Cluster' protocol, to manipulate a collection of ClusterNode.
// 'mqbnet::ClusterNodeImp' is a mechanism, implementing the
// 'mqbnet::ClusterNode' protocol to represent and interact with a node.
//
// NOTE: The 'mqbnet::Cluster' and 'mqbnet::ClusterNode' interfaces are mainly
//       so that those components can be mocked for testing.

// MQB

#include <mqbcfg_messages.h>
#include <mqbnet_channel.h>
#include <mqbnet_cluster.h>

#include <bmqio_channel.h>
#include <bmqio_status.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_throttle.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

namespace mqbnet {

// FORWARD DECLARATIONS
class ClusterImp;
class ClusterNodeImp;

// ====================
// class ClusterNodeImp
// ====================

/// Mechanism to represent and interact with a node.
class ClusterNodeImp : public ClusterNode {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.CLUSTERNODEIMP");

  private:
    // DATA
    ClusterImp* d_cluster_p;
    // Cluster this node belongs to

    mqbcfg::ClusterNode d_config;
    // Configuration of this node

    bsl::string d_description;
    // Brief description of this node

    Channel d_channel;
    // Channel associated to this node

    bmqp_ctrlmsg::ClientIdentity d_identity;

    bmqio::Channel::ReadCallback d_readCb;

    bool d_isReading;
    // Indicates if post-negotiation read has
    // started.

  private:
    // NOT IMPLEMENTED
    ClusterNodeImp(const ClusterNodeImp&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    ClusterNodeImp& operator=(const ClusterNodeImp&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterNodeImp, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object, associated to the specified `cluster` with the
    /// specified `config` and using the specified `allocator`.
    ClusterNodeImp(ClusterImp*                cluster,
                   const mqbcfg::ClusterNode& config,
                   bdlbb::BlobBufferFactory*  blobBufferFactory,
                   Channel::ItemPool*         itemPool,
                   bslma::Allocator*          allocator);

    /// Destructor.
    ~ClusterNodeImp() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Return associated channel.
    Channel& channel() BSLS_KEYWORD_OVERRIDE;

    /// Set the channel associated to this node to the specified `value` and
    /// return a pointer to this object.  Store the specified `identity`.
    /// The specified `readCb` serves as read data callback when
    /// `enableRead` is called.
    ClusterNode* setChannel(const bsl::weak_ptr<bmqio::Channel>& value,
                            const bmqp_ctrlmsg::ClientIdentity&  identity,
                            const bmqio::Channel::ReadCallback&  readCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Start reading from the channel.  Return true if `read` is successful
    /// or if it is already reading.
    bool enableRead() BSLS_KEYWORD_OVERRIDE;

    /// Reset the channel associated to this node.
    ClusterNode* resetChannel() BSLS_KEYWORD_OVERRIDE;

    /// Close the channel associated to this node, if any.
    void closeChannel() BSLS_KEYWORD_OVERRIDE;

    /// Enqueue the specified message `blob` of the specified `type`to be
    /// written to the channel associated to this node.  Return 0 on
    /// success, and a non-zero value otherwise.  Note that success does not
    /// imply that the data has been written or will be successfully written
    /// to the underlying stream used by this channel.
    bmqt::GenericResult::Enum
    write(const bdlbb::Blob&    blob,
          bmqp::EventType::Enum type) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    const bmqp_ctrlmsg::ClientIdentity& identity() const BSLS_KEYWORD_OVERRIDE;
    // Return identity from the last received negotiation message.

    /// Return the id of this node.
    int nodeId() const BSLS_KEYWORD_OVERRIDE;

    /// Return the name of this node.
    const bsl::string& hostName() const BSLS_KEYWORD_OVERRIDE;

    /// Return a string which contains a brief description of the node.
    /// Note that this method is provided only for logging purposes and
    /// format of returned string can change anytime.
    const bsl::string& nodeDescription() const BSLS_KEYWORD_OVERRIDE;

    /// Return the dataCenter this node resides in.
    const bsl::string& dataCenter() const BSLS_KEYWORD_OVERRIDE;

    /// Return a pointer to the cluster this node belongs to.
    const Cluster* cluster() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this node is available, i.e., it has an attached
    /// channel.
    bool isAvailable() const BSLS_KEYWORD_OVERRIDE;
};

// =============
// class Cluster
// =============

/// Mechanism to manipulate a collection of nodes.
class ClusterImp : public Cluster {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.CLUSTERIMP");

  private:
    // PRIVATE TYPES

    /// A set of cluster observers
    typedef bsl::unordered_set<ClusterObserver*> ObserversSet;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    bsl::string d_name;
    // Name of this Cluster

    bsl::vector<mqbcfg::ClusterNode> d_nodesConfig;
    // Configuration of nodes in this
    // Cluster.

    int d_selfNodeId;
    // node Id of the current broker in this
    // cluster, or -1 if the broker isn't a
    // member of that cluster.

    ClusterNode* d_selfNode;
    // Node of the current broker in this
    // cluster, or 0 (null) if the broker
    // isn't a member of that cluster.

    bsl::list<ClusterNodeImp> d_nodes;

    Cluster::NodesList d_nodesList;
    // List of all Nodes in the cluster.  A
    // cluster will never be more than a
    // handful (2, 4, 6, ..) nodes, so we
    // can use a vector list and linear
    // lookup won't hurt us (can't use a
    // vector because ClusterNode is not
    // copyable).
    // NOTE: 'd_nodes' is the real list
    // holding the nodes, 'd_nodesList' is
    // just a list of pointer to the nodes
    // (in 'd_nodes'), to honor the
    // 'mqbnet::Cluster' interface and so
    // that we don't keep recreating it.

    ObserversSet d_nodeStateObservers;
    // Registered observers of a change of
    // state of a node in the cluster.

    bdlmt::Throttle d_failedWritesThrottler;
    // Throttling for failed writes on any
    // peer node in the cluster.

    bslmt::Mutex d_mutex;
    // Mutex for thread-safety of this
    // component.

    bool d_isReadEnabled;
    // 'true' if cluster has been
    // initialized to enable reading from
    // its nodes.

    // FRIENDS
    friend class ClusterNodeImp;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    ClusterImp(const ClusterImp&);             // = delete;
    ClusterImp& operator=(const ClusterImp&);  // = delete;

  private:
    /// Notify all nodeStateObservers that the specified `node` is now in
    /// the specified `state`.
    void notifyObserversOfNodeStateChange(ClusterNode* node, bool state);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterImp, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object with the specified `name`, `nodesConfig` and
    /// `selfNodeId`, using the specified `allocator`.
    ClusterImp(const bsl::string&                      name,
               const bsl::vector<mqbcfg::ClusterNode>& nodesConfig,
               int                                     selfNodeId,
               bdlbb::BlobBufferFactory*               blobBufferFactory,
               Channel::ItemPool*                      itemPool,
               bslma::Allocator*                       allocator);

    /// Destructor
    ~ClusterImp() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Register the specified `observer` to be notified of node state
    /// changes.  Return a pointer to this object.
    Cluster* registerObserver(ClusterObserver* observer) BSLS_KEYWORD_OVERRIDE;

    /// Un-register the specified `observer` from being notified of node
    /// state changes.  Return a pointer to this object.
    Cluster*
    unregisterObserver(ClusterObserver* observer) BSLS_KEYWORD_OVERRIDE;

    /// Write the specified `blob` of the specified `type` to all connected
    /// nodes of this cluster (with the exception of the current node).
    /// Return the maximum number of pending items across all cluster
    /// channels prior to broadcasting.
    int writeAll(const bdlbb::Blob&    blob,
                 bmqp::EventType::Enum type) BSLS_KEYWORD_OVERRIDE;

    /// Send the specified `blob` to all currently up nodes of this cluster
    /// (exception of the current node).  Return the maximum number of
    /// pending items across all cluster channels prior to broadcasting.
    int broadcast(const bdlbb::Blob& blob) BSLS_KEYWORD_OVERRIDE;

    /// Close the channels associated to all nodes in this cluster.
    void closeChannels() BSLS_KEYWORD_OVERRIDE;

    /// Lookup the node having the specified `nodeId` and return a pointer
    /// to it, or the null pointer if no such node was found.
    ClusterNode* lookupNode(int nodeId) BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the list of nodes part of this cluster.
    ///
    /// NOTE: the returned list itself should *NOT* be changed.
    Cluster::NodesList& nodes() BSLS_KEYWORD_OVERRIDE;

    /// Start reading from all channels.
    void enableRead() BSLS_KEYWORD_OVERRIDE;

    /// Process incoming proxy connection.
    void
    onProxyConnectionUp(const bsl::shared_ptr<bmqio::Channel>& channel,
                        const bmqp_ctrlmsg::ClientIdentity&    identity,
                        const bsl::string& description) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the name of that cluster.
    const bsl::string& name() const BSLS_KEYWORD_OVERRIDE;

    /// Return the node Id of the current broker in that cluster, or -1 if
    /// the broker isn't a member of that cluster.
    int selfNodeId() const BSLS_KEYWORD_OVERRIDE;

    /// Return a pointer to the node of the current broker in that cluster,
    /// or a null pointer if the broker isn't a member of that cluster.
    const ClusterNode* selfNode() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference not offering modifiable access to the list of
    /// nodes part of this cluster.
    const Cluster::NodesList& nodes() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// class ClusterNodeImp
// --------------------

inline Channel& ClusterNodeImp::channel()
{
    return d_channel;
}

inline const bmqp_ctrlmsg::ClientIdentity& ClusterNodeImp::identity() const
{
    return d_identity;
}

inline int ClusterNodeImp::nodeId() const
{
    return d_config.id();
}

inline const bsl::string& ClusterNodeImp::hostName() const
{
    return d_config.name();
}

inline const bsl::string& ClusterNodeImp::nodeDescription() const
{
    return d_description;
}

inline const bsl::string& ClusterNodeImp::dataCenter() const
{
    return d_config.dataCenter();
}

inline const Cluster* ClusterNodeImp::cluster() const
{
    return d_cluster_p;
}

inline bool ClusterNodeImp::isAvailable() const
{
    return d_channel.isAvailable();
}

// ----------------
// class ClusterImp
// ----------------

// MANIPULATORS
inline ClusterNode* ClusterImp::lookupNode(int nodeId)
{
    for (bsl::list<ClusterNodeImp>::iterator it = d_nodes.begin();
         it != d_nodes.end();
         ++it) {
        if (it->nodeId() == nodeId) {
            return &(*it);  // RETURN
        }
    }

    return 0;
}

// ACCESSORS
inline const bsl::string& ClusterImp::name() const
{
    return d_name;
}

inline int ClusterImp::selfNodeId() const
{
    return d_selfNodeId;
}

inline const ClusterNode* ClusterImp::selfNode() const
{
    return d_selfNode;
}

inline Cluster::NodesList& ClusterImp::nodes()
{
    return d_nodesList;
}

inline const Cluster::NodesList& ClusterImp::nodes() const
{
    return d_nodesList;
}

}  // close package namespace
}  // close enterprise namespace

#endif
