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

// mqba_dispatchereventsource.cpp                                     -*-C++-*-
#include <mqba_dispatchereventsource.h>

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

namespace BloombergLP {
namespace mqba {

namespace {
const int k_POOL_GROW_BY = 1024;
}  // close unnamed namespace

// --------------------------
// class DispatcherEventSource
// --------------------------

DispatcherEventSource::DispatcherEventSource(bslma::Allocator* allocator)
: d_ackEventPool(k_POOL_GROW_BY, allocator)
, d_callbackEventPool(k_POOL_GROW_BY, allocator)
, d_clusterStateEventPool(k_POOL_GROW_BY, allocator)
, d_confirmEventPool(k_POOL_GROW_BY, allocator)
, d_controlMessageEventPool(k_POOL_GROW_BY, allocator)
, d_dispatcherEventPool(k_POOL_GROW_BY, allocator)
, d_pushEventPool(k_POOL_GROW_BY, allocator)
, d_putEventPool(k_POOL_GROW_BY, allocator)
, d_receiptEventPool(k_POOL_GROW_BY, allocator)
, d_recoveryEventPool(k_POOL_GROW_BY, allocator)
, d_rejectEventPool(k_POOL_GROW_BY, allocator)
, d_storageEventPool(k_POOL_GROW_BY, allocator)
{
    // NOTHING
}

DispatcherEventSource::~DispatcherEventSource()
{
    // NOTHING
}

bsl::shared_ptr<mqbevt::AckEvent> DispatcherEventSource::getAckEvent()
{
    return d_ackEventPool.getObject();
}

bsl::shared_ptr<mqbevt::CallbackEvent>
DispatcherEventSource::getCallbackEvent()
{
    return d_callbackEventPool.getObject();
}

bsl::shared_ptr<mqbevt::ClusterStateEvent>
DispatcherEventSource::getClusterStateEvent()
{
    return d_clusterStateEventPool.getObject();
}

bsl::shared_ptr<mqbevt::ConfirmEvent> DispatcherEventSource::getConfirmEvent()
{
    return d_confirmEventPool.getObject();
}

bsl::shared_ptr<mqbevt::ControlMessageEvent>
DispatcherEventSource::getControlMessageEvent()
{
    return d_controlMessageEventPool.getObject();
}

bsl::shared_ptr<mqbevt::DispatcherEvent>
DispatcherEventSource::getDispatcherEvent()
{
    return d_dispatcherEventPool.getObject();
}

bsl::shared_ptr<mqbevt::PushEvent> DispatcherEventSource::getPushEvent()
{
    return d_pushEventPool.getObject();
}

bsl::shared_ptr<mqbevt::PutEvent> DispatcherEventSource::getPutEvent()
{
    return d_putEventPool.getObject();
}

bsl::shared_ptr<mqbevt::ReceiptEvent> DispatcherEventSource::getReceiptEvent()
{
    return d_receiptEventPool.getObject();
}

bsl::shared_ptr<mqbevt::RecoveryEvent>
DispatcherEventSource::getRecoveryEvent()
{
    return d_recoveryEventPool.getObject();
}

bsl::shared_ptr<mqbevt::RejectEvent> DispatcherEventSource::getRejectEvent()
{
    return d_rejectEventPool.getObject();
}

bsl::shared_ptr<mqbevt::StorageEvent> DispatcherEventSource::getStorageEvent()
{
    return d_storageEventPool.getObject();
}

}  // close package namespace
}  // close enterprise namespace
