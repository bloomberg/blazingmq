// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbsi_log.cpp                                                      -*-C++-*-
#include <mqbsi_log.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace mqbsi {

// ----------------
// class LogFactory
// ----------------

LogFactory::~LogFactory()
{
    // NOTHING
}

// ------------------
// struct LogOpResult
// ------------------

bsl::ostream& LogOpResult::print(bsl::ostream&     stream,
                                 LogOpResult::Enum value,
                                 int               level,
                                 int               spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << LogOpResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* LogOpResult::toAscii(LogOpResult::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SUCCESS)
        CASE(UNKNOWN)
        CASE(UNSUPPORTED_OPERATION)
        CASE(LOG_ALREADY_OPENED)
        CASE(LOG_ALREADY_CLOSED)
        CASE(LOG_READONLY)
        CASE(FILE_NOT_EXIST)
        CASE(FILE_OPEN_FAILURE)
        CASE(FILE_CLOSE_FAILURE)
        CASE(FILE_GROW_FAILURE)
        CASE(FILE_TRUNCATE_FAILURE)
        CASE(FILE_SEEK_FAILURE)
        CASE(FILE_FLUSH_FAILURE)
        CASE(FILE_MSYNC_FAILURE)
        CASE(OFFSET_OUT_OF_RANGE)
        CASE(REACHED_END_OF_LOG)
        CASE(REACHED_END_OF_RECORD)
        CASE(INVALID_BLOB_SECTION)
        CASE(BYTE_READ_FAILURE)
        CASE(BYTE_WRITE_FAILURE)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool LogOpResult::fromAscii(LogOpResult::Enum*       out,
                            const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(LogOpResult::e_##M),           \
                                       str.data(),                            \
                                       static_cast<int>(str.length()))) {     \
        *out = LogOpResult::e_##M;                                            \
        return true;                                                          \
    }

    CHECKVALUE(SUCCESS)
    CHECKVALUE(UNKNOWN)
    CHECKVALUE(UNSUPPORTED_OPERATION)
    CHECKVALUE(LOG_ALREADY_OPENED)
    CHECKVALUE(LOG_ALREADY_CLOSED)
    CHECKVALUE(LOG_READONLY)
    CHECKVALUE(FILE_NOT_EXIST)
    CHECKVALUE(FILE_OPEN_FAILURE)
    CHECKVALUE(FILE_CLOSE_FAILURE)
    CHECKVALUE(FILE_GROW_FAILURE)
    CHECKVALUE(FILE_TRUNCATE_FAILURE)
    CHECKVALUE(FILE_SEEK_FAILURE)
    CHECKVALUE(FILE_FLUSH_FAILURE)
    CHECKVALUE(FILE_MSYNC_FAILURE)
    CHECKVALUE(OFFSET_OUT_OF_RANGE)
    CHECKVALUE(REACHED_END_OF_LOG)
    CHECKVALUE(REACHED_END_OF_RECORD)
    CHECKVALUE(INVALID_BLOB_SECTION)
    CHECKVALUE(BYTE_READ_FAILURE)
    CHECKVALUE(BYTE_WRITE_FAILURE)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ---------------
// class LogConfig
// ---------------

bsl::ostream&
LogConfig::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("maxSize", maxSize());
    printer.printAttribute("logId", logId());
    if (!location().empty()) {
        // Fields relevant only to on-disk log were specified
        printer.printAttribute("location", location());
        printer.printAttribute("reserveOnDisk", reserveOnDisk());
        printer.printAttribute("prefaultPages", prefaultPages());
    }
    printer.end();

    return stream;
}

// --------------------
// class LogIdGenerator
// --------------------

LogIdGenerator::~LogIdGenerator()
{
    // NOTHING
}

// ---------
// class Log
// ---------

Log::~Log()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
