// Copyright 2025 Bloomberg Finance L.P.
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

// bmqu_alignedprinter.t.cpp                                         -*-C++-*-
#include <bmqu_alignedprinter.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_string.h>
#include <bsl_vector.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_basicUsage()
// ------------------------------------------------------------------------
// BASIC USAGE
//
// Concerns:
//   Ensure proper behavior of 'AlignedPrinter' with string vector fields.
//
// Plan:
//   1. Create AlignedPrinter with string vector fields
//   2. Print various types of values
//   3. Verify output formatting is correct
//
// Testing:
//   Basic functionality with bsl::vector<bsl::string>
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BASIC USAGE");

    bmqu::MemOutStream       stream;
    bsl::vector<bsl::string> fields;
    fields.emplace_back("Queue URI");
    fields.emplace_back("QueueKey");
    fields.emplace_back("Number of AppIds");

    const int            indent = 4;
    bmqu::AlignedPrinter printer(stream, &fields, indent);

    // Test printing various types
    bsl::string uri       = "bmq://bmq.tutorial.workqueue/sample-queue";
    bsl::string queueKey  = "sample";
    const int   numAppIds = 1;

    printer << uri << queueKey << numAppIds;

    bsl::string output = stream.str();

    // Verify output contains expected elements
    BMQTST_ASSERT(output.find("Queue URI") != bsl::string::npos);
    BMQTST_ASSERT(output.find("QueueKey") != bsl::string::npos);
    BMQTST_ASSERT(output.find("Number of AppIds") != bsl::string::npos);
    BMQTST_ASSERT(output.find(uri) != bsl::string::npos);
    BMQTST_ASSERT(output.find(queueKey) != bsl::string::npos);
    BMQTST_ASSERT(output.find("1") != bsl::string::npos);
}

static void test2_alignment()
// ------------------------------------------------------------------------
// ALIGNMENT
//
// Concerns:
//   Ensure proper alignment of fields with different lengths.
//
// Plan:
//   1. Create fields with varying lengths
//   2. Print values and verify alignment
//
// Testing:
//   Field alignment with mixed field name lengths
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALIGNMENT");

    bmqu::MemOutStream       stream;
    bsl::vector<bsl::string> fields;
    fields.emplace_back("Short");
    fields.emplace_back("Very Long Field Name");
    fields.emplace_back("Med");

    bmqu::AlignedPrinter printer(stream, &fields, 2);

    printer << "value1" << "value2" << "value3";

    bsl::string output = stream.str();

    // Verify all field names are present
    BMQTST_ASSERT(output.find("Short") != bsl::string::npos);
    BMQTST_ASSERT(output.find("Very Long Field Name") != bsl::string::npos);
    BMQTST_ASSERT(output.find("Med") != bsl::string::npos);

    // Basic structure check - should contain colons for alignment
    BMQTST_ASSERT(output.find(": value1") != bsl::string::npos);
    BMQTST_ASSERT(output.find(": value2") != bsl::string::npos);
    BMQTST_ASSERT(output.find(": value3") != bsl::string::npos);
}

static void test3_emptyFields()
// ------------------------------------------------------------------------
// EMPTY FIELDS
//
// Concerns:
//   Behavior when fields vector is empty (should assert in debug mode)
//
// Plan:
//   Verify the assertion behavior for empty fields
//
// Testing:
//   Precondition validation
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EMPTY FIELDS");

    bmqu::MemOutStream       stream;
    bsl::vector<bsl::string> fields;  // Empty vector

#ifdef BDE_BUILD_TARGET_SAFE_2
    // In safe mode, this should assert
    BMQTST_ASSERT_SAFE_FAIL(bmqu::AlignedPrinter(stream, &fields, 4));
#endif
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_emptyFields(); break;
    case 2: test2_alignment(); break;
    case 1: test1_basicUsage(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
}