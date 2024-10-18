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

// mqbc_controlmessagetransmitter.cpp                                 -*-C++-*-
#include <mqbc_controlmessagetransmitter.h>

#include <mqbscm_version.h>
// MQB
#include <mqbi_cluster.h>
#include <mqbnet_cluster.h>
#include <mqbnet_session.h>

// MWC
#include <mwcio_status.h>

// BDE
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbc {

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
                       << "Failed to encode schema message: " << message
                       << " destined to cluster node "
                       << destination->nodeDescription() << ", rc: " << rc;
        return;  // RETURN
    }

    bmqt::GenericResult::Enum writeRc = destination->write(
        schemaBuilder->blob_sp(),
        bmqp::EventType::e_CONTROL);
    if (bmqt::GenericResult::e_SUCCESS != writeRc) {
        BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                       << "Failed to write schema message: " << message
                       << " to cluster node " << destination->nodeDescription()
                       << ", rc: " << writeRc;
    }
    else {
        BALL_LOG_INFO << "Sent message '" << message << "' to cluster node "
                      << destination->nodeDescription();
    }
}

void ControlMessageTransmitter::broadcastMessageHelper(
    const bmqp_ctrlmsg::ControlMessage& message,
    bmqp::SchemaEventBuilder*           schemaBuilder,
    bool                                broadcastToProxies)
{
    // executed by *ANY* thread

    int rc = schemaBuilder->setMessage(message, bmqp::EventType::e_CONTROL);
    if (0 != rc) {
        BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                       << "Failed to encode cluster schema message " << message
                       << " destined to entire cluster, rc: " << rc;
        return;  // RETURN
    }

    // Broadcast to cluster, using the unicast channel to ensure ordering of
    // events.

    d_cluster_p->netCluster().writeAll(schemaBuilder->blob_sp(),
                                       bmqp::EventType::e_CONTROL);

    BALL_LOG_INFO << "Broadcasted message '" << message
                  << "' to all cluster nodes";

    if (!broadcastToProxies) {
        return;  // RETURN
    }

    bsl::vector<bsl::weak_ptr<mqbnet::Session> > sessions;
    for (mqbnet::TransportManagerIterator sessIt(d_transportManager_p); sessIt;
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
            mwcio::Status status;
            sessionSp->channel()->write(&status, schemaBuilder->blob());
            if (status.category() == mwcio::StatusCategory::e_SUCCESS) {
                BALL_LOG_INFO << "Sent message '" << message << "' to proxy "
                              << sessionSp->description();
            }
            else {
                BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                               << "Failed to write schema message: " << message
                               << " to proxy " << sessionSp->description()
                               << ", status: " << status;
            }
        }
    }
}

// CREATORS
ControlMessageTransmitter::ControlMessageTransmitter(
    bdlbb::BlobBufferFactory* bufferFactory,
    mqbi::Cluster*            cluster,
    mqbnet::TransportManager* transportManager,
    bslma::Allocator*         allocator)
: d_allocator_p(allocator)
, d_bufferFactory_p(bufferFactory)
, d_schemaBuilder(bufferFactory, allocator)
, d_cluster_p(cluster)
, d_transportManager_p(transportManager)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_allocator_p);
    BSLS_ASSERT_SAFE(d_bufferFactory_p);
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
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    d_schemaBuilder.reset();
    sendMessageHelper(message, destination, &d_schemaBuilder);
}

void ControlMessageTransmitter::sendMessage(
    const bmqp_ctrlmsg::ControlMessage&    message,
    const bsl::shared_ptr<mwcio::Channel>& channel,
    const bsl::string&                     description)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    d_schemaBuilder.reset();

    int rc = d_schemaBuilder.setMessage(message, bmqp::EventType::e_CONTROL);
    if (0 != rc) {
        BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                       << "Failed to encode schema message: " << message
                       << " destined to cluster node " << description
                       << ", rc: " << rc;
        return;  // RETURN
    }

    mwcio::Status status;
    channel->write(&status, d_schemaBuilder.blob());
    if (status.category() != mwcio::StatusCategory::e_SUCCESS) {
        BALL_LOG_ERROR << "#CLUSTER_SEND_FAILURE "
                       << "Failed to write schema message: " << message
                       << " to session " << description
                       << ", status: " << status;
    }
    else {
        BALL_LOG_INFO << "Sent message '" << message << "' to session "
                      << description;
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

    bmqp::SchemaEventBuilder schemaBuilder(d_bufferFactory_p, d_allocator_p);
    sendMessageHelper(message, destination, &schemaBuilder);
}

void ControlMessageTransmitter::broadcastMessage(
    const bmqp_ctrlmsg::ControlMessage& message,
    bool                                broadcastToProxies)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    d_schemaBuilder.reset();
    broadcastMessageHelper(message, &d_schemaBuilder, broadcastToProxies);
}

void ControlMessageTransmitter::broadcastMessageSafe(
    const bmqp_ctrlmsg::ControlMessage& message)
{
    // executed by *ANY* thread

    // Since this method can be invoked from any thread, schema event builder
    // is created on the stack, instead of using 'd_schemaBuilder'.

    bmqp::SchemaEventBuilder schemaBuilder(d_bufferFactory_p, d_allocator_p);
    broadcastMessageHelper(message, &schemaBuilder, false);
}

}  // close package namespace
}  // close enterprise namespace
