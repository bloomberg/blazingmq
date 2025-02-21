// Copyright 2014-2025 Bloomberg Finance L.P.
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

namespace BloombergLP {
namespace m_bmqstoragetool {

// void printRecord(bsl::ostream&                         stream,
//         const bmqp_ctrlmsg::ClusterMessage&   rec,
//         const mqbc::ClusterStateRecordHeader& header,
//         const mqbsi::LedgerRecordId&          recId,
//         bslma::Allocator*                     allocator)
// {
// // PRECONDITION
// BSLS_ASSERT(stream.good());

// // Print record header and recordId
// bmqu::MemOutStream ss(allocator);
// ss << header.recordType() << " Record, offset: " << recId.offset();
// bsl::string delimiter(ss.length(), '=', allocator);
// stream << delimiter << '\n' << ss.str() << '\n';

// const bsls::Types::Uint64 epochValue = header.timestamp();
// bdlt::Datetime            datetime;
// const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);

// bsl::vector<const char*> fields(allocator);
// fields.push_back("LogId");
// fields.push_back("ElectorTerm");
// fields.push_back("SequenceNumber");
// fields.push_back("HeaderWords");
// fields.push_back("LeaderAdvisoryWords");
// fields.push_back("Timestamp");
// if (rc == 0) {
// fields.push_back("Epoch");
// }

// bmqu::AlignedPrinter printer(stream, &fields);
// printer << recId.logId() << header.electorTerm() << header.sequenceNumber()
//    << header.headerWords() << header.leaderAdvisoryWords();
// if (rc == 0) {
// printer << datetime << epochValue;
// }
// else {
// printer << epochValue;
// }

// // Print record
// rec.print(stream, 0, 2);
// stream << '\n';
// }



}  // close package namespace
}  // close enterprise namespace
