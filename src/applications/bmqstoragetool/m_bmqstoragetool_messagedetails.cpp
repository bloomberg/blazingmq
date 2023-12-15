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

// m_bmqstoragetool_messagedetails.cpp -*-C++-*-
#include <m_bmqstoragetool_messagedetails.h>

// BDE
#include <bslma_allocator.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>

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

void MessageDetails::print(bsl::ostream& os) const
{
    // Print message record
    bsl::stringstream ss;
    ss << "MESSAGE Record, index: " << d_messageRecord.d_recordIndex
       << ", offset: " << d_messageRecord.d_recordOffset;
    bsl::string delimiter(ss.str().size(), '=');
    os << delimiter << bsl::endl << ss.str() << bsl::endl;

    mqbs::FileStoreProtocolPrinter::printRecord(os, d_messageRecord.d_record);

    // Print confirmations records
    if (!d_confirmRecords.empty()) {
        for (auto& rec : d_confirmRecords) {
            os << "CONFIRM Record, index: " << rec.d_recordIndex
               << ", offset: " << rec.d_recordOffset << bsl::endl;
            mqbs::FileStoreProtocolPrinter::printRecord(os, rec.d_record);
        }
    }

    // Print deletion record
    if (d_deleteRecord.d_isValid) {
        os << "DELETE Record, index: " << d_deleteRecord.d_recordIndex
           << ", offset: " << d_deleteRecord.d_recordOffset << bsl::endl;
        mqbs::FileStoreProtocolPrinter::printRecord(os,
                                                    d_deleteRecord.d_record);
    }
}

}  // close package namespace
}  // close enterprise namespace
