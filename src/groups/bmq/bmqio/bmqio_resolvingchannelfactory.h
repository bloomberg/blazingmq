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

// bmqio_resolvingchannelfactory.h                                    -*-C++-*-
#ifndef INCLUDED_BMQIO_RESOLVINGCHANNELFACTORY
#define INCLUDED_BMQIO_RESOLVINGCHANNELFACTORY

//@PURPOSE: Provide a 'ChannelFactory' that resolves the URIs of its channels
//
//@CLASSES:
// bmqio::ResolvingChannelFactory
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a mechanism,
// 'bmqio::ResolvingChannelFactory', which is an implementation of the
// 'bmqio::ChannelFactory' protocol that resolves the 'peerUri' of the channels
// created by a base 'bmqio::ChannelFactory' and updates them on its channels
// dynamically.

#include <bmqex_executionpolicy.h>
#include <bmqex_executionproperty.h>
#include <bmqex_executor.h>
#include <bmqio_channelfactory.h>
#include <bmqio_decoratingchannelpartialimp.h>
#include <bmqu_sharedresource.h>

// BDE
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>

// NTC
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

namespace BloombergLP {

namespace bmqio {

// ===================================
// class ResolvingChannelFactoryConfig
// ===================================

/// Configuration for a `ResolvingChannelFactory`.
class ResolvingChannelFactoryConfig {
  public:
    // TYPES

    /// Callback invoked by the `ResolvingChannelFactory` when the
    /// `peerUri` of the specified `baseChannel` needs to be resolved.  On
    /// return, if the specified `resolvedUri` remains empty, the `peerUri`
    /// of the resolved Channel will be unchanged.  Otherwise, the `peerUri`
    /// of the channel wrapping `baseChannel` will be updated to start
    /// returning the `resolvedUri`.
    typedef bsl::function<void(bsl::string*   resolvedUri,
                               const Channel& baseChannel)>
        ResolutionFn;

    /// An execution policy defining how the resolution callback is to be
    /// executed.
    typedef bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay,
                                   bmqex::Executor>
        ExecutionPolicy;

  private:
    // PRIVATE DATA
    ChannelFactory* d_baseFactory_p;

    ResolutionFn d_resolutionFn;

    ExecutionPolicy d_executionPolicy;

    bslma::Allocator* d_allocator_p;

    // FRIENDS
    friend class ResolvingChannelFactory;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ResolvingChannelFactoryConfig,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `ResolvingChannelFactoryConfig` object initialized with the
    /// specified `baseFactory` and `executionPolicy`.  Optionally specify a
    /// `basicAllocator` used to supply memory.  If `basicAllocator` is 0,
    /// the default memory allocator is used.
    ResolvingChannelFactoryConfig(ChannelFactory*        baseFactory,
                                  const ExecutionPolicy& executionPolicy,
                                  bslma::Allocator*      basicAllocator = 0);

    /// Create a `ResolvingChannelFactoryConfig` object having the same
    /// value as the specified `original` object.  Optionally specify a
    /// `basicAllocator` used to supply memory.  If `basicAllocator` is 0,
    /// the default memory allocator is used.
    ResolvingChannelFactoryConfig(
        const ResolvingChannelFactoryConfig& original,
        bslma::Allocator*                    basicAllocator = 0);

  public:
    // MANIPULATORS

    /// Set the function that will be used to resolved the `peerUri` of new
    /// `Channel` objects to the specified `value` and return a reference
    /// providing modifiable access to this object.  By default, this is
    /// 'ResolvingChannelFactoryUtil::defaultResolutionFn(
    ///                     _1,
    ///                     _2,
    ///                     bmqio::ResolveUtil::getDomainName,
    ///                     true)'.
    ResolvingChannelFactoryConfig& resolutionFn(const ResolutionFn& value);
};

// =====================================
// class ResolvingChannelFactory_Channel
// =====================================

/// A `Channel` produced by a `ResolvingChannelFactory`.
class ResolvingChannelFactory_Channel : public DecoratingChannelPartialImp {
  private:
    // PRIVATE DATA
    bsl::string d_resolvedPeerUri;

    bsls::AtomicPointer<const bsl::string> d_peerUri;

  private:
    // NOT IMPLEMENTED
    ResolvingChannelFactory_Channel(const ResolvingChannelFactory_Channel&)
        BSLS_KEYWORD_DELETED;
    ResolvingChannelFactory_Channel&
    operator=(const ResolvingChannelFactory_Channel&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ResolvingChannelFactory_Channel,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS
    explicit ResolvingChannelFactory_Channel(
        const bsl::shared_ptr<Channel>& channel,
        bslma::Allocator*               basicAllocator = 0);

  public:
    // MANIPULATORS

    /// Return a reference providing modifiable access to the resolvedUri
    /// of this object.
    bsl::string& resolvedUri();

    /// If `resolvedUri` is not empty, start returning it from `peerUri`,
    /// otherwise keep returning the `peerUri` from our base.
    void updatePeerUri();

  public:
    // ACCESSORS
    // DecoratingChannelPartialImp

    /// Return our base Channel's peerUri until our resolution is done, and
    /// start returning the resolved peerUri after that.
    const bsl::string& peerUri() const BSLS_KEYWORD_OVERRIDE;
};

// =============================
// class ResolvingChannelFactory
// =============================

/// `ChannelFactory` implementation that resolves the `peerUri` of a base
/// channel.
class ResolvingChannelFactory : public ChannelFactory {
  public:
    // TYPES

    /// Defines a short alias type for `ResolvingChannelFactoryConfig`.
    typedef ResolvingChannelFactoryConfig Config;

  private:
    // PRIVATE TYPES
    typedef ResolvingChannelFactory_Channel ResolvingChannel;

  private:
    // PRIVATE DATA
    Config d_config;

    // Used to make sure no callback is invoked an a destroyed object.
    mutable bmqu::SharedResource<ResolvingChannelFactory> d_self;

  private:
    // NOT IMPLEMENTED
    ResolvingChannelFactory(const ResolvingChannelFactory&)
        BSLS_KEYWORD_DELETED;
    ResolvingChannelFactory&
    operator=(const ResolvingChannelFactory&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ResolvingChannelFactory,
                                   bslma::UsesBslmaAllocator)

  private:
    // PRIVATE ACCESSORS

    /// Callback invoked on our config's executor to resolve the base
    /// channel's `peerUri`.
    void resolveUriFn(const bsl::shared_ptr<ResolvingChannel>& channel) const;

    /// Handle an event from our base `ChannelFactory`.
    void baseResultCallback(const ResultCallback&           userCb,
                            ChannelFactoryEvent::Enum       event,
                            const Status&                   status,
                            const bsl::shared_ptr<Channel>& channel) const;

  public:
    // CREATORS

    /// Create a `ResolvingChannelFactory` configured according to the
    /// specified `config`.  Optionally specify a `basicAllocator` used to
    /// supply memory.  If `basicAllocator` is 0, the default memory
    /// allocator is used.
    explicit ResolvingChannelFactory(const Config&     config,
                                     bslma::Allocator* basicAllocator = 0);

    /// Destroy this object.
    ~ResolvingChannelFactory() BSLS_KEYWORD_OVERRIDE;

  public:
    // MANIPULATORS
    // ChannelFactory
    void listen(Status*                      status,
                bslma::ManagedPtr<OpHandle>* handle,
                const ListenOptions&         options,
                const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;

    void connect(Status*                      status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const ConnectOptions&        options,
                 const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;
};

// ==================================
// struct ResolvingChannelFactoryUtil
// ==================================

/// Namespace for free functions useful when working with a
/// `ResolvingChannelFactory`.
struct ResolvingChannelFactoryUtil {
    // TYPES

    /// Type for a function that can resolve an ip address into its domain
    /// name, returning `error` containing error information.
    typedef bsl::function<ntsa::Error(bsl::string*           domainName,
                                      const ntsa::IpAddress& address)>
        ResolveFn;

    // CLASS METHODS

    /// Assume the `peerUri` of the specified `baseChannel` is of the form
    /// `<ip address>:<port>`.  If the `<ip address>` can be resolved using
    /// `bmqio::ResolveUtil::getDomainName` then load into the specified
    /// `resolvedUri` a string of the form
    /// `<ip address>(<resolved ip address>):<port>`.  If there's an error,
    /// return without modifying `resolvedUri` and if the specified
    /// `verbose` is `true`, log out an error message.
    static void defaultResolutionFn(bsl::string*     resolvedUri,
                                    const Channel&   baseChannel,
                                    const ResolveFn& resolveFn,
                                    bool             verbose);
};

}  // close package namespace
}  // close enterprise namespace

#endif
