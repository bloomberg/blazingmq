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

// mqbmock_clusterstateledgeriterator.cpp                             -*-C++-*-
#include <mqbmock_clusterstateledgeriterator.h>

#include <mqbscm_version.h>
// BDE
#include <bslim_printer.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbmock {

// ---------------------------------
// struct ClusterStateLedgerIterator
// ---------------------------------

// CREATORS
ClusterStateLedgerIterator::ClusterStateLedgerIterator(
    const LedgerRecords& records)
: d_firstNextCall(true)
, d_iter(records.cbegin())
, d_iterEnd(records.cend())
, d_currRecordHeader()
{
    d_currRecordHeader.setHeaderWords(
        mqbc::ClusterStateRecordHeader::k_HEADER_NUM_WORDS);
}

ClusterStateLedgerIterator::~ClusterStateLedgerIterator()
{
    // NOTHING
}

// MANIPULATORS
int ClusterStateLedgerIterator::next()
{
    if (d_iter == d_iterEnd) {
        return 1;  // RETURN
    }

    if (d_firstNextCall) {
        d_firstNextCall = false;
    }
    else {
        ++d_iter;

        if (d_iter == d_iterEnd) {
            return 1;  // RETURN
        }
    }

    const bmqp_ctrlmsg::LeaderMessageSequence* lsn    = 0;
    const bmqp_ctrlmsg::ClusterMessageChoice&  choice = d_iter->choice();
    switch (choice.selectionId()) {
    case MsgChoice::SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        lsn = &choice.partitionPrimaryAdvisory().sequenceNumber();
        d_currRecordHeader.setRecordType(
            mqbc::ClusterStateRecordType::e_UPDATE);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_LEADER_ADVISORY: {
        lsn = &choice.leaderAdvisory().sequenceNumber();
        d_currRecordHeader.setRecordType(
            mqbc::ClusterStateRecordType::e_SNAPSHOT);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        lsn = &choice.queueAssignmentAdvisory().sequenceNumber();
        d_currRecordHeader.setRecordType(
            mqbc::ClusterStateRecordType::e_UPDATE);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        lsn = &choice.queueUnassignedAdvisory().sequenceNumber();
        d_currRecordHeader.setRecordType(
            mqbc::ClusterStateRecordType::e_UPDATE);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
        lsn = &choice.queueUpdateAdvisory().sequenceNumber();
        d_currRecordHeader.setRecordType(
            mqbc::ClusterStateRecordType::e_UPDATE);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_LEADER_ADVISORY_COMMIT: {
        lsn = &choice.leaderAdvisoryCommit().sequenceNumber();
        d_currRecordHeader.setRecordType(
            mqbc::ClusterStateRecordType::e_COMMIT);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_UNDEFINED:
    default: {
        BSLS_ASSERT_SAFE(
            false &&
            "Unsupported cluster message type for cluster state ledger");
        return -1;  // RETURN
    }
    }

    d_currRecordHeader.setElectorTerm(lsn->electorTerm())
        .setSequenceNumber(lsn->sequenceNumber());

    return 0;
}

// ACCESSORS
bool ClusterStateLedgerIterator::isValid() const
{
    return d_iter != d_iterEnd;
}

const mqbc::ClusterStateRecordHeader&
ClusterStateLedgerIterator::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_currRecordHeader;
}

int ClusterStateLedgerIterator::loadClusterMessage(
    bmqp_ctrlmsg::ClusterMessage* message) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(message);
    BSLS_ASSERT_SAFE(isValid());

    *message = *d_iter;

    return 0;
}

bsl::ostream& ClusterStateLedgerIterator::print(bsl::ostream& stream,
                                                int           level,
                                                int spacesPerLevel) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("headerWords", d_currRecordHeader.headerWords());
    printer.printAttribute("recordType", d_currRecordHeader.recordType());
    printer.printAttribute("leaderAdvisoryWords",
                           d_currRecordHeader.leaderAdvisoryWords());
    printer.printAttribute("electorTerm", d_currRecordHeader.electorTerm());
    printer.printAttribute("sequenceNumber",
                           d_currRecordHeader.sequenceNumber());
    printer.printAttribute("timestamp", d_currRecordHeader.timestamp());
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
