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

// mqbc_clusterstateledger.t.cpp                                      -*-C++-*-
#include <mqbc_clusterstateledger.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslmf_assert.h>
#include <bsls_annotation.h>
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// Disable some compiler warning for simplified write of the
// 'ClusterStateLedgerTestImp'.
#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wweak-vtables"
// Disabling 'weak-vtables' so that we can define all interface methods
// inline, without the following warning:
//..
//  mqbc_clusterstateledger.t.cpp:44:1: error: 'ClusterStateLedgerTestImp'
// has no out-of-line virtual method definitions; its vtable will be
// emitted in every translation unit [-Werror,-Wweak-vtables]
//..
#endif  // BSLS_PLATFORM_CMP_CLANG

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------
namespace {

/// A test implementation of the `mqbc::ClusterStateLedger` protocol
struct ClusterStateLedgerTestImp
: bsls::ProtocolTestImp<mqbc::ClusterStateLedger> {
  public:
    // MANIPULATORS
    int open() BSLS_KEYWORD_OVERRIDE { return markDone(); }

    int close() BSLS_KEYWORD_OVERRIDE { return markDone(); }

    int apply(const bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int apply(const bmqp_ctrlmsg::QueueAssignmentAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int apply(const bmqp_ctrlmsg::QueueUnassignedAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int apply(const bmqp_ctrlmsg::QueueUpdateAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int
    apply(const bmqp_ctrlmsg::LeaderAdvisory& advisory) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int apply(const bmqp_ctrlmsg::ClusterMessage& clusterMessage)
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int apply(const bdlbb::Blob&   record,
              mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    void
    setIsFirstLeaderAdvisory(BSLS_ANNOTATION_UNUSED bool isFirstLeaderAdvisory)
        BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    // ACCESSORS
    void setCommitCb(BSLS_ANNOTATION_UNUSED const CommitCb& value)
        BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    bool isOpen() const BSLS_KEYWORD_OVERRIDE { return markDone(); }

    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator>
    getIterator() const BSLS_KEYWORD_OVERRIDE
    {
        markDone();
        return bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator>();
    }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_clusterStateLedger_protocol()
// ------------------------------------------------------------------------
// PROTOCOL TEST:
//   Ensure this class is a properly defined protocol.
//
// Concerns:
//: 1 The protocol has no data members.
//:
//: 2 The protocol has a virtual destructor.
//:
//: 3 All methods of the protocol are publicly accessible.
//
// Plan:
//: 1 Define a concrete derived implementation,
//:   'ClusterStateLedgerTestImp', of the protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'ClusterStateLedgerTestImp', and use it to verify
//:   that:
//:
//:   1 The protocol has no data members. (C-1)
//:
//:   2 The protocol has a virtual destructor. (C-2)
//:
//: 3 Use the 'BSLS_PROTOCOLTEST_ASSERT' macro to verify that
//:   non-creator methods of the protocol are:
//:
//:   1 publicly accessible. (C-3)
//
// Testing:
//   PROTOCOL TEST
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CLUSTER STATE LEDGER - PROTOCOL TEST");

    PV("Creating a test object");
    bsls::ProtocolTest<ClusterStateLedgerTestImp> testObj(s_verbosityLevel >
                                                          2);

    PV("Verify that the protocol is abstract");
    ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        BSLS_PROTOCOLTEST_ASSERT(testObj, open());
        BSLS_PROTOCOLTEST_ASSERT(testObj, close());
        BSLS_PROTOCOLTEST_ASSERT(
            testObj,
            apply(bmqp_ctrlmsg::PartitionPrimaryAdvisory()));
        BSLS_PROTOCOLTEST_ASSERT(
            testObj,
            apply(bmqp_ctrlmsg::QueueAssignmentAdvisory()));
        BSLS_PROTOCOLTEST_ASSERT(
            testObj,
            apply(bmqp_ctrlmsg::QueueUnassignedAdvisory()));
        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 apply(bmqp_ctrlmsg::QueueUpdateAdvisory()));
        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 apply(bmqp_ctrlmsg::LeaderAdvisory()));
        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 apply(bmqp_ctrlmsg::ClusterMessage()));
        BSLS_PROTOCOLTEST_ASSERT(testObj, setIsFirstLeaderAdvisory(true));
        BSLS_PROTOCOLTEST_ASSERT(
            testObj,
            setCommitCb(mqbc::ClusterStateLedger::CommitCb()));
        BSLS_PROTOCOLTEST_ASSERT(testObj, isOpen());
        BSLS_PROTOCOLTEST_ASSERT(testObj, getIterator());
    }

    // ClusterStateLedgerConsistency
    BSLMF_ASSERT(mqbc::ClusterStateLedgerConsistency::e_EVENTUAL !=
                 mqbc::ClusterStateLedgerConsistency::e_STRONG);
}

static void test2_commitStatus_fromAscii()
// ------------------------------------------------------------------------
// COMMIT STATUS - FROM ASCII
//
// Concerns:
//   Proper behavior of the 'ClusterStateLedgerCommitStatus::fromAscii'
//   method.
//
// Plan:
//   Verify that the 'fromAscii' method returns the string representation
//   of every enum value of 'ClusterStateLedgerCommitStatus::fromAscii'.
//
// Testing:
//   'ClusterStateLedgerCommitStatus::fromAscii'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("COMMIT STATUS - FROM ASCII");

    struct Test {
        int         d_line;
        const char* d_input;
        bool        d_isValid;
        int         d_expected;
    } k_DATA[] = {{L_, "SUCCESS", true, 0},
                  {L_, "CANCELED", true, -1},
                  {L_, "TIMEOUT", true, -2},
                  {L_, "invalid", false, -1}};
    // NOTE: Using the 'integer' value instead of the enum to ensure the
    //       numeric values are *never* changed.

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: fromAscii(" << test.d_input
                        << ") == " << test.d_expected);

        mqbc::ClusterStateLedgerCommitStatus::Enum obj;
        ASSERT_EQ_D(
            test.d_line,
            mqbc::ClusterStateLedgerCommitStatus::fromAscii(&obj,
                                                            test.d_input),
            test.d_isValid);
        if (test.d_isValid) {
            ASSERT_EQ_D(test.d_line, static_cast<int>(obj), test.d_expected);
        }
    }
}

static void test3_commitStatus_toAscii()
// ------------------------------------------------------------------------
// COMMIT STATUS - TO ASCII
//
// Concerns:
//   Proper behavior of the 'ClusterStateLedgerCommitStatus::toAscii'
//   method.
//
// Plan:
//   Verify that the 'toAscii' method returns the string representation of
//   every enum value of 'ClusterStateLedgerCommitStatus'.
//
// Testing:
//   'ClusterStateLedgerCommitStatus::toAscii'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("COMMIT STATUS - TO ASCII");

    struct Test {
        int         d_line;
        int         d_value;
        const char* d_expected;
    } k_DATA[] = {
        {L_, 0, "SUCCESS"},
        {L_, -1, "CANCELED"},
        {L_, -2, "TIMEOUT"},
    };
    // NOTE: Using the 'integer' value instead of the enum to ensure the
    //       numeric values are *never* changed.

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: toAscii(" << test.d_value
                        << ") == " << test.d_expected);

        bsl::string ascii(s_allocator_p);
        ascii = mqbc::ClusterStateLedgerCommitStatus::toAscii(
            mqbc::ClusterStateLedgerCommitStatus::Enum(test.d_value));

        ASSERT_EQ_D(test.d_line, ascii, test.d_expected);
    }
}

static void test4_commitStatus_print()
// ------------------------------------------------------------------------
// COMMIT STATUS - PRINT
//
// Concerns:
//   Proper behavior of the 'ClusterStateLedgerCommitStatus::print' method.
//
// Plan:
//   1. Verify that the 'print' and 'operator<<' methods output the
//      expected string representation of every enum value of
//      'ClusterStateLedgerCommitStatus'.
//   2. Verify that the 'print' method outputs nothing when the stream has
//      the bad bit set.
//
// Testing:
//   'ClusterStateLedgerCommitStatus::print'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("COMMIT STATUS - PRINT");

    // 1.
    struct Test {
        int         d_line;
        int         d_value;
        const char* d_expected;
    } k_DATA[] = {{L_, 0, "SUCCESS"},
                  {L_, -1, "CANCELED"},
                  {L_, -2, "TIMEOUT"},
                  {L_, -9, "(* UNKNOWN *)"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: print(" << test.d_value
                        << ") == " << test.d_expected);

        // 1.
        mwcu::MemOutStream                         out(s_allocator_p);
        mqbc::ClusterStateLedgerCommitStatus::Enum obj(
            static_cast<mqbc::ClusterStateLedgerCommitStatus::Enum>(
                test.d_value));

        // print
        mqbc::ClusterStateLedgerCommitStatus::print(out, obj, 0, 0);

        PVV(test.d_line << ": '" << out.str());

        bsl::string expected(s_allocator_p);
        expected.assign(test.d_expected);
        expected.append("\n");
        ASSERT_EQ_D(test.d_line, out.str(), expected);

        // operator<<
        out.reset();
        out << obj;

        ASSERT_EQ_D(test.d_line, out.str(), test.d_expected);

        // 2. 'badbit' set
        out.reset();
        out.setstate(bsl::ios_base::badbit);
        mqbc::ClusterStateLedgerCommitStatus::print(out, obj, 0, -1);

        ASSERT_EQ_D(test.d_line, out.str(), "");
    }
}

static void test5_clusterStateLedgerConsistency_fromAscii()
// ------------------------------------------------------------------------
// CLUSTER STATE LEDGER - FROM ASCII
//
// Concerns:
//   Proper behavior of the 'ClusterStateLedgerConsistency::fromAscii'
//   method.
//
// Plan:
//   Verify that the 'fromAscii' method returns the string representation
//   of every enum value of 'ClusterStateLedgerConsistency::fromAscii'.
//
// Testing:
//   'ClusterStateLedgerConsistency::fromAscii'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CLUSTER STATE LEDGER CONSISTENCY -"
                                      " FROM ASCII");

    struct Test {
        int         d_line;
        const char* d_input;
        bool        d_isValid;
        int         d_expected;
    } k_DATA[] = {
        {L_,
         "EVENTUAL",
         true,
         mqbc::ClusterStateLedgerConsistency ::e_EVENTUAL},
        {L_, "STRONG", true, mqbc::ClusterStateLedgerConsistency ::e_STRONG},
        {L_, "invalid", false, -1}};
    // NOTE: Using the 'integer' value instead of the enum to ensure the
    //       numeric values are *never* changed.

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: fromAscii(" << test.d_input
                        << ") == " << test.d_expected);

        mqbc::ClusterStateLedgerConsistency::Enum obj;
        ASSERT_EQ_D(
            test.d_line,
            mqbc::ClusterStateLedgerConsistency::fromAscii(&obj, test.d_input),
            test.d_isValid);
        if (test.d_isValid) {
            ASSERT_EQ_D(test.d_line, static_cast<int>(obj), test.d_expected);
        }
    }
}

static void test6_clusterStateLedgerConsistency_toAscii()
// ------------------------------------------------------------------------
// CLUSTER STATE LEDGER CONSISTENCY - TO ASCII
//
// Concerns:
//   Proper behavior of the 'ClusterStateLedgerConsistency::toAscii'
//   method.
//
// Plan:
//   Verify that the 'toAscii' method returns the string representation of
//   every enum value of 'ClusterStateLedgerConsistency'.
//
// Testing:
//   'ClusterStateLedgerConsistency::toAscii'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CLUSTER STATE LEDGER CONSISTENCY -"
                                      " TO ASCII");

    struct Test {
        int         d_line;
        int         d_value;
        const char* d_expected;
    } k_DATA[] = {
        {L_, mqbc::ClusterStateLedgerConsistency::e_EVENTUAL, "EVENTUAL"},
        {L_, mqbc::ClusterStateLedgerConsistency::e_STRONG, "STRONG"},
        {L_, -1, "(* UNKNOWN *)"},
    };
    // NOTE: Using the 'integer' value instead of the enum to ensure the
    //       numeric values are *never* changed.

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: toAscii(" << test.d_value
                        << ") == " << test.d_expected);

        bsl::string ascii(s_allocator_p);
        ascii = mqbc::ClusterStateLedgerConsistency::toAscii(
            mqbc::ClusterStateLedgerConsistency::Enum(test.d_value));

        ASSERT_EQ_D(test.d_line, ascii, test.d_expected);
    }
}

static void test7_clusterStateLedgerConsistency_print()
// ------------------------------------------------------------------------
// CLUSTER STATE LEDGER CONSISTENCY - PRINT
//
// Concerns:
//   Proper behavior of the 'ClusterStateLedgerConsistency::print' method.
//
// Plan:
//   1. Verify that the 'print' and 'operator<<' methods output the
//      expected string representation of every enum value of
//      'ClusterStateLedgerConsistency'.
//   2. Verify that the 'print' method outputs nothing when the stream has
//      the bad bit set.
//
// Testing:
//   'ClusterStateLedgerConsistency::print'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CLUSTER STATE LEDGER CONSISTENCY -"
                                      " PRINT");

    // 1.
    struct Test {
        int         d_line;
        int         d_value;
        const char* d_expected;
    } k_DATA[] = {
        {L_, mqbc::ClusterStateLedgerConsistency::e_EVENTUAL, "EVENTUAL"},
        {L_, mqbc::ClusterStateLedgerConsistency::e_STRONG, "STRONG"},
        {L_, -1, "(* UNKNOWN *)"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: print(" << test.d_value
                        << ") == " << test.d_expected);

        // 1.
        mwcu::MemOutStream                        out(s_allocator_p);
        mqbc::ClusterStateLedgerConsistency::Enum obj(
            static_cast<mqbc::ClusterStateLedgerConsistency::Enum>(
                test.d_value));

        // print
        mqbc::ClusterStateLedgerConsistency::print(out, obj, 0, 0);

        PVV(test.d_line << ": '" << out.str());

        bsl::string expected(s_allocator_p);
        expected.assign(test.d_expected);
        expected.append("\n");
        ASSERT_EQ_D(test.d_line, out.str(), expected);

        // operator<<
        out.reset();
        out << obj;

        ASSERT_EQ_D(test.d_line, out.str(), test.d_expected);

        // 2. 'badbit' set
        out.reset();
        out.setstate(bsl::ios_base::badbit);
        mqbc::ClusterStateLedgerConsistency::print(out, obj, 0, -1);

        ASSERT_EQ_D(test.d_line, out.str(), "");
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
    case 7: test7_clusterStateLedgerConsistency_print(); break;
    case 6: test6_clusterStateLedgerConsistency_toAscii(); break;
    case 5: test5_clusterStateLedgerConsistency_fromAscii(); break;
    case 4: test4_commitStatus_print(); break;
    case 3: test3_commitStatus_toAscii(); break;
    case 2: test2_commitStatus_fromAscii(); break;
    case 1: test1_clusterStateLedger_protocol(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
