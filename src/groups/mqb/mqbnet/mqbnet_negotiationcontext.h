// Copyright 2025 Bloomberg Finance L.P.
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

// mqbnet_negotiationcontext.h                                -*-C++-*-
#ifndef INCLUDED_MQBNET_NEGOTIATIONCONTEXT
#define INCLUDED_MQBNET_NEGOTIATIONCONTEXT

/// @file mqbnet_negotiationcontext.h
///
/// @brief Provide the context for negotiating and establishing sessions.
///

// MQB
#include <mqbnet_initialconnectioncontext.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

namespace BloombergLP {
namespace mqbnet {

// ========================
// class NegotiationContext
// ========================

// VST for the context associated with a session being negotiated.
class NegotiationContext {
  private:
    // DATA
    /// The associated InitialConnectionContext passed in by the caller.
    /// Held, not owned
    InitialConnectionContext* d_initialConnectionContext_p;

    /// The negotiation message received from the remote peer.
    bmqp_ctrlmsg::NegotiationMessage d_negotiationMessage;

    /// The cluster involved in the session being negotiated, or empty if
    /// none.
    bsl::string d_clusterName;

    /// The type of the session being negotiated.
    ConnectionType::Enum d_connectionType;

    /// If non-zero, enable smart-heartbeat and specify
    /// that the connection should be proactively
    /// reset if no data has been received from this
    /// channel for the 'maxMissedHeartbeats' number of
    /// heartbeat intervals.  When enabled, heartbeat
    /// requests will be sent if no 'regular' data is
    /// being received.
    int d_maxMissedHeartbeats;

    /// The event processor to use for initiating the
    /// read on the channel once the session has been
    /// successfully negotiated.  This may or may not be
    /// set by the caller, before invoking
    /// 'InitialConnectionHandler::handleInitialConnection()';
    /// and may or may not be changed by the negotiator concrete
    /// implementation before invoking the
    /// 'InitialConnectionCompleteCb'.  Note that a value of 0 will
    /// use the negotiated session as the default event
    /// processor.
    SessionEventProcessor* d_eventProcessor_p;

    /// mqbnet::Cluster to inform about incoming (proxy) connection
    Cluster* d_cluster_p;

    // NOT IMPLEMENTED
    NegotiationContext(const NegotiationContext&);             // = delete;
    NegotiationContext& operator=(const NegotiationContext&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(NegotiationContext,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    NegotiationContext(
        InitialConnectionContext*               initialConnectionContext,
        const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
        const bsl::string&                      clusterName,
        ConnectionType::Enum                    connectionType,
        int                                     maxMissedHeartbeat,
        SessionEventProcessor*                  eventProcessor,
        Cluster*                                cluster,
        bslma::Allocator*                       allocator = 0);

    // MANIPULATORS
    NegotiationContext&
    setInitialConnectionContext(InitialConnectionContext* value);
    NegotiationContext&
    setNegotiationMessage(const bmqp_ctrlmsg::NegotiationMessage& value);
    NegotiationContext& setClusterName(const bsl::string& value);
    NegotiationContext& setConnectionType(ConnectionType::Enum value);
    NegotiationContext& setMaxMissedHeartbeats(int value);
    NegotiationContext& setEventProcessor(SessionEventProcessor* value);
    NegotiationContext& setCluster(Cluster* value);

    // ACCESSORS
    InitialConnectionContext*               initialConnectionContext() const;
    const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage() const;
    const bsl::string&                      clusterName() const;
    ConnectionType::Enum                    connectionType() const;
    int                                     maxMissedHeartbeats() const;
    SessionEventProcessor*                  eventProcessor() const;
    Cluster*                                cluster() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
