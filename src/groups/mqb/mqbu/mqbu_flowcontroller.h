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
// limitations under the License.

// mqbu_flowcontroller.h                                              -*-C++-*-
#ifndef INCLUDED_MQBU_FLOWCONTROLLER
#define INCLUDED_MQBU_FLOWCONTROLLER

//@PURPOSE: Provide a mechanism to limit data rate
//
//@CLASSES:
//  mqbu::FlowController: mechanism to monitor/manage a rate of actions.
//
//@DESCRIPTION:
//
/// Thread-safety
///-------------
// This object is *not* thread safe.
//

// MQB
#include <mqbscm_version.h>

// BDE
#include <bdlb_print.h>
#include <bdlt_timeunitratio.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>

namespace BloombergLP {

namespace mqbu {

class FlowController {
    // Mechanism to apply 'leaky bucket' logic to incoming traffic.

  public:
    // PUBLIC DATA

    struct Policy {
        enum Enum {
            /// Just calculate the moving (5 min) average
            e_NONE = 0,
            /// Enforce rate limit
            e_LIMIT = 1
        };
    };

    class Config {
        // VST for 'leaky bucket' parameters

      private:
        // PRIVATE DATA
        Policy::Enum d_policy;

        /// Leaky bucket drain rate.
        int d_ratePerMs;

        /// Leaky bucket size.
        int d_burst;

      public:
        // CREATORS
        Config();
        explicit Config(Policy::Enum policy, int rate, int burst);

        // PUBLIC MANIPULATORS

        /// Scale the rate and burst by the specified 'weight / total' ratio.
        void scale(int weight, int total);

        // PUBLIC ACCESSORS
        Policy::Enum       policy() const;
        bsls::Types::Int64 ratePerMs() const;
        bsls::Types::Int64 burst() const;

        bsl::ostream&
        print(bsl::ostream& stream, int level, int spacesPerLevel) const;
    };

    struct Watermark {
        enum Enum {
            /// No data
            e_ZERO = 0,
            /// No overload
            e_LOW = 1,
            /// Resource(s) is(are) at the first high wm
            e_HIGH = 2,
            /// Drop the data
            e_STRICT = 3
        };
    };

  private:
    // PRIVATE DATA

    /// Number of history records to keep
    static const int k_HISTORY_SIZE = 4;
    /// Timespan of one history record
    static const int k_RECORD_MS =
        bdlt::TimeUnitRatio::k_MS_PER_S;  // 1 sec per record

    /// Current level of the bucket
    bsls::Types::Int64 d_count;

    /// Ms since last `update` call.
    bsls::Types::Int64 d_lastUpdateMs;

    /// Current configuration
    Config d_config;

    /// History records
    bsls::Types::Int64 d_history[k_HISTORY_SIZE];

    bsls::Types::Int64 d_currentAverageWatermark;
    bsls::Types::Int64 d_previousAverageWatermark;

    /// Index of the current record
    int d_currentRecord;

    /// Number of hits since the last history update
    bsls::Types::Int64 d_currentSecondCount;

    /// Ms since the last history update
    bsls::Types::Int64 d_currentSecondMs;

    /// Total count in all history records
    bsls::Types::Int64 d_totalCount;

    /// Number of `update` calls for the window calculating average watermark
    /// and max burst.  Once the number is greater than the window size, save
    /// the current value as `previous` and then reset it.  The result is then
    /// a function (avg or max) of two values: the previous and the current.
    int d_currentHits;

    int d_currentMaxBurst;
    int d_previousMaxBurst;

    bool d_isHistoryFull;

  public:
    // CREATORS
    FlowController();

    ~FlowController();

    // MANIPULATORS

    /// Add to the buck.  Return either `e_Low` if the bucket is not full,
    /// `e_High` - if the bucket is full but the current policy does not limit,
    /// `e_Strict` - if the bucket is full but the current policy does limit.
    Watermark::Enum add(int howMany);

    /// Compare the current watermark with the specified `lowThreshold`.  If
    /// above, enforce limiting policy with 0.75 of either current rate and
    /// burst values if policy already limits or average observed values from
    /// the history.
    void checkWatermark(bsls::Types::Int64 lowThreshold,
                        bsls::Types::Int64 maxRateLimit);

    /// Provide time to the leaky bucket algorithm in the specified `ms`.
    /// Provide current watermark in the specified `watermark`.
    void update(bsls::Types::Int64 ms, bsls::Types::Int64 watermark);

    /// Change the configuration
    void configure(const Config& config);

    // ACCESSORS

    /// Return policy.  Return rate, and burst from the recorded history.
    Config survey(Policy::Enum policy) const;

    /// Return current policy, rate, and burst
    const Config& config() const;

    /// Return average watermark in the window.
    bsls::Types::Int64 averageWatermark() const;

    /// Return max burst in the window.
    int maxBurst() const;

    /// Return `true` if there is no recorded activity and the policy does not
    /// limit.  Return `false` otherwise.
    bool isIdle() const;

    /// Return `true` if the bucket is full and the policy does limit.
    /// Return `false` otherwise.
    bool isFull() const;

    bsl::ostream&
    print(bsl::ostream& stream, int level, int spacesPerLevel) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------------
// class FlowController::Config
// ----------------------------

inline FlowController::Config::Config()
: d_policy(Policy::e_NONE)
, d_ratePerMs(0)
, d_burst(0)
{
    // NOTHING
}

inline FlowController::Config::Config(Policy::Enum policy, int rate, int burst)
: d_policy(policy)
, d_ratePerMs(rate)
, d_burst(burst)
{
    // NOTHING
}

inline FlowController::Policy::Enum FlowController::Config::policy() const
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

    stream << "policy: " << (policy() == Policy::e_NONE ? "Off" : "On")
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

    if (d_burst < d_ratePerMs) {
        d_burst = d_ratePerMs;
    }
}

// --------------------
// class FlowController
// --------------------

inline void FlowController::configure(const Config& config)
{
    d_config = config;
}

inline bsls::Types::Int64 FlowController::averageWatermark() const
{
    return (d_previousAverageWatermark + d_currentAverageWatermark) / 2;
}

inline int FlowController::maxBurst() const
{
    return d_currentMaxBurst > d_previousMaxBurst ? d_currentMaxBurst
                                                  : d_previousMaxBurst;
}

inline bool FlowController::isIdle() const
{
    return d_count == 0 && d_totalCount == 0 &&
           config().policy() == Policy::e_NONE;
}

inline bool FlowController::isFull() const
{
    return (d_count > config().burst() &&
            config().policy() == Policy::e_LIMIT);
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
