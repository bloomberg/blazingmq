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

// mqbevt_pushevent.h -*-C++-*-
#ifndef INCLUDED_MQBEVT_PUSHEVENT
#define INCLUDED_MQBEVT_PUSHEVENT

//@PURPOSE: Provide a DispatcherEvent interface view for 'e_PUSH' events.
//
//@CLASSES:
//  mqbevt::PushEvent: Interface view for 'e_PUSH' events
//
//@DESCRIPTION: 'mqbevt::PushEvent' provides a DispatcherEvent interface
// view of an event of type 'e_PUSH'.

// BMQ
#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}

namespace mqbevt {

// ===============
// class PushEvent
// ===============

/// DispatcherEvent interface view of an event of type `e_PUSH`.
class PushEvent {
  public:
    // CREATORS

    /// Destructor.
    virtual ~PushEvent();

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.  The blob represents the raw content of
    /// the `bmqp::Event` this pushEvent originates from.  The `blob` is
    /// only valid when `isRelay() == false`.  Typically, `blob` is used
    /// when there may be multiple push messages; while `guid` and `queueId`
    /// are used when only one is present.
    virtual const bsl::shared_ptr<bdlbb::Blob>& blob() const = 0;

    virtual const bsl::shared_ptr<bdlbb::Blob>& options() const = 0;

    /// Return a pointer to the cluster node this event originate from, or
    /// null if it doesn't come from a cluster node.  This is mainly useful
    /// for logging purposes.
    virtual mqbnet::ClusterNode* clusterNode() const = 0;

    /// Return a reference not offering modifiable access to the GUID
    /// associated to this event.  This data member is only valid when
    /// `isRelay() == true`.
    virtual const bmqt::MessageGUID& guid() const = 0;

    /// Return whether this event is a relay event or not.
    virtual bool isRelay() const = 0;

    /// Return the queueId associated to this event.  This data member is
    /// only valid when `isRelay() == true`.
    virtual int queueId() const = 0;

    /// Return a reference not offering modifiable access to the
    /// subQueueInfos associated with a message in this event.
    virtual const bmqp::Protocol::SubQueueInfosArray&
    subQueueInfos() const = 0;

    /// Return (true, *) if the associated PUSH message contains message
    /// properties.  Return (true, true) if the properties is de-compressed
    /// even if the `compressionAlgorithmType` is not `e_NONE`.
    virtual const bmqp::MessagePropertiesInfo&
    messagePropertiesInfo() const = 0;

    /// Return the compression algorithm type using which a message in this
    /// event is compressed.
    virtual bmqt::CompressionAlgorithmType::Enum
    compressionAlgorithmType() const = 0;

    /// Return 'true' if the associated PUSH message is Out-of-Order - not the
    /// first delivery attempt or put-aside (no matching subscription).
    virtual bool isOutOfOrderPush() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
