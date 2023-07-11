// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqp_queueid.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQP_QUEUEID
#define INCLUDED_BMQP_QUEUEID

//@PURPOSE: Provide a value-semantic type for a key related to a queue.
//
//@CLASSES:
//  bmqp::QueueId: VST constituting a queue key
//
//@DESCRIPTION: 'bmqp::QueueId' provides a value-semantic type, 'QueueId',
// which is used as a key for a queue.
//

// BMQ

// BDE
#include <bsl_iosfwd.h>
#include <bslh_hash.h>

namespace BloombergLP {
namespace bmqp {

// =============
// class QueueId
// =============

/// Value-semantic type for a key related to a queue.
class QueueId {
  public:
    // PUBLIC CONSTANTS

    /// The first reserved queueId: any queueId >= this value are reserved
    /// for special meaning.
    static const unsigned int k_RESERVED_QUEUE_ID = static_cast<unsigned int>(
        -10);

    /// Constant representing the numeric value of the `primary` of a queue.
    static const unsigned int k_PRIMARY_QUEUE_ID = static_cast<unsigned int>(
        -2);

    static const unsigned int k_UNASSIGNED_QUEUE_ID =
        static_cast<unsigned int>(-1);

    /// The first reserved subQueueId: any subQueueId >= this value are
    /// reserved for special meaning.
    static const unsigned int k_RESERVED_SUBQUEUE_ID =
        static_cast<unsigned int>(-10);  // 4294967286

    /// Constant representing the numeric value of an invalid, or unassigned
    /// subQueue id.
    static const unsigned int k_UNASSIGNED_SUBQUEUE_ID =
        static_cast<unsigned int>(-1);  // 4294967295

    /// Default SubQueueId.  This is the value used for any queue that
    /// doesn't have an `appId` specified, and it will be omitted from
    /// communication with upstream.
    static const unsigned int k_DEFAULT_SUBQUEUE_ID = 0;

    // PUBLIC TYPES

    /// Value-semantic type to assist printing integer queue ids.
    struct QueueIdInt {
        // PUBLIC DATA
        int d_value;

        // CREATORS
        explicit QueueIdInt(int value);
    };

    /// Value-semantic type to assist printing integer subqueue ids.
    struct SubQueueIdInt {
        // PUBLIC DATA
        unsigned int d_value;

        // CREATORS
        explicit SubQueueIdInt(unsigned int id);
    };

  private:
    // DATA
    QueueIdInt    d_id;
    SubQueueIdInt d_subId;

  public:
    // CREATORS

    /// Create a `bmqp::QueueId` object using the specified `id` and the
    /// optionally specified `subId`.
    explicit QueueId(int id, unsigned int subId = k_DEFAULT_SUBQUEUE_ID);

    // MANIPULATORS
    QueueId& setId(int value);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    QueueId& setSubId(unsigned int value);

    // ACCESSORS
    int id() const;

    /// Get the value of the corresponding attribute.
    unsigned int subId() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained as contained in the specified `lhs` object and
/// the value itself is the same in both objects, return false otherwise.
bool operator==(const QueueId& lhs, const QueueId& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const QueueId& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const QueueId::QueueIdInt& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                 stream,
                         const QueueId::SubQueueIdInt& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class QueueId
// -------------

// CREATORS
inline QueueId::QueueId(int id, unsigned int subId)
: d_id(id)
, d_subId(subId)
{
    // NOTHING
}

inline QueueId::QueueIdInt::QueueIdInt(int value)
: d_value(value)
{
    // NOTHING
}

inline QueueId::SubQueueIdInt::SubQueueIdInt(unsigned int value)
: d_value(value)
{
    // NOTHING
}

// MANIPULATORS
inline QueueId& QueueId::setId(int value)
{
    d_id.d_value = value;
    return *this;
}

inline QueueId& QueueId::setSubId(unsigned int value)
{
    d_subId.d_value = value;
    return *this;
}

// ACCESSORS
inline int QueueId::id() const
{
    return d_id.d_value;
}

inline unsigned int QueueId::subId() const
{
    return d_subId.d_value;
}

// FREE FUNCTIONS

/// Apply the specified `hashAlgo` to the specified `queueId`.
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const QueueId& queueId)
{
    using bslh::hashAppend;  // for ADL
    hashAppend(hashAlgo, queueId.id());
    hashAppend(hashAlgo, queueId.subId());
}

}  // close package namespace

// FREE OPERATORS
inline bool bmqp::operator==(const QueueId& lhs, const QueueId& rhs)
{
    return lhs.id() == rhs.id() && lhs.subId() == rhs.subId();
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream&        stream,
                                      const bmqp::QueueId& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
