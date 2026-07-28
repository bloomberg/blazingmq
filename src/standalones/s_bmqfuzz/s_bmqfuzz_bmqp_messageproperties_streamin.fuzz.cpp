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

#include <bmqp_messageproperties.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < sizeof(bmqp::MessagePropertiesHeader) + 1) {
        return 0;
    }

    bool isNewStyle = (data[0] & 1);
    data++;
    size--;

    bslma::Allocator*              alloc = bslma::Default::defaultAllocator();
    bdlbb::PooledBlobBufferFactory bufferFactory(1024, alloc);

    bdlbb::Blob blob(&bufferFactory, alloc);
    bdlbb::BlobUtil::append(&blob,
                            reinterpret_cast<const char*>(data),
                            static_cast<int>(size));

    bmqp::MessageProperties props(alloc);
    int                     rc = props.streamIn(blob, isNewStyle);
    if (rc != 0) {
        return 0;
    }

    props.numProperties();
    props.totalSize();

    return 0;
}
