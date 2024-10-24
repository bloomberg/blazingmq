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

// mqbblp_routers.t.cpp                                               -*-C++-*-
#include <mqbblp_routers.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_event.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqt_messageguid.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbmock_queue.h>
#include <mqbmock_queuehandle.h>
#include <mqbs_inmemorystorage.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsls_annotation.h>
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace BloombergLP {

/// Mechanism to mock QueueHandle and StorageIterator for Router testing.
struct TestStorage {
    unsigned int                             d_subQueueId;
    mqbconfm::Domain                         d_domainCfg;
    mqbu::CapacityMeter                      d_capacityMeter;
    mqbu::StorageKey                         d_storageKey;
    mqbs::InMemoryStorage                    d_storage;
    bslma::ManagedPtr<mqbi::StorageIterator> d_iterator;
    bdlbb::PooledBlobBufferFactory           d_bufferFactory;

    bsl::shared_ptr<mqbmock::Queue> d_queue_sp;

    bslma::Allocator* d_allocator_p;

    TestStorage(unsigned int subQueueId, bslma::Allocator* allocator)
    : d_subQueueId(subQueueId)
    , d_domainCfg(allocator)
    , d_capacityMeter("cm", allocator)
    , d_storageKey(d_subQueueId)
    , d_storage(bmqt::Uri("uri", allocator),
                d_storageKey,
                1,
                d_domainCfg,
                &d_capacityMeter,
                allocator)
    , d_iterator(d_storage.getIterator(mqbu::StorageKey()))
    , d_bufferFactory(32, allocator)
    , d_queue_sp(new(*allocator) mqbmock::Queue(0, allocator), allocator)
    , d_allocator_p(allocator)

    {
        bmqt::MessageGUID guid;
        guid.fromHex("00000000000000000000000000000001");
        mqbi::StorageMessageAttributes     attributes;
        const bsl::shared_ptr<bdlbb::Blob> appData(
            new (*allocator) bdlbb::Blob(&d_bufferFactory, allocator),
            allocator);
        const bsl::shared_ptr<bdlbb::Blob> options(
            new (*allocator) bdlbb::Blob(&d_bufferFactory, allocator),
            allocator);
        // TODO: put data for Expression evaluation

        mqbi::StorageResult::Enum rc =
            d_storage.put(&attributes, guid, appData, options);

        ASSERT_EQ(rc, mqbi::StorageResult::e_SUCCESS);
    }

    ~TestStorage() { d_storage.removeAll(mqbu::StorageKey()); }

    mqbmock::QueueHandle getHandle()
    {
        bsl::shared_ptr<mqbi::QueueHandleRequesterContext> clientContext(
            new (*d_allocator_p)
                mqbi::QueueHandleRequesterContext(d_allocator_p),
            d_allocator_p);
        bmqp_ctrlmsg::QueueHandleParameters handleParameters(d_allocator_p);

        return mqbmock::QueueHandle(d_queue_sp,
                                    clientContext,
                                    0,  // stats,
                                    handleParameters,
                                    d_allocator_p);
    }
};

struct Visitor {
    mqbi::QueueHandle*         d_handle;
    unsigned int               d_subQueueId;
    mqbblp::Routers::Consumer* d_consumer;

    Visitor()
    : d_handle(0)
    , d_subQueueId(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID)
    , d_consumer(0)
    {
        // NOTHING
    }
    bool visit(const mqbblp::Routers::Subscription* subscription)
    {
        d_subQueueId = subscription->subQueueId();
        d_consumer   = subscription->consumer();
        d_handle     = subscription->handle();

        return true;
    }
};

struct Item {
    int d_i;

    Item(int i)
    : d_i(i)
    {
    }
    bool operator==(const Item& other) const { return d_i == other.d_i; }
};

}

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_registry()
// ------------------------------------------------------------------------
// Testing mqbblp::Routers::Registry::record functionality
//
//  1. Subsequent call with the same 'key' returns the original item.
//  2. Release of last mqbblp::Routers::Registry::SharedItem should erase
//     the item from the registry.
// ------------------------------------------------------------------------
{
    typedef mqbblp::Routers::Registry<int, Item> Registry;

    Registry registry(s_allocator_p);
    Item     value13(13);
    Item     value14(14);
    int      key12 = 12;

    {
        Registry::SharedItem si1 = registry.record(key12, value13);
        ASSERT_EQ(registry.size(), size_t(1));

        ASSERT(si1->value() == value13);

        Registry::SharedItem si2 = registry.record(key12, value14);

        ASSERT_EQ(registry.size(), size_t(1));

        ASSERT(si2->value() == value13);
    }
    ASSERT_EQ(registry.size(), size_t(0));
}

static void test2_priority()
// ------------------------------------------------------------------------
// Testing mqbblp::Routers::Expressions, mqbblp::Routers::Consumers, and
// mqbblp::Routers::Priority combination for memory leaks.
//
// Priority (weakly) registers Subscribers, Subscriber owns SharedItem
// reference to Consumer.
// ------------------------------------------------------------------------
{
    mqbblp::Routers::Expressions expressions(s_allocator_p);

    expressions.record(bmqp_ctrlmsg::Expression(s_allocator_p),
                       mqbblp::Routers::Expression());

    mqbi::QueueHandle* handle = 0;
    ++handle;
    const bmqp_ctrlmsg::StreamParameters   streamParameters(s_allocator_p);
    mqbblp::Routers::Consumers             consumers(s_allocator_p);
    const unsigned int                     subQueueId = 13;
    mqbblp::Routers::Consumers::SharedItem consumer   = consumers.record(
        handle,
        mqbblp::Routers::Consumer(streamParameters,
                                  subQueueId,
                                  s_allocator_p));

    mqbblp::Routers::Priority priority(s_allocator_p);

    priority.d_subscribers.record(handle,
                                  mqbblp::Routers::Subscriber(consumer,
                                                              s_allocator_p));
}

static void test3_parse()
// ------------------------------------------------------------------------
// Testing mqbblp::Routers::AppContext::load and iterateGroups methods.
//
//  1. One handle with one subscription with one consumer.
//  2. One handle with one subscription with two consumers at different
//     priorities.
//  3. Two handles each with one subscription with two consumers at
//     different priorities.
// ------------------------------------------------------------------------
{
    bmqp_ctrlmsg::StreamParameters       streamParams(s_allocator_p);
    bmqp::SchemaLearner                  schemaLearner(s_allocator_p);
    mqbblp::Routers::QueueRoutingContext queueContext(schemaLearner,
                                                      s_allocator_p);
    unsigned int                         subQueueId = 13;
    TestStorage                          storage(subQueueId, s_allocator_p);

    mqbmock::QueueHandle handle1 = storage.getHandle();

    bmqp_ctrlmsg::SubQueueIdInfo subStreamInfo1(s_allocator_p);

    bsl::string  appId("foo", s_allocator_p);
    unsigned int upstreamSubQueueId = 1;
    subStreamInfo1.appId()          = appId;
    subStreamInfo1.subId()          = subQueueId;

    streamParams.appId() = appId;
    streamParams.subscriptions().resize(1);

    handle1.registerSubStream(subStreamInfo1,
                              upstreamSubQueueId,
                              mqbi::QueueCounts(1, 0));

    int priorityCount = 2;
    int priority      = 2;

    Visitor visitor1, visitor2;

    {
        bmqp_ctrlmsg::Subscription& subscription =
            streamParams.subscriptions()[0];

        subscription.expression() = bmqp_ctrlmsg::Expression();
        subscription.consumers().resize(1);

        {
            bmqp_ctrlmsg::ConsumerInfo& ci = subscription.consumers()[0];

            ci.consumerPriority()       = priority;
            ci.consumerPriorityCount()  = priorityCount;
            ci.maxUnconfirmedMessages() = 1024;
            ci.maxUnconfirmedBytes()    = 1024;
        }

        handle1.setStreamParameters(streamParams);

        // One consumer with one subscription
        {
            mqbblp::Routers::AppContext appContext(queueContext,
                                                   s_allocator_p);
            bmqu::MemOutStream          errorStream(s_allocator_p);
            appContext.load(&handle1,
                            &errorStream,
                            subStreamInfo1.subId(),
                            upstreamSubQueueId,
                            streamParams,
                            0);
            ASSERT_EQ(errorStream.str(), "");
            ASSERT_EQ(appContext.finalize(), size_t(priorityCount));
            appContext.registerSubscriptions();

            mqbblp::Routers::RoundRobin router(appContext.d_priorities);
            ASSERT_EQ(router.iterateGroups(
                          bdlf::BindUtil::bind(&Visitor::visit,
                                               &visitor1,
                                               bdlf::PlaceHolders::_1),
                          storage.d_iterator.get()),
                      mqbblp::Routers::e_SUCCESS);

            ASSERT_EQ(&handle1, visitor1.d_handle);
            ASSERT_EQ(subStreamInfo1.subId(), visitor1.d_subQueueId);
        }
        // One consumer with two subscriptions
        subscription.consumers().resize(2);
        {
            bmqp_ctrlmsg::ConsumerInfo& ci = subscription.consumers()[1];
            ci.consumerPriority()          = priority - 1;
            ci.consumerPriorityCount()     = priorityCount;
            ci.maxUnconfirmedMessages()    = 1024;
            ci.maxUnconfirmedBytes()       = 1024;
        }
        handle1.setStreamParameters(streamParams);
        {
            mqbblp::Routers::AppContext appContext(queueContext,
                                                   s_allocator_p);
            bmqu::MemOutStream          errorStream(s_allocator_p);

            appContext.load(&handle1,
                            &errorStream,
                            subStreamInfo1.subId(),
                            upstreamSubQueueId,
                            streamParams,
                            0);
            ASSERT_EQ(errorStream.str(), "");
            ASSERT_EQ(appContext.finalize(), size_t(priorityCount));
            appContext.registerSubscriptions();

            mqbblp::Routers::RoundRobin router(appContext.d_priorities);

            ASSERT_EQ(router.iterateGroups(
                          bdlf::BindUtil::bind(&Visitor::visit,
                                               &visitor1,
                                               bdlf::PlaceHolders::_1),
                          storage.d_iterator.get()),
                      mqbblp::Routers::e_SUCCESS);

            ASSERT_EQ(&handle1, visitor1.d_handle);
            ASSERT_EQ(subStreamInfo1.subId(), visitor1.d_subQueueId);
        }
        // Two consumers with two subscriptions
        mqbmock::QueueHandle         handle2 = storage.getHandle();
        bmqp_ctrlmsg::SubQueueIdInfo subStreamInfo2(s_allocator_p);

        subStreamInfo2.appId() = appId;
        subStreamInfo2.subId() = 14;

        handle2.registerSubStream(subStreamInfo2,
                                  upstreamSubQueueId,
                                  mqbi::QueueCounts(1, 0));

        handle2.setStreamParameters(streamParams);

        {
            mqbblp::Routers::AppContext appContext(queueContext,
                                                   s_allocator_p);
            bmqu::MemOutStream          errorStream(s_allocator_p);

            mqbblp::Routers::RoundRobin router(appContext.d_priorities);

            appContext.load(&handle1,
                            &errorStream,
                            subStreamInfo1.subId(),
                            upstreamSubQueueId,
                            streamParams,
                            0);
            ASSERT_EQ(errorStream.str(), "");
            appContext.load(&handle2,
                            &errorStream,
                            subStreamInfo2.subId(),
                            upstreamSubQueueId,
                            streamParams,
                            0);
            ASSERT_EQ(errorStream.str(), "");
            ASSERT_EQ(appContext.finalize(), size_t(2 * priorityCount));
            appContext.registerSubscriptions();

            ASSERT_EQ(router.iterateGroups(
                          bdlf::BindUtil::bind(&Visitor::visit,
                                               &visitor1,
                                               bdlf::PlaceHolders::_1),
                          storage.d_iterator.get()),
                      mqbblp::Routers::e_SUCCESS);

            ASSERT_EQ(router.iterateGroups(
                          bdlf::BindUtil::bind(&Visitor::visit,
                                               &visitor2,
                                               bdlf::PlaceHolders::_1),
                          storage.d_iterator.get()),
                      mqbblp::Routers::e_SUCCESS);

            ASSERT_EQ(visitor2.d_handle, visitor1.d_handle);
            ASSERT_EQ(visitor2.d_subQueueId, visitor1.d_subQueueId);

            ASSERT_EQ(router.iterateGroups(
                          bdlf::BindUtil::bind(&Visitor::visit,
                                               &visitor2,
                                               bdlf::PlaceHolders::_1),
                          storage.d_iterator.get()),
                      mqbblp::Routers::e_SUCCESS);

            if (visitor1.d_handle == &handle1) {
                ASSERT_EQ(subStreamInfo1.subId(), visitor1.d_subQueueId);

                ASSERT_EQ(&handle2, visitor2.d_handle);
                ASSERT_EQ(subStreamInfo2.subId(), visitor2.d_subQueueId);
            }
            else {
                ASSERT_EQ(&handle2, visitor1.d_handle);
                ASSERT_EQ(subStreamInfo2.subId(), visitor1.d_subQueueId);

                ASSERT_EQ(&handle1, visitor2.d_handle);
                ASSERT_EQ(subStreamInfo1.subId(), visitor2.d_subQueueId);
            }
        }

        ASSERT_EQ(handle2.unregisterSubStream(subStreamInfo1,
                                              mqbi::QueueCounts(1, 0),
                                              false),
                  true);
    }

    ASSERT_EQ(handle1.unregisterSubStream(subStreamInfo1,
                                          mqbi::QueueCounts(1, 0),
                                          false),
              true);
}

static void test4_generate()
// ------------------------------------------------------------------------
//  Testing mqbblp::Routers::AppContext::generate method
//
//  Parse two handles each with one subscription with two consumers at
//  different priorities.
//  Generated streamParameters contain one subscription with two
// accumulated consumers.
// ------------------------------------------------------------------------
{
    bmqp_ctrlmsg::StreamParameters       in(s_allocator_p);
    bmqp::SchemaLearner                  schemaLearner(s_allocator_p);
    mqbblp::Routers::QueueRoutingContext queueContext(schemaLearner,
                                                      s_allocator_p);
    unsigned int                         upstreamSubQueueId = 1;
    mqbblp::Routers::AppContext appContext(queueContext, s_allocator_p);
    bmqu::MemOutStream          errorStream(s_allocator_p);

    bsl::string           appId("foo", s_allocator_p);
    int                   priorityCount = 2;
    int                   priority      = 2;
    mqbmock::QueueHandle* handle        = 0;
    unsigned int          subQueueId    = 13;

    in.appId() = appId;
    in.subscriptions().resize(1);

    {
        bmqp_ctrlmsg::Subscription& subscription = in.subscriptions()[0];

        subscription.consumers().resize(2);
        {
            bmqp_ctrlmsg::ConsumerInfo& ci = subscription.consumers()[0];

            ci.consumerPriority()      = priority;
            ci.consumerPriorityCount() = priorityCount;
        }
        {
            bmqp_ctrlmsg::ConsumerInfo& ci = subscription.consumers()[1];

            ci.consumerPriority()      = priority - 1;
            ci.consumerPriorityCount() = priorityCount;
        }
    }

    appContext
        .load(++handle, &errorStream, subQueueId, upstreamSubQueueId, in, 0);
    ASSERT_EQ(errorStream.str(), "");
    appContext.load(++handle,
                    &errorStream,
                    subQueueId + 1,
                    upstreamSubQueueId,
                    in,
                    0);
    ASSERT_EQ(errorStream.str(), "");
    ASSERT_EQ(appContext.finalize(), 2 * size_t(priorityCount));

    bmqp_ctrlmsg::StreamParameters out(s_allocator_p);
    appContext.generate(&out);

    ASSERT_EQ(out.subscriptions().size(), size_t(1));
    const bmqp_ctrlmsg::Subscription& subscription = out.subscriptions()[0];

    ASSERT_EQ(subscription.consumers().size(), size_t(2));
    {
        const bmqp_ctrlmsg::ConsumerInfo& ci = subscription.consumers()[0];

        ASSERT_EQ(ci.consumerPriority(), priority);
        ASSERT_EQ(ci.consumerPriorityCount(), 2 * priorityCount);
    }
    {
        const bmqp_ctrlmsg::ConsumerInfo& ci = subscription.consumers()[1];

        ASSERT_EQ(ci.consumerPriority(), priority - 1);
        ASSERT_EQ(ci.consumerPriorityCount(), 2 * priorityCount);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Initialize Crc32c
    bmqp::Crc32c::initialize();

    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(s_allocator_p);
    bmqt::UriParser::initialize(s_allocator_p);

    mqbcfg::AppConfig brokerConfig(s_allocator_p);
    mqbcfg::BrokerConfig::set(brokerConfig);
    // expect BALL_LOG_ERROR
    switch (_testCase) {
    case 0:
    case 1: test1_registry(); break;
    case 2: test2_priority(); break;
    case 3: test3_parse(); break;
    case 4: test4_generate(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqt::UriParser::shutdown();
    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
