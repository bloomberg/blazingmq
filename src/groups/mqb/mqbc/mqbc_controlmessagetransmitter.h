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

// mqbc_controlmessagetransmitter.h                                   -*-C++-*-
#ifndef INCLUDED_MQBC_CONTROLMESSAGETRANSMITTER
#define INCLUDED_MQBC_CONTROLMESSAGETRANSMITTER

//@PURPOSE: Provide a mechanism to transmit control messages to peer nodes.
//
//@CLASSES:
//  mqbc::ControlMessageTransmitter: Transmitter of messages to peer nodes.
//
//@DESCRIPTION: 'mqbc::ControlMessageTransmitter' provides a mechanism to
// transmit messages to peer nodes in the same cluster.
//
/// Thread Safety
///-------------
// The 'mqbc::ControlMessageTransmitter' object is not thread safe and should
// always be manipulated from the associated cluster's dispatcher thread,
// unless specified by the function (such as 'sendMessageSafe').

// MQB

#include <mqbnet_transportmanager.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_schemaeventbuilder.h>

#include <bmqio_channel.h>

// BDE
#include <ball_log.h>
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
}

namespace mqbc {

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
    bslma::Allocator* d_allocator_p;
    // Allocator to use.

    /// Blob pool to use.  Held, not owned.
    BlobSpPool* d_blobSpPool_p;

    bmqp::SchemaEventBuilder d_schemaBuilder;
    // Schema event builder to use.  Must be used
    // only in cluster dispatcher thread.

    mqbi::Cluster* d_cluster_p;
    // Associated cluster.

    mqbnet::TransportManager* d_transportManager_p;

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
    /// nodes in the cluster.  If the specified `broadcastToProxies` is
    /// true, send the `message` to all proxies connected to this cluster.
    ///
    /// THREAD: This method can be invoked from any thread.
    void broadcastMessageHelper(const bmqp_ctrlmsg::ControlMessage& message,
                                bmqp::SchemaEventBuilder* schemaBuilder,
                                bool                      broadcastToProxies);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ControlMessageTransmitter,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance of `ControlMessageTransmitter` associated with
    /// the specified `cluster` and using the specified `blobSpPool_p`.
    /// Use the specified `allocator` for memory allocations.
    ControlMessageTransmitter(BlobSpPool*               blobSpPool_p,
                              mqbi::Cluster*            cluster,
                              mqbnet::TransportManager* transportManager,
                              bslma::Allocator*         allocator);

    /// Destructor
    ~ControlMessageTransmitter();

    // MANIPULATORS

    /// Encode and send the specified schema `message` to the specified
    /// peer `destination` or the specified `channel`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void sendMessage(const bmqp_ctrlmsg::ControlMessage& message,
                     mqbnet::ClusterNode*                destination);
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
    /// in the cluster.  If the optionally specified `broadcastToProxies` is
    /// true, send the `message` to all proxies connected to this cluster.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void broadcastMessage(const bmqp_ctrlmsg::ControlMessage& message,
                          bool broadcastToProxies = false);

    /// Encode and send the specified schema `message` to the all peer nodes
    /// in the cluster.
    ///
    /// THREAD: This method can be invoked from any thread.
    void broadcastMessageSafe(const bmqp_ctrlmsg::ControlMessage& message);
};

}  // close package namespace
}  // close enterprise namespace

#endif
