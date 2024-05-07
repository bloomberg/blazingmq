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

// mwcst_stringkey.cpp -*-C++-*-
#include <mwcst_stringkey.h>

#include <mwcscm_version.h>
#include <mwcst_printutil.h>

namespace BloombergLP {
namespace mwcst {

// ---------------
// class StringKey
// ---------------

// MANIPULATORS
void StringKey::makeCopy()
{
    if (!d_isOwned) {
        char* newString = (char*)d_allocator_p->allocate(d_length);
        bsl::memcpy(newString, d_string_p, d_length);

        d_string_p = newString;
        d_isOwned  = true;
    }
}

// ACCESSORS
bsl::ostream&
StringKey::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    return mwcst::PrintUtil::stringRefPrint(stream,
                                            bslstl::StringRef(d_string_p,
                                                              d_length),
                                            level,
                                            spacesPerLevel);
}

}  // close package namespace
}  // close enterprise namespace
