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

// bmqst_stringkey.cpp -*-C++-*-
#include <bmqst_stringkey.h>

#include <bmqscm_version.h>
#include <bmqst_printutil.h>

namespace BloombergLP {
namespace bmqst {

// ---------------
// class StringKey
// ---------------

// MANIPULATORS
void StringKey::makeCopy()
{
    if (!d_isOwned) {
        char* newString = static_cast<char*>(
            d_allocator_p->allocate(d_length));
        bsl::memcpy(newString, d_string_p, d_length);

        d_string_p = newString;
        d_isOwned  = true;
    }
}

// ACCESSORS
bsl::ostream&
StringKey::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    return bmqst::PrintUtil::stringRefPrint(stream,
                                            bslstl::StringRef(d_string_p,
                                                              d_length),
                                            level,
                                            spacesPerLevel);
}

}  // close package namespace
}  // close enterprise namespace
