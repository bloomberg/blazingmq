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

// bmqex_executionpolicy.t.cpp                                        -*-C++-*-
#include <bmqex_executionpolicy.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqex_executionproperty.h>

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
//   bmqex::ExecutionPolicy's constructor
// ------------------------------------------------------------------------
{
    typedef int ExecutorType;
    // NOTE: This is not a real executor type, but for the purpose of this
    //       test it will do.

    ExecutorType         executor = 42;
    bslma::TestAllocator allocator;

    // construct
    bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType> p(
        bmqex::ExecutionProperty::e_NEVER_BLOCKING,
        executor,
        &allocator);

    // check postconditions
    BMQTST_ASSERT_EQ(p.blocking(), bmqex::ExecutionProperty::e_NEVER_BLOCKING);
    BMQTST_ASSERT_EQ(p.executor(), executor);
    BMQTST_ASSERT_EQ(p.allocator(), &allocator);
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
//   bmqex::ExecutionPolicy's copy constructor
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
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWay, ExecutorType1>
            original(bmqex::ExecutionProperty::e_ALWAYS_BLOCKING,
                     executor,
                     &allocator);

        // make a copy
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWay, ExecutorType1>
            copy = original;

        // check the copy
        BMQTST_ASSERT_EQ(copy.blocking(), original.blocking());
        BMQTST_ASSERT_EQ(copy.executor(), original.executor());
        BMQTST_ASSERT_EQ(copy.allocator(), original.allocator());
    }

    // executor-converting copy
    {
        // make original
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWay, ExecutorType1>
            original(bmqex::ExecutionProperty::e_ALWAYS_BLOCKING,
                     executor,
                     &allocator);

        // make a copy
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWay, ExecutorType2>
            copy = original;

        // check the copy
        BMQTST_ASSERT_EQ(copy.blocking(), original.blocking());
        BMQTST_ASSERT_EQ(copy.executor(),
                         static_cast<ExecutorType1>(original.executor()));
        BMQTST_ASSERT_EQ(copy.allocator(), original.allocator());
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
//   bmqex::ExecutionPolicy::oneWay
//   bmqex::ExecutionPolicy::twoWay
//   bmqex::ExecutionPolicy::twoWayR
//   bmqex::ExecutionPolicy::neverBlocking
//   bmqex::ExecutionPolicy::possiblyBlocking
//   bmqex::ExecutionPolicy::alwaysBlocking
//   bmqex::ExecutionPolicy::useExecutor
//   bmqex::ExecutionPolicy::useAllocator
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
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWay, ExecutorType1>
            p1(bmqex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.oneWay();

        BMQTST_ASSERT_EQ(p2.blocking(), p1.blocking());
        BMQTST_ASSERT_EQ(p2.executor(), p1.executor());
        BMQTST_ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // twoWay
    {
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p1(bmqex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWay, ExecutorType1>
            p2 = p1.twoWay();

        BMQTST_ASSERT_EQ(p2.blocking(), p1.blocking());
        BMQTST_ASSERT_EQ(p2.executor(), p1.executor());
        BMQTST_ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // twoWayR
    {
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p1(bmqex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWayR<int>,
                               ExecutorType1>
            p2 = p1.twoWayR<int>();

        BMQTST_ASSERT_EQ(p2.blocking(), p1.blocking());
        BMQTST_ASSERT_EQ(p2.executor(), p1.executor());
        BMQTST_ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // neverBlocking
    {
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p1(bmqex::ExecutionProperty::e_ALWAYS_BLOCKING,
               executor1,
               &allocator1);

        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.neverBlocking();

        BMQTST_ASSERT_EQ(p2.blocking(),
                         bmqex::ExecutionProperty::e_NEVER_BLOCKING);
        BMQTST_ASSERT_EQ(p2.executor(), p1.executor());
        BMQTST_ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // possiblyBlocking
    {
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p1(bmqex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.possiblyBlocking();

        BMQTST_ASSERT_EQ(p2.blocking(),
                         bmqex::ExecutionProperty::e_POSSIBLY_BLOCKING);
        BMQTST_ASSERT_EQ(p2.executor(), p1.executor());
        BMQTST_ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // alwaysBlocking
    {
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p1(bmqex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.alwaysBlocking();

        BMQTST_ASSERT_EQ(p2.blocking(),
                         bmqex::ExecutionProperty::e_ALWAYS_BLOCKING);
        BMQTST_ASSERT_EQ(p2.executor(), p1.executor());
        BMQTST_ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // useExecutor
    {
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p1(bmqex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType2>
            p2 = p1.useExecutor(executor2);

        BMQTST_ASSERT_EQ(p2.blocking(), p1.blocking());
        BMQTST_ASSERT_EQ(static_cast<int>(p2.executor()),
                         static_cast<int>(executor2));
        // NOTE: casting to int to avoid floating-point comparison.
        BMQTST_ASSERT_EQ(p2.allocator(), p1.allocator());
    }

    // useAllocator
    {
        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p1(bmqex::ExecutionProperty::e_NEVER_BLOCKING,
               executor1,
               &allocator1);

        bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay, ExecutorType1>
            p2 = p1.useAllocator(&allocator2);

        BMQTST_ASSERT_EQ(p2.blocking(), p1.blocking());
        BMQTST_ASSERT_EQ(p2.executor(), p1.executor());
        BMQTST_ASSERT_EQ(p2.allocator(), &allocator2);
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
//   bmqex::ExecutionPolicy::k_IS_ONE_WAY
//   bmqex::ExecutionPolicy::k_IS_TWO_WAY
// ------------------------------------------------------------------------
{
    typedef bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay>
        OneWayPolicy;

    typedef bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWay>
        TwoWayPolicy;

    typedef bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWayR<int> >
        TwoWayPolicyWithResult;

    BMQTST_ASSERT(OneWayPolicy::k_IS_ONE_WAY && !OneWayPolicy::k_IS_TWO_WAY);

    BMQTST_ASSERT(TwoWayPolicy::k_IS_TWO_WAY && !TwoWayPolicy::k_IS_ONE_WAY);

    BMQTST_ASSERT(TwoWayPolicyWithResult::k_IS_TWO_WAY &&
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
//   bmqex::ExecutionPolicyUtil::defaultPolicy
//   bmqex::ExecutionPolicyUtil::oneWay
//   bmqex::ExecutionPolicyUtil::twoWay
//   bmqex::ExecutionPolicyUtil::twoWayR
//   bmqex::ExecutionPolicyUtil::neverBlocking
//   bmqex::ExecutionPolicyUtil::possiblyBlocking
//   bmqex::ExecutionPolicyUtil::alwaysBlocking
//   bmqex::ExecutionPolicyUtil::useExecutor
//   bmqex::ExecutionPolicyUtil::useAllocator
// ------------------------------------------------------------------------
{
    typedef int ExecutorType;
    // NOTE: This is not a real executor type, but for the purpose of this
    //       test it will do.

    typedef bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay,
                                   bmqex::SystemExecutor>
        DefaultPolicyType;

    ExecutorType         executor = 42;
    bslma::TestAllocator allocator;
    DefaultPolicyType    defaultPolicy =
        bmqex::ExecutionPolicyUtil::defaultPolicy();

    // defaultPolicy
    {
        BMQTST_ASSERT_EQ(defaultPolicy.blocking(),
                         bmqex::ExecutionProperty::e_POSSIBLY_BLOCKING);
        BMQTST_ASSERT(defaultPolicy.executor() == bmqex::SystemExecutor());
        BMQTST_ASSERT_EQ(defaultPolicy.allocator(),
                         bslma::Default::allocator());
    }

    // oneWay
    {
        DefaultPolicyType::RebindOneWay::Type p =
            bmqex::ExecutionPolicyUtil::oneWay();

        BMQTST_ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        BMQTST_ASSERT(p.executor() == defaultPolicy.executor());
        BMQTST_ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // twoWay
    {
        DefaultPolicyType::RebindTwoWay::Type p =
            bmqex::ExecutionPolicyUtil::twoWay();

        BMQTST_ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        BMQTST_ASSERT(p.executor() == defaultPolicy.executor());
        BMQTST_ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // twoWayR
    {
        DefaultPolicyType::RebindTwoWayR<int>::Type p =
            bmqex::ExecutionPolicyUtil::twoWayR<int>();

        BMQTST_ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        BMQTST_ASSERT(p.executor() == defaultPolicy.executor());
        BMQTST_ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // neverBlocking
    {
        DefaultPolicyType p = bmqex::ExecutionPolicyUtil::neverBlocking();

        BMQTST_ASSERT_EQ(p.blocking(),
                         bmqex::ExecutionProperty::e_NEVER_BLOCKING);
        BMQTST_ASSERT(p.executor() == defaultPolicy.executor());
        BMQTST_ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // possiblyBlocking
    {
        DefaultPolicyType p = bmqex::ExecutionPolicyUtil::possiblyBlocking();

        BMQTST_ASSERT_EQ(p.blocking(),
                         bmqex::ExecutionProperty::e_POSSIBLY_BLOCKING);
        BMQTST_ASSERT(p.executor() == defaultPolicy.executor());
        BMQTST_ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // alwaysBlocking
    {
        DefaultPolicyType p = bmqex::ExecutionPolicyUtil::alwaysBlocking();

        BMQTST_ASSERT_EQ(p.blocking(),
                         bmqex::ExecutionProperty::e_ALWAYS_BLOCKING);
        BMQTST_ASSERT(p.executor() == defaultPolicy.executor());
        BMQTST_ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // useExecutor
    {
        DefaultPolicyType::RebindExecutor<ExecutorType>::Type p =
            bmqex::ExecutionPolicyUtil::useExecutor(executor);

        BMQTST_ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        BMQTST_ASSERT_EQ(p.executor(), executor);
        BMQTST_ASSERT_EQ(p.allocator(), defaultPolicy.allocator());
    }

    // useAllocator
    {
        DefaultPolicyType p = bmqex::ExecutionPolicyUtil::useAllocator(
            &allocator);

        BMQTST_ASSERT_EQ(p.blocking(), defaultPolicy.blocking());
        BMQTST_ASSERT(p.executor() == defaultPolicy.executor());
        BMQTST_ASSERT_EQ(p.allocator(), &allocator);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_policy_constructor(); break;
    case 2: test2_policy_copyConstructor(); break;
    case 3: test3_policy_transformations(); break;
    case 4: test4_policy_traits(); break;
    case 5: test5_util(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
