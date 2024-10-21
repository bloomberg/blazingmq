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

// mqbc_clusterstateledgerprotocol.t.cpp                              -*-C++-*-
#include <mqbc_clusterstateledgerprotocol.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_limits.h>
#include <bsls_assert.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

const unsigned int k_UNSIGNED_INT_MAX =
    bsl::numeric_limits<unsigned int>::max();

const bsls::Types::Uint64 k_UINT64_MAX =
    bsl::numeric_limits<bsls::Types::Uint64>::max();

struct PrintTestData {
    int         d_line;
    int         d_type;
    const char* d_expected;
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_breathingTest()
// --------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    using namespace mqbc;

    {
        // -------------------------------------
        // ClusterStateFileHeader Breathing Test
        // -------------------------------------
        PV("ClusterStateFileHeader");

        // Create default ClusterStateFileHeader
        ClusterStateFileHeader fh;
        const unsigned char    numWords = sizeof(fh) / 4;

        ASSERT_EQ(fh.protocolVersion(), ClusterStateLedgerProtocol::k_VERSION);
        ASSERT_EQ(fh.headerWords(), numWords);
        ASSERT_EQ(fh.fileKey(), mqbu::StorageKey::k_NULL_KEY);

        // Create ClusterStateFileHeader, set fields, assert fields
        mqbu::StorageKey       key(mqbu::StorageKey::BinaryRepresentation(),
                             "12345");
        ClusterStateFileHeader fh2;
        fh2.setProtocolVersion(3).setHeaderWords(63).setFileKey(key);

        ASSERT_EQ(fh2.protocolVersion(), 3U);
        ASSERT_EQ(fh2.headerWords(), 63U);
        ASSERT_EQ(fh2.fileKey(), key);
    }

    {
        // ---------------------------------------
        // ClusterStateRecordHeader Breathing Test
        // ---------------------------------------
        PV("ClusterStateRecordHeader");

        // Create default ClusterStateRecordHeader
        ClusterStateRecordHeader fh;
        const unsigned int       numWords = sizeof(fh) / 4;

        ASSERT_EQ(fh.headerWords(), numWords);
        ASSERT_EQ(fh.recordType(), ClusterStateRecordType::e_UNDEFINED);
        ASSERT_EQ(fh.leaderAdvisoryWords(), 0U);
        ASSERT_EQ(fh.electorTerm(), 0ULL);
        ASSERT_EQ(fh.sequenceNumber(), 0ULL);
        ASSERT_EQ(fh.timestamp(), 0ULL);

        // Create ClusterStateRecordHeader, set fields, assert fields
        ClusterStateRecordHeader fh2;
        fh2.setHeaderWords(15)
            .setRecordType(ClusterStateRecordType::e_COMMIT)
            .setLeaderAdvisoryWords(k_UNSIGNED_INT_MAX)
            .setElectorTerm(k_UINT64_MAX)
            .setSequenceNumber(k_UINT64_MAX)
            .setTimestamp(k_UINT64_MAX);

        ASSERT_EQ(fh2.headerWords(), 15U);
        ASSERT_EQ(fh2.recordType(), ClusterStateRecordType::e_COMMIT);
        ASSERT_EQ(fh2.leaderAdvisoryWords(), k_UNSIGNED_INT_MAX);
        ASSERT_EQ(fh2.electorTerm(), k_UINT64_MAX);
        ASSERT_EQ(fh2.sequenceNumber(), k_UINT64_MAX);
        ASSERT_EQ(fh2.timestamp(), k_UINT64_MAX);
    }
}

template <typename ENUM_TYPE, typename ARRAY, int SIZE>
static void printEnumHelper(ARRAY (&data)[SIZE])
{
    for (size_t idx = 0; idx < SIZE; ++idx) {
        const PrintTestData& test = data[idx];

        PVVV("Line [" << test.d_line << "]");

        bmqu::MemOutStream out(s_allocator_p);
        bmqu::MemOutStream expected(s_allocator_p);

        typedef typename ENUM_TYPE::Enum T;

        T obj = static_cast<T>(test.d_type);

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        ENUM_TYPE::print(out, obj, 0, -1);

        ASSERT_EQ(out.str(), "");

        out.clear();
        ENUM_TYPE::print(out, obj, 0, -1);

        ASSERT_EQ(out.str(), expected.str());

        out.reset();
        out << obj;

        ASSERT_EQ(out.str(), expected.str());
    }
}

static void test2_enumPrint()
{
    // --------------------------------------------------------------------
    // ENUM LAYOUT
    //
    // Concerns:
    //   Check that enums print methods work correct
    //
    // Plan:
    //   1. For every type of enum for which there is a corresponding stream
    //      operator and print method check that layout of each enum value.
    //      equal to expected value:
    //      1.1. Check layout of stream operator
    //      1.2. Check layout of print method
    //   2. Check that layout for invalid value is equal to expected.
    //
    // Testing:
    //   ClusterStateRecordType::print
    //   operator<<(bsl::ostream&, ClusterStateRecordType::Enum)
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("ENUM LAYOUT");

    PV("Test mqbc::ClusterStateRecordType printing");
    {
        BSLMF_ASSERT(mqbc::ClusterStateRecordType::k_LOWEST_SUPPORTED_TYPE ==
                     mqbc::ClusterStateRecordType::e_SNAPSHOT);

        BSLMF_ASSERT(mqbc::ClusterStateRecordType::k_HIGHEST_SUPPORTED_TYPE ==
                     mqbc::ClusterStateRecordType::e_ACK);

        PrintTestData k_DATA[] = {
            {L_, mqbc::ClusterStateRecordType::e_UNDEFINED, "UNDEFINED"},
            {L_, mqbc::ClusterStateRecordType::e_SNAPSHOT, "SNAPSHOT"},
            {L_, mqbc::ClusterStateRecordType::e_UPDATE, "UPDATE"},
            {L_, mqbc::ClusterStateRecordType::e_COMMIT, "COMMIT"},
            {L_, mqbc::ClusterStateRecordType::e_ACK, "ACK"},
            {L_, -1, "(* UNKNOWN *)"}};

        printEnumHelper<mqbc::ClusterStateRecordType>(k_DATA);
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
    case 2: test2_enumPrint(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
