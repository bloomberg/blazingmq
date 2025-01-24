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

// bmqstoragetool
#include <m_bmqstoragetool_recordprinter.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_memoutstream.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// Functor to match AppKey
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

}  // close unnamed namespace

// =======================
// namespace RecordPrinter
// =======================

namespace RecordPrinter {

void printRecord(bsl::ostream&                  stream,
                 const mqbs::MessageRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator)
{
    // PRECONDITION
    BSLS_ASSERT(stream.good());

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

    bmqu::AlignedPrinter printer(stream, &fields);
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

    bmqu::MemOutStream fileKeyStr(allocator), queueKeyStr(allocator);
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

void printRecord(bsl::ostream&                  stream,
                 const mqbs::ConfirmRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator)
{
    // PRECONDITION
    BSLS_ASSERT(stream.good());

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

    bmqu::MemOutStream queueKeyStr(allocator), appKeyStr(allocator);
    queueKeyStr << rec.queueKey();

    if (rec.appKey().isNull()) {
        appKeyStr << "** NULL **";
    }
    else {
        appKeyStr << rec.appKey();
    }

    bsl::string appIdStr(allocator);
    if (queueInfo_p) {
        if (!findQueueAppIdByAppKey(&appIdStr,
                                    queueInfo_p->appIds(),
                                    rec.appKey())) {
            appIdStr = "** NULL **";
        }
    }

    bmqu::AlignedPrinter printer(stream, &fields);
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

void printRecord(bsl::ostream&                  stream,
                 const mqbs::DeletionRecord&    rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator)
{
    // PRECONDITION
    BSLS_ASSERT(stream.good());

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

    bmqu::MemOutStream queueKeyStr(allocator);
    queueKeyStr << rec.queueKey();

    bmqu::AlignedPrinter printer(stream, &fields);
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

void printRecord(bsl::ostream&                  stream,
                 const mqbs::QueueOpRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator)

{
    // PRECONDITION
    BSLS_ASSERT(stream.good());

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
    fields.push_back("QueueOpType");
    if (mqbs::QueueOpType::e_CREATION == rec.type() ||
        mqbs::QueueOpType::e_ADDITION == rec.type()) {
        fields.push_back("QLIST OffsetWords");
    }

    bmqu::MemOutStream queueKeyStr(allocator), appKeyStr(allocator);
    queueKeyStr << rec.queueKey();

    if (rec.appKey().isNull()) {
        appKeyStr << "** NULL **";
    }
    else {
        appKeyStr << rec.appKey();
    }

    bsl::string appIdStr(allocator);
    if (queueInfo_p) {
        if (!findQueueAppIdByAppKey(&appIdStr,
                                    queueInfo_p->appIds(),
                                    rec.appKey())) {
            appIdStr = "** NULL **";
        }
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
    printer << epochValue << queueKeyStr.str();
    if (queueInfo_p)
        printer << queueInfo_p->uri();

    printer << appKeyStr.str();
    if (queueInfo_p)
        printer << appIdStr;

    printer << rec.type();

    if (mqbs::QueueOpType::e_CREATION == rec.type() ||
        mqbs::QueueOpType::e_ADDITION == rec.type()) {
        printer << rec.queueUriRecordOffsetWords();
    }

    stream << "\n";
}

void printRecord(bsl::ostream&                         stream,
                 const bmqp_ctrlmsg::ClusterMessage&   rec,
                 const mqbc::ClusterStateRecordHeader& header,
                 const mqbsi::LedgerRecordId&          recId,
                 bslma::Allocator*                     allocator)
{
    // PRECONDITION
    BSLS_ASSERT(stream.good());

    // Print record header and recordId
    bmqu::MemOutStream ss(allocator);
    ss << header.recordType() << " Record, offset: " << recId.offset();
    bsl::string delimiter(ss.length(), '=', allocator);
    stream << delimiter << '\n' << ss.str() << '\n';

    const bsls::Types::Uint64 epochValue = header.timestamp();
    bdlt::Datetime            datetime;
    const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);

    bsl::vector<const char*> fields(allocator);
    fields.push_back("LogId");
    fields.push_back("ElectorTerm");
    fields.push_back("SequenceNumber");
    fields.push_back("HeaderWords");
    fields.push_back("LeaderAdvisoryWords");
    fields.push_back("Timestamp");
    if (rc == 0) {
        fields.push_back("Epoch");
    }

    bmqu::AlignedPrinter printer(stream, &fields);
    printer << recId.logId() << header.electorTerm() << header.sequenceNumber()
            << header.headerWords() << header.leaderAdvisoryWords();
    if (rc == 0) {
        printer << datetime << epochValue;
    }
    else {
        printer << epochValue;
    }

    // Print record
    rec.print(stream, 0, 2);
    stream << '\n';
}

}  // close namespace RecordPrinter

}  // close package namespace
}  // close enterprise namespace
