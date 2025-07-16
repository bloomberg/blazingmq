// Copyright 2024 Bloomberg Finance L.P.
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

// m_bmqtool_statutil.h                                               -*-C++-*-

#ifndef INCLUDED_M_BMQTOOL_STATUTIL
#define INCLUDED_M_BMQTOOL_STATUTIL

//@PURPOSE: Provide utility routines as well as some constants for statics
// calculations.
//
//@CLASSES:
//  m_bmqtool::StatUtil: Statics calculation routines.
//
//@DESCRIPTION: 'm_bmqtool::StatUtil' provides utility routines for statistics
// calculation.

// BMQTOOL
#include <m_bmqtool_parameters.h>

// BDe
#include <bsl_vector.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace m_bmqtool {

// Index of stats in the stat context
const int k_STAT_MSG = 0;  // Message
const int k_STAT_EVT = 1;  // Event
const int k_STAT_LAT = 2;  // Message Latency

// Dump stats every 1 second
const int k_STAT_DUMP_INTERVAL = 1;

// How often (in ms) should a message be stamped with the latency: because
// computing the current time is expensive, we will only insert the timestamp
// inside a sample subset of the messages (1 every 'k_LATENCY_INTERVAL_MS'
// time), as computed by the configured frequency of message publishing.
const int k_LATENCY_INTERVAL_MS = 5;

struct StatUtil {
    /// Return the current time -in nanoseconds- using either the system time
    /// or the performance timer (depending on the value of the specified
    /// 'resolutionTimer'
    static bsls::Types::Int64
    getNowAsNs(ParametersLatency::Value resolutionTimer);

    /// Compute the `k`th percentile value of the specified `data` (data must
    /// be sorted). Formula:
    ///  - compute the index (k percent * data size)
    ///  - if index is not a whole number, round up to nearest whole number and
    ///    return value at that index
    ///  - if index it not a whole number, compute the average of the data at
    ///    index and index + 1
    static bsls::Types::Int64
    computePercentile(const bsl::vector<bsls::Types::Int64>& data, double k);
};

}  // close package namespace
}  // close enterprise namespace

#endif  // INCLUDED_M_BMQTOOL_STATUTIL
