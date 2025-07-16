// Copyright 2016-2023 Bloomberg Finance L.P.
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

// bmqp_crc32c.cpp                                                    -*-C++-*-
#include <bmqp_crc32c.h>

#include <bmqscm_version.h>

// BDE
#include <ball_log.h>
#include <bdlde_crc32c.h>
#include <bsla_maybeunused.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

namespace {

BSLA_MAYBE_UNUSED const char k_LOG_CATEGORY[] = "BMQP.CRC32C";

}  // close unnamed namespace

// -------------
// struct Crc32c
// -------------

const unsigned int Crc32c::k_NULL_CRC32C = bdlde::Crc32c::k_NULL_CRC32C;

unsigned int
Crc32c::calculate(const void* data, unsigned int length, unsigned int crc)
{
    return bdlde::Crc32c::calculate(data, length, crc);
}

unsigned int Crc32c::calculate(const bdlbb::Blob& blob, unsigned int crc)
{
    const int numBuffers = blob.numDataBuffers();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(numBuffers == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return crc;  // RETURN
    }

    for (int i = 0; i < (numBuffers - 1); ++i) {
        const bdlbb::BlobBuffer& buffer = blob.buffer(i);

        crc = calculate(buffer.data(), buffer.size(), crc);
    }

    // Handle last data buffer
    crc = calculate(blob.buffer(numBuffers - 1).data(),
                    blob.lastDataBufferLength(),
                    crc);

    return crc;
}

}  // close package namespace
}  // close enterprise namespace
