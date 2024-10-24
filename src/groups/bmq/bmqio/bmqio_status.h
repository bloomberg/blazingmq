// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqio_status.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQIO_STATUS
#define INCLUDED_BMQIO_STATUS

//@PURPOSE: Provide an object representing the result of an I/O operation.
//
//@CLASSES:
// bmqio::Status
//
//@DESCRIPTION: This component defines a value semantic type, 'bmqio::Status',
// which represents the result of a read/write operation or a connection
// attempt.  Its salient attributes are a status category defining a broad
// category of error (enum), an error code (integer), a source id indicating
// in what component the error happened (integer), and an optional error
// message.

#include <bmqvt_propertybag.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bsl_iosfwd.h>
#include <bsl_string.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace bmqio {

// =====================
// struct StatusCategory
// =====================

/// This enum represents the various category of errors.
struct StatusCategory {
    // TYPES
    enum Enum {
        e_SUCCESS       = 0,
        e_GENERIC_ERROR = 1,
        e_CONNECTION    = 2,
        e_TIMEOUT       = 3,
        e_CANCELED      = 4,
        e_LIMIT         = 5
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `StatusCategory::Type` value.
    static bsl::ostream& print(bsl::ostream&        stream,
                               StatusCategory::Enum value,
                               int                  level          = 0,
                               int                  spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(StatusCategory::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, StatusCategory::Enum value);

// ============
// class Status
// ============

/// Connection or I/O operation result status.
class Status {
  private:
    // DATA
    StatusCategory::Enum d_category;
    // Broad category of the error.

    mutable bdlb::NullableValue<bmqvt::PropertyBag> d_properties;
    // Optional associated
    // properties

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Status, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `Status` indicating a success.
    explicit Status(bslma::Allocator* basicAllocator = 0);

    /// Create a `Status` with the specified `category`.
    Status(StatusCategory::Enum category,
           bslma::Allocator*    basicAllocator = 0);

    /// Create a `Status` with the specified `category` and one integer
    /// property with the specified `propName` and `propValue`.
    Status(StatusCategory::Enum     category,
           const bslstl::StringRef& propName,
           int                      propValue,
           bslma::Allocator*        basicAllocator = 0);

    /// Copy construct a `Status` from the specified `other`.  Optionally
    /// specify a `basicAllocator` used to supply memory.
    Status(const Status& other, bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS
    Status& operator=(const Status& rhs);

    /// Reset all members of this `Status`.  `status.reset(...)` is
    /// equivalent to `status = Status(...)` and return a reference offering
    /// modifiable access to this object.
    Status& reset();
    Status& reset(StatusCategory::Enum category);
    Status& reset(StatusCategory::Enum     category,
                  const bslstl::StringRef& propName,
                  int                      propValue);
    Status& reset(StatusCategory::Enum     category,
                  const bslstl::StringRef& propName,
                  const bslstl::StringRef& propValue);

    /// Set the status category to the specified `value` and return a
    /// reference offering modifiable access to this object.
    Status& setCategory(StatusCategory::Enum value);

    /// Return a reference providing modifiable access to the properties of
    /// this object.
    bmqvt::PropertyBag& properties();

    // ACCESSORS

    /// Return `true` if this `Status` represents a success result, (i.e.
    /// `category() == Status::BMQIO_SUCCESS`) and `false` otherwise.
    operator bool() const;

    /// Return the status category.
    StatusCategory::Enum category() const;

    /// Return a reference providing const access to the properties of this
    /// object.
    const bmqvt::PropertyBag& properties() const;

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
bsl::ostream& operator<<(bsl::ostream& stream, const Status& value);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------
// class Status
// ------------

// CREATORS
inline Status::Status(bslma::Allocator* basicAllocator)
: d_category(StatusCategory::e_SUCCESS)
, d_properties(basicAllocator)
{
}

inline Status::Status(StatusCategory::Enum category,
                      bslma::Allocator*    basicAllocator)
: d_category(category)
, d_properties(basicAllocator)
{
}

inline Status::Status(StatusCategory::Enum     category,
                      const bslstl::StringRef& propName,
                      int                      propValue,
                      bslma::Allocator*        basicAllocator)
: d_category(category)
, d_properties(basicAllocator)
{
    d_properties.makeValue().set(propName, propValue);
}

inline Status::Status(const Status& other, bslma::Allocator* basicAllocator)
: d_category(other.d_category)
, d_properties(other.d_properties, basicAllocator)
{
}

// MANIPULATORS
inline Status& Status::reset()
{
    d_category = StatusCategory::e_SUCCESS;
    d_properties.reset();
    return *this;
}

inline Status& Status::reset(StatusCategory::Enum category)
{
    d_category = category;
    d_properties.reset();
    return *this;
}

inline Status& Status::reset(StatusCategory::Enum     category,
                             const bslstl::StringRef& propName,
                             int                      propValue)
{
    d_category = category;
    d_properties.reset();
    d_properties.makeValue().set(propName, propValue);
    return *this;
}

inline Status& Status::reset(StatusCategory::Enum     category,
                             const bslstl::StringRef& propName,
                             const bslstl::StringRef& propValue)
{
    d_category = category;
    d_properties.reset();
    d_properties.makeValue().set(propName, propValue);
    return *this;
}

inline Status& Status::setCategory(StatusCategory::Enum value)
{
    d_category = value;
    return *this;
}

inline bmqvt::PropertyBag& Status::properties()
{
    if (d_properties.isNull()) {
        d_properties.makeValue();
    }

    return d_properties.value();
}

// ACCESSORS
inline Status::operator bool() const
{
    return d_category == StatusCategory::e_SUCCESS;
}

inline StatusCategory::Enum Status::category() const
{
    return d_category;
}

inline const bmqvt::PropertyBag& Status::properties() const
{
    if (d_properties.isNull()) {
        d_properties.makeValue();
    }

    return d_properties.value();
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& bmqio::operator<<(bsl::ostream&               stream,
                                       bmqio::StatusCategory::Enum value)
{
    return StatusCategory::print(stream, value, 0, -1);
}

inline bsl::ostream& bmqio::operator<<(bsl::ostream&        stream,
                                       const bmqio::Status& value)
{
    return value.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
