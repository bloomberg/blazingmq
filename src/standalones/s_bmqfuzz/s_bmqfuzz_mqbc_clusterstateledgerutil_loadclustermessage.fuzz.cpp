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

#include <mqbc_clusterstateledgerutil.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bmqp_ctrlmsg_messages.h>

using namespace BloombergLP;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1) {
        return 0;
    }
    FuzzedDataProvider provider(data, size);

    const int         offset  = provider.ConsumeIntegralInRange<int>(0, 4096);
    const std::string payload = provider.ConsumeRemainingBytesAsString();

    bdlbb::PooledBlobBufferFactory factory(32);
    bdlbb::Blob                    blob(&factory);
    bdlbb::BlobUtil::append(&blob,
                            payload.data(),
                            static_cast<int>(payload.size()));

    bmqp_ctrlmsg::ClusterMessage clusterMessage;
    mqbc::ClusterStateLedgerUtil::loadClusterMessage(&clusterMessage, blob, offset);
    return 0;
}
