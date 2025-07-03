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

// bmqex_bindutil.t.cpp                                               -*-C++-*-
#include <bmqex_bindutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqex_executionpolicy.h>

// BDE
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>  // bsl::allocator_arg
#include <bsl_ostream.h>
#include <bslma_testallocator.h>
#include <bslmf_movableref.h>
#include <bsls_assert.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ============
// struct SumOf
// ============

/// Provides a VST representing the sum of several (from 0 to 9) integers
/// provided on construction.
struct SumOf {
    // DATA
    int d_value;

    // CREATORS
    explicit SumOf(int v1 = 0,
                   int v2 = 0,
                   int v3 = 0,
                   int v4 = 0,
                   int v5 = 0,
                   int v6 = 0,
                   int v7 = 0,
                   int v8 = 0,
                   int v9 = 0)
    : d_value(v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9)
    {
        // NOTHING
    }
};

// ====================
// struct SetFlagOnCall
// ====================

/// Provides a functor that sets a boolean flag.
struct SetFlagOnCall {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // DATA
    bool* d_flag_p;

    // CREATORS
    explicit SetFlagOnCall(bool* flag)
    : d_flag_p(flag)
    {
        // NOTHING
    }

    // ACCESSORS
    void operator()() const { *d_flag_p = true; }
};

// ==========================
// struct AssignEmplaceOnCall
// ==========================

/// Provides a functor that, when called as `f(args...)`, assigns a value
/// of type `VALUE` direct-non-list-initialized from `args...` to a
/// destination object provided on construction.
template <class VALUE>
struct AssignEmplaceOnCall {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // DATA
    VALUE* d_dst_p;

    // CREATORS
    explicit AssignEmplaceOnCall(VALUE* dst)
    : d_dst_p(dst)
    {
        // PRECONDITIONS
        BSLS_ASSERT(dst);
    }

    // ACCESSORS
    void operator()() const { *d_dst_p = VALUE(); }

    template <class ARG1>
    void operator()(const ARG1& arg1) const
    {
        *d_dst_p = VALUE(arg1);
    }

    template <class ARG1, class ARG2>
    void operator()(const ARG1& arg1, const ARG2& arg2) const
    {
        *d_dst_p = VALUE(arg1, arg2);
    }

    template <class ARG1, class ARG2, class ARG3>
    void operator()(const ARG1& arg1, const ARG2& arg2, const ARG3& arg3) const
    {
        *d_dst_p = VALUE(arg1, arg2, arg3);
    }

    template <class ARG1, class ARG2, class ARG3, class ARG4>
    void operator()(const ARG1& arg1,
                    const ARG2& arg2,
                    const ARG3& arg3,
                    const ARG4& arg4) const
    {
        *d_dst_p = VALUE(arg1, arg2, arg3, arg4);
    }

    template <class ARG1, class ARG2, class ARG3, class ARG4, class ARG5>
    void operator()(const ARG1& arg1,
                    const ARG2& arg2,
                    const ARG3& arg3,
                    const ARG4& arg4,
                    const ARG5& arg5) const
    {
        *d_dst_p = VALUE(arg1, arg2, arg3, arg4, arg5);
    }

    template <class ARG1,
              class ARG2,
              class ARG3,
              class ARG4,
              class ARG5,
              class ARG6>
    void operator()(const ARG1& arg1,
                    const ARG2& arg2,
                    const ARG3& arg3,
                    const ARG4& arg4,
                    const ARG5& arg5,
                    const ARG6& arg6) const
    {
        *d_dst_p = VALUE(arg1, arg2, arg3, arg4, arg5, arg6);
    }

    template <class ARG1,
              class ARG2,
              class ARG3,
              class ARG4,
              class ARG5,
              class ARG6,
              class ARG7>
    void operator()(const ARG1& arg1,
                    const ARG2& arg2,
                    const ARG3& arg3,
                    const ARG4& arg4,
                    const ARG5& arg5,
                    const ARG6& arg6,
                    const ARG7& arg7) const
    {
        *d_dst_p = VALUE(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

    template <class ARG1,
              class ARG2,
              class ARG3,
              class ARG4,
              class ARG5,
              class ARG6,
              class ARG7,
              class ARG8>
    void operator()(const ARG1& arg1,
                    const ARG2& arg2,
                    const ARG3& arg3,
                    const ARG4& arg4,
                    const ARG5& arg5,
                    const ARG6& arg6,
                    const ARG7& arg7,
                    const ARG8& arg8) const
    {
        *d_dst_p = VALUE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }

    template <class ARG1,
              class ARG2,
              class ARG3,
              class ARG4,
              class ARG5,
              class ARG6,
              class ARG7,
              class ARG8,
              class ARG9>
    void operator()(const ARG1& arg1,
                    const ARG2& arg2,
                    const ARG3& arg3,
                    const ARG4& arg4,
                    const ARG5& arg5,
                    const ARG6& arg6,
                    const ARG7& arg7,
                    const ARG8& arg8,
                    const ARG9& arg9) const
    {
        *d_dst_p = VALUE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_bindUtil_bindExecute()
// ------------------------------------------------------------------------
// BINDUTIL BINDEXECUTE
//
// Concerns:
//   Ensure proper behavior of the 'bindExecute' function.
//
// Plan:
//   Create a bind wrapper via a call to 'bmqex::BindUtil::bindExecute'
//   with an execution policy and a function object. Call the wrapper and
//   check that the supplied function object was executed according to the
//   supplied execution policy.
//
// Testing:
//   bmqex::BindUtil::bindExecute
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    bool                 executed = false;

    // create a bind wrapper
    bsl::function<void()> wrapper(
        bsl::allocator_arg,
        &alloc,
        bmqex::BindUtil::bindExecute(
            bmqex::ExecutionPolicyUtil::defaultPolicy().useAllocator(&alloc),
            SetFlagOnCall(&executed)));

    // target not executed yet
    BMQTST_ASSERT(!executed);

    // invoke the bind wrapper
    wrapper();

    // target executed
    BMQTST_ASSERT(executed);
}

static void test2_bindWrapper_creators()
// ------------------------------------------------------------------------
// BINDWRAPPER CREATORS
//
// Concerns:
//   Ensure proper behavior of creator methods.
//
// Plan:
//   1. Create a bind wrapper by Initializing it with an execution policy
//      and a function object. Call the wrapper and check that the
//      supplied function object was executed according to the supplied
//      execution policy.
//
//   2. Create a bind wrapper by copying another wrapper. Call the copy and
//      check that it behaves as the original would.
//
//   3. Create a bind wrapper by moving another wrapper. Call the copy and
//      check that it behaves as the original would.
//
// Testing:
//   bmqex::BindUtil::BindUtil_BindWrapper init constructor
//   bmqex::BindUtil::BindUtil_BindWrapper copy constructor
//   bmqex::BindUtil::BindUtil_BindWrapper move constructor
// ------------------------------------------------------------------------
{
    typedef bmqex::BindUtil_BindWrapper<bmqex::ExecutionPolicy<>,
                                        SetFlagOnCall>
        BindWrapper;

    bslma::TestAllocator alloc;

    // 1. init c-tor
    {
        bool executed = false;

        // create a bind wrapper
        BindWrapper wrapper(
            bmqex::ExecutionPolicyUtil::defaultPolicy().useAllocator(&alloc),
            SetFlagOnCall(&executed),
            &alloc);

        // target not executed yet
        BMQTST_ASSERT(!executed);

        // invoke the bind wrapper
        wrapper();

        // target executed
        BMQTST_ASSERT(executed);
    }

    // 2. copy c-tor
    {
        bool executed = false;

        // create a bind wrapper
        BindWrapper wrapper(
            bmqex::ExecutionPolicyUtil::defaultPolicy().useAllocator(&alloc),
            SetFlagOnCall(&executed),
            &alloc);

        // make a copy
        BindWrapper wrapperCopy(wrapper, &alloc);

        // invoke the bind wrapper copy
        wrapperCopy();

        // target executed
        BMQTST_ASSERT(executed);
    }

    // 3. move c-tor
    {
        bool executed = false;

        // create a bind wrapper
        BindWrapper wrapper(
            bmqex::ExecutionPolicyUtil::defaultPolicy().useAllocator(&alloc),
            SetFlagOnCall(&executed),
            &alloc);

        // make a copy
        BindWrapper wrapperCopy(bslmf::MovableRefUtil::move(wrapper), &alloc);

        // invoke the bind wrapper copy
        wrapperCopy();

        // target executed
        BMQTST_ASSERT(executed);
    }
}

static void test3_bindWrapper_callOperator()
// ------------------------------------------------------------------------
// BINDWRAPPER CALL OPERATOR
//
// Concerns:
//   Ensure proper behavior of the call operator.
//
// Plan:
//   Invoke a bind wrapper with 0 and up to 9 arguments. Check that the
//   call operator behave properly.
//
// Testing:
//   bmqex::BindUtil::BindUtil_BindWrapper::operator()
// ------------------------------------------------------------------------
{
    typedef bmqex::BindUtil_BindWrapper<bmqex::ExecutionPolicy<>,
                                        AssignEmplaceOnCall<SumOf> >
        BindWrapper;

    bslma::TestAllocator alloc;
    SumOf                sumOf(777);

    // create a bind wrapper
    BindWrapper wrapper(
        bmqex::ExecutionPolicyUtil::defaultPolicy().useAllocator(&alloc),
        AssignEmplaceOnCall<SumOf>(&sumOf),
        &alloc);

    // invoke the wrapper with 0 up to 9 arguments
    wrapper();
    BMQTST_ASSERT_EQ(sumOf.d_value, 0);

    wrapper(2);
    BMQTST_ASSERT_EQ(sumOf.d_value, 2);

    wrapper(2, 3);
    BMQTST_ASSERT_EQ(sumOf.d_value, 5);

    wrapper(2, 3, 5);
    BMQTST_ASSERT_EQ(sumOf.d_value, 10);

    wrapper(2, 3, 5, 7);
    BMQTST_ASSERT_EQ(sumOf.d_value, 17);

    wrapper(2, 3, 5, 7, 11);
    BMQTST_ASSERT_EQ(sumOf.d_value, 28);

    wrapper(2, 3, 5, 7, 11, 13);
    BMQTST_ASSERT_EQ(sumOf.d_value, 41);

    wrapper(2, 3, 5, 7, 11, 13, 17);
    BMQTST_ASSERT_EQ(sumOf.d_value, 58);

    wrapper(2, 3, 5, 7, 11, 13, 17, 19);
    BMQTST_ASSERT_EQ(sumOf.d_value, 77);

    wrapper(2, 3, 5, 7, 11, 13, 17, 19, 23);
    BMQTST_ASSERT_EQ(sumOf.d_value, 100);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_bindUtil_bindExecute(); break;
    case 2: test2_bindWrapper_creators(); break;
    case 3: test3_bindWrapper_callOperator(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
