// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqp_eventutil.h                                                   -*-C++-*-
#ifndef INCLUDED_BMQP_EVENTUTIL
#define INCLUDED_BMQP_EVENTUTIL

//@PURPOSE: Provide utilities for BlazingMQ protocol events.
//
//@CLASSES:
//  bmqp::EventUtil: Utilities for BlazingMQ protocol events
//
//@DESCRIPTION: 'bmqp::EventUtil' provides a set of utility methods to be used
// to manipulate BlazingMQ protocol events.
//
/// Thread Safety
///-------------
// Thread safe.

// BMQ
#include <bmqp_blobpoolutil.h>
#include <bmqp_protocol.h>
#include <bmqp_queueid.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_sharedobjectpool.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace bmqp {

// FORWARD DECLARATION
class PushMessageIterator;
class Event;

// =========================
// struct EventUtilQueueInfo
// =========================

/// VST to uniquely identify a Queue.  If `d_subscriptionId` is not set
/// (`bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID`) then the key is
/// `bmqp::QueueId(d_queueId, bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID)`.
/// Otherwise, the key is `d_subscriptionId`
struct EventUtilQueueInfo {
    unsigned int d_subscriptionId;

    const bmqp::PushHeader d_header;
    const int              d_applicationDataSize;

    EventUtilQueueInfo(unsigned int            subscriptionId,
                       const bmqp::PushHeader& header,
                       int                     applicationDataSize);
};
// =========================
// struct EventUtilEventInfo
// =========================

/// Type containing information used for constructing and populating a valid
/// `bmqimp::Event`.
struct EventUtilEventInfo {
  public:
    // TYPES
    typedef bsl::vector<EventUtilQueueInfo> Ids;

    // PUBLIC DATA
    bdlbb::Blob d_blob;

    Ids d_ids;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(EventUtilEventInfo,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `bmqp::EventUtilEventInfo` object.  All memory allocations
    /// will be done using the specified `allocator`.
    explicit EventUtilEventInfo(bslma::Allocator* allocator);

    /// Create a `bmqp::EventUtil_EventInfo` object using the specified
    /// `blob` and `queueIds`.  All memory allocations will be done using
    /// the specified `allocator`.
    EventUtilEventInfo(const bdlbb::Blob& blob,
                       const Ids&         queueIds,
                       bslma::Allocator*  allocator);

    /// Copy constructor from the specified `original` using the specified
    /// `allocator`.
    EventUtilEventInfo(const EventUtilEventInfo& original,
                       bslma::Allocator*         allocator);
};

// ================
// struct EventUtil
// ================

/// Utilities for BlazingMQ protocol events.
struct EventUtil {
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

    // CLASS METHODS

    /// PushEvent Utilities
    ///-------------------

    /// Flatten the specified `event` (i.e. convert messages with multiple
    /// SubQueueIds to multiple messages with each one of the SubQueueId)
    /// into the specified `eventInfos` using the specified `bufferFactory`
    /// and `allocator`.  Return 0 on success, or non-zero error code in
    /// case of failure.  The behavior is undefined unless the `event` is a
    /// valid push event.  Note that this function is exclusively used by
    /// the SDK and thus we can assume that the messages will only contain
    /// old flavor of SubQueueIdsArray (i.e. SubQueueIdsArrayOld).  Use the
    /// specified `blobSpPool_p` to allocate shared pointer to blobs.
    static int flattenPushEvent(bsl::vector<EventUtilEventInfo>* eventInfos,
                                const Event&                     event,
                                bdlbb::BlobBufferFactory*        bufferFactory,
                                BlobSpPool*                      blobSpPool_p,
                                bslma::Allocator*                allocator);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// struct EventUtilQueueInfo
// --------------------------

inline EventUtilQueueInfo::EventUtilQueueInfo(unsigned int subscriptionId,
                                              const bmqp::PushHeader& header,
                                              int appDataSize)
: d_subscriptionId(subscriptionId)
, d_header(header)
, d_applicationDataSize(appDataSize)
{
    // NOTHING
}

// --------------------------
// struct EventUtil_EventInfo
// --------------------------

// CREATORS
inline EventUtilEventInfo::EventUtilEventInfo(bslma::Allocator* allocator)
: d_blob(allocator)
, d_ids(allocator)
{
    // NOTHING
}

inline EventUtilEventInfo::EventUtilEventInfo(const bdlbb::Blob& blob,
                                              const Ids&         ids,
                                              bslma::Allocator*  allocator)
: d_blob(blob, allocator)
, d_ids(ids, allocator)
{
    // NOTHING
}

inline EventUtilEventInfo::EventUtilEventInfo(
    const EventUtilEventInfo& original,
    bslma::Allocator*         allocator)
: d_blob(original.d_blob, allocator)
, d_ids(original.d_ids, allocator)
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace

#endif
