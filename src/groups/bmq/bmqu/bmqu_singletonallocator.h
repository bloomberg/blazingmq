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

#ifndef INCLUDED_BMQU_SINGLETONALLOCATOR
#define INCLUDED_BMQU_SINGLETONALLOCATOR

//@PURPOSE: Provide a process-lifetime allocator for static initialization.
//
//@CLASSES:
//  bmqu::SingletonAllocator: accessor to a process-lifetime allocator
//
//@DESCRIPTION: This component provides a utility struct,
// 'bmqu::SingletonAllocator', exposing a single static method 'allocator()'
// that returns a pointer to a process-lifetime
// 'bdlma::BufferedSequentialAllocator'.  The allocator is lazily initialized
// on first call in a thread-safe manner and is backed by a fixed-size static
// buffer.  It is intended for one-time static initialization of lightweight
// objects (e.g., shared_ptr control blocks) that live for the duration of the
// process and should not allocate from the global allocator.

// BDE
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqu {

// =========================
// struct SingletonAllocator
// =========================

/// Utility struct providing access to a process-lifetime allocator.
struct SingletonAllocator {
    // CLASS METHODS

    /// Return a pointer to a process-lifetime allocator backed by a
    /// fixed-size static buffer.  The allocator is lazily initialized on
    /// first call in a thread-safe manner.
    static bslma::Allocator* allocator();
};

}  // close package namespace
}  // close enterprise namespace

#endif
