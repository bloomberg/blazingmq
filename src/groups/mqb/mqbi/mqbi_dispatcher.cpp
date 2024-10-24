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
#include <bslim_printer.h>
#include <bsls_annotation.h>

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

// CLASS METHODS
Dispatcher::ProcessorFunctor
Dispatcher::voidToProcessorFunctor(const Dispatcher::VoidFunctor& functor)
{
    return bdlf::BindUtil::bind(functor);
}

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

    // bslim::Printer printer(&stream, level, spacesPerLevel);
    // printer.start();

    // // Print the type, source and destination (if any)
    // printer.printAttribute("type", type());
    // if (source()) {
    //     printer.printAttribute("source", source()->description());
    // }
    // if (destination()) {
    //     printer.printAttribute("destination", destination()->description());
    // }

    // switch (type()) {
    // case DispatcherEventType::e_UNDEFINED: {
    //     // Nothing more to print
    // } break;
    // case DispatcherEventType::e_DISPATCHER: {
    //     printer.printAttribute("hasFinalizeCallback",
    //                            (finalizeCallback() ? "yes" : "no"));
    // } break;
    // case DispatcherEventType::e_CALLBACK: {
    //     // Nothing more to print
    // } break;
    // case DispatcherEventType::e_CONTROL_MSG: {
    //     printer.printAttribute("controlMessage", d_controlMessage);
    // } break;
    // case DispatcherEventType::e_CONFIRM: {
    //     printer.printAttribute("blobLength",
    //                            (d_blob_sp ? d_blob_sp->length() : -1));
    //     printer.printAttribute("confirmMessage.queueId",
    //                            d_confirmMessage.queueId());
    //     printer.printAttribute("confirmMessage.subQueueId",
    //                            d_confirmMessage.subQueueId());
    //     printer.printAttribute("confirmMessage.guid",
    //                            d_confirmMessage.messageGUID());
    //     printer.printAttribute("partitionId", d_partitionId);
    //     printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    // } break;
    // case DispatcherEventType::e_REJECT: {
    //     printer.printAttribute("blobLength",
    //                            (d_blob_sp ? d_blob_sp->length() : -1));
    //     printer.printAttribute("rejectMessage.queueId",
    //                            d_rejectMessage.queueId());
    //     printer.printAttribute("rejectMessage.subqueueId",
    //                            d_rejectMessage.subQueueId());
    //     printer.printAttribute("rejectMessage.guid",
    //                            d_rejectMessage.messageGUID());
    //     printer.printAttribute("partitionId", d_partitionId);
    //     printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    // } break;
    // case DispatcherEventType::e_PUSH: {
    //     printer.printAttribute("blobLength",
    //                            (d_blob_sp ? d_blob_sp->length() : -1));
    //     printer.printAttribute("optionsLength",
    //                            (d_options_sp ? d_options_sp->length() :
    //                            -1));
    //     printer.printAttribute("guid", d_guid);
    //     printer.printAttribute("queueId", d_queueId);
    //     for (bmqp::Protocol::SubQueueInfosArray::size_type i = 0;
    //          i < d_subQueueInfos.size();
    //          ++i) {
    //         mwcu::MemOutStream out;
    //         out << "subQueueInfo[" << i << "]: ";
    //         printer.printAttribute(out.str().data(), d_subQueueInfos[i]);
    //     }
    //     printer.printAttribute("msGroupId", d_msgGroupId);
    //     printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    // } break;
    // case DispatcherEventType::e_PUT: {
    //     printer.printAttribute("blobLength",
    //                            (d_blob_sp ? d_blob_sp->length() : -1));
    //     printer.printAttribute("optionsLength",
    //                            (d_options_sp ? d_options_sp->length() :
    //                            -1));
    //     printer.printAttribute("partitionId", d_partitionId);
    //     printer.printAttribute("putHeader.queueId", d_putHeader.queueId());
    //     printer.printAttribute("putHeader.guid", d_putHeader.messageGUID());
    //     printer.printAttribute("putHeader.flags", d_putHeader.flags());
    //     printer.printAttribute("msGroupId", d_msgGroupId);
    //     printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    // } break;
    // case DispatcherEventType::e_ACK: {
    //     printer.printAttribute("ackMessage.status", d_ackMessage.status());
    //     printer.printAttribute("ackMessage.correlationId",
    //                            d_ackMessage.correlationId());
    //     printer.printAttribute("ackMessage.guid",
    //     d_ackMessage.messageGUID());
    //     printer.printAttribute("ackMessage.queueId",
    //     d_ackMessage.queueId()); printer.printAttribute("blobLength",
    //                            (d_blob_sp ? d_blob_sp->length() : -1));
    //     printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    // } break;
    // case DispatcherEventType::e_CLUSTER_STATE: {
    //     printer.printAttribute("blobLength",
    //                            (d_blob_sp ? d_blob_sp->length() : -1));
    // } break;
    // case DispatcherEventType::e_STORAGE: {
    //     printer.printAttribute("blobLength",
    //                            (d_blob_sp ? d_blob_sp->length() : -1));
    //     printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    // } break;
    // case DispatcherEventType::e_RECOVERY: {
    //     printer.printAttribute("blobLength",
    //                            (d_blob_sp ? d_blob_sp->length() : -1));
    //     printer.printAttribute("isRelay", (d_isRelay ? "true" : "false"));
    // } break;
    // case DispatcherEventType::e_REPLICATION_RECEIPT: {
    //     printer.printAttribute("blobLength",
    //                            (d_blob_sp ? d_blob_sp->length() : -1));
    //     // can't parse the blob and print the Receipt struct?
    // } break;
    // default: {
    //     BSLS_ASSERT_OPT(false && "Unknown DispatcherEventType");
    // }
    // }

    // printer.end();

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
