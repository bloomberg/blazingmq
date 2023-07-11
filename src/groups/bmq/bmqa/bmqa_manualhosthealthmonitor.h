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

// bmqa_manualhosthealthmonitor.h                                     -*-C++-*-
#ifndef INCLUDED_BMQA_MANUALHOSTHEALTHMONITOR
#define INCLUDED_BMQA_MANUALHOSTHEALTHMONITOR

//@PURPOSE: Provide a minimal implementation of 'bmqpi::HostHealthMonitor'.
//
//@CLASSES:
//  bmqa::ManualHostHealthMonitor: A 'HostHealthMonitor' that derives its host
//  health state from a value explicitly set through a setter method.
//
//@DESCRIPTION:
// 'bmqa::ManualHostHealthMonitor' is a minimal implementation of
// 'bmqpi::HostHealthMonitor', which is primarily useful for unit-testing, and
// for integrating with other systems for determining host health.

// BMQ

#include <bmqpi_hosthealthmonitor.h>
#include <bmqt_hosthealthstate.h>

// BDE
#include <bdlmt_signaler.h>
#include <bsl_memory.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace bmqa {

// =============================
// class ManualHostHealthMonitor
// =============================

class ManualHostHealthMonitor : public bmqpi::HostHealthMonitor {
  private:
    // PRIVATE DATA
    bsl::shared_ptr<bmqpi::HostHealthMonitor> d_impl_sp;

  public:
    // CREATORS

    /// Constructs a `ManualHostHealthMonitor` with the given initial state.
    /// Optionally specify an `allocator` to supply memory. If `allocator`
    /// is 0, the currently installed default allocator is used.
    ManualHostHealthMonitor(bmqt::HostHealthState::Enum initialState,
                            bslma::Allocator*           allocator = 0);

    /// Destructor.
    ~ManualHostHealthMonitor() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bdlmt::SignalerConnection
    observeHostHealth(const HostHealthChangeFn& cb) BSLS_KEYWORD_OVERRIDE;

    void setState(bmqt::HostHealthState::Enum newState);

    // ACCESSORS
    bmqt::HostHealthState::Enum hostState() const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
