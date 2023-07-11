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

// bmqimp_manualhosthealthmonitor.h                                   -*-C++-*-
#ifndef INCLUDED_BMQIMP_MANUALHOSTHEALTHMONITOR
#define INCLUDED_BMQIMP_MANUALHOSTHEALTHMONITOR

//@PURPOSE: Provide a basic implementation of `HostHealthMonitor`.
//
//@CLASSES:
// bmqimp::ManualHostHealthMonitor: 'HostHealthMonitor' class for which the
// health of the host is explicitly set by the client via setter method.
// This can be useful for unit-tests and for integration with other systems
// for determining host health.
//
//@DESCRIPTION:
// Provides an implementation of the 'bmqpi::HostHealthMonitor' protocol, which
// facilitates easy emulation of changes to host health.

// BMQ

#include <bmqpi_hosthealthmonitor.h>
#include <bmqt_hosthealthstate.h>

// BDE
#include <bdlmt_signaler.h>
#include <bsl_functional.h>
#include <bslma_allocator.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqimp {

// =============================
// class ManualHostHealthMonitor
// =============================

/// Implementation of `HostHealthMonitor`, with methods for manipulation of
/// the host health state.
class ManualHostHealthMonitor : public bmqpi::HostHealthMonitor {
  private:
    // PRIVATE DATA

    // Last set host health state.
    bmqt::HostHealthState::Enum d_state;

    // Subclasses used by unit-tests may wish to introspect this field.

    // Signaler used to notify registrants of a change to host health.
    bdlmt::Signaler<void(bmqt::HostHealthState::Enum)> d_signaler;

  public:
    // CREATORS

    /// Constructs a `ManualHostHealthMonitor` with the given initial state.
    /// Optionally specify an `allocator` to supply memory. If `allocator`
    /// is 0, the currently installed default allocator is used.
    ManualHostHealthMonitor(bmqt::HostHealthState::Enum initialState,
                            bslma::Allocator*           allocator = 0);

    ~ManualHostHealthMonitor() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Registers the specified `cb` to be invoked each time the health of
    /// the host changes.
    bdlmt::SignalerConnection
    observeHostHealth(const HostHealthChangeFn& cb) BSLS_KEYWORD_OVERRIDE;

    /// Assigns a new state for the host health, and triggers the underlying
    /// `d_signaler` if the state has changed.
    void setState(bmqt::HostHealthState::Enum newState);

    // ACCESSORS

    /// Queries the current health of the host.
    bmqt::HostHealthState::Enum hostState() const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of registered observers to the state change.
    /// Useful for unit-testing HostHealthMonitor-BrokerSession integration.
    bsl::size_t numRegistrants() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
