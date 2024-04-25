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

// bmqpi_hosthealthmonitor.h                                          -*-C++-*-
#ifndef INCLUDED_BMQPI_HOSTHEALTHMONITOR
#define INCLUDED_BMQPI_HOSTHEALTHMONITOR

/// @file bmqpi_hosthealthmonitor.h
///
/// @brief Provide an interface for monitoring the health of the host.
///
/// @bbref{bmqpi::HostHealthMonitor} is a pure interface for a monitor of the
/// health of the host. BlazingMQ sessions can use such objects to
/// conditionally suspend queue activity while the host is marked unhealthy.

// BMQ

#include <bmqt_hosthealthstate.h>

// BDE
#include <bdlmt_signaler.h>
#include <bsl_functional.h>

namespace BloombergLP {
namespace bmqpi {

// =======================
// class HostHealthMonitor
// =======================

/// A pure interface for monitoring the health of the host.
class HostHealthMonitor {
  public:
    // TYPES

    /// Invoked as a response to the HostHealthMonitor detecting a change
    /// in the state of the host health.
    typedef bsl::function<void(bmqt::HostHealthState::Enum)>
        HostHealthChangeFn;

  public:
    // CREATORS

    /// Destructor
    virtual ~HostHealthMonitor();

    // MANIPULATORS

    /// Registers the specified `cb` to be invoked each time the health of
    /// the host changes.
    virtual bdlmt::SignalerConnection
    observeHostHealth(const HostHealthChangeFn& cb) = 0;

    // ACCESSORS

    /// Queries the current health of the host.
    virtual bmqt::HostHealthState::Enum hostState() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
