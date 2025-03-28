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

// mqbnet_authenticator.h -*-C++-*-
#ifndef INCLUDED_MQBNET_AUTHENTICATOR
#define INCLUDED_MQBNET_AUTHENTICATOR

//@PURPOSE:
//
//@CLASSES:
//
//@DESCRIPTION:

// MQB

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

// =======================
// class AuthenticatorContext
// =======================

///
class AuthenticatorContext {
  private:
    // DATA
    bool d_isIncoming;
    // True if the session being authenticated originates
    // from a remote peer (i.e., a 'listen'); false if
    // it originates from us (i.e., a 'connect).

    int d_maxMissedHeartbeat;
    // If non-zero, enable smart-heartbeat and specify
    // that the connection should be proactively
    // resetted if no data has been received from this
    // channel for the 'maxMissedHeartbeat' number of
    // heartbeat intervals.  When enabled, heartbeat
    // requests will be sent if no 'regular' data is
    // being received.

    void* d_userData_p;
    // Raw pointer, held not owned, to some user data
    // the Negotiator concrete implementation can use
    // while negotiating the session.  This may or may
    // not be set by the caller, before invoking
    // 'Negotiator::negotiate()'; and should not be
    // changed during negotiation (this data is not
    // used by the session factory, so changing it will
    // have no effect).  This is used to bind high
    // level data (from application layer) to the
    // application layer (the negotiator concrete
    // implementation) (typically for the case of
    // 'connect' sessions to provide information to use
    // for negotiating the session with the remote
    // peer).

  public:
    // CREATORS

    /// Create a new object having the specified `isIncoming` value.
    AuthenticatorContext(bool isIncoming);

    // MANIPULATORS
    AuthenticatorContext& setMaxMissedHeartbeat(int value);
    AuthenticatorContext& setUserData(void* value);
    AuthenticatorContext& setResultState(void* value);

    // ACCESSORS
    bool  isIncoming() const;
    int   maxMissedHeartbeat() const;
    void* userData() const;
    void* resultState() const;
};

// ================
// class Authenticator
// ================

/// Protocol for an Authenticator
class Authenticator {
  public:
    // TYPES

    ///
    typedef bsl::function<void(int                status,
                               const bsl::string& errorDescription)>
        AuthenticationCb;

  public:
    // CREATORS

    /// Destructor
    virtual ~Authenticator();

    // MANIPULATORS

    /// Method invoked by the client of this object to negotiate a session
    /// using the specified `channel`.  The specified `negotiationCb` must
    /// be called with the result, whether success or failure, of the
    /// negotiation.  The specified `context` is an in-out member holding
    /// the negotiation context to use; and the Negotiator concrete
    /// implementation can modify some of the members during the negotiation
    /// (i.e., between the `negotiate()` method and the invocation of the
    /// `AuthenticationCb` method.  Note that if no negotiation is needed, the
    /// `negotiationCb` may be invoked directly from inside the call to
    /// `negotiate`.
    virtual void authenticate(AuthenticatorContext*                  context,
                              const bsl::shared_ptr<bmqio::Channel>& channel,
                              const AuthenticationCb& authenticationCb) = 0;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class AuthenticatorContext
// -----------------------

inline AuthenticatorContext::AuthenticatorContext(bool isIncoming)
: d_isIncoming(isIncoming)
, d_maxMissedHeartbeat(0)
, d_userData_p(0)
{
    // NOTHING
}

inline AuthenticatorContext&
AuthenticatorContext::setMaxMissedHeartbeat(int value)
{
    d_maxMissedHeartbeat = value;
    return *this;
}

inline AuthenticatorContext& AuthenticatorContext::setUserData(void* value)
{
    d_userData_p = value;
    return *this;
}

inline bool AuthenticatorContext::isIncoming() const
{
    return d_isIncoming;
}

inline int AuthenticatorContext::maxMissedHeartbeat() const
{
    return d_maxMissedHeartbeat;
}

inline void* AuthenticatorContext::userData() const
{
    return d_userData_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
