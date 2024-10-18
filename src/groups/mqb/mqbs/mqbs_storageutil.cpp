// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbs_storageutil.cpp                                               -*-C++-*-
#include <mqbs_storageutil.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqp_protocol.h>

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>

// BDE
#include <bdlde_md5.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bdlt_timeunitratio.h>
#include <bsls_performancehint.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// ------------------
// struct StorageUtil
// ------------------

// CLASS METHODS
void StorageUtil::generateStorageKey(
    mqbu::StorageKey*                     key,
    bsl::unordered_set<mqbu::StorageKey>* keys,
    const bslstl::StringRef&              value)
{
    // In order to generate a storage key for 'value', we salt the 'value' with
    // the current time.  During subsequent invocations of this routine, this
    // method is required to generate *different* storage keys than those
    // stored in 'keys' (i.e., previously generated), even if the 'value' is
    // the same.  An example: if a queue is created, deleted and then created
    // again, we want a different storage key to be generated for it for its
    // second instance.

    bdlde::Md5::Md5Digest digest;
    bdlde::Md5            md5(value.data(), value.length());

    bsls::Types::Int64 time = bmqsys::Time::highResolutionTimer();
    md5.update(&time, sizeof(time));

    md5.loadDigestAndReset(&digest);
    key->fromBinary(digest.buffer());

    while (keys->find(*key) != keys->end()) {
        // 'hashKey' already exists. Re-hash the hash, and append the current
        // time to the md5 input data (so that collisions won't potentially
        // degenerate to a long 'linkedList' like, since the hash of the hash
        // has a deterministic value).

        md5.update(digest.buffer(), mqbs::FileStoreProtocol::k_HASH_LENGTH);
        time = bmqsys::Time::highResolutionTimer();
        md5.update(&time, sizeof(time));
        md5.loadDigestAndReset(&digest);
        key->fromBinary(digest.buffer());
    }

    // Found a unique key
    keys->insert(*key);
}

bool StorageUtil::queueMessagesCountComparator(const QueueMessagesCount& lhs,
                                               const QueueMessagesCount& rhs)
{
    return lhs.second > rhs.second;
}

void StorageUtil::mergeQueueMessagesCountMap(
    QueueMessagesCountMap*       out,
    const QueueMessagesCountMap& other)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    for (QueueMessagesCountMap::const_iterator it = other.cbegin();
         it != other.cend();
         ++it) {
        const bsl::pair<QueueMessagesCountMap::iterator, bool> insertRC =
            out->insert(*it);
        if (!insertRC.second) {
            // Entry for that queueUri already existed, need to merge new
            // values.
            //
            // Note: Even though we don't expect a queue to appear in more than
            //       one partition, this logic handles that scenario.
            insertRC.first->second += it->second;
        }
    }
}

void StorageUtil::mergeDomainQueueMessagesCountMap(
    DomainQueueMessagesCountMap*       out,
    const DomainQueueMessagesCountMap& other)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    for (DomainQueueMessagesCountMap::const_iterator it = other.cbegin();
         it != other.cend();
         ++it) {
        const bsl::pair<DomainQueueMessagesCountMap::iterator, bool> insertRC =
            out->insert(*it);
        if (insertRC.second) {
            continue;  // CONTINUE
        }

        // Entry for that domain already existed, need to merge new values.
        mergeQueueMessagesCountMap(&(insertRC.first->second), it->second);
    }
}

void StorageUtil::loadArrivalTime(
    bsls::Types::Int64*                   out,
    const mqbi::StorageMessageAttributes& attributes)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    // If the arrival timepoint of the message is zero (i.e., unset), use the
    // arrival timestamp to calculate the arrival time.  If the primary has not
    // changed or restarted since this message's arrival, arrival timepoint
    // will be non-zero.

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(0 !=
                                            attributes.arrivalTimepoint())) {
        const bsls::Types::Int64 timeDeltaNs =
            bmqsys::Time::highResolutionTimer() -
            attributes.arrivalTimepoint();

        const bsls::Types::Int64 currentTimeNs =
            bdlt::EpochUtil::convertToTimeInterval(bdlt::CurrentTime::utc())
                .totalNanoseconds();

        *out = currentTimeNs - timeDeltaNs;
    }
    else {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        *out = attributes.arrivalTimestamp();

        // Here, time will be in seconds.  Change it to nano seconds.
        *out *= bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND;
    }
}

void StorageUtil::loadArrivalTime(
    bdlt::Datetime*                       out,
    const mqbi::StorageMessageAttributes& attributes)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    bsls::Types::Int64 arrivalTimeNs;
    bsls::TimeInterval arrivalTimeInterval;
    loadArrivalTime(&arrivalTimeNs, attributes);
    arrivalTimeInterval.addNanoseconds(arrivalTimeNs);
    bdlt::EpochUtil::convertFromTimeInterval(out, arrivalTimeInterval);
}

void StorageUtil::loadArrivalTimeDelta(
    bsls::Types::Int64*                   out,
    const mqbi::StorageMessageAttributes& attributes)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    // If the arrival timepoint of the message is zero (i.e., unset), use the
    // arrival timestamp to calculate the arrival time delta.  If the primary
    // has not changed or restarted since this message's arrival, arrival
    // timepoint will be non-zero.

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(0 !=
                                            attributes.arrivalTimepoint())) {
        *out = bmqsys::Time::highResolutionTimer() -
               attributes.arrivalTimepoint();
    }
    else {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        *out = bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()) -
               attributes.arrivalTimestamp();

        // Here, time delta will be in seconds.  Change it to nano seconds.
        *out *= bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND;
    }
}

int StorageUtil::loadRecordHeaderAndPos(
    bmqu::BlobObjectProxy<mqbs::RecordHeader>* recordHeader,
    bmqu::BlobPosition*                        recordPosition,
    const bmqp::StorageMessageIterator&        storageIter,
    const bsl::shared_ptr<bdlbb::Blob>&        stroageEvent,
    const bslstl::StringRef&                   partitionDesc)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(recordHeader);
    BSLS_ASSERT_SAFE(recordPosition);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS            = 0,
        rc_PROTO_VER_MISMATCH = -1,
        rc_INVALID_MSG_TYPE   = -2,
        rc_INVALID_RECORD_POS = -3,
        rc_INVALID_RECORD_HDR = -4
    };

    const bmqp::StorageHeader& header = storageIter.header();

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            header.storageProtocolVersion() != FileStoreProtocol::k_VERSION)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc << "Storage protocol version mismatch. Self: "
            << FileStoreProtocol::k_VERSION
            << ", received: " << header.storageProtocolVersion()
            << ", for type: " << header.messageType()
            << ", with journal offset (in words): "
            << header.journalOffsetWords() << ". Ignoring entire event."
            << BMQTSK_ALARMLOG_END;
        return rc_PROTO_VER_MISMATCH;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp::StorageMessageType::e_UNDEFINED == header.messageType())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc << "Received an unexpected storage message type: "
            << header.messageType() << " with journal offset (in words): "
            << header.journalOffsetWords() << ". Ignoring entire event."
            << BMQTSK_ALARMLOG_END;
        return rc_INVALID_MSG_TYPE;  // RETURN
    }

    const int rc = storageIter.loadDataPosition(recordPosition);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(0 != rc)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc
            << "Failed to load record position for storage msg "
            << header.messageType() << ", with journal offset (in words): "
            << header.journalOffsetWords() << ", rc: " << rc
            << ". Ignoring entire event." << BMQTSK_ALARMLOG_END;
        return rc_INVALID_RECORD_POS;  // RETURN
    }

    recordHeader->reset(stroageEvent.get(),
                        *recordPosition,
                        true,    // read
                        false);  // write
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!recordHeader->isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc << "Failed to read RecordHeader for storage msg "
            << header.messageType() << ", with journal offset (in words): "
            << header.journalOffsetWords() << ". Ignoring entire event."
            << BMQTSK_ALARMLOG_END;
        return rc_INVALID_RECORD_HDR;  // RETURN
    }

    return rc_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
