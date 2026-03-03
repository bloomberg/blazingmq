// Copyright 2014-2026 Bloomberg Finance L.P.
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

// mqbi_dispatchereventsource.h                                       -*-C++-*-
#ifndef INCLUDED_MQBI_DISPATCHEREVENTSOURCE
#define INCLUDED_MQBI_DISPATCHEREVENTSOURCE

//@PURPOSE: Provide an interface for a DispatcherEventSource.
//
//@CLASSES:
//  mqbi::DispatcherEventSource: Interface for a DispatcherEventSource
//
//@DESCRIPTION: 'mqbi::DispatcherEventSource' provide an interface for
//  a DispatcherEventSource.

// BDE
#include <bsl_memory.h>  // bsl::shared_ptr

namespace BloombergLP {

namespace mqbi {

// FORWARD DECLARATIONS
class DispatcherEvent;

// ===========================
// class DispatcherEventSource
// ===========================

class DispatcherEventSource {
  public:
    // PUBLIC TYPES
    typedef bsl::shared_ptr<mqbi::DispatcherEvent> DispatcherEventSp;

    // CREATORS
    virtual ~DispatcherEventSource();

    // MANIPULATORS

    /// @brief Get an event for mqbi::Dispatcher.
    /// @return A shared pointer to event.
    /// The behaviour is undefined unless all the shared pointers to events
    /// acquired with `getEvent` are destructed before destructor is called
    /// for this event source.
    virtual DispatcherEventSp getEvent() = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
