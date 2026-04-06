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

// mqbc_watchdogcontext.h                                             -*-C++-*-
#ifndef INCLUDED_MQBC_WATCHDOGCONTEXT
#define INCLUDED_MQBC_WATCHDOGCONTEXT

/// @file mqbc_watchdogcontext.h
///
/// @brief Watchdog timer context used by FSM components.
///
/// @bbref{mqbc::WatchdogContext} is a class that holds the context for a
/// single watchdog timer instance, including generation count, active flag,
/// event handle, and retry counter.
///
/// Thread Safety                                {#mqbc_watchdogcontext_thread}
/// =============
///
/// Not thread safe.

// BDE
#include <bdlmt_eventscheduler.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mqbc {

// =====================
// class WatchdogContext
// =====================

/// Per-partition or per-cluster watchdog context.
class WatchdogContext {
  public:
    // DATA

    /// Generation count to detect and ignore stale watchdog triggers.
    /// Should be incremented each time the watchdog is stopped
    bsls::AtomicInt d_generation;

    /// Whether the watchdog timer is currently active.  Since the event
    /// handle does not invalidate upon watchdog firing, we must rely on
    /// this flag.
    bsls::AtomicBool d_active;

    /// Event handle for the scheduled watchdog timer.
    bdlmt::EventSchedulerEventHandle d_eventHandle;

    /// Number of retries remaining before terminating the broker, reset for
    /// each generation.
    bsls::AtomicInt d_retriesRemaining;

    // CREATORS

    /// Create a default `WatchdogContext` with no active timer, zero
    /// generation and zero retries remaining.
    WatchdogContext();

    // NOT IMPLEMENTED
    WatchdogContext(const WatchdogContext&) BSLS_KEYWORD_DELETED;
    WatchdogContext& operator=(const WatchdogContext&) BSLS_KEYWORD_DELETED;
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class WatchdogContext
// ---------------------

// CREATORS
inline WatchdogContext::WatchdogContext()
: d_generation(0)
, d_active(false)
, d_eventHandle()
, d_retriesRemaining(0)
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace

#endif
