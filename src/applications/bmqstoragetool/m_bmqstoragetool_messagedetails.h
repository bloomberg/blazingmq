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
#include <bdlt_epochutil.h>
#include <bsl_optional.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// FREE FUNCTIONS

/// Find AppId in the specified `appIds` by the specified `appKey` and store
/// the result in the specified `appId`. Return `true` on success and `false
/// otherwise.
bool findQueueAppIdByAppKey(
    bsl::string_view*                                        appId,
    const bsl::vector<BloombergLP::bmqp_ctrlmsg::AppIdInfo>& appIds,
    const mqbu::StorageKey&                                  appKey);

// ===================
// struct QueueDetails
// ===================

// Value-semantic type representing queue details.
struct QueueDetails {
    // PUBLIC TYPES

    struct AppDetails {
        bsls::Types::Uint64 d_recordsNumber;
        bsl::string_view    d_appId;

        bool operator==(const AppDetails& other) const
        {
            return d_recordsNumber == other.d_recordsNumber &&
                   d_appId == other.d_appId;
        }
    };
    typedef bsl::unordered_map<mqbu::StorageKey, AppDetails> AppDetailsMap;
    // Map of records deteils per App

    // PUBLIC DATA

    bsls::Types::Uint64 d_recordsNumber;
    // All records counts, related to the queue
    bsls::Types::Uint64 d_messageRecordsNumber;
    // Message records counts, related to the queue
    bsls::Types::Uint64 d_confirmRecordsNumber;
    // Confirm records counts, related to the queue
    bsls::Types::Uint64 d_deleteRecordsNumber;
    // Delete records counts, related to the queue
    bsls::Types::Uint64 d_queueOpRecordsNumber;
    // Queue operation records counts, related to the queue
    AppDetailsMap d_appDetailsMap;
    // Map containing all records counts, related to the queue per App
    bsl::string_view d_queueUri;

    // CREATORS

    explicit QueueDetails(bslma::Allocator* allocator)
    : d_recordsNumber(0)
    , d_messageRecordsNumber(0)
    , d_confirmRecordsNumber(0)
    , d_deleteRecordsNumber(0)
    , d_queueOpRecordsNumber(0)
    , d_appDetailsMap(allocator)
    {
        // NOTHING
    }

    bool operator==(const QueueDetails& other) const
    {
        bool result = d_recordsNumber == other.d_recordsNumber &&
                      d_messageRecordsNumber == other.d_messageRecordsNumber &&
                      d_confirmRecordsNumber == other.d_confirmRecordsNumber &&
                      d_deleteRecordsNumber == other.d_deleteRecordsNumber &&
                      d_queueOpRecordsNumber == other.d_queueOpRecordsNumber &&
                      d_appDetailsMap == other.d_appDetailsMap &&
                      d_queueUri == other.d_queueUri;
        return result;
    }
};

// TYPES

/// Map of queue details per Queue.
typedef bsl::unordered_map<mqbu::StorageKey, QueueDetails> QueueDetailsMap;

// ====================
// struct RecordDetails
// ====================

// Value-semantic type representing record details.
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
    // URI of the queue.
    bsl::string_view d_appId;
    // ID of the application.

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

// ====================
// class MessageDetails
// ====================

class MessageDetails {
  private:
    // DATA
    RecordDetails<mqbs::MessageRecord> d_messageRecord;
    // Message record
    bsl::vector<RecordDetails<mqbs::ConfirmRecord> > d_confirmRecords;
    // All the confirm records related to the `d_messageRecord`
    bsl::optional<RecordDetails<mqbs::DeletionRecord> > d_deleteRecord;
    // Delete record related to the `d_messageRecord`
    bsl::optional<bmqp_ctrlmsg::QueueInfo> d_queueInfo;
    // A pointer to the QueueInfo of the message's queue
    bslma::Allocator* d_allocator_p;
    // Allocator used inside te class

  public:
    // CREATORS

    /// Constructor using the specified arguments.
    explicit MessageDetails(
        const mqbs::MessageRecord&                    record,
        bsls::Types::Uint64                           recordIndex,
        bsls::Types::Uint64                           recordOffset,
        const bsl::optional<bmqp_ctrlmsg::QueueInfo>& queueInfo,
        bslma::Allocator*                             allocator);

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

    /// Return a reference to the non-modifiable message record
    const RecordDetails<mqbs::MessageRecord>& messageRecord() const;

    /// Return a reference to the non-modifiable vector of all the confirm
    /// records related to the `d_messageRecord`
    const bsl::vector<RecordDetails<mqbs::ConfirmRecord> >&
    confirmRecords() const;

    /// Return a reference to the non-modifiable delete record
    const bsl::optional<RecordDetails<mqbs::DeletionRecord> >&
    deleteRecord() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
