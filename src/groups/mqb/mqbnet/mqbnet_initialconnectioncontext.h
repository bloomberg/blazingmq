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

// mqbnet_initialconnectioncontext.h                        -*-C++-*-
#ifndef INCLUDED_MQBNET_INITIALCONNECTIONCONTEXT
#define INCLUDED_MQBNET_INITIALCONNECTIONCONTEXT

//@PURPOSE: Provide a context for an initial connection handler.
//
//@CLASSES:
//  mqbnet::InitialConnectionContext: VST for the context associated to
//  an initial connection
//
//@DESCRIPTION: 'InitialConnectionContext' provides the context
// associated to an initial connection being established
//

// BDE
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqio {
class Channel;
};

namespace mqbnet {

// FORWARD DECLARATION
class SessionEventProcessor;
class Cluster;
class Session;
struct NegotiationContext;

// ==============================
// class InitialConnectionContext
// ==============================

/// VST for the context associated to a session being negotiated.  Each
/// session being negotiated get its own context; and the
/// InitialConnectionHandler concrete implementation can modify some of the
/// members during the handleInitialConnection() (i.e., between the
/// `handleInitialConnection()` method and the invocation of the
/// `InitialConnectionCompleteCb` method.
class InitialConnectionContext {
  public:
    typedef bsl::function<void(
        int                                    status,
        const bsl::string&                     errorDescription,
        const bsl::shared_ptr<Session>&        session,
        const bsl::shared_ptr<bmqio::Channel>& channel,
        const InitialConnectionContext*        initialConnectionContext)>
        InitialConnectionCompleteCb;

  private:
    // DATA

    /// True if the session being negotiated originates
    /// from a remote peer (i.e., a 'listen'); false if
    /// it originates from us (i.e., a 'connect).
    bool d_isIncoming;

    /// Raw pointer, held not owned, to some user data
    /// the session factory will pass back to the
    /// 'resultCb' method (used to inform of the
    /// success/failure of a session negotiation).  This
    /// may or may not be set by the caller, before
    /// invoking 'InitialConnectionHandler::handleInitialConnection()';
    /// and may or may not be changed by the negotiator concrete
    /// implementation before invoking the
    /// 'InitialConnectionCompleteCb'.  This is used to bind low level
    /// data (from transport layer) to the session; and
    /// can be overriden/set by the negotiation
    /// implementation (typically for the case of
    /// 'listen' sessions, since those are
    /// 'sporadically' happening and there is not enough
    /// context at the transport layer to find back this
    /// data).
    void* d_resultState_p;

    /// Raw pointer, held not owned, to some user data
    /// the InitialConnectionHandler concrete implementation can use
    /// while negotiating the session.  This may or may
    /// not be set by the caller, before invoking
    /// 'InitialConnectionHandler::handleInitialConnection()';
    /// and should not be changed during negotiation (this data is not
    /// used by the session factory, so changing it will
    /// have no effect).  This is used to bind high
    /// level data (from application layer) to the
    /// application layer (the negotiator concrete
    /// implementation) (typically for the case of
    /// 'connect' sessions to provide information to use
    /// for negotiating the session with the remote
    /// peer).
    void* d_userData_p;

    /// The channel to use for the initial connection.
    bsl::shared_ptr<bmqio::Channel> d_channelSp;

    /// The callback to invoke to notify of the status of the initial
    /// connection.
    InitialConnectionCompleteCb d_initialConnectionCompleteCb;

    /// The NegotiationContext updated upon receiving a negotiation message.
    bsl::shared_ptr<NegotiationContext> d_negotiationCtxSp;

  public:
    // CREATORS

    /// Create a new object having the specified `isIncoming` value.
    InitialConnectionContext(bool isIncoming);

    ~InitialConnectionContext();

    // MANIPULATORS

    /// Set the corresponding field to the specified `value` and return a
    /// reference offering modifiable access to this object.
    InitialConnectionContext& setUserData(void* value);
    InitialConnectionContext& setResultState(void* value);
    InitialConnectionContext&
    setChannel(const bsl::shared_ptr<bmqio::Channel>& value);
    InitialConnectionContext&
    setCompleteCb(const InitialConnectionCompleteCb& value);
    InitialConnectionContext&
    setNegotiationContext(const bsl::shared_ptr<NegotiationContext>& value);

    // ACCESSORS

    /// Return the value of the corresponding field.
    bool                                       isIncoming() const;
    void*                                      userData() const;
    void*                                      resultState() const;
    const bsl::shared_ptr<bmqio::Channel>&     channel() const;
    const bsl::shared_ptr<NegotiationContext>& negotiationContext() const;

    void complete(int                                     rc,
                  const bsl::string&                      error,
                  const bsl::shared_ptr<mqbnet::Session>& session) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
