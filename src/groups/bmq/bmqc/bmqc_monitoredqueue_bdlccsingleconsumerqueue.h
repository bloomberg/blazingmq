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

// bmqc_monitoredqueue_bdlccsingleconsumerqueue.h                     -*-C++-*-
#ifndef INCLUDED_BMQC_MONITOREDQUEUE_BDLCCSINGLECONSUMERQUEUE
#define INCLUDED_BMQC_MONITOREDQUEUE_BDLCCSINGLECONSUMERQUEUE

//@PURPOSE: Provide 'MonitoredQueueTraits' for 'bdlcc::SingleConsumerQueue'.
//
//@CLASSES:
//  MonitoredQueueTraits: specialization for 'bdlcc::SingleConsumerQueue'
//
//@SEE_ALSO: bmqc_monitoredqueue, bdlcc_singleconsumerqueue
//
//@DESCRIPTION: This component defines a partial specialization of
// 'bmqc::MonitoredQueueTraits' that interfaces
// 'bmqc::MonitoredSingleConsumerQueue' with 'bdlcc::SingleConsumerQueue'

#include <bmqc_monitoredqueue.h>

// BDE
#include <bdlcc_singleconsumerqueue.h>
#include <bsl_limits.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>

namespace BloombergLP {

namespace bmqc {

// ==================================================================
// struct MonitoredQueueTraits< bdlcc::SingleConsumerQueue<ELEMENT> >
// ==================================================================

/// This specialization provides the types and functions necessary to
/// interface a `bmqc::MonitoredQueue` with a `bdlcc::SingleConsumerQueue`.
template <typename ELEMENT>
struct MonitoredQueueTraits<bdlcc::SingleConsumerQueue<ELEMENT> > {
    // PUBLIC TYPES
    typedef ELEMENT                             ElementType;
    typedef int                                 InitialCapacityType;
    typedef bdlcc::SingleConsumerQueue<ELEMENT> QueueType;

    // CLASS METHODS

    /// Return the maximum number of elements that may be stored in the
    /// specified `queue`.  See the documentation of
    /// `bdlcc::SingleConsumerQueue` for more details.
    static int capacity(const QueueType& queue);

    /// Return `true` if the specified `queue` is enqueue disabled, and
    /// `false` otherwise.  See the documentation of
    /// `bdlcc::SingleConsumerQueue` for more details.
    static bool isPushBackDisabled(const QueueType& queue);

    /// Disable enqueuing into the specified `queue`.  See the documentation
    /// of `bdlcc::SingleConsumerQueue` for more details.
    static void disablePushBack(QueueType* queue);

    /// Enable enqueuing into the specified `queue`.  See the documentation
    /// of `bdlcc::SingleConsumerQueue` for more details.
    static void enablePushBack(QueueType* queue);

    /// Remove the element from the front of the specified `queue` and load
    /// that element into the specified `value`.  Return 0 on success, and a
    /// non-zero value otherwise.  See the documentation of
    /// `bdlcc::SingleConsumerQueue` for more details.
    static int popFront(QueueType* queue, ElementType* buffer);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------------------------------------------------
// struct MonitoredQueueTraits< bdlcc::SingleConsumerQueue<ELEMENT> >
// ------------------------------------------------------------------

template <typename ELEMENT>
inline bool
MonitoredQueueTraits<bdlcc::SingleConsumerQueue<ELEMENT> >::isPushBackDisabled(
    const QueueType& queue)
{
    return queue.isPushBackDisabled();
}

template <typename ELEMENT>
inline int
MonitoredQueueTraits<bdlcc::SingleConsumerQueue<ELEMENT> >::capacity(
    BSLA_UNUSED const QueueType& queue)
{
    return bsl::numeric_limits<int>::max();
}

template <typename ELEMENT>
inline int
MonitoredQueueTraits<bdlcc::SingleConsumerQueue<ELEMENT> >::popFront(
    QueueType*   queue,
    ElementType* buffer)
{
    return queue->popFront(buffer);
}

template <typename ELEMENT>
inline void
MonitoredQueueTraits<bdlcc::SingleConsumerQueue<ELEMENT> >::disablePushBack(
    QueueType* queue)
{
    queue->disablePushBack();
}

template <typename ELEMENT>
inline void
MonitoredQueueTraits<bdlcc::SingleConsumerQueue<ELEMENT> >::enablePushBack(
    QueueType* queue)
{
    queue->enablePushBack();
}

}  // close package namespace
}  // close enterprise namespace

#endif
