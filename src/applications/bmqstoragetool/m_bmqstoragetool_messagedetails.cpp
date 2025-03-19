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

#include <m_bmqstoragetool_messagedetails.h>

// BDE
#include <bslma_allocator.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>

// BMQ
#include <bmqu_memoutstream.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// ====================
// class AppKeyMatcher
// ====================

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

// =====================
// class MessageDetails
// =====================

// CREATORS
MessageDetails::MessageDetails(
    const mqbs::MessageRecord&                    record,
    bsls::Types::Uint64                           recordIndex,
    bsls::Types::Uint64                           recordOffset,
    const bsl::optional<bmqp_ctrlmsg::QueueInfo>& queueInfo,
    bslma::Allocator*                             allocator)
: d_messageRecord(RecordDetails<mqbs::MessageRecord>(record,
                                                     recordIndex,
                                                     recordOffset,
                                                     allocator))
, d_confirmRecords(allocator)
, d_deleteRecord()
, d_queueInfo(queueInfo)
, d_allocator_p(allocator)
{
    if (d_queueInfo.has_value()) {
        d_messageRecord.d_queueUri = d_queueInfo->uri();
    }
}

void MessageDetails::addConfirmRecord(const mqbs::ConfirmRecord& record,
                                      bsls::Types::Uint64        recordIndex,
                                      bsls::Types::Uint64        recordOffset)
{
    d_confirmRecords.push_back(
        RecordDetails<mqbs::ConfirmRecord>(record,
                                           recordIndex,
                                           recordOffset,
                                           d_allocator_p));
    if (d_queueInfo.has_value()) {
        RecordDetails<mqbs::ConfirmRecord>& details =
            *d_confirmRecords.rbegin();
        details.d_queueUri = d_queueInfo->uri();
        if (!findQueueAppIdByAppKey(&details.d_appId,
                                    d_queueInfo->appIds(),
                                    record.appKey())) {
            details.d_appId = "** NULL **";
        }
    }
}

void MessageDetails::addDeleteRecord(const mqbs::DeletionRecord& record,
                                     bsls::Types::Uint64         recordIndex,
                                     bsls::Types::Uint64         recordOffset)
{
    d_deleteRecord.emplace(RecordDetails<mqbs::DeletionRecord>(record,
                                                               recordIndex,
                                                               recordOffset,
                                                               d_allocator_p));
    if (d_queueInfo.has_value())
        d_deleteRecord->d_queueUri = d_queueInfo->uri();
}

const RecordDetails<mqbs::MessageRecord>& MessageDetails::messageRecord() const
{
    return d_messageRecord;
}

const bsl::vector<RecordDetails<mqbs::ConfirmRecord> >&
MessageDetails::confirmRecords() const
{
    return d_confirmRecords;
}

const bsl::optional<RecordDetails<mqbs::DeletionRecord> >&
MessageDetails::deleteRecord() const
{
    return d_deleteRecord;
}

}  // close package namespace
}  // close enterprise namespace
