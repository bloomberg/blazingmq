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

// mqbevt_receiptevent.h -*-C++-*-
#ifndef INCLUDED_MQBEVT_RECEIPTEVENT
#define INCLUDED_MQBEVT_RECEIPTEVENT

//@PURPOSE: Provide a DispatcherEvent interface view for
//          'e_REPLICATION_RECEIPT' events.
//
//@CLASSES:
//  mqbevt::ReceiptEvent: Interface view for 'e_REPLICATION_RECEIPT' events
//
//@DESCRIPTION: 'mqbevt::ReceiptEvent' provides a DispatcherEvent interface
// view of an event of type 'e_REPLICATION_RECEIPT'.

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}

namespace mqbevt {

// ==================
// class ReceiptEvent
// ==================

/// DispatcherEvent interface view of an event of type `e_REPLICATION_RECEIPT`.
class ReceiptEvent {
  public:
    // CREATORS

    /// Destructor.
    virtual ~ReceiptEvent();

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this recoveryEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.
    virtual const bsl::shared_ptr<bdlbb::Blob>& blob() const = 0;

    virtual mqbnet::ClusterNode* clusterNode() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
