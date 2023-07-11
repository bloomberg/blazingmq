// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbi_cluster.cpp                                                   -*-C++-*-
#include <mqbi_cluster.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bsl_iostream.h>
#include <bsl_ostream.h>

namespace BloombergLP {
namespace mqbi {

// -----------------------
// struct ClusterErrorCode
// -----------------------

bsl::ostream& ClusterErrorCode::print(bsl::ostream&          stream,
                                      ClusterErrorCode::Enum value,
                                      int                    level,
                                      int                    spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ClusterErrorCode::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ClusterErrorCode::toAscii(ClusterErrorCode::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNKNOWN)
        CASE(STOPPING)
        CASE(ACTIVE_LOST)
        CASE(NOT_LEADER)
        CASE(NOT_PRIMARY)
        CASE(NO_PARTITION)
        CASE(NODE_DOWN)
        CASE(UNKNOWN_QUEUE)
        CASE(LIMIT)
        CASE(NOT_FOLLOWER)
        CASE(NOT_REPLICA)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ClusterErrorCode::fromAscii(ClusterErrorCode::Enum*  out,
                                 const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(ClusterErrorCode::e_##M),      \
                                       str)) {                                \
        *out = ClusterErrorCode::e_##M;                                       \
        return true;                                                          \
    }

    CHECKVALUE(UNKNOWN)
    CHECKVALUE(STOPPING)
    CHECKVALUE(ACTIVE_LOST)
    CHECKVALUE(NOT_LEADER)
    CHECKVALUE(NOT_PRIMARY)
    CHECKVALUE(NO_PARTITION)
    CHECKVALUE(NODE_DOWN)
    CHECKVALUE(UNKNOWN_QUEUE)
    CHECKVALUE(LIMIT)
    CHECKVALUE(NOT_FOLLOWER)
    CHECKVALUE(NOT_REPLICA)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -------------
// class Cluster
// -------------

Cluster::~Cluster()
{
    // NOTHING (pure interface)
}

}  // close package namespace
}  // close enterprise namespace
