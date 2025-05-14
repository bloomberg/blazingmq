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

#include <gmock/gmock.h>

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

    /// CSL file iterator.
    mqbc::IncoreClusterStateLedgerIterator* d_cslFileIt_p;

  public:
    // CREATORS

    /// Default constructor.
    explicit FileManagerMock();

    /// Construct using the specified `journalFile`.
    explicit FileManagerMock(const JournalFile& journalFile);

    /// Construct using the specified `cslFileIterator`.
    explicit FileManagerMock(
        mqbc::IncoreClusterStateLedgerIterator* cslFileIterator);

    // MANIPULATORS

    /// Return pointer to modifiable journal file iterator.
    mqbs::JournalFileIterator* journalFileIterator() BSLS_KEYWORD_OVERRIDE;

    /// Return pointer to modifiable CSL file iterator.
    mqbc::IncoreClusterStateLedgerIterator*
    cslFileIterator() BSLS_KEYWORD_OVERRIDE;

    MOCK_METHOD0(dataFileIterator, mqbs::DataFileIterator*());
    MOCK_CONST_METHOD1(fillQueueMapFromCslFile, void(QueueMap*));
};

}  // close package namespace

}  // close enterprise namespace

#endif  // M_BMQSTORAGETOOL_FILEMANAGERMOCK_H
