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

// bmqt_resultcode.cpp                                                -*-C++-*-
#include <bmqt_resultcode.h>

#include <bmqscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bsl_ostream.h>

namespace BloombergLP {
namespace bmqt {

// --------------------
// struct GenericResult
// --------------------

bsl::ostream& GenericResult::print(bsl::ostream&       stream,
                                   GenericResult::Enum value,
                                   int                 level,
                                   int                 spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << GenericResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* GenericResult::toAscii(GenericResult::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(SUCCESS)
        BMQT_CASE(UNKNOWN)
        BMQT_CASE(TIMEOUT)
        BMQT_CASE(NOT_CONNECTED)
        BMQT_CASE(CANCELED)
        BMQT_CASE(NOT_SUPPORTED)
        BMQT_CASE(REFUSED)
        BMQT_CASE(INVALID_ARGUMENT)
        BMQT_CASE(NOT_READY)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool GenericResult::fromAscii(GenericResult::Enum*     out,
                              const bslstl::StringRef& str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(GenericResult::e_##M),         \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = GenericResult::e_##M;                                          \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(SUCCESS)
    BMQT_CHECKVALUE(UNKNOWN)
    BMQT_CHECKVALUE(TIMEOUT)
    BMQT_CHECKVALUE(NOT_CONNECTED)
    BMQT_CHECKVALUE(CANCELED)
    BMQT_CHECKVALUE(NOT_SUPPORTED)
    BMQT_CHECKVALUE(REFUSED)
    BMQT_CHECKVALUE(INVALID_ARGUMENT)
    BMQT_CHECKVALUE(NOT_READY)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

// ----------------------
// struct OpenQueueResult
// ----------------------

bsl::ostream& OpenQueueResult::print(bsl::ostream&         stream,
                                     OpenQueueResult::Enum value,
                                     int                   level,
                                     int                   spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << OpenQueueResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* OpenQueueResult::toAscii(OpenQueueResult::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(SUCCESS)
        BMQT_CASE(UNKNOWN)
        BMQT_CASE(TIMEOUT)
        BMQT_CASE(NOT_CONNECTED)
        BMQT_CASE(CANCELED)
        BMQT_CASE(NOT_SUPPORTED)
        BMQT_CASE(REFUSED)
        BMQT_CASE(INVALID_ARGUMENT)
        BMQT_CASE(NOT_READY)
        BMQT_CASE(ALREADY_OPENED)
        BMQT_CASE(ALREADY_IN_PROGRESS)
        BMQT_CASE(INVALID_URI)
        BMQT_CASE(INVALID_FLAGS)
        BMQT_CASE(CORRELATIONID_NOT_UNIQUE)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool OpenQueueResult::fromAscii(OpenQueueResult::Enum*   out,
                                const bslstl::StringRef& str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(OpenQueueResult::e_##M),       \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = OpenQueueResult::e_##M;                                        \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(SUCCESS)
    BMQT_CHECKVALUE(UNKNOWN)
    BMQT_CHECKVALUE(TIMEOUT)
    BMQT_CHECKVALUE(NOT_CONNECTED)
    BMQT_CHECKVALUE(CANCELED)
    BMQT_CHECKVALUE(NOT_SUPPORTED)
    BMQT_CHECKVALUE(REFUSED)
    BMQT_CHECKVALUE(INVALID_ARGUMENT)
    BMQT_CHECKVALUE(NOT_READY)
    BMQT_CHECKVALUE(ALREADY_OPENED)
    BMQT_CHECKVALUE(ALREADY_IN_PROGRESS)
    BMQT_CHECKVALUE(INVALID_URI)
    BMQT_CHECKVALUE(INVALID_FLAGS)
    BMQT_CHECKVALUE(CORRELATIONID_NOT_UNIQUE)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

// ---------------------------
// struct ConfigureQueueResult
// ---------------------------

bsl::ostream& ConfigureQueueResult::print(bsl::ostream&              stream,
                                          ConfigureQueueResult::Enum value,
                                          int                        level,
                                          int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ConfigureQueueResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ConfigureQueueResult::toAscii(ConfigureQueueResult::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(SUCCESS)
        BMQT_CASE(UNKNOWN)
        BMQT_CASE(TIMEOUT)
        BMQT_CASE(NOT_CONNECTED)
        BMQT_CASE(CANCELED)
        BMQT_CASE(NOT_SUPPORTED)
        BMQT_CASE(REFUSED)
        BMQT_CASE(INVALID_ARGUMENT)
        BMQT_CASE(NOT_READY)
        BMQT_CASE(ALREADY_IN_PROGRESS)
        BMQT_CASE(INVALID_QUEUE)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool ConfigureQueueResult::fromAscii(ConfigureQueueResult::Enum* out,
                                     const bslstl::StringRef&    str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(ConfigureQueueResult::e_##M),  \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = ConfigureQueueResult::e_##M;                                   \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(SUCCESS)
    BMQT_CHECKVALUE(UNKNOWN)
    BMQT_CHECKVALUE(TIMEOUT)
    BMQT_CHECKVALUE(NOT_CONNECTED)
    BMQT_CHECKVALUE(CANCELED)
    BMQT_CHECKVALUE(NOT_SUPPORTED)
    BMQT_CHECKVALUE(REFUSED)
    BMQT_CHECKVALUE(INVALID_ARGUMENT)
    BMQT_CHECKVALUE(NOT_READY)
    BMQT_CHECKVALUE(ALREADY_IN_PROGRESS)
    BMQT_CHECKVALUE(INVALID_QUEUE)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

// -----------------------
// struct CloseQueueResult
// -----------------------

bsl::ostream& CloseQueueResult::print(bsl::ostream&          stream,
                                      CloseQueueResult::Enum value,
                                      int                    level,
                                      int                    spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << CloseQueueResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* CloseQueueResult::toAscii(CloseQueueResult::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(SUCCESS)
        BMQT_CASE(UNKNOWN)
        BMQT_CASE(TIMEOUT)
        BMQT_CASE(NOT_CONNECTED)
        BMQT_CASE(CANCELED)
        BMQT_CASE(NOT_SUPPORTED)
        BMQT_CASE(REFUSED)
        BMQT_CASE(INVALID_ARGUMENT)
        BMQT_CASE(NOT_READY)
        BMQT_CASE(ALREADY_CLOSED)
        BMQT_CASE(ALREADY_IN_PROGRESS)
        BMQT_CASE(UNKNOWN_QUEUE)
        BMQT_CASE(INVALID_QUEUE)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool CloseQueueResult::fromAscii(CloseQueueResult::Enum*  out,
                                 const bslstl::StringRef& str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(CloseQueueResult::e_##M),      \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = CloseQueueResult::e_##M;                                       \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(SUCCESS)
    BMQT_CHECKVALUE(UNKNOWN)
    BMQT_CHECKVALUE(TIMEOUT)
    BMQT_CHECKVALUE(NOT_CONNECTED)
    BMQT_CHECKVALUE(CANCELED)
    BMQT_CHECKVALUE(NOT_SUPPORTED)
    BMQT_CHECKVALUE(REFUSED)
    BMQT_CHECKVALUE(INVALID_ARGUMENT)
    BMQT_CHECKVALUE(NOT_READY)
    BMQT_CHECKVALUE(ALREADY_CLOSED)
    BMQT_CHECKVALUE(ALREADY_IN_PROGRESS)
    BMQT_CHECKVALUE(UNKNOWN_QUEUE)
    BMQT_CHECKVALUE(INVALID_QUEUE)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

// -------------------------
// struct EventBuilderResult
// -------------------------

bsl::ostream& EventBuilderResult::print(bsl::ostream&            stream,
                                        EventBuilderResult::Enum value,
                                        int                      level,
                                        int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << EventBuilderResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* EventBuilderResult::toAscii(EventBuilderResult::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(SUCCESS)
        BMQT_CASE(UNKNOWN)
        BMQT_CASE(QUEUE_INVALID)
        BMQT_CASE(QUEUE_READONLY)
        BMQT_CASE(MISSING_CORRELATION_ID)
        BMQT_CASE(EVENT_TOO_BIG)
        BMQT_CASE(PAYLOAD_TOO_BIG)
        BMQT_CASE(PAYLOAD_EMPTY)
        BMQT_CASE(OPTION_TOO_BIG)
#ifdef BMQ_ENABLE_MSG_GROUPID
        BMQT_CASE(INVALID_MSG_GROUP_ID)
#endif
        BMQT_CASE(QUEUE_SUSPENDED)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool EventBuilderResult::fromAscii(EventBuilderResult::Enum* out,
                                   const bslstl::StringRef&  str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(EventBuilderResult::e_##M),    \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = EventBuilderResult::e_##M;                                     \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(SUCCESS)
    BMQT_CHECKVALUE(UNKNOWN)
    BMQT_CHECKVALUE(QUEUE_INVALID)
    BMQT_CHECKVALUE(QUEUE_READONLY)
    BMQT_CHECKVALUE(MISSING_CORRELATION_ID)
    BMQT_CHECKVALUE(EVENT_TOO_BIG)
    BMQT_CHECKVALUE(PAYLOAD_TOO_BIG)
    BMQT_CHECKVALUE(PAYLOAD_EMPTY)
    BMQT_CHECKVALUE(OPTION_TOO_BIG)
#ifdef BMQ_ENABLE_MSG_GROUPID
    BMQT_CHECKVALUE(INVALID_MSG_GROUP_ID)
#endif
    BMQT_CHECKVALUE(QUEUE_SUSPENDED)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

// ----------------
// struct AckResult
// ----------------

bsl::ostream& AckResult::print(bsl::ostream&   stream,
                               AckResult::Enum value,
                               int             level,
                               int             spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << AckResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* AckResult::toAscii(AckResult::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(SUCCESS)
        BMQT_CASE(UNKNOWN)
        BMQT_CASE(TIMEOUT)
        BMQT_CASE(NOT_CONNECTED)
        BMQT_CASE(CANCELED)
        BMQT_CASE(NOT_SUPPORTED)
        BMQT_CASE(REFUSED)
        BMQT_CASE(INVALID_ARGUMENT)
        BMQT_CASE(NOT_READY)
        BMQT_CASE(LIMIT_MESSAGES)
        BMQT_CASE(LIMIT_BYTES)
        BMQT_CASE(LIMIT_QUEUE_MESSAGES)
        BMQT_CASE(LIMIT_QUEUE_BYTES)
        BMQT_CASE(STORAGE_FAILURE)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool AckResult::fromAscii(AckResult::Enum* out, const bslstl::StringRef& str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(AckResult::e_##M),             \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = AckResult::e_##M;                                              \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(SUCCESS)
    BMQT_CHECKVALUE(UNKNOWN)
    BMQT_CHECKVALUE(TIMEOUT)
    BMQT_CHECKVALUE(NOT_CONNECTED)
    BMQT_CHECKVALUE(CANCELED)
    BMQT_CHECKVALUE(NOT_SUPPORTED)
    BMQT_CHECKVALUE(REFUSED)
    BMQT_CHECKVALUE(INVALID_ARGUMENT)
    BMQT_CHECKVALUE(NOT_READY)
    BMQT_CHECKVALUE(LIMIT_MESSAGES)
    BMQT_CHECKVALUE(LIMIT_BYTES)
    BMQT_CHECKVALUE(LIMIT_DOMAIN_MESSAGES)
    BMQT_CHECKVALUE(LIMIT_DOMAIN_BYTES)
    BMQT_CHECKVALUE(LIMIT_QUEUE_MESSAGES)
    BMQT_CHECKVALUE(LIMIT_QUEUE_BYTES)
    BMQT_CHECKVALUE(STORAGE_FAILURE)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

// -----------------
// struct PostResult
// -----------------

bsl::ostream& PostResult::print(bsl::ostream&    stream,
                                PostResult::Enum value,
                                int              level,
                                int              spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << PostResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* PostResult::toAscii(PostResult::Enum value)
{
#define BMQT_CASE(X)                                                          \
    case e_##X: return #X;

    switch (value) {
        BMQT_CASE(SUCCESS)
        BMQT_CASE(UNKNOWN)
        BMQT_CASE(TIMEOUT)
        BMQT_CASE(NOT_CONNECTED)
        BMQT_CASE(CANCELED)
        BMQT_CASE(NOT_SUPPORTED)
        BMQT_CASE(REFUSED)
        BMQT_CASE(INVALID_ARGUMENT)
        BMQT_CASE(NOT_READY)
        BMQT_CASE(BW_LIMIT)
    default: return "(* UNKNOWN *)";
    }

#undef BMQT_CASE
}

bool PostResult::fromAscii(PostResult::Enum* out, const bslstl::StringRef& str)
{
#define BMQT_CHECKVALUE(M)                                                    \
    if (bdlb::String::areEqualCaseless(toAscii(PostResult::e_##M),            \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = PostResult::e_##M;                                             \
        return true;                                                          \
    }

    BMQT_CHECKVALUE(SUCCESS)
    BMQT_CHECKVALUE(UNKNOWN)
    BMQT_CHECKVALUE(TIMEOUT)
    BMQT_CHECKVALUE(NOT_CONNECTED)
    BMQT_CHECKVALUE(CANCELED)
    BMQT_CHECKVALUE(NOT_SUPPORTED)
    BMQT_CHECKVALUE(REFUSED)
    BMQT_CHECKVALUE(INVALID_ARGUMENT)
    BMQT_CHECKVALUE(NOT_READY)
    BMQT_CHECKVALUE(BW_LIMIT)

    // Invalid string
    return false;

#undef BMQT_CHECKVALUE
}

}  // close package namespace
}  // close enterprise namespace
