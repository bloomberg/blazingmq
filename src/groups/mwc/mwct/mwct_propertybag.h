// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mwct_propertybag.h                                                 -*-C++-*-
#ifndef INCLUDED_MWCT_PROPERTYBAG
#define INCLUDED_MWCT_PROPERTYBAG

//@PURPOSE: Provide a bag of named properties.
//
//@CLASSES:
// mwct::PropertyBag:      bag of named properties
// mwct::PropertyBagValue: variant type representing a property
//
//@DESCRIPTION: This component defines a mechanism, 'mwct::PropertyBag', for a
// collection of named properties of type 'mwct::PropertyBagValue'.  The
// properties can be updated and accessed in a thread-safe way.  A
// 'PropertyBagValue' primarily holds two kind of values: either a generic
// shared_ptr to arbitrary data, or a datum of any type.  Because 'int' and
// 'string' are common types, 'PropertyBag' exposes convenient utilities to
// store and retrieve properties of those type, taking care of the to and from
// datum convertions under the hood.
//
/// Usage example
///-------------
// First, let's create a PropertyBag object, and set a few properties on it:
//..
//  mwct::PropertyBag bag(allocator);
//  bag.set("myIntValue",    5)
//     .set("myStringValue", "hello world");
//..
// Then, we can retrieve the property from it:
// ..
//   int value;
//   bag.load(&value, "myIntValue");
//   ASSERT(value == 5);
//..

// MWC

// BDE
#include <bdlb_variant.h>
#include <bdld_datum.h>
#include <bdld_manageddatum.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
#include <bsls_spinlock.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwct {

// ======================
// class PropertyBagValue
// ======================

/// Type (variant-like) representing an item in a `PropertyBag`, composed of
/// a name (its key in the PropertyBag) and associated value.
class PropertyBagValue {
  private:
    // PRIVATE TYPES
    typedef bdlb::Variant2<bdld::ManagedDatum, bsl::shared_ptr<void> > Value;

    // DATA
    bsl::string d_name;  // our key string in the PropertyBag
    Value       d_value;

    // PRIVATE CREATORS

    /// Create a `PropertyBagValue` object with the specified `name` and
    /// holding the specified `datum` allocated with the specified
    /// `datumAllocator`.  Use the specified `allocator` for memory needs.
    PropertyBagValue(const bslstl::StringRef& name,
                     const bdld::Datum&       datum,
                     bslma::Allocator*        datumAllocator,
                     bslma::Allocator*        allocator = 0);

    /// Create a `PropertyBagValue` object with the specified `name` and
    /// holding the specified `sptr` value.  Use the specified `allocator`
    /// for memory needs.
    PropertyBagValue(const bslstl::StringRef&     name,
                     const bsl::shared_ptr<void>& sptr,
                     bslma::Allocator*            allocator = 0);

    /// Create a `PropertyBagValue` object that holds the same value as the
    /// specified `original` value.  Note that if the `original` value is a
    /// Datum, it will be cloned using the specified `allocator`.
    PropertyBagValue(const PropertyBagValue& original,
                     bslma::Allocator*       allocator = 0);

    // NOT IMPLEMENTED
    PropertyBagValue& operator=(const PropertyBagValue&) BSLS_KEYWORD_DELETED;

    // FRIENDS
    friend class PropertyBag;
    friend class bslma::SharedPtrInplaceRep<PropertyBagValue>;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PropertyBagValue, bslma::UsesBslmaAllocator)

    // ACCESSORS

    /// Return the name of this property.
    const bsl::string& name() const;

    /// Return `true` if this object is holding a datum and `false`
    /// otherwise.
    bool isDatum() const;

    /// Return `true` if this object is holding a shared_ptr and `false`
    /// otherwise.
    bool isPtr() const;

    /// Return a reference providing const access to the datum stored in
    /// this object.  The behavior is undefined unless `isDatum()` returns
    /// `true` on this object.
    const bdld::Datum& theDatum() const;

    /// Return a reference providing const access to the shared_ptr stored
    /// in this object.  The behavior is undefined unless `isPtr`()'
    /// returns `true` on this object.
    const bsl::shared_ptr<void>& thePtr() const;

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
bsl::ostream& operator<<(bsl::ostream& stream, const PropertyBagValue& value);

// =================
// class PropertyBag
// =================

/// Collection of named properties with thread-safe accessors.
class PropertyBag {
  public:
    // TYPES
    typedef PropertyBagValue Value;

  private:
    // PRIVATE TYPES
    typedef bsl::shared_ptr<Value> ValueSPtr;

    /// Note that the key is a string ref to the name held inside the value.
    typedef bsl::unordered_map<bslstl::StringRef, ValueSPtr> ValueMap;

    // DATA
    ValueMap               d_values;
    mutable bsls::SpinLock d_lock;

  private:
    /// Safely insert the specified `value` into the map.  Because the map's
    /// key is a string ref to the name in the value, when inserting a new
    /// entry for an already existing value, some containers may keep the
    /// previous key, which would then be pointing to garbage; therefore
    /// always perform an erase prior to inserting a value.  Note that the
    /// `d_lock` *MUST* be acquired prior to calling this method.
    void insertValueImp(const ValueSPtr& value);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PropertyBag, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new empty `PropertyBag` object using the specified
    /// `allocator` for memory allocations.
    explicit PropertyBag(bslma::Allocator* allocator = 0);

    /// Create a new `PropertyBag` object having the same values as the
    /// specified `original`.  Use the specified `allocator` for memory
    /// allocations.
    PropertyBag(const PropertyBag& original, bslma::Allocator* allocator = 0);

    // MANIPULATORS

    /// Assign the values of the specified `rhs` to this object.
    PropertyBag& operator=(const PropertyBag& rhs);

    /// Import the specified `value` into this object, copying it and
    /// storing it under its `name()`.  Return a reference providing
    /// modifiable access to this object.
    PropertyBag& import(const Value& value);

    /// Store copies the specified `values` in this object, storing them
    /// under their respective `name`s.  Return a reference providing
    /// modifiable access to this object.
    PropertyBag& import(const bsl::vector<bslma::ManagedPtr<Value> >& values);

    /// Set the value stored under the specified `key` (or `value.name()`)
    /// to a copy of the specified `value`.  Return a reference providing
    /// modifiable access to this object.
    PropertyBag& set(const bslstl::StringRef& key, const Value& value);

    /// Set the value stored under the specified `key` to the specified
    /// `datum`, which was created with the specified `datumAllocator`.  The
    /// behavior is undefined unless `datum` was created with the specified
    /// `datumAllocator`.  This function will `adopt` the `datum` and take
    /// responsibility for destroying it when necessary.  Return a reference
    /// offering modifiable access to this object.
    PropertyBag& set(const bslstl::StringRef& key,
                     const bdld::Datum&       datum,
                     bslma::Allocator*        datumAllocator = 0);

    /// Set the value stored under the specified `key` to the specified
    /// `ptr`.  Return a reference offering modifiable access to this
    /// object.
    PropertyBag& set(const bslstl::StringRef&     key,
                     const bsl::shared_ptr<void>& ptr);

    /// Sugar for storing an integer Datum with the specified `value` under
    /// the specified `key`.  Return a reference offering modifiable access
    /// to this object.
    PropertyBag& set(const bslstl::StringRef& key, int value);
    PropertyBag& set(const bslstl::StringRef& key, bsls::Types::Int64 value);

    /// Sugar for storing a string Datum with the specified `value` under
    /// the specified `key`.  Note that a full copy of the string will be
    /// made for ownership.  Return a reference offering modifiable access
    /// to this object.
    PropertyBag& set(const bslstl::StringRef& key,
                     const bslstl::StringRef& value);

    /// Remove the value stored under the specified `key`.  Return a
    /// reference offering modifiable access to this object.
    PropertyBag& unset(const bslstl::StringRef& key);

    // ACCESSORS

    /// Load all values currently stored in this object into the specified
    /// `dest`.
    void loadAll(bsl::vector<bslma::ManagedPtr<Value> >* dest) const;

    /// Load into the specified `dest` a ManagedPtr referring to the value
    /// stored under the specified `key`.  Return `true` on success or
    /// `false` if this object doesn't have anything stored under the
    /// `key`, with no effect on `dest`.
    bool load(bslma::ManagedPtr<Value>* dest,
              const bslstl::StringRef&  key) const;

    /// Sugar for loading an integer Datum stored under the specified
    /// `key`.  Return `true` if the `key` was found with the appropriate
    /// type and `false` otherwise.
    bool load(int* dest, const bslstl::StringRef& key) const;
    bool load(bsls::Types::Int64* dest, const bslstl::StringRef& key) const;

    /// Sugar for loading a string Datum stored under the specified `key`.
    /// Return `true` if the `key` was found with the appropriate type and
    /// `false` otherwise.
    bool load(bslstl::StringRef* dest, const bslstl::StringRef& key) const;

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

    /// Return the allocator this object was created with.
    bslma::Allocator* allocator() const;
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, const PropertyBag& value);

// ======================
// struct PropertyBagUtil
// ======================

/// Utility functions to aid in working with PropertyBag objects.
struct PropertyBagUtil {
    // CLASS METHODS

    /// `set` all properties from the specified `src` into the specified
    /// `dest`, inserting new ones or updating existing ones.
    static void update(PropertyBag* dest, const PropertyBag& src);
};

// ============================================================================
//              INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
// ============================================================================

// ----------------------
// class PropertyBagValue
// ----------------------

// ACCESSORS
inline const bsl::string& PropertyBagValue::name() const
{
    return d_name;
}

inline bool PropertyBagValue::isDatum() const
{
    return d_value.is<bdld::ManagedDatum>();
}

inline bool PropertyBagValue::isPtr() const
{
    return d_value.is<bsl::shared_ptr<void> >();
}

inline const bdld::Datum& PropertyBagValue::theDatum() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isDatum());

    return *d_value.the<bdld::ManagedDatum>();
}

inline const bsl::shared_ptr<void>& PropertyBagValue::thePtr() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isPtr());

    return d_value.the<bsl::shared_ptr<void> >();
}

// -----------------
// class PropertyBag
// -----------------

inline bslma::Allocator* PropertyBag::allocator() const
{
    return d_values.get_allocator().mechanism();
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mwct::operator<<(bsl::ostream&                 stream,
                                      const mwct::PropertyBagValue& value)
{
    return value.print(stream, 0, -1);
}

inline bsl::ostream& mwct::operator<<(bsl::ostream&            stream,
                                      const mwct::PropertyBag& value)
{
    return value.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
