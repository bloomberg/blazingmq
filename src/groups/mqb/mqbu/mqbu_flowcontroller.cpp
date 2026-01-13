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

// mqbu_flowcontroller.h                                              -*-C++-*-
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
, d_lastRecord(0)
, d_lastSecondCount(0)
, d_lastSecondMs(0)
, d_lastSecondMax(0)
, d_highWatermark(0)
, d_pendingLowWatermarks(0)
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
        return e_Low;
    }
    d_count += howMany;

    Watermark result = e_Low;

    if (d_count > d_config.burst()) {
        // Over the bucket
        // Need to know what is the current policy and the strict threshold

        if (d_config.policy() == e_Limit) {
            result = e_Strict;
            // Possibly could Undo by d_count.subtract(howMany);
        }
        else {
            result = e_High;
        }
    }

    return result;
}

bool FlowController::updateWatermark(bsls::Types::Int64 watermark,
                                     bsls::Types::Int64 highThreshold,
                                     int consecutiveLowWatermarks)
{
    if (d_lastSecondCount == 0 &&
        d_config.policy() == mqbu::FlowController::e_None) {
        // no data
        return true;  // RETURN
    }

    if (watermark < highThreshold) {
        if (d_pendingLowWatermarks > 0) {
            --d_pendingLowWatermarks;
        }
        else {
            if (d_config.policy() == mqbu::FlowController::e_Limit) {
                configure(mqbu::FlowController::Config());
                d_highWatermark = 0;
            }
            return true;  // RETURN
        }
    }
    else {
        d_pendingLowWatermarks = consecutiveLowWatermarks;
    }

    mqbu::FlowController::Config config = d_config;

    if (config.policy() == mqbu::FlowController::e_None) {
        config          = survey(mqbu::FlowController::e_Limit);
        d_highWatermark = watermark;
    }

    if (d_highWatermark <= watermark) {
        // scale down

        config.scale(75, 100);

        configure(config);
    }

    if (d_count > config.burst() && config.policy() == e_Limit) {
        // Over the bucket
        return false;  // RETURN
    }

    return true;
}

void FlowController::update(bsls::Types::Int64 ms)
{
    const bsls::Types::Int64 deltaMs = ms - d_lastUpdateMs;

    BSLS_ASSERT_SAFE(0 <= deltaMs);

    d_lastUpdateMs = ms;

    const Config& config = d_config;

    bsls::Types::Int64 count = d_count;  // current bucket level
    bsls::Types::Int64 drain = 0;        // drain the bucket

    if (config.policy() > e_None) {
        bsls::Types::Int64 rate = config.ratePerMs() * deltaMs;

        drain = count > rate ? rate : count;
    }
    else {
        BSLS_ASSERT_SAFE(config.burst() == 0);

        drain = count;  // drain everything
    }

    if (drain > 0) {
        d_count -= drain;

        // Accumulate count per each min within approximately 5 min
        bsls::Types::Int64 elapsedSinceLastSecondMs = d_lastSecondMs + deltaMs;
        while (elapsedSinceLastSecondMs > k_RECORD_MS) {
            // Accumulate the count at the last minute leaving previous whole
            // minutes blank.
            d_history[d_lastRecord] = d_lastSecondCount;
            d_lastSecondCount       = 0;
            d_lastSecondMax         = 0;
            elapsedSinceLastSecondMs -= k_RECORD_MS;

            if (++d_lastRecord == k_HISTORY_SIZE) {
                d_lastRecord = 0;
            }
        }
        // the last second
        d_lastSecondCount += count;
        d_lastSecondMs = elapsedSinceLastSecondMs;

        if (count > d_lastSecondMax) {
            d_lastSecondMax = count;
        }
    }
}

FlowController::Config FlowController::survey(Policy policy) const
{
    bsls::Types::Int64 sum = d_lastSecondCount;
    bsls::Types::Int64 ms  = d_lastSecondMs + k_RECORD_MS * k_HISTORY_SIZE;

    // Calculate moving average within approximately 5 min
    for (int i = 0; i < k_HISTORY_SIZE; i++) {
        bsls::Types::Int64 count = d_history[i];
        sum += count;
    }
    // Use 'd_lastMinuteMax' as the burst size
    return Config(policy, sum * 1000 / ms, d_lastSecondMax);
}

FlowController::Config FlowController::config() const
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

    stream << "config: [" << d_config << "]"
           << ", count: " << d_count << ", highNumEvents: " << d_highWatermark
           << ", lastMinuteCount: " << d_lastSecondCount
           << ", lastMinuteMax: " << d_lastSecondMax
           << ", row: " << d_pendingLowWatermarks;

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
