// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_version.t.cpp                                                 -*-C++-*-
#include <bmqt_version.h>

// BMQ
#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// Some GNU header defines these functions as macros for POSIX
// compatibility
#ifdef major
#undef major
#endif

#ifdef minor
#undef minor
#endif

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
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   Constructor
//   Comparison operators
//   Accessors
//   Manipulators
//   Printing
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // Creators
    bmqt::Version a;
    ASSERT_EQ(a.major(), 0);
    ASSERT_EQ(a.minor(), 0);

    bmqt::Version b(1, 2);
    ASSERT_EQ(b.major(), 1);
    ASSERT_EQ(b.minor(), 2);

    // Equality
    ASSERT_EQ(a, a);
    ASSERT_EQ(b, b);
    ASSERT_NE(a, b);

    // Copy constructor
    bmqt::Version c(b);
    ASSERT_EQ(c, c);
    ASSERT_EQ(b, c);
    ASSERT_EQ(c.major(), 1);
    ASSERT_EQ(c.minor(), 2);

    // Manipulators and accessors
    c.setMinor(3);
    ASSERT_EQ(c.major(), 1);
    ASSERT_EQ(c.minor(), 3);
    ASSERT_NE(b, c);
    c.setMajor(4);
    ASSERT_EQ(c.major(), 4);
    ASSERT_EQ(c.minor(), 3);

    // Operator <
    bmqt::Version ab(1, 2);
    bmqt::Version ad(1, 4);
    bmqt::Version bb(2, 2);
    bmqt::Version be(2, 5);

    ASSERT_EQ(ab < ab, false);
    ASSERT_EQ(ab < ad, true);
    ASSERT_EQ(ab < bb, true);
    ASSERT_EQ(ab < be, true);
    ASSERT_EQ(ad < bb, true);
    ASSERT_EQ(ad < be, true);
    ASSERT_EQ(bb < be, true);

    ASSERT_EQ(ad < ab, false);
    ASSERT_EQ(bb < ab, false);
    ASSERT_EQ(be < ab, false);
    ASSERT_EQ(bb < ad, false);
    ASSERT_EQ(be < ad, false);
    ASSERT_EQ(be < bb, false);

    // Printing
    {
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
        os << a;
        bsl::string str(bmqtst::TestHelperUtil::allocator());
        str.assign(os.str().data(), os.str().length());
        ASSERT_EQ(str, "[ major = 0 minor = 0 ]");
    }
    {
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
        os << b;
        bsl::string str(bmqtst::TestHelperUtil::allocator());
        str.assign(os.str().data(), os.str().length());
        ASSERT_EQ(str, "[ major = 1 minor = 2 ]");
    }
    {
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
        os << c;
        bsl::string str(bmqtst::TestHelperUtil::allocator());
        str.assign(os.str().data(), os.str().length());
        ASSERT_EQ(str, "[ major = 4 minor = 3 ]");
    }
    {
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        out.setstate(bsl::ios_base::badbit);
        a.print(out, 0, -1);
        ASSERT_EQ(out.str(), "");
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
