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

// mqbc_clusterstateledgeriterator.h                                  -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERSTATELEDGERITERATOR
#define INCLUDED_MQBC_CLUSTERSTATELEDGERITERATOR

/// @file mqbc_clusterstateledgeriterator.h
///
/// @brief Provide an interface to iterate through an
/// @bbref{mqbc::ClusterStateLedger}.
///
/// The @bbref{mqbc::ClusterStateLedgerIterator} base protocol is the interface
/// for a mechanism to iterate through an @bbref{mqbc::ClusterStateLedger}.
///
/// @see @bbref{mqbc::ClusterStateLedger}.
///
/// Thread Safety                     {#mqbc_clusterstateledgeriterator_thread}
/// =============
///
/// The @bbref{mqbc::ClusterStateLedgerIterator} object is not thread safe and
/// should always be manipulated from the associated cluster's dispatcher
/// thread, unless explicitly documented in a method's contract.

// MQB
#include <mqbc_clusterstateledgerprotocol.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>

namespace BloombergLP {
namespace mqbc {

// =================================
// struct ClusterStateLedgerIterator
// =================================

/// Provide an interface to iterate through an
/// @bbref{mqbc::ClusterStateLedger}.
class ClusterStateLedgerIterator {
  public:
    // CREATORS

    /// Destructor
    virtual ~ClusterStateLedgerIterator();

    // MANIPULATORS

    /// Iterate to the next record in the @bbref{mqbc::ClusterStateLedger}.
    /// Return 0 if a record is found, 1 if the end of the ledger
    /// is reached, or < 0 if an error was encountered.  If this method
    /// returns a non-zero value, this iterator becomes invalid and
    /// `isValid()` must return false.  Note that this method must be called
    /// once prior to accessing the first record.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual int next() = 0;

    /// Assign self to be a copy of the specified `other`.  This uses the
    /// assignment operator under the hood, but circumvents object slicing.
    /// Behavior is undefined unless `other` and this object have the same
    /// type.
    virtual void copy(const ClusterStateLedgerIterator& other) = 0;

    // ACCESSORS

    /// Return a new `ClusterStateLedgerIterator`, which is a clone of self,
    /// using the specified `allocator`.
    virtual bslma::ManagedPtr<ClusterStateLedgerIterator>
    clone(bslma::Allocator* allocator) const = 0;

    /// Return true if this iterator is initialized and valid, and the
    /// previous call to `next()` returned 0; return false otherwise.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual bool isValid() const = 0;

    /// Return a const reference to the `ClusterStateRecordHeader` at the
    /// iterator position.  Behavior is undefined unless `isValid()` returns
    /// true.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual const ClusterStateRecordHeader& header() const = 0;

    /// Load the cluster message recorded at the iterator position to the
    /// specified `message`.  Return 0 on success or a non-zero error value
    /// otherwise.  Behavior is undefined unless `isValid()` returns true.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual int
    loadClusterMessage(bmqp_ctrlmsg::ClusterMessage* message) const = 0;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.  Behavior is
    /// undefined unless `isValid()` returns true.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual bsl::ostream& print(bsl::ostream& stream,
                                int           level          = 0,
                                int           spacesPerLevel = 4) const = 0;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                     stream,
                         const ClusterStateLedgerIterator& rhs);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------
// struct ClusterStateLedgerIterator
// ---------------------------------

// FREE OPERATORS
inline bsl::ostream& mqbc::operator<<(bsl::ostream&                     stream,
                                      const ClusterStateLedgerIterator& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
