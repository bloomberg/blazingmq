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

// bmqa_queueid.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQA_QUEUEID
#define INCLUDED_BMQA_QUEUEID

//@PURPOSE: Provide a value-semantic efficient identifier for a queue.
//
//@CLASSES:
//  bmqa::QueueId: Value-semantic efficient identifier for a queue.
//
//@DESCRIPTION: This component implements a value-semantic class,
// 'bmqa::QueueId', which can be used to efficiently identify the queue
// associated with a message event.  A 'bmqa::QueueId' instance can be created
// with a 64-bit integer, raw pointer, shared pointer, or
// 'bmqt::CorrelationId'.

// BMQ

#include <bmqt_correlationid.h>
#include <bmqt_queueoptions.h>
#include <bmqt_uri.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqimp {
class Queue;
}

namespace bmqa {

// =============
// class QueueId
// =============

/// Value-semantic efficient identifier for a queue
class QueueId {
    // FRIENDS
    friend bool operator==(const QueueId& lhs, const QueueId& rhs);
    friend bool operator!=(const QueueId& lhs, const QueueId& rhs);
    friend bool operator<(const QueueId& lhs, const QueueId& rhs);

  private:
    // DATA
    bsl::shared_ptr<bmqimp::Queue> d_impl_sp;  // pimpl

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueId, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Default constructor.  Create a new QueueId associated with an
    /// automatically generated correlation Id, using the optionally
    /// specified `allocator`.
    explicit QueueId(bslma::Allocator* allocator = 0);

    /// Copy constructor, create a new queueId having the same values as the
    /// specified `other`, and using the optionally specified `allocator`.
    QueueId(const QueueId& other, bslma::Allocator* allocator = 0);

    /// Create a new QueueId associated to the correlation Id having the
    /// specified `correlationId` value, using the optionally specified
    /// `allocator`.
    explicit QueueId(const bmqt::CorrelationId& correlationId,
                     bslma::Allocator*          allocator = 0);

    /// Create a new QueueId associated to the correlation Id having the
    /// specified `numeric` correlationId value, using the optionally
    /// specified `allocator`.
    explicit QueueId(bsls::Types::Int64 numeric,
                     bslma::Allocator*  allocator = 0);

    /// Create a new QueueId associated to the correlation Id having the
    /// specified `pointer` correlationId value, using the optionally
    /// specified `allocator`.
    explicit QueueId(void* pointer, bslma::Allocator* allocator = 0);

    /// Create a new QueueId associated to the correlation Id having the
    /// specified `sharedPtr` correlationId value, using the optionally
    /// specified `allocator`.  The lifetime of `sharedPtr` is tied to this
    /// object, and it is the responsibility of the user to manage it
    /// accordingly.
    explicit QueueId(const bsl::shared_ptr<void>& sharedPtr,
                     bslma::Allocator*            allocator = 0);

    // MANIPULATORS

    /// Assignment operator, from the specified `rhs` using the specified
    /// `allocator`.
    QueueId& operator=(const QueueId& rhs);

    // ACCESSORS

    /// Return the correlationId associated to this QueueId. The behavior is
    /// undefined unless this QueueId is valid.
    const bmqt::CorrelationId& correlationId() const;

    /// Return the flags used when opening this queue.
    bsls::Types::Uint64 flags() const;

    /// Return the URI associated to this QueueId. The behavior is
    /// undefined unless this QueueId is valid.
    const bmqt::Uri& uri() const;

    /// Return the options used when opening this queue.
    const bmqt::QueueOptions& options() const;

    /// Return whether this QueueId is valid, i.e., is associated to an
    /// opened queue.
    bool isValid() const;

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

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const QueueId& rhs);

/// Return `true` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return false otherwise.
bool operator==(const QueueId& lhs, const QueueId& rhs);

/// Return `false` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return `true` otherwise.
bool operator!=(const QueueId& lhs, const QueueId& rhs);

/// Operator used to allow comparison between the specified `lhs` and `rhs`
/// CorrelationId objects so that CorrelationId can be used as key in a map.
bool operator<(const QueueId& lhs, const QueueId& rhs);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class QueueId
// -------------

inline bsl::ostream& bmqa::operator<<(bsl::ostream&        stream,
                                      const bmqa::QueueId& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bool bmqa::operator==(const bmqa::QueueId& lhs,
                             const bmqa::QueueId& rhs)
{
    return rhs.d_impl_sp.get() == lhs.d_impl_sp.get();
}

inline bool bmqa::operator!=(const bmqa::QueueId& lhs,
                             const bmqa::QueueId& rhs)
{
    return rhs.d_impl_sp.get() != lhs.d_impl_sp.get();
}

inline bool bmqa::operator<(const bmqa::QueueId& lhs, const bmqa::QueueId& rhs)
{
    return rhs.d_impl_sp.get() < lhs.d_impl_sp.get();
}

}  // close enterprise namespace

#endif
