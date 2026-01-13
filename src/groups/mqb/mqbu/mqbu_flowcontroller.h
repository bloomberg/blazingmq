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
    // The 'leaky bucket' parameters come from multiple sources:
    //  1.  The parent              (the return value of 'post')
    //  2.  Own config              (the input argument of 'config')
    //  3.  Own overload            (the input argument of 'overload')
    //  4.  'FlowControl' message   (the input argument of 'control')
    //
    // Time for the 'leaky bucket' algorithm should be provided by calling
    // 'update1sec' every 1 sec.
    //
    // The 'leaky bucket' algorithm is thread-safe, so are all methods except
    // for 'update1sec'.  Updating parameters is atomic (consistent).
    //
    // The result of merging parameters from multiple sources is the strictest
    // set of parameters.
    //
    // The only reference kept is to parent (a parent does not keep references
    // children).  Any change gets enforced to all children (in 'post' method)
    // but not to parent.
    // The 'version' of parameters helps to detect changes:
    //  - When child's version is lower than the parent's one, the child must
    //      recalculate ('merge') its parameters and so must do its children.

  public:
    // PUBLIC DATA

    enum Policy {
        e_None = 0  // Just calculate the moving (5 min) average
        ,
        e_Limit = 1  // Enforce rate limit
    };

    class Config {
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
    bsls::Types::Int64 d_lastSecondCount;
    bsls::Types::Int64 d_lastSecondMs;
    bsls::Types::Int64 d_lastSecondMax;

    unsigned int d_highWatermark;

    unsigned int d_pendingLowWatermarks;

  private:
    // PRIVATE MANIPULATORS

  public:
    // CREATORS
    FlowController();

    ~FlowController();

    // MANIPULATORS

    Watermark add(int howMany);

    bool updateWatermark(bsls::Types::Int64 numEvents,
                         bsls::Types::Int64 watermark,
                         int                consecutiveLowWatermarks);
    void update(bsls::Types::Int64 ms);

    void configure(const Config& config);

    // ACCESSORS

    Config survey(Policy policy) const;

    Config config() const;

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
    d_config = config;
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
