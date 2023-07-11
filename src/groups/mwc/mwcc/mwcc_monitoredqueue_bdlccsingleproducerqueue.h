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

// mwcc_monitoredqueue_bdlccsingleproducerqueue.h                     -*-C++-*-
#ifndef INCLUDED_MWCC_MONITOREDQUEUE_BDLCCSINGLEPRODUCERQUEUE
#define INCLUDED_MWCC_MONITOREDQUEUE_BDLCCSINGLEPRODUCERQUEUE

//@PURPOSE: Provide 'MonitoredQueueTraits' for 'bdlcc::SingleProducerQueue'.
//
//@CLASSES:
//  MonitoredQueueTraits: specialization for 'bdlcc::SingleProducerQueue'
//
//@SEE_ALSO: mwcc_monitoredqueue, bdlcc_singleproducerqueue
//
//@DESCRIPTION: This component defines a partial specialization of
// 'mwcc::MonitoredQueueTraits' that interfaces
// 'mwcc::MonitoredSingleProducerQueue' with 'bdlcc::SingleProducerQueue'

// MWC

#include <mwcc_monitoredqueue.h>

// BDE
#include <bdlcc_singleproducerqueue.h>
#include <bsl_limits.h>
#include <bslma_allocator.h>
#include <bsls_annotation.h>

namespace BloombergLP {

namespace mwcc {

// ==================================================================
// struct MonitoredQueueTraits< bdlcc::SingleProducerQueue<ELEMENT> >
// ==================================================================

/// This specialization provides the types and functions necessary to
/// interface a `mwcc::MonitoredQueue` with a `bdlcc::SingleProducerQueue`.
template <typename ELEMENT>
struct MonitoredQueueTraits<bdlcc::SingleProducerQueue<ELEMENT> > {
    // PUBLIC TYPES
    typedef ELEMENT                             ElementType;
    typedef int                                 InitialCapacityType;
    typedef bdlcc::SingleProducerQueue<ELEMENT> QueueType;

    // CLASS METHODS

    /// Return the maximum number of elements that may be stored in the
    /// specified `queue`.  See the documentation of
    /// `bdlcc::SingleProducerQueue` for more details.
    static int capacity(const QueueType& queue);

    /// Return `true` if the specified `queue` is enqueue disabled, and
    /// `false` otherwise.  See the documentation of
    /// `bdlcc::SingleProducerQueue` for more details.
    static bool isPushBackDisabled(const QueueType& queue);

    /// Disable enqueuing into the specified `queue`.  See the documentation
    /// of `bdlcc::SingleProducerQueue` for more details.
    static void disablePushBack(QueueType* queue);

    /// Enable enqueuing into the specified `queue`.  See the documentation
    /// of `bdlcc::SingleProducerQueue` for more details.
    static void enablePushBack(QueueType* queue);

    /// Remove the element from the front of the specified `queue` and load
    /// that element into the specified `value`.  Return 0 on success, and a
    /// non-zero value otherwise.  See the documentation of
    /// `bdlcc::SingleProducerQueue` for more details.
    static int popFront(QueueType* queue, ElementType* buffer);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------------------------------------------------
// struct MonitoredQueueTraits< bdlcc::SingleProducerQueue<ELEMENT> >
// ------------------------------------------------------------------

template <typename ELEMENT>
inline bool
MonitoredQueueTraits<bdlcc::SingleProducerQueue<ELEMENT> >::isPushBackDisabled(
    const QueueType& queue)
{
    return queue.isPushBackDisabled();
}

template <typename ELEMENT>
inline int
MonitoredQueueTraits<bdlcc::SingleProducerQueue<ELEMENT> >::capacity(
    BSLS_ANNOTATION_UNUSED const QueueType& queue)
{
    return bsl::numeric_limits<int>::max();
}

template <typename ELEMENT>
inline int
MonitoredQueueTraits<bdlcc::SingleProducerQueue<ELEMENT> >::popFront(
    QueueType*   queue,
    ElementType* buffer)
{
    return queue->popFront(buffer);
}

template <typename ELEMENT>
inline void
MonitoredQueueTraits<bdlcc::SingleProducerQueue<ELEMENT> >::disablePushBack(
    QueueType* queue)
{
    queue->disablePushBack();
}

template <typename ELEMENT>
inline void
MonitoredQueueTraits<bdlcc::SingleProducerQueue<ELEMENT> >::enablePushBack(
    QueueType* queue)
{
    queue->enablePushBack();
}

}  // close package namespace
}  // close enterprise namespace

#endif
