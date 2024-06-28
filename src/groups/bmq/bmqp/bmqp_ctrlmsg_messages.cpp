// Copyright 2023 Bloomberg Finance L.P.
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

// bmqp_ctrlmsg_messages.cpp         *DO NOT EDIT*         @generated -*-C++-*-

#include <bmqp_ctrlmsg_messages.h>

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
namespace bmqp_ctrlmsg {

// ------------------
// class AdminCommand
// ------------------

// CONSTANTS

const char AdminCommand::CLASS_NAME[] = "AdminCommand";

const bdlat_AttributeInfo AdminCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_COMMAND,
     "command",
     sizeof("command") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_REROUTED,
     "rerouted",
     sizeof("rerouted") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* AdminCommand::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            AdminCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* AdminCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_COMMAND:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND];
    case ATTRIBUTE_ID_REROUTED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REROUTED];
    default: return 0;
    }
}

// CREATORS

AdminCommand::AdminCommand(bslma::Allocator* basicAllocator)
: d_command(basicAllocator)
, d_rerouted()
{
}

AdminCommand::AdminCommand(const AdminCommand& original,
                           bslma::Allocator*   basicAllocator)
: d_command(original.d_command, basicAllocator)
, d_rerouted(original.d_rerouted)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AdminCommand::AdminCommand(AdminCommand&& original) noexcept
: d_command(bsl::move(original.d_command)),
  d_rerouted(bsl::move(original.d_rerouted))
{
}

AdminCommand::AdminCommand(AdminCommand&&    original,
                           bslma::Allocator* basicAllocator)
: d_command(bsl::move(original.d_command), basicAllocator)
, d_rerouted(bsl::move(original.d_rerouted))
{
}
#endif

AdminCommand::~AdminCommand()
{
}

// MANIPULATORS

AdminCommand& AdminCommand::operator=(const AdminCommand& rhs)
{
    if (this != &rhs) {
        d_command  = rhs.d_command;
        d_rerouted = rhs.d_rerouted;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AdminCommand& AdminCommand::operator=(AdminCommand&& rhs)
{
    if (this != &rhs) {
        d_command  = bsl::move(rhs.d_command);
        d_rerouted = bsl::move(rhs.d_rerouted);
    }

    return *this;
}
#endif

void AdminCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_command);
    bdlat_ValueTypeFunctions::reset(&d_rerouted);
}

// ACCESSORS

bsl::ostream&
AdminCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("command", this->command());
    printer.printAttribute("rerouted", this->rerouted());
    printer.end();
    return stream;
}

// --------------------------
// class AdminCommandResponse
// --------------------------

// CONSTANTS

const char AdminCommandResponse::CLASS_NAME[] = "AdminCommandResponse";

const bdlat_AttributeInfo AdminCommandResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_TEXT,
     "text",
     sizeof("text") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
AdminCommandResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            AdminCommandResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* AdminCommandResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_TEXT: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TEXT];
    default: return 0;
    }
}

// CREATORS

AdminCommandResponse::AdminCommandResponse(bslma::Allocator* basicAllocator)
: d_text(basicAllocator)
{
}

AdminCommandResponse::AdminCommandResponse(
    const AdminCommandResponse& original,
    bslma::Allocator*           basicAllocator)
: d_text(original.d_text, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AdminCommandResponse::AdminCommandResponse(AdminCommandResponse&& original)
    noexcept : d_text(bsl::move(original.d_text))
{
}

AdminCommandResponse::AdminCommandResponse(AdminCommandResponse&& original,
                                           bslma::Allocator* basicAllocator)
: d_text(bsl::move(original.d_text), basicAllocator)
{
}
#endif

AdminCommandResponse::~AdminCommandResponse()
{
}

// MANIPULATORS

AdminCommandResponse&
AdminCommandResponse::operator=(const AdminCommandResponse& rhs)
{
    if (this != &rhs) {
        d_text = rhs.d_text;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AdminCommandResponse&
AdminCommandResponse::operator=(AdminCommandResponse&& rhs)
{
    if (this != &rhs) {
        d_text = bsl::move(rhs.d_text);
    }

    return *this;
}
#endif

void AdminCommandResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_text);
}

// ACCESSORS

bsl::ostream& AdminCommandResponse::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("text", this->text());
    printer.end();
    return stream;
}

// ---------------
// class AppIdInfo
// ---------------

// CONSTANTS

const char AppIdInfo::CLASS_NAME[] = "AppIdInfo";

const bdlat_AttributeInfo AppIdInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_APP_KEY,
     "appKey",
     sizeof("appKey") - 1,
     "",
     bdlat_FormattingMode::e_HEX}};

// CLASS METHODS

const bdlat_AttributeInfo* AppIdInfo::lookupAttributeInfo(const char* name,
                                                          int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            AppIdInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* AppIdInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_APP_KEY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_KEY];
    default: return 0;
    }
}

// CREATORS

AppIdInfo::AppIdInfo(bslma::Allocator* basicAllocator)
: d_appKey(basicAllocator)
, d_appId(basicAllocator)
{
}

AppIdInfo::AppIdInfo(const AppIdInfo&  original,
                     bslma::Allocator* basicAllocator)
: d_appKey(original.d_appKey, basicAllocator)
, d_appId(original.d_appId, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AppIdInfo::AppIdInfo(AppIdInfo&& original) noexcept
: d_appKey(bsl::move(original.d_appKey)),
  d_appId(bsl::move(original.d_appId))
{
}

AppIdInfo::AppIdInfo(AppIdInfo&& original, bslma::Allocator* basicAllocator)
: d_appKey(bsl::move(original.d_appKey), basicAllocator)
, d_appId(bsl::move(original.d_appId), basicAllocator)
{
}
#endif

AppIdInfo::~AppIdInfo()
{
}

// MANIPULATORS

AppIdInfo& AppIdInfo::operator=(const AppIdInfo& rhs)
{
    if (this != &rhs) {
        d_appId  = rhs.d_appId;
        d_appKey = rhs.d_appKey;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AppIdInfo& AppIdInfo::operator=(AppIdInfo&& rhs)
{
    if (this != &rhs) {
        d_appId  = bsl::move(rhs.d_appId);
        d_appKey = bsl::move(rhs.d_appKey);
    }

    return *this;
}
#endif

void AppIdInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_appId);
    bdlat_ValueTypeFunctions::reset(&d_appKey);
}

// ACCESSORS

bsl::ostream&
AppIdInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("appId", this->appId());
    {
        bool multilineFlag = (0 <= spacesPerLevel);
        bdlb::Print::indent(stream, level + 1, spacesPerLevel);
        stream << (multilineFlag ? "" : " ");
        stream << "appKey = [ ";
        bdlb::Print::singleLineHexDump(stream,
                                       this->appKey().begin(),
                                       this->appKey().end());
        stream << " ]" << (multilineFlag ? "\n" : "");
    }
    printer.end();
    return stream;
}

// --------------------
// class ClientLanguage
// --------------------

// CONSTANTS

const char ClientLanguage::CLASS_NAME[] = "ClientLanguage";

const bdlat_EnumeratorInfo ClientLanguage::ENUMERATOR_INFO_ARRAY[] = {
    {ClientLanguage::E_UNKNOWN, "E_UNKNOWN", sizeof("E_UNKNOWN") - 1, ""},
    {ClientLanguage::E_CPP, "E_CPP", sizeof("E_CPP") - 1, ""},
    {ClientLanguage::E_JAVA, "E_JAVA", sizeof("E_JAVA") - 1, ""}};

// CLASS METHODS

int ClientLanguage::fromInt(ClientLanguage::Value* result, int number)
{
    switch (number) {
    case ClientLanguage::E_UNKNOWN:
    case ClientLanguage::E_CPP:
    case ClientLanguage::E_JAVA:
        *result = static_cast<ClientLanguage::Value>(number);
        return 0;
    default: return -1;
    }
}

int ClientLanguage::fromString(ClientLanguage::Value* result,
                               const char*            string,
                               int                    stringLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            ClientLanguage::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<ClientLanguage::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* ClientLanguage::toString(ClientLanguage::Value value)
{
    switch (value) {
    case E_UNKNOWN: {
        return "E_UNKNOWN";
    }
    case E_CPP: {
        return "E_CPP";
    }
    case E_JAVA: {
        return "E_JAVA";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ----------------
// class ClientType
// ----------------

// CONSTANTS

const char ClientType::CLASS_NAME[] = "ClientType";

const bdlat_EnumeratorInfo ClientType::ENUMERATOR_INFO_ARRAY[] = {
    {ClientType::E_UNKNOWN, "E_UNKNOWN", sizeof("E_UNKNOWN") - 1, ""},
    {ClientType::E_TCPCLIENT, "E_TCPCLIENT", sizeof("E_TCPCLIENT") - 1, ""},
    {ClientType::E_TCPBROKER, "E_TCPBROKER", sizeof("E_TCPBROKER") - 1, ""},
    {ClientType::E_TCPADMIN, "E_TCPADMIN", sizeof("E_TCPADMIN") - 1, ""}};

// CLASS METHODS

int ClientType::fromInt(ClientType::Value* result, int number)
{
    switch (number) {
    case ClientType::E_UNKNOWN:
    case ClientType::E_TCPCLIENT:
    case ClientType::E_TCPBROKER:
    case ClientType::E_TCPADMIN:
        *result = static_cast<ClientType::Value>(number);
        return 0;
    default: return -1;
    }
}

int ClientType::fromString(ClientType::Value* result,
                           const char*        string,
                           int                stringLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            ClientType::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<ClientType::Value>(enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* ClientType::toString(ClientType::Value value)
{
    switch (value) {
    case E_UNKNOWN: {
        return "E_UNKNOWN";
    }
    case E_TCPCLIENT: {
        return "E_TCPCLIENT";
    }
    case E_TCPBROKER: {
        return "E_TCPBROKER";
    }
    case E_TCPADMIN: {
        return "E_TCPADMIN";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ------------------------
// class CloseQueueResponse
// ------------------------

// CONSTANTS

const char CloseQueueResponse::CLASS_NAME[] = "CloseQueueResponse";

// CLASS METHODS

const bdlat_AttributeInfo*
CloseQueueResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* CloseQueueResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void CloseQueueResponse::reset()
{
}

// ACCESSORS

bsl::ostream& CloseQueueResponse::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ------------------------
// class ClusterSyncRequest
// ------------------------

// CONSTANTS

const char ClusterSyncRequest::CLASS_NAME[] = "ClusterSyncRequest";

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterSyncRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* ClusterSyncRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void ClusterSyncRequest::reset()
{
}

// ACCESSORS

bsl::ostream& ClusterSyncRequest::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// -------------------------
// class ClusterSyncResponse
// -------------------------

// CONSTANTS

const char ClusterSyncResponse::CLASS_NAME[] = "ClusterSyncResponse";

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterSyncResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* ClusterSyncResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void ClusterSyncResponse::reset()
{
}

// ACCESSORS

bsl::ostream& ClusterSyncResponse::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ------------------
// class ConsumerInfo
// ------------------

// CONSTANTS

const char ConsumerInfo::CLASS_NAME[] = "ConsumerInfo";

const bsls::Types::Int64
    ConsumerInfo::DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES = 0;

const bsls::Types::Int64
    ConsumerInfo::DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES = 0;

const int ConsumerInfo::DEFAULT_INITIALIZER_CONSUMER_PRIORITY = -2147483648;

const int ConsumerInfo::DEFAULT_INITIALIZER_CONSUMER_PRIORITY_COUNT = 0;

const bdlat_AttributeInfo ConsumerInfo::ATTRIBUTE_INFO_ARRAY[] = {
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
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CONSUMER_PRIORITY_COUNT,
     "consumerPriorityCount",
     sizeof("consumerPriorityCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* ConsumerInfo::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ConsumerInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ConsumerInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES];
    case ATTRIBUTE_ID_CONSUMER_PRIORITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY];
    case ATTRIBUTE_ID_CONSUMER_PRIORITY_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY_COUNT];
    default: return 0;
    }
}

// CREATORS

ConsumerInfo::ConsumerInfo()
: d_maxUnconfirmedMessages(DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES)
, d_maxUnconfirmedBytes(DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES)
, d_consumerPriority(DEFAULT_INITIALIZER_CONSUMER_PRIORITY)
, d_consumerPriorityCount(DEFAULT_INITIALIZER_CONSUMER_PRIORITY_COUNT)
{
}

// MANIPULATORS

void ConsumerInfo::reset()
{
    d_maxUnconfirmedMessages = DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES;
    d_maxUnconfirmedBytes    = DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES;
    d_consumerPriority       = DEFAULT_INITIALIZER_CONSUMER_PRIORITY;
    d_consumerPriorityCount  = DEFAULT_INITIALIZER_CONSUMER_PRIORITY_COUNT;
}

// ACCESSORS

bsl::ostream&
ConsumerInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("maxUnconfirmedMessages",
                           this->maxUnconfirmedMessages());
    printer.printAttribute("maxUnconfirmedBytes", this->maxUnconfirmedBytes());
    printer.printAttribute("consumerPriority", this->consumerPriority());
    printer.printAttribute("consumerPriorityCount",
                           this->consumerPriorityCount());
    printer.end();
    return stream;
}

// ----------------
// class Disconnect
// ----------------

// CONSTANTS

const char Disconnect::CLASS_NAME[] = "Disconnect";

// CLASS METHODS

const bdlat_AttributeInfo* Disconnect::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* Disconnect::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void Disconnect::reset()
{
}

// ACCESSORS

bsl::ostream& Disconnect::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ------------------------
// class DisconnectResponse
// ------------------------

// CONSTANTS

const char DisconnectResponse::CLASS_NAME[] = "DisconnectResponse";

// CLASS METHODS

const bdlat_AttributeInfo*
DisconnectResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* DisconnectResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void DisconnectResponse::reset()
{
}

// ACCESSORS

bsl::ostream& DisconnectResponse::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// --------------------
// class DumpActionType
// --------------------

// CONSTANTS

const char DumpActionType::CLASS_NAME[] = "DumpActionType";

const bdlat_EnumeratorInfo DumpActionType::ENUMERATOR_INFO_ARRAY[] = {
    {DumpActionType::E_ON, "E_ON", sizeof("E_ON") - 1, ""},
    {DumpActionType::E_OFF, "E_OFF", sizeof("E_OFF") - 1, ""},
    {DumpActionType::E_MESSAGE_COUNT,
     "E_MESSAGE_COUNT",
     sizeof("E_MESSAGE_COUNT") - 1,
     ""},
    {DumpActionType::E_TIME_IN_SECONDS,
     "E_TIME_IN_SECONDS",
     sizeof("E_TIME_IN_SECONDS") - 1,
     ""}};

// CLASS METHODS

int DumpActionType::fromInt(DumpActionType::Value* result, int number)
{
    switch (number) {
    case DumpActionType::E_ON:
    case DumpActionType::E_OFF:
    case DumpActionType::E_MESSAGE_COUNT:
    case DumpActionType::E_TIME_IN_SECONDS:
        *result = static_cast<DumpActionType::Value>(number);
        return 0;
    default: return -1;
    }
}

int DumpActionType::fromString(DumpActionType::Value* result,
                               const char*            string,
                               int                    stringLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            DumpActionType::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<DumpActionType::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* DumpActionType::toString(DumpActionType::Value value)
{
    switch (value) {
    case E_ON: {
        return "E_ON";
    }
    case E_OFF: {
        return "E_OFF";
    }
    case E_MESSAGE_COUNT: {
        return "E_MESSAGE_COUNT";
    }
    case E_TIME_IN_SECONDS: {
        return "E_TIME_IN_SECONDS";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -----------------
// class DumpMsgType
// -----------------

// CONSTANTS

const char DumpMsgType::CLASS_NAME[] = "DumpMsgType";

const bdlat_EnumeratorInfo DumpMsgType::ENUMERATOR_INFO_ARRAY[] = {
    {DumpMsgType::E_INCOMING, "E_INCOMING", sizeof("E_INCOMING") - 1, ""},
    {DumpMsgType::E_OUTGOING, "E_OUTGOING", sizeof("E_OUTGOING") - 1, ""},
    {DumpMsgType::E_PUSH, "E_PUSH", sizeof("E_PUSH") - 1, ""},
    {DumpMsgType::E_ACK, "E_ACK", sizeof("E_ACK") - 1, ""},
    {DumpMsgType::E_PUT, "E_PUT", sizeof("E_PUT") - 1, ""},
    {DumpMsgType::E_CONFIRM, "E_CONFIRM", sizeof("E_CONFIRM") - 1, ""}};

// CLASS METHODS

int DumpMsgType::fromInt(DumpMsgType::Value* result, int number)
{
    switch (number) {
    case DumpMsgType::E_INCOMING:
    case DumpMsgType::E_OUTGOING:
    case DumpMsgType::E_PUSH:
    case DumpMsgType::E_ACK:
    case DumpMsgType::E_PUT:
    case DumpMsgType::E_CONFIRM:
        *result = static_cast<DumpMsgType::Value>(number);
        return 0;
    default: return -1;
    }
}

int DumpMsgType::fromString(DumpMsgType::Value* result,
                            const char*         string,
                            int                 stringLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            DumpMsgType::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<DumpMsgType::Value>(enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* DumpMsgType::toString(DumpMsgType::Value value)
{
    switch (value) {
    case E_INCOMING: {
        return "E_INCOMING";
    }
    case E_OUTGOING: {
        return "E_OUTGOING";
    }
    case E_PUSH: {
        return "E_PUSH";
    }
    case E_ACK: {
        return "E_ACK";
    }
    case E_PUT: {
        return "E_PUT";
    }
    case E_CONFIRM: {
        return "E_CONFIRM";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ----------------------
// class ElectionProposal
// ----------------------

// CONSTANTS

const char ElectionProposal::CLASS_NAME[] = "ElectionProposal";

// CLASS METHODS

const bdlat_AttributeInfo*
ElectionProposal::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* ElectionProposal::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void ElectionProposal::reset()
{
}

// ACCESSORS

bsl::ostream& ElectionProposal::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ----------------------
// class ElectionResponse
// ----------------------

// CONSTANTS

const char ElectionResponse::CLASS_NAME[] = "ElectionResponse";

// CLASS METHODS

const bdlat_AttributeInfo*
ElectionResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* ElectionResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void ElectionResponse::reset()
{
}

// ACCESSORS

bsl::ostream& ElectionResponse::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// -----------------------
// class ElectorNodeStatus
// -----------------------

// CONSTANTS

const char ElectorNodeStatus::CLASS_NAME[] = "ElectorNodeStatus";

const bdlat_AttributeInfo ElectorNodeStatus::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_IS_AVAILABLE,
     "isAvailable",
     sizeof("isAvailable") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ElectorNodeStatus::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ElectorNodeStatus::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ElectorNodeStatus::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_IS_AVAILABLE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_AVAILABLE];
    default: return 0;
    }
}

// CREATORS

ElectorNodeStatus::ElectorNodeStatus()
: d_isAvailable()
{
}

// MANIPULATORS

void ElectorNodeStatus::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_isAvailable);
}

// ACCESSORS

bsl::ostream& ElectorNodeStatus::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("isAvailable", this->isAvailable());
    printer.end();
    return stream;
}

// -----------------------
// class ExpressionVersion
// -----------------------

// CONSTANTS

const char ExpressionVersion::CLASS_NAME[] = "ExpressionVersion";

const bdlat_EnumeratorInfo ExpressionVersion::ENUMERATOR_INFO_ARRAY[] = {
    {ExpressionVersion::E_UNDEFINED,
     "E_UNDEFINED",
     sizeof("E_UNDEFINED") - 1,
     ""},
    {ExpressionVersion::E_VERSION_1,
     "E_VERSION_1",
     sizeof("E_VERSION_1") - 1,
     ""}};

// CLASS METHODS

int ExpressionVersion::fromInt(ExpressionVersion::Value* result, int number)
{
    switch (number) {
    case ExpressionVersion::E_UNDEFINED:
    case ExpressionVersion::E_VERSION_1:
        *result = static_cast<ExpressionVersion::Value>(number);
        return 0;
    default: return -1;
    }
}

int ExpressionVersion::fromString(ExpressionVersion::Value* result,
                                  const char*               string,
                                  int                       stringLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            ExpressionVersion::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<ExpressionVersion::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* ExpressionVersion::toString(ExpressionVersion::Value value)
{
    switch (value) {
    case E_UNDEFINED: {
        return "E_UNDEFINED";
    }
    case E_VERSION_1: {
        return "E_VERSION_1";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ---------------------------------
// class FollowerClusterStateRequest
// ---------------------------------

// CONSTANTS

const char FollowerClusterStateRequest::CLASS_NAME[] =
    "FollowerClusterStateRequest";

// CLASS METHODS

const bdlat_AttributeInfo*
FollowerClusterStateRequest::lookupAttributeInfo(const char* name,
                                                 int         nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo*
FollowerClusterStateRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void FollowerClusterStateRequest::reset()
{
}

// ACCESSORS

bsl::ostream&
FollowerClusterStateRequest::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ------------------------
// class FollowerLSNRequest
// ------------------------

// CONSTANTS

const char FollowerLSNRequest::CLASS_NAME[] = "FollowerLSNRequest";

// CLASS METHODS

const bdlat_AttributeInfo*
FollowerLSNRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* FollowerLSNRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void FollowerLSNRequest::reset()
{
}

// ACCESSORS

bsl::ostream& FollowerLSNRequest::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// --------------
// class GuidInfo
// --------------

// CONSTANTS

const char GuidInfo::CLASS_NAME[] = "GuidInfo";

const char GuidInfo::DEFAULT_INITIALIZER_CLIENT_ID[] = "";

const bsls::Types::Int64
    GuidInfo::DEFAULT_INITIALIZER_NANO_SECONDS_FROM_EPOCH = 0;

const bdlat_AttributeInfo GuidInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLIENT_ID,
     "clientId",
     sizeof("clientId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NANO_SECONDS_FROM_EPOCH,
     "nanoSecondsFromEpoch",
     sizeof("nanoSecondsFromEpoch") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* GuidInfo::lookupAttributeInfo(const char* name,
                                                         int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            GuidInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* GuidInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLIENT_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT_ID];
    case ATTRIBUTE_ID_NANO_SECONDS_FROM_EPOCH:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NANO_SECONDS_FROM_EPOCH];
    default: return 0;
    }
}

// CREATORS

GuidInfo::GuidInfo(bslma::Allocator* basicAllocator)
: d_nanoSecondsFromEpoch(DEFAULT_INITIALIZER_NANO_SECONDS_FROM_EPOCH)
, d_clientId(DEFAULT_INITIALIZER_CLIENT_ID, basicAllocator)
{
}

GuidInfo::GuidInfo(const GuidInfo& original, bslma::Allocator* basicAllocator)
: d_nanoSecondsFromEpoch(original.d_nanoSecondsFromEpoch)
, d_clientId(original.d_clientId, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
GuidInfo::GuidInfo(GuidInfo&& original) noexcept
: d_nanoSecondsFromEpoch(bsl::move(original.d_nanoSecondsFromEpoch)),
  d_clientId(bsl::move(original.d_clientId))
{
}

GuidInfo::GuidInfo(GuidInfo&& original, bslma::Allocator* basicAllocator)
: d_nanoSecondsFromEpoch(bsl::move(original.d_nanoSecondsFromEpoch))
, d_clientId(bsl::move(original.d_clientId), basicAllocator)
{
}
#endif

GuidInfo::~GuidInfo()
{
}

// MANIPULATORS

GuidInfo& GuidInfo::operator=(const GuidInfo& rhs)
{
    if (this != &rhs) {
        d_clientId             = rhs.d_clientId;
        d_nanoSecondsFromEpoch = rhs.d_nanoSecondsFromEpoch;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
GuidInfo& GuidInfo::operator=(GuidInfo&& rhs)
{
    if (this != &rhs) {
        d_clientId             = bsl::move(rhs.d_clientId);
        d_nanoSecondsFromEpoch = bsl::move(rhs.d_nanoSecondsFromEpoch);
    }

    return *this;
}
#endif

void GuidInfo::reset()
{
    d_clientId             = DEFAULT_INITIALIZER_CLIENT_ID;
    d_nanoSecondsFromEpoch = DEFAULT_INITIALIZER_NANO_SECONDS_FROM_EPOCH;
}

// ACCESSORS

bsl::ostream&
GuidInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clientId", this->clientId());
    printer.printAttribute("nanoSecondsFromEpoch",
                           this->nanoSecondsFromEpoch());
    printer.end();
    return stream;
}

// -----------------------
// class HeartbeatResponse
// -----------------------

// CONSTANTS

const char HeartbeatResponse::CLASS_NAME[] = "HeartbeatResponse";

// CLASS METHODS

const bdlat_AttributeInfo*
HeartbeatResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* HeartbeatResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void HeartbeatResponse::reset()
{
}

// ACCESSORS

bsl::ostream& HeartbeatResponse::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ---------------------
// class LeaderHeartbeat
// ---------------------

// CONSTANTS

const char LeaderHeartbeat::CLASS_NAME[] = "LeaderHeartbeat";

// CLASS METHODS

const bdlat_AttributeInfo*
LeaderHeartbeat::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* LeaderHeartbeat::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void LeaderHeartbeat::reset()
{
}

// ACCESSORS

bsl::ostream& LeaderHeartbeat::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ---------------------------
// class LeaderMessageSequence
// ---------------------------

// CONSTANTS

const char LeaderMessageSequence::CLASS_NAME[] = "LeaderMessageSequence";

const bdlat_AttributeInfo LeaderMessageSequence::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ELECTOR_TERM,
     "electorTerm",
     sizeof("electorTerm") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
LeaderMessageSequence::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LeaderMessageSequence::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* LeaderMessageSequence::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ELECTOR_TERM:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTOR_TERM];
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

LeaderMessageSequence::LeaderMessageSequence()
: d_electorTerm()
, d_sequenceNumber()
{
}

// MANIPULATORS

void LeaderMessageSequence::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_electorTerm);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
}

// ACCESSORS

bsl::ostream& LeaderMessageSequence::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("electorTerm", this->electorTerm());
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.end();
    return stream;
}

// -------------------
// class LeaderPassive
// -------------------

// CONSTANTS

const char LeaderPassive::CLASS_NAME[] = "LeaderPassive";

// CLASS METHODS

const bdlat_AttributeInfo* LeaderPassive::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* LeaderPassive::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void LeaderPassive::reset()
{
}

// ACCESSORS

bsl::ostream& LeaderPassive::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// -------------------------
// class LeaderSyncDataQuery
// -------------------------

// CONSTANTS

const char LeaderSyncDataQuery::CLASS_NAME[] = "LeaderSyncDataQuery";

// CLASS METHODS

const bdlat_AttributeInfo*
LeaderSyncDataQuery::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* LeaderSyncDataQuery::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void LeaderSyncDataQuery::reset()
{
}

// ACCESSORS

bsl::ostream& LeaderSyncDataQuery::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// --------------------------
// class LeaderSyncStateQuery
// --------------------------

// CONSTANTS

const char LeaderSyncStateQuery::CLASS_NAME[] = "LeaderSyncStateQuery";

// CLASS METHODS

const bdlat_AttributeInfo*
LeaderSyncStateQuery::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* LeaderSyncStateQuery::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void LeaderSyncStateQuery::reset()
{
}

// ACCESSORS

bsl::ostream& LeaderSyncStateQuery::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// -----------------------------------
// class LeadershipCessionNotification
// -----------------------------------

// CONSTANTS

const char LeadershipCessionNotification::CLASS_NAME[] =
    "LeadershipCessionNotification";

// CLASS METHODS

const bdlat_AttributeInfo*
LeadershipCessionNotification::lookupAttributeInfo(const char* name,
                                                   int         nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo*
LeadershipCessionNotification::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void LeadershipCessionNotification::reset()
{
}

// ACCESSORS

bsl::ostream&
LeadershipCessionNotification::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ----------------
// class NodeStatus
// ----------------

// CONSTANTS

const char NodeStatus::CLASS_NAME[] = "NodeStatus";

const bdlat_EnumeratorInfo NodeStatus::ENUMERATOR_INFO_ARRAY[] = {
    {NodeStatus::E_UNKNOWN, "E_UNKNOWN", sizeof("E_UNKNOWN") - 1, ""},
    {NodeStatus::E_STARTING, "E_STARTING", sizeof("E_STARTING") - 1, ""},
    {NodeStatus::E_AVAILABLE, "E_AVAILABLE", sizeof("E_AVAILABLE") - 1, ""},
    {NodeStatus::E_STOPPING, "E_STOPPING", sizeof("E_STOPPING") - 1, ""},
    {NodeStatus::E_UNAVAILABLE,
     "E_UNAVAILABLE",
     sizeof("E_UNAVAILABLE") - 1,
     ""}};

// CLASS METHODS

int NodeStatus::fromInt(NodeStatus::Value* result, int number)
{
    switch (number) {
    case NodeStatus::E_UNKNOWN:
    case NodeStatus::E_STARTING:
    case NodeStatus::E_AVAILABLE:
    case NodeStatus::E_STOPPING:
    case NodeStatus::E_UNAVAILABLE:
        *result = static_cast<NodeStatus::Value>(number);
        return 0;
    default: return -1;
    }
}

int NodeStatus::fromString(NodeStatus::Value* result,
                           const char*        string,
                           int                stringLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            NodeStatus::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<NodeStatus::Value>(enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* NodeStatus::toString(NodeStatus::Value value)
{
    switch (value) {
    case E_UNKNOWN: {
        return "E_UNKNOWN";
    }
    case E_STARTING: {
        return "E_STARTING";
    }
    case E_AVAILABLE: {
        return "E_AVAILABLE";
    }
    case E_STOPPING: {
        return "E_STOPPING";
    }
    case E_UNAVAILABLE: {
        return "E_UNAVAILABLE";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// --------------------------
// class PartitionPrimaryInfo
// --------------------------

// CONSTANTS

const char PartitionPrimaryInfo::CLASS_NAME[] = "PartitionPrimaryInfo";

const bdlat_AttributeInfo PartitionPrimaryInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIMARY_NODE_ID,
     "primaryNodeId",
     sizeof("primaryNodeId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIMARY_LEASE_ID,
     "primaryLeaseId",
     sizeof("primaryLeaseId") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionPrimaryInfo::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionPrimaryInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PartitionPrimaryInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_PRIMARY_NODE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_NODE_ID];
    case ATTRIBUTE_ID_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_LEASE_ID];
    default: return 0;
    }
}

// CREATORS

PartitionPrimaryInfo::PartitionPrimaryInfo()
: d_primaryLeaseId()
, d_partitionId()
, d_primaryNodeId()
{
}

// MANIPULATORS

void PartitionPrimaryInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_primaryNodeId);
    bdlat_ValueTypeFunctions::reset(&d_primaryLeaseId);
}

// ACCESSORS

bsl::ostream& PartitionPrimaryInfo::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("primaryNodeId", this->primaryNodeId());
    printer.printAttribute("primaryLeaseId", this->primaryLeaseId());
    printer.end();
    return stream;
}

// -----------------------------
// class PartitionSequenceNumber
// -----------------------------

// CONSTANTS

const char PartitionSequenceNumber::CLASS_NAME[] = "PartitionSequenceNumber";

const bdlat_AttributeInfo PartitionSequenceNumber::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PRIMARY_LEASE_ID,
     "primaryLeaseId",
     sizeof("primaryLeaseId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionSequenceNumber::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionSequenceNumber::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PartitionSequenceNumber::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

PartitionSequenceNumber::PartitionSequenceNumber()
: d_sequenceNumber()
, d_primaryLeaseId()
{
}

// MANIPULATORS

void PartitionSequenceNumber::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_primaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
}

// ACCESSORS

bsl::ostream& PartitionSequenceNumber::print(bsl::ostream& stream,
                                             int           level,
                                             int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("primaryLeaseId", this->primaryLeaseId());
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.end();
    return stream;
}

// ------------------------------------
// class PartitionSyncDataQueryResponse
// ------------------------------------

// CONSTANTS

const char PartitionSyncDataQueryResponse::CLASS_NAME[] =
    "PartitionSyncDataQueryResponse";

const bdlat_AttributeInfo
    PartitionSyncDataQueryResponse::ATTRIBUTE_INFO_ARRAY[] = {
        {ATTRIBUTE_ID_PARTITION_ID,
         "partitionId",
         sizeof("partitionId") - 1,
         "",
         bdlat_FormattingMode::e_DEC},
        {ATTRIBUTE_ID_END_PRIMARY_LEASE_ID,
         "endPrimaryLeaseId",
         sizeof("endPrimaryLeaseId") - 1,
         "",
         bdlat_FormattingMode::e_DEC},
        {ATTRIBUTE_ID_END_SEQUENCE_NUM,
         "endSequenceNum",
         sizeof("endSequenceNum") - 1,
         "",
         bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionSyncDataQueryResponse::lookupAttributeInfo(const char* name,
                                                    int         nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionSyncDataQueryResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
PartitionSyncDataQueryResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_END_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_END_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_END_SEQUENCE_NUM:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_END_SEQUENCE_NUM];
    default: return 0;
    }
}

// CREATORS

PartitionSyncDataQueryResponse::PartitionSyncDataQueryResponse()
: d_endSequenceNum()
, d_endPrimaryLeaseId()
, d_partitionId()
{
}

// MANIPULATORS

void PartitionSyncDataQueryResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_endPrimaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_endSequenceNum);
}

// ACCESSORS

bsl::ostream& PartitionSyncDataQueryResponse::print(bsl::ostream& stream,
                                                    int           level,
                                                    int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("endPrimaryLeaseId", this->endPrimaryLeaseId());
    printer.printAttribute("endSequenceNum", this->endSequenceNum());
    printer.end();
    return stream;
}

// -----------------------------
// class PartitionSyncStateQuery
// -----------------------------

// CONSTANTS

const char PartitionSyncStateQuery::CLASS_NAME[] = "PartitionSyncStateQuery";

const int PartitionSyncStateQuery::DEFAULT_INITIALIZER_PARTITION_ID = -1;

const bdlat_AttributeInfo PartitionSyncStateQuery::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionSyncStateQuery::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionSyncStateQuery::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PartitionSyncStateQuery::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    default: return 0;
    }
}

// CREATORS

PartitionSyncStateQuery::PartitionSyncStateQuery()
: d_partitionId(DEFAULT_INITIALIZER_PARTITION_ID)
{
}

// MANIPULATORS

void PartitionSyncStateQuery::reset()
{
    d_partitionId = DEFAULT_INITIALIZER_PARTITION_ID;
}

// ACCESSORS

bsl::ostream& PartitionSyncStateQuery::print(bsl::ostream& stream,
                                             int           level,
                                             int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.end();
    return stream;
}

// -------------------
// class PrimaryStatus
// -------------------

// CONSTANTS

const char PrimaryStatus::CLASS_NAME[] = "PrimaryStatus";

const bdlat_EnumeratorInfo PrimaryStatus::ENUMERATOR_INFO_ARRAY[] = {
    {PrimaryStatus::E_UNDEFINED, "E_UNDEFINED", sizeof("E_UNDEFINED") - 1, ""},
    {PrimaryStatus::E_PASSIVE, "E_PASSIVE", sizeof("E_PASSIVE") - 1, ""},
    {PrimaryStatus::E_ACTIVE, "E_ACTIVE", sizeof("E_ACTIVE") - 1, ""}};

// CLASS METHODS

int PrimaryStatus::fromInt(PrimaryStatus::Value* result, int number)
{
    switch (number) {
    case PrimaryStatus::E_UNDEFINED:
    case PrimaryStatus::E_PASSIVE:
    case PrimaryStatus::E_ACTIVE:
        *result = static_cast<PrimaryStatus::Value>(number);
        return 0;
    default: return -1;
    }
}

int PrimaryStatus::fromString(PrimaryStatus::Value* result,
                              const char*           string,
                              int                   stringLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            PrimaryStatus::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<PrimaryStatus::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* PrimaryStatus::toString(PrimaryStatus::Value value)
{
    switch (value) {
    case E_UNDEFINED: {
        return "E_UNDEFINED";
    }
    case E_PASSIVE: {
        return "E_PASSIVE";
    }
    case E_ACTIVE: {
        return "E_ACTIVE";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ----------------------------
// class QueueAssignmentRequest
// ----------------------------

// CONSTANTS

const char QueueAssignmentRequest::CLASS_NAME[] = "QueueAssignmentRequest";

const bdlat_AttributeInfo QueueAssignmentRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_QUEUE_URI,
     "queueUri",
     sizeof("queueUri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueAssignmentRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueAssignmentRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueAssignmentRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_QUEUE_URI:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URI];
    default: return 0;
    }
}

// CREATORS

QueueAssignmentRequest::QueueAssignmentRequest(
    bslma::Allocator* basicAllocator)
: d_queueUri(basicAllocator)
{
}

QueueAssignmentRequest::QueueAssignmentRequest(
    const QueueAssignmentRequest& original,
    bslma::Allocator*             basicAllocator)
: d_queueUri(original.d_queueUri, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueAssignmentRequest::QueueAssignmentRequest(
    QueueAssignmentRequest&& original) noexcept
: d_queueUri(bsl::move(original.d_queueUri))
{
}

QueueAssignmentRequest::QueueAssignmentRequest(
    QueueAssignmentRequest&& original,
    bslma::Allocator*        basicAllocator)
: d_queueUri(bsl::move(original.d_queueUri), basicAllocator)
{
}
#endif

QueueAssignmentRequest::~QueueAssignmentRequest()
{
}

// MANIPULATORS

QueueAssignmentRequest&
QueueAssignmentRequest::operator=(const QueueAssignmentRequest& rhs)
{
    if (this != &rhs) {
        d_queueUri = rhs.d_queueUri;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueAssignmentRequest&
QueueAssignmentRequest::operator=(QueueAssignmentRequest&& rhs)
{
    if (this != &rhs) {
        d_queueUri = bsl::move(rhs.d_queueUri);
    }

    return *this;
}
#endif

void QueueAssignmentRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_queueUri);
}

// ACCESSORS

bsl::ostream& QueueAssignmentRequest::print(bsl::ostream& stream,
                                            int           level,
                                            int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queueUri", this->queueUri());
    printer.end();
    return stream;
}

// ------------------------------
// class QueueUnassignmentRequest
// ------------------------------

// CONSTANTS

const char QueueUnassignmentRequest::CLASS_NAME[] = "QueueUnassignmentRequest";

const bdlat_AttributeInfo QueueUnassignmentRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_QUEUE_URI,
     "queueUri",
     sizeof("queueUri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_QUEUE_KEY,
     "queueKey",
     sizeof("queueKey") - 1,
     "",
     bdlat_FormattingMode::e_HEX}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueUnassignmentRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueUnassignmentRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
QueueUnassignmentRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_QUEUE_URI:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URI];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_QUEUE_KEY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_KEY];
    default: return 0;
    }
}

// CREATORS

QueueUnassignmentRequest::QueueUnassignmentRequest(
    bslma::Allocator* basicAllocator)
: d_queueKey(basicAllocator)
, d_queueUri(basicAllocator)
, d_partitionId()
{
}

QueueUnassignmentRequest::QueueUnassignmentRequest(
    const QueueUnassignmentRequest& original,
    bslma::Allocator*               basicAllocator)
: d_queueKey(original.d_queueKey, basicAllocator)
, d_queueUri(original.d_queueUri, basicAllocator)
, d_partitionId(original.d_partitionId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUnassignmentRequest::QueueUnassignmentRequest(
    QueueUnassignmentRequest&& original) noexcept
: d_queueKey(bsl::move(original.d_queueKey)),
  d_queueUri(bsl::move(original.d_queueUri)),
  d_partitionId(bsl::move(original.d_partitionId))
{
}

QueueUnassignmentRequest::QueueUnassignmentRequest(
    QueueUnassignmentRequest&& original,
    bslma::Allocator*          basicAllocator)
: d_queueKey(bsl::move(original.d_queueKey), basicAllocator)
, d_queueUri(bsl::move(original.d_queueUri), basicAllocator)
, d_partitionId(bsl::move(original.d_partitionId))
{
}
#endif

QueueUnassignmentRequest::~QueueUnassignmentRequest()
{
}

// MANIPULATORS

QueueUnassignmentRequest&
QueueUnassignmentRequest::operator=(const QueueUnassignmentRequest& rhs)
{
    if (this != &rhs) {
        d_queueUri    = rhs.d_queueUri;
        d_partitionId = rhs.d_partitionId;
        d_queueKey    = rhs.d_queueKey;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUnassignmentRequest&
QueueUnassignmentRequest::operator=(QueueUnassignmentRequest&& rhs)
{
    if (this != &rhs) {
        d_queueUri    = bsl::move(rhs.d_queueUri);
        d_partitionId = bsl::move(rhs.d_partitionId);
        d_queueKey    = bsl::move(rhs.d_queueKey);
    }

    return *this;
}
#endif

void QueueUnassignmentRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_queueUri);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_queueKey);
}

// ACCESSORS

bsl::ostream& QueueUnassignmentRequest::print(bsl::ostream& stream,
                                              int           level,
                                              int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queueUri", this->queueUri());
    printer.printAttribute("partitionId", this->partitionId());
    {
        bool multilineFlag = (0 <= spacesPerLevel);
        bdlb::Print::indent(stream, level + 1, spacesPerLevel);
        stream << (multilineFlag ? "" : " ");
        stream << "queueKey = [ ";
        bdlb::Print::singleLineHexDump(stream,
                                       this->queueKey().begin(),
                                       this->queueKey().end());
        stream << " ]" << (multilineFlag ? "\n" : "");
    }
    printer.end();
    return stream;
}

// --------------------------
// class RegistrationResponse
// --------------------------

// CONSTANTS

const char RegistrationResponse::CLASS_NAME[] = "RegistrationResponse";

// CLASS METHODS

const bdlat_AttributeInfo*
RegistrationResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* RegistrationResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void RegistrationResponse::reset()
{
}

// ACCESSORS

bsl::ostream& RegistrationResponse::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ---------------------
// class ReplicaDataType
// ---------------------

// CONSTANTS

const char ReplicaDataType::CLASS_NAME[] = "ReplicaDataType";

const bdlat_EnumeratorInfo ReplicaDataType::ENUMERATOR_INFO_ARRAY[] = {
    {ReplicaDataType::E_UNKNOWN, "E_UNKNOWN", sizeof("E_UNKNOWN") - 1, ""},
    {ReplicaDataType::E_PULL, "E_PULL", sizeof("E_PULL") - 1, ""},
    {ReplicaDataType::E_PUSH, "E_PUSH", sizeof("E_PUSH") - 1, ""},
    {ReplicaDataType::E_DROP, "E_DROP", sizeof("E_DROP") - 1, ""}};

// CLASS METHODS

int ReplicaDataType::fromInt(ReplicaDataType::Value* result, int number)
{
    switch (number) {
    case ReplicaDataType::E_UNKNOWN:
    case ReplicaDataType::E_PULL:
    case ReplicaDataType::E_PUSH:
    case ReplicaDataType::E_DROP:
        *result = static_cast<ReplicaDataType::Value>(number);
        return 0;
    default: return -1;
    }
}

int ReplicaDataType::fromString(ReplicaDataType::Value* result,
                                const char*             string,
                                int                     stringLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            ReplicaDataType::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<ReplicaDataType::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* ReplicaDataType::toString(ReplicaDataType::Value value)
{
    switch (value) {
    case E_UNKNOWN: {
        return "E_UNKNOWN";
    }
    case E_PULL: {
        return "E_PULL";
    }
    case E_PUSH: {
        return "E_PUSH";
    }
    case E_DROP: {
        return "E_DROP";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ------------------------------
// class ReverseConnectionRequest
// ------------------------------

// CONSTANTS

const char ReverseConnectionRequest::CLASS_NAME[] = "ReverseConnectionRequest";

const char ReverseConnectionRequest::DEFAULT_INITIALIZER_CLUSTER_NAME[] = "";

const int ReverseConnectionRequest::DEFAULT_INITIALIZER_CLUSTER_NODE_ID = -1;

const bdlat_AttributeInfo ReverseConnectionRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PROTOCOL_VERSION,
     "protocolVersion",
     sizeof("protocolVersion") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CLUSTER_NAME,
     "clusterName",
     sizeof("clusterName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CLUSTER_NODE_ID,
     "clusterNodeId",
     sizeof("clusterNodeId") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
ReverseConnectionRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ReverseConnectionRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
ReverseConnectionRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PROTOCOL_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROTOCOL_VERSION];
    case ATTRIBUTE_ID_CLUSTER_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME];
    case ATTRIBUTE_ID_CLUSTER_NODE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NODE_ID];
    default: return 0;
    }
}

// CREATORS

ReverseConnectionRequest::ReverseConnectionRequest(
    bslma::Allocator* basicAllocator)
: d_clusterName(DEFAULT_INITIALIZER_CLUSTER_NAME, basicAllocator)
, d_protocolVersion()
, d_clusterNodeId(DEFAULT_INITIALIZER_CLUSTER_NODE_ID)
{
}

ReverseConnectionRequest::ReverseConnectionRequest(
    const ReverseConnectionRequest& original,
    bslma::Allocator*               basicAllocator)
: d_clusterName(original.d_clusterName, basicAllocator)
, d_protocolVersion(original.d_protocolVersion)
, d_clusterNodeId(original.d_clusterNodeId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReverseConnectionRequest::ReverseConnectionRequest(
    ReverseConnectionRequest&& original) noexcept
: d_clusterName(bsl::move(original.d_clusterName)),
  d_protocolVersion(bsl::move(original.d_protocolVersion)),
  d_clusterNodeId(bsl::move(original.d_clusterNodeId))
{
}

ReverseConnectionRequest::ReverseConnectionRequest(
    ReverseConnectionRequest&& original,
    bslma::Allocator*          basicAllocator)
: d_clusterName(bsl::move(original.d_clusterName), basicAllocator)
, d_protocolVersion(bsl::move(original.d_protocolVersion))
, d_clusterNodeId(bsl::move(original.d_clusterNodeId))
{
}
#endif

ReverseConnectionRequest::~ReverseConnectionRequest()
{
}

// MANIPULATORS

ReverseConnectionRequest&
ReverseConnectionRequest::operator=(const ReverseConnectionRequest& rhs)
{
    if (this != &rhs) {
        d_protocolVersion = rhs.d_protocolVersion;
        d_clusterName     = rhs.d_clusterName;
        d_clusterNodeId   = rhs.d_clusterNodeId;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReverseConnectionRequest&
ReverseConnectionRequest::operator=(ReverseConnectionRequest&& rhs)
{
    if (this != &rhs) {
        d_protocolVersion = bsl::move(rhs.d_protocolVersion);
        d_clusterName     = bsl::move(rhs.d_clusterName);
        d_clusterNodeId   = bsl::move(rhs.d_clusterNodeId);
    }

    return *this;
}
#endif

void ReverseConnectionRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_protocolVersion);
    d_clusterName   = DEFAULT_INITIALIZER_CLUSTER_NAME;
    d_clusterNodeId = DEFAULT_INITIALIZER_CLUSTER_NODE_ID;
}

// ACCESSORS

bsl::ostream& ReverseConnectionRequest::print(bsl::ostream& stream,
                                              int           level,
                                              int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("protocolVersion", this->protocolVersion());
    printer.printAttribute("clusterName", this->clusterName());
    printer.printAttribute("clusterNodeId", this->clusterNodeId());
    printer.end();
    return stream;
}

// --------------------------
// class RoutingConfiguration
// --------------------------

// CONSTANTS

const char RoutingConfiguration::CLASS_NAME[] = "RoutingConfiguration";

const bdlat_AttributeInfo RoutingConfiguration::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_FLAGS,
     "flags",
     sizeof("flags") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
RoutingConfiguration::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RoutingConfiguration::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RoutingConfiguration::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_FLAGS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS];
    default: return 0;
    }
}

// CREATORS

RoutingConfiguration::RoutingConfiguration()
: d_flags()
{
}

// MANIPULATORS

void RoutingConfiguration::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_flags);
}

// ACCESSORS

bsl::ostream& RoutingConfiguration::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("flags", this->flags());
    printer.end();
    return stream;
}

// -------------------------------
// class RoutingConfigurationFlags
// -------------------------------

// CONSTANTS

const char RoutingConfigurationFlags::CLASS_NAME[] =
    "RoutingConfigurationFlags";

const bdlat_EnumeratorInfo RoutingConfigurationFlags::ENUMERATOR_INFO_ARRAY[] =
    {{RoutingConfigurationFlags::E_AT_MOST_ONCE,
      "E_AT_MOST_ONCE",
      sizeof("E_AT_MOST_ONCE") - 1,
      ""},
     {RoutingConfigurationFlags::E_DELIVER_CONSUMER_PRIORITY,
      "E_DELIVER_CONSUMER_PRIORITY",
      sizeof("E_DELIVER_CONSUMER_PRIORITY") - 1,
      ""},
     {RoutingConfigurationFlags::E_DELIVER_ALL,
      "E_DELIVER_ALL",
      sizeof("E_DELIVER_ALL") - 1,
      ""},
     {RoutingConfigurationFlags::E_HAS_MULTIPLE_SUB_STREAMS,
      "E_HAS_MULTIPLE_SUB_STREAMS",
      sizeof("E_HAS_MULTIPLE_SUB_STREAMS") - 1,
      ""}};

// CLASS METHODS

int RoutingConfigurationFlags::fromInt(
    RoutingConfigurationFlags::Value* result,
    int                               number)
{
    switch (number) {
    case RoutingConfigurationFlags::E_AT_MOST_ONCE:
    case RoutingConfigurationFlags::E_DELIVER_CONSUMER_PRIORITY:
    case RoutingConfigurationFlags::E_DELIVER_ALL:
    case RoutingConfigurationFlags::E_HAS_MULTIPLE_SUB_STREAMS:
        *result = static_cast<RoutingConfigurationFlags::Value>(number);
        return 0;
    default: return -1;
    }
}

int RoutingConfigurationFlags::fromString(
    RoutingConfigurationFlags::Value* result,
    const char*                       string,
    int                               stringLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            RoutingConfigurationFlags::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<RoutingConfigurationFlags::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char*
RoutingConfigurationFlags::toString(RoutingConfigurationFlags::Value value)
{
    switch (value) {
    case E_AT_MOST_ONCE: {
        return "E_AT_MOST_ONCE";
    }
    case E_DELIVER_CONSUMER_PRIORITY: {
        return "E_DELIVER_CONSUMER_PRIORITY";
    }
    case E_DELIVER_ALL: {
        return "E_DELIVER_ALL";
    }
    case E_HAS_MULTIPLE_SUB_STREAMS: {
        return "E_HAS_MULTIPLE_SUB_STREAMS";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ---------------------
// class ScoutingRequest
// ---------------------

// CONSTANTS

const char ScoutingRequest::CLASS_NAME[] = "ScoutingRequest";

// CLASS METHODS

const bdlat_AttributeInfo*
ScoutingRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* ScoutingRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void ScoutingRequest::reset()
{
}

// ACCESSORS

bsl::ostream& ScoutingRequest::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ----------------------
// class ScoutingResponse
// ----------------------

// CONSTANTS

const char ScoutingResponse::CLASS_NAME[] = "ScoutingResponse";

const bdlat_AttributeInfo ScoutingResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_WILL_VOTE,
     "willVote",
     sizeof("willVote") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ScoutingResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ScoutingResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ScoutingResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_WILL_VOTE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_WILL_VOTE];
    default: return 0;
    }
}

// CREATORS

ScoutingResponse::ScoutingResponse()
: d_willVote()
{
}

// MANIPULATORS

void ScoutingResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_willVote);
}

// ACCESSORS

bsl::ostream& ScoutingResponse::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("willVote", this->willVote());
    printer.end();
    return stream;
}

// --------------------
// class StatusCategory
// --------------------

// CONSTANTS

const char StatusCategory::CLASS_NAME[] = "StatusCategory";

const bdlat_EnumeratorInfo StatusCategory::ENUMERATOR_INFO_ARRAY[] = {
    {StatusCategory::E_SUCCESS, "E_SUCCESS", sizeof("E_SUCCESS") - 1, ""},
    {StatusCategory::E_UNKNOWN, "E_UNKNOWN", sizeof("E_UNKNOWN") - 1, ""},
    {StatusCategory::E_TIMEOUT, "E_TIMEOUT", sizeof("E_TIMEOUT") - 1, ""},
    {StatusCategory::E_NOT_CONNECTED,
     "E_NOT_CONNECTED",
     sizeof("E_NOT_CONNECTED") - 1,
     ""},
    {StatusCategory::E_CANCELED, "E_CANCELED", sizeof("E_CANCELED") - 1, ""},
    {StatusCategory::E_NOT_SUPPORTED,
     "E_NOT_SUPPORTED",
     sizeof("E_NOT_SUPPORTED") - 1,
     ""},
    {StatusCategory::E_REFUSED, "E_REFUSED", sizeof("E_REFUSED") - 1, ""},
    {StatusCategory::E_INVALID_ARGUMENT,
     "E_INVALID_ARGUMENT",
     sizeof("E_INVALID_ARGUMENT") - 1,
     ""},
    {StatusCategory::E_NOT_READY,
     "E_NOT_READY",
     sizeof("E_NOT_READY") - 1,
     ""}};

// CLASS METHODS

int StatusCategory::fromInt(StatusCategory::Value* result, int number)
{
    switch (number) {
    case StatusCategory::E_SUCCESS:
    case StatusCategory::E_UNKNOWN:
    case StatusCategory::E_TIMEOUT:
    case StatusCategory::E_NOT_CONNECTED:
    case StatusCategory::E_CANCELED:
    case StatusCategory::E_NOT_SUPPORTED:
    case StatusCategory::E_REFUSED:
    case StatusCategory::E_INVALID_ARGUMENT:
    case StatusCategory::E_NOT_READY:
        *result = static_cast<StatusCategory::Value>(number);
        return 0;
    default: return -1;
    }
}

int StatusCategory::fromString(StatusCategory::Value* result,
                               const char*            string,
                               int                    stringLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            StatusCategory::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<StatusCategory::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* StatusCategory::toString(StatusCategory::Value value)
{
    switch (value) {
    case E_SUCCESS: {
        return "E_SUCCESS";
    }
    case E_UNKNOWN: {
        return "E_UNKNOWN";
    }
    case E_TIMEOUT: {
        return "E_TIMEOUT";
    }
    case E_NOT_CONNECTED: {
        return "E_NOT_CONNECTED";
    }
    case E_CANCELED: {
        return "E_CANCELED";
    }
    case E_NOT_SUPPORTED: {
        return "E_NOT_SUPPORTED";
    }
    case E_REFUSED: {
        return "E_REFUSED";
    }
    case E_INVALID_ARGUMENT: {
        return "E_INVALID_ARGUMENT";
    }
    case E_NOT_READY: {
        return "E_NOT_READY";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -----------------
// class StopRequest
// -----------------

// CONSTANTS

const char StopRequest::CLASS_NAME[] = "StopRequest";

const bdlat_AttributeInfo StopRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLUSTER_NAME,
     "clusterName",
     sizeof("clusterName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* StopRequest::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StopRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StopRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLUSTER_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME];
    default: return 0;
    }
}

// CREATORS

StopRequest::StopRequest(bslma::Allocator* basicAllocator)
: d_clusterName(basicAllocator)
{
}

StopRequest::StopRequest(const StopRequest& original,
                         bslma::Allocator*  basicAllocator)
: d_clusterName(original.d_clusterName, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StopRequest::StopRequest(StopRequest&& original) noexcept
: d_clusterName(bsl::move(original.d_clusterName))
{
}

StopRequest::StopRequest(StopRequest&&     original,
                         bslma::Allocator* basicAllocator)
: d_clusterName(bsl::move(original.d_clusterName), basicAllocator)
{
}
#endif

StopRequest::~StopRequest()
{
}

// MANIPULATORS

StopRequest& StopRequest::operator=(const StopRequest& rhs)
{
    if (this != &rhs) {
        d_clusterName = rhs.d_clusterName;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StopRequest& StopRequest::operator=(StopRequest&& rhs)
{
    if (this != &rhs) {
        d_clusterName = bsl::move(rhs.d_clusterName);
    }

    return *this;
}
#endif

void StopRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clusterName);
}

// ACCESSORS

bsl::ostream&
StopRequest::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clusterName", this->clusterName());
    printer.end();
    return stream;
}

// ------------------
// class StopResponse
// ------------------

// CONSTANTS

const char StopResponse::CLASS_NAME[] = "StopResponse";

const bdlat_AttributeInfo StopResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLUSTER_NAME,
     "clusterName",
     sizeof("clusterName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* StopResponse::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StopResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StopResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLUSTER_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME];
    default: return 0;
    }
}

// CREATORS

StopResponse::StopResponse(bslma::Allocator* basicAllocator)
: d_clusterName(basicAllocator)
{
}

StopResponse::StopResponse(const StopResponse& original,
                           bslma::Allocator*   basicAllocator)
: d_clusterName(original.d_clusterName, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StopResponse::StopResponse(StopResponse&& original) noexcept
: d_clusterName(bsl::move(original.d_clusterName))
{
}

StopResponse::StopResponse(StopResponse&&    original,
                           bslma::Allocator* basicAllocator)
: d_clusterName(bsl::move(original.d_clusterName), basicAllocator)
{
}
#endif

StopResponse::~StopResponse()
{
}

// MANIPULATORS

StopResponse& StopResponse::operator=(const StopResponse& rhs)
{
    if (this != &rhs) {
        d_clusterName = rhs.d_clusterName;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StopResponse& StopResponse::operator=(StopResponse&& rhs)
{
    if (this != &rhs) {
        d_clusterName = bsl::move(rhs.d_clusterName);
    }

    return *this;
}
#endif

void StopResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clusterName);
}

// ACCESSORS

bsl::ostream&
StopResponse::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clusterName", this->clusterName());
    printer.end();
    return stream;
}

// -----------------------------
// class StorageSyncResponseType
// -----------------------------

// CONSTANTS

const char StorageSyncResponseType::CLASS_NAME[] = "StorageSyncResponseType";

const bdlat_EnumeratorInfo StorageSyncResponseType::ENUMERATOR_INFO_ARRAY[] = {
    {StorageSyncResponseType::E_UNDEFINED,
     "E_UNDEFINED",
     sizeof("E_UNDEFINED") - 1,
     ""},
    {StorageSyncResponseType::E_PATCH, "E_PATCH", sizeof("E_PATCH") - 1, ""},
    {StorageSyncResponseType::E_FILE, "E_FILE", sizeof("E_FILE") - 1, ""},
    {StorageSyncResponseType::E_IN_SYNC,
     "E_IN_SYNC",
     sizeof("E_IN_SYNC") - 1,
     ""},
    {StorageSyncResponseType::E_EMPTY, "E_EMPTY", sizeof("E_EMPTY") - 1, ""}};

// CLASS METHODS

int StorageSyncResponseType::fromInt(StorageSyncResponseType::Value* result,
                                     int                             number)
{
    switch (number) {
    case StorageSyncResponseType::E_UNDEFINED:
    case StorageSyncResponseType::E_PATCH:
    case StorageSyncResponseType::E_FILE:
    case StorageSyncResponseType::E_IN_SYNC:
    case StorageSyncResponseType::E_EMPTY:
        *result = static_cast<StorageSyncResponseType::Value>(number);
        return 0;
    default: return -1;
    }
}

int StorageSyncResponseType::fromString(StorageSyncResponseType::Value* result,
                                        const char*                     string,
                                        int stringLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            StorageSyncResponseType::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<StorageSyncResponseType::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char*
StorageSyncResponseType::toString(StorageSyncResponseType::Value value)
{
    switch (value) {
    case E_UNDEFINED: {
        return "E_UNDEFINED";
    }
    case E_PATCH: {
        return "E_PATCH";
    }
    case E_FILE: {
        return "E_FILE";
    }
    case E_IN_SYNC: {
        return "E_IN_SYNC";
    }
    case E_EMPTY: {
        return "E_EMPTY";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// --------------------
// class SubQueueIdInfo
// --------------------

// CONSTANTS

const char SubQueueIdInfo::CLASS_NAME[] = "SubQueueIdInfo";

const unsigned int SubQueueIdInfo::DEFAULT_INITIALIZER_SUB_ID = 0;

const char SubQueueIdInfo::DEFAULT_INITIALIZER_APP_ID[] = "__default";

const bdlat_AttributeInfo SubQueueIdInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SUB_ID,
     "subId",
     sizeof("subId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
SubQueueIdInfo::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            SubQueueIdInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* SubQueueIdInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SUB_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUB_ID];
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    default: return 0;
    }
}

// CREATORS

SubQueueIdInfo::SubQueueIdInfo(bslma::Allocator* basicAllocator)
: d_appId(DEFAULT_INITIALIZER_APP_ID, basicAllocator)
, d_subId(DEFAULT_INITIALIZER_SUB_ID)
{
}

SubQueueIdInfo::SubQueueIdInfo(const SubQueueIdInfo& original,
                               bslma::Allocator*     basicAllocator)
: d_appId(original.d_appId, basicAllocator)
, d_subId(original.d_subId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SubQueueIdInfo::SubQueueIdInfo(SubQueueIdInfo&& original) noexcept
: d_appId(bsl::move(original.d_appId)),
  d_subId(bsl::move(original.d_subId))
{
}

SubQueueIdInfo::SubQueueIdInfo(SubQueueIdInfo&&  original,
                               bslma::Allocator* basicAllocator)
: d_appId(bsl::move(original.d_appId), basicAllocator)
, d_subId(bsl::move(original.d_subId))
{
}
#endif

SubQueueIdInfo::~SubQueueIdInfo()
{
}

// MANIPULATORS

SubQueueIdInfo& SubQueueIdInfo::operator=(const SubQueueIdInfo& rhs)
{
    if (this != &rhs) {
        d_subId = rhs.d_subId;
        d_appId = rhs.d_appId;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SubQueueIdInfo& SubQueueIdInfo::operator=(SubQueueIdInfo&& rhs)
{
    if (this != &rhs) {
        d_subId = bsl::move(rhs.d_subId);
        d_appId = bsl::move(rhs.d_appId);
    }

    return *this;
}
#endif

void SubQueueIdInfo::reset()
{
    d_subId = DEFAULT_INITIALIZER_SUB_ID;
    d_appId = DEFAULT_INITIALIZER_APP_ID;
}

// ACCESSORS

bsl::ostream& SubQueueIdInfo::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("subId", this->subId());
    printer.printAttribute("appId", this->appId());
    printer.end();
    return stream;
}

// ---------------
// class SyncPoint
// ---------------

// CONSTANTS

const char SyncPoint::CLASS_NAME[] = "SyncPoint";

const bdlat_AttributeInfo SyncPoint::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PRIMARY_LEASE_ID,
     "primaryLeaseId",
     sizeof("primaryLeaseId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SEQUENCE_NUM,
     "sequenceNum",
     sizeof("sequenceNum") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_DATA_FILE_OFFSET_DWORDS,
     "dataFileOffsetDwords",
     sizeof("dataFileOffsetDwords") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_QLIST_FILE_OFFSET_WORDS,
     "qlistFileOffsetWords",
     sizeof("qlistFileOffsetWords") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* SyncPoint::lookupAttributeInfo(const char* name,
                                                          int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            SyncPoint::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* SyncPoint::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_SEQUENCE_NUM:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUM];
    case ATTRIBUTE_ID_DATA_FILE_OFFSET_DWORDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_FILE_OFFSET_DWORDS];
    case ATTRIBUTE_ID_QLIST_FILE_OFFSET_WORDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QLIST_FILE_OFFSET_WORDS];
    default: return 0;
    }
}

// CREATORS

SyncPoint::SyncPoint()
: d_sequenceNum()
, d_primaryLeaseId()
, d_dataFileOffsetDwords()
, d_qlistFileOffsetWords()
{
}

// MANIPULATORS

void SyncPoint::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_primaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNum);
    bdlat_ValueTypeFunctions::reset(&d_dataFileOffsetDwords);
    bdlat_ValueTypeFunctions::reset(&d_qlistFileOffsetWords);
}

// ACCESSORS

bsl::ostream&
SyncPoint::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("primaryLeaseId", this->primaryLeaseId());
    printer.printAttribute("sequenceNum", this->sequenceNum());
    printer.printAttribute("dataFileOffsetDwords",
                           this->dataFileOffsetDwords());
    printer.printAttribute("qlistFileOffsetWords",
                           this->qlistFileOffsetWords());
    printer.end();
    return stream;
}

// --------------------
// class ClientIdentity
// --------------------

// CONSTANTS

const char ClientIdentity::CLASS_NAME[] = "ClientIdentity";

const int ClientIdentity::DEFAULT_INITIALIZER_SDK_VERSION = 999999;

const char ClientIdentity::DEFAULT_INITIALIZER_PROCESS_NAME[] = "";

const int ClientIdentity::DEFAULT_INITIALIZER_PID = 0;

const int ClientIdentity::DEFAULT_INITIALIZER_SESSION_ID = 1;

const char ClientIdentity::DEFAULT_INITIALIZER_HOST_NAME[] = "";

const char ClientIdentity::DEFAULT_INITIALIZER_FEATURES[] = "";

const char ClientIdentity::DEFAULT_INITIALIZER_CLUSTER_NAME[] = "";

const int ClientIdentity::DEFAULT_INITIALIZER_CLUSTER_NODE_ID = -1;

const ClientLanguage::Value ClientIdentity::DEFAULT_INITIALIZER_SDK_LANGUAGE =
    ClientLanguage::E_CPP;

const bdlat_AttributeInfo ClientIdentity::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PROTOCOL_VERSION,
     "protocolVersion",
     sizeof("protocolVersion") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SDK_VERSION,
     "sdkVersion",
     sizeof("sdkVersion") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CLIENT_TYPE,
     "clientType",
     sizeof("clientType") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PROCESS_NAME,
     "processName",
     sizeof("processName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PID,
     "pid",
     sizeof("pid") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SESSION_ID,
     "sessionId",
     sizeof("sessionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_HOST_NAME,
     "hostName",
     sizeof("hostName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_FEATURES,
     "features",
     sizeof("features") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CLUSTER_NAME,
     "clusterName",
     sizeof("clusterName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CLUSTER_NODE_ID,
     "clusterNodeId",
     sizeof("clusterNodeId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SDK_LANGUAGE,
     "sdkLanguage",
     sizeof("sdkLanguage") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_GUID_INFO,
     "guidInfo",
     sizeof("guidInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClientIdentity::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 12; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClientIdentity::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClientIdentity::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PROTOCOL_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROTOCOL_VERSION];
    case ATTRIBUTE_ID_SDK_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SDK_VERSION];
    case ATTRIBUTE_ID_CLIENT_TYPE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT_TYPE];
    case ATTRIBUTE_ID_PROCESS_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROCESS_NAME];
    case ATTRIBUTE_ID_PID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PID];
    case ATTRIBUTE_ID_SESSION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SESSION_ID];
    case ATTRIBUTE_ID_HOST_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME];
    case ATTRIBUTE_ID_FEATURES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FEATURES];
    case ATTRIBUTE_ID_CLUSTER_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME];
    case ATTRIBUTE_ID_CLUSTER_NODE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NODE_ID];
    case ATTRIBUTE_ID_SDK_LANGUAGE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SDK_LANGUAGE];
    case ATTRIBUTE_ID_GUID_INFO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID_INFO];
    default: return 0;
    }
}

// CREATORS

ClientIdentity::ClientIdentity(bslma::Allocator* basicAllocator)
: d_processName(DEFAULT_INITIALIZER_PROCESS_NAME, basicAllocator)
, d_hostName(DEFAULT_INITIALIZER_HOST_NAME, basicAllocator)
, d_features(DEFAULT_INITIALIZER_FEATURES, basicAllocator)
, d_clusterName(DEFAULT_INITIALIZER_CLUSTER_NAME, basicAllocator)
, d_guidInfo(basicAllocator)
, d_protocolVersion()
, d_sdkVersion(DEFAULT_INITIALIZER_SDK_VERSION)
, d_pid(DEFAULT_INITIALIZER_PID)
, d_sessionId(DEFAULT_INITIALIZER_SESSION_ID)
, d_clusterNodeId(DEFAULT_INITIALIZER_CLUSTER_NODE_ID)
, d_clientType(static_cast<ClientType::Value>(0))
, d_sdkLanguage(DEFAULT_INITIALIZER_SDK_LANGUAGE)
{
}

ClientIdentity::ClientIdentity(const ClientIdentity& original,
                               bslma::Allocator*     basicAllocator)
: d_processName(original.d_processName, basicAllocator)
, d_hostName(original.d_hostName, basicAllocator)
, d_features(original.d_features, basicAllocator)
, d_clusterName(original.d_clusterName, basicAllocator)
, d_guidInfo(original.d_guidInfo, basicAllocator)
, d_protocolVersion(original.d_protocolVersion)
, d_sdkVersion(original.d_sdkVersion)
, d_pid(original.d_pid)
, d_sessionId(original.d_sessionId)
, d_clusterNodeId(original.d_clusterNodeId)
, d_clientType(original.d_clientType)
, d_sdkLanguage(original.d_sdkLanguage)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClientIdentity::ClientIdentity(ClientIdentity&& original) noexcept
: d_processName(bsl::move(original.d_processName)),
  d_hostName(bsl::move(original.d_hostName)),
  d_features(bsl::move(original.d_features)),
  d_clusterName(bsl::move(original.d_clusterName)),
  d_guidInfo(bsl::move(original.d_guidInfo)),
  d_protocolVersion(bsl::move(original.d_protocolVersion)),
  d_sdkVersion(bsl::move(original.d_sdkVersion)),
  d_pid(bsl::move(original.d_pid)),
  d_sessionId(bsl::move(original.d_sessionId)),
  d_clusterNodeId(bsl::move(original.d_clusterNodeId)),
  d_clientType(bsl::move(original.d_clientType)),
  d_sdkLanguage(bsl::move(original.d_sdkLanguage))
{
}

ClientIdentity::ClientIdentity(ClientIdentity&&  original,
                               bslma::Allocator* basicAllocator)
: d_processName(bsl::move(original.d_processName), basicAllocator)
, d_hostName(bsl::move(original.d_hostName), basicAllocator)
, d_features(bsl::move(original.d_features), basicAllocator)
, d_clusterName(bsl::move(original.d_clusterName), basicAllocator)
, d_guidInfo(bsl::move(original.d_guidInfo), basicAllocator)
, d_protocolVersion(bsl::move(original.d_protocolVersion))
, d_sdkVersion(bsl::move(original.d_sdkVersion))
, d_pid(bsl::move(original.d_pid))
, d_sessionId(bsl::move(original.d_sessionId))
, d_clusterNodeId(bsl::move(original.d_clusterNodeId))
, d_clientType(bsl::move(original.d_clientType))
, d_sdkLanguage(bsl::move(original.d_sdkLanguage))
{
}
#endif

ClientIdentity::~ClientIdentity()
{
}

// MANIPULATORS

ClientIdentity& ClientIdentity::operator=(const ClientIdentity& rhs)
{
    if (this != &rhs) {
        d_protocolVersion = rhs.d_protocolVersion;
        d_sdkVersion      = rhs.d_sdkVersion;
        d_clientType      = rhs.d_clientType;
        d_processName     = rhs.d_processName;
        d_pid             = rhs.d_pid;
        d_sessionId       = rhs.d_sessionId;
        d_hostName        = rhs.d_hostName;
        d_features        = rhs.d_features;
        d_clusterName     = rhs.d_clusterName;
        d_clusterNodeId   = rhs.d_clusterNodeId;
        d_sdkLanguage     = rhs.d_sdkLanguage;
        d_guidInfo        = rhs.d_guidInfo;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClientIdentity& ClientIdentity::operator=(ClientIdentity&& rhs)
{
    if (this != &rhs) {
        d_protocolVersion = bsl::move(rhs.d_protocolVersion);
        d_sdkVersion      = bsl::move(rhs.d_sdkVersion);
        d_clientType      = bsl::move(rhs.d_clientType);
        d_processName     = bsl::move(rhs.d_processName);
        d_pid             = bsl::move(rhs.d_pid);
        d_sessionId       = bsl::move(rhs.d_sessionId);
        d_hostName        = bsl::move(rhs.d_hostName);
        d_features        = bsl::move(rhs.d_features);
        d_clusterName     = bsl::move(rhs.d_clusterName);
        d_clusterNodeId   = bsl::move(rhs.d_clusterNodeId);
        d_sdkLanguage     = bsl::move(rhs.d_sdkLanguage);
        d_guidInfo        = bsl::move(rhs.d_guidInfo);
    }

    return *this;
}
#endif

void ClientIdentity::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_protocolVersion);
    d_sdkVersion = DEFAULT_INITIALIZER_SDK_VERSION;
    bdlat_ValueTypeFunctions::reset(&d_clientType);
    d_processName   = DEFAULT_INITIALIZER_PROCESS_NAME;
    d_pid           = DEFAULT_INITIALIZER_PID;
    d_sessionId     = DEFAULT_INITIALIZER_SESSION_ID;
    d_hostName      = DEFAULT_INITIALIZER_HOST_NAME;
    d_features      = DEFAULT_INITIALIZER_FEATURES;
    d_clusterName   = DEFAULT_INITIALIZER_CLUSTER_NAME;
    d_clusterNodeId = DEFAULT_INITIALIZER_CLUSTER_NODE_ID;
    d_sdkLanguage   = DEFAULT_INITIALIZER_SDK_LANGUAGE;
    bdlat_ValueTypeFunctions::reset(&d_guidInfo);
}

// ACCESSORS

bsl::ostream& ClientIdentity::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("protocolVersion", this->protocolVersion());
    printer.printAttribute("sdkVersion", this->sdkVersion());
    printer.printAttribute("clientType", this->clientType());
    printer.printAttribute("processName", this->processName());
    printer.printAttribute("pid", this->pid());
    printer.printAttribute("sessionId", this->sessionId());
    printer.printAttribute("hostName", this->hostName());
    printer.printAttribute("features", this->features());
    printer.printAttribute("clusterName", this->clusterName());
    printer.printAttribute("clusterNodeId", this->clusterNodeId());
    printer.printAttribute("sdkLanguage", this->sdkLanguage());
    printer.printAttribute("guidInfo", this->guidInfo());
    printer.end();
    return stream;
}

// ------------------
// class DumpMessages
// ------------------

// CONSTANTS

const char DumpMessages::CLASS_NAME[] = "DumpMessages";

const int DumpMessages::DEFAULT_INITIALIZER_DUMP_ACTION_VALUE = 0;

const bdlat_AttributeInfo DumpMessages::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_MSG_TYPE_TO_DUMP,
     "msgTypeToDump",
     sizeof("msgTypeToDump") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_DUMP_ACTION_TYPE,
     "dumpActionType",
     sizeof("dumpActionType") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_DUMP_ACTION_VALUE,
     "dumpActionValue",
     sizeof("dumpActionValue") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* DumpMessages::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DumpMessages::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DumpMessages::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MSG_TYPE_TO_DUMP:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_TYPE_TO_DUMP];
    case ATTRIBUTE_ID_DUMP_ACTION_TYPE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_ACTION_TYPE];
    case ATTRIBUTE_ID_DUMP_ACTION_VALUE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_ACTION_VALUE];
    default: return 0;
    }
}

// CREATORS

DumpMessages::DumpMessages()
: d_dumpActionValue(DEFAULT_INITIALIZER_DUMP_ACTION_VALUE)
, d_msgTypeToDump(static_cast<DumpMsgType::Value>(0))
, d_dumpActionType(static_cast<DumpActionType::Value>(0))
{
}

// MANIPULATORS

void DumpMessages::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_msgTypeToDump);
    bdlat_ValueTypeFunctions::reset(&d_dumpActionType);
    d_dumpActionValue = DEFAULT_INITIALIZER_DUMP_ACTION_VALUE;
}

// ACCESSORS

bsl::ostream&
DumpMessages::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("msgTypeToDump", this->msgTypeToDump());
    printer.printAttribute("dumpActionType", this->dumpActionType());
    printer.printAttribute("dumpActionValue", this->dumpActionValue());
    printer.end();
    return stream;
}

// --------------------------
// class ElectorMessageChoice
// --------------------------

// CONSTANTS

const char ElectorMessageChoice::CLASS_NAME[] = "ElectorMessageChoice";

const bdlat_SelectionInfo ElectorMessageChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ELECTION_PROPOSAL,
     "electionProposal",
     sizeof("electionProposal") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ELECTION_RESPONSE,
     "electionResponse",
     sizeof("electionResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LEADER_HEARTBEAT,
     "leaderHeartbeat",
     sizeof("leaderHeartbeat") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ELECTOR_NODE_STATUS,
     "electorNodeStatus",
     sizeof("electorNodeStatus") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_HEARTBEAT_RESPONSE,
     "heartbeatResponse",
     sizeof("heartbeatResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SCOUTING_REQUEST,
     "scoutingRequest",
     sizeof("scoutingRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SCOUTING_RESPONSE,
     "scoutingResponse",
     sizeof("scoutingResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION,
     "leadershipCessionNotification",
     sizeof("leadershipCessionNotification") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ElectorMessageChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 8; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ElectorMessageChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ElectorMessageChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ELECTION_PROPOSAL:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTION_PROPOSAL];
    case SELECTION_ID_ELECTION_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTION_RESPONSE];
    case SELECTION_ID_LEADER_HEARTBEAT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_HEARTBEAT];
    case SELECTION_ID_ELECTOR_NODE_STATUS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTOR_NODE_STATUS];
    case SELECTION_ID_HEARTBEAT_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_HEARTBEAT_RESPONSE];
    case SELECTION_ID_SCOUTING_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SCOUTING_REQUEST];
    case SELECTION_ID_SCOUTING_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SCOUTING_RESPONSE];
    case SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_LEADERSHIP_CESSION_NOTIFICATION];
    default: return 0;
    }
}

// CREATORS

ElectorMessageChoice::ElectorMessageChoice(
    const ElectorMessageChoice& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_ELECTION_PROPOSAL: {
        new (d_electionProposal.buffer())
            ElectionProposal(original.d_electionProposal.object());
    } break;
    case SELECTION_ID_ELECTION_RESPONSE: {
        new (d_electionResponse.buffer())
            ElectionResponse(original.d_electionResponse.object());
    } break;
    case SELECTION_ID_LEADER_HEARTBEAT: {
        new (d_leaderHeartbeat.buffer())
            LeaderHeartbeat(original.d_leaderHeartbeat.object());
    } break;
    case SELECTION_ID_ELECTOR_NODE_STATUS: {
        new (d_electorNodeStatus.buffer())
            ElectorNodeStatus(original.d_electorNodeStatus.object());
    } break;
    case SELECTION_ID_HEARTBEAT_RESPONSE: {
        new (d_heartbeatResponse.buffer())
            HeartbeatResponse(original.d_heartbeatResponse.object());
    } break;
    case SELECTION_ID_SCOUTING_REQUEST: {
        new (d_scoutingRequest.buffer())
            ScoutingRequest(original.d_scoutingRequest.object());
    } break;
    case SELECTION_ID_SCOUTING_RESPONSE: {
        new (d_scoutingResponse.buffer())
            ScoutingResponse(original.d_scoutingResponse.object());
    } break;
    case SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION: {
        new (d_leadershipCessionNotification.buffer())
            LeadershipCessionNotification(
                original.d_leadershipCessionNotification.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectorMessageChoice::ElectorMessageChoice(ElectorMessageChoice&& original)
    noexcept : d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_ELECTION_PROPOSAL: {
        new (d_electionProposal.buffer())
            ElectionProposal(bsl::move(original.d_electionProposal.object()));
    } break;
    case SELECTION_ID_ELECTION_RESPONSE: {
        new (d_electionResponse.buffer())
            ElectionResponse(bsl::move(original.d_electionResponse.object()));
    } break;
    case SELECTION_ID_LEADER_HEARTBEAT: {
        new (d_leaderHeartbeat.buffer())
            LeaderHeartbeat(bsl::move(original.d_leaderHeartbeat.object()));
    } break;
    case SELECTION_ID_ELECTOR_NODE_STATUS: {
        new (d_electorNodeStatus.buffer()) ElectorNodeStatus(
            bsl::move(original.d_electorNodeStatus.object()));
    } break;
    case SELECTION_ID_HEARTBEAT_RESPONSE: {
        new (d_heartbeatResponse.buffer()) HeartbeatResponse(
            bsl::move(original.d_heartbeatResponse.object()));
    } break;
    case SELECTION_ID_SCOUTING_REQUEST: {
        new (d_scoutingRequest.buffer())
            ScoutingRequest(bsl::move(original.d_scoutingRequest.object()));
    } break;
    case SELECTION_ID_SCOUTING_RESPONSE: {
        new (d_scoutingResponse.buffer())
            ScoutingResponse(bsl::move(original.d_scoutingResponse.object()));
    } break;
    case SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION: {
        new (d_leadershipCessionNotification.buffer())
            LeadershipCessionNotification(
                bsl::move(original.d_leadershipCessionNotification.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ElectorMessageChoice&
ElectorMessageChoice::operator=(const ElectorMessageChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ELECTION_PROPOSAL: {
            makeElectionProposal(rhs.d_electionProposal.object());
        } break;
        case SELECTION_ID_ELECTION_RESPONSE: {
            makeElectionResponse(rhs.d_electionResponse.object());
        } break;
        case SELECTION_ID_LEADER_HEARTBEAT: {
            makeLeaderHeartbeat(rhs.d_leaderHeartbeat.object());
        } break;
        case SELECTION_ID_ELECTOR_NODE_STATUS: {
            makeElectorNodeStatus(rhs.d_electorNodeStatus.object());
        } break;
        case SELECTION_ID_HEARTBEAT_RESPONSE: {
            makeHeartbeatResponse(rhs.d_heartbeatResponse.object());
        } break;
        case SELECTION_ID_SCOUTING_REQUEST: {
            makeScoutingRequest(rhs.d_scoutingRequest.object());
        } break;
        case SELECTION_ID_SCOUTING_RESPONSE: {
            makeScoutingResponse(rhs.d_scoutingResponse.object());
        } break;
        case SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION: {
            makeLeadershipCessionNotification(
                rhs.d_leadershipCessionNotification.object());
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
ElectorMessageChoice&
ElectorMessageChoice::operator=(ElectorMessageChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ELECTION_PROPOSAL: {
            makeElectionProposal(bsl::move(rhs.d_electionProposal.object()));
        } break;
        case SELECTION_ID_ELECTION_RESPONSE: {
            makeElectionResponse(bsl::move(rhs.d_electionResponse.object()));
        } break;
        case SELECTION_ID_LEADER_HEARTBEAT: {
            makeLeaderHeartbeat(bsl::move(rhs.d_leaderHeartbeat.object()));
        } break;
        case SELECTION_ID_ELECTOR_NODE_STATUS: {
            makeElectorNodeStatus(bsl::move(rhs.d_electorNodeStatus.object()));
        } break;
        case SELECTION_ID_HEARTBEAT_RESPONSE: {
            makeHeartbeatResponse(bsl::move(rhs.d_heartbeatResponse.object()));
        } break;
        case SELECTION_ID_SCOUTING_REQUEST: {
            makeScoutingRequest(bsl::move(rhs.d_scoutingRequest.object()));
        } break;
        case SELECTION_ID_SCOUTING_RESPONSE: {
            makeScoutingResponse(bsl::move(rhs.d_scoutingResponse.object()));
        } break;
        case SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION: {
            makeLeadershipCessionNotification(
                bsl::move(rhs.d_leadershipCessionNotification.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ElectorMessageChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ELECTION_PROPOSAL: {
        d_electionProposal.object().~ElectionProposal();
    } break;
    case SELECTION_ID_ELECTION_RESPONSE: {
        d_electionResponse.object().~ElectionResponse();
    } break;
    case SELECTION_ID_LEADER_HEARTBEAT: {
        d_leaderHeartbeat.object().~LeaderHeartbeat();
    } break;
    case SELECTION_ID_ELECTOR_NODE_STATUS: {
        d_electorNodeStatus.object().~ElectorNodeStatus();
    } break;
    case SELECTION_ID_HEARTBEAT_RESPONSE: {
        d_heartbeatResponse.object().~HeartbeatResponse();
    } break;
    case SELECTION_ID_SCOUTING_REQUEST: {
        d_scoutingRequest.object().~ScoutingRequest();
    } break;
    case SELECTION_ID_SCOUTING_RESPONSE: {
        d_scoutingResponse.object().~ScoutingResponse();
    } break;
    case SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION: {
        d_leadershipCessionNotification.object()
            .~LeadershipCessionNotification();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ElectorMessageChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ELECTION_PROPOSAL: {
        makeElectionProposal();
    } break;
    case SELECTION_ID_ELECTION_RESPONSE: {
        makeElectionResponse();
    } break;
    case SELECTION_ID_LEADER_HEARTBEAT: {
        makeLeaderHeartbeat();
    } break;
    case SELECTION_ID_ELECTOR_NODE_STATUS: {
        makeElectorNodeStatus();
    } break;
    case SELECTION_ID_HEARTBEAT_RESPONSE: {
        makeHeartbeatResponse();
    } break;
    case SELECTION_ID_SCOUTING_REQUEST: {
        makeScoutingRequest();
    } break;
    case SELECTION_ID_SCOUTING_RESPONSE: {
        makeScoutingResponse();
    } break;
    case SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION: {
        makeLeadershipCessionNotification();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ElectorMessageChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

ElectionProposal& ElectorMessageChoice::makeElectionProposal()
{
    if (SELECTION_ID_ELECTION_PROPOSAL == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_electionProposal.object());
    }
    else {
        reset();
        new (d_electionProposal.buffer()) ElectionProposal();
        d_selectionId = SELECTION_ID_ELECTION_PROPOSAL;
    }

    return d_electionProposal.object();
}

ElectionProposal&
ElectorMessageChoice::makeElectionProposal(const ElectionProposal& value)
{
    if (SELECTION_ID_ELECTION_PROPOSAL == d_selectionId) {
        d_electionProposal.object() = value;
    }
    else {
        reset();
        new (d_electionProposal.buffer()) ElectionProposal(value);
        d_selectionId = SELECTION_ID_ELECTION_PROPOSAL;
    }

    return d_electionProposal.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectionProposal&
ElectorMessageChoice::makeElectionProposal(ElectionProposal&& value)
{
    if (SELECTION_ID_ELECTION_PROPOSAL == d_selectionId) {
        d_electionProposal.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_electionProposal.buffer()) ElectionProposal(bsl::move(value));
        d_selectionId = SELECTION_ID_ELECTION_PROPOSAL;
    }

    return d_electionProposal.object();
}
#endif

ElectionResponse& ElectorMessageChoice::makeElectionResponse()
{
    if (SELECTION_ID_ELECTION_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_electionResponse.object());
    }
    else {
        reset();
        new (d_electionResponse.buffer()) ElectionResponse();
        d_selectionId = SELECTION_ID_ELECTION_RESPONSE;
    }

    return d_electionResponse.object();
}

ElectionResponse&
ElectorMessageChoice::makeElectionResponse(const ElectionResponse& value)
{
    if (SELECTION_ID_ELECTION_RESPONSE == d_selectionId) {
        d_electionResponse.object() = value;
    }
    else {
        reset();
        new (d_electionResponse.buffer()) ElectionResponse(value);
        d_selectionId = SELECTION_ID_ELECTION_RESPONSE;
    }

    return d_electionResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectionResponse&
ElectorMessageChoice::makeElectionResponse(ElectionResponse&& value)
{
    if (SELECTION_ID_ELECTION_RESPONSE == d_selectionId) {
        d_electionResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_electionResponse.buffer()) ElectionResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_ELECTION_RESPONSE;
    }

    return d_electionResponse.object();
}
#endif

LeaderHeartbeat& ElectorMessageChoice::makeLeaderHeartbeat()
{
    if (SELECTION_ID_LEADER_HEARTBEAT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_leaderHeartbeat.object());
    }
    else {
        reset();
        new (d_leaderHeartbeat.buffer()) LeaderHeartbeat();
        d_selectionId = SELECTION_ID_LEADER_HEARTBEAT;
    }

    return d_leaderHeartbeat.object();
}

LeaderHeartbeat&
ElectorMessageChoice::makeLeaderHeartbeat(const LeaderHeartbeat& value)
{
    if (SELECTION_ID_LEADER_HEARTBEAT == d_selectionId) {
        d_leaderHeartbeat.object() = value;
    }
    else {
        reset();
        new (d_leaderHeartbeat.buffer()) LeaderHeartbeat(value);
        d_selectionId = SELECTION_ID_LEADER_HEARTBEAT;
    }

    return d_leaderHeartbeat.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderHeartbeat&
ElectorMessageChoice::makeLeaderHeartbeat(LeaderHeartbeat&& value)
{
    if (SELECTION_ID_LEADER_HEARTBEAT == d_selectionId) {
        d_leaderHeartbeat.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leaderHeartbeat.buffer()) LeaderHeartbeat(bsl::move(value));
        d_selectionId = SELECTION_ID_LEADER_HEARTBEAT;
    }

    return d_leaderHeartbeat.object();
}
#endif

ElectorNodeStatus& ElectorMessageChoice::makeElectorNodeStatus()
{
    if (SELECTION_ID_ELECTOR_NODE_STATUS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_electorNodeStatus.object());
    }
    else {
        reset();
        new (d_electorNodeStatus.buffer()) ElectorNodeStatus();
        d_selectionId = SELECTION_ID_ELECTOR_NODE_STATUS;
    }

    return d_electorNodeStatus.object();
}

ElectorNodeStatus&
ElectorMessageChoice::makeElectorNodeStatus(const ElectorNodeStatus& value)
{
    if (SELECTION_ID_ELECTOR_NODE_STATUS == d_selectionId) {
        d_electorNodeStatus.object() = value;
    }
    else {
        reset();
        new (d_electorNodeStatus.buffer()) ElectorNodeStatus(value);
        d_selectionId = SELECTION_ID_ELECTOR_NODE_STATUS;
    }

    return d_electorNodeStatus.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectorNodeStatus&
ElectorMessageChoice::makeElectorNodeStatus(ElectorNodeStatus&& value)
{
    if (SELECTION_ID_ELECTOR_NODE_STATUS == d_selectionId) {
        d_electorNodeStatus.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_electorNodeStatus.buffer()) ElectorNodeStatus(bsl::move(value));
        d_selectionId = SELECTION_ID_ELECTOR_NODE_STATUS;
    }

    return d_electorNodeStatus.object();
}
#endif

HeartbeatResponse& ElectorMessageChoice::makeHeartbeatResponse()
{
    if (SELECTION_ID_HEARTBEAT_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_heartbeatResponse.object());
    }
    else {
        reset();
        new (d_heartbeatResponse.buffer()) HeartbeatResponse();
        d_selectionId = SELECTION_ID_HEARTBEAT_RESPONSE;
    }

    return d_heartbeatResponse.object();
}

HeartbeatResponse&
ElectorMessageChoice::makeHeartbeatResponse(const HeartbeatResponse& value)
{
    if (SELECTION_ID_HEARTBEAT_RESPONSE == d_selectionId) {
        d_heartbeatResponse.object() = value;
    }
    else {
        reset();
        new (d_heartbeatResponse.buffer()) HeartbeatResponse(value);
        d_selectionId = SELECTION_ID_HEARTBEAT_RESPONSE;
    }

    return d_heartbeatResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
HeartbeatResponse&
ElectorMessageChoice::makeHeartbeatResponse(HeartbeatResponse&& value)
{
    if (SELECTION_ID_HEARTBEAT_RESPONSE == d_selectionId) {
        d_heartbeatResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_heartbeatResponse.buffer()) HeartbeatResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_HEARTBEAT_RESPONSE;
    }

    return d_heartbeatResponse.object();
}
#endif

ScoutingRequest& ElectorMessageChoice::makeScoutingRequest()
{
    if (SELECTION_ID_SCOUTING_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_scoutingRequest.object());
    }
    else {
        reset();
        new (d_scoutingRequest.buffer()) ScoutingRequest();
        d_selectionId = SELECTION_ID_SCOUTING_REQUEST;
    }

    return d_scoutingRequest.object();
}

ScoutingRequest&
ElectorMessageChoice::makeScoutingRequest(const ScoutingRequest& value)
{
    if (SELECTION_ID_SCOUTING_REQUEST == d_selectionId) {
        d_scoutingRequest.object() = value;
    }
    else {
        reset();
        new (d_scoutingRequest.buffer()) ScoutingRequest(value);
        d_selectionId = SELECTION_ID_SCOUTING_REQUEST;
    }

    return d_scoutingRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ScoutingRequest&
ElectorMessageChoice::makeScoutingRequest(ScoutingRequest&& value)
{
    if (SELECTION_ID_SCOUTING_REQUEST == d_selectionId) {
        d_scoutingRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_scoutingRequest.buffer()) ScoutingRequest(bsl::move(value));
        d_selectionId = SELECTION_ID_SCOUTING_REQUEST;
    }

    return d_scoutingRequest.object();
}
#endif

ScoutingResponse& ElectorMessageChoice::makeScoutingResponse()
{
    if (SELECTION_ID_SCOUTING_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_scoutingResponse.object());
    }
    else {
        reset();
        new (d_scoutingResponse.buffer()) ScoutingResponse();
        d_selectionId = SELECTION_ID_SCOUTING_RESPONSE;
    }

    return d_scoutingResponse.object();
}

ScoutingResponse&
ElectorMessageChoice::makeScoutingResponse(const ScoutingResponse& value)
{
    if (SELECTION_ID_SCOUTING_RESPONSE == d_selectionId) {
        d_scoutingResponse.object() = value;
    }
    else {
        reset();
        new (d_scoutingResponse.buffer()) ScoutingResponse(value);
        d_selectionId = SELECTION_ID_SCOUTING_RESPONSE;
    }

    return d_scoutingResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ScoutingResponse&
ElectorMessageChoice::makeScoutingResponse(ScoutingResponse&& value)
{
    if (SELECTION_ID_SCOUTING_RESPONSE == d_selectionId) {
        d_scoutingResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_scoutingResponse.buffer()) ScoutingResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_SCOUTING_RESPONSE;
    }

    return d_scoutingResponse.object();
}
#endif

LeadershipCessionNotification&
ElectorMessageChoice::makeLeadershipCessionNotification()
{
    if (SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_leadershipCessionNotification.object());
    }
    else {
        reset();
        new (d_leadershipCessionNotification.buffer())
            LeadershipCessionNotification();
        d_selectionId = SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION;
    }

    return d_leadershipCessionNotification.object();
}

LeadershipCessionNotification&
ElectorMessageChoice::makeLeadershipCessionNotification(
    const LeadershipCessionNotification& value)
{
    if (SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION == d_selectionId) {
        d_leadershipCessionNotification.object() = value;
    }
    else {
        reset();
        new (d_leadershipCessionNotification.buffer())
            LeadershipCessionNotification(value);
        d_selectionId = SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION;
    }

    return d_leadershipCessionNotification.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeadershipCessionNotification&
ElectorMessageChoice::makeLeadershipCessionNotification(
    LeadershipCessionNotification&& value)
{
    if (SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION == d_selectionId) {
        d_leadershipCessionNotification.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leadershipCessionNotification.buffer())
            LeadershipCessionNotification(bsl::move(value));
        d_selectionId = SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION;
    }

    return d_leadershipCessionNotification.object();
}
#endif

// ACCESSORS

bsl::ostream& ElectorMessageChoice::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ELECTION_PROPOSAL: {
        printer.printAttribute("electionProposal",
                               d_electionProposal.object());
    } break;
    case SELECTION_ID_ELECTION_RESPONSE: {
        printer.printAttribute("electionResponse",
                               d_electionResponse.object());
    } break;
    case SELECTION_ID_LEADER_HEARTBEAT: {
        printer.printAttribute("leaderHeartbeat", d_leaderHeartbeat.object());
    } break;
    case SELECTION_ID_ELECTOR_NODE_STATUS: {
        printer.printAttribute("electorNodeStatus",
                               d_electorNodeStatus.object());
    } break;
    case SELECTION_ID_HEARTBEAT_RESPONSE: {
        printer.printAttribute("heartbeatResponse",
                               d_heartbeatResponse.object());
    } break;
    case SELECTION_ID_SCOUTING_REQUEST: {
        printer.printAttribute("scoutingRequest", d_scoutingRequest.object());
    } break;
    case SELECTION_ID_SCOUTING_RESPONSE: {
        printer.printAttribute("scoutingResponse",
                               d_scoutingResponse.object());
    } break;
    case SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION: {
        printer.printAttribute("leadershipCessionNotification",
                               d_leadershipCessionNotification.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ElectorMessageChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ELECTION_PROPOSAL:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTION_PROPOSAL].name();
    case SELECTION_ID_ELECTION_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTION_RESPONSE].name();
    case SELECTION_ID_LEADER_HEARTBEAT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_HEARTBEAT].name();
    case SELECTION_ID_ELECTOR_NODE_STATUS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTOR_NODE_STATUS]
            .name();
    case SELECTION_ID_HEARTBEAT_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_HEARTBEAT_RESPONSE].name();
    case SELECTION_ID_SCOUTING_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SCOUTING_REQUEST].name();
    case SELECTION_ID_SCOUTING_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SCOUTING_RESPONSE].name();
    case SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_LEADERSHIP_CESSION_NOTIFICATION]
                .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------
// class Expression
// ----------------

// CONSTANTS

const char Expression::CLASS_NAME[] = "Expression";

const ExpressionVersion::Value Expression::DEFAULT_INITIALIZER_VERSION =
    ExpressionVersion::E_UNDEFINED;

const bdlat_AttributeInfo Expression::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_VERSION,
     "version",
     sizeof("version") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_TEXT,
     "text",
     sizeof("text") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Expression::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Expression::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Expression::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSION];
    case ATTRIBUTE_ID_TEXT: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TEXT];
    default: return 0;
    }
}

// CREATORS

Expression::Expression(bslma::Allocator* basicAllocator)
: d_text(basicAllocator)
, d_version(DEFAULT_INITIALIZER_VERSION)
{
}

Expression::Expression(const Expression& original,
                       bslma::Allocator* basicAllocator)
: d_text(original.d_text, basicAllocator)
, d_version(original.d_version)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Expression::Expression(Expression&& original) noexcept
: d_text(bsl::move(original.d_text)),
  d_version(bsl::move(original.d_version))
{
}

Expression::Expression(Expression&& original, bslma::Allocator* basicAllocator)
: d_text(bsl::move(original.d_text), basicAllocator)
, d_version(bsl::move(original.d_version))
{
}
#endif

Expression::~Expression()
{
}

// MANIPULATORS

Expression& Expression::operator=(const Expression& rhs)
{
    if (this != &rhs) {
        d_version = rhs.d_version;
        d_text    = rhs.d_text;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Expression& Expression::operator=(Expression&& rhs)
{
    if (this != &rhs) {
        d_version = bsl::move(rhs.d_version);
        d_text    = bsl::move(rhs.d_text);
    }

    return *this;
}
#endif

void Expression::reset()
{
    d_version = DEFAULT_INITIALIZER_VERSION;
    bdlat_ValueTypeFunctions::reset(&d_text);
}

// ACCESSORS

bsl::ostream&
Expression::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("version", this->version());
    printer.printAttribute("text", this->text());
    printer.end();
    return stream;
}

// -------------------------
// class FollowerLSNResponse
// -------------------------

// CONSTANTS

const char FollowerLSNResponse::CLASS_NAME[] = "FollowerLSNResponse";

const bdlat_AttributeInfo FollowerLSNResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
FollowerLSNResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            FollowerLSNResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* FollowerLSNResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

FollowerLSNResponse::FollowerLSNResponse()
: d_sequenceNumber()
{
}

// MANIPULATORS

void FollowerLSNResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
}

// ACCESSORS

bsl::ostream& FollowerLSNResponse::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.end();
    return stream;
}

// -----------------------
// class LeaderAdvisoryAck
// -----------------------

// CONSTANTS

const char LeaderAdvisoryAck::CLASS_NAME[] = "LeaderAdvisoryAck";

const bdlat_AttributeInfo LeaderAdvisoryAck::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SEQUENCE_NUMBER_ACKED,
     "sequenceNumberAcked",
     sizeof("sequenceNumberAcked") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
LeaderAdvisoryAck::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LeaderAdvisoryAck::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* LeaderAdvisoryAck::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SEQUENCE_NUMBER_ACKED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER_ACKED];
    default: return 0;
    }
}

// CREATORS

LeaderAdvisoryAck::LeaderAdvisoryAck()
: d_sequenceNumberAcked()
{
}

// MANIPULATORS

void LeaderAdvisoryAck::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumberAcked);
}

// ACCESSORS

bsl::ostream& LeaderAdvisoryAck::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNumberAcked", this->sequenceNumberAcked());
    printer.end();
    return stream;
}

// --------------------------
// class LeaderAdvisoryCommit
// --------------------------

// CONSTANTS

const char LeaderAdvisoryCommit::CLASS_NAME[] = "LeaderAdvisoryCommit";

const bdlat_AttributeInfo LeaderAdvisoryCommit::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_SEQUENCE_NUMBER_COMMITTED,
     "sequenceNumberCommitted",
     sizeof("sequenceNumberCommitted") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
LeaderAdvisoryCommit::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LeaderAdvisoryCommit::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* LeaderAdvisoryCommit::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    case ATTRIBUTE_ID_SEQUENCE_NUMBER_COMMITTED:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_SEQUENCE_NUMBER_COMMITTED];
    default: return 0;
    }
}

// CREATORS

LeaderAdvisoryCommit::LeaderAdvisoryCommit()
: d_sequenceNumber()
, d_sequenceNumberCommitted()
{
}

// MANIPULATORS

void LeaderAdvisoryCommit::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumberCommitted);
}

// ACCESSORS

bsl::ostream& LeaderAdvisoryCommit::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.printAttribute("sequenceNumberCommitted",
                           this->sequenceNumberCommitted());
    printer.end();
    return stream;
}

// ----------------------------------
// class LeaderSyncStateQueryResponse
// ----------------------------------

// CONSTANTS

const char LeaderSyncStateQueryResponse::CLASS_NAME[] =
    "LeaderSyncStateQueryResponse";

const bdlat_AttributeInfo
    LeaderSyncStateQueryResponse::ATTRIBUTE_INFO_ARRAY[] = {
        {ATTRIBUTE_ID_LEADER_MESSAGE_SEQUENCE,
         "leaderMessageSequence",
         sizeof("leaderMessageSequence") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
LeaderSyncStateQueryResponse::lookupAttributeInfo(const char* name,
                                                  int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LeaderSyncStateQueryResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
LeaderSyncStateQueryResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_LEADER_MESSAGE_SEQUENCE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_MESSAGE_SEQUENCE];
    default: return 0;
    }
}

// CREATORS

LeaderSyncStateQueryResponse::LeaderSyncStateQueryResponse()
: d_leaderMessageSequence()
{
}

// MANIPULATORS

void LeaderSyncStateQueryResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_leaderMessageSequence);
}

// ACCESSORS

bsl::ostream& LeaderSyncStateQueryResponse::print(bsl::ostream& stream,
                                                  int           level,
                                                  int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("leaderMessageSequence",
                           this->leaderMessageSequence());
    printer.end();
    return stream;
}

// ------------------------
// class NodeStatusAdvisory
// ------------------------

// CONSTANTS

const char NodeStatusAdvisory::CLASS_NAME[] = "NodeStatusAdvisory";

const bdlat_AttributeInfo NodeStatusAdvisory::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_STATUS,
     "status",
     sizeof("status") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
NodeStatusAdvisory::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            NodeStatusAdvisory::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* NodeStatusAdvisory::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_STATUS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATUS];
    default: return 0;
    }
}

// CREATORS

NodeStatusAdvisory::NodeStatusAdvisory()
: d_status(static_cast<NodeStatus::Value>(0))
{
}

// MANIPULATORS

void NodeStatusAdvisory::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_status);
}

// ACCESSORS

bsl::ostream& NodeStatusAdvisory::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("status", this->status());
    printer.end();
    return stream;
}

// ------------------------------
// class PartitionPrimaryAdvisory
// ------------------------------

// CONSTANTS

const char PartitionPrimaryAdvisory::CLASS_NAME[] = "PartitionPrimaryAdvisory";

const bdlat_AttributeInfo PartitionPrimaryAdvisory::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PARTITIONS,
     "partitions",
     sizeof("partitions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionPrimaryAdvisory::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionPrimaryAdvisory::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
PartitionPrimaryAdvisory::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    case ATTRIBUTE_ID_PARTITIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITIONS];
    default: return 0;
    }
}

// CREATORS

PartitionPrimaryAdvisory::PartitionPrimaryAdvisory(
    bslma::Allocator* basicAllocator)
: d_partitions(basicAllocator)
, d_sequenceNumber()
{
}

PartitionPrimaryAdvisory::PartitionPrimaryAdvisory(
    const PartitionPrimaryAdvisory& original,
    bslma::Allocator*               basicAllocator)
: d_partitions(original.d_partitions, basicAllocator)
, d_sequenceNumber(original.d_sequenceNumber)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionPrimaryAdvisory::PartitionPrimaryAdvisory(
    PartitionPrimaryAdvisory&& original) noexcept
: d_partitions(bsl::move(original.d_partitions)),
  d_sequenceNumber(bsl::move(original.d_sequenceNumber))
{
}

PartitionPrimaryAdvisory::PartitionPrimaryAdvisory(
    PartitionPrimaryAdvisory&& original,
    bslma::Allocator*          basicAllocator)
: d_partitions(bsl::move(original.d_partitions), basicAllocator)
, d_sequenceNumber(bsl::move(original.d_sequenceNumber))
{
}
#endif

PartitionPrimaryAdvisory::~PartitionPrimaryAdvisory()
{
}

// MANIPULATORS

PartitionPrimaryAdvisory&
PartitionPrimaryAdvisory::operator=(const PartitionPrimaryAdvisory& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = rhs.d_sequenceNumber;
        d_partitions     = rhs.d_partitions;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionPrimaryAdvisory&
PartitionPrimaryAdvisory::operator=(PartitionPrimaryAdvisory&& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = bsl::move(rhs.d_sequenceNumber);
        d_partitions     = bsl::move(rhs.d_partitions);
    }

    return *this;
}
#endif

void PartitionPrimaryAdvisory::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
    bdlat_ValueTypeFunctions::reset(&d_partitions);
}

// ACCESSORS

bsl::ostream& PartitionPrimaryAdvisory::print(bsl::ostream& stream,
                                              int           level,
                                              int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.printAttribute("partitions", this->partitions());
    printer.end();
    return stream;
}

// -------------------------
// class PrimaryStateRequest
// -------------------------

// CONSTANTS

const char PrimaryStateRequest::CLASS_NAME[] = "PrimaryStateRequest";

const bdlat_AttributeInfo PrimaryStateRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
PrimaryStateRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PrimaryStateRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PrimaryStateRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

PrimaryStateRequest::PrimaryStateRequest()
: d_sequenceNumber()
, d_partitionId()
{
}

// MANIPULATORS

void PrimaryStateRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
}

// ACCESSORS

bsl::ostream& PrimaryStateRequest::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.end();
    return stream;
}

// --------------------------
// class PrimaryStateResponse
// --------------------------

// CONSTANTS

const char PrimaryStateResponse::CLASS_NAME[] = "PrimaryStateResponse";

const bdlat_AttributeInfo PrimaryStateResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
PrimaryStateResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PrimaryStateResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PrimaryStateResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

PrimaryStateResponse::PrimaryStateResponse()
: d_sequenceNumber()
, d_partitionId()
{
}

// MANIPULATORS

void PrimaryStateResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
}

// ACCESSORS

bsl::ostream& PrimaryStateResponse::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.end();
    return stream;
}

// ---------------------------
// class PrimaryStatusAdvisory
// ---------------------------

// CONSTANTS

const char PrimaryStatusAdvisory::CLASS_NAME[] = "PrimaryStatusAdvisory";

const PrimaryStatus::Value PrimaryStatusAdvisory::DEFAULT_INITIALIZER_STATUS =
    PrimaryStatus::E_UNDEFINED;

const bdlat_AttributeInfo PrimaryStatusAdvisory::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIMARY_LEASE_ID,
     "primaryLeaseId",
     sizeof("primaryLeaseId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STATUS,
     "status",
     sizeof("status") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
PrimaryStatusAdvisory::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PrimaryStatusAdvisory::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PrimaryStatusAdvisory::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_STATUS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATUS];
    default: return 0;
    }
}

// CREATORS

PrimaryStatusAdvisory::PrimaryStatusAdvisory()
: d_primaryLeaseId()
, d_partitionId()
, d_status(DEFAULT_INITIALIZER_STATUS)
{
}

// MANIPULATORS

void PrimaryStatusAdvisory::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_primaryLeaseId);
    d_status = DEFAULT_INITIALIZER_STATUS;
}

// ACCESSORS

bsl::ostream& PrimaryStatusAdvisory::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("primaryLeaseId", this->primaryLeaseId());
    printer.printAttribute("status", this->status());
    printer.end();
    return stream;
}

// ---------------------------
// class QueueHandleParameters
// ---------------------------

// CONSTANTS

const char QueueHandleParameters::CLASS_NAME[] = "QueueHandleParameters";

const int QueueHandleParameters::DEFAULT_INITIALIZER_READ_COUNT = 0;

const int QueueHandleParameters::DEFAULT_INITIALIZER_WRITE_COUNT = 0;

const int QueueHandleParameters::DEFAULT_INITIALIZER_ADMIN_COUNT = 0;

const bdlat_AttributeInfo QueueHandleParameters::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_Q_ID,
     "qId",
     sizeof("qId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SUB_ID_INFO,
     "subIdInfo",
     sizeof("subIdInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_FLAGS,
     "flags",
     sizeof("flags") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_READ_COUNT,
     "readCount",
     sizeof("readCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_WRITE_COUNT,
     "writeCount",
     sizeof("writeCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ADMIN_COUNT,
     "adminCount",
     sizeof("adminCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueHandleParameters::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 7; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueHandleParameters::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueHandleParameters::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_Q_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_Q_ID];
    case ATTRIBUTE_ID_SUB_ID_INFO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUB_ID_INFO];
    case ATTRIBUTE_ID_FLAGS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS];
    case ATTRIBUTE_ID_READ_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_READ_COUNT];
    case ATTRIBUTE_ID_WRITE_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_WRITE_COUNT];
    case ATTRIBUTE_ID_ADMIN_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADMIN_COUNT];
    default: return 0;
    }
}

// CREATORS

QueueHandleParameters::QueueHandleParameters(bslma::Allocator* basicAllocator)
: d_flags()
, d_uri(basicAllocator)
, d_subIdInfo(basicAllocator)
, d_qId()
, d_readCount(DEFAULT_INITIALIZER_READ_COUNT)
, d_writeCount(DEFAULT_INITIALIZER_WRITE_COUNT)
, d_adminCount(DEFAULT_INITIALIZER_ADMIN_COUNT)
{
}

QueueHandleParameters::QueueHandleParameters(
    const QueueHandleParameters& original,
    bslma::Allocator*            basicAllocator)
: d_flags(original.d_flags)
, d_uri(original.d_uri, basicAllocator)
, d_subIdInfo(original.d_subIdInfo, basicAllocator)
, d_qId(original.d_qId)
, d_readCount(original.d_readCount)
, d_writeCount(original.d_writeCount)
, d_adminCount(original.d_adminCount)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueHandleParameters::QueueHandleParameters(QueueHandleParameters&& original)
    noexcept : d_flags(bsl::move(original.d_flags)),
               d_uri(bsl::move(original.d_uri)),
               d_subIdInfo(bsl::move(original.d_subIdInfo)),
               d_qId(bsl::move(original.d_qId)),
               d_readCount(bsl::move(original.d_readCount)),
               d_writeCount(bsl::move(original.d_writeCount)),
               d_adminCount(bsl::move(original.d_adminCount))
{
}

QueueHandleParameters::QueueHandleParameters(QueueHandleParameters&& original,
                                             bslma::Allocator* basicAllocator)
: d_flags(bsl::move(original.d_flags))
, d_uri(bsl::move(original.d_uri), basicAllocator)
, d_subIdInfo(bsl::move(original.d_subIdInfo), basicAllocator)
, d_qId(bsl::move(original.d_qId))
, d_readCount(bsl::move(original.d_readCount))
, d_writeCount(bsl::move(original.d_writeCount))
, d_adminCount(bsl::move(original.d_adminCount))
{
}
#endif

QueueHandleParameters::~QueueHandleParameters()
{
}

// MANIPULATORS

QueueHandleParameters&
QueueHandleParameters::operator=(const QueueHandleParameters& rhs)
{
    if (this != &rhs) {
        d_uri        = rhs.d_uri;
        d_qId        = rhs.d_qId;
        d_subIdInfo  = rhs.d_subIdInfo;
        d_flags      = rhs.d_flags;
        d_readCount  = rhs.d_readCount;
        d_writeCount = rhs.d_writeCount;
        d_adminCount = rhs.d_adminCount;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueHandleParameters&
QueueHandleParameters::operator=(QueueHandleParameters&& rhs)
{
    if (this != &rhs) {
        d_uri        = bsl::move(rhs.d_uri);
        d_qId        = bsl::move(rhs.d_qId);
        d_subIdInfo  = bsl::move(rhs.d_subIdInfo);
        d_flags      = bsl::move(rhs.d_flags);
        d_readCount  = bsl::move(rhs.d_readCount);
        d_writeCount = bsl::move(rhs.d_writeCount);
        d_adminCount = bsl::move(rhs.d_adminCount);
    }

    return *this;
}
#endif

void QueueHandleParameters::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_qId);
    bdlat_ValueTypeFunctions::reset(&d_subIdInfo);
    bdlat_ValueTypeFunctions::reset(&d_flags);
    d_readCount  = DEFAULT_INITIALIZER_READ_COUNT;
    d_writeCount = DEFAULT_INITIALIZER_WRITE_COUNT;
    d_adminCount = DEFAULT_INITIALIZER_ADMIN_COUNT;
}

// ACCESSORS

bsl::ostream& QueueHandleParameters::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("qId", this->qId());
    printer.printAttribute("subIdInfo", this->subIdInfo());
    printer.printAttribute("flags", this->flags());
    printer.printAttribute("readCount", this->readCount());
    printer.printAttribute("writeCount", this->writeCount());
    printer.printAttribute("adminCount", this->adminCount());
    printer.end();
    return stream;
}

// ---------------
// class QueueInfo
// ---------------

// CONSTANTS

const char QueueInfo::CLASS_NAME[] = "QueueInfo";

const bdlat_AttributeInfo QueueInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_KEY,
     "key",
     sizeof("key") - 1,
     "",
     bdlat_FormattingMode::e_HEX},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_APP_IDS,
     "appIds",
     sizeof("appIds") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* QueueInfo::lookupAttributeInfo(const char* name,
                                                          int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_KEY: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_APP_IDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_IDS];
    default: return 0;
    }
}

// CREATORS

QueueInfo::QueueInfo(bslma::Allocator* basicAllocator)
: d_key(basicAllocator)
, d_appIds(basicAllocator)
, d_uri(basicAllocator)
, d_partitionId()
{
}

QueueInfo::QueueInfo(const QueueInfo&  original,
                     bslma::Allocator* basicAllocator)
: d_key(original.d_key, basicAllocator)
, d_appIds(original.d_appIds, basicAllocator)
, d_uri(original.d_uri, basicAllocator)
, d_partitionId(original.d_partitionId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueInfo::QueueInfo(QueueInfo&& original) noexcept
: d_key(bsl::move(original.d_key)),
  d_appIds(bsl::move(original.d_appIds)),
  d_uri(bsl::move(original.d_uri)),
  d_partitionId(bsl::move(original.d_partitionId))
{
}

QueueInfo::QueueInfo(QueueInfo&& original, bslma::Allocator* basicAllocator)
: d_key(bsl::move(original.d_key), basicAllocator)
, d_appIds(bsl::move(original.d_appIds), basicAllocator)
, d_uri(bsl::move(original.d_uri), basicAllocator)
, d_partitionId(bsl::move(original.d_partitionId))
{
}
#endif

QueueInfo::~QueueInfo()
{
}

// MANIPULATORS

QueueInfo& QueueInfo::operator=(const QueueInfo& rhs)
{
    if (this != &rhs) {
        d_uri         = rhs.d_uri;
        d_key         = rhs.d_key;
        d_partitionId = rhs.d_partitionId;
        d_appIds      = rhs.d_appIds;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueInfo& QueueInfo::operator=(QueueInfo&& rhs)
{
    if (this != &rhs) {
        d_uri         = bsl::move(rhs.d_uri);
        d_key         = bsl::move(rhs.d_key);
        d_partitionId = bsl::move(rhs.d_partitionId);
        d_appIds      = bsl::move(rhs.d_appIds);
    }

    return *this;
}
#endif

void QueueInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_key);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_appIds);
}

// ACCESSORS

bsl::ostream&
QueueInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    {
        bool multilineFlag = (0 <= spacesPerLevel);
        bdlb::Print::indent(stream, level + 1, spacesPerLevel);
        stream << (multilineFlag ? "" : " ");
        stream << "key = [ ";
        bdlb::Print::singleLineHexDump(stream,
                                       this->key().begin(),
                                       this->key().end());
        stream << " ]" << (multilineFlag ? "\n" : "");
    }
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("appIds", this->appIds());
    printer.end();
    return stream;
}

// ---------------------
// class QueueInfoUpdate
// ---------------------

// CONSTANTS

const char QueueInfoUpdate::CLASS_NAME[] = "QueueInfoUpdate";

const bdlat_AttributeInfo QueueInfoUpdate::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_KEY,
     "key",
     sizeof("key") - 1,
     "",
     bdlat_FormattingMode::e_HEX},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ADDED_APP_IDS,
     "addedAppIds",
     sizeof("addedAppIds") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_REMOVED_APP_IDS,
     "removedAppIds",
     sizeof("removedAppIds") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_DOMAIN,
     "domain",
     sizeof("domain") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueInfoUpdate::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueInfoUpdate::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueInfoUpdate::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_KEY: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_ADDED_APP_IDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADDED_APP_IDS];
    case ATTRIBUTE_ID_REMOVED_APP_IDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REMOVED_APP_IDS];
    case ATTRIBUTE_ID_DOMAIN:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN];
    default: return 0;
    }
}

// CREATORS

QueueInfoUpdate::QueueInfoUpdate(bslma::Allocator* basicAllocator)
: d_key(basicAllocator)
, d_addedAppIds(basicAllocator)
, d_removedAppIds(basicAllocator)
, d_uri(basicAllocator)
, d_domain(basicAllocator)
, d_partitionId()
{
}

QueueInfoUpdate::QueueInfoUpdate(const QueueInfoUpdate& original,
                                 bslma::Allocator*      basicAllocator)
: d_key(original.d_key, basicAllocator)
, d_addedAppIds(original.d_addedAppIds, basicAllocator)
, d_removedAppIds(original.d_removedAppIds, basicAllocator)
, d_uri(original.d_uri, basicAllocator)
, d_domain(original.d_domain, basicAllocator)
, d_partitionId(original.d_partitionId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueInfoUpdate::QueueInfoUpdate(QueueInfoUpdate&& original) noexcept
: d_key(bsl::move(original.d_key)),
  d_addedAppIds(bsl::move(original.d_addedAppIds)),
  d_removedAppIds(bsl::move(original.d_removedAppIds)),
  d_uri(bsl::move(original.d_uri)),
  d_domain(bsl::move(original.d_domain)),
  d_partitionId(bsl::move(original.d_partitionId))
{
}

QueueInfoUpdate::QueueInfoUpdate(QueueInfoUpdate&& original,
                                 bslma::Allocator* basicAllocator)
: d_key(bsl::move(original.d_key), basicAllocator)
, d_addedAppIds(bsl::move(original.d_addedAppIds), basicAllocator)
, d_removedAppIds(bsl::move(original.d_removedAppIds), basicAllocator)
, d_uri(bsl::move(original.d_uri), basicAllocator)
, d_domain(bsl::move(original.d_domain), basicAllocator)
, d_partitionId(bsl::move(original.d_partitionId))
{
}
#endif

QueueInfoUpdate::~QueueInfoUpdate()
{
}

// MANIPULATORS

QueueInfoUpdate& QueueInfoUpdate::operator=(const QueueInfoUpdate& rhs)
{
    if (this != &rhs) {
        d_uri           = rhs.d_uri;
        d_key           = rhs.d_key;
        d_partitionId   = rhs.d_partitionId;
        d_addedAppIds   = rhs.d_addedAppIds;
        d_removedAppIds = rhs.d_removedAppIds;
        d_domain        = rhs.d_domain;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueInfoUpdate& QueueInfoUpdate::operator=(QueueInfoUpdate&& rhs)
{
    if (this != &rhs) {
        d_uri           = bsl::move(rhs.d_uri);
        d_key           = bsl::move(rhs.d_key);
        d_partitionId   = bsl::move(rhs.d_partitionId);
        d_addedAppIds   = bsl::move(rhs.d_addedAppIds);
        d_removedAppIds = bsl::move(rhs.d_removedAppIds);
        d_domain        = bsl::move(rhs.d_domain);
    }

    return *this;
}
#endif

void QueueInfoUpdate::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_key);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_addedAppIds);
    bdlat_ValueTypeFunctions::reset(&d_removedAppIds);
    bdlat_ValueTypeFunctions::reset(&d_domain);
}

// ACCESSORS

bsl::ostream& QueueInfoUpdate::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    {
        bool multilineFlag = (0 <= spacesPerLevel);
        bdlb::Print::indent(stream, level + 1, spacesPerLevel);
        stream << (multilineFlag ? "" : " ");
        stream << "key = [ ";
        bdlb::Print::singleLineHexDump(stream,
                                       this->key().begin(),
                                       this->key().end());
        stream << " ]" << (multilineFlag ? "\n" : "");
    }
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("addedAppIds", this->addedAppIds());
    printer.printAttribute("removedAppIds", this->removedAppIds());
    printer.printAttribute("domain", this->domain());
    printer.end();
    return stream;
}

// ---------------------------
// class QueueStreamParameters
// ---------------------------

// CONSTANTS

const char QueueStreamParameters::CLASS_NAME[] = "QueueStreamParameters";

const bsls::Types::Int64
    QueueStreamParameters::DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES = 0;

const bsls::Types::Int64
    QueueStreamParameters::DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES = 0;

const int QueueStreamParameters::DEFAULT_INITIALIZER_CONSUMER_PRIORITY =
    -2147483648;

const int QueueStreamParameters::DEFAULT_INITIALIZER_CONSUMER_PRIORITY_COUNT =
    0;

const bdlat_AttributeInfo QueueStreamParameters::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SUB_ID_INFO,
     "subIdInfo",
     sizeof("subIdInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
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
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CONSUMER_PRIORITY_COUNT,
     "consumerPriorityCount",
     sizeof("consumerPriorityCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueStreamParameters::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueStreamParameters::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueStreamParameters::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SUB_ID_INFO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUB_ID_INFO];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES];
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES];
    case ATTRIBUTE_ID_CONSUMER_PRIORITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY];
    case ATTRIBUTE_ID_CONSUMER_PRIORITY_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY_COUNT];
    default: return 0;
    }
}

// CREATORS

QueueStreamParameters::QueueStreamParameters(bslma::Allocator* basicAllocator)
: d_maxUnconfirmedMessages(DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES)
, d_maxUnconfirmedBytes(DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES)
, d_subIdInfo(basicAllocator)
, d_consumerPriority(DEFAULT_INITIALIZER_CONSUMER_PRIORITY)
, d_consumerPriorityCount(DEFAULT_INITIALIZER_CONSUMER_PRIORITY_COUNT)
{
}

QueueStreamParameters::QueueStreamParameters(
    const QueueStreamParameters& original,
    bslma::Allocator*            basicAllocator)
: d_maxUnconfirmedMessages(original.d_maxUnconfirmedMessages)
, d_maxUnconfirmedBytes(original.d_maxUnconfirmedBytes)
, d_subIdInfo(original.d_subIdInfo, basicAllocator)
, d_consumerPriority(original.d_consumerPriority)
, d_consumerPriorityCount(original.d_consumerPriorityCount)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueStreamParameters::QueueStreamParameters(
    QueueStreamParameters&& original) noexcept
: d_maxUnconfirmedMessages(bsl::move(original.d_maxUnconfirmedMessages)),
  d_maxUnconfirmedBytes(bsl::move(original.d_maxUnconfirmedBytes)),
  d_subIdInfo(bsl::move(original.d_subIdInfo)),
  d_consumerPriority(bsl::move(original.d_consumerPriority)),
  d_consumerPriorityCount(bsl::move(original.d_consumerPriorityCount))
{
}

QueueStreamParameters::QueueStreamParameters(QueueStreamParameters&& original,
                                             bslma::Allocator* basicAllocator)
: d_maxUnconfirmedMessages(bsl::move(original.d_maxUnconfirmedMessages))
, d_maxUnconfirmedBytes(bsl::move(original.d_maxUnconfirmedBytes))
, d_subIdInfo(bsl::move(original.d_subIdInfo), basicAllocator)
, d_consumerPriority(bsl::move(original.d_consumerPriority))
, d_consumerPriorityCount(bsl::move(original.d_consumerPriorityCount))
{
}
#endif

QueueStreamParameters::~QueueStreamParameters()
{
}

// MANIPULATORS

QueueStreamParameters&
QueueStreamParameters::operator=(const QueueStreamParameters& rhs)
{
    if (this != &rhs) {
        d_subIdInfo              = rhs.d_subIdInfo;
        d_maxUnconfirmedMessages = rhs.d_maxUnconfirmedMessages;
        d_maxUnconfirmedBytes    = rhs.d_maxUnconfirmedBytes;
        d_consumerPriority       = rhs.d_consumerPriority;
        d_consumerPriorityCount  = rhs.d_consumerPriorityCount;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueStreamParameters&
QueueStreamParameters::operator=(QueueStreamParameters&& rhs)
{
    if (this != &rhs) {
        d_subIdInfo              = bsl::move(rhs.d_subIdInfo);
        d_maxUnconfirmedMessages = bsl::move(rhs.d_maxUnconfirmedMessages);
        d_maxUnconfirmedBytes    = bsl::move(rhs.d_maxUnconfirmedBytes);
        d_consumerPriority       = bsl::move(rhs.d_consumerPriority);
        d_consumerPriorityCount  = bsl::move(rhs.d_consumerPriorityCount);
    }

    return *this;
}
#endif

void QueueStreamParameters::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_subIdInfo);
    d_maxUnconfirmedMessages = DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES;
    d_maxUnconfirmedBytes    = DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES;
    d_consumerPriority       = DEFAULT_INITIALIZER_CONSUMER_PRIORITY;
    d_consumerPriorityCount  = DEFAULT_INITIALIZER_CONSUMER_PRIORITY_COUNT;
}

// ACCESSORS

bsl::ostream& QueueStreamParameters::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("subIdInfo", this->subIdInfo());
    printer.printAttribute("maxUnconfirmedMessages",
                           this->maxUnconfirmedMessages());
    printer.printAttribute("maxUnconfirmedBytes", this->maxUnconfirmedBytes());
    printer.printAttribute("consumerPriority", this->consumerPriority());
    printer.printAttribute("consumerPriorityCount",
                           this->consumerPriorityCount());
    printer.end();
    return stream;
}

// -------------------------
// class RegistrationRequest
// -------------------------

// CONSTANTS

const char RegistrationRequest::CLASS_NAME[] = "RegistrationRequest";

const bdlat_AttributeInfo RegistrationRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
RegistrationRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RegistrationRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RegistrationRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

RegistrationRequest::RegistrationRequest()
: d_sequenceNumber()
{
}

// MANIPULATORS

void RegistrationRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
}

// ACCESSORS

bsl::ostream& RegistrationRequest::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.end();
    return stream;
}

// ------------------------
// class ReplicaDataRequest
// ------------------------

// CONSTANTS

const char ReplicaDataRequest::CLASS_NAME[] = "ReplicaDataRequest";

const bdlat_AttributeInfo ReplicaDataRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_REPLICA_DATA_TYPE,
     "replicaDataType",
     sizeof("replicaDataType") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BEGIN_SEQUENCE_NUMBER,
     "beginSequenceNumber",
     sizeof("beginSequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_END_SEQUENCE_NUMBER,
     "endSequenceNumber",
     sizeof("endSequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ReplicaDataRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ReplicaDataRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ReplicaDataRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_REPLICA_DATA_TYPE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REPLICA_DATA_TYPE];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_BEGIN_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BEGIN_SEQUENCE_NUMBER];
    case ATTRIBUTE_ID_END_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_END_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

ReplicaDataRequest::ReplicaDataRequest()
: d_beginSequenceNumber()
, d_endSequenceNumber()
, d_partitionId()
, d_replicaDataType(static_cast<ReplicaDataType::Value>(0))
{
}

// MANIPULATORS

void ReplicaDataRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_replicaDataType);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_beginSequenceNumber);
    bdlat_ValueTypeFunctions::reset(&d_endSequenceNumber);
}

// ACCESSORS

bsl::ostream& ReplicaDataRequest::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("replicaDataType", this->replicaDataType());
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("beginSequenceNumber", this->beginSequenceNumber());
    printer.printAttribute("endSequenceNumber", this->endSequenceNumber());
    printer.end();
    return stream;
}

// -------------------------
// class ReplicaDataResponse
// -------------------------

// CONSTANTS

const char ReplicaDataResponse::CLASS_NAME[] = "ReplicaDataResponse";

const bdlat_AttributeInfo ReplicaDataResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_REPLICA_DATA_TYPE,
     "replicaDataType",
     sizeof("replicaDataType") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BEGIN_SEQUENCE_NUMBER,
     "beginSequenceNumber",
     sizeof("beginSequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_END_SEQUENCE_NUMBER,
     "endSequenceNumber",
     sizeof("endSequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ReplicaDataResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ReplicaDataResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ReplicaDataResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_REPLICA_DATA_TYPE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REPLICA_DATA_TYPE];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_BEGIN_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BEGIN_SEQUENCE_NUMBER];
    case ATTRIBUTE_ID_END_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_END_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

ReplicaDataResponse::ReplicaDataResponse()
: d_beginSequenceNumber()
, d_endSequenceNumber()
, d_partitionId()
, d_replicaDataType(static_cast<ReplicaDataType::Value>(0))
{
}

// MANIPULATORS

void ReplicaDataResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_replicaDataType);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_beginSequenceNumber);
    bdlat_ValueTypeFunctions::reset(&d_endSequenceNumber);
}

// ACCESSORS

bsl::ostream& ReplicaDataResponse::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("replicaDataType", this->replicaDataType());
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("beginSequenceNumber", this->beginSequenceNumber());
    printer.printAttribute("endSequenceNumber", this->endSequenceNumber());
    printer.end();
    return stream;
}

// -------------------------
// class ReplicaStateRequest
// -------------------------

// CONSTANTS

const char ReplicaStateRequest::CLASS_NAME[] = "ReplicaStateRequest";

const bdlat_AttributeInfo ReplicaStateRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ReplicaStateRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ReplicaStateRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ReplicaStateRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

ReplicaStateRequest::ReplicaStateRequest()
: d_sequenceNumber()
, d_partitionId()
{
}

// MANIPULATORS

void ReplicaStateRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
}

// ACCESSORS

bsl::ostream& ReplicaStateRequest::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.end();
    return stream;
}

// --------------------------
// class ReplicaStateResponse
// --------------------------

// CONSTANTS

const char ReplicaStateResponse::CLASS_NAME[] = "ReplicaStateResponse";

const bdlat_AttributeInfo ReplicaStateResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ReplicaStateResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ReplicaStateResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ReplicaStateResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    default: return 0;
    }
}

// CREATORS

ReplicaStateResponse::ReplicaStateResponse()
: d_sequenceNumber()
, d_partitionId()
{
}

// MANIPULATORS

void ReplicaStateResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
}

// ACCESSORS

bsl::ostream& ReplicaStateResponse::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.end();
    return stream;
}

// -----------------------------
// class StateNotificationChoice
// -----------------------------

// CONSTANTS

const char StateNotificationChoice::CLASS_NAME[] = "StateNotificationChoice";

const bdlat_SelectionInfo StateNotificationChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_LEADER_PASSIVE,
     "leaderPassive",
     sizeof("leaderPassive") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
StateNotificationChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            StateNotificationChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* StateNotificationChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_LEADER_PASSIVE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_PASSIVE];
    default: return 0;
    }
}

// CREATORS

StateNotificationChoice::StateNotificationChoice(
    const StateNotificationChoice& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_LEADER_PASSIVE: {
        new (d_leaderPassive.buffer())
            LeaderPassive(original.d_leaderPassive.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StateNotificationChoice::StateNotificationChoice(
    StateNotificationChoice&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_LEADER_PASSIVE: {
        new (d_leaderPassive.buffer())
            LeaderPassive(bsl::move(original.d_leaderPassive.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

StateNotificationChoice&
StateNotificationChoice::operator=(const StateNotificationChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_LEADER_PASSIVE: {
            makeLeaderPassive(rhs.d_leaderPassive.object());
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
StateNotificationChoice&
StateNotificationChoice::operator=(StateNotificationChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_LEADER_PASSIVE: {
            makeLeaderPassive(bsl::move(rhs.d_leaderPassive.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void StateNotificationChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_LEADER_PASSIVE: {
        d_leaderPassive.object().~LeaderPassive();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int StateNotificationChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_LEADER_PASSIVE: {
        makeLeaderPassive();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int StateNotificationChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

LeaderPassive& StateNotificationChoice::makeLeaderPassive()
{
    if (SELECTION_ID_LEADER_PASSIVE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_leaderPassive.object());
    }
    else {
        reset();
        new (d_leaderPassive.buffer()) LeaderPassive();
        d_selectionId = SELECTION_ID_LEADER_PASSIVE;
    }

    return d_leaderPassive.object();
}

LeaderPassive&
StateNotificationChoice::makeLeaderPassive(const LeaderPassive& value)
{
    if (SELECTION_ID_LEADER_PASSIVE == d_selectionId) {
        d_leaderPassive.object() = value;
    }
    else {
        reset();
        new (d_leaderPassive.buffer()) LeaderPassive(value);
        d_selectionId = SELECTION_ID_LEADER_PASSIVE;
    }

    return d_leaderPassive.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderPassive&
StateNotificationChoice::makeLeaderPassive(LeaderPassive&& value)
{
    if (SELECTION_ID_LEADER_PASSIVE == d_selectionId) {
        d_leaderPassive.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leaderPassive.buffer()) LeaderPassive(bsl::move(value));
        d_selectionId = SELECTION_ID_LEADER_PASSIVE;
    }

    return d_leaderPassive.object();
}
#endif

// ACCESSORS

bsl::ostream& StateNotificationChoice::print(bsl::ostream& stream,
                                             int           level,
                                             int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_LEADER_PASSIVE: {
        printer.printAttribute("leaderPassive", d_leaderPassive.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* StateNotificationChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_LEADER_PASSIVE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_PASSIVE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ------------
// class Status
// ------------

// CONSTANTS

const char Status::CLASS_NAME[] = "Status";

const char Status::DEFAULT_INITIALIZER_MESSAGE[] = "";

const bdlat_AttributeInfo Status::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CATEGORY,
     "category",
     sizeof("category") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CODE,
     "code",
     sizeof("code") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MESSAGE,
     "message",
     sizeof("message") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Status::lookupAttributeInfo(const char* name,
                                                       int         nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Status::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Status::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CATEGORY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CATEGORY];
    case ATTRIBUTE_ID_CODE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CODE];
    case ATTRIBUTE_ID_MESSAGE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE];
    default: return 0;
    }
}

// CREATORS

Status::Status(bslma::Allocator* basicAllocator)
: d_message(DEFAULT_INITIALIZER_MESSAGE, basicAllocator)
, d_code()
, d_category(static_cast<StatusCategory::Value>(0))
{
}

Status::Status(const Status& original, bslma::Allocator* basicAllocator)
: d_message(original.d_message, basicAllocator)
, d_code(original.d_code)
, d_category(original.d_category)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Status::Status(Status&& original) noexcept
: d_message(bsl::move(original.d_message)),
  d_code(bsl::move(original.d_code)),
  d_category(bsl::move(original.d_category))
{
}

Status::Status(Status&& original, bslma::Allocator* basicAllocator)
: d_message(bsl::move(original.d_message), basicAllocator)
, d_code(bsl::move(original.d_code))
, d_category(bsl::move(original.d_category))
{
}
#endif

Status::~Status()
{
}

// MANIPULATORS

Status& Status::operator=(const Status& rhs)
{
    if (this != &rhs) {
        d_category = rhs.d_category;
        d_code     = rhs.d_code;
        d_message  = rhs.d_message;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Status& Status::operator=(Status&& rhs)
{
    if (this != &rhs) {
        d_category = bsl::move(rhs.d_category);
        d_code     = bsl::move(rhs.d_code);
        d_message  = bsl::move(rhs.d_message);
    }

    return *this;
}
#endif

void Status::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_category);
    bdlat_ValueTypeFunctions::reset(&d_code);
    d_message = DEFAULT_INITIALIZER_MESSAGE;
}

// ACCESSORS

bsl::ostream&
Status::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("category", this->category());
    printer.printAttribute("code", this->code());
    printer.printAttribute("message", this->message());
    printer.end();
    return stream;
}

// -------------------------
// class StorageSyncResponse
// -------------------------

// CONSTANTS

const char StorageSyncResponse::CLASS_NAME[] = "StorageSyncResponse";

const bdlat_AttributeInfo StorageSyncResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STORAGE_SYNC_RESPONSE_TYPE,
     "storageSyncResponseType",
     sizeof("storageSyncResponseType") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_BEGIN_SYNC_POINT,
     "beginSyncPoint",
     sizeof("beginSyncPoint") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_END_SYNC_POINT,
     "endSyncPoint",
     sizeof("endSyncPoint") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StorageSyncResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StorageSyncResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StorageSyncResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_STORAGE_SYNC_RESPONSE_TYPE:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_STORAGE_SYNC_RESPONSE_TYPE];
    case ATTRIBUTE_ID_BEGIN_SYNC_POINT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BEGIN_SYNC_POINT];
    case ATTRIBUTE_ID_END_SYNC_POINT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_END_SYNC_POINT];
    default: return 0;
    }
}

// CREATORS

StorageSyncResponse::StorageSyncResponse()
: d_beginSyncPoint()
, d_endSyncPoint()
, d_partitionId()
, d_storageSyncResponseType(static_cast<StorageSyncResponseType::Value>(0))
{
}

// MANIPULATORS

void StorageSyncResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_storageSyncResponseType);
    bdlat_ValueTypeFunctions::reset(&d_beginSyncPoint);
    bdlat_ValueTypeFunctions::reset(&d_endSyncPoint);
}

// ACCESSORS

bsl::ostream& StorageSyncResponse::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("storageSyncResponseType",
                           this->storageSyncResponseType());
    printer.printAttribute("beginSyncPoint", this->beginSyncPoint());
    printer.printAttribute("endSyncPoint", this->endSyncPoint());
    printer.end();
    return stream;
}

// -------------------------
// class SyncPointOffsetPair
// -------------------------

// CONSTANTS

const char SyncPointOffsetPair::CLASS_NAME[] = "SyncPointOffsetPair";

const bdlat_AttributeInfo SyncPointOffsetPair::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SYNC_POINT,
     "syncPoint",
     sizeof("syncPoint") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_OFFSET,
     "offset",
     sizeof("offset") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
SyncPointOffsetPair::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            SyncPointOffsetPair::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* SyncPointOffsetPair::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SYNC_POINT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYNC_POINT];
    case ATTRIBUTE_ID_OFFSET:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OFFSET];
    default: return 0;
    }
}

// CREATORS

SyncPointOffsetPair::SyncPointOffsetPair()
: d_offset()
, d_syncPoint()
{
}

// MANIPULATORS

void SyncPointOffsetPair::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_syncPoint);
    bdlat_ValueTypeFunctions::reset(&d_offset);
}

// ACCESSORS

bsl::ostream& SyncPointOffsetPair::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("syncPoint", this->syncPoint());
    printer.printAttribute("offset", this->offset());
    printer.end();
    return stream;
}

// --------------------
// class BrokerResponse
// --------------------

// CONSTANTS

const char BrokerResponse::CLASS_NAME[] = "BrokerResponse";

const bool BrokerResponse::DEFAULT_INITIALIZER_IS_DEPRECATED_SDK = false;

const bdlat_AttributeInfo BrokerResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_RESULT,
     "result",
     sizeof("result") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PROTOCOL_VERSION,
     "protocolVersion",
     sizeof("protocolVersion") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BROKER_VERSION,
     "brokerVersion",
     sizeof("brokerVersion") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_IS_DEPRECATED_SDK,
     "isDeprecatedSdk",
     sizeof("isDeprecatedSdk") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_BROKER_IDENTITY,
     "brokerIdentity",
     sizeof("brokerIdentity") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
BrokerResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            BrokerResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* BrokerResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_RESULT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESULT];
    case ATTRIBUTE_ID_PROTOCOL_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROTOCOL_VERSION];
    case ATTRIBUTE_ID_BROKER_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION];
    case ATTRIBUTE_ID_IS_DEPRECATED_SDK:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_DEPRECATED_SDK];
    case ATTRIBUTE_ID_BROKER_IDENTITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_IDENTITY];
    default: return 0;
    }
}

// CREATORS

BrokerResponse::BrokerResponse(bslma::Allocator* basicAllocator)
: d_result(basicAllocator)
, d_brokerIdentity(basicAllocator)
, d_protocolVersion()
, d_brokerVersion()
, d_isDeprecatedSdk(DEFAULT_INITIALIZER_IS_DEPRECATED_SDK)
{
}

BrokerResponse::BrokerResponse(const BrokerResponse& original,
                               bslma::Allocator*     basicAllocator)
: d_result(original.d_result, basicAllocator)
, d_brokerIdentity(original.d_brokerIdentity, basicAllocator)
, d_protocolVersion(original.d_protocolVersion)
, d_brokerVersion(original.d_brokerVersion)
, d_isDeprecatedSdk(original.d_isDeprecatedSdk)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerResponse::BrokerResponse(BrokerResponse&& original) noexcept
: d_result(bsl::move(original.d_result)),
  d_brokerIdentity(bsl::move(original.d_brokerIdentity)),
  d_protocolVersion(bsl::move(original.d_protocolVersion)),
  d_brokerVersion(bsl::move(original.d_brokerVersion)),
  d_isDeprecatedSdk(bsl::move(original.d_isDeprecatedSdk))
{
}

BrokerResponse::BrokerResponse(BrokerResponse&&  original,
                               bslma::Allocator* basicAllocator)
: d_result(bsl::move(original.d_result), basicAllocator)
, d_brokerIdentity(bsl::move(original.d_brokerIdentity), basicAllocator)
, d_protocolVersion(bsl::move(original.d_protocolVersion))
, d_brokerVersion(bsl::move(original.d_brokerVersion))
, d_isDeprecatedSdk(bsl::move(original.d_isDeprecatedSdk))
{
}
#endif

BrokerResponse::~BrokerResponse()
{
}

// MANIPULATORS

BrokerResponse& BrokerResponse::operator=(const BrokerResponse& rhs)
{
    if (this != &rhs) {
        d_result          = rhs.d_result;
        d_protocolVersion = rhs.d_protocolVersion;
        d_brokerVersion   = rhs.d_brokerVersion;
        d_isDeprecatedSdk = rhs.d_isDeprecatedSdk;
        d_brokerIdentity  = rhs.d_brokerIdentity;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerResponse& BrokerResponse::operator=(BrokerResponse&& rhs)
{
    if (this != &rhs) {
        d_result          = bsl::move(rhs.d_result);
        d_protocolVersion = bsl::move(rhs.d_protocolVersion);
        d_brokerVersion   = bsl::move(rhs.d_brokerVersion);
        d_isDeprecatedSdk = bsl::move(rhs.d_isDeprecatedSdk);
        d_brokerIdentity  = bsl::move(rhs.d_brokerIdentity);
    }

    return *this;
}
#endif

void BrokerResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_result);
    bdlat_ValueTypeFunctions::reset(&d_protocolVersion);
    bdlat_ValueTypeFunctions::reset(&d_brokerVersion);
    d_isDeprecatedSdk = DEFAULT_INITIALIZER_IS_DEPRECATED_SDK;
    bdlat_ValueTypeFunctions::reset(&d_brokerIdentity);
}

// ACCESSORS

bsl::ostream& BrokerResponse::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("result", this->result());
    printer.printAttribute("protocolVersion", this->protocolVersion());
    printer.printAttribute("brokerVersion", this->brokerVersion());
    printer.printAttribute("isDeprecatedSdk", this->isDeprecatedSdk());
    printer.printAttribute("brokerIdentity", this->brokerIdentity());
    printer.end();
    return stream;
}

// ----------------
// class CloseQueue
// ----------------

// CONSTANTS

const char CloseQueue::CLASS_NAME[] = "CloseQueue";

const bool CloseQueue::DEFAULT_INITIALIZER_IS_FINAL = false;

const bdlat_AttributeInfo CloseQueue::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_HANDLE_PARAMETERS,
     "handleParameters",
     sizeof("handleParameters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_IS_FINAL,
     "isFinal",
     sizeof("isFinal") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* CloseQueue::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            CloseQueue::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* CloseQueue::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_HANDLE_PARAMETERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HANDLE_PARAMETERS];
    case ATTRIBUTE_ID_IS_FINAL:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_FINAL];
    default: return 0;
    }
}

// CREATORS

CloseQueue::CloseQueue(bslma::Allocator* basicAllocator)
: d_handleParameters(basicAllocator)
, d_isFinal(DEFAULT_INITIALIZER_IS_FINAL)
{
}

CloseQueue::CloseQueue(const CloseQueue& original,
                       bslma::Allocator* basicAllocator)
: d_handleParameters(original.d_handleParameters, basicAllocator)
, d_isFinal(original.d_isFinal)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CloseQueue::CloseQueue(CloseQueue&& original) noexcept
: d_handleParameters(bsl::move(original.d_handleParameters)),
  d_isFinal(bsl::move(original.d_isFinal))
{
}

CloseQueue::CloseQueue(CloseQueue&& original, bslma::Allocator* basicAllocator)
: d_handleParameters(bsl::move(original.d_handleParameters), basicAllocator)
, d_isFinal(bsl::move(original.d_isFinal))
{
}
#endif

CloseQueue::~CloseQueue()
{
}

// MANIPULATORS

CloseQueue& CloseQueue::operator=(const CloseQueue& rhs)
{
    if (this != &rhs) {
        d_handleParameters = rhs.d_handleParameters;
        d_isFinal          = rhs.d_isFinal;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CloseQueue& CloseQueue::operator=(CloseQueue&& rhs)
{
    if (this != &rhs) {
        d_handleParameters = bsl::move(rhs.d_handleParameters);
        d_isFinal          = bsl::move(rhs.d_isFinal);
    }

    return *this;
}
#endif

void CloseQueue::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_handleParameters);
    d_isFinal = DEFAULT_INITIALIZER_IS_FINAL;
}

// ACCESSORS

bsl::ostream&
CloseQueue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("handleParameters", this->handleParameters());
    printer.printAttribute("isFinal", this->isFinal());
    printer.end();
    return stream;
}

// --------------------------
// class ConfigureQueueStream
// --------------------------

// CONSTANTS

const char ConfigureQueueStream::CLASS_NAME[] = "ConfigureQueueStream";

const bdlat_AttributeInfo ConfigureQueueStream::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_Q_ID,
     "qId",
     sizeof("qId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STREAM_PARAMETERS,
     "streamParameters",
     sizeof("streamParameters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ConfigureQueueStream::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ConfigureQueueStream::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ConfigureQueueStream::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_Q_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_Q_ID];
    case ATTRIBUTE_ID_STREAM_PARAMETERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STREAM_PARAMETERS];
    default: return 0;
    }
}

// CREATORS

ConfigureQueueStream::ConfigureQueueStream(bslma::Allocator* basicAllocator)
: d_streamParameters(basicAllocator)
, d_qId()
{
}

ConfigureQueueStream::ConfigureQueueStream(
    const ConfigureQueueStream& original,
    bslma::Allocator*           basicAllocator)
: d_streamParameters(original.d_streamParameters, basicAllocator)
, d_qId(original.d_qId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureQueueStream::ConfigureQueueStream(ConfigureQueueStream&& original)
    noexcept : d_streamParameters(bsl::move(original.d_streamParameters)),
               d_qId(bsl::move(original.d_qId))
{
}

ConfigureQueueStream::ConfigureQueueStream(ConfigureQueueStream&& original,
                                           bslma::Allocator* basicAllocator)
: d_streamParameters(bsl::move(original.d_streamParameters), basicAllocator)
, d_qId(bsl::move(original.d_qId))
{
}
#endif

ConfigureQueueStream::~ConfigureQueueStream()
{
}

// MANIPULATORS

ConfigureQueueStream&
ConfigureQueueStream::operator=(const ConfigureQueueStream& rhs)
{
    if (this != &rhs) {
        d_qId              = rhs.d_qId;
        d_streamParameters = rhs.d_streamParameters;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureQueueStream&
ConfigureQueueStream::operator=(ConfigureQueueStream&& rhs)
{
    if (this != &rhs) {
        d_qId              = bsl::move(rhs.d_qId);
        d_streamParameters = bsl::move(rhs.d_streamParameters);
    }

    return *this;
}
#endif

void ConfigureQueueStream::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_qId);
    bdlat_ValueTypeFunctions::reset(&d_streamParameters);
}

// ACCESSORS

bsl::ostream& ConfigureQueueStream::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("qId", this->qId());
    printer.printAttribute("streamParameters", this->streamParameters());
    printer.end();
    return stream;
}

// --------------------
// class ElectorMessage
// --------------------

// CONSTANTS

const char ElectorMessage::CLASS_NAME[] = "ElectorMessage";

const bdlat_AttributeInfo ElectorMessage::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_TERM,
     "term",
     sizeof("term") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo*
ElectorMessage::lookupAttributeInfo(const char* name, int nameLength)
{
    if (bdlb::String::areEqualCaseless("electionProposal", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("electionResponse", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("leaderHeartbeat", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("electorNodeStatus",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("heartbeatResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("scoutingRequest", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("scoutingResponse", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("leadershipCessionNotification",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ElectorMessage::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ElectorMessage::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_TERM: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TERM];
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

ElectorMessage::ElectorMessage()
: d_term()
, d_choice()
{
}

// MANIPULATORS

void ElectorMessage::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_term);
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream& ElectorMessage::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("term", this->term());
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// --------------------
// class LeaderAdvisory
// --------------------

// CONSTANTS

const char LeaderAdvisory::CLASS_NAME[] = "LeaderAdvisory";

const bdlat_AttributeInfo LeaderAdvisory::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PARTITIONS,
     "partitions",
     sizeof("partitions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUES,
     "queues",
     sizeof("queues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
LeaderAdvisory::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LeaderAdvisory::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* LeaderAdvisory::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    case ATTRIBUTE_ID_PARTITIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITIONS];
    case ATTRIBUTE_ID_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES];
    default: return 0;
    }
}

// CREATORS

LeaderAdvisory::LeaderAdvisory(bslma::Allocator* basicAllocator)
: d_queues(basicAllocator)
, d_partitions(basicAllocator)
, d_sequenceNumber()
{
}

LeaderAdvisory::LeaderAdvisory(const LeaderAdvisory& original,
                               bslma::Allocator*     basicAllocator)
: d_queues(original.d_queues, basicAllocator)
, d_partitions(original.d_partitions, basicAllocator)
, d_sequenceNumber(original.d_sequenceNumber)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderAdvisory::LeaderAdvisory(LeaderAdvisory&& original) noexcept
: d_queues(bsl::move(original.d_queues)),
  d_partitions(bsl::move(original.d_partitions)),
  d_sequenceNumber(bsl::move(original.d_sequenceNumber))
{
}

LeaderAdvisory::LeaderAdvisory(LeaderAdvisory&&  original,
                               bslma::Allocator* basicAllocator)
: d_queues(bsl::move(original.d_queues), basicAllocator)
, d_partitions(bsl::move(original.d_partitions), basicAllocator)
, d_sequenceNumber(bsl::move(original.d_sequenceNumber))
{
}
#endif

LeaderAdvisory::~LeaderAdvisory()
{
}

// MANIPULATORS

LeaderAdvisory& LeaderAdvisory::operator=(const LeaderAdvisory& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = rhs.d_sequenceNumber;
        d_partitions     = rhs.d_partitions;
        d_queues         = rhs.d_queues;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderAdvisory& LeaderAdvisory::operator=(LeaderAdvisory&& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = bsl::move(rhs.d_sequenceNumber);
        d_partitions     = bsl::move(rhs.d_partitions);
        d_queues         = bsl::move(rhs.d_queues);
    }

    return *this;
}
#endif

void LeaderAdvisory::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
    bdlat_ValueTypeFunctions::reset(&d_partitions);
    bdlat_ValueTypeFunctions::reset(&d_queues);
}

// ACCESSORS

bsl::ostream& LeaderAdvisory::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.printAttribute("partitions", this->partitions());
    printer.printAttribute("queues", this->queues());
    printer.end();
    return stream;
}

// ---------------
// class OpenQueue
// ---------------

// CONSTANTS

const char OpenQueue::CLASS_NAME[] = "OpenQueue";

const bdlat_AttributeInfo OpenQueue::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_HANDLE_PARAMETERS,
     "handleParameters",
     sizeof("handleParameters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* OpenQueue::lookupAttributeInfo(const char* name,
                                                          int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            OpenQueue::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* OpenQueue::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_HANDLE_PARAMETERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HANDLE_PARAMETERS];
    default: return 0;
    }
}

// CREATORS

OpenQueue::OpenQueue(bslma::Allocator* basicAllocator)
: d_handleParameters(basicAllocator)
{
}

OpenQueue::OpenQueue(const OpenQueue&  original,
                     bslma::Allocator* basicAllocator)
: d_handleParameters(original.d_handleParameters, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenQueue::OpenQueue(OpenQueue&& original) noexcept
: d_handleParameters(bsl::move(original.d_handleParameters))
{
}

OpenQueue::OpenQueue(OpenQueue&& original, bslma::Allocator* basicAllocator)
: d_handleParameters(bsl::move(original.d_handleParameters), basicAllocator)
{
}
#endif

OpenQueue::~OpenQueue()
{
}

// MANIPULATORS

OpenQueue& OpenQueue::operator=(const OpenQueue& rhs)
{
    if (this != &rhs) {
        d_handleParameters = rhs.d_handleParameters;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenQueue& OpenQueue::operator=(OpenQueue&& rhs)
{
    if (this != &rhs) {
        d_handleParameters = bsl::move(rhs.d_handleParameters);
    }

    return *this;
}
#endif

void OpenQueue::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_handleParameters);
}

// ACCESSORS

bsl::ostream&
OpenQueue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("handleParameters", this->handleParameters());
    printer.end();
    return stream;
}

// ----------------------------
// class PartitionMessageChoice
// ----------------------------

// CONSTANTS

const char PartitionMessageChoice::CLASS_NAME[] = "PartitionMessageChoice";

const bdlat_SelectionInfo PartitionMessageChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_REPLICA_STATE_REQUEST,
     "replicaStateRequest",
     sizeof("replicaStateRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_REPLICA_STATE_RESPONSE,
     "replicaStateResponse",
     sizeof("replicaStateResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PRIMARY_STATE_REQUEST,
     "primaryStateRequest",
     sizeof("primaryStateRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PRIMARY_STATE_RESPONSE,
     "primaryStateResponse",
     sizeof("primaryStateResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_REPLICA_DATA_REQUEST,
     "replicaDataRequest",
     sizeof("replicaDataRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_REPLICA_DATA_RESPONSE,
     "replicaDataResponse",
     sizeof("replicaDataResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
PartitionMessageChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            PartitionMessageChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* PartitionMessageChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_REPLICA_STATE_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICA_STATE_REQUEST];
    case SELECTION_ID_REPLICA_STATE_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICA_STATE_RESPONSE];
    case SELECTION_ID_PRIMARY_STATE_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIMARY_STATE_REQUEST];
    case SELECTION_ID_PRIMARY_STATE_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIMARY_STATE_RESPONSE];
    case SELECTION_ID_REPLICA_DATA_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICA_DATA_REQUEST];
    case SELECTION_ID_REPLICA_DATA_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICA_DATA_RESPONSE];
    default: return 0;
    }
}

// CREATORS

PartitionMessageChoice::PartitionMessageChoice(
    const PartitionMessageChoice& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_REPLICA_STATE_REQUEST: {
        new (d_replicaStateRequest.buffer())
            ReplicaStateRequest(original.d_replicaStateRequest.object());
    } break;
    case SELECTION_ID_REPLICA_STATE_RESPONSE: {
        new (d_replicaStateResponse.buffer())
            ReplicaStateResponse(original.d_replicaStateResponse.object());
    } break;
    case SELECTION_ID_PRIMARY_STATE_REQUEST: {
        new (d_primaryStateRequest.buffer())
            PrimaryStateRequest(original.d_primaryStateRequest.object());
    } break;
    case SELECTION_ID_PRIMARY_STATE_RESPONSE: {
        new (d_primaryStateResponse.buffer())
            PrimaryStateResponse(original.d_primaryStateResponse.object());
    } break;
    case SELECTION_ID_REPLICA_DATA_REQUEST: {
        new (d_replicaDataRequest.buffer())
            ReplicaDataRequest(original.d_replicaDataRequest.object());
    } break;
    case SELECTION_ID_REPLICA_DATA_RESPONSE: {
        new (d_replicaDataResponse.buffer())
            ReplicaDataResponse(original.d_replicaDataResponse.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionMessageChoice::PartitionMessageChoice(
    PartitionMessageChoice&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_REPLICA_STATE_REQUEST: {
        new (d_replicaStateRequest.buffer()) ReplicaStateRequest(
            bsl::move(original.d_replicaStateRequest.object()));
    } break;
    case SELECTION_ID_REPLICA_STATE_RESPONSE: {
        new (d_replicaStateResponse.buffer()) ReplicaStateResponse(
            bsl::move(original.d_replicaStateResponse.object()));
    } break;
    case SELECTION_ID_PRIMARY_STATE_REQUEST: {
        new (d_primaryStateRequest.buffer()) PrimaryStateRequest(
            bsl::move(original.d_primaryStateRequest.object()));
    } break;
    case SELECTION_ID_PRIMARY_STATE_RESPONSE: {
        new (d_primaryStateResponse.buffer()) PrimaryStateResponse(
            bsl::move(original.d_primaryStateResponse.object()));
    } break;
    case SELECTION_ID_REPLICA_DATA_REQUEST: {
        new (d_replicaDataRequest.buffer()) ReplicaDataRequest(
            bsl::move(original.d_replicaDataRequest.object()));
    } break;
    case SELECTION_ID_REPLICA_DATA_RESPONSE: {
        new (d_replicaDataResponse.buffer()) ReplicaDataResponse(
            bsl::move(original.d_replicaDataResponse.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

PartitionMessageChoice&
PartitionMessageChoice::operator=(const PartitionMessageChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_REPLICA_STATE_REQUEST: {
            makeReplicaStateRequest(rhs.d_replicaStateRequest.object());
        } break;
        case SELECTION_ID_REPLICA_STATE_RESPONSE: {
            makeReplicaStateResponse(rhs.d_replicaStateResponse.object());
        } break;
        case SELECTION_ID_PRIMARY_STATE_REQUEST: {
            makePrimaryStateRequest(rhs.d_primaryStateRequest.object());
        } break;
        case SELECTION_ID_PRIMARY_STATE_RESPONSE: {
            makePrimaryStateResponse(rhs.d_primaryStateResponse.object());
        } break;
        case SELECTION_ID_REPLICA_DATA_REQUEST: {
            makeReplicaDataRequest(rhs.d_replicaDataRequest.object());
        } break;
        case SELECTION_ID_REPLICA_DATA_RESPONSE: {
            makeReplicaDataResponse(rhs.d_replicaDataResponse.object());
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
PartitionMessageChoice&
PartitionMessageChoice::operator=(PartitionMessageChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_REPLICA_STATE_REQUEST: {
            makeReplicaStateRequest(
                bsl::move(rhs.d_replicaStateRequest.object()));
        } break;
        case SELECTION_ID_REPLICA_STATE_RESPONSE: {
            makeReplicaStateResponse(
                bsl::move(rhs.d_replicaStateResponse.object()));
        } break;
        case SELECTION_ID_PRIMARY_STATE_REQUEST: {
            makePrimaryStateRequest(
                bsl::move(rhs.d_primaryStateRequest.object()));
        } break;
        case SELECTION_ID_PRIMARY_STATE_RESPONSE: {
            makePrimaryStateResponse(
                bsl::move(rhs.d_primaryStateResponse.object()));
        } break;
        case SELECTION_ID_REPLICA_DATA_REQUEST: {
            makeReplicaDataRequest(
                bsl::move(rhs.d_replicaDataRequest.object()));
        } break;
        case SELECTION_ID_REPLICA_DATA_RESPONSE: {
            makeReplicaDataResponse(
                bsl::move(rhs.d_replicaDataResponse.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void PartitionMessageChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_REPLICA_STATE_REQUEST: {
        d_replicaStateRequest.object().~ReplicaStateRequest();
    } break;
    case SELECTION_ID_REPLICA_STATE_RESPONSE: {
        d_replicaStateResponse.object().~ReplicaStateResponse();
    } break;
    case SELECTION_ID_PRIMARY_STATE_REQUEST: {
        d_primaryStateRequest.object().~PrimaryStateRequest();
    } break;
    case SELECTION_ID_PRIMARY_STATE_RESPONSE: {
        d_primaryStateResponse.object().~PrimaryStateResponse();
    } break;
    case SELECTION_ID_REPLICA_DATA_REQUEST: {
        d_replicaDataRequest.object().~ReplicaDataRequest();
    } break;
    case SELECTION_ID_REPLICA_DATA_RESPONSE: {
        d_replicaDataResponse.object().~ReplicaDataResponse();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int PartitionMessageChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_REPLICA_STATE_REQUEST: {
        makeReplicaStateRequest();
    } break;
    case SELECTION_ID_REPLICA_STATE_RESPONSE: {
        makeReplicaStateResponse();
    } break;
    case SELECTION_ID_PRIMARY_STATE_REQUEST: {
        makePrimaryStateRequest();
    } break;
    case SELECTION_ID_PRIMARY_STATE_RESPONSE: {
        makePrimaryStateResponse();
    } break;
    case SELECTION_ID_REPLICA_DATA_REQUEST: {
        makeReplicaDataRequest();
    } break;
    case SELECTION_ID_REPLICA_DATA_RESPONSE: {
        makeReplicaDataResponse();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int PartitionMessageChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

ReplicaStateRequest& PartitionMessageChoice::makeReplicaStateRequest()
{
    if (SELECTION_ID_REPLICA_STATE_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_replicaStateRequest.object());
    }
    else {
        reset();
        new (d_replicaStateRequest.buffer()) ReplicaStateRequest();
        d_selectionId = SELECTION_ID_REPLICA_STATE_REQUEST;
    }

    return d_replicaStateRequest.object();
}

ReplicaStateRequest& PartitionMessageChoice::makeReplicaStateRequest(
    const ReplicaStateRequest& value)
{
    if (SELECTION_ID_REPLICA_STATE_REQUEST == d_selectionId) {
        d_replicaStateRequest.object() = value;
    }
    else {
        reset();
        new (d_replicaStateRequest.buffer()) ReplicaStateRequest(value);
        d_selectionId = SELECTION_ID_REPLICA_STATE_REQUEST;
    }

    return d_replicaStateRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReplicaStateRequest&
PartitionMessageChoice::makeReplicaStateRequest(ReplicaStateRequest&& value)
{
    if (SELECTION_ID_REPLICA_STATE_REQUEST == d_selectionId) {
        d_replicaStateRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_replicaStateRequest.buffer())
            ReplicaStateRequest(bsl::move(value));
        d_selectionId = SELECTION_ID_REPLICA_STATE_REQUEST;
    }

    return d_replicaStateRequest.object();
}
#endif

ReplicaStateResponse& PartitionMessageChoice::makeReplicaStateResponse()
{
    if (SELECTION_ID_REPLICA_STATE_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_replicaStateResponse.object());
    }
    else {
        reset();
        new (d_replicaStateResponse.buffer()) ReplicaStateResponse();
        d_selectionId = SELECTION_ID_REPLICA_STATE_RESPONSE;
    }

    return d_replicaStateResponse.object();
}

ReplicaStateResponse& PartitionMessageChoice::makeReplicaStateResponse(
    const ReplicaStateResponse& value)
{
    if (SELECTION_ID_REPLICA_STATE_RESPONSE == d_selectionId) {
        d_replicaStateResponse.object() = value;
    }
    else {
        reset();
        new (d_replicaStateResponse.buffer()) ReplicaStateResponse(value);
        d_selectionId = SELECTION_ID_REPLICA_STATE_RESPONSE;
    }

    return d_replicaStateResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReplicaStateResponse&
PartitionMessageChoice::makeReplicaStateResponse(ReplicaStateResponse&& value)
{
    if (SELECTION_ID_REPLICA_STATE_RESPONSE == d_selectionId) {
        d_replicaStateResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_replicaStateResponse.buffer())
            ReplicaStateResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_REPLICA_STATE_RESPONSE;
    }

    return d_replicaStateResponse.object();
}
#endif

PrimaryStateRequest& PartitionMessageChoice::makePrimaryStateRequest()
{
    if (SELECTION_ID_PRIMARY_STATE_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_primaryStateRequest.object());
    }
    else {
        reset();
        new (d_primaryStateRequest.buffer()) PrimaryStateRequest();
        d_selectionId = SELECTION_ID_PRIMARY_STATE_REQUEST;
    }

    return d_primaryStateRequest.object();
}

PrimaryStateRequest& PartitionMessageChoice::makePrimaryStateRequest(
    const PrimaryStateRequest& value)
{
    if (SELECTION_ID_PRIMARY_STATE_REQUEST == d_selectionId) {
        d_primaryStateRequest.object() = value;
    }
    else {
        reset();
        new (d_primaryStateRequest.buffer()) PrimaryStateRequest(value);
        d_selectionId = SELECTION_ID_PRIMARY_STATE_REQUEST;
    }

    return d_primaryStateRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PrimaryStateRequest&
PartitionMessageChoice::makePrimaryStateRequest(PrimaryStateRequest&& value)
{
    if (SELECTION_ID_PRIMARY_STATE_REQUEST == d_selectionId) {
        d_primaryStateRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_primaryStateRequest.buffer())
            PrimaryStateRequest(bsl::move(value));
        d_selectionId = SELECTION_ID_PRIMARY_STATE_REQUEST;
    }

    return d_primaryStateRequest.object();
}
#endif

PrimaryStateResponse& PartitionMessageChoice::makePrimaryStateResponse()
{
    if (SELECTION_ID_PRIMARY_STATE_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_primaryStateResponse.object());
    }
    else {
        reset();
        new (d_primaryStateResponse.buffer()) PrimaryStateResponse();
        d_selectionId = SELECTION_ID_PRIMARY_STATE_RESPONSE;
    }

    return d_primaryStateResponse.object();
}

PrimaryStateResponse& PartitionMessageChoice::makePrimaryStateResponse(
    const PrimaryStateResponse& value)
{
    if (SELECTION_ID_PRIMARY_STATE_RESPONSE == d_selectionId) {
        d_primaryStateResponse.object() = value;
    }
    else {
        reset();
        new (d_primaryStateResponse.buffer()) PrimaryStateResponse(value);
        d_selectionId = SELECTION_ID_PRIMARY_STATE_RESPONSE;
    }

    return d_primaryStateResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PrimaryStateResponse&
PartitionMessageChoice::makePrimaryStateResponse(PrimaryStateResponse&& value)
{
    if (SELECTION_ID_PRIMARY_STATE_RESPONSE == d_selectionId) {
        d_primaryStateResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_primaryStateResponse.buffer())
            PrimaryStateResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_PRIMARY_STATE_RESPONSE;
    }

    return d_primaryStateResponse.object();
}
#endif

ReplicaDataRequest& PartitionMessageChoice::makeReplicaDataRequest()
{
    if (SELECTION_ID_REPLICA_DATA_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_replicaDataRequest.object());
    }
    else {
        reset();
        new (d_replicaDataRequest.buffer()) ReplicaDataRequest();
        d_selectionId = SELECTION_ID_REPLICA_DATA_REQUEST;
    }

    return d_replicaDataRequest.object();
}

ReplicaDataRequest&
PartitionMessageChoice::makeReplicaDataRequest(const ReplicaDataRequest& value)
{
    if (SELECTION_ID_REPLICA_DATA_REQUEST == d_selectionId) {
        d_replicaDataRequest.object() = value;
    }
    else {
        reset();
        new (d_replicaDataRequest.buffer()) ReplicaDataRequest(value);
        d_selectionId = SELECTION_ID_REPLICA_DATA_REQUEST;
    }

    return d_replicaDataRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReplicaDataRequest&
PartitionMessageChoice::makeReplicaDataRequest(ReplicaDataRequest&& value)
{
    if (SELECTION_ID_REPLICA_DATA_REQUEST == d_selectionId) {
        d_replicaDataRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_replicaDataRequest.buffer())
            ReplicaDataRequest(bsl::move(value));
        d_selectionId = SELECTION_ID_REPLICA_DATA_REQUEST;
    }

    return d_replicaDataRequest.object();
}
#endif

ReplicaDataResponse& PartitionMessageChoice::makeReplicaDataResponse()
{
    if (SELECTION_ID_REPLICA_DATA_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_replicaDataResponse.object());
    }
    else {
        reset();
        new (d_replicaDataResponse.buffer()) ReplicaDataResponse();
        d_selectionId = SELECTION_ID_REPLICA_DATA_RESPONSE;
    }

    return d_replicaDataResponse.object();
}

ReplicaDataResponse& PartitionMessageChoice::makeReplicaDataResponse(
    const ReplicaDataResponse& value)
{
    if (SELECTION_ID_REPLICA_DATA_RESPONSE == d_selectionId) {
        d_replicaDataResponse.object() = value;
    }
    else {
        reset();
        new (d_replicaDataResponse.buffer()) ReplicaDataResponse(value);
        d_selectionId = SELECTION_ID_REPLICA_DATA_RESPONSE;
    }

    return d_replicaDataResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReplicaDataResponse&
PartitionMessageChoice::makeReplicaDataResponse(ReplicaDataResponse&& value)
{
    if (SELECTION_ID_REPLICA_DATA_RESPONSE == d_selectionId) {
        d_replicaDataResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_replicaDataResponse.buffer())
            ReplicaDataResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_REPLICA_DATA_RESPONSE;
    }

    return d_replicaDataResponse.object();
}
#endif

// ACCESSORS

bsl::ostream& PartitionMessageChoice::print(bsl::ostream& stream,
                                            int           level,
                                            int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_REPLICA_STATE_REQUEST: {
        printer.printAttribute("replicaStateRequest",
                               d_replicaStateRequest.object());
    } break;
    case SELECTION_ID_REPLICA_STATE_RESPONSE: {
        printer.printAttribute("replicaStateResponse",
                               d_replicaStateResponse.object());
    } break;
    case SELECTION_ID_PRIMARY_STATE_REQUEST: {
        printer.printAttribute("primaryStateRequest",
                               d_primaryStateRequest.object());
    } break;
    case SELECTION_ID_PRIMARY_STATE_RESPONSE: {
        printer.printAttribute("primaryStateResponse",
                               d_primaryStateResponse.object());
    } break;
    case SELECTION_ID_REPLICA_DATA_REQUEST: {
        printer.printAttribute("replicaDataRequest",
                               d_replicaDataRequest.object());
    } break;
    case SELECTION_ID_REPLICA_DATA_RESPONSE: {
        printer.printAttribute("replicaDataResponse",
                               d_replicaDataResponse.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* PartitionMessageChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_REPLICA_STATE_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICA_STATE_REQUEST]
            .name();
    case SELECTION_ID_REPLICA_STATE_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICA_STATE_RESPONSE]
            .name();
    case SELECTION_ID_PRIMARY_STATE_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIMARY_STATE_REQUEST]
            .name();
    case SELECTION_ID_PRIMARY_STATE_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIMARY_STATE_RESPONSE]
            .name();
    case SELECTION_ID_REPLICA_DATA_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICA_DATA_REQUEST]
            .name();
    case SELECTION_ID_REPLICA_DATA_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICA_DATA_RESPONSE]
            .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------------------
// class PartitionSyncDataQuery
// ----------------------------

// CONSTANTS

const char PartitionSyncDataQuery::CLASS_NAME[] = "PartitionSyncDataQuery";

const bdlat_AttributeInfo PartitionSyncDataQuery::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LAST_PRIMARY_LEASE_ID,
     "lastPrimaryLeaseId",
     sizeof("lastPrimaryLeaseId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LAST_SEQUENCE_NUM,
     "lastSequenceNum",
     sizeof("lastSequenceNum") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_UPTO_PRIMARY_LEASE_ID,
     "uptoPrimaryLeaseId",
     sizeof("uptoPrimaryLeaseId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_UPTO_SEQUENCE_NUM,
     "uptoSequenceNum",
     sizeof("uptoSequenceNum") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LAST_SYNC_POINT_OFFSET_PAIR,
     "lastSyncPointOffsetPair",
     sizeof("lastSyncPointOffsetPair") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionSyncDataQuery::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionSyncDataQuery::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PartitionSyncDataQuery::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_LAST_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LAST_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_LAST_SEQUENCE_NUM:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LAST_SEQUENCE_NUM];
    case ATTRIBUTE_ID_UPTO_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_UPTO_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_UPTO_SEQUENCE_NUM:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_UPTO_SEQUENCE_NUM];
    case ATTRIBUTE_ID_LAST_SYNC_POINT_OFFSET_PAIR:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_LAST_SYNC_POINT_OFFSET_PAIR];
    default: return 0;
    }
}

// CREATORS

PartitionSyncDataQuery::PartitionSyncDataQuery()
: d_lastSequenceNum()
, d_uptoSequenceNum()
, d_lastSyncPointOffsetPair()
, d_lastPrimaryLeaseId()
, d_uptoPrimaryLeaseId()
, d_partitionId()
{
}

// MANIPULATORS

void PartitionSyncDataQuery::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_lastPrimaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_lastSequenceNum);
    bdlat_ValueTypeFunctions::reset(&d_uptoPrimaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_uptoSequenceNum);
    bdlat_ValueTypeFunctions::reset(&d_lastSyncPointOffsetPair);
}

// ACCESSORS

bsl::ostream& PartitionSyncDataQuery::print(bsl::ostream& stream,
                                            int           level,
                                            int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("lastPrimaryLeaseId", this->lastPrimaryLeaseId());
    printer.printAttribute("lastSequenceNum", this->lastSequenceNum());
    printer.printAttribute("uptoPrimaryLeaseId", this->uptoPrimaryLeaseId());
    printer.printAttribute("uptoSequenceNum", this->uptoSequenceNum());
    printer.printAttribute("lastSyncPointOffsetPair",
                           this->lastSyncPointOffsetPair());
    printer.end();
    return stream;
}

// ----------------------------------
// class PartitionSyncDataQueryStatus
// ----------------------------------

// CONSTANTS

const char PartitionSyncDataQueryStatus::CLASS_NAME[] =
    "PartitionSyncDataQueryStatus";

const bdlat_AttributeInfo
    PartitionSyncDataQueryStatus::ATTRIBUTE_INFO_ARRAY[] = {
        {ATTRIBUTE_ID_PARTITION_ID,
         "partitionId",
         sizeof("partitionId") - 1,
         "",
         bdlat_FormattingMode::e_DEC},
        {ATTRIBUTE_ID_STATUS,
         "status",
         sizeof("status") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionSyncDataQueryStatus::lookupAttributeInfo(const char* name,
                                                  int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionSyncDataQueryStatus::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
PartitionSyncDataQueryStatus::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_STATUS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATUS];
    default: return 0;
    }
}

// CREATORS

PartitionSyncDataQueryStatus::PartitionSyncDataQueryStatus(
    bslma::Allocator* basicAllocator)
: d_status(basicAllocator)
, d_partitionId()
{
}

PartitionSyncDataQueryStatus::PartitionSyncDataQueryStatus(
    const PartitionSyncDataQueryStatus& original,
    bslma::Allocator*                   basicAllocator)
: d_status(original.d_status, basicAllocator)
, d_partitionId(original.d_partitionId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionSyncDataQueryStatus::PartitionSyncDataQueryStatus(
    PartitionSyncDataQueryStatus&& original) noexcept
: d_status(bsl::move(original.d_status)),
  d_partitionId(bsl::move(original.d_partitionId))
{
}

PartitionSyncDataQueryStatus::PartitionSyncDataQueryStatus(
    PartitionSyncDataQueryStatus&& original,
    bslma::Allocator*              basicAllocator)
: d_status(bsl::move(original.d_status), basicAllocator)
, d_partitionId(bsl::move(original.d_partitionId))
{
}
#endif

PartitionSyncDataQueryStatus::~PartitionSyncDataQueryStatus()
{
}

// MANIPULATORS

PartitionSyncDataQueryStatus& PartitionSyncDataQueryStatus::operator=(
    const PartitionSyncDataQueryStatus& rhs)
{
    if (this != &rhs) {
        d_partitionId = rhs.d_partitionId;
        d_status      = rhs.d_status;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionSyncDataQueryStatus&
PartitionSyncDataQueryStatus::operator=(PartitionSyncDataQueryStatus&& rhs)
{
    if (this != &rhs) {
        d_partitionId = bsl::move(rhs.d_partitionId);
        d_status      = bsl::move(rhs.d_status);
    }

    return *this;
}
#endif

void PartitionSyncDataQueryStatus::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_status);
}

// ACCESSORS

bsl::ostream& PartitionSyncDataQueryStatus::print(bsl::ostream& stream,
                                                  int           level,
                                                  int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("status", this->status());
    printer.end();
    return stream;
}

// -------------------------------------
// class PartitionSyncStateQueryResponse
// -------------------------------------

// CONSTANTS

const char PartitionSyncStateQueryResponse::CLASS_NAME[] =
    "PartitionSyncStateQueryResponse";

const bdlat_AttributeInfo
    PartitionSyncStateQueryResponse::ATTRIBUTE_INFO_ARRAY[] = {
        {ATTRIBUTE_ID_PARTITION_ID,
         "partitionId",
         sizeof("partitionId") - 1,
         "",
         bdlat_FormattingMode::e_DEC},
        {ATTRIBUTE_ID_PRIMARY_LEASE_ID,
         "primaryLeaseId",
         sizeof("primaryLeaseId") - 1,
         "",
         bdlat_FormattingMode::e_DEC},
        {ATTRIBUTE_ID_SEQUENCE_NUM,
         "sequenceNum",
         sizeof("sequenceNum") - 1,
         "",
         bdlat_FormattingMode::e_DEC},
        {ATTRIBUTE_ID_LAST_SYNC_POINT_OFFSET_PAIR,
         "lastSyncPointOffsetPair",
         sizeof("lastSyncPointOffsetPair") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionSyncStateQueryResponse::lookupAttributeInfo(const char* name,
                                                     int         nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionSyncStateQueryResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
PartitionSyncStateQueryResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_SEQUENCE_NUM:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUM];
    case ATTRIBUTE_ID_LAST_SYNC_POINT_OFFSET_PAIR:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_LAST_SYNC_POINT_OFFSET_PAIR];
    default: return 0;
    }
}

// CREATORS

PartitionSyncStateQueryResponse::PartitionSyncStateQueryResponse()
: d_sequenceNum()
, d_lastSyncPointOffsetPair()
, d_primaryLeaseId()
, d_partitionId()
{
}

// MANIPULATORS

void PartitionSyncStateQueryResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_primaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNum);
    bdlat_ValueTypeFunctions::reset(&d_lastSyncPointOffsetPair);
}

// ACCESSORS

bsl::ostream& PartitionSyncStateQueryResponse::print(bsl::ostream& stream,
                                                     int           level,
                                                     int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("primaryLeaseId", this->primaryLeaseId());
    printer.printAttribute("sequenceNum", this->sequenceNum());
    printer.printAttribute("lastSyncPointOffsetPair",
                           this->lastSyncPointOffsetPair());
    printer.end();
    return stream;
}

// -----------------------------
// class QueueAssignmentAdvisory
// -----------------------------

// CONSTANTS

const char QueueAssignmentAdvisory::CLASS_NAME[] = "QueueAssignmentAdvisory";

const bdlat_AttributeInfo QueueAssignmentAdvisory::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUES,
     "queues",
     sizeof("queues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueAssignmentAdvisory::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueAssignmentAdvisory::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueAssignmentAdvisory::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    case ATTRIBUTE_ID_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES];
    default: return 0;
    }
}

// CREATORS

QueueAssignmentAdvisory::QueueAssignmentAdvisory(
    bslma::Allocator* basicAllocator)
: d_queues(basicAllocator)
, d_sequenceNumber()
{
}

QueueAssignmentAdvisory::QueueAssignmentAdvisory(
    const QueueAssignmentAdvisory& original,
    bslma::Allocator*              basicAllocator)
: d_queues(original.d_queues, basicAllocator)
, d_sequenceNumber(original.d_sequenceNumber)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueAssignmentAdvisory::QueueAssignmentAdvisory(
    QueueAssignmentAdvisory&& original) noexcept
: d_queues(bsl::move(original.d_queues)),
  d_sequenceNumber(bsl::move(original.d_sequenceNumber))
{
}

QueueAssignmentAdvisory::QueueAssignmentAdvisory(
    QueueAssignmentAdvisory&& original,
    bslma::Allocator*         basicAllocator)
: d_queues(bsl::move(original.d_queues), basicAllocator)
, d_sequenceNumber(bsl::move(original.d_sequenceNumber))
{
}
#endif

QueueAssignmentAdvisory::~QueueAssignmentAdvisory()
{
}

// MANIPULATORS

QueueAssignmentAdvisory&
QueueAssignmentAdvisory::operator=(const QueueAssignmentAdvisory& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = rhs.d_sequenceNumber;
        d_queues         = rhs.d_queues;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueAssignmentAdvisory&
QueueAssignmentAdvisory::operator=(QueueAssignmentAdvisory&& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = bsl::move(rhs.d_sequenceNumber);
        d_queues         = bsl::move(rhs.d_queues);
    }

    return *this;
}
#endif

void QueueAssignmentAdvisory::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
    bdlat_ValueTypeFunctions::reset(&d_queues);
}

// ACCESSORS

bsl::ostream& QueueAssignmentAdvisory::print(bsl::ostream& stream,
                                             int           level,
                                             int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.printAttribute("queues", this->queues());
    printer.end();
    return stream;
}

// -------------------------------
// class QueueUnAssignmentAdvisory
// -------------------------------

// CONSTANTS

const char QueueUnAssignmentAdvisory::CLASS_NAME[] =
    "QueueUnAssignmentAdvisory";

const bdlat_AttributeInfo QueueUnAssignmentAdvisory::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PRIMARY_NODE_ID,
     "primaryNodeId",
     sizeof("primaryNodeId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIMARY_LEASE_ID,
     "primaryLeaseId",
     sizeof("primaryLeaseId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_QUEUES,
     "queues",
     sizeof("queues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueUnAssignmentAdvisory::lookupAttributeInfo(const char* name,
                                               int         nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueUnAssignmentAdvisory::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
QueueUnAssignmentAdvisory::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PRIMARY_NODE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_NODE_ID];
    case ATTRIBUTE_ID_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES];
    default: return 0;
    }
}

// CREATORS

QueueUnAssignmentAdvisory::QueueUnAssignmentAdvisory(
    bslma::Allocator* basicAllocator)
: d_queues(basicAllocator)
, d_primaryLeaseId()
, d_primaryNodeId()
, d_partitionId()
{
}

QueueUnAssignmentAdvisory::QueueUnAssignmentAdvisory(
    const QueueUnAssignmentAdvisory& original,
    bslma::Allocator*                basicAllocator)
: d_queues(original.d_queues, basicAllocator)
, d_primaryLeaseId(original.d_primaryLeaseId)
, d_primaryNodeId(original.d_primaryNodeId)
, d_partitionId(original.d_partitionId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUnAssignmentAdvisory::QueueUnAssignmentAdvisory(
    QueueUnAssignmentAdvisory&& original) noexcept
: d_queues(bsl::move(original.d_queues)),
  d_primaryLeaseId(bsl::move(original.d_primaryLeaseId)),
  d_primaryNodeId(bsl::move(original.d_primaryNodeId)),
  d_partitionId(bsl::move(original.d_partitionId))
{
}

QueueUnAssignmentAdvisory::QueueUnAssignmentAdvisory(
    QueueUnAssignmentAdvisory&& original,
    bslma::Allocator*           basicAllocator)
: d_queues(bsl::move(original.d_queues), basicAllocator)
, d_primaryLeaseId(bsl::move(original.d_primaryLeaseId))
, d_primaryNodeId(bsl::move(original.d_primaryNodeId))
, d_partitionId(bsl::move(original.d_partitionId))
{
}
#endif

QueueUnAssignmentAdvisory::~QueueUnAssignmentAdvisory()
{
}

// MANIPULATORS

QueueUnAssignmentAdvisory&
QueueUnAssignmentAdvisory::operator=(const QueueUnAssignmentAdvisory& rhs)
{
    if (this != &rhs) {
        d_primaryNodeId  = rhs.d_primaryNodeId;
        d_primaryLeaseId = rhs.d_primaryLeaseId;
        d_partitionId    = rhs.d_partitionId;
        d_queues         = rhs.d_queues;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUnAssignmentAdvisory&
QueueUnAssignmentAdvisory::operator=(QueueUnAssignmentAdvisory&& rhs)
{
    if (this != &rhs) {
        d_primaryNodeId  = bsl::move(rhs.d_primaryNodeId);
        d_primaryLeaseId = bsl::move(rhs.d_primaryLeaseId);
        d_partitionId    = bsl::move(rhs.d_partitionId);
        d_queues         = bsl::move(rhs.d_queues);
    }

    return *this;
}
#endif

void QueueUnAssignmentAdvisory::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_primaryNodeId);
    bdlat_ValueTypeFunctions::reset(&d_primaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_queues);
}

// ACCESSORS

bsl::ostream& QueueUnAssignmentAdvisory::print(bsl::ostream& stream,
                                               int           level,
                                               int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("primaryNodeId", this->primaryNodeId());
    printer.printAttribute("primaryLeaseId", this->primaryLeaseId());
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("queues", this->queues());
    printer.end();
    return stream;
}

// -----------------------------
// class QueueUnassignedAdvisory
// -----------------------------

// CONSTANTS

const char QueueUnassignedAdvisory::CLASS_NAME[] = "QueueUnassignedAdvisory";

const bdlat_AttributeInfo QueueUnassignedAdvisory::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIMARY_LEASE_ID,
     "primaryLeaseId",
     sizeof("primaryLeaseId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIMARY_NODE_ID,
     "primaryNodeId",
     sizeof("primaryNodeId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_QUEUES,
     "queues",
     sizeof("queues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueUnassignedAdvisory::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueUnassignedAdvisory::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueUnassignedAdvisory::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_PRIMARY_NODE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_NODE_ID];
    case ATTRIBUTE_ID_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES];
    default: return 0;
    }
}

// CREATORS

QueueUnassignedAdvisory::QueueUnassignedAdvisory(
    bslma::Allocator* basicAllocator)
: d_queues(basicAllocator)
, d_sequenceNumber()
, d_primaryLeaseId()
, d_partitionId()
, d_primaryNodeId()
{
}

QueueUnassignedAdvisory::QueueUnassignedAdvisory(
    const QueueUnassignedAdvisory& original,
    bslma::Allocator*              basicAllocator)
: d_queues(original.d_queues, basicAllocator)
, d_sequenceNumber(original.d_sequenceNumber)
, d_primaryLeaseId(original.d_primaryLeaseId)
, d_partitionId(original.d_partitionId)
, d_primaryNodeId(original.d_primaryNodeId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUnassignedAdvisory::QueueUnassignedAdvisory(
    QueueUnassignedAdvisory&& original) noexcept
: d_queues(bsl::move(original.d_queues)),
  d_sequenceNumber(bsl::move(original.d_sequenceNumber)),
  d_primaryLeaseId(bsl::move(original.d_primaryLeaseId)),
  d_partitionId(bsl::move(original.d_partitionId)),
  d_primaryNodeId(bsl::move(original.d_primaryNodeId))
{
}

QueueUnassignedAdvisory::QueueUnassignedAdvisory(
    QueueUnassignedAdvisory&& original,
    bslma::Allocator*         basicAllocator)
: d_queues(bsl::move(original.d_queues), basicAllocator)
, d_sequenceNumber(bsl::move(original.d_sequenceNumber))
, d_primaryLeaseId(bsl::move(original.d_primaryLeaseId))
, d_partitionId(bsl::move(original.d_partitionId))
, d_primaryNodeId(bsl::move(original.d_primaryNodeId))
{
}
#endif

QueueUnassignedAdvisory::~QueueUnassignedAdvisory()
{
}

// MANIPULATORS

QueueUnassignedAdvisory&
QueueUnassignedAdvisory::operator=(const QueueUnassignedAdvisory& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = rhs.d_sequenceNumber;
        d_partitionId    = rhs.d_partitionId;
        d_primaryLeaseId = rhs.d_primaryLeaseId;
        d_primaryNodeId  = rhs.d_primaryNodeId;
        d_queues         = rhs.d_queues;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUnassignedAdvisory&
QueueUnassignedAdvisory::operator=(QueueUnassignedAdvisory&& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = bsl::move(rhs.d_sequenceNumber);
        d_partitionId    = bsl::move(rhs.d_partitionId);
        d_primaryLeaseId = bsl::move(rhs.d_primaryLeaseId);
        d_primaryNodeId  = bsl::move(rhs.d_primaryNodeId);
        d_queues         = bsl::move(rhs.d_queues);
    }

    return *this;
}
#endif

void QueueUnassignedAdvisory::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_primaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_primaryNodeId);
    bdlat_ValueTypeFunctions::reset(&d_queues);
}

// ACCESSORS

bsl::ostream& QueueUnassignedAdvisory::print(bsl::ostream& stream,
                                             int           level,
                                             int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("primaryLeaseId", this->primaryLeaseId());
    printer.printAttribute("primaryNodeId", this->primaryNodeId());
    printer.printAttribute("queues", this->queues());
    printer.end();
    return stream;
}

// -------------------------
// class QueueUpdateAdvisory
// -------------------------

// CONSTANTS

const char QueueUpdateAdvisory::CLASS_NAME[] = "QueueUpdateAdvisory";

const bdlat_AttributeInfo QueueUpdateAdvisory::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SEQUENCE_NUMBER,
     "sequenceNumber",
     sizeof("sequenceNumber") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUE_UPDATES,
     "queueUpdates",
     sizeof("queueUpdates") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueUpdateAdvisory::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueUpdateAdvisory::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueUpdateAdvisory::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SEQUENCE_NUMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUMBER];
    case ATTRIBUTE_ID_QUEUE_UPDATES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_UPDATES];
    default: return 0;
    }
}

// CREATORS

QueueUpdateAdvisory::QueueUpdateAdvisory(bslma::Allocator* basicAllocator)
: d_queueUpdates(basicAllocator)
, d_sequenceNumber()
{
}

QueueUpdateAdvisory::QueueUpdateAdvisory(const QueueUpdateAdvisory& original,
                                         bslma::Allocator* basicAllocator)
: d_queueUpdates(original.d_queueUpdates, basicAllocator)
, d_sequenceNumber(original.d_sequenceNumber)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUpdateAdvisory::QueueUpdateAdvisory(QueueUpdateAdvisory&& original)
    noexcept : d_queueUpdates(bsl::move(original.d_queueUpdates)),
               d_sequenceNumber(bsl::move(original.d_sequenceNumber))
{
}

QueueUpdateAdvisory::QueueUpdateAdvisory(QueueUpdateAdvisory&& original,
                                         bslma::Allocator*     basicAllocator)
: d_queueUpdates(bsl::move(original.d_queueUpdates), basicAllocator)
, d_sequenceNumber(bsl::move(original.d_sequenceNumber))
{
}
#endif

QueueUpdateAdvisory::~QueueUpdateAdvisory()
{
}

// MANIPULATORS

QueueUpdateAdvisory&
QueueUpdateAdvisory::operator=(const QueueUpdateAdvisory& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = rhs.d_sequenceNumber;
        d_queueUpdates   = rhs.d_queueUpdates;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUpdateAdvisory& QueueUpdateAdvisory::operator=(QueueUpdateAdvisory&& rhs)
{
    if (this != &rhs) {
        d_sequenceNumber = bsl::move(rhs.d_sequenceNumber);
        d_queueUpdates   = bsl::move(rhs.d_queueUpdates);
    }

    return *this;
}
#endif

void QueueUpdateAdvisory::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sequenceNumber);
    bdlat_ValueTypeFunctions::reset(&d_queueUpdates);
}

// ACCESSORS

bsl::ostream& QueueUpdateAdvisory::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNumber", this->sequenceNumber());
    printer.printAttribute("queueUpdates", this->queueUpdates());
    printer.end();
    return stream;
}

// -----------------------
// class StateNotification
// -----------------------

// CONSTANTS

const char StateNotification::CLASS_NAME[] = "StateNotification";

const bdlat_AttributeInfo StateNotification::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo*
StateNotification::lookupAttributeInfo(const char* name, int nameLength)
{
    if (bdlb::String::areEqualCaseless("leaderPassive", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StateNotification::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StateNotification::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

StateNotification::StateNotification()
: d_choice()
{
}

// MANIPULATORS

void StateNotification::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream& StateNotification::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// ------------------------
// class StorageSyncRequest
// ------------------------

// CONSTANTS

const char StorageSyncRequest::CLASS_NAME[] = "StorageSyncRequest";

const bdlat_AttributeInfo StorageSyncRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BEGIN_SYNC_POINT_OFFSET_PAIR,
     "beginSyncPointOffsetPair",
     sizeof("beginSyncPointOffsetPair") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_END_SYNC_POINT_OFFSET_PAIR,
     "endSyncPointOffsetPair",
     sizeof("endSyncPointOffsetPair") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StorageSyncRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StorageSyncRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StorageSyncRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_BEGIN_SYNC_POINT_OFFSET_PAIR:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_BEGIN_SYNC_POINT_OFFSET_PAIR];
    case ATTRIBUTE_ID_END_SYNC_POINT_OFFSET_PAIR:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_END_SYNC_POINT_OFFSET_PAIR];
    default: return 0;
    }
}

// CREATORS

StorageSyncRequest::StorageSyncRequest()
: d_beginSyncPointOffsetPair()
, d_endSyncPointOffsetPair()
, d_partitionId()
{
}

// MANIPULATORS

void StorageSyncRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_beginSyncPointOffsetPair);
    bdlat_ValueTypeFunctions::reset(&d_endSyncPointOffsetPair);
}

// ACCESSORS

bsl::ostream& StorageSyncRequest::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("beginSyncPointOffsetPair",
                           this->beginSyncPointOffsetPair());
    printer.printAttribute("endSyncPointOffsetPair",
                           this->endSyncPointOffsetPair());
    printer.end();
    return stream;
}

// ------------------
// class Subscription
// ------------------

// CONSTANTS

const char Subscription::CLASS_NAME[] = "Subscription";

const bdlat_AttributeInfo Subscription::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_S_ID,
     "sId",
     sizeof("sId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_EXPRESSION,
     "expression",
     sizeof("expression") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CONSUMERS,
     "consumers",
     sizeof("consumers") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Subscription::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 3; ++i) {
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
    case ATTRIBUTE_ID_S_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_S_ID];
    case ATTRIBUTE_ID_EXPRESSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION];
    case ATTRIBUTE_ID_CONSUMERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMERS];
    default: return 0;
    }
}

// CREATORS

Subscription::Subscription(bslma::Allocator* basicAllocator)
: d_consumers(basicAllocator)
, d_expression(basicAllocator)
, d_sId()
{
}

Subscription::Subscription(const Subscription& original,
                           bslma::Allocator*   basicAllocator)
: d_consumers(original.d_consumers, basicAllocator)
, d_expression(original.d_expression, basicAllocator)
, d_sId(original.d_sId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Subscription::Subscription(Subscription&& original) noexcept
: d_consumers(bsl::move(original.d_consumers)),
  d_expression(bsl::move(original.d_expression)),
  d_sId(bsl::move(original.d_sId))
{
}

Subscription::Subscription(Subscription&&    original,
                           bslma::Allocator* basicAllocator)
: d_consumers(bsl::move(original.d_consumers), basicAllocator)
, d_expression(bsl::move(original.d_expression), basicAllocator)
, d_sId(bsl::move(original.d_sId))
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
        d_sId        = rhs.d_sId;
        d_expression = rhs.d_expression;
        d_consumers  = rhs.d_consumers;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Subscription& Subscription::operator=(Subscription&& rhs)
{
    if (this != &rhs) {
        d_sId        = bsl::move(rhs.d_sId);
        d_expression = bsl::move(rhs.d_expression);
        d_consumers  = bsl::move(rhs.d_consumers);
    }

    return *this;
}
#endif

void Subscription::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sId);
    bdlat_ValueTypeFunctions::reset(&d_expression);
    bdlat_ValueTypeFunctions::reset(&d_consumers);
}

// ACCESSORS

bsl::ostream&
Subscription::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sId", this->sId());
    printer.printAttribute("expression", this->expression());
    printer.printAttribute("consumers", this->consumers());
    printer.end();
    return stream;
}

// ----------------------------------
// class ConfigureQueueStreamResponse
// ----------------------------------

// CONSTANTS

const char ConfigureQueueStreamResponse::CLASS_NAME[] =
    "ConfigureQueueStreamResponse";

const bdlat_AttributeInfo
    ConfigureQueueStreamResponse::ATTRIBUTE_INFO_ARRAY[] = {
        {ATTRIBUTE_ID_REQUEST,
         "request",
         sizeof("request") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ConfigureQueueStreamResponse::lookupAttributeInfo(const char* name,
                                                  int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ConfigureQueueStreamResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
ConfigureQueueStreamResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_REQUEST:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REQUEST];
    default: return 0;
    }
}

// CREATORS

ConfigureQueueStreamResponse::ConfigureQueueStreamResponse(
    bslma::Allocator* basicAllocator)
: d_request(basicAllocator)
{
}

ConfigureQueueStreamResponse::ConfigureQueueStreamResponse(
    const ConfigureQueueStreamResponse& original,
    bslma::Allocator*                   basicAllocator)
: d_request(original.d_request, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureQueueStreamResponse::ConfigureQueueStreamResponse(
    ConfigureQueueStreamResponse&& original) noexcept
: d_request(bsl::move(original.d_request))
{
}

ConfigureQueueStreamResponse::ConfigureQueueStreamResponse(
    ConfigureQueueStreamResponse&& original,
    bslma::Allocator*              basicAllocator)
: d_request(bsl::move(original.d_request), basicAllocator)
{
}
#endif

ConfigureQueueStreamResponse::~ConfigureQueueStreamResponse()
{
}

// MANIPULATORS

ConfigureQueueStreamResponse& ConfigureQueueStreamResponse::operator=(
    const ConfigureQueueStreamResponse& rhs)
{
    if (this != &rhs) {
        d_request = rhs.d_request;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureQueueStreamResponse&
ConfigureQueueStreamResponse::operator=(ConfigureQueueStreamResponse&& rhs)
{
    if (this != &rhs) {
        d_request = bsl::move(rhs.d_request);
    }

    return *this;
}
#endif

void ConfigureQueueStreamResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_request);
}

// ACCESSORS

bsl::ostream& ConfigureQueueStreamResponse::print(bsl::ostream& stream,
                                                  int           level,
                                                  int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("request", this->request());
    printer.end();
    return stream;
}

// ----------------------------------
// class FollowerClusterStateResponse
// ----------------------------------

// CONSTANTS

const char FollowerClusterStateResponse::CLASS_NAME[] =
    "FollowerClusterStateResponse";

const bdlat_AttributeInfo
    FollowerClusterStateResponse::ATTRIBUTE_INFO_ARRAY[] = {
        {ATTRIBUTE_ID_CLUSTER_STATE_SNAPSHOT,
         "clusterStateSnapshot",
         sizeof("clusterStateSnapshot") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
FollowerClusterStateResponse::lookupAttributeInfo(const char* name,
                                                  int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            FollowerClusterStateResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
FollowerClusterStateResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLUSTER_STATE_SNAPSHOT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_STATE_SNAPSHOT];
    default: return 0;
    }
}

// CREATORS

FollowerClusterStateResponse::FollowerClusterStateResponse(
    bslma::Allocator* basicAllocator)
: d_clusterStateSnapshot(basicAllocator)
{
}

FollowerClusterStateResponse::FollowerClusterStateResponse(
    const FollowerClusterStateResponse& original,
    bslma::Allocator*                   basicAllocator)
: d_clusterStateSnapshot(original.d_clusterStateSnapshot, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FollowerClusterStateResponse::FollowerClusterStateResponse(
    FollowerClusterStateResponse&& original) noexcept
: d_clusterStateSnapshot(bsl::move(original.d_clusterStateSnapshot))
{
}

FollowerClusterStateResponse::FollowerClusterStateResponse(
    FollowerClusterStateResponse&& original,
    bslma::Allocator*              basicAllocator)
: d_clusterStateSnapshot(bsl::move(original.d_clusterStateSnapshot),
                         basicAllocator)
{
}
#endif

FollowerClusterStateResponse::~FollowerClusterStateResponse()
{
}

// MANIPULATORS

FollowerClusterStateResponse& FollowerClusterStateResponse::operator=(
    const FollowerClusterStateResponse& rhs)
{
    if (this != &rhs) {
        d_clusterStateSnapshot = rhs.d_clusterStateSnapshot;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FollowerClusterStateResponse&
FollowerClusterStateResponse::operator=(FollowerClusterStateResponse&& rhs)
{
    if (this != &rhs) {
        d_clusterStateSnapshot = bsl::move(rhs.d_clusterStateSnapshot);
    }

    return *this;
}
#endif

void FollowerClusterStateResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clusterStateSnapshot);
}

// ACCESSORS

bsl::ostream& FollowerClusterStateResponse::print(bsl::ostream& stream,
                                                  int           level,
                                                  int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clusterStateSnapshot",
                           this->clusterStateSnapshot());
    printer.end();
    return stream;
}

// ---------------------------------
// class LeaderSyncDataQueryResponse
// ---------------------------------

// CONSTANTS

const char LeaderSyncDataQueryResponse::CLASS_NAME[] =
    "LeaderSyncDataQueryResponse";

const bdlat_AttributeInfo LeaderSyncDataQueryResponse::ATTRIBUTE_INFO_ARRAY[] =
    {{ATTRIBUTE_ID_LEADER_SYNC_DATA,
      "leaderSyncData",
      sizeof("leaderSyncData") - 1,
      "",
      bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
LeaderSyncDataQueryResponse::lookupAttributeInfo(const char* name,
                                                 int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LeaderSyncDataQueryResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
LeaderSyncDataQueryResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_LEADER_SYNC_DATA:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_SYNC_DATA];
    default: return 0;
    }
}

// CREATORS

LeaderSyncDataQueryResponse::LeaderSyncDataQueryResponse(
    bslma::Allocator* basicAllocator)
: d_leaderSyncData(basicAllocator)
{
}

LeaderSyncDataQueryResponse::LeaderSyncDataQueryResponse(
    const LeaderSyncDataQueryResponse& original,
    bslma::Allocator*                  basicAllocator)
: d_leaderSyncData(original.d_leaderSyncData, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderSyncDataQueryResponse::LeaderSyncDataQueryResponse(
    LeaderSyncDataQueryResponse&& original) noexcept
: d_leaderSyncData(bsl::move(original.d_leaderSyncData))
{
}

LeaderSyncDataQueryResponse::LeaderSyncDataQueryResponse(
    LeaderSyncDataQueryResponse&& original,
    bslma::Allocator*             basicAllocator)
: d_leaderSyncData(bsl::move(original.d_leaderSyncData), basicAllocator)
{
}
#endif

LeaderSyncDataQueryResponse::~LeaderSyncDataQueryResponse()
{
}

// MANIPULATORS

LeaderSyncDataQueryResponse&
LeaderSyncDataQueryResponse::operator=(const LeaderSyncDataQueryResponse& rhs)
{
    if (this != &rhs) {
        d_leaderSyncData = rhs.d_leaderSyncData;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderSyncDataQueryResponse&
LeaderSyncDataQueryResponse::operator=(LeaderSyncDataQueryResponse&& rhs)
{
    if (this != &rhs) {
        d_leaderSyncData = bsl::move(rhs.d_leaderSyncData);
    }

    return *this;
}
#endif

void LeaderSyncDataQueryResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_leaderSyncData);
}

// ACCESSORS

bsl::ostream& LeaderSyncDataQueryResponse::print(bsl::ostream& stream,
                                                 int           level,
                                                 int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("leaderSyncData", this->leaderSyncData());
    printer.end();
    return stream;
}

// ------------------------
// class NegotiationMessage
// ------------------------

// CONSTANTS

const char NegotiationMessage::CLASS_NAME[] = "NegotiationMessage";

const bdlat_SelectionInfo NegotiationMessage::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_CLIENT_IDENTITY,
     "clientIdentity",
     sizeof("clientIdentity") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_BROKER_RESPONSE,
     "brokerResponse",
     sizeof("brokerResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_REVERSE_CONNECTION_REQUEST,
     "reverseConnectionRequest",
     sizeof("reverseConnectionRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
NegotiationMessage::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            NegotiationMessage::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* NegotiationMessage::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_CLIENT_IDENTITY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLIENT_IDENTITY];
    case SELECTION_ID_BROKER_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_BROKER_RESPONSE];
    case SELECTION_ID_REVERSE_CONNECTION_REQUEST:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_REVERSE_CONNECTION_REQUEST];
    default: return 0;
    }
}

// CREATORS

NegotiationMessage::NegotiationMessage(const NegotiationMessage& original,
                                       bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_CLIENT_IDENTITY: {
        new (d_clientIdentity.buffer())
            ClientIdentity(original.d_clientIdentity.object(), d_allocator_p);
    } break;
    case SELECTION_ID_BROKER_RESPONSE: {
        new (d_brokerResponse.buffer())
            BrokerResponse(original.d_brokerResponse.object(), d_allocator_p);
    } break;
    case SELECTION_ID_REVERSE_CONNECTION_REQUEST: {
        new (d_reverseConnectionRequest.buffer()) ReverseConnectionRequest(
            original.d_reverseConnectionRequest.object(),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
NegotiationMessage::NegotiationMessage(NegotiationMessage&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_CLIENT_IDENTITY: {
        new (d_clientIdentity.buffer())
            ClientIdentity(bsl::move(original.d_clientIdentity.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_BROKER_RESPONSE: {
        new (d_brokerResponse.buffer())
            BrokerResponse(bsl::move(original.d_brokerResponse.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_REVERSE_CONNECTION_REQUEST: {
        new (d_reverseConnectionRequest.buffer()) ReverseConnectionRequest(
            bsl::move(original.d_reverseConnectionRequest.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

NegotiationMessage::NegotiationMessage(NegotiationMessage&& original,
                                       bslma::Allocator*    basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_CLIENT_IDENTITY: {
        new (d_clientIdentity.buffer())
            ClientIdentity(bsl::move(original.d_clientIdentity.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_BROKER_RESPONSE: {
        new (d_brokerResponse.buffer())
            BrokerResponse(bsl::move(original.d_brokerResponse.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_REVERSE_CONNECTION_REQUEST: {
        new (d_reverseConnectionRequest.buffer()) ReverseConnectionRequest(
            bsl::move(original.d_reverseConnectionRequest.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

NegotiationMessage&
NegotiationMessage::operator=(const NegotiationMessage& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_CLIENT_IDENTITY: {
            makeClientIdentity(rhs.d_clientIdentity.object());
        } break;
        case SELECTION_ID_BROKER_RESPONSE: {
            makeBrokerResponse(rhs.d_brokerResponse.object());
        } break;
        case SELECTION_ID_REVERSE_CONNECTION_REQUEST: {
            makeReverseConnectionRequest(
                rhs.d_reverseConnectionRequest.object());
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
NegotiationMessage& NegotiationMessage::operator=(NegotiationMessage&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_CLIENT_IDENTITY: {
            makeClientIdentity(bsl::move(rhs.d_clientIdentity.object()));
        } break;
        case SELECTION_ID_BROKER_RESPONSE: {
            makeBrokerResponse(bsl::move(rhs.d_brokerResponse.object()));
        } break;
        case SELECTION_ID_REVERSE_CONNECTION_REQUEST: {
            makeReverseConnectionRequest(
                bsl::move(rhs.d_reverseConnectionRequest.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void NegotiationMessage::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_CLIENT_IDENTITY: {
        d_clientIdentity.object().~ClientIdentity();
    } break;
    case SELECTION_ID_BROKER_RESPONSE: {
        d_brokerResponse.object().~BrokerResponse();
    } break;
    case SELECTION_ID_REVERSE_CONNECTION_REQUEST: {
        d_reverseConnectionRequest.object().~ReverseConnectionRequest();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int NegotiationMessage::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_CLIENT_IDENTITY: {
        makeClientIdentity();
    } break;
    case SELECTION_ID_BROKER_RESPONSE: {
        makeBrokerResponse();
    } break;
    case SELECTION_ID_REVERSE_CONNECTION_REQUEST: {
        makeReverseConnectionRequest();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int NegotiationMessage::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

ClientIdentity& NegotiationMessage::makeClientIdentity()
{
    if (SELECTION_ID_CLIENT_IDENTITY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clientIdentity.object());
    }
    else {
        reset();
        new (d_clientIdentity.buffer()) ClientIdentity(d_allocator_p);
        d_selectionId = SELECTION_ID_CLIENT_IDENTITY;
    }

    return d_clientIdentity.object();
}

ClientIdentity&
NegotiationMessage::makeClientIdentity(const ClientIdentity& value)
{
    if (SELECTION_ID_CLIENT_IDENTITY == d_selectionId) {
        d_clientIdentity.object() = value;
    }
    else {
        reset();
        new (d_clientIdentity.buffer()) ClientIdentity(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLIENT_IDENTITY;
    }

    return d_clientIdentity.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClientIdentity& NegotiationMessage::makeClientIdentity(ClientIdentity&& value)
{
    if (SELECTION_ID_CLIENT_IDENTITY == d_selectionId) {
        d_clientIdentity.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clientIdentity.buffer())
            ClientIdentity(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLIENT_IDENTITY;
    }

    return d_clientIdentity.object();
}
#endif

BrokerResponse& NegotiationMessage::makeBrokerResponse()
{
    if (SELECTION_ID_BROKER_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_brokerResponse.object());
    }
    else {
        reset();
        new (d_brokerResponse.buffer()) BrokerResponse(d_allocator_p);
        d_selectionId = SELECTION_ID_BROKER_RESPONSE;
    }

    return d_brokerResponse.object();
}

BrokerResponse&
NegotiationMessage::makeBrokerResponse(const BrokerResponse& value)
{
    if (SELECTION_ID_BROKER_RESPONSE == d_selectionId) {
        d_brokerResponse.object() = value;
    }
    else {
        reset();
        new (d_brokerResponse.buffer()) BrokerResponse(value, d_allocator_p);
        d_selectionId = SELECTION_ID_BROKER_RESPONSE;
    }

    return d_brokerResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerResponse& NegotiationMessage::makeBrokerResponse(BrokerResponse&& value)
{
    if (SELECTION_ID_BROKER_RESPONSE == d_selectionId) {
        d_brokerResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_brokerResponse.buffer())
            BrokerResponse(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_BROKER_RESPONSE;
    }

    return d_brokerResponse.object();
}
#endif

ReverseConnectionRequest& NegotiationMessage::makeReverseConnectionRequest()
{
    if (SELECTION_ID_REVERSE_CONNECTION_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_reverseConnectionRequest.object());
    }
    else {
        reset();
        new (d_reverseConnectionRequest.buffer())
            ReverseConnectionRequest(d_allocator_p);
        d_selectionId = SELECTION_ID_REVERSE_CONNECTION_REQUEST;
    }

    return d_reverseConnectionRequest.object();
}

ReverseConnectionRequest& NegotiationMessage::makeReverseConnectionRequest(
    const ReverseConnectionRequest& value)
{
    if (SELECTION_ID_REVERSE_CONNECTION_REQUEST == d_selectionId) {
        d_reverseConnectionRequest.object() = value;
    }
    else {
        reset();
        new (d_reverseConnectionRequest.buffer())
            ReverseConnectionRequest(value, d_allocator_p);
        d_selectionId = SELECTION_ID_REVERSE_CONNECTION_REQUEST;
    }

    return d_reverseConnectionRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReverseConnectionRequest& NegotiationMessage::makeReverseConnectionRequest(
    ReverseConnectionRequest&& value)
{
    if (SELECTION_ID_REVERSE_CONNECTION_REQUEST == d_selectionId) {
        d_reverseConnectionRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_reverseConnectionRequest.buffer())
            ReverseConnectionRequest(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_REVERSE_CONNECTION_REQUEST;
    }

    return d_reverseConnectionRequest.object();
}
#endif

// ACCESSORS

bsl::ostream& NegotiationMessage::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_CLIENT_IDENTITY: {
        printer.printAttribute("clientIdentity", d_clientIdentity.object());
    } break;
    case SELECTION_ID_BROKER_RESPONSE: {
        printer.printAttribute("brokerResponse", d_brokerResponse.object());
    } break;
    case SELECTION_ID_REVERSE_CONNECTION_REQUEST: {
        printer.printAttribute("reverseConnectionRequest",
                               d_reverseConnectionRequest.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* NegotiationMessage::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_CLIENT_IDENTITY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLIENT_IDENTITY].name();
    case SELECTION_ID_BROKER_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_BROKER_RESPONSE].name();
    case SELECTION_ID_REVERSE_CONNECTION_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REVERSE_CONNECTION_REQUEST]
            .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------------
// class OpenQueueResponse
// -----------------------

// CONSTANTS

const char OpenQueueResponse::CLASS_NAME[] = "OpenQueueResponse";

const int OpenQueueResponse::DEFAULT_INITIALIZER_DEDUPLICATION_TIME_MS =
    300000;

const bdlat_AttributeInfo OpenQueueResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ORIGINAL_REQUEST,
     "originalRequest",
     sizeof("originalRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_ROUTING_CONFIGURATION,
     "routingConfiguration",
     sizeof("routingConfiguration") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_DEDUPLICATION_TIME_MS,
     "deduplicationTimeMs",
     sizeof("deduplicationTimeMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
OpenQueueResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            OpenQueueResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* OpenQueueResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ORIGINAL_REQUEST:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ORIGINAL_REQUEST];
    case ATTRIBUTE_ID_ROUTING_CONFIGURATION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROUTING_CONFIGURATION];
    case ATTRIBUTE_ID_DEDUPLICATION_TIME_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DEDUPLICATION_TIME_MS];
    default: return 0;
    }
}

// CREATORS

OpenQueueResponse::OpenQueueResponse(bslma::Allocator* basicAllocator)
: d_routingConfiguration()
, d_originalRequest(basicAllocator)
, d_deduplicationTimeMs(DEFAULT_INITIALIZER_DEDUPLICATION_TIME_MS)
{
}

OpenQueueResponse::OpenQueueResponse(const OpenQueueResponse& original,
                                     bslma::Allocator*        basicAllocator)
: d_routingConfiguration(original.d_routingConfiguration)
, d_originalRequest(original.d_originalRequest, basicAllocator)
, d_deduplicationTimeMs(original.d_deduplicationTimeMs)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenQueueResponse::OpenQueueResponse(OpenQueueResponse&& original) noexcept
: d_routingConfiguration(bsl::move(original.d_routingConfiguration)),
  d_originalRequest(bsl::move(original.d_originalRequest)),
  d_deduplicationTimeMs(bsl::move(original.d_deduplicationTimeMs))
{
}

OpenQueueResponse::OpenQueueResponse(OpenQueueResponse&& original,
                                     bslma::Allocator*   basicAllocator)
: d_routingConfiguration(bsl::move(original.d_routingConfiguration))
, d_originalRequest(bsl::move(original.d_originalRequest), basicAllocator)
, d_deduplicationTimeMs(bsl::move(original.d_deduplicationTimeMs))
{
}
#endif

OpenQueueResponse::~OpenQueueResponse()
{
}

// MANIPULATORS

OpenQueueResponse& OpenQueueResponse::operator=(const OpenQueueResponse& rhs)
{
    if (this != &rhs) {
        d_originalRequest      = rhs.d_originalRequest;
        d_routingConfiguration = rhs.d_routingConfiguration;
        d_deduplicationTimeMs  = rhs.d_deduplicationTimeMs;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenQueueResponse& OpenQueueResponse::operator=(OpenQueueResponse&& rhs)
{
    if (this != &rhs) {
        d_originalRequest      = bsl::move(rhs.d_originalRequest);
        d_routingConfiguration = bsl::move(rhs.d_routingConfiguration);
        d_deduplicationTimeMs  = bsl::move(rhs.d_deduplicationTimeMs);
    }

    return *this;
}
#endif

void OpenQueueResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_originalRequest);
    bdlat_ValueTypeFunctions::reset(&d_routingConfiguration);
    d_deduplicationTimeMs = DEFAULT_INITIALIZER_DEDUPLICATION_TIME_MS;
}

// ACCESSORS

bsl::ostream& OpenQueueResponse::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("originalRequest", this->originalRequest());
    printer.printAttribute("routingConfiguration",
                           this->routingConfiguration());
    printer.printAttribute("deduplicationTimeMs", this->deduplicationTimeMs());
    printer.end();
    return stream;
}

// ----------------------
// class PartitionMessage
// ----------------------

// CONSTANTS

const char PartitionMessage::CLASS_NAME[] = "PartitionMessage";

const bdlat_AttributeInfo PartitionMessage::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionMessage::lookupAttributeInfo(const char* name, int nameLength)
{
    if (bdlb::String::areEqualCaseless("replicaStateRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("replicaStateResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("primaryStateRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("primaryStateResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("replicaDataRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("replicaDataResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionMessage::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PartitionMessage::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

PartitionMessage::PartitionMessage()
: d_choice()
{
}

// MANIPULATORS

void PartitionMessage::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream& PartitionMessage::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// ----------------------
// class StreamParameters
// ----------------------

// CONSTANTS

const char StreamParameters::CLASS_NAME[] = "StreamParameters";

const char StreamParameters::DEFAULT_INITIALIZER_APP_ID[] = "__default";

const bdlat_AttributeInfo StreamParameters::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_SUBSCRIPTIONS,
     "subscriptions",
     sizeof("subscriptions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StreamParameters::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StreamParameters::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StreamParameters::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_SUBSCRIPTIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS];
    default: return 0;
    }
}

// CREATORS

StreamParameters::StreamParameters(bslma::Allocator* basicAllocator)
: d_subscriptions(basicAllocator)
, d_appId(DEFAULT_INITIALIZER_APP_ID, basicAllocator)
{
}

StreamParameters::StreamParameters(const StreamParameters& original,
                                   bslma::Allocator*       basicAllocator)
: d_subscriptions(original.d_subscriptions, basicAllocator)
, d_appId(original.d_appId, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StreamParameters::StreamParameters(StreamParameters&& original) noexcept
: d_subscriptions(bsl::move(original.d_subscriptions)),
  d_appId(bsl::move(original.d_appId))
{
}

StreamParameters::StreamParameters(StreamParameters&& original,
                                   bslma::Allocator*  basicAllocator)
: d_subscriptions(bsl::move(original.d_subscriptions), basicAllocator)
, d_appId(bsl::move(original.d_appId), basicAllocator)
{
}
#endif

StreamParameters::~StreamParameters()
{
}

// MANIPULATORS

StreamParameters& StreamParameters::operator=(const StreamParameters& rhs)
{
    if (this != &rhs) {
        d_appId         = rhs.d_appId;
        d_subscriptions = rhs.d_subscriptions;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StreamParameters& StreamParameters::operator=(StreamParameters&& rhs)
{
    if (this != &rhs) {
        d_appId         = bsl::move(rhs.d_appId);
        d_subscriptions = bsl::move(rhs.d_subscriptions);
    }

    return *this;
}
#endif

void StreamParameters::reset()
{
    d_appId = DEFAULT_INITIALIZER_APP_ID;
    bdlat_ValueTypeFunctions::reset(&d_subscriptions);
}

// ACCESSORS

bsl::ostream& StreamParameters::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("appId", this->appId());
    printer.printAttribute("subscriptions", this->subscriptions());
    printer.end();
    return stream;
}

// ----------------------------------
// class ClusterStateFSMMessageChoice
// ----------------------------------

// CONSTANTS

const char ClusterStateFSMMessageChoice::CLASS_NAME[] =
    "ClusterStateFSMMessageChoice";

const bdlat_SelectionInfo
    ClusterStateFSMMessageChoice::SELECTION_INFO_ARRAY[] = {
        {SELECTION_ID_FOLLOWER_L_S_N_REQUEST,
         "followerLSNRequest",
         sizeof("followerLSNRequest") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT},
        {SELECTION_ID_FOLLOWER_L_S_N_RESPONSE,
         "followerLSNResponse",
         sizeof("followerLSNResponse") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT},
        {SELECTION_ID_REGISTRATION_REQUEST,
         "registrationRequest",
         sizeof("registrationRequest") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT},
        {SELECTION_ID_REGISTRATION_RESPONSE,
         "registrationResponse",
         sizeof("registrationResponse") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT},
        {SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST,
         "followerClusterStateRequest",
         sizeof("followerClusterStateRequest") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT},
        {SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE,
         "followerClusterStateResponse",
         sizeof("followerClusterStateResponse") - 1,
         "",
         bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ClusterStateFSMMessageChoice::lookupSelectionInfo(const char* name,
                                                  int         nameLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ClusterStateFSMMessageChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo*
ClusterStateFSMMessageChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_FOLLOWER_L_S_N_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_FOLLOWER_L_S_N_REQUEST];
    case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_FOLLOWER_L_S_N_RESPONSE];
    case SELECTION_ID_REGISTRATION_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REGISTRATION_REQUEST];
    case SELECTION_ID_REGISTRATION_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REGISTRATION_RESPONSE];
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_FOLLOWER_CLUSTER_STATE_REQUEST];
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_FOLLOWER_CLUSTER_STATE_RESPONSE];
    default: return 0;
    }
}

// CREATORS

ClusterStateFSMMessageChoice::ClusterStateFSMMessageChoice(
    const ClusterStateFSMMessageChoice& original,
    bslma::Allocator*                   basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_FOLLOWER_L_S_N_REQUEST: {
        new (d_followerLSNRequest.buffer())
            FollowerLSNRequest(original.d_followerLSNRequest.object());
    } break;
    case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE: {
        new (d_followerLSNResponse.buffer())
            FollowerLSNResponse(original.d_followerLSNResponse.object());
    } break;
    case SELECTION_ID_REGISTRATION_REQUEST: {
        new (d_registrationRequest.buffer())
            RegistrationRequest(original.d_registrationRequest.object());
    } break;
    case SELECTION_ID_REGISTRATION_RESPONSE: {
        new (d_registrationResponse.buffer())
            RegistrationResponse(original.d_registrationResponse.object());
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST: {
        new (d_followerClusterStateRequest.buffer())
            FollowerClusterStateRequest(
                original.d_followerClusterStateRequest.object());
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE: {
        new (d_followerClusterStateResponse.buffer())
            FollowerClusterStateResponse(
                original.d_followerClusterStateResponse.object(),
                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStateFSMMessageChoice::ClusterStateFSMMessageChoice(
    ClusterStateFSMMessageChoice&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_FOLLOWER_L_S_N_REQUEST: {
        new (d_followerLSNRequest.buffer()) FollowerLSNRequest(
            bsl::move(original.d_followerLSNRequest.object()));
    } break;
    case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE: {
        new (d_followerLSNResponse.buffer()) FollowerLSNResponse(
            bsl::move(original.d_followerLSNResponse.object()));
    } break;
    case SELECTION_ID_REGISTRATION_REQUEST: {
        new (d_registrationRequest.buffer()) RegistrationRequest(
            bsl::move(original.d_registrationRequest.object()));
    } break;
    case SELECTION_ID_REGISTRATION_RESPONSE: {
        new (d_registrationResponse.buffer()) RegistrationResponse(
            bsl::move(original.d_registrationResponse.object()));
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST: {
        new (d_followerClusterStateRequest.buffer())
            FollowerClusterStateRequest(
                bsl::move(original.d_followerClusterStateRequest.object()));
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE: {
        new (d_followerClusterStateResponse.buffer())
            FollowerClusterStateResponse(
                bsl::move(original.d_followerClusterStateResponse.object()),
                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ClusterStateFSMMessageChoice::ClusterStateFSMMessageChoice(
    ClusterStateFSMMessageChoice&& original,
    bslma::Allocator*              basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_FOLLOWER_L_S_N_REQUEST: {
        new (d_followerLSNRequest.buffer()) FollowerLSNRequest(
            bsl::move(original.d_followerLSNRequest.object()));
    } break;
    case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE: {
        new (d_followerLSNResponse.buffer()) FollowerLSNResponse(
            bsl::move(original.d_followerLSNResponse.object()));
    } break;
    case SELECTION_ID_REGISTRATION_REQUEST: {
        new (d_registrationRequest.buffer()) RegistrationRequest(
            bsl::move(original.d_registrationRequest.object()));
    } break;
    case SELECTION_ID_REGISTRATION_RESPONSE: {
        new (d_registrationResponse.buffer()) RegistrationResponse(
            bsl::move(original.d_registrationResponse.object()));
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST: {
        new (d_followerClusterStateRequest.buffer())
            FollowerClusterStateRequest(
                bsl::move(original.d_followerClusterStateRequest.object()));
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE: {
        new (d_followerClusterStateResponse.buffer())
            FollowerClusterStateResponse(
                bsl::move(original.d_followerClusterStateResponse.object()),
                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ClusterStateFSMMessageChoice& ClusterStateFSMMessageChoice::operator=(
    const ClusterStateFSMMessageChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_FOLLOWER_L_S_N_REQUEST: {
            makeFollowerLSNRequest(rhs.d_followerLSNRequest.object());
        } break;
        case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE: {
            makeFollowerLSNResponse(rhs.d_followerLSNResponse.object());
        } break;
        case SELECTION_ID_REGISTRATION_REQUEST: {
            makeRegistrationRequest(rhs.d_registrationRequest.object());
        } break;
        case SELECTION_ID_REGISTRATION_RESPONSE: {
            makeRegistrationResponse(rhs.d_registrationResponse.object());
        } break;
        case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST: {
            makeFollowerClusterStateRequest(
                rhs.d_followerClusterStateRequest.object());
        } break;
        case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE: {
            makeFollowerClusterStateResponse(
                rhs.d_followerClusterStateResponse.object());
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
ClusterStateFSMMessageChoice&
ClusterStateFSMMessageChoice::operator=(ClusterStateFSMMessageChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_FOLLOWER_L_S_N_REQUEST: {
            makeFollowerLSNRequest(
                bsl::move(rhs.d_followerLSNRequest.object()));
        } break;
        case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE: {
            makeFollowerLSNResponse(
                bsl::move(rhs.d_followerLSNResponse.object()));
        } break;
        case SELECTION_ID_REGISTRATION_REQUEST: {
            makeRegistrationRequest(
                bsl::move(rhs.d_registrationRequest.object()));
        } break;
        case SELECTION_ID_REGISTRATION_RESPONSE: {
            makeRegistrationResponse(
                bsl::move(rhs.d_registrationResponse.object()));
        } break;
        case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST: {
            makeFollowerClusterStateRequest(
                bsl::move(rhs.d_followerClusterStateRequest.object()));
        } break;
        case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE: {
            makeFollowerClusterStateResponse(
                bsl::move(rhs.d_followerClusterStateResponse.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ClusterStateFSMMessageChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_FOLLOWER_L_S_N_REQUEST: {
        d_followerLSNRequest.object().~FollowerLSNRequest();
    } break;
    case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE: {
        d_followerLSNResponse.object().~FollowerLSNResponse();
    } break;
    case SELECTION_ID_REGISTRATION_REQUEST: {
        d_registrationRequest.object().~RegistrationRequest();
    } break;
    case SELECTION_ID_REGISTRATION_RESPONSE: {
        d_registrationResponse.object().~RegistrationResponse();
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST: {
        d_followerClusterStateRequest.object().~FollowerClusterStateRequest();
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE: {
        d_followerClusterStateResponse.object()
            .~FollowerClusterStateResponse();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ClusterStateFSMMessageChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_FOLLOWER_L_S_N_REQUEST: {
        makeFollowerLSNRequest();
    } break;
    case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE: {
        makeFollowerLSNResponse();
    } break;
    case SELECTION_ID_REGISTRATION_REQUEST: {
        makeRegistrationRequest();
    } break;
    case SELECTION_ID_REGISTRATION_RESPONSE: {
        makeRegistrationResponse();
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST: {
        makeFollowerClusterStateRequest();
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE: {
        makeFollowerClusterStateResponse();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ClusterStateFSMMessageChoice::makeSelection(const char* name,
                                                int         nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

FollowerLSNRequest& ClusterStateFSMMessageChoice::makeFollowerLSNRequest()
{
    if (SELECTION_ID_FOLLOWER_L_S_N_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_followerLSNRequest.object());
    }
    else {
        reset();
        new (d_followerLSNRequest.buffer()) FollowerLSNRequest();
        d_selectionId = SELECTION_ID_FOLLOWER_L_S_N_REQUEST;
    }

    return d_followerLSNRequest.object();
}

FollowerLSNRequest& ClusterStateFSMMessageChoice::makeFollowerLSNRequest(
    const FollowerLSNRequest& value)
{
    if (SELECTION_ID_FOLLOWER_L_S_N_REQUEST == d_selectionId) {
        d_followerLSNRequest.object() = value;
    }
    else {
        reset();
        new (d_followerLSNRequest.buffer()) FollowerLSNRequest(value);
        d_selectionId = SELECTION_ID_FOLLOWER_L_S_N_REQUEST;
    }

    return d_followerLSNRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FollowerLSNRequest& ClusterStateFSMMessageChoice::makeFollowerLSNRequest(
    FollowerLSNRequest&& value)
{
    if (SELECTION_ID_FOLLOWER_L_S_N_REQUEST == d_selectionId) {
        d_followerLSNRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_followerLSNRequest.buffer())
            FollowerLSNRequest(bsl::move(value));
        d_selectionId = SELECTION_ID_FOLLOWER_L_S_N_REQUEST;
    }

    return d_followerLSNRequest.object();
}
#endif

FollowerLSNResponse& ClusterStateFSMMessageChoice::makeFollowerLSNResponse()
{
    if (SELECTION_ID_FOLLOWER_L_S_N_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_followerLSNResponse.object());
    }
    else {
        reset();
        new (d_followerLSNResponse.buffer()) FollowerLSNResponse();
        d_selectionId = SELECTION_ID_FOLLOWER_L_S_N_RESPONSE;
    }

    return d_followerLSNResponse.object();
}

FollowerLSNResponse& ClusterStateFSMMessageChoice::makeFollowerLSNResponse(
    const FollowerLSNResponse& value)
{
    if (SELECTION_ID_FOLLOWER_L_S_N_RESPONSE == d_selectionId) {
        d_followerLSNResponse.object() = value;
    }
    else {
        reset();
        new (d_followerLSNResponse.buffer()) FollowerLSNResponse(value);
        d_selectionId = SELECTION_ID_FOLLOWER_L_S_N_RESPONSE;
    }

    return d_followerLSNResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FollowerLSNResponse& ClusterStateFSMMessageChoice::makeFollowerLSNResponse(
    FollowerLSNResponse&& value)
{
    if (SELECTION_ID_FOLLOWER_L_S_N_RESPONSE == d_selectionId) {
        d_followerLSNResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_followerLSNResponse.buffer())
            FollowerLSNResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_FOLLOWER_L_S_N_RESPONSE;
    }

    return d_followerLSNResponse.object();
}
#endif

RegistrationRequest& ClusterStateFSMMessageChoice::makeRegistrationRequest()
{
    if (SELECTION_ID_REGISTRATION_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_registrationRequest.object());
    }
    else {
        reset();
        new (d_registrationRequest.buffer()) RegistrationRequest();
        d_selectionId = SELECTION_ID_REGISTRATION_REQUEST;
    }

    return d_registrationRequest.object();
}

RegistrationRequest& ClusterStateFSMMessageChoice::makeRegistrationRequest(
    const RegistrationRequest& value)
{
    if (SELECTION_ID_REGISTRATION_REQUEST == d_selectionId) {
        d_registrationRequest.object() = value;
    }
    else {
        reset();
        new (d_registrationRequest.buffer()) RegistrationRequest(value);
        d_selectionId = SELECTION_ID_REGISTRATION_REQUEST;
    }

    return d_registrationRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RegistrationRequest& ClusterStateFSMMessageChoice::makeRegistrationRequest(
    RegistrationRequest&& value)
{
    if (SELECTION_ID_REGISTRATION_REQUEST == d_selectionId) {
        d_registrationRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_registrationRequest.buffer())
            RegistrationRequest(bsl::move(value));
        d_selectionId = SELECTION_ID_REGISTRATION_REQUEST;
    }

    return d_registrationRequest.object();
}
#endif

RegistrationResponse& ClusterStateFSMMessageChoice::makeRegistrationResponse()
{
    if (SELECTION_ID_REGISTRATION_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_registrationResponse.object());
    }
    else {
        reset();
        new (d_registrationResponse.buffer()) RegistrationResponse();
        d_selectionId = SELECTION_ID_REGISTRATION_RESPONSE;
    }

    return d_registrationResponse.object();
}

RegistrationResponse& ClusterStateFSMMessageChoice::makeRegistrationResponse(
    const RegistrationResponse& value)
{
    if (SELECTION_ID_REGISTRATION_RESPONSE == d_selectionId) {
        d_registrationResponse.object() = value;
    }
    else {
        reset();
        new (d_registrationResponse.buffer()) RegistrationResponse(value);
        d_selectionId = SELECTION_ID_REGISTRATION_RESPONSE;
    }

    return d_registrationResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RegistrationResponse& ClusterStateFSMMessageChoice::makeRegistrationResponse(
    RegistrationResponse&& value)
{
    if (SELECTION_ID_REGISTRATION_RESPONSE == d_selectionId) {
        d_registrationResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_registrationResponse.buffer())
            RegistrationResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_REGISTRATION_RESPONSE;
    }

    return d_registrationResponse.object();
}
#endif

FollowerClusterStateRequest&
ClusterStateFSMMessageChoice::makeFollowerClusterStateRequest()
{
    if (SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_followerClusterStateRequest.object());
    }
    else {
        reset();
        new (d_followerClusterStateRequest.buffer())
            FollowerClusterStateRequest();
        d_selectionId = SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST;
    }

    return d_followerClusterStateRequest.object();
}

FollowerClusterStateRequest&
ClusterStateFSMMessageChoice::makeFollowerClusterStateRequest(
    const FollowerClusterStateRequest& value)
{
    if (SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST == d_selectionId) {
        d_followerClusterStateRequest.object() = value;
    }
    else {
        reset();
        new (d_followerClusterStateRequest.buffer())
            FollowerClusterStateRequest(value);
        d_selectionId = SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST;
    }

    return d_followerClusterStateRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FollowerClusterStateRequest&
ClusterStateFSMMessageChoice::makeFollowerClusterStateRequest(
    FollowerClusterStateRequest&& value)
{
    if (SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST == d_selectionId) {
        d_followerClusterStateRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_followerClusterStateRequest.buffer())
            FollowerClusterStateRequest(bsl::move(value));
        d_selectionId = SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST;
    }

    return d_followerClusterStateRequest.object();
}
#endif

FollowerClusterStateResponse&
ClusterStateFSMMessageChoice::makeFollowerClusterStateResponse()
{
    if (SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_followerClusterStateResponse.object());
    }
    else {
        reset();
        new (d_followerClusterStateResponse.buffer())
            FollowerClusterStateResponse(d_allocator_p);
        d_selectionId = SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE;
    }

    return d_followerClusterStateResponse.object();
}

FollowerClusterStateResponse&
ClusterStateFSMMessageChoice::makeFollowerClusterStateResponse(
    const FollowerClusterStateResponse& value)
{
    if (SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE == d_selectionId) {
        d_followerClusterStateResponse.object() = value;
    }
    else {
        reset();
        new (d_followerClusterStateResponse.buffer())
            FollowerClusterStateResponse(value, d_allocator_p);
        d_selectionId = SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE;
    }

    return d_followerClusterStateResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FollowerClusterStateResponse&
ClusterStateFSMMessageChoice::makeFollowerClusterStateResponse(
    FollowerClusterStateResponse&& value)
{
    if (SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE == d_selectionId) {
        d_followerClusterStateResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_followerClusterStateResponse.buffer())
            FollowerClusterStateResponse(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE;
    }

    return d_followerClusterStateResponse.object();
}
#endif

// ACCESSORS

bsl::ostream& ClusterStateFSMMessageChoice::print(bsl::ostream& stream,
                                                  int           level,
                                                  int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_FOLLOWER_L_S_N_REQUEST: {
        printer.printAttribute("followerLSNRequest",
                               d_followerLSNRequest.object());
    } break;
    case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE: {
        printer.printAttribute("followerLSNResponse",
                               d_followerLSNResponse.object());
    } break;
    case SELECTION_ID_REGISTRATION_REQUEST: {
        printer.printAttribute("registrationRequest",
                               d_registrationRequest.object());
    } break;
    case SELECTION_ID_REGISTRATION_RESPONSE: {
        printer.printAttribute("registrationResponse",
                               d_registrationResponse.object());
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST: {
        printer.printAttribute("followerClusterStateRequest",
                               d_followerClusterStateRequest.object());
    } break;
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE: {
        printer.printAttribute("followerClusterStateResponse",
                               d_followerClusterStateResponse.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ClusterStateFSMMessageChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_FOLLOWER_L_S_N_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_FOLLOWER_L_S_N_REQUEST]
            .name();
    case SELECTION_ID_FOLLOWER_L_S_N_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_FOLLOWER_L_S_N_RESPONSE]
            .name();
    case SELECTION_ID_REGISTRATION_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REGISTRATION_REQUEST]
            .name();
    case SELECTION_ID_REGISTRATION_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REGISTRATION_RESPONSE]
            .name();
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_REQUEST:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_FOLLOWER_CLUSTER_STATE_REQUEST]
                .name();
    case SELECTION_ID_FOLLOWER_CLUSTER_STATE_RESPONSE:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_FOLLOWER_CLUSTER_STATE_RESPONSE]
                .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ---------------------
// class ConfigureStream
// ---------------------

// CONSTANTS

const char ConfigureStream::CLASS_NAME[] = "ConfigureStream";

const bdlat_AttributeInfo ConfigureStream::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_Q_ID,
     "qId",
     sizeof("qId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STREAM_PARAMETERS,
     "streamParameters",
     sizeof("streamParameters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ConfigureStream::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ConfigureStream::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ConfigureStream::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_Q_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_Q_ID];
    case ATTRIBUTE_ID_STREAM_PARAMETERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STREAM_PARAMETERS];
    default: return 0;
    }
}

// CREATORS

ConfigureStream::ConfigureStream(bslma::Allocator* basicAllocator)
: d_streamParameters(basicAllocator)
, d_qId()
{
}

ConfigureStream::ConfigureStream(const ConfigureStream& original,
                                 bslma::Allocator*      basicAllocator)
: d_streamParameters(original.d_streamParameters, basicAllocator)
, d_qId(original.d_qId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureStream::ConfigureStream(ConfigureStream&& original) noexcept
: d_streamParameters(bsl::move(original.d_streamParameters)),
  d_qId(bsl::move(original.d_qId))
{
}

ConfigureStream::ConfigureStream(ConfigureStream&& original,
                                 bslma::Allocator* basicAllocator)
: d_streamParameters(bsl::move(original.d_streamParameters), basicAllocator)
, d_qId(bsl::move(original.d_qId))
{
}
#endif

ConfigureStream::~ConfigureStream()
{
}

// MANIPULATORS

ConfigureStream& ConfigureStream::operator=(const ConfigureStream& rhs)
{
    if (this != &rhs) {
        d_qId              = rhs.d_qId;
        d_streamParameters = rhs.d_streamParameters;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureStream& ConfigureStream::operator=(ConfigureStream&& rhs)
{
    if (this != &rhs) {
        d_qId              = bsl::move(rhs.d_qId);
        d_streamParameters = bsl::move(rhs.d_streamParameters);
    }

    return *this;
}
#endif

void ConfigureStream::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_qId);
    bdlat_ValueTypeFunctions::reset(&d_streamParameters);
}

// ACCESSORS

bsl::ostream& ConfigureStream::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("qId", this->qId());
    printer.printAttribute("streamParameters", this->streamParameters());
    printer.end();
    return stream;
}

// ----------------------------
// class ClusterStateFSMMessage
// ----------------------------

// CONSTANTS

const char ClusterStateFSMMessage::CLASS_NAME[] = "ClusterStateFSMMessage";

const bdlat_AttributeInfo ClusterStateFSMMessage::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterStateFSMMessage::lookupAttributeInfo(const char* name, int nameLength)
{
    if (bdlb::String::areEqualCaseless("followerLSNRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("followerLSNResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("registrationRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("registrationResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("followerClusterStateRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("followerClusterStateResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterStateFSMMessage::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterStateFSMMessage::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

ClusterStateFSMMessage::ClusterStateFSMMessage(
    bslma::Allocator* basicAllocator)
: d_choice(basicAllocator)
{
}

ClusterStateFSMMessage::ClusterStateFSMMessage(
    const ClusterStateFSMMessage& original,
    bslma::Allocator*             basicAllocator)
: d_choice(original.d_choice, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStateFSMMessage::ClusterStateFSMMessage(
    ClusterStateFSMMessage&& original) noexcept
: d_choice(bsl::move(original.d_choice))
{
}

ClusterStateFSMMessage::ClusterStateFSMMessage(
    ClusterStateFSMMessage&& original,
    bslma::Allocator*        basicAllocator)
: d_choice(bsl::move(original.d_choice), basicAllocator)
{
}
#endif

ClusterStateFSMMessage::~ClusterStateFSMMessage()
{
}

// MANIPULATORS

ClusterStateFSMMessage&
ClusterStateFSMMessage::operator=(const ClusterStateFSMMessage& rhs)
{
    if (this != &rhs) {
        d_choice = rhs.d_choice;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStateFSMMessage&
ClusterStateFSMMessage::operator=(ClusterStateFSMMessage&& rhs)
{
    if (this != &rhs) {
        d_choice = bsl::move(rhs.d_choice);
    }

    return *this;
}
#endif

void ClusterStateFSMMessage::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream& ClusterStateFSMMessage::print(bsl::ostream& stream,
                                            int           level,
                                            int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// -----------------------------
// class ConfigureStreamResponse
// -----------------------------

// CONSTANTS

const char ConfigureStreamResponse::CLASS_NAME[] = "ConfigureStreamResponse";

const bdlat_AttributeInfo ConfigureStreamResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_REQUEST,
     "request",
     sizeof("request") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ConfigureStreamResponse::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ConfigureStreamResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ConfigureStreamResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_REQUEST:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REQUEST];
    default: return 0;
    }
}

// CREATORS

ConfigureStreamResponse::ConfigureStreamResponse(
    bslma::Allocator* basicAllocator)
: d_request(basicAllocator)
{
}

ConfigureStreamResponse::ConfigureStreamResponse(
    const ConfigureStreamResponse& original,
    bslma::Allocator*              basicAllocator)
: d_request(original.d_request, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureStreamResponse::ConfigureStreamResponse(
    ConfigureStreamResponse&& original) noexcept
: d_request(bsl::move(original.d_request))
{
}

ConfigureStreamResponse::ConfigureStreamResponse(
    ConfigureStreamResponse&& original,
    bslma::Allocator*         basicAllocator)
: d_request(bsl::move(original.d_request), basicAllocator)
{
}
#endif

ConfigureStreamResponse::~ConfigureStreamResponse()
{
}

// MANIPULATORS

ConfigureStreamResponse&
ConfigureStreamResponse::operator=(const ConfigureStreamResponse& rhs)
{
    if (this != &rhs) {
        d_request = rhs.d_request;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureStreamResponse&
ConfigureStreamResponse::operator=(ConfigureStreamResponse&& rhs)
{
    if (this != &rhs) {
        d_request = bsl::move(rhs.d_request);
    }

    return *this;
}
#endif

void ConfigureStreamResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_request);
}

// ACCESSORS

bsl::ostream& ConfigureStreamResponse::print(bsl::ostream& stream,
                                             int           level,
                                             int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("request", this->request());
    printer.end();
    return stream;
}

// --------------------------
// class ClusterMessageChoice
// --------------------------

// CONSTANTS

const char ClusterMessageChoice::CLASS_NAME[] = "ClusterMessageChoice";

const bdlat_SelectionInfo ClusterMessageChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_PARTITION_PRIMARY_ADVISORY,
     "partitionPrimaryAdvisory",
     sizeof("partitionPrimaryAdvisory") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LEADER_ADVISORY,
     "leaderAdvisory",
     sizeof("leaderAdvisory") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY,
     "queueAssignmentAdvisory",
     sizeof("queueAssignmentAdvisory") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_NODE_STATUS_ADVISORY,
     "nodeStatusAdvisory",
     sizeof("nodeStatusAdvisory") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LEADER_SYNC_STATE_QUERY,
     "leaderSyncStateQuery",
     sizeof("leaderSyncStateQuery") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE,
     "leaderSyncStateQueryResponse",
     sizeof("leaderSyncStateQueryResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LEADER_SYNC_DATA_QUERY,
     "leaderSyncDataQuery",
     sizeof("leaderSyncDataQuery") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE,
     "leaderSyncDataQueryResponse",
     sizeof("leaderSyncDataQueryResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST,
     "queueAssignmentRequest",
     sizeof("queueAssignmentRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STORAGE_SYNC_REQUEST,
     "storageSyncRequest",
     sizeof("storageSyncRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STORAGE_SYNC_RESPONSE,
     "storageSyncResponse",
     sizeof("storageSyncResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PARTITION_SYNC_STATE_QUERY,
     "partitionSyncStateQuery",
     sizeof("partitionSyncStateQuery") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE,
     "partitionSyncStateQueryResponse",
     sizeof("partitionSyncStateQueryResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PARTITION_SYNC_DATA_QUERY,
     "partitionSyncDataQuery",
     sizeof("partitionSyncDataQuery") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE,
     "partitionSyncDataQueryResponse",
     sizeof("partitionSyncDataQueryResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS,
     "partitionSyncDataQueryStatus",
     sizeof("partitionSyncDataQueryStatus") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PRIMARY_STATUS_ADVISORY,
     "primaryStatusAdvisory",
     sizeof("primaryStatusAdvisory") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_SYNC_REQUEST,
     "clusterSyncRequest",
     sizeof("clusterSyncRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_SYNC_RESPONSE,
     "clusterSyncResponse",
     sizeof("clusterSyncResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY,
     "queueUnAssignmentAdvisory",
     sizeof("queueUnAssignmentAdvisory") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY,
     "queueUnassignedAdvisory",
     sizeof("queueUnassignedAdvisory") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LEADER_ADVISORY_ACK,
     "leaderAdvisoryAck",
     sizeof("leaderAdvisoryAck") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LEADER_ADVISORY_COMMIT,
     "leaderAdvisoryCommit",
     sizeof("leaderAdvisoryCommit") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STATE_NOTIFICATION,
     "stateNotification",
     sizeof("stateNotification") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STOP_REQUEST,
     "stopRequest",
     sizeof("stopRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STOP_RESPONSE,
     "stopResponse",
     sizeof("stopResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST,
     "queueUnassignmentRequest",
     sizeof("queueUnassignmentRequest") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_UPDATE_ADVISORY,
     "queueUpdateAdvisory",
     sizeof("queueUpdateAdvisory") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE,
     "clusterStateFSMMessage",
     sizeof("clusterStateFSMMessage") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PARTITION_MESSAGE,
     "partitionMessage",
     sizeof("partitionMessage") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ClusterMessageChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 30; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ClusterMessageChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ClusterMessageChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_PARTITION_PRIMARY_ADVISORY:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_PARTITION_PRIMARY_ADVISORY];
    case SELECTION_ID_LEADER_ADVISORY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_ADVISORY];
    case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_QUEUE_ASSIGNMENT_ADVISORY];
    case SELECTION_ID_NODE_STATUS_ADVISORY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_NODE_STATUS_ADVISORY];
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_SYNC_STATE_QUERY];
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_LEADER_SYNC_STATE_QUERY_RESPONSE];
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_SYNC_DATA_QUERY];
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_LEADER_SYNC_DATA_QUERY_RESPONSE];
    case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_ASSIGNMENT_REQUEST];
    case SELECTION_ID_STORAGE_SYNC_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_SYNC_REQUEST];
    case SELECTION_ID_STORAGE_SYNC_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_SYNC_RESPONSE];
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_PARTITION_SYNC_STATE_QUERY];
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_PARTITION_SYNC_STATE_QUERY_RESPONSE];
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_PARTITION_SYNC_DATA_QUERY];
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_PARTITION_SYNC_DATA_QUERY_RESPONSE];
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_PARTITION_SYNC_DATA_QUERY_STATUS];
    case SELECTION_ID_PRIMARY_STATUS_ADVISORY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIMARY_STATUS_ADVISORY];
    case SELECTION_ID_CLUSTER_SYNC_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_SYNC_REQUEST];
    case SELECTION_ID_CLUSTER_SYNC_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_SYNC_RESPONSE];
    case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_QUEUE_UN_ASSIGNMENT_ADVISORY];
    case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_QUEUE_UNASSIGNED_ADVISORY];
    case SELECTION_ID_LEADER_ADVISORY_ACK:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_ADVISORY_ACK];
    case SELECTION_ID_LEADER_ADVISORY_COMMIT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_ADVISORY_COMMIT];
    case SELECTION_ID_STATE_NOTIFICATION:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STATE_NOTIFICATION];
    case SELECTION_ID_STOP_REQUEST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STOP_REQUEST];
    case SELECTION_ID_STOP_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STOP_RESPONSE];
    case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_QUEUE_UNASSIGNMENT_REQUEST];
    case SELECTION_ID_QUEUE_UPDATE_ADVISORY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_UPDATE_ADVISORY];
    case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_CLUSTER_STATE_F_S_M_MESSAGE];
    case SELECTION_ID_PARTITION_MESSAGE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PARTITION_MESSAGE];
    default: return 0;
    }
}

// CREATORS

ClusterMessageChoice::ClusterMessageChoice(
    const ClusterMessageChoice& original,
    bslma::Allocator*           basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        new (d_partitionPrimaryAdvisory.buffer()) PartitionPrimaryAdvisory(
            original.d_partitionPrimaryAdvisory.object(),
            d_allocator_p);
    } break;
    case SELECTION_ID_LEADER_ADVISORY: {
        new (d_leaderAdvisory.buffer())
            LeaderAdvisory(original.d_leaderAdvisory.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        new (d_queueAssignmentAdvisory.buffer()) QueueAssignmentAdvisory(
            original.d_queueAssignmentAdvisory.object(),
            d_allocator_p);
    } break;
    case SELECTION_ID_NODE_STATUS_ADVISORY: {
        new (d_nodeStatusAdvisory.buffer())
            NodeStatusAdvisory(original.d_nodeStatusAdvisory.object());
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY: {
        new (d_leaderSyncStateQuery.buffer())
            LeaderSyncStateQuery(original.d_leaderSyncStateQuery.object());
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE: {
        new (d_leaderSyncStateQueryResponse.buffer())
            LeaderSyncStateQueryResponse(
                original.d_leaderSyncStateQueryResponse.object());
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY: {
        new (d_leaderSyncDataQuery.buffer())
            LeaderSyncDataQuery(original.d_leaderSyncDataQuery.object());
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE: {
        new (d_leaderSyncDataQueryResponse.buffer())
            LeaderSyncDataQueryResponse(
                original.d_leaderSyncDataQueryResponse.object(),
                d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST: {
        new (d_queueAssignmentRequest.buffer())
            QueueAssignmentRequest(original.d_queueAssignmentRequest.object(),
                                   d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_SYNC_REQUEST: {
        new (d_storageSyncRequest.buffer())
            StorageSyncRequest(original.d_storageSyncRequest.object());
    } break;
    case SELECTION_ID_STORAGE_SYNC_RESPONSE: {
        new (d_storageSyncResponse.buffer())
            StorageSyncResponse(original.d_storageSyncResponse.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY: {
        new (d_partitionSyncStateQuery.buffer()) PartitionSyncStateQuery(
            original.d_partitionSyncStateQuery.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE: {
        new (d_partitionSyncStateQueryResponse.buffer())
            PartitionSyncStateQueryResponse(
                original.d_partitionSyncStateQueryResponse.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY: {
        new (d_partitionSyncDataQuery.buffer())
            PartitionSyncDataQuery(original.d_partitionSyncDataQuery.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE: {
        new (d_partitionSyncDataQueryResponse.buffer())
            PartitionSyncDataQueryResponse(
                original.d_partitionSyncDataQueryResponse.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS: {
        new (d_partitionSyncDataQueryStatus.buffer())
            PartitionSyncDataQueryStatus(
                original.d_partitionSyncDataQueryStatus.object(),
                d_allocator_p);
    } break;
    case SELECTION_ID_PRIMARY_STATUS_ADVISORY: {
        new (d_primaryStatusAdvisory.buffer())
            PrimaryStatusAdvisory(original.d_primaryStatusAdvisory.object());
    } break;
    case SELECTION_ID_CLUSTER_SYNC_REQUEST: {
        new (d_clusterSyncRequest.buffer())
            ClusterSyncRequest(original.d_clusterSyncRequest.object());
    } break;
    case SELECTION_ID_CLUSTER_SYNC_RESPONSE: {
        new (d_clusterSyncResponse.buffer())
            ClusterSyncResponse(original.d_clusterSyncResponse.object());
    } break;
    case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
        new (d_queueUnAssignmentAdvisory.buffer()) QueueUnAssignmentAdvisory(
            original.d_queueUnAssignmentAdvisory.object(),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        new (d_queueUnassignedAdvisory.buffer()) QueueUnassignedAdvisory(
            original.d_queueUnassignedAdvisory.object(),
            d_allocator_p);
    } break;
    case SELECTION_ID_LEADER_ADVISORY_ACK: {
        new (d_leaderAdvisoryAck.buffer())
            LeaderAdvisoryAck(original.d_leaderAdvisoryAck.object());
    } break;
    case SELECTION_ID_LEADER_ADVISORY_COMMIT: {
        new (d_leaderAdvisoryCommit.buffer())
            LeaderAdvisoryCommit(original.d_leaderAdvisoryCommit.object());
    } break;
    case SELECTION_ID_STATE_NOTIFICATION: {
        new (d_stateNotification.buffer())
            StateNotification(original.d_stateNotification.object());
    } break;
    case SELECTION_ID_STOP_REQUEST: {
        new (d_stopRequest.buffer())
            StopRequest(original.d_stopRequest.object(), d_allocator_p);
    } break;
    case SELECTION_ID_STOP_RESPONSE: {
        new (d_stopResponse.buffer())
            StopResponse(original.d_stopResponse.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST: {
        new (d_queueUnassignmentRequest.buffer()) QueueUnassignmentRequest(
            original.d_queueUnassignmentRequest.object(),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
        new (d_queueUpdateAdvisory.buffer())
            QueueUpdateAdvisory(original.d_queueUpdateAdvisory.object(),
                                d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE: {
        new (d_clusterStateFSMMessage.buffer())
            ClusterStateFSMMessage(original.d_clusterStateFSMMessage.object(),
                                   d_allocator_p);
    } break;
    case SELECTION_ID_PARTITION_MESSAGE: {
        new (d_partitionMessage.buffer())
            PartitionMessage(original.d_partitionMessage.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterMessageChoice::ClusterMessageChoice(ClusterMessageChoice&& original)
    noexcept : d_selectionId(original.d_selectionId),
               d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        new (d_partitionPrimaryAdvisory.buffer()) PartitionPrimaryAdvisory(
            bsl::move(original.d_partitionPrimaryAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_LEADER_ADVISORY: {
        new (d_leaderAdvisory.buffer())
            LeaderAdvisory(bsl::move(original.d_leaderAdvisory.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        new (d_queueAssignmentAdvisory.buffer()) QueueAssignmentAdvisory(
            bsl::move(original.d_queueAssignmentAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_NODE_STATUS_ADVISORY: {
        new (d_nodeStatusAdvisory.buffer()) NodeStatusAdvisory(
            bsl::move(original.d_nodeStatusAdvisory.object()));
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY: {
        new (d_leaderSyncStateQuery.buffer()) LeaderSyncStateQuery(
            bsl::move(original.d_leaderSyncStateQuery.object()));
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE: {
        new (d_leaderSyncStateQueryResponse.buffer())
            LeaderSyncStateQueryResponse(
                bsl::move(original.d_leaderSyncStateQueryResponse.object()));
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY: {
        new (d_leaderSyncDataQuery.buffer()) LeaderSyncDataQuery(
            bsl::move(original.d_leaderSyncDataQuery.object()));
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE: {
        new (d_leaderSyncDataQueryResponse.buffer())
            LeaderSyncDataQueryResponse(
                bsl::move(original.d_leaderSyncDataQueryResponse.object()),
                d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST: {
        new (d_queueAssignmentRequest.buffer()) QueueAssignmentRequest(
            bsl::move(original.d_queueAssignmentRequest.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_SYNC_REQUEST: {
        new (d_storageSyncRequest.buffer()) StorageSyncRequest(
            bsl::move(original.d_storageSyncRequest.object()));
    } break;
    case SELECTION_ID_STORAGE_SYNC_RESPONSE: {
        new (d_storageSyncResponse.buffer()) StorageSyncResponse(
            bsl::move(original.d_storageSyncResponse.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY: {
        new (d_partitionSyncStateQuery.buffer()) PartitionSyncStateQuery(
            bsl::move(original.d_partitionSyncStateQuery.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE: {
        new (d_partitionSyncStateQueryResponse.buffer())
            PartitionSyncStateQueryResponse(bsl::move(
                original.d_partitionSyncStateQueryResponse.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY: {
        new (d_partitionSyncDataQuery.buffer()) PartitionSyncDataQuery(
            bsl::move(original.d_partitionSyncDataQuery.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE: {
        new (d_partitionSyncDataQueryResponse.buffer())
            PartitionSyncDataQueryResponse(
                bsl::move(original.d_partitionSyncDataQueryResponse.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS: {
        new (d_partitionSyncDataQueryStatus.buffer())
            PartitionSyncDataQueryStatus(
                bsl::move(original.d_partitionSyncDataQueryStatus.object()),
                d_allocator_p);
    } break;
    case SELECTION_ID_PRIMARY_STATUS_ADVISORY: {
        new (d_primaryStatusAdvisory.buffer()) PrimaryStatusAdvisory(
            bsl::move(original.d_primaryStatusAdvisory.object()));
    } break;
    case SELECTION_ID_CLUSTER_SYNC_REQUEST: {
        new (d_clusterSyncRequest.buffer()) ClusterSyncRequest(
            bsl::move(original.d_clusterSyncRequest.object()));
    } break;
    case SELECTION_ID_CLUSTER_SYNC_RESPONSE: {
        new (d_clusterSyncResponse.buffer()) ClusterSyncResponse(
            bsl::move(original.d_clusterSyncResponse.object()));
    } break;
    case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
        new (d_queueUnAssignmentAdvisory.buffer()) QueueUnAssignmentAdvisory(
            bsl::move(original.d_queueUnAssignmentAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        new (d_queueUnassignedAdvisory.buffer()) QueueUnassignedAdvisory(
            bsl::move(original.d_queueUnassignedAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_LEADER_ADVISORY_ACK: {
        new (d_leaderAdvisoryAck.buffer()) LeaderAdvisoryAck(
            bsl::move(original.d_leaderAdvisoryAck.object()));
    } break;
    case SELECTION_ID_LEADER_ADVISORY_COMMIT: {
        new (d_leaderAdvisoryCommit.buffer()) LeaderAdvisoryCommit(
            bsl::move(original.d_leaderAdvisoryCommit.object()));
    } break;
    case SELECTION_ID_STATE_NOTIFICATION: {
        new (d_stateNotification.buffer()) StateNotification(
            bsl::move(original.d_stateNotification.object()));
    } break;
    case SELECTION_ID_STOP_REQUEST: {
        new (d_stopRequest.buffer())
            StopRequest(bsl::move(original.d_stopRequest.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_STOP_RESPONSE: {
        new (d_stopResponse.buffer())
            StopResponse(bsl::move(original.d_stopResponse.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST: {
        new (d_queueUnassignmentRequest.buffer()) QueueUnassignmentRequest(
            bsl::move(original.d_queueUnassignmentRequest.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
        new (d_queueUpdateAdvisory.buffer()) QueueUpdateAdvisory(
            bsl::move(original.d_queueUpdateAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE: {
        new (d_clusterStateFSMMessage.buffer()) ClusterStateFSMMessage(
            bsl::move(original.d_clusterStateFSMMessage.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_PARTITION_MESSAGE: {
        new (d_partitionMessage.buffer())
            PartitionMessage(bsl::move(original.d_partitionMessage.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ClusterMessageChoice::ClusterMessageChoice(ClusterMessageChoice&& original,
                                           bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        new (d_partitionPrimaryAdvisory.buffer()) PartitionPrimaryAdvisory(
            bsl::move(original.d_partitionPrimaryAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_LEADER_ADVISORY: {
        new (d_leaderAdvisory.buffer())
            LeaderAdvisory(bsl::move(original.d_leaderAdvisory.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        new (d_queueAssignmentAdvisory.buffer()) QueueAssignmentAdvisory(
            bsl::move(original.d_queueAssignmentAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_NODE_STATUS_ADVISORY: {
        new (d_nodeStatusAdvisory.buffer()) NodeStatusAdvisory(
            bsl::move(original.d_nodeStatusAdvisory.object()));
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY: {
        new (d_leaderSyncStateQuery.buffer()) LeaderSyncStateQuery(
            bsl::move(original.d_leaderSyncStateQuery.object()));
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE: {
        new (d_leaderSyncStateQueryResponse.buffer())
            LeaderSyncStateQueryResponse(
                bsl::move(original.d_leaderSyncStateQueryResponse.object()));
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY: {
        new (d_leaderSyncDataQuery.buffer()) LeaderSyncDataQuery(
            bsl::move(original.d_leaderSyncDataQuery.object()));
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE: {
        new (d_leaderSyncDataQueryResponse.buffer())
            LeaderSyncDataQueryResponse(
                bsl::move(original.d_leaderSyncDataQueryResponse.object()),
                d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST: {
        new (d_queueAssignmentRequest.buffer()) QueueAssignmentRequest(
            bsl::move(original.d_queueAssignmentRequest.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_SYNC_REQUEST: {
        new (d_storageSyncRequest.buffer()) StorageSyncRequest(
            bsl::move(original.d_storageSyncRequest.object()));
    } break;
    case SELECTION_ID_STORAGE_SYNC_RESPONSE: {
        new (d_storageSyncResponse.buffer()) StorageSyncResponse(
            bsl::move(original.d_storageSyncResponse.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY: {
        new (d_partitionSyncStateQuery.buffer()) PartitionSyncStateQuery(
            bsl::move(original.d_partitionSyncStateQuery.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE: {
        new (d_partitionSyncStateQueryResponse.buffer())
            PartitionSyncStateQueryResponse(bsl::move(
                original.d_partitionSyncStateQueryResponse.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY: {
        new (d_partitionSyncDataQuery.buffer()) PartitionSyncDataQuery(
            bsl::move(original.d_partitionSyncDataQuery.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE: {
        new (d_partitionSyncDataQueryResponse.buffer())
            PartitionSyncDataQueryResponse(
                bsl::move(original.d_partitionSyncDataQueryResponse.object()));
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS: {
        new (d_partitionSyncDataQueryStatus.buffer())
            PartitionSyncDataQueryStatus(
                bsl::move(original.d_partitionSyncDataQueryStatus.object()),
                d_allocator_p);
    } break;
    case SELECTION_ID_PRIMARY_STATUS_ADVISORY: {
        new (d_primaryStatusAdvisory.buffer()) PrimaryStatusAdvisory(
            bsl::move(original.d_primaryStatusAdvisory.object()));
    } break;
    case SELECTION_ID_CLUSTER_SYNC_REQUEST: {
        new (d_clusterSyncRequest.buffer()) ClusterSyncRequest(
            bsl::move(original.d_clusterSyncRequest.object()));
    } break;
    case SELECTION_ID_CLUSTER_SYNC_RESPONSE: {
        new (d_clusterSyncResponse.buffer()) ClusterSyncResponse(
            bsl::move(original.d_clusterSyncResponse.object()));
    } break;
    case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
        new (d_queueUnAssignmentAdvisory.buffer()) QueueUnAssignmentAdvisory(
            bsl::move(original.d_queueUnAssignmentAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        new (d_queueUnassignedAdvisory.buffer()) QueueUnassignedAdvisory(
            bsl::move(original.d_queueUnassignedAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_LEADER_ADVISORY_ACK: {
        new (d_leaderAdvisoryAck.buffer()) LeaderAdvisoryAck(
            bsl::move(original.d_leaderAdvisoryAck.object()));
    } break;
    case SELECTION_ID_LEADER_ADVISORY_COMMIT: {
        new (d_leaderAdvisoryCommit.buffer()) LeaderAdvisoryCommit(
            bsl::move(original.d_leaderAdvisoryCommit.object()));
    } break;
    case SELECTION_ID_STATE_NOTIFICATION: {
        new (d_stateNotification.buffer()) StateNotification(
            bsl::move(original.d_stateNotification.object()));
    } break;
    case SELECTION_ID_STOP_REQUEST: {
        new (d_stopRequest.buffer())
            StopRequest(bsl::move(original.d_stopRequest.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_STOP_RESPONSE: {
        new (d_stopResponse.buffer())
            StopResponse(bsl::move(original.d_stopResponse.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST: {
        new (d_queueUnassignmentRequest.buffer()) QueueUnassignmentRequest(
            bsl::move(original.d_queueUnassignmentRequest.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
        new (d_queueUpdateAdvisory.buffer()) QueueUpdateAdvisory(
            bsl::move(original.d_queueUpdateAdvisory.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE: {
        new (d_clusterStateFSMMessage.buffer()) ClusterStateFSMMessage(
            bsl::move(original.d_clusterStateFSMMessage.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_PARTITION_MESSAGE: {
        new (d_partitionMessage.buffer())
            PartitionMessage(bsl::move(original.d_partitionMessage.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ClusterMessageChoice&
ClusterMessageChoice::operator=(const ClusterMessageChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
            makePartitionPrimaryAdvisory(
                rhs.d_partitionPrimaryAdvisory.object());
        } break;
        case SELECTION_ID_LEADER_ADVISORY: {
            makeLeaderAdvisory(rhs.d_leaderAdvisory.object());
        } break;
        case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
            makeQueueAssignmentAdvisory(
                rhs.d_queueAssignmentAdvisory.object());
        } break;
        case SELECTION_ID_NODE_STATUS_ADVISORY: {
            makeNodeStatusAdvisory(rhs.d_nodeStatusAdvisory.object());
        } break;
        case SELECTION_ID_LEADER_SYNC_STATE_QUERY: {
            makeLeaderSyncStateQuery(rhs.d_leaderSyncStateQuery.object());
        } break;
        case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE: {
            makeLeaderSyncStateQueryResponse(
                rhs.d_leaderSyncStateQueryResponse.object());
        } break;
        case SELECTION_ID_LEADER_SYNC_DATA_QUERY: {
            makeLeaderSyncDataQuery(rhs.d_leaderSyncDataQuery.object());
        } break;
        case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE: {
            makeLeaderSyncDataQueryResponse(
                rhs.d_leaderSyncDataQueryResponse.object());
        } break;
        case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST: {
            makeQueueAssignmentRequest(rhs.d_queueAssignmentRequest.object());
        } break;
        case SELECTION_ID_STORAGE_SYNC_REQUEST: {
            makeStorageSyncRequest(rhs.d_storageSyncRequest.object());
        } break;
        case SELECTION_ID_STORAGE_SYNC_RESPONSE: {
            makeStorageSyncResponse(rhs.d_storageSyncResponse.object());
        } break;
        case SELECTION_ID_PARTITION_SYNC_STATE_QUERY: {
            makePartitionSyncStateQuery(
                rhs.d_partitionSyncStateQuery.object());
        } break;
        case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE: {
            makePartitionSyncStateQueryResponse(
                rhs.d_partitionSyncStateQueryResponse.object());
        } break;
        case SELECTION_ID_PARTITION_SYNC_DATA_QUERY: {
            makePartitionSyncDataQuery(rhs.d_partitionSyncDataQuery.object());
        } break;
        case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE: {
            makePartitionSyncDataQueryResponse(
                rhs.d_partitionSyncDataQueryResponse.object());
        } break;
        case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS: {
            makePartitionSyncDataQueryStatus(
                rhs.d_partitionSyncDataQueryStatus.object());
        } break;
        case SELECTION_ID_PRIMARY_STATUS_ADVISORY: {
            makePrimaryStatusAdvisory(rhs.d_primaryStatusAdvisory.object());
        } break;
        case SELECTION_ID_CLUSTER_SYNC_REQUEST: {
            makeClusterSyncRequest(rhs.d_clusterSyncRequest.object());
        } break;
        case SELECTION_ID_CLUSTER_SYNC_RESPONSE: {
            makeClusterSyncResponse(rhs.d_clusterSyncResponse.object());
        } break;
        case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
            makeQueueUnAssignmentAdvisory(
                rhs.d_queueUnAssignmentAdvisory.object());
        } break;
        case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
            makeQueueUnassignedAdvisory(
                rhs.d_queueUnassignedAdvisory.object());
        } break;
        case SELECTION_ID_LEADER_ADVISORY_ACK: {
            makeLeaderAdvisoryAck(rhs.d_leaderAdvisoryAck.object());
        } break;
        case SELECTION_ID_LEADER_ADVISORY_COMMIT: {
            makeLeaderAdvisoryCommit(rhs.d_leaderAdvisoryCommit.object());
        } break;
        case SELECTION_ID_STATE_NOTIFICATION: {
            makeStateNotification(rhs.d_stateNotification.object());
        } break;
        case SELECTION_ID_STOP_REQUEST: {
            makeStopRequest(rhs.d_stopRequest.object());
        } break;
        case SELECTION_ID_STOP_RESPONSE: {
            makeStopResponse(rhs.d_stopResponse.object());
        } break;
        case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST: {
            makeQueueUnassignmentRequest(
                rhs.d_queueUnassignmentRequest.object());
        } break;
        case SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
            makeQueueUpdateAdvisory(rhs.d_queueUpdateAdvisory.object());
        } break;
        case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE: {
            makeClusterStateFSMMessage(rhs.d_clusterStateFSMMessage.object());
        } break;
        case SELECTION_ID_PARTITION_MESSAGE: {
            makePartitionMessage(rhs.d_partitionMessage.object());
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
ClusterMessageChoice&
ClusterMessageChoice::operator=(ClusterMessageChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
            makePartitionPrimaryAdvisory(
                bsl::move(rhs.d_partitionPrimaryAdvisory.object()));
        } break;
        case SELECTION_ID_LEADER_ADVISORY: {
            makeLeaderAdvisory(bsl::move(rhs.d_leaderAdvisory.object()));
        } break;
        case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
            makeQueueAssignmentAdvisory(
                bsl::move(rhs.d_queueAssignmentAdvisory.object()));
        } break;
        case SELECTION_ID_NODE_STATUS_ADVISORY: {
            makeNodeStatusAdvisory(
                bsl::move(rhs.d_nodeStatusAdvisory.object()));
        } break;
        case SELECTION_ID_LEADER_SYNC_STATE_QUERY: {
            makeLeaderSyncStateQuery(
                bsl::move(rhs.d_leaderSyncStateQuery.object()));
        } break;
        case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE: {
            makeLeaderSyncStateQueryResponse(
                bsl::move(rhs.d_leaderSyncStateQueryResponse.object()));
        } break;
        case SELECTION_ID_LEADER_SYNC_DATA_QUERY: {
            makeLeaderSyncDataQuery(
                bsl::move(rhs.d_leaderSyncDataQuery.object()));
        } break;
        case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE: {
            makeLeaderSyncDataQueryResponse(
                bsl::move(rhs.d_leaderSyncDataQueryResponse.object()));
        } break;
        case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST: {
            makeQueueAssignmentRequest(
                bsl::move(rhs.d_queueAssignmentRequest.object()));
        } break;
        case SELECTION_ID_STORAGE_SYNC_REQUEST: {
            makeStorageSyncRequest(
                bsl::move(rhs.d_storageSyncRequest.object()));
        } break;
        case SELECTION_ID_STORAGE_SYNC_RESPONSE: {
            makeStorageSyncResponse(
                bsl::move(rhs.d_storageSyncResponse.object()));
        } break;
        case SELECTION_ID_PARTITION_SYNC_STATE_QUERY: {
            makePartitionSyncStateQuery(
                bsl::move(rhs.d_partitionSyncStateQuery.object()));
        } break;
        case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE: {
            makePartitionSyncStateQueryResponse(
                bsl::move(rhs.d_partitionSyncStateQueryResponse.object()));
        } break;
        case SELECTION_ID_PARTITION_SYNC_DATA_QUERY: {
            makePartitionSyncDataQuery(
                bsl::move(rhs.d_partitionSyncDataQuery.object()));
        } break;
        case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE: {
            makePartitionSyncDataQueryResponse(
                bsl::move(rhs.d_partitionSyncDataQueryResponse.object()));
        } break;
        case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS: {
            makePartitionSyncDataQueryStatus(
                bsl::move(rhs.d_partitionSyncDataQueryStatus.object()));
        } break;
        case SELECTION_ID_PRIMARY_STATUS_ADVISORY: {
            makePrimaryStatusAdvisory(
                bsl::move(rhs.d_primaryStatusAdvisory.object()));
        } break;
        case SELECTION_ID_CLUSTER_SYNC_REQUEST: {
            makeClusterSyncRequest(
                bsl::move(rhs.d_clusterSyncRequest.object()));
        } break;
        case SELECTION_ID_CLUSTER_SYNC_RESPONSE: {
            makeClusterSyncResponse(
                bsl::move(rhs.d_clusterSyncResponse.object()));
        } break;
        case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
            makeQueueUnAssignmentAdvisory(
                bsl::move(rhs.d_queueUnAssignmentAdvisory.object()));
        } break;
        case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
            makeQueueUnassignedAdvisory(
                bsl::move(rhs.d_queueUnassignedAdvisory.object()));
        } break;
        case SELECTION_ID_LEADER_ADVISORY_ACK: {
            makeLeaderAdvisoryAck(bsl::move(rhs.d_leaderAdvisoryAck.object()));
        } break;
        case SELECTION_ID_LEADER_ADVISORY_COMMIT: {
            makeLeaderAdvisoryCommit(
                bsl::move(rhs.d_leaderAdvisoryCommit.object()));
        } break;
        case SELECTION_ID_STATE_NOTIFICATION: {
            makeStateNotification(bsl::move(rhs.d_stateNotification.object()));
        } break;
        case SELECTION_ID_STOP_REQUEST: {
            makeStopRequest(bsl::move(rhs.d_stopRequest.object()));
        } break;
        case SELECTION_ID_STOP_RESPONSE: {
            makeStopResponse(bsl::move(rhs.d_stopResponse.object()));
        } break;
        case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST: {
            makeQueueUnassignmentRequest(
                bsl::move(rhs.d_queueUnassignmentRequest.object()));
        } break;
        case SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
            makeQueueUpdateAdvisory(
                bsl::move(rhs.d_queueUpdateAdvisory.object()));
        } break;
        case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE: {
            makeClusterStateFSMMessage(
                bsl::move(rhs.d_clusterStateFSMMessage.object()));
        } break;
        case SELECTION_ID_PARTITION_MESSAGE: {
            makePartitionMessage(bsl::move(rhs.d_partitionMessage.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ClusterMessageChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        d_partitionPrimaryAdvisory.object().~PartitionPrimaryAdvisory();
    } break;
    case SELECTION_ID_LEADER_ADVISORY: {
        d_leaderAdvisory.object().~LeaderAdvisory();
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        d_queueAssignmentAdvisory.object().~QueueAssignmentAdvisory();
    } break;
    case SELECTION_ID_NODE_STATUS_ADVISORY: {
        d_nodeStatusAdvisory.object().~NodeStatusAdvisory();
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY: {
        d_leaderSyncStateQuery.object().~LeaderSyncStateQuery();
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE: {
        d_leaderSyncStateQueryResponse.object()
            .~LeaderSyncStateQueryResponse();
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY: {
        d_leaderSyncDataQuery.object().~LeaderSyncDataQuery();
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE: {
        d_leaderSyncDataQueryResponse.object().~LeaderSyncDataQueryResponse();
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST: {
        d_queueAssignmentRequest.object().~QueueAssignmentRequest();
    } break;
    case SELECTION_ID_STORAGE_SYNC_REQUEST: {
        d_storageSyncRequest.object().~StorageSyncRequest();
    } break;
    case SELECTION_ID_STORAGE_SYNC_RESPONSE: {
        d_storageSyncResponse.object().~StorageSyncResponse();
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY: {
        d_partitionSyncStateQuery.object().~PartitionSyncStateQuery();
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE: {
        d_partitionSyncStateQueryResponse.object()
            .~PartitionSyncStateQueryResponse();
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY: {
        d_partitionSyncDataQuery.object().~PartitionSyncDataQuery();
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE: {
        d_partitionSyncDataQueryResponse.object()
            .~PartitionSyncDataQueryResponse();
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS: {
        d_partitionSyncDataQueryStatus.object()
            .~PartitionSyncDataQueryStatus();
    } break;
    case SELECTION_ID_PRIMARY_STATUS_ADVISORY: {
        d_primaryStatusAdvisory.object().~PrimaryStatusAdvisory();
    } break;
    case SELECTION_ID_CLUSTER_SYNC_REQUEST: {
        d_clusterSyncRequest.object().~ClusterSyncRequest();
    } break;
    case SELECTION_ID_CLUSTER_SYNC_RESPONSE: {
        d_clusterSyncResponse.object().~ClusterSyncResponse();
    } break;
    case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
        d_queueUnAssignmentAdvisory.object().~QueueUnAssignmentAdvisory();
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        d_queueUnassignedAdvisory.object().~QueueUnassignedAdvisory();
    } break;
    case SELECTION_ID_LEADER_ADVISORY_ACK: {
        d_leaderAdvisoryAck.object().~LeaderAdvisoryAck();
    } break;
    case SELECTION_ID_LEADER_ADVISORY_COMMIT: {
        d_leaderAdvisoryCommit.object().~LeaderAdvisoryCommit();
    } break;
    case SELECTION_ID_STATE_NOTIFICATION: {
        d_stateNotification.object().~StateNotification();
    } break;
    case SELECTION_ID_STOP_REQUEST: {
        d_stopRequest.object().~StopRequest();
    } break;
    case SELECTION_ID_STOP_RESPONSE: {
        d_stopResponse.object().~StopResponse();
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST: {
        d_queueUnassignmentRequest.object().~QueueUnassignmentRequest();
    } break;
    case SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
        d_queueUpdateAdvisory.object().~QueueUpdateAdvisory();
    } break;
    case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE: {
        d_clusterStateFSMMessage.object().~ClusterStateFSMMessage();
    } break;
    case SELECTION_ID_PARTITION_MESSAGE: {
        d_partitionMessage.object().~PartitionMessage();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ClusterMessageChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        makePartitionPrimaryAdvisory();
    } break;
    case SELECTION_ID_LEADER_ADVISORY: {
        makeLeaderAdvisory();
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        makeQueueAssignmentAdvisory();
    } break;
    case SELECTION_ID_NODE_STATUS_ADVISORY: {
        makeNodeStatusAdvisory();
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY: {
        makeLeaderSyncStateQuery();
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE: {
        makeLeaderSyncStateQueryResponse();
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY: {
        makeLeaderSyncDataQuery();
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE: {
        makeLeaderSyncDataQueryResponse();
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST: {
        makeQueueAssignmentRequest();
    } break;
    case SELECTION_ID_STORAGE_SYNC_REQUEST: {
        makeStorageSyncRequest();
    } break;
    case SELECTION_ID_STORAGE_SYNC_RESPONSE: {
        makeStorageSyncResponse();
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY: {
        makePartitionSyncStateQuery();
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE: {
        makePartitionSyncStateQueryResponse();
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY: {
        makePartitionSyncDataQuery();
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE: {
        makePartitionSyncDataQueryResponse();
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS: {
        makePartitionSyncDataQueryStatus();
    } break;
    case SELECTION_ID_PRIMARY_STATUS_ADVISORY: {
        makePrimaryStatusAdvisory();
    } break;
    case SELECTION_ID_CLUSTER_SYNC_REQUEST: {
        makeClusterSyncRequest();
    } break;
    case SELECTION_ID_CLUSTER_SYNC_RESPONSE: {
        makeClusterSyncResponse();
    } break;
    case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
        makeQueueUnAssignmentAdvisory();
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        makeQueueUnassignedAdvisory();
    } break;
    case SELECTION_ID_LEADER_ADVISORY_ACK: {
        makeLeaderAdvisoryAck();
    } break;
    case SELECTION_ID_LEADER_ADVISORY_COMMIT: {
        makeLeaderAdvisoryCommit();
    } break;
    case SELECTION_ID_STATE_NOTIFICATION: {
        makeStateNotification();
    } break;
    case SELECTION_ID_STOP_REQUEST: {
        makeStopRequest();
    } break;
    case SELECTION_ID_STOP_RESPONSE: {
        makeStopResponse();
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST: {
        makeQueueUnassignmentRequest();
    } break;
    case SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
        makeQueueUpdateAdvisory();
    } break;
    case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE: {
        makeClusterStateFSMMessage();
    } break;
    case SELECTION_ID_PARTITION_MESSAGE: {
        makePartitionMessage();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ClusterMessageChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

PartitionPrimaryAdvisory& ClusterMessageChoice::makePartitionPrimaryAdvisory()
{
    if (SELECTION_ID_PARTITION_PRIMARY_ADVISORY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_partitionPrimaryAdvisory.object());
    }
    else {
        reset();
        new (d_partitionPrimaryAdvisory.buffer())
            PartitionPrimaryAdvisory(d_allocator_p);
        d_selectionId = SELECTION_ID_PARTITION_PRIMARY_ADVISORY;
    }

    return d_partitionPrimaryAdvisory.object();
}

PartitionPrimaryAdvisory& ClusterMessageChoice::makePartitionPrimaryAdvisory(
    const PartitionPrimaryAdvisory& value)
{
    if (SELECTION_ID_PARTITION_PRIMARY_ADVISORY == d_selectionId) {
        d_partitionPrimaryAdvisory.object() = value;
    }
    else {
        reset();
        new (d_partitionPrimaryAdvisory.buffer())
            PartitionPrimaryAdvisory(value, d_allocator_p);
        d_selectionId = SELECTION_ID_PARTITION_PRIMARY_ADVISORY;
    }

    return d_partitionPrimaryAdvisory.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionPrimaryAdvisory& ClusterMessageChoice::makePartitionPrimaryAdvisory(
    PartitionPrimaryAdvisory&& value)
{
    if (SELECTION_ID_PARTITION_PRIMARY_ADVISORY == d_selectionId) {
        d_partitionPrimaryAdvisory.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_partitionPrimaryAdvisory.buffer())
            PartitionPrimaryAdvisory(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_PARTITION_PRIMARY_ADVISORY;
    }

    return d_partitionPrimaryAdvisory.object();
}
#endif

LeaderAdvisory& ClusterMessageChoice::makeLeaderAdvisory()
{
    if (SELECTION_ID_LEADER_ADVISORY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_leaderAdvisory.object());
    }
    else {
        reset();
        new (d_leaderAdvisory.buffer()) LeaderAdvisory(d_allocator_p);
        d_selectionId = SELECTION_ID_LEADER_ADVISORY;
    }

    return d_leaderAdvisory.object();
}

LeaderAdvisory&
ClusterMessageChoice::makeLeaderAdvisory(const LeaderAdvisory& value)
{
    if (SELECTION_ID_LEADER_ADVISORY == d_selectionId) {
        d_leaderAdvisory.object() = value;
    }
    else {
        reset();
        new (d_leaderAdvisory.buffer()) LeaderAdvisory(value, d_allocator_p);
        d_selectionId = SELECTION_ID_LEADER_ADVISORY;
    }

    return d_leaderAdvisory.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderAdvisory&
ClusterMessageChoice::makeLeaderAdvisory(LeaderAdvisory&& value)
{
    if (SELECTION_ID_LEADER_ADVISORY == d_selectionId) {
        d_leaderAdvisory.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leaderAdvisory.buffer())
            LeaderAdvisory(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_LEADER_ADVISORY;
    }

    return d_leaderAdvisory.object();
}
#endif

QueueAssignmentAdvisory& ClusterMessageChoice::makeQueueAssignmentAdvisory()
{
    if (SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueAssignmentAdvisory.object());
    }
    else {
        reset();
        new (d_queueAssignmentAdvisory.buffer())
            QueueAssignmentAdvisory(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY;
    }

    return d_queueAssignmentAdvisory.object();
}

QueueAssignmentAdvisory& ClusterMessageChoice::makeQueueAssignmentAdvisory(
    const QueueAssignmentAdvisory& value)
{
    if (SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY == d_selectionId) {
        d_queueAssignmentAdvisory.object() = value;
    }
    else {
        reset();
        new (d_queueAssignmentAdvisory.buffer())
            QueueAssignmentAdvisory(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY;
    }

    return d_queueAssignmentAdvisory.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueAssignmentAdvisory& ClusterMessageChoice::makeQueueAssignmentAdvisory(
    QueueAssignmentAdvisory&& value)
{
    if (SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY == d_selectionId) {
        d_queueAssignmentAdvisory.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueAssignmentAdvisory.buffer())
            QueueAssignmentAdvisory(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY;
    }

    return d_queueAssignmentAdvisory.object();
}
#endif

NodeStatusAdvisory& ClusterMessageChoice::makeNodeStatusAdvisory()
{
    if (SELECTION_ID_NODE_STATUS_ADVISORY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_nodeStatusAdvisory.object());
    }
    else {
        reset();
        new (d_nodeStatusAdvisory.buffer()) NodeStatusAdvisory();
        d_selectionId = SELECTION_ID_NODE_STATUS_ADVISORY;
    }

    return d_nodeStatusAdvisory.object();
}

NodeStatusAdvisory&
ClusterMessageChoice::makeNodeStatusAdvisory(const NodeStatusAdvisory& value)
{
    if (SELECTION_ID_NODE_STATUS_ADVISORY == d_selectionId) {
        d_nodeStatusAdvisory.object() = value;
    }
    else {
        reset();
        new (d_nodeStatusAdvisory.buffer()) NodeStatusAdvisory(value);
        d_selectionId = SELECTION_ID_NODE_STATUS_ADVISORY;
    }

    return d_nodeStatusAdvisory.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
NodeStatusAdvisory&
ClusterMessageChoice::makeNodeStatusAdvisory(NodeStatusAdvisory&& value)
{
    if (SELECTION_ID_NODE_STATUS_ADVISORY == d_selectionId) {
        d_nodeStatusAdvisory.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_nodeStatusAdvisory.buffer())
            NodeStatusAdvisory(bsl::move(value));
        d_selectionId = SELECTION_ID_NODE_STATUS_ADVISORY;
    }

    return d_nodeStatusAdvisory.object();
}
#endif

LeaderSyncStateQuery& ClusterMessageChoice::makeLeaderSyncStateQuery()
{
    if (SELECTION_ID_LEADER_SYNC_STATE_QUERY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_leaderSyncStateQuery.object());
    }
    else {
        reset();
        new (d_leaderSyncStateQuery.buffer()) LeaderSyncStateQuery();
        d_selectionId = SELECTION_ID_LEADER_SYNC_STATE_QUERY;
    }

    return d_leaderSyncStateQuery.object();
}

LeaderSyncStateQuery& ClusterMessageChoice::makeLeaderSyncStateQuery(
    const LeaderSyncStateQuery& value)
{
    if (SELECTION_ID_LEADER_SYNC_STATE_QUERY == d_selectionId) {
        d_leaderSyncStateQuery.object() = value;
    }
    else {
        reset();
        new (d_leaderSyncStateQuery.buffer()) LeaderSyncStateQuery(value);
        d_selectionId = SELECTION_ID_LEADER_SYNC_STATE_QUERY;
    }

    return d_leaderSyncStateQuery.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderSyncStateQuery&
ClusterMessageChoice::makeLeaderSyncStateQuery(LeaderSyncStateQuery&& value)
{
    if (SELECTION_ID_LEADER_SYNC_STATE_QUERY == d_selectionId) {
        d_leaderSyncStateQuery.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leaderSyncStateQuery.buffer())
            LeaderSyncStateQuery(bsl::move(value));
        d_selectionId = SELECTION_ID_LEADER_SYNC_STATE_QUERY;
    }

    return d_leaderSyncStateQuery.object();
}
#endif

LeaderSyncStateQueryResponse&
ClusterMessageChoice::makeLeaderSyncStateQueryResponse()
{
    if (SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_leaderSyncStateQueryResponse.object());
    }
    else {
        reset();
        new (d_leaderSyncStateQueryResponse.buffer())
            LeaderSyncStateQueryResponse();
        d_selectionId = SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE;
    }

    return d_leaderSyncStateQueryResponse.object();
}

LeaderSyncStateQueryResponse&
ClusterMessageChoice::makeLeaderSyncStateQueryResponse(
    const LeaderSyncStateQueryResponse& value)
{
    if (SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE == d_selectionId) {
        d_leaderSyncStateQueryResponse.object() = value;
    }
    else {
        reset();
        new (d_leaderSyncStateQueryResponse.buffer())
            LeaderSyncStateQueryResponse(value);
        d_selectionId = SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE;
    }

    return d_leaderSyncStateQueryResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderSyncStateQueryResponse&
ClusterMessageChoice::makeLeaderSyncStateQueryResponse(
    LeaderSyncStateQueryResponse&& value)
{
    if (SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE == d_selectionId) {
        d_leaderSyncStateQueryResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leaderSyncStateQueryResponse.buffer())
            LeaderSyncStateQueryResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE;
    }

    return d_leaderSyncStateQueryResponse.object();
}
#endif

LeaderSyncDataQuery& ClusterMessageChoice::makeLeaderSyncDataQuery()
{
    if (SELECTION_ID_LEADER_SYNC_DATA_QUERY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_leaderSyncDataQuery.object());
    }
    else {
        reset();
        new (d_leaderSyncDataQuery.buffer()) LeaderSyncDataQuery();
        d_selectionId = SELECTION_ID_LEADER_SYNC_DATA_QUERY;
    }

    return d_leaderSyncDataQuery.object();
}

LeaderSyncDataQuery&
ClusterMessageChoice::makeLeaderSyncDataQuery(const LeaderSyncDataQuery& value)
{
    if (SELECTION_ID_LEADER_SYNC_DATA_QUERY == d_selectionId) {
        d_leaderSyncDataQuery.object() = value;
    }
    else {
        reset();
        new (d_leaderSyncDataQuery.buffer()) LeaderSyncDataQuery(value);
        d_selectionId = SELECTION_ID_LEADER_SYNC_DATA_QUERY;
    }

    return d_leaderSyncDataQuery.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderSyncDataQuery&
ClusterMessageChoice::makeLeaderSyncDataQuery(LeaderSyncDataQuery&& value)
{
    if (SELECTION_ID_LEADER_SYNC_DATA_QUERY == d_selectionId) {
        d_leaderSyncDataQuery.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leaderSyncDataQuery.buffer())
            LeaderSyncDataQuery(bsl::move(value));
        d_selectionId = SELECTION_ID_LEADER_SYNC_DATA_QUERY;
    }

    return d_leaderSyncDataQuery.object();
}
#endif

LeaderSyncDataQueryResponse&
ClusterMessageChoice::makeLeaderSyncDataQueryResponse()
{
    if (SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_leaderSyncDataQueryResponse.object());
    }
    else {
        reset();
        new (d_leaderSyncDataQueryResponse.buffer())
            LeaderSyncDataQueryResponse(d_allocator_p);
        d_selectionId = SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE;
    }

    return d_leaderSyncDataQueryResponse.object();
}

LeaderSyncDataQueryResponse&
ClusterMessageChoice::makeLeaderSyncDataQueryResponse(
    const LeaderSyncDataQueryResponse& value)
{
    if (SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE == d_selectionId) {
        d_leaderSyncDataQueryResponse.object() = value;
    }
    else {
        reset();
        new (d_leaderSyncDataQueryResponse.buffer())
            LeaderSyncDataQueryResponse(value, d_allocator_p);
        d_selectionId = SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE;
    }

    return d_leaderSyncDataQueryResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderSyncDataQueryResponse&
ClusterMessageChoice::makeLeaderSyncDataQueryResponse(
    LeaderSyncDataQueryResponse&& value)
{
    if (SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE == d_selectionId) {
        d_leaderSyncDataQueryResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leaderSyncDataQueryResponse.buffer())
            LeaderSyncDataQueryResponse(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE;
    }

    return d_leaderSyncDataQueryResponse.object();
}
#endif

QueueAssignmentRequest& ClusterMessageChoice::makeQueueAssignmentRequest()
{
    if (SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueAssignmentRequest.object());
    }
    else {
        reset();
        new (d_queueAssignmentRequest.buffer())
            QueueAssignmentRequest(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST;
    }

    return d_queueAssignmentRequest.object();
}

QueueAssignmentRequest& ClusterMessageChoice::makeQueueAssignmentRequest(
    const QueueAssignmentRequest& value)
{
    if (SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST == d_selectionId) {
        d_queueAssignmentRequest.object() = value;
    }
    else {
        reset();
        new (d_queueAssignmentRequest.buffer())
            QueueAssignmentRequest(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST;
    }

    return d_queueAssignmentRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueAssignmentRequest& ClusterMessageChoice::makeQueueAssignmentRequest(
    QueueAssignmentRequest&& value)
{
    if (SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST == d_selectionId) {
        d_queueAssignmentRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueAssignmentRequest.buffer())
            QueueAssignmentRequest(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST;
    }

    return d_queueAssignmentRequest.object();
}
#endif

StorageSyncRequest& ClusterMessageChoice::makeStorageSyncRequest()
{
    if (SELECTION_ID_STORAGE_SYNC_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_storageSyncRequest.object());
    }
    else {
        reset();
        new (d_storageSyncRequest.buffer()) StorageSyncRequest();
        d_selectionId = SELECTION_ID_STORAGE_SYNC_REQUEST;
    }

    return d_storageSyncRequest.object();
}

StorageSyncRequest&
ClusterMessageChoice::makeStorageSyncRequest(const StorageSyncRequest& value)
{
    if (SELECTION_ID_STORAGE_SYNC_REQUEST == d_selectionId) {
        d_storageSyncRequest.object() = value;
    }
    else {
        reset();
        new (d_storageSyncRequest.buffer()) StorageSyncRequest(value);
        d_selectionId = SELECTION_ID_STORAGE_SYNC_REQUEST;
    }

    return d_storageSyncRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageSyncRequest&
ClusterMessageChoice::makeStorageSyncRequest(StorageSyncRequest&& value)
{
    if (SELECTION_ID_STORAGE_SYNC_REQUEST == d_selectionId) {
        d_storageSyncRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_storageSyncRequest.buffer())
            StorageSyncRequest(bsl::move(value));
        d_selectionId = SELECTION_ID_STORAGE_SYNC_REQUEST;
    }

    return d_storageSyncRequest.object();
}
#endif

StorageSyncResponse& ClusterMessageChoice::makeStorageSyncResponse()
{
    if (SELECTION_ID_STORAGE_SYNC_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_storageSyncResponse.object());
    }
    else {
        reset();
        new (d_storageSyncResponse.buffer()) StorageSyncResponse();
        d_selectionId = SELECTION_ID_STORAGE_SYNC_RESPONSE;
    }

    return d_storageSyncResponse.object();
}

StorageSyncResponse&
ClusterMessageChoice::makeStorageSyncResponse(const StorageSyncResponse& value)
{
    if (SELECTION_ID_STORAGE_SYNC_RESPONSE == d_selectionId) {
        d_storageSyncResponse.object() = value;
    }
    else {
        reset();
        new (d_storageSyncResponse.buffer()) StorageSyncResponse(value);
        d_selectionId = SELECTION_ID_STORAGE_SYNC_RESPONSE;
    }

    return d_storageSyncResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageSyncResponse&
ClusterMessageChoice::makeStorageSyncResponse(StorageSyncResponse&& value)
{
    if (SELECTION_ID_STORAGE_SYNC_RESPONSE == d_selectionId) {
        d_storageSyncResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_storageSyncResponse.buffer())
            StorageSyncResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_STORAGE_SYNC_RESPONSE;
    }

    return d_storageSyncResponse.object();
}
#endif

PartitionSyncStateQuery& ClusterMessageChoice::makePartitionSyncStateQuery()
{
    if (SELECTION_ID_PARTITION_SYNC_STATE_QUERY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_partitionSyncStateQuery.object());
    }
    else {
        reset();
        new (d_partitionSyncStateQuery.buffer()) PartitionSyncStateQuery();
        d_selectionId = SELECTION_ID_PARTITION_SYNC_STATE_QUERY;
    }

    return d_partitionSyncStateQuery.object();
}

PartitionSyncStateQuery& ClusterMessageChoice::makePartitionSyncStateQuery(
    const PartitionSyncStateQuery& value)
{
    if (SELECTION_ID_PARTITION_SYNC_STATE_QUERY == d_selectionId) {
        d_partitionSyncStateQuery.object() = value;
    }
    else {
        reset();
        new (d_partitionSyncStateQuery.buffer())
            PartitionSyncStateQuery(value);
        d_selectionId = SELECTION_ID_PARTITION_SYNC_STATE_QUERY;
    }

    return d_partitionSyncStateQuery.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionSyncStateQuery& ClusterMessageChoice::makePartitionSyncStateQuery(
    PartitionSyncStateQuery&& value)
{
    if (SELECTION_ID_PARTITION_SYNC_STATE_QUERY == d_selectionId) {
        d_partitionSyncStateQuery.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_partitionSyncStateQuery.buffer())
            PartitionSyncStateQuery(bsl::move(value));
        d_selectionId = SELECTION_ID_PARTITION_SYNC_STATE_QUERY;
    }

    return d_partitionSyncStateQuery.object();
}
#endif

PartitionSyncStateQueryResponse&
ClusterMessageChoice::makePartitionSyncStateQueryResponse()
{
    if (SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_partitionSyncStateQueryResponse.object());
    }
    else {
        reset();
        new (d_partitionSyncStateQueryResponse.buffer())
            PartitionSyncStateQueryResponse();
        d_selectionId = SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE;
    }

    return d_partitionSyncStateQueryResponse.object();
}

PartitionSyncStateQueryResponse&
ClusterMessageChoice::makePartitionSyncStateQueryResponse(
    const PartitionSyncStateQueryResponse& value)
{
    if (SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE == d_selectionId) {
        d_partitionSyncStateQueryResponse.object() = value;
    }
    else {
        reset();
        new (d_partitionSyncStateQueryResponse.buffer())
            PartitionSyncStateQueryResponse(value);
        d_selectionId = SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE;
    }

    return d_partitionSyncStateQueryResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionSyncStateQueryResponse&
ClusterMessageChoice::makePartitionSyncStateQueryResponse(
    PartitionSyncStateQueryResponse&& value)
{
    if (SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE == d_selectionId) {
        d_partitionSyncStateQueryResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_partitionSyncStateQueryResponse.buffer())
            PartitionSyncStateQueryResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE;
    }

    return d_partitionSyncStateQueryResponse.object();
}
#endif

PartitionSyncDataQuery& ClusterMessageChoice::makePartitionSyncDataQuery()
{
    if (SELECTION_ID_PARTITION_SYNC_DATA_QUERY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_partitionSyncDataQuery.object());
    }
    else {
        reset();
        new (d_partitionSyncDataQuery.buffer()) PartitionSyncDataQuery();
        d_selectionId = SELECTION_ID_PARTITION_SYNC_DATA_QUERY;
    }

    return d_partitionSyncDataQuery.object();
}

PartitionSyncDataQuery& ClusterMessageChoice::makePartitionSyncDataQuery(
    const PartitionSyncDataQuery& value)
{
    if (SELECTION_ID_PARTITION_SYNC_DATA_QUERY == d_selectionId) {
        d_partitionSyncDataQuery.object() = value;
    }
    else {
        reset();
        new (d_partitionSyncDataQuery.buffer()) PartitionSyncDataQuery(value);
        d_selectionId = SELECTION_ID_PARTITION_SYNC_DATA_QUERY;
    }

    return d_partitionSyncDataQuery.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionSyncDataQuery& ClusterMessageChoice::makePartitionSyncDataQuery(
    PartitionSyncDataQuery&& value)
{
    if (SELECTION_ID_PARTITION_SYNC_DATA_QUERY == d_selectionId) {
        d_partitionSyncDataQuery.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_partitionSyncDataQuery.buffer())
            PartitionSyncDataQuery(bsl::move(value));
        d_selectionId = SELECTION_ID_PARTITION_SYNC_DATA_QUERY;
    }

    return d_partitionSyncDataQuery.object();
}
#endif

PartitionSyncDataQueryResponse&
ClusterMessageChoice::makePartitionSyncDataQueryResponse()
{
    if (SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_partitionSyncDataQueryResponse.object());
    }
    else {
        reset();
        new (d_partitionSyncDataQueryResponse.buffer())
            PartitionSyncDataQueryResponse();
        d_selectionId = SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE;
    }

    return d_partitionSyncDataQueryResponse.object();
}

PartitionSyncDataQueryResponse&
ClusterMessageChoice::makePartitionSyncDataQueryResponse(
    const PartitionSyncDataQueryResponse& value)
{
    if (SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE == d_selectionId) {
        d_partitionSyncDataQueryResponse.object() = value;
    }
    else {
        reset();
        new (d_partitionSyncDataQueryResponse.buffer())
            PartitionSyncDataQueryResponse(value);
        d_selectionId = SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE;
    }

    return d_partitionSyncDataQueryResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionSyncDataQueryResponse&
ClusterMessageChoice::makePartitionSyncDataQueryResponse(
    PartitionSyncDataQueryResponse&& value)
{
    if (SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE == d_selectionId) {
        d_partitionSyncDataQueryResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_partitionSyncDataQueryResponse.buffer())
            PartitionSyncDataQueryResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE;
    }

    return d_partitionSyncDataQueryResponse.object();
}
#endif

PartitionSyncDataQueryStatus&
ClusterMessageChoice::makePartitionSyncDataQueryStatus()
{
    if (SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_partitionSyncDataQueryStatus.object());
    }
    else {
        reset();
        new (d_partitionSyncDataQueryStatus.buffer())
            PartitionSyncDataQueryStatus(d_allocator_p);
        d_selectionId = SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS;
    }

    return d_partitionSyncDataQueryStatus.object();
}

PartitionSyncDataQueryStatus&
ClusterMessageChoice::makePartitionSyncDataQueryStatus(
    const PartitionSyncDataQueryStatus& value)
{
    if (SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS == d_selectionId) {
        d_partitionSyncDataQueryStatus.object() = value;
    }
    else {
        reset();
        new (d_partitionSyncDataQueryStatus.buffer())
            PartitionSyncDataQueryStatus(value, d_allocator_p);
        d_selectionId = SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS;
    }

    return d_partitionSyncDataQueryStatus.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionSyncDataQueryStatus&
ClusterMessageChoice::makePartitionSyncDataQueryStatus(
    PartitionSyncDataQueryStatus&& value)
{
    if (SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS == d_selectionId) {
        d_partitionSyncDataQueryStatus.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_partitionSyncDataQueryStatus.buffer())
            PartitionSyncDataQueryStatus(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS;
    }

    return d_partitionSyncDataQueryStatus.object();
}
#endif

PrimaryStatusAdvisory& ClusterMessageChoice::makePrimaryStatusAdvisory()
{
    if (SELECTION_ID_PRIMARY_STATUS_ADVISORY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_primaryStatusAdvisory.object());
    }
    else {
        reset();
        new (d_primaryStatusAdvisory.buffer()) PrimaryStatusAdvisory();
        d_selectionId = SELECTION_ID_PRIMARY_STATUS_ADVISORY;
    }

    return d_primaryStatusAdvisory.object();
}

PrimaryStatusAdvisory& ClusterMessageChoice::makePrimaryStatusAdvisory(
    const PrimaryStatusAdvisory& value)
{
    if (SELECTION_ID_PRIMARY_STATUS_ADVISORY == d_selectionId) {
        d_primaryStatusAdvisory.object() = value;
    }
    else {
        reset();
        new (d_primaryStatusAdvisory.buffer()) PrimaryStatusAdvisory(value);
        d_selectionId = SELECTION_ID_PRIMARY_STATUS_ADVISORY;
    }

    return d_primaryStatusAdvisory.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PrimaryStatusAdvisory&
ClusterMessageChoice::makePrimaryStatusAdvisory(PrimaryStatusAdvisory&& value)
{
    if (SELECTION_ID_PRIMARY_STATUS_ADVISORY == d_selectionId) {
        d_primaryStatusAdvisory.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_primaryStatusAdvisory.buffer())
            PrimaryStatusAdvisory(bsl::move(value));
        d_selectionId = SELECTION_ID_PRIMARY_STATUS_ADVISORY;
    }

    return d_primaryStatusAdvisory.object();
}
#endif

ClusterSyncRequest& ClusterMessageChoice::makeClusterSyncRequest()
{
    if (SELECTION_ID_CLUSTER_SYNC_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterSyncRequest.object());
    }
    else {
        reset();
        new (d_clusterSyncRequest.buffer()) ClusterSyncRequest();
        d_selectionId = SELECTION_ID_CLUSTER_SYNC_REQUEST;
    }

    return d_clusterSyncRequest.object();
}

ClusterSyncRequest&
ClusterMessageChoice::makeClusterSyncRequest(const ClusterSyncRequest& value)
{
    if (SELECTION_ID_CLUSTER_SYNC_REQUEST == d_selectionId) {
        d_clusterSyncRequest.object() = value;
    }
    else {
        reset();
        new (d_clusterSyncRequest.buffer()) ClusterSyncRequest(value);
        d_selectionId = SELECTION_ID_CLUSTER_SYNC_REQUEST;
    }

    return d_clusterSyncRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterSyncRequest&
ClusterMessageChoice::makeClusterSyncRequest(ClusterSyncRequest&& value)
{
    if (SELECTION_ID_CLUSTER_SYNC_REQUEST == d_selectionId) {
        d_clusterSyncRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterSyncRequest.buffer())
            ClusterSyncRequest(bsl::move(value));
        d_selectionId = SELECTION_ID_CLUSTER_SYNC_REQUEST;
    }

    return d_clusterSyncRequest.object();
}
#endif

ClusterSyncResponse& ClusterMessageChoice::makeClusterSyncResponse()
{
    if (SELECTION_ID_CLUSTER_SYNC_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterSyncResponse.object());
    }
    else {
        reset();
        new (d_clusterSyncResponse.buffer()) ClusterSyncResponse();
        d_selectionId = SELECTION_ID_CLUSTER_SYNC_RESPONSE;
    }

    return d_clusterSyncResponse.object();
}

ClusterSyncResponse&
ClusterMessageChoice::makeClusterSyncResponse(const ClusterSyncResponse& value)
{
    if (SELECTION_ID_CLUSTER_SYNC_RESPONSE == d_selectionId) {
        d_clusterSyncResponse.object() = value;
    }
    else {
        reset();
        new (d_clusterSyncResponse.buffer()) ClusterSyncResponse(value);
        d_selectionId = SELECTION_ID_CLUSTER_SYNC_RESPONSE;
    }

    return d_clusterSyncResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterSyncResponse&
ClusterMessageChoice::makeClusterSyncResponse(ClusterSyncResponse&& value)
{
    if (SELECTION_ID_CLUSTER_SYNC_RESPONSE == d_selectionId) {
        d_clusterSyncResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterSyncResponse.buffer())
            ClusterSyncResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_CLUSTER_SYNC_RESPONSE;
    }

    return d_clusterSyncResponse.object();
}
#endif

QueueUnAssignmentAdvisory&
ClusterMessageChoice::makeQueueUnAssignmentAdvisory()
{
    if (SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueUnAssignmentAdvisory.object());
    }
    else {
        reset();
        new (d_queueUnAssignmentAdvisory.buffer())
            QueueUnAssignmentAdvisory(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY;
    }

    return d_queueUnAssignmentAdvisory.object();
}

QueueUnAssignmentAdvisory& ClusterMessageChoice::makeQueueUnAssignmentAdvisory(
    const QueueUnAssignmentAdvisory& value)
{
    if (SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY == d_selectionId) {
        d_queueUnAssignmentAdvisory.object() = value;
    }
    else {
        reset();
        new (d_queueUnAssignmentAdvisory.buffer())
            QueueUnAssignmentAdvisory(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY;
    }

    return d_queueUnAssignmentAdvisory.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUnAssignmentAdvisory& ClusterMessageChoice::makeQueueUnAssignmentAdvisory(
    QueueUnAssignmentAdvisory&& value)
{
    if (SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY == d_selectionId) {
        d_queueUnAssignmentAdvisory.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueUnAssignmentAdvisory.buffer())
            QueueUnAssignmentAdvisory(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY;
    }

    return d_queueUnAssignmentAdvisory.object();
}
#endif

QueueUnassignedAdvisory& ClusterMessageChoice::makeQueueUnassignedAdvisory()
{
    if (SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueUnassignedAdvisory.object());
    }
    else {
        reset();
        new (d_queueUnassignedAdvisory.buffer())
            QueueUnassignedAdvisory(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY;
    }

    return d_queueUnassignedAdvisory.object();
}

QueueUnassignedAdvisory& ClusterMessageChoice::makeQueueUnassignedAdvisory(
    const QueueUnassignedAdvisory& value)
{
    if (SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY == d_selectionId) {
        d_queueUnassignedAdvisory.object() = value;
    }
    else {
        reset();
        new (d_queueUnassignedAdvisory.buffer())
            QueueUnassignedAdvisory(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY;
    }

    return d_queueUnassignedAdvisory.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUnassignedAdvisory& ClusterMessageChoice::makeQueueUnassignedAdvisory(
    QueueUnassignedAdvisory&& value)
{
    if (SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY == d_selectionId) {
        d_queueUnassignedAdvisory.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueUnassignedAdvisory.buffer())
            QueueUnassignedAdvisory(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY;
    }

    return d_queueUnassignedAdvisory.object();
}
#endif

LeaderAdvisoryAck& ClusterMessageChoice::makeLeaderAdvisoryAck()
{
    if (SELECTION_ID_LEADER_ADVISORY_ACK == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_leaderAdvisoryAck.object());
    }
    else {
        reset();
        new (d_leaderAdvisoryAck.buffer()) LeaderAdvisoryAck();
        d_selectionId = SELECTION_ID_LEADER_ADVISORY_ACK;
    }

    return d_leaderAdvisoryAck.object();
}

LeaderAdvisoryAck&
ClusterMessageChoice::makeLeaderAdvisoryAck(const LeaderAdvisoryAck& value)
{
    if (SELECTION_ID_LEADER_ADVISORY_ACK == d_selectionId) {
        d_leaderAdvisoryAck.object() = value;
    }
    else {
        reset();
        new (d_leaderAdvisoryAck.buffer()) LeaderAdvisoryAck(value);
        d_selectionId = SELECTION_ID_LEADER_ADVISORY_ACK;
    }

    return d_leaderAdvisoryAck.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderAdvisoryAck&
ClusterMessageChoice::makeLeaderAdvisoryAck(LeaderAdvisoryAck&& value)
{
    if (SELECTION_ID_LEADER_ADVISORY_ACK == d_selectionId) {
        d_leaderAdvisoryAck.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leaderAdvisoryAck.buffer()) LeaderAdvisoryAck(bsl::move(value));
        d_selectionId = SELECTION_ID_LEADER_ADVISORY_ACK;
    }

    return d_leaderAdvisoryAck.object();
}
#endif

LeaderAdvisoryCommit& ClusterMessageChoice::makeLeaderAdvisoryCommit()
{
    if (SELECTION_ID_LEADER_ADVISORY_COMMIT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_leaderAdvisoryCommit.object());
    }
    else {
        reset();
        new (d_leaderAdvisoryCommit.buffer()) LeaderAdvisoryCommit();
        d_selectionId = SELECTION_ID_LEADER_ADVISORY_COMMIT;
    }

    return d_leaderAdvisoryCommit.object();
}

LeaderAdvisoryCommit& ClusterMessageChoice::makeLeaderAdvisoryCommit(
    const LeaderAdvisoryCommit& value)
{
    if (SELECTION_ID_LEADER_ADVISORY_COMMIT == d_selectionId) {
        d_leaderAdvisoryCommit.object() = value;
    }
    else {
        reset();
        new (d_leaderAdvisoryCommit.buffer()) LeaderAdvisoryCommit(value);
        d_selectionId = SELECTION_ID_LEADER_ADVISORY_COMMIT;
    }

    return d_leaderAdvisoryCommit.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeaderAdvisoryCommit&
ClusterMessageChoice::makeLeaderAdvisoryCommit(LeaderAdvisoryCommit&& value)
{
    if (SELECTION_ID_LEADER_ADVISORY_COMMIT == d_selectionId) {
        d_leaderAdvisoryCommit.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_leaderAdvisoryCommit.buffer())
            LeaderAdvisoryCommit(bsl::move(value));
        d_selectionId = SELECTION_ID_LEADER_ADVISORY_COMMIT;
    }

    return d_leaderAdvisoryCommit.object();
}
#endif

StateNotification& ClusterMessageChoice::makeStateNotification()
{
    if (SELECTION_ID_STATE_NOTIFICATION == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_stateNotification.object());
    }
    else {
        reset();
        new (d_stateNotification.buffer()) StateNotification();
        d_selectionId = SELECTION_ID_STATE_NOTIFICATION;
    }

    return d_stateNotification.object();
}

StateNotification&
ClusterMessageChoice::makeStateNotification(const StateNotification& value)
{
    if (SELECTION_ID_STATE_NOTIFICATION == d_selectionId) {
        d_stateNotification.object() = value;
    }
    else {
        reset();
        new (d_stateNotification.buffer()) StateNotification(value);
        d_selectionId = SELECTION_ID_STATE_NOTIFICATION;
    }

    return d_stateNotification.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StateNotification&
ClusterMessageChoice::makeStateNotification(StateNotification&& value)
{
    if (SELECTION_ID_STATE_NOTIFICATION == d_selectionId) {
        d_stateNotification.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_stateNotification.buffer()) StateNotification(bsl::move(value));
        d_selectionId = SELECTION_ID_STATE_NOTIFICATION;
    }

    return d_stateNotification.object();
}
#endif

StopRequest& ClusterMessageChoice::makeStopRequest()
{
    if (SELECTION_ID_STOP_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_stopRequest.object());
    }
    else {
        reset();
        new (d_stopRequest.buffer()) StopRequest(d_allocator_p);
        d_selectionId = SELECTION_ID_STOP_REQUEST;
    }

    return d_stopRequest.object();
}

StopRequest& ClusterMessageChoice::makeStopRequest(const StopRequest& value)
{
    if (SELECTION_ID_STOP_REQUEST == d_selectionId) {
        d_stopRequest.object() = value;
    }
    else {
        reset();
        new (d_stopRequest.buffer()) StopRequest(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STOP_REQUEST;
    }

    return d_stopRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StopRequest& ClusterMessageChoice::makeStopRequest(StopRequest&& value)
{
    if (SELECTION_ID_STOP_REQUEST == d_selectionId) {
        d_stopRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_stopRequest.buffer())
            StopRequest(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STOP_REQUEST;
    }

    return d_stopRequest.object();
}
#endif

StopResponse& ClusterMessageChoice::makeStopResponse()
{
    if (SELECTION_ID_STOP_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_stopResponse.object());
    }
    else {
        reset();
        new (d_stopResponse.buffer()) StopResponse(d_allocator_p);
        d_selectionId = SELECTION_ID_STOP_RESPONSE;
    }

    return d_stopResponse.object();
}

StopResponse& ClusterMessageChoice::makeStopResponse(const StopResponse& value)
{
    if (SELECTION_ID_STOP_RESPONSE == d_selectionId) {
        d_stopResponse.object() = value;
    }
    else {
        reset();
        new (d_stopResponse.buffer()) StopResponse(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STOP_RESPONSE;
    }

    return d_stopResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StopResponse& ClusterMessageChoice::makeStopResponse(StopResponse&& value)
{
    if (SELECTION_ID_STOP_RESPONSE == d_selectionId) {
        d_stopResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_stopResponse.buffer())
            StopResponse(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STOP_RESPONSE;
    }

    return d_stopResponse.object();
}
#endif

QueueUnassignmentRequest& ClusterMessageChoice::makeQueueUnassignmentRequest()
{
    if (SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueUnassignmentRequest.object());
    }
    else {
        reset();
        new (d_queueUnassignmentRequest.buffer())
            QueueUnassignmentRequest(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST;
    }

    return d_queueUnassignmentRequest.object();
}

QueueUnassignmentRequest& ClusterMessageChoice::makeQueueUnassignmentRequest(
    const QueueUnassignmentRequest& value)
{
    if (SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST == d_selectionId) {
        d_queueUnassignmentRequest.object() = value;
    }
    else {
        reset();
        new (d_queueUnassignmentRequest.buffer())
            QueueUnassignmentRequest(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST;
    }

    return d_queueUnassignmentRequest.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUnassignmentRequest& ClusterMessageChoice::makeQueueUnassignmentRequest(
    QueueUnassignmentRequest&& value)
{
    if (SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST == d_selectionId) {
        d_queueUnassignmentRequest.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueUnassignmentRequest.buffer())
            QueueUnassignmentRequest(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST;
    }

    return d_queueUnassignmentRequest.object();
}
#endif

QueueUpdateAdvisory& ClusterMessageChoice::makeQueueUpdateAdvisory()
{
    if (SELECTION_ID_QUEUE_UPDATE_ADVISORY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueUpdateAdvisory.object());
    }
    else {
        reset();
        new (d_queueUpdateAdvisory.buffer())
            QueueUpdateAdvisory(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UPDATE_ADVISORY;
    }

    return d_queueUpdateAdvisory.object();
}

QueueUpdateAdvisory&
ClusterMessageChoice::makeQueueUpdateAdvisory(const QueueUpdateAdvisory& value)
{
    if (SELECTION_ID_QUEUE_UPDATE_ADVISORY == d_selectionId) {
        d_queueUpdateAdvisory.object() = value;
    }
    else {
        reset();
        new (d_queueUpdateAdvisory.buffer())
            QueueUpdateAdvisory(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UPDATE_ADVISORY;
    }

    return d_queueUpdateAdvisory.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueUpdateAdvisory&
ClusterMessageChoice::makeQueueUpdateAdvisory(QueueUpdateAdvisory&& value)
{
    if (SELECTION_ID_QUEUE_UPDATE_ADVISORY == d_selectionId) {
        d_queueUpdateAdvisory.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueUpdateAdvisory.buffer())
            QueueUpdateAdvisory(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_UPDATE_ADVISORY;
    }

    return d_queueUpdateAdvisory.object();
}
#endif

ClusterStateFSMMessage& ClusterMessageChoice::makeClusterStateFSMMessage()
{
    if (SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterStateFSMMessage.object());
    }
    else {
        reset();
        new (d_clusterStateFSMMessage.buffer())
            ClusterStateFSMMessage(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE;
    }

    return d_clusterStateFSMMessage.object();
}

ClusterStateFSMMessage& ClusterMessageChoice::makeClusterStateFSMMessage(
    const ClusterStateFSMMessage& value)
{
    if (SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE == d_selectionId) {
        d_clusterStateFSMMessage.object() = value;
    }
    else {
        reset();
        new (d_clusterStateFSMMessage.buffer())
            ClusterStateFSMMessage(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE;
    }

    return d_clusterStateFSMMessage.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStateFSMMessage& ClusterMessageChoice::makeClusterStateFSMMessage(
    ClusterStateFSMMessage&& value)
{
    if (SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE == d_selectionId) {
        d_clusterStateFSMMessage.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterStateFSMMessage.buffer())
            ClusterStateFSMMessage(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE;
    }

    return d_clusterStateFSMMessage.object();
}
#endif

PartitionMessage& ClusterMessageChoice::makePartitionMessage()
{
    if (SELECTION_ID_PARTITION_MESSAGE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_partitionMessage.object());
    }
    else {
        reset();
        new (d_partitionMessage.buffer()) PartitionMessage();
        d_selectionId = SELECTION_ID_PARTITION_MESSAGE;
    }

    return d_partitionMessage.object();
}

PartitionMessage&
ClusterMessageChoice::makePartitionMessage(const PartitionMessage& value)
{
    if (SELECTION_ID_PARTITION_MESSAGE == d_selectionId) {
        d_partitionMessage.object() = value;
    }
    else {
        reset();
        new (d_partitionMessage.buffer()) PartitionMessage(value);
        d_selectionId = SELECTION_ID_PARTITION_MESSAGE;
    }

    return d_partitionMessage.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionMessage&
ClusterMessageChoice::makePartitionMessage(PartitionMessage&& value)
{
    if (SELECTION_ID_PARTITION_MESSAGE == d_selectionId) {
        d_partitionMessage.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_partitionMessage.buffer()) PartitionMessage(bsl::move(value));
        d_selectionId = SELECTION_ID_PARTITION_MESSAGE;
    }

    return d_partitionMessage.object();
}
#endif

// ACCESSORS

bsl::ostream& ClusterMessageChoice::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        printer.printAttribute("partitionPrimaryAdvisory",
                               d_partitionPrimaryAdvisory.object());
    } break;
    case SELECTION_ID_LEADER_ADVISORY: {
        printer.printAttribute("leaderAdvisory", d_leaderAdvisory.object());
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        printer.printAttribute("queueAssignmentAdvisory",
                               d_queueAssignmentAdvisory.object());
    } break;
    case SELECTION_ID_NODE_STATUS_ADVISORY: {
        printer.printAttribute("nodeStatusAdvisory",
                               d_nodeStatusAdvisory.object());
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY: {
        printer.printAttribute("leaderSyncStateQuery",
                               d_leaderSyncStateQuery.object());
    } break;
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE: {
        printer.printAttribute("leaderSyncStateQueryResponse",
                               d_leaderSyncStateQueryResponse.object());
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY: {
        printer.printAttribute("leaderSyncDataQuery",
                               d_leaderSyncDataQuery.object());
    } break;
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE: {
        printer.printAttribute("leaderSyncDataQueryResponse",
                               d_leaderSyncDataQueryResponse.object());
    } break;
    case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST: {
        printer.printAttribute("queueAssignmentRequest",
                               d_queueAssignmentRequest.object());
    } break;
    case SELECTION_ID_STORAGE_SYNC_REQUEST: {
        printer.printAttribute("storageSyncRequest",
                               d_storageSyncRequest.object());
    } break;
    case SELECTION_ID_STORAGE_SYNC_RESPONSE: {
        printer.printAttribute("storageSyncResponse",
                               d_storageSyncResponse.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY: {
        printer.printAttribute("partitionSyncStateQuery",
                               d_partitionSyncStateQuery.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE: {
        printer.printAttribute("partitionSyncStateQueryResponse",
                               d_partitionSyncStateQueryResponse.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY: {
        printer.printAttribute("partitionSyncDataQuery",
                               d_partitionSyncDataQuery.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE: {
        printer.printAttribute("partitionSyncDataQueryResponse",
                               d_partitionSyncDataQueryResponse.object());
    } break;
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS: {
        printer.printAttribute("partitionSyncDataQueryStatus",
                               d_partitionSyncDataQueryStatus.object());
    } break;
    case SELECTION_ID_PRIMARY_STATUS_ADVISORY: {
        printer.printAttribute("primaryStatusAdvisory",
                               d_primaryStatusAdvisory.object());
    } break;
    case SELECTION_ID_CLUSTER_SYNC_REQUEST: {
        printer.printAttribute("clusterSyncRequest",
                               d_clusterSyncRequest.object());
    } break;
    case SELECTION_ID_CLUSTER_SYNC_RESPONSE: {
        printer.printAttribute("clusterSyncResponse",
                               d_clusterSyncResponse.object());
    } break;
    case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
        printer.printAttribute("queueUnAssignmentAdvisory",
                               d_queueUnAssignmentAdvisory.object());
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        printer.printAttribute("queueUnassignedAdvisory",
                               d_queueUnassignedAdvisory.object());
    } break;
    case SELECTION_ID_LEADER_ADVISORY_ACK: {
        printer.printAttribute("leaderAdvisoryAck",
                               d_leaderAdvisoryAck.object());
    } break;
    case SELECTION_ID_LEADER_ADVISORY_COMMIT: {
        printer.printAttribute("leaderAdvisoryCommit",
                               d_leaderAdvisoryCommit.object());
    } break;
    case SELECTION_ID_STATE_NOTIFICATION: {
        printer.printAttribute("stateNotification",
                               d_stateNotification.object());
    } break;
    case SELECTION_ID_STOP_REQUEST: {
        printer.printAttribute("stopRequest", d_stopRequest.object());
    } break;
    case SELECTION_ID_STOP_RESPONSE: {
        printer.printAttribute("stopResponse", d_stopResponse.object());
    } break;
    case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST: {
        printer.printAttribute("queueUnassignmentRequest",
                               d_queueUnassignmentRequest.object());
    } break;
    case SELECTION_ID_QUEUE_UPDATE_ADVISORY: {
        printer.printAttribute("queueUpdateAdvisory",
                               d_queueUpdateAdvisory.object());
    } break;
    case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE: {
        printer.printAttribute("clusterStateFSMMessage",
                               d_clusterStateFSMMessage.object());
    } break;
    case SELECTION_ID_PARTITION_MESSAGE: {
        printer.printAttribute("partitionMessage",
                               d_partitionMessage.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ClusterMessageChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_PARTITION_PRIMARY_ADVISORY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PARTITION_PRIMARY_ADVISORY]
            .name();
    case SELECTION_ID_LEADER_ADVISORY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_ADVISORY].name();
    case SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_ASSIGNMENT_ADVISORY]
            .name();
    case SELECTION_ID_NODE_STATUS_ADVISORY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_NODE_STATUS_ADVISORY]
            .name();
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_SYNC_STATE_QUERY]
            .name();
    case SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_LEADER_SYNC_STATE_QUERY_RESPONSE]
                .name();
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_SYNC_DATA_QUERY]
            .name();
    case SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_LEADER_SYNC_DATA_QUERY_RESPONSE]
                .name();
    case SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_ASSIGNMENT_REQUEST]
            .name();
    case SELECTION_ID_STORAGE_SYNC_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_SYNC_REQUEST]
            .name();
    case SELECTION_ID_STORAGE_SYNC_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_SYNC_RESPONSE]
            .name();
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PARTITION_SYNC_STATE_QUERY]
            .name();
    case SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_PARTITION_SYNC_STATE_QUERY_RESPONSE]
                .name();
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PARTITION_SYNC_DATA_QUERY]
            .name();
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_PARTITION_SYNC_DATA_QUERY_RESPONSE]
                .name();
    case SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_PARTITION_SYNC_DATA_QUERY_STATUS]
                .name();
    case SELECTION_ID_PRIMARY_STATUS_ADVISORY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIMARY_STATUS_ADVISORY]
            .name();
    case SELECTION_ID_CLUSTER_SYNC_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_SYNC_REQUEST]
            .name();
    case SELECTION_ID_CLUSTER_SYNC_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_SYNC_RESPONSE]
            .name();
    case SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_QUEUE_UN_ASSIGNMENT_ADVISORY]
                .name();
    case SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_UNASSIGNED_ADVISORY]
            .name();
    case SELECTION_ID_LEADER_ADVISORY_ACK:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_ADVISORY_ACK]
            .name();
    case SELECTION_ID_LEADER_ADVISORY_COMMIT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LEADER_ADVISORY_COMMIT]
            .name();
    case SELECTION_ID_STATE_NOTIFICATION:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STATE_NOTIFICATION].name();
    case SELECTION_ID_STOP_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STOP_REQUEST].name();
    case SELECTION_ID_STOP_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STOP_RESPONSE].name();
    case SELECTION_ID_QUEUE_UNASSIGNMENT_REQUEST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_UNASSIGNMENT_REQUEST]
            .name();
    case SELECTION_ID_QUEUE_UPDATE_ADVISORY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_UPDATE_ADVISORY]
            .name();
    case SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_CLUSTER_STATE_F_S_M_MESSAGE]
                .name();
    case SELECTION_ID_PARTITION_MESSAGE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PARTITION_MESSAGE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------------
// class ClusterMessage
// --------------------

// CONSTANTS

const char ClusterMessage::CLASS_NAME[] = "ClusterMessage";

const bdlat_AttributeInfo ClusterMessage::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterMessage::lookupAttributeInfo(const char* name, int nameLength)
{
    if (bdlb::String::areEqualCaseless("partitionPrimaryAdvisory",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("leaderAdvisory", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("queueAssignmentAdvisory",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("nodeStatusAdvisory",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("leaderSyncStateQuery",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("leaderSyncStateQueryResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("leaderSyncDataQuery",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("leaderSyncDataQueryResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("queueAssignmentRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("storageSyncRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("storageSyncResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("partitionSyncStateQuery",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("partitionSyncStateQueryResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("partitionSyncDataQuery",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("partitionSyncDataQueryResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("partitionSyncDataQueryStatus",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("primaryStatusAdvisory",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("clusterSyncRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("clusterSyncResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("queueUnAssignmentAdvisory",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("queueUnassignedAdvisory",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("leaderAdvisoryAck",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("leaderAdvisoryCommit",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("stateNotification",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("stopRequest", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("stopResponse", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("queueUnassignmentRequest",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("queueUpdateAdvisory",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("clusterStateFSMMessage",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("partitionMessage", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterMessage::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterMessage::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

ClusterMessage::ClusterMessage(bslma::Allocator* basicAllocator)
: d_choice(basicAllocator)
{
}

ClusterMessage::ClusterMessage(const ClusterMessage& original,
                               bslma::Allocator*     basicAllocator)
: d_choice(original.d_choice, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterMessage::ClusterMessage(ClusterMessage&& original) noexcept
: d_choice(bsl::move(original.d_choice))
{
}

ClusterMessage::ClusterMessage(ClusterMessage&&  original,
                               bslma::Allocator* basicAllocator)
: d_choice(bsl::move(original.d_choice), basicAllocator)
{
}
#endif

ClusterMessage::~ClusterMessage()
{
}

// MANIPULATORS

ClusterMessage& ClusterMessage::operator=(const ClusterMessage& rhs)
{
    if (this != &rhs) {
        d_choice = rhs.d_choice;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterMessage& ClusterMessage::operator=(ClusterMessage&& rhs)
{
    if (this != &rhs) {
        d_choice = bsl::move(rhs.d_choice);
    }

    return *this;
}
#endif

void ClusterMessage::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream& ClusterMessage::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// --------------------------
// class ControlMessageChoice
// --------------------------

// CONSTANTS

const char ControlMessageChoice::CLASS_NAME[] = "ControlMessageChoice";

const bdlat_SelectionInfo ControlMessageChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_STATUS,
     "status",
     sizeof("status") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DISCONNECT,
     "disconnect",
     sizeof("disconnect") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DISCONNECT_RESPONSE,
     "disconnectResponse",
     sizeof("disconnectResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ADMIN_COMMAND,
     "adminCommand",
     sizeof("adminCommand") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ADMIN_COMMAND_RESPONSE,
     "adminCommandResponse",
     sizeof("adminCommandResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_MESSAGE,
     "clusterMessage",
     sizeof("clusterMessage") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_OPEN_QUEUE,
     "openQueue",
     sizeof("openQueue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_OPEN_QUEUE_RESPONSE,
     "openQueueResponse",
     sizeof("openQueueResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLOSE_QUEUE,
     "closeQueue",
     sizeof("closeQueue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLOSE_QUEUE_RESPONSE,
     "closeQueueResponse",
     sizeof("closeQueueResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONFIGURE_QUEUE_STREAM,
     "configureQueueStream",
     sizeof("configureQueueStream") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE,
     "configureQueueStreamResponse",
     sizeof("configureQueueStreamResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONFIGURE_STREAM,
     "configureStream",
     sizeof("configureStream") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONFIGURE_STREAM_RESPONSE,
     "configureStreamResponse",
     sizeof("configureStreamResponse") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ControlMessageChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 14; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ControlMessageChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ControlMessageChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_STATUS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STATUS];
    case SELECTION_ID_DISCONNECT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DISCONNECT];
    case SELECTION_ID_DISCONNECT_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DISCONNECT_RESPONSE];
    case SELECTION_ID_ADMIN_COMMAND:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ADMIN_COMMAND];
    case SELECTION_ID_ADMIN_COMMAND_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ADMIN_COMMAND_RESPONSE];
    case SELECTION_ID_CLUSTER_MESSAGE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_MESSAGE];
    case SELECTION_ID_OPEN_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_QUEUE];
    case SELECTION_ID_OPEN_QUEUE_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_QUEUE_RESPONSE];
    case SELECTION_ID_CLOSE_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_QUEUE];
    case SELECTION_ID_CLOSE_QUEUE_RESPONSE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_QUEUE_RESPONSE];
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIGURE_QUEUE_STREAM];
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_CONFIGURE_QUEUE_STREAM_RESPONSE];
    case SELECTION_ID_CONFIGURE_STREAM:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIGURE_STREAM];
    case SELECTION_ID_CONFIGURE_STREAM_RESPONSE:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_CONFIGURE_STREAM_RESPONSE];
    default: return 0;
    }
}

// CREATORS

ControlMessageChoice::ControlMessageChoice(
    const ControlMessageChoice& original,
    bslma::Allocator*           basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        new (d_status.buffer())
            Status(original.d_status.object(), d_allocator_p);
    } break;
    case SELECTION_ID_DISCONNECT: {
        new (d_disconnect.buffer()) Disconnect(original.d_disconnect.object());
    } break;
    case SELECTION_ID_DISCONNECT_RESPONSE: {
        new (d_disconnectResponse.buffer())
            DisconnectResponse(original.d_disconnectResponse.object());
    } break;
    case SELECTION_ID_ADMIN_COMMAND: {
        new (d_adminCommand.buffer())
            AdminCommand(original.d_adminCommand.object(), d_allocator_p);
    } break;
    case SELECTION_ID_ADMIN_COMMAND_RESPONSE: {
        new (d_adminCommandResponse.buffer())
            AdminCommandResponse(original.d_adminCommandResponse.object(),
                                 d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_MESSAGE: {
        new (d_clusterMessage.buffer())
            ClusterMessage(original.d_clusterMessage.object(), d_allocator_p);
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        new (d_openQueue.buffer())
            OpenQueue(original.d_openQueue.object(), d_allocator_p);
    } break;
    case SELECTION_ID_OPEN_QUEUE_RESPONSE: {
        new (d_openQueueResponse.buffer())
            OpenQueueResponse(original.d_openQueueResponse.object(),
                              d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        new (d_closeQueue.buffer())
            CloseQueue(original.d_closeQueue.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_QUEUE_RESPONSE: {
        new (d_closeQueueResponse.buffer())
            CloseQueueResponse(original.d_closeQueueResponse.object());
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM: {
        new (d_configureQueueStream.buffer())
            ConfigureQueueStream(original.d_configureQueueStream.object(),
                                 d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE: {
        new (d_configureQueueStreamResponse.buffer())
            ConfigureQueueStreamResponse(
                original.d_configureQueueStreamResponse.object(),
                d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_STREAM: {
        new (d_configureStream.buffer())
            ConfigureStream(original.d_configureStream.object(),
                            d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_STREAM_RESPONSE: {
        new (d_configureStreamResponse.buffer()) ConfigureStreamResponse(
            original.d_configureStreamResponse.object(),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ControlMessageChoice::ControlMessageChoice(ControlMessageChoice&& original)
    noexcept : d_selectionId(original.d_selectionId),
               d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        new (d_status.buffer())
            Status(bsl::move(original.d_status.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DISCONNECT: {
        new (d_disconnect.buffer())
            Disconnect(bsl::move(original.d_disconnect.object()));
    } break;
    case SELECTION_ID_DISCONNECT_RESPONSE: {
        new (d_disconnectResponse.buffer()) DisconnectResponse(
            bsl::move(original.d_disconnectResponse.object()));
    } break;
    case SELECTION_ID_ADMIN_COMMAND: {
        new (d_adminCommand.buffer())
            AdminCommand(bsl::move(original.d_adminCommand.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_ADMIN_COMMAND_RESPONSE: {
        new (d_adminCommandResponse.buffer()) AdminCommandResponse(
            bsl::move(original.d_adminCommandResponse.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_MESSAGE: {
        new (d_clusterMessage.buffer())
            ClusterMessage(bsl::move(original.d_clusterMessage.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        new (d_openQueue.buffer())
            OpenQueue(bsl::move(original.d_openQueue.object()), d_allocator_p);
    } break;
    case SELECTION_ID_OPEN_QUEUE_RESPONSE: {
        new (d_openQueueResponse.buffer())
            OpenQueueResponse(bsl::move(original.d_openQueueResponse.object()),
                              d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        new (d_closeQueue.buffer())
            CloseQueue(bsl::move(original.d_closeQueue.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_QUEUE_RESPONSE: {
        new (d_closeQueueResponse.buffer()) CloseQueueResponse(
            bsl::move(original.d_closeQueueResponse.object()));
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM: {
        new (d_configureQueueStream.buffer()) ConfigureQueueStream(
            bsl::move(original.d_configureQueueStream.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE: {
        new (d_configureQueueStreamResponse.buffer())
            ConfigureQueueStreamResponse(
                bsl::move(original.d_configureQueueStreamResponse.object()),
                d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_STREAM: {
        new (d_configureStream.buffer())
            ConfigureStream(bsl::move(original.d_configureStream.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_STREAM_RESPONSE: {
        new (d_configureStreamResponse.buffer()) ConfigureStreamResponse(
            bsl::move(original.d_configureStreamResponse.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ControlMessageChoice::ControlMessageChoice(ControlMessageChoice&& original,
                                           bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        new (d_status.buffer())
            Status(bsl::move(original.d_status.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DISCONNECT: {
        new (d_disconnect.buffer())
            Disconnect(bsl::move(original.d_disconnect.object()));
    } break;
    case SELECTION_ID_DISCONNECT_RESPONSE: {
        new (d_disconnectResponse.buffer()) DisconnectResponse(
            bsl::move(original.d_disconnectResponse.object()));
    } break;
    case SELECTION_ID_ADMIN_COMMAND: {
        new (d_adminCommand.buffer())
            AdminCommand(bsl::move(original.d_adminCommand.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_ADMIN_COMMAND_RESPONSE: {
        new (d_adminCommandResponse.buffer()) AdminCommandResponse(
            bsl::move(original.d_adminCommandResponse.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_MESSAGE: {
        new (d_clusterMessage.buffer())
            ClusterMessage(bsl::move(original.d_clusterMessage.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        new (d_openQueue.buffer())
            OpenQueue(bsl::move(original.d_openQueue.object()), d_allocator_p);
    } break;
    case SELECTION_ID_OPEN_QUEUE_RESPONSE: {
        new (d_openQueueResponse.buffer())
            OpenQueueResponse(bsl::move(original.d_openQueueResponse.object()),
                              d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        new (d_closeQueue.buffer())
            CloseQueue(bsl::move(original.d_closeQueue.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_CLOSE_QUEUE_RESPONSE: {
        new (d_closeQueueResponse.buffer()) CloseQueueResponse(
            bsl::move(original.d_closeQueueResponse.object()));
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM: {
        new (d_configureQueueStream.buffer()) ConfigureQueueStream(
            bsl::move(original.d_configureQueueStream.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE: {
        new (d_configureQueueStreamResponse.buffer())
            ConfigureQueueStreamResponse(
                bsl::move(original.d_configureQueueStreamResponse.object()),
                d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_STREAM: {
        new (d_configureStream.buffer())
            ConfigureStream(bsl::move(original.d_configureStream.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_CONFIGURE_STREAM_RESPONSE: {
        new (d_configureStreamResponse.buffer()) ConfigureStreamResponse(
            bsl::move(original.d_configureStreamResponse.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ControlMessageChoice&
ControlMessageChoice::operator=(const ControlMessageChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_STATUS: {
            makeStatus(rhs.d_status.object());
        } break;
        case SELECTION_ID_DISCONNECT: {
            makeDisconnect(rhs.d_disconnect.object());
        } break;
        case SELECTION_ID_DISCONNECT_RESPONSE: {
            makeDisconnectResponse(rhs.d_disconnectResponse.object());
        } break;
        case SELECTION_ID_ADMIN_COMMAND: {
            makeAdminCommand(rhs.d_adminCommand.object());
        } break;
        case SELECTION_ID_ADMIN_COMMAND_RESPONSE: {
            makeAdminCommandResponse(rhs.d_adminCommandResponse.object());
        } break;
        case SELECTION_ID_CLUSTER_MESSAGE: {
            makeClusterMessage(rhs.d_clusterMessage.object());
        } break;
        case SELECTION_ID_OPEN_QUEUE: {
            makeOpenQueue(rhs.d_openQueue.object());
        } break;
        case SELECTION_ID_OPEN_QUEUE_RESPONSE: {
            makeOpenQueueResponse(rhs.d_openQueueResponse.object());
        } break;
        case SELECTION_ID_CLOSE_QUEUE: {
            makeCloseQueue(rhs.d_closeQueue.object());
        } break;
        case SELECTION_ID_CLOSE_QUEUE_RESPONSE: {
            makeCloseQueueResponse(rhs.d_closeQueueResponse.object());
        } break;
        case SELECTION_ID_CONFIGURE_QUEUE_STREAM: {
            makeConfigureQueueStream(rhs.d_configureQueueStream.object());
        } break;
        case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE: {
            makeConfigureQueueStreamResponse(
                rhs.d_configureQueueStreamResponse.object());
        } break;
        case SELECTION_ID_CONFIGURE_STREAM: {
            makeConfigureStream(rhs.d_configureStream.object());
        } break;
        case SELECTION_ID_CONFIGURE_STREAM_RESPONSE: {
            makeConfigureStreamResponse(
                rhs.d_configureStreamResponse.object());
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
ControlMessageChoice&
ControlMessageChoice::operator=(ControlMessageChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_STATUS: {
            makeStatus(bsl::move(rhs.d_status.object()));
        } break;
        case SELECTION_ID_DISCONNECT: {
            makeDisconnect(bsl::move(rhs.d_disconnect.object()));
        } break;
        case SELECTION_ID_DISCONNECT_RESPONSE: {
            makeDisconnectResponse(
                bsl::move(rhs.d_disconnectResponse.object()));
        } break;
        case SELECTION_ID_ADMIN_COMMAND: {
            makeAdminCommand(bsl::move(rhs.d_adminCommand.object()));
        } break;
        case SELECTION_ID_ADMIN_COMMAND_RESPONSE: {
            makeAdminCommandResponse(
                bsl::move(rhs.d_adminCommandResponse.object()));
        } break;
        case SELECTION_ID_CLUSTER_MESSAGE: {
            makeClusterMessage(bsl::move(rhs.d_clusterMessage.object()));
        } break;
        case SELECTION_ID_OPEN_QUEUE: {
            makeOpenQueue(bsl::move(rhs.d_openQueue.object()));
        } break;
        case SELECTION_ID_OPEN_QUEUE_RESPONSE: {
            makeOpenQueueResponse(bsl::move(rhs.d_openQueueResponse.object()));
        } break;
        case SELECTION_ID_CLOSE_QUEUE: {
            makeCloseQueue(bsl::move(rhs.d_closeQueue.object()));
        } break;
        case SELECTION_ID_CLOSE_QUEUE_RESPONSE: {
            makeCloseQueueResponse(
                bsl::move(rhs.d_closeQueueResponse.object()));
        } break;
        case SELECTION_ID_CONFIGURE_QUEUE_STREAM: {
            makeConfigureQueueStream(
                bsl::move(rhs.d_configureQueueStream.object()));
        } break;
        case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE: {
            makeConfigureQueueStreamResponse(
                bsl::move(rhs.d_configureQueueStreamResponse.object()));
        } break;
        case SELECTION_ID_CONFIGURE_STREAM: {
            makeConfigureStream(bsl::move(rhs.d_configureStream.object()));
        } break;
        case SELECTION_ID_CONFIGURE_STREAM_RESPONSE: {
            makeConfigureStreamResponse(
                bsl::move(rhs.d_configureStreamResponse.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ControlMessageChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        d_status.object().~Status();
    } break;
    case SELECTION_ID_DISCONNECT: {
        d_disconnect.object().~Disconnect();
    } break;
    case SELECTION_ID_DISCONNECT_RESPONSE: {
        d_disconnectResponse.object().~DisconnectResponse();
    } break;
    case SELECTION_ID_ADMIN_COMMAND: {
        d_adminCommand.object().~AdminCommand();
    } break;
    case SELECTION_ID_ADMIN_COMMAND_RESPONSE: {
        d_adminCommandResponse.object().~AdminCommandResponse();
    } break;
    case SELECTION_ID_CLUSTER_MESSAGE: {
        d_clusterMessage.object().~ClusterMessage();
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        d_openQueue.object().~OpenQueue();
    } break;
    case SELECTION_ID_OPEN_QUEUE_RESPONSE: {
        d_openQueueResponse.object().~OpenQueueResponse();
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        d_closeQueue.object().~CloseQueue();
    } break;
    case SELECTION_ID_CLOSE_QUEUE_RESPONSE: {
        d_closeQueueResponse.object().~CloseQueueResponse();
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM: {
        d_configureQueueStream.object().~ConfigureQueueStream();
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE: {
        d_configureQueueStreamResponse.object()
            .~ConfigureQueueStreamResponse();
    } break;
    case SELECTION_ID_CONFIGURE_STREAM: {
        d_configureStream.object().~ConfigureStream();
    } break;
    case SELECTION_ID_CONFIGURE_STREAM_RESPONSE: {
        d_configureStreamResponse.object().~ConfigureStreamResponse();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ControlMessageChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_STATUS: {
        makeStatus();
    } break;
    case SELECTION_ID_DISCONNECT: {
        makeDisconnect();
    } break;
    case SELECTION_ID_DISCONNECT_RESPONSE: {
        makeDisconnectResponse();
    } break;
    case SELECTION_ID_ADMIN_COMMAND: {
        makeAdminCommand();
    } break;
    case SELECTION_ID_ADMIN_COMMAND_RESPONSE: {
        makeAdminCommandResponse();
    } break;
    case SELECTION_ID_CLUSTER_MESSAGE: {
        makeClusterMessage();
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        makeOpenQueue();
    } break;
    case SELECTION_ID_OPEN_QUEUE_RESPONSE: {
        makeOpenQueueResponse();
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        makeCloseQueue();
    } break;
    case SELECTION_ID_CLOSE_QUEUE_RESPONSE: {
        makeCloseQueueResponse();
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM: {
        makeConfigureQueueStream();
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE: {
        makeConfigureQueueStreamResponse();
    } break;
    case SELECTION_ID_CONFIGURE_STREAM: {
        makeConfigureStream();
    } break;
    case SELECTION_ID_CONFIGURE_STREAM_RESPONSE: {
        makeConfigureStreamResponse();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ControlMessageChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Status& ControlMessageChoice::makeStatus()
{
    if (SELECTION_ID_STATUS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_status.object());
    }
    else {
        reset();
        new (d_status.buffer()) Status(d_allocator_p);
        d_selectionId = SELECTION_ID_STATUS;
    }

    return d_status.object();
}

Status& ControlMessageChoice::makeStatus(const Status& value)
{
    if (SELECTION_ID_STATUS == d_selectionId) {
        d_status.object() = value;
    }
    else {
        reset();
        new (d_status.buffer()) Status(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STATUS;
    }

    return d_status.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Status& ControlMessageChoice::makeStatus(Status&& value)
{
    if (SELECTION_ID_STATUS == d_selectionId) {
        d_status.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_status.buffer()) Status(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STATUS;
    }

    return d_status.object();
}
#endif

Disconnect& ControlMessageChoice::makeDisconnect()
{
    if (SELECTION_ID_DISCONNECT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_disconnect.object());
    }
    else {
        reset();
        new (d_disconnect.buffer()) Disconnect();
        d_selectionId = SELECTION_ID_DISCONNECT;
    }

    return d_disconnect.object();
}

Disconnect& ControlMessageChoice::makeDisconnect(const Disconnect& value)
{
    if (SELECTION_ID_DISCONNECT == d_selectionId) {
        d_disconnect.object() = value;
    }
    else {
        reset();
        new (d_disconnect.buffer()) Disconnect(value);
        d_selectionId = SELECTION_ID_DISCONNECT;
    }

    return d_disconnect.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Disconnect& ControlMessageChoice::makeDisconnect(Disconnect&& value)
{
    if (SELECTION_ID_DISCONNECT == d_selectionId) {
        d_disconnect.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_disconnect.buffer()) Disconnect(bsl::move(value));
        d_selectionId = SELECTION_ID_DISCONNECT;
    }

    return d_disconnect.object();
}
#endif

DisconnectResponse& ControlMessageChoice::makeDisconnectResponse()
{
    if (SELECTION_ID_DISCONNECT_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_disconnectResponse.object());
    }
    else {
        reset();
        new (d_disconnectResponse.buffer()) DisconnectResponse();
        d_selectionId = SELECTION_ID_DISCONNECT_RESPONSE;
    }

    return d_disconnectResponse.object();
}

DisconnectResponse&
ControlMessageChoice::makeDisconnectResponse(const DisconnectResponse& value)
{
    if (SELECTION_ID_DISCONNECT_RESPONSE == d_selectionId) {
        d_disconnectResponse.object() = value;
    }
    else {
        reset();
        new (d_disconnectResponse.buffer()) DisconnectResponse(value);
        d_selectionId = SELECTION_ID_DISCONNECT_RESPONSE;
    }

    return d_disconnectResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DisconnectResponse&
ControlMessageChoice::makeDisconnectResponse(DisconnectResponse&& value)
{
    if (SELECTION_ID_DISCONNECT_RESPONSE == d_selectionId) {
        d_disconnectResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_disconnectResponse.buffer())
            DisconnectResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_DISCONNECT_RESPONSE;
    }

    return d_disconnectResponse.object();
}
#endif

AdminCommand& ControlMessageChoice::makeAdminCommand()
{
    if (SELECTION_ID_ADMIN_COMMAND == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_adminCommand.object());
    }
    else {
        reset();
        new (d_adminCommand.buffer()) AdminCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_ADMIN_COMMAND;
    }

    return d_adminCommand.object();
}

AdminCommand& ControlMessageChoice::makeAdminCommand(const AdminCommand& value)
{
    if (SELECTION_ID_ADMIN_COMMAND == d_selectionId) {
        d_adminCommand.object() = value;
    }
    else {
        reset();
        new (d_adminCommand.buffer()) AdminCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ADMIN_COMMAND;
    }

    return d_adminCommand.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AdminCommand& ControlMessageChoice::makeAdminCommand(AdminCommand&& value)
{
    if (SELECTION_ID_ADMIN_COMMAND == d_selectionId) {
        d_adminCommand.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_adminCommand.buffer())
            AdminCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ADMIN_COMMAND;
    }

    return d_adminCommand.object();
}
#endif

AdminCommandResponse& ControlMessageChoice::makeAdminCommandResponse()
{
    if (SELECTION_ID_ADMIN_COMMAND_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_adminCommandResponse.object());
    }
    else {
        reset();
        new (d_adminCommandResponse.buffer())
            AdminCommandResponse(d_allocator_p);
        d_selectionId = SELECTION_ID_ADMIN_COMMAND_RESPONSE;
    }

    return d_adminCommandResponse.object();
}

AdminCommandResponse& ControlMessageChoice::makeAdminCommandResponse(
    const AdminCommandResponse& value)
{
    if (SELECTION_ID_ADMIN_COMMAND_RESPONSE == d_selectionId) {
        d_adminCommandResponse.object() = value;
    }
    else {
        reset();
        new (d_adminCommandResponse.buffer())
            AdminCommandResponse(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ADMIN_COMMAND_RESPONSE;
    }

    return d_adminCommandResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AdminCommandResponse&
ControlMessageChoice::makeAdminCommandResponse(AdminCommandResponse&& value)
{
    if (SELECTION_ID_ADMIN_COMMAND_RESPONSE == d_selectionId) {
        d_adminCommandResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_adminCommandResponse.buffer())
            AdminCommandResponse(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ADMIN_COMMAND_RESPONSE;
    }

    return d_adminCommandResponse.object();
}
#endif

ClusterMessage& ControlMessageChoice::makeClusterMessage()
{
    if (SELECTION_ID_CLUSTER_MESSAGE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterMessage.object());
    }
    else {
        reset();
        new (d_clusterMessage.buffer()) ClusterMessage(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_MESSAGE;
    }

    return d_clusterMessage.object();
}

ClusterMessage&
ControlMessageChoice::makeClusterMessage(const ClusterMessage& value)
{
    if (SELECTION_ID_CLUSTER_MESSAGE == d_selectionId) {
        d_clusterMessage.object() = value;
    }
    else {
        reset();
        new (d_clusterMessage.buffer()) ClusterMessage(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_MESSAGE;
    }

    return d_clusterMessage.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterMessage&
ControlMessageChoice::makeClusterMessage(ClusterMessage&& value)
{
    if (SELECTION_ID_CLUSTER_MESSAGE == d_selectionId) {
        d_clusterMessage.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterMessage.buffer())
            ClusterMessage(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_MESSAGE;
    }

    return d_clusterMessage.object();
}
#endif

OpenQueue& ControlMessageChoice::makeOpenQueue()
{
    if (SELECTION_ID_OPEN_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_openQueue.object());
    }
    else {
        reset();
        new (d_openQueue.buffer()) OpenQueue(d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_QUEUE;
    }

    return d_openQueue.object();
}

OpenQueue& ControlMessageChoice::makeOpenQueue(const OpenQueue& value)
{
    if (SELECTION_ID_OPEN_QUEUE == d_selectionId) {
        d_openQueue.object() = value;
    }
    else {
        reset();
        new (d_openQueue.buffer()) OpenQueue(value, d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_QUEUE;
    }

    return d_openQueue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenQueue& ControlMessageChoice::makeOpenQueue(OpenQueue&& value)
{
    if (SELECTION_ID_OPEN_QUEUE == d_selectionId) {
        d_openQueue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_openQueue.buffer()) OpenQueue(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_QUEUE;
    }

    return d_openQueue.object();
}
#endif

OpenQueueResponse& ControlMessageChoice::makeOpenQueueResponse()
{
    if (SELECTION_ID_OPEN_QUEUE_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_openQueueResponse.object());
    }
    else {
        reset();
        new (d_openQueueResponse.buffer()) OpenQueueResponse(d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_QUEUE_RESPONSE;
    }

    return d_openQueueResponse.object();
}

OpenQueueResponse&
ControlMessageChoice::makeOpenQueueResponse(const OpenQueueResponse& value)
{
    if (SELECTION_ID_OPEN_QUEUE_RESPONSE == d_selectionId) {
        d_openQueueResponse.object() = value;
    }
    else {
        reset();
        new (d_openQueueResponse.buffer())
            OpenQueueResponse(value, d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_QUEUE_RESPONSE;
    }

    return d_openQueueResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
OpenQueueResponse&
ControlMessageChoice::makeOpenQueueResponse(OpenQueueResponse&& value)
{
    if (SELECTION_ID_OPEN_QUEUE_RESPONSE == d_selectionId) {
        d_openQueueResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_openQueueResponse.buffer())
            OpenQueueResponse(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_OPEN_QUEUE_RESPONSE;
    }

    return d_openQueueResponse.object();
}
#endif

CloseQueue& ControlMessageChoice::makeCloseQueue()
{
    if (SELECTION_ID_CLOSE_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_closeQueue.object());
    }
    else {
        reset();
        new (d_closeQueue.buffer()) CloseQueue(d_allocator_p);
        d_selectionId = SELECTION_ID_CLOSE_QUEUE;
    }

    return d_closeQueue.object();
}

CloseQueue& ControlMessageChoice::makeCloseQueue(const CloseQueue& value)
{
    if (SELECTION_ID_CLOSE_QUEUE == d_selectionId) {
        d_closeQueue.object() = value;
    }
    else {
        reset();
        new (d_closeQueue.buffer()) CloseQueue(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLOSE_QUEUE;
    }

    return d_closeQueue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CloseQueue& ControlMessageChoice::makeCloseQueue(CloseQueue&& value)
{
    if (SELECTION_ID_CLOSE_QUEUE == d_selectionId) {
        d_closeQueue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_closeQueue.buffer())
            CloseQueue(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLOSE_QUEUE;
    }

    return d_closeQueue.object();
}
#endif

CloseQueueResponse& ControlMessageChoice::makeCloseQueueResponse()
{
    if (SELECTION_ID_CLOSE_QUEUE_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_closeQueueResponse.object());
    }
    else {
        reset();
        new (d_closeQueueResponse.buffer()) CloseQueueResponse();
        d_selectionId = SELECTION_ID_CLOSE_QUEUE_RESPONSE;
    }

    return d_closeQueueResponse.object();
}

CloseQueueResponse&
ControlMessageChoice::makeCloseQueueResponse(const CloseQueueResponse& value)
{
    if (SELECTION_ID_CLOSE_QUEUE_RESPONSE == d_selectionId) {
        d_closeQueueResponse.object() = value;
    }
    else {
        reset();
        new (d_closeQueueResponse.buffer()) CloseQueueResponse(value);
        d_selectionId = SELECTION_ID_CLOSE_QUEUE_RESPONSE;
    }

    return d_closeQueueResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CloseQueueResponse&
ControlMessageChoice::makeCloseQueueResponse(CloseQueueResponse&& value)
{
    if (SELECTION_ID_CLOSE_QUEUE_RESPONSE == d_selectionId) {
        d_closeQueueResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_closeQueueResponse.buffer())
            CloseQueueResponse(bsl::move(value));
        d_selectionId = SELECTION_ID_CLOSE_QUEUE_RESPONSE;
    }

    return d_closeQueueResponse.object();
}
#endif

ConfigureQueueStream& ControlMessageChoice::makeConfigureQueueStream()
{
    if (SELECTION_ID_CONFIGURE_QUEUE_STREAM == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_configureQueueStream.object());
    }
    else {
        reset();
        new (d_configureQueueStream.buffer())
            ConfigureQueueStream(d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_QUEUE_STREAM;
    }

    return d_configureQueueStream.object();
}

ConfigureQueueStream& ControlMessageChoice::makeConfigureQueueStream(
    const ConfigureQueueStream& value)
{
    if (SELECTION_ID_CONFIGURE_QUEUE_STREAM == d_selectionId) {
        d_configureQueueStream.object() = value;
    }
    else {
        reset();
        new (d_configureQueueStream.buffer())
            ConfigureQueueStream(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_QUEUE_STREAM;
    }

    return d_configureQueueStream.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureQueueStream&
ControlMessageChoice::makeConfigureQueueStream(ConfigureQueueStream&& value)
{
    if (SELECTION_ID_CONFIGURE_QUEUE_STREAM == d_selectionId) {
        d_configureQueueStream.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_configureQueueStream.buffer())
            ConfigureQueueStream(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_QUEUE_STREAM;
    }

    return d_configureQueueStream.object();
}
#endif

ConfigureQueueStreamResponse&
ControlMessageChoice::makeConfigureQueueStreamResponse()
{
    if (SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_configureQueueStreamResponse.object());
    }
    else {
        reset();
        new (d_configureQueueStreamResponse.buffer())
            ConfigureQueueStreamResponse(d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE;
    }

    return d_configureQueueStreamResponse.object();
}

ConfigureQueueStreamResponse&
ControlMessageChoice::makeConfigureQueueStreamResponse(
    const ConfigureQueueStreamResponse& value)
{
    if (SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE == d_selectionId) {
        d_configureQueueStreamResponse.object() = value;
    }
    else {
        reset();
        new (d_configureQueueStreamResponse.buffer())
            ConfigureQueueStreamResponse(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE;
    }

    return d_configureQueueStreamResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureQueueStreamResponse&
ControlMessageChoice::makeConfigureQueueStreamResponse(
    ConfigureQueueStreamResponse&& value)
{
    if (SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE == d_selectionId) {
        d_configureQueueStreamResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_configureQueueStreamResponse.buffer())
            ConfigureQueueStreamResponse(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE;
    }

    return d_configureQueueStreamResponse.object();
}
#endif

ConfigureStream& ControlMessageChoice::makeConfigureStream()
{
    if (SELECTION_ID_CONFIGURE_STREAM == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_configureStream.object());
    }
    else {
        reset();
        new (d_configureStream.buffer()) ConfigureStream(d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_STREAM;
    }

    return d_configureStream.object();
}

ConfigureStream&
ControlMessageChoice::makeConfigureStream(const ConfigureStream& value)
{
    if (SELECTION_ID_CONFIGURE_STREAM == d_selectionId) {
        d_configureStream.object() = value;
    }
    else {
        reset();
        new (d_configureStream.buffer()) ConfigureStream(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_STREAM;
    }

    return d_configureStream.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureStream&
ControlMessageChoice::makeConfigureStream(ConfigureStream&& value)
{
    if (SELECTION_ID_CONFIGURE_STREAM == d_selectionId) {
        d_configureStream.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_configureStream.buffer())
            ConfigureStream(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_STREAM;
    }

    return d_configureStream.object();
}
#endif

ConfigureStreamResponse& ControlMessageChoice::makeConfigureStreamResponse()
{
    if (SELECTION_ID_CONFIGURE_STREAM_RESPONSE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_configureStreamResponse.object());
    }
    else {
        reset();
        new (d_configureStreamResponse.buffer())
            ConfigureStreamResponse(d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_STREAM_RESPONSE;
    }

    return d_configureStreamResponse.object();
}

ConfigureStreamResponse& ControlMessageChoice::makeConfigureStreamResponse(
    const ConfigureStreamResponse& value)
{
    if (SELECTION_ID_CONFIGURE_STREAM_RESPONSE == d_selectionId) {
        d_configureStreamResponse.object() = value;
    }
    else {
        reset();
        new (d_configureStreamResponse.buffer())
            ConfigureStreamResponse(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_STREAM_RESPONSE;
    }

    return d_configureStreamResponse.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigureStreamResponse& ControlMessageChoice::makeConfigureStreamResponse(
    ConfigureStreamResponse&& value)
{
    if (SELECTION_ID_CONFIGURE_STREAM_RESPONSE == d_selectionId) {
        d_configureStreamResponse.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_configureStreamResponse.buffer())
            ConfigureStreamResponse(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIGURE_STREAM_RESPONSE;
    }

    return d_configureStreamResponse.object();
}
#endif

// ACCESSORS

bsl::ostream& ControlMessageChoice::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        printer.printAttribute("status", d_status.object());
    } break;
    case SELECTION_ID_DISCONNECT: {
        printer.printAttribute("disconnect", d_disconnect.object());
    } break;
    case SELECTION_ID_DISCONNECT_RESPONSE: {
        printer.printAttribute("disconnectResponse",
                               d_disconnectResponse.object());
    } break;
    case SELECTION_ID_ADMIN_COMMAND: {
        printer.printAttribute("adminCommand", d_adminCommand.object());
    } break;
    case SELECTION_ID_ADMIN_COMMAND_RESPONSE: {
        printer.printAttribute("adminCommandResponse",
                               d_adminCommandResponse.object());
    } break;
    case SELECTION_ID_CLUSTER_MESSAGE: {
        printer.printAttribute("clusterMessage", d_clusterMessage.object());
    } break;
    case SELECTION_ID_OPEN_QUEUE: {
        printer.printAttribute("openQueue", d_openQueue.object());
    } break;
    case SELECTION_ID_OPEN_QUEUE_RESPONSE: {
        printer.printAttribute("openQueueResponse",
                               d_openQueueResponse.object());
    } break;
    case SELECTION_ID_CLOSE_QUEUE: {
        printer.printAttribute("closeQueue", d_closeQueue.object());
    } break;
    case SELECTION_ID_CLOSE_QUEUE_RESPONSE: {
        printer.printAttribute("closeQueueResponse",
                               d_closeQueueResponse.object());
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM: {
        printer.printAttribute("configureQueueStream",
                               d_configureQueueStream.object());
    } break;
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE: {
        printer.printAttribute("configureQueueStreamResponse",
                               d_configureQueueStreamResponse.object());
    } break;
    case SELECTION_ID_CONFIGURE_STREAM: {
        printer.printAttribute("configureStream", d_configureStream.object());
    } break;
    case SELECTION_ID_CONFIGURE_STREAM_RESPONSE: {
        printer.printAttribute("configureStreamResponse",
                               d_configureStreamResponse.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ControlMessageChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STATUS].name();
    case SELECTION_ID_DISCONNECT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DISCONNECT].name();
    case SELECTION_ID_DISCONNECT_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DISCONNECT_RESPONSE]
            .name();
    case SELECTION_ID_ADMIN_COMMAND:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ADMIN_COMMAND].name();
    case SELECTION_ID_ADMIN_COMMAND_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ADMIN_COMMAND_RESPONSE]
            .name();
    case SELECTION_ID_CLUSTER_MESSAGE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_MESSAGE].name();
    case SELECTION_ID_OPEN_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_QUEUE].name();
    case SELECTION_ID_OPEN_QUEUE_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_QUEUE_RESPONSE]
            .name();
    case SELECTION_ID_CLOSE_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_QUEUE].name();
    case SELECTION_ID_CLOSE_QUEUE_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_QUEUE_RESPONSE]
            .name();
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIGURE_QUEUE_STREAM]
            .name();
    case SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_CONFIGURE_QUEUE_STREAM_RESPONSE]
                .name();
    case SELECTION_ID_CONFIGURE_STREAM:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIGURE_STREAM].name();
    case SELECTION_ID_CONFIGURE_STREAM_RESPONSE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIGURE_STREAM_RESPONSE]
            .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------------
// class ControlMessage
// --------------------

// CONSTANTS

const char ControlMessage::CLASS_NAME[] = "ControlMessage";

const bdlat_AttributeInfo ControlMessage::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_R_ID,
     "rId",
     sizeof("rId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo*
ControlMessage::lookupAttributeInfo(const char* name, int nameLength)
{
    if (bdlb::String::areEqualCaseless("status", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("disconnect", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("disconnectResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("adminCommand", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("adminCommandResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("clusterMessage", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("openQueue", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("openQueueResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("closeQueue", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("closeQueueResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("configureQueueStream",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("configureQueueStreamResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("configureStream", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("configureStreamResponse",
                                       name,
                                       nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ControlMessage::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ControlMessage::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_R_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_R_ID];
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

ControlMessage::ControlMessage(bslma::Allocator* basicAllocator)
: d_choice(basicAllocator)
, d_rId()
{
}

ControlMessage::ControlMessage(const ControlMessage& original,
                               bslma::Allocator*     basicAllocator)
: d_choice(original.d_choice, basicAllocator)
, d_rId(original.d_rId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ControlMessage::ControlMessage(ControlMessage&& original) noexcept
: d_choice(bsl::move(original.d_choice)),
  d_rId(bsl::move(original.d_rId))
{
}

ControlMessage::ControlMessage(ControlMessage&&  original,
                               bslma::Allocator* basicAllocator)
: d_choice(bsl::move(original.d_choice), basicAllocator)
, d_rId(bsl::move(original.d_rId))
{
}
#endif

ControlMessage::~ControlMessage()
{
}

// MANIPULATORS

ControlMessage& ControlMessage::operator=(const ControlMessage& rhs)
{
    if (this != &rhs) {
        d_rId    = rhs.d_rId;
        d_choice = rhs.d_choice;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ControlMessage& ControlMessage::operator=(ControlMessage&& rhs)
{
    if (this != &rhs) {
        d_rId    = bsl::move(rhs.d_rId);
        d_choice = bsl::move(rhs.d_choice);
    }

    return *this;
}
#endif

void ControlMessage::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_rId);
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream& ControlMessage::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("rId", this->rId());
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

}  // close package namespace
}  // close enterprise namespace

// GENERATED BY @BLP_BAS_CODEGEN_VERSION@
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package bmqp_ctrlmsg --msgComponent messages bmqp_ctrlmsg.xsd
// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright 2024 Bloomberg Finance L.P. All rights reserved.
//      Property of Bloomberg Finance L.P. (BFLP)
//      This software is made available solely pursuant to the
//      terms of a BFLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
