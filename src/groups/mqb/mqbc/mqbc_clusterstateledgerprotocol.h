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

// mqbc_clusterstateledgerprotocol.h                                  -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERSTATELEDGERPROTOCOL
#define INCLUDED_MQBC_CLUSTERSTATELEDGERPROTOCOL

/// @file mqbc_clusterstateledgerprotocol.h
///
/// @brief Provide definitions for BlazingMQ cluster state ledger protocol
/// structures.
///
/// @bbref{mqbc::ClusterStateLedgerProtocol} provides definitions for a set of
/// structures defining the binary layout of the protocol messages used by
/// BlazingMQ cluster state ledger to persist messages on disk.

// MQB
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_protocol.h>  // for bmqp::Protocol::k_WORD_SIZE, etc.

// BDE
#include <bsl_ostream.h>
#include <bsla_annotations.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbc {

// =================================
// struct ClusterStateLedgerProtocol
// =================================

/// Namespace for protocol generic values and routines
struct ClusterStateLedgerProtocol {
    // CONSTANTS

    /// Version of the protocol.
    static const int k_VERSION = 1;
};

// =============================
// struct ClusterStateFileHeader
// =============================

/// This struct represents the header for a file in a cluster state ledger.
///
/// ClusterStateFileHeader structure datagram [8 bytes]:
///
/// ```
/// +---------------+---------------+---------------+---------------+
/// |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
/// +---------------+---------------+---------------+---------------+
/// |PV |HeaderWords|                  FileKey                      |
/// +---------------+---------------+---------------+---------------+
/// |            FileKey            |           Reserved            |
/// +---------------+---------------+---------------+---------------+
///
/// Protocol Version (PV).: Protocol Version (up to 4 concurrent versions)
/// HeaderWords...........: Total size (in words) of this header
/// FileKey...............: First 5 bytes of hashed file name
/// ```
struct ClusterStateFileHeader {
  private:
    // PRIVATE CONSTANTS
    static const int k_PROTOCOL_VERSION_NUM_BITS = 2;
    static const int k_HEADER_WORDS_NUM_BITS     = 6;

    static const int k_PROTOCOL_VERSION_START_IDX = 6;
    static const int k_HEADER_WORDS_START_IDX     = 0;

    static const int k_PROTOCOL_VERSION_MASK;
    static const int k_HEADER_WORDS_MASK;

  private:
    // DATA

    /// Protocol version and the total size (in words) of this header.
    unsigned char d_protocolVersionAndHeaderWords;

    /// File key, i.e., first 5 bytes of the hashed file name.
    char d_fileKey[mqbu::StorageKey::e_KEY_LENGTH_BINARY];

    /// Reserved.
    BSLA_MAYBE_UNUSED unsigned char d_reserved[2];

  public:
    // CONSTANTS

    /// Total size (in words) of this header.
    static const unsigned int k_HEADER_NUM_WORDS;

  public:
    // PUBLIC CLASS DATA

    /// Minimum size (bytes) of an `ClusterStateFileHeader` (that is
    /// sufficient to capture header words).  This value should *never*
    /// change.
    static const int k_MIN_HEADER_SIZE = 1;

  public:
    // CREATORS

    /// Create this object with `headerWords` set to appropriate value
    /// derived from `sizeof()` operator and the protocol version set to the
    /// current one.  All other fields are set to zero.
    explicit ClusterStateFileHeader();

    // MANIPULATORS

    /// Set the protocol version to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ClusterStateFileHeader& setProtocolVersion(unsigned char value);

    /// Set the total size (in words) of this header to the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.
    ClusterStateFileHeader& setHeaderWords(unsigned int value);

    /// Set the file key to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ClusterStateFileHeader& setFileKey(const mqbu::StorageKey& value);

    // ACCESSORS

    /// Return the protocol version.
    unsigned char protocolVersion() const;

    /// Return the total size (in words) of this header.
    unsigned int headerWords() const;

    /// Return the file key.
    const mqbu::StorageKey& fileKey() const;
};

// =============================
// struct ClusterStateRecordType
// =============================

/// This struct defines various types of cluster state records.
struct ClusterStateRecordType {
    // TYPES
    enum Enum {
        e_UNDEFINED = 0,
        e_SNAPSHOT  = 1,
        e_UPDATE    = 2,
        e_COMMIT    = 3,
        e_ACK       = 4
    };

    // PUBLIC CONSTANTS

    /// @note This value must always be equal to the lowest type in the enum
    ///       because it is being used as a lower bound to verify that a
    ///       `ClusterStateRecord`'s `type` field is a supported type.
    static const int k_LOWEST_SUPPORTED_TYPE = e_SNAPSHOT;

    /// @note This value must always be equal to the highest type in the enum
    ///       because it is being used as an upper bound to verify a
    ///       `ClusterStateRecord`'s `type` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_TYPE = e_ACK;

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// @bbref{ClusterStateRecordType::Enum} value.
    static bsl::ostream& print(bsl::ostream&                stream,
                               ClusterStateRecordType::Enum value,
                               int                          level = 0,
                               int spacesPerLevel                 = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << ClusterStateRecordType::toAscii(
    ///                                ClusterStateRecordType::e_SNAPSHOT);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// SNAPSHOT
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ClusterStateRecordType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                stream,
                         ClusterStateRecordType::Enum value);

// ===============================
// struct ClusterStateRecordHeader
// ===============================

/// This struct represents the header for each `ClusterStateRecord` present
/// in the cluster state ledger.
///
/// ClusterStateRecordHeader structure datagram [24 bytes]:
///
/// ```
/// +---------------+---------------+---------------+---------------+
/// |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
/// +---------------+---------------+---------------+---------------+
/// +---------------+---------------+---------------+---------------+
/// |   HW  |  RT   |                   Reserved                    |
/// +---------------+---------------+---------------+---------------+
/// |                       LeaderAdvisoryWords                     |
/// +---------------+---------------+---------------+---------------+
/// |                    Elector Term Upper Bits                    |
/// +---------------+---------------+---------------+---------------+
/// |                    Elector Term Lower Bits                    |
/// +---------------+---------------+---------------+---------------+
/// |                  Sequence Number Upper Bits                   |
/// +---------------+---------------+---------------+---------------+
/// |                  Sequence Number Lower Bits                   |
/// +---------------+---------------+---------------+---------------+
/// |                     Timestamp Upper Bits                      |
/// +---------------+---------------+---------------+---------------+
/// |                     Timestamp Lower Bits                      |
/// +---------------+---------------+---------------+---------------+
///
/// Header Words (HW)..........: Total size (in words) of this header.
/// Record Type (RT)...........: Type of the record. Currently available
///                              types are Snapshot Record, Update Record,
///                              and Commit Record.
/// LeaderAdvisoryWords........: Total size (in words) of the leader
///                              advisory representing the cluster state
///                              updates.
/// Elector Term Upper Bits....: Upper 32 bits of the leader elector term.
/// Elector Term Lower Bits....: Lower 32 bits of the leader elector term.
/// Sequence Number Upper Bits.: Upper 32 bits of the sequence number.
/// Sequence Number Lower Bits.: Lower 32 bits of the sequence number.
/// Timestamp Upper Bits.......: Upper 32 bits of the timestamp.
/// Timestamp Lower Bits.......: Lower 32 bits of the timestamp.
/// ```
///
/// @note A BER-encoded leader advisory message, followd by a CRC32-C computed
///       over the header and payload, will follow this header.  The total size
///       of the leader advisory message and CRC32-C is `LeaderAdvisoryWords`.
///
struct ClusterStateRecordHeader {
  private:
    // PRIVATE CONSTANTS
    static const int k_HEADER_WORDS_NUM_BITS = 4;
    static const int k_RECORD_TYPE_NUM_BITS  = 4;

    static const int k_HEADER_WORDS_START_IDX = 4;
    static const int k_RECORD_TYPE_START_IDX  = 0;

    static const int k_HEADER_WORDS_MASK;
    static const int k_RECORD_TYPE_MASK;

  private:
    // DATA

    /// Total size (in words) of this header and record type (Update, Snapshot,
    /// Commit).
    unsigned char d_headerWordsAndRecordType;

    /// Reserved.
    BSLA_MAYBE_UNUSED unsigned char d_reserved[3];

    /// Total size (in words) of the leader advisory representing the cluster
    /// state updates.
    bdlb::BigEndianUint32 d_leaderAdvisoryWords;

    /// Upper 32 bits of the leader elector term.
    bdlb::BigEndianUint32 d_electorTermUpperBits;

    /// Lower 32 bits of the leader elector term.
    bdlb::BigEndianUint32 d_electorTermLowerBits;

    /// Upper 32 bits of the sequence number.
    bdlb::BigEndianUint32 d_seqNumUpperBits;

    /// Lower 32 bits of the sequence number.
    bdlb::BigEndianUint32 d_seqNumLowerBits;

    /// Upper 32 bits of the timestamp.
    bdlb::BigEndianUint32 d_timestampUpperBits;

    /// Lower 32 bits of the timestamp.
    bdlb::BigEndianUint32 d_timestampLowerBits;

  public:
    // CONSTANTS

    /// Total size (in words) of this header.
    static const unsigned int k_HEADER_NUM_WORDS;

  public:
    // PUBLIC CLASS DATA

    /// Minimum size (bytes) of an `ClusterStateRecordHeader` (that is
    /// sufficient to capture header words).  This value should *never*
    /// change.
    static const int k_MIN_HEADER_SIZE = 1;

  public:
    // CREATORS

    /// Create this object with `headerWords` set to appropriate value
    /// derived from sizeof() operator.  All other fields are set to zero.
    explicit ClusterStateRecordHeader();

    // MANIPULATORS

    /// Set the total size (in words) of this header to the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.
    ClusterStateRecordHeader& setHeaderWords(unsigned int value);

    /// Set the record type to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ClusterStateRecordHeader&
    setRecordType(ClusterStateRecordType::Enum value);

    /// Set the total size (in words) of the leader advisory to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    ClusterStateRecordHeader& setLeaderAdvisoryWords(unsigned int value);

    /// Set the leader elector term to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ClusterStateRecordHeader& setElectorTerm(bsls::Types::Uint64 value);

    /// Set the sequence number to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ClusterStateRecordHeader& setSequenceNumber(bsls::Types::Uint64 value);

    /// Set the timestamp to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ClusterStateRecordHeader& setTimestamp(bsls::Types::Uint64 value);

    // ACCESSORS

    /// Return the total size (in words) of this header.
    unsigned int headerWords() const;

    /// Return the record type.
    ClusterStateRecordType::Enum recordType() const;

    /// Return the total size (in words) of the leader advisory.
    unsigned int leaderAdvisoryWords() const;

    /// Return the leader elector term.
    bsls::Types::Uint64 electorTerm() const;

    /// Return the sequence number.
    bsls::Types::Uint64 sequenceNumber() const;

    /// Return the timestamp.
    bsls::Types::Uint64 timestamp() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------------
// struct ClusterStatefileHeader
// -----------------------------

// CREATORS
inline ClusterStateFileHeader::ClusterStateFileHeader()
{
    bsl::memset(this, 0, sizeof(ClusterStateFileHeader));
    setProtocolVersion(ClusterStateLedgerProtocol::k_VERSION);
    setHeaderWords(sizeof(ClusterStateFileHeader) /
                   bmqp::Protocol::k_WORD_SIZE);
    setFileKey(mqbu::StorageKey::k_NULL_KEY);
}

// MANIPULATORS
inline ClusterStateFileHeader&
ClusterStateFileHeader::setProtocolVersion(unsigned char value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value <= (1 << k_PROTOCOL_VERSION_NUM_BITS) - 1);

    d_protocolVersionAndHeaderWords = static_cast<unsigned char>(
        (d_protocolVersionAndHeaderWords & k_HEADER_WORDS_MASK) |
        (value << k_PROTOCOL_VERSION_START_IDX));

    return *this;
}

inline ClusterStateFileHeader&
ClusterStateFileHeader::setHeaderWords(unsigned int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_protocolVersionAndHeaderWords = static_cast<unsigned char>(
        (d_protocolVersionAndHeaderWords & k_PROTOCOL_VERSION_MASK) |
        (value << k_HEADER_WORDS_START_IDX));

    return *this;
}

inline ClusterStateFileHeader&
ClusterStateFileHeader::setFileKey(const mqbu::StorageKey& value)
{
    bsl::memcpy(d_fileKey,
                value.data(),
                mqbu::StorageKey::e_KEY_LENGTH_BINARY);

    return *this;
}

// ACCESSORS
inline unsigned char ClusterStateFileHeader::protocolVersion() const
{
    return static_cast<unsigned char>(
        (d_protocolVersionAndHeaderWords & k_PROTOCOL_VERSION_MASK) >>
        k_PROTOCOL_VERSION_START_IDX);
}

inline unsigned int ClusterStateFileHeader::headerWords() const
{
    return (d_protocolVersionAndHeaderWords & k_HEADER_WORDS_MASK) >>
           k_HEADER_WORDS_START_IDX;
}

inline const mqbu::StorageKey& ClusterStateFileHeader::fileKey() const
{
    return reinterpret_cast<const mqbu::StorageKey&>(d_fileKey);
}

// -------------------------------
// struct ClusterStateRecordHeader
// -------------------------------

// CREATORS
inline ClusterStateRecordHeader::ClusterStateRecordHeader()
{
    bsl::memset(this, 0, sizeof(ClusterStateRecordHeader));
    setHeaderWords(sizeof(ClusterStateRecordHeader) /
                   bmqp::Protocol::k_WORD_SIZE);
}

// MANIPULATORS
inline ClusterStateRecordHeader&
ClusterStateRecordHeader::setHeaderWords(unsigned int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_headerWordsAndRecordType = static_cast<unsigned char>(
        (d_headerWordsAndRecordType & k_RECORD_TYPE_MASK) |
        (value << k_HEADER_WORDS_START_IDX));

    return *this;
}

inline ClusterStateRecordHeader&
ClusterStateRecordHeader::setRecordType(ClusterStateRecordType::Enum value)
{
    d_headerWordsAndRecordType = static_cast<unsigned char>(
        (d_headerWordsAndRecordType & k_HEADER_WORDS_MASK) |
        (value << k_RECORD_TYPE_START_IDX));

    return *this;
}

inline ClusterStateRecordHeader&
ClusterStateRecordHeader::setLeaderAdvisoryWords(unsigned int value)
{
    d_leaderAdvisoryWords = value;
    return *this;
}

inline ClusterStateRecordHeader&
ClusterStateRecordHeader::setElectorTerm(bsls::Types::Uint64 value)
{
    bmqp::Protocol::split(&d_electorTermUpperBits,
                          &d_electorTermLowerBits,
                          value);
    return *this;
}

inline ClusterStateRecordHeader&
ClusterStateRecordHeader::setSequenceNumber(bsls::Types::Uint64 value)
{
    bmqp::Protocol::split(&d_seqNumUpperBits, &d_seqNumLowerBits, value);
    return *this;
}

inline ClusterStateRecordHeader&
ClusterStateRecordHeader::setTimestamp(bsls::Types::Uint64 value)
{
    bmqp::Protocol::split(&d_timestampUpperBits, &d_timestampLowerBits, value);
    return *this;
}

// ACCESSORS
inline unsigned int ClusterStateRecordHeader::headerWords() const
{
    return (d_headerWordsAndRecordType & k_HEADER_WORDS_MASK) >>
           k_HEADER_WORDS_START_IDX;
}

inline ClusterStateRecordType::Enum
ClusterStateRecordHeader::recordType() const
{
    return static_cast<ClusterStateRecordType::Enum>(
        (d_headerWordsAndRecordType & k_RECORD_TYPE_MASK) >>
        k_RECORD_TYPE_START_IDX);
}

inline unsigned int ClusterStateRecordHeader::leaderAdvisoryWords() const
{
    return d_leaderAdvisoryWords;
}

inline bsls::Types::Uint64 ClusterStateRecordHeader::electorTerm() const
{
    return bmqp::Protocol::combine(d_electorTermUpperBits,
                                   d_electorTermLowerBits);
}

inline bsls::Types::Uint64 ClusterStateRecordHeader::sequenceNumber() const
{
    return bmqp::Protocol::combine(d_seqNumUpperBits, d_seqNumLowerBits);
}

inline bsls::Types::Uint64 ClusterStateRecordHeader::timestamp() const
{
    return bmqp::Protocol::combine(d_timestampUpperBits, d_timestampLowerBits);
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mqbc::operator<<(bsl::ostream& stream,
                                      mqbc::ClusterStateRecordType::Enum value)
{
    return mqbc::ClusterStateRecordType::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
