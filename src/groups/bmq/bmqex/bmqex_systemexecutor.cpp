// Copyright 2018-2023 Bloomberg Finance L.P.
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

#include <bmqex_systemexecutor.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_functional.h>
#include <bslma_default.h>
#include <bslmt_lockguard.h>
#include <bslmt_threadattributes.h>

namespace BloombergLP {
namespace bmqex {

// ----------------------------
// class SystemExecutor_Context
// ----------------------------

// PRIVATE MANIPULATORS
void SystemExecutor_Context::finalizeThread(
    const bslmt::ThreadUtil::Handle& thread) BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(
        bslmt::ThreadUtil::areEqual(thread, bslmt::ThreadUtil::self()));

    bslmt::LockGuard<bslmt::Mutex> lock(&d_threadExitMutex);  // LOCK

    unsigned unjoinedThreads = 1;
    if (d_lastCompletedThread != bslmt::ThreadUtil::invalidHandle()) {
        // We have unjoined threads besides this one.

        // join the last completed thread
        int rc = bslmt::ThreadUtil::join(d_lastCompletedThread);
        BSLS_ASSERT_OPT(rc == 0);

        // decrement unjoined threads counter
        unjoinedThreads = d_unjoinedThreads.subtractAcqRel(1);
    }

    // this is the last completed thread now
    d_lastCompletedThread = thread;

    if (unjoinedThreads == 1) {
        // This is the last unjoined thread. Report that.
        d_threadExitCondition.signal();

        // NOTE: This may be a false-positive report, since another thread may
        //       have been spawned right after the 'd_unjoinedThreads' counter
        //       check.
    }
}

// CREATORS
SystemExecutor_Context::SystemExecutor_Context(bslma::Allocator* allocator)
: d_threadDataAllocator(
      sizeof(SystemExecutor_ThreadData<bsl::function<void()> >),
      allocator)
, d_threadExitMutex()
, d_threadExitCondition()
, d_unjoinedThreads(0)
, d_lastCompletedThread(bslmt::ThreadUtil::invalidHandle())
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);

    // NOTE: The block size for the thread data allocator is an arbitrary
    //       value, large enough to hold an 'SystemExecutor_ThreadData' object
    //       containing a "small" 'bsl::function'-sized functor.
}

SystemExecutor_Context::~SystemExecutor_Context()
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_threadExitMutex);  // LOCK

    if (d_unjoinedThreads != 0) {
        // We have unjoined threads.

        // wait till all threads complete
        while (d_unjoinedThreads != 1 ||
               d_lastCompletedThread == bslmt::ThreadUtil::invalidHandle()) {
            d_threadExitCondition.wait(&d_threadExitMutex);  // UNLOCK / LOCK
        }

        // join the last thread
        int rc = bslmt::ThreadUtil::join(d_lastCompletedThread);
        BSLS_ASSERT_OPT(rc == 0);
    }
}

// ACCESSORS
bslma::Allocator*
SystemExecutor_Context::allocator() const BSLS_KEYWORD_NOEXCEPT
{
    return d_allocator_p;
}

// --------------------
// class SystemExecutor
// --------------------

// CREATORS
SystemExecutor::SystemExecutor(bslma::Allocator* basicAllocator)
: d_context_sp(bsl::allocate_shared<SystemExecutor_Context>(basicAllocator))
, d_threadAttributes()
{
}

SystemExecutor::SystemExecutor(const bslmt::ThreadAttributes& threadAttributes,
                               bslma::Allocator*              basicAllocator)
: d_context_sp(bsl::allocate_shared<SystemExecutor_Context>(basicAllocator))
, d_threadAttributes(
      bsl::allocate_shared<bslmt::ThreadAttributes>(basicAllocator,
                                                    threadAttributes))
{
    // PRECONDITIONS
    BSLS_ASSERT(threadAttributes.detachedState() ==
                bslmt::ThreadAttributes::e_CREATE_JOINABLE);
}

}  // close package namespace

}  // close enterprise namespace
