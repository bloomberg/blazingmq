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

// mqbs_filestore.t.cpp                                               -*-C++-*-
#include <mqbs_filestore.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_storage.h>
#include <mqbmock_dispatcher.h>
#include <mqbnet_mockcluster.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreset.h>
#include <mqbs_filestoretestutil.h>
#include <mqbstat_clusterstats.h>
#include <mqbu_messageguidutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_blobpoolutil.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>
#include <bmqu_printutil.h>

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bdlb_random.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_fixedthreadpool.h>
#include <bdlpcre_regex.h>
#include <bdls_filesystemutil.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_iostream.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_platform.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// CONSTANTS
const size_t k_SIZEOF_HEADERS_DATA_FILE = sizeof(mqbs::FileHeader) +
                                          sizeof(mqbs::DataFileHeader);
const size_t k_SIZEOF_HEADERS_QLIST_FILE = sizeof(mqbs::FileHeader) +
                                           sizeof(mqbs::QlistFileHeader);
const size_t k_SIZEOF_HEADERS_JOURNAL_FILE = sizeof(mqbs::FileHeader) +
                                             sizeof(mqbs::JournalFileHeader);

const int k_NODE_ID = 12345;

// ALIASES
typedef mqbs::FileStoreTestUtil_Record                 Record;
typedef mqbs::DataStore::AppInfos                      AppInfos;
typedef mqbs::FileStore::SyncPointOffsetPairs          SyncPointOffsetPairs;
typedef bsl::pair<mqbs::DataStoreRecordHandle, Record> HandleRecordPair;

// FUNCTIONS

void recoveredQueuesCb(
    int                                           partitionId,
    const mqbs::DataStoreConfig::QueueKeyInfoMap& queueKeyInfoMap)
{
    static_cast<void>(partitionId);
    static_cast<void>(queueKeyInfoMap);
}

// CLASSES

// ===================
// struct RecordWriter
// ===================

class RecordWriter {
  private:
    // PRIVATE TYPES
    class MessageState {
      public:
        // PUBLIC DATA
        bsl::vector<bsl::shared_ptr<Record> > d_records;

        bool d_confirmed;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(MessageState,
                                       bslma::UsesBslmaAllocator);

        // CREATORS
        explicit MessageState(bslma::Allocator* allocator = 0)
        : d_records(allocator)
        , d_confirmed(false)
        {
            // NOTHING
        }
    };

    class QueueState {
      public:
        // PUBLIC DATA
        bslma::Allocator* d_allocator_p;

        bsl::string d_uri;

        mqbu::StorageKey d_queueKey;

        bsl::vector<bsl::shared_ptr<MessageState> > d_messages;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(QueueState, bslma::UsesBslmaAllocator);

        // CREATORS
        explicit QueueState(bsl::string_view  uri,
                            mqbu::StorageKey  queueKey,
                            bslma::Allocator* allocator = 0)
        : d_allocator_p(bslma::Default::allocator(allocator))
        , d_uri(uri, d_allocator_p)
        , d_queueKey(queueKey)
        , d_messages(d_allocator_p)
        {
            // NOTHING
        }
    };

    // DATA
    bslma::Allocator* d_allocator_p;

    mqbs::FileStore* d_fs_p;

    bdlbb::PooledBlobBufferFactory d_bufferFactory;

    bsl::vector<bsl::shared_ptr<QueueState> > d_queues;

    bsl::vector<size_t> d_openedQueues;
    bsl::vector<size_t> d_closedQueues;

    mqbu::StorageKey generateQueueKey(size_t queueIndex) const
    {
        // This generator works with assumption that the binary size of a
        // StorageKey is 5
        BSLS_ASSERT_SAFE(5 == mqbu::StorageKey::e_KEY_LENGTH_BINARY);

        char          buff[mqbu::StorageKey::e_KEY_LENGTH_BINARY];
        bsl::uint32_t numRecords = queueIndex;

        // Fill 4 bytes with a number of queues
        bsl::memcpy(buff, &numRecords, sizeof(bsl::uint32_t));

        // Fill the remaining byte with random value
        buff[4] = rand() % 256;

        return mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                buff);
    }

    bool writeOpenQueueRecord(bsl::shared_ptr<QueueState>& qstate)
    {
        bsl::shared_ptr<Record> rec;
        rec.createInplace(d_allocator_p);
        rec->d_recordType  = mqbs::RecordType::e_QUEUE_OP;
        rec->d_queueOpType = mqbs::QueueOpType::e_CREATION;
        rec->d_uri         = qstate->d_uri;
        rec->d_queueKey    = qstate->d_queueKey;
        rec->d_timestamp   = bdlt::EpochUtil::convertToTimeT64(
            bdlt::CurrentTime::utc());

        bmqt::Uri uri(qstate->d_uri, d_allocator_p);

        mqbs::DataStoreRecordHandle handle;
        const int rc = d_fs_p->writeQueueCreationRecord(&handle,
                                                        uri,
                                                        rec->d_queueKey,
                                                        AppInfos(),
                                                        rec->d_timestamp,
                                                        true);  // isNewQueue

        if (0 != rc) {
            bsl::cout << "Error writing QueueCreationRecord, rc: " << rc
                      << bsl::endl;
            return false;  // RETURN
        }
        return true;
    }

    bool writeQueueDeletionRecord(bsl::shared_ptr<QueueState>& qstate)
    {
        bsl::shared_ptr<Record> rec;
        rec.createInplace(d_allocator_p);
        rec->d_recordType  = mqbs::RecordType::e_QUEUE_OP;
        rec->d_queueOpType = mqbs::QueueOpType::e_DELETION;
        rec->d_queueKey    = qstate->d_queueKey;
        rec->d_timestamp   = bdlt::EpochUtil::convertToTimeT64(
            bdlt::CurrentTime::utc());

        mqbs::DataStoreRecordHandle handle;
        const int rc = d_fs_p->writeQueueDeletionRecord(&handle,
                                                        rec->d_queueKey,
                                                        mqbu::StorageKey(),
                                                        rec->d_timestamp);
        if (0 != rc) {
            bsl::cout << "Error writing QueueDeletionRecord, rc: " << rc
                      << bsl::endl;
            return false;  // RETURN
        }
        qstate->d_messages.clear();
        return true;
    }

    bool writePurgeRecord(bsl::shared_ptr<QueueState>& qstate)
    {
        bsl::shared_ptr<Record> rec;
        rec.createInplace(d_allocator_p);
        rec->d_recordType  = mqbs::RecordType::e_QUEUE_OP;
        rec->d_queueOpType = mqbs::QueueOpType::e_PURGE;
        rec->d_queueKey    = qstate->d_queueKey;
        rec->d_timestamp   = bdlt::EpochUtil::convertToTimeT64(
            bdlt::CurrentTime::utc());

        mqbs::DataStoreRecordHandle handle;
        const int                   rc = d_fs_p->writeQueuePurgeRecord(
            &handle,
            rec->d_queueKey,
            mqbu::StorageKey(),
            rec->d_timestamp,
            mqbs::DataStoreRecordHandle());
        if (0 != rc) {
            bsl::cout << "Error writing QueuePurgeRecord, rc: " << rc
                      << bsl::endl;
            return false;  // RETURN
        }
        qstate->d_messages.clear();
        return true;
    }

    bool writeMessageRecord(bsl::shared_ptr<QueueState>& qstate)
    {
        size_t randVariance = qstate->d_messages.size() + 1u;

        bmqp::MessagePropertiesInfo messagePropertiesInfo =
            (0 == randVariance % 2)
                ? bmqp::MessagePropertiesInfo::makeNoSchema()
                : bmqp::MessagePropertiesInfo();

        size_t appDataLen = 700 + (randVariance % 5) * 10;

        bsl::shared_ptr<Record> rec;
        rec.createInplace(d_allocator_p);
        rec->d_recordType = mqbs::RecordType::e_MESSAGE;
        rec->d_queueKey   = qstate->d_queueKey;

        rec->d_msgAttributes = mqbi::StorageMessageAttributes(
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()),
            randVariance % mqbs::FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD,
            appDataLen,
            messagePropertiesInfo,
            bmqt::CompressionAlgorithmType::e_NONE,
            bsl::numeric_limits<unsigned int>::max() / randVariance);
        // crc value
        mqbu::MessageGUIDUtil::generateGUID(&rec->d_guid);
        rec->d_appData_sp.createInplace(d_allocator_p,
                                        &d_bufferFactory,
                                        d_allocator_p);
        bsl::string payloadStr(appDataLen, 'x', d_allocator_p);
        bdlbb::BlobUtil::append(rec->d_appData_sp.get(),
                                payloadStr.c_str(),
                                payloadStr.length());

        mqbs::DataStoreRecordHandle handle;
        const int rc = d_fs_p->writeMessageRecord(&rec->d_msgAttributes,
                                                  &handle,
                                                  rec->d_guid,
                                                  rec->d_appData_sp,
                                                  rec->d_options_sp,
                                                  rec->d_queueKey);

        if (0 != rc) {
            bsl::cout << "Error writing MessageRecord, rc: " << rc
                      << bsl::endl;
            return false;  // RETURN
        }

        bsl::shared_ptr<MessageState> message_sp =
            bsl::allocate_shared<MessageState>(d_allocator_p);
        message_sp->d_records.emplace_back(bslmf::MovableRefUtil::move(rec));
        qstate->d_messages.emplace_back(
            bslmf::MovableRefUtil::move(message_sp));
        return true;
    }

    bool writeConfirmRecord(bsl::shared_ptr<QueueState>& qstate,
                            size_t                       messageOffset)
    {
        bsl::shared_ptr<MessageState>& mstate = qstate->d_messages.at(
            messageOffset);

        size_t randVariance = qstate->d_messages.size() + 1u;

        bsl::shared_ptr<Record> rec;
        rec.createInplace(d_allocator_p);
        rec->d_recordType = mqbs::RecordType::e_CONFIRM;
        rec->d_guid       = mstate->d_records.front()->d_guid;
        rec->d_queueKey   = qstate->d_queueKey;
        rec->d_timestamp  = bdlt::EpochUtil::convertToTimeT64(
            bdlt::CurrentTime::utc());

        mqbs::DataStoreRecordHandle handle;
        const int                   rc = d_fs_p->writeConfirmRecord(
            &handle,
            rec->d_guid,
            rec->d_queueKey,
            mqbu::StorageKey(),
            rec->d_timestamp,
            mqbs::ConfirmReason::e_CONFIRMED);

        if (0 != rc) {
            bsl::cout << "Error writing ConfirmRecord, rc: " << rc
                      << bsl::endl;
            return false;  // RETURN
        }

        mstate->d_records.emplace_back(bslmf::MovableRefUtil::move(rec));
        mstate->d_confirmed = true;
        return true;
    }

    bool writeDeletionRecord(bsl::shared_ptr<QueueState>& qstate,
                             size_t                       messageOffset)
    {
        {
            bsl::shared_ptr<MessageState>& mstate = qstate->d_messages.at(
                messageOffset);
            size_t randVariance = qstate->d_messages.size() + 1u;

            bsl::shared_ptr<Record> rec;
            rec.createInplace(d_allocator_p);
            rec->d_recordType = mqbs::RecordType::e_DELETION;
            rec->d_guid       = mstate->d_records.front()->d_guid;
            rec->d_queueKey   = qstate->d_queueKey;
            rec->d_deletionRecordFlag =
                (randVariance % 2 == 0
                     ? mqbs::DeletionRecordFlag::e_NONE
                     : mqbs::DeletionRecordFlag::e_TTL_EXPIRATION);
            rec->d_timestamp = bdlt::EpochUtil::convertToTimeT64(
                bdlt::CurrentTime::utc());

            const int rc = d_fs_p->writeDeletionRecord(
                rec->d_guid,
                rec->d_queueKey,
                rec->d_deletionRecordFlag,
                rec->d_timestamp);

            if (0 != rc) {
                bsl::cout << "Error writing DeletionRecord, rc: " << rc
                          << bsl::endl;
                return false;  // RETURN
            }

            mstate->d_records.emplace_back(bslmf::MovableRefUtil::move(rec));
        }

        qstate->d_messages.at(messageOffset) = qstate->d_messages.back();
        qstate->d_messages.pop_back();
        return true;
    }

    bool writeSyncPtRecord()
    {
        const int rc = d_fs_p->issueSyncPoint();
        if (rc) {
            bsl::cout << "Error writing SyncPt, rc: " << rc << bsl::endl;
            return false;  // RETURN
        }
        return true;
    }

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RecordWriter, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RecordWriter(mqbs::FileStore* fs, size_t numQueues)
    : d_allocator_p(bmqtst::TestHelperUtil::allocator())
    , d_fs_p(fs)
    , d_bufferFactory(1024, d_allocator_p)
    , d_queues(d_allocator_p)
    , d_openedQueues(d_allocator_p)
    , d_closedQueues(d_allocator_p)
    {
        // PRECONDITIONS
        BMQTST_ASSERT(0 < numQueues);

        d_queues.reserve(numQueues);
        d_closedQueues.reserve(numQueues);
        d_openedQueues.reserve(numQueues);
        for (size_t i = 0; i < numQueues; ++i) {
            bmqu::MemOutStream uri(d_allocator_p);
            uri << "bmq://bmq.test.mem.priority/queue" << i;

            d_queues.emplace_back(
                bsl::allocate_shared<QueueState>(d_allocator_p,
                                                 uri.str(),
                                                 generateQueueKey(i)));
            d_closedQueues.push_back(i);
        }
    }

    bool writeRecords(size_t targetAddedRecords,
                      size_t recordsPerQueue,
                      double targetOpenedQueues)
    {
        int seed = 58133;

        size_t addedRecords = 0;
        size_t prevRecords  = 1;
        while (addedRecords < targetAddedRecords) {
            // Stale loop prevention: add a SyncPt if stale
            prevRecords = addedRecords;
            if ((bdlb::Random::generate15(&seed) % 128) == 0 ||
                prevRecords == addedRecords) {
                if (!writeSyncPtRecord()) {
                    return false;
                }
                if (targetAddedRecords <= ++addedRecords) {
                    return true;
                }
            }

            // 1. Either open or close a queue
            const double currentOpenedQueuesRatio = d_openedQueues.size() /
                                                    static_cast<double>(
                                                        d_queues.size());
            if (currentOpenedQueuesRatio < targetOpenedQueues) {
                // Open a queue that is closed (or skip if nothing to open)
                if (!d_closedQueues.empty()) {
                    size_t offset = bdlb::Random::generate15(&seed) %
                                    d_closedQueues.size();
                    size_t queueIndex = d_closedQueues.at(offset);

                    bsl::shared_ptr<QueueState>& qstate = d_queues.at(
                        queueIndex);
                    if (!writeOpenQueueRecord(qstate)) {
                        return false;
                    }
                    d_closedQueues.at(offset) = d_closedQueues.back();
                    d_closedQueues.pop_back();
                    d_openedQueues.push_back(queueIndex);
                    if (targetAddedRecords <= ++addedRecords) {
                        return true;
                    }
                }
            }
            else {
                // Close a queue that is opened (or skip if nothing to close)
                if (!d_openedQueues.empty()) {
                    size_t offset = bdlb::Random::generate15(&seed) %
                                    d_openedQueues.size();
                    size_t queueIndex = d_openedQueues.at(offset);
                    bsl::shared_ptr<QueueState>& qstate = d_queues.at(
                        queueIndex);

                    bool purge = qstate->d_messages.empty() &&
                                 (0 == (bdlb::Random::generate15(&seed) % 2));
                    if (purge) {
                        if (!writePurgeRecord(qstate)) {
                            return false;
                        }
                        if (targetAddedRecords <= ++addedRecords) {
                            return true;
                        }
                    }
                    else {
                        if (!writeQueueDeletionRecord(qstate)) {
                            return false;
                        }
                        d_openedQueues.at(offset) = d_openedQueues.back();
                        d_openedQueues.pop_back();
                        d_closedQueues.push_back(queueIndex);
                        if (targetAddedRecords <= ++addedRecords) {
                            return true;
                        }
                    }
                }
            }

            if (d_openedQueues.empty()) {
                continue;
            }

            for (size_t i = 0; i < recordsPerQueue; ++i) {
                size_t offset = bdlb::Random::generate15(&seed) %
                                d_openedQueues.size();
                size_t queueIndex = d_openedQueues.at(offset);
                bsl::shared_ptr<QueueState>& qstate = d_queues.at(queueIndex);

                if (qstate->d_messages.size() < recordsPerQueue) {
                    if (!writeMessageRecord(qstate)) {
                        return false;
                    }
                    if (targetAddedRecords <= ++addedRecords) {
                        return true;
                    }
                }
                else {
                    size_t messageOffset = bdlb::Random::generate15(&seed) %
                                           qstate->d_messages.size();
                    bool confirmed =
                        qstate->d_messages.at(messageOffset)->d_confirmed;

                    bool deletion = confirmed ||
                                    (0 ==
                                     (bdlb::Random::generate15(&seed) % 2));
                    if (deletion) {
                        if (!writeDeletionRecord(qstate, messageOffset)) {
                            return false;
                        }
                        if (targetAddedRecords <= ++addedRecords) {
                            return true;
                        }
                    }
                    else {
                        if (!writeConfirmRecord(qstate, messageOffset)) {
                            return false;
                        }
                        if (targetAddedRecords <= ++addedRecords) {
                            return true;
                        }
                    }
                }
            }
        }

        return true;
    }
};

// =============
// struct Tester
// =============
struct Tester {
  private:
    // DATA
    bdlmt::EventScheduler                  d_scheduler;
    bdlbb::PooledBlobBufferFactory         d_bufferFactory;
    bsl::string                            d_clusterLocation;
    bsl::string                            d_clusterArchiveLocation;
    bmqp::BlobPoolUtil::BlobSpPoolSp       d_blobSpPool_sp;
    mqbcfg::PartitionConfig                d_partitionCfg;
    mqbcfg::ClusterDefinition              d_clusterCfg;
    bsl::vector<mqbcfg::ClusterNode>       d_clusterNodesCfg;
    mqbcfg::ClusterNode                    d_clusterNodeCfg;
    bslma::ManagedPtr<mqbnet::MockCluster> d_cluster_mp;
    bsl::shared_ptr<bmqst::StatContext>    d_clusterStatsRootContext_sp;
    mqbstat::ClusterStats                  d_clusterStats;
    mqbnet::ClusterNode*                   d_node_p;
    mqbs::DataStoreConfig                  d_dsCfg;
    bdlmt::FixedThreadPool                 d_miscWorkThreadPool;
    mqbmock::Dispatcher                    d_dispatcher;
    // must outlive FileStore
    bslma::ManagedPtr<mqbs::FileStore> d_fs_mp;
    mqbs::FileStore::StateSpPool       d_statePool;

  public:
    // CREATORS
    Tester(const char* location)
    : d_scheduler(bsls::SystemClockType::e_MONOTONIC,
                  bmqtst::TestHelperUtil::allocator())
    , d_bufferFactory(1024, bmqtst::TestHelperUtil::allocator())
    , d_clusterLocation(location, bmqtst::TestHelperUtil::allocator())
    , d_clusterArchiveLocation(location, bmqtst::TestHelperUtil::allocator())
    , d_blobSpPool_sp(bmqp::BlobPoolUtil::createBlobPool(
          &d_bufferFactory,
          bmqtst::TestHelperUtil::allocator()))
    , d_partitionCfg(bmqtst::TestHelperUtil::allocator())
    , d_clusterCfg(bmqtst::TestHelperUtil::allocator())
    , d_clusterNodesCfg(bmqtst::TestHelperUtil::allocator())
    , d_clusterNodeCfg(bmqtst::TestHelperUtil::allocator())
    , d_clusterStatsRootContext_sp(
          mqbstat::ClusterStatsUtil::initializeStatContextCluster(
              2,
              bmqtst::TestHelperUtil::allocator()))
    , d_clusterStats(bmqtst::TestHelperUtil::allocator())
    , d_miscWorkThreadPool(1, 1, bmqtst::TestHelperUtil::allocator())
    , d_dispatcher(bmqtst::TestHelperUtil::allocator())
    , d_statePool(1024, bmqtst::TestHelperUtil::allocator())
    {
        bdls::FilesystemUtil::remove(d_clusterLocation, true);
        bdls::FilesystemUtil::remove(d_clusterArchiveLocation, true);

        bdls::FilesystemUtil::createDirectories(d_clusterLocation, true);
        bdls::FilesystemUtil::createDirectories(d_clusterArchiveLocation,
                                                true);

        d_partitionCfg.maxDataFileSize()     = 16ULL * 1024 * 1024 * 1024;
        d_partitionCfg.maxQlistFileSize()    = 1024ULL * 1024 * 1024;
        d_partitionCfg.maxJournalFileSize()  = 1024ULL * 1 * 1024 * 1024;
        d_partitionCfg.location()            = d_clusterLocation;
        d_partitionCfg.archiveLocation()     = d_clusterArchiveLocation;
        d_partitionCfg.numPartitions()       = 1;
        d_partitionCfg.maxArchivedFileSets() = 1;
        d_partitionCfg.preallocate()         = false;
        d_partitionCfg.prefaultPages()       = false;

        d_clusterCfg.name().assign("mock-cluster");
        d_clusterCfg.partitionConfig() = d_partitionCfg;

        d_clusterNodeCfg.name().assign("foobar");
        d_clusterNodeCfg.id()         = k_NODE_ID;
        d_clusterNodeCfg.dataCenter() = "US-WEST";
        d_clusterNodeCfg.transport().makeTcp().endpoint().assign(
            "tcp://localhost:34567");
        d_clusterNodesCfg.push_back(d_clusterNodeCfg);

        d_clusterCfg.nodes() = d_clusterNodesCfg;

        d_cluster_mp.load(
            new (*bmqtst::TestHelperUtil::allocator())
                mqbnet::MockCluster(d_clusterCfg,
                                    &d_bufferFactory,
                                    bmqtst::TestHelperUtil::allocator()),
            bmqtst::TestHelperUtil::allocator());
        d_node_p = d_cluster_mp->lookupNode(k_NODE_ID);

        d_dsCfg
            .setScheduler(&d_scheduler)
            // provide a scheduler which has not been started
            .setBufferFactory(&d_bufferFactory)
            .setPreallocate(d_partitionCfg.preallocate())
            .setPrefaultPages(d_partitionCfg.prefaultPages())
            .setLocation(d_partitionCfg.location())
            .setArchiveLocation(d_partitionCfg.archiveLocation())
            .setNodeId(k_NODE_ID)  // TBD: clusterNodeCfg.id())
            .setPartitionId(0)
            .setMaxDataFileSize(d_partitionCfg.maxDataFileSize())
            .setMaxJournalFileSize(d_partitionCfg.maxJournalFileSize())
            .setMaxQlistFileSize(d_partitionCfg.maxQlistFileSize())
            .setRecoveredQueuesCb(bdlf::BindUtil::bind(
                &recoveredQueuesCb,
                bdlf::PlaceHolders::_1,    // partitionId
                bdlf::PlaceHolders::_2));  // queueKeyInfoMap

        d_clusterStats.initialize("testCluster",
                                  1,  // numPartitions
                                  d_clusterStatsRootContext_sp.get(),
                                  bmqtst::TestHelperUtil::allocator());

        // Need to initialize misc work pool for rollover background work
        int rc = d_miscWorkThreadPool.start();
        ASSERT(0 == rc);
        d_miscWorkThreadPool.enable();

        d_fs_mp.load(new (*bmqtst::TestHelperUtil::allocator())
                         mqbs::FileStore(d_dsCfg,
                                         0,  // processorId
                                         &d_dispatcher,
                                         d_cluster_mp.get(),
                                         d_clusterStats.getPartitionStats(
                                             d_dsCfg.partitionId()),
                                         d_blobSpPool_sp.get(),
                                         &d_statePool,
                                         &d_miscWorkThreadPool,
                                         true,  // isCSLModeEnabled
                                         true,  // isFSMWorkflow
                                         true,  // doesFSMwriteQLIST
                                         1,     // replicationFactor
                                         bmqtst::TestHelperUtil::allocator()),
                     bmqtst::TestHelperUtil::allocator());

        // To pass `inDispatcherThread` checks:
        d_fs_mp->setThreadId(bslmt::ThreadUtil::selfId());
    }

    ~Tester()
    {
        bdls::FilesystemUtil::remove(d_clusterLocation, true);
        bdls::FilesystemUtil::remove(d_clusterArchiveLocation, true);
    }

    // ACCESSORS
    mqbs::FileStore& fileStore() const { return *(d_fs_mp); }

    mqbnet::ClusterNode* node() const { return d_node_p; }
};

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    const char k_FILE_STORE_LOCATION[] = "./test-cluster123-1";

    Tester           tester(k_FILE_STORE_LOCATION);
    mqbs::FileStore& fs = tester.fileStore();

    int rc = fs.open();
    BMQTST_ASSERT_EQ(0, rc);
    if (rc) {
        cout << "Failed to open partition, rc: " << rc << endl;
        return;  // RETURN
    }

    BMQTST_ASSERT_EQ(true, fs.isOpen());
    BMQTST_ASSERT_EQ(1U, fs.clusterSize());
    BMQTST_ASSERT_EQ(0ULL, fs.numRecords());
    BMQTST_ASSERT_EQ(true, fs.syncPoints().empty());
    BMQTST_ASSERT_EQ(0U, fs.primaryLeaseId());
    BMQTST_ASSERT_EQ(0ULL, fs.sequenceNumber());

    // Set primary.
    unsigned int        primaryLeaseId = 1;
    bsls::Types::Uint64 seqNum         = 1;

    fs.setActivePrimary(tester.node(), primaryLeaseId);

    BMQTST_ASSERT_EQ(primaryLeaseId, fs.primaryLeaseId());
    BMQTST_ASSERT_EQ(seqNum, fs.sequenceNumber());
    BMQTST_ASSERT_EQ(tester.node(), fs.primaryNode());

    // Primary must have issued a SyncPt.  Verify it.

    const bmqp_ctrlmsg::SyncPoint& sp = fs.syncPoints().front().syncPoint();
    BMQTST_ASSERT_EQ(1U, fs.syncPoints().size());
    BMQTST_ASSERT_EQ(primaryLeaseId, sp.primaryLeaseId());
    BMQTST_ASSERT_EQ(seqNum, sp.sequenceNum());
    BMQTST_ASSERT_EQ(
        (k_SIZEOF_HEADERS_DATA_FILE / bmqp::Protocol::k_DWORD_SIZE),
        sp.dataFileOffsetDwords());
    BMQTST_ASSERT_EQ(
        (k_SIZEOF_HEADERS_QLIST_FILE / bmqp::Protocol::k_WORD_SIZE),
        sp.qlistFileOffsetWords());
    BMQTST_ASSERT_EQ(k_SIZEOF_HEADERS_JOURNAL_FILE,
                     fs.syncPoints().front().offset());

    // Write various records to the partition and keep track of them in memory.
    // Then close and re-open the partition, and verify that retrieved records
    // match in-memory stuff.

    SyncPointOffsetPairs spOffsetPairs(bmqtst::TestHelperUtil::allocator());
    bsl::vector<HandleRecordPair> records(bmqtst::TestHelperUtil::allocator());

    // Add one SyncPt written by the primary (to both 'spOffsetPairs' and
    // 'records').

    Record rec(bmqtst::TestHelperUtil::allocator());
    rec.d_recordType    = mqbs::RecordType::e_JOURNAL_OP;
    rec.d_journalOpType = mqbs::JournalOpType::e_SYNCPOINT;
    rec.d_syncPtType    = mqbs::SyncPointType::e_REGULAR;

    rec.d_syncPoint.primaryLeaseId()       = primaryLeaseId;
    rec.d_syncPoint.sequenceNum()          = sp.sequenceNum();
    rec.d_syncPoint.dataFileOffsetDwords() = sp.dataFileOffsetDwords();
    rec.d_syncPoint.qlistFileOffsetWords() = sp.qlistFileOffsetWords();
    records.push_back(bsl::make_pair(mqbs::DataStoreRecordHandle(), rec));
    spOffsetPairs.push_back(fs.syncPoints().front());

    const size_t        k_NUM_RECORDS     = 1200;
    bsls::Types::Uint64 numRecordsWritten = 0;

    RecordWriter writer(&fs, 16u);
    const bool   success = writer.writeRecords(k_NUM_RECORDS, 65536u, 0.8);
    if (!success) {
        // There IS a problem already, try to close FileStore to see
        // if it crashes anyhow at this point.
        fs.close();

        BMQTST_ASSERT(false && "Writing records failed");
    }

    const SyncPointOffsetPairs& fsSpOffsetPair = fs.syncPoints();
    BMQTST_ASSERT_EQ(spOffsetPairs.size(), fsSpOffsetPair.size());
    for (size_t i = 0; i < spOffsetPairs.size(); ++i) {
        BMQTST_ASSERT_EQ_D(i,
                           spOffsetPairs[i].syncPoint(),
                           fsSpOffsetPair[i].syncPoint());
        BMQTST_ASSERT_EQ_D(i,
                           spOffsetPairs[i].offset(),
                           fsSpOffsetPair[i].offset());
    }
    BMQTST_ASSERT_EQ(k_NUM_RECORDS, fs.numRecords());

    mqbs::FileStoreIterator fsIt(&fs);
    while (fsIt.next()) {
        // TBD: verify
    }

    fs.close();

    BMQTST_ASSERT_EQ(false, fs.isOpen());

    // TBD: Open it again, and iterate over it again, and check retrieved
    // queue uris, keys, appIds, appKeys against in-memory data structure.
}

static void test2_printTest()
// ------------------------------------------------------------------------
// PRINT TEST
//
// Concerns:
//   Test printing a 'mqbs::FileStoreIterator'
//
// Testing:
//   operator<<(bsl::ostream& stream, const FileStoreIterator& rhs
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRINT TEST");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    const char k_FILE_STORE_LOCATION[] = "./test-cluster123-2";

    Tester           tester(k_FILE_STORE_LOCATION);
    mqbs::FileStore& fs = tester.fileStore();
    BSLS_ASSERT_OPT(fs.open() == 0);

    // Set primary.
    unsigned int        primaryLeaseId = 1;
    bsls::Types::Uint64 seqNum         = 1;
    fs.setActivePrimary(tester.node(), primaryLeaseId);

    // Write various records to the partition.
    SyncPointOffsetPairs spOffsetPairs(bmqtst::TestHelperUtil::allocator());
    bsl::vector<HandleRecordPair> records(bmqtst::TestHelperUtil::allocator());

    const size_t k_NUM_QUEUES        = 16u;
    const size_t k_NUM_RECORDS       = 1024u;
    const size_t k_RECORDS_PER_QUEUE = 16u;

    RecordWriter writer(&fs, k_NUM_QUEUES);
    const bool   success = writer.writeRecords(k_NUM_RECORDS,
                                             k_RECORDS_PER_QUEUE,
                                             0.8);
    if (!success) {
        // There IS a problem already, try to close FileStore to see
        // if it crashes anyhow at this point.
        fs.close();

        BMQTST_ASSERT(false && "Writing records failed");
    }

    struct RecordSearchContext {
        // PUBLIC DATA
        bsl::shared_ptr<bdlpcre::RegEx> d_regex_sp;

        bsl::string d_description;

        size_t d_num;

        RecordSearchContext(bsl::string_view  regex,
                            bsl::string_view  description,
                            bslma::Allocator* allocator = 0)
        : d_regex_sp(bsl::allocate_shared<bdlpcre::RegEx>(allocator))
        , d_description(description, allocator)
        , d_num(0)
        {
            bsl::string errorMessage(allocator);
            size_t      errorOffset = 0;
            d_regex_sp->prepare(&errorMessage, &errorOffset, regex.data());
            BMQTST_ASSERT(d_regex_sp->isPrepared());
        }

        bool feed(bsl::string_view line)
        {
            if (d_regex_sp->match(line) == bdlpcre::RegEx::k_STATUS_SUCCESS) {
                ++d_num;
                return true;
            }
            return false;
        }
    };

    bsl::vector<RecordSearchContext> contexts(
        bmqtst::TestHelperUtil::allocator());

    contexts.emplace_back(RecordSearchContext(
        "\\[ queueOpRecord = \\[ header = \\[ type = QUEUE_OP flags = 0 "
        "primaryLeaseId = 1 sequenceNumber = [0-9]+ timestamp = [0-9]+ ] "
        "flags = 0 "
        "queueKey = [0-9A-F]+ appKey = 0000000000 type = CREATION "
        "queueUriRecordOffsetWords = [0-9]+ ] ]",
        "QUEUE CREATION",
        bmqtst::TestHelperUtil::allocator()));

    contexts.emplace_back(RecordSearchContext(
        "\\[ queueOpRecord = \\[ header = \\[ type = QUEUE_OP flags = 0 "
        "primaryLeaseId = 1 sequenceNumber = [0-9]+ timestamp = [0-9]+ ] "
        "flags = 0 "
        "queueKey = [0-9A-F]+ appKey = 0000000000 type = DELETION "
        "queueUriRecordOffsetWords = [0-9]+ ] ]",
        "QUEUE DELETION",
        bmqtst::TestHelperUtil::allocator()));

    contexts.emplace_back(RecordSearchContext(
        "\\[ messageRecord = \\[ header = \\[ type = MESSAGE flags = [0-9]+ "
        "primaryLeaseId = 1 sequenceNumber = [0-9]+ timestamp = [0-9]+ ] "
        "refCount "
        "= "
        "[0-9]+ queueKey = [0-9A-F]+ fileKey = 0000000000 messageOffsetDwords "
        "= [0-9]+ "
        "messageGUID = [0-9|A-Z]+ crc32c = [0-9]+ compressionAlgorithmType = "
        "NONE ] ]",
        "MESSAGE",
        bmqtst::TestHelperUtil::allocator()));

    contexts.emplace_back(RecordSearchContext(
        "\\[ confirmRecord = \\[ header = \\[ type = CONFIRM flags = 0 "
        "primaryLeaseId = 1 sequenceNumber = [0-9]+ timestamp = [0-9]+ ] "
        "reason = CONFIRMED queueKey = [0-9A-F]+ appKey = 0000000000 "
        "messageGUID = [0-9|A-Z]+ ] ]",
        "CONFIRM",
        bmqtst::TestHelperUtil::allocator()));

    mqbs::FileStoreIterator fsIt(&fs);
    bmqu::MemOutStream      stream(bmqtst::TestHelperUtil::allocator());
    while (fsIt.next()) {
        stream.reset();
        stream << fsIt;

        bool found = false;
        for (size_t i = 0; i < contexts.size(); ++i) {
            if (contexts[i].feed(stream.str())) {
                found = true;
                break;
            }
        }

        if (!found) {
            BALL_LOG_SET_CATEGORY("MQBS.FILESTORE.T");
            BALL_LOG_ERROR << "Unrecognized record not matching any pattern: "
                           << stream.str();
            BMQTST_ASSERT(false && "Unrecognized record");
        }
    }

    for (size_t i = 0; i < contexts.size(); i++) {
        if (0 == contexts[i].d_num) {
            BALL_LOG_SET_CATEGORY("MQBS.FILESTORE.T");
            BALL_LOG_ERROR << "No records found for pattern "
                           << contexts[i].d_description;
            BMQTST_ASSERT(false && "No records found for pattern");
        }
    }

    PV("Bad stream test");
    stream.reset();
    stream << "INVALID";
    stream.clear(bsl::ios_base::badbit);
    stream << fsIt;
    BMQTST_ASSERT_EQ(stream.str(), "INVALID");

    fs.close();
}

static void testN1_rolloverPerformanceTest()
// ------------------------------------------------------------------------
// ROLLOVER PERFORMANCE TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    const char k_FILE_STORE_LOCATION[] = "./test-cluster123-1";

    Tester           tester(k_FILE_STORE_LOCATION);
    mqbs::FileStore& fs = tester.fileStore();

    int rc = fs.open();
    BMQTST_ASSERT_EQ(0, rc);
    if (rc) {
        cout << "Failed to open partition, rc: " << rc << endl;
        return;  // RETURN
    }

    BMQTST_ASSERT_EQ(true, fs.isOpen());
    BMQTST_ASSERT_EQ(1U, fs.clusterSize());
    BMQTST_ASSERT_EQ(0ULL, fs.numRecords());
    BMQTST_ASSERT_EQ(true, fs.syncPoints().empty());
    BMQTST_ASSERT_EQ(0U, fs.primaryLeaseId());
    BMQTST_ASSERT_EQ(0ULL, fs.sequenceNumber());

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.  TBD: figure
    // out the right way to "fix" this.

    Record dummy(bmqtst::TestHelperUtil::allocator());
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Record, bslma::UsesBslmaAllocator> >(
            dummy));

    // Set primary.
    unsigned int        primaryLeaseId = 1;
    bsls::Types::Uint64 seqNum         = 1;

    fs.setActivePrimary(tester.node(), primaryLeaseId);

    BMQTST_ASSERT_EQ(primaryLeaseId, fs.primaryLeaseId());
    BMQTST_ASSERT_EQ(seqNum, fs.sequenceNumber());
    BMQTST_ASSERT_EQ(tester.node(), fs.primaryNode());

    // Primary must have issued a SyncPt.  Verify it.

    const bmqp_ctrlmsg::SyncPoint& sp = fs.syncPoints().front().syncPoint();
    BMQTST_ASSERT_EQ(1U, fs.syncPoints().size());
    BMQTST_ASSERT_EQ(primaryLeaseId, sp.primaryLeaseId());
    BMQTST_ASSERT_EQ(seqNum, sp.sequenceNum());
    BMQTST_ASSERT_EQ(
        (k_SIZEOF_HEADERS_DATA_FILE / bmqp::Protocol::k_DWORD_SIZE),
        sp.dataFileOffsetDwords());
    BMQTST_ASSERT_EQ(
        (k_SIZEOF_HEADERS_QLIST_FILE / bmqp::Protocol::k_WORD_SIZE),
        sp.qlistFileOffsetWords());
    BMQTST_ASSERT_EQ(k_SIZEOF_HEADERS_JOURNAL_FILE,
                     fs.syncPoints().front().offset());

    // Write various records to the partition and keep track of them in memory.
    // Then close and re-open the partition, and verify that retrieved records
    // match in-memory stuff.

    bsl::vector<HandleRecordPair> records(bmqtst::TestHelperUtil::allocator());

    // Add one SyncPt written by the primary (to both 'spOffsetPairs' and
    // 'records').

    Record rec(bmqtst::TestHelperUtil::allocator());
    rec.d_recordType    = mqbs::RecordType::e_JOURNAL_OP;
    rec.d_journalOpType = mqbs::JournalOpType::e_SYNCPOINT;
    rec.d_syncPtType    = mqbs::SyncPointType::e_REGULAR;

    rec.d_syncPoint.primaryLeaseId()       = primaryLeaseId;
    rec.d_syncPoint.sequenceNum()          = sp.sequenceNum();
    rec.d_syncPoint.dataFileOffsetDwords() = sp.dataFileOffsetDwords();
    rec.d_syncPoint.qlistFileOffsetWords() = sp.qlistFileOffsetWords();
    records.push_back(bsl::make_pair(mqbs::DataStoreRecordHandle(), rec));

    const size_t        k_NUM_RECORDS       = 1024u * 1024u;
    const size_t        k_RECORDS_PER_QUEUE = 1024;
    const size_t        k_NUM_QUEUES        = 1024;
    bsls::Types::Uint64 numRecordsWritten   = 0;

    RecordWriter writer(&fs, k_NUM_QUEUES);
    bool         success = writer.writeRecords(k_NUM_RECORDS,
                                       k_RECORDS_PER_QUEUE,
                                       0.8);

    BMQTST_ASSERT_EQ(true, success);
    if (!success) {
        fs.close();
        return;  // RETURN
    }

    const size_t             k_NUM_ROLLOVERS = 16;
    const bsls::Types::Int64 start           = bsls::TimeUtil::getTimer();
    for (size_t i = 0; i < k_NUM_ROLLOVERS; i++) {
        fs.forceRollover();
    }
    const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    bsl::cout << "Rollover time avg: "
              << bmqu::PrintUtil::prettyTimeInterval((end - start) /
                                                     k_NUM_ROLLOVERS)
              << " (" << k_NUM_ROLLOVERS << " iters)" << bsl::endl;

    fs.close();
}

}  // close unnamed namespace

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize(bmqtst::TestHelperUtil::allocator());
    mqbu::MessageGUIDUtil::initialize();
    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 2: test2_printTest(); break;
    case 1: test1_breathingTest(); break;
    case -1: testN1_rolloverPerformanceTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();
    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_ALLOC);
}
