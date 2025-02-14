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
// struct HeartbeatChecker
// =======================

/// Enumeration for host health states.
struct HeartbeatMonitor {
    // TYPES
  private:
    bsls::AtomicInt d_packetReceived;
    // Used by smart-heartbeat to detect whether
    // a heartbeat needs to be sent and if
    // channel is stale.  Written (to 1) by the
    // IO thread when receiving a packet, read
    // and reset (to 0) from the heartbeat event
    // in scheduler thread, hence an atomic
    // (however, using the 'relaxed' memory
    // model because we dont need strong
    // ordering guarantees).

    const int d_maxMissedHeartbeats;
    // If non-zero, enable smart-heartbeat and
    // specify that this channel should be
    // proactively resetted if no data has been
    // received from this channel for the
    // 'maxMissedHeartbeat' number of heartbeat
    // intervals.  When enabled, heartbeat
    // requests will be sent if no 'regular'
    // data is being received.

    int d_missedHeartbeatCounter;
    // Counter of how many of the last
    // consecutive 'heartbeat' check events
    // fired with no data received on the
    // channel.  This variable is entirely and
    // solely managed from within the event
    // scheduler thread.

  public:
    // CREATORS
    HeartbeatMonitor(int maxMissedHeartbeats,
                     int initialMissedHeartbeatCounter = 0);

    // PUBLIC MANIPULATORS

    /// Send Hearbeat request to the specified `channel` if there was no
    /// `checkData` call since last invocation of `checkHeartbeat`.  Return
    /// `true` if the current number of missed hearbeats is below the max;
    /// return `false` otherwise indicating that the `channel` is dead.
    bool checkHeartbeat(bmqio::Channel* channel);

    /// Reset this object state as received a packet.  If the specified `event`
    /// is Hearbeat request, reply with Heartbeat response to the specified
    /// `channel`.  Return true if the `event` needs further processing (not a
    /// Hearbeat request or response).
    bool checkData(bmqio::Channel* channel, const bmqp::Event& event);

    // PUBLIC ACCESSORS
    /// Return the max number of missed hearbeats after which the channel
    /// should be considered as dead.
    int maxMissedHeartbeats() const;
};

inline int HeartbeatMonitor::maxMissedHeartbeats() const
{
    return d_maxMissedHeartbeats;
}

}  // close package namespace
}  // close enterprise namespace

#endif
