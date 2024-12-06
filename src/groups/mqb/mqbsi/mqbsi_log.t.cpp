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

// mqbsi_log.t.cpp                                                    -*-C++-*-
#include <mqbsi_log.h>

// MQB
#include <mqbu_storagekey.h>

// BDE
#include <bdlbb_blob.h>
#include <bsls_protocoltest.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// Disable some compiler warning for simplified write of the 'LogTestImp'.
#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wweak-vtables"
// Disabling 'weak-vtables' so that we can define all interface methods
// inline, without the following warning:
//..
//  mqbsi_log.t.cpp:44:1: error: 'LogTestImp' has no out-of-line virtual
//  method definitions; its vtable will be emitted in every translation
//  unit [-Werror,-Wweak-vtables]
//..
#endif  // BSLS_PLATFORM_CMP_CLANG

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

// ALIASES
typedef mqbsi::Log::Offset Offset;

// CLASSES
// =================
// struct LogTestImp
// =================

/// A test implementation of the `mqbsi::Log` protocol.
struct LogTestImp : bsls::ProtocolTestImp<mqbsi::Log> {
  private:
    // DATA
    mqbsi::LogConfig d_config;

  public:
    // CREATORS
    LogTestImp()
    : d_config(0, mqbu::StorageKey(), bmqtst::TestHelperUtil::allocator())
    {
        // NOTHING
    }

    // MANIPULATORS
    int open(int flags) BSLS_KEYWORD_OVERRIDE { return markDone(); }

    int close() BSLS_KEYWORD_OVERRIDE { return markDone(); }

    int seek(mqbsi::Log::Offset offset) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    void
    updateOutstandingNumBytes(bsls::Types::Int64 value) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    void setOutstandingNumBytes(bsls::Types::Int64 value) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    Offset
    write(const void* entry, int offset, int length) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    Offset write(const bdlbb::Blob&        entry,
                 const bmqu::BlobPosition& offset,
                 int                       length) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    Offset write(const bdlbb::Blob&       entry,
                 const bmqu::BlobSection& section) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int flush(Offset offset = 0) BSLS_KEYWORD_OVERRIDE { return markDone(); }

    int
    read(void* entry, int length, Offset offset) const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int read(bdlbb::Blob* entry,
             int          length,
             Offset       offset) const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int
    alias(void** entry, int length, Offset offset) const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int alias(bdlbb::Blob* entry,
              int          length,
              Offset       offset) const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bool isOpened() const BSLS_KEYWORD_OVERRIDE { return markDone(); }

    bsls::Types::Int64 totalNumBytes() const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bsls::Types::Int64 outstandingNumBytes() const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    Offset currentOffset() const BSLS_KEYWORD_OVERRIDE { return markDone(); }

    const mqbsi::LogConfig& logConfig() const BSLS_KEYWORD_OVERRIDE
    {
        markDone();
        return d_config;
    }

    bool supportsAliasing() const BSLS_KEYWORD_OVERRIDE { return markDone(); }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_log_protocol()
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
//: 1 Define a concrete derived implementation, 'Log', of the protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'Log', and use it to verify
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
    bmqtst::TestHelper::printTestName("LOG");

    PV("Creating a test object");
    bsls::ProtocolTest<LogTestImp> testObj(
        bmqtst::TestHelperUtil::verbosityLevel() > 2);

    PV("Verify that the protocol is abstract");
    BMQTST_ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    BMQTST_ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    BMQTST_ASSERT(testObj.testVirtualDestructor());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_log_protocol(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}

#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic pop
#endif
