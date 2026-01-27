// Copyright 2017-2026 Bloomberg Finance L.P.
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

// mqbu_flowcontroller.cpp                                            -*-C++-*-
#include <mqbu_flowcontroller.h>

namespace BloombergLP {
namespace mqbu {

// --------------------
// class FlowController
// --------------------

FlowController::FlowController()
: d_count(0)
, d_lastUpdateMs(0)
, d_config()
, d_currentRecord(0)
, d_historySize(0)
, d_isHistoryFull()
, d_currentSecondCount(0)
, d_currentSecondMs(0)
, d_totalCount(0)
, d_currentHits(0)
, d_currentAverageWatermark(0)
, d_previousAverageWatermark(0)
, d_currentMaxBurst(0)
, d_previousMaxBurst(0)
{
    for (int i = 0; i < k_HISTORY_SIZE; i++) {
        d_history[i] = 0;
    }
}

FlowController::FlowController::~FlowController()
{
    // NOTHING
}

FlowController::Watermark FlowController::add(int howMany)
{
    if (howMany == 0) {
        return e_Zero;
    }
    d_count += howMany;
    d_currentSecondCount += howMany;

    if (howMany > d_currentMaxBurst) {
        d_currentMaxBurst = howMany;
    }

    Watermark result = e_Low;

    if (d_count > config().burst()) {
        // Over the bucket
        // Need to know what is the current policy and the strict threshold

        if (config().policy() == e_Limit) {
            result = e_Strict;
            // Possibly could Undo by d_count.subtract(howMany);
        }
        else {
            result = e_High;
        }
    }

    return result;
}

void FlowController::checkWatermark(bsls::Types::Int64 lowThreshold,
                                    bsls::Types::Int64 maxRateLimit)
{
    const FlowController::Policy policy    = d_config.policy();
    bsls::Types::Int64           watermark = averageWatermark();

    if (watermark < lowThreshold) {
        if (policy == mqbu::FlowController::e_Limit) {
            if (d_config.ratePerMs() < maxRateLimit) {
                // scale up, by 100

                mqbu::FlowController::Config config(
                    mqbu::FlowController::e_Limit,
                    d_config.ratePerMs() + 100,
                    d_config.burst() + 100);

                configure(config);
            }
            else {
                configure(mqbu::FlowController::Config());
            }
        }
    }
    else {
        mqbu::FlowController::Config config =
            (policy == mqbu::FlowController::e_Limit
                 ? d_config
                 : survey(mqbu::FlowController::e_Limit));

        // scale down, 75% (alternatively, lowThreshold / watermark)

        config.scale(75, 100);

        configure(config);
    }
}

void FlowController::update(bsls::Types::Int64 ms,
                            bsls::Types::Int64 watermark)
{
    const bsls::Types::Int64 deltaMs = ms - d_lastUpdateMs;

    BSLS_ASSERT_SAFE(0 <= deltaMs);

    d_lastUpdateMs = ms;

    const Config& config = d_config;

    if (config.policy() > e_None) {
        bsls::Types::Int64 rate = config.ratePerMs() * deltaMs;

        if (rate > d_count) {
            d_count = 0;
        }
        else {
            d_count -= rate;
        }
    }
    else {
        BSLS_ASSERT_SAFE(config.burst() == 0);

        d_count = 0;  // drain everything
    }

    // Accumulate count per each min within approximately 5 min
    bsls::Types::Int64 elapsedSinceLastSecondMs = d_currentSecondMs + deltaMs;

    while (elapsedSinceLastSecondMs > k_RECORD_MS) {
        // Accumulate the count at the last minute leaving previous whole
        // minutes blank.

        d_totalCount -= d_history[d_currentRecord];
        d_history[d_currentRecord] = d_currentSecondCount;
        d_totalCount += d_currentSecondCount;

        d_currentSecondCount = 0;

        elapsedSinceLastSecondMs -= k_RECORD_MS;

        if (++d_currentRecord == k_HISTORY_SIZE) {
            d_isHistoryFull = true;
            d_currentRecord = 0;
        }
    }

    // the last second
    d_currentSecondMs = elapsedSinceLastSecondMs;

    if (d_currentHits > 10 || deltaMs > 10) {
        // readings are too old.  Advance the window.

        if (deltaMs > 100) {
            // narrow the window down to just one reading ('watermark')
            d_currentAverageWatermark = watermark;
        }
        d_previousAverageWatermark = d_currentAverageWatermark;
        d_currentAverageWatermark  = 0;
        d_currentHits              = 0;

        d_previousMaxBurst = d_currentMaxBurst;
        d_currentMaxBurst  = 0;
    }

    ++d_currentHits;
    d_currentAverageWatermark += (watermark - d_currentAverageWatermark) /
                                 d_currentHits;
}

FlowController::Config FlowController::survey(Policy policy) const
{
    bsls::Types::Int64 sum = d_currentSecondCount + d_totalCount;
    const int numRecords  = d_isHistoryFull ? k_HISTORY_SIZE : d_currentRecord;
    bsls::Types::Int64 ms = d_currentSecondMs + k_RECORD_MS * numRecords;

    // Calculate moving average within approximately 5 min
    // Use 'd_currentMaxBurst' as the burst size

    return Config(policy, sum * 1000 / ms, maxBurst());
}

const FlowController::Config& FlowController::config() const
{
    return d_config;
}

bsl::ostream& FlowController::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);

    stream << "config: [" << d_config << "]" << ", count: " << d_count
           << ", watermark: " << averageWatermark()
           << ", currentSecondCount: " << d_currentSecondCount
           << ", maxBurst: " << maxBurst()
           << ", currentAverageWatermark: " << d_currentAverageWatermark
           << ", previousAverageWatermark: " << d_previousAverageWatermark
           << ", currentSecondHits: " << d_currentHits;

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
