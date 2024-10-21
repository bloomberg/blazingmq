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

// bmqc_orderedhashset.h                                              -*-C++-*-
#ifndef INCLUDED_BMQC_ORDEREDHASHSET
#define INCLUDED_BMQC_ORDEREDHASHSET

//@PURPOSE: Provide a hash set with predictive iteration order.
//
//@CLASSES:
//  bmqc::OrderedHashSet : Hash set with predictive iteration order.
//
//@SEE_ALSO: bmqc::OrderedHashMap
//
//@DESCRIPTION: 'bmqc::OrderedHashSet' is a specialization of
// 'bmqc::OrderedHashMap' in which key and value are of the same type and the
// same storage.  All other features of 'bmqc::OrderedHashMap' are preserved
//

#include <bmqc_orderedhashmap.h>

// BDE
#include <bsl_functional.h>  // for 'bsl::hash'

namespace BloombergLP {
namespace bmqc {

/// Pre-C++11 template typedef
template <typename KEY, typename HASH = bsl::hash<KEY> >
struct OrderedHashSet {
    typedef OrderedHashMap<KEY, KEY, HASH, const KEY> Type;
};

}  // close package namespace
}  // close enterprise namespace

#endif
