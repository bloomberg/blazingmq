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

// mqbstat_dispatcherstats.h                                          -*-C++-*-
#ifndef INCLUDED_MQBSTAT_DISPATCHERSTATS
#define INCLUDED_MQBSTAT_DISPATCHERSTATS

//@PURPOSE: Provide mechanism to keep track of Dispatcher statistics.
//
//@CLASSES:
//  mqbstat::DispatcherStats: Mechanism to maintain stats of a dispatcher
//  mqbstat::DomainStatsUtil: Utilities to initialize statistics
//
//@DESCRIPTION: 'mqbstat::DomainStats' provides a mechanism to keep track of
// domain level statistics.  'mqbstat::DomainStatsUtil' is a utility namespace
// exposing methods to initialize the stat contexts.

// BMQ
#include <bmqst_statcontext.h>

// BDE
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbstat {

// =====================
// class DispatcherStats
// =====================

/// Mechanism to keep track of individual overall statistics of a dispatcher
class DispatcherStats {
  public:
    // TYPES

    /// Enum representing the various type of events for which statistics
    /// are monitored.
    struct EventType {
        // TYPES
        enum Enum {
            k_STAT_QUEUE = 1, // Queue/Dequeue
            k_STAT_TIME  = 2  // Event queued time
        };
    };

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum {
            e_ENQ_1 = 0,
            e_ENQ_2 = 1
        };
    };

  private:
    // NOT IMPLEMENTED
    DispatcherStats(const DispatcherStats&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    DispatcherStats& operator=(const DispatcherStats&) BSLS_CPP11_DELETED;    
};       

// ==========================
// struct DispatcherStatsUtil
// ==========================

/// Utility namespace of methods to initialize dispatcher stats.
struct DispatcherStatsUtil {
    // CLASS METHODS

    /// Initialize the statistics for the dispatcher stat context, keeping the
    /// specified `historySize` of history.  Return the created top level
    /// stat context to use for all dispatcher level statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bsl::shared_ptr<bmqst::StatContext>
    initializeStatContext(int historySize, bslma::Allocator* allocator);

    /// Initialize the statistics for the dispatcher queue stat context,
    /// with the specified `parent` context and `name`.  Return the created 
    /// stat context to use for all dispatcher queue level statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bslma::ManagedPtr<bmqst::StatContext>
    initializeDispatcherQueueStatContext(bmqst::StatContext*      parent,
                                const bslstl::StringRef& name,
                                bslma::Allocator*        allocator);
};

}  // close package namespace
}  // close enterprise namespace

#endif
