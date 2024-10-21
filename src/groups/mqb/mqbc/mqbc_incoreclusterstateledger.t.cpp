// Copyright 2023 Bloomberg Finance L.P.
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

// mqbc_incoreclusterstateledger.t.cpp                                -*-C++-*-
#include <mqbc_incoreclusterstateledger.h>

// ----------------------------------------------------------------------------
//                                   NOTICE
//
// Strong consistency mode is neither implemented nor tested for this
// component.
// ----------------------------------------------------------------------------

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqt_uri.h>

// MQB
#include <mqbc_clusterstateledgeriterator.h>
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbc_clusterstateledgerutil.h>
#include <mqbc_clusterutil.h>
#include <mqbmock_cluster.h>
#include <mqbnet_cluster.h>
#include <mqbsi_ledger.h>
#include <mqbu_storagekey.h>

#include <bmqio_testchannel.h>
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>

// BDE
#include <balber_berencoder.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdls_filesystemutil.h>
#include <bdlsb_memoutstreambuf.h>
#include <bsl_algorithm.h>
#include <bsl_cstdio.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// SYS
#include <unistd.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bmqu_tempdirectory.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
// NOTE: At this time, we are only testing for eventual consistency.
//
// - breathing test - open, accessors (state + description), close
// - [OPTIONAL] open, open (fail), close, close (fail)
// - apply (leader + follower):
//     o PartitionPrimaryAdvisory,
//       QueueAssignmentAdvisory,
//       QueueUnassignedAdvisory
//       QueueUpdateAdvisory
//       LeaderAdvisory
//     o LeaderAdvisoryAck (at leader)
//     o LeaderAdvisoryCommit (at follower)
//   *Verify*:
//     o Message was broadcasted
//     o Message was written to disk (require 'ClusterStateLedgerIterator')
// - Open logs that were already written at a particular location:
//     o apply enough advisories to trigger rollover
//     o apply some more advisories and "save" them in a map/list,
//       'lastAdvisories'
//     o close the CSL
//     o open the CSL and instantiate 'ClusterStateLedgerIterator'.
//       - Verify the snapshot, then iterate over each record at a time and
//         compare to 'lastAdvisories'.
//
//-----------------------------------------------------------------------------
// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// FUNCTIONS

/// Verify that the record header at the specified `cslIter` position has
/// the specified `recordType` and `sequenceNumber`.
void verifyRecordHeader(
    const mqbc::ClusterStateLedgerIterator&    cslIter,
    mqbc::ClusterStateRecordType::Enum         recordType,
    const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber)
{
    ASSERT_EQ(cslIter.header().headerWords(),
              mqbc::ClusterStateRecordHeader::k_HEADER_NUM_WORDS);
    ASSERT_EQ(cslIter.header().recordType(), recordType);
    ASSERT_EQ(cslIter.header().electorTerm(), sequenceNumber.electorTerm());
    ASSERT_EQ(cslIter.header().sequenceNumber(),
              sequenceNumber.sequenceNumber());
}

/// Verify that the record at the specified `cslIter` position is a leader
/// advisory commit of the specified `sequenceNumber`.
void verifyLeaderAdvisoryCommit(
    const mqbc::ClusterStateLedgerIterator&    cslIter,
    const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber)
{
    ASSERT_EQ(cslIter.header().headerWords(),
              mqbc::ClusterStateRecordHeader::k_HEADER_NUM_WORDS);
    ASSERT_EQ(cslIter.header().recordType(),
              mqbc::ClusterStateRecordType::e_COMMIT);

    bmqp_ctrlmsg::ClusterMessage msg;
    const int                    rc = cslIter.loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isLeaderAdvisoryCommitValue());
    ASSERT_EQ(msg.choice().leaderAdvisoryCommit().sequenceNumberCommitted(),
              sequenceNumber);
}

bool compareQueueInfo(const bmqp_ctrlmsg::QueueInfo& lhs,
                      const bmqp_ctrlmsg::QueueInfo& rhs)
{
    const size_t prefexLength =
        bsl::string("bmq://bmq.test.mmap.priority/q").size();

    const int lhsQueueNum = bsl::stoi(lhs.uri().substr(prefexLength));
    const int rhsQueueNum = bsl::stoi(rhs.uri().substr(prefexLength));

    return lhsQueueNum < rhsQueueNum;
}

// CLASSES

// ===================
// struct AdvisoryInfo
// ===================

struct AdvisoryInfo {
  public:
    // PUBLIC DATA
    bmqp_ctrlmsg::ControlMessage        d_advisory;
    bmqp_ctrlmsg::LeaderMessageSequence d_sequenceNumber;
    mqbc::ClusterStateRecordType::Enum  d_recordType;

  public:
    // CREATOR
    AdvisoryInfo(const bmqp_ctrlmsg::ControlMessage&        advisory,
                 const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
                 mqbc::ClusterStateRecordType::Enum         recordType)
    : d_advisory(advisory)
    , d_sequenceNumber(sequenceNumber)
    , d_recordType(recordType)
    {
        // NOTHING
    }
};

// =============
// struct Tester
// =============

/// This class provides a wrapper on top of the IncoreClusterStateLedger
/// under test and implements a few mechanisms to help testing the object.
struct Tester {
  private:
    // PRIVATE TYPES
    typedef mqbmock::Cluster::TestChannelMapCIter TestChannelMapCIter;

  public:
    // PUBLIC DATA
    bdlbb::PooledBlobBufferFactory                    d_bufferFactory;
    mqbc::ClusterStateLedgerConsistency::Enum         d_consistencyLevel;
    bmqu::TempDirectory                               d_tempDir;
    bsl::string                                       d_location;
    bslma::ManagedPtr<mqbmock::Cluster>               d_cluster_mp;
    bslma::ManagedPtr<mqbc::IncoreClusterStateLedger> d_clusterStateLedger_mp;
    bsl::deque<bmqp_ctrlmsg::ControlMessage>          d_committedMessages;
    bsls::Types::Int64                                d_commitCounter;

  public:
    // CREATORS
    Tester(bool isLeader = true, const bslstl::StringRef& location = "")
    : d_bufferFactory(1024, s_allocator_p)
    , d_consistencyLevel(mqbc::ClusterStateLedgerConsistency::e_EVENTUAL)
    , d_tempDir(s_allocator_p)
    , d_location(!location.empty() ? bsl::string(location, s_allocator_p)
                                   : d_tempDir.path())
    , d_cluster_mp(0)
    , d_clusterStateLedger_mp(0)
    , d_committedMessages(s_allocator_p)
    , d_commitCounter(0)
    {
        mqbmock::Cluster::ClusterNodeDefs clusterNodeDefs(s_allocator_p);
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "E1",
            "US-EAST",
            41234,
            mqbmock::Cluster::k_LEADER_NODE_ID,
            s_allocator_p);
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "E2",
            "US-EAST",
            41235,
            mqbmock::Cluster::k_LEADER_NODE_ID + 1,
            s_allocator_p);
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "W1",
            "US-WEST",
            41236,
            mqbmock::Cluster::k_LEADER_NODE_ID + 2,
            s_allocator_p);
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "W2",
            "US-WEST",
            41237,
            mqbmock::Cluster::k_LEADER_NODE_ID + 3,
            s_allocator_p);

        d_cluster_mp.load(new (*s_allocator_p)
                              mqbmock::Cluster(&d_bufferFactory,
                                               s_allocator_p,
                                               true,  // isClusterMember
                                               isLeader,
                                               true,   // isCSLMode
                                               false,  // isFSMWorkflow
                                               clusterNodeDefs,
                                               "testCluster",
                                               d_location),
                          s_allocator_p);

        // Set cluster state's leader node: One node is selected to serve as
        // leader (1st among the nodes of the cluster)
        mqbnet::ClusterNode* leaderNode =
            d_cluster_mp->_clusterData()
                ->membership()
                .netCluster()
                ->lookupNode(mqbmock::Cluster::k_LEADER_NODE_ID);
        BSLS_ASSERT_OPT(leaderNode != 0);
        d_cluster_mp->_clusterData()->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_LEADER,
            1,  // term
            leaderNode,
            mqbc::ElectorInfoLeaderStatus::e_ACTIVE);

        // Set partition primaries in the cluster state
        int                         pid = 0;
        mqbnet::Cluster::NodesList& nodes =
            d_cluster_mp->_clusterData()->membership().netCluster()->nodes();
        for (mqbnet::Cluster::NodesList::iterator iter = nodes.begin();
             iter != nodes.end();
             ++iter) {
            d_cluster_mp->_state().setPartitionPrimary(pid, 1, *iter);
            ++pid;
        }

        d_clusterStateLedger_mp.load(
            new (*s_allocator_p) mqbc::IncoreClusterStateLedger(
                d_cluster_mp->_clusterDefinition(),
                d_consistencyLevel,
                d_cluster_mp->_clusterData(),
                &d_cluster_mp->_state(),
                d_cluster_mp->_bufferFactory(),
                s_allocator_p),
            s_allocator_p);
        d_clusterStateLedger_mp->setCommitCb(
            bdlf::BindUtil::bind(&Tester::onCommitCb,
                                 this,
                                 bdlf::PlaceHolders::_1,    // advisory
                                 bdlf::PlaceHolders::_2));  // status

        bmqp_ctrlmsg::LeaderMessageSequence leaderSeqNum;
        leaderSeqNum.electorTerm()    = 1;
        leaderSeqNum.sequenceNumber() = 1;
        d_cluster_mp->_clusterData()->electorInfo().setLeaderMessageSequence(
            leaderSeqNum);
    }

    // MANIPULATORS
    void onCommitCb(const bmqp_ctrlmsg::ControlMessage&        advisory,
                    mqbc::ClusterStateLedgerCommitStatus::Enum status)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(advisory.choice().isClusterMessageValue());

        PVV("# [" << d_commitCounter << ": " << status << "]: " << advisory);
        d_committedMessages.push_back(advisory);
        ++d_commitCounter;

        mqbc::ClusterUtil::apply(&d_cluster_mp->_state(),
                                 advisory.choice().clusterMessage(),
                                 *d_cluster_mp->_clusterData());
    }

    /// Load into the specified `event` a bmqp::Event of type
    /// `e_CLUSTER_STATE` containing a ledger record of the specified
    /// `clusterMessage` having the specified `sequenceNumber`, `timestamp`
    /// and `recordType`.
    void constructEventBlob(
        bdlbb::Blob*                               event,
        const bmqp_ctrlmsg::ClusterMessage&        clusterMessage,
        const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
        bsls::Types::Uint64                        timestamp,
        mqbc::ClusterStateRecordType::Enum         recordType)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(event);

        // Create ledger record
        bdlbb::Blob record(d_cluster_mp->_bufferFactory(), s_allocator_p);
        BSLS_ASSERT_OPT(
            mqbc::ClusterStateLedgerUtil::appendRecord(&record,
                                                       clusterMessage,
                                                       sequenceNumber,
                                                       timestamp,
                                                       recordType) == 0);

        // Construct event blob
        bmqp::EventHeader eventHeader(bmqp::EventType::e_CLUSTER_STATE);
        eventHeader.setLength(sizeof(bmqp::EventHeader) + record.length());

        bdlbb::BlobUtil::append(event,
                                reinterpret_cast<char*>(&eventHeader),
                                sizeof(bmqp::EventHeader));
        bdlbb::BlobUtil::append(event, record);
    }

    // ACCESSORS
    bool hasNoMoreBroadcastedMessages() const
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(d_cluster_mp->_channels().size() > 0);

        for (TestChannelMapCIter citer = d_cluster_mp->_channels().cbegin();
             citer != d_cluster_mp->_channels().cend();
             ++citer) {
            if (!citer->second->hasNoMoreWriteCalls()) {
                return false;
            }
        }

        return true;
    }

    bool hasBroadcastedMessages(int number, bool isFinal = true) const
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(d_cluster_mp->_channels().size() > 0);

        size_t numMessages = 0;
        bool   first       = true;
        for (TestChannelMapCIter citer = d_cluster_mp->_channels().cbegin();
             citer != d_cluster_mp->_channels().cend();
             ++citer) {
            if (citer->first->nodeId() == d_cluster_mp->_clusterData()
                                              ->membership()
                                              .netCluster()
                                              ->selfNodeId()) {
                continue;  // CONTINUE
            }

            if (!citer->second->waitFor(number, isFinal)) {
                return false;  // RETURN
            }

            if (first) {
                numMessages = citer->second->writeCalls().size();
                first       = false;
            }
            else {
                BSLS_ASSERT_OPT(numMessages ==
                                citer->second->writeCalls().size());
            }
        }

        return true;
    }

    size_t numCommittedMessages() const { return d_committedMessages.size(); }

    bmqp_ctrlmsg::ControlMessage broadcastedMessage(int index)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(d_cluster_mp->_channels().size() > 0);

        bdlbb::Blob* blob = 0;
        for (TestChannelMapCIter citer = d_cluster_mp->_channels().cbegin();
             citer != d_cluster_mp->_channels().cend();
             ++citer) {
            if (citer->first->nodeId() == d_cluster_mp->_clusterData()
                                              ->membership()
                                              .netCluster()
                                              ->selfNodeId()) {
                continue;  // CONTINUE
            }

            BSLS_ASSERT_OPT(citer->second->waitFor(index + 1, false));

            if (!blob) {
                blob = &citer->second->writeCalls()[index].d_blob;
            }
            else {
                BSLS_ASSERT_OPT(
                    bdlbb::BlobUtil::compare(
                        *blob,
                        citer->second->writeCalls()[index].d_blob) == 0);
            }
        }

        bdlbb::Blob record(d_cluster_mp->_bufferFactory(), s_allocator_p);
        bdlbb::BlobUtil::append(&record, *blob, sizeof(bmqp::EventHeader));

        bmqp_ctrlmsg::ControlMessage  controlMessage;
        bmqp_ctrlmsg::ClusterMessage& clusterMessage =
            controlMessage.choice().makeClusterMessage();
        int rc = mqbc::ClusterStateLedgerUtil::loadClusterMessage(
            &clusterMessage,
            record);
        BSLS_ASSERT_OPT(rc == 0);

        return controlMessage;
    }

    const bmqp_ctrlmsg::ControlMessage& committedMessage(int index) const
    {
        return d_committedMessages.at(index);
    }

    ~Tester()
    {
        bsl::string pattern(s_allocator_p);
        pattern.append(
            d_cluster_mp->_clusterDefinition().partitionConfig().location());
        pattern.append("bmq_cs_*.bmq");

        bsl::vector<bsl::string> files(s_allocator_p);
        bdls::FilesystemUtil::findMatchingPaths(&files, pattern.c_str());
        for (size_t i = 0; i < files.size(); ++i) {
            bsl::remove(files[i].c_str());
        }
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("INCORE CLUSTER STATE LEDGER"
                                      " - BREATHING TEST");

    Tester                          tester;
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();

    ASSERT_EQ(obj->open(), 0);
    ASSERT_EQ(obj->description(),
              "IncoreClusterStateLedger (cluster: testCluster) : ");

    ASSERT_EQ(tester.numCommittedMessages(), 0U);
    ASSERT(tester.hasBroadcastedMessages(0));
    ASSERT_EQ(obj->close(), 0);
    ASSERT(tester.hasNoMoreBroadcastedMessages());
}

static void test2_apply_PartitionPrimaryAdvisory()
// ------------------------------------------------------------------------
// PARTITION PRIMARY INFO
//
// Concerns:
//   Applying 'PartitionPrimaryAdvisory' (only at leader).
//
// Testing:
//   int apply(const bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPLY - PARTITION PRIMARY ADVISORY");

    Tester                          tester;
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    // Build 'PartitionPrimaryAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
    pinfo.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo.partitionId()    = 1U;
    pinfo.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::PartitionPrimaryAdvisory advisory;
    advisory.partitions().push_back(pinfo);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&advisory.sequenceNumber());
    ASSERT_EQ(obj->apply(advisory), 0);

    // Verify
    bmqp_ctrlmsg::ControlMessage expected;
    expected.choice()
        .makeClusterMessage()
        .choice()
        .makePartitionPrimaryAdvisory(advisory);

    ASSERT_EQ(tester.numCommittedMessages(), 1U);
    ASSERT_EQ(tester.committedMessage(0), expected);
    ASSERT(tester.hasBroadcastedMessages(2));
    ASSERT_EQ(tester.broadcastedMessage(0), expected);

    BSLS_ASSERT_OPT(obj->close() == 0);
    ASSERT(tester.hasNoMoreBroadcastedMessages());
}

static void test3_apply_QueueAssignmentAdvisory()
// ------------------------------------------------------------------------
// QUEUE ASSIGNMENT ADVISORY
//
// Concerns:
//   Applying 'QueueAssignmentAdvisory' (only at leader).
//
// Testing:
//   int apply(const bmqp_ctrlmsg::QueueAssignmentAdvisory& advisory);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPLY - QUEUE ASSIGNMENT ADVISORY");

    Tester                          tester;
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    // Build 'QueueAssignmentAdvisory'
    bmqp_ctrlmsg::QueueAssignmentAdvisory qadvisory;
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qadvisory.sequenceNumber());

    bmqp_ctrlmsg::QueueInfo qinfo;
    qinfo.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qinfo.partitionId() = 1U;

    mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(), "7777");
    key.loadBinary(&qinfo.key());

    qadvisory.queues().push_back(qinfo);

    ASSERT_EQ(obj->apply(qadvisory), 0);

    // Verify
    bmqp_ctrlmsg::ControlMessage expected;
    expected.choice()
        .makeClusterMessage()
        .choice()
        .makeQueueAssignmentAdvisory(qadvisory);

    ASSERT_EQ(tester.committedMessage(0), expected);
    ASSERT_EQ(tester.numCommittedMessages(), 1U);
    ASSERT_EQ(tester.broadcastedMessage(0), expected);
    ASSERT(tester.hasBroadcastedMessages(2));

    BSLS_ASSERT_OPT(obj->close() == 0);
}

static void test4_apply_QueueUnassignedAdvisory()
// ------------------------------------------------------------------------
// QUEUE UNASSIGNED ADVISORY
//
// Concerns:
//   Applying 'QueueUnassignedAdvisory' (only at leader).
//
// Testing:
//   int apply(const bmqp_ctrlmsg::QueueUnassignedAdvisory& advisory);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPLY - QUEUE UNASSIGNED ADVISORY");

    Tester                          tester;
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    // Build 'QueueUnassignedAdvisory'
    bmqp_ctrlmsg::QueueUnassignedAdvisory qadvisory;
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qadvisory.sequenceNumber());

    bmqp_ctrlmsg::QueueInfo qinfo;
    qinfo.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qinfo.partitionId() = 1U;

    qadvisory.queues().push_back(qinfo);

    ASSERT_EQ(obj->apply(qadvisory), 0);

    // Verify
    bmqp_ctrlmsg::ControlMessage expected;
    expected.choice()
        .makeClusterMessage()
        .choice()
        .makeQueueUnassignedAdvisory(qadvisory);
    ASSERT_EQ(tester.numCommittedMessages(), 1U);
    ASSERT_EQ(tester.committedMessage(0), expected);
    ASSERT(tester.hasBroadcastedMessages(2));
    ASSERT_EQ(tester.broadcastedMessage(0), expected);

    BSLS_ASSERT_OPT(obj->close() == 0);
}

static void test5_apply_QueueUpdateAdvisory()
// ------------------------------------------------------------------------
// QUEUE UPDATE ADVISORY
//
// Concerns:
//   Applying 'QueueUpdateAdvisory' (only at leader).
//
// Testing:
//   int apply(const bmqp_ctrlmsg::QueueUpdateAdvisory& advisory);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPLY - QUEUE UPDATE ADVISORY");

    Tester                          tester;
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    // Build 'QueueUpdateAdvisory'
    bmqp_ctrlmsg::QueueUpdateAdvisory qadvisory;
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qadvisory.sequenceNumber());

    bmqp_ctrlmsg::QueueInfoUpdate qupdate;
    qupdate.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qupdate.partitionId() = 1U;

    mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(), "7777");
    key.loadBinary(&qupdate.key());

    qupdate.addedAppIds().resize(1);
    qupdate.removedAppIds().resize(1);
    qupdate.domain() = "bmq.test.mmap.priority";

    bmqp_ctrlmsg::AppIdInfo& addedAppId = qupdate.addedAppIds().back();
    addedAppId.appId()                  = "App1";
    mqbu::StorageKey appKey1(mqbu::StorageKey::BinaryRepresentation(),
                             "12345");
    appKey1.loadBinary(&addedAppId.appKey());

    bmqp_ctrlmsg::AppIdInfo& removedAppId = qupdate.removedAppIds().back();
    removedAppId.appId()                  = "App2";
    mqbu::StorageKey appKey2(mqbu::StorageKey::BinaryRepresentation(),
                             "23456");
    appKey2.loadBinary(&removedAppId.appKey());

    qadvisory.queueUpdates().push_back(qupdate);

    ASSERT_EQ(obj->apply(qadvisory), 0);

    // Verify
    bmqp_ctrlmsg::ControlMessage expected;
    expected.choice().makeClusterMessage().choice().makeQueueUpdateAdvisory(
        qadvisory);
    ASSERT_EQ(tester.numCommittedMessages(), 1U);
    ASSERT_EQ(tester.committedMessage(0), expected);
    ASSERT(tester.hasBroadcastedMessages(2));
    ASSERT_EQ(tester.broadcastedMessage(0), expected);

    BSLS_ASSERT_OPT(obj->close() == 0);
}

static void test6_apply_LeaderAdvisory()
// ------------------------------------------------------------------------
// LEADER ADVISORY
//
// Concerns:
//   Applying 'LeaderAdvisory' (only at leader).
//
// Testing:
//   int apply(const bmqp_ctrlmsg::LeaderAdvisory& advisory);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPLY - LEADER ADVISORY");

    Tester                          tester;
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    // Build 'LeaderAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
    pinfo.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo.partitionId()    = 1U;
    pinfo.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::QueueInfo qinfo;
    qinfo.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qinfo.partitionId() = 1U;

    mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(), "7777");
    key.loadBinary(&qinfo.key());

    bmqp_ctrlmsg::LeaderAdvisory leaderAdvisory;
    leaderAdvisory.queues().push_back(qinfo);
    leaderAdvisory.partitions().push_back(pinfo);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&leaderAdvisory.sequenceNumber());

    ASSERT_EQ(obj->apply(leaderAdvisory), 0);

    // Verify
    bmqp_ctrlmsg::ControlMessage expected;
    expected.choice().makeClusterMessage().choice().makeLeaderAdvisory(
        leaderAdvisory);
    ASSERT_EQ(tester.numCommittedMessages(), 1U);
    ASSERT_EQ(tester.committedMessage(0), expected);
    ASSERT(tester.hasBroadcastedMessages(2));
    ASSERT_EQ(tester.broadcastedMessage(0), expected);

    BSLS_ASSERT_OPT(obj->close() == 0);
    ASSERT(tester.hasNoMoreBroadcastedMessages());
}

static void test7_apply_ClusterStateRecord()
// ------------------------------------------------------------------------
// CLUSTER STATE RECORD
//
// Concerns:
//   Applying cluster state record of type 'e_SNAPSHOT' or 'e_UPDATE' (we
//   test only at follower).
//
// Testing:
//   int apply(const bdlbb::Blob& record)  // for 'record' of type
//                                         // 'e_SNAPSHOT' or 'e_UPDATE'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPLY - CLUSTER STATE RECORD");

    Tester                          tester(false);  // isLeader
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    // 1. Create an update record
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
    pinfo.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo.partitionId()    = 1U;
    pinfo.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::PartitionPrimaryAdvisory pmAdvisory;
    pmAdvisory.partitions().push_back(pinfo);

    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&pmAdvisory.sequenceNumber());

    bmqp_ctrlmsg::ClusterMessage updateMessage;
    updateMessage.choice().makePartitionPrimaryAdvisory(pmAdvisory);

    bdlbb::Blob updateEvent(tester.d_cluster_mp->_bufferFactory(),
                            s_allocator_p);
    tester.constructEventBlob(&updateEvent,
                              updateMessage,
                              pmAdvisory.sequenceNumber(),
                              123456,
                              mqbc::ClusterStateRecordType::e_UPDATE);

    // Apply the update record
    ASSERT_EQ(obj->apply(updateEvent,
                         tester.d_cluster_mp->netCluster().lookupNode(
                             mqbmock::Cluster::k_LEADER_NODE_ID)),
              0);
    ASSERT_EQ(tester.numCommittedMessages(), 0U);
    ASSERT(tester.hasBroadcastedMessages(0));

    // Verify that the underlying ledger contains the update record
    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator> cslIter =
        obj->getIterator();
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       pmAdvisory.sequenceNumber());
    ASSERT_EQ(cslIter->header().timestamp(), 123456U);

    bmqp_ctrlmsg::ClusterMessage msg;
    int                          rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isPartitionPrimaryAdvisoryValue());
    ASSERT_EQ(msg.choice().partitionPrimaryAdvisory(), pmAdvisory);

    // 2. Create a snapshot record
    bmqp_ctrlmsg::QueueInfo qinfo;
    qinfo.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qinfo.partitionId() = 1U;

    bmqp_ctrlmsg::LeaderAdvisory leaderAdvisory;
    leaderAdvisory.queues().push_back(qinfo);
    leaderAdvisory.partitions().push_back(pinfo);

    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&leaderAdvisory.sequenceNumber());

    bmqp_ctrlmsg::ClusterMessage snapshotMessage;
    snapshotMessage.choice().makeLeaderAdvisory(leaderAdvisory);

    bdlbb::Blob snapshotEvent(tester.d_cluster_mp->_bufferFactory(),
                              s_allocator_p);
    tester.constructEventBlob(&snapshotEvent,
                              snapshotMessage,
                              leaderAdvisory.sequenceNumber(),
                              123567,
                              mqbc::ClusterStateRecordType::e_SNAPSHOT);

    // Apply the snapshot record
    ASSERT_EQ(obj->apply(snapshotEvent,
                         tester.d_cluster_mp->netCluster().lookupNode(
                             mqbmock::Cluster::k_LEADER_NODE_ID)),
              0);
    ASSERT_EQ(tester.numCommittedMessages(), 0U);
    ASSERT(tester.hasBroadcastedMessages(0));

    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_SNAPSHOT,
                       leaderAdvisory.sequenceNumber());
    ASSERT_EQ(cslIter->header().timestamp(), 123567U);

    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isLeaderAdvisoryValue());
    ASSERT_EQ(msg.choice().leaderAdvisory(), leaderAdvisory);
}

static void test8_apply_ClusterStateRecordAck()
// ------------------------------------------------------------------------
// CLUSTER STATE RECORD ACK
//
// Concerns:
//   Applying 'LeaderAdvisoryAck' (only at leader).
//     - For an invalid advisory (one that has already been committed)
//   NOTE: At this time only weak consistency is implemented, so all
//         valid advisories are immediately committed and this test driver
//         can only test for an "invalid" advisory.
//         Once strong consistency is supported, testing an ACK for valid
//         advisories will be possible.
//
// Testing:
//   int apply(const bdlbb::Blob& record)  // for 'record' of type 'e_ACK'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPLY - CLUSTER STATE RECORD ACK");

    Tester                          tester;
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
    pinfo.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo.partitionId()    = 1U;
    pinfo.primaryLeaseId() = 2U;

    // Build 'PartitionPrimaryAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryAdvisory advisory;
    advisory.partitions().push_back(pinfo);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&advisory.sequenceNumber());

    ASSERT_EQ(obj->apply(advisory), 0);

    bmqp_ctrlmsg::ControlMessage expected;
    expected.choice()
        .makeClusterMessage()
        .choice()
        .makePartitionPrimaryAdvisory(advisory);
    BSLS_ASSERT_OPT(tester.numCommittedMessages() == 1U);
    BSLS_ASSERT_OPT(tester.committedMessage(0) == expected);
    BSLS_ASSERT_OPT(tester.hasBroadcastedMessages(2));
    BSLS_ASSERT_OPT(tester.broadcastedMessage(0) == expected);

    // Build 'LeaderAdvisoryAck'
    bmqp_ctrlmsg::LeaderAdvisoryAck ack;
    ack.sequenceNumberAcked() = advisory.sequenceNumber();

    bmqp_ctrlmsg::ClusterMessage message;
    message.choice().makeLeaderAdvisoryAck(ack);

    bdlbb::Blob ackEvent(tester.d_cluster_mp->_bufferFactory(), s_allocator_p);
    tester.constructEventBlob(&ackEvent,
                              message,
                              ack.sequenceNumberAcked(),
                              123456,
                              mqbc::ClusterStateRecordType::e_ACK);

    ASSERT_NE(obj->apply(ackEvent,
                         tester.d_cluster_mp->netCluster().lookupNode(
                             mqbmock::Cluster::k_LEADER_NODE_ID + 1)),
              0);
    BSLS_ASSERT_OPT(obj->close() == 0);
    ASSERT(tester.hasNoMoreBroadcastedMessages());
}

static void test9_apply_ClusterStateRecordCommit()
// ------------------------------------------------------------------------
// CLUSTER STATE RECORD COMMIT
//
// Concerns:
//   Applying 'LeaderAdvisoryCommit' (we test only at follower).
//     - For a valid advisory (one that has been previously applied)
//     - For an invalid advisory
//
// Testing:
//   int apply(const bdlbb::Blob& record)  // for 'record' of type
//                                         // 'e_COMMIT'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPLY - CLUSTER STATE RECORD COMMIT");

    Tester                          tester(false);  // isLeader
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    // Build 'PartitionPrimaryAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
    pinfo.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo.partitionId()    = 1U;
    pinfo.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::PartitionPrimaryAdvisory advisory;
    advisory.partitions().push_back(pinfo);
    advisory.sequenceNumber().electorTerm()    = 1U;
    advisory.sequenceNumber().sequenceNumber() = 2U;

    bmqp_ctrlmsg::ClusterMessage advisoryMessage;
    advisoryMessage.choice().makePartitionPrimaryAdvisory(advisory);

    bdlbb::Blob advisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                              s_allocator_p);
    tester.constructEventBlob(&advisoryEvent,
                              advisoryMessage,
                              advisory.sequenceNumber(),
                              123456,
                              mqbc::ClusterStateRecordType::e_UPDATE);

    // Apply advisory
    ASSERT_EQ(obj->apply(advisoryEvent,
                         tester.d_cluster_mp->netCluster().lookupNode(
                             mqbmock::Cluster::k_LEADER_NODE_ID)),
              0);
    ASSERT_EQ(tester.numCommittedMessages(), 0U);
    ASSERT(tester.hasBroadcastedMessages(0));

    // 1. Build and apply 'LeaderAdvisoryCommit' for an advisory that has been
    //    previously applied (but not yet committed)
    bmqp_ctrlmsg::LeaderAdvisoryCommit commit;
    commit.sequenceNumberCommitted()         = advisory.sequenceNumber();
    commit.sequenceNumber().electorTerm()    = 1U;
    commit.sequenceNumber().sequenceNumber() = 3U;

    bmqp_ctrlmsg::ClusterMessage commitMessage;
    commitMessage.choice().makeLeaderAdvisoryCommit(commit);

    bdlbb::Blob commitEvent(tester.d_cluster_mp->_bufferFactory(),
                            s_allocator_p);
    tester.constructEventBlob(&commitEvent,
                              commitMessage,
                              commit.sequenceNumber(),
                              123567,
                              mqbc::ClusterStateRecordType::e_COMMIT);

    // Verify commit
    ASSERT_EQ(obj->apply(commitEvent,
                         tester.d_cluster_mp->netCluster().lookupNode(
                             mqbmock::Cluster::k_LEADER_NODE_ID)),
              0);
    ASSERT_EQ(tester.numCommittedMessages(), 1U);

    bmqp_ctrlmsg::ControlMessage expected;
    expected.choice()
        .makeClusterMessage()
        .choice()
        .makePartitionPrimaryAdvisory(advisory);
    ASSERT_EQ(tester.committedMessage(0), expected);

    // 2. Apply 'LeaderAdvisoryCommit' for an advisory that has already been
    //    previously committed
    ASSERT_NE(obj->apply(commitEvent,
                         tester.d_cluster_mp->netCluster().lookupNode(
                             mqbmock::Cluster::k_LEADER_NODE_ID)),
              0);
    ASSERT_EQ(tester.numCommittedMessages(), 1U);

    BSLS_ASSERT_OPT(obj->close() == 0);
    ASSERT(tester.hasNoMoreBroadcastedMessages());
}

static void test10_persistanceLeader()
// ------------------------------------------------------------------------
// PERSISTENCE LEADER
//
// Concerns:
//   IncoreCSL provides persistence of the logs at the leader node.
//
// Plan:
//   1 Apply advisories of different types
//   2 Close the CSL
//   3 Open the CSL and instantiate ClusterStateLedgerIterator
//   4 Iterate through the records and verify that they are as expected
//
//  Testing:
//    Persistence of the logs at the leader node.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PERSISTENCE LEADER");

    Tester                          tester;
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    // 1. Apply advisories of different types

    // Apply 'PartitionPrimaryAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
    pinfo.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo.partitionId()    = 1U;
    pinfo.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::PartitionPrimaryAdvisory pmAdvisory;
    pmAdvisory.partitions().push_back(pinfo);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&pmAdvisory.sequenceNumber());
    BSLS_ASSERT_OPT(obj->apply(pmAdvisory) == 0);
    BSLS_ASSERT_OPT(tester.numCommittedMessages() == 1U);

    // Apply 'QueueAssignmentAdvisory'
    bmqp_ctrlmsg::QueueAssignmentAdvisory qAssignAdvisory;
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qAssignAdvisory.sequenceNumber());

    bmqp_ctrlmsg::QueueInfo qinfo;
    qinfo.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qinfo.partitionId() = 1U;

    mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(), "7777");
    key.loadBinary(&qinfo.key());

    qAssignAdvisory.queues().push_back(qinfo);

    BSLS_ASSERT_OPT(obj->apply(qAssignAdvisory) == 0);
    BSLS_ASSERT_OPT(tester.numCommittedMessages() == 2U);

    // Apply 'QueueUnassignedAdvisory'
    bmqp_ctrlmsg::QueueUnassignedAdvisory qUnassignedAdvisory;
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qUnassignedAdvisory.sequenceNumber());

    qUnassignedAdvisory.queues().push_back(qinfo);

    BSLS_ASSERT_OPT(obj->apply(qUnassignedAdvisory) == 0);
    BSLS_ASSERT_OPT(tester.numCommittedMessages() == 3U);

    // Apply 'QueueUpdateAdvisory'
    bmqp_ctrlmsg::QueueUpdateAdvisory qUpdateAdvisory;
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qUpdateAdvisory.sequenceNumber());

    bmqp_ctrlmsg::QueueInfoUpdate qupdate;
    qupdate.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qupdate.partitionId() = 1U;

    mqbu::StorageKey key2(mqbu::StorageKey::BinaryRepresentation(), "8888");
    key2.loadBinary(&qupdate.key());

    qupdate.addedAppIds().resize(1);
    qupdate.removedAppIds().resize(1);
    qupdate.domain() = "bmq.test.mmap.priority";

    bmqp_ctrlmsg::AppIdInfo& addedAppId = qupdate.addedAppIds().back();
    addedAppId.appId()                  = "App1";
    mqbu::StorageKey appKey1(mqbu::StorageKey::BinaryRepresentation(),
                             "12345");
    appKey1.loadBinary(&addedAppId.appKey());

    bmqp_ctrlmsg::AppIdInfo& removedAppId = qupdate.removedAppIds().back();
    removedAppId.appId()                  = "App2";
    mqbu::StorageKey appKey2(mqbu::StorageKey::BinaryRepresentation(),
                             "23456");
    appKey2.loadBinary(&removedAppId.appKey());

    qUpdateAdvisory.queueUpdates().push_back(qupdate);

    BSLS_ASSERT_OPT(obj->apply(qUpdateAdvisory) == 0);
    BSLS_ASSERT_OPT(tester.numCommittedMessages() == 4U);

    // Apply 'LeaderAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo2;
    pinfo2.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo2.partitionId()    = 2U;
    pinfo2.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::QueueInfo qinfo2;
    qinfo2.uri()         = "bmq://bmq.test.mmap.priority/q2";
    qinfo2.partitionId() = 2U;

    mqbu::StorageKey key3(mqbu::StorageKey::BinaryRepresentation(), "9999");
    key3.loadBinary(&qinfo2.key());

    bmqp_ctrlmsg::LeaderAdvisory leaderAdvisory;
    leaderAdvisory.queues().push_back(qinfo2);
    leaderAdvisory.partitions().push_back(pinfo2);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&leaderAdvisory.sequenceNumber());

    BSLS_ASSERT_OPT(obj->apply(leaderAdvisory) == 0);
    BSLS_ASSERT_OPT(tester.numCommittedMessages() == 5U);

    // 2. Close the CSL
    BSLS_ASSERT_OPT(obj->close() == 0);

    // 3. Open the CSL and instantiate ClusterStateLedgerIterator
    BSLS_ASSERT_OPT(obj->open() == 0);

    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator> cslIter =
        obj->getIterator();

    // 4. Iterate through the records and verify that they are as expected

    // Verify 'PartitionPrimaryAdvisory' and its commit
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       pmAdvisory.sequenceNumber());

    bmqp_ctrlmsg::ClusterMessage msg;
    int                          rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(cslIter->loadClusterMessage(&msg), 0);
    ASSERT(msg.choice().isPartitionPrimaryAdvisoryValue());
    ASSERT_EQ(msg.choice().partitionPrimaryAdvisory(), pmAdvisory);

    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());

    verifyLeaderAdvisoryCommit(*cslIter, pmAdvisory.sequenceNumber());

    // Verify 'QueueAssignmentAdvisory' and its commit
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       qAssignAdvisory.sequenceNumber());

    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isQueueAssignmentAdvisoryValue());
    ASSERT_EQ(msg.choice().queueAssignmentAdvisory(), qAssignAdvisory);

    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());

    verifyLeaderAdvisoryCommit(*cslIter, qAssignAdvisory.sequenceNumber());

    // Verify 'QueueUnassignedAdvisory' and its commit
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       qUnassignedAdvisory.sequenceNumber());

    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isQueueUnassignedAdvisoryValue());
    ASSERT_EQ(msg.choice().queueUnassignedAdvisory(), qUnassignedAdvisory);

    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());

    verifyLeaderAdvisoryCommit(*cslIter, qUnassignedAdvisory.sequenceNumber());

    // Verify 'QueueUpdateAdvisory' and its commit
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       qUpdateAdvisory.sequenceNumber());

    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isQueueUpdateAdvisoryValue());
    ASSERT_EQ(msg.choice().queueUpdateAdvisory(), qUpdateAdvisory);

    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());

    verifyLeaderAdvisoryCommit(*cslIter, qUpdateAdvisory.sequenceNumber());

    // Verify 'LeaderAdvisory' and its commit
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_SNAPSHOT,
                       leaderAdvisory.sequenceNumber());

    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isLeaderAdvisoryValue());
    ASSERT_EQ(msg.choice().leaderAdvisory(), leaderAdvisory);

    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());

    verifyLeaderAdvisoryCommit(*cslIter, leaderAdvisory.sequenceNumber());

    // Verify end of ledger
    ASSERT_EQ(cslIter->next(), 1);
    ASSERT(!cslIter->isValid());
}

static void test11_persistanceFollower()
// ------------------------------------------------------------------------
// PERSISTENCE FOLLOWER
//
// Concerns:
//   IncoreCSL provides persistence of the logs at the follower node.
//
// Plan:
//   1 Apply advisories of different types
//   2 Close the CSL
//   3 Open the CSL and instantiate ClusterStateLedgerIterator
//   4 Iterate through the records and verify that they are as expected
//
//  Testing:
//    Persistence of the logs at the follower node.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PERSISTENCE FOLLOWER");

    Tester                          tester(false);  // isLeader
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    // 1. Apply advisories of different types

    // Apply 'PartitionPrimaryAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
    pinfo.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo.partitionId()    = 1U;
    pinfo.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::ClusterMessage            pmAdvisoryMsg;
    bmqp_ctrlmsg::PartitionPrimaryAdvisory& pmAdvisory =
        pmAdvisoryMsg.choice().makePartitionPrimaryAdvisory();
    pmAdvisory.partitions().push_back(pinfo);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&pmAdvisory.sequenceNumber());

    bdlbb::Blob pmAdvisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                                s_allocator_p);
    tester.constructEventBlob(&pmAdvisoryEvent,
                              pmAdvisoryMsg,
                              pmAdvisory.sequenceNumber(),
                              123001,
                              mqbc::ClusterStateRecordType::e_UPDATE);

    BSLS_ASSERT_OPT(obj->apply(pmAdvisoryEvent,
                               tester.d_cluster_mp->netCluster().lookupNode(
                                   mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

    // Apply 'QueueAssignmentAdvisory'
    bmqp_ctrlmsg::ClusterMessage           qAssignAdvisoryMsg;
    bmqp_ctrlmsg::QueueAssignmentAdvisory& qAssignAdvisory =
        qAssignAdvisoryMsg.choice().makeQueueAssignmentAdvisory();
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qAssignAdvisory.sequenceNumber());

    bmqp_ctrlmsg::QueueInfo qinfo;
    qinfo.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qinfo.partitionId() = 1U;

    mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(), "7777");
    key.loadBinary(&qinfo.key());

    qAssignAdvisory.queues().push_back(qinfo);

    bdlbb::Blob qAssignAdvisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                                     s_allocator_p);
    tester.constructEventBlob(&qAssignAdvisoryEvent,
                              qAssignAdvisoryMsg,
                              qAssignAdvisory.sequenceNumber(),
                              123002,
                              mqbc::ClusterStateRecordType::e_UPDATE);

    BSLS_ASSERT_OPT(obj->apply(qAssignAdvisoryEvent,
                               tester.d_cluster_mp->netCluster().lookupNode(
                                   mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

    // Apply 'QueueUnassignedAdvisory'
    bmqp_ctrlmsg::ClusterMessage           qUnassignedAdvisoryMsg;
    bmqp_ctrlmsg::QueueUnassignedAdvisory& qUnassignedAdvisory =
        qUnassignedAdvisoryMsg.choice().makeQueueUnassignedAdvisory();
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qUnassignedAdvisory.sequenceNumber());

    qUnassignedAdvisory.queues().push_back(qinfo);

    bdlbb::Blob qUnassignedAdvisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                                         s_allocator_p);
    tester.constructEventBlob(&qUnassignedAdvisoryEvent,
                              qUnassignedAdvisoryMsg,
                              qUnassignedAdvisory.sequenceNumber(),
                              123003,
                              mqbc::ClusterStateRecordType::e_UPDATE);

    BSLS_ASSERT_OPT(obj->apply(qUnassignedAdvisoryEvent,
                               tester.d_cluster_mp->netCluster().lookupNode(
                                   mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

    // Apply 'QueueUpdateAdvisory'
    bmqp_ctrlmsg::ClusterMessage       qUpdateAdvisoryMsg;
    bmqp_ctrlmsg::QueueUpdateAdvisory& qUpdateAdvisory =
        qUpdateAdvisoryMsg.choice().makeQueueUpdateAdvisory();
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qUpdateAdvisory.sequenceNumber());

    bmqp_ctrlmsg::QueueInfoUpdate qupdate;
    qupdate.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qupdate.partitionId() = 1U;

    mqbu::StorageKey key2(mqbu::StorageKey::BinaryRepresentation(), "8888");
    key2.loadBinary(&qupdate.key());

    qupdate.addedAppIds().resize(1);
    qupdate.removedAppIds().resize(1);
    qupdate.domain() = "bmq.test.mmap.priority";

    bmqp_ctrlmsg::AppIdInfo& addedAppId = qupdate.addedAppIds().back();
    addedAppId.appId()                  = "App1";
    mqbu::StorageKey appKey1(mqbu::StorageKey::BinaryRepresentation(),
                             "12345");
    appKey1.loadBinary(&addedAppId.appKey());

    bmqp_ctrlmsg::AppIdInfo& removedAppId = qupdate.removedAppIds().back();
    removedAppId.appId()                  = "App2";
    mqbu::StorageKey appKey2(mqbu::StorageKey::BinaryRepresentation(),
                             "23456");
    appKey2.loadBinary(&removedAppId.appKey());

    qUpdateAdvisory.queueUpdates().push_back(qupdate);

    bdlbb::Blob qUpdateAdvisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                                     s_allocator_p);
    tester.constructEventBlob(&qUpdateAdvisoryEvent,
                              qUpdateAdvisoryMsg,
                              qUpdateAdvisory.sequenceNumber(),
                              123004,
                              mqbc::ClusterStateRecordType::e_UPDATE);

    BSLS_ASSERT_OPT(obj->apply(qUpdateAdvisoryEvent,
                               tester.d_cluster_mp->netCluster().lookupNode(
                                   mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

    // Apply 'LeaderAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo2;
    pinfo2.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo2.partitionId()    = 2U;
    pinfo2.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::QueueInfo qinfo2;
    qinfo2.uri()         = "bmq://bmq.test.mmap.priority/q2";
    qinfo2.partitionId() = 2U;

    mqbu::StorageKey key3(mqbu::StorageKey::BinaryRepresentation(), "9999");
    key3.loadBinary(&qinfo2.key());

    bmqp_ctrlmsg::ClusterMessage  leaderAdvisoryMsg;
    bmqp_ctrlmsg::LeaderAdvisory& leaderAdvisory =
        leaderAdvisoryMsg.choice().makeLeaderAdvisory();
    leaderAdvisory.queues().push_back(qinfo2);
    leaderAdvisory.partitions().push_back(pinfo2);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&leaderAdvisory.sequenceNumber());

    bdlbb::Blob leaderAdvisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                                    s_allocator_p);
    tester.constructEventBlob(&leaderAdvisoryEvent,
                              leaderAdvisoryMsg,
                              leaderAdvisory.sequenceNumber(),
                              123005,
                              mqbc::ClusterStateRecordType::e_SNAPSHOT);

    BSLS_ASSERT_OPT(obj->apply(leaderAdvisoryEvent,
                               tester.d_cluster_mp->netCluster().lookupNode(
                                   mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

    // Apply commit of above 'LeaderAdvisory'
    bmqp_ctrlmsg::ClusterMessage        commitMsg;
    bmqp_ctrlmsg::LeaderAdvisoryCommit& commit =
        commitMsg.choice().makeLeaderAdvisoryCommit();
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&commit.sequenceNumber());
    commit.sequenceNumberCommitted() = leaderAdvisory.sequenceNumber();

    bdlbb::Blob commitEvent(tester.d_cluster_mp->_bufferFactory(),
                            s_allocator_p);
    tester.constructEventBlob(&commitEvent,
                              commitMsg,
                              commit.sequenceNumber(),
                              123006,
                              mqbc::ClusterStateRecordType::e_COMMIT);

    BSLS_ASSERT_OPT(obj->apply(commitEvent,
                               tester.d_cluster_mp->netCluster().lookupNode(
                                   mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

    // 2. Close the CSL
    BSLS_ASSERT_OPT(obj->close() == 0);

    // 3. Open the CSL and instantiate ClusterStateLedgerIterator
    BSLS_ASSERT_OPT(obj->open() == 0);

    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator> cslIter =
        obj->getIterator();

    // 4. Iterate through the records and verify that they are as expected

    // Verify 'PartitionPrimaryAdvisory'
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       pmAdvisory.sequenceNumber());
    ASSERT_EQ(cslIter->header().timestamp(), 123001U);

    bmqp_ctrlmsg::ClusterMessage msg;
    int                          rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isPartitionPrimaryAdvisoryValue());
    ASSERT_EQ(msg.choice().partitionPrimaryAdvisory(), pmAdvisory);

    // Verify 'QueueAssignmentAdvisory'
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       qAssignAdvisory.sequenceNumber());
    ASSERT_EQ(cslIter->header().timestamp(), 123002U);

    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isQueueAssignmentAdvisoryValue());
    ASSERT_EQ(msg.choice().queueAssignmentAdvisory(), qAssignAdvisory);

    // Verify 'QueueUnassignedAdvisory'
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       qUnassignedAdvisory.sequenceNumber());
    ASSERT_EQ(cslIter->header().timestamp(), 123003U);

    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isQueueUnassignedAdvisoryValue());
    ASSERT_EQ(msg.choice().queueUnassignedAdvisory(), qUnassignedAdvisory);

    // Verify 'QueueUpdateAdvisory'
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       qUpdateAdvisory.sequenceNumber());
    ASSERT_EQ(cslIter->header().timestamp(), 123004U);

    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isQueueUpdateAdvisoryValue());
    ASSERT_EQ(msg.choice().queueUpdateAdvisory(), qUpdateAdvisory);

    // Verify 'LeaderAdvisory' and its commit
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_SNAPSHOT,
                       leaderAdvisory.sequenceNumber());
    ASSERT_EQ(cslIter->header().timestamp(), 123005U);

    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isLeaderAdvisoryValue());
    ASSERT_EQ(msg.choice().leaderAdvisory(), leaderAdvisory);

    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());

    verifyLeaderAdvisoryCommit(*cslIter, leaderAdvisory.sequenceNumber());

    // Verify end of ledger
    ASSERT_EQ(cslIter->next(), 1);
    ASSERT(!cslIter->isValid());
}

static void test12_persistanceAcrossRollover()
// ------------------------------------------------------------------------
// PERSISTENCE ACROSS ROLLOVER
//
// Concerns:
//   Rollover works properly and the IncoreCSL provides persistence across
//   rollovers.
//
// Plan:
//   Open logs that were already written at a particular location:
//     1 Apply enough advisories to trigger rollover
//     2 Apply some more advisories and "save" them in a list,
//       lastAdvisories
//     3 Close the CSL
//     4 Open the CSL and instantiate ClusterStateLedgerIterator.
//     5 Verify the snapshot, then iterate over each record at a time and
//       compare to lastAdvisories
//
//  Testing:
//    Rollover and persistence.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PERSISTENCE ACROSS ROLLOVER");

    Tester                          tester;
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    const mqbsi::Ledger* ledger = obj->ledger();
    ASSERT_EQ(obj->ledger()->numLogs(), 1U);

    // Build 'QueueAssignmentAdvisory'
    bmqp_ctrlmsg::QueueAssignmentAdvisory qadvisory;
    for (size_t i = 0; i < 50; ++i) {
        bmqu::MemOutStream uriStream(s_allocator_p);
        uriStream << "bmq://bmq.test.mmap.priority/q" << i;

        bmqp_ctrlmsg::QueueInfo qinfo;
        qinfo.uri()         = uriStream.str();
        qinfo.partitionId() = i % 4U;

        mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(),
                             bsl::to_string(12300 + i).c_str());
        key.loadBinary(&qinfo.key());

        qadvisory.queues().push_back(qinfo);
    }

    bdlsb::MemOutStreamBuf osb;
    balber::BerEncoder     encoder;
    int                    rc = encoder.encode(&osb, qadvisory);
    BSLS_ASSERT_OPT(rc == 0);

    // 1. Apply enough advisories to trigger rollover
    size_t i = 0;
    while (ledger->numLogs() == 1U) {
        tester.d_cluster_mp->_clusterData()
            ->electorInfo()
            .nextLeaderMessageSequence(&qadvisory.sequenceNumber());

        ASSERT_EQ(obj->apply(qadvisory), 0);

        bmqp_ctrlmsg::ControlMessage expected;
        expected.choice()
            .makeClusterMessage()
            .choice()
            .makeQueueAssignmentAdvisory(qadvisory);
        BSLS_ASSERT_OPT(tester.numCommittedMessages() == i + 1);
        BSLS_ASSERT_OPT(tester.committedMessage(i) == expected);
        BSLS_ASSERT_OPT(tester.hasBroadcastedMessages(2 * (i + 1)));
        BSLS_ASSERT_OPT(tester.broadcastedMessage(2 * i) == expected);

        ++i;
    }
    ASSERT_EQ(ledger->numLogs(), 2U);

    // Since our logic for determining the newest log compares the last
    // modification times and its precision is only up to the second, we sleep
    // for 1 second to make sure the logs are written 1 second apart.
    sleep(1);

    // 2. Apply some more advisories and "save" them in a list
    bsl::vector<AdvisoryInfo> lastAdvisories(s_allocator_p);

    bmqp_ctrlmsg::ControlMessage advisoryToCauseRollover;
    advisoryToCauseRollover.choice()
        .makeClusterMessage()
        .choice()
        .makeQueueAssignmentAdvisory(qadvisory);
    lastAdvisories.push_back(
        AdvisoryInfo(advisoryToCauseRollover,
                     qadvisory.sequenceNumber(),
                     mqbc::ClusterStateRecordType::e_UPDATE));

    // Apply 'PartitionPrimaryAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
    pinfo.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo.partitionId()    = 1U;
    pinfo.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::PartitionPrimaryAdvisory pmAdvisory;
    pmAdvisory.partitions().push_back(pinfo);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&pmAdvisory.sequenceNumber());
    ASSERT_EQ(obj->apply(pmAdvisory), 0);

    bmqp_ctrlmsg::ControlMessage expectedPmAdvisory;
    expectedPmAdvisory.choice()
        .makeClusterMessage()
        .choice()
        .makePartitionPrimaryAdvisory(pmAdvisory);
    ASSERT_EQ(tester.numCommittedMessages(), i + 1);
    ASSERT_EQ(tester.committedMessage(i), expectedPmAdvisory);
    ASSERT(tester.hasBroadcastedMessages(2 * (i + 1)));
    ASSERT_EQ(tester.broadcastedMessage(2 * i), expectedPmAdvisory);

    lastAdvisories.push_back(
        AdvisoryInfo(expectedPmAdvisory,
                     pmAdvisory.sequenceNumber(),
                     mqbc::ClusterStateRecordType::e_UPDATE));

    // Apply 'QueueUnassignedAdvisory'
    bmqp_ctrlmsg::QueueUnassignedAdvisory qUnassignedAdvisory;
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qUnassignedAdvisory.sequenceNumber());

    bmqp_ctrlmsg::QueueInfo qinfo;
    qinfo.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qinfo.partitionId() = 1U;

    mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(), "12301");
    key.loadBinary(&qinfo.key());

    qUnassignedAdvisory.queues().push_back(qinfo);

    ASSERT_EQ(obj->apply(qUnassignedAdvisory), 0);

    bmqp_ctrlmsg::ControlMessage expectedQUnassignedAdvisory;
    expectedQUnassignedAdvisory.choice()
        .makeClusterMessage()
        .choice()
        .makeQueueUnassignedAdvisory(qUnassignedAdvisory);
    ASSERT_EQ(tester.numCommittedMessages(), i + 2);
    ASSERT_EQ(tester.committedMessage(i + 1), expectedQUnassignedAdvisory);
    ASSERT(tester.hasBroadcastedMessages(2 * (i + 2)));
    ASSERT_EQ(tester.broadcastedMessage(2 * (i + 1)),
              expectedQUnassignedAdvisory);

    lastAdvisories.push_back(
        AdvisoryInfo(expectedQUnassignedAdvisory,
                     qUnassignedAdvisory.sequenceNumber(),
                     mqbc::ClusterStateRecordType::e_UPDATE));

    // Apply 'LeaderAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo2;
    pinfo2.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo2.partitionId()    = 2U;
    pinfo2.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::QueueInfo qinfo2;
    qinfo2.uri()         = "bmq://bmq.test.mmap.priority/q2";
    qinfo2.partitionId() = 2U;

    mqbu::StorageKey key2(mqbu::StorageKey::BinaryRepresentation(), "12302");
    key2.loadBinary(&qinfo2.key());

    bmqp_ctrlmsg::LeaderAdvisory leaderAdvisory;
    leaderAdvisory.queues().push_back(qinfo2);
    leaderAdvisory.partitions().push_back(pinfo2);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&leaderAdvisory.sequenceNumber());

    ASSERT_EQ(obj->apply(leaderAdvisory), 0);

    bmqp_ctrlmsg::ControlMessage expectedLeaderAdvisory;
    expectedLeaderAdvisory.choice()
        .makeClusterMessage()
        .choice()
        .makeLeaderAdvisory(leaderAdvisory);
    ASSERT_EQ(tester.numCommittedMessages(), i + 3);
    ASSERT_EQ(tester.committedMessage(i + 2), expectedLeaderAdvisory);
    ASSERT(tester.hasBroadcastedMessages(2 * (i + 3)));
    ASSERT_EQ(tester.broadcastedMessage(2 * (i + 2)), expectedLeaderAdvisory);

    lastAdvisories.push_back(
        AdvisoryInfo(expectedLeaderAdvisory,
                     leaderAdvisory.sequenceNumber(),
                     mqbc::ClusterStateRecordType::e_SNAPSHOT));

    // 3. Close the CSL
    BSLS_ASSERT_OPT(obj->close() == 0);

    // 4. Open the CSL and instantiate ClusterStateLedgerIterator.
    BSLS_ASSERT_OPT(obj->open() == 0);

    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator> cslIter =
        obj->getIterator();

    // 5. Verify the snapshot and the last advisories
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_SNAPSHOT,
                       qadvisory.sequenceNumber());

    bmqp_ctrlmsg::ClusterMessage snapshotMsg;
    rc = cslIter->loadClusterMessage(&snapshotMsg);
    ASSERT_EQ(cslIter->loadClusterMessage(&snapshotMsg), 0);
    ASSERT(snapshotMsg.choice().isLeaderAdvisoryValue());

    bmqp_ctrlmsg::LeaderAdvisory& snapshot =
        snapshotMsg.choice().leaderAdvisory();
    bsl::sort(snapshot.queues().begin(),
              snapshot.queues().end(),
              compareQueueInfo);
    ASSERT_EQ(snapshot.queues(), qadvisory.queues());

    for (bsl::vector<AdvisoryInfo>::const_iterator cit =
             lastAdvisories.cbegin();
         cit != lastAdvisories.cend();
         ++cit) {
        ASSERT_EQ(cslIter->next(), 0);
        ASSERT(cslIter->isValid());
        verifyRecordHeader(*cslIter, cit->d_recordType, cit->d_sequenceNumber);

        bmqp_ctrlmsg::ClusterMessage msg;
        rc = cslIter->loadClusterMessage(&msg);
        ASSERT_EQ(rc, 0);
        ASSERT(cit->d_advisory.choice().isClusterMessageValue());
        ASSERT_EQ(msg, cit->d_advisory.choice().clusterMessage());

        ASSERT_EQ(cslIter->next(), 0);
        ASSERT(cslIter->isValid());
        verifyLeaderAdvisoryCommit(*cslIter, cit->d_sequenceNumber);
    }

    // Verify end of ledger
    ASSERT_EQ(cslIter->next(), 1);
    ASSERT(!cslIter->isValid());

    BSLS_ASSERT_OPT(obj->close() == 0);

    // TODO Test that if node is available, a cluster state snapshot is written
    // upon rollover, and that a snapshot is *NOT* written if node is not
    // available.
    // Can explicitly set node status via:
    // d_cluster_mp->_clusterData()->membership().setSelfNodeStatus(
    //                                  bmqp_ctrlmsg::NodeStatus::E_AVAILABLE);
}

static void test13_rolloverUncommittedAdvisories()
// ------------------------------------------------------------------------
// ROLLOVER UNCOMMITTED ADVISORIES
//
// Concerns:
//   During rollover, uncommitted advisories are written to the new log
//   before the cluster state snapshot.
//
// Plan:
//   1 Apply some advisory to remain uncommitted
//   2 Apply enough committed advisories to trigger rollover
//   3 Close the CSL
//   4 Open the CSL and instantiate ClusterStateLedgerIterator.
//   5 Verify that the uncommitted advisories are written to the new log,
//     followed by a snapshot
//
//  Testing:
//    Rollover uncommitted advisories.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ROLLOVER UNCOMMITTED ADVISORIES");

    Tester                          tester(false);  // Leader
    mqbc::IncoreClusterStateLedger* obj = tester.d_clusterStateLedger_mp.get();
    BSLS_ASSERT_OPT(obj->open() == 0);

    const mqbsi::Ledger* ledger = obj->ledger();
    ASSERT_EQ(obj->ledger()->numLogs(), 1U);

    // 1. Apply some advisory to remain uncommitted
    bsl::vector<AdvisoryInfo> uncommittedAdvisories(s_allocator_p);

    // Apply 'PartitionPrimaryAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
    pinfo.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo.partitionId()    = 1U;
    pinfo.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::ClusterMessage            pmAdvisoryMsg;
    bmqp_ctrlmsg::PartitionPrimaryAdvisory& pmAdvisory =
        pmAdvisoryMsg.choice().makePartitionPrimaryAdvisory();
    pmAdvisory.partitions().push_back(pinfo);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&pmAdvisory.sequenceNumber());

    bdlbb::Blob pmAdvisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                                s_allocator_p);
    tester.constructEventBlob(&pmAdvisoryEvent,
                              pmAdvisoryMsg,
                              pmAdvisory.sequenceNumber(),
                              111001,
                              mqbc::ClusterStateRecordType::e_UPDATE);

    BSLS_ASSERT_OPT(obj->apply(pmAdvisoryEvent,
                               tester.d_cluster_mp->netCluster().lookupNode(
                                   mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

    bmqp_ctrlmsg::ControlMessage expectedPmAdvisory;
    expectedPmAdvisory.choice()
        .makeClusterMessage()
        .choice()
        .makePartitionPrimaryAdvisory(pmAdvisory);
    uncommittedAdvisories.push_back(
        AdvisoryInfo(expectedPmAdvisory,
                     pmAdvisory.sequenceNumber(),
                     mqbc::ClusterStateRecordType::e_UPDATE));

    BSLS_ASSERT_OPT(tester.numCommittedMessages() == 0);
    BSLS_ASSERT_OPT(tester.hasBroadcastedMessages(0));

    // Apply 'QueueUnassignedAdvisory'
    bmqp_ctrlmsg::ClusterMessage           qUnassignedAdvisoryMsg;
    bmqp_ctrlmsg::QueueUnassignedAdvisory& qUnassignedAdvisory =
        qUnassignedAdvisoryMsg.choice().makeQueueUnassignedAdvisory();
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&qUnassignedAdvisory.sequenceNumber());

    bmqp_ctrlmsg::QueueInfo qinfo;
    qinfo.uri()         = "bmq://bmq.test.mmap.priority/q1";
    qinfo.partitionId() = 1U;

    mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(), "12301");
    key.loadBinary(&qinfo.key());

    qUnassignedAdvisory.queues().push_back(qinfo);

    bdlbb::Blob qUnassignedAdvisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                                         s_allocator_p);
    tester.constructEventBlob(&qUnassignedAdvisoryEvent,
                              qUnassignedAdvisoryMsg,
                              qUnassignedAdvisory.sequenceNumber(),
                              111002,
                              mqbc::ClusterStateRecordType::e_UPDATE);

    BSLS_ASSERT_OPT(obj->apply(qUnassignedAdvisoryEvent,
                               tester.d_cluster_mp->netCluster().lookupNode(
                                   mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

    bmqp_ctrlmsg::ControlMessage expectedQUnassignedAdvisory;
    expectedQUnassignedAdvisory.choice()
        .makeClusterMessage()
        .choice()
        .makeQueueUnassignedAdvisory(qUnassignedAdvisory);
    uncommittedAdvisories.push_back(
        AdvisoryInfo(expectedQUnassignedAdvisory,
                     qUnassignedAdvisory.sequenceNumber(),
                     mqbc::ClusterStateRecordType::e_UPDATE));

    BSLS_ASSERT_OPT(tester.numCommittedMessages() == 0);
    BSLS_ASSERT_OPT(tester.hasBroadcastedMessages(0));

    // Apply 'LeaderAdvisory'
    bmqp_ctrlmsg::PartitionPrimaryInfo pinfo2;
    pinfo2.primaryNodeId()  = mqbmock::Cluster::k_LEADER_NODE_ID;
    pinfo2.partitionId()    = 2U;
    pinfo2.primaryLeaseId() = 2U;

    bmqp_ctrlmsg::QueueInfo qinfo2;
    qinfo2.uri()         = "bmq://bmq.test.mmap.priority/q2";
    qinfo2.partitionId() = 2U;

    mqbu::StorageKey key2(mqbu::StorageKey::BinaryRepresentation(), "12302");
    key2.loadBinary(&qinfo2.key());

    bmqp_ctrlmsg::ClusterMessage  leaderAdvisoryMsg;
    bmqp_ctrlmsg::LeaderAdvisory& leaderAdvisory =
        leaderAdvisoryMsg.choice().makeLeaderAdvisory();
    leaderAdvisory.queues().push_back(qinfo2);
    leaderAdvisory.partitions().push_back(pinfo2);
    tester.d_cluster_mp->_clusterData()
        ->electorInfo()
        .nextLeaderMessageSequence(&leaderAdvisory.sequenceNumber());

    bdlbb::Blob leaderAdvisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                                    s_allocator_p);
    tester.constructEventBlob(&leaderAdvisoryEvent,
                              leaderAdvisoryMsg,
                              leaderAdvisory.sequenceNumber(),
                              111003,
                              mqbc::ClusterStateRecordType::e_SNAPSHOT);

    BSLS_ASSERT_OPT(obj->apply(leaderAdvisoryEvent,
                               tester.d_cluster_mp->netCluster().lookupNode(
                                   mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

    bmqp_ctrlmsg::ControlMessage expectedLeaderAdvisory;
    expectedLeaderAdvisory.choice()
        .makeClusterMessage()
        .choice()
        .makeLeaderAdvisory(leaderAdvisory);
    uncommittedAdvisories.push_back(
        AdvisoryInfo(expectedLeaderAdvisory,
                     leaderAdvisory.sequenceNumber(),
                     mqbc::ClusterStateRecordType::e_SNAPSHOT));

    BSLS_ASSERT_OPT(tester.numCommittedMessages() == 0);
    BSLS_ASSERT_OPT(tester.hasBroadcastedMessages(0));

    // 2. Apply enough committed advisories to trigger rollover

    // Build 'QueueAssignmentAdvisory'
    bmqp_ctrlmsg::ClusterMessage           qAssignAdvisoryMsg;
    bmqp_ctrlmsg::QueueAssignmentAdvisory& qAssignAdvisory =
        qAssignAdvisoryMsg.choice().makeQueueAssignmentAdvisory();
    for (size_t i = 0; i < 50; ++i) {
        bmqu::MemOutStream uriStream(s_allocator_p);
        uriStream << "bmq://bmq.test.mmap.priority/q" << i;

        bmqp_ctrlmsg::QueueInfo queueInfo;
        queueInfo.uri()         = uriStream.str();
        queueInfo.partitionId() = i % 4U;

        mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                                  bsl::to_string(12300 + i).c_str());
        queueKey.loadBinary(&queueInfo.key());

        qAssignAdvisory.queues().push_back(queueInfo);
    }

    bdlsb::MemOutStreamBuf osb;
    balber::BerEncoder     encoder;
    int                    rc = encoder.encode(&osb, qAssignAdvisory);
    BSLS_ASSERT_OPT(rc == 0);

    bmqp_ctrlmsg::ClusterMessage        commitMsg;
    bmqp_ctrlmsg::LeaderAdvisoryCommit& commit =
        commitMsg.choice().makeLeaderAdvisoryCommit();

    // Repeatedly apply above advisory and its commit, until rollover
    size_t i = 0;
    while (ledger->numLogs() == 1U) {
        tester.d_cluster_mp->_clusterData()
            ->electorInfo()
            .nextLeaderMessageSequence(&qAssignAdvisory.sequenceNumber());

        bdlbb::Blob qAssignAdvisoryEvent(tester.d_cluster_mp->_bufferFactory(),
                                         s_allocator_p);
        tester.constructEventBlob(&qAssignAdvisoryEvent,
                                  qAssignAdvisoryMsg,
                                  qAssignAdvisory.sequenceNumber(),
                                  123000 + (2 * i),
                                  mqbc::ClusterStateRecordType::e_UPDATE);

        BSLS_ASSERT_OPT(
            obj->apply(qAssignAdvisoryEvent,
                       tester.d_cluster_mp->netCluster().lookupNode(
                           mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

        tester.d_cluster_mp->_clusterData()
            ->electorInfo()
            .nextLeaderMessageSequence(&commit.sequenceNumber());
        commit.sequenceNumberCommitted() = qAssignAdvisory.sequenceNumber();

        bdlbb::Blob commitEvent(tester.d_cluster_mp->_bufferFactory(),
                                s_allocator_p);
        tester.constructEventBlob(&commitEvent,
                                  commitMsg,
                                  commit.sequenceNumber(),
                                  123001 + (2 * i),
                                  mqbc::ClusterStateRecordType::e_COMMIT);

        BSLS_ASSERT_OPT(
            obj->apply(commitEvent,
                       tester.d_cluster_mp->netCluster().lookupNode(
                           mqbmock::Cluster::k_LEADER_NODE_ID)) == 0);

        bmqp_ctrlmsg::ControlMessage expected;
        expected.choice()
            .makeClusterMessage()
            .choice()
            .makeQueueAssignmentAdvisory(qAssignAdvisory);
        BSLS_ASSERT_OPT(tester.numCommittedMessages() == i + 1);
        BSLS_ASSERT_OPT(tester.committedMessage(i) == expected);
        BSLS_ASSERT_OPT(tester.hasBroadcastedMessages(0));

        ++i;
    }
    ASSERT_EQ(ledger->numLogs(), 2U);

    // Since our logic for determining the newest log compares the last
    // modification times and its precision is only up to the second, we sleep
    // for 1 second to make sure the logs are written 1 second apart.
    sleep(1);

    // 3. Close the CSL
    BSLS_ASSERT_OPT(obj->close() == 0);

    // 4. Open the CSL and instantiate ClusterStateLedgerIterator.
    BSLS_ASSERT_OPT(obj->open() == 0);

    bslma::ManagedPtr<mqbc::ClusterStateLedgerIterator> cslIter =
        obj->getIterator();

    // 5. Verify that the uncommitted advisories are written to the new log,
    //    followed by a snapshot
    for (bsl::vector<AdvisoryInfo>::const_iterator cit =
             uncommittedAdvisories.cbegin();
         cit != uncommittedAdvisories.cend();
         ++cit) {
        ASSERT_EQ(cslIter->next(), 0);
        ASSERT(cslIter->isValid());
        verifyRecordHeader(*cslIter, cit->d_recordType, cit->d_sequenceNumber);

        bmqp_ctrlmsg::ClusterMessage msg;
        rc = cslIter->loadClusterMessage(&msg);
        ASSERT_EQ(rc, 0);
        ASSERT(cit->d_advisory.choice().isClusterMessageValue());
        ASSERT_EQ(msg, cit->d_advisory.choice().clusterMessage());
    }

    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_SNAPSHOT,
                       qAssignAdvisory.sequenceNumber());

    bmqp_ctrlmsg::ClusterMessage snapshotMsg;
    rc = cslIter->loadClusterMessage(&snapshotMsg);
    ASSERT_EQ(cslIter->loadClusterMessage(&snapshotMsg), 0);
    ASSERT(snapshotMsg.choice().isLeaderAdvisoryValue());

    bmqp_ctrlmsg::LeaderAdvisory& snapshot =
        snapshotMsg.choice().leaderAdvisory();
    bsl::sort(snapshot.queues().begin(),
              snapshot.queues().end(),
              compareQueueInfo);
    ASSERT_EQ(snapshot.queues(), qAssignAdvisory.queues());

    // Verify that the advisory to cause rollover is right after the snapshot
    bmqp_ctrlmsg::ControlMessage advisoryToCauseRollover;
    advisoryToCauseRollover.choice()
        .makeClusterMessage()
        .choice()
        .makeQueueAssignmentAdvisory(qAssignAdvisory);
    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyRecordHeader(*cslIter,
                       mqbc::ClusterStateRecordType::e_UPDATE,
                       qAssignAdvisory.sequenceNumber());

    bmqp_ctrlmsg::ClusterMessage msg;
    rc = cslIter->loadClusterMessage(&msg);
    ASSERT_EQ(rc, 0);
    ASSERT(msg.choice().isQueueAssignmentAdvisoryValue());
    ASSERT_EQ(msg.choice().queueAssignmentAdvisory(), qAssignAdvisory);

    ASSERT_EQ(cslIter->next(), 0);
    ASSERT(cslIter->isValid());
    verifyLeaderAdvisoryCommit(*cslIter, qAssignAdvisory.sequenceNumber());

    // Verify end of ledger
    ASSERT_EQ(cslIter->next(), 1);
    ASSERT(!cslIter->isValid());

    BSLS_ASSERT_OPT(obj->close() == 0);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize(s_allocator_p);
    bmqp::ProtocolUtil::initialize(s_allocator_p);
    bmqp::Crc32c::initialize();
    bmqt::UriParser::initialize(s_allocator_p);

    switch (_testCase) {
    case 0:
    case 13: test13_rolloverUncommittedAdvisories(); break;
    case 12: test12_persistanceAcrossRollover(); break;
    case 11: test11_persistanceFollower(); break;
    case 10: test10_persistanceLeader(); break;
    case 9: test9_apply_ClusterStateRecordCommit(); break;
    case 8: test8_apply_ClusterStateRecordAck(); break;
    case 7: test7_apply_ClusterStateRecord(); break;
    case 6: test6_apply_LeaderAdvisory(); break;
    case 5: test5_apply_QueueUpdateAdvisory(); break;
    case 4: test4_apply_QueueUnassignedAdvisory(); break;
    case 3: test3_apply_QueueAssignmentAdvisory(); break;
    case 2: test2_apply_PartitionPrimaryAdvisory(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqt::UriParser::shutdown();
    bmqp::ProtocolUtil::shutdown();
    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
    // The tester object makes use of 'bmqu::TempDirectory', which
    // allocates a temporary string using the default allocator.
}
