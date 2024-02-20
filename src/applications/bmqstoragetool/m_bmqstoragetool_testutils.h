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

#ifndef INCLUDED_M_BMQSTORAGETOOL_TESTUTILS_H
#define INCLUDED_M_BMQSTORAGETOOL_TESTUTILS_H

// bmqstoragetool
#include <m_bmqstoragetool_filemanager.h>
#include <m_bmqstoragetool_parameters.h>

// BMQ
#include <bmqt_messageguid.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_messageguidutil.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_list.h>
#include <bsl_utility.h>
#include <bslma_default.h>
#include <bsls_alignedbuffer.h>

// GMOCK
#include <gmock/gmock.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
using namespace bsl;
using namespace mqbs;
using namespace ::testing;

namespace BloombergLP {

namespace m_bmqstoragetool {

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace TestUtils {

typedef bsls::AlignedBuffer<FileStoreProtocol::k_JOURNAL_RECORD_SIZE>
    RecordBufferType;

typedef bsl::pair<RecordType::Enum, RecordBufferType> NodeType;

typedef bsl::list<NodeType> RecordsListType;

class JournalFile {
  private:
    size_t                    d_numRecords;
    MappedFileDescriptor      d_mfd;
    char*                     d_mem_p;
    MemoryBlock               d_block;
    bsls::Types::Uint64       d_currPos;
    const bsls::Types::Uint64 d_timestampIncrement;
    FileHeader                d_fileHeader;
    JournalFileIterator       d_iterator;
    bslma::Allocator*         d_allocator_p;

    // MANIPULATORS

    void createFileHeader();

  public:
    // CREATORS
    JournalFile(const size_t numRecords, bslma::Allocator* allocator);
    ;

    ~JournalFile();
    ;

    // ACCESSORS

    const MappedFileDescriptor& mappedFileDescriptor() const;

    const FileHeader& fileHeader() const;

    const bsls::Types::Uint64 timestampIncrement();

    // MANIPULATORS

    void addAllTypesRecords(RecordsListType* records);

    bsl::vector<bmqt::MessageGUID>
    addJournalRecordsWithOutstandingAndConfirmedMessages(
        RecordsListType* records,
        bool             expectOutstandingResult);

    // Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    // records. MessageRecord and ConfirmRecord records have the same GUID.
    // DeleteRecord records even records have the same GUID as MessageRecord,
    // odd ones - not the same.
    bsl::vector<bmqt::MessageGUID>
    addJournalRecordsWithPartiallyConfirmedMessages(RecordsListType* records);

    // Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    // records. MessageRecord ConfirmRecord and DeletionRecord records have the
    // same GUID. queueKey1 is used for even records, queueKey2 for odd ones.
    // Returns GUIDs for queueKey1 if 'captureAllGUIDs' is false or all GUIds
    // otherwise.
    bsl::vector<bmqt::MessageGUID>
    addJournalRecordsWithTwoQueueKeys(RecordsListType* records,
                                      const char*      queueKey1,
                                      const char*      queueKey2,
                                      bool captureAllGUIDs = false);

    bsl::vector<bmqt::MessageGUID>
    addJournalRecordsWithConfirmedMessagesWithDifferentOrder(
        RecordsListType*           records,
        size_t                     numMessages,
        bsl::vector<unsigned int>& messageOffsets);
};

void outputGuidString(bsl::ostream&            ostream,
                      const bmqt::MessageGUID& messageGUID,
                      const bool               addNewLine = true);

struct DataMessage {
    int         d_line;
    const char* d_appData_p;
    const char* d_options_p;
};

char* addDataRecords(bslma::Allocator*          ta,
                     MappedFileDescriptor*      mfd,
                     FileHeader*                fileHeader,
                     const DataMessage*         messages,
                     const unsigned int         numMessages,
                     bsl::vector<unsigned int>& messageOffsets);

class FileManagerMock : public FileManager {
    mqbs::JournalFileIterator d_journalFileIt;

  public:
    // CREATORS
    explicit FileManagerMock(bslma::Allocator* allocator = 0)
    {
        EXPECT_CALL(*this, dataFileIterator()).WillRepeatedly(Return(nullptr));
    }
    explicit FileManagerMock(const JournalFile& journalFile,
                             bslma::Allocator*  allocator = 0)
    : d_journalFileIt(&journalFile.mappedFileDescriptor(),
                      journalFile.fileHeader(),
                      false)
    {
        // Prepare parameters
        EXPECT_CALL(*this, dataFileIterator()).WillRepeatedly(Return(nullptr));
    }

    // MANIPULATORS
    mqbs::JournalFileIterator* journalFileIterator() BSLS_KEYWORD_OVERRIDE
    {
        return &d_journalFileIt;
    };

    MOCK_METHOD0(dataFileIterator, mqbs::DataFileIterator*());
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline const MappedFileDescriptor& JournalFile::mappedFileDescriptor() const
{
    return d_mfd;
}

inline const FileHeader& JournalFile::fileHeader() const
{
    return d_fileHeader;
}

inline const bsls::Types::Uint64 JournalFile::timestampIncrement()
{
    return d_timestampIncrement;
}

}  // close TestUtils namespace

}  // close package namespace

}  // close enterprise namespace

#endif  // M_BMQSTORAGETOOL_TESTUTILS_H
