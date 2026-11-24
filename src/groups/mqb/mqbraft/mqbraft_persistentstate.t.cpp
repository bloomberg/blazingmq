// Copyright 2026 Bloomberg Finance L.P.
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

// mqbraft_persistentstate.t.cpp -*-C++-*-
#include <mqbraft_persistentstate.h>

// MQB
#include <mqbraft_raftnode.h>

// BMQ
#include <bmqu_memoutstream.h>
#include <bmqu_tempdirectory.h>

// BDE
#include <bdls_filesystemutil.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bsl_string.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

using namespace BloombergLP;
using namespace BloombergLP::mqbraft;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_saveAndLoad()
// SAVE AND LOAD
//
// A missing file loads as term 0 with no vote.  A saved state loads back
// as saved, and a second save replaces the first.
{
    bmqtst::TestHelper::printTestName("SAVE AND LOAD");

    bslma::Allocator*   alloc = bmqtst::TestHelperUtil::allocator();
    bmqu::TempDirectory tempDir(alloc);
    const bsl::string   path = tempDir.path() + "/bmq_raft_0.bmq_raft";
    bmqu::MemOutStream  errorDescription(alloc);

    PersistentState loaded;
    loaded.d_currentTerm = 99;
    BMQTST_ASSERT_EQ(
        0,
        PersistentStateUtil::load(&loaded, errorDescription, path));
    BMQTST_ASSERT_EQ(loaded.d_currentTerm, 0ULL);
    BMQTST_ASSERT_EQ(loaded.d_voteTerm, 0ULL);
    BMQTST_ASSERT_EQ(loaded.d_votedFor, RaftNode::k_INVALID_NODE_ID);

    PersistentState first;
    first.d_currentTerm = 5;
    first.d_voteTerm    = 5;
    first.d_votedFor    = 2;
    BMQTST_ASSERT_EQ(0,
                     PersistentStateUtil::save(errorDescription, path, first));
    BMQTST_ASSERT(!bdls::FilesystemUtil::exists(path + ".tmp"));

    BMQTST_ASSERT_EQ(
        0,
        PersistentStateUtil::load(&loaded, errorDescription, path));
    BMQTST_ASSERT_EQ(loaded.d_currentTerm, 5ULL);
    BMQTST_ASSERT_EQ(loaded.d_voteTerm, 5ULL);
    BMQTST_ASSERT_EQ(loaded.d_votedFor, 2);

    PersistentState second;
    second.d_currentTerm = 8;
    BMQTST_ASSERT_EQ(
        0,
        PersistentStateUtil::save(errorDescription, path, second));

    BMQTST_ASSERT_EQ(
        0,
        PersistentStateUtil::load(&loaded, errorDescription, path));
    BMQTST_ASSERT_EQ(loaded.d_currentTerm, 8ULL);
    BMQTST_ASSERT_EQ(loaded.d_voteTerm, 0ULL);
    BMQTST_ASSERT_EQ(loaded.d_votedFor, RaftNode::k_INVALID_NODE_ID);
}

static void test2_damagedFile()
// DAMAGED FILE
//
// A file that fails its checksum, or is shorter than expected, does not
// load, and leaves the output unchanged.
{
    bmqtst::TestHelper::printTestName("DAMAGED FILE");

    bslma::Allocator*   alloc = bmqtst::TestHelperUtil::allocator();
    bmqu::TempDirectory tempDir(alloc);
    const bsl::string   path = tempDir.path() + "/bmq_raft_0.bmq_raft";
    bmqu::MemOutStream  errorDescription(alloc);

    PersistentState state;
    state.d_currentTerm = 5;
    state.d_voteTerm    = 5;
    state.d_votedFor    = 2;
    BMQTST_ASSERT_EQ(0,
                     PersistentStateUtil::save(errorDescription, path, state));

    // Change one byte of the term.
    {
        bsl::fstream file(path.c_str(),
                          bsl::ios::in | bsl::ios::out | bsl::ios::binary);
        file.seekp(12);
        file.put('\x7f');
    }

    PersistentState loaded;
    loaded.d_currentTerm = 99;
    BMQTST_ASSERT_NE(
        0,
        PersistentStateUtil::load(&loaded, errorDescription, path));
    BMQTST_ASSERT_EQ(loaded.d_currentTerm, 99ULL);

    // Cut the file short.
    {
        bsl::ofstream file(path.c_str(),
                           bsl::ios::out | bsl::ios::trunc | bsl::ios::binary);
        file.write("BMQR", 4);
    }

    BMQTST_ASSERT_NE(
        0,
        PersistentStateUtil::load(&loaded, errorDescription, path));
    BMQTST_ASSERT_EQ(loaded.d_currentTerm, 99ULL);
}

// ============================================================================
//                                 MAIN PROGRAM
// ============================================================================

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_damagedFile(); break;
    case 1: test1_saveAndLoad(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
