// Copyright 2020-2023 Bloomberg Finance L.P.
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

// mqbc_clusterstatetable.cpp                                         -*-C++-*-
#include <mqbc_clusterstatetable.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>

namespace BloombergLP {
namespace mqbc {

// -----------------------------
// struct ClusterStateTableState
// -----------------------------

bsl::ostream& ClusterStateTableState::print(bsl::ostream& stream,
                                            ClusterStateTableState::Enum value,
                                            int                          level,
                                            int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ClusterStateTableState::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ClusterStateTableState::toAscii(ClusterStateTableState::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNKNOWN)
        CASE(FOL_HEALING)
        CASE(LDR_HEALING_STG1)
        CASE(LDR_HEALING_STG2)
        CASE(FOL_HEALED)
        CASE(LDR_HEALED)
        CASE(STOPPING)
        CASE(STOPPED)
        CASE(NUM_STATES)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ClusterStateTableState::fromAscii(ClusterStateTableState::Enum* out,
                                       const bslstl::StringRef&      str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(                                       \
            toAscii(ClusterStateTableState::e_##M),                           \
            str.data(),                                                       \
            static_cast<int>(str.length()))) {                                \
        *out = ClusterStateTableState::e_##M;                                 \
        return true;                                                          \
    }

    CHECKVALUE(UNKNOWN)
    CHECKVALUE(FOL_HEALING)
    CHECKVALUE(LDR_HEALING_STG1)
    CHECKVALUE(LDR_HEALING_STG2)
    CHECKVALUE(FOL_HEALED)
    CHECKVALUE(LDR_HEALED)
    CHECKVALUE(STOPPING)
    CHECKVALUE(STOPPED)
    CHECKVALUE(NUM_STATES)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -----------------------------
// struct ClusterStateTableEvent
// -----------------------------

bsl::ostream& ClusterStateTableEvent::print(bsl::ostream& stream,
                                            ClusterStateTableEvent::Enum value,
                                            int                          level,
                                            int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ClusterStateTableEvent::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ClusterStateTableEvent::toAscii(ClusterStateTableEvent::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SLCT_LDR)
        CASE(SLCT_FOL)
        CASE(FOL_LSN_RQST)
        CASE(FOL_LSN_RSPN)
        CASE(QUORUM_LSN)
        CASE(LOST_QUORUM_LSN)
        CASE(SELF_HIGHEST_LSN)
        CASE(FOL_HIGHEST_LSN)
        CASE(FAIL_FOL_LSN_RSPN)
        CASE(FOL_CSL_RQST)
        CASE(FOL_CSL_RSPN)
        CASE(FAIL_FOL_CSL_RSPN)
        CASE(CRASH_FOL_CSL)
        CASE(STOP_NODE)
        CASE(STOP_SUCCESS)
        CASE(REGISTRATION_RQST)
        CASE(REGISTRATION_RSPN)
        CASE(FAIL_REGISTRATION_RSPN)
        CASE(RST_UNKNOWN)
        CASE(CSL_CMT_SUCCESS)
        CASE(CSL_CMT_FAIL)
        CASE(WATCH_DOG)
        CASE(NUM_EVENTS)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ClusterStateTableEvent::fromAscii(ClusterStateTableEvent::Enum* out,
                                       const bslstl::StringRef&      str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(                                       \
            toAscii(ClusterStateTableEvent::e_##M),                           \
            str.data(),                                                       \
            static_cast<int>(str.length()))) {                                \
        *out = ClusterStateTableEvent::e_##M;                                 \
        return true;                                                          \
    }

    CHECKVALUE(SLCT_LDR)
    CHECKVALUE(SLCT_FOL)
    CHECKVALUE(FOL_LSN_RQST)
    CHECKVALUE(FOL_LSN_RSPN)
    CHECKVALUE(QUORUM_LSN)
    CHECKVALUE(LOST_QUORUM_LSN)
    CHECKVALUE(SELF_HIGHEST_LSN)
    CHECKVALUE(FOL_HIGHEST_LSN)
    CHECKVALUE(FAIL_FOL_LSN_RSPN)
    CHECKVALUE(FOL_CSL_RQST)
    CHECKVALUE(FOL_CSL_RSPN)
    CHECKVALUE(FAIL_FOL_CSL_RSPN)
    CHECKVALUE(CRASH_FOL_CSL)
    CHECKVALUE(STOP_NODE)
    CHECKVALUE(STOP_SUCCESS)
    CHECKVALUE(REGISTRATION_RQST)
    CHECKVALUE(REGISTRATION_RSPN)
    CHECKVALUE(FAIL_REGISTRATION_RSPN)
    CHECKVALUE(RST_UNKNOWN)
    CHECKVALUE(CSL_CMT_SUCCESS)
    CHECKVALUE(CSL_CMT_FAIL)
    CHECKVALUE(WATCH_DOG)
    CHECKVALUE(NUM_EVENTS)

    // Invalid string
    return false;

#undef CHECKVALUE
}

}  // close package namespace
}  // close enterprise namespace
