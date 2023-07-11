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

// mqbc_incoreclusterstateledgeriterator.h                            -*-C++-*-
#ifndef INCLUDED_MQBC_INCORECLUSTERSTATELEDGERITERATOR
#define INCLUDED_MQBC_INCORECLUSTERSTATELEDGERITERATOR

//@PURPOSE: Provide an iterator through an 'mqbc::IncoreClusterStateLedger'
//
//@CLASSES:
//  mqbc::IncoreClusterStateLedgerIterator: Iterator through an
//                                          'mqbc::IncoreClusterStateLedger'
//
//@SEE_ALSO:
//  mqbc::ClusterStateLedgerIterator
//  mqbc::IncoreClusterStateLedger
//
//@DESCRIPTION: The 'mqbc::IncoreClusterStateLedgerIterator' class is a
// concrete implementation of the 'mqbc::ClusterStateLedgerIterator' interface
// to iterate through an 'mqbc::IncoreClusterStateLedger'.  Note that any
// ledger iterated by this component *must* support aliasing.
//
/// Thread Safety
///-------------
// The 'mqbc::IncoreClusterStateLedgerIterator' object is not thread safe and
// should always be manipulated from the associated cluster's dispatcher
// thread, unless explicitly documented in a method's contract.

// MQB

#include <mqbc_clusterstateledgeriterator.h>
#include <mqbsi_ledger.h>
#include <mqbsi_log.h>

// BDE
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mqbc {

// =======================================
// struct IncoreClusterStateLedgerIterator
// =======================================

/// Provide a concrete implementation of the
/// `mqbc::ClusterStateLedgerIterator` interface to iterate through an
/// `mqbc::IncoreClusterStateLedger`.  Note that any ledger iterated by this
/// component *must* support aliasing.
class IncoreClusterStateLedgerIterator BSLS_KEYWORD_FINAL
: public ClusterStateLedgerIterator {
  private:
    // DATA
    bool d_firstInvocation;
    // Whether the next call to 'next()' is the
    // first invocation.

    bool d_isValid;
    // Whether this iterator is in a valid state.

    const mqbsi::Ledger* d_ledger_p;
    // Ledger to iterate through.

    mqbsi::LedgerRecordId d_currRecordId;
    // Id of the record at the current iterator
    // position.

    ClusterStateRecordHeader* d_currRecordHeader_p;
    // Header of the record at the current
    // iterator position.

  public:
    // CREATORS

    /// Create an instance of `IncoreClusterStateLedgerIterator` iterating
    /// through the specified `ledger`.  Note that `ledger` must already be
    /// opened.
    IncoreClusterStateLedgerIterator(const mqbsi::Ledger* ledger);

    /// Copy constructor, create a new instance of
    /// `IncoreClusterStateLedgerIterator` pointing to the same position of
    /// the same ledger as the specified `other`.
    IncoreClusterStateLedgerIterator(
        const IncoreClusterStateLedgerIterator& other);

    /// Assignment operator, pointing self's iterator position to be the
    /// same position of the same ledger as the specified `rhs`.
    IncoreClusterStateLedgerIterator&
    operator=(const IncoreClusterStateLedgerIterator& rhs);

    /// Destructor
    ~IncoreClusterStateLedgerIterator() BSLS_KEYWORD_OVERRIDE;

  private:
    // PRIVATE MANIPULATORS

    /// Increment the current record offset by the specified `amount`.
    void incrementOffset(mqbsi::Log::Offset amount);

  public:
    // MANIPULATORS
    //   (virtual mqbc::ClusterStateLedgerIterator)

    /// Iterate to the next record in the `mqbc::ClusterStateLedger`.
    /// Return 0 if a record is found, 1 if the end of the ledger
    /// is reached, or < 0 if an error was encountered.  If this method
    /// returns a non-zero value, this iterator becomes invalid and
    /// `isValid()` must return false.  Note that this method must be called
    /// once prior to accessing the first record.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual int next() BSLS_KEYWORD_OVERRIDE;

    /// Assign self to be a copy of the specified `other`.  This uses the
    /// assignment operator under the hood, but circumvents object slicing.
    /// Behavior is undefined unless `other` and this object have the same
    /// type.
    virtual void
    copy(const ClusterStateLedgerIterator& other) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual mqbc::ClusterStateLedgerIterator)

    /// Return a new `ClusterStateLedgerIterator`, which is a clone of self,
    /// using the specified `allocator`.
    virtual bslma::ManagedPtr<ClusterStateLedgerIterator>
    clone(bslma::Allocator* allocator) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this iterator is initialized and valid, and the
    /// previous call to `next()` returned 0; return false otherwise.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual bool isValid() const BSLS_KEYWORD_OVERRIDE;

    /// Return a const reference to the `ClusterStateRecordHeader` at the
    /// iterator position.  Behavior is undefined unless `isValid()` returns
    /// true.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual const ClusterStateRecordHeader&
    header() const BSLS_KEYWORD_OVERRIDE;

    /// Load the cluster message recorded at the iterator position to the
    /// specified `message`.  Return 0 on success or a non-zero error value
    /// otherwise.  Behavior is undefined unless `isValid()` returns true.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual int loadClusterMessage(bmqp_ctrlmsg::ClusterMessage* message) const
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
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual bsl::ostream&
    print(bsl::ostream& stream,
          int           level          = 0,
          int           spacesPerLevel = 4) const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the current ledger record Id.
    const mqbsi::LedgerRecordId& currRecordId() const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                           stream,
                         const IncoreClusterStateLedgerIterator& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------------
// struct IncoreClusterStateLedgerIterator
// ---------------------------------------

// CREATORS
inline IncoreClusterStateLedgerIterator::IncoreClusterStateLedgerIterator(
    const IncoreClusterStateLedgerIterator& other)
: d_firstInvocation(other.d_firstInvocation)
, d_isValid(other.d_isValid)
, d_ledger_p(other.d_ledger_p)
, d_currRecordId(other.d_currRecordId)
, d_currRecordHeader_p(other.d_currRecordHeader_p)
{
    // NOTHING
}

inline IncoreClusterStateLedgerIterator&
IncoreClusterStateLedgerIterator::operator=(
    const IncoreClusterStateLedgerIterator& rhs)
{
    d_firstInvocation    = rhs.d_firstInvocation;
    d_isValid            = rhs.d_isValid;
    d_ledger_p           = rhs.d_ledger_p;
    d_currRecordId       = rhs.d_currRecordId;
    d_currRecordHeader_p = rhs.d_currRecordHeader_p;

    return *this;
}

// MANIPULATORS
//   (virtual mqbc::ClusterStateLedgerIterator)
inline void
IncoreClusterStateLedgerIterator::copy(const ClusterStateLedgerIterator& other)
{
    *this = dynamic_cast<const IncoreClusterStateLedgerIterator&>(other);
}

// ACCESSORS
//   (virtual mqbc::ClusterStateLedgerIterator)
inline bslma::ManagedPtr<ClusterStateLedgerIterator>
IncoreClusterStateLedgerIterator::clone(bslma::Allocator* allocator) const
{
    bslma::ManagedPtr<ClusterStateLedgerIterator> mp(
        new (*allocator) IncoreClusterStateLedgerIterator(*this),
        allocator);

    return mp;
}

inline bool IncoreClusterStateLedgerIterator::isValid() const
{
    return d_isValid;
}

// ACCESSORS
inline const mqbsi::LedgerRecordId&
IncoreClusterStateLedgerIterator::currRecordId() const
{
    return d_currRecordId;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream&
mqbc::operator<<(bsl::ostream&                           stream,
                 const IncoreClusterStateLedgerIterator& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
