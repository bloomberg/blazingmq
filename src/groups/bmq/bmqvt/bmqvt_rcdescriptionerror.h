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

// bmqvt_rcdescriptionerror.h -*-C++-*-
#ifndef INCLUDED_BMQT_RCDESCRIPTIONERROR
#define INCLUDED_BMQT_RCDESCRIPTIONERROR

//@PURPOSE: Provide a value-semantic class for a typical error.
//
//@CLASSES:
//  bmqvt::RcDescriptionError: An error type with an 'int' and a 'bsl::string'
//
//@DESCRIPTION: The 'bmqvt::RcDescriptionError' provides a type that represents
// a typical error.  It contains the return code (rc) and an error message
// (description).
//

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace bmqvt {

// ========================
// class RcDescriptionError
// ========================

/// An error type with an `int` and a `bsl::string`.
class RcDescriptionError {
  private:
    // DATA
    int d_rc;
    // The return code associated to object.

    bsl::string d_description;
    // The description associated to this object.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RcDescriptionError,
                                   bslma::UsesBslmaAllocator)

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
    /// the initial indentation (as governed by `level`).
    static bsl::ostream& print(bsl::ostream&             stream,
                               const RcDescriptionError& value,
                               int                       level,
                               int                       spacesPerLevel);

    // CREATORS

    /// Create an object of type `RcDescriptionError` having the default
    /// error value.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    explicit RcDescriptionError(bslma::Allocator* basicAllocator = 0);

    /// Create a copy of the specified `rhs` RcDescriptionError object.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    RcDescriptionError(const RcDescriptionError& rhs,
                       bslma::Allocator*         basicAllocator = 0);

    /// Create an object of type `RcDescriptionError` with the specified
    /// `rc` and `description`.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    RcDescriptionError(const int                rc,
                       const bslstl::StringRef& description,
                       bslma::Allocator*        basicAllocator = 0);

    // ACCESSORS

    /// Returns the return code associated to this object.
    const int& rc() const;

    /// Returns the description associated to this object.
    const bsl::string& description() const;
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&             stream,
                         const RcDescriptionError& value);

/// Compares the specified `lhs` and `rhs` and return `true` if their values
/// are equal or `false` otherwise.
bool operator==(const RcDescriptionError& lhs, const RcDescriptionError& rhs);

/// Compares the specified `lhs` and `rhs` and return `false` if their
/// values are equal or `true` otherwise.
bool operator!=(const RcDescriptionError& lhs, const RcDescriptionError& rhs);

// ============================================================================
//                         INLINE FUNCTION DEFINITIONS
// ============================================================================

// ------------------------
// class RcDescriptionError
// ------------------------

inline RcDescriptionError::RcDescriptionError(bslma::Allocator* basicAllocator)
: d_rc(0)
, d_description(basicAllocator)
{
    // NOTHING
}

inline RcDescriptionError::RcDescriptionError(const RcDescriptionError& rhs,
                                              bslma::Allocator* basicAllocator)
: d_rc(rhs.d_rc)
, d_description(rhs.d_description, basicAllocator)
{
    // NOTHING
}

inline RcDescriptionError::RcDescriptionError(
    const int                rc,
    const bslstl::StringRef& description,
    bslma::Allocator*        basicAllocator)
: d_rc(rc)
, d_description(description, basicAllocator)
{
    // NOTHING
}

inline const int& RcDescriptionError::rc() const
{
    return d_rc;
}

inline const bsl::string& RcDescriptionError::description() const
{
    return d_description;
}

// ------------------------
// class RcDescriptionError
// ------------------------

// FREE OPERATORS
inline bsl::ostream& operator<<(bsl::ostream&             stream,
                                const RcDescriptionError& value)
{
    return RcDescriptionError::print(stream, value, 0, -1);
}

inline bool operator==(const RcDescriptionError& lhs,
                       const RcDescriptionError& rhs)
{
    return ((lhs.rc() == rhs.rc()) &&
            (lhs.description() == rhs.description()));
}

inline bool operator!=(const RcDescriptionError& lhs,
                       const RcDescriptionError& rhs)
{
    return !(lhs == rhs);
}

}  // close package namespace

}  // close enterprise namespace

#endif
