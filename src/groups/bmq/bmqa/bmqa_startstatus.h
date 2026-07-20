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

#ifndef INCLUDED_BMQA_STARTSTATUS
#define INCLUDED_BMQA_STARTSTATUS

/// @file bmqa_startstatus.h
///
/// @brief Provide Value-Semantic Type for a session start operation status
///
/// This component provides a specific value-semantic type for the result of an
/// asynchronous start operation of a @bbref{bmqa::Session} with the BlazingMQ
/// broker, providing applications with the result and context of the requested
/// operation.
///
/// A @bbref{bmqa::StartStatus} type is composed of 2 attributes:
///
///   1. **result**: indicates the status of the operation (success, timeout,
///      etc.) as specified in the corresponding result code enum,
///      @bbref{bmqt::GenericResult::Enum}.
///
///   2. **errorDescription**: optional string with a human readable
///      description of the error, if any

// BMQ

#include <bmqt_resultcode.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_unspecifiedbool.h>

namespace BloombergLP {
namespace bmqa {

// =================
// class StartStatus
// =================

/// A value-semantic type for an asynchronous start operation with the message
/// queue broker.
class StartStatus {
  private:
    // DATA

    /// Result code of the operation (success, failure)
    bmqt::GenericResult::Enum d_result;

    /// Optional string with a human readable description of the error, if any
    bsl::string d_errorDescription;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StartStatus, bslma::UsesBslmaAllocator)

    // TYPES

    /// Use of an `UnspecifiedBool` to prevent implicit conversions to
    /// integral values, and comparisons between different classes which
    /// have boolean operators.
    typedef bsls::UnspecifiedBool<StartStatus>::BoolType BoolType;

    // CREATORS

    /// Default constructor, use the optionally specified `allocator`.
    explicit StartStatus(bslma::Allocator* allocator = 0);

    /// Create a new `bmqa::StartStatus` using the optionally specified
    /// `allocator`.
    StartStatus(const bmqa::StartStatus& other,
                bslma::Allocator*        allocator = 0);

    /// Create a new `bmqa::StartStatus` object having the specified `result`
    /// and `errorDescription`, using the optionally specified `allocator` to
    /// supply memory.
    StartStatus(bmqt::GenericResult::Enum result,
                const bsl::string&        errorDescription,
                bslma::Allocator*         allocator = 0);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.
    StartStatus& operator=(const StartStatus& rhs);

    // ACCESSORS

    /// Return true if this result indicates success, and false otherwise.
    operator BoolType() const;

    /// Return the result code that indicates success or the cause of a
    /// failure.
    bmqt::GenericResult::Enum result() const;

    /// Return a printable description of the error, if `result` indicates
    /// failure.  Return an empty string otherwise.
    const bsl::string& errorDescription() const;

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
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const StartStatus& lhs, const StartStatus& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const StartStatus& lhs, const StartStatus& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const StartStatus& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class StartStatus
// -----------------

// CREATORS
inline StartStatus::StartStatus(bslma::Allocator* allocator)
: d_result(bmqt::GenericResult::e_SUCCESS)
, d_errorDescription(allocator)
{
    // NOTHING
}

inline StartStatus::StartStatus(const StartStatus& other,
                                bslma::Allocator*  allocator)
: d_result(other.d_result)
, d_errorDescription(other.d_errorDescription, allocator)
{
    // NOTHING
}

inline StartStatus::StartStatus(bmqt::GenericResult::Enum result,
                                const bsl::string&        errorDescription,
                                bslma::Allocator*         allocator)
: d_result(result)
, d_errorDescription(errorDescription, allocator)
{
    // NOTHING
}

// MANIPULATORS
inline StartStatus& StartStatus::operator=(const StartStatus& other)
{
    d_result           = other.result();
    d_errorDescription = other.errorDescription();
    return *this;
}

// ACCESSORS
inline StartStatus::operator BoolType() const
{
    return bsls::UnspecifiedBool<StartStatus>::makeValue(
        d_result == bmqt::GenericResult::e_SUCCESS);
}

inline bmqt::GenericResult::Enum StartStatus::result() const
{
    return d_result;
}

inline const bsl::string& StartStatus::errorDescription() const
{
    return d_errorDescription;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& bmqa::operator<<(bsl::ostream&            stream,
                                      const bmqa::StartStatus& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bool bmqa::operator==(const bmqa::StartStatus& lhs,
                             const bmqa::StartStatus& rhs)
{
    return lhs.result() == rhs.result() &&
           lhs.errorDescription() == rhs.errorDescription();
}

inline bool bmqa::operator!=(const bmqa::StartStatus& lhs,
                             const bmqa::StartStatus& rhs)
{
    return !(lhs == rhs);
}

}  // close enterprise namespace

#endif
