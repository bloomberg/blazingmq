// Copyright 2024 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGETOOL_FILEMANAGERMOCK_H
#define INCLUDED_M_BMQSTORAGETOOL_FILEMANAGERMOCK_H

//@PURPOSE: Provide mock implementation of FileManager for unit tests.
//
//@CLASSES:
//  FileManagerMock: provides FileManager implementation with mocked methods.
//
//@DESCRIPTION: FileManager mock for unit testing.

// bmqstoragetool
#include <m_bmqstoragetool_filemanager.h>
#include <m_bmqstoragetool_journalfile.h>

// GMOCK
// If mwcst_testhelper.h was defined before gtest.h, preserve macroses values.
// If not, undefine values from gtest.h.
#pragma push_macro("ASSERT_EQ")
#pragma push_macro("ASSERT_NE")
#pragma push_macro("ASSERT_LT")
#pragma push_macro("ASSERT_LE")
#pragma push_macro("ASSERT_GT")
#pragma push_macro("ASSERT_GE")
#pragma push_macro("TEST_F")
#pragma push_macro("TEST")

#include <gmock/gmock.h>

#undef ASSERT_EQ
#undef ASSERT_NE
#undef ASSERT_LT
#undef ASSERT_LE
#undef ASSERT_GT
#undef ASSERT_GE
#undef TEST_F
#undef TEST
#pragma pop_macro("ASSERT_EQ")
#pragma pop_macro("ASSERT_NE")
#pragma pop_macro("ASSERT_LT")
#pragma pop_macro("ASSERT_LE")
#pragma pop_macro("ASSERT_GT")
#pragma pop_macro("ASSERT_GE")
#pragma pop_macro("TEST_F")
#pragma pop_macro("TEST")

namespace BloombergLP {

namespace m_bmqstoragetool {

// =====================
// class FileManagerMock
// =====================

/// This class provides FileManager implementation with mocked methods.
class FileManagerMock : public FileManager {
  private:
    // PRIVATE DATA

    /// Journal file iterator.
    mqbs::JournalFileIterator d_journalFileIt;

    /// Data file iterator.
    mqbs::DataFileIterator d_dataFileIt;

  public:
    // CREATORS

    /// Default constructor.
    explicit FileManagerMock();

    /// Construct using the specified `journalFile`.
    explicit FileManagerMock(const JournalFile& journalFile);

    // MANIPULATORS

    /// Return pointer to modifiable journal file iterator.
    mqbs::JournalFileIterator* journalFileIterator() BSLS_KEYWORD_OVERRIDE;

    MOCK_METHOD0(dataFileIterator, mqbs::DataFileIterator*());
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// =====================
// class FileManagerMock
// =====================

inline FileManagerMock::FileManagerMock()
: d_journalFileIt()
, d_dataFileIt()
{
    EXPECT_CALL(*this, dataFileIterator())
        .WillRepeatedly(testing::Return(&d_dataFileIt));
}

inline FileManagerMock::FileManagerMock(const JournalFile& journalFile)
: d_journalFileIt(&journalFile.mappedFileDescriptor(),
                  journalFile.fileHeader(),
                  false)
, d_dataFileIt()
{
    EXPECT_CALL(*this, dataFileIterator())
        .WillRepeatedly(testing::Return(&d_dataFileIt));
}

inline mqbs::JournalFileIterator* FileManagerMock::journalFileIterator()
{
    return &d_journalFileIt;
}

}  // close package namespace

}  // close enterprise namespace

#endif  // M_BMQSTORAGETOOL_FILEMANAGERMOCK_H
