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

// ---------------------
// class DispatcherEvent
// ---------------------

DispatcherEvent::DispatcherEvent(bslma::Allocator* /* allocator */)
: d_destination_p(0)
{
    // NOTHING
}

DispatcherEvent::~DispatcherEvent()
{
    // NOTHING
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
