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

// mqbc_clusterstateledgerutil.cpp                                    -*-C++-*-
#include <mqbc_clusterstateledgerutil.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>

// MWC
#include <mwctsk_alarmlog.h>
#include <mwcu_blob.h>
#include <mwcu_blobobjectproxy.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlb_print.h>
#include <bdlb_scopeexit.h>
#include <bdlb_string.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_iosfwd.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// SYS
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace BloombergLP {
namespace mqbc {

// -------------------------------
// struct ClusterStateLedgerUtilRc
// -------------------------------

bsl::ostream&
ClusterStateLedgerUtilRc::print(bsl::ostream&                  stream,
                                ClusterStateLedgerUtilRc::Enum value,
                                int                            level,
                                int                            spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ClusterStateLedgerUtilRc::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
ClusterStateLedgerUtilRc::toAscii(ClusterStateLedgerUtilRc::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SUCCESS)
        CASE(UNKNOWN)
        CASE(FILE_OPEN_FAILURE)
        CASE(BYTE_READ_FAILURE)
        CASE(MISSING_HEADER)
        CASE(INVALID_PROTOCOL_VERSION)
        CASE(INVALID_LOG_ID)
        CASE(INVALID_HEADER_WORDS)
        CASE(INVALID_RECORD_TYPE)
        CASE(INVALID_LEADER_ADVISORY_WORDS)
        CASE(INVALID_CHECKSUM)
        CASE(ENCODING_FAILURE)
        CASE(WRITE_FAILURE)
        CASE(RECORD_ALIAS_FAILURE)
        CASE(DECODE_FAILURE)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ClusterStateLedgerUtilRc::fromAscii(ClusterStateLedgerUtilRc::Enum* out,
                                         const bslstl::StringRef&        str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(                                       \
            toAscii(ClusterStateLedgerUtilRc::e_##M),                         \
            str.data(),                                                       \
            str.length())) {                                                  \
        *out = ClusterStateLedgerUtilRc::e_##M;                               \
        return true;                                                          \
    }

    CHECKVALUE(SUCCESS)
    CHECKVALUE(UNKNOWN)
    CHECKVALUE(FILE_OPEN_FAILURE)
    CHECKVALUE(BYTE_READ_FAILURE)
    CHECKVALUE(MISSING_HEADER)
    CHECKVALUE(INVALID_PROTOCOL_VERSION)
    CHECKVALUE(INVALID_LOG_ID)
    CHECKVALUE(INVALID_HEADER_WORDS)
    CHECKVALUE(INVALID_RECORD_TYPE)
    CHECKVALUE(INVALID_LEADER_ADVISORY_WORDS)
    CHECKVALUE(INVALID_CHECKSUM)
    CHECKVALUE(ENCODING_FAILURE)
    CHECKVALUE(WRITE_FAILURE)
    CHECKVALUE(RECORD_ALIAS_FAILURE)
    CHECKVALUE(DECODE_FAILURE)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -----------------------------
// struct ClusterStateLedgerUtil
// -----------------------------

// FUNCTIONS
int ClusterStateLedgerUtil::validateFileHeader(
    const ClusterStateFileHeader& header,
    const mqbu::StorageKey&       expectedLogId)
{
    if (static_cast<const int>(header.protocolVersion()) !=
        ClusterStateLedgerProtocol::k_VERSION) {
        return ClusterStateLedgerUtilRc::e_INVALID_PROTOCOL_VERSION;  // RETURN
    }
    if (header.fileKey().isNull()) {
        return ClusterStateLedgerUtilRc::e_INVALID_LOG_ID;  // RETURN
    }
    if (header.headerWords() < ClusterStateFileHeader::k_MIN_HEADER_SIZE) {
        return ClusterStateLedgerUtilRc::e_INVALID_HEADER_WORDS;  // RETURN
    }

    // Ensure that the 'header' contains the 'expectedLogId', if specified.
    if (!expectedLogId.isNull() && (header.fileKey() != expectedLogId)) {
        return ClusterStateLedgerUtilRc::e_INVALID_LOG_ID;  // RETURN
    }

    return ClusterStateLedgerUtilRc::e_SUCCESS;
}

int ClusterStateLedgerUtil::validateRecordHeader(
    const ClusterStateRecordHeader& header)
{
    if (header.headerWords() < ClusterStateRecordHeader::k_MIN_HEADER_SIZE) {
        return ClusterStateLedgerUtilRc::e_INVALID_HEADER_WORDS;  // RETURN
    }
    if (header.recordType() == ClusterStateRecordType::e_UNDEFINED ||
        header.recordType() <
            ClusterStateRecordType::k_LOWEST_SUPPORTED_TYPE ||
        header.recordType() >
            ClusterStateRecordType::k_HIGHEST_SUPPORTED_TYPE) {
        return ClusterStateLedgerUtilRc::e_INVALID_RECORD_TYPE;  // RETURN
    }
    if (header.leaderAdvisoryWords() == 0) {
        return ClusterStateLedgerUtilRc::
            e_INVALID_LEADER_ADVISORY_WORDS;  // RETURN
    }

    return ClusterStateLedgerUtilRc::e_SUCCESS;
}

int ClusterStateLedgerUtil::extractLogId(mqbu::StorageKey*  logId,
                                         const bsl::string& logPath)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(logId);

    BALL_LOG_DEBUG << "Extracting log id from '" << logPath << "'";

    // Open file
    int fd = ::open(logPath.c_str(),
                    O_RDONLY,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        BALL_LOG_ERROR << "'open()' failure for file [path: " << logPath
                       << ", errno: " << errno << " (" << bsl::strerror(errno)
                       << ")]";
        return ClusterStateLedgerUtilRc::e_FILE_OPEN_FAILURE;  // RETURN
    }

    bdlb::ScopeExitAny guard(bdlf::BindUtil::bind(::close, fd));

    // Read file header
    ClusterStateFileHeader header;
    int                    length  = sizeof(ClusterStateFileHeader);
    int                    numRead = 0;
    int                    rc      = ClusterStateLedgerUtilRc::e_UNKNOWN;
    do {
        rc = ::read(fd, reinterpret_cast<char*>(&header) + numRead, length);
        if (rc < 0) {
            BALL_LOG_ERROR << "'read()' failure for file [path: " << logPath
                           << ", errno: " << errno << " ("
                           << bsl::strerror(errno) << ")]";
            return ClusterStateLedgerUtilRc::e_BYTE_READ_FAILURE;  // RETURN
        }
        numRead += rc;
        length -= rc;
    } while (length > 0);

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(length == 0);

    // Validate file header
    rc = validateFileHeader(header);
    if (rc != 0) {
        return rc;  // RETURN
    }

    // Extract log ID
    *logId = header.fileKey();
    BALL_LOG_DEBUG << "LogId extracted: " << *logId;

    return ClusterStateLedgerUtilRc::e_SUCCESS;
}

int ClusterStateLedgerUtil::validateLog(mqbsi::Log::Offset* offset,
                                        const bsl::shared_ptr<mqbsi::Log>& log)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(log);
    BSLS_ASSERT_SAFE(log->supportsAliasing());

    // Read and validate file header
    ClusterStateFileHeader* fileHeader;
    int rc = log->alias(reinterpret_cast<void**>(&fileHeader),
                        sizeof(ClusterStateFileHeader),
                        0);
    if (rc != 0) {
        return rc * 100 + ClusterStateLedgerUtilRc::e_RECORD_ALIAS_FAILURE;
        // RETURN
    }

    rc = validateFileHeader(*fileHeader, log->logConfig().logId());
    if (rc != 0) {
        return rc;  // RETURN
    }

    // Read and validate each record and header in the log
    ClusterStateRecordHeader* recHeader;
    mqbsi::Log::Offset        currOffset = fileHeader->headerWords() *
                                    bmqp::Protocol::k_WORD_SIZE;
    while (static_cast<mqbsi::Log::UnsignedOffset>(currOffset) +
               sizeof(ClusterStateRecordHeader) <=
           static_cast<bsls::Types::Uint64>(log->totalNumBytes())) {
        rc = log->alias(reinterpret_cast<void**>(&recHeader),
                        sizeof(ClusterStateRecordHeader),
                        currOffset);
        if (rc != 0) {
            return rc * 100 + ClusterStateLedgerUtilRc::e_RECORD_ALIAS_FAILURE;
            // RETURN
        }

        rc = validateRecordHeader(*recHeader);
        if (rc != 0) {
            break;  // BREAK
        }

        // Validate CRC32-C on header + payload
        const bsls::Types::Int64 recordSize =
            ClusterStateLedgerUtil::recordSize(*recHeader);
        bdlbb::Blob blob;
        rc = log->alias(&blob, static_cast<int>(recordSize), currOffset);
        if (rc != 0) {
            BALL_LOG_ERROR << "Unable to read header and record in log '"
                           << log->logConfig().location() << "' at offset "
                           << currOffset << ". [rc:" << rc << "]";
            return rc;  // RETURN
        }

        bdlb::BigEndianUint32 crc32cExpected;
        bdlbb::BlobUtil::copy(reinterpret_cast<char*>(&crc32cExpected),
                              blob,
                              blob.length() - sizeof(bdlb::BigEndianUint32),
                              sizeof(bdlb::BigEndianUint32));
        blob.setLength(blob.length() - sizeof(bdlb::BigEndianUint32));

        bdlb::BigEndianUint32 crc32cComputed;
        crc32cComputed = bmqp::Crc32c::calculate(blob);
        if (crc32cComputed != crc32cExpected) {
            MWCTSK_ALARMLOG_ALARM("CLUSTER")
                << "CSL Recovery: CRC mismatch for record with sequenceNumber "
                << "[" << recHeader->sequenceNumber() << "] in file '"
                << log->logConfig().location() << "' with fileKey ["
                << fileHeader->fileKey() << "] at offset " << currOffset << "."
                << " CRC32-C Expected: " << crc32cExpected << "."
                << " CRC32-C Computed: " << crc32cComputed << "."
                << " ClusterStateRecordHeader: [headerWords: "
                << recHeader->headerWords()
                << ", recordType: " << recHeader->recordType()
                << ", leaderAdvisoryWords: "
                << recHeader->leaderAdvisoryWords()
                << ", electorTerm: " << recHeader->electorTerm()
                << ", sequenceNumber: " << recHeader->sequenceNumber()
                << ", timestamp: " << recHeader->timestamp() << "]"
                << MWCTSK_ALARMLOG_END;
            return ClusterStateLedgerUtilRc::e_INVALID_CHECKSUM;  // RETURN
        }

        currOffset += recordSize;
    }

    *offset = currOffset;

    return ClusterStateLedgerUtilRc::e_SUCCESS;
}

int ClusterStateLedgerUtil::writeFileHeader(mqbsi::Ledger*          ledger,
                                            const mqbu::StorageKey& logId)
{
    ClusterStateFileHeader fileHeader;
    const int headerWords = ClusterStateFileHeader::k_HEADER_NUM_WORDS;
    fileHeader.setProtocolVersion(mqbc::ClusterStateLedgerProtocol::k_VERSION)
        .setHeaderWords(headerWords)
        .setFileKey(logId);

    mqbsi::LedgerRecordId recordId;
    int                   rc = ledger->writeRecord(&recordId,
                                 static_cast<void*>(&fileHeader),
                                 0,
                                 sizeof(ClusterStateFileHeader));
    if (rc != 0) {
        return rc * 100 + ClusterStateLedgerUtilRc::e_WRITE_FAILURE;  // RETURN
    }

    return ClusterStateLedgerUtilRc::e_SUCCESS;
}

int ClusterStateLedgerUtil::appendRecord(
    bdlbb::Blob*                               blob,
    const bmqp_ctrlmsg::ClusterMessage&        clusterMessage,
    const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
    bsls::Types::Uint64                        timestamp,
    ClusterStateRecordType::Enum               recordType)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);
    BSLS_ASSERT_SAFE(!clusterMessage.choice().isUndefinedValue());

    // Insert record header
    //
    // Note: We use placement new to create the object directly in the blob
    // buffer, because we need to set leaderAdvisoryWords at a later time.
    blob->setLength(sizeof(ClusterStateRecordHeader));
    BSLS_ASSERT_SAFE(blob->numDataBuffers() == 1 &&
                     "The buffers allocated by the supplied bufferFactory "
                     "are too small");

    ClusterStateRecordHeader* recordHeader = new (blob->buffer(0).data())
        ClusterStateRecordHeader;
    const int headerWords = ClusterStateRecordHeader::k_HEADER_NUM_WORDS;
    recordHeader->setHeaderWords(headerWords)
        .setRecordType(recordType)
        .setElectorTerm(sequenceNumber.electorTerm())
        .setSequenceNumber(sequenceNumber.sequenceNumber())
        .setTimestamp(timestamp);

    // Encode and append 'advisory' to the blob
    mwcu::MemOutStream os;
    int                rc = bmqp::ProtocolUtil::encodeMessage(os,
                                               blob,
                                               clusterMessage,
                                               bmqp::EncodingType::e_BER);
    if (rc != 0) {
        return rc * 100 +
               ClusterStateLedgerUtilRc::e_ENCODING_FAILURE;  // RETURN
    }

    // Append padding
    const int blobNumWords = bmqp::ProtocolUtil::appendPadding(blob,
                                                               blob->length());

    // Append CRC32-C
    BSLMF_ASSERT(
        (sizeof(bdlb::BigEndianUint32) % bmqp::Protocol::k_WORD_SIZE) == 0);
    recordHeader->setLeaderAdvisoryWords(
        blobNumWords +
        (sizeof(bdlb::BigEndianUint32) / bmqp::Protocol::k_WORD_SIZE) -
        headerWords);
    bdlb::BigEndianUint32 crc32c;
    crc32c = bmqp::Crc32c::calculate(*blob);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&crc32c),
                            sizeof(bdlb::BigEndianUint32));

    return ClusterStateLedgerUtilRc::e_SUCCESS;
}

int ClusterStateLedgerUtil::loadClusterMessage(
    bmqp_ctrlmsg::ClusterMessage*   message,
    const mqbsi::Ledger&            ledger,
    const ClusterStateRecordHeader& recordHeader,
    const mqbsi::LedgerRecordId&    recordId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(message);
    BSLS_ASSERT_SAFE(ledger.supportsAliasing());

    char*                 entry;
    mqbsi::LedgerRecordId adjustedRecordId(
        recordId.logId(),
        recordId.offset() +
            recordHeader.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    const int msgLen = recordHeader.leaderAdvisoryWords() *
                       bmqp::Protocol::k_WORD_SIZE;
    int rc = ledger.aliasRecord(reinterpret_cast<void**>(&entry),
                                msgLen,
                                adjustedRecordId);
    if (rc != 0) {
        return rc * 100 + ClusterStateLedgerUtilRc::e_RECORD_ALIAS_FAILURE;
        // RETURN
    }

    mwcu::MemOutStream os;
    rc = bmqp::ProtocolUtil::decodeMessage(os,
                                           message,
                                           entry,
                                           0,
                                           msgLen,
                                           bmqp::EncodingType::e_BER);
    if (rc != 0) {
        return rc * 100 +
               ClusterStateLedgerUtilRc::e_DECODE_FAILURE;  // RETURN
    }

    return ClusterStateLedgerUtilRc::e_SUCCESS;
}

int ClusterStateLedgerUtil::loadClusterMessage(
    bmqp_ctrlmsg::ClusterMessage* message,
    const mqbsi::Ledger&          ledger,
    const mqbsi::LedgerRecordId&  recordId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(ledger.supportsAliasing());

    ClusterStateRecordHeader* header;
    int rc = ledger.aliasRecord(reinterpret_cast<void**>(&header),
                                sizeof(ClusterStateRecordHeader),
                                recordId);
    if (rc != 0) {
        return rc * 100 +
               ClusterStateLedgerUtilRc::e_RECORD_ALIAS_FAILURE;  // RETURN
    }

    rc = validateRecordHeader(*header);
    if (rc != 0) {
        return rc;  // RETURN
    }

    return loadClusterMessage(message, ledger, *header, recordId);
}

int ClusterStateLedgerUtil::loadClusterMessage(
    bmqp_ctrlmsg::ClusterMessage*   message,
    const ClusterStateRecordHeader& recordHeader,
    const bdlbb::Blob&              record,
    int                             offset)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(message);

    const int recordHeaderSize = recordHeader.headerWords() *
                                 bmqp::Protocol::k_WORD_SIZE;

    mwcu::MemOutStream os;
    int                rc = bmqp::ProtocolUtil::decodeMessage(os,
                                               message,
                                               record,
                                               offset + recordHeaderSize,
                                               bmqp::EncodingType::e_BER);
    if (rc != 0) {
        return rc * 100 +
               ClusterStateLedgerUtilRc::e_DECODE_FAILURE;  // RETURN
    }

    return ClusterStateLedgerUtilRc::e_SUCCESS;
}

int ClusterStateLedgerUtil::loadClusterMessage(
    bmqp_ctrlmsg::ClusterMessage* message,
    const bdlbb::Blob&            record,
    int                           offset)
{
    mwcu::BlobPosition headerPosition;
    int rc = mwcu::BlobUtil::findOffsetSafe(&headerPosition, record, offset);
    if (rc != 0) {
        return rc * 100 +
               ClusterStateLedgerUtilRc::e_MISSING_HEADER;  // RETURN
    }

    mwcu::BlobObjectProxy<ClusterStateRecordHeader> header(
        &record,
        headerPosition,
        -ClusterStateRecordHeader::k_MIN_HEADER_SIZE,
        true,    // read
        false);  // write
    if (!header.isSet()) {
        return ClusterStateLedgerUtilRc::e_MISSING_HEADER;  // RETURN
    }
    header.resize(header->headerWords() * bmqp::Protocol::k_WORD_SIZE);
    if (!header.isSet()) {
        return ClusterStateLedgerUtilRc::e_MISSING_HEADER;  // RETURN
    }

    rc = validateRecordHeader(*header);
    if (rc != 0) {
        return rc;  // RETURN
    }

    return loadClusterMessage(message, *header, record, offset);
}

}  // close package namespace
}  // close enterprise namespace
