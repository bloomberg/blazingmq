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

// mqbs_virtualstorage.t.cpp                                          -*-C++-*-
#include <mqbs_virtualstorage.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_storage.h>
#include <mqbmock_cluster.h>
#include <mqbmock_domain.h>
#include <mqbmock_queue.h>
#include <mqbmock_queueengine.h>
#include <mqbs_inmemorystorage.h>
#include <mqbstat_brokerstats.h>
#include <mqbu_messageguidutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// MWC
#include <mwcu_memoutstream.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
// - breathingTest
// - unsupportedOperations
// - put
// - hasMessage
// - getMessageSize
// - get
// - remove
// - removeAll
// - getIterator
//-----------------------------------------------------------------------------

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

// CONSTANTS
const int              k_PARTITION_ID = 1;
const char             k_HEX_QUEUE[]  = "ABCDEF1234";
const char             k_URI_STR[]    = "bmq://mydomain/testqueue";
const char             k_APP_ID[]     = "ABCDEF1111";
const mqbu::StorageKey k_QUEUE_KEY(mqbu::StorageKey::HexRepresentation(),
                                   k_HEX_QUEUE);
const mqbu::StorageKey k_APP_KEY(mqbu::StorageKey::HexRepresentation(),
                                 k_APP_ID);
const unsigned int     k_DEFAULT_MSG_SIZE = 25;

// ALIASES
typedef bsl::vector<bmqt::MessageGUID> MessageGuids;
typedef MessageGuids::iterator         MessageGuidsI;
typedef MessageGuids::const_iterator   MessageGuidsCI;

const bsls::Types::Int64 k_INT64_ZERO = 0;
const bsls::Types::Int64 k_INT64_MAX =
    bsl::numeric_limits<bsls::Types::Int64>::max();

// FUNCTIONS

/// Load into the specified `guids` a new message GUID unique from all
/// existing GUIDs in `guids`, and return it.
static bmqt::MessageGUID generateUniqueGUID(MessageGuids* guids)
{
    bmqt::MessageGUID uniqueGUID;
    do {
        mqbu::MessageGUIDUtil::generateGUID(&uniqueGUID);
    } while (bsl::find(guids->begin(), guids->end(), uniqueGUID) !=
             guids->end());

    guids->push_back(uniqueGUID);
    return uniqueGUID;
}

// CLASSES
// =============
// struct Tester
// =============
struct Tester {
  private:
    // DATA
    bdlbb::PooledBlobBufferFactory          d_bufferFactory;
    mqbmock::Cluster                        d_mockCluster;
    mqbmock::Domain                         d_mockDomain;
    mqbmock::Queue                          d_mockQueue;
    mqbmock::QueueEngine                    d_mockQueueEngine;
    bsl::shared_ptr<mqbi::Storage>          d_storage_sp;
    bslma::ManagedPtr<mqbs::VirtualStorage> d_virtualStorage_mp;
    bslma::Allocator*                       d_allocator_p;

  public:
    // CREATORS
    Tester(const bslstl::StringRef& uri         = k_URI_STR,
           const mqbu::StorageKey&  queueKey    = k_QUEUE_KEY,
           int                      partitionId = k_PARTITION_ID,
           bsls::Types::Int64       ttlSeconds  = k_INT64_MAX,
           bslma::Allocator*        allocator   = s_allocator_p)
    : d_bufferFactory(1024, allocator)
    , d_mockCluster(&d_bufferFactory, allocator)
    , d_mockDomain(&d_mockCluster, allocator)
    , d_mockQueue(&d_mockDomain, allocator)
    , d_mockQueueEngine(allocator)
    , d_allocator_p(allocator)
    {
        d_mockDomain.capacityMeter()->setLimits(k_INT64_MAX, k_INT64_MAX);
        d_mockQueue._setQueueEngine(&d_mockQueueEngine);

        mqbconfm::Domain domainCfg;
        domainCfg.deduplicationTimeMs() = 0;  // No history
        domainCfg.messageTtl()          = ttlSeconds;

        d_storage_sp.load(new (*d_allocator_p) mqbs::InMemoryStorage(
                              bmqt::Uri(uri, d_allocator_p),
                              queueKey,
                              partitionId,
                              domainCfg,
                              d_mockDomain.capacityMeter(),
                              bmqp::RdaInfo(),
                              d_allocator_p),
                          d_allocator_p);
        d_storage_sp->setQueue(&d_mockQueue);
        BSLS_ASSERT_OPT(d_storage_sp->queue() == &d_mockQueue);

        d_virtualStorage_mp.load(new (*d_allocator_p)
                                     mqbs::VirtualStorage(d_storage_sp.get(),
                                                          k_APP_ID,
                                                          k_APP_KEY,
                                                          d_allocator_p),
                                 d_allocator_p);
    }

    ~Tester()
    {
        d_virtualStorage_mp->removeAll(mqbu::StorageKey::k_NULL_KEY);
        d_virtualStorage_mp->close();
        d_storage_sp->removeAll(mqbu::StorageKey::k_NULL_KEY);
        d_storage_sp->close();
    }

    // MANIPULATORS
    int configure(bsls::Types::Int64 msgCapacity,
                  bsls::Types::Int64 byteCapacity,
                  double             msgWatermarkRatio  = 0.8,
                  double             byteWatermarkRatio = 0.8,
                  bsls::Types::Int64 messageTtl         = k_INT64_MAX)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(d_storage_sp && "Storage was not created");

        mqbconfm::Storage config;
        mqbconfm::Limits  limits;

        config.makeInMemory();

        limits.messages()               = msgCapacity;
        limits.messagesWatermarkRatio() = msgWatermarkRatio;
        limits.bytes()                  = byteCapacity;
        limits.bytesWatermarkRatio()    = byteWatermarkRatio;

        mwcu::MemOutStream errDescription(s_allocator_p);
        return d_storage_sp->configure(errDescription,
                                       config,
                                       limits,
                                       messageTtl,
                                       0);  // maxDeliveryAttempts
    }

    mqbi::StorageResult::Enum addPhysicalMessages(const MessageGuids guids,
                                                  const int dataOffset = 0)
    {
        for (size_t i = 0; i != guids.size(); i++) {
            const bmqt::MessageGUID& guid = guids[i];

            mqbi::StorageMessageAttributes attributes(
                static_cast<bsls::Types::Uint64>(dataOffset + i),
                1,
                bmqp::MessagePropertiesInfo::makeNoSchema(),
                bmqt::CompressionAlgorithmType::e_NONE);

            const bsl::shared_ptr<bdlbb::Blob> appDataPtr(
                new (*d_allocator_p)
                    bdlbb::Blob(&d_bufferFactory, d_allocator_p),
                d_allocator_p);
            const int data = i + dataOffset;
            bdlbb::BlobUtil::append(&(*appDataPtr),
                                    reinterpret_cast<const char*>(&data),
                                    static_cast<int>(sizeof(int)));

            mqbi::StorageResult::Enum rc =
                d_storage_sp->put(&attributes, guid, appDataPtr, appDataPtr);
            PV("rc = " << rc);
            if (rc != mqbi::StorageResult::e_SUCCESS) {
                return rc;  // RETURN
            }
        }

        return mqbi::StorageResult::e_SUCCESS;
    }

    mqbs::VirtualStorage& vStorage() { return *d_virtualStorage_mp.ptr(); }

    mqbi::Storage& storage() { return *d_storage_sp.ptr(); }

    mqbmock::Queue& mockQueue() { return d_mockQueue; }
};

}  // close anonymous namespace

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
    mwctst::TestHelper::printTestName("BREATHING TEST");
    Tester tester;

    ASSERT_EQ(tester.vStorage().queueUri().asString(), k_URI_STR);
    ASSERT_EQ(tester.vStorage().queueKey(), k_QUEUE_KEY);
    ASSERT_EQ(tester.vStorage().appId(), k_APP_ID);
    ASSERT_EQ(tester.vStorage().appKey(), k_APP_KEY);
    ASSERT_EQ(tester.vStorage().config(), mqbconfm::Storage());
    ASSERT_EQ(tester.vStorage().isPersistent(), false);
    ASSERT_EQ(tester.vStorage().numMessages(k_APP_KEY), k_INT64_ZERO);
    ASSERT_EQ(tester.vStorage().numBytes(k_APP_KEY), k_INT64_ZERO);
}

static void test2_unsupportedOperations()
// ------------------------------------------------------------------------
// UNSUPPORTED OPERATIONS
//
// Concerns:
//   A 'mqbs::VirtualStorage' implements, but does not adhere to, some
//   operations declared in its interface, in the sense that these methods
//   fail to execute.
//
// Testing:
//   queue()
//   capacityMeter()
//   isEmpty()
//   setQueue(...)
//   put(msgGUID, attributes, appData, options, storageKeys)
//   releaseRef(...)
//   dispatcherFlush()
//   numVirtualStorages()
//   hasVirtualStorage(...)
//   loadVirtualStorageDetails(...)
//   gc(...)
//   addVirtualStorage(...)
//   removeVirtualStorage(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("UNSUPPORTED OPRATIONS");
    Tester tester;

    bmqt::MessageGUID              guid;
    mqbi::StorageMessageAttributes attributes;
    bsl::shared_ptr<bdlbb::Blob>   appData;
    bsl::shared_ptr<bdlbb::Blob>   options;
    mqbi::Storage::AppIdKeyPairs   appIdKeyPairs;
    bsls::Types::Uint64            latestMsgTimestamp;
    bsls::Types::Int64             configuredTtlValue;
    bsls::Types::Uint64            secondsFromEpoch = 5;
    mwcu::MemOutStream             errDescription(s_allocator_p);

    ASSERT_OPT_FAIL(tester.vStorage().queue());
    ASSERT_OPT_FAIL(tester.vStorage().capacityMeter());
    ASSERT_OPT_FAIL(tester.vStorage().isEmpty());
    ASSERT_OPT_FAIL(tester.vStorage().setQueue(&tester.mockQueue()));
    ASSERT_OPT_FAIL(
        tester.vStorage().put(&attributes, guid, appData, options));
    ASSERT_OPT_FAIL(tester.vStorage().releaseRef(guid, k_APP_KEY, 0));
    ASSERT_OPT_FAIL(tester.vStorage().dispatcherFlush(false));
    ASSERT_OPT_FAIL(tester.vStorage().numVirtualStorages());
    ASSERT_OPT_FAIL(tester.vStorage().hasVirtualStorage(k_APP_KEY));
    ASSERT_OPT_FAIL(tester.vStorage().hasVirtualStorage(k_APP_ID));
    ASSERT_OPT_FAIL(
        tester.vStorage().loadVirtualStorageDetails(&appIdKeyPairs));
    ASSERT_OPT_FAIL(tester.vStorage().gcExpiredMessages(&latestMsgTimestamp,
                                                        &configuredTtlValue,
                                                        secondsFromEpoch));
    ASSERT_OPT_FAIL(tester.vStorage().addVirtualStorage(errDescription,
                                                        k_APP_ID,
                                                        k_APP_KEY));
    ASSERT_OPT_FAIL(tester.vStorage().removeVirtualStorage(k_APP_KEY));
}

static void test3_put()
// ------------------------------------------------------------------------
// PUT
//
// Concerns:
//   Verify that 'put' works as intended.
//
// Testing:
//   put(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PUT");
    Tester tester;

    // Put 20 new messages
    MessageGuids      guids;
    bmqt::MessageGUID newGuid;
    const int         k_MSG_COUNT = 20;
    for (int i = 1; i <= k_MSG_COUNT; ++i) {
        newGuid = generateUniqueGUID(&guids);
        ASSERT_EQ(
            tester.vStorage().put(newGuid,
                                  k_DEFAULT_MSG_SIZE,
                                  bmqp::RdaInfo(),
                                  bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID),
            mqbi::StorageResult::e_SUCCESS);
        ASSERT_EQ(tester.vStorage().numMessages(k_APP_KEY), i);
        ASSERT_EQ(tester.vStorage().numBytes(k_APP_KEY),
                  i * k_DEFAULT_MSG_SIZE);
    }

    int msgSize;
    for (MessageGuidsCI cit = guids.cbegin(); cit != guids.cend(); ++cit) {
        const bmqt::MessageGUID& guid = *cit;
        ASSERT(tester.vStorage().hasMessage(guid));
        ASSERT_EQ(tester.vStorage().getMessageSize(&msgSize, guid),
                  mqbi::StorageResult::e_SUCCESS);
        ASSERT_EQ(static_cast<unsigned int>(msgSize), k_DEFAULT_MSG_SIZE);
    }

    // Inserting duplicated messageGUID should not succeed and should have no
    // side effect
    for (MessageGuidsCI cit = guids.cbegin(); cit != guids.cend(); ++cit) {
        const bmqt::MessageGUID& guid = *cit;
        ASSERT_EQ(
            tester.vStorage().put(guid,
                                  k_DEFAULT_MSG_SIZE,
                                  bmqp::RdaInfo(),
                                  bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID),
            mqbi::StorageResult::e_GUID_NOT_UNIQUE);
        // For now, this still returns e_SUCCESS.
        ASSERT_EQ(tester.vStorage().numMessages(k_APP_KEY), k_MSG_COUNT);
        ASSERT_EQ(tester.vStorage().numBytes(k_APP_KEY),
                  k_MSG_COUNT * k_DEFAULT_MSG_SIZE);
        ASSERT(tester.vStorage().hasMessage(guid));
        ASSERT_EQ(tester.vStorage().getMessageSize(&msgSize, guid),
                  mqbi::StorageResult::e_SUCCESS);
        ASSERT_EQ(static_cast<unsigned int>(msgSize), k_DEFAULT_MSG_SIZE);
    }
}

static void test4_hasMessage()
// ------------------------------------------------------------------------
// HAS MESSAGE
//
// Concerns:
//   Verify that 'hasMessage' works as intended.
//
// Testing:
//   hasMessage(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("HAS MESSAGE");
    Tester tester;

    // Put 10 new messages
    MessageGuids      guids;
    bmqt::MessageGUID newGuid;
    const int         k_MSG_COUNT = 10;
    for (int i = 1; i <= k_MSG_COUNT; ++i) {
        newGuid = generateUniqueGUID(&guids);
        BSLS_ASSERT_OPT(
            tester.vStorage().put(newGuid,
                                  k_DEFAULT_MSG_SIZE,
                                  bmqp::RdaInfo(),
                                  bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID) ==
            mqbi::StorageResult::e_SUCCESS);
        BSLS_ASSERT_OPT(tester.vStorage().numMessages(k_APP_KEY) == i);
        BSLS_ASSERT_OPT(tester.vStorage().numBytes(k_APP_KEY) ==
                        i * k_DEFAULT_MSG_SIZE);
    }

    for (MessageGuidsCI cit = guids.cbegin(); cit != guids.cend(); ++cit) {
        ASSERT(tester.vStorage().hasMessage(*cit));
    }

    // Check 'hasMessage' with non-existent GUIDs
    for (int i = 1; i <= k_MSG_COUNT; ++i) {
        newGuid = generateUniqueGUID(&guids);
        ASSERT(!tester.vStorage().hasMessage(newGuid));
    }
}

static void test5_getMessageSize()
// ------------------------------------------------------------------------
// GET MESSAGE SIZE
//
// Concerns:
//   Verify that 'getMessageSize' works as intended.
//
// Testing:
//   getMessageSize(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GET MESSAGE SIZE");
    Tester tester;

    // Put 10 new messages
    MessageGuids      guids;
    bmqt::MessageGUID newGuid;
    const int         k_MSG_COUNT = 10;
    for (int i = 1; i <= k_MSG_COUNT; ++i) {
        newGuid = generateUniqueGUID(&guids);
        BSLS_ASSERT_OPT(
            tester.vStorage().put(newGuid,
                                  k_DEFAULT_MSG_SIZE,
                                  bmqp::RdaInfo(),
                                  bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID) ==
            mqbi::StorageResult::e_SUCCESS);
        BSLS_ASSERT_OPT(tester.vStorage().numMessages(k_APP_KEY) == i);
        BSLS_ASSERT_OPT(tester.vStorage().numBytes(k_APP_KEY) ==
                        i * k_DEFAULT_MSG_SIZE);
    }

    int msgSize;
    for (MessageGuidsCI cit = guids.cbegin(); cit != guids.cend();
         ++cit, msgSize = 0) {
        const bmqt::MessageGUID& guid = *cit;
        BSLS_ASSERT_OPT(tester.vStorage().hasMessage(guid));
        ASSERT_EQ(tester.vStorage().getMessageSize(&msgSize, guid),
                  mqbi::StorageResult::e_SUCCESS);
        ASSERT_EQ(static_cast<unsigned int>(msgSize), k_DEFAULT_MSG_SIZE);
    }

    // Check 'getMessageSize' with non-existent GUIDs
    for (int i = 1; i <= k_MSG_COUNT; ++i, msgSize = 0) {
        newGuid = generateUniqueGUID(&guids);
        BSLS_ASSERT_OPT(!tester.vStorage().hasMessage(newGuid));
        ASSERT_EQ(tester.vStorage().getMessageSize(&msgSize, newGuid),
                  mqbi::StorageResult::e_GUID_NOT_FOUND);
        ASSERT_EQ(msgSize, 0);
    }
}

static void test6_get()
// ------------------------------------------------------------------------
// GET
//
// Concerns:
//   Verify that 'get' works as intended.
//
// Testing:
//   get(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GET");
    Tester tester;

    // Put 10 new messages
    MessageGuids      guids;
    bmqt::MessageGUID newGuid;
    const int         k_MSG_COUNT = 10;
    for (int i = 1; i <= k_MSG_COUNT; ++i) {
        newGuid = generateUniqueGUID(&guids);
        BSLS_ASSERT_OPT(
            tester.vStorage().put(newGuid,
                                  k_DEFAULT_MSG_SIZE,
                                  bmqp::RdaInfo(),
                                  bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID) ==
            mqbi::StorageResult::e_SUCCESS);
        BSLS_ASSERT_OPT(tester.vStorage().numMessages(k_APP_KEY) == i);
        BSLS_ASSERT_OPT(tester.vStorage().numBytes(k_APP_KEY) ==
                        i * k_DEFAULT_MSG_SIZE);
    }

    // Since mqbs::VirtualStorage::get() retrieves the physical message
    // from its underlying physical storage, we must store the message
    // there as well.
    BSLS_ASSERT_OPT(tester.configure(k_INT64_MAX, k_INT64_MAX) == 0);
    BSLS_ASSERT_OPT(tester.addPhysicalMessages(guids) ==
                    mqbi::StorageResult::e_SUCCESS);

    // Check 'get' overloads
    for (int i = 0; i < k_MSG_COUNT; ++i) {
        {
            mqbi::StorageMessageAttributes attributes;
            ASSERT_EQ(tester.storage().get(&attributes, guids[i]),
                      mqbi::StorageResult::e_SUCCESS);
            ASSERT_EQ(attributes.arrivalTimestamp(),
                      static_cast<bsls::Types::Uint64>(i));
            ASSERT_EQ(attributes.refCount(), static_cast<unsigned int>(1));
            ASSERT(attributes.messagePropertiesInfo().isPresent());
        }

        {
            mqbi::StorageMessageAttributes attributes;
            bsl::shared_ptr<bdlbb::Blob>   appData;
            bsl::shared_ptr<bdlbb::Blob>   options;
            ASSERT_EQ(tester.storage().get(&appData,
                                           &options,
                                           &attributes,
                                           guids[i]),
                      mqbi::StorageResult::e_SUCCESS);

            ASSERT_EQ(attributes.arrivalTimestamp(),
                      static_cast<bsls::Types::Uint64>(i));
            ASSERT_EQ(attributes.refCount(), static_cast<unsigned int>(1));
            ASSERT(attributes.messagePropertiesInfo().isPresent());
            ASSERT_EQ(*(reinterpret_cast<int*>(appData->buffer(0).data())), i);
        }
    }

    // Check 'get' with a non-existent GUID
    mqbi::StorageMessageAttributes attributes;
    bsl::shared_ptr<bdlbb::Blob>   appData;
    bsl::shared_ptr<bdlbb::Blob>   options;
    ASSERT_EQ(tester.storage().get(&attributes, generateUniqueGUID(&guids)),
              mqbi::StorageResult::e_GUID_NOT_FOUND);
    ASSERT_EQ(tester.storage().get(&appData,
                                   &options,
                                   &attributes,
                                   generateUniqueGUID(&guids)),
              mqbi::StorageResult::e_GUID_NOT_FOUND);
}

static void test7_remove()
// ------------------------------------------------------------------------
// REMOVE
//
// Concerns:
//   Verify that 'remove' works as intended.
//
// Testing:
//   remove(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("REMOVE");
    Tester tester;

    // Put 20 new messages
    MessageGuids      guids;
    bmqt::MessageGUID newGuid;
    const int         k_MSG_COUNT = 20;
    for (int i = 1; i <= k_MSG_COUNT; ++i) {
        newGuid = generateUniqueGUID(&guids);

        BSLS_ASSERT_OPT(
            tester.vStorage().put(newGuid,
                                  k_DEFAULT_MSG_SIZE,
                                  bmqp::RdaInfo(),
                                  bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID) ==
            mqbi::StorageResult::e_SUCCESS);
        BSLS_ASSERT_OPT(tester.vStorage().numMessages(k_APP_KEY) == i);
        BSLS_ASSERT_OPT(tester.vStorage().numBytes(k_APP_KEY) ==
                        i * k_DEFAULT_MSG_SIZE);
    }

    // Remove half of those messages
    int msgSize;
    for (int i = 0; i < k_MSG_COUNT / 2; ++i) {
        const bmqt::MessageGUID& guid = guids[i];
        BSLS_ASSERT_OPT(tester.vStorage().hasMessage(guid));
        BSLS_ASSERT_OPT(tester.vStorage().getMessageSize(&msgSize, guid) ==
                        mqbi::StorageResult::e_SUCCESS);
        BSLS_ASSERT_OPT(static_cast<unsigned int>(msgSize) ==
                        k_DEFAULT_MSG_SIZE);

        ASSERT_EQ(tester.vStorage().remove(guid, &msgSize),
                  mqbi::StorageResult::e_SUCCESS)
        ASSERT_EQ(static_cast<unsigned int>(msgSize), k_DEFAULT_MSG_SIZE);
        ASSERT(!tester.vStorage().hasMessage(guid));
        ASSERT_EQ(tester.vStorage().getMessageSize(&msgSize, guid),
                  mqbi::StorageResult::e_GUID_NOT_FOUND);
        ASSERT_EQ(tester.vStorage().numMessages(k_APP_KEY),
                  k_MSG_COUNT - i - 1);
        ASSERT_EQ(tester.vStorage().numBytes(k_APP_KEY),
                  (k_MSG_COUNT - i - 1) * k_DEFAULT_MSG_SIZE);
    }

    // Sanity check: the remaining half is not removed
    for (int i = k_MSG_COUNT / 2; i < k_MSG_COUNT; ++i) {
        const bmqt::MessageGUID& guid = guids[i];
        ASSERT(tester.vStorage().hasMessage(guid));
    }
}

static void test8_removeAll()
// ------------------------------------------------------------------------
// REMOVE ALL
//
// Concerns:
//   Verify that 'removeAll' works as intended.
//
// Testing:
//   removeAll(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("REMOVE ALL");
    Tester tester;

    // Put 20 new messages
    MessageGuids      guids;
    bmqt::MessageGUID newGuid;
    const int         k_MSG_COUNT = 20;
    for (int i = 1; i <= k_MSG_COUNT; ++i) {
        newGuid = generateUniqueGUID(&guids);

        BSLS_ASSERT_OPT(
            tester.vStorage().put(newGuid,
                                  k_DEFAULT_MSG_SIZE,
                                  bmqp::RdaInfo(),
                                  bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID) ==
            mqbi::StorageResult::e_SUCCESS);
        BSLS_ASSERT_OPT(tester.vStorage().numMessages(k_APP_KEY) == i);
        BSLS_ASSERT_OPT(tester.vStorage().numBytes(k_APP_KEY) ==
                        i * k_DEFAULT_MSG_SIZE);
    }

    // Remove all messages
    ASSERT_EQ(tester.vStorage().removeAll(k_APP_KEY),
              mqbi::StorageResult::e_SUCCESS);
    ASSERT_EQ(tester.vStorage().numMessages(k_APP_KEY), 0);
    ASSERT_EQ(tester.vStorage().numBytes(k_APP_KEY), 0);

    int msgSize;
    for (MessageGuidsCI cit = guids.cbegin(); cit != guids.cend(); ++cit) {
        const bmqt::MessageGUID& guid = *cit;
        ASSERT(!tester.vStorage().hasMessage(guid));
        ASSERT_EQ(tester.vStorage().getMessageSize(&msgSize, guid),
                  mqbi::StorageResult::e_GUID_NOT_FOUND);
    }
}

static void test9_getIterator()
// ------------------------------------------------------------------------
// GET ITERATOR
//
// Concerns:
//   Verify that 'getIterator' works as intended.
//
// Testing:
//   getIterator(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("GET ITERATOR");
    Tester tester;

    // Put 10 new messages
    MessageGuids      guids;
    bmqt::MessageGUID newGuid;
    const int         k_MSG_COUNT = 10;
    for (int i = 1; i <= k_MSG_COUNT; ++i) {
        newGuid = generateUniqueGUID(&guids);
        BSLS_ASSERT_OPT(
            tester.vStorage().put(newGuid,
                                  k_DEFAULT_MSG_SIZE,
                                  bmqp::RdaInfo(),
                                  bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID) ==
            mqbi::StorageResult::e_SUCCESS);
        BSLS_ASSERT_OPT(tester.vStorage().numMessages(k_APP_KEY) == i);
        BSLS_ASSERT_OPT(tester.vStorage().numBytes(k_APP_KEY) ==
                        i * k_DEFAULT_MSG_SIZE);
    }

    // Since mqbs::VirtualStorageIterator iterates through physical messages
    // from its underlying physical storage, we must store the message
    // there as well.
    BSLS_ASSERT_OPT(tester.configure(k_INT64_MAX, k_INT64_MAX) == 0);
    BSLS_ASSERT_OPT(tester.addPhysicalMessages(guids) ==
                    mqbi::StorageResult::e_SUCCESS);

    // Check Iterator
    bslma::ManagedPtr<mqbi::StorageIterator> iterator;
    iterator = tester.vStorage().getIterator(k_APP_KEY);

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
    ASSERT_EQ(tester.vStorage().getIterator(&iterator, k_APP_KEY, guids[5]),
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
    bmqt::MessageGUID randomGuid = generateUniqueGUID(&guids);
    ASSERT_EQ(tester.vStorage().getIterator(&iterator, k_APP_KEY, randomGuid),
              mqbi::StorageResult::e_GUID_NOT_FOUND);

    ASSERT_EQ(tester.vStorage().removeAll(k_APP_KEY),
              mqbi::StorageResult::e_SUCCESS);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);

    bmqt::UriParser::initialize(s_allocator_p);

    {
        mqbcfg::AppConfig brokerConfig(s_allocator_p);
        mqbcfg::BrokerConfig::set(brokerConfig);

        bsl::shared_ptr<mwcst::StatContext> statContext =
            mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

        switch (_testCase) {
        case 0:
        case 9: test9_getIterator(); break;
        case 8: test8_removeAll(); break;
        case 7: test7_remove(); break;
        case 6: test6_get(); break;
        case 5: test5_getMessageSize(); break;
        case 4: test4_hasMessage(); break;
        case 3: test3_put(); break;
        case 2: test2_unsupportedOperations(); break;
        case 1: test1_breathingTest(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            s_testStatus = -1;
        } break;
        }
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);
}
