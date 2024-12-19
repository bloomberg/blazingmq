// Copyright 2024 Bloomberg Finance L.P.
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

// bmqp_blobpoolutil.cpp                                              -*-C++-*-
#include <bmqp_blobpoolutil.h>

#include <bmqscm_version.h>

// BDE
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqp {

namespace {
const int k_BLOB_POOL_GROWTH_STRATEGY = 1024;

/// Create a new blob at the specified `arena` address, using the specified
/// `bufferFactory` and `allocator`.
void createBlob(bdlbb::BlobBufferFactory* bufferFactory,
                void*                     arena,
                bslma::Allocator*         allocator)
{
    new (arena) bdlbb::Blob(bufferFactory, allocator);
}

}  // close unnamed namespace

// ------------------
// class BlobPoolUtil
// ------------------

BlobPoolUtil::BlobSpPool
BlobPoolUtil::createBlobPool(bdlbb::BlobBufferFactory* blobBufferFactory_p,
                             bslma::Allocator*         allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(blobBufferFactory_p);

    return BlobSpPool(
        bdlf::BindUtil::bind(&createBlob,
                             blobBufferFactory_p,
                             bdlf::PlaceHolders::_1,   // arena
                             bdlf::PlaceHolders::_2),  // allocator
        k_BLOB_POOL_GROWTH_STRATEGY,
        bslma::Default::allocator(allocator));
}

}  // close package namespace
}  // close enterprise namespace
