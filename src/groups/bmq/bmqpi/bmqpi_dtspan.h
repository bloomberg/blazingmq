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

// bmqpi_dtspan.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQPI_DTSPAN
#define INCLUDED_BMQPI_DTSPAN

/// @file bmqpi_dtspan.h
///
/// @brief Provide an interface representing a span of a distributed trace.
///
/// @bbref{bmqpi::DTSpan} is a pure interface representing a node within a
/// distributed trace graph (which forms a DAG with edges as invocations).
///
/// @bbref{bmqpi::DTSpan::Baggage} represents a set of key-values used to
/// describe metadata belonging to a @bbref{bmqpi::DTSpan}. The phrase
/// "baggage" is borrowed from the OpenTelemetry standard's terminology.

// BMQ

// BDE
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqpi {

// ============
// class DTSpan
// ============

/// A pure interface for representing a span of a distributed trace.
class DTSpan {
  public:
    // PUBLIC CREATORS

    /// Destructor
    virtual ~DTSpan();

    // PUBLIC ACCESSORS

    /// Returns the name of the operation that this `DTSpan` represents.
    virtual bsl::string_view operation() const = 0;

    // =============
    // class Baggage
    // =============

    // PUBLIC INNER CLASSES

    /// A set of key-values used to describe a `DTSpan`.
    class Baggage BSLS_KEYWORD_FINAL {
      private:
        // PRIVATE TYPEDEFS
        typedef bsl::unordered_map<bsl::string, bsl::string> MapType;

        // PRIVATE DATA
        MapType d_data;

      public:
        // PUBLIC TYPES
        typedef MapType::const_iterator const_iterator;

        // PUBLIC CREATORS
        Baggage(bslma::Allocator* allocator = 0);

        // PUBLIC ACCESSORS

        /// Returns a const-iterator used to iterate over key-values.
        const_iterator begin() const;

        /// Returns a const-iterator representing the end of key-values.
        const_iterator end() const;

        /// Returns whether this object holds a value associated with the
        /// specified `key`.
        bool has(const bsl::string& key) const;

        /// Returns the value currently associated with `key`, or `dflt` if
        /// no associated value is currently held.
        ///
        /// Beware that if `key` is not found and the returned `string_view`
        /// outlives `dflt`, then the `string_view` will reference a
        /// deallocated address.
        bsl::string_view get(const bsl::string&      key,
                             const bsl::string_view& dflt = "") const;

        // PUBLIC MANIPULATORS

        /// Stores the specified `value` associated with the specified
        /// `key`. Note that if such a `key` was already stored, then its
        /// associated value will be replaced by the supplied one.
        void put(const bsl::string_view& key, const bsl::string_view& value);

        /// Erases any value currently associated with `key`, and returns
        /// whether any such value was previously held.
        bool erase(const bsl::string& key);
    };
};

}  // close package namespace
}  // close enterprise namespace

#endif
