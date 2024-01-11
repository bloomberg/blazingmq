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

// mwcu_blobiterator.h                                                -*-C++-*-
#ifndef INCLUDED_MWCU_BLOBITERATOR
#define INCLUDED_MWCU_BLOBITERATOR

//@PURPOSE: Provide an iterator over a section of a blob
//
//@CLASSES:
// mwcu::BlobIterator: a blob iterator
//
//@DESCRIPTION:
// This component defines a mechanism, 'mwcu::BlobIterator', useful for
// iterating over a section of a blob, starting at a given position, and
// proceeding either for a specified length, or for a specified number of
// calls to 'advance'.

// MWC

#include <mwcu_blob.h>

// BDE
#include <bsl_cstddef.h>

namespace BloombergLP {
namespace mwcu {

// ==================
// class BlobIterator
// ==================

/// A blob iterator
class BlobIterator {
  private:
    // DATA
    const bdlbb::Blob* d_blob_p;  // the blob we're iterating over

    mwcu::BlobPosition d_pos;  // current position

    int d_remaining;  // distance to travel, or number of
                      // advancements remaining until we're
                      // done

    bool d_advancingLength;
    // If true, then 'd_remaining' is the
    // length remaining to be traversed.
    // Otherwise, 'd_remaining' is the
    // number of advancements that will
    // result in valid positions in the
    // blob

  public:
    // CREATORS

    /// Create a default-initialized instance of this class.
    BlobIterator();

    /// Create a `BlobIterator` over the specified `blob`, starting at the
    /// specified `start`, proceeding for the specified `lengthOrSteps`
    /// bytes if the specified `advancingLength` is true, or for
    /// `lengthOrSteps` calls to `advance` if `advancingLength` is false,
    /// until reaching its end.
    BlobIterator(const bdlbb::Blob*        blob,
                 const mwcu::BlobPosition& start,
                 int                       lengthOrSteps,
                 bool                      advancingLength);

    BlobIterator(const BlobIterator& other);

    // MANIPULATORS

    /// Assignment operator
    BlobIterator& operator=(const BlobIterator& rhs);

    /// Prefix increment operator
    const BlobIterator& operator++();

    /// Postfix increment operator
    BlobIterator operator++(int);

    /// Reset the `BlobIterator` over the specified `blob`, starting at the
    /// specified `start`, proceeding for the specified `lengthOrSteps`
    /// bytes if the specified `advancingLength` is true, or for
    /// `lengthOrSteps` calls to `advance` if `advancingLength` is false,
    /// until reaching its end.
    void reset(const bdlbb::Blob*        blob,
               const mwcu::BlobPosition& start,
               int                       lengthOrSteps,
               bool                      advancingLength);

    /// Advance the specified `length` bytes further into the blob.  Return
    /// the value of `!atEnd` after advancing.  The result is undefined
    /// unless `atEnd` is false when this is called.
    bool advance(int length);

    /// Advance the specified `length` bytes further into the blob.  Return
    /// the value of `!atEnd` after advancing.  The result is undefined
    /// unless `atEnd` is false when this is called.  Note that this method
    /// does not check if there are more than `length` remaining bytes in
    /// the blob.
    bool advanceRaw(int length);

    // ACCESSORS

    /// Return true if this iterator refers to the same value as `rhs`
    /// and false otherwise.
    bool operator==(const BlobIterator& rhs) const;

    /// Return false if this iterator refers to the same value as `rhs`
    /// and true otherwise.
    bool operator!=(const BlobIterator& rhs) const;

    /// Return a reference to the byte being referred to by this
    /// iterator.  The behavior is undefined if this iterator isn't
    /// referring to a valid value.
    const char& operator*() const;

    /// Return a pointer to the byte being referred to by this
    /// iterator.  The behavior is undefined if this iterator isn't
    /// referring to a valid value.
    const char* operator->() const;

    /// Return `true` if this `BlobIterator` has reached its end.
    bool atEnd() const;

    /// Return a non-modifiable reference to the current position of this
    /// iterator in the `blob`.  Note that it is legal to call this when
    /// `atEnd` returns true.
    const mwcu::BlobPosition& position() const;

    /// Return the distance or number of steps remaining before this
    /// iterator reaches its end.
    int remaining() const;

    /// Return the blob we're iterating over.
    const bdlbb::Blob* blob() const;

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
bsl::ostream& operator<<(bsl::ostream& stream, const BlobIterator& object);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// class BlobIterator
// ------------------

// CREATORS
inline BlobIterator::BlobIterator(const bdlbb::Blob*        blob,
                                  const mwcu::BlobPosition& start,
                                  int                       lengthOrSteps,
                                  bool                      advancingLength)
: d_blob_p(blob)
, d_pos(start)
, d_remaining(lengthOrSteps)
, d_advancingLength(advancingLength)
{
}

inline BlobIterator::BlobIterator()
: d_blob_p(NULL)
, d_remaining(0)
, d_advancingLength(false)
{
}

inline BlobIterator::BlobIterator(const BlobIterator& other)
: d_blob_p(other.d_blob_p)
, d_pos(other.d_pos)
, d_remaining(other.d_remaining)
, d_advancingLength(other.d_advancingLength)
{
}

// MANIPULATORS
inline BlobIterator& BlobIterator::operator=(const BlobIterator& rhs)
{
    d_blob_p          = rhs.d_blob_p;
    d_pos             = rhs.d_pos;
    d_remaining       = rhs.d_remaining;
    d_advancingLength = rhs.d_advancingLength;

    return *this;
}

inline const BlobIterator& BlobIterator::operator++()
{
    advance(1);
    return *this;
}

inline BlobIterator BlobIterator::operator++(int)
{
    BlobIterator r(*this);
    ++*this;
    return r;
}

inline void BlobIterator::reset(const bdlbb::Blob*        blob,
                                const mwcu::BlobPosition& start,
                                int                       lengthOrSteps,
                                bool                      advancingLength)
{
    d_blob_p          = blob;
    d_pos             = start;
    d_remaining       = lengthOrSteps;
    d_advancingLength = advancingLength;
}

inline bool BlobIterator::advance(int length)
{
    int rc = mwcu::BlobUtil::findOffsetSafe(&d_pos, *d_blob_p, d_pos, length);
    if (rc != 0) {
        d_remaining = -1;
        return false;  // RETURN
    }
    d_remaining -= (d_advancingLength ? length : 1);
    return !atEnd();
}

inline bool BlobIterator::advanceRaw(int length)
{
    int rc = mwcu::BlobUtil::findOffset(&d_pos, *d_blob_p, d_pos, length);
    if (rc != 0) {
        d_remaining = -1;
        return false;  // RETURN
    }
    d_remaining -= (d_advancingLength ? length : 1);
    return !atEnd();
}

// ACCESSORS
inline bool BlobIterator::operator==(const BlobIterator& rhs) const
{
    return (d_blob_p == rhs.d_blob_p && d_pos == rhs.d_pos) ||
           (atEnd() && rhs.atEnd());
}

inline bool BlobIterator::operator!=(const BlobIterator& rhs) const
{
    return !(*this == rhs);
}

inline const char& BlobIterator::operator*() const
{
    return d_blob_p->buffer(d_pos.buffer()).data()[d_pos.byte()];
}

inline const char* BlobIterator::operator->() const
{
    return &**this;
}

inline bool BlobIterator::atEnd() const
{
    return d_remaining <= 0;
}

inline const mwcu::BlobPosition& BlobIterator::position() const
{
    return d_pos;
}

inline int BlobIterator::remaining() const
{
    return d_remaining;
}

inline const bdlbb::Blob* BlobIterator::blob() const
{
    return d_blob_p;
}

inline bsl::ostream&
BlobIterator::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    return d_pos.print(stream, level, spacesPerLevel);
}

// FREE OPERATORS
inline bsl::ostream& operator<<(bsl::ostream&       stream,
                                const BlobIterator& object)
{
    return object.print(stream, 0, 4);
}

}  // close package namespace
}  // close enterprise namespace

#endif
