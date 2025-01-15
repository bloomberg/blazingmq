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

// mqbc_clusterstateledger.h                                          -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERSTATELEDGER
#define INCLUDED_MQBC_CLUSTERSTATELEDGER

/// @file mqbc_clusterstateledger.h
///
/// @brief Provide an interface to maintain replicated log of cluster's state.
///
/// The @bbref{mqbc::ClusterStateLedger} base protocol is the interface for a
/// mechanism to maintain a replicated log of cluster's state.  Each node in
/// the cluster keeps an instance of a concrete implementation of this
/// interface.  The ledger is written to only upon notification from the leader
/// node, and the leader broadcasts advisories which followers apply to their
/// ledger and its in-memory representation.  Leader will apply update to
/// self's ledger, broadcast it asynchronously to cluster nodes, and also
/// advertise the update to cluster state's observers when appropriate
/// consistency level has been achieved.
/// @bbref{mqbc::ClusterStateLedgerConsistency} provides a namespaced enum for
/// consistency criteria of a @bbref{mqbc::ClusterStateLedger} implementation.
/// Leader will apply an update to cluster state in self's ledger, broadcast it
/// asynchronously, and notify via a callback when the specified consistency
/// level has been achieved.
///
/// Thread Safety                             {#mqbc_clusterstateledger_thread}
/// =============
///
/// The @bbref{mqbc::ClusterStateLedger} object is not thread safe and should
/// always be manipulated from the associated cluster's dispatcher thread,
/// unless explicitly documented in a method's contract.

// MQB
#include <mqbc_clusterstateledgeriterator.h>
#include <mqbc_electorinfo.h>
#include <mqbcfg_messages.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_managedptr.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}

namespace mqbc {

class ClusterState;

// =====================================
// struct ClusterStateLedgerCommitStatus
// =====================================

/// This struct defines the type of status of a commit operation.
struct ClusterStateLedgerCommitStatus {
    // TYPES

    /// Enumeration used to distinguish among different type of commit status.
    enum Enum {
        /// Operation was success.
        e_SUCCESS = 0,
        /// Operation was canceled (leader pre-emption, etc.).
        e_CANCELED = -1,
        /// Operation timeout.
        e_TIMEOUT = -2
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value` to
    /// the specified output `stream`, and return a reference to `stream`.
    /// Optionally specify an initial indentation `level`, whose absolute value
    /// is incremented recursively for nested objects.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this and all
    /// of its nested objects.  If `level` is negative, suppress indentation of
    /// the first line.  If `spacesPerLevel` is negative, format the entire
    /// output on one line, suppressing all but the initial indentation (as
    /// governed by `level`).  See `toAscii` for what constitutes the string
    /// representation of a @bbref{ClusterStateLedgerCommitStatus::Enum} value.
    static bsl::ostream& print(bsl::ostream&                        stream,
                               ClusterStateLedgerCommitStatus::Enum value,
                               int                                  level = 0,
                               int spacesPerLevel                         = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ClusterStateLedgerCommitStatus::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ClusterStateLedgerCommitStatus::Enum* out,
                          const bslstl::StringRef&              str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                        stream,
                         ClusterStateLedgerCommitStatus::Enum value);

// ====================================
// struct ClusterStateLedgerConsistency
// ====================================

/// This struct defines the type of consistency criteria for the persisted
/// cluster state of a @bbref{mqbc::ClusterStateLedger} implementation.
///
/// @note *STRONG* consistency level will not be supported initially.
struct ClusterStateLedgerConsistency {
    // TYPES

    // Enumeration used to distinguish among different consistency criteria.
    enum Enum {
        /// Leader does not wait for acks from any followers.
        e_EVENTUAL,
        /// Leader waits for acks from majority of the followers.
        e_STRONG
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value` to
    /// the specified output `stream`, and return a reference to `stream`.
    /// Optionally specify an initial indentation `level`, whose absolute value
    /// is incremented recursively for nested objects.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this and all
    /// of its nested objects.  If `level` is negative, suppress indentation of
    /// the first line.  If `spacesPerLevel` is negative, format the entire
    /// output on one line, suppressing all but the initial indentation (as
    /// governed by `level`).  See `toAscii` for what constitutes the string
    /// representation of a @bbref{ClusterStateLedgerConsistency::Enum} value.
    static bsl::ostream& print(bsl::ostream&                       stream,
                               ClusterStateLedgerConsistency::Enum value,
                               int                                 level = 0,
                               int spacesPerLevel                        = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ClusterStateLedgerConsistency::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ClusterStateLedgerConsistency::Enum* out,
                          const bslstl::StringRef&             str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                       stream,
                         ClusterStateLedgerConsistency::Enum value);

// ========================
// class ClusterStateLedger
// ========================

/// Provide a base protocol to maintain a replicated log of cluster's state.  A
/// concrete implementation can be configured with the desired consistency
/// level.  A leader node will apply update to self's ledger, broadcast it
/// asynchronously, and advertise when the configured consistency level has
/// been achieved.
///
/// @todo Declare these methods once the parameter object types have been
/// defined in @bbref{mqbc::ClusterState}:
/// ```
/// virtual int apply(const ClusterStateQueueInfo& queueInfo) = 0;
/// virtual int apply(const UriToQueueInfoMap& queuesInfo) = 0;
/// virtual int apply(const ClusterStatePartitionInfo& partitionInfo) = 0;
/// virtual int apply(const PartitionsInfo& partitionsInfo) = 0;
/// ```
///
/// @todo Apply the specified message to self and replicate if self is leader.
///
/// @todo Notify via 'commitCb' when consistency level has been achieved.
class ClusterStateLedger : public ElectorInfoObserver {
  public:
    // TYPES

    /// Invoked when the status of a commit operation becomes available,
    /// `CommitCb` is an alias for a callback function object (functor) that
    /// takes as an argument the specified `status` indicating the result of
    /// the commit operation for the specified `advisory`.
    ///
    /// The `advisory` message can be uniquely identified by the
    /// LeaderMessageSequence it contains.  Note that upon receiving a
    /// successful commit notification for an LSN, it is implicit that
    /// advisory messages having a sequence number *smaller* than that LSN
    /// have also been committed.
    ///
    /// Callback will be invoked at leader node in one of these situations:
    ///   - Leader has received sufficient number of acknowledgements from
    ///     followers (ClusterStateLedgerCommitStatus=SUCCESS) for an LSN.
    ///   - Leader has timed out while waiting for majority of
    ///     acknowledgements (ClusterStateLedgerCommitStatus=TIMEOUT).
    ///   - Commit operation has been canceled (Maybe leader lost quorum or
    ///     was preempted) (ClusterStateLedgerCommitStatus=CANCELED).
    ///
    /// @todo Review this.
    ///
    /// Callback will be invoked at follower node when finished:
    ///   - Follower succesfully applied the message
    ///     (ClusterStateLedgerCommitStatus=SUCCESS).
    ///   - Follower could not apply the message
    ///    (ClusterStateLedgerCommitStatus=CANCELED)
    typedef bsl::function<void(const bmqp_ctrlmsg::ControlMessage&  advisory,
                               ClusterStateLedgerCommitStatus::Enum status)>
        CommitCb;

  public:
    // CREATORS

    /// Destructor.
    ~ClusterStateLedger() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Open the ledger, and return 0 on success or a non-zero value
    /// otherwise.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual int open() = 0;

    /// Close the ledger, and return 0 on success or a non-zero value
    /// otherwise.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    virtual int close() = 0;

    /// @{
    /// Apply the specified `advisory` to self and replicate to followers.
    /// Notify via `commitCb` when consistency level has been achieved.
    /// Note that *only* a leader node may invoke this routine.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    virtual int
    apply(const bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory) = 0;
    virtual int
    apply(const bmqp_ctrlmsg::QueueAssignmentAdvisory& advisory) = 0;
    virtual int
    apply(const bmqp_ctrlmsg::QueueUnassignedAdvisory& advisory)         = 0;
    virtual int apply(const bmqp_ctrlmsg::QueueUpdateAdvisory& advisory) = 0;
    virtual int apply(const bmqp_ctrlmsg::LeaderAdvisory& advisory)      = 0;
    /// @}

    /// Apply the advisory contained in the specified `clusterMessage` to
    /// self and replicate to followers.  Notify via `commitCb` when
    /// consistency level has been achieved.  Note that *only* a leader node
    /// may invoke this routine.  Behavior is undefined unless the contained
    /// advisory is one of `PartitionPrimaryAdvisory`,
    /// `QueueAssignmentAdvisory`, `QueueUnassignedAdvisory`.
    /// `QueueUpdateAdvisory` or `LeaderAdvisory`.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    virtual int apply(const bmqp_ctrlmsg::ClusterMessage& clusterMessage) = 0;

    /// Apply the specified raw `record` received from the specified
    /// `source` node.  Note that while a replica node may receive any type
    /// of record from the leader, the leader may *only* receive ack records
    /// from a replica.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    virtual int apply(const bdlbb::Blob&   record,
                      mqbnet::ClusterNode* source) = 0;

    /// Set the commit callback to the specified `value`.
    virtual void setCommitCb(const CommitCb& value) = 0;

    // ACCESSORS

    /// Return true if this ledger is opened, false otherwise.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual bool isOpen() const = 0;

    /// Return an iterator to this ledger.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    virtual bslma::ManagedPtr<ClusterStateLedgerIterator>
    getIterator() const = 0;
};

}  // close package namespace

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ------------------------------------
// struct ClusterStateLedgerConsistency
// ------------------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbc::operator<<(bsl::ostream&                             stream,
                 mqbc::ClusterStateLedgerConsistency::Enum value)
{
    return ClusterStateLedgerConsistency::print(stream, value, 0, -1);
}

// -------------------------------------
// struct ClusterStateLedgerCommitStatus
// -------------------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbc::operator<<(bsl::ostream&                              stream,
                 mqbc::ClusterStateLedgerCommitStatus::Enum value)
{
    return ClusterStateLedgerCommitStatus::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
