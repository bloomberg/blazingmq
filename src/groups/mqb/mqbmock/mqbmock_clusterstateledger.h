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

// mqbmock_clusterstateledger.h                                       -*-C++-*-
#ifndef INCLUDED_MQBMOCK_CLUSTERSTATELEDGER
#define INCLUDED_MQBMOCK_CLUSTERSTATELEDGER

//@PURPOSE: Provide a mock implementation of 'mqbc::ClusterStateLedger'.
//
//@CLASSES:
//  mqbmock::ClusterStateLedger: Mock impl of 'mqbc::ClusterStateLedger'
//
//@DESCRIPTION: This component provides a mock implementation,
// 'mqbmock::ClusterStateLedger', of the 'mqbc::ClusterStateLedger' protocol
// that is used to maintain a replicated log of cluster's state.
//
/// Thread Safety
///-------------
// The 'mqbmock::ClusterStateLedger' object is not thread safe and should
// always be manipulated from the associated cluster's dispatcher thread,
// unless explicitly documented in a method's contract.

// MQB

#include <mqbc_clusterdata.h>
#include <mqbc_clusterstateledger.h>
#include <mqbmock_clusterstateledgeriterator.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_functional.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbmock {

// ========================
// class ClusterStateLedger
// ========================

/// Mock implementation of `mqbc::ClusterStateLedger` interface.
class ClusterStateLedger : public mqbc::ClusterStateLedger {
  public:
    // TYPES
    typedef bsl::vector<bmqp_ctrlmsg::ClusterMessage> Advisories;
    typedef Advisories::const_iterator                AdvisoriesCIter;

  private:
    // PRIVATE TYPES
    typedef ClusterStateLedgerIterator::LedgerRecords LedgerRecords;

    typedef bmqp_ctrlmsg::ClusterMessageChoice MsgChoice;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator used to supply memory.

    bool d_isOpen;
    // Flag to indicate open/close status of this object.

    bool d_pauseCommitCb;
    // Flag to indicate whether to pause commit callback.

    CommitCb d_commitCb;
    // Callback invoked when the status of a commit
    // operation becomes available.

    mqbc::ClusterData* d_clusterData_p;
    // Cluster's transient state.

    LedgerRecords d_records;
    // List of records stored in this ledger.

    Advisories d_uncommittedAdvisories;
    // List of uncommitted (but not canceled) advisories.

  private:
    // PRIVATE MANIPULATORS

    /// Internal helper method to apply the specified `clusterMessage`.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int
    applyAdvisoryInternal(const bmqp_ctrlmsg::ClusterMessage& clusterMessage);

    // PRIVATE ACCESSORS

    /// Return true if self node is the leader, false otherwise.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    bool isSelfLeader() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterStateLedger,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance of `ClusterStateLedger` using the specified
    /// `clusterData` and `allocator`.
    ClusterStateLedger(mqbc::ClusterData* clusterData,
                       bslma::Allocator*  allocator);

    /// Destructor.
    ~ClusterStateLedger() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbc::ClusterStateLedger)

    /// Open the ledger, and return 0 on success or a non-zero value
    /// otherwise.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int open() BSLS_KEYWORD_OVERRIDE;

    /// Close the ledger, and return 0 on success or a non-zero value
    /// otherwise.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int close() BSLS_KEYWORD_OVERRIDE;

    /// Apply the specified `advisory` to self and replicate to followers.
    /// Notify via `commitCb` when consistency level has been achieved.
    /// Note that *only* a leader node may invoke this routine.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int apply(const bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int apply(const bmqp_ctrlmsg::QueueAssignmentAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int apply(const bmqp_ctrlmsg::QueueUnAssignmentAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int apply(const bmqp_ctrlmsg::QueueUpdateAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;

    /// Apply the specified `advisory` to self and replicate to followers.
    /// Notify via `commitCb` when consistency level has been achieved.  Note
    /// that *only* a leader node may invoke this routine.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int
    apply(const bmqp_ctrlmsg::LeaderAdvisory& advisory) BSLS_KEYWORD_OVERRIDE;

    /// Apply the advisory contained in the specified `clusterMessage` to
    /// self and replicate to followers.  Notify via `commitCb` when
    /// consistency level has been achieved.  Note that *only* a leader node
    /// may invoke this routine.  Behavior is undefined unless the contained
    /// advisory is one of `PartitionPrimaryAdvisory`,
    /// `QueueAssignmentAdvisory`, `QueueUnAssignmentAdvisory`.
    /// `QueueUpdateAdvisory` or `LeaderAdvisory`.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int apply(const bmqp_ctrlmsg::ClusterMessage& clusterMessage)
        BSLS_KEYWORD_OVERRIDE;

    /// Apply the specified raw `record` received from the specified
    /// `source` node.  Note that while a follower node may receive any type
    /// of record from the leader, the leader may *only* receive ack records
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int apply(const bdlbb::Blob&   record,
              mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Set the commit callback to the specified `value`.
    void setCommitCb(const CommitCb& value) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Set the pause commit callback flag to the specified `value`.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    void _setPauseCommitCb(bool value);

    /// Commit all pending advisories with the specified commit `status`.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    void _commitAdvisories(mqbc::ClusterStateLedgerCommitStatus::Enum status);

    // ACCESSORS
    //   (virtual mqbc::ClusterStateLedger)

    /// Return true if this ledger is opened, false otherwise.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    bool isOpen() const BSLS_KEYWORD_OVERRIDE;

    /// Return an iterator to this ledger.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator>
    getIterator() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the list of uncommitted advisories.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    const Advisories& _uncommittedAdvisories() const;
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class ClusterStateLedger
// ------------------------

// PRIVATE ACCESSORS
inline bool ClusterStateLedger::isSelfLeader() const
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    return d_clusterData_p->electorInfo().isSelfLeader();
}

// MANIPULATORS
//   (virtual mqbc::ClusterStateLedger)
inline void ClusterStateLedger::setCommitCb(const CommitCb& value)
{
    d_commitCb = value;
}

// MANIPULATORS
inline void ClusterStateLedger::_setPauseCommitCb(bool value)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    d_pauseCommitCb = value;
}

// ACCESSORS
//   (virtual mqbc::ClusterStateLedger)
inline bool ClusterStateLedger::isOpen() const
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    return d_isOpen;
}

// ACCESSORS
inline const ClusterStateLedger::Advisories&
ClusterStateLedger::_uncommittedAdvisories() const
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    return d_uncommittedAdvisories;
}

}  // close package namespace
}  // close enterprise namespace

#endif
