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

// mqbblp_relayqueueengine.t.cpp                                      -*-C++-*-
#include <mqbblp_relayqueueengine.h>

// MQB
#include <mqbblp_queueenginetester.h>
#include <mqbblp_queueengineutil.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbi_queueengine.h>
#include <mqbmock_queuehandle.h>
#include <mqbstat_brokerstats.h>

#include <bmqu_memoutstream.h>

// BMQ
#include <bmqp_protocol.h>

// BDE
#include <bdlmt_eventscheduler.h>
#include <bsl_iostream.h>
#include <bslmt_semaphore.h>
#include <bsls_timeinterval.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

const mqbi::QueueHandle* k_nullHandle_p = 0;

/// Return a fanout domain.
mqbconfm::Domain fanoutConfig()
{
    mqbconfm::Domain domainConfig;
    domainConfig.mode().makeFanout();

    return domainConfig;
}

}  // close unnamed namespace

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
// Plan:
//  1) 3 consumers with the same priority
//  2) Post 3 messages to the queue, and invoke the engine to deliver them
//     to the highest priority consumers
//  3) Verify that each consumer received 1 message
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    mqbconfm::Domain domainConfig;
    domainConfig.mode().makePriority();

    mqbblp::QueueEngineTester tester(domainConfig,
                                     false,  // start scheduler
                                     s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1) 3 consumers with the same priority
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=1");

    // 2) Post 3 messages to the queue, and invoke the engine to deliver them
    //    to the highest priority consumers
    tester.post("1,2,3", guard.engine());

    tester.afterNewMessage(3);

    // 3) Verify that each consumer received 1 message
    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    PVV(L_ << ": C3 Messages: " << C3->_messages());

    // TODO: For each message, verify that it was delivered once to exactly
    //       one handle (and one handle only!)
    // ASSERT(tester.wasDeliveredOnce("a,b,c"));
    ASSERT_EQ(C1->_numMessages(), 1);
    ASSERT_EQ(C2->_numMessages(), 1);
    ASSERT_EQ(C3->_numMessages(), 1);

    // Confirm
    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0"));
}

static void test2_aggregateDownstream()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//  a) If a client has multiple highest priority consumers, then the queue
//     engine will deliver to that client one message per highest priority
//     consumer before moving on to the next client
//
// Plan:
//  1) Configure 3 handles with the same priority, C2 has 2 highest
//     priority consumers, C1 and C3 have 1 highest priority consumer.
//     Post 4 messages, deliver, and verify.
//  2) Configure C3 to have 3 highest priority consumers.  Post 6 messages,
//     deliver, and verify.
//  3) Configure C3 to have 2 highest priority consumers.  Post 5 messages,
//     deliver, and verify.
//  4) Configure C2 to have 1 highest priority consumer.  Post 4 messages,
//     deliver, and verify.
//  5) Configure C2 to have 0 highest priority consumer.  Post 6 messages,
//     deliver, and verify.
//
// Testing:
//   'afterNewMessage()' with client having multiple highest priority
//   consumers
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("AGGREGATE DOWNSTREAM");

    mqbconfm::Domain domainConfig;
    domainConfig.mode().makePriority();

    mqbblp::QueueEngineTester tester(domainConfig,
                                     false,  // start scheduler
                                     s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=1");

    // 1) C2: 2 highest priority consumers
    tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=2");

    // C1: 1
    // C2: 2
    // C3: 1
    tester.post("1,2,3,4", guard.engine());
    tester.afterNewMessage(4);

    ASSERT_EQ(C1->_numMessages(), 1);
    ASSERT_EQ(C2->_numMessages(), 2);
    ASSERT_EQ(C3->_numMessages(), 1);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0"));

    // 2) C3: 3 highest priority consumers
    tester.getHandle("C3 readCount=1");
    tester.getHandle("C3 readCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=2");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=3");

    // C1: 1
    // C2: 2
    // C3: 3
    tester.post("5,6,7,8,9,10", guard.engine());
    tester.afterNewMessage(6);

    ASSERT_EQ(C1->_numMessages(), 1);
    ASSERT_EQ(C2->_numMessages(), 2);
    ASSERT_EQ(C3->_numMessages(), 3);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1,2"));

    // 3) C3: 2 highest priority consumers
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=2");
    tester.releaseHandle("C3 readCount=1");

    // C1: 1
    // C2: 2
    // C3: 2
    tester.post("11,12,13,14,15", guard.engine());
    tester.afterNewMessage(5);

    ASSERT_EQ(C1->_numMessages(), 1);
    ASSERT_EQ(C2->_numMessages(), 2);
    ASSERT_EQ(C3->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1"));

    // 4) C2: 1 highest priority consumer
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");
    tester.releaseHandle("C2 readCount=1");

    // C1: 1
    // C2: 1
    // C3: 2
    tester.post("16,17,18,19", guard.engine());
    tester.afterNewMessage(4);

    ASSERT_EQ(C1->_numMessages(), 1);
    ASSERT_EQ(C2->_numMessages(), 1);
    ASSERT_EQ(C3->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1"));

    // 5) C2: No highest priority consumers
    tester.configureHandle("C2 consumerPriority=0 consumerPriorityCount=1");

    // C1: 1
    // C2: 0
    // C3: 2
    tester.post("20,21,22,23,24,25", guard.engine());
    tester.afterNewMessage(6);

    ASSERT_EQ(C1->_numMessages(), 2);
    ASSERT_EQ(C2->_numMessages(), 0);
    ASSERT_EQ(C3->_numMessages(), 4);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1,2,3"));
}

static void test3_reconfigure()
// ------------------------------------------------------------------------
// RECONFIGURE
//
// Concerns:
//   a) If a handle is reconfigured for a client, then the queue engine
//      correctly accounts for the reconfigure with respect to distributing
//      messages.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the same priority. Post 2
//      messages, deliver, and verify.
//   2) Reconfigure C1 to a lower priority.  Post 2 messages, deliver, and
//      verify that C2 received both messages (and C1 received none).
//   3) Reconfigure C2 to C1's priority.  Post 4 messages, deliver, and
//      verify that both C1 and C2 received 2 messages each.
//   4) Bring up a new handle, C3, and configure it with a new highest
//      priority.  Post 2 messages, deliver, and verify that C3 received
//      both messages (and C1 and C2 received none).
//   5) Reconfigure C3 to C1 and C2's priority, and reconfigure C1 to have
//      a 'consumerPriorityCount' of 2.  Post 8 messages, deliver, and
//      verify that C1 received 4 messages, and C2 and C3 received 2
//      messages each.
//
// Testing:
//   Queue Engine delivery across handle reconfigure.
//     - 'configureHandle()'
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks
    // from 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RECONFIGURE");

    mqbconfm::Domain domainConfig;
    domainConfig.mode().makePriority();

    mqbblp::QueueEngineTester tester(domainConfig,
                                     false,  // start scheduler
                                     s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    // 1)
    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("1,2", guard.engine());
    tester.afterNewMessage(2);

    ASSERT_EQ(C1->_numMessages(), 1);
    ASSERT_EQ(C2->_numMessages(), 1);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0"));

    // 2) C1: Lower priority
    tester.configureHandle("C1 consumerPriority=0 consumerPriorityCount=1");

    // C1: 0
    // C2: 1
    tester.post("3,4", guard.engine());
    tester.afterNewMessage(2);

    ASSERT_EQ(C1->_numMessages(), 0);
    ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));

    // 3) C2: Lower priority
    tester.configureHandle("C2 consumerPriority=0 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("5,6,7,8", guard.engine());
    tester.afterNewMessage(4);

    ASSERT_EQ(C1->_numMessages(), 2);
    ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0,1"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));

    // 4) C3: New highest priority
    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=1");

    // C1: 0
    // C2: 0
    // C3: 1
    tester.post("9,10", guard.engine());
    tester.afterNewMessage(2);

    ASSERT_EQ(C1->_numMessages(), 0);
    ASSERT_EQ(C2->_numMessages(), 0);
    ASSERT_EQ(C3->_numMessages(), 2);

    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1"));

    // 5) C3: Lower priority, C1: Increase priority count
    tester.configureHandle("C3 consumerPriority=0 consumerPriorityCount=1");
    tester.configureHandle("C1 consumerPriority=0 consumerPriorityCount=2");

    // C1: 2
    // C2: 1
    // C3: 1
    tester.post("11,12,13,14,15,16,17,18", guard.engine());
    tester.afterNewMessage(8);

    ASSERT_EQ(C1->_numMessages(), 4);
    ASSERT_EQ(C2->_numMessages(), 2);
    ASSERT_EQ(C3->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0,1,2,3"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1"));
}

static void test4_cannotDeliver()
// ------------------------------------------------------------------------
// CANNOT DELIVER
//
// Concerns:
//   a) If it is not possible to deliver a message to a client, then the
//      queue engine attempts to deliver messages to other clients.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the same priority. Post 2
//      messages, deliver, and verify.
//   2) Disable delivery for C1.  Post 2 messages, deliver, and verify
//      that C2 received both messages (and C2 received none).
//   3) Disable delivery for C2.  Post 1 message, deliver, and verify that
//      neither C2 nor C1 received any messages.
//   4) Enable delivery for C1.  Verify that C1 received the "pending"
//      message from step 3 above (and C2 received none).
//   5) Enable delivery for C2.  Verify that neither C1 nor C2 received any
//      messages.  Post 4 messages, deliver, and verify that both C1 and
//      C2 received 2 messages each.
//
// Testing:
//   Queue Engine delivery when it is not possible to deliver to one or
//   more highest priority consumers.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("CANNOT CONSUMERS");

    mqbconfm::Domain domainConfig;
    domainConfig.mode().makePriority();

    mqbblp::QueueEngineTester tester(domainConfig,
                                     false,  // start scheduler
                                     s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("1,2", guard.engine());
    tester.afterNewMessage(2);

    ASSERT_EQ(C1->_numMessages(), 1);
    ASSERT_EQ(C2->_numMessages(), 1);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0"));

    // 2) C1: Can't deliver
    C1->_setCanDeliver(false);

    // C1: N.A.
    // C2: 1
    tester.post("3,4", guard.engine());
    tester.afterNewMessage(2);

    ASSERT_EQ(C1->_numMessages(), 0);
    ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));

    // 3) C2: Can't deliver
    C2->_setCanDeliver(false);

    // C1: N.A.
    // C2: N.A.
    tester.post("5", guard.engine());
    tester.afterNewMessage(1);

    ASSERT_EQ(C1->_numMessages(), 0);
    ASSERT_EQ(C2->_numMessages(), 0);

    // 4) C1: Can deliver
    C1->_setCanDeliver(true);

    ASSERT_EQ(C1->_numMessages(), 1);
    ASSERT_EQ(C2->_numMessages(), 0);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));

    // 5) C2: Can deliver
    C2->_setCanDeliver(true);

    ASSERT_EQ(C1->_numMessages(), 0);
    ASSERT_EQ(C2->_numMessages(), 0);

    // C1: 1
    // C2: 1
    tester.post("6,7,8,9", guard.engine());
    tester.afterNewMessage(4);

    ASSERT_EQ(C1->_numMessages(), 2);
    ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0,1"));

    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
}
static void test5_localRedelivery()
// ------------------------------------------------------------------------
// REDELIVERY TO OTHER CONSUMERS
//
// Concerns:
//   a) If a consumer goes down without confirming messages, then the queue
//      engine attempts to deliver messages that were previously sent but
//      not confirmed to other consumers.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the same priority. Post 4
//      messages, deliver, and verify that C1 and C2 each got 2 messages.
//      C1 confirms the 1st message but not the 2nd.
//   2) Bring C1 down.  Verify that C1's 2nd message was indeed
//      "redelivered" to C2.
// Testing:
//   Queue Engine redelivery of messages that were sent but not confirmed
//   when other consumers are available and able to receive messages.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("REDELIVERY TO OTHER CONSUMERS");

    mqbconfm::Domain domainConfig;
    domainConfig.mode().makePriority();

    mqbblp::QueueEngineTester tester(domainConfig,
                                     false,  // start scheduler
                                     s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("1,2,3,4", guard.engine());
    tester.afterNewMessage(4);

    ASSERT_EQ(C1->_numMessages(), 2);
    ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());

    const bsl::string unconfirmedMessage =
        mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(), "0");

    PVV(L_ << ": unconfirmedMessage: " << unconfirmedMessage);

    // 2)
    tester.dropHandle("C1");

    PVV(L_ << ": C2 Messages: " << C2->_messages());

    ASSERT_EQ(C2->_numMessages(), 3);
    ASSERT_EQ(unconfirmedMessage,
              mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(), "2"));

    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1,2"));
}

static void test6_clearDeliveryStateWhenLostReaders()
// ------------------------------------------------------------------------
// REDELIVERY TO FIRST CONSUMER UP
//
// Concerns:
//   a) If a last consumer goes down, then the list of messages that need
//      to be delivered and the list of messages that need redelivery (i.e.
//      messages that were sent to a client who went down without
//      confirming them) are cleared (because those messages will be
//      re-routed by the primary to another client).
//
// Plan:
//   1) Configure 1 handle, C1.  Post 2 messages, deliver, and verify.
//      Confirm the 1st message but not the 2nd.
//   2) Disable delivery for C1.  Post 1 message, deliver, and verify that
//      C1 did *not* receive the message.
//   3) Bring C1 down and then back up.  Verify that C1 did *not* receive
//      any message.
//
// Testing:
//   RelayQueueEngine clearing of pending-delivery and pending-redelivery
//   message lists when it loses the last consumer.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("REDELIVERY TO FIRST CONSUMER UP");

    mqbconfm::Domain domainConfig;
    domainConfig.mode().makePriority();

    mqbblp::QueueEngineTester tester(domainConfig,
                                     false,  // start scheduler
                                     s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");

    // C1: 2
    tester.post("1,2", guard.engine());
    tester.afterNewMessage(2);

    ASSERT_EQ(C1->_numMessages(), 2);
    ASSERT_EQ(C1->_messages(), "1,2");

    tester.confirm("C1", "1");

    // 2) C1: Can't deliver
    C1->_setCanDeliver(false);

    // C1: N.A.
    tester.post("3", guard.engine());
    tester.afterNewMessage(1);

    ASSERT_EQ(C1->_numMessages(), 1);
    ASSERT_EQ(C1->_messages(), "2");

    // 3)
    tester.configureHandle("C1 consumerPriority=-2147483648"
                           " consumerPriorityCount=0");
    tester.dropHandle("C1");

    C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");

    ASSERT_EQ(C1->_numMessages(), 0);
    ASSERT_EQ(C1->_messages(), "");
}

static void test7_broadcastMode()
// ------------------------------------------------------------------------
// Broadcast mode works fine
//
// Concerns:
//   a) If we broadcast, all handlers should be getting messages.
//
// Plan:
//   1) Configure 2 handles, C1, C2.  Post 2 messages, deliver, and verify.
//      Confirm both handles got all messages.
//   2) Disable delivery for C1.  Post 1 message, deliver, and verify that
//      C1 did *not* receive the message.  C2 received it.
//   3) Re-enable delivery for C1.  It should receive new messages but
//      *not* receive the lost ones.
//   4) Drop C1.  Verify that C2 continues to receive as usually.
//
// Testing:
//   RelayQueueEngine is sending to all handlers if mode is broadcast.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BROADCAST MODE");

    mqbconfm::Domain domainConfig(s_allocator_p);
    domainConfig.mode().makeBroadcast();
    domainConfig.storage().config().makeInMemory();

    mqbblp::QueueEngineTester tester(domainConfig,
                                     false,  // start scheduler
                                     s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=2");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    // Note: For the RelayQueueEngine, one has to call 'afterNewMessage()'
    //       after each single message.  We couldn't do 'tester.post("1,2");'.
    tester.post("1", guard.engine());
    tester.afterNewMessage(1);
    tester.post("2", guard.engine());
    tester.afterNewMessage(1);

    ASSERT_EQ(C1->_messages(), "1,2");
    ASSERT_EQ(C2->_messages(), "1,2");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // 2) C1: Can't deliver
    C1->_setCanDeliver(false);

    tester.post("3", guard.engine());
    tester.afterNewMessage(1);

    ASSERT_EQ(C1->_messages(), "");
    ASSERT_EQ(C2->_messages(), "3");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // 3)  C1: Can deliver again
    C1->_setCanDeliver(true);

    tester.post("4", guard.engine());
    tester.afterNewMessage(1);

    ASSERT_EQ(C1->_messages(), "4");
    ASSERT_EQ(C2->_messages(), "4");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // 4)  C1: Dropped
    tester.dropHandle("C1");

    tester.post("5", guard.engine());
    tester.afterNewMessage(1);

    ASSERT_EQ(C2->_messages(), "5");
    C2->_resetUnconfirmed();
}

static void test8_priority_beforeMessageRemoved_garbageCollection()
// ------------------------------------------------------------------------
// BEFORE MESSAGE REMOVED - GARBAGE COLLECTION
//
// Concerns:
//   a) If the queue engine is notified that a message is about to be
//      removed from the queue (e.g. it's TTL expired, it was deleted by
//      admin task, confirmed by all recipients, etc.), then it updates its
//      position in the stream of the queue to advance past the message
//      accordingly (if needed).
//
// Plan:
//   1) Bring up a consumer C1 and post 4 messages.
//   2) Simulate queue garbage collection (e.g., TTL expiration) of the
//      first 2 messages (this involves invoking
//      'beforeMessageRemoved(msgGUID)' followed by removing the message
//      from storage -- for each message in turn).
//   3) Invoke message delivery in the engine and verify that only the last
//      2 messages were delivered to C1.
//
// Testing:
//   Queue Engine adjusting its position in the stream of the queue upon
//   receiving notification prior to a message being removed.
//   - 'beforeMessageRemoved()'
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BEFORE MESSAGE REMOVED - GARBAGE "
                                      "COLLECTION");

    mqbconfm::Domain domainConfig(s_allocator_p);
    domainConfig.mode().makePriority();

    mqbblp::QueueEngineTester tester(domainConfig,
                                     false,  // start scheduler
                                     s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1) Bring up a consumer C1 and post 4 messages.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");

    PV(L_ << ": post ['1','2','3','4']");
    tester.post("1,2,3,4", guard.engine());

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    ASSERT_EQ(C1->_numMessages(), 0);

    // 2) Simulate queue garbage collection (e.g., TTL expiration) of the first
    //    2 messages (this involves invoking 'beforeMessageRemoved(msgGUID)'
    //    followed by removing the message from storage -- for each message in
    //    turn).
    PV(L_ << ": Garbage collect (TTL expiration) ['1','2']");
    tester.garbageCollectMessages(2);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    ASSERT_EQ(C1->_numMessages(), 0);

    // 3) Invoke message delivery in the engine and verify that only the last 2
    //    messages were delivered to C1.
    PV(L_ << ": afterNewMessage ['3','4']");
    tester.afterNewMessage(2);

    PV(L_ << ": C1 Messages: " << C1->_messages());
    ASSERT_EQ(C1->_numMessages(), 2);
    ASSERT_EQ(C1->_messages(), "3,4");
}

static void test9_releaseHandle_isDeletedFlag()
// ------------------------------------------------------------------------
// RELEASE HANDLE - IS-DELETED FLAG
//
// Concerns:
//   1. Ensure that releasing a handle fully (i.e., all counts go to zero
//      OR 'isFinal=true' is explicitly specified) results in the handle
//      being deleted, as indicated by the 'isDeleted' flag propagated via
//      the 'releaseHandle' callback invoation.
//
// Plan:
//   1. Bring up consumers:
//      - C1 with writeCount=1
//      - C2 with readCount=2.
//      - C3 with readCount=2
//   2. Release from C2 one reader but pass 'isFinal=true' and verify that
//      C2 was fully deleted.
//   3. Release from C1 one writer but pass 'isFinal=false' and verify that
//      C1 was fully deleted.
//   4. Release from C3 two readers and pass 'isFinal=true' (i.e. correct
//      scenario) and verify that C3 was fully deleted.
//
// Testing:
//  void releaseHandle(
//      mqbi::QueueHandle                                *handle,
//      const bmqp_ctrlmsg::QueueHandleParameters&        handleParameters,
//      bool                                              isFinal,
//      const mqbi::QueueHandle::HandleReleasedCallback&  releasedCb)
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RELEASE HANDLE - IS-DELETED FLAG");

    mqbconfm::Domain domainConfig(s_allocator_p);
    domainConfig.mode().makePriority();

    mqbblp::QueueEngineTester tester(domainConfig, false, s_allocator_p);
    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1. Bring up a consumer C1 with writeCount=1, a consumer C2 with
    //    readCount=2, and a consumer C3 with readCount=2.
    tester.getHandle("C1 writeCount=1");
    tester.getHandle("C2 readCount=2");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=2");
    tester.getHandle("C3 readCount=2");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=2");

    bool isDeleted = false;

    // 2. Release from C2 one reader but pass 'isFinal=true' and verify that
    //    C2 was fully deleted.
    ASSERT_EQ(tester.releaseHandle("C2 readCount=1 isFinal=true", &isDeleted),
              0);
    ASSERT_EQ(isDeleted, true);

    // 3. Release from C1 one writer but pass 'isFinal=false' and verify that
    //    C1 was fully deleted.
    ASSERT_EQ(tester.releaseHandle("C1 writeCount=1 isFinal=false",
                                   &isDeleted),
              0);
    ASSERT_EQ(isDeleted, true);

    // 4. Release from C3 two readers and pass 'isFinal=true' (i.e. correct
    //    scenario) and verify that C3 was fully deleted.
    ASSERT_EQ(tester.releaseHandle("C3 readCount=2 isFinal=true", &isDeleted),
              0);
    ASSERT_EQ(isDeleted, true);
}

static void test10_configureFanoutAppIds()
// ------------------------------------------------------------------------
// CONFIGURING DIFFERENT APPIDs FOR UPSTREAM
//
// Concerns:
//   1. Ensure that relay queue engine sends upstream different AppIds
//      separately.
//
// Plan:
//   1. Bring up a producer and 3 fanout consumers.
//   2. Configure each with different maxUnconfirmedMessages.
//   3. Verify that the parameters cached in the queue state have been
//      configured with the streamParameters requested, and that these did
//      not get merged across the different appIds.
//
// Testing:
//  configureHandle(mqbi::QueueHandle*,
//                  const bmqp_ctrlmsg::QueueStreamParameters&,
//                  const mqbi::QueueHandle::HandleConfiguredCallback&);
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName(
        "CONFIGURING DIFFERENT APPIDs FOR UPSTREAM");

    mqbblp::QueueEngineTester tester(fanoutConfig(), false, s_allocator_p);
    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    //   1. Bring up a producer and 3 fanout consumers.
    mqbmock::QueueHandle* P1 = tester.getHandle("C1 writeCount=1");
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C1@b readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C1@c readCount=1");

    ASSERT_EQ(P1, C1);
    ASSERT_EQ(C1, C2);
    ASSERT_EQ(C2, C3);

    //   2. Configure each with different maxUnconfirmedMessages.
    ASSERT_EQ(tester.configureHandle(
                  "C1@a consumerPriority=1 maxUnconfirmedMessages=1"),
              0);

    ASSERT_EQ(tester.configureHandle(
                  "C1@b consumerPriority=1 maxUnconfirmedMessages=2"),
              0);
    // this test verifies that 'a' is not overwritten by 'b' at this point

    ASSERT_EQ(tester.configureHandle(
                  "C1@c consumerPriority=1 maxUnconfirmedMessages=3"),
              0);

    //   3. Verify that the parameters cached in the queue state have been
    //      configured with the streamParameters requested, and that these did
    //      not get merged across the different appIds.

    const bmqp_ctrlmsg::StreamParameters& a = C1->_streamParameters("a");
    const bmqp_ctrlmsg::StreamParameters& b = C2->_streamParameters("b");
    const bmqp_ctrlmsg::StreamParameters& c = C3->_streamParameters("c");

    PVV(L_ << ": a: " << a);
    PVV(L_ << ": b: " << b);
    PVV(L_ << ": c: " << c);

    bmqp_ctrlmsg::StreamParameters upstream1;
    tester.getUpstreamParameters(&upstream1, "a");

    bmqp_ctrlmsg::StreamParameters upstream2;
    tester.getUpstreamParameters(&upstream2, "b");

    bmqp_ctrlmsg::StreamParameters upstream3;
    tester.getUpstreamParameters(&upstream3, "c");

    PVV(L_ << ": upstream1: " << upstream1);
    PVV(L_ << ": upstream2: " << upstream2);
    PVV(L_ << ": upstream3: " << upstream3);

    ASSERT_EQ(a, upstream1);
    ASSERT_EQ(b, upstream2);
    ASSERT_EQ(c, upstream3);

    // Release the handle and verify it succeeds
    // ASSERT_EQ(tester.releaseHandle("C1@a readCount=1"), 0);
}

static void test11_roundRobinAndRedelivery()
// ------------------------------------------------------------------------
// ROUND-ROBIN AND REDELIVERY
//
// Concerns:
//   1. Verifying redelivery per fanout appId..
//
// Plan:
//  1. Bring up a consumer C1 with appId 'a', 'b', and 'c'.
//  2. Post 3 messages (2 + 1)
//  3. Verify that each consumer received messages according to
//      consumerPriorityCount and consumerPriority
//  4. Close higher priority handle and verify messages redelivered to
//      lower priority handle.
//
// Testing:
//   'getHandle' and 'configureHandle' for multiple distinct appIds.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("ROUND-ROBIN AND REDELIVERY");

    mqbconfm::Domain          config = fanoutConfig();
    bsl::vector<bsl::string>& appIDs = config.mode().fanout().appIDs();
    appIDs.push_back("a");
    appIDs.push_back("b");
    appIDs.push_back("c");

    mqbblp::QueueEngineTester                                tester(config,
                                     false,  // start scheduler
                                     s_allocator_p);
    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1. Bring up a consumer C1 with appId 'a', 'b', and 'c'.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    ASSERT_NE(C1, k_nullHandle_p);
    ASSERT_EQ(C1, tester.getHandle("C1@b readCount=2"));
    ASSERT_EQ(C1, tester.getHandle("C1@c readCount=1"));

    ASSERT_EQ(tester.configureHandle(
                  "C1@a consumerPriority=2 consumerPriorityCount=2"),
              0);
    ASSERT_EQ(tester.configureHandle(
                  "C1@b consumerPriority=2 consumerPriorityCount=1"),
              0);
    ASSERT_EQ(tester.configureHandle(
                  "C1@c consumerPriority=2 consumerPriorityCount=1"),
              0);

    mqbmock::QueueHandle* C2 = tester.getHandle("C2@a readCount=1");
    ASSERT_NE(C2, k_nullHandle_p);
    ASSERT_EQ(C2, tester.getHandle("C2@b readCount=2"));
    ASSERT_EQ(C2, tester.getHandle("C2@c readCount=1"));

    ASSERT_EQ(tester.configureHandle(
                  "C2@a consumerPriority=2 consumerPriorityCount=1"),
              0);
    ASSERT_EQ(tester.configureHandle(
                  "C2@b consumerPriority=2 consumerPriorityCount=2"),
              0);
    ASSERT_EQ(tester.configureHandle(
                  "C2@c consumerPriority=1 consumerPriorityCount=1"),
              0);

    // 2. Post 3 messages (2 + 1)
    tester.post("1,2,3", guard.engine());
    tester.afterNewMessage(3);

    // 3. Verify that each consumer received messages according to
    //      consumerPriorityCount and consumerPriority
    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));
    PVV(L_ << ": C1@c Messages: " << C1->_messages("c"));

    PVV(L_ << ": C2@a Messages: " << C2->_messages("a"));
    PVV(L_ << ": C2@b Messages: " << C2->_messages("b"));
    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    ASSERT_EQ(C1->_numMessages("a"), 2);
    ASSERT_EQ(C1->_numMessages("b"), 1);
    ASSERT_EQ(C1->_numMessages("c"), 3);

    ASSERT_EQ(C2->_numMessages("a"), 1);
    ASSERT_EQ(C2->_numMessages("b"), 2);
    ASSERT_EQ(C2->_numMessages("c"), 0);

    ASSERT_EQ(C1->_messages("c"), "1,2,3");

    // 4. Close higher priority handle and verify messages redelivered to
    //      lower priority handle

    tester.releaseHandle("C1@c readCount=1");
    ASSERT_EQ(C2->_numMessages("c"), 3);
    ASSERT_EQ(C2->_messages("c"), "1,2,3");
}

static void test12_redeliverAfterGc()
// ------------------------------------------------------------------------
// REDELIVERY AFTER GC
//
// Concerns:
//   'beforeMessageRemoved' should remove message from the redeliveryList
//   (as well as from the storage).  This case triggers
//   'beforeMessageRemoved' by calling GC.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1.
//      Disable delivery to C2. Post 4 messages, verify that C1 got 4
//      messages and C2 got none.
//   2) Bring C1 down.  Verify C2 still has no messages.  The queue now has
//      all 4 messages in it's redeliveryList
//   3) GC one (first) message.  That should remove it from the storage as
//      well as from the redelivery list.
//   4) Now enable and trigger delivery on C2.  Verify that C2 got 3
//      messages.
// Testing:
//   Queue Engine redelivery of unconfirmed messages after removing some of
//   them.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("REDELIVERY AFTER GC");

    mqbconfm::Domain domainConfig(s_allocator_p);
    domainConfig.mode().makePriority();

    mqbblp::QueueEngineTester tester(domainConfig, false, s_allocator_p);
    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1"
                           " maxUnconfirmedMessages=0");

    tester.post("1,2,3,4", guard.engine());
    tester.afterNewMessage(4);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());

    ASSERT_EQ(C1->_numMessages(), 4);
    ASSERT_EQ(C2->_numMessages(), 0);

    ASSERT_EQ(C1->_messages(), "1,2,3,4");

    // 2)
    tester.dropHandle("C1");
    ASSERT_EQ(C2->_numMessages(), 0);

    // 3)
    tester.garbageCollectMessages(1);

    // 4)
    C2->_setCanDeliver(true);

    PVV(L_ << ": C2 Messages: " << C2->_messages());

    ASSERT_EQ(C2->_numMessages(), 3);
    ASSERT_EQ(C2->_messages(), "2,3,4");

    tester.confirm("C2", "2,3,4");
}

static void test13_deconfigureWhenOpen()
// ------------------------------------------------------------------------
// DECONFIGURE IN BETWEEN OPEN AND CONFIGURE (internal-ticket D152318299)
//
// Concerns:
//  The following sequence
//  1. client1 opens the queue (this inserts empty QueueStreamParameters
//                             into the handle and into the internal cache)
//  2. client1 configures the queue (this updates the cache entry and then
//                             iterates the cache to find highest priority)
//  3. client2 opens the queue
//  4. client1 deconfigures the queue (before client2 configured the queue)
//
// Plan:
//
// Testing:
//  configureHandle(mqbi::QueueHandle*,
//                  const bmqp_ctrlmsg::QueueStreamParameters&,
//                  const mqbi::QueueHandle::HandleConfiguredCallback&);
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName(
        "DECONFIGURE IN BETWEEN OPEN AND CONFIGURE");

    mqbblp::QueueEngineTester tester(fanoutConfig(), false, s_allocator_p);
    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    //   1. Bring up 1st consumer.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    ASSERT_NE(C1, k_nullHandle_p);

    //   2. Configure 1st consumer.
    ASSERT_EQ(tester.configureHandle("C1@a consumerPriority=1"), 0);

    //   3. Bring up 2nd consumer.
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@a readCount=1");
    ASSERT_NE(C2, k_nullHandle_p);

    //   4. Deconfigure 1st consumer.
    bmqu::MemOutStream os;
    os << "C1@a consumerPriority="
       << bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID;

    ASSERT_EQ(tester.configureHandle(os.str()), 0);

    // Release the handle and verify it succeeds
    ASSERT_EQ(tester.releaseHandle("C1@a readCount=1"), 0);
    ASSERT_EQ(tester.releaseHandle("C2@a readCount=1"), 0);
}

static void test14_throttleRedeliveryPriority()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY PRIORITY
//
// Concerns:
//   After a consumer disconnects ungracefully, the unconfirmed messages
//   should be redelivered to the remaining consumer in a throttled manner.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1.
//      Post 4 messages, verify that C1 got 4 messages and C2 got none.
//   2) Bring C1 down.  Verify C2 receives only the first message in the
//      redelivery list.
//   3) Advance the system clock by the value of the throttling delay and
//      verify C2 only received one additonal message.
//   4) Verify only one message comes in each time the system clock is
//      advanced by the throttling delay.
// Testing:
//   Queue Engine throttled redelivery of unconfirmed messages after the
//   rda reaches 2.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY PRIORITY");

    mqbconfm::Domain config;
    config.mode().makePriority();
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(config, s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // Set up for the expected message delay
    bmqp::RdaInfo rdaInfo = bmqp::RdaInfo()
                                .setCounter(config.maxDeliveryAttempts() - 1)
                                .setPotentiallyPoisonous(true);
    bsls::TimeInterval expectedDelay;
    // The message throttle config used from the MockQueue is default
    // constructed so it will have the same threshold and interval values as
    // this one.
    mqbcfg::MessageThrottleConfig messageThrottleConfig;
    mqbblp::QueueEngineUtil::loadMessageDelay(rdaInfo,
                                              messageThrottleConfig,
                                              &expectedDelay);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2,3,4", guard.engine());
    tester.afterNewMessage(4);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    ASSERT_EQ(C1->_numMessages(), 4);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 0);

    // 2)
    tester.dropHandle("C1");

    // Only one message should be redelivered here since the other messages are
    // being throttled.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 1);

    // 3)
    // TODO: Right now, delays are hardcoded not in a utility. Move to a
    // utility
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 2);

    // 4)
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 3);

    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 4);
}

static void test15_throttleRedeliveryFanout()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY FANOUT
//
// Concerns:
//   After a consumer disconnects ungracefully, the unconfirmed messages
//   should be redelivered to the remaining consumer in a throttled manner
//   for a particular app id. The other app ids shouldn't be affected.
//
// Plan:
//   1) Configure 3 handles, C1, C2, C3  each with distinct app ids.
//      Configure handle C4 with the same app id as C3 but with a lower
//      priority.
//      Post 4 messages, verify that C1, C2, C3 got 4 messages and C4 got
//      none.
//   2) Bring C3 down.  Verify C4 receives only the first message in the
//      redelivery list.  Verify C1 and C2 are unaffected.
//   3) Confirm the first two messages for C1 and C2.  Verify that C4 is
//      unaffected and no more messages come in for C1 and C2.
//   4) Advance the system clock by the value of the throttling delay and
//      verify C4 only received one additonal message.
//   5) Verify only one message comes in for C4 each time the system clock
//      is advanced by the throttling delay.  Verify C1 and C2 remain
//      unaffected.
// Testing:
//   Queue Engine throttled redelivery of unconfirmed messages after the
//   rda reaches 2.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY FANOUT");

    mqbconfm::Domain          config = fanoutConfig();
    bsl::vector<bsl::string>& appIDs = config.mode().fanout().appIDs();
    appIDs.push_back("a");
    appIDs.push_back("b");
    appIDs.push_back("c");
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(config, s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // Set up for the expected message delay
    bmqp::RdaInfo rdaInfo = bmqp::RdaInfo()
                                .setCounter(config.maxDeliveryAttempts() - 1)
                                .setPotentiallyPoisonous(true);
    bsls::TimeInterval expectedDelay;
    // The message throttle config used from the MockQueue is default
    // constructed so it will have the same threshold and interval values as
    // this one.
    mqbcfg::MessageThrottleConfig messageThrottleConfig;
    mqbblp::QueueEngineUtil::loadMessageDelay(rdaInfo,
                                              messageThrottleConfig,
                                              &expectedDelay);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@b readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C3@c readCount=1");
    mqbmock::QueueHandle* C4 = tester.getHandle("C4@c readCount=1");

    tester.configureHandle("C1@a maxUnconfirmedMessages=4"
                           " consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2@b maxUnconfirmedMessages=4"
                           " consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C3@c maxUnconfirmedMessages=4"
                           " consumerPriority=2 consumerPriorityCount=1");
    tester.configureHandle("C4@c maxUnconfirmedMessages=4"
                           " consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2,3,4", guard.engine());
    tester.afterNewMessage(4);

    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    ASSERT_EQ(C1->_numMessages("a"), 4);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    ASSERT_EQ(C2->_numMessages("b"), 4);
    PVV(L_ << ": C3 Messages: " << C3->_messages("c"));
    ASSERT_EQ(C3->_numMessages("c"), 4);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    ASSERT_EQ(C4->_numMessages("c"), 0);

    // 2)
    tester.dropHandle("C3");

    // Only one message should be redelivered to C4 since the other messages
    // are being throttled.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    ASSERT_EQ(C1->_numMessages("a"), 4);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    ASSERT_EQ(C2->_numMessages("b"), 4);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    ASSERT_EQ(C4->_numMessages("c"), 1);

    // 3)
    tester.confirm("C1@a", "1,2");
    tester.confirm("C2@b", "1,2");
    // The messages being confirmed for C1 and C2 shouldn't have any effect on
    // C4. No messages should be delivered to C1 and C2.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    ASSERT_EQ(C1->_numMessages("a"), 2);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    ASSERT_EQ(C2->_numMessages("b"), 2);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    ASSERT_EQ(C4->_numMessages("c"), 1);

    // 4)
    // TODO: Right now, delays are hardcoded not in a utility. Move to a
    // utility
    tester.advanceTime(expectedDelay);
    // C4 should receive the next throttled message. C1 and C2 should be
    // unaffected.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    ASSERT_EQ(C1->_numMessages("a"), 2);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    ASSERT_EQ(C2->_numMessages("b"), 2);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    ASSERT_EQ(C4->_numMessages("c"), 2);

    // 5)
    tester.advanceTime(expectedDelay);
    // C4 should receive the next throttled message. C1 and C2 should be
    // unaffected.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    ASSERT_EQ(C1->_numMessages("a"), 2);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    ASSERT_EQ(C2->_numMessages("b"), 2);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    ASSERT_EQ(C4->_numMessages("c"), 3);

    tester.advanceTime(expectedDelay);
    // C4 should receive the next throttled message. C1 and C2 should be
    // unaffected.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    ASSERT_EQ(C1->_numMessages("a"), 2);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    ASSERT_EQ(C2->_numMessages("b"), 2);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    ASSERT_EQ(C4->_numMessages("c"), 4);
}

static void test16_throttleRedeliveryCancelledDelay()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY CANCELLED DELAY
//
// Concerns:
//   If the message preceding a delayed message is confirmed, the delay on
//   the current message should be cancelled and sent right away.  A
//   confirmed message which was sent before the delayed and preceding
//   shouldn't have an effect on the current delay.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1.
//      Post 4 messages.
//   2) Bring C1 down.  Verify C2 receives only the first message in the
//      redelivery list.
//   3) Advance the system clock by the value of the throttling delay and
//      verify C2 only received one additonal message.
//   4) Confirm only the 1st message sent and verify the 3rd message still
//      being throttled and hasn't been delivered
//   5) Advance the system clock by the value of the throttling delay and
//      verify C2 received only the 3rd message.
//   6) Confirm the 2nd and 3rd message and verify the throttle on the 4th
//      message is cancelled and has come in.
// Testing:
//   mqbblp::QueueEngine cancelThrottle on a delayed message.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY CANCELLED DELAY");

    mqbconfm::Domain config;
    config.mode().makePriority();
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(config, s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // Set up for the expected message delay
    bmqp::RdaInfo rdaInfo = bmqp::RdaInfo()
                                .setCounter(config.maxDeliveryAttempts() - 1)
                                .setPotentiallyPoisonous(true);
    bsls::TimeInterval expectedDelay;
    // The message throttle config used from the MockQueue is default
    // constructed so it will have the same threshold and interval values as
    // this one.
    mqbcfg::MessageThrottleConfig messageThrottleConfig;
    mqbblp::QueueEngineUtil::loadMessageDelay(rdaInfo,
                                              messageThrottleConfig,
                                              &expectedDelay);

    // 1)
    tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2,3,4", guard.engine());
    tester.afterNewMessage(4);

    // 2)
    tester.dropHandle("C1");

    // Only one message should be redelivered here since the other messages are
    // being throttled.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 1);

    // 3)
    // TODO: Right now, delays are hardcoded not in a utility. Move to a
    // utility
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 2);

    // 4)
    // Confirming the first message should still keep the delay between the
    // second and third message intact (ie. no new messages should come in).
    tester.confirm("C2", "1");
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 1);

    // 5)
    // Another message should come in after the delay
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 2);

    // 6)
    // Since the message before the currently delayed message was confirmed,
    // the delay of the current message should be cut short.
    tester.confirm("C2", "2");
    tester.confirm("C2", "3");
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 1);
}

static void test17_throttleRedeliveryNewHandle()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY NEW HANDLE
//
// Concerns:
//   If a message is currently being delayed and a new handle shows up, the
//   delay should be cancelled and the message sent right away.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1. Post 2
//      messages.
//   2) Bring C1 down.  Verify C2 receives only the first message in the
//      redelivery list (the second messages should be throttled).
//   3) Bring up a new handle (C1) and configure it. Verify the second
//      message is delivered to C1 without delay.
// Testing:
//   mqbblp::QueueEngine a newly configured handle should end the delay for
//   for the current message.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY NEW HANDLE");

    mqbconfm::Domain config;
    config.mode().makePriority();
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(config, s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // 1)
    tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2", guard.engine());
    tester.afterNewMessage(2);

    // 2)
    tester.dropHandle("C1");

    // Only one message should be redelivered here since the other message is
    // being throttled.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 1);

    // 3)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    ASSERT_EQ(C1->_numMessages(), 1);
}

static void test18_throttleRedeliveryNoMoreHandles()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY NO MORE HANDLES
//
// Concerns:
//   If a message is currently being delayed and the last handle
//   disappears, the delay on the message should be cancelled.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1. Post 2
//      messages.
//   2) Bring C1 down.  Verify C2 receives only the first message in the
//      redelivery list (the second messages should be throttled).
//   3) Bring C2 down. Bring up C3.  Verify C3 hasn't received any message
//      (C2 was the last comsumer so the app would then be deleted).
//   4) Advance the time by the throttling delay and verify the C3 still
//      hasn't received any message (if the throttle wasn't cancelled
//      properly, a segmentation fault should occur since our scheduled
//      throttle would be executing processRedeliveryList() on an app that
//      has been deleted).
// Testing:
//   mqbblp::QueueEngine the last handle disappering for a particular app
//   should end the delay for the current message.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY NO MORE HANDLES");

    mqbconfm::Domain config;
    config.mode().makePriority();
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(config, s_allocator_p);

    mqbblp::QueueEngineTesterGuard<mqbblp::RelayQueueEngine> guard(&tester);

    // Set up for the expected message delay
    bmqp::RdaInfo rdaInfo = bmqp::RdaInfo()
                                .setCounter(config.maxDeliveryAttempts() - 1)
                                .setPotentiallyPoisonous(true);
    bsls::TimeInterval expectedDelay;
    // The message throttle config used from the MockQueue is default
    // constructed so it will have the same threshold and interval values as
    // this one.
    mqbcfg::MessageThrottleConfig messageThrottleConfig;
    mqbblp::QueueEngineUtil::loadMessageDelay(rdaInfo,
                                              messageThrottleConfig,
                                              &expectedDelay);

    // 1)
    tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2", guard.engine());
    tester.afterNewMessage(2);

    // 2)
    tester.dropHandle("C1");

    // Only one message should be redelivered here since the other message is
    // being throttled.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 1);

    // 3)
    // We still should not have received the throttled message.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    ASSERT_EQ(C2->_numMessages(), 1);
    tester.dropHandle("C2");

    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=1");
    PVV(L_ << ": C3 Messages: " << C3->_messages());
    ASSERT_EQ(C3->_numMessages(), 0);

    // 4)
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C3 Messages: " << C3->_messages());
    ASSERT_EQ(C3->_numMessages(), 0);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    {
        bmqt::UriParser::initialize(s_allocator_p);
        bmqp::ProtocolUtil::initialize(s_allocator_p);

        mqbcfg::AppConfig brokerConfig(s_allocator_p);
        mqbcfg::BrokerConfig::set(brokerConfig);

        bsl::shared_ptr<bmqst::StatContext> statContext =
            mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

        switch (_testCase) {
        case 0:
        case 18: test18_throttleRedeliveryNoMoreHandles(); break;
        case 17: test17_throttleRedeliveryNewHandle(); break;
        case 16: test16_throttleRedeliveryCancelledDelay(); break;
        case 15: test15_throttleRedeliveryFanout(); break;
        case 14: test14_throttleRedeliveryPriority(); break;
        case 13: test13_deconfigureWhenOpen(); break;
        case 12: test12_redeliverAfterGc(); break;
        case 11: test11_roundRobinAndRedelivery(); break;
        case 10: test10_configureFanoutAppIds(); break;
        case 9: test9_releaseHandle_isDeletedFlag(); break;
        case 8: test8_priority_beforeMessageRemoved_garbageCollection(); break;
        case 7: test7_broadcastMode(); break;
        case 6: test6_clearDeliveryStateWhenLostReaders(); break;
        case 5: test5_localRedelivery(); break;
        case 4: test4_cannotDeliver(); break;
        case 3: test3_reconfigure(); break;
        case 2: test2_aggregateDownstream(); break;
        case 1: test1_breathingTest(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            s_testStatus = -1;
        } break;
        }

        bmqp::ProtocolUtil::shutdown();
        bmqt::UriParser::shutdown();
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
