// Copyright 2014-2023 Bloomberg Finance L.P.
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

#include <m_bmqstoragetool_messagedetails.h>

// BDE
#include <bslma_allocator.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>

// MWC
#include <mwcu_alignedprinter.h>
#include <mwcu_memoutstream.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {
// ============================================================================
//                            PRINT HELPERS
// These methods are enhanced versions of
// mqbs::FileStoreProtocolPrinter::printRecord() methods.
// ----------------------------------------------------------------------------

void printRecord(bsl::ostream&                  stream,
                 const mqbs::MessageRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p)
{
    bsl::vector<const char*> fields;
    fields.push_back("PrimaryLeaseId");
    fields.push_back("SequenceNumber");
    fields.push_back("Timestamp");
    fields.push_back("Epoch");
    fields.push_back("FileKey");
    fields.push_back("QueueKey");
    if (queueInfo_p)
        fields.push_back("QueueUri");
    fields.push_back("RefCount");
    fields.push_back("MsgOffsetDwords");
    fields.push_back("GUID");
    fields.push_back("Crc32c");

    mwcu::AlignedPrinter printer(stream, &fields);
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

    mwcu::MemOutStream fileKeyStr, queueKeyStr;
    fileKeyStr << rec.fileKey();
    queueKeyStr << rec.queueKey();

    printer << epochValue << fileKeyStr.str() << queueKeyStr.str();
    if (queueInfo_p)
        printer << queueInfo_p->uri();
    printer << rec.refCount() << rec.messageOffsetDwords() << rec.messageGUID()
            << rec.crc32c();

    stream << "\n";
}

bool findQueueAppIdByAppKey(
    bsl::string*                                            appId,
    const bsl::vector<BloombergLP::bmqp_ctrlmsg::AppIdInfo> appIds,
    const mqbu::StorageKey&                                 appKey)
{
    if (appKey.isNull())
        return false;  // RETURN

    auto it = bsl::find_if(appIds.begin(),
                           appIds.end(),
                           [&](const bmqp_ctrlmsg::AppIdInfo& appIdInfo) {
                               auto key = mqbu::StorageKey(
                                   mqbu::StorageKey::BinaryRepresentation(),
                                   appIdInfo.appKey().begin());
                               return (key == appKey);
                           });

    if (it != appIds.end()) {
        *appId = it->appId();
        return true;  // RETURN
    }
    return false;
}

void printRecord(bsl::ostream&                  stream,
                 const mqbs::ConfirmRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p)
{
    bsl::vector<const char*> fields;
    fields.push_back("PrimaryLeaseId");
    fields.push_back("SequenceNumber");
    fields.push_back("Timestamp");
    fields.push_back("Epoch");
    fields.push_back("QueueKey");
    if (queueInfo_p)
        fields.push_back("QueueUri");
    fields.push_back("AppKey");
    if (queueInfo_p)
        fields.push_back("AppId");
    fields.push_back("GUID");

    mwcu::MemOutStream queueKeyStr, appKeyStr;
    queueKeyStr << rec.queueKey();

    if (rec.appKey().isNull()) {
        appKeyStr << "** NULL **";
    }
    else {
        appKeyStr << rec.appKey();
    }

    bsl::string appIdStr;
    if (queueInfo_p) {
        if (!findQueueAppIdByAppKey(&appIdStr,
                                    queueInfo_p->appIds(),
                                    rec.appKey())) {
            appIdStr = "** NULL **";
        }
    }

    mwcu::AlignedPrinter printer(stream, &fields);
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

    printer << epochValue << queueKeyStr.str();
    if (queueInfo_p)
        printer << queueInfo_p->uri();
    printer << appKeyStr.str();
    if (queueInfo_p)
        printer << appIdStr;
    printer << rec.messageGUID();
    stream << "\n";
}

void printRecord(bsl::ostream&                  stream,
                 const mqbs::DeletionRecord&    rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p)
{
    bsl::vector<const char*> fields;
    fields.push_back("PrimaryLeaseId");
    fields.push_back("SequenceNumber");
    fields.push_back("Timestamp");
    fields.push_back("Epoch");
    fields.push_back("QueueKey");
    if (queueInfo_p)
        fields.push_back("QueueUri");
    fields.push_back("DeletionFlag");
    fields.push_back("GUID");

    mwcu::MemOutStream queueKeyStr;
    queueKeyStr << rec.queueKey();

    mwcu::AlignedPrinter printer(stream, &fields);
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
    printer << epochValue << queueKeyStr.str();
    if (queueInfo_p)
        printer << queueInfo_p->uri();
    printer << rec.deletionRecordFlag() << rec.messageGUID();
    stream << "\n";
}

}  // close unnamed namespace

// =====================
// class MessageDetails
// =====================

// CREATORS
MessageDetails::MessageDetails(const mqbs::MessageRecord& record,
                               bsls::Types::Uint64        recordIndex,
                               bsls::Types::Uint64        recordOffset,
                               bslma::Allocator*          allocator)
: d_messageRecord(
      RecordDetails<mqbs::MessageRecord>(record, recordIndex, recordOffset))
, d_confirmRecords(allocator)
{
    // NOTHING
}

void MessageDetails::addConfirmRecord(const mqbs::ConfirmRecord& record,
                                      bsls::Types::Uint64        recordIndex,
                                      bsls::Types::Uint64        recordOffset)
{
    d_confirmRecords.push_back(
        RecordDetails<mqbs::ConfirmRecord>(record, recordIndex, recordOffset));
}

void MessageDetails::addDeleteRecord(const mqbs::DeletionRecord& record,
                                     bsls::Types::Uint64         recordIndex,
                                     bsls::Types::Uint64         recordOffset)
{
    d_deleteRecord = RecordDetails<mqbs::DeletionRecord>(record,
                                                         recordIndex,
                                                         recordOffset);
}

void MessageDetails::print(bsl::ostream& os, const QueueMap& queueMap) const
{
    // Check if queueInfo is present for queue key
    bmqp_ctrlmsg::QueueInfo queueInfo;
    bool                    queueInfoPresent = queueMap.findInfoByKey(
        &queueInfo,
        d_messageRecord.d_record.queueKey());
    bmqp_ctrlmsg::QueueInfo* queueInfo_p = queueInfoPresent ? &queueInfo
                                                            : nullptr;

    // Print message record
    bsl::stringstream ss;
    ss << "MESSAGE Record, index: " << d_messageRecord.d_recordIndex
       << ", offset: " << d_messageRecord.d_recordOffset;
    bsl::string delimiter(ss.str().size(), '=');
    os << delimiter << '\n' << ss.str() << '\n';

    printRecord(os, d_messageRecord.d_record, queueInfo_p);

    // Print confirmations records
    if (!d_confirmRecords.empty()) {
        for (auto& rec : d_confirmRecords) {
            os << "CONFIRM Record, index: " << rec.d_recordIndex
               << ", offset: " << rec.d_recordOffset << '\n';
            printRecord(os, rec.d_record, queueInfo_p);
        }
    }

    // Print deletion record
    if (d_deleteRecord.d_isValid) {
        os << "DELETE Record, index: " << d_deleteRecord.d_recordIndex
           << ", offset: " << d_deleteRecord.d_recordOffset << '\n';
        printRecord(os, d_deleteRecord.d_record, queueInfo_p);
    }
}

unsigned int MessageDetails::dataRecordOffset() const
{
    return d_messageRecord.d_record.messageOffsetDwords();
}

bsls::Types::Uint64 MessageDetails::messageRecordIndex() const
{
    return d_messageRecord.d_recordIndex;
}

}  // close package namespace
}  // close enterprise namespace
