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

// bmqt_sessionoptions.t.cpp                                          -*-C++-*-
#include <bmqt_sessionoptions.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_ios.h>

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
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // Create default sessionOptions
    bmqt::SessionOptions sessionOptions(s_allocator_p);

    // Make sure 'k_BROKER_DEFAULT_PORT' and the default brokerUri are in sync
    {
        PV("CHECKING k_BROKER_DEFAULT_PORT and brokerUri()");
        bmqu::MemOutStream ss(s_allocator_p);
        ss << "tcp://localhost:"
           << bmqt::SessionOptions::k_BROKER_DEFAULT_PORT;
        bsl::string str(ss.str().data(), ss.str().length(), s_allocator_p);
        ASSERT_EQ(str, sessionOptions.brokerUri());
    }
}

static void test2_printTest()
{
    const char* const sampleSessionOptionsLayout =
        "[ brokerUri = \"tcp://localhost:30114\" processNameOverride = \"\" "
        "numProcessingThreads = 1 "
        "blobBufferSize = 4096 channelHighWatermark = 134217728 "
        "statsDumpInterval = 300 connectTimeout = 60 disconnectTimeout = 30 "
        "openQueueTimeout = 300 configureQueueTimeout = 300 "
        "closeQueueTimeout = 300 eventQueueLowWatermark = 50 "
        "eventQueueHighWatermark = 2000 hasHostHealthMonitor = false "
        "hasDistributedTracing = false ]";
    bmqtst::TestHelper::printTestName("PRINT");
    PV("Testing print");
    bmqu::MemOutStream stream(s_allocator_p);
    // Create default sessionOptions
    bmqt::SessionOptions sessionOptions(s_allocator_p);
    stream << sessionOptions;
    ASSERT_EQ(stream.str(), sampleSessionOptionsLayout);
    stream.reset();
    PV("Bad stream test");
    stream << "NO LAYOUT";
    stream.clear(bsl::ios_base::badbit);
    stream << sessionOptions;
    ASSERT_EQ(stream.str(), "NO LAYOUT");
}

static void test3_setterGetterAndCopyTest()
{
    bmqtst::TestHelper::printTestName("SETTER GETTER");
    PVV("Setter getter test");
    // Create default sessionOptions
    bmqt::SessionOptions obj(s_allocator_p);

    PVV("Checking setter and getter for brokerUri");
    const char* const brokerUri = "tcp://localhost:30115";
    ASSERT_NE(obj.brokerUri(), brokerUri);
    obj.setBrokerUri(brokerUri);
    ASSERT_EQ(obj.brokerUri(), brokerUri);

    PVV("Checking setter and getter for numProcessingThreads");
    const int numProcessingThreads = 2;
    ASSERT_NE(obj.numProcessingThreads(), numProcessingThreads);
    obj.setNumProcessingThreads(numProcessingThreads);
    ASSERT_EQ(obj.numProcessingThreads(), numProcessingThreads);

    PVV("Checking setter and getter for blobBufferSize");
    const int blobBufferSize = 8 * 1024;
    ASSERT_NE(obj.blobBufferSize(), blobBufferSize);
    obj.setBlobBufferSize(blobBufferSize);
    ASSERT_EQ(obj.blobBufferSize(), blobBufferSize);

    PVV("Checking setter and getter for channelHighWatermark");
    const bsls::Types::Int64 channelHighWatermark = 256 * 1024 * 1024;
    ASSERT_NE(obj.channelHighWatermark(), channelHighWatermark);
    obj.setChannelHighWatermark(channelHighWatermark);
    ASSERT_EQ(obj.channelHighWatermark(), channelHighWatermark);

    PVV("Checking setter and getter for statsDumpInterval");
    const bsls::TimeInterval statsDumpInterval(6 * 60.0);
    obj.setStatsDumpInterval(statsDumpInterval);
    ASSERT_EQ(obj.statsDumpInterval(), statsDumpInterval);

    PVV("Checking setter and getter for connectTimeout");
    const bsls::TimeInterval connectTimeout(70);
    ASSERT_NE(obj.connectTimeout(), connectTimeout);
    obj.setConnectTimeout(connectTimeout);
    ASSERT_EQ(obj.connectTimeout(), connectTimeout);

    PVV("Checking setter and getter for openQueueTimeout");
    const bsls::TimeInterval openQueueTimeout(
        bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT + 1);
    ASSERT_NE(obj.openQueueTimeout(), openQueueTimeout);
    obj.setOpenQueueTimeout(openQueueTimeout);
    ASSERT_EQ(obj.openQueueTimeout(), openQueueTimeout);

    PVV("Checking setter and getter for configureQueueTimeout");
    const bsls::TimeInterval configureQueueTimeout(
        bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT + 2);
    ASSERT_NE(obj.configureQueueTimeout(), configureQueueTimeout);
    obj.setConfigureQueueTimeout(configureQueueTimeout);
    ASSERT_EQ(obj.configureQueueTimeout(), configureQueueTimeout);

    PVV("Checking setter and getter for closeQueueTimeout");
    const bsls::TimeInterval closeQueueTimeout(
        bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT + 3);
    ASSERT_NE(obj.closeQueueTimeout(), closeQueueTimeout);
    obj.setCloseQueueTimeout(closeQueueTimeout);
    ASSERT_EQ(obj.closeQueueTimeout(), closeQueueTimeout);

    PVV("Checking setter and getter for eventQueueLowWatermark, "
        "eventQueueHighWatermark");
    const int eventQueueLowWatermark  = 51;
    const int eventQueueHighWatermark = 2001;
    ASSERT_NE(obj.eventQueueLowWatermark(), eventQueueLowWatermark);
    ASSERT_NE(obj.eventQueueHighWatermark(), eventQueueHighWatermark);
    obj.configureEventQueue(eventQueueLowWatermark, eventQueueHighWatermark);
    ASSERT_EQ(obj.eventQueueLowWatermark(), eventQueueLowWatermark);
    ASSERT_EQ(obj.eventQueueHighWatermark(), eventQueueHighWatermark);

    PVV("Copy constructor test");
    bmqt::SessionOptions objCopy(obj);
    ASSERT_EQ(objCopy.brokerUri(), brokerUri);
    ASSERT_EQ(objCopy.numProcessingThreads(), numProcessingThreads);
    ASSERT_EQ(objCopy.blobBufferSize(), blobBufferSize);
    ASSERT_EQ(objCopy.channelHighWatermark(), channelHighWatermark);
    ASSERT_EQ(objCopy.statsDumpInterval(), statsDumpInterval);
    ASSERT_EQ(objCopy.connectTimeout(), connectTimeout);
    ASSERT_EQ(objCopy.openQueueTimeout(), openQueueTimeout);
    ASSERT_EQ(objCopy.configureQueueTimeout(), configureQueueTimeout);
    ASSERT_EQ(objCopy.closeQueueTimeout(), closeQueueTimeout);
    ASSERT_EQ(objCopy.eventQueueLowWatermark(), eventQueueLowWatermark);
    ASSERT_EQ(objCopy.eventQueueHighWatermark(), eventQueueHighWatermark);
}
// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_setterGetterAndCopyTest(); break;
    case 2: test2_printTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
