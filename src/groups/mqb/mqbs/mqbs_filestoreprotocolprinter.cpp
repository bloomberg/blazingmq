// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mqbs_filestoreprotocolprinter.cpp                                  -*-C++-*-
#include <mqbs_filestoreprotocolprinter.h>

// MQB
#include <mqbs_filesystemutil.h>
#include <mqbs_offsetptr.h>

// BMQ
#include <bmqp_messageproperties.h>
#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>
#include <bmqt_propertytype.h>

#include <bmqu_alignedprinter.h>
#include <bmqu_blob.h>
#include <bmqu_jsonprinter.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_stringutil.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <ball_log.h>
#include <bdlb_print.h>
#include <bdlbb_blob.h>
#include <bdls_filesystemutil.h>
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBS.FILESTOREPROTOCOLPRINTER");

}  // close unnamed namespace

// FREE FUNCTIONS
bsl::ostream& operator<<(bsl::ostream&           stream,
                         const mqbs::DataHeader& dataHeader)
{
    bmqu::OutStreamFormatSaver fmtSaver(stream);

    stream << "    HeaderWords:  " << dataHeader.headerWords() << '\n';
    stream << "    OptionsWords: " << dataHeader.optionsWords() << '\n';
    stream << "    MessageWords: " << dataHeader.messageWords() << '\n';

    stream << "    Flags:\n";

    const static int k_NUM_FLAGS = 1;

    const char* flagStrings[k_NUM_FLAGS];
    flagStrings[0] = mqbs::DataHeaderFlags::toAscii(
        mqbs::DataHeaderFlags::e_MESSAGE_PROPERTIES);

    stream << "        " << flagStrings[0] << ": " << bsl::boolalpha
           << mqbs::DataHeaderFlagUtil::isSet(
                  dataHeader.flags(),
                  mqbs::DataHeaderFlags::e_MESSAGE_PROPERTIES)
           << '\n';

    return stream;
}

bsl::ostream& operator<<(bsl::ostream&               stream,
                         const mqbs::DataFileHeader& header)
{
    stream << "Data File Header: \n";

    FileStoreProtocolPrinter::printDataFileHeader<bmqu::AlignedPrinter>(
        stream,
        header);
    stream << '\n';

    return stream;
}

bsl::ostream& operator<<(bsl::ostream&                stream,
                         const mqbs::QlistFileHeader& header)
{
    bsl::vector<const char*> fields;
    fields.push_back("HeaderWords");

    stream << "Qlist File Header: \n";

    bmqu::AlignedPrinter printer(stream, &fields);
    printer << static_cast<int>(header.headerWords());
    stream << '\n';

    return stream;
}

bsl::ostream& operator<<(bsl::ostream&                     stream,
                         const mqbs::MappedFileDescriptor& mfd)
{
    stream << "BlazingMQ File Header" << '\n';

    FileStoreProtocolPrinter::printFileHeader<bmqu::AlignedPrinter>(stream,
                                                                    mfd);

    stream << '\n';

    return stream;
}

bsl::ostream& operator<<(bsl::ostream&                 stream,
                         const mqbs::DataFileIterator& it)
{
    const char*  data;
    unsigned int length;
    it.loadApplicationData(&data, &length);
    stream << '\n'
           << "Record Index: " << it.recordIndex() << ", Size: " << length
           << " bytes";

    return stream;
}

bsl::ostream& operator<<(bsl::ostream&                  stream,
                         const mqbs::QlistFileIterator& it)
{
    // For convenience
    const int k_KEY_LEN = mqbs::FileStoreProtocol::k_KEY_LENGTH;

    const char*  uri      = 0;
    const char*  queueKey = 0;
    unsigned int uriLen   = 0;

    it.loadQueueUri(&uri, &uriLen);
    it.loadQueueUriHash(&queueKey);

    bsl::string queueUri(uri, uriLen);

    bmqu::MemOutStream queueKeyStr;
    bdlb::Print::singleLineHexDump(queueKeyStr, queueKey, k_KEY_LEN);

    stream << '\n'
           << "Record Index: " << it.recordIndex()
           << ", Queue Uri: " << queueUri
           << ", Queue Key: " << queueKeyStr.str();

    return stream;
}

bsl::ostream& operator<<(bsl::ostream&                    stream,
                         const mqbs::JournalFileIterator& it)
{
    stream << '\n'
           << "Record Index: " << it.recordIndex()
           << ", Record Type: " << it.recordType();

    return stream;
}

bsl::ostream& operator<<(bsl::ostream&                          stream,
                         const bmqp::MessagePropertiesIterator& iterator)
{
    const size_t k_MIN_LENGTH = 128;

    // Save stream flags
    bmqu::OutStreamFormatSaver fmtSaver(stream);

    switch (iterator.type()) {
    case bmqt::PropertyType::e_UNDEFINED: {
        stream << "** NA **";
        return stream;  // RETURN
    }

    case bmqt::PropertyType::e_BOOL: {
        stream << bsl::boolalpha << iterator.getAsBool();
        return stream;  // RETURN
    }

    case bmqt::PropertyType::e_CHAR: {
        stream << iterator.getAsChar()
               << " (as integer: " << static_cast<int>(iterator.getAsChar())
               << ", as hex: " << bsl::hex << bsl::uppercase
               << static_cast<int>(iterator.getAsChar()) << bsl::dec << ")";
        return stream;  // RETURN
    }

    case bmqt::PropertyType::e_SHORT: {
        stream << iterator.getAsShort();
        return stream;  // RETURN
    }

    case bmqt::PropertyType::e_INT32: {
        stream << iterator.getAsInt32();
        return stream;  // RETURN
    }

    case bmqt::PropertyType::e_INT64: {
        stream << iterator.getAsInt64();
        return stream;  // RETURN
    }

    case bmqt::PropertyType::e_STRING: {
        const bsl::string& val = iterator.getAsString();
        bdlb::Print::printString(stream,
                                 val.c_str(),
                                 bsl::min(k_MIN_LENGTH, val.size()));
        return stream;  // RETURN
    }

    case bmqt::PropertyType::e_BINARY: {
        const bsl::vector<char>& val = iterator.getAsBinary();
        bdlb::Print::singleLineHexDump(stream,
                                       val.data(),
                                       bsl::min(k_MIN_LENGTH, val.size()));
        return stream;  // RETURN
    }

    default: BSLS_ASSERT_SAFE(false && "Unreachable by design.");
    }

    return stream;
}

// ==================================
// namespace FileStoreProtocolPrinter
// ==================================

namespace FileStoreProtocolPrinter {

void printRecord(bsl::ostream& stream, const mqbs::MessageRecord& rec)
{
    bsl::vector<const char*> fields;
    fields.reserve(10);
    fields.push_back("PrimaryLeaseId");
    fields.push_back("SequenceNumber");
    fields.push_back("Timestamp");
    fields.push_back("Epoch");
    fields.push_back("FileKey");
    fields.push_back("QueueKey");
    fields.push_back("RefCount");
    fields.push_back("MsgOffsetDwords");
    fields.push_back("GUID");
    fields.push_back("Crc32c");

    bmqu::AlignedPrinter printer(stream, &fields);
    printer << rec.header().primaryLeaseId() << rec.header().sequenceNumber();

    bsls::Types::Uint64 epochValue = rec.header().timestamp();
    bdlt::Datetime      datetime;
    int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
    if (0 != rc) {
        printer << 0;
    }
    else {
        printer << datetime;
    }

    bmqu::MemOutStream fileKeyStr, queueKeyStr;
    fileKeyStr << rec.fileKey();
    queueKeyStr << rec.queueKey();

    printer << epochValue << fileKeyStr.str() << queueKeyStr.str()
            << rec.refCount() << rec.messageOffsetDwords() << rec.messageGUID()
            << rec.crc32c();

    stream << "\n";
}

void printRecord(bsl::ostream& stream, const mqbs::ConfirmRecord& rec)
{
    bsl::vector<const char*> fields;
    fields.reserve(7);
    fields.push_back("PrimaryLeaseId");
    fields.push_back("SequenceNumber");
    fields.push_back("Timestamp");
    fields.push_back("Epoch");
    fields.push_back("QueueKey");
    fields.push_back("AppKey");
    fields.push_back("GUID");

    bmqu::MemOutStream queueKeyStr, appKeyStr;
    queueKeyStr << rec.queueKey();

    if (rec.appKey().isNull()) {
        appKeyStr << "** NULL **";
    }
    else {
        appKeyStr << rec.appKey();
    }

    bmqu::AlignedPrinter printer(stream, &fields);
    printer << rec.header().primaryLeaseId() << rec.header().sequenceNumber();

    bsls::Types::Uint64 epochValue = rec.header().timestamp();
    bdlt::Datetime      datetime;
    int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
    if (0 != rc) {
        printer << 0;
    }
    else {
        printer << datetime;
    }
    printer << epochValue << queueKeyStr.str() << appKeyStr.str()
            << rec.messageGUID();
    stream << "\n";
}

void printRecord(bsl::ostream& stream, const mqbs::DeletionRecord& rec)
{
    bsl::vector<const char*> fields;
    fields.push_back("PrimaryLeaseId");
    fields.push_back("SequenceNumber");
    fields.push_back("Timestamp");
    fields.push_back("Epoch");
    fields.push_back("QueueKey");
    fields.push_back("DeletionFlag");
    fields.push_back("GUID");

    bmqu::MemOutStream queueKeyStr;
    queueKeyStr << rec.queueKey();

    bmqu::AlignedPrinter printer(stream, &fields);
    printer << rec.header().primaryLeaseId() << rec.header().sequenceNumber();

    bsls::Types::Uint64 epochValue = rec.header().timestamp();
    bdlt::Datetime      datetime;
    int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
    if (0 != rc) {
        printer << 0;
    }
    else {
        printer << datetime;
    }
    printer << epochValue << queueKeyStr.str() << rec.deletionRecordFlag()
            << rec.messageGUID();
    stream << "\n";
}

void printRecord(bsl::ostream& stream, const mqbs::QueueOpRecord& rec)
{
    bsl::vector<const char*> fields;
    fields.reserve(8);
    fields.push_back("PrimaryLeaseId");
    fields.push_back("SequenceNumber");
    fields.push_back("Timestamp");
    fields.push_back("Epoch");
    fields.push_back("QueueKey");
    fields.push_back("AppKey");
    fields.push_back("QueueOpType");
    if (mqbs::QueueOpType::e_CREATION == rec.type() ||
        mqbs::QueueOpType::e_ADDITION == rec.type()) {
        fields.push_back("QLIST OffsetWords");
    }
    fields.push_back("StartPrimaryLeaseId");
    fields.push_back("StartSequenceNumber");

    bmqu::MemOutStream queueKeyStr, appKeyStr;
    queueKeyStr << rec.queueKey();

    if (rec.appKey().isNull()) {
        appKeyStr << "** NULL **";
    }
    else {
        appKeyStr << rec.appKey();
    }

    bmqu::AlignedPrinter printer(stream, &fields);
    printer << rec.header().primaryLeaseId() << rec.header().sequenceNumber();

    bsls::Types::Uint64 epochValue = rec.header().timestamp();
    bdlt::Datetime      datetime;
    int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
    if (0 != rc) {
        printer << 0;
    }
    else {
        printer << datetime;
    }
    printer << epochValue << queueKeyStr.str() << appKeyStr.str()
            << rec.type();

    if (mqbs::QueueOpType::e_CREATION == rec.type() ||
        mqbs::QueueOpType::e_ADDITION == rec.type()) {
        printer << rec.queueUriRecordOffsetWords();
    }
    printer << rec.startPrimaryLeaseId() << rec.startSequenceNumber();

    stream << "\n";
}

void printRecord(bsl::ostream& stream, const mqbs::JournalOpRecord& rec)
{
    bsl::vector<const char*> fields;
    fields.reserve(10);
    fields.push_back("PrimaryLeaseId");
    fields.push_back("SequenceNumber");
    fields.push_back("Timestamp");
    fields.push_back("Epoch");
    fields.push_back("JournalOpType");
    fields.push_back("SyncPointType");
    fields.push_back("SyncPtPrimaryLeaseId");
    fields.push_back("SyncPtSequenceNumber");
    fields.push_back("PrimaryNodeId");
    fields.push_back("DataFileOffsetDwords");

    bmqu::AlignedPrinter printer(stream, &fields);
    printer << rec.header().primaryLeaseId() << rec.header().sequenceNumber();

    bsls::Types::Uint64 epochValue = rec.header().timestamp();
    bdlt::Datetime      datetime;
    int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
    if (0 != rc) {
        printer << 0;
    }
    else {
        printer << datetime;
    }

    printer << epochValue << rec.type() << rec.syncPointType()
            << rec.primaryNodeId() << rec.sequenceNum() << rec.primaryLeaseId()
            << rec.dataFileOffsetDwords();

    stream << "\n";
}

void printOption(bsl::ostream&                stream,
                 BSLS_ANNOTATION_UNUSED const bmqp::OptionsView* ov,
                 const bmqp::OptionsView::const_iterator&        cit)
{
    switch (*cit) {
    case bmqp::OptionType::e_SUB_QUEUE_IDS_OLD: {
        stream << "      " << *cit;
    } break;  // BREAK
    case bmqp::OptionType::e_MSG_GROUP_ID: {
        stream << "      " << *cit;
    } break;  // BREAK
    case bmqp::OptionType::e_SUB_QUEUE_INFOS: {
        stream << "      " << *cit;
    } break;  // BREAK
    case bmqp::OptionType::e_UNDEFINED: {
        stream << "      " << *cit;
    } break;  // BREAK
    }
}

void printOptions(bsl::ostream& stream, const char* options, unsigned int len)
{
    stream << '\n';

    if (0 == len) {
        return;  // RETURN
    }

    bsl::shared_ptr<char> optionsAreaSp(const_cast<char*>(options),
                                        bslstl::SharedPtrNilDeleter(),
                                        0);
    bdlbb::BlobBuffer     optionsBuffer(optionsAreaSp, len);
    bdlbb::Blob           optionsBlob;
    optionsBlob.appendDataBuffer(optionsBuffer);

    bmqp::OptionsView optionsView(&optionsBlob,
                                  bmqu::BlobPosition(0, 0),
                                  len,
                                  bslma::Default::allocator(0));

    if (!optionsView.isValid()) {
        stream << "   *** Invalid Options ***";
        stream << '\n';
        return;  // RETURN
    }

    for (bmqp::OptionsView::const_iterator cit = optionsView.begin();
         cit != optionsView.end();
         ++cit) {
        printOption(stream, &optionsView, cit);
        stream << '\n';
    }
}

void printPayload(bsl::ostream& stream, const char* data, unsigned int len)
{
    unsigned int minLen = bsl::min(len, 1024u);
    stream << "First " << minLen << " bytes of payload: \n";
    bdlb::Print::hexDump(stream, data, minLen);
    if (minLen < len) {
        stream << "And " << (len - minLen) << " more bytes (redacted)";
    }
}

int printMessageProperties(unsigned int* propertiesAreaLen,
                           bsl::ostream& stream,
                           const char*   appData,
                           const bmqp::MessagePropertiesInfo& logic)
{
    const bmqp::MessagePropertiesHeader& mph =
        *reinterpret_cast<const bmqp::MessagePropertiesHeader*>(appData);
    bsl::shared_ptr<char> propertiesAreaSp(const_cast<char*>(appData),
                                           bslstl::SharedPtrNilDeleter(),
                                           0);

    *propertiesAreaLen = mph.messagePropertiesAreaWords() *
                         bmqp::Protocol::k_WORD_SIZE;

    bdlbb::BlobBuffer propertiesBuffer(propertiesAreaSp, *propertiesAreaLen);
    bdlbb::Blob       propertiesBlob;
    propertiesBlob.appendDataBuffer(propertiesBuffer);

    bmqp::MessageProperties properties;
    int rc = properties.streamIn(propertiesBlob, logic.isExtended());
    if (rc) {
        return rc;  // RETURN
    }

    bmqp::MessagePropertiesIterator iter(&properties);
    while (iter.hasNext()) {
        stream << "    Name [" << iter.name() << "], Type [" << iter.type()
               << "], Value [" << iter << "]\n";
    }

    return 0;
}

void printIterator(mqbs::DataFileIterator& it)
{
    bmqu::MemOutStream dataHeaderOsstr;
    bmqu::MemOutStream optionsOsstr;
    bmqu::MemOutStream propsOsstr;

    dataHeaderOsstr << it.dataHeader();

    const char*  options    = 0;
    unsigned int optionsLen = 0;
    it.loadOptions(&options, &optionsLen);
    printOptions(optionsOsstr, options, optionsLen);

    const char*  appData    = 0;
    unsigned int appDataLen = 0;
    it.loadApplicationData(&appData, &appDataLen);

    const mqbs::DataHeader& dh                = it.dataHeader();
    unsigned int            propertiesAreaLen = 0;
    if (mqbs::DataHeaderFlagUtil::isSet(
            dh.flags(),
            mqbs::DataHeaderFlags::e_MESSAGE_PROPERTIES)) {
        int rc = printMessageProperties(&propertiesAreaLen,
                                        propsOsstr,
                                        appData,
                                        bmqp::MessagePropertiesInfo(dh));
        if (rc) {
            BALL_LOG_ERROR << "Failed to retrieve message properties, rc: "
                           << rc;
        }

        appDataLen -= propertiesAreaLen;
        appData += propertiesAreaLen;
    }

    // Payload
    bmqu::MemOutStream payloadOsstr;
    printPayload(payloadOsstr, appData, appDataLen);

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << '\n'
                               << "DataRecord index: " << it.recordIndex()
                               << ", offset: " << it.recordOffset() << '\n'
                               << "DataHeader: " << '\n'
                               << dataHeaderOsstr.str();

        if (0 != optionsLen) {
            BALL_LOG_OUTPUT_STREAM << "\nOptions: " << '\n'
                                   << optionsOsstr.str();
        }

        if (0 != propertiesAreaLen) {
            BALL_LOG_OUTPUT_STREAM << "\nProperties: " << '\n'
                                   << propsOsstr.str();
        }

        BALL_LOG_OUTPUT_STREAM << "\nPayload: " << '\n' << payloadOsstr.str();
    }
}

void printIterator(mqbs::QlistFileIterator& it)
{
    const char*        uri       = 0;
    const char*        queueKey  = 0;
    unsigned int       uriLen    = 0;
    const unsigned int numAppIds = it.queueRecordHeader()->numAppIds();

    it.loadQueueUri(&uri, &uriLen);
    it.loadQueueUriHash(&queueKey);

    bsl::vector<bsl::pair<const char*, unsigned int> > appIdLenPairs;
    bsl::vector<const char*>                           appIdHashes;
    it.loadAppIds(&appIdLenPairs);
    it.loadAppIdHashes(&appIdHashes);

    bsl::vector<const char*> fields;
    fields.reserve(3);
    fields.push_back("Queue URI");
    fields.push_back("QueueKey");
    fields.push_back("NumAppIds");

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << '\n'
                               << "QueueUriRecord index: " << it.recordIndex()
                               << ", offset: " << it.recordOffset() << '\n'
                               << "QueueUriRecord:" << '\n';

        bmqu::AlignedPrinter printer(BALL_LOG_OUTPUT_STREAM, &fields);
        printer << bsl::string(uri, uriLen)
                << mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                    queueKey)
                << numAppIds;

        if (0 != numAppIds) {
            bsl::vector<const char*> appIdsInfo;
            for (size_t n = 0; n < numAppIds; ++n) {
                appIdsInfo.push_back("AppId");
                appIdsInfo.push_back("AppKey");
            }

            bmqu::AlignedPrinter p(BALL_LOG_OUTPUT_STREAM, &appIdsInfo);
            for (size_t n = 0; n < numAppIds; ++n) {
                p << bsl::string(appIdLenPairs[n].first,
                                 appIdLenPairs[n].second)
                  << mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                      appIdHashes[n]);
            }
        }
    }
}

void printIterator(mqbs::JournalFileIterator& it)
{
    mqbs::RecordType::Enum recordType = it.recordType();
    bmqu::MemOutStream     oss;

    switch (recordType) {
    case mqbs::RecordType::e_MESSAGE: {
        const mqbs::MessageRecord& record = it.asMessageRecord();
        printRecord(oss, record);
    } break;
    case mqbs::RecordType::e_CONFIRM: {
        const mqbs::ConfirmRecord& record = it.asConfirmRecord();
        printRecord(oss, record);
    } break;
    case mqbs::RecordType::e_DELETION: {
        const mqbs::DeletionRecord& record = it.asDeletionRecord();
        printRecord(oss, record);
    } break;
    case mqbs::RecordType::e_QUEUE_OP: {
        const mqbs::QueueOpRecord& record = it.asQueueOpRecord();
        printRecord(oss, record);
    } break;
    case mqbs::RecordType::e_JOURNAL_OP: {
        const mqbs::JournalOpRecord& record = it.asJournalOpRecord();
        printRecord(oss, record);
    } break;
    case mqbs::RecordType::e_UNDEFINED:
    default:
        BALL_LOG_ERROR << "Unexpected record type: " << recordType;
        return;  // RETURN
    }

    BALL_LOG_INFO << '\n'
                  << "Record index: " << it.recordIndex()
                  << ", offset: " << it.recordOffset() << '\n'
                  << recordType << " Record:" << '\n'
                  << oss.str();
}

}  // close namespace FileStoreProtocolPrinter

}  // close package namespace
}  // close enterprise namespace
