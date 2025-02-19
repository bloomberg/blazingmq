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

// bmqp_heartbeatmonitor.h                                            -*-C++-*-
#ifndef INCLUDED_BMQP_HEARTBEATMONITOR
#define INCLUDED_BMQP_HEARTBEATMONITOR

/// @file bmqp_heartbeatmonitor.h
///
/// @brief Connectivity checker.
///
/// Provide a tracker for connection health using heartbeats.

// BMQ
#include <bmqio_channel.h>
#include <bmqp_event.h>

// BDE
#include <bsls_atomic.h>

namespace BloombergLP {
namespace bmqp {

// =======================
// struct HeartbeatMonitor
// =======================

/// The class monitoring channel heartbeat.
struct HeartbeatMonitor {
  private:
    // PRIVATE DATA

    /// A state indicating if the channel is stale and needs to send a
    /// heartbeat.  Written (to 1) by the IO thread when receiving a packet,
    /// read and reset (to 0) from the heartbeat event in the scheduler thread,
    /// hence an atomic (however, using the 'relaxed' memory model because we
    /// don't need strong ordering guarantees).
    bsls::AtomicInt d_packetReceived;

    /// If non-zero, enable heartbeat monitoring and proactively reset the
    /// channel if it receives no data for the 'maxMissedHeartbeat' number of
    /// heartbeat intervals.  When enabled, send a heartbeat request every time
    /// when the channel is stale (no received data) for a heartbeat interval.
    const int d_maxMissedHeartbeats;

    /// Counter of last consecutive 'heartbeat' intervals with no data received
    /// on the channel.  This variable is entirely and solely managed by the
    /// scheduler thread.
    int d_missedHeartbeatCounter;

  public:
    // CREATORS
    explicit HeartbeatMonitor(int maxMissedHeartbeats,
                              int initialMissedHeartbeatCounter = 0);

    // PUBLIC MANIPULATORS

    /// Send Heartbeat request to the specified `channel` if there was no
    /// `checkData` call since last invocation of `checkHeartbeat`.  Return
    /// `true` if the current number of consecutive missed heartbeats is not
    /// equal to the `d_maxMissedHeartbeats`.  Return false otherwise
    /// indicating that the `channel` should be proactively closed.
    bool checkHeartbeat(bmqio::Channel* channel);

    /// Reset this object state as received a packet.  If the specified `event`
    /// is Heartbeat request, reply with Heartbeat response to the specified
    /// `channel`.  Return true if the `event` needs further processing (not a
    /// Heartbeat request or response).
    bool checkData(bmqio::Channel* channel, const bmqp::Event& event);

    // PUBLIC ACCESSORS
    /// Return the max number of missed heartbeats after which the channel
    /// should be considered as dead.
    int maxMissedHeartbeats() const;

    /// Return `true` is the current configuration (`d_maxMissedHeartbeats`) is
    /// valid.  Currently, supporting only positive numbers.
    bool isHearbeatEnabled() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline bool HeartbeatMonitor::checkData(bmqio::Channel*    channel,
                                        const bmqp::Event& event)
{
    BSLS_ASSERT_SAFE(channel);

    d_packetReceived.storeRelaxed(1);

    // Process heartbeat: if we receive a heartbeat request, simply reply
    // with a heartbeat response.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(event.isHeartbeatReqEvent())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        channel->write(0,  // status
                       bmqp::ProtocolUtil::heartbeatRspBlob());
        // We explicitly ignore any failure as failure implies issues with
        // the channel, which is what the heartbeat is trying to expose.
    }
    else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                 event.isHeartbeatRspEvent())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Nothing to be done, we already updated the packet's counter
        // above, just 'drop' that event now.
    }
    else {
        return true;  // RETURN
    }

    return false;
}

inline int HeartbeatMonitor::maxMissedHeartbeats() const
{
    return d_maxMissedHeartbeats;
}

inline bool HeartbeatMonitor::isHearbeatEnabled() const
{
    return d_maxMissedHeartbeats > 0;
}

}  // close package namespace
}  // close enterprise namespace

#endif
