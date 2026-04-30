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

#ifndef INCLUDED_MQBNET_CONTROLMESSAGETRANSMITTER
#define INCLUDED_MQBNET_CONTROLMESSAGETRANSMITTER

/// @file mqbnet_controlmessagetransmitter.h
///
/// @brief Provide a mechanism to transmit control messages to peer nodes.
///
/// @bbref{mqbnet::ControlMessageTransmitter} provides a mechanism to transmit
/// messages to peer nodes in the same cluster.
///
/// Thread Safety                    {#mqbnet_controlmessagetransmitter_thread}
/// =============
///
/// The @bbref{mqbnet::ControlMessageTransmitter} object is not thread safe and
/// should always be manipulated from the associated cluster's dispatcher
/// thread, unless specified by the function (such as `sendMessageSafe`).

// MQB
#include <mqbnet_transportmanager.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_schemaeventbuilder.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlbb {
class BlobBufferFactory;
}
namespace mqbi {
class Cluster;
}
namespace mqbnet {
class ClusterNode;

// ===============================
// class ControlMessageTransmitter
// ===============================

/// This class provides a mechanism to transmit messages to peer nodes in
/// the same cluster.
class ControlMessageTransmitter {
  public:
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.ControlMessageTransmitter");

    // DATA

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    /// Blob pool to use.  Held, not owned.
    BlobSpPool* d_blobSpPool_p;

    /// Schema event builder to use.  Must be used only in cluster dispatcher
    /// thread.
    bmqp::SchemaEventBuilder d_schemaBuilder;

    /// Associated cluster.
    mqbnet::Cluster* d_cluster_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operators are not implemented.
    ControlMessageTransmitter(const ControlMessageTransmitter&);
    ControlMessageTransmitter& operator=(const ControlMessageTransmitter&);

  private:
    // PRIVATE MANIPULATORS

    /// Use the specified `schemaBuilder` to build a BlazingMQ schema event
    /// for the specified `message`, then encode and send it to the
    /// specified peer `destination`.
    ///
    /// THREAD: This method can be invoked from any thread.
    void sendMessageHelper(const bmqp_ctrlmsg::ControlMessage& message,
                           mqbnet::ClusterNode*                destination,
                           bmqp::SchemaEventBuilder*           schemaBuilder);

    /// Use the specified `schemaBuilder` to build a BlazingMQ schema event
    /// for the specified `message`, then encode and send it to all peer
    /// nodes in the cluster.  If the specified `transportManager` is not `0`,
    /// send the `message` to all proxies known to the `transportManager`.
    ///
    /// THREAD: This method can be invoked from any thread.
    void broadcastMessageHelper(const bmqp_ctrlmsg::ControlMessage& message,
                                bmqp::SchemaEventBuilder* schemaBuilder,
                                mqbnet::TransportManager* transportManager);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ControlMessageTransmitter,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance of `ControlMessageTransmitter` associated with
    /// the specified `cluster` and using the specified `blobSpPool_p`.
    /// Use the specified `allocator` for memory allocations.
    ControlMessageTransmitter(BlobSpPool*       blobSpPool_p,
                              mqbnet::Cluster*  cluster,
                              bslma::Allocator* allocator);

    /// Destructor
    ~ControlMessageTransmitter();

    // MANIPULATORS

    /// Encode and send the specified schema `message` to the specified
    /// peer `destination`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void sendMessage(const bmqp_ctrlmsg::ControlMessage& message,
                     mqbnet::ClusterNode*                destination);

    /// Encode and send the specified schema `message` to the specified
    /// `channel` with the specified `description`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void sendMessage(const bmqp_ctrlmsg::ControlMessage&    message,
                     const bsl::shared_ptr<bmqio::Channel>& channel,
                     const bsl::string&                     description);

    /// Encode and send the specified schema `message` to the specified
    /// peer `destination`.
    ///
    /// THREAD: This method can be invoked from any thread.
    void sendMessageSafe(const bmqp_ctrlmsg::ControlMessage& message,
                         mqbnet::ClusterNode*                destination);

    /// Encode and send the specified schema `message` to the all peer nodes
    /// in the cluster.  If the specified `transportManager` is not `0`,
    /// send the `message` to all proxies known to the `transportManager`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void broadcastMessage(const bmqp_ctrlmsg::ControlMessage& message,
                          mqbnet::TransportManager* transportManager = 0);
};

}  // close package namespace
}  // close enterprise namespace

#endif
