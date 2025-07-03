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

// mqbc_clusterstateledgerutil.h                                      -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERSTATELEDGERUTIL
#define INCLUDED_MQBC_CLUSTERSTATELEDGERUTIL

/// @file mqbc_clusterstateledgerutil.h
///
/// @brief Provide utilities for BlazingMQ cluster state ledger protocol.
///
/// @bbref{mqbc::ClusterStateLedgerUtil} provides utilities for BlazingMQ
/// cluster state ledger, including how to read/write/validate a cluster state
/// ledger file.  Note that any ledger using this component *must* support
/// aliasing.
///
/// @see @bbref{mqbc::ClusterStateLedgerProtocol}

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbsi_ledger.h>
#include <mqbsi_log.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbc {

// ===============================
// struct ClusterStateLedgerUtilRc
// ===============================

/// This enum represents the return codes for the
/// @bbref{mqbc::ClusterStateLedgerUtil} component.
struct ClusterStateLedgerUtilRc {
    // TYPES
    enum Enum {

        /// @name ClusterStateLedgerUtilRc_Enum_Generic
        /// Generic
        /// @{

        /// Success.
        e_SUCCESS = 0,

        /// Unknown result.
        e_UNKNOWN = -1,

        /// @}

        /// @name ClusterStateLedgerUtilRc_Enum_File
        /// File operations
        /// @{

        /// Failure to open the log file.
        e_FILE_OPEN_FAILURE = -2,
        /// Failed to read bytes from the log file.
        e_BYTE_READ_FAILURE = -3,

        /// @}

        /// @name ClusterStateLedgerUtilRc_Enum_Header
        /// Header validity
        /// @{

        /// Record header is missing.
        e_MISSING_HEADER = -4,

        /// Invalid protocol version in the header.
        e_INVALID_PROTOCOL_VERSION = -5,

        /// Invalid log ID in the header.
        e_INVALID_LOG_ID = -6,

        /// Invalid header words in the header.
        e_INVALID_HEADER_WORDS = -7,

        /// Invalid record type in the header.
        e_INVALID_RECORD_TYPE = -8,

        /// Invalid leader advisory words in the header.
        e_INVALID_LEADER_ADVISORY_WORDS = -9,

        /// Invalid checksum over the record.
        e_INVALID_CHECKSUM = -10,

        /// @}

        /// @name ClusterStateLedgerUtilRc_Enum_Record
        /// Record read/write
        /// @{

        /// Failure to encode advisory to blob.
        e_ENCODING_FAILURE = -11,

        /// Failure to write advisory to ledger.
        e_WRITE_FAILURE = -12,

        /// Failure to alias the record.
        e_RECORD_ALIAS_FAILURE = -13,

        /// Failure to decode the cluster message.
        e_DECODE_FAILURE = -14

        /// @}
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
    /// representation of a @bbref{LogOpResult::Enum} value.
    static bsl::ostream& print(bsl::ostream&                  stream,
                               ClusterStateLedgerUtilRc::Enum value,
                               int                            level = 0,
                               int spacesPerLevel                   = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ClusterStateLedgerUtilRc::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ClusterStateLedgerUtilRc::Enum* out,
                          const bslstl::StringRef&        str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                  stream,
                         ClusterStateLedgerUtilRc::Enum value);

// =============================
// struct ClusterStateLedgerUtil
// =============================

/// Utilities for BlazingMQ cluster state ledger protocol.  Note that any
/// ledger using this compomemt *must* support aliasing.
struct ClusterStateLedgerUtil {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.CLUSTERSTATELEDGERUTIL");

  public:
    // FUNCTIONS

    /// Validate the specified file `header`.  If the optionally specified
    /// `expectedLogId` is set, also ensure that the `header` contains the
    /// `expectedLogId`.  Return 0 on success or a non-zero error value
    /// otherwise.
    static int validateFileHeader(
        const ClusterStateFileHeader& header,
        const mqbu::StorageKey&       expectedLogId = mqbu::StorageKey());

    /// Validate the specified record `header`.  Return 0 on success or a
    /// non-zero error value otherwise.
    static int validateRecordHeader(const ClusterStateRecordHeader& header);

    /// Load the ID of the log located at the specified `logPath` into the
    /// specfied `logId`.  Return 0 on success or a non-zero error value
    /// otherwise.
    static int extractLogId(mqbu::StorageKey*  logId,
                            const bsl::string& logPath);

    /// Validate that the specified `log` complies with
    /// `mqbc::ClusterStateLedgerProtocol` and populate the specified
    /// `offset` with the latest offset of the log.  Return 0 on success or
    /// a non-zero error value otherwise.  Behavior is undefined unless the
    /// `log` is opened.
    static int validateLog(mqbsi::Log::Offset*                offset,
                           const bsl::shared_ptr<mqbsi::Log>& log);

    /// Called when rolling over to a new log in the ledger, this method
    /// writes a new `mqbc::ClusterStateFileHeader` into the specified
    /// `ledger` associated with the new file having the specified `logId`.
    /// Return 0 on success or a non-zero error value otherwise.
    static int writeFileHeader(mqbsi::Ledger*          ledger,
                               const mqbu::StorageKey& logId);

    /// Append to the specified `blob` a record of the specified
    /// `recordType`, with the specified `sequenceNumber` and `timestamp`,
    /// and containing the message in the specified `clusterMessage`.
    /// Return 0 on success, and a non-zero error code otherwise.  The
    /// behavior is undefined unless the `clusterMessage` is instatntiated
    /// with the appropriate advisory.
    static int
    appendRecord(bdlbb::Blob*                               blob,
                 const bmqp_ctrlmsg::ClusterMessage&        clusterMessage,
                 const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
                 bsls::Types::Uint64                        timestamp,
                 ClusterStateRecordType::Enum               recordType);

    /// Load the cluster message recorded at the specified `recordId` with
    /// the specified `recordHeader` from the specified `ledger` into the
    /// specified `message`.  Return 0 on success, and a non-zero error code
    /// otherwise.
    static int loadClusterMessage(bmqp_ctrlmsg::ClusterMessage*   message,
                                  const mqbsi::Ledger&            ledger,
                                  const ClusterStateRecordHeader& recordHeader,
                                  const mqbsi::LedgerRecordId&    recordId);

    /// Load the cluster message recorded at the specified `recordId` from
    /// the specified `ledger` into the specified `message`.  Return 0 on
    /// success, and a non-zero error code otherwise.
    static int loadClusterMessage(bmqp_ctrlmsg::ClusterMessage* message,
                                  const mqbsi::Ledger&          ledger,
                                  const mqbsi::LedgerRecordId&  recordId);

    /// Load the cluster message at the optionally specified `offset` of the
    /// specified `record` with the specified `recordHeader` into the
    /// specified `message`.  Return 0 on success, and a non-zero error code
    /// otherwise.
    static int loadClusterMessage(bmqp_ctrlmsg::ClusterMessage*   message,
                                  const ClusterStateRecordHeader& recordHeader,
                                  const bdlbb::Blob&              record,
                                  int                             offset = 0);

    /// Load the cluster message at the optionally specified `offset` of the
    /// specified `record` into the specified `message`.  Return 0 on
    /// success, and a non-zero error code otherwise.
    static int loadClusterMessage(bmqp_ctrlmsg::ClusterMessage* message,
                                  const bdlbb::Blob&            record,
                                  int                           offset = 0);

    /// Return the size in bytes of the specified `header`.
    static bsls::Types::Int64
    recordSize(const ClusterStateRecordHeader& header);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------------
// struct ClusterStateLedgerUtil
// -----------------------------

inline bsls::Types::Int64
ClusterStateLedgerUtil::recordSize(const ClusterStateRecordHeader& header)
{
    return (header.headerWords() + header.leaderAdvisoryWords()) *
           bmqp::Protocol::k_WORD_SIZE;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mqbc::operator<<(bsl::ostream&                  stream,
                                      ClusterStateLedgerUtilRc::Enum value)
{
    return ClusterStateLedgerUtilRc::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
