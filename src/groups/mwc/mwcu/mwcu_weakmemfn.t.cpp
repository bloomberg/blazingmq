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

// mwcu_weakmemfn.t.cpp                                               -*-C++-*-
#include <mwcu_weakmemfn.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bsl_memory.h>
#include <bslma_testallocator.h>
#include <bslmf_issame.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Provides an empty class for test purposes.
struct Empty {};

/// Provides an "Integer" class for test purposes.
class Int {
  private:
    int d_value;

  public:
    // CREATORS
    Int(int value)  // IMPLICIT
    : d_value(value)
    {
        // NOTHING
    }

  public:
    // MANIPULATORS
    Int& set(int value)
    {
        d_value = value;
        return *this;
    }

    void reset() { d_value = 0; }

  public:
    // ACCESSORS
    int get() const { return d_value; }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_weakMemFn_resultType()
// ------------------------------------------------------------------------
// WEAKMEMFN RESULT TYPE
//
// Concerns:
//   Ensure that the result type of 'mwcu::WeakMemFn' conforms to the
//   specification.
//
// Plan:
//   Check the result type of the 'mwcu::WeakMemFn' class template, given
//   different member function prototypes, including the ones which return
//   type is:
//   - 'void';
//   - a (possibly const / volatile) lvalue reference;
//   - a (possibly const / volatile) lvalue.
//
// Testing:
//   mwcu::WeakMemFn::ResultType
// ------------------------------------------------------------------------
{
    typedef int   R;
    typedef Empty C;

    // void -> mwcu::WeakMemFnResult<void>
    ASSERT((bsl::is_same<mwcu::WeakMemFnResult<void>,
                         mwcu::WeakMemFn<void (C::*)()>::ResultType>::value));

    // R -> mwcu::WeakMemFnResult<R>
    ASSERT((bsl::is_same<mwcu::WeakMemFnResult<R>,
                         mwcu::WeakMemFn<R (C::*)()>::ResultType>::value));

    // const R -> mwcu::WeakMemFnResult<const R>
    ASSERT(
        (bsl::is_same<mwcu::WeakMemFnResult<const R>,
                      mwcu::WeakMemFn<const R (C::*)()>::ResultType>::value));

    // volatile R -> mwcu::WeakMemFnResult<volatile R>
    ASSERT((bsl::is_same<
            mwcu::WeakMemFnResult<volatile R>,
            mwcu::WeakMemFn<volatile R (C::*)()>::ResultType>::value));

    // const volatile R -> mwcu::WeakMemFnResult<const volatile R>
    ASSERT((bsl::is_same<
            mwcu::WeakMemFnResult<const volatile R>,
            mwcu::WeakMemFn<const volatile R (C::*)()>::ResultType>::value));

    // R& -> mwcu::WeakMemFnResult<R&>
    ASSERT((bsl::is_same<mwcu::WeakMemFnResult<R&>,
                         mwcu::WeakMemFn<R& (C::*)()>::ResultType>::value));

    // const R& -> mwcu::WeakMemFnResult<const R&>
    ASSERT(
        (bsl::is_same<mwcu::WeakMemFnResult<const R&>,
                      mwcu::WeakMemFn<const R& (C::*)()>::ResultType>::value));

    // volatile R& -> mwcu::WeakMemFnResult<volatile R&>
    ASSERT((bsl::is_same<
            mwcu::WeakMemFnResult<volatile R&>,
            mwcu::WeakMemFn<volatile R& (C::*)()>::ResultType>::value));

    // const volatile R& -> mwcu::WeakMemFnResult<const volatile R&>
    ASSERT((bsl::is_same<
            mwcu::WeakMemFnResult<const volatile R&>,
            mwcu::WeakMemFn<const volatile R& (C::*)()>::ResultType>::value));
}

static void test2_weakMemFn_callOperator()
// ------------------------------------------------------------------------
// WEAKMEMFN CALL OPERATOR
//
// Concerns:
//   Ensure proper behavior of the 'mwcu::WeakMemFn's call operator.
//
// Plan:
//   Legend:
//   'C'       - a class type
//   'c'       - an instance of 'C'
//   'weakC'   - an instance of 'bsl::weak_ptr<C>', pointing to 'c'
//   'MemFn'   - a member function pointer type on 'C'
//   'memFn'   - an instance of 'MemFn'
//   'Args...' - arguments types of 'MemFn'
//   'args...' - values of types 'Args...'
//
//   Construct an instance of 'mwcu::WeakMemFn<MemFn>', 'weakMemFn',
//   specifying a member function pointer 'memFn' of type 'MemFn'. Call
//   'weakMemFn(weakC, args...)'. Check that this operation is a no-op if
//   'weakC' has expired, or is identical to 'c.*memFn(args...)' otherwise.
//
//   Test the call operator in following situations:
//   - 'weakC' has NOT expired, the return type of 'MemFn' is 'void';
//   - 'weakC' has NOT expired, the return type of 'MemFn' is an lvalue;
//   - 'weakC' has NOT expired, the return type of 'MemFn' is an lvalue
//     reference;
//   - 'weakC' has expired, the return type of 'MemFn' is 'void';
//   - 'weakC' has expired, the return type of 'MemFn' is an lvalue;
//   - 'weakC' has expired, the return type of 'MemFn' is an lvalue
//     reference;
//
// Testing:
//   'mwcu::WeakMemFn's call operator
// ------------------------------------------------------------------------
{
    typedef mwcu::WeakMemFn<int (Int::*)() const> GetFn;
    typedef mwcu::WeakMemFn<Int& (Int::*)(int)>   SetFn;
    typedef mwcu::WeakMemFn<void (Int::*)()>      ResetFn;

    bslma::TestAllocator alloc;
    bsl::shared_ptr<Int> intPtr     = bsl::allocate_shared<Int>(&alloc, 10);
    bsl::weak_ptr<Int>   weakIntPtr = bsl::weak_ptr<Int>(intPtr);

    GetFn             getFn(&Int::get);
    GetFn::ResultType getRes = getFn(weakIntPtr);
    ASSERT_EQ(getRes.isNull(), false);
    ASSERT_EQ(getRes.value(), 10);

    SetFn             setFn(&Int::set);
    SetFn::ResultType setRes = setFn(weakIntPtr, 42);
    ASSERT_EQ(setRes.isNull(), false);
    ASSERT_EQ(setRes.value().get(), 42);

    ResetFn             resetFn(&Int::reset);
    ResetFn::ResultType resetRes = resetFn(weakIntPtr);
    ASSERT_EQ(resetRes.isNull(), false);
    ASSERT_EQ(intPtr->get(), 0);

    intPtr.reset();

    getRes = getFn(weakIntPtr);
    ASSERT_EQ(getRes.isNull(), true);

    setRes = setFn(weakIntPtr, 42);
    ASSERT_EQ(setRes.isNull(), true);

    resetRes = resetFn(weakIntPtr);
    ASSERT_EQ(resetRes.isNull(), true);
}

static void test3_weakMemFnInstance_resultType()
// ------------------------------------------------------------------------
// WEAKMEMFNINSTANCE RESULT TYPE
//
// Concerns:
//   Ensure that the result type of 'mwcu::WeakMemFnInstance' conforms to
//   the specification.
//
// Plan:
//   Check the result type of the 'mwcu::WeakMemFnInstance' class
//   template, given different member function prototypes, including the
//   ones which return type is:
//   - 'void';
//   - a (possibly const / volatile) lvalue reference;
//   - a (possibly const / volatile) lvalue.
//
// Testing:
//   mwcu::WeakMemFnInstance::ResultType
// ------------------------------------------------------------------------
{
    typedef int   R;
    typedef Empty C;

    // void -> mwcu::WeakMemFnResult<void>
    ASSERT((bsl::is_same<
            mwcu::WeakMemFnResult<void>,
            mwcu::WeakMemFnInstance<void (C::*)()>::ResultType>::value));

    // R -> mwcu::WeakMemFnResult<R>
    ASSERT((
        bsl::is_same<mwcu::WeakMemFnResult<R>,
                     mwcu::WeakMemFnInstance<R (C::*)()>::ResultType>::value));

    // const R -> mwcu::WeakMemFnResult<const R>
    ASSERT((bsl::is_same<
            mwcu::WeakMemFnResult<const R>,
            mwcu::WeakMemFnInstance<const R (C::*)()>::ResultType>::value));

    // volatile R -> mwcu::WeakMemFnResult<volatile R>
    ASSERT((bsl::is_same<
            mwcu::WeakMemFnResult<volatile R>,
            mwcu::WeakMemFnInstance<volatile R (C::*)()>::ResultType>::value));

    // const volatile R -> mwcu::WeakMemFnResult<const volatile R>
    ASSERT((bsl::is_same<mwcu::WeakMemFnResult<const volatile R>,
                         mwcu::WeakMemFnInstance<const volatile R (
                             C::*)()>::ResultType>::value));

    // R& -> mwcu::WeakMemFnResult<R&>
    ASSERT((bsl::is_same<
            mwcu::WeakMemFnResult<R&>,
            mwcu::WeakMemFnInstance<R& (C::*)()>::ResultType>::value));

    // const R& -> mwcu::WeakMemFnResult<const R&>
    ASSERT((bsl::is_same<
            mwcu::WeakMemFnResult<const R&>,
            mwcu::WeakMemFnInstance<const R& (C::*)()>::ResultType>::value));

    // volatile R& -> mwcu::WeakMemFnResult<volatile R&>
    ASSERT((bsl::is_same<mwcu::WeakMemFnResult<volatile R&>,
                         mwcu::WeakMemFnInstance<volatile R& (
                             C::*)()>::ResultType>::value));

    // const volatile R& -> mwcu::WeakMemFnResult<const volatile R&>
    ASSERT((bsl::is_same<mwcu::WeakMemFnResult<const volatile R&>,
                         mwcu::WeakMemFnInstance<const volatile R& (
                             C::*)()>::ResultType>::value));
}

static void test4_weakMemFnInstance_callOperator()
// ------------------------------------------------------------------------
// WEAKMEMFNINSTANCE CALL OPERATOR
//
// Concerns:
// Concerns:
//   Ensure proper behavior of the 'mwcu::WeakMemFnInstance's call
//   operator.
//
// Plan:
//   Legend:
//   'C'       - a class type
//   'c'       - an instance of 'C'
//   'weakC'   - an instance of 'bsl::weak_ptr<C>', pointing to 'c'
//   'MemFn'   - a member function pointer type on 'C'
//   'memFn'   - an instance of 'MemFn'
//   'Args...' - arguments types of 'MemFn'
//   'args...' - values of types 'Args...'
//
//   Construct an instance of 'mwcu::WeakMemFnInstance<MemFn>',
//   'weakMemFn', specifing a member function pointer 'memFn' of type
//   'MemFn' and a weak object pointer 'weakC' of type 'bsl::weak_ptr<C>'.
//   Call 'weakMemFn(args...)'. Check that this operation is a no-op if
//   'weakC' has expired, or is identical to 'c.*memFn(args...)' otherwise.
//
//   Test the call operator in following situations:
//   - 'weakC' has NOT expired, the return type of 'MemFn' is 'void';
//   - 'weakC' has NOT expired, the return type of 'MemFn' is an lvalue;
//   - 'weakC' has NOT expired, the return type of 'MemFn' is an lvalue
//     reference;
//   - 'weakC' has expired, the return type of 'MemFn' is 'void';
//   - 'weakC' has expired, the return type of 'MemFn' is an lvalue;
//   - 'weakC' has expired, the return type of 'MemFn' is an lvalue
//     reference;
//
// Testing:
//   'mwcu::WeakMemFnInstance's call operator
// ------------------------------------------------------------------------
{
    typedef mwcu::WeakMemFnInstance<int (Int::*)() const> GetFn;
    typedef mwcu::WeakMemFnInstance<Int& (Int::*)(int)>   SetFn;
    typedef mwcu::WeakMemFnInstance<void (Int::*)()>      ResetFn;

    bslma::TestAllocator alloc;
    bsl::shared_ptr<Int> intPtr     = bsl::allocate_shared<Int>(&alloc, 10);
    bsl::weak_ptr<Int>   weakIntPtr = bsl::weak_ptr<Int>(intPtr);

    GetFn             getFn(&Int::get, weakIntPtr);
    GetFn::ResultType getRes = getFn();
    ASSERT_EQ(getRes.isNull(), false);
    ASSERT_EQ(getRes.value(), 10);

    SetFn             setFn(&Int::set, weakIntPtr);
    SetFn::ResultType setRes = setFn(42);
    ASSERT_EQ(setRes.isNull(), false);
    ASSERT_EQ(setRes.value().get(), 42);

    ResetFn             resetFn(&Int::reset, weakIntPtr);
    ResetFn::ResultType resetRes = resetFn();
    ASSERT_EQ(resetRes.isNull(), false);
    ASSERT_EQ(intPtr->get(), 0);

    intPtr.reset();

    getRes = getFn();
    ASSERT_EQ(getRes.isNull(), true);

    setRes = setFn(42);
    ASSERT_EQ(setRes.isNull(), true);

    resetRes = resetFn();
    ASSERT_EQ(resetRes.isNull(), true);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_weakMemFn_resultType(); break;
    case 2: test2_weakMemFn_callOperator(); break;
    case 3: test3_weakMemFnInstance_resultType(); break;
    case 4: test4_weakMemFnInstance_callOperator(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
