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

// bmqa_manualhosthealthmonitor.cpp                                   -*-C++-*-
#include <bmqa_manualhosthealthmonitor.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_manualhosthealthmonitor.h>

// BDE
#include <bslma_allocator.h>
#include <bslma_default.h>

namespace BloombergLP {
namespace bmqa {

// -----------------------------
// class ManualHostHealthMonitor
// -----------------------------

ManualHostHealthMonitor::ManualHostHealthMonitor(
    bmqt::HostHealthState::Enum initialState,
    bslma::Allocator*           allocator)
: d_impl_sp()
{
    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    d_impl_sp.reset(new (*alloc)
                        bmqimp::ManualHostHealthMonitor(initialState, alloc),
                    alloc);
}

ManualHostHealthMonitor::~ManualHostHealthMonitor()
{
    // NOTHING
}

bdlmt::SignalerConnection
ManualHostHealthMonitor::observeHostHealth(const HostHealthChangeFn& cb)
{
    return d_impl_sp->observeHostHealth(cb);
}

void ManualHostHealthMonitor::setState(bmqt::HostHealthState::Enum newState)
{
    bmqimp::ManualHostHealthMonitor* impl =
        reinterpret_cast<bmqimp::ManualHostHealthMonitor*>(d_impl_sp.get());
    impl->setState(newState);
}

bmqt::HostHealthState::Enum ManualHostHealthMonitor::hostState() const
{
    return d_impl_sp->hostState();
}

}  // close package namespace
}  // close enterprise namespace
