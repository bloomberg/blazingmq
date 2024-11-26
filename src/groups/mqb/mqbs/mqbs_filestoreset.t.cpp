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

// mqbs_filestoreset.t.cpp                                            -*-C++-*-
#include <mqbs_filestoreset.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_string.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

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
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    const bsl::string        k_DATA_FILE         = "data";
    const bsls::Types::Int64 k_DATA_FILE_SIZE    = 1 * 1024 * 1024;
    const bsl::string        k_JOURNAL_FILE      = "journal";
    const bsls::Types::Int64 k_JOURNAL_FILE_SIZE = 2048;
    const bsl::string        k_QLIST_FILE        = "qlist";
    const bsls::Types::Int64 k_QLIST_FILE_SIZE   = 1024;

    // Default constructor
    PV("Default constructor");
    mqbs::FileStoreSet obj1(bmqtst::TestHelperUtil::allocator());

    ASSERT_EQ(obj1.dataFile(), "");
    ASSERT_EQ(obj1.journalFile(), "");
    ASSERT_EQ(obj1.qlistFile(), "");

    // Set some values
    obj1.setDataFile(k_DATA_FILE)
        .setDataFileSize(k_DATA_FILE_SIZE)
        .setJournalFile(k_JOURNAL_FILE)
        .setJournalFileSize(k_JOURNAL_FILE_SIZE)
        .setQlistFile(k_QLIST_FILE)
        .setQlistFileSize(k_QLIST_FILE_SIZE);

    ASSERT_EQ(obj1.dataFile(), k_DATA_FILE);
    ASSERT_EQ(obj1.dataFileSize(), k_DATA_FILE_SIZE);
    ASSERT_EQ(obj1.journalFile(), k_JOURNAL_FILE);
    ASSERT_EQ(obj1.journalFileSize(), k_JOURNAL_FILE_SIZE);
    ASSERT_EQ(obj1.qlistFile(), k_QLIST_FILE);
    ASSERT_EQ(obj1.qlistFileSize(), k_QLIST_FILE_SIZE);

    // Copy constructor
    PV("Copy constructor");
    mqbs::FileStoreSet obj2(obj1, bmqtst::TestHelperUtil::allocator());

    ASSERT_EQ(obj1.dataFile(), obj2.dataFile());
    ASSERT_EQ(obj1.dataFileSize(), obj2.dataFileSize());
    ASSERT_EQ(obj1.journalFile(), obj2.journalFile());
    ASSERT_EQ(obj1.journalFileSize(), obj2.journalFileSize());
    ASSERT_EQ(obj1.qlistFile(), obj2.qlistFile());
    ASSERT_EQ(obj1.qlistFileSize(), obj2.qlistFileSize());

    // Reset 'obj1'
    PV("reset");
    obj1.reset();

    ASSERT_EQ(obj1.dataFile(), "");
    ASSERT_EQ(obj1.dataFileSize(), 0LL)
    ASSERT_EQ(obj1.journalFile(), "");
    ASSERT_EQ(obj1.journalFileSize(), 0LL)
    ASSERT_EQ(obj1.qlistFile(), "");
    ASSERT_EQ(obj1.qlistFileSize(), 0LL)

    ASSERT_EQ(obj2.dataFile(), k_DATA_FILE);
    ASSERT_EQ(obj2.dataFileSize(), k_DATA_FILE_SIZE);
    ASSERT_EQ(obj2.journalFile(), k_JOURNAL_FILE);
    ASSERT_EQ(obj2.journalFileSize(), k_JOURNAL_FILE_SIZE);
    ASSERT_EQ(obj2.qlistFile(), k_QLIST_FILE);
    ASSERT_EQ(obj2.qlistFileSize(), k_QLIST_FILE_SIZE);

    // Print
    PV("Print");

    const char* expected = "[ dataFile = \"data\""
                           " dataFileSize = 1,048,576"
                           " qlistFile = \"qlist\""
                           " qlistFileSize = 1,024"
                           " journalFile = \"journal\""
                           " journalFileSize = 2,048 ]";
    {
        PVV("Print (print function)");
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        obj2.print(out, 0, -1);
        ASSERT_EQ(out.str(), expected);
    }

    {
        PVV("Print (stream operator)");
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        out << obj2;
        ASSERT_EQ(out.str(), expected);
    }

    {
        PVV("Print (bad stream)");
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        out.setstate(bsl::ios_base::badbit);
        obj2.print(out, 0, -1);
        ASSERT_EQ(out.str(), "");
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
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
