// Copyright 2026 Bloomberg Finance L.P.
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

#include <mqbnet_channelitem.h>

// BMQ
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>
#include <bmqu_atomicstate.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_memory.h>
#include <bslma_managedptr.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

const int k_BUFFER_SIZE = 256;
const int k_QUEUE_ID    = 7;
const int k_SUBQUEUE_ID = 9;

/// Return a blob of the specified `size` bytes, using the specified
/// `bufferFactory` and `allocator`.
bsl::shared_ptr<bdlbb::Blob> makeBlob(bdlbb::BlobBufferFactory* bufferFactory,
                                      int                       size,
                                      bslma::Allocator*         allocator)
{
    bsl::shared_ptr<bdlbb::Blob> blob_sp;
    blob_sp.createInplace(allocator, bufferFactory, allocator);
    blob_sp->setLength(size);

    return blob_sp;
}

/// Return the subscriptions to use in the tests, using the specified
/// `allocator`.
bmqp::Protocol::SubQueueInfosArray
makeSubQueueInfos(bslma::Allocator* allocator)
{
    bmqp::Protocol::SubQueueInfosArray subQueueInfos(allocator);
    subQueueInfos.push_back(bmqp::SubQueueInfo(1));
    subQueueInfos.push_back(bmqp::SubQueueInfo(2));

    return subQueueInfos;
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
//   Each item reports its own type, event type and fields, and is recovered
//   from the base class by 'the'.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    bdlbb::PooledBlobBufferFactory bufferFactory(k_BUFFER_SIZE, alloc);

    const bsl::shared_ptr<bdlbb::Blob> data_sp = makeBlob(&bufferFactory,
                                                          k_BUFFER_SIZE,
                                                          alloc);
    const bmqt::MessageGUID guid = bmqp::MessageGUIDGenerator::testGUID();
    const bsl::shared_ptr<bmqu::AtomicState> state_sp;

    {
        PVV("PUT");

        bmqp::PutHeader putHeader;
        putHeader.setQueueId(k_QUEUE_ID).setMessageGUID(guid);

        bslma::ManagedPtr<mqbnet::ChannelItem> item(
            bslma::ManagedPtrUtil::allocateManaged<mqbnet::ChannelPutItem>(
                alloc,
                putHeader,
                data_sp,
                state_sp));

        BMQTST_ASSERT_EQ(item->type(), mqbnet::ChannelItemType::e_PUT);
        BMQTST_ASSERT_EQ(item->eventType(), bmqp::EventType::e_PUT);

        const mqbnet::ChannelPutItem* put =
            item->the<mqbnet::ChannelPutItem>();
        BMQTST_ASSERT_EQ(put->putHeader().queueId(), k_QUEUE_ID);
        BMQTST_ASSERT_EQ(put->data(), data_sp);
    }

    {
        PVV("PUSH with payload");

        const bmqp::Protocol::SubQueueInfosArray subQueueInfos =
            makeSubQueueInfos(alloc);

        bslma::ManagedPtr<mqbnet::ChannelItem> item(
            bslma::ManagedPtrUtil::allocateManaged<
                mqbnet::ChannelExplicitPayloadPushItem>(
                alloc,
                k_QUEUE_ID,
                guid,
                0,
                bmqt::CompressionAlgorithmType::e_NONE,
                bmqp::MessagePropertiesInfo(),
                data_sp,
                subQueueInfos,
                state_sp,
                alloc));

        BMQTST_ASSERT_EQ(item->type(),
                         mqbnet::ChannelItemType::e_EXPLICIT_PAYLOAD_PUSH);
        BMQTST_ASSERT_EQ(item->eventType(), bmqp::EventType::e_PUSH);

        const mqbnet::ChannelExplicitPayloadPushItem* push =
            item->the<mqbnet::ChannelExplicitPayloadPushItem>();
        BMQTST_ASSERT_EQ(push->queueId(), k_QUEUE_ID);
        BMQTST_ASSERT_EQ(push->msgId(), guid);
        BMQTST_ASSERT_EQ(push->data(), data_sp);
        BMQTST_ASSERT_EQ(push->subQueueInfos().size(), subQueueInfos.size());
    }

    {
        PVV("PUSH without payload");

        const bmqp::Protocol::SubQueueInfosArray subQueueInfos =
            makeSubQueueInfos(alloc);

        bslma::ManagedPtr<mqbnet::ChannelItem> item(
            bslma::ManagedPtrUtil::allocateManaged<
                mqbnet::ChannelImplicitPayloadPushItem>(
                alloc,
                k_QUEUE_ID,
                guid,
                0,
                bmqt::CompressionAlgorithmType::e_NONE,
                bmqp::MessagePropertiesInfo(),
                subQueueInfos,
                state_sp,
                alloc));

        BMQTST_ASSERT_EQ(item->type(),
                         mqbnet::ChannelItemType::e_IMPLICIT_PAYLOAD_PUSH);
        BMQTST_ASSERT_EQ(item->eventType(), bmqp::EventType::e_PUSH);

        const mqbnet::ChannelImplicitPayloadPushItem* push =
            item->the<mqbnet::ChannelImplicitPayloadPushItem>();
        BMQTST_ASSERT_EQ(push->queueId(), k_QUEUE_ID);
        BMQTST_ASSERT_EQ(push->msgId(), guid);
        BMQTST_ASSERT_EQ(push->subQueueInfos().size(), subQueueInfos.size());
    }

    {
        PVV("ACK");

        bslma::ManagedPtr<mqbnet::ChannelItem> item(
            bslma::ManagedPtrUtil::allocateManaged<mqbnet::ChannelAckItem>(
                alloc,
                1,
                2,
                guid,
                k_QUEUE_ID,
                state_sp));

        BMQTST_ASSERT_EQ(item->type(), mqbnet::ChannelItemType::e_ACK);
        BMQTST_ASSERT_EQ(item->eventType(), bmqp::EventType::e_ACK);

        const mqbnet::ChannelAckItem* ack =
            item->the<mqbnet::ChannelAckItem>();
        BMQTST_ASSERT_EQ(ack->status(), 1);
        BMQTST_ASSERT_EQ(ack->correlationId(), 2);
        BMQTST_ASSERT_EQ(ack->guid(), guid);
        BMQTST_ASSERT_EQ(ack->queueId(), k_QUEUE_ID);
    }

    {
        PVV("CONFIRM");

        bslma::ManagedPtr<mqbnet::ChannelItem> item(
            bslma::ManagedPtrUtil::allocateManaged<mqbnet::ChannelConfirmItem>(
                alloc,
                k_QUEUE_ID,
                k_SUBQUEUE_ID,
                guid,
                state_sp));

        BMQTST_ASSERT_EQ(item->type(), mqbnet::ChannelItemType::e_CONFIRM);
        BMQTST_ASSERT_EQ(item->eventType(), bmqp::EventType::e_CONFIRM);

        const mqbnet::ChannelConfirmItem* confirm =
            item->the<mqbnet::ChannelConfirmItem>();
        BMQTST_ASSERT_EQ(confirm->queueId(), k_QUEUE_ID);
        BMQTST_ASSERT_EQ(confirm->subQueueId(), k_SUBQUEUE_ID);
        BMQTST_ASSERT_EQ(confirm->guid(), guid);
    }

    {
        PVV("REJECT");

        bslma::ManagedPtr<mqbnet::ChannelItem> item(
            bslma::ManagedPtrUtil::allocateManaged<mqbnet::ChannelRejectItem>(
                alloc,
                k_QUEUE_ID,
                k_SUBQUEUE_ID,
                guid,
                state_sp));

        BMQTST_ASSERT_EQ(item->type(), mqbnet::ChannelItemType::e_REJECT);
        BMQTST_ASSERT_EQ(item->eventType(), bmqp::EventType::e_REJECT);

        const mqbnet::ChannelRejectItem* reject =
            item->the<mqbnet::ChannelRejectItem>();
        BMQTST_ASSERT_EQ(reject->queueId(), k_QUEUE_ID);
        BMQTST_ASSERT_EQ(reject->subQueueId(), k_SUBQUEUE_ID);
        BMQTST_ASSERT_EQ(reject->guid(), guid);
    }

    {
        PVV("BLOB");

        bslma::ManagedPtr<mqbnet::ChannelItem> item(
            bslma::ManagedPtrUtil::allocateManaged<mqbnet::ChannelBlobItem>(
                alloc,
                data_sp,
                bmqp::EventType::e_CONTROL,
                state_sp));

        BMQTST_ASSERT_EQ(item->type(), mqbnet::ChannelItemType::e_BLOB);
        BMQTST_ASSERT_EQ(item->eventType(), bmqp::EventType::e_CONTROL);

        const mqbnet::ChannelBlobItem* blob =
            item->the<mqbnet::ChannelBlobItem>();
        BMQTST_ASSERT_EQ(blob->data(), data_sp);
    }

    {
        PVV("WAKE UP");

        bslma::ManagedPtr<mqbnet::ChannelItem> item(
            bslma::ManagedPtrUtil::allocateManaged<mqbnet::ChannelWakeUpItem>(
                alloc));

        BMQTST_ASSERT_EQ(item->type(), mqbnet::ChannelItemType::e_WAKE_UP);
        BMQTST_ASSERT_EQ(item->eventType(), bmqp::EventType::e_UNDEFINED);
        BMQTST_ASSERT(!item->state());
    }
}

static void test2_state()
// ------------------------------------------------------------------------
// STATE
//
// Concerns:
//   An item hands back the state it was created with, and canceling that
//   state is visible through the item.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("STATE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    bsl::shared_ptr<bmqu::AtomicState> state_sp;
    state_sp.createInplace(alloc);

    bslma::ManagedPtr<mqbnet::ChannelItem> item(
        bslma::ManagedPtrUtil::allocateManaged<mqbnet::ChannelConfirmItem>(
            alloc,
            k_QUEUE_ID,
            k_SUBQUEUE_ID,
            bmqp::MessageGUIDGenerator::testGUID(),
            state_sp));

    BMQTST_ASSERT_EQ(item->state(), state_sp);

    BMQTST_ASSERT(state_sp->cancel());
    BMQTST_ASSERT(!item->state()->process());
}

// TEMPORARY: measurement scaffolding, to be deleted before merging.
//
// 'FlatItem' mirrors a single struct holding the union of every event type's
// fields, so that 'test3_itemSizes' can compare it against the per-type items
// in one run.
struct FlatItem {
    bmqp::EventType::Enum                d_type;
    bmqp::PutHeader                      d_putHeader;
    bsl::shared_ptr<bdlbb::Blob>         d_data_sp;
    int                                  d_queueId;
    int                                  d_subQueueId;
    bmqt::MessageGUID                    d_msgId;
    int                                  d_flags;
    bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;
    bmqp::MessagePropertiesInfo          d_messagePropertiesInfo;
    bmqp::Protocol::SubQueueInfosArray   d_subQueueInfos;
    int                                  d_correlationId;
    int                                  d_status;
    bsl::shared_ptr<bmqu::AtomicState>   d_state;
    size_t                               d_numBytes;
};

// TEMPORARY: to be deleted before merging.
static void test3_itemSizes()
{
#define PRINT_SIZE(X) cout << "  " << #X << ": " << sizeof(X) << '\n'

    cout << "flat:\n";
    PRINT_SIZE(FlatItem);

    cout << "per type:\n";
    PRINT_SIZE(mqbnet::ChannelItem);
    PRINT_SIZE(mqbnet::ChannelPutItem);
    PRINT_SIZE(mqbnet::ChannelExplicitPayloadPushItem);
    PRINT_SIZE(mqbnet::ChannelImplicitPayloadPushItem);
    PRINT_SIZE(mqbnet::ChannelAckItem);
    PRINT_SIZE(mqbnet::ChannelConfirmItem);
    PRINT_SIZE(mqbnet::ChannelRejectItem);
    PRINT_SIZE(mqbnet::ChannelBlobItem);
    PRINT_SIZE(mqbnet::ChannelWakeUpItem);

    cout << "members:\n";
    PRINT_SIZE(bmqp::PutHeader);
    PRINT_SIZE(bmqt::MessageGUID);
    PRINT_SIZE(bmqp::MessagePropertiesInfo);
    PRINT_SIZE(bmqp::Protocol::SubQueueInfosArray);
    PRINT_SIZE(bsl::shared_ptr<bdlbb::Blob>);

#undef PRINT_SIZE
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
    case 2: test2_state(); break;
    case 3: test3_itemSizes(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
