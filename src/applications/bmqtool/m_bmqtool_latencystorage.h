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

#ifndef INCLUDED_M_BMQTOOL_LATENCYSTORAGE
#define INCLUDED_M_BMQTOOL_LATENCYSTORAGE

//@PURPOSE: Provide a container for storing and analyzing latencies.
//
//@CLASSES:
//  m_bmqtool::LatencyStorage: Storage and analysis of latency measurements.
//
//@DESCRIPTION: 'm_bmqtool::LatencyStorage' stores latency measurements
// (in nanoseconds) with automatic rounding to a specified precision for
// bucketing. It provides APIs to compute percentiles, save JSON reports,
// and print human-readable summaries.

// BDE
#include <bsl_map.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace m_bmqtool {

class LatencyStorage {
  private:
    // PRIVATE TYPES
    typedef bsl::map<bsls::Types::Int64, bsls::Types::Int64>
        LatencyMap;  // Maps rounded latency to counter

    // DATA
    bslma::Allocator*  d_allocator_p;
    bsl::string        d_origin;
    bsls::Types::Int64 d_digitsLimit;
    bsls::Types::Int64 d_totalCount;
    LatencyMap         d_latencies;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(LatencyStorage, bslma::UsesBslmaAllocator)

    // CREATORS

    /// @brief Create a LatencyStorage with the specified origin and precision.
    /// @param origin Origin name for JSON reports (e.g., "end2end", "ack")
    /// @param latencyDigits Number of decimal digits for rounding (1-9)
    /// @param allocator Optional allocator for memory (uses default if 0)
    explicit LatencyStorage(bsl::string_view  origin,
                            int               latencyDigits,
                            bslma::Allocator* allocator = 0);

    // MANIPULATORS

    /// @brief Insert a latency sample.
    /// @param latency Latency value in nanoseconds
    void insert(bsls::Types::Int64 latency);

    /// @brief Save the latency report to a JSON file.
    /// @param filename Path to output file
    /// @return 0 on success, non-zero error code on failure
    int save(const bsl::string& filename) const;

    /// @brief Print a summary of latency statistics.
    /// @param stream Output stream to write to
    void printSummary(bsl::ostream& stream) const;

    // ACCESSORS

    /// @brief Return the total number of latencies stored.
    bsls::Types::Int64 totalCount() const;

    /// @brief Compute a percentile latency value.
    /// @param percentile Percentile level (0-100)
    /// @return Latency at the specified percentile, or 0 if empty
    bsls::Types::Int64 computePercentile(double percentile) const;

    /// @brief Get the minimum latency.
    bsls::Types::Int64 minLatency() const;

    /// @brief Get the maximum latency.
    bsls::Types::Int64 maxLatency() const;

    /// @brief Get the average latency.
    bsls::Types::Int64 avgLatency() const;
};

// INLINE DEFINITIONS
inline bsls::Types::Int64 LatencyStorage::totalCount() const
{
    return d_totalCount;
}

}  // close package namespace
}  // close enterprise namespace

#endif  // INCLUDED_M_BMQTOOL_LATENCYSTORAGE
