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

// mwcio_statchannel.cpp                                              -*-C++-*-
#include <mwcio_statchannel.h>

#include <mwcscm_version.h>
// MWC
#include <mwcst_statcontext.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_iostream.h>
#include <bslmt_lockguard.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mwcio {

// -----------------------
// class StatChannelConfig
// -----------------------

// CREATORS
StatChannelConfig::StatChannelConfig(
    const bsl::shared_ptr<mwcio::Channel>&     channel,
    const bsl::shared_ptr<mwcst::StatContext>& statContext,
    BSLS_ANNOTATION_UNUSED bslma::Allocator* basicAllocator)
: d_channel_sp(channel)
, d_statContext_sp(statContext)
{
    // NOTHING
}

StatChannelConfig::StatChannelConfig(
    const StatChannelConfig& other,
    BSLS_ANNOTATION_UNUSED bslma::Allocator* basicAllocator)
: d_channel_sp(other.d_channel_sp)
, d_statContext_sp(other.d_statContext_sp)
{
    // NOTHING
}

// -----------------
// class StatChannel
// -----------------

// PRIVATE MANIPULATORS
void StatChannel::readCbWrapper(const Channel::ReadCallback& userCb,
                                const Status&                status,
                                int*                         numNeeded,
                                bdlbb::Blob*                 blob)
{
    const int length = blob->length();
    userCb(status, numNeeded, blob);
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(status &&
                                            (length > blob->length()))) {
        d_config.d_statContext_sp->adjustValue(Stat::e_BYTES_IN, length);
    }
}

// CREATORS
StatChannel::StatChannel(const StatChannelConfig& config,
                         bslma::Allocator*        basicAllocator)
: DecoratingChannelPartialImp(config.d_channel_sp, basicAllocator)
, d_config(config, basicAllocator)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config.d_statContext_sp);
}

StatChannel::~StatChannel()
{
    // NOTHING
}

// MANIPULATORS
void StatChannel::read(Status*                   status,
                       int                       numBytes,
                       const ReadCallback&       readCallback,
                       const bsls::TimeInterval& timeout)
{
    d_config.d_channel_sp->read(
        status,
        numBytes,
        bdlf::BindUtil::bind(&StatChannel::readCbWrapper,
                             this,
                             readCallback,
                             bdlf::PlaceHolders::_1,   // status
                             bdlf::PlaceHolders::_2,   // numNeeded
                             bdlf::PlaceHolders::_3),  // blob
        timeout);
}

void StatChannel::write(Status*            status,
                        const bdlbb::Blob& blob,
                        bsls::Types::Int64 highWatermark)
{
    // 'status' is an optional parameter, but here we want to ensure to always
    // have one
    Status localStatus;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!status)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        status = &localStatus;
    }

    d_config.d_channel_sp->write(status, blob, highWatermark);

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(*status)) {
        d_config.d_statContext_sp->adjustValue(Stat::e_BYTES_OUT,
                                               blob.length());
    }
}

}  // close package namespace
}  // close enterprise namespace
