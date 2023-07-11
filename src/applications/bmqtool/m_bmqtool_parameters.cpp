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

// m_bmqtool_parameters.cpp                                           -*-C++-*-
#include <m_bmqtool_parameters.h>

// BMQ
#include <bmqt_queueflags.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlb_string.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace m_bmqtool {

// ==========================
// struct ParametersVerbosity
// ==========================

bsl::ostream& ParametersVerbosity::print(bsl::ostream&              stream,
                                         ParametersVerbosity::Value value,
                                         int                        level,
                                         int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printValue(ParametersVerbosity::toAscii(value));
    printer.end();

    return stream;
}

const char* ParametersVerbosity::toAscii(ParametersVerbosity::Value value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SILENT)
        CASE(TRACE)
        CASE(DEBUG)
        CASE(INFO)
        CASE(WARNING)
        CASE(ERROR)
        CASE(FATAL)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ParametersVerbosity::fromAscii(ParametersVerbosity::Value* out,
                                    const bslstl::StringRef&    str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(ParametersVerbosity::e_##M),   \
                                       str)) {                                \
        *out = ParametersVerbosity::e_##M;                                    \
        return true;                                                          \
    }

    CHECKVALUE(SILENT);
    CHECKVALUE(TRACE);
    CHECKVALUE(DEBUG);
    CHECKVALUE(INFO);
    CHECKVALUE(WARNING);
    CHECKVALUE(ERROR);
    CHECKVALUE(FATAL);

    // Invalid string
    return false;

#undef CHECKVALUE
}

bool ParametersVerbosity::isValid(const bsl::string* str, bsl::ostream& stream)
{
    ParametersVerbosity::Value value;
    if (fromAscii(&value, *str) == true) {
        return true;  // RETURN
    }

    stream << "Error: verbosity parameter must be one of "
           << "[silent, trace, debug, info, warning, error or fatal]\n";
    return false;
}

// =====================
// struct ParametersMode
// =====================

bsl::ostream& ParametersMode::print(bsl::ostream&         stream,
                                    ParametersMode::Value value,
                                    int                   level,
                                    int                   spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printValue(ParametersMode::toAscii(value));
    printer.end();

    return stream;
}

const char* ParametersMode::toAscii(ParametersMode::Value value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(CLI)
        CASE(AUTO)
        CASE(STORAGE)
        CASE(SYSCHK);
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ParametersMode::fromAscii(ParametersMode::Value*   out,
                               const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(ParametersMode::e_##M),        \
                                       str)) {                                \
        *out = ParametersMode::e_##M;                                         \
        return true;                                                          \
    }

    CHECKVALUE(CLI);
    CHECKVALUE(AUTO);
    CHECKVALUE(STORAGE);
    CHECKVALUE(SYSCHK);

    // Invalid string
    return false;

#undef CHECKVALUE
}

bool ParametersMode::isValid(const bsl::string* str, bsl::ostream& stream)
{
    ParametersMode::Value value;
    if (fromAscii(&value, *str) == true) {
        return true;  // RETURN
    }

    stream << "Error: mode parameter must be one of [cli, auto, syschk]\n";
    return false;
}

// ========================
// struct ParametersLatency
// ========================

bsl::ostream& ParametersLatency::print(bsl::ostream&            stream,
                                       ParametersLatency::Value value,
                                       int                      level,
                                       int                      spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printValue(ParametersLatency::toAscii(value));
    printer.end();

    return stream;
}

const char* ParametersLatency::toAscii(ParametersLatency::Value value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NONE)
        CASE(HIRES)
        CASE(EPOCH)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ParametersLatency::fromAscii(ParametersLatency::Value* out,
                                  const bslstl::StringRef&  str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(ParametersLatency::e_##M),     \
                                       str)) {                                \
        *out = ParametersLatency::e_##M;                                      \
        return true;                                                          \
    }

    CHECKVALUE(NONE);
    CHECKVALUE(HIRES);
    CHECKVALUE(EPOCH);

    // Invalid string
    return false;

#undef CHECKVALUE
}

bool ParametersLatency::isValid(const bsl::string* str, bsl::ostream& stream)
{
    ParametersLatency::Value value;
    if (fromAscii(&value, *str) == true) {
        return true;  // RETURN
    }

    stream << "Error: latency parameter must be one of [none, hires or epoch]"
           << bsl::endl;
    return false;
}

// ================
// class Parameters
// ================

Parameters::Parameters(bslma::Allocator* allocator)
: d_broker(allocator)
, d_queueUri(allocator)
, d_qlistFilePath(allocator)
, d_journalFilePath(allocator)
, d_dataFilePath(allocator)
, d_latencyReportPath(allocator)
, d_logFilePath(allocator)
, d_messageProperties(allocator)
, d_subscriptions(allocator)
{
    CommandLineParameters params(allocator);
    const bool            rc = from(bsl::cerr, params);
    BSLS_ASSERT_OPT(rc);
}

bsl::ostream&
Parameters::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printForeign(mode(), &ParametersMode::print, "mode");
    printer.printAttribute("broker", broker());
    printer.printAttribute("queueUri", queueUri());
    printer.printAttribute("qlistFilePath", qlistFilePath());
    printer.printAttribute("journalFilePath", journalFilePath());
    printer.printAttribute("dataFilePath", dataFilePath());
    mwcu::MemOutStream flagsOs;
    bmqt::QueueFlagsUtil::prettyPrint(flagsOs, d_queueFlags);
    flagsOs << " (" << d_queueFlags << ")" << bsl::ends;
    printer.printAttribute("queueFlags", flagsOs.str());
    printer.printForeign(latency(), &ParametersLatency::print, "latency");
    printer.printAttribute("latencyReportPath", d_latencyReportPath);
    printer.printAttribute("logFilePath", logFilePath());
    printer.printAttribute("dumpMsg", dumpMsg());
    printer.printAttribute("confirmMsg", confirmMsg());
    printer.printAttribute("eventSize", eventSize());
    printer.printAttribute("msgSize", msgSize());
    printer.printAttribute("postRate", postRate());
    printer.printAttribute("postInterval", postInterval());
    printer.printAttribute("eventsCount", eventsCount());
    printer.printAttribute("maxUnconfirmedMsgs", maxUnconfirmedMsgs());
    printer.printAttribute("maxUnconfirmedBytes", maxUnconfirmedBytes());
    printer.printForeign(verbosity(),
                         &ParametersVerbosity::print,
                         "verbosity");
    printer.printAttribute("logFormat", logFormat());
    printer.printAttribute("memoryDebug", memoryDebug());
    printer.printAttribute("numProcessingThreads", numProcessingThreads());
    printer.printAttribute("shutdownGrace", shutdownGrace());
    printer.printAttribute("noSessionEventHandler", noSessionEventHandler());
    printer.printAttribute("sequentialMessagePattern",
                           d_sequentialMessagePattern);
    printer.printAttribute("messageProperties", d_messageProperties);
    printer.printAttribute("subscriptions", d_subscriptions);
    printer.end();

    return stream;
}

bool Parameters::from(bsl::ostream&                stream,
                      const CommandLineParameters& params)
{
    // Convert 'string' to 'ParametersXXX::Value'
    ParametersVerbosity::Value paramVerbosity;
    ParametersMode::Value      paramMode;
    ParametersLatency::Value   paramLatency;

    // No need to verify the values... optParse took care of validating them
    ParametersVerbosity::fromAscii(&paramVerbosity, params.verbosity());
    ParametersMode::fromAscii(&paramMode, params.mode());
    ParametersLatency::fromAscii(&paramLatency, params.latency());

    // Convert queueFlags string to int, and validate it
    bsls::Types::Uint64 flags;
    mwcu::MemOutStream  errStream;
    if (bmqt::QueueFlagsUtil::fromString(errStream,
                                         &flags,
                                         params.queueFlags()) != 0) {
        stream << "Invalid parameter queueFlags '" << params.queueFlags()
               << "': " << errStream.str() << "\n";
        return false;  // RETURN
    }

    // Extract out the two parameters from maxUnconfirmed string
    int maxUnconfirmedMsgs  = 0;
    int maxUnconfirmedBytes = 0;
    if (sscanf(params.maxUnconfirmed().c_str(),
               "%d:%d",
               &maxUnconfirmedMsgs,
               &maxUnconfirmedBytes) != 2) {
        stream << "Invalid maxUnconfirmed parameter "
               << "'" << params.maxUnconfirmed() << "'"
               << "\n";
        return false;  // RETURN
    }

    // Calculate eventsCount based on eventsCountStr
    bsl::istringstream is(params.eventsCount());
    int                eventsCount;
    is >> eventsCount;
    if (is.fail()) {
        stream << "Invalid eventsCount parameter "
               << "'" << params.eventsCount() << "'"
               << ": Not a number"
               << "\n";
        return false;  // RETURN
    }
    if (!is.eof()) {
        char unit;
        is >> unit;
        is.get();
        if (!is.eof() || tolower(unit) != 's') {
            stream << "Invalid eventsCount parameter "
                   << "'" << params.eventsCount() << "'"
                   << ": Unit must be 's' or 'S'"
                   << "\n";
            return false;  // RETURN
        }
        const int eventsDurationMs =
            eventsCount * bdlt::TimeUnitRatio::k_MILLISECONDS_PER_SECOND;
        const int numPosts = eventsDurationMs / params.postInterval();
        eventsCount        = params.postRate() * numPosts;
    }

    // Populate output parameters struct
    setVerbosity(paramVerbosity);
    setLogFormat(params.logFormat());
    setMode(paramMode);
    setBroker(params.broker());
    setQueueUri(params.queueUri());
    setQueueFlags(flags);
    setMemoryDebug(params.memoryDebug());
    setNumProcessingThreads(params.threads());
    setNoSessionEventHandler(params.noSessionEventHandler());
    setLatency(paramLatency);
    setLatencyReportPath(params.latencyReport());
    setDumpMsg(params.dumpMsg());
    setConfirmMsg(params.confirmMsg());
    setMsgSize(params.msgSize());
    setPostInterval(params.postInterval());
    setPostRate(params.postRate());
    setEventsCount(eventsCount);
    setEventSize(params.eventSize());
    setMaxUnconfirmedMsgs(maxUnconfirmedMsgs);
    setMaxUnconfirmedBytes(maxUnconfirmedBytes);
    setStoragePath(params.storage());
    setLogFilePath(params.log());
    setSequentialMessagePattern(params.sequentialMessagePattern());
    setShutdownGrace(params.shutdownGrace());
    setMessageProperties(params.messageProperties());
    setSubscriptions(params.subscriptions());

    return true;
}

void Parameters::dump(bsl::ostream& stream)
{
    print(stream, 0, -1);
}

bool Parameters::validate(bsl::string* error)
{
    mwcu::MemOutStream ss;

    if (d_queueFlags == 0 && d_mode != ParametersMode::e_CLI &&
        d_mode != ParametersMode::e_STORAGE &&
        d_mode != ParametersMode::e_SYSCHK) {
        ss << "QueueFlags must be specified if not in interactive, storage or "
           << "syschk mode\n";
    }
    if (d_queueUri.empty() && d_mode != ParametersMode::e_CLI &&
        d_mode != ParametersMode::e_STORAGE &&
        d_mode != ParametersMode::e_SYSCHK) {
        ss << "QueueURI must be specified if not in interactive, storage or "
           << "syschk mode\n";
    }
    if (d_noSessionEventHandler && d_mode != ParametersMode::e_CLI &&
        d_mode != ParametersMode::e_STORAGE) {
        ss << "NoSessionEventHandler is only to use in interactive or storage "
           << "mode\n";
    }

    error->assign(ss.str().data(), ss.str().length());
    return error->empty();
}

}  // close package namespace
}  // close enterprise namespace
