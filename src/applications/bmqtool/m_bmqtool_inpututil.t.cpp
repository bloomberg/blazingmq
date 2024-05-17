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

// bmqtool
#include <m_bmqtool_inpututil.h>

// BMQ
#include <mwcu_memoutstream.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqtool;
using namespace bsl;
// using namespace mqbs;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_parsePropertiesTest()
// ------------------------------------------------------------------------
// PARSEPROPERTIES TEST
//
// Concerns:
//   Proper behavior of the 'parseProperties' method.
//
// Plan:
//   Verify that the 'parseProperties' method returns the correct return code
//   error details for every applicable scenario.
//
// Testing:
//   parseProperties
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PARSEPROPERTIES TEST");

    struct Test {
        int         d_line;
        const char* d_propertiesStr;
        const char* d_expectedError;
        bool        d_expectedRc;
        size_t      d_expectedNumProperties;
    } k_DATA[] = {
        // Badly formed property string
        {L_,
         "",
         "Unexpected empty properties string, use empty brackets '[ ]' "
         "instead",
         false,
         0},
        {L_, "some prop ]", "Expected open marker '[' missed", false, 0},
        {L_, "[ some prop ", "Expected close marker ']' missed", false, 0},
        {L_,
         "[ foo (WRONG_TYPE) = 1 ]",
         "Failed to decode MessagePropertyType: (WRONG_TYPE)",
         false,
         0},
        // Well formed property string
        {L_, "[ ]", "", true, 0},
        {L_, "[ foo (STRING) = \"foo string\" ]", "", true, 1},
        {L_, "[ foo (INT32) = 123 ]", "", true, 1},
        {L_, "[ foo (INT64) = 123 ]", "", true, 1},
        {L_, "[ foo (CHAR) = 7F ]", "", true, 1},
        {L_, "[ foo (SHORT) = 123 ]", "", true, 1},
        {L_, "[ foo (BOOL) = true ]", "", true, 1},
        {L_, "[ foo (BINARY) = \" 0:  6262   |bb   |\n\" ]", "", true, 1},
        {L_, "[ foo (INT32) = 123 bar (INT32) = 456 ]", "", true, 2},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bsl::vector<MessageProperty> parsedProperties(s_allocator_p);
        mwcu::MemOutStream           error(s_allocator_p);
        bsl::string propertiesStr(test.d_propertiesStr, s_allocator_p);
        // Check return code
        ASSERT_EQ_D(test.d_line,
                    InputUtil::parseProperties(&parsedProperties,
                                               propertiesStr,
                                               s_allocator_p,
                                               &error),
                    test.d_expectedRc);
        // Check error description
        ASSERT_EQ_D(test.d_line, error.str(), test.d_expectedError);
        // Check number of properties
        ASSERT_EQ_D(test.d_line,
                    parsedProperties.size(),
                    test.d_expectedNumProperties);
    }
}

static void test2_populatePropertiesTest()
// ------------------------------------------------------------------------
// POPULATEPROPERTIES TEST
//
// Concerns:
//   Proper behavior of the 'populateProperties' method.
//
// Plan:
//   Verify that the 'populateProperties' method returns the correct return
//   code error details for every applicable scenario.
//
// Testing:
//   populateProperties
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("POPULATEPROPERTIES TEST");

    struct Test {
        int         d_line;
        const char* d_name;
        const char* d_type;
        const char* d_value;
        const char* d_expectedPropertyString;
    } k_DATA[] = {
        {L_,
         "foo",
         "E_STRING",
         "foo string",
         "[ foo (STRING) = \"foo string\" ]"},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqa::MessageProperties      properties(s_allocator_p);
        bsl::vector<MessageProperty> inputProperties(s_allocator_p);

        MessageProperty messageProperty(s_allocator_p);
        messageProperty.name() = test.d_name;
        MessagePropertyType::fromString(&messageProperty.type(), test.d_type);
        messageProperty.value() = test.d_value;

        inputProperties.push_back(messageProperty);

        InputUtil::populateProperties(&properties, inputProperties);
        mwcu::MemOutStream result;
        result << properties;
        ASSERT_EQ_D(test.d_line, result.str(), test.d_expectedPropertyString);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_parsePropertiesTest(); break;
    // case 2: test2_populatePropertiesTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
