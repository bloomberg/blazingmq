// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqimp_stat.h                                                      -*-C++-*-
#ifndef INCLUDED_BMQIMP_STAT
#define INCLUDED_BMQIMP_STAT

//@PURPOSE: Provide utilities for stat manipulation.
//
//@CLASSES:
//  bmqimp::Stat:     Struct to hold together context, table and tip
//  bmqimp::StatUtil: Utility functions to operate on stats
//
//@DESCRIPTION: Provide a struct 'bmqimp::Stat' to hold together the
// statContext, as well as the table and tip (with and without delta). Also
// provide 'bmqimp::StatUtil', a utility namespace with functions to operate on
// stats.

// BMQ

// MWC
#include <mwcst_basictableinfoprovider.h>
#include <mwcst_table.h>
#include <mwcst_tablerecords.h>

// BDE
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mwcst {
class StatContext;
}

namespace bmqimp {

// ===========
// struct Stat
// ===========

/// Struct to hold together statContext, table and tip for delta and non
/// delta stats.
struct Stat {
  private:
    // NOT IMPLEMENTED

    /// Not implemented
    Stat(const Stat&);
    Stat& operator=(const Stat&);

  public:
    // PUBLIC DATA
    bslma::ManagedPtr<mwcst::StatContext> d_statContext_mp;
    // StatContext

    mutable mwcst::Table d_table;
    // Table with all data

    mwcu::BasicTableInfoProvider d_tip;
    // Tip for all the data

    mutable mwcst::Table d_tableNoDelta;
    // Table without the delta data

    mwcu::BasicTableInfoProvider d_tipNoDelta;
    // Tip for the no delta data

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Stat, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor using the specified `allocator`.
    explicit Stat(bslma::Allocator* allocator);

    // ACCESSORS

    /// Print the stats to the specified `stream`; print the `delta` stats
    /// column if the specified `includeDelta` is true.
    void printStats(bsl::ostream& stream, bool includeDelta) const;
};

// ===============
// struct StatUtil
// ===============

struct StatUtil {
    // CLASS METHODS

    /// Return true if the specified `record` should be filtered-out because
    /// it represents a TotalValue data; or false otherwise.
    static bool filterDirect(const mwcst::TableRecords::Record& record);

    /// Return true if the specified `record` should be filtered-out because
    /// it represents a TotalValue data or a top level record; or false
    /// otherwise.
    static bool
    filterDirectAndTopLevel(const mwcst::TableRecords::Record& record);
};

}  // close package namespace
}  // close enterprise namespace

#endif
