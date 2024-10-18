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

// mqbnet_mockcluster.h                                               -*-C++-*-
#ifndef INCLUDED_MQBNET_MOCKCLUSTER
#define INCLUDED_MQBNET_MOCKCLUSTER

//@PURPOSE: Provide a mechanism to mock a cluster of nodes.
//
//@CLASSES:
//  mqbnet::MockClusterNode  : Mechanism to mock a node
//  mqbnet::MockCluster      : Mechanism to mock a collection of nodes
//
//@DESCRIPTION: 'mqbnet::MockCluster' is a mechanism to mock the
// 'mqbnet::Cluster' protocol, to manipulate a collection of ClusterNode.
// 'mqbnet::MockClusterNode' is a mechanism to mock the 'mqbnet::ClusterNode'
// protocol to represent and interact with a node.

// MQB

#include <mqbcfg_messages.h>
#include <mqbnet_cluster.h>

// MWC
#include <mwcio_channel.h>
#include <mwcio_status.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>

namespace BloombergLP {

namespace mqbnet {

// FORWARD DECLARATIONS
class MockCluster;
class MockClusterNode;

// =====================
// class MockClusterNode
// =====================

/// Mechanism to mock a cluster node.
class MockClusterNode : public ClusterNode {
  private:
    // DATA
    MockCluster* d_cluster_p;
    // Cluster this node belongs to

    mqbcfg::ClusterNode d_config;
    // Configuration of this node

    bsl::string d_description;

    Channel d_channel;
    // Channel associated to this node, if
    // any

    bmqp_ctrlmsg::ClientIdentity d_identity;

    mwcio::Channel::ReadCallback d_readCb;

    bool d_isReading;
    // Indicates if post-negotiation read
    // has started.
  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    MockClusterNode(const MockClusterNode&);             // = delete;
    MockClusterNode& operator=(const MockClusterNode&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MockClusterNode, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object, associated to the specified `cluster` with the
    /// specified `config` and using the specified `allocator`.
    MockClusterNode(MockCluster*               cluster,
                    const mqbcfg::ClusterNode& config,
                    bdlbb::BlobBufferFactory*  blobBufferFactory,
                    Channel::ItemPool*         itemPool,
                    bslma::Allocator*          allocator);

    /// Destructor.
    ~MockClusterNode() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbnet::ClusterNode)

    /// Return associated channel.
    Channel& channel() BSLS_KEYWORD_OVERRIDE;

    /// Set the channel associated to this node to the specified `value` and
    /// return a pointer to this object.  Store the specified `identity`.
    /// The specified `readCb` serves as read data callback when
    /// `enableRead` is called.
    ClusterNode* setChannel(const bsl::weak_ptr<mwcio::Channel>& value,
                            const bmqp_ctrlmsg::ClientIdentity&  identity,
                            const mwcio::Channel::ReadCallback&  readCb)
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
    write(const bsl::shared_ptr<bdlbb::Blob>& blob,
          bmqp::EventType::Enum               type) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual mqbnet::ClusterNode)

    /// Return identity from the last received negotiation message.
    const bmqp_ctrlmsg::ClientIdentity& identity() const BSLS_KEYWORD_OVERRIDE;

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

// =================
// class MockCluster
// =================

/// Mechanism to manipulate a collection of nodes.
class MockCluster : public Cluster {
  private:
    // PRIVATE TYPES

    /// A set of cluster observers
    typedef bsl::unordered_set<ClusterObserver*> ObserversSet;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    mqbcfg::ClusterDefinition d_config;
    // Configuration of this Cluster.

    bsl::list<MockClusterNode> d_nodes;

    Cluster::NodesList d_nodesList;
    // List of all Nodes in the cluster.  A
    // cluster will never be more than a
    // handful (2, 4, 6, ..) nodes, so we
    // can use a vector list and linear
    // lookup won't hurt us (can't use a
    // vector because ClusterNode is not
    // copyable). NOTE: 'd_nodes' is the
    // real list holding the nodes,
    // 'd_nodesList' is just a list of
    // pointer to the nodes (in 'd_nodes'),
    // to honor the 'mqbnet::Cluster'
    // interface and so that we don't keep
    // recreating it.

    int d_selfNodeId;
    // node Id of the current broker in
    // this cluster, or -1 if the broker
    // isn't a member of that cluster.

    ObserversSet d_nodeStateObservers;
    // Registered observers of a change of
    // state of a node in the cluster.

    bslmt::Mutex d_mutex;
    // Mutex for thread-safety of this
    // component.

    bool d_isReadEnabled;
    // 'true' if cluster has been
    // initialized to enable reading from
    // its nodes.

    bool d_disableBroadcast;
    // Whether to disable broadcast.

    // FRIENDS
    friend class MockClusterNode;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    MockCluster(const MockCluster&);             // = delete;
    MockCluster& operator=(const MockCluster&);  // = delete;

  private:
    /// Notify all nodeStateObservers that the specified `node` is now in
    /// the specified `state`.
    void notifyObserversOfNodeStateChange(ClusterNode* node, bool state);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MockCluster, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object with the specified `config`, using the specified
    /// `allocator`.
    MockCluster(const mqbcfg::ClusterDefinition& config,
                bdlbb::BlobBufferFactory*        blobBufferFactory,
                Channel::ItemPool*               itemPool,
                bslma::Allocator*                allocator);

    /// Destructor
    ~MockCluster() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbnet::Cluster)

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
    int writeAll(const bsl::shared_ptr<bdlbb::Blob>& blob,
                 bmqp::EventType::Enum type) BSLS_KEYWORD_OVERRIDE;

    /// Send the specified `blob` to all currently up nodes of this cluster
    /// (exception of the current node).  Return the maximum number of
    /// pending items across all cluster channels prior to broadcasting.
    int
    broadcast(const bsl::shared_ptr<bdlbb::Blob>& blob) BSLS_KEYWORD_OVERRIDE;

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
    onProxyConnectionUp(const bsl::shared_ptr<mwcio::Channel>& channel,
                        const bmqp_ctrlmsg::ClientIdentity&    identity,
                        const bsl::string& description) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (specific to mqbnet::MockCluster)
    MockCluster& _setSelfNodeId(int value);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to self.
    MockCluster& _setDisableBroadcast(bool value);

    // ACCESSORS
    //   (virtual mqbnet::Cluster)

    /// Return the name of that cluster.
    const bsl::string& name() const BSLS_KEYWORD_OVERRIDE;

    /// Return the node Id of the current broker in that cluster, or -1 if
    /// the broker isn't a member of that cluster.
    int selfNodeId() const BSLS_KEYWORD_OVERRIDE;

    /// Return a pointer to the node of the current broker in that cluster,
    /// or a null pointer if the broker isn't a member of that cluster.
    const mqbnet::ClusterNode* selfNode() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference not offering modifiable access to the list of
    /// nodes part of this cluster.
    const Cluster::NodesList& nodes() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class MockClusterNode
// ---------------------

inline Channel& MockClusterNode::channel()
{
    return d_channel;
}

inline const bmqp_ctrlmsg::ClientIdentity& MockClusterNode::identity() const
{
    return d_identity;
}

inline int MockClusterNode::nodeId() const
{
    return d_config.id();
}

inline const bsl::string& MockClusterNode::hostName() const
{
    return d_config.name();
}

inline const bsl::string& MockClusterNode::nodeDescription() const
{
    return d_description;
}

inline const bsl::string& MockClusterNode::dataCenter() const
{
    return d_config.dataCenter();
}

inline const Cluster* MockClusterNode::cluster() const
{
    return d_cluster_p;
}

inline bool MockClusterNode::isAvailable() const
{
    return d_channel.isAvailable();
}

// -----------------
// class MockCluster
// -----------------

inline const bsl::string& MockCluster::name() const
{
    return d_config.name();
}

inline int MockCluster::selfNodeId() const
{
    return d_selfNodeId;
}

inline const mqbnet::ClusterNode* MockCluster::selfNode() const
{
    for (bsl::list<MockClusterNode>::const_iterator cit = d_nodes.begin();
         cit != d_nodes.end();
         ++cit) {
        if (cit->nodeId() == d_selfNodeId) {
            return &(*cit);  // RETURN
        }
    }

    return 0;
}

inline Cluster::NodesList& MockCluster::nodes()
{
    return d_nodesList;
}

inline const Cluster::NodesList& MockCluster::nodes() const
{
    return d_nodesList;
}

}  // close package namespace
}  // close enterprise namespace

#endif
