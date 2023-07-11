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

// bmqt_hosthealthstate.cpp                                           -*-C++-*-
#include <bmqt_hosthealthstate.h>

#include <bmqscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bsl_ostream.h>

namespace BloombergLP {
namespace bmqt {

// ----------------------
// struct HostHealthState
// ----------------------

bsl::ostream& HostHealthState::print(bsl::ostream&         stream,
                                     HostHealthState::Enum value,
                                     int                   level,
                                     int                   spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << HostHealthState::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* HostHealthState::toAscii(HostHealthState::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(UNKNOWN)
        BMQT_CASE(HEALTHY)
        BMQT_CASE(UNHEALTHY)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool HostHealthState::fromAscii(HostHealthState::Enum*   out,
                                const bslstl::StringRef& str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(HostHealthState::e_##M),       \
                                       str)) {                                \
        *out = HostHealthState::e_##M;                                        \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(UNKNOWN)
    BMQT_CHECKVALUE(HEALTHY)
    BMQT_CHECKVALUE(UNHEALTHY)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

}  // close package namespace
}  // close enterprise namespace
