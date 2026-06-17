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

#include <bslma_allocator.h>
#include <mqbplug_authorizer.h>

// MQB
#include <mqbplug_authenticator.h>

// BMQ

// BDE
#include <bsl_type_traits.h>
#include <bslmf_assert.h>
#include <bslmf_movableref.h>

// TEST_DRIVER
#include <bmqtst_testhelper.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// CONVENIENCE
using namespace BloombergLP;

namespace {

/// This type wraps `TYPE` into a `bslmf::MovableRef<TYPE>`.
template <class TYPE>
struct AddMovableRef {
    // PUBLIC TYPES

    /// This `typedef` is an alias to the return type of this meta-function.
    typedef bslmf::MovableRef<TYPE> type;
};

/// This type checks if `T` is constructible with an allocator-extended copy
/// constructor.
template <typename TYPE>
struct IsExtendedCopyConstructible
: bsl::is_constructible<
      TYPE,
      typename bsl::add_lvalue_reference<
          typename bsl::add_const<TYPE>::type>::type,
      typename bsl::add_lvalue_reference<typename bsl::add_const<
          typename TYPE::allocator_type>::type>::type> {};

/// This type checks if `T` is constructible with an allocator-extended move
/// constructor.
template <typename TYPE>
struct IsExtendedMoveConstructible
: bsl::is_constructible<
      TYPE,
      typename AddMovableRef<TYPE>::type,
      typename bsl::add_lvalue_reference<typename bsl::add_const<
          typename TYPE::allocator_type>::type>::type> {};

/// This type checks if `TYPE` supports all seven of the BDE allocator model's
/// special constructor functions.
///
/// In other words, `IsAllocatorAware<TYPE>::value` is `true` if:
/// - `TYPE` is copy constructible
/// - `TYPE` is copy constructible with a BDE-style allocator
/// - `TYPE` is move constructible
/// - `TYPE` is move constructible with a BDE-style allocator
/// - `TYPE` is copy assignable
/// - `TYPE` is move assignable
/// - `TYPE` is destructible
template <typename TYPE>
struct IsAllocatorAware
: bsl::integral_constant<bool,
                         bsl::is_copy_constructible<TYPE>::value &&
                             bsl::is_move_constructible<TYPE>::value &&
                             bsl::is_copy_assignable<TYPE>::value &&
                             bsl::is_move_assignable<TYPE>::value &&
                             IsExtendedCopyConstructible<TYPE>::value &&
                             IsExtendedMoveConstructible<TYPE>::value &&
                             bsl::is_destructible<TYPE>::value> {};

}

BSLMF_ASSERT(IsAllocatorAware<mqbplug::Action_ConnectClusterNode>::value);

BSLMF_ASSERT(IsAllocatorAware<mqbplug::Action_QueueRead>::value);

BSLMF_ASSERT(IsAllocatorAware<mqbplug::Action_QueueWrite>::value);

BSLMF_ASSERT(IsAllocatorAware<mqbplug::Action_ExecuteAdminCommand>::value);

BSLMF_ASSERT(IsAllocatorAware<mqbplug::Action>::value);

TEST(Action_ConnectClient, equalityTest)
{
    bsl::allocator<>              alloc = bmqtst::TestHelperUtil::allocator();
    mqbplug::Action_ConnectClient obj1;

    EXPECT_TRUE(obj1 == obj1);
    EXPECT_FALSE(obj1 != obj1);

    // All objects of this type are equal to each other
    mqbplug::Action_ConnectClient obj2;
    EXPECT_FALSE(obj1 != obj2);
    EXPECT_TRUE(obj1 == obj2);
}

TEST(Action_ConnectProxy, equalityTest)
{
    bsl::allocator<>             alloc = bmqtst::TestHelperUtil::allocator();
    mqbplug::Action_ConnectProxy obj1;

    EXPECT_TRUE(obj1 == obj1);
    EXPECT_FALSE(obj1 != obj1);

    // All objects of this type are equal to each other
    mqbplug::Action_ConnectProxy obj2;
    EXPECT_FALSE(obj1 != obj2);
    EXPECT_TRUE(obj1 == obj2);
}

TEST(Action_ConnectAdmin, equalityTest)
{
    bsl::allocator<>             alloc = bmqtst::TestHelperUtil::allocator();
    mqbplug::Action_ConnectAdmin obj1;

    EXPECT_TRUE(obj1 == obj1);
    EXPECT_FALSE(obj1 != obj1);

    // All objects of this type are equal to each other
    mqbplug::Action_ConnectAdmin obj2;
    EXPECT_FALSE(obj1 != obj2);
    EXPECT_TRUE(obj1 == obj2);
}

TEST(Action_ConnectClusterNode, equalityTest)
{
    bsl::allocator<> alloc = bmqtst::TestHelperUtil::allocator();
    mqbplug::Action_ConnectClusterNode obj1("test1");

    EXPECT_TRUE(obj1 == obj1);
    EXPECT_FALSE(obj1 != obj1);

    mqbplug::Action_ConnectClusterNode obj2("test2");
    EXPECT_TRUE(obj1 != obj2);
    EXPECT_FALSE(obj1 == obj2);
}

TEST(Action_QueueRead, equalityTest)
{
    bsl::allocator<>          alloc = bmqtst::TestHelperUtil::allocator();
    mqbplug::Action_QueueRead obj1("test1");

    EXPECT_TRUE(obj1 == obj1);
    EXPECT_FALSE(obj1 != obj1);

    mqbplug::Action_QueueRead obj2("test2");
    EXPECT_TRUE(obj1 != obj2);
    EXPECT_FALSE(obj1 == obj2);
}

TEST(Action_QueueWrite, equalityTest)
{
    bsl::allocator<>           alloc = bmqtst::TestHelperUtil::allocator();
    mqbplug::Action_QueueWrite obj1("test1");

    EXPECT_TRUE(obj1 == obj1);
    EXPECT_FALSE(obj1 != obj1);

    mqbplug::Action_QueueWrite obj2("test2");
    EXPECT_TRUE(obj1 != obj2);
    EXPECT_FALSE(obj1 == obj2);
}

TEST(Action_ExecuteAdminCommand, equalityTest)
{
    bsl::allocator<> alloc = bmqtst::TestHelperUtil::allocator();
    mqbplug::Action_ExecuteAdminCommand obj1("test1");

    EXPECT_TRUE(obj1 == obj1);
    EXPECT_FALSE(obj1 != obj1);

    mqbplug::Action_ExecuteAdminCommand obj2("test2");
    EXPECT_TRUE(obj1 != obj2);
    EXPECT_FALSE(obj1 == obj2);
}

// ========================================================================
//                                  MAIN
// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    ::testing::InitGoogleTest(&argc, argv);

    bmqtst::TestHelperUtil::testStatus() = RUN_ALL_TESTS();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
