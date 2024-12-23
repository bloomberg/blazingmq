// Copyright 2014-2023 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGETOOL_COMPOSITESEQUENCENUMBER
#define INCLUDED_M_BMQSTORAGETOOL_COMPOSITESEQUENCENUMBER

//@PURPOSE: Provide value-semantic type to represent composite sequence number
//(consists of primary lease Id and sequence number),
// which is used for message filtering.
//
//@CLASSES:
//  m_bmqstoragetool::CompositeSequenceNumber: Value-semantic type to represent
//  composite sequence number.
//
//@DESCRIPTION: 'CompositeSequenceNumber' provides value-semantic type to
// represent composite sequence number.
// There could be sequence numbers collision inside journal file for different
// lease Ids, so need to handle composite sequence number taking into account
// primary lease Id too.

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =============================
// class CompositeSequenceNumber
// =============================

class CompositeSequenceNumber {
  private:
    // DATA
    bsls::Types::Uint64 d_leaseId;
    // Primary Lease Id
    bsls::Types::Uint64 d_seqNumber;
    // Sequence Number
    bool d_isSet;
    // Set to `true` if the value of this object is set

  public:
    // CREATORS

    /// Create an un-initialized CompositeSequenceNumber.  Note that
    /// `isSet()` would return false.
    CompositeSequenceNumber();

    /// Create CompositeSequenceNumber from the specified `leaseId` and
    /// `sequenceNumber`
    explicit CompositeSequenceNumber(const bsls::Types::Uint64 leaseId,
                                     const bsls::Types::Uint64 sequenceNumber);

    // MANIPULATORS

    /// Initialize this CompositeSequenceNumber from the specified
    /// `seqNumString` representation in format `<leaseId>-<sequenceNumber>`.
    /// Return a reference offering modifiable access to this object. If
    /// convertion is successfull, `isSet()` would return `true`. Otherwise,
    /// `isSet()` would return `false` and specified `errorDescription` is
    /// filled with error description.
    CompositeSequenceNumber& fromString(bsl::ostream&      errorDescription,
                                        const bsl::string& seqNumString);

    // ACCESSORS

    /// Return `true` if the value of this object is not set.
    bool isSet() const;

    /// Return Primary Lease Id value.
    bsls::Types::Uint64 leaseId() const;

    /// Return Sequence Number value.
    bsls::Types::Uint64 sequenceNumber() const;

    /// Write the value of this object to the specified output `stream` in a
    /// human-readable format, and return a reference to `stream`.
    /// Optionally specify an initial indentation `level`.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this
    /// object.  If `level` is negative, suppress indentation of the first
    /// line.  If `spacesPerLevel` is negative, format the entire output on
    /// one line, suppressing all but the initial indentation (as governed
    /// by `level`).  If `stream` is not valid on entry, this operation has
    /// no effect.  Note that this human-readable format is not fully
    /// specified, and can change without notice.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

// -----------------------------
// class CompositeSequenceNumber
// -----------------------------

/// Write the value of the specified `rhs` object to the specified output
/// `stream` in a human-readable format, and return a reference to `stream`.
/// Note that this human-readable format is not fully specified, and can
/// change without notice.
bsl::ostream& operator<<(bsl::ostream&                  stream,
                         const CompositeSequenceNumber& rhs);

/// Return true if the specified `lhs` instance is equal to the
/// specified `rhs` instance, false otherwise.
bool operator==(const CompositeSequenceNumber& lhs,
                const CompositeSequenceNumber& rhs);

/// Return true if the specified `lhs` instance is less than the
/// specified `rhs` instance, false otherwise.
bool operator<(const CompositeSequenceNumber& lhs,
               const CompositeSequenceNumber& rhs);

/// Return true if the specified `lhs` instance is less or equal to the
/// specified `rhs` instance, false otherwise.
bool operator<=(const CompositeSequenceNumber& lhs,
                const CompositeSequenceNumber& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// =============================
// class CompositeSequenceNumber
// =============================

// ACCESSORS

inline bool CompositeSequenceNumber::isSet() const
{
    return d_isSet;
}

inline bsls::Types::Uint64 CompositeSequenceNumber::leaseId() const
{
    return d_leaseId;
}

inline bsls::Types::Uint64 CompositeSequenceNumber::sequenceNumber() const
{
    return d_seqNumber;
}

}  // close package namespace

// =============================
// class CompositeSequenceNumber
// =============================

// FREE OPERATORS

inline bsl::ostream& m_bmqstoragetool::operator<<(
    bsl::ostream&                                    stream,
    const m_bmqstoragetool::CompositeSequenceNumber& rhs)
{
    // PRECONDITIONS
    BSLS_ASSERT(rhs.isSet());

    return rhs.print(stream, 0, -1);
}

inline bool m_bmqstoragetool::operator==(
    const m_bmqstoragetool::CompositeSequenceNumber& lhs,
    const m_bmqstoragetool::CompositeSequenceNumber& rhs)
{
    // PRECONDITIONS
    BSLS_ASSERT(lhs.isSet() && rhs.isSet());

    return (lhs.leaseId() == rhs.leaseId() &&
            lhs.sequenceNumber() == rhs.sequenceNumber());
}

inline bool m_bmqstoragetool::operator<(
    const m_bmqstoragetool::CompositeSequenceNumber& lhs,
    const m_bmqstoragetool::CompositeSequenceNumber& rhs)
{
    // PRECONDITIONS
    BSLS_ASSERT(lhs.isSet() && rhs.isSet());

    // Check leaseId first
    if (lhs.leaseId() < rhs.leaseId()) {
        return true;  // RETURN
    }
    else if (lhs.leaseId() == rhs.leaseId()) {
        if (lhs.sequenceNumber() < rhs.sequenceNumber()) {
            return true;  // RETURN
        }
    }

    return false;
}

inline bool m_bmqstoragetool::operator<=(
    const m_bmqstoragetool::CompositeSequenceNumber& lhs,
    const m_bmqstoragetool::CompositeSequenceNumber& rhs)
{
    // PRECONDITIONS
    BSLS_ASSERT(lhs.isSet() && rhs.isSet());

    return (lhs < rhs || lhs == rhs);
}

}  // close enterprise namespace

#endif
