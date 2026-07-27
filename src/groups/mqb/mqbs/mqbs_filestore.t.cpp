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

#include <mqbs_filestore.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_storage.h>
#include <mqbmock_cluster.h>
#include <mqbmock_dispatcher.h>
#include <mqbmock_domain.h>
#include <mqbmock_queue.h>
#include <mqbnet_mockcluster.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreset.h>
#include <mqbs_filestoretestutil.h>
#include <mqbs_filestoreutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbstat_clusterstats.h>
#include <mqbu_messageguidutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_blobpoolutil.h>
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_storageeventbuilder.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

#include <bmqu_memoutstream.h>
#include <bmqu_time.h>

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
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_platform.h>
#include <bsls_systemclocktype.h>
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

// CLASSES

/// Helper to post dummy messages to a `ReplicatedStorage` for testing.
class StoragePoster {
    /// Target storage to post messages to.
    bsl::shared_ptr<mqbs::ReplicatedStorage> d_storage_sp;

    /// Factory for allocating blob buffers for message payloads.
    bdlbb::PooledBlobBufferFactory d_bufferFactory;

  public:
    /// Create a `StoragePoster` that posts to the specified `storage`,
    /// using the specified `allocator` for memory allocation.
    StoragePoster(const bsl::shared_ptr<mqbs::ReplicatedStorage>& storage,
                  bslma::Allocator*                               allocator)
    : d_storage_sp(storage)
    , d_bufferFactory(1024, allocator)
    {
    }

    /// Post a dummy message to the underlying storage. Return the result
    /// of the put operation.
    mqbi::StorageResult::Enum postMessage()
    {
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        bsl::shared_ptr<bdlbb::Blob> appData_sp;
        appData_sp.createInplace(bmqtst::TestHelperUtil::allocator(),
                                 &d_bufferFactory,
                                 bmqtst::TestHelperUtil::allocator());
        bsl::string payload(10, 'x', bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobUtil::append(appData_sp.get(),
                                payload.c_str(),
                                payload.length());

        bsls::Types::Uint64 timestamp = bdlt::EpochUtil::convertToTimeT64(
            bdlt::CurrentTime::utc());

        mqbi::StorageMessageAttributes attributes(
            timestamp,
            1,  // refCount
            static_cast<unsigned int>(appData_sp->length()),
            bmqp::MessagePropertiesInfo(),
            bmqt::CompressionAlgorithmType::e_NONE,
            true,  // hasReceipt
            0,     // queueHandle
            bmqp::Crc32c::calculate(*appData_sp));

        bsl::shared_ptr<bdlbb::Blob> options_sp;

        return d_storage_sp->put(&attributes, guid, appData_sp, options_sp);
    }
};

// FUNCTIONS

void recoveredQueuesCb(
    int                                           partitionId,
    const mqbs::DataStoreConfig::QueueKeyInfoMap* queueKeyInfoMap)
{
    static_cast<void>(partitionId);
    static_cast<void>(queueKeyInfoMap);
}

/// Return true if the journal file at the specified `journalFilePath` holds at
/// least one sync point record of the specified `subtype`.
static bool journalHasSyncPoint(const bsl::string&        journalFilePath,
                                mqbs::SyncPointType::Enum subtype)
{
    mqbs::FileStoreSet fileSet(bmqtst::TestHelperUtil::allocator());
    fileSet.setJournalFile(journalFilePath)
        .setJournalFileSize(
            bdls::FilesystemUtil::getFileSize(journalFilePath));

    mqbs::MappedFileDescriptor journalFd;
    bmqu::MemOutStream         errDesc(bmqtst::TestHelperUtil::allocator());
    int rc = mqbs::FileStoreUtil::openFileSetReadMode(errDesc,
                                                      fileSet,
                                                      &journalFd);
    BMQTST_ASSERT_EQ(0, rc);
    if (0 != rc) {
        return false;  // RETURN
    }

    mqbs::JournalFileIterator jit;
    rc = mqbs::FileStoreUtil::loadIterators(errDesc, fileSet, &jit, journalFd);
    BMQTST_ASSERT_EQ(0, rc);

    bool found = false;
    while (1 == jit.nextRecord()) {
        if (mqbs::RecordType::e_JOURNAL_OP == jit.recordType()) {
            const mqbs::JournalOpRecord& rec = jit.asJournalOpRecord();
            if (mqbs::JournalOpType::e_SYNCPOINT == rec.type() &&
                subtype == rec.syncPointType()) {
                found = true;
                break;
            }
        }
    }

    mqbs::FileSystemUtil::close(&journalFd);
    return found;
}

/// Return the number of QueueOp.DELETION records in the journal file at the
/// specified `journalFilePath`, or -1 if it could not be opened.
static int journalDeletionCount(const bsl::string& journalFilePath)
{
    mqbs::FileStoreSet fileSet(bmqtst::TestHelperUtil::allocator());
    fileSet.setJournalFile(journalFilePath)
        .setJournalFileSize(
            bdls::FilesystemUtil::getFileSize(journalFilePath));

    mqbs::MappedFileDescriptor journalFd;
    bmqu::MemOutStream         errDesc(bmqtst::TestHelperUtil::allocator());
    int rc = mqbs::FileStoreUtil::openFileSetReadMode(errDesc,
                                                      fileSet,
                                                      &journalFd);
    BMQTST_ASSERT_EQ(0, rc);
    if (0 != rc) {
        return -1;  // RETURN
    }

    mqbs::JournalFileIterator jit;
    rc = mqbs::FileStoreUtil::loadIterators(errDesc, fileSet, &jit, journalFd);
    BMQTST_ASSERT_EQ(0, rc);

    int count = 0;
    while (1 == jit.nextRecord()) {
        if (mqbs::RecordType::e_QUEUE_OP == jit.recordType() &&
            mqbs::QueueOpType::e_DELETION == jit.asQueueOpRecord().type()) {
            ++count;
        }
    }

    mqbs::FileSystemUtil::close(&journalFd);
    return count;
}

/// Build a storage event carrying a single regular SyncPt journal record with
/// the specified `leaseId`, `seqNum`, `journalOffsetWords`, `dataOffsetDwords`
/// and `qlistOffsetWords`, and apply it to the specified `fs` as a
/// partition-sync event received from `source`.  Use the specified
/// `blobSpPool`, `bufferFactory`, `partitionId` and `allocator`.
void applyReplicatedSyncPoint(mqbs::FileStore*                fs,
                              bmqp::BlobPoolUtil::BlobSpPool* blobSpPool,
                              bdlbb::BlobBufferFactory*       bufferFactory,
                              mqbnet::ClusterNode*            source,
                              int                             partitionId,
                              unsigned int                    leaseId,
                              bsls::Types::Uint64             seqNum,
                              unsigned int      journalOffsetWords,
                              unsigned int      dataOffsetDwords,
                              unsigned int      qlistOffsetWords,
                              bslma::Allocator* allocator)
{
    // Lay out a regular SyncPt journal record in a fresh blob buffer.  The
    // RecordHeader carries the PSN that 'processStorageEvent' validates.
    bdlbb::BlobBuffer recBuf;
    bufferFactory->allocate(&recBuf);
    bsl::memset(recBuf.data(),
                0,
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

    mqbs::JournalOpRecord* rec = new (recBuf.data())
        mqbs::JournalOpRecord(mqbs::JournalOpType::e_SYNCPOINT,
                              mqbs::SyncPointType::e_REGULAR,
                              seqNum,
                              k_NODE_ID,
                              leaseId,
                              dataOffsetDwords,
                              qlistOffsetWords,
                              mqbs::RecordHeader::k_MAGIC);
    rec->header()
        .setPrimaryLeaseId(leaseId)
        .setSequenceNumber(seqNum)
        .setTimestamp(
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));

    bdlbb::BlobBuffer journalRecBuf(
        recBuf.buffer(),
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

    bmqp::StorageEventBuilder      builder(mqbs::FileStoreProtocol::k_VERSION,
                                      bmqp::EventType::e_PARTITION_SYNC,
                                      blobSpPool,
                                      allocator);
    bmqt::EventBuilderResult::Enum brc = builder.packMessage(
        bmqp::StorageMessageType::e_JOURNAL_OP,
        static_cast<unsigned int>(partitionId),
        0,  // flags
        journalOffsetWords,
        journalRecBuf);
    BMQTST_ASSERT_EQ(brc, bmqt::EventBuilderResult::e_SUCCESS);

    fs->processStorageEvent(builder.blob(),
                            true,  // isPartitionSyncEvent
                            source);
}

// CLASSES
// ============
// class Tester
// ============
class Tester {
  private:
    // DATA
    bslma::Allocator*                      d_allocator_p;
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
    explicit Tester(bsl::string_view    location,
                    bsls::Types::Uint64 maxJournalFileSize = 1 * 1024 * 1024)
    : d_allocator_p(bmqtst::TestHelperUtil::allocator())
    , d_scheduler(bsls::SystemClockType::e_MONOTONIC, d_allocator_p)
    , d_bufferFactory(1024, d_allocator_p)
    , d_clusterLocation(location, d_allocator_p)
    , d_clusterArchiveLocation(location, d_allocator_p)
    , d_blobSpPool_sp(
          bmqp::BlobPoolUtil::createBlobPool(&d_bufferFactory, d_allocator_p))
    , d_partitionCfg(d_allocator_p)
    , d_clusterCfg(d_allocator_p)
    , d_clusterNodesCfg(d_allocator_p)
    , d_clusterNodeCfg(d_allocator_p)
    , d_clusterStatsRootContext_sp(
          mqbstat::ClusterStatsUtil::initializeStatContextCluster(
              2,
              d_allocator_p))
    , d_clusterStats(d_allocator_p)
    , d_miscWorkThreadPool(1, 1, d_allocator_p)
    , d_dispatcher(d_allocator_p)
    , d_statePool(1024, d_allocator_p)
    {
        bdls::FilesystemUtil::remove(d_clusterLocation, true);
        bdls::FilesystemUtil::remove(d_clusterArchiveLocation, true);

        bdls::FilesystemUtil::createDirectories(d_clusterLocation, true);
        bdls::FilesystemUtil::createDirectories(d_clusterArchiveLocation,
                                                true);
        {
            BSLA_MAYBE_UNUSED const int rc = d_scheduler.start();
            BMQTST_ASSERT_EQ(rc, 0);
        }

        {
            BSLA_MAYBE_UNUSED const int rc = d_miscWorkThreadPool.start();
            BMQTST_ASSERT_EQ(rc, 0);
        }

        d_partitionCfg.maxDataFileSize()     = 100 * 1024 * 1024;
        d_partitionCfg.maxQlistFileSize()    = 1 * 1024 * 1024;
        d_partitionCfg.maxCSLFileSize()      = 1 * 1024 * 1024;
        d_partitionCfg.maxJournalFileSize()  = maxJournalFileSize;
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

        d_cluster_mp =
            bslma::ManagedPtrUtil::allocateManaged<mqbnet::MockCluster>(
                d_allocator_p,
                d_clusterCfg,
                &d_bufferFactory);
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
                                  d_allocator_p);
        d_fs_mp = bslma::ManagedPtrUtil::allocateManaged<mqbs::FileStore>(
            d_allocator_p,
            d_dsCfg,
            0,  // processorId
            &d_dispatcher,
            d_cluster_mp.get(),
            d_clusterStats.getPartitionStats(d_dsCfg.partitionId()),
            d_blobSpPool_sp.get(),
            &d_statePool,
            &d_miscWorkThreadPool,
            true,  // isFSMWorkflow
            true,  // doesFSMwriteQLIST
            1);    // replicationFactor

        // To pass `inDispatcherThread` checks:
        d_fs_mp->setThreadId(bslmt::ThreadUtil::selfId());
    }

    ~Tester()
    {
        d_scheduler.stop();
        d_miscWorkThreadPool.stop();

        bdls::FilesystemUtil::remove(d_clusterLocation, true);
        bdls::FilesystemUtil::remove(d_clusterArchiveLocation, true);
    }

    // MANIPULATORS
    bool writeRecords(mqbs::FileStore*               fs,
                      bsl::vector<HandleRecordPair>* records,
                      SyncPointOffsetPairs*          spOffsetPairs,
                      unsigned int                   leaseId,
                      bsls::Types::Uint64*           seqNum,
                      bsls::Types::Uint64*           numRecordsWritten,
                      bsls::Types::Uint64            numRecords)
    {
        // TBD:  need to create a FileBackedStorage-like data structure, which
        // maintains a map of 'QueueKey ->
        // OrderedHashMap(Guid->list(Handles))'.  This will be useful while
        // deleting a record and purging the queue.

        typedef bsl::map<mqbu::StorageKey, bsl::string> QueueKeyUriMap;
        typedef QueueKeyUriMap::iterator                QueueKeyUriMapIter;
        typedef bsl::vector<bmqt::MessageGUID>          Guids;
        typedef bsl::map<mqbu::StorageKey, Guids>       QueueKeyGuidsMap;
        typedef QueueKeyGuidsMap::iterator              QueueKeyGuidsMapIter;

        QueueKeyUriMap   queueKeyUriMap(bmqtst::TestHelperUtil::allocator());
        QueueKeyGuidsMap queueKeyGuidsMap(bmqtst::TestHelperUtil::allocator());
        QueueKeyGuidsMap queueKeyConfGuidsMap(
            bmqtst::TestHelperUtil::allocator());
        bsl::string  uriBase("bmq://si.amw.bmq.stats/",
                            bmqtst::TestHelperUtil::allocator());
        const size_t k_DIVISOR = 7;
        int          rc        = 0;
        int          seed      = 58133;
        // initial seed for bdlb::Random

        for (size_t i = 0; i < numRecords; ++i) {
            // Total 7 types of records.
            // QueueOp - creation, purge, deletion.
            // Message
            // Confirm
            // Deletion
            // JournalOp - SyncPt

            size_t recType = i % k_DIVISOR;

            if (0 == recType) {
                // Write a queue creation record.

                bsl::string uri(uriBase, bmqtst::TestHelperUtil::allocator());
                bmqu::MemOutStream osstr;
                osstr << "queue" << i;
                uri.append(osstr.str().data(), osstr.str().length());

                osstr.reset();  // clear the stream

                // Generate unique queue-key.
                // TBD: make this uniq-ify the keys.

                osstr << i;
                for (size_t j = 0; j < mqbu::StorageKey::e_KEY_LENGTH_BINARY;
                     ++j) {
                    osstr << 'x';
                }

                bsl::string      queueKeyStr(osstr.str().data(),
                                        osstr.str().length());
                mqbu::StorageKey queueKey(
                    mqbu::StorageKey::BinaryRepresentation(),
                    queueKeyStr
                        .substr(0, mqbu::StorageKey::e_KEY_LENGTH_BINARY)
                        .c_str());

                mqbs::DataStoreRecordHandle handle;
                Record rec(bmqtst::TestHelperUtil::allocator());
                rec.d_recordType  = mqbs::RecordType::e_QUEUE_OP;
                rec.d_queueOpType = mqbs::QueueOpType::e_CREATION;
                rec.d_uri         = uri;
                rec.d_queueKey    = queueKey;
                rec.d_timestamp   = bdlt::EpochUtil::convertToTimeT64(
                    bdlt::CurrentTime::utc());

                rc = fs->writeQueueCreationRecord(
                    &handle,
                    bmqt::Uri(rec.d_uri, bmqtst::TestHelperUtil::allocator()),
                    rec.d_queueKey,
                    AppInfos(),
                    rec.d_timestamp,
                    true);  // isNewQueue

                if (0 != rc) {
                    bsl::cout
                        << "Error writing QueueCreationRecord, rc: " << rc
                        << bsl::endl;
                    return false;  // RETURN
                }

                records->push_back(bsl::make_pair(handle, rec));
                ++(*seqNum);
                ++(*numRecordsWritten);

                // Add this queue uri/key to the list of valid pairs.

                queueKeyUriMap[rec.d_queueKey] = rec.d_uri;

                continue;  // CONTINUE
            }

            if (1 == recType) {
                // Write a message record.  Randomly choose a queue uri/key
                // pair from 'queueUriKeyPairs', and write a message record for
                // that pair.  Also update 'queueIndexGuidsMap' entry for that
                // pair by adding guid to the list of guids associated with
                // that pair.

                size_t offset = bdlb::Random::generate15(&seed) %
                                queueKeyUriMap.size();

                QueueKeyUriMapIter it = queueKeyUriMap.begin();
                bsl::advance(it, offset);

                BSLS_ASSERT(!it->first.isNull());
                BSLS_ASSERT(!it->second.empty());

                mqbs::DataStoreRecordHandle handle;
                Record rec(bmqtst::TestHelperUtil::allocator());
                rec.d_recordType = mqbs::RecordType::e_MESSAGE;
                rec.d_queueKey   = it->first;
                bmqp::MessagePropertiesInfo messagePropertiesInfo =
                    (0 == i % 2) ? bmqp::MessagePropertiesInfo::makeNoSchema()
                                 : bmqp::MessagePropertiesInfo();

                // crc value
                mqbu::MessageGUIDUtil::generateGUID(&rec.d_guid);
                rec.d_appData_sp.createInplace(
                    bmqtst::TestHelperUtil::allocator(),
                    &d_bufferFactory,
                    bmqtst::TestHelperUtil::allocator());
                bsl::string payloadStr(i * 10,
                                       'x',
                                       bmqtst::TestHelperUtil::allocator());
                bdlbb::BlobUtil::append(rec.d_appData_sp.get(),
                                        payloadStr.c_str(),
                                        payloadStr.length());

                const unsigned int appDataLen = static_cast<unsigned int>(
                    rec.d_appData_sp->length());

                rec.d_msgAttributes = mqbi::StorageMessageAttributes(
                    bdlt::EpochUtil::convertToTimeT64(
                        bdlt::CurrentTime::utc()),
                    i % mqbs::FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD,
                    appDataLen,
                    messagePropertiesInfo,
                    bmqt::CompressionAlgorithmType::e_NONE,
                    bsl::numeric_limits<unsigned int>::max() / i);

                rc = fs->writeMessageRecord(&rec.d_msgAttributes,
                                            &handle,
                                            rec.d_guid,
                                            rec.d_appData_sp,
                                            rec.d_options_sp,
                                            rec.d_queueKey);

                if (0 != rc) {
                    bsl::cout << "Error writing MessageRecord, rc: " << rc
                              << bsl::endl;
                    return false;  // RETURN
                }

                records->push_back(bsl::make_pair(handle, rec));
                ++(*seqNum);
                ++(*numRecordsWritten);

                // Add the guid to the list of valid guids for the queue key,
                // so that we can later use this guid to confirm.  Note that
                // the choice of using operator[] on the 'queueKeyGuidsMap' is
                // deliberate, as an entry for the queue key may or may not
                // exist.

                queueKeyGuidsMap[rec.d_queueKey].push_back(rec.d_guid);

                continue;  // CONTINUE
            }

            if (2 == recType) {
                // Write a confirm record.  Randomly retrieve a queue key entry
                // from 'queueKeyGuidsMap' (this chosen queue key *must* exist
                // in 'queueKeyUriMap').  Then take the last guid from the list
                // of guids associated with that queue key, confirm it, remove
                // it from that list (and it list then becomes empty, delete
                // this queue key entry from 'queueKeyGuidsMap'), and add it to
                // the entry for that queue key->list(guids) in
                // 'queueKeyConfGuidsMap'.

                size_t offset = bdlb::Random::generate15(&seed) %
                                queueKeyGuidsMap.size();

                QueueKeyGuidsMapIter it = queueKeyGuidsMap.begin();
                bsl::advance(it, offset);

                BSLS_ASSERT(!it->first.isNull());
                BSLS_ASSERT(!it->second.empty());
                BSLS_ASSERT(queueKeyUriMap.end() !=
                            queueKeyUriMap.find(it->first));

                Guids& guids = it->second;
                BSLS_ASSERT(!guids.empty());

                mqbs::DataStoreRecordHandle handle;
                Record rec(bmqtst::TestHelperUtil::allocator());
                rec.d_recordType = mqbs::RecordType::e_CONFIRM;
                rec.d_guid       = guids.back();
                rec.d_queueKey   = it->first;
                rec.d_timestamp  = bdlt::EpochUtil::convertToTimeT64(
                    bdlt::CurrentTime::utc());

                rc = fs->writeConfirmRecord(&handle,
                                            rec.d_guid,
                                            rec.d_queueKey,
                                            mqbu::StorageKey(),
                                            rec.d_timestamp,
                                            mqbs::ConfirmReason::e_CONFIRMED);
                if (0 != rc) {
                    bsl::cout << "Error writing ConfirmRecord, rc: " << rc
                              << bsl::endl;
                    return false;  // RETURN
                }

                guids.pop_back();

                if (guids.empty()) {
                    queueKeyGuidsMap.erase(it);
                }

                records->push_back(bsl::make_pair(handle, rec));
                ++(*seqNum);
                ++(*numRecordsWritten);

                // Add the guid to the list of confirmed guids for the queue
                // key, so that we can later use this guid to delete.  Note
                // that the choice of using operator[] on the
                // 'queueKeyConfGuidsMap' is deliberate, as an entry for the
                // queue key may or may not exist.

                queueKeyConfGuidsMap[rec.d_queueKey].push_back(rec.d_guid);

                continue;  // CONTINUE
            }

            if (3 == recType) {
                if (i < 20) {
                    // No need to start writing deletion records immediately.

                    continue;  // CONTINUE
                }

                // Write a deletion record.  Randomly retrieve a queue key from
                // 'queueKeyConfGuidsMap' (this queue key *must* exist in
                // 'queueKeyUriMap').  Then take the last guid from the list of
                // confirmed guids associated with this pair, write its
                // deletion record, remove it from that list.  If that list is
                // now empty, delete that queue key entry from
                // 'queueKeyConfGuidsMap'.

                size_t offset = bdlb::Random::generate15(&seed) %
                                queueKeyConfGuidsMap.size();

                QueueKeyGuidsMapIter it = queueKeyConfGuidsMap.begin();
                bsl::advance(it, offset);

                BSLS_ASSERT(!it->first.isNull());
                BSLS_ASSERT(!it->second.empty());
                BSLS_ASSERT(queueKeyUriMap.end() !=
                            queueKeyUriMap.find(it->first));

                Guids& guids = it->second;
                BSLS_ASSERT(!guids.empty());

                Record rec(bmqtst::TestHelperUtil::allocator());
                rec.d_recordType = mqbs::RecordType::e_DELETION;
                rec.d_guid       = guids.back();
                rec.d_queueKey   = it->first;
                rec.d_deletionRecordFlag =
                    (i % 2 == 0 ? mqbs::DeletionRecordFlag::e_NONE
                                : mqbs::DeletionRecordFlag::e_TTL_EXPIRATION);
                rec.d_timestamp = bdlt::EpochUtil::convertToTimeT64(
                    bdlt::CurrentTime::utc());

                rc = fs->writeDeletionRecord(rec.d_guid,
                                             rec.d_queueKey,
                                             rec.d_deletionRecordFlag,
                                             rec.d_timestamp);
                if (0 != rc) {
                    bsl::cout << "Error writing DeletionRecord, rc: " << rc
                              << bsl::endl;
                    return false;  // RETURN
                }

                guids.pop_back();

                if (guids.empty()) {
                    queueKeyConfGuidsMap.erase(it);
                }

                // TBD: We don't have a way to remove records from the
                // FileStore associated with this queue key.

                ++(*seqNum);

                continue;  // CONTINUE
            }

            if (4 == recType) {
                // Write a SyncPt.

                mqbs::FileStoreSet fileSet(
                    bmqtst::TestHelperUtil::allocator());
                fs->loadCurrentFiles(&fileSet);

                BSLS_ASSERT((fileSet.dataFileSize() %
                             bmqp::Protocol::k_DWORD_SIZE) == 0);
                BSLS_ASSERT((fileSet.qlistFileSize() %
                             bmqp::Protocol::k_WORD_SIZE) == 0);

                rc = fs->issueSyncPoint();

                if (rc) {
                    bsl::cout << "Error writing SyncPt, rc: " << rc
                              << bsl::endl;
                    return false;  // RETURN
                }

                Record rec(bmqtst::TestHelperUtil::allocator());
                rec.d_recordType    = mqbs::RecordType::e_JOURNAL_OP;
                rec.d_journalOpType = mqbs::JournalOpType::e_SYNCPOINT;
                rec.d_syncPtType    = mqbs::SyncPointType::e_REGULAR;

                rec.d_syncPoint.primaryLeaseId() = leaseId;
                rec.d_syncPoint.sequenceNum()    = ++(*seqNum);
                rec.d_syncPoint.dataFileOffsetDwords() =
                    fileSet.dataFileSize() / bmqp::Protocol::k_DWORD_SIZE;
                rec.d_syncPoint.qlistFileOffsetWords() =
                    fileSet.qlistFileSize() / bmqp::Protocol::k_WORD_SIZE;

                bmqp_ctrlmsg::SyncPointOffsetPair spoPair;
                spoPair.syncPoint() = rec.d_syncPoint;
                spoPair.offset()    = fileSet.journalFileSize();
                spOffsetPairs->push_back(spoPair);
                records->push_back(
                    bsl::make_pair(mqbs::DataStoreRecordHandle(), rec));

                continue;  // CONTINUE
            }

            if (5 == recType) {
                if (i < 100) {
                    // No need to write QueuePurge record too soon.

                    continue;  // CONTINUE
                }

                // Write a QueuePurge record.  Randomly retrieve a queue key
                // from 'queueKeyUriMap'.  Then write a QueuePurge record for
                // this queue key, and then remove entries for this queue key
                // from 'queueKeyGuidsMap' as well as 'queueKeyConfGuidsMap' if
                // they exist.

                size_t offset = bdlb::Random::generate15(&seed) %
                                queueKeyUriMap.size();

                QueueKeyUriMapIter it = queueKeyUriMap.begin();
                bsl::advance(it, offset);

                BSLS_ASSERT(!it->first.isNull());
                BSLS_ASSERT(!it->second.empty());

                mqbs::DataStoreRecordHandle handle;
                Record rec(bmqtst::TestHelperUtil::allocator());
                rec.d_recordType  = mqbs::RecordType::e_QUEUE_OP;
                rec.d_queueOpType = mqbs::QueueOpType::e_PURGE;
                rec.d_queueKey    = it->first;
                rec.d_timestamp   = bdlt::EpochUtil::convertToTimeT64(
                    bdlt::CurrentTime::utc());

                rc = fs->writeQueuePurgeRecord(&handle,
                                               rec.d_queueKey,
                                               mqbu::StorageKey(),
                                               rec.d_timestamp,
                                               mqbs::DataStoreRecordHandle());
                if (0 != rc) {
                    bsl::cout << "Error writing QueuePurgeRecord, rc: " << rc
                              << bsl::endl;
                    return false;  // RETURN
                }

                // TBD: We don't have a way to remove records from the
                // FileStore associated with this queue key.

                records->push_back(bsl::make_pair(handle, rec));
                ++(*seqNum);
                ++(*numRecordsWritten);

                queueKeyGuidsMap.erase(it->first);
                queueKeyConfGuidsMap.erase(it->first);

                continue;  // CONTINUE
            }

            if (6 == recType) {
                if (i < 200) {
                    // No need to write QueueDeletion record too soon.

                    continue;  // CONTINUE
                }

                // Write a QueueDeletion record, if applicable.  Randomly
                // retrieve a queue key from 'queueKeyUriMap'.  If there is no
                // entry for the pair's index in 'queueIndexGuidsMap' *and*
                // 'queueIndexConfGuidsMap', write a QueueDeletion record for
                // this pair, and also remove this entry from 'queueKeyUriMap',
                // else simply continue.

                size_t offset = bdlb::Random::generate15(&seed) %
                                queueKeyUriMap.size();

                QueueKeyUriMapIter it = queueKeyUriMap.begin();
                bsl::advance(it, offset);

                BSLS_ASSERT(!it->first.isNull());
                BSLS_ASSERT(!it->second.empty());

                if (0 != queueKeyGuidsMap.count(it->first) ||
                    0 != queueKeyConfGuidsMap.count(it->first)) {
                    continue;  // CONTINUE
                }

                mqbs::DataStoreRecordHandle handle;
                Record rec(bmqtst::TestHelperUtil::allocator());
                rec.d_recordType  = mqbs::RecordType::e_QUEUE_OP;
                rec.d_queueOpType = mqbs::QueueOpType::e_DELETION;
                rec.d_queueKey    = it->first;
                rec.d_timestamp   = bdlt::EpochUtil::convertToTimeT64(
                    bdlt::CurrentTime::utc());

                rc = fs->writeQueueDeletionRecord(&handle,
                                                  rec.d_queueKey,
                                                  mqbu::StorageKey(),
                                                  rec.d_timestamp);
                if (0 != rc) {
                    bsl::cout
                        << "Error writing QueueDeletionRecord, rc: " << rc
                        << bsl::endl;
                    return false;  // RETURN
                }

                // TBD: We don't have a way to remove records from the
                // FileStore associated with this queue key.

                records->push_back(bsl::make_pair(handle, rec));
                ++(*seqNum);
                ++(*numRecordsWritten);

                continue;  // CONTINUE
            }
        }

        return true;
    }

    // ACCESSORS
    mqbs::FileStore& fileStore() const { return *(d_fs_mp); }

    mqbnet::ClusterNode* node() const { return d_node_p; }

    bdlmt::FixedThreadPool& miscWorkThreadPool()
    {
        return d_miscWorkThreadPool;
    }

    mqbmock::Dispatcher& dispatcher() { return d_dispatcher; }

    bdlmt::EventScheduler& scheduler() { return d_scheduler; }

    bdlbb::BlobBufferFactory& bufferFactory() { return d_bufferFactory; }
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

    Tester             tester("./test-cluster123-1");
    mqbs::FileStore&   fs             = tester.fileStore();
    const unsigned int primaryLeaseId = 1U;

    int rc = fs.open(0, primaryLeaseId);
    BMQTST_ASSERT_EQ(0, rc);
    if (rc) {
        cout << "Failed to open partition, rc: " << rc << endl;
        return;  // RETURN
    }

    BMQTST_ASSERT_EQ(true, fs.isOpen());
    BMQTST_ASSERT_EQ(1U, fs.clusterSize());
    BMQTST_ASSERT_EQ(0ULL, fs.numRecords());
    BMQTST_ASSERT_EQ(true, fs.syncPoints().empty());
    BMQTST_ASSERT_EQ(0U, fs.writeHeadLeaseId());
    BMQTST_ASSERT_EQ(0ULL, fs.writeHeadSeqNum());

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.  TBD: figure
    // out the right way to "fix" this.

    Record dummy(bmqtst::TestHelperUtil::allocator());
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Record, bslma::UsesBslmaAllocator> >(
            dummy));

    fs.setActivePrimary(tester.node(), primaryLeaseId);

    bsls::Types::Uint64 seqNum = 1;
    BMQTST_ASSERT_EQ(primaryLeaseId, fs.writeHeadLeaseId());
    BMQTST_ASSERT_EQ(seqNum, fs.writeHeadSeqNum());
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
    bool                success           = tester.writeRecords(&fs,
                                       &records,
                                       &spOffsetPairs,
                                       primaryLeaseId,
                                       &seqNum,
                                       &numRecordsWritten,
                                       k_NUM_RECORDS);

    BMQTST_ASSERT_EQ(true, success);
    if (!success) {
        fs.close();
        return;  // RETURN
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
    BMQTST_ASSERT_EQ(numRecordsWritten, fs.numRecords());

    mqbs::FileStoreIterator fsIt(&fs);
    while (fsIt.next()) {
        // TBD: verify
    }

    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);

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

    Tester             tester("./test-cluster123-2");
    mqbs::FileStore&   fs             = tester.fileStore();
    const unsigned int primaryLeaseId = 1;

    int rc = fs.open(0, primaryLeaseId);
    BSLS_ASSERT_OPT(rc == 0);
    fs.setActivePrimary(tester.node(), primaryLeaseId);

    // Write various records to the partition.
    SyncPointOffsetPairs spOffsetPairs(bmqtst::TestHelperUtil::allocator());
    bsl::vector<HandleRecordPair> records(bmqtst::TestHelperUtil::allocator());

    const size_t        k_NUM_RECORDS     = 10;
    bsls::Types::Uint64 numRecordsWritten = 0;
    bsls::Types::Uint64 seqNum            = 1;
    BSLS_ASSERT_OPT(tester.writeRecords(&fs,
                                        &records,
                                        &spOffsetPairs,
                                        primaryLeaseId,
                                        &seqNum,
                                        &numRecordsWritten,
                                        k_NUM_RECORDS));

    bdlpcre::RegEx expectedOut(bmqtst::TestHelperUtil::allocator());
    bsl::string    errorMessage(bmqtst::TestHelperUtil::allocator());
    size_t         errorOffset = 0;
    expectedOut.prepare(
        &errorMessage,
        &errorOffset,
        "\\[ queueOpRecord = \\[ header = \\[ type = QUEUE_OP flags = 0 "
        "primaryLeaseId = 1 sequenceNumber = 2 timestamp = [0-9]* ] flags = 0 "
        "queueKey = 3078787878 appKey = 0000000000 type = CREATION "
        "queueUriRecordOffsetWords = 9 ] ]\\n"

        "\\[ messageRecord = \\[ header = \\[ type = MESSAGE flags = 1 "
        "primaryLeaseId = 1 sequenceNumber = 3 timestamp = [0-9]* ] refCount "
        "= "
        "1 queueKey = 3078787878 fileKey = 0000000000 messageOffsetDwords = 5 "
        "messageGUID = [0-9|A-Z]* crc32c = [0-9]* compressionAlgorithmType = "
        "NONE ] ]\\n"

        "\\[ confirmRecord = \\[ header = \\[ type = CONFIRM flags = 0 "
        "primaryLeaseId = 1 sequenceNumber = 4 timestamp = [0-9]* ] "
        "reason = CONFIRMED queueKey = 3078787878 appKey = 0000000000 "
        "messageGUID = [0-9|A-Z]* ] ]\\n"

        "\\[ queueOpRecord = \\[ header = \\[ type = QUEUE_OP flags = 0 "
        "primaryLeaseId = 1 sequenceNumber = 6 timestamp = [0-9]* ] flags = 0 "
        "queueKey = 3778787878 appKey = 0000000000 type = CREATION "
        "queueUriRecordOffsetWords = 27 ] ]\\n"

        "\\[ messageRecord = \\[ header = \\[ type = MESSAGE flags = 8 "
        "primaryLeaseId = 1 sequenceNumber = 7 timestamp = [0-9]* ] refCount "
        "= "
        "8 queueKey = 3778787878 fileKey = 0000000000 messageOffsetDwords = 8 "
        "messageGUID = [0-9|A-Z]* crc32c = [0-9]* compressionAlgorithmType = "
        "NONE ] ]\\n",
        bdlpcre::RegEx::k_FLAG_MULTILINE);
    BSLS_ASSERT_OPT(expectedOut.isPrepared());

    mqbs::FileStoreIterator fsIt(&fs);
    bmqu::MemOutStream      stream(bmqtst::TestHelperUtil::allocator());
    while (fsIt.next()) {
        stream << fsIt << "\n";
    }
    BMQTST_ASSERT_EQ(expectedOut.match(stream.str().data(),
                                       stream.str().length()),
                     0);

    PV("Bad stream test");
    stream.reset();
    stream << "INVALID";
    stream.clear(bsl::ios_base::badbit);
    stream << fsIt;
    BMQTST_ASSERT_EQ(stream.str(), "INVALID");

    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);
}

static void test3_partitionFullAlarm()
// ------------------------------------------------------------------------
// PARTITION FULL ALARM
//
// Concerns:
//   Verify that writing records to the journal until it is full triggers
//   a partition-full alarm (rollover failure), and that purging records
//   afterwards decreases the outstanding byte count.
//
// Testing:
//   writeQueueCreationRecord, writeMessageRecord, removeRecordRaw
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    Tester           tester("./test-cluster123-3");
    mqbs::FileStore& fs             = tester.fileStore();
    unsigned int     primaryLeaseId = 1;

    // Disable in-place callback execution in mock dispatcher to prevent
    // thread races between the main thread (that modifies FileStore) and
    // scheduler thread (that performs gc on FileStore).
    tester.dispatcher().setEnqueueOnly(true);

    int rc = fs.open(0, primaryLeaseId);
    BMQTST_ASSERT_EQ(0, rc);
    if (rc) {
        cout << "Failed to open partition, rc: " << rc << endl;
        return;  // RETURN
    }

    fs.setActivePrimary(tester.node(), primaryLeaseId);

    // Create a storage and register it with the FileStore.
    bmqt::Uri        queueUri("bmq://si.amw.bmq.stats/testQueue",
                       bmqtst::TestHelperUtil::allocator());
    mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                              "ABCDE");

    mqbmock::Cluster mockCluster(bmqtst::TestHelperUtil::allocator());
    mqbmock::Domain  mockDomain(&mockCluster,
                               bmqtst::TestHelperUtil::allocator());
    mqbconfm::Domain domainCfg(bmqtst::TestHelperUtil::allocator());
    domainCfg.messageTtl() = bsl::numeric_limits<bsls::Types::Int64>::max();
    domainCfg.storage().config().makeFileBacked();
    bmqu::MemOutStream errDesc(bmqtst::TestHelperUtil::allocator());
    mockDomain.configure(errDesc, domainCfg);

    bsl::shared_ptr<mqbs::ReplicatedStorage> storage_sp;
    fs.createStorage(&storage_sp, queueUri, queueKey, &mockDomain);

    mqbconfm::Limits limits;
    limits.messages() = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.bytes()    = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.messagesWatermarkRatio() = 0.8;
    limits.bytesWatermarkRatio()    = 0.8;
    storage_sp->configure(domainCfg.storage().config(),
                          limits,
                          domainCfg.messageTtl(),
                          0);  // maxDeliveryAttempts

    fs.registerStorage(storage_sp.get());

    mqbmock::Queue mockQueue(&mockDomain, bmqtst::TestHelperUtil::allocator());
    storage_sp->setQueue(&mockQueue);

    // 1. Create a queue.
    mqbs::DataStoreRecordHandle queueHandle;
    bsls::Types::Uint64         timestamp = bdlt::EpochUtil::convertToTimeT64(
        bdlt::CurrentTime::utc());

    rc = fs.writeQueueCreationRecord(&queueHandle,
                                     queueUri,
                                     queueKey,
                                     AppInfos(),
                                     timestamp,
                                     true);  // isNewQueue
    BMQTST_ASSERT_EQ(0, rc);

    // 2. Write message records until the journal is full.
    StoragePoster poster(storage_sp, bmqtst::TestHelperUtil::allocator());

    const size_t k_MAX_ITERATIONS = 20000;
    size_t       numWritten       = 0;
    int          failedRc         = 0;

    for (size_t i = 0; i < k_MAX_ITERATIONS; ++i) {
        mqbi::StorageResult::Enum putRc = poster.postMessage();
        if (putRc != mqbi::StorageResult::e_SUCCESS) {
            failedRc = static_cast<int>(putRc);
            break;
        }

        ++numWritten;
    }

    BMQTST_ASSERT_D("journal should have filled up", failedRc != 0);
    BMQTST_ASSERT_D("should have written some records before failure",
                    numWritten > 0);

    const bsls::Types::Uint64 numRecordsBeforePurge = fs.numRecords();
    BMQTST_ASSERT_D("records should exist before purge",
                    numRecordsBeforePurge > 0);

    // 3. Verify subsequent writes also fail (journal unavailable).
    BMQTST_ASSERT_D("write after full should fail",
                    poster.postMessage() != mqbi::StorageResult::e_SUCCESS);

    // 4. Purge the queue via storage.
    storage_sp->removeAll(mqbu::StorageKey());
    tester.dispatcher().processQueue();

    // 5. Verify the number of records is small after purge + rollover.
    BMQTST_ASSERT_D("records after purge should be < 1% of pre-purge",
                    fs.numRecords() < numRecordsBeforePurge / 100);

    // 6. Verify writes succeed after purge.
    BMQTST_ASSERT_D("write after purge should succeed",
                    poster.postMessage() == mqbi::StorageResult::e_SUCCESS);

    // 7. Close and reopen the partition.  Wait for background work from
    //    the rollover (gcWorkerDispatched, deleteArchiveFilesCb) to finish.
    tester.miscWorkThreadPool().drain();
    tester.scheduler().cancelAllEventsAndWait();
    tester.dispatcher().processQueue();
    fs.unregisterStorage(storage_sp.get());
    fs.close();
    BMQTST_ASSERT_EQ(false, fs.isOpen());

    rc = fs.open(0, 2);
    BMQTST_ASSERT_EQ(0, rc);
    BMQTST_ASSERT_EQ(true, fs.isOpen());

    // 8. Verify writes succeed after reopen.
    fs.registerStorage(storage_sp.get());
    fs.setActivePrimary(tester.node(), ++primaryLeaseId);
    BMQTST_ASSERT_D("write after reopen should succeed",
                    poster.postMessage() == mqbi::StorageResult::e_SUCCESS);

    fs.unregisterStorage(storage_sp.get());
    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);
}

static void test4_recoverMessagesAcrossLeaseIds()
// ------------------------------------------------------------------------
// RECOVER MESSAGES ACROSS MULTIPLE PRIMARY LEASE IDS
//
// Concerns:
//   In the FSM workflow a live partition can be sent back through recovery,
//   re-opening the same FileStore in recovery mode.  Verify that recovering
//   a FileStore whose journal spans multiple primary leaseIds succeeds when
//   its 'd_highestSeqNums' already holds entries for those leaseIds (as it
//   does once the partition has been live under several primary terms).
//
// Testing:
//   recoverMessages (re-recovery across leaseIds)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    Tester           tester("./test-cluster123-4");
    mqbs::FileStore& fs             = tester.fileStore();
    unsigned int     primaryLeaseId = 1;

    // Disable in-place callback execution in mock dispatcher to prevent
    // thread races between the main thread (that modifies FileStore) and
    // scheduler thread (that performs gc on FileStore).
    tester.dispatcher().setEnqueueOnly(true);

    int rc = fs.open(0, primaryLeaseId);
    BMQTST_ASSERT_EQ(0, rc);
    if (rc) {
        cout << "Failed to open partition, rc: " << rc << endl;
        return;  // RETURN
    }

    fs.setActivePrimary(tester.node(), primaryLeaseId);

    // Create a storage and register it with the FileStore.
    bmqt::Uri        queueUri("bmq://si.amw.bmq.stats/testQueue",
                       bmqtst::TestHelperUtil::allocator());
    mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                              "ABCDE");

    mqbmock::Cluster mockCluster(bmqtst::TestHelperUtil::allocator());
    mqbmock::Domain  mockDomain(&mockCluster,
                               bmqtst::TestHelperUtil::allocator());
    mqbconfm::Domain domainCfg(bmqtst::TestHelperUtil::allocator());
    domainCfg.messageTtl() = bsl::numeric_limits<bsls::Types::Int64>::max();
    domainCfg.storage().config().makeFileBacked();
    bmqu::MemOutStream errDesc(bmqtst::TestHelperUtil::allocator());
    mockDomain.configure(errDesc, domainCfg);

    bsl::shared_ptr<mqbs::ReplicatedStorage> storage_sp;
    fs.createStorage(&storage_sp, queueUri, queueKey, &mockDomain);

    mqbconfm::Limits limits;
    limits.messages() = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.bytes()    = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.messagesWatermarkRatio() = 0.8;
    limits.bytesWatermarkRatio()    = 0.8;
    storage_sp->configure(domainCfg.storage().config(),
                          limits,
                          domainCfg.messageTtl(),
                          0);  // maxDeliveryAttempts

    fs.registerStorage(storage_sp.get());

    mqbmock::Queue mockQueue(&mockDomain, bmqtst::TestHelperUtil::allocator());
    storage_sp->setQueue(&mockQueue);

    // Write the queue creation record and a few messages under leaseId 1.
    mqbs::DataStoreRecordHandle queueHandle;
    bsls::Types::Uint64         timestamp = bdlt::EpochUtil::convertToTimeT64(
        bdlt::CurrentTime::utc());
    rc = fs.writeQueueCreationRecord(&queueHandle,
                                     queueUri,
                                     queueKey,
                                     AppInfos(),
                                     timestamp,
                                     true);  // isNewQueue
    BMQTST_ASSERT_EQ(0, rc);

    StoragePoster poster(storage_sp, bmqtst::TestHelperUtil::allocator());
    const size_t  k_NUM_MSGS_PER_LEASE = 5;
    for (size_t i = 0; i < k_NUM_MSGS_PER_LEASE; ++i) {
        BMQTST_ASSERT_EQ(poster.postMessage(), mqbi::StorageResult::e_SUCCESS);
    }

    // Bump the primary leaseId and write more messages, so that the journal
    // spans two distinct leaseIds and 'd_highestSeqNums' holds an entry for
    // each.
    fs.setActivePrimary(tester.node(), ++primaryLeaseId);
    for (size_t i = 0; i < k_NUM_MSGS_PER_LEASE; ++i) {
        BMQTST_ASSERT_EQ(poster.postMessage(), mqbi::StorageResult::e_SUCCESS);
    }

    const bsls::Types::Uint64 numRecords = fs.numRecords();
    BMQTST_ASSERT_D("records should exist before reopen", numRecords > 0);

    // Close and reopen.  The reopen re-runs 'recoverMessages' on the same
    // FileStore, whose 'd_highestSeqNums' still holds entries for both
    // leaseIds.
    tester.miscWorkThreadPool().drain();
    tester.scheduler().cancelAllEventsAndWait();
    tester.dispatcher().processQueue();
    fs.unregisterStorage(storage_sp.get());
    fs.close();
    BMQTST_ASSERT_EQ(false, fs.isOpen());

    rc = fs.open(0, primaryLeaseId);
    BMQTST_ASSERT_EQ(0, rc);
    BMQTST_ASSERT_EQ(true, fs.isOpen());

    // All records from both leaseIds should have been recovered.
    BMQTST_ASSERT_EQ(fs.numRecords(), numRecords);

    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);
}

static void test5_writeHeadFollowsAppliedLease()
// ------------------------------------------------------------------------
// WRITE HEAD FOLLOWS APPLIED LEASE ID
//
// Concerns:
//   When a replica applies a record whose primary leaseId is higher than its
//   current write head via a partition-sync event, the write head must advance
//   to that leaseId (together with the sequence number).
//
// Testing:
//   processStorageEvent
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    Tester           tester("./test-cluster123-5");
    mqbs::FileStore& fs             = tester.fileStore();
    unsigned int     primaryLeaseId = 1;

    int rc = fs.open(0, primaryLeaseId);
    BMQTST_ASSERT_EQ(0, rc);

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool =
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator());

    const int          k_PARTITION_ID   = 0;
    const unsigned int dataOffsetDwords = k_SIZEOF_HEADERS_DATA_FILE /
                                          bmqp::Protocol::k_DWORD_SIZE;
    const unsigned int qlistOffsetWords = k_SIZEOF_HEADERS_QLIST_FILE /
                                          bmqp::Protocol::k_WORD_SIZE;

    unsigned int journalOffsetWords = k_SIZEOF_HEADERS_JOURNAL_FILE /
                                      bmqp::Protocol::k_WORD_SIZE;
    const unsigned int k_RECORD_WORDS =
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE /
        bmqp::Protocol::k_WORD_SIZE;

    fs.setActivePrimary(tester.node(), primaryLeaseId);
    // A sync point is issued -> [1, 1]
    journalOffsetWords += k_RECORD_WORDS;
    BMQTST_ASSERT_EQ(1U, fs.writeHeadLeaseId());
    BMQTST_ASSERT_EQ(1ULL, fs.writeHeadSeqNum());

    applyReplicatedSyncPoint(&fs,
                             blobSpPool.get(),
                             &bufferFactory,
                             tester.node(),
                             k_PARTITION_ID,
                             primaryLeaseId,
                             2,  // seqNum
                             journalOffsetWords,
                             dataOffsetDwords,
                             qlistOffsetWords,
                             bmqtst::TestHelperUtil::allocator());
    journalOffsetWords += k_RECORD_WORDS;
    BMQTST_ASSERT_EQ(primaryLeaseId, fs.writeHeadLeaseId());
    BMQTST_ASSERT_EQ(2ULL, fs.writeHeadSeqNum());

    primaryLeaseId = 2;
    for (bsls::Types::Uint64 seqNum = 1; seqNum <= 4; ++seqNum) {
        applyReplicatedSyncPoint(&fs,
                                 blobSpPool.get(),
                                 &bufferFactory,
                                 tester.node(),
                                 k_PARTITION_ID,
                                 primaryLeaseId,
                                 seqNum,
                                 journalOffsetWords,
                                 dataOffsetDwords,
                                 qlistOffsetWords,
                                 bmqtst::TestHelperUtil::allocator());
        journalOffsetWords += k_RECORD_WORDS;
        BMQTST_ASSERT_EQ(primaryLeaseId, fs.writeHeadLeaseId());
        BMQTST_ASSERT_EQ(seqNum, fs.writeHeadSeqNum());
    }

    // The delayed 'setActivePrimary' for leaseId 2 now arrives.  Verify it
    // does not reset the sequence number of leaseId 2 back to zero.
    fs.setActivePrimary(tester.node(), primaryLeaseId);
    // A sync point is issued -> [2, 5]
    BMQTST_ASSERT_EQ(primaryLeaseId, fs.writeHeadLeaseId());
    BMQTST_ASSERT_GE(5ULL, fs.writeHeadSeqNum());
    BMQTST_ASSERT_EQ(tester.node(), fs.primaryNode());

    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);
}

static void test6_leaseTransitionWithoutSeal()
// ------------------------------------------------------------------------
// LEASE TRANSITION WITHOUT OLD-LEASE SYNC POINT
//
// Concerns:
//   'setActivePrimary' does not write a sync point on behalf of the previous
//   primary at the primary-switch boundary.  Verify that a journal whose
//   lease transition is *not* separated by an old-lease sync point (the last
//   record of the previous lease is a non-sync-point record) recovers
//   cleanly when the FileStore is closed and re-opened.
//
// Testing:
//   setActivePrimary (no old-lease seal), openInRecoveryMode
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    Tester           tester("./test-cluster123-6");
    mqbs::FileStore& fs             = tester.fileStore();
    unsigned int     primaryLeaseId = 1;

    int rc = fs.open(0, primaryLeaseId);
    BMQTST_ASSERT_EQ(0, rc);

    // Set primary with leaseId 1; a sync point is issued -> [1, 1].
    fs.setActivePrimary(tester.node(), primaryLeaseId);
    BMQTST_ASSERT_EQ(primaryLeaseId, fs.writeHeadLeaseId());
    BMQTST_ASSERT_EQ(1ULL, fs.writeHeadSeqNum());

    // Create a storage and register it with the FileStore.
    bmqt::Uri        queueUri("bmq://si.amw.bmq.stats/testQueue",
                       bmqtst::TestHelperUtil::allocator());
    mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                              "ABCDE");

    mqbmock::Cluster mockCluster(bmqtst::TestHelperUtil::allocator());
    mqbmock::Domain  mockDomain(&mockCluster,
                               bmqtst::TestHelperUtil::allocator());
    mqbconfm::Domain domainCfg(bmqtst::TestHelperUtil::allocator());
    domainCfg.messageTtl() = bsl::numeric_limits<bsls::Types::Int64>::max();
    domainCfg.storage().config().makeFileBacked();
    bmqu::MemOutStream errDesc(bmqtst::TestHelperUtil::allocator());
    mockDomain.configure(errDesc, domainCfg);

    bsl::shared_ptr<mqbs::ReplicatedStorage> storage_sp;
    fs.createStorage(&storage_sp, queueUri, queueKey, &mockDomain);

    mqbconfm::Limits limits;
    limits.messages() = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.bytes()    = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.messagesWatermarkRatio() = 0.8;
    limits.bytesWatermarkRatio()    = 0.8;
    storage_sp->configure(domainCfg.storage().config(),
                          limits,
                          domainCfg.messageTtl(),
                          0);  // maxDeliveryAttempts

    fs.registerStorage(storage_sp.get());

    mqbmock::Queue mockQueue(&mockDomain, bmqtst::TestHelperUtil::allocator());
    storage_sp->setQueue(&mockQueue);

    // Write non-sync-point records under leaseId 1.
    mqbs::DataStoreRecordHandle queueHandle;
    bsls::Types::Uint64         timestamp = bdlt::EpochUtil::convertToTimeT64(
        bdlt::CurrentTime::utc());
    rc = fs.writeQueueCreationRecord(&queueHandle,
                                     queueUri,
                                     queueKey,
                                     AppInfos(),
                                     timestamp,
                                     true);  // isNewQueue
    BMQTST_ASSERT_EQ(0, rc);

    StoragePoster poster(storage_sp, bmqtst::TestHelperUtil::allocator());
    for (size_t i = 0; i < 3; ++i) {
        BMQTST_ASSERT_EQ(poster.postMessage(), mqbi::StorageResult::e_SUCCESS);
    }
    BMQTST_ASSERT_EQ(1U, fs.writeHeadLeaseId());
    BMQTST_ASSERT_EQ(5ULL, fs.writeHeadSeqNum());

    // Bump the primary leaseId to 2.  A single sync point [2, 1] is issued for
    // the new lease.
    primaryLeaseId = 2;
    fs.setActivePrimary(tester.node(), primaryLeaseId);
    BMQTST_ASSERT_EQ(primaryLeaseId, fs.writeHeadLeaseId());
    BMQTST_ASSERT_EQ(1ULL, fs.writeHeadSeqNum());

    // Write a few more message records under leaseId 2.
    for (size_t i = 0; i < 3; ++i) {
        BMQTST_ASSERT_EQ(poster.postMessage(), mqbi::StorageResult::e_SUCCESS);
    }
    BMQTST_ASSERT_EQ(primaryLeaseId, fs.writeHeadLeaseId());

    const bsls::Types::Uint64 numRecords = fs.numRecords();
    BMQTST_ASSERT_D("records should exist before reopen", numRecords > 0);

    // Close and reopen
    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);

    rc = fs.open(0, primaryLeaseId);
    BMQTST_ASSERT_EQ(0, rc);
    BMQTST_ASSERT_EQ(true, fs.isOpen());

    // Every record from both leaseIds should have been recovered.
    BMQTST_ASSERT_EQ(fs.numRecords(), numRecords);

    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);
}

static void test7_conformExtraQueueWithoutRollover()
// ------------------------------------------------------------------------
// CONFORM EXTRA QUEUE WITHOUT ROLLOVER
//
// Concerns:
//   When the primary conforms its journal to the cluster state and the
//   corrective QueueOp.DELETION fits without rolling over, it is appended in
//   place (no rollover) to mark the extra queue deleted.
//
// Testing:
//   Corrective QueueOp.DELETION during conform without rollover
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CONFORM EXTRA QUEUE WITHOUT ROLLOVER");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    const char k_FILE_STORE_LOCATION[] = "./test-cluster123-6";

    Tester           tester(k_FILE_STORE_LOCATION);
    mqbs::FileStore& fs = tester.fileStore();

    const unsigned int k_OLD_LEASE = 1;

    // --- Phase 1: write a journal containing queue 'extraQ'.
    int rc = fs.open(0, k_OLD_LEASE);
    BMQTST_ASSERT_EQ(0, rc);
    BMQTST_ASSERT_EQ(true, fs.isOpen());

    fs.setActivePrimary(tester.node(), k_OLD_LEASE);

    const bmqt::Uri        queueUri("bmq://si.amw.bmq.stats/extraQ",
                             bmqtst::TestHelperUtil::allocator());
    const mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                                    "ABCDE");
    mqbs::DataStoreRecordHandle queueHandle;
    BMQTST_ASSERT_EQ(
        0,
        fs.writeQueueCreationRecord(
            &queueHandle,
            queueUri,
            queueKey,
            AppInfos(),
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()),
            true));  // isNewQueue

    mqbs::FileStoreSet fileSet(bmqtst::TestHelperUtil::allocator());
    fs.loadCurrentFiles(&fileSet);
    const bsl::string journalFileBefore(fileSet.journalFile(),
                                        bmqtst::TestHelperUtil::allocator());

    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);
    BMQTST_ASSERT_EQ(false, fs.isOpen());

    // --- Phase 2: reopen as the new primary with an empty cluster state.
    // 'extraQ' is extra and the corrective QueueOp.DELETION fits, so it is
    // appended in place without rolling over.
    const unsigned int                     k_NEW_LEASE = k_OLD_LEASE + 1;
    mqbs::DataStoreConfig::QueueKeyInfoMap clusterState(
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(0, fs.open(&clusterState, k_NEW_LEASE));
    BMQTST_ASSERT_EQ(true, fs.isOpen());

    fs.loadCurrentFiles(&fileSet);

    // No rollover: same journal file.
    BMQTST_ASSERT_EQ(journalFileBefore, fileSet.journalFile());

    // Exactly one corrective QueueOp.DELETION was appended.
    BMQTST_ASSERT_EQ(1, journalDeletionCount(fileSet.journalFile()));

    // Write head reflects the new lease at (2, 1).
    BMQTST_ASSERT_EQ(k_NEW_LEASE, fs.writeHeadLeaseId());
    BMQTST_ASSERT_EQ(1ULL, fs.writeHeadSeqNum());

    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);
}

static void test8_conformExtraQueueCausesRollover()
// ------------------------------------------------------------------------
// CONFORM EXTRA QUEUE CAUSES ROLLOVER
//
// Concerns:
//   When the primary conforms its journal to the cluster state and the
//   corrective QueueOp.DELETIONs do not fit on a near-full journal, conform
//   rolls over once.  Rollover compacts away every extra queue, so no
//   corrective DELETION is written.
//
// Testing:
//   Corrective QueueOp.DELETION during conform causes rollover
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CONFORM EXTRA QUEUE CAUSES ROLLOVER");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    const char k_FILE_STORE_LOCATION[] = "./test-cluster123-4";

    const bsls::Types::Uint64 k_MAX_JOURNAL = 8 * 1024;

    const bsls::Types::Uint64 k_REQUESTED_JOURNAL_SPACE =
        static_cast<bsls::Types::Uint64>(
            mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE) *
        (1 + 2 + 16);

    Tester           tester(k_FILE_STORE_LOCATION, k_MAX_JOURNAL);
    mqbs::FileStore& fs = tester.fileStore();
    tester.dispatcher().setEnqueueOnly(true);

    const unsigned int k_OLD_LEASE = 1;

    // --- Phase 1: build a near-full journal containing queues 'extraQ' and
    // 'extraQ2'.
    int rc = fs.open(0, k_OLD_LEASE);
    BMQTST_ASSERT_EQ(0, rc);
    BMQTST_ASSERT_EQ(true, fs.isOpen());

    fs.setActivePrimary(tester.node(), k_OLD_LEASE);

    const bmqt::Uri        queueUri("bmq://si.amw.bmq.stats/extraQ",
                             bmqtst::TestHelperUtil::allocator());
    const mqbu::StorageKey queueKey(mqbu::StorageKey::BinaryRepresentation(),
                                    "ABCDE");
    mqbs::DataStoreRecordHandle queueHandle;
    BMQTST_ASSERT_EQ(
        0,
        fs.writeQueueCreationRecord(
            &queueHandle,
            queueUri,
            queueKey,
            AppInfos(),
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()),
            true));  // isNewQueue

    // A second extra queue: both extra queues are compacted away together by
    // the single rollover, so neither yields a corrective DELETION.
    const bmqt::Uri        queueUri2("bmq://si.amw.bmq.stats/extraQ2",
                              bmqtst::TestHelperUtil::allocator());
    const mqbu::StorageKey queueKey2(mqbu::StorageKey::BinaryRepresentation(),
                                     "FGHIJ");
    mqbs::DataStoreRecordHandle queueHandle2;
    BMQTST_ASSERT_EQ(
        0,
        fs.writeQueueCreationRecord(
            &queueHandle2,
            queueUri2,
            queueKey2,
            AppInfos(),
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()),
            true));  // isNewQueue

    mqbs::FileStoreSet fileSet(bmqtst::TestHelperUtil::allocator());
    fs.loadCurrentFiles(&fileSet);
    const bsl::string journalFileBefore(fileSet.journalFile(),
                                        bmqtst::TestHelperUtil::allocator());

    // Fill with message records until the journal's near-full.
    const bsls::Types::Uint64 k_FILL_UNTIL_POS = k_MAX_JOURNAL -
                                                 k_REQUESTED_JOURNAL_SPACE;

    bsl::size_t guard = 0;
    while (true) {
        BSLS_ASSERT_OPT(++guard < 100000);
        fs.loadCurrentFiles(&fileSet);
        if (fileSet.journalFileSize() >= k_FILL_UNTIL_POS) {
            break;
        }

        mqbs::DataStoreRecordHandle msgHandle;
        bmqt::MessageGUID           guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);

        bsl::shared_ptr<bdlbb::Blob> appData;
        appData.createInplace(bmqtst::TestHelperUtil::allocator(),
                              &tester.bufferFactory(),
                              bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobUtil::append(appData.get(), "x", 1);
        bsl::shared_ptr<bdlbb::Blob> options;

        mqbi::StorageMessageAttributes attributes(
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()),
            1,  // refCount
            static_cast<unsigned int>(appData->length()),
            bmqp::MessagePropertiesInfo(),
            bmqt::CompressionAlgorithmType::e_NONE,
            0U);  // crc32c

        rc = fs.writeMessageRecord(&attributes,
                                   &msgHandle,
                                   guid,
                                   appData,
                                   options,
                                   queueKey);
        BMQTST_ASSERT_EQ(0, rc);
        if (0 != rc) {
            fs.close();
            return;  // RETURN
        }
    }

    // Phase 1 must not have rolled over: same journal file, near-full.
    fs.loadCurrentFiles(&fileSet);
    BMQTST_ASSERT_EQ(journalFileBefore, fileSet.journalFile());
    BMQTST_ASSERT_GE(fileSet.journalFileSize(), k_FILL_UNTIL_POS);
    const bsls::Types::Uint64 nearFullPos = fileSet.journalFileSize();

    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);
    BMQTST_ASSERT_EQ(false, fs.isOpen());

    // --- Phase 2: reopen as primary with an empty cluster state, so both
    // 'extraQ' and 'extraQ2' are extra queues.  The corrective DELETIONs do
    // not fit on the near-full journal, so conform rolls over once.  Rollover
    // compacts away both extra queues (their records are never outstanding),
    // so NO corrective DELETION is written.
    const unsigned int                     k_NEW_LEASE = 2;
    mqbs::DataStoreConfig::QueueKeyInfoMap clusterState(
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(0, fs.open(&clusterState, k_NEW_LEASE));
    BMQTST_ASSERT_EQ(true, fs.isOpen());

    fs.loadCurrentFiles(&fileSet);

    // Conform rolled the journal over to a fresh, smaller file.
    BMQTST_ASSERT_NE(journalFileBefore, fileSet.journalFile());
    BMQTST_ASSERT_LT(fileSet.journalFileSize(), nearFullPos);
    BMQTST_ASSERT_LT(fileSet.journalFileSize(), k_FILL_UNTIL_POS);

    // The rollover produced a well-formed new journal: it begins with an
    // e_REGULAR sync point.
    BMQTST_ASSERT(journalHasSyncPoint(fileSet.journalFile(),
                                      mqbs::SyncPointType::e_REGULAR));

    // The extra queues were compacted away by the rollover, so no corrective
    // QueueOp.DELETION was written.
    BMQTST_ASSERT_EQ(0, journalDeletionCount(fileSet.journalFile()));

    // The rollover sync point is stamped under the NEW lease at (2, 1), so the
    // write head reflects the new lease.
    BMQTST_ASSERT_EQ(k_NEW_LEASE, fs.writeHeadLeaseId());
    BMQTST_ASSERT_EQ(1ULL, fs.writeHeadSeqNum());

    rc = fs.close();
    BMQTST_ASSERT_EQ(0, rc);
}

}  // close unnamed namespace

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqu::Time::initialize();

    switch (_testCase) {
    case 0:
    case 8: test8_conformExtraQueueCausesRollover(); break;
    case 7: test7_conformExtraQueueWithoutRollover(); break;
    case 6: test6_leaseTransitionWithoutSeal(); break;
    case 5: test5_writeHeadFollowsAppliedLease(); break;
    case 4: test4_recoverMessagesAcrossLeaseIds(); break;
    case 3: test3_partitionFullAlarm(); break;
    case 2: test2_printTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqu::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_ALLOC);
}
