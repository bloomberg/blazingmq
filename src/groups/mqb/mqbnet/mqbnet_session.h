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

// mqbnet_session.h                                                   -*-C++-*-
#ifndef INCLUDED_MQBNET_SESSION
#define INCLUDED_MQBNET_SESSION

//@PURPOSE: Provide a protocol for a Session object.
//
//@CLASSES:
//  mqbnet::Session              : protocol for a session object
//  mqbnet::SessionEventProcessor: protocol for a processor of session events
//
//@DESCRIPTION: 'mqbnet::Session' is a protocol for a session used and managed
// by the various components in the 'mqbnet' package.
// 'mqbnet::SessionEventProcessor' is a protocol for a processor of session
// events.

// MQB

// BMQ
#include <bmqp_event.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsls_timeinterval.h>

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

// ===========================
// class SessionEventProcessor
// ===========================

class SessionEventProcessor {
  public:
    // CREATORS

    /// Destructor
    virtual ~SessionEventProcessor();

    // MANIPULATORS

    /// Process the specified `event` received from the remote peer
    /// represented by the optionally specified `source`.  Note that this
    /// method is the entry point for all incoming events coming from the
    /// remote peer.
    virtual void processEvent(const bmqp::Event& event,
                              ClusterNode*       source = 0) = 0;
};

// =============
// class Session
// =============

/// Protocol for a session object.
class Session : public SessionEventProcessor {
  public:
    // TYPES

    /// Signature of a shutdown callback.
    typedef bsl::function<void(void)> ShutdownCb;

    /// Signature of an admin command processed callback, that is used to
    /// pass execution result.  The specified `rc` is a return code, it is
    /// expected to be 0 on success and non-zero on error.  The specified
    /// `results` is expected to contain admin command execution result or
    /// an error message.
    typedef bsl::function<void(int rc, const bsl::string& results)>
        AdminCommandProcessedCb;

    /// Signature of an admin command enqueue callback, that is used to
    /// enqueue admin commands for execution.  The specified `source`
    /// declares origin of the specified `command`, that needs to be
    /// executed.  The execution result (structured, non-structured text or
    /// possible error message) is expected to be passed to the specified
    /// `onProcessed` callback.
    typedef bsl::function<void(const bslstl::StringRef&       source,
                               const bsl::string&             command,
                               const AdminCommandProcessedCb& onProcessed,
                               bool fromReroute)>
        AdminCommandEnqueueCb;

  public:
    // CREATORS

    /// Destructor
    ~Session() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //  (virtual: mqbnet::SessionEventProcessor)

    /// Method invoked when the channel associated with the session is going
    /// down.  The session object will be destroyed once the specified
    /// `handle` goes out of scope. Set `isBrokerShutdown` to true if the
    /// channel is going down from a shutdown and false otherwise.  This
    /// method is executed on the IO thread, so if the session object's
    /// destruction must execute some long synchronous or heavy operation,
    /// it could offload it to a separate thread, passing in the `handle` to
    /// prevent destruction of the session object until the shutdown
    /// sequence completes.
    virtual void tearDown(const bsl::shared_ptr<void>& handle,
                          bool                         isBrokerShutdown) = 0;

    /// Initiate the shutdown of the session and invoke the specified
    /// `callback` upon completion of (asynchronous) shutdown sequence or
    /// if the specified `timeout` is expired.
    virtual void initiateShutdown(const ShutdownCb&         callback,
                                  const bsls::TimeInterval& timeout) = 0;

    /// Make the session abandon any work it has.
    virtual void invalidate() = 0;

    /// Return the channel associated to this session.
    virtual bsl::shared_ptr<mwcio::Channel> channel() const = 0;

    /// Return the clusterNode associated to this session, or 0 if there are
    /// none.
    virtual ClusterNode* clusterNode() const = 0;

    // ACCESSORS
    //  (virtual: mqbnet::SessionEventProcessor)

    /// Return a reference not offering modifiable access to the negotiation
    /// message received from the remote peer of this session during the
    /// negotiation phase.
    virtual const bmqp_ctrlmsg::NegotiationMessage&
    negotiationMessage() const = 0;

    /// Return a printable description of this session (e.g. for logging).
    virtual const bsl::string& description() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
