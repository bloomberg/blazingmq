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

// mqbnet_mockcluster.cpp                                             -*-C++-*-
#include <mqbnet_mockcluster.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqp_protocol.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslmt_lockguard.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mqbnet {

// ---------------------
// class MockClusterNode
// ---------------------

MockClusterNode::MockClusterNode(MockCluster*               cluster,
                                 const mqbcfg::ClusterNode& config,
                                 bdlbb::BlobBufferFactory*  blobBufferFactory,
                                 Channel::ItemPool*         itemPool,
                                 bslma::Allocator*          allocator)
: d_cluster_p(cluster)
, d_config(config, allocator)
, d_description(allocator)
, d_channel(blobBufferFactory, itemPool, config.name(), allocator)
, d_identity(allocator)
, d_isReading(false)
{
    BSLS_ASSERT_SAFE(d_cluster_p &&
                     "A ClusterNode should always be part of a Cluster");

    mwcu::MemOutStream osstr;
    osstr << "[" << hostName() << ", " << nodeId() << "]";
    d_description.assign(osstr.str().data(), osstr.str().length());
}

MockClusterNode::~MockClusterNode()
{
    // NOTHING: Needed due to inheritance
}

ClusterNode*
MockClusterNode::setChannel(const bsl::weak_ptr<mwcio::Channel>& value,
                            const bmqp_ctrlmsg::ClientIdentity&  identity,
                            const mwcio::Channel::ReadCallback&  readCb)
{
    // Save the value
    d_channel.setChannel(value);
    d_identity = identity;
    d_readCb   = readCb;

    // Notify the cluster of changes to this node
    d_cluster_p->notifyObserversOfNodeStateChange(this, true);

    return this;
}

bool MockClusterNode::enableRead()
{
    const bsl::shared_ptr<mwcio::Channel> channelSp = d_channel.channel();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!channelSp)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    if (d_isReading) {
        return true;  // RETURN
    }

    mwcio::Status readStatus;
    channelSp->read(&readStatus, bmqp::Protocol::k_PACKET_MIN_SIZE, d_readCb);

    if (!readStatus) {
        channelSp->close();
        return false;  // RETURN
    }

    d_isReading = true;

    return true;
}

ClusterNode* MockClusterNode::resetChannel()
{
    d_channel.resetChannel();

    // Notify the cluster of changes to this node
    d_cluster_p->notifyObserversOfNodeStateChange(this, false);

    return this;
}

void MockClusterNode::closeChannel()
{
    d_channel.closeChannel();
}

bmqt::GenericResult::Enum
MockClusterNode::write(const bsl::shared_ptr<bdlbb::Blob>& blob,
                       bmqp::EventType::Enum               type)
{
    return d_channel.writeBlob(blob, type);
}

// -----------------
// class MockCluster
// -----------------

void MockCluster::notifyObserversOfNodeStateChange(ClusterNode* node,
                                                   bool         state)
{
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        // NOTE: Mutex is outside the objects iteration, which means it's not
        //       safe to call some method of this component from within the
        //       observers callback.  If required, this can be changed by
        //       making a copy of the set inside the mutex, and iterating over
        //       the copy after releasing the mutex.
        for (ObserversSet::iterator it = d_nodeStateObservers.begin();
             it != d_nodeStateObservers.end();
             ++it) {
            (*it)->onNodeStateChange(node, state);
        }
    }

    if (d_isReadEnabled && state) {
        BSLS_ASSERT_SAFE(node);

        node->enableRead();
    }
}

MockCluster::MockCluster(const mqbcfg::ClusterDefinition& config,
                         bdlbb::BlobBufferFactory*        blobBufferFactory,
                         Channel::ItemPool*               itemPool,
                         bslma::Allocator*                allocator)
: d_allocator_p(allocator)
, d_config(config, allocator)
, d_nodes(allocator)
, d_nodesList(allocator)
, d_selfNodeId(-1)
, d_nodeStateObservers(allocator)
, d_isReadEnabled(false)
, d_disableBroadcast(false)
{
    // Create the nodes
    bsl::vector<mqbcfg::ClusterNode>::const_iterator nodeIt;
    for (nodeIt = d_config.nodes().begin(); nodeIt != d_config.nodes().end();
         ++nodeIt) {
        d_nodes.emplace_back(this, *nodeIt, blobBufferFactory, itemPool);
        d_nodesList.emplace_back(&d_nodes.back());
    }
}

MockCluster::~MockCluster()
{
    // NOTHING: Needed due to inheritance
}

Cluster* MockCluster::registerObserver(ClusterObserver* observer)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_nodeStateObservers.insert(observer);

    return this;
}

Cluster* MockCluster::unregisterObserver(ClusterObserver* observer)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_nodeStateObservers.erase(observer);

    return this;
}

int MockCluster::writeAll(const bsl::shared_ptr<bdlbb::Blob>& blob,
                          bmqp::EventType::Enum               type)
{
    for (bsl::list<MockClusterNode>::iterator it = d_nodes.begin();
         it != d_nodes.end();
         ++it) {
        // Write to all peers (except self)
        if (it->nodeId() != selfNodeId()) {
            it->write(blob, type);
        }
    }

    return 0;
}

int MockCluster::broadcast(const bsl::shared_ptr<bdlbb::Blob>& blob)
{
    if (d_disableBroadcast) {
        return 0;  // RETURN
    }

    // No multicast support in mock cluster, just forward to the individual
    // nodes.
    return writeAll(blob, bmqp::EventType::e_STORAGE);
}

void MockCluster::closeChannels()
{
    for (bsl::list<MockClusterNode>::iterator it = d_nodes.begin();
         it != d_nodes.end();
         ++it) {
        it->closeChannel();
    }
}

ClusterNode* MockCluster::lookupNode(int nodeId)
{
    for (bsl::list<MockClusterNode>::iterator it = d_nodes.begin();
         it != d_nodes.end();
         ++it) {
        if (it->nodeId() == nodeId) {
            return &(*it);  // RETURN
        }
    }

    return 0;
}

void MockCluster::enableRead()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_isReadEnabled);

    d_isReadEnabled = true;
    for (bsl::list<MockClusterNode>::iterator it = d_nodes.begin();
         it != d_nodes.end();
         ++it) {
        if (it->nodeId() != d_selfNodeId) {
            it->enableRead();
        }
    }
}

void MockCluster::onProxyConnectionUp(
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<mwcio::Channel>& channel,
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ClientIdentity& identity,
    BSLS_ANNOTATION_UNUSED const bsl::string& description)
{
    // NOTHING
}

MockCluster& MockCluster::_setSelfNodeId(int value)
{
    d_selfNodeId = value;
    return *this;
}

MockCluster& MockCluster::_setDisableBroadcast(bool value)
{
    d_disableBroadcast = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace
