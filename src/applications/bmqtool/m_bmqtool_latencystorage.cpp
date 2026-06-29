// Copyright 2026 Bloomberg Finance L.P.
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

// BMQTOOL
#include <m_bmqtool_latencystorage.h>

// BDE
#include <bsl_cmath.h>
#include <bsl_fstream.h>
#include <bslma_default.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// BMQ
#include <bmqu_printutil.h>

namespace BloombergLP {
namespace m_bmqtool {

LatencyStorage::LatencyStorage(bsl::string_view  origin,
                               int               latencyDigits,
                               bslma::Allocator* allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_origin(origin, d_allocator_p)
, d_digitsLimit(1)
, d_totalCount(0)
, d_latencies(d_allocator_p)
{
    BSLS_ASSERT_SAFE(1 <= latencyDigits && latencyDigits <= 9);

    for (int i = 0; i < latencyDigits; ++i) {
        d_digitsLimit *= 10;
    }
}

void LatencyStorage::insert(bsls::Types::Int64 latency)
{
    BSLS_ASSERT_SAFE(0 <= latency);

    bsls::Types::Int64 divisor = 1;
    while (divisor * d_digitsLimit < latency) {
        divisor *= 10;
    }

    bsls::Types::Int64 rounded = (latency / divisor) * divisor;

    ++d_latencies[rounded];
    ++d_totalCount;
}

bsls::Types::Int64 LatencyStorage::computePercentile(double percentile) const
{
    BSLS_ASSERT_SAFE(0 <= percentile && percentile <= 100.0);

    if (d_totalCount == 0) {
        return 0;  // RETURN
    }

    bsls::Types::Int64 targetCount = static_cast<bsls::Types::Int64>(
        bsl::ceil(percentile * static_cast<double>(d_totalCount) / 100.0));

    bsls::Types::Int64 accumulated = 0;
    for (LatencyMap::const_iterator it = d_latencies.begin();
         it != d_latencies.end();
         ++it) {
        accumulated += it->second;
        if (targetCount <= accumulated) {
            return it->first;  // RETURN
        }
    }

    // We should be able to find the required bucket in the loop,
    // keep this return for safety.
    return d_latencies.rbegin()->first;  // RETURN
}

bsls::Types::Int64 LatencyStorage::minLatency() const
{
    if (d_latencies.empty()) {
        return 0;  // RETURN
    }
    return d_latencies.begin()->first;
}

bsls::Types::Int64 LatencyStorage::maxLatency() const
{
    if (d_latencies.empty()) {
        return 0;  // RETURN
    }
    return d_latencies.rbegin()->first;
}

bsls::Types::Int64 LatencyStorage::avgLatency() const
{
    if (d_totalCount == 0) {
        return 0;  // RETURN
    }

    bsls::Types::Int64 sum = 0;
    for (LatencyMap::const_iterator it = d_latencies.begin();
         it != d_latencies.end();
         ++it) {
        sum += it->first * it->second;
    }
    return sum / d_totalCount;
}

int LatencyStorage::save(const bsl::string& filename) const
{
    bsl::ofstream file(filename.c_str());
    if (!file) {
        return -1;  // RETURN
    }

    file << "{\n";
    file << "  \"origin\": \"" << d_origin << "\",\n";
    file << "  \"min\": " << minLatency() << ",\n";
    file << "  \"max\": " << maxLatency() << ",\n";
    file << "  \"avg\": " << avgLatency() << ",\n";
    file << "  \"median\": " << computePercentile(50) << ",\n";
    file << "  \"95percentile\": " << computePercentile(95) << ",\n";
    file << "  \"96percentile\": " << computePercentile(96) << ",\n";
    file << "  \"97percentile\": " << computePercentile(97) << ",\n";
    file << "  \"98percentile\": " << computePercentile(98) << ",\n";
    file << "  \"99percentile\": " << computePercentile(99) << ",\n";
    file << "  \"dataPoints\": {";
    for (LatencyMap::const_iterator cit = d_latencies.cbegin();
         cit != d_latencies.cend();
         ++cit) {
        if (cit != d_latencies.cbegin()) {
            // Not the first entry, add a separator
            file << ",";
        }
        file << "\n    \"" << cit->first << "\": " << cit->second;
    }
    file << "\n  }\n";
    file << "}\n";

    if (!file) {
        return -2;  // RETURN
    }

    file.close();
    if (!file) {
        return -3;  // RETURN
    }

    return 0;
}

void LatencyStorage::printSummary(bsl::ostream& stream) const
{
#define BMQTOOL_LSTAT(DESC, TIMESTAMP)                                        \
    stream << "    " << (DESC) << ": "                                        \
           << bmqu::PrintUtil::prettyTimeInterval(TIMESTAMP) << "\n";

    stream << "    totalCount......: " << d_totalCount << "\n";
    BMQTOOL_LSTAT("min.............", minLatency());
    BMQTOOL_LSTAT("avg.............", avgLatency());
    BMQTOOL_LSTAT("max.............", maxLatency());
    BMQTOOL_LSTAT("median..........", computePercentile(50));
    BMQTOOL_LSTAT("95Percentile....", computePercentile(95));
    BMQTOOL_LSTAT("96Percentile....", computePercentile(96));
    BMQTOOL_LSTAT("97Percentile....", computePercentile(97));
    BMQTOOL_LSTAT("98Percentile....", computePercentile(98));
    BMQTOOL_LSTAT("99Percentile....", computePercentile(99));

#undef BMQTOOL_LSTAT
}

}  // close package namespace
}  // close enterprise namespace
