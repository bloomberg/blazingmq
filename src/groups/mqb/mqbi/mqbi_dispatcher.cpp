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

// mqbi_dispatcher.cpp                                                -*-C++-*-
#include <mqbi_dispatcher.h>

#include <mqbscm_version.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_ostream.h>
#include <bsl_variant.h>
#include <bsla_annotations.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace mqbi {

// ---------------------------
// struct DispatcherClientType
// ---------------------------

bsl::ostream& DispatcherClientType::print(bsl::ostream&              stream,
                                          DispatcherClientType::Enum value,
                                          int                        level,
                                          int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << DispatcherClientType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* DispatcherClientType::toAscii(DispatcherClientType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(SESSION)
        CASE(QUEUE)
        CASE(CLUSTER)
        CASE(ALL)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool DispatcherClientType::fromAscii(DispatcherClientType::Enum* out,
                                     const bslstl::StringRef&    str)
{
#define CHECKVALUE(X)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(DispatcherClientType::e_##X),  \
                                       str)) {                                \
        *out = DispatcherClientType::e_##X;                                   \
        return true;                                                          \
    }

    CHECKVALUE(UNDEFINED)
    CHECKVALUE(SESSION)
    CHECKVALUE(QUEUE)
    CHECKVALUE(CLUSTER)
    CHECKVALUE(ALL)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// --------------------------
// struct DispatcherEventType
// --------------------------

bsl::ostream& DispatcherEventType::print(bsl::ostream&             stream,
                                         DispatcherEventType::Enum value,
                                         int                       level,
                                         int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << DispatcherEventType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* DispatcherEventType::toAscii(DispatcherEventType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(DISPATCHER)
        CASE(CALLBACK)
        CASE(CONTROL_MSG)
        CASE(CONFIRM)
        CASE(REJECT)
        CASE(PUSH)
        CASE(PUT)
        CASE(ACK)
        CASE(CLUSTER_STATE)
        CASE(STORAGE)
        CASE(RECOVERY)
        CASE(REPLICATION_RECEIPT)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool DispatcherEventType::fromAscii(DispatcherEventType::Enum* out,
                                    const bslstl::StringRef&   str)
{
#define CHECKVALUE(X)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(DispatcherEventType::e_##X),   \
                                       str)) {                                \
        *out = DispatcherEventType::e_##X;                                    \
        return true;                                                          \
    }

    CHECKVALUE(UNDEFINED)
    CHECKVALUE(DISPATCHER)
    CHECKVALUE(CALLBACK)
    CHECKVALUE(CONTROL_MSG)
    CHECKVALUE(CONFIRM)
    CHECKVALUE(REJECT)
    CHECKVALUE(PUSH)
    CHECKVALUE(PUT)
    CHECKVALUE(ACK)
    CHECKVALUE(CLUSTER_STATE)
    CHECKVALUE(STORAGE)
    CHECKVALUE(RECOVERY)
    CHECKVALUE(REPLICATION_RECEIPT)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ----------------
// class Dispatcher
// ----------------

// CREATORS
Dispatcher::~Dispatcher()
{
    // NOTHING
}

bsl::ostream& DispatcherEvent::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    // NOTE: We don't print the 'd_clusterNode_p' member because
    //       mqbi::Dispatcher only has a 'by name' only reference.

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();

    // Print the type, source and destination (if any)
    printer.printAttribute("type", type());
    if (source()) {
        printer.printAttribute("source", source()->description());
    }
    if (destination()) {
        printer.printAttribute("destination", destination()->description());
    }

    struct PrintVisitor {
        bslim::Printer& d_printer;

        void operator()(bsl::monostate) const
        {
            // Nothing more to print
        }

        void operator()(const mqbi::DispatcherDispatcherEvent& event) const
        {
            d_printer.printAttribute(
                "hasFinalizeCallback",
                (event.finalizeCallback().empty() ? "yes" : "no"));
        }

        void operator()(const mqbi::DispatcherCallbackEvent& event) const
        {
            // Nothing more to print
        }

        void operator()(const mqbi::DispatcherControlMessageEvent& event) const
        {
            d_printer.printAttribute("controlMessage", event.controlMessage());
        }

        void operator()(const mqbi::DispatcherConfirmEvent& event) const
        {
            d_printer.printAttribute("blobLength",
                                     (event.blob() ? event.blob()->length()
                                                   : -1));
            d_printer.printAttribute("confirmMessage.queueId",
                                     event.confirmMessage().queueId());
            d_printer.printAttribute("confirmMessage.subQueueId",
                                     event.confirmMessage().subQueueId());
            d_printer.printAttribute("confirmMessage.guid",
                                     event.confirmMessage().messageGUID());
            d_printer.printAttribute("partitionId", event.partitionId());
            d_printer.printAttribute("isRelay",
                                     (event.isRelay() ? "true" : "false"));
        }

        void operator()(const mqbi::DispatcherRejectEvent& event) const
        {
            d_printer.printAttribute("blobLength",
                                     (event.blob() ? event.blob()->length()
                                                   : -1));
            d_printer.printAttribute("rejectMessage.queueId",
                                     event.rejectMessage().queueId());
            d_printer.printAttribute("rejectMessage.subqueueId",
                                     event.rejectMessage().subQueueId());
            d_printer.printAttribute("rejectMessage.guid",
                                     event.rejectMessage().messageGUID());
            d_printer.printAttribute("partitionId", event.partitionId());
            d_printer.printAttribute("isRelay",
                                     (event.isRelay() ? "true" : "false"));
        }

        void operator()(const mqbi::DispatcherPushEvent& event) const
        {
            d_printer.printAttribute("blobLength",
                                     (event.blob() ? event.blob()->length()
                                                   : -1));
            d_printer.printAttribute(
                "optionsLength",
                (event.options() ? event.options()->length() : -1));
            d_printer.printAttribute("guid", event.guid());
            d_printer.printAttribute("queueId", event.queueId());
            for (bmqp::Protocol::SubQueueInfosArray::size_type i = 0;
                 i < event.subQueueInfos().size();
                 ++i) {
                bmqu::MemOutStream out;
                out << "subQueueInfo[" << i << "]: ";
                d_printer.printAttribute(out.str().data(),
                                         event.subQueueInfos()[i]);
            }
            d_printer.printAttribute("msgGroupId", event.msgGroupId());
            d_printer.printAttribute("isRelay",
                                     (event.isRelay() ? "true" : "false"));
        }

        void operator()(const mqbi::DispatcherPutEvent& event) const
        {
            d_printer.printAttribute("blobLength",
                                     (event.blob() ? event.blob()->length()
                                                   : -1));
            d_printer.printAttribute(
                "optionsLength",
                (event.options() ? event.options()->length() : -1));
            d_printer.printAttribute("partitionId", event.partitionId());
            d_printer.printAttribute("putHeader.queueId",
                                     event.putHeader().queueId());
            d_printer.printAttribute("putHeader.guid",
                                     event.putHeader().messageGUID());
            d_printer.printAttribute("putHeader.flags",
                                     event.putHeader().flags());
            d_printer.printAttribute("isRelay",
                                     (event.isRelay() ? "true" : "false"));
        }

        void operator()(const mqbi::DispatcherAckEvent& event) const
        {
            d_printer.printAttribute("ackMessage.status",
                                     event.ackMessage().status());
            d_printer.printAttribute("ackMessage.correlationId",
                                     event.ackMessage().correlationId());
            d_printer.printAttribute("ackMessage.guid",
                                     event.ackMessage().messageGUID());
            d_printer.printAttribute("ackMessage.queueId",
                                     event.ackMessage().queueId());
            d_printer.printAttribute("blobLength",
                                     (event.blob() ? event.blob()->length()
                                                   : -1));
            d_printer.printAttribute("isRelay",
                                     (event.isRelay() ? "true" : "false"));
        }

        void operator()(const mqbi::DispatcherClusterStateEvent& event) const
        {
            d_printer.printAttribute("blobLength",
                                     (event.blob() ? event.blob()->length()
                                                   : -1));
        }

        void operator()(const mqbi::DispatcherStorageEvent& event) const
        {
            d_printer.printAttribute("blobLength",
                                     (event.blob() ? event.blob()->length()
                                                   : -1));
            d_printer.printAttribute("isRelay",
                                     (event.isRelay() ? "true" : "false"));
        }

        void operator()(const mqbi::DispatcherRecoveryEvent& event) const
        {
            d_printer.printAttribute("blobLength",
                                     (event.blob() ? event.blob()->length()
                                                   : -1));
            d_printer.printAttribute("isRelay",
                                     (event.isRelay() ? "true" : "false"));
        }

        void operator()(const mqbi::DispatcherReceiptEvent& event) const
        {
            d_printer.printAttribute("blobLength",
                                     (event.blob() ? event.blob()->length()
                                                   : -1));
            // can't parse the blob and print the Receipt struct?
        }
    };

    PrintVisitor printVisitor = {printer};
    bsl::visit(printVisitor, d_eventImpl);

    printer.end();

    return stream;
}

// --------------------------
// class DispatcherClientData
// --------------------------

bsl::ostream& DispatcherClientData::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clientType", d_clientType);
    printer.printAttribute("processorHandle", d_processorHandle);
    printer.printAttribute("addedToFlushList",
                           (d_addedToFlushList ? "yes" : "no"));
    printer.end();

    return stream;
}

// ----------------------
// class DispatcherClient
// ----------------------

DispatcherClient::~DispatcherClient()
{
    // NOTHING
}

// -------------------------------
// class DispatcherDispatcherEvent
// -------------------------------

DispatcherDispatcherEvent::DispatcherDispatcherEvent(
    bslma::Allocator* allocator)
: d_callback(allocator)
, d_finalizeCallback(allocator)
{
    // NOTHING
}

DispatcherDispatcherEvent::DispatcherDispatcherEvent(
    bslmf::MovableRef<DispatcherDispatcherEvent> other)
: d_callback(bslmf::MovableRefUtil::move(other.d_callback))
, d_finalizeCallback(bslmf::MovableRefUtil::move(other.d_finalizeCallback))
{
    // NOTHING
}

const bmqu::ManagedCallback& DispatcherDispatcherEvent::callback() const
{
    return d_callback;
}

const bmqu::ManagedCallback&
DispatcherDispatcherEvent::finalizeCallback() const
{
    return d_finalizeCallback;
}

bmqu::ManagedCallback& DispatcherDispatcherEvent::callback()
{
    return d_callback;
}

bmqu::ManagedCallback& DispatcherDispatcherEvent::finalizeCallback()
{
    return d_finalizeCallback;
}

// -----------------------------
// class DispatcherCallbackEvent
// -----------------------------

DispatcherCallbackEvent::DispatcherCallbackEvent(bslma::Allocator* allocator)
: d_callback(allocator)
{
    // NOTHING
}

DispatcherCallbackEvent::DispatcherCallbackEvent(
    bslmf::MovableRef<DispatcherCallbackEvent> other)
: d_callback(bslmf::MovableRefUtil::move(other.d_callback))
{
    // NOTHING
}

const bmqu::ManagedCallback& DispatcherCallbackEvent::callback() const
{
    return d_callback;
}

bmqu::ManagedCallback& DispatcherCallbackEvent::callback()
{
    return d_callback;
}

// -----------------------------------
// class DispatcherControlMessageEvent
// -----------------------------------

DispatcherControlMessageEvent::DispatcherControlMessageEvent(
    bslma::Allocator* allocator)
: d_controlMessage(allocator)
{
    // NOTHING
}

DispatcherControlMessageEvent::DispatcherControlMessageEvent(
    bslmf::MovableRef<DispatcherControlMessageEvent> other)
: d_controlMessage(bslmf::MovableRefUtil::move(other.d_controlMessage))
{
    // NOTHING
}

const bmqp_ctrlmsg::ControlMessage&
DispatcherControlMessageEvent::controlMessage() const
{
    return d_controlMessage;
}

DispatcherControlMessageEvent&
DispatcherControlMessageEvent::setControlMessage(
    const bmqp_ctrlmsg::ControlMessage& value)
{
    d_controlMessage = value;
    return *this;
}

// ----------------------------
// class DispatcherConfirmEvent
// ----------------------------

DispatcherConfirmEvent::DispatcherConfirmEvent(bslma::Allocator* allocator)
: d_blob_sp(0, allocator)
, d_clusterNode_p(0)
, d_confirmMessage()
, d_partitionId(-1)  // TODO this const is declared in
                     // mqbs::DataStore::k_INVALID_PARTITION_ID
, d_isRelay(false)
{
    // NOTHING
}

DispatcherConfirmEvent::DispatcherConfirmEvent(
    bslmf::MovableRef<DispatcherConfirmEvent> other)
: d_blob_sp(bslmf::MovableRefUtil::move(other.d_blob_sp))
, d_clusterNode_p(other.d_clusterNode_p)
, d_confirmMessage(bslmf::MovableRefUtil::move(other.d_confirmMessage))
, d_partitionId(other.d_partitionId)
, d_isRelay(other.d_isRelay)
{
    // NOTHING
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherConfirmEvent::blob() const
{
    return d_blob_sp;
}

mqbnet::ClusterNode* DispatcherConfirmEvent::clusterNode() const
{
    return d_clusterNode_p;
}

const bmqp::ConfirmMessage& DispatcherConfirmEvent::confirmMessage() const
{
    return d_confirmMessage;
}

int DispatcherConfirmEvent::partitionId() const
{
    return d_partitionId;
}

bool DispatcherConfirmEvent::isRelay() const
{
    return d_isRelay;
}

DispatcherConfirmEvent&
DispatcherConfirmEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

DispatcherConfirmEvent&
DispatcherConfirmEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

DispatcherConfirmEvent&
DispatcherConfirmEvent::setConfirmMessage(const bmqp::ConfirmMessage& value)
{
    d_confirmMessage = value;
    return *this;
}

DispatcherConfirmEvent& DispatcherConfirmEvent::setPartitionId(int value)
{
    d_partitionId = value;
    return *this;
}

DispatcherConfirmEvent& DispatcherConfirmEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}

// ---------------------------
// class DispatcherRejectEvent
// ---------------------------

DispatcherRejectEvent::DispatcherRejectEvent(bslma::Allocator* allocator)
: d_blob_sp(0, allocator)
, d_clusterNode_p(0)
, d_rejectMessage()
, d_partitionId(-1)
, d_isRelay(false)
{
    // NOTHING
}

DispatcherRejectEvent::DispatcherRejectEvent(
    bslmf::MovableRef<DispatcherRejectEvent> other)
: d_blob_sp(bslmf::MovableRefUtil::move(other.d_blob_sp))
, d_clusterNode_p(other.d_clusterNode_p)
, d_rejectMessage(bslmf::MovableRefUtil::move(other.d_rejectMessage))
, d_partitionId(other.d_partitionId)
, d_isRelay(other.d_isRelay)
{
    // NOTHING
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherRejectEvent::blob() const
{
    return d_blob_sp;
}

mqbnet::ClusterNode* DispatcherRejectEvent::clusterNode() const
{
    return d_clusterNode_p;
}

const bmqp::RejectMessage& DispatcherRejectEvent::rejectMessage() const
{
    return d_rejectMessage;
}

int DispatcherRejectEvent::partitionId() const
{
    return d_partitionId;
}

bool DispatcherRejectEvent::isRelay() const
{
    return d_isRelay;
}

DispatcherRejectEvent&
DispatcherRejectEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

DispatcherRejectEvent&
DispatcherRejectEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

DispatcherRejectEvent&
DispatcherRejectEvent::setRejectMessage(const bmqp::RejectMessage& value)
{
    d_rejectMessage = value;
    return *this;
}

DispatcherRejectEvent& DispatcherRejectEvent::setPartitionId(int value)
{
    d_partitionId = value;
    return *this;
}

DispatcherRejectEvent& DispatcherRejectEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}

// -------------------------
// class DispatcherPushEvent
// -------------------------

DispatcherPushEvent::DispatcherPushEvent(bslma::Allocator* allocator)
: d_blob_sp(0, allocator)
, d_options_sp(0, allocator)
, d_clusterNode_p(0)
, d_guid()
, d_messagePropertiesInfo()
, d_queueId(-1)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_isOutOfOrder(false)
, d_isRelay(false)
, d_subQueueInfos(allocator)
, d_msgGroupId(allocator)
{
    // NOTHING
}

DispatcherPushEvent::DispatcherPushEvent(
    bslmf::MovableRef<DispatcherPushEvent> other)
: d_blob_sp(bslmf::MovableRefUtil::move(other.d_blob_sp))
, d_options_sp(bslmf::MovableRefUtil::move(other.d_options_sp))
, d_clusterNode_p(other.d_clusterNode_p)
, d_guid(bslmf::MovableRefUtil::move(other.d_guid))
, d_messagePropertiesInfo(
      bslmf::MovableRefUtil::move(other.d_messagePropertiesInfo))
, d_queueId(other.d_queueId)
, d_compressionAlgorithmType(other.d_compressionAlgorithmType)
, d_isOutOfOrder(other.d_isOutOfOrder)
, d_isRelay(other.d_isRelay)
, d_subQueueInfos(bslmf::MovableRefUtil::move(other.d_subQueueInfos))
, d_msgGroupId(bslmf::MovableRefUtil::move(other.d_msgGroupId))
{
    // NOTHING
}

DispatcherPushEvent::DispatcherPushEvent(
    const bsl::shared_ptr<bdlbb::Blob>&  blob_sp,
    const bmqt::MessageGUID&             msgGUID,
    const bmqp::MessagePropertiesInfo&   mp,
    int                                  queueId,
    bmqt::CompressionAlgorithmType::Enum cat,
    bslma::Allocator*                    allocator)
: d_blob_sp(blob_sp)
, d_options_sp(0, allocator)
, d_clusterNode_p(0)
, d_guid(msgGUID)
, d_messagePropertiesInfo(mp)
, d_queueId(queueId)
, d_compressionAlgorithmType(cat)
, d_isOutOfOrder(false)
, d_isRelay(false)
, d_subQueueInfos(allocator)
, d_msgGroupId(allocator)
{
    // NOTHING
}

DispatcherPushEvent::DispatcherPushEvent(
    const bsl::shared_ptr<bdlbb::Blob>& blob_sp,
    bslma::Allocator*                   allocator)
: d_blob_sp(blob_sp)
, d_options_sp(0, allocator)
, d_clusterNode_p(0)
, d_guid()
, d_messagePropertiesInfo()
, d_queueId(-1)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_isOutOfOrder(false)
, d_isRelay(false)
, d_subQueueInfos(allocator)
, d_msgGroupId(allocator)
{
    // NOTHING
}

DispatcherPushEvent::DispatcherPushEvent(
    const bsl::shared_ptr<bdlbb::Blob>&  blob_sp,
    const bsl::shared_ptr<bdlbb::Blob>&  options_sp,
    const bmqt::MessageGUID&             msgGUID,
    const bmqp::MessagePropertiesInfo&   mp,
    bmqt::CompressionAlgorithmType::Enum cat,
    bool                                 isOutOfOrder,
    bslma::Allocator*                    allocator)
: d_blob_sp(blob_sp)
, d_options_sp(options_sp)
, d_clusterNode_p(0)
, d_guid(msgGUID)
, d_messagePropertiesInfo(mp)
, d_queueId(-1)
, d_compressionAlgorithmType(cat)
, d_isOutOfOrder(isOutOfOrder)
, d_isRelay(false)
, d_subQueueInfos(allocator)
, d_msgGroupId(allocator)
{
    // NOTHING
}

DispatcherPushEvent::DispatcherPushEvent(
    const bsl::shared_ptr<bdlbb::Blob>&       blob_sp,
    const bmqt::MessageGUID&                  msgGUID,
    const bmqp::MessagePropertiesInfo&        mp,
    int                                       queueId,
    bmqt::CompressionAlgorithmType::Enum      cat,
    bool                                      isOutOfOrder,
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
    const bmqp::Protocol::MsgGroupId&         msgGroupId,
    bslma::Allocator*                         allocator)
: d_blob_sp(blob_sp)
, d_options_sp(0, allocator)
, d_clusterNode_p(0)
, d_guid(msgGUID)
, d_messagePropertiesInfo(mp)
, d_queueId(queueId)
, d_compressionAlgorithmType(cat)
, d_isOutOfOrder(isOutOfOrder)
, d_isRelay(false)
, d_subQueueInfos(subQueueInfos)
, d_msgGroupId(msgGroupId)
{
    // NOTHING
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherPushEvent::blob() const
{
    return d_blob_sp;
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherPushEvent::options() const
{
    return d_options_sp;
}

mqbnet::ClusterNode* DispatcherPushEvent::clusterNode() const
{
    return d_clusterNode_p;
}

bool DispatcherPushEvent::isRelay() const
{
    return d_isRelay;
}

int DispatcherPushEvent::queueId() const
{
    return d_queueId;
}

const bmqp::Protocol::SubQueueInfosArray&
DispatcherPushEvent::subQueueInfos() const
{
    return d_subQueueInfos;
}

const bmqp::MessagePropertiesInfo&
DispatcherPushEvent::messagePropertiesInfo() const
{
    return d_messagePropertiesInfo;
}

bmqt::CompressionAlgorithmType::Enum
DispatcherPushEvent::compressionAlgorithmType() const
{
    return d_compressionAlgorithmType;
}

bool DispatcherPushEvent::isOutOfOrderPush() const
{
    return d_isOutOfOrder;
}

const bmqt::MessageGUID& DispatcherPushEvent::guid() const
{
    return d_guid;
}

const bmqp::Protocol::MsgGroupId& DispatcherPushEvent::msgGroupId() const
{
    return d_msgGroupId;
}

DispatcherPushEvent& DispatcherPushEvent::setCompressionAlgorithmType(
    bmqt::CompressionAlgorithmType::Enum value)
{
    d_compressionAlgorithmType = value;
    return *this;
}

DispatcherPushEvent& DispatcherPushEvent::setOutOfOrderPush(bool value)
{
    d_isOutOfOrder = value;
    return *this;
}

DispatcherPushEvent&
DispatcherPushEvent::setMsgGroupId(const bmqp::Protocol::MsgGroupId& value)
{
    d_msgGroupId = value;
    return *this;
}

DispatcherPushEvent& DispatcherPushEvent::setQueueId(int value)
{
    d_queueId = value;
    return *this;
}

DispatcherPushEvent& DispatcherPushEvent::setSubQueueInfos(
    const bmqp::Protocol::SubQueueInfosArray& value)
{
    d_subQueueInfos = value;
    return *this;
}

DispatcherPushEvent& DispatcherPushEvent::setMessagePropertiesInfo(
    const bmqp::MessagePropertiesInfo& value)
{
    d_messagePropertiesInfo = value;

    return *this;
}

DispatcherPushEvent& DispatcherPushEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}

DispatcherPushEvent&
DispatcherPushEvent::setOptions(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_options_sp = value;
    return *this;
}

DispatcherPushEvent&
DispatcherPushEvent::setGuid(const bmqt::MessageGUID& value)
{
    d_guid = value;
    return *this;
}

DispatcherPushEvent&
DispatcherPushEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

DispatcherPushEvent&
DispatcherPushEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

// ------------------------
// class DispatcherPutEvent
// ------------------------

DispatcherPutEvent::DispatcherPutEvent(bslma::Allocator* allocator)
: d_blob_sp(0, allocator)
, d_options_sp(0, allocator)
, d_clusterNode_p(0)
, d_putHeader()
, d_queueHandle_p(0)
, d_partitionId(-1)
, d_isRelay(false)
, d_genCount(0)
, d_state()
{
}

DispatcherPutEvent::DispatcherPutEvent(
    bslmf::MovableRef<DispatcherPutEvent> other)
: d_blob_sp(bslmf::MovableRefUtil::move(other.d_blob_sp))
, d_options_sp(bslmf::MovableRefUtil::move(other.d_options_sp))
, d_clusterNode_p(other.d_clusterNode_p)
, d_putHeader(bslmf::MovableRefUtil::move(other.d_putHeader))
, d_queueHandle_p(other.d_queueHandle_p)
, d_partitionId(other.d_partitionId)
, d_isRelay(other.d_isRelay)
, d_genCount(other.d_genCount)
, d_state(bslmf::MovableRefUtil::move(other.d_state))
{
    // NOTHING
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherPutEvent::blob() const
{
    return d_blob_sp;
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherPutEvent::options() const
{
    return d_options_sp;
}

mqbnet::ClusterNode* DispatcherPutEvent::clusterNode() const
{
    return d_clusterNode_p;
}

bool DispatcherPutEvent::isRelay() const
{
    return d_isRelay;
}

int DispatcherPutEvent::partitionId() const
{
    return d_partitionId;
}

const bmqp::PutHeader& DispatcherPutEvent::putHeader() const
{
    return d_putHeader;
}

QueueHandle* DispatcherPutEvent::queueHandle() const
{
    return d_queueHandle_p;
}

bsls::Types::Uint64 DispatcherPutEvent::genCount() const
{
    return d_genCount;
}

const bsl::shared_ptr<bmqu::AtomicState>& DispatcherPutEvent::state() const
{
    return d_state;
}

DispatcherPutEvent&
DispatcherPutEvent::setPutHeader(const bmqp::PutHeader& value)
{
    d_putHeader = value;
    return *this;
}

DispatcherPutEvent& DispatcherPutEvent::setQueueHandle(QueueHandle* value)
{
    d_queueHandle_p = value;
    return *this;
}

DispatcherPutEvent& DispatcherPutEvent::setGenCount(unsigned int genCount)
{
    d_genCount = genCount;
    return *this;
}

DispatcherPutEvent&
DispatcherPutEvent::setState(const bsl::shared_ptr<bmqu::AtomicState>& state)
{
    d_state = state;
    return *this;
}

DispatcherPutEvent&
DispatcherPutEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

DispatcherPutEvent& DispatcherPutEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}

DispatcherPutEvent&
DispatcherPutEvent::setOptions(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_options_sp = value;
    return *this;
}

DispatcherPutEvent& DispatcherPutEvent::setPartitionId(int value)
{
    d_partitionId = value;
    return *this;
}

DispatcherPutEvent&
DispatcherPutEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

// ------------------------
// class DispatcherAckEvent
// ------------------------

DispatcherAckEvent::DispatcherAckEvent(bslma::Allocator* allocator)
: d_blob_sp(0, allocator)
, d_options_sp(0, allocator)
, d_clusterNode_p(0)
, d_ackMessage()
, d_isRelay(false)
{
    // NOTHING
}

DispatcherAckEvent::DispatcherAckEvent(
    bslmf::MovableRef<DispatcherAckEvent> other)
: d_blob_sp(bslmf::MovableRefUtil::move(other.d_blob_sp))
, d_options_sp(bslmf::MovableRefUtil::move(other.d_options_sp))
, d_clusterNode_p(other.d_clusterNode_p)
, d_ackMessage(bslmf::MovableRefUtil::move(other.d_ackMessage))
, d_isRelay(other.d_isRelay)
{
    // NOTHING
}

const bmqp::AckMessage& DispatcherAckEvent::ackMessage() const
{
    return d_ackMessage;
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherAckEvent::options() const
{
    return d_options_sp;
}

bool DispatcherAckEvent::isRelay() const
{
    return d_isRelay;
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherAckEvent::blob() const
{
    return d_blob_sp;
}

mqbnet::ClusterNode* DispatcherAckEvent::clusterNode() const
{
    return d_clusterNode_p;
}

bmqp::AckMessage& DispatcherAckEvent::ackMessage()
{
    return d_ackMessage;
}

DispatcherAckEvent&
DispatcherAckEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

DispatcherAckEvent& DispatcherAckEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}

DispatcherAckEvent&
DispatcherAckEvent::setOptions(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_options_sp = value;
    return *this;
}

DispatcherAckEvent&
DispatcherAckEvent::setAckMessage(const bmqp::AckMessage& value)
{
    d_ackMessage = value;
    return *this;
}

DispatcherAckEvent&
DispatcherAckEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

// ---------------------------------
// class DispatcherClusterStateEvent
// ---------------------------------

DispatcherClusterStateEvent::DispatcherClusterStateEvent(
    bslma::Allocator* allocator)
: d_blob_sp(0, allocator)
, d_clusterNode_p(0)
, d_isRelay(false)
{
    // NOTHING
}

DispatcherClusterStateEvent::DispatcherClusterStateEvent(
    bslmf::MovableRef<DispatcherClusterStateEvent> other)
: d_blob_sp(bslmf::MovableRefUtil::move(other.d_blob_sp))
, d_clusterNode_p(other.d_clusterNode_p)
, d_isRelay(other.d_isRelay)
{
    // NOTHING
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherClusterStateEvent::blob() const
{
    return d_blob_sp;
}

bool DispatcherClusterStateEvent::isRelay() const
{
    return d_isRelay;
}

mqbnet::ClusterNode* DispatcherClusterStateEvent::clusterNode() const
{
    return d_clusterNode_p;
}

DispatcherClusterStateEvent&
DispatcherClusterStateEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

DispatcherClusterStateEvent&
DispatcherClusterStateEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}

DispatcherClusterStateEvent&
DispatcherClusterStateEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

// ----------------------------
// class DispatcherStorageEvent
// ----------------------------

DispatcherStorageEvent::DispatcherStorageEvent(bslma::Allocator* allocator)
: d_blob_sp(0, allocator)
, d_clusterNode_p(0)
, d_isRelay(false)
{
    // NOTHING
}

DispatcherStorageEvent::DispatcherStorageEvent(
    bslmf::MovableRef<DispatcherStorageEvent> other)
: d_blob_sp(bslmf::MovableRefUtil::move(other.d_blob_sp))
, d_clusterNode_p(other.d_clusterNode_p)
, d_isRelay(other.d_isRelay)
{
    // NOTHING
}

bool DispatcherStorageEvent::isRelay() const
{
    return d_isRelay;
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherStorageEvent::blob() const
{
    return d_blob_sp;
}

mqbnet::ClusterNode* DispatcherStorageEvent::clusterNode() const
{
    return d_clusterNode_p;
}

DispatcherStorageEvent&
DispatcherStorageEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

DispatcherStorageEvent& DispatcherStorageEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}

DispatcherStorageEvent&
DispatcherStorageEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

// -----------------------------
// class DispatcherRecoveryEvent
// -----------------------------

DispatcherRecoveryEvent::DispatcherRecoveryEvent(bslma::Allocator* allocator)
: d_blob_sp(0, allocator)
, d_clusterNode_p(0)
, d_isRelay(false)
{
    // NOTHING
}
DispatcherRecoveryEvent::DispatcherRecoveryEvent(
    bslmf::MovableRef<DispatcherRecoveryEvent> other)
: d_blob_sp(bslmf::MovableRefUtil::move(other.d_blob_sp))
, d_clusterNode_p(other.d_clusterNode_p)
, d_isRelay(other.d_isRelay)
{
    // NOTHING
}
bool DispatcherRecoveryEvent::isRelay() const
{
    return d_isRelay;
}
const bsl::shared_ptr<bdlbb::Blob>& DispatcherRecoveryEvent::blob() const
{
    return d_blob_sp;
}
mqbnet::ClusterNode* DispatcherRecoveryEvent::clusterNode() const
{
    return d_clusterNode_p;
}
DispatcherRecoveryEvent&
DispatcherRecoveryEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}
DispatcherRecoveryEvent& DispatcherRecoveryEvent::setIsRelay(bool value)
{
    d_isRelay = value;
    return *this;
}
DispatcherRecoveryEvent&
DispatcherRecoveryEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

// ----------------------------
// class DispatcherReceiptEvent
// ----------------------------

DispatcherReceiptEvent::DispatcherReceiptEvent(bslma::Allocator* allocator)
: d_blob_sp(0, allocator)
, d_clusterNode_p(0)
{
    // NOTHING
}

DispatcherReceiptEvent::DispatcherReceiptEvent(
    bslmf::MovableRef<DispatcherReceiptEvent> other)
: d_blob_sp(bslmf::MovableRefUtil::move(other.d_blob_sp))
, d_clusterNode_p(other.d_clusterNode_p)
{
    // NOTHING
}

const bsl::shared_ptr<bdlbb::Blob>& DispatcherReceiptEvent::blob() const
{
    return d_blob_sp;
}

mqbnet::ClusterNode* DispatcherReceiptEvent::clusterNode() const
{
    return d_clusterNode_p;
}

DispatcherReceiptEvent&
DispatcherReceiptEvent::setBlob(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_blob_sp = value;
    return *this;
}

DispatcherReceiptEvent&
DispatcherReceiptEvent::setClusterNode(mqbnet::ClusterNode* value)
{
    d_clusterNode_p = value;
    return *this;
}

} // close package namespace
} // close enterprise namespace