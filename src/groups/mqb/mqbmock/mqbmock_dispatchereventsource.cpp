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

// mqbmock_dispatchereventsource.cpp                                  -*-C++-*-
#include <mqbmock_dispatchereventsource.h>

#include <mqbscm_version.h>

// MQB
#include <mqbevt_ackevent.h>
#include <mqbevt_callbackevent.h>
#include <mqbevt_clusterstateevent.h>
#include <mqbevt_confirmevent.h>
#include <mqbevt_controlmessageevent.h>
#include <mqbevt_dispatcherevent.h>
#include <mqbevt_pushevent.h>
#include <mqbevt_putevent.h>
#include <mqbevt_receiptevent.h>
#include <mqbevt_recoveryevent.h>
#include <mqbevt_rejectevent.h>
#include <mqbevt_storageevent.h>

// BDE
#include <bslma_default.h>

namespace BloombergLP {
namespace mqbmock {

// --------------------------
// class DispatcherEventSource
// --------------------------

DispatcherEventSource::DispatcherEventSource(bslma::Allocator* allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
{
    // NOTHING
}

DispatcherEventSource::~DispatcherEventSource()
{
    // NOTHING
}

bsl::shared_ptr<mqbevt::AckEvent> DispatcherEventSource::getAckEvent()
{
    return bsl::allocate_shared<mqbevt::AckEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::CallbackEvent>
DispatcherEventSource::getCallbackEvent()
{
    return bsl::allocate_shared<mqbevt::CallbackEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::ClusterStateEvent>
DispatcherEventSource::getClusterStateEvent()
{
    return bsl::allocate_shared<mqbevt::ClusterStateEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::ConfirmEvent> DispatcherEventSource::getConfirmEvent()
{
    return bsl::allocate_shared<mqbevt::ConfirmEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::ControlMessageEvent>
DispatcherEventSource::getControlMessageEvent()
{
    return bsl::allocate_shared<mqbevt::ControlMessageEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::DispatcherEvent>
DispatcherEventSource::getDispatcherEvent()
{
    return bsl::allocate_shared<mqbevt::DispatcherEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::PushEvent> DispatcherEventSource::getPushEvent()
{
    return bsl::allocate_shared<mqbevt::PushEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::PutEvent> DispatcherEventSource::getPutEvent()
{
    return bsl::allocate_shared<mqbevt::PutEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::ReceiptEvent> DispatcherEventSource::getReceiptEvent()
{
    return bsl::allocate_shared<mqbevt::ReceiptEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::RecoveryEvent>
DispatcherEventSource::getRecoveryEvent()
{
    return bsl::allocate_shared<mqbevt::RecoveryEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::RejectEvent> DispatcherEventSource::getRejectEvent()
{
    return bsl::allocate_shared<mqbevt::RejectEvent>(d_allocator_p);
}

bsl::shared_ptr<mqbevt::StorageEvent> DispatcherEventSource::getStorageEvent()
{
    return bsl::allocate_shared<mqbevt::StorageEvent>(d_allocator_p);
}

}  // close package namespace
}  // close enterprise namespace
