// Copyright 2014-2025 Bloomberg Finance L.P.
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

// bmqtst_loggingallocator.cpp                                        -*-C++-*-
#include <bmqtst_loggingallocator.h>

#include <bmqscm_version.h>

#include <bmqu_memoutstream.h>

// BDE
#include <balst_stacktraceprintutil.h>  // balst::StackTracePrintUtil
#include <bsl_iostream.h>               // bsl::cerr

namespace BloombergLP {
namespace bmqtst {

namespace {

static bool isStackInExceptions(const bslstl::StringRef& str)
{
    return (str.find("bdlmt::EventSchedulerTestTimeSource::"
                     "EventSchedulerTestTimeSource") != bsl::string::npos ||
            str.find("bdls::FilesystemUtil::makeUnsafeTemporaryFilename") !=
                bsl::string::npos ||
            str.find("bdls::FilesystemUtil::remove") != bsl::string::npos);
}

}

// ----------------------
// class LoggingAllocator
// ----------------------

// CREATORS

LoggingAllocator::LoggingAllocator(bslma::Allocator* allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
{
    // NOTHING
}

LoggingAllocator::~LoggingAllocator()
{
    // NOTHING
}

// MANIPULATORS

void* LoggingAllocator::allocate(size_type size)
{
    bmqu::MemOutStream os(d_allocator_p);
    balst::StackTracePrintUtil::printStackTrace(os);

    if (!isStackInExceptions(os.str())) {
        bsl::cerr << os.str() << bsl::endl;
        BSLS_ASSERT_OPT(false);
    }
    return d_allocator_p->allocate(size);
}

void LoggingAllocator::deallocate(void* address)
{
    d_allocator_p->deallocate(address);
}

}  // close package namespace
}  // close enterprise namespace
