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

// mwcio_channel.cpp                                                  -*-C++-*-
#include <mwcio_channel.h>

#include <mwcscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>

namespace BloombergLP {
namespace mwcio {

// ---------------------------
// struct ChannelWatermarkType
// ---------------------------

bsl::ostream& ChannelWatermarkType::print(bsl::ostream&              stream,
                                          ChannelWatermarkType::Enum value,
                                          int                        level,
                                          int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ChannelWatermarkType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ChannelWatermarkType::toAscii(ChannelWatermarkType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(LOW_WATERMARK)
        CASE(HIGH_WATERMARK)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ChannelWatermarkType::fromAscii(ChannelWatermarkType::Enum* out,
                                     const bslstl::StringRef&    str)
{
#define CHECKVALUE(M)                                                                            \
    if (bdlb::String::areEqualCaseless(toAscii(ChannelWatermarkType::e_##M),  \
                                       str.data(),                            \
                                       static_cast<int>(str.length())) {                       \
        *out = ChannelWatermarkType::e_##M;                                   \
        return true;                                                          \
    }

    CHECKVALUE(LOW_WATERMARK)
    CHECKVALUE(HIGH_WATERMARK)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -------------
// class Channel
// -------------

Channel::~Channel()
{
    // NOTHING (pure interface)
}

}  // close package namespace
}  // close enterprise namespace
