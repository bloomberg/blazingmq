// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbs_journalfileiterator.h                                         -*-C++-*-
#ifndef INCLUDED_MQBS_JOURNALFILEITERATOR
#define INCLUDED_MQBS_JOURNALFILEITERATOR

//@PURPOSE: Provide a mechanism to iterate over a BlazingMQ journal file.
//
//@CLASSES:
//
//  mqbs::JournalFileIterator: Mechanism to iterate over a BlazingMQ journal
//  file.
//
//@DESCRIPTION: This component provides a mechanism to iterate over a BlazingMQ
// journal file.

// MQB

#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblockiterator.h>
#include <mqbs_offsetptr.h>

// BDE
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// =========================
// class JournalFileIterator
// =========================

/// This component provides a mechanism to iterate over a BlazingMQ journal
/// file.
class JournalFileIterator {
  private:
    // DATA
    const MappedFileDescriptor* d_mfd_p;

    bool d_isReverseMode;
    // This flag needs to be maintained
    // instead of relying on
    // d_blockIter.isForwardIterator()
    // because d_blockIter could be in
    // clear()'d state even though this
    // instance is configured in reverse
    // mode.

    MemoryBlockIterator d_blockIter;

    bsls::Types::Uint64 d_journalHeaderOffset;

    bsls::Types::Uint64 d_lastRecordOffset;

    bsls::Types::Uint64 d_lastSyncPointOffset;

    bsls::Types::Uint64 d_journalRecordIndex;

    unsigned int d_advanceLength;

    unsigned int d_recordSize;

  public:
    // CREATORS

    /// Create an instance in invalid state.
    JournalFileIterator();

    /// Create an iterator instance over the journal file represented by the
    /// specified `mfd` and having specified `fileHeader`, with the
    /// optionally specified `reverseMode` flag indicating the direction of
    /// iteration.
    JournalFileIterator(const MappedFileDescriptor* mfd,
                        const FileHeader&           fileHeader,
                        bool                        reverseMode = false);

    // MANIPULATORS

    /// Reset this instance with the file represented by the specified `mfd`
    /// and having specified `fileHeader.  Return 0 if `nextRecord()' can be
    /// invoked on this instance, non-zero value otherwise.
    int reset(const MappedFileDescriptor* mfd,
              const FileHeader&           fileHeader,
              bool                        isReverse = false);

    /// Reset this instance to invalid state.
    void clear();

    /// Advance to the next message.  Return 1 if the new position is valid
    /// and represents a valid record, 0 if iteration has reached the end of
    /// the file, or < 0 if an error was encountered.  Note that if this
    /// method returns 0, this instance goes in an invalid state, and after
    /// that, only valid operations on this instance are assignment, `reset`
    /// and `isValid`.
    int nextRecord();

    /// Advance to the message with the specified distance to the actual
    /// direction.  Return 1 if the new position is valid and represents a
    /// valid record, 0 if iteration has reached the end of the file, or < 0 if
    /// an error was encountered.  Note that if this method returns 0, this
    /// instance goes in an invalid state, and after that, only valid
    /// operations on this instance are assignment, `reset` and `isValid`.
    int advance(const bsls::Types::Uint64 distance);

    /// Changes the direction of the iterator.  Unlike calling reset,
    /// calling this function maintains the current file position within the
    /// journal.  Returns 0 if it was successful, otherwise < 0.  If the
    /// iterator was a forward iterator, it becomes a reverse iterator,
    /// similarly if the iterator was a reverse iterator, it becomes a
    /// forward iterator.  Behaviour is undefined unless `isValid` returns
    /// true.
    void flipDirection();

    // ACCESSORS

    /// Return true if this iterator is initialized and valid, and `next()`
    /// can be called on this instance, or return false in all other cases.
    bool isValid() const;

    /// Return true if there is enough available space left for the iterator
    /// to navigate to another next record.  If this function returns false,
    /// then either we are either at the beginning or end of the file.
    /// Behaviour is undefined unless `isValid` returns true.
    bool hasRecordSizeRemaining() const;

    /// Return true if this instance has been configured to iterate in
    /// reverse direction, false otherwise.  The behavior is undefined
    /// unless `isValid()` returns true.
    bool isReverseMode() const;

    /// Return a const reference to the JournalFileHeader of this file.
    /// Behavior is undefined unless `isValid` returns true.
    const JournalFileHeader& header() const;

    /// Return the header of the record currently pointed to by this
    /// iterator.  Behavior is undefined unless last call to `next()`
    /// returned 1.
    const RecordHeader& recordHeader() const;

    /// Return the type of the record currently pointed to by this iterator.
    /// Behavior is undefined unless last call to `next()` returned 1.
    RecordType::Enum recordType() const;

    /// Return the offset of the record currently pointed by this iterator.
    /// Behavior is undefined unless last call to `nextRecord` returned 1.
    bsls::Types::Uint64 recordOffset() const;

    /// Return the index of the record in the file currently pointed to by
    /// this iterator.  Behaviour is undefined unless the last call to
    /// `nextRecord` returned 1.  Indices are zero-indexed.
    bsls::Types::Uint64 recordIndex() const;

    const MessageRecord&  asMessageRecord() const;
    const ConfirmRecord&  asConfirmRecord() const;
    const DeletionRecord& asDeletionRecord() const;
    const QueueOpRecord&  asQueueOpRecord() const;

    /// Return a reference offering non-modifiable access to the record
    /// currently pointed to by this iterator.  The behavior is undefined
    /// unless last call to `next()` returned 1 and `recordType(`) returned
    /// the same record type.
    const JournalOpRecord& asJournalOpRecord() const;

    /// Return the position of the last valid sync point record in the
    /// journal.  Return value of zero implies no valid journal sync point
    /// is present.  Note that return value of zero does not imply that
    /// journal is corrupt.
    bsls::Types::Uint64 lastSyncPointPosition() const;

    /// Return a reference offering non-modifiable access to the last valid
    /// record representing the journal sync point.  The behavior is
    /// undefined unless `lastSyncPointPosition` returns a non-zero value.
    const JournalOpRecord& lastSyncPoint() const;

    /// Return the position of last valid record in the journal.  Return
    /// value of zero implies that there are no valid records in the
    /// journal.
    bsls::Types::Uint64 lastRecordPosition() const;

    /// Return a reference offering non-modifiable access to the header of
    /// the last record in the journal.  Behavior is undefined unless
    /// `isValid` returns true, and `lastRecordPosition` returns a non-zero
    /// value.
    const RecordHeader& lastRecordHeader() const;

    /// Return the position of the first valid record in the journal.
    /// Return value of zero implies that there are no valid records in the
    /// journal.
    bsls::Types::Uint64 firstRecordPosition() const;

    /// Return the mapped file descriptor associated with this instance.
    const MappedFileDescriptor* mappedFileDescriptor() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class JournalFileIterator
// -------------------------

// CREATORS
inline JournalFileIterator::JournalFileIterator()
{
    clear();
}

inline JournalFileIterator::JournalFileIterator(
    const MappedFileDescriptor* mfd,
    const FileHeader&           fileHeader,
    bool                        reverseMode)
{
    reset(mfd, fileHeader, reverseMode);
}

// MANIPULATORS
inline void JournalFileIterator::clear()
{
    d_blockIter.clear();
    d_mfd_p               = 0;
    d_isReverseMode       = false;
    d_journalHeaderOffset = 0;
    d_lastRecordOffset    = 0;
    d_lastSyncPointOffset = 0;
    d_journalRecordIndex  = 0;
    d_advanceLength       = 0;
    d_recordSize          = 0;
}

// ACCESSORS
inline bool JournalFileIterator::isValid() const
{
    return 0 != d_advanceLength;
}

inline bool JournalFileIterator::hasRecordSizeRemaining() const
{
    BSLS_ASSERT_SAFE(isValid());
    if (d_blockIter.isForwardIterator()) {
        return d_blockIter.remaining() >= 2 * d_recordSize;  // RETURN
    }
    return d_blockIter.remaining() >= d_recordSize;
}

inline bool JournalFileIterator::isReverseMode() const
{
    return d_isReverseMode;
}

inline const JournalFileHeader& JournalFileIterator::header() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_journalHeaderOffset);

    // Note that below, we must use 'd_mfd_p' instead of 'd_blockIter' because
    // 'd_blockIter' might be in clear()'d state if this journal is empty
    // (i.e., if it has no records, just FileHeader and JournalFileHeader).

    OffsetPtr<const JournalFileHeader> rec(d_mfd_p->block(),
                                           d_journalHeaderOffset);
    return *rec;
}

inline const RecordHeader& JournalFileIterator::recordHeader() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_blockIter.position());

    OffsetPtr<const RecordHeader> recHeader(*d_blockIter.block(),
                                            d_blockIter.position());
    return *recHeader;
}

inline RecordType::Enum JournalFileIterator::recordType() const
{
    return recordHeader().type();
}

inline bsls::Types::Uint64 JournalFileIterator::recordOffset() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_blockIter.position());
    return d_blockIter.position();
}

inline bsls::Types::Uint64 JournalFileIterator::recordIndex() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_blockIter.position());
    return d_journalRecordIndex - 1;
}

inline const MessageRecord& JournalFileIterator::asMessageRecord() const
{
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == recordType());

    OffsetPtr<const MessageRecord> rec(*d_blockIter.block(),
                                       d_blockIter.position());
    return *rec;
}

inline const ConfirmRecord& JournalFileIterator::asConfirmRecord() const
{
    BSLS_ASSERT_SAFE(RecordType::e_CONFIRM == recordType());

    OffsetPtr<const ConfirmRecord> rec(*d_blockIter.block(),
                                       d_blockIter.position());
    return *rec;
}

inline const DeletionRecord& JournalFileIterator::asDeletionRecord() const
{
    BSLS_ASSERT_SAFE(RecordType::e_DELETION == recordType());

    OffsetPtr<const DeletionRecord> rec(*d_blockIter.block(),
                                        d_blockIter.position());
    return *rec;
}

inline const QueueOpRecord& JournalFileIterator::asQueueOpRecord() const
{
    BSLS_ASSERT_SAFE(RecordType::e_QUEUE_OP == recordType());

    OffsetPtr<const QueueOpRecord> rec(*d_blockIter.block(),
                                       d_blockIter.position());
    return *rec;
}

inline const JournalOpRecord& JournalFileIterator::asJournalOpRecord() const
{
    BSLS_ASSERT_SAFE(RecordType::e_JOURNAL_OP == recordType());

    OffsetPtr<const JournalOpRecord> rec(*d_blockIter.block(),
                                         d_blockIter.position());
    return *rec;
}

inline bsls::Types::Uint64 JournalFileIterator::lastSyncPointPosition() const
{
    BSLS_ASSERT_SAFE(isValid());
    return d_lastSyncPointOffset;
}

inline const JournalOpRecord& JournalFileIterator::lastSyncPoint() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_lastSyncPointOffset);

    // Note that below, we must use 'd_mfd_p' instead of 'd_blockIter' because
    // 'd_blockIter' might be in clear()'d state if this journal is empty
    // (i.e., if it has no records, just FileHeader and JournalFileHeader).

    OffsetPtr<const JournalOpRecord> rec(d_mfd_p->block(),
                                         d_lastSyncPointOffset);
    return *rec;
}

inline bsls::Types::Uint64 JournalFileIterator::lastRecordPosition() const
{
    BSLS_ASSERT_SAFE(isValid());
    return d_lastRecordOffset;
}

inline const RecordHeader& JournalFileIterator::lastRecordHeader() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_lastRecordOffset);

    OffsetPtr<const RecordHeader> recHeader(d_mfd_p->block(),
                                            d_lastRecordOffset);
    return *recHeader;
}

inline bsls::Types::Uint64 JournalFileIterator::firstRecordPosition() const
{
    BSLS_ASSERT_SAFE(isValid());
    if (d_journalHeaderOffset == 0 || d_lastRecordOffset == 0) {
        return 0;  // RETURN
    }

    const JournalFileHeader& jh                = header();
    const unsigned int       journalHeaderSize = jh.headerWords() *
                                           bmqp::Protocol::k_WORD_SIZE;
    return journalHeaderSize + d_journalHeaderOffset;
}

inline const MappedFileDescriptor*
JournalFileIterator::mappedFileDescriptor() const
{
    return d_mfd_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
