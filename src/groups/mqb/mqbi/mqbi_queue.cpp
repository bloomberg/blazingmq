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

// mqbi_queue.cpp                                                     -*-C++-*-
#include <mqbi_queue.h>

#include <mqbscm_version.h>
// BDE
#include <bsl_iostream.h>

namespace BloombergLP {
namespace mqbi {

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream&                   stream,
                         const QueueHandleReleaseResult& rhs)
{
    stream << "["
           << (rhs.hasNoHandleStreamConsumers() ? " hasNoHandleStreamConsumers"
                                                : "")
           << (rhs.hasNoHandleStreamProducers() ? " hasNoHandleStreamProducers"
                                                : "")
           << (rhs.hasNoQueueStreamConsumers() ? " hasNoQueueStreamConsumers"
                                               : "")
           << (rhs.hasNoQueueStreamProducers() ? " hasNoQueueStreamProducers"
                                               : "")
           << (rhs.hasNoHandleClients() ? " hasNoHandleClients" : "") << " ]";
    return stream;
}

// ---------------------------------
// class QueueHandleRequesterContext
// ---------------------------------

// Initialize `s_previousRequesterId` to -1, so that pre-incremented value
// starts from zero.
bsls::AtomicInt64 QueueHandleRequesterContext::s_previousRequesterId(-1);

// ------------------
// class InlineClient
// ------------------

InlineClient::~InlineClient()
{
    // NOTHING
}

// --------------------------
// class QueueHandleRequester
// --------------------------

QueueHandleRequester::~QueueHandleRequester()
{
    // NOTHING
}

// -----------------------------
// class QueueHandle::StreamInfo
// -----------------------------

QueueHandle::StreamInfo::StreamInfo(const QueueCounts& counts,
                                    unsigned int       downstreamSubQueueId,
                                    unsigned int       upstreamSubQueueId,
                                    bslma::Allocator*  allocator_p)
: d_counts(counts)
, d_downstreamSubQueueId(downstreamSubQueueId)
, d_upstreamSubQueueId(upstreamSubQueueId)
, d_streamParameters(allocator_p)
{
    // NOTHING
}

QueueHandle::StreamInfo::StreamInfo(const StreamInfo& other,
                                    bslma::Allocator* allocator_p)
: d_counts(other.d_counts)
, d_downstreamSubQueueId(other.d_downstreamSubQueueId)
, d_upstreamSubQueueId(other.d_upstreamSubQueueId)
, d_streamParameters(other.d_streamParameters, allocator_p)
{
    // NOTHING
}
// -----------------
// class QueueHandle
// -----------------

QueueHandle::~QueueHandle()
{
    // NOTHING
}

// -----------
// class Queue
// -----------

Queue::~Queue()
{
    // NOTHING
}

// ------------------------
// class QueueHandleFactory
// ------------------------

QueueHandleFactory::~QueueHandleFactory()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
