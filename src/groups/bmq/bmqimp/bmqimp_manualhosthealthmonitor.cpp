// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqimp_manualhosthealthmonitor.cpp                                 -*-C++-*-
#include <bmqimp_manualhosthealthmonitor.h>

#include <bmqscm_version.h>
namespace BloombergLP {
namespace bmqimp {

// -----------------------------
// class ManualHostHealthMonitor
// -----------------------------

ManualHostHealthMonitor::ManualHostHealthMonitor(
    bmqt::HostHealthState::Enum state,
    bslma::Allocator*           allocator)
: d_state(state)
, d_signaler(allocator)
{
    // NOTHING
}

ManualHostHealthMonitor::~ManualHostHealthMonitor()
{
    // NOTHING
}

bdlmt::SignalerConnection ManualHostHealthMonitor::observeHostHealth(
    const bmqpi::HostHealthMonitor::HostHealthChangeFn& cb)
{
    return d_signaler.connect(cb);
}

void ManualHostHealthMonitor::setState(bmqt::HostHealthState::Enum newState)
{
    if (d_state != newState) {
        d_state = newState;
        d_signaler(newState);
    }
}

bmqt::HostHealthState::Enum ManualHostHealthMonitor::hostState() const
{
    return d_state;
}

bsl::size_t ManualHostHealthMonitor::numRegistrants() const
{
    return d_signaler.slotCount();
}
}  // close package namespace
}  // close enterprise namespace
