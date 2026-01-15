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

// BMQ
#include <bmqu_memoutstream.h>

// MQB
#include <mqbi_storage.h>

// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_ostream.h>
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

// -------------------------------
// class DispatcherDispatcherEvent
// -------------------------------

DispatcherDispatcherEvent::~DispatcherDispatcherEvent()
{
    // NOTHING (interface)
}

// -----------------------------
// class DispatcherCallbackEvent
// -----------------------------

DispatcherCallbackEvent::~DispatcherCallbackEvent()
{
    // NOTHING (interface)
}

// -----------------------------------
// class DispatcherControlMessageEvent
// -----------------------------------

DispatcherControlMessageEvent::~DispatcherControlMessageEvent()
{
    // NOTHING (interface)
}

// ----------------------------
// class DispatcherConfirmEvent
// ----------------------------

DispatcherConfirmEvent::~DispatcherConfirmEvent()
{
    // NOTHING (interface)
}

// ---------------------------
// class DispatcherRejectEvent
// ---------------------------

DispatcherRejectEvent::~DispatcherRejectEvent()
{
    // NOTHING (interface)
}

// -------------------------
// class DispatcherPushEvent
// -------------------------

DispatcherPushEvent::~DispatcherPushEvent()
{
    // NOTHING (interface)
}

// ------------------------
// class DispatcherPutEvent
// ------------------------

DispatcherPutEvent::~DispatcherPutEvent()
{
    // NOTHING (interface)
}

// ------------------------
// class DispatcherAckEvent
// ------------------------

DispatcherAckEvent::~DispatcherAckEvent()
{
    // NOTHING (interface)
}

// ---------------------------------
// class DispatcherClusterStateEvent
// ---------------------------------

DispatcherClusterStateEvent::~DispatcherClusterStateEvent()
{
    // NOTHING (interface)
}

// ----------------------------
// class DispatcherStorageEvent
// ----------------------------

DispatcherStorageEvent::~DispatcherStorageEvent()
{
    // NOTHING (interface)
}

// -----------------------------
// class DispatcherRecoveryEvent
// -----------------------------

DispatcherRecoveryEvent::~DispatcherRecoveryEvent()
{
    // NOTHING (interface)
}

// ----------------------------
// class DispatcherReceiptEvent
// ----------------------------

DispatcherReceiptEvent::~DispatcherReceiptEvent()
{
    // NOTHING (interface)
}

// ---------------------
// class DispatcherEvent
// ---------------------

DispatcherEvent::DispatcherEvent(bslma::Allocator* allocator)
: d_type(DispatcherEventType::e_UNDEFINED)
, d_source_p(0)
, d_destination_p(0)
, d_ackMessage()
, d_blob_sp(0, allocator)
, d_options_sp(0, allocator)
, d_clusterNode_p(0)
, d_confirmMessage()
, d_rejectMessage()
, d_controlMessage(allocator)
, d_guid(bmqt::MessageGUID())
, d_isRelay(false)
, d_partitionId(mqbi::Storage::k_INVALID_PARTITION_ID)
, d_putHeader()
, d_queueHandle_p(0)
, d_queueId(bmqp::QueueId::k_UNASSIGNED_QUEUE_ID)
, d_subQueueInfos(allocator)
, d_messagePropertiesInfo()
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_isOutOfOrder(false)
, d_genCount(0)
, d_callback(allocator)
, d_finalizeCallback(allocator)
{
    // NOTHING
}

DispatcherEvent::~DispatcherEvent()
{
    // NOTHING (interface)
}

void DispatcherEvent::reset()
{
    switch (d_type) {
    case mqbi::DispatcherEventType::e_UNDEFINED: {
        // NOTHING
    } break;
    case mqbi::DispatcherEventType::e_DISPATCHER: {
        if (!d_finalizeCallback.empty()) {
            // We only set finalizeCallback on e_DISPATCHER events

            // TODO(678098): make a special event type that handles this case
            d_finalizeCallback();
        }

        d_callback.reset();
        d_finalizeCallback.reset();
    } break;
    case mqbi::DispatcherEventType::e_CALLBACK: {
        d_callback.reset();
    } break;
    case mqbi::DispatcherEventType::e_CONTROL_MSG: {
        d_controlMessage.reset();
    } break;
    case mqbi::DispatcherEventType::e_CONFIRM: {
        d_blob_sp.reset();
        d_clusterNode_p  = 0;
        d_confirmMessage = bmqp::ConfirmMessage();
        d_isRelay        = false;
    } break;
    case mqbi::DispatcherEventType::e_REJECT: {
        d_blob_sp.reset();
        d_clusterNode_p = 0;
        d_rejectMessage = bmqp::RejectMessage();
        d_isRelay       = false;
        d_partitionId   = mqbi::Storage::k_INVALID_PARTITION_ID;
    } break;
    case mqbi::DispatcherEventType::e_PUSH: {
        d_blob_sp.reset();
        d_options_sp.reset();
        d_clusterNode_p = 0;
        d_guid          = bmqt::MessageGUID();
        d_isRelay       = false;
        d_queueId       = bmqp::QueueId::k_UNASSIGNED_QUEUE_ID;
        d_subQueueInfos.clear();
        d_messagePropertiesInfo    = bmqp::MessagePropertiesInfo();
        d_compressionAlgorithmType = bmqt::CompressionAlgorithmType::e_NONE;
        d_isOutOfOrder             = false;
    } break;
    case mqbi::DispatcherEventType::e_PUT: {
        d_blob_sp.reset();
        d_options_sp.reset();
        d_clusterNode_p = 0;
        d_isRelay       = false;
        d_putHeader     = bmqp::PutHeader();
        d_queueHandle_p = 0;
        d_genCount      = 0;
        d_state.reset();
    } break;
    case mqbi::DispatcherEventType::e_ACK: {
        d_ackMessage = bmqp::AckMessage();
        d_blob_sp.reset();
        d_options_sp.reset();
        d_clusterNode_p = 0;
        d_isRelay       = false;
    } break;
    case mqbi::DispatcherEventType::e_CLUSTER_STATE: {
        d_blob_sp.reset();
        d_clusterNode_p = 0;
    } break;
    case mqbi::DispatcherEventType::e_STORAGE: {
        d_blob_sp.reset();
        d_clusterNode_p = 0;
        d_isRelay       = false;
    } break;
    case mqbi::DispatcherEventType::e_RECOVERY: {
        d_blob_sp.reset();
        d_clusterNode_p = 0;
        d_isRelay       = false;
    } break;
    case mqbi::DispatcherEventType::e_REPLICATION_RECEIPT: {
        d_blob_sp.reset();
        d_clusterNode_p = 0;
    } break;
    default: {
        BSLS_ASSERT_OPT(false && "Unexpected event type");
    } break;
    }

    d_type          = DispatcherEventType::e_UNDEFINED;
    d_source_p      = 0;
    d_destination_p = 0;
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

    switch (type()) {
    case DispatcherEventType::e_UNDEFINED: {
        // Nothing more to print
    } break;
    case DispatcherEventType::e_DISPATCHER: {
        printer.printAttribute("hasFinalizeCallback",
                               (finalizeCallback().empty() ? "no" : "yes"));
    } break;
    case DispatcherEventType::e_CALLBACK: {
        // Nothing more to print
    } break;
    case DispatcherEventType::e_CONTROL_MSG: {
        printer.printAttribute("controlMessage", d_controlMessage);
    } break;
    case DispatcherEventType::e_CONFIRM: {
        printer.printAttribute("blobLength",
                               (d_blob_sp ? d_blob_sp->length() : -1));
        printer.printAttribute("confirmMessage.queueId",
                               d_confirmMessage.queueId());
        printer.printAttribute("confirmMessage.subQueueId",
                               d_confirmMessage.subQueueId());
        printer.printAttribute("confirmMessage.guid",
                               d_confirmMessage.messageGUID());
        printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    } break;
    case DispatcherEventType::e_REJECT: {
        printer.printAttribute("blobLength",
                               (d_blob_sp ? d_blob_sp->length() : -1));
        printer.printAttribute("rejectMessage.queueId",
                               d_rejectMessage.queueId());
        printer.printAttribute("rejectMessage.subqueueId",
                               d_rejectMessage.subQueueId());
        printer.printAttribute("rejectMessage.guid",
                               d_rejectMessage.messageGUID());
        printer.printAttribute("partitionId", d_partitionId);
        printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    } break;
    case DispatcherEventType::e_PUSH: {
        printer.printAttribute("blobLength",
                               (d_blob_sp ? d_blob_sp->length() : -1));
        printer.printAttribute("optionsLength",
                               (d_options_sp ? d_options_sp->length() : -1));
        printer.printAttribute("guid", d_guid);
        printer.printAttribute("queueId", d_queueId);
        for (bmqp::Protocol::SubQueueInfosArray::size_type i = 0;
             i < d_subQueueInfos.size();
             ++i) {
            bmqu::MemOutStream out;
            out << "subQueueInfo[" << i << "]: ";
            printer.printAttribute(out.str().data(), d_subQueueInfos[i]);
        }
        printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    } break;
    case DispatcherEventType::e_PUT: {
        printer.printAttribute("blobLength",
                               (d_blob_sp ? d_blob_sp->length() : -1));
        printer.printAttribute("optionsLength",
                               (d_options_sp ? d_options_sp->length() : -1));
        printer.printAttribute("putHeader.queueId", d_putHeader.queueId());
        printer.printAttribute("putHeader.guid", d_putHeader.messageGUID());
        printer.printAttribute("putHeader.flags", d_putHeader.flags());
        printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    } break;
    case DispatcherEventType::e_ACK: {
        printer.printAttribute("ackMessage.status", d_ackMessage.status());
        printer.printAttribute("ackMessage.correlationId",
                               d_ackMessage.correlationId());
        printer.printAttribute("ackMessage.guid", d_ackMessage.messageGUID());
        printer.printAttribute("ackMessage.queueId", d_ackMessage.queueId());
        printer.printAttribute("blobLength",
                               (d_blob_sp ? d_blob_sp->length() : -1));
        printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    } break;
    case DispatcherEventType::e_CLUSTER_STATE: {
        printer.printAttribute("blobLength",
                               (d_blob_sp ? d_blob_sp->length() : -1));
    } break;
    case DispatcherEventType::e_STORAGE: {
        printer.printAttribute("blobLength",
                               (d_blob_sp ? d_blob_sp->length() : -1));
        printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    } break;
    case DispatcherEventType::e_RECOVERY: {
        printer.printAttribute("blobLength",
                               (d_blob_sp ? d_blob_sp->length() : -1));
        printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    } break;
    case DispatcherEventType::e_REPLICATION_RECEIPT: {
        printer.printAttribute("blobLength",
                               (d_blob_sp ? d_blob_sp->length() : -1));
        // can't parse the blob and print the Receipt struct?
    } break;
    default: {
        BSLS_ASSERT_OPT(false && "Unknown DispatcherEventType");
    }
    }

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

const bslmt::ThreadUtil::Id DispatcherClient::k_ANY_THREAD_ID = 0;

DispatcherClient::~DispatcherClient()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
