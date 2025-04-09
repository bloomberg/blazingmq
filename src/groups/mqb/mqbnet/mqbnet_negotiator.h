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

// mqbnet_negotiator.h                                                -*-C++-*-
#ifndef INCLUDED_MQBNET_NEGOTIATOR
#define INCLUDED_MQBNET_NEGOTIATOR

//@PURPOSE: Provide a protocol for a session negotiator.
//
//@CLASSES:
//  mqbnet::Negotiator:        protocol for a session negotiator
//
//@SEE_ALSO:
//  mqbnet::Session: protocol of the negotiated sessions
//
//@DESCRIPTION: 'mqbnet::Negotiator' is a protocol for a session negotiator
// that uses a provided established channel to negotiate and create an
// 'mqbnet::Session' object.  'mqbnet::InitialConnectionHandlerContext' is a
// value-semantic type holding the context associated with a session being
// negotiated.  It allows bi-directional generic communication between the
// application layer and the transport layer: for example, a user data
// information can be passed in at application layer, kept and carried over in
// the transport layer and retrieved in the negotiator concrete implementation.
// Similarly, a 'cookie' can be passed in from application layer, to the result
// callback notification in the transport layer (usefull for 'listen-like'
// established connection where the entry point doesn't allow to bind specific
// user data, which then can be retrieved at application layer during
// negotiation).
//
// Note that the 'InitialConnectionHandlerContext' passed in to the
// 'Negotiator::negotiate()' method can be modified in the negotiator concrete
// implementation to set some of its members that the caller will leverage and
// use.

// MQB
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_initialconnectionhandlercontext.h>

// BDE
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqio {
class Channel;
}

namespace mqbnet {

// FORWARD DECLARATION
class Session;
class SessionEventProcessor;
class Cluster;

// ================
// class Negotiator
// ================

/// Protocol for a session negotiator.
class Negotiator {
  public:
    // CREATORS

    /// Destructor
    virtual ~Negotiator();

    // MANIPULATORS

    /// Create a `session` based on the type of initial connection message in
    /// the specified `context`.
    virtual int createSessionOnMsgType(
        bsl::shared_ptr<mqbnet::Session>*                session,
        const bsl::shared_ptr<InitialConnectionContext>& context) = 0;

    /// Send out outbound negotiation message or reverse connection request
    /// with the specified `context`.
    virtual int negotiateOutboundOrReverse(
        const bsl::shared_ptr<InitialConnectionContext>& context) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
