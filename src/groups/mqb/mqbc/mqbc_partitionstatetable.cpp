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

// mqbc_partitionstatetable.cpp                                       -*-C++-*-
#include <mqbc_partitionstatetable.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>

namespace BloombergLP {
namespace mqbc {

// -------------------------------
// struct PartitionStateTableState
// -------------------------------

bsl::ostream&
PartitionStateTableState::print(bsl::ostream&                  stream,
                                PartitionStateTableState::Enum value,
                                int                            level,
                                int                            spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << PartitionStateTableState::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
PartitionStateTableState::toAscii(PartitionStateTableState::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNKNOWN)
        CASE(PRIMARY_HEALING_STG1)
        CASE(PRIMARY_HEALING_STG2)
        CASE(REPLICA_HEALING_STG1)
        CASE(REPLICA_HEALING_STG2)
        CASE(PRIMARY_HEALED)
        CASE(REPLICA_HEALED)
        CASE(NUM_STATES)
    default: return "(* UNDEFINED *)";
    }

#undef CASE
}

// -------------------------------
// struct PartitionStateTableEvent
// -------------------------------

bsl::ostream&
PartitionStateTableEvent::print(bsl::ostream&                  stream,
                                PartitionStateTableEvent::Enum value,
                                int                            level,
                                int                            spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << PartitionStateTableEvent::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
PartitionStateTableEvent::toAscii(PartitionStateTableEvent::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(RST_UNKNOWN)
        CASE(DETECT_SELF_PRIMARY)
        CASE(DETECT_SELF_REPLICA)
        CASE(REPLICA_STATE_RQST)
        CASE(REPLICA_STATE_RSPN)
        CASE(FAIL_REPLICA_STATE_RSPN)
        CASE(PRIMARY_STATE_RQST)
        CASE(PRIMARY_STATE_RSPN)
        CASE(FAIL_PRIMARY_STATE_RSPN)
        CASE(REPLICA_DATA_RQST_PULL)
        CASE(REPLICA_DATA_RQST_PUSH)
        CASE(REPLICA_DATA_RQST_DROP)
        CASE(REPLICA_DATA_RSPN_PULL)
        CASE(REPLICA_DATA_RSPN_PUSH)
        CASE(REPLICA_DATA_RSPN_DROP)
        CASE(FAIL_REPLICA_DATA_RSPN_PULL)
        CASE(CRASH_REPLICA_DATA_RSPN_PULL)
        CASE(FAIL_REPLICA_DATA_RSPN_PUSH)
        CASE(FAIL_REPLICA_DATA_RSPN_DROP)
        CASE(DONE_SENDING_DATA_CHUNKS)
        CASE(ERROR_SENDING_DATA_CHUNKS)
        CASE(DONE_RECEIVING_DATA_CHUNKS)
        CASE(ERROR_RECEIVING_DATA_CHUNKS)
        CASE(RECOVERY_DATA)
        CASE(LIVE_DATA)
        CASE(QUORUM_REPLICA_DATA_RSPN)
        CASE(PUT)
        CASE(ISSUE_LIVESTREAM)
        CASE(QUORUM_REPLICA_SEQ)
        CASE(SELF_HIGHEST_SEQ)
        CASE(REPLICA_HIGHEST_SEQ)
        CASE(WATCH_DOG)
        CASE(NUM_EVENTS)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

}  // close package namespace
}  // close enterprise namespace
