// Copyright 2025-2026 Bloomberg Finance L.P.
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

#include <bmqtst_loggingallocator.h>

#include <bmqscm_version.h>

#include <bmqu_memoutstream.h>

// BDE
#include <balst_stacktraceprintutil.h>
#include <bsl_iostream.h>
#include <bsl_string_view.h>
#include <bslma_mallocfreeallocator.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqtst {

namespace {

/// @brief Return whether the specified `stackTrace` belongs to a known
/// allocation.
bool isKnownAllocation(bsl::string_view stackTrace)
{
    const bsl::string_view k_KNOWN[] = {
        "bdlmt::EventSchedulerTestTimeSource::EventSchedulerTestTimeSource",
        "bdls::FilesystemUtil::makeUnsafeTemporaryFilename",
        "bdls::FilesystemUtil::remove",
        "MessageGUIDUtil::initialize"};

    const size_t k_NUM_KNOWN = sizeof(k_KNOWN) / sizeof(*k_KNOWN);
    for (size_t i = 0; i < k_NUM_KNOWN; ++i) {
        if (stackTrace.find(k_KNOWN[i]) != bsl::string_view::npos) {
            return true;  // RETURN
        }
    }
    return false;
}

}  // close unnamed namespace

// ----------------------
// class LoggingAllocator
// ----------------------

// CREATORS
LoggingAllocator::LoggingAllocator(bslma::Allocator* allocator)
: d_allocator_p(allocator)
{
    BSLS_ASSERT_OPT(d_allocator_p);
}

LoggingAllocator::~LoggingAllocator()
{
    // NOTHING
}

// MANIPULATORS
void* LoggingAllocator::allocate(size_type size)
{
    // Do not use the test allocators, to keep their counters unchanged.
    bmqu::MemOutStream os(&bslma::MallocFreeAllocator::singleton());
    balst::StackTracePrintUtil::printStackTrace(os);

    if (!isKnownAllocation(os.str())) {
        bsl::cerr << os.str() << bsl::endl;
    }

    return d_allocator_p->allocate(size);
}

void LoggingAllocator::deallocate(void* address)
{
    d_allocator_p->deallocate(address);
}

}  // close package namespace
}  // close enterprise namespace
