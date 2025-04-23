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

// bmqimp_initialconnectionchannelfactory.h                     -*-C++-*-
#ifndef INCLUDED_BMQIMP_NEGOTIATEDCHANNELFACTORY
#define INCLUDED_BMQIMP_NEGOTIATEDCHANNELFACTORY

//@PURPOSE: Provide a 'ChannelFactory' that negotiates upon connecting to peer
//
//@CLASSES:
// bmqimp::InitialConnectionChannelFactory
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a mechanism,
// 'bmqimp::InitialConnectionChannelFactory', which is an implementation of the
// 'bmqio::ChannelFactory' protocol that performs initial connection with a
// peer on top of a channel created using a base 'bmqio::ChannelFactory'.

// BMQ
#include <bmqio_channel.h>
#include <bmqio_channelfactory.h>
#include <bmqio_connectoptions.h>
#include <bmqio_listenoptions.h>
#include <bmqio_status.h>
#include <bmqp_blobpoolutil.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqu_sharedresource.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

namespace bmqimp {

// ===========================================
// class InitialConnectionChannelFactoryConfig
// ===========================================

/// Configuration for a `InitialConnectionChannelFactory`.
class InitialConnectionChannelFactoryConfig {
  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

  private:
    // PRIVATE DATA
    bmqio::ChannelFactory*              d_baseFactory_p;
    bmqp_ctrlmsg::AuthenticationMessage d_authenticationMessage;
    bsls::TimeInterval                  d_authenticationTimeout;
    bmqp_ctrlmsg::NegotiationMessage    d_negotiationMessage;
    bsls::TimeInterval                  d_negotiationTimeout;
    BlobSpPool*                         d_blobSpPool_p;
    bslma::Allocator*                   d_allocator_p;

    // FRIENDS
    friend class InitialConnectionChannelFactory;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(InitialConnectionChannelFactoryConfig,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    InitialConnectionChannelFactoryConfig(
        bmqio::ChannelFactory*                     base,
        const bmqp_ctrlmsg::AuthenticationMessage& authenticationMessage,
        const bsls::TimeInterval&                  authenticationTimeout,
        const bmqp_ctrlmsg::NegotiationMessage&    negotiationMessage,
        const bsls::TimeInterval&                  negotiationTimeout,
        BlobSpPool*                                blobSpPool_p,
        bslma::Allocator*                          basicAllocator = 0);

    InitialConnectionChannelFactoryConfig(
        const InitialConnectionChannelFactoryConfig& original,
        bslma::Allocator*                            basicAllocator = 0);
};

// =====================================
// class InitialConnectionChannelFactory
// =====================================

/// `ChannelFactory` implementation that performs initial connection
/// (authentication and negotiation) with peer upon channel connection.
class InitialConnectionChannelFactory : public bmqio::ChannelFactory {
  public:
    // TYPES
    typedef InitialConnectionChannelFactoryConfig Config;

    // CONSTANTS

    /// Name of a property set on the channel representing the broker's
    /// style of MessageProperties.
    /// Temporary; shall remove after 2nd roll out of "new style" brokers.
    static const char* k_CHANNEL_PROPERTY_MPS_EX;

    /// Temporary safety switch to control configure request.
    static const char* k_CHANNEL_PROPERTY_CONFIGURE_STREAM;

    static const char* k_CHANNEL_PROPERTY_HEARTBEAT_INTERVAL_MS;

    static const char* k_CHANNEL_PROPERTY_MAX_MISSED_HEARTBEATS;

  private:
    // TYPES
    enum ACTION { AUTHENTICATION = 0, NEGOTIATION = 1 };

    // PRIVATE DATA
    Config d_config;

    // Used to make sure no callback is invoked an a destroyed object.
    mutable bmqu::SharedResource<InitialConnectionChannelFactory> d_self;

    // NOT IMPLEMENTED
    InitialConnectionChannelFactory(const InitialConnectionChannelFactory&)
        BSLS_KEYWORD_DELETED;
    InitialConnectionChannelFactory&
    operator=(const InitialConnectionChannelFactory&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(InitialConnectionChannelFactory,
                                   bslma::UsesBslmaAllocator)

  private:
    // PRIVATE ACCESSORS

    /// Handle an event from our base ChannelFactory.
    void
    baseResultCallback(const ResultCallback&                  userCb,
                       bmqio::ChannelFactoryEvent::Enum       event,
                       const bmqio::Status&                   status,
                       const bsl::shared_ptr<bmqio::Channel>& channel) const;

    void sendRequest(const bsl::shared_ptr<bmqio::Channel>& channel,
                     const ACTION                           action,
                     const ResultCallback&                  cb) const;

    void readResponse(const bsl::shared_ptr<bmqio::Channel>& channel,
                      const ACTION                           action,
                      const ResultCallback&                  cb) const;

    void authenticate(const bsl::shared_ptr<bmqio::Channel>& channel,
                      const ResultCallback&                  cb) const;

    void negotiate(const bsl::shared_ptr<bmqio::Channel>& channel,
                   const ResultCallback&                  cb) const;

    void readPacketsCb(const bsl::shared_ptr<bmqio::Channel>& channel,
                       const ResultCallback&                  cb,
                       const bmqio::Status&                   status,
                       int*                                   numNeeded,
                       bdlbb::Blob*                           blob) const;

    int decodeInitialConnectionMessage(
        const bdlbb::Blob&                                  packet,
        bsl::optional<bmqp_ctrlmsg::AuthenticationMessage>* authenticationMsg,
        bsl::optional<bmqp_ctrlmsg::NegotiationMessage>*    negotiationMsg,
        const ResultCallback&                               cb,
        const bsl::shared_ptr<bmqio::Channel>&              channel) const;

    void onBrokerAuthenticationResponse(
        const bmqp_ctrlmsg::AuthenticationMessage& response,
        const ResultCallback&                      cb,
        const bsl::shared_ptr<bmqio::Channel>&     channel) const;

    void onBrokerNegotiationResponse(
        const bmqp_ctrlmsg::NegotiationMessage& response,
        const ResultCallback&                   cb,
        const bsl::shared_ptr<bmqio::Channel>&  channel) const;

  public:
    // CREATORS
    explicit InitialConnectionChannelFactory(
        const Config&     config,
        bslma::Allocator* basicAllocator = 0);

    ~InitialConnectionChannelFactory() BSLS_KEYWORD_OVERRIDE;

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
};

}  // close package namespace
}  // close enterprise namespace

#endif
