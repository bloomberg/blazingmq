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

// bmqpi_dtcontext.t.cpp                                              -*-C++-*-
#include <bmqpi_dtcontext.h>

// BMQ
#include <bmqpi_dtspan.h>

// BDE
#include <bsl_memory.h>
#include <bslma_managedptr.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;

/// A test implementation of the `bmqpi::DTContext` protocol.
struct DTContextTestImp : public bsls::ProtocolTestImp<bmqpi::DTContext> {
    bsl::shared_ptr<bmqpi::DTSpan> span() const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bslma::ManagedPtr<void>
    scope(const bsl::shared_ptr<bmqpi::DTSpan>&) BSLS_KEYWORD_OVERRIDE;
};

// Define one of DTContextTestImp methods out-of-line, to instruct the compiler
// to bake the class's vtable into *this* translation unit.
bslma::ManagedPtr<void>
DTContextTestImp::scope(const bsl::shared_ptr<bmqpi::DTSpan>&)
{
    markDone();
    return bslma::ManagedPtr<void>();
}

// ============================================================================
//                                    TESTS
// ============================================================================

static void test1_breathingTest()
// ------------------------------------------------------------------------
// PROTOCOL TEST:
//   Ensure this class is a properly defined protocol class.
//
// Plan:
//: 1 Define a concrete derived implementation, 'DTContextTestImp', of the
//:   protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'DTContextTestImp', and use it to verify that:
//:
//:   1 The protocol is abstract. (C-1)
//:
//:   2 The protocol has no data members. (C-2)
//:
//:   3 The protocol has a virtual destructor. (C-3)
//:
//: 3 Use the 'BSLS_PROTOCOLTEST_ASSERT' macro to verify that
//:   non-creator methods of the protocol are:
//:
//:   1 virtual, (C-4)
//:
//:   2 publicly accessible. (C-5)
//
// Testing:
//   PROTOCOL TEST
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // The default allocator check fails in this test case because the
    // 'markDone' methods of AbstractSession may sometimes return a
    // memory-aware object without utilizing the parameter allocator.

    mwctst::TestHelper::printTestName("BREATHING TEST");

    PV("Creating a concrete object");
    bsls::ProtocolTest<DTContextTestImp> context;

    PV("Verify that the protocol is abstract");
    ASSERT(context.testAbstract());

    PV("Verify that there are no data members");
    ASSERT(context.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    ASSERT(context.testVirtualDestructor());

    PV("Verify that all methods are public and virtual");

    BSLS_PROTOCOLTEST_ASSERT(context, span());
    BSLS_PROTOCOLTEST_ASSERT(context, scope(NULL));
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
