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

// mqbevt_putevent.h                                                  -*-C++-*-
#ifndef INCLUDED_MQBEVT_PUTEVENT
#define INCLUDED_MQBEVT_PUTEVENT

//@PURPOSE: Provide a concrete DispatcherEvent for 'e_PUT' events.
//
//@CLASSES:
//  mqbevt::PutEvent: Concrete event for 'e_PUT' events
//
//@DESCRIPTION: 'mqbevt::PutEvent' provides a concrete implementation
// of a dispatcher event of type 'e_PUT'.

// MQB
#include <mqbi_dispatcher.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqu_atomicstate.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
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

/// Concrete dispatcher event for 'e_PUT' type events.
class PutEvent : public mqbi::DispatcherEvent {
  public:
    // CLASS DATA

    /// The event type constant for this event class.
    static const mqbi::DispatcherEventType::Enum k_TYPE =
        mqbi::DispatcherEventType::e_PUT;

  private:
    // DATA

    /// Blob associated to this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// Options blob associated to this event.
    bsl::shared_ptr<bdlbb::Blob> d_options_sp;

    /// Cluster node this event originates from.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// Whether this event is a relay event.
    bool d_isRelay;

    /// Put header associated to this event.
    bmqp::PutHeader d_putHeader;

    /// Queue handle associated to this event.
    mqbi::QueueHandle* d_queueHandle_p;

    /// Generation count for this PUT message.
    bsls::Types::Uint64 d_genCount;

    /// Atomic state for this PUT message.
    bsl::shared_ptr<bmqu::AtomicState> d_state_sp;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PutEvent, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor using the specified `allocator`.
    explicit PutEvent(bslma::Allocator* allocator);

    /// Destructor.
    ~PutEvent() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Set the blob to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PutEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    /// Set the options blob to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PutEvent& setOptions(const bsl::shared_ptr<bdlbb::Blob>& value);

    /// Set the cluster node to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PutEvent& setClusterNode(mqbnet::ClusterNode* value);

    /// Set the isRelay flag to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PutEvent& setIsRelay(bool value);

    /// Set the put header to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PutEvent& setPutHeader(const bmqp::PutHeader& value);

    /// Set the queue handle to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PutEvent& setQueueHandle(mqbi::QueueHandle* value);

    /// Set the generation count to the specified `value` and return a
    /// reference offering modifiable access to this object.
    PutEvent& setGenCount(bsls::Types::Uint64 value);

    /// Set the atomic state to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PutEvent& setState(const bsl::shared_ptr<bmqu::AtomicState>& value);

    /// Reset all members of this event to default values.
    void reset() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a reference not offering modifiable access to the options
    /// blob associated to this event.
    const bsl::shared_ptr<bdlbb::Blob>& options() const;

    /// Return a pointer to the cluster node this event originates from.
    mqbnet::ClusterNode* clusterNode() const;

    /// Return whether this event is a relay event.
    bool isRelay() const;

    /// Return a reference not offering modifiable access to the put header
    /// associated to this event.
    const bmqp::PutHeader& putHeader() const;

    /// Return a pointer to the queue handle associated to this event.
    mqbi::QueueHandle* queueHandle() const;

    /// Return the generation count for this PUT message.
    bsls::Types::Uint64 genCount() const;

    /// Return a reference not offering modifiable access to the atomic
    /// state for this PUT message.
    const bsl::shared_ptr<bmqu::AtomicState>& state() const;

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

inline const bsl::shared_ptr<bdlbb::Blob>& PutEvent::blob() const
{
    return d_blob_sp;
}

inline const bsl::shared_ptr<bdlbb::Blob>& PutEvent::options() const
{
    return d_options_sp;
}

inline mqbnet::ClusterNode* PutEvent::clusterNode() const
{
    return d_clusterNode_p;
}

inline bool PutEvent::isRelay() const
{
    return d_isRelay;
}

inline const bmqp::PutHeader& PutEvent::putHeader() const
{
    return d_putHeader;
}

inline mqbi::QueueHandle* PutEvent::queueHandle() const
{
    return d_queueHandle_p;
}

inline bsls::Types::Uint64 PutEvent::genCount() const
{
    return d_genCount;
}

inline const bsl::shared_ptr<bmqu::AtomicState>& PutEvent::state() const
{
    return d_state_sp;
}

inline mqbi::DispatcherEventType::Enum PutEvent::type() const
{
    return k_TYPE;
}

inline PutEvent& PutEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

inline PutEvent&
PutEvent::setOptions(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_options_sp = value;
    return *this;
}

inline PutEvent& PutEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

inline PutEvent& PutEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}

inline PutEvent& PutEvent::setPutHeader(const bmqp::PutHeader& value)
{
    d_putHeader = value;
    return *this;
}

inline PutEvent& PutEvent::setQueueHandle(mqbi::QueueHandle* value)
{
    d_queueHandle_p = value;
    return *this;
}

inline PutEvent& PutEvent::setGenCount(bsls::Types::Uint64 value)
{
    d_genCount = value;
    return *this;
}

inline PutEvent&
PutEvent::setState(const bsl::shared_ptr<bmqu::AtomicState>& value)
{
    d_state_sp = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
