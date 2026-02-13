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

// mqba_dispatchereventsource.h                                       -*-C++-*-
#ifndef INCLUDED_MQBA_DISPATCHEREVENTSOURCE
#define INCLUDED_MQBA_DISPATCHEREVENTSOURCE

//@PURPOSE: Provide an implementation of mqbi::DispatcherEventSource.
//
//@CLASSES:
//  mqba::DispatcherEventSource: Implementation of mqbi::DispatcherEventSource
//
//@DESCRIPTION: 'mqba::DispatcherEventSource' provides an implementation of
// the 'mqbi::DispatcherEventSource' interface using object pools to
// efficiently manage dispatcher event allocation.

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
#include <mqbi_dispatchereventsource.h>

// BDE
#include <bdlcc_sharedobjectpool.h>
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mqba {

// ==========================
// class DispatcherEventSource
// ==========================

/// Implementation of mqbi::DispatcherEventSource using object pools for
/// efficient event allocation.
class DispatcherEventSource BSLS_KEYWORD_FINAL
: public mqbi::DispatcherEventSource {
  public:
    // PUBLIC TYPES
    typedef bsl::shared_ptr<mqbi::DispatcherEvent> DispatcherEventSp;
    typedef bslmf::MovableRef<DispatcherEventSp>   DispatcherEventRvRef;

  private:
    // PRIVATE TYPES
    template <class EVENT_TYPE>
    struct PoolTraits {
        typedef bdlcc::SharedObjectPool<
            EVENT_TYPE,
            bdlcc::ObjectPoolFunctors::DefaultCreator,
            bdlcc::ObjectPoolFunctors::Reset<EVENT_TYPE> >
            Pool;
    };

    // DATA
    typename PoolTraits<mqbevt::AckEvent>::Pool      d_ackEventPool;
    typename PoolTraits<mqbevt::CallbackEvent>::Pool d_callbackEventPool;
    typename PoolTraits<mqbevt::ClusterStateEvent>::Pool
                                                    d_clusterStateEventPool;
    typename PoolTraits<mqbevt::ConfirmEvent>::Pool d_confirmEventPool;
    typename PoolTraits<mqbevt::ControlMessageEvent>::Pool
        d_controlMessageEventPool;
    typename PoolTraits<mqbevt::DispatcherEvent>::Pool d_dispatcherEventPool;
    typename PoolTraits<mqbevt::PushEvent>::Pool       d_pushEventPool;
    typename PoolTraits<mqbevt::PutEvent>::Pool        d_putEventPool;
    typename PoolTraits<mqbevt::ReceiptEvent>::Pool    d_receiptEventPool;
    typename PoolTraits<mqbevt::RecoveryEvent>::Pool   d_recoveryEventPool;
    typename PoolTraits<mqbevt::RejectEvent>::Pool     d_rejectEventPool;
    typename PoolTraits<mqbevt::StorageEvent>::Pool    d_storageEventPool;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherEventSource,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a DispatcherEventSource using the specified `allocator`.
    explicit DispatcherEventSource(bslma::Allocator* allocator = 0);

    /// Destructor.
    ~DispatcherEventSource() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Return a shared pointer to a typed event from this event source.
    bsl::shared_ptr<mqbevt::AckEvent> getAckEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::CallbackEvent>
    getCallbackEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::ClusterStateEvent>
    getClusterStateEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::ConfirmEvent>
    getConfirmEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::ControlMessageEvent>
    getControlMessageEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::DispatcherEvent>
    getDispatcherEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::PushEvent> getPushEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::PutEvent>  getPutEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::ReceiptEvent>
    getReceiptEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::RecoveryEvent>
    getRecoveryEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::RejectEvent>
    getRejectEvent() BSLS_KEYWORD_OVERRIDE;
    bsl::shared_ptr<mqbevt::StorageEvent>
    getStorageEvent() BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
