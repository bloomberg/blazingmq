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

// mqbnet_initialconnectionhandler.h                            -*-C++-*-
#ifndef INCLUDED_MQBNET_INITIALCONNECTIONHANDLER
#define INCLUDED_MQBNET_INITIALCONNECTIONHANDLER

//@PURPOSE:
//
//@CLASSES:
//
//@DESCRIPTION: Read from IO and commands authenticator and negotiator.
// A session would be created at the end upon success.

// MQB
#include <mqbnet_negotiator.h>

// BMQ

// BDE
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>

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

// =====================================
// class InitialConnectionHandlerContext
// =====================================

/// VST for the context associated to a session being negotiated.  Each
/// session being negotiated get its own context; and the Negotiator
/// concrete implementation can modify some of the members during the
/// negotiation (i.e., between the `negotiate()` method and the invocation
/// of the `NegotiationCb` method.
class InitialConnectionHandlerContext {
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
    // 'Negotiator::negotiate()'; and may or may not be
    // changed by the negotiator concrete
    // implementation before invoking the
    // 'NegotiationCb'.  Note that a value of 0 will
    // use the negotiated session as the default event
    // processor.

    void* d_resultState_p;
    // Raw pointer, held not owned, to some user data
    // the session factory will pass back to the
    // 'resultCb' method (used to inform of the
    // success/failure of a session negotiation).  This
    // may or may not be set by the caller, before
    // invoking 'Negotiator::negotiate()'; and may or
    // may not be changed by the negotiator concrete
    // implementation before invoking the
    // 'NegotiationCb'.  This is used to bind low level
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

    Cluster* d_cluster_p;
    // mqbnet::Cluster to inform about incoming (proxy)
    // connection

  public:
    // CREATORS

    /// Create a new object having the specified `isIncoming` value.
    InitialConnectionHandlerContext(bool isIncoming);

    // MANIPULATORS
    InitialConnectionHandlerContext& setMaxMissedHeartbeat(int value);
    InitialConnectionHandlerContext& setUserData(void* value);
    InitialConnectionHandlerContext& setResultState(void* value);
    InitialConnectionHandlerContext&
    setEventProcessor(SessionEventProcessor* value);

    /// Set the corresponding field to the specified `value` and return a
    /// reference offering modifiable access to this object.
    InitialConnectionHandlerContext& setCluster(Cluster* cluster);

    // ACCESSORS
    bool     isIncoming() const;
    Cluster* cluster() const;
    int      maxMissedHeartbeat() const;
    void*    userData() const;
    void*    resultState() const;

    /// Return the value of the corresponding field.
    SessionEventProcessor* eventProcessor() const;
};

// ==============================
// class InitialConnectionHandler
// ==============================

class InitialConnectionHandler {
  public:
    // TYPES
    typedef bsl::function<void(int                status,
                               const bsl::string& errorDescription,
                               const bsl::shared_ptr<Session>& session)>
        InitialConnectionCb;

  public:
    // CREATORS

    /// Destructor
    virtual ~InitialConnectionHandler();

    // MANIPULATORS

    virtual void
    initialConnect(mqbnet::InitialConnectionHandlerContext* context,
                   const bsl::shared_ptr<bmqio::Channel>&   channel,
                   const InitialConnectionCb& initialConnectionCb) = 0;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class InitialConnectionHandlerContext
// -----------------------

inline InitialConnectionHandlerContext::InitialConnectionHandlerContext(
    bool isIncoming)
: d_isIncoming(isIncoming)
, d_maxMissedHeartbeat(0)
, d_eventProcessor_p(0)
, d_resultState_p(0)
, d_userData_p(0)
, d_cluster_p(0)
{
    // NOTHING
}

inline InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setMaxMissedHeartbeat(int value)
{
    d_maxMissedHeartbeat = value;
    return *this;
}

inline InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setUserData(void* value)
{
    d_userData_p = value;
    return *this;
}

inline InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setResultState(void* value)
{
    d_resultState_p = value;
    return *this;
}

inline InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setEventProcessor(
    SessionEventProcessor* value)
{
    d_eventProcessor_p = value;
    return *this;
}

inline InitialConnectionHandlerContext&
InitialConnectionHandlerContext::setCluster(Cluster* cluster)
{
    d_cluster_p = cluster;
    return *this;
}

inline bool InitialConnectionHandlerContext::isIncoming() const
{
    return d_isIncoming;
}

inline Cluster* InitialConnectionHandlerContext::cluster() const
{
    return d_cluster_p;
}

inline int InitialConnectionHandlerContext::maxMissedHeartbeat() const
{
    return d_maxMissedHeartbeat;
}

inline void* InitialConnectionHandlerContext::userData() const
{
    return d_userData_p;
}

inline void* InitialConnectionHandlerContext::resultState() const
{
    return d_resultState_p;
}

inline SessionEventProcessor*
InitialConnectionHandlerContext::eventProcessor() const
{
    return d_eventProcessor_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
