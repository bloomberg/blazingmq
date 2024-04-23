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

// bmqa_openqueuestatus.h                                             -*-C++-*-
#ifndef INCLUDED_BMQA_OPENQUEUESTATUS
#define INCLUDED_BMQA_OPENQUEUESTATUS

/// @file bmqa_openqueuestatus.h
///
/// @brief Provide Value-Semantic Type for an open queue operation status
///
/// This component provides a specific value-semantic type for the result of an
/// open queue operation with the BlazingMQ broker, providing applications with
/// the result and context of the requested operation.
///
/// A @bbref{bmqa::OpenQueueStatus} type is composed of 3 attributes:
///
///   1. **result**: indicates the status of the operation (success, failure,
///      etc.) as specified in the corresponding result code enum,
///      @bbref{bmqt::OpenQueueResult::Enum}.
///
///   2. **queueId**: queueId associated with the open queue operation
///
///   3. **errorDescription**: optional string with a human readable
///      description of the error, if any

// BMQ

#include <bmqa_queueid.h>
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

// =====================
// class OpenQueueStatus
// =====================

/// A value-semantic type for an open queue operation with the message queue
/// broker.
class OpenQueueStatus {
  private:
    // DATA
    QueueId d_queueId;
    // queueId associated with the open
    // queue operation

    bmqt::OpenQueueResult::Enum d_result;
    // Result code of the operation
    // (success, failure)

    bsl::string d_errorDescription;
    // Optional string with a human
    // readable description of the error,
    // if any

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OpenQueueStatus, bslma::UsesBslmaAllocator)

    // TYPES

    // Use of an `UnspecifiedBool` to prevent implicit conversions to
    // integral values, and comparisons between different classes which
    // have boolean operators.
    typedef bsls::UnspecifiedBool<OpenQueueStatus>::BoolType BoolType;

    // CREATORS

    /// Default constructor, use the optionally specified `allocator`.
    explicit OpenQueueStatus(bslma::Allocator* allocator = 0);

    /// Create a new `bmqa::OpenQueueStatus` using the optionally specified
    /// `allocator`.
    OpenQueueStatus(const bmqa::OpenQueueStatus& other,
                    bslma::Allocator*            allocator = 0);

    /// Create a new `bmqa::OpenQueueStatus` object having the specified
    /// `queueId`, `result`, and `errorDescription`, using the
    /// optionally specified `allocator` to supply memory.
    OpenQueueStatus(const QueueId&              queueId,
                    bmqt::OpenQueueResult::Enum result,
                    const bsl::string&          errorDescription,
                    bslma::Allocator*           allocator = 0);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.
    OpenQueueStatus& operator=(const OpenQueueStatus& rhs);

    // ACCESSORS

    /// Return true if this result indicates success, and false otherwise.
    operator BoolType() const;

    /// Return the queueId associated to this operation result, if any.
    const QueueId& queueId() const;

    /// Return the result code that indicates success or the cause of a
    /// failure.
    bmqt::OpenQueueResult::Enum result() const;

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
bool operator==(const OpenQueueStatus& lhs, const OpenQueueStatus& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const OpenQueueStatus& lhs, const OpenQueueStatus& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const OpenQueueStatus& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class OpenQueueStatus
// ---------------------

// CREATORS
inline OpenQueueStatus::OpenQueueStatus(bslma::Allocator* allocator)
: d_queueId(allocator)
, d_result(bmqt::OpenQueueResult::e_SUCCESS)
, d_errorDescription(allocator)
{
    // NOTHING
}

inline OpenQueueStatus::OpenQueueStatus(const OpenQueueStatus& other,
                                        bslma::Allocator*      allocator)
: d_queueId(other.d_queueId, allocator)
, d_result(other.d_result)
, d_errorDescription(other.d_errorDescription, allocator)
{
    // NOTHING
}

inline OpenQueueStatus::OpenQueueStatus(const QueueId&              queueId,
                                        bmqt::OpenQueueResult::Enum result,
                                        const bsl::string& errorDescription,
                                        bslma::Allocator*  allocator)
: d_queueId(queueId, allocator)
, d_result(result)
, d_errorDescription(errorDescription)
{
    // NOTHING
}

// MANIPULATORS
inline OpenQueueStatus&
OpenQueueStatus::operator=(const OpenQueueStatus& other)
{
    d_queueId          = other.queueId();
    d_result           = other.result();
    d_errorDescription = other.errorDescription();
    return *this;
}

// ACCESSORS
inline OpenQueueStatus::operator BoolType() const
{
    return bsls::UnspecifiedBool<OpenQueueStatus>::makeValue(
        d_result == bmqt::OpenQueueResult::e_SUCCESS);
}

inline const QueueId& OpenQueueStatus::queueId() const
{
    return d_queueId;
}

inline bmqt::OpenQueueResult::Enum OpenQueueStatus::result() const
{
    return d_result;
}

inline const bsl::string& OpenQueueStatus::errorDescription() const
{
    return d_errorDescription;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& bmqa::operator<<(bsl::ostream&                stream,
                                      const bmqa::OpenQueueStatus& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bool bmqa::operator==(const bmqa::OpenQueueStatus& lhs,
                             const bmqa::OpenQueueStatus& rhs)
{
    return lhs.queueId() == rhs.queueId() && lhs.result() == rhs.result() &&
           lhs.errorDescription() == rhs.errorDescription();
}

inline bool bmqa::operator!=(const bmqa::OpenQueueStatus& lhs,
                             const bmqa::OpenQueueStatus& rhs)
{
    return !(lhs == rhs);
}

}  // close enterprise namespace

#endif
