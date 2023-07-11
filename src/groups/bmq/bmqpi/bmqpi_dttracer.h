// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqpi_dttracer.h                                                   -*-C++-*-
#ifndef INCLUDED_BMQPI_DTTRACER
#define INCLUDED_BMQPI_DTTRACER

//@PURPOSE: Provide an interface that can create new 'DTSpan' objects.
//
//@CLASSES:
//  bmqpi::DTTracer: Interface for creators of 'DTSpan' objects.
//
//@DESCRIPTION:
// 'bmqpi::DTTracer' is a pure interface for creators of new 'DTSpan' objects.

// BMQ

#include <bmqpi_dtspan.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string_view.h>

namespace BloombergLP {
namespace bmqpi {

// ==============
// class DTTracer
// ==============

/// A pure interface for creators of `DTSpan` objects.
class DTTracer {
  public:
    // PUBLIC CREATORS

    /// Destructor
    virtual ~DTTracer();

    // PUBLIC METHODS

    /// Creates and returns a new `DTSpan` representing `operation` as a
    /// child of `parent`, having the key-value tags defined by `baggage`.
    virtual bsl::shared_ptr<DTSpan> createChildSpan(
        const bsl::shared_ptr<DTSpan>& parent,
        const bsl::string_view&        operation,
        const DTSpan::Baggage&         baggage = DTSpan::Baggage()) const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
