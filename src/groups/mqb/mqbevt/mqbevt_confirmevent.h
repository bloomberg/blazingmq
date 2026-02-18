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

// mqbevt_confirmevent.h                                              -*-C++-*-
#ifndef INCLUDED_MQBEVT_CONFIRMEVENT
#define INCLUDED_MQBEVT_CONFIRMEVENT

//@PURPOSE: Provide a concrete DispatcherEvent for 'e_CONFIRM' events.
//
//@CLASSES:
//  mqbevt::ConfirmEvent: Concrete event for 'e_CONFIRM' events
//
//@DESCRIPTION: 'mqbevt::ConfirmEvent' provides a concrete implementation
// of a dispatcher event of type 'e_CONFIRM'.

// MQB
#include <mqbi_dispatcher.h>

// BMQ
#include <bmqp_protocol.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}

namespace mqbevt {

// ==================
// class ConfirmEvent
// ==================

/// Concrete dispatcher event for 'e_CONFIRM' type events.
class ConfirmEvent : public mqbi::DispatcherEvent {
  public:
    // CLASS DATA

    /// The event type constant for this event class.
    static const mqbi::DispatcherEventType::Enum k_TYPE =
        mqbi::DispatcherEventType::e_CONFIRM;

  private:
    // DATA

    /// Source client of this event.
    mqbi::DispatcherClient* d_source_p;

    /// Blob associated to this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// Cluster node this event originates from.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// Confirm message associated to this event.
    bmqp::ConfirmMessage d_confirmMessage;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ConfirmEvent, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor using the specified `allocator`.
    explicit ConfirmEvent(bslma::Allocator* allocator);

    /// Destructor.
    ~ConfirmEvent() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Set the blob to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ConfirmEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    /// Set the cluster node to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ConfirmEvent& setClusterNode(mqbnet::ClusterNode* value);

    /// Set the confirm message to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ConfirmEvent& setConfirmMessage(const bmqp::ConfirmMessage& value);

    /// Set the source client to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ConfirmEvent& setSource(mqbi::DispatcherClient* value);

    /// Reset all members of this event to default values.
    void reset() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the source client of this event.
    mqbi::DispatcherClient* source() const;

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a pointer to the cluster node this event originates from.
    mqbnet::ClusterNode* clusterNode() const;

    /// Return a reference not offering modifiable access to the confirm
    /// message associated to this event.
    const bmqp::ConfirmMessage& confirmMessage() const;

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

inline mqbi::DispatcherClient* ConfirmEvent::source() const
{
    return d_source_p;
}

inline const bsl::shared_ptr<bdlbb::Blob>& ConfirmEvent::blob() const
{
    return d_blob_sp;
}

inline mqbnet::ClusterNode* ConfirmEvent::clusterNode() const
{
    return d_clusterNode_p;
}

inline const bmqp::ConfirmMessage& ConfirmEvent::confirmMessage() const
{
    return d_confirmMessage;
}

inline mqbi::DispatcherEventType::Enum ConfirmEvent::type() const
{
    return k_TYPE;
}

inline ConfirmEvent&
ConfirmEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

inline ConfirmEvent& ConfirmEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

inline ConfirmEvent&
ConfirmEvent::setConfirmMessage(const bmqp::ConfirmMessage& value)
{
    d_confirmMessage = value;
    return *this;
}

inline ConfirmEvent& ConfirmEvent::setSource(mqbi::DispatcherClient* value)
{
    d_source_p = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
