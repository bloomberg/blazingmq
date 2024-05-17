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

//@PURPOSE: Provide helper classes and mocks for unit testing.
//
//@CLASSES:
//  m_bmqstoragetool::TestUtils::JournalFile: provides methods to create
//  in-memory journal file with required content.
//  m_bmqstoragetool::TestUtils::FileManagerMock: provides FileManager
//  implementation with mocked methods.
//
//@DESCRIPTION: Helper class to create in-memory journal file and FileManager
// mock for unit testing.

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
// Undefine macroses from gtest.h which are defined in mwctst_testhelper.h
#undef ASSERT_EQ
#undef ASSERT_NE
#undef ASSERT_LT
#undef ASSERT_LE
#undef ASSERT_GT
#undef ASSERT_GE
#undef TEST_F
#undef TEST

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

typedef bsl::vector<bmqt::MessageGUID> GuidVectorType;

// =================
// class JournalFile
// =================

class JournalFile {
  private:
    // PRIVATE DATA

    size_t d_numRecords;
    // Number of records.
    MappedFileDescriptor d_mfd;
    // Mapped file descriptor.
    bsl::vector<char> d_buffer;
    // Buffer holding the memory allocated for journal file.
    MemoryBlock d_block;
    // Current memory block.
    bsls::Types::Uint64 d_currPos;
    // Current position.
    const bsls::Types::Uint64 d_timestampIncrement;
    // Value of timestamp incrementation.
    FileHeader d_fileHeader;
    // File header data.
    JournalFileIterator d_iterator;
    // Journal file iterator.

    // PRIVATE MANIPULATORS

    void createFileHeader();
    // Fill journal file header with test data.

  public:
    // CREATORS

    /// Constructor using the specified `numRecords` and `allocator`.
    explicit JournalFile(size_t numRecords, bslma::Allocator* allocator);

    /// Destructor.
    ~JournalFile();

    // ACCESSORS

    /// Return reference to mapped file descriptor.
    const MappedFileDescriptor& mappedFileDescriptor() const;

    /// Return reference to file header.
    const FileHeader& fileHeader() const;

    /// Return value of timestamp incrementation.
    bsls::Types::Uint64 timestampIncrement() const;

    // MANIPULATORS

    /// Generate sequence of all types records. Store list of created records
    /// in the specified `records`.
    void addAllTypesRecords(RecordsListType* records);

    /// Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    /// records. MessageRecord and ConfirmRecord records have the same GUID.
    /// DeleteRecord records even records have the same GUID as MessageRecord,
    /// odd ones - not the same. Store list of created records in the specified
    /// `records`. If `expectOutstandingResult` is `true`, store GUIDs of
    /// outstanding messages in the specified `expectedGUIDs`. Otherwise store
    /// GUIDs of confirmed messages.
    void addJournalRecordsWithOutstandingAndConfirmedMessages(
        RecordsListType* records,
        GuidVectorType*  expectedGUIDs,
        bool             expectOutstandingResult);

    /// Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    /// records. MessageRecord and ConfirmRecord records have the same GUID.
    /// DeleteRecord records even records have the same GUID as MessageRecord,
    /// odd ones - not the same. Store list of created records in the specified
    /// `records`. Store GUIDs of partially confirmed messages in the specified
    /// `expectedGUIDs`.
    void addJournalRecordsWithPartiallyConfirmedMessages(
        RecordsListType* records,
        GuidVectorType*  expectedGUIDs);

    /// Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    /// records. MessageRecord ConfirmRecord and DeletionRecord records have
    /// the same GUID. queueKey1 is used for even records, queueKey2 for odd
    /// ones. Store list of created records in the specified `records`. Store
    /// GUIDs for `queueKey1` in the specified `expectedGUIDs` if
    /// `captureAllGUIDs` is `false` or all GUIds otherwise.
    void addJournalRecordsWithTwoQueueKeys(RecordsListType* records,
                                           GuidVectorType*  expectedGUIDs,
                                           const char*      queueKey1,
                                           const char*      queueKey2,
                                           bool captureAllGUIDs = false);

    /// Generate sequence of MessageRecord and DeleteRecord
    /// records with size `numMessages`. Change GUIDs order for 2nd and 3rd
    /// DeleteRecord. Set message offsets from the given `messageOffsets`.
    /// Store list of created records in the specified `records`. Store
    /// confirmed GUIDs in the specified `expectedGUIDs`.
    void addJournalRecordsWithConfirmedMessagesWithDifferentOrder(
        RecordsListType*           records,
        GuidVectorType*            expectedGUIDs,
        size_t                     numMessages,
        bsl::vector<unsigned int>& messageOffsets);
};

/// Output the specified `messageGUID` as a string to the specified `ostream`.
void outputGuidString(bsl::ostream&            ostream,
                      const bmqt::MessageGUID& messageGUID,
                      const bool               addNewLine = true);

/// Value semantic type representing data message parameters.
struct DataMessage {
    int         d_line;
    const char* d_appData_p;
    const char* d_options_p;
};

/// Allocate in memory storage data file and generate sequence of data records
/// using the specified arguments. Return pointer to allocated memory.
char* addDataRecords(bslma::Allocator*          ta,
                     MappedFileDescriptor*      mfd,
                     FileHeader*                fileHeader,
                     const DataMessage*         messages,
                     const unsigned int         numMessages,
                     bsl::vector<unsigned int>& messageOffsets);

// =====================
// class FileManagerMock
// =====================

/// This class provides FileManager implementation with mocked methods.
class FileManagerMock : public FileManager {
  private:
    // PRIVATE DATA

    mqbs::JournalFileIterator d_journalFileIt;
    // Journal file iterator.
    mqbs::DataFileIterator d_dataFileIt;
    // Data file iterator.

  public:
    // CREATORS

    /// Default constructor.
    explicit FileManagerMock()
    {
        EXPECT_CALL(*this, dataFileIterator())
            .WillRepeatedly(Return(&d_dataFileIt));
    }

    /// Constructor using the specified `journalFile` and `allocator`.
    explicit FileManagerMock(const JournalFile& journalFile)
    : d_journalFileIt(&journalFile.mappedFileDescriptor(),
                      journalFile.fileHeader(),
                      false)
    {
        EXPECT_CALL(*this, dataFileIterator())
            .WillRepeatedly(Return(&d_dataFileIt));
    }

    // MANIPULATORS

    /// Return pointer to modifiable journal file iterator.
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

inline bsls::Types::Uint64 JournalFile::timestampIncrement() const
{
    return d_timestampIncrement;
}

}  // close TestUtils namespace

}  // close package namespace

}  // close enterprise namespace

#endif  // M_BMQSTORAGETOOL_TESTUTILS_H
