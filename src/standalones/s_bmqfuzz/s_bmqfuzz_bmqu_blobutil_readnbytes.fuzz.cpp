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

#include <fuzzer/FuzzedDataProvider.h>

#include <bmqu_blob.h>

#include <bdlbb_blob.h>
#include <bsl_cstring.h>
#include <bsl_memory.h>
#include <bsl_vector.h>

using namespace BloombergLP;

namespace {

const int k_MAX_BUFFERS  = 32;
const int k_MAX_BUF_SIZE = 512;
const int k_MAX_LENGTH   = 64 * 1024;

}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);

    bslma::Allocator* alloc = bslma::Default::allocator();
    bdlbb::Blob       blob(alloc);

    const int numBuffers = provider.ConsumeIntegralInRange(0, k_MAX_BUFFERS);
    for (int i = 0; i < numBuffers; ++i) {
        const int bufSize = provider.ConsumeIntegralInRange(1, k_MAX_BUF_SIZE);
        bsl::shared_ptr<char> bufData(
            static_cast<char*>(alloc->allocate(bufSize)),
            alloc);
        const std::vector<uint8_t> bytes = provider.ConsumeBytes<uint8_t>(
            bufSize);
        bsl::memcpy(bufData.get(), bytes.data(), bytes.size());
        blob.appendDataBuffer(bdlbb::BlobBuffer(bufData, bufSize));
    }

    blob.setLength(provider.ConsumeIntegralInRange(0, blob.length()));

    const int startBuffer = provider.ConsumeIntegralInRange(-2,
                                                            numBuffers + 2);
    const int startByte   = provider.ConsumeIntegralInRange(-2,
                                                          k_MAX_BUF_SIZE + 2);
    const int length      = provider.ConsumeIntegralInRange(0, k_MAX_LENGTH);
    bsl::vector<char> out(length > 0 ? length : 1, alloc);

    bmqu::BlobUtil::readNBytes(out.data(),
                               blob,
                               bmqu::BlobPosition(startBuffer, startByte),
                               length);

    return 0;
}
