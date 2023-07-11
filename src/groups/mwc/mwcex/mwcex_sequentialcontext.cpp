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

// mwcex_sequentialcontext.cpp                                        -*-C++-*-
#include <mwcex_sequentialcontext.h>

#include <mwcscm_version.h>
// MWC
#include <mwcsys_threadutil.h>

// BDE
#include <bdlf_memfn.h>
#include <bsl_algorithm.h>  // bsl::swap
#include <bslmt_threadattributes.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mwcex {

// -----------------------------------
// struct SequentialContext_ThreadUtil
// -----------------------------------

// CLASS METHODS
bsls::Types::Uint64
SequentialContext_ThreadUtil::invalidThreadId() BSLS_KEYWORD_NOEXCEPT
{
    return bslmt::ThreadUtil::idAsUint64(
        bslmt::ThreadUtil::handleToId(bslmt::ThreadUtil::invalidHandle()));
}

// -----------------------
// class SequentialContext
// -----------------------

// PRIVATE MANIPULATORS
void SequentialContext::run() BSLS_NOTHROW_SPEC
{
    // NOTE: 'BSLS_NOTHROW_SPEC' specification ensures that 'bsl::terminate' is
    //       called in case of exception in C++03.

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    while (!d_doStop && (!d_doJoin || !d_jobQueue.empty())) {
        if (!d_jobQueue.empty()) {
            // pick next job
            Job& job = d_jobQueue.front();

            // set the 'd_isExecuting' flag, so the job won't get removed from
            // the queue by 'dropPendingJobs'
            d_isExecuting = true;

            // execute the job outside the lock
            d_mutex.unlock();  // UNLOCK
            job();
            d_mutex.lock();  // LOCK

            // unset the 'd_isExecuting' flag
            d_isExecuting = false;

            // remove the job from the queue
            d_jobQueue.pop_front();

            continue;  // CONTINUE
        }

        // wait for an event
        d_condition.wait(&d_mutex);  // UNLOCK / LOCK
    }

    // unset the working thread id, as the same id may be reused by the OS
    // after the thread terminates
    d_workingThreadId.storeRelease(
        SequentialContext_ThreadUtil::invalidThreadId());
}

// CREATORS
SequentialContext::SequentialContext(bslma::Allocator* basicAllocator)
: d_mutex()
, d_condition()
, d_workingThread(bslmt::ThreadUtil::invalidHandle())
, d_workingThreadId(SequentialContext_ThreadUtil::invalidThreadId())
, d_isExecuting(false)
, d_doStop(false)
, d_doJoin(false)
, d_jobQueue(basicAllocator)
{
    // NOTHING
}

SequentialContext::~SequentialContext()
{
    stop();
    join();
}

// MANIPULATORS
int SequentialContext::start()
{
    return start(mwcsys::ThreadUtil::defaultAttributes());
}

int SequentialContext::start(const bslmt::ThreadAttributes& threadAttributes)
{
    // PRECONDITIONS
    BSLS_ASSERT(threadAttributes.detachedState() ==
                bslmt::ThreadAttributes::e_CREATE_JOINABLE);

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    // PRECONDITIONS
    BSLS_ASSERT(d_workingThreadId.loadAcquire() ==
                SequentialContext_ThreadUtil::invalidThreadId());

    // NOTE: The value of 'd_workingThreadId' is more reliable that the one of
    //       'd_workingThread' when checking that the working thread is not
    //       running, as the latter is invalidated in 'join' before the thread
    //       completes.

    // spawn the working thread
    if (bslmt::ThreadUtil::createWithAllocator(
            &d_workingThread,
            threadAttributes,
            bdlf::MemFnUtil::memFn(&SequentialContext::run, this),
            allocator()) != 0) {
        // Failed to create the thread.
        return -1;  // RETURN
    }

    // set the working thread id
    d_workingThreadId.storeRelease(bslmt::ThreadUtil::idAsUint64(
        bslmt::ThreadUtil::handleToId(d_workingThread)));

    // keep the working thread alive
    d_doStop = false;
    d_doJoin = false;

    // success
    return 0;
}

void SequentialContext::stop() BSLS_KEYWORD_NOEXCEPT
{
    {
        bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

        // tell the working thread to stop
        d_doStop = true;
    }  // UNLOCK

    // wake the working thread
    d_condition.signal();
}

void SequentialContext::join() BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(d_workingThreadId.loadAcquire() !=
                bslmt::ThreadUtil::selfIdAsUint64());

    bslmt::ThreadUtil::Handle workingThread;
    {
        bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

        if (d_workingThread == bslmt::ThreadUtil::invalidHandle()) {
            // The working thread is already joined, or is being joinded. Do
            // nothing.
            return;  // RETURN
        }

        // tell the working thread to stop when there is no more work
        d_doJoin = true;

        // invalidate the thread handle, so subsequent invocations of 'join'
        // complete immediately
        workingThread   = d_workingThread;
        d_workingThread = bslmt::ThreadUtil::invalidHandle();
    }  // UNLOCK

    // wake the working thread
    d_condition.signal();

    // join the thread
    bslmt::ThreadUtil::join(workingThread);
}

size_t SequentialContext::dropPendingJobs() BSLS_KEYWORD_NOEXCEPT
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    const size_t jobsToDrop = d_jobQueue.empty() ? 0
                                                 : d_jobQueue.size() -
                                                       (d_isExecuting ? 1 : 0);

    for (size_t i = 0; i < jobsToDrop; ++i) {
        // NOTE: Can't call 'erase' because it requires elements to be
        //       moveable, so we 'pop_back'.
        d_jobQueue.pop_back();
    }

    return jobsToDrop;
}

// ACCESSORS
size_t SequentialContext::outstandingJobs() const BSLS_KEYWORD_NOEXCEPT
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    return d_jobQueue.size();
}

SequentialContext::ExecutorType
SequentialContext::executor() const BSLS_KEYWORD_NOEXCEPT
{
    return ExecutorType(const_cast<SequentialContext*>(this));
}

bslma::Allocator* SequentialContext::allocator() const BSLS_KEYWORD_NOEXCEPT
{
    return d_jobQueue.get_allocator().mechanism();
}

// -------------------------------
// class SequentialContextExecutor
// -------------------------------

// PRIVATE CREATORS
SequentialContextExecutor::SequentialContextExecutor(ContextType* context)
    BSLS_KEYWORD_NOEXCEPT : d_context_p(context)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);
}

// MANIPULATORS
void SequentialContextExecutor::swap(SequentialContextExecutor& other)
    BSLS_KEYWORD_NOEXCEPT
{
    bsl::swap(d_context_p, other.d_context_p);
}

// ACCESSORS
bool SequentialContextExecutor::runningInThisThread() const
    BSLS_KEYWORD_NOEXCEPT
{
    return d_context_p->d_workingThreadId.loadAcquire() ==
           bslmt::ThreadUtil::selfIdAsUint64();
}

SequentialContextExecutor::ContextType&
SequentialContextExecutor::context() const BSLS_KEYWORD_NOEXCEPT
{
    return *d_context_p;
}

}  // close package namespace
}  // close enterprise namespace
