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
#include <bdlbb_pooledblobbufferfactory.h>
#include <bmqa_messageproperties.h>
#include <mwcu_memoutstream.h>
#include <mwcu_tempfile.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>

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

static void test1_decodeHexDumpTest()
// ------------------------------------------------------------------------
// DECODEHEXDUMP TEST
//
// Concerns:
//   Proper behavior of the 'decodeHexDump' method.
//
// Plan:
//   Verify that the 'decodeHexDump' method returns the correct return code,
//   output and error details for every applicable scenario.
//
// Testing:
//   decodeHexDump
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("DECODEHEXDUMP TEST");

    struct Test {
        int         d_line;
        const char* d_hexdumpStr;
        const char* d_expectedError;
        bool        d_expectedRc;
        const char* d_expectedOutput;
    } k_DATA[] = {
        // Empty dump
        {L_, "", "", true, ""},
        {L_, "\n\n\n", "", true, ""},
        // Correct dump
        {L_,
         "     0:    68656C6C 6F20776F 726C6421      |hello world!|",
         "",
         true,
         "hello world!"},
        {L_,
         "     0:    68656C6C 6F20776F 726C6421      |hello world!|\n\nsome "
         "text",
         "",
         true,
         "hello world!"},
        // Wrong dump
        {L_,
         "foo",
         "Wrong hexdump format, space delimeter is not detected",
         false,
         ""},
        {L_,
         "     0:    68656C6C 6F20776F 726C6421 WRONG     |hello world!|",
         "HexDecoder convert error: -1",
         false,
         "hello world!"},
        {L_,
         "     0:    68656C6C6F20776F 726C6421      |hello world!|",
         "Wrong hexdump format, block size is greater than 8: 16",
         false,
         ""},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bsl::istringstream input(bsl::string(test.d_hexdumpStr, s_allocator_p),
                                 s_allocator_p);
        mwcu::MemOutStream output(s_allocator_p);
        mwcu::MemOutStream error(s_allocator_p);
        bool               rc =
            InputUtil::decodeHexDump(&output, &error, input, s_allocator_p);
        // Check rc
        ASSERT_EQ_D(test.d_line, rc, test.d_expectedRc);
        // Check error
        ASSERT_EQ_D(test.d_line, error.str(), test.d_expectedError);
        // Check output
        ASSERT_EQ_D(test.d_line, output.str(), test.d_expectedOutput);
    }
}

static void test2_loadMessageFromFileTest()
// ------------------------------------------------------------------------
// LOADMESSAGEDROMFILE TEST
//
// Concerns:
//   Proper behavior of the 'loadMessageFromFile' method.
//
// Plan:
//   Verify that the 'loadMessageFromFile' method returns the correct return
//   code, output and error details for every applicable scenario.
//
// Testing:
//   loadMessageFromFile
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("LOADMESSAGEDROMFILE TEST");

    struct Test {
        int         d_line;
        const char* d_fileContent;
        const char* d_expectedError;
        bool        d_expectedRc;
        const char* d_expectedPayload;
    } k_DATA[] = {
        // Bad formed file format
        {L_,
         "foo bar",
         "Unexpected file format, either 'Message Properties:' or 'Message "
         "Payload:' expected",
         false,
         ""},
        {L_,
         "Message Properties:\n\n{",
         "Properties '[' marker missed",
         false,
         ""},
        {L_,
         "Message Properties:\n\n[ foo bar",
         "Properties ']' marker missed",
         false,
         ""},
        {L_,
         "Message Properties:\n\n[ foo bar ]\n"
         "\n\nNot Message Properties hexdump:\n\n",
         "Unexpected file format, 'Message Properties hexdump:' expected",
         false,
         ""},
        {L_,
         "Message Properties:\n\n[ sample_str (STRING) = \"foo baz bar\" x "
         "(INT32) = 10 char_data (CHAR) 7F  short_data (SHORT) -32767 "
         "int64_data (INT64) = 100 ]"
         "\n\n\nMessage Properties hexdump:\n\n"
         "     0:   1B00001A 00050800 00010009 14000008     "
         "|................|\n"
         "    16:   000A1800 000B000A 0C000002 000A1000     "
         "|................|\n"
         "    32:   00040001 63686172 5F646174 617F696E     "
         "|....char_data.in|\n"
         "    48:   7436345F 64617461 00000000 00000064     "
         "|t64_data.......d|\n"
         "    64:   73616D70 6C655F73 7472666F 6F206261     |sample_strfoo "
         "ba|\n"
         "    80:   7A206261 7273686F 72745F64 61746180     |z "
         "barshort_data.|\n"
         "    96:   01780000 000A0202                       |.x......        "
         "|\n"
         "\n\nNot Message Payload:\n\n",
         "Unexpected file format, 'Message Payload:' expected",
         false,
         ""},

        // Well formed file format

        // Without properties
        {L_,
         "Application Data:\n\n"
         "     0:    68656C6C 6F20776F 726C6421      |hello world!|",
         "",
         true,
         "hello world!"},

        // With properties
        {L_,
         "Message Properties:\n\n[ sample_str (STRING) = \"foo baz bar\" x "
         "(INT32) = 10 char_data (CHAR) 7F short_data (SHORT) -32767 "
         "int64_data (INT64) = 100 ]"
         "\n\n\nMessage Properties hexdump:\n\n"
         "     0:   1B00001A 00050800 00010009 14000008     "
         "|................|\n"
         "    16:   000A1800 000B000A 0C000002 000A1000     "
         "|................|\n"
         "    32:   00040001 63686172 5F646174 617F696E     "
         "|....char_data.in|\n"
         "    48:   7436345F 64617461 00000000 00000064     "
         "|t64_data.......d|\n"
         "    64:   73616D70 6C655F73 7472666F 6F206261     |sample_strfoo "
         "ba|\n"
         "    80:   7A206261 7273686F 72745F64 61746180     |z "
         "barshort_data.|\n"
         "    96:   01780000 000A0202                       |.x......        "
         "|\n"
         "\n\nMessage Payload:\n\n"
         "     0:    68656C6C 6F20776F 726C6421      |hello world!|",
         "",
         true,
         "hello world!"},
        // With multiline properties string
        {L_,
         "Message Properties:\n\n[ sample_str (STRING) = \"foo baz bar\"\n x "
         "(INT32) = 10 char_data (CHAR) 7F\n short_data (SHORT) -32767 "
         "int64_data (INT64) = 100 ]"
         "\n\n\nMessage Properties hexdump:\n\n"
         "     0:   1B00001A 00050800 00010009 14000008     "
         "|................|\n"
         "    16:   000A1800 000B000A 0C000002 000A1000     "
         "|................|\n"
         "    32:   00040001 63686172 5F646174 617F696E     "
         "|....char_data.in|\n"
         "    48:   7436345F 64617461 00000000 00000064     "
         "|t64_data.......d|\n"
         "    64:   73616D70 6C655F73 7472666F 6F206261     |sample_strfoo "
         "ba|\n"
         "    80:   7A206261 7273686F 72745F64 61746180     |z "
         "barshort_data.|\n"
         "    96:   01780000 000A0202                       |.x......        "
         "|\n"
         "\n\nMessage Payload:\n\n"
         "     0:    68656C6C 6F20776F 726C6421      |hello world!|",
         "",
         true,
         "hello world!"},
    };

    // Check wrong file path
    {
        mwcu::MemOutStream stream(s_allocator_p);
        mwcu::MemOutStream error(s_allocator_p);
        bool               rc = InputUtil::loadMessageFromFile(&stream,
                                                 &stream,
                                                 &error,
                                                 "wrongFilePath",
                                                 s_allocator_p);
        ASSERT_EQ(rc, false);
        ASSERT_EQ(error.str(), "Failed to open file: wrongFilePath");
    }

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        // Create temp file and write content
        mwcu::TempFile    tempFile(s_allocator_p);
        const bsl::string filePath(tempFile.path(), s_allocator_p);
        {
            bsl::ofstream ofs(filePath.c_str());
            BSLS_ASSERT(ofs.is_open());
            ofs << test.d_fileContent;
            // ofs.close();
        }

        mwcu::MemOutStream payload(s_allocator_p);
        bsl::ostringstream properties(s_allocator_p);
        mwcu::MemOutStream error(s_allocator_p);
        bool               rc = InputUtil::loadMessageFromFile(&payload,
                                                 &properties,
                                                 &error,
                                                 filePath,
                                                 s_allocator_p);

        // bsl::cout << "PAYLOAD: " << " SIZE: " << payload.str().size() <<
        // '\n'; bsl::cout << "PROPS: "  << " SIZE: " <<
        // properties.str().size() << '\n';

        // Check rc
        ASSERT_EQ_D(test.d_line, rc, test.d_expectedRc);
        // Check error
        ASSERT_EQ_D(test.d_line, error.str(), test.d_expectedError);
        // Check payload
        ASSERT_EQ_D(test.d_line, payload.str(), test.d_expectedPayload);
        // Check properties (deserialize into properties instance)
        bmqa::MessageProperties        messageProperties(s_allocator_p);
        bdlbb::PooledBlobBufferFactory bufferFactory(128, s_allocator_p);
        bdlbb::Blob                    blob(&bufferFactory, s_allocator_p);
        bdlbb::BlobUtil::append(&blob,
                                properties.str().c_str(),
                                static_cast<int>(properties.str().size()));
        ASSERT_EQ_D(test.d_line, messageProperties.streamIn(blob), 0);
    }
}

// static void test1_parsePropertiesTest()
// // ------------------------------------------------------------------------
// // PARSEPROPERTIES TEST
// //
// // Concerns:
// //   Proper behavior of the 'parseProperties' method.
// //
// // Plan:
// //   Verify that the 'parseProperties' method returns the correct return
// code
// //   error details for every applicable scenario.
// //
// // Testing:
// //   parseProperties
// // ------------------------------------------------------------------------
// {
//     mwctst::TestHelper::printTestName("PARSEPROPERTIES TEST");

//     struct Test {
//         int         d_line;
//         const char* d_propertiesStr;
//         const char* d_expectedError;
//         bool        d_expectedRc;
//         size_t      d_expectedNumProperties;
//     } k_DATA[] = {
//         // Badly formed property string
//         {L_,
//          "",
//          "Unexpected empty properties string, use empty brackets '[ ]' "
//          "instead",
//          false,
//          0},
//         {L_, "some prop ]", "Expected open marker '[' missed", false, 0},
//         {L_, "[ some prop ", "Expected close marker ']' missed", false, 0},
//         {L_,
//          "[ foo (WRONG_TYPE) = 1 ]",
//          "Failed to decode MessagePropertyType: (WRONG_TYPE)",
//          false,
//          0},
//         // Well formed property string
//         {L_, "[ ]", "", true, 0},
//         {L_, "[ foo (STRING) = \"foo string\" ]", "", true, 1},
//         {L_, "[ foo (INT32) = 123 ]", "", true, 1},
//         {L_, "[ foo (INT64) = 123 ]", "", true, 1},
//         {L_, "[ foo (CHAR) = 7F ]", "", true, 1},
//         {L_, "[ foo (SHORT) = 123 ]", "", true, 1},
//         {L_, "[ foo (BOOL) = true ]", "", true, 1},
//         {L_, "[ foo (BINARY) = \" 0:  6262   |bb   |\n\" ]", "", true, 1},
//         {L_, "[ foo (INT32) = 123 bar (INT32) = 456 ]", "", true, 2},
//     };

//     const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
//     for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
//         const Test& test = k_DATA[idx];

//         bsl::vector<MessageProperty> parsedProperties(s_allocator_p);
//         mwcu::MemOutStream           error(s_allocator_p);
//         bsl::string propertiesStr(test.d_propertiesStr, s_allocator_p);
//         // Check return code
//         ASSERT_EQ_D(test.d_line,
//                     InputUtil::parseProperties(&parsedProperties,
//                                                propertiesStr,
//                                                s_allocator_p,
//                                                &error),
//                     test.d_expectedRc);
//         // Check error description
//         ASSERT_EQ_D(test.d_line, error.str(), test.d_expectedError);
//         // Check number of properties
//         ASSERT_EQ_D(test.d_line,
//                     parsedProperties.size(),
//                     test.d_expectedNumProperties);
//     }
// }

// static void test2_populatePropertiesTest()
// // ------------------------------------------------------------------------
// // POPULATEPROPERTIES TEST
// //
// // Concerns:
// //   Proper behavior of the 'populateProperties' method.
// //
// // Plan:
// //   Verify that the 'populateProperties' method returns the correct return
// //   code error details for every applicable scenario.
// //
// // Testing:
// //   populateProperties
// // ------------------------------------------------------------------------
// {
//     mwctst::TestHelper::printTestName("POPULATEPROPERTIES TEST");

//     struct Test {
//         int         d_line;
//         const char* d_name;
//         const char* d_type;
//         const char* d_value;
//         const char* d_expectedPropertyString;
//     } k_DATA[] = {
//         {L_,
//          "foo",
//          "E_STRING",
//          "foo string",
//          "[ foo (STRING) = \"foo string\" ]"},
//     };

//     const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
//     for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
//         const Test& test = k_DATA[idx];

//         bmqa::MessageProperties      properties(s_allocator_p);
//         bsl::vector<MessageProperty> inputProperties(s_allocator_p);

//         MessageProperty messageProperty(s_allocator_p);
//         messageProperty.name() = test.d_name;
//         MessagePropertyType::fromString(&messageProperty.type(),
//         test.d_type); messageProperty.value() = test.d_value;

//         inputProperties.push_back(messageProperty);

//         InputUtil::populateProperties(&properties, inputProperties);
//         mwcu::MemOutStream result;
//         result << properties;
//         ASSERT_EQ_D(test.d_line, result.str(),
//         test.d_expectedPropertyString);
//     }
// }

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_decodeHexDumpTest(); break;
    case 2: test2_loadMessageFromFileTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    // TODO: due to mwcu::TempFile tempFile
    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
    // TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);
}
