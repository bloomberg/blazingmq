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

// mqbi_storage.cpp                                                   -*-C++-*-
#include <mqbi_storage.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bsl_ostream.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace mqbi {

// --------------------
// struct StorageResult
// --------------------

bsl::ostream& StorageResult::print(bsl::ostream&       stream,
                                   StorageResult::Enum value,
                                   int                 level,
                                   int                 spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << StorageResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* StorageResult::toAscii(StorageResult::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SUCCESS)
        CASE(INVALID_OPERATION)
        CASE(GUID_NOT_UNIQUE)
        CASE(GUID_NOT_FOUND)
        CASE(LIMIT_MESSAGES)
        CASE(LIMIT_BYTES)
        CASE(ZERO_REFERENCES)
        CASE(NON_ZERO_REFERENCES)
        CASE(WRITE_FAILURE)
        CASE(APPKEY_NOT_FOUND)
        CASE(DUPLICATE)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool StorageResult::fromAscii(StorageResult::Enum*     out,
                              const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(StorageResult::e_##M),         \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = StorageResult::e_##M;                                          \
        return true;                                                          \
    }

    CHECKVALUE(SUCCESS)
    CHECKVALUE(INVALID_OPERATION)
    CHECKVALUE(GUID_NOT_UNIQUE)
    CHECKVALUE(GUID_NOT_FOUND)
    CHECKVALUE(LIMIT_MESSAGES)
    CHECKVALUE(LIMIT_BYTES)
    CHECKVALUE(ZERO_REFERENCES)
    CHECKVALUE(NON_ZERO_REFERENCES)
    CHECKVALUE(WRITE_FAILURE)
    CHECKVALUE(APPKEY_NOT_FOUND)
    CHECKVALUE(DUPLICATE)

    // Invalid string
    return false;

#undef CHECKVALUE
}

bmqt::AckResult::Enum StorageResult::toAckResult(StorageResult::Enum value)
{
#define CASE(X, Y)                                                            \
    case e_##X: return bmqt::AckResult::e_##Y;

    switch (value) {
        CASE(SUCCESS, SUCCESS)
        CASE(INVALID_OPERATION, UNKNOWN)
        CASE(GUID_NOT_UNIQUE, UNKNOWN)
        CASE(GUID_NOT_FOUND, UNKNOWN)
        CASE(ZERO_REFERENCES, UNKNOWN)
        CASE(NON_ZERO_REFERENCES, UNKNOWN)
        CASE(APPKEY_NOT_FOUND, UNKNOWN)
        CASE(WRITE_FAILURE, STORAGE_FAILURE)
        CASE(LIMIT_MESSAGES, LIMIT_MESSAGES)
        CASE(LIMIT_BYTES, LIMIT_BYTES)
        CASE(DUPLICATE, SUCCESS)
    default: return bmqt::AckResult::e_UNKNOWN;
    }

#undef CASE
}

// ------------------------------
// class StorageMessageAttributes
// ------------------------------

bsl::ostream&
StorageMessageAttributes::print(bsl::ostream&                   stream,
                                const StorageMessageAttributes& value,
                                int                             level,
                                int                             spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();

    printer.printAttribute("arrivalTimestamp", value.arrivalTimestamp());
    printer.printAttribute("arrivalTimepoint", value.arrivalTimepoint());
    printer.printAttribute("refCount", value.refCount());
    printer.printAttribute("hasMessageProperties",
                           value.messagePropertiesInfo().isPresent());
    printer.printAttribute("crc32c", value.crc32c());

    printer.end();

    return stream;
}

// ---------------------
// class StorageIterator
// ---------------------

StorageIterator::~StorageIterator()
{
    // NOTHING
}

// -------------
// class Storage
// -------------

Storage::~Storage()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
