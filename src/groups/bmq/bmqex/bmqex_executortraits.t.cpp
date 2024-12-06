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

// bmqex_executortraits.t.cpp                                         -*-C++-*-
#include <bmqex_executortraits.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ====================
// struct SetFlagOnCall
// ====================

/// Provides a function object that sets a boolean flag on call.
struct SetFlagOnCall {
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

// ===================
// struct PostExecutor
// ===================

/// Provides an executor for test purposes that only has a `post` function.
struct PostExecutor {
    // DATA

    /// The number of calls to `post`.
    mutable int d_postCallCount;

    // CREATORS
    PostExecutor()
    : d_postCallCount(0)
    {
        // NOTHING
    }

    // ACCESSORS
    template <class FUNCTION>
    void post(FUNCTION f) const
    {
        f();
        ++d_postCallCount;
    }
};

// ===========================
// struct PostDispatchExecutor
// ===========================

/// Provides an executor for test purposes that has `post` and `dispatch`
/// functions.
struct PostDispatchExecutor {
    // DATA

    /// The number of calls to `post`.
    mutable int d_postCallCount;

    /// The number of calls to `dispatch`.
    mutable int d_dispatchCallCount;

    // CREATORS
    PostDispatchExecutor()
    : d_postCallCount(0)
    , d_dispatchCallCount(0)
    {
        // NOTHING
    }

    // ACCESSORS
    template <class FUNCTION>
    void post(FUNCTION f) const
    {
        f();
        ++d_postCallCount;
    }

    template <class FUNCTION>
    void dispatch(FUNCTION f) const
    {
        f();
        ++d_dispatchCallCount;
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_post()
// ------------------------------------------------------------------------
// POST
//
// Concerns:
//   Ensure proper behavior of the 'post' function.
//
// Plan:
//   Let 'f' be a function object of type 'F', and 'e' an executor object
//   of type 'E', such that 'e.post(f)' is a valid operations. Check that
//   'bmqex::ExecutorTraits<E>::post(e, f)' results in a call to
//   'e.post(f)'.
//
// Testing:
//   bmqex::ExecutorTraits::post
// ------------------------------------------------------------------------
{
    PostDispatchExecutor executor;
    BMQTST_ASSERT_EQ(executor.d_postCallCount, 0);

    bool executed = false;
    bmqex::ExecutorTraits<PostDispatchExecutor>::post(
        executor,
        SetFlagOnCall(&executed));
    BMQTST_ASSERT(executed);
    BMQTST_ASSERT_EQ(executor.d_postCallCount, 1);

    executed = false;
    bmqex::ExecutorTraits<PostDispatchExecutor>::post(
        executor,
        SetFlagOnCall(&executed));
    BMQTST_ASSERT(executed);
    BMQTST_ASSERT_EQ(executor.d_postCallCount, 2);
}

static void test2_dispatch()
// ------------------------------------------------------------------------
// DISPATCH
//
// Concerns:
//   Ensure proper behavior of the 'dispatch' function.
//
// Plan:
//   1. Let 'f' be a function object of type 'F', and 'e' an executor
//      object of type 'E', such that 'e.dispatch(f)' is a valid
//      operations. Check that 'bmqex::ExecutorTraits<E>::dispatch(e, f)'
//      results in a call to 'e.dispatch(f)'.
//
//   2. Let 'f' be a function object of type 'F', and 'e' an executor
//      object of type 'E', such that 'e.dispatch(f)' is not a valid
//      operations. Check that 'bmqex::ExecutorTraits<E>::dispatch(e, f)'
//      results in a call to 'e.post(f)'.
//
// Testing:
//   bmqex::ExecutorTraits::dispatch
// ------------------------------------------------------------------------
{
    // 1. dispatchable executor
    {
        PostDispatchExecutor executor;
        BMQTST_ASSERT_EQ(executor.d_dispatchCallCount, 0);

        bool executed = false;
        bmqex::ExecutorTraits<PostDispatchExecutor>::dispatch(
            executor,
            SetFlagOnCall(&executed));
        BMQTST_ASSERT(executed);
        BMQTST_ASSERT_EQ(executor.d_dispatchCallCount, 1);

        executed = false;
        bmqex::ExecutorTraits<PostDispatchExecutor>::dispatch(
            executor,
            SetFlagOnCall(&executed));
        BMQTST_ASSERT(executed);
        BMQTST_ASSERT_EQ(executor.d_dispatchCallCount, 2);
    }

    // 2. non-dispatchable executor
    {
        PostExecutor executor;
        BMQTST_ASSERT_EQ(executor.d_postCallCount, 0);

        bool executed = false;
        bmqex::ExecutorTraits<PostExecutor>::dispatch(
            executor,
            SetFlagOnCall(&executed));
        BMQTST_ASSERT(executed);
        BMQTST_ASSERT_EQ(executor.d_postCallCount, 1);

        executed = false;
        bmqex::ExecutorTraits<PostExecutor>::dispatch(
            executor,
            SetFlagOnCall(&executed));
        BMQTST_ASSERT(executed);
        BMQTST_ASSERT_EQ(executor.d_postCallCount, 2);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_post(); break;
    case 2: test2_dispatch(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
