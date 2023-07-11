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

// mwcex_executionpolicy.t.cpp                                        -*-C++-*-
#include <mwcex_executionpolicy.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// MWC
#include <mwcex_executionproperty.h>

// BDE
#include <bslma_default.h>
#include <bslma_testallocator.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_policy_constructor()
// ------------------------------------------------------------------------
// POLICY CONSTRUCTOR
//
// Concerns:
//   Ensure proper behavior of policy's constructor.
//
// Plan:
//   Create a policy. Check postconditions.
//
// Testing:
//   mwcex::ExecutionPolicy's constructor
// ------------------------------------------------------------------------
{
    typedef int ExecutorType;
    // NOTE: This is not a real executor type, but for the purpose of this
    //       test it will do.

    ExecutorType         executor = 42;
    bslma::TestAllocator allocator;

    // construct
    mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType> p(
        mwcex::ExecutionProperty::e_NEVER_BLOCKING,
        executor,
        &allocator);

    // check postconditions
    ASSERT_EQ(p.blocking(), mwcex::ExecutionProperty::e_NEVER_BLOCKING);
    ASSERT_EQ(p.executor(), executor);
    ASSERT_EQ(p.allocator(), &allocator);
}

static void test2_policy_copyConstructor()
// ------------------------------------------------------------------------
// POLICY COPY CONSTRUCTOR
//
// Concerns:
//   Ensure proper behavior of policy's copy constructor.
//
// Plan:
//   Create a policy. Copy it. Check postconditions.
//
// Testing:
//   mwcex::ExecutionPolicy's copy constructor
// ------------------------------------------------------------------------
{
    typedef int   ExecutorType1;
    typedef float ExecutorType2;
    // NOTE: These are not a real executor types, but for the purpose of this
    //       test it will do.

    ExecutorType1        executor = 42;
    bslma::TestAllocator allocator;

    // regular copy
    {
        // make original
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::TwoWay, ExecutorType1>
            original(mwcex::ExecutionProperty::e_ALWAYS_BLOCKING,
                     executor,
                     &allocator);

        // make a copy
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::TwoWay, ExecutorType1>
            copy = original;

        // check the copy
        ASSERT_EQ(copy.blocking(), original.blocking());
        ASSERT_EQ(copy.executor(), original.executor());
        ASSERT_EQ(copy.allocator(), original.allocator());
    }

    // executor-converting copy
    {
        // make original
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::TwoWay, ExecutorType1>
            original(mwcex::ExecutionProperty::e_ALWAYS_BLOCKING,
                     executor,
                     &allocator);

        // make a copy
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::TwoWay, ExecutorType2>
            copy = original;

        // check the copy
        ASSERT_EQ(copy.blocking(), original.blocking());
        ASSERT_EQ(copy.executor(),
                  static_cast<ExecutorType1>(original.executor()));
        ASSERT_EQ(copy.allocator(), original.allocator());
    }
}

static void test3_policy_transformations()
// ------------------------------------------------------------------------
// POLICY TRANSFORMATIONS
//
// Concerns:
//   Ensure proper behavior of policy's transformation methods.
//
// Plan:
//   Check that each policy transformation method returns a policy object
//   having the same set of properties as the original object, except for
//   the transformed property.
//
// Testing:
//   mwcex::ExecutionPolicy::oneWay
//   mwcex::ExecutionPolicy::twoWay
//   mwcex::ExecutionPolicy::twoWayR
//   mwcex::ExecutionPolicy::neverBlocking
//   mwcex::ExecutionPolicy::possiblyBlocking
//   mwcex::ExecutionPolicy::alwaysBlocking
//   mwcex::ExecutionPolicy::useExecutor
//   mwcex::ExecutionPolicy::useAllocator
// ------------------------------------------------------------------------
{
    typedef int   ExecutorType1;
    typedef float ExecutorType2;
    // NOTE: These are not a real executor types, but for the purpose of this
    //       test it will do.

    ExecutorType1        executor1 = 42;
    ExecutorType2        executor2 = 24;
    bslma::TestAllocator allocator1;
    bslma::TestAllocator allocator2;

    // oneWay
    {
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::TwoWay, ExecutorType1>
            p1(mwcex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.oneWay();

        ASSERT_EQ(p2.blocking(), p1.blocking());
        ASSERT_EQ(p2.executor(), p1.executor());
        ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // twoWay
    {
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p1(mwcex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::TwoWay, ExecutorType1>
            p2 = p1.twoWay();

        ASSERT_EQ(p2.blocking(), p1.blocking());
        ASSERT_EQ(p2.executor(), p1.executor());
        ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // twoWayR
    {
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p1(mwcex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::TwoWayR<int>,
                               ExecutorType1>
            p2 = p1.twoWayR<int>();

        ASSERT_EQ(p2.blocking(), p1.blocking());
        ASSERT_EQ(p2.executor(), p1.executor());
        ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // neverBlocking
    {
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p1(mwcex::ExecutionProperty::e_ALWAYS_BLOCKING,
               executor1,
               &allocator1);

        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.neverBlocking();

        ASSERT_EQ(p2.blocking(), mwcex::ExecutionProperty::e_NEVER_BLOCKING);
        ASSERT_EQ(p2.executor(), p1.executor());
        ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // possiblyBlocking
    {
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p1(mwcex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.possiblyBlocking();

        ASSERT_EQ(p2.blocking(),
                  mwcex::ExecutionProperty::e_POSSIBLY_BLOCKING);
        ASSERT_EQ(p2.executor(), p1.executor());
        ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // alwaysBlocking
    {
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p1(mwcex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.alwaysBlocking();

        ASSERT_EQ(p2.blocking(), mwcex::ExecutionProperty::e_ALWAYS_BLOCKING);
        ASSERT_EQ(p2.executor(), p1.executor());
        ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // useExecutor
    {
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p1(mwcex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType2>
            p2 = p1.useExecutor(executor2);

        ASSERT_EQ(p2.blocking(), p1.blocking());
        ASSERT_EQ(static_cast<int>(p2.executor()),
                  static_cast<int>(executor2));
        // NOTE: casting to int to avoid floating-point comparison.
        ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // useAllocator
    {
        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p1(mwcex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.useAllocator(&allocator2);

        ASSERT_EQ(p2.blocking(), p1.blocking());
        ASSERT_EQ(p2.executor(), p1.executor());
        ASSERT_EQ(p2.allocator(), &allocator2);
    }
}

static void test4_policy_traits()
// ------------------------------------------------------------------------
// POLICY TRAITS
//
// Concerns:
//   Ensure proper behavior of policy's traits.
//
// Plan:
//   Check that each policy traits returns the right value.
//
// Testing:
//   mwcex::ExecutionPolicy::k_IS_ONE_WAY
//   mwcex::ExecutionPolicy::k_IS_TWO_WAY
// ------------------------------------------------------------------------
{
    typedef mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay>
        OneWayPolicy;

    typedef mwcex::ExecutionPolicy<mwcex::ExecutionProperty::TwoWay>
        TwoWayPolicy;

    typedef mwcex::ExecutionPolicy<mwcex::ExecutionProperty::TwoWayR<int> >
        TwoWayPolicyWithResult;

    ASSERT(OneWayPolicy::k_IS_ONE_WAY && !OneWayPolicy::k_IS_TWO_WAY);

    ASSERT(TwoWayPolicy::k_IS_TWO_WAY && !TwoWayPolicy::k_IS_ONE_WAY);

    ASSERT(TwoWayPolicyWithResult::k_IS_TWO_WAY &&
           !TwoWayPolicyWithResult::k_IS_ONE_WAY);
}

static void test5_util()
// ------------------------------------------------------------------------
// UTIL
//
// Concerns:
//   Ensure proper behavior of utility functions.
//
// Plan:
//   Check each utility function.
//
// Testing:
//   mwcex::ExecutionPolicyUtil::defaultPolicy
//   mwcex::ExecutionPolicyUtil::oneWay
//   mwcex::ExecutionPolicyUtil::twoWay
//   mwcex::ExecutionPolicyUtil::twoWayR
//   mwcex::ExecutionPolicyUtil::neverBlocking
//   mwcex::ExecutionPolicyUtil::possiblyBlocking
//   mwcex::ExecutionPolicyUtil::alwaysBlocking
//   mwcex::ExecutionPolicyUtil::useExecutor
//   mwcex::ExecutionPolicyUtil::useAllocator
// ------------------------------------------------------------------------
{
    typedef int ExecutorType;
    // NOTE: This is not a real executor type, but for the purpose of this
    //       test it will do.

    typedef mwcex::ExecutionPolicy<mwcex::ExecutionProperty::OneWay,
                                   mwcex::SystemExecutor>
        DefaultPolicyType;

    ExecutorType         executor = 42;
    bslma::TestAllocator allocator;
    DefaultPolicyType    defaultPolicy =
        mwcex::ExecutionPolicyUtil::defaultPolicy();

    // defaultPolicy
    {
        ASSERT_EQ(defaultPolicy.blocking(),
                  mwcex::ExecutionProperty::e_POSSIBLY_BLOCKING);
        ASSERT(defaultPolicy.executor() == mwcex::SystemExecutor());
        ASSERT_EQ(defaultPolicy.allocator(), bslma::Default::allocator());
    }

    // oneWay
    {
        DefaultPolicyType::RebindOneWay::Type p =
            mwcex::ExecutionPolicyUtil::oneWay();

        ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        ASSERT(p.executor() == defaultPolicy.executor());
        ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // twoWay
    {
        DefaultPolicyType::RebindTwoWay::Type p =
            mwcex::ExecutionPolicyUtil::twoWay();

        ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        ASSERT(p.executor() == defaultPolicy.executor());
        ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // twoWayR
    {
        DefaultPolicyType::RebindTwoWayR<int>::Type p =
            mwcex::ExecutionPolicyUtil::twoWayR<int>();

        ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        ASSERT(p.executor() == defaultPolicy.executor());
        ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // neverBlocking
    {
        DefaultPolicyType p = mwcex::ExecutionPolicyUtil::neverBlocking();

        ASSERT_EQ(p.blocking(), mwcex::ExecutionProperty::e_NEVER_BLOCKING);
        ASSERT(p.executor() == defaultPolicy.executor());
        ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // possiblyBlocking
    {
        DefaultPolicyType p = mwcex::ExecutionPolicyUtil::possiblyBlocking();

        ASSERT_EQ(p.blocking(), mwcex::ExecutionProperty::e_POSSIBLY_BLOCKING);
        ASSERT(p.executor() == defaultPolicy.executor());
        ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // alwaysBlocking
    {
        DefaultPolicyType p = mwcex::ExecutionPolicyUtil::alwaysBlocking();

        ASSERT_EQ(p.blocking(), mwcex::ExecutionProperty::e_ALWAYS_BLOCKING);
        ASSERT(p.executor() == defaultPolicy.executor());
        ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // useExecutor
    {
        DefaultPolicyType::RebindExecutor<ExecutorType>::Type p =
            mwcex::ExecutionPolicyUtil::useExecutor(executor);

        ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        ASSERT_EQ(p.executor(), executor);
        ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // useAllocator
    {
        DefaultPolicyType p = mwcex::ExecutionPolicyUtil::useAllocator(
            &allocator);

        ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        ASSERT(p.executor() == defaultPolicy.executor());
        ASSERT_EQ(p.allocator(), &allocator);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_policy_constructor(); break;
    case 2: test2_policy_copyConstructor(); break;
    case 3: test3_policy_transformations(); break;
    case 4: test4_policy_traits(); break;
    case 5: test5_util(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
