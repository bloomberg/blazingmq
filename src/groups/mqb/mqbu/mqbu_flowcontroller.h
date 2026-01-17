// Copyright 2015-2026 Bloomberg Finance L.P.
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

// mqbu_flowcontroller.h                                              -*-C++-*-
#ifndef INCLUDED_MQBU_FLOWCONTROLLER
#define INCLUDED_MQBU_FLOWCONTROLLER

//@PURPOSE: Provide a mechanism to limit data rate
//
//@CLASSES:
//  mqbu::FlowController: mechanism to monitor/ manage a rate of actions.
//
//@DESCRIPTION:
//
/// Thread-safety
///-------------
// This object is *thread* *safe*.
//
/// Usage
///-----
//

// MQB
#include <mqbscm_version.h>

// BDE
#include <bdlb_print.h>
#include <bdlt_timeunitratio.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bsls_atomic.h>

namespace BloombergLP {

namespace mqbu {

class FlowController {
    // Mechanism to apply 'leaky bucket' logic to incoming traffic.

  public:
    // PUBLIC DATA

    enum Policy {
        e_None = 0  // Just calculate the moving (5 min) average
        ,
        e_Limit = 1  // Enforce rate limit
    };

    struct Config {
        // VST for 'leaky bucket' parameters

      private:
        // PRIVATE DATA
        Policy d_policy;

        bsls::Types::Int64 d_ratePerMs;
        // Leaky bucket drain rate.

        bsls::Types::Int64 d_burst;
        // Leaky bucket size.

      public:
        // CREATORS
        Config();
        Config(Policy policy, int rate, int burst);

        // PUBLIC MANIP{ULATORS
        void scale(int weight, int total);
        // Scale the 'd_rate' by the specified 'weight / total' ratio.

        // PUBLIC ACCESSORS
        Policy             policy() const;
        bsls::Types::Int64 ratePerMs() const;
        bsls::Types::Int64 burst() const;

        bsl::ostream&
        print(bsl::ostream& stream, int level, int spacesPerLevel) const;
    };

    enum Watermark {
        e_Low = 0  // No overload
        ,
        e_High = 1  // Resource(s) is(are) at the first high wm
        ,
        e_Strict = 2  // Drop the data
    };

  private:
    // PRIVATE DATA
    bsls::Types::Int64 d_count;
    bsls::Types::Int64 d_lastUpdateMs;

    Config d_config;

    // Average calculation is tread unsafe, driven by 'update' and 'survey'
    // assuming singe-thread (the Registry)
    static const int k_HISTORY_SIZE = 4;
    static const int k_RECORD_MS =
        bdlt::TimeUnitRatio::k_MS_PER_S;  // 1 sec per record

    bsls::Types::Int64 d_history[k_HISTORY_SIZE];
    int                d_lastRecord;
    bsls::Types::Int64 d_currentSecondCount;
    bsls::Types::Int64 d_currentSecondMs;
    bsls::Types::Int64 d_currentSecondMaxBurst;

    bsls::Types::Int64 d_totalCount;

    bsls::Types::Int64 d_currentSecondHits;
    bsls::Types::Int64 d_currentAverageWatermark;
    bsls::Types::Int64 d_previousAverageWatermark;

  private:
    // PRIVATE MANIPULATORS

  public:
    // CREATORS
    FlowController();

    ~FlowController();

    // MANIPULATORS

    Watermark add(int howMany);

    bool checkWatermark(bsls::Types::Int64 lowThreshold,
                        bsls::Types::Int64 highThreshold);
    void update(bsls::Types::Int64 ms, bsls::Types::Int64 watermark);

    void configure(const Config& config);

    // ACCESSORS

    Config survey(Policy policy) const;

    Config config() const;

    bsls::Types::Int64 averageWatermark() const;

    bool isIdle() const;

    bsl::ostream&
    print(bsl::ostream& stream, int level, int spacesPerLevel) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class FlowController::Rate
// --------------------------

inline FlowController::Config::Config()
: d_policy(e_None)
, d_ratePerMs(0)
, d_burst(0)
{
    // NOTHING
}

inline FlowController::Config::Config(Policy policy, int rate, int burst)
: d_policy(policy)
, d_ratePerMs(rate)
, d_burst(burst)
{
    // NOTHING
}

inline FlowController::Policy FlowController::Config::policy() const
{
    return d_policy;
}

inline bsls::Types::Int64 FlowController::Config::ratePerMs() const
{
    return d_ratePerMs;
}

inline bsls::Types::Int64 FlowController::Config::burst() const
{
    return d_burst;
}

inline bsl::ostream& FlowController::Config::print(bsl::ostream& stream,
                                                   int           level,
                                                   int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);

    stream << "policy: " << (policy() == e_None ? "Off" : "On")
           << ", ratePerMs: " << ratePerMs() << ", burst: " << burst();

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

inline void FlowController::Config::scale(int weight, int total)
{
    BSLS_ASSERT_SAFE(total > 0);

    d_ratePerMs = (d_ratePerMs * weight) / total;
    d_burst     = (d_burst * weight) / total;
}

// --------------------
// class FlowController
// --------------------

inline void FlowController::configure(const Config& config)
{
    bsls::Types::Int64 delta = d_config.burst() - config.burst();

    d_config = config;

    // Reset the state to an empty bucket.
    d_count += delta;
}

inline bsls::Types::Int64 FlowController::averageWatermark() const
{
    return (d_previousAverageWatermark + d_currentAverageWatermark) / 2;
}

inline bool FlowController::isIdle() const
{
    return d_totalCount == 0;
}

// FREE OPERATORS
inline bsl::ostream& operator<<(bsl::ostream&                       stream,
                                const mqbu::FlowController::Config& value)
{
    return value.print(stream, 0, -1);
}

inline bsl::ostream& operator<<(bsl::ostream&               stream,
                                const mqbu::FlowController& value)
{
    return value.print(stream, 0, -1);
}

inline bool operator==(const mqbu::FlowController::Config& left,
                       const mqbu::FlowController::Config& right)
{
    return (left.burst() == right.burst() && left.policy() == right.policy() &&
            left.ratePerMs() == right.ratePerMs());
}

inline bool operator!=(const mqbu::FlowController::Config& left,
                       const mqbu::FlowController::Config& right)
{
    return !(left == right);
}

}  // close package namespace
}  // close enterprise namespace

#endif  // INCLUDED_MQBU_FLOWCONTROLLER
