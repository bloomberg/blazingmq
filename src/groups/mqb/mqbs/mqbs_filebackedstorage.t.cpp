// Copyright 2024 Bloomberg Finance L.P.
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

// mqbs_filebackedstorage.t.cpp                                       -*-C++-*-
#include <mqbs_filebackedstorage.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_queue.h>
#include <mqbmock_cluster.h>
#include <mqbmock_domain.h>
#include <mqbmock_queue.h>
#include <mqbmock_queueengine.h>
#include <mqbs_datastore.h>
#include <mqbs_replicatedstorage.h>
#include <mqbstat_brokerstats.h>
#include <mqbu_capacitymeter.h>
#include <mqbu_messageguidutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <ball_loggermanager.h>
#include <ball_severity.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_algorithm.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_objectbuffer.h>
#include <bsls_types.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
// - breathingTest
// - configure
// - supportedOperations
// - put_noVirtualStorage
// - getMessageSize
// - get_noVirtualStorages
// - remove_messageNotFound
//   removeMessage
// - addVirtualStorage
// - hasVirtualStorage
// - removeVirtualStorage
// - put_withVirtualStorages
// - removeAllMessages
//   removeAllMessages_appKeyNotFound
// - get_withVirtualStorages
// - releaseRef
// - getIterator_noVirtualStorages
//   getIterator_withVirtualStorages
// - capacityMeter_limitMessages
//   capacityMeter_limitBytes
// - garbageCollect
// - addQueueOpRecordHandle
// - doNotRecordLastConfirmInPriorityMode
// - doNotRecordLastConfirmInFanoutMode
//-----------------------------------------------------------------------------

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

// CONSTANTS
const int  k_PARTITION_ID       = 1;
const int  k_PROXY_PARTITION_ID = mqbs::DataStore::k_INVALID_PARTITION_ID;
const char k_HEX_QUEUE[]        = "ABCDEF1234";
const bsls::Types::Int64 k_DEFAULT_MSG   = 20;
const bsls::Types::Int64 k_DEFAULT_BYTES = 2048;
const char               k_URI_STR[]     = "bmq://mydomain/testqueue";
const char               k_APP_ID1[]     = "app1";
const char               k_APP_ID2[]     = "app2";
const char               k_APP_ID3[]     = "app3";
const mqbu::StorageKey   k_QUEUE_KEY(mqbu::StorageKey::HexRepresentation(),
                                   k_HEX_QUEUE);
const mqbu::StorageKey   k_APP_KEY1(mqbu::StorageKey::HexRepresentation(),
                                  "ABCDEF1111");
const mqbu::StorageKey   k_APP_KEY2(mqbu::StorageKey::HexRepresentation(),
                                  "ABCDEF2222");
const mqbu::StorageKey   k_APP_KEY3(mqbu::StorageKey::HexRepresentation(),
                                  "ABCDEF3333");

// ALIASES

const bsls::Types::Int64 k_INT64_ZERO = 0;
const bsls::Types::Int64 k_INT64_MAX =
    bsl::numeric_limits<bsls::Types::Int64>::max();
const mqbu::StorageKey k_NULL_KEY = mqbu::StorageKey::k_NULL_KEY;

// FUNCTIONS
static mqbconfm::Storage fileBackedStorageConfig()
{
    mqbconfm::Storage config;
    config.makeInMemory();
    return config;
}

static bmqt::MessageGUID generateRandomGUID()
{
    bmqt::MessageGUID randomGUID;
    mqbu::MessageGUIDUtil::generateGUID(&randomGUID);
    return randomGUID;
}

static bmqt::MessageGUID
generateUniqueGUID(const bsl::vector<bmqt::MessageGUID>& guids)
{
    bmqt::MessageGUID uniqueGUID;
    do {
        // This loop will exit from the first iteration due to the current
        // implementation of generator

        uniqueGUID = generateRandomGUID();
    } while (bsl::find(guids.begin(), guids.end(), uniqueGUID) != guids.end());

    return uniqueGUID;
}

// CLASSES

// ===================
// class MockDataStore
// ===================

/// Minimal mock implementation of the `mqbs::DataStore` interface
/// required by `mqbs::FileBackedStorage`.

class MockDataStore : public mqbs::DataStore {
  private:
    // PRIVATE TYPES

    typedef mqbs::DataStoreConfig::Records        Records;
    typedef mqbs::DataStoreConfig::RecordIterator RecordIterator;
    typedef bsl::pair<RecordIterator, bool>       InsertRc;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    mqbs::DataStoreConfig      d_config;
    mqbi::DispatcherClientData d_dispatcherClientData;
    bsl::string                d_description;

    Records d_records;

    bsl::map<bsls::Types::Uint64, mqbi::StorageMessageAttributes> d_attributes;
    bsl::map<bsls::Types::Uint64, bsl::shared_ptr<bdlbb::Blob> >  d_appData;
    bsl::map<bsls::Types::Uint64, bsl::shared_ptr<bdlbb::Blob> >  d_options;

    bsls::Types::Uint64 d_message_counter;
    bsls::Types::Uint64 d_confirm_counter;
    bsls::Types::Uint64 d_deletion_counter;

  public:
    MockDataStore(bslma::Allocator* allocator, int partitionId)
    : d_allocator_p(allocator)
    , d_attributes(allocator)
    , d_appData(allocator)
    , d_options(allocator)
    {
        d_message_counter  = 0ULL;
        d_confirm_counter  = 0ULL;
        d_deletion_counter = 0ULL;

        d_config.setPartitionId(partitionId);
    }

    bsls::Types::Uint64 getMessageCounter() const { return d_message_counter; }

    bsls::Types::Uint64 getConfirmCounter() const { return d_confirm_counter; }

    bsls::Types::Uint64 getDeletionCounter() const
    {
        return d_deletion_counter;
    }

    int
    writeMessageRecord(mqbi::StorageMessageAttributes* attributes,
                       mqbs::DataStoreRecordHandle*    handle,
                       BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& guid,
                       const bsl::shared_ptr<bdlbb::Blob>&             appData,
                       const bsl::shared_ptr<bdlbb::Blob>&             options,
                       BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& queueKey)
        BSLS_KEYWORD_OVERRIDE
    {
        d_message_counter++;

        bsls::Types::Uint64          id             = d_message_counter;
        bsls::Types::Uint64          sequenceNum    = id;
        unsigned int                 primaryLeaseId = 0;
        const mqbs::RecordType::Enum recType   = mqbs::RecordType::e_MESSAGE;
        bsls::Types::Uint64          recOffset = id;
        mqbs::DataStoreConfig::RecordIterator* iter =
            reinterpret_cast<mqbs::DataStoreConfig::RecordIterator*>(handle);

        InsertRc insertRc = d_records.insert(bsl::make_pair(
            mqbs::DataStoreRecordKey(sequenceNum, primaryLeaseId),
            mqbs::DataStoreRecord(recType, recOffset)));

        // Needed for FileBackedStorage::gcExpiredMessages
        insertRc.first->second.d_arrivalTimestamp =
            attributes->arrivalTimestamp();

        *iter = insertRc.first;

        d_attributes.insert({id, *attributes});
        d_appData.insert({id, appData});
        d_options.insert({id, options});

        return 0;
    }

    int writeConfirmRecord(mqbs::DataStoreRecordHandle*,
                           const bmqt::MessageGUID&,
                           const mqbu::StorageKey&,
                           const mqbu::StorageKey&,
                           bsls::Types::Uint64,
                           mqbs::ConfirmReason::Enum) BSLS_KEYWORD_OVERRIDE
    {
        d_confirm_counter++;
        return 0;
    }

    int writeDeletionRecord(const bmqt::MessageGUID&,
                            const mqbu::StorageKey&,
                            mqbs::DeletionRecordFlag::Enum,
                            bsls::Types::Uint64) BSLS_KEYWORD_OVERRIDE
    {
        d_deletion_counter++;
        return 0;
    }

    void loadMessageAttributesRaw(
        mqbi::StorageMessageAttributes*    buffer,
        const mqbs::DataStoreRecordHandle& handle) const BSLS_KEYWORD_OVERRIDE
    {
        const mqbs::DataStoreConfig::RecordIterator& iter =
            *reinterpret_cast<const mqbs::DataStoreConfig::RecordIterator*>(
                &handle);
        bsls::Types::Uint64 id = iter->second.d_recordOffset;

        *buffer = d_attributes.at(id);
    }

    void loadMessageRaw(bsl::shared_ptr<bdlbb::Blob>*      appData,
                        bsl::shared_ptr<bdlbb::Blob>*      options,
                        mqbi::StorageMessageAttributes*    attributes,
                        const mqbs::DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE
    {
        loadMessageAttributesRaw(attributes, handle);

        const mqbs::DataStoreConfig::RecordIterator& iter =
            *reinterpret_cast<const mqbs::DataStoreConfig::RecordIterator*>(
                &handle);
        bsls::Types::Uint64 id = iter->second.d_recordOffset;

        *appData = d_appData.at(id);
        *options = d_options.at(id);
    }

    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE { return nullptr; }

    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE
    {
        return d_dispatcherClientData;
    }

    void onDispatcherEvent(const mqbi::DispatcherEvent&) BSLS_KEYWORD_OVERRIDE
    {
    }

    void flush() BSLS_KEYWORD_OVERRIDE {}

    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE
    {
        return nullptr;
    }

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE
    {
        return d_dispatcherClientData;
    }

    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE
    {
        return d_description;
    }

    int open(const QueueKeyInfoMap&) BSLS_KEYWORD_OVERRIDE { return 0; }

    void close(bool) BSLS_KEYWORD_OVERRIDE {}

    void createStorage(bsl::shared_ptr<mqbs::ReplicatedStorage>*,
                       const bmqt::Uri&,
                       const mqbu::StorageKey&,
                       mqbi::Domain*) BSLS_KEYWORD_OVERRIDE
    {
    }

    int writeQueueCreationRecord(mqbs::DataStoreRecordHandle*,
                                 const bmqt::Uri&,
                                 const mqbu::StorageKey&,
                                 const AppIdKeyPairs&,
                                 bsls::Types::Uint64,
                                 bool) BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }

    int writeQueuePurgeRecord(mqbs::DataStoreRecordHandle*,
                              const mqbu::StorageKey&,
                              const mqbu::StorageKey&,
                              bsls::Types::Uint64) BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }

    int writeQueueDeletionRecord(mqbs::DataStoreRecordHandle*,
                                 const mqbu::StorageKey&,
                                 const mqbu::StorageKey&,
                                 bsls::Types::Uint64) BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }

    int writeSyncPointRecord(const bmqp_ctrlmsg::SyncPoint&,
                             mqbs::SyncPointType::Enum) BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }

    int removeRecord(const mqbs::DataStoreRecordHandle&) BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }

    void
    removeRecordRaw(const mqbs::DataStoreRecordHandle&) BSLS_KEYWORD_OVERRIDE
    {
    }

    void processStorageEvent(const bsl::shared_ptr<bdlbb::Blob>&,
                             bool,
                             mqbnet::ClusterNode*) BSLS_KEYWORD_OVERRIDE
    {
    }

    int processRecoveryEvent(const bsl::shared_ptr<bdlbb::Blob>&)
        BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }

    void processReceiptEvent(unsigned int,
                             bsls::Types::Uint64,
                             mqbnet::ClusterNode*) BSLS_KEYWORD_OVERRIDE
    {
    }

    int issueSyncPoint() BSLS_KEYWORD_OVERRIDE { return 0; }

    void setActivePrimary(mqbnet::ClusterNode*,
                          unsigned int) BSLS_KEYWORD_OVERRIDE
    {
    }

    void clearPrimary() BSLS_KEYWORD_OVERRIDE {}

    void dispatcherFlush(bool, bool) BSLS_KEYWORD_OVERRIDE {}

    bool isOpen() const BSLS_KEYWORD_OVERRIDE { return true; }

    const mqbs::DataStoreConfig& config() const BSLS_KEYWORD_OVERRIDE
    {
        return d_config;
    }

    unsigned int clusterSize() const BSLS_KEYWORD_OVERRIDE { return 1U; }

    bsls::Types::Uint64 numRecords() const BSLS_KEYWORD_OVERRIDE
    {
        return d_attributes.size();
    }

    void loadMessageRecordRaw(mqbs::MessageRecord*,
                              const mqbs::DataStoreRecordHandle&) const
        BSLS_KEYWORD_OVERRIDE
    {
    }

    void loadConfirmRecordRaw(mqbs::ConfirmRecord*,
                              const mqbs::DataStoreRecordHandle&) const
        BSLS_KEYWORD_OVERRIDE
    {
    }

    void loadDeletionRecordRaw(mqbs::DeletionRecord*,
                               const mqbs::DataStoreRecordHandle&) const
        BSLS_KEYWORD_OVERRIDE
    {
    }

    void loadQueueOpRecordRaw(mqbs::QueueOpRecord*,
                              const mqbs::DataStoreRecordHandle&) const
        BSLS_KEYWORD_OVERRIDE
    {
    }

    unsigned int getMessageLenRaw(const mqbs::DataStoreRecordHandle&) const
        BSLS_KEYWORD_OVERRIDE
    {
        return sizeof(int);
    }

    unsigned int primaryLeaseId() const BSLS_KEYWORD_OVERRIDE { return 0; }

    bool
    hasReceipt(const mqbs::DataStoreRecordHandle&) const BSLS_KEYWORD_OVERRIDE
    {
        return true;
    }
};

// =================
// struct BaseTester
// =================

/// BaseTester class provides testing capabilities to verify both
/// FileBackedStorage and InMemoryStorage
struct BaseTester {
  private:
    // PRIVATE TYPES
    typedef mqbs::DataStoreConfig::Records Records;

  protected:
    // DATA
    bdlbb::PooledBlobBufferFactory             d_bufferFactory;
    mqbmock::Cluster                           d_mockCluster;
    mqbmock::Domain                            d_mockDomain;
    mqbmock::Queue                             d_mockQueue;
    mqbmock::QueueEngine                       d_mockQueueEngine;
    bslma::ManagedPtr<mqbs::ReplicatedStorage> d_replicatedStorage_mp;
    Records                                    d_records;
    bslma::Allocator*                          d_allocator_p;

  public:
    // CREATORS
    explicit BaseTester(bslma::Allocator* allocator)
    : d_bufferFactory(1024, allocator)
    , d_mockCluster(&d_bufferFactory, allocator)
    , d_mockDomain(&d_mockCluster, allocator)
    , d_mockQueue(&d_mockDomain, allocator)
    , d_mockQueueEngine(allocator)
    , d_replicatedStorage_mp()
    , d_records(allocator)
    , d_allocator_p(allocator)
    {
        d_mockDomain.capacityMeter()->setLimits(k_INT64_MAX, k_INT64_MAX);
        d_mockQueue._setQueueEngine(&d_mockQueueEngine);
    }

    ~BaseTester()
    {
        d_replicatedStorage_mp->removeAll(k_NULL_KEY);
        d_replicatedStorage_mp->close();
    }

    // MANIPULATORS
    void setupReplicatedStorage(bsls::Types::Int64 ttlSeconds,
                                bool               toConfigure)
    {
        d_replicatedStorage_mp->setQueue(&d_mockQueue);
        BSLS_ASSERT_OPT(d_replicatedStorage_mp->queue() == &d_mockQueue);

        if (toConfigure) {
            configure(k_DEFAULT_MSG, k_DEFAULT_BYTES, 0.8, 0.8, ttlSeconds);
        }
    }

    int configure(bsls::Types::Int64 msgCapacity,
                  bsls::Types::Int64 byteCapacity,
                  double             msgWatermarkRatio  = 0.8,
                  double             byteWatermarkRatio = 0.8,
                  bsls::Types::Int64 messageTtl         = k_INT64_MAX)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(d_replicatedStorage_mp && "Storage was not created");

        mqbconfm::Storage config;
        mqbconfm::Limits  limits;

        config.makeInMemory();

        limits.messages()               = msgCapacity;
        limits.messagesWatermarkRatio() = msgWatermarkRatio;
        limits.bytes()                  = byteCapacity;
        limits.bytesWatermarkRatio()    = byteWatermarkRatio;

        mwcu::MemOutStream errDescription(s_allocator_p);
        return d_replicatedStorage_mp->configure(errDescription,
                                                 config,
                                                 limits,
                                                 messageTtl,
                                                 0);  // maxDeliveryAttempts
    }

    mqbs::ReplicatedStorage& storage()
    {
        return *d_replicatedStorage_mp.ptr();
    }

    /// NOTE: Here the `addMessages` function adds the timestamp
    ///       as incrementing values from 0 or from dataOffset provided
    ///       Thus acting like a vector clock, also easier to verify.
    mqbi::StorageResult::Enum
    addMessages(bsl::vector<bmqt::MessageGUID>* guidHolder,
                const int                       msgCount,
                int                             dataOffset   = 0,
                bool                            useSameGuids = false,
                int                             refCount     = 1)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(guidHolder);

        int guidCount = static_cast<int>(guidHolder->size());

        if (useSameGuids) {
            BSLS_ASSERT_OPT(guidCount == msgCount);
        }
        else {
            guidCount += msgCount;
            guidHolder->reserve(guidCount);

            for (int i = 0; i < msgCount; i++) {
                bmqt::MessageGUID guid;
                mqbu::MessageGUIDUtil::generateGUID(&guid);
                guidHolder->push_back(guid);
            }
        }

        for (int i = 0; i < msgCount; i++) {
            const bmqt::MessageGUID& guid = guidHolder->at(guidCount -
                                                           msgCount + i);

            const int data = i + dataOffset;

            mqbi::StorageMessageAttributes attributes(
                static_cast<bsls::Types::Uint64>(data),
                refCount,
                bmqp::MessagePropertiesInfo::makeNoSchema(),
                bmqt::CompressionAlgorithmType::e_NONE);

            const bsl::shared_ptr<bdlbb::Blob> appDataPtr(
                new (*s_allocator_p)
                    bdlbb::Blob(&d_bufferFactory, s_allocator_p),
                s_allocator_p);

            bdlbb::BlobUtil::append(&(*appDataPtr),
                                    reinterpret_cast<const char*>(&data),
                                    static_cast<int>(sizeof(int)));

            mqbi::StorageResult::Enum rc = d_replicatedStorage_mp->put(
                &attributes,
                guid,
                appDataPtr,
                appDataPtr);

            if (rc != mqbi::StorageResult::e_SUCCESS) {
                return rc;  // RETURN
            }
        }

        return mqbi::StorageResult::e_SUCCESS;
    }

    void insertDataStoreRecord(mqbs::DataStoreRecordHandle*    handle,
                               const mqbs::DataStoreRecordKey& key,
                               const mqbs::DataStoreRecord&    record)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(handle && "'handle' must be specified");

        // TYPES
        typedef mqbs::DataStoreConfig::RecordIterator RecordIterator;
        typedef bsl::pair<RecordIterator, bool>       InsertRc;

        InsertRc insertRc = d_records.insert(bsl::make_pair(key, record));
        BSLS_ASSERT_SAFE(insertRc.second);
        RecordIterator& recordItRef = *reinterpret_cast<RecordIterator*>(
            handle);
        recordItRef = insertRc.first;
    }

  private:
    // NOT IMPLEMENTED
    BaseTester(const BaseTester&) BSLS_KEYWORD_DELETED;
    BaseTester& operator=(const BaseTester&) BSLS_KEYWORD_DELETED;
};

// =============
// struct Tester
// =============
struct Tester : public BaseTester {
    MockDataStore d_dataStore;

  public:
    Tester(const bslstl::StringRef& uri,
           const mqbu::StorageKey&  queueKey,
           int                      partitionId,
           bsls::Types::Int64       ttlSeconds,
           bslma::Allocator*        allocator,
           bool                     toConfigure = false)
    : BaseTester(allocator)
    , d_dataStore(allocator, partitionId)
    {
        mqbconfm::Domain domainCfg;
        domainCfg.deduplicationTimeMs() = 0;  // No history
        domainCfg.messageTtl()          = ttlSeconds;

        d_replicatedStorage_mp.load(
            new (*d_allocator_p)
                mqbs::FileBackedStorage(&d_dataStore,
                                        bmqt::Uri(uri, s_allocator_p),
                                        queueKey,
                                        domainCfg,
                                        d_mockDomain.capacityMeter(),
                                        d_allocator_p),
            d_allocator_p);

        setupReplicatedStorage(ttlSeconds, toConfigure);
    }

    const MockDataStore& dataStore() { return d_dataStore; }
};

// ================
// struct BasicTest
// ================

/// Fixture instantiating a tester of `mqbs::FileBackedStorage` having not yet
/// configured the storage with a storage configuration.
struct BasicTest : mwctst::Test {
    // PUBLIC DATA
    Tester d_tester;

    // CREATORS
    BasicTest();
    ~BasicTest() BSLS_KEYWORD_OVERRIDE;
};

// ----------------
// struct BasicTest
// ----------------

// CREATORS
BasicTest::BasicTest()
: d_tester(k_URI_STR,
           k_QUEUE_KEY,
           k_PROXY_PARTITION_ID,
           k_INT64_MAX,  // ttlSeconds
           s_allocator_p,
           false)  // toConfigure
{
    // NOTHING
}

BasicTest::~BasicTest()
{
    // NOTHING
}

// ===========
// struct Test
// ===========

/// Fixture instantiating a tester of `mqbs::FileBackedStorage` having already
/// configured the storage with an FileBackedStorage configuration.
struct Test : mwctst::Test {
    // PUBLIC DATA
    Tester d_tester;

    // CREATORS
    Test();
    ~Test() BSLS_KEYWORD_OVERRIDE;
};

// -----------
// struct Test
// -----------
// CREATORS
Test::Test()
: d_tester(k_URI_STR,
           k_QUEUE_KEY,
           k_PARTITION_ID,
           k_INT64_MAX,  // ttlSeconds
           s_allocator_p,
           true)  // toConfigure
{
    // NOTHING
}

Test::~Test()
{
    // NOTHING
}

// =============
// struct GCTest
// =============

/// Fixture instantiating a tester of `mqbs::FileBackedStorage` instantiated
/// with parameterized `TTL` and having already configured the storage with
/// an FileBackedStorage configuration.
struct GCTest : mwctst::Test {
    // PUBLIC DATA
    bsls::ObjectBuffer<Tester> d_testerBuffer;

    // CREATORS
    GCTest();
    ~GCTest() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    void configure(bsls::Types::Int64 ttlSeconds);

    Tester& tester();
};

// -------------
// struct GCTest
// -------------
// CREATORS
GCTest::GCTest()
: d_testerBuffer()
{
    // NOTHING
}

GCTest::~GCTest()
{
    d_testerBuffer.object().~Tester();
}

// MANIPULATORS
void GCTest::configure(bsls::Types::Int64 ttlSeconds)
{
    new (d_testerBuffer.buffer()) Tester(k_URI_STR,
                                         k_QUEUE_KEY,
                                         k_PARTITION_ID,
                                         ttlSeconds,
                                         s_allocator_p,
                                         true);  // toConfigure
}

Tester& GCTest::tester()
{
    return d_testerBuffer.object();
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

TEST_F(BasicTest, breathingTest)
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
//  Testing:
//   - Default constructor 'mqbs::FileBackedStorage'
//   - setQueue(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    ASSERT_EQ(storage.queueUri().asString(), k_URI_STR);
    ASSERT_EQ(storage.queueKey(), k_QUEUE_KEY);
    ASSERT_EQ(storage.config(), mqbconfm::Storage());
    ASSERT_EQ(storage.isPersistent(), true);
    ASSERT_EQ(storage.numMessages(k_NULL_KEY), k_INT64_ZERO);
    ASSERT_EQ(storage.numBytes(k_NULL_KEY), k_INT64_ZERO);
    ASSERT_EQ(storage.isEmpty(), true);
    ASSERT_EQ(storage.partitionId(), k_PROXY_PARTITION_ID);
    ASSERT_NE(storage.queue(), static_cast<mqbi::Queue*>(0));
    // Queue has been set via call to 'setQueue'

    ASSERT_PASS(storage.dispatcherFlush(true, false));
    // Does nothing, at the time of this writing

    ASSERT_EQ(storage.queueOpRecordHandles().empty(), true);
}

TEST_F(BasicTest, configure)
// ------------------------------------------------------------------------
// CONFIGURE
//
// Concerns:
//   1. Configuring for the first time using an FileBackedStorage
//      configuration and limits should succeed.
//   2. Attempting to configure an already configured FileBackedStorage
//      should be allowed.
//
//  Testing:
//   - configure(...) + config()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("CONFIGURE");

    ASSERT_EQ(d_tester.configure(k_DEFAULT_MSG, k_DEFAULT_BYTES), 0);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    ASSERT_EQ(storage.capacityMeter()->byteCapacity(), k_DEFAULT_BYTES);
    ASSERT_EQ(storage.config(), fileBackedStorageConfig());

    ASSERT_EQ(d_tester.configure(k_DEFAULT_MSG, k_DEFAULT_BYTES + 5), 0);
    ASSERT_EQ(storage.capacityMeter()->byteCapacity(), k_DEFAULT_BYTES + 5);
    ASSERT_EQ(storage.config(), fileBackedStorageConfig());
}

TEST_F(Test, supportedOperations)
// ------------------------------------------------------------------------
// SUPPORTED OPERATIONS
//
// Concerns:
//   A 'mqbs::FileBackedStorage' implements ReplicatedStorage interface.
//
// Testing:
//   processMessageRecord(...)
//   processConfirmRecord(...)
//   processDeletionRecord(...)
//   purge(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SUPPORTED OPERATIONS");

    bmqt::MessageGUID guid = generateRandomGUID();
    mqbu::StorageKey  appKey;
    unsigned int      msgLen   = 0;
    unsigned int      refCount = 1;

    // CONSTANTS
    const int                 k_PRIMARY_LEASE_ID = 17;
    const bsls::Types::Uint64 k_RECORD_OFFSET    = 4096;

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    {
        const bsls::Types::Uint64    k_SEQUENCE_NUM = 1024;
        mqbs::DataStoreRecordKey     key(k_SEQUENCE_NUM, k_PRIMARY_LEASE_ID);
        const mqbs::RecordType::Enum k_RECORD_TYPE =
            mqbs::RecordType::e_MESSAGE;
        mqbs::DataStoreRecord       record(k_RECORD_TYPE, k_RECORD_OFFSET);
        mqbs::DataStoreRecordHandle handle;
        d_tester.insertDataStoreRecord(&handle, key, record);

        ASSERT_OPT_PASS(
            storage.processMessageRecord(guid, msgLen, refCount, handle));
    }

    {
        const bsls::Types::Uint64    k_SEQUENCE_NUM = 1025;
        mqbs::DataStoreRecordKey     key(k_SEQUENCE_NUM, k_PRIMARY_LEASE_ID);
        const mqbs::RecordType::Enum k_RECORD_TYPE =
            mqbs::RecordType::e_CONFIRM;
        mqbs::DataStoreRecord       record(k_RECORD_TYPE, k_RECORD_OFFSET);
        mqbs::DataStoreRecordHandle handle;
        d_tester.insertDataStoreRecord(&handle, key, record);

        ASSERT_OPT_PASS(
            storage.processConfirmRecord(guid,
                                         appKey,
                                         mqbs::ConfirmReason::e_CONFIRMED,
                                         handle));
    }

    {
        const bsls::Types::Uint64    k_SEQUENCE_NUM = 1026;
        mqbs::DataStoreRecordKey     key(k_SEQUENCE_NUM, k_PRIMARY_LEASE_ID);
        const mqbs::RecordType::Enum k_RECORD_TYPE =
            mqbs::RecordType::e_DELETION;
        mqbs::DataStoreRecord       record(k_RECORD_TYPE, k_RECORD_OFFSET);
        mqbs::DataStoreRecordHandle handle;
        d_tester.insertDataStoreRecord(&handle, key, record);

        ASSERT_OPT_PASS(storage.processDeletionRecord(guid));
    }

    ASSERT_OPT_PASS(storage.purge(appKey));
}

TEST_F(Test, put_noVirtualStorage)
// ------------------------------------------------------------------------
// Put Test - with no virtual storages
//
// Testing:
//   Verifies the 'put' operation in absence of virtual storages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PUT - WITH NO VIRTUAL STORAGES");

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    const int k_MSG_COUNT = 10;

    // Check 'put' - To physical storage (StorageKeys = NULL)
    ASSERT_EQ(d_tester.addMessages(&guids, k_MSG_COUNT),
              mqbi::StorageResult::e_SUCCESS);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // Check accessors and manipulators for Messages
    ASSERT_EQ(storage.numMessages(mqbu::StorageKey::k_NULL_KEY), k_MSG_COUNT);
    ASSERT_EQ(static_cast<unsigned int>(
                  storage.numBytes(mqbu::StorageKey::k_NULL_KEY)),
              k_MSG_COUNT * sizeof(int));

    for (int i = 0; i < k_MSG_COUNT; ++i) {
        ASSERT_EQ_D(i, storage.hasMessage(guids[i]), true);
    }

    ASSERT_EQ(storage.removeAll(mqbu::StorageKey::k_NULL_KEY),
              mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(storage.numBytes(mqbu::StorageKey::k_NULL_KEY), 0);
    ASSERT_EQ(storage.numMessages(mqbu::StorageKey::k_NULL_KEY), 0);
}

TEST_F(Test, getMessageSize)
// ------------------------------------------------------------------------
// GET MESSAGE SIZE
//
// Testing:
//   - 'getMessageSize(...)'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GET MESSAGE SIZE");

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    const int k_MSG_COUNT = 10;

    ASSERT_EQ(d_tester.addMessages(&guids, k_MSG_COUNT),
              mqbi::StorageResult::e_SUCCESS);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // Check getMessageSize
    int msgSize;
    for (int i = 0; i < k_MSG_COUNT; i++) {
        ASSERT_EQ(storage.getMessageSize(&msgSize, guids[i]),
                  mqbi::StorageResult::e_SUCCESS);
        ASSERT_EQ(static_cast<unsigned int>(msgSize), sizeof(int));
    }

    // Check with random GUID
    bmqt::MessageGUID randomGuid;
    mqbu::MessageGUIDUtil::generateGUID(&randomGuid);
    ASSERT_EQ(storage.getMessageSize(&msgSize, randomGuid),
              mqbi::StorageResult::e_GUID_NOT_FOUND);

    ASSERT_EQ(storage.removeAll(mqbu::StorageKey::k_NULL_KEY),
              mqbi::StorageResult::e_SUCCESS);
}

TEST_F(Test, get_noVirtualStorages)
// ------------------------------------------------------------------------
// Get Test - with no virtual storages
//
// Testing:
//   Verifies the 'get' operation in absence of virtual storages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GET - WITH NO VIRTUAL STORAGES");

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    const int k_MSG_COUNT = 5;

    // Put to physical storage - StorageKeys NULL
    BSLS_ASSERT_OPT(d_tester.addMessages(&guids, k_MSG_COUNT) ==
                    mqbi::StorageResult::e_SUCCESS);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // Check 'get' overloads
    for (int i = 0; i < k_MSG_COUNT; ++i) {
        {
            mqbi::StorageMessageAttributes attributes;
            ASSERT_EQ(storage.get(&attributes, guids[i]),
                      mqbi::StorageResult::e_SUCCESS);
            ASSERT_EQ(attributes.arrivalTimestamp(),
                      static_cast<bsls::Types::Uint64>(i));
            ASSERT_EQ(attributes.arrivalTimepoint(), 0LL);
            ASSERT_EQ(attributes.refCount(), static_cast<unsigned int>(1));
            ASSERT(attributes.messagePropertiesInfo().isPresent());
        }

        {
            mqbi::StorageMessageAttributes attributes;
            bsl::shared_ptr<bdlbb::Blob>   appData;
            bsl::shared_ptr<bdlbb::Blob>   options;
            ASSERT_EQ(storage.get(&appData, &options, &attributes, guids[i]),
                      mqbi::StorageResult::e_SUCCESS);

            ASSERT_EQ(attributes.arrivalTimestamp(),
                      static_cast<bsls::Types::Uint64>(i));
            ASSERT_EQ(attributes.arrivalTimepoint(), 0LL);
            ASSERT_EQ(attributes.refCount(), static_cast<unsigned int>(1));
            ASSERT(attributes.messagePropertiesInfo().isPresent());
            ASSERT_EQ(*(reinterpret_cast<int*>(appData->buffer(0).data())), i);
        }
    }

    // Check 'get' with a non-existent GUID
    mqbi::StorageMessageAttributes attributes;
    bsl::shared_ptr<bdlbb::Blob>   appData;
    bsl::shared_ptr<bdlbb::Blob>   options;
    ASSERT_EQ(storage.get(&attributes, generateUniqueGUID(guids)),
              mqbi::StorageResult::e_GUID_NOT_FOUND);
    ASSERT_EQ(storage.get(&appData,
                          &options,
                          &attributes,
                          generateUniqueGUID(guids)),
              mqbi::StorageResult::e_GUID_NOT_FOUND);
}

TEST_F(Test, remove_messageNotFound)
// ------------------------------------------------------------------------
// Remove Messages Test
//
// Testing:
//   Verifies the 'remove' in a 'mqbs::FileBackedStorage'. Check GUIDs that
//   in storage as well as GUID not in storage
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("REMOVE - MESSAGE NOT FOUND");

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // 'remove' one random message
    bmqt::MessageGUID randomGUID = generateRandomGUID();
    BSLS_ASSERT_OPT(!storage.hasMessage(randomGUID));

    int removedMsgSize = -1;
    ASSERT_EQ(storage.remove(randomGUID, &removedMsgSize),
              mqbi::StorageResult::e_GUID_NOT_FOUND);
}

TEST_F(Test, removeMessage)
// ------------------------------------------------------------------------
// Remove Messages Test
//
// Testing:
//   Verifies the 'remove' in a 'mqbs::FileBackedStorage'. Check GUIDs that
//   in storage as well as GUID not in storage
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("REMOVE MESSAGE");

    const int k_MSG_COUNT = 10;

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    // Check 'put' - To physical storage (StorageKeys = NULL)
    BSLS_ASSERT_OPT(d_tester.addMessages(&guids, k_MSG_COUNT) ==
                    mqbi::StorageResult::e_SUCCESS);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // Remove messages one by one
    for (int i = 0; i < k_MSG_COUNT; ++i) {
        // Remove message
        BSLS_ASSERT_OPT(storage.hasMessage(guids[i]));
        int removedMsgSize = -1;
        ASSERT_EQ_D("message " << i << "[" << guids[i] << "]",
                    storage.remove(guids[i], &removedMsgSize),
                    mqbi::StorageResult::e_SUCCESS);

        // Verify message was removed
        ASSERT(!storage.hasMessage(guids[i]));
        ASSERT_EQ(storage.numMessages(mqbu::StorageKey::k_NULL_KEY),
                  k_MSG_COUNT - i - 1);
        ASSERT_EQ(static_cast<unsigned int>(removedMsgSize), sizeof(int));
    }
}

TEST_F(Test, addVirtualStorage)
// ------------------------------------------------------------------------
// ADD VIRTUAL STORAGE
//
// Testing:
//   Verifies the add operation for virtual storage in a
//   'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ADD VIRTUAL STORAGE");

    mwcu::MemOutStream errDescription(s_allocator_p);
    bsl::string        dummyAppId(s_allocator_p);
    mqbu::StorageKey   dummyAppKey;

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // Virtual Storage- Add
    ASSERT_EQ(storage.numVirtualStorages(), 0);

    ASSERT_EQ(storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1),
              0);
    ASSERT_EQ(storage.addVirtualStorage(errDescription, k_APP_ID2, k_APP_KEY2),
              0);
    ASSERT_EQ(storage.numVirtualStorages(), 2);
}

TEST_F(Test, hasVirtualStorage)
// ------------------------------------------------------------------------
// Virtual Storage Test
//
// Testing:
//   Verifies the 'hasVirtualStorage' check for virtual storages that were
//   successfully added
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("HAS VIRTUAL STORAGE");

    mwcu::MemOutStream errDescription(s_allocator_p);
    bsl::string        dummyAppId(s_allocator_p);
    mqbu::StorageKey   dummyAppKey;

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // Add Virtual Storages
    storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1);
    storage.addVirtualStorage(errDescription, k_APP_ID2, k_APP_KEY2);
    BSLS_ASSERT_OPT(storage.numVirtualStorages() == 2);

    // Verify 'hasVirtualStorage'
    ASSERT(storage.hasVirtualStorage(k_APP_ID1, &dummyAppKey));
    ASSERT_EQ(dummyAppKey, k_APP_KEY1);
    ASSERT(storage.hasVirtualStorage(k_APP_ID2, &dummyAppKey));
    ASSERT_EQ(dummyAppKey, k_APP_KEY2);
    ASSERT(!storage.hasVirtualStorage(k_APP_ID3, &dummyAppKey));
    ASSERT_EQ(dummyAppKey, mqbu::StorageKey::k_NULL_KEY);

    ASSERT(storage.hasVirtualStorage(k_APP_KEY1, &dummyAppId));
    ASSERT_EQ(dummyAppId, k_APP_ID1);
    ASSERT(storage.hasVirtualStorage(k_APP_KEY2, &dummyAppId));
    ASSERT_EQ(dummyAppId, k_APP_ID2);
    ASSERT(!storage.hasVirtualStorage(k_APP_KEY3, &dummyAppId));
    ASSERT_EQ(dummyAppId, "");
}

TEST_F(Test, removeVirtualStorage)
// ------------------------------------------------------------------------
// Virtual Storage Test
//
// Testing:
//   Verifies the remove operation for virtual storage
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("REMOVE VIRTUAL STORAGE");

    mwcu::MemOutStream errDescription(s_allocator_p);
    bsl::string        dummyAppId(s_allocator_p);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // Virtual Storage - Add
    storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1);
    storage.addVirtualStorage(errDescription, k_APP_ID2, k_APP_KEY2);

    // Verify removal
    ASSERT(storage.removeVirtualStorage(k_APP_KEY1));
    ASSERT(!storage.hasVirtualStorage(k_APP_KEY1, &dummyAppId));
    ASSERT_EQ(storage.numVirtualStorages(), 1);

    ASSERT(!storage.removeVirtualStorage(k_APP_KEY3));
    ASSERT_EQ(storage.numVirtualStorages(), 1);

    ASSERT(storage.removeVirtualStorage(k_APP_KEY2));
    ASSERT(!storage.hasVirtualStorage(k_APP_KEY2, &dummyAppId));
    ASSERT_EQ(storage.numVirtualStorages(), 0);
}

TEST_F(BasicTest, put_withVirtualStorages)
// ------------------------------------------------------------------------
// Put Test with virtual storages
//
// Testing:
//   Verifies the 'put' operation in presense of virtual storages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PUT - WITH VIRTUAL STORAGES");

    // CONSTANTS
    const bsls::Types::Int64 k_MSG_LIMIT   = 80;
    const bsls::Types::Int64 k_BYTES_LIMIT = 2048;

    mwcu::MemOutStream errDescription(s_allocator_p);

    int rc = d_tester.configure(k_MSG_LIMIT, k_BYTES_LIMIT);
    BSLS_ASSERT_OPT(rc == 0);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    rc = storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1);
    BSLS_ASSERT_OPT(rc == 0);
    rc = storage.addVirtualStorage(errDescription, k_APP_ID2, k_APP_KEY2);
    BSLS_ASSERT_OPT(rc == 0);

    // Scenario:
    //  Two Virtual Storages.

    const int                k_MSG_COUNT = 20;
    const bsls::Types::Int64 k_MSG_COUNT_INT64 =
        static_cast<bsls::Types::Int64>(k_MSG_COUNT);
    const bsls::Types::Int64 k_BYTE_PER_MSG = static_cast<bsls::Types::Int64>(
        sizeof(int));

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    // Check 'put'- To physical storage (StorageKeys = NULL)
    ASSERT_EQ(d_tester.addMessages(&guids, k_MSG_COUNT),
              mqbi::StorageResult::e_SUCCESS);

    // Verify number of messages for each Virtual storage
    ASSERT_EQ(storage.numMessages(mqbu::StorageKey::k_NULL_KEY),
              k_MSG_COUNT_INT64);
    ASSERT_EQ(storage.numMessages(k_APP_KEY1), k_MSG_COUNT_INT64);
    ASSERT_EQ(storage.numMessages(k_APP_KEY2), k_MSG_COUNT_INT64);

    // Verify number of bytes for each Virtual storage
    ASSERT_EQ(storage.numBytes(mqbu::StorageKey::k_NULL_KEY),
              k_MSG_COUNT_INT64 * k_BYTE_PER_MSG);
    // TBD: In fact, numBytes() == 2 * k_MSG_COUNT_INT64 * k_BYTE_PER_MSG.
    // The current result is due to capacity meter only updating on 'put'
    // to physical storage.
    ASSERT_EQ(storage.numBytes(k_APP_KEY1),
              k_MSG_COUNT_INT64 * k_BYTE_PER_MSG);
    ASSERT_EQ(storage.numBytes(k_APP_KEY2),
              k_MSG_COUNT_INT64 * k_BYTE_PER_MSG);

    // Verify capacity meter updates only on 'put' to physical storage
    ASSERT_EQ(storage.capacityMeter()->bytes(),
              k_MSG_COUNT_INT64 * k_BYTE_PER_MSG);
    ASSERT_EQ(storage.capacityMeter()->messages(), k_MSG_COUNT_INT64);
    ASSERT_EQ(storage.removeAll(mqbu::StorageKey::k_NULL_KEY),
              mqbi::StorageResult::e_SUCCESS);
}

TEST_F(BasicTest, removeAllMessages_appKeyNotFound)
// ------------------------------------------------------------------------
// REMOVE ALL MESSAGES - APPKEY NOT FOUND
//
// Testing:
//   Verifies the 'removeAll' in a presence of multiple storages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------

{
    mwctst::TestHelper::printTestName("REMOVE ALL MESSAGES "
                                      "- APPKEY NOT FOUND");

    mwcu::MemOutStream errDescription(s_allocator_p);

    const bsls::Types::Int64 k_MSG_LIMIT   = 80;
    const bsls::Types::Int64 k_BYTES_LIMIT = 2048;

    BSLS_ASSERT_OPT(d_tester.configure(k_MSG_LIMIT, k_BYTES_LIMIT) == 0);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    BSLS_ASSERT_OPT(
        storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1) == 0);

    // Scenario
    // One virtual Storage
    // - Attempt to 'removeAll' for non-existent 'k_APP_KEY2' and verify
    //   appropriate return code.
    const int                k_MSG_COUNT = 20;
    const bsls::Types::Int64 k_MSG_COUNT_INT64 =
        static_cast<bsls::Types::Int64>(k_MSG_COUNT);
    const bsls::Types::Int64 k_BYTE_PER_MSG = static_cast<bsls::Types::Int64>(
        sizeof(int));

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);
    d_tester.addMessages(&guids, k_MSG_COUNT);

    // Verify 'removeAll' operation
    BSLS_ASSERT_OPT(storage.numMessages(k_NULL_KEY) == k_MSG_COUNT_INT64);
    BSLS_ASSERT_OPT(storage.numBytes(k_NULL_KEY) ==
                    k_MSG_COUNT_INT64 * k_BYTE_PER_MSG);
    BSLS_ASSERT_OPT(storage.numMessages(k_APP_KEY1) == k_MSG_COUNT_INT64);
    BSLS_ASSERT_OPT(storage.numBytes(k_APP_KEY1) ==
                    k_MSG_COUNT_INT64 * k_BYTE_PER_MSG);
    ASSERT_EQ(storage.removeAll(k_APP_KEY2),
              mqbi::StorageResult::e_APPKEY_NOT_FOUND);
}

TEST_F(BasicTest, removeAllMessages)
// ------------------------------------------------------------------------
// RemoveAll Messages Test
//
// Testing:
//   Verifies the 'removeAll' in a presence of multiple storages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------

{
    mwctst::TestHelper::printTestName("Remove All Messages Test");

    mwcu::MemOutStream errDescription(s_allocator_p);

    const bsls::Types::Int64 k_MSG_LIMIT   = 80;
    const bsls::Types::Int64 k_BYTES_LIMIT = 2048;

    BSLS_ASSERT_OPT(d_tester.configure(k_MSG_LIMIT, k_BYTES_LIMIT) == 0);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    BSLS_ASSERT_OPT(
        storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1) == 0);
    BSLS_ASSERT_OPT(
        storage.addVirtualStorage(errDescription, k_APP_ID2, k_APP_KEY2) == 0);

    // Scenario
    // Two Virtual Storages
    // Check 'removeAll' using these appKeys.
    const int                k_MSG_COUNT = 20;
    const bsls::Types::Int64 k_MSG_COUNT_INT64 =
        static_cast<bsls::Types::Int64>(k_MSG_COUNT);
    const bsls::Types::Int64 k_BYTE_PER_MSG = static_cast<bsls::Types::Int64>(
        sizeof(int));

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    ASSERT_EQ(d_tester.addMessages(&guids, k_MSG_COUNT),
              mqbi::StorageResult::e_SUCCESS);

    // Verify 'removeAll' operation
    ASSERT_EQ(storage.numMessages(k_APP_KEY2), k_MSG_COUNT_INT64);
    ASSERT_EQ(storage.numBytes(k_APP_KEY2),
              k_MSG_COUNT_INT64 * k_BYTE_PER_MSG);
    ASSERT_EQ(storage.numMessages(k_APP_KEY1), k_MSG_COUNT_INT64);
    ASSERT_EQ(storage.numBytes(k_APP_KEY1),
              k_MSG_COUNT_INT64 * k_BYTE_PER_MSG);

    ASSERT_EQ(storage.removeAll(k_APP_KEY2), mqbi::StorageResult ::e_SUCCESS);
    ASSERT_EQ(storage.numMessages(k_APP_KEY2), 0);
    ASSERT_EQ(storage.numBytes(k_APP_KEY2), 0);
    ASSERT_EQ(storage.numMessages(k_APP_KEY1), k_MSG_COUNT_INT64);
    ASSERT_EQ(storage.numBytes(k_APP_KEY1),
              k_MSG_COUNT_INT64 * k_BYTE_PER_MSG);

    ASSERT_EQ(storage.removeAll(mqbu::StorageKey::k_NULL_KEY),
              mqbi::StorageResult::e_SUCCESS);
    ASSERT_EQ(storage.numMessages(k_APP_KEY1), 0);
    ASSERT_EQ(storage.numBytes(k_APP_KEY1), 0);
    ASSERT_EQ(storage.numMessages(mqbu::StorageKey::k_NULL_KEY), 0);
    ASSERT_EQ(storage.numBytes(mqbu::StorageKey::k_NULL_KEY), 0);
}

TEST_F(BasicTest, get_withVirtualStorages)
// ------------------------------------------------------------------------
// Get Test With virtual storages
//
// Testing:
//   Verifies the 'get' operation in presence of virtual storages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("Get - with Virtual Storage Test");

    mwcu::MemOutStream errDescription(s_allocator_p);

    const bsls::Types::Int64 k_MSG_LIMIT   = 80;
    const bsls::Types::Int64 k_BYTES_LIMIT = 2048;

    BSLS_ASSERT_OPT(d_tester.configure(k_MSG_LIMIT, k_BYTES_LIMIT) == 0);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1);

    // Scenario
    // Single Virtual Storages

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    // Check 'put'- To physical storage (StorageKeys = NULL)
    ASSERT_EQ(d_tester.addMessages(&guids, 20),
              mqbi::StorageResult::e_SUCCESS);

    // Verify 'get' operation
    mqbi::StorageMessageAttributes attributes;
    bsl::shared_ptr<bdlbb::Blob>   appData;
    bsl::shared_ptr<bdlbb::Blob>   options;

    // 'get' messageAttributes: Should reflect correct number of references and
    // verify data
    ASSERT_EQ(storage.get(&attributes, guids[10]),
              mqbi::StorageResult::e_SUCCESS);
    ASSERT_EQ(attributes.refCount(), 1U);

    // 'get' overload to grab data
    ASSERT_EQ(storage.get(&appData, &options, &attributes, guids[15]),
              mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(attributes.arrivalTimestamp(),
              static_cast<bsls::Types::Uint64>(15));
    ASSERT_EQ(attributes.arrivalTimepoint(), 0LL);

    ASSERT(attributes.messagePropertiesInfo().isPresent());
    ASSERT_EQ(*(reinterpret_cast<int*>(appData->buffer(0).data())), 15);

    ASSERT_EQ(storage.removeAll(mqbu::StorageKey::k_NULL_KEY),
              mqbi::StorageResult::e_SUCCESS);
}

TEST_F(BasicTest, confirm)
// ------------------------------------------------------------------------
// RELEASE REF
//
// Testing:
//   Verifies the 'releaseRef' operation in presence of virtual storages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("RELEASE REF");

    const bsls::Types::Int64 k_MSG_LIMIT   = 80;
    const bsls::Types::Int64 k_BYTES_LIMIT = 2048;

    mwcu::MemOutStream errDescription(s_allocator_p);

    BSLS_ASSERT_OPT(d_tester.configure(k_MSG_LIMIT, k_BYTES_LIMIT) == 0);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1);

    // Scenario:
    // Single Virtual Storage
    // 'get' operation to test references in message attributes.
    // We then use the releaseRef to check the decrease in refCount for the
    // particular message.
    const int  k_MSG_COUNT     = 20;
    const int  dataOffset      = 0;
    const bool useSameGuids    = false;
    const int  defaultRefCount = 2;

    const bsls::Types::Int64 k_BYTE_PER_MSG = static_cast<bsls::Types::Int64>(
        sizeof(int));

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    // Check 'put': To physical storage (StorageKeys = NULL)
    ASSERT_EQ(d_tester.addMessages(&guids,
                                   k_MSG_COUNT,
                                   dataOffset,
                                   useSameGuids,
                                   defaultRefCount),
              mqbi::StorageResult::e_SUCCESS);

    mqbi::StorageMessageAttributes attributes;
    BSLS_ASSERT_OPT(storage.get(&attributes, guids[5]) ==
                    mqbi::StorageResult::e_SUCCESS);
    ASSERT_EQ(attributes.refCount(), 2U);
    // Attempt 'releaseRef' with non-existent GUID
    ASSERT_EQ(storage.confirm(generateUniqueGUID(guids), k_APP_KEY1, 0),
              mqbi::StorageResult::e_GUID_NOT_FOUND);

    // 'releaseRef' on 'APP_KEY1' and verify refCount decreased by 1
    ASSERT_EQ(storage.confirm(guids[5], k_APP_KEY1, 0),
              mqbi::StorageResult::e_NON_ZERO_REFERENCES);

    BSLS_ASSERT_OPT(storage.get(&attributes, guids[5]) ==
                    mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(storage.numMessages(k_APP_KEY1), 19);
    ASSERT_EQ(storage.numBytes(k_APP_KEY1), 19 * k_BYTE_PER_MSG);

    // 'releaseRef' on 'APP_KEY1' *with the same guid* and verify no effect
    ASSERT_EQ(storage.confirm(guids[5], k_APP_KEY1, 0),
              mqbi::StorageResult::e_GUID_NOT_FOUND);

    BSLS_ASSERT_OPT(storage.get(&attributes, guids[5]) ==
                    mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(storage.numMessages(k_APP_KEY1), 19);
    ASSERT_EQ(storage.numBytes(k_APP_KEY1), 19 * k_BYTE_PER_MSG);

    // 'releaseRef' on the physical storage and verify refCount decreased to 0
    ASSERT_EQ(storage.releaseRef(guids[5]),
              mqbi::StorageResult::e_ZERO_REFERENCES);

    BSLS_ASSERT_OPT(storage.removeAll(k_NULL_KEY) ==
                    mqbi::StorageResult::e_SUCCESS);
}

TEST_F(Test, getIterator_noVirtualStorages)
// ------------------------------------------------------------------------
// Iterator Test
//
// Testing:
//   Verifies the iterator in absence of virtual storages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("Iterator- No virtual storages Test");

    const int k_MSG_COUNT = 10;

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    // Put to physical storage: StorageKeys NULL
    ASSERT_EQ(d_tester.addMessages(&guids, k_MSG_COUNT),
              mqbi::StorageResult::e_SUCCESS);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // Check Iterator
    bslma::ManagedPtr<mqbi::StorageIterator> iterator;
    iterator = storage.getIterator(mqbu::StorageKey::k_NULL_KEY);

    int msgData = 0;
    do {
        ASSERT_EQ(iterator->guid(), guids[msgData]);
        ASSERT_EQ(
            *(reinterpret_cast<int*>(iterator->appData()->buffer(0).data())),
            msgData);
        ASSERT_EQ(
            *(reinterpret_cast<int*>(iterator->options()->buffer(0).data())),
            msgData);
        ASSERT_EQ(iterator->attributes().arrivalTimestamp(),
                  static_cast<bsls::Types::Uint64>(msgData));
        msgData++;
        iterator->advance();
    } while (!iterator->atEnd());

    // Check iterator's 'reset'
    iterator->reset();
    ASSERT_EQ(iterator->guid(), guids[0]);

    // Check Iterator from specific point
    msgData = 5;
    ASSERT_EQ(
        storage.getIterator(&iterator, mqbu::StorageKey::k_NULL_KEY, guids[5]),
        mqbi::StorageResult::e_SUCCESS);

    do {
        ASSERT_EQ(iterator->guid(), guids[msgData]);
        ASSERT_EQ(
            *(reinterpret_cast<int*>(iterator->appData()->buffer(0).data())),
            msgData);
        msgData++;
        iterator->advance();
    } while (!iterator->atEnd());

    // Check iterator with random GUID
    bmqt::MessageGUID randomGuid;
    mqbu::MessageGUIDUtil::generateGUID(&randomGuid);
    ASSERT_EQ(storage.getIterator(&iterator,
                                  mqbu::StorageKey::k_NULL_KEY,
                                  randomGuid),
              mqbi::StorageResult::e_GUID_NOT_FOUND);

    ASSERT_EQ(storage.removeAll(mqbu::StorageKey::k_NULL_KEY),
              mqbi::StorageResult::e_SUCCESS);
}

TEST_F(BasicTest, getIterator_withVirtualStorages)
// ------------------------------------------------------------------------
// Iterator Test
//
// Testing:
//   Verifies the iterator in presence of virtual storages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("Iterator Test- In presence of Virtual");

    mwcu::MemOutStream errDescription(s_allocator_p);

    const bsls::Types::Int64 k_MSG_LIMIT   = 80;
    const bsls::Types::Int64 k_BYTES_LIMIT = 2048;

    BSLS_ASSERT_OPT(d_tester.configure(k_MSG_LIMIT, k_BYTES_LIMIT) == 0);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1);
    storage.addVirtualStorage(errDescription, k_APP_ID2, k_APP_KEY2);

    // Scenario:
    // Two Virtual Storages
    // Try iterator for physical storage as well as both of these storages.

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    // Check 'put': To physical storage (StorageKeys = NULL)
    ASSERT_EQ(d_tester.addMessages(&guids, 20),
              mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(d_tester.addMessages(&guids, 20, 20),
              mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(d_tester.addMessages(&guids, 20, 40),
              mqbi::StorageResult::e_SUCCESS);

    // Check Iterator
    bslma::ManagedPtr<mqbi::StorageIterator> iterator;
    int                                      msgData = 0;
    // mqbu::StorageKey::k_NULL_KEY- Gives all messages in physical and virtual
    // (Total 60 messages)
    iterator = storage.getIterator(mqbu::StorageKey::k_NULL_KEY);

    do {
        ASSERT_EQ(iterator->guid(), guids[msgData]);
        ASSERT_EQ(
            *(reinterpret_cast<int*>(iterator->appData()->buffer(0).data())),
            msgData);
        msgData++;
        iterator->advance();
    } while (!iterator->atEnd());

    // Check if count all 60 messages seen
    ASSERT_EQ(msgData, 60);

    // 'k_APP_KEY2'- Also should have all 60 messages
    msgData  = 0;
    iterator = storage.getIterator(k_APP_KEY2);

    do {
        ASSERT_EQ(iterator->guid(), guids[msgData]);
        ASSERT_EQ(
            *(reinterpret_cast<int*>(iterator->appData()->buffer(0).data())),
            msgData);
        msgData++;
        iterator->advance();
    } while (!iterator->atEnd());

    ASSERT_EQ(msgData, 60);

    // 'k_APP_KEY1' Should have 40 messages
    // Without GUIDs - guids[20] to guids[40]
    msgData  = 0;
    iterator = storage.getIterator(k_APP_KEY1);

    for (int i = 20; i < 40; ++i) {
        storage.confirm(guids[i], k_APP_KEY1, 0);
    }

    do {
        // skip the 20 in between
        if (msgData == 20) {
            msgData += 20;
        }
        ASSERT_EQ(iterator->guid(), guids[msgData]);
        ASSERT_EQ(
            *(reinterpret_cast<int*>(iterator->appData()->buffer(0).data())),
            msgData);
        msgData++;
        iterator->advance();
    } while (!iterator->atEnd());

    ASSERT_EQ(msgData, 60);

    ASSERT_EQ(storage.removeAll(mqbu::StorageKey::k_NULL_KEY),
              mqbi::StorageResult::e_SUCCESS);
}

TEST_F(Test, capacityMeter_limitMessages)
// ------------------------------------------------------------------------
// Capacity Meter Test
//
// Testing:
//   Verifies the capacity meter functionality with respect to
//   limits on messages in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("Capacity Meter- Limit Messages");

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    // Put to physical storage: StorageKeys NULL
    ASSERT_EQ(d_tester.addMessages(&guids, k_DEFAULT_MSG),
              mqbi::StorageResult::e_SUCCESS);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    // Access CapacityMeter
    ASSERT_EQ(static_cast<unsigned int>(storage.capacityMeter()->bytes()),
              k_DEFAULT_MSG * sizeof(int));
    ASSERT_EQ(storage.capacityMeter()->messages(), k_DEFAULT_MSG);

    // Try to insert more than Capacity Meter - Check success first time
    ASSERT_EQ(d_tester.addMessages(&guids, 1), mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(storage.capacityMeter()->messages(), k_DEFAULT_MSG + 1);

    // Try to insert more than Capacity Meter - Check failure after it's
    // already full
    ASSERT_EQ(d_tester.addMessages(&guids, 1),
              mqbi::StorageResult::e_LIMIT_MESSAGES);

    ASSERT_EQ(storage.capacityMeter()->messages(), k_DEFAULT_MSG + 1);

    // Finally, remove all
    ASSERT_EQ(storage.removeAll(mqbu::StorageKey::k_NULL_KEY),
              mqbi::StorageResult::e_SUCCESS);
    ASSERT_EQ(storage.capacityMeter()->messages(), 0);
}

TEST_F(BasicTest, capacityMeter_limitBytes)
// ------------------------------------------------------------------------
// Capacity Meter Test
//
// Testing:
//   Verifies the capacity meter functionality with respect to
//   limits on bytes in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("Capacity Meter - Limit Bytes");

    const bsls::Types::Int64 k_MSG_LIMIT   = 30;
    const bsls::Types::Int64 k_BYTES_LIMIT = 80;

    BSLS_ASSERT_OPT(d_tester.configure(k_MSG_LIMIT, k_BYTES_LIMIT) == 0);

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    // Insert Max messages possible in 80bytes
    const int k_MSG_COUNT = 20;
    ASSERT_EQ(d_tester.addMessages(&guids, k_MSG_COUNT),
              mqbi::StorageResult::e_SUCCESS);

    // Try to insert more than Capacity Meter - Check success first time
    ASSERT_EQ(d_tester.addMessages(&guids, 1), mqbi::StorageResult::e_SUCCESS);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    ASSERT_EQ(storage.capacityMeter()->bytes(), 84);

    // Try to insert more than Capacity Meter, check failure after it's full
    ASSERT_EQ(d_tester.addMessages(&guids, 1),
              mqbi::StorageResult::e_LIMIT_BYTES);

    ASSERT_EQ(storage.removeAll(mqbu::StorageKey::k_NULL_KEY),
              mqbi::StorageResult::e_SUCCESS);
    ASSERT_EQ(storage.capacityMeter()->bytes(), 0);
}

TEST_F(GCTest, garbageCollect)
// ------------------------------------------------------------------------
// GARBAGE COLLECT
//
// Testing:
//   Verifies the 'gc' functionality with respect to TTL of messages
//   in a 'mqbs::FileBackedStorage'.
// ------------------------------------------------------------------------

{
    mwctst::TestHelper::printTestName("GARBAGE COLLECT");

    // Set with TTL of 20: control GC test by manipulating secondsFromEpoch
    // input
    const int k_TTL = 20;

    configure(k_TTL);

    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    int k_MSG_COUNT = 10;

    // Use offset of '1' so the first message at timestamp 1 and increments
    // from there
    ASSERT_EQ(tester().addMessages(&guids, k_MSG_COUNT, 1),
              mqbi::StorageResult::e_SUCCESS);

    bsls::Types::Uint64 latestMsgTimestamp;
    bsls::Types::Int64  configuredTtlValue;
    bsls::Types::Uint64 secondsFromEpoch = 5;

    mqbs::ReplicatedStorage& storage = tester().storage();

    // Case 1: Remove Zero messages (secondsFromEpoch = Low Value)
    // Such that '0 < seccondsFromEpoch - msgTimeStamp <= TTL'
    ASSERT_EQ(storage.gcExpiredMessages(&latestMsgTimestamp,
                                        &configuredTtlValue,
                                        secondsFromEpoch),
              0);

    ASSERT_EQ(configuredTtlValue, k_TTL);

    // Case 2: Remove half the messages (secondsFromEpoch = 26).
    // Here Half the messages fail the condition TTL check condition.
    secondsFromEpoch = 26;  // Since TTL is 20 half the messages expire
    ASSERT_EQ(storage.gcExpiredMessages(&latestMsgTimestamp,
                                        &configuredTtlValue,
                                        secondsFromEpoch),
              k_MSG_COUNT / 2);

    // Case 3: Remove all messages (secondsFromEpoch = HighValue).
    // Here all messages expire in the check condition.
    secondsFromEpoch = 100;
    ASSERT_EQ(storage.gcExpiredMessages(&latestMsgTimestamp,
                                        &configuredTtlValue,
                                        secondsFromEpoch),
              k_MSG_COUNT / 2);

    // No messages left
    ASSERT_EQ(storage.numMessages(mqbu::StorageKey::k_NULL_KEY), 0);
    ASSERT_EQ(storage.numBytes(mqbu::StorageKey::k_NULL_KEY), 0);
}

TEST_F(Test, addQueueOpRecordHandle)
{
    // CONSTANTS
    const bsls::Types::Uint64    k_SEQUENCE_NUM     = 1024;
    const int                    k_PRIMARY_LEASE_ID = 17;
    const mqbs::RecordType::Enum k_RECORD_TYPE = mqbs::RecordType::e_QUEUE_OP;
    const bsls::Types::Uint64    k_RECORD_OFFSET = 4096;

    mqbs::DataStoreRecordKey    key(k_SEQUENCE_NUM, k_PRIMARY_LEASE_ID);
    mqbs::DataStoreRecord       record(k_RECORD_TYPE, k_RECORD_OFFSET);
    mqbs::DataStoreRecordHandle handle;
    d_tester.insertDataStoreRecord(&handle, key, record);

    mqbs::ReplicatedStorage& storage = d_tester.storage();

    ASSERT(storage.queueOpRecordHandles().empty());
    storage.addQueueOpRecordHandle(handle);

    ASSERT(storage.queueOpRecordHandles().size() == 1U);
    ASSERT(storage.queueOpRecordHandles()[0] == handle);
}

TEST_F(Test, doNotRecordLastConfirmInPriorityMode)
{
    mwctst::TestHelper::printTestName(
        "Do Not Record Last Confirm In Priority Mode");

    const int                      k_MSG_COUNT = 1;
    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    const MockDataStore&     data_store = d_tester.dataStore();
    mqbs::ReplicatedStorage& storage    = d_tester.storage();

    ASSERT_EQ(data_store.getMessageCounter(), 0ULL);
    ASSERT_EQ(data_store.getConfirmCounter(), 0ULL);
    ASSERT_EQ(data_store.getDeletionCounter(), 0ULL);

    ASSERT_EQ(d_tester.addMessages(&guids, k_MSG_COUNT),
              mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(data_store.getMessageCounter(), 1ULL);
    ASSERT_EQ(data_store.getConfirmCounter(), 0ULL);
    ASSERT_EQ(data_store.getDeletionCounter(), 0ULL);

    ASSERT_EQ(storage.releaseRef(guids[0]),
              mqbi::StorageResult::e_ZERO_REFERENCES);

    ASSERT_EQ(data_store.getMessageCounter(), 1ULL);
    ASSERT_EQ(data_store.getConfirmCounter(), 0ULL);
    ASSERT_EQ(data_store.getDeletionCounter(), 1ULL);
}

TEST_F(Test, doNotRecordLastConfirmInFanoutMode)
{
    mwctst::TestHelper::printTestName(
        "Do Not Record Last Confirm in Fanout Mode");
    mwcu::MemOutStream errDescription(s_allocator_p);

    const MockDataStore&     data_store = d_tester.dataStore();
    mqbs::ReplicatedStorage& storage    = d_tester.storage();

    ASSERT_EQ(storage.addVirtualStorage(errDescription, k_APP_ID1, k_APP_KEY1),
              0);
    ASSERT_EQ(storage.addVirtualStorage(errDescription, k_APP_ID2, k_APP_KEY2),
              0);
    ASSERT_EQ(storage.addVirtualStorage(errDescription, k_APP_ID3, k_APP_KEY3),
              0);

    const int                      k_MSG_COUNT     = 1;
    const int                      dataOffset      = 0;
    const bool                     useSameGuids    = false;
    const int                      defaultRefCount = 3;
    bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);

    ASSERT_EQ(data_store.getMessageCounter(), 0ULL);
    ASSERT_EQ(data_store.getConfirmCounter(), 0ULL);
    ASSERT_EQ(data_store.getDeletionCounter(), 0ULL);

    ASSERT_EQ(d_tester.addMessages(&guids,
                                   k_MSG_COUNT,
                                   dataOffset,
                                   useSameGuids,
                                   defaultRefCount),
              mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(data_store.getMessageCounter(), 1ULL);
    ASSERT_EQ(data_store.getConfirmCounter(), 0ULL);
    ASSERT_EQ(data_store.getDeletionCounter(), 0ULL);

    ASSERT_EQ(storage.confirm(guids[0], k_APP_KEY1, 1),
              mqbi::StorageResult::e_NON_ZERO_REFERENCES);

    ASSERT_EQ(data_store.getMessageCounter(), 1ULL);
    ASSERT_EQ(data_store.getConfirmCounter(), 1ULL);
    ASSERT_EQ(data_store.getDeletionCounter(), 0ULL);

    ASSERT_EQ(storage.confirm(guids[0], k_APP_KEY2, 2),
              mqbi::StorageResult::e_NON_ZERO_REFERENCES);

    ASSERT_EQ(data_store.getMessageCounter(), 1ULL);
    ASSERT_EQ(data_store.getConfirmCounter(), 2ULL);
    ASSERT_EQ(data_store.getDeletionCounter(), 0ULL);

    ASSERT_EQ(storage.confirm(guids[0], k_APP_KEY3, 3),
              mqbi::StorageResult::e_ZERO_REFERENCES);

    int msgSize;
    ASSERT_EQ(storage.remove(guids[0], &msgSize),
              mqbi::StorageResult::e_SUCCESS);

    ASSERT_EQ(data_store.getMessageCounter(), 1ULL);
    ASSERT_EQ(data_store.getConfirmCounter(), 2ULL);
    ASSERT_EQ(data_store.getDeletionCounter(), 1ULL);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    BALL_LOG_SET_CATEGORY("MAIN");

    TEST_PROLOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);

    bmqt::UriParser::initialize(s_allocator_p);
    mwcsys::Time::initialize(s_allocator_p);

    mqbu::MessageGUIDUtil::initialize();

    {
        mqbcfg::AppConfig brokerConfig(s_allocator_p);
        mqbcfg::BrokerConfig::set(brokerConfig);

        bsl::shared_ptr<mwcst::StatContext> statContext =
            mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

        mwctst::runTest(_testCase);
    }

    mwcsys::Time::shutdown();
    bmqt::UriParser::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);
}