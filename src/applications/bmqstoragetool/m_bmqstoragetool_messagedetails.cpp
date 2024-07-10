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

class AppKeyMatcher {
    const mqbu::StorageKey* d_appKey;

  public:
    AppKeyMatcher(const mqbu::StorageKey& appKey)
    : d_appKey(&appKey)
    {
    }

    bool operator()(const bmqp_ctrlmsg::AppIdInfo& appIdInfo)
    {
        const mqbu::StorageKey key = mqbu::StorageKey(
            mqbu::StorageKey::BinaryRepresentation(),
            appIdInfo.appKey().begin());
        return (key == *d_appKey);
    }
};

/// Print the specified message record `rec` and QueueInfo pointed by the
/// specified `queueInfo_p` to the specified `stream`, using the specified
/// `allocator` for memory allocation.
void printRecord(bsl::ostream&                  stream,
                 const mqbs::MessageRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator)
{
    bsl::vector<const char*> fields(allocator);
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
    const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
    if (0 != rc) {
        printer << 0;
    }
    else {
        printer << datetime;
    }

    mwcu::MemOutStream fileKeyStr(allocator), queueKeyStr(allocator);
    fileKeyStr << rec.fileKey();
    queueKeyStr << rec.queueKey();

    printer << epochValue << fileKeyStr.str() << queueKeyStr.str();
    if (queueInfo_p)
        printer << queueInfo_p->uri();
    printer << rec.refCount() << rec.messageOffsetDwords() << rec.messageGUID()
            << rec.crc32c();

    stream << "\n";
}

/// Find AppId in the specified `appIds` by the specified `appKey` and store
/// the result in the specified `appId`. Return `true` on success and `false
/// otherwise.
bool findQueueAppIdByAppKey(
    bsl::string*                                             appId,
    const bsl::vector<BloombergLP::bmqp_ctrlmsg::AppIdInfo>& appIds,
    const mqbu::StorageKey&                                  appKey)
{
    // PRECONDITIONS
    BSLS_ASSERT(appId);

    if (appKey.isNull())
        return false;  // RETURN

    bsl::vector<BloombergLP::bmqp_ctrlmsg::AppIdInfo>::const_iterator it =
        bsl::find_if(appIds.cbegin(), appIds.cend(), AppKeyMatcher(appKey));

    if (it != appIds.end()) {
        *appId = it->appId();
        return true;  // RETURN
    }
    return false;
}

/// Print the specified confirm record `rec` and QueueInfo pointed by the
/// specified `queueInfo_p` to the specified `stream`, using the specified
/// `allocator` for memory allocation.
void printRecord(bsl::ostream&                  stream,
                 const mqbs::ConfirmRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator)
{
    bsl::vector<const char*> fields(allocator);
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

    mwcu::MemOutStream queueKeyStr(allocator), appKeyStr(allocator);
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
    const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
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

/// Print the specified delete record `rec` and QueueInfo pointed by the
/// specified `queueInfo_p` to the specified `stream`, using the specified
/// `allocator` for memory allocation.
void printRecord(bsl::ostream&                  stream,
                 const mqbs::DeletionRecord&    rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator)
{
    bsl::vector<const char*> fields(allocator);
    fields.push_back("PrimaryLeaseId");
    fields.push_back("SequenceNumber");
    fields.push_back("Timestamp");
    fields.push_back("Epoch");
    fields.push_back("QueueKey");
    if (queueInfo_p)
        fields.push_back("QueueUri");
    fields.push_back("DeletionFlag");
    fields.push_back("GUID");

    mwcu::MemOutStream queueKeyStr(allocator);
    queueKeyStr << rec.queueKey();

    mwcu::AlignedPrinter printer(stream, &fields);
    printer << rec.header().primaryLeaseId() << rec.header().sequenceNumber();

    bsls::Types::Uint64 epochValue = rec.header().timestamp();
    bdlt::Datetime      datetime;
    const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
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
, d_allocator_p(allocator)
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
    bmqp_ctrlmsg::QueueInfo queueInfo(d_allocator_p);
    const bool              queueInfoPresent = queueMap.findInfoByKey(
        &queueInfo,
        d_messageRecord.d_record.queueKey());
    bmqp_ctrlmsg::QueueInfo* queueInfo_p = queueInfoPresent ? &queueInfo : 0;

    // Print message record
    mwcu::MemOutStream ss(d_allocator_p);
    ss << "MESSAGE Record, index: " << d_messageRecord.d_recordIndex
       << ", offset: " << d_messageRecord.d_recordOffset;
    bsl::string delimiter(ss.length(), '=', d_allocator_p);
    os << delimiter << '\n' << ss.str() << '\n';

    printRecord(os, d_messageRecord.d_record, queueInfo_p, d_allocator_p);

    // Print confirmations records
    if (!d_confirmRecords.empty()) {
        bsl::vector<RecordDetails<mqbs::ConfirmRecord> >::const_iterator it =
            d_confirmRecords.begin();
        for (; it != d_confirmRecords.end(); ++it) {
            os << "CONFIRM Record, index: " << it->d_recordIndex
               << ", offset: " << it->d_recordOffset << '\n';
            printRecord(os, it->d_record, queueInfo_p, d_allocator_p);
        }
    }

    // Print deletion record
    if (d_deleteRecord.d_isValid) {
        os << "DELETE Record, index: " << d_deleteRecord.d_recordIndex
           << ", offset: " << d_deleteRecord.d_recordOffset << '\n';
        printRecord(os, d_deleteRecord.d_record, queueInfo_p, d_allocator_p);
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
