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

// bmqio_statchannel.h                                                -*-C++-*-
#ifndef INCLUDED_BMQIO_STATCHANNEL
#define INCLUDED_BMQIO_STATCHANNEL

//@PURPOSE: Provide a 'bmqio::Channel' that collects stats.
//
//@CLASSES:
// bmqio::StatChannel
// bmqio::StatChannelConfig
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a mechanism, 'bmqio::StatChannel', which
// is a concrete implementation of the 'bmqio::Channel' protocol that collects
// stats.

#include <bmqio_channel.h>
#include <bmqio_channelfactory.h>
#include <bmqio_decoratingchannelpartialimp.h>
#include <bmqst_statcontext.h>

// BDE
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqio {

// FORWARD DECLARE
class StatChannel;

// =======================
// class StatChannelConfig
// =======================

/// Configuration for a `StatChannel`.
class StatChannelConfig {
  private:
    // DATA
    bsl::shared_ptr<Channel> d_channel_sp;
    // underlying Channel

    bsl::shared_ptr<bmqst::StatContext> d_statContext_sp;
    // stat conext for this channel

    // FRIENDS
    friend class StatChannel;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatChannelConfig,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    StatChannelConfig(const bsl::shared_ptr<Channel>&            channel,
                      const bsl::shared_ptr<bmqst::StatContext>& statContext,
                      bslma::Allocator* basicAllocator = 0);
    StatChannelConfig(const StatChannelConfig& other,
                      bslma::Allocator*        basicAllocator = 0);
};

// =================
// class StatChannel
// =================

/// Implementation of the `Channel` protocol that collects stats.
class StatChannel : public DecoratingChannelPartialImp {
  public:
    // TYPES
    typedef StatChannelConfig Config;

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    ///
    /// NOTE: The values in this enum must match and be in the same order as
    ///       the stat context configuration used (see
    ///       `bmqio::StatChannelFactory`).
    struct Stat {
        // TYPES
        enum Enum { e_BYTES_IN = 0, e_BYTES_OUT = 1, e_CONNECTIONS = 2 };
    };

  private:
    // DATA
    Config            d_config;
    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE MANIPULATORS
    void readCbWrapper(const Channel::ReadCallback& userCb,
                       const Status&                status,
                       int*                         numNeeded,
                       bdlbb::Blob*                 blob);

    // NOT IMPLEMENTED
    StatChannel(const StatChannel&);
    StatChannel& operator=(const StatChannel&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatChannel, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit StatChannel(const Config&     config,
                         bslma::Allocator* basicAllocator = 0);

    ~StatChannel() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    // channel
    void read(Status*                      status,
              int                          numBytes,
              const Channel::ReadCallback& readCallback,
              const bsls::TimeInterval&    timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    void write(Status*            status,
               const bdlbb::Blob& blob,
               bsls::Types::Int64 highWatermark) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
