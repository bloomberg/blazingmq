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

// bmqp_blobpoolutil.h                                                -*-C++-*-
#ifndef INCLUDED_BMQP_BLOBPOOLUTIL
#define INCLUDED_BMQP_BLOBPOOLUTIL

//@PURPOSE: Provide a mechanism to build Blob shared pointer pools compatible
//  with event builders used in BlazingMQ.
//
//@CLASSES:
//  bmqp::BlobPoolUtil: mechanism to build bdlbb::Blob shared pointer pool.
//
//@DESCRIPTION:
//

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_sharedobjectpool.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqp {

struct BlobPoolUtil {
    // TYPES

    /// Pool of shared pointers to Blobs
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

    // CLASS METHODS

    /// Create a blob shared pointer pool, using the specified
    /// `blobBufferFactory_p`.  Use the optionally specified `allocator`
    /// for memory allocations.
    static BlobSpPool
    createBlobPool(bdlbb::BlobBufferFactory* blobBufferFactory_p,
                   bslma::Allocator*         allocator = 0);
};

}  // close package namespace
}  // close enterprise namespace

#endif
