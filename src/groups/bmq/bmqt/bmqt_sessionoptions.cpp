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

// bmqt_sessionoptions.cpp                                            -*-C++-*-
#include <bmqt_sessionoptions.h>

#include <bmqscm_version.h>
// BDE
#include <bslim_printer.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqt {

// --------------------
// class SessionOptions
// --------------------

const char SessionOptions::k_BROKER_DEFAULT_URI[] = "tcp://localhost:30114";

SessionOptions::SessionOptions(bslma::Allocator* allocator)
: d_brokerUri(k_BROKER_DEFAULT_URI, allocator)
, d_processNameOverride(allocator)
, d_numProcessingThreads(1)
, d_blobBufferSize(4 * 1024)
, d_channelHighWatermark(128 * 1024 * 1024)
, d_statsDumpInterval(5 * 60.0)
, d_connectTimeout(60)
, d_disconnectTimeout(30)
, d_openQueueTimeout(k_QUEUE_OPERATION_DEFAULT_TIMEOUT)
, d_configureQueueTimeout(k_QUEUE_OPERATION_DEFAULT_TIMEOUT)
, d_closeQueueTimeout(k_QUEUE_OPERATION_DEFAULT_TIMEOUT)
, d_eventQueueLowWatermark(50)
, d_eventQueueHighWatermark(2 * 1000)
, d_eventQueueSize(-1)  // DEPRECATED: will be removed in future release
, d_hostHealthMonitor_sp(NULL)
, d_dtContext_sp(NULL)
, d_dtTracer_sp(NULL)
{
    // NOTHING
}

SessionOptions::SessionOptions(const SessionOptions& other,
                               bslma::Allocator*     allocator)
: d_brokerUri(other.brokerUri(), allocator)
, d_processNameOverride(other.processNameOverride(), allocator)
, d_numProcessingThreads(other.numProcessingThreads())
, d_blobBufferSize(other.blobBufferSize())
, d_channelHighWatermark(other.channelHighWatermark())
, d_statsDumpInterval(other.statsDumpInterval())
, d_connectTimeout(other.connectTimeout())
, d_disconnectTimeout(other.disconnectTimeout())
, d_openQueueTimeout(other.openQueueTimeout())
, d_configureQueueTimeout(other.configureQueueTimeout())
, d_closeQueueTimeout(other.closeQueueTimeout())
, d_eventQueueLowWatermark(other.eventQueueLowWatermark())
, d_eventQueueHighWatermark(other.eventQueueHighWatermark())
, d_eventQueueSize(-1)  // DEPRECATED: will be removed in future release
, d_hostHealthMonitor_sp(other.hostHealthMonitor())
, d_dtContext_sp(other.traceContext())
, d_dtTracer_sp(other.tracer())
{
    // NOTHING
}

bsl::ostream& SessionOptions::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("brokerUri", d_brokerUri);
    printer.printAttribute("processNameOverride", d_processNameOverride);
    printer.printAttribute("numProcessingThreads", d_numProcessingThreads);
    printer.printAttribute("blobBufferSize", d_blobBufferSize);
    printer.printAttribute("channelHighWatermark", d_channelHighWatermark);
    printer.printAttribute("statsDumpInterval",
                           d_statsDumpInterval.totalSecondsAsDouble());
    printer.printAttribute("connectTimeout",
                           d_connectTimeout.totalSecondsAsDouble());
    printer.printAttribute("disconnectTimeout",
                           d_disconnectTimeout.totalSecondsAsDouble());
    printer.printAttribute("openQueueTimeout",
                           d_openQueueTimeout.totalSecondsAsDouble());
    printer.printAttribute("configureQueueTimeout",
                           d_configureQueueTimeout.totalSecondsAsDouble());
    printer.printAttribute("closeQueueTimeout",
                           d_closeQueueTimeout.totalSecondsAsDouble());
    printer.printAttribute("eventQueueLowWatermark", d_eventQueueLowWatermark);
    printer.printAttribute("eventQueueHighWatermark",
                           d_eventQueueHighWatermark);
    printer.printAttribute("hasHostHealthMonitor",
                           d_hostHealthMonitor_sp != NULL);
    printer.printAttribute("hasDistributedTracing", d_dtTracer_sp != NULL);
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
