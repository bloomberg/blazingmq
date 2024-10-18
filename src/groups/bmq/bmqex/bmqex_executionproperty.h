// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqex_executionproperty.h                                          -*-C++-*-
#ifndef INCLUDED_BMQEX_EXECUTIONPROPERTY
#define INCLUDED_BMQEX_EXECUTIONPROPERTY

//@PURPOSE: Provides utility tag types to be used with 'bmqex::ExecutionPolicy'
//
//@CLASSES:
//  ExecutionProperty: a namespace for utility tag types and enumeration values
//
//@SEE ALSO:
//  bmqex::ExecutionPolicy
//
//@DESCRIPTION:
// This component provides a struct, 'bmqex::ExecutionProperty', that serves as
// a namespace for utility tag types and enumeration values to be used with
// 'bmqex::ExecutionPolicy' to specify the execution policy direction and
// blocking properties.

namespace BloombergLP {
namespace bmqex {

// ========================
// struct ExecutionProperty
// ========================

/// Provides a namespace for utility tag types and enumeration values to be
/// used with `bmqex::ExecutionPolicy` to specify the execution policy
/// direction and blocking properties.
struct ExecutionProperty {
    // TYPES

    /// Provides a tag type defining the One-Way direction property.
    struct OneWay {};

    /// Provides a tag type defining the Two-Way direction property.
    struct TwoWay {};

    /// Provides a tag type defining the Two-Way direction property and the
    /// result type of the execution function.
    template <class R>
    struct TwoWayR {};

    enum Blocking {
        // Provides a enumeration type defining the blocking behavior property.

        e_NEVER_BLOCKING    = 0,
        e_POSSIBLY_BLOCKING = 1,
        e_ALWAYS_BLOCKING   = 2
    };
};

}  // close package namespace
}  // close enterprise namespace

#endif
