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

// mqbs_datafileiterator.h                                            -*-C++-*-
#ifndef INCLUDED_MQBS_DATAFILEITERATOR
#define INCLUDED_MQBS_DATAFILEITERATOR

//@PURPOSE: Provide a mechanism to iterate over a BlazingMQ data file.
//
//@CLASSES:
//  mqbs::DataFileIterator: Mechanism to iterate over a BlazingMQ data file.
//
//@DESCRIPTION: 'mqbs::DataFileIterator' is a mechanism to iterate over a
// BlazingMQ data file.

// MQB

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

// ======================
// class DataFileIterator
// ======================

/// This component provides a mechanism to iterate over a BlazingMQ data
/// file.
class DataFileIterator {
  private:
    // DATA
    const MappedFileDescriptor* d_mfd_p;

    MemoryBlockIterator d_blockIter;

    bsls::Types::Uint64 d_dataFileHeaderOffset;

    bsls::Types::Uint64 d_dataRecordIndex;

    unsigned int d_advanceLength;

  public:
    // CREATORS

    /// Create an instance in invalid state.
    DataFileIterator();

    /// Create an iterator instance over the data file represented by the
    /// specified `mfd` and having specified `fileHeader`.
    DataFileIterator(const MappedFileDescriptor* mfd,
                     const FileHeader&           fileHeader);

    // MANIPULATORS

    /// Reset this instance with the file represented by the specified `mfd`
    /// and having specified `fileHeader.  Return 0 if `nextRecord()' can be
    /// invoked on this instance, non-zero value otherwise.
    int reset(const MappedFileDescriptor* mfd, const FileHeader& fileHeader);

    /// Reset this instance to invalid state.
    void clear();

    /// Advance to the next message.  Return 1 if the new position is valid
    /// and represents a valid record, 0 if iteration has reached the end of
    /// the file, or < 0 if an error was encountered.  Note that if this
    /// method returns 0, this instance goes in an invalid state, and after
    /// that, only valid operations on this instance are assignment, `reset`
    /// and `isValid`.
    int nextRecord();

    /// Changes the direction of the iterator.  Unlike calling reset,
    /// calling this function maintains the current file position within the
    /// journal.  Returns 0 if it was successful, otherwise < 0.  If the
    /// iterator was a forward iterator, it becomes a reverse iterator,
    /// similarly if the iterator was a reverse iterator, it becomes a
    /// forward iterator.  The behaviour is undefined unless `isValid`
    /// returns true.
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

    /// Return a const reference to the DataFileHeader of this file.
    /// Behavior is undefined unless `isValid` returns true.
    const DataFileHeader& header() const;

    /// Return the offset of the record currently pointed by this iterator.
    /// Behavior is undefined unless last call to `next()` returned 1.
    bsls::Types::Uint64 recordOffset() const;

    /// Return the index of the record in the file currently pointed to by
    /// this iterator.  Behaviour is undefined unless the last call to
    /// `nextRecord` returned 1.  Indices are zero-indexed.
    bsls::Types::Uint64 recordIndex() const;

    /// Return the position of the first valid record in the qList file.
    /// Return value of zero implies that there are no records in the file.
    bsls::Types::Uint64 firstRecordPosition() const;

    /// Return a reference offering non-modifiable access to the
    /// `DataHeader` currently pointed to by this iterator.  The behavior is
    /// undefined unless last call to `nextRecord()` returned 1.
    const DataHeader& dataHeader() const;

    /// Load the application data and its length in the specified `data` and
    /// `length` buffers respectively.  The behavior is undefined unless
    /// `nextRecord` returned true.  Note that application data contains
    /// optional message properties and message payload.
    void loadApplicationData(const char** data, unsigned int* length) const;

    /// Load the options and their length in the specified `options` and
    /// `length` buffers respectively.  The behavior is undefined unless
    /// `nextRecord` returned true.  Note that if no options are associated
    /// with the message, `options` and `length` will be zero.
    void loadOptions(const char** options, unsigned int* length) const;

    /// Return the associated mapped file descriptor.
    const MappedFileDescriptor* mappedFileDescriptor() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------
// class DataFileIterator
// ----------------------

// CREATORS
inline DataFileIterator::DataFileIterator()
{
    clear();
}

inline DataFileIterator::DataFileIterator(const MappedFileDescriptor* mfd,
                                          const FileHeader& fileHeader)
{
    reset(mfd, fileHeader);
}

// MANIPULATORS
inline void DataFileIterator::clear()
{
    d_blockIter.clear();
    d_mfd_p                = 0;
    d_dataFileHeaderOffset = 0;
    d_dataRecordIndex      = 0;
    d_advanceLength        = 0;
}

// ACCESSORS
inline bool DataFileIterator::isValid() const
{
    return 0 != d_advanceLength;
}

inline bool DataFileIterator::hasRecordSizeRemaining() const
{
    BSLS_ASSERT_SAFE(isValid());
    if (isReverseMode()) {
        // In reverse mode, the advance length covers both the record size and
        // header size.
        return d_blockIter.remaining() >= d_advanceLength;  // RETURN
    }
    return d_blockIter.remaining() >= (d_advanceLength + sizeof(DataHeader));
}

inline bool DataFileIterator::isReverseMode() const
{
    return !d_blockIter.isForwardIterator();
}

inline const DataFileHeader& DataFileIterator::header() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_dataFileHeaderOffset);

    OffsetPtr<const DataFileHeader> rec(*d_blockIter.block(),
                                        d_dataFileHeaderOffset);
    return *rec;
}

inline bsls::Types::Uint64 DataFileIterator::recordOffset() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_blockIter.position());
    return d_blockIter.position();
}

inline bsls::Types::Uint64 DataFileIterator::recordIndex() const
{
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(0 != d_blockIter.position());
    return d_dataRecordIndex - 1;
}

inline bsls::Types::Uint64 DataFileIterator::firstRecordPosition() const
{
    BSLS_ASSERT_SAFE(isValid());
    if (d_dataFileHeaderOffset == 0) {
        return 0;  // RETURN
    }

    const DataFileHeader& dfh     = header();
    const unsigned int    dfhSize = dfh.headerWords() *
                                 bmqp::Protocol::k_WORD_SIZE;
    if (d_mfd_p->fileSize() <= d_dataFileHeaderOffset + dfhSize) {
        return 0;  // RETURN
    }

    OffsetPtr<const DataHeader> dataHeader(d_mfd_p->block(),
                                           d_dataFileHeaderOffset + dfhSize);

    if ((dataHeader->messageWords() - dataHeader->headerWords()) <= 0) {
        return 0;  // RETURN
    }

    return d_dataFileHeaderOffset + dfhSize;
}

inline const DataHeader& DataFileIterator::dataHeader() const
{
    OffsetPtr<const DataHeader> rec(*d_blockIter.block(), recordOffset());
    return *rec;
}

inline const MappedFileDescriptor*
DataFileIterator::mappedFileDescriptor() const
{
    return d_mfd_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
