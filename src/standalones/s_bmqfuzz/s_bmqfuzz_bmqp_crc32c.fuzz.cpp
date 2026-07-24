// Copyright 2026 Bloomberg Finance L.P.
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

#include <cstdlib>

#include <fuzzer/FuzzedDataProvider.h>
#include <string>

#include <bmqp_crc32c.h>

#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);

    const int bufferSize       = provider.ConsumeIntegralInRange<int>(1, 256);
    const unsigned int seedCrc = provider.ConsumeIntegral<unsigned int>();
    const std::string  payload = provider.ConsumeRemainingBytesAsString();

    const unsigned int rawCrc = bmqp::Crc32c::calculate(
        payload.empty() ? 0 : payload.data(),
        static_cast<unsigned int>(payload.size()),
        seedCrc);

    bdlbb::PooledBlobBufferFactory factory(bufferSize);
    bdlbb::Blob                    blob(&factory);
    bdlbb::BlobUtil::append(&blob,
                            payload.data(),
                            static_cast<int>(payload.size()));

    const unsigned int blobCrc = bmqp::Crc32c::calculate(blob, seedCrc);

    if (rawCrc != blobCrc) {
        abort();
    }

    return 0;
}
