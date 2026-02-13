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

// mqbevt_confirmevent.h -*-C++-*-
#ifndef INCLUDED_MQBEVT_CONFIRMEVENT
#define INCLUDED_MQBEVT_CONFIRMEVENT

//@PURPOSE: Provide a DispatcherEvent interface view for 'e_CONFIRM' events.
//
//@CLASSES:
//  mqbevt::ConfirmEvent: Interface view for 'e_CONFIRM' events
//
//@DESCRIPTION: 'mqbevt::ConfirmEvent' provides a DispatcherEvent interface
// view of an event of type 'e_CONFIRM'.

// BMQ
#include <bmqp_protocol.h>

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
// class ConfirmEvent
// ==================

/// DispatcherEvent interface view of an event of type `e_CONFIRM`.
class ConfirmEvent {
  public:
    // CREATORS

    /// Destructor.
    virtual ~ConfirmEvent();

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this confirmEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple confirm messages; while `confirmMessage`
    /// is used when only one is present.
    virtual const bsl::shared_ptr<bdlbb::Blob>& blob() const = 0;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    virtual mqbnet::ClusterNode* clusterNode() const = 0;

    /// Return a reference not offering modifiable access to the confirm
    /// message associated to this event.  This protocol struct is only
    /// valid when `isRelay() == true`.
    virtual const bmqp::ConfirmMessage& confirmMessage() const = 0;

    /// Return whether this event is a relay event or not.
    virtual bool isRelay() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
