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

// bmqp_queueutil.t.cpp                                             -*-C++-*-
#include <bmqp_queueutil.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueid.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bsl_string.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_createQueueIdFromHandleParameters()
// ------------------------------------------------------------------------
// createQueueIdFromHandleParameters
//
// Concerns:
//   Proper behavior of the 'createQueueIdFromHandleParameters' method.
//
// Plan:
//   Test method on handleParameters with and without subId.
//
// Testing:
//   createQueueIdFromHandleParameters
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("createQueueIdFromHandleParameters");

    struct Test {
        int          d_line;
        int          d_id;
        bool         d_hasSubId;
        unsigned int d_subId;
    } k_DATA[] = {{L_, 5, true, 16}, {L_, 15, false, 0}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqp_ctrlmsg::QueueHandleParameters handleParams(
            bmqtst::TestHelperUtil::allocator());
        handleParams.qId() = test.d_id;

        if (test.d_hasSubId) {
            handleParams.subIdInfo().makeValue().subId() = test.d_subId;
        }

        PVV(test.d_line << ": creating queueId from handleParameters "
                        << handleParams);

        bmqp::QueueId result =
            bmqp::QueueUtil::createQueueIdFromHandleParameters(handleParams);

        if (test.d_hasSubId) {
            BMQTST_ASSERT_EQ_D("line " << test.d_line,
                               result,
                               bmqp::QueueId(test.d_id, test.d_subId));
        }
        else {
            BMQTST_ASSERT_EQ_D("line " << test.d_line,
                               result,
                               bmqp::QueueId(test.d_id));
        }
    }
}

static void test2_extractSubQueueId()
// ------------------------------------------------------------------------
// extractSubQueueId
//
// Concerns:
//   Proper behavior of the 'extractSubQueueId' method.
//
// Plan:
//   Test the method on stream and handle parameters with and without
//   subId.
//
// Testing:
//   extractSubQueueId
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("extractSubQueueId");

    struct Test {
        int          d_line;
        int          d_id;
        bool         d_hasSubId;
        unsigned int d_subId;
    } k_DATA[] = {{L_, 5, true, 16}, {L_, 15, false, 0}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqp_ctrlmsg::QueueStreamParameters streamParams(
            bmqtst::TestHelperUtil::allocator());
        bmqp_ctrlmsg::QueueHandleParameters handleParams(
            bmqtst::TestHelperUtil::allocator());

        if (test.d_hasSubId) {
            streamParams.subIdInfo().makeValue().subId() = test.d_subId;
            handleParams.subIdInfo().makeValue().subId() = test.d_subId;
        }

        PVV(test.d_line << ": extracting subQueueId from streamParameters "
                        << streamParams);
        PVV(test.d_line << ": extracting subQueueId from handleParameters "
                        << handleParams);

        if (test.d_hasSubId) {
            BMQTST_ASSERT_EQ_D(
                "line " << test.d_line,
                bmqp::QueueUtil::extractSubQueueId(streamParams),
                test.d_subId);
            BMQTST_ASSERT_EQ_D(
                "line " << test.d_line,
                bmqp::QueueUtil::extractSubQueueId(handleParams),
                test.d_subId);
        }
        else {
            BMQTST_ASSERT_EQ_D(
                "line " << test.d_line,
                bmqp::QueueUtil::extractSubQueueId(streamParams),
                bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
            BMQTST_ASSERT_EQ_D(
                "line " << test.d_line,
                bmqp::QueueUtil::extractSubQueueId(handleParams),
                bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
        }
    }
}

static void test3_extractCanonicalHandleParameters()
// ------------------------------------------------------------------------
// extractCanonicalHandleParameters
//
// Concerns:
//   Proper behavior of the 'extractCanonicalHandleParameters' method.
//
// Plan:
//   Test the method on handle parameters:
//     1. With canonical URI and no subStream information
//     1. With canonical URI and subStream information
//     2. With full URI and no subStream information
//     4. With full URI and subStream information
//
// Testing:
//   extractCanonicalHandleParameters
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("extractCanonicalHandleParameters");

    // Short alias
    const int k_READ  = bmqt::QueueFlags::e_READ;
    const int k_WRITE = bmqt::QueueFlags::e_WRITE;

    struct Test {
        int                 d_line;
        bsls::Types::Uint64 d_flags;
        const char*         d_uri;
        bool                d_hasSubStreamInfo;
        const char*         d_appId;
        unsigned int        d_subId;
        unsigned int        d_id;
        int                 d_readCount;
        int                 d_writeCount;
        int                 d_adminCount;
    } k_DATA[] = {
        {L_,
         k_READ | k_WRITE,                   // flags
         "bmq://bmq.test.mmap.priority/q1",  // uri
         false,                              // d_hasSubStreamInfo
         "",                                 // appId
         0,                                  // subId
         9,                                  // id
         1,                                  // readCount
         2,                                  // writeCount
         0},                                 // adminCount
        {L_,
         k_READ | k_WRITE,                   // flags
         "bmq://bmq.test.mmap.priority/q1",  // uri
         true,                               // d_hasSubStreamInfo
         "foo",                              // appId
         1,                                  // subId
         10,                                 // id
         2,                                  // readCount
         1,                                  // writeCount
         0},                                 // adminCount
        {L_,
         k_READ | k_WRITE,                        // flags
         "bmq://bmq.test.mmap.fanout/q1?id=foo",  // uri
         false,                                   // d_hasSubStreamInfo
         "",                                      // appId
         0,                                       // subId
         11,                                      // id
         3,                                       // readCount
         3,                                       // writeCount
         0},                                      // adminCount
        {L_,
         k_READ | k_WRITE,                        // flags
         "bmq://bmq.test.mmap.fanout/q1?id=bar",  // uri
         true,                                    // d_hasSubStreamInfo
         "bar",                                   // appId
         4,                                       // subId
         12,                                      // id
         1,                                       // readCount
         1,                                       // writeCount
         0}                                       // adminCount
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqp_ctrlmsg::QueueHandleParameters handleParams(
            bmqtst::TestHelperUtil::allocator());
        handleParams.flags() = test.d_flags;
        handleParams.uri()   = bsl::string(test.d_uri,
                                         bmqtst::TestHelperUtil::allocator());
        if (test.d_hasSubStreamInfo) {
            bmqp_ctrlmsg::SubQueueIdInfo& subStreamInfo =
                handleParams.subIdInfo().makeValue();
            subStreamInfo.appId() =
                bsl::string(test.d_appId, bmqtst::TestHelperUtil::allocator());
            subStreamInfo.subId() = test.d_subId;
        }
        handleParams.readCount()  = test.d_readCount;
        handleParams.writeCount() = test.d_writeCount;
        handleParams.adminCount() = test.d_adminCount;

        PVV(test.d_line << ": extracting canonical handle parameters from "
                        << "'handleParameters': " << handleParams);

        bmqp_ctrlmsg::QueueHandleParameters result(
            bmqtst::TestHelperUtil::allocator());
        result = bmqp::QueueUtil::extractCanonicalHandleParameters(
            &result,
            handleParams);
        bmqt::Uri   expectedUri(bmqtst::TestHelperUtil::allocator());
        bsl::string errorDescription(bmqtst::TestHelperUtil::allocator());
        int         rc = bmqt::UriParser::parse(&expectedUri,
                                        &errorDescription,
                                        test.d_uri);
        BSLS_ASSERT_OPT(rc == 0);

        BMQTST_ASSERT_EQ(result.flags(), test.d_flags);
        BMQTST_ASSERT_EQ(result.uri(), expectedUri.canonical());

        BMQTST_ASSERT_EQ(result.subIdInfo().isNull(), true);
        BMQTST_ASSERT_EQ(result.readCount(), test.d_readCount);
        BMQTST_ASSERT_EQ(result.writeCount(), test.d_writeCount);
        BMQTST_ASSERT_EQ(result.adminCount(), test.d_adminCount);
    }
}

static void test4_isEmpty()
// ------------------------------------------------------------------------
// IS EMPTY
//
// Concerns:
//   Proper behavior of the 'isEmpty' method.
//
// Plan:
//   Test the method on handle parameters:
//     1. With all zero counts
//     2. With all counts <= 0
//     3. With some counts < 0 and at least one count > 0
//     4. With nonzero counts
//
// Testing:
//   isEmpty
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("isEmpty");

    struct Test {
        int  d_line;
        bool d_expected;
        int  d_readCount;
        int  d_writeCount;
        int  d_adminCount;
    } k_DATA[] = {
        {L_,
         true,
         0,   // readCount
         0,   // writeCount
         0},  // adminCount
        {L_,
         true,
         -1,   // readCount
         -1,   // writeCount
         -1},  // adminCount
        {L_,
         true,
         -1,  // readCount
         0,   // writeCount
         0},  // adminCount
        {L_,
         false,
         1,   // readCount
         -1,  // writeCount
         0},  // adminCount
        {L_,
         false,
         0,   // readCount
         1,   // writeCount
         0},  // adminCount
        {L_,
         false,
         0,   // readCount
         0,   // writeCount
         1},  // adminCount
        {L_,
         false,
         0,  // readCount
         0,  // writeCount
         1}  // adminCount
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqp_ctrlmsg::QueueHandleParameters handleParams(
            bmqtst::TestHelperUtil::allocator());
        handleParams.readCount()  = test.d_readCount;
        handleParams.writeCount() = test.d_writeCount;
        handleParams.adminCount() = test.d_adminCount;

        PVV(test.d_line << ": extracting 'isEmpty(handleParameters) == "
                        << bsl::boolalpha << test.d_expected << "',"
                        << " handleParameters: " << handleParams);

        BMQTST_ASSERT_EQ(bmqp::QueueUtil::isEmpty(handleParams),
                         test.d_expected);
    }
}

static void test5_isValidFanoutConsumerSubId()
{
    bmqtst::TestHelper::printTestName("isValidFanoutConsumerSubQueueId");

    using namespace bmqp;

    BMQTST_ASSERT(!QueueUtil::isValidFanoutConsumerSubQueueId(
        bmqp::QueueId::k_RESERVED_SUBQUEUE_ID));
    BMQTST_ASSERT(!QueueUtil::isValidFanoutConsumerSubQueueId(
        bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID));
    BMQTST_ASSERT(!QueueUtil::isValidFanoutConsumerSubQueueId(
        bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID));

    BMQTST_ASSERT(QueueUtil::isValidFanoutConsumerSubQueueId(123));
    BMQTST_ASSERT(QueueUtil::isValidFanoutConsumerSubQueueId(99999));
    BMQTST_ASSERT(QueueUtil::isValidFanoutConsumerSubQueueId(987654321));
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
    case 5: test5_isValidFanoutConsumerSubId(); break;
    case 4: test4_isEmpty(); break;
    case 3: test3_extractCanonicalHandleParameters(); break;
    case 2: test2_extractSubQueueId(); break;
    case 1: test1_createQueueIdFromHandleParameters(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
