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

// mqba_initialconnectionhandler.h                          -*-C++-*-
#ifndef INCLUDED_MQBA_INITIALCONNECTIONHANDLER
#define INCLUDED_MQBA_INITIALCONNECTIONHANDLER

// MQB
#include <mqbnet_authenticator.h>
#include <mqbnet_initialconnectionhandler.h>
#include <mqbnet_negotiator.h>

// BMQ
#include <bmqio_status.h>
#include <bmqma_countingallocatorstore.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlmt_fixedthreadpool.h>

namespace BloombergLP {

namespace mqba {

// ==================
// class InitialConnectionHandler
// ==================

class InitialConnectionHandler : public mqbnet::InitialConnectionHandler {
  private:
    // PRIVATE TYPES
    struct ConnectionType {
        // Enum representing the type of session being negotiated, from that
        // side of the connection's point of view.
        enum Enum {
            e_UNKNOWN,
            e_CLUSTER_PROXY,   // Reverse connection proxy -> broker
            e_CLUSTER_MEMBER,  // Cluster node -> cluster node
            e_CLIENT,          // Either SDK or Proxy -> Proxy or cluster node
            e_ADMIN
        };
    };
    /// Struct used to hold the context associated to a session being
    /// negotiated
    struct InitialConnectionContext {
        // PUBLIC DATA

        /// The associated negotiatorContext, passed in by the caller.
        mqbnet::InitialConnectionHandlerContext* d_handlerContext_p;

        /// The channel to use for the negotiation.
        bsl::shared_ptr<bmqio::Channel> d_channelSp;

        /// The callback to invoke to notify of the status of the negotiation.
        // TODO: should this type
        mqbnet::InitialConnectionHandler::InitialConnectionCb
            d_initialConnectionCb;

        /// The initial connection message received from the remote peer.
        bmqp_ctrlmsg::NegotiationMessage d_initialConnectionMessage;

        /// The cluster involved in the session being negotiated, or empty if
        /// none.
        bsl::string d_clusterName;

        /// True if this is a "reversed" connection (on either side of the
        /// connection).
        bool d_isReversed;

        /// The type of the session being negotiated.
        ConnectionType::Enum d_connectionType;
    };

    typedef bsl::shared_ptr<InitialConnectionContext>
        InitialConnectionContextSp;

  private:
    // DATA

    /// Not own.
    mqbnet::Authenticator* d_authenticator_p;
    mqbnet::Negotiator*    d_negotiator_p;

    bdlmt::FixedThreadPool d_threadPool;

    /// Allocator store to spawn new allocators for sub components.
    bmqma::CountingAllocatorStore d_allocators;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator not implemented.
    InitialConnectionHandler(const InitialConnectionHandler&);  // = delete
    InitialConnectionHandler&
    operator=(const InitialConnectionHandler&);  // = delete

  private:
    // PRIVATE MANIPULATORS
    void readCallback(const bmqio::Status&              status,
                      int*                              numNeeded,
                      bdlbb::Blob*                      blob,
                      const InitialConnectionContextSp& context);

    int decodeNegotiationMessage(bsl::ostream& errorDescription,
                                 const InitialConnectionContextSp& context,
                                 bdlbb::Blob&                      blob);

    void scheduleRead(const InitialConnectionContextSp& context);

  public:
    // CREATORS

    InitialConnectionHandler(mqbnet::Authenticator* authenticator,
                             mqbnet::Negotiator*    negotiator,
                             bslma::Allocator*      allocator);

    /// Destructor
    ~InitialConnectionHandler() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    void initialConnect(mqbnet::InitialConnectionHandlerContext* context,
                        const bsl::shared_ptr<bmqio::Channel>&   channel,
                        const InitialConnectionCb& initialConnectionCb)
        BSLS_KEYWORD_OVERRIDE;
};
}
}

#endif
