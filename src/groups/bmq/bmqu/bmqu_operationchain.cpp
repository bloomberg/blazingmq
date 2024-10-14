// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqu_operationchain.cpp                                            -*-C++-*-
#include <bmqu_operationchain.h>

#include <bmqscm_version.h>

// BDE
#include <bsl_iterator.h>
#include <bslmt_mutexassert.h>

namespace BloombergLP {
namespace bmqu {

// ------------------------------------
// class OperationChain_Job::TargetBase
// ------------------------------------

// CREATORS
OperationChain_Job::TargetBase::~TargetBase()
{
    // NOTHING
}

// --------------------
// class OperationChain
// --------------------

// PRIVATE MANIPULATORS
void OperationChain::onOperationCompleted(JobHandle handle)
    BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK

    // PRECONDITIONS
    BSLS_ASSERT(d_numLinks != 0);
    BSLS_ASSERT(d_numJobsRunning != 0);

    // remove job for completed operation from the list
    d_jobList.erase(handle);

    // decrement the number of running jobs in this chain
    if (--d_numJobsRunning != 0) {
        return;  // RETURN
    }

    // decrement the number of links in this chain and continue executing
    // operations, given the chain is started and is not empty
    if (--d_numLinks != 0 && d_isStarted) {
        run(&lock);  // UNLOCK
        return;      // RETURN
    }

    // notify all waiting threads
    d_condition.broadcast();
}

void OperationChain::run(LockGuard* lock) BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(lock);
    BSLMT_MUTEXASSERT_IS_LOCKED(&d_mutex);
    BSLS_ASSERT(d_isStarted);
    BSLS_ASSERT(d_numLinks != 0);
    BSLS_ASSERT(d_numJobsRunning == 0);
    BSLS_ASSERT(!d_jobList.empty());
    BSLS_ASSERT(d_jobList.size() >= d_jobList.front().id());

    // set the number of running jobs in this chain
    d_numJobsRunning = d_jobList.front().id();

    unsigned          numJobs = d_numJobsRunning;
    JobList::iterator jobIt   = d_jobList.begin();

    lock->release()->unlock();  // UNLOCK

    // run jobs
    for (; numJobs != 0; --numJobs) {
        // NOTE: Running jobs may be removing themselves from the job list
        //       while this loop is still running.

        // Get job handle. Note that in this implementation the job handle and
        // the job list iterator are the same thing.
        JobHandle jobHandle = jobIt;

        // Increment job iterator, but only if it doesn't refer to the last
        // job. The reason being that a splice operation may be in progress on
        // the job list, meaning the "next" pointers in the list node is
        // possibly being modified, so we don't want to read from that pointer
        // (even though in this case that should be safe because the read data
        // is not used). Note also that we increment the iterator *before*
        // executing the job. That is because as soon as the job starts
        // executing the iterator may become invalid.
        if (numJobs > 1) {
            ++jobIt;
        }

        // execute the job
        jobHandle->execute(this, jobHandle);
    }
}

// CREATORS
OperationChain::OperationChain(bslma::Allocator* basicAllocator)
: d_mutex()
, d_condition()
, d_isStarted(false)
, d_numLinks(0)
, d_numJobsRunning(0)
, d_jobList(basicAllocator)
{
    // NOTHING
}

OperationChain::OperationChain(bool              createStarted,
                               bslma::Allocator* basicAllocator)
: d_mutex()
, d_condition()
, d_isStarted(createStarted)
, d_numLinks(0)
, d_numJobsRunning(0)
, d_jobList(basicAllocator)
{
    // NOTHING
}

OperationChain::~OperationChain()
{
    stop();
    join();
}

// MANIPULATORS
void OperationChain::start()
{
    LockGuard lock(&d_mutex);  // LOCK

    // started
    d_isStarted = true;

    if (d_numJobsRunning != 0 || d_numLinks == 0) {
        // The chain is already executing operations, or there is no operation
        // to execute. Either way, do nothing.
        return;  // RETURN
    }

    // start executing operations
    run(&lock);  // UNLOCK
}

void OperationChain::stop() BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK

    // stopped
    d_isStarted = false;
}

void OperationChain::join() BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK
    for (; d_numJobsRunning != 0; d_condition.wait(&d_mutex))
        ;  // LOCK/UNLOCK
}

void OperationChain::append(Link* const* links, size_t count)
{
    // PRECONDITIONS
    BSLS_ASSERT(links || count == 0);

    if (count == 0) {
        // Nothing to do.
        return;  // RETURN
    }

    LockGuard lock(&d_mutex);  // LOCK

    // the number of links before append
    const unsigned prevNumLinks = d_numLinks;

    for (; count != 0; ++links, --count) {
        // make sure the link is using the same allocator as this chain
        BSLS_ASSERT((*links) && (*links)->allocator() == allocator());

        // if the link is empty, skip it
        if ((*links)->numOperations() == 0) {
            continue;  // CONTINUE
        }

        // increment the number of links in this chain
        ++d_numLinks;

        // transfer ownership of all jobs to this chain
        d_jobList.splice(d_jobList.end(), (*links)->d_jobList);
    }

    // if the chain is started and the first appended link is the first one in
    // the chain, start executing operations right away
    if (d_isStarted && d_numLinks != 0 && prevNumLinks == 0) {
        run(&lock);  // UNLOCK
    }
}

int OperationChain::popBack() BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK

    if (d_numLinks == 0 || (d_numJobsRunning != 0 && d_numLinks == 1)) {
        return -1;  // RETURN
    }

    // check invariants
    BSLS_ASSERT_SAFE(d_numLinks != 0);
    BSLS_ASSERT_SAFE(!d_jobList.empty());
    BSLS_ASSERT_SAFE(d_jobList.back().id() == 1);

    // decrement the number of links in this chain
    --d_numLinks;

    // remove jobs from this chain
    unsigned jobId     = 1;
    unsigned prevJobId = 0;
    for (; !d_jobList.empty() && jobId > prevJobId;) {
        d_jobList.pop_back();

        prevJobId = jobId;
        jobId     = d_jobList.empty() ? 0 : d_jobList.back().id();
    }

    return 0;
}

int OperationChain::popBack(Link* link) BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(link);
    BSLS_ASSERT(link->allocator() == allocator());

    LockGuard lock(&d_mutex);  // LOCK

    if (d_numLinks == 0 || (d_numJobsRunning != 0 && d_numLinks == 1)) {
        return -1;  // RETURN
    }

    // check invariants
    BSLS_ASSERT_SAFE(d_numLinks != 0);
    BSLS_ASSERT_SAFE(!d_jobList.empty());
    BSLS_ASSERT_SAFE(d_jobList.back().id() == 1);

    // decrement the number of links in this chain
    --d_numLinks;

    // discard current contents of recipient link
    link->d_jobList.clear();

    // transfer jobs ownership from this chain to recipient link
    unsigned jobId     = 1;
    unsigned prevJobId = 0;
    for (; !d_jobList.empty() && jobId > prevJobId;) {
        link->d_jobList.splice(link->d_jobList.end(),
                               d_jobList,
                               --d_jobList.end(),
                               d_jobList.end());

        prevJobId = jobId;
        jobId     = d_jobList.empty() ? 0 : d_jobList.back().id();
    }

    return 0;
}

size_t OperationChain::removeAll() BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK

    // check invariants
    BSLS_ASSERT_SAFE(d_jobList.size() >= d_numJobsRunning);
    BSLS_ASSERT_SAFE(d_numJobsRunning == 0 || d_numLinks != 0);

    // number of jobs removed
    size_t numJobs = d_jobList.size() - d_numJobsRunning;

    // reset the number of links in this chain
    d_numLinks = (d_numJobsRunning == 0) ? 0 : 1;

    // remove all jobs (except the ones that are currently running)
    for (size_t i = 0; i < numJobs; ++i) {
        d_jobList.pop_back();
    }

    return numJobs;
}

// ACCESSORS
bool OperationChain::isStarted() const BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK
    return d_isStarted;
}

bool OperationChain::isRunning() const BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK
    return d_numJobsRunning != 0;
}

size_t OperationChain::numLinks() const BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK
    return d_numLinks;
}

size_t OperationChain::numOperations() const BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK
    return d_jobList.size();
}

size_t OperationChain::numOperationsPending() const BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK
    return d_jobList.size() - d_numJobsRunning;
}

size_t OperationChain::numOperationsExecuting() const BSLS_KEYWORD_NOEXCEPT
{
    LockGuard lock(&d_mutex);  // LOCK
    return d_numJobsRunning;
}

bslma::Allocator* OperationChain::allocator() const BSLS_KEYWORD_NOEXCEPT
{
    return d_jobList.get_allocator().mechanism();
}

// ------------------------
// class OperationChainLink
// ------------------------

// CREATORS
OperationChainLink::OperationChainLink(bslma::Allocator* basicAllocator)
: d_jobList(basicAllocator)
{
    // NOTE: This constructor is allowed to allocate memory, and potentially
    //       can throw.
}

OperationChainLink::OperationChainLink(
    bslmf::MovableRef<OperationChainLink> original)
: d_jobList(bslmf::MovableRefUtil::move(
      bslmf::MovableRefUtil::access(original).d_jobList))
{
    // NOTE: This constructor is *not* noexcept because the move constructor of
    //       'bsl::list' is allowed to throw.
}

// MANIPULATORS
OperationChainLink& OperationChainLink::operator=(
    bslmf::MovableRef<OperationChainLink> rhs) BSLS_KEYWORD_NOEXCEPT
{
    // l-value reference to the link object
    OperationChainLink& rhsRef = bslmf::MovableRefUtil::access(rhs);

    // PRECONDITIONS
    BSLS_ASSERT(rhsRef.allocator() == allocator());

    // check for self assignment
    if (this == &rhsRef) {
        return *this;  // RETURN
    }

    // discard current contents and move contents from the other link
    d_jobList.clear();
    d_jobList.splice(d_jobList.end(), rhsRef.d_jobList);

    return *this;
}

size_t OperationChainLink::removeAll() BSLS_KEYWORD_NOEXCEPT
{
    size_t count = d_jobList.size();
    d_jobList.clear();

    return count;
}

void OperationChainLink::swap(OperationChainLink& other) BSLS_KEYWORD_NOEXCEPT
{
    // NOTE: This method is implemented this way because 'swap' does not work
    //       on 'JobList' - the code does not compile due to 'Job' not being a
    //       copyable type, and although the copy constructor should not be
    //       used, 'bsl::list::swap' is allowed to fallback to copy in some
    //       situations.

    // l-value references to both link objects
    OperationChainLink& lhs = *this;
    OperationChainLink& rhs = other;

    // PRECONDITIONS
    BSLS_ASSERT(lhs.allocator() == rhs.allocator());

    // move jobs from 'rhs' to 'lhs'
    size_t lhsNumJobs = lhs.d_jobList.size();
    lhs.d_jobList.splice(lhs.d_jobList.end(), rhs.d_jobList);

    // move jobs from 'lhs' to 'rhs'
    JobList::iterator first = d_jobList.begin();
    JobList::iterator last  = d_jobList.begin();
    bsl::advance(last, lhsNumJobs);
    rhs.d_jobList.splice(rhs.d_jobList.end(), lhs.d_jobList, first, last);
}

// ACCESSORS
size_t OperationChainLink::numOperations() const BSLS_KEYWORD_NOEXCEPT
{
    return d_jobList.size();
}

bslma::Allocator* OperationChainLink::allocator() const BSLS_KEYWORD_NOEXCEPT
{
    return d_jobList.get_allocator().mechanism();
}

}  // close package namespace
}  // close enterprise namespace
