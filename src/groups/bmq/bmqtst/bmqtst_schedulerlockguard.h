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

#ifndef INCLUDED_BMQTST_SCHEDULERLOCKGUARD
#define INCLUDED_BMQTST_SCHEDULERLOCKGUARD

//@PURPOSE: Provide an RAII guard that pauses a 'bdlmt::EventScheduler'.
//
//@CLASSES:
//   bmqtst::SchedulerLockGuard: RAII guard pausing a 'bdlmt::EventScheduler'
//
//@DESCRIPTION: 'bmqtst::SchedulerLockGuard' provides an RAII mechanism that
// pauses a 'bdlmt::EventScheduler' for the lifetime of the guard.  While the
// guard is alive, the scheduler's dispatcher thread is parked inside a
// callback so that no scheduled event (timer, watchdog, gc, ...) can fire; the
// thread holding the guard may therefore inspect or mutate state shared with
// scheduler callbacks without racing the scheduler thread.  On destruction the
// scheduler is resumed and all now-due events are drained before the
// destructor returns.
//
// This is primarily useful in tests that drive an object on the main thread
// while a real 'bdlmt::EventScheduler' runs background callbacks that touch
// the same state.
//
/// Thread Safety
///-------------
// A single guard instance is not thread safe; it is intended to be created and
// destroyed by the same thread.  The scheduler supplied at construction must
// be running.
//
/// Warning
///-------
// Do *not* advance a test time source (e.g. via
// 'bdlmt::EventSchedulerTestTimeSource::advanceTime') while a guard is active:
// 'advanceTime' blocks until the dispatcher thread processes the change, but
// that thread is parked, so the call would deadlock.
//
/// Usage Example
///-------------
// The following example illustrates typical intended usage of this component.
//
//..
//  {
//      bmqtst::SchedulerLockGuard guard(scheduler_p);
//
//      // The scheduler is paused here; inspect or mutate state shared with
//      // scheduler callbacks without racing the scheduler thread.
//  }
//  // The scheduler has been resumed and all now-due events drained.
//..

// BDE
#include <bsl_memory.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace bslma {
class Allocator;
}

namespace bmqtst {

// FORWARD DECLARATION

/// Component-private context shared between the pause callback and the
/// `SchedulerLockGuard` that owns it.  Defined in the `.cpp`.
struct SchedulerLockGuard_Context;

// ========================
// class SchedulerLockGuard
// ========================

/// RAII guard that keeps a `bdlmt::EventScheduler` paused for its lifetime.
class SchedulerLockGuard {
  private:
    // DATA

    /// Scheduler paused by this guard.
    bdlmt::EventScheduler* d_scheduler_p;

    /// Context shared with the pause callback; used to release the parked
    /// dispatcher thread on destruction.
    bsl::shared_ptr<SchedulerLockGuard_Context> d_context_sp;

  private:
    // NOT IMPLEMENTED
    SchedulerLockGuard(const SchedulerLockGuard&) BSLS_KEYWORD_DELETED;
    SchedulerLockGuard&
    operator=(const SchedulerLockGuard&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Pause the specified `scheduler` and create a guard that keeps it paused
    /// until this object is destroyed, blocking until the scheduler's
    /// dispatcher thread is parked.  Optionally specify a `basicAllocator`
    /// used to supply memory; if `basicAllocator` is 0, the currently
    /// installed default allocator is used.  The behavior is undefined unless
    /// `scheduler` is running.
    explicit SchedulerLockGuard(bdlmt::EventScheduler* scheduler,
                                bslma::Allocator*      basicAllocator = 0);

    /// Resume the scheduler and drain all now-due events, then destroy this
    /// object.
    ~SchedulerLockGuard();
};

}  // close package namespace
}  // close enterprise namespace

#endif
