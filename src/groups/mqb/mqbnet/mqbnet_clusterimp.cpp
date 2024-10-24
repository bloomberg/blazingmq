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

// mqbnet_clusterimp.cpp                                              -*-C++-*-
#include <mqbnet_clusterimp.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqp_protocol.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslmt_lockguard.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mqbnet {

// --------------------
// class ClusterNodeImp
// --------------------

ClusterNodeImp::ClusterNodeImp(ClusterImp*                cluster,
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

    bmqu::MemOutStream osstr;
    osstr << "[" << hostName() << ", " << nodeId() << "]";
    d_description.assign(osstr.str().data(), osstr.str().length());
}

ClusterNodeImp::~ClusterNodeImp()
{
    // NOTHING: Needed due to inheritance
}

ClusterNode*
ClusterNodeImp::setChannel(const bsl::weak_ptr<bmqio::Channel>& value,
                           const bmqp_ctrlmsg::ClientIdentity&  identity,
                           const bmqio::Channel::ReadCallback&  readCb)
{
    // Save the value
    d_readCb    = readCb;
    d_isReading = false;
    d_identity  = identity;

    d_channel.setChannel(value);

    // Notify the cluster of changes to this node
    d_cluster_p->notifyObserversOfNodeStateChange(this, true);

    return this;
}

bool ClusterNodeImp::enableRead()
{
    const bsl::shared_ptr<bmqio::Channel> channelSp = d_channel.channel();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!channelSp)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_readCb);

    if (d_isReading) {
        return true;  // RETURN
    }

    bmqio::Status readStatus;
    channelSp->read(&readStatus, bmqp::Protocol::k_PACKET_MIN_SIZE, d_readCb);

    if (!readStatus) {
        BALL_LOG_ERROR << "#TCP_READ_ERROR " << nodeDescription()
                       << ": Failed reading from the channel "
                       << "[status: " << readStatus << ", "
                       << "channel: '" << channelSp->peerUri() << "']";

        channelSp->close();

        return false;  // RETURN
    }

    d_isReading = true;

    BALL_LOG_INFO << nodeDescription() << ": reading from the channel '"
                  << channelSp->peerUri() << "'";

    return true;
}

ClusterNode* ClusterNodeImp::resetChannel()
{
    d_channel.resetChannel();
    d_isReading = false;
    d_identity.reset();
    d_readCb = bmqio::Channel::ReadCallback();

    // Notify the cluster of changes to this node
    d_cluster_p->notifyObserversOfNodeStateChange(this, false);

    // If we want to apply application logic to pending items before clearing
    // the buffer, it can be done in the observer call.

    return this;
}

void ClusterNodeImp::closeChannel()
{
    d_channel.closeChannel();
}

bmqt::GenericResult::Enum ClusterNodeImp::write(const bdlbb::Blob&    blob,
                                                bmqp::EventType::Enum type)
{
    return d_channel.writeBlob(blob, type);
}

// ----------------
// class ClusterImp
// ----------------

void ClusterImp::notifyObserversOfNodeStateChange(ClusterNode* node,
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

ClusterImp::ClusterImp(const bsl::string&                      name,
                       const bsl::vector<mqbcfg::ClusterNode>& nodesConfig,
                       int                                     selfNodeId,
                       bdlbb::BlobBufferFactory* blobBufferFactory,
                       Channel::ItemPool*        itemPool,
                       bslma::Allocator*         allocator)
: d_allocator_p(allocator)
, d_name(name, allocator)
, d_nodesConfig(nodesConfig, allocator)
, d_selfNodeId(selfNodeId)
, d_selfNode(0)  // set below
, d_nodes(allocator)
, d_nodesList(allocator)
, d_nodeStateObservers(allocator)
, d_failedWritesThrottler()
, d_mutex()
, d_isReadEnabled(false)
{
    // Create the nodes
    bsl::vector<mqbcfg::ClusterNode>::const_iterator nodeIt;
    for (nodeIt = d_nodesConfig.begin(); nodeIt != d_nodesConfig.end();
         ++nodeIt) {
        d_nodes.emplace_back(this, *nodeIt, blobBufferFactory, itemPool);
        d_nodesList.emplace_back(&d_nodes.back());
        if (nodeIt->id() == selfNodeId) {
            d_selfNode = d_nodesList.back();
        }
    }

    // 1 log per 1 second interval
    d_failedWritesThrottler.initialize(
        1,
        bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND);
}

ClusterImp::~ClusterImp()
{
    // NOTHING: Needed due to inheritance
}

Cluster* ClusterImp::registerObserver(ClusterObserver* observer)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_nodeStateObservers.insert(observer);

    return this;
}

Cluster* ClusterImp::unregisterObserver(ClusterObserver* observer)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_nodeStateObservers.erase(observer);

    return this;
}

int ClusterImp::writeAll(const bdlbb::Blob& blob, bmqp::EventType::Enum type)
{
    unsigned int maxPushChannelPendingItems = 0;
    unsigned int maxChannelPendingItems     = 0;
    for (bsl::list<ClusterNodeImp>::iterator it = d_nodes.begin();
         it != d_nodes.end();
         ++it) {
        // Write to all peers (except self)
        if (it->nodeId() != selfNodeId()) {
            unsigned int numItems = it->channel().numItems();
            if (numItems > maxPushChannelPendingItems) {
                // Ignore replicas to which we do not send PUSH data
                // Note that PUSH data get written after Storage
                if (it->channel().numItems(bmqp::EventType::e_PUSH)) {
                    maxPushChannelPendingItems = numItems;
                }
                else if (numItems > maxChannelPendingItems) {
                    maxChannelPendingItems = numItems;
                }
            }

            bmqt::GenericResult::Enum rc = it->write(blob, type);

            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                    bmqt::GenericResult::e_SUCCESS != rc &&
                    bmqt::GenericResult::e_NOT_CONNECTED != rc)) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

                if (d_failedWritesThrottler.requestPermission()) {
                    BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                                   << "Failed to write blob of length ["
                                   << blob.length() << "] bytes, to node "
                                   << it->nodeDescription() << ", rc: " << rc
                                   << ".";
                }
            }
        }
    }

    if (0 == maxPushChannelPendingItems) {
        maxPushChannelPendingItems = maxChannelPendingItems;
    }

    return maxPushChannelPendingItems;
}

int ClusterImp::broadcast(const bdlbb::Blob& blob)
{
    return writeAll(blob, bmqp::EventType::e_STORAGE);
}

void ClusterImp::closeChannels()
{
    for (bsl::list<ClusterNodeImp>::iterator it = d_nodes.begin();
         it != d_nodes.end();
         ++it) {
        it->closeChannel();
    }
}

void ClusterImp::enableRead()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_isReadEnabled);

    d_isReadEnabled = true;

    for (bsl::list<ClusterNodeImp>::iterator it = d_nodes.begin();
         it != d_nodes.end();
         ++it) {
        if (it->nodeId() != d_selfNodeId) {
            it->enableRead();
        }
    }
}

void ClusterImp::onProxyConnectionUp(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const bmqp_ctrlmsg::ClientIdentity&    identity,
    const bsl::string&                     description)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    // NOTE: Mutex is outside the objects iteration, which means it's not safe
    //       to call some method of this component from within the observers
    //       callback.  If required, this can be changed by making a copy of
    //       the set inside the mutex, and iterating over the copy after
    //       releasing the mutex.
    for (ObserversSet::iterator it = d_nodeStateObservers.begin();
         it != d_nodeStateObservers.end();
         ++it) {
        (*it)->onProxyConnectionUp(channel, identity, description);
    }
}

}  // close package namespace
}  // close enterprise namespace
