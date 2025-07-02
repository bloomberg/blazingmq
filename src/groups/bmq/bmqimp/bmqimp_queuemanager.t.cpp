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

// bmqimp_queuemanager.t.cpp                                          -*-C++-*-
#include <bmqimp_queuemanager.h>

// BMQ
#include <bmqimp_event.h>
#include <bmqimp_stat.h>
#include <bmqp_crc32c.h>
#include <bmqp_protocolutil.h>
#include <bmqp_pusheventbuilder.h>
#include <bmqp_puteventbuilder.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsls_assert.h>
#include <bsls_types.h>

#include <bmqst_statcontext.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_cstring.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

void enableQueueStat(bmqimp::QueueManager::QueueSp& queueSp)
{
    BSLS_ASSERT_SAFE(queueSp != 0);

    bmqimp::QueueState::Enum k_STATE = bmqimp::QueueState::e_OPENED;

    bmqimp::Stat queuesStats(bmqtst::TestHelperUtil::allocator());
    bmqst::StatValue::SnapshotLocation start;
    bmqst::StatValue::SnapshotLocation end;

    bmqst::StatContextConfiguration config(
        "stats",
        bmqtst::TestHelperUtil::allocator());
    config.defaultHistorySize(2);

    bmqst::StatContext rootStatContext(config,
                                       bmqtst::TestHelperUtil::allocator());

    start.setLevel(0).setIndex(0);
    end.setLevel(0).setIndex(1);

    bmqimp::QueueStatsUtil::initializeStats(
        &queuesStats,
        &rootStatContext,
        start,
        end,
        bmqtst::TestHelperUtil::allocator());

    bmqst::StatContext* pStatContext = queuesStats.d_statContext_mp.get();

    queueSp->setState(k_STATE);
    queueSp->registerStatContext(pStatContext);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
namespace {

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//   1) Exercise the basic functionality of the component
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqimp::QueueManager obj(bmqtst::TestHelperUtil::allocator());

    const char k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";

    bmqt::Uri                 uri(k_URI, bmqtst::TestHelperUtil::allocator());
    const bmqt::CorrelationId k_CORID;
    const bmqp::QueueId       k_QUEUE_ID(0, 0);
    const bmqimp::QueueState::Enum k_QUEUE_STATE =
        bmqimp::QueueState::e_OPENING_OPN;

    bsl::vector<bmqimp::QueueManager::QueueSp> queues(
        bmqtst::TestHelperUtil::allocator());
    obj.lookupQueuesByState(&queues, k_QUEUE_STATE);
    BMQTST_ASSERT(obj.lookupQueue(uri).get() == 0);
    BMQTST_ASSERT(obj.lookupQueue(k_CORID).get() == 0);
    BMQTST_ASSERT_EQ(queues.size(), 0U);

    BMQTST_ASSERT_SAFE_FAIL(obj.subStreamCount(
        bsl::string(uri.canonical(), bmqtst::TestHelperUtil::allocator())));
}

static void test2_generateQueueIdTest()
// ------------------------------------------------------------------------
// GENERATE QUEUE ID TEST
//
// Concerns:
//   Check generation of bmqp::QueueId
//
// Plan:
//   1) Generate bmqp::QueueId with different set of arguments
//      ----------------------------------------------
//     |  Known URI | URI has appID | Read flag is set|
//     | ---------------------------------------------|
//     |   No       |    Yes        |     No          |
//     | ---------------------------------------------|
//     |   No       |    No         |     Yes         |
//     | ---------------------------------------------|
//     |   No       |    Yes        |     Yes         |
//     | ---------------------------------------------|
//     |   Yes      |    Yes        |     No          |
//     | ---------------------------------------------|
//     |   Yes      |    Yes        |     Yes         |
//      ----------------------------------------------
//   2) For each case check generated queueId and queueSubId values.
//
// Testing:
//   void
//   QueueManager::generateQueueAndSubQueueId(bmqp::QueueId       *queueId,
//                                            const bmqt::Uri&     uri,
//                                            bsls::Types::Uint64  flags)
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("GENERATE QUEUE ID TEST");

    const char k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";

    bmqt::Uri            uri(k_URI, bmqtst::TestHelperUtil::allocator());
    bmqimp::QueueManager obj(bmqtst::TestHelperUtil::allocator());

    PVV("Invalid cases");
    {
        bmqp::QueueId queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
        bmqt::Uri     emptyUri(bmqtst::TestHelperUtil::allocator());

        // NULL output QueueId
        BMQTST_ASSERT_SAFE_FAIL(obj.generateQueueAndSubQueueId(0, uri, 0));

        // Not valid URI
        BMQTST_ASSERT_SAFE_FAIL(
            obj.generateQueueAndSubQueueId(&queueId, emptyUri, 0));
    }

    PVV("[Uri: unknown] [AppId: set] [Reader flag: not set]");
    {
        bsls::Types::Uint64 flags = 0;
        bmqp::QueueId       queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);

        obj.generateQueueAndSubQueueId(&queueId, uri, flags);

        BMQTST_ASSERT_EQ(queueId.id(), 0);
        BMQTST_ASSERT_EQ(queueId.subId(),
                         bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

        // The second generation increments queueId
        obj.generateQueueAndSubQueueId(&queueId, uri, flags);

        BMQTST_ASSERT_EQ(queueId.id(), 1);
        BMQTST_ASSERT_EQ(queueId.subId(),
                         bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
    }

    PVV("[Uri: unknown] [AppId: not set] [Reader flag: set]");
    {
        bmqt::Uri uriNoId("bmq://ts.trades.myapp/my.queue",
                          bmqtst::TestHelperUtil::allocator());

        bsls::Types::Uint64 flags = 0;
        bmqp::QueueId       queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);

        bmqt::QueueFlagsUtil::setReader(&flags);

        obj.generateQueueAndSubQueueId(&queueId, uriNoId, flags);

        BMQTST_ASSERT_EQ(queueId.id(), 2);
        BMQTST_ASSERT_EQ(queueId.subId(),
                         bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
    }

    PVV("[Uri: unknown] [AppId: set] [Reader flag: set]");
    {
        // Reader flag and a new uri with appId.  QueueId should be incremented
        // and subQueueId set to the initial value.
        unsigned int k_SUB_ID = bmqimp::QueueManager::k_INITIAL_SUBQUEUE_ID;

        bsls::Types::Uint64 flags = 0;
        bmqp::QueueId       queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);

        bmqt::QueueFlagsUtil::setReader(&flags);

        obj.generateQueueAndSubQueueId(&queueId, uri, flags);

        BMQTST_ASSERT_EQ(queueId.id(), 3);
        BMQTST_ASSERT_EQ(queueId.subId(), k_SUB_ID);
    }

    PVV("[Uri: known] [AppId: set] [Reader flag: not set]");
    {
        // Insert a valid 'bmqimp::Queue' object so that uri becomes known for
        // the 'bmqimp::QueueManager'
        bmqimp::QueueManager::QueueSp queueSp;
        bsls::Types::Uint64           flags = 0;

        bmqp::QueueId             queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
        const bmqt::CorrelationId k_CORID = bmqt::CorrelationId::autoValue();

        obj.generateQueueAndSubQueueId(&queueId, uri, flags);

        BMQTST_ASSERT_EQ(queueId.id(), 4);
        BMQTST_ASSERT_EQ(queueId.subId(),
                         bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

        queueSp.createInplace(bmqtst::TestHelperUtil::allocator(),
                              bmqtst::TestHelperUtil::allocator());

        (*queueSp)
            .setUri(uri)
            .setId(queueId.id())
            .setSubQueueId(queueId.subId())
            .setFlags(flags)
            .setCorrelationId(k_CORID);

        obj.insertQueue(queueSp);

        // Now URI is known, queueId should be the same
        obj.generateQueueAndSubQueueId(&queueId, uri, flags);

        BMQTST_ASSERT_EQ(queueId.id(), 4);
        BMQTST_ASSERT_EQ(queueId.subId(),
                         bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
    }

    PVV("[Uri: known] [AppId: set] [Reader flag: set]");
    {
        // With reader flag and a known uri that has appId the subQueueId
        // should be incremented
        bsls::Types::Uint64 flags = 0;
        bmqp::QueueId       queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);

        bmqt::QueueFlagsUtil::setReader(&flags);
        obj.generateQueueAndSubQueueId(&queueId, uri, flags);

        BMQTST_ASSERT_EQ(queueId.id(), 4);
        BMQTST_ASSERT_EQ(queueId.subId(), 1U);

        obj.generateQueueAndSubQueueId(&queueId, uri, flags);

        BMQTST_ASSERT_EQ(queueId.id(), 4);
        BMQTST_ASSERT_EQ(queueId.subId(), 2U);
    }
}

static void test3_insertQueueTest()
// ------------------------------------------------------------------------
// INSERT QUEUE TEST
//
// Concerns:
//   Check insertion of a valid bmqimp::Queue object
//
// Plan:
//   1) Create a valid bmqimp::Queue object
//   2) Check that it can be inserted into bmqimp::QueueManager
//   3) Check that the queue object can be found using
//      bmqimp::QueueManager accessors
//
// Testing:
//   bmqimp::QueueManager::insertQueue(
//                              const bmqimp::QueueManager::QueueSp& queue)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("INSERT QUEUE TEST");

    bmqimp::QueueManager obj(bmqtst::TestHelperUtil::allocator());

    const char k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";

    bmqt::Uri                 uri(k_URI, bmqtst::TestHelperUtil::allocator());
    const bmqt::CorrelationId k_CORID = bmqt::CorrelationId::autoValue();
    bmqimp::QueueManager::QueueSp queueSp;

    // Cannot insert null object
    BMQTST_ASSERT_SAFE_FAIL(obj.insertQueue(queueSp));

    queueSp.createInplace(bmqtst::TestHelperUtil::allocator(),
                          bmqtst::TestHelperUtil::allocator());

    // Cannot insert queue object without queue ID.
    BMQTST_ASSERT_SAFE_FAIL(obj.insertQueue(queueSp));

    bmqp::QueueId queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    queueSp->setId(queueId.id());

    // Cannot insert queue object with invalid queue ID.
    BMQTST_ASSERT_SAFE_FAIL(obj.insertQueue(queueSp));

    queueId.setId(123);
    queueSp->setId(queueId.id());

    // Cannot insert queue object with not generated queue ID.
    BMQTST_ASSERT_SAFE_FAIL(obj.insertQueue(queueSp));

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);

    obj.generateQueueAndSubQueueId(&queueId, uri, flags);

    (*queueSp)
        .setUri(uri)
        .setId(queueId.id())
        .setSubQueueId(queueId.subId())
        .setFlags(flags)
        .setCorrelationId(k_CORID);

    BSLS_ASSERT(queueSp->state() == bmqimp::QueueState::e_CLOSED);

    obj.insertQueue(queueSp);

    bsl::vector<bmqimp::QueueManager::QueueSp> queues(
        bmqtst::TestHelperUtil::allocator());
    obj.lookupQueuesByState(&queues, bmqimp::QueueState::e_CLOSED);

    BMQTST_ASSERT_EQ(queues.size(), 1U);

    BMQTST_ASSERT(obj.lookupQueue(uri) == queueSp);
    BMQTST_ASSERT(obj.lookupQueue(k_CORID) == queueSp);
    BMQTST_ASSERT(obj.lookupQueue(queueId) == queueSp);

    BMQTST_ASSERT(obj.subStreamCount(
                      bsl::string(uri.canonical(),
                                  bmqtst::TestHelperUtil::allocator())) == 0);

    // Cannot insert the second queue object with the same queue and subqueue
    // ID.
    BMQTST_ASSERT_SAFE_FAIL(obj.insertQueue(queueSp));
}

static void test4_lookupQueueByUri()
// ------------------------------------------------------------------------
// LOOKUP QUEUE
//
// Concerns:
//   Check lookup of queue by a:
//     1. Valid URI associated with previously inserted queue
//     2. Valid URI *NOT* associated with a previously inserted queue
//
// Plan:
//   1) Create a valid bmqimp::Queue object having a valid URI with appId
//      and insert into the bmqimp::QueueManager under test.
//   2) Verify that it can be successfully looked up by the URI by the
//      bmqimp::QueueManager
//   2) Verify that a similar, but different, URI leads to unsuccessful
//      lookup by the bmqimp::QueueManager
//
// Testing:
//   bmqimp::QueueManager::lookupQueue(const bmqt::Uri uri);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LOOKUP QUEUE");

    bmqimp::QueueManager obj(bmqtst::TestHelperUtil::allocator());

    const char k_URI1[] = "bmq://ts.trades.myapp/my.queue?id=foo";
    const char k_URI2[] = "bmq://ts.trades.myapp/my.queue?id=bar";

    bmqt::Uri uri1(k_URI1, bmqtst::TestHelperUtil::allocator());
    bmqt::Uri uri2(k_URI2, bmqtst::TestHelperUtil::allocator());
    bmqimp::QueueManager::QueueSp queueSp;
    bmqp::QueueId                 queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    bsls::Types::Uint64           flags   = 0;
    const bmqt::CorrelationId     k_CORID = bmqt::CorrelationId::autoValue();

    queueSp.createInplace(bmqtst::TestHelperUtil::allocator(),
                          bmqtst::TestHelperUtil::allocator());
    bmqt::QueueFlagsUtil::setReader(&flags);
    obj.generateQueueAndSubQueueId(&queueId, uri1, flags);

    (*queueSp)
        .setUri(uri1)
        .setId(queueId.id())
        .setSubQueueId(queueId.subId())
        .setFlags(flags)
        .setCorrelationId(k_CORID);

    obj.insertQueue(queueSp);

    BMQTST_ASSERT(obj.lookupQueue(uri1) == queueSp);
    BMQTST_ASSERT(obj.lookupQueue(uri2) == bmqimp::QueueManager::QueueSp());
}

static void test6_removeQueueTest()
// ------------------------------------------------------------------------
// REMOVE QUEUE TEST
//
// Concerns:
//   Check removing of a valid bmqimp::Queue object as well as a
//   bmqimp::Queue object that has already been removed or simply cannot
//   be found
//
// Plan:
//   1) Insert a valid bmqimp::Queue object into bmqimp::QueueManager
//   2) Check that it can be removed
//   3) Attempt to remove it again and verify that an empty shared pointer
//      is returned.
//
// Testing:
//   bmqimp::QueueManagerQueueManager::QueueSp
//   bmqimp::QueueManagerQueueManager::removeQueue(
//                                              const bmqimp::Queue *queue)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("REMOVE QUEUE TEST");

    bmqimp::QueueManager obj(bmqtst::TestHelperUtil::allocator());

    const char k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";

    bmqt::Uri                 uri(k_URI, bmqtst::TestHelperUtil::allocator());
    const bmqt::CorrelationId k_CORID = bmqt::CorrelationId::autoValue();
    bmqimp::QueueManager::QueueSp queueSp;

    queueSp.createInplace(bmqtst::TestHelperUtil::allocator(),
                          bmqtst::TestHelperUtil::allocator());

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);

    bmqp::QueueId queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    obj.generateQueueAndSubQueueId(&queueId, uri, flags);

    (*queueSp)
        .setUri(uri)
        .setId(queueId.id())
        .setSubQueueId(queueId.subId())
        .setFlags(flags)
        .setCorrelationId(k_CORID);

    obj.insertQueue(queueSp);

    BMQTST_ASSERT(obj.removeQueue(queueSp.get()) == queueSp);
    BMQTST_ASSERT(obj.lookupQueue(uri) == 0);
    BMQTST_ASSERT(obj.lookupQueue(k_CORID) == 0);
    BMQTST_ASSERT(obj.lookupQueue(queueId) == 0);

    // Cannot remove the same queue object twice
    BMQTST_ASSERT((!(obj.removeQueue(queueSp.get()))));
}

static void test8_substreamCountTest()
// ------------------------------------------------------------------------
// SUBSTREAM COUNT TEST
//
// Concerns:
//   Exercise substream count manipulations
//
// Plan:
//   1) Check correctness of substream count accessors and manipulators
//
// Testing:
//   unsigned int
//   QueueManager::subStreamCount(const bsl::string& canonicalUri) const
//   void
//   QueueManager::incrementSubStreamCount(const bsl::string& canonicalUri)
//   void
//   QueueManager::decrementSubStreamCount(const bsl::string& canonicalUri)
//   void
//   QueueManager::resetSubStreamCount(const bsl::string& canonicalUri)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("RESET STATE TEST");

    bmqimp::QueueManager obj(bmqtst::TestHelperUtil::allocator());

    const char k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";

    bmqt::Uri uri(k_URI, bmqtst::TestHelperUtil::allocator());
    bmqimp::QueueManager::QueueSp queueSp;
    bmqp::QueueId                 queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    bsls::Types::Uint64           flags   = 0;
    const bmqt::CorrelationId     k_CORID = bmqt::CorrelationId::autoValue();

    queueSp.createInplace(bmqtst::TestHelperUtil::allocator(),
                          bmqtst::TestHelperUtil::allocator());
    bmqt::QueueFlagsUtil::setReader(&flags);
    obj.generateQueueAndSubQueueId(&queueId, uri, flags);

    (*queueSp)
        .setUri(uri)
        .setId(queueId.id())
        .setSubQueueId(queueId.subId())
        .setFlags(flags)
        .setCorrelationId(k_CORID);

    bsl::string uriCanonical(uri.canonical(),
                             bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_SAFE_FAIL(obj.incrementSubStreamCount(uriCanonical));
    BMQTST_ASSERT_SAFE_FAIL(obj.decrementSubStreamCount(uriCanonical));
    BMQTST_ASSERT_SAFE_FAIL(obj.resetSubStreamCount(uriCanonical));
    BMQTST_ASSERT_SAFE_FAIL(obj.subStreamCount(uriCanonical));

    obj.insertQueue(queueSp);

    BMQTST_ASSERT(obj.lookupQueue(queueId) == queueSp);
    BMQTST_ASSERT_EQ(obj.subStreamCount(uriCanonical), 0U);

    BMQTST_ASSERT_SAFE_FAIL(obj.decrementSubStreamCount(uriCanonical));

    obj.incrementSubStreamCount(uriCanonical);
    obj.incrementSubStreamCount(uriCanonical);
    obj.incrementSubStreamCount(uriCanonical);

    BMQTST_ASSERT_EQ(obj.subStreamCount(uriCanonical), 3U);

    obj.decrementSubStreamCount(uriCanonical);

    BMQTST_ASSERT_EQ(obj.subStreamCount(uriCanonical), 2U);

    obj.resetSubStreamCount(uriCanonical);

    BMQTST_ASSERT_EQ(obj.subStreamCount(uriCanonical), 0U);

    obj.resetState();

    BMQTST_ASSERT_SAFE_FAIL(obj.incrementSubStreamCount(uriCanonical));
    BMQTST_ASSERT_SAFE_FAIL(obj.decrementSubStreamCount(uriCanonical));
    BMQTST_ASSERT_SAFE_FAIL(obj.resetSubStreamCount(uriCanonical));
    BMQTST_ASSERT_SAFE_FAIL(obj.subStreamCount(uriCanonical));
}

static void test9_pushStatsTest()
// --------------------------------------------------------------------
// BASIC PUSH EVENT STATISTICS TEST
//
// Concerns:
//   Check basic behavior of the bmqimp::QueueManager statistics interface
//
// Plan:
//   1) Create a bmqimp::QueueManager object and populate it with a valid
//      bmqimp::Queue with some generated 'queueId'
//   2) Create a bmqp::Event that contains a single PUSH message with the
//      same 'queueId'
//   3) Collect bmqimp::QueueManager PUSH message statistics providing
//      a valid bmqp::PushMessageIterator and compare results with
//      expected values
//
// Testing:
//   bmqimp::QueueManager::updateStatsOnPushEvent
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BASIC PUSH EVENT STATISTICS");

    const char  k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";
    const char* buffer  = "abcdefghijklmnopqrstuvwxyz";

    const bmqt::CorrelationId k_CORID = bmqt::CorrelationId::autoValue();
    const bmqt::MessageGUID   k_GUID;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PushEventBuilder peb(blobSpPool.get(),
                               bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob payload(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqt::Uri   uri(k_URI, bmqtst::TestHelperUtil::allocator());
    bmqimp::QueueManager::QueueSp queueSp;
    bmqp::QueueId                 queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    bmqp::PushMessageIterator     msgIterator(&bufferFactory,
                                          bmqtst::TestHelperUtil::allocator());
    bmqimp::QueueManager::EventInfos eventInfos(
        bmqtst::TestHelperUtil::allocator());
    int                 eventMessageCount = 0;
    bsls::Types::Uint64 flags             = 0;

    bool hasMessageWithMultipleSubQueueIds = false;

    bmqimp::QueueManager obj(bmqtst::TestHelperUtil::allocator());

    // Fails due to empty iterator
    BMQTST_ASSERT_SAFE_FAIL(obj.onPushEvent(&eventInfos,
                                            &eventMessageCount,
                                            &hasMessageWithMultipleSubQueueIds,
                                            msgIterator));

    // Make a valid iterator
    obj.generateQueueAndSubQueueId(&queueId, uri, flags);
    bdlbb::BlobUtil::append(&payload, buffer, bsl::strlen(buffer));

    int rc = peb.packMessage(payload,
                             queueId.id(),
                             k_GUID,
                             flags,
                             bmqt::CompressionAlgorithmType::e_NONE);

    BSLS_ASSERT_SAFE(rc == bmqt::EventBuilderResult::e_SUCCESS);

    bmqp::Event rawEvent(peb.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(true == rawEvent.isValid());
    BSLS_ASSERT_SAFE(true == rawEvent.isPushEvent());

    rawEvent.loadPushMessageIterator(&msgIterator);

    // Fails due to no queues
    BMQTST_ASSERT_SAFE_FAIL(obj.onPushEvent(&eventInfos,
                                            &eventMessageCount,
                                            &hasMessageWithMultipleSubQueueIds,
                                            msgIterator));

    // Add a queue with enabled statistics
    queueSp.createInplace(bmqtst::TestHelperUtil::allocator(),
                          bmqtst::TestHelperUtil::allocator());
    bmqt::QueueFlagsUtil::setReader(&flags);

    (*queueSp)
        .setUri(uri)
        .setId(queueId.id())
        .setSubQueueId(queueId.subId())
        .setFlags(flags)
        .setCorrelationId(k_CORID);

    enableQueueStat(queueSp);

    obj.insertQueue(queueSp);

    rc = obj.onPushEvent(&eventInfos,
                         &eventMessageCount,
                         &hasMessageWithMultipleSubQueueIds,
                         msgIterator);

    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(eventInfos.size(), 1U);
    BMQTST_ASSERT_EQ(eventInfos[0].d_ids.size(), 1U);
    BMQTST_ASSERT_EQ(eventInfos[0].d_ids[0].d_header.queueId(), queueId.id());

    const unsigned int sId = bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;
    BMQTST_ASSERT_EQ(eventInfos[0].d_ids[0].d_subscriptionId, sId);
    BMQTST_ASSERT_EQ(eventMessageCount, 1);
    BMQTST_ASSERT_EQ(hasMessageWithMultipleSubQueueIds, false);
}

static void test10_putStatsTest()
// --------------------------------------------------------------------
// BASIC PUT EVENT STATISTICS TEST
//
// Concerns:
//   Check basic behavior of the bmqimp::QueueManager statistics interface
//
// Plan:
//   1) Create a bmqimp::QueueManager object and populate it with a valid
//      bmqimp::Queue with some generated 'queueId'
//   2) Create a bmqp::Event that contains a single PUT message with the
//      same 'queueId'
//   3) Collect bmqimp::QueueManager PUT message statistics providing
//      a valid bmqp::PutMessageIterator and compare results with
//      expected values
//
// Testing:
//   bmqimp::QueueManager::updateStatsOnPutEvent
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BASIC PUT EVENT STATISTICS");

    const char  k_URI[]   = "bmq://ts.trades.myapp/my.queue?id=my.app";
    const char* k_PAYLOAD = "abcdefghijklmnopqrstuvwxyz";

    const int k_PAYLOAD_LEN = bsl::strlen(k_PAYLOAD);

    const bmqt::CorrelationId k_CORID = bmqt::CorrelationId::autoValue();
    const bmqt::MessageGUID   k_GUID;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PutEventBuilder peb(blobSpPool.get(),
                              bmqtst::TestHelperUtil::allocator());
    bmqt::Uri             uri(k_URI, bmqtst::TestHelperUtil::allocator());
    bmqimp::QueueManager::QueueSp queueSp;
    bmqp::QueueId                 queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    bmqp::PutMessageIterator      msgIterator(&bufferFactory,
                                         bmqtst::TestHelperUtil::allocator());
    int                           eventMessageCount = 0;
    bsls::Types::Uint64           flags             = 0;

    bmqimp::QueueManager obj(bmqtst::TestHelperUtil::allocator());

    // Fails due to empty iterator
    BMQTST_ASSERT_SAFE_FAIL(
        obj.updateStatsOnPutEvent(&eventMessageCount, msgIterator));

    // Make a valid iterator
    obj.generateQueueAndSubQueueId(&queueId, uri, flags);

    peb.startMessage();
    peb.setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN);

    int rc = peb.packMessage(queueId.id());

    BSLS_ASSERT_SAFE(rc == bmqt::EventBuilderResult::e_SUCCESS);

    bmqp::Event rawEvent(peb.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(true == rawEvent.isValid());
    BSLS_ASSERT_SAFE(true == rawEvent.isPutEvent());

    rawEvent.loadPutMessageIterator(&msgIterator);

    // Fails due to no queues
    BMQTST_ASSERT_SAFE_FAIL(
        obj.updateStatsOnPutEvent(&eventMessageCount, msgIterator));

    // Add a queue with enabled statistics
    queueSp.createInplace(bmqtst::TestHelperUtil::allocator(),
                          bmqtst::TestHelperUtil::allocator());
    bmqt::QueueFlagsUtil::setWriter(&flags);

    (*queueSp)
        .setUri(uri)
        .setId(queueId.id())
        .setSubQueueId(queueId.subId())
        .setFlags(flags)
        .setCorrelationId(k_CORID);

    enableQueueStat(queueSp);

    obj.insertQueue(queueSp);

    rc = obj.updateStatsOnPutEvent(&eventMessageCount, msgIterator);

    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(eventMessageCount, 1);
}

}  // close unnamed namespace

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
    case 10: test10_putStatsTest(); break;
    case 9: test9_pushStatsTest(); break;
    case 8: test8_substreamCountTest(); break;
    case 6: test6_removeQueueTest(); break;
    case 4: test4_lookupQueueByUri(); break;
    case 3: test3_insertQueueTest(); break;
    case 2: test2_generateQueueIdTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();
    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);

    // Check for default allocator is explicitly disabled as
    // 'bmqimp::QueueManager::insertQueue' may allocate
    // temporaries with default allocator.
}
