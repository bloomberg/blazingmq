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
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>
#include <bmqu_memoutstream.h>
#include <bslma_default.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 2) {
        return 0;
    }

    const bmqp::EncodingType::Enum encodingType =
        (data[0] & 1) ? bmqp::EncodingType::e_JSON : bmqp::EncodingType::e_BER;

    const int bufferSize = 1 + (data[1] % 256);

    data += 2;
    size -= 2;

    bslma::Allocator*              alloc = bslma::Default::defaultAllocator();
    bdlbb::PooledBlobBufferFactory bufferFactory(bufferSize, alloc);

    bdlbb::Blob blob(&bufferFactory, alloc);
    bdlbb::BlobUtil::append(&blob,
                            reinterpret_cast<const char*>(data),
                            static_cast<int>(size));

    bmqp_ctrlmsg::ControlMessage message(alloc);
    bmqu::MemOutStream           errorDescription(alloc);

    const int rc = bmqp::ProtocolUtil::decodeMessage(errorDescription,
                                                     &message,
                                                     blob,
                                                     0,
                                                     encodingType,
                                                     alloc);
    if (rc != 0) {
        return 0;
    }

    bmqu::MemOutStream printStream(alloc);
    printStream << message;

    return 0;
}
