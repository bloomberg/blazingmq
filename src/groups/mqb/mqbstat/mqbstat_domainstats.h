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

// mqbstat_domainstats.h                                              -*-C++-*-
#ifndef INCLUDED_MQBSTAT_DOMAINSTATS
#define INCLUDED_MQBSTAT_DOMAINSTATS

//@PURPOSE: Provide mechanism to keep track of Domain statistics.
//
//@CLASSES:
//  mqbstat::DomainStats:     Mechanism to maintain stats of a domain
//  mqbstat::DomainStatsUtil: Utilities to initialize statistics
//
//@DESCRIPTION: 'mqbstat::DomainStats' provides a mechanism to keep track of
// domain level statistics.  'mqbstat::DomainStatsUtil' is a utility namespace
// exposing methods to initialize the stat contexts.

// MQB

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbi {
class Domain;
}
namespace bmqst {
class StatContext;
}

namespace mqbstat {

// =================
// class DomainStats
// =================

/// Mechanism to keep track of individual overall statistics of a domain
class DomainStats {
  public:
    // TYPES

    /// Enum representing the various type of events for which statistics
    /// are monitored.
    struct EventType {
        // TYPES
        enum Enum { e_CFG_MSGS, e_CFG_BYTES, e_QUEUE_COUNT };
    };

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum { e_CFG_MSGS, e_CFG_BYTES, e_QUEUE_COUNT };
    };

  private:
    // DATA
    bslma::ManagedPtr<bmqst::StatContext> d_statContext_mp;
    // StatContext

  private:
    // NOT IMPLEMENTED
    DomainStats(const DomainStats&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    DomainStats& operator=(const DomainStats&) BSLS_CPP11_DELETED;

  public:
    // CLASS METHODS

    /// Get the value of the specified `stat` reported to the domain
    /// represented by its associated specified `context` as the difference
    /// between the latest snapshot-ed value (i.e., `snapshotId == 0`) and
    /// the value that was recorded at the specified `snapshotId` snapshots
    /// ago.
    ///
    /// THREAD: This method can only be invoked from the `snapshot` thread.
    static bsls::Types::Int64 getValue(const bmqst::StatContext& context,
                                       int                       snapshotId,
                                       const Stat::Enum&         stat);

    // CREATORS

    /// Create a new object in an uninitialized state.
    DomainStats();

    // MANIPULATORS

    /// Initialize this object for the domain with the specified `name`, and
    /// register it as a subcontext of the specified `domainStatContext` and
    /// using the specified `allocator`.
    void initialize(mqbi::Domain*       domain,
                    bmqst::StatContext* domainStatContext,
                    bslma::Allocator*   allocator);

    /// Update statistics for the event of the specified `type` and with the
    /// specified `value` (depending on the `type`, `value` can represent
    /// the number of bytes, a counter, ...
    void onEvent(EventType::Enum type, bsls::Types::Int64 value);

    /// Return a pointer to the statcontext.
    bmqst::StatContext* statContext();
};

// ======================
// struct DomainStatsUtil
// ======================

/// Utility namespace of methods to initialize domain stats.
struct DomainStatsUtil {
    // CLASS METHODS

    /// Initialize the statistics for the domain stat context, keeping the
    /// specified `historySize` of history.  Return the created top level
    /// stat context to use for all domain level statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bsl::shared_ptr<bmqst::StatContext>
    initializeStatContext(int historySize, bslma::Allocator* allocator);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// class DomainStats
// ------------------

inline bmqst::StatContext* DomainStats::statContext()
{
    return d_statContext_mp.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
