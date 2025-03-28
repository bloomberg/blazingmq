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

// mqba_authenticator.h                                           -*-C++-*-
#ifndef INCLUDED_MQBA_AUTHENTICATOR
#define INCLUDED_MQBA_AUTHENTICATOR

/// @file mqba_authenticator.h
///
/// @brief
///
///
/// Thread Safety                              {#mqba_authenticator_thread}
/// =============
///
/// The implementation must be thread safe as 'authenticate()' may be called
/// concurrently from many IO threads.

// MQB
#include <mqbconfm_messages.h>
#include <mqbnet_authenticator.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbblp {
class ClusterCatalog;
}
namespace mqbi {
class Dispatcher;
}

namespace mqba {

// =======================
// class Authenticator
// =======================

/// Authenticator for a BlazingMQ session with client or broker
class Authenticator : public mqbnet::Authenticator {
  public:
    // TYPES

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
    struct AuthenticationContext {
        // PUBLIC DATA

        /// The associated authenticatorContext, passed in by the caller.
        mqbnet::AuthenticatorContext* d_authenticatorContext_p;

        /// The channel to use for the authentication.
        bsl::shared_ptr<bmqio::Channel> d_channelSp;

        /// The callback to invoke to notify of the status of the
        /// authentication.
        mqbnet::Authenticator::AuthenticationCb d_authenticationCb;

        /// The negotiation message received from the remote peer.
        bmqp_ctrlmsg::NegotiationMessage d_authenticationMessage;

        /// True if this is a "reversed" connection (on either side of the
        /// connection).
        bool d_isReversed;

        /// The type of the session being negotiated.
        ConnectionType::Enum d_connectionType;
    };

    typedef bsl::shared_ptr<AuthenticationContext> AuthenticationContextSp;

  private:
    // DATA

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    /// Cluster catalog to query for cluster information.
    mqbblp::ClusterCatalog* d_clusterCatalog_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator not implemented.
    Authenticator(const Authenticator&);             // = delete
    Authenticator& operator=(const Authenticator&);  // = delete

  private:
    // PRIVATE MANIPULATORS

    /// Read callback method invoked when receiving data in the specified
    /// `blob`, if the specified `status` indicates success.  The specified
    /// `numNeeded` can be used to indicate if more bytes are needed in
    /// order to get a full message.  The specified `context` holds the
    /// negotiation context associated to this read.
    void readCallback(const bmqio::Status&           status,
                      int*                           numNeeded,
                      bdlbb::Blob*                   blob,
                      const AuthenticationContextSp& context);

    /// Decode the negotiation messages received in the specified `blob` and
    /// store it, on success, in the corresponding member of the specified
    /// `context`, returning 0.  Return a non-zero code on error and
    /// populate the specified `errorDescription` with a description of the
    /// error.
    int decodeNegotiationMessage(bsl::ostream& errorDescription,
                                 const AuthenticationContextSp& context,
                                 const bdlbb::Blob&             blob);

    /// Invoked when received a `ClientIdentity` negotiation message with
    /// the specified `context`.  Creates and return a Session on success,
    /// or return a null pointer and populate the specified
    /// `errorDescription` with a description of the error on failure.
    int onAuthenticationRequest(bsl::ostream& errorDescription,
                                const AuthenticationContextSp& context);

    /// Invoked when received a `BrokerResponse` negotiation message with
    /// the specified `context`.  Creates and return a Session on success,
    /// or return a null pointer and populate the specified
    /// `errorDescription` with a description of the error on failure.
    int onAuthenticationResponse(bsl::ostream& errorDescription,
                                 const AuthenticationContextSp& context);

    /// Send the specified `message` to the peer associated with the
    /// specified `context` and return 0 on success, or return a non-zero
    /// code on error and populate the specified `errorDescription` with a
    /// description of the error.
    int
    sendAuthenticationMessage(bsl::ostream& errorDescription,
                              const bmqp_ctrlmsg::NegotiationMessage& message,
                              const AuthenticationContextSp&          context);

    /// Initiate an outbound negotiation (i.e., send out some negotiation
    /// message and schedule a read of the response) using the specified
    /// `context`.
    /// Senario: reverse connection
    void
    initiateOutboundAuthentication(const AuthenticationContextSp& context);

    /// Schedule a read for the negotiation of the session of the specified
    /// `context`.
    void scheduleRead(const AuthenticationContextSp& context);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Authenticator, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `Authenticator` using the specified
    /// `bufferFactory`, `dispatcher`, `statContext`, `scheduler` and
    /// `blobSpPool` to inject in the negotiated sessions.  Use the
    /// specified `allocator` for all memory allocations.
    Authenticator(bslma::Allocator* allocator);

    /// Destructor
    ~Authenticator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Set the cluster catalog to use to the specified `value` and return a
    /// reference offering modifiable access to this object.
    Authenticator& setClusterCatalog(mqbblp::ClusterCatalog* value);

    // MANIPULATORS
    //   (virtual: mqbnet::Authenticator)

    /// Negotiate the connection on the specified `channel` associated with
    /// the specified negotiation `context` and invoke the specified
    /// `negotiationCb` once the negotiation is complete (either success or
    /// failure).  Note that if no negotiation are needed, the
    /// `negotiationCb` may be invoked directly from inside the call to
    /// `negotiate`.
    void authenticate(mqbnet::AuthenticatorContext*          context,
                      const bsl::shared_ptr<bmqio::Channel>& channel,
                      const mqbnet::Authenticator::AuthenticationCb&
                          authenticationCb) BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class Authenticator
// -----------------------

inline Authenticator&
Authenticator::setClusterCatalog(mqbblp::ClusterCatalog* value)
{
    d_clusterCatalog_p = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
