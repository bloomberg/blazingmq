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

// mqbnet_authenticationcontext.h                                -*-C++-*-
#ifndef INCLUDED_MQBNET_AUTHENTICATIONCONTEXT
#define INCLUDED_MQBNET_AUTHENTICATIONCONTEXT

/// @file mqbnet_authenticationcontext.h
///
/// @brief Provide the context for initial connection handler for establishing
/// sessions.
///

// MQB
#include <mqbnet_initialconnectioncontext.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

namespace BloombergLP {
namespace mqbnet {

struct ConnectionType {
    // Enum representing the type of session being authenticated, from that
    // side of the connection's point of view.
    enum Enum {
        e_UNKNOWN,
        e_CLUSTER_PROXY,   // Reverse connection proxy -> broker
        e_CLUSTER_MEMBER,  // Cluster node -> cluster node
        e_CLIENT,          // Either SDK or Proxy -> Proxy or cluster node
        e_ADMIN
    };
};

// ===========================
// class AuthenticationContext
// ===========================

// VST for an implementation of AuthenticationContext
struct AuthenticationContext {
    // DATA
    /// The associated InitialConnectionContext passed in by the caller.
    /// Held, not owned
    InitialConnectionContext* d_initialConnectionContext_p;

    /// The authentication message received from the remote peer.
    bmqp_ctrlmsg::AuthenticationMessage d_authenticationMessage;

    /// The cluster involved in the session being authenticated,
    /// or empty if none.
    bsl::string d_clusterName;

    /// True if this is a "reversed" connection (on either side of the
    /// connection).
    bool d_isReversed;

    /// The type of the session being authenticated.
    ConnectionType::Enum d_connectionType;
};

}  // close package namespace
}  // close enterprise namespace

#endif
