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

// bmqpi_dtcontext.h                                                  -*-C++-*-
#ifndef INCLUDED_BMQPI_DTCONTEXT
#define INCLUDED_BMQPI_DTCONTEXT

//@PURPOSE: Provide an interface for a context with a notion of a current span.
//
//@CLASSES:
//  bmqpi::DTContext: Interface for a context with a notion of a current span.
//
//@DESCRIPTION:
// 'bmqpi::DTContext' is a pure interface representing a context which defines
// a "current" 'bmqpi::DTSpan' for the calling thread. Implementations *may*
// return different spans for each thread, or return a single span shared by
// all calling threads.
//
// Many distributed trace libraries provide a well-defined thread-local storage
// slot for the span currently under execution, which allows a function to
// obtain a reference to its caller across library boundaries, without changes
// to its API to facilitate dependency injection. This storage slot is set by
// instantiating an RAII "Span Guard" that writes a span to the TLS at
// construction, and reverts its state on destruction (emulating the semantics
// of a call-stack). The API of 'bmqpi::DTContext' aims to make it easy to wrap
// these common patterns in a library-agnostic manner, while also facilitating
// test double implementations.
//

// BMQ

#include <bmqpi_dtspan.h>

// BDE
#include <bsl_memory.h>
#include <bslma_managedptr.h>

namespace BloombergLP {
namespace bmqpi {

// ===============
// class DTContext
// ===============

/// A pure interface for a context with a notion of a current span.
class DTContext {
  public:
    // PUBLIC CREATORS

    /// Destructor
    virtual ~DTContext();

    // PUBLIC ACCESSORS

    /// Returns the current `DTSpan` for the calling thread according to
    /// this `DTContext`, or an empty `shared_ptr` if no span is currently
    /// set.
    virtual bsl::shared_ptr<bmqpi::DTSpan> span() const = 0;

    // PUBLIC MANIPULATORS

    /// Letting `previous = span()` on some thread `T`, returns an object
    /// `token` which promises:
    /// * `span()` will return `newSpan` when called on thread `T`,
    ///    following the construction of `token`.
    /// * `span()` will return `previous` when called on thread `T`,
    ///    following the destruction of `token`.
    ///
    /// Note that if `token2 = scope(otherSpan)` is called on thread `T`
    /// during the lifetime of `token`, then `span()` will return
    /// `otherSpan`. After `token2` is destroyed, it will again return
    /// `newSpan`.
    virtual bslma::ManagedPtr<void>
    scope(const bsl::shared_ptr<DTSpan>& newSpan) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
