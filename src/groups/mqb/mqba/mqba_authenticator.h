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
/// This component is *NOT* thread safe.

// MQB
#include <mqbauthn_authenticationcontroller.h>
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
#include <bdlmt_eventscheduler.h>
#include <bdlmt_threadpool.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string_view.h>
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

    /// True if this component is started.
    bool d_isStarted;

    /// Authentication Controller.
    mqbauthn::AuthenticationController* d_authnController_p;

    /// Thread pool to run authentication and reauthentication tasks.
    bdlmt::ThreadPool d_threadPool;

    /// Used to track the duration of a valid authenticated connection.
    /// If reauthentication does not occur within the specified time,
    /// an event is triggered to close the channel.
    bdlmt::EventScheduler d_scheduler;

    BlobSpPool* d_blobSpPool_p;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

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
        const bsl::shared_ptr<bmqio::Channel>&     channel,
        bmqp::EncodingType::Enum                   authenticationEncodingType);

    /// Schedule an authentication job in the thread pool using the
    /// specified `context` and `channel`.  Return 0 on success, or a
    /// non-zero error code and populate the specified `errorDescription`
    /// with a description of the error otherwise.
    int authenticateAsync(bsl::ostream&                     errorDescription,
                          const InitialConnectionContextSp& context,
                          const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Schedule a re-authentication job in the thread pool using the
    /// specified `context` and `channel`.  Return 0 on success, or a
    /// non-zero error code and populate the specified `errorDescription`
    /// with a description of the error otherwise.
    int reauthenticateAsync(bsl::ostream&                  errorDescription,
                            const AuthenticationContextSp& context,
                            const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Authenticate the connection using the `AuthenticationMessage` stored in
    /// `context`.  If authentication fails, invoke
    /// `initialConnectionCompleteCb` to close the `channel`. Also, update the
    /// state of `context` as appropriate. Return 0 on success, or a
    /// non-zero error code otherwise.
    void authenticate(const InitialConnectionContextSp&      context,
                      const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Reauthenticate the connection using the `AuthenticationMessage`
    /// stored in `context`.  If re-authentication fails, invoke
    /// `initialConnectionCompleteCb` to close the `channel`. Also, update the
    /// state of `context` as appropriate.
    void reauthenticate(const AuthenticationContextSp&         context,
                        const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Close the specified `channel` with an error code and name
    /// indicating the re-authentication error or authentication timeout.
    void onReauthenticateErrorOrTimeout(
        const int                              errorCode,
        const bsl::string&                     errorName,
        const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Process the authentication request in `request` and store the
    /// result in `response`.  Return 0 on success, or a non-zero error
    /// code and populate `errorDescription` with a description of the error
    /// otherwise.
    int processAuthentication(
        bsl::ostream&                            errorDescription,
        bmqp_ctrlmsg::AuthenticateResponse*      response,
        const bmqp_ctrlmsg::AuthenticateRequest& request,
        const bsl::shared_ptr<bmqio::Channel>&   channel,
        const AuthenticationContextSp&           authenticationContext);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Authenticator, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `Authenticator` using the specified `authnController` and
    /// `blobSpPool`. Use the specified `allocator` for all memory allocations.
    Authenticator(mqbauthn::AuthenticationController* authnController,
                  BlobSpPool*                         blobSpPool,
                  bslma::Allocator*                   allocator);

    /// Destructor
    ~Authenticator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::Authenticator)

    /// Start the authenticator.  Return 0 on success, or a non-zero error
    /// code and populate the specified `errorDescription` with a description
    /// of the error otherwise.
    /// This method will block until the thread pool is started.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop the authenticator.  This method will block until the thread pool
    /// is stopped.
    void stop() BSLS_KEYWORD_OVERRIDE;

    /// Authenticate the connection based on the type of AuthenticationMessage
    /// `authenticationMsg`.  Create an
    /// AuthenticationContext and store into `context`.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    int handleAuthentication(bsl::ostream& errorDescription,
                             const InitialConnectionContextSp& context,
                             const bmqp_ctrlmsg::AuthenticationMessage&
                                 authenticationMsg) BSLS_KEYWORD_OVERRIDE;

    /// Send out outbound authentication message with the specified `context`.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    int authenticationOutbound(const AuthenticationContextSp& context)
        BSLS_KEYWORD_OVERRIDE;

    /// Cancel any reauthentication timer for the specified `context`.
    /// This method is called when the channel is being closed, to ensure that
    /// no reauthentication is attempted afterwards.
    void cancelReauthenticationTimer(const AuthenticationContextSp& context)
        BSLS_KEYWORD_OVERRIDE;

    /// ACCESSORS

    /// Return the anonymous credential used for authentication.
    /// If no anonymous credential is set, return an empty optional.
    const bsl::optional<mqbcfg::Credential>&
    anonymousCredential() const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
