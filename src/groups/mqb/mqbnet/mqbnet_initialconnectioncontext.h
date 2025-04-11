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
#include <bslstl_sharedptr.h>

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
    typedef bsl::function<void(int                status,
                               const bsl::string& errorDescription,
                               const bsl::shared_ptr<Session>& session)>
        InitialConnectionCompleteCb;

  private:
    // DATA
    bool d_isIncoming;
    // True if the session being negotiated originates
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

    SessionEventProcessor* d_eventProcessor_p;
    // The event processor to use for initiating the
    // read on the channel once the session has been
    // successfully negotiated.  This may or may not be
    // set by the caller, before invoking
    // 'Negotiator::handleInitialConnection()'; and may or may not be
    // changed by the negotiator concrete
    // implementation before invoking the
    // 'InitialConnectionCompleteCb'.  Note that a value of 0 will
    // use the negotiated session as the default event
    // processor.

    void* d_resultState_p;
    // Raw pointer, held not owned, to some user data
    // the session factory will pass back to the
    // 'resultCb' method (used to inform of the
    // success/failure of a session negotiation).  This
    // may or may not be set by the caller, before
    // invoking 'Negotiator::handleInitialConnection()'; and may or
    // may not be changed by the negotiator concrete
    // implementation before invoking the
    // 'InitialConnectionCompleteCb'.  This is used to bind low level
    // data (from transport layer) to the session; and
    // can be overriden/set by the negotiation
    // implementation (typically for the case of
    // 'listen' sessions, since those are
    // 'sporadically' happening and there is not enough
    // context at the transport layer to find back this
    // data).

    void* d_userData_p;
    // Raw pointer, held not owned, to some user data
    // the Negotiator concrete implementation can use
    // while negotiating the session.  This may or may
    // not be set by the caller, before invoking
    // 'Negotiator::handleInitialConnection()'; and should not be
    // changed during negotiation (this data is not
    // used by the session factory, so changing it will
    // have no effect).  This is used to bind high
    // level data (from application layer) to the
    // application layer (the negotiator concrete
    // implementation) (typically for the case of
    // 'connect' sessions to provide information to use
    // for negotiating the session with the remote
    // peer).

    Cluster* d_cluster_p;
    // mqbnet::Cluster to inform about incoming (proxy)
    // connection

    /// The channel to use for the initial connection.
    bsl::shared_ptr<bmqio::Channel> d_channelSp;

    /// The callback to invoke to notify of the status of the initial
    /// connection.
    InitialConnectionCompleteCb d_initialConnectionCompleteCb;

    bsl::shared_ptr<NegotiationContext> d_negotiationCtxSp;

  public:
    // CREATORS

    /// Create a new object having the specified `isIncoming` value.
    InitialConnectionContext(bool isIncoming);

    // MANIPULATORS

    /// Set the corresponding field to the specified `value` and return a
    /// reference offering modifiable access to this object.
    InitialConnectionContext& setMaxMissedHeartbeat(int value);
    InitialConnectionContext& setUserData(void* value);
    InitialConnectionContext& setResultState(void* value);
    InitialConnectionContext& setEventProcessor(SessionEventProcessor* value);
    InitialConnectionContext& setCluster(Cluster* cluster);
    InitialConnectionContext&
    setChannel(const bsl::shared_ptr<bmqio::Channel>& value);
    InitialConnectionContext&
    setInitialConnectionCompleteCb(const InitialConnectionCompleteCb& value);

    // ACCESSORS
    bool                                   isIncoming() const;
    Cluster*                               cluster() const;
    int                                    maxMissedHeartbeat() const;
    void*                                  userData() const;
    void*                                  resultState() const;
    const bsl::shared_ptr<bmqio::Channel>& channel() const;
    const InitialConnectionCompleteCb&     initialConnectionCompleteCb() const;

    /// Return the value of the corresponding field.
    SessionEventProcessor* eventProcessor() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
