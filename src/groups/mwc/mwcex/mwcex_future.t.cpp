// Copyright 2023 Bloomberg Finance L.P.
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

// mwcex_future.t.cpp                                                 -*-C++-*-
#include <mwcex_future.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_exception.h>
#include <bsl_memory.h>
#include <bsl_utility.h>  // bsl::pair
#include <bslma_testallocator.h>
#include <bsls_annotation.h>
#include <bsls_libraryfeatures.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// =======================
// struct ThrowOnCopyValue
// =======================

/// Provides a type that throws an instance of
/// `ThrowOnCopyValue::ExceptionType` on copy construction or copy
/// assignment.
struct ThrowOnCopyValue {
    // TYPES

    /// Defines the type of the thrown exception.
    struct ExceptionType {};

    // CREATORS
    ThrowOnCopyValue()
    {
        // NOTHING
    }

    BSLS_ANNOTATION_NORETURN
    ThrowOnCopyValue(const ThrowOnCopyValue&) { throw ExceptionType(); }

    // MANIPULATORS
    BSLS_ANNOTATION_NORETURN
    ThrowOnCopyValue& operator=(const ThrowOnCopyValue&)
    {
        throw ExceptionType();
    }
};

// ==========================
// struct ThrowOnCopyCallback
// ==========================

/// Provides a type, suitable to be used as a future callback, that throws
/// an instance of `ThrowOnCopyCallback::ExceptionType` on copy construction
/// or copy assignment.
struct ThrowOnCopyCallback : ThrowOnCopyValue {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // MANIPULATORS
    template <class ARG>
    void operator()(const ARG&) const
    {
        // NOTHING
    }
};

// ==========================
// struct ThrowOnCallCallback
// ==========================

/// Provides a type, suitable to be used as a future callback, that throws
/// an instance of `ThrowOnCallCallback::ExceptionType` on call.
struct ThrowOnCallCallback {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    /// Defines the type of the thrown exception.
    struct ExceptionType {};

    // MANIPULATORS
    template <class ARG>
    BSLS_ANNOTATION_NORETURN void operator()(const ARG&) const
    {
        throw ExceptionType();
    }
};

// =============
// struct Assign
// =============

/// Provides a function object that assigns the specified `src` to the
/// specified `*dst`.
struct Assign {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    template <class DST, class SRC>
    void operator()(DST* dst, const SRC& src) const
    {
        *dst = src;
    }

    template <class DST, class SRC, class IGNORED>
    void operator()(DST* dst, const SRC& src, const IGNORED&) const
    {
        *dst = src;
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_sharedState_creators()
// ------------------------------------------------------------------------
// SHARED STATE CREATORS
//
// Concerns:
//   Ensure proper behavior of creator methods.
//
// Plan:
//   Construct an instance of 'mwcex::FutureSharedState'. Check
//   postconditions. Destroy the object.
//
// Testing:
//   'mwcex::FutureSharedState's constructors
//   'mwcex::FutureSharedState's destructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // default constructor
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // check postconditions
        ASSERT_EQ(sharedState.isReady(), false);
        ASSERT_EQ(sharedState.clockType(), bsls::SystemClockType::e_MONOTONIC);
        ASSERT_EQ(sharedState.allocator(), &alloc);
    }

    // clock constructor
    {
        mwcex::FutureSharedState<int> sharedState(
            bsls::SystemClockType::e_REALTIME,
            &alloc);

        // check postconditions
        ASSERT_EQ(sharedState.isReady(), false);
        ASSERT_EQ(sharedState.clockType(), bsls::SystemClockType::e_REALTIME);
        ASSERT_EQ(sharedState.allocator(), &alloc);
    }
}

static void test2_sharedState_setValue()
// ------------------------------------------------------------------------
// SHARED STATE SET VALUE
//
// Concerns:
//   Ensure proper behavior of the 'setValue' method.
//
// Plan:
//   1. Create a shared state. Make the state ready by calling
//      'setValue'. Check that the state has become ready and contains
//      the assigned value.
//
//   2. Create a shared state. Attach a callback. Make the state ready by
//      calling 'setValue'. Check that the attached callback has been
//      invoked, and then destroyed.
//
//   3. Create a shared state. Attach a callback. Call 'setValue'
//      specifying a value that throws on copy. Check that:
//      - The call had no effect;
//      - The shared state stays usable.
//
//   4. Create a shared state. Attach a callback that throws on call. Call
//      'setValue' specifying a value. Check that:
//      - The callback has been invoked, and then destroyed;
//      - The exception thrown by the callback is propagated to the caller
//        of 'setValue';
//      - After 'setValue()' returns the shared state is ready and contains
//        the assigned value.
//
// Testing:
//   mwcex::FutureSharedState::setValue
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. general use-case
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // set value
        sharedState.setValue(42);

        // state is ready and contains assigned value
        ASSERT_EQ(sharedState.isReady(), true);
        ASSERT_EQ(sharedState.get(), 42);
    }

    // 2. callback invocation
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // attach callback
        int                  result = 0;
        bslma::TestAllocator callbackAlloc;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  Assign(),
                                  &result,
                                  bdlf::PlaceHolders::_1));

        // callback not invoked or destroyed yet
        ASSERT_EQ(result, 0);
        ASSERT_NE(callbackAlloc.numBytesInUse(), 0);

        // set value
        sharedState.setValue(42);

        // callback invoked and destroyed
        ASSERT_EQ(result, 42);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);
    }

    // 3. exception safety, value throws on construction
    {
        // create shared state
        mwcex::FutureSharedState<ThrowOnCopyValue> sharedState(&alloc);

        // attach callback
        bslma::TestAllocator callbackAlloc;
        bool                 callbackInvoked = false;
        sharedState.whenReady<ThrowOnCopyValue>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  Assign(),
                                  &callbackInvoked,
                                  true,
                                  bdlf::PlaceHolders::_1));

        // set value that throws on copy
        ThrowOnCopyValue value;
        bool             exceptionThrown = false;
        try {
            sharedState.setValue(value);
        }
        catch (const ThrowOnCopyValue::ExceptionType&) {
            exceptionThrown = true;
        }

        // exception thrown
        ASSERT(exceptionThrown);

        // shared state not ready
        ASSERT(!sharedState.isReady());

        // callback not invoked or destroyed
        ASSERT_EQ(callbackInvoked, false);
        ASSERT_NE(callbackAlloc.numBytesInUse(), 0);

        // shared state can still be made ready
        sharedState.emplaceValue();
        ASSERT(sharedState.isReady());

        // callback invoked and destroyed
        ASSERT_EQ(callbackInvoked, true);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);
    }

    // 4. exception safety, callback throws on invocation
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // attach callback that throws on call
        bslma::TestAllocator callbackAlloc;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  ThrowOnCallCallback(),
                                  bdlf::PlaceHolders::_1));

        // set a value, should result in the callback throwing
        bool exceptionThrown = false;
        try {
            sharedState.setValue(42);
        }
        catch (const ThrowOnCallCallback::ExceptionType&) {
            exceptionThrown = true;
        }

        // callback invoked and destroyed
        ASSERT_EQ(exceptionThrown, true);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);

        // shared state is ready and contains the value
        ASSERT_EQ(sharedState.isReady(), true);
        ASSERT_EQ(sharedState.get(), 42);
    }
}

static void test3_sharedState_emplaceValue()
// ------------------------------------------------------------------------
// SHARED STATE EMPLACE VALUE
//
// Concerns:
//   Ensure proper behavior of the 'emplaceValue' method.
//
// Plan:
//   1. Create a shared state. Make the state ready by calling
//      'emplaceValue'. Check that the state has become ready and
//      contains a value constructed from specified arguments.
//
//   2. Create a shared state. Attach a callback. Make the state ready by
//      calling 'emplaceValue'. Check that the attached callback has
//      been invoked, and then destroyed.
//
//   3. Create a shared state. Attach a callback. Call 'emplaceValue'
//      specifying a value that throws on copy. Check that:
//      - The call had no effect;
//      - The shared state stays usable.
//
//   4. Create a shared state. Attach a callback that throws on call. Call
//      'emplaceValue' specifying a value. Check that:
//      - The callback has been invoked, and then destroyed;
//      - The exception thrown by the callback is propagated to the caller
//        of 'emplaceValue';
//      - After 'emplaceValue' returns the shared state is ready and
//        contains the assigned value.
//
// Testing:
//   mwcex::FutureSharedState::emplaceValue
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. general use-case
    {
        // create shared state
        mwcex::FutureSharedState<bsl::pair<int, int> > sharedState(&alloc);

        // set value
        sharedState.emplaceValue(4, 2);

        // state is ready and contains assigned value
        ASSERT_EQ(sharedState.isReady(), true);
        ASSERT_EQ(sharedState.get(), (bsl::pair<int, int>(4, 2)));
    }

    // 2. callback invocation
    {
        // create shared state
        mwcex::FutureSharedState<bsl::pair<int, int> > sharedState(&alloc);

        // attach callback
        bsl::pair<int, int>  result(0, 0);
        bslma::TestAllocator callbackAlloc;
        sharedState.whenReady<bsl::pair<int, int> >(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  Assign(),
                                  &result,
                                  bdlf::PlaceHolders::_1));

        // callback not invoked or destroyed yet
        ASSERT_EQ(result, (bsl::pair<int, int>(0, 0)));
        ASSERT_NE(callbackAlloc.numBytesInUse(), 0);

        // set value
        sharedState.emplaceValue(4, 2);

        // callback invoked and destroyed
        ASSERT_EQ(result, (bsl::pair<int, int>(4, 2)));
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);
    }

    // 3. exception safety
    {
        // create shared state
        mwcex::FutureSharedState<ThrowOnCopyValue> sharedState(&alloc);

        // attach callback
        bslma::TestAllocator callbackAlloc;
        bool                 callbackInvoked = false;
        sharedState.whenReady<ThrowOnCopyValue>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  Assign(),
                                  &callbackInvoked,
                                  true,
                                  bdlf::PlaceHolders::_1));

        // set value that throws on copy
        ThrowOnCopyValue value;
        bool             exceptionThrown = false;
        try {
            sharedState.emplaceValue(value);
        }
        catch (const ThrowOnCopyValue::ExceptionType&) {
            exceptionThrown = true;
        }

        // exception thrown
        ASSERT(exceptionThrown);

        // shared state not ready
        ASSERT(!sharedState.isReady());

        // callback not invoked or destroyed
        ASSERT_EQ(callbackInvoked, false);
        ASSERT_NE(callbackAlloc.numBytesInUse(), 0);

        // shared state can still be made ready
        sharedState.emplaceValue();
        ASSERT(sharedState.isReady());

        // callback invoked and destroyed
        ASSERT_EQ(callbackInvoked, true);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);
    }

    // 4. exception safety, callback throws on invocation
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // attach callback that throws on call
        bslma::TestAllocator callbackAlloc;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  ThrowOnCallCallback(),
                                  bdlf::PlaceHolders::_1));

        // set a value, should result in the callback throwing
        bool exceptionThrown = false;
        try {
            sharedState.emplaceValue(42);
        }
        catch (const ThrowOnCallCallback::ExceptionType&) {
            exceptionThrown = true;
        }

        // callback invoked and destroyed
        ASSERT_EQ(exceptionThrown, true);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);

        // shared state is ready and contains the value
        ASSERT_EQ(sharedState.isReady(), true);
        ASSERT_EQ(sharedState.get(), 42);
    }
}

static void test4_sharedState_setException_pointer()
// ------------------------------------------------------------------------
// SHARED STATE SET EXCEPTION POINTER
//
// Concerns:
//   Ensure proper behavior of the 'setException' method.
//
// Plan:
//   1. Create a shared state. Make the state ready by calling
//      'setException' specifying an exception pointer. Check that the
//      state has become ready and contains the assigned exception.
//
//   2. Create a shared state. Attach a callback. Make the state ready by
//      calling 'setException' specifying an exception pointer. Check
//      that the attached callback has been invoked, and then destroyed.
//
//   3. Create a shared state. Attach a callback that throws on call. Call
//      'setException' specifying an exception pointer. Check that:
//      - The callback has been invoked, and then destroyed;
//      - The exception thrown by the callback is propagated to the caller
//        of 'setException';
//      - After 'setException' returns the shared state is ready and
//        contains the assigned exception.
//
// Testing:
//   mwcex::FutureSharedState::setException
// ------------------------------------------------------------------------
{
#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    bslma::TestAllocator alloc;

    // 1. general use-case
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // set exception
        try {
            throw 42;
        }
        catch (...) {
            sharedState.setException(bsl::current_exception());
        }

        // retrieve exception
        int exception = 0;
        try {
            sharedState.get();
        }
        catch (int e) {
            exception = e;
        }

        // exception thrown
        ASSERT_EQ(exception, 42);
    }

    // 2. callback invocation
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // attach callback
        int                  result = 0;
        bslma::TestAllocator callbackAlloc;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  Assign(),
                                  &result,
                                  bdlf::PlaceHolders::_1));

        // callback not invoked or destroyed yet
        ASSERT_EQ(result, 0);
        ASSERT_NE(callbackAlloc.numBytesInUse(), 0);

        // set exception
        try {
            throw 42;
        }
        catch (...) {
            try {
                sharedState.setException(bsl::current_exception());
            }
            catch (int e) {
                result = e;
            }
        }

        // callback invoked and destroyed
        ASSERT_EQ(result, 42);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);
    }

    // 3. exception safety, callback throws on invocation
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // attach callback that throws on call
        bslma::TestAllocator callbackAlloc;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  ThrowOnCallCallback(),
                                  bdlf::PlaceHolders::_1));

        // set an exception, should result in the callback throwing
        bool exceptionThrown = false;
        try {
            throw 42;
        }
        catch (...) {
            try {
                sharedState.setException(bsl::current_exception());
            }
            catch (const ThrowOnCallCallback::ExceptionType&) {
                exceptionThrown = true;
            }
        }

        // callback invoked and destroyed
        ASSERT_EQ(exceptionThrown, true);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);

        // shared state is ready and contains the exception
        int result = 0;
        ASSERT_EQ(sharedState.isReady(), true);
        try {
            sharedState.get();
        }
        catch (int e) {
            result = e;
        }
        ASSERT_EQ(result, 42);
    }

#endif
}

static void test5_sharedState_setException_object()
// ------------------------------------------------------------------------
// SHARED STATE SET EXCEPTION OBJECT
//
// Concerns:
//   Ensure proper behavior of the 'setException' method.
//
// Plan:
//   1. Create a shared state. Make the state ready by calling
//      'setException' specifying an exception object. Check that the
//       state has become ready and contains the assigned exception.
//
//   2. Create a shared state. Attach a callback. Make the state ready by
//      calling 'setException' specifying an exception object. Check that
//      the attached callback has been invoked, and then destroyed.
//
//   3. Create a shared state. Attach a callback. Call 'setException'
//      specifying a value that throws on copy. Check that:
//      - The call had no effect;
//      - The shared state stays usable.
//
//   4. Create a shared state. Attach a callback that throws on call. Call
//      'setException' specifying an exception object. Check that:
//      - The callback has been invoked, and then destroyed;
//      - The exception thrown by the callback is propagated to the caller
//        of 'setException';
//      - After 'setException' returns the shared state is ready and
//        contains the assigned exception.
//
// Testing:
//   mwcex::FutureSharedState::setException
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. general use-case
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // set exception
        sharedState.setException(42);

        // retrieve exception
        int exception = 0;
        try {
            sharedState.get();
        }
        catch (int e) {
            exception = e;
        }

        // exception thrown
        ASSERT_EQ(exception, 42);
    }

    // 2. callback invocation
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // attach callback
        int                  result = 0;
        bslma::TestAllocator callbackAlloc;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  Assign(),
                                  &result,
                                  bdlf::PlaceHolders::_1));

        // callback not invoked or destroyed yet
        ASSERT_EQ(result, 0);
        ASSERT_NE(callbackAlloc.numBytesInUse(), 0);

        // set exception
        try {
            sharedState.setException(42);
        }
        catch (int e) {
            result = e;
        }

        // callback invoked and destroyed
        ASSERT_EQ(result, 42);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);
    }

    // 3. exception safety
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // attach callback
        bslma::TestAllocator callbackAlloc;
        bool                 callbackInvoked = false;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  Assign(),
                                  &callbackInvoked,
                                  true,
                                  bdlf::PlaceHolders::_1));

        // set value that throws on copy
        ThrowOnCopyValue value;
        bool             exceptionThrown = false;
        try {
            sharedState.setException(value);
        }
        catch (const ThrowOnCopyValue::ExceptionType&) {
            exceptionThrown = true;
        }

        // exception thrown
        ASSERT(exceptionThrown);

        // shared state not ready
        ASSERT(!sharedState.isReady());

        // callback not invoked or destroyed
        ASSERT_EQ(callbackInvoked, false);
        ASSERT_NE(callbackAlloc.numBytesInUse(), 0);

        // shared state can still be made ready
        sharedState.emplaceValue();
        ASSERT(sharedState.isReady());

        // callback invoked and destroyed
        ASSERT_EQ(callbackInvoked, true);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);
    }

    // 4. exception safety, callback throws on invocation
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // attach callback that throws on call
        bslma::TestAllocator callbackAlloc;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bindS(&callbackAlloc,
                                  ThrowOnCallCallback(),
                                  bdlf::PlaceHolders::_1));

        // set an exception, should result in the callback throwing
        bool exceptionThrown = false;
        try {
            sharedState.setException(42);
        }
        catch (const ThrowOnCallCallback::ExceptionType&) {
            exceptionThrown = true;
        }

        // callback invoked and destroyed
        ASSERT_EQ(exceptionThrown, true);
        ASSERT_EQ(callbackAlloc.numBytesInUse(), 0);

        // shared state is ready and contains the exception
        int result = 0;
        ASSERT_EQ(sharedState.isReady(), true);
        try {
            sharedState.get();
        }
        catch (int e) {
            result = e;
        }
        ASSERT_EQ(result, 42);
    }
}

static void test6_sharedState_whenReady()
// ------------------------------------------------------------------------
// SHARED STATE WHEN READY
//
// Concerns:
//   Ensure proper behavior of the 'whenReady' method.
//
// Plan:
//   1. Attach a callback to a ready shared state. Check that the callback
//      is invoked immediately, and is passed the contained result.
//
//   2. Attach a callback to a non-ready shared state. Then, make the
//      shared state ready. Check that the callback is invoked only after
//      the shared state has become ready, and is passed the contained
//      result.
//
//   3. Create a shared state. Call 'whenReady' specifying a value that
//      throws on copy. Check that:
//      - The call had no effect;
//      - The shared state stays usable.
//
// Testing:
//   mwcex::FutureSharedState::whenReady
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. ready shared state
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // make the shared state ready
        sharedState.setValue(42);

        // attach the callback
        int result = 0;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bind(Assign(), &result, bdlf::PlaceHolders::_1));

        // callback invoked with the result
        ASSERT_EQ(result, 42);
    }

    // 2. non-ready shared state
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // attach the callback
        int result = 0;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bind(Assign(), &result, bdlf::PlaceHolders::_1));

        // callback not invoked
        ASSERT_EQ(result, 0);

        // make the shared state ready
        sharedState.setValue(42);

        // callback invoked with the result
        ASSERT_EQ(result, 42);
    }

    // 3. exception safety
    {
        // create shared state
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // set value that throws on copy
        ThrowOnCopyCallback callback;
        bool                exceptionThrown = false;
        try {
            sharedState.whenReady<int>(callback);
        }
        catch (const ThrowOnCopyCallback::ExceptionType&) {
            exceptionThrown = true;
        }

        // exception thrown
        ASSERT(exceptionThrown);

        // shared state not ready
        ASSERT(!sharedState.isReady());

        // shared state can still be attached a callback and made ready
        int result = 0;
        sharedState.whenReady<int>(
            bdlf::BindUtil::bind(Assign(), &result, bdlf::PlaceHolders::_1));

        sharedState.setValue(42);
        ASSERT_EQ(sharedState.isReady(), true);
        ASSERT_EQ(result, 42);
    }
}

static void test7_sharedState_isReady()
// ------------------------------------------------------------------------
// SHARED STATE IS READY
//
// Concerns:
//   Ensure proper behavior of the 'isReady' method.
//
// Plan:
//   Check that 'isReady()' return 'true' if the shared state is assigned
//   a value, an exception pointer or an exception object, and 'false'
//   otherwise.
//
// Testing:
//   mwcex::FutureSharedState::isReady
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // assign nothing, check not ready
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        ASSERT_EQ(sharedState.isReady(), false);
    }

    // assign value, check ready
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        sharedState.setValue(42);
        ASSERT_EQ(sharedState.isReady(), true);
    }

    // assign exception pointer, check ready
    {
#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
        mwcex::FutureSharedState<int> sharedState(&alloc);

        try {
            throw 42;
        }
        catch (...) {
            sharedState.setException(bsl::current_exception());
        }
        ASSERT_EQ(sharedState.isReady(), true);
#endif
    }

    // assign exception object, check ready
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        sharedState.setException(42);
        ASSERT_EQ(sharedState.isReady(), true);
    }
}

static void test8_sharedState_get()
// ------------------------------------------------------------------------
// SHARED STATE GET
//
// Concerns:
//   Ensure proper behavior of the 'get' method.
//
// Plan:
//   1. Call 'get' on a shared state containing a value. Check that
//      'get()' returns a reference to the contained value.
//
//   2. Call 'get' on a shared state containing an exception pointer.
//      Check that 'get()' throws the contained exception.
//
//   3. Call 'get' on a shared state containing an exception object.
//      Check that 'get()' throws the contained exception.
//
// Testing:
//   mwcex::FutureSharedState::get
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. shared state contains a value
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // make the state ready with a value
        sharedState.setValue(42);

        // the state contains the assigned value
        ASSERT_EQ(sharedState.get(), 42);
    }

    // 2. shared state contains an exception pointer
    {
#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // make the state ready with an exception pointer
        try {
            throw 42;
        }
        catch (...) {
            sharedState.setException(bsl::current_exception());
        }

        // the state contains the assigned exception
        int exception = 0;
        try {
            sharedState.get();
        }
        catch (int e) {
            exception = e;
        }
        ASSERT_EQ(exception, 42);
#endif
    }

    // 3. shared state contains an exception object
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // make the state ready with an exception object
        sharedState.setException(42);

        // the state contains the assigned exception
        int exception = 0;
        try {
            sharedState.get();
        }
        catch (int e) {
            exception = e;
        }
        ASSERT_EQ(exception, 42);
    }
}

static void test9_sharedState_wait()
// ------------------------------------------------------------------------
// SHARED STATE WAIT
//
// Concerns:
//   Ensure proper behavior of the 'wait' method.
//
// Plan:
//   Call 'wait' on a ready shared state and check that the calling
//   thread is not blocked.
//
// Testing:
//   mwcex::FutureSharedState::wait
// ------------------------------------------------------------------------
{
    bslma::TestAllocator          alloc;
    mwcex::FutureSharedState<int> sharedState(&alloc);

    // make the state ready
    sharedState.emplaceValue();

    // 'wait' does not block
    sharedState.wait();
}

static void test10_sharedState_waitFor()
// ------------------------------------------------------------------------
// SHARED STATE WAIT FOR
//
// Concerns:
//   Ensure proper behavior of the 'waitFor' method.
//
// Plan:
//   1. Call 'waitFor' on a ready shared state. Check that:
//      - The calling thread is not blocked;
//      - The returned value is 'mwcex::FutureStatus::e_READY'.
//
//   2. Call 'waitFor' on a not ready shared state. Check that the
//      returned value is 'mwcex::FutureStatus::e_TIMEOUT'.
//
// Testing:
//   mwcex::FutureSharedState::waitFor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. shared state ready
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // make the state ready
        sharedState.emplaceValue();

        // ok
        bsls::TimeInterval duration(1000000.0);
        ASSERT_EQ(sharedState.waitFor(duration), mwcex::FutureStatus::e_READY);
    }

    // 2. shared state not ready
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // timeout
        bsls::TimeInterval duration(0.1);
        ASSERT_EQ(sharedState.waitFor(duration),
                  mwcex::FutureStatus::e_TIMEOUT);
    }
}

static void test11_sharedState_waitUntil()
// ------------------------------------------------------------------------
// SHARED STATE WAIT UNTIL
//
// Concerns:
//   Ensure proper behavior of the 'waitUntil' method.
//
// Plan:
//   1. Call 'waitUntil' on a ready shared state. Check that:
//      - The calling thread is not blocked;
//      - The returned value is 'mwcex::FutureStatus::e_READY'.
//
//   2. Call 'waitUntil' on a not ready shared state. Check that the
//      returned value is 'mwcex::FutureStatus::e_TIMEOUT'.
//
// Testing:
//   mwcex::FutureSharedState::waitUntil
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. shared state ready
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // make the state ready
        sharedState.emplaceValue();

        // ok
        bsls::TimeInterval time(1000000.0);
        ASSERT_EQ(sharedState.waitUntil(time), mwcex::FutureStatus::e_READY);
    }

    // 2. shared state not ready
    {
        mwcex::FutureSharedState<int> sharedState(&alloc);

        // timeout
        bsls::TimeInterval time(0.1);
        ASSERT_EQ(sharedState.waitUntil(time), mwcex::FutureStatus::e_TIMEOUT);
    }
}

static void test12_future_creators()
// ------------------------------------------------------------------------
// FUTURE CREATORS
//
// Concerns:
//   Ensure proper behavior of creator methods.
//
// Plan:
//   1. Default-construct a non-specialized future object. Check the future
//      is not valid.
//
//   2. Construct a non-specialized future object with a shared state.
//      Check that the future is valid and has acquired the ownership of
//      the shared state. Then destroy the future and check the the shared
//      state ownership has been released.
//
//   3. Default-construct a void-specialized future object. Check the
//      future is not valid.
//
//   4. Construct a void-specialized future object with a shared state.
//      Check that the future is valid and has acquired the ownership of
//      the shared state. Then destroy the future and check the the shared
//      state ownership has been released.
//
//   5. Default-construct a reference-specialized future object. Check the
//      future is not valid.
//
//   6. Construct a reference-specialized future object with a shared
//      state. Check that the future is valid and has acquired the
//      ownership of the shared state. Then destroy the future and check
//      the the shared state ownership has been released.
//
// Testing:
//   'mwcex::Future's constructors
//   'mwcex::Future's destructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. non-specialized, default-construct
    {
        mwcex::Future<int> future;

        // not valid
        ASSERT_EQ(future.isValid(), false);
    }

    // 2. non-specialized, state-construct
    {
        // create shared state
        typedef mwcex::Future<int>::SharedStateType SharedState;
        bsl::shared_ptr<SharedState>                sharedState =
            bsl::allocate_shared<SharedState>(&alloc);

        {
            // create future
            mwcex::Future<int> future(sharedState);

            // is valid
            ASSERT(future.isValid());

            // shared state ownership acquired
            ASSERT_EQ(sharedState.use_count(), 2);

        }  // destroy future

        // shared state ownership released
        ASSERT_EQ(sharedState.use_count(), 1);
    }

    // 3. void-specialized, default-construct
    {
        mwcex::Future<void> future;

        // not valid
        ASSERT_EQ(future.isValid(), false);
    }

    // 4. void-specialized, state-construct
    {
        // create shared state
        typedef mwcex::Future<void>::SharedStateType SharedState;
        bsl::shared_ptr<SharedState>                 sharedState =
            bsl::allocate_shared<SharedState>(&alloc);

        {
            // create future
            mwcex::Future<void> future(sharedState);

            // is valid
            ASSERT(future.isValid());

            // shared state ownership acquired
            ASSERT_EQ(sharedState.use_count(), 2);

        }  // destroy future

        // shared state ownership released
        ASSERT_EQ(sharedState.use_count(), 1);
    }

    // 5. reference-specialized, default-construct
    {
        mwcex::Future<int&> future;

        // not valid
        ASSERT_EQ(future.isValid(), false);
    }

    // 6. reference-specialized, state-construct
    {
        // create shared state
        typedef mwcex::Future<int&>::SharedStateType SharedState;
        bsl::shared_ptr<SharedState>                 sharedState =
            bsl::allocate_shared<SharedState>(&alloc);

        {
            // create future
            mwcex::Future<int&> future(sharedState);

            // is valid
            ASSERT(future.isValid());

            // shared state ownership acquired
            ASSERT_EQ(sharedState.use_count(), 2);

        }  // destroy future

        // shared state ownership released
        ASSERT_EQ(sharedState.use_count(), 1);
    }
}

static void test13_future_swap()
// ------------------------------------------------------------------------
// FUTURE SWAP
//
// Concerns:
//   Ensure proper behavior of the 'swap' method.
//
// Plan:
//   1. Create two non-specialized future object referring to different
//      shared states. Swap futures. Check that futures swapped shared
//      states.
//
//   2. Create two void-specialized future object referring to different
//      shared states. Swap futures. Check that futures swapped shared
//      states.
//
//   3. Create two reference-specialized future object referring to
//      different shared states. Swap futures. Check that futures swapped
//      shared states.
//
// Testing:
//   mwcex::Future::swap
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. non-specialized
    {
        typedef mwcex::Future<int>::SharedStateType SharedState;

        // make a shared state for the first future
        bsl::shared_ptr<SharedState> sharedState1 =
            bsl::allocate_shared<SharedState>(&alloc);

        // make a shared state for the second future
        bsl::shared_ptr<SharedState> sharedState2 =
            bsl::allocate_shared<SharedState>(&alloc);

        // create futures
        mwcex::Future<int> future1(sharedState1);
        mwcex::Future<int> future2(sharedState2);

        // swap futures
        future1.swap(future2);

        // first future now refers to the second shared state
        ASSERT_EQ(sharedState2.use_count(), 2);
        future1 = mwcex::Future<int>();
        ASSERT_EQ(sharedState2.use_count(), 1);

        // second future now refers to the first shared state
        ASSERT_EQ(sharedState1.use_count(), 2);
        future2 = mwcex::Future<int>();
        ASSERT_EQ(sharedState1.use_count(), 1);
    }

    // 2. void-specialized
    {
        typedef mwcex::Future<void>::SharedStateType SharedState;

        // make a shared state for the first future
        bsl::shared_ptr<SharedState> sharedState1 =
            bsl::allocate_shared<SharedState>(&alloc);

        // make a shared state for the second future
        bsl::shared_ptr<SharedState> sharedState2 =
            bsl::allocate_shared<SharedState>(&alloc);

        // create futures
        mwcex::Future<void> future1(sharedState1);
        mwcex::Future<void> future2(sharedState2);

        // swap futures
        future1.swap(future2);

        // first future now refers to the second shared state
        ASSERT_EQ(sharedState2.use_count(), 2);
        future1 = mwcex::Future<void>();
        ASSERT_EQ(sharedState2.use_count(), 1);

        // second future now refers to the first shared state
        ASSERT_EQ(sharedState1.use_count(), 2);
        future2 = mwcex::Future<void>();
        ASSERT_EQ(sharedState1.use_count(), 1);
    }

    // 3. reference-specialized
    {
        typedef mwcex::Future<int&>::SharedStateType SharedState;

        // make a shared state for the first future
        bsl::shared_ptr<SharedState> sharedState1 =
            bsl::allocate_shared<SharedState>(&alloc);

        // make a shared state for the second future
        bsl::shared_ptr<SharedState> sharedState2 =
            bsl::allocate_shared<SharedState>(&alloc);

        // create futures
        mwcex::Future<int&> future1(sharedState1);
        mwcex::Future<int&> future2(sharedState2);

        // swap futures
        future1.swap(future2);

        // first future now refers to the second shared state
        ASSERT_EQ(sharedState2.use_count(), 2);
        future1 = mwcex::Future<int&>();
        ASSERT_EQ(sharedState2.use_count(), 1);

        // second future now refers to the first shared state
        ASSERT_EQ(sharedState1.use_count(), 2);
        future2 = mwcex::Future<int&>();
        ASSERT_EQ(sharedState1.use_count(), 1);
    }
}

static void test14_future_isValid()
// ------------------------------------------------------------------------
// FUTURE IS VALID
//
// Concerns:
//   Ensure proper behavior of the 'isValid' method.
//
// Plan:
//   1. Create a non-specialized future object having no shared state.
//      Check that 'isValid()' returns 'false'.
//
//   2. Create a non-specialized future object having a shared state. Check
//      that 'isValid()' returns 'true'.
//
//   3. Create a void-specialized future object having no shared state.
//      Check that 'isValid()' returns 'false'.
//
//   4. Create a void-specialized future object having a shared state.
//      Check that 'isValid()' returns 'true'.
//
//   5. Create a reference-specialized future object having no shared
//      state. Check that 'isValid()' returns 'false'.
//
//   6. Create a reference-specialized future object having a shared state.
//      Check that 'isValid()' returns 'true'.
//
// Testing:
//   mwcex::Future::isValid
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. non-specialized, no shared state
    {
        mwcex::Future<int> future;

        ASSERT_EQ(future.isValid(), false);
    }

    // 2. non-specialized, have shared state
    {
        typedef mwcex::Future<int>::SharedStateType SharedState;
        mwcex::Future<int> future(bsl::allocate_shared<SharedState>(&alloc));

        ASSERT_EQ(future.isValid(), true);
    }

    // 3. void-specialized, no shared state
    {
        mwcex::Future<void> future;

        ASSERT_EQ(future.isValid(), false);
    }

    // 4. void-specialized, have shared state
    {
        typedef mwcex::Future<void>::SharedStateType SharedState;
        mwcex::Future<void> future(bsl::allocate_shared<SharedState>(&alloc));

        ASSERT_EQ(future.isValid(), true);
    }

    // 5. reference-specialized, no shared state
    {
        mwcex::Future<int&> future;

        ASSERT_EQ(future.isValid(), false);
    }

    // 6. reference-specialized, have shared state
    {
        typedef mwcex::Future<int&>::SharedStateType SharedState;
        mwcex::Future<int&> future(bsl::allocate_shared<SharedState>(&alloc));

        ASSERT_EQ(future.isValid(), true);
    }
}

static void test15_futureResult_creators()
// ------------------------------------------------------------------------
// FUTURE RESULT CREATORS
//
// Concerns:
//   Ensure proper behavior of creator methods.
//
// Plan:
//   1. Construct a non-specialized result object with a future. Check
//      that the result refers to the specified future's shared state.
//
//   2. Construct a non-specialized result object with a shared state.
//      Check that the result refers to the specified shared state.
//
//   3. Construct a void-specialized result object with a future. Check
//      that the result refers to the specified future's shared state.
//
//   4. Construct a void-specialized result object with a shared state.
//      Check that the result refers to the specified shared state.
//
//   5. Construct a reference-specialized result object with a future.
//      Check that the result refers to the specified future's shared
//      state.
//
//   6. Construct a reference-specialized result object with a shared
//      state. Check that the result refers to the specified shared state.
//
// Testing:
//   'mwcex::FutureResult's constructors
//   'mwcex::FutureResult's destructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. non-specialized, future-construct
    {
        typedef mwcex::Future<int>                  Future;
        typedef mwcex::FutureResult<int>            FutureResult;
        typedef mwcex::Future<int>::SharedStateType SharedState;

        // create a ready shared state
        bsl::shared_ptr<SharedState> sharedState =
            bsl::allocate_shared<SharedState>(&alloc);
        sharedState->setValue(42);

        // create a result
        Future       future(sharedState);
        FutureResult result(future);

        // result refers to the future's shared state
        ASSERT_EQ(result.get(), future.get());
    }

    // 2. non-specialized, state-construct
    {
        typedef mwcex::FutureResult<int>            FutureResult;
        typedef mwcex::Future<int>::SharedStateType SharedState;

        // create a ready shared state
        SharedState sharedState(&alloc);
        sharedState.setValue(42);

        // create a result
        FutureResult result(&sharedState);

        // result refers to the shared state
        ASSERT_EQ(result.get(), sharedState.get());
    }

    // 3. void-specialized, future-construct
    {
        typedef mwcex::Future<void>                  Future;
        typedef mwcex::FutureResult<void>            FutureResult;
        typedef mwcex::Future<void>::SharedStateType SharedState;

        // create a ready shared state
        bsl::shared_ptr<SharedState> sharedState =
            bsl::allocate_shared<SharedState>(&alloc);
        sharedState->setException(42);

        // create a result
        Future       future(sharedState);
        FutureResult result(future);

        // result refers to the future's shared state
        int val1 = 0;
        int val2 = 0;
        try {
            result.get();
        }
        catch (int e) {
            val1 = e;
        }
        try {
            future.get();
        }
        catch (int e) {
            val2 = e;
        }
        ASSERT_EQ(val1, val2);
    }

    // 4. void-specialized, state-construct
    {
        typedef mwcex::FutureResult<void>            FutureResult;
        typedef mwcex::Future<void>::SharedStateType SharedState;

        // create a ready shared state
        SharedState sharedState(&alloc);
        sharedState.setException(42);

        // create a result
        FutureResult result(&sharedState);

        // result refers to the shared state
        int val1 = 0;
        int val2 = 0;
        try {
            result.get();
        }
        catch (int e) {
            val1 = e;
        }
        try {
            sharedState.get();
        }
        catch (int e) {
            val2 = e;
        }
        ASSERT_EQ(val1, val2);
    }

    // 5. reference-specialized, future-construct
    {
        typedef mwcex::Future<int&>                  Future;
        typedef mwcex::FutureResult<int&>            FutureResult;
        typedef mwcex::Future<int&>::SharedStateType SharedState;

        // create a ready shared state
        int                          val = 42;
        bsl::shared_ptr<SharedState> sharedState =
            bsl::allocate_shared<SharedState>(&alloc);
        sharedState->setValue(val);

        // create a result
        Future       future(sharedState);
        FutureResult result(future);

        // result refers to the future's shared state
        ASSERT_EQ(result.get(), future.get());
    }

    // 6. reference-specialized, state-construct
    {
        typedef mwcex::FutureResult<int&>            FutureResult;
        typedef mwcex::Future<int&>::SharedStateType SharedState;

        // create a ready shared state
        int         val = 42;
        SharedState sharedState(&alloc);
        sharedState.setValue(val);

        // create a result
        FutureResult result(&sharedState);

        // result refers to the shared state
        ASSERT_EQ(result.get(), sharedState.get());
    }
}

static void test16_futureResult_swap()
// ------------------------------------------------------------------------
// FUTURE RESULT SWAP
//
// Concerns:
//   Ensure proper behavior of the 'swap' method.
//
// Plan:
//   1. Create two non-specialized result object referring to different
//      shared states. Swap results. Check that results swapped shared
//      states.
//
//   2. Create two void-specialized result object referring to different
//      shared states. Swap results. Check that results swapped shared
//      states.
//
//   3. Create two reference-specialized result object referring to
//      different shared states. Swap results. Check that results swapped
//      shared states.
//
// Testing:
//   mwcex::FutureResult::swap
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. non-specialized
    {
        typedef mwcex::FutureResult<int>            FutureResult;
        typedef mwcex::Future<int>::SharedStateType SharedState;

        // create fist ready shared state
        SharedState sharedState1(&alloc);
        sharedState1.setValue(1);

        // create second ready shared state
        SharedState sharedState2(&alloc);
        sharedState2.setValue(2);

        // create results
        FutureResult result1(&sharedState1);
        FutureResult result2(&sharedState2);

        // swap results
        result1.swap(result2);

        // first result now refers the second shared state
        ASSERT_EQ(result1.get(), sharedState2.get());

        // second result now refers the first shared state
        ASSERT_EQ(result2.get(), sharedState1.get());
    }

    // 2. void-specialized
    {
        typedef mwcex::FutureResult<void>            FutureResult;
        typedef mwcex::Future<void>::SharedStateType SharedState;

        // create fist ready shared state
        SharedState sharedState1(&alloc);
        sharedState1.setException(1);

        // create second ready shared state
        SharedState sharedState2(&alloc);
        sharedState2.setException(2);

        // create results
        FutureResult result1(&sharedState1);
        FutureResult result2(&sharedState2);

        // swap results
        result1.swap(result2);

        // first result now refers the second shared state
        {
            int val1 = 0;
            int val2 = 0;
            try {
                result1.get();
            }
            catch (int e) {
                val1 = e;
            }
            try {
                sharedState2.get();
            }
            catch (int e) {
                val2 = e;
            }
            ASSERT_EQ(val1, val2);
        }

        // second result now refers the first shared state
        {
            int val1 = 0;
            int val2 = 0;
            try {
                result2.get();
            }
            catch (int e) {
                val1 = e;
            }
            try {
                sharedState1.get();
            }
            catch (int e) {
                val2 = e;
            }
            ASSERT_EQ(val1, val2);
        }
    }

    // 3. reference-specialized
    {
        typedef mwcex::FutureResult<int&>            FutureResult;
        typedef mwcex::Future<int&>::SharedStateType SharedState;

        // create fist ready shared state
        int         val1 = 1;
        SharedState sharedState1(&alloc);
        sharedState1.setValue(val1);

        // create second ready shared state
        int         val2 = 2;
        SharedState sharedState2(&alloc);
        sharedState2.setValue(val2);

        // create results
        FutureResult result1(&sharedState1);
        FutureResult result2(&sharedState2);

        // swap results
        result1.swap(result2);

        // first result now refers the second shared state
        ASSERT_EQ(result1.get(), sharedState2.get());

        // second result now refers the first shared state
        ASSERT_EQ(result2.get(), sharedState1.get());
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    // mwcex::FutureSharedState
    case 1: test1_sharedState_creators(); break;
    case 2: test2_sharedState_setValue(); break;
    case 3: test3_sharedState_emplaceValue(); break;
    case 4: test4_sharedState_setException_pointer(); break;
    case 5: test5_sharedState_setException_object(); break;
    case 6: test6_sharedState_whenReady(); break;
    case 7: test7_sharedState_isReady(); break;
    case 8: test8_sharedState_get(); break;
    case 9: test9_sharedState_wait(); break;
    case 10: test10_sharedState_waitFor(); break;
    case 11: test11_sharedState_waitUntil(); break;

    // mwcex::Future
    case 12: test12_future_creators(); break;
    case 13: test13_future_swap(); break;
    case 14: test14_future_isValid(); break;

    // mwcex::FutureResult
    case 15: test15_futureResult_creators(); break;
    case 16: test16_futureResult_swap(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
