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

// mqba_negotiationcontext.h                                    -*-C++-*-
#ifndef INCLUDED_MQBA_NEGOTIATIONCONTEXT
#define INCLUDED_MQBA_NEGOTIATIONCONTEXT

/// @file mqba_negotiationcontext.h
///
/// @brief Provide the context for negotiator for establishing sessions.
///

// MQB
#include <mqbnet_negotiator.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

namespace BloombergLP {
namespace mqba {

struct ConnectionType {
    // Enum representing the type of session being negotiated, from that
    // side of the connection's point of view.
    enum Enum {
        e_UNKNOWN,
        e_CLUSTER_PROXY,   // Reverse connection proxy -> broker
        e_CLUSTER_MEMBER,  // Cluster node -> cluster node
        e_CLIENT,          // Either SDK or Proxy -> Proxy or cluster node
        e_ADMIN
    };
};

// ========================
// class NegotiationContext
// ========================

// VST for an implementation of NegotiatiorContext
struct NegotiationContext {
    // DATA
    /// The associated negotiatorContext, passed in by the caller.
    mqbnet::NegotiatorContext* d_negotiatorContext_p;

    /// The channel to use for the negotiation.
    bsl::shared_ptr<bmqio::Channel> d_channelSp;

    /// The callback to invoke to notify of the status of the negotiation.
    mqbnet::Negotiator::NegotiationCb d_negotiationCb;

    /// The negotiation message received from the remote peer.
    bmqp_ctrlmsg::NegotiationMessage d_negotiationMessage;

    /// The cluster involved in the session being negotiated, or empty if
    /// none.
    bsl::string d_clusterName;

    /// True if this is a "reversed" connection (on either side of the
    /// connection).
    bool d_isReversed;

    /// The type of the session being negotiated.
    ConnectionType::Enum d_connectionType;
};

}  // close package namespace
}  // close enterprise namespace

#endif
