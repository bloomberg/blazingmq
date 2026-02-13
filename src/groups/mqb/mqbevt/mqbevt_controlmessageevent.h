// Copyright 2026 Bloomberg Finance L.P.
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

// mqbevt_controlmessageevent.h                                       -*-C++-*-
#ifndef INCLUDED_MQBEVT_CONTROLMESSAGEEVENT
#define INCLUDED_MQBEVT_CONTROLMESSAGEEVENT

//@PURPOSE: Provide a concrete DispatcherEvent for 'e_CONTROL_MSG' events.
//
//@CLASSES:
//  mqbevt::ControlMessageEvent: Concrete event for 'e_CONTROL_MSG' events
//
//@DESCRIPTION: 'mqbevt::ControlMessageEvent' provides a concrete
// implementation of a dispatcher event of type 'e_CONTROL_MSG'.

// MQB
#include <mqbi_dispatcher.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace mqbevt {

// =========================
// class ControlMessageEvent
// =========================

/// Concrete dispatcher event for 'e_CONTROL_MSG' type events.
class ControlMessageEvent : public mqbi::DispatcherEvent {
  public:
    // CLASS DATA

    /// The event type constant for this event class.
    static const mqbi::DispatcherEventType::Enum k_TYPE =
        mqbi::DispatcherEventType::e_CONTROL_MSG;

  private:
    // DATA

    /// ControlMessage in this event.
    bmqp_ctrlmsg::ControlMessage d_controlMessage;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ControlMessageEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor using the specified `allocator`.
    explicit ControlMessageEvent(bslma::Allocator* allocator);

    /// Destructor.
    ~ControlMessageEvent() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Set the control message to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ControlMessageEvent&
    setControlMessage(const bmqp_ctrlmsg::ControlMessage& value);

    /// Reset all members of this event to default values.
    void reset() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a reference not offering modifiable access to the control
    /// message associated to this event.
    const bmqp_ctrlmsg::ControlMessage& controlMessage() const;

    /// Return the type of this event.
    mqbi::DispatcherEventType::Enum type() const BSLS_KEYWORD_OVERRIDE;

    /// Format this object to the specified output `stream`.
    bsl::ostream& print(bsl::ostream& stream,
                        int           level = 0,
                        int spacesPerLevel  = 4) const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline const bmqp_ctrlmsg::ControlMessage&
ControlMessageEvent::controlMessage() const
{
    return d_controlMessage;
}

inline mqbi::DispatcherEventType::Enum ControlMessageEvent::type() const
{
    return k_TYPE;
}

inline ControlMessageEvent& ControlMessageEvent::setControlMessage(
    const bmqp_ctrlmsg::ControlMessage& value)
{
    d_controlMessage = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
