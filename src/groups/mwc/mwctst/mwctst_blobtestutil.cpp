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

// mwctst_blobtestutil.cpp                                            -*-C++-*-
#include <mwctst_blobtestutil.h>

// BDE
#include <bsl_algorithm.h>
#include <bsl_cstring.h>  // for 'bsl::memcpy'
#include <bsl_iterator.h>
#include <bsl_memory.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mwctst {

// -------------------
// struct BlobTestUtil
// -------------------

// CLASS METHODS
bdlbb::Blob& BlobTestUtil::fromString(bdlbb::Blob*             blob,
                                      const bslstl::StringRef& format,
                                      bslma::Allocator*        allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);

    blob->removeAll();  // clear the blob

    if (format.empty()) {
        // Empty blob
        return *blob;  // RETURN
    }

    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    size_t idx = 0;
    while (idx < format.size()) {
        const char* dataStart = format.data() + idx;
        while ((idx < format.size()) && format[idx] != '|') {
            ++idx;
        }

        const size_t length = (format.data() + idx) - dataStart;
        char* bufData       = reinterpret_cast<char*>(alloc->allocate(length));
        bsl::memcpy(bufData, dataStart, length);
        bsl::shared_ptr<char> buffer;
        buffer.reset(bufData, alloc);
        bdlbb::BlobBuffer blobBuffer(buffer, length);
        blob->appendDataBuffer(blobBuffer);

        if ((idx < format.size()) && format[idx] == '|') {
            ++idx;
        }
    }

    // The 'idx' is now at the end of the string, and we know that the string
    // was not empty, so iterate backward to count the 'X'.
    int numX = 0;
    while ((idx > 0) && format[--idx] == 'X') {
        ++numX;
    }
    if (numX != 0) {
        blob->setLength(blob->length() - numX);
    }

    return *blob;
}

bsl::string& BlobTestUtil::toString(bsl::string*       str,
                                    const bdlbb::Blob& blob,
                                    bool               toFormat)
{
    str->reserve(blob.length());

    for (int bufferIdx = 0; bufferIdx < blob.numDataBuffers(); ++bufferIdx) {
        // NOTE: to avoid dependency on 'mwcu' package, inline implementation
        // of 'mwcu::BlobUtil::bufferSize' routine.
        const int bufferSize = bufferIdx == blob.numDataBuffers() - 1
                                   ? blob.lastDataBufferLength()
                                   : blob.buffer(bufferIdx).size();

        bsl::copy(blob.buffer(bufferIdx).data(),
                  blob.buffer(bufferIdx).data() + bufferSize,
                  bsl::back_inserter(*str));
        if (toFormat && (bufferIdx < blob.numDataBuffers() - 1)) {
            str->push_back('|');
        }
    }

    if (blob.length() < blob.totalSize()) {
        str->append(blob.totalSize() - blob.length(), 'X');
    }

    return *str;
}

}  // close package namespace
}  // close enterprise namespace
