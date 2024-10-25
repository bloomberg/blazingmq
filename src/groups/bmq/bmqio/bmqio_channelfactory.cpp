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

// bmqio_channelfactory.cpp                                           -*-C++-*-
#include <bmqio_channelfactory.h>

#include <bmqscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bsl_ostream.h>

namespace BloombergLP {
namespace bmqio {

// --------------------------
// struct ChannelFactoryEvent
// --------------------------

bsl::ostream& ChannelFactoryEvent::print(bsl::ostream&             stream,
                                         ChannelFactoryEvent::Enum value,
                                         int                       level,
                                         int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ChannelFactoryEvent::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ChannelFactoryEvent::toAscii(ChannelFactoryEvent::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(CHANNEL_UP)
        CASE(CONNECT_ATTEMPT_FAILED)
        CASE(CONNECT_FAILED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -----------------------------------
// class ChannelFactoryOperationHandle
// -----------------------------------

// CREATORS
ChannelFactoryOperationHandle::~ChannelFactoryOperationHandle()
{
    // NOTHING (pure interface)
}

// --------------------
// class ChannelFactory
// --------------------

ChannelFactory::~ChannelFactory()
{
    // NOTHING (pure interface)
}

}  // close package namespace
}  // close enterprise namespace
