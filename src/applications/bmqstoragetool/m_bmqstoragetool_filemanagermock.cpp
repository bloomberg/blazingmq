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

// bmqstoragetool
#include <m_bmqstoragetool_filemanagermock.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

FileManagerMock::FileManagerMock()
: d_journalFileIt()
, d_dataFileIt()
{
    EXPECT_CALL(*this, dataFileIterator())
        .WillRepeatedly(testing::Return(&d_dataFileIt));
}

FileManagerMock::FileManagerMock(const JournalFile& journalFile)
: d_journalFileIt(&journalFile.mappedFileDescriptor(),
                  journalFile.fileHeader(),
                  false)
, d_dataFileIt()
{
    EXPECT_CALL(*this, dataFileIterator())
        .WillRepeatedly(testing::Return(&d_dataFileIt));
}

mqbs::JournalFileIterator* FileManagerMock::journalFileIterator()
{
    return &d_journalFileIt;
}

}  // close package namespace

}  // close enterprise namespace
