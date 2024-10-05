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

// bmqa_configurequeuestatus.h                                        -*-C++-*-
#ifndef INCLUDED_BMQA_CONFIGUREQUEUESTATUS
#define INCLUDED_BMQA_CONFIGUREQUEUESTATUS

/// @file bmqa_configurequeuestatus.h
///
/// @brief Provide Value-Semantic Type for a configure queue operation status
///
/// This component provides a specific value-semantic type for the result of a
/// configure queue operation with the BlazingMQ broker, providing applications
/// with the result and context of the requested operation.
///
///  A @bbref{bmqa::ConfigureQueueStatus} type is composed of 3 attributes:
///
///    1. **result**: indicates the status of the operation (success, failure,
///       etc.) as specified in the corresponding result code enum,
///       @bbref{bmqt::ConfigureQueueResult::Enum}.
///
///    2. **queueId**: queueId associated with the configure queue operation.
///
///    3. **errorDescription**: optional string with a human readable
///       description of the error, if any

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

namespace BloombergLP {
namespace bmqa {

// ==========================
// class ConfigureQueueStatus
// ==========================

/// A value-semantic type for a configure queue operation with the message
/// queue broker.
class ConfigureQueueStatus {
  private:
    // DATA

    /// queueId associated with the open
    /// queue operation
    QueueId d_queueId;

    /// Status code of the operation
    /// (success, failure)
    bmqt::ConfigureQueueResult::Enum d_result;

    /// Optional string with a human
    /// readable description of the error,
    /// if any
    bsl::string d_errorDescription;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ConfigureQueueStatus,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Default constructor, use the optionally specified `allocator`.
    explicit ConfigureQueueStatus(bslma::Allocator* allocator = 0);

    /// Create a new `bmqa::ConfigureQueueStatus` using the optionally
    /// specified `allocator`.
    ConfigureQueueStatus(const bmqa::ConfigureQueueStatus& other,
                         bslma::Allocator*                 allocator = 0);

    /// Create a new `bmqa::ConfigureQueueStatus` object having the
    /// specified `queueId`, `result`, and `errorDescription`, using the
    /// optionally specified `allocator` to supply memory.
    ConfigureQueueStatus(const QueueId&                   queueId,
                         bmqt::ConfigureQueueResult::Enum result,
                         const bsl::string&               errorDescription,
                         bslma::Allocator*                allocator = 0);

    // MANIPULATORS

    /// Assign to this `ConfigureQueueStatus` the same values as the one
    /// from the specified `rhs`.
    ConfigureQueueStatus& operator=(const ConfigureQueueStatus& rhs);

    // ACCESSORS

    /// Return true if this result indicates success, and false otherwise.
    operator bool() const;

    /// Return the queueId associated to this operation result, if any.
    const QueueId& queueId() const;

    /// Return the result code that indicates success or the cause of a
    /// failure.
    bmqt::ConfigureQueueResult::Enum result() const;

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
bool operator==(const ConfigureQueueStatus& lhs,
                const ConfigureQueueStatus& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const ConfigureQueueStatus& lhs,
                const ConfigureQueueStatus& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&               stream,
                         const ConfigureQueueStatus& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class ConfigureQueueStatus
// --------------------------

// CREATORS
inline ConfigureQueueStatus::ConfigureQueueStatus(bslma::Allocator* allocator)
: d_queueId(allocator)
, d_result(bmqt::ConfigureQueueResult::e_SUCCESS)
, d_errorDescription(allocator)
{
    // NOTHING
}

inline ConfigureQueueStatus::ConfigureQueueStatus(
    const ConfigureQueueStatus& other,
    bslma::Allocator*           allocator)
: d_queueId(other.d_queueId, allocator)
, d_result(other.d_result)
, d_errorDescription(other.d_errorDescription)
{
    // NOTHING
}

inline ConfigureQueueStatus::ConfigureQueueStatus(
    const QueueId&                   queueId,
    bmqt::ConfigureQueueResult::Enum result,
    const bsl::string&               errorDescription,
    bslma::Allocator*                allocator)
: d_queueId(queueId, allocator)
, d_result(result)
, d_errorDescription(errorDescription)
{
    // NOTHING
}

// MANIPULATORS
inline ConfigureQueueStatus&
ConfigureQueueStatus::operator=(const ConfigureQueueStatus& other)
{
    d_queueId          = other.queueId();
    d_result           = other.result();
    d_errorDescription = other.errorDescription();
    return *this;
}

// ACCESSORS
inline ConfigureQueueStatus::operator bool() const
{
    return result() == bmqt::ConfigureQueueResult::e_SUCCESS;
}

inline const QueueId& ConfigureQueueStatus::queueId() const
{
    return d_queueId;
}

inline bmqt::ConfigureQueueResult::Enum ConfigureQueueStatus::result() const
{
    return d_result;
}

inline const bsl::string& ConfigureQueueStatus::errorDescription() const
{
    return d_errorDescription;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& bmqa::operator<<(bsl::ostream&                     stream,
                                      const bmqa::ConfigureQueueStatus& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bool bmqa::operator==(const bmqa::ConfigureQueueStatus& lhs,
                             const bmqa::ConfigureQueueStatus& rhs)
{
    return lhs.queueId() == rhs.queueId() && lhs.result() == rhs.result() &&
           lhs.errorDescription() == rhs.errorDescription();
}

inline bool bmqa::operator!=(const bmqa::ConfigureQueueStatus& lhs,
                             const bmqa::ConfigureQueueStatus& rhs)
{
    return !(lhs == rhs);
}

}  // close enterprise namespace

#endif
