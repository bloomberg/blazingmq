// Copyright 2025 Bloomberg Finance L.P.
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

// bmqp_heartbeatmonitor.cpp                                          -*-C++-*-
#include <bmqp_heartbeatmonitor.h>
#include <bmqp_protocolutil.h>

namespace BloombergLP {
namespace bmqp {

// -----------------------
// struct HeartbeatMonitor
// -----------------------

HeartbeatMonitor::HeartbeatMonitor(int maxMissedHeartbeats,
                                   int initialMissedHeartbeatCounter)
: d_packetReceived(0)
, d_maxMissedHeartbeats(maxMissedHeartbeats)
, d_missedHeartbeatCounter(initialMissedHeartbeatCounter)
{
    // NOTHING
}

bool HeartbeatMonitor::checkHeartbeat(bmqio::Channel* channel)
{
    // executed by the *SCHEDULER* thread
    BSLS_ASSERT_SAFE(channel);
    BSLS_ASSERT_SAFE(maxMissedHeartbeats() > 0);

    // Make sure the remote peer is alive by checking _incoming_ traffic.
    // Absence of data since the last 'onHeartbeatSchedulerEvent' triggers
    // sending heartbeat request - 'heartbeatReqBlob()'.
    // The remote peer is supposed to reply with a heartbeat response -
    // 'heartbeatRspBlob()'.  After 'd_maxMissedHeartbeat + 1' invocations
    // with no incoming data, forcibly close the channel.
    // There is no concern about remote peer sending high rate of data
    // because sending 'heartbeatRspBlob()' is done in IO thread
    // immediately upon receiving 'heartbeatReqBlob()'.

    // Perform 'incoming' traffic channel monitoring

    bool result = true;

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(d_packetReceived.loadRelaxed() !=
                                            0)) {
        // A packet was received on the channel since the last heartbeat
        // check, simply reset the associated counters.
        d_packetReceived.storeRelaxed(0);
        d_missedHeartbeatCounter = 0;
    }
    else {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (d_missedHeartbeatCounter < d_maxMissedHeartbeats) {
            // Send heartbeat
            channel->write(0,  // status
                           bmqp::ProtocolUtil::heartbeatReqBlob());
            // We explicitly ignore any failure as failure implies issues with
            // the channel, which is what the heartbeat is trying to expose.
        }
        else if (d_missedHeartbeatCounter == d_maxMissedHeartbeats) {
            // This is edge-triggered.   Return 'false' only once to avoid
            // excessive warnings and 'close' calls.

            result = false;
        }

        ++d_missedHeartbeatCounter;
    }

    return result;
}

}  // close package namespace
}  // close enterprise namespace
