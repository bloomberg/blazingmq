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

// mqbevt_callbackevent.h                                             -*-C++-*-
#ifndef INCLUDED_MQBEVT_CALLBACKEVENT
#define INCLUDED_MQBEVT_CALLBACKEVENT

//@PURPOSE: Provide a concrete DispatcherEvent for 'e_CALLBACK' events.
//
//@CLASSES:
//  mqbevt::CallbackEvent: Concrete event for 'e_CALLBACK' events
//
//@DESCRIPTION: 'mqbevt::CallbackEvent' provides a concrete implementation
// of a dispatcher event of type 'e_CALLBACK'.

// MQB
#include <mqbi_dispatcher.h>

// BMQ
#include <bmqu_managedcallback.h>

// BDE
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace mqbevt {

// ===================
// class CallbackEvent
// ===================

/// Concrete dispatcher event for 'e_CALLBACK' type events.
class CallbackEvent : public mqbi::DispatcherEvent {
  public:
    // CLASS DATA

    /// The event type constant for this event class.
    static const mqbi::DispatcherEventType::Enum k_TYPE =
        mqbi::DispatcherEventType::e_CALLBACK;

  private:
    // DATA

    /// Source client of this event.
    mqbi::DispatcherClient* d_source_p;

    /// Callback in this event.
    bmqu::ManagedCallback d_callback;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(CallbackEvent, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor using the specified `allocator`.
    explicit CallbackEvent(bslma::Allocator* allocator);

    /// Destructor.
    ~CallbackEvent() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Return a reference offering modifiable access to the callback.
    bmqu::ManagedCallback& callback();

    /// Set the callback to the specified `value` and return a reference
    /// offering modifiable access to this object.
    CallbackEvent& setCallback(const mqbi::Dispatcher::VoidFunctor& value);
    CallbackEvent&
    setCallback(bslmf::MovableRef<mqbi::Dispatcher::VoidFunctor> value);

    /// Set the source client to the specified `value` and return a
    /// reference offering modifiable access to this object.
    CallbackEvent& setSource(mqbi::DispatcherClient* value);

    /// Reset all members of this event to default values.
    void reset() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the source client of this event.
    mqbi::DispatcherClient* source() const;

    /// Return a reference not offering modifiable access to the callback.
    const bmqu::ManagedCallback& callback() const;

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

inline mqbi::DispatcherClient* CallbackEvent::source() const
{
    return d_source_p;
}

inline bmqu::ManagedCallback& CallbackEvent::callback()
{
    return d_callback;
}

inline const bmqu::ManagedCallback& CallbackEvent::callback() const
{
    return d_callback;
}

inline mqbi::DispatcherEventType::Enum CallbackEvent::type() const
{
    return k_TYPE;
}

inline CallbackEvent&
CallbackEvent::setCallback(const mqbi::Dispatcher::VoidFunctor& value)
{
    d_callback.set(value);
    return *this;
}

inline CallbackEvent& CallbackEvent::setCallback(
    bslmf::MovableRef<mqbi::Dispatcher::VoidFunctor> value)
{
    d_callback.set(value);
    return *this;
}

inline CallbackEvent& CallbackEvent::setSource(mqbi::DispatcherClient* value)
{
    d_source_p = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
