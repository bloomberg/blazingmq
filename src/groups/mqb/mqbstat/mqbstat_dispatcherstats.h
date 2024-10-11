// Copyright 2024 Bloomberg Finance L.P.
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
namespace mwcst {
class StatContext;
}

namespace mqbstat {

// =====================
// class DispatcherStats
// =====================

/// Mechanism to keep track of individual overall statistics of a dispatcher
class DispatcherStats {
  public:
    // TYPES

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum {
            e_ENQ_START = 0,

            e_ENQ_UNDEFINED = e_ENQ_START,
            e_ENQ_DISPATCHER,
            e_ENQ_CALLBACK,
            e_ENQ_CONTROL_MSG,
            e_ENQ_CONFIRM,
            e_ENQ_REJECT,
            e_ENQ_PUSH,
            e_ENQ_PUT,
            e_ENQ_ACK,
            e_ENQ_CLUSTER_STATE,
            e_ENQ_STORAGE,
            e_ENQ_RECOVERY,
            e_ENQ_REPLICATION_RECEIPT,

            e_ENQ_END = e_ENQ_REPLICATION_RECEIPT,

            e_DONE_START = e_ENQ_END + 1,

            e_DONE_UNDEFINED = e_DONE_START,
            e_DONE_DISPATCHER,
            e_DONE_CALLBACK,
            e_DONE_CONTROL_MSG,
            e_DONE_CONFIRM,
            e_DONE_REJECT,
            e_DONE_PUSH,
            e_DONE_PUT,
            e_DONE_ACK,
            e_DONE_CLUSTER_STATE,
            e_DONE_STORAGE,
            e_DONE_RECOVERY,
            e_DONE_REPLICATION_RECEIPT,

            e_DONE_END = e_DONE_REPLICATION_RECEIPT,

            e_CLIENT_COUNT
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

    /// Initialize the statistics for the domain stat context, keeping the
    /// specified `historySize` of history.  Return the created top level
    /// stat context to use for all domain level statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bsl::shared_ptr<mwcst::StatContext>
    initializeStatContext(int historySize, bslma::Allocator* allocator);

    /// Initialize the statistics for the domain stat context, keeping the
    /// specified `historySize` of history.  Return the created top level
    /// stat context to use for all domain level statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bslma::ManagedPtr<mwcst::StatContext>
    initializeClientStatContext(mwcst::StatContext*      parent,
                                const bslstl::StringRef& name,
                                bslma::Allocator*        allocator);
};

}  // close package namespace
}  // close enterprise namespace

#endif
