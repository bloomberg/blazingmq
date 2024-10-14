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

// mqbs_memoryblock.t.cpp                                             -*-C++-*-
#include <mqbs_memoryblock.h>

// BDE
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Testing:
//   Verifies the default and parameterized constructors of
//   'mqbs::MemoryBlock'.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Breathing Test");

    {
        PV("DefaultContructor");
        mqbs::MemoryBlock obj;
        ASSERT_EQ(obj.size(), 0U);
        ASSERT_EQ(obj.base(), static_cast<const char*>(0U));
    }

    {
        PV("Parametrized Constructor");
        char                      base = 'a';
        const bsls::Types::Uint64 size = 9876;
        mqbs::MemoryBlock         obj(&base, size);
        ASSERT_EQ(obj.base(), &base);
        ASSERT_EQ(obj.size(), size);

        PV("clear");
        obj.clear();
        ASSERT_EQ(obj.size(), 0U);
        ASSERT_EQ(obj.base(), static_cast<const char*>(0U));
    }
}

static void test2_operations()
// ------------------------------------------------------------------------
// Operations Test
//
// Testing:
//   Verifies the manipulators and accessors for the 'size' and 'base'
//   attributes.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("size operations");

    char                      base    = 1;
    char                      newBase = 2;
    const bsls::Types::Uint64 size    = 10000;
    const bsls::Types::Uint64 newSize = 1000;

    mqbs::MemoryBlock obj(&base, size);

    // 'size()' should reflect the size used in constructor
    ASSERT_EQ(obj.base(), &base);
    ASSERT_EQ(obj.size(), size);

    // 'setSize()' should change the size to the new expected size.
    obj.setSize(newSize);
    ASSERT_EQ(obj.size(), newSize);

    // 'setBase()' should change the base to the new expected address.
    obj.setBase(&newBase);
    ASSERT_EQ(obj.base(), &newBase);
}

static void test3_reset()
// ------------------------------------------------------------------------
// Reset Test
//
// Testing:
//   Verifies the effects of 'reset()' of a
//   'mqbs::MemoryBlock'.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("reset operation");

    mqbs::MemoryBlock obj;
    ASSERT_EQ(obj.base(), static_cast<const char*>(0U));
    ASSERT_EQ(obj.size(), 0U);
    obj.clear();

    char                      dummy = 1;
    const bsls::Types::Uint64 size  = 10000;

    // 'reset()' should set the size and base to the expected values.
    obj.reset(&dummy, size);
    ASSERT_EQ(obj.size(), size);
    ASSERT_EQ(obj.base(), &dummy);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_reset(); break;
    case 2: test2_operations(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
