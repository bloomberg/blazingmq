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

#include <bmqp_heartbeatmonitor.h>

// BMQ
#include <bmqp_protocol.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_cstring.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_heartbeatBlobs()
// ------------------------------------------------------------------------
// HEARTBEAT BLOBS
//
// Concerns:
//   Verify the lazily created blobs for the heartbeat request and
//   response are correct.
//
// Testing:
//   - static const bdlbb::Blob& heartbeatReqBlob();
//   - static const bdlbb::Blob& heartbeatRspBlob();
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HEARTBEAT BLOBS");

    PV("Verifying the HeartbeatReq blob")
    {
        bmqp::EventHeader expectedHeader(bmqp::EventType::e_HEARTBEAT_REQ);

        const bdlbb::Blob& blob = bmqp::HeartbeatMonitor::heartbeatReqBlob();

        BMQTST_ASSERT_EQ(blob.length(),
                         static_cast<int>(sizeof(bmqp::EventHeader)));
        BMQTST_ASSERT_EQ(blob.numDataBuffers(), 1);
        BMQTST_ASSERT_EQ(0,
                         memcmp(blob.buffer(0).data(),
                                &expectedHeader,
                                sizeof(bmqp::EventHeader)));
    }

    PV("Verifying the HeartbeatRsp blob")
    {
        bmqp::EventHeader expectedHeader(bmqp::EventType::e_HEARTBEAT_RSP);

        const bdlbb::Blob& blob = bmqp::HeartbeatMonitor::heartbeatRspBlob();

        BMQTST_ASSERT_EQ(blob.length(),
                         static_cast<int>(sizeof(bmqp::EventHeader)));
        BMQTST_ASSERT_EQ(blob.numDataBuffers(), 1);
        BMQTST_ASSERT_EQ(0,
                         memcmp(blob.buffer(0).data(),
                                &expectedHeader,
                                sizeof(bmqp::EventHeader)));
    }
}

// ============================================================================
//                                MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_heartbeatBlobs(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
