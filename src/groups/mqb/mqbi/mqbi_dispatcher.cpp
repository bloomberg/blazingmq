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

}  // close package namespace
}  // close enterprise namespace
