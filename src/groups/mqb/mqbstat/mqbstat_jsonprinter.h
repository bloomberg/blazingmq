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

// mqbstat_jsonprinter.h                                              -*-C++-*-
#ifndef INCLUDED_MQBSTAT_JSONPRINTER
#define INCLUDED_MQBSTAT_JSONPRINTER

//@PURPOSE: Provide a mechanism to print statistics as a json
//
//@CLASSES:
//  mqbstat::JsonPrinter: statistics printer to json
//
//@DESCRIPTION: 'mqbstat::JsonPrinter' handles the printing of the statistics
// as a compact or pretty json.  It is responsible solely for printing, so any
// statistics updates (e.g. making a new snapshot of the used StatContexts)
// must be done before calling to this component.

// MQB
#include <mqbcfg_messages.h>

// MWC
#include <mwcst_statcontext.h>

// BDE
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace mqbstat {

// FORWARD DECLARATIONS
namespace {
class JsonPrinterImpl;
}  // close unnamed namespace

// =================
// class JsonPrinter
// =================

class JsonPrinter {
  private:
    // PRIVATE TYPES
    typedef bsl::unordered_map<bsl::string, mwcst::StatContext*>
        StatContextsMap;

  private:
    // DATA
    /// Allocator to use
    bslma::Allocator* d_allocator_p;

    /// Managed pointer to the printer implementation.
    const bslma::ManagedPtr<JsonPrinterImpl> d_impl_mp;

  private:
    // NOT IMPLEMENTED
    JsonPrinter(const JsonPrinter& other) BSLS_CPP11_DELETED;
    JsonPrinter& operator=(const JsonPrinter& other) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(JsonPrinter, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `JsonPrinter` object, using the specified `config`,
    /// `statContextsMap` and the specified `allocator`.
    JsonPrinter(const mqbcfg::StatsConfig& config,
                const StatContextsMap&     statContextsMap,
                bslma::Allocator*          allocator);

    // ACCESSORS

    /// Print the json-encoded stats to the specified `out`.
    /// If the specified `compact` flag is `true`, the json is printed in
    /// compact form, otherwise the json is printed in pretty form.
    /// Return `0` on success, and non-zero return code on failure.
    ///
    /// THREAD: This method is called in the *StatController scheduler* thread.
    int printStats(bsl::string* out, bool compact) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
