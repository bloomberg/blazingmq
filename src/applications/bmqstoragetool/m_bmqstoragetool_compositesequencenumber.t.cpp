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

// bmqstoragetool
#include <m_bmqstoragetool_compositesequencenumber.h>

// BMQ
#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
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
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        CompositeSequenceNumber compositeSeqNum;
        ASSERT(compositeSeqNum.isUnset());
    }

    {
        CompositeSequenceNumber compositeSeqNum(1, 2);
        ASSERT(!compositeSeqNum.isUnset());
        ASSERT_EQ(compositeSeqNum.leaseId(), 1ul);
        ASSERT_EQ(compositeSeqNum.sequenceNumber(), 2ul);
    }
}

static void test2_fromStringTest()
// ------------------------------------------------------------------------
// FROM STRING TEST
//
// Concerns:
//   Exercise the functionality to initialize component from string
//   representation.
//
// Testing:
//   fromString method
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FROM STRING TEST");

    bmqu::MemOutStream errorDescription(s_allocator_p);

    // Valid string
    {
        CompositeSequenceNumber compositeSeqNum;

        bsl::string inputString("123-456", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), false);
        ASSERT_EQ(compositeSeqNum.leaseId(), 123u);
        ASSERT_EQ(compositeSeqNum.sequenceNumber(), 456u);
        ASSERT(errorDescription.str().empty());
    }

    // Valid string with leading zeros
    {
        CompositeSequenceNumber compositeSeqNum;

        bsl::string inputString("00123-000456", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), false);
        ASSERT_EQ(compositeSeqNum.leaseId(), 123u);
        ASSERT_EQ(compositeSeqNum.sequenceNumber(), 456u);
        ASSERT(errorDescription.str().empty());
    }

    // Empty string
    {
        CompositeSequenceNumber compositeSeqNum;

        bsl::string inputString("", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), true);
        ASSERT(!errorDescription.str().empty());
        ASSERT_EQ(errorDescription.str(), "Invalid input: empty string.");
    }

    // Invalid string with missed separator
    {
        CompositeSequenceNumber compositeSeqNum;

        bsl::string inputString("123456", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), true);
        ASSERT(!errorDescription.str().empty());
        ASSERT_EQ(errorDescription.str(),
                  "Invalid format: no '-' separator found.");
    }

    // Invalid string with wrong separator
    {
        CompositeSequenceNumber compositeSeqNum;

        bsl::string inputString("123_456", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), true);
        ASSERT(!errorDescription.str().empty());
        ASSERT_EQ(errorDescription.str(),
                  "Invalid format: no '-' separator found.");
    }

    // Invalid string with non-numeric value in first part
    {
        CompositeSequenceNumber compositeSeqNum;

        bsl::string inputString("1a23-456", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), true);
        ASSERT(!errorDescription.str().empty());
        ASSERT_EQ(errorDescription.str(),
                  "Invalid input: non-numeric values encountered.");
    }

    // Invalid string with non-numeric value in second part
    {
        CompositeSequenceNumber compositeSeqNum;

        bsl::string inputString("123-45a6", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), true);
        ASSERT(!errorDescription.str().empty());
        ASSERT_EQ(errorDescription.str(),
                  "Invalid input: non-numeric values encountered.");
    }

    // Invalid string with zero value in first part
    {
        CompositeSequenceNumber compositeSeqNum;

        bsl::string inputString("0-456", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), true);
        ASSERT(!errorDescription.str().empty());
        ASSERT_EQ(errorDescription.str(),
                  "Invalid input: zero values encountered.");
    }

    // Invalid string with zero value in second part
    {
        CompositeSequenceNumber compositeSeqNum;

        bsl::string inputString("123-0", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), true);
        ASSERT(!errorDescription.str().empty());
        ASSERT_EQ(errorDescription.str(),
                  "Invalid input: zero values encountered.");
    }

    // Invalid string with out of range value in first part
    {
        CompositeSequenceNumber compositeSeqNum;

        // Simulate unsigned int overflow
        bsl::string inputString("11111111111-123", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), true);
        ASSERT(!errorDescription.str().empty());
        ASSERT_EQ(errorDescription.str(),
                  "Invalid input: number out of range.");
    }

    // Invalid string with out of range value in second part
    {
        CompositeSequenceNumber compositeSeqNum;

        // Simulate bsls::Types::Uint64 overflow
        bsl::string inputString("123-111111111111111111111", s_allocator_p);
        errorDescription.reset();

        compositeSeqNum.fromString(errorDescription, inputString);
        ASSERT_EQ(compositeSeqNum.isUnset(), true);
        ASSERT(!errorDescription.str().empty());
        ASSERT_EQ(errorDescription.str(),
                  "Invalid input: number out of range.");
    }
}

static void test3_comparisonTest()
// ------------------------------------------------------------------------
// COMPARISON TEST
//
// Concerns:
//   Exercise the functionality to compare objects.
//
// Testing:
//   operator<() and operator<=() methods
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("COMPARISON TEST");

    bmqu::MemOutStream errorDescription(s_allocator_p);

    // leaseId is less, seqNumber is greater
    {
        CompositeSequenceNumber lhs;
        lhs.fromString(errorDescription, bsl::string("1-2", s_allocator_p));

        CompositeSequenceNumber rhs;
        rhs.fromString(errorDescription, bsl::string("2-1", s_allocator_p));

        ASSERT(!lhs.isUnset() && !rhs.isUnset());
        ASSERT(lhs < rhs);
    }

    // leaseId is less, seqNumber is less
    {
        CompositeSequenceNumber lhs;
        lhs.fromString(errorDescription, bsl::string("1-1", s_allocator_p));

        CompositeSequenceNumber rhs;
        rhs.fromString(errorDescription, bsl::string("2-2", s_allocator_p));

        ASSERT(!lhs.isUnset() && !rhs.isUnset());
        ASSERT(lhs < rhs);
    }

    // leaseId is greater, seqNumber is greater
    {
        CompositeSequenceNumber lhs;
        lhs.fromString(errorDescription, bsl::string("3-2", s_allocator_p));

        CompositeSequenceNumber rhs;
        rhs.fromString(errorDescription, bsl::string("2-1", s_allocator_p));

        ASSERT(!lhs.isUnset() && !rhs.isUnset());
        ASSERT_EQ((lhs < rhs), false);
    }

    // leaseId is greater, seqNumber is less
    {
        CompositeSequenceNumber lhs;
        lhs.fromString(errorDescription, bsl::string("3-1", s_allocator_p));

        CompositeSequenceNumber rhs;
        rhs.fromString(errorDescription, bsl::string("2-2", s_allocator_p));

        ASSERT(!lhs.isUnset() && !rhs.isUnset());
        ASSERT_EQ((lhs < rhs), false);
    }

    // leaseId is equal, seqNumber is less
    {
        CompositeSequenceNumber lhs;
        lhs.fromString(errorDescription, bsl::string("1-1", s_allocator_p));

        CompositeSequenceNumber rhs;
        rhs.fromString(errorDescription, bsl::string("1-2", s_allocator_p));

        ASSERT(!lhs.isUnset() && !rhs.isUnset());
        ASSERT(lhs < rhs);
    }

    // leaseId is equal, seqNumber is greater
    {
        CompositeSequenceNumber lhs;
        lhs.fromString(errorDescription, bsl::string("1-2", s_allocator_p));

        CompositeSequenceNumber rhs;
        rhs.fromString(errorDescription, bsl::string("1-1", s_allocator_p));

        ASSERT(!lhs.isUnset() && !rhs.isUnset());
        ASSERT_EQ((lhs < rhs), false);
    }

    // Compare for equality using '<=': leaseId is equal, seqNumber is equal
    {
        CompositeSequenceNumber lhs;
        lhs.fromString(errorDescription, bsl::string("1-2", s_allocator_p));

        CompositeSequenceNumber rhs;
        rhs.fromString(errorDescription, bsl::string("1-2", s_allocator_p));

        ASSERT(!lhs.isUnset() && !rhs.isUnset());
        ASSERT_EQ((lhs <= rhs), true);
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
    case 2: test2_fromStringTest(); break;
    case 3: test3_comparisonTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
