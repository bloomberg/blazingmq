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

// mqbevt_pushevent.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBEVT_PUSHEVENT
#define INCLUDED_MQBEVT_PUSHEVENT

//@PURPOSE: Provide a concrete DispatcherEvent for 'e_PUSH' events.
//
//@CLASSES:
//  mqbevt::PushEvent: Concrete event for 'e_PUSH' events
//
//@DESCRIPTION: 'mqbevt::PushEvent' provides a concrete implementation
// of a dispatcher event of type 'e_PUSH'.

// MQB
#include <mqbi_dispatcher.h>

// BMQ
#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>

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

// ===============
// class PushEvent
// ===============

/// Concrete dispatcher event for 'e_PUSH' type events.
class PushEvent : public mqbi::DispatcherEvent {
  public:
    // CLASS DATA

    /// The event type constant for this event class.
    static const mqbi::DispatcherEventType::Enum k_TYPE =
        mqbi::DispatcherEventType::e_PUSH;

  private:
    // DATA

    /// Source client of this event.
    mqbi::DispatcherClient* d_source_p;

    /// Blob associated to this event.
    bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// Options blob associated to this event.
    bsl::shared_ptr<bdlbb::Blob> d_options_sp;

    /// Cluster node this event originates from.
    mqbnet::ClusterNode* d_clusterNode_p;

    /// GUID associated to this event.
    bmqt::MessageGUID d_guid;

    /// Queue ID associated to this event.
    int d_queueId;

    /// SubQueueInfos associated with a message in this event.
    bmqp::Protocol::SubQueueInfosArray d_subQueueInfos;

    /// Message properties info associated to this event.
    bmqp::MessagePropertiesInfo d_messagePropertiesInfo;

    /// Compression algorithm type using which a message in this event is
    /// compressed.
    bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;

    /// Whether the associated PUSH message is out-of-order.
    bool d_isOutOfOrderPush;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PushEvent, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor using the specified `allocator`.
    explicit PushEvent(bslma::Allocator* allocator);

    /// Destructor.
    ~PushEvent() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Set the blob to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushEvent& setBlob(const bsl::shared_ptr<bdlbb::Blob>& value);

    /// Set the options blob to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushEvent& setOptions(const bsl::shared_ptr<bdlbb::Blob>& value);

    /// Set the cluster node to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushEvent& setClusterNode(mqbnet::ClusterNode* value);

    /// Set the GUID to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushEvent& setGuid(const bmqt::MessageGUID& value);

    /// Set the isRelay flag to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushEvent& setIsRelay(bool value);

    /// Set the queue ID to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushEvent& setQueueId(int value);

    /// Return a reference offering modifiable access to the subQueueInfos.
    bmqp::Protocol::SubQueueInfosArray& subQueueInfos();

    /// Set the subQueueInfos to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushEvent&
    setSubQueueInfos(const bmqp::Protocol::SubQueueInfosArray& value);

    /// Set the message properties info to the specified `value` and return
    /// a reference offering modifiable access to this object.
    PushEvent&
    setMessagePropertiesInfo(const bmqp::MessagePropertiesInfo& value);

    /// Set the compression algorithm type to the specified `value` and
    /// return a reference offering modifiable access to this object.
    PushEvent&
    setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::Enum value);

    /// Set the outOfOrderPush flag to the specified `value` and return a
    /// reference offering modifiable access to this object.
    PushEvent& setOutOfOrderPush(bool value);

    /// Set the source client to the specified `value` and return a
    /// reference offering modifiable access to this object.
    PushEvent& setSource(mqbi::DispatcherClient* value);

    /// Reset all members of this event to default values.
    void reset() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the source client of this event.
    mqbi::DispatcherClient* source() const;

    /// Return a reference not offering modifiable access to the blob
    /// associated to this event.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;

    /// Return a reference not offering modifiable access to the options
    /// blob associated to this event.
    const bsl::shared_ptr<bdlbb::Blob>& options() const;

    /// Return a pointer to the cluster node this event originates from.
    mqbnet::ClusterNode* clusterNode() const;

    /// Return a reference not offering modifiable access to the GUID
    /// associated to this event.
    const bmqt::MessageGUID& guid() const;

    /// Return the queue ID associated to this event.
    int queueId() const;

    /// Return a reference not offering modifiable access to the
    /// subQueueInfos associated with a message in this event.
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos() const;

    /// Return a reference not offering modifiable access to the message
    /// properties info associated to this event.
    const bmqp::MessagePropertiesInfo& messagePropertiesInfo() const;

    /// Return the compression algorithm type using which a message in this
    /// event is compressed.
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType() const;

    /// Return whether the associated PUSH message is out-of-order.
    bool isOutOfOrderPush() const;

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

inline mqbi::DispatcherClient* PushEvent::source() const
{
    return d_source_p;
}

inline const bsl::shared_ptr<bdlbb::Blob>& PushEvent::blob() const
{
    return d_blob_sp;
}

inline const bsl::shared_ptr<bdlbb::Blob>& PushEvent::options() const
{
    return d_options_sp;
}

inline mqbnet::ClusterNode* PushEvent::clusterNode() const
{
    return d_clusterNode_p;
}

inline const bmqt::MessageGUID& PushEvent::guid() const
{
    return d_guid;
}

inline int PushEvent::queueId() const
{
    return d_queueId;
}

inline bmqp::Protocol::SubQueueInfosArray& PushEvent::subQueueInfos()
{
    return d_subQueueInfos;
}

inline const bmqp::Protocol::SubQueueInfosArray&
PushEvent::subQueueInfos() const
{
    return d_subQueueInfos;
}

inline const bmqp::MessagePropertiesInfo&
PushEvent::messagePropertiesInfo() const
{
    return d_messagePropertiesInfo;
}

inline bmqt::CompressionAlgorithmType::Enum
PushEvent::compressionAlgorithmType() const
{
    return d_compressionAlgorithmType;
}

inline bool PushEvent::isOutOfOrderPush() const
{
    return d_isOutOfOrderPush;
}

inline mqbi::DispatcherEventType::Enum PushEvent::type() const
{
    return k_TYPE;
}

inline PushEvent& PushEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

inline PushEvent&
PushEvent::setOptions(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_options_sp = value;
    return *this;
}

inline PushEvent& PushEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

inline PushEvent& PushEvent::setGuid(const bmqt::MessageGUID& value)
{
    d_guid = value;
    return *this;
}

inline PushEvent& PushEvent::setQueueId(int value)
{
    d_queueId = value;
    return *this;
}

inline PushEvent&
PushEvent::setMessagePropertiesInfo(const bmqp::MessagePropertiesInfo& value)
{
    d_messagePropertiesInfo = value;
    return *this;
}

inline PushEvent& PushEvent::setCompressionAlgorithmType(
    bmqt::CompressionAlgorithmType::Enum value)
{
    d_compressionAlgorithmType = value;
    return *this;
}

inline PushEvent&
PushEvent::setSubQueueInfos(const bmqp::Protocol::SubQueueInfosArray& value)
{
    d_subQueueInfos = value;
    return *this;
}

inline PushEvent& PushEvent::setOutOfOrderPush(bool value)
{
    d_isOutOfOrderPush = value;
    return *this;
}

inline PushEvent& PushEvent::setSource(mqbi::DispatcherClient* value)
{
    d_source_p = value;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
