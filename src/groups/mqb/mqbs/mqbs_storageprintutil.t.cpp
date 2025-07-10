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

// mqbs_storageprintutil.t.cpp                                        -*-C++-*-
#include <mqbs_storageprintutil.h>

// MQB
#include <mqbcmd_messages.h>
#include <mqbi_queueengine.h>
#include <mqbs_inmemorystorage.h>
#include <mqbu_capacitymeter.h>
#include <mqbu_messageguidutil.h>
#include <mqbu_storagekey.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlt_datetimetz.h>
#include <bdlt_epochutil.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

// CONSTANTS
const char             k_HEX_QUEUE[] = "ABCDEF1234";
const mqbu::StorageKey k_QUEUE_KEY(mqbu::StorageKey::HexRepresentation(),
                                   k_HEX_QUEUE);
const char*            k_APP_ID1 = "ABCDEF1111";
const mqbu::StorageKey k_APP_KEY1(mqbu::StorageKey::HexRepresentation(),
                                  k_APP_ID1);
const char*            k_APP_ID2 = "ABCDEF2222";
const mqbu::StorageKey k_APP_KEY2(mqbu::StorageKey::HexRepresentation(),
                                  k_APP_ID2);

const bsls::Types::Int64 k_INT64_MAX =
    bsl::numeric_limits<bsls::Types::Int64>::max();

// STRUCTS
struct TestData {
    const bslstl::StringRef d_appId;
    bsls::Types::Int64 d_offset, d_count, d_expectedCount, d_expectedIndex[10];
};

// FUNCTIONS

/// Verify that the specified `output` matches the expected field values of
/// a message having the specified `guid`.
void verifyMessageConstruction(const mqbcmd::Message&   output,
                               const bmqt::MessageGUID& guid,
                               const mqbi::Storage*     storage)
{
    mqbcmd::Message    expected;
    bmqu::MemOutStream guidStr;
    int                msgSize = -1;

    guidStr << guid;
    storage->getMessageSize(&msgSize, guid);

    expected.guid()             = guidStr.str();
    expected.arrivalTimestamp() = bdlt::DatetimeTz(bdlt::EpochUtil::epoch(),
                                                   0);
    expected.sizeBytes()        = msgSize;

    BMQTST_ASSERT_EQ(expected, output);
}

/// Verify that the specified `output` matches the expected output based on
/// the specified `test` data, given the specified list of `guids`.
void verifyOutput(const mqbcmd::QueueContents&          queueContents,
                  const TestData&                       test,
                  const bsl::vector<bmqt::MessageGUID>& guids,
                  mqbi::Storage*                        storage)
{
    unsigned int index;
    for (index = 0; index < test.d_expectedCount; ++index) {
        int                      expectedIndex = test.d_expectedIndex[index];
        const bmqt::MessageGUID& guid          = guids[expectedIndex];
        verifyMessageConstruction(queueContents.messages()[index],
                                  guid,
                                  storage);
    }

    BMQTST_ASSERT_EQ(index, queueContents.messages().size());
}

// CLASSES
// =============
// struct Tester
// =============

struct Tester {
  private:
    // DATA
    bslma::Allocator*                        d_allocator_p;
    bdlbb::PooledBlobBufferFactory           d_bufferFactory;
    bsl::vector<bmqt::MessageGUID>           d_guids;
    mqbu::CapacityMeter                      d_capacityMeter;
    bslma::ManagedPtr<mqbs::InMemoryStorage> d_storage_mp;

  public:
    // CREATORS
    Tester()
    : d_allocator_p(bmqtst::TestHelperUtil::allocator())
    , d_bufferFactory(1024, d_allocator_p)
    , d_guids(d_allocator_p)
    , d_capacityMeter(bsl::string("test", d_allocator_p), 0, d_allocator_p)
    {
        d_capacityMeter.setLimits(k_INT64_MAX, k_INT64_MAX);

        mqbconfm::Domain domainCfg;
        domainCfg.deduplicationTimeMs() = 0;  // No history
        domainCfg.messageTtl()          = k_INT64_MAX;

        const bsl::string uri("my.domain/myqueue", d_allocator_p);
        d_storage_mp.load(new (*d_allocator_p) mqbs::InMemoryStorage(
                              bmqt::Uri(uri, d_allocator_p),
                              k_QUEUE_KEY,
                              0,
                              domainCfg,
                              &d_capacityMeter,
                              d_allocator_p),
                          d_allocator_p);

        bmqu::MemOutStream errorDescription(d_allocator_p);
        d_storage_mp->addVirtualStorage(errorDescription,
                                        k_APP_ID1,
                                        k_APP_KEY1);
        d_storage_mp->addVirtualStorage(errorDescription,
                                        k_APP_ID2,
                                        k_APP_KEY2);

        mqbconfm::Storage config;
        config.makeInMemory();

        mqbconfm::Limits limits;
        limits.messages()               = k_INT64_MAX;
        limits.messagesWatermarkRatio() = 0.8;
        limits.bytes()                  = k_INT64_MAX;
        limits.bytesWatermarkRatio()    = 0.8;

        d_storage_mp->configure(errorDescription,
                                config,
                                limits,
                                domainCfg.messageTtl(),
                                domainCfg.maxDeliveryAttempts());
    }

    // MANIPULATORS
    void populateMessages()
    {
        for (int i = 0; i < 10; ++i) {
            d_guids.emplace_back();
            bmqt::MessageGUID& guid = d_guids.back();
            mqbu::MessageGUIDUtil::generateGUID(&guid);

            bsl::shared_ptr<bdlbb::Blob> appDataPtr =
                bsl::allocate_shared<bdlbb::Blob>(d_allocator_p,
                                                  &d_bufferFactory);
            appDataPtr->setLength(i * 10);

            mqbi::StorageMessageAttributes attributes;
            attributes.setAppDataLen(appDataPtr->length());
            d_storage_mp->put(&attributes, guid, appDataPtr, appDataPtr);
        }

        for (int i = 0; i < 5; ++i) {
            d_storage_mp->confirm(d_guids[i * 2], k_APP_KEY1, 0);
            d_storage_mp->confirm(d_guids[i * 2 + 1], k_APP_KEY2, 0);
        }
    }

    // ACCESSORS
    mqbs::InMemoryStorage* storage() const { return d_storage_mp.get(); }

    const bsl::vector<bmqt::MessageGUID>& guids() const { return d_guids; }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_listMessage()
// ------------------------------------------------------------------------
// test1_listMessage
//
// Concerns:
//   Ensure proper behavior of 'listMessage' method.
//
// Plan:
//   Test various inputs.
//
// Testing:
//   listMessage(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LIST MESSAGE");

    Tester tester;
    tester.populateMessages();

    const mqbu::StorageKey K_APP_KEYS[3]  = {mqbu::StorageKey::k_NULL_KEY,
                                             k_APP_KEY1,
                                             k_APP_KEY2};
    const int              K_NUM_APP_KEYS = 3;

    for (int idx = 0; idx < K_NUM_APP_KEYS; ++idx) {
        mqbu::StorageKey appKey = K_APP_KEYS[idx];

        for (bslma::ManagedPtr<mqbi::StorageIterator> it =
                 tester.storage()->getIterator(appKey);
             !it->atEnd();
             it->advance()) {
            mqbcmd::Message message;
            int             rc = mqbs::StoragePrintUtil::listMessage(&message,
                                                         tester.storage(),
                                                         *it);
            BSLS_ASSERT_OPT(rc == 0);
            verifyMessageConstruction(message, it->guid(), tester.storage());
        }
    }
}

static void test2_listMessages()
// ------------------------------------------------------------------------
// test2_listMessages
//
// Concerns:
//   Ensure proper behavior of 'listMessages' method.
//
// Plan:
//   Test various inputs.
//
// Testing:
//   listMessages(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LIST MESSAGES");

    Tester tester;
    tester.populateMessages();

    struct TestData k_DATA[] = {
        //     appId   offs count #  expected
        {"", 0, 10, 10, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
        {"", 0, 0, 10, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
        {"", 0, 3, 3, {0, 1, 2}},
        {"", -3, 2, 2, {7, 8}},
        {"", -3, -2, 2, {5, 6}},
        {"", 8, 10, 2, {8, 9}},
        {"", 0, 100, 10, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
        {"", -20, 2, 2, {0, 1}},
        {k_APP_ID1, 0, 0, 5, {1, 3, 5, 7, 9}},
        {k_APP_ID2, 0, 0, 5, {0, 2, 4, 6, 8}},
    };

    const int k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (int idx = 0; idx < k_NUM_DATA; ++idx) {
        const TestData& test = k_DATA[idx];

        mqbcmd::QueueContents queueContents(
            bmqtst::TestHelperUtil::allocator());
        mqbs::StoragePrintUtil::listMessages(&queueContents,
                                             test.d_appId,
                                             test.d_offset,
                                             test.d_count,
                                             tester.storage());
        verifyOutput(queueContents, test, tester.guids(), tester.storage());
    }
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
    case 2: test2_listMessages(); break;
    case 1: test1_listMessage(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
