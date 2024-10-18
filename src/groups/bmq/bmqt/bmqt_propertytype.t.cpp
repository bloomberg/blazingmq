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

// bmqt_propertytype.t.cpp                                            -*-C++-*-
#include <bmqt_propertytype.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqu_memoutstream.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    PV("Testing PropertyType");
    bmqt::PropertyType::Enum obj;
    bsl::string              str;
    bool                     res;

    PV("Testing toAscii");
    str = bmqt::PropertyType::toAscii(bmqt::PropertyType::e_BINARY);
    ASSERT_EQ(str, "BINARY");

    obj = static_cast<bmqt::PropertyType::Enum>(-1);
    str = bmqt::PropertyType::toAscii(obj);
    ASSERT_EQ(str, "(* UNKNOWN *)");

    PV("Testing fromAscii");
    res = bmqt::PropertyType::fromAscii(&obj, "STRING");
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, bmqt::PropertyType::e_STRING);
    res = bmqt::PropertyType::fromAscii(&obj, "blahblah");
    ASSERT_EQ(res, false);

    PV("Testing: fromAscii(toAscii(value)) = value");
    res = bmqt::PropertyType::fromAscii(
        &obj,
        bmqt::PropertyType::toAscii(bmqt::PropertyType::e_INT64));
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, bmqt::PropertyType::e_INT64);

    PV("Testing: toAscii(fromAscii(value)) = value");
    res = bmqt::PropertyType::fromAscii(&obj, "INT64");
    ASSERT_EQ(res, true);
    str = bmqt::PropertyType::toAscii(obj);
    ASSERT_EQ(str, "INT64");
}

static void test2_printTest()
{
    bmqtst::TestHelper::printTestName("PRINT");
    PV("Testing print");
    bmqu::MemOutStream stream(s_allocator_p);
    stream << bmqt::PropertyType::e_INT64;
    ASSERT_EQ(stream.str(), "INT64");
    stream.reset();
    bmqt::PropertyType::print(stream, bmqt::PropertyType::e_INT64, 0, 0);
    ASSERT_EQ(stream.str(), "INT64\n");
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_printTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
