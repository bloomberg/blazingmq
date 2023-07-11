// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbsl_ondisklog.h                                                  -*-C++-*-
#ifndef INCLUDED_MQBSL_ONDISKLOG
#define INCLUDED_MQBSL_ONDISKLOG

//@PURPOSE: Provide an interface for reading and writing to an on-disk log.
//
//@CLASSES:
//  mqbsl::OnDiskLogConfig: VST representing the configuration of on-disk log.
//  mqbsl::OnDiskLog:       Interface for reading and writing to on-disk log.
//
//@SEE_ALSO: mqbsi::Log
//
//@DESCRIPTION: 'mqbsl::OnDiskLog' an interface for reading and writing to an
// on-disk log.
//
/// Thread Safety
///-------------
// Components implementing the 'mqbsl::OnDiskLog' interface are *NOT* required
// to be thread safe.

// MQB

#include <mqbsi_log.h>

// BDE
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace mqbsl {

// ===============
// class OnDiskLog
// ===============

/// This class provides an interface for reading and writing to an on-disk
/// log.
class OnDiskLog : public mqbsi::Log {
  public:
    // CREATORS

    /// Destructor
    ~OnDiskLog() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the config of this on-disk log.
    virtual const mqbsi::LogConfig& config() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
