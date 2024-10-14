// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqsys_time.cpp                                                    -*-C++-*-
#include <bmqsys_time.h>

#include <bmqscm_version.h>
// BDE
#include <bdlf_bind.h>
#include <bslma_default.h>
#include <bslmf_allocatorargt.h>
#include <bslmt_qlock.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_objectbuffer.h>
#include <bsls_systemtime.h>
#include <bsls_timeutil.h>

namespace BloombergLP {
namespace bmqsys {

namespace {
bsls::ObjectBuffer<Time::SystemTimeCb> g_realTimeClock;
bsls::ObjectBuffer<Time::SystemTimeCb> g_monotonicClock;

/// Globals holding the various timer callback set up.  Note that an object
/// buffer is needed to avoid exit time destructors because bsl::function is
/// a non-POD type.
bsls::ObjectBuffer<Time::HighResolutionTimeCb> g_highResTimer;

int g_initialized = 0;
// Integer to keep track of the number of
// calls to 'initialize' for the 'Time'.
// If the value is non-zero, then it has
// already been initialized, otherwise it
// can be initialized.  Each call to
// 'initialize' increments the value of
// this integer by one.  Each call to
// 'shutdown' decrements the value of this
// integer by one.  If the decremented
// value is zero, then the 'Time' is
// destroyed.

bslmt::QLock g_initLock = BSLMT_QLOCK_INITIALIZER;
// Lock used to provide thread-safe
// protection for accessing the
// 'g_initialized' counter.
}  // close unnamed namespace

// -----------
// struct Time
// -----------

void Time::initialize(bslma::Allocator* allocator)
{
    // PRECONDITIONS
    bslmt::QLockGuard qlockGuard(&g_initLock);

    // NOTE: We pre-increment here instead of post-incrementing inside the
    //       conditional check below because the post-increment of an int does
    //       not work correctly with versions of IBM xlc12 released following
    //       the Dec 2015 PTF.
    ++g_initialized;
    if (g_initialized > 1) {
        return;  // RETURN
    }

    bslma::Allocator* alloc = bslma::Default::globalAllocator(allocator);

    new (g_realTimeClock.buffer()) SystemTimeCb(
        bsl::allocator_arg,
        alloc,
        bdlf::BindUtil::bind(&bsls::SystemTime::nowRealtimeClock));

    new (g_monotonicClock.buffer()) SystemTimeCb(
        bsl::allocator_arg,
        alloc,
        bdlf::BindUtil::bind(&bsls::SystemTime::nowMonotonicClock));

    new (g_highResTimer.buffer())
        HighResolutionTimeCb(bsl::allocator_arg,
                             alloc,
                             bdlf::BindUtil::bind(&bsls::TimeUtil::getTimer));

    bsls::TimeUtil::initialize();
}

void Time::initialize(const SystemTimeCb&         realTimeClockCb,
                      const SystemTimeCb&         monotonicClockCb,
                      const HighResolutionTimeCb& highResTimeCb,
                      bslma::Allocator*           allocator)
{
    // PRECONDITIONS
    bslmt::QLockGuard qlockGuard(&g_initLock);

    // NOTE: We pre-increment here instead of post-incrementing inside the
    //       conditional check below because the post-increment of an int does
    //       not work correctly with versions of IBM xlc12 released following
    //       the Dec 2015 PTF.
    ++g_initialized;
    if (g_initialized > 1) {
        return;  // RETURN
    }

    bslma::Allocator* alloc = bslma::Default::globalAllocator(allocator);

    new (g_realTimeClock.buffer())
        SystemTimeCb(bsl::allocator_arg, alloc, realTimeClockCb);
    new (g_monotonicClock.buffer())
        SystemTimeCb(bsl::allocator_arg, alloc, monotonicClockCb);
    new (g_highResTimer.buffer())
        HighResolutionTimeCb(bsl::allocator_arg, alloc, highResTimeCb);
}

void Time::shutdown()
{
    bslmt::QLockGuard qlockGuard(&g_initLock);

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized > 0 && "'initialize' was not called");

    if (--g_initialized != 0) {
        return;  // RETURN
    }

    g_realTimeClock.object().~SystemTimeCb();
    g_monotonicClock.object().~SystemTimeCb();
    g_highResTimer.object().~HighResolutionTimeCb();
}

bsls::TimeInterval Time::nowRealtimeClock()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized && "Not initialized");

    return g_realTimeClock.object()();
}

bsls::TimeInterval Time::nowMonotonicClock()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized && "Not initialized");

    return g_monotonicClock.object()();
}

bsls::Types::Int64 Time::highResolutionTimer()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized && "Not initialized");

    return g_highResTimer.object()();
}

}  // close package namespace
}  // close enterprise namespace
