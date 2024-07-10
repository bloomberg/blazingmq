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

// mwcst_value.h -*-C++-*-
#ifndef INCLUDED_MWCST_VALUE
#define INCLUDED_MWCST_VALUE

//@PURPOSE: A variant value able to represent multiple types
//
//@CLASSES:
//  mwcst::Value: a variant value
//
//@DESCRIPTION:
//  A variant Value that can hold any value of a number of types.  It is
//  hashable, comparable, and can be created to store a reference to a string,
//  or to make a copy of the string.  When copy constructed, if the original
//  class held a string, the new object always makes a copy.  This makes the
//  'Value' an excellent key in a normal or hash map since it doesn't require
//  a copy of the lookup argument if it's not a 'bsl::string', but will
//  correctly make a copy when the value is inserted into a container if
//  necessary.
//
//  Note that once a 'Value' owns its value, it acts like a true
//  value-semantic object until 'clear' or 'set' is called on it again.
//

#include <bdlb_variant.h>
#include <bdlt_datetimetz.h>
#include <bdlt_datetz.h>
#include <bdlt_timetz.h>
#include <bsl_string.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcst {

// ===========
// class Value
// ===========

/// A variant value that can be used as a key in containers
class Value {
  public:
    // PUBLIC TYPES
    typedef bdlb::Variant<bool,
                          int,
                          bsls::Types::Int64,
                          double,
                          bdlt::DateTz,
                          bdlt::TimeTz,
                          bdlt::DatetimeTz,
                          bslstl::StringRef>
        ValueType;

  private:
    // DATA
    ValueType         d_value;
    mutable size_t    d_hash;
    bool              d_owned;
    bslma::Allocator* d_allocator_p;

    // FRIENDS
    friend bool          operator==(const Value&, const Value&);
    friend bool          operator!=(const Value&, const Value&);
    friend bool          operator<(const Value&, const Value&);
    friend bsl::ostream& operator<<(bsl::ostream&, const Value&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Value, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a null value
    explicit Value(bslma::Allocator* basicAllocator = 0);

    /// Create a value with the specified `value`.
    template <typename TYPE>
    explicit Value(const TYPE& value, bslma::Allocator* basicAllocator = 0);

    /// Copy construct this value from the specified `other` value, making
    /// a copy of the value held by `other`.
    Value(const Value& other, bslma::Allocator* basicAllocator = 0);

    ~Value();

    // MANIPULATORS

    /// Assign to this `value` the value of the specified `rhs` `Value`,
    /// making a copy of the value of `rhs` held its own copy of the value,
    /// and copying its reference otherwise.
    Value& operator=(const Value& rhs);

    /// Make this value null
    void clear();

    /// Set this object to the specified `value`
    template <typename TYPE>
    void set(const TYPE& value);

    /// Force the `Value` to make a copy of its value if it's being held by
    /// a reference, otherwise do nothing.
    void ownValue();

    // ACCESSORS

    /// Return true if this value is null
    bool isNull() const;

    /// Return true if this value has the specified type
    template <typename TYPE>
    bool is() const;

    /// Return a non-modifiable reference to this value, if it's of the
    /// parameterized `TYPE`.  The behavior is undefined if the type of
    /// this value is not the parameterized `TYPE`
    template <typename TYPE>
    const TYPE& the() const;

    /// Apply the specified `visitor` to this value by passing the current
    /// value to the `visitor` object's `operator()`, and return the value
    /// returned by the `visitor` object's `operator()`.  If the value is
    /// null, a default constructed `bslmf::Nil` will be passed to the
    /// visitor.  The `visitor` must define a public typedef of
    /// `ResultType`
    template <class VISITOR>
    typename VISITOR::ResultType apply(const VISITOR& visitor) const;

    /// Return the hash of this `Value`.
    size_t hash() const;
};

// FREE OPERATORS
inline bool   operator==(const Value& lhs, const Value& rhs);
inline bool   operator!=(const Value& lhs, const Value& rhs);
inline bool   operator<(const Value& lhs, const Value& rhs);
bsl::ostream& operator<<(bsl::ostream& stream, const Value& value);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------
// class Value
// -----------

// CREATORS
inline Value::Value(bslma::Allocator* basicAllocator)
: d_value(basicAllocator)
, d_hash(0)
, d_owned(false)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

template <typename TYPE>
inline Value::Value(const TYPE& value, bslma::Allocator* basicAllocator)
: d_value(basicAllocator)
, d_hash(0)
, d_owned(false)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    set(value);
}

inline Value::Value(const Value& other, bslma::Allocator* basicAllocator)
: d_value(other.d_value, basicAllocator)
, d_hash(other.d_hash)
, d_owned(false)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    if (other.d_owned) {
        ownValue();
    }
}

inline Value::~Value()
{
    clear();
}

// MANIPULATORS
inline Value& Value::operator=(const Value& rhs)
{
    if (this != &rhs) {
        clear();
        d_value = rhs.d_value;
        d_hash  = rhs.d_hash;
        if (rhs.d_owned) {
            ownValue();
        }
    }

    return *this;
}

inline void Value::clear()
{
    if (d_owned) {
        d_allocator_p->deallocate(
            const_cast<char*>(d_value.the<bslstl::StringRef>().data()));
        d_owned = false;
    }

    d_value.reset();
    d_hash = 0;
}

template <typename TYPE>
inline void Value::set(const TYPE& value)
{
    clear();
    d_value.assign(value);
    d_hash = 0;
}

template <>
inline void Value::set<bsl::string>(const bsl::string& value)
{
    clear();
    d_value.assignTo<bslstl::StringRef>(value);
    d_hash = 0;
}

// ACCESSORS
inline bool Value::isNull() const
{
    return d_value.isUnset();
}

template <typename TYPE>
inline bool Value::is() const
{
    return d_value.is<TYPE>();
}

template <typename TYPE>
inline const TYPE& Value::the() const
{
    return d_value.the<TYPE>();
}

template <class VISITOR>
inline typename VISITOR::ResultType Value::apply(const VISITOR& visitor) const
{
    return d_value.apply(visitor);
}

}  // close package namespace

// FREE OPERATORS
inline bool mwcst::operator==(const mwcst::Value& lhs, const mwcst::Value& rhs)
{
    if (lhs.hash() != rhs.hash()) {
        return false;
    }

    return lhs.d_value == rhs.d_value;
}

inline bool mwcst::operator!=(const mwcst::Value& lhs, const mwcst::Value& rhs)
{
    if (lhs.hash() != rhs.hash()) {
        return true;
    }

    return lhs.d_value != rhs.d_value;
}

}  // close enterprise namespace

// ==================
// struct hash<Value>
// ==================

namespace bsl {

template <>
struct hash<BloombergLP::mwcst::Value> {
    // ACCESSORS
    size_t operator()(const BloombergLP::mwcst::Value& value) const;
};

// ACCESSORS
inline size_t hash<BloombergLP::mwcst::Value>::operator()(
    const BloombergLP::mwcst::Value& value) const
{
    return value.hash();
}

}  // close namespace bsl

#endif
