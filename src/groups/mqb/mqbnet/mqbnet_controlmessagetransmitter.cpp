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

// mqbnet_controlmessagetransmitter.cpp                               -*-C++-*-
#include <mqbnet_controlmessagetransmitter.h>

#include <mqbscm_version.h>
// MQB
#include <mqbi_cluster.h>
#include <mqbnet_cluster.h>
#include <mqbnet_session.h>

#include <bmqio_status.h>

// BDE
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbnet {

// -------------------------------
// class ControlMessageTransmitter
// -------------------------------

// PRIVATE MANIPULATORS
void ControlMessageTransmitter::sendMessageHelper(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                destination,
    bmqp::SchemaEventBuilder*           schemaBuilder)
{
    // executed by *ANY* thread

    int rc = schemaBuilder->setMessage(message, bmqp::EventType::e_CONTROL);
    if (0 != rc) {
        BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                       << "Failed to encode schema message destined to "
                       << "cluster node [ " << destination->nodeDescription()
                       << " ], rc: " << rc << ", message: " << message;
        return;  // RETURN
    }

    bmqt::GenericResult::Enum writeRc =
        destination->write(schemaBuilder->blob(), bmqp::EventType::e_CONTROL);
    if (bmqt::GenericResult::e_SUCCESS != writeRc) {
        BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                       << "Failed to write schema message to cluster node [ "
                       << destination->nodeDescription()
                       << " ], rc: " << writeRc
                       << ", length: " << schemaBuilder->blob()->length()
                       << ", message: " << message;
    }
    else {
        BALL_LOG_INFO << "Sent message to cluster node [ "
                      << destination->nodeDescription()
                      << " ], message: " << message;
    }
}

void ControlMessageTransmitter::broadcastMessageHelper(
    const bmqp_ctrlmsg::ControlMessage& message,
    bmqp::SchemaEventBuilder*           schemaBuilder,
    mqbnet::TransportManager*           transportManager)
{
    // executed by *ANY* thread

    int rc = schemaBuilder->setMessage(message, bmqp::EventType::e_CONTROL);
    if (0 != rc) {
        BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                       << "Failed to encode cluster schema message "
                       << "destined to entire cluster, rc: " << rc
                       << ", message: " << message;
        return;  // RETURN
    }

    // Broadcast to cluster, using the unicast channel to ensure ordering of
    // events.

    d_cluster_p->writeAll(schemaBuilder->blob(), bmqp::EventType::e_CONTROL);

    BALL_LOG_INFO << "Broadcasted message to all cluster nodes, message: "
                  << message;

    if (!transportManager) {
        return;  // RETURN
    }

    bsl::vector<bsl::weak_ptr<mqbnet::Session> > sessions;
    for (mqbnet::TransportManagerIterator sessIt(transportManager); sessIt;
         ++sessIt) {
        sessions.push_back(sessIt.session());
    }

    for (unsigned int i = 0; i < sessions.size(); ++i) {
        bsl::shared_ptr<mqbnet::Session> sessionSp = sessions[i].lock();
        if (!sessionSp) {
            continue;  // CONTINUE
        }

        const bmqp_ctrlmsg::NegotiationMessage& negoMsg =
            sessionSp->negotiationMessage();
        if (mqbnet::ClusterUtil::isProxy(sessionSp->negotiationMessage(),
                                         d_cluster_p->name()) &&
            bmqp::ProtocolUtil::hasFeature(
                bmqp::HighAvailabilityFeatures::k_FIELD_NAME,
                bmqp::HighAvailabilityFeatures::k_BROADCAST_TO_PROXIES,
                negoMsg.clientIdentity().features())) {
            bmqio::Status status;
            sessionSp->channel()->write(&status, *schemaBuilder->blob());
            if (status.category() == bmqio::StatusCategory::e_SUCCESS) {
                BALL_LOG_INFO << "Sent message to proxy [ "
                              << sessionSp->description()
                              << " ], message: " << message;
            }
            else {
                BALL_LOG_ERROR
                    << "#CLUSTER_SEND_FAILURE "
                    << "Failed to write schema message to proxy [ "
                    << sessionSp->description() << " ], status: " << status
                    << ", length: " << schemaBuilder->blob()->length()
                    << ", message: " << message;
            }
        }
    }
}

// CREATORS
ControlMessageTransmitter::ControlMessageTransmitter(
    BlobSpPool*       blobSpPool_p,
    mqbnet::Cluster*  cluster,
    bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_blobSpPool_p(blobSpPool_p)
, d_schemaBuilder(d_blobSpPool_p, bmqp::EncodingType::e_BER, allocator)
, d_cluster_p(cluster)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_allocator_p);
    BSLS_ASSERT_SAFE(d_blobSpPool_p);
    BSLS_ASSERT_SAFE(d_cluster_p);
}

ControlMessageTransmitter::~ControlMessageTransmitter()
{
    // NOTHING
}

void ControlMessageTransmitter::sendMessage(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                destination)
{
    d_schemaBuilder.reset();
    sendMessageHelper(message, destination, &d_schemaBuilder);
}

void ControlMessageTransmitter::sendMessage(
    const bmqp_ctrlmsg::ControlMessage&    message,
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const bsl::string&                     description)
{
    d_schemaBuilder.reset();

    int rc = d_schemaBuilder.setMessage(message, bmqp::EventType::e_CONTROL);
    if (0 != rc) {
        BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                       << "Failed to encode schema message "
                       << "destined to session [ " << description
                       << " ], rc: " << rc << ", message: " << message;
        return;  // RETURN
    }

    bmqio::Status status;
    channel->write(&status, *d_schemaBuilder.blob());
    if (status.category() != bmqio::StatusCategory::e_SUCCESS) {
        BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                       << "Failed to write schema message to session [ "
                       << description << " ], status: " << status
                       << ", length: " << d_schemaBuilder.blob()->length()
                       << ", message: " << message;
    }
    else {
        BALL_LOG_INFO << "Sent message to session [ " << description
                      << " ], message: " << message;
    }
}

// MANIPULATORS
void ControlMessageTransmitter::sendMessageSafe(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                destination)
{
    // executed by *ANY* thread

    // Since this method can be invoked from any thread, schema event builder
    // is created on the stack, instead of using 'd_schemaBuilder'.

    bmqp::SchemaEventBuilder schemaBuilder(d_blobSpPool_p,
                                           bmqp::EncodingType::e_BER,
                                           d_allocator_p);
    sendMessageHelper(message, destination, &schemaBuilder);
}

void ControlMessageTransmitter::broadcastMessage(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::TransportManager*           transportManager)
{
    d_schemaBuilder.reset();
    broadcastMessageHelper(message, &d_schemaBuilder, transportManager);
}

}  // close package namespace
}  // close enterprise namespace
