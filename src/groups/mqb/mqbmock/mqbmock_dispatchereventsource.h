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

// mqbmock_dispatchereventsource.h                                    -*-C++-*-
#ifndef INCLUDED_MQBMOCK_DISPATCHEREVENTSOURCE
#define INCLUDED_MQBMOCK_DISPATCHEREVENTSOURCE

//@PURPOSE: Provide a mock implementation of mqbi::DispatcherEventSource.
//
//@CLASSES:
//  mqbmock::DispatcherEventSource: Mock implementation of
//  DispatcherEventSource
//
//@DESCRIPTION: 'mqbmock::DispatcherEventSource' provides a mock implementation
// of the 'mqbi::DispatcherEventSource' interface that allocates new event
// objects on each call using 'bsl::allocate_shared'.

// MQB
#include <mqbi_dispatchereventsource.h>

// BDE
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATIONS
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

namespace mqbmock {

// ==========================
// class DispatcherEventSource
// ==========================

/// Mock implementation of mqbi::DispatcherEventSource that allocates new
/// events using bsl::allocate_shared.
class DispatcherEventSource BSLS_KEYWORD_FINAL
: public mqbi::DispatcherEventSource {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;

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
