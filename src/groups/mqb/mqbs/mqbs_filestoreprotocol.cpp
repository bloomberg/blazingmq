// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbs_filestoreprotocol.cpp                                         -*-C++-*-
#include <mqbs_filestoreprotocol.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_bitmaskutil.h>
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdlb_tokenizer.h>
#include <bslim_printer.h>
#include <bslmf_assert.h>
#include <bsls_alignmentfromtype.h>

namespace BloombergLP {
namespace mqbs {

namespace {
// Compile-time assertions for alignment of various structs in the protocol
BSLMF_ASSERT(4 == bsls::AlignmentFromType<FileHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<JournalFileHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<QueueRecordHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<RecordHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<MessageRecord>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<ConfirmRecord>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<DeletionRecord>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<QueueOpRecord>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<JournalOpRecord>::VALUE);
BSLMF_ASSERT(1 == bsls::AlignmentFromType<DataFileHeader>::VALUE);
BSLMF_ASSERT(1 == bsls::AlignmentFromType<QlistFileHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<DataHeader>::VALUE);

// Compile-time assertions for size of various structs in the protocol
BSLMF_ASSERT(0 == FileStoreProtocol::k_JOURNAL_RECORD_SIZE % 4);
BSLMF_ASSERT(0 == sizeof(FileHeader) % 8);
BSLMF_ASSERT(0 == sizeof(DataFileHeader) % 8);
BSLMF_ASSERT(0 == sizeof(QlistFileHeader) % 4);
BSLMF_ASSERT(0 == sizeof(JournalFileHeader) % 4);
BSLMF_ASSERT(20 == sizeof(RecordHeader));
BSLMF_ASSERT(60 == sizeof(MessageRecord));
BSLMF_ASSERT(0 == sizeof(MessageRecord) % 4);
BSLMF_ASSERT(60 == sizeof(ConfirmRecord));
BSLMF_ASSERT(0 == sizeof(ConfirmRecord) % 4);
BSLMF_ASSERT(60 == sizeof(DeletionRecord));
BSLMF_ASSERT(0 == sizeof(DeletionRecord) % 4);
BSLMF_ASSERT(60 == sizeof(QueueOpRecord));
BSLMF_ASSERT(0 == sizeof(QueueOpRecord) % 4);
BSLMF_ASSERT(60 == sizeof(JournalOpRecord));
BSLMF_ASSERT(0 == sizeof(JournalOpRecord) % 4);
BSLMF_ASSERT(0 == sizeof(DataHeader) % 4);
BSLMF_ASSERT(2 == sizeof(short));

BSLMF_ASSERT(FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD <=
             MessageRecord::k_REFCOUNT_MAX_VALUE);
}  // close unnamed namespace

const unsigned int FileHeader::k_MAGIC1;
const unsigned int FileHeader::k_MAGIC2;

/// Force variables/symbols definition so they can be used in other files
const int FileStoreProtocol::k_VERSION;

// ------------------------
// struct FileStoreProtocol
// ------------------------

const char* FileStoreProtocol::k_DATA_FILE_EXTENSION(".bmq_data");
const char* FileStoreProtocol::k_JOURNAL_FILE_EXTENSION(".bmq_journal");
const char* FileStoreProtocol::k_QLIST_FILE_EXTENSION(".bmq_qlist");
const char* FileStoreProtocol::k_COMMON_FILE_EXTENSION_PREFIX(".bmq_");
const char* FileStoreProtocol::k_COMMON_FILE_PREFIX("bmq_");

// --------------
// struct Bitness
// --------------

bsl::ostream& Bitness::print(bsl::ostream& stream,
                             Bitness::Enum value,
                             int           level,
                             int           spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << Bitness::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* Bitness::toAscii(Bitness::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(32)
        CASE(64)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ---------------
// struct FileType
// ---------------

bsl::ostream& FileType::print(bsl::ostream&  stream,
                              FileType::Enum value,
                              int            level,
                              int            spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << FileType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* FileType::toAscii(FileType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(DATA)
        CASE(JOURNAL)
        CASE(QLIST)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -----------------
// struct FileHeader
// -----------------

const int FileHeader::k_PROTOCOL_VERSION_MASK = bdlb::BitMaskUtil::one(
    FileHeader::k_PROTOCOL_VERSION_START_IDX,
    FileHeader::k_PROTOCOL_VERSION_NUM_BITS);

const int FileHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    FileHeader::k_HEADER_WORDS_START_IDX,
    FileHeader::k_HEADER_WORDS_NUM_BITS);

const int FileHeader::k_FILE_TYPE_MASK = bdlb::BitMaskUtil::one(
    FileHeader::k_FILE_TYPE_START_IDX,
    FileHeader::k_FILE_TYPE_NUM_BITS);

const int FileHeader::k_BITNESS_MASK = bdlb::BitMaskUtil::one(
    FileHeader::k_BITNESS_START_IDX,
    FileHeader::k_BITNESS_NUM_BITS);

// ----------------------
// struct DataHeaderFlags
// ----------------------

bsl::ostream& DataHeaderFlags::print(bsl::ostream&         stream,
                                     DataHeaderFlags::Enum value,
                                     int                   level,
                                     int                   spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << DataHeaderFlags::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* DataHeaderFlags::toAscii(DataHeaderFlags::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(MESSAGE_PROPERTIES)
        CASE(UNUSED2)
        CASE(UNUSED3)
        CASE(UNUSED4)
        CASE(UNUSED5)
        CASE(UNUSED6)
        CASE(UNUSED7)
        CASE(UNUSED8)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool DataHeaderFlags::fromAscii(DataHeaderFlags::Enum*   out,
                                const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(DataHeaderFlags::e_##M),       \
                                       str.data(),                            \
                                       static_cast<int>(str.length()))) {     \
        *out = DataHeaderFlags::e_##M;                                        \
        return true;                                                          \
    }

    CHECKVALUE(MESSAGE_PROPERTIES)
    CHECKVALUE(UNUSED2)
    CHECKVALUE(UNUSED3)
    CHECKVALUE(UNUSED4)
    CHECKVALUE(UNUSED5)
    CHECKVALUE(UNUSED6)
    CHECKVALUE(UNUSED7)
    CHECKVALUE(UNUSED8)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -------------------------
// struct DataHeaderFlagUtil
// -------------------------

bool DataHeaderFlagUtil::isValid(bsl::ostream& errorDescription, int flags)
{
    if (isSet(flags, DataHeaderFlags::e_UNUSED3) ||
        isSet(flags, DataHeaderFlags::e_UNUSED4) ||
        isSet(flags, DataHeaderFlags::e_UNUSED5) ||
        isSet(flags, DataHeaderFlags::e_UNUSED6) ||
        isSet(flags, DataHeaderFlags::e_UNUSED7)) {
        errorDescription << "UNUSED flags are invalid.";
        return false;  // RETURN
    }

    return true;
}

bsl::ostream& DataHeaderFlagUtil::prettyPrint(bsl::ostream& stream, int flags)
{
#define CHECKVALUE(M)                                                         \
    if (flags & DataHeaderFlags::e_##M) {                                     \
        stream << (first ? "" : ",")                                          \
               << DataHeaderFlags::toAscii(DataHeaderFlags::e_##M);           \
        first = false;                                                        \
    }

    bool first = true;

    CHECKVALUE(MESSAGE_PROPERTIES)
    CHECKVALUE(UNUSED2)
    CHECKVALUE(UNUSED3)
    CHECKVALUE(UNUSED4)
    CHECKVALUE(UNUSED5)
    CHECKVALUE(UNUSED6)
    CHECKVALUE(UNUSED7)

    return stream;

#undef CHECKVALUE
}

int DataHeaderFlagUtil::fromString(bsl::ostream&      errorDescription,
                                   int*               out,
                                   const bsl::string& str)
{
    int rc = 0;
    *out   = 0;

    bdlb::Tokenizer tokenizer(str, ",");
    for (bdlb::TokenizerIterator it = tokenizer.begin(); it != tokenizer.end();
         ++it) {
        DataHeaderFlags::Enum value;
        if (DataHeaderFlags::fromAscii(&value, *it) == false) {
            if (rc == 0) {  // First wrong flag
                errorDescription << "Invalid flag(s) '" << *it << "'";
            }
            else {
                errorDescription << ",'" << *it << "'";
            }
            rc = -1;
        }
        else {
            *out |= value;
        }
    }

    return rc;
}

// -----------------
// struct DataHeader
// -----------------

const int DataHeader::k_MSG_WORDS_MASK = bdlb::BitMaskUtil::one(
    DataHeader::k_MSG_WORDS_START_IDX,
    DataHeader::k_MSG_WORDS_NUM_BITS);

const int DataHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    DataHeader::k_HEADER_WORDS_START_IDX,
    DataHeader::k_HEADER_WORDS_NUM_BITS);

const int DataHeader::k_OPTIONS_WORDS_MASK = bdlb::BitMaskUtil::one(
    DataHeader::k_OPTIONS_WORDS_START_IDX,
    DataHeader::k_OPTIONS_WORDS_NUM_BITS);

const int DataHeader::k_FLAGS_MASK = bdlb::BitMaskUtil::one(
    DataHeader::k_FLAGS_START_IDX,
    DataHeader::k_FLAGS_NUM_BITS);

// ------------------------
// struct QueueRecordHeader
// ------------------------

const int QueueRecordHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    QueueRecordHeader::k_HEADER_WORDS_START_IDX,
    QueueRecordHeader::k_HEADER_WORDS_NUM_BITS);

const int QueueRecordHeader::k_QUEUE_RECORD_WORDS_MASK =
    bdlb::BitMaskUtil::one(QueueRecordHeader::k_QUEUE_RECORD_WORDS_START_IDX,
                           QueueRecordHeader::k_QUEUE_RECORD_WORDS_NUM_BITS);

// -----------------
// struct RecordType
// -----------------

bsl::ostream& RecordType::print(bsl::ostream&    stream,
                                RecordType::Enum value,
                                int              level,
                                int              spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << RecordType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* RecordType::toAscii(RecordType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(MESSAGE)
        CASE(CONFIRM)
        CASE(DELETION)
        CASE(QUEUE_OP)
        CASE(JOURNAL_OP)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -------------------
// struct RecordHeader
// -------------------

const int RecordHeader::k_TYPE_MASK = bdlb::BitMaskUtil::one(
    RecordHeader::k_TYPE_START_IDX,
    RecordHeader::k_TYPE_NUM_BITS);

const int RecordHeader::k_FLAGS_MASK = bdlb::BitMaskUtil::one(
    RecordHeader::k_FLAGS_START_IDX,
    RecordHeader::k_FLAGS_NUM_BITS);

bsl::ostream&
RecordHeader::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("type", type());
    printer.printAttribute("flags", flags());
    printer.printAttribute("primaryLeaseId", primaryLeaseId());
    printer.printAttribute("sequenceNumber", sequenceNumber());
    printer.printAttribute("timestamp", timestamp());
    printer.end();

    return stream;
}

// --------------------
// struct MessageRecord
// --------------------

const char MessageRecord::k_CAT_MASK               = 7;  // 00000111
const int MessageRecord::k_REFCOUNT_LOW_BITS_MASK = RecordHeader::k_FLAGS_MASK;

bsl::ostream&
MessageRecord::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("header", header());
    printer.printAttribute("refCount", refCount());
    printer.printAttribute("queueKey", queueKey());
    printer.printAttribute("fileKey", fileKey());
    printer.printAttribute("messageOffsetDwords", messageOffsetDwords());
    printer.printAttribute("messageGUID", messageGUID());
    printer.printAttribute("crc32c", crc32c());
    printer.printAttribute("compressionAlgorithmType",
                           compressionAlgorithmType());
    printer.end();

    return stream;
}

// --------------------
// struct ConfirmReason
// --------------------

bsl::ostream& ConfirmReason::print(bsl::ostream&       stream,
                                   ConfirmReason::Enum value,
                                   int                 level,
                                   int                 spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ConfirmReason::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ConfirmReason::toAscii(ConfirmReason::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(CONFIRMED)
        CASE(REJECTED)
        CASE(AUTO_CONFIRMED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// --------------------
// struct ConfirmRecord
// --------------------

bsl::ostream&
ConfirmRecord::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("header", header());
    printer.printAttribute("reason", reason());
    printer.printAttribute("queueKey", queueKey());
    printer.printAttribute("appKey", appKey());
    printer.printAttribute("messageGUID", messageGUID());
    printer.end();

    return stream;
}

// -------------------------
// struct DeletionRecordFlag
// -------------------------

bsl::ostream& DeletionRecordFlag::print(bsl::ostream&            stream,
                                        DeletionRecordFlag::Enum value,
                                        int                      level,
                                        int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << DeletionRecordFlag::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* DeletionRecordFlag::toAscii(DeletionRecordFlag::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NONE)
        CASE(IMPLICIT_CONFIRM)
        CASE(TTL_EXPIRATION)
        CASE(NO_SC_QUORUM)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ---------------------
// struct DeletionRecord
// ---------------------

bsl::ostream& DeletionRecord::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("header", header());
    printer.printAttribute("deletionRecordFlag", deletionRecordFlag());
    printer.printAttribute("queueKey", queueKey());
    printer.printAttribute("messageGUID", messageGUID());
    printer.end();

    return stream;
}

// ------------------
// struct QueueOpType
// ------------------

bsl::ostream& QueueOpType::print(bsl::ostream&     stream,
                                 QueueOpType::Enum value,
                                 int               level,
                                 int               spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << QueueOpType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* QueueOpType::toAscii(QueueOpType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(PURGE)
        CASE(CREATION)
        CASE(DELETION)
        CASE(ADDITION)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// --------------------
// struct QueueOpRecord
// --------------------

bsl::ostream&
QueueOpRecord::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("header", header());
    printer.printAttribute("flags", flags());
    printer.printAttribute("queueKey", queueKey());
    printer.printAttribute("appKey", appKey());
    printer.printAttribute("type", type());
    printer.printAttribute("queueUriRecordOffsetWords",
                           queueUriRecordOffsetWords());
    printer.end();

    return stream;
}

// --------------------
// struct JournalOpType
// --------------------

bsl::ostream& JournalOpType::print(bsl::ostream&       stream,
                                   JournalOpType::Enum value,
                                   int                 level,
                                   int                 spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << JournalOpType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* JournalOpType::toAscii(JournalOpType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(UNUSED)
        CASE(SYNCPOINT)
        CASE(UPDATE_STORAGE_SIZE)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// --------------------
// struct SyncPointType
// --------------------

bsl::ostream& SyncPointType::print(bsl::ostream&       stream,
                                   SyncPointType::Enum value,
                                   int                 level,
                                   int                 spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << SyncPointType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* SyncPointType::toAscii(SyncPointType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(REGULAR)
        CASE(ROLLOVER)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ----------------------
// struct JournalOpRecord
// ----------------------

bsl::ostream& JournalOpRecord::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("header", header());
    printer.printAttribute("flags", flags());
    printer.printAttribute("type", type());
    if (JournalOpType::e_SYNCPOINT == type()) {
        printer.printAttribute("syncPointType", syncPointType());

        const JournalOpRecord::SyncPointData& syncPoint = syncPointData();
        printer.printAttribute("sequenceNum", syncPoint.sequenceNum());
        printer.printAttribute("primaryNodeId", syncPoint.primaryNodeId());
        printer.printAttribute("primaryLeaseId", syncPoint.primaryLeaseId());
        printer.printAttribute("dataFileOffsetDwords", syncPoint.dataFileOffsetDwords());
        printer.printAttribute("qlistFileOffsetWords", syncPoint.qlistFileOffsetWords());
    } else if (JournalOpType::e_UPDATE_STORAGE_SIZE == type()) {
        const mqbs::JournalOpRecord::UpdateStorageSizeData& ussd = updateStorageSizeData();
        printer.printAttribute("maxJournalFileSize", ussd.maxJournalFileSize());
        printer.printAttribute("maxDataFileSize", ussd.maxDataFileSize());
        printer.printAttribute("maxQlistFileSize", ussd.maxQlistFileSize());
    }

    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
