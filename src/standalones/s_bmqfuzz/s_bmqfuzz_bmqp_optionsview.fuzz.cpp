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

#include <fuzzer/FuzzedDataProvider.h>

#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>
#include <bmqu_blob.h>

#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bslma_default.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace BloombergLP;

namespace {

const int k_MIN_BUF_SIZE = 1;
const int k_MAX_BUF_SIZE = 512;

}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);

    bslma::Allocator* alloc = bslma::Default::allocator();

    const int      bufferSize = provider.ConsumeIntegralInRange(k_MIN_BUF_SIZE,
                                                           k_MAX_BUF_SIZE);
    const uint32_t sizeSelector = provider.ConsumeIntegral<uint32_t>();

    bdlbb::PooledBlobBufferFactory factory(bufferSize, alloc);
    bdlbb::Blob                    blob(&factory, alloc);

    const std::vector<uint8_t> payload =
        provider.ConsumeRemainingBytes<uint8_t>();
    bdlbb::BlobUtil::append(&blob,
                            reinterpret_cast<const char*>(payload.data()),
                            static_cast<int>(payload.size()));

    const bmqu::BlobPosition optionsAreaPos(0, 0);
    const int                optionsAreaSize = static_cast<int>(
        sizeSelector % (static_cast<uint32_t>(blob.length()) + 1));

    bmqp::OptionsView view(alloc);
    const int         rc = view.reset(&blob, optionsAreaPos, optionsAreaSize);
    if (rc != 0) {
        return 0;
    }

    for (bmqp::OptionsView::const_iterator it = view.begin(); it != view.end();
         ++it) {
        (void)*it;
    }

    const bool hasSubQueueInfos = view.find(
                                      bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
                                  view.end();
    const bool hasSubQueueIdsOld =
        view.find(bmqp::OptionType::e_SUB_QUEUE_IDS_OLD) != view.end();

    if (hasSubQueueInfos || hasSubQueueIdsOld) {
        bmqp::Protocol::SubQueueInfosArray subQueueInfos(alloc);
        view.loadSubQueueInfosOption(&subQueueInfos);
    }

    if (hasSubQueueIdsOld) {
        bmqp::Protocol::SubQueueIdsArrayOld subQueueIdsOld(alloc);
        view.loadSubQueueIdsOption(&subQueueIdsOld);
    }

    if (view.find(bmqp::OptionType::e_MSG_GROUP_ID) != view.end()) {
        bmqp::Protocol::MsgGroupId msgGroupId(alloc);
        view.loadMsgGroupIdOption(&msgGroupId);
    }

    return 0;
}
