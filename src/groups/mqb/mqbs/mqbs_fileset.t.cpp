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

// mqbs_fileset.t.cpp                                                 -*-C++-*-
#include <mqbs_fileset.h>

// MQB
#include <mqbs_filestore.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbu_storagekey.h>

// BDE
#include <bsls_atomic.h>
#include <bsls_types.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
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
    mwctst::TestHelper::printTestName("BREATHING TEST");

    // Default constructor
    mqbs::FileSet obj(static_cast<mqbs::FileStore*>(0), s_allocator_p);
    ASSERT_SAFE_PASS(new (&obj) mqbs::FileSet(static_cast<mqbs::FileStore*>(0),
                                              s_allocator_p));

    ASSERT_EQ(obj.d_store_p, static_cast<mqbs::FileStore*>(0));
    ASSERT_EQ(obj.d_dataFileKey.isNull(), true);
    ASSERT_EQ(obj.d_dataFile.isValid(), false);
    ASSERT_EQ(obj.d_journalFile.isValid(), false);
    ASSERT_EQ(obj.d_qlistFile.isValid(), false);
    ASSERT_EQ(obj.d_dataFilePosition, 0ULL);
    ASSERT_EQ(obj.d_journalFilePosition, 0ULL);
    ASSERT_EQ(obj.d_qlistFilePosition, 0ULL);
    ASSERT_EQ(obj.d_dataFileName.empty(), true);
    ASSERT_EQ(obj.d_journalFileName.empty(), true);
    ASSERT_EQ(obj.d_qlistFileName.empty(), true);
    ASSERT_EQ(obj.d_outstandingBytesJournal, 0ULL);
    ASSERT_EQ(obj.d_outstandingBytesData, 0ULL);
    ASSERT_EQ(obj.d_outstandingBytesQlist, 0ULL);
    ASSERT_EQ(obj.d_journalFileAvailable, true);
    ASSERT_EQ(obj.d_fileSetRolloverPolicyAlarm, false);
    ASSERT_EQ(obj.d_aliasedBlobBufferCount, 1LL);

    ASSERT_EQ(obj.d_allocator_p, s_allocator_p);
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
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
