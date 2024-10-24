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

// mqbi_dispatcher.t.cpp                                              -*-C++-*-
#include <mqbi_dispatcher.h>

// BMQ
#include <bmqu_printutil.h>

// BDE
#include <bsl_cstdlib.h>
#include <bsl_ctime.h>
#include <bsl_functional.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsls_timeutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void testN1_dispatcherEventPeformance()
// ------------------------------------------------------------------------
// DISPATCHER EVENT PERFORMANCE
//
// Concerns:
//
// Plan:
//
// Testing:
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DISPATCHER EVENT PERFORMANCE");

    bsl::cout << "sizeof(mqbi::DispatcherEvent): "
              << sizeof(mqbi::DispatcherEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherDispatcherEvent): "
              << sizeof(mqbi::DispatcherDispatcherEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherCallbackEvent): "
              << sizeof(mqbi::DispatcherCallbackEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherControlMessageEvent): "
              << sizeof(mqbi::DispatcherControlMessageEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherConfirmEvent): "
              << sizeof(mqbi::DispatcherConfirmEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherRejectEvent): "
              << sizeof(mqbi::DispatcherRejectEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherPushEvent): "
              << sizeof(mqbi::DispatcherPushEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherPutEvent): "
              << sizeof(mqbi::DispatcherPutEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherAckEvent): "
              << sizeof(mqbi::DispatcherAckEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherClusterStateEvent): "
              << sizeof(mqbi::DispatcherClusterStateEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherStorageEvent): "
              << sizeof(mqbi::DispatcherStorageEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherRecoveryEvent): "
              << sizeof(mqbi::DispatcherRecoveryEvent) << bsl::endl;
    bsl::cout << "  sizeof(mqbi::DispatcherReceiptEvent): "
              << sizeof(mqbi::DispatcherReceiptEvent) << bsl::endl;

    const size_t k_ITERS_NUM = 100000000;

    const bmqp::PutHeader                                 header;
    const bsl::shared_ptr<bdlbb::Blob>                    blob;
    const bsl::shared_ptr<BloombergLP::bmqu::AtomicState> state;
    const bmqt::MessageGUID                               guid;
    const bmqp::MessagePropertiesInfo                     info;
    const bmqp::Protocol::SubQueueInfosArray              subQueueInfos;
    const bsl::string                                     msgGroupId;
    const bmqp::ConfirmMessage                            confirm;

    mqbi::DispatcherEvent event(s_allocator_p);

    {
        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            event.reset();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        bsl::cout << "mqbi::DispatcherEvent::reset():" << bsl::endl;
        bsl::cout << "       total: "
                  << bmqu::PrintUtil::prettyTimeInterval(end - begin) << " ("
                  << k_ITERS_NUM << " iterations)" << bsl::endl;
        bsl::cout << "    per call: "
                  << bmqu::PrintUtil::prettyTimeInterval((end - begin) /
                                                         k_ITERS_NUM)
                  << bsl::endl
                  << bsl::endl;
    }

    {
        bsl::shared_ptr<mqbi::DispatcherEvent> eventSp;
        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            eventSp.createInplace(s_allocator_p, s_allocator_p);
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        if (eventSp->type() != mqbi::DispatcherEventType::e_UNDEFINED) {
            bsl::cout << eventSp->type() << bsl::endl;
        }

        bsl::cout << "mqbi::DispatcherEvent::DispatcherEvent():" << bsl::endl;
        bsl::cout << "       total: "
                  << bmqu::PrintUtil::prettyTimeInterval(end - begin) << " ("
                  << k_ITERS_NUM << " iterations)" << bsl::endl;
        bsl::cout << "    per call: "
                  << bmqu::PrintUtil::prettyTimeInterval((end - begin) /
                                                         k_ITERS_NUM)
                  << bsl::endl
                  << bsl::endl;
    }

    {
        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            event.setSource(NULL)
                .makePutEvent()
                .setIsRelay(true)  // Relay message
                .setPutHeader(header)
                .setPartitionId(1)  // Only replica uses
                .setBlob(blob)
                .setOptions(blob)
                .setGenCount(10)
                .setState(state);
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        bsl::cout << "mqbi::DispatcherEvent::makePutEvent():" << bsl::endl;
        bsl::cout << "       total: "
                  << bmqu::PrintUtil::prettyTimeInterval(end - begin) << " ("
                  << k_ITERS_NUM << " iterations)" << bsl::endl;
        bsl::cout << "    per call: "
                  << bmqu::PrintUtil::prettyTimeInterval((end - begin) /
                                                         k_ITERS_NUM)
                  << bsl::endl
                  << bsl::endl;
    }

    {
        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            event.setSource(NULL)
                .makePushEvent()
                .setGuid(guid)
                .setQueueId(678)
                .setMessagePropertiesInfo(info)
                .setSubQueueInfos(subQueueInfos)
                .setMsgGroupId(msgGroupId)
                .setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_NONE)
                .setOutOfOrderPush(false)
                .setBlob(blob);
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        bsl::cout << "mqbi::DispatcherEvent::makePushEvent():" << bsl::endl;
        bsl::cout << "       total: "
                  << bmqu::PrintUtil::prettyTimeInterval(end - begin) << " ("
                  << k_ITERS_NUM << " iterations)" << bsl::endl;
        bsl::cout << "    per call: "
                  << bmqu::PrintUtil::prettyTimeInterval((end - begin) /
                                                         k_ITERS_NUM)
                  << bsl::endl;
    }

    {
        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            event.setSource(NULL)
                .makeConfirmEvent()
                .setConfirmMessage(confirm)
                .setPartitionId(678)
                .setIsRelay(true);  // Relay message
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        bsl::cout << "mqbi::DispatcherEvent::makeConfirmEvent():" << bsl::endl;
        bsl::cout << "       total: "
                  << bmqu::PrintUtil::prettyTimeInterval(end - begin) << " ("
                  << k_ITERS_NUM << " iterations)" << bsl::endl;
        bsl::cout << "    per call: "
                  << bmqu::PrintUtil::prettyTimeInterval((end - begin) /
                                                         k_ITERS_NUM)
                  << bsl::endl
                  << bsl::endl;
    }

    bsl::cout << sizeof(bmqp::MessagePropertiesInfo) << bsl::endl;
    bsl::cout << sizeof(bmqp::Protocol::MsgGroupId) << bsl::endl;

    bsl::cout << sizeof(bsls::ObjectBuffer<bmqp::SubQueueInfo>) << bsl::endl;
    bsl::cout << sizeof(bmqc::ArraySpan<bmqp::SubQueueInfo>) << bsl::endl;
    bsl::cout << sizeof(bsl::allocator<bmqp::SubQueueInfo>) << bsl::endl;
    bsl::cout << event.type() << bsl::endl;
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // To be called only once per process instantiation.
    bsls::TimeUtil::initialize();

    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case -1: testN1_dispatcherEventPeformance(); break;
    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
