// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqimp_authenticatedchannelfactory.h                     -*-C++-*-
#ifndef INCLUDED_BMQIMP_AUTHENTICATEDCHANNELFACTORY
#define INCLUDED_BMQIMP_AUTHENTICATEDCHANNELFACTORY

/// @file bmqimp_authenticatedchannelfactory.h
///
/// @brief Provide a `ChannelFactory` that authenticates
/// upon connecting to peer.
///
/// @bbref{bmqimp::AuthenticatedChannelFactory} is an implementation of the
/// `bmqio::ChannelFactory` protocol that performs the initial authentication
/// with a peer on top of a channel created using a base
/// `bmqio::ChannelFactory`.  It also provides function to handle
/// reauthentication events.

// BMQ
#include <bmqio_channel.h>
#include <bmqio_channelfactory.h>
#include <bmqio_connectoptions.h>
#include <bmqio_listenoptions.h>
#include <bmqio_status.h>
#include <bmqp_blobpoolutil.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqt_sessionoptions.h>
#include <bmqu_sharedresource.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_variant.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

namespace bmqimp {

// =======================================
// class AuthenticatedChannelFactoryConfig
// =======================================

/// Configuration for a `AuthenticatedChannelFactory`.
class AuthenticatedChannelFactoryConfig {
  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool          BlobSpPool;
    typedef bmqt::SessionOptions::AuthnCredentialCb AuthnCredentialCb;

  private:
    // PRIVATE DATA
    bmqio::ChannelFactory* d_baseFactory_p;

    /// Used to schedule events for sending reauthentication requests.
    /// Held, not owned.
    bdlmt::EventScheduler* d_scheduler_p;

    AuthnCredentialCb  d_authnCredentialCb;
    bsls::TimeInterval d_authenticationTimeout;
    BlobSpPool*        d_blobSpPool_p;
    bslma::Allocator*  d_allocator_p;

    // FRIENDS
    friend class AuthenticatedChannelFactory;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AuthenticatedChannelFactoryConfig,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    AuthenticatedChannelFactoryConfig(
        bmqio::ChannelFactory*    base,
        bdlmt::EventScheduler*    scheduler,
        AuthnCredentialCb         authnCredentialCb,
        const bsls::TimeInterval& authenticationTimeout,
        BlobSpPool*               blobSpPool_p,
        bslma::Allocator*         basicAllocator = 0);

    AuthenticatedChannelFactoryConfig(
        const AuthenticatedChannelFactoryConfig& original,
        bslma::Allocator*                        basicAllocator = 0);
};

// =================================
// class AuthenticatedChannelFactory
// =================================

/// `ChannelFactory` implementation that performs initial connection
/// (authentication and negotiation) with peer upon channel connection.
class AuthenticatedChannelFactory : public bmqio::ChannelFactory {
  public:
    // TYPES
    typedef AuthenticatedChannelFactoryConfig Config;

    typedef bdlmt::EventScheduler::RecurringEventHandle EventHandle;

  private:
    // PRIVATE DATA
    Config d_config;

    /// Handle for the scheduled reauthentication timer.  An authentication
    /// request is sent when this event fires.
    bdlmt::EventScheduler::EventHandle d_reauthenticationTimeoutHandle;

    /// Used to make sure no callback is invoked on a destroyed object.
    mutable bmqu::SharedResource<AuthenticatedChannelFactory> d_self;

    // NOT IMPLEMENTED
    AuthenticatedChannelFactory(const AuthenticatedChannelFactory&)
        BSLS_KEYWORD_DELETED;
    AuthenticatedChannelFactory&
    operator=(const AuthenticatedChannelFactory&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AuthenticatedChannelFactory,
                                   bslma::UsesBslmaAllocator)

  private:
    // PRIVATE ACCESSORS
    void sendRequest(const bsl::shared_ptr<bmqio::Channel>& channel,
                     const ResultCallback&                  cb) const;

    void readResponse(const bsl::shared_ptr<bmqio::Channel>& channel,
                      const ResultCallback&                  cb) const;

    void authenticate(const bsl::shared_ptr<bmqio::Channel>& channel,
                      const ResultCallback&                  cb) const;

    /// Given the specified `lifetimeMs`, return the interval in milliseconds
    /// after which reauthentication should be performed.  This interval is
    /// calculated with a buffer to avoid cutting too close to the actual
    /// expiration time.
    int timeoutInterval(int lifetimeMs) const;

    // PRIVATE MANIPULATORS

    /// Handle an event from our base ChannelFactory.
    void baseResultCallback(const ResultCallback&                  cb,
                            bmqio::ChannelFactoryEvent::Enum       event,
                            const bmqio::Status&                   status,
                            const bsl::shared_ptr<bmqio::Channel>& channel);

    void readPacketsCb(const bsl::shared_ptr<bmqio::Channel>& channel,
                       const ResultCallback&                  cb,
                       const bmqio::Status&                   status,
                       int*                                   numNeeded,
                       bdlbb::Blob*                           blob);

    void onBrokerAuthenticationResponse(
        const bdlbb::Blob&                     packet,
        const ResultCallback&                  cb,
        const bsl::shared_ptr<bmqio::Channel>& channel);

    void onChannelDown(const bmqio::Status& status);

  public:
    // CREATORS
    explicit AuthenticatedChannelFactory(const Config&     config,
                                         bslma::Allocator* basicAllocator = 0);

    ~AuthenticatedChannelFactory() BSLS_KEYWORD_OVERRIDE;

  public:
    // MANIPULATORS
    void listen(bmqio::Status*               status,
                bslma::ManagedPtr<OpHandle>* handle,
                const bmqio::ListenOptions&  options,
                const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;

    void connect(bmqio::Status*               status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const bmqio::ConnectOptions& options,
                 const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;

    void
    processAuthenticationEvent(const bmqp::Event&                     event,
                               const ResultCallback&                  cb,
                               const bsl::shared_ptr<bmqio::Channel>& channel);
};

}  // close package namespace
}  // close enterprise namespace

#endif
