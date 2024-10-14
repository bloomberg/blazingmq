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

// bmqio_decoratingchannelpartialimp.h                                -*-C++-*-
#ifndef INCLUDED_BMQIO_DECORATINGCHANNELPARTIALIMP
#define INCLUDED_BMQIO_DECORATINGCHANNELPARTIALIMP

//@PURPOSE: Provide a partial imp of 'bmqio::Channel' for decorating channels
//
//@CLASSES:
// bmqio::DecoratingChannelPartialImp
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a partial implementation,
// 'bmqio::DecoratingChannelPartialImp', of the 'bmqio::Channel' protocol for
// 'bmqio::Channel' implementations that decorate an underlying
// 'bmqio::Channel'.  It mostly handles forwarding the most logical methods to
// the underlying 'bmqio::Channel'.

#include <bmqio_channel.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqio {

// =================================
// class DecoratingChannelPartialImp
// =================================

/// Partial implementation of the `Channel` protocol for decorating Channel
/// implementations.
class DecoratingChannelPartialImp : public Channel {
  private:
    // DATA
    bsl::shared_ptr<Channel> d_base;

    // NOT IMPLEMENTED
    DecoratingChannelPartialImp(const DecoratingChannelPartialImp&);
    DecoratingChannelPartialImp& operator=(const DecoratingChannelPartialImp&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DecoratingChannelPartialImp,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit DecoratingChannelPartialImp(const bsl::shared_ptr<Channel>& base,
                                         bslma::Allocator* basicAllocator = 0);
    ~DecoratingChannelPartialImp() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    // Channel
    void read(Status*                   status,
              int                       numBytes,
              const ReadCallback&       readCallback,
              const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    void write(Status*            status,
               const bdlbb::Blob& blob,
               bsls::Types::Int64 watermark = bsl::numeric_limits<int>::max())
        BSLS_KEYWORD_OVERRIDE;

    void cancelRead() BSLS_KEYWORD_OVERRIDE;
    void close(const Status& status = Status()) BSLS_KEYWORD_OVERRIDE;
    int  execute(const ExecuteCb& cb) BSLS_KEYWORD_OVERRIDE;
    bdlmt::SignalerConnection onClose(const CloseFn& cb) BSLS_KEYWORD_OVERRIDE;
    bdlmt::SignalerConnection
    onWatermark(const WatermarkFn& cb) BSLS_KEYWORD_OVERRIDE;

    /// Forward to the underlying base `Channel`.
    bmqvt::PropertyBag& properties() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a pointer providing modifiable access to the underlying base
    /// `Channel`.
    Channel* base() const;

    // Channel
    const bsl::string& peerUri() const BSLS_KEYWORD_OVERRIDE;

    /// Forward to the underlying base `Channel`.
    const bmqvt::PropertyBag& properties() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                      INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
// ============================================================================

// MANIPULATORS
inline void
DecoratingChannelPartialImp::read(Status*                   status,
                                  int                       numBytes,
                                  const ReadCallback&       readCallback,
                                  const bsls::TimeInterval& timeout)
{
    d_base->read(status, numBytes, readCallback, timeout);
}

inline void DecoratingChannelPartialImp::write(Status*            status,
                                               const bdlbb::Blob& blob,
                                               bsls::Types::Int64 watermark)
{
    d_base->write(status, blob, watermark);
}

inline void DecoratingChannelPartialImp::cancelRead()
{
    d_base->cancelRead();
}

inline void DecoratingChannelPartialImp::close(const Status& status)
{
    d_base->close(status);
}

inline int DecoratingChannelPartialImp::execute(const ExecuteCb& cb)
{
    return d_base->execute(cb);
}

inline bdlmt::SignalerConnection
DecoratingChannelPartialImp::onClose(const CloseFn& cb)
{
    return d_base->onClose(cb);
}

inline bdlmt::SignalerConnection
DecoratingChannelPartialImp::onWatermark(const WatermarkFn& cb)
{
    return d_base->onWatermark(cb);
}

inline bmqvt::PropertyBag& DecoratingChannelPartialImp::properties()
{
    return d_base->properties();
}

// ACCESSORS
inline Channel* DecoratingChannelPartialImp::base() const
{
    return d_base.get();
}

inline const bsl::string& DecoratingChannelPartialImp::peerUri() const
{
    return d_base->peerUri();
}

inline const bmqvt::PropertyBag&
DecoratingChannelPartialImp::properties() const
{
    return d_base->properties();
}

}  // close package namespace
}  // close enterprise namespace

#endif
