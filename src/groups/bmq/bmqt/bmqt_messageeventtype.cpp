// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_messageeventtype.cpp                                          -*-C++-*-
#include <bmqt_messageeventtype.h>

#include <bmqscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bsl_ostream.h>

namespace BloombergLP {
namespace bmqt {

// -----------------------
// struct MessageEventType
// -----------------------

bsl::ostream& MessageEventType::print(bsl::ostream&          stream,
                                      MessageEventType::Enum value,
                                      int                    level,
                                      int                    spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << MessageEventType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* MessageEventType::toAscii(MessageEventType::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(UNDEFINED)
        BMQT_CASE(PUT)
        BMQT_CASE(PUSH)
        BMQT_CASE(ACK)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool MessageEventType::fromAscii(MessageEventType::Enum*  out,
                                 const bslstl::StringRef& str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(MessageEventType::e_##M),      \
                                       str)) {                                \
        *out = MessageEventType::e_##M;                                       \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(UNDEFINED)
    BMQT_CHECKVALUE(PUT)
    BMQT_CHECKVALUE(PUSH)
    BMQT_CHECKVALUE(ACK)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

}  // close package namespace
}  // close enterprise namespace
