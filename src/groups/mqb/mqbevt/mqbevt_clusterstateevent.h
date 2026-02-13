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

// mqbevt_clusterstateevent.h                                         -*-C++-*-
#ifndef INCLUDED_MQBEVT_CLUSTERSTATEEVENT
#define INCLUDED_MQBEVT_CLUSTERSTATEEVENT

//@PURPOSE: Provide a concrete DispatcherEvent for 'e_CLUSTER_STATE' events.
//
//@CLASSES:
//  mqbevt::ClusterStateEvent: Concrete event for 'e_CLUSTER_STATE' events
//
//@DESCRIPTION: 'mqbevt::ClusterStateEvent' provides a concrete implementation
// of a dispatcher event of type 'e_CLUSTER_STATE'.

// MQB
#include <mqbi_dispatcher.h>

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

// =======================
// class ClusterStateEvent
// =======================

/// Concrete dispatcher event for 'e_CLUSTER_STATE' type events.
class ClusterStateEvent : public mqbi::DispatcherEvent {
  public:
    // CLASS DATA

    /// The event type constant for this event class.
    static const mqbi::DispatcherEventType::Enum k_TYPE =
        mqbi::DispatcherEventType::e_CLUSTER_STATE;

  private:
    // DATA

    /// Blob associated to this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// Cluster node this event originates from.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// Whether this event is a relay event.
    /// TODO: this is possibly not used, for some reason it was assigned in
    /// mqbblp_cluster macro
    bool d_isRelay;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterStateEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor using the specified `allocator`.
    explicit ClusterStateEvent(bslma::Allocator* allocator);

    /// Destructor.
    ~ClusterStateEvent() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Set the blob to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ClusterStateEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    /// Set the cluster node to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ClusterStateEvent& setClusterNode(mqbnet::ClusterNode* value);

    /// Set the isRelay flag to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ClusterStateEvent& setIsRelay(bool value);

    /// Reset all members of this event to default values.
    void reset() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a pointer to the cluster node this event originates from.
    mqbnet::ClusterNode* clusterNode() const;

    /// Return whether this event is a relay event.
    bool isRelay() const;

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

inline const bsl::shared_ptr<bdlbb::Blob>& ClusterStateEvent::blob() const
{
    return d_blob_sp;
}

inline mqbnet::ClusterNode* ClusterStateEvent::clusterNode() const
{
    return d_clusterNode_p;
}

inline bool ClusterStateEvent::isRelay() const
{
    return d_isRelay;
}

inline mqbi::DispatcherEventType::Enum ClusterStateEvent::type() const
{
    return k_TYPE;
}

inline ClusterStateEvent&
ClusterStateEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

inline ClusterStateEvent&
ClusterStateEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

inline ClusterStateEvent& ClusterStateEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
