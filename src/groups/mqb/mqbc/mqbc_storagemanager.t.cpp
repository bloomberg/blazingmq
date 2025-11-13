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

// mqbc_storagemanager.t.cpp                                          -*-C++-*-
#include <mqbc_storagemanager.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_storagemessageiterator.h>
#include <bmqt_compressionalgorithmtype.h>

// MQB
#include <mqbc_clusterstate.h>
#include <mqbc_clusterutil.h>
#include <mqbc_partitionfsm.h>
#include <mqbc_partitionfsmobserver.h>
#include <mqbc_storageutil.h>
#include <mqbcfg_messages.h>
#include <mqbi_storage.h>
#include <mqbmock_cluster.h>
#include <mqbnet_mockcluster.h>
#include <mqbs_datastore.h>
#include <mqbs_filestore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoretestutil.h>
#include <mqbs_storageutil.h>
#include <mqbscm_version.h>
#include <mqbstat_brokerstats.h>
#include <mqbu_messageguidutil.h>
#include <mqbu_storagekey.h>

#include <bmqsys_time.h>
#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdlmt_fixedthreadpool.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bsls_systemtime.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bmqu_tempdirectory.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {
// CONSTANTS
static const bsls::Types::Int64 k_WATCHDOG_TIMEOUT_DURATION = 5 * 60;
// 5 minutes

// TYPES
typedef mqbmock::Cluster::TestChannelMapCIter TestChannelMapCIter;

typedef mqbc::StorageManager::NodeToSeqNumCtxMap NodeToSeqNumCtxMap;
typedef mqbc::StorageManager::NodeToSeqNumCtxMap::const_iterator
    NodeToSeqNumCtxMapCIter;
typedef bsl::pair<mqbnet::ClusterNode*, bmqp_ctrlmsg::PartitionSequenceNumber>
    NodeSeqNumPair;

typedef bsl::unordered_map<int, int> ReqIdToNodeIdMap;

typedef bsl::unordered_map<int, bmqp_ctrlmsg::PartitionSequenceNumber>
                                          NodeIdToSeqNumMap;
typedef NodeIdToSeqNumMap::const_iterator NodeIdToSeqNumMapCIter;

// FUNCTIONS
void mockOnRecoveryStatus(int status)
{
    PV("Storage recovery completed with status: " << status);
}

void mockOnPartitionPrimaryStatus(int          partitionId,
                                  int          status,
                                  unsigned int primaryLeaseId)
{
    PV("On partition primary status, partitionId: "
       << partitionId << ", status: " << status
       << ", primaryLeaseId: " << primaryLeaseId);
}

// CLASSES
// ===================
// struct TesterHelper
// ===================

/// Provide helper methods to abstract away the code needed to modify the
/// cluster state which is required by storage manager.
struct TestHelper {
  private:
    // PRIVATE TYPES
    typedef mqbnet::Cluster::NodesList NodesList;
    typedef NodesList::iterator        NodesListIter;

  public:
    // PUBLIC DATA
    bslma::ManagedPtr<mqbmock::Cluster> d_cluster_mp;

    bmqu::TempDirectory d_tempDir;

    bmqu::TempDirectory d_tempArchiveDir;

    // CREATORS
    TestHelper()
    : d_cluster_mp(0)
    , d_tempDir(bmqtst::TestHelperUtil::allocator())
    , d_tempArchiveDir(bmqtst::TestHelperUtil::allocator())
    {
        // Create the cluster
        mqbmock::Cluster::ClusterNodeDefs clusterNodeDefs(
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "E1",
            "US-EAST",
            41234,
            mqbmock::Cluster::k_LEADER_NODE_ID,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "E2",
            "US-EAST",
            41235,
            mqbmock::Cluster::k_LEADER_NODE_ID + 1,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "W1",
            "US-WEST",
            41236,
            mqbmock::Cluster::k_LEADER_NODE_ID + 2,
            bmqtst::TestHelperUtil::allocator());
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            "W2",
            "US-WEST",
            41237,
            mqbmock::Cluster::k_LEADER_NODE_ID + 3,
            bmqtst::TestHelperUtil::allocator());

        d_cluster_mp.load(
            new (*bmqtst::TestHelperUtil::allocator())
                mqbmock::Cluster(bmqtst::TestHelperUtil::allocator(),
                                 true,   // isClusterMember
                                 false,  // isLeader
                                 true,   // isCSLMode
                                 true,   // isFSMWorkflow
                                 false,  // doesFSMwriteQLIST
                                 clusterNodeDefs,
                                 "testCluster",
                                 d_tempDir.path(),
                                 d_tempArchiveDir.path()),
            bmqtst::TestHelperUtil::allocator());

        d_cluster_mp->_clusterData()->stats().setIsMember(true);

        BSLS_ASSERT_OPT(d_cluster_mp->_channels().size() > 0);

        bmqsys::Time::initialize(
            &bsls::SystemTime::nowRealtimeClock,
            bdlf::BindUtil::bind(&TestHelper::nowMonotonicClock, this),
            bdlf::BindUtil::bind(&TestHelper::highResolutionTimer, this),
            bmqtst::TestHelperUtil::allocator());

        // Start the cluster
        bmqu::MemOutStream errorDescription;
        int                rc = d_cluster_mp->start(errorDescription);
        BSLS_ASSERT_OPT(rc == 0);
    }

    void setPartitionPrimary(mqbc::StorageManager* storageManager,
                             int                   partitionId,
                             unsigned int          leaseId,
                             mqbnet::ClusterNode*  node)
    {
        d_cluster_mp->_state()->setPartitionPrimary(partitionId,
                                                    leaseId,
                                                    node);
        storageManager->setPrimaryForPartition(partitionId, node, leaseId);
    }

    void clearChannels()
    {
        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            cit->second->reset();
        }
    }

    void verifyPrimarySendsReplicaStateRqst(
        int                                   partitionId,
        int                                   selfNodeId,
        bmqp_ctrlmsg::PartitionSequenceNumber seqNum =
            bmqp_ctrlmsg::PartitionSequenceNumber(),
        ReqIdToNodeIdMap* reqIdToNodeIdMap = 0)
    {
        bmqp_ctrlmsg::ClusterMessage       expectedMessage;
        bmqp_ctrlmsg::ReplicaStateRequest& replicaStateRequest =
            expectedMessage.choice()
                .makePartitionMessage()
                .choice()
                .makeReplicaStateRequest();

        replicaStateRequest.partitionId()    = partitionId;
        replicaStateRequest.latestSequenceNumber() = seqNum;

        // TODO: set sequence number once add mocked recovery manager to this
        //       test helper class.

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() != selfNodeId) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message.choice().clusterMessage(),
                                 expectedMessage);

                if (reqIdToNodeIdMap) {
                    reqIdToNodeIdMap->insert(
                        bsl::make_pair(message.rId().value(),
                                       cit->first->nodeId()));
                }
            }
            else {
                // Make sure that primary node does not end up receiving
                // replicaStateRequest.
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyPrimarySendsDataChunks(
        int                                          partitionId,
        const bmqp_ctrlmsg::PartitionSequenceNumber& selfSeqNum,
        const NodeIdToSeqNumMap&                     destinationReplicas)
    // const mqbs::FileStore&                          fs,
    // const bsl::vector<mqbs::DataStoreRecordHandle>& handles)
    {
        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            NodeIdToSeqNumMapCIter replicaCit = destinationReplicas.find(
                cit->first->nodeId());
            if (replicaCit != destinationReplicas.end()) {
                // TODO The logic is flawed if self and primary have different
                // primary leaseIds, e.g. (2,10) vs (1,2).  We will have to fix
                // this after Mohit had updated the send-side logic.

                // Note: There is always a preceding ReplicaDataRqstPush

                if (replicaCit->second.sequenceNumber() ==
                    selfSeqNum.sequenceNumber()) {
                    // No data chunk was sent
                    BMQTST_ASSERT(cit->second->waitFor(1, false));

                    continue;  // CONTINUE
                }

                // Data chunks were sent
                BMQTST_ASSERT_LT(replicaCit->second.sequenceNumber(),
                                 selfSeqNum.sequenceNumber());
                BMQTST_ASSERT(cit->second->waitFor(2, false));

                // Verify data chunks
                //
                // Taking inspirations from
                // mqbc::RecoveryManager::processReceiveDataChunks()
                // TODO need?
                // const int numDataChunks =  selfSeqNum.sequenceNumber()
                //                       - replicaCit->second.sequenceNumber();

                bsl::shared_ptr<bdlbb::Blob> blob_sp;
                blob_sp.createInplace(bmqtst::TestHelperUtil::allocator(),
                                      bmqtst::TestHelperUtil::allocator());
                bdlbb::BlobUtil::append(
                    blob_sp.get(),
                    cit->second->writeCalls()[1].d_blob,
                    0,
                    cit->second->writeCalls()[1].d_blob.length(),
                    bmqtst::TestHelperUtil::allocator());

                bmqp::Event event(blob_sp.get(),
                                  bmqtst::TestHelperUtil::allocator());
                BSLS_ASSERT_SAFE(event.isPartitionSyncEvent());

                bmqp::StorageMessageIterator iter;
                event.loadStorageMessageIterator(&iter);
                BSLS_ASSERT_SAFE(iter.isValid());

                // Iterate over each message and validate a few things.
                while (1 == iter.next()) {
                    const bmqp::StorageHeader& header = iter.header();
                    BMQTST_ASSERT_EQ(static_cast<unsigned int>(partitionId),
                                     header.partitionId());
                    bmqu::MemOutStream partitionDesc;
                    partitionDesc << d_cluster_mp->_clusterData()
                                         ->identity()
                                         .description()
                                  << " Partition [" << partitionId << "]: ";

                    bmqu::BlobPosition                        recordPosition;
                    bmqu::BlobObjectProxy<mqbs::RecordHeader> recHeader;
                    int rc = mqbs::StorageUtil::loadRecordHeaderAndPos(
                        &recHeader,
                        &recordPosition,
                        iter,
                        blob_sp,
                        partitionDesc.str());
                    BMQTST_ASSERT_EQ(rc, 0);

                    bmqp_ctrlmsg::PartitionSequenceNumber recordSeqNum;
                    recordSeqNum.primaryLeaseId() =
                        recHeader->primaryLeaseId();
                    recordSeqNum.sequenceNumber() =
                        recHeader->sequenceNumber();
                    BMQTST_ASSERT_GT(recordSeqNum, replicaCit->second);
                    BMQTST_ASSERT_LE(recordSeqNum, selfSeqNum);
                }

                /*int recordOffset = 0;
                for (bmqp_ctrlmsg::PartitionSequenceNumber currSeqNum
                                                          = replicaCit->second;
                     currSeqNum < selfSeqNum;
                     ++currSeqNum.sequenceNumber()) {*/

                /*const int recordOffset =
                                     blob.length()
                                   - (   (  selfSeqNum.sequenceNumber()
                                          - currSeqNum.sequenceNumber())
                                       * mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);*/

                // bmqu::BlobPosition                         pos;
                // bmqu::BlobUtil::findOffsetSafe(&pos, blob, recordOffset);
                // TODO
                // bmqu::BlobObjectProxy<mqbs::QueueOpRecord> record(&blob,
                //                                                   pos);
                // BMQTST_ASSERT(record.isSet());

                // bmqu::BlobObjectProxy<mqbs::RecordHeader> recordHdr(&blob,
                //                                                     pos);
                // BMQTST_ASSERT(recordHdr.isSet());
                // PV("XXM: " << currSeqNum << ", " << recordHdr->type());

                /*
                mqbs::QueueOpRecord expectedRecord;
                fs.loadQueueOpRecordRaw(
                              &expectedRecord,
                              handles.at(currSeqNum.sequenceNumber() - 1));
                BMQTST_ASSERT_EQ(record->header(),   expectedRecord.header());
                BMQTST_ASSERT_EQ(record->flags(),    expectedRecord.flags());
                BMQTST_ASSERT_EQ(record->queueKey(),
                expectedRecord.queueKey()); BMQTST_ASSERT_EQ(record->appKey(),
                expectedRecord.appKey()); BMQTST_ASSERT_EQ(record->type(),
                expectedRecord.type());
                BMQTST_ASSERT_EQ(record->queueUriRecordOffsetWords(),
                          expectedRecord.queueUriRecordOffsetWords());
                BMQTST_ASSERT_EQ(record->magic(),    expectedRecord.magic());
                */

                // recordOffset +=
                //           mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
                //}
            }
            else {
                // Make sure that primary node does not end up receiving
                // data chunks or send it to upto date replicas.
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyReplicaSendsDataChunksAndReplicaDataRspnPull(
        // int                                          partitionId,
        int                                          primaryNodeId,
        const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
        const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum)
    // const mqbs::FileStore&                       fs,
    // const mqbs::DataStoreRecordHandle&           handle)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(beginSeqNum < endSeqNum);

        // TODO Implement this logic after data chunk send logic is updated.
        /*
        bmqp_ctrlmsg::ClusterMessage      expectedMessage;
        bmqp_ctrlmsg::ReplicaDataResponse& replicaDataResponse =
                            expectedMessage.choice().makePartitionMessage()
                                           .choice().makeReplicaDataResponse();

        replicaDataResponse.replicaDataType()     =
                                         bmqp_ctrlmsg::ReplicaDataType::E_PULL;
        replicaDataResponse.partitionId()         = partitionId;
        replicaDataResponse.beginSequenceNumber() = beginSeqNum;
        replicaDataResponse.endSequenceNumber()   = endSeqNum;

        mqbs::QueueOpRecord expectedRecord;
        fs.loadQueueOpRecordRaw(&expectedRecord, handle);
        */

        const int expectedNumDataChunks = endSeqNum.sequenceNumber() -
                                          beginSeqNum.sequenceNumber();

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() == primaryNodeId) {
                BMQTST_ASSERT(
                    cit->second->waitFor(expectedNumDataChunks, false));
                // BMQTST_ASSERT(!cit->second->waitFor(expectedNumDataChunks +
                // 1,
                //       false));
                /*
                // Verify data chunks
                const bdlbb::Blob blob = cit->second->writeCalls()[0].d_blob;

                // TODO While the record is only 60 bytes, we are sending 96
                //      redundant bytes before that.  We need to improve the
                //      send chunk logic to *only* send the required data.
                bmqu::BlobPosition                         pos;
                bmqu::BlobUtil::findOffsetSafe(&pos, blob, 96);
                bmqu::BlobObjectProxy<mqbs::QueueOpRecord> record(&blob, pos);
                assert(record.isSet());

                BMQTST_ASSERT_EQ(record->header(),   expectedRecord.header());
                BMQTST_ASSERT_EQ(record->flags(),    expectedRecord.flags());
                BMQTST_ASSERT_EQ(record->queueKey(),
                expectedRecord.queueKey()); BMQTST_ASSERT_EQ(record->appKey(),
                expectedRecord.appKey()); BMQTST_ASSERT_EQ(record->type(),
                expectedRecord.type());
                BMQTST_ASSERT_EQ(record->queueUriRecordOffsetWords(),
                          expectedRecord.queueUriRecordOffsetWords());
                BMQTST_ASSERT_EQ(record->magic(), expectedRecord.magic());

                // Verify ReplicaDataRspnPull
                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                       &message,
                       cit->second->writeCalls()[expectedNumDataChunks].d_blob,
                       bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message.choice().clusterMessage(),
                expectedMessage);
                */
            }
            else {
                // Make sure that replica node does not send chunks to anyone
                // except primary.
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyPrimarySendsReplicaDataRqstPull(
        int                                          partitionId,
        int                                          highestSeqNumNodeId,
        const bmqp_ctrlmsg::PartitionSequenceNumber& highestSeqNum)
    {
        bmqp_ctrlmsg::ClusterMessage      expectedMessage;
        bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRequest =
            expectedMessage.choice()
                .makePartitionMessage()
                .choice()
                .makeReplicaDataRequest();

        replicaDataRequest.partitionId() = partitionId;
        replicaDataRequest.replicaDataType() =
            bmqp_ctrlmsg::ReplicaDataType::E_PULL;
        replicaDataRequest.endSequenceNumber() = highestSeqNum;

        // TODO: set sequence number once add mocked recovery manager to this
        //       test helper class.

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() == highestSeqNumNodeId) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message.choice().clusterMessage(),
                                 expectedMessage);
            }
            else {
                // Make sure that no other node except highestSeqNumNodeId
                // receives replicaDataRequest of type PULL.
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyPrimarySendsReplicaDataRqstPush(
        int                                          partitionId,
        const NodeIdToSeqNumMap&                     destinationReplicas,
        const bmqp_ctrlmsg::PartitionSequenceNumber& selfSeqNum)
    {
        bmqp_ctrlmsg::ClusterMessage      expectedMessage;
        bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRequest =
            expectedMessage.choice()
                .makePartitionMessage()
                .choice()
                .makeReplicaDataRequest();

        replicaDataRequest.replicaDataType() =
            bmqp_ctrlmsg::ReplicaDataType::E_PUSH;
        replicaDataRequest.partitionId()       = partitionId;
        replicaDataRequest.endSequenceNumber() = selfSeqNum;

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (destinationReplicas.find(cit->first->nodeId()) !=
                destinationReplicas.end()) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                replicaDataRequest.beginSequenceNumber() =
                    destinationReplicas.at(cit->first->nodeId());
                BMQTST_ASSERT_EQ(message.choice().clusterMessage(),
                                 expectedMessage);
            }
            else {
                // Make sure that primary node does not end up receiving
                // data chunks or send it to upto date replicas.
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyPrimarySendsFailedReplicaStateRspn(int rogueNodeId,
                                                  int rogueRequestId)
    {
        bmqp_ctrlmsg::ControlMessage failureMessage;
        failureMessage.rId() = rogueRequestId;

        bmqp_ctrlmsg::Status& failureResponse =
            failureMessage.choice().makeStatus();
        failureResponse.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        failureResponse.code()     = mqbi::ClusterErrorCode::e_NOT_REPLICA;
        failureResponse.message()  = "Not a replica";

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() == rogueNodeId) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message, failureMessage);
            }
            else {
                // Make sure that no other node except the rogue node ends up
                // receiving failure replicaStateResponse.
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyReplicaSendsFailedPrimaryStateRspn(int rogueNodeId,
                                                  int rogueRequestId)
    {
        bmqp_ctrlmsg::ControlMessage failureMessage;
        failureMessage.rId() = rogueRequestId;

        bmqp_ctrlmsg::Status& failureResponse =
            failureMessage.choice().makeStatus();
        failureResponse.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        failureResponse.code()     = mqbi::ClusterErrorCode::e_NOT_PRIMARY;
        failureResponse.message()  = "Not a primary";

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() == rogueNodeId) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message, failureMessage);
            }
            else {
                // Make sure that no other node except the rogue node ends up
                // receiving failure primaryStateResponse.
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyReplicaSendsPrimaryStateRqst(
        int                                   partitionId,
        int                                   primaryNodeId,
        bmqp_ctrlmsg::PartitionSequenceNumber seqNum =
            bmqp_ctrlmsg::PartitionSequenceNumber())
    {
        bmqp_ctrlmsg::ClusterMessage       expectedMessage;
        bmqp_ctrlmsg::PrimaryStateRequest& primaryStateRequest =
            expectedMessage.choice()
                .makePartitionMessage()
                .choice()
                .makePrimaryStateRequest();

        primaryStateRequest.partitionId()    = partitionId;
        primaryStateRequest.latestSequenceNumber() = seqNum;

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() == primaryNodeId) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message.choice().clusterMessage(),
                                 expectedMessage);
            }
            else {
                // Make sure that other replica nodes dont receive
                // primaryStateRequest.
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    void verifyReplicaSendsReplicaStateRspn(
        int                                   partitionId,
        int                                   primaryNodeId,
        bmqp_ctrlmsg::PartitionSequenceNumber seqNum =
            bmqp_ctrlmsg::PartitionSequenceNumber())
    {
        bmqp_ctrlmsg::ClusterMessage        expectedMessage;
        bmqp_ctrlmsg::ReplicaStateResponse& replicaStateResponse =
            expectedMessage.choice()
                .makePartitionMessage()
                .choice()
                .makeReplicaStateResponse();

        replicaStateResponse.partitionId()    = partitionId;
        replicaStateResponse.latestSequenceNumber() = seqNum;

        for (TestChannelMapCIter cit = d_cluster_mp->_channels().cbegin();
             cit != d_cluster_mp->_channels().cend();
             ++cit) {
            if (cit->first->nodeId() == primaryNodeId) {
                BMQTST_ASSERT(cit->second->waitFor(1, false));

                bmqp_ctrlmsg::ControlMessage message;
                mqbc::ClusterUtil::extractMessage(
                    &message,
                    cit->second->writeCalls()[0].d_blob,
                    bmqtst::TestHelperUtil::allocator());
                BMQTST_ASSERT_EQ(message.choice().clusterMessage(),
                                 expectedMessage);
            }
            else {
                // Make sure that other replica nodes dont receive
                // primaryStateRequest.
                BMQTST_ASSERT(!cit->second->waitFor(1, false));
            }
        }
    }

    NodeSeqNumPair
    getHighestSeqNumNodeDetails(mqbnet::ClusterNode*      selfNode,
                                const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap)
    {
        // Return the highest sequence number (node, seqNum) pair in the
        // specified 'nodeToSeqNumCtxMap'.  In case of a tie, the specified
        // 'selfNode' is considered the highest.

        mqbnet::ClusterNode*                  highestSeqNumReplica = selfNode;
        bmqp_ctrlmsg::PartitionSequenceNumber highestSeqNum(
            nodeToSeqNumCtxMap.at(selfNode).d_seqNum);
        for (NodeToSeqNumCtxMapCIter cit = nodeToSeqNumCtxMap.cbegin();
             cit != nodeToSeqNumCtxMap.cend();
             ++cit) {
            if (cit->second.d_seqNum > highestSeqNum) {
                highestSeqNumReplica = cit->first;
                highestSeqNum        = cit->second.d_seqNum;
            }
        }
        return NodeSeqNumPair(highestSeqNumReplica, highestSeqNum);
    }

    bsls::TimeInterval nowMonotonicClock() const
    {
        return d_cluster_mp->_scheduler().now();
    }

    bsls::Types::Int64 highResolutionTimer() const
    {
        return d_cluster_mp->_scheduler().now().totalNanoseconds();
    }

    mqbu::StorageKey
    writeQueueCreationRecord(mqbs::DataStoreRecordHandle* handle,
                             mqbs::FileStore*             fs,
                             int                          partitionId)
    {
        // Write a queue creation record to the specified 'fs', and load the
        // record into the specified 'handle'.  Return the queue key.

        bsl::string        uri("bmq://si.amw.bmq.stats/queue0",
                        bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream osstr;

        // Generate queue-key.
        for (size_t j = 0; j < mqbu::StorageKey::e_KEY_LENGTH_BINARY; ++j) {
            osstr << 'x';
        }

        bsl::string      queueKeyStr(osstr.str().data(), osstr.str().length());
        mqbu::StorageKey queueKey(
            mqbu::StorageKey::BinaryRepresentation(),
            queueKeyStr.substr(0, mqbu::StorageKey::e_KEY_LENGTH_BINARY)
                .c_str());

        mqbs::FileStoreTestUtil_Record rec(
            bmqtst::TestHelperUtil::allocator());
        rec.d_recordType  = mqbs::RecordType::e_QUEUE_OP;
        rec.d_queueOpType = mqbs::QueueOpType::e_CREATION;
        rec.d_uri         = uri;
        rec.d_queueKey    = queueKey;
        rec.d_timestamp   = bdlt::EpochUtil::convertToTimeT64(
            bdlt::CurrentTime::utc());

        bmqt::Uri uri_t(rec.d_uri, bmqtst::TestHelperUtil::allocator());
        const int rc = fs->writeQueueCreationRecord(
            handle,
            uri_t,
            rec.d_queueKey,
            mqbs::DataStore::AppInfos(),
            rec.d_timestamp,
            true);  // isNewQueue

        bmqp_ctrlmsg::QueueInfo advisory(bmqtst::TestHelperUtil::allocator());

        advisory.uri() = rec.d_uri;
        queueKey.loadBinary(&advisory.key());
        advisory.partitionId() = partitionId;
        d_cluster_mp->_state()->assignQueue(advisory);

        BSLS_ASSERT_OPT(rc == 0);
        return queueKey;
    }

    void writeMessageRecord(mqbs::FileStore*        fs,
                            const mqbu::StorageKey& queueKey,
                            size_t                  recNum)
    {
        // Write a message record.

        mqbs::DataStoreRecordHandle    handle;
        mqbs::FileStoreTestUtil_Record rec(
            bmqtst::TestHelperUtil::allocator());
        rec.d_recordType = mqbs::RecordType::e_MESSAGE;
        rec.d_queueKey   = queueKey;
        bmqp::MessagePropertiesInfo mpsInfo;

        if (0 == recNum % 2) {
            mpsInfo = bmqp::MessagePropertiesInfo::makeInvalidSchema();
        }

        // crc value
        mqbu::MessageGUIDUtil::generateGUID(&rec.d_guid);
        rec.d_appData_sp.createInplace(bmqtst::TestHelperUtil::allocator(),
                                       d_cluster_mp->bufferFactory(),
                                       bmqtst::TestHelperUtil::allocator());
        bsl::string payloadStr(recNum * 10,
                               'x',
                               bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobUtil::append(rec.d_appData_sp.get(),
                                payloadStr.c_str(),
                                payloadStr.length());

        const unsigned int appDataLen = static_cast<unsigned int>(
            rec.d_appData_sp->length());

        rec.d_msgAttributes = mqbi::StorageMessageAttributes(
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()),
            recNum % mqbs::FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD,
            appDataLen,
            mpsInfo,
            bmqt::CompressionAlgorithmType::e_NONE,
            bsl::numeric_limits<unsigned int>::max() / recNum);

        const int rc = fs->writeMessageRecord(&rec.d_msgAttributes,
                                              &handle,
                                              rec.d_guid,
                                              rec.d_appData_sp,
                                              rec.d_options_sp,
                                              rec.d_queueKey);

        BSLS_ASSERT_OPT(rc == 0);
    }

    void
    initializeRecords(mqbs::DataStoreRecordHandle*           handle,
                      int                                    numRecords,
                      bmqp_ctrlmsg::PartitionSequenceNumber* selfSeqNum = 0)
    {
        // Write one queue creation record and the specified 'numRecords'
        // message records of that queue to the journal and data files at the
        // beginning of a test run, and load the handle of the queue creation
        // record into the specified 'handle'.  Load the resulting partition
        // sequence number into the optionally specified 'selfSeqNum', assuming
        // primary lease Id is 1.

        static const int k_PARTITION_ID = 1;

        const int selfNodeId = d_cluster_mp->_clusterData()
                                   ->membership()
                                   .netCluster()
                                   ->selfNodeId();

        mqbnet::ClusterNode* selfNode = d_cluster_mp->_clusterData()
                                            ->membership()
                                            .netCluster()
                                            ->lookupNode(selfNodeId);

        const mqbcfg::PartitionConfig& partitionCfg =
            d_cluster_mp->_clusterDefinition().partitionConfig();

        mqbs::DataStoreConfig dsCfg;
        dsCfg.setScheduler(&d_cluster_mp->_scheduler())
            .setBufferFactory(&d_cluster_mp->_clusterData()->bufferFactory())
            .setPreallocate(partitionCfg.preallocate())
            .setPrefaultPages(partitionCfg.prefaultPages())
            .setLocation(partitionCfg.location())
            .setArchiveLocation(partitionCfg.archiveLocation())
            .setNodeId(d_cluster_mp->_clusterData()
                           ->membership()
                           .selfNode()
                           ->nodeId())
            .setPartitionId(k_PARTITION_ID)
            .setMaxDataFileSize(partitionCfg.maxDataFileSize())
            .setMaxJournalFileSize(partitionCfg.maxJournalFileSize())
            .setMaxQlistFileSize(partitionCfg.maxQlistFileSize())
            .setMaxArchivedFileSets(partitionCfg.maxArchivedFileSets());

        bdlmt::FixedThreadPool threadPool(1,
                                          100,
                                          bmqtst::TestHelperUtil::allocator());
        threadPool.start();

        mqbs::FileStore fs(
            dsCfg,
            1,
            d_cluster_mp->dispatcher(),
            &d_cluster_mp->netCluster(),
            d_cluster_mp->_clusterData()->stats().getPartitionStats(
                k_PARTITION_ID),
            &d_cluster_mp->_clusterData()->blobSpPool(),
            &d_cluster_mp->_clusterData()->stateSpPool(),
            &threadPool,
            d_cluster_mp->isCSLModeEnabled(),
            d_cluster_mp->isFSMWorkflow(),
            d_cluster_mp->doesFSMwriteQLIST(),
            1,  // replicationFactor
            bmqtst::TestHelperUtil::allocator());

        dynamic_cast<mqbnet::MockCluster&>(d_cluster_mp->netCluster())
            ._setDisableBroadcast(true);
        fs.open();
        // TODO: clean this up since its a hack to set replica node as primary!
        // had to explicitly setActivePrimary for fileStore because of the
        // assert in writeRecords which allows writes only if current node is
        // primary for the fileStore.
        // TODO: set primary to self but also correct it to the right node
        // after writing 'numRecords' records.
        fs.setActivePrimary(selfNode, 1U);

        const mqbu::StorageKey& queueKey =
            writeQueueCreationRecord(handle, &fs, k_PARTITION_ID);
        for (int i = 1; i <= numRecords; i++) {
            writeMessageRecord(&fs, queueKey, i);
        }

        fs.close();
        dynamic_cast<mqbnet::MockCluster&>(d_cluster_mp->netCluster())
            ._setDisableBroadcast(false);

        if (selfSeqNum) {
            selfSeqNum->primaryLeaseId() = 1U;

            // One sync point, one queue creation record, and 'numRecords'
            // message records
            selfSeqNum->sequenceNumber() = 2U + numRecords;
        }
    }

    ~TestHelper() { bmqsys::Time::shutdown(); }
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
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - START STOP "
                                      "STORAGEMANAGER SUCCESSFULLY");

    TestHelper helper;

    mqbs::DataStoreRecordHandle handle;
    helper.initializeRecords(&handle, 1);

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test2_unknownDetectSelfPrimary()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Detect Self node as primary
//  4) Verify the actions as per FSM.
//  5) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - DETECT PRIMARY");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;
    const int        rc             = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     1U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test3_unknownDetectSelfReplica()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Detect Self node as replica
//  4) Verify the actions as per FSM.
//  5) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - DETECT REPLICA");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);
    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();
    const int primaryNodeId = selfNodeId + 1;

    mqbnet::ClusterNode* primaryNode = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(primaryNodeId);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               primaryNode);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     1U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    helper.verifyReplicaSendsPrimaryStateRqst(k_PARTITION_ID, primaryNodeId);
    helper.clearChannels();

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test4_primaryHealingStage1DetectSelfReplica()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to Primary healing stage 1 and then detect self replica.
//  4) Verify the actions as per FSM.
//  5) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - "
                                      "PRIMARY HEALING STAGE 1 DETECTS SELF AS"
                                      " REPLICA");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Apply Detect Self Replica event to Node in primaryHealingStage1.

    const int            primaryNodeId = selfNodeId + 1;
    mqbnet::ClusterNode* primaryNode   = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(primaryNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               primaryNode);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     1U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    helper.verifyReplicaSendsPrimaryStateRqst(k_PARTITION_ID, primaryNodeId);
    helper.clearChannels();

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test5_primaryHealingStage1ReceivesReplicaStateRqst()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure primary in healing stage 1 sends Failure ReplicaStateResponse
//   when it receives a ReplicaStateRequest from any other node.
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to Primary healing stage 1.
//  4) Verify the actions as per FSM.
//  5) Send ReplicaStateRequest to this primary.
//  6) Check that primary sends failure Response to received request.
//  7) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - "
                                      "PRIMARY HEALING STAGE 1 SENDS FAILURE"
                                      " REPLICA STATE RESPONSE");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Receives ReplicaStateRequest from a rogue node.
    static const int             k_ROGUE_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_ROGUE_REQUEST_ID;
    bmqp_ctrlmsg::ReplicaStateRequest& replicaStateRequest =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaStateRequest();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    replicaStateRequest.partitionId()    = k_PARTITION_ID;
    replicaStateRequest.latestSequenceNumber() = seqNum;

    mqbnet::ClusterNode* source = helper.d_cluster_mp->_clusterData()
                                      ->membership()
                                      .netCluster()
                                      ->lookupNode(selfNodeId + 1);

    storageManager.processReplicaStateRequest(message, source);

    helper.verifyPrimarySendsFailedReplicaStateRspn(selfNodeId + 1,
                                                    k_ROGUE_REQUEST_ID);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     1U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test6_primaryHealingStage1ReceivesReplicaStateRspnQuorum()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure primary in healing stage 1 correctly stores ReplicaSeq
//   when it receives a ReplicaStateRspn from a replica node.
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to Primary healing stage 1.
//  4) Verify the actions as per FSM.
//  5) Send ReplicaStateResponse from all replicas to this primary.
//  6) Check that primary stores Replica Sequence number and goes to stg2.
//  7) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - "
                                      "PRIMARY HEALING STAGE 1 RECEIVES"
                                      " REPLICA STATE RESPONSE QUORUM");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    // Receives ReplicaStateResponse from replica nodes.
    BSLS_ASSERT_OPT(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size() ==
                    1);
    static const int             k_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_REQUEST_ID;
    bmqp_ctrlmsg::ReplicaStateResponse& replicaStateResponse =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaStateResponse();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    replicaStateResponse.partitionId()    = k_PARTITION_ID;
    replicaStateResponse.latestSequenceNumber() = seqNum;

    helper.d_cluster_mp->requestManager().processResponse(message);

    message.rId() = k_REQUEST_ID + 1;
    helper.d_cluster_mp->requestManager().processResponse(message);

    message.rId() = k_REQUEST_ID + 2;
    helper.d_cluster_mp->requestManager().processResponse(message);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     4U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG2);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test7_primaryHealingStage1ReceivesPrimaryStateRequestQuorum()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure primary in healing stage 1 correctly stores ReplicaSeq
//   when it receives a PrimaryStateRequest from replica nodes.
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to Primary healing stage 1.
//  4) Verify the actions as per FSM.
//  5) Send PrimaryStateRequest from all replicas to this primary.
//  6) Check that primary stores Replica Sequence number and goes to stg2.
//  7) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - "
                                      "PRIMARY HEALING STAGE 1 RECEIVES"
                                      " PRIMARY STATE REQUEST QUORUM");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    // Receives PrimaryStateRequest from replica nodes.
    BSLS_ASSERT_OPT(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size() ==
                    1);
    mqbnet::ClusterNode* replica1 =
        helper.d_cluster_mp->netCluster().lookupNode(selfNodeId + 1);
    static const int             k_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_REQUEST_ID;
    bmqp_ctrlmsg::PrimaryStateRequest& primaryStateRequest =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makePrimaryStateRequest();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    primaryStateRequest.partitionId()    = k_PARTITION_ID;
    primaryStateRequest.latestSequenceNumber() = seqNum;

    storageManager.processPrimaryStateRequest(message, replica1);

    message.rId() = k_REQUEST_ID + 1;
    mqbnet::ClusterNode* replica2 =
        helper.d_cluster_mp->netCluster().lookupNode(selfNodeId + 2);
    storageManager.processPrimaryStateRequest(message, replica2);

    message.rId() = k_REQUEST_ID + 2;
    mqbnet::ClusterNode* replica3 =
        helper.d_cluster_mp->netCluster().lookupNode(selfNodeId - 1);
    storageManager.processPrimaryStateRequest(message, replica3);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     4U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG2);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test8_primaryHealingStage1ReceivesPrimaryStateRqst()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure primary in healing stage 1 correctly stores ReplicaSeq
//   when it receives PrimaryStateRequest from a replica node.
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to Primary healing stage 1.
//  4) Verify the actions as per FSM.
//  5) Send PrimaryStateRequest to this primary.
//  6) Check that primary stores Replica Sequence number
//  7) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - "
                                      "PRIMARY HEALING STAGE 1 RECEIVES"
                                      " PRIMARY STATE REQUEST");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    BSLS_ASSERT_OPT(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size() ==
                    1);

    // Receives PrimaryStateRequest from a replica node.
    mqbnet::ClusterNode* replicaNode = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(selfNodeId + 1);

    static const int             k_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_REQUEST_ID;
    bmqp_ctrlmsg::PrimaryStateRequest& primaryStateRequest =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makePrimaryStateRequest();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    primaryStateRequest.partitionId()    = k_PARTITION_ID;
    primaryStateRequest.latestSequenceNumber() = seqNum;

    storageManager.processPrimaryStateRequest(message, replicaNode);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     2U);
    BMQTST_ASSERT_EQ(
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).count(replicaNode),
        1U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test9_primaryHealingStage1ReceivesReplicaStateRspnNoQuorum()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure primary in healing stage 1 correctly stores ReplicaSeq
//   when it receives a ReplicaStateRspn from a replica node.
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to Primary healing stage 1.
//  4) Verify the actions as per FSM.
//  5) Send mix of success/failure ReplicaStateResponse to this primary.
//  6) Check that primary stores Replica Sequence number, remains in stg1.
//  7) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - "
                                      "PRIMARY HEALING STAGE 1 RECEIVES"
                                      " REPLICA STATE RESPONSE NO QUORUM");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    // Receives success ReplicaStateResponse from a replica node.
    BSLS_ASSERT_OPT(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size() ==
                    1);
    static const int             k_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_REQUEST_ID;
    bmqp_ctrlmsg::ReplicaStateResponse& replicaStateResponse =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaStateResponse();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    replicaStateResponse.partitionId()    = k_PARTITION_ID;
    replicaStateResponse.latestSequenceNumber() = seqNum;

    helper.d_cluster_mp->requestManager().processResponse(message);

    bmqp_ctrlmsg::ControlMessage failureMsg;
    bmqp_ctrlmsg::Status&        response = failureMsg.choice().makeStatus();
    response.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    response.code()     = mqbi::ClusterErrorCode::e_NOT_REPLICA;
    response.message()  = "Not a replica";

    failureMsg.rId() = 2;
    helper.d_cluster_mp->requestManager().processResponse(failureMsg);

    failureMsg.rId() = 3;
    helper.d_cluster_mp->requestManager().processResponse(failureMsg);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     2U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test10_primaryHealingStage1QuorumSendsReplicaDataRequestPull()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure primary in healing stage 1 achives quorum and then sends
//   ReplicaDataRequestPull to the replica node with highest seqNum.
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to Primary healing stage 1.
//  4) Verify the actions as per FSM.
//  5) Send ReplicaStateResponse from all replicas to this primary.
//  6) Check that primary stores Replica Sequence number and goes to stg2.
//  7) Verify that primary sends ReplicaDataRequestPull to highest seqNum
//     replica node.
//  8) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - "
                                      "PRIMARY HEALING STAGE 1 QUORUM"
                                      " SENDS REPLICA DATA REQUEST PULL");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    // Receives ReplicaStateResponse from a replica node.
    BSLS_ASSERT_OPT(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size() ==
                    1);
    static const int             k_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_REQUEST_ID;
    bmqp_ctrlmsg::ReplicaStateResponse& replicaStateResponse =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaStateResponse();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    replicaStateResponse.partitionId()    = k_PARTITION_ID;
    replicaStateResponse.latestSequenceNumber() = seqNum;

    helper.d_cluster_mp->requestManager().processResponse(message);

    message.rId()                         = k_REQUEST_ID + 1;
    seqNum.sequenceNumber()               = 7U;
    seqNum.primaryLeaseId()               = 1U;
    replicaStateResponse.latestSequenceNumber() = seqNum;
    helper.d_cluster_mp->requestManager().processResponse(message);

    message.rId()                         = k_REQUEST_ID + 2;
    seqNum.sequenceNumber()               = 3U;
    seqNum.primaryLeaseId()               = 1U;
    replicaStateResponse.latestSequenceNumber() = seqNum;
    helper.d_cluster_mp->requestManager().processResponse(message);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     4U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG2);

    const NodeSeqNumPair& highestSeqNumNode =
        helper.getHighestSeqNumNodeDetails(
            selfNode,
            storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID));

    BMQTST_ASSERT_NE(highestSeqNumNode.first, selfNode);
    BMQTST_ASSERT_EQ(highestSeqNumNode.second.sequenceNumber(), 7U);

    helper.verifyPrimarySendsReplicaDataRqstPull(
        k_PARTITION_ID,
        highestSeqNumNode.first->nodeId(),
        highestSeqNumNode.second);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test11_primaryHealingStage2DetectSelfReplica()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to Primary healing stage 2 and then detect self replica.
//  4) Verify the actions as per FSM.
//  5) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - "
                                      "PRIMARY HEALING STAGE 2 DETECTS SELF AS"
                                      " REPLICA");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Receives ReplicaStateResponse from replica nodes.
    static const int             k_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_REQUEST_ID;
    bmqp_ctrlmsg::ReplicaStateResponse& replicaStateResponse =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaStateResponse();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    replicaStateResponse.partitionId()    = k_PARTITION_ID;
    replicaStateResponse.latestSequenceNumber() = seqNum;

    helper.d_cluster_mp->requestManager().processResponse(message);

    message.rId()                         = k_REQUEST_ID + 1;
    seqNum.sequenceNumber()               = 7U;
    seqNum.primaryLeaseId()               = 1U;
    replicaStateResponse.latestSequenceNumber() = seqNum;
    helper.d_cluster_mp->requestManager().processResponse(message);

    message.rId()                         = k_REQUEST_ID + 2;
    seqNum.sequenceNumber()               = 3U;
    seqNum.primaryLeaseId()               = 1U;
    replicaStateResponse.latestSequenceNumber() = seqNum;
    helper.d_cluster_mp->requestManager().processResponse(message);

    BSLS_ASSERT_OPT(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size() ==
                    4U);
    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG2);

    const NodeSeqNumPair& highestSeqNumNode =
        helper.getHighestSeqNumNodeDetails(
            selfNode,
            storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID));

    BSLS_ASSERT_OPT(highestSeqNumNode.first != selfNode);
    BSLS_ASSERT_OPT(highestSeqNumNode.second.sequenceNumber() == 7U);

    helper.verifyPrimarySendsReplicaDataRqstPull(
        k_PARTITION_ID,
        highestSeqNumNode.first->nodeId(),
        highestSeqNumNode.second);
    helper.clearChannels();

    // Apply Detect Self Replica event to Node in primaryHealingStage2.
    const int            primaryNodeId = selfNodeId + 1;
    mqbnet::ClusterNode* primaryNode   = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(primaryNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               primaryNode);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     1U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    helper.verifyReplicaSendsPrimaryStateRqst(k_PARTITION_ID, primaryNodeId);
    helper.clearChannels();

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test12_replicaHealingDetectSelfPrimary()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to healing Replica and then detect self primary.
//  4) Verify the actions as per FSM.
//  5) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "BREATHING TEST - "
        "HEALING REPLICA DETECTS SELF AS PRIMARY");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();
    const int primaryNodeId = selfNodeId + 1;

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    mqbnet::ClusterNode* primaryNode = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(primaryNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               primaryNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    helper.verifyReplicaSendsPrimaryStateRqst(k_PARTITION_ID, primaryNodeId);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Apply Detect Self Primary event to Self Node.

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               selfNode);

    BMQTST_ASSERT_EQ(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size(),
                     1U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID, selfNodeId);
    helper.clearChannels();

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test13_replicaHealingReceivesReplicaStateRqst()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to healing Replica.
//  4) Send ReplicaStateRqst to this Replica.
//  5) Check that Replica sends ReplicaStateRspn, stores primarySeqNum.
//  6) Verify the actions as per FSM.
//  7) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "BREATHING TEST - "
        "HEALING REPLICA RECEIVES REPLICA STATE REQUEST");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();
    const int primaryNodeId = selfNodeId + 1;

    mqbnet::ClusterNode* primaryNode = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(primaryNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               primaryNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    helper.verifyReplicaSendsPrimaryStateRqst(k_PARTITION_ID, primaryNodeId);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Receives ReplicaStateRequest from the primary node.
    static const int             k_PRIMARY_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_PRIMARY_REQUEST_ID;
    bmqp_ctrlmsg::ReplicaStateRequest& replicaStateRequest =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaStateRequest();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    replicaStateRequest.partitionId()    = k_PARTITION_ID;
    replicaStateRequest.latestSequenceNumber() = seqNum;

    storageManager.processReplicaStateRequest(message, primaryNode);

    helper.verifyReplicaSendsReplicaStateRspn(k_PARTITION_ID, primaryNodeId);

    BMQTST_ASSERT_EQ(nodeToSeqNumCtxMap.size(), 2U);
    BMQTST_ASSERT_EQ(nodeToSeqNumCtxMap.at(primaryNode).d_seqNum, seqNum);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test14_replicaHealingReceivesPrimaryStateRspn()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to healing Replica.
//  4) Send PrimaryStateRspn to this Replica.
//  5) Check that Replica stores primarySeqNum.
//  6) Verify the actions as per FSM.
//  7) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "BREATHING TEST - "
        "HEALING REPLICA RECEIVES PRIMARY STATE RESPONSE");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();
    const int primaryNodeId = selfNodeId + 1;

    mqbnet::ClusterNode* primaryNode = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(primaryNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               primaryNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    helper.verifyReplicaSendsPrimaryStateRqst(k_PARTITION_ID, primaryNodeId);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Receives PrimaryStateResponse from the primary node.
    static const int             k_PRIMARY_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_PRIMARY_REQUEST_ID;
    bmqp_ctrlmsg::PrimaryStateResponse& primaryStateResponse =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makePrimaryStateResponse();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    primaryStateResponse.partitionId()    = k_PARTITION_ID;
    primaryStateResponse.latestSequenceNumber() = seqNum;

    helper.d_cluster_mp->requestManager().processResponse(message);

    BMQTST_ASSERT_EQ(nodeToSeqNumCtxMap.size(), 2U);
    BMQTST_ASSERT_EQ(nodeToSeqNumCtxMap.at(primaryNode).d_seqNum, seqNum);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test15_replicaHealingReceivesFailedPrimaryStateRspn()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to healing Replica.
//  4) Send failed PrimaryStateRspn to this Replica.
//  5) Check that Replica does not store primarySeqNum.
//  6) Verify the actions as per FSM.
//  7) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "BREATHING TEST - "
        "HEALING REPLICA RECEIVES FAILED PRIMARY STATE RESPONSE");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();
    const int primaryNodeId = selfNodeId + 1;

    mqbnet::ClusterNode* primaryNode = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(primaryNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               primaryNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    helper.verifyReplicaSendsPrimaryStateRqst(k_PARTITION_ID, primaryNodeId);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Receives Failed PrimaryStateResponse from the primary node.
    static const int             k_PRIMARY_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage failureMsg;
    bmqp_ctrlmsg::Status&        response = failureMsg.choice().makeStatus();
    response.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    response.code()     = mqbi::ClusterErrorCode::e_NOT_PRIMARY;
    response.message()  = "Not a primary";
    failureMsg.rId()    = k_PRIMARY_REQUEST_ID;

    helper.d_cluster_mp->requestManager().processResponse(failureMsg);

    BMQTST_ASSERT_EQ(nodeToSeqNumCtxMap.size(), 1U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test16_replicaHealingReceivesPrimaryStateRqst()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper building and starting of the StorageManager
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to healing Replica.
//  4) Send PrimaryStateRqst to this Replica.
//  5) Check that Replica sends failed PrimaryStateRspn.
//  6) Verify the actions as per FSM.
//  7) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "BREATHING TEST - "
        "HEALING REPLICA RECEIVES PRIMARY STATE REQUEST");

    TestHelper helper;

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int k_PARTITION_ID = 1;

    int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();
    const int primaryNodeId = selfNodeId + 1;

    mqbnet::ClusterNode* primaryNode = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(primaryNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               primaryNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    helper.verifyReplicaSendsPrimaryStateRqst(k_PARTITION_ID, primaryNodeId);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Receives PrimaryStateRequest from a rogue node.
    const int            rogueNodeId = selfNodeId + 2;
    mqbnet::ClusterNode* rogueNode   = helper.d_cluster_mp->_clusterData()
                                         ->membership()
                                         .netCluster()
                                         ->lookupNode(rogueNodeId);
    static const int             k_ROGUE_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_ROGUE_REQUEST_ID;
    bmqp_ctrlmsg::PrimaryStateRequest& primaryStateRequest =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makePrimaryStateRequest();

    bmqp_ctrlmsg::PartitionSequenceNumber seqNum;
    seqNum.sequenceNumber() = 1U;
    seqNum.primaryLeaseId() = 1U;

    primaryStateRequest.partitionId()    = k_PARTITION_ID;
    primaryStateRequest.latestSequenceNumber() = seqNum;

    storageManager.processPrimaryStateRequest(message, rogueNode);

    helper.verifyReplicaSendsFailedPrimaryStateRspn(rogueNodeId,
                                                    k_ROGUE_REQUEST_ID);

    BMQTST_ASSERT_EQ(nodeToSeqNumCtxMap.size(), 1U);
    BMQTST_ASSERT_EQ(nodeToSeqNumCtxMap.count(rogueNode), 0U);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test17_replicaHealingReceivesReplicaDataRqstPull()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure replica node sends data chunks and response upon receiving
//   ReplicaDataRqstPull.
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to healing Replica.
//  4) Send ReplicaStateRqst to this Replica.
//  5) Check that Replica sends ReplicaStateRspn, stores primarySeqNum.
//  6) Send ReplicaDataRqstPull
//  7) Check that Replica sends data chunks.
//  8) Check that Replica sends ReplicaDataRspnPull.
//  9) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "BREATHING TEST - "
        "HEALING REPLICA RECEIVES REPLICA DATA REQUEST PULL");

    // TODO: debug on why the global allocator check fails for fileStore
    // allocating some memory through default allocator.

    TestHelper helper;

    bmqp_ctrlmsg::PartitionSequenceNumber selfSeqNum;
    mqbs::DataStoreRecordHandle           handle;
    helper.initializeRecords(&handle, 1, &selfSeqNum);

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    static const int k_PARTITION_ID = 1;

    bmqu::MemOutStream errorDescription;

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    const int primaryNodeId = selfNodeId + 1;

    int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);
    storageManager.initializeQueueKeyInfoMap(*helper.d_cluster_mp->_state());

    mqbs::FileStore& fs = storageManager.fileStore(k_PARTITION_ID);
    fs.setIgnoreCrc32c(true);

    mqbnet::ClusterNode* primaryNode = helper.d_cluster_mp->_clusterData()
                                           ->membership()
                                           .netCluster()
                                           ->lookupNode(primaryNodeId);
    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               1,  // primaryLeaseId
                               primaryNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    helper.verifyReplicaSendsPrimaryStateRqst(k_PARTITION_ID,
                                              primaryNodeId,
                                              selfSeqNum);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Receives ReplicaStateRequest from the primary node.
    static const int             k_PRIMARY_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_PRIMARY_REQUEST_ID;
    bmqp_ctrlmsg::ReplicaStateRequest& replicaStateRequest =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaStateRequest();

    bmqp_ctrlmsg::PartitionSequenceNumber k_PRIMARY_SEQ_NUM;
    k_PRIMARY_SEQ_NUM.sequenceNumber() = 1U;
    k_PRIMARY_SEQ_NUM.primaryLeaseId() = 1U;

    replicaStateRequest.partitionId()    = k_PARTITION_ID;
    replicaStateRequest.latestSequenceNumber() = k_PRIMARY_SEQ_NUM;

    storageManager.processReplicaStateRequest(message, primaryNode);

    helper.verifyReplicaSendsReplicaStateRspn(k_PARTITION_ID,
                                              primaryNodeId,
                                              selfSeqNum);
    helper.clearChannels();

    BMQTST_ASSERT_EQ(nodeToSeqNumCtxMap.size(), 2U);
    BMQTST_ASSERT_EQ(nodeToSeqNumCtxMap.at(primaryNode).d_seqNum,
                     k_PRIMARY_SEQ_NUM);
    BMQTST_ASSERT_EQ(storageManager.partitionHealthState(k_PARTITION_ID),
                     mqbc::PartitionFSM::State::e_REPLICA_HEALING);

    // Receives ReplicaDataRequestPULL from the primary node.
    bmqp_ctrlmsg::ControlMessage dataMessage;
    dataMessage.rId() = k_PRIMARY_REQUEST_ID + 1;
    bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRequest =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaDataRequest();

    replicaDataRequest.replicaDataType() =
        bmqp_ctrlmsg::ReplicaDataType::E_PULL;
    replicaDataRequest.partitionId()         = k_PARTITION_ID;
    replicaDataRequest.beginSequenceNumber() = k_PRIMARY_SEQ_NUM;
    replicaDataRequest.endSequenceNumber()   = selfSeqNum;

    storageManager.processReplicaDataRequest(message, primaryNode);

    BSLS_ASSERT_OPT(fs.primaryLeaseId() == selfSeqNum.primaryLeaseId());
    BSLS_ASSERT_OPT(fs.sequenceNumber() == selfSeqNum.sequenceNumber());

    // Verify that Replica sends data chunks followed by ReplicaDataRspnPull.
    helper.verifyReplicaSendsDataChunksAndReplicaDataRspnPull(
        primaryNodeId,
        k_PRIMARY_SEQ_NUM,
        selfSeqNum);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test18_primaryHealingStage1SelfHighestSendsDataChunks()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure primary node sends data chunks to replicas if it has the
//   highest sequence number.
//
// Plan:
//  1) Create a StorageManager on the stack
//  2) Invoke start.
//  3) Transition to Primary healing stage 1 and collect sequence numbers.
//  4) Receive ReplicaStateRspns from replica nodes.
//  5) Transition to Primary healing stage 2, and self has highest seq num.
//  6) Verify that self sends ReplicaDataRqstPush to outdated replicas.
//  7) Verify that self sends data chunks to outdated replicas.
//  8) Invoke stop.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST - "
                                      "PRIMARY HEALING STAGE 1 SELF HIGHEST"
                                      " SENDS DATA CHUNKS");

    TestHelper helper;

    bmqp_ctrlmsg::PartitionSequenceNumber    selfSeqNum;
    bsl::vector<mqbs::DataStoreRecordHandle> handles;
    handles.resize(1);
    // TODO Should we also populate message record handles into 'handles'?
    helper.initializeRecords(&handles[0], 10, &selfSeqNum);

    mqbc::StorageManager storageManager(
        helper.d_cluster_mp->_clusterDefinition(),
        helper.d_cluster_mp.get(),
        helper.d_cluster_mp->_clusterData(),
        helper.d_cluster_mp->_state(),
        helper.d_cluster_mp->_clusterData()->domainFactory(),
        helper.d_cluster_mp->dispatcher(),
        k_WATCHDOG_TIMEOUT_DURATION,
        mockOnRecoveryStatus,
        mockOnPartitionPrimaryStatus,
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream errorDescription;

    static const int          k_PARTITION_ID     = 1;
    static const unsigned int k_PRIMARY_LEASE_ID = 1U;

    const int rc = storageManager.start(errorDescription);
    BSLS_ASSERT_OPT(rc == 0);
    storageManager.initializeQueueKeyInfoMap(*helper.d_cluster_mp->_state());

    mqbs::FileStore& fs = storageManager.fileStore(k_PARTITION_ID);
    fs.setIgnoreCrc32c(true);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_UNKNOWN);

    const int selfNodeId = helper.d_cluster_mp->_clusterData()
                               ->membership()
                               .netCluster()
                               ->selfNodeId();

    mqbnet::ClusterNode* selfNode = helper.d_cluster_mp->_clusterData()
                                        ->membership()
                                        .netCluster()
                                        ->lookupNode(selfNodeId);

    helper.setPartitionPrimary(&storageManager,
                               k_PARTITION_ID,
                               k_PRIMARY_LEASE_ID,  // primaryLeaseId
                               selfNode);

    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG1);

    ReqIdToNodeIdMap reqIdToNodeIdMap;
    helper.verifyPrimarySendsReplicaStateRqst(k_PARTITION_ID,
                                              selfNodeId,
                                              selfSeqNum,
                                              &reqIdToNodeIdMap);
    helper.clearChannels();

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID);

    BSLS_ASSERT_OPT(nodeToSeqNumCtxMap.size() == 1);

    // Receives ReplicaStateResponse from replica nodes.
    static const int             k_REQUEST_ID = 1;
    bmqp_ctrlmsg::ControlMessage message;
    message.rId() = k_REQUEST_ID;
    bmqp_ctrlmsg::ReplicaStateResponse& replicaStateResponse =
        message.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaStateResponse();

    bmqp_ctrlmsg::PartitionSequenceNumber k_REPLICA_SEQ_NUM_1;
    k_REPLICA_SEQ_NUM_1.primaryLeaseId()  = k_PRIMARY_LEASE_ID;
    k_REPLICA_SEQ_NUM_1.sequenceNumber()  = 3U;
    replicaStateResponse.partitionId()    = k_PARTITION_ID;
    replicaStateResponse.latestSequenceNumber() = k_REPLICA_SEQ_NUM_1;

    helper.d_cluster_mp->requestManager().processResponse(message);

    bmqp_ctrlmsg::PartitionSequenceNumber k_REPLICA_SEQ_NUM_2;
    k_REPLICA_SEQ_NUM_2.primaryLeaseId()  = k_PRIMARY_LEASE_ID;
    k_REPLICA_SEQ_NUM_2.sequenceNumber()  = 5U;
    message.rId()                         = k_REQUEST_ID + 1;
    replicaStateResponse.latestSequenceNumber() = k_REPLICA_SEQ_NUM_2;
    helper.d_cluster_mp->requestManager().processResponse(message);

    message.rId()                         = k_REQUEST_ID + 2;
    replicaStateResponse.latestSequenceNumber() = selfSeqNum;
    helper.d_cluster_mp->requestManager().processResponse(message);

    BSLS_ASSERT_OPT(storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID).size() ==
                    4U);
    BSLS_ASSERT_OPT(storageManager.partitionHealthState(k_PARTITION_ID) ==
                    mqbc::PartitionFSM::State::e_PRIMARY_HEALING_STG2);

    BSLS_ASSERT_OPT(fs.primaryLeaseId() == selfSeqNum.primaryLeaseId());
    BSLS_ASSERT_OPT(fs.sequenceNumber() == selfSeqNum.sequenceNumber());

    const NodeSeqNumPair& highestSeqNumNode =
        helper.getHighestSeqNumNodeDetails(
            selfNode,
            storageManager.nodeToSeqNumCtxMap(k_PARTITION_ID));

    BSLS_ASSERT_OPT(highestSeqNumNode.first->nodeId() == selfNodeId);
    BSLS_ASSERT_OPT(highestSeqNumNode.second.sequenceNumber() == 12U);

    NodeIdToSeqNumMap destinationReplicas;
    destinationReplicas.insert(
        bsl::make_pair(reqIdToNodeIdMap.at(1), k_REPLICA_SEQ_NUM_1));
    destinationReplicas.insert(
        bsl::make_pair(reqIdToNodeIdMap.at(2), k_REPLICA_SEQ_NUM_2));
    destinationReplicas.insert(
        bsl::make_pair(reqIdToNodeIdMap.at(3), selfSeqNum));

    // Verify that self sends ReplicaDataRqstPush and data chunks to outdated
    // replicas.
    helper.verifyPrimarySendsReplicaDataRqstPush(k_PARTITION_ID,
                                                 destinationReplicas,
                                                 selfSeqNum);

    helper.verifyPrimarySendsDataChunks(k_PARTITION_ID,
                                        selfSeqNum,
                                        destinationReplicas);

    // Stop the cluster
    storageManager.stop();
    helper.d_cluster_mp->stop();
}

static void test19_fileSizesHardLimits()
// ------------------------------------------------------------------------
// FILE SIZES HARD LIMITS
//
// Concerns:
//   Ensure StorageManager is able to early-detect overflow in the file sizes
//   configuration and gracefully return an error code on start.
//
// Plan:
//   Try to start mqbc::StorageManager with different partition configurations
//   and check that it respects hard limits on the file sizes.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FILE SIZES HARD LIMITS");

    TestHelper helper;

    mqbs::DataStoreRecordHandle handle;
    helper.initializeRecords(&handle, 1);

    struct LocalFuncs {
        static void testFileSizes(int                 line,
                                  TestHelper&         helper,
                                  bsls::Types::Uint64 dataFileSize,
                                  bsls::Types::Uint64 journalFileSize,
                                  bsls::Types::Uint64 qlistFileSize,
                                  bool                expectFailure)
        {
            // Make a copy to allow modification
            mqbcfg::ClusterDefinition clusterDef =
                helper.d_cluster_mp->_clusterDefinition();
            clusterDef.partitionConfig().maxDataFileSize() = dataFileSize;
            clusterDef.partitionConfig().maxJournalFileSize() =
                journalFileSize;
            clusterDef.partitionConfig().maxQlistFileSize() = qlistFileSize;
            clusterDef.partitionConfig().maxCSLFileSize()   = qlistFileSize;

            mqbc::StorageManager storageManager(
                clusterDef,
                helper.d_cluster_mp.get(),
                helper.d_cluster_mp->_clusterData(),
                helper.d_cluster_mp->_state(),
                helper.d_cluster_mp->_clusterData()->domainFactory(),
                helper.d_cluster_mp->dispatcher(),
                k_WATCHDOG_TIMEOUT_DURATION,
                mockOnRecoveryStatus,
                mockOnPartitionPrimaryStatus,
                bmqtst::TestHelperUtil::allocator());

            bmqu::MemOutStream errorDescription(
                bmqtst::TestHelperUtil::allocator());
            const int rc = storageManager.start(errorDescription);
            if (rc == 0) {
                storageManager.stop();
            }
            BMQTST_ASSERT_EQ_D("line: " << line << ", expected failure: "
                                        << expectFailure << ", rc: " << rc,
                               expectFailure,
                               (rc != 0));
        }
    };

    LocalFuncs::testFileSizes(L_, helper, 1000ULL, 1000ULL, 1000ULL, false);
    LocalFuncs::testFileSizes(
        L_,
        helper,
        mqbs::FileStoreProtocol::k_MAX_DATA_FILE_SIZE_HARD,
        mqbs::FileStoreProtocol::k_MAX_JOURNAL_FILE_SIZE_HARD,
        mqbs::FileStoreProtocol::k_MAX_QLIST_FILE_SIZE_HARD,
        false);
    LocalFuncs::testFileSizes(
        L_,
        helper,
        mqbs::FileStoreProtocol::k_MAX_DATA_FILE_SIZE_HARD + 1,
        mqbs::FileStoreProtocol::k_MAX_JOURNAL_FILE_SIZE_HARD,
        mqbs::FileStoreProtocol::k_MAX_QLIST_FILE_SIZE_HARD,
        true);
    LocalFuncs::testFileSizes(
        L_,
        helper,
        mqbs::FileStoreProtocol::k_MAX_DATA_FILE_SIZE_HARD,
        mqbs::FileStoreProtocol::k_MAX_JOURNAL_FILE_SIZE_HARD + 1,
        mqbs::FileStoreProtocol::k_MAX_QLIST_FILE_SIZE_HARD,
        true);
    LocalFuncs::testFileSizes(
        L_,
        helper,
        mqbs::FileStoreProtocol::k_MAX_DATA_FILE_SIZE_HARD,
        mqbs::FileStoreProtocol::k_MAX_JOURNAL_FILE_SIZE_HARD,
        mqbs::FileStoreProtocol::k_MAX_QLIST_FILE_SIZE_HARD + 1,
        true);
    LocalFuncs::testFileSizes(
        L_,
        helper,
        mqbs::FileStoreProtocol::k_MAX_DATA_FILE_SIZE_HARD << 8,
        mqbs::FileStoreProtocol::k_MAX_JOURNAL_FILE_SIZE_HARD << 8,
        mqbs::FileStoreProtocol::k_MAX_QLIST_FILE_SIZE_HARD << 8,
        true);

    // Stop the cluster
    helper.d_cluster_mp->stop();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());
    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
        // TODO: overview the removed tests or remove this comment
        //      - test23_primaryHealingStage2SendsReplicaDataRqstPushDrop();
        //      - test22_replicaHealingDetectSelfPrimary();
        //      - test21_replicaHealingReceivesReplicaDataRqstDrop();
        //      - test20_replicaHealingReceivesReplicaDataRqstPush();
        //      - test19_primaryHealedSendsDataChunks();
    case 19: test19_fileSizesHardLimits(); break;
    case 18: test18_primaryHealingStage1SelfHighestSendsDataChunks(); break;
    case 17: test17_replicaHealingReceivesReplicaDataRqstPull(); break;
    case 16: test16_replicaHealingReceivesPrimaryStateRqst(); break;
    case 15: test15_replicaHealingReceivesFailedPrimaryStateRspn(); break;
    case 14: test14_replicaHealingReceivesPrimaryStateRspn(); break;
    case 13: test13_replicaHealingReceivesReplicaStateRqst(); break;
    case 12: test12_replicaHealingDetectSelfPrimary(); break;
    case 11: test11_primaryHealingStage2DetectSelfReplica(); break;
    case 10:
        test10_primaryHealingStage1QuorumSendsReplicaDataRequestPull();
        break;
    case 9:
        test9_primaryHealingStage1ReceivesReplicaStateRspnNoQuorum();
        break;
    case 8: test8_primaryHealingStage1ReceivesPrimaryStateRqst(); break;
    case 7:
        test7_primaryHealingStage1ReceivesPrimaryStateRequestQuorum();
        break;
    case 6: test6_primaryHealingStage1ReceivesReplicaStateRspnQuorum(); break;
    case 5: test5_primaryHealingStage1ReceivesReplicaStateRqst(); break;
    case 4: test4_primaryHealingStage1DetectSelfReplica(); break;
    case 3: test3_unknownDetectSelfReplica(); break;
    case 2: test2_unknownDetectSelfPrimary(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();
    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
    // Can't ensure no default memory is allocated because
    // 'bdlmt::EventSchedulerTestTimeSource' inside 'mqbmock::Cluster' uses
    // the default allocator in its constructor.
}
