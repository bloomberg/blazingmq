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

// VST for an implementation of NegotiationContext
struct NegotiationContext {
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
    /// resetted if no data has been received from this
    /// channel for the 'maxMissedHeartbeat' number of
    /// heartbeat intervals.  When enabled, heartbeat
    /// requests will be sent if no 'regular' data is
    /// being received.
    int d_maxMissedHeartbeat;

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
};

}  // close package namespace
}  // close enterprise namespace

#endif
