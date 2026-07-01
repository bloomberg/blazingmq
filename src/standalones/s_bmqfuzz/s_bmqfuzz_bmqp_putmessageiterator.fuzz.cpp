// Copyright 2025 Bloomberg Finance L.P.
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

#include <bmqp_putmessageiterator.h>

#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_cstring.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < sizeof(bmqp::EventHeader) + 1) {
        return 0;
    }

    bool decompressFlag = (data[0] & 1);
    data++;
    size--;

    bslma::Allocator*              alloc = bslma::Default::defaultAllocator();
    bdlbb::PooledBlobBufferFactory bufferFactory(1024, alloc);

    bdlbb::Blob blob(&bufferFactory, alloc);
    bdlbb::BlobUtil::append(&blob,
                            reinterpret_cast<const char*>(data),
                            static_cast<int>(size));

    bmqp::EventHeader eventHeader;
    bsl::memcpy(&eventHeader, data, sizeof(bmqp::EventHeader));

    bmqp::PutMessageIterator iter(&bufferFactory, alloc);
    int rc = iter.reset(&blob, eventHeader, decompressFlag);
    if (rc != 0) {
        return 0;
    }

    while (iter.next() == 1) {
        iter.header();
        iter.applicationDataSize();

        if (iter.hasOptions()) {
            bdlbb::Blob optBlob(&bufferFactory, alloc);
            iter.loadOptions(&optBlob);
        }

        if (decompressFlag && iter.hasMessageProperties()) {
            bdlbb::Blob propsBlob(&bufferFactory, alloc);
            iter.loadMessageProperties(&propsBlob);
        }
    }

    return 0;
}
