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

// bmqt_queueflags.cpp                                                -*-C++-*-
#include <bmqt_queueflags.h>

#include <bmqscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdlb_tokenizer.h>
#include <bsl_cstddef.h>  // for 'size_t'
#include <bsl_ostream.h>

namespace BloombergLP {
namespace bmqt {

// ---------------------
// struct QueueFlagsUtil
// ---------------------

bool QueueFlagsUtil::isValid(bsl::ostream&       errorDescription,
                             bsls::Types::Uint64 flags)
{
    bool res = true;

    if (isSet(flags, QueueFlags::e_ADMIN)) {
        errorDescription
            << "'ADMIN' mode is valid only for BlazingMQ admin tasks.";
        return false;  // RETURN
    }

    // A queue must be open in at least one of the 'Read' or 'Write' mode
    if (!isSet(flags, QueueFlags::e_READ) &&
        !isSet(flags, QueueFlags::e_WRITE)) {
        errorDescription << "At least one of 'READ' or 'WRITE' mode must be "
                         << "set.";
        res = false;
    }

    return res;
}

bsls::Types::Uint64 QueueFlagsUtil::additions(bsls::Types::Uint64 oldFlags,
                                              bsls::Types::Uint64 newFlags)
{
    // We only want the bits that are set in 'newFlags' and not in 'oldFlags'
    return ~oldFlags & newFlags;
}

bsls::Types::Uint64 QueueFlagsUtil::removals(bsls::Types::Uint64 oldFlags,
                                             bsls::Types::Uint64 newFlags)
{
    // We only want the bits that are not set in 'newFlags' and were in
    // 'oldFlags'
    return ~newFlags & oldFlags;
}

bsl::ostream& QueueFlagsUtil::prettyPrint(bsl::ostream&       stream,
                                          bsls::Types::Uint64 flags)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (flags & QueueFlags::e_##M) {                                          \
        stream << (first ? "" : ",")                                          \
               << QueueFlags::toAscii(QueueFlags::e_##M);                     \
        first = false;                                                        \
    }

    bool first = true;

    BMQT_CHECKVALUE(ADMIN)
    BMQT_CHECKVALUE(READ)
    BMQT_CHECKVALUE(WRITE)
    BMQT_CHECKVALUE(ACK)

    return stream;

#undef BMQT_CHECKVALUE
}

int QueueFlagsUtil::fromString(bsl::ostream&        errorDescription,
                               bsls::Types::Uint64* out,
                               const bsl::string&   str)
{
    int rc = 0;
    *out   = 0;

    bdlb::Tokenizer tokenizer(str, ",");
    for (bdlb::TokenizerIterator it = tokenizer.begin(); it != tokenizer.end();
         ++it) {
        QueueFlags::Enum value;
        if (QueueFlags::fromAscii(&value, *it) == false) {
            if (rc == 0) {  // First wrong flag
                errorDescription << "Invalid flag(s) '" << *it << "'";
            }
            else {
                errorDescription << ",'" << *it << "'";
            }
            rc = -1;
        }
        else {
            *out |= value;
        }
    }

    return rc;
}

// -----------------
// struct QueueFlags
// -----------------

bsl::ostream& QueueFlags::print(bsl::ostream&    stream,
                                QueueFlags::Enum value,
                                int              level,
                                int              spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << QueueFlags::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* QueueFlags::toAscii(QueueFlags::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(ADMIN)
        BMQT_CASE(READ)
        BMQT_CASE(WRITE)
        BMQT_CASE(ACK)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool QueueFlags::fromAscii(QueueFlags::Enum* out, const bslstl::StringRef& str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(QueueFlags::e_##M),            \
                                       str.data(),                            \
                                       static_cast<int>(str.length()))) {     \
        *out = QueueFlags::e_##M;                                             \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(ADMIN)
    BMQT_CHECKVALUE(READ)
    BMQT_CHECKVALUE(WRITE)
    BMQT_CHECKVALUE(ACK)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

}  // close package namespace
}  // close enterprise namespace
