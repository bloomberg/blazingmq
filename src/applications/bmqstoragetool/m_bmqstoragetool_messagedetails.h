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

#ifndef INCLUDED_M_BMQSTORAGETOOL_MESSGEDETAILS
#define INCLUDED_M_BMQSTORAGETOOL_MESSGEDETAILS

//@PURPOSE: Provide a representation of message details.
//
//@CLASSES:
//  m_bmqstoragetool::MessageDetails: representation of message details.
//
//@DESCRIPTION: 'MessageDetails' provides a representation of message details.

// bmqstoragetool
#include <m_bmqstoragetool_queuemap.h>

// BMQ
#include <bmqu_memoutstream.h>

// MQB
#include <mqbs_filestoreprotocol.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlt_epochutil.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class MessageDetails
// =====================

class MessageDetails {
  public:
    // PUBLIC TYPES

    // Value-semantic type representing message details.
    template <typename RECORD_TYPE>
    struct RecordDetails {
        // DATA
        RECORD_TYPE d_record;
        // Record from journal file.
        bsls::Types::Uint64 d_recordIndex;
        // Index of the record from journal file.
        bsls::Types::Uint64 d_recordOffset;
        // Offset of the record from journal file.
        bsl::string_view d_queueUri;
        // URI of the string
        bsl::string_view d_appId;

        // CREATORS

        /// Constructor with the specified `record`, `recordIndex` and
        /// `recordOffset`.
        RecordDetails(RECORD_TYPE         record,
                      bsls::Types::Uint64 recordIndex,
                      bsls::Types::Uint64 recordOffset)
        : d_record(record)
        , d_recordIndex(recordIndex)
        , d_recordOffset(recordOffset)
        {
            // NOTHING
        }
    };

  private:
    // DATA
    RecordDetails<mqbs::MessageRecord> d_messageRecord;
    // Message record
    bsl::vector<RecordDetails<mqbs::ConfirmRecord> > d_confirmRecords;
    // All the confirm records related to the `d_messageRecord`
    bdlb::NullableValue<RecordDetails<mqbs::DeletionRecord> > d_deleteRecord;
    // Delete record related to the `d_messageRecord`
    bmqp_ctrlmsg::QueueInfo* d_queueInfo_p;
    // A pointer to the QueueInfo of the message's queue
    bslma::Allocator* d_allocator_p;
    // Allocator used inside te class

  public:
    // CREATORS

    /// Constructor using the specified arguments.
    explicit MessageDetails(const mqbs::MessageRecord& record,
                            bsls::Types::Uint64        recordIndex,
                            bsls::Types::Uint64        recordOffset,
                            const QueueMap&            queueMap,
                            bslma::Allocator*          allocator);

    // MANIPULATORS

    /// Add confirmation record to message details.
    void addConfirmRecord(const mqbs::ConfirmRecord& record,
                          bsls::Types::Uint64        recordIndex,
                          bsls::Types::Uint64        recordOffset);

    /// Add deletion record to message details.
    void addDeleteRecord(const mqbs::DeletionRecord& record,
                         bsls::Types::Uint64         recordIndex,
                         bsls::Types::Uint64         recordOffset);

    // ACCESSORS

    /// Return message's data record offset.
    unsigned int dataRecordOffset() const;

    /// Return message record index in Journal file.
    bsls::Types::Uint64 messageRecordIndex() const;

    /// Return a reference to the non-modifiable message record
    const RecordDetails<mqbs::MessageRecord>& messageRecord() const;

    /// Return a reference to the non-modifiable vector of all the confirm
    /// records related to the `d_messageRecord`
    const bsl::vector<RecordDetails<mqbs::ConfirmRecord> >&
    confirmRecords() const;

    /// Return a reference to the non-modifiable delete record
    const bdlb::NullableValue<RecordDetails<mqbs::DeletionRecord> >&
    deleteRecord() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
