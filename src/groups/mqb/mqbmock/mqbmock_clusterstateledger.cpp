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

// mqbmock_clusterstateledger.cpp                                     -*-C++-*-
#include <mqbmock_clusterstateledger.h>

#include <mqbscm_version.h>
// MQB
#include <mqbmock_clusterstateledgeriterator.h>

// BDE
#include <bsla_annotations.h>
#include <bslmf_allocatorargt.h>

namespace BloombergLP {
namespace mqbmock {

// ------------------------
// class ClusterStateLedger
// ------------------------

// PRIVATE MANIPULATORS
int ClusterStateLedger::applyAdvisoryInternal(
    const bmqp_ctrlmsg::ClusterMessage& clusterMessage)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    d_uncommittedAdvisories.emplace_back(clusterMessage);

    return 0;
}

// CREATORS
ClusterStateLedger::ClusterStateLedger(mqbc::ClusterData* clusterData,
                                       bslma::Allocator*  allocator)
: d_allocator_p(allocator)
, d_isOpen(false)
, d_pauseCommitCb(false)
, d_commitCb()
, d_clusterData_p(clusterData)
, d_records(d_allocator_p)
, d_uncommittedAdvisories(d_allocator_p)
{
    // NOTHING
}

ClusterStateLedger::~ClusterStateLedger()
{
    // NOTHING
}

// MANIPULATORS
//   (virtual mqbc::ClusterStateLedger)
int ClusterStateLedger::open()
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    d_isOpen = true;

    return 0;
}

int ClusterStateLedger::close()
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    d_isOpen = false;

    return 0;
}

int ClusterStateLedger::apply(
    const bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(isSelfLeader());

    bmqp_ctrlmsg::ClusterMessage clusterMessage;
    clusterMessage.choice().makePartitionPrimaryAdvisory(advisory);

    return applyAdvisoryInternal(clusterMessage);
}

int ClusterStateLedger::apply(
    const bmqp_ctrlmsg::QueueAssignmentAdvisory& advisory)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(isSelfLeader());

    bmqp_ctrlmsg::ClusterMessage clusterMessage;
    clusterMessage.choice().makeQueueAssignmentAdvisory(advisory);

    return applyAdvisoryInternal(clusterMessage);
}

int ClusterStateLedger::apply(
    const bmqp_ctrlmsg::QueueUnassignedAdvisory& advisory)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(isSelfLeader());

    bmqp_ctrlmsg::ClusterMessage clusterMessage;
    clusterMessage.choice().makeQueueUnassignedAdvisory(advisory);

    return applyAdvisoryInternal(clusterMessage);
}

int ClusterStateLedger::apply(
    const bmqp_ctrlmsg::QueueUpdateAdvisory& advisory)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(isSelfLeader());

    bmqp_ctrlmsg::ClusterMessage clusterMessage;
    clusterMessage.choice().makeQueueUpdateAdvisory(advisory);

    return applyAdvisoryInternal(clusterMessage);
}

int ClusterStateLedger::apply(const bmqp_ctrlmsg::LeaderAdvisory& advisory)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));
    // NOTE: We remove the assert below to allow artificially setting the
    //       ledger snapshot before the leader is elected.
    //
    // BSLS_ASSERT_SAFE(isSelfLeader());

    bmqp_ctrlmsg::ClusterMessage clusterMessage;
    clusterMessage.choice().makeLeaderAdvisory(advisory);

    return applyAdvisoryInternal(clusterMessage);
}

int ClusterStateLedger::apply(
    const bmqp_ctrlmsg::ClusterMessage& clusterMessage)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(isSelfLeader());

    const bmqp_ctrlmsg::ClusterMessageChoice& choice = clusterMessage.choice();
    switch (choice.selectionId()) {
    case MsgChoice::SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        return apply(choice.partitionPrimaryAdvisory());  // RETURN
    }
    case MsgChoice::SELECTION_ID_LEADER_ADVISORY: {
        return apply(choice.leaderAdvisory());  // RETURN
    }
    case MsgChoice::SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        return apply(choice.queueAssignmentAdvisory());  // RETURN
    }
    case MsgChoice::SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        return apply(choice.queueUnassignedAdvisory());  // RETURN
    }
    case MsgChoice::SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
        return apply(choice.queueUpdateAdvisory());  // RETURN
    }
    case MsgChoice::SELECTION_ID_UNDEFINED:
    default: {
        BSLS_ASSERT_SAFE(
            false &&
            "Unsupported cluster message type for cluster state ledger");
        return -1;  // RETURN
    }
    }

    BSLS_ASSERT_OPT(false && "Unreachable by design.");
    return -1;
}

int ClusterStateLedger::apply(BSLA_UNUSED const bdlbb::Blob& record,
                              BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    // NOT IMPLEMENTED
    return -1;
}

// MANIPULATORS
void ClusterStateLedger::_commitAdvisories(
    mqbc::ClusterStateLedgerCommitStatus::Enum status)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    for (AdvisoriesCIter cit = d_uncommittedAdvisories.cbegin();
         cit != d_uncommittedAdvisories.cend();
         ++cit) {
        if (status == mqbc::ClusterStateLedgerCommitStatus::e_SUCCESS) {
            // Store the record and its commit
            d_records.emplace_back(*cit);

            bmqp_ctrlmsg::ClusterMessage        commitRecord;
            bmqp_ctrlmsg::LeaderAdvisoryCommit& commit =
                commitRecord.choice().makeLeaderAdvisoryCommit();
            d_clusterData_p->electorInfo().nextLeaderMessageSequence(
                &commit.sequenceNumber());

            const bmqp_ctrlmsg::ClusterMessageChoice& choice = cit->choice();
            switch (choice.selectionId()) {
            case MsgChoice::SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
                commit.sequenceNumberCommitted() =
                    choice.partitionPrimaryAdvisory().sequenceNumber();
            } break;  // BREAK
            case MsgChoice::SELECTION_ID_LEADER_ADVISORY: {
                commit.sequenceNumberCommitted() =
                    choice.leaderAdvisory().sequenceNumber();
            } break;  // BREAK
            case MsgChoice::SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
                commit.sequenceNumberCommitted() =
                    choice.queueAssignmentAdvisory().sequenceNumber();
            } break;  // BREAK
            case MsgChoice::SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
                commit.sequenceNumberCommitted() =
                    choice.queueUnassignedAdvisory().sequenceNumber();
            } break;  // BREAK
            case MsgChoice::SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
                commit.sequenceNumberCommitted() =
                    choice.queueUpdateAdvisory().sequenceNumber();
            } break;  // BREAK
            case MsgChoice::SELECTION_ID_UNDEFINED:
            default: {
                BSLS_ASSERT_SAFE(false &&
                                 "Unsupported cluster message type for "
                                 "cluster state ledger.");
                return;
            }
            }

            d_records.emplace_back(commitRecord);
        }

        if (!d_pauseCommitCb) {
            BSLS_ASSERT_SAFE(d_commitCb);

            bmqp_ctrlmsg::ControlMessage controlMessage;
            controlMessage.choice().makeClusterMessage(*cit);
            d_commitCb(controlMessage, status);
        }
    }

    d_uncommittedAdvisories.clear();
}

// ACCESSORS
//   (virtual mqbc::ClusterStateLedger)
bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator>
ClusterStateLedger::getIterator() const
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    return bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator>(
        new (*d_allocator_p) mqbmock::ClusterStateLedgerIterator(d_records),
        d_allocator_p);
}

}  // close package namespace
}  // close enterprise namespace
