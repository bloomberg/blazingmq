// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqp_event.t.cpp                                                   -*-C++-*-
#include <bmqp_event.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_ios.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::Blob blob(s_allocator_p);

    // "Un-cloned" event
    bmqp::Event event1(&blob, s_allocator_p);
    ASSERT_EQ(event1.isValid(), false);
    ASSERT_EQ(event1.isCloned(), false);
    ASSERT_EQ(event1.blob(), &blob);

    // Cloned event
    bmqp::Event event2(&blob, s_allocator_p, true /* clone == true */);
    ASSERT_EQ(event2.isValid(), false);
    ASSERT_EQ(event2.isCloned(), true);
    ASSERT_EQ(&blob == event2.blob(), false);

    // Initialize from "un-cloned" event
    bmqp::Event event3(event1, s_allocator_p);
    ASSERT_EQ(event3.isValid(), false);
    ASSERT_EQ(event3.isCloned(), false);
    ASSERT_EQ(event3.blob(), &blob);

    // Initialize from cloned event
    bmqp::Event event4(event2, s_allocator_p);
    ASSERT_EQ(event4.isValid(), false);
    ASSERT_EQ(event4.isCloned(), true);
    ASSERT_EQ(&blob == event4.blob(), false);

    // Create an "un-cloned" event and assign an "un-cloned" event
    bmqp::Event event5(event1, s_allocator_p);
    event5 = event3;
    ASSERT_EQ(event5.isValid(), false);
    ASSERT_EQ(event5.isCloned(), false);
    ASSERT_EQ(event5.blob(), &blob);

    // Assign a cloned event to an "un-cloned" event
    event5 = event2;
    ASSERT_EQ(event5.isValid(), false);
    ASSERT_EQ(event5.isCloned(), true);
    ASSERT_EQ(&blob == event5.blob(), false);

    // Default ctor
    bmqp::Event event6(s_allocator_p);
    ASSERT_EQ(event6.isValid(), false);
    ASSERT_EQ(event6.isCloned(), false);
    ASSERT_EQ(event6.blob(), static_cast<bdlbb::Blob*>(0));

    // Clone
    bmqp::Event event7 = event3.clone(s_allocator_p);
    ASSERT_EQ(event7.isValid(), false);
    ASSERT_EQ(event7.isCloned(), true);
    ASSERT_EQ(event7.blob() == &blob, false);

    // Reset
    event6.reset(&blob);
    ASSERT_EQ(event6.isValid(), false);
    ASSERT_EQ(event6.isCloned(), false);
    ASSERT_EQ(event6.blob(), &blob);

    event6.reset(&blob, true /* clone == true */);
    ASSERT_EQ(event6.isValid(), false);
    ASSERT_EQ(event6.isCloned(), true);
    ASSERT_EQ(&blob == event6.blob(), false);

    // Clear
    event1.clear();
    ASSERT_EQ(event1.isValid(), false);
    ASSERT_EQ(event1.isCloned(), false);
    ASSERT_EQ(event1.blob(), static_cast<bdlbb::Blob*>(0));

    // Clear
    event2.clear();
    ASSERT_EQ(event2.isValid(), false);
    ASSERT_EQ(event2.isCloned(), false);
    ASSERT_EQ(event1.blob(), static_cast<bdlbb::Blob*>(0));

    // Make sure events derived from event1 and event2 are not changed.
    ASSERT_EQ(event3.isValid(), false);
    ASSERT_EQ(event3.isCloned(), false);
    ASSERT_EQ(event3.blob(), &blob);

    ASSERT_EQ(event4.isValid(), false);
    ASSERT_EQ(event4.isCloned(), true);
    ASSERT_EQ(&blob == event4.blob(), false);
}

static void test2_isValid()
{
    mwctst::TestHelper::printTestName("IS VALID");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    {
        // Create blob of length 1 (something shorter than
        // EventHeader::k_MIN_HEADER_SIZE), isValid() should fail.
        bdlbb::Blob blob(&bufferFactory, s_allocator_p);
        bdlbb::BlobUtil::append(&blob, "a", 0, 1);

        bmqp::Event event(&blob, s_allocator_p, false);

        ASSERT_EQ(event.isValid(), false);
    }

    {
        // Create a blob containing an EventHeader which advertises a big
        // number for its header words (bigger than blob's length), isValid()
        // should fail.
        bmqp::EventHeader eh(bmqp::EventType::e_PUT);
        eh.setHeaderWords(100);  // specify big number fo header words
        eh.setLength(sizeof(eh));

        bdlbb::Blob blob(&bufferFactory, s_allocator_p);
        bdlbb::BlobUtil::append(&blob,
                                reinterpret_cast<const char*>(&eh),
                                sizeof(eh));

        // Ensure that blob has less data than what eventHeader advertises
        ASSERT_LT(blob.length(),
                  eh.headerWords() * bmqp::Protocol::k_WORD_SIZE);

        // isValid() should fail
        bmqp::Event event(&blob, s_allocator_p, false);
        ASSERT_EQ(false, event.isValid());
    }
}

static void test3_eventTypes()
// --------------------------------------------------------------------
// EVENT TYPES
//
// Concerns:
//   bmqp::Event type accessors.
//
// Plan:
//   For every type of bmqp::Event do the following:
//     1. Create a valid bmqp::Event object.
//     2. Verify that type checks are passed for each type except
//        bmqp::EventType::e_UNDEFINED for which assert should be
//        fired on every check.
//
// Testing:
//   bmqp::Event::isValid()
//   bmqp::Event::type()
//   bmqp::Event::isControlEvent()
//   bmqp::Event::isPutEvent()
//   bmqp::Event::isConfirmEvent()
//   bmqp::Event::isPushEvent()
//   bmqp::Event::isAckEvent()
//   bmqp::Event::isClusterStateEvent()
//   bmqp::Event::isElectorEvent()
//   bmqp::Event::isStorageEvent()
//   bmqp::Event::isRecoveryEvent()
//   bmqp::Event::isPartitionSyncEvent()
//   bmqp::Event::isHeartbeatReqEvent()
//   bmqp::Event::isHeartbeatRspEvent()
// --------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("EVENT TYPES");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    struct Test {
        int                   d_line;
        bmqp::EventType::Enum d_type;
        bool                  d_isValid;
        bool                  d_isControlEvent;
        bool                  d_isPutEvent;
        bool                  d_isConfirmEvent;
        bool                  d_isPushEvent;
        bool                  d_isAckEvent;
        bool                  d_isClusterStateEvent;
        bool                  d_isElectorEvent;
        bool                  d_isStorageEvent;
        bool                  d_isRecoveryEvent;
        bool                  d_isPartitionSyncEvent;
        bool                  d_isHeartbeatReqEvent;
        bool                  d_isHeartbeatRspEvent;
    } k_DATA[] = {{L_,
                   bmqp::EventType::e_UNDEFINED,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_CONTROL,
                   true,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_PUT,
                   true,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_CONFIRM,
                   true,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_PUSH,
                   true,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_ACK,
                   true,
                   false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_CLUSTER_STATE,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_ELECTOR,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_STORAGE,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_RECOVERY,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_PARTITION_SYNC,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false},
                  {L_,
                   bmqp::EventType::e_HEARTBEAT_REQ,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   true,
                   false},
                  {L_,
                   bmqp::EventType::e_HEARTBEAT_RSP,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   true},
                  {L_,
                   bmqp::EventType::e_REPLICATION_RECEIPT,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&       test = k_DATA[idx];
        bmqp::EventHeader eh(test.d_type);

        PVVV(test.d_line << ": Testing: bmqp::Event [" << test.d_type << "]");

        bdlbb::Blob blob(&bufferFactory, s_allocator_p);
        bdlbb::BlobUtil::append(&blob,
                                reinterpret_cast<const char*>(&eh),
                                sizeof(eh));

        bmqp::Event event(&blob, s_allocator_p, false);

        ASSERT_EQ(event.isValid(), test.d_isValid);

        if (bmqp::EventType::e_UNDEFINED == test.d_type) {
            ASSERT_SAFE_FAIL(event.type());
            ASSERT_SAFE_FAIL(event.isControlEvent());
            ASSERT_SAFE_FAIL(event.isPutEvent());
            ASSERT_SAFE_FAIL(event.isConfirmEvent());
            ASSERT_SAFE_FAIL(event.isPushEvent());
            ASSERT_SAFE_FAIL(event.isAckEvent());
            ASSERT_SAFE_FAIL(event.isClusterStateEvent());
            ASSERT_SAFE_FAIL(event.isElectorEvent());
            ASSERT_SAFE_FAIL(event.isStorageEvent());
            ASSERT_SAFE_FAIL(event.isRecoveryEvent());
            ASSERT_SAFE_FAIL(event.isPartitionSyncEvent());
            ASSERT_SAFE_FAIL(event.isHeartbeatReqEvent());
            ASSERT_SAFE_FAIL(event.isHeartbeatRspEvent());
            continue;
        }
        ASSERT_EQ(event.type(), test.d_type);
        ASSERT_EQ(event.isControlEvent(), test.d_isControlEvent);
        ASSERT_EQ(event.isPutEvent(), test.d_isPutEvent);
        ASSERT_EQ(event.isConfirmEvent(), test.d_isConfirmEvent);
        ASSERT_EQ(event.isPushEvent(), test.d_isPushEvent);
        ASSERT_EQ(event.isAckEvent(), test.d_isAckEvent);
        ASSERT_EQ(event.isClusterStateEvent(), test.d_isClusterStateEvent);
        ASSERT_EQ(event.isElectorEvent(), test.d_isElectorEvent);
        ASSERT_EQ(event.isStorageEvent(), test.d_isStorageEvent);
        ASSERT_EQ(event.isRecoveryEvent(), test.d_isRecoveryEvent);
        ASSERT_EQ(event.isPartitionSyncEvent(), test.d_isPartitionSyncEvent);
        ASSERT_EQ(event.isHeartbeatReqEvent(), test.d_isHeartbeatReqEvent);
        ASSERT_EQ(event.isHeartbeatRspEvent(), test.d_isHeartbeatRspEvent);
    }
}

static void test4_eventLoading()
// --------------------------------------------------------------------
// EVENT LOADING
//
// Concerns:
//   bmqp::Event control and elector message loading.
//
// Plan:
//     For each encoding E in 'bmqp::EncodingType::Enum', do the following:
//     1. Create a valid bmqp::Event of type 'bmqp::EventType::e_CONTROL'
//        that contains a valid 'bmqp_ctrlmsg::ControlMessage' using
//        encoding E
//     2. Verify that the message can be loaded from the event.
//     3. Do the same steps for 'bmqp::EventType::e_ELECTOR' and
//        'bmqp_ctrlmsg::ElectorMessage'
//     Note: For now, BER encoding is the only valid encoding for elector
//           messages, so we are only testing this.
//
// Testing:
//   template <class TYPE>
//       int bmqp::Event::loadControlEvent(TYPE *message) const
//   template <class TYPE>
//       int bmqp::Event::loadElectorEvent(TYPE *message) const
// --------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("EVENT LOADING");
    // Disable check that no memory was allocated from the default allocator
    s_ignoreCheckDefAlloc = true;
    // baljsn::Encoder constructor does not pass the allocator to the
    // formatter.

    bmqp::ProtocolUtil::initialize(s_allocator_p);

    int                            rc;
    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    struct Test {
        int                      d_line;
        bmqp::EncodingType::Enum d_encodingType;
    } k_DATA[] = {{
                      L_,
                      bmqp::EncodingType::e_BER,
                  },
                  {L_, bmqp::EncodingType::e_JSON}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];
        PVV(test.d_line << ": Testing " << test.d_encodingType << "encoding");

        bmqp::SchemaEventBuilder obj(&bufferFactory,
                                     s_allocator_p,
                                     test.d_encodingType);

        BSLS_ASSERT_OPT(obj.blob().length() == 0);
        {
            PVV(test.d_line << ": Create a control message");

            bmqp_ctrlmsg::ControlMessage ctrlMessage(s_allocator_p);
            bmqp_ctrlmsg::Status& status = ctrlMessage.choice().makeStatus();
            status.code()                = 123;
            status.message()             = "Test";

            // Encode the message
            rc = obj.setMessage(ctrlMessage, bmqp::EventType::e_CONTROL);
            BSLS_ASSERT_OPT(rc == 0);
            BSLS_ASSERT_OPT(obj.blob().length() > 0);
            BSLS_ASSERT_OPT(obj.blob().length() % 4 == 0);

            PVV(test.d_line << ": Decode and compare message");

            bmqp::Event ctrlEvent(&obj.blob(), s_allocator_p);

            ASSERT_EQ(ctrlEvent.isValid(), true);
            ASSERT_EQ(ctrlEvent.isControlEvent(), true);

            bmqp_ctrlmsg::ControlMessage decodedCtrlMsg(s_allocator_p);
            rc = ctrlEvent.loadControlEvent(&decodedCtrlMsg);

            ASSERT_EQ(rc, 0);
            ASSERT(decodedCtrlMsg.choice().isStatusValue());
            ASSERT_EQ(decodedCtrlMsg.choice().status().code(), status.code());
            ASSERT_EQ(decodedCtrlMsg.choice().status().message(),
                      status.message());
        }

        PVV(test.d_line << ": Reset");

        obj.reset();
        BSLS_ASSERT_OPT(obj.blob().length() == 0);
    }

    {
        bmqp::SchemaEventBuilder obj(&bufferFactory, s_allocator_p);
        BSLS_ASSERT_OPT(obj.blob().length() == 0);

        PVV(L_ << ": Create an elector message");

        bmqp_ctrlmsg::ElectorMessage electorMsg;
        electorMsg.choice().makeElectionProposal();

        // Encode the message
        rc = obj.setMessage(electorMsg, bmqp::EventType::e_ELECTOR);

        BSLS_ASSERT_OPT(rc == 0);
        BSLS_ASSERT_OPT(obj.blob().length() > 0);
        BSLS_ASSERT_OPT(obj.blob().length() % 4 == 0);

        PVV(L_ << ": Decode and compare message");

        bmqp::Event electorEvent(&obj.blob(), s_allocator_p);

        BSLS_ASSERT_OPT(electorEvent.isValid());
        BSLS_ASSERT_OPT(electorEvent.isElectorEvent());

        bmqp_ctrlmsg::ElectorMessage decodedElectorMsg;
        rc = electorEvent.loadElectorEvent(&decodedElectorMsg);

        ASSERT_EQ(rc, 0);
        ASSERT(decodedElectorMsg.choice().isElectionProposalValue());

        PVV(L_ << ": Reset");

        obj.reset();
        BSLS_ASSERT_OPT(obj.blob().length() == 0);
    }

    bmqp::ProtocolUtil::shutdown();
}

static void test5_iteratorLoading()
// --------------------------------------------------------------------
// ITERATOR LOADING
//
// Concerns:
//   Basic check of bmqp::Event manipulators used to load iterators.
//
// Plan:
//     For each bmqp::Event event type:
//     1. Create a valid bmqp::Event of that type.
//     2. Verify that related message iterator can be loaded from the
//        event and an attemt to load an iterator of any different type
//        fires assert.
//     3. Verify that in all the cases the iterators are in invalid state.
//
// Testing:
//   bmqp::Event::loadAckMessageIterator(
//                                      AckMessageIterator *iterator) const
//   bmqp::Event::loadConfirmMessageIterator(
//                                  ConfirmMessageIterator *iterator) const
//   bmqp::Event::loadPushMessageIterator(
//                                     PushMessageIterator *iterator) const
//   bmqp::Event::loadPutMessageIterator(
//                                      PutMessageIterator *iterator) const
//   bmqp::Event::loadStorageMessageIterator(
//                                  StorageMessageIterator *iterator) const
//   bmqp::Event::loadRecoveryMessageIterator(
//                                 RecoveryMessageIterator *iterator) const
// --------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("ITERATOR LOADING");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    struct Test {
        int                   d_line;
        bmqp::EventType::Enum d_type;
    } k_DATA[] = {{L_, bmqp::EventType::e_UNDEFINED},
                  {L_, bmqp::EventType::e_CONTROL},
                  {L_, bmqp::EventType::e_PUT},
                  {L_, bmqp::EventType::e_CONFIRM},
                  {L_, bmqp::EventType::e_PUSH},
                  {L_, bmqp::EventType::e_ACK},
                  {L_, bmqp::EventType::e_CLUSTER_STATE},
                  {L_, bmqp::EventType::e_ELECTOR},
                  {L_, bmqp::EventType::e_STORAGE},
                  {L_, bmqp::EventType::e_RECOVERY},
                  {L_, bmqp::EventType::e_PARTITION_SYNC},
                  {L_, bmqp::EventType::e_HEARTBEAT_REQ},
                  {L_, bmqp::EventType::e_HEARTBEAT_RSP}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&       test = k_DATA[idx];
        bmqp::EventHeader eh(test.d_type);

        PVVV(test.d_line << ": Testing: bmqp::Event [" << test.d_type << "]");

        bdlbb::Blob blob(&bufferFactory, s_allocator_p);
        bdlbb::BlobUtil::append(&blob,
                                reinterpret_cast<const char*>(&eh),
                                sizeof(eh));

        bmqp::Event event(&blob, s_allocator_p, false);

        ASSERT_EQ(event.isValid(),
                  (test.d_type != bmqp::EventType::e_UNDEFINED));

        bmqp::AckMessageIterator      ackIter;
        bmqp::ConfirmMessageIterator  confirmIter;
        bmqp::PushMessageIterator     pushIter(&bufferFactory, s_allocator_p);
        bmqp::PutMessageIterator      putIter(&bufferFactory, s_allocator_p);
        bmqp::StorageMessageIterator  storageIter;
        bmqp::RecoveryMessageIterator recoveryIter;

        if (bmqp::EventType::e_ACK == test.d_type) {
            event.loadAckMessageIterator(&ackIter);
        }
        else {
            ASSERT_SAFE_FAIL(event.loadAckMessageIterator(&ackIter));
        }

        if (bmqp::EventType::e_CONFIRM == test.d_type) {
            event.loadConfirmMessageIterator(&confirmIter);
        }
        else {
            ASSERT_SAFE_FAIL(event.loadConfirmMessageIterator(&confirmIter));
        }

        if (bmqp::EventType::e_PUSH == test.d_type) {
            event.loadPushMessageIterator(&pushIter);
        }
        else {
            ASSERT_SAFE_FAIL(event.loadPushMessageIterator(&pushIter));
        }

        if (bmqp::EventType::e_PUT == test.d_type) {
            event.loadPutMessageIterator(&putIter);
        }
        else {
            ASSERT_SAFE_FAIL(event.loadPutMessageIterator(&putIter));
        }

        if (bmqp::EventType::e_STORAGE == test.d_type ||
            bmqp::EventType::e_PARTITION_SYNC == test.d_type) {
            event.loadStorageMessageIterator(&storageIter);
        }
        else {
            ASSERT_SAFE_FAIL(event.loadStorageMessageIterator(&storageIter));
        }

        if (bmqp::EventType::e_RECOVERY == test.d_type) {
            event.loadRecoveryMessageIterator(&recoveryIter);
        }
        else {
            ASSERT_SAFE_FAIL(event.loadRecoveryMessageIterator(&recoveryIter));
        }

        ASSERT_EQ(ackIter.isValid(), false);
        ASSERT_EQ(confirmIter.isValid(), false);
        ASSERT_EQ(pushIter.isValid(), false);
        ASSERT_EQ(putIter.isValid(), false);
        ASSERT_EQ(storageIter.isValid(), false);
        ASSERT_EQ(recoveryIter.isValid(), false);
    }
}

static void test6_printing()
// --------------------------------------------------------------------
// PRINT
//
// Concerns:
//   bmqp::Event print methods.
//
// Plan:
//   For every valid bmqp::EventType do the following:
//     1. Create a bmqp::Event object of the specified type.
//     2. Check than the output of the 'print' method and '<<' operator
//        matches the expected pattern.
//
// Testing:
//   bmqp::Event::print()
//
//   bsl::ostream&
//   bmqp::operator<<(bsl::ostream& stream,
//                    const bmqp::Event& rhs)
// --------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PRINT");

    BSLMF_ASSERT(bmqp::EventType::e_CONTROL ==
                 bmqp::EventType::k_LOWEST_SUPPORTED_EVENT_TYPE);

    BSLMF_ASSERT(bmqp::EventType::e_REPLICATION_RECEIPT ==
                 bmqp::EventType::k_HIGHEST_SUPPORTED_EVENT_TYPE);

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    struct Test {
        bmqp::EventType::Enum d_type;
        const char*           d_expected;
    } k_DATA[] = {
        {bmqp::EventType::e_UNDEFINED, ""},
        {bmqp::EventType::e_CONTROL, "[ type = CONTROL ]"},
        {bmqp::EventType::e_PUT, "[ type = PUT ]"},
        {bmqp::EventType::e_CONFIRM, "[ type = CONFIRM ]"},
        {bmqp::EventType::e_PUSH, "[ type = PUSH ]"},
        {bmqp::EventType::e_ACK, "[ type = ACK ]"},
        {bmqp::EventType::e_CLUSTER_STATE, "[ type = CLUSTER_STATE ]"},
        {bmqp::EventType::e_ELECTOR, "[ type = ELECTOR ]"},
        {bmqp::EventType::e_STORAGE, "[ type = STORAGE ]"},
        {bmqp::EventType::e_RECOVERY, "[ type = RECOVERY ]"},
        {bmqp::EventType::e_PARTITION_SYNC, "[ type = PARTITION_SYNC ]"},
        {bmqp::EventType::e_HEARTBEAT_REQ, "[ type = HEARTBEAT_REQ ]"},
        {bmqp::EventType::e_HEARTBEAT_RSP, "[ type = HEARTBEAT_RSP ]"},
        {bmqp::EventType::e_REJECT, "[ type = REJECT ]"},
        {bmqp::EventType::e_REPLICATION_RECEIPT,
         "[ type = REPLICATION_RECEIPT ]"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&        test = k_DATA[idx];
        mwcu::MemOutStream out(s_allocator_p);
        mwcu::MemOutStream expected(s_allocator_p);
        bmqp::EventHeader  eh(static_cast<bmqp::EventType::Enum>(test.d_type));
        bdlbb::Blob        blob(&bufferFactory, s_allocator_p);

        bdlbb::BlobUtil::append(&blob,
                                reinterpret_cast<const char*>(&eh),
                                sizeof(eh));

        bmqp::Event obj(&blob, s_allocator_p, false);

        if (test.d_type == bmqp::EventType::e_UNDEFINED) {
            ASSERT_SAFE_FAIL(obj.print(out, 0, -1));
        }
        else {
            expected << test.d_expected;

            out.setstate(bsl::ios_base::badbit);
            obj.print(out, 0, -1);

            ASSERT_EQ(out.str(), "");

            out.clear();
            obj.print(out, 0, -1);

            ASSERT_EQ(out.str(), expected.str());

            out.reset();
            out << obj;

            ASSERT_EQ(out.str(), expected.str());
        }
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 6: test6_printing(); break;
    case 5: test5_iteratorLoading(); break;
    case 4: test4_eventLoading(); break;
    case 3: test3_eventTypes(); break;
    case 2: test2_isValid(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
