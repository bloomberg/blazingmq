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

#ifndef INCLUDED_M_BMQSTORAGETOOL_JOURNALFILE_H
#define INCLUDED_M_BMQSTORAGETOOL_JOURNALFILE_H

//@PURPOSE: Provide JournalFile class for unit testing.
//
//@CLASSES:
//  JournalFile: provides methods to create in-memory journal file with
//               required contents for unit testing.
//
//@DESCRIPTION: Helper class to create in-memory journal file.

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>

// BDE
#include <bsl_list.h>
#include <bsl_vector.h>
#include <bslstl_pair.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// =================
// class JournalFile
// =================

class JournalFile {
  public:
    // PUBLIC TYPES
    typedef bsls::AlignedBuffer<mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE>
        RecordBufferType;

    typedef bsl::pair<mqbs::RecordType::Enum, RecordBufferType> NodeType;

    typedef bsl::list<NodeType> RecordsListType;

    typedef bsl::vector<bmqt::MessageGUID> GuidVectorType;

  private:
    // PRIVATE DATA

    /// Number of records.
    size_t d_numRecords;

    /// Mapped file descriptor.
    mqbs::MappedFileDescriptor d_mfd;

    /// Buffer holding the memory allocated for journal file.
    bsl::vector<char> d_buffer;

    /// Current memory block.
    mqbs::MemoryBlock d_block;

    /// Current position.
    bsls::Types::Uint64 d_currPos;

    /// Value of timestamp incrementation.
    const bsls::Types::Uint64 d_timestampIncrement;

    /// File header data.
    mqbs::FileHeader d_fileHeader;

    /// Journal file iterator.
    mqbs::JournalFileIterator d_iterator;

    // PRIVATE MANIPULATORS

    /// Fill journal file header with test data.
    void createFileHeader();

  public:
    // CREATORS

    /// Construct using the specified `numRecords` and `allocator`.
    explicit JournalFile(size_t numRecords, bslma::Allocator* allocator);

    /// Destructor.
    ~JournalFile();

    // ACCESSORS

    /// Return reference to mapped file descriptor.
    const mqbs::MappedFileDescriptor& mappedFileDescriptor() const;

    /// Return reference to file header.
    const mqbs::FileHeader& fileHeader() const;

    /// Return value of timestamp incrementation.
    bsls::Types::Uint64 timestampIncrement() const;

    // MANIPULATORS

    /// Generate sequence of all types of records. Store list of created
    /// records in the specified `records`.
    void addAllTypesRecords(RecordsListType* records);

    /// Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    /// records. MessageRecord and ConfirmRecord records have the same
    /// GUID. DeleteRecord records even records have the same GUID as
    /// MessageRecord, odd ones - not the same. Store list of created
    /// records in the specified `records`. If `expectOutstandingResult` is
    /// `true`, store GUIDs of outstanding messages in the specified
    /// `expectedGUIDs`. Otherwise store GUIDs of confirmed messages.
    void addJournalRecordsWithOutstandingAndConfirmedMessages(
        RecordsListType* records,
        GuidVectorType*  expectedGUIDs,
        bool             expectOutstandingResult);

    /// Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    /// records. MessageRecord and ConfirmRecord records have the same
    /// GUID. DeleteRecord records even records have the same GUID as
    /// MessageRecord, odd ones - not the same. Store list of created
    /// records in the specified `records`. Store GUIDs of partially
    /// confirmed messages in the specified `expectedGUIDs`.
    void addJournalRecordsWithPartiallyConfirmedMessages(
        RecordsListType* records,
        GuidVectorType*  expectedGUIDs);

    /// Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    /// records. MessageRecord ConfirmRecord and DeletionRecord records
    /// have the same GUID. queueKey1 is used for even records, queueKey2
    /// for odd ones. Store list of created records in the specified
    /// `records`. Store GUIDs for `queueKey1` in the specified
    /// `expectedGUIDs` if `captureAllGUIDs` is `false` or all GUIds
    /// otherwise.
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

    /// Generate sequence of multiple types of records. Increase Primary Lease
    /// Id after the specified `numRecordsWithSameLeaseId` records. Store list
    /// of created records in the specified `records`.
    void addMultipleTypesRecordsWithMultipleLeaseId(
        RecordsListType* records,
        size_t           numRecordsWithSameLeaseId);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// =================
// class JournalFile
// =================

inline const mqbs::MappedFileDescriptor&
JournalFile::mappedFileDescriptor() const
{
    return d_mfd;
}

inline const mqbs::FileHeader& JournalFile::fileHeader() const
{
    return d_fileHeader;
}

inline bsls::Types::Uint64 JournalFile::timestampIncrement() const
{
    return d_timestampIncrement;
}

}  // close package namespace

}  // close enterprise namespace

#endif  // M_BMQSTORAGETOOL_JOURNALFILE_H
