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

// m_bmqstoragetool_messagedetails.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_MESSGEDETAILS
#define INCLUDED_M_BMQSTORAGETOOL_MESSGEDETAILS

// MQB
#include <mqbs_filestoreprotocol.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class MessageDetails
// =====================

// Value-semantic type representing message details.
class MessageDetails {
  private:
    // TYPES
    template <typename RECORD_TYPE>
    struct RecordDetails {
        RECORD_TYPE         d_record;
        bsls::Types::Uint64 d_recordIndex;
        bsls::Types::Uint64 d_recordOffset;

        RecordDetails() = default;

        explicit RecordDetails(RECORD_TYPE         record,
                               bsls::Types::Uint64 recordIndex,
                               bsls::Types::Uint64 recordOffset)
        : d_record(record)
        , d_recordIndex(recordIndex)
        , d_recordOffset(recordOffset)
        {
            // NOTHING
        }
    };

    // DATA
    RecordDetails<mqbs::MessageRecord>               d_messageRecord;
    bsl::vector<RecordDetails<mqbs::ConfirmRecord> > d_confirmRecords;
    RecordDetails<mqbs::DeletionRecord>              d_deleteRecord;

  public:
    // CREATORS
    explicit MessageDetails(const mqbs::MessageRecord& record,
                            bsls::Types::Uint64        recordIndex,
                            bsls::Types::Uint64        recordOffset,
                            bslma::Allocator*          allocator);

    // MANIPULATORS
    void addConfirmRecord(const mqbs::ConfirmRecord& record,
                          bsls::Types::Uint64        recordIndex,
                          bsls::Types::Uint64        recordOffset);

    void addDeleteRecord(const mqbs::DeletionRecord& record,
                         bsls::Types::Uint64         recordIndex,
                         bsls::Types::Uint64         recordOffset);

    // ACCESSORS

    // Prints this object to the specified `os` stream.
    void print(bsl::ostream& os) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif