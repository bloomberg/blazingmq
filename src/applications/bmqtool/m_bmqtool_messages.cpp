// Copyright 2014-2025 Bloomberg Finance L.P.
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

// m_bmqtool_messages.cpp          *DO NOT EDIT*           @generated -*-C++-*-

#include <m_bmqtool_messages.h>

#include <bdlat_formattingmode.h>
#include <bdlat_valuetypefunctions.h>
#include <bdlb_print.h>
#include <bdlb_printmethods.h>
#include <bdlb_string.h>

#include <bdlb_nullablevalue.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslim_printer.h>
#include <bsls_assert.h>
#include <bsls_types.h>

#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>

namespace BloombergLP {
namespace m_bmqtool {

// ----------------------
// class BatchPostCommand
// ----------------------

// CONSTANTS

const char BatchPostCommand::CLASS_NAME[] = "BatchPostCommand";

const int BatchPostCommand::DEFAULT_INITIALIZER_MSG_SIZE = 1024;

const bsls::Types::Int64 BatchPostCommand::DEFAULT_INITIALIZER_EVENT_SIZE = 1;

const bsls::Types::Int64 BatchPostCommand::DEFAULT_INITIALIZER_EVENTS_COUNT =
    0;

const int BatchPostCommand::DEFAULT_INITIALIZER_POST_INTERVAL = 1000;

const int BatchPostCommand::DEFAULT_INITIALIZER_POST_RATE = 1;

const char BatchPostCommand::DEFAULT_INITIALIZER_AUTO_INCREMENTED[] = "";

const bdlat_AttributeInfo BatchPostCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PAYLOAD,
     "payload",
     sizeof("payload") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MSG_SIZE,
     "msgSize",
     sizeof("msgSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_EVENT_SIZE,
     "eventSize",
     sizeof("eventSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_EVENTS_COUNT,
     "eventsCount",
     sizeof("eventsCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_POST_INTERVAL,
     "postInterval",
     sizeof("postInterval") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_POST_RATE,
     "postRate",
     sizeof("postRate") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_AUTO_INCREMENTED,
     "autoIncremented",
     sizeof("autoIncremented") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE}};

// CLASS METHODS

const bdlat_AttributeInfo*
BatchPostCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 8; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            BatchPostCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* BatchPostCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_PAYLOAD:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD];
    case ATTRIBUTE_ID_MSG_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE];
    case ATTRIBUTE_ID_EVENT_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE];
    case ATTRIBUTE_ID_EVENTS_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT];
    case ATTRIBUTE_ID_POST_INTERVAL:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL];
    case ATTRIBUTE_ID_POST_RATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE];
    case ATTRIBUTE_ID_AUTO_INCREMENTED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTO_INCREMENTED];
    default: return 0;
    }
}

// CREATORS

BatchPostCommand::BatchPostCommand(bslma::Allocator* basicAllocator)
: d_eventSize(DEFAULT_INITIALIZER_EVENT_SIZE)
, d_eventsCount(DEFAULT_INITIALIZER_EVENTS_COUNT)
, d_payload(basicAllocator)
, d_uri(basicAllocator)
, d_autoIncremented(DEFAULT_INITIALIZER_AUTO_INCREMENTED, basicAllocator)
, d_msgSize(DEFAULT_INITIALIZER_MSG_SIZE)
, d_postInterval(DEFAULT_INITIALIZER_POST_INTERVAL)
, d_postRate(DEFAULT_INITIALIZER_POST_RATE)
{
}

BatchPostCommand::BatchPostCommand(const BatchPostCommand& original,
                                   bslma::Allocator*       basicAllocator)
: d_eventSize(original.d_eventSize)
, d_eventsCount(original.d_eventsCount)
, d_payload(original.d_payload, basicAllocator)
, d_uri(original.d_uri, basicAllocator)
, d_autoIncremented(original.d_autoIncremented, basicAllocator)
, d_msgSize(original.d_msgSize)
, d_postInterval(original.d_postInterval)
, d_postRate(original.d_postRate)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BatchPostCommand::BatchPostCommand(BatchPostCommand&& original) noexcept
: d_eventSize(bsl::move(original.d_eventSize)),
  d_eventsCount(bsl::move(original.d_eventsCount)),
  d_payload(bsl::move(original.d_payload)),
  d_uri(bsl::move(original.d_uri)),
  d_autoIncremented(bsl::move(original.d_autoIncremented)),
  d_msgSize(bsl::move(original.d_msgSize)),
  d_postInterval(bsl::move(original.d_postInterval)),
  d_postRate(bsl::move(original.d_postRate))
{
}

BatchPostCommand::BatchPostCommand(BatchPostCommand&& original,
                                   bslma::Allocator*  basicAllocator)
: d_eventSize(bsl::move(original.d_eventSize))
, d_eventsCount(bsl::move(original.d_eventsCount))
, d_payload(bsl::move(original.d_payload), basicAllocator)
, d_uri(bsl::move(original.d_uri), basicAllocator)
, d_autoIncremented(bsl::move(original.d_autoIncremented), basicAllocator)
, d_msgSize(bsl::move(original.d_msgSize))
, d_postInterval(bsl::move(original.d_postInterval))
, d_postRate(bsl::move(original.d_postRate))
{
}
#endif

BatchPostCommand::~BatchPostCommand()
{
}

// MANIPULATORS

BatchPostCommand& BatchPostCommand::operator=(const BatchPostCommand& rhs)
{
    if (this != &rhs) {
        d_uri             = rhs.d_uri;
        d_payload         = rhs.d_payload;
        d_msgSize         = rhs.d_msgSize;
        d_eventSize       = rhs.d_eventSize;
        d_eventsCount     = rhs.d_eventsCount;
        d_postInterval    = rhs.d_postInterval;
        d_postRate        = rhs.d_postRate;
        d_autoIncremented = rhs.d_autoIncremented;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BatchPostCommand& BatchPostCommand::operator=(BatchPostCommand&& rhs)
{
    if (this != &rhs) {
        d_uri             = bsl::move(rhs.d_uri);
        d_payload         = bsl::move(rhs.d_payload);
        d_msgSize         = bsl::move(rhs.d_msgSize);
        d_eventSize       = bsl::move(rhs.d_eventSize);
        d_eventsCount     = bsl::move(rhs.d_eventsCount);
        d_postInterval    = bsl::move(rhs.d_postInterval);
        d_postRate        = bsl::move(rhs.d_postRate);
        d_autoIncremented = bsl::move(rhs.d_autoIncremented);
    }

    return *this;
}
#endif

void BatchPostCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_payload);
    d_msgSize         = DEFAULT_INITIALIZER_MSG_SIZE;
    d_eventSize       = DEFAULT_INITIALIZER_EVENT_SIZE;
    d_eventsCount     = DEFAULT_INITIALIZER_EVENTS_COUNT;
    d_postInterval    = DEFAULT_INITIALIZER_POST_INTERVAL;
    d_postRate        = DEFAULT_INITIALIZER_POST_RATE;
    d_autoIncremented = DEFAULT_INITIALIZER_AUTO_INCREMENTED;
}

// ACCESSORS

bsl::ostream& BatchPostCommand::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("payload", this->payload());
    printer.printAttribute("msgSize", this->msgSize());
    printer.printAttribute("eventSize", this->eventSize());
    printer.printAttribute("eventsCount", this->eventsCount());
    printer.printAttribute("postInterval", this->postInterval());
    printer.printAttribute("postRate", this->postRate());
    printer.printAttribute("autoIncremented", this->autoIncremented());
    printer.end();
    return stream;
}

// -----------------------
// class CloseQueueCommand
// -----------------------

// CONSTANTS

const char CloseQueueCommand::CLASS_NAME[] = "CloseQueueCommand";

const bool CloseQueueCommand::DEFAULT_INITIALIZER_ASYNC = false;

const bdlat_AttributeInfo CloseQueueCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ASYNC,
     "async",
     sizeof("async") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE}};

// CLASS METHODS

const bdlat_AttributeInfo*
CloseQueueCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            CloseQueueCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* CloseQueueCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_ASYNC:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC];
    default: return 0;
    }
}

// CREATORS

CloseQueueCommand::CloseQueueCommand(bslma::Allocator* basicAllocator)
: d_uri(basicAllocator)
, d_async(DEFAULT_INITIALIZER_ASYNC)
{
}

CloseQueueCommand::CloseQueueCommand(const CloseQueueCommand& original,
                                     bslma::Allocator*        basicAllocator)
: d_uri(original.d_uri, basicAllocator)
, d_async(original.d_async)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CloseQueueCommand::CloseQueueCommand(CloseQueueCommand&& original) noexcept
: d_uri(bsl::move(original.d_uri)),
  d_async(bsl::move(original.d_async))
{
}

CloseQueueCommand::CloseQueueCommand(CloseQueueCommand&& original,
                                     bslma::Allocator*   basicAllocator)
: d_uri(bsl::move(original.d_uri), basicAllocator)
, d_async(bsl::move(original.d_async))
{
}
#endif

CloseQueueCommand::~CloseQueueCommand()
{
}

// MANIPULATORS

CloseQueueCommand& CloseQueueCommand::operator=(const CloseQueueCommand& rhs)
{
    if (this != &rhs) {
        d_uri   = rhs.d_uri;
        d_async = rhs.d_async;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CloseQueueCommand& CloseQueueCommand::operator=(CloseQueueCommand&& rhs)
{
    if (this != &rhs) {
        d_uri   = bsl::move(rhs.d_uri);
        d_async = bsl::move(rhs.d_async);
    }

    return *this;
}
#endif

void CloseQueueCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    d_async = DEFAULT_INITIALIZER_ASYNC;
}

// ACCESSORS

bsl::ostream& CloseQueueCommand::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("async", this->async());
    printer.end();
    return stream;
}

// -------------------------
// class CloseStorageCommand
// -------------------------

// CONSTANTS

const char CloseStorageCommand::CLASS_NAME[] = "CloseStorageCommand";

// CLASS METHODS

const bdlat_AttributeInfo*
CloseStorageCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* CloseStorageCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void CloseStorageCommand::reset()
{
}

// ACCESSORS

bsl::ostream& CloseStorageCommand::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// --------------------
// class ConfirmCommand
// --------------------

// CONSTANTS

const char ConfirmCommand::CLASS_NAME[] = "ConfirmCommand";

const bdlat_AttributeInfo ConfirmCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_GUID,
     "guid",
     sizeof("guid") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ConfirmCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ConfirmCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ConfirmCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_GUID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID];
    default: return 0;
    }
}

// CREATORS

ConfirmCommand::ConfirmCommand(bslma::Allocator* basicAllocator)
: d_uri(basicAllocator)
, d_guid(basicAllocator)
{
}

ConfirmCommand::ConfirmCommand(const ConfirmCommand& original,
                               bslma::Allocator*     basicAllocator)
: d_uri(original.d_uri, basicAllocator)
, d_guid(original.d_guid, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfirmCommand::ConfirmCommand(ConfirmCommand&& original) noexcept
: d_uri(bsl::move(original.d_uri)),
  d_guid(bsl::move(original.d_guid))
{
}

ConfirmCommand::ConfirmCommand(ConfirmCommand&&  original,
                               bslma::Allocator* basicAllocator)
: d_uri(bsl::move(original.d_uri), basicAllocator)
, d_guid(bsl::move(original.d_guid), basicAllocator)
{
}
#endif

ConfirmCommand::~ConfirmCommand()
{
}

// MANIPULATORS

ConfirmCommand& ConfirmCommand::operator=(const ConfirmCommand& rhs)
{
    if (this != &rhs) {
        d_uri  = rhs.d_uri;
        d_guid = rhs.d_guid;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfirmCommand& ConfirmCommand::operator=(ConfirmCommand&& rhs)
{
    if (this != &rhs) {
        d_uri  = bsl::move(rhs.d_uri);
        d_guid = bsl::move(rhs.d_guid);
    }

    return *this;
}
#endif

void ConfirmCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_guid);
}

// ACCESSORS

bsl::ostream& ConfirmCommand::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("guid", this->guid());
    printer.end();
    return stream;
}

// -----------------------
// class DataCommandChoice
// -----------------------

// CONSTANTS

const char DataCommandChoice::CLASS_NAME[] = "DataCommandChoice";

const bdlat_SelectionInfo DataCommandChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_N, "n", sizeof("n") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_NEXT,
     "next",
     sizeof("next") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_P, "p", sizeof("p") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_PREV,
     "prev",
     sizeof("prev") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_R, "r", sizeof("r") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_RECORD,
     "record",
     sizeof("record") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_LIST,
     "list",
     sizeof("list") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_L, "l", sizeof("l") - 1, "", bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_SelectionInfo*
DataCommandChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 8; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            DataCommandChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* DataCommandChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_N: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_N];
    case SELECTION_ID_NEXT: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT];
    case SELECTION_ID_P: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_P];
    case SELECTION_ID_PREV: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV];
    case SELECTION_ID_R: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_R];
    case SELECTION_ID_RECORD:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD];
    case SELECTION_ID_LIST: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST];
    case SELECTION_ID_L: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_L];
    default: return 0;
    }
}

// CREATORS

DataCommandChoice::DataCommandChoice(const DataCommandChoice& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        new (d_n.buffer()) bsls::Types::Uint64(original.d_n.object());
    } break;
    case SELECTION_ID_NEXT: {
        new (d_next.buffer()) bsls::Types::Uint64(original.d_next.object());
    } break;
    case SELECTION_ID_P: {
        new (d_p.buffer()) bsls::Types::Uint64(original.d_p.object());
    } break;
    case SELECTION_ID_PREV: {
        new (d_prev.buffer()) bsls::Types::Uint64(original.d_prev.object());
    } break;
    case SELECTION_ID_R: {
        new (d_r.buffer()) bsls::Types::Uint64(original.d_r.object());
    } break;
    case SELECTION_ID_RECORD: {
        new (d_record.buffer())
            bsls::Types::Uint64(original.d_record.object());
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) int(original.d_list.object());
    } break;
    case SELECTION_ID_L: {
        new (d_l.buffer()) int(original.d_l.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DataCommandChoice::DataCommandChoice(DataCommandChoice&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        new (d_n.buffer())
            bsls::Types::Uint64(bsl::move(original.d_n.object()));
    } break;
    case SELECTION_ID_NEXT: {
        new (d_next.buffer())
            bsls::Types::Uint64(bsl::move(original.d_next.object()));
    } break;
    case SELECTION_ID_P: {
        new (d_p.buffer())
            bsls::Types::Uint64(bsl::move(original.d_p.object()));
    } break;
    case SELECTION_ID_PREV: {
        new (d_prev.buffer())
            bsls::Types::Uint64(bsl::move(original.d_prev.object()));
    } break;
    case SELECTION_ID_R: {
        new (d_r.buffer())
            bsls::Types::Uint64(bsl::move(original.d_r.object()));
    } break;
    case SELECTION_ID_RECORD: {
        new (d_record.buffer())
            bsls::Types::Uint64(bsl::move(original.d_record.object()));
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) int(bsl::move(original.d_list.object()));
    } break;
    case SELECTION_ID_L: {
        new (d_l.buffer()) int(bsl::move(original.d_l.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

DataCommandChoice& DataCommandChoice::operator=(const DataCommandChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_N: {
            makeN(rhs.d_n.object());
        } break;
        case SELECTION_ID_NEXT: {
            makeNext(rhs.d_next.object());
        } break;
        case SELECTION_ID_P: {
            makeP(rhs.d_p.object());
        } break;
        case SELECTION_ID_PREV: {
            makePrev(rhs.d_prev.object());
        } break;
        case SELECTION_ID_R: {
            makeR(rhs.d_r.object());
        } break;
        case SELECTION_ID_RECORD: {
            makeRecord(rhs.d_record.object());
        } break;
        case SELECTION_ID_LIST: {
            makeList(rhs.d_list.object());
        } break;
        case SELECTION_ID_L: {
            makeL(rhs.d_l.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DataCommandChoice& DataCommandChoice::operator=(DataCommandChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_N: {
            makeN(bsl::move(rhs.d_n.object()));
        } break;
        case SELECTION_ID_NEXT: {
            makeNext(bsl::move(rhs.d_next.object()));
        } break;
        case SELECTION_ID_P: {
            makeP(bsl::move(rhs.d_p.object()));
        } break;
        case SELECTION_ID_PREV: {
            makePrev(bsl::move(rhs.d_prev.object()));
        } break;
        case SELECTION_ID_R: {
            makeR(bsl::move(rhs.d_r.object()));
        } break;
        case SELECTION_ID_RECORD: {
            makeRecord(bsl::move(rhs.d_record.object()));
        } break;
        case SELECTION_ID_LIST: {
            makeList(bsl::move(rhs.d_list.object()));
        } break;
        case SELECTION_ID_L: {
            makeL(bsl::move(rhs.d_l.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void DataCommandChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        // no destruction required
    } break;
    case SELECTION_ID_NEXT: {
        // no destruction required
    } break;
    case SELECTION_ID_P: {
        // no destruction required
    } break;
    case SELECTION_ID_PREV: {
        // no destruction required
    } break;
    case SELECTION_ID_R: {
        // no destruction required
    } break;
    case SELECTION_ID_RECORD: {
        // no destruction required
    } break;
    case SELECTION_ID_LIST: {
        // no destruction required
    } break;
    case SELECTION_ID_L: {
        // no destruction required
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int DataCommandChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_N: {
        makeN();
    } break;
    case SELECTION_ID_NEXT: {
        makeNext();
    } break;
    case SELECTION_ID_P: {
        makeP();
    } break;
    case SELECTION_ID_PREV: {
        makePrev();
    } break;
    case SELECTION_ID_R: {
        makeR();
    } break;
    case SELECTION_ID_RECORD: {
        makeRecord();
    } break;
    case SELECTION_ID_LIST: {
        makeList();
    } break;
    case SELECTION_ID_L: {
        makeL();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int DataCommandChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

bsls::Types::Uint64& DataCommandChoice::makeN()
{
    if (SELECTION_ID_N == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_n.object());
    }
    else {
        reset();
        new (d_n.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_N;
    }

    return d_n.object();
}

bsls::Types::Uint64& DataCommandChoice::makeN(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_N == d_selectionId) {
        d_n.object() = value;
    }
    else {
        reset();
        new (d_n.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_N;
    }

    return d_n.object();
}

bsls::Types::Uint64& DataCommandChoice::makeNext()
{
    if (SELECTION_ID_NEXT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_next.object());
    }
    else {
        reset();
        new (d_next.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_NEXT;
    }

    return d_next.object();
}

bsls::Types::Uint64& DataCommandChoice::makeNext(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_NEXT == d_selectionId) {
        d_next.object() = value;
    }
    else {
        reset();
        new (d_next.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_NEXT;
    }

    return d_next.object();
}

bsls::Types::Uint64& DataCommandChoice::makeP()
{
    if (SELECTION_ID_P == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_p.object());
    }
    else {
        reset();
        new (d_p.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_P;
    }

    return d_p.object();
}

bsls::Types::Uint64& DataCommandChoice::makeP(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_P == d_selectionId) {
        d_p.object() = value;
    }
    else {
        reset();
        new (d_p.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_P;
    }

    return d_p.object();
}

bsls::Types::Uint64& DataCommandChoice::makePrev()
{
    if (SELECTION_ID_PREV == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_prev.object());
    }
    else {
        reset();
        new (d_prev.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_PREV;
    }

    return d_prev.object();
}

bsls::Types::Uint64& DataCommandChoice::makePrev(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_PREV == d_selectionId) {
        d_prev.object() = value;
    }
    else {
        reset();
        new (d_prev.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_PREV;
    }

    return d_prev.object();
}

bsls::Types::Uint64& DataCommandChoice::makeR()
{
    if (SELECTION_ID_R == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_r.object());
    }
    else {
        reset();
        new (d_r.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_R;
    }

    return d_r.object();
}

bsls::Types::Uint64& DataCommandChoice::makeR(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_R == d_selectionId) {
        d_r.object() = value;
    }
    else {
        reset();
        new (d_r.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_R;
    }

    return d_r.object();
}

bsls::Types::Uint64& DataCommandChoice::makeRecord()
{
    if (SELECTION_ID_RECORD == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_record.object());
    }
    else {
        reset();
        new (d_record.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_RECORD;
    }

    return d_record.object();
}

bsls::Types::Uint64& DataCommandChoice::makeRecord(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_RECORD == d_selectionId) {
        d_record.object() = value;
    }
    else {
        reset();
        new (d_record.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_RECORD;
    }

    return d_record.object();
}

int& DataCommandChoice::makeList()
{
    if (SELECTION_ID_LIST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_list.object());
    }
    else {
        reset();
        new (d_list.buffer()) int();
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

int& DataCommandChoice::makeList(int value)
{
    if (SELECTION_ID_LIST == d_selectionId) {
        d_list.object() = value;
    }
    else {
        reset();
        new (d_list.buffer()) int(value);
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

int& DataCommandChoice::makeL()
{
    if (SELECTION_ID_L == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_l.object());
    }
    else {
        reset();
        new (d_l.buffer()) int();
        d_selectionId = SELECTION_ID_L;
    }

    return d_l.object();
}

int& DataCommandChoice::makeL(int value)
{
    if (SELECTION_ID_L == d_selectionId) {
        d_l.object() = value;
    }
    else {
        reset();
        new (d_l.buffer()) int(value);
        d_selectionId = SELECTION_ID_L;
    }

    return d_l.object();
}

// ACCESSORS

bsl::ostream& DataCommandChoice::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        printer.printAttribute("n", d_n.object());
    } break;
    case SELECTION_ID_NEXT: {
        printer.printAttribute("next", d_next.object());
    } break;
    case SELECTION_ID_P: {
        printer.printAttribute("p", d_p.object());
    } break;
    case SELECTION_ID_PREV: {
        printer.printAttribute("prev", d_prev.object());
    } break;
    case SELECTION_ID_R: {
        printer.printAttribute("r", d_r.object());
    } break;
    case SELECTION_ID_RECORD: {
        printer.printAttribute("record", d_record.object());
    } break;
    case SELECTION_ID_LIST: {
        printer.printAttribute("list", d_list.object());
    } break;
    case SELECTION_ID_L: {
        printer.printAttribute("l", d_l.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* DataCommandChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_N: return SELECTION_INFO_ARRAY[SELECTION_INDEX_N].name();
    case SELECTION_ID_NEXT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT].name();
    case SELECTION_ID_P: return SELECTION_INFO_ARRAY[SELECTION_INDEX_P].name();
    case SELECTION_ID_PREV:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV].name();
    case SELECTION_ID_R: return SELECTION_INFO_ARRAY[SELECTION_INDEX_R].name();
    case SELECTION_ID_RECORD:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD].name();
    case SELECTION_ID_LIST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST].name();
    case SELECTION_ID_L: return SELECTION_INFO_ARRAY[SELECTION_INDEX_L].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------------
// class DumpQueueCommand
// ----------------------

// CONSTANTS

const char DumpQueueCommand::CLASS_NAME[] = "DumpQueueCommand";

const bdlat_AttributeInfo DumpQueueCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_KEY,
     "key",
     sizeof("key") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
DumpQueueCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DumpQueueCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DumpQueueCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_KEY: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY];
    default: return 0;
    }
}

// CREATORS

DumpQueueCommand::DumpQueueCommand(bslma::Allocator* basicAllocator)
: d_uri(basicAllocator)
, d_key(basicAllocator)
{
}

DumpQueueCommand::DumpQueueCommand(const DumpQueueCommand& original,
                                   bslma::Allocator*       basicAllocator)
: d_uri(original.d_uri, basicAllocator)
, d_key(original.d_key, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DumpQueueCommand::DumpQueueCommand(DumpQueueCommand&& original) noexcept
: d_uri(bsl::move(original.d_uri)),
  d_key(bsl::move(original.d_key))
{
}

DumpQueueCommand::DumpQueueCommand(DumpQueueCommand&& original,
                                   bslma::Allocator*  basicAllocator)
: d_uri(bsl::move(original.d_uri), basicAllocator)
, d_key(bsl::move(original.d_key), basicAllocator)
{
}
#endif

DumpQueueCommand::~DumpQueueCommand()
{
}

// MANIPULATORS

DumpQueueCommand& DumpQueueCommand::operator=(const DumpQueueCommand& rhs)
{
    if (this != &rhs) {
        d_uri = rhs.d_uri;
        d_key = rhs.d_key;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DumpQueueCommand& DumpQueueCommand::operator=(DumpQueueCommand&& rhs)
{
    if (this != &rhs) {
        d_uri = bsl::move(rhs.d_uri);
        d_key = bsl::move(rhs.d_key);
    }

    return *this;
}
#endif

void DumpQueueCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_key);
}

// ACCESSORS

bsl::ostream& DumpQueueCommand::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("key", this->key());
    printer.end();
    return stream;
}

// ------------------------------
// class JournalCommandChoiceType
// ------------------------------

// CONSTANTS

const char JournalCommandChoiceType::CLASS_NAME[] = "JournalCommandChoiceType";

const bdlat_EnumeratorInfo JournalCommandChoiceType::ENUMERATOR_INFO_ARRAY[] =
    {{JournalCommandChoiceType::CONFIRM, "confirm", sizeof("confirm") - 1, ""},
     {JournalCommandChoiceType::DELETE, "delete", sizeof("delete") - 1, ""},
     {JournalCommandChoiceType::JOP, "jop", sizeof("jop") - 1, ""},
     {JournalCommandChoiceType::MESSAGE, "message", sizeof("message") - 1, ""},
     {JournalCommandChoiceType::QOP, "qop", sizeof("qop") - 1, ""}};

// CLASS METHODS

int JournalCommandChoiceType::fromInt(JournalCommandChoiceType::Value* result,
                                      int                              number)
{
    switch (number) {
    case JournalCommandChoiceType::CONFIRM:
    case JournalCommandChoiceType::DELETE:
    case JournalCommandChoiceType::JOP:
    case JournalCommandChoiceType::MESSAGE:
    case JournalCommandChoiceType::QOP:
        *result = static_cast<JournalCommandChoiceType::Value>(number);
        return 0;
    default: return -1;
    }
}

int JournalCommandChoiceType::fromString(
    JournalCommandChoiceType::Value* result,
    const char*                      string,
    int                              stringLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            JournalCommandChoiceType::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<JournalCommandChoiceType::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char*
JournalCommandChoiceType::toString(JournalCommandChoiceType::Value value)
{
    switch (value) {
    case CONFIRM: {
        return "confirm";
    }
    case DELETE: {
        return "delete";
    }
    case JOP: {
        return "jop";
    }
    case MESSAGE: {
        return "message";
    }
    case QOP: {
        return "qop";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -----------------
// class ListCommand
// -----------------

// CONSTANTS

const char ListCommand::CLASS_NAME[] = "ListCommand";

const bdlat_AttributeInfo ListCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* ListCommand::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ListCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ListCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    default: return 0;
    }
}

// CREATORS

ListCommand::ListCommand(bslma::Allocator* basicAllocator)
: d_uri(basicAllocator)
{
}

ListCommand::ListCommand(const ListCommand& original,
                         bslma::Allocator*  basicAllocator)
: d_uri(original.d_uri, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ListCommand::ListCommand(ListCommand&& original) noexcept
: d_uri(bsl::move(original.d_uri))
{
}

ListCommand::ListCommand(ListCommand&&     original,
                         bslma::Allocator* basicAllocator)
: d_uri(bsl::move(original.d_uri), basicAllocator)
{
}
#endif

ListCommand::~ListCommand()
{
}

// MANIPULATORS

ListCommand& ListCommand::operator=(const ListCommand& rhs)
{
    if (this != &rhs) {
        d_uri = rhs.d_uri;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ListCommand& ListCommand::operator=(ListCommand&& rhs)
{
    if (this != &rhs) {
        d_uri = bsl::move(rhs.d_uri);
    }

    return *this;
}
#endif

void ListCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
}

// ACCESSORS

bsl::ostream&
ListCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.end();
    return stream;
}

// -----------------------
// class ListQueuesCommand
// -----------------------

// CONSTANTS

const char ListQueuesCommand::CLASS_NAME[] = "ListQueuesCommand";

// CLASS METHODS

const bdlat_AttributeInfo*
ListQueuesCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* ListQueuesCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void ListQueuesCommand::reset()
{
}

// ACCESSORS

bsl::ostream& ListQueuesCommand::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ---------------------
// class LoadPostCommand
// ---------------------

// CONSTANTS

const char LoadPostCommand::CLASS_NAME[] = "LoadPostCommand";

const bdlat_AttributeInfo LoadPostCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_FILE,
     "file",
     sizeof("file") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
LoadPostCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LoadPostCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* LoadPostCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_FILE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE];
    default: return 0;
    }
}

// CREATORS

LoadPostCommand::LoadPostCommand(bslma::Allocator* basicAllocator)
: d_uri(basicAllocator)
, d_file(basicAllocator)
{
}

LoadPostCommand::LoadPostCommand(const LoadPostCommand& original,
                                 bslma::Allocator*      basicAllocator)
: d_uri(original.d_uri, basicAllocator)
, d_file(original.d_file, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LoadPostCommand::LoadPostCommand(LoadPostCommand&& original) noexcept
: d_uri(bsl::move(original.d_uri)),
  d_file(bsl::move(original.d_file))
{
}

LoadPostCommand::LoadPostCommand(LoadPostCommand&& original,
                                 bslma::Allocator* basicAllocator)
: d_uri(bsl::move(original.d_uri), basicAllocator)
, d_file(bsl::move(original.d_file), basicAllocator)
{
}
#endif

LoadPostCommand::~LoadPostCommand()
{
}

// MANIPULATORS

LoadPostCommand& LoadPostCommand::operator=(const LoadPostCommand& rhs)
{
    if (this != &rhs) {
        d_uri  = rhs.d_uri;
        d_file = rhs.d_file;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LoadPostCommand& LoadPostCommand::operator=(LoadPostCommand&& rhs)
{
    if (this != &rhs) {
        d_uri  = bsl::move(rhs.d_uri);
        d_file = bsl::move(rhs.d_file);
    }

    return *this;
}
#endif

void LoadPostCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_file);
}

// ACCESSORS

bsl::ostream& LoadPostCommand::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("file", this->file());
    printer.end();
    return stream;
}

// -------------------------
// class MessagePropertyType
// -------------------------

// CONSTANTS

const char MessagePropertyType::CLASS_NAME[] = "MessagePropertyType";

const bdlat_EnumeratorInfo MessagePropertyType::ENUMERATOR_INFO_ARRAY[] = {
    {MessagePropertyType::E_STRING, "E_STRING", sizeof("E_STRING") - 1, ""},
    {MessagePropertyType::E_INT, "E_INT", sizeof("E_INT") - 1, ""}};

// CLASS METHODS

int MessagePropertyType::fromInt(MessagePropertyType::Value* result,
                                 int                         number)
{
    switch (number) {
    case MessagePropertyType::E_STRING:
    case MessagePropertyType::E_INT:
        *result = static_cast<MessagePropertyType::Value>(number);
        return 0;
    default: return -1;
    }
}

int MessagePropertyType::fromString(MessagePropertyType::Value* result,
                                    const char*                 string,
                                    int                         stringLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            MessagePropertyType::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<MessagePropertyType::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* MessagePropertyType::toString(MessagePropertyType::Value value)
{
    switch (value) {
    case E_STRING: {
        return "E_STRING";
    }
    case E_INT: {
        return "E_INT";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ---------------------
// class MetadataCommand
// ---------------------

// CONSTANTS

const char MetadataCommand::CLASS_NAME[] = "MetadataCommand";

// CLASS METHODS

const bdlat_AttributeInfo*
MetadataCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* MetadataCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void MetadataCommand::reset()
{
}

// ACCESSORS

bsl::ostream& MetadataCommand::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ------------------------
// class OpenStorageCommand
// ------------------------

// CONSTANTS

const char OpenStorageCommand::CLASS_NAME[] = "OpenStorageCommand";

const bdlat_AttributeInfo OpenStorageCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PATH,
     "path",
     sizeof("path") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
OpenStorageCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            OpenStorageCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* OpenStorageCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PATH: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH];
    default: return 0;
    }
}

// CREATORS

OpenStorageCommand::OpenStorageCommand(bslma::Allocator* basicAllocator)
: d_path(basicAllocator)
{
}

OpenStorageCommand::OpenStorageCommand(const OpenStorageCommand& original,
                                       bslma::Allocator* basicAllocator)
: d_path(original.d_path, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenStorageCommand::OpenStorageCommand(OpenStorageCommand&& original) noexcept
: d_path(bsl::move(original.d_path))
{
}

OpenStorageCommand::OpenStorageCommand(OpenStorageCommand&& original,
                                       bslma::Allocator*    basicAllocator)
: d_path(bsl::move(original.d_path), basicAllocator)
{
}
#endif

OpenStorageCommand::~OpenStorageCommand()
{
}

// MANIPULATORS

OpenStorageCommand&
OpenStorageCommand::operator=(const OpenStorageCommand& rhs)
{
    if (this != &rhs) {
        d_path = rhs.d_path;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenStorageCommand& OpenStorageCommand::operator=(OpenStorageCommand&& rhs)
{
    if (this != &rhs) {
        d_path = bsl::move(rhs.d_path);
    }

    return *this;
}
#endif

void OpenStorageCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_path);
}

// ACCESSORS

bsl::ostream& OpenStorageCommand::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("path", this->path());
    printer.end();
    return stream;
}

// ------------------------
// class QlistCommandChoice
// ------------------------

// CONSTANTS

const char QlistCommandChoice::CLASS_NAME[] = "QlistCommandChoice";

const bdlat_SelectionInfo QlistCommandChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_N, "n", sizeof("n") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_NEXT,
     "next",
     sizeof("next") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_P, "p", sizeof("p") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_PREV,
     "prev",
     sizeof("prev") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_R, "r", sizeof("r") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_RECORD,
     "record",
     sizeof("record") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_LIST,
     "list",
     sizeof("list") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_L, "l", sizeof("l") - 1, "", bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_SelectionInfo*
QlistCommandChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 8; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            QlistCommandChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* QlistCommandChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_N: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_N];
    case SELECTION_ID_NEXT: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT];
    case SELECTION_ID_P: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_P];
    case SELECTION_ID_PREV: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV];
    case SELECTION_ID_R: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_R];
    case SELECTION_ID_RECORD:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD];
    case SELECTION_ID_LIST: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST];
    case SELECTION_ID_L: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_L];
    default: return 0;
    }
}

// CREATORS

QlistCommandChoice::QlistCommandChoice(const QlistCommandChoice& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        new (d_n.buffer()) bsls::Types::Uint64(original.d_n.object());
    } break;
    case SELECTION_ID_NEXT: {
        new (d_next.buffer()) bsls::Types::Uint64(original.d_next.object());
    } break;
    case SELECTION_ID_P: {
        new (d_p.buffer()) bsls::Types::Uint64(original.d_p.object());
    } break;
    case SELECTION_ID_PREV: {
        new (d_prev.buffer()) bsls::Types::Uint64(original.d_prev.object());
    } break;
    case SELECTION_ID_R: {
        new (d_r.buffer()) bsls::Types::Uint64(original.d_r.object());
    } break;
    case SELECTION_ID_RECORD: {
        new (d_record.buffer())
            bsls::Types::Uint64(original.d_record.object());
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) int(original.d_list.object());
    } break;
    case SELECTION_ID_L: {
        new (d_l.buffer()) int(original.d_l.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QlistCommandChoice::QlistCommandChoice(QlistCommandChoice&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        new (d_n.buffer())
            bsls::Types::Uint64(bsl::move(original.d_n.object()));
    } break;
    case SELECTION_ID_NEXT: {
        new (d_next.buffer())
            bsls::Types::Uint64(bsl::move(original.d_next.object()));
    } break;
    case SELECTION_ID_P: {
        new (d_p.buffer())
            bsls::Types::Uint64(bsl::move(original.d_p.object()));
    } break;
    case SELECTION_ID_PREV: {
        new (d_prev.buffer())
            bsls::Types::Uint64(bsl::move(original.d_prev.object()));
    } break;
    case SELECTION_ID_R: {
        new (d_r.buffer())
            bsls::Types::Uint64(bsl::move(original.d_r.object()));
    } break;
    case SELECTION_ID_RECORD: {
        new (d_record.buffer())
            bsls::Types::Uint64(bsl::move(original.d_record.object()));
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) int(bsl::move(original.d_list.object()));
    } break;
    case SELECTION_ID_L: {
        new (d_l.buffer()) int(bsl::move(original.d_l.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

QlistCommandChoice&
QlistCommandChoice::operator=(const QlistCommandChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_N: {
            makeN(rhs.d_n.object());
        } break;
        case SELECTION_ID_NEXT: {
            makeNext(rhs.d_next.object());
        } break;
        case SELECTION_ID_P: {
            makeP(rhs.d_p.object());
        } break;
        case SELECTION_ID_PREV: {
            makePrev(rhs.d_prev.object());
        } break;
        case SELECTION_ID_R: {
            makeR(rhs.d_r.object());
        } break;
        case SELECTION_ID_RECORD: {
            makeRecord(rhs.d_record.object());
        } break;
        case SELECTION_ID_LIST: {
            makeList(rhs.d_list.object());
        } break;
        case SELECTION_ID_L: {
            makeL(rhs.d_l.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QlistCommandChoice& QlistCommandChoice::operator=(QlistCommandChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_N: {
            makeN(bsl::move(rhs.d_n.object()));
        } break;
        case SELECTION_ID_NEXT: {
            makeNext(bsl::move(rhs.d_next.object()));
        } break;
        case SELECTION_ID_P: {
            makeP(bsl::move(rhs.d_p.object()));
        } break;
        case SELECTION_ID_PREV: {
            makePrev(bsl::move(rhs.d_prev.object()));
        } break;
        case SELECTION_ID_R: {
            makeR(bsl::move(rhs.d_r.object()));
        } break;
        case SELECTION_ID_RECORD: {
            makeRecord(bsl::move(rhs.d_record.object()));
        } break;
        case SELECTION_ID_LIST: {
            makeList(bsl::move(rhs.d_list.object()));
        } break;
        case SELECTION_ID_L: {
            makeL(bsl::move(rhs.d_l.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void QlistCommandChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        // no destruction required
    } break;
    case SELECTION_ID_NEXT: {
        // no destruction required
    } break;
    case SELECTION_ID_P: {
        // no destruction required
    } break;
    case SELECTION_ID_PREV: {
        // no destruction required
    } break;
    case SELECTION_ID_R: {
        // no destruction required
    } break;
    case SELECTION_ID_RECORD: {
        // no destruction required
    } break;
    case SELECTION_ID_LIST: {
        // no destruction required
    } break;
    case SELECTION_ID_L: {
        // no destruction required
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int QlistCommandChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_N: {
        makeN();
    } break;
    case SELECTION_ID_NEXT: {
        makeNext();
    } break;
    case SELECTION_ID_P: {
        makeP();
    } break;
    case SELECTION_ID_PREV: {
        makePrev();
    } break;
    case SELECTION_ID_R: {
        makeR();
    } break;
    case SELECTION_ID_RECORD: {
        makeRecord();
    } break;
    case SELECTION_ID_LIST: {
        makeList();
    } break;
    case SELECTION_ID_L: {
        makeL();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int QlistCommandChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

bsls::Types::Uint64& QlistCommandChoice::makeN()
{
    if (SELECTION_ID_N == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_n.object());
    }
    else {
        reset();
        new (d_n.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_N;
    }

    return d_n.object();
}

bsls::Types::Uint64& QlistCommandChoice::makeN(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_N == d_selectionId) {
        d_n.object() = value;
    }
    else {
        reset();
        new (d_n.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_N;
    }

    return d_n.object();
}

bsls::Types::Uint64& QlistCommandChoice::makeNext()
{
    if (SELECTION_ID_NEXT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_next.object());
    }
    else {
        reset();
        new (d_next.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_NEXT;
    }

    return d_next.object();
}

bsls::Types::Uint64& QlistCommandChoice::makeNext(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_NEXT == d_selectionId) {
        d_next.object() = value;
    }
    else {
        reset();
        new (d_next.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_NEXT;
    }

    return d_next.object();
}

bsls::Types::Uint64& QlistCommandChoice::makeP()
{
    if (SELECTION_ID_P == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_p.object());
    }
    else {
        reset();
        new (d_p.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_P;
    }

    return d_p.object();
}

bsls::Types::Uint64& QlistCommandChoice::makeP(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_P == d_selectionId) {
        d_p.object() = value;
    }
    else {
        reset();
        new (d_p.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_P;
    }

    return d_p.object();
}

bsls::Types::Uint64& QlistCommandChoice::makePrev()
{
    if (SELECTION_ID_PREV == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_prev.object());
    }
    else {
        reset();
        new (d_prev.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_PREV;
    }

    return d_prev.object();
}

bsls::Types::Uint64& QlistCommandChoice::makePrev(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_PREV == d_selectionId) {
        d_prev.object() = value;
    }
    else {
        reset();
        new (d_prev.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_PREV;
    }

    return d_prev.object();
}

bsls::Types::Uint64& QlistCommandChoice::makeR()
{
    if (SELECTION_ID_R == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_r.object());
    }
    else {
        reset();
        new (d_r.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_R;
    }

    return d_r.object();
}

bsls::Types::Uint64& QlistCommandChoice::makeR(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_R == d_selectionId) {
        d_r.object() = value;
    }
    else {
        reset();
        new (d_r.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_R;
    }

    return d_r.object();
}

bsls::Types::Uint64& QlistCommandChoice::makeRecord()
{
    if (SELECTION_ID_RECORD == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_record.object());
    }
    else {
        reset();
        new (d_record.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_RECORD;
    }

    return d_record.object();
}

bsls::Types::Uint64& QlistCommandChoice::makeRecord(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_RECORD == d_selectionId) {
        d_record.object() = value;
    }
    else {
        reset();
        new (d_record.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_RECORD;
    }

    return d_record.object();
}

int& QlistCommandChoice::makeList()
{
    if (SELECTION_ID_LIST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_list.object());
    }
    else {
        reset();
        new (d_list.buffer()) int();
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

int& QlistCommandChoice::makeList(int value)
{
    if (SELECTION_ID_LIST == d_selectionId) {
        d_list.object() = value;
    }
    else {
        reset();
        new (d_list.buffer()) int(value);
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

int& QlistCommandChoice::makeL()
{
    if (SELECTION_ID_L == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_l.object());
    }
    else {
        reset();
        new (d_l.buffer()) int();
        d_selectionId = SELECTION_ID_L;
    }

    return d_l.object();
}

int& QlistCommandChoice::makeL(int value)
{
    if (SELECTION_ID_L == d_selectionId) {
        d_l.object() = value;
    }
    else {
        reset();
        new (d_l.buffer()) int(value);
        d_selectionId = SELECTION_ID_L;
    }

    return d_l.object();
}

// ACCESSORS

bsl::ostream& QlistCommandChoice::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        printer.printAttribute("n", d_n.object());
    } break;
    case SELECTION_ID_NEXT: {
        printer.printAttribute("next", d_next.object());
    } break;
    case SELECTION_ID_P: {
        printer.printAttribute("p", d_p.object());
    } break;
    case SELECTION_ID_PREV: {
        printer.printAttribute("prev", d_prev.object());
    } break;
    case SELECTION_ID_R: {
        printer.printAttribute("r", d_r.object());
    } break;
    case SELECTION_ID_RECORD: {
        printer.printAttribute("record", d_record.object());
    } break;
    case SELECTION_ID_LIST: {
        printer.printAttribute("list", d_list.object());
    } break;
    case SELECTION_ID_L: {
        printer.printAttribute("l", d_l.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* QlistCommandChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_N: return SELECTION_INFO_ARRAY[SELECTION_INDEX_N].name();
    case SELECTION_ID_NEXT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT].name();
    case SELECTION_ID_P: return SELECTION_INFO_ARRAY[SELECTION_INDEX_P].name();
    case SELECTION_ID_PREV:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV].name();
    case SELECTION_ID_R: return SELECTION_INFO_ARRAY[SELECTION_INDEX_R].name();
    case SELECTION_ID_RECORD:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD].name();
    case SELECTION_ID_LIST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST].name();
    case SELECTION_ID_L: return SELECTION_INFO_ARRAY[SELECTION_INDEX_L].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ------------------
// class StartCommand
// ------------------

// CONSTANTS

const char StartCommand::CLASS_NAME[] = "StartCommand";

const bool StartCommand::DEFAULT_INITIALIZER_ASYNC = false;

const bdlat_AttributeInfo StartCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ASYNC,
     "async",
     sizeof("async") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE}};

// CLASS METHODS

const bdlat_AttributeInfo* StartCommand::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StartCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StartCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ASYNC:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC];
    default: return 0;
    }
}

// CREATORS

StartCommand::StartCommand()
: d_async(DEFAULT_INITIALIZER_ASYNC)
{
}

// MANIPULATORS

void StartCommand::reset()
{
    d_async = DEFAULT_INITIALIZER_ASYNC;
}

// ACCESSORS

bsl::ostream&
StartCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("async", this->async());
    printer.end();
    return stream;
}

// -----------------
// class StopCommand
// -----------------

// CONSTANTS

const char StopCommand::CLASS_NAME[] = "StopCommand";

const bool StopCommand::DEFAULT_INITIALIZER_ASYNC = false;

const bdlat_AttributeInfo StopCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ASYNC,
     "async",
     sizeof("async") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE}};

// CLASS METHODS

const bdlat_AttributeInfo* StopCommand::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StopCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StopCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ASYNC:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC];
    default: return 0;
    }
}

// CREATORS

StopCommand::StopCommand()
: d_async(DEFAULT_INITIALIZER_ASYNC)
{
}

// MANIPULATORS

void StopCommand::reset()
{
    d_async = DEFAULT_INITIALIZER_ASYNC;
}

// ACCESSORS

bsl::ostream&
StopCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("async", this->async());
    printer.end();
    return stream;
}

// ------------------
// class Subscription
// ------------------

// CONSTANTS

const char Subscription::CLASS_NAME[] = "Subscription";

const bdlat_AttributeInfo Subscription::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CORRELATION_ID,
     "correlationId",
     sizeof("correlationId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_EXPRESSION,
     "expression",
     sizeof("expression") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES,
     "maxUnconfirmedMessages",
     sizeof("maxUnconfirmedMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES,
     "maxUnconfirmedBytes",
     sizeof("maxUnconfirmedBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CONSUMER_PRIORITY,
     "consumerPriority",
     sizeof("consumerPriority") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* Subscription::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Subscription::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Subscription::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CORRELATION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CORRELATION_ID];
    case ATTRIBUTE_ID_EXPRESSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES];
    case ATTRIBUTE_ID_CONSUMER_PRIORITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY];
    default: return 0;
    }
}

// CREATORS

Subscription::Subscription(bslma::Allocator* basicAllocator)
: d_expression(basicAllocator)
, d_correlationId()
, d_maxUnconfirmedMessages()
, d_maxUnconfirmedBytes()
, d_consumerPriority()
{
}

Subscription::Subscription(const Subscription& original,
                           bslma::Allocator*   basicAllocator)
: d_expression(original.d_expression, basicAllocator)
, d_correlationId(original.d_correlationId)
, d_maxUnconfirmedMessages(original.d_maxUnconfirmedMessages)
, d_maxUnconfirmedBytes(original.d_maxUnconfirmedBytes)
, d_consumerPriority(original.d_consumerPriority)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Subscription::Subscription(Subscription&& original) noexcept
: d_expression(bsl::move(original.d_expression)),
  d_correlationId(bsl::move(original.d_correlationId)),
  d_maxUnconfirmedMessages(bsl::move(original.d_maxUnconfirmedMessages)),
  d_maxUnconfirmedBytes(bsl::move(original.d_maxUnconfirmedBytes)),
  d_consumerPriority(bsl::move(original.d_consumerPriority))
{
}

Subscription::Subscription(Subscription&&    original,
                           bslma::Allocator* basicAllocator)
: d_expression(bsl::move(original.d_expression), basicAllocator)
, d_correlationId(bsl::move(original.d_correlationId))
, d_maxUnconfirmedMessages(bsl::move(original.d_maxUnconfirmedMessages))
, d_maxUnconfirmedBytes(bsl::move(original.d_maxUnconfirmedBytes))
, d_consumerPriority(bsl::move(original.d_consumerPriority))
{
}
#endif

Subscription::~Subscription()
{
}

// MANIPULATORS

Subscription& Subscription::operator=(const Subscription& rhs)
{
    if (this != &rhs) {
        d_correlationId          = rhs.d_correlationId;
        d_expression             = rhs.d_expression;
        d_maxUnconfirmedMessages = rhs.d_maxUnconfirmedMessages;
        d_maxUnconfirmedBytes    = rhs.d_maxUnconfirmedBytes;
        d_consumerPriority       = rhs.d_consumerPriority;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Subscription& Subscription::operator=(Subscription&& rhs)
{
    if (this != &rhs) {
        d_correlationId          = bsl::move(rhs.d_correlationId);
        d_expression             = bsl::move(rhs.d_expression);
        d_maxUnconfirmedMessages = bsl::move(rhs.d_maxUnconfirmedMessages);
        d_maxUnconfirmedBytes    = bsl::move(rhs.d_maxUnconfirmedBytes);
        d_consumerPriority       = bsl::move(rhs.d_consumerPriority);
    }

    return *this;
}
#endif

void Subscription::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_correlationId);
    bdlat_ValueTypeFunctions::reset(&d_expression);
    bdlat_ValueTypeFunctions::reset(&d_maxUnconfirmedMessages);
    bdlat_ValueTypeFunctions::reset(&d_maxUnconfirmedBytes);
    bdlat_ValueTypeFunctions::reset(&d_consumerPriority);
}

// ACCESSORS

bsl::ostream&
Subscription::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("correlationId", this->correlationId());
    printer.printAttribute("expression", this->expression());
    printer.printAttribute("maxUnconfirmedMessages",
                           this->maxUnconfirmedMessages());
    printer.printAttribute("maxUnconfirmedBytes", this->maxUnconfirmedBytes());
    printer.printAttribute("consumerPriority", this->consumerPriority());
    printer.end();
    return stream;
}

// ---------------------------
// class ConfigureQueueCommand
// ---------------------------

// CONSTANTS

const char ConfigureQueueCommand::CLASS_NAME[] = "ConfigureQueueCommand";

const bool ConfigureQueueCommand::DEFAULT_INITIALIZER_ASYNC = false;

const int ConfigureQueueCommand::DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES =
    1024;

const int ConfigureQueueCommand::DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES =
    33554432;

const int ConfigureQueueCommand::DEFAULT_INITIALIZER_CONSUMER_PRIORITY = 0;

const bdlat_AttributeInfo ConfigureQueueCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ASYNC,
     "async",
     sizeof("async") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES,
     "maxUnconfirmedMessages",
     sizeof("maxUnconfirmedMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES,
     "maxUnconfirmedBytes",
     sizeof("maxUnconfirmedBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_CONSUMER_PRIORITY,
     "consumerPriority",
     sizeof("consumerPriority") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_SUBSCRIPTIONS,
     "subscriptions",
     sizeof("subscriptions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ConfigureQueueCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ConfigureQueueCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ConfigureQueueCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_ASYNC:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES];
    case ATTRIBUTE_ID_CONSUMER_PRIORITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY];
    case ATTRIBUTE_ID_SUBSCRIPTIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS];
    default: return 0;
    }
}

// CREATORS

ConfigureQueueCommand::ConfigureQueueCommand(bslma::Allocator* basicAllocator)
: d_subscriptions(basicAllocator)
, d_uri(basicAllocator)
, d_maxUnconfirmedMessages(DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES)
, d_maxUnconfirmedBytes(DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES)
, d_consumerPriority(DEFAULT_INITIALIZER_CONSUMER_PRIORITY)
, d_async(DEFAULT_INITIALIZER_ASYNC)
{
}

ConfigureQueueCommand::ConfigureQueueCommand(
    const ConfigureQueueCommand& original,
    bslma::Allocator*            basicAllocator)
: d_subscriptions(original.d_subscriptions, basicAllocator)
, d_uri(original.d_uri, basicAllocator)
, d_maxUnconfirmedMessages(original.d_maxUnconfirmedMessages)
, d_maxUnconfirmedBytes(original.d_maxUnconfirmedBytes)
, d_consumerPriority(original.d_consumerPriority)
, d_async(original.d_async)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureQueueCommand::ConfigureQueueCommand(
    ConfigureQueueCommand&& original) noexcept
: d_subscriptions(bsl::move(original.d_subscriptions)),
  d_uri(bsl::move(original.d_uri)),
  d_maxUnconfirmedMessages(bsl::move(original.d_maxUnconfirmedMessages)),
  d_maxUnconfirmedBytes(bsl::move(original.d_maxUnconfirmedBytes)),
  d_consumerPriority(bsl::move(original.d_consumerPriority)),
  d_async(bsl::move(original.d_async))
{
}

ConfigureQueueCommand::ConfigureQueueCommand(ConfigureQueueCommand&& original,
                                             bslma::Allocator* basicAllocator)
: d_subscriptions(bsl::move(original.d_subscriptions), basicAllocator)
, d_uri(bsl::move(original.d_uri), basicAllocator)
, d_maxUnconfirmedMessages(bsl::move(original.d_maxUnconfirmedMessages))
, d_maxUnconfirmedBytes(bsl::move(original.d_maxUnconfirmedBytes))
, d_consumerPriority(bsl::move(original.d_consumerPriority))
, d_async(bsl::move(original.d_async))
{
}
#endif

ConfigureQueueCommand::~ConfigureQueueCommand()
{
}

// MANIPULATORS

ConfigureQueueCommand&
ConfigureQueueCommand::operator=(const ConfigureQueueCommand& rhs)
{
    if (this != &rhs) {
        d_uri                    = rhs.d_uri;
        d_async                  = rhs.d_async;
        d_maxUnconfirmedMessages = rhs.d_maxUnconfirmedMessages;
        d_maxUnconfirmedBytes    = rhs.d_maxUnconfirmedBytes;
        d_consumerPriority       = rhs.d_consumerPriority;
        d_subscriptions          = rhs.d_subscriptions;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureQueueCommand&
ConfigureQueueCommand::operator=(ConfigureQueueCommand&& rhs)
{
    if (this != &rhs) {
        d_uri                    = bsl::move(rhs.d_uri);
        d_async                  = bsl::move(rhs.d_async);
        d_maxUnconfirmedMessages = bsl::move(rhs.d_maxUnconfirmedMessages);
        d_maxUnconfirmedBytes    = bsl::move(rhs.d_maxUnconfirmedBytes);
        d_consumerPriority       = bsl::move(rhs.d_consumerPriority);
        d_subscriptions          = bsl::move(rhs.d_subscriptions);
    }

    return *this;
}
#endif

void ConfigureQueueCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    d_async                  = DEFAULT_INITIALIZER_ASYNC;
    d_maxUnconfirmedMessages = DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES;
    d_maxUnconfirmedBytes    = DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES;
    d_consumerPriority       = DEFAULT_INITIALIZER_CONSUMER_PRIORITY;
    bdlat_ValueTypeFunctions::reset(&d_subscriptions);
}

// ACCESSORS

bsl::ostream& ConfigureQueueCommand::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("async", this->async());
    printer.printAttribute("maxUnconfirmedMessages",
                           this->maxUnconfirmedMessages());
    printer.printAttribute("maxUnconfirmedBytes", this->maxUnconfirmedBytes());
    printer.printAttribute("consumerPriority", this->consumerPriority());
    printer.printAttribute("subscriptions", this->subscriptions());
    printer.end();
    return stream;
}

// -----------------
// class DataCommand
// -----------------

// CONSTANTS

const char DataCommand::CLASS_NAME[] = "DataCommand";

const bdlat_AttributeInfo DataCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo* DataCommand::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    if (bdlb::String::areEqualCaseless("n", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("next", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("p", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("prev", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("r", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("record", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("list", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("l", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DataCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DataCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

DataCommand::DataCommand()
: d_choice()
{
}

// MANIPULATORS

void DataCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream&
DataCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// --------------------------
// class JournalCommandChoice
// --------------------------

// CONSTANTS

const char JournalCommandChoice::CLASS_NAME[] = "JournalCommandChoice";

const bdlat_SelectionInfo JournalCommandChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_N, "n", sizeof("n") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_NEXT,
     "next",
     sizeof("next") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_P, "p", sizeof("p") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_PREV,
     "prev",
     sizeof("prev") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_R, "r", sizeof("r") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_RECORD,
     "record",
     sizeof("record") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_LIST,
     "list",
     sizeof("list") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_L, "l", sizeof("l") - 1, "", bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_DUMP,
     "dump",
     sizeof("dump") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {SELECTION_ID_TYPE,
     "type",
     sizeof("type") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
JournalCommandChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 10; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            JournalCommandChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* JournalCommandChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_N: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_N];
    case SELECTION_ID_NEXT: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT];
    case SELECTION_ID_P: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_P];
    case SELECTION_ID_PREV: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV];
    case SELECTION_ID_R: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_R];
    case SELECTION_ID_RECORD:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD];
    case SELECTION_ID_LIST: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST];
    case SELECTION_ID_L: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_L];
    case SELECTION_ID_DUMP: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP];
    case SELECTION_ID_TYPE: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TYPE];
    default: return 0;
    }
}

// CREATORS

JournalCommandChoice::JournalCommandChoice(
    const JournalCommandChoice& original,
    bslma::Allocator*           basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        new (d_n.buffer()) bsls::Types::Uint64(original.d_n.object());
    } break;
    case SELECTION_ID_NEXT: {
        new (d_next.buffer()) bsls::Types::Uint64(original.d_next.object());
    } break;
    case SELECTION_ID_P: {
        new (d_p.buffer()) bsls::Types::Uint64(original.d_p.object());
    } break;
    case SELECTION_ID_PREV: {
        new (d_prev.buffer()) bsls::Types::Uint64(original.d_prev.object());
    } break;
    case SELECTION_ID_R: {
        new (d_r.buffer()) bsls::Types::Uint64(original.d_r.object());
    } break;
    case SELECTION_ID_RECORD: {
        new (d_record.buffer())
            bsls::Types::Uint64(original.d_record.object());
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) int(original.d_list.object());
    } break;
    case SELECTION_ID_L: {
        new (d_l.buffer()) int(original.d_l.object());
    } break;
    case SELECTION_ID_DUMP: {
        new (d_dump.buffer())
            bsl::string(original.d_dump.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TYPE: {
        new (d_type.buffer())
            JournalCommandChoiceType::Value(original.d_type.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
JournalCommandChoice::JournalCommandChoice(JournalCommandChoice&& original)
    noexcept : d_selectionId(original.d_selectionId),
               d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        new (d_n.buffer())
            bsls::Types::Uint64(bsl::move(original.d_n.object()));
    } break;
    case SELECTION_ID_NEXT: {
        new (d_next.buffer())
            bsls::Types::Uint64(bsl::move(original.d_next.object()));
    } break;
    case SELECTION_ID_P: {
        new (d_p.buffer())
            bsls::Types::Uint64(bsl::move(original.d_p.object()));
    } break;
    case SELECTION_ID_PREV: {
        new (d_prev.buffer())
            bsls::Types::Uint64(bsl::move(original.d_prev.object()));
    } break;
    case SELECTION_ID_R: {
        new (d_r.buffer())
            bsls::Types::Uint64(bsl::move(original.d_r.object()));
    } break;
    case SELECTION_ID_RECORD: {
        new (d_record.buffer())
            bsls::Types::Uint64(bsl::move(original.d_record.object()));
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) int(bsl::move(original.d_list.object()));
    } break;
    case SELECTION_ID_L: {
        new (d_l.buffer()) int(bsl::move(original.d_l.object()));
    } break;
    case SELECTION_ID_DUMP: {
        new (d_dump.buffer())
            bsl::string(bsl::move(original.d_dump.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TYPE: {
        new (d_type.buffer()) JournalCommandChoiceType::Value(
            bsl::move(original.d_type.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

JournalCommandChoice::JournalCommandChoice(JournalCommandChoice&& original,
                                           bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        new (d_n.buffer())
            bsls::Types::Uint64(bsl::move(original.d_n.object()));
    } break;
    case SELECTION_ID_NEXT: {
        new (d_next.buffer())
            bsls::Types::Uint64(bsl::move(original.d_next.object()));
    } break;
    case SELECTION_ID_P: {
        new (d_p.buffer())
            bsls::Types::Uint64(bsl::move(original.d_p.object()));
    } break;
    case SELECTION_ID_PREV: {
        new (d_prev.buffer())
            bsls::Types::Uint64(bsl::move(original.d_prev.object()));
    } break;
    case SELECTION_ID_R: {
        new (d_r.buffer())
            bsls::Types::Uint64(bsl::move(original.d_r.object()));
    } break;
    case SELECTION_ID_RECORD: {
        new (d_record.buffer())
            bsls::Types::Uint64(bsl::move(original.d_record.object()));
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) int(bsl::move(original.d_list.object()));
    } break;
    case SELECTION_ID_L: {
        new (d_l.buffer()) int(bsl::move(original.d_l.object()));
    } break;
    case SELECTION_ID_DUMP: {
        new (d_dump.buffer())
            bsl::string(bsl::move(original.d_dump.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TYPE: {
        new (d_type.buffer()) JournalCommandChoiceType::Value(
            bsl::move(original.d_type.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

JournalCommandChoice&
JournalCommandChoice::operator=(const JournalCommandChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_N: {
            makeN(rhs.d_n.object());
        } break;
        case SELECTION_ID_NEXT: {
            makeNext(rhs.d_next.object());
        } break;
        case SELECTION_ID_P: {
            makeP(rhs.d_p.object());
        } break;
        case SELECTION_ID_PREV: {
            makePrev(rhs.d_prev.object());
        } break;
        case SELECTION_ID_R: {
            makeR(rhs.d_r.object());
        } break;
        case SELECTION_ID_RECORD: {
            makeRecord(rhs.d_record.object());
        } break;
        case SELECTION_ID_LIST: {
            makeList(rhs.d_list.object());
        } break;
        case SELECTION_ID_L: {
            makeL(rhs.d_l.object());
        } break;
        case SELECTION_ID_DUMP: {
            makeDump(rhs.d_dump.object());
        } break;
        case SELECTION_ID_TYPE: {
            makeType(rhs.d_type.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
JournalCommandChoice&
JournalCommandChoice::operator=(JournalCommandChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_N: {
            makeN(bsl::move(rhs.d_n.object()));
        } break;
        case SELECTION_ID_NEXT: {
            makeNext(bsl::move(rhs.d_next.object()));
        } break;
        case SELECTION_ID_P: {
            makeP(bsl::move(rhs.d_p.object()));
        } break;
        case SELECTION_ID_PREV: {
            makePrev(bsl::move(rhs.d_prev.object()));
        } break;
        case SELECTION_ID_R: {
            makeR(bsl::move(rhs.d_r.object()));
        } break;
        case SELECTION_ID_RECORD: {
            makeRecord(bsl::move(rhs.d_record.object()));
        } break;
        case SELECTION_ID_LIST: {
            makeList(bsl::move(rhs.d_list.object()));
        } break;
        case SELECTION_ID_L: {
            makeL(bsl::move(rhs.d_l.object()));
        } break;
        case SELECTION_ID_DUMP: {
            makeDump(bsl::move(rhs.d_dump.object()));
        } break;
        case SELECTION_ID_TYPE: {
            makeType(bsl::move(rhs.d_type.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void JournalCommandChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        // no destruction required
    } break;
    case SELECTION_ID_NEXT: {
        // no destruction required
    } break;
    case SELECTION_ID_P: {
        // no destruction required
    } break;
    case SELECTION_ID_PREV: {
        // no destruction required
    } break;
    case SELECTION_ID_R: {
        // no destruction required
    } break;
    case SELECTION_ID_RECORD: {
        // no destruction required
    } break;
    case SELECTION_ID_LIST: {
        // no destruction required
    } break;
    case SELECTION_ID_L: {
        // no destruction required
    } break;
    case SELECTION_ID_DUMP: {
        typedef bsl::string Type;
        d_dump.object().~Type();
    } break;
    case SELECTION_ID_TYPE: {
        typedef JournalCommandChoiceType::Value Type;
        d_type.object().~Type();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int JournalCommandChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_N: {
        makeN();
    } break;
    case SELECTION_ID_NEXT: {
        makeNext();
    } break;
    case SELECTION_ID_P: {
        makeP();
    } break;
    case SELECTION_ID_PREV: {
        makePrev();
    } break;
    case SELECTION_ID_R: {
        makeR();
    } break;
    case SELECTION_ID_RECORD: {
        makeRecord();
    } break;
    case SELECTION_ID_LIST: {
        makeList();
    } break;
    case SELECTION_ID_L: {
        makeL();
    } break;
    case SELECTION_ID_DUMP: {
        makeDump();
    } break;
    case SELECTION_ID_TYPE: {
        makeType();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int JournalCommandChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

bsls::Types::Uint64& JournalCommandChoice::makeN()
{
    if (SELECTION_ID_N == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_n.object());
    }
    else {
        reset();
        new (d_n.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_N;
    }

    return d_n.object();
}

bsls::Types::Uint64& JournalCommandChoice::makeN(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_N == d_selectionId) {
        d_n.object() = value;
    }
    else {
        reset();
        new (d_n.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_N;
    }

    return d_n.object();
}

bsls::Types::Uint64& JournalCommandChoice::makeNext()
{
    if (SELECTION_ID_NEXT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_next.object());
    }
    else {
        reset();
        new (d_next.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_NEXT;
    }

    return d_next.object();
}

bsls::Types::Uint64& JournalCommandChoice::makeNext(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_NEXT == d_selectionId) {
        d_next.object() = value;
    }
    else {
        reset();
        new (d_next.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_NEXT;
    }

    return d_next.object();
}

bsls::Types::Uint64& JournalCommandChoice::makeP()
{
    if (SELECTION_ID_P == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_p.object());
    }
    else {
        reset();
        new (d_p.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_P;
    }

    return d_p.object();
}

bsls::Types::Uint64& JournalCommandChoice::makeP(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_P == d_selectionId) {
        d_p.object() = value;
    }
    else {
        reset();
        new (d_p.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_P;
    }

    return d_p.object();
}

bsls::Types::Uint64& JournalCommandChoice::makePrev()
{
    if (SELECTION_ID_PREV == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_prev.object());
    }
    else {
        reset();
        new (d_prev.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_PREV;
    }

    return d_prev.object();
}

bsls::Types::Uint64& JournalCommandChoice::makePrev(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_PREV == d_selectionId) {
        d_prev.object() = value;
    }
    else {
        reset();
        new (d_prev.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_PREV;
    }

    return d_prev.object();
}

bsls::Types::Uint64& JournalCommandChoice::makeR()
{
    if (SELECTION_ID_R == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_r.object());
    }
    else {
        reset();
        new (d_r.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_R;
    }

    return d_r.object();
}

bsls::Types::Uint64& JournalCommandChoice::makeR(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_R == d_selectionId) {
        d_r.object() = value;
    }
    else {
        reset();
        new (d_r.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_R;
    }

    return d_r.object();
}

bsls::Types::Uint64& JournalCommandChoice::makeRecord()
{
    if (SELECTION_ID_RECORD == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_record.object());
    }
    else {
        reset();
        new (d_record.buffer()) bsls::Types::Uint64();
        d_selectionId = SELECTION_ID_RECORD;
    }

    return d_record.object();
}

bsls::Types::Uint64&
JournalCommandChoice::makeRecord(bsls::Types::Uint64 value)
{
    if (SELECTION_ID_RECORD == d_selectionId) {
        d_record.object() = value;
    }
    else {
        reset();
        new (d_record.buffer()) bsls::Types::Uint64(value);
        d_selectionId = SELECTION_ID_RECORD;
    }

    return d_record.object();
}

int& JournalCommandChoice::makeList()
{
    if (SELECTION_ID_LIST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_list.object());
    }
    else {
        reset();
        new (d_list.buffer()) int();
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

int& JournalCommandChoice::makeList(int value)
{
    if (SELECTION_ID_LIST == d_selectionId) {
        d_list.object() = value;
    }
    else {
        reset();
        new (d_list.buffer()) int(value);
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

int& JournalCommandChoice::makeL()
{
    if (SELECTION_ID_L == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_l.object());
    }
    else {
        reset();
        new (d_l.buffer()) int();
        d_selectionId = SELECTION_ID_L;
    }

    return d_l.object();
}

int& JournalCommandChoice::makeL(int value)
{
    if (SELECTION_ID_L == d_selectionId) {
        d_l.object() = value;
    }
    else {
        reset();
        new (d_l.buffer()) int(value);
        d_selectionId = SELECTION_ID_L;
    }

    return d_l.object();
}

bsl::string& JournalCommandChoice::makeDump()
{
    if (SELECTION_ID_DUMP == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_dump.object());
    }
    else {
        reset();
        new (d_dump.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_DUMP;
    }

    return d_dump.object();
}

bsl::string& JournalCommandChoice::makeDump(const bsl::string& value)
{
    if (SELECTION_ID_DUMP == d_selectionId) {
        d_dump.object() = value;
    }
    else {
        reset();
        new (d_dump.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DUMP;
    }

    return d_dump.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& JournalCommandChoice::makeDump(bsl::string&& value)
{
    if (SELECTION_ID_DUMP == d_selectionId) {
        d_dump.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_dump.buffer()) bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DUMP;
    }

    return d_dump.object();
}
#endif

JournalCommandChoiceType::Value& JournalCommandChoice::makeType()
{
    if (SELECTION_ID_TYPE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_type.object());
    }
    else {
        reset();
        new (d_type.buffer()) JournalCommandChoiceType::Value(
            static_cast<JournalCommandChoiceType::Value>(0));
        d_selectionId = SELECTION_ID_TYPE;
    }

    return d_type.object();
}

JournalCommandChoiceType::Value&
JournalCommandChoice::makeType(JournalCommandChoiceType::Value value)
{
    if (SELECTION_ID_TYPE == d_selectionId) {
        d_type.object() = value;
    }
    else {
        reset();
        new (d_type.buffer()) JournalCommandChoiceType::Value(value);
        d_selectionId = SELECTION_ID_TYPE;
    }

    return d_type.object();
}

// ACCESSORS

bsl::ostream& JournalCommandChoice::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_N: {
        printer.printAttribute("n", d_n.object());
    } break;
    case SELECTION_ID_NEXT: {
        printer.printAttribute("next", d_next.object());
    } break;
    case SELECTION_ID_P: {
        printer.printAttribute("p", d_p.object());
    } break;
    case SELECTION_ID_PREV: {
        printer.printAttribute("prev", d_prev.object());
    } break;
    case SELECTION_ID_R: {
        printer.printAttribute("r", d_r.object());
    } break;
    case SELECTION_ID_RECORD: {
        printer.printAttribute("record", d_record.object());
    } break;
    case SELECTION_ID_LIST: {
        printer.printAttribute("list", d_list.object());
    } break;
    case SELECTION_ID_L: {
        printer.printAttribute("l", d_l.object());
    } break;
    case SELECTION_ID_DUMP: {
        printer.printAttribute("dump", d_dump.object());
    } break;
    case SELECTION_ID_TYPE: {
        printer.printAttribute("type", d_type.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* JournalCommandChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_N: return SELECTION_INFO_ARRAY[SELECTION_INDEX_N].name();
    case SELECTION_ID_NEXT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT].name();
    case SELECTION_ID_P: return SELECTION_INFO_ARRAY[SELECTION_INDEX_P].name();
    case SELECTION_ID_PREV:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV].name();
    case SELECTION_ID_R: return SELECTION_INFO_ARRAY[SELECTION_INDEX_R].name();
    case SELECTION_ID_RECORD:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD].name();
    case SELECTION_ID_LIST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST].name();
    case SELECTION_ID_L: return SELECTION_INFO_ARRAY[SELECTION_INDEX_L].name();
    case SELECTION_ID_DUMP:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP].name();
    case SELECTION_ID_TYPE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TYPE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ---------------------
// class MessageProperty
// ---------------------

// CONSTANTS

const char MessageProperty::CLASS_NAME[] = "MessageProperty";

const MessagePropertyType::Value MessageProperty::DEFAULT_INITIALIZER_TYPE =
    MessagePropertyType::E_STRING;

const bdlat_AttributeInfo MessageProperty::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_VALUE,
     "value",
     sizeof("value") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_TYPE,
     "type",
     sizeof("type") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_DEFAULT_VALUE}};

// CLASS METHODS

const bdlat_AttributeInfo*
MessageProperty::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            MessageProperty::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* MessageProperty::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_VALUE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE];
    case ATTRIBUTE_ID_TYPE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE];
    default: return 0;
    }
}

// CREATORS

MessageProperty::MessageProperty(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_value(basicAllocator)
, d_type(DEFAULT_INITIALIZER_TYPE)
{
}

MessageProperty::MessageProperty(const MessageProperty& original,
                                 bslma::Allocator*      basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_value(original.d_value, basicAllocator)
, d_type(original.d_type)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
MessageProperty::MessageProperty(MessageProperty&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_value(bsl::move(original.d_value)),
  d_type(bsl::move(original.d_type))
{
}

MessageProperty::MessageProperty(MessageProperty&& original,
                                 bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_value(bsl::move(original.d_value), basicAllocator)
, d_type(bsl::move(original.d_type))
{
}
#endif

MessageProperty::~MessageProperty()
{
}

// MANIPULATORS

MessageProperty& MessageProperty::operator=(const MessageProperty& rhs)
{
    if (this != &rhs) {
        d_name  = rhs.d_name;
        d_value = rhs.d_value;
        d_type  = rhs.d_type;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
MessageProperty& MessageProperty::operator=(MessageProperty&& rhs)
{
    if (this != &rhs) {
        d_name  = bsl::move(rhs.d_name);
        d_value = bsl::move(rhs.d_value);
        d_type  = bsl::move(rhs.d_type);
    }

    return *this;
}
#endif

void MessageProperty::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_value);
    d_type = DEFAULT_INITIALIZER_TYPE;
}

// ACCESSORS

bsl::ostream& MessageProperty::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("value", this->value());
    printer.printAttribute("type", this->type());
    printer.end();
    return stream;
}

// ----------------------
// class OpenQueueCommand
// ----------------------

// CONSTANTS

const char OpenQueueCommand::CLASS_NAME[] = "OpenQueueCommand";

const bool OpenQueueCommand::DEFAULT_INITIALIZER_ASYNC = false;

const int OpenQueueCommand::DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES =
    1024;

const int OpenQueueCommand::DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES =
    33554432;

const int OpenQueueCommand::DEFAULT_INITIALIZER_CONSUMER_PRIORITY = 0;

const bdlat_AttributeInfo OpenQueueCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_FLAGS,
     "flags",
     sizeof("flags") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ASYNC,
     "async",
     sizeof("async") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES,
     "maxUnconfirmedMessages",
     sizeof("maxUnconfirmedMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES,
     "maxUnconfirmedBytes",
     sizeof("maxUnconfirmedBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_CONSUMER_PRIORITY,
     "consumerPriority",
     sizeof("consumerPriority") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_SUBSCRIPTIONS,
     "subscriptions",
     sizeof("subscriptions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
OpenQueueCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 7; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            OpenQueueCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* OpenQueueCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_FLAGS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS];
    case ATTRIBUTE_ID_ASYNC:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES];
    case ATTRIBUTE_ID_CONSUMER_PRIORITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY];
    case ATTRIBUTE_ID_SUBSCRIPTIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS];
    default: return 0;
    }
}

// CREATORS

OpenQueueCommand::OpenQueueCommand(bslma::Allocator* basicAllocator)
: d_subscriptions(basicAllocator)
, d_uri(basicAllocator)
, d_flags(basicAllocator)
, d_maxUnconfirmedMessages(DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES)
, d_maxUnconfirmedBytes(DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES)
, d_consumerPriority(DEFAULT_INITIALIZER_CONSUMER_PRIORITY)
, d_async(DEFAULT_INITIALIZER_ASYNC)
{
}

OpenQueueCommand::OpenQueueCommand(const OpenQueueCommand& original,
                                   bslma::Allocator*       basicAllocator)
: d_subscriptions(original.d_subscriptions, basicAllocator)
, d_uri(original.d_uri, basicAllocator)
, d_flags(original.d_flags, basicAllocator)
, d_maxUnconfirmedMessages(original.d_maxUnconfirmedMessages)
, d_maxUnconfirmedBytes(original.d_maxUnconfirmedBytes)
, d_consumerPriority(original.d_consumerPriority)
, d_async(original.d_async)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenQueueCommand::OpenQueueCommand(OpenQueueCommand&& original) noexcept
: d_subscriptions(bsl::move(original.d_subscriptions)),
  d_uri(bsl::move(original.d_uri)),
  d_flags(bsl::move(original.d_flags)),
  d_maxUnconfirmedMessages(bsl::move(original.d_maxUnconfirmedMessages)),
  d_maxUnconfirmedBytes(bsl::move(original.d_maxUnconfirmedBytes)),
  d_consumerPriority(bsl::move(original.d_consumerPriority)),
  d_async(bsl::move(original.d_async))
{
}

OpenQueueCommand::OpenQueueCommand(OpenQueueCommand&& original,
                                   bslma::Allocator*  basicAllocator)
: d_subscriptions(bsl::move(original.d_subscriptions), basicAllocator)
, d_uri(bsl::move(original.d_uri), basicAllocator)
, d_flags(bsl::move(original.d_flags), basicAllocator)
, d_maxUnconfirmedMessages(bsl::move(original.d_maxUnconfirmedMessages))
, d_maxUnconfirmedBytes(bsl::move(original.d_maxUnconfirmedBytes))
, d_consumerPriority(bsl::move(original.d_consumerPriority))
, d_async(bsl::move(original.d_async))
{
}
#endif

OpenQueueCommand::~OpenQueueCommand()
{
}

// MANIPULATORS

OpenQueueCommand& OpenQueueCommand::operator=(const OpenQueueCommand& rhs)
{
    if (this != &rhs) {
        d_uri                    = rhs.d_uri;
        d_flags                  = rhs.d_flags;
        d_async                  = rhs.d_async;
        d_maxUnconfirmedMessages = rhs.d_maxUnconfirmedMessages;
        d_maxUnconfirmedBytes    = rhs.d_maxUnconfirmedBytes;
        d_consumerPriority       = rhs.d_consumerPriority;
        d_subscriptions          = rhs.d_subscriptions;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenQueueCommand& OpenQueueCommand::operator=(OpenQueueCommand&& rhs)
{
    if (this != &rhs) {
        d_uri                    = bsl::move(rhs.d_uri);
        d_flags                  = bsl::move(rhs.d_flags);
        d_async                  = bsl::move(rhs.d_async);
        d_maxUnconfirmedMessages = bsl::move(rhs.d_maxUnconfirmedMessages);
        d_maxUnconfirmedBytes    = bsl::move(rhs.d_maxUnconfirmedBytes);
        d_consumerPriority       = bsl::move(rhs.d_consumerPriority);
        d_subscriptions          = bsl::move(rhs.d_subscriptions);
    }

    return *this;
}
#endif

void OpenQueueCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_flags);
    d_async                  = DEFAULT_INITIALIZER_ASYNC;
    d_maxUnconfirmedMessages = DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES;
    d_maxUnconfirmedBytes    = DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES;
    d_consumerPriority       = DEFAULT_INITIALIZER_CONSUMER_PRIORITY;
    bdlat_ValueTypeFunctions::reset(&d_subscriptions);
}

// ACCESSORS

bsl::ostream& OpenQueueCommand::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("flags", this->flags());
    printer.printAttribute("async", this->async());
    printer.printAttribute("maxUnconfirmedMessages",
                           this->maxUnconfirmedMessages());
    printer.printAttribute("maxUnconfirmedBytes", this->maxUnconfirmedBytes());
    printer.printAttribute("consumerPriority", this->consumerPriority());
    printer.printAttribute("subscriptions", this->subscriptions());
    printer.end();
    return stream;
}

// ------------------
// class QlistCommand
// ------------------

// CONSTANTS

const char QlistCommand::CLASS_NAME[] = "QlistCommand";

const bdlat_AttributeInfo QlistCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo* QlistCommand::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    if (bdlb::String::areEqualCaseless("n", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("next", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("p", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("prev", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("r", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("record", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("list", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("l", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QlistCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QlistCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

QlistCommand::QlistCommand()
: d_choice()
{
}

// MANIPULATORS

void QlistCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream&
QlistCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// ---------------------------
// class CommandLineParameters
// ---------------------------

// CONSTANTS

const char CommandLineParameters::CLASS_NAME[] = "CommandLineParameters";

const char CommandLineParameters::DEFAULT_INITIALIZER_MODE[] = "cli";

const char CommandLineParameters::DEFAULT_INITIALIZER_BROKER[] =
    "tcp://localhost:30114";

const char CommandLineParameters::DEFAULT_INITIALIZER_QUEUE_URI[] = "";

const char CommandLineParameters::DEFAULT_INITIALIZER_QUEUE_FLAGS[] = "";

const char CommandLineParameters::DEFAULT_INITIALIZER_LATENCY[] = "none";

const char CommandLineParameters::DEFAULT_INITIALIZER_LATENCY_REPORT[] = "";

const bool CommandLineParameters::DEFAULT_INITIALIZER_DUMP_MSG = false;

const bool CommandLineParameters::DEFAULT_INITIALIZER_CONFIRM_MSG = false;

const bsls::Types::Int64
    CommandLineParameters::DEFAULT_INITIALIZER_EVENT_SIZE = 1;

const int CommandLineParameters::DEFAULT_INITIALIZER_MSG_SIZE = 1024;

const int CommandLineParameters::DEFAULT_INITIALIZER_POST_RATE = 1;

const char CommandLineParameters::DEFAULT_INITIALIZER_EVENTS_COUNT[] = "0";

const char CommandLineParameters::DEFAULT_INITIALIZER_MAX_UNCONFIRMED[] =
    "1024:33554432";

const int CommandLineParameters::DEFAULT_INITIALIZER_POST_INTERVAL = 1000;

const char CommandLineParameters::DEFAULT_INITIALIZER_VERBOSITY[] = "info";

const char CommandLineParameters::DEFAULT_INITIALIZER_LOG_FORMAT[] =
    "%d (%t) %s %F:%l %m\n";

const bool CommandLineParameters::DEFAULT_INITIALIZER_MEMORY_DEBUG = false;

const int CommandLineParameters::DEFAULT_INITIALIZER_THREADS = 1;

const int CommandLineParameters::DEFAULT_INITIALIZER_SHUTDOWN_GRACE = 0;

const bool
    CommandLineParameters::DEFAULT_INITIALIZER_NO_SESSION_EVENT_HANDLER =
        false;

const char CommandLineParameters::DEFAULT_INITIALIZER_STORAGE[] = "";

const char CommandLineParameters::DEFAULT_INITIALIZER_LOG[] = "";

const char
    CommandLineParameters::DEFAULT_INITIALIZER_SEQUENTIAL_MESSAGE_PATTERN[] =
        "";

const int CommandLineParameters::DEFAULT_INITIALIZER_AUTO_PUB_SUB_MODULO = 0;

const int CommandLineParameters::DEFAULT_INITIALIZER_TIMEOUT_SEC = 300;

const bdlat_AttributeInfo CommandLineParameters::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_MODE,
     "mode",
     sizeof("mode") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_BROKER,
     "broker",
     sizeof("broker") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_QUEUE_URI,
     "queueUri",
     sizeof("queueUri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_QUEUE_FLAGS,
     "queueFlags",
     sizeof("queueFlags") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_LATENCY,
     "latency",
     sizeof("latency") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_LATENCY_REPORT,
     "latencyReport",
     sizeof("latencyReport") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_DUMP_MSG,
     "dumpMsg",
     sizeof("dumpMsg") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_CONFIRM_MSG,
     "confirmMsg",
     sizeof("confirmMsg") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_EVENT_SIZE,
     "eventSize",
     sizeof("eventSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_MSG_SIZE,
     "msgSize",
     sizeof("msgSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_POST_RATE,
     "postRate",
     sizeof("postRate") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_EVENTS_COUNT,
     "eventsCount",
     sizeof("eventsCount") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_MAX_UNCONFIRMED,
     "maxUnconfirmed",
     sizeof("maxUnconfirmed") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_POST_INTERVAL,
     "postInterval",
     sizeof("postInterval") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_VERBOSITY,
     "verbosity",
     sizeof("verbosity") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_LOG_FORMAT,
     "logFormat",
     sizeof("logFormat") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_MEMORY_DEBUG,
     "memoryDebug",
     sizeof("memoryDebug") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_THREADS,
     "threads",
     sizeof("threads") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_SHUTDOWN_GRACE,
     "shutdownGrace",
     sizeof("shutdownGrace") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_NO_SESSION_EVENT_HANDLER,
     "noSessionEventHandler",
     sizeof("noSessionEventHandler") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_STORAGE,
     "storage",
     sizeof("storage") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_LOG,
     "log",
     sizeof("log") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_SEQUENTIAL_MESSAGE_PATTERN,
     "sequentialMessagePattern",
     sizeof("sequentialMessagePattern") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_MESSAGE_PROPERTIES,
     "messageProperties",
     sizeof("messageProperties") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_SUBSCRIPTIONS,
     "subscriptions",
     sizeof("subscriptions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_AUTO_PUB_SUB_MODULO,
     "autoPubSubModulo",
     sizeof("autoPubSubModulo") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_TIMEOUT_SEC,
     "timeoutSec",
     sizeof("timeoutSec") - 1,
     "",
     bdlat_FormattingMode::e_DEC | bdlat_FormattingMode::e_DEFAULT_VALUE}};

// CLASS METHODS

const bdlat_AttributeInfo*
CommandLineParameters::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 27; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            CommandLineParameters::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* CommandLineParameters::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MODE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE];
    case ATTRIBUTE_ID_BROKER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER];
    case ATTRIBUTE_ID_QUEUE_URI:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URI];
    case ATTRIBUTE_ID_QUEUE_FLAGS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_FLAGS];
    case ATTRIBUTE_ID_LATENCY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY];
    case ATTRIBUTE_ID_LATENCY_REPORT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_REPORT];
    case ATTRIBUTE_ID_DUMP_MSG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_MSG];
    case ATTRIBUTE_ID_CONFIRM_MSG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIRM_MSG];
    case ATTRIBUTE_ID_EVENT_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE];
    case ATTRIBUTE_ID_MSG_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE];
    case ATTRIBUTE_ID_POST_RATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE];
    case ATTRIBUTE_ID_EVENTS_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED];
    case ATTRIBUTE_ID_POST_INTERVAL:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL];
    case ATTRIBUTE_ID_VERBOSITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY];
    case ATTRIBUTE_ID_LOG_FORMAT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT];
    case ATTRIBUTE_ID_MEMORY_DEBUG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MEMORY_DEBUG];
    case ATTRIBUTE_ID_THREADS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THREADS];
    case ATTRIBUTE_ID_SHUTDOWN_GRACE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_GRACE];
    case ATTRIBUTE_ID_NO_SESSION_EVENT_HANDLER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NO_SESSION_EVENT_HANDLER];
    case ATTRIBUTE_ID_STORAGE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE];
    case ATTRIBUTE_ID_LOG: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG];
    case ATTRIBUTE_ID_SEQUENTIAL_MESSAGE_PATTERN:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_SEQUENTIAL_MESSAGE_PATTERN];
    case ATTRIBUTE_ID_MESSAGE_PROPERTIES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES];
    case ATTRIBUTE_ID_SUBSCRIPTIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS];
    case ATTRIBUTE_ID_AUTO_PUB_SUB_MODULO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTO_PUB_SUB_MODULO];
    case ATTRIBUTE_ID_TIMEOUT_SEC:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMEOUT_SEC];
    default: return 0;
    }
}

// CREATORS

CommandLineParameters::CommandLineParameters(bslma::Allocator* basicAllocator)
: d_eventSize(DEFAULT_INITIALIZER_EVENT_SIZE)
, d_subscriptions(basicAllocator)
, d_messageProperties(basicAllocator)
, d_mode(DEFAULT_INITIALIZER_MODE, basicAllocator)
, d_broker(DEFAULT_INITIALIZER_BROKER, basicAllocator)
, d_queueUri(DEFAULT_INITIALIZER_QUEUE_URI, basicAllocator)
, d_queueFlags(DEFAULT_INITIALIZER_QUEUE_FLAGS, basicAllocator)
, d_latency(DEFAULT_INITIALIZER_LATENCY, basicAllocator)
, d_latencyReport(DEFAULT_INITIALIZER_LATENCY_REPORT, basicAllocator)
, d_eventsCount(DEFAULT_INITIALIZER_EVENTS_COUNT, basicAllocator)
, d_maxUnconfirmed(DEFAULT_INITIALIZER_MAX_UNCONFIRMED, basicAllocator)
, d_verbosity(DEFAULT_INITIALIZER_VERBOSITY, basicAllocator)
, d_logFormat(DEFAULT_INITIALIZER_LOG_FORMAT, basicAllocator)
, d_storage(DEFAULT_INITIALIZER_STORAGE, basicAllocator)
, d_log(DEFAULT_INITIALIZER_LOG, basicAllocator)
, d_sequentialMessagePattern(DEFAULT_INITIALIZER_SEQUENTIAL_MESSAGE_PATTERN,
                             basicAllocator)
, d_msgSize(DEFAULT_INITIALIZER_MSG_SIZE)
, d_postRate(DEFAULT_INITIALIZER_POST_RATE)
, d_postInterval(DEFAULT_INITIALIZER_POST_INTERVAL)
, d_threads(DEFAULT_INITIALIZER_THREADS)
, d_shutdownGrace(DEFAULT_INITIALIZER_SHUTDOWN_GRACE)
, d_autoPubSubModulo(DEFAULT_INITIALIZER_AUTO_PUB_SUB_MODULO)
, d_timeoutSec(DEFAULT_INITIALIZER_TIMEOUT_SEC)
, d_dumpMsg(DEFAULT_INITIALIZER_DUMP_MSG)
, d_confirmMsg(DEFAULT_INITIALIZER_CONFIRM_MSG)
, d_memoryDebug(DEFAULT_INITIALIZER_MEMORY_DEBUG)
, d_noSessionEventHandler(DEFAULT_INITIALIZER_NO_SESSION_EVENT_HANDLER)
{
}

CommandLineParameters::CommandLineParameters(
    const CommandLineParameters& original,
    bslma::Allocator*            basicAllocator)
: d_eventSize(original.d_eventSize)
, d_subscriptions(original.d_subscriptions, basicAllocator)
, d_messageProperties(original.d_messageProperties, basicAllocator)
, d_mode(original.d_mode, basicAllocator)
, d_broker(original.d_broker, basicAllocator)
, d_queueUri(original.d_queueUri, basicAllocator)
, d_queueFlags(original.d_queueFlags, basicAllocator)
, d_latency(original.d_latency, basicAllocator)
, d_latencyReport(original.d_latencyReport, basicAllocator)
, d_eventsCount(original.d_eventsCount, basicAllocator)
, d_maxUnconfirmed(original.d_maxUnconfirmed, basicAllocator)
, d_verbosity(original.d_verbosity, basicAllocator)
, d_logFormat(original.d_logFormat, basicAllocator)
, d_storage(original.d_storage, basicAllocator)
, d_log(original.d_log, basicAllocator)
, d_sequentialMessagePattern(original.d_sequentialMessagePattern,
                             basicAllocator)
, d_msgSize(original.d_msgSize)
, d_postRate(original.d_postRate)
, d_postInterval(original.d_postInterval)
, d_threads(original.d_threads)
, d_shutdownGrace(original.d_shutdownGrace)
, d_autoPubSubModulo(original.d_autoPubSubModulo)
, d_timeoutSec(original.d_timeoutSec)
, d_dumpMsg(original.d_dumpMsg)
, d_confirmMsg(original.d_confirmMsg)
, d_memoryDebug(original.d_memoryDebug)
, d_noSessionEventHandler(original.d_noSessionEventHandler)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CommandLineParameters::CommandLineParameters(
    CommandLineParameters&& original) noexcept
: d_eventSize(bsl::move(original.d_eventSize)),
  d_subscriptions(bsl::move(original.d_subscriptions)),
  d_messageProperties(bsl::move(original.d_messageProperties)),
  d_mode(bsl::move(original.d_mode)),
  d_broker(bsl::move(original.d_broker)),
  d_queueUri(bsl::move(original.d_queueUri)),
  d_queueFlags(bsl::move(original.d_queueFlags)),
  d_latency(bsl::move(original.d_latency)),
  d_latencyReport(bsl::move(original.d_latencyReport)),
  d_eventsCount(bsl::move(original.d_eventsCount)),
  d_maxUnconfirmed(bsl::move(original.d_maxUnconfirmed)),
  d_verbosity(bsl::move(original.d_verbosity)),
  d_logFormat(bsl::move(original.d_logFormat)),
  d_storage(bsl::move(original.d_storage)),
  d_log(bsl::move(original.d_log)),
  d_sequentialMessagePattern(bsl::move(original.d_sequentialMessagePattern)),
  d_msgSize(bsl::move(original.d_msgSize)),
  d_postRate(bsl::move(original.d_postRate)),
  d_postInterval(bsl::move(original.d_postInterval)),
  d_threads(bsl::move(original.d_threads)),
  d_shutdownGrace(bsl::move(original.d_shutdownGrace)),
  d_autoPubSubModulo(bsl::move(original.d_autoPubSubModulo)),
  d_timeoutSec(bsl::move(original.d_timeoutSec)),
  d_dumpMsg(bsl::move(original.d_dumpMsg)),
  d_confirmMsg(bsl::move(original.d_confirmMsg)),
  d_memoryDebug(bsl::move(original.d_memoryDebug)),
  d_noSessionEventHandler(bsl::move(original.d_noSessionEventHandler))
{
}

CommandLineParameters::CommandLineParameters(CommandLineParameters&& original,
                                             bslma::Allocator* basicAllocator)
: d_eventSize(bsl::move(original.d_eventSize))
, d_subscriptions(bsl::move(original.d_subscriptions), basicAllocator)
, d_messageProperties(bsl::move(original.d_messageProperties), basicAllocator)
, d_mode(bsl::move(original.d_mode), basicAllocator)
, d_broker(bsl::move(original.d_broker), basicAllocator)
, d_queueUri(bsl::move(original.d_queueUri), basicAllocator)
, d_queueFlags(bsl::move(original.d_queueFlags), basicAllocator)
, d_latency(bsl::move(original.d_latency), basicAllocator)
, d_latencyReport(bsl::move(original.d_latencyReport), basicAllocator)
, d_eventsCount(bsl::move(original.d_eventsCount), basicAllocator)
, d_maxUnconfirmed(bsl::move(original.d_maxUnconfirmed), basicAllocator)
, d_verbosity(bsl::move(original.d_verbosity), basicAllocator)
, d_logFormat(bsl::move(original.d_logFormat), basicAllocator)
, d_storage(bsl::move(original.d_storage), basicAllocator)
, d_log(bsl::move(original.d_log), basicAllocator)
, d_sequentialMessagePattern(bsl::move(original.d_sequentialMessagePattern),
                             basicAllocator)
, d_msgSize(bsl::move(original.d_msgSize))
, d_postRate(bsl::move(original.d_postRate))
, d_postInterval(bsl::move(original.d_postInterval))
, d_threads(bsl::move(original.d_threads))
, d_shutdownGrace(bsl::move(original.d_shutdownGrace))
, d_autoPubSubModulo(bsl::move(original.d_autoPubSubModulo))
, d_timeoutSec(bsl::move(original.d_timeoutSec))
, d_dumpMsg(bsl::move(original.d_dumpMsg))
, d_confirmMsg(bsl::move(original.d_confirmMsg))
, d_memoryDebug(bsl::move(original.d_memoryDebug))
, d_noSessionEventHandler(bsl::move(original.d_noSessionEventHandler))
{
}
#endif

CommandLineParameters::~CommandLineParameters()
{
}

// MANIPULATORS

CommandLineParameters&
CommandLineParameters::operator=(const CommandLineParameters& rhs)
{
    if (this != &rhs) {
        d_mode                     = rhs.d_mode;
        d_broker                   = rhs.d_broker;
        d_queueUri                 = rhs.d_queueUri;
        d_queueFlags               = rhs.d_queueFlags;
        d_latency                  = rhs.d_latency;
        d_latencyReport            = rhs.d_latencyReport;
        d_dumpMsg                  = rhs.d_dumpMsg;
        d_confirmMsg               = rhs.d_confirmMsg;
        d_eventSize                = rhs.d_eventSize;
        d_msgSize                  = rhs.d_msgSize;
        d_postRate                 = rhs.d_postRate;
        d_eventsCount              = rhs.d_eventsCount;
        d_maxUnconfirmed           = rhs.d_maxUnconfirmed;
        d_postInterval             = rhs.d_postInterval;
        d_verbosity                = rhs.d_verbosity;
        d_logFormat                = rhs.d_logFormat;
        d_memoryDebug              = rhs.d_memoryDebug;
        d_threads                  = rhs.d_threads;
        d_shutdownGrace            = rhs.d_shutdownGrace;
        d_noSessionEventHandler    = rhs.d_noSessionEventHandler;
        d_storage                  = rhs.d_storage;
        d_log                      = rhs.d_log;
        d_sequentialMessagePattern = rhs.d_sequentialMessagePattern;
        d_messageProperties        = rhs.d_messageProperties;
        d_subscriptions            = rhs.d_subscriptions;
        d_autoPubSubModulo         = rhs.d_autoPubSubModulo;
        d_timeoutSec               = rhs.d_timeoutSec;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CommandLineParameters&
CommandLineParameters::operator=(CommandLineParameters&& rhs)
{
    if (this != &rhs) {
        d_mode                     = bsl::move(rhs.d_mode);
        d_broker                   = bsl::move(rhs.d_broker);
        d_queueUri                 = bsl::move(rhs.d_queueUri);
        d_queueFlags               = bsl::move(rhs.d_queueFlags);
        d_latency                  = bsl::move(rhs.d_latency);
        d_latencyReport            = bsl::move(rhs.d_latencyReport);
        d_dumpMsg                  = bsl::move(rhs.d_dumpMsg);
        d_confirmMsg               = bsl::move(rhs.d_confirmMsg);
        d_eventSize                = bsl::move(rhs.d_eventSize);
        d_msgSize                  = bsl::move(rhs.d_msgSize);
        d_postRate                 = bsl::move(rhs.d_postRate);
        d_eventsCount              = bsl::move(rhs.d_eventsCount);
        d_maxUnconfirmed           = bsl::move(rhs.d_maxUnconfirmed);
        d_postInterval             = bsl::move(rhs.d_postInterval);
        d_verbosity                = bsl::move(rhs.d_verbosity);
        d_logFormat                = bsl::move(rhs.d_logFormat);
        d_memoryDebug              = bsl::move(rhs.d_memoryDebug);
        d_threads                  = bsl::move(rhs.d_threads);
        d_shutdownGrace            = bsl::move(rhs.d_shutdownGrace);
        d_noSessionEventHandler    = bsl::move(rhs.d_noSessionEventHandler);
        d_storage                  = bsl::move(rhs.d_storage);
        d_log                      = bsl::move(rhs.d_log);
        d_sequentialMessagePattern = bsl::move(rhs.d_sequentialMessagePattern);
        d_messageProperties        = bsl::move(rhs.d_messageProperties);
        d_subscriptions            = bsl::move(rhs.d_subscriptions);
        d_autoPubSubModulo         = bsl::move(rhs.d_autoPubSubModulo);
        d_timeoutSec               = bsl::move(rhs.d_timeoutSec);
    }

    return *this;
}
#endif

void CommandLineParameters::reset()
{
    d_mode                  = DEFAULT_INITIALIZER_MODE;
    d_broker                = DEFAULT_INITIALIZER_BROKER;
    d_queueUri              = DEFAULT_INITIALIZER_QUEUE_URI;
    d_queueFlags            = DEFAULT_INITIALIZER_QUEUE_FLAGS;
    d_latency               = DEFAULT_INITIALIZER_LATENCY;
    d_latencyReport         = DEFAULT_INITIALIZER_LATENCY_REPORT;
    d_dumpMsg               = DEFAULT_INITIALIZER_DUMP_MSG;
    d_confirmMsg            = DEFAULT_INITIALIZER_CONFIRM_MSG;
    d_eventSize             = DEFAULT_INITIALIZER_EVENT_SIZE;
    d_msgSize               = DEFAULT_INITIALIZER_MSG_SIZE;
    d_postRate              = DEFAULT_INITIALIZER_POST_RATE;
    d_eventsCount           = DEFAULT_INITIALIZER_EVENTS_COUNT;
    d_maxUnconfirmed        = DEFAULT_INITIALIZER_MAX_UNCONFIRMED;
    d_postInterval          = DEFAULT_INITIALIZER_POST_INTERVAL;
    d_verbosity             = DEFAULT_INITIALIZER_VERBOSITY;
    d_logFormat             = DEFAULT_INITIALIZER_LOG_FORMAT;
    d_memoryDebug           = DEFAULT_INITIALIZER_MEMORY_DEBUG;
    d_threads               = DEFAULT_INITIALIZER_THREADS;
    d_shutdownGrace         = DEFAULT_INITIALIZER_SHUTDOWN_GRACE;
    d_noSessionEventHandler = DEFAULT_INITIALIZER_NO_SESSION_EVENT_HANDLER;
    d_storage               = DEFAULT_INITIALIZER_STORAGE;
    d_log                   = DEFAULT_INITIALIZER_LOG;
    d_sequentialMessagePattern =
        DEFAULT_INITIALIZER_SEQUENTIAL_MESSAGE_PATTERN;
    bdlat_ValueTypeFunctions::reset(&d_messageProperties);
    bdlat_ValueTypeFunctions::reset(&d_subscriptions);
    d_autoPubSubModulo = DEFAULT_INITIALIZER_AUTO_PUB_SUB_MODULO;
    d_timeoutSec       = DEFAULT_INITIALIZER_TIMEOUT_SEC;
}

// ACCESSORS

bsl::ostream& CommandLineParameters::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("mode", this->mode());
    printer.printAttribute("broker", this->broker());
    printer.printAttribute("queueUri", this->queueUri());
    printer.printAttribute("queueFlags", this->queueFlags());
    printer.printAttribute("latency", this->latency());
    printer.printAttribute("latencyReport", this->latencyReport());
    printer.printAttribute("dumpMsg", this->dumpMsg());
    printer.printAttribute("confirmMsg", this->confirmMsg());
    printer.printAttribute("eventSize", this->eventSize());
    printer.printAttribute("msgSize", this->msgSize());
    printer.printAttribute("postRate", this->postRate());
    printer.printAttribute("eventsCount", this->eventsCount());
    printer.printAttribute("maxUnconfirmed", this->maxUnconfirmed());
    printer.printAttribute("postInterval", this->postInterval());
    printer.printAttribute("verbosity", this->verbosity());
    printer.printAttribute("logFormat", this->logFormat());
    printer.printAttribute("memoryDebug", this->memoryDebug());
    printer.printAttribute("threads", this->threads());
    printer.printAttribute("shutdownGrace", this->shutdownGrace());
    printer.printAttribute("noSessionEventHandler",
                           this->noSessionEventHandler());
    printer.printAttribute("storage", this->storage());
    printer.printAttribute("log", this->log());
    printer.printAttribute("sequentialMessagePattern",
                           this->sequentialMessagePattern());
    printer.printAttribute("messageProperties", this->messageProperties());
    printer.printAttribute("subscriptions", this->subscriptions());
    printer.printAttribute("autoPubSubModulo", this->autoPubSubModulo());
    printer.printAttribute("timeoutSec", this->timeoutSec());
    printer.end();
    return stream;
}

// --------------------
// class JournalCommand
// --------------------

// CONSTANTS

const char JournalCommand::CLASS_NAME[] = "JournalCommand";

const bdlat_AttributeInfo JournalCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo*
JournalCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    if (bdlb::String::areEqualCaseless("n", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("next", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("p", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("prev", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("r", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("record", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("list", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("l", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("dump", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("type", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            JournalCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* JournalCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

JournalCommand::JournalCommand(bslma::Allocator* basicAllocator)
: d_choice(basicAllocator)
{
}

JournalCommand::JournalCommand(const JournalCommand& original,
                               bslma::Allocator*     basicAllocator)
: d_choice(original.d_choice, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
JournalCommand::JournalCommand(JournalCommand&& original) noexcept
: d_choice(bsl::move(original.d_choice))
{
}

JournalCommand::JournalCommand(JournalCommand&&  original,
                               bslma::Allocator* basicAllocator)
: d_choice(bsl::move(original.d_choice), basicAllocator)
{
}
#endif

JournalCommand::~JournalCommand()
{
}

// MANIPULATORS

JournalCommand& JournalCommand::operator=(const JournalCommand& rhs)
{
    if (this != &rhs) {
        d_choice = rhs.d_choice;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
JournalCommand& JournalCommand::operator=(JournalCommand&& rhs)
{
    if (this != &rhs) {
        d_choice = bsl::move(rhs.d_choice);
    }

    return *this;
}
#endif

void JournalCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream& JournalCommand::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// -----------------
// class PostCommand
// -----------------

// CONSTANTS

const char PostCommand::CLASS_NAME[] = "PostCommand";

const bool PostCommand::DEFAULT_INITIALIZER_ASYNC = false;

const char PostCommand::DEFAULT_INITIALIZER_GROUPID[] = "";

const char PostCommand::DEFAULT_INITIALIZER_COMPRESSION_ALGORITHM_TYPE[] =
    "none";

const bdlat_AttributeInfo PostCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PAYLOAD,
     "payload",
     sizeof("payload") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ASYNC,
     "async",
     sizeof("async") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_GROUPID,
     "groupid",
     sizeof("groupid") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_COMPRESSION_ALGORITHM_TYPE,
     "compressionAlgorithmType",
     sizeof("compressionAlgorithmType") - 1,
     "",
     bdlat_FormattingMode::e_TEXT | bdlat_FormattingMode::e_DEFAULT_VALUE},
    {ATTRIBUTE_ID_MESSAGE_PROPERTIES,
     "messageProperties",
     sizeof("messageProperties") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* PostCommand::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PostCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PostCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_PAYLOAD:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD];
    case ATTRIBUTE_ID_ASYNC:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC];
    case ATTRIBUTE_ID_GROUPID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GROUPID];
    case ATTRIBUTE_ID_COMPRESSION_ALGORITHM_TYPE:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_COMPRESSION_ALGORITHM_TYPE];
    case ATTRIBUTE_ID_MESSAGE_PROPERTIES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES];
    default: return 0;
    }
}

// CREATORS

PostCommand::PostCommand(bslma::Allocator* basicAllocator)
: d_payload(basicAllocator)
, d_messageProperties(basicAllocator)
, d_uri(basicAllocator)
, d_groupid(DEFAULT_INITIALIZER_GROUPID, basicAllocator)
, d_compressionAlgorithmType(DEFAULT_INITIALIZER_COMPRESSION_ALGORITHM_TYPE,
                             basicAllocator)
, d_async(DEFAULT_INITIALIZER_ASYNC)
{
}

PostCommand::PostCommand(const PostCommand& original,
                         bslma::Allocator*  basicAllocator)
: d_payload(original.d_payload, basicAllocator)
, d_messageProperties(original.d_messageProperties, basicAllocator)
, d_uri(original.d_uri, basicAllocator)
, d_groupid(original.d_groupid, basicAllocator)
, d_compressionAlgorithmType(original.d_compressionAlgorithmType,
                             basicAllocator)
, d_async(original.d_async)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PostCommand::PostCommand(PostCommand&& original) noexcept
: d_payload(bsl::move(original.d_payload)),
  d_messageProperties(bsl::move(original.d_messageProperties)),
  d_uri(bsl::move(original.d_uri)),
  d_groupid(bsl::move(original.d_groupid)),
  d_compressionAlgorithmType(bsl::move(original.d_compressionAlgorithmType)),
  d_async(bsl::move(original.d_async))
{
}

PostCommand::PostCommand(PostCommand&&     original,
                         bslma::Allocator* basicAllocator)
: d_payload(bsl::move(original.d_payload), basicAllocator)
, d_messageProperties(bsl::move(original.d_messageProperties), basicAllocator)
, d_uri(bsl::move(original.d_uri), basicAllocator)
, d_groupid(bsl::move(original.d_groupid), basicAllocator)
, d_compressionAlgorithmType(bsl::move(original.d_compressionAlgorithmType),
                             basicAllocator)
, d_async(bsl::move(original.d_async))
{
}
#endif

PostCommand::~PostCommand()
{
}

// MANIPULATORS

PostCommand& PostCommand::operator=(const PostCommand& rhs)
{
    if (this != &rhs) {
        d_uri                      = rhs.d_uri;
        d_payload                  = rhs.d_payload;
        d_async                    = rhs.d_async;
        d_groupid                  = rhs.d_groupid;
        d_compressionAlgorithmType = rhs.d_compressionAlgorithmType;
        d_messageProperties        = rhs.d_messageProperties;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PostCommand& PostCommand::operator=(PostCommand&& rhs)
{
    if (this != &rhs) {
        d_uri                      = bsl::move(rhs.d_uri);
        d_payload                  = bsl::move(rhs.d_payload);
        d_async                    = bsl::move(rhs.d_async);
        d_groupid                  = bsl::move(rhs.d_groupid);
        d_compressionAlgorithmType = bsl::move(rhs.d_compressionAlgorithmType);
        d_messageProperties        = bsl::move(rhs.d_messageProperties);
    }

    return *this;
}
#endif

void PostCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_payload);
    d_async   = DEFAULT_INITIALIZER_ASYNC;
    d_groupid = DEFAULT_INITIALIZER_GROUPID;
    d_compressionAlgorithmType =
        DEFAULT_INITIALIZER_COMPRESSION_ALGORITHM_TYPE;
    bdlat_ValueTypeFunctions::reset(&d_messageProperties);
}

// ACCESSORS

bsl::ostream&
PostCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("payload", this->payload());
    printer.printAttribute("async", this->async());
    printer.printAttribute("groupid", this->groupid());
    printer.printAttribute("compressionAlgorithmType",
                           this->compressionAlgorithmType());
    printer.printAttribute("messageProperties", this->messageProperties());
    printer.end();
    return stream;
}

// -------------
// class Command
// -------------

// CONSTANTS

const char Command::CLASS_NAME[] = "Command";

const bdlat_SelectionInfo Command::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_START,
     "start",
     sizeof("start") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STOP,
     "stop",
     sizeof("stop") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_OPEN_QUEUE,
     "openQueue",
     sizeof("openQueue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONFIGURE_QUEUE,
     "configureQueue",
     sizeof("configureQueue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLOSE_QUEUE,
     "closeQueue",
     sizeof("closeQueue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_POST,
     "post",
     sizeof("post") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LIST,
     "list",
     sizeof("list") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONFIRM,
     "confirm",
     sizeof("confirm") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_BATCH_POST,
     "batch-post",
     sizeof("batch-post") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LOAD_POST,
     "load-post",
     sizeof("load-post") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_OPEN_STORAGE,
     "openStorage",
     sizeof("openStorage") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLOSE_STORAGE,
     "closeStorage",
     sizeof("closeStorage") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_METADATA,
     "metadata",
     sizeof("metadata") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LIST_QUEUES,
     "listQueues",
     sizeof("listQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DUMP_QUEUE,
     "dumpQueue",
     sizeof("dumpQueue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DATA,
     "data",
     sizeof("data") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QLIST,
     "qlist",
     sizeof("qlist") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_JOURNAL,
     "journal",
     sizeof("journal") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* Command::lookupSelectionInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 18; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            Command::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* Command::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_START:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_START];
    case SELECTION_ID_STOP: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STOP];
    case SELECTION_ID_OPEN_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_QUEUE];
    case SELECTION_ID_CONFIGURE_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIGURE_QUEUE];
    case SELECTION_ID_CLOSE_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_QUEUE];
    case SELECTION_ID_POST: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_POST];
    case SELECTION_ID_LIST: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST];
    case SELECTION_ID_CONFIRM:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIRM];
    case SELECTION_ID_BATCH_POST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_BATCH_POST];
    case SELECTION_ID_LOAD_POST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LOAD_POST];
    case SELECTION_ID_OPEN_STORAGE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_STORAGE];
    case SELECTION_ID_CLOSE_STORAGE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_STORAGE];
    case SELECTION_ID_METADATA:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_METADATA];
    case SELECTION_ID_LIST_QUEUES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_QUEUES];
    case SELECTION_ID_DUMP_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP_QUEUE];
    case SELECTION_ID_DATA: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DATA];
    case SELECTION_ID_QLIST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QLIST];
    case SELECTION_ID_JOURNAL:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_JOURNAL];
    default: return 0;
    }
}

// CREATORS

Command::Command(const Command& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_START: {
        new (d_start.buffer()) StartCommand(original.d_start.object());
    } break;
    case SELECTION_ID_STOP: {
        new (d_stop.buffer()) StopCommand(original.d_stop.object());
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        new (d_openQueue.buffer())
            OpenQueueCommand(original.d_openQueue.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE: {
        new (d_configureQueue.buffer())
            ConfigureQueueCommand(original.d_configureQueue.object(),
                                  d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        new (d_closeQueue.buffer())
            CloseQueueCommand(original.d_closeQueue.object(), d_allocator_p);
    } break;
    case SELECTION_ID_POST: {
        new (d_post.buffer())
            PostCommand(original.d_post.object(), d_allocator_p);
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer())
            ListCommand(original.d_list.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CONFIRM: {
        new (d_confirm.buffer())
            ConfirmCommand(original.d_confirm.object(), d_allocator_p);
    } break;
    case SELECTION_ID_BATCH_POST: {
        new (d_batchPost.buffer())
            BatchPostCommand(original.d_batchPost.object(), d_allocator_p);
    } break;
    case SELECTION_ID_LOAD_POST: {
        new (d_loadPost.buffer())
            LoadPostCommand(original.d_loadPost.object(), d_allocator_p);
    } break;
    case SELECTION_ID_OPEN_STORAGE: {
        new (d_openStorage.buffer())
            OpenStorageCommand(original.d_openStorage.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_STORAGE: {
        new (d_closeStorage.buffer())
            CloseStorageCommand(original.d_closeStorage.object());
    } break;
    case SELECTION_ID_METADATA: {
        new (d_metadata.buffer())
            MetadataCommand(original.d_metadata.object());
    } break;
    case SELECTION_ID_LIST_QUEUES: {
        new (d_listQueues.buffer())
            ListQueuesCommand(original.d_listQueues.object());
    } break;
    case SELECTION_ID_DUMP_QUEUE: {
        new (d_dumpQueue.buffer())
            DumpQueueCommand(original.d_dumpQueue.object(), d_allocator_p);
    } break;
    case SELECTION_ID_DATA: {
        new (d_data.buffer()) DataCommand(original.d_data.object());
    } break;
    case SELECTION_ID_QLIST: {
        new (d_qlist.buffer()) QlistCommand(original.d_qlist.object());
    } break;
    case SELECTION_ID_JOURNAL: {
        new (d_journal.buffer())
            JournalCommand(original.d_journal.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Command::Command(Command&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_START: {
        new (d_start.buffer())
            StartCommand(bsl::move(original.d_start.object()));
    } break;
    case SELECTION_ID_STOP: {
        new (d_stop.buffer()) StopCommand(bsl::move(original.d_stop.object()));
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        new (d_openQueue.buffer())
            OpenQueueCommand(bsl::move(original.d_openQueue.object()),
                             d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE: {
        new (d_configureQueue.buffer()) ConfigureQueueCommand(
            bsl::move(original.d_configureQueue.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        new (d_closeQueue.buffer())
            CloseQueueCommand(bsl::move(original.d_closeQueue.object()),
                              d_allocator_p);
    } break;
    case SELECTION_ID_POST: {
        new (d_post.buffer())
            PostCommand(bsl::move(original.d_post.object()), d_allocator_p);
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer())
            ListCommand(bsl::move(original.d_list.object()), d_allocator_p);
    } break;
    case SELECTION_ID_CONFIRM: {
        new (d_confirm.buffer())
            ConfirmCommand(bsl::move(original.d_confirm.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_BATCH_POST: {
        new (d_batchPost.buffer())
            BatchPostCommand(bsl::move(original.d_batchPost.object()),
                             d_allocator_p);
    } break;
    case SELECTION_ID_LOAD_POST: {
        new (d_loadPost.buffer())
            LoadPostCommand(bsl::move(original.d_loadPost.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_OPEN_STORAGE: {
        new (d_openStorage.buffer())
            OpenStorageCommand(bsl::move(original.d_openStorage.object()),
                               d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_STORAGE: {
        new (d_closeStorage.buffer())
            CloseStorageCommand(bsl::move(original.d_closeStorage.object()));
    } break;
    case SELECTION_ID_METADATA: {
        new (d_metadata.buffer())
            MetadataCommand(bsl::move(original.d_metadata.object()));
    } break;
    case SELECTION_ID_LIST_QUEUES: {
        new (d_listQueues.buffer())
            ListQueuesCommand(bsl::move(original.d_listQueues.object()));
    } break;
    case SELECTION_ID_DUMP_QUEUE: {
        new (d_dumpQueue.buffer())
            DumpQueueCommand(bsl::move(original.d_dumpQueue.object()),
                             d_allocator_p);
    } break;
    case SELECTION_ID_DATA: {
        new (d_data.buffer()) DataCommand(bsl::move(original.d_data.object()));
    } break;
    case SELECTION_ID_QLIST: {
        new (d_qlist.buffer())
            QlistCommand(bsl::move(original.d_qlist.object()));
    } break;
    case SELECTION_ID_JOURNAL: {
        new (d_journal.buffer())
            JournalCommand(bsl::move(original.d_journal.object()),
                           d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

Command::Command(Command&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_START: {
        new (d_start.buffer())
            StartCommand(bsl::move(original.d_start.object()));
    } break;
    case SELECTION_ID_STOP: {
        new (d_stop.buffer()) StopCommand(bsl::move(original.d_stop.object()));
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        new (d_openQueue.buffer())
            OpenQueueCommand(bsl::move(original.d_openQueue.object()),
                             d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE: {
        new (d_configureQueue.buffer()) ConfigureQueueCommand(
            bsl::move(original.d_configureQueue.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        new (d_closeQueue.buffer())
            CloseQueueCommand(bsl::move(original.d_closeQueue.object()),
                              d_allocator_p);
    } break;
    case SELECTION_ID_POST: {
        new (d_post.buffer())
            PostCommand(bsl::move(original.d_post.object()), d_allocator_p);
    } break;
    case SELECTION_ID_LIST: {
        new (d_list.buffer())
            ListCommand(bsl::move(original.d_list.object()), d_allocator_p);
    } break;
    case SELECTION_ID_CONFIRM: {
        new (d_confirm.buffer())
            ConfirmCommand(bsl::move(original.d_confirm.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_BATCH_POST: {
        new (d_batchPost.buffer())
            BatchPostCommand(bsl::move(original.d_batchPost.object()),
                             d_allocator_p);
    } break;
    case SELECTION_ID_LOAD_POST: {
        new (d_loadPost.buffer())
            LoadPostCommand(bsl::move(original.d_loadPost.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_OPEN_STORAGE: {
        new (d_openStorage.buffer())
            OpenStorageCommand(bsl::move(original.d_openStorage.object()),
                               d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_STORAGE: {
        new (d_closeStorage.buffer())
            CloseStorageCommand(bsl::move(original.d_closeStorage.object()));
    } break;
    case SELECTION_ID_METADATA: {
        new (d_metadata.buffer())
            MetadataCommand(bsl::move(original.d_metadata.object()));
    } break;
    case SELECTION_ID_LIST_QUEUES: {
        new (d_listQueues.buffer())
            ListQueuesCommand(bsl::move(original.d_listQueues.object()));
    } break;
    case SELECTION_ID_DUMP_QUEUE: {
        new (d_dumpQueue.buffer())
            DumpQueueCommand(bsl::move(original.d_dumpQueue.object()),
                             d_allocator_p);
    } break;
    case SELECTION_ID_DATA: {
        new (d_data.buffer()) DataCommand(bsl::move(original.d_data.object()));
    } break;
    case SELECTION_ID_QLIST: {
        new (d_qlist.buffer())
            QlistCommand(bsl::move(original.d_qlist.object()));
    } break;
    case SELECTION_ID_JOURNAL: {
        new (d_journal.buffer())
            JournalCommand(bsl::move(original.d_journal.object()),
                           d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

Command& Command::operator=(const Command& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_START: {
            makeStart(rhs.d_start.object());
        } break;
        case SELECTION_ID_STOP: {
            makeStop(rhs.d_stop.object());
        } break;
        case SELECTION_ID_OPEN_QUEUE: {
            makeOpenQueue(rhs.d_openQueue.object());
        } break;
        case SELECTION_ID_CONFIGURE_QUEUE: {
            makeConfigureQueue(rhs.d_configureQueue.object());
        } break;
        case SELECTION_ID_CLOSE_QUEUE: {
            makeCloseQueue(rhs.d_closeQueue.object());
        } break;
        case SELECTION_ID_POST: {
            makePost(rhs.d_post.object());
        } break;
        case SELECTION_ID_LIST: {
            makeList(rhs.d_list.object());
        } break;
        case SELECTION_ID_CONFIRM: {
            makeConfirm(rhs.d_confirm.object());
        } break;
        case SELECTION_ID_BATCH_POST: {
            makeBatchPost(rhs.d_batchPost.object());
        } break;
        case SELECTION_ID_LOAD_POST: {
            makeLoadPost(rhs.d_loadPost.object());
        } break;
        case SELECTION_ID_OPEN_STORAGE: {
            makeOpenStorage(rhs.d_openStorage.object());
        } break;
        case SELECTION_ID_CLOSE_STORAGE: {
            makeCloseStorage(rhs.d_closeStorage.object());
        } break;
        case SELECTION_ID_METADATA: {
            makeMetadata(rhs.d_metadata.object());
        } break;
        case SELECTION_ID_LIST_QUEUES: {
            makeListQueues(rhs.d_listQueues.object());
        } break;
        case SELECTION_ID_DUMP_QUEUE: {
            makeDumpQueue(rhs.d_dumpQueue.object());
        } break;
        case SELECTION_ID_DATA: {
            makeData(rhs.d_data.object());
        } break;
        case SELECTION_ID_QLIST: {
            makeQlist(rhs.d_qlist.object());
        } break;
        case SELECTION_ID_JOURNAL: {
            makeJournal(rhs.d_journal.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Command& Command::operator=(Command&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_START: {
            makeStart(bsl::move(rhs.d_start.object()));
        } break;
        case SELECTION_ID_STOP: {
            makeStop(bsl::move(rhs.d_stop.object()));
        } break;
        case SELECTION_ID_OPEN_QUEUE: {
            makeOpenQueue(bsl::move(rhs.d_openQueue.object()));
        } break;
        case SELECTION_ID_CONFIGURE_QUEUE: {
            makeConfigureQueue(bsl::move(rhs.d_configureQueue.object()));
        } break;
        case SELECTION_ID_CLOSE_QUEUE: {
            makeCloseQueue(bsl::move(rhs.d_closeQueue.object()));
        } break;
        case SELECTION_ID_POST: {
            makePost(bsl::move(rhs.d_post.object()));
        } break;
        case SELECTION_ID_LIST: {
            makeList(bsl::move(rhs.d_list.object()));
        } break;
        case SELECTION_ID_CONFIRM: {
            makeConfirm(bsl::move(rhs.d_confirm.object()));
        } break;
        case SELECTION_ID_BATCH_POST: {
            makeBatchPost(bsl::move(rhs.d_batchPost.object()));
        } break;
        case SELECTION_ID_LOAD_POST: {
            makeLoadPost(bsl::move(rhs.d_loadPost.object()));
        } break;
        case SELECTION_ID_OPEN_STORAGE: {
            makeOpenStorage(bsl::move(rhs.d_openStorage.object()));
        } break;
        case SELECTION_ID_CLOSE_STORAGE: {
            makeCloseStorage(bsl::move(rhs.d_closeStorage.object()));
        } break;
        case SELECTION_ID_METADATA: {
            makeMetadata(bsl::move(rhs.d_metadata.object()));
        } break;
        case SELECTION_ID_LIST_QUEUES: {
            makeListQueues(bsl::move(rhs.d_listQueues.object()));
        } break;
        case SELECTION_ID_DUMP_QUEUE: {
            makeDumpQueue(bsl::move(rhs.d_dumpQueue.object()));
        } break;
        case SELECTION_ID_DATA: {
            makeData(bsl::move(rhs.d_data.object()));
        } break;
        case SELECTION_ID_QLIST: {
            makeQlist(bsl::move(rhs.d_qlist.object()));
        } break;
        case SELECTION_ID_JOURNAL: {
            makeJournal(bsl::move(rhs.d_journal.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void Command::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_START: {
        d_start.object().~StartCommand();
    } break;
    case SELECTION_ID_STOP: {
        d_stop.object().~StopCommand();
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        d_openQueue.object().~OpenQueueCommand();
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE: {
        d_configureQueue.object().~ConfigureQueueCommand();
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        d_closeQueue.object().~CloseQueueCommand();
    } break;
    case SELECTION_ID_POST: {
        d_post.object().~PostCommand();
    } break;
    case SELECTION_ID_LIST: {
        d_list.object().~ListCommand();
    } break;
    case SELECTION_ID_CONFIRM: {
        d_confirm.object().~ConfirmCommand();
    } break;
    case SELECTION_ID_BATCH_POST: {
        d_batchPost.object().~BatchPostCommand();
    } break;
    case SELECTION_ID_LOAD_POST: {
        d_loadPost.object().~LoadPostCommand();
    } break;
    case SELECTION_ID_OPEN_STORAGE: {
        d_openStorage.object().~OpenStorageCommand();
    } break;
    case SELECTION_ID_CLOSE_STORAGE: {
        d_closeStorage.object().~CloseStorageCommand();
    } break;
    case SELECTION_ID_METADATA: {
        d_metadata.object().~MetadataCommand();
    } break;
    case SELECTION_ID_LIST_QUEUES: {
        d_listQueues.object().~ListQueuesCommand();
    } break;
    case SELECTION_ID_DUMP_QUEUE: {
        d_dumpQueue.object().~DumpQueueCommand();
    } break;
    case SELECTION_ID_DATA: {
        d_data.object().~DataCommand();
    } break;
    case SELECTION_ID_QLIST: {
        d_qlist.object().~QlistCommand();
    } break;
    case SELECTION_ID_JOURNAL: {
        d_journal.object().~JournalCommand();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int Command::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_START: {
        makeStart();
    } break;
    case SELECTION_ID_STOP: {
        makeStop();
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        makeOpenQueue();
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE: {
        makeConfigureQueue();
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        makeCloseQueue();
    } break;
    case SELECTION_ID_POST: {
        makePost();
    } break;
    case SELECTION_ID_LIST: {
        makeList();
    } break;
    case SELECTION_ID_CONFIRM: {
        makeConfirm();
    } break;
    case SELECTION_ID_BATCH_POST: {
        makeBatchPost();
    } break;
    case SELECTION_ID_LOAD_POST: {
        makeLoadPost();
    } break;
    case SELECTION_ID_OPEN_STORAGE: {
        makeOpenStorage();
    } break;
    case SELECTION_ID_CLOSE_STORAGE: {
        makeCloseStorage();
    } break;
    case SELECTION_ID_METADATA: {
        makeMetadata();
    } break;
    case SELECTION_ID_LIST_QUEUES: {
        makeListQueues();
    } break;
    case SELECTION_ID_DUMP_QUEUE: {
        makeDumpQueue();
    } break;
    case SELECTION_ID_DATA: {
        makeData();
    } break;
    case SELECTION_ID_QLIST: {
        makeQlist();
    } break;
    case SELECTION_ID_JOURNAL: {
        makeJournal();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int Command::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

StartCommand& Command::makeStart()
{
    if (SELECTION_ID_START == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_start.object());
    }
    else {
        reset();
        new (d_start.buffer()) StartCommand();
        d_selectionId = SELECTION_ID_START;
    }

    return d_start.object();
}

StartCommand& Command::makeStart(const StartCommand& value)
{
    if (SELECTION_ID_START == d_selectionId) {
        d_start.object() = value;
    }
    else {
        reset();
        new (d_start.buffer()) StartCommand(value);
        d_selectionId = SELECTION_ID_START;
    }

    return d_start.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StartCommand& Command::makeStart(StartCommand&& value)
{
    if (SELECTION_ID_START == d_selectionId) {
        d_start.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_start.buffer()) StartCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_START;
    }

    return d_start.object();
}
#endif

StopCommand& Command::makeStop()
{
    if (SELECTION_ID_STOP == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_stop.object());
    }
    else {
        reset();
        new (d_stop.buffer()) StopCommand();
        d_selectionId = SELECTION_ID_STOP;
    }

    return d_stop.object();
}

StopCommand& Command::makeStop(const StopCommand& value)
{
    if (SELECTION_ID_STOP == d_selectionId) {
        d_stop.object() = value;
    }
    else {
        reset();
        new (d_stop.buffer()) StopCommand(value);
        d_selectionId = SELECTION_ID_STOP;
    }

    return d_stop.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StopCommand& Command::makeStop(StopCommand&& value)
{
    if (SELECTION_ID_STOP == d_selectionId) {
        d_stop.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_stop.buffer()) StopCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_STOP;
    }

    return d_stop.object();
}
#endif

OpenQueueCommand& Command::makeOpenQueue()
{
    if (SELECTION_ID_OPEN_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_openQueue.object());
    }
    else {
        reset();
        new (d_openQueue.buffer()) OpenQueueCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_QUEUE;
    }

    return d_openQueue.object();
}

OpenQueueCommand& Command::makeOpenQueue(const OpenQueueCommand& value)
{
    if (SELECTION_ID_OPEN_QUEUE == d_selectionId) {
        d_openQueue.object() = value;
    }
    else {
        reset();
        new (d_openQueue.buffer()) OpenQueueCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_QUEUE;
    }

    return d_openQueue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenQueueCommand& Command::makeOpenQueue(OpenQueueCommand&& value)
{
    if (SELECTION_ID_OPEN_QUEUE == d_selectionId) {
        d_openQueue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_openQueue.buffer())
            OpenQueueCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_QUEUE;
    }

    return d_openQueue.object();
}
#endif

ConfigureQueueCommand& Command::makeConfigureQueue()
{
    if (SELECTION_ID_CONFIGURE_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_configureQueue.object());
    }
    else {
        reset();
        new (d_configureQueue.buffer()) ConfigureQueueCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_QUEUE;
    }

    return d_configureQueue.object();
}

ConfigureQueueCommand&
Command::makeConfigureQueue(const ConfigureQueueCommand& value)
{
    if (SELECTION_ID_CONFIGURE_QUEUE == d_selectionId) {
        d_configureQueue.object() = value;
    }
    else {
        reset();
        new (d_configureQueue.buffer())
            ConfigureQueueCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_QUEUE;
    }

    return d_configureQueue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureQueueCommand&
Command::makeConfigureQueue(ConfigureQueueCommand&& value)
{
    if (SELECTION_ID_CONFIGURE_QUEUE == d_selectionId) {
        d_configureQueue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_configureQueue.buffer())
            ConfigureQueueCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_QUEUE;
    }

    return d_configureQueue.object();
}
#endif

CloseQueueCommand& Command::makeCloseQueue()
{
    if (SELECTION_ID_CLOSE_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_closeQueue.object());
    }
    else {
        reset();
        new (d_closeQueue.buffer()) CloseQueueCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_CLOSE_QUEUE;
    }

    return d_closeQueue.object();
}

CloseQueueCommand& Command::makeCloseQueue(const CloseQueueCommand& value)
{
    if (SELECTION_ID_CLOSE_QUEUE == d_selectionId) {
        d_closeQueue.object() = value;
    }
    else {
        reset();
        new (d_closeQueue.buffer()) CloseQueueCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLOSE_QUEUE;
    }

    return d_closeQueue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CloseQueueCommand& Command::makeCloseQueue(CloseQueueCommand&& value)
{
    if (SELECTION_ID_CLOSE_QUEUE == d_selectionId) {
        d_closeQueue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_closeQueue.buffer())
            CloseQueueCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLOSE_QUEUE;
    }

    return d_closeQueue.object();
}
#endif

PostCommand& Command::makePost()
{
    if (SELECTION_ID_POST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_post.object());
    }
    else {
        reset();
        new (d_post.buffer()) PostCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_POST;
    }

    return d_post.object();
}

PostCommand& Command::makePost(const PostCommand& value)
{
    if (SELECTION_ID_POST == d_selectionId) {
        d_post.object() = value;
    }
    else {
        reset();
        new (d_post.buffer()) PostCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_POST;
    }

    return d_post.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PostCommand& Command::makePost(PostCommand&& value)
{
    if (SELECTION_ID_POST == d_selectionId) {
        d_post.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_post.buffer()) PostCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_POST;
    }

    return d_post.object();
}
#endif

ListCommand& Command::makeList()
{
    if (SELECTION_ID_LIST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_list.object());
    }
    else {
        reset();
        new (d_list.buffer()) ListCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

ListCommand& Command::makeList(const ListCommand& value)
{
    if (SELECTION_ID_LIST == d_selectionId) {
        d_list.object() = value;
    }
    else {
        reset();
        new (d_list.buffer()) ListCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ListCommand& Command::makeList(ListCommand&& value)
{
    if (SELECTION_ID_LIST == d_selectionId) {
        d_list.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_list.buffer()) ListCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}
#endif

ConfirmCommand& Command::makeConfirm()
{
    if (SELECTION_ID_CONFIRM == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_confirm.object());
    }
    else {
        reset();
        new (d_confirm.buffer()) ConfirmCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIRM;
    }

    return d_confirm.object();
}

ConfirmCommand& Command::makeConfirm(const ConfirmCommand& value)
{
    if (SELECTION_ID_CONFIRM == d_selectionId) {
        d_confirm.object() = value;
    }
    else {
        reset();
        new (d_confirm.buffer()) ConfirmCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIRM;
    }

    return d_confirm.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfirmCommand& Command::makeConfirm(ConfirmCommand&& value)
{
    if (SELECTION_ID_CONFIRM == d_selectionId) {
        d_confirm.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_confirm.buffer())
            ConfirmCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIRM;
    }

    return d_confirm.object();
}
#endif

BatchPostCommand& Command::makeBatchPost()
{
    if (SELECTION_ID_BATCH_POST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_batchPost.object());
    }
    else {
        reset();
        new (d_batchPost.buffer()) BatchPostCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_BATCH_POST;
    }

    return d_batchPost.object();
}

BatchPostCommand& Command::makeBatchPost(const BatchPostCommand& value)
{
    if (SELECTION_ID_BATCH_POST == d_selectionId) {
        d_batchPost.object() = value;
    }
    else {
        reset();
        new (d_batchPost.buffer()) BatchPostCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_BATCH_POST;
    }

    return d_batchPost.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BatchPostCommand& Command::makeBatchPost(BatchPostCommand&& value)
{
    if (SELECTION_ID_BATCH_POST == d_selectionId) {
        d_batchPost.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_batchPost.buffer())
            BatchPostCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_BATCH_POST;
    }

    return d_batchPost.object();
}
#endif

LoadPostCommand& Command::makeLoadPost()
{
    if (SELECTION_ID_LOAD_POST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_loadPost.object());
    }
    else {
        reset();
        new (d_loadPost.buffer()) LoadPostCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_LOAD_POST;
    }

    return d_loadPost.object();
}

LoadPostCommand& Command::makeLoadPost(const LoadPostCommand& value)
{
    if (SELECTION_ID_LOAD_POST == d_selectionId) {
        d_loadPost.object() = value;
    }
    else {
        reset();
        new (d_loadPost.buffer()) LoadPostCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_LOAD_POST;
    }

    return d_loadPost.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LoadPostCommand& Command::makeLoadPost(LoadPostCommand&& value)
{
    if (SELECTION_ID_LOAD_POST == d_selectionId) {
        d_loadPost.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_loadPost.buffer())
            LoadPostCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_LOAD_POST;
    }

    return d_loadPost.object();
}
#endif

OpenStorageCommand& Command::makeOpenStorage()
{
    if (SELECTION_ID_OPEN_STORAGE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_openStorage.object());
    }
    else {
        reset();
        new (d_openStorage.buffer()) OpenStorageCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_STORAGE;
    }

    return d_openStorage.object();
}

OpenStorageCommand& Command::makeOpenStorage(const OpenStorageCommand& value)
{
    if (SELECTION_ID_OPEN_STORAGE == d_selectionId) {
        d_openStorage.object() = value;
    }
    else {
        reset();
        new (d_openStorage.buffer()) OpenStorageCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_STORAGE;
    }

    return d_openStorage.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenStorageCommand& Command::makeOpenStorage(OpenStorageCommand&& value)
{
    if (SELECTION_ID_OPEN_STORAGE == d_selectionId) {
        d_openStorage.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_openStorage.buffer())
            OpenStorageCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_STORAGE;
    }

    return d_openStorage.object();
}
#endif

CloseStorageCommand& Command::makeCloseStorage()
{
    if (SELECTION_ID_CLOSE_STORAGE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_closeStorage.object());
    }
    else {
        reset();
        new (d_closeStorage.buffer()) CloseStorageCommand();
        d_selectionId = SELECTION_ID_CLOSE_STORAGE;
    }

    return d_closeStorage.object();
}

CloseStorageCommand&
Command::makeCloseStorage(const CloseStorageCommand& value)
{
    if (SELECTION_ID_CLOSE_STORAGE == d_selectionId) {
        d_closeStorage.object() = value;
    }
    else {
        reset();
        new (d_closeStorage.buffer()) CloseStorageCommand(value);
        d_selectionId = SELECTION_ID_CLOSE_STORAGE;
    }

    return d_closeStorage.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CloseStorageCommand& Command::makeCloseStorage(CloseStorageCommand&& value)
{
    if (SELECTION_ID_CLOSE_STORAGE == d_selectionId) {
        d_closeStorage.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_closeStorage.buffer()) CloseStorageCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_CLOSE_STORAGE;
    }

    return d_closeStorage.object();
}
#endif

MetadataCommand& Command::makeMetadata()
{
    if (SELECTION_ID_METADATA == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_metadata.object());
    }
    else {
        reset();
        new (d_metadata.buffer()) MetadataCommand();
        d_selectionId = SELECTION_ID_METADATA;
    }

    return d_metadata.object();
}

MetadataCommand& Command::makeMetadata(const MetadataCommand& value)
{
    if (SELECTION_ID_METADATA == d_selectionId) {
        d_metadata.object() = value;
    }
    else {
        reset();
        new (d_metadata.buffer()) MetadataCommand(value);
        d_selectionId = SELECTION_ID_METADATA;
    }

    return d_metadata.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
MetadataCommand& Command::makeMetadata(MetadataCommand&& value)
{
    if (SELECTION_ID_METADATA == d_selectionId) {
        d_metadata.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_metadata.buffer()) MetadataCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_METADATA;
    }

    return d_metadata.object();
}
#endif

ListQueuesCommand& Command::makeListQueues()
{
    if (SELECTION_ID_LIST_QUEUES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_listQueues.object());
    }
    else {
        reset();
        new (d_listQueues.buffer()) ListQueuesCommand();
        d_selectionId = SELECTION_ID_LIST_QUEUES;
    }

    return d_listQueues.object();
}

ListQueuesCommand& Command::makeListQueues(const ListQueuesCommand& value)
{
    if (SELECTION_ID_LIST_QUEUES == d_selectionId) {
        d_listQueues.object() = value;
    }
    else {
        reset();
        new (d_listQueues.buffer()) ListQueuesCommand(value);
        d_selectionId = SELECTION_ID_LIST_QUEUES;
    }

    return d_listQueues.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ListQueuesCommand& Command::makeListQueues(ListQueuesCommand&& value)
{
    if (SELECTION_ID_LIST_QUEUES == d_selectionId) {
        d_listQueues.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_listQueues.buffer()) ListQueuesCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_LIST_QUEUES;
    }

    return d_listQueues.object();
}
#endif

DumpQueueCommand& Command::makeDumpQueue()
{
    if (SELECTION_ID_DUMP_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_dumpQueue.object());
    }
    else {
        reset();
        new (d_dumpQueue.buffer()) DumpQueueCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_DUMP_QUEUE;
    }

    return d_dumpQueue.object();
}

DumpQueueCommand& Command::makeDumpQueue(const DumpQueueCommand& value)
{
    if (SELECTION_ID_DUMP_QUEUE == d_selectionId) {
        d_dumpQueue.object() = value;
    }
    else {
        reset();
        new (d_dumpQueue.buffer()) DumpQueueCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DUMP_QUEUE;
    }

    return d_dumpQueue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DumpQueueCommand& Command::makeDumpQueue(DumpQueueCommand&& value)
{
    if (SELECTION_ID_DUMP_QUEUE == d_selectionId) {
        d_dumpQueue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_dumpQueue.buffer())
            DumpQueueCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DUMP_QUEUE;
    }

    return d_dumpQueue.object();
}
#endif

DataCommand& Command::makeData()
{
    if (SELECTION_ID_DATA == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_data.object());
    }
    else {
        reset();
        new (d_data.buffer()) DataCommand();
        d_selectionId = SELECTION_ID_DATA;
    }

    return d_data.object();
}

DataCommand& Command::makeData(const DataCommand& value)
{
    if (SELECTION_ID_DATA == d_selectionId) {
        d_data.object() = value;
    }
    else {
        reset();
        new (d_data.buffer()) DataCommand(value);
        d_selectionId = SELECTION_ID_DATA;
    }

    return d_data.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DataCommand& Command::makeData(DataCommand&& value)
{
    if (SELECTION_ID_DATA == d_selectionId) {
        d_data.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_data.buffer()) DataCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_DATA;
    }

    return d_data.object();
}
#endif

QlistCommand& Command::makeQlist()
{
    if (SELECTION_ID_QLIST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_qlist.object());
    }
    else {
        reset();
        new (d_qlist.buffer()) QlistCommand();
        d_selectionId = SELECTION_ID_QLIST;
    }

    return d_qlist.object();
}

QlistCommand& Command::makeQlist(const QlistCommand& value)
{
    if (SELECTION_ID_QLIST == d_selectionId) {
        d_qlist.object() = value;
    }
    else {
        reset();
        new (d_qlist.buffer()) QlistCommand(value);
        d_selectionId = SELECTION_ID_QLIST;
    }

    return d_qlist.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QlistCommand& Command::makeQlist(QlistCommand&& value)
{
    if (SELECTION_ID_QLIST == d_selectionId) {
        d_qlist.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_qlist.buffer()) QlistCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_QLIST;
    }

    return d_qlist.object();
}
#endif

JournalCommand& Command::makeJournal()
{
    if (SELECTION_ID_JOURNAL == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_journal.object());
    }
    else {
        reset();
        new (d_journal.buffer()) JournalCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_JOURNAL;
    }

    return d_journal.object();
}

JournalCommand& Command::makeJournal(const JournalCommand& value)
{
    if (SELECTION_ID_JOURNAL == d_selectionId) {
        d_journal.object() = value;
    }
    else {
        reset();
        new (d_journal.buffer()) JournalCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_JOURNAL;
    }

    return d_journal.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
JournalCommand& Command::makeJournal(JournalCommand&& value)
{
    if (SELECTION_ID_JOURNAL == d_selectionId) {
        d_journal.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_journal.buffer())
            JournalCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_JOURNAL;
    }

    return d_journal.object();
}
#endif

// ACCESSORS

bsl::ostream&
Command::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_START: {
        printer.printAttribute("start", d_start.object());
    } break;
    case SELECTION_ID_STOP: {
        printer.printAttribute("stop", d_stop.object());
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        printer.printAttribute("openQueue", d_openQueue.object());
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE: {
        printer.printAttribute("configureQueue", d_configureQueue.object());
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        printer.printAttribute("closeQueue", d_closeQueue.object());
    } break;
    case SELECTION_ID_POST: {
        printer.printAttribute("post", d_post.object());
    } break;
    case SELECTION_ID_LIST: {
        printer.printAttribute("list", d_list.object());
    } break;
    case SELECTION_ID_CONFIRM: {
        printer.printAttribute("confirm", d_confirm.object());
    } break;
    case SELECTION_ID_BATCH_POST: {
        printer.printAttribute("batchPost", d_batchPost.object());
    } break;
    case SELECTION_ID_LOAD_POST: {
        printer.printAttribute("loadPost", d_loadPost.object());
    } break;
    case SELECTION_ID_OPEN_STORAGE: {
        printer.printAttribute("openStorage", d_openStorage.object());
    } break;
    case SELECTION_ID_CLOSE_STORAGE: {
        printer.printAttribute("closeStorage", d_closeStorage.object());
    } break;
    case SELECTION_ID_METADATA: {
        printer.printAttribute("metadata", d_metadata.object());
    } break;
    case SELECTION_ID_LIST_QUEUES: {
        printer.printAttribute("listQueues", d_listQueues.object());
    } break;
    case SELECTION_ID_DUMP_QUEUE: {
        printer.printAttribute("dumpQueue", d_dumpQueue.object());
    } break;
    case SELECTION_ID_DATA: {
        printer.printAttribute("data", d_data.object());
    } break;
    case SELECTION_ID_QLIST: {
        printer.printAttribute("qlist", d_qlist.object());
    } break;
    case SELECTION_ID_JOURNAL: {
        printer.printAttribute("journal", d_journal.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* Command::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_START:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_START].name();
    case SELECTION_ID_STOP:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STOP].name();
    case SELECTION_ID_OPEN_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_QUEUE].name();
    case SELECTION_ID_CONFIGURE_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIGURE_QUEUE].name();
    case SELECTION_ID_CLOSE_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_QUEUE].name();
    case SELECTION_ID_POST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_POST].name();
    case SELECTION_ID_LIST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST].name();
    case SELECTION_ID_CONFIRM:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIRM].name();
    case SELECTION_ID_BATCH_POST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_BATCH_POST].name();
    case SELECTION_ID_LOAD_POST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LOAD_POST].name();
    case SELECTION_ID_OPEN_STORAGE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_STORAGE].name();
    case SELECTION_ID_CLOSE_STORAGE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_STORAGE].name();
    case SELECTION_ID_METADATA:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_METADATA].name();
    case SELECTION_ID_LIST_QUEUES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_QUEUES].name();
    case SELECTION_ID_DUMP_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP_QUEUE].name();
    case SELECTION_ID_DATA:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DATA].name();
    case SELECTION_ID_QLIST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QLIST].name();
    case SELECTION_ID_JOURNAL:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_JOURNAL].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}
}  // close package namespace
}  // close enterprise namespace

// GENERATED BY BLP_BAS_CODEGEN_2025.10.09.2
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package m_bmqtool --msgComponent messages bmqtoolcmd.xsd
