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

#ifndef INCLUDED_MQBSTAT_FLATJSONPRINTER
#define INCLUDED_MQBSTAT_FLATJSONPRINTER

//@PURPOSE: Provide a mechanism to print statistics as a flat JSON
//
//@CLASSES:
//  mqbstat::FlatJsonPrinter: statistics printer to flat JSON
//
//@DESCRIPTION: 'mqbstat::FlatJsonPrinter' handles the printing of the
// statistics as a compact or pretty JSON.  It is responsible solely for
// printing, so any statistics updates (e.g. making a new snapshot of the used
// StatContexts) must be done before calling to this component.

#include <bmqst_statcontext.h>

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace mqbstat {

// =====================
// class FlatJsonPrinter
// =====================

// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
class FlatJsonPrinter {
  private:
    // PRIVATE TYPES
    /// Forward declaration of the printer implementation type.
    class FlatJsonPrinterImpl;

    // DATA
    /// Managed pointer to the printer implementation.
    bslma::ManagedPtr<FlatJsonPrinterImpl> d_impl_mp;

  private:
    // NOT IMPLEMENTED
    FlatJsonPrinter(const FlatJsonPrinter& other) BSLS_KEYWORD_DELETED;
    FlatJsonPrinter&
    operator=(const FlatJsonPrinter& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FlatJsonPrinter, bslma::UsesBslmaAllocator)

    // PUBLIC TYPES
    typedef bsl::unordered_map<bsl::string, bmqst::StatContext*>
        StatContextsMap;

    // CREATORS

    /// Create a new `FlatJsonPrinter` object, using the specified
    /// `statContextsMap` and the optionally specified `allocator`.
    explicit FlatJsonPrinter(const StatContextsMap& statContextsMap,
                             bslma::Allocator*      allocator = 0);

    // MANIPULATORS

    /// Print the flat JSON-encoded stats to the specified `stream`, using
    /// the specified `statId` to identify the snapshot.
    ///
    /// THREAD: This method is called in the `snapshot` thread.
    void printStats(bsl::ostream& stream, int statId);
};
// NOLINTEND(cppcoreguidelines-special-member-functions)

}  // close package namespace
}  // close enterprise namespace

#endif
