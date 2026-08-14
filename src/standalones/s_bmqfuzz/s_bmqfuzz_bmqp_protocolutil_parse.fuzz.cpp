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
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include <bmqp_protocolutil.h>
#include <bmqt_compressionalgorithmtype.h>

#include <bmqu_blob.h>

#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>

#include <bslma_default.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);

    const int  bufferSize     = provider.ConsumeIntegralInRange<int>(1, 256);
    const bool decompressFlag = provider.ConsumeBool();
    const bool haveMessageProperties    = provider.ConsumeBool();
    const bool haveNewMessageProperties = provider.ConsumeBool();
    const bool separateMPs              = provider.ConsumeBool();

    static const bmqt::CompressionAlgorithmType::Enum k_CATS[] = {
        bmqt::CompressionAlgorithmType::e_UNKNOWN,
        bmqt::CompressionAlgorithmType::e_NONE,
        bmqt::CompressionAlgorithmType::e_ZLIB};
    const bmqt::CompressionAlgorithmType::Enum cat =
        k_CATS[provider.ConsumeIntegralInRange<size_t>(0, 2)];

    const unsigned int offsetSeed = provider.ConsumeIntegral<unsigned int>();
    const unsigned int lengthSeed = provider.ConsumeIntegral<unsigned int>();

    bslma::Allocator*              alloc = bslma::Default::defaultAllocator();
    bdlbb::PooledBlobBufferFactory bufferFactory(bufferSize, alloc);

    const std::vector<uint8_t> bytes =
        provider.ConsumeRemainingBytes<uint8_t>();

    bdlbb::Blob input(&bufferFactory, alloc);
    bdlbb::BlobUtil::append(&input,
                            reinterpret_cast<const char*>(bytes.data()),
                            static_cast<int>(bytes.size()));

    const int blobLength = input.length();
    const int offset     = static_cast<int>(offsetSeed % (blobLength + 1));

    bmqu::BlobPosition position;
    if (bmqu::BlobUtil::findOffsetSafe(&position, input, offset) != 0) {
        return 0;
    }

    const int length = static_cast<int>(lengthSeed %
                                        (blobLength - offset + 1));

    bdlbb::Blob dataOutput(&bufferFactory, alloc);
    bdlbb::Blob messagePropertiesOutput(&bufferFactory, alloc);
    int         messagePropertiesSize = 0;

    bmqp::ProtocolUtil::parse(separateMPs ? &messagePropertiesOutput : 0,
                              &messagePropertiesSize,
                              &dataOutput,
                              input,
                              length,
                              decompressFlag,
                              position,
                              haveMessageProperties,
                              haveNewMessageProperties,
                              cat,
                              &bufferFactory,
                              alloc);

    return 0;
}
