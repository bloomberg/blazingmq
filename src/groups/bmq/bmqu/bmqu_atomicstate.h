// Copyright 2020-2023 Bloomberg Finance L.P.
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

// bmqu_atomicstate.h                                                 -*-C++-*-
#ifndef INCLUDED_BMQU_ATOMICSTATE
#define INCLUDED_BMQU_ATOMICSTATE

//@PURPOSE: Provide a mechanism to change a state atomically.
//
//@CLASSES:
//  bmqu::AtomicState: maintain the state
//
//@DESCRIPTION: This mechanism assists sharing a resource between multiple
// clients.  It supports four atomic operations: 'process', 'cancel',
// 'tryLock', and 'unlock'.  'cancel' can be called only once.  'process' can
// be called multiple times (as in the case when processing needs a retry).
// 'process' returns true if 'cancel' has not been called.  'cancel' returns
// 'true' if 'process' has not been called.  'process' will spin until 'unlock'
// if 'tryLock' has succeeded.  This allows canceling a resource before
// 'process' and modifying un-processed and un-canceled resource under lock.
//

// BDE
#include <bsls_assert.h>
#include <bsls_atomic.h>

namespace BloombergLP {
namespace bmqu {

// =============================
// class DispatcherRecoveryEvent
// =============================

class AtomicState {
  private:
    // PRIVATE TYPES
    enum Enum { e_INIT = 0, e_CANCEL = 1, e_LOCK = 2, e_PROCESS = 4 };

    // PRIVATE DATA
    bsls::AtomicInt d_value;

  public:
    // CREATORS
    AtomicState();

    // MANIPULATORS

    /// Return true if `cancel` has not been called.
    bool process();

    /// Return true if `process` has not been called.  The behavior is
    /// undefined if `cancel` is called more than once.
    bool cancel();

    /// Attempt to lock.  Return true if not cancelled or processed.
    bool tryLock();

    /// Unlock.  The behavior is undefined if `tryLock` has not succeeded or
    /// if called more than once.
    void unlock();

    /// Reset the state to the initial value.
    void reset();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class AtomicState
// ---------------------

inline AtomicState::AtomicState()
: d_value(e_INIT)
{
    // NOTHING
}

// MANIPULATORS
inline bool AtomicState::cancel()
{
    const int result = d_value.add(e_CANCEL);

    // Make sure 'cancel' is not called second time.
    BSLS_ASSERT_SAFE((result & e_CANCEL) == e_CANCEL);

    // Return 'true' if not processed ((result & e_PROCESS) == 0)
    return (result & (e_CANCEL + e_PROCESS)) == e_CANCEL;
}

inline bool AtomicState::tryLock()
{
    const int result = d_value.add(e_LOCK);

    // Make sure 'tryLock' is not called second time.
    BSLS_ASSERT_SAFE((result & e_LOCK) == e_LOCK);

    // Return 'true' if not processed ((result & e_PROCESS) == 0)
    // and not cancelled((result & e_CANCEL) == 0)
    if (result == e_LOCK) {
        return true;  // RETURN
    }
    else {
        d_value.subtract(e_LOCK);
        return false;  // RETURN
    }
}

inline void AtomicState::unlock()
{
    const int result = d_value.subtract(e_LOCK);

    BSLS_ASSERT_SAFE(result >= e_INIT);
    (void)result;
}

inline void AtomicState::reset()
{
    d_value.store(e_INIT);
}

}  // close package namespace
}  // close enterprise namespace

#endif
