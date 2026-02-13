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

// mqbevt_putevent.h -*-C++-*-
#ifndef INCLUDED_MQBEVT_PUTEVENT
#define INCLUDED_MQBEVT_PUTEVENT

//@PURPOSE: Provide a DispatcherEvent interface view for 'e_PUT' events.
//
//@CLASSES:
//  mqbevt::PutEvent: Interface view for 'e_PUT' events
//
//@DESCRIPTION: 'mqbevt::PutEvent' provides a DispatcherEvent interface
// view of an event of type 'e_PUT'.

// BMQ
#include <bmqp_protocol.h>
#include <bmqu_atomicstate.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbi {
class QueueHandle;
}

namespace mqbnet {
class ClusterNode;
}

namespace mqbevt {

// ==============
// class PutEvent
// ==============

/// DispatcherEvent interface view of an event of type `e_PUT`.
class PutEvent {
  public:
    // CREATORS

    /// Destructor.
    virtual ~PutEvent();

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this putEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    virtual const bsl::shared_ptr<bdlbb::Blob>& blob() const = 0;

    virtual const bsl::shared_ptr<bdlbb::Blob>& options() const = 0;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    virtual mqbnet::ClusterNode* clusterNode() const = 0;

    /// Return whether this event is a relay event or not.
    virtual bool isRelay() const = 0;

    /// Return a reference not offering modifiable access to the put header
    /// associated to this event.  This protocol struct is only valid when
    /// `isRelay() == true`.
    virtual const bmqp::PutHeader& putHeader() const = 0;

    /// TBD:
    virtual mqbi::QueueHandle* queueHandle() const = 0;

    /// PUT messages carry `genCount`; if there is a mismatch between PUT
    /// `genCount` and current upstream 'genCount, then the PUT message gets
    /// dropped to avoid out of order PUTs.
    virtual bsls::Types::Uint64 genCount() const = 0;

    virtual const bsl::shared_ptr<bmqu::AtomicState>& state() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
