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
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_recordprinter.h>

// BDE
#include <bslma_allocator.h>

// BMQ
#include <bmqu_memoutstream.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

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
    bmqu::MemOutStream ss(d_allocator_p);
    ss << "MESSAGE Record, index: " << d_messageRecord.d_recordIndex
       << ", offset: " << d_messageRecord.d_recordOffset;
    bsl::string delimiter(ss.length(), '=', d_allocator_p);
    os << delimiter << '\n' << ss.str() << '\n';

    RecordPrinter::printRecord(os,
                               d_messageRecord.d_record,
                               queueInfo_p,
                               d_allocator_p);

    // Print confirmations records
    if (!d_confirmRecords.empty()) {
        bsl::vector<RecordDetails<mqbs::ConfirmRecord> >::const_iterator it =
            d_confirmRecords.begin();
        for (; it != d_confirmRecords.end(); ++it) {
            os << "CONFIRM Record, index: " << it->d_recordIndex
               << ", offset: " << it->d_recordOffset << '\n';
            RecordPrinter::printRecord(os,
                                       it->d_record,
                                       queueInfo_p,
                                       d_allocator_p);
        }
    }

    // Print deletion record
    if (d_deleteRecord.d_isValid) {
        os << "DELETE Record, index: " << d_deleteRecord.d_recordIndex
           << ", offset: " << d_deleteRecord.d_recordOffset << '\n';
        RecordPrinter::printRecord(os,
                                   d_deleteRecord.d_record,
                                   queueInfo_p,
                                   d_allocator_p);
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
