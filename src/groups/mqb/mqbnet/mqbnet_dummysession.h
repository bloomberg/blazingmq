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

// mqbnet_dummysession.h                                              -*-C++-*-
#ifndef INCLUDED_MQBNET_DUMMYSESSION
#define INCLUDED_MQBNET_DUMMYSESSION

//@PURPOSE: Provide a dummy implementation of the 'mqbnet::Session' protocol.
//
//@CLASSES:
//  mqbnet::DummySession : dummy 'mqbnet::Session' implementation
//
//@DESCRIPTION: The channel and session factory framework requires creation of
// a 'mqbnet::Session' object that is associated to the created
// 'mwcio::Channel'.  This session's lifecycle is tied to the one of the
// channel and usually is in charge of processing the reads.  When creating a
// cluster, we actually want to have a higher level object
// ('mqbnet::ClusterNode') that will be associated to a channel, but which
// lifecycle will not be strongly linked to an underlying channel: rather a
// channel will dynamically get associated / de-associated to it.  However, the
// framework still requires a 'session' object to be created, so
// 'mqbnet::DummySession' provides such a mechanism.  It basically is simply a
// component implementing the 'mqbnet::Session' protocol, and holding on to the
// associated channel.

// MQB

#include <mqbnet_session.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqp_ctrlmsg {
class NegotiationMessage;
}
namespace mwcio {
class Channel;
}

namespace mqbnet {

// FORWARD DECLARATION
class ClusterNode;

// ==================
// class DummySession
// ==================

/// dummy `mqbnet::Session` implementation
class DummySession : public Session {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.DUMMYSESSION");

  private:
    // DATA
    bsl::shared_ptr<mwcio::Channel> d_channel_sp;
    // Channel associated to this session.

    bmqp_ctrlmsg::NegotiationMessage d_negotiationMessage;
    // Negotiation message received from the
    // remote peer.

    ClusterNode* d_clusterNode_p;
    // ClusterNode associated to this
    // session (if any)

    bsl::string d_description;
    // Description of this session.

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    DummySession(const DummySession& other,
                 bslma::Allocator*   allocator = 0);       // = delete
    DummySession& operator=(const DummySession& other);  // = delete

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DummySession, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object associated with the specified `channel`,
    /// `negotiationMessage`, `clusterNode` and having the specified
    /// `description`.  Use the specified `allocator` for any memory
    /// allocation.
    DummySession(const bsl::shared_ptr<mwcio::Channel>&  channel,
                 const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
                 ClusterNode*                            clusterNode,
                 const bsl::string&                      description,
                 bslma::Allocator*                       allocator);

    /// Destructor
    ~DummySession() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::Session)

    /// Method invoked when the channel associated with the session is going
    /// down.  The session object will be destroyed once the specified
    /// `handle` goes out of scope. Set `isBrokerShutdown` to true if the
    /// channel is going down from a shutdown and false otherwise.  This
    /// method is executed on the IO thread, so if the session object's
    /// destruction must execute some long synchronous or heavy operation,
    /// it could offload it to a separate thread, passing in the `handle` to
    /// prevent destruction of the session object until the shutdown
    /// sequence completes.
    void tearDown(const bsl::shared_ptr<void>& handle,
                  bool isBrokerShutdown) BSLS_KEYWORD_OVERRIDE;

    /// Initiate the shutdown of the session and invoke the specified
    /// `callback` upon completion of (asynchronous) shutdown sequence or
    /// if the specified `timeout` is expired.  If the optional (temporary)
    /// specified 'suppportShutdownV2' is 'true' execute shutdown logic V2
    /// where upstream (not downstream) nodes deconfigure  queues and the
    /// shutting down node (not downstream) waits for CONFIRMS.
    void
    initiateShutdown(const ShutdownCb&         callback,
                     const bsls::TimeInterval& timeout,
                     bool suppportShutdownV2 = false) BSLS_KEYWORD_OVERRIDE;

    /// Make the session abandon any work it has.
    void invalidate() BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `event` received from the remote peer
    /// represented by the optionally specified `source`.  Note that this
    /// method is the entry point for all incoming events coming from the
    /// remote peer.
    void processEvent(const bmqp::Event&   event,
                      mqbnet::ClusterNode* source = 0) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::Session)

    /// Return the channel associated to this session.
    bsl::shared_ptr<mwcio::Channel> channel() const BSLS_KEYWORD_OVERRIDE;

    /// Return the clusterNode associated to this session, or 0 if there are
    /// none.
    ClusterNode* clusterNode() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbnet::Session)
    const bmqp_ctrlmsg::NegotiationMessage&
    negotiationMessage() const BSLS_KEYWORD_OVERRIDE;
    // Return a reference not offering modifiable access to the negotiation
    // message received from the remote peer of this session during the
    // negotiation phase.

    /// Return a printable description of this session (e.g. for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// class DummySession
// ------------------

inline bsl::shared_ptr<mwcio::Channel> DummySession::channel() const
{
    return d_channel_sp;
}

inline ClusterNode* DummySession::clusterNode() const
{
    return d_clusterNode_p;
}

inline const bmqp_ctrlmsg::NegotiationMessage&
DummySession::negotiationMessage() const
{
    return d_negotiationMessage;
}

inline const bsl::string& DummySession::description() const
{
    return d_description;
}

}  // close package namespace
}  // close enterprise namespace

#endif
