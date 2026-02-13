// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqbevt_dispatcherevent.h                                           -*-C++-*-
#ifndef INCLUDED_MQBEVT_DISPATCHEREVENT
#define INCLUDED_MQBEVT_DISPATCHEREVENT

//@PURPOSE: Provide a DispatcherEvent interface view for 'e_DISPATCHER' events.
//
//@CLASSES:
//  mqbevt::DispatcherEvent: Interface view for 'e_DISPATCHER' events
//
//@DESCRIPTION: 'mqbevt::DispatcherEvent' provides a DispatcherEvent interface
// view of an event of type 'e_DISPATCHER'.

// MQB
#include <bmqu_managedcallback.h>

namespace BloombergLP {
namespace mqbevt {

// =====================
// class DispatcherEvent
// =====================

/// DispatcherEvent interface view of an event of type `e_DISPATCHER`.
class DispatcherEvent {
  public:
    // CREATORS

    /// Destructor.
    virtual ~DispatcherEvent();

    // ACCESSORS

    /// Return a reference not offering modifiable access to the callback
    /// associated to this event.
    virtual const bmqu::ManagedCallback& callback() const = 0;

    /// Return a reference not offering modifiable access to the finalize
    /// callback, if any, associated to this event.
    virtual const bmqu::ManagedCallback& finalizeCallback() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
