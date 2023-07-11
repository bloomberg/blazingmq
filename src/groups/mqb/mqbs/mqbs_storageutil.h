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

// mqbs_storageutil.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBS_STORAGEUTIL
#define INCLUDED_MQBS_STORAGEUTIL

//@PURPOSE: Provide miscellaneous utilities related to BMQ storage.
//
//@CLASSES:
//  mqbs::StorageUtil: Miscellaneous utilities related to BMQ storage
//
//@SEE ALSO: mqbs::StorageCollectionUtil, mqbs::StoragePrintUtil
//
//@DESCRIPTION: 'mqbs::StorageUtil' provides miscellaneous utilities for BMQ
// storage that do not qualify as part of 'mqbs::StorageCollectionUtil' nor
// 'mqbs::StoragePrintUtil'.

// MQB

#include <mqbi_storage.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_storagemessageiterator.h>
#include <bmqt_uri.h>

// MWC
#include <mwcu_blob.h>
#include <mwcu_blobobjectproxy.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlt_datetime.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// ==================
// struct StorageUtil
// ==================

/// This `struct` provides a utility namespace of miscellaneous functions
/// related to BMQ storage.
struct StorageUtil {
  private:
    // CLASS SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBS.STORAGEUTIL");

  public:
    // TYPES

    /// Pair of queueUri and numMessages
    typedef bsl::pair<bmqt::Uri, bsls::Types::Int64> QueueMessagesCount;

    /// [queueUri] -> numMessages
    typedef bsl::unordered_map<bmqt::Uri, bsls::Types::Int64>
        QueueMessagesCountMap;

    /// [domainName][queueUri] -> numMessages
    typedef bsl::unordered_map<bsl::string, QueueMessagesCountMap>
        DomainQueueMessagesCountMap;

  public:
    // CLASS METHODS

    /// Populate the specified `key` buffer with a unique character array of
    /// length `mqbu::StorageKey::k_KEY_LENGTH_BINARY` by hashing the
    /// specified `value`, and also populate the specified `keys` with the
    /// result.  Behaviour is undefined unless `key` and `keys` are not
    /// null.  Note that different invocations of this routine will generate
    /// different storage keys for the same `value`, because `value` is
    /// salted with current time while generating the storage key.
    static void generateStorageKey(mqbu::StorageKey*                     key,
                                   bsl::unordered_set<mqbu::StorageKey>* keys,
                                   const bslstl::StringRef& value);

    /// Return `true` if numMessages in the specified `lhs` is greater than
    /// that of the specified `rhs`.  Return false otherwise.
    static bool queueMessagesCountComparator(const QueueMessagesCount& lhs,
                                             const QueueMessagesCount& rhs);

    /// Merge the queue messages info from the specified `other` into the
    /// specified `out`.
    static void mergeQueueMessagesCountMap(QueueMessagesCountMap*       out,
                                           const QueueMessagesCountMap& other);

    /// Merge the content from the specified `other` into the specified
    /// `out`.
    static void
    mergeDomainQueueMessagesCountMap(DomainQueueMessagesCountMap*       out,
                                     const DomainQueueMessagesCountMap& other);

    /// Load into the specified `out` the arrival time of a message in the
    /// storage having the specified `attributes`, in nanoseconds from
    /// epoch.
    static void
    loadArrivalTime(bsls::Types::Int64*                   out,
                    const mqbi::StorageMessageAttributes& attributes);

    /// Load into the specified `out` the arrival time of a message in the
    /// storage having the specified `attributes`.
    static void
    loadArrivalTime(bdlt::Datetime*                       out,
                    const mqbi::StorageMessageAttributes& attributes);

    /// Load into the specified `out` the length of time that has passed
    /// since the arrival of a message in the storage having the specified
    /// `attributes`, in nanoseconds.
    static void
    loadArrivalTimeDelta(bsls::Types::Int64*                   out,
                         const mqbi::StorageMessageAttributes& attributes);

    /// Load the specified `recordHeader` and `recordPosition` of the
    /// storage header currently pointed by the specified `storageIter` over
    /// the specified `stroageEvent`.  Return 0 on success and non-zero code
    /// on error.  Use the specified `partitionDesc` for logging.
    static int loadRecordHeaderAndPos(
        mwcu::BlobObjectProxy<mqbs::RecordHeader>* recordHeader,
        mwcu::BlobPosition*                        recordPosition,
        const bmqp::StorageMessageIterator&        storageIter,
        const bsl::shared_ptr<bdlbb::Blob>&        stroageEvent,
        const bslstl::StringRef&                   partitionDesc);
};

}  // close package namespace
}  // close enterprise namespace

#endif
