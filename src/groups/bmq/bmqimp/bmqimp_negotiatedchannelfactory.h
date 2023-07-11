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

// bmqimp_negotiatedchannelfactory.h                                  -*-C++-*-
#ifndef INCLUDED_BMQIMP_NEGOTIATEDCHANNELFACTORY
#define INCLUDED_BMQIMP_NEGOTIATEDCHANNELFACTORY

//@PURPOSE: Provide a 'ChannelFactory' that negotiates upon connecting to peer
//
//@CLASSES:
// bmqimp::NegotiatedChannelFactory
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a mechanism,
// 'bmqimp::NegotiatedChannelFactory', which is an implementation of the
// 'mwcio::ChannelFactory' protocol that performs initial negotiation with a
// peer on top of a channel created using a base 'mwcio::ChannelFactory'.

// BMQ

#include <bmqp_ctrlmsg_messages.h>

// MWC
#include <mwcio_channel.h>
#include <mwcio_channelfactory.h>
#include <mwcio_connectoptions.h>
#include <mwcio_listenoptions.h>
#include <mwcio_status.h>
#include <mwcu_sharedresource.h>

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

// ====================================
// class NegotiatedChannelFactoryConfig
// ====================================

/// Configuration for a `NegotiatedChannelFactory`.
class NegotiatedChannelFactoryConfig {
  private:
    // PRIVATE DATA
    mwcio::ChannelFactory*           d_baseFactory_p;
    bmqp_ctrlmsg::NegotiationMessage d_negotiationMessage;
    bsls::TimeInterval               d_negotiationTimeout;
    bdlbb::BlobBufferFactory*        d_bufferFactory_p;
    bslma::Allocator*                d_allocator_p;

    // FRIENDS
    friend class NegotiatedChannelFactory;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(NegotiatedChannelFactoryConfig,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    NegotiatedChannelFactoryConfig(
        mwcio::ChannelFactory*                  base,
        const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
        const bsls::TimeInterval&               negotiationTimeout,
        bdlbb::BlobBufferFactory*               bufferFactory,
        bslma::Allocator*                       basicAllocator = 0);

    NegotiatedChannelFactoryConfig(
        const NegotiatedChannelFactoryConfig& original,
        bslma::Allocator*                     basicAllocator = 0);
};

// ==============================
// class NegotiatedChannelFactory
// ==============================

/// `ChannelFactory` implementation that performs negotiation with the peer
/// upon connection.
class NegotiatedChannelFactory : public mwcio::ChannelFactory {
  public:
    // TYPES
    typedef NegotiatedChannelFactoryConfig Config;

    // CONSTANTS

    /// Name of a property set on the channel representing the broker's
    /// style of MessageProperties.
    /// Temporary; shall remove after 2nd roll out of "new style" brokers.
    static const char* k_CHANNEL_PROPERTY_MPS_EX;

  private:
    // PRIVATE DATA
    Config d_config;

    // Used to make sure no callback is invoked an a destroyed object.
    mutable mwcu::SharedResource<NegotiatedChannelFactory> d_self;

    // NOT IMPLEMENTED
    NegotiatedChannelFactory(const NegotiatedChannelFactory&)
        BSLS_KEYWORD_DELETED;
    NegotiatedChannelFactory&
    operator=(const NegotiatedChannelFactory&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(NegotiatedChannelFactory,
                                   bslma::UsesBslmaAllocator)

  private:
    // PRIVATE ACCESSORS

    /// Handle an event from our base ChannelFactory.
    void
    baseResultCallback(const ResultCallback&                  userCb,
                       mwcio::ChannelFactoryEvent::Enum       event,
                       const mwcio::Status&                   status,
                       const bsl::shared_ptr<mwcio::Channel>& channel) const;

    void negotiate(const bsl::shared_ptr<mwcio::Channel>& channel,
                   const ResultCallback&                  cb) const;

    void readPacketsCb(const bsl::shared_ptr<mwcio::Channel>& channel,
                       const ResultCallback&                  cb,
                       const mwcio::Status&                   status,
                       int*                                   numNeeded,
                       bdlbb::Blob*                           blob) const;

    void onBrokerNegotiationResponse(
        const bdlbb::Blob&                     packet,
        const ResultCallback&                  cb,
        const bsl::shared_ptr<mwcio::Channel>& channel) const;

  public:
    // CREATORS
    explicit NegotiatedChannelFactory(const Config&     config,
                                      bslma::Allocator* basicAllocator = 0);

    ~NegotiatedChannelFactory() BSLS_KEYWORD_OVERRIDE;

  public:
    // MANIPULATORS
    void listen(mwcio::Status*               status,
                bslma::ManagedPtr<OpHandle>* handle,
                const mwcio::ListenOptions&  options,
                const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;

    void connect(mwcio::Status*               status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const mwcio::ConnectOptions& options,
                 const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
