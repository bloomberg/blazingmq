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

// mqbevt_dispatcherevent.h                                           -*-C++-*-
#ifndef INCLUDED_MQBEVT_DISPATCHEREVENT
#define INCLUDED_MQBEVT_DISPATCHEREVENT

//@PURPOSE: Provide a concrete DispatcherEvent for 'e_DISPATCHER' events.
//
//@CLASSES:
//  mqbevt::DispatcherEvent: Concrete event for 'e_DISPATCHER' events
//
//@DESCRIPTION: 'mqbevt::DispatcherEvent' provides a concrete implementation
// of a dispatcher event of type 'e_DISPATCHER'.

// MQB
#include <mqbi_dispatcher.h>

// BMQ
#include <bmqu_managedcallback.h>

// BDE
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace mqbevt {

// =====================
// class DispatcherEvent
// =====================

/// Concrete dispatcher event for 'e_DISPATCHER' type events.
class DispatcherEvent : public mqbi::DispatcherEvent {
  public:
    // CLASS DATA

    /// The event type constant for this event class.
    static const mqbi::DispatcherEventType::Enum k_TYPE =
        mqbi::DispatcherEventType::e_DISPATCHER;

  private:
    // DATA

    /// Source client of this event.
    mqbi::DispatcherClient* d_source_p;

    /// In-place storage for the callback in this event.
    bmqu::ManagedCallback d_callback;

    /// Callback embedded in this event.  This callback is called when the
    /// 'Dispatcher::execute' method is used to enqueue an event to multiple
    /// processors, and will be called when the last processor finished
    /// processing it.
    bmqu::ManagedCallback d_finalizeCallback;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherEvent, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor using the specified `allocator`.
    explicit DispatcherEvent(bslma::Allocator* allocator);

    /// Destructor.
    ~DispatcherEvent() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Return a reference offering modifiable access to the callback.
    bmqu::ManagedCallback& callback();

    /// Return a reference offering modifiable access to the finalize
    /// callback.
    bmqu::ManagedCallback& finalizeCallback();

    /// Set the callback to the specified `value` and return a reference
    /// offering modifiable access to this object.
    DispatcherEvent& setCallback(const mqbi::Dispatcher::VoidFunctor& value);
    DispatcherEvent&
    setCallback(bslmf::MovableRef<mqbi::Dispatcher::VoidFunctor> value);

    /// Set the finalize callback to the specified `value` and return a
    /// reference offering modifiable access to this object.
    DispatcherEvent&
    setFinalizeCallback(const mqbi::Dispatcher::VoidFunctor& value);
    DispatcherEvent& setFinalizeCallback(
        bslmf::MovableRef<mqbi::Dispatcher::VoidFunctor> value);

    /// Set the source client to the specified `value` and return a
    /// reference offering modifiable access to this object.
    DispatcherEvent& setSource(mqbi::DispatcherClient* value);

    /// Reset all members of this event to default values.
    void reset() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the source client of this event.
    mqbi::DispatcherClient* source() const;

    /// Return a reference not offering modifiable access to the callback
    /// associated to this event.
    const bmqu::ManagedCallback& callback() const;

    /// Return a reference not offering modifiable access to the finalize
    /// callback, if any, associated to this event.
    const bmqu::ManagedCallback& finalizeCallback() const;

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

// ---------------------
// class DispatcherEvent
// ---------------------

inline mqbi::DispatcherClient* DispatcherEvent::source() const
{
    return d_source_p;
}

inline bmqu::ManagedCallback& DispatcherEvent::callback()
{
    return d_callback;
}

inline bmqu::ManagedCallback& DispatcherEvent::finalizeCallback()
{
    return d_finalizeCallback;
}

inline const bmqu::ManagedCallback& DispatcherEvent::callback() const
{
    return d_callback;
}

inline const bmqu::ManagedCallback& DispatcherEvent::finalizeCallback() const
{
    return d_finalizeCallback;
}

inline mqbi::DispatcherEventType::Enum DispatcherEvent::type() const
{
    return k_TYPE;
}

inline DispatcherEvent&
DispatcherEvent::setCallback(const mqbi::Dispatcher::VoidFunctor& value)
{
    d_callback.set(value);
    return *this;
}

inline DispatcherEvent& DispatcherEvent::setCallback(
    bslmf::MovableRef<mqbi::Dispatcher::VoidFunctor> value)
{
    d_callback.set(value);
    return *this;
}

inline DispatcherEvent& DispatcherEvent::setFinalizeCallback(
    const mqbi::Dispatcher::VoidFunctor& value)
{
    d_finalizeCallback.set(value);
    return *this;
}

inline DispatcherEvent& DispatcherEvent::setFinalizeCallback(
    bslmf::MovableRef<mqbi::Dispatcher::VoidFunctor> value)
{
    d_finalizeCallback.set(value);
    return *this;
}

inline DispatcherEvent&
DispatcherEvent::setSource(mqbi::DispatcherClient* value)
{
    d_source_p = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
