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

#include <bmqt_tlsprotocolversion.h>

// BMQ
#include <bmqt_resultcode.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bsl_algorithm.h>

// TEST_DRIVER
#include <bmqtst_testhelper.h>
#include <bslstl_iterator.h>
#include <gtest/gtest.h>

// CONVENIENCE
using namespace BloombergLP;

namespace {
}

TEST(TlsProtocolVersion, printRepresentation)
{
    struct TestCase {
        bmqt::TlsProtocolVersion::Value d_input;
        const char*                     d_expected;
    };

    struct Locals {
        static void check(const TestCase& testCase)
        {
            bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
            bmqt::TlsProtocolVersion::print(out, testCase.d_input, 0, -1);
            bsl::string printed(out.str(),
                                bmqtst::TestHelperUtil::allocator());
            EXPECT_STREQ(testCase.d_expected, printed.c_str());
        }
    };

    TestCase k_DATA[] = {
        {bmqt::TlsProtocolVersion::e_TLS1_3, "[ \"TLSv1.3\" ]"}};
    bsl::for_each(bsl::cbegin(k_DATA), bsl::cend(k_DATA), Locals::check);
}

TEST(TlsProtocolVersion, fromAsciiInvertsToAscii)
{
    struct TestCase {
        bmqt::TlsProtocolVersion::Value d_input;
    };

    struct Locals {
        static void check(const TestCase& testCase)
        {
            bmqt::TlsProtocolVersion::Value converted;
            bool result = bmqt::TlsProtocolVersion::fromAscii(
                &converted,
                bmqt::TlsProtocolVersion::toAscii(testCase.d_input));
            EXPECT_TRUE(result);
            EXPECT_EQ(testCase.d_input, converted);
        }
    };

    TestCase k_DATA[] = {{bmqt::TlsProtocolVersion::e_TLS1_3}};
    bsl::for_each(bsl::cbegin(k_DATA), bsl::cend(k_DATA), Locals::check);
}

TEST(TlsProtocolVersion, toAsciiInvertsFromAscii)
{
    struct TestCase {
        const char* d_input;
    };

    struct Locals {
        static void check(const TestCase& testCase)
        {
            bmqt::TlsProtocolVersion::Value converted;
            EXPECT_TRUE(bmqt::TlsProtocolVersion::fromAscii(&converted,
                                                            testCase.d_input));
            EXPECT_STREQ(testCase.d_input,
                         bmqt::TlsProtocolVersion::toAscii(converted));
        }
    };

    TestCase k_DATA[] = {{"TLSv1.3"}};
    bsl::for_each(bsl::cbegin(k_DATA), bsl::cend(k_DATA), Locals::check);
}

TEST(TlsProtocolVersion, badAsciiGivesErrorCode)
{
    bmqt::TlsProtocolVersion::Value out;
    bool result = bmqt::TlsProtocolVersion::fromAscii(&out, "garbage");
    EXPECT_FALSE(result);
}

TEST(TlsProtocolVersionUtil, parsesMinTlsVersion)
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    struct TestCase {
        const char*                     d_input;
        bmqt::TlsProtocolVersion::Value d_expected;
    };

    struct Locals {
        static void check(const TestCase& testCase)
        {
            bmqt::TlsProtocolVersion::Value out;
            bmqt::TlsProtocolVersionUtil::parseMinTlsVersion(&out,
                                                             testCase.d_input);
            EXPECT_EQ(testCase.d_expected, out);
        }
    };

    TestCase k_DATA[] = {
        {"TLSv1.3", bmqt::TlsProtocolVersion::e_TLS1_3},
        {"TLSv1.3,TLSv1.3", bmqt::TlsProtocolVersion::e_TLS1_3},
        {"garbage,TLSv.13", bmqt::TlsProtocolVersion::e_TLS1_3}};
    bsl::for_each(bsl::cbegin(k_DATA), bsl::cend(k_DATA), Locals::check);
}

TEST(TlsProtocolVersionUtil, badVersionStringGivesErrorCode)
{
    struct TestCase {
        const char* d_input;
    };

    struct Locals {
        static void check(const TestCase& testCase)
        {
            bmqt::TlsProtocolVersion::Value out;
            int rc = bmqt::TlsProtocolVersionUtil::parseMinTlsVersion(
                &out,
                testCase.d_input);
            EXPECT_NE(0, rc)
                << "Value parsed successfully: " << testCase.d_input;
        }
    };

    TestCase k_DATA[] = {{"garbage"}, {""}, {",,,"}};
    bsl::for_each(bsl::cbegin(k_DATA), bsl::cend(k_DATA), Locals::check);
}
// ========================================================================
//                                  MAIN
// ========================================================================

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    ::testing::InitGoogleTest(&argc, argv);

    bmqtst::TestHelperUtil::testStatus() = RUN_ALL_TESTS();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
