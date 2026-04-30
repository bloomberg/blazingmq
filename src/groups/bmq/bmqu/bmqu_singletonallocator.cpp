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

#include <bmqu_singletonallocator.h>

#include <bmqscm_version.h>

// BDE
#include <bdlma_bufferedsequentialallocator.h>
#include <bdlma_concurrentallocatoradapter.h>
#include <bsl_new.h>
#include <bslma_default.h>
#include <bslmt_mutex.h>
#include <bslmt_once.h>
#include <bsls_objectbuffer.h>

namespace BloombergLP {
namespace bmqu {

// -------------------------
// struct SingletonAllocator
// -------------------------

bslma::Allocator* SingletonAllocator::allocator()
{
    typedef bdlma::BufferedSequentialAllocator SeqAllocType;
    typedef bdlma::ConcurrentAllocatorAdapter  AdapterType;

    static char                             buffer[4096];
    static bsls::ObjectBuffer<bslmt::Mutex> mutex;
    static bsls::ObjectBuffer<SeqAllocType> seqAlloc;
    static bsls::ObjectBuffer<AdapterType>  adapter;
    BSLMT_ONCE_DO
    {
        new (static_cast<void*>(mutex.buffer())) bslmt::Mutex();
        new (static_cast<void*>(seqAlloc.buffer()))
            SeqAllocType(buffer,
                         sizeof(buffer),
                         bslma::Default::globalAllocator());
        new (static_cast<void*>(adapter.buffer()))
            AdapterType(&mutex.object(), &seqAlloc.object());
    }
    return &adapter.object();
}

}  // close package namespace
}  // close enterprise namespace
