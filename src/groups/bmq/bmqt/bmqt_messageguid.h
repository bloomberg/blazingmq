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

// bmqt_messageguid.h                                                 -*-C++-*-
#ifndef INCLUDED_BMQT_MESSAGEGUID
#define INCLUDED_BMQT_MESSAGEGUID

//@PURPOSE: Provide a value-semantic global unique identifier for BlazingMQ
// messages.
//
//@CLASSES:
//  bmqt::MessageGUID         : Value-semantic global unique ID for BlazingMQ
//                              message
//  bmqt::MessageGUIDLess     : Binary function for comparing GUIDs.
//  bmqt::MessageGUIDHashAlgo : Provide a hashing algorithm for Message GUID.
//
//@DESCRIPTION: 'bmqt::MessageGUID' provides a value-semantic global unique
// identifier for BlazingMQ messages.  Each bmqa::Message delivered to
// BlazingMQ client from BlazingMQ broker contains a unique MessageGUID.  The
// binary function 'bmqt::MessageGUIDLess' can be used for comparing GUIDs, and
// an optimized custom hash function is provided with
// 'bmqt::MessageGUIDHashAlgo'.
//
//
/// Externalization
///---------------
// For convenience, this class provides 'toHex' method that can be used to
// externalize a 'bmqt::MessageGUID' instance.  Applications can persist the
// resultant buffer (on filesystem, in database) to keep track of last
// processed message ID across task instantiations.  'fromHex' method can be
// used to convert a valid externalized buffer back to a message ID.
//
/// Efficient comparison and hash function
///--------------------------------------
// This component also provides efficient comparison and hash functions for
// convenience, and thus, applications can use this component as a key in
// associative containers.
//
//
/// Example 1: Externalizing
///  - - - - - - - - - - - -
//..
//  // Below, 'msg' is a valid instance of 'bmqa::Message' obtained from an
//  // instance of 'bmqa::Session':
//
//  bmqt::MessageGUID g1 = msg.messageId();
//
//  char buffer[bmqt::MessageGUID::e_SIZE_HEX];
//  g1.toHex(buffer);
//
//  BSLS_ASSERT(true == bmqt::MessageGUID::isValidHexRepresentation(buffer));
//
//  bmqt::MessageGUID g2;
//  g2.fromHex(buffer);
//
//  BSLS_ASSERT(g1 == g2);
//..
//

// BMQ

// BDE
#include <bsl_cstring.h>  // for bsl::memset, bsl::memcmp

#include <bsl_cstddef.h>

#include <bsl_iosfwd.h>
#include <bslh_hash.h>
#include <bslmf_isbitwiseequalitycomparable.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_annotation.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqt {

// =================
// class MessageGUID
// =================

/// This class provides a value-semantic type to represent a global unique
/// ID for BlazingMQ messages.
class MessageGUID {
    // FRIENDS
    friend bool operator==(const MessageGUID& lhs, const MessageGUID& rhs);
    friend bool operator!=(const MessageGUID& lhs, const MessageGUID& rhs);
    friend struct MessageGUIDLess;
    template <class HASH_ALGORITHM>
    friend void hashAppend(HASH_ALGORITHM& hashAlgo, const MessageGUID& guid);

  public:
    // TYPES
    enum Enum {
        // Enum representing the size of a buffer needed to represent a GUID
        e_SIZE_BINARY = 16  // Binary format of the GUID

        ,
        e_SIZE_HEX = 2 * e_SIZE_BINARY
        // Hexadecimal string representation of the GUID
    };

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MessageGUID,
                                   bslmf::IsBitwiseEqualityComparable)
    BSLMF_NESTED_TRAIT_DECLARATION(MessageGUID, bsl::is_trivially_copyable)

  private:
    // PRIVATE CONSTANTS
    static const char k_UNSET_GUID[e_SIZE_BINARY];
    //  Constant representing an unset GUID

  private:
    // IMPLEMENTATION NOTE: Some structs in bmqp::Protocol.h blindly
    //                      reinterpret_cast a char[e_SIZE_BINARY] to a
    //                      MessageGUID (instead of using 'fromBinary()') so
    //                      that they can return a const MessageGUID&.
    // IMPLEMENTATION NOTE: See mqbu_messageguidutil.cpp for internal layout of
    //                      MessageGUID

    // DATA
    char d_buffer[e_SIZE_BINARY];

  public:
    // CLASS METHODS

    /// Return true if the specified `buffer` is a valid hex representation
    /// of a MessageGUID.  The behavior is undefined unless `buffer` is
    /// non-null and length of the `buffer` is equal to `e_SIZE_HEX`.
    static bool isValidHexRepresentation(const char* buffer);

    // CREATORS

    /// Create an un-initialized MessageGUID.  Note that `isUnset()` would
    /// return true.
    MessageGUID();

    /// Destructor.
    ~MessageGUID();

    // MANIPULATORS

    /// Initialize this MessageGUID with the binary representation from the
    /// specified `buffer`, in binary format.  Behavior is undefined unless
    /// `buffer` is non-null and of length `e_SIZE_BINARY`.  Return a
    /// reference offering modifiable access to this object.
    MessageGUID& fromBinary(const unsigned char* buffer);

    /// Initialize this MessageGUID with the hexadecimal representation from
    /// the specified `buffer`.  Behavior is undefined unless
    /// `isValidHexRepresentation()` returns true for the provided
    /// `buffer`.  Return a reference offering modifiable access to this
    /// object.
    MessageGUID& fromHex(const char* buffer);

    // ACCESSORS

    /// Return `true` if the value of this object is not set.
    bool isUnset() const;

    /// Write the binary representation of this object to the specified
    /// `destination`.  Behavior is undefined unless `destination` is of
    /// length `e_SIZE_BINARY`.  Note that `destination` can later be used
    /// to populate a MessageGUID object using `fromBinary` method.
    void toBinary(unsigned char* destination) const;

    /// Write the hex representation of this object to the specified
    /// `destination`.  Behavior is undefined unless `destination` is of
    /// length `e_SIZE_HEX`.  Note that `destination` can later be used to
    /// populate a MessageGUID object using `fromHex` method.
    void toHex(char* destination) const;

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
    /// specified, and can change without notice.  Applications relying on a
    /// fixed length format must use `toHex` method.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

// -----------------
// class MessageGUID
// -----------------

/// Write the value of the specified `rhs` object to the specified output
/// `stream` in a human-readable format, and return a reference to `stream`.
/// Note that this human-readable format is not fully specified, and can
/// change without notice.  Applications relying on a fixed length format
/// must use `toHex` method.
bsl::ostream& operator<<(bsl::ostream& stream, const MessageGUID& rhs);

/// Return `true` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return false otherwise.
bool operator==(const MessageGUID& lhs, const MessageGUID& rhs);

/// Return `false` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return `true` otherwise.
bool operator!=(const MessageGUID& lhs, const MessageGUID& rhs);

/// Return true if the specified `lhs` instance is smaller than the
/// specified `rhs` instance, false otherwise.  Note that criteria for
/// comparison is implementation defined, and result of this routine *does*
/// *not* in any way signify the order of creation/arrival/delivery of
/// messages to which `lhs` and `rhs` instances belong.  Note that this
/// operator is provided so that MessageGUID can be used as a key in
/// associative containers (map, set, etc).
bool operator<(const MessageGUID& lhs, const MessageGUID& rhs);

// =====================
// class MessageGUIDLess
// =====================

/// This struct provides a binary function for comparing 2 GUIDs.
struct MessageGUIDLess {
    // ACCESSORS

    /// Return `true` if the specified `lhs` should be considered as having
    /// a value less than the specified `rhs`.
    bool operator()(const MessageGUID& lhs, const MessageGUID& rhs) const;
};

// =========================
// class MessageGUIDHashAlgo
// =========================

/// This class provides a hashing algorithm for `bmqt::MessageGUID`.  At the
/// time of writing, this algorithm was found to be approximately 4x faster
/// than the default hashing algorithm that comes with `bslh` package.
/// Performance-critical applications may want to use this hashing algorithm
/// instead of the default one.
class MessageGUIDHashAlgo {
  private:
    // DATA
    bsls::Types::Uint64 d_result;

  public:
    // TYPES
    typedef bsls::Types::Uint64 result_type;

    // CREATORS

    /// Constructor
    MessageGUIDHashAlgo();

    // MANIPULATORS
    void operator()(const void* data, size_t numBytes);

    /// Compute and return the hash for the GUID.
    result_type computeHash();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class MessageGUID
// -----------------

// CREATORS
inline MessageGUID::MessageGUID()
{
    bsl::memcpy(d_buffer, k_UNSET_GUID, e_SIZE_BINARY);
}

inline MessageGUID::~MessageGUID()
{
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    bsl::memcpy(d_buffer, k_UNSET_GUID, e_SIZE_BINARY);
#endif
}

// ACCESSORS
inline bool MessageGUID::isUnset() const
{
    return 0 == bsl::memcmp(d_buffer, k_UNSET_GUID, e_SIZE_BINARY);
}

// FREE FUNCTIONS
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const MessageGUID& guid)
{
    using bslh::hashAppend;               // for ADL
    hashAppend(hashAlgo, guid.d_buffer);  // hashes entire buffer array
}

// ----------------------
// struct MessageGUIDLess
// ----------------------

inline bool MessageGUIDLess::operator()(const MessageGUID& lhs,
                                        const MessageGUID& rhs) const
{
    return 0 >
           bsl::memcmp(lhs.d_buffer, rhs.d_buffer, MessageGUID::e_SIZE_BINARY);
}

// -------------------------
// class MessageGUIDHashAlgo
// -------------------------

// CREATORS
inline MessageGUIDHashAlgo::MessageGUIDHashAlgo()
: d_result(0)
{
}

// MANIPULATORS
inline void
MessageGUIDHashAlgo::operator()(const void*                   data,
                                BSLS_ANNOTATION_UNUSED size_t numBytes)
{
    // Implementation note: we implement the 'djb2' hash algorithm (more
    // details at http://www.cse.yorku.ca/~oz/hash.html).

    // At the time of writing, this algorithm came out to be about 400% faster
    // than 'bslh::SpookyHashAlgorithm', which is the default hashing algorithm
    // in 'bslh' hashing framework.  Note that while
    // 'bslh::SpookyHashAlgorithm' is slower, it may have a better uniform
    // distribution than this algorithm (although some literature claims djb2
    // to have a very good distribution as well).  Both algorithms were found
    // to be collision free in testing (see mqbu_messageguidtutil.t).

    // We have slightly modified the djb2 algorithm by unrolling djb2 'while'
    // loop, by using our knowledge that 'numBytes' is always 16 for
    // 'bmqt::MessageGUID'.  For reference, the unmodified djb2 algorithm has
    // been specified at the end of this method.  Our unrolled version comes
    // out to be about 25% faster than the looped version.  The unrolled
    // version has data dependency, so its not the ILP but probably the absence
    // of branching which makes it faster than the looped version.

    d_result = 5381ULL;

    const char* start = reinterpret_cast<const char*>(data);
    d_result          = (d_result << 5) + d_result + start[0];
    d_result          = (d_result << 5) + d_result + start[1];
    d_result          = (d_result << 5) + d_result + start[2];
    d_result          = (d_result << 5) + d_result + start[3];
    d_result          = (d_result << 5) + d_result + start[4];
    d_result          = (d_result << 5) + d_result + start[5];
    d_result          = (d_result << 5) + d_result + start[6];
    d_result          = (d_result << 5) + d_result + start[7];
    d_result          = (d_result << 5) + d_result + start[8];
    d_result          = (d_result << 5) + d_result + start[9];
    d_result          = (d_result << 5) + d_result + start[10];
    d_result          = (d_result << 5) + d_result + start[11];
    d_result          = (d_result << 5) + d_result + start[12];
    d_result          = (d_result << 5) + d_result + start[13];
    d_result          = (d_result << 5) + d_result + start[14];
    d_result          = (d_result << 5) + d_result + start[15];

    // For reference, 'loop' version of djb2 algorithm:
    //..
    //  size_t index = 0;
    //  while (index++ < numBytes) {
    //      d_result = (d_result << 5) + d_result +  // same as 'd_result * 33'
    //                 (reinterpret_cast<const char*>(data))[index];
    //  }
    //..
}

inline MessageGUIDHashAlgo::result_type MessageGUIDHashAlgo::computeHash()
{
    return d_result;
}

}  // close package namespace

// -----------------
// class MessageGUID
// -----------------

// FREE OPERATORS
inline bool bmqt::operator==(const bmqt::MessageGUID& lhs,
                             const bmqt::MessageGUID& rhs)
{
    return 0 ==
           bsl::memcmp(lhs.d_buffer, rhs.d_buffer, MessageGUID::e_SIZE_BINARY);
}

inline bool bmqt::operator!=(const bmqt::MessageGUID& lhs,
                             const bmqt::MessageGUID& rhs)
{
    return !(lhs == rhs);
}

inline bool bmqt::operator<(const bmqt::MessageGUID& lhs,
                            const bmqt::MessageGUID& rhs)
{
    MessageGUIDLess less;
    return less(lhs, rhs);
}

inline bsl::ostream& bmqt::operator<<(bsl::ostream&            stream,
                                      const bmqt::MessageGUID& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
