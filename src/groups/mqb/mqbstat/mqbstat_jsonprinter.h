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

//@PURPOSE: Provide a mechanism to print statistics as a JSON
//
//@CLASSES:
//  mqbstat::JsonPrinter: statistics printer to JSON
//
//@DESCRIPTION: 'mqbstat::JsonPrinter' handles the printing of the statistics
// as a compact or pretty JSON.  It is responsible solely for printing, so any
// statistics updates (e.g. making a new snapshot of the used StatContexts)
// must be done before calling to this component.

// MWC
#include <mwcst_statcontext.h>

// BDE
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>

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
    // DATA
    /// Managed pointer to the printer implementation.
    bslma::ManagedPtr<JsonPrinterImpl> d_impl_mp;

  private:
    // NOT IMPLEMENTED
    JsonPrinter(const JsonPrinter& other) BSLS_CPP11_DELETED;
    JsonPrinter& operator=(const JsonPrinter& other) BSLS_CPP11_DELETED;

  public:
    // PUBLIC TYPES
    typedef bsl::unordered_map<bsl::string, mwcst::StatContext*>
        StatContextsMap;

    // CREATORS

    /// Create a new `JsonPrinter` object, using the specified
    /// `statContextsMap` and the optionally specified `allocator`.
    explicit JsonPrinter(const StatContextsMap& statContextsMap,
                         bslma::Allocator*      allocator = 0);

    // ACCESSORS

    /// Print the JSON-encoded stats to the specified `out`.
    /// If the specified `compact` flag is `true`, the JSON is printed in
    /// compact form, otherwise the JSON is printed in pretty form.
    /// Return `0` on success, and non-zero return code on failure.
    ///
    /// THREAD: This method is called in the *StatController scheduler* thread.
    int printStats(bsl::string* out, bool compact) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
