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

// bmqa_messageeventbuilder.t.cpp                                     -*-C++-*-
#include <bmqa_messageeventbuilder.h>

// BMQ
#include <bmqa_mocksession.h>
#include <bmqt_queueflags.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't ensure no default memory is allocated because a default
    // QueueId is instantiated and that uses the default allocator to
    // allocate memory for an automatically generated CorrelationId.

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqa::MessageEventBuilder obj;
}

static void test2_testMessageEventSizeCount()
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't ensure no default memory is allocated because a default
    // QueueId is instantiated and that uses the default allocator to
    // allocate memory for an automatically generated CorrelationId.

    bmqtst::TestHelper::printTestName("MESSAGE EVENT SIZE AND COUNT TEST");

    // Stage 1: preparation
    // Start a session and open a queue
    bmqa::MockSession session(
        bmqt::SessionOptions(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    {
        // Start session
        BMQA_EXPECT_CALL(session, start()).returning(0);
        const int rc = session.start();
        BMQTST_ASSERT_EQ(rc, 0);
    }

    bmqt::Uri uri(bmqtst::TestHelperUtil::allocator());

    {
        // Parse uri
        bsl::string error(bmqtst::TestHelperUtil::allocator());
        bsl::string input("bmq://my.domain/queue",
                          bmqtst::TestHelperUtil::allocator());
        const int   rc = bmqt::UriParser::parse(&uri, &error, input);
        BMQTST_ASSERT_EQ(rc, 0);
    }

    bmqt::CorrelationId queueCId = bmqt::CorrelationId::autoValue();
    bmqa::QueueId       queueId(queueCId, bmqtst::TestHelperUtil::allocator());

    {
        // Open queue
        BMQA_EXPECT_CALL(session,
                         openQueue(&queueId, uri, bmqt::QueueFlags::e_WRITE))
            .returning(0);
        const int rc = session.openQueue(&queueId,
                                         uri,
                                         bmqt::QueueFlags::e_WRITE);
        BMQTST_ASSERT_EQ(rc, 0);
    }

    // Stage 2: populate MessageEventBuilder
    bmqa::MessageEventBuilder builder;
    session.loadMessageEventBuilder(&builder);

    // Empty MessageEvent should contain at least its header
    BMQTST_ASSERT(builder.messageEventSize() > 0);
    BMQTST_ASSERT_EQ(0, builder.messageCount());

    const bsl::string payload("test payload",
                              bmqtst::TestHelperUtil::allocator());

    // Pack some messages
    for (int i = 1; i <= 5; i++) {
        const int messageEventSizeBefore = builder.messageEventSize();
        const int messageCountBefore     = builder.messageCount();

        bmqa::Message& msg = builder.startMessage();
        msg.setCorrelationId(bmqt::CorrelationId::autoValue());
        msg.setDataRef(payload.c_str(), payload.size());

        // Make sure that 'messageEventSize' and 'messageCount' remain the same
        // before packing the message
        BMQTST_ASSERT_EQ(messageEventSizeBefore, builder.messageEventSize());
        BMQTST_ASSERT_EQ(messageCountBefore, builder.messageCount());

        builder.packMessage(queueId);

        // Make sure that 'messageEventSize' and 'messageCount' increase
        // after packing the message
        BMQTST_ASSERT_LT(messageEventSizeBefore, builder.messageEventSize());
        BMQTST_ASSERT_LT(messageCountBefore, builder.messageCount());
        BMQTST_ASSERT_EQ(i, builder.messageCount());
    }

    // Stage 3: start a new message but do not pack
    const int messageEventSizeFinal = builder.messageEventSize();
    const int messageCountFinal     = builder.messageCount();

    {
        bmqa::Message& msg = builder.startMessage();
        msg.setCorrelationId(bmqt::CorrelationId::autoValue());
        msg.setDataRef(payload.c_str(), payload.size());
        // Avoid holding reference to the 'msg' for too long
    }

    // Make sure that 'messageEventSize' and 'messageCount' remain the same
    // since we do not pack the last started message
    BMQTST_ASSERT_EQ(messageEventSizeFinal, builder.messageEventSize());
    BMQTST_ASSERT_EQ(messageCountFinal, builder.messageCount());

    // Stage 4: build MessageEvent
    // MessageEventBuilder switches from WRITE mode to READ:
    {
        const bmqa::MessageEvent& event = builder.messageEvent();
        // Avoid holding reference to the 'event' for too long
    }

    // We had non-packed Message before, make sure it was not added to the blob
    BMQTST_ASSERT_EQ(messageEventSizeFinal, builder.messageEventSize());
    BMQTST_ASSERT_EQ(messageCountFinal, builder.messageCount());

    // Stage 5: reset MessageEventBuilder
    // MessageEventBuilder switches from READ mode to WRITE:
    builder.reset();

    // Since we resetted the MessageEventBuilder, the currently built message
    // event is smaller than the populated one from the previous steps
    BMQTST_ASSERT_LT(builder.messageEventSize(), messageEventSizeFinal);
    BMQTST_ASSERT_EQ(0, builder.messageCount());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    case 2: test2_testMessageEventSizeCount(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
