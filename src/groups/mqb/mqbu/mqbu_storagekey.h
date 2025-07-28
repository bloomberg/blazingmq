// Copyright 2016-2023 Bloomberg Finance L.P.
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

// mqbu_storagekey.h                                                  -*-C++-*-
#ifndef INCLUDED_MQBU_STORAGEKEY
#define INCLUDED_MQBU_STORAGEKEY

//@PURPOSE: Provide a VST which represents a fixed-length key in storage.
//
//@CLASSES:
//  mqbu::StorageKey: VST representing fixed-length key in storage.
//
//@DESCRIPTION: 'mqbu::StorageKey' provides a VST which can be used to
// represent a fixed-length key in the storage.  This key can be stored on disk
// as well for fast retrieval from in-memory associative containers.

// BDE
#include <bdlb_print.h>
#include <bsl_cstddef.h>
#include <bsl_cstring.h>  // for bsl::memcpy, etc
#include <bsl_ostream.h>

#include <bsl_type_traits.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslh_hash.h>
#include <bslmf_isbitwiseequalitycomparable.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_platform.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbu {

// ================
// class StorageKey
// ================

class StorageKey {
  private:
    // FRIENDS
    friend bool operator==(const StorageKey& lhs, const StorageKey& rhs);
    friend bool operator!=(const StorageKey& lhs, const StorageKey& rhs);
    friend struct StorageKeyLess;
    template <class HASH_ALGORITHM>
    friend void hashAppend(HASH_ALGORITHM& hashAlgo, const StorageKey& key);

  public:
    // TYPES
    enum Enum {
        // Enum representing the size of a buffer needed to represent a
        // StorageKey.

        e_KEY_LENGTH_BINARY = 5  // Number of bytes in a binary representation

        ,
        e_KEY_LENGTH_HEX = 2 * e_KEY_LENGTH_BINARY
        // Number of bytes in a hex representation
    };

    // PUBLIC CONSTANTS

    /// This public constant represents an empty storage key.
    static const StorageKey k_NULL_KEY;

  private:
    // PRIVATE CONSTANTS

  private:
    // DATA
    char d_buffer[e_KEY_LENGTH_BINARY];

  public:
    // TYPES

    /// Marker used to disambiguate constructors.
    typedef struct {
    } BinaryRepresentation;

    /// Marker used to disambiguate constructors.
    typedef struct {
    } HexRepresentation;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StorageKey,
                                   bslmf::IsBitwiseEqualityComparable)
    BSLMF_NESTED_TRAIT_DECLARATION(StorageKey, bsl::is_trivially_copyable)

    // CREATORS

    /// Default constructor initialized with a null key. Note that
    /// `isNull()` will return true.
    StorageKey();

    /// Constructor that takes a binary representation of the storage key.
    StorageKey(const BinaryRepresentation&, const void* data);

    /// Constructor that takes a hex representation of the storage key.
    StorageKey(const HexRepresentation&, const char* data);

    /// Constructor that uses 4 bytes from the specified `value` integer and
    /// a constant 5th byte to create an instance.  Note that an instance of
    /// StorageKey created in such way *must* *not* be persisted on disk or
    /// used out-of-process in any way, and this flavor is provided purely
    /// as a convenience.
    explicit StorageKey(unsigned int value);

    // MANIPULATORS

    /// Initialize this StorageKey with hex representation from the
    /// specified `value`.  Behavior is undefined unless `value` is non-null
    /// and of length e_KEY_LENGTH_HEX.
    void fromHex(const char* value);

    /// Initialize this StorageKey with the binary representation from the
    /// specified value'.  Behavior is undefined unless `value` is non-null
    /// and of length e_KEY_LENGTH_BINARY.
    void fromBinary(const void* data);

    /// Reset this instance.  Note that `isNull()` will return true after
    /// this.
    void reset();

    // ACCESSORS

    /// Return `true` if this instance is null.  Note that this method will
    /// return true for a default constructed instance.
    bool isNull() const;

    /// Return the binary representation of this instance.
    const char* data() const;

    /// Write the hex representation of this object to the specified `out`.
    /// Behavior is undefined unless `out` is non-null and of length
    /// e_KEY_LENGTH_HEX.
    void loadHex(char* out) const;

    /// Write the binary representation of this object to the specified
    /// `out`.
    void loadBinary(bsl::vector<char>* out) const;

    /// Format a hex representation of this object to the specified `os` and
    /// return a reference to `os`.  Optionally specify an initial
    /// indentation `level`, whose absolute value is incremented recursively
    /// for nested objects. If `level` is specified, optionally specify
    /// `spacesPerLevel`, whose absolute value indicates the number of
    /// spaces per indentation level for this and all of its nested objects.
    /// If `level` is negative, suppress indentation of the first line. If
    /// `spacesPerLevel` is negative, format the entire output on one line,
    /// suppressing all but the initial indentation (as governed by
    /// `level`).
    bsl::ostream&
    print(bsl::ostream& os, int level = 0, int spacesPerLevel = 4) const;
};

// ----------------
// class StorageKey
// ----------------

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const StorageKey& rhs);

/// Return `true` if `rhs` object contains the value of the same type as
/// contained in the `lhs` object and the value itself is the same in both
/// objects, return `false` otherwise.
bool operator==(const StorageKey& lhs, const StorageKey& rhs);

/// Return `false` if `rhs` object contains the value of the same type as
/// contained in the `lhs` object and the value itself is the same in both
/// objects, return `true` otherwise.
bool operator!=(const StorageKey& lhs, const StorageKey& rhs);

/// Return `true` if the specified `lhs` instance is smaller than the
/// specified `rhs` instance, false otherwise.  Note that this operator is
/// provided so that StorageKey can be used as a key in associative
/// containers (map, set, etc.)
bool operator<(const StorageKey& lhs, const StorageKey& rhs);

// ========================
// struct StorageKeyHexUtil
// ========================

struct StorageKeyHexUtil {
  private:
    // PRIVATE CONSTANTS

    /// Conversion tables
    static const char k_INT_TO_HEX_TABLE[];
    static const char k_HEX_TO_INT_TABLE[];

  public:
    // PUBLIC CLASS METHODS

    /// @brief Convert a data from hex to binary representation.
    /// @param bin The destination buffer (binary representation).
    /// @param binLength The size of destination buffer.
    /// @param hex The source buffer.  The behaviour is undefined unless the
    ///            length of `hex` buffer is twice the `length`.
    static void hexToBinary(char* bin, size_t binLength, const char* hex);

    /// @brief Convert a data from binary to hex representation.
    /// @param hex The destination buffer (hex representation).  The behaviour
    ///            is undefined unless the length of `hex` buffer is twice the
    ///            `length`.
    /// @param bin The source buffer.
    /// @param binLength The size of source buffer.
    static void binaryToHex(char* hex, const char* bin, size_t binLength);
};

// =====================
// struct StorageKeyLess
// =====================

struct StorageKeyLess {
    // ACCESSORS

    /// Return `true` if the specified `lhs` should be considered as having
    /// a value less than the specified `rhs`.
    bool operator()(const StorageKey& lhs, const StorageKey& rhs) const;
};

// ========================
// class StorageKeyHashAlgo
// ========================

/// This class provides a hashing algorithm for `mqbu::StorageKey`.  At the
/// time of writing, this algorithm was found to be approximately X than the
/// default hashing algorithm which comes with `bslh` package.
/// Performance-critical applications may want to use this hashing algorithm
/// instead of the default one.
class StorageKeyHashAlgo {
  private:
    // DATA
    bsls::Types::Uint64 d_result;

  public:
    // TYPES
    typedef bsls::Types::Uint64 result_type;

    // CREATORS

    /// Default constructor.
    StorageKeyHashAlgo();

    // MANIPULATORS
    void operator()(const void* data, size_t numBytes);

    /// Compute and return the hash for the StorageKey.
    result_type computeHash();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class StorageKey
// ----------------

// CREATORS
inline StorageKey::StorageKey()
{
    bsl::memset(d_buffer, '\0', StorageKey::e_KEY_LENGTH_BINARY);
}

inline StorageKey::StorageKey(const BinaryRepresentation&, const void* data)
{
    bsl::memcpy(d_buffer, data, StorageKey::e_KEY_LENGTH_BINARY);
}

inline StorageKey::StorageKey(const HexRepresentation&, const char* data)
{
    StorageKey::fromHex(data);
}

inline StorageKey::StorageKey(unsigned int value)
{
    // We initialize 'd_buffer' from 'value' in such a way that when printed,
    // same sequence is printed for a given 'value' irrespective of platform's
    // endianness.  This is done so as to make printed sequence consistent and
    // debuggable in the logs.

#if defined(BSLS_PLATFORM_IS_BIG_ENDIAN)
    bsl::memcpy(&(d_buffer[1]),
                reinterpret_cast<char*>(&value),
                sizeof(value));
#else
    const unsigned char* buffer = reinterpret_cast<const unsigned char*>(
        &value);
    d_buffer[1] = buffer[3];
    d_buffer[2] = buffer[2];
    d_buffer[3] = buffer[1];
    d_buffer[4] = buffer[0];
#endif

    // Populate 1st byte with a fixed value so that comparison of two instances
    // created with same 'value' works correctly.  Note that we cannot choose
    // '\0' otherwise for an instance initialized with a 'value' of 0, 'isNull'
    // will return true.

    d_buffer[0] = '0';
}

// MANIPULATORS
inline void StorageKey::fromHex(const char* value)
{
    StorageKeyHexUtil::hexToBinary(d_buffer,
                                   StorageKey::e_KEY_LENGTH_BINARY,
                                   value);
}

inline void StorageKey::fromBinary(const void* data)
{
    bsl::memcpy(d_buffer, data, StorageKey::e_KEY_LENGTH_BINARY);
}

inline void StorageKey::reset()
{
    *this = k_NULL_KEY;
}

// ACCESSORS
inline bool StorageKey::isNull() const
{
    return *this == k_NULL_KEY;
}

inline const char* StorageKey::data() const
{
    return d_buffer;
}

inline void StorageKey::loadHex(char* out) const
{
    StorageKeyHexUtil::binaryToHex(out,
                                   d_buffer,
                                   StorageKey::e_KEY_LENGTH_BINARY);
}

inline void StorageKey::loadBinary(bsl::vector<char>* out) const
{
    out->assign(d_buffer, d_buffer + StorageKey::e_KEY_LENGTH_BINARY);
}

inline bsl::ostream&
StorageKey::print(bsl::ostream& os, int level, int spacesPerLevel) const
{
    bdlb::Print::indent(os, level, spacesPerLevel);
    bdlb::Print::singleLineHexDump(os,
                                   d_buffer,
                                   StorageKey::e_KEY_LENGTH_BINARY);
    return os;
}

// FREE FUNCTIONS
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const StorageKey& key)
{
    using bslh::hashAppend;              // for ADL
    hashAppend(hashAlgo, key.d_buffer);  // hashes entire buffer array
}

inline bsl::ostream& operator<<(bsl::ostream&           stream,
                                const mqbu::StorageKey& rhs)
{
    return rhs.print(stream, 0, -1);
}

// --------------------
// class StorageKeyLess
// --------------------

inline bool StorageKeyLess::operator()(const StorageKey& lhs,
                                       const StorageKey& rhs) const
{
    return 0 > bsl::memcmp(lhs.d_buffer,
                           rhs.d_buffer,
                           StorageKey::e_KEY_LENGTH_BINARY);
}

// ------------------------
// class StorageKeyHashAlgo
// ------------------------

// CREATORS
inline StorageKeyHashAlgo::StorageKeyHashAlgo()
: d_result(0)
{
}

// MANIPULATORS
inline void StorageKeyHashAlgo::operator()(const void*        data,
                                           BSLA_UNUSED size_t numBytes)
{
    // 10K Keys
    // --------
    // o With 'bmqc::OrderedHashMap': Average of 17M insertions per second
    //                                (compared to average of 12M insertions
    //                                per second using the default hash)

    d_result = 5381ULL;

    const unsigned char* start = reinterpret_cast<const unsigned char*>(data);
    d_result += (start[0]) + (start[1] << 8) + (start[2] << 16) +
                (start[3] << 24);
}

inline StorageKeyHashAlgo::result_type StorageKeyHashAlgo::computeHash()
{
    return d_result;
}

}  // close package namespace

// ----------------
// class StorageKey
// ----------------

// FREE OPERATORS
inline bool mqbu::operator==(const mqbu::StorageKey& lhs,
                             const mqbu::StorageKey& rhs)
{
    return 0 == bsl::memcmp(lhs.d_buffer,
                            rhs.d_buffer,
                            mqbu::StorageKey::e_KEY_LENGTH_BINARY);
}

inline bool mqbu::operator!=(const mqbu::StorageKey& lhs,
                             const mqbu::StorageKey& rhs)
{
    return !(lhs == rhs);
}

inline bool mqbu::operator<(const mqbu::StorageKey& lhs,
                            const mqbu::StorageKey& rhs)
{
    StorageKeyLess less;
    return less(lhs, rhs);
}

}  // close enterprise namespace

#endif
