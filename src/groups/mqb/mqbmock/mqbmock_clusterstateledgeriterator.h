// Copyright 2021-2023 Bloomberg Finance L.P.
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

// mqbmock_clusterstateledgeriterator.h                               -*-C++-*-
#ifndef INCLUDED_MQBMOCK_CLUSTERSTATELEDGERITERATOR
#define INCLUDED_MQBMOCK_CLUSTERSTATELEDGERITERATOR

//@PURPOSE: Provide mock implementation of 'mqbc::ClusterStateLedgerIterator'.
//
//@CLASSES:
//  mqbmock::ClusterStateLedgerIterator: Mock implementation of
//                                       'mqbc::ClusterStateLedgerIterator'
//
//@DESCRIPTION: This component provides a mock implementation,
// 'mqbmock::ClusterStateLedgerIterator', of the
// 'mqbc::ClusterStateLedgerIterator' protocol that is used to iterate through
// an 'mqbc::ClusterStateLedger'.
//
/// Thread Safety
///-------------
// The 'mqbmock::ClusterStateLedgerIterator' object is not thread safe and
// should always be manipulated from the associated cluster's dispatcher
// thread, unless explicitly documented in a method's contract.

// MQB

#include <mqbc_clusterstateledgeriterator.h>
#include <mqbc_clusterstateledgerprotocol.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_list.h>
#include <bsl_ostream.h>

namespace BloombergLP {
namespace mqbmock {

// =================================
// struct ClusterStateLedgerIterator
// =================================

/// Mock implementation of `mqbc::ClusterStateLedgerIterator` interface.
class ClusterStateLedgerIterator BSLS_KEYWORD_FINAL
: public mqbc::ClusterStateLedgerIterator {
  public:
    // PUBLIC TYPES
    typedef bsl::list<bmqp_ctrlmsg::ClusterMessage> LedgerRecords;
    typedef LedgerRecords::const_iterator           LedgerRecordsCIter;

  private:
    // TYPES
    typedef bmqp_ctrlmsg::ClusterMessageChoice MsgChoice;

  private:
    // DATA
    bool d_firstNextCall;
    // Whether next() will be called for the
    // first time.  If so, we do not advance
    // the internal iterator as per the
    // contract of next().

    LedgerRecordsCIter d_iter;
    // Internal iterator through the ledger
    // records.

    LedgerRecordsCIter d_iterEnd;
    // End position of the internal iterator.

    mqbc::ClusterStateRecordHeader d_currRecordHeader;
    // Header of the record at the current
    // iterator position.

  public:
    // CREATORS

    /// Create an instance of `ClusterStateLedgerIterator` iterating through
    /// the specified `records`.
    ClusterStateLedgerIterator(const LedgerRecords& records);

    /// Copy constructor, create a new instance of
    /// `ClusterStateLedgerIterator` pointing to the same position of the
    /// same ledger as the specified `other`.
    ClusterStateLedgerIterator(const ClusterStateLedgerIterator& other);

    /// Assignment operator, pointing self's iterator position to be the
    /// same position of the same ledger as the specified `rhs`.
    ClusterStateLedgerIterator&
    operator=(const ClusterStateLedgerIterator& rhs);

    /// Destructor
    ~ClusterStateLedgerIterator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Iterate to the next record in the `mqbc::ClusterStateLedger`.
    /// Return 0 if a record is found, 1 if the end of the ledger
    /// is reached, or < 0 if an error was encountered.  If this method
    /// returns a non-zero value, this iterator becomes invalid and
    /// `isValid()` must return false.  Note that this method must be called
    /// once prior to accessing the first record.
    int next() BSLS_KEYWORD_OVERRIDE;

    /// Assign self to be a copy of the specified `other`.  This uses the
    /// assignment operator under the hood, but circumvents object slicing.
    /// Behavior is undefined unless `other` and this object have the same
    /// type.
    void
    copy(const mqbc::ClusterStateLedgerIterator& other) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a new `ClusterStateLedgerIterator`, which is a clone of self,
    /// using the specified `allocator`.
    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator>
    clone(bslma::Allocator* allocator) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this iterator is initialized and valid, and the
    /// previous call to `next()` returned 0; return false otherwise.
    bool isValid() const BSLS_KEYWORD_OVERRIDE;

    /// Return a const reference to the `ClusterStateRecordHeader` at the
    /// iterator position.  Behavior is undefined unless `isValid()` returns
    /// true.
    const mqbc::ClusterStateRecordHeader& header() const BSLS_KEYWORD_OVERRIDE;

    /// Load the cluster message recorded at the iterator position to the
    /// specified `message`.  Return 0 on success or a non-zero error value
    /// otherwise.  Behavior is undefined unless `isValid()` returns true.
    int loadClusterMessage(bmqp_ctrlmsg::ClusterMessage* message) const
        BSLS_KEYWORD_OVERRIDE;

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
    bsl::ostream& print(bsl::ostream& stream,
                        int           level = 0,
                        int spacesPerLevel  = 4) const BSLS_KEYWORD_OVERRIDE;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                     stream,
                         const ClusterStateLedgerIterator& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------
// struct ClusterStateLedgerIterator
// ---------------------------------

// CREATOR
inline ClusterStateLedgerIterator::ClusterStateLedgerIterator(
    const ClusterStateLedgerIterator& other)
: d_firstNextCall(other.d_firstNextCall)
, d_iter(other.d_iter)
, d_iterEnd(other.d_iterEnd)
, d_currRecordHeader(other.d_currRecordHeader)
{
    // NOTHING
}

inline ClusterStateLedgerIterator&
ClusterStateLedgerIterator::operator=(const ClusterStateLedgerIterator& rhs)
{
    d_firstNextCall    = rhs.d_firstNextCall;
    d_iter             = rhs.d_iter;
    d_iterEnd          = rhs.d_iterEnd;
    d_currRecordHeader = rhs.d_currRecordHeader;

    return *this;
}

// MANIPULATORS
//   (virtual mqbc::ClusterStateLedgerIterator)
inline void
ClusterStateLedgerIterator::copy(const mqbc::ClusterStateLedgerIterator& other)
{
    *this = dynamic_cast<const ClusterStateLedgerIterator&>(other);
}

// ACCESSORS
inline bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator>
ClusterStateLedgerIterator::clone(bslma::Allocator* allocator) const
{
    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator> mp(
        new (*allocator) ClusterStateLedgerIterator(*this),
        allocator);

    return mp;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mqbmock::operator<<(bsl::ostream& stream,
                                         const ClusterStateLedgerIterator& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
