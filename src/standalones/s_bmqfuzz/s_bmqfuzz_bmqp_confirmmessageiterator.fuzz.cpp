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

#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bslma_default.h>

#include <bmqp_confirmmessageiterator.h>
#include <bmqp_protocol.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < sizeof(bmqp::EventHeader)) {
        return 0;
    }

    bslma::Allocator*              alloc = bslma::Default::defaultAllocator();
    bdlbb::PooledBlobBufferFactory bufferFactory(1024, alloc);

    bdlbb::Blob blob(&bufferFactory, alloc);
    bdlbb::BlobUtil::append(&blob,
                            reinterpret_cast<const char*>(data),
                            static_cast<int>(size));

    bmqp::EventHeader eventHeader;
    bsl::memcpy(&eventHeader, data, sizeof(bmqp::EventHeader));

    bmqp::ConfirmMessageIterator iter;
    int                          rc = iter.reset(&blob, eventHeader);
    if (rc != 0) {
        return 0;
    }

    while (iter.next() == 1) {
        iter.header();
        iter.message();
    }

    return 0;
}
