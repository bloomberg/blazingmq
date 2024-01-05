// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwcu_noop.h                                                        -*-C++-*-
#ifndef INCLUDED_MWCU_NOOP
#define INCLUDED_MWCU_NOOP

//@PURPOSE: Provide a no-op functor taking an arbitrary number of arguments.
//
//@CLASSES:
//  mwcu::NoOp: a no-op functor.
//
//@DESCRIPTION:
// This component provides a no-op functor, 'mwcu::NoOp', taking an arbitrary
// number of template arguments (up to 9 in C++03) and doing nothing.

// MWC

// BDE
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>

// clang-format off

#if BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES
// Include version that can be compiled with C++03
// Generated on Fri Jan  5 17:20:31 2024
// Command line: sim_cpp11_features.pl mwcu_noop.h
# define COMPILING_MWCU_NOOP_H
# include <mwcu_noop_cpp03.h>
# undef COMPILING_MWCU_NOOP_H
#else

// clang-format on

namespace BloombergLP {
namespace mwcu {

// ==========
// class NoOp
// ==========

/// A no-op functor.
class NoOp {
  public:
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

  public:
    // ACCESSORS
    // clang-format off
#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES // $var-args=9
    // clang-format on

    /// Do nothing.
    template <class... ARGS>
    void operator()(const ARGS&...) const BSLS_KEYWORD_NOEXCEPT;

#endif

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(NoOp, bsl::is_trivially_copyable)
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// ----------
// class NoOp
// ----------

// ACCESSORS
// clang-format off
#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES // $var-args=9
// clang-format on
template <class... ARGS>
inline void NoOp::operator()(const ARGS&...) const BSLS_KEYWORD_NOEXCEPT
{
    // NOTHING
}
#endif

}  // close package namespace
}  // close enterprise namespace

// clang-format off

#endif // End C++11 code

// clang-format on

#endif
