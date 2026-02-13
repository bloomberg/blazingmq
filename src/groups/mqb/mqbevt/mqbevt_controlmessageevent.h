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

// mqbevt_controlmessageevent.h -*-C++-*-
#ifndef INCLUDED_MQBEVT_CONTROLMESSAGEEVENT
#define INCLUDED_MQBEVT_CONTROLMESSAGEEVENT

//@PURPOSE: Provide a DispatcherEvent interface view for 'e_CONTROL_MSG'
// events.
//
//@CLASSES:
//  mqbevt::ControlMessageEvent: Interface view for 'e_CONTROL_MSG' events
//
//@DESCRIPTION: 'mqbevt::ControlMessageEvent' provides a DispatcherEvent
// interface view of an event of type 'e_CONTROL_MSG'.

// BMQ
#include <bmqp_ctrlmsg_messages.h>

namespace BloombergLP {
namespace mqbevt {

// =========================
// class ControlMessageEvent
// =========================

/// DispatcherEvent interface view of an event of type `e_CONTROL_MSG`.
class ControlMessageEvent {
  public:
    // CREATORS

    /// Destructor.
    virtual ~ControlMessageEvent();

    // ACCESSORS

    /// Return a reference not offering modifiable access to the control
    /// message associated to this event.
    virtual const bmqp_ctrlmsg::ControlMessage& controlMessage() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
