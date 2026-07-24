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

#include <bmqtst_schedulerlockguard.h>

#include <bmqscm_version.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_eventscheduler.h>
#include <bslma_default.h>
#include <bslmt_semaphore.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqtst {

// ---------------------------------
// struct SchedulerLockGuard_Context
// ---------------------------------

/// Context shared between the pause callback (which runs on the scheduler
/// dispatcher thread) and the `SchedulerLockGuard` that owns it.  Co-owned via
/// `shared_ptr` so it outlives whichever of the two ends last.  Defined at
/// package scope (not in the unnamed namespace) to match its forward
/// declaration in the header.
struct SchedulerLockGuard_Context {
    // DATA

    /// Posted by the dispatcher thread once it has entered the pause callback.
    bslmt::Semaphore d_enteredSem;

    /// Latch on which the parked dispatcher thread blocks; posted to release
    /// it.
    bslmt::Semaphore d_releaseSem;
};

namespace {

/// Post on the specified `sem_p`.
void postSemaphore(bslmt::Semaphore* sem_p)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(sem_p);

    sem_p->post();
}

/// Functor scheduled by `SchedulerLockGuard` to park the scheduler's
/// dispatcher thread.  Co-owns the shared context via a `shared_ptr` so it
/// stays alive for the callback's lifetime.
class SchedulerPauseFunctor {
  private:
    // DATA
    bsl::shared_ptr<SchedulerLockGuard_Context> d_context_sp;

  public:
    // CREATORS
    explicit SchedulerPauseFunctor(
        const bsl::shared_ptr<SchedulerLockGuard_Context>& context)
    : d_context_sp(context)
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(d_context_sp);
    }

    // ACCESSORS

    /// Run on the scheduler dispatcher thread: signal that the dispatcher is
    /// parked, then block on the release latch until the owning guard is
    /// destroyed.  The scheduler's internal mutex is *not* held while this
    /// runs, so other threads may still (re)schedule events; those events
    /// simply will not fire until this returns.
    void operator()() const
    {
        d_context_sp->d_enteredSem.post();
        d_context_sp->d_releaseSem.wait();
    }
};

}  // close unnamed namespace

// ------------------------
// class SchedulerLockGuard
// ------------------------

SchedulerLockGuard::SchedulerLockGuard(bdlmt::EventScheduler* scheduler,
                                       bslma::Allocator*      basicAllocator)
: d_scheduler_p(scheduler)
, d_context_sp(
      bsl::allocate_shared<SchedulerLockGuard_Context>(basicAllocator))
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_scheduler_p);

    // Schedule a callback which parks the scheduler's dispatcher thread, and
    // block until that thread confirms it is parked.  Once parked, no other
    // scheduled event can fire until this guard is destroyed.
    d_scheduler_p->scheduleEvent(d_scheduler_p->now(),
                                 SchedulerPauseFunctor(d_context_sp));
    d_context_sp->d_enteredSem.wait();
}

SchedulerLockGuard::~SchedulerLockGuard()
{
    // Release the parked dispatcher thread, then wait until the scheduler has
    // drained all now-due events (including any event that became due while
    // the scheduler was paused).
    d_context_sp->d_releaseSem.post();

    bslmt::Semaphore drainSem;
    d_scheduler_p->scheduleEvent(d_scheduler_p->now(),
                                 bdlf::BindUtil::bind(&postSemaphore,
                                                      &drainSem));
    drainSem.wait();
}

}  // close package namespace
}  // close enterprise namespace
