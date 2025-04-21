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

// mqbs_qlistfileiterator.h                                           -*-C++-*-
#ifndef INCLUDED_MQBS_QLISTFILEITERATOR
#define INCLUDED_MQBS_QLISTFILEITERATOR

//@PURPOSE: Provide a mechanism to iterate over a BlazingMQ Qlist file.
//
//@CLASSES:
//  mqbs::QlistFileIterator:   Mechanism to iterate over a BlazingMQ Qlist file
//
//@DESCRIPTION: 'mqbs::QlistFileIterator' provides a mechanism to iterate over
// a BlazingMQ queue list file.

// MQB

#include <ball_log.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblockiterator.h>

// MQB
#include <mqbs_offsetptr.h>

// BDE
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// =======================
// class QlistFileIterator
// =======================

/// This component provides a mechanism to iterate over a BlazingMQ queue
/// list file.
class QlistFileIterator {
  private:
    // DATA
    const MappedFileDescriptor* d_mfd_p;

    MemoryBlockIterator d_blockIter;

    bsls::Types::Uint64 d_qlistHeaderOffset;

    bsls::Types::Uint64 d_qlistRecordIndex;

    unsigned int d_advanceLength;

  public:
    // TYPES
    typedef bsl::pair<const char*, unsigned int> AppIdLengthPair;

    // CREATORS

    /// Create an instance in invalid state.
    QlistFileIterator();

    /// Create an iterator instance over the qlist file represented by the
    /// specified `mfd` and having specified `fileHeader`.
    QlistFileIterator(const MappedFileDescriptor* mfd,
                      const FileHeader&           fileHeader);

    // MANIPULATORS

    /// Reset this instance with the file represented by the specified `mfd`
    /// and having specified `fileHeader. Return 0 if `nextRecord()' can be
    /// invoked on this instance, non-zero value otherwise.
    int reset(const MappedFileDescriptor* mfd, const FileHeader& fileHeader);

    /// Reset this instance to invalid state.
    void clear();

    /// Advance to the next message.  Return 1 if the new position is valid
    /// and represents a valid record, 0 if iteration has reached the end of
    /// the file, or < 0 if an error was encountered.  Note that if this
    /// method returns 0, this instance goes in an invalid state, and after
    /// that, only valid operations on this instance are assignment, `reset`
    int nextRecord();

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

    /// Return the offset of the record currently pointed by this iterator.
    /// Behavior is undefined unless last call to `next()` returned 1.
    bsls::Types::Uint64 recordOffset() const;

    /// Return the index of the record in the file currently pointed to by
    /// this iterator.  Behaviour is undefined unless the last call to
    /// `nextRecord` returned 1.  Indices are zero-indexed.
    bsls::Types::Uint64 recordIndex() const;

    /// Return the position of the first valid record in the qlist file.
    /// Return value of zero implies that there are no valid records.
    bsls::Types::Uint64 firstRecordPosition() const;

    /// Return a reference offering non-modifiable access to the
    /// QlistFileHeader of this file.  The behavior is undefined unless
    /// `isValid` returns true.
    const QlistFileHeader& header() const;

    /// Return a pointer to the header of the record currently being pointed
    /// to by this iterator instance.  The behavior is undefined unless last
    /// call to `nextRecord()` succeeded.
    const QueueRecordHeader* queueRecordHeader() const;

    /// Load the specified `data` and `length` buffers with the uri of the
    /// queue.  The behavior is undefined unless last call to `nextRecord()`
    /// succeeded.
    void loadQueueUri(const char** data, unsigned int* length) const;

    /// Load the specified `data` buffer with the hash of the queue uri.
    /// The behavior is undefined unless last call to `nextRecord()`
    /// succeeded.
    void loadQueueUriHash(const char** data) const;

    /// Return the number of appIds present in the queue uri record
    /// currently being pointed by this iterator.  The behavior is undefined
    /// unless last call to `nextRecord()` succeeded.
    unsigned int numAppIds() const;

    void loadAppIds(bsl::vector<AppIdLengthPair>* appIds) const;

    void loadAppIdHashes(bsl::vector<const char*>* appIdHashes) const;

    /// Return the associated mapped file descriptor.
    const MappedFileDescriptor* mappedFileDescriptor() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class QlistFileIterator
// -----------------------

// CREATORS
inline QlistFileIterator::QlistFileIterator()
{
    clear();
}

inline QlistFileIterator::QlistFileIterator(const MappedFileDescriptor* mfd,
                                            const FileHeader& fileHeader)
{
    reset(mfd, fileHeader);
}

// MANIPULATORS
inline void QlistFileIterator::clear()
{
    d_blockIter.clear();
    d_mfd_p             = 0;
    d_qlistHeaderOffset = 0;
    d_qlistRecordIndex  = 0;
    d_advanceLength     = 0;
}

// ACCESSORS
inline bool QlistFileIterator::isValid() const
{
    return 0 != d_advanceLength;
}

inline bool QlistFileIterator::hasRecordSizeRemaining() const
{
    BSLS_ASSERT_SAFE(isValid());
    if (isReverseMode()) {
        // In reverse mode, the advance length covers both the record size and
        // header size.
        return d_blockIter.remaining() >= d_advanceLength;  // RETURN
    }

    return d_blockIter.remaining() >=
           (d_advanceLength + (queueRecordHeader()->headerWords() *
                               bmqp::Protocol::k_WORD_SIZE));
}

inline bool QlistFileIterator::isReverseMode() const
{
    return !d_blockIter.isForwardIterator();
}

inline bsls::Types::Uint64 QlistFileIterator::recordOffset() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_blockIter.position());
    return d_blockIter.position();
}

inline bsls::Types::Uint64 QlistFileIterator::recordIndex() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_blockIter.position());
    return d_qlistRecordIndex - 1;
}

inline bsls::Types::Uint64 QlistFileIterator::firstRecordPosition() const
{
    BSLS_ASSERT_SAFE(isValid());
    if (d_qlistHeaderOffset == 0) {
        return 0;  // RETURN
    }

    const QlistFileHeader& qfh     = header();
    const unsigned int     qfhSize = qfh.headerWords() *
                                 bmqp::Protocol::k_WORD_SIZE;
    if (d_mfd_p->fileSize() <= d_qlistHeaderOffset + qfhSize) {
        return 0;  // RETURN
    }

    OffsetPtr<const QueueRecordHeader> qrh(d_mfd_p->block(),
                                           d_qlistHeaderOffset + qfhSize);
    if (qrh->queueRecordWords() == 0) {
        return 0;  // RETURN
    }
    return d_qlistHeaderOffset + qfhSize;
}

inline const QlistFileHeader& QlistFileIterator::header() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_qlistHeaderOffset);

    BALL_LOG_SET_CATEGORY("yyan82 TODO rm");
    BALL_LOG_ERROR << "d_blockIter.block()->base() = "
                   << d_blockIter.block()->base()
                   << ", d_blockIter.block()->size() = "
                   << d_blockIter.block()->size()
                   << ", d_qlistHeaderOffset = " << d_qlistHeaderOffset;
    OffsetPtr<const QlistFileHeader> rec(*d_blockIter.block(),
                                         d_qlistHeaderOffset);
    return *rec;
}

inline const QueueRecordHeader* QlistFileIterator::queueRecordHeader() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_blockIter.position());

    OffsetPtr<const QueueRecordHeader> rec(*d_blockIter.block(),
                                           d_blockIter.position());
    return rec.get();
}

inline unsigned int QlistFileIterator::numAppIds() const
{
    return queueRecordHeader()->numAppIds();
}

inline const MappedFileDescriptor*
QlistFileIterator::mappedFileDescriptor() const
{
    return d_mfd_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
