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

// mqbc_clusterstateledgeriterator.t.cpp                              -*-C++-*-
#include <mqbc_clusterstateledgeriterator.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbmock_clusterstateledgeriterator.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_ostream.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// Disable some compiler warning for simplified write of the
// 'ClusterStateLedgerIteratorTestImp'.
#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wweak-vtables"
// Disabling 'weak-vtables' so that we can define all interface methods
// inline, without the following warning:
//..
//  mqbc_clusterstateledgeriterator.t.cpp:34:1: error:
//  'ClusterStateLedgerIteratorTestImp' has no out-of-line virtual method
//  definitions; its vtable will be emitted in every translation unit
//  [-Werror,-Wweak-vtables]
//..
#endif  // BSLS_PLATFORM_CMP_CLANG

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------
namespace {

/// A test implementation of the `mqbc::ClusterStateLedgerIterator` protocol
struct ClusterStateLedgerIteratorTestImp
: bsls::ProtocolTestImp<mqbc::ClusterStateLedgerIterator> {
    mqbc::ClusterStateRecordHeader      d_header;
    bmqp_ctrlmsg::LeaderMessageSequence d_leaderMessageSequence;

  public:
    // MANIPULATORS
    int next() BSLS_KEYWORD_OVERRIDE { return markDone(); }

    void
    copy(const mqbc::ClusterStateLedgerIterator& other) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    // ACCESSORS
    bslma::ManagedPtr<ClusterStateLedgerIterator>
    clone(bslma::Allocator* allocator) const BSLS_KEYWORD_OVERRIDE
    {
        markDone();
        return bslma::ManagedPtr<ClusterStateLedgerIterator>();
    }

    bool isValid() const BSLS_KEYWORD_OVERRIDE { return markDone(); }

    const mqbc::ClusterStateRecordHeader& header() const BSLS_KEYWORD_OVERRIDE
    {
        markDone();
        return d_header;
    }

    int loadClusterMessage(bmqp_ctrlmsg::ClusterMessage* message) const
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bsl::ostream& print(bsl::ostream& stream,
                        int           level = 0,
                        int spacesPerLevel  = 4) const BSLS_KEYWORD_OVERRIDE
    {
        markDone();
        return stream;
    }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_clusterStateLedgerIterator_protocol()
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
//:   'ClusterStateLedgerIteratorTestImp', of the protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'ClusterStateLedgerIteratorTestImp', and use it to
//:   verify that:
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
    bmqtst::TestHelper::printTestName("CLUSTER STATE LEDGER ITERATOR - "
                                      "PROTOCOL TEST");

    PV("Creating a test object");
    bsls::ProtocolTest<ClusterStateLedgerIteratorTestImp> testObj(
        s_verbosityLevel > 2);

    PV("Verify that the protocol is abstract");
    ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        mqbmock::ClusterStateLedgerIterator::LedgerRecords records(
            s_allocator_p);
        mqbmock::ClusterStateLedgerIterator cslIter(records);
        bmqp_ctrlmsg::ClusterMessage        clusterMessage;
        bmqu::MemOutStream                  os;
        BSLS_PROTOCOLTEST_ASSERT(testObj, copy(cslIter));
        BSLS_PROTOCOLTEST_ASSERT(testObj, clone(0));
        BSLS_PROTOCOLTEST_ASSERT(testObj, next());
        BSLS_PROTOCOLTEST_ASSERT(testObj, isValid());
        BSLS_PROTOCOLTEST_ASSERT(testObj, header());
        BSLS_PROTOCOLTEST_ASSERT(testObj, loadClusterMessage(&clusterMessage));
        BSLS_PROTOCOLTEST_ASSERT(testObj, print(os));
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
    case 1: test1_clusterStateLedgerIterator_protocol(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
