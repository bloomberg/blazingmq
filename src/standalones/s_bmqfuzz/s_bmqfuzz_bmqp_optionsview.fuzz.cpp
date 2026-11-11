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

#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_string.h>
#include <bslma_default.h>
#include <bsls_assert.h>

#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>

using namespace BloombergLP;

/// @brief Fuzz `bmqp::OptionsView` over the options area in the specified
/// `data` of the specified `size`.
///
/// @details The input is two control bytes followed by the options area.  The
/// control bytes shape the blob like the event blob that the message
/// iterators pass to `OptionsView::reset`, where the options area is preceded
/// by the message headers, followed by the application data, and split across
/// network buffers:
/// - byte 0: the blob buffer size, in [1, 64]
/// - byte 1: the number of filler bytes before (high nibble) and after (low
///   nibble) the options area, each in [0, 15]
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    enum {
        k_NUM_CONTROL_BYTES = 2,
        k_MAX_BUFFER_SIZE   = 64,
        k_MAX_FILLER        = 15
    };

    if (size < k_NUM_CONTROL_BYTES) {
        return 0;
    }

    const int bufferSize   = 1 + data[0] % k_MAX_BUFFER_SIZE;
    const int leadingSize  = data[1] >> 4;
    const int trailingSize = data[1] & k_MAX_FILLER;
    data += k_NUM_CONTROL_BYTES;
    size -= k_NUM_CONTROL_BYTES;

    bslma::Allocator*              alloc = bslma::Default::defaultAllocator();
    bdlbb::PooledBlobBufferFactory bufferFactory(bufferSize, alloc);
    const bsl::string              filler(k_MAX_FILLER, '\xff', alloc);

    bdlbb::Blob blob(&bufferFactory, alloc);
    bdlbb::BlobUtil::append(&blob, filler.data(), leadingSize);
    bdlbb::BlobUtil::append(&blob,
                            reinterpret_cast<const char*>(data),
                            static_cast<int>(size));
    bdlbb::BlobUtil::append(&blob, filler.data(), trailingSize);

    bmqu::BlobPosition optionsAreaPos;
    const int          posRc = bmqu::BlobUtil::findOffsetSafe(&optionsAreaPos,
                                                     blob,
                                                     leadingSize);
    if (posRc != 0) {
        return 0;
    }

    bmqp::OptionsView view(alloc);
    const int rc = view.reset(&blob, optionsAreaPos, static_cast<int>(size));
    BSLS_ASSERT_OPT(view.isValid() == (rc == 0));
    if (rc != 0) {
        return 0;
    }

    for (bmqp::OptionsView::const_iterator it = view.begin(); it != view.end();
         ++it) {
        BSLS_ASSERT_OPT(*it >= bmqp::OptionType::k_LOWEST_SUPPORTED_TYPE);
        BSLS_ASSERT_OPT(*it <= bmqp::OptionType::k_HIGHEST_SUPPORTED_TYPE);
    }

    const bool hasSubQueueIdsOld =
        view.find(bmqp::OptionType::e_SUB_QUEUE_IDS_OLD) != view.end();
    const bool hasSubQueueInfos = view.find(
                                      bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
                                  view.end();

    if (hasSubQueueIdsOld) {
        bmqp::Protocol::SubQueueIdsArrayOld subQueueIds(alloc);
        const int loadRc = view.loadSubQueueIdsOption(&subQueueIds);
        BSLS_ASSERT_OPT(loadRc != 0 || !subQueueIds.empty());
    }

    if (hasSubQueueInfos || hasSubQueueIdsOld) {
        bmqp::Protocol::SubQueueInfosArray subQueueInfos(alloc);
        const int loadRc = view.loadSubQueueInfosOption(&subQueueInfos);
        BSLS_ASSERT_OPT(loadRc != 0 || !subQueueInfos.empty());
    }

    if (view.find(bmqp::OptionType::e_MSG_GROUP_ID) != view.end()) {
        bmqp::Protocol::MsgGroupId msgGroupId(alloc);
        const int loadRc = view.loadMsgGroupIdOption(&msgGroupId);
        BSLS_ASSERT_OPT(loadRc != 0 ||
                        (!msgGroupId.empty() &&
                         msgGroupId.size() <=
                             static_cast<size_t>(
                                 bmqp::Protocol::k_MSG_GROUP_ID_MAX_LENGTH)));
    }

    // Exercise copy and dump
    bmqp::OptionsView copy(view, alloc);
    BSLS_ASSERT_OPT(copy.isValid());

    bmqu::MemOutStream os(alloc);
    view.dumpBlob(os);

    view.clear();
    BSLS_ASSERT_OPT(!view.isValid());

    return 0;
}
