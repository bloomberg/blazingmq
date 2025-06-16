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

/// @file bmqt_messageguid.h
///
/// @brief Provide a value-semantic global unique identifier for BlazingMQ
/// messages.
///
/// @bbref{bmqt::MessageGUID} provides a value-semantic global unique
/// identifier for BlazingMQ messages.  Each @bbref{bmqa::Message} delivered to
/// BlazingMQ client from BlazingMQ broker contains a unique
/// @bbref{bmqt::MessageGUID}.  The binary functor
/// @bbref{bmqt::MessageGUIDLess} can be used for comparing GUIDs, and an
/// optimized custom hash function is provided with
/// @bbref{bmqt::MessageGUIDHashAlgo}.
///
///
/// Externalization                         {#bmqt_messageguid_externalization}
/// ===============
///
/// For convenience, this class provides `toHex` method that can be used to
/// externalize a @bbref{bmqt::MessageGUID} instance.  Applications can persist
/// the resultant buffer (on filesystem, in database) to keep track of last
/// processed message ID across task instantiations.  `fromHex` method can be
/// used to convert a valid externalized buffer back to a message ID.
///
/// Efficient comparison and hash function        {#bmqt_messageguid_efficient}
/// ======================================
///
/// This component also provides efficient comparison and hash functions for
/// convenience, and thus, applications can use this component as a key in
/// associative containers.
///
/// Usage                                             {#bmqt_messageguid_usage}
/// =====
///
/// This section illustrates intended use of this component.
///
/// Example 1: Externalizing                            {#bmqt_messageguid_ex1}
/// ------------------------
///
/// ```
/// // Below, 'msg' is a valid instance of 'bmqa::Message' obtained from an
/// // instance of 'bmqa::Session':
///
/// bmqt::MessageGUID g1 = msg.messageId();
///
/// char buffer[bmqt::MessageGUID::e_SIZE_HEX];
/// g1.toHex(buffer);
///
/// BSLS_ASSERT(true == bmqt::MessageGUID::isValidHexRepresentation(buffer));
///
/// bmqt::MessageGUID g2;
/// g2.fromHex(buffer);
///
/// BSLS_ASSERT(g1 == g2);
/// ```

// BDE
#include <bsl_cstring.h>  // for bsl::memset, bsl::memcmp

#include <bsl_iosfwd.h>
#include <bsla_annotations.h>
#include <bslh_hash.h>
#include <bslmf_isbitwiseequalitycomparable.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
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

    /// Enum representing the size of a buffer needed to represent a GUID
    enum Enum {
        /// Binary format of the GUID
        e_SIZE_BINARY = 16

        ,
        /// Hexadecimal string representation of the GUID
        e_SIZE_HEX = 2 * e_SIZE_BINARY
    };

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MessageGUID,
                                   bslmf::IsBitwiseEqualityComparable)
    BSLMF_NESTED_TRAIT_DECLARATION(MessageGUID, bsl::is_trivially_copyable)

  private:
    // PRIVATE CONSTANTS

    /// Constant representing an unset GUID
    static const char k_UNSET_GUID[e_SIZE_BINARY];

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
inline void MessageGUIDHashAlgo::operator()(const void*        data,
                                            BSLA_UNUSED size_t numBytes)
{
    // Implementation note: the implementation is based on Jon Maiga's research
    // on different bit mixers and their qualities (look for `mxm`):
    // https://jonkagstrom.com/bit-mixer-construction/index.html

    // Typically, bit mixers are used as the last step of computing more
    // general hashes.  But it's more than enough to use it on its own for
    // our specific use case here.

    // Performance evaluation, hash quality and avalanche effect are here:
    // https://github.com/bloomberg/blazingmq/pull/348

    struct LocalFuncs {
        /// Return the "mxm" bit mix on the specified `x`.
        inline static bsls::Types::Uint64 mix(bsls::Types::Uint64 x)
        {
            x *= 0xbf58476d1ce4e5b9ULL;
            x ^= x >> 56;
            x *= 0x94d049bb133111ebULL;
            return x;
        }

        /// Return the hash combination of the specified `lhs` and `rhs`.
        inline static bsls::Types::Uint64 combine(bsls::Types::Uint64 lhs,
                                                  bsls::Types::Uint64 rhs)
        {
            lhs ^= rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);
            return lhs;
        }
    };

    // `data` buffer might not be aligned to 8 bytes, so recasting the pointer
    // might lead to UB
    bsls::Types::Uint64 parts[2];
    bsl::memcpy(parts, data, bmqt::MessageGUID::e_SIZE_BINARY);

    parts[0] = LocalFuncs::mix(parts[0]);
    parts[1] = LocalFuncs::mix(parts[1]);
    d_result = LocalFuncs::combine(parts[0], parts[1]);
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
