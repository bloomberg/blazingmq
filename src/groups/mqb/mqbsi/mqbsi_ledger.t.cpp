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

// mqbsi_ledger.t.cpp                                                 -*-C++-*-
#include <mqbsi_ledger.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslmf_assert.h>
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace mqbsi;

// Disable some compiler warning for simplified write of the
// 'LedgerTestImp'.
#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wweak-vtables"
// Disabling 'weak-vtables' so that we can define all interface methods
// inline, without the following warning:
//..
//  mqbc_clusterstateledger.t.cpp:44:1: error: 'LedgerTestImp'
// has no out-of-line virtual method definitions; its vtable will be
// emitted in every translation unit [-Werror,-Wweak-vtables]
//..
#endif  // BSLS_PLATFORM_CMP_CLANG

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------
namespace {

/// A test implementation of the `mqbsi::Ledger` protocol
struct LedgerTestImp : bsls::ProtocolTestImp<mqbsi::Ledger> {
    LogSp d_logSp;
    Logs  d_logs;

  public:
    // MANIPULATORS
    int open(int flags) BSLS_KEYWORD_OVERRIDE { return markDone(); }

    int close() BSLS_KEYWORD_OVERRIDE { return markDone(); }

    int
    updateOutstandingNumBytes(const mqbu::StorageKey& logId,
                              bsls::Types::Int64 value) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int setOutstandingNumBytes(const mqbu::StorageKey& logId,
                               bsls::Types::Int64 value) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int writeRecord(LedgerRecordId* recordId,
                    const void*     record,
                    int             offset,
                    int             length) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int writeRecord(LedgerRecordId*           recordId,
                    const bdlbb::Blob&        record,
                    const bmqu::BlobPosition& offset,
                    int                       length) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int writeRecord(LedgerRecordId*          recordId,
                    const bdlbb::Blob&       record,
                    const bmqu::BlobSection& section) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int flush() BSLS_KEYWORD_OVERRIDE { return markDone(); }

    int readRecord(void*                 entry,
                   int                   length,
                   const LedgerRecordId& recordId) const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int readRecord(bdlbb::Blob*          entry,
                   int                   length,
                   const LedgerRecordId& recordId) const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int aliasRecord(void**                entry,
                    int                   length,
                    const LedgerRecordId& recordId) const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int aliasRecord(bdlbb::Blob*          entry,
                    int                   length,
                    const LedgerRecordId& recordId) const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bool isOpened() const BSLS_KEYWORD_OVERRIDE { return markDone(); }

    bool supportsAliasing() const BSLS_KEYWORD_OVERRIDE { return markDone(); }

    size_t numLogs() const BSLS_KEYWORD_OVERRIDE { return markDone(); }

    const Logs& logs() const BSLS_KEYWORD_OVERRIDE
    {
        markDone();
        return d_logs;
    }

    const Ledger::LogSp& currentLog() const BSLS_KEYWORD_OVERRIDE
    {
        markDone();
        return d_logSp;
    }

    bsls::Types::Int64 outstandingNumBytes(const mqbu::StorageKey& logId) const
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bsls::Types::Int64
    totalNumBytes(const mqbu::StorageKey& logId) const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_ledger_protocol()
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
//:   'Ledger', of the protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'Ledger', and use it to verify
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
    bmqtst::TestHelper::printTestName("LEDGER");

    PV("Creating a test object");
    bsls::ProtocolTest<LedgerTestImp> testObj(s_verbosityLevel > 2);

    PV("Verify that the protocol is abstract");
    ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    ASSERT(testObj.testVirtualDestructor());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_ledger_protocol(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}

#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic pop
#endif
