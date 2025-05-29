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
/// @brief Provide an authenticator for authenticating a connection.
///
/// @bbref{mqba::Authenticator} implements the @bbref{mqbnet::Authenticator}
/// interface to authenticate a connection with a BlazingMQ client or another
/// bmqbrkr.  From a @bbref{bmqio::Channel}, it will exchange authentication
/// message and authenticate depending on the authentication message received.
///
/// Thread Safety                              {#mqba_authenticator_thread}
/// =============
/// This component is owned by `InitialConnectionHandler`, and its functions
/// are called only from there.  It is not thread safe.

// MQB
#include <mqbconfm_messages.h>
#include <mqbnet_authenticationcontext.h>
#include <mqbnet_authenticator.h>
#include <mqbnet_initialconnectioncontext.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_sharedobjectpool.h>
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

// ===================
// class Authenticator
// ===================

/// Authenticator for a BlazingMQ session with client or broker
class Authenticator : public mqbnet::Authenticator {
  public:
    // TYPES

    /// Type of a pool of shared pointers to blob
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

    typedef mqbnet::AuthenticationContext::State State;

  private:
    typedef bsl::shared_ptr<mqbnet::AuthenticationContext>
        AuthenticationContextSp;

    typedef bsl::shared_ptr<mqbnet::InitialConnectionContext>
        InitialConnectionContextSp;

  private:
    // DATA

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    BlobSpPool* d_blobSpPool_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator not implemented.
    Authenticator(const Authenticator&);             // = delete
    Authenticator& operator=(const Authenticator&);  // = delete

  private:
    // PRIVATE MANIPULATORS

    /// Handle an incoming `AuthenticationRequest` message by authenticating
    /// using the specified `AuthenticationMessage` and `context`.  On success,
    /// create an `AuthenticationContext` and stores it in `context`.  The
    /// behavior of this function is undefined unless `authenticationMsg` is an
    /// `AuthenticationRequest` and this is an incoming connection.
    /// Return 0 on success; otherwise, return a non-zero error code and
    /// populate `errorDescription` with details of the failure.
    int onAuthenticationRequest(
        bsl::ostream&                              errorDescription,
        const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg,
        const InitialConnectionContextSp&          context);

    /// Handle an incoming `AuthenticationResponse` message by authenticating
    /// using the specified `AuthenticationMessage` and `context`.  On success,
    /// create an `AuthenticationContext` and stores it in `context`. The
    /// behavior of this function is undefined unless `authenticationMsg` is an
    /// `AuthenticationResponse`.
    /// Return 0 on success; otherwise, return a non-zero error code and
    /// populate `errorDescription` with details of the failure.
    int onAuthenticationResponse(
        bsl::ostream&                              errorDescription,
        const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg,
        const InitialConnectionContextSp&          context);

    /// Send the specified `message` to the peer associated with the
    /// specified `context` and return 0 on success, or return a non-zero
    /// code on error and populate the specified `errorDescription` with a
    /// description of the error.
    int sendAuthenticationMessage(
        bsl::ostream&                              errorDescription,
        const bmqp_ctrlmsg::AuthenticationMessage& message,
        const AuthenticationContextSp&             context);

    /// Initiate an outbound authentication (i.e., send out some authentication
    /// message and schedule a read of the response) using the specified
    /// `context`.
    /// Senario: reverse connection
    void
    initiateOutboundAuthentication(const AuthenticationContextSp& context);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Authenticator, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `Authenticator` using the specified
    /// `bufferFactory`, `dispatcher`, `statContext`, `scheduler` and
    /// `blobSpPool` to inject in the negotiated sessions.  Use the
    /// specified `allocator` for all memory allocations.
    Authenticator(BlobSpPool* blobSpPool, bslma::Allocator* allocator);

    /// Destructor
    ~Authenticator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::Authenticator)

    /// Authenticate the connection using the specified `authenticationMsg`
    /// and `context`.  An `AuthenticationContext` will be created and stored
    /// into `context`.  Set `isContinueRead` to true if further reading
    /// should continue, or false if authentication is complete.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    int handleAuthentication(bsl::ostream& errorDescription,
                             bool*         isContinueRead,
                             const InitialConnectionContextSp& context,
                             const bmqp_ctrlmsg::AuthenticationMessage&
                                 authenticationMsg) BSLS_KEYWORD_OVERRIDE;

    /// Send out outbound authentication message or reverse connection request
    /// with the specified `context`.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    int authenticationOutboundOrReverse(const AuthenticationContextSp& context)
        BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
