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

// bmqex_executor.t.cpp                                               -*-C++-*-
#include <bmqex_executor.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqex_executortraits.h>

// BDE
#include <bsl_functional.h>  // bsl::reference_wrapper
#include <bslma_testallocator.h>
#include <bslmf_assert.h>
#include <bslmf_movableref.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ====================
// struct SetFlagOnCall
// ====================

/// Provides a function object that sets a boolean flag.
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

    // MANIPULATORS
    void operator()() const { *d_flag_p = true; }
};

// ==================
// class StatExecutor
// ==================

/// Provides a dummy executor that collects statistics of its `post` and
/// `dispatch` member functions calls count.
class StatExecutor {
  public:
    // TYPES

    /// Provides statistics.
    struct Statistics {
        // DATA

        /// The number of calls to `post`
        int d_postCallCount;

        /// The number of calls to `dispatch`.
        int d_dispatchCallCount;

        // CREATORS
        Statistics()
        : d_postCallCount(0)
        , d_dispatchCallCount(0)
        {
            // NOTHING
        }
    };

  private:
    // PRIVATE DATA
    Statistics* d_statistics_p;

  public:
    // CREATORS

    /// Create a `StatExecutor` object holding a pointer to he specified
    /// `statistics`. The behavior is undefined if `statistics` in null.
    explicit StatExecutor(Statistics* statistics)
    : d_statistics_p(statistics)
    {
        // PRECONDITIONS
        BSLS_ASSERT(statistics);
    }

  public:
    // MANIPULATORS

    /// Perform `++statistics()->d_postCallCount` followed by `f()`.
    template <class FUNCTION>
    void post(FUNCTION f) const
    {
        ++d_statistics_p->d_postCallCount;
        f();
    }

    /// Perform `++statistics()->d_dispatchCallCount` followed by `f()`.
    template <class FUNCTION>
    void dispatch(FUNCTION f) const
    {
        ++d_statistics_p->d_dispatchCallCount;
        f();
    }

  public:
    // ACCESSORS

    /// Return `true` if `statistics() == rhs.statistics()`, and `false`
    /// otherwise.
    bool operator==(const StatExecutor& rhs) const
    {
        return d_statistics_p == rhs.d_statistics_p;
    }

    /// Return a pointer to the statistics provided on construction.
    Statistics* statistics() const { return d_statistics_p; }
};

// =======================
// class SelfEqualExecutor
// =======================

/// Provides a dummy executor that always compares equal to itself.
class SelfEqualExecutor {
  public:
    // MANIPULATORS
    template <class F>
    void post(const F&) const
    {
        // not a valid operation
        BSLS_ASSERT_OPT(false);
    }

    template <class F>
    void dispatch(const F&) const
    {
        // not a valid operation
        BSLS_ASSERT_OPT(false);
    }

  public:
    // ACCESSORS
    bool operator==(const SelfEqualExecutor&) const { return true; }
};

// =========================
// class SelfUnequalExecutor
// =========================

/// Provides a dummy executor that always compares unequal to itself.
class SelfUnequalExecutor {
  public:
    // MANIPULATORS
    template <class F>
    void post(const F&) const
    {
        // not a valid operation
        BSLS_ASSERT_OPT(false);
    }

    template <class F>
    void dispatch(const F&) const
    {
        // not a valid operation
        BSLS_ASSERT_OPT(false);
    }

  public:
    // ACCESSORS
    bool operator==(const SelfUnequalExecutor&) const { return false; }
};

// =====================
// class ExecutorElarger
// =====================

/// Provides an adapter that increase the size of the executor object so it
/// won't fit in the small on-stack buffer used by `bmqex::Executor` for
/// optimization.
template <class EXECUTOR>
class ExecutorElarger {
  private:
    // PRIVATA DATA
    char d_padding[128];

    EXECUTOR d_executor;

  public:
    // CREATORS
    ExecutorElarger()
    : d_executor()
    {
        // NOTHING
    }

    ExecutorElarger(EXECUTOR executor)  // IMPLICIT
    : d_executor(bslmf::MovableRefUtil::move(executor))
    {
        // NOTHING
    }

  public:
    // MANIPULATORS
    template <class F>
    void post(BSLS_COMPILERFEATURES_FORWARD_REF(F) f) const
    {
        bmqex::ExecutorTraits<EXECUTOR>::post(
            d_executor,
            BSLS_COMPILERFEATURES_FORWARD(F, f));
    }

    template <class F>
    void dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(F) f) const
    {
        bmqex::ExecutorTraits<EXECUTOR>::dispatch(
            d_executor,
            BSLS_COMPILERFEATURES_FORWARD(F, f));
    }

  public:
    // ACCESSORS
    bool operator==(const ExecutorElarger& rhs) const
    {
        return d_executor == rhs.d_executor;
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_creators()
// ------------------------------------------------------------------------
// CREATORS
//
// Concerns:
//   Ensure proper behavior of creator methods.
//
// Plan:
//   1. Default-construct an instance of 'bmqex::Executor'. Check
//      postconditions.
//
//   2. Construct an instance of 'bmqex::Executor' from another executor.
//      Check postconditions.
//
//   3. Copy-construct an instance of 'bmqex::Executor'. Check
//      postconditions.
//
//   4. Move-construct an instance of 'bmqex::Executor'. Check
//      postconditions.
//
// Testing:
//   - default constructor
//   - initialization constructor
//   - copy constructor,
//   - move constructor.
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLMF_ASSERT(bmqex::Executor_Box_SboImpCanHold<StatExecutor>::value ==
                 true);
    BSLMF_ASSERT(bmqex::Executor_Box_SboImpCanHold<
                     ExecutorElarger<StatExecutor> >::value == false);

    bslma::TestAllocator     alloc;
    StatExecutor::Statistics estat;

    // 1. default constructor
    {
        bmqex::Executor ex;

        // 'ex's target is empty
        ASSERT_EQ(static_cast<bool>(ex), false);
    }

    // 2. construct from another executor
    {
        StatExecutor                  smallEx(&estat);
        ExecutorElarger<StatExecutor> largeEx = smallEx;

        // create an 'bmqex::Executor' from a small executor to enable SBO
        bmqex::Executor ex1(smallEx, &alloc);

        // check postconditions
        ASSERT(ex1.target<StatExecutor>());
        ASSERT(*ex1.target<StatExecutor>() == smallEx);

        // create an 'bmqex::Executor' from a large executor to disable SBO
        bmqex::Executor ex2(largeEx, &alloc);

        // check postconditions
        ASSERT(ex2.target<ExecutorElarger<StatExecutor> >());
        ASSERT(*ex2.target<ExecutorElarger<StatExecutor> >() == largeEx);
    }

    // 3. copy constructor
    {
        StatExecutor                  smallEx(&estat);
        ExecutorElarger<StatExecutor> largeEx = smallEx;

        bmqex::Executor ex1;                   // empty
        bmqex::Executor ex2(smallEx, &alloc);  // contains small target
        bmqex::Executor ex3(largeEx, &alloc);  // contains large target

        // copy an empty executor
        bmqex::Executor ex1Copy(ex1);

        // check postconditions
        ASSERT(!ex1Copy);

        // copy an empty executor specifying an (to be ignored) allocator
        bmqex::Executor ex1CopyAlloc(ex1, &alloc);

        // check postconditions, the copy constructor should have been selected
        // against the template one
        ASSERT(!ex1CopyAlloc);

        // copy executor containing a small target
        bmqex::Executor ex2Copy(ex2);

        // check postconditions
        ASSERT(ex2Copy.target<StatExecutor>());
        ASSERT(ex2.target<StatExecutor>() != ex2Copy.target<StatExecutor>());
        ASSERT(*ex2Copy.target<StatExecutor>() == smallEx);

        // copy executor containing a large target
        bmqex::Executor ex3Copy(ex3);

        // check postconditions
        ASSERT(ex3Copy.target<ExecutorElarger<StatExecutor> >());
        ASSERT(ex3.target<ExecutorElarger<StatExecutor> >() ==
               ex3Copy.target<ExecutorElarger<StatExecutor> >());
        ASSERT(*ex3Copy.target<ExecutorElarger<StatExecutor> >() == largeEx);
    }

    // 4. move constructor
    {
        StatExecutor                  smallEx(&estat);
        ExecutorElarger<StatExecutor> largeEx = smallEx;

        bmqex::Executor ex1;                   // empty
        bmqex::Executor ex2(smallEx, &alloc);  // contains small target
        bmqex::Executor ex3(largeEx, &alloc);  // contains large target

        // move an empty executor
        bmqex::Executor ex1Copy(bslmf::MovableRefUtil::move(ex1));

        // check postconditions
        ASSERT(!ex1Copy);

        // move an empty executor specifying an (to be ignored) allocator
        bmqex::Executor ex1CopyAlloc(bslmf::MovableRefUtil::move(ex1), &alloc);

        // check postconditions, the move constructor should have been selected
        // against the template one
        ASSERT(!ex1CopyAlloc);

        // move executor containing a small target
        bmqex::Executor ex2Copy(bslmf::MovableRefUtil::move(ex2));

        // check postconditions
        ASSERT(ex2.target<StatExecutor>());
        ASSERT(ex2Copy.target<StatExecutor>());
        ASSERT(ex2.target<StatExecutor>() != ex2Copy.target<StatExecutor>());
        ASSERT(*ex2Copy.target<StatExecutor>() == smallEx);

        // move executor containing a large target
        bmqex::Executor ex3Copy(bslmf::MovableRefUtil::move(ex3));

        // check postconditions
        ASSERT(ex3Copy.target<ExecutorElarger<StatExecutor> >());
        ASSERT(*ex3Copy.target<ExecutorElarger<StatExecutor> >() == largeEx);
    }
}

static void test2_assignment()
// ------------------------------------------------------------------------
// ASSIGNMENT
//
// Concerns:
//   Ensure proper behavior of the assignment operators and 'assign'
//   methods.
//
// Plan:
//   1. Executor-assign an instance of 'bmqex::Executor'. Check
//      postconditions.
//
//   2. Copy-assign an instance of 'bmqex::Executor'. Check
//      postconditions.
//
//   3. Move-assign an instance of 'bmqex::Executor'. Check
//      postconditions.
//
//   4. Assign a new target to an instance of 'bmqex::Executor' via the
//      'assign' member function. Check postconditions.
//
// Testing:
//   - assignment operators
//   - assign
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLMF_ASSERT(bmqex::Executor_Box_SboImpCanHold<StatExecutor>::value ==
                 true);
    BSLMF_ASSERT(bmqex::Executor_Box_SboImpCanHold<
                     ExecutorElarger<StatExecutor> >::value == false);

    bslma::TestAllocator     alloc;
    StatExecutor::Statistics estat;

    // 1. executor assignment operator
    {
        StatExecutor                  smallEx(&estat);
        ExecutorElarger<StatExecutor> largeEx = smallEx;

        // create an 'bmqex::Executor' from a small executor to enable SBO
        bmqex::Executor ex1;
        ex1 = smallEx;

        // check postconditions
        ASSERT(*ex1.target<StatExecutor>() == smallEx);

        // create an 'bmqex::Executor' from a large executor to disable SBO
        bmqex::Executor ex2;
        ex2 = largeEx;
        ASSERT(*ex2.target<ExecutorElarger<StatExecutor> >() == largeEx);
    }

    // 2. copy assignment operator
    {
        StatExecutor                  smallEx(&estat);
        ExecutorElarger<StatExecutor> largeEx = smallEx;

        bmqex::Executor ex1;                   // empty
        bmqex::Executor ex2(smallEx, &alloc);  // contains small target
        bmqex::Executor ex3(largeEx, &alloc);  // contains large target

        // copy an empty executor
        bmqex::Executor ex1Copy;
        ex1Copy = ex1;

        // check postconditions
        ASSERT(!ex1Copy);

        // copy executor containing a small target
        bmqex::Executor ex2Copy;
        ex2Copy = ex2;

        // check postconditions
        ASSERT(ex2Copy.target<StatExecutor>());
        ASSERT(ex2.target<StatExecutor>() != ex2Copy.target<StatExecutor>());
        ASSERT(*ex2Copy.target<StatExecutor>() == smallEx);

        // copy executor containing a large target
        bmqex::Executor ex3Copy;
        ex3Copy = ex3;

        // check postconditions
        ASSERT(ex3Copy.target<ExecutorElarger<StatExecutor> >());
        ASSERT(ex3.target<ExecutorElarger<StatExecutor> >() ==
               ex3Copy.target<ExecutorElarger<StatExecutor> >());
        ASSERT(*ex3Copy.target<ExecutorElarger<StatExecutor> >() == largeEx);
    }

    // 3. move assignment operator
    {
        StatExecutor                  smallEx(&estat);
        ExecutorElarger<StatExecutor> largeEx = smallEx;

        bmqex::Executor ex1;                   // empty
        bmqex::Executor ex2(smallEx, &alloc);  // contains small target
        bmqex::Executor ex3(largeEx, &alloc);  // contains large target

        // move an empty executor
        bmqex::Executor ex1Copy;
        ex1Copy = bslmf::MovableRefUtil::move(ex1);

        // check postconditions
        ASSERT(!ex1Copy);

        // move executor containing a small target
        bmqex::Executor ex2Copy;
        ex2Copy = bslmf::MovableRefUtil::move(ex2);

        // check postconditions
        ASSERT(ex2Copy.target<StatExecutor>());
        ASSERT(ex2.target<StatExecutor>() != ex2Copy.target<StatExecutor>());
        ASSERT(*ex2Copy.target<StatExecutor>() == smallEx);

        // move executor containing a large target
        bmqex::Executor ex3Copy;
        ex3Copy = bslmf::MovableRefUtil::move(ex3);

        // check postconditions
        ASSERT(ex3Copy.target<ExecutorElarger<StatExecutor> >());
        ASSERT(*ex3Copy.target<ExecutorElarger<StatExecutor> >() == largeEx);
    }

    // 4. assign
    {
        StatExecutor    ex1(&estat);
        bmqex::Executor ex2;

        // do assign
        ex2.assign(ex1, &alloc);

        // check
        ASSERT(ex2.target<StatExecutor>());
        ASSERT(*ex2.target<StatExecutor>() == ex1);
    }
}

static void test3_post()
// ------------------------------------------------------------------------
// POST
//
// Concerns:
//   Ensure proper behavior of the 'post' method.
//
// Plan:
//   Create an instance of 'bmqex::Executor' 'ex1' by constructing
//   it from another executor 'ex2'. Call 'ex1.post(f)', where 'f'
//   is a function object, and check that this operation is equivalent
//   to 'ex2.post(f)'.
//
// Testing:
//   post
// ------------------------------------------------------------------------
{
    bslma::TestAllocator     alloc;
    StatExecutor::Statistics estat;
    bmqex::Executor          ex(StatExecutor(&estat), &alloc);

    bool executed = false;
    ex.post(SetFlagOnCall(&executed));

    ASSERT(executed);
    ASSERT_EQ(estat.d_postCallCount, 1);
    ASSERT_EQ(estat.d_dispatchCallCount, 0);
}

static void test4_dispatch()
// ------------------------------------------------------------------------
// DISPATCH
//
// Concerns:
//   Ensure proper behavior of the 'dispatch' method.
//
// Plan:
//   Create an instance of 'bmqex::Executor' 'ex1' by constructing
//   it from another executor 'ex2'. Call 'ex1.dispatch(f)', where 'f'
//   is a function object, and check that this operation is equivalent
//   to 'ex2.dispatch(f)'.
//
// Testing:
//   dispatch
// ------------------------------------------------------------------------
{
    bslma::TestAllocator     alloc;
    StatExecutor::Statistics estat;
    bmqex::Executor          ex(StatExecutor(&estat), &alloc);

    bool executed = false;
    ex.dispatch(SetFlagOnCall(&executed));

    ASSERT(executed);
    ASSERT_EQ(estat.d_postCallCount, 0);
    ASSERT_EQ(estat.d_dispatchCallCount, 1);
}

static void test5_swap()
// ------------------------------------------------------------------------
// SWAP
//
// Concerns:
//   Ensure proper behavior of the 'swap' method.
//
// Plan:
//   1. Swap two executors containing small targets. Check postconditions.
//
//   2. Swap two executors containing large targets. Check postconditions.
//
//   3. Swap two executors containing a small and a large target
//      respectively. Check postconditions.
//
// Testing:
//   swap
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLMF_ASSERT(bmqex::Executor_Box_SboImpCanHold<StatExecutor>::value ==
                 true);
    BSLMF_ASSERT(bmqex::Executor_Box_SboImpCanHold<
                     ExecutorElarger<StatExecutor> >::value == false);

    bslma::TestAllocator alloc;

    // 1. small targets
    {
        StatExecutor::Statistics estat1;
        StatExecutor::Statistics estat2;
        StatExecutor             smallEx1(&estat1);
        StatExecutor             smallEx2(&estat2);

        bmqex::Executor ex1(smallEx1, &alloc);
        bmqex::Executor ex2(smallEx2, &alloc);

        // do swap
        ex1.swap(ex2);

        // check
        ASSERT(*ex1.target<StatExecutor>() == smallEx2);
        ASSERT(*ex2.target<StatExecutor>() == smallEx1);
    }

    // 2. large targets
    {
        StatExecutor::Statistics      estat1;
        StatExecutor::Statistics      estat2;
        ExecutorElarger<StatExecutor> largeEx1 = StatExecutor(&estat1);
        ExecutorElarger<StatExecutor> largeEx2 = StatExecutor(&estat2);

        bmqex::Executor ex1(largeEx1, &alloc);
        bmqex::Executor ex2(largeEx2, &alloc);

        // do swap
        ex1.swap(ex2);

        // check
        ASSERT(*ex1.target<ExecutorElarger<StatExecutor> >() == largeEx2);
        ASSERT(*ex2.target<ExecutorElarger<StatExecutor> >() == largeEx1);
    }

    // 3. small and large targets
    {
        StatExecutor::Statistics      estat1;
        StatExecutor::Statistics      estat2;
        StatExecutor                  smallEx(&estat1);
        ExecutorElarger<StatExecutor> largeEx = StatExecutor(&estat2);

        bmqex::Executor ex1(smallEx, &alloc);
        bmqex::Executor ex2(largeEx, &alloc);

        // do swap
        ex1.swap(ex2);

        // check
        ASSERT(*ex1.target<ExecutorElarger<StatExecutor> >() == largeEx);
        ASSERT(*ex2.target<StatExecutor>() == smallEx);
    }
}

static void test6_boolOperator()
// ------------------------------------------------------------------------
// BOOL OPERATOR
//
// Concerns:
//   Ensure proper behavior of the boolean conversion operator.
//
// Plan:
//   Create two executors:
//   1. default-constructed
//   2. constructed from another executor
//   Check that the boolean conversion operator returns 'false' for the
//   first executor instance and 'true' for the second.
//
// Testing:
//   operator bool
// ------------------------------------------------------------------------
{
    bslma::TestAllocator     alloc;
    StatExecutor::Statistics estat;
    bmqex::Executor          ex1;
    bmqex::Executor          ex2(StatExecutor(&estat), &alloc);

    ASSERT_EQ(static_cast<bool>(ex1), false);
    ASSERT_EQ(static_cast<bool>(ex2), true);
}

static void test7_target()
// ------------------------------------------------------------------------
// TARGET
//
// Concerns:
//   Ensure proper behavior of the 'target' method.
//
// Plan:
//   Create two executors:
//   1. default-constructed
//   2. constructed from another executor 'e'
//   Check that 'target()' returns a null-pointer for the first
//   executor instance and a pointer to a copy of 'e' for the second.
//
// Testing:
//   target
// ------------------------------------------------------------------------
{
    bslma::TestAllocator     alloc;
    StatExecutor::Statistics estat;

    // check default-constructed executor
    // NOTE: check const and non-const overloads
    {
        bmqex::Executor ex;
        ASSERT(!ex.target<StatExecutor>());
        ASSERT(!bsl::cref(ex).get().target<StatExecutor>());
    }

    // check non-default-constructed executor
    // NOTE: check const and non-const overloads
    {
        StatExecutor    ex1(&estat);
        bmqex::Executor ex2(ex1, &alloc);

        ASSERT_EQ(ex2.target<StatExecutor>()->statistics(), &estat);
        ASSERT_EQ(bsl::cref(ex2).get().target<StatExecutor>()->statistics(),
                  &estat);
    }
}

static void test8_targetType()
// ------------------------------------------------------------------------
// TARGET TYPE
//
// Concerns:
//   Ensure proper behavior of the 'targeType' method.
//
// Plan:
//   Create two executors:
//   1. default-constructed
//   2. constructed from another executor of type 'T'
//   Check that 'targetType()' returns 'typeid(void)' for the first
//   executor instance and 'typeid(T)' for the second.
//
// Testing:
//   targeType
// ------------------------------------------------------------------------
{
    bslma::TestAllocator     alloc;
    StatExecutor::Statistics estat;
    bmqex::Executor          ex1;
    bmqex::Executor          ex2(StatExecutor(&estat), &alloc);

    ASSERT_EQ(ex1.targetType() == typeid(void), true);
    ASSERT_EQ(ex2.targetType() == typeid(StatExecutor), true);
}

static void test9_equalityComparison()
// ------------------------------------------------------------------------
// EQUALITY COMPARISON
//
// Concerns:
//   Ensure proper behavior of the equality comparison operator.
//
// Plan:
//   Check that:
//   : o Two empty executors compare equal;
//   : o An empty executor does not compare equal with a non-empty executor;
//   : o Executors sharing the same target compares equal;
//   : o Executors with different target types compare unequal;
//   : o Executors with the same target type compare equal if their targets
//   :   compare equal, and vice versa.
//
// Testing:
//   Equality comparison
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLMF_ASSERT(bmqex::Executor_Box_SboImpCanHold<
                     ExecutorElarger<SelfUnequalExecutor> >::value == false);

    bslma::TestAllocator alloc;

    // two empty executors compare equal
    ASSERT_EQ(bmqex::Executor() == bmqex::Executor(), true);
    ASSERT_EQ(bmqex::Executor() != bmqex::Executor(), false);

    // empty executor is not equal to a non-empty executor
    ASSERT_EQ(bmqex::Executor() ==
                  bmqex::Executor(SelfEqualExecutor(), &alloc),
              false);
    ASSERT_EQ(bmqex::Executor() !=
                  bmqex::Executor(SelfEqualExecutor(), &alloc),
              true);

    // non-empty executor is not equal to an empty executor
    ASSERT_EQ(bmqex::Executor(SelfEqualExecutor(), &alloc) ==
                  bmqex::Executor(),
              false);
    ASSERT_EQ(bmqex::Executor(SelfEqualExecutor(), &alloc) !=
                  bmqex::Executor(),
              true);

    // executors sharing the same target compares equal
    bmqex::Executor ex1(ExecutorElarger<SelfUnequalExecutor>(), &alloc);
    bmqex::Executor ex2 = ex1;
    ASSERT_EQ(ex1.target<SelfUnequalExecutor>() ==
                  ex2.target<SelfUnequalExecutor>(),
              true);
    ASSERT_EQ(ex1 == ex2, true);
    ASSERT_EQ(ex1 != ex2, false);

    // executors with different target types compare unequal
    ASSERT_EQ(bmqex::Executor(SelfEqualExecutor(), &alloc) ==
                  bmqex::Executor(SelfUnequalExecutor(), &alloc),
              false);
    ASSERT_EQ(bmqex::Executor(SelfEqualExecutor(), &alloc) !=
                  bmqex::Executor(SelfUnequalExecutor(), &alloc),
              true);

    // executors with the same target type compare equal if their targets
    // compare equal ...
    ASSERT_EQ(bmqex::Executor(SelfUnequalExecutor(), &alloc) ==
                  bmqex::Executor(SelfUnequalExecutor(), &alloc),
              false);
    ASSERT_EQ(bmqex::Executor(SelfUnequalExecutor(), &alloc) !=
                  bmqex::Executor(SelfUnequalExecutor(), &alloc),
              true);

    // ... and vice versa
    ASSERT_EQ(bmqex::Executor(SelfEqualExecutor(), &alloc) ==
                  bmqex::Executor(SelfEqualExecutor(), &alloc),
              true);
    ASSERT_EQ(bmqex::Executor(SelfEqualExecutor(), &alloc) !=
                  bmqex::Executor(SelfEqualExecutor(), &alloc),
              false);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_creators(); break;
    case 2: test2_assignment(); break;
    case 3: test3_post(); break;
    case 4: test4_dispatch(); break;
    case 5: test5_swap(); break;
    case 6: test6_boolOperator(); break;
    case 7: test7_target(); break;
    case 8: test8_targetType(); break;
    case 9: test9_equalityComparison(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
