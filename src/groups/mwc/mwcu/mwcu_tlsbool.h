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

// mwcu_tlsbool.h                                                     -*-C++-*-
#ifndef INCLUDED_MWCU_TLSBOOL
#define INCLUDED_MWCU_TLSBOOL

//@PURPOSE: Provide a mechanism representing a boolean in ThreadLocalStorage.
//
//@CLASSES:
//  mwcu::TLSBool: mechanism representing a boolean value in ThreadLocalStorage
//
//@DESCRIPTION: 'mwcu::TLSBool' is a mechanism representing a boolean value,
// stored in ThreadLocalStorage (that is a bool having a value per thread),
// using the POSIX compliant pthread 'specifics' functionality.
//
/// Functionality
///-------------
// TBD: Comment about Default vs Initial value -- assert vs safe mode
//
/// Limitations
///-----------
// TBD: PTHREAD_KEYS_MAX typically 4096.
//
/// Usage
///-----
// This section illustrates the intended use of this component.
//
/// Example 1: TBD:
///- - - - - - - -
// TBD:

// MWC

// BDE
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_types.h>
#include <bsls_unspecifiedbool.h>

namespace BloombergLP {
namespace mwcu {

// =============
// class TLSBool
// =============

/// Mechanism representing a boolean value stored in ThreadLocalStorage.
class TLSBool {
  private:
    // PRIVATE TYPES
    enum State {
        // Enum representing the state of the value, as stored in the thread
        // local variable.
        e_UNINITIALIZED = 0  // The 'd_key' failed to be created - must remain
                             // 0 (see comment in the constructor).

        ,
        e_FALSE = 1  // The value is false

        ,
        e_TRUE = 2  // The value is true
    };

    enum BitFlags {
        // Enum representing the index of the bit for the various flags stored
        // in the 'd_flags' member (for efficient memory footprint).
        e_CREATED = 0  // Has the thread key been successfully created ?

        ,
        e_INITIAL_VALUE = 1  // Value to use as initial value, if querying
                             // before setting the value.

        ,
        e_DEFAULT_VALUE = 2  // Default value to return by default in case the
                             // thread key failed to be created.
    };

    // DATA
    char d_flags;
    // bitmap of flags representing the various
    // attributes of this value.  The 'BitFlags' enum
    // describes the meaning of each bit.

    bslmt::ThreadUtil::Key d_key;
    // Thread key for accessing the value.

  private:
    // PRIVATE MANIPULATORS

    /// Set the bit at the specified `index` in d_flags to the specified
    /// `value`.
    void setFlag(int index, bool value);

    // PRIVATE ACCESSORS

    /// Get the bit at the specified `index` from d_flags.
    bool getFlag(int index) const;

    /// Return the internal value without performing any validation.  The
    /// object must be valid.
    bool valueRaw() const;

  public:
    // TYPES

    /// Use of an `UnspecifiedBool` to prevent implicit conversions to
    /// integral values, and comparisons between different classes which
    /// have boolean operators.
    typedef bsls::UnspecifiedBool<TLSBool>::BoolType BoolType;

  public:
    // CREATORS

    /// Constructor for a new object.  For each thread local variable, the
    /// optionally specified `initialValue` will be used if trying to access
    /// the value of this object before it has been set.  If the object
    /// fails to initialize, an assertion will abort the program.
    explicit TLSBool(bool initialValue = false);

    /// Constructor for a new object.  For each thread local variable, the
    /// specified `initialValue` will be used if trying to access the value
    /// of this object before it has been set.  If the object fails to
    /// created (which can be checked with `isValid`, always return the
    /// specified `defaultValue` when accessing the value.
    TLSBool(bool initialValue, bool defaultValue);

    /// Destructor
    ~TLSBool();

    // MANIPULATORS

    /// Set to the specified `value` the instance of this object for the
    /// current thread.
    void operator=(bool value);

    // ACCESSORS

    /// Conversion operator to convert this instance to an instance of a
    /// `BoolType`.
    operator BoolType() const;

    /// Equality operator for comparing the instance to the specified
    /// `value`.  Returns `true` if `BoolType()` is `true` and `value` is
    /// `true`, or `BoolType()` is `false` and `value` is `false`, otherwise
    /// `false`.
    bool operator==(bool value) const;

    /// Return whether this object has been successfully initialized.  There
    /// is a maximum number of thread local variables which can be created
    /// (PTHREAD_KEYS_MAX), so it could fail if reaching the limit.
    bool isValid() const;

    /// Return the current value of this object (or the `initialValue` if it
    /// hasn't yet been set) if it is valid, or return the specified `def`
    /// if it is not.
    bool getDefault(bool def) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class TLSBool
// -------------

inline void TLSBool::setFlag(int index, bool value)
{
    BSLS_ASSERT_SAFE(index < static_cast<int>(sizeof(d_flags) * 8));

    if (value) {
        d_flags = d_flags | (1 << index);
    }
    else {
        d_flags = d_flags & ~(1 << index);
    }
}

inline bool TLSBool::getFlag(int index) const
{
    return d_flags & (1 << index);
}

inline bool TLSBool::valueRaw() const
{
    BSLS_ASSERT_SAFE(isValid());

    bsls::Types::IntPtr value = reinterpret_cast<bsls::Types::IntPtr>(
        bslmt::ThreadUtil::getSpecific(d_key));
    switch (value) {
    case e_UNINITIALIZED: return getFlag(e_INITIAL_VALUE);  // RETURN
    case e_FALSE: return false;                             // RETURN
    case e_TRUE: return true;                               // RETURN
    default:
        BSLS_ASSERT_OPT(false && "Unexpected value");
        return false;  // RETURN
    }
}

inline TLSBool::TLSBool(bool initialValue)
: d_flags(0)
, d_key(0)
{
    setFlag(e_INITIAL_VALUE, initialValue);

    int rc = bslmt::ThreadUtil::createKey(&d_key, 0);
    BSLS_ASSERT_OPT((rc == 0) && "Failed to create key");

    setFlag(e_CREATED, (rc == 0));

    // Per contract, the value in each thread is initialized to 0, i.e.
    // 'e_UNINITIALIZED' ('A key is shared among all threads and the value
    // associated with key for each thread is 0 until it is set by that thread
    // using setSpecific').
}

inline TLSBool::TLSBool(bool initialValue, bool defaultValue)
: d_flags(0)
, d_key(0)
{
    setFlag(e_INITIAL_VALUE, initialValue);
    setFlag(e_DEFAULT_VALUE, defaultValue);

    int rc = bslmt::ThreadUtil::createKey(&d_key, 0);

    setFlag(e_CREATED, (rc == 0));

    // Per contract, the value in each thread is initialized to 0, i.e.
    // 'e_UNINITIALIZED' ('A key is shared among all threads and the value
    // associated with key for each thread is 0 until it is set by that thread
    // using setSpecific').
}

inline TLSBool::~TLSBool()
{
    if (getFlag(e_CREATED)) {
        bslmt::ThreadUtil::deleteKey(d_key);
    }
}

inline void TLSBool::operator=(bool value)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    bslmt::ThreadUtil::setSpecific(
        d_key,
        reinterpret_cast<void*>((value ? e_TRUE : e_FALSE)));
}

inline TLSBool::operator TLSBool::BoolType() const
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        const bool flag = getFlag(e_DEFAULT_VALUE);
        return bsls::UnspecifiedBool<TLSBool>::makeValue(flag);  // RETURN
    }
    return bsls::UnspecifiedBool<TLSBool>::makeValue(valueRaw());
}

inline bool TLSBool::operator==(bool value) const
{
    return *this ? value : !value;
}

inline bool TLSBool::isValid() const
{
    return getFlag(e_CREATED);
}

inline bool TLSBool::getDefault(bool def) const
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return def;  // RETURN
    }

    return valueRaw();
}

}  // close package namespace
}  // close enterprise namespace

#endif
