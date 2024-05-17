// Copyright 2022-2024 Bloomberg Finance L.P.
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

// mwcu_safecast.h                                                    -*-C++-*-
#ifndef INCLUDED_MWCU_SAFECAST
#define INCLUDED_MWCU_SAFECAST

//@PURPOSE: Provide utility functions for casting types safely without
// warnings.
//
//@NAMESPACES:
// mwcu::SafeCast
//
//@FUNCTIONS:
//  safeCast:   convert from one numeric type to another
//
//@DESCRIPTION: This component provides a function, 'mwcu::SafeCast::safeCast',
// which implements explicit type coversion in order to suppress '-Wconversion'
// warnings during the compilation.

// BDE
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bslmf_enableif.h>
#include <bslmf_integralconstant.h>
#include <bslmf_issame.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mwcu {

namespace {
template <typename T>
struct is_castable
: bsl::integral_constant<
      bool,
      bsl::is_same<T, unsigned char>::value ||
          bsl::is_same<T, unsigned short int>::value ||
          bsl::is_same<T, unsigned int>::value ||
          bsl::is_same<T, unsigned long int>::value ||
          bsl::is_same<T, unsigned long long int>::value ||
          bsl::is_same<T, char>::value || bsl::is_same<T, short int>::value ||
          bsl::is_same<T, int>::value || bsl::is_same<T, long int>::value ||
          bsl::is_same<T, long long int>::value> {
    // This 'struct' template provides a meta-function to determine whether the
    // (template parameter) 'T' is apt for use as a parameter of 'safeCast'
    // meta-function presented below.
};

/// Explisitly convert the specified 'value' from type 'FROM' to type 'TO' to
/// avoid conversion warnings.  Perform runtime checks of numeric limits to
/// prevent value changes.  Asserts if the conversion is impossible.
template <typename FROM, typename TO>
typename bsl::enable_if<is_castable<FROM>::value && is_castable<TO>::value,
                        TO>::type
safeCast(FROM value)
{
    if (value < FROM(0)) {
        // 'value' is negative
        const TO kMin = bsl::numeric_limits<TO>::min();
        BSLS_ASSERT(static_cast<long long int>(kMin) <=
                    static_cast<long long int>(value));
    }
    else {
        const TO kMax = bsl::numeric_limits<TO>::max();
        BSLS_ASSERT(static_cast<long long unsigned>(value) <=
                    static_cast<long long unsigned>(kMax));
    }

    return static_cast<TO>(value);
}

}

// ===============
// struct SafeCast
// ===============

namespace SafeCast {

// Defenitions of 'safeCast' with only one template parameter for user's
// convenience.  Only result type has to be specified.

template <typename T>
T safe_cast(char value)
{
    return safeCast<char, T>(value);
}

template <typename T>
T safe_cast(short value)
{
    return safeCast<short, T>(value);
}

template <typename T>
T safe_cast(int value)
{
    return safeCast<int, T>(value);
}

template <typename T>
T safe_cast(long value)
{
    return safeCast<long, T>(value);
}

template <typename T>
T safe_cast(long long value)
{
    return safeCast<long long, T>(value);
}

template <typename T>
T safe_cast(unsigned char value)
{
    return safeCast<unsigned char, T>(value);
}

template <typename T>
T safe_cast(unsigned short value)
{
    return safeCast<unsigned short, T>(value);
}

template <typename T>
T safe_cast(unsigned int value)
{
    return safeCast<unsigned int, T>(value);
}

template <typename T>
T safe_cast(unsigned long value)
{
    return safeCast<unsigned long, T>(value);
}

template <typename T>
T safe_cast(unsigned long long value)
{
    return safeCast<unsigned long long, T>(value);
}

}  // close SafeCast namespace

}  // close package namespace

}  // close enterprise namespace

#endif
