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

// mwcst_stringkey.h -*-C++-*-
#ifndef INCLUDED_MWCST_STRINGKEY
#define INCLUDED_MWCST_STRINGKEY

//@PURPOSE: Provide a smart string key for associative containers.
//
//@CLASSES:
// mwcst::StringKey: a better string key for containers
//
//@DESCRIPTION:
// This class provides a mechanism, 'mwcst::StringKey', which is intended to be
// used as a more efficient string key in associative containers.  This is
// done by allowing the 'StringKey' to either hold a reference to an existing
// string, or to own its own copy of it.
//
// The semantics of this are unusual, so care should be taken when using it:
// - When a 'StringKey' is created using an existing string (using the
//   bslstl::StringRef or char *,int constructors), the 'StringKey' holds a
//   reference to the supplied string, and the supplied string must live for
//   as long as the 'StringKey', unless 'makeCopy()' is used.
// - The 'StringKey' provides a function, 'makeCopy()' which can be used to
//   force the 'StringKey' to create a copy of its referenced string.  If the
//   'StringKey' already owns its string, 'makeCopy()' does nothing.
// - When a 'StringKey' is copy constructed, it automatically creates a copy
//   of the string referenced by the original 'StringKey'.
//
/// USAGE
///-----
//  First, we need a container to work with:
//..
//  bsl::hash_map<StringKey, int, StringKeyHasher> myHash;
//  myHash["one"] = 1;
//  myHash["two"] = 2;
//  myHash["three"] = 3;
//..
// Now, we check to see if another value is in the container.  If it's not, we
// insert it
//..
//  StringKey key("four");      // No copy
//  if (myHash.find(key) == myHash.end()) { // Still no copy
//      myHash[key] = 4;            // Make a copy for the key of the container
//  }
//..
//

#ifndef INCLUDED_BSLMA_ALLOCATOR
#include <bslma_allocator.h>
#endif

#ifndef INCLUDED_BSLMA_USESBSLMAALLOCATOR
#include <bslma_usesbslmaallocator.h>
#endif

#ifndef INCLUDED_BSLMF_NESTEDTRAITDECLARATION
#include <bslmf_nestedtraitdeclaration.h>
#endif

#ifndef INCLUDED_BSL_ALGORITHM
#include <bsl_algorithm.h>
#endif

#ifndef INCLUDED_BSL_CSTRING
#include <bsl_cstring.h>
#endif

#ifndef INCLUDED_BSL_IOSFWD
#include <bsl_iosfwd.h>
#endif

#ifndef INCLUDED_BSL_STRING
#include <bsl_string.h>
#endif

namespace BloombergLP {
namespace mwcst {

// ===============
// class StringKey
// ===============

/// A string that can hold a reference or a copy to a string
class StringKey {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;  // Source of memory used for internal
                                      // structure allocations

    const char* d_string_p;  // The string we're referencing

    int d_length;  // The length of 'd_string_p'

    bool d_isOwned;  // Is 'd_string_p' owned by us?

    mutable size_t d_hash;  // Our hash

    // FRIENDS
    friend bool operator==(const StringKey&, const StringKey&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StringKey, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance of `StringKey` class initialized with a reference
    /// to the specified `string` without copying it.  If an optional
    /// parameter `basicAllocator` is not `0` it'll be used for allocating
    /// the object's internal structures.
    StringKey(const bslstl::StringRef& string,
              bslma::Allocator*        basicAllocator = 0);

    /// Create an instance of `StringKey` class initialized with a reference
    /// to the specified `string`, with the specified `length` without
    /// copying it.  If an optional parameter `basicAllocator` is not `0`
    /// it'll be used for allocating the object's internal structures.
    StringKey(const char*       string,
              int               length,
              bslma::Allocator* basicAllocator = 0);

    /// Create a `StringKey` with a copy of the string referenced by the
    /// specified `other` `StringKey`.    If an optional parameter
    /// `basicAllocator` is not `0` it'll be used for allocating the
    /// object's internal structures.
    StringKey(const StringKey& other, bslma::Allocator* basicAllocator = 0);

    /// Deallocate the object.
    ~StringKey();

    // MANIPULATORS

    /// Make a copy of the string referenced by the object and start
    /// referring to it instead.  Do nothing if we already have our own
    /// copy.
    void makeCopy();

    /// Set the length of the string currently being referenced by this
    /// `StringKey` to the specified 'newLength.  The behavior is undefined
    /// unless this `StringKey` does NOT own the currently referenced
    /// string.
    void setLength(int newLength);

    // ACCESSORS

    /// Return our referenced string
    const char* string() const;

    /// Return the length of our string
    int length() const;

    /// Return the hash of this `StringKey`
    size_t hashValue() const;

    /// Write the value of this object to the specified output `stream` in
    /// a human-readable format, and return a reference to `stream`.
    /// Optionally specify an initial indentation `level`, whose absolute
    /// value is incremented recursively for nested objects.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute
    /// value indicates the number of spaces per indentation level for this
    /// and all of its nested objects.  If `level` is negative, suppress
    /// indentation of the first line.  If `spacesPerLevel` is negative,
    /// format the entire output on one line, suppressing all but the
    /// initial indentation (as governed by `level`).  If `stream` is not
    /// valid on entry, this operation has no effect.  Note that this
    /// human-readable format is not fully specified, and can change
    /// without notice.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` is less than the specified `rhs`,
/// and `false` otherwise.  A `StringKey` object is less than another if the
/// string it references compares as less than the string referenced by the
/// other `StringKey`.
bool operator<(const StringKey& lhs, const StringKey& rhs);

/// Return `true` if the specified `lhs` and `rhs` objects have the same
/// value, and `false` otherwise.  Two `StringKey` objects have the same
/// value if they refer to strings of the same length and data.
bool operator==(const StringKey& lhs, const StringKey& rhs);

/// Return `true` if the specified `lhs` and `rhs` objects have the same
/// value, and `false` otherwise.  Two `StringKey` objects have the same
/// value if they refer to strings of either differing length, or differing
/// data.
bool operator!=(const StringKey& lhs, const StringKey& rhs);

/// Write the value of the specified `object` to the specified output
/// `stream`, and return a reference to `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const StringKey& object);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------
// class StringKey
// ---------------

// CREATORS
inline StringKey::StringKey(const bslstl::StringRef& string,
                            bslma::Allocator*        basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_string_p(string.data())
, d_length(static_cast<int>(string.length()))
, d_isOwned(false)
, d_hash(0)
{
}

inline StringKey::StringKey(const char*       string,
                            int               length,
                            bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_string_p(string)
, d_length(length)
, d_isOwned(false)
, d_hash(0)
{
}

inline StringKey::StringKey(const StringKey&  other,
                            bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_string_p(other.d_string_p)
, d_length(other.d_length)
, d_isOwned(false)
, d_hash(other.d_hash)
{
    makeCopy();
}

inline StringKey::~StringKey()
{
    if (d_isOwned) {
        d_allocator_p->deallocate(const_cast<char*>(d_string_p));
    }
}

// MANIPULATORS
inline void StringKey::setLength(int newLength)
{
    d_length = newLength;
    d_hash   = 0;
}

// ACCESSORS
inline const char* StringKey::string() const
{
    return d_string_p;
}

inline int StringKey::length() const
{
    return d_length;
}

inline size_t StringKey::hashValue() const
{
    if (d_hash) {
        return d_hash;
    }
    size_t      hash = 1;
    const char* str  = string();
    int         len  = length();

    while (len > 0) {
        switch (len) {
        default:
        case 16: hash = 5 * hash + str[15]; break;
        case 15: hash = 5 * hash + str[14]; break;
        case 14: hash = 5 * hash + str[13]; break;
        case 13: hash = 5 * hash + str[12]; break;
        case 12: hash = 5 * hash + str[11]; break;
        case 11: hash = 5 * hash + str[10]; break;
        case 10: hash = 5 * hash + str[9]; break;
        case 9: hash = 5 * hash + str[8]; break;
        case 8: hash = 5 * hash + str[7]; break;
        case 7: hash = 5 * hash + str[6]; break;
        case 6: hash = 5 * hash + str[5]; break;
        case 5: hash = 5 * hash + str[4]; break;
        case 4: hash = 5 * hash + str[3]; break;
        case 3: hash = 5 * hash + str[2]; break;
        case 2: hash = 5 * hash + str[1]; break;
        case 1: hash = 5 * hash + str[0]; break;
        }
        str += 16;
        len -= 16;
    }
    d_hash = hash;
    return d_hash;
}

}  // close package namespace

// FREE OPERATORS
inline bool mwcst::operator<(const mwcst::StringKey& lhs,
                             const mwcst::StringKey& rhs)
{
    int ret = bsl::memcmp(lhs.string(),
                          rhs.string(),
                          bsl::min(lhs.length(), rhs.length()));

    if (ret != 0) {
        return ret < 0;
    }
    else {
        return lhs.length() < rhs.length();
    }
}

inline bool mwcst::operator==(const mwcst::StringKey& lhs,
                              const mwcst::StringKey& rhs)
{
    if ((lhs.length() != rhs.length()) ||
        (lhs.hashValue() != rhs.hashValue())) {
        return false;
    }

    return bsl::memcmp(lhs.string(),
                       rhs.string(),
                       bsl::min(lhs.length(), rhs.length())) == 0;
}

inline bool mwcst::operator!=(const mwcst::StringKey& lhs,
                              const mwcst::StringKey& rhs)
{
    if ((lhs.length() != rhs.length()) ||
        (lhs.hashValue() != rhs.hashValue())) {
        return true;
    }

    return bsl::memcmp(lhs.string(),
                       rhs.string(),
                       bsl::min(lhs.length(), rhs.length())) != 0;
}

inline bsl::ostream& mwcst::operator<<(bsl::ostream&           stream,
                                       const mwcst::StringKey& object)
{
    return object.print(stream, 0, -1);
}

}  // close enterprise namespace

namespace bsl {

// =========================================
// struct hash<BloombergLP::mwcst::StringKey>
// =========================================

template <>
struct hash<BloombergLP::mwcst::StringKey> {
    // ACCESSORS
    size_t operator()(const BloombergLP::mwcst::StringKey& key) const;
};

// -----------------------------------------
// struct hash<BloombergLP::mwcst::StringKey>
// -----------------------------------------

inline size_t hash<BloombergLP::mwcst::StringKey>::operator()(
    const BloombergLP::mwcst::StringKey& key) const
{
    return key.hashValue();
}

}  // close namespace bsl

#endif
