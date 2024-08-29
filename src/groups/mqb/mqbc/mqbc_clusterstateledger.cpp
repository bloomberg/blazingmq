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

// mqbc_clusterstateledger.cpp                                        -*-C++-*-
#include <mqbc_clusterstateledger.h>

#include <mqbscm_version.h>
// MQB
#include <mqbnet_cluster.h>

// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bsl_iostream.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace mqbc {

// ------------------------------------
// struct ClusterStateLedgerConsistency
// ------------------------------------

bsl::ostream&
ClusterStateLedgerConsistency::print(bsl::ostream& stream,
                                     ClusterStateLedgerConsistency::Enum value,
                                     int                                 level,
                                     int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ClusterStateLedgerConsistency::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ClusterStateLedgerConsistency::toAscii(
    ClusterStateLedgerConsistency::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(EVENTUAL)
        CASE(STRONG)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ClusterStateLedgerConsistency::fromAscii(
    ClusterStateLedgerConsistency::Enum* out,
    const bslstl::StringRef&             str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(                                       \
            toAscii(ClusterStateLedgerConsistency::e_##M),                    \
            str.data(),                                                       \
            static_cast<int>(str.length()))) {                                \
        *out = ClusterStateLedgerConsistency::e_##M;                          \
        return true;                                                          \
    }

    CHECKVALUE(EVENTUAL)
    CHECKVALUE(STRONG)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -------------------------------------
// struct ClusterStateLedgerCommitStatus
// -------------------------------------

bsl::ostream& ClusterStateLedgerCommitStatus::print(
    bsl::ostream&                        stream,
    ClusterStateLedgerCommitStatus::Enum value,
    int                                  level,
    int                                  spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ClusterStateLedgerCommitStatus::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ClusterStateLedgerCommitStatus::toAscii(
    ClusterStateLedgerCommitStatus::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SUCCESS)
        CASE(CANCELED)
        CASE(TIMEOUT)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ClusterStateLedgerCommitStatus::fromAscii(
    ClusterStateLedgerCommitStatus::Enum* out,
    const bslstl::StringRef&              str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(                                       \
            toAscii(ClusterStateLedgerCommitStatus::e_##M),                   \
            str.data(),                                                       \
            static_cast<int>(str.length()))) {                                \
        *out = ClusterStateLedgerCommitStatus::e_##M;                         \
        return true;                                                          \
    }

    CHECKVALUE(SUCCESS)
    CHECKVALUE(CANCELED)
    CHECKVALUE(TIMEOUT)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ------------------------
// class ClusterStateLedger
// ------------------------

// CREATORS
ClusterStateLedger::~ClusterStateLedger()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
