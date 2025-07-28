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

// mqbblp_pushstream.t.cpp                                            -*-C++-*-
#include <mqbblp_pushstream.h>

// MQB
#include <mqbmock_cluster.h>
#include <mqbmock_domain.h>

// BMQ
#include <bmqp_messageguidgenerator.h>

#include <mqbcfg_messages.h>
#include <mqbs_inmemorystorage.h>

#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_limits.h>
#include <bsl_memory.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_basic()
{
    bmqtst::TestHelper::printTestName("PushStream basic test");

    bsl::shared_ptr<bdlma::ConcurrentPool> pushElementsPool(
        bsl::allocate_shared<bdlma::ConcurrentPool>(
            bmqtst::TestHelperUtil::allocator(),
            sizeof(mqbblp::PushStream::Element),
            bmqtst::TestHelperUtil::allocator()));

    mqbblp::PushStream                                 ps(pushElementsPool,
                          bmqtst::TestHelperUtil::allocator());
    unsigned int                                       subQueueId = 0;
    bsl::shared_ptr<mqbblp::RelayQueueEngine_AppState> app;  // unused
    bmqp::SubQueueInfo                                 subscription;

    mqbblp::PushStream::iterator itGuid = ps.findOrAppendMessage(
        bmqp::MessageGUIDGenerator::testGUID());

    mqbblp::PushStream::Apps::iterator itApp =
        ps.d_apps.emplace(subQueueId, app).first;

    mqbblp::PushStream::Element* element =
        ps.create(subscription.rdaInfo(), subscription.id(), itGuid, itApp);

    ps.add(element);
    ps.remove(element, true);
}

static void test2_iterations()
{
    bmqtst::TestHelper::printTestName("PushStream basic test");

    // Imitate {m1, a1}, {m2, a2}, {m1, a2}, {m2, a1}

    bsl::shared_ptr<bdlma::ConcurrentPool> pushElementsPool(
        bsl::allocate_shared<bdlma::ConcurrentPool>(
            bmqtst::TestHelperUtil::allocator(),
            sizeof(mqbblp::PushStream::Element),
            bmqtst::TestHelperUtil::allocator()));

    mqbblp::PushStream ps(pushElementsPool,
                          bmqtst::TestHelperUtil::allocator());
    unsigned int       subQueueId1 = 1;
    unsigned int       subQueueId2 = 2;

    bsl::shared_ptr<mqbblp::RelayQueueEngine_AppState> unused;

    bmqp::SubQueueInfo subscription1(1);
    bmqp::SubQueueInfo subscription2(2);

    mqbblp::PushStream::iterator itGuid1 = ps.findOrAppendMessage(
        bmqp::MessageGUIDGenerator::testGUID());

    mqbblp::PushStream::Apps::iterator itApp1 =
        ps.d_apps.emplace(subQueueId1, unused).first;

    mqbblp::PushStream::Element* element1 = ps.create(subscription1.rdaInfo(),
                                                      subscription1.id(),
                                                      itGuid1,
                                                      itApp1);

    ps.add(element1);

    mqbblp::PushStream::iterator itGuid2 = ps.findOrAppendMessage(
        bmqp::MessageGUIDGenerator::testGUID());

    mqbblp::PushStream::Apps::iterator itApp2 =
        ps.d_apps.emplace(subQueueId2, unused).first;

    mqbblp::PushStream::Element* element2 = ps.create(subscription2.rdaInfo(),
                                                      subscription2.id(),
                                                      itGuid2,
                                                      itApp2);

    ps.add(element2);

    mqbblp::PushStream::Element* element3 = ps.create(subscription2.rdaInfo(),
                                                      subscription2.id(),
                                                      itGuid1,
                                                      itApp2);

    ps.add(element3);

    mqbblp::PushStream::Element* element4 = ps.create(subscription1.rdaInfo(),
                                                      subscription1.id(),
                                                      itGuid2,
                                                      itApp1);

    ps.add(element4);

    mqbu::CapacityMeter dummyCapacityMeter(
        "dummy",
        0,
        bmqtst::TestHelperUtil::allocator());
    bmqt::Uri        dummyUri("dummy", bmqtst::TestHelperUtil::allocator());
    mqbmock::Cluster dummyCluster(bmqtst::TestHelperUtil::allocator());
    mqbmock::Domain  dummyDomain(&dummyCluster,
                                bmqtst::TestHelperUtil::allocator());
    mqbconfm::Domain dummyDomainConfig(bmqtst::TestHelperUtil::allocator());

    mqbs::InMemoryStorage dummyStorage(dummyUri,
                                       mqbu::StorageKey::k_NULL_KEY,
                                       &dummyDomain,
                                       mqbs::DataStore::k_INVALID_PARTITION_ID,
                                       dummyDomainConfig,
                                       &dummyCapacityMeter,
                                       bmqtst::TestHelperUtil::allocator());

    mqbconfm::Storage config;
    mqbconfm::Limits  limits;

    config.makeInMemory();

    limits.messages() = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.bytes()    = bsl::numeric_limits<bsls::Types::Int64>::max();

    bmqu::MemOutStream errorDescription(bmqtst::TestHelperUtil::allocator());
    dummyStorage.configure(errorDescription,
                           config,
                           limits,
                           bsl::numeric_limits<bsls::Types::Int64>::max(),
                           0);

    {
        mqbblp::PushStreamIterator pit(&dummyStorage,
                                       &ps,
                                       ps.d_stream.begin());

        BMQTST_ASSERT(!pit.atEnd());
        BMQTST_ASSERT_EQ(pit.numApps(), 2u);

        BMQTST_ASSERT_EQ(element1, pit.element(0));
        BMQTST_ASSERT_EQ(element3, pit.element(1));

        BMQTST_ASSERT(pit.advance());
        BMQTST_ASSERT_EQ(pit.numApps(), 2u);

        BMQTST_ASSERT_EQ(element2, pit.element(0));
        BMQTST_ASSERT_EQ(element4, pit.element(1));

        BMQTST_ASSERT(!pit.advance());
    }

    {
        mqbblp::VirtualPushStreamIterator vit(subQueueId1,
                                              &dummyStorage,
                                              &ps,
                                              ps.d_stream.begin());

        BMQTST_ASSERT(!vit.atEnd());
        BMQTST_ASSERT_EQ(vit.numApps(), 1u);

        BMQTST_ASSERT_EQ(element1, vit.element(0));

        vit.advance();

        BMQTST_ASSERT(!vit.atEnd());
        BMQTST_ASSERT_EQ(vit.numApps(), 1u);

        BMQTST_ASSERT_EQ(element4, vit.element(0));

        vit.advance();

        BMQTST_ASSERT(vit.atEnd());
    }

    ps.remove(element2, true);
    ps.remove(element3, true);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 1: test1_basic(); break;
    case 2: test2_iterations(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
