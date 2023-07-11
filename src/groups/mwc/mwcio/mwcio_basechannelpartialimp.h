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

// mwcio_basechannelpartialimp.h                                      -*-C++-*-
#ifndef INCLUDED_MWCIO_BASECHANNELPARTIALIMP
#define INCLUDED_MWCIO_BASECHANNELPARTIALIMP

//@PURPOSE: Provide a partial 'Channel' imp for a 'base' channel
//
//@CLASSES:
// mwcio::BaseChannelPartialImp
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a partial implementation of the
// 'mwcio::Channel' protocol for a 'base' Channel', that is a channel that is
// not decorating another 'channel'.

// MWC

#include <mwcio_channel.h>
#include <mwcio_status.h>
#include <mwct_propertybag.h>

// BDE
#include <bdlmt_signaler.h>
#include <bsl_limits.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mwcio {

// ===========================
// class BaseChannelPartialImp
// ===========================

/// Partial implementation of the `Channel` protocol that tries to handle
/// the signal hookups and property bag.
class BaseChannelPartialImp : public Channel {
  public:
    // TYPES
    typedef bdlmt::Signaler<CloseFnType>     CloseSignaler;
    typedef bdlmt::Signaler<WatermarkFnType> WatermarkSignaler;

  private:
    // PRIVATE DATA
    CloseSignaler     d_closeSignaler;
    WatermarkSignaler d_watermarkSignaler;
    mwct::PropertyBag d_properties;

    // NOT IMPLEMENTED
    BaseChannelPartialImp(const BaseChannelPartialImp&) BSLS_KEYWORD_DELETED;
    BaseChannelPartialImp&
    operator=(const BaseChannelPartialImp&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BaseChannelPartialImp,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit BaseChannelPartialImp(bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS

    /// Return a reference providing modifiable access to the `Signaler`
    /// for our `close` event.  This is exposed for access by the owning
    /// `ChannelFactory`.
    CloseSignaler& closeSignaler();

    /// Return a reference providing modifiable access to the `Signaler` for
    /// our `watermark` event.  This is exposed for access by the owning
    /// `ChannelFactory`.
    WatermarkSignaler& watermarkSignaler();

    // Channel

    /// Register the specified `cb` to be invoked when a `close` event
    /// occurs for this channel.  Return a `bdlmt::SignalerConnection`
    /// object than can be used to unregister the callback.
    bdlmt::SignalerConnection onClose(const CloseFn& cb) BSLS_KEYWORD_OVERRIDE;

    /// Register the specified `cb` to be invoked when a `watermark` event
    /// occurs for this channel.  Return a `bdlmt::SignalerConnection`
    /// object than can be used to unregister the callback.
    bdlmt::SignalerConnection
    onWatermark(const WatermarkFn& cb) BSLS_KEYWORD_OVERRIDE;

    /// Return a reference providing modifiable access to the properties of
    /// this Channel.
    mwct::PropertyBag& properties() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the allocator this object was created with.
    bslma::Allocator* allocator() const;

    // Channel

    /// Return a reference providing modifiable access to the properties of
    /// this Channel.
    const mwct::PropertyBag& properties() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                      INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
// ============================================================================

// MANIPULATORS
inline BaseChannelPartialImp::CloseSignaler&
BaseChannelPartialImp::closeSignaler()
{
    return d_closeSignaler;
}

inline BaseChannelPartialImp::WatermarkSignaler&
BaseChannelPartialImp::watermarkSignaler()
{
    return d_watermarkSignaler;
}

inline mwct::PropertyBag& BaseChannelPartialImp::properties()
{
    return d_properties;
}

// ACCESSORS
inline bslma::Allocator* BaseChannelPartialImp::allocator() const
{
    return d_properties.allocator();
}

inline const mwct::PropertyBag& BaseChannelPartialImp::properties() const
{
    return d_properties;
}

}  // close package namespace
}  // close enterprise namespace

#endif
