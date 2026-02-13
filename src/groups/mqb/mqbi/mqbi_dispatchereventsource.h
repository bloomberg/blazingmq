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
//@DESCRIPTION: 'mqbi::DispatcherEventSource' provides an interface for
// acquiring typed dispatcher events.  Events can be obtained in two ways:
//
//: o Concrete getters: 'getAckEvent()', 'getPutEvent()', etc.
//: o Template getter: 'get<EVENT_TYPE>()' which dispatches to the
//:   appropriate concrete getter based on the template parameter.

// BDE
#include <bsl_memory.h>  // bsl::shared_ptr

namespace BloombergLP {

// FORWARD DECLARATIONS
namespace mqbi {
class DispatcherEvent;
}
namespace mqbevt {
class AckEvent;
class CallbackEvent;
class ClusterStateEvent;
class ConfirmEvent;
class ControlMessageEvent;
class DispatcherEvent;
class PushEvent;
class PutEvent;
class ReceiptEvent;
class RecoveryEvent;
class RejectEvent;
class StorageEvent;
}

namespace mqbi {

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

    /// Return a shared pointer to a typed event from this event source.
    /// Use the template method `get<EVENT_TYPE>()` for convenient access.
    virtual bsl::shared_ptr<mqbevt::AckEvent>      getAckEvent()      = 0;
    virtual bsl::shared_ptr<mqbevt::CallbackEvent> getCallbackEvent() = 0;
    virtual bsl::shared_ptr<mqbevt::ClusterStateEvent>
                                                  getClusterStateEvent() = 0;
    virtual bsl::shared_ptr<mqbevt::ConfirmEvent> getConfirmEvent()      = 0;
    virtual bsl::shared_ptr<mqbevt::ControlMessageEvent>
    getControlMessageEvent()                                              = 0;
    virtual bsl::shared_ptr<mqbevt::DispatcherEvent> getDispatcherEvent() = 0;
    virtual bsl::shared_ptr<mqbevt::PushEvent>       getPushEvent()       = 0;
    virtual bsl::shared_ptr<mqbevt::PutEvent>        getPutEvent()        = 0;
    virtual bsl::shared_ptr<mqbevt::ReceiptEvent>    getReceiptEvent()    = 0;
    virtual bsl::shared_ptr<mqbevt::RecoveryEvent>   getRecoveryEvent()   = 0;
    virtual bsl::shared_ptr<mqbevt::RejectEvent>     getRejectEvent()     = 0;
    virtual bsl::shared_ptr<mqbevt::StorageEvent>    getStorageEvent()    = 0;

    /// Return a shared pointer to an event of the specified `EVENT_TYPE`.
    template <class EVENT_TYPE>
    bsl::shared_ptr<EVENT_TYPE> get();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class DispatcherEventSource
// ---------------------------

template <>
inline bsl::shared_ptr<mqbevt::AckEvent>
DispatcherEventSource::get<mqbevt::AckEvent>()
{
    return getAckEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::CallbackEvent>
DispatcherEventSource::get<mqbevt::CallbackEvent>()
{
    return getCallbackEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::ClusterStateEvent>
DispatcherEventSource::get<mqbevt::ClusterStateEvent>()
{
    return getClusterStateEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::ConfirmEvent>
DispatcherEventSource::get<mqbevt::ConfirmEvent>()
{
    return getConfirmEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::ControlMessageEvent>
DispatcherEventSource::get<mqbevt::ControlMessageEvent>()
{
    return getControlMessageEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::DispatcherEvent>
DispatcherEventSource::get<mqbevt::DispatcherEvent>()
{
    return getDispatcherEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::PushEvent>
DispatcherEventSource::get<mqbevt::PushEvent>()
{
    return getPushEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::PutEvent>
DispatcherEventSource::get<mqbevt::PutEvent>()
{
    return getPutEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::ReceiptEvent>
DispatcherEventSource::get<mqbevt::ReceiptEvent>()
{
    return getReceiptEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::RecoveryEvent>
DispatcherEventSource::get<mqbevt::RecoveryEvent>()
{
    return getRecoveryEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::RejectEvent>
DispatcherEventSource::get<mqbevt::RejectEvent>()
{
    return getRejectEvent();
}

template <>
inline bsl::shared_ptr<mqbevt::StorageEvent>
DispatcherEventSource::get<mqbevt::StorageEvent>()
{
    return getStorageEvent();
}

}  // close package namespace
}  // close enterprise namespace

#endif
