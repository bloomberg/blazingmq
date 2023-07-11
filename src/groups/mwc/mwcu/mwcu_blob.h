// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwcu_blob.h                                                    -*-C++-*-
#ifndef INCLUDED_MWCU_BLOB
#define INCLUDED_MWCU_BLOB

//@PURPOSE: Provide utility functions for working with Blobs.
//
//@CLASSES:
//  mwcu::BlobPosition       : POD defining a position in a blob.
//  mwcu::BlobSection        : POD defining a range of bytes in a blob.
//  mwcu::BlobUtil           : Static blob manipulation functions.
//  mwcu::BlobStartHexDumper : Pretty printer for blobs.
//
//@DESCRIPTION: This component provides a utility, 'mwcu::BlobUtil', which
// implements a set of static utility functions allowing to copy data from
// 'bdlbb::Blob' objects to raw memory buffers, or copy data from one blob
// object to another blob object causing the data actually to be copied
// instead of duplicating buffer references.

// MWC

// BDE
#include <bdlbb_blob.h>
#include <bsl_iostream.h>

namespace BloombergLP {
namespace mwcu {

// ==================
// class BlobPosition
// ==================

/// Value-semantic type representing a position in a blob, containing both
/// the buffer and the position in that buffer.
class BlobPosition {
  private:
    // DATA
    int d_buffer;  // The buffer we're pointing into

    int d_byte;  // The byte in 'd_buffer'

  public:
    // CREATORS;

    /// Create an object representing the first byte of the first buffer,
    /// i.e. initialized with default values of zero.
    BlobPosition();

    /// Create an object initialized with the specified `buffer` and `byte`.
    BlobPosition(int buffer, int byte);

    // MANIPULATORS

    /// Set the position to the specified `buffer`.
    void setBuffer(int buffer);

    /// Set the position to the specified `byte`.
    void setByte(int byte);

    // ACCESSORS

    /// Return the buffer attribute.
    int buffer() const;

    /// Return the byte attribute.
    int byte() const;

    bool operator<(const BlobPosition& rhs) const;
    bool operator==(const BlobPosition& rhs) const;
    bool operator!=(const BlobPosition& rhs) const;
    bool operator>=(const BlobPosition& rhs) const;

    /// Write the string representation of this object to the specified
    /// output `stream`, and return a reference to `stream`.  Optionally
    /// specify an initial indentation `level`, whose absolute value is
    /// incremented recursively for nested objects.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this and
    /// all of its nested objects.  If `level` is negative, suppress
    /// indentation of the first line.  If `spacesPerLevel` is negative,
    /// format the entire output on one line, suppressing all but the
    /// initial indentation (as governed by `level`).
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified object to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const BlobPosition& object);

// =================
// class BlobSection
// =================

/// A value-semantic type representing a section of a blob, identified by
/// its start and end positions.
class BlobSection {
  private:
    // DATA
    BlobPosition d_start;  // first byte of the section
    BlobPosition d_end;    // one past the end byte

  public:
    // CREATORS

    /// Create a blob section initialized with default values (zeros).
    BlobSection();

    /// Create a blob section initialized with the specified `start` and
    /// `end` positions, which respectively point to the first byte and to
    /// one past the end byte.
    BlobSection(const BlobPosition& start, const BlobPosition& end);

    // MANIPULATORS

    /// Return a modifiable reference to the start position.
    BlobPosition& start();

    /// Return a modifiable reference to the the end position.
    BlobPosition& end();

    // ACCESSORS

    /// Returns the start position of the section.
    const BlobPosition& start() const;

    /// Return the end position of the section.
    const BlobPosition& end() const;

    /// Write the string representation of this object to the specified
    /// output `stream`, and return a reference to `stream`.  Optionally
    /// specify an initial indentation `level`, whose absolute value is
    /// incremented recursively for nested objects.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this and
    /// all of its nested objects.  If `level` is negative, suppress
    /// indentation of the first line.  If `spacesPerLevel` is negative,
    /// format the entire output on one line, suppressing all but the
    /// initial indentation (as governed by `level`).
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `object` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const BlobSection& object);

// ===============
// struct BlobUtil
// ===============

/// This struct implements a set of static utility functions performing
/// data transfer between bdlbb::Blob data containers and raw memory
/// buffers.
struct BlobUtil {
    /// Return the size of the buffer at the specified `index` in the
    /// specified `blob`.  This will return the buffer's size if it's not
    /// the last blob buffer, and `lastDataBufferLength()` if it is.  The
    /// behavior is undefined if `blob` doesn't have contain at least
    /// `index` buffers.
    static int bufferSize(const bdlbb::Blob& blob, int index);

    /// Return `true` if the specified `pos` is a valid position for the
    /// specified `blob`.  A position is valid if it lies within one of the
    /// buffers of the `blob`, or if it is at byte 0 of one past the last
    /// buffer of the `blob`.
    static bool isValidPos(const bdlbb::Blob& blob, const BlobPosition& pos);

    /// Find the position `offset` bytes from the optionally-specified
    /// `start` and place it in the specified `pos`  If `start` isn't
    /// provided, the search starts at the beginning of the specified
    /// `blob`.  Return 0 on success or a non-zero value if the range
    /// [start, start + offset] doesn't fall within the `blob`.
    static int
    findOffsetSafe(BlobPosition* pos, const bdlbb::Blob& blob, int offset);
    static int findOffsetSafe(BlobPosition*       pos,
                              const bdlbb::Blob&  blob,
                              const BlobPosition& start,
                              int                 offset);

    /// Find the position `offset` bytes from the optionally-specified
    /// `start` and place it in the specified `pos` If `start` isn't
    /// provided, the search starts at the beginning of the specified
    /// `blob`.  Return 0 on success or a non-zero value if the range
    /// [start, start + offset] doesn't fall within the `blob`.  Note that
    /// this method does not check if `start` is a valid position in the
    /// blob.
    static int
    findOffset(BlobPosition* pos, const bdlbb::Blob& blob, int offset);
    static int findOffset(BlobPosition*       pos,
                          const bdlbb::Blob&  blob,
                          const BlobPosition& start,
                          int                 offset);

    /// Return `true` if the specified `section` is a valid section of the
    /// specified `blob`, and `false` otherwise.  A section is valid if
    /// `section.start() < section.end()`  and both the start and the end
    /// are valid positions in the blob.
    static bool isValidSection(const bdlbb::Blob& blob,
                               const BlobSection& section);

    /// Find the distance from the specified `section`s `start()` to the
    /// `section`s `end()` in the specified `blob` and return it in the
    /// specified `size`.  Return `0` on success or a negative value if the
    /// `section` isn't valid.
    static int sectionSize(int*               size,
                           const bdlbb::Blob& blob,
                           const BlobSection& section);

    /// Find the offset into the specified `blob` corresponding to the
    /// specified `position` and return it in the specified `offset`.
    /// Return `0` on success or a negative value if `position` isn't
    /// valid.
    static int positionToOffsetSafe(int*                offset,
                                    const bdlbb::Blob&  blob,
                                    const BlobPosition& position);

    /// Find the offset into the specified `blob` corresponding to the
    /// specified `position` and return it in the specified `offset`.
    /// Return `0`.  Note that this method does not check if `position` is a
    /// valid position in the blob.
    static int positionToOffset(int*                offset,
                                const bdlbb::Blob&  blob,
                                const BlobPosition& position);

    /// Reserve the specified `length` bytes at the end of the specified
    /// `blob`, and if the optionally specified `pos` is provided, load
    /// into it the position of the first byte of the reserved bytes.
    static void reserve(bdlbb::Blob* blob, int length);
    static void reserve(BlobPosition* pos, bdlbb::Blob* blob, int length);

    /// Overwrite `length` characters in the specified `blob` at the
    /// specified `pos` with the specified `buf`.  Return `0` on success or
    /// a negative value if the blob is too short.
    static int writeBytes(const bdlbb::Blob*  blob,
                          const BlobPosition& pos,
                          const char*         buf,
                          int                 length);

    /// Append to the blob pointed by `destination` `length` bytes of data
    /// starting from offset `offsetInBuffer` from the beginning of the
    /// buffer `startBufferIndex` in blob `source`.  The blob object pointed
    /// by `destination` must have enough preallocated buffers to
    /// accommodate the appended data or must be supplied with a buffer
    /// factory allowing to extend the blob dynamically.  This
    /// function does not reassign blob buffers from one blob object to
    /// another but instead uses memcpy() to copy data from the buffers
    /// owned by `source` to the buffers owned by `destination`.
    static void appendBlobFromIndex(bdlbb::Blob*       destination,
                                    const bdlbb::Blob& source,
                                    int                startBufferIndex,
                                    int                offsetInBuffer,
                                    int                length);

    /// Append to the specified `dest` blob the specified `section` of the
    /// specified `src` blob.  Note that if the starting blob buffer of
    /// `src` actually comes from the same block of memory as the last
    /// buffer of `dest` then the buffer in `dest` will simply be extended
    /// to avoid creating unnecessary blob buffers.  Return 0 on success or
    /// a non-zero value if the specified `section` doesn't fall within
    /// the `src` blob'.
    static int appendToBlob(bdlbb::Blob*       dest,
                            const bdlbb::Blob& src,
                            const BlobSection& section);

    /// Append to the specified `dest` blob the specified `length` bytes of
    /// the specified `src` blob, starting at the specified `start`.
    /// Note that if the starting blob buffer of `src` actually comes from
    /// the same block of memory as the last buffer of `dest` then the
    /// buffer in `dest` will simply be extended to avoid creating
    /// unnecessary blob buffers.  Return `0` on success or a non-zero
    /// value if [start, start + length] doesn't fall within the `src`
    /// blob.
    static int appendToBlob(bdlbb::Blob*        dest,
                            const bdlbb::Blob&  src,
                            const BlobPosition& start,
                            int                 length);

    /// Append to the specified' dest' blob the specified `src` blob from
    /// the specified `start` position to the end of the blob.  Return 0 on
    /// success or a non-zero value if the `start` position doesn't fall
    /// within the `src` blob.
    static int appendToBlob(bdlbb::Blob*        dest,
                            const bdlbb::Blob&  src,
                            const BlobPosition& start);

    /// Copy to memory buffer `destination` `length` bytes of data starting
    /// from offset `offsetInBuffer` of blob buffer with index
    /// `startBufferIndex` in blob `source`.  The buffer `destination` must
    /// have enough space to accommodate the copied data, and the blob
    /// `source` must have at least `length` bytes of data starting from the
    /// offset `offsetInBuffer` in blob buffer `startBufferIndex`.
    static void copyToRawBufferFromIndex(char*              destination,
                                         const bdlbb::Blob& source,
                                         int                startBufferIndex,
                                         int                offsetInBuffer,
                                         int                length);

    /// Compare the specified `length` bytes of the specified `data`
    /// against the section of the specified `blob` starting at the
    /// specified `pos`.  If this section lies within the `blob` return `0`
    /// and in the specified `cmpResult` return `0` if the sections are
    /// equal, `-1` if the section in the `blob` is lexicographically less
    /// than the `data` section, and `1` if the `data` section is
    /// lexicographically less than the `blob`s section'.  Return a
    /// negative value if the section doesn't fit in the blob.
    static int compareSection(int*                cmpResult,
                              const bdlbb::Blob&  blob,
                              const BlobPosition& pos,
                              const char*         data,
                              int                 length);

    /// Read up to the specified `length` bytes starting from the specified
    /// `start` position in the specified `blob` into the specified `buf`.
    /// Return a negative value if the `start` position isn't valid,
    /// otherwise return the number of bytes read.
    static int readUpToNBytes(char*               buf,
                              const bdlbb::Blob&  blob,
                              const BlobPosition& start,
                              int                 length);

    /// Read the range of bytes [start, start + length) into the
    /// specified `buf`.  Return 0 on success or a negative
    /// value if the range [start, start + length) doesn't fall within the
    /// specified `blob`.
    static int readNBytes(char*               buf,
                          const bdlbb::Blob&  blob,
                          const BlobPosition& start,
                          int                 length);

    /// Return a pointer to an aligned buffer of the section of the
    /// specified `blob` of the specified `length` starting at the
    /// specified `start` position.   If this section of the `blob` spans
    /// multiple buffers or isn't aligned according to the specified
    /// `alignment`, copy it to the specified `storage` if the specified
    /// `copyFromBlob` is true and return `storage`, or a null pointer if
    /// the section isn't valid.  The behavior is undefined unless
    /// `alignment` is a positive, integral power of 2 and `storage` can
    /// hold at least `length` bytes.
    static char* getAlignedSectionSafe(char*               storage,
                                       const bdlbb::Blob&  blob,
                                       const BlobPosition& start,
                                       int                 length,
                                       int                 alignment,
                                       bool                copyFromBlob);

    /// Return a pointer to an aligned buffer of the section of the
    /// specified `blob` of the specified `length` starting at the specified
    /// `start` position.  If this section of the `blob` spans multiple
    /// buffers or isn't aligned according to the specified `alignment`,
    /// copy it to the specified `storage` if the specified `copyFromBlob`
    /// is true and return `storage`, or a null pointer if the section isn't
    /// valid.  The behavior is undefined unless `alignment` is a positive,
    /// integral power of 2 and `storage` can hold at least `length` bytes.
    /// Note that this method does not check if `start` and `length` are
    /// valid wrt `blob`.
    static char* getAlignedSection(char*               storage,
                                   const bdlbb::Blob&  blob,
                                   const BlobPosition& start,
                                   int                 length,
                                   int                 alignment,
                                   bool                copyFromBlob);

    /// Return a pointer to an object of the parameterized type `T` in the
    /// specified `blob` at the specified `start` position.  If the object
    /// does not span multiple blob buffers and is aligned correctly for
    /// its type, a pointer to the `blob`s memory is returned.  Otherwise,
    /// the object is copied into the specified `storage` if `copyFromBlob`
    /// is true, and a pointer to `storage` is returned.  Return NULL if
    /// `start` is not a valid position or offset in the `blob`, or the
    /// blob is too short.
    template <typename TYPE>
    static TYPE* getAlignedObjectSafe(TYPE*               storage,
                                      const bdlbb::Blob&  blob,
                                      const BlobPosition& start,
                                      bool                copyFromBlob);

    /// Return a pointer to an object of the parameterized type `T` in the
    /// specified `blob` at the specified `start` position.  If the object
    /// does not span multiple blob buffers and is aligned correctly for
    /// its type, a pointer to the `blob`s memory is returned.  Otherwise,
    /// the object is copied into the specified `storage` if `copyFromBlob`
    /// is true, and a pointer to `storage` is returned.  Return NULL if
    /// `start` is not a valid position or offset in the `blob`, or the
    /// blob is too short.  Note that this method does not check if `start`
    /// is valid wrt `blob`.
    template <typename TYPE>
    static TYPE* getAlignedObject(TYPE*               storage,
                                  const bdlbb::Blob&  blob,
                                  const BlobPosition& start,
                                  bool                copyFromBlob);
};

// ========================
// class BlobStartHexDumper
// ========================

class BlobStartHexDumper {
    // A pretty printer for blobs that prints up to a specified number of
    // bytes of a blob.

  private:
    // DATA
    const bdlbb::Blob* d_blob_p;  // The blob to dump

    int d_length;  // The maximum number of bytes to print.

    // FRIENDS
    friend bsl::ostream& operator<<(bsl::ostream&, const BlobStartHexDumper&);

  public:
    // CREATORS
    BlobStartHexDumper(const bdlbb::Blob* blob);
    // Create a 'BlobStartHexDumper' set to dump the specified 'blob', up
    // to the first kilobyte.

    BlobStartHexDumper(const bdlbb::Blob* blob, int length);
    // Create a 'BlobStartHexDumper' set to dump the specified 'blob', up
    // to 'length' number of bytes.
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, const BlobStartHexDumper& src);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// class BlobPosition
// ------------------

// CREATORS
inline BlobPosition::BlobPosition()
: d_buffer(0)
, d_byte(0)
{
    // NOTHING
}

inline BlobPosition::BlobPosition(int buffer, int byte)
: d_buffer(buffer)
, d_byte(byte)
{
    // NOTHING
}

// MANIPULATORS
inline void BlobPosition::setBuffer(int buffer)
{
    d_buffer = buffer;
}

inline void BlobPosition::setByte(int byte)
{
    d_byte = byte;
}

// ACCESSORS
inline int BlobPosition::buffer() const
{
    return d_buffer;
}

inline int BlobPosition::byte() const
{
    return d_byte;
}

inline bool BlobPosition::operator<(const BlobPosition& rhs) const
{
    return (d_buffer < rhs.d_buffer) ||
           ((d_buffer == rhs.d_buffer) && (d_byte < rhs.d_byte));
}

inline bool BlobPosition::operator==(const BlobPosition& rhs) const
{
    return (d_buffer == rhs.d_buffer) && (d_byte == rhs.d_byte);
}

inline bool BlobPosition::operator!=(const BlobPosition& rhs) const
{
    return !(*this == rhs);
}

inline bool BlobPosition::operator>=(const BlobPosition& rhs) const
{
    return (rhs < *this) || (*this == rhs);
}

// -----------------
// class BlobSection
// -----------------

// CREATORS
inline BlobSection::BlobSection()
: d_start()
, d_end()
{
}

inline BlobSection::BlobSection(const BlobPosition& start,
                                const BlobPosition& end)
: d_start(start)
, d_end(end)
{
}

// MANIPULATORS
inline BlobPosition& BlobSection::start()
{
    return d_start;
}

inline BlobPosition& BlobSection::end()
{
    return d_end;
}

// ACCESSORS
inline const BlobPosition& BlobSection::start() const
{
    return d_start;
}

inline const BlobPosition& BlobSection::end() const
{
    return d_end;
}

// ---------------
// struct BlobUtil
// ---------------

inline int BlobUtil::bufferSize(const bdlbb::Blob& blob, int index)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(index < blob.numDataBuffers());

    return index == blob.numDataBuffers() - 1 ? blob.lastDataBufferLength()
                                              : blob.buffer(index).size();
}

inline int BlobUtil::findOffsetSafe(BlobPosition*      pos,
                                    const bdlbb::Blob& blob,
                                    int                offset)
{
    return findOffsetSafe(pos, blob, BlobPosition(), offset);
}

inline int BlobUtil::findOffsetSafe(BlobPosition*       pos,
                                    const bdlbb::Blob&  blob,
                                    const BlobPosition& start,
                                    int                 offset)
{
    // Check if 'start' is a valid position in the blob.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValidPos(blob, start))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    // Check if 'offset' has a proper value if 'start' position points to
    // the buffer past the last one of the blob.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            start.buffer() == blob.numDataBuffers() && offset > 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -2;  // RETURN
    }

    return findOffset(pos, blob, start, offset);
}

inline int
BlobUtil::findOffset(BlobPosition* pos, const bdlbb::Blob& blob, int offset)
{
    return findOffset(pos, blob, BlobPosition(), offset);
}

inline int BlobUtil::positionToOffsetSafe(int*                offset,
                                          const bdlbb::Blob&  blob,
                                          const BlobPosition& position)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValidPos(blob, position))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    return positionToOffset(offset, blob, position);
}

inline void BlobUtil::reserve(bdlbb::Blob* blob, int length)
{
    blob->setLength(blob->length() + length);
}

template <typename TYPE>
TYPE* BlobUtil::getAlignedObjectSafe(TYPE*               storage,
                                     const bdlbb::Blob&  blob,
                                     const BlobPosition& start,
                                     bool                copyFromBlob)
{
    return reinterpret_cast<TYPE*>(
        getAlignedSectionSafe(reinterpret_cast<char*>(storage),
                              blob,
                              start,
                              sizeof(TYPE),
                              bsls::AlignmentFromType<TYPE>::VALUE,
                              copyFromBlob));
}

template <typename TYPE>
TYPE* BlobUtil::getAlignedObject(TYPE*               storage,
                                 const bdlbb::Blob&  blob,
                                 const BlobPosition& start,
                                 bool                copyFromBlob)
{
    return reinterpret_cast<TYPE*>(
        getAlignedSection(reinterpret_cast<char*>(storage),
                          blob,
                          start,
                          sizeof(TYPE),
                          bsls::AlignmentFromType<TYPE>::VALUE,
                          copyFromBlob));
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mwcu::operator<<(bsl::ostream&             stream,
                                      const mwcu::BlobPosition& object)
{
    return object.print(stream, 0, -1);
}

inline bsl::ostream& mwcu::operator<<(bsl::ostream&            stream,
                                      const mwcu::BlobSection& object)
{
    return object.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
