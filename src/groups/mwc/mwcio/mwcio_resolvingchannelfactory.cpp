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

// mwcio_resolvingchannelfactory.cpp                                  -*-C++-*-
#include <mwcio_resolvingchannelfactory.h>

#include <mwcscm_version.h>
// MWC
#include <mwcex_executionutil.h>
#include <mwcio_resolveutil.h>
#include <mwcu_weakmemfn.h>

// BDE
#include <ball_log.h>
#include <bdlb_stringrefutil.h>
#include <bdlf_bind.h>
#include <bdlma_localsequentialallocator.h>
#include <bsls_assert.h>

// NTC
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

namespace BloombergLP {
namespace mwcio {

BALL_LOG_SET_NAMESPACE_CATEGORY("MWCIO.RESOLVINGCHANNELFACTORY")

// -----------------------------------
// class ResolvingChannelFactoryConfig
// -----------------------------------

// CREATORS
ResolvingChannelFactoryConfig::ResolvingChannelFactoryConfig(
    ChannelFactory*        baseFactory,
    const ExecutionPolicy& executionPolicy,
    bslma::Allocator*      basicAllocator)
: d_baseFactory_p(baseFactory)
, d_resolutionFn(
      bsl::allocator_arg,
      basicAllocator,
      bdlf::BindUtil::bind(&ResolvingChannelFactoryUtil::defaultResolutionFn,
                           bdlf::PlaceHolders::_1,
                           bdlf::PlaceHolders::_2,
                           &mwcio::ResolveUtil::getDomainName,
                           true))
, d_executionPolicy(executionPolicy)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // PRECONDITIONS
    BSLS_ASSERT(baseFactory);
    BSLS_ASSERT(executionPolicy.executor());
}

ResolvingChannelFactoryConfig::ResolvingChannelFactoryConfig(
    const ResolvingChannelFactoryConfig& original,
    bslma::Allocator*                    basicAllocator)
: d_baseFactory_p(original.d_baseFactory_p)
, d_resolutionFn(bsl::allocator_arg, basicAllocator, original.d_resolutionFn)
, d_executionPolicy(original.d_executionPolicy)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

// MANIPULATORS
ResolvingChannelFactoryConfig&
ResolvingChannelFactoryConfig::resolutionFn(const ResolutionFn& value)
{
    // PRECONDITIONS
    BSLS_ASSERT(value);

    d_resolutionFn = value;
    return *this;
}

// -------------------------------------
// class ResolvingChannelFactory_Channel
// -------------------------------------

// CREATORS
ResolvingChannelFactory_Channel::ResolvingChannelFactory_Channel(
    const bsl::shared_ptr<Channel>& channel,
    bslma::Allocator*               basicAllocator)
: DecoratingChannelPartialImp(channel, basicAllocator)
, d_resolvedPeerUri(basicAllocator)
, d_peerUri()
{
    // PRECONDITIONS
    BSLS_ASSERT(channel);

    d_peerUri = &channel->peerUri();
}

// MANIPULATORS
bsl::string& ResolvingChannelFactory_Channel::resolvedUri()
{
    return d_resolvedPeerUri;
}

void ResolvingChannelFactory_Channel::updatePeerUri()
{
    if (!d_resolvedPeerUri.empty()) {
        d_peerUri = &d_resolvedPeerUri;
    }
}

// ACCESSORS
const bsl::string& ResolvingChannelFactory_Channel::peerUri() const
{
    return *d_peerUri;
}

// -----------------------------
// class ResolvingChannelFactory
// -----------------------------

// PRIVATE ACCESSORS
void ResolvingChannelFactory::resolveUriFn(
    const bsl::shared_ptr<ResolvingChannel>& channel) const
{
    d_config.d_resolutionFn(&channel->resolvedUri(), *channel->base());
    channel->updatePeerUri();
}

void ResolvingChannelFactory::baseResultCallback(
    const ResultCallback&           userCb,
    ChannelFactoryEvent::Enum       event,
    const Status&                   status,
    const bsl::shared_ptr<Channel>& channel) const
{
    if (event != ChannelFactoryEvent::e_CHANNEL_UP) {
        userCb(event, status, channel);
        return;  // RETURN
    }

    bsl::shared_ptr<ResolvingChannel> newChannel;
    newChannel.createInplace(d_config.d_allocator_p,
                             channel,
                             d_config.d_allocator_p);

    mwcex::ExecutionUtil::execute(
        d_config.d_executionPolicy,
        bdlf::BindUtil::bind(mwcu::WeakMemFnUtil::weakMemFn(
                                 &ResolvingChannelFactory::resolveUriFn,
                                 d_self.acquireWeak()),
                             newChannel));

    userCb(event, status, newChannel);
}

// CREATORS
ResolvingChannelFactory::ResolvingChannelFactory(
    const Config&     config,
    bslma::Allocator* basicAllocator)
: d_config(config, basicAllocator)
, d_self(this)  // use default allocator
{
    // NOTHING
}

ResolvingChannelFactory::~ResolvingChannelFactory()
{
    // synchronize with any currently running callbacks and prevent future
    // callback invocations
    d_self.invalidate();
}

// MANIPULATORS
void ResolvingChannelFactory::listen(Status*                      status,
                                     bslma::ManagedPtr<OpHandle>* handle,
                                     const ListenOptions&         options,
                                     const ResultCallback&        cb)
{
    d_config.d_baseFactory_p->listen(
        status,
        handle,
        options,
        bdlf::BindUtil::bind(mwcu::WeakMemFnUtil::weakMemFn(
                                 &ResolvingChannelFactory::baseResultCallback,
                                 d_self.acquireWeak()),
                             cb,
                             bdlf::PlaceHolders::_1,    // event
                             bdlf::PlaceHolders::_2,    // status
                             bdlf::PlaceHolders::_3));  // channel
}

void ResolvingChannelFactory::connect(Status*                      status,
                                      bslma::ManagedPtr<OpHandle>* handle,
                                      const ConnectOptions&        options,
                                      const ResultCallback&        cb)
{
    d_config.d_baseFactory_p->connect(
        status,
        handle,
        options,
        bdlf::BindUtil::bind(mwcu::WeakMemFnUtil::weakMemFn(
                                 &ResolvingChannelFactory::baseResultCallback,
                                 d_self.acquireWeak()),
                             cb,
                             bdlf::PlaceHolders::_1,    // event
                             bdlf::PlaceHolders::_2,    // status
                             bdlf::PlaceHolders::_3));  // channel
}

// ----------------------------------
// struct ResolvingChannelFactoryUtil
// ----------------------------------

// CLASS METHODS
void ResolvingChannelFactoryUtil::defaultResolutionFn(
    bsl::string*     resolvedUri,
    const Channel&   baseChannel,
    const ResolveFn& resolveFn,
    bool             verbose)
{
    bslstl::StringRef peerUri = baseChannel.peerUri();
    bslstl::StringRef colon   = bdlb::StringRefUtil::strstr(peerUri, ":");
    if (colon.length() == 0) {
        if (verbose) {
            BALL_LOG_WARN << "Cannot resolve peerUri.  Can't find ':' in "
                          << peerUri;
        }

        return;  // RETURN
    }

    bdlma::LocalSequentialAllocator<128> arena;

    bsl::string     ipAddrStr(peerUri.data(), colon.data(), &arena);
    ntsa::IpAddress ipAddr;

    if (!ipAddr.parse(ipAddrStr)) {
        if (verbose) {
            BALL_LOG_WARN << "Cannot resolve peerUri.  Ip address invalid in "
                          << peerUri;
        }

        return;  // RETURN
    }

    bsl::string resolvedName(&arena);
    ntsa::Error error = resolveFn(&resolvedName, ipAddr);
    if (error.code() != ntsa::Error::e_OK) {
        if (verbose) {
            BALL_LOG_WARN << "Cannot resolve peerUri.  Resolution failed for "
                          << peerUri << ", Error: " << error;
        }

        return;  // RETURN
    }

    // Reserve enough room in the resolvedUri for the string we'll return
    resolvedUri->clear();
    resolvedUri->reserve(resolvedName.length() + peerUri.length() + 2);
    resolvedUri->append(ipAddrStr);
    resolvedUri->append(1, '~');
    resolvedUri->append(resolvedName);
    resolvedUri->append(colon.data(), peerUri.end());
}

}  // close package namespace
}  // close enterprise namespace
