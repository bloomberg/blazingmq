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

// bmqio_basechannelpartialimp.cpp                                    -*-C++-*-
#include <bmqio_basechannelpartialimp.h>

#include <bmqscm_version.h>
// BDE
#include <bdlf_bind.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqio {

// ---------------------------
// class BaseChannelPartialImp
// ---------------------------

// CREATORS
BaseChannelPartialImp::BaseChannelPartialImp(bslma::Allocator* basicAllocator)
: d_closeSignaler(basicAllocator)
, d_watermarkSignaler(basicAllocator)
, d_properties(basicAllocator)
{
    // NOTHING
}

// MANIPULATORS
bdlmt::SignalerConnection BaseChannelPartialImp::onClose(const CloseFn& cb)
{
    // PRECONDITIONS
    BSLS_ASSERT(cb);

    return d_closeSignaler.connect(cb);
}

bdlmt::SignalerConnection
BaseChannelPartialImp::onWatermark(const WatermarkFn& cb)
{
    // PRECONDITIONS
    BSLS_ASSERT(cb);

    return d_watermarkSignaler.connect(cb);
}

}  // close package namespace
}  // close enterprise namespace
