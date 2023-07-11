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

// mqbc_clusterstateledgerprotocol.cpp                                -*-C++-*-
#include <mqbc_clusterstateledgerprotocol.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_bitmaskutil.h>
#include <bdlb_print.h>
#include <bslmf_assert.h>
#include <bsls_alignmentfromtype.h>

namespace BloombergLP {
namespace mqbc {

namespace {
// Compile-time assertions for alignment of various structs in the protocol
BSLMF_ASSERT(1 == bsls::AlignmentFromType<ClusterStateFileHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<ClusterStateRecordHeader>::VALUE);

// Compile-time assertions for size of various structs in the protocol
BSLMF_ASSERT(0 == sizeof(ClusterStateFileHeader) % 4);
BSLMF_ASSERT(0 == sizeof(ClusterStateRecordHeader) % 4);
}  // close unnamed namespace

/// Force variables/symbols definition so they can be used in other files
const int ClusterStateLedgerProtocol::k_VERSION;

// -----------------------------
// struct ClusterStateFileHeader
// -----------------------------

const int ClusterStateFileHeader::k_PROTOCOL_VERSION_MASK =
    bdlb::BitMaskUtil::one(
        ClusterStateFileHeader::k_PROTOCOL_VERSION_START_IDX,
        ClusterStateFileHeader::k_PROTOCOL_VERSION_NUM_BITS);

const int ClusterStateFileHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    ClusterStateFileHeader::k_HEADER_WORDS_START_IDX,
    ClusterStateFileHeader::k_HEADER_WORDS_NUM_BITS);

const unsigned int ClusterStateFileHeader::k_HEADER_NUM_WORDS =
    sizeof(ClusterStateFileHeader) / bmqp::Protocol::k_WORD_SIZE;

// -----------------------------
// struct ClusterStateRecordType
// -----------------------------

bsl::ostream& ClusterStateRecordType::print(bsl::ostream& stream,
                                            ClusterStateRecordType::Enum value,
                                            int                          level,
                                            int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ClusterStateRecordType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ClusterStateRecordType::toAscii(ClusterStateRecordType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(SNAPSHOT)
        CASE(UPDATE)
        CASE(COMMIT)
        CASE(ACK)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -------------------------------
// struct ClusterStateRecordHeader
// -------------------------------

const int ClusterStateRecordHeader::k_HEADER_WORDS_MASK =
    bdlb::BitMaskUtil::one(ClusterStateRecordHeader::k_HEADER_WORDS_START_IDX,
                           ClusterStateRecordHeader::k_HEADER_WORDS_NUM_BITS);

const int ClusterStateRecordHeader::k_RECORD_TYPE_MASK =
    bdlb::BitMaskUtil::one(ClusterStateRecordHeader::k_RECORD_TYPE_START_IDX,
                           ClusterStateRecordHeader::k_RECORD_TYPE_NUM_BITS);

const unsigned int ClusterStateRecordHeader::k_HEADER_NUM_WORDS =
    sizeof(ClusterStateRecordHeader) / bmqp::Protocol::k_WORD_SIZE;

}  // close package namespace
}  // close enterprise namespace
