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

// mqbstat_queuestats.t.cpp                                           -*-C++-*-
#include <mqbstat_queuestats.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbmock_cluster.h>
#include <mqbmock_domain.h>
#include <mqbstat_brokerstats.h>

// BMQ
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

// MWC
#include <mwcst_statcontext.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_memory.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   - Initialize all stat contexts and ensure they are default
//     initialized.
//
// Plan:
//   Instantiate the component under test and verify values
//
// Testing:
//   Stat Context initialization
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("Breathing Test");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    mqbmock::Cluster               mockCluster(&bufferFactory, s_allocator_p);
    mqbmock::Domain                mockDomain(&mockCluster, s_allocator_p);

    // Create statcontexts
    const int k_HISTORY_SIZE = 2;

    bsl::shared_ptr<mwcst::StatContext> client =
        mqbstat::QueueStatsUtil::initializeStatContextClients(k_HISTORY_SIZE,
                                                              s_allocator_p);
    mwcst::StatContext* domain = mockDomain.queueStatContext();

    using namespace mqbstat;
    typedef QueueStatsClient::Stat ClientStat;
    typedef QueueStatsDomain::Stat DomainStat;

    // Create queuestat objects and assert that subcontexts are created
    QueueStatsClient queueStatsClient;
    queueStatsClient.initialize(bmqt::Uri(), client.get(), s_allocator_p);

    QueueStatsDomain queueStatsDomain(s_allocator_p);
    queueStatsDomain.initialize(bmqt::Uri(), &mockDomain);

    client->snapshot();
    domain->snapshot();

    // Check subcontexts
    ASSERT_EQ(client->numSubcontexts(), 1);
    ASSERT_EQ(domain->numSubcontexts(), 1);

#define ASSERT_EQ_TO_0_CLIENTSTAT(PARAM)                                      \
    ASSERT_EQ(0, QueueStatsClient::getValue(*client, 1, ClientStat::PARAM));  \
    ASSERT_EQ(0,                                                              \
              QueueStatsClient::getValue(*queueStatsClient.statContext(),     \
                                         1,                                   \
                                         ClientStat::PARAM));

#define ASSERT_EQ_TO_0_DOMAINSTAT(PARAM)                                      \
    ASSERT_EQ(0, QueueStatsDomain::getValue(*domain, 1, DomainStat::PARAM));  \
    ASSERT_EQ(0,                                                              \
              QueueStatsDomain::getValue(*queueStatsDomain.statContext(),     \
                                         1,                                   \
                                         DomainStat::PARAM));

    ASSERT_EQ_TO_0_CLIENTSTAT(e_PUSH_MESSAGES_DELTA);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_PUT_MESSAGES_DELTA);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_ACK_DELTA);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_CONFIRM_DELTA);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_PUSH_BYTES_DELTA);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_PUT_BYTES_DELTA);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_PUSH_MESSAGES_ABS);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_PUT_MESSAGES_ABS);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_ACK_ABS);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_CONFIRM_ABS);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_PUSH_BYTES_ABS);
    ASSERT_EQ_TO_0_CLIENTSTAT(e_PUT_BYTES_ABS);

    ASSERT_EQ_TO_0_DOMAINSTAT(e_NB_CONSUMER);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_NB_PRODUCER);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_MESSAGES_CURRENT);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_MESSAGES_MAX);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_BYTES_CURRENT);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_BYTES_MAX);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_PUT_BYTES_ABS);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_PUSH_BYTES_ABS);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_ACK_ABS);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_CONFIRM_ABS);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_PUT_MESSAGES_ABS);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_PUSH_MESSAGES_ABS);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_PUT_MESSAGES_DELTA);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_PUSH_MESSAGES_DELTA);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_PUT_BYTES_DELTA);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_PUSH_BYTES_DELTA);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_ACK_DELTA);
    ASSERT_EQ_TO_0_DOMAINSTAT(e_CONFIRM_DELTA);

#undef ASSERT_EQ_TO_0_CLIENTSTAT
#undef ASSERT_EQ_TO_0_DOMAINSTAT
}

static void test2_queueStatsClient()
// ------------------------------------------------------------------------
// QUEUESTATSCLIENT
//
// Concerns:
//   - Ensure that onEvent triggers value changes and the changed values
//     are as expected.
//
// Plan:
//   - Instantiate the component under test
//   - Trigger onEvent with data
//   - Ensure correct change in values
//
// Testing:
//   QueueStatsClient manipulation
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("QueueStatsClient");

    // Create statcontexts
    const int                           k_HISTORY_SIZE = 3;
    bsl::shared_ptr<mwcst::StatContext> client =
        mqbstat::QueueStatsUtil::initializeStatContextClients(k_HISTORY_SIZE,
                                                              s_allocator_p);

    client->snapshot();

    using namespace mqbstat;
    typedef QueueStatsClient::Stat ClientStat;

    QueueStatsClient queueStatsClient;
    queueStatsClient.initialize(bmqt::Uri(), client.get(), s_allocator_p);

    const int k_DUMMY = 0;

    // Create two snapshot values

    // *SNAPSHOT 1*
    // 1 ack: bytes irrelevant
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_ACK, k_DUMMY);

    // 2 confirms: bytes irrelevant
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_CONFIRM, k_DUMMY);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_CONFIRM, k_DUMMY);

    // 1 push: 9 bytes
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUSH, 9);

    // 2 puts: 22 bytes
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUT, 9);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUT, 13);
    client->snapshot();

    // *SNAPSHOT 2*
    // 4 acks : bytes irrelevant
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_ACK, k_DUMMY);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_ACK, k_DUMMY);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_ACK, k_DUMMY);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_ACK, k_DUMMY);

    // 1 confirm: bytes irrelevant
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_CONFIRM, k_DUMMY);

    // 2 push: 38 bytes
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUSH, 18);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUSH, 20);

    // 5 put: 35 bytes
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUT, 5);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUT, 6);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUT, 7);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUT, 8);
    queueStatsClient.onEvent(QueueStatsClient::EventType::e_PUT, 9);
    client->snapshot();

#define ASSERT_EQ_CLIENTSTAT(PARAM, SNAPSHOT, VALUE)                          \
    ASSERT_EQ(VALUE,                                                          \
              QueueStatsClient::getValue(*queueStatsClient.statContext(),     \
                                         SNAPSHOT,                            \
                                         ClientStat::PARAM));

    ASSERT_EQ_CLIENTSTAT(e_PUSH_MESSAGES_DELTA, 1, 2);
    ASSERT_EQ_CLIENTSTAT(e_PUT_MESSAGES_DELTA, 1, 5);
    ASSERT_EQ_CLIENTSTAT(e_ACK_DELTA, 1, 4);
    ASSERT_EQ_CLIENTSTAT(e_CONFIRM_DELTA, 1, 1);
    ASSERT_EQ_CLIENTSTAT(e_PUSH_BYTES_DELTA, 1, 38);
    ASSERT_EQ_CLIENTSTAT(e_PUT_BYTES_DELTA, 1, 35);
    ASSERT_EQ_CLIENTSTAT(e_PUSH_MESSAGES_ABS, 0, 3);
    ASSERT_EQ_CLIENTSTAT(e_PUT_MESSAGES_ABS, 0, 7);
    ASSERT_EQ_CLIENTSTAT(e_ACK_ABS, 0, 5);
    ASSERT_EQ_CLIENTSTAT(e_CONFIRM_ABS, 0, 3);
    ASSERT_EQ_CLIENTSTAT(e_PUSH_BYTES_ABS, 0, 47);
    ASSERT_EQ_CLIENTSTAT(e_PUT_BYTES_ABS, 0, 57);

#undef ASSERT_EQ_CLIENTSTAT
}

static void test3_queueStatsDomain()
// ------------------------------------------------------------------------
// QUEUESTATSDOMAIN
//
// Concerns:
//   - Ensure that onEvent triggers value changes and the changed values
//     are as expected.
//
// Plan:
//   - Instantiate the component under test
//   - Trigger onEvent with data
//   - Ensure correct change in values
//
// Testing:
//   QueueStatsDomain manipulation
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("QueueStatsDomain");

    // Create statcontext
    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    mqbmock::Cluster               mockCluster(&bufferFactory, s_allocator_p);
    mqbmock::Domain                mockDomain(&mockCluster, s_allocator_p);

    mwcst::StatContext* domain = mockDomain.queueStatContext();

    domain->snapshot();

    using namespace mqbstat;
    typedef QueueStatsDomain::Stat DomainStat;

    QueueStatsDomain queueStatsDomain(s_allocator_p);
    queueStatsDomain.initialize(bmqt::Uri(), &mockDomain);

    const int k_DUMMY = 0;

#define ASSERT_EQ_DOMAINSTAT(PARAM, SNAPSHOT, VALUE)                          \
    ASSERT_EQ(VALUE,                                                          \
              QueueStatsDomain::getValue(*queueStatsDomain.statContext(),     \
                                         SNAPSHOT,                            \
                                         DomainStat::PARAM));

    // Create two snapshot values

    // *SNAPSHOT 1*
    // add 2 producers
    bmqt::QueueFlags::Enum producer;
    bmqt::QueueFlags::fromAscii(&producer, "WRITE");
    queueStatsDomain.setWriterCount(2);

    // 2 acks : bytes irrelevant
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_ACK, k_DUMMY);
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_ACK, k_DUMMY);

    // 1 confirm : bytes irrelevant
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_CONFIRM, k_DUMMY);

    // 1 push : 9 bytes
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_PUSH, 9);

    // 3 puts : 33 bytes
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_PUT, 10);
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_PUT, 11);
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_PUT, 12);

    // 1 add message : 15 bytes
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_ADD_MESSAGE, 15);
    domain->snapshot();

    // The following stats are not range based, and therefore always return the
    // most recent value regardless of the supplied snapshotId
    ASSERT_EQ_DOMAINSTAT(e_NB_PRODUCER, 0, 2);
    ASSERT_EQ_DOMAINSTAT(e_NB_CONSUMER, 0, 0);
    ASSERT_EQ_DOMAINSTAT(e_MESSAGES_CURRENT, 0, 1);
    ASSERT_EQ_DOMAINSTAT(e_BYTES_CURRENT, 0, 15);
    ASSERT_EQ_DOMAINSTAT(e_ACK_ABS, 0, 2);
    ASSERT_EQ_DOMAINSTAT(e_CONFIRM_ABS, 0, 1);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_MESSAGES_ABS, 0, 1);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_BYTES_ABS, 0, 9);
    ASSERT_EQ_DOMAINSTAT(e_PUT_MESSAGES_ABS, 0, 3);
    ASSERT_EQ_DOMAINSTAT(e_PUT_BYTES_ABS, 0, 33);

    ASSERT_EQ_DOMAINSTAT(e_ACK_DELTA, 1, 2);
    ASSERT_EQ_DOMAINSTAT(e_CONFIRM_DELTA, 1, 1);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_MESSAGES_DELTA, 1, 1);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_BYTES_DELTA, 1, 9);
    ASSERT_EQ_DOMAINSTAT(e_PUT_MESSAGES_DELTA, 1, 3);
    ASSERT_EQ_DOMAINSTAT(e_PUT_BYTES_DELTA, 1, 33);

    // *SNAPSHOT 2*
    // add 3 consumers, close 1 producer
    bmqt::QueueFlags::Enum consumer;
    bmqt::QueueFlags::fromAscii(&consumer, "READ");
    queueStatsDomain.setReaderCount(3).setWriterCount(1);

    // 4 acks : bytes irrelevant
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_ACK, k_DUMMY);
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_ACK, k_DUMMY);
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_ACK, k_DUMMY);
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_ACK, k_DUMMY);

    // 3 confirms : bytes irrelevant
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_CONFIRM, k_DUMMY);
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_CONFIRM, k_DUMMY);
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_CONFIRM, k_DUMMY);

    // 1 push : 9 bytes
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_PUSH, 11);

    // 2 puts : 22 bytes
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_PUT, 10);
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_PUT, 12);

    // del 1 message
    queueStatsDomain.onEvent(QueueStatsDomain::EventType::e_DEL_MESSAGE, 15);
    domain->snapshot();

    // The following stats are not range based, and therefore always return the
    // most recent value regardless of the supplied snapshotId
    ASSERT_EQ_DOMAINSTAT(e_NB_PRODUCER, 0, 1);
    ASSERT_EQ_DOMAINSTAT(e_NB_CONSUMER, 0, 3);
    ASSERT_EQ_DOMAINSTAT(e_MESSAGES_CURRENT, 0, 0);
    ASSERT_EQ_DOMAINSTAT(e_BYTES_CURRENT, 0, 0);
    ASSERT_EQ_DOMAINSTAT(e_ACK_ABS, 0, 6);
    ASSERT_EQ_DOMAINSTAT(e_CONFIRM_ABS, 0, 4);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_MESSAGES_ABS, 0, 2);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_BYTES_ABS, 0, 20);
    ASSERT_EQ_DOMAINSTAT(e_PUT_MESSAGES_ABS, 0, 5);
    ASSERT_EQ_DOMAINSTAT(e_PUT_BYTES_ABS, 0, 55);

    // Compare now and previous snapshot
    ASSERT_EQ_DOMAINSTAT(e_ACK_DELTA, 1, 4);
    ASSERT_EQ_DOMAINSTAT(e_CONFIRM_DELTA, 1, 3);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_MESSAGES_DELTA, 1, 1);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_BYTES_DELTA, 1, 11);
    ASSERT_EQ_DOMAINSTAT(e_PUT_MESSAGES_DELTA, 1, 2);
    ASSERT_EQ_DOMAINSTAT(e_PUT_BYTES_DELTA, 1, 22);

    // Compare now and two-snapshots ago; since two-snapshots ago was the start
    // time, the delta and abs stat should be the same
    ASSERT_EQ_DOMAINSTAT(e_ACK_DELTA, 2, 6);
    ASSERT_EQ_DOMAINSTAT(e_CONFIRM_DELTA, 2, 4);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_MESSAGES_DELTA, 2, 2);
    ASSERT_EQ_DOMAINSTAT(e_PUSH_BYTES_DELTA, 2, 20);
    ASSERT_EQ_DOMAINSTAT(e_PUT_MESSAGES_DELTA, 2, 5);
    ASSERT_EQ_DOMAINSTAT(e_PUT_BYTES_DELTA, 2, 55);

#undef ASSERT_EQ_DOMAINSTAT
}

static void test4_queueStatsDomainContent()
// ------------------------------------------------------------------------
// QUEUESTATSDOMAINCONTENT
//
// Concerns:
//   - Ensure that onEvent triggers value changes and the changed values
//     are as expected for the queue content
//
// Plan:
//   - Instantiate the component under test
//   - Trigger onEvent with data
//   - Ensure correct change in values
//
// Testing:
//   QueueStatsDomain manipulation
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("QueueStatsDomainContent");

#define ASSERT_EQ_DOMAINSTAT(PARAM, SNAPSHOT, VALUE)                          \
    ASSERT_EQ(VALUE,                                                          \
              mqbstat::QueueStatsDomain::getValue(                            \
                  *obj.statContext(),                                         \
                  SNAPSHOT,                                                   \
                  mqbstat::QueueStatsDomain::Stat::PARAM));

    // Create the necessary objects to test
    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    mqbmock::Cluster               mockCluster(&bufferFactory, s_allocator_p);
    mqbmock::Domain                mockDomain(&mockCluster, s_allocator_p);
    mwcst::StatContext*            sc = mockDomain.queueStatContext();

    mqbstat::QueueStatsDomain obj(s_allocator_p);
    obj.initialize(bmqt::Uri(), &mockDomain);

    // Initial Snapshot
    {
        sc->snapshot();

        // Verify post-snapshot stats
        ASSERT_EQ_DOMAINSTAT(e_MESSAGES_CURRENT, 0, 0);
        ASSERT_EQ_DOMAINSTAT(e_MESSAGES_MAX, 0, 0);
        ASSERT_EQ_DOMAINSTAT(e_BYTES_CURRENT, 0, 0);
        ASSERT_EQ_DOMAINSTAT(e_BYTES_MAX, 0, 0);
    }

    {
        obj.onEvent(mqbstat::QueueStatsDomain::EventType::e_ADD_MESSAGE, 3);
        obj.onEvent(mqbstat::QueueStatsDomain::EventType::e_ADD_MESSAGE, 5);

        sc->snapshot();

        // Verify post-snapshot stats
        ASSERT_EQ_DOMAINSTAT(e_MESSAGES_CURRENT, 0, 2);
        ASSERT_EQ_DOMAINSTAT(e_MESSAGES_MAX, 0, 2);
        ASSERT_EQ_DOMAINSTAT(e_BYTES_CURRENT, 0, 8);
        ASSERT_EQ_DOMAINSTAT(e_BYTES_MAX, 0, 8);
    }

    {
        obj.onEvent(mqbstat::QueueStatsDomain::EventType::e_ADD_MESSAGE, 7);
        obj.onEvent(mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE, 3);

        sc->snapshot();

        // Verify post-snapshot stats
        ASSERT_EQ_DOMAINSTAT(e_MESSAGES_CURRENT, 0, 2);
        ASSERT_EQ_DOMAINSTAT(e_MESSAGES_MAX, 0, 3);
        ASSERT_EQ_DOMAINSTAT(e_BYTES_CURRENT, 0, 12);
        ASSERT_EQ_DOMAINSTAT(e_BYTES_MAX, 0, 15);
    }

    {
        obj.onEvent(mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE, 5);

        sc->snapshot();

        // Verify post-snapshot stats
        ASSERT_EQ_DOMAINSTAT(e_MESSAGES_CURRENT, 0, 1);
        ASSERT_EQ_DOMAINSTAT(e_MESSAGES_MAX, 0, 2);
        ASSERT_EQ_DOMAINSTAT(e_BYTES_CURRENT, 0, 7);
        ASSERT_EQ_DOMAINSTAT(e_BYTES_MAX, 0, 12);
    }

    // Verify 'historical' accross snapshot stats

    // [0-1] snapshots
    ASSERT_EQ_DOMAINSTAT(e_MESSAGES_MAX, 1, 3);
    ASSERT_EQ_DOMAINSTAT(e_BYTES_MAX, 1, 15);

    // [0-2] snapshots
    ASSERT_EQ_DOMAINSTAT(e_MESSAGES_MAX, 2, 3);
    ASSERT_EQ_DOMAINSTAT(e_BYTES_MAX, 2, 15);

    // [0-3] snapshots
    ASSERT_EQ_DOMAINSTAT(e_MESSAGES_MAX, 3, 3);
    ASSERT_EQ_DOMAINSTAT(e_BYTES_MAX, 3, 15);

#undef ASSERT_EQ_DOMAINSTAT
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(s_allocator_p);

    {
        mqbcfg::AppConfig brokerConfig(s_allocator_p);
        mqbcfg::BrokerConfig::set(brokerConfig);

        bsl::shared_ptr<mwcst::StatContext> statContext =
            mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);
        switch (_testCase) {
        case 0:
        case 4: test4_queueStatsDomainContent(); break;
        case 3: test3_queueStatsDomain(); break;
        case 2: test2_queueStatsClient(); break;
        case 1: test1_breathingTest(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            s_testStatus = -1;
        } break;
        }
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_DEFAULT);
    // Do not check fro default/global allocator usage.
}
