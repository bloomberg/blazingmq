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
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>

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
#include <bsl_memory.h>
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
typedef mqbs::DataStore::AppIdKeyPairs                 AppIdKeyPairs;
typedef mqbs::FileStore::SyncPointOffsetPairs          SyncPointOffsetPairs;
typedef bsl::pair<mqbs::DataStoreRecordHandle, Record> HandleRecordPair;

typedef bdlcc::SharedObjectPool<
    bdlbb::Blob,
    bdlcc::ObjectPoolFunctors::DefaultCreator,
    bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
    BlobSpPool;

// FUNCTIONS

/// Create a new blob at the specified `arena` address, using the specified
/// `bufferFactory` and `allocator`.
void createBlob(bdlbb::BlobBufferFactory* bufferFactory,
                void*                     arena,
                bslma::Allocator*         allocator)
{
    new (arena) bdlbb::Blob(bufferFactory, allocator);
}

void queueCreationCb(int*                    status,
                     int                     partitionId,
                     const bmqt::Uri&        uri,
                     const mqbu::StorageKey& queueKey)
{
    static_cast<void>(status);
    static_cast<void>(partitionId);
    static_cast<void>(uri);
    static_cast<void>(queueKey);
}

void queueDeletionCb(int*                    status,
                     int                     partitionId,
                     const bmqt::Uri&        uri,
                     const mqbu::StorageKey& queueKey)
{
    static_cast<void>(status);
    static_cast<void>(partitionId);
    static_cast<void>(uri);
    static_cast<void>(queueKey);
}

void recoveredQueuesCb(
    int                                           partitionId,
    const mqbs::DataStoreConfig::QueueKeyInfoMap& queueKeyInfoMap)
{
    static_cast<void>(partitionId);
    static_cast<void>(queueKeyInfoMap);
}

// CLASSES
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
    BlobSpPool                             d_blobSpPool;
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
    mqbi::DispatcherClientData             d_dispatcherClientData;
    mqbmock::Dispatcher                    d_dispatcher;
    // must outlive FileStore
    bslma::ManagedPtr<mqbs::FileStore> d_fs_mp;
    mqbs::FileStore::StateSpPool       d_statePool;

  public:
    // CREATORS
    Tester(const char* location)
    : d_scheduler(bsls::SystemClockType::e_MONOTONIC, s_allocator_p)
    , d_bufferFactory(1024, s_allocator_p)
    , d_clusterLocation(location, s_allocator_p)
    , d_clusterArchiveLocation(location, s_allocator_p)
    , d_blobSpPool(bdlf::BindUtil::bind(&createBlob,
                                        &d_bufferFactory,
                                        bdlf::PlaceHolders::_1,   // arena
                                        bdlf::PlaceHolders::_2),  // alloc
                   1024,  // blob pool growth strategy
                   s_allocator_p)
    , d_partitionCfg(s_allocator_p)
    , d_clusterCfg(s_allocator_p)
    , d_clusterNodesCfg(s_allocator_p)
    , d_clusterNodeCfg(s_allocator_p)
    , d_clusterStatsRootContext_sp(
          mqbstat::ClusterStatsUtil::initializeStatContextCluster(
              2,
              s_allocator_p))
    , d_clusterStats(s_allocator_p)
    , d_miscWorkThreadPool(1, 1, s_allocator_p)
    , d_dispatcherClientData()
    , d_dispatcher(s_allocator_p)
    , d_statePool(1024, s_allocator_p)
    {
        bdls::FilesystemUtil::remove(d_clusterLocation, true);
        bdls::FilesystemUtil::remove(d_clusterArchiveLocation, true);

        bdls::FilesystemUtil::createDirectories(d_clusterLocation, true);
        bdls::FilesystemUtil::createDirectories(d_clusterArchiveLocation,
                                                true);

        d_partitionCfg.maxDataFileSize()     = 100 * 1024 * 1024;
        d_partitionCfg.maxQlistFileSize()    = 1 * 1024 * 1024;
        d_partitionCfg.maxJournalFileSize()  = 1 * 1024 * 1024;
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

        d_cluster_mp.load(new (*s_allocator_p)
                              mqbnet::MockCluster(d_clusterCfg,
                                                  &d_bufferFactory,
                                                  s_allocator_p),
                          s_allocator_p);
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
            .setQueueCreationCb(
                bdlf::BindUtil::bind(&queueCreationCb,
                                     bdlf::PlaceHolders::_1,   // status
                                     bdlf::PlaceHolders::_2,   // partitionId
                                     bdlf::PlaceHolders::_3,   // QueueUri
                                     bdlf::PlaceHolders::_4))  // QueueKey
            .setQueueDeletionCb(
                bdlf::BindUtil::bind(&queueDeletionCb,
                                     bdlf::PlaceHolders::_1,   // status
                                     bdlf::PlaceHolders::_2,   // partitionId
                                     bdlf::PlaceHolders::_3,   // QueueUri
                                     bdlf::PlaceHolders::_4))  // QueueKey
            .setRecoveredQueuesCb(bdlf::BindUtil::bind(
                &recoveredQueuesCb,
                bdlf::PlaceHolders::_1,    // partitionId
                bdlf::PlaceHolders::_2));  // queueKeyInfoMap

        d_dispatcherClientData.setDispatcher(&d_dispatcher);
        d_dispatcher._setInDispatcherThread(true);

        d_clusterStats.initialize("testCluster",
                                  1,  // numPartitions
                                  d_clusterStatsRootContext_sp.get(),
                                  s_allocator_p);
        d_fs_mp.load(new (*s_allocator_p)
                         mqbs::FileStore(d_dsCfg,
                                         0,  // processorId
                                         &d_dispatcher,
                                         d_cluster_mp.get(),
                                         &d_clusterStats,
                                         &d_blobSpPool,
                                         &d_statePool,
                                         &d_miscWorkThreadPool,
                                         false,  // isCSLModeEnabled
                                         false,  // isFSMWorkflow
                                         1,      // replicationFactor
                                         s_allocator_p),
                     s_allocator_p);
    }

    ~Tester()
    {
        bdls::FilesystemUtil::remove(d_clusterLocation, true);
        bdls::FilesystemUtil::remove(d_clusterArchiveLocation, true);
    }

    // MANIPULATORS
    bool writeRecords(mqbs::FileStore*               fs,
                      bsl::vector<HandleRecordPair>* records,
                      SyncPointOffsetPairs*          spOffsetPairs,
                      unsigned int*                  leaseId,
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

        QueueKeyUriMap   queueKeyUriMap(s_allocator_p);
        QueueKeyGuidsMap queueKeyGuidsMap(s_allocator_p);
        QueueKeyGuidsMap queueKeyConfGuidsMap(s_allocator_p);
        bsl::string      uriBase("bmq://si.amw.bmq.stats/", s_allocator_p);
        const size_t     k_DIVISOR = 7;
        int              rc        = 0;
        int              seed      = 58133;
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

                bsl::string        uri(uriBase, s_allocator_p);
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
                Record                      rec(s_allocator_p);
                rec.d_recordType  = mqbs::RecordType::e_QUEUE_OP;
                rec.d_queueOpType = mqbs::QueueOpType::e_CREATION;
                rec.d_uri         = uri;
                rec.d_queueKey    = queueKey;
                rec.d_timestamp   = bdlt::EpochUtil::convertToTimeT64(
                    bdlt::CurrentTime::utc());

                rc = fs->writeQueueCreationRecord(&handle,
                                                  bmqt::Uri(rec.d_uri,
                                                            s_allocator_p),
                                                  rec.d_queueKey,
                                                  AppIdKeyPairs(),
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
                Record                      rec(s_allocator_p);
                rec.d_recordType = mqbs::RecordType::e_MESSAGE;
                rec.d_queueKey   = it->first;
                bmqp::MessagePropertiesInfo messagePropertiesInfo =
                    (0 == i % 2) ? bmqp::MessagePropertiesInfo::makeNoSchema()
                                 : bmqp::MessagePropertiesInfo();

                rec.d_msgAttributes = mqbi::StorageMessageAttributes(
                    bdlt::EpochUtil::convertToTimeT64(
                        bdlt::CurrentTime::utc()),
                    i % mqbs::FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD,
                    messagePropertiesInfo,
                    bmqt::CompressionAlgorithmType::e_NONE,
                    bsl::numeric_limits<unsigned int>::max() / i);
                // crc value
                mqbu::MessageGUIDUtil::generateGUID(&rec.d_guid);
                rec.d_appData_sp.createInplace(s_allocator_p,
                                               &d_bufferFactory,
                                               s_allocator_p);
                bsl::string payloadStr(i * 10, 'x', s_allocator_p);
                bdlbb::BlobUtil::append(rec.d_appData_sp.get(),
                                        payloadStr.c_str(),
                                        payloadStr.length());

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
                Record                      rec(s_allocator_p);
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

                Record rec(s_allocator_p);
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

                mqbs::FileStoreSet fileSet(s_allocator_p);
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

                Record rec(s_allocator_p);
                rec.d_recordType    = mqbs::RecordType::e_JOURNAL_OP;
                rec.d_journalOpType = mqbs::JournalOpType::e_SYNCPOINT;
                rec.d_syncPtType    = mqbs::SyncPointType::e_REGULAR;

                rec.d_syncPoint.primaryLeaseId() = *leaseId;
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
                Record                      rec(s_allocator_p);
                rec.d_recordType  = mqbs::RecordType::e_QUEUE_OP;
                rec.d_queueOpType = mqbs::QueueOpType::e_PURGE;
                rec.d_queueKey    = it->first;
                rec.d_timestamp   = bdlt::EpochUtil::convertToTimeT64(
                    bdlt::CurrentTime::utc());

                rc = fs->writeQueuePurgeRecord(&handle,
                                               rec.d_queueKey,
                                               mqbu::StorageKey(),
                                               rec.d_timestamp);
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
                Record                      rec(s_allocator_p);
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
    s_ignoreCheckDefAlloc = true;

    const char k_FILE_STORE_LOCATION[] = "./test-cluster123-1";

    Tester           tester(k_FILE_STORE_LOCATION);
    mqbs::FileStore& fs = tester.fileStore();

    int rc = fs.open();
    ASSERT_EQ(0, rc);
    if (rc) {
        cout << "Failed to open partition, rc: " << rc << endl;
        return;  // RETURN
    }

    ASSERT_EQ(true, fs.isOpen());
    ASSERT_EQ(1U, fs.clusterSize());
    ASSERT_EQ(0ULL, fs.numRecords());
    ASSERT_EQ(true, fs.syncPoints().empty());
    ASSERT_EQ(0U, fs.primaryLeaseId());
    ASSERT_EQ(0ULL, fs.sequenceNumber());

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.  TBD: figure
    // out the right way to "fix" this.

    Record dummy(s_allocator_p);
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Record, bslma::UsesBslmaAllocator> >(
            dummy));

    // Set primary.
    unsigned int        primaryLeaseId = 1;
    bsls::Types::Uint64 seqNum         = 1;

    fs.setActivePrimary(tester.node(), primaryLeaseId);

    ASSERT_EQ(primaryLeaseId, fs.primaryLeaseId());
    ASSERT_EQ(seqNum, fs.sequenceNumber());
    ASSERT_EQ(tester.node(), fs.primaryNode());

    // Primary must have issued a SyncPt.  Verify it.

    const bmqp_ctrlmsg::SyncPoint& sp = fs.syncPoints().front().syncPoint();
    ASSERT_EQ(1U, fs.syncPoints().size());
    ASSERT_EQ(primaryLeaseId, sp.primaryLeaseId());
    ASSERT_EQ(seqNum, sp.sequenceNum());
    ASSERT_EQ((k_SIZEOF_HEADERS_DATA_FILE / bmqp::Protocol::k_DWORD_SIZE),
              sp.dataFileOffsetDwords());
    ASSERT_EQ((k_SIZEOF_HEADERS_QLIST_FILE / bmqp::Protocol::k_WORD_SIZE),
              sp.qlistFileOffsetWords());
    ASSERT_EQ(k_SIZEOF_HEADERS_JOURNAL_FILE, fs.syncPoints().front().offset());

    // Write various records to the partition and keep track of them in memory.
    // Then close and re-open the partition, and verify that retrieved records
    // match in-memory stuff.

    SyncPointOffsetPairs          spOffsetPairs(s_allocator_p);
    bsl::vector<HandleRecordPair> records(s_allocator_p);

    // Add one SyncPt written by the primary (to both 'spOffsetPairs' and
    // 'records').

    Record rec(s_allocator_p);
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
                                       &primaryLeaseId,
                                       &seqNum,
                                       &numRecordsWritten,
                                       k_NUM_RECORDS);

    ASSERT_EQ(true, success);
    if (!success) {
        fs.close();
        return;  // RETURN
    }

    const SyncPointOffsetPairs& fsSpOffsetPair = fs.syncPoints();
    ASSERT_EQ(spOffsetPairs.size(), fsSpOffsetPair.size());
    for (size_t i = 0; i < spOffsetPairs.size(); ++i) {
        ASSERT_EQ_D(i,
                    spOffsetPairs[i].syncPoint(),
                    fsSpOffsetPair[i].syncPoint());
        ASSERT_EQ_D(i, spOffsetPairs[i].offset(), fsSpOffsetPair[i].offset());
    }
    ASSERT_EQ(numRecordsWritten, fs.numRecords());

    mqbs::FileStoreIterator fsIt(&fs);
    while (fsIt.next()) {
        // TBD: verify
    }

    fs.close();

    ASSERT_EQ(false, fs.isOpen());

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

    s_ignoreCheckDefAlloc = true;

    const char k_FILE_STORE_LOCATION[] = "./test-cluster123-2";

    Tester           tester(k_FILE_STORE_LOCATION);
    mqbs::FileStore& fs = tester.fileStore();
    BSLS_ASSERT_OPT(fs.open() == 0);

    // Set primary.
    unsigned int        primaryLeaseId = 1;
    bsls::Types::Uint64 seqNum         = 1;
    fs.setActivePrimary(tester.node(), primaryLeaseId);

    // Write various records to the partition.
    SyncPointOffsetPairs          spOffsetPairs(s_allocator_p);
    bsl::vector<HandleRecordPair> records(s_allocator_p);

    const size_t        k_NUM_RECORDS     = 10;
    bsls::Types::Uint64 numRecordsWritten = 0;
    BSLS_ASSERT_OPT(tester.writeRecords(&fs,
                                        &records,
                                        &spOffsetPairs,
                                        &primaryLeaseId,
                                        &seqNum,
                                        &numRecordsWritten,
                                        k_NUM_RECORDS));

    bdlpcre::RegEx expectedOut(s_allocator_p);
    bsl::string    errorMessage(s_allocator_p);
    size_t         errorOffset;
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
    bmqu::MemOutStream      stream(s_allocator_p);
    while (fsIt.next()) {
        stream << fsIt << "\n";
    }
    ASSERT_EQ(expectedOut.match(stream.str().data(), stream.str().length()),
              0);

    PV("Bad stream test");
    stream.reset();
    stream << "INVALID";
    stream.clear(bsl::ios_base::badbit);
    stream << fsIt;
    ASSERT_EQ(stream.str(), "INVALID");

    fs.close();
}

}  // close unnamed namespace

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize();
    mqbu::MessageGUIDUtil::initialize();
    bmqp::ProtocolUtil::initialize();

    switch (_testCase) {
    case 0:
    case 2: test2_printTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();
    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_ALLOC);
}
