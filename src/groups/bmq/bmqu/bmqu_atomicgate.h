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

// bmqu_atomicgate.h                                                  -*-C++-*-
#ifndef INCLUDED_BMQU_ATOMICGATE
#define INCLUDED_BMQU_ATOMICGATE

/// @file bmqu_atomicgate.h
/// @brief Thread-safe binary state (gate) mechanism.

// BDE
#include <bsla_annotations.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqu {

// ================
// class AtomicGate
// ================

/// @brief Thread-safe binary state (open/close) mechanism.
///
/// This mechanism maintains binary state (open/close) allowing efficiently
/// check the state (`tryEnter`/`leave`), change it to open (`open`), and to
/// close (`closeAndDrain`) waiting for all `leave` calls matching `tryEnter`
/// calls.
class AtomicGate {
  private:
    // PRIVATE TYPES

    /// The Enum values have arithmetical meaning and cannot be changed, or
    /// extended, or reordered.
    enum Enum { e_INIT = 0, e_CLOSE = 1, e_ENTER = 2 };

    // PRIVATE DATA
    bsls::AtomicInt d_value;

  public:
    // CREATORS

    /// Create an `AtomicGate` object with the specified `isOpen` initial
    /// state.
    AtomicGate(bool isOpen);

    /// Destroy this object.  The behavior is undefined if `tryEnter` calls
    /// are not matched by `leave` calls.
    ~AtomicGate();

    // MANIPULATORS

    /// Close the gate and wait for all `tryEnter` calls to be matched by
    /// `leave` calls.  The behavior is undefined if called more than once
    /// without an intervening `open` call.
    void closeAndDrain();

    /// Open the gate.  Undo `closeAndDrain`.
    void open();

    /// Return true if `closeAndDrain` has not been called, and increment
    /// the internal counter.
    bool tryEnter();

    /// Undo `tryEnter`.  The behavior is undefined if `tryEnter` has
    /// returned `false`, or if called more than once per successful
    /// `tryEnter`.
    void leave();
};

// ================
// class GateKeeper
// ================

/// @brief RAII wrapper around `AtomicGate`.
///
/// This mechanism is a wrapper around `AtomicGate` allowing `tryEnter` and
/// `leave` using RAII (`Status`). `open` and `close` are not thread-safe
/// and can be called from the thread maintaining the status, while `Status`
/// (`tryEnter`/`leave`) is thread-safe and can be called from any thread
/// attempting to `enter` the gate.
class GateKeeper {
  private:
    // DATA
    AtomicGate d_gate;
    bool       d_isOpen;

  public:
    // TYPES

    /// RAII wrapper for `AtomicGate::tryEnter` and `AtomicGate::leave`.
    class Status {
      private:
        // DATA
        AtomicGate& d_gate;
        bool        d_isOpen;

      private:
        // NOT IMPLEMENTED
        Status(const Status& other) BSLS_KEYWORD_DELETED;
        Status& operator=(const Status& other) BSLS_KEYWORD_DELETED;

      public:
        // CREATORS

        /// Create a `Status` object that attempts to enter the specified
        /// `gateKeeper`.  If successful, `isOpen` will return true.
        explicit Status(GateKeeper& gateKeeper);

        /// Destroy this object.  If `isOpen` returns true, leave the gate.
        ~Status();

        // ACCESSORS

        /// Return true if the gate was successfully entered.
        bool isOpen() const;
    };

  public:
    // CREATORS

    /// Create a `GateKeeper` object with the gate initially closed.
    GateKeeper();

    // MANIPULATORS

    /// Open the gate.  Has no effect if the gate is already open.
    void open();

    /// Close the gate and wait for all outstanding `Status` objects to be
    /// destroyed.  Has no effect if the gate is already closed.
    void close();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class AtomicGate
// ----------------

inline AtomicGate::AtomicGate(bool isOpen)
: d_value(isOpen ? e_INIT : e_CLOSE)
{
    // NOTHING
}

inline AtomicGate::~AtomicGate()
{
    BSLS_ASSERT_SAFE(d_value <= e_CLOSE);
}

inline bool AtomicGate::tryEnter()
{
    const int result = d_value.add(e_ENTER);

    if (result & e_CLOSE) {
        d_value.subtract(e_ENTER);
        return false;  // RETURN
    }
    else {
        return true;  // RETURN
    }
}

inline void AtomicGate::open()
{
    BSLA_MAYBE_UNUSED const int result = d_value.subtract(e_CLOSE);

    BSLS_ASSERT_SAFE(result >= e_INIT);
    BSLS_ASSERT_SAFE((result & e_CLOSE) == 0);
}

inline void AtomicGate::leave()
{
    BSLA_MAYBE_UNUSED const int result = d_value.subtract(e_ENTER);

    BSLS_ASSERT_SAFE(result >= e_INIT);
}

// ------------------------
// class GateKeeper::Status
// ------------------------

inline GateKeeper::Status::Status(GateKeeper& gateKeeper)
: d_gate(gateKeeper.d_gate)
, d_isOpen(d_gate.tryEnter())
{
    // NOTHING
}

inline GateKeeper::Status::~Status()
{
    if (d_isOpen) {
        d_gate.leave();
    }
}

inline bool GateKeeper::Status::isOpen() const
{
    return d_isOpen;
}

// ----------------
// class GateKeeper
// ----------------

inline GateKeeper::GateKeeper()
: d_gate(false)
, d_isOpen(false)
{
    // NOTHING
}

inline void GateKeeper::open()
{
    if (!d_isOpen) {
        d_isOpen = true;
        d_gate.open();
    }
}

inline void GateKeeper::close()
{
    if (d_isOpen) {
        d_isOpen = false;
        d_gate.closeAndDrain();
    }
}

}  // close package namespace
}  // close enterprise namespace

#endif
