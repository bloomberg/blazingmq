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

#ifndef INCLUDED_MQBSTAT_TABLEPRINTER
#define INCLUDED_MQBSTAT_TABLEPRINTER

//@PURPOSE: Provide a mechanism to print statistics to streams
//
//@CLASSES:
//  mqbstat::TablePrinter: bmqbrkr statistics printer
//
//@DESCRIPTION: 'mqbstat::TablePrinter' handles the formatting and printing of
// all the statistics to a given stream.  It holds the tables and table info
// providers which can be printed.

// MQB
#include <bmqst_basictableinfoprovider.h>
#include <bmqst_statcontext.h>
#include <bmqst_table.h>

// BDE
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbstat {

// ==================
// class TablePrinter
// ==================

// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
class TablePrinter {
  public:
    // PUBLIC TYPES
    typedef bsl::unordered_map<bsl::string, bmqst::StatContext*>
        StatContextsMap;

  private:
    // PRIVATE TYPES

    /// Context including table and tip for printing and statcontext for
    /// stats.
    // NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
    struct Context {
        /// Stat Context pointer
        bmqst::StatContext* d_statContext_p;

        /// Table
        bmqst::Table d_table;

        /// tip
        bmqst::BasicTableInfoProvider d_tip;
    };
    // NOLINTEND(cppcoreguidelines-pro-type-member-init)

    typedef bsl::shared_ptr<Context>                   ContextSp;
    typedef bsl::unordered_map<bsl::string, ContextSp> ContextsMap;

    // DATA

    /// HiRes timer value of the last allocator snapshot.
    bsls::Types::Int64 d_lastAllocatorSnapshot;

    /// Contexts map
    ContextsMap d_contexts;

    // NOT IMPLEMENTED
    TablePrinter(const TablePrinter& other) BSLS_KEYWORD_DELETED;
    TablePrinter& operator=(const TablePrinter& other) BSLS_KEYWORD_DELETED;

    // PRIVATE MANIPULATORS

    /// Initialize table and tips.
    void initializeTablesAndTips(int historySize);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TablePrinter, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `TablePrinter` object, using the specified
    /// `statContextsMap`, `historySize` and the specified `allocator` for
    /// memory allocation.
    explicit TablePrinter(const StatContextsMap& statContextsMap,
                          int                    historySize,
                          bslma::Allocator*      allocator);

    // MANIPULATORS

    /// Print the stats with the specified `statId` to the specified
    /// `stream`.  Negative `statId` suppresses the header.
    ///
    /// THREAD: This method is called in the `snapshot` thread.
    void printStats(bsl::ostream& stream, int statId);
};
// NOLINTEND(cppcoreguidelines-special-member-functions)

}  // close package namespace
}  // close enterprise namespace

#endif
