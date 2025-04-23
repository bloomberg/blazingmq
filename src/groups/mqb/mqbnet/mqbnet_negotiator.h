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
// 'mqbnet::Session' object.  'mqbnet::NegotiatorContext' is a value-semantic
// type holding the context associated with a session being negotiated.  It
// allows bi-directional generic communication between the application layer
// and the transport layer: for example, a user data information can be passed
// in at application layer, kept and carried over in the transport layer and
// retrieved in the negotiator concrete implementation.  Similarly, a 'cookie'
// can be passed in from application layer, to the result callback notification
// in the transport layer (usefull for 'listen-like' established connection
// where the entry point doesn't allow to bind specific user data, which then
// can be retrieved at application layer during negotiation).
//
// Note that the 'NegotiatorContext' passed in to the 'Negotiator::negotiate()'
// method can be modified in the negotiator concrete implementation to set some
// of its members that the caller will leverage and use.

// MQB
#include <mqbnet_negotiatorcontext.h>

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
    // TYPES

    /// Signature of the callback method to invoke when the negotiation is
    /// complete, either successfully or with failure.  If the specified
    /// `status` is 0, the negotiation was a success and the specified
    /// `session` contains the resulting session; otherwise a non-zero
    /// `status` value indicates that the negotiation failed, and the
    /// specified `errorDescription` can contain a description of the error.
    /// Note that the `session` should no longer be used once the
    /// negotiation callback has been invoked.
    typedef bsl::function<void(int                status,
                               const bsl::string& errorDescription,
                               const bsl::shared_ptr<Session>& session)>
        NegotiationCb;

  public:
    // CREATORS

    /// Destructor
    virtual ~Negotiator();

    // MANIPULATORS

    /// Method invoked by the client of this object to negotiate a session
    /// using the specified `channel`.  The specified `negotiationCb` must
    /// be called with the result, whether success or failure, of the
    /// negotiation.  The specified `context` is an in-out member holding
    /// the negotiation context to use; and the Negotiator concrete
    /// implementation can modify some of the members during the negotiation
    /// (i.e., between the `negotiate()` method and the invocation of the
    /// `NegotiationCb` method.  Note that if no negotiation is needed, the
    /// `negotiationCb` may be invoked directly from inside the call to
    /// `negotiate`.
    virtual void negotiate(NegotiatorContext*                     context,
                           const bsl::shared_ptr<bmqio::Channel>& channel,
                           const NegotiationCb& negotiationCb) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
