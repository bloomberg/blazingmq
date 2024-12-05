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

// bmqa_abstractsession.t.cpp                                         -*-C++-*-
#include <bmqa_abstractsession.h>

// BDE
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// Disable some compiler warning for simplified write of the
// 'AbstractSessionTestImp'.
#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wweak-vtables"
// Disabling 'weak-vtables' so that we can define all interface methods
// inline, without the following warning:
//..
//  bmqa_abstractsession.t.cpp:20:1: error: 'AbstractSessionTestImp' has no
//  out-of-line virtual method definitions; its vtable will be emitted in
//  every translation unit [-Werror,-Wweak-vtables]
//..
#endif  // BSLS_PLATFORM_CMP_CLANG

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------

/// A test implementation of the `bmqa::AbstractSession` protocol
struct AbstractSessionTestImp : bsls::ProtocolTestImp<bmqa::AbstractSession> {
    int start(const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int startAsync(const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    void stop() BSLS_KEYWORD_OVERRIDE { markDone(); }

    void stopAsync() BSLS_KEYWORD_OVERRIDE { markDone(); }

    void finalizeStop() BSLS_KEYWORD_OVERRIDE { markDone(); }

    void loadMessageEventBuilder(bmqa::MessageEventBuilder* builder)
        BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    void loadConfirmEventBuilder(bmqa::ConfirmEventBuilder* builder)
        BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    void loadMessageProperties(bmqa::MessageProperties* buffer)
        BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    int getQueueId(bmqa::QueueId*   queueId,
                   const bmqt::Uri& uri) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int
    getQueueId(bmqa::QueueId*             queueId,
               const bmqt::CorrelationId& correlationId) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int openQueue(bmqa::QueueId*            queueId,
                  const bmqt::Uri&          uri,
                  bsls::Types::Uint64       flags,
                  const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                  const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bmqa::OpenQueueStatus
    openQueueSync(bmqa::QueueId*            queueId,
                  const bmqt::Uri&          uri,
                  bsls::Types::Uint64       flags,
                  const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                  const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int
    openQueueAsync(bmqa::QueueId*            queueId,
                   const bmqt::Uri&          uri,
                   bsls::Types::Uint64       flags,
                   const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    void
    openQueueAsync(bmqa::QueueId*            queueId,
                   const bmqt::Uri&          uri,
                   bsls::Types::Uint64       flags,
                   const OpenQueueCallback&  callback,
                   const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    int configureQueue(bmqa::QueueId*            queueId,
                       const bmqt::QueueOptions& options,
                       const bsls::TimeInterval& timeout =
                           bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bmqa::ConfigureQueueStatus
    configureQueueSync(bmqa::QueueId*            queueId,
                       const bmqt::QueueOptions& options,
                       const bsls::TimeInterval& timeout) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int configureQueueAsync(bmqa::QueueId*            queueId,
                            const bmqt::QueueOptions& options,
                            const bsls::TimeInterval& timeout =
                                bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    void configureQueueAsync(bmqa::QueueId*                queueId,
                             const bmqt::QueueOptions&     options,
                             const ConfigureQueueCallback& callback,
                             const bsls::TimeInterval&     timeout =
                                 bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    int closeQueue(bmqa::QueueId*            queueId,
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bmqa::CloseQueueStatus
    closeQueueSync(bmqa::QueueId*            queueId,
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int closeQueueAsync(bmqa::QueueId*            queueId,
                        const bsls::TimeInterval& timeout =
                            bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    void closeQueueAsync(bmqa::QueueId*            queueId,
                         const CloseQueueCallback& callback,
                         const bsls::TimeInterval& timeout =
                             bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    bmqa::Event nextEvent(const bsls::TimeInterval& timeout =
                              bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int post(const bmqa::MessageEvent& event) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int confirmMessage(const bmqa::Message& message) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int confirmMessage(const bmqa::MessageConfirmationCookie& cookie)
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int
    confirmMessages(bmqa::ConfirmEventBuilder* builder) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int configureMessageDumping(const bslstl::StringRef& command)
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }
};

/// A dummy implementation of the `bmqa::AbstractSession` protocol
/// overriding only one method.
struct AbstractSessionDummyImp : bsls::ProtocolTestImp<bmqa::AbstractSession> {
    int configureMessageDumping(const bslstl::StringRef& command)
        BSLS_KEYWORD_OVERRIDE
    {
        return -1497;
    }
};

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// PROTOCOL TEST:
//   Ensure this class is a NOT a properly defined protocol.
//
// Concerns:
//: 1 The protocol is NOT abstract: objects of it can be created.
//:
//: 2 The protocol has no data members.
//:
//: 3 The protocol has a virtual destructor.
//:
//: 4 All methods of the protocol are publicly accessible.
//
// Plan:
//: 1 Define a concrete derived implementation, 'AbstractSessionTestImp',
//:   of the protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'AbstractSessionTestImp', and use it to verify
//:   that:
//:
//:   1 The protocol is NOT abstract. (C-1)
//:
//:   2 The protocol has no data members. (C-2)
//:
//:   3 The protocol has a virtual destructor. (C-3)
//:
//: 3 Use the 'BSLS_PROTOCOLTEST_ASSERT' macro to verify that
//:   non-creator methods of the protocol are:
//:
//:   1 virtual, (C-4)
//:
//:   2 publicly accessible. (C-5)
//
// Testing:
//   PROTOCOL TEST
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // The default allocator check fails in this test case because the
    // 'markDone' methods of AbstractSession may sometimes return a
    // memory-aware object without utilizing the parameter allocator.

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    PV("Creating a concrete object");
    bmqa::AbstractSession concreteObj;

    PV("Creating a test object");
    bsls::ProtocolTest<AbstractSessionTestImp> testObj(
        bmqtst::TestHelperUtil::verbosityLevel() > 2);

    PV("Verify that the protocol is NOT abstract");
    ASSERT(!testObj.testAbstract());

    PV("Verify that there are no data members");
    ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    ASSERT(testObj.testVirtualDestructor());

    PV("Verify that methods are public and virtual");

    bmqa::ConfirmEventBuilder*      dummyConfirmEventBuilderPtr = 0;
    bmqa::Message                   dummyMessage;
    bmqa::MessageConfirmationCookie dummyMessageConfirmationCookie;
    bmqa::MessageEvent              dummyMessageEvent;
    bmqa::MessageEventBuilder*      dummyMessageEventBuilderPtr = 0;
    bmqa::MessageProperties*        dummyMessagePropertiesPtr   = 0;
    bmqa::QueueId*                  dummyQueueIdPtr             = 0;
    bmqt::CorrelationId             dummyCorrelationId;
    bmqt::QueueOptions              dummyQueueOptions;
    bmqt::Uri                       dummyUri;
    bsls::TimeInterval              dummyTimeInterval(0);

    bmqa::OpenQueueStatus openQueueResult(bmqtst::TestHelperUtil::allocator());
    bmqa::ConfigureQueueStatus configureQueueResult(
        bmqtst::TestHelperUtil::allocator());
    bmqa::CloseQueueStatus closeQueueResult(
        bmqtst::TestHelperUtil::allocator());

    const bmqa::AbstractSession::OpenQueueCallback      openQueueCallback;
    const bmqa::AbstractSession::ConfigureQueueCallback configureQueueCallback;
    const bmqa::AbstractSession::CloseQueueCallback     closeQueueCallback;

    BSLS_PROTOCOLTEST_ASSERT(testObj, start(dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj, startAsync(dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj, stop());
    BSLS_PROTOCOLTEST_ASSERT(testObj, stopAsync());
    BSLS_PROTOCOLTEST_ASSERT(testObj, finalizeStop());
    BSLS_PROTOCOLTEST_ASSERT(
        testObj,
        loadMessageEventBuilder(dummyMessageEventBuilderPtr));
    BSLS_PROTOCOLTEST_ASSERT(
        testObj,
        loadConfirmEventBuilder(dummyConfirmEventBuilderPtr));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             loadMessageProperties(dummyMessagePropertiesPtr));
    BSLS_PROTOCOLTEST_ASSERT(testObj, getQueueId(dummyQueueIdPtr, dummyUri));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             getQueueId(dummyQueueIdPtr, dummyCorrelationId));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             openQueue(dummyQueueIdPtr,
                                       dummyUri,
                                       0,  // flags
                                       dummyQueueOptions,
                                       dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             openQueueSync(dummyQueueIdPtr,
                                           dummyUri,
                                           0,  // flags
                                           dummyQueueOptions,
                                           dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             openQueueAsync(dummyQueueIdPtr,
                                            dummyUri,
                                            0,  // flags
                                            dummyQueueOptions,
                                            dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             openQueueAsync(dummyQueueIdPtr,
                                            dummyUri,
                                            0,  // flags
                                            openQueueCallback,
                                            dummyQueueOptions,
                                            dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(
        testObj,
        configureQueue(dummyQueueIdPtr, dummyQueueOptions, dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             configureQueueSync(dummyQueueIdPtr,
                                                dummyQueueOptions,
                                                dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             configureQueueAsync(dummyQueueIdPtr,
                                                 dummyQueueOptions,
                                                 dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             configureQueueAsync(dummyQueueIdPtr,
                                                 dummyQueueOptions,
                                                 configureQueueCallback,
                                                 dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             closeQueue(dummyQueueIdPtr, dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             closeQueueSync(dummyQueueIdPtr,
                                            dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             closeQueueAsync(dummyQueueIdPtr,
                                             dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             closeQueueAsync(dummyQueueIdPtr,
                                             closeQueueCallback,
                                             dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj, nextEvent(dummyTimeInterval));
    BSLS_PROTOCOLTEST_ASSERT(testObj, post(dummyMessageEvent));
    BSLS_PROTOCOLTEST_ASSERT(testObj, confirmMessage(dummyMessage));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             confirmMessage(dummyMessageConfirmationCookie));
    BSLS_PROTOCOLTEST_ASSERT(testObj,
                             confirmMessages(dummyConfirmEventBuilderPtr));
    BSLS_PROTOCOLTEST_ASSERT(testObj, configureMessageDumping(""));
}

static void test2_instanceInvariants()
// ------------------------------------------------------------------------
// INSTANCE INVARIANTS:
//   Ensure that 'AbstractSession' and deriving classes can yield an
//   instance, and that any method of that instance that has not been
//   overriden fires an assert upon attempted use.
//
// Concerns:
//: 1 The protocol is NOT abstract: objects of it can be created.
//:
//: 2 Instances of the class are dysfunctional: methods that have not been
//:   overriden fire an assert upon attempted usage.
//:
// Plan:
//: 1 Define a concrete derived implementation, 'AbstractSessionDummyImp',
//:   of the protocol.
//:
//: 2 Create an object of the 'AbstractSessionDummyImp' and use it to
//:   verify that:
//:
//:   1 Methods that have not been overriden fire an assert.
//:
//:   2 Methods that have been overriden do not fire an assert.
//:
// Testing:
//   INSTANCE INVARIANTS
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't ensure no default memory is allocated because a default
    // QueueId is instantiated and that uses the default allocator to
    // allocate memory for an automatically generated CorrelationId.

    PV("Creating a concrete object");
    bmqa::AbstractSession concreteObj;

    PV("Creating a test object");
    AbstractSessionDummyImp testObj;

    PV("Verify that non-overriden methods fire an assert");

    bmqa::ConfirmEventBuilder*      dummyConfirmEventBuilderPtr = 0;
    bmqa::Message                   dummyMessage;
    bmqa::MessageConfirmationCookie dummyMessageConfirmationCookie;
    bmqa::MessageEvent              dummyMessageEvent;
    bmqa::MessageEventBuilder*      dummyMessageEventBuilderPtr = 0;
    bmqa::MessageProperties*        dummyMessagePropertiesPtr   = 0;
    bmqa::QueueId*                  dummyQueueIdPtr             = 0;
    bmqt::CorrelationId             dummyCorrelationId;
    bmqt::QueueOptions              dummyQueueOptions;
    bmqt::Uri dummyUri(bmqtst::TestHelperUtil::allocator());
    bsls::TimeInterval              dummyTimeInterval(0);

    bmqa::OpenQueueStatus openQueueResult(bmqtst::TestHelperUtil::allocator());
    bmqa::ConfigureQueueStatus configureQueueResult(
        bmqtst::TestHelperUtil::allocator());
    bmqa::CloseQueueStatus closeQueueResult(
        bmqtst::TestHelperUtil::allocator());

    const bmqa::AbstractSession::OpenQueueCallback      openQueueCallback;
    const bmqa::AbstractSession::ConfigureQueueCallback configureQueueCallback;
    const bmqa::AbstractSession::CloseQueueCallback     closeQueueCallback;

    // Base class instance
    ASSERT_OPT_FAIL(concreteObj.start(dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.startAsync(dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.stop());
    ASSERT_OPT_FAIL(concreteObj.stopAsync());
    ASSERT_OPT_FAIL(concreteObj.finalizeStop());
    ASSERT_OPT_FAIL(
        concreteObj.loadMessageEventBuilder(dummyMessageEventBuilderPtr));
    ASSERT_OPT_FAIL(
        concreteObj.loadConfirmEventBuilder(dummyConfirmEventBuilderPtr));
    ASSERT_OPT_FAIL(
        concreteObj.loadMessageProperties(dummyMessagePropertiesPtr));
    ASSERT_OPT_FAIL(concreteObj.getQueueId(dummyQueueIdPtr, dummyUri));
    ASSERT_OPT_FAIL(
        concreteObj.getQueueId(dummyQueueIdPtr, dummyCorrelationId));
    ASSERT_OPT_FAIL(concreteObj.openQueue(dummyQueueIdPtr,
                                          dummyUri,
                                          0,
                                          dummyQueueOptions,
                                          dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.openQueueSync(dummyQueueIdPtr,
                                              dummyUri,
                                              0,
                                              dummyQueueOptions,
                                              dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.openQueueAsync(dummyQueueIdPtr,
                                               dummyUri,
                                               0,
                                               dummyQueueOptions,
                                               dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.openQueueAsync(dummyQueueIdPtr,
                                               dummyUri,
                                               0,
                                               openQueueCallback,
                                               dummyQueueOptions,
                                               dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.configureQueue(dummyQueueIdPtr,
                                               dummyQueueOptions,
                                               dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.configureQueueSync(dummyQueueIdPtr,
                                                   dummyQueueOptions,
                                                   dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.configureQueueAsync(dummyQueueIdPtr,
                                                    dummyQueueOptions,
                                                    dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.configureQueueAsync(dummyQueueIdPtr,
                                                    dummyQueueOptions,
                                                    configureQueueCallback,
                                                    dummyTimeInterval));
    ASSERT_OPT_FAIL(
        concreteObj.closeQueue(dummyQueueIdPtr, dummyTimeInterval));
    ASSERT_OPT_FAIL(
        concreteObj.closeQueueSync(dummyQueueIdPtr, dummyTimeInterval));
    ASSERT_OPT_FAIL(
        concreteObj.closeQueueAsync(dummyQueueIdPtr, dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.closeQueueAsync(dummyQueueIdPtr,
                                                closeQueueCallback,
                                                dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.nextEvent(dummyTimeInterval));
    ASSERT_OPT_FAIL(concreteObj.post(dummyMessageEvent));
    ASSERT_OPT_FAIL(concreteObj.confirmMessage(dummyMessage));
    ASSERT_OPT_FAIL(
        concreteObj.confirmMessage(dummyMessageConfirmationCookie));
    ASSERT_OPT_FAIL(concreteObj.confirmMessages(dummyConfirmEventBuilderPtr));
    ASSERT_OPT_FAIL(concreteObj.configureMessageDumping(""));

    // Derived instance
    ASSERT_OPT_FAIL(testObj.start(dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.startAsync(dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.stop());
    ASSERT_OPT_FAIL(testObj.stopAsync());
    ASSERT_OPT_FAIL(testObj.finalizeStop());
    ASSERT_OPT_FAIL(
        testObj.loadMessageEventBuilder(dummyMessageEventBuilderPtr));
    ASSERT_OPT_FAIL(
        testObj.loadConfirmEventBuilder(dummyConfirmEventBuilderPtr));
    ASSERT_OPT_FAIL(testObj.loadMessageProperties(dummyMessagePropertiesPtr));
    ASSERT_OPT_FAIL(testObj.getQueueId(dummyQueueIdPtr, dummyUri));
    ASSERT_OPT_FAIL(testObj.getQueueId(dummyQueueIdPtr, dummyCorrelationId));
    ASSERT_OPT_FAIL(testObj.openQueue(dummyQueueIdPtr,
                                      dummyUri,
                                      0,
                                      dummyQueueOptions,
                                      dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.openQueueSync(dummyQueueIdPtr,
                                          dummyUri,
                                          0,  // flags
                                          dummyQueueOptions,
                                          dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.openQueueAsync(dummyQueueIdPtr,
                                           dummyUri,
                                           0,
                                           dummyQueueOptions,
                                           dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.openQueueAsync(dummyQueueIdPtr,
                                           dummyUri,
                                           0,
                                           openQueueCallback,
                                           dummyQueueOptions,
                                           dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.configureQueue(dummyQueueIdPtr,
                                           dummyQueueOptions,
                                           dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.configureQueueSync(dummyQueueIdPtr,
                                               dummyQueueOptions,
                                               dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.configureQueueAsync(dummyQueueIdPtr,
                                                dummyQueueOptions,
                                                dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.configureQueueAsync(dummyQueueIdPtr,
                                                dummyQueueOptions,
                                                configureQueueCallback,
                                                dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.closeQueue(dummyQueueIdPtr, dummyTimeInterval));
    ASSERT_OPT_FAIL(
        testObj.closeQueueSync(dummyQueueIdPtr, dummyTimeInterval));
    ASSERT_OPT_FAIL(
        testObj.closeQueueAsync(dummyQueueIdPtr, dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.closeQueueAsync(dummyQueueIdPtr,
                                            closeQueueCallback,
                                            dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.nextEvent(dummyTimeInterval));
    ASSERT_OPT_FAIL(testObj.post(dummyMessageEvent));
    ASSERT_OPT_FAIL(testObj.confirmMessage(dummyMessage));
    ASSERT_OPT_FAIL(testObj.confirmMessage(dummyMessageConfirmationCookie));
    ASSERT_OPT_FAIL(testObj.confirmMessages(dummyConfirmEventBuilderPtr));

    PV("Verify that overriden methods execute as intended");

    ASSERT_OPT_PASS(testObj.configureMessageDumping(""));
    ASSERT_EQ(testObj.configureMessageDumping(""), -1497);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_instanceInvariants(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
