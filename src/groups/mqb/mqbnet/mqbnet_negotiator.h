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
//  mqbnet::NegotiatorContext: VST for the context associated to a negotiation
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
// class NegotiatorContext
// =======================

/// VST for the context associated to a session being negotiated.  Each
/// session being negotiated get its own context; and the Negotiator
/// concrete implementation can modify some of the members during the
/// negotiation (i.e., between the `negotiate()` method and the invocation
/// of the `NegotiationCb` method.
class NegotiatorContext {
  private:
    // DATA
    bool d_isIncoming;
    // True if the session being negotiated originates
    // from a remote peer (i.e., a 'listen'); false if
    // it originates from us (i.e., a 'connect).

    char d_maxMissedHeartbeat;
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
    NegotiatorContext(bool isIncoming);

    // MANIPULATORS
    NegotiatorContext& setMaxMissedHeartbeat(char value);
    NegotiatorContext& setUserData(void* value);
    NegotiatorContext& setResultState(void* value);
    NegotiatorContext& setEventProcessor(SessionEventProcessor* value);

    /// Set the corresponding field to the specified `value` and return a
    /// reference offering modifiable access to this object.
    NegotiatorContext& setCluster(Cluster* cluster);

    // ACCESSORS
    bool     isIncoming() const;
    Cluster* cluster() const;
    char     maxMissedHeartbeat() const;
    void*    userData() const;
    void*    resultState() const;

    /// Return the value of the corresponding field.
    SessionEventProcessor* eventProcessor() const;
};

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

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class NegotiatorContext
// -----------------------

inline NegotiatorContext::NegotiatorContext(bool isIncoming)
: d_isIncoming(isIncoming)
, d_maxMissedHeartbeat(0)
, d_eventProcessor_p(0)
, d_resultState_p(0)
, d_userData_p(0)
, d_cluster_p(0)
{
    // NOTHING
}

inline NegotiatorContext& NegotiatorContext::setMaxMissedHeartbeat(char value)
{
    d_maxMissedHeartbeat = value;
    return *this;
}

inline NegotiatorContext& NegotiatorContext::setUserData(void* value)
{
    d_userData_p = value;
    return *this;
}

inline NegotiatorContext& NegotiatorContext::setResultState(void* value)
{
    d_resultState_p = value;
    return *this;
}

inline NegotiatorContext&
NegotiatorContext::setEventProcessor(SessionEventProcessor* value)
{
    d_eventProcessor_p = value;
    return *this;
}

inline NegotiatorContext& NegotiatorContext::setCluster(Cluster* cluster)
{
    d_cluster_p = cluster;
    return *this;
}

inline bool NegotiatorContext::isIncoming() const
{
    return d_isIncoming;
}

inline Cluster* NegotiatorContext::cluster() const
{
    return d_cluster_p;
}

inline char NegotiatorContext::maxMissedHeartbeat() const
{
    return d_maxMissedHeartbeat;
}

inline void* NegotiatorContext::userData() const
{
    return d_userData_p;
}

inline void* NegotiatorContext::resultState() const
{
    return d_resultState_p;
}

inline SessionEventProcessor* NegotiatorContext::eventProcessor() const
{
    return d_eventProcessor_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
