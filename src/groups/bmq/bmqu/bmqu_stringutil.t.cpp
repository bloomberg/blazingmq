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

#include <bmqu_stringutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_contains()
// ------------------------------------------------------------------------
// bmqu::StringUtil::contains
//
// Concerns:
//   Ensure proper behavior of the 'contains' method.
//
// Plan:
//   Test various strings, focusing on edge cases.
//
// Testing:
//   Proper behavior of the 'contains(str, substr)' method.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("contains");

    struct Test {
        int         d_line;
        const char* d_str;
        const char* d_substr;
        bool        d_result;
    } k_DATA[] = {{L_, "", "", true},
                  {L_, "no", "", true},
                  {L_, "", "no", false},
                  {L_, "atTheStart", "atT", true},
                  {L_, "atTheEnd", "End", true},
                  {L_, "inTheMiddle", "The", true},
                  {L_, "beginFail", "begiN", false},
                  {L_, "endFail", "ailN", false},
                  {L_, "middleFail", "dleN", false}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking if '" << test.d_substr << "' "
                        << "is a substring of '" << test.d_str << "'");

        bsl::string str(test.d_str, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ_D("line " << test.d_line,
                           bmqu::StringUtil::contains(str, test.d_substr),
                           test.d_result);
    }
}

static void test2_trim()
// ------------------------------------------------------------------------
// bmqu::StringUtil::trim
//
// Concerns:
//   Ensure proper behavior of the 'trim' method.
//
// Plan:
//   Test various strings, focusing on edge cases.
//
// Testing:
//   Proper behavior of the 'trim(str)' method.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("trim");

    struct Test {
        int         d_line;
        const char* d_str;
        const char* d_expected;
    } k_DATA[] = {{L_, "", ""},
                  {L_, "abc", "abc"},
                  {L_, "abc ", "abc"},
                  {L_, " abc", "abc"},
                  {L_, " abc ", "abc"},
                  {L_, "  abc   ", "abc"},
                  {L_, "   a b c    ", "a b c"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": trimming '" << test.d_str << "'");

        bsl::string input(test.d_str, bmqtst::TestHelperUtil::allocator());
        bmqu::StringUtil::trim(&input);
        BMQTST_ASSERT_EQ_D("line " << test.d_line, input, test.d_expected);
    }
}

static void test3_ltrim()
// ------------------------------------------------------------------------
// bmqu::StringUtil::ltrim
//
// Concerns:
//   Ensure proper behavior of the 'ltrim' method.
//
// Plan:
//   Test various strings, focusing on edge cases.
//
// Testing:
//   Proper behavior of the 'ltrim(str)' method.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ltrim");

    struct Test {
        int         d_line;
        const char* d_str;
        const char* d_expected;
    } k_DATA[] = {{L_, "", ""},
                  {L_, "abc", "abc"},
                  {L_, "abc ", "abc "},
                  {L_, " abc", "abc"},
                  {L_, " abc ", "abc "},
                  {L_, "  abc   ", "abc   "},
                  {L_, "   a b c  ", "a b c  "}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": ltrimming '" << test.d_str << "'");

        bsl::string input(test.d_str, bmqtst::TestHelperUtil::allocator());
        bmqu::StringUtil::ltrim(&input);
        BMQTST_ASSERT_EQ_D("line " << test.d_line, input, test.d_expected);
    }
}

static void test4_rtrim()
// ------------------------------------------------------------------------
// bmqu::StringUtil::rtrim
//
// Concerns:
//   Ensure proper behavior of the 'rtrim' method.
//
// Plan:
//   Test various strings, focusing on edge cases.
//
// Testing:
//   Proper behavior of the 'rtrim(str)' method.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("rtrim");

    struct Test {
        int         d_line;
        const char* d_str;
        const char* d_expected;
    } k_DATA[] = {{L_, "", ""},
                  {L_, "abc", "abc"},
                  {L_, "abc ", "abc"},
                  {L_, " abc", " abc"},
                  {L_, " abc ", " abc"},
                  {L_, "  abc   ", "  abc"},
                  {L_, "   a b c  ", "   a b c"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": rtrimming '" << test.d_str << "'");

        bsl::string input(test.d_str, bmqtst::TestHelperUtil::allocator());
        bmqu::StringUtil::rtrim(&input);
        BMQTST_ASSERT_EQ_D("line " << test.d_line, input, test.d_expected);
    }
}

static void test5_strTokenizeRef()
// ------------------------------------------------------------------------
// bmqu::StringUtil::strTokenizeRef
//
// Concerns:
//   Ensure proper behavior of the 'strTokenizeRef' method.
//
// Plan:
//   Test various strings, focusing on edge cases.
//
// Testing:
//   Proper behavior of the 'strTokenizeRef(str, delims)' method.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // The vector returned by 'bmqu::StringUtil::strTokenizeRef' uses the
    // default allocator.

    bmqtst::TestHelper::printTestName("strTokenizeRef");

    bsl::string                    string(bmqtst::TestHelperUtil::allocator());
    bsl::vector<bslstl::StringRef> tokens(bmqtst::TestHelperUtil::allocator());

    struct Test {
        int         d_line;
        const char* d_input;
        const char* d_delims;
        size_t      d_nbTokens;
        const char* d_token1;
        const char* d_token2;
        const char* d_token3;
    } k_DATA[] = {
        {L_, "", "", 0, "", "", ""},
        {L_, "", ":,", 0, "", "", ""},
        {L_, "hello", "", 1, "hello", "", ""},
        {L_, "hello", ":,", 1, "hello", "", ""},
        {L_, "hello:bonjour:shalom", ":,", 3, "hello", "bonjour", "shalom"},
        {L_, "hello,bonjour,shalom", ":,", 3, "hello", "bonjour", "shalom"},
        {L_, ":", ":,", 2, "", "", ""},
        {L_, "::", ":,", 3, "", "", ""},
        {L_, "bonjour::hello", ":,", 3, "bonjour", "", "hello"},
        {L_, ":bonjour:hello", ":,", 3, "", "bonjour", "hello"},
        {L_, "bonjour:hello:", ":,", 3, "bonjour", "hello", ""}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": tokenizing '" << test.d_input << "'");

        bsl::string input(test.d_input, bmqtst::TestHelperUtil::allocator());
        tokens = bmqu::StringUtil::strTokenizeRef(input, test.d_delims);

        BMQTST_ASSERT_EQ_D("line " << test.d_line,
                           tokens.size(),
                           test.d_nbTokens);
        if (test.d_nbTokens >= 1) {
            BMQTST_ASSERT_EQ_D("line " << test.d_line,
                               tokens[0],
                               test.d_token1);
        }
        if (test.d_nbTokens >= 2) {
            BMQTST_ASSERT_EQ_D("line " << test.d_line,
                               tokens[1],
                               test.d_token2);
        }
        if (test.d_nbTokens >= 3) {
            BMQTST_ASSERT_EQ_D("line " << test.d_line,
                               tokens[2],
                               test.d_token3);
        }
    }
}

static void test6_match()
// ------------------------------------------------------------------------
// bmqu::StringUtil::match
//
// Concerns:
//   Ensure proper behavior of the 'match' method.
//
// Plan:
//   Test various strings, focusing on edge cases.
//
// Testing:
//   Proper behavior of the 'match(str, pattern)' method.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("match");

    struct Test {
        int         d_line;
        const char* d_input;
        const char* d_pattern;
        bool        d_expected;
    } k_DATA[] = {// No wildcards
                  {L_, "hello", "hello", true},
                  {L_, "hello", "hellX", false},
                  {L_, "hello", "helloX", false},
                  {L_, "hello", "heXllo", false},
                  {L_, "hello", "Xhello", false},

                  // ?
                  {L_, "hello", "he?lo", true},
                  {L_, "hello", "?ello", true},
                  {L_, "hello", "hell?", true},
                  {L_, "hello", "he??o", true},
                  {L_, "hello", "?????", true},
                  {L_, "he?lo", "he?lo", true},
                  {L_, "hello", "he?o", false},
                  {L_, "hello", "he?Xo", false},
                  {L_, "hello", "hello?", false},

                  // *
                  {L_, "hello", "*", true},
                  {L_, "hello", "he*", true},
                  {L_, "hello", "he*o", true},
                  {L_, "hello", "he*lo", true},
                  {L_, "hello", "*llo", true},
                  {L_, "hello", "h*l*o", true},
                  {L_, "hello", "h*l*", true},
                  {L_, "hello", "*ll*", true},
                  {L_, "hello", "*llo*", true},
                  {L_, "hello", "hello*", true},
                  {L_, "hello", "hel*lo", true},
                  {L_, "hello", "*hello", true},
                  {L_, "hello", "*lo", true},
                  {L_, "hello", "*o", true},
                  {L_, "hello", "h*lo", true},
                  {L_, "hello", "h*o", true},
                  {L_, "hello", "*Xlo", false},
                  {L_, "hello", "h*Xlo", false},
                  {L_, "hello", "*llX", false},
                  {L_, "hello", "*loX", false},
                  {L_, "hello", "*lX", false},
                  {L_, "hello", "*l", false},
                  {L_, "hello", "*ll", false},
                  {L_, "hello", "hellX*", false},
                  {L_, "hello", "hello*X", false},
                  {L_, "hello", "hel*Xo", false},
                  {L_, "hello", "*helloX", false},
                  {L_, "hello", "*hellX", false},
                  {L_, "foobarbar", "foo*bar", true},

                  // ? and *
                  {L_, "hello", "?*", true},
                  {L_, "hello", "??*", true},
                  {L_, "hello", "h?*", true},
                  {L_, "hello", "h??*", true},
                  {L_, "hello", "?e*", true},
                  {L_, "hello", "?e*o", true},

                  // multiple *
                  {L_, "hello", "**", true},
                  {L_, "hello", "h*****o", true},

                  // empty 'str', 'pattern', or both
                  {L_, "", "fish bone", false},
                  {L_, "fish bone", "", false},
                  {L_, "", "", true}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": matching '" << test.d_input << "'" << " with '"
                        << test.d_pattern << "'");

        BMQTST_ASSERT_EQ_D(
            "line " << test.d_line << ": when matching '" << test.d_input
                    << "' against the pattern '" << test.d_pattern << "'",
            bmqu::StringUtil::match(test.d_input, test.d_pattern),
            test.d_expected);
    }
}

static void test7_squeeze()
// ------------------------------------------------------------------------
// bmqu::StringUtil::squeeze
//
// Concerns:
//   Ensure proper behavior of the 'squeeze' method.
//
// Plan:
//   Test various strings, focusing on edge cases.
//
// Testing:
//   Proper behavior of the 'squeeze(str, characters)' method.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("squeeze");

    struct Test {
        int         d_line;
        const char* d_str;
        const char* d_characters;
        const char* d_expected;
    } k_DATA[] = {// usage example 1
                  {L_, "hello   there spaces", " ", "hello there spaces"},
                  // usage example 2
                  {L_, "mississippi", "ps", "misisipi"},
                  // usage example 3
                  {L_, "wakka", "", "wakka"},
                  // empty 'str'
                  {L_, "", "whatever", ""},
                  {L_, "", "", ""},
                  // one-character 'str'
                  {L_, "x", "whatever", "x"},
                  {L_, "x", "x", "x"},
                  // realistic example
                  {L_,
                   "DOMAINS   DOMAIN foo QUEUE bar   LIST baz 0   10",
                   " ",
                   "DOMAINS DOMAIN foo QUEUE bar LIST baz 0 10"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];
        bsl::string str(test.d_str, bmqtst::TestHelperUtil::allocator());

        PVV(test.d_line << ": squeeze(\"" << str << "\", \""
                        << test.d_characters << "\")");

        BMQTST_ASSERT_EQ_D("line " << test.d_line,
                           bmqu::StringUtil::squeeze(&str, test.d_characters),
                           test.d_expected);
    }
}

static void test8_isPrintable()
// ------------------------------------------------------------------------
// bmqu::StringUtil::isPrintable
//
// Concerns:
//   Ensure proper behavior of the 'isPrintable' method.
//
// Plan:
//   Test printable strings and the empty string, plus strings containing
//   various non-printable bytes (control characters, DEL, and high-bit
//   bytes), including one with an embedded NUL.
//
// Testing:
//   Proper behavior of the 'isPrintable(str)' method.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("isPrintable");

    struct Test {
        int         d_line;
        const char* d_str;
        bool        d_result;
    } k_DATA[] = {{L_, "", true},
                  {L_, "hello world", true},
                  {L_, " ", true},  // 0x20, first printable
                  {L_, "~", true},  // 0x7e, last printable
                  {L_, "abc\ndef", false},
                  {L_, "abc\tdef", false},
                  {L_, "\r", false},
                  {L_, "\x1f", false},   // just below first printable
                  {L_, "\x7f", false},   // DEL
                  {L_, "\x80", false},   // high-bit byte
                  {L_, "\xff", false}};  // high-bit byte

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking printability of line");
        BMQTST_ASSERT_EQ_D("line " << test.d_line,
                           bmqu::StringUtil::isPrintable(test.d_str),
                           test.d_result);
    }

    // A NUL embedded mid-string must be detected, so pass an explicit length
    // rather than relying on NUL-termination.
    const char k_EMBEDDED_NUL[] = {'a', '\0', 'b'};
    BMQTST_ASSERT_EQ(
        bmqu::StringUtil::isPrintable(
            bslstl::StringRef(k_EMBEDDED_NUL, sizeof(k_EMBEDDED_NUL))),
        false);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 8: test8_isPrintable(); break;
    case 7: test7_squeeze(); break;
    case 6: test6_match(); break;
    case 5: test5_strTokenizeRef(); break;
    case 4: test4_rtrim(); break;
    case 3: test3_ltrim(); break;
    case 2: test2_trim(); break;
    case 1: test1_contains(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
