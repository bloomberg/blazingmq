// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbstat_brokerstats.h                                              -*-C++-*-
#ifndef INCLUDED_MQBSTAT_BROKERSTATS
#define INCLUDED_MQBSTAT_BROKERSTATS

//@PURPOSE: Provide mechanism to keep track of Broker statistics.
//
//@CLASSES:
//  mqbstat::BrokerStats:     Mechanism to maintain stats of a broker
//  mqbstat::BrokerStatsUtil: Utilities to initialize statistics
//
//@DESCRIPTION: 'mqbstat::BrokerStats' provides a mechanism to keep track of
// broker level statistics.  'mqbstat::BrokerStatsUtil' is a utility namespace
// exposing methods to initialize the stat contexts.

// MQB

// BMQ
#include <bmqst_statcontext.h>
#include <bmqt_uri.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbstat {

// =================
// class BrokerStats
// =================

/// Mechanism to keep track of individual overall statistics of a broker
class BrokerStats {
  public:
    // TYPES

    /// Enum representing the various type of events for which statistics
    /// are monitored.
    struct EventType {
        // TYPES
        enum Enum {
            e_CLIENT_CREATED,
            e_CLIENT_DESTROYED,
            e_QUEUE_CREATED,
            e_QUEUE_DESTROYED
        };
    };

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum { e_CLIENT_COUNT, e_QUEUE_COUNT };
    };

  private:
    // CLASS DATA
    static BrokerStats s_instance;

    // DATA
    bmqst::StatContext* d_statContext_p;  // StatContext

  private:
    // NOT IMPLEMENTED
    BrokerStats(const BrokerStats&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    BrokerStats& operator=(const BrokerStats&) BSLS_CPP11_DELETED;

    // CREATORS

    /// Create a new object in an uninitialized state.
    BrokerStats();

  public:
    // CLASS METHODS

    /// Return a reference to the only instance of this class.
    static BrokerStats& instance();

    /// Get the value of the specified `stat` reported to the broker
    /// represented by its associated specified `context` as the difference
    /// between the latest snapshot-ed value (i.e., `snapshotId == 0`) and
    /// the value that was recorded at the specified `snapshotId` snapshots
    /// ago.
    ///
    /// THREAD: This method can only be invoked from the `snapshot` thread.
    static bsls::Types::Int64 getValue(const bmqst::StatContext& context,
                                       int                       snapshotId,
                                       const Stat::Enum&         stat);

    // MANIPULATORS

    /// Initialize this object, and register it as the unique instance of
    /// this class, as returned by the `instance()` class method.  Register
    /// a subcontext of the specified `brokerStatContext`.  This method
    /// ought to be called exactly once.
    void initialize(bmqst::StatContext* brokerStatContext);

    /// Update statistics for the event of the specified `type` and with the
    /// specified `value` (depending on the `type`, `value` can represent
    /// the number of bytes, a counter, ...
    template <EventType::Enum type>
    void onEvent();

    /// Return a pointer to the statcontext.
    bmqst::StatContext* statContext();
};

// ======================
// struct BrokerStatsUtil
// ======================

/// Utility namespace of methods to initialize broker stats.
struct BrokerStatsUtil {
    // CLASS METHODS

    /// Initialize the statistics for the broker stat context, keeping the
    /// specified `historySize` of history.  Return the created top level
    /// stat context to use for all broker level statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bsl::shared_ptr<bmqst::StatContext>
    initializeStatContext(int historySize, bslma::Allocator* allocator);
};

//------------------------
// struct BrokerStatsIndex
//------------------------

/// Namespace for the constants of stat values that applies to the queues
/// from the clients
struct BrokerStatsIndex {
    enum Enum { e_STAT_QUEUE_COUNT, e_STAT_CLIENT_COUNT };
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class BrokerStats
// -----------------

inline bmqst::StatContext* BrokerStats::statContext()
{
    return d_statContext_p;
}

template <>
inline void
BrokerStats::onEvent<BrokerStats::EventType::Enum::e_CLIENT_CREATED>()
{
    BSLS_ASSERT_SAFE(d_statContext_p && "initialize was not called");

    d_statContext_p->adjustValue(BrokerStatsIndex::e_STAT_CLIENT_COUNT, 1);
}

template <>
inline void
BrokerStats::onEvent<BrokerStats::EventType::Enum::e_CLIENT_DESTROYED>()
{
    BSLS_ASSERT_SAFE(d_statContext_p && "initialize was not called");

    d_statContext_p->adjustValue(BrokerStatsIndex::e_STAT_CLIENT_COUNT, -1);
}

template <>
inline void
BrokerStats::onEvent<BrokerStats::EventType::Enum::e_QUEUE_CREATED>()
{
    BSLS_ASSERT_SAFE(d_statContext_p && "initialize was not called");

    d_statContext_p->adjustValue(BrokerStatsIndex::e_STAT_QUEUE_COUNT, 1);
}

template <>
inline void
BrokerStats::onEvent<BrokerStats::EventType::Enum::e_QUEUE_DESTROYED>()
{
    BSLS_ASSERT_SAFE(d_statContext_p && "initialize was not called");

    d_statContext_p->adjustValue(BrokerStatsIndex::e_STAT_QUEUE_COUNT, -1);
}

}  // close package namespace
}  // close enterprise namespace

#endif
