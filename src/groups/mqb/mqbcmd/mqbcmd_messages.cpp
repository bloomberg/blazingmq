// Copyright 2019-2024 Bloomberg Finance L.P.
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

// mqbcmd_messages.cpp            *DO NOT EDIT*            @generated -*-C++-*-

#include <mqbcmd_messages.h>

#include <bdlat_formattingmode.h>
#include <bdlat_valuetypefunctions.h>
#include <bdlb_print.h>
#include <bdlb_printmethods.h>
#include <bdlb_string.h>

#include <bdlb_nullableallocatedvalue.h>
#include <bdlb_nullablevalue.h>
#include <bdlt_datetimetz.h>
#include <bdlt_datetz.h>
#include <bdlt_timetz.h>
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
namespace mqbcmd {

// ---------------------
// class AddReverseProxy
// ---------------------

// CONSTANTS

const char AddReverseProxy::CLASS_NAME[] = "AddReverseProxy";

const bdlat_AttributeInfo AddReverseProxy::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLUSTER_NAME,
     "clusterName",
     sizeof("clusterName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_REMOTE_PEER,
     "remotePeer",
     sizeof("remotePeer") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
AddReverseProxy::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            AddReverseProxy::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* AddReverseProxy::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLUSTER_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME];
    case ATTRIBUTE_ID_REMOTE_PEER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REMOTE_PEER];
    default: return 0;
    }
}

// CREATORS

AddReverseProxy::AddReverseProxy(bslma::Allocator* basicAllocator)
: d_clusterName(basicAllocator)
, d_remotePeer(basicAllocator)
{
}

AddReverseProxy::AddReverseProxy(const AddReverseProxy& original,
                                 bslma::Allocator*      basicAllocator)
: d_clusterName(original.d_clusterName, basicAllocator)
, d_remotePeer(original.d_remotePeer, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AddReverseProxy::AddReverseProxy(AddReverseProxy&& original) noexcept
: d_clusterName(bsl::move(original.d_clusterName)),
  d_remotePeer(bsl::move(original.d_remotePeer))
{
}

AddReverseProxy::AddReverseProxy(AddReverseProxy&& original,
                                 bslma::Allocator* basicAllocator)
: d_clusterName(bsl::move(original.d_clusterName), basicAllocator)
, d_remotePeer(bsl::move(original.d_remotePeer), basicAllocator)
{
}
#endif

AddReverseProxy::~AddReverseProxy()
{
}

// MANIPULATORS

AddReverseProxy& AddReverseProxy::operator=(const AddReverseProxy& rhs)
{
    if (this != &rhs) {
        d_clusterName = rhs.d_clusterName;
        d_remotePeer  = rhs.d_remotePeer;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AddReverseProxy& AddReverseProxy::operator=(AddReverseProxy&& rhs)
{
    if (this != &rhs) {
        d_clusterName = bsl::move(rhs.d_clusterName);
        d_remotePeer  = bsl::move(rhs.d_remotePeer);
    }

    return *this;
}
#endif

void AddReverseProxy::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clusterName);
    bdlat_ValueTypeFunctions::reset(&d_remotePeer);
}

// ACCESSORS

bsl::ostream& AddReverseProxy::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clusterName", this->clusterName());
    printer.printAttribute("remotePeer", this->remotePeer());
    printer.end();
    return stream;
}

// ------------------
// class BrokerConfig
// ------------------

// CONSTANTS

const char BrokerConfig::CLASS_NAME[] = "BrokerConfig";

const bdlat_AttributeInfo BrokerConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_AS_J_S_O_N,
     "asJSON",
     sizeof("asJSON") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* BrokerConfig::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            BrokerConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* BrokerConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_AS_J_S_O_N:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AS_J_S_O_N];
    default: return 0;
    }
}

// CREATORS

BrokerConfig::BrokerConfig(bslma::Allocator* basicAllocator)
: d_asJSON(basicAllocator)
{
}

BrokerConfig::BrokerConfig(const BrokerConfig& original,
                           bslma::Allocator*   basicAllocator)
: d_asJSON(original.d_asJSON, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerConfig::BrokerConfig(BrokerConfig&& original) noexcept
: d_asJSON(bsl::move(original.d_asJSON))
{
}

BrokerConfig::BrokerConfig(BrokerConfig&&    original,
                           bslma::Allocator* basicAllocator)
: d_asJSON(bsl::move(original.d_asJSON), basicAllocator)
{
}
#endif

BrokerConfig::~BrokerConfig()
{
}

// MANIPULATORS

BrokerConfig& BrokerConfig::operator=(const BrokerConfig& rhs)
{
    if (this != &rhs) {
        d_asJSON = rhs.d_asJSON;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerConfig& BrokerConfig::operator=(BrokerConfig&& rhs)
{
    if (this != &rhs) {
        d_asJSON = bsl::move(rhs.d_asJSON);
    }

    return *this;
}
#endif

void BrokerConfig::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_asJSON);
}

// ACCESSORS

bsl::ostream&
BrokerConfig::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("asJSON", this->asJSON());
    printer.end();
    return stream;
}

// -------------------
// class CapacityMeter
// -------------------

// CONSTANTS

const char CapacityMeter::CLASS_NAME[] = "CapacityMeter";

const bdlat_AttributeInfo CapacityMeter::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_IS_DISABLED,
     "isDisabled",
     sizeof("isDisabled") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NUM_MESSAGES,
     "numMessages",
     sizeof("numMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MESSAGE_CAPACITY,
     "messageCapacity",
     sizeof("messageCapacity") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_MESSAGES_RESERVED,
     "numMessagesReserved",
     sizeof("numMessagesReserved") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_BYTES,
     "numBytes",
     sizeof("numBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BYTE_CAPACITY,
     "byteCapacity",
     sizeof("byteCapacity") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_BYTES_RESERVED,
     "numBytesReserved",
     sizeof("numBytesReserved") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PARENT,
     "parent",
     sizeof("parent") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* CapacityMeter::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            CapacityMeter::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* CapacityMeter::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_IS_DISABLED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_DISABLED];
    case ATTRIBUTE_ID_NUM_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_MESSAGES];
    case ATTRIBUTE_ID_MESSAGE_CAPACITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_CAPACITY];
    case ATTRIBUTE_ID_NUM_MESSAGES_RESERVED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_MESSAGES_RESERVED];
    case ATTRIBUTE_ID_NUM_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_BYTES];
    case ATTRIBUTE_ID_BYTE_CAPACITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTE_CAPACITY];
    case ATTRIBUTE_ID_NUM_BYTES_RESERVED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_BYTES_RESERVED];
    case ATTRIBUTE_ID_PARENT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARENT];
    default: return 0;
    }
}

// CREATORS

CapacityMeter::CapacityMeter(bslma::Allocator* basicAllocator)
: d_numMessages()
, d_messageCapacity()
, d_numMessagesReserved()
, d_numBytes()
, d_byteCapacity()
, d_numBytesReserved()
, d_name(basicAllocator)
, d_parent(basicAllocator)
, d_isDisabled()
{
}

CapacityMeter::CapacityMeter(const CapacityMeter& original,
                             bslma::Allocator*    basicAllocator)
: d_numMessages(original.d_numMessages)
, d_messageCapacity(original.d_messageCapacity)
, d_numMessagesReserved(original.d_numMessagesReserved)
, d_numBytes(original.d_numBytes)
, d_byteCapacity(original.d_byteCapacity)
, d_numBytesReserved(original.d_numBytesReserved)
, d_name(original.d_name, basicAllocator)
, d_parent(original.d_parent, basicAllocator)
, d_isDisabled(original.d_isDisabled)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CapacityMeter::CapacityMeter(CapacityMeter&& original) noexcept
: d_numMessages(bsl::move(original.d_numMessages)),
  d_messageCapacity(bsl::move(original.d_messageCapacity)),
  d_numMessagesReserved(bsl::move(original.d_numMessagesReserved)),
  d_numBytes(bsl::move(original.d_numBytes)),
  d_byteCapacity(bsl::move(original.d_byteCapacity)),
  d_numBytesReserved(bsl::move(original.d_numBytesReserved)),
  d_name(bsl::move(original.d_name)),
  d_parent(bsl::move(original.d_parent)),
  d_isDisabled(bsl::move(original.d_isDisabled))
{
}

CapacityMeter::CapacityMeter(CapacityMeter&&   original,
                             bslma::Allocator* basicAllocator)
: d_numMessages(bsl::move(original.d_numMessages))
, d_messageCapacity(bsl::move(original.d_messageCapacity))
, d_numMessagesReserved(bsl::move(original.d_numMessagesReserved))
, d_numBytes(bsl::move(original.d_numBytes))
, d_byteCapacity(bsl::move(original.d_byteCapacity))
, d_numBytesReserved(bsl::move(original.d_numBytesReserved))
, d_name(bsl::move(original.d_name), basicAllocator)
, d_parent(bsl::move(original.d_parent), basicAllocator)
, d_isDisabled(bsl::move(original.d_isDisabled))
{
}
#endif

CapacityMeter::~CapacityMeter()
{
}

// MANIPULATORS

CapacityMeter& CapacityMeter::operator=(const CapacityMeter& rhs)
{
    if (this != &rhs) {
        d_name                = rhs.d_name;
        d_isDisabled          = rhs.d_isDisabled;
        d_numMessages         = rhs.d_numMessages;
        d_messageCapacity     = rhs.d_messageCapacity;
        d_numMessagesReserved = rhs.d_numMessagesReserved;
        d_numBytes            = rhs.d_numBytes;
        d_byteCapacity        = rhs.d_byteCapacity;
        d_numBytesReserved    = rhs.d_numBytesReserved;
        d_parent              = rhs.d_parent;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CapacityMeter& CapacityMeter::operator=(CapacityMeter&& rhs)
{
    if (this != &rhs) {
        d_name                = bsl::move(rhs.d_name);
        d_isDisabled          = bsl::move(rhs.d_isDisabled);
        d_numMessages         = bsl::move(rhs.d_numMessages);
        d_messageCapacity     = bsl::move(rhs.d_messageCapacity);
        d_numMessagesReserved = bsl::move(rhs.d_numMessagesReserved);
        d_numBytes            = bsl::move(rhs.d_numBytes);
        d_byteCapacity        = bsl::move(rhs.d_byteCapacity);
        d_numBytesReserved    = bsl::move(rhs.d_numBytesReserved);
        d_parent              = bsl::move(rhs.d_parent);
    }

    return *this;
}
#endif

void CapacityMeter::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_isDisabled);
    bdlat_ValueTypeFunctions::reset(&d_numMessages);
    bdlat_ValueTypeFunctions::reset(&d_messageCapacity);
    bdlat_ValueTypeFunctions::reset(&d_numMessagesReserved);
    bdlat_ValueTypeFunctions::reset(&d_numBytes);
    bdlat_ValueTypeFunctions::reset(&d_byteCapacity);
    bdlat_ValueTypeFunctions::reset(&d_numBytesReserved);
    bdlat_ValueTypeFunctions::reset(&d_parent);
}

// ACCESSORS

bsl::ostream&
CapacityMeter::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("isDisabled", this->isDisabled());
    printer.printAttribute("numMessages", this->numMessages());
    printer.printAttribute("messageCapacity", this->messageCapacity());
    printer.printAttribute("numMessagesReserved", this->numMessagesReserved());
    printer.printAttribute("numBytes", this->numBytes());
    printer.printAttribute("byteCapacity", this->byteCapacity());
    printer.printAttribute("numBytesReserved", this->numBytesReserved());
    printer.printAttribute("parent", this->parent());
    printer.end();
    return stream;
}

// --------------------------
// class ClientMsgGroupsCount
// --------------------------

// CONSTANTS

const char ClientMsgGroupsCount::CLASS_NAME[] = "ClientMsgGroupsCount";

const bdlat_AttributeInfo ClientMsgGroupsCount::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLIENT_DESCRIPTION,
     "clientDescription",
     sizeof("clientDescription") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NUM_MSG_GROUP_IDS,
     "numMsgGroupIds",
     sizeof("numMsgGroupIds") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClientMsgGroupsCount::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClientMsgGroupsCount::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClientMsgGroupsCount::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLIENT_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT_DESCRIPTION];
    case ATTRIBUTE_ID_NUM_MSG_GROUP_IDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_MSG_GROUP_IDS];
    default: return 0;
    }
}

// CREATORS

ClientMsgGroupsCount::ClientMsgGroupsCount(bslma::Allocator* basicAllocator)
: d_clientDescription(basicAllocator)
, d_numMsgGroupIds()
{
}

ClientMsgGroupsCount::ClientMsgGroupsCount(
    const ClientMsgGroupsCount& original,
    bslma::Allocator*           basicAllocator)
: d_clientDescription(original.d_clientDescription, basicAllocator)
, d_numMsgGroupIds(original.d_numMsgGroupIds)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClientMsgGroupsCount::ClientMsgGroupsCount(ClientMsgGroupsCount&& original)
    noexcept : d_clientDescription(bsl::move(original.d_clientDescription)),
               d_numMsgGroupIds(bsl::move(original.d_numMsgGroupIds))
{
}

ClientMsgGroupsCount::ClientMsgGroupsCount(ClientMsgGroupsCount&& original,
                                           bslma::Allocator* basicAllocator)
: d_clientDescription(bsl::move(original.d_clientDescription), basicAllocator)
, d_numMsgGroupIds(bsl::move(original.d_numMsgGroupIds))
{
}
#endif

ClientMsgGroupsCount::~ClientMsgGroupsCount()
{
}

// MANIPULATORS

ClientMsgGroupsCount&
ClientMsgGroupsCount::operator=(const ClientMsgGroupsCount& rhs)
{
    if (this != &rhs) {
        d_clientDescription = rhs.d_clientDescription;
        d_numMsgGroupIds    = rhs.d_numMsgGroupIds;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClientMsgGroupsCount&
ClientMsgGroupsCount::operator=(ClientMsgGroupsCount&& rhs)
{
    if (this != &rhs) {
        d_clientDescription = bsl::move(rhs.d_clientDescription);
        d_numMsgGroupIds    = bsl::move(rhs.d_numMsgGroupIds);
    }

    return *this;
}
#endif

void ClientMsgGroupsCount::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clientDescription);
    bdlat_ValueTypeFunctions::reset(&d_numMsgGroupIds);
}

// ACCESSORS

bsl::ostream& ClientMsgGroupsCount::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clientDescription", this->clientDescription());
    printer.printAttribute("numMsgGroupIds", this->numMsgGroupIds());
    printer.end();
    return stream;
}

// -------------------
// class ClusterDomain
// -------------------

// CONSTANTS

const char ClusterDomain::CLASS_NAME[] = "ClusterDomain";

const bdlat_AttributeInfo ClusterDomain::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NUM_ASSIGNED_QUEUES,
     "numAssignedQueues",
     sizeof("numAssignedQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LOADED,
     "loaded",
     sizeof("loaded") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* ClusterDomain::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterDomain::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterDomain::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_NUM_ASSIGNED_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_ASSIGNED_QUEUES];
    case ATTRIBUTE_ID_LOADED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOADED];
    default: return 0;
    }
}

// CREATORS

ClusterDomain::ClusterDomain(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_numAssignedQueues()
, d_loaded()
{
}

ClusterDomain::ClusterDomain(const ClusterDomain& original,
                             bslma::Allocator*    basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_numAssignedQueues(original.d_numAssignedQueues)
, d_loaded(original.d_loaded)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterDomain::ClusterDomain(ClusterDomain&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_numAssignedQueues(bsl::move(original.d_numAssignedQueues)),
  d_loaded(bsl::move(original.d_loaded))
{
}

ClusterDomain::ClusterDomain(ClusterDomain&&   original,
                             bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_numAssignedQueues(bsl::move(original.d_numAssignedQueues))
, d_loaded(bsl::move(original.d_loaded))
{
}
#endif

ClusterDomain::~ClusterDomain()
{
}

// MANIPULATORS

ClusterDomain& ClusterDomain::operator=(const ClusterDomain& rhs)
{
    if (this != &rhs) {
        d_name              = rhs.d_name;
        d_numAssignedQueues = rhs.d_numAssignedQueues;
        d_loaded            = rhs.d_loaded;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterDomain& ClusterDomain::operator=(ClusterDomain&& rhs)
{
    if (this != &rhs) {
        d_name              = bsl::move(rhs.d_name);
        d_numAssignedQueues = bsl::move(rhs.d_numAssignedQueues);
        d_loaded            = bsl::move(rhs.d_loaded);
    }

    return *this;
}
#endif

void ClusterDomain::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_numAssignedQueues);
    bdlat_ValueTypeFunctions::reset(&d_loaded);
}

// ACCESSORS

bsl::ostream&
ClusterDomain::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("numAssignedQueues", this->numAssignedQueues());
    printer.printAttribute("loaded", this->loaded());
    printer.end();
    return stream;
}

// -----------------
// class ClusterNode
// -----------------

// CONSTANTS

const char ClusterNode::CLASS_NAME[] = "ClusterNode";

const bdlat_AttributeInfo ClusterNode::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_HOST_NAME,
     "hostName",
     sizeof("hostName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NODE_ID,
     "nodeId",
     sizeof("nodeId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_DATA_CENTER,
     "dataCenter",
     sizeof("dataCenter") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* ClusterNode::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterNode::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterNode::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_HOST_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME];
    case ATTRIBUTE_ID_NODE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_ID];
    case ATTRIBUTE_ID_DATA_CENTER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_CENTER];
    default: return 0;
    }
}

// CREATORS

ClusterNode::ClusterNode(bslma::Allocator* basicAllocator)
: d_hostName(basicAllocator)
, d_dataCenter(basicAllocator)
, d_nodeId()
{
}

ClusterNode::ClusterNode(const ClusterNode& original,
                         bslma::Allocator*  basicAllocator)
: d_hostName(original.d_hostName, basicAllocator)
, d_dataCenter(original.d_dataCenter, basicAllocator)
, d_nodeId(original.d_nodeId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterNode::ClusterNode(ClusterNode&& original) noexcept
: d_hostName(bsl::move(original.d_hostName)),
  d_dataCenter(bsl::move(original.d_dataCenter)),
  d_nodeId(bsl::move(original.d_nodeId))
{
}

ClusterNode::ClusterNode(ClusterNode&&     original,
                         bslma::Allocator* basicAllocator)
: d_hostName(bsl::move(original.d_hostName), basicAllocator)
, d_dataCenter(bsl::move(original.d_dataCenter), basicAllocator)
, d_nodeId(bsl::move(original.d_nodeId))
{
}
#endif

ClusterNode::~ClusterNode()
{
}

// MANIPULATORS

ClusterNode& ClusterNode::operator=(const ClusterNode& rhs)
{
    if (this != &rhs) {
        d_hostName   = rhs.d_hostName;
        d_nodeId     = rhs.d_nodeId;
        d_dataCenter = rhs.d_dataCenter;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterNode& ClusterNode::operator=(ClusterNode&& rhs)
{
    if (this != &rhs) {
        d_hostName   = bsl::move(rhs.d_hostName);
        d_nodeId     = bsl::move(rhs.d_nodeId);
        d_dataCenter = bsl::move(rhs.d_dataCenter);
    }

    return *this;
}
#endif

void ClusterNode::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_hostName);
    bdlat_ValueTypeFunctions::reset(&d_nodeId);
    bdlat_ValueTypeFunctions::reset(&d_dataCenter);
}

// ACCESSORS

bsl::ostream&
ClusterNode::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("hostName", this->hostName());
    printer.printAttribute("nodeId", this->nodeId());
    printer.printAttribute("dataCenter", this->dataCenter());
    printer.end();
    return stream;
}

// -----------------
// class CommandSpec
// -----------------

// CONSTANTS

const char CommandSpec::CLASS_NAME[] = "CommandSpec";

const bdlat_AttributeInfo CommandSpec::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_COMMAND,
     "command",
     sizeof("command") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_DESCRIPTION,
     "description",
     sizeof("description") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* CommandSpec::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            CommandSpec::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* CommandSpec::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_COMMAND:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND];
    case ATTRIBUTE_ID_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DESCRIPTION];
    default: return 0;
    }
}

// CREATORS

CommandSpec::CommandSpec(bslma::Allocator* basicAllocator)
: d_command(basicAllocator)
, d_description(basicAllocator)
{
}

CommandSpec::CommandSpec(const CommandSpec& original,
                         bslma::Allocator*  basicAllocator)
: d_command(original.d_command, basicAllocator)
, d_description(original.d_description, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CommandSpec::CommandSpec(CommandSpec&& original) noexcept
: d_command(bsl::move(original.d_command)),
  d_description(bsl::move(original.d_description))
{
}

CommandSpec::CommandSpec(CommandSpec&&     original,
                         bslma::Allocator* basicAllocator)
: d_command(bsl::move(original.d_command), basicAllocator)
, d_description(bsl::move(original.d_description), basicAllocator)
{
}
#endif

CommandSpec::~CommandSpec()
{
}

// MANIPULATORS

CommandSpec& CommandSpec::operator=(const CommandSpec& rhs)
{
    if (this != &rhs) {
        d_command     = rhs.d_command;
        d_description = rhs.d_description;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CommandSpec& CommandSpec::operator=(CommandSpec&& rhs)
{
    if (this != &rhs) {
        d_command     = bsl::move(rhs.d_command);
        d_description = bsl::move(rhs.d_description);
    }

    return *this;
}
#endif

void CommandSpec::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_command);
    bdlat_ValueTypeFunctions::reset(&d_description);
}

// ACCESSORS

bsl::ostream&
CommandSpec::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("command", this->command());
    printer.printAttribute("description", this->description());
    printer.end();
    return stream;
}

// ------------------
// class ConsumerInfo
// ------------------

// CONSTANTS

const char ConsumerInfo::CLASS_NAME[] = "ConsumerInfo";

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
: d_maxUnconfirmedMessages()
, d_maxUnconfirmedBytes()
, d_consumerPriority()
, d_consumerPriorityCount()
{
}

// MANIPULATORS

void ConsumerInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_maxUnconfirmedMessages);
    bdlat_ValueTypeFunctions::reset(&d_maxUnconfirmedBytes);
    bdlat_ValueTypeFunctions::reset(&d_consumerPriority);
    bdlat_ValueTypeFunctions::reset(&d_consumerPriorityCount);
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

// --------------------
// class ConsumerStatus
// --------------------

// CONSTANTS

const char ConsumerStatus::CLASS_NAME[] = "ConsumerStatus";

const bdlat_EnumeratorInfo ConsumerStatus::ENUMERATOR_INFO_ARRAY[] = {
    {ConsumerStatus::ALIVE, "alive", sizeof("alive") - 1, ""},
    {ConsumerStatus::REGISTERED, "registered", sizeof("registered") - 1, ""},
    {ConsumerStatus::UNAUTHORIZED,
     "unauthorized",
     sizeof("unauthorized") - 1,
     ""}};

// CLASS METHODS

int ConsumerStatus::fromInt(ConsumerStatus::Value* result, int number)
{
    switch (number) {
    case ConsumerStatus::ALIVE:
    case ConsumerStatus::REGISTERED:
    case ConsumerStatus::UNAUTHORIZED:
        *result = static_cast<ConsumerStatus::Value>(number);
        return 0;
    default: return -1;
    }
}

int ConsumerStatus::fromString(ConsumerStatus::Value* result,
                               const char*            string,
                               int                    stringLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            ConsumerStatus::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<ConsumerStatus::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* ConsumerStatus::toString(ConsumerStatus::Value value)
{
    switch (value) {
    case ALIVE: {
        return "alive";
    }
    case REGISTERED: {
        return "registered";
    }
    case UNAUTHORIZED: {
        return "unauthorized";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -------------
// class Context
// -------------

// CONSTANTS

const char Context::CLASS_NAME[] = "Context";

const bdlat_AttributeInfo Context::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_QUEUE_HANDLE_PARAMETERS_JSON,
     "queueHandleParametersJson",
     sizeof("queueHandleParametersJson") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Context::lookupAttributeInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Context::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Context::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_QUEUE_HANDLE_PARAMETERS_JSON:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_QUEUE_HANDLE_PARAMETERS_JSON];
    default: return 0;
    }
}

// CREATORS

Context::Context(bslma::Allocator* basicAllocator)
: d_queueHandleParametersJson(basicAllocator)
{
}

Context::Context(const Context& original, bslma::Allocator* basicAllocator)
: d_queueHandleParametersJson(original.d_queueHandleParametersJson,
                              basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Context::Context(Context&& original) noexcept
: d_queueHandleParametersJson(bsl::move(original.d_queueHandleParametersJson))
{
}

Context::Context(Context&& original, bslma::Allocator* basicAllocator)
: d_queueHandleParametersJson(bsl::move(original.d_queueHandleParametersJson),
                              basicAllocator)
{
}
#endif

Context::~Context()
{
}

// MANIPULATORS

Context& Context::operator=(const Context& rhs)
{
    if (this != &rhs) {
        d_queueHandleParametersJson = rhs.d_queueHandleParametersJson;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Context& Context::operator=(Context&& rhs)
{
    if (this != &rhs) {
        d_queueHandleParametersJson = bsl::move(
            rhs.d_queueHandleParametersJson);
    }

    return *this;
}
#endif

void Context::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_queueHandleParametersJson);
}

// ACCESSORS

bsl::ostream&
Context::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queueHandleParametersJson",
                           this->queueHandleParametersJson());
    printer.end();
    return stream;
}

// -----------------------
// class DomainReconfigure
// -----------------------

// CONSTANTS

const char DomainReconfigure::CLASS_NAME[] = "DomainReconfigure";

const bdlat_SelectionInfo DomainReconfigure::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_DOMAIN,
     "domain",
     sizeof("domain") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_SelectionInfo*
DomainReconfigure::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            DomainReconfigure::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* DomainReconfigure::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_DOMAIN:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN];
    default: return 0;
    }
}

// CREATORS

DomainReconfigure::DomainReconfigure(const DomainReconfigure& original,
                                     bslma::Allocator*        basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            bsl::string(original.d_domain.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainReconfigure::DomainReconfigure(DomainReconfigure&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            bsl::string(bsl::move(original.d_domain.object()), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

DomainReconfigure::DomainReconfigure(DomainReconfigure&& original,
                                     bslma::Allocator*   basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            bsl::string(bsl::move(original.d_domain.object()), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

DomainReconfigure& DomainReconfigure::operator=(const DomainReconfigure& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DOMAIN: {
            makeDomain(rhs.d_domain.object());
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
DomainReconfigure& DomainReconfigure::operator=(DomainReconfigure&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DOMAIN: {
            makeDomain(bsl::move(rhs.d_domain.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void DomainReconfigure::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        typedef bsl::string Type;
        d_domain.object().~Type();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int DomainReconfigure::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_DOMAIN: {
        makeDomain();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int DomainReconfigure::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

bsl::string& DomainReconfigure::makeDomain()
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domain.object());
    }
    else {
        reset();
        new (d_domain.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}

bsl::string& DomainReconfigure::makeDomain(const bsl::string& value)
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        d_domain.object() = value;
    }
    else {
        reset();
        new (d_domain.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& DomainReconfigure::makeDomain(bsl::string&& value)
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        d_domain.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domain.buffer()) bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}
#endif

// ACCESSORS

bsl::ostream& DomainReconfigure::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        printer.printAttribute("domain", d_domain.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* DomainReconfigure::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ------------------
// class ElectorState
// ------------------

// CONSTANTS

const char ElectorState::CLASS_NAME[] = "ElectorState";

const bdlat_EnumeratorInfo ElectorState::ENUMERATOR_INFO_ARRAY[] = {
    {ElectorState::DORMANT, "DORMANT", sizeof("DORMANT") - 1, ""},
    {ElectorState::FOLLOWER, "FOLLOWER", sizeof("FOLLOWER") - 1, ""},
    {ElectorState::CANDIDATE, "CANDIDATE", sizeof("CANDIDATE") - 1, ""},
    {ElectorState::LEADER, "LEADER", sizeof("LEADER") - 1, ""}};

// CLASS METHODS

int ElectorState::fromInt(ElectorState::Value* result, int number)
{
    switch (number) {
    case ElectorState::DORMANT:
    case ElectorState::FOLLOWER:
    case ElectorState::CANDIDATE:
    case ElectorState::LEADER:
        *result = static_cast<ElectorState::Value>(number);
        return 0;
    default: return -1;
    }
}

int ElectorState::fromString(ElectorState::Value* result,
                             const char*          string,
                             int                  stringLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            ElectorState::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<ElectorState::Value>(enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* ElectorState::toString(ElectorState::Value value)
{
    switch (value) {
    case DORMANT: {
        return "DORMANT";
    }
    case FOLLOWER: {
        return "FOLLOWER";
    }
    case CANDIDATE: {
        return "CANDIDATE";
    }
    case LEADER: {
        return "LEADER";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// --------------------
// class EncodingFormat
// --------------------

// CONSTANTS

const char EncodingFormat::CLASS_NAME[] = "EncodingFormat";

const bdlat_EnumeratorInfo EncodingFormat::ENUMERATOR_INFO_ARRAY[] = {
    {EncodingFormat::TEXT, "TEXT", sizeof("TEXT") - 1, ""},
    {EncodingFormat::JSON_COMPACT,
     "JSON_COMPACT",
     sizeof("JSON_COMPACT") - 1,
     ""},
    {EncodingFormat::JSON_PRETTY,
     "JSON_PRETTY",
     sizeof("JSON_PRETTY") - 1,
     ""}};

// CLASS METHODS

int EncodingFormat::fromInt(EncodingFormat::Value* result, int number)
{
    switch (number) {
    case EncodingFormat::TEXT:
    case EncodingFormat::JSON_COMPACT:
    case EncodingFormat::JSON_PRETTY:
        *result = static_cast<EncodingFormat::Value>(number);
        return 0;
    default: return -1;
    }
}

int EncodingFormat::fromString(EncodingFormat::Value* result,
                               const char*            string,
                               int                    stringLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            EncodingFormat::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<EncodingFormat::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* EncodingFormat::toString(EncodingFormat::Value value)
{
    switch (value) {
    case TEXT: {
        return "TEXT";
    }
    case JSON_COMPACT: {
        return "JSON_COMPACT";
    }
    case JSON_PRETTY: {
        return "JSON_PRETTY";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -----------
// class Error
// -----------

// CONSTANTS

const char Error::CLASS_NAME[] = "Error";

const bdlat_AttributeInfo Error::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_MESSAGE,
     "message",
     sizeof("message") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Error::lookupAttributeInfo(const char* name,
                                                      int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Error::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Error::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MESSAGE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE];
    default: return 0;
    }
}

// CREATORS

Error::Error(bslma::Allocator* basicAllocator)
: d_message(basicAllocator)
{
}

Error::Error(const Error& original, bslma::Allocator* basicAllocator)
: d_message(original.d_message, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error::Error(Error&& original) noexcept
: d_message(bsl::move(original.d_message))
{
}

Error::Error(Error&& original, bslma::Allocator* basicAllocator)
: d_message(bsl::move(original.d_message), basicAllocator)
{
}
#endif

Error::~Error()
{
}

// MANIPULATORS

Error& Error::operator=(const Error& rhs)
{
    if (this != &rhs) {
        d_message = rhs.d_message;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& Error::operator=(Error&& rhs)
{
    if (this != &rhs) {
        d_message = bsl::move(rhs.d_message);
    }

    return *this;
}
#endif

void Error::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_message);
}

// ACCESSORS

bsl::ostream&
Error::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("message", this->message());
    printer.end();
    return stream;
}

// --------------
// class FileInfo
// --------------

// CONSTANTS

const char FileInfo::CLASS_NAME[] = "FileInfo";

const bdlat_AttributeInfo FileInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_POSITION_BYTES,
     "positionBytes",
     sizeof("positionBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SIZE_BYTES,
     "sizeBytes",
     sizeof("sizeBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_OUTSTANDING_BYTES,
     "outstandingBytes",
     sizeof("outstandingBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* FileInfo::lookupAttributeInfo(const char* name,
                                                         int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            FileInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* FileInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_POSITION_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POSITION_BYTES];
    case ATTRIBUTE_ID_SIZE_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SIZE_BYTES];
    case ATTRIBUTE_ID_OUTSTANDING_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OUTSTANDING_BYTES];
    default: return 0;
    }
}

// CREATORS

FileInfo::FileInfo()
: d_positionBytes()
, d_sizeBytes()
, d_outstandingBytes()
{
}

// MANIPULATORS

void FileInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_positionBytes);
    bdlat_ValueTypeFunctions::reset(&d_sizeBytes);
    bdlat_ValueTypeFunctions::reset(&d_outstandingBytes);
}

// ACCESSORS

bsl::ostream&
FileInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("positionBytes", this->positionBytes());
    printer.printAttribute("sizeBytes", this->sizeBytes());
    printer.printAttribute("outstandingBytes", this->outstandingBytes());
    printer.end();
    return stream;
}

// -------------
// class FileSet
// -------------

// CONSTANTS

const char FileSet::CLASS_NAME[] = "FileSet";

const bdlat_AttributeInfo FileSet::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_DATA_FILE_NAME,
     "dataFileName",
     sizeof("dataFileName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ALIASED_BLOB_BUFFER_COUNT,
     "aliasedBlobBufferCount",
     sizeof("aliasedBlobBufferCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* FileSet::lookupAttributeInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            FileSet::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* FileSet::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_DATA_FILE_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_FILE_NAME];
    case ATTRIBUTE_ID_ALIASED_BLOB_BUFFER_COUNT:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_ALIASED_BLOB_BUFFER_COUNT];
    default: return 0;
    }
}

// CREATORS

FileSet::FileSet(bslma::Allocator* basicAllocator)
: d_aliasedBlobBufferCount()
, d_dataFileName(basicAllocator)
{
}

FileSet::FileSet(const FileSet& original, bslma::Allocator* basicAllocator)
: d_aliasedBlobBufferCount(original.d_aliasedBlobBufferCount)
, d_dataFileName(original.d_dataFileName, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FileSet::FileSet(FileSet&& original) noexcept
: d_aliasedBlobBufferCount(bsl::move(original.d_aliasedBlobBufferCount)),
  d_dataFileName(bsl::move(original.d_dataFileName))
{
}

FileSet::FileSet(FileSet&& original, bslma::Allocator* basicAllocator)
: d_aliasedBlobBufferCount(bsl::move(original.d_aliasedBlobBufferCount))
, d_dataFileName(bsl::move(original.d_dataFileName), basicAllocator)
{
}
#endif

FileSet::~FileSet()
{
}

// MANIPULATORS

FileSet& FileSet::operator=(const FileSet& rhs)
{
    if (this != &rhs) {
        d_dataFileName           = rhs.d_dataFileName;
        d_aliasedBlobBufferCount = rhs.d_aliasedBlobBufferCount;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FileSet& FileSet::operator=(FileSet&& rhs)
{
    if (this != &rhs) {
        d_dataFileName           = bsl::move(rhs.d_dataFileName);
        d_aliasedBlobBufferCount = bsl::move(rhs.d_aliasedBlobBufferCount);
    }

    return *this;
}
#endif

void FileSet::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_dataFileName);
    bdlat_ValueTypeFunctions::reset(&d_aliasedBlobBufferCount);
}

// ACCESSORS

bsl::ostream&
FileSet::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("dataFileName", this->dataFileName());
    printer.printAttribute("aliasedBlobBufferCount",
                           this->aliasedBlobBufferCount());
    printer.end();
    return stream;
}

// --------------------
// class FileStoreState
// --------------------

// CONSTANTS

const char FileStoreState::CLASS_NAME[] = "FileStoreState";

const bdlat_EnumeratorInfo FileStoreState::ENUMERATOR_INFO_ARRAY[] = {
    {FileStoreState::OPEN, "open", sizeof("open") - 1, ""},
    {FileStoreState::CLOSED, "closed", sizeof("closed") - 1, ""},
    {FileStoreState::STOPPING, "stopping", sizeof("stopping") - 1, ""}};

// CLASS METHODS

int FileStoreState::fromInt(FileStoreState::Value* result, int number)
{
    switch (number) {
    case FileStoreState::OPEN:
    case FileStoreState::CLOSED:
    case FileStoreState::STOPPING:
        *result = static_cast<FileStoreState::Value>(number);
        return 0;
    default: return -1;
    }
}

int FileStoreState::fromString(FileStoreState::Value* result,
                               const char*            string,
                               int                    stringLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            FileStoreState::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<FileStoreState::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* FileStoreState::toString(FileStoreState::Value value)
{
    switch (value) {
    case OPEN: {
        return "open";
    }
    case CLOSED: {
        return "closed";
    }
    case STOPPING: {
        return "stopping";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -----------------
// class HelpCommand
// -----------------

// CONSTANTS

const char HelpCommand::CLASS_NAME[] = "HelpCommand";

const bool HelpCommand::DEFAULT_INITIALIZER_PLUMBING = false;

const bdlat_AttributeInfo HelpCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PLUMBING,
     "plumbing",
     sizeof("plumbing") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* HelpCommand::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            HelpCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* HelpCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PLUMBING:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUMBING];
    default: return 0;
    }
}

// CREATORS

HelpCommand::HelpCommand()
: d_plumbing(DEFAULT_INITIALIZER_PLUMBING)
{
}

// MANIPULATORS

void HelpCommand::reset()
{
    d_plumbing = DEFAULT_INITIALIZER_PLUMBING;
}

// ACCESSORS

bsl::ostream&
HelpCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("plumbing", this->plumbing());
    printer.end();
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

// ------------------
// class LeaderStatus
// ------------------

// CONSTANTS

const char LeaderStatus::CLASS_NAME[] = "LeaderStatus";

const bdlat_EnumeratorInfo LeaderStatus::ENUMERATOR_INFO_ARRAY[] = {
    {LeaderStatus::UNDEFINED, "UNDEFINED", sizeof("UNDEFINED") - 1, ""},
    {LeaderStatus::PASSIVE, "PASSIVE", sizeof("PASSIVE") - 1, ""},
    {LeaderStatus::ACTIVE, "ACTIVE", sizeof("ACTIVE") - 1, ""}};

// CLASS METHODS

int LeaderStatus::fromInt(LeaderStatus::Value* result, int number)
{
    switch (number) {
    case LeaderStatus::UNDEFINED:
    case LeaderStatus::PASSIVE:
    case LeaderStatus::ACTIVE:
        *result = static_cast<LeaderStatus::Value>(number);
        return 0;
    default: return -1;
    }
}

int LeaderStatus::fromString(LeaderStatus::Value* result,
                             const char*          string,
                             int                  stringLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            LeaderStatus::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<LeaderStatus::Value>(enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* LeaderStatus::toString(LeaderStatus::Value value)
{
    switch (value) {
    case UNDEFINED: {
        return "UNDEFINED";
    }
    case PASSIVE: {
        return "PASSIVE";
    }
    case ACTIVE: {
        return "ACTIVE";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ------------------------------
// class LeastRecentlyUsedGroupId
// ------------------------------

// CONSTANTS

const char LeastRecentlyUsedGroupId::CLASS_NAME[] = "LeastRecentlyUsedGroupId";

const bdlat_AttributeInfo LeastRecentlyUsedGroupId::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLIENT_DESCRIPTION,
     "clientDescription",
     sizeof("clientDescription") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MSG_GROUP_ID,
     "msgGroupId",
     sizeof("msgGroupId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_LAST_SEEN_DELTA_NANOSECONDS,
     "lastSeenDeltaNanoseconds",
     sizeof("lastSeenDeltaNanoseconds") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
LeastRecentlyUsedGroupId::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LeastRecentlyUsedGroupId::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
LeastRecentlyUsedGroupId::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLIENT_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT_DESCRIPTION];
    case ATTRIBUTE_ID_MSG_GROUP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_GROUP_ID];
    case ATTRIBUTE_ID_LAST_SEEN_DELTA_NANOSECONDS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_LAST_SEEN_DELTA_NANOSECONDS];
    default: return 0;
    }
}

// CREATORS

LeastRecentlyUsedGroupId::LeastRecentlyUsedGroupId(
    bslma::Allocator* basicAllocator)
: d_lastSeenDeltaNanoseconds()
, d_clientDescription(basicAllocator)
, d_msgGroupId(basicAllocator)
{
}

LeastRecentlyUsedGroupId::LeastRecentlyUsedGroupId(
    const LeastRecentlyUsedGroupId& original,
    bslma::Allocator*               basicAllocator)
: d_lastSeenDeltaNanoseconds(original.d_lastSeenDeltaNanoseconds)
, d_clientDescription(original.d_clientDescription, basicAllocator)
, d_msgGroupId(original.d_msgGroupId, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeastRecentlyUsedGroupId::LeastRecentlyUsedGroupId(
    LeastRecentlyUsedGroupId&& original) noexcept
: d_lastSeenDeltaNanoseconds(bsl::move(original.d_lastSeenDeltaNanoseconds)),
  d_clientDescription(bsl::move(original.d_clientDescription)),
  d_msgGroupId(bsl::move(original.d_msgGroupId))
{
}

LeastRecentlyUsedGroupId::LeastRecentlyUsedGroupId(
    LeastRecentlyUsedGroupId&& original,
    bslma::Allocator*          basicAllocator)
: d_lastSeenDeltaNanoseconds(bsl::move(original.d_lastSeenDeltaNanoseconds))
, d_clientDescription(bsl::move(original.d_clientDescription), basicAllocator)
, d_msgGroupId(bsl::move(original.d_msgGroupId), basicAllocator)
{
}
#endif

LeastRecentlyUsedGroupId::~LeastRecentlyUsedGroupId()
{
}

// MANIPULATORS

LeastRecentlyUsedGroupId&
LeastRecentlyUsedGroupId::operator=(const LeastRecentlyUsedGroupId& rhs)
{
    if (this != &rhs) {
        d_clientDescription        = rhs.d_clientDescription;
        d_msgGroupId               = rhs.d_msgGroupId;
        d_lastSeenDeltaNanoseconds = rhs.d_lastSeenDeltaNanoseconds;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LeastRecentlyUsedGroupId&
LeastRecentlyUsedGroupId::operator=(LeastRecentlyUsedGroupId&& rhs)
{
    if (this != &rhs) {
        d_clientDescription        = bsl::move(rhs.d_clientDescription);
        d_msgGroupId               = bsl::move(rhs.d_msgGroupId);
        d_lastSeenDeltaNanoseconds = bsl::move(rhs.d_lastSeenDeltaNanoseconds);
    }

    return *this;
}
#endif

void LeastRecentlyUsedGroupId::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clientDescription);
    bdlat_ValueTypeFunctions::reset(&d_msgGroupId);
    bdlat_ValueTypeFunctions::reset(&d_lastSeenDeltaNanoseconds);
}

// ACCESSORS

bsl::ostream& LeastRecentlyUsedGroupId::print(bsl::ostream& stream,
                                              int           level,
                                              int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clientDescription", this->clientDescription());
    printer.printAttribute("msgGroupId", this->msgGroupId());
    printer.printAttribute("lastSeenDeltaNanoseconds",
                           this->lastSeenDeltaNanoseconds());
    printer.end();
    return stream;
}

// ------------------
// class ListMessages
// ------------------

// CONSTANTS

const char ListMessages::CLASS_NAME[] = "ListMessages";

const bdlat_AttributeInfo ListMessages::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_OFFSET,
     "offset",
     sizeof("offset") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_COUNT,
     "count",
     sizeof("count") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* ListMessages::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ListMessages::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ListMessages::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_OFFSET:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OFFSET];
    case ATTRIBUTE_ID_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COUNT];
    default: return 0;
    }
}

// CREATORS

ListMessages::ListMessages(bslma::Allocator* basicAllocator)
: d_appId(basicAllocator)
, d_offset()
, d_count()
{
}

ListMessages::ListMessages(const ListMessages& original,
                           bslma::Allocator*   basicAllocator)
: d_appId(original.d_appId, basicAllocator)
, d_offset(original.d_offset)
, d_count(original.d_count)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ListMessages::ListMessages(ListMessages&& original) noexcept
: d_appId(bsl::move(original.d_appId)),
  d_offset(bsl::move(original.d_offset)),
  d_count(bsl::move(original.d_count))
{
}

ListMessages::ListMessages(ListMessages&&    original,
                           bslma::Allocator* basicAllocator)
: d_appId(bsl::move(original.d_appId), basicAllocator)
, d_offset(bsl::move(original.d_offset))
, d_count(bsl::move(original.d_count))
{
}
#endif

ListMessages::~ListMessages()
{
}

// MANIPULATORS

ListMessages& ListMessages::operator=(const ListMessages& rhs)
{
    if (this != &rhs) {
        d_appId  = rhs.d_appId;
        d_offset = rhs.d_offset;
        d_count  = rhs.d_count;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ListMessages& ListMessages::operator=(ListMessages&& rhs)
{
    if (this != &rhs) {
        d_appId  = bsl::move(rhs.d_appId);
        d_offset = bsl::move(rhs.d_offset);
        d_count  = bsl::move(rhs.d_count);
    }

    return *this;
}
#endif

void ListMessages::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_appId);
    bdlat_ValueTypeFunctions::reset(&d_offset);
    bdlat_ValueTypeFunctions::reset(&d_count);
}

// ACCESSORS

bsl::ostream&
ListMessages::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("appId", this->appId());
    printer.printAttribute("offset", this->offset());
    printer.printAttribute("count", this->count());
    printer.end();
    return stream;
}

// --------------
// class Locality
// --------------

// CONSTANTS

const char Locality::CLASS_NAME[] = "Locality";

const bdlat_EnumeratorInfo Locality::ENUMERATOR_INFO_ARRAY[] = {
    {Locality::REMOTE, "remote", sizeof("remote") - 1, ""},
    {Locality::LOCAL, "local", sizeof("local") - 1, ""},
    {Locality::MEMBER, "member", sizeof("member") - 1, ""}};

// CLASS METHODS

int Locality::fromInt(Locality::Value* result, int number)
{
    switch (number) {
    case Locality::REMOTE:
    case Locality::LOCAL:
    case Locality::MEMBER:
        *result = static_cast<Locality::Value>(number);
        return 0;
    default: return -1;
    }
}

int Locality::fromString(Locality::Value* result,
                         const char*      string,
                         int              stringLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            Locality::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<Locality::Value>(enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* Locality::toString(Locality::Value value)
{
    switch (value) {
    case REMOTE: {
        return "remote";
    }
    case LOCAL: {
        return "local";
    }
    case MEMBER: {
        return "member";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -------------
// class Message
// -------------

// CONSTANTS

const char Message::CLASS_NAME[] = "Message";

const bdlat_AttributeInfo Message::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_GUID,
     "guid",
     sizeof("guid") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_SIZE_BYTES,
     "sizeBytes",
     sizeof("sizeBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ARRIVAL_TIMESTAMP,
     "arrivalTimestamp",
     sizeof("arrivalTimestamp") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Message::lookupAttributeInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Message::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Message::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_GUID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID];
    case ATTRIBUTE_ID_SIZE_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SIZE_BYTES];
    case ATTRIBUTE_ID_ARRIVAL_TIMESTAMP:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ARRIVAL_TIMESTAMP];
    default: return 0;
    }
}

// CREATORS

Message::Message(bslma::Allocator* basicAllocator)
: d_sizeBytes()
, d_guid(basicAllocator)
, d_arrivalTimestamp()
{
}

Message::Message(const Message& original, bslma::Allocator* basicAllocator)
: d_sizeBytes(original.d_sizeBytes)
, d_guid(original.d_guid, basicAllocator)
, d_arrivalTimestamp(original.d_arrivalTimestamp)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Message::Message(Message&& original) noexcept
: d_sizeBytes(bsl::move(original.d_sizeBytes)),
  d_guid(bsl::move(original.d_guid)),
  d_arrivalTimestamp(bsl::move(original.d_arrivalTimestamp))
{
}

Message::Message(Message&& original, bslma::Allocator* basicAllocator)
: d_sizeBytes(bsl::move(original.d_sizeBytes))
, d_guid(bsl::move(original.d_guid), basicAllocator)
, d_arrivalTimestamp(bsl::move(original.d_arrivalTimestamp))
{
}
#endif

Message::~Message()
{
}

// MANIPULATORS

Message& Message::operator=(const Message& rhs)
{
    if (this != &rhs) {
        d_guid             = rhs.d_guid;
        d_sizeBytes        = rhs.d_sizeBytes;
        d_arrivalTimestamp = rhs.d_arrivalTimestamp;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Message& Message::operator=(Message&& rhs)
{
    if (this != &rhs) {
        d_guid             = bsl::move(rhs.d_guid);
        d_sizeBytes        = bsl::move(rhs.d_sizeBytes);
        d_arrivalTimestamp = bsl::move(rhs.d_arrivalTimestamp);
    }

    return *this;
}
#endif

void Message::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_guid);
    bdlat_ValueTypeFunctions::reset(&d_sizeBytes);
    bdlat_ValueTypeFunctions::reset(&d_arrivalTimestamp);
}

// ACCESSORS

bsl::ostream&
Message::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("guid", this->guid());
    printer.printAttribute("sizeBytes", this->sizeBytes());
    printer.printAttribute("arrivalTimestamp", this->arrivalTimestamp());
    printer.end();
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

// -------------------
// class PrimaryStatus
// -------------------

// CONSTANTS

const char PrimaryStatus::CLASS_NAME[] = "PrimaryStatus";

const bdlat_EnumeratorInfo PrimaryStatus::ENUMERATOR_INFO_ARRAY[] = {
    {PrimaryStatus::UNDEFINED, "UNDEFINED", sizeof("UNDEFINED") - 1, ""},
    {PrimaryStatus::PASSIVE, "PASSIVE", sizeof("PASSIVE") - 1, ""},
    {PrimaryStatus::ACTIVE, "ACTIVE", sizeof("ACTIVE") - 1, ""}};

// CLASS METHODS

int PrimaryStatus::fromInt(PrimaryStatus::Value* result, int number)
{
    switch (number) {
    case PrimaryStatus::UNDEFINED:
    case PrimaryStatus::PASSIVE:
    case PrimaryStatus::ACTIVE:
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
    case UNDEFINED: {
        return "UNDEFINED";
    }
    case PASSIVE: {
        return "PASSIVE";
    }
    case ACTIVE: {
        return "ACTIVE";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ------------------------
// class PurgedQueueDetails
// ------------------------

// CONSTANTS

const char PurgedQueueDetails::CLASS_NAME[] = "PurgedQueueDetails";

const bdlat_AttributeInfo PurgedQueueDetails::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_QUEUE_URI,
     "queueUri",
     sizeof("queueUri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_APP_KEY,
     "appKey",
     sizeof("appKey") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NUM_MESSAGES_PURGED,
     "numMessagesPurged",
     sizeof("numMessagesPurged") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_BYTES_PURGED,
     "numBytesPurged",
     sizeof("numBytesPurged") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
PurgedQueueDetails::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PurgedQueueDetails::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PurgedQueueDetails::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_QUEUE_URI:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URI];
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_APP_KEY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_KEY];
    case ATTRIBUTE_ID_NUM_MESSAGES_PURGED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_MESSAGES_PURGED];
    case ATTRIBUTE_ID_NUM_BYTES_PURGED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_BYTES_PURGED];
    default: return 0;
    }
}

// CREATORS

PurgedQueueDetails::PurgedQueueDetails(bslma::Allocator* basicAllocator)
: d_numMessagesPurged()
, d_numBytesPurged()
, d_queueUri(basicAllocator)
, d_appId(basicAllocator)
, d_appKey(basicAllocator)
{
}

PurgedQueueDetails::PurgedQueueDetails(const PurgedQueueDetails& original,
                                       bslma::Allocator* basicAllocator)
: d_numMessagesPurged(original.d_numMessagesPurged)
, d_numBytesPurged(original.d_numBytesPurged)
, d_queueUri(original.d_queueUri, basicAllocator)
, d_appId(original.d_appId, basicAllocator)
, d_appKey(original.d_appKey, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgedQueueDetails::PurgedQueueDetails(PurgedQueueDetails&& original) noexcept
: d_numMessagesPurged(bsl::move(original.d_numMessagesPurged)),
  d_numBytesPurged(bsl::move(original.d_numBytesPurged)),
  d_queueUri(bsl::move(original.d_queueUri)),
  d_appId(bsl::move(original.d_appId)),
  d_appKey(bsl::move(original.d_appKey))
{
}

PurgedQueueDetails::PurgedQueueDetails(PurgedQueueDetails&& original,
                                       bslma::Allocator*    basicAllocator)
: d_numMessagesPurged(bsl::move(original.d_numMessagesPurged))
, d_numBytesPurged(bsl::move(original.d_numBytesPurged))
, d_queueUri(bsl::move(original.d_queueUri), basicAllocator)
, d_appId(bsl::move(original.d_appId), basicAllocator)
, d_appKey(bsl::move(original.d_appKey), basicAllocator)
{
}
#endif

PurgedQueueDetails::~PurgedQueueDetails()
{
}

// MANIPULATORS

PurgedQueueDetails&
PurgedQueueDetails::operator=(const PurgedQueueDetails& rhs)
{
    if (this != &rhs) {
        d_queueUri          = rhs.d_queueUri;
        d_appId             = rhs.d_appId;
        d_appKey            = rhs.d_appKey;
        d_numMessagesPurged = rhs.d_numMessagesPurged;
        d_numBytesPurged    = rhs.d_numBytesPurged;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgedQueueDetails& PurgedQueueDetails::operator=(PurgedQueueDetails&& rhs)
{
    if (this != &rhs) {
        d_queueUri          = bsl::move(rhs.d_queueUri);
        d_appId             = bsl::move(rhs.d_appId);
        d_appKey            = bsl::move(rhs.d_appKey);
        d_numMessagesPurged = bsl::move(rhs.d_numMessagesPurged);
        d_numBytesPurged    = bsl::move(rhs.d_numBytesPurged);
    }

    return *this;
}
#endif

void PurgedQueueDetails::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_queueUri);
    bdlat_ValueTypeFunctions::reset(&d_appId);
    bdlat_ValueTypeFunctions::reset(&d_appKey);
    bdlat_ValueTypeFunctions::reset(&d_numMessagesPurged);
    bdlat_ValueTypeFunctions::reset(&d_numBytesPurged);
}

// ACCESSORS

bsl::ostream& PurgedQueueDetails::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queueUri", this->queueUri());
    printer.printAttribute("appId", this->appId());
    printer.printAttribute("appKey", this->appKey());
    printer.printAttribute("numMessagesPurged", this->numMessagesPurged());
    printer.printAttribute("numBytesPurged", this->numBytesPurged());
    printer.end();
    return stream;
}

// -------------------------------
// class RelayQueueEngineSubStream
// -------------------------------

// CONSTANTS

const char RelayQueueEngineSubStream::CLASS_NAME[] =
    "RelayQueueEngineSubStream";

const bdlat_AttributeInfo RelayQueueEngineSubStream::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_APP_KEY,
     "appKey",
     sizeof("appKey") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NUM_MESSAGES,
     "numMessages",
     sizeof("numMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
RelayQueueEngineSubStream::lookupAttributeInfo(const char* name,
                                               int         nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RelayQueueEngineSubStream::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
RelayQueueEngineSubStream::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_APP_KEY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_KEY];
    case ATTRIBUTE_ID_NUM_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_MESSAGES];
    default: return 0;
    }
}

// CREATORS

RelayQueueEngineSubStream::RelayQueueEngineSubStream(
    bslma::Allocator* basicAllocator)
: d_appId(basicAllocator)
, d_appKey(basicAllocator)
, d_numMessages()
{
}

RelayQueueEngineSubStream::RelayQueueEngineSubStream(
    const RelayQueueEngineSubStream& original,
    bslma::Allocator*                basicAllocator)
: d_appId(original.d_appId, basicAllocator)
, d_appKey(original.d_appKey, basicAllocator)
, d_numMessages(original.d_numMessages)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RelayQueueEngineSubStream::RelayQueueEngineSubStream(
    RelayQueueEngineSubStream&& original) noexcept
: d_appId(bsl::move(original.d_appId)),
  d_appKey(bsl::move(original.d_appKey)),
  d_numMessages(bsl::move(original.d_numMessages))
{
}

RelayQueueEngineSubStream::RelayQueueEngineSubStream(
    RelayQueueEngineSubStream&& original,
    bslma::Allocator*           basicAllocator)
: d_appId(bsl::move(original.d_appId), basicAllocator)
, d_appKey(bsl::move(original.d_appKey), basicAllocator)
, d_numMessages(bsl::move(original.d_numMessages))
{
}
#endif

RelayQueueEngineSubStream::~RelayQueueEngineSubStream()
{
}

// MANIPULATORS

RelayQueueEngineSubStream&
RelayQueueEngineSubStream::operator=(const RelayQueueEngineSubStream& rhs)
{
    if (this != &rhs) {
        d_appId       = rhs.d_appId;
        d_appKey      = rhs.d_appKey;
        d_numMessages = rhs.d_numMessages;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RelayQueueEngineSubStream&
RelayQueueEngineSubStream::operator=(RelayQueueEngineSubStream&& rhs)
{
    if (this != &rhs) {
        d_appId       = bsl::move(rhs.d_appId);
        d_appKey      = bsl::move(rhs.d_appKey);
        d_numMessages = bsl::move(rhs.d_numMessages);
    }

    return *this;
}
#endif

void RelayQueueEngineSubStream::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_appId);
    bdlat_ValueTypeFunctions::reset(&d_appKey);
    bdlat_ValueTypeFunctions::reset(&d_numMessages);
}

// ACCESSORS

bsl::ostream& RelayQueueEngineSubStream::print(bsl::ostream& stream,
                                               int           level,
                                               int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("appId", this->appId());
    printer.printAttribute("appKey", this->appKey());
    printer.printAttribute("numMessages", this->numMessages());
    printer.end();
    return stream;
}

// ----------------------
// class RemoteStreamInfo
// ----------------------

// CONSTANTS

const char RemoteStreamInfo::CLASS_NAME[] = "RemoteStreamInfo";

const bdlat_AttributeInfo RemoteStreamInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ID, "id", sizeof("id") - 1, "", bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STATE,
     "state",
     sizeof("state") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_GEN_COUNT,
     "genCount",
     sizeof("genCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
RemoteStreamInfo::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RemoteStreamInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RemoteStreamInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID];
    case ATTRIBUTE_ID_STATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATE];
    case ATTRIBUTE_ID_GEN_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GEN_COUNT];
    default: return 0;
    }
}

// CREATORS

RemoteStreamInfo::RemoteStreamInfo(bslma::Allocator* basicAllocator)
: d_genCount()
, d_state(basicAllocator)
, d_id()
{
}

RemoteStreamInfo::RemoteStreamInfo(const RemoteStreamInfo& original,
                                   bslma::Allocator*       basicAllocator)
: d_genCount(original.d_genCount)
, d_state(original.d_state, basicAllocator)
, d_id(original.d_id)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RemoteStreamInfo::RemoteStreamInfo(RemoteStreamInfo&& original) noexcept
: d_genCount(bsl::move(original.d_genCount)),
  d_state(bsl::move(original.d_state)),
  d_id(bsl::move(original.d_id))
{
}

RemoteStreamInfo::RemoteStreamInfo(RemoteStreamInfo&& original,
                                   bslma::Allocator*  basicAllocator)
: d_genCount(bsl::move(original.d_genCount))
, d_state(bsl::move(original.d_state), basicAllocator)
, d_id(bsl::move(original.d_id))
{
}
#endif

RemoteStreamInfo::~RemoteStreamInfo()
{
}

// MANIPULATORS

RemoteStreamInfo& RemoteStreamInfo::operator=(const RemoteStreamInfo& rhs)
{
    if (this != &rhs) {
        d_id       = rhs.d_id;
        d_state    = rhs.d_state;
        d_genCount = rhs.d_genCount;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RemoteStreamInfo& RemoteStreamInfo::operator=(RemoteStreamInfo&& rhs)
{
    if (this != &rhs) {
        d_id       = bsl::move(rhs.d_id);
        d_state    = bsl::move(rhs.d_state);
        d_genCount = bsl::move(rhs.d_genCount);
    }

    return *this;
}
#endif

void RemoteStreamInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_id);
    bdlat_ValueTypeFunctions::reset(&d_state);
    bdlat_ValueTypeFunctions::reset(&d_genCount);
}

// ACCESSORS

bsl::ostream& RemoteStreamInfo::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("id", this->id());
    printer.printAttribute("state", this->state());
    printer.printAttribute("genCount", this->genCount());
    printer.end();
    return stream;
}

// -------------------------------
// class ResourceUsageMonitorState
// -------------------------------

// CONSTANTS

const char ResourceUsageMonitorState::CLASS_NAME[] =
    "ResourceUsageMonitorState";

const bdlat_EnumeratorInfo ResourceUsageMonitorState::ENUMERATOR_INFO_ARRAY[] =
    {{ResourceUsageMonitorState::STATE_NORMAL,
      "STATE_NORMAL",
      sizeof("STATE_NORMAL") - 1,
      ""},
     {ResourceUsageMonitorState::STATE_HIGH_WATERMARK,
      "STATE_HIGH_WATERMARK",
      sizeof("STATE_HIGH_WATERMARK") - 1,
      ""},
     {ResourceUsageMonitorState::STATE_FULL,
      "STATE_FULL",
      sizeof("STATE_FULL") - 1,
      ""}};

// CLASS METHODS

int ResourceUsageMonitorState::fromInt(
    ResourceUsageMonitorState::Value* result,
    int                               number)
{
    switch (number) {
    case ResourceUsageMonitorState::STATE_NORMAL:
    case ResourceUsageMonitorState::STATE_HIGH_WATERMARK:
    case ResourceUsageMonitorState::STATE_FULL:
        *result = static_cast<ResourceUsageMonitorState::Value>(number);
        return 0;
    default: return -1;
    }
}

int ResourceUsageMonitorState::fromString(
    ResourceUsageMonitorState::Value* result,
    const char*                       string,
    int                               stringLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            ResourceUsageMonitorState::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<ResourceUsageMonitorState::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char*
ResourceUsageMonitorState::toString(ResourceUsageMonitorState::Value value)
{
    switch (value) {
    case STATE_NORMAL: {
        return "STATE_NORMAL";
    }
    case STATE_HIGH_WATERMARK: {
        return "STATE_HIGH_WATERMARK";
    }
    case STATE_FULL: {
        return "STATE_FULL";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -------------------
// class RouteResponse
// -------------------

// CONSTANTS

const char RouteResponse::CLASS_NAME[] = "RouteResponse";

const bdlat_AttributeInfo RouteResponse::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SOURCE_NODE_DESCRIPTION,
     "sourceNodeDescription",
     sizeof("sourceNodeDescription") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_RESPONSE,
     "response",
     sizeof("response") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* RouteResponse::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RouteResponse::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RouteResponse::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SOURCE_NODE_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SOURCE_NODE_DESCRIPTION];
    case ATTRIBUTE_ID_RESPONSE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESPONSE];
    default: return 0;
    }
}

// CREATORS

RouteResponse::RouteResponse(bslma::Allocator* basicAllocator)
: d_sourceNodeDescription(basicAllocator)
, d_response(basicAllocator)
{
}

RouteResponse::RouteResponse(const RouteResponse& original,
                             bslma::Allocator*    basicAllocator)
: d_sourceNodeDescription(original.d_sourceNodeDescription, basicAllocator)
, d_response(original.d_response, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouteResponse::RouteResponse(RouteResponse&& original) noexcept
: d_sourceNodeDescription(bsl::move(original.d_sourceNodeDescription)),
  d_response(bsl::move(original.d_response))
{
}

RouteResponse::RouteResponse(RouteResponse&&   original,
                             bslma::Allocator* basicAllocator)
: d_sourceNodeDescription(bsl::move(original.d_sourceNodeDescription),
                          basicAllocator)
, d_response(bsl::move(original.d_response), basicAllocator)
{
}
#endif

RouteResponse::~RouteResponse()
{
}

// MANIPULATORS

RouteResponse& RouteResponse::operator=(const RouteResponse& rhs)
{
    if (this != &rhs) {
        d_sourceNodeDescription = rhs.d_sourceNodeDescription;
        d_response              = rhs.d_response;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouteResponse& RouteResponse::operator=(RouteResponse&& rhs)
{
    if (this != &rhs) {
        d_sourceNodeDescription = bsl::move(rhs.d_sourceNodeDescription);
        d_response              = bsl::move(rhs.d_response);
    }

    return *this;
}
#endif

void RouteResponse::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sourceNodeDescription);
    bdlat_ValueTypeFunctions::reset(&d_response);
}

// ACCESSORS

bsl::ostream&
RouteResponse::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sourceNodeDescription",
                           this->sourceNodeDescription());
    printer.printAttribute("response", this->response());
    printer.end();
    return stream;
}

// -------------------------
// class StorageQueueCommand
// -------------------------

// CONSTANTS

const char StorageQueueCommand::CLASS_NAME[] = "StorageQueueCommand";

const bdlat_SelectionInfo StorageQueueCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_PURGE_APP_ID,
     "purgeAppId",
     sizeof("purgeAppId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_SelectionInfo*
StorageQueueCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            StorageQueueCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* StorageQueueCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_PURGE_APP_ID:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGE_APP_ID];
    default: return 0;
    }
}

// CREATORS

StorageQueueCommand::StorageQueueCommand(const StorageQueueCommand& original,
                                         bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        new (d_purgeAppId.buffer())
            bsl::string(original.d_purgeAppId.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageQueueCommand::StorageQueueCommand(StorageQueueCommand&& original)
    noexcept : d_selectionId(original.d_selectionId),
               d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        new (d_purgeAppId.buffer())
            bsl::string(bsl::move(original.d_purgeAppId.object()),
                        d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

StorageQueueCommand::StorageQueueCommand(StorageQueueCommand&& original,
                                         bslma::Allocator*     basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        new (d_purgeAppId.buffer())
            bsl::string(bsl::move(original.d_purgeAppId.object()),
                        d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

StorageQueueCommand&
StorageQueueCommand::operator=(const StorageQueueCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_PURGE_APP_ID: {
            makePurgeAppId(rhs.d_purgeAppId.object());
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
StorageQueueCommand& StorageQueueCommand::operator=(StorageQueueCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_PURGE_APP_ID: {
            makePurgeAppId(bsl::move(rhs.d_purgeAppId.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void StorageQueueCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        typedef bsl::string Type;
        d_purgeAppId.object().~Type();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int StorageQueueCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        makePurgeAppId();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int StorageQueueCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

bsl::string& StorageQueueCommand::makePurgeAppId()
{
    if (SELECTION_ID_PURGE_APP_ID == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_purgeAppId.object());
    }
    else {
        reset();
        new (d_purgeAppId.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_PURGE_APP_ID;
    }

    return d_purgeAppId.object();
}

bsl::string& StorageQueueCommand::makePurgeAppId(const bsl::string& value)
{
    if (SELECTION_ID_PURGE_APP_ID == d_selectionId) {
        d_purgeAppId.object() = value;
    }
    else {
        reset();
        new (d_purgeAppId.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_PURGE_APP_ID;
    }

    return d_purgeAppId.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& StorageQueueCommand::makePurgeAppId(bsl::string&& value)
{
    if (SELECTION_ID_PURGE_APP_ID == d_selectionId) {
        d_purgeAppId.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_purgeAppId.buffer())
            bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_PURGE_APP_ID;
    }

    return d_purgeAppId.object();
}
#endif

// ACCESSORS

bsl::ostream& StorageQueueCommand::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        printer.printAttribute("purgeAppId", d_purgeAppId.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* StorageQueueCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGE_APP_ID].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------------
// class StorageQueueInfo
// ----------------------

// CONSTANTS

const char StorageQueueInfo::CLASS_NAME[] = "StorageQueueInfo";

const bdlat_AttributeInfo StorageQueueInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_QUEUE_URI,
     "queueUri",
     sizeof("queueUri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_QUEUE_KEY,
     "queueKey",
     sizeof("queueKey") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_MESSAGES,
     "numMessages",
     sizeof("numMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_BYTES,
     "numBytes",
     sizeof("numBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_IS_PERSISTENT,
     "isPersistent",
     sizeof("isPersistent") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_INTERNAL_QUEUE_ID,
     "internalQueueId",
     sizeof("internalQueueId") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
StorageQueueInfo::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 7; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StorageQueueInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StorageQueueInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_QUEUE_URI:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URI];
    case ATTRIBUTE_ID_QUEUE_KEY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_KEY];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_NUM_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_MESSAGES];
    case ATTRIBUTE_ID_NUM_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_BYTES];
    case ATTRIBUTE_ID_IS_PERSISTENT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_PERSISTENT];
    case ATTRIBUTE_ID_INTERNAL_QUEUE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INTERNAL_QUEUE_ID];
    default: return 0;
    }
}

// CREATORS

StorageQueueInfo::StorageQueueInfo(bslma::Allocator* basicAllocator)
: d_numMessages()
, d_numBytes()
, d_queueUri(basicAllocator)
, d_queueKey(basicAllocator)
, d_internalQueueId()
, d_partitionId()
, d_isPersistent()
{
}

StorageQueueInfo::StorageQueueInfo(const StorageQueueInfo& original,
                                   bslma::Allocator*       basicAllocator)
: d_numMessages(original.d_numMessages)
, d_numBytes(original.d_numBytes)
, d_queueUri(original.d_queueUri, basicAllocator)
, d_queueKey(original.d_queueKey, basicAllocator)
, d_internalQueueId(original.d_internalQueueId)
, d_partitionId(original.d_partitionId)
, d_isPersistent(original.d_isPersistent)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageQueueInfo::StorageQueueInfo(StorageQueueInfo&& original) noexcept
: d_numMessages(bsl::move(original.d_numMessages)),
  d_numBytes(bsl::move(original.d_numBytes)),
  d_queueUri(bsl::move(original.d_queueUri)),
  d_queueKey(bsl::move(original.d_queueKey)),
  d_internalQueueId(bsl::move(original.d_internalQueueId)),
  d_partitionId(bsl::move(original.d_partitionId)),
  d_isPersistent(bsl::move(original.d_isPersistent))
{
}

StorageQueueInfo::StorageQueueInfo(StorageQueueInfo&& original,
                                   bslma::Allocator*  basicAllocator)
: d_numMessages(bsl::move(original.d_numMessages))
, d_numBytes(bsl::move(original.d_numBytes))
, d_queueUri(bsl::move(original.d_queueUri), basicAllocator)
, d_queueKey(bsl::move(original.d_queueKey), basicAllocator)
, d_internalQueueId(bsl::move(original.d_internalQueueId))
, d_partitionId(bsl::move(original.d_partitionId))
, d_isPersistent(bsl::move(original.d_isPersistent))
{
}
#endif

StorageQueueInfo::~StorageQueueInfo()
{
}

// MANIPULATORS

StorageQueueInfo& StorageQueueInfo::operator=(const StorageQueueInfo& rhs)
{
    if (this != &rhs) {
        d_queueUri        = rhs.d_queueUri;
        d_queueKey        = rhs.d_queueKey;
        d_partitionId     = rhs.d_partitionId;
        d_numMessages     = rhs.d_numMessages;
        d_numBytes        = rhs.d_numBytes;
        d_isPersistent    = rhs.d_isPersistent;
        d_internalQueueId = rhs.d_internalQueueId;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageQueueInfo& StorageQueueInfo::operator=(StorageQueueInfo&& rhs)
{
    if (this != &rhs) {
        d_queueUri        = bsl::move(rhs.d_queueUri);
        d_queueKey        = bsl::move(rhs.d_queueKey);
        d_partitionId     = bsl::move(rhs.d_partitionId);
        d_numMessages     = bsl::move(rhs.d_numMessages);
        d_numBytes        = bsl::move(rhs.d_numBytes);
        d_isPersistent    = bsl::move(rhs.d_isPersistent);
        d_internalQueueId = bsl::move(rhs.d_internalQueueId);
    }

    return *this;
}
#endif

void StorageQueueInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_queueUri);
    bdlat_ValueTypeFunctions::reset(&d_queueKey);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_numMessages);
    bdlat_ValueTypeFunctions::reset(&d_numBytes);
    bdlat_ValueTypeFunctions::reset(&d_isPersistent);
    bdlat_ValueTypeFunctions::reset(&d_internalQueueId);
}

// ACCESSORS

bsl::ostream& StorageQueueInfo::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queueUri", this->queueUri());
    printer.printAttribute("queueKey", this->queueKey());
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("numMessages", this->numMessages());
    printer.printAttribute("numBytes", this->numBytes());
    printer.printAttribute("isPersistent", this->isPersistent());
    printer.printAttribute("internalQueueId", this->internalQueueId());
    printer.end();
    return stream;
}

// -----------
// class SubId
// -----------

// CONSTANTS

const char SubId::CLASS_NAME[] = "SubId";

const bdlat_AttributeInfo SubId::ATTRIBUTE_INFO_ARRAY[] = {
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

const bdlat_AttributeInfo* SubId::lookupAttributeInfo(const char* name,
                                                      int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            SubId::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* SubId::lookupAttributeInfo(int id)
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

SubId::SubId(bslma::Allocator* basicAllocator)
: d_appId(basicAllocator)
, d_subId()
{
}

SubId::SubId(const SubId& original, bslma::Allocator* basicAllocator)
: d_appId(original.d_appId, basicAllocator)
, d_subId(original.d_subId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SubId::SubId(SubId&& original) noexcept : d_appId(bsl::move(original.d_appId)),
                                          d_subId(bsl::move(original.d_subId))
{
}

SubId::SubId(SubId&& original, bslma::Allocator* basicAllocator)
: d_appId(bsl::move(original.d_appId), basicAllocator)
, d_subId(bsl::move(original.d_subId))
{
}
#endif

SubId::~SubId()
{
}

// MANIPULATORS

SubId& SubId::operator=(const SubId& rhs)
{
    if (this != &rhs) {
        d_subId = rhs.d_subId;
        d_appId = rhs.d_appId;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SubId& SubId::operator=(SubId&& rhs)
{
    if (this != &rhs) {
        d_subId = bsl::move(rhs.d_subId);
        d_appId = bsl::move(rhs.d_appId);
    }

    return *this;
}
#endif

void SubId::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_subId);
    bdlat_ValueTypeFunctions::reset(&d_appId);
}

// ACCESSORS

bsl::ostream&
SubId::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("subId", this->subId());
    printer.printAttribute("appId", this->appId());
    printer.end();
    return stream;
}

// ----------------
// class Subscriber
// ----------------

// CONSTANTS

const char Subscriber::CLASS_NAME[] = "Subscriber";

const bdlat_AttributeInfo Subscriber::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_DOWNSTREAM_SUB_QUEUE_ID,
     "downstreamSubQueueId",
     sizeof("downstreamSubQueueId") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* Subscriber::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Subscriber::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Subscriber::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_DOWNSTREAM_SUB_QUEUE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOWNSTREAM_SUB_QUEUE_ID];
    default: return 0;
    }
}

// CREATORS

Subscriber::Subscriber()
: d_downstreamSubQueueId()
{
}

// MANIPULATORS

void Subscriber::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_downstreamSubQueueId);
}

// ACCESSORS

bsl::ostream&
Subscriber::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("downstreamSubQueueId",
                           this->downstreamSubQueueId());
    printer.end();
    return stream;
}

// ------------------------
// class UninitializedQueue
// ------------------------

// CONSTANTS

const char UninitializedQueue::CLASS_NAME[] = "UninitializedQueue";

// CLASS METHODS

const bdlat_AttributeInfo*
UninitializedQueue::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* UninitializedQueue::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void UninitializedQueue::reset()
{
}

// ACCESSORS

bsl::ostream& UninitializedQueue::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// --------------------
// class VirtualStorage
// --------------------

// CONSTANTS

const char VirtualStorage::CLASS_NAME[] = "VirtualStorage";

const bdlat_AttributeInfo VirtualStorage::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_APP_KEY,
     "appKey",
     sizeof("appKey") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NUM_MESSAGES,
     "numMessages",
     sizeof("numMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
VirtualStorage::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            VirtualStorage::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* VirtualStorage::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_APP_KEY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_KEY];
    case ATTRIBUTE_ID_NUM_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_MESSAGES];
    default: return 0;
    }
}

// CREATORS

VirtualStorage::VirtualStorage(bslma::Allocator* basicAllocator)
: d_appId(basicAllocator)
, d_appKey(basicAllocator)
, d_numMessages()
{
}

VirtualStorage::VirtualStorage(const VirtualStorage& original,
                               bslma::Allocator*     basicAllocator)
: d_appId(original.d_appId, basicAllocator)
, d_appKey(original.d_appKey, basicAllocator)
, d_numMessages(original.d_numMessages)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
VirtualStorage::VirtualStorage(VirtualStorage&& original) noexcept
: d_appId(bsl::move(original.d_appId)),
  d_appKey(bsl::move(original.d_appKey)),
  d_numMessages(bsl::move(original.d_numMessages))
{
}

VirtualStorage::VirtualStorage(VirtualStorage&&  original,
                               bslma::Allocator* basicAllocator)
: d_appId(bsl::move(original.d_appId), basicAllocator)
, d_appKey(bsl::move(original.d_appKey), basicAllocator)
, d_numMessages(bsl::move(original.d_numMessages))
{
}
#endif

VirtualStorage::~VirtualStorage()
{
}

// MANIPULATORS

VirtualStorage& VirtualStorage::operator=(const VirtualStorage& rhs)
{
    if (this != &rhs) {
        d_appId       = rhs.d_appId;
        d_appKey      = rhs.d_appKey;
        d_numMessages = rhs.d_numMessages;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
VirtualStorage& VirtualStorage::operator=(VirtualStorage&& rhs)
{
    if (this != &rhs) {
        d_appId       = bsl::move(rhs.d_appId);
        d_appKey      = bsl::move(rhs.d_appKey);
        d_numMessages = bsl::move(rhs.d_numMessages);
    }

    return *this;
}
#endif

void VirtualStorage::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_appId);
    bdlat_ValueTypeFunctions::reset(&d_appKey);
    bdlat_ValueTypeFunctions::reset(&d_numMessages);
}

// ACCESSORS

bsl::ostream& VirtualStorage::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("appId", this->appId());
    printer.printAttribute("appKey", this->appKey());
    printer.printAttribute("numMessages", this->numMessages());
    printer.end();
    return stream;
}

// ----------
// class Void
// ----------

// CONSTANTS

const char Void::CLASS_NAME[] = "Void";

// CLASS METHODS

const bdlat_AttributeInfo* Void::lookupAttributeInfo(const char* name,
                                                     int         nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* Void::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void Void::reset()
{
}

// ACCESSORS

bsl::ostream& Void::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// -------------------
// class ActiveFileSet
// -------------------

// CONSTANTS

const char ActiveFileSet::CLASS_NAME[] = "ActiveFileSet";

const bdlat_AttributeInfo ActiveFileSet::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_DATA_FILE,
     "dataFile",
     sizeof("dataFile") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_JOURNAL_FILE,
     "journalFile",
     sizeof("journalFile") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QLIST_FILE,
     "qlistFile",
     sizeof("qlistFile") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* ActiveFileSet::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ActiveFileSet::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ActiveFileSet::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_DATA_FILE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_FILE];
    case ATTRIBUTE_ID_JOURNAL_FILE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_JOURNAL_FILE];
    case ATTRIBUTE_ID_QLIST_FILE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QLIST_FILE];
    default: return 0;
    }
}

// CREATORS

ActiveFileSet::ActiveFileSet()
: d_dataFile()
, d_journalFile()
, d_qlistFile()
{
}

// MANIPULATORS

void ActiveFileSet::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_dataFile);
    bdlat_ValueTypeFunctions::reset(&d_journalFile);
    bdlat_ValueTypeFunctions::reset(&d_qlistFile);
}

// ACCESSORS

bsl::ostream&
ActiveFileSet::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("dataFile", this->dataFile());
    printer.printAttribute("journalFile", this->journalFile());
    printer.printAttribute("qlistFile", this->qlistFile());
    printer.end();
    return stream;
}

// -------------------------
// class BrokerConfigCommand
// -------------------------

// CONSTANTS

const char BrokerConfigCommand::CLASS_NAME[] = "BrokerConfigCommand";

const bdlat_SelectionInfo BrokerConfigCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_DUMP,
     "dump",
     sizeof("dump") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
BrokerConfigCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            BrokerConfigCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* BrokerConfigCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_DUMP: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP];
    default: return 0;
    }
}

// CREATORS

BrokerConfigCommand::BrokerConfigCommand(const BrokerConfigCommand& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_DUMP: {
        new (d_dump.buffer()) Void(original.d_dump.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerConfigCommand::BrokerConfigCommand(BrokerConfigCommand&& original)
    noexcept : d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_DUMP: {
        new (d_dump.buffer()) Void(bsl::move(original.d_dump.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

BrokerConfigCommand&
BrokerConfigCommand::operator=(const BrokerConfigCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DUMP: {
            makeDump(rhs.d_dump.object());
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
BrokerConfigCommand& BrokerConfigCommand::operator=(BrokerConfigCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DUMP: {
            makeDump(bsl::move(rhs.d_dump.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void BrokerConfigCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_DUMP: {
        d_dump.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int BrokerConfigCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_DUMP: {
        makeDump();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int BrokerConfigCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& BrokerConfigCommand::makeDump()
{
    if (SELECTION_ID_DUMP == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_dump.object());
    }
    else {
        reset();
        new (d_dump.buffer()) Void();
        d_selectionId = SELECTION_ID_DUMP;
    }

    return d_dump.object();
}

Void& BrokerConfigCommand::makeDump(const Void& value)
{
    if (SELECTION_ID_DUMP == d_selectionId) {
        d_dump.object() = value;
    }
    else {
        reset();
        new (d_dump.buffer()) Void(value);
        d_selectionId = SELECTION_ID_DUMP;
    }

    return d_dump.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& BrokerConfigCommand::makeDump(Void&& value)
{
    if (SELECTION_ID_DUMP == d_selectionId) {
        d_dump.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_dump.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_DUMP;
    }

    return d_dump.object();
}
#endif

// ACCESSORS

bsl::ostream& BrokerConfigCommand::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_DUMP: {
        printer.printAttribute("dump", d_dump.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* BrokerConfigCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_DUMP:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------
// class ClearCache
// ----------------

// CONSTANTS

const char ClearCache::CLASS_NAME[] = "ClearCache";

const bdlat_SelectionInfo ClearCache::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_DOMAIN,
     "domain",
     sizeof("domain") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {SELECTION_ID_ALL,
     "all",
     sizeof("all") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* ClearCache::lookupSelectionInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ClearCache::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ClearCache::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_DOMAIN:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN];
    case SELECTION_ID_ALL: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ALL];
    default: return 0;
    }
}

// CREATORS

ClearCache::ClearCache(const ClearCache& original,
                       bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            bsl::string(original.d_domain.object(), d_allocator_p);
    } break;
    case SELECTION_ID_ALL: {
        new (d_all.buffer()) Void(original.d_all.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClearCache::ClearCache(ClearCache&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            bsl::string(bsl::move(original.d_domain.object()), d_allocator_p);
    } break;
    case SELECTION_ID_ALL: {
        new (d_all.buffer()) Void(bsl::move(original.d_all.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ClearCache::ClearCache(ClearCache&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            bsl::string(bsl::move(original.d_domain.object()), d_allocator_p);
    } break;
    case SELECTION_ID_ALL: {
        new (d_all.buffer()) Void(bsl::move(original.d_all.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ClearCache& ClearCache::operator=(const ClearCache& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DOMAIN: {
            makeDomain(rhs.d_domain.object());
        } break;
        case SELECTION_ID_ALL: {
            makeAll(rhs.d_all.object());
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
ClearCache& ClearCache::operator=(ClearCache&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DOMAIN: {
            makeDomain(bsl::move(rhs.d_domain.object()));
        } break;
        case SELECTION_ID_ALL: {
            makeAll(bsl::move(rhs.d_all.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ClearCache::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        typedef bsl::string Type;
        d_domain.object().~Type();
    } break;
    case SELECTION_ID_ALL: {
        d_all.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ClearCache::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_DOMAIN: {
        makeDomain();
    } break;
    case SELECTION_ID_ALL: {
        makeAll();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ClearCache::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

bsl::string& ClearCache::makeDomain()
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domain.object());
    }
    else {
        reset();
        new (d_domain.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}

bsl::string& ClearCache::makeDomain(const bsl::string& value)
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        d_domain.object() = value;
    }
    else {
        reset();
        new (d_domain.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& ClearCache::makeDomain(bsl::string&& value)
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        d_domain.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domain.buffer()) bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}
#endif

Void& ClearCache::makeAll()
{
    if (SELECTION_ID_ALL == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_all.object());
    }
    else {
        reset();
        new (d_all.buffer()) Void();
        d_selectionId = SELECTION_ID_ALL;
    }

    return d_all.object();
}

Void& ClearCache::makeAll(const Void& value)
{
    if (SELECTION_ID_ALL == d_selectionId) {
        d_all.object() = value;
    }
    else {
        reset();
        new (d_all.buffer()) Void(value);
        d_selectionId = SELECTION_ID_ALL;
    }

    return d_all.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& ClearCache::makeAll(Void&& value)
{
    if (SELECTION_ID_ALL == d_selectionId) {
        d_all.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_all.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_ALL;
    }

    return d_all.object();
}
#endif

// ACCESSORS

bsl::ostream&
ClearCache::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        printer.printAttribute("domain", d_domain.object());
    } break;
    case SELECTION_ID_ALL: {
        printer.printAttribute("all", d_all.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ClearCache::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN].name();
    case SELECTION_ID_ALL:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ALL].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------
// class ClusterInfo
// -----------------

// CONSTANTS

const char ClusterInfo::CLASS_NAME[] = "ClusterInfo";

const bdlat_AttributeInfo ClusterInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_LOCALITY,
     "locality",
     sizeof("locality") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NODES,
     "nodes",
     sizeof("nodes") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* ClusterInfo::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_LOCALITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCALITY];
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_NODES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES];
    default: return 0;
    }
}

// CREATORS

ClusterInfo::ClusterInfo(bslma::Allocator* basicAllocator)
: d_nodes(basicAllocator)
, d_name(basicAllocator)
, d_locality(static_cast<Locality::Value>(0))
{
}

ClusterInfo::ClusterInfo(const ClusterInfo& original,
                         bslma::Allocator*  basicAllocator)
: d_nodes(original.d_nodes, basicAllocator)
, d_name(original.d_name, basicAllocator)
, d_locality(original.d_locality)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterInfo::ClusterInfo(ClusterInfo&& original) noexcept
: d_nodes(bsl::move(original.d_nodes)),
  d_name(bsl::move(original.d_name)),
  d_locality(bsl::move(original.d_locality))
{
}

ClusterInfo::ClusterInfo(ClusterInfo&&     original,
                         bslma::Allocator* basicAllocator)
: d_nodes(bsl::move(original.d_nodes), basicAllocator)
, d_name(bsl::move(original.d_name), basicAllocator)
, d_locality(bsl::move(original.d_locality))
{
}
#endif

ClusterInfo::~ClusterInfo()
{
}

// MANIPULATORS

ClusterInfo& ClusterInfo::operator=(const ClusterInfo& rhs)
{
    if (this != &rhs) {
        d_locality = rhs.d_locality;
        d_name     = rhs.d_name;
        d_nodes    = rhs.d_nodes;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterInfo& ClusterInfo::operator=(ClusterInfo&& rhs)
{
    if (this != &rhs) {
        d_locality = bsl::move(rhs.d_locality);
        d_name     = bsl::move(rhs.d_name);
        d_nodes    = bsl::move(rhs.d_nodes);
    }

    return *this;
}
#endif

void ClusterInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_locality);
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_nodes);
}

// ACCESSORS

bsl::ostream&
ClusterInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("locality", this->locality());
    printer.printAttribute("name", this->name());
    printer.printAttribute("nodes", this->nodes());
    printer.end();
    return stream;
}

// ---------------------
// class ClusterNodeInfo
// ---------------------

// CONSTANTS

const char ClusterNodeInfo::CLASS_NAME[] = "ClusterNodeInfo";

const bdlat_AttributeInfo ClusterNodeInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_DESCRIPTION,
     "description",
     sizeof("description") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_IS_AVAILABLE,
     "isAvailable",
     sizeof("isAvailable") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_STATUS,
     "status",
     sizeof("status") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PRIMARY_FOR_PARTITION_IDS,
     "primaryForPartitionIds",
     sizeof("primaryForPartitionIds") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterNodeInfo::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterNodeInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterNodeInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DESCRIPTION];
    case ATTRIBUTE_ID_IS_AVAILABLE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_AVAILABLE];
    case ATTRIBUTE_ID_STATUS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATUS];
    case ATTRIBUTE_ID_PRIMARY_FOR_PARTITION_IDS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_PRIMARY_FOR_PARTITION_IDS];
    default: return 0;
    }
}

// CREATORS

ClusterNodeInfo::ClusterNodeInfo(bslma::Allocator* basicAllocator)
: d_primaryForPartitionIds(basicAllocator)
, d_description(basicAllocator)
, d_status(static_cast<NodeStatus::Value>(0))
, d_isAvailable()
{
}

ClusterNodeInfo::ClusterNodeInfo(const ClusterNodeInfo& original,
                                 bslma::Allocator*      basicAllocator)
: d_primaryForPartitionIds(original.d_primaryForPartitionIds, basicAllocator)
, d_description(original.d_description, basicAllocator)
, d_status(original.d_status)
, d_isAvailable(original.d_isAvailable)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterNodeInfo::ClusterNodeInfo(ClusterNodeInfo&& original) noexcept
: d_primaryForPartitionIds(bsl::move(original.d_primaryForPartitionIds)),
  d_description(bsl::move(original.d_description)),
  d_status(bsl::move(original.d_status)),
  d_isAvailable(bsl::move(original.d_isAvailable))
{
}

ClusterNodeInfo::ClusterNodeInfo(ClusterNodeInfo&& original,
                                 bslma::Allocator* basicAllocator)
: d_primaryForPartitionIds(bsl::move(original.d_primaryForPartitionIds),
                           basicAllocator)
, d_description(bsl::move(original.d_description), basicAllocator)
, d_status(bsl::move(original.d_status))
, d_isAvailable(bsl::move(original.d_isAvailable))
{
}
#endif

ClusterNodeInfo::~ClusterNodeInfo()
{
}

// MANIPULATORS

ClusterNodeInfo& ClusterNodeInfo::operator=(const ClusterNodeInfo& rhs)
{
    if (this != &rhs) {
        d_description            = rhs.d_description;
        d_isAvailable            = rhs.d_isAvailable;
        d_status                 = rhs.d_status;
        d_primaryForPartitionIds = rhs.d_primaryForPartitionIds;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterNodeInfo& ClusterNodeInfo::operator=(ClusterNodeInfo&& rhs)
{
    if (this != &rhs) {
        d_description            = bsl::move(rhs.d_description);
        d_isAvailable            = bsl::move(rhs.d_isAvailable);
        d_status                 = bsl::move(rhs.d_status);
        d_primaryForPartitionIds = bsl::move(rhs.d_primaryForPartitionIds);
    }

    return *this;
}
#endif

void ClusterNodeInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_description);
    bdlat_ValueTypeFunctions::reset(&d_isAvailable);
    bdlat_ValueTypeFunctions::reset(&d_status);
    bdlat_ValueTypeFunctions::reset(&d_primaryForPartitionIds);
}

// ACCESSORS

bsl::ostream& ClusterNodeInfo::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("description", this->description());
    printer.printAttribute("isAvailable", this->isAvailable());
    printer.printAttribute("status", this->status());
    printer.printAttribute("primaryForPartitionIds",
                           this->primaryForPartitionIds());
    printer.end();
    return stream;
}

// ------------------
// class ClusterQueue
// ------------------

// CONSTANTS

const char ClusterQueue::CLASS_NAME[] = "ClusterQueue";

const bdlat_AttributeInfo ClusterQueue::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NUM_IN_FLIGHT_CONTEXTS,
     "numInFlightContexts",
     sizeof("numInFlightContexts") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_IS_ASSIGNED,
     "isAssigned",
     sizeof("isAssigned") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_IS_PRIMARY_AVAILABLE,
     "isPrimaryAvailable",
     sizeof("isPrimaryAvailable") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ID, "id", sizeof("id") - 1, "", bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SUB_IDS,
     "subIds",
     sizeof("subIds") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIMARY_NODE_DESCRIPTION,
     "primaryNodeDescription",
     sizeof("primaryNodeDescription") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_KEY,
     "key",
     sizeof("key") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_IS_CREATED,
     "isCreated",
     sizeof("isCreated") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CONTEXTS,
     "contexts",
     sizeof("contexts") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* ClusterQueue::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 11; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterQueue::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterQueue::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_NUM_IN_FLIGHT_CONTEXTS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_IN_FLIGHT_CONTEXTS];
    case ATTRIBUTE_ID_IS_ASSIGNED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_ASSIGNED];
    case ATTRIBUTE_ID_IS_PRIMARY_AVAILABLE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_PRIMARY_AVAILABLE];
    case ATTRIBUTE_ID_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID];
    case ATTRIBUTE_ID_SUB_IDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUB_IDS];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_PRIMARY_NODE_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_NODE_DESCRIPTION];
    case ATTRIBUTE_ID_KEY: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY];
    case ATTRIBUTE_ID_IS_CREATED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_CREATED];
    case ATTRIBUTE_ID_CONTEXTS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONTEXTS];
    default: return 0;
    }
}

// CREATORS

ClusterQueue::ClusterQueue(bslma::Allocator* basicAllocator)
: d_subIds(basicAllocator)
, d_contexts(basicAllocator)
, d_uri(basicAllocator)
, d_key(basicAllocator)
, d_primaryNodeDescription(basicAllocator)
, d_numInFlightContexts()
, d_id()
, d_partitionId()
, d_isAssigned()
, d_isPrimaryAvailable()
, d_isCreated()
{
}

ClusterQueue::ClusterQueue(const ClusterQueue& original,
                           bslma::Allocator*   basicAllocator)
: d_subIds(original.d_subIds, basicAllocator)
, d_contexts(original.d_contexts, basicAllocator)
, d_uri(original.d_uri, basicAllocator)
, d_key(original.d_key, basicAllocator)
, d_primaryNodeDescription(original.d_primaryNodeDescription, basicAllocator)
, d_numInFlightContexts(original.d_numInFlightContexts)
, d_id(original.d_id)
, d_partitionId(original.d_partitionId)
, d_isAssigned(original.d_isAssigned)
, d_isPrimaryAvailable(original.d_isPrimaryAvailable)
, d_isCreated(original.d_isCreated)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterQueue::ClusterQueue(ClusterQueue&& original) noexcept
: d_subIds(bsl::move(original.d_subIds)),
  d_contexts(bsl::move(original.d_contexts)),
  d_uri(bsl::move(original.d_uri)),
  d_key(bsl::move(original.d_key)),
  d_primaryNodeDescription(bsl::move(original.d_primaryNodeDescription)),
  d_numInFlightContexts(bsl::move(original.d_numInFlightContexts)),
  d_id(bsl::move(original.d_id)),
  d_partitionId(bsl::move(original.d_partitionId)),
  d_isAssigned(bsl::move(original.d_isAssigned)),
  d_isPrimaryAvailable(bsl::move(original.d_isPrimaryAvailable)),
  d_isCreated(bsl::move(original.d_isCreated))
{
}

ClusterQueue::ClusterQueue(ClusterQueue&&    original,
                           bslma::Allocator* basicAllocator)
: d_subIds(bsl::move(original.d_subIds), basicAllocator)
, d_contexts(bsl::move(original.d_contexts), basicAllocator)
, d_uri(bsl::move(original.d_uri), basicAllocator)
, d_key(bsl::move(original.d_key), basicAllocator)
, d_primaryNodeDescription(bsl::move(original.d_primaryNodeDescription),
                           basicAllocator)
, d_numInFlightContexts(bsl::move(original.d_numInFlightContexts))
, d_id(bsl::move(original.d_id))
, d_partitionId(bsl::move(original.d_partitionId))
, d_isAssigned(bsl::move(original.d_isAssigned))
, d_isPrimaryAvailable(bsl::move(original.d_isPrimaryAvailable))
, d_isCreated(bsl::move(original.d_isCreated))
{
}
#endif

ClusterQueue::~ClusterQueue()
{
}

// MANIPULATORS

ClusterQueue& ClusterQueue::operator=(const ClusterQueue& rhs)
{
    if (this != &rhs) {
        d_uri                    = rhs.d_uri;
        d_numInFlightContexts    = rhs.d_numInFlightContexts;
        d_isAssigned             = rhs.d_isAssigned;
        d_isPrimaryAvailable     = rhs.d_isPrimaryAvailable;
        d_id                     = rhs.d_id;
        d_subIds                 = rhs.d_subIds;
        d_partitionId            = rhs.d_partitionId;
        d_primaryNodeDescription = rhs.d_primaryNodeDescription;
        d_key                    = rhs.d_key;
        d_isCreated              = rhs.d_isCreated;
        d_contexts               = rhs.d_contexts;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterQueue& ClusterQueue::operator=(ClusterQueue&& rhs)
{
    if (this != &rhs) {
        d_uri                    = bsl::move(rhs.d_uri);
        d_numInFlightContexts    = bsl::move(rhs.d_numInFlightContexts);
        d_isAssigned             = bsl::move(rhs.d_isAssigned);
        d_isPrimaryAvailable     = bsl::move(rhs.d_isPrimaryAvailable);
        d_id                     = bsl::move(rhs.d_id);
        d_subIds                 = bsl::move(rhs.d_subIds);
        d_partitionId            = bsl::move(rhs.d_partitionId);
        d_primaryNodeDescription = bsl::move(rhs.d_primaryNodeDescription);
        d_key                    = bsl::move(rhs.d_key);
        d_isCreated              = bsl::move(rhs.d_isCreated);
        d_contexts               = bsl::move(rhs.d_contexts);
    }

    return *this;
}
#endif

void ClusterQueue::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_numInFlightContexts);
    bdlat_ValueTypeFunctions::reset(&d_isAssigned);
    bdlat_ValueTypeFunctions::reset(&d_isPrimaryAvailable);
    bdlat_ValueTypeFunctions::reset(&d_id);
    bdlat_ValueTypeFunctions::reset(&d_subIds);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_primaryNodeDescription);
    bdlat_ValueTypeFunctions::reset(&d_key);
    bdlat_ValueTypeFunctions::reset(&d_isCreated);
    bdlat_ValueTypeFunctions::reset(&d_contexts);
}

// ACCESSORS

bsl::ostream&
ClusterQueue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("numInFlightContexts", this->numInFlightContexts());
    printer.printAttribute("isAssigned", this->isAssigned());
    printer.printAttribute("isPrimaryAvailable", this->isPrimaryAvailable());
    printer.printAttribute("id", this->id());
    printer.printAttribute("subIds", this->subIds());
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("primaryNodeDescription",
                           this->primaryNodeDescription());
    printer.printAttribute("key", this->key());
    printer.printAttribute("isCreated", this->isCreated());
    printer.printAttribute("contexts", this->contexts());
    printer.end();
    return stream;
}

// -------------------
// class DangerCommand
// -------------------

// CONSTANTS

const char DangerCommand::CLASS_NAME[] = "DangerCommand";

const bdlat_SelectionInfo DangerCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_SHUTDOWN,
     "shutdown",
     sizeof("shutdown") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TERMINATE,
     "terminate",
     sizeof("terminate") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* DangerCommand::lookupSelectionInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            DangerCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* DangerCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_SHUTDOWN:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SHUTDOWN];
    case SELECTION_ID_TERMINATE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TERMINATE];
    default: return 0;
    }
}

// CREATORS

DangerCommand::DangerCommand(const DangerCommand& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_SHUTDOWN: {
        new (d_shutdown.buffer()) Void(original.d_shutdown.object());
    } break;
    case SELECTION_ID_TERMINATE: {
        new (d_terminate.buffer()) Void(original.d_terminate.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DangerCommand::DangerCommand(DangerCommand&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_SHUTDOWN: {
        new (d_shutdown.buffer())
            Void(bsl::move(original.d_shutdown.object()));
    } break;
    case SELECTION_ID_TERMINATE: {
        new (d_terminate.buffer())
            Void(bsl::move(original.d_terminate.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

DangerCommand& DangerCommand::operator=(const DangerCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SHUTDOWN: {
            makeShutdown(rhs.d_shutdown.object());
        } break;
        case SELECTION_ID_TERMINATE: {
            makeTerminate(rhs.d_terminate.object());
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
DangerCommand& DangerCommand::operator=(DangerCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SHUTDOWN: {
            makeShutdown(bsl::move(rhs.d_shutdown.object()));
        } break;
        case SELECTION_ID_TERMINATE: {
            makeTerminate(bsl::move(rhs.d_terminate.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void DangerCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_SHUTDOWN: {
        d_shutdown.object().~Void();
    } break;
    case SELECTION_ID_TERMINATE: {
        d_terminate.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int DangerCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_SHUTDOWN: {
        makeShutdown();
    } break;
    case SELECTION_ID_TERMINATE: {
        makeTerminate();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int DangerCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& DangerCommand::makeShutdown()
{
    if (SELECTION_ID_SHUTDOWN == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_shutdown.object());
    }
    else {
        reset();
        new (d_shutdown.buffer()) Void();
        d_selectionId = SELECTION_ID_SHUTDOWN;
    }

    return d_shutdown.object();
}

Void& DangerCommand::makeShutdown(const Void& value)
{
    if (SELECTION_ID_SHUTDOWN == d_selectionId) {
        d_shutdown.object() = value;
    }
    else {
        reset();
        new (d_shutdown.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SHUTDOWN;
    }

    return d_shutdown.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& DangerCommand::makeShutdown(Void&& value)
{
    if (SELECTION_ID_SHUTDOWN == d_selectionId) {
        d_shutdown.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_shutdown.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SHUTDOWN;
    }

    return d_shutdown.object();
}
#endif

Void& DangerCommand::makeTerminate()
{
    if (SELECTION_ID_TERMINATE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_terminate.object());
    }
    else {
        reset();
        new (d_terminate.buffer()) Void();
        d_selectionId = SELECTION_ID_TERMINATE;
    }

    return d_terminate.object();
}

Void& DangerCommand::makeTerminate(const Void& value)
{
    if (SELECTION_ID_TERMINATE == d_selectionId) {
        d_terminate.object() = value;
    }
    else {
        reset();
        new (d_terminate.buffer()) Void(value);
        d_selectionId = SELECTION_ID_TERMINATE;
    }

    return d_terminate.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& DangerCommand::makeTerminate(Void&& value)
{
    if (SELECTION_ID_TERMINATE == d_selectionId) {
        d_terminate.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_terminate.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_TERMINATE;
    }

    return d_terminate.object();
}
#endif

// ACCESSORS

bsl::ostream&
DangerCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_SHUTDOWN: {
        printer.printAttribute("shutdown", d_shutdown.object());
    } break;
    case SELECTION_ID_TERMINATE: {
        printer.printAttribute("terminate", d_terminate.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* DangerCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_SHUTDOWN:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SHUTDOWN].name();
    case SELECTION_ID_TERMINATE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TERMINATE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------
// class ElectorInfo
// -----------------

// CONSTANTS

const char ElectorInfo::CLASS_NAME[] = "ElectorInfo";

const bdlat_AttributeInfo ElectorInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ELECTOR_STATE,
     "electorState",
     sizeof("electorState") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_LEADER_NODE,
     "leaderNode",
     sizeof("leaderNode") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_LEADER_MESSAGE_SEQUENCE,
     "leaderMessageSequence",
     sizeof("leaderMessageSequence") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_LEADER_STATUS,
     "leaderStatus",
     sizeof("leaderStatus") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* ElectorInfo::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ElectorInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ElectorInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ELECTOR_STATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTOR_STATE];
    case ATTRIBUTE_ID_LEADER_NODE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_NODE];
    case ATTRIBUTE_ID_LEADER_MESSAGE_SEQUENCE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_MESSAGE_SEQUENCE];
    case ATTRIBUTE_ID_LEADER_STATUS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_STATUS];
    default: return 0;
    }
}

// CREATORS

ElectorInfo::ElectorInfo(bslma::Allocator* basicAllocator)
: d_leaderNode(basicAllocator)
, d_leaderMessageSequence()
, d_leaderStatus(static_cast<LeaderStatus::Value>(0))
, d_electorState(static_cast<ElectorState::Value>(0))
{
}

ElectorInfo::ElectorInfo(const ElectorInfo& original,
                         bslma::Allocator*  basicAllocator)
: d_leaderNode(original.d_leaderNode, basicAllocator)
, d_leaderMessageSequence(original.d_leaderMessageSequence)
, d_leaderStatus(original.d_leaderStatus)
, d_electorState(original.d_electorState)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectorInfo::ElectorInfo(ElectorInfo&& original) noexcept
: d_leaderNode(bsl::move(original.d_leaderNode)),
  d_leaderMessageSequence(bsl::move(original.d_leaderMessageSequence)),
  d_leaderStatus(bsl::move(original.d_leaderStatus)),
  d_electorState(bsl::move(original.d_electorState))
{
}

ElectorInfo::ElectorInfo(ElectorInfo&&     original,
                         bslma::Allocator* basicAllocator)
: d_leaderNode(bsl::move(original.d_leaderNode), basicAllocator)
, d_leaderMessageSequence(bsl::move(original.d_leaderMessageSequence))
, d_leaderStatus(bsl::move(original.d_leaderStatus))
, d_electorState(bsl::move(original.d_electorState))
{
}
#endif

ElectorInfo::~ElectorInfo()
{
}

// MANIPULATORS

ElectorInfo& ElectorInfo::operator=(const ElectorInfo& rhs)
{
    if (this != &rhs) {
        d_electorState          = rhs.d_electorState;
        d_leaderNode            = rhs.d_leaderNode;
        d_leaderMessageSequence = rhs.d_leaderMessageSequence;
        d_leaderStatus          = rhs.d_leaderStatus;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectorInfo& ElectorInfo::operator=(ElectorInfo&& rhs)
{
    if (this != &rhs) {
        d_electorState          = bsl::move(rhs.d_electorState);
        d_leaderNode            = bsl::move(rhs.d_leaderNode);
        d_leaderMessageSequence = bsl::move(rhs.d_leaderMessageSequence);
        d_leaderStatus          = bsl::move(rhs.d_leaderStatus);
    }

    return *this;
}
#endif

void ElectorInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_electorState);
    bdlat_ValueTypeFunctions::reset(&d_leaderNode);
    bdlat_ValueTypeFunctions::reset(&d_leaderMessageSequence);
    bdlat_ValueTypeFunctions::reset(&d_leaderStatus);
}

// ACCESSORS

bsl::ostream&
ElectorInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("electorState", this->electorState());
    printer.printAttribute("leaderNode", this->leaderNode());
    printer.printAttribute("leaderMessageSequence",
                           this->leaderMessageSequence());
    printer.printAttribute("leaderStatus", this->leaderStatus());
    printer.end();
    return stream;
}

// ----------------------
// class GetTunableChoice
// ----------------------

// CONSTANTS

const char GetTunableChoice::CLASS_NAME[] = "GetTunableChoice";

const bdlat_SelectionInfo GetTunableChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ALL,
     "all",
     sizeof("all") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SELF,
     "self",
     sizeof("self") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
GetTunableChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            GetTunableChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* GetTunableChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ALL: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ALL];
    case SELECTION_ID_SELF: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SELF];
    default: return 0;
    }
}

// CREATORS

GetTunableChoice::GetTunableChoice(const GetTunableChoice& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_ALL: {
        new (d_all.buffer()) Void(original.d_all.object());
    } break;
    case SELECTION_ID_SELF: {
        new (d_self.buffer()) Void(original.d_self.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
GetTunableChoice::GetTunableChoice(GetTunableChoice&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_ALL: {
        new (d_all.buffer()) Void(bsl::move(original.d_all.object()));
    } break;
    case SELECTION_ID_SELF: {
        new (d_self.buffer()) Void(bsl::move(original.d_self.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

GetTunableChoice& GetTunableChoice::operator=(const GetTunableChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ALL: {
            makeAll(rhs.d_all.object());
        } break;
        case SELECTION_ID_SELF: {
            makeSelf(rhs.d_self.object());
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
GetTunableChoice& GetTunableChoice::operator=(GetTunableChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ALL: {
            makeAll(bsl::move(rhs.d_all.object()));
        } break;
        case SELECTION_ID_SELF: {
            makeSelf(bsl::move(rhs.d_self.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void GetTunableChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ALL: {
        d_all.object().~Void();
    } break;
    case SELECTION_ID_SELF: {
        d_self.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int GetTunableChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ALL: {
        makeAll();
    } break;
    case SELECTION_ID_SELF: {
        makeSelf();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int GetTunableChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& GetTunableChoice::makeAll()
{
    if (SELECTION_ID_ALL == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_all.object());
    }
    else {
        reset();
        new (d_all.buffer()) Void();
        d_selectionId = SELECTION_ID_ALL;
    }

    return d_all.object();
}

Void& GetTunableChoice::makeAll(const Void& value)
{
    if (SELECTION_ID_ALL == d_selectionId) {
        d_all.object() = value;
    }
    else {
        reset();
        new (d_all.buffer()) Void(value);
        d_selectionId = SELECTION_ID_ALL;
    }

    return d_all.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& GetTunableChoice::makeAll(Void&& value)
{
    if (SELECTION_ID_ALL == d_selectionId) {
        d_all.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_all.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_ALL;
    }

    return d_all.object();
}
#endif

Void& GetTunableChoice::makeSelf()
{
    if (SELECTION_ID_SELF == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_self.object());
    }
    else {
        reset();
        new (d_self.buffer()) Void();
        d_selectionId = SELECTION_ID_SELF;
    }

    return d_self.object();
}

Void& GetTunableChoice::makeSelf(const Void& value)
{
    if (SELECTION_ID_SELF == d_selectionId) {
        d_self.object() = value;
    }
    else {
        reset();
        new (d_self.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SELF;
    }

    return d_self.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& GetTunableChoice::makeSelf(Void&& value)
{
    if (SELECTION_ID_SELF == d_selectionId) {
        d_self.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_self.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SELF;
    }

    return d_self.object();
}
#endif

// ACCESSORS

bsl::ostream& GetTunableChoice::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ALL: {
        printer.printAttribute("all", d_all.object());
    } break;
    case SELECTION_ID_SELF: {
        printer.printAttribute("self", d_self.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* GetTunableChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ALL:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ALL].name();
    case SELECTION_ID_SELF:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SELF].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------
// class Help
// ----------

// CONSTANTS

const char Help::CLASS_NAME[] = "Help";

const bdlat_AttributeInfo Help::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_COMMANDS,
     "commands",
     sizeof("commands") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_IS_PLUMBING,
     "isPlumbing",
     sizeof("isPlumbing") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Help::lookupAttributeInfo(const char* name,
                                                     int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Help::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Help::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_COMMANDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMANDS];
    case ATTRIBUTE_ID_IS_PLUMBING:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_PLUMBING];
    default: return 0;
    }
}

// CREATORS

Help::Help(bslma::Allocator* basicAllocator)
: d_commands(basicAllocator)
, d_isPlumbing()
{
}

Help::Help(const Help& original, bslma::Allocator* basicAllocator)
: d_commands(original.d_commands, basicAllocator)
, d_isPlumbing(original.d_isPlumbing)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Help::Help(Help&& original) noexcept
: d_commands(bsl::move(original.d_commands)),
  d_isPlumbing(bsl::move(original.d_isPlumbing))
{
}

Help::Help(Help&& original, bslma::Allocator* basicAllocator)
: d_commands(bsl::move(original.d_commands), basicAllocator)
, d_isPlumbing(bsl::move(original.d_isPlumbing))
{
}
#endif

Help::~Help()
{
}

// MANIPULATORS

Help& Help::operator=(const Help& rhs)
{
    if (this != &rhs) {
        d_commands   = rhs.d_commands;
        d_isPlumbing = rhs.d_isPlumbing;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Help& Help::operator=(Help&& rhs)
{
    if (this != &rhs) {
        d_commands   = bsl::move(rhs.d_commands);
        d_isPlumbing = bsl::move(rhs.d_isPlumbing);
    }

    return *this;
}
#endif

void Help::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_commands);
    bdlat_ValueTypeFunctions::reset(&d_isPlumbing);
}

// ACCESSORS

bsl::ostream&
Help::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("commands", this->commands());
    printer.printAttribute("isPlumbing", this->isPlumbing());
    printer.end();
    return stream;
}

// --------------------------------
// class MessageGroupIdManagerIndex
// --------------------------------

// CONSTANTS

const char MessageGroupIdManagerIndex::CLASS_NAME[] =
    "MessageGroupIdManagerIndex";

const bdlat_AttributeInfo MessageGroupIdManagerIndex::ATTRIBUTE_INFO_ARRAY[] =
    {{ATTRIBUTE_ID_LEAST_RECENTLY_USED_GROUP_IDS,
      "leastRecentlyUsedGroupIds",
      sizeof("leastRecentlyUsedGroupIds") - 1,
      "",
      bdlat_FormattingMode::e_DEFAULT},
     {ATTRIBUTE_ID_NUM_MSG_GROUPS_PER_CLIENT,
      "numMsgGroupsPerClient",
      sizeof("numMsgGroupsPerClient") - 1,
      "",
      bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
MessageGroupIdManagerIndex::lookupAttributeInfo(const char* name,
                                                int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            MessageGroupIdManagerIndex::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
MessageGroupIdManagerIndex::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_LEAST_RECENTLY_USED_GROUP_IDS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_LEAST_RECENTLY_USED_GROUP_IDS];
    case ATTRIBUTE_ID_NUM_MSG_GROUPS_PER_CLIENT:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_NUM_MSG_GROUPS_PER_CLIENT];
    default: return 0;
    }
}

// CREATORS

MessageGroupIdManagerIndex::MessageGroupIdManagerIndex(
    bslma::Allocator* basicAllocator)
: d_leastRecentlyUsedGroupIds(basicAllocator)
, d_numMsgGroupsPerClient(basicAllocator)
{
}

MessageGroupIdManagerIndex::MessageGroupIdManagerIndex(
    const MessageGroupIdManagerIndex& original,
    bslma::Allocator*                 basicAllocator)
: d_leastRecentlyUsedGroupIds(original.d_leastRecentlyUsedGroupIds,
                              basicAllocator)
, d_numMsgGroupsPerClient(original.d_numMsgGroupsPerClient, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
MessageGroupIdManagerIndex::MessageGroupIdManagerIndex(
    MessageGroupIdManagerIndex&& original) noexcept
: d_leastRecentlyUsedGroupIds(bsl::move(original.d_leastRecentlyUsedGroupIds)),
  d_numMsgGroupsPerClient(bsl::move(original.d_numMsgGroupsPerClient))
{
}

MessageGroupIdManagerIndex::MessageGroupIdManagerIndex(
    MessageGroupIdManagerIndex&& original,
    bslma::Allocator*            basicAllocator)
: d_leastRecentlyUsedGroupIds(bsl::move(original.d_leastRecentlyUsedGroupIds),
                              basicAllocator)
, d_numMsgGroupsPerClient(bsl::move(original.d_numMsgGroupsPerClient),
                          basicAllocator)
{
}
#endif

MessageGroupIdManagerIndex::~MessageGroupIdManagerIndex()
{
}

// MANIPULATORS

MessageGroupIdManagerIndex&
MessageGroupIdManagerIndex::operator=(const MessageGroupIdManagerIndex& rhs)
{
    if (this != &rhs) {
        d_leastRecentlyUsedGroupIds = rhs.d_leastRecentlyUsedGroupIds;
        d_numMsgGroupsPerClient     = rhs.d_numMsgGroupsPerClient;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
MessageGroupIdManagerIndex&
MessageGroupIdManagerIndex::operator=(MessageGroupIdManagerIndex&& rhs)
{
    if (this != &rhs) {
        d_leastRecentlyUsedGroupIds = bsl::move(
            rhs.d_leastRecentlyUsedGroupIds);
        d_numMsgGroupsPerClient = bsl::move(rhs.d_numMsgGroupsPerClient);
    }

    return *this;
}
#endif

void MessageGroupIdManagerIndex::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_leastRecentlyUsedGroupIds);
    bdlat_ValueTypeFunctions::reset(&d_numMsgGroupsPerClient);
}

// ACCESSORS

bsl::ostream& MessageGroupIdManagerIndex::print(bsl::ostream& stream,
                                                int           level,
                                                int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("leastRecentlyUsedGroupIds",
                           this->leastRecentlyUsedGroupIds());
    printer.printAttribute("numMsgGroupsPerClient",
                           this->numMsgGroupsPerClient());
    printer.end();
    return stream;
}

// -------------------
// class PartitionInfo
// -------------------

// CONSTANTS

const char PartitionInfo::CLASS_NAME[] = "PartitionInfo";

const bdlat_AttributeInfo PartitionInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NUM_QUEUES_MAPPED,
     "numQueuesMapped",
     sizeof("numQueuesMapped") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_ACTIVE_QUEUES,
     "numActiveQueues",
     sizeof("numActiveQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIMARY_NODE,
     "primaryNode",
     sizeof("primaryNode") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PRIMARY_LEASE_ID,
     "primaryLeaseId",
     sizeof("primaryLeaseId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIMARY_STATUS,
     "primaryStatus",
     sizeof("primaryStatus") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* PartitionInfo::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PartitionInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NUM_QUEUES_MAPPED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_QUEUES_MAPPED];
    case ATTRIBUTE_ID_NUM_ACTIVE_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_ACTIVE_QUEUES];
    case ATTRIBUTE_ID_PRIMARY_NODE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_NODE];
    case ATTRIBUTE_ID_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_PRIMARY_STATUS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_STATUS];
    default: return 0;
    }
}

// CREATORS

PartitionInfo::PartitionInfo(bslma::Allocator* basicAllocator)
: d_primaryNode(basicAllocator)
, d_primaryLeaseId()
, d_numQueuesMapped()
, d_numActiveQueues()
, d_primaryStatus(static_cast<PrimaryStatus::Value>(0))
{
}

PartitionInfo::PartitionInfo(const PartitionInfo& original,
                             bslma::Allocator*    basicAllocator)
: d_primaryNode(original.d_primaryNode, basicAllocator)
, d_primaryLeaseId(original.d_primaryLeaseId)
, d_numQueuesMapped(original.d_numQueuesMapped)
, d_numActiveQueues(original.d_numActiveQueues)
, d_primaryStatus(original.d_primaryStatus)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionInfo::PartitionInfo(PartitionInfo&& original) noexcept
: d_primaryNode(bsl::move(original.d_primaryNode)),
  d_primaryLeaseId(bsl::move(original.d_primaryLeaseId)),
  d_numQueuesMapped(bsl::move(original.d_numQueuesMapped)),
  d_numActiveQueues(bsl::move(original.d_numActiveQueues)),
  d_primaryStatus(bsl::move(original.d_primaryStatus))
{
}

PartitionInfo::PartitionInfo(PartitionInfo&&   original,
                             bslma::Allocator* basicAllocator)
: d_primaryNode(bsl::move(original.d_primaryNode), basicAllocator)
, d_primaryLeaseId(bsl::move(original.d_primaryLeaseId))
, d_numQueuesMapped(bsl::move(original.d_numQueuesMapped))
, d_numActiveQueues(bsl::move(original.d_numActiveQueues))
, d_primaryStatus(bsl::move(original.d_primaryStatus))
{
}
#endif

PartitionInfo::~PartitionInfo()
{
}

// MANIPULATORS

PartitionInfo& PartitionInfo::operator=(const PartitionInfo& rhs)
{
    if (this != &rhs) {
        d_numQueuesMapped = rhs.d_numQueuesMapped;
        d_numActiveQueues = rhs.d_numActiveQueues;
        d_primaryNode     = rhs.d_primaryNode;
        d_primaryLeaseId  = rhs.d_primaryLeaseId;
        d_primaryStatus   = rhs.d_primaryStatus;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionInfo& PartitionInfo::operator=(PartitionInfo&& rhs)
{
    if (this != &rhs) {
        d_numQueuesMapped = bsl::move(rhs.d_numQueuesMapped);
        d_numActiveQueues = bsl::move(rhs.d_numActiveQueues);
        d_primaryNode     = bsl::move(rhs.d_primaryNode);
        d_primaryLeaseId  = bsl::move(rhs.d_primaryLeaseId);
        d_primaryStatus   = bsl::move(rhs.d_primaryStatus);
    }

    return *this;
}
#endif

void PartitionInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_numQueuesMapped);
    bdlat_ValueTypeFunctions::reset(&d_numActiveQueues);
    bdlat_ValueTypeFunctions::reset(&d_primaryNode);
    bdlat_ValueTypeFunctions::reset(&d_primaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_primaryStatus);
}

// ACCESSORS

bsl::ostream&
PartitionInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("numQueuesMapped", this->numQueuesMapped());
    printer.printAttribute("numActiveQueues", this->numActiveQueues());
    printer.printAttribute("primaryNode", this->primaryNode());
    printer.printAttribute("primaryLeaseId", this->primaryLeaseId());
    printer.printAttribute("primaryStatus", this->primaryStatus());
    printer.end();
    return stream;
}

// ----------------------
// class PurgeQueueResult
// ----------------------

// CONSTANTS

const char PurgeQueueResult::CLASS_NAME[] = "PurgeQueueResult";

const bdlat_SelectionInfo PurgeQueueResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE,
     "queue",
     sizeof("queue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
PurgeQueueResult::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            PurgeQueueResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* PurgeQueueResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE];
    default: return 0;
    }
}

// CREATORS

PurgeQueueResult::PurgeQueueResult(const PurgeQueueResult& original,
                                   bslma::Allocator*       basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE: {
        new (d_queue.buffer())
            PurgedQueueDetails(original.d_queue.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgeQueueResult::PurgeQueueResult(PurgeQueueResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE: {
        new (d_queue.buffer())
            PurgedQueueDetails(bsl::move(original.d_queue.object()),
                               d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

PurgeQueueResult::PurgeQueueResult(PurgeQueueResult&& original,
                                   bslma::Allocator*  basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE: {
        new (d_queue.buffer())
            PurgedQueueDetails(bsl::move(original.d_queue.object()),
                               d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

PurgeQueueResult& PurgeQueueResult::operator=(const PurgeQueueResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_QUEUE: {
            makeQueue(rhs.d_queue.object());
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
PurgeQueueResult& PurgeQueueResult::operator=(PurgeQueueResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_QUEUE: {
            makeQueue(bsl::move(rhs.d_queue.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void PurgeQueueResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_QUEUE: {
        d_queue.object().~PurgedQueueDetails();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int PurgeQueueResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_QUEUE: {
        makeQueue();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int PurgeQueueResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Error& PurgeQueueResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& PurgeQueueResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& PurgeQueueResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

PurgedQueueDetails& PurgeQueueResult::makeQueue()
{
    if (SELECTION_ID_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queue.object());
    }
    else {
        reset();
        new (d_queue.buffer()) PurgedQueueDetails(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE;
    }

    return d_queue.object();
}

PurgedQueueDetails&
PurgeQueueResult::makeQueue(const PurgedQueueDetails& value)
{
    if (SELECTION_ID_QUEUE == d_selectionId) {
        d_queue.object() = value;
    }
    else {
        reset();
        new (d_queue.buffer()) PurgedQueueDetails(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE;
    }

    return d_queue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgedQueueDetails& PurgeQueueResult::makeQueue(PurgedQueueDetails&& value)
{
    if (SELECTION_ID_QUEUE == d_selectionId) {
        d_queue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queue.buffer())
            PurgedQueueDetails(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE;
    }

    return d_queue.object();
}
#endif

// ACCESSORS

bsl::ostream& PurgeQueueResult::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_QUEUE: {
        printer.printAttribute("queue", d_queue.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* PurgeQueueResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ------------------
// class QueueCommand
// ------------------

// CONSTANTS

const char QueueCommand::CLASS_NAME[] = "QueueCommand";

const bdlat_SelectionInfo QueueCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_PURGE_APP_ID,
     "purgeAppId",
     sizeof("purgeAppId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {SELECTION_ID_INTERNALS,
     "internals",
     sizeof("internals") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_MESSAGES,
     "messages",
     sizeof("messages") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* QueueCommand::lookupSelectionInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            QueueCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* QueueCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_PURGE_APP_ID:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGE_APP_ID];
    case SELECTION_ID_INTERNALS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_INTERNALS];
    case SELECTION_ID_MESSAGES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_MESSAGES];
    default: return 0;
    }
}

// CREATORS

QueueCommand::QueueCommand(const QueueCommand& original,
                           bslma::Allocator*   basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        new (d_purgeAppId.buffer())
            bsl::string(original.d_purgeAppId.object(), d_allocator_p);
    } break;
    case SELECTION_ID_INTERNALS: {
        new (d_internals.buffer()) Void(original.d_internals.object());
    } break;
    case SELECTION_ID_MESSAGES: {
        new (d_messages.buffer())
            ListMessages(original.d_messages.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueCommand::QueueCommand(QueueCommand&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        new (d_purgeAppId.buffer())
            bsl::string(bsl::move(original.d_purgeAppId.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_INTERNALS: {
        new (d_internals.buffer())
            Void(bsl::move(original.d_internals.object()));
    } break;
    case SELECTION_ID_MESSAGES: {
        new (d_messages.buffer())
            ListMessages(bsl::move(original.d_messages.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

QueueCommand::QueueCommand(QueueCommand&&    original,
                           bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        new (d_purgeAppId.buffer())
            bsl::string(bsl::move(original.d_purgeAppId.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_INTERNALS: {
        new (d_internals.buffer())
            Void(bsl::move(original.d_internals.object()));
    } break;
    case SELECTION_ID_MESSAGES: {
        new (d_messages.buffer())
            ListMessages(bsl::move(original.d_messages.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

QueueCommand& QueueCommand::operator=(const QueueCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_PURGE_APP_ID: {
            makePurgeAppId(rhs.d_purgeAppId.object());
        } break;
        case SELECTION_ID_INTERNALS: {
            makeInternals(rhs.d_internals.object());
        } break;
        case SELECTION_ID_MESSAGES: {
            makeMessages(rhs.d_messages.object());
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
QueueCommand& QueueCommand::operator=(QueueCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_PURGE_APP_ID: {
            makePurgeAppId(bsl::move(rhs.d_purgeAppId.object()));
        } break;
        case SELECTION_ID_INTERNALS: {
            makeInternals(bsl::move(rhs.d_internals.object()));
        } break;
        case SELECTION_ID_MESSAGES: {
            makeMessages(bsl::move(rhs.d_messages.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void QueueCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        typedef bsl::string Type;
        d_purgeAppId.object().~Type();
    } break;
    case SELECTION_ID_INTERNALS: {
        d_internals.object().~Void();
    } break;
    case SELECTION_ID_MESSAGES: {
        d_messages.object().~ListMessages();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int QueueCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        makePurgeAppId();
    } break;
    case SELECTION_ID_INTERNALS: {
        makeInternals();
    } break;
    case SELECTION_ID_MESSAGES: {
        makeMessages();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int QueueCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

bsl::string& QueueCommand::makePurgeAppId()
{
    if (SELECTION_ID_PURGE_APP_ID == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_purgeAppId.object());
    }
    else {
        reset();
        new (d_purgeAppId.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_PURGE_APP_ID;
    }

    return d_purgeAppId.object();
}

bsl::string& QueueCommand::makePurgeAppId(const bsl::string& value)
{
    if (SELECTION_ID_PURGE_APP_ID == d_selectionId) {
        d_purgeAppId.object() = value;
    }
    else {
        reset();
        new (d_purgeAppId.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_PURGE_APP_ID;
    }

    return d_purgeAppId.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& QueueCommand::makePurgeAppId(bsl::string&& value)
{
    if (SELECTION_ID_PURGE_APP_ID == d_selectionId) {
        d_purgeAppId.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_purgeAppId.buffer())
            bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_PURGE_APP_ID;
    }

    return d_purgeAppId.object();
}
#endif

Void& QueueCommand::makeInternals()
{
    if (SELECTION_ID_INTERNALS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_internals.object());
    }
    else {
        reset();
        new (d_internals.buffer()) Void();
        d_selectionId = SELECTION_ID_INTERNALS;
    }

    return d_internals.object();
}

Void& QueueCommand::makeInternals(const Void& value)
{
    if (SELECTION_ID_INTERNALS == d_selectionId) {
        d_internals.object() = value;
    }
    else {
        reset();
        new (d_internals.buffer()) Void(value);
        d_selectionId = SELECTION_ID_INTERNALS;
    }

    return d_internals.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& QueueCommand::makeInternals(Void&& value)
{
    if (SELECTION_ID_INTERNALS == d_selectionId) {
        d_internals.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_internals.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_INTERNALS;
    }

    return d_internals.object();
}
#endif

ListMessages& QueueCommand::makeMessages()
{
    if (SELECTION_ID_MESSAGES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_messages.object());
    }
    else {
        reset();
        new (d_messages.buffer()) ListMessages(d_allocator_p);
        d_selectionId = SELECTION_ID_MESSAGES;
    }

    return d_messages.object();
}

ListMessages& QueueCommand::makeMessages(const ListMessages& value)
{
    if (SELECTION_ID_MESSAGES == d_selectionId) {
        d_messages.object() = value;
    }
    else {
        reset();
        new (d_messages.buffer()) ListMessages(value, d_allocator_p);
        d_selectionId = SELECTION_ID_MESSAGES;
    }

    return d_messages.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ListMessages& QueueCommand::makeMessages(ListMessages&& value)
{
    if (SELECTION_ID_MESSAGES == d_selectionId) {
        d_messages.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_messages.buffer())
            ListMessages(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_MESSAGES;
    }

    return d_messages.object();
}
#endif

// ACCESSORS

bsl::ostream&
QueueCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID: {
        printer.printAttribute("purgeAppId", d_purgeAppId.object());
    } break;
    case SELECTION_ID_INTERNALS: {
        printer.printAttribute("internals", d_internals.object());
    } break;
    case SELECTION_ID_MESSAGES: {
        printer.printAttribute("messages", d_messages.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* QueueCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE_APP_ID:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGE_APP_ID].name();
    case SELECTION_ID_INTERNALS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_INTERNALS].name();
    case SELECTION_ID_MESSAGES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_MESSAGES].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -------------------
// class QueueContents
// -------------------

// CONSTANTS

const char QueueContents::CLASS_NAME[] = "QueueContents";

const bdlat_AttributeInfo QueueContents::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_MESSAGES,
     "messages",
     sizeof("messages") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_OFFSET,
     "offset",
     sizeof("offset") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_TOTAL_QUEUE_MESSAGES,
     "totalQueueMessages",
     sizeof("totalQueueMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* QueueContents::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueContents::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueContents::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES];
    case ATTRIBUTE_ID_OFFSET:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OFFSET];
    case ATTRIBUTE_ID_TOTAL_QUEUE_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TOTAL_QUEUE_MESSAGES];
    default: return 0;
    }
}

// CREATORS

QueueContents::QueueContents(bslma::Allocator* basicAllocator)
: d_offset()
, d_totalQueueMessages()
, d_messages(basicAllocator)
{
}

QueueContents::QueueContents(const QueueContents& original,
                             bslma::Allocator*    basicAllocator)
: d_offset(original.d_offset)
, d_totalQueueMessages(original.d_totalQueueMessages)
, d_messages(original.d_messages, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueContents::QueueContents(QueueContents&& original) noexcept
: d_offset(bsl::move(original.d_offset)),
  d_totalQueueMessages(bsl::move(original.d_totalQueueMessages)),
  d_messages(bsl::move(original.d_messages))
{
}

QueueContents::QueueContents(QueueContents&&   original,
                             bslma::Allocator* basicAllocator)
: d_offset(bsl::move(original.d_offset))
, d_totalQueueMessages(bsl::move(original.d_totalQueueMessages))
, d_messages(bsl::move(original.d_messages), basicAllocator)
{
}
#endif

QueueContents::~QueueContents()
{
}

// MANIPULATORS

QueueContents& QueueContents::operator=(const QueueContents& rhs)
{
    if (this != &rhs) {
        d_messages           = rhs.d_messages;
        d_offset             = rhs.d_offset;
        d_totalQueueMessages = rhs.d_totalQueueMessages;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueContents& QueueContents::operator=(QueueContents&& rhs)
{
    if (this != &rhs) {
        d_messages           = bsl::move(rhs.d_messages);
        d_offset             = bsl::move(rhs.d_offset);
        d_totalQueueMessages = bsl::move(rhs.d_totalQueueMessages);
    }

    return *this;
}
#endif

void QueueContents::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_messages);
    bdlat_ValueTypeFunctions::reset(&d_offset);
    bdlat_ValueTypeFunctions::reset(&d_totalQueueMessages);
}

// ACCESSORS

bsl::ostream&
QueueContents::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("messages", this->messages());
    printer.printAttribute("offset", this->offset());
    printer.printAttribute("totalQueueMessages", this->totalQueueMessages());
    printer.end();
    return stream;
}

// ------------------
// class QueueStorage
// ------------------

// CONSTANTS

const char QueueStorage::CLASS_NAME[] = "QueueStorage";

const bdlat_AttributeInfo QueueStorage::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NUM_MESSAGES,
     "numMessages",
     sizeof("numMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_BYTES,
     "numBytes",
     sizeof("numBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_VIRTUAL_STORAGES,
     "virtualStorages",
     sizeof("virtualStorages") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* QueueStorage::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueStorage::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueStorage::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NUM_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_MESSAGES];
    case ATTRIBUTE_ID_NUM_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_BYTES];
    case ATTRIBUTE_ID_VIRTUAL_STORAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VIRTUAL_STORAGES];
    default: return 0;
    }
}

// CREATORS

QueueStorage::QueueStorage(bslma::Allocator* basicAllocator)
: d_virtualStorages(basicAllocator)
, d_numMessages()
, d_numBytes()
{
}

QueueStorage::QueueStorage(const QueueStorage& original,
                           bslma::Allocator*   basicAllocator)
: d_virtualStorages(original.d_virtualStorages, basicAllocator)
, d_numMessages(original.d_numMessages)
, d_numBytes(original.d_numBytes)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueStorage::QueueStorage(QueueStorage&& original) noexcept
: d_virtualStorages(bsl::move(original.d_virtualStorages)),
  d_numMessages(bsl::move(original.d_numMessages)),
  d_numBytes(bsl::move(original.d_numBytes))
{
}

QueueStorage::QueueStorage(QueueStorage&&    original,
                           bslma::Allocator* basicAllocator)
: d_virtualStorages(bsl::move(original.d_virtualStorages), basicAllocator)
, d_numMessages(bsl::move(original.d_numMessages))
, d_numBytes(bsl::move(original.d_numBytes))
{
}
#endif

QueueStorage::~QueueStorage()
{
}

// MANIPULATORS

QueueStorage& QueueStorage::operator=(const QueueStorage& rhs)
{
    if (this != &rhs) {
        d_numMessages     = rhs.d_numMessages;
        d_numBytes        = rhs.d_numBytes;
        d_virtualStorages = rhs.d_virtualStorages;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueStorage& QueueStorage::operator=(QueueStorage&& rhs)
{
    if (this != &rhs) {
        d_numMessages     = bsl::move(rhs.d_numMessages);
        d_numBytes        = bsl::move(rhs.d_numBytes);
        d_virtualStorages = bsl::move(rhs.d_virtualStorages);
    }

    return *this;
}
#endif

void QueueStorage::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_numMessages);
    bdlat_ValueTypeFunctions::reset(&d_numBytes);
    bdlat_ValueTypeFunctions::reset(&d_virtualStorages);
}

// ACCESSORS

bsl::ostream&
QueueStorage::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("numMessages", this->numMessages());
    printer.printAttribute("numBytes", this->numBytes());
    printer.printAttribute("virtualStorages", this->virtualStorages());
    printer.end();
    return stream;
}

// --------------------------
// class ResourceUsageMonitor
// --------------------------

// CONSTANTS

const char ResourceUsageMonitor::CLASS_NAME[] = "ResourceUsageMonitor";

const bdlat_AttributeInfo ResourceUsageMonitor::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_STATE,
     "state",
     sizeof("state") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_MESSAGES_STATE,
     "messagesState",
     sizeof("messagesState") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_NUM_MESSAGES,
     "numMessages",
     sizeof("numMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MESSAGES_LOW_WATERMARK_RATIO,
     "messagesLowWatermarkRatio",
     sizeof("messagesLowWatermarkRatio") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MESSAGES_HIGH_WATERMARK_RATIO,
     "messagesHighWatermarkRatio",
     sizeof("messagesHighWatermarkRatio") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MESSAGES_CAPACITY,
     "messagesCapacity",
     sizeof("messagesCapacity") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BYTES_STATE,
     "bytesState",
     sizeof("bytesState") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_NUM_BYTES,
     "numBytes",
     sizeof("numBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BYTES_LOW_WATERMARK_RATIO,
     "bytesLowWatermarkRatio",
     sizeof("bytesLowWatermarkRatio") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BYTES_HIGH_WATERMARK_RATIO,
     "bytesHighWatermarkRatio",
     sizeof("bytesHighWatermarkRatio") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BYTES_CAPACITY,
     "bytesCapacity",
     sizeof("bytesCapacity") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
ResourceUsageMonitor::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 11; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ResourceUsageMonitor::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ResourceUsageMonitor::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_STATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATE];
    case ATTRIBUTE_ID_MESSAGES_STATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES_STATE];
    case ATTRIBUTE_ID_NUM_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_MESSAGES];
    case ATTRIBUTE_ID_MESSAGES_LOW_WATERMARK_RATIO:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_MESSAGES_LOW_WATERMARK_RATIO];
    case ATTRIBUTE_ID_MESSAGES_HIGH_WATERMARK_RATIO:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_MESSAGES_HIGH_WATERMARK_RATIO];
    case ATTRIBUTE_ID_MESSAGES_CAPACITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES_CAPACITY];
    case ATTRIBUTE_ID_BYTES_STATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES_STATE];
    case ATTRIBUTE_ID_NUM_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_BYTES];
    case ATTRIBUTE_ID_BYTES_LOW_WATERMARK_RATIO:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_BYTES_LOW_WATERMARK_RATIO];
    case ATTRIBUTE_ID_BYTES_HIGH_WATERMARK_RATIO:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_BYTES_HIGH_WATERMARK_RATIO];
    case ATTRIBUTE_ID_BYTES_CAPACITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES_CAPACITY];
    default: return 0;
    }
}

// CREATORS

ResourceUsageMonitor::ResourceUsageMonitor()
: d_numMessages()
, d_messagesLowWatermarkRatio()
, d_messagesHighWatermarkRatio()
, d_messagesCapacity()
, d_numBytes()
, d_bytesLowWatermarkRatio()
, d_bytesHighWatermarkRatio()
, d_bytesCapacity()
, d_state(static_cast<ResourceUsageMonitorState::Value>(0))
, d_messagesState(static_cast<ResourceUsageMonitorState::Value>(0))
, d_bytesState(static_cast<ResourceUsageMonitorState::Value>(0))
{
}

// MANIPULATORS

void ResourceUsageMonitor::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_state);
    bdlat_ValueTypeFunctions::reset(&d_messagesState);
    bdlat_ValueTypeFunctions::reset(&d_numMessages);
    bdlat_ValueTypeFunctions::reset(&d_messagesLowWatermarkRatio);
    bdlat_ValueTypeFunctions::reset(&d_messagesHighWatermarkRatio);
    bdlat_ValueTypeFunctions::reset(&d_messagesCapacity);
    bdlat_ValueTypeFunctions::reset(&d_bytesState);
    bdlat_ValueTypeFunctions::reset(&d_numBytes);
    bdlat_ValueTypeFunctions::reset(&d_bytesLowWatermarkRatio);
    bdlat_ValueTypeFunctions::reset(&d_bytesHighWatermarkRatio);
    bdlat_ValueTypeFunctions::reset(&d_bytesCapacity);
}

// ACCESSORS

bsl::ostream& ResourceUsageMonitor::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("state", this->state());
    printer.printAttribute("messagesState", this->messagesState());
    printer.printAttribute("numMessages", this->numMessages());
    printer.printAttribute("messagesLowWatermarkRatio",
                           this->messagesLowWatermarkRatio());
    printer.printAttribute("messagesHighWatermarkRatio",
                           this->messagesHighWatermarkRatio());
    printer.printAttribute("messagesCapacity", this->messagesCapacity());
    printer.printAttribute("bytesState", this->bytesState());
    printer.printAttribute("numBytes", this->numBytes());
    printer.printAttribute("bytesLowWatermarkRatio",
                           this->bytesLowWatermarkRatio());
    printer.printAttribute("bytesHighWatermarkRatio",
                           this->bytesHighWatermarkRatio());
    printer.printAttribute("bytesCapacity", this->bytesCapacity());
    printer.end();
    return stream;
}

// -----------------------
// class RouteResponseList
// -----------------------

// CONSTANTS

const char RouteResponseList::CLASS_NAME[] = "RouteResponseList";

const bdlat_AttributeInfo RouteResponseList::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_RESPONSES,
     "responses",
     sizeof("responses") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
RouteResponseList::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RouteResponseList::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RouteResponseList::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_RESPONSES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESPONSES];
    default: return 0;
    }
}

// CREATORS

RouteResponseList::RouteResponseList(bslma::Allocator* basicAllocator)
: d_responses(basicAllocator)
{
}

RouteResponseList::RouteResponseList(const RouteResponseList& original,
                                     bslma::Allocator*        basicAllocator)
: d_responses(original.d_responses, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouteResponseList::RouteResponseList(RouteResponseList&& original) noexcept
: d_responses(bsl::move(original.d_responses))
{
}

RouteResponseList::RouteResponseList(RouteResponseList&& original,
                                     bslma::Allocator*   basicAllocator)
: d_responses(bsl::move(original.d_responses), basicAllocator)
{
}
#endif

RouteResponseList::~RouteResponseList()
{
}

// MANIPULATORS

RouteResponseList& RouteResponseList::operator=(const RouteResponseList& rhs)
{
    if (this != &rhs) {
        d_responses = rhs.d_responses;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouteResponseList& RouteResponseList::operator=(RouteResponseList&& rhs)
{
    if (this != &rhs) {
        d_responses = bsl::move(rhs.d_responses);
    }

    return *this;
}
#endif

void RouteResponseList::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_responses);
}

// ACCESSORS

bsl::ostream& RouteResponseList::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("responses", this->responses());
    printer.end();
    return stream;
}

// ----------------------
// class SetTunableChoice
// ----------------------

// CONSTANTS

const char SetTunableChoice::CLASS_NAME[] = "SetTunableChoice";

const bdlat_SelectionInfo SetTunableChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ALL,
     "all",
     sizeof("all") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SELF,
     "self",
     sizeof("self") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
SetTunableChoice::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            SetTunableChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* SetTunableChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ALL: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ALL];
    case SELECTION_ID_SELF: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SELF];
    default: return 0;
    }
}

// CREATORS

SetTunableChoice::SetTunableChoice(const SetTunableChoice& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_ALL: {
        new (d_all.buffer()) Void(original.d_all.object());
    } break;
    case SELECTION_ID_SELF: {
        new (d_self.buffer()) Void(original.d_self.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SetTunableChoice::SetTunableChoice(SetTunableChoice&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_ALL: {
        new (d_all.buffer()) Void(bsl::move(original.d_all.object()));
    } break;
    case SELECTION_ID_SELF: {
        new (d_self.buffer()) Void(bsl::move(original.d_self.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

SetTunableChoice& SetTunableChoice::operator=(const SetTunableChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ALL: {
            makeAll(rhs.d_all.object());
        } break;
        case SELECTION_ID_SELF: {
            makeSelf(rhs.d_self.object());
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
SetTunableChoice& SetTunableChoice::operator=(SetTunableChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ALL: {
            makeAll(bsl::move(rhs.d_all.object()));
        } break;
        case SELECTION_ID_SELF: {
            makeSelf(bsl::move(rhs.d_self.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void SetTunableChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ALL: {
        d_all.object().~Void();
    } break;
    case SELECTION_ID_SELF: {
        d_self.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int SetTunableChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ALL: {
        makeAll();
    } break;
    case SELECTION_ID_SELF: {
        makeSelf();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int SetTunableChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& SetTunableChoice::makeAll()
{
    if (SELECTION_ID_ALL == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_all.object());
    }
    else {
        reset();
        new (d_all.buffer()) Void();
        d_selectionId = SELECTION_ID_ALL;
    }

    return d_all.object();
}

Void& SetTunableChoice::makeAll(const Void& value)
{
    if (SELECTION_ID_ALL == d_selectionId) {
        d_all.object() = value;
    }
    else {
        reset();
        new (d_all.buffer()) Void(value);
        d_selectionId = SELECTION_ID_ALL;
    }

    return d_all.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& SetTunableChoice::makeAll(Void&& value)
{
    if (SELECTION_ID_ALL == d_selectionId) {
        d_all.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_all.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_ALL;
    }

    return d_all.object();
}
#endif

Void& SetTunableChoice::makeSelf()
{
    if (SELECTION_ID_SELF == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_self.object());
    }
    else {
        reset();
        new (d_self.buffer()) Void();
        d_selectionId = SELECTION_ID_SELF;
    }

    return d_self.object();
}

Void& SetTunableChoice::makeSelf(const Void& value)
{
    if (SELECTION_ID_SELF == d_selectionId) {
        d_self.object() = value;
    }
    else {
        reset();
        new (d_self.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SELF;
    }

    return d_self.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& SetTunableChoice::makeSelf(Void&& value)
{
    if (SELECTION_ID_SELF == d_selectionId) {
        d_self.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_self.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SELF;
    }

    return d_self.object();
}
#endif

// ACCESSORS

bsl::ostream& SetTunableChoice::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ALL: {
        printer.printAttribute("all", d_all.object());
    } break;
    case SELECTION_ID_SELF: {
        printer.printAttribute("self", d_self.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* SetTunableChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ALL:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ALL].name();
    case SELECTION_ID_SELF:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SELF].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------------
// class StorageContent
// --------------------

// CONSTANTS

const char StorageContent::CLASS_NAME[] = "StorageContent";

const bdlat_AttributeInfo StorageContent::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_STORAGES,
     "storages",
     sizeof("storages") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StorageContent::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StorageContent::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StorageContent::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_STORAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGES];
    default: return 0;
    }
}

// CREATORS

StorageContent::StorageContent(bslma::Allocator* basicAllocator)
: d_storages(basicAllocator)
{
}

StorageContent::StorageContent(const StorageContent& original,
                               bslma::Allocator*     basicAllocator)
: d_storages(original.d_storages, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageContent::StorageContent(StorageContent&& original) noexcept
: d_storages(bsl::move(original.d_storages))
{
}

StorageContent::StorageContent(StorageContent&&  original,
                               bslma::Allocator* basicAllocator)
: d_storages(bsl::move(original.d_storages), basicAllocator)
{
}
#endif

StorageContent::~StorageContent()
{
}

// MANIPULATORS

StorageContent& StorageContent::operator=(const StorageContent& rhs)
{
    if (this != &rhs) {
        d_storages = rhs.d_storages;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageContent& StorageContent::operator=(StorageContent&& rhs)
{
    if (this != &rhs) {
        d_storages = bsl::move(rhs.d_storages);
    }

    return *this;
}
#endif

void StorageContent::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_storages);
}

// ACCESSORS

bsl::ostream& StorageContent::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("storages", this->storages());
    printer.end();
    return stream;
}

// --------------------------
// class StorageDomainCommand
// --------------------------

// CONSTANTS

const char StorageDomainCommand::CLASS_NAME[] = "StorageDomainCommand";

const bdlat_SelectionInfo StorageDomainCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_QUEUE_STATUS,
     "queueStatus",
     sizeof("queueStatus") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PURGE,
     "purge",
     sizeof("purge") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
StorageDomainCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            StorageDomainCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* StorageDomainCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_QUEUE_STATUS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_STATUS];
    case SELECTION_ID_PURGE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGE];
    default: return 0;
    }
}

// CREATORS

StorageDomainCommand::StorageDomainCommand(
    const StorageDomainCommand& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_QUEUE_STATUS: {
        new (d_queueStatus.buffer()) Void(original.d_queueStatus.object());
    } break;
    case SELECTION_ID_PURGE: {
        new (d_purge.buffer()) Void(original.d_purge.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageDomainCommand::StorageDomainCommand(StorageDomainCommand&& original)
    noexcept : d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_QUEUE_STATUS: {
        new (d_queueStatus.buffer())
            Void(bsl::move(original.d_queueStatus.object()));
    } break;
    case SELECTION_ID_PURGE: {
        new (d_purge.buffer()) Void(bsl::move(original.d_purge.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

StorageDomainCommand&
StorageDomainCommand::operator=(const StorageDomainCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_QUEUE_STATUS: {
            makeQueueStatus(rhs.d_queueStatus.object());
        } break;
        case SELECTION_ID_PURGE: {
            makePurge(rhs.d_purge.object());
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
StorageDomainCommand&
StorageDomainCommand::operator=(StorageDomainCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_QUEUE_STATUS: {
            makeQueueStatus(bsl::move(rhs.d_queueStatus.object()));
        } break;
        case SELECTION_ID_PURGE: {
            makePurge(bsl::move(rhs.d_purge.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void StorageDomainCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_QUEUE_STATUS: {
        d_queueStatus.object().~Void();
    } break;
    case SELECTION_ID_PURGE: {
        d_purge.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int StorageDomainCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_QUEUE_STATUS: {
        makeQueueStatus();
    } break;
    case SELECTION_ID_PURGE: {
        makePurge();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int StorageDomainCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& StorageDomainCommand::makeQueueStatus()
{
    if (SELECTION_ID_QUEUE_STATUS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueStatus.object());
    }
    else {
        reset();
        new (d_queueStatus.buffer()) Void();
        d_selectionId = SELECTION_ID_QUEUE_STATUS;
    }

    return d_queueStatus.object();
}

Void& StorageDomainCommand::makeQueueStatus(const Void& value)
{
    if (SELECTION_ID_QUEUE_STATUS == d_selectionId) {
        d_queueStatus.object() = value;
    }
    else {
        reset();
        new (d_queueStatus.buffer()) Void(value);
        d_selectionId = SELECTION_ID_QUEUE_STATUS;
    }

    return d_queueStatus.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& StorageDomainCommand::makeQueueStatus(Void&& value)
{
    if (SELECTION_ID_QUEUE_STATUS == d_selectionId) {
        d_queueStatus.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueStatus.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_QUEUE_STATUS;
    }

    return d_queueStatus.object();
}
#endif

Void& StorageDomainCommand::makePurge()
{
    if (SELECTION_ID_PURGE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_purge.object());
    }
    else {
        reset();
        new (d_purge.buffer()) Void();
        d_selectionId = SELECTION_ID_PURGE;
    }

    return d_purge.object();
}

Void& StorageDomainCommand::makePurge(const Void& value)
{
    if (SELECTION_ID_PURGE == d_selectionId) {
        d_purge.object() = value;
    }
    else {
        reset();
        new (d_purge.buffer()) Void(value);
        d_selectionId = SELECTION_ID_PURGE;
    }

    return d_purge.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& StorageDomainCommand::makePurge(Void&& value)
{
    if (SELECTION_ID_PURGE == d_selectionId) {
        d_purge.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_purge.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_PURGE;
    }

    return d_purge.object();
}
#endif

// ACCESSORS

bsl::ostream& StorageDomainCommand::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_QUEUE_STATUS: {
        printer.printAttribute("queueStatus", d_queueStatus.object());
    } break;
    case SELECTION_ID_PURGE: {
        printer.printAttribute("purge", d_purge.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* StorageDomainCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_QUEUE_STATUS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_STATUS].name();
    case SELECTION_ID_PURGE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------------------
// class StoragePartitionCommand
// -----------------------------

// CONSTANTS

const char StoragePartitionCommand::CLASS_NAME[] = "StoragePartitionCommand";

const bdlat_SelectionInfo StoragePartitionCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ENABLE,
     "enable",
     sizeof("enable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DISABLE,
     "disable",
     sizeof("disable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SUMMARY,
     "summary",
     sizeof("summary") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
StoragePartitionCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            StoragePartitionCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* StoragePartitionCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ENABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ENABLE];
    case SELECTION_ID_DISABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DISABLE];
    case SELECTION_ID_SUMMARY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SUMMARY];
    default: return 0;
    }
}

// CREATORS

StoragePartitionCommand::StoragePartitionCommand(
    const StoragePartitionCommand& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_ENABLE: {
        new (d_enable.buffer()) Void(original.d_enable.object());
    } break;
    case SELECTION_ID_DISABLE: {
        new (d_disable.buffer()) Void(original.d_disable.object());
    } break;
    case SELECTION_ID_SUMMARY: {
        new (d_summary.buffer()) Void(original.d_summary.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StoragePartitionCommand::StoragePartitionCommand(
    StoragePartitionCommand&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_ENABLE: {
        new (d_enable.buffer()) Void(bsl::move(original.d_enable.object()));
    } break;
    case SELECTION_ID_DISABLE: {
        new (d_disable.buffer()) Void(bsl::move(original.d_disable.object()));
    } break;
    case SELECTION_ID_SUMMARY: {
        new (d_summary.buffer()) Void(bsl::move(original.d_summary.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

StoragePartitionCommand&
StoragePartitionCommand::operator=(const StoragePartitionCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ENABLE: {
            makeEnable(rhs.d_enable.object());
        } break;
        case SELECTION_ID_DISABLE: {
            makeDisable(rhs.d_disable.object());
        } break;
        case SELECTION_ID_SUMMARY: {
            makeSummary(rhs.d_summary.object());
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
StoragePartitionCommand&
StoragePartitionCommand::operator=(StoragePartitionCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ENABLE: {
            makeEnable(bsl::move(rhs.d_enable.object()));
        } break;
        case SELECTION_ID_DISABLE: {
            makeDisable(bsl::move(rhs.d_disable.object()));
        } break;
        case SELECTION_ID_SUMMARY: {
            makeSummary(bsl::move(rhs.d_summary.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void StoragePartitionCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ENABLE: {
        d_enable.object().~Void();
    } break;
    case SELECTION_ID_DISABLE: {
        d_disable.object().~Void();
    } break;
    case SELECTION_ID_SUMMARY: {
        d_summary.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int StoragePartitionCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ENABLE: {
        makeEnable();
    } break;
    case SELECTION_ID_DISABLE: {
        makeDisable();
    } break;
    case SELECTION_ID_SUMMARY: {
        makeSummary();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int StoragePartitionCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& StoragePartitionCommand::makeEnable()
{
    if (SELECTION_ID_ENABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_enable.object());
    }
    else {
        reset();
        new (d_enable.buffer()) Void();
        d_selectionId = SELECTION_ID_ENABLE;
    }

    return d_enable.object();
}

Void& StoragePartitionCommand::makeEnable(const Void& value)
{
    if (SELECTION_ID_ENABLE == d_selectionId) {
        d_enable.object() = value;
    }
    else {
        reset();
        new (d_enable.buffer()) Void(value);
        d_selectionId = SELECTION_ID_ENABLE;
    }

    return d_enable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& StoragePartitionCommand::makeEnable(Void&& value)
{
    if (SELECTION_ID_ENABLE == d_selectionId) {
        d_enable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_enable.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_ENABLE;
    }

    return d_enable.object();
}
#endif

Void& StoragePartitionCommand::makeDisable()
{
    if (SELECTION_ID_DISABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_disable.object());
    }
    else {
        reset();
        new (d_disable.buffer()) Void();
        d_selectionId = SELECTION_ID_DISABLE;
    }

    return d_disable.object();
}

Void& StoragePartitionCommand::makeDisable(const Void& value)
{
    if (SELECTION_ID_DISABLE == d_selectionId) {
        d_disable.object() = value;
    }
    else {
        reset();
        new (d_disable.buffer()) Void(value);
        d_selectionId = SELECTION_ID_DISABLE;
    }

    return d_disable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& StoragePartitionCommand::makeDisable(Void&& value)
{
    if (SELECTION_ID_DISABLE == d_selectionId) {
        d_disable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_disable.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_DISABLE;
    }

    return d_disable.object();
}
#endif

Void& StoragePartitionCommand::makeSummary()
{
    if (SELECTION_ID_SUMMARY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_summary.object());
    }
    else {
        reset();
        new (d_summary.buffer()) Void();
        d_selectionId = SELECTION_ID_SUMMARY;
    }

    return d_summary.object();
}

Void& StoragePartitionCommand::makeSummary(const Void& value)
{
    if (SELECTION_ID_SUMMARY == d_selectionId) {
        d_summary.object() = value;
    }
    else {
        reset();
        new (d_summary.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SUMMARY;
    }

    return d_summary.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& StoragePartitionCommand::makeSummary(Void&& value)
{
    if (SELECTION_ID_SUMMARY == d_selectionId) {
        d_summary.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_summary.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SUMMARY;
    }

    return d_summary.object();
}
#endif

// ACCESSORS

bsl::ostream& StoragePartitionCommand::print(bsl::ostream& stream,
                                             int           level,
                                             int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ENABLE: {
        printer.printAttribute("enable", d_enable.object());
    } break;
    case SELECTION_ID_DISABLE: {
        printer.printAttribute("disable", d_disable.object());
    } break;
    case SELECTION_ID_SUMMARY: {
        printer.printAttribute("summary", d_summary.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* StoragePartitionCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ENABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ENABLE].name();
    case SELECTION_ID_DISABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DISABLE].name();
    case SELECTION_ID_SUMMARY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SUMMARY].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ------------------
// class StorageQueue
// ------------------

// CONSTANTS

const char StorageQueue::CLASS_NAME[] = "StorageQueue";

const bdlat_AttributeInfo StorageQueue::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CANONICAL_URI,
     "canonicalUri",
     sizeof("canonicalUri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_COMMAND,
     "command",
     sizeof("command") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* StorageQueue::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StorageQueue::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StorageQueue::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CANONICAL_URI:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CANONICAL_URI];
    case ATTRIBUTE_ID_COMMAND:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND];
    default: return 0;
    }
}

// CREATORS

StorageQueue::StorageQueue(bslma::Allocator* basicAllocator)
: d_canonicalUri(basicAllocator)
, d_command(basicAllocator)
{
}

StorageQueue::StorageQueue(const StorageQueue& original,
                           bslma::Allocator*   basicAllocator)
: d_canonicalUri(original.d_canonicalUri, basicAllocator)
, d_command(original.d_command, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageQueue::StorageQueue(StorageQueue&& original) noexcept
: d_canonicalUri(bsl::move(original.d_canonicalUri)),
  d_command(bsl::move(original.d_command))
{
}

StorageQueue::StorageQueue(StorageQueue&&    original,
                           bslma::Allocator* basicAllocator)
: d_canonicalUri(bsl::move(original.d_canonicalUri), basicAllocator)
, d_command(bsl::move(original.d_command), basicAllocator)
{
}
#endif

StorageQueue::~StorageQueue()
{
}

// MANIPULATORS

StorageQueue& StorageQueue::operator=(const StorageQueue& rhs)
{
    if (this != &rhs) {
        d_canonicalUri = rhs.d_canonicalUri;
        d_command      = rhs.d_command;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageQueue& StorageQueue::operator=(StorageQueue&& rhs)
{
    if (this != &rhs) {
        d_canonicalUri = bsl::move(rhs.d_canonicalUri);
        d_command      = bsl::move(rhs.d_command);
    }

    return *this;
}
#endif

void StorageQueue::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_canonicalUri);
    bdlat_ValueTypeFunctions::reset(&d_command);
}

// ACCESSORS

bsl::ostream&
StorageQueue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("canonicalUri", this->canonicalUri());
    printer.printAttribute("command", this->command());
    printer.end();
    return stream;
}

// ------------------
// class Subscription
// ------------------

// CONSTANTS

const char Subscription::CLASS_NAME[] = "Subscription";

const bdlat_AttributeInfo Subscription::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_DOWNSTREAM_SUBSCRIPTION_ID,
     "downstreamSubscriptionId",
     sizeof("downstreamSubscriptionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CONSUMER,
     "consumer",
     sizeof("consumer") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_SUBSCRIBER,
     "subscriber",
     sizeof("subscriber") - 1,
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
    case ATTRIBUTE_ID_DOWNSTREAM_SUBSCRIPTION_ID:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_DOWNSTREAM_SUBSCRIPTION_ID];
    case ATTRIBUTE_ID_CONSUMER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER];
    case ATTRIBUTE_ID_SUBSCRIBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIBER];
    default: return 0;
    }
}

// CREATORS

Subscription::Subscription()
: d_subscriber()
, d_consumer()
, d_downstreamSubscriptionId()
{
}

// MANIPULATORS

void Subscription::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_downstreamSubscriptionId);
    bdlat_ValueTypeFunctions::reset(&d_consumer);
    bdlat_ValueTypeFunctions::reset(&d_subscriber);
}

// ACCESSORS

bsl::ostream&
Subscription::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("downstreamSubscriptionId",
                           this->downstreamSubscriptionId());
    printer.printAttribute("consumer", this->consumer());
    printer.printAttribute("subscriber", this->subscriber());
    printer.end();
    return stream;
}

// -----------
// class Value
// -----------

// CONSTANTS

const char Value::CLASS_NAME[] = "Value";

const bdlat_SelectionInfo Value::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_THE_NULL,
     "theNull",
     sizeof("theNull") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_THE_BOOL,
     "theBool",
     sizeof("theBool") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {SELECTION_ID_THE_INTEGER,
     "theInteger",
     sizeof("theInteger") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {SELECTION_ID_THE_DOUBLE,
     "theDouble",
     sizeof("theDouble") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_THE_DATE,
     "theDate",
     sizeof("theDate") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_THE_TIME,
     "theTime",
     sizeof("theTime") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_THE_DATETIME,
     "theDatetime",
     sizeof("theDatetime") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_THE_STRING,
     "theString",
     sizeof("theString") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_SelectionInfo* Value::lookupSelectionInfo(const char* name,
                                                      int         nameLength)
{
    for (int i = 0; i < 8; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            Value::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* Value::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_THE_NULL:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_NULL];
    case SELECTION_ID_THE_BOOL:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_BOOL];
    case SELECTION_ID_THE_INTEGER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_INTEGER];
    case SELECTION_ID_THE_DOUBLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_DOUBLE];
    case SELECTION_ID_THE_DATE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_DATE];
    case SELECTION_ID_THE_TIME:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_TIME];
    case SELECTION_ID_THE_DATETIME:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_DATETIME];
    case SELECTION_ID_THE_STRING:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_STRING];
    default: return 0;
    }
}

// CREATORS

Value::Value(const Value& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_THE_NULL: {
        new (d_theNull.buffer()) Void(original.d_theNull.object());
    } break;
    case SELECTION_ID_THE_BOOL: {
        new (d_theBool.buffer()) bool(original.d_theBool.object());
    } break;
    case SELECTION_ID_THE_INTEGER: {
        new (d_theInteger.buffer())
            bsls::Types::Int64(original.d_theInteger.object());
    } break;
    case SELECTION_ID_THE_DOUBLE: {
        new (d_theDouble.buffer()) double(original.d_theDouble.object());
    } break;
    case SELECTION_ID_THE_DATE: {
        new (d_theDate.buffer()) bdlt::DateTz(original.d_theDate.object());
    } break;
    case SELECTION_ID_THE_TIME: {
        new (d_theTime.buffer()) bdlt::TimeTz(original.d_theTime.object());
    } break;
    case SELECTION_ID_THE_DATETIME: {
        new (d_theDatetime.buffer())
            bdlt::DatetimeTz(original.d_theDatetime.object());
    } break;
    case SELECTION_ID_THE_STRING: {
        new (d_theString.buffer())
            bsl::string(original.d_theString.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Value::Value(Value&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_THE_NULL: {
        new (d_theNull.buffer()) Void(bsl::move(original.d_theNull.object()));
    } break;
    case SELECTION_ID_THE_BOOL: {
        new (d_theBool.buffer()) bool(bsl::move(original.d_theBool.object()));
    } break;
    case SELECTION_ID_THE_INTEGER: {
        new (d_theInteger.buffer())
            bsls::Types::Int64(bsl::move(original.d_theInteger.object()));
    } break;
    case SELECTION_ID_THE_DOUBLE: {
        new (d_theDouble.buffer()) double(
            bsl::move(original.d_theDouble.object()));
    } break;
    case SELECTION_ID_THE_DATE: {
        new (d_theDate.buffer())
            bdlt::DateTz(bsl::move(original.d_theDate.object()));
    } break;
    case SELECTION_ID_THE_TIME: {
        new (d_theTime.buffer())
            bdlt::TimeTz(bsl::move(original.d_theTime.object()));
    } break;
    case SELECTION_ID_THE_DATETIME: {
        new (d_theDatetime.buffer())
            bdlt::DatetimeTz(bsl::move(original.d_theDatetime.object()));
    } break;
    case SELECTION_ID_THE_STRING: {
        new (d_theString.buffer())
            bsl::string(bsl::move(original.d_theString.object()),
                        d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

Value::Value(Value&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_THE_NULL: {
        new (d_theNull.buffer()) Void(bsl::move(original.d_theNull.object()));
    } break;
    case SELECTION_ID_THE_BOOL: {
        new (d_theBool.buffer()) bool(bsl::move(original.d_theBool.object()));
    } break;
    case SELECTION_ID_THE_INTEGER: {
        new (d_theInteger.buffer())
            bsls::Types::Int64(bsl::move(original.d_theInteger.object()));
    } break;
    case SELECTION_ID_THE_DOUBLE: {
        new (d_theDouble.buffer()) double(
            bsl::move(original.d_theDouble.object()));
    } break;
    case SELECTION_ID_THE_DATE: {
        new (d_theDate.buffer())
            bdlt::DateTz(bsl::move(original.d_theDate.object()));
    } break;
    case SELECTION_ID_THE_TIME: {
        new (d_theTime.buffer())
            bdlt::TimeTz(bsl::move(original.d_theTime.object()));
    } break;
    case SELECTION_ID_THE_DATETIME: {
        new (d_theDatetime.buffer())
            bdlt::DatetimeTz(bsl::move(original.d_theDatetime.object()));
    } break;
    case SELECTION_ID_THE_STRING: {
        new (d_theString.buffer())
            bsl::string(bsl::move(original.d_theString.object()),
                        d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

Value& Value::operator=(const Value& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_THE_NULL: {
            makeTheNull(rhs.d_theNull.object());
        } break;
        case SELECTION_ID_THE_BOOL: {
            makeTheBool(rhs.d_theBool.object());
        } break;
        case SELECTION_ID_THE_INTEGER: {
            makeTheInteger(rhs.d_theInteger.object());
        } break;
        case SELECTION_ID_THE_DOUBLE: {
            makeTheDouble(rhs.d_theDouble.object());
        } break;
        case SELECTION_ID_THE_DATE: {
            makeTheDate(rhs.d_theDate.object());
        } break;
        case SELECTION_ID_THE_TIME: {
            makeTheTime(rhs.d_theTime.object());
        } break;
        case SELECTION_ID_THE_DATETIME: {
            makeTheDatetime(rhs.d_theDatetime.object());
        } break;
        case SELECTION_ID_THE_STRING: {
            makeTheString(rhs.d_theString.object());
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
Value& Value::operator=(Value&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_THE_NULL: {
            makeTheNull(bsl::move(rhs.d_theNull.object()));
        } break;
        case SELECTION_ID_THE_BOOL: {
            makeTheBool(bsl::move(rhs.d_theBool.object()));
        } break;
        case SELECTION_ID_THE_INTEGER: {
            makeTheInteger(bsl::move(rhs.d_theInteger.object()));
        } break;
        case SELECTION_ID_THE_DOUBLE: {
            makeTheDouble(bsl::move(rhs.d_theDouble.object()));
        } break;
        case SELECTION_ID_THE_DATE: {
            makeTheDate(bsl::move(rhs.d_theDate.object()));
        } break;
        case SELECTION_ID_THE_TIME: {
            makeTheTime(bsl::move(rhs.d_theTime.object()));
        } break;
        case SELECTION_ID_THE_DATETIME: {
            makeTheDatetime(bsl::move(rhs.d_theDatetime.object()));
        } break;
        case SELECTION_ID_THE_STRING: {
            makeTheString(bsl::move(rhs.d_theString.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void Value::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_THE_NULL: {
        d_theNull.object().~Void();
    } break;
    case SELECTION_ID_THE_BOOL: {
        // no destruction required
    } break;
    case SELECTION_ID_THE_INTEGER: {
        // no destruction required
    } break;
    case SELECTION_ID_THE_DOUBLE: {
        // no destruction required
    } break;
    case SELECTION_ID_THE_DATE: {
        // no destruction required
    } break;
    case SELECTION_ID_THE_TIME: {
        // no destruction required
    } break;
    case SELECTION_ID_THE_DATETIME: {
        // no destruction required
    } break;
    case SELECTION_ID_THE_STRING: {
        typedef bsl::string Type;
        d_theString.object().~Type();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int Value::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_THE_NULL: {
        makeTheNull();
    } break;
    case SELECTION_ID_THE_BOOL: {
        makeTheBool();
    } break;
    case SELECTION_ID_THE_INTEGER: {
        makeTheInteger();
    } break;
    case SELECTION_ID_THE_DOUBLE: {
        makeTheDouble();
    } break;
    case SELECTION_ID_THE_DATE: {
        makeTheDate();
    } break;
    case SELECTION_ID_THE_TIME: {
        makeTheTime();
    } break;
    case SELECTION_ID_THE_DATETIME: {
        makeTheDatetime();
    } break;
    case SELECTION_ID_THE_STRING: {
        makeTheString();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int Value::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& Value::makeTheNull()
{
    if (SELECTION_ID_THE_NULL == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_theNull.object());
    }
    else {
        reset();
        new (d_theNull.buffer()) Void();
        d_selectionId = SELECTION_ID_THE_NULL;
    }

    return d_theNull.object();
}

Void& Value::makeTheNull(const Void& value)
{
    if (SELECTION_ID_THE_NULL == d_selectionId) {
        d_theNull.object() = value;
    }
    else {
        reset();
        new (d_theNull.buffer()) Void(value);
        d_selectionId = SELECTION_ID_THE_NULL;
    }

    return d_theNull.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& Value::makeTheNull(Void&& value)
{
    if (SELECTION_ID_THE_NULL == d_selectionId) {
        d_theNull.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_theNull.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_THE_NULL;
    }

    return d_theNull.object();
}
#endif

bool& Value::makeTheBool()
{
    if (SELECTION_ID_THE_BOOL == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_theBool.object());
    }
    else {
        reset();
        new (d_theBool.buffer()) bool();
        d_selectionId = SELECTION_ID_THE_BOOL;
    }

    return d_theBool.object();
}

bool& Value::makeTheBool(bool value)
{
    if (SELECTION_ID_THE_BOOL == d_selectionId) {
        d_theBool.object() = value;
    }
    else {
        reset();
        new (d_theBool.buffer()) bool(value);
        d_selectionId = SELECTION_ID_THE_BOOL;
    }

    return d_theBool.object();
}

bsls::Types::Int64& Value::makeTheInteger()
{
    if (SELECTION_ID_THE_INTEGER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_theInteger.object());
    }
    else {
        reset();
        new (d_theInteger.buffer()) bsls::Types::Int64();
        d_selectionId = SELECTION_ID_THE_INTEGER;
    }

    return d_theInteger.object();
}

bsls::Types::Int64& Value::makeTheInteger(bsls::Types::Int64 value)
{
    if (SELECTION_ID_THE_INTEGER == d_selectionId) {
        d_theInteger.object() = value;
    }
    else {
        reset();
        new (d_theInteger.buffer()) bsls::Types::Int64(value);
        d_selectionId = SELECTION_ID_THE_INTEGER;
    }

    return d_theInteger.object();
}

double& Value::makeTheDouble()
{
    if (SELECTION_ID_THE_DOUBLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_theDouble.object());
    }
    else {
        reset();
        new (d_theDouble.buffer()) double();
        d_selectionId = SELECTION_ID_THE_DOUBLE;
    }

    return d_theDouble.object();
}

double& Value::makeTheDouble(double value)
{
    if (SELECTION_ID_THE_DOUBLE == d_selectionId) {
        d_theDouble.object() = value;
    }
    else {
        reset();
        new (d_theDouble.buffer()) double(value);
        d_selectionId = SELECTION_ID_THE_DOUBLE;
    }

    return d_theDouble.object();
}

bdlt::DateTz& Value::makeTheDate()
{
    if (SELECTION_ID_THE_DATE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_theDate.object());
    }
    else {
        reset();
        new (d_theDate.buffer()) bdlt::DateTz();
        d_selectionId = SELECTION_ID_THE_DATE;
    }

    return d_theDate.object();
}

bdlt::DateTz& Value::makeTheDate(const bdlt::DateTz& value)
{
    if (SELECTION_ID_THE_DATE == d_selectionId) {
        d_theDate.object() = value;
    }
    else {
        reset();
        new (d_theDate.buffer()) bdlt::DateTz(value);
        d_selectionId = SELECTION_ID_THE_DATE;
    }

    return d_theDate.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bdlt::DateTz& Value::makeTheDate(bdlt::DateTz&& value)
{
    if (SELECTION_ID_THE_DATE == d_selectionId) {
        d_theDate.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_theDate.buffer()) bdlt::DateTz(bsl::move(value));
        d_selectionId = SELECTION_ID_THE_DATE;
    }

    return d_theDate.object();
}
#endif

bdlt::TimeTz& Value::makeTheTime()
{
    if (SELECTION_ID_THE_TIME == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_theTime.object());
    }
    else {
        reset();
        new (d_theTime.buffer()) bdlt::TimeTz();
        d_selectionId = SELECTION_ID_THE_TIME;
    }

    return d_theTime.object();
}

bdlt::TimeTz& Value::makeTheTime(const bdlt::TimeTz& value)
{
    if (SELECTION_ID_THE_TIME == d_selectionId) {
        d_theTime.object() = value;
    }
    else {
        reset();
        new (d_theTime.buffer()) bdlt::TimeTz(value);
        d_selectionId = SELECTION_ID_THE_TIME;
    }

    return d_theTime.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bdlt::TimeTz& Value::makeTheTime(bdlt::TimeTz&& value)
{
    if (SELECTION_ID_THE_TIME == d_selectionId) {
        d_theTime.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_theTime.buffer()) bdlt::TimeTz(bsl::move(value));
        d_selectionId = SELECTION_ID_THE_TIME;
    }

    return d_theTime.object();
}
#endif

bdlt::DatetimeTz& Value::makeTheDatetime()
{
    if (SELECTION_ID_THE_DATETIME == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_theDatetime.object());
    }
    else {
        reset();
        new (d_theDatetime.buffer()) bdlt::DatetimeTz();
        d_selectionId = SELECTION_ID_THE_DATETIME;
    }

    return d_theDatetime.object();
}

bdlt::DatetimeTz& Value::makeTheDatetime(const bdlt::DatetimeTz& value)
{
    if (SELECTION_ID_THE_DATETIME == d_selectionId) {
        d_theDatetime.object() = value;
    }
    else {
        reset();
        new (d_theDatetime.buffer()) bdlt::DatetimeTz(value);
        d_selectionId = SELECTION_ID_THE_DATETIME;
    }

    return d_theDatetime.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bdlt::DatetimeTz& Value::makeTheDatetime(bdlt::DatetimeTz&& value)
{
    if (SELECTION_ID_THE_DATETIME == d_selectionId) {
        d_theDatetime.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_theDatetime.buffer()) bdlt::DatetimeTz(bsl::move(value));
        d_selectionId = SELECTION_ID_THE_DATETIME;
    }

    return d_theDatetime.object();
}
#endif

bsl::string& Value::makeTheString()
{
    if (SELECTION_ID_THE_STRING == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_theString.object());
    }
    else {
        reset();
        new (d_theString.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_THE_STRING;
    }

    return d_theString.object();
}

bsl::string& Value::makeTheString(const bsl::string& value)
{
    if (SELECTION_ID_THE_STRING == d_selectionId) {
        d_theString.object() = value;
    }
    else {
        reset();
        new (d_theString.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_THE_STRING;
    }

    return d_theString.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& Value::makeTheString(bsl::string&& value)
{
    if (SELECTION_ID_THE_STRING == d_selectionId) {
        d_theString.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_theString.buffer())
            bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_THE_STRING;
    }

    return d_theString.object();
}
#endif

// ACCESSORS

bsl::ostream&
Value::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_THE_NULL: {
        printer.printAttribute("theNull", d_theNull.object());
    } break;
    case SELECTION_ID_THE_BOOL: {
        printer.printAttribute("theBool", d_theBool.object());
    } break;
    case SELECTION_ID_THE_INTEGER: {
        printer.printAttribute("theInteger", d_theInteger.object());
    } break;
    case SELECTION_ID_THE_DOUBLE: {
        printer.printAttribute("theDouble", d_theDouble.object());
    } break;
    case SELECTION_ID_THE_DATE: {
        printer.printAttribute("theDate", d_theDate.object());
    } break;
    case SELECTION_ID_THE_TIME: {
        printer.printAttribute("theTime", d_theTime.object());
    } break;
    case SELECTION_ID_THE_DATETIME: {
        printer.printAttribute("theDatetime", d_theDatetime.object());
    } break;
    case SELECTION_ID_THE_STRING: {
        printer.printAttribute("theString", d_theString.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* Value::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_THE_NULL:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_NULL].name();
    case SELECTION_ID_THE_BOOL:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_BOOL].name();
    case SELECTION_ID_THE_INTEGER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_INTEGER].name();
    case SELECTION_ID_THE_DOUBLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_DOUBLE].name();
    case SELECTION_ID_THE_DATE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_DATE].name();
    case SELECTION_ID_THE_TIME:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_TIME].name();
    case SELECTION_ID_THE_DATETIME:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_DATETIME].name();
    case SELECTION_ID_THE_STRING:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_THE_STRING].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------
// class ClusterList
// -----------------

// CONSTANTS

const char ClusterList::CLASS_NAME[] = "ClusterList";

const bdlat_AttributeInfo ClusterList::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLUSTERS,
     "clusters",
     sizeof("clusters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* ClusterList::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterList::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterList::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLUSTERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTERS];
    default: return 0;
    }
}

// CREATORS

ClusterList::ClusterList(bslma::Allocator* basicAllocator)
: d_clusters(basicAllocator)
{
}

ClusterList::ClusterList(const ClusterList& original,
                         bslma::Allocator*  basicAllocator)
: d_clusters(original.d_clusters, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterList::ClusterList(ClusterList&& original) noexcept
: d_clusters(bsl::move(original.d_clusters))
{
}

ClusterList::ClusterList(ClusterList&&     original,
                         bslma::Allocator* basicAllocator)
: d_clusters(bsl::move(original.d_clusters), basicAllocator)
{
}
#endif

ClusterList::~ClusterList()
{
}

// MANIPULATORS

ClusterList& ClusterList::operator=(const ClusterList& rhs)
{
    if (this != &rhs) {
        d_clusters = rhs.d_clusters;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterList& ClusterList::operator=(ClusterList&& rhs)
{
    if (this != &rhs) {
        d_clusters = bsl::move(rhs.d_clusters);
    }

    return *this;
}
#endif

void ClusterList::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clusters);
}

// ACCESSORS

bsl::ostream&
ClusterList::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clusters", this->clusters());
    printer.end();
    return stream;
}

// ------------------------
// class ClusterQueueHelper
// ------------------------

// CONSTANTS

const char ClusterQueueHelper::CLASS_NAME[] = "ClusterQueueHelper";

const bdlat_AttributeInfo ClusterQueueHelper::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLUSTER_NAME,
     "clusterName",
     sizeof("clusterName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_LOCALITY,
     "locality",
     sizeof("locality") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_NUM_QUEUES,
     "numQueues",
     sizeof("numQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_QUEUE_KEYS,
     "numQueueKeys",
     sizeof("numQueueKeys") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_PENDING_REOPEN_QUEUE_REQUESTS,
     "numPendingReopenQueueRequests",
     sizeof("numPendingReopenQueueRequests") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_DOMAINS,
     "domains",
     sizeof("domains") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUES,
     "queues",
     sizeof("queues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterQueueHelper::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 7; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterQueueHelper::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterQueueHelper::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLUSTER_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME];
    case ATTRIBUTE_ID_LOCALITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCALITY];
    case ATTRIBUTE_ID_NUM_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_QUEUES];
    case ATTRIBUTE_ID_NUM_QUEUE_KEYS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_QUEUE_KEYS];
    case ATTRIBUTE_ID_NUM_PENDING_REOPEN_QUEUE_REQUESTS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_NUM_PENDING_REOPEN_QUEUE_REQUESTS];
    case ATTRIBUTE_ID_DOMAINS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAINS];
    case ATTRIBUTE_ID_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES];
    default: return 0;
    }
}

// CREATORS

ClusterQueueHelper::ClusterQueueHelper(bslma::Allocator* basicAllocator)
: d_queues(basicAllocator)
, d_domains(basicAllocator)
, d_clusterName(basicAllocator)
, d_numQueues()
, d_numQueueKeys()
, d_numPendingReopenQueueRequests()
, d_locality(static_cast<Locality::Value>(0))
{
}

ClusterQueueHelper::ClusterQueueHelper(const ClusterQueueHelper& original,
                                       bslma::Allocator* basicAllocator)
: d_queues(original.d_queues, basicAllocator)
, d_domains(original.d_domains, basicAllocator)
, d_clusterName(original.d_clusterName, basicAllocator)
, d_numQueues(original.d_numQueues)
, d_numQueueKeys(original.d_numQueueKeys)
, d_numPendingReopenQueueRequests(original.d_numPendingReopenQueueRequests)
, d_locality(original.d_locality)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterQueueHelper::ClusterQueueHelper(ClusterQueueHelper&& original) noexcept
: d_queues(bsl::move(original.d_queues)),
  d_domains(bsl::move(original.d_domains)),
  d_clusterName(bsl::move(original.d_clusterName)),
  d_numQueues(bsl::move(original.d_numQueues)),
  d_numQueueKeys(bsl::move(original.d_numQueueKeys)),
  d_numPendingReopenQueueRequests(
      bsl::move(original.d_numPendingReopenQueueRequests)),
  d_locality(bsl::move(original.d_locality))
{
}

ClusterQueueHelper::ClusterQueueHelper(ClusterQueueHelper&& original,
                                       bslma::Allocator*    basicAllocator)
: d_queues(bsl::move(original.d_queues), basicAllocator)
, d_domains(bsl::move(original.d_domains), basicAllocator)
, d_clusterName(bsl::move(original.d_clusterName), basicAllocator)
, d_numQueues(bsl::move(original.d_numQueues))
, d_numQueueKeys(bsl::move(original.d_numQueueKeys))
, d_numPendingReopenQueueRequests(
      bsl::move(original.d_numPendingReopenQueueRequests))
, d_locality(bsl::move(original.d_locality))
{
}
#endif

ClusterQueueHelper::~ClusterQueueHelper()
{
}

// MANIPULATORS

ClusterQueueHelper&
ClusterQueueHelper::operator=(const ClusterQueueHelper& rhs)
{
    if (this != &rhs) {
        d_clusterName                   = rhs.d_clusterName;
        d_locality                      = rhs.d_locality;
        d_numQueues                     = rhs.d_numQueues;
        d_numQueueKeys                  = rhs.d_numQueueKeys;
        d_numPendingReopenQueueRequests = rhs.d_numPendingReopenQueueRequests;
        d_domains                       = rhs.d_domains;
        d_queues                        = rhs.d_queues;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterQueueHelper& ClusterQueueHelper::operator=(ClusterQueueHelper&& rhs)
{
    if (this != &rhs) {
        d_clusterName                   = bsl::move(rhs.d_clusterName);
        d_locality                      = bsl::move(rhs.d_locality);
        d_numQueues                     = bsl::move(rhs.d_numQueues);
        d_numQueueKeys                  = bsl::move(rhs.d_numQueueKeys);
        d_numPendingReopenQueueRequests = bsl::move(
            rhs.d_numPendingReopenQueueRequests);
        d_domains = bsl::move(rhs.d_domains);
        d_queues  = bsl::move(rhs.d_queues);
    }

    return *this;
}
#endif

void ClusterQueueHelper::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clusterName);
    bdlat_ValueTypeFunctions::reset(&d_locality);
    bdlat_ValueTypeFunctions::reset(&d_numQueues);
    bdlat_ValueTypeFunctions::reset(&d_numQueueKeys);
    bdlat_ValueTypeFunctions::reset(&d_numPendingReopenQueueRequests);
    bdlat_ValueTypeFunctions::reset(&d_domains);
    bdlat_ValueTypeFunctions::reset(&d_queues);
}

// ACCESSORS

bsl::ostream& ClusterQueueHelper::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clusterName", this->clusterName());
    printer.printAttribute("locality", this->locality());
    printer.printAttribute("numQueues", this->numQueues());
    printer.printAttribute("numQueueKeys", this->numQueueKeys());
    printer.printAttribute("numPendingReopenQueueRequests",
                           this->numPendingReopenQueueRequests());
    printer.printAttribute("domains", this->domains());
    printer.printAttribute("queues", this->queues());
    printer.end();
    return stream;
}

// ---------------------------
// class ConfigProviderCommand
// ---------------------------

// CONSTANTS

const char ConfigProviderCommand::CLASS_NAME[] = "ConfigProviderCommand";

const bdlat_SelectionInfo ConfigProviderCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_CLEAR_CACHE,
     "clearCache",
     sizeof("clearCache") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ConfigProviderCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ConfigProviderCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ConfigProviderCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_CLEAR_CACHE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLEAR_CACHE];
    default: return 0;
    }
}

// CREATORS

ConfigProviderCommand::ConfigProviderCommand(
    const ConfigProviderCommand& original,
    bslma::Allocator*            basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        new (d_clearCache.buffer())
            ClearCache(original.d_clearCache.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigProviderCommand::ConfigProviderCommand(ConfigProviderCommand&& original)
    noexcept : d_selectionId(original.d_selectionId),
               d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        new (d_clearCache.buffer())
            ClearCache(bsl::move(original.d_clearCache.object()),
                       d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ConfigProviderCommand::ConfigProviderCommand(ConfigProviderCommand&& original,
                                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        new (d_clearCache.buffer())
            ClearCache(bsl::move(original.d_clearCache.object()),
                       d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ConfigProviderCommand&
ConfigProviderCommand::operator=(const ConfigProviderCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_CLEAR_CACHE: {
            makeClearCache(rhs.d_clearCache.object());
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
ConfigProviderCommand&
ConfigProviderCommand::operator=(ConfigProviderCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_CLEAR_CACHE: {
            makeClearCache(bsl::move(rhs.d_clearCache.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ConfigProviderCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        d_clearCache.object().~ClearCache();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ConfigProviderCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        makeClearCache();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ConfigProviderCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

ClearCache& ConfigProviderCommand::makeClearCache()
{
    if (SELECTION_ID_CLEAR_CACHE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clearCache.object());
    }
    else {
        reset();
        new (d_clearCache.buffer()) ClearCache(d_allocator_p);
        d_selectionId = SELECTION_ID_CLEAR_CACHE;
    }

    return d_clearCache.object();
}

ClearCache& ConfigProviderCommand::makeClearCache(const ClearCache& value)
{
    if (SELECTION_ID_CLEAR_CACHE == d_selectionId) {
        d_clearCache.object() = value;
    }
    else {
        reset();
        new (d_clearCache.buffer()) ClearCache(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLEAR_CACHE;
    }

    return d_clearCache.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClearCache& ConfigProviderCommand::makeClearCache(ClearCache&& value)
{
    if (SELECTION_ID_CLEAR_CACHE == d_selectionId) {
        d_clearCache.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clearCache.buffer())
            ClearCache(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLEAR_CACHE;
    }

    return d_clearCache.object();
}
#endif

// ACCESSORS

bsl::ostream& ConfigProviderCommand::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        printer.printAttribute("clearCache", d_clearCache.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ConfigProviderCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLEAR_CACHE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------
// class DomainInfo
// ----------------

// CONSTANTS

const char DomainInfo::CLASS_NAME[] = "DomainInfo";

const bdlat_AttributeInfo DomainInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CONFIG_JSON,
     "configJson",
     sizeof("configJson") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CLUSTER_NAME,
     "clusterName",
     sizeof("clusterName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CAPACITY_METER,
     "capacityMeter",
     sizeof("capacityMeter") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUE_URIS,
     "queueUris",
     sizeof("queueUris") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_STORAGE_CONTENT,
     "storageContent",
     sizeof("storageContent") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* DomainInfo::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DomainInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DomainInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_CONFIG_JSON:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG_JSON];
    case ATTRIBUTE_ID_CLUSTER_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME];
    case ATTRIBUTE_ID_CAPACITY_METER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CAPACITY_METER];
    case ATTRIBUTE_ID_QUEUE_URIS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URIS];
    case ATTRIBUTE_ID_STORAGE_CONTENT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE_CONTENT];
    default: return 0;
    }
}

// CREATORS

DomainInfo::DomainInfo(bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_queueUris(basicAllocator)
, d_name(basicAllocator)
, d_configJson(basicAllocator)
, d_clusterName(basicAllocator)
, d_storageContent(basicAllocator)
{
    d_capacityMeter = new (*d_allocator_p) CapacityMeter(d_allocator_p);
}

DomainInfo::DomainInfo(const DomainInfo& original,
                       bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_queueUris(original.d_queueUris, basicAllocator)
, d_name(original.d_name, basicAllocator)
, d_configJson(original.d_configJson, basicAllocator)
, d_clusterName(original.d_clusterName, basicAllocator)
, d_storageContent(original.d_storageContent, basicAllocator)
{
    d_capacityMeter = new (*d_allocator_p)
        CapacityMeter(*original.d_capacityMeter, d_allocator_p);
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainInfo::DomainInfo(DomainInfo&& original) noexcept
: d_allocator_p(original.d_allocator_p),
  d_queueUris(bsl::move(original.d_queueUris)),
  d_name(bsl::move(original.d_name)),
  d_configJson(bsl::move(original.d_configJson)),
  d_clusterName(bsl::move(original.d_clusterName)),
  d_storageContent(bsl::move(original.d_storageContent))
{
    d_capacityMeter          = original.d_capacityMeter;
    original.d_capacityMeter = 0;
}

DomainInfo::DomainInfo(DomainInfo&& original, bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_queueUris(bsl::move(original.d_queueUris), basicAllocator)
, d_name(bsl::move(original.d_name), basicAllocator)
, d_configJson(bsl::move(original.d_configJson), basicAllocator)
, d_clusterName(bsl::move(original.d_clusterName), basicAllocator)
, d_storageContent(bsl::move(original.d_storageContent), basicAllocator)
{
    if (d_allocator_p == original.d_allocator_p) {
        d_capacityMeter          = original.d_capacityMeter;
        original.d_capacityMeter = 0;
    }
    else {
        d_capacityMeter = new (*d_allocator_p)
            CapacityMeter(bsl::move(*original.d_capacityMeter), d_allocator_p);
    }
}
#endif

DomainInfo::~DomainInfo()
{
    d_allocator_p->deleteObject(d_capacityMeter);
}

// MANIPULATORS

DomainInfo& DomainInfo::operator=(const DomainInfo& rhs)
{
    if (this != &rhs) {
        d_name        = rhs.d_name;
        d_configJson  = rhs.d_configJson;
        d_clusterName = rhs.d_clusterName;
        if (d_capacityMeter) {
            *d_capacityMeter = *rhs.d_capacityMeter;
        }
        else {
            d_capacityMeter = new (*d_allocator_p)
                CapacityMeter(*rhs.d_capacityMeter, d_allocator_p);
        }
        d_queueUris      = rhs.d_queueUris;
        d_storageContent = rhs.d_storageContent;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainInfo& DomainInfo::operator=(DomainInfo&& rhs)
{
    if (this != &rhs) {
        d_name        = bsl::move(rhs.d_name);
        d_configJson  = bsl::move(rhs.d_configJson);
        d_clusterName = bsl::move(rhs.d_clusterName);
        if (d_allocator_p == rhs.d_allocator_p) {
            d_allocator_p->deleteObject(d_capacityMeter);
            d_capacityMeter     = rhs.d_capacityMeter;
            rhs.d_capacityMeter = 0;
        }
        else if (d_capacityMeter) {
            *d_capacityMeter = bsl::move(*rhs.d_capacityMeter);
        }
        else {
            d_capacityMeter = new (*d_allocator_p)
                CapacityMeter(bsl::move(*rhs.d_capacityMeter), d_allocator_p);
        }
        d_queueUris      = bsl::move(rhs.d_queueUris);
        d_storageContent = bsl::move(rhs.d_storageContent);
    }

    return *this;
}
#endif

void DomainInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_configJson);
    bdlat_ValueTypeFunctions::reset(&d_clusterName);
    BSLS_ASSERT(d_capacityMeter);
    bdlat_ValueTypeFunctions::reset(d_capacityMeter);
    bdlat_ValueTypeFunctions::reset(&d_queueUris);
    bdlat_ValueTypeFunctions::reset(&d_storageContent);
}

// ACCESSORS

bsl::ostream&
DomainInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("configJson", this->configJson());
    printer.printAttribute("clusterName", this->clusterName());
    printer.printAttribute("capacityMeter", this->capacityMeter());
    printer.printAttribute("queueUris", this->queueUris());
    printer.printAttribute("storageContent", this->storageContent());
    printer.end();
    return stream;
}

// -----------------
// class DomainQueue
// -----------------

// CONSTANTS

const char DomainQueue::CLASS_NAME[] = "DomainQueue";

const bdlat_AttributeInfo DomainQueue::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_COMMAND,
     "command",
     sizeof("command") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* DomainQueue::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DomainQueue::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DomainQueue::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_COMMAND:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND];
    default: return 0;
    }
}

// CREATORS

DomainQueue::DomainQueue(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_command(basicAllocator)
{
}

DomainQueue::DomainQueue(const DomainQueue& original,
                         bslma::Allocator*  basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_command(original.d_command, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainQueue::DomainQueue(DomainQueue&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_command(bsl::move(original.d_command))
{
}

DomainQueue::DomainQueue(DomainQueue&&     original,
                         bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_command(bsl::move(original.d_command), basicAllocator)
{
}
#endif

DomainQueue::~DomainQueue()
{
}

// MANIPULATORS

DomainQueue& DomainQueue::operator=(const DomainQueue& rhs)
{
    if (this != &rhs) {
        d_name    = rhs.d_name;
        d_command = rhs.d_command;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainQueue& DomainQueue::operator=(DomainQueue&& rhs)
{
    if (this != &rhs) {
        d_name    = bsl::move(rhs.d_name);
        d_command = bsl::move(rhs.d_command);
    }

    return *this;
}
#endif

void DomainQueue::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_command);
}

// ACCESSORS

bsl::ostream&
DomainQueue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("command", this->command());
    printer.end();
    return stream;
}

// ---------------------------
// class DomainResolverCommand
// ---------------------------

// CONSTANTS

const char DomainResolverCommand::CLASS_NAME[] = "DomainResolverCommand";

const bdlat_SelectionInfo DomainResolverCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_CLEAR_CACHE,
     "clearCache",
     sizeof("clearCache") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
DomainResolverCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            DomainResolverCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* DomainResolverCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_CLEAR_CACHE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLEAR_CACHE];
    default: return 0;
    }
}

// CREATORS

DomainResolverCommand::DomainResolverCommand(
    const DomainResolverCommand& original,
    bslma::Allocator*            basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        new (d_clearCache.buffer())
            ClearCache(original.d_clearCache.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainResolverCommand::DomainResolverCommand(DomainResolverCommand&& original)
    noexcept : d_selectionId(original.d_selectionId),
               d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        new (d_clearCache.buffer())
            ClearCache(bsl::move(original.d_clearCache.object()),
                       d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

DomainResolverCommand::DomainResolverCommand(DomainResolverCommand&& original,
                                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        new (d_clearCache.buffer())
            ClearCache(bsl::move(original.d_clearCache.object()),
                       d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

DomainResolverCommand&
DomainResolverCommand::operator=(const DomainResolverCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_CLEAR_CACHE: {
            makeClearCache(rhs.d_clearCache.object());
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
DomainResolverCommand&
DomainResolverCommand::operator=(DomainResolverCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_CLEAR_CACHE: {
            makeClearCache(bsl::move(rhs.d_clearCache.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void DomainResolverCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        d_clearCache.object().~ClearCache();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int DomainResolverCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        makeClearCache();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int DomainResolverCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

ClearCache& DomainResolverCommand::makeClearCache()
{
    if (SELECTION_ID_CLEAR_CACHE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clearCache.object());
    }
    else {
        reset();
        new (d_clearCache.buffer()) ClearCache(d_allocator_p);
        d_selectionId = SELECTION_ID_CLEAR_CACHE;
    }

    return d_clearCache.object();
}

ClearCache& DomainResolverCommand::makeClearCache(const ClearCache& value)
{
    if (SELECTION_ID_CLEAR_CACHE == d_selectionId) {
        d_clearCache.object() = value;
    }
    else {
        reset();
        new (d_clearCache.buffer()) ClearCache(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLEAR_CACHE;
    }

    return d_clearCache.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClearCache& DomainResolverCommand::makeClearCache(ClearCache&& value)
{
    if (SELECTION_ID_CLEAR_CACHE == d_selectionId) {
        d_clearCache.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clearCache.buffer())
            ClearCache(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLEAR_CACHE;
    }

    return d_clearCache.object();
}
#endif

// ACCESSORS

bsl::ostream& DomainResolverCommand::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE: {
        printer.printAttribute("clearCache", d_clearCache.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* DomainResolverCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_CLEAR_CACHE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLEAR_CACHE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------------
// class FileStoreSummary
// ----------------------

// CONSTANTS

const char FileStoreSummary::CLASS_NAME[] = "FileStoreSummary";

const bdlat_AttributeInfo FileStoreSummary::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PRIMARY_NODE_DESCRIPTION,
     "primaryNodeDescription",
     sizeof("primaryNodeDescription") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
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
    {ATTRIBUTE_ID_IS_AVAILABLE,
     "isAvailable",
     sizeof("isAvailable") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_FILE_SETS,
     "fileSets",
     sizeof("fileSets") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_ACTIVE_FILE_SET,
     "activeFileSet",
     sizeof("activeFileSet") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_TOTAL_MAPPED_BYTES,
     "totalMappedBytes",
     sizeof("totalMappedBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_OUTSTANDING_RECORDS,
     "numOutstandingRecords",
     sizeof("numOutstandingRecords") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_UNRECEIPTED_MESSAGES,
     "numUnreceiptedMessages",
     sizeof("numUnreceiptedMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NAGLE_PACKET_COUNT,
     "naglePacketCount",
     sizeof("naglePacketCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STORAGE_CONTENT,
     "storageContent",
     sizeof("storageContent") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
FileStoreSummary::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 11; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            FileStoreSummary::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* FileStoreSummary::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PRIMARY_NODE_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_NODE_DESCRIPTION];
    case ATTRIBUTE_ID_PRIMARY_LEASE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIMARY_LEASE_ID];
    case ATTRIBUTE_ID_SEQUENCE_NUM:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENCE_NUM];
    case ATTRIBUTE_ID_IS_AVAILABLE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_AVAILABLE];
    case ATTRIBUTE_ID_FILE_SETS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_SETS];
    case ATTRIBUTE_ID_ACTIVE_FILE_SET:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACTIVE_FILE_SET];
    case ATTRIBUTE_ID_TOTAL_MAPPED_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TOTAL_MAPPED_BYTES];
    case ATTRIBUTE_ID_NUM_OUTSTANDING_RECORDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_OUTSTANDING_RECORDS];
    case ATTRIBUTE_ID_NUM_UNRECEIPTED_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_UNRECEIPTED_MESSAGES];
    case ATTRIBUTE_ID_NAGLE_PACKET_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAGLE_PACKET_COUNT];
    case ATTRIBUTE_ID_STORAGE_CONTENT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE_CONTENT];
    default: return 0;
    }
}

// CREATORS

FileStoreSummary::FileStoreSummary(bslma::Allocator* basicAllocator)
: d_sequenceNum()
, d_totalMappedBytes()
, d_fileSets(basicAllocator)
, d_primaryNodeDescription(basicAllocator)
, d_storageContent(basicAllocator)
, d_activeFileSet()
, d_primaryLeaseId()
, d_numOutstandingRecords()
, d_numUnreceiptedMessages()
, d_naglePacketCount()
, d_isAvailable()
{
}

FileStoreSummary::FileStoreSummary(const FileStoreSummary& original,
                                   bslma::Allocator*       basicAllocator)
: d_sequenceNum(original.d_sequenceNum)
, d_totalMappedBytes(original.d_totalMappedBytes)
, d_fileSets(original.d_fileSets, basicAllocator)
, d_primaryNodeDescription(original.d_primaryNodeDescription, basicAllocator)
, d_storageContent(original.d_storageContent, basicAllocator)
, d_activeFileSet(original.d_activeFileSet)
, d_primaryLeaseId(original.d_primaryLeaseId)
, d_numOutstandingRecords(original.d_numOutstandingRecords)
, d_numUnreceiptedMessages(original.d_numUnreceiptedMessages)
, d_naglePacketCount(original.d_naglePacketCount)
, d_isAvailable(original.d_isAvailable)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FileStoreSummary::FileStoreSummary(FileStoreSummary&& original) noexcept
: d_sequenceNum(bsl::move(original.d_sequenceNum)),
  d_totalMappedBytes(bsl::move(original.d_totalMappedBytes)),
  d_fileSets(bsl::move(original.d_fileSets)),
  d_primaryNodeDescription(bsl::move(original.d_primaryNodeDescription)),
  d_storageContent(bsl::move(original.d_storageContent)),
  d_activeFileSet(bsl::move(original.d_activeFileSet)),
  d_primaryLeaseId(bsl::move(original.d_primaryLeaseId)),
  d_numOutstandingRecords(bsl::move(original.d_numOutstandingRecords)),
  d_numUnreceiptedMessages(bsl::move(original.d_numUnreceiptedMessages)),
  d_naglePacketCount(bsl::move(original.d_naglePacketCount)),
  d_isAvailable(bsl::move(original.d_isAvailable))
{
}

FileStoreSummary::FileStoreSummary(FileStoreSummary&& original,
                                   bslma::Allocator*  basicAllocator)
: d_sequenceNum(bsl::move(original.d_sequenceNum))
, d_totalMappedBytes(bsl::move(original.d_totalMappedBytes))
, d_fileSets(bsl::move(original.d_fileSets), basicAllocator)
, d_primaryNodeDescription(bsl::move(original.d_primaryNodeDescription),
                           basicAllocator)
, d_storageContent(bsl::move(original.d_storageContent), basicAllocator)
, d_activeFileSet(bsl::move(original.d_activeFileSet))
, d_primaryLeaseId(bsl::move(original.d_primaryLeaseId))
, d_numOutstandingRecords(bsl::move(original.d_numOutstandingRecords))
, d_numUnreceiptedMessages(bsl::move(original.d_numUnreceiptedMessages))
, d_naglePacketCount(bsl::move(original.d_naglePacketCount))
, d_isAvailable(bsl::move(original.d_isAvailable))
{
}
#endif

FileStoreSummary::~FileStoreSummary()
{
}

// MANIPULATORS

FileStoreSummary& FileStoreSummary::operator=(const FileStoreSummary& rhs)
{
    if (this != &rhs) {
        d_primaryNodeDescription = rhs.d_primaryNodeDescription;
        d_primaryLeaseId         = rhs.d_primaryLeaseId;
        d_sequenceNum            = rhs.d_sequenceNum;
        d_isAvailable            = rhs.d_isAvailable;
        d_fileSets               = rhs.d_fileSets;
        d_activeFileSet          = rhs.d_activeFileSet;
        d_totalMappedBytes       = rhs.d_totalMappedBytes;
        d_numOutstandingRecords  = rhs.d_numOutstandingRecords;
        d_numUnreceiptedMessages = rhs.d_numUnreceiptedMessages;
        d_naglePacketCount       = rhs.d_naglePacketCount;
        d_storageContent         = rhs.d_storageContent;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FileStoreSummary& FileStoreSummary::operator=(FileStoreSummary&& rhs)
{
    if (this != &rhs) {
        d_primaryNodeDescription = bsl::move(rhs.d_primaryNodeDescription);
        d_primaryLeaseId         = bsl::move(rhs.d_primaryLeaseId);
        d_sequenceNum            = bsl::move(rhs.d_sequenceNum);
        d_isAvailable            = bsl::move(rhs.d_isAvailable);
        d_fileSets               = bsl::move(rhs.d_fileSets);
        d_activeFileSet          = bsl::move(rhs.d_activeFileSet);
        d_totalMappedBytes       = bsl::move(rhs.d_totalMappedBytes);
        d_numOutstandingRecords  = bsl::move(rhs.d_numOutstandingRecords);
        d_numUnreceiptedMessages = bsl::move(rhs.d_numUnreceiptedMessages);
        d_naglePacketCount       = bsl::move(rhs.d_naglePacketCount);
        d_storageContent         = bsl::move(rhs.d_storageContent);
    }

    return *this;
}
#endif

void FileStoreSummary::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_primaryNodeDescription);
    bdlat_ValueTypeFunctions::reset(&d_primaryLeaseId);
    bdlat_ValueTypeFunctions::reset(&d_sequenceNum);
    bdlat_ValueTypeFunctions::reset(&d_isAvailable);
    bdlat_ValueTypeFunctions::reset(&d_fileSets);
    bdlat_ValueTypeFunctions::reset(&d_activeFileSet);
    bdlat_ValueTypeFunctions::reset(&d_totalMappedBytes);
    bdlat_ValueTypeFunctions::reset(&d_numOutstandingRecords);
    bdlat_ValueTypeFunctions::reset(&d_numUnreceiptedMessages);
    bdlat_ValueTypeFunctions::reset(&d_naglePacketCount);
    bdlat_ValueTypeFunctions::reset(&d_storageContent);
}

// ACCESSORS

bsl::ostream& FileStoreSummary::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("primaryNodeDescription",
                           this->primaryNodeDescription());
    printer.printAttribute("primaryLeaseId", this->primaryLeaseId());
    printer.printAttribute("sequenceNum", this->sequenceNum());
    printer.printAttribute("isAvailable", this->isAvailable());
    printer.printAttribute("fileSets", this->fileSets());
    printer.printAttribute("activeFileSet", this->activeFileSet());
    printer.printAttribute("totalMappedBytes", this->totalMappedBytes());
    printer.printAttribute("numOutstandingRecords",
                           this->numOutstandingRecords());
    printer.printAttribute("numUnreceiptedMessages",
                           this->numUnreceiptedMessages());
    printer.printAttribute("naglePacketCount", this->naglePacketCount());
    printer.printAttribute("storageContent", this->storageContent());
    printer.end();
    return stream;
}

// ----------------
// class GetTunable
// ----------------

// CONSTANTS

const char GetTunable::CLASS_NAME[] = "GetTunable";

const bdlat_AttributeInfo GetTunable::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo* GetTunable::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    if (bdlb::String::areEqualCaseless("all", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("self", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            GetTunable::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* GetTunable::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

GetTunable::GetTunable(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_choice()
{
}

GetTunable::GetTunable(const GetTunable& original,
                       bslma::Allocator* basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_choice(original.d_choice)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
GetTunable::GetTunable(GetTunable&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_choice(bsl::move(original.d_choice))
{
}

GetTunable::GetTunable(GetTunable&& original, bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_choice(bsl::move(original.d_choice))
{
}
#endif

GetTunable::~GetTunable()
{
}

// MANIPULATORS

GetTunable& GetTunable::operator=(const GetTunable& rhs)
{
    if (this != &rhs) {
        d_name   = rhs.d_name;
        d_choice = rhs.d_choice;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
GetTunable& GetTunable::operator=(GetTunable&& rhs)
{
    if (this != &rhs) {
        d_name   = bsl::move(rhs.d_name);
        d_choice = bsl::move(rhs.d_choice);
    }

    return *this;
}
#endif

void GetTunable::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream&
GetTunable::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// --------------------------
// class MessageGroupIdHelper
// --------------------------

// CONSTANTS

const char MessageGroupIdHelper::CLASS_NAME[] = "MessageGroupIdHelper";

const bdlat_AttributeInfo MessageGroupIdHelper::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_TIMEOUT_NANOSECONDS,
     "timeoutNanoseconds",
     sizeof("timeoutNanoseconds") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_MSG_GROUP_IDS,
     "maxMsgGroupIds",
     sizeof("maxMsgGroupIds") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_IS_REBALANCE_ON,
     "isRebalanceOn",
     sizeof("isRebalanceOn") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_STATUS,
     "status",
     sizeof("status") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
MessageGroupIdHelper::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            MessageGroupIdHelper::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* MessageGroupIdHelper::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_TIMEOUT_NANOSECONDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMEOUT_NANOSECONDS];
    case ATTRIBUTE_ID_MAX_MSG_GROUP_IDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_MSG_GROUP_IDS];
    case ATTRIBUTE_ID_IS_REBALANCE_ON:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_REBALANCE_ON];
    case ATTRIBUTE_ID_STATUS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATUS];
    default: return 0;
    }
}

// CREATORS

MessageGroupIdHelper::MessageGroupIdHelper(bslma::Allocator* basicAllocator)
: d_timeoutNanoseconds()
, d_status(basicAllocator)
, d_maxMsgGroupIds()
, d_isRebalanceOn()
{
}

MessageGroupIdHelper::MessageGroupIdHelper(
    const MessageGroupIdHelper& original,
    bslma::Allocator*           basicAllocator)
: d_timeoutNanoseconds(original.d_timeoutNanoseconds)
, d_status(original.d_status, basicAllocator)
, d_maxMsgGroupIds(original.d_maxMsgGroupIds)
, d_isRebalanceOn(original.d_isRebalanceOn)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
MessageGroupIdHelper::MessageGroupIdHelper(MessageGroupIdHelper&& original)
    noexcept : d_timeoutNanoseconds(bsl::move(original.d_timeoutNanoseconds)),
               d_status(bsl::move(original.d_status)),
               d_maxMsgGroupIds(bsl::move(original.d_maxMsgGroupIds)),
               d_isRebalanceOn(bsl::move(original.d_isRebalanceOn))
{
}

MessageGroupIdHelper::MessageGroupIdHelper(MessageGroupIdHelper&& original,
                                           bslma::Allocator* basicAllocator)
: d_timeoutNanoseconds(bsl::move(original.d_timeoutNanoseconds))
, d_status(bsl::move(original.d_status), basicAllocator)
, d_maxMsgGroupIds(bsl::move(original.d_maxMsgGroupIds))
, d_isRebalanceOn(bsl::move(original.d_isRebalanceOn))
{
}
#endif

MessageGroupIdHelper::~MessageGroupIdHelper()
{
}

// MANIPULATORS

MessageGroupIdHelper&
MessageGroupIdHelper::operator=(const MessageGroupIdHelper& rhs)
{
    if (this != &rhs) {
        d_timeoutNanoseconds = rhs.d_timeoutNanoseconds;
        d_maxMsgGroupIds     = rhs.d_maxMsgGroupIds;
        d_isRebalanceOn      = rhs.d_isRebalanceOn;
        d_status             = rhs.d_status;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
MessageGroupIdHelper&
MessageGroupIdHelper::operator=(MessageGroupIdHelper&& rhs)
{
    if (this != &rhs) {
        d_timeoutNanoseconds = bsl::move(rhs.d_timeoutNanoseconds);
        d_maxMsgGroupIds     = bsl::move(rhs.d_maxMsgGroupIds);
        d_isRebalanceOn      = bsl::move(rhs.d_isRebalanceOn);
        d_status             = bsl::move(rhs.d_status);
    }

    return *this;
}
#endif

void MessageGroupIdHelper::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_timeoutNanoseconds);
    bdlat_ValueTypeFunctions::reset(&d_maxMsgGroupIds);
    bdlat_ValueTypeFunctions::reset(&d_isRebalanceOn);
    bdlat_ValueTypeFunctions::reset(&d_status);
}

// ACCESSORS

bsl::ostream& MessageGroupIdHelper::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("timeoutNanoseconds", this->timeoutNanoseconds());
    printer.printAttribute("maxMsgGroupIds", this->maxMsgGroupIds());
    printer.printAttribute("isRebalanceOn", this->isRebalanceOn());
    printer.printAttribute("status", this->status());
    printer.end();
    return stream;
}

// ------------------
// class NodeStatuses
// ------------------

// CONSTANTS

const char NodeStatuses::CLASS_NAME[] = "NodeStatuses";

const bdlat_AttributeInfo NodeStatuses::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NODES,
     "nodes",
     sizeof("nodes") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* NodeStatuses::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            NodeStatuses::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* NodeStatuses::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NODES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES];
    default: return 0;
    }
}

// CREATORS

NodeStatuses::NodeStatuses(bslma::Allocator* basicAllocator)
: d_nodes(basicAllocator)
{
}

NodeStatuses::NodeStatuses(const NodeStatuses& original,
                           bslma::Allocator*   basicAllocator)
: d_nodes(original.d_nodes, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
NodeStatuses::NodeStatuses(NodeStatuses&& original) noexcept
: d_nodes(bsl::move(original.d_nodes))
{
}

NodeStatuses::NodeStatuses(NodeStatuses&&    original,
                           bslma::Allocator* basicAllocator)
: d_nodes(bsl::move(original.d_nodes), basicAllocator)
{
}
#endif

NodeStatuses::~NodeStatuses()
{
}

// MANIPULATORS

NodeStatuses& NodeStatuses::operator=(const NodeStatuses& rhs)
{
    if (this != &rhs) {
        d_nodes = rhs.d_nodes;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
NodeStatuses& NodeStatuses::operator=(NodeStatuses&& rhs)
{
    if (this != &rhs) {
        d_nodes = bsl::move(rhs.d_nodes);
    }

    return *this;
}
#endif

void NodeStatuses::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_nodes);
}

// ACCESSORS

bsl::ostream&
NodeStatuses::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("nodes", this->nodes());
    printer.end();
    return stream;
}

// --------------------
// class PartitionsInfo
// --------------------

// CONSTANTS

const char PartitionsInfo::CLASS_NAME[] = "PartitionsInfo";

const bdlat_AttributeInfo PartitionsInfo::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITIONS,
     "partitions",
     sizeof("partitions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionsInfo::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionsInfo::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PartitionsInfo::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITIONS];
    default: return 0;
    }
}

// CREATORS

PartitionsInfo::PartitionsInfo(bslma::Allocator* basicAllocator)
: d_partitions(basicAllocator)
{
}

PartitionsInfo::PartitionsInfo(const PartitionsInfo& original,
                               bslma::Allocator*     basicAllocator)
: d_partitions(original.d_partitions, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionsInfo::PartitionsInfo(PartitionsInfo&& original) noexcept
: d_partitions(bsl::move(original.d_partitions))
{
}

PartitionsInfo::PartitionsInfo(PartitionsInfo&&  original,
                               bslma::Allocator* basicAllocator)
: d_partitions(bsl::move(original.d_partitions), basicAllocator)
{
}
#endif

PartitionsInfo::~PartitionsInfo()
{
}

// MANIPULATORS

PartitionsInfo& PartitionsInfo::operator=(const PartitionsInfo& rhs)
{
    if (this != &rhs) {
        d_partitions = rhs.d_partitions;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionsInfo& PartitionsInfo::operator=(PartitionsInfo&& rhs)
{
    if (this != &rhs) {
        d_partitions = bsl::move(rhs.d_partitions);
    }

    return *this;
}
#endif

void PartitionsInfo::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitions);
}

// ACCESSORS

bsl::ostream& PartitionsInfo::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitions", this->partitions());
    printer.end();
    return stream;
}

// -------------------
// class PriorityGroup
// -------------------

// CONSTANTS

const char PriorityGroup::CLASS_NAME[] = "PriorityGroup";

const bdlat_AttributeInfo PriorityGroup::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ID, "id", sizeof("id") - 1, "", bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_HIGHEST_SUBSCRIPTIONS,
     "highestSubscriptions",
     sizeof("highestSubscriptions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* PriorityGroup::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PriorityGroup::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PriorityGroup::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID];
    case ATTRIBUTE_ID_HIGHEST_SUBSCRIPTIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGHEST_SUBSCRIPTIONS];
    default: return 0;
    }
}

// CREATORS

PriorityGroup::PriorityGroup(bslma::Allocator* basicAllocator)
: d_highestSubscriptions(basicAllocator)
, d_id()
{
}

PriorityGroup::PriorityGroup(const PriorityGroup& original,
                             bslma::Allocator*    basicAllocator)
: d_highestSubscriptions(original.d_highestSubscriptions, basicAllocator)
, d_id(original.d_id)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PriorityGroup::PriorityGroup(PriorityGroup&& original) noexcept
: d_highestSubscriptions(bsl::move(original.d_highestSubscriptions)),
  d_id(bsl::move(original.d_id))
{
}

PriorityGroup::PriorityGroup(PriorityGroup&&   original,
                             bslma::Allocator* basicAllocator)
: d_highestSubscriptions(bsl::move(original.d_highestSubscriptions),
                         basicAllocator)
, d_id(bsl::move(original.d_id))
{
}
#endif

PriorityGroup::~PriorityGroup()
{
}

// MANIPULATORS

PriorityGroup& PriorityGroup::operator=(const PriorityGroup& rhs)
{
    if (this != &rhs) {
        d_id                   = rhs.d_id;
        d_highestSubscriptions = rhs.d_highestSubscriptions;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PriorityGroup& PriorityGroup::operator=(PriorityGroup&& rhs)
{
    if (this != &rhs) {
        d_id                   = bsl::move(rhs.d_id);
        d_highestSubscriptions = bsl::move(rhs.d_highestSubscriptions);
    }

    return *this;
}
#endif

void PriorityGroup::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_id);
    bdlat_ValueTypeFunctions::reset(&d_highestSubscriptions);
}

// ACCESSORS

bsl::ostream&
PriorityGroup::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("id", this->id());
    printer.printAttribute("highestSubscriptions",
                           this->highestSubscriptions());
    printer.end();
    return stream;
}

// ------------------
// class PurgedQueues
// ------------------

// CONSTANTS

const char PurgedQueues::CLASS_NAME[] = "PurgedQueues";

const bdlat_AttributeInfo PurgedQueues::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_QUEUES,
     "queues",
     sizeof("queues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* PurgedQueues::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PurgedQueues::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PurgedQueues::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES];
    default: return 0;
    }
}

// CREATORS

PurgedQueues::PurgedQueues(bslma::Allocator* basicAllocator)
: d_queues(basicAllocator)
{
}

PurgedQueues::PurgedQueues(const PurgedQueues& original,
                           bslma::Allocator*   basicAllocator)
: d_queues(original.d_queues, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgedQueues::PurgedQueues(PurgedQueues&& original) noexcept
: d_queues(bsl::move(original.d_queues))
{
}

PurgedQueues::PurgedQueues(PurgedQueues&&    original,
                           bslma::Allocator* basicAllocator)
: d_queues(bsl::move(original.d_queues), basicAllocator)
{
}
#endif

PurgedQueues::~PurgedQueues()
{
}

// MANIPULATORS

PurgedQueues& PurgedQueues::operator=(const PurgedQueues& rhs)
{
    if (this != &rhs) {
        d_queues = rhs.d_queues;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgedQueues& PurgedQueues::operator=(PurgedQueues&& rhs)
{
    if (this != &rhs) {
        d_queues = bsl::move(rhs.d_queues);
    }

    return *this;
}
#endif

void PurgedQueues::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_queues);
}

// ACCESSORS

bsl::ostream&
PurgedQueues::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queues", this->queues());
    printer.end();
    return stream;
}

// --------------------------
// class QueueHandleSubStream
// --------------------------

// CONSTANTS

const char QueueHandleSubStream::CLASS_NAME[] = "QueueHandleSubStream";

const bdlat_AttributeInfo QueueHandleSubStream::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SUB_ID,
     "subId",
     sizeof("subId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PARAMETERS_JSON,
     "parametersJson",
     sizeof("parametersJson") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_UNCONFIRMED_MONITORS,
     "unconfirmedMonitors",
     sizeof("unconfirmedMonitors") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_NUM_UNCONFIRMED_MESSAGES,
     "numUnconfirmedMessages",
     sizeof("numUnconfirmedMessages") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueHandleSubStream::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueHandleSubStream::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueHandleSubStream::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SUB_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUB_ID];
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_PARAMETERS_JSON:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARAMETERS_JSON];
    case ATTRIBUTE_ID_UNCONFIRMED_MONITORS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_UNCONFIRMED_MONITORS];
    case ATTRIBUTE_ID_NUM_UNCONFIRMED_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_UNCONFIRMED_MESSAGES];
    default: return 0;
    }
}

// CREATORS

QueueHandleSubStream::QueueHandleSubStream(bslma::Allocator* basicAllocator)
: d_unconfirmedMonitors(basicAllocator)
, d_parametersJson(basicAllocator)
, d_numUnconfirmedMessages()
, d_appId(basicAllocator)
, d_subId()
{
}

QueueHandleSubStream::QueueHandleSubStream(
    const QueueHandleSubStream& original,
    bslma::Allocator*           basicAllocator)
: d_unconfirmedMonitors(original.d_unconfirmedMonitors, basicAllocator)
, d_parametersJson(original.d_parametersJson, basicAllocator)
, d_numUnconfirmedMessages(original.d_numUnconfirmedMessages)
, d_appId(original.d_appId, basicAllocator)
, d_subId(original.d_subId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueHandleSubStream::QueueHandleSubStream(
    QueueHandleSubStream&& original) noexcept
: d_unconfirmedMonitors(bsl::move(original.d_unconfirmedMonitors)),
  d_parametersJson(bsl::move(original.d_parametersJson)),
  d_numUnconfirmedMessages(bsl::move(original.d_numUnconfirmedMessages)),
  d_appId(bsl::move(original.d_appId)),
  d_subId(bsl::move(original.d_subId))
{
}

QueueHandleSubStream::QueueHandleSubStream(QueueHandleSubStream&& original,
                                           bslma::Allocator* basicAllocator)
: d_unconfirmedMonitors(bsl::move(original.d_unconfirmedMonitors),
                        basicAllocator)
, d_parametersJson(bsl::move(original.d_parametersJson), basicAllocator)
, d_numUnconfirmedMessages(bsl::move(original.d_numUnconfirmedMessages))
, d_appId(bsl::move(original.d_appId), basicAllocator)
, d_subId(bsl::move(original.d_subId))
{
}
#endif

QueueHandleSubStream::~QueueHandleSubStream()
{
}

// MANIPULATORS

QueueHandleSubStream&
QueueHandleSubStream::operator=(const QueueHandleSubStream& rhs)
{
    if (this != &rhs) {
        d_subId                  = rhs.d_subId;
        d_appId                  = rhs.d_appId;
        d_parametersJson         = rhs.d_parametersJson;
        d_unconfirmedMonitors    = rhs.d_unconfirmedMonitors;
        d_numUnconfirmedMessages = rhs.d_numUnconfirmedMessages;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueHandleSubStream&
QueueHandleSubStream::operator=(QueueHandleSubStream&& rhs)
{
    if (this != &rhs) {
        d_subId                  = bsl::move(rhs.d_subId);
        d_appId                  = bsl::move(rhs.d_appId);
        d_parametersJson         = bsl::move(rhs.d_parametersJson);
        d_unconfirmedMonitors    = bsl::move(rhs.d_unconfirmedMonitors);
        d_numUnconfirmedMessages = bsl::move(rhs.d_numUnconfirmedMessages);
    }

    return *this;
}
#endif

void QueueHandleSubStream::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_subId);
    bdlat_ValueTypeFunctions::reset(&d_appId);
    bdlat_ValueTypeFunctions::reset(&d_parametersJson);
    bdlat_ValueTypeFunctions::reset(&d_unconfirmedMonitors);
    bdlat_ValueTypeFunctions::reset(&d_numUnconfirmedMessages);
}

// ACCESSORS

bsl::ostream& QueueHandleSubStream::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("subId", this->subId());
    printer.printAttribute("appId", this->appId());
    printer.printAttribute("parametersJson", this->parametersJson());
    printer.printAttribute("unconfirmedMonitors", this->unconfirmedMonitors());
    printer.printAttribute("numUnconfirmedMessages",
                           this->numUnconfirmedMessages());
    printer.end();
    return stream;
}

// -------------------
// class QueueStatuses
// -------------------

// CONSTANTS

const char QueueStatuses::CLASS_NAME[] = "QueueStatuses";

const bdlat_AttributeInfo QueueStatuses::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_QUEUE_STATUSES,
     "queueStatuses",
     sizeof("queueStatuses") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* QueueStatuses::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueStatuses::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueStatuses::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_QUEUE_STATUSES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_STATUSES];
    default: return 0;
    }
}

// CREATORS

QueueStatuses::QueueStatuses(bslma::Allocator* basicAllocator)
: d_queueStatuses(basicAllocator)
{
}

QueueStatuses::QueueStatuses(const QueueStatuses& original,
                             bslma::Allocator*    basicAllocator)
: d_queueStatuses(original.d_queueStatuses, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueStatuses::QueueStatuses(QueueStatuses&& original) noexcept
: d_queueStatuses(bsl::move(original.d_queueStatuses))
{
}

QueueStatuses::QueueStatuses(QueueStatuses&&   original,
                             bslma::Allocator* basicAllocator)
: d_queueStatuses(bsl::move(original.d_queueStatuses), basicAllocator)
{
}
#endif

QueueStatuses::~QueueStatuses()
{
}

// MANIPULATORS

QueueStatuses& QueueStatuses::operator=(const QueueStatuses& rhs)
{
    if (this != &rhs) {
        d_queueStatuses = rhs.d_queueStatuses;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueStatuses& QueueStatuses::operator=(QueueStatuses&& rhs)
{
    if (this != &rhs) {
        d_queueStatuses = bsl::move(rhs.d_queueStatuses);
    }

    return *this;
}
#endif

void QueueStatuses::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_queueStatuses);
}

// ACCESSORS

bsl::ostream&
QueueStatuses::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queueStatuses", this->queueStatuses());
    printer.end();
    return stream;
}

// ----------------
// class SetTunable
// ----------------

// CONSTANTS

const char SetTunable::CLASS_NAME[] = "SetTunable";

const bdlat_AttributeInfo SetTunable::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_VALUE,
     "value",
     sizeof("value") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED}};

// CLASS METHODS

const bdlat_AttributeInfo* SetTunable::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    if (bdlb::String::areEqualCaseless("all", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("self", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            SetTunable::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* SetTunable::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_VALUE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE];
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    default: return 0;
    }
}

// CREATORS

SetTunable::SetTunable(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_value(basicAllocator)
, d_choice()
{
}

SetTunable::SetTunable(const SetTunable& original,
                       bslma::Allocator* basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_value(original.d_value, basicAllocator)
, d_choice(original.d_choice)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SetTunable::SetTunable(SetTunable&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_value(bsl::move(original.d_value)),
  d_choice(bsl::move(original.d_choice))
{
}

SetTunable::SetTunable(SetTunable&& original, bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_value(bsl::move(original.d_value), basicAllocator)
, d_choice(bsl::move(original.d_choice))
{
}
#endif

SetTunable::~SetTunable()
{
}

// MANIPULATORS

SetTunable& SetTunable::operator=(const SetTunable& rhs)
{
    if (this != &rhs) {
        d_name   = rhs.d_name;
        d_value  = rhs.d_value;
        d_choice = rhs.d_choice;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SetTunable& SetTunable::operator=(SetTunable&& rhs)
{
    if (this != &rhs) {
        d_name   = bsl::move(rhs.d_name);
        d_value  = bsl::move(rhs.d_value);
        d_choice = bsl::move(rhs.d_choice);
    }

    return *this;
}
#endif

void SetTunable::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_value);
    bdlat_ValueTypeFunctions::reset(&d_choice);
}

// ACCESSORS

bsl::ostream&
SetTunable::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("value", this->value());
    printer.printAttribute("choice", this->choice());
    printer.end();
    return stream;
}

// -------------------
// class StorageDomain
// -------------------

// CONSTANTS

const char StorageDomain::CLASS_NAME[] = "StorageDomain";

const bdlat_AttributeInfo StorageDomain::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_COMMAND,
     "command",
     sizeof("command") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* StorageDomain::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StorageDomain::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StorageDomain::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_COMMAND:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND];
    default: return 0;
    }
}

// CREATORS

StorageDomain::StorageDomain(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_command()
{
}

StorageDomain::StorageDomain(const StorageDomain& original,
                             bslma::Allocator*    basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_command(original.d_command)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageDomain::StorageDomain(StorageDomain&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_command(bsl::move(original.d_command))
{
}

StorageDomain::StorageDomain(StorageDomain&&   original,
                             bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_command(bsl::move(original.d_command))
{
}
#endif

StorageDomain::~StorageDomain()
{
}

// MANIPULATORS

StorageDomain& StorageDomain::operator=(const StorageDomain& rhs)
{
    if (this != &rhs) {
        d_name    = rhs.d_name;
        d_command = rhs.d_command;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageDomain& StorageDomain::operator=(StorageDomain&& rhs)
{
    if (this != &rhs) {
        d_name    = bsl::move(rhs.d_name);
        d_command = bsl::move(rhs.d_command);
    }

    return *this;
}
#endif

void StorageDomain::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_command);
}

// ACCESSORS

bsl::ostream&
StorageDomain::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("command", this->command());
    printer.end();
    return stream;
}

// ----------------------
// class StoragePartition
// ----------------------

// CONSTANTS

const char StoragePartition::CLASS_NAME[] = "StoragePartition";

const bdlat_AttributeInfo StoragePartition::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_COMMAND,
     "command",
     sizeof("command") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StoragePartition::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StoragePartition::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StoragePartition::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_COMMAND:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND];
    default: return 0;
    }
}

// CREATORS

StoragePartition::StoragePartition()
: d_command()
, d_partitionId()
{
}

// MANIPULATORS

void StoragePartition::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_command);
}

// ACCESSORS

bsl::ostream& StoragePartition::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("command", this->command());
    printer.end();
    return stream;
}

// -------------
// class Tunable
// -------------

// CONSTANTS

const char Tunable::CLASS_NAME[] = "Tunable";

const bdlat_AttributeInfo Tunable::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_VALUE,
     "value",
     sizeof("value") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_DESCRIPTION,
     "description",
     sizeof("description") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Tunable::lookupAttributeInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Tunable::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Tunable::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_VALUE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE];
    case ATTRIBUTE_ID_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DESCRIPTION];
    default: return 0;
    }
}

// CREATORS

Tunable::Tunable(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_description(basicAllocator)
, d_value(basicAllocator)
{
}

Tunable::Tunable(const Tunable& original, bslma::Allocator* basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_description(original.d_description, basicAllocator)
, d_value(original.d_value, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunable::Tunable(Tunable&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_description(bsl::move(original.d_description)),
  d_value(bsl::move(original.d_value))
{
}

Tunable::Tunable(Tunable&& original, bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_description(bsl::move(original.d_description), basicAllocator)
, d_value(bsl::move(original.d_value), basicAllocator)
{
}
#endif

Tunable::~Tunable()
{
}

// MANIPULATORS

Tunable& Tunable::operator=(const Tunable& rhs)
{
    if (this != &rhs) {
        d_name        = rhs.d_name;
        d_value       = rhs.d_value;
        d_description = rhs.d_description;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunable& Tunable::operator=(Tunable&& rhs)
{
    if (this != &rhs) {
        d_name        = bsl::move(rhs.d_name);
        d_value       = bsl::move(rhs.d_value);
        d_description = bsl::move(rhs.d_description);
    }

    return *this;
}
#endif

void Tunable::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_value);
    bdlat_ValueTypeFunctions::reset(&d_description);
}

// ACCESSORS

bsl::ostream&
Tunable::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("value", this->value());
    printer.printAttribute("description", this->description());
    printer.end();
    return stream;
}

// -------------------------
// class TunableConfirmation
// -------------------------

// CONSTANTS

const char TunableConfirmation::CLASS_NAME[] = "TunableConfirmation";

const bdlat_AttributeInfo TunableConfirmation::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_OLD_VALUE,
     "oldValue",
     sizeof("oldValue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_NEW_VALUE,
     "newValue",
     sizeof("newValue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
TunableConfirmation::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            TunableConfirmation::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* TunableConfirmation::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_OLD_VALUE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OLD_VALUE];
    case ATTRIBUTE_ID_NEW_VALUE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NEW_VALUE];
    default: return 0;
    }
}

// CREATORS

TunableConfirmation::TunableConfirmation(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_oldValue(basicAllocator)
, d_newValue(basicAllocator)
{
}

TunableConfirmation::TunableConfirmation(const TunableConfirmation& original,
                                         bslma::Allocator* basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_oldValue(original.d_oldValue, basicAllocator)
, d_newValue(original.d_newValue, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TunableConfirmation::TunableConfirmation(TunableConfirmation&& original)
    noexcept : d_name(bsl::move(original.d_name)),
               d_oldValue(bsl::move(original.d_oldValue)),
               d_newValue(bsl::move(original.d_newValue))
{
}

TunableConfirmation::TunableConfirmation(TunableConfirmation&& original,
                                         bslma::Allocator*     basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_oldValue(bsl::move(original.d_oldValue), basicAllocator)
, d_newValue(bsl::move(original.d_newValue), basicAllocator)
{
}
#endif

TunableConfirmation::~TunableConfirmation()
{
}

// MANIPULATORS

TunableConfirmation&
TunableConfirmation::operator=(const TunableConfirmation& rhs)
{
    if (this != &rhs) {
        d_name     = rhs.d_name;
        d_oldValue = rhs.d_oldValue;
        d_newValue = rhs.d_newValue;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TunableConfirmation& TunableConfirmation::operator=(TunableConfirmation&& rhs)
{
    if (this != &rhs) {
        d_name     = bsl::move(rhs.d_name);
        d_oldValue = bsl::move(rhs.d_oldValue);
        d_newValue = bsl::move(rhs.d_newValue);
    }

    return *this;
}
#endif

void TunableConfirmation::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_oldValue);
    bdlat_ValueTypeFunctions::reset(&d_newValue);
}

// ACCESSORS

bsl::ostream& TunableConfirmation::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("oldValue", this->oldValue());
    printer.printAttribute("newValue", this->newValue());
    printer.end();
    return stream;
}

// --------------------------------
// class ClusterDomainQueueStatuses
// --------------------------------

// CONSTANTS

const char ClusterDomainQueueStatuses::CLASS_NAME[] =
    "ClusterDomainQueueStatuses";

const bdlat_AttributeInfo ClusterDomainQueueStatuses::ATTRIBUTE_INFO_ARRAY[] =
    {{ATTRIBUTE_ID_STATUSES,
      "statuses",
      sizeof("statuses") - 1,
      "",
      bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterDomainQueueStatuses::lookupAttributeInfo(const char* name,
                                                int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterDomainQueueStatuses::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
ClusterDomainQueueStatuses::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_STATUSES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATUSES];
    default: return 0;
    }
}

// CREATORS

ClusterDomainQueueStatuses::ClusterDomainQueueStatuses(
    bslma::Allocator* basicAllocator)
: d_statuses(basicAllocator)
{
}

ClusterDomainQueueStatuses::ClusterDomainQueueStatuses(
    const ClusterDomainQueueStatuses& original,
    bslma::Allocator*                 basicAllocator)
: d_statuses(original.d_statuses, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterDomainQueueStatuses::ClusterDomainQueueStatuses(
    ClusterDomainQueueStatuses&& original) noexcept
: d_statuses(bsl::move(original.d_statuses))
{
}

ClusterDomainQueueStatuses::ClusterDomainQueueStatuses(
    ClusterDomainQueueStatuses&& original,
    bslma::Allocator*            basicAllocator)
: d_statuses(bsl::move(original.d_statuses), basicAllocator)
{
}
#endif

ClusterDomainQueueStatuses::~ClusterDomainQueueStatuses()
{
}

// MANIPULATORS

ClusterDomainQueueStatuses&
ClusterDomainQueueStatuses::operator=(const ClusterDomainQueueStatuses& rhs)
{
    if (this != &rhs) {
        d_statuses = rhs.d_statuses;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterDomainQueueStatuses&
ClusterDomainQueueStatuses::operator=(ClusterDomainQueueStatuses&& rhs)
{
    if (this != &rhs) {
        d_statuses = bsl::move(rhs.d_statuses);
    }

    return *this;
}
#endif

void ClusterDomainQueueStatuses::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_statuses);
}

// ACCESSORS

bsl::ostream& ClusterDomainQueueStatuses::print(bsl::ostream& stream,
                                                int           level,
                                                int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("statuses", this->statuses());
    printer.end();
    return stream;
}

// ------------------------
// class ClusterProxyStatus
// ------------------------

// CONSTANTS

const char ClusterProxyStatus::CLASS_NAME[] = "ClusterProxyStatus";

const bdlat_AttributeInfo ClusterProxyStatus::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_DESCRIPTION,
     "description",
     sizeof("description") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ACTIVE_NODE_DESCRIPTION,
     "activeNodeDescription",
     sizeof("activeNodeDescription") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_IS_HEALTHY,
     "isHealthy",
     sizeof("isHealthy") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NODE_STATUSES,
     "nodeStatuses",
     sizeof("nodeStatuses") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUES_INFO,
     "queuesInfo",
     sizeof("queuesInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterProxyStatus::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterProxyStatus::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterProxyStatus::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DESCRIPTION];
    case ATTRIBUTE_ID_ACTIVE_NODE_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACTIVE_NODE_DESCRIPTION];
    case ATTRIBUTE_ID_IS_HEALTHY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_HEALTHY];
    case ATTRIBUTE_ID_NODE_STATUSES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_STATUSES];
    case ATTRIBUTE_ID_QUEUES_INFO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES_INFO];
    default: return 0;
    }
}

// CREATORS

ClusterProxyStatus::ClusterProxyStatus(bslma::Allocator* basicAllocator)
: d_description(basicAllocator)
, d_activeNodeDescription(basicAllocator)
, d_queuesInfo(basicAllocator)
, d_nodeStatuses(basicAllocator)
, d_isHealthy()
{
}

ClusterProxyStatus::ClusterProxyStatus(const ClusterProxyStatus& original,
                                       bslma::Allocator* basicAllocator)
: d_description(original.d_description, basicAllocator)
, d_activeNodeDescription(original.d_activeNodeDescription, basicAllocator)
, d_queuesInfo(original.d_queuesInfo, basicAllocator)
, d_nodeStatuses(original.d_nodeStatuses, basicAllocator)
, d_isHealthy(original.d_isHealthy)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterProxyStatus::ClusterProxyStatus(ClusterProxyStatus&& original) noexcept
: d_description(bsl::move(original.d_description)),
  d_activeNodeDescription(bsl::move(original.d_activeNodeDescription)),
  d_queuesInfo(bsl::move(original.d_queuesInfo)),
  d_nodeStatuses(bsl::move(original.d_nodeStatuses)),
  d_isHealthy(bsl::move(original.d_isHealthy))
{
}

ClusterProxyStatus::ClusterProxyStatus(ClusterProxyStatus&& original,
                                       bslma::Allocator*    basicAllocator)
: d_description(bsl::move(original.d_description), basicAllocator)
, d_activeNodeDescription(bsl::move(original.d_activeNodeDescription),
                          basicAllocator)
, d_queuesInfo(bsl::move(original.d_queuesInfo), basicAllocator)
, d_nodeStatuses(bsl::move(original.d_nodeStatuses), basicAllocator)
, d_isHealthy(bsl::move(original.d_isHealthy))
{
}
#endif

ClusterProxyStatus::~ClusterProxyStatus()
{
}

// MANIPULATORS

ClusterProxyStatus&
ClusterProxyStatus::operator=(const ClusterProxyStatus& rhs)
{
    if (this != &rhs) {
        d_description           = rhs.d_description;
        d_activeNodeDescription = rhs.d_activeNodeDescription;
        d_isHealthy             = rhs.d_isHealthy;
        d_nodeStatuses          = rhs.d_nodeStatuses;
        d_queuesInfo            = rhs.d_queuesInfo;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterProxyStatus& ClusterProxyStatus::operator=(ClusterProxyStatus&& rhs)
{
    if (this != &rhs) {
        d_description           = bsl::move(rhs.d_description);
        d_activeNodeDescription = bsl::move(rhs.d_activeNodeDescription);
        d_isHealthy             = bsl::move(rhs.d_isHealthy);
        d_nodeStatuses          = bsl::move(rhs.d_nodeStatuses);
        d_queuesInfo            = bsl::move(rhs.d_queuesInfo);
    }

    return *this;
}
#endif

void ClusterProxyStatus::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_description);
    bdlat_ValueTypeFunctions::reset(&d_activeNodeDescription);
    bdlat_ValueTypeFunctions::reset(&d_isHealthy);
    bdlat_ValueTypeFunctions::reset(&d_nodeStatuses);
    bdlat_ValueTypeFunctions::reset(&d_queuesInfo);
}

// ACCESSORS

bsl::ostream& ClusterProxyStatus::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("description", this->description());
    printer.printAttribute("activeNodeDescription",
                           this->activeNodeDescription());
    printer.printAttribute("isHealthy", this->isHealthy());
    printer.printAttribute("nodeStatuses", this->nodeStatuses());
    printer.printAttribute("queuesInfo", this->queuesInfo());
    printer.end();
    return stream;
}

// -------------------
// class DomainCommand
// -------------------

// CONSTANTS

const char DomainCommand::CLASS_NAME[] = "DomainCommand";

const bdlat_SelectionInfo DomainCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_PURGE,
     "purge",
     sizeof("purge") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_INFO,
     "info",
     sizeof("info") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE,
     "queue",
     sizeof("queue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* DomainCommand::lookupSelectionInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            DomainCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* DomainCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_PURGE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGE];
    case SELECTION_ID_INFO: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_INFO];
    case SELECTION_ID_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE];
    default: return 0;
    }
}

// CREATORS

DomainCommand::DomainCommand(const DomainCommand& original,
                             bslma::Allocator*    basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE: {
        new (d_purge.buffer()) Void(original.d_purge.object());
    } break;
    case SELECTION_ID_INFO: {
        new (d_info.buffer()) Void(original.d_info.object());
    } break;
    case SELECTION_ID_QUEUE: {
        new (d_queue.buffer())
            DomainQueue(original.d_queue.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainCommand::DomainCommand(DomainCommand&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE: {
        new (d_purge.buffer()) Void(bsl::move(original.d_purge.object()));
    } break;
    case SELECTION_ID_INFO: {
        new (d_info.buffer()) Void(bsl::move(original.d_info.object()));
    } break;
    case SELECTION_ID_QUEUE: {
        new (d_queue.buffer())
            DomainQueue(bsl::move(original.d_queue.object()), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

DomainCommand::DomainCommand(DomainCommand&&   original,
                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE: {
        new (d_purge.buffer()) Void(bsl::move(original.d_purge.object()));
    } break;
    case SELECTION_ID_INFO: {
        new (d_info.buffer()) Void(bsl::move(original.d_info.object()));
    } break;
    case SELECTION_ID_QUEUE: {
        new (d_queue.buffer())
            DomainQueue(bsl::move(original.d_queue.object()), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

DomainCommand& DomainCommand::operator=(const DomainCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_PURGE: {
            makePurge(rhs.d_purge.object());
        } break;
        case SELECTION_ID_INFO: {
            makeInfo(rhs.d_info.object());
        } break;
        case SELECTION_ID_QUEUE: {
            makeQueue(rhs.d_queue.object());
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
DomainCommand& DomainCommand::operator=(DomainCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_PURGE: {
            makePurge(bsl::move(rhs.d_purge.object()));
        } break;
        case SELECTION_ID_INFO: {
            makeInfo(bsl::move(rhs.d_info.object()));
        } break;
        case SELECTION_ID_QUEUE: {
            makeQueue(bsl::move(rhs.d_queue.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void DomainCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE: {
        d_purge.object().~Void();
    } break;
    case SELECTION_ID_INFO: {
        d_info.object().~Void();
    } break;
    case SELECTION_ID_QUEUE: {
        d_queue.object().~DomainQueue();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int DomainCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_PURGE: {
        makePurge();
    } break;
    case SELECTION_ID_INFO: {
        makeInfo();
    } break;
    case SELECTION_ID_QUEUE: {
        makeQueue();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int DomainCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& DomainCommand::makePurge()
{
    if (SELECTION_ID_PURGE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_purge.object());
    }
    else {
        reset();
        new (d_purge.buffer()) Void();
        d_selectionId = SELECTION_ID_PURGE;
    }

    return d_purge.object();
}

Void& DomainCommand::makePurge(const Void& value)
{
    if (SELECTION_ID_PURGE == d_selectionId) {
        d_purge.object() = value;
    }
    else {
        reset();
        new (d_purge.buffer()) Void(value);
        d_selectionId = SELECTION_ID_PURGE;
    }

    return d_purge.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& DomainCommand::makePurge(Void&& value)
{
    if (SELECTION_ID_PURGE == d_selectionId) {
        d_purge.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_purge.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_PURGE;
    }

    return d_purge.object();
}
#endif

Void& DomainCommand::makeInfo()
{
    if (SELECTION_ID_INFO == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_info.object());
    }
    else {
        reset();
        new (d_info.buffer()) Void();
        d_selectionId = SELECTION_ID_INFO;
    }

    return d_info.object();
}

Void& DomainCommand::makeInfo(const Void& value)
{
    if (SELECTION_ID_INFO == d_selectionId) {
        d_info.object() = value;
    }
    else {
        reset();
        new (d_info.buffer()) Void(value);
        d_selectionId = SELECTION_ID_INFO;
    }

    return d_info.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& DomainCommand::makeInfo(Void&& value)
{
    if (SELECTION_ID_INFO == d_selectionId) {
        d_info.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_info.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_INFO;
    }

    return d_info.object();
}
#endif

DomainQueue& DomainCommand::makeQueue()
{
    if (SELECTION_ID_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queue.object());
    }
    else {
        reset();
        new (d_queue.buffer()) DomainQueue(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE;
    }

    return d_queue.object();
}

DomainQueue& DomainCommand::makeQueue(const DomainQueue& value)
{
    if (SELECTION_ID_QUEUE == d_selectionId) {
        d_queue.object() = value;
    }
    else {
        reset();
        new (d_queue.buffer()) DomainQueue(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE;
    }

    return d_queue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainQueue& DomainCommand::makeQueue(DomainQueue&& value)
{
    if (SELECTION_ID_QUEUE == d_selectionId) {
        d_queue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queue.buffer()) DomainQueue(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE;
    }

    return d_queue.object();
}
#endif

// ACCESSORS

bsl::ostream&
DomainCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_PURGE: {
        printer.printAttribute("purge", d_purge.object());
    } break;
    case SELECTION_ID_INFO: {
        printer.printAttribute("info", d_info.object());
    } break;
    case SELECTION_ID_QUEUE: {
        printer.printAttribute("queue", d_queue.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* DomainCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_PURGE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGE].name();
    case SELECTION_ID_INFO:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_INFO].name();
    case SELECTION_ID_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------------
// class ElectorCommand
// --------------------

// CONSTANTS

const char ElectorCommand::CLASS_NAME[] = "ElectorCommand";

const bdlat_SelectionInfo ElectorCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_SET_TUNABLE,
     "setTunable",
     sizeof("setTunable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_GET_TUNABLE,
     "getTunable",
     sizeof("getTunable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LIST_TUNABLES,
     "listTunables",
     sizeof("listTunables") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ElectorCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ElectorCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ElectorCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_SET_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SET_TUNABLE];
    case SELECTION_ID_GET_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_GET_TUNABLE];
    case SELECTION_ID_LIST_TUNABLES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_TUNABLES];
    default: return 0;
    }
}

// CREATORS

ElectorCommand::ElectorCommand(const ElectorCommand& original,
                               bslma::Allocator*     basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        new (d_setTunable.buffer())
            SetTunable(original.d_setTunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        new (d_getTunable.buffer())
            GetTunable(original.d_getTunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        new (d_listTunables.buffer()) Void(original.d_listTunables.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectorCommand::ElectorCommand(ElectorCommand&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        new (d_setTunable.buffer())
            SetTunable(bsl::move(original.d_setTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        new (d_getTunable.buffer())
            GetTunable(bsl::move(original.d_getTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        new (d_listTunables.buffer())
            Void(bsl::move(original.d_listTunables.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ElectorCommand::ElectorCommand(ElectorCommand&&  original,
                               bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        new (d_setTunable.buffer())
            SetTunable(bsl::move(original.d_setTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        new (d_getTunable.buffer())
            GetTunable(bsl::move(original.d_getTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        new (d_listTunables.buffer())
            Void(bsl::move(original.d_listTunables.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ElectorCommand& ElectorCommand::operator=(const ElectorCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SET_TUNABLE: {
            makeSetTunable(rhs.d_setTunable.object());
        } break;
        case SELECTION_ID_GET_TUNABLE: {
            makeGetTunable(rhs.d_getTunable.object());
        } break;
        case SELECTION_ID_LIST_TUNABLES: {
            makeListTunables(rhs.d_listTunables.object());
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
ElectorCommand& ElectorCommand::operator=(ElectorCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SET_TUNABLE: {
            makeSetTunable(bsl::move(rhs.d_setTunable.object()));
        } break;
        case SELECTION_ID_GET_TUNABLE: {
            makeGetTunable(bsl::move(rhs.d_getTunable.object()));
        } break;
        case SELECTION_ID_LIST_TUNABLES: {
            makeListTunables(bsl::move(rhs.d_listTunables.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ElectorCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        d_setTunable.object().~SetTunable();
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        d_getTunable.object().~GetTunable();
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        d_listTunables.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ElectorCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        makeSetTunable();
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        makeGetTunable();
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        makeListTunables();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ElectorCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

SetTunable& ElectorCommand::makeSetTunable()
{
    if (SELECTION_ID_SET_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_setTunable.object());
    }
    else {
        reset();
        new (d_setTunable.buffer()) SetTunable(d_allocator_p);
        d_selectionId = SELECTION_ID_SET_TUNABLE;
    }

    return d_setTunable.object();
}

SetTunable& ElectorCommand::makeSetTunable(const SetTunable& value)
{
    if (SELECTION_ID_SET_TUNABLE == d_selectionId) {
        d_setTunable.object() = value;
    }
    else {
        reset();
        new (d_setTunable.buffer()) SetTunable(value, d_allocator_p);
        d_selectionId = SELECTION_ID_SET_TUNABLE;
    }

    return d_setTunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SetTunable& ElectorCommand::makeSetTunable(SetTunable&& value)
{
    if (SELECTION_ID_SET_TUNABLE == d_selectionId) {
        d_setTunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_setTunable.buffer())
            SetTunable(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_SET_TUNABLE;
    }

    return d_setTunable.object();
}
#endif

GetTunable& ElectorCommand::makeGetTunable()
{
    if (SELECTION_ID_GET_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_getTunable.object());
    }
    else {
        reset();
        new (d_getTunable.buffer()) GetTunable(d_allocator_p);
        d_selectionId = SELECTION_ID_GET_TUNABLE;
    }

    return d_getTunable.object();
}

GetTunable& ElectorCommand::makeGetTunable(const GetTunable& value)
{
    if (SELECTION_ID_GET_TUNABLE == d_selectionId) {
        d_getTunable.object() = value;
    }
    else {
        reset();
        new (d_getTunable.buffer()) GetTunable(value, d_allocator_p);
        d_selectionId = SELECTION_ID_GET_TUNABLE;
    }

    return d_getTunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
GetTunable& ElectorCommand::makeGetTunable(GetTunable&& value)
{
    if (SELECTION_ID_GET_TUNABLE == d_selectionId) {
        d_getTunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_getTunable.buffer())
            GetTunable(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_GET_TUNABLE;
    }

    return d_getTunable.object();
}
#endif

Void& ElectorCommand::makeListTunables()
{
    if (SELECTION_ID_LIST_TUNABLES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_listTunables.object());
    }
    else {
        reset();
        new (d_listTunables.buffer()) Void();
        d_selectionId = SELECTION_ID_LIST_TUNABLES;
    }

    return d_listTunables.object();
}

Void& ElectorCommand::makeListTunables(const Void& value)
{
    if (SELECTION_ID_LIST_TUNABLES == d_selectionId) {
        d_listTunables.object() = value;
    }
    else {
        reset();
        new (d_listTunables.buffer()) Void(value);
        d_selectionId = SELECTION_ID_LIST_TUNABLES;
    }

    return d_listTunables.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& ElectorCommand::makeListTunables(Void&& value)
{
    if (SELECTION_ID_LIST_TUNABLES == d_selectionId) {
        d_listTunables.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_listTunables.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_LIST_TUNABLES;
    }

    return d_listTunables.object();
}
#endif

// ACCESSORS

bsl::ostream& ElectorCommand::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        printer.printAttribute("setTunable", d_setTunable.object());
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        printer.printAttribute("getTunable", d_getTunable.object());
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        printer.printAttribute("listTunables", d_listTunables.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ElectorCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SET_TUNABLE].name();
    case SELECTION_ID_GET_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_GET_TUNABLE].name();
    case SELECTION_ID_LIST_TUNABLES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_TUNABLES].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ---------------
// class FileStore
// ---------------

// CONSTANTS

const char FileStore::CLASS_NAME[] = "FileStore";

const bdlat_AttributeInfo FileStore::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STATE,
     "state",
     sizeof("state") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_SUMMARY,
     "summary",
     sizeof("summary") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* FileStore::lookupAttributeInfo(const char* name,
                                                          int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            FileStore::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* FileStore::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_STATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATE];
    case ATTRIBUTE_ID_SUMMARY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUMMARY];
    default: return 0;
    }
}

// CREATORS

FileStore::FileStore(bslma::Allocator* basicAllocator)
: d_summary(basicAllocator)
, d_partitionId()
, d_state(static_cast<FileStoreState::Value>(0))
{
}

FileStore::FileStore(const FileStore&  original,
                     bslma::Allocator* basicAllocator)
: d_summary(original.d_summary, basicAllocator)
, d_partitionId(original.d_partitionId)
, d_state(original.d_state)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FileStore::FileStore(FileStore&& original) noexcept
: d_summary(bsl::move(original.d_summary)),
  d_partitionId(bsl::move(original.d_partitionId)),
  d_state(bsl::move(original.d_state))
{
}

FileStore::FileStore(FileStore&& original, bslma::Allocator* basicAllocator)
: d_summary(bsl::move(original.d_summary), basicAllocator)
, d_partitionId(bsl::move(original.d_partitionId))
, d_state(bsl::move(original.d_state))
{
}
#endif

FileStore::~FileStore()
{
}

// MANIPULATORS

FileStore& FileStore::operator=(const FileStore& rhs)
{
    if (this != &rhs) {
        d_partitionId = rhs.d_partitionId;
        d_state       = rhs.d_state;
        d_summary     = rhs.d_summary;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FileStore& FileStore::operator=(FileStore&& rhs)
{
    if (this != &rhs) {
        d_partitionId = bsl::move(rhs.d_partitionId);
        d_state       = bsl::move(rhs.d_state);
        d_summary     = bsl::move(rhs.d_summary);
    }

    return *this;
}
#endif

void FileStore::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_state);
    bdlat_ValueTypeFunctions::reset(&d_summary);
}

// ACCESSORS

bsl::ostream&
FileStore::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("state", this->state());
    printer.printAttribute("summary", this->summary());
    printer.end();
    return stream;
}

// -----------------
// class QueueHandle
// -----------------

// CONSTANTS

const char QueueHandle::CLASS_NAME[] = "QueueHandle";

const bdlat_AttributeInfo QueueHandle::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLIENT_DESCRIPTION,
     "clientDescription",
     sizeof("clientDescription") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PARAMETERS_JSON,
     "parametersJson",
     sizeof("parametersJson") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_IS_CLIENT_CLUSTER_MEMBER,
     "isClientClusterMember",
     sizeof("isClientClusterMember") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_SUB_STREAMS,
     "subStreams",
     sizeof("subStreams") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* QueueHandle::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueHandle::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueHandle::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLIENT_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT_DESCRIPTION];
    case ATTRIBUTE_ID_PARAMETERS_JSON:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARAMETERS_JSON];
    case ATTRIBUTE_ID_IS_CLIENT_CLUSTER_MEMBER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_CLIENT_CLUSTER_MEMBER];
    case ATTRIBUTE_ID_SUB_STREAMS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUB_STREAMS];
    default: return 0;
    }
}

// CREATORS

QueueHandle::QueueHandle(bslma::Allocator* basicAllocator)
: d_subStreams(basicAllocator)
, d_clientDescription(basicAllocator)
, d_parametersJson(basicAllocator)
, d_isClientClusterMember()
{
}

QueueHandle::QueueHandle(const QueueHandle& original,
                         bslma::Allocator*  basicAllocator)
: d_subStreams(original.d_subStreams, basicAllocator)
, d_clientDescription(original.d_clientDescription, basicAllocator)
, d_parametersJson(original.d_parametersJson, basicAllocator)
, d_isClientClusterMember(original.d_isClientClusterMember)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueHandle::QueueHandle(QueueHandle&& original) noexcept
: d_subStreams(bsl::move(original.d_subStreams)),
  d_clientDescription(bsl::move(original.d_clientDescription)),
  d_parametersJson(bsl::move(original.d_parametersJson)),
  d_isClientClusterMember(bsl::move(original.d_isClientClusterMember))
{
}

QueueHandle::QueueHandle(QueueHandle&&     original,
                         bslma::Allocator* basicAllocator)
: d_subStreams(bsl::move(original.d_subStreams), basicAllocator)
, d_clientDescription(bsl::move(original.d_clientDescription), basicAllocator)
, d_parametersJson(bsl::move(original.d_parametersJson), basicAllocator)
, d_isClientClusterMember(bsl::move(original.d_isClientClusterMember))
{
}
#endif

QueueHandle::~QueueHandle()
{
}

// MANIPULATORS

QueueHandle& QueueHandle::operator=(const QueueHandle& rhs)
{
    if (this != &rhs) {
        d_clientDescription     = rhs.d_clientDescription;
        d_parametersJson        = rhs.d_parametersJson;
        d_isClientClusterMember = rhs.d_isClientClusterMember;
        d_subStreams            = rhs.d_subStreams;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueHandle& QueueHandle::operator=(QueueHandle&& rhs)
{
    if (this != &rhs) {
        d_clientDescription     = bsl::move(rhs.d_clientDescription);
        d_parametersJson        = bsl::move(rhs.d_parametersJson);
        d_isClientClusterMember = bsl::move(rhs.d_isClientClusterMember);
        d_subStreams            = bsl::move(rhs.d_subStreams);
    }

    return *this;
}
#endif

void QueueHandle::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clientDescription);
    bdlat_ValueTypeFunctions::reset(&d_parametersJson);
    bdlat_ValueTypeFunctions::reset(&d_isClientClusterMember);
    bdlat_ValueTypeFunctions::reset(&d_subStreams);
}

// ACCESSORS

bsl::ostream&
QueueHandle::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clientDescription", this->clientDescription());
    printer.printAttribute("parametersJson", this->parametersJson());
    printer.printAttribute("isClientClusterMember",
                           this->isClientClusterMember());
    printer.printAttribute("subStreams", this->subStreams());
    printer.end();
    return stream;
}

// ------------------------
// class ReplicationCommand
// ------------------------

// CONSTANTS

const char ReplicationCommand::CLASS_NAME[] = "ReplicationCommand";

const bdlat_SelectionInfo ReplicationCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_SET_TUNABLE,
     "setTunable",
     sizeof("setTunable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_GET_TUNABLE,
     "getTunable",
     sizeof("getTunable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_LIST_TUNABLES,
     "listTunables",
     sizeof("listTunables") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ReplicationCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ReplicationCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ReplicationCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_SET_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SET_TUNABLE];
    case SELECTION_ID_GET_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_GET_TUNABLE];
    case SELECTION_ID_LIST_TUNABLES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_TUNABLES];
    default: return 0;
    }
}

// CREATORS

ReplicationCommand::ReplicationCommand(const ReplicationCommand& original,
                                       bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        new (d_setTunable.buffer())
            SetTunable(original.d_setTunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        new (d_getTunable.buffer())
            GetTunable(original.d_getTunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        new (d_listTunables.buffer()) Void(original.d_listTunables.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReplicationCommand::ReplicationCommand(ReplicationCommand&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        new (d_setTunable.buffer())
            SetTunable(bsl::move(original.d_setTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        new (d_getTunable.buffer())
            GetTunable(bsl::move(original.d_getTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        new (d_listTunables.buffer())
            Void(bsl::move(original.d_listTunables.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ReplicationCommand::ReplicationCommand(ReplicationCommand&& original,
                                       bslma::Allocator*    basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        new (d_setTunable.buffer())
            SetTunable(bsl::move(original.d_setTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        new (d_getTunable.buffer())
            GetTunable(bsl::move(original.d_getTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        new (d_listTunables.buffer())
            Void(bsl::move(original.d_listTunables.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ReplicationCommand&
ReplicationCommand::operator=(const ReplicationCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SET_TUNABLE: {
            makeSetTunable(rhs.d_setTunable.object());
        } break;
        case SELECTION_ID_GET_TUNABLE: {
            makeGetTunable(rhs.d_getTunable.object());
        } break;
        case SELECTION_ID_LIST_TUNABLES: {
            makeListTunables(rhs.d_listTunables.object());
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
ReplicationCommand& ReplicationCommand::operator=(ReplicationCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SET_TUNABLE: {
            makeSetTunable(bsl::move(rhs.d_setTunable.object()));
        } break;
        case SELECTION_ID_GET_TUNABLE: {
            makeGetTunable(bsl::move(rhs.d_getTunable.object()));
        } break;
        case SELECTION_ID_LIST_TUNABLES: {
            makeListTunables(bsl::move(rhs.d_listTunables.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ReplicationCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        d_setTunable.object().~SetTunable();
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        d_getTunable.object().~GetTunable();
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        d_listTunables.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ReplicationCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        makeSetTunable();
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        makeGetTunable();
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        makeListTunables();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ReplicationCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

SetTunable& ReplicationCommand::makeSetTunable()
{
    if (SELECTION_ID_SET_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_setTunable.object());
    }
    else {
        reset();
        new (d_setTunable.buffer()) SetTunable(d_allocator_p);
        d_selectionId = SELECTION_ID_SET_TUNABLE;
    }

    return d_setTunable.object();
}

SetTunable& ReplicationCommand::makeSetTunable(const SetTunable& value)
{
    if (SELECTION_ID_SET_TUNABLE == d_selectionId) {
        d_setTunable.object() = value;
    }
    else {
        reset();
        new (d_setTunable.buffer()) SetTunable(value, d_allocator_p);
        d_selectionId = SELECTION_ID_SET_TUNABLE;
    }

    return d_setTunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SetTunable& ReplicationCommand::makeSetTunable(SetTunable&& value)
{
    if (SELECTION_ID_SET_TUNABLE == d_selectionId) {
        d_setTunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_setTunable.buffer())
            SetTunable(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_SET_TUNABLE;
    }

    return d_setTunable.object();
}
#endif

GetTunable& ReplicationCommand::makeGetTunable()
{
    if (SELECTION_ID_GET_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_getTunable.object());
    }
    else {
        reset();
        new (d_getTunable.buffer()) GetTunable(d_allocator_p);
        d_selectionId = SELECTION_ID_GET_TUNABLE;
    }

    return d_getTunable.object();
}

GetTunable& ReplicationCommand::makeGetTunable(const GetTunable& value)
{
    if (SELECTION_ID_GET_TUNABLE == d_selectionId) {
        d_getTunable.object() = value;
    }
    else {
        reset();
        new (d_getTunable.buffer()) GetTunable(value, d_allocator_p);
        d_selectionId = SELECTION_ID_GET_TUNABLE;
    }

    return d_getTunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
GetTunable& ReplicationCommand::makeGetTunable(GetTunable&& value)
{
    if (SELECTION_ID_GET_TUNABLE == d_selectionId) {
        d_getTunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_getTunable.buffer())
            GetTunable(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_GET_TUNABLE;
    }

    return d_getTunable.object();
}
#endif

Void& ReplicationCommand::makeListTunables()
{
    if (SELECTION_ID_LIST_TUNABLES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_listTunables.object());
    }
    else {
        reset();
        new (d_listTunables.buffer()) Void();
        d_selectionId = SELECTION_ID_LIST_TUNABLES;
    }

    return d_listTunables.object();
}

Void& ReplicationCommand::makeListTunables(const Void& value)
{
    if (SELECTION_ID_LIST_TUNABLES == d_selectionId) {
        d_listTunables.object() = value;
    }
    else {
        reset();
        new (d_listTunables.buffer()) Void(value);
        d_selectionId = SELECTION_ID_LIST_TUNABLES;
    }

    return d_listTunables.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& ReplicationCommand::makeListTunables(Void&& value)
{
    if (SELECTION_ID_LIST_TUNABLES == d_selectionId) {
        d_listTunables.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_listTunables.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_LIST_TUNABLES;
    }

    return d_listTunables.object();
}
#endif

// ACCESSORS

bsl::ostream& ReplicationCommand::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE: {
        printer.printAttribute("setTunable", d_setTunable.object());
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        printer.printAttribute("getTunable", d_getTunable.object());
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        printer.printAttribute("listTunables", d_listTunables.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ReplicationCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_SET_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SET_TUNABLE].name();
    case SELECTION_ID_GET_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_GET_TUNABLE].name();
    case SELECTION_ID_LIST_TUNABLES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_TUNABLES].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------
// class StatCommand
// -----------------

// CONSTANTS

const char StatCommand::CLASS_NAME[] = "StatCommand";

const bdlat_SelectionInfo StatCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_SHOW,
     "show",
     sizeof("show") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SET_TUNABLE,
     "setTunable",
     sizeof("setTunable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_GET_TUNABLE,
     "getTunable",
     sizeof("getTunable") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {SELECTION_ID_LIST_TUNABLES,
     "listTunables",
     sizeof("listTunables") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* StatCommand::lookupSelectionInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            StatCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* StatCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_SHOW: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SHOW];
    case SELECTION_ID_SET_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SET_TUNABLE];
    case SELECTION_ID_GET_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_GET_TUNABLE];
    case SELECTION_ID_LIST_TUNABLES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_TUNABLES];
    default: return 0;
    }
}

// CREATORS

StatCommand::StatCommand(const StatCommand& original,
                         bslma::Allocator*  basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SHOW: {
        new (d_show.buffer()) Void(original.d_show.object());
    } break;
    case SELECTION_ID_SET_TUNABLE: {
        new (d_setTunable.buffer())
            SetTunable(original.d_setTunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        new (d_getTunable.buffer())
            bsl::string(original.d_getTunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        new (d_listTunables.buffer()) Void(original.d_listTunables.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatCommand::StatCommand(StatCommand&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_SHOW: {
        new (d_show.buffer()) Void(bsl::move(original.d_show.object()));
    } break;
    case SELECTION_ID_SET_TUNABLE: {
        new (d_setTunable.buffer())
            SetTunable(bsl::move(original.d_setTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        new (d_getTunable.buffer())
            bsl::string(bsl::move(original.d_getTunable.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        new (d_listTunables.buffer())
            Void(bsl::move(original.d_listTunables.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

StatCommand::StatCommand(StatCommand&&     original,
                         bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SHOW: {
        new (d_show.buffer()) Void(bsl::move(original.d_show.object()));
    } break;
    case SELECTION_ID_SET_TUNABLE: {
        new (d_setTunable.buffer())
            SetTunable(bsl::move(original.d_setTunable.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        new (d_getTunable.buffer())
            bsl::string(bsl::move(original.d_getTunable.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        new (d_listTunables.buffer())
            Void(bsl::move(original.d_listTunables.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

StatCommand& StatCommand::operator=(const StatCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SHOW: {
            makeShow(rhs.d_show.object());
        } break;
        case SELECTION_ID_SET_TUNABLE: {
            makeSetTunable(rhs.d_setTunable.object());
        } break;
        case SELECTION_ID_GET_TUNABLE: {
            makeGetTunable(rhs.d_getTunable.object());
        } break;
        case SELECTION_ID_LIST_TUNABLES: {
            makeListTunables(rhs.d_listTunables.object());
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
StatCommand& StatCommand::operator=(StatCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SHOW: {
            makeShow(bsl::move(rhs.d_show.object()));
        } break;
        case SELECTION_ID_SET_TUNABLE: {
            makeSetTunable(bsl::move(rhs.d_setTunable.object()));
        } break;
        case SELECTION_ID_GET_TUNABLE: {
            makeGetTunable(bsl::move(rhs.d_getTunable.object()));
        } break;
        case SELECTION_ID_LIST_TUNABLES: {
            makeListTunables(bsl::move(rhs.d_listTunables.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void StatCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_SHOW: {
        d_show.object().~Void();
    } break;
    case SELECTION_ID_SET_TUNABLE: {
        d_setTunable.object().~SetTunable();
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        typedef bsl::string Type;
        d_getTunable.object().~Type();
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        d_listTunables.object().~Void();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int StatCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_SHOW: {
        makeShow();
    } break;
    case SELECTION_ID_SET_TUNABLE: {
        makeSetTunable();
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        makeGetTunable();
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        makeListTunables();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int StatCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& StatCommand::makeShow()
{
    if (SELECTION_ID_SHOW == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_show.object());
    }
    else {
        reset();
        new (d_show.buffer()) Void();
        d_selectionId = SELECTION_ID_SHOW;
    }

    return d_show.object();
}

Void& StatCommand::makeShow(const Void& value)
{
    if (SELECTION_ID_SHOW == d_selectionId) {
        d_show.object() = value;
    }
    else {
        reset();
        new (d_show.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SHOW;
    }

    return d_show.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& StatCommand::makeShow(Void&& value)
{
    if (SELECTION_ID_SHOW == d_selectionId) {
        d_show.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_show.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SHOW;
    }

    return d_show.object();
}
#endif

SetTunable& StatCommand::makeSetTunable()
{
    if (SELECTION_ID_SET_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_setTunable.object());
    }
    else {
        reset();
        new (d_setTunable.buffer()) SetTunable(d_allocator_p);
        d_selectionId = SELECTION_ID_SET_TUNABLE;
    }

    return d_setTunable.object();
}

SetTunable& StatCommand::makeSetTunable(const SetTunable& value)
{
    if (SELECTION_ID_SET_TUNABLE == d_selectionId) {
        d_setTunable.object() = value;
    }
    else {
        reset();
        new (d_setTunable.buffer()) SetTunable(value, d_allocator_p);
        d_selectionId = SELECTION_ID_SET_TUNABLE;
    }

    return d_setTunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SetTunable& StatCommand::makeSetTunable(SetTunable&& value)
{
    if (SELECTION_ID_SET_TUNABLE == d_selectionId) {
        d_setTunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_setTunable.buffer())
            SetTunable(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_SET_TUNABLE;
    }

    return d_setTunable.object();
}
#endif

bsl::string& StatCommand::makeGetTunable()
{
    if (SELECTION_ID_GET_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_getTunable.object());
    }
    else {
        reset();
        new (d_getTunable.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_GET_TUNABLE;
    }

    return d_getTunable.object();
}

bsl::string& StatCommand::makeGetTunable(const bsl::string& value)
{
    if (SELECTION_ID_GET_TUNABLE == d_selectionId) {
        d_getTunable.object() = value;
    }
    else {
        reset();
        new (d_getTunable.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_GET_TUNABLE;
    }

    return d_getTunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& StatCommand::makeGetTunable(bsl::string&& value)
{
    if (SELECTION_ID_GET_TUNABLE == d_selectionId) {
        d_getTunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_getTunable.buffer())
            bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_GET_TUNABLE;
    }

    return d_getTunable.object();
}
#endif

Void& StatCommand::makeListTunables()
{
    if (SELECTION_ID_LIST_TUNABLES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_listTunables.object());
    }
    else {
        reset();
        new (d_listTunables.buffer()) Void();
        d_selectionId = SELECTION_ID_LIST_TUNABLES;
    }

    return d_listTunables.object();
}

Void& StatCommand::makeListTunables(const Void& value)
{
    if (SELECTION_ID_LIST_TUNABLES == d_selectionId) {
        d_listTunables.object() = value;
    }
    else {
        reset();
        new (d_listTunables.buffer()) Void(value);
        d_selectionId = SELECTION_ID_LIST_TUNABLES;
    }

    return d_listTunables.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& StatCommand::makeListTunables(Void&& value)
{
    if (SELECTION_ID_LIST_TUNABLES == d_selectionId) {
        d_listTunables.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_listTunables.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_LIST_TUNABLES;
    }

    return d_listTunables.object();
}
#endif

// ACCESSORS

bsl::ostream&
StatCommand::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_SHOW: {
        printer.printAttribute("show", d_show.object());
    } break;
    case SELECTION_ID_SET_TUNABLE: {
        printer.printAttribute("setTunable", d_setTunable.object());
    } break;
    case SELECTION_ID_GET_TUNABLE: {
        printer.printAttribute("getTunable", d_getTunable.object());
    } break;
    case SELECTION_ID_LIST_TUNABLES: {
        printer.printAttribute("listTunables", d_listTunables.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* StatCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_SHOW:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SHOW].name();
    case SELECTION_ID_SET_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SET_TUNABLE].name();
    case SELECTION_ID_GET_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_GET_TUNABLE].name();
    case SELECTION_ID_LIST_TUNABLES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_TUNABLES].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------------
// class SubscriptionGroup
// -----------------------

// CONSTANTS

const char SubscriptionGroup::CLASS_NAME[] = "SubscriptionGroup";

const bdlat_AttributeInfo SubscriptionGroup::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ID, "id", sizeof("id") - 1, "", bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_EXPRESSION,
     "expression",
     sizeof("expression") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_UPSTREAM_SUB_QUEUE_ID,
     "upstreamSubQueueId",
     sizeof("upstreamSubQueueId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PRIORITY_GROUP,
     "priorityGroup",
     sizeof("priorityGroup") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
SubscriptionGroup::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            SubscriptionGroup::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* SubscriptionGroup::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID];
    case ATTRIBUTE_ID_EXPRESSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION];
    case ATTRIBUTE_ID_UPSTREAM_SUB_QUEUE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_UPSTREAM_SUB_QUEUE_ID];
    case ATTRIBUTE_ID_PRIORITY_GROUP:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIORITY_GROUP];
    default: return 0;
    }
}

// CREATORS

SubscriptionGroup::SubscriptionGroup(bslma::Allocator* basicAllocator)
: d_expression(basicAllocator)
, d_priorityGroup(basicAllocator)
, d_id()
, d_upstreamSubQueueId()
{
}

SubscriptionGroup::SubscriptionGroup(const SubscriptionGroup& original,
                                     bslma::Allocator*        basicAllocator)
: d_expression(original.d_expression, basicAllocator)
, d_priorityGroup(original.d_priorityGroup, basicAllocator)
, d_id(original.d_id)
, d_upstreamSubQueueId(original.d_upstreamSubQueueId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SubscriptionGroup::SubscriptionGroup(SubscriptionGroup&& original) noexcept
: d_expression(bsl::move(original.d_expression)),
  d_priorityGroup(bsl::move(original.d_priorityGroup)),
  d_id(bsl::move(original.d_id)),
  d_upstreamSubQueueId(bsl::move(original.d_upstreamSubQueueId))
{
}

SubscriptionGroup::SubscriptionGroup(SubscriptionGroup&& original,
                                     bslma::Allocator*   basicAllocator)
: d_expression(bsl::move(original.d_expression), basicAllocator)
, d_priorityGroup(bsl::move(original.d_priorityGroup), basicAllocator)
, d_id(bsl::move(original.d_id))
, d_upstreamSubQueueId(bsl::move(original.d_upstreamSubQueueId))
{
}
#endif

SubscriptionGroup::~SubscriptionGroup()
{
}

// MANIPULATORS

SubscriptionGroup& SubscriptionGroup::operator=(const SubscriptionGroup& rhs)
{
    if (this != &rhs) {
        d_id                 = rhs.d_id;
        d_expression         = rhs.d_expression;
        d_upstreamSubQueueId = rhs.d_upstreamSubQueueId;
        d_priorityGroup      = rhs.d_priorityGroup;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SubscriptionGroup& SubscriptionGroup::operator=(SubscriptionGroup&& rhs)
{
    if (this != &rhs) {
        d_id                 = bsl::move(rhs.d_id);
        d_expression         = bsl::move(rhs.d_expression);
        d_upstreamSubQueueId = bsl::move(rhs.d_upstreamSubQueueId);
        d_priorityGroup      = bsl::move(rhs.d_priorityGroup);
    }

    return *this;
}
#endif

void SubscriptionGroup::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_id);
    bdlat_ValueTypeFunctions::reset(&d_expression);
    bdlat_ValueTypeFunctions::reset(&d_upstreamSubQueueId);
    bdlat_ValueTypeFunctions::reset(&d_priorityGroup);
}

// ACCESSORS

bsl::ostream& SubscriptionGroup::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("id", this->id());
    printer.printAttribute("expression", this->expression());
    printer.printAttribute("upstreamSubQueueId", this->upstreamSubQueueId());
    printer.printAttribute("priorityGroup", this->priorityGroup());
    printer.end();
    return stream;
}

// --------------
// class Tunables
// --------------

// CONSTANTS

const char Tunables::CLASS_NAME[] = "Tunables";

const bdlat_AttributeInfo Tunables::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_TUNABLES,
     "tunables",
     sizeof("tunables") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Tunables::lookupAttributeInfo(const char* name,
                                                         int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Tunables::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Tunables::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_TUNABLES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TUNABLES];
    default: return 0;
    }
}

// CREATORS

Tunables::Tunables(bslma::Allocator* basicAllocator)
: d_tunables(basicAllocator)
{
}

Tunables::Tunables(const Tunables& original, bslma::Allocator* basicAllocator)
: d_tunables(original.d_tunables, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunables::Tunables(Tunables&& original) noexcept
: d_tunables(bsl::move(original.d_tunables))
{
}

Tunables::Tunables(Tunables&& original, bslma::Allocator* basicAllocator)
: d_tunables(bsl::move(original.d_tunables), basicAllocator)
{
}
#endif

Tunables::~Tunables()
{
}

// MANIPULATORS

Tunables& Tunables::operator=(const Tunables& rhs)
{
    if (this != &rhs) {
        d_tunables = rhs.d_tunables;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunables& Tunables::operator=(Tunables&& rhs)
{
    if (this != &rhs) {
        d_tunables = bsl::move(rhs.d_tunables);
    }

    return *this;
}
#endif

void Tunables::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_tunables);
}

// ACCESSORS

bsl::ostream&
Tunables::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("tunables", this->tunables());
    printer.end();
    return stream;
}

// -------------------------
// class ClusterStateCommand
// -------------------------

// CONSTANTS

const char ClusterStateCommand::CLASS_NAME[] = "ClusterStateCommand";

const bdlat_SelectionInfo ClusterStateCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ELECTOR,
     "elector",
     sizeof("elector") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ClusterStateCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ClusterStateCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ClusterStateCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ELECTOR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTOR];
    default: return 0;
    }
}

// CREATORS

ClusterStateCommand::ClusterStateCommand(const ClusterStateCommand& original,
                                         bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ELECTOR: {
        new (d_elector.buffer())
            ElectorCommand(original.d_elector.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStateCommand::ClusterStateCommand(ClusterStateCommand&& original)
    noexcept : d_selectionId(original.d_selectionId),
               d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ELECTOR: {
        new (d_elector.buffer())
            ElectorCommand(bsl::move(original.d_elector.object()),
                           d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ClusterStateCommand::ClusterStateCommand(ClusterStateCommand&& original,
                                         bslma::Allocator*     basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ELECTOR: {
        new (d_elector.buffer())
            ElectorCommand(bsl::move(original.d_elector.object()),
                           d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ClusterStateCommand&
ClusterStateCommand::operator=(const ClusterStateCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ELECTOR: {
            makeElector(rhs.d_elector.object());
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
ClusterStateCommand& ClusterStateCommand::operator=(ClusterStateCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ELECTOR: {
            makeElector(bsl::move(rhs.d_elector.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ClusterStateCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ELECTOR: {
        d_elector.object().~ElectorCommand();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ClusterStateCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ELECTOR: {
        makeElector();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ClusterStateCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

ElectorCommand& ClusterStateCommand::makeElector()
{
    if (SELECTION_ID_ELECTOR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_elector.object());
    }
    else {
        reset();
        new (d_elector.buffer()) ElectorCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_ELECTOR;
    }

    return d_elector.object();
}

ElectorCommand& ClusterStateCommand::makeElector(const ElectorCommand& value)
{
    if (SELECTION_ID_ELECTOR == d_selectionId) {
        d_elector.object() = value;
    }
    else {
        reset();
        new (d_elector.buffer()) ElectorCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ELECTOR;
    }

    return d_elector.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectorCommand& ClusterStateCommand::makeElector(ElectorCommand&& value)
{
    if (SELECTION_ID_ELECTOR == d_selectionId) {
        d_elector.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_elector.buffer())
            ElectorCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ELECTOR;
    }

    return d_elector.object();
}
#endif

// ACCESSORS

bsl::ostream& ClusterStateCommand::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ELECTOR: {
        printer.printAttribute("elector", d_elector.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ClusterStateCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ELECTOR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTOR].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ---------------------------
// class ClusterStorageSummary
// ---------------------------

// CONSTANTS

const char ClusterStorageSummary::CLASS_NAME[] = "ClusterStorageSummary";

const bdlat_AttributeInfo ClusterStorageSummary::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLUSTER_FILE_STORE_LOCATION,
     "clusterFileStoreLocation",
     sizeof("clusterFileStoreLocation") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_FILE_STORES,
     "fileStores",
     sizeof("fileStores") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterStorageSummary::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterStorageSummary::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterStorageSummary::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLUSTER_FILE_STORE_LOCATION:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_CLUSTER_FILE_STORE_LOCATION];
    case ATTRIBUTE_ID_FILE_STORES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_STORES];
    default: return 0;
    }
}

// CREATORS

ClusterStorageSummary::ClusterStorageSummary(bslma::Allocator* basicAllocator)
: d_fileStores(basicAllocator)
, d_clusterFileStoreLocation(basicAllocator)
{
}

ClusterStorageSummary::ClusterStorageSummary(
    const ClusterStorageSummary& original,
    bslma::Allocator*            basicAllocator)
: d_fileStores(original.d_fileStores, basicAllocator)
, d_clusterFileStoreLocation(original.d_clusterFileStoreLocation,
                             basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStorageSummary::ClusterStorageSummary(
    ClusterStorageSummary&& original) noexcept
: d_fileStores(bsl::move(original.d_fileStores)),
  d_clusterFileStoreLocation(bsl::move(original.d_clusterFileStoreLocation))
{
}

ClusterStorageSummary::ClusterStorageSummary(ClusterStorageSummary&& original,
                                             bslma::Allocator* basicAllocator)
: d_fileStores(bsl::move(original.d_fileStores), basicAllocator)
, d_clusterFileStoreLocation(bsl::move(original.d_clusterFileStoreLocation),
                             basicAllocator)
{
}
#endif

ClusterStorageSummary::~ClusterStorageSummary()
{
}

// MANIPULATORS

ClusterStorageSummary&
ClusterStorageSummary::operator=(const ClusterStorageSummary& rhs)
{
    if (this != &rhs) {
        d_clusterFileStoreLocation = rhs.d_clusterFileStoreLocation;
        d_fileStores               = rhs.d_fileStores;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStorageSummary&
ClusterStorageSummary::operator=(ClusterStorageSummary&& rhs)
{
    if (this != &rhs) {
        d_clusterFileStoreLocation = bsl::move(rhs.d_clusterFileStoreLocation);
        d_fileStores               = bsl::move(rhs.d_fileStores);
    }

    return *this;
}
#endif

void ClusterStorageSummary::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clusterFileStoreLocation);
    bdlat_ValueTypeFunctions::reset(&d_fileStores);
}

// ACCESSORS

bsl::ostream& ClusterStorageSummary::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clusterFileStoreLocation",
                           this->clusterFileStoreLocation());
    printer.printAttribute("fileStores", this->fileStores());
    printer.end();
    return stream;
}

// ------------
// class Domain
// ------------

// CONSTANTS

const char Domain::CLASS_NAME[] = "Domain";

const bdlat_AttributeInfo Domain::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_COMMAND,
     "command",
     sizeof("command") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Domain::lookupAttributeInfo(const char* name,
                                                       int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Domain::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Domain::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_COMMAND:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND];
    default: return 0;
    }
}

// CREATORS

Domain::Domain(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_command(basicAllocator)
{
}

Domain::Domain(const Domain& original, bslma::Allocator* basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_command(original.d_command, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Domain::Domain(Domain&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_command(bsl::move(original.d_command))
{
}

Domain::Domain(Domain&& original, bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_command(bsl::move(original.d_command), basicAllocator)
{
}
#endif

Domain::~Domain()
{
}

// MANIPULATORS

Domain& Domain::operator=(const Domain& rhs)
{
    if (this != &rhs) {
        d_name    = rhs.d_name;
        d_command = rhs.d_command;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Domain& Domain::operator=(Domain&& rhs)
{
    if (this != &rhs) {
        d_name    = bsl::move(rhs.d_name);
        d_command = bsl::move(rhs.d_command);
    }

    return *this;
}
#endif

void Domain::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_command);
}

// ACCESSORS

bsl::ostream&
Domain::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("command", this->command());
    printer.end();
    return stream;
}

// -------------------
// class ElectorResult
// -------------------

// CONSTANTS

const char ElectorResult::CLASS_NAME[] = "ElectorResult";

const bdlat_SelectionInfo ElectorResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLE,
     "tunable",
     sizeof("tunable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLES,
     "tunables",
     sizeof("tunables") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLE_CONFIRMATION,
     "tunableConfirmation",
     sizeof("tunableConfirmation") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* ElectorResult::lookupSelectionInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ElectorResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ElectorResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE];
    case SELECTION_ID_TUNABLES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLES];
    case SELECTION_ID_TUNABLE_CONFIRMATION:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE_CONFIRMATION];
    default: return 0;
    }
}

// CREATORS

ElectorResult::ElectorResult(const ElectorResult& original,
                             bslma::Allocator*    basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(original.d_tunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(original.d_tunables.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(original.d_tunableConfirmation.object(),
                                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectorResult::ElectorResult(ElectorResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(bsl::move(original.d_tunable.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(bsl::move(original.d_tunables.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer()) TunableConfirmation(
            bsl::move(original.d_tunableConfirmation.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ElectorResult::ElectorResult(ElectorResult&&   original,
                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(bsl::move(original.d_tunable.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(bsl::move(original.d_tunables.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer()) TunableConfirmation(
            bsl::move(original.d_tunableConfirmation.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ElectorResult& ElectorResult::operator=(const ElectorResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_TUNABLE: {
            makeTunable(rhs.d_tunable.object());
        } break;
        case SELECTION_ID_TUNABLES: {
            makeTunables(rhs.d_tunables.object());
        } break;
        case SELECTION_ID_TUNABLE_CONFIRMATION: {
            makeTunableConfirmation(rhs.d_tunableConfirmation.object());
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
ElectorResult& ElectorResult::operator=(ElectorResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_TUNABLE: {
            makeTunable(bsl::move(rhs.d_tunable.object()));
        } break;
        case SELECTION_ID_TUNABLES: {
            makeTunables(bsl::move(rhs.d_tunables.object()));
        } break;
        case SELECTION_ID_TUNABLE_CONFIRMATION: {
            makeTunableConfirmation(
                bsl::move(rhs.d_tunableConfirmation.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ElectorResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_TUNABLE: {
        d_tunable.object().~Tunable();
    } break;
    case SELECTION_ID_TUNABLES: {
        d_tunables.object().~Tunables();
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        d_tunableConfirmation.object().~TunableConfirmation();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ElectorResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_TUNABLE: {
        makeTunable();
    } break;
    case SELECTION_ID_TUNABLES: {
        makeTunables();
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        makeTunableConfirmation();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ElectorResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Error& ElectorResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& ElectorResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& ElectorResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

Tunable& ElectorResult::makeTunable()
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunable.object());
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}

Tunable& ElectorResult::makeTunable(const Tunable& value)
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        d_tunable.object() = value;
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunable& ElectorResult::makeTunable(Tunable&& value)
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        d_tunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}
#endif

Tunables& ElectorResult::makeTunables()
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunables.object());
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}

Tunables& ElectorResult::makeTunables(const Tunables& value)
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        d_tunables.object() = value;
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunables& ElectorResult::makeTunables(Tunables&& value)
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        d_tunables.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}
#endif

TunableConfirmation& ElectorResult::makeTunableConfirmation()
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunableConfirmation.object());
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}

TunableConfirmation&
ElectorResult::makeTunableConfirmation(const TunableConfirmation& value)
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        d_tunableConfirmation.object() = value;
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TunableConfirmation&
ElectorResult::makeTunableConfirmation(TunableConfirmation&& value)
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        d_tunableConfirmation.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}
#endif

// ACCESSORS

bsl::ostream&
ElectorResult::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_TUNABLE: {
        printer.printAttribute("tunable", d_tunable.object());
    } break;
    case SELECTION_ID_TUNABLES: {
        printer.printAttribute("tunables", d_tunables.object());
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        printer.printAttribute("tunableConfirmation",
                               d_tunableConfirmation.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ElectorResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE].name();
    case SELECTION_ID_TUNABLES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLES].name();
    case SELECTION_ID_TUNABLE_CONFIRMATION:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE_CONFIRMATION]
            .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------
// class QueueState
// ----------------

// CONSTANTS

const char QueueState::CLASS_NAME[] = "QueueState";

const bdlat_AttributeInfo QueueState::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_HANDLE_PARAMETERS_JSON,
     "handleParametersJson",
     sizeof("handleParametersJson") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_STREAM_PARAMETERS_JSON,
     "streamParametersJson",
     sizeof("streamParametersJson") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ID, "id", sizeof("id") - 1, "", bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_KEY,
     "key",
     sizeof("key") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PARTITION_ID,
     "partitionId",
     sizeof("partitionId") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STORAGE,
     "storage",
     sizeof("storage") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CAPACITY_METER,
     "capacityMeter",
     sizeof("capacityMeter") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_HANDLES,
     "handles",
     sizeof("handles") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* QueueState::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueState::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueState::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    case ATTRIBUTE_ID_HANDLE_PARAMETERS_JSON:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HANDLE_PARAMETERS_JSON];
    case ATTRIBUTE_ID_STREAM_PARAMETERS_JSON:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STREAM_PARAMETERS_JSON];
    case ATTRIBUTE_ID_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID];
    case ATTRIBUTE_ID_KEY: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY];
    case ATTRIBUTE_ID_PARTITION_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_ID];
    case ATTRIBUTE_ID_STORAGE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE];
    case ATTRIBUTE_ID_CAPACITY_METER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CAPACITY_METER];
    case ATTRIBUTE_ID_HANDLES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HANDLES];
    default: return 0;
    }
}

// CREATORS

QueueState::QueueState(bslma::Allocator* basicAllocator)
: d_handles(basicAllocator)
, d_uri(basicAllocator)
, d_handleParametersJson(basicAllocator)
, d_streamParametersJson(basicAllocator)
, d_key(basicAllocator)
, d_storage(basicAllocator)
, d_capacityMeter(basicAllocator)
, d_id()
, d_partitionId()
{
}

QueueState::QueueState(const QueueState& original,
                       bslma::Allocator* basicAllocator)
: d_handles(original.d_handles, basicAllocator)
, d_uri(original.d_uri, basicAllocator)
, d_handleParametersJson(original.d_handleParametersJson, basicAllocator)
, d_streamParametersJson(original.d_streamParametersJson, basicAllocator)
, d_key(original.d_key, basicAllocator)
, d_storage(original.d_storage, basicAllocator)
, d_capacityMeter(original.d_capacityMeter, basicAllocator)
, d_id(original.d_id)
, d_partitionId(original.d_partitionId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueState::QueueState(QueueState&& original) noexcept
: d_handles(bsl::move(original.d_handles)),
  d_uri(bsl::move(original.d_uri)),
  d_handleParametersJson(bsl::move(original.d_handleParametersJson)),
  d_streamParametersJson(bsl::move(original.d_streamParametersJson)),
  d_key(bsl::move(original.d_key)),
  d_storage(bsl::move(original.d_storage)),
  d_capacityMeter(bsl::move(original.d_capacityMeter)),
  d_id(bsl::move(original.d_id)),
  d_partitionId(bsl::move(original.d_partitionId))
{
}

QueueState::QueueState(QueueState&& original, bslma::Allocator* basicAllocator)
: d_handles(bsl::move(original.d_handles), basicAllocator)
, d_uri(bsl::move(original.d_uri), basicAllocator)
, d_handleParametersJson(bsl::move(original.d_handleParametersJson),
                         basicAllocator)
, d_streamParametersJson(bsl::move(original.d_streamParametersJson),
                         basicAllocator)
, d_key(bsl::move(original.d_key), basicAllocator)
, d_storage(bsl::move(original.d_storage), basicAllocator)
, d_capacityMeter(bsl::move(original.d_capacityMeter), basicAllocator)
, d_id(bsl::move(original.d_id))
, d_partitionId(bsl::move(original.d_partitionId))
{
}
#endif

QueueState::~QueueState()
{
}

// MANIPULATORS

QueueState& QueueState::operator=(const QueueState& rhs)
{
    if (this != &rhs) {
        d_uri                  = rhs.d_uri;
        d_handleParametersJson = rhs.d_handleParametersJson;
        d_streamParametersJson = rhs.d_streamParametersJson;
        d_id                   = rhs.d_id;
        d_key                  = rhs.d_key;
        d_partitionId          = rhs.d_partitionId;
        d_storage              = rhs.d_storage;
        d_capacityMeter        = rhs.d_capacityMeter;
        d_handles              = rhs.d_handles;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueState& QueueState::operator=(QueueState&& rhs)
{
    if (this != &rhs) {
        d_uri                  = bsl::move(rhs.d_uri);
        d_handleParametersJson = bsl::move(rhs.d_handleParametersJson);
        d_streamParametersJson = bsl::move(rhs.d_streamParametersJson);
        d_id                   = bsl::move(rhs.d_id);
        d_key                  = bsl::move(rhs.d_key);
        d_partitionId          = bsl::move(rhs.d_partitionId);
        d_storage              = bsl::move(rhs.d_storage);
        d_capacityMeter        = bsl::move(rhs.d_capacityMeter);
        d_handles              = bsl::move(rhs.d_handles);
    }

    return *this;
}
#endif

void QueueState::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
    bdlat_ValueTypeFunctions::reset(&d_handleParametersJson);
    bdlat_ValueTypeFunctions::reset(&d_streamParametersJson);
    bdlat_ValueTypeFunctions::reset(&d_id);
    bdlat_ValueTypeFunctions::reset(&d_key);
    bdlat_ValueTypeFunctions::reset(&d_partitionId);
    bdlat_ValueTypeFunctions::reset(&d_storage);
    bdlat_ValueTypeFunctions::reset(&d_capacityMeter);
    bdlat_ValueTypeFunctions::reset(&d_handles);
}

// ACCESSORS

bsl::ostream&
QueueState::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.printAttribute("handleParametersJson",
                           this->handleParametersJson());
    printer.printAttribute("streamParametersJson",
                           this->streamParametersJson());
    printer.printAttribute("id", this->id());
    printer.printAttribute("key", this->key());
    printer.printAttribute("partitionId", this->partitionId());
    printer.printAttribute("storage", this->storage());
    printer.printAttribute("capacityMeter", this->capacityMeter());
    printer.printAttribute("handles", this->handles());
    printer.end();
    return stream;
}

// -----------------------
// class ReplicationResult
// -----------------------

// CONSTANTS

const char ReplicationResult::CLASS_NAME[] = "ReplicationResult";

const bdlat_SelectionInfo ReplicationResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLE,
     "tunable",
     sizeof("tunable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLES,
     "tunables",
     sizeof("tunables") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLE_CONFIRMATION,
     "tunableConfirmation",
     sizeof("tunableConfirmation") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ReplicationResult::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ReplicationResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ReplicationResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE];
    case SELECTION_ID_TUNABLES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLES];
    case SELECTION_ID_TUNABLE_CONFIRMATION:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE_CONFIRMATION];
    default: return 0;
    }
}

// CREATORS

ReplicationResult::ReplicationResult(const ReplicationResult& original,
                                     bslma::Allocator*        basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(original.d_tunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(original.d_tunables.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(original.d_tunableConfirmation.object(),
                                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReplicationResult::ReplicationResult(ReplicationResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(bsl::move(original.d_tunable.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(bsl::move(original.d_tunables.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer()) TunableConfirmation(
            bsl::move(original.d_tunableConfirmation.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ReplicationResult::ReplicationResult(ReplicationResult&& original,
                                     bslma::Allocator*   basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(bsl::move(original.d_tunable.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(bsl::move(original.d_tunables.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer()) TunableConfirmation(
            bsl::move(original.d_tunableConfirmation.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ReplicationResult& ReplicationResult::operator=(const ReplicationResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_TUNABLE: {
            makeTunable(rhs.d_tunable.object());
        } break;
        case SELECTION_ID_TUNABLES: {
            makeTunables(rhs.d_tunables.object());
        } break;
        case SELECTION_ID_TUNABLE_CONFIRMATION: {
            makeTunableConfirmation(rhs.d_tunableConfirmation.object());
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
ReplicationResult& ReplicationResult::operator=(ReplicationResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_TUNABLE: {
            makeTunable(bsl::move(rhs.d_tunable.object()));
        } break;
        case SELECTION_ID_TUNABLES: {
            makeTunables(bsl::move(rhs.d_tunables.object()));
        } break;
        case SELECTION_ID_TUNABLE_CONFIRMATION: {
            makeTunableConfirmation(
                bsl::move(rhs.d_tunableConfirmation.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ReplicationResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_TUNABLE: {
        d_tunable.object().~Tunable();
    } break;
    case SELECTION_ID_TUNABLES: {
        d_tunables.object().~Tunables();
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        d_tunableConfirmation.object().~TunableConfirmation();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ReplicationResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_TUNABLE: {
        makeTunable();
    } break;
    case SELECTION_ID_TUNABLES: {
        makeTunables();
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        makeTunableConfirmation();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ReplicationResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Error& ReplicationResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& ReplicationResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& ReplicationResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

Tunable& ReplicationResult::makeTunable()
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunable.object());
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}

Tunable& ReplicationResult::makeTunable(const Tunable& value)
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        d_tunable.object() = value;
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunable& ReplicationResult::makeTunable(Tunable&& value)
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        d_tunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}
#endif

Tunables& ReplicationResult::makeTunables()
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunables.object());
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}

Tunables& ReplicationResult::makeTunables(const Tunables& value)
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        d_tunables.object() = value;
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunables& ReplicationResult::makeTunables(Tunables&& value)
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        d_tunables.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}
#endif

TunableConfirmation& ReplicationResult::makeTunableConfirmation()
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunableConfirmation.object());
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}

TunableConfirmation&
ReplicationResult::makeTunableConfirmation(const TunableConfirmation& value)
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        d_tunableConfirmation.object() = value;
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TunableConfirmation&
ReplicationResult::makeTunableConfirmation(TunableConfirmation&& value)
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        d_tunableConfirmation.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}
#endif

// ACCESSORS

bsl::ostream& ReplicationResult::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_TUNABLE: {
        printer.printAttribute("tunable", d_tunable.object());
    } break;
    case SELECTION_ID_TUNABLES: {
        printer.printAttribute("tunables", d_tunables.object());
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        printer.printAttribute("tunableConfirmation",
                               d_tunableConfirmation.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ReplicationResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE].name();
    case SELECTION_ID_TUNABLES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLES].name();
    case SELECTION_ID_TUNABLE_CONFIRMATION:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE_CONFIRMATION]
            .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------------
// class RouterConsumer
// --------------------

// CONSTANTS

const char RouterConsumer::CLASS_NAME[] = "RouterConsumer";

const bdlat_AttributeInfo RouterConsumer::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PRIORITY,
     "priority",
     sizeof("priority") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_QUEUE_HANDLE,
     "queueHandle",
     sizeof("queueHandle") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_COUNT,
     "count",
     sizeof("count") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_EXPRESSION,
     "expression",
     sizeof("expression") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
RouterConsumer::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RouterConsumer::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RouterConsumer::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PRIORITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRIORITY];
    case ATTRIBUTE_ID_QUEUE_HANDLE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_HANDLE];
    case ATTRIBUTE_ID_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COUNT];
    case ATTRIBUTE_ID_EXPRESSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION];
    default: return 0;
    }
}

// CREATORS

RouterConsumer::RouterConsumer(bslma::Allocator* basicAllocator)
: d_expression(basicAllocator)
, d_queueHandle(basicAllocator)
, d_count()
, d_priority()
{
}

RouterConsumer::RouterConsumer(const RouterConsumer& original,
                               bslma::Allocator*     basicAllocator)
: d_expression(original.d_expression, basicAllocator)
, d_queueHandle(original.d_queueHandle, basicAllocator)
, d_count(original.d_count)
, d_priority(original.d_priority)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouterConsumer::RouterConsumer(RouterConsumer&& original) noexcept
: d_expression(bsl::move(original.d_expression)),
  d_queueHandle(bsl::move(original.d_queueHandle)),
  d_count(bsl::move(original.d_count)),
  d_priority(bsl::move(original.d_priority))
{
}

RouterConsumer::RouterConsumer(RouterConsumer&&  original,
                               bslma::Allocator* basicAllocator)
: d_expression(bsl::move(original.d_expression), basicAllocator)
, d_queueHandle(bsl::move(original.d_queueHandle), basicAllocator)
, d_count(bsl::move(original.d_count))
, d_priority(bsl::move(original.d_priority))
{
}
#endif

RouterConsumer::~RouterConsumer()
{
}

// MANIPULATORS

RouterConsumer& RouterConsumer::operator=(const RouterConsumer& rhs)
{
    if (this != &rhs) {
        d_priority    = rhs.d_priority;
        d_queueHandle = rhs.d_queueHandle;
        d_count       = rhs.d_count;
        d_expression  = rhs.d_expression;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouterConsumer& RouterConsumer::operator=(RouterConsumer&& rhs)
{
    if (this != &rhs) {
        d_priority    = bsl::move(rhs.d_priority);
        d_queueHandle = bsl::move(rhs.d_queueHandle);
        d_count       = bsl::move(rhs.d_count);
        d_expression  = bsl::move(rhs.d_expression);
    }

    return *this;
}
#endif

void RouterConsumer::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_priority);
    bdlat_ValueTypeFunctions::reset(&d_queueHandle);
    bdlat_ValueTypeFunctions::reset(&d_count);
    bdlat_ValueTypeFunctions::reset(&d_expression);
}

// ACCESSORS

bsl::ostream& RouterConsumer::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("priority", this->priority());
    printer.printAttribute("queueHandle", this->queueHandle());
    printer.printAttribute("count", this->count());
    printer.printAttribute("expression", this->expression());
    printer.end();
    return stream;
}

// -------------
// class Routing
// -------------

// CONSTANTS

const char Routing::CLASS_NAME[] = "Routing";

const bdlat_AttributeInfo Routing::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SUBSCRIPTION_GROUPS,
     "subscriptionGroups",
     sizeof("subscriptionGroups") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Routing::lookupAttributeInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Routing::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Routing::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SUBSCRIPTION_GROUPS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTION_GROUPS];
    default: return 0;
    }
}

// CREATORS

Routing::Routing(bslma::Allocator* basicAllocator)
: d_subscriptionGroups(basicAllocator)
{
}

Routing::Routing(const Routing& original, bslma::Allocator* basicAllocator)
: d_subscriptionGroups(original.d_subscriptionGroups, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Routing::Routing(Routing&& original) noexcept
: d_subscriptionGroups(bsl::move(original.d_subscriptionGroups))
{
}

Routing::Routing(Routing&& original, bslma::Allocator* basicAllocator)
: d_subscriptionGroups(bsl::move(original.d_subscriptionGroups),
                       basicAllocator)
{
}
#endif

Routing::~Routing()
{
}

// MANIPULATORS

Routing& Routing::operator=(const Routing& rhs)
{
    if (this != &rhs) {
        d_subscriptionGroups = rhs.d_subscriptionGroups;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Routing& Routing::operator=(Routing&& rhs)
{
    if (this != &rhs) {
        d_subscriptionGroups = bsl::move(rhs.d_subscriptionGroups);
    }

    return *this;
}
#endif

void Routing::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_subscriptionGroups);
}

// ACCESSORS

bsl::ostream&
Routing::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("subscriptionGroups", this->subscriptionGroups());
    printer.end();
    return stream;
}

// ----------------
// class StatResult
// ----------------

// CONSTANTS

const char StatResult::CLASS_NAME[] = "StatResult";

const bdlat_SelectionInfo StatResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STATS,
     "stats",
     sizeof("stats") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {SELECTION_ID_TUNABLE,
     "tunable",
     sizeof("tunable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLES,
     "tunables",
     sizeof("tunables") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLE_CONFIRMATION,
     "tunableConfirmation",
     sizeof("tunableConfirmation") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* StatResult::lookupSelectionInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            StatResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* StatResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_STATS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STATS];
    case SELECTION_ID_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE];
    case SELECTION_ID_TUNABLES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLES];
    case SELECTION_ID_TUNABLE_CONFIRMATION:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE_CONFIRMATION];
    default: return 0;
    }
}

// CREATORS

StatResult::StatResult(const StatResult& original,
                       bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_STATS: {
        new (d_stats.buffer())
            bsl::string(original.d_stats.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(original.d_tunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(original.d_tunables.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(original.d_tunableConfirmation.object(),
                                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatResult::StatResult(StatResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_STATS: {
        new (d_stats.buffer())
            bsl::string(bsl::move(original.d_stats.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(bsl::move(original.d_tunable.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(bsl::move(original.d_tunables.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer()) TunableConfirmation(
            bsl::move(original.d_tunableConfirmation.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

StatResult::StatResult(StatResult&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_STATS: {
        new (d_stats.buffer())
            bsl::string(bsl::move(original.d_stats.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(bsl::move(original.d_tunable.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(bsl::move(original.d_tunables.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer()) TunableConfirmation(
            bsl::move(original.d_tunableConfirmation.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

StatResult& StatResult::operator=(const StatResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_STATS: {
            makeStats(rhs.d_stats.object());
        } break;
        case SELECTION_ID_TUNABLE: {
            makeTunable(rhs.d_tunable.object());
        } break;
        case SELECTION_ID_TUNABLES: {
            makeTunables(rhs.d_tunables.object());
        } break;
        case SELECTION_ID_TUNABLE_CONFIRMATION: {
            makeTunableConfirmation(rhs.d_tunableConfirmation.object());
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
StatResult& StatResult::operator=(StatResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_STATS: {
            makeStats(bsl::move(rhs.d_stats.object()));
        } break;
        case SELECTION_ID_TUNABLE: {
            makeTunable(bsl::move(rhs.d_tunable.object()));
        } break;
        case SELECTION_ID_TUNABLES: {
            makeTunables(bsl::move(rhs.d_tunables.object()));
        } break;
        case SELECTION_ID_TUNABLE_CONFIRMATION: {
            makeTunableConfirmation(
                bsl::move(rhs.d_tunableConfirmation.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void StatResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_STATS: {
        typedef bsl::string Type;
        d_stats.object().~Type();
    } break;
    case SELECTION_ID_TUNABLE: {
        d_tunable.object().~Tunable();
    } break;
    case SELECTION_ID_TUNABLES: {
        d_tunables.object().~Tunables();
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        d_tunableConfirmation.object().~TunableConfirmation();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int StatResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_STATS: {
        makeStats();
    } break;
    case SELECTION_ID_TUNABLE: {
        makeTunable();
    } break;
    case SELECTION_ID_TUNABLES: {
        makeTunables();
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        makeTunableConfirmation();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int StatResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Error& StatResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& StatResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& StatResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

bsl::string& StatResult::makeStats()
{
    if (SELECTION_ID_STATS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_stats.object());
    }
    else {
        reset();
        new (d_stats.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_STATS;
    }

    return d_stats.object();
}

bsl::string& StatResult::makeStats(const bsl::string& value)
{
    if (SELECTION_ID_STATS == d_selectionId) {
        d_stats.object() = value;
    }
    else {
        reset();
        new (d_stats.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STATS;
    }

    return d_stats.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& StatResult::makeStats(bsl::string&& value)
{
    if (SELECTION_ID_STATS == d_selectionId) {
        d_stats.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_stats.buffer()) bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STATS;
    }

    return d_stats.object();
}
#endif

Tunable& StatResult::makeTunable()
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunable.object());
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}

Tunable& StatResult::makeTunable(const Tunable& value)
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        d_tunable.object() = value;
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunable& StatResult::makeTunable(Tunable&& value)
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        d_tunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}
#endif

Tunables& StatResult::makeTunables()
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunables.object());
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}

Tunables& StatResult::makeTunables(const Tunables& value)
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        d_tunables.object() = value;
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunables& StatResult::makeTunables(Tunables&& value)
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        d_tunables.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}
#endif

TunableConfirmation& StatResult::makeTunableConfirmation()
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunableConfirmation.object());
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}

TunableConfirmation&
StatResult::makeTunableConfirmation(const TunableConfirmation& value)
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        d_tunableConfirmation.object() = value;
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TunableConfirmation&
StatResult::makeTunableConfirmation(TunableConfirmation&& value)
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        d_tunableConfirmation.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}
#endif

// ACCESSORS

bsl::ostream&
StatResult::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_STATS: {
        printer.printAttribute("stats", d_stats.object());
    } break;
    case SELECTION_ID_TUNABLE: {
        printer.printAttribute("tunable", d_tunable.object());
    } break;
    case SELECTION_ID_TUNABLES: {
        printer.printAttribute("tunables", d_tunables.object());
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        printer.printAttribute("tunableConfirmation",
                               d_tunableConfirmation.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* StatResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_STATS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STATS].name();
    case SELECTION_ID_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE].name();
    case SELECTION_ID_TUNABLES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLES].name();
    case SELECTION_ID_TUNABLE_CONFIRMATION:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE_CONFIRMATION]
            .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------------
// class StorageCommand
// --------------------

// CONSTANTS

const char StorageCommand::CLASS_NAME[] = "StorageCommand";

const bdlat_SelectionInfo StorageCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_SUMMARY,
     "summary",
     sizeof("summary") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PARTITION,
     "partition",
     sizeof("partition") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DOMAIN,
     "domain",
     sizeof("domain") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE,
     "queue",
     sizeof("queue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_REPLICATION,
     "replication",
     sizeof("replication") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
StorageCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            StorageCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* StorageCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_SUMMARY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SUMMARY];
    case SELECTION_ID_PARTITION:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PARTITION];
    case SELECTION_ID_DOMAIN:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN];
    case SELECTION_ID_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE];
    case SELECTION_ID_REPLICATION:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICATION];
    default: return 0;
    }
}

// CREATORS

StorageCommand::StorageCommand(const StorageCommand& original,
                               bslma::Allocator*     basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SUMMARY: {
        new (d_summary.buffer()) Void(original.d_summary.object());
    } break;
    case SELECTION_ID_PARTITION: {
        new (d_partition.buffer())
            StoragePartition(original.d_partition.object());
    } break;
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            StorageDomain(original.d_domain.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE: {
        new (d_queue.buffer())
            StorageQueue(original.d_queue.object(), d_allocator_p);
    } break;
    case SELECTION_ID_REPLICATION: {
        new (d_replication.buffer())
            ReplicationCommand(original.d_replication.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageCommand::StorageCommand(StorageCommand&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_SUMMARY: {
        new (d_summary.buffer()) Void(bsl::move(original.d_summary.object()));
    } break;
    case SELECTION_ID_PARTITION: {
        new (d_partition.buffer())
            StoragePartition(bsl::move(original.d_partition.object()));
    } break;
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            StorageDomain(bsl::move(original.d_domain.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE: {
        new (d_queue.buffer())
            StorageQueue(bsl::move(original.d_queue.object()), d_allocator_p);
    } break;
    case SELECTION_ID_REPLICATION: {
        new (d_replication.buffer())
            ReplicationCommand(bsl::move(original.d_replication.object()),
                               d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

StorageCommand::StorageCommand(StorageCommand&&  original,
                               bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SUMMARY: {
        new (d_summary.buffer()) Void(bsl::move(original.d_summary.object()));
    } break;
    case SELECTION_ID_PARTITION: {
        new (d_partition.buffer())
            StoragePartition(bsl::move(original.d_partition.object()));
    } break;
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            StorageDomain(bsl::move(original.d_domain.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE: {
        new (d_queue.buffer())
            StorageQueue(bsl::move(original.d_queue.object()), d_allocator_p);
    } break;
    case SELECTION_ID_REPLICATION: {
        new (d_replication.buffer())
            ReplicationCommand(bsl::move(original.d_replication.object()),
                               d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

StorageCommand& StorageCommand::operator=(const StorageCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SUMMARY: {
            makeSummary(rhs.d_summary.object());
        } break;
        case SELECTION_ID_PARTITION: {
            makePartition(rhs.d_partition.object());
        } break;
        case SELECTION_ID_DOMAIN: {
            makeDomain(rhs.d_domain.object());
        } break;
        case SELECTION_ID_QUEUE: {
            makeQueue(rhs.d_queue.object());
        } break;
        case SELECTION_ID_REPLICATION: {
            makeReplication(rhs.d_replication.object());
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
StorageCommand& StorageCommand::operator=(StorageCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SUMMARY: {
            makeSummary(bsl::move(rhs.d_summary.object()));
        } break;
        case SELECTION_ID_PARTITION: {
            makePartition(bsl::move(rhs.d_partition.object()));
        } break;
        case SELECTION_ID_DOMAIN: {
            makeDomain(bsl::move(rhs.d_domain.object()));
        } break;
        case SELECTION_ID_QUEUE: {
            makeQueue(bsl::move(rhs.d_queue.object()));
        } break;
        case SELECTION_ID_REPLICATION: {
            makeReplication(bsl::move(rhs.d_replication.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void StorageCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_SUMMARY: {
        d_summary.object().~Void();
    } break;
    case SELECTION_ID_PARTITION: {
        d_partition.object().~StoragePartition();
    } break;
    case SELECTION_ID_DOMAIN: {
        d_domain.object().~StorageDomain();
    } break;
    case SELECTION_ID_QUEUE: {
        d_queue.object().~StorageQueue();
    } break;
    case SELECTION_ID_REPLICATION: {
        d_replication.object().~ReplicationCommand();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int StorageCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_SUMMARY: {
        makeSummary();
    } break;
    case SELECTION_ID_PARTITION: {
        makePartition();
    } break;
    case SELECTION_ID_DOMAIN: {
        makeDomain();
    } break;
    case SELECTION_ID_QUEUE: {
        makeQueue();
    } break;
    case SELECTION_ID_REPLICATION: {
        makeReplication();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int StorageCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& StorageCommand::makeSummary()
{
    if (SELECTION_ID_SUMMARY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_summary.object());
    }
    else {
        reset();
        new (d_summary.buffer()) Void();
        d_selectionId = SELECTION_ID_SUMMARY;
    }

    return d_summary.object();
}

Void& StorageCommand::makeSummary(const Void& value)
{
    if (SELECTION_ID_SUMMARY == d_selectionId) {
        d_summary.object() = value;
    }
    else {
        reset();
        new (d_summary.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SUMMARY;
    }

    return d_summary.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& StorageCommand::makeSummary(Void&& value)
{
    if (SELECTION_ID_SUMMARY == d_selectionId) {
        d_summary.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_summary.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SUMMARY;
    }

    return d_summary.object();
}
#endif

StoragePartition& StorageCommand::makePartition()
{
    if (SELECTION_ID_PARTITION == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_partition.object());
    }
    else {
        reset();
        new (d_partition.buffer()) StoragePartition();
        d_selectionId = SELECTION_ID_PARTITION;
    }

    return d_partition.object();
}

StoragePartition& StorageCommand::makePartition(const StoragePartition& value)
{
    if (SELECTION_ID_PARTITION == d_selectionId) {
        d_partition.object() = value;
    }
    else {
        reset();
        new (d_partition.buffer()) StoragePartition(value);
        d_selectionId = SELECTION_ID_PARTITION;
    }

    return d_partition.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StoragePartition& StorageCommand::makePartition(StoragePartition&& value)
{
    if (SELECTION_ID_PARTITION == d_selectionId) {
        d_partition.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_partition.buffer()) StoragePartition(bsl::move(value));
        d_selectionId = SELECTION_ID_PARTITION;
    }

    return d_partition.object();
}
#endif

StorageDomain& StorageCommand::makeDomain()
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domain.object());
    }
    else {
        reset();
        new (d_domain.buffer()) StorageDomain(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}

StorageDomain& StorageCommand::makeDomain(const StorageDomain& value)
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        d_domain.object() = value;
    }
    else {
        reset();
        new (d_domain.buffer()) StorageDomain(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageDomain& StorageCommand::makeDomain(StorageDomain&& value)
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        d_domain.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domain.buffer()) StorageDomain(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}
#endif

StorageQueue& StorageCommand::makeQueue()
{
    if (SELECTION_ID_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queue.object());
    }
    else {
        reset();
        new (d_queue.buffer()) StorageQueue(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE;
    }

    return d_queue.object();
}

StorageQueue& StorageCommand::makeQueue(const StorageQueue& value)
{
    if (SELECTION_ID_QUEUE == d_selectionId) {
        d_queue.object() = value;
    }
    else {
        reset();
        new (d_queue.buffer()) StorageQueue(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE;
    }

    return d_queue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageQueue& StorageCommand::makeQueue(StorageQueue&& value)
{
    if (SELECTION_ID_QUEUE == d_selectionId) {
        d_queue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queue.buffer()) StorageQueue(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE;
    }

    return d_queue.object();
}
#endif

ReplicationCommand& StorageCommand::makeReplication()
{
    if (SELECTION_ID_REPLICATION == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_replication.object());
    }
    else {
        reset();
        new (d_replication.buffer()) ReplicationCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_REPLICATION;
    }

    return d_replication.object();
}

ReplicationCommand&
StorageCommand::makeReplication(const ReplicationCommand& value)
{
    if (SELECTION_ID_REPLICATION == d_selectionId) {
        d_replication.object() = value;
    }
    else {
        reset();
        new (d_replication.buffer()) ReplicationCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_REPLICATION;
    }

    return d_replication.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReplicationCommand& StorageCommand::makeReplication(ReplicationCommand&& value)
{
    if (SELECTION_ID_REPLICATION == d_selectionId) {
        d_replication.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_replication.buffer())
            ReplicationCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_REPLICATION;
    }

    return d_replication.object();
}
#endif

// ACCESSORS

bsl::ostream& StorageCommand::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_SUMMARY: {
        printer.printAttribute("summary", d_summary.object());
    } break;
    case SELECTION_ID_PARTITION: {
        printer.printAttribute("partition", d_partition.object());
    } break;
    case SELECTION_ID_DOMAIN: {
        printer.printAttribute("domain", d_domain.object());
    } break;
    case SELECTION_ID_QUEUE: {
        printer.printAttribute("queue", d_queue.object());
    } break;
    case SELECTION_ID_REPLICATION: {
        printer.printAttribute("replication", d_replication.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* StorageCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_SUMMARY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SUMMARY].name();
    case SELECTION_ID_PARTITION:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PARTITION].name();
    case SELECTION_ID_DOMAIN:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN].name();
    case SELECTION_ID_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE].name();
    case SELECTION_ID_REPLICATION:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICATION].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------------
// class ClusterCommand
// --------------------

// CONSTANTS

const char ClusterCommand::CLASS_NAME[] = "ClusterCommand";

const bdlat_SelectionInfo ClusterCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_STATUS,
     "status",
     sizeof("status") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_HELPER,
     "queueHelper",
     sizeof("queueHelper") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_FORCE_GC_QUEUES,
     "forceGcQueues",
     sizeof("forceGcQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STORAGE,
     "storage",
     sizeof("storage") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STATE,
     "state",
     sizeof("state") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ClusterCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ClusterCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ClusterCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_STATUS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STATUS];
    case SELECTION_ID_QUEUE_HELPER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_HELPER];
    case SELECTION_ID_FORCE_GC_QUEUES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_FORCE_GC_QUEUES];
    case SELECTION_ID_STORAGE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE];
    case SELECTION_ID_STATE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STATE];
    default: return 0;
    }
}

// CREATORS

ClusterCommand::ClusterCommand(const ClusterCommand& original,
                               bslma::Allocator*     basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        new (d_status.buffer()) Void(original.d_status.object());
    } break;
    case SELECTION_ID_QUEUE_HELPER: {
        new (d_queueHelper.buffer()) Void(original.d_queueHelper.object());
    } break;
    case SELECTION_ID_FORCE_GC_QUEUES: {
        new (d_forceGcQueues.buffer()) Void(original.d_forceGcQueues.object());
    } break;
    case SELECTION_ID_STORAGE: {
        new (d_storage.buffer())
            StorageCommand(original.d_storage.object(), d_allocator_p);
    } break;
    case SELECTION_ID_STATE: {
        new (d_state.buffer())
            ClusterStateCommand(original.d_state.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterCommand::ClusterCommand(ClusterCommand&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        new (d_status.buffer()) Void(bsl::move(original.d_status.object()));
    } break;
    case SELECTION_ID_QUEUE_HELPER: {
        new (d_queueHelper.buffer())
            Void(bsl::move(original.d_queueHelper.object()));
    } break;
    case SELECTION_ID_FORCE_GC_QUEUES: {
        new (d_forceGcQueues.buffer())
            Void(bsl::move(original.d_forceGcQueues.object()));
    } break;
    case SELECTION_ID_STORAGE: {
        new (d_storage.buffer())
            StorageCommand(bsl::move(original.d_storage.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_STATE: {
        new (d_state.buffer())
            ClusterStateCommand(bsl::move(original.d_state.object()),
                                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ClusterCommand::ClusterCommand(ClusterCommand&&  original,
                               bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        new (d_status.buffer()) Void(bsl::move(original.d_status.object()));
    } break;
    case SELECTION_ID_QUEUE_HELPER: {
        new (d_queueHelper.buffer())
            Void(bsl::move(original.d_queueHelper.object()));
    } break;
    case SELECTION_ID_FORCE_GC_QUEUES: {
        new (d_forceGcQueues.buffer())
            Void(bsl::move(original.d_forceGcQueues.object()));
    } break;
    case SELECTION_ID_STORAGE: {
        new (d_storage.buffer())
            StorageCommand(bsl::move(original.d_storage.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_STATE: {
        new (d_state.buffer())
            ClusterStateCommand(bsl::move(original.d_state.object()),
                                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ClusterCommand& ClusterCommand::operator=(const ClusterCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_STATUS: {
            makeStatus(rhs.d_status.object());
        } break;
        case SELECTION_ID_QUEUE_HELPER: {
            makeQueueHelper(rhs.d_queueHelper.object());
        } break;
        case SELECTION_ID_FORCE_GC_QUEUES: {
            makeForceGcQueues(rhs.d_forceGcQueues.object());
        } break;
        case SELECTION_ID_STORAGE: {
            makeStorage(rhs.d_storage.object());
        } break;
        case SELECTION_ID_STATE: {
            makeState(rhs.d_state.object());
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
ClusterCommand& ClusterCommand::operator=(ClusterCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_STATUS: {
            makeStatus(bsl::move(rhs.d_status.object()));
        } break;
        case SELECTION_ID_QUEUE_HELPER: {
            makeQueueHelper(bsl::move(rhs.d_queueHelper.object()));
        } break;
        case SELECTION_ID_FORCE_GC_QUEUES: {
            makeForceGcQueues(bsl::move(rhs.d_forceGcQueues.object()));
        } break;
        case SELECTION_ID_STORAGE: {
            makeStorage(bsl::move(rhs.d_storage.object()));
        } break;
        case SELECTION_ID_STATE: {
            makeState(bsl::move(rhs.d_state.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ClusterCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        d_status.object().~Void();
    } break;
    case SELECTION_ID_QUEUE_HELPER: {
        d_queueHelper.object().~Void();
    } break;
    case SELECTION_ID_FORCE_GC_QUEUES: {
        d_forceGcQueues.object().~Void();
    } break;
    case SELECTION_ID_STORAGE: {
        d_storage.object().~StorageCommand();
    } break;
    case SELECTION_ID_STATE: {
        d_state.object().~ClusterStateCommand();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ClusterCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_STATUS: {
        makeStatus();
    } break;
    case SELECTION_ID_QUEUE_HELPER: {
        makeQueueHelper();
    } break;
    case SELECTION_ID_FORCE_GC_QUEUES: {
        makeForceGcQueues();
    } break;
    case SELECTION_ID_STORAGE: {
        makeStorage();
    } break;
    case SELECTION_ID_STATE: {
        makeState();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ClusterCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& ClusterCommand::makeStatus()
{
    if (SELECTION_ID_STATUS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_status.object());
    }
    else {
        reset();
        new (d_status.buffer()) Void();
        d_selectionId = SELECTION_ID_STATUS;
    }

    return d_status.object();
}

Void& ClusterCommand::makeStatus(const Void& value)
{
    if (SELECTION_ID_STATUS == d_selectionId) {
        d_status.object() = value;
    }
    else {
        reset();
        new (d_status.buffer()) Void(value);
        d_selectionId = SELECTION_ID_STATUS;
    }

    return d_status.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& ClusterCommand::makeStatus(Void&& value)
{
    if (SELECTION_ID_STATUS == d_selectionId) {
        d_status.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_status.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_STATUS;
    }

    return d_status.object();
}
#endif

Void& ClusterCommand::makeQueueHelper()
{
    if (SELECTION_ID_QUEUE_HELPER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueHelper.object());
    }
    else {
        reset();
        new (d_queueHelper.buffer()) Void();
        d_selectionId = SELECTION_ID_QUEUE_HELPER;
    }

    return d_queueHelper.object();
}

Void& ClusterCommand::makeQueueHelper(const Void& value)
{
    if (SELECTION_ID_QUEUE_HELPER == d_selectionId) {
        d_queueHelper.object() = value;
    }
    else {
        reset();
        new (d_queueHelper.buffer()) Void(value);
        d_selectionId = SELECTION_ID_QUEUE_HELPER;
    }

    return d_queueHelper.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& ClusterCommand::makeQueueHelper(Void&& value)
{
    if (SELECTION_ID_QUEUE_HELPER == d_selectionId) {
        d_queueHelper.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueHelper.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_QUEUE_HELPER;
    }

    return d_queueHelper.object();
}
#endif

Void& ClusterCommand::makeForceGcQueues()
{
    if (SELECTION_ID_FORCE_GC_QUEUES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_forceGcQueues.object());
    }
    else {
        reset();
        new (d_forceGcQueues.buffer()) Void();
        d_selectionId = SELECTION_ID_FORCE_GC_QUEUES;
    }

    return d_forceGcQueues.object();
}

Void& ClusterCommand::makeForceGcQueues(const Void& value)
{
    if (SELECTION_ID_FORCE_GC_QUEUES == d_selectionId) {
        d_forceGcQueues.object() = value;
    }
    else {
        reset();
        new (d_forceGcQueues.buffer()) Void(value);
        d_selectionId = SELECTION_ID_FORCE_GC_QUEUES;
    }

    return d_forceGcQueues.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& ClusterCommand::makeForceGcQueues(Void&& value)
{
    if (SELECTION_ID_FORCE_GC_QUEUES == d_selectionId) {
        d_forceGcQueues.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_forceGcQueues.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_FORCE_GC_QUEUES;
    }

    return d_forceGcQueues.object();
}
#endif

StorageCommand& ClusterCommand::makeStorage()
{
    if (SELECTION_ID_STORAGE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_storage.object());
    }
    else {
        reset();
        new (d_storage.buffer()) StorageCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE;
    }

    return d_storage.object();
}

StorageCommand& ClusterCommand::makeStorage(const StorageCommand& value)
{
    if (SELECTION_ID_STORAGE == d_selectionId) {
        d_storage.object() = value;
    }
    else {
        reset();
        new (d_storage.buffer()) StorageCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE;
    }

    return d_storage.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageCommand& ClusterCommand::makeStorage(StorageCommand&& value)
{
    if (SELECTION_ID_STORAGE == d_selectionId) {
        d_storage.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_storage.buffer())
            StorageCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE;
    }

    return d_storage.object();
}
#endif

ClusterStateCommand& ClusterCommand::makeState()
{
    if (SELECTION_ID_STATE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_state.object());
    }
    else {
        reset();
        new (d_state.buffer()) ClusterStateCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_STATE;
    }

    return d_state.object();
}

ClusterStateCommand&
ClusterCommand::makeState(const ClusterStateCommand& value)
{
    if (SELECTION_ID_STATE == d_selectionId) {
        d_state.object() = value;
    }
    else {
        reset();
        new (d_state.buffer()) ClusterStateCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STATE;
    }

    return d_state.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStateCommand& ClusterCommand::makeState(ClusterStateCommand&& value)
{
    if (SELECTION_ID_STATE == d_selectionId) {
        d_state.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_state.buffer())
            ClusterStateCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STATE;
    }

    return d_state.object();
}
#endif

// ACCESSORS

bsl::ostream& ClusterCommand::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_STATUS: {
        printer.printAttribute("status", d_status.object());
    } break;
    case SELECTION_ID_QUEUE_HELPER: {
        printer.printAttribute("queueHelper", d_queueHelper.object());
    } break;
    case SELECTION_ID_FORCE_GC_QUEUES: {
        printer.printAttribute("forceGcQueues", d_forceGcQueues.object());
    } break;
    case SELECTION_ID_STORAGE: {
        printer.printAttribute("storage", d_storage.object());
    } break;
    case SELECTION_ID_STATE: {
        printer.printAttribute("state", d_state.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ClusterCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_STATUS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STATUS].name();
    case SELECTION_ID_QUEUE_HELPER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_HELPER].name();
    case SELECTION_ID_FORCE_GC_QUEUES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_FORCE_GC_QUEUES].name();
    case SELECTION_ID_STORAGE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE].name();
    case SELECTION_ID_STATE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STATE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -------------------
// class ClusterStatus
// -------------------

// CONSTANTS

const char ClusterStatus::CLASS_NAME[] = "ClusterStatus";

const bdlat_AttributeInfo ClusterStatus::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_DESCRIPTION,
     "description",
     sizeof("description") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_SELF_NODE_DESCRIPTION,
     "selfNodeDescription",
     sizeof("selfNodeDescription") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_IS_HEALTHY,
     "isHealthy",
     sizeof("isHealthy") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NODE_STATUSES,
     "nodeStatuses",
     sizeof("nodeStatuses") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_ELECTOR_INFO,
     "electorInfo",
     sizeof("electorInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PARTITIONS_INFO,
     "partitionsInfo",
     sizeof("partitionsInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUES_INFO,
     "queuesInfo",
     sizeof("queuesInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CLUSTER_STORAGE_SUMMARY,
     "clusterStorageSummary",
     sizeof("clusterStorageSummary") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* ClusterStatus::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterStatus::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterStatus::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DESCRIPTION];
    case ATTRIBUTE_ID_SELF_NODE_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SELF_NODE_DESCRIPTION];
    case ATTRIBUTE_ID_IS_HEALTHY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_HEALTHY];
    case ATTRIBUTE_ID_NODE_STATUSES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_STATUSES];
    case ATTRIBUTE_ID_ELECTOR_INFO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTOR_INFO];
    case ATTRIBUTE_ID_PARTITIONS_INFO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITIONS_INFO];
    case ATTRIBUTE_ID_QUEUES_INFO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES_INFO];
    case ATTRIBUTE_ID_CLUSTER_STORAGE_SUMMARY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_STORAGE_SUMMARY];
    default: return 0;
    }
}

// CREATORS

ClusterStatus::ClusterStatus(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_description(basicAllocator)
, d_selfNodeDescription(basicAllocator)
, d_queuesInfo(basicAllocator)
, d_partitionsInfo(basicAllocator)
, d_nodeStatuses(basicAllocator)
, d_electorInfo(basicAllocator)
, d_clusterStorageSummary(basicAllocator)
, d_isHealthy()
{
}

ClusterStatus::ClusterStatus(const ClusterStatus& original,
                             bslma::Allocator*    basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_description(original.d_description, basicAllocator)
, d_selfNodeDescription(original.d_selfNodeDescription, basicAllocator)
, d_queuesInfo(original.d_queuesInfo, basicAllocator)
, d_partitionsInfo(original.d_partitionsInfo, basicAllocator)
, d_nodeStatuses(original.d_nodeStatuses, basicAllocator)
, d_electorInfo(original.d_electorInfo, basicAllocator)
, d_clusterStorageSummary(original.d_clusterStorageSummary, basicAllocator)
, d_isHealthy(original.d_isHealthy)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStatus::ClusterStatus(ClusterStatus&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_description(bsl::move(original.d_description)),
  d_selfNodeDescription(bsl::move(original.d_selfNodeDescription)),
  d_queuesInfo(bsl::move(original.d_queuesInfo)),
  d_partitionsInfo(bsl::move(original.d_partitionsInfo)),
  d_nodeStatuses(bsl::move(original.d_nodeStatuses)),
  d_electorInfo(bsl::move(original.d_electorInfo)),
  d_clusterStorageSummary(bsl::move(original.d_clusterStorageSummary)),
  d_isHealthy(bsl::move(original.d_isHealthy))
{
}

ClusterStatus::ClusterStatus(ClusterStatus&&   original,
                             bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_description(bsl::move(original.d_description), basicAllocator)
, d_selfNodeDescription(bsl::move(original.d_selfNodeDescription),
                        basicAllocator)
, d_queuesInfo(bsl::move(original.d_queuesInfo), basicAllocator)
, d_partitionsInfo(bsl::move(original.d_partitionsInfo), basicAllocator)
, d_nodeStatuses(bsl::move(original.d_nodeStatuses), basicAllocator)
, d_electorInfo(bsl::move(original.d_electorInfo), basicAllocator)
, d_clusterStorageSummary(bsl::move(original.d_clusterStorageSummary),
                          basicAllocator)
, d_isHealthy(bsl::move(original.d_isHealthy))
{
}
#endif

ClusterStatus::~ClusterStatus()
{
}

// MANIPULATORS

ClusterStatus& ClusterStatus::operator=(const ClusterStatus& rhs)
{
    if (this != &rhs) {
        d_name                  = rhs.d_name;
        d_description           = rhs.d_description;
        d_selfNodeDescription   = rhs.d_selfNodeDescription;
        d_isHealthy             = rhs.d_isHealthy;
        d_nodeStatuses          = rhs.d_nodeStatuses;
        d_electorInfo           = rhs.d_electorInfo;
        d_partitionsInfo        = rhs.d_partitionsInfo;
        d_queuesInfo            = rhs.d_queuesInfo;
        d_clusterStorageSummary = rhs.d_clusterStorageSummary;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStatus& ClusterStatus::operator=(ClusterStatus&& rhs)
{
    if (this != &rhs) {
        d_name                  = bsl::move(rhs.d_name);
        d_description           = bsl::move(rhs.d_description);
        d_selfNodeDescription   = bsl::move(rhs.d_selfNodeDescription);
        d_isHealthy             = bsl::move(rhs.d_isHealthy);
        d_nodeStatuses          = bsl::move(rhs.d_nodeStatuses);
        d_electorInfo           = bsl::move(rhs.d_electorInfo);
        d_partitionsInfo        = bsl::move(rhs.d_partitionsInfo);
        d_queuesInfo            = bsl::move(rhs.d_queuesInfo);
        d_clusterStorageSummary = bsl::move(rhs.d_clusterStorageSummary);
    }

    return *this;
}
#endif

void ClusterStatus::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_description);
    bdlat_ValueTypeFunctions::reset(&d_selfNodeDescription);
    bdlat_ValueTypeFunctions::reset(&d_isHealthy);
    bdlat_ValueTypeFunctions::reset(&d_nodeStatuses);
    bdlat_ValueTypeFunctions::reset(&d_electorInfo);
    bdlat_ValueTypeFunctions::reset(&d_partitionsInfo);
    bdlat_ValueTypeFunctions::reset(&d_queuesInfo);
    bdlat_ValueTypeFunctions::reset(&d_clusterStorageSummary);
}

// ACCESSORS

bsl::ostream&
ClusterStatus::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("description", this->description());
    printer.printAttribute("selfNodeDescription", this->selfNodeDescription());
    printer.printAttribute("isHealthy", this->isHealthy());
    printer.printAttribute("nodeStatuses", this->nodeStatuses());
    printer.printAttribute("electorInfo", this->electorInfo());
    printer.printAttribute("partitionsInfo", this->partitionsInfo());
    printer.printAttribute("queuesInfo", this->queuesInfo());
    printer.printAttribute("clusterStorageSummary",
                           this->clusterStorageSummary());
    printer.end();
    return stream;
}

// --------------------
// class DomainsCommand
// --------------------

// CONSTANTS

const char DomainsCommand::CLASS_NAME[] = "DomainsCommand";

const bdlat_SelectionInfo DomainsCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_DOMAIN,
     "domain",
     sizeof("domain") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_RESOLVER,
     "resolver",
     sizeof("resolver") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_RECONFIGURE,
     "reconfigure",
     sizeof("reconfigure") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
DomainsCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            DomainsCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* DomainsCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_DOMAIN:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN];
    case SELECTION_ID_RESOLVER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_RESOLVER];
    case SELECTION_ID_RECONFIGURE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_RECONFIGURE];
    default: return 0;
    }
}

// CREATORS

DomainsCommand::DomainsCommand(const DomainsCommand& original,
                               bslma::Allocator*     basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            Domain(original.d_domain.object(), d_allocator_p);
    } break;
    case SELECTION_ID_RESOLVER: {
        new (d_resolver.buffer())
            DomainResolverCommand(original.d_resolver.object(), d_allocator_p);
    } break;
    case SELECTION_ID_RECONFIGURE: {
        new (d_reconfigure.buffer())
            DomainReconfigure(original.d_reconfigure.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainsCommand::DomainsCommand(DomainsCommand&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            Domain(bsl::move(original.d_domain.object()), d_allocator_p);
    } break;
    case SELECTION_ID_RESOLVER: {
        new (d_resolver.buffer())
            DomainResolverCommand(bsl::move(original.d_resolver.object()),
                                  d_allocator_p);
    } break;
    case SELECTION_ID_RECONFIGURE: {
        new (d_reconfigure.buffer())
            DomainReconfigure(bsl::move(original.d_reconfigure.object()),
                              d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

DomainsCommand::DomainsCommand(DomainsCommand&&  original,
                               bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        new (d_domain.buffer())
            Domain(bsl::move(original.d_domain.object()), d_allocator_p);
    } break;
    case SELECTION_ID_RESOLVER: {
        new (d_resolver.buffer())
            DomainResolverCommand(bsl::move(original.d_resolver.object()),
                                  d_allocator_p);
    } break;
    case SELECTION_ID_RECONFIGURE: {
        new (d_reconfigure.buffer())
            DomainReconfigure(bsl::move(original.d_reconfigure.object()),
                              d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

DomainsCommand& DomainsCommand::operator=(const DomainsCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DOMAIN: {
            makeDomain(rhs.d_domain.object());
        } break;
        case SELECTION_ID_RESOLVER: {
            makeResolver(rhs.d_resolver.object());
        } break;
        case SELECTION_ID_RECONFIGURE: {
            makeReconfigure(rhs.d_reconfigure.object());
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
DomainsCommand& DomainsCommand::operator=(DomainsCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DOMAIN: {
            makeDomain(bsl::move(rhs.d_domain.object()));
        } break;
        case SELECTION_ID_RESOLVER: {
            makeResolver(bsl::move(rhs.d_resolver.object()));
        } break;
        case SELECTION_ID_RECONFIGURE: {
            makeReconfigure(bsl::move(rhs.d_reconfigure.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void DomainsCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        d_domain.object().~Domain();
    } break;
    case SELECTION_ID_RESOLVER: {
        d_resolver.object().~DomainResolverCommand();
    } break;
    case SELECTION_ID_RECONFIGURE: {
        d_reconfigure.object().~DomainReconfigure();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int DomainsCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_DOMAIN: {
        makeDomain();
    } break;
    case SELECTION_ID_RESOLVER: {
        makeResolver();
    } break;
    case SELECTION_ID_RECONFIGURE: {
        makeReconfigure();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int DomainsCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Domain& DomainsCommand::makeDomain()
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domain.object());
    }
    else {
        reset();
        new (d_domain.buffer()) Domain(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}

Domain& DomainsCommand::makeDomain(const Domain& value)
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        d_domain.object() = value;
    }
    else {
        reset();
        new (d_domain.buffer()) Domain(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Domain& DomainsCommand::makeDomain(Domain&& value)
{
    if (SELECTION_ID_DOMAIN == d_selectionId) {
        d_domain.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domain.buffer()) Domain(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN;
    }

    return d_domain.object();
}
#endif

DomainResolverCommand& DomainsCommand::makeResolver()
{
    if (SELECTION_ID_RESOLVER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_resolver.object());
    }
    else {
        reset();
        new (d_resolver.buffer()) DomainResolverCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_RESOLVER;
    }

    return d_resolver.object();
}

DomainResolverCommand&
DomainsCommand::makeResolver(const DomainResolverCommand& value)
{
    if (SELECTION_ID_RESOLVER == d_selectionId) {
        d_resolver.object() = value;
    }
    else {
        reset();
        new (d_resolver.buffer()) DomainResolverCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_RESOLVER;
    }

    return d_resolver.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainResolverCommand&
DomainsCommand::makeResolver(DomainResolverCommand&& value)
{
    if (SELECTION_ID_RESOLVER == d_selectionId) {
        d_resolver.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_resolver.buffer())
            DomainResolverCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_RESOLVER;
    }

    return d_resolver.object();
}
#endif

DomainReconfigure& DomainsCommand::makeReconfigure()
{
    if (SELECTION_ID_RECONFIGURE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_reconfigure.object());
    }
    else {
        reset();
        new (d_reconfigure.buffer()) DomainReconfigure(d_allocator_p);
        d_selectionId = SELECTION_ID_RECONFIGURE;
    }

    return d_reconfigure.object();
}

DomainReconfigure&
DomainsCommand::makeReconfigure(const DomainReconfigure& value)
{
    if (SELECTION_ID_RECONFIGURE == d_selectionId) {
        d_reconfigure.object() = value;
    }
    else {
        reset();
        new (d_reconfigure.buffer()) DomainReconfigure(value, d_allocator_p);
        d_selectionId = SELECTION_ID_RECONFIGURE;
    }

    return d_reconfigure.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainReconfigure& DomainsCommand::makeReconfigure(DomainReconfigure&& value)
{
    if (SELECTION_ID_RECONFIGURE == d_selectionId) {
        d_reconfigure.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_reconfigure.buffer())
            DomainReconfigure(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_RECONFIGURE;
    }

    return d_reconfigure.object();
}
#endif

// ACCESSORS

bsl::ostream& DomainsCommand::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN: {
        printer.printAttribute("domain", d_domain.object());
    } break;
    case SELECTION_ID_RESOLVER: {
        printer.printAttribute("resolver", d_resolver.object());
    } break;
    case SELECTION_ID_RECONFIGURE: {
        printer.printAttribute("reconfigure", d_reconfigure.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* DomainsCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN].name();
    case SELECTION_ID_RESOLVER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_RESOLVER].name();
    case SELECTION_ID_RECONFIGURE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_RECONFIGURE].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------------
// class RoundRobinRouter
// ----------------------

// CONSTANTS

const char RoundRobinRouter::CLASS_NAME[] = "RoundRobinRouter";

const bdlat_AttributeInfo RoundRobinRouter::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CONSUMERS,
     "consumers",
     sizeof("consumers") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
RoundRobinRouter::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RoundRobinRouter::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RoundRobinRouter::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CONSUMERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMERS];
    default: return 0;
    }
}

// CREATORS

RoundRobinRouter::RoundRobinRouter(bslma::Allocator* basicAllocator)
: d_consumers(basicAllocator)
{
}

RoundRobinRouter::RoundRobinRouter(const RoundRobinRouter& original,
                                   bslma::Allocator*       basicAllocator)
: d_consumers(original.d_consumers, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RoundRobinRouter::RoundRobinRouter(RoundRobinRouter&& original) noexcept
: d_consumers(bsl::move(original.d_consumers))
{
}

RoundRobinRouter::RoundRobinRouter(RoundRobinRouter&& original,
                                   bslma::Allocator*  basicAllocator)
: d_consumers(bsl::move(original.d_consumers), basicAllocator)
{
}
#endif

RoundRobinRouter::~RoundRobinRouter()
{
}

// MANIPULATORS

RoundRobinRouter& RoundRobinRouter::operator=(const RoundRobinRouter& rhs)
{
    if (this != &rhs) {
        d_consumers = rhs.d_consumers;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RoundRobinRouter& RoundRobinRouter::operator=(RoundRobinRouter&& rhs)
{
    if (this != &rhs) {
        d_consumers = bsl::move(rhs.d_consumers);
    }

    return *this;
}
#endif

void RoundRobinRouter::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_consumers);
}

// ACCESSORS

bsl::ostream& RoundRobinRouter::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("consumers", this->consumers());
    printer.end();
    return stream;
}

// -------------------
// class StorageResult
// -------------------

// CONSTANTS

const char StorageResult::CLASS_NAME[] = "StorageResult";

const bdlat_SelectionInfo StorageResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_SUCCESS,
     "success",
     sizeof("success") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STORAGE_CONTENT,
     "storageContent",
     sizeof("storageContent") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_STORAGE_SUMMARY,
     "clusterStorageSummary",
     sizeof("clusterStorageSummary") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_REPLICATION_RESULT,
     "replicationResult",
     sizeof("replicationResult") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PURGED_QUEUES,
     "purgedQueues",
     sizeof("purgedQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* StorageResult::lookupSelectionInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            StorageResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* StorageResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_SUCCESS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS];
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_STORAGE_CONTENT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_CONTENT];
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STORAGE_SUMMARY];
    case SELECTION_ID_REPLICATION_RESULT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICATION_RESULT];
    case SELECTION_ID_PURGED_QUEUES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGED_QUEUES];
    default: return 0;
    }
}

// CREATORS

StorageResult::StorageResult(const StorageResult& original,
                             bslma::Allocator*    basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(original.d_success.object());
    } break;
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        new (d_storageContent.buffer())
            StorageContent(original.d_storageContent.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(original.d_clusterStorageSummary.object(),
                                  d_allocator_p);
    } break;
    case SELECTION_ID_REPLICATION_RESULT: {
        new (d_replicationResult.buffer())
            ReplicationResult(original.d_replicationResult.object(),
                              d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(original.d_purgedQueues.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageResult::StorageResult(StorageResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        new (d_storageContent.buffer())
            StorageContent(bsl::move(original.d_storageContent.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        new (d_clusterStorageSummary.buffer()) ClusterStorageSummary(
            bsl::move(original.d_clusterStorageSummary.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_REPLICATION_RESULT: {
        new (d_replicationResult.buffer())
            ReplicationResult(bsl::move(original.d_replicationResult.object()),
                              d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(original.d_purgedQueues.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

StorageResult::StorageResult(StorageResult&&   original,
                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        new (d_storageContent.buffer())
            StorageContent(bsl::move(original.d_storageContent.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        new (d_clusterStorageSummary.buffer()) ClusterStorageSummary(
            bsl::move(original.d_clusterStorageSummary.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_REPLICATION_RESULT: {
        new (d_replicationResult.buffer())
            ReplicationResult(bsl::move(original.d_replicationResult.object()),
                              d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(original.d_purgedQueues.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

StorageResult& StorageResult::operator=(const StorageResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SUCCESS: {
            makeSuccess(rhs.d_success.object());
        } break;
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_STORAGE_CONTENT: {
            makeStorageContent(rhs.d_storageContent.object());
        } break;
        case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
            makeClusterStorageSummary(rhs.d_clusterStorageSummary.object());
        } break;
        case SELECTION_ID_REPLICATION_RESULT: {
            makeReplicationResult(rhs.d_replicationResult.object());
        } break;
        case SELECTION_ID_PURGED_QUEUES: {
            makePurgedQueues(rhs.d_purgedQueues.object());
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
StorageResult& StorageResult::operator=(StorageResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SUCCESS: {
            makeSuccess(bsl::move(rhs.d_success.object()));
        } break;
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_STORAGE_CONTENT: {
            makeStorageContent(bsl::move(rhs.d_storageContent.object()));
        } break;
        case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
            makeClusterStorageSummary(
                bsl::move(rhs.d_clusterStorageSummary.object()));
        } break;
        case SELECTION_ID_REPLICATION_RESULT: {
            makeReplicationResult(bsl::move(rhs.d_replicationResult.object()));
        } break;
        case SELECTION_ID_PURGED_QUEUES: {
            makePurgedQueues(bsl::move(rhs.d_purgedQueues.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void StorageResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        d_success.object().~Void();
    } break;
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        d_storageContent.object().~StorageContent();
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        d_clusterStorageSummary.object().~ClusterStorageSummary();
    } break;
    case SELECTION_ID_REPLICATION_RESULT: {
        d_replicationResult.object().~ReplicationResult();
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        d_purgedQueues.object().~PurgedQueues();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int StorageResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_SUCCESS: {
        makeSuccess();
    } break;
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        makeStorageContent();
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        makeClusterStorageSummary();
    } break;
    case SELECTION_ID_REPLICATION_RESULT: {
        makeReplicationResult();
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        makePurgedQueues();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int StorageResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& StorageResult::makeSuccess()
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_success.object());
    }
    else {
        reset();
        new (d_success.buffer()) Void();
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

Void& StorageResult::makeSuccess(const Void& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = value;
    }
    else {
        reset();
        new (d_success.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& StorageResult::makeSuccess(Void&& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_success.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}
#endif

Error& StorageResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& StorageResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& StorageResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

StorageContent& StorageResult::makeStorageContent()
{
    if (SELECTION_ID_STORAGE_CONTENT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_storageContent.object());
    }
    else {
        reset();
        new (d_storageContent.buffer()) StorageContent(d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE_CONTENT;
    }

    return d_storageContent.object();
}

StorageContent& StorageResult::makeStorageContent(const StorageContent& value)
{
    if (SELECTION_ID_STORAGE_CONTENT == d_selectionId) {
        d_storageContent.object() = value;
    }
    else {
        reset();
        new (d_storageContent.buffer()) StorageContent(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE_CONTENT;
    }

    return d_storageContent.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageContent& StorageResult::makeStorageContent(StorageContent&& value)
{
    if (SELECTION_ID_STORAGE_CONTENT == d_selectionId) {
        d_storageContent.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_storageContent.buffer())
            StorageContent(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE_CONTENT;
    }

    return d_storageContent.object();
}
#endif

ClusterStorageSummary& StorageResult::makeClusterStorageSummary()
{
    if (SELECTION_ID_CLUSTER_STORAGE_SUMMARY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterStorageSummary.object());
    }
    else {
        reset();
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STORAGE_SUMMARY;
    }

    return d_clusterStorageSummary.object();
}

ClusterStorageSummary&
StorageResult::makeClusterStorageSummary(const ClusterStorageSummary& value)
{
    if (SELECTION_ID_CLUSTER_STORAGE_SUMMARY == d_selectionId) {
        d_clusterStorageSummary.object() = value;
    }
    else {
        reset();
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STORAGE_SUMMARY;
    }

    return d_clusterStorageSummary.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStorageSummary&
StorageResult::makeClusterStorageSummary(ClusterStorageSummary&& value)
{
    if (SELECTION_ID_CLUSTER_STORAGE_SUMMARY == d_selectionId) {
        d_clusterStorageSummary.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STORAGE_SUMMARY;
    }

    return d_clusterStorageSummary.object();
}
#endif

ReplicationResult& StorageResult::makeReplicationResult()
{
    if (SELECTION_ID_REPLICATION_RESULT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_replicationResult.object());
    }
    else {
        reset();
        new (d_replicationResult.buffer()) ReplicationResult(d_allocator_p);
        d_selectionId = SELECTION_ID_REPLICATION_RESULT;
    }

    return d_replicationResult.object();
}

ReplicationResult&
StorageResult::makeReplicationResult(const ReplicationResult& value)
{
    if (SELECTION_ID_REPLICATION_RESULT == d_selectionId) {
        d_replicationResult.object() = value;
    }
    else {
        reset();
        new (d_replicationResult.buffer())
            ReplicationResult(value, d_allocator_p);
        d_selectionId = SELECTION_ID_REPLICATION_RESULT;
    }

    return d_replicationResult.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReplicationResult&
StorageResult::makeReplicationResult(ReplicationResult&& value)
{
    if (SELECTION_ID_REPLICATION_RESULT == d_selectionId) {
        d_replicationResult.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_replicationResult.buffer())
            ReplicationResult(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_REPLICATION_RESULT;
    }

    return d_replicationResult.object();
}
#endif

PurgedQueues& StorageResult::makePurgedQueues()
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_purgedQueues.object());
    }
    else {
        reset();
        new (d_purgedQueues.buffer()) PurgedQueues(d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}

PurgedQueues& StorageResult::makePurgedQueues(const PurgedQueues& value)
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        d_purgedQueues.object() = value;
    }
    else {
        reset();
        new (d_purgedQueues.buffer()) PurgedQueues(value, d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgedQueues& StorageResult::makePurgedQueues(PurgedQueues&& value)
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        d_purgedQueues.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}
#endif

// ACCESSORS

bsl::ostream&
StorageResult::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        printer.printAttribute("success", d_success.object());
    } break;
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        printer.printAttribute("storageContent", d_storageContent.object());
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        printer.printAttribute("clusterStorageSummary",
                               d_clusterStorageSummary.object());
    } break;
    case SELECTION_ID_REPLICATION_RESULT: {
        printer.printAttribute("replicationResult",
                               d_replicationResult.object());
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        printer.printAttribute("purgedQueues", d_purgedQueues.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* StorageResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS].name();
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_STORAGE_CONTENT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_CONTENT].name();
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STORAGE_SUMMARY]
            .name();
    case SELECTION_ID_REPLICATION_RESULT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REPLICATION_RESULT].name();
    case SELECTION_ID_PURGED_QUEUES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGED_QUEUES].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------
// class AppState
// --------------

// CONSTANTS

const char AppState::CLASS_NAME[] = "AppState";

const bdlat_AttributeInfo AppState::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NUM_CONSUMERS,
     "numConsumers",
     sizeof("numConsumers") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_REDELIVERY_LIST_LENGTH,
     "redeliveryListLength",
     sizeof("redeliveryListLength") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ROUND_ROBIN_ROUTER,
     "roundRobinRouter",
     sizeof("roundRobinRouter") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* AppState::lookupAttributeInfo(const char* name,
                                                         int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            AppState::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* AppState::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_NUM_CONSUMERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_CONSUMERS];
    case ATTRIBUTE_ID_REDELIVERY_LIST_LENGTH:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REDELIVERY_LIST_LENGTH];
    case ATTRIBUTE_ID_ROUND_ROBIN_ROUTER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROUND_ROBIN_ROUTER];
    default: return 0;
    }
}

// CREATORS

AppState::AppState(bslma::Allocator* basicAllocator)
: d_redeliveryListLength()
, d_appId(basicAllocator)
, d_roundRobinRouter(basicAllocator)
, d_numConsumers()
{
}

AppState::AppState(const AppState& original, bslma::Allocator* basicAllocator)
: d_redeliveryListLength(original.d_redeliveryListLength)
, d_appId(original.d_appId, basicAllocator)
, d_roundRobinRouter(original.d_roundRobinRouter, basicAllocator)
, d_numConsumers(original.d_numConsumers)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AppState::AppState(AppState&& original) noexcept
: d_redeliveryListLength(bsl::move(original.d_redeliveryListLength)),
  d_appId(bsl::move(original.d_appId)),
  d_roundRobinRouter(bsl::move(original.d_roundRobinRouter)),
  d_numConsumers(bsl::move(original.d_numConsumers))
{
}

AppState::AppState(AppState&& original, bslma::Allocator* basicAllocator)
: d_redeliveryListLength(bsl::move(original.d_redeliveryListLength))
, d_appId(bsl::move(original.d_appId), basicAllocator)
, d_roundRobinRouter(bsl::move(original.d_roundRobinRouter), basicAllocator)
, d_numConsumers(bsl::move(original.d_numConsumers))
{
}
#endif

AppState::~AppState()
{
}

// MANIPULATORS

AppState& AppState::operator=(const AppState& rhs)
{
    if (this != &rhs) {
        d_appId                = rhs.d_appId;
        d_numConsumers         = rhs.d_numConsumers;
        d_redeliveryListLength = rhs.d_redeliveryListLength;
        d_roundRobinRouter     = rhs.d_roundRobinRouter;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AppState& AppState::operator=(AppState&& rhs)
{
    if (this != &rhs) {
        d_appId                = bsl::move(rhs.d_appId);
        d_numConsumers         = bsl::move(rhs.d_numConsumers);
        d_redeliveryListLength = bsl::move(rhs.d_redeliveryListLength);
        d_roundRobinRouter     = bsl::move(rhs.d_roundRobinRouter);
    }

    return *this;
}
#endif

void AppState::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_appId);
    bdlat_ValueTypeFunctions::reset(&d_numConsumers);
    bdlat_ValueTypeFunctions::reset(&d_redeliveryListLength);
    bdlat_ValueTypeFunctions::reset(&d_roundRobinRouter);
}

// ACCESSORS

bsl::ostream&
AppState::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("appId", this->appId());
    printer.printAttribute("numConsumers", this->numConsumers());
    printer.printAttribute("redeliveryListLength",
                           this->redeliveryListLength());
    printer.printAttribute("roundRobinRouter", this->roundRobinRouter());
    printer.end();
    return stream;
}

// -------------
// class Cluster
// -------------

// CONSTANTS

const char Cluster::CLASS_NAME[] = "Cluster";

const bdlat_AttributeInfo Cluster::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_COMMAND,
     "command",
     sizeof("command") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Cluster::lookupAttributeInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Cluster::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Cluster::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_COMMAND:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND];
    default: return 0;
    }
}

// CREATORS

Cluster::Cluster(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_command(basicAllocator)
{
}

Cluster::Cluster(const Cluster& original, bslma::Allocator* basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_command(original.d_command, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Cluster::Cluster(Cluster&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_command(bsl::move(original.d_command))
{
}

Cluster::Cluster(Cluster&& original, bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_command(bsl::move(original.d_command), basicAllocator)
{
}
#endif

Cluster::~Cluster()
{
}

// MANIPULATORS

Cluster& Cluster::operator=(const Cluster& rhs)
{
    if (this != &rhs) {
        d_name    = rhs.d_name;
        d_command = rhs.d_command;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Cluster& Cluster::operator=(Cluster&& rhs)
{
    if (this != &rhs) {
        d_name    = bsl::move(rhs.d_name);
        d_command = bsl::move(rhs.d_command);
    }

    return *this;
}
#endif

void Cluster::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_command);
}

// ACCESSORS

bsl::ostream&
Cluster::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("command", this->command());
    printer.end();
    return stream;
}

// -------------------
// class ClusterResult
// -------------------

// CONSTANTS

const char ClusterResult::CLASS_NAME[] = "ClusterResult";

const bdlat_SelectionInfo ClusterResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SUCCESS,
     "success",
     sizeof("success") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ELECTOR_RESULT,
     "electorResult",
     sizeof("electorResult") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STORAGE_RESULT,
     "storageResult",
     sizeof("storageResult") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_QUEUE_HELPER,
     "clusterQueueHelper",
     sizeof("clusterQueueHelper") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_STATUS,
     "clusterStatus",
     sizeof("clusterStatus") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_PROXY_STATUS,
     "clusterProxyStatus",
     sizeof("clusterProxyStatus") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* ClusterResult::lookupSelectionInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 7; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ClusterResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ClusterResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_SUCCESS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS];
    case SELECTION_ID_ELECTOR_RESULT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTOR_RESULT];
    case SELECTION_ID_STORAGE_RESULT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_RESULT];
    case SELECTION_ID_CLUSTER_QUEUE_HELPER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_QUEUE_HELPER];
    case SELECTION_ID_CLUSTER_STATUS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STATUS];
    case SELECTION_ID_CLUSTER_PROXY_STATUS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_PROXY_STATUS];
    default: return 0;
    }
}

// CREATORS

ClusterResult::ClusterResult(const ClusterResult& original,
                             bslma::Allocator*    basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(original.d_success.object());
    } break;
    case SELECTION_ID_ELECTOR_RESULT: {
        new (d_electorResult.buffer())
            ElectorResult(original.d_electorResult.object(), d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_RESULT: {
        new (d_storageResult.buffer())
            StorageResult(original.d_storageResult.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        new (d_clusterQueueHelper.buffer())
            ClusterQueueHelper(original.d_clusterQueueHelper.object(),
                               d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        new (d_clusterStatus.buffer())
            ClusterStatus(original.d_clusterStatus.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        new (d_clusterProxyStatus.buffer())
            ClusterProxyStatus(original.d_clusterProxyStatus.object(),
                               d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterResult::ClusterResult(ClusterResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_ELECTOR_RESULT: {
        new (d_electorResult.buffer())
            ElectorResult(bsl::move(original.d_electorResult.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_RESULT: {
        new (d_storageResult.buffer())
            StorageResult(bsl::move(original.d_storageResult.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        new (d_clusterQueueHelper.buffer()) ClusterQueueHelper(
            bsl::move(original.d_clusterQueueHelper.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        new (d_clusterStatus.buffer())
            ClusterStatus(bsl::move(original.d_clusterStatus.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        new (d_clusterProxyStatus.buffer()) ClusterProxyStatus(
            bsl::move(original.d_clusterProxyStatus.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ClusterResult::ClusterResult(ClusterResult&&   original,
                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_ELECTOR_RESULT: {
        new (d_electorResult.buffer())
            ElectorResult(bsl::move(original.d_electorResult.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_RESULT: {
        new (d_storageResult.buffer())
            StorageResult(bsl::move(original.d_storageResult.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        new (d_clusterQueueHelper.buffer()) ClusterQueueHelper(
            bsl::move(original.d_clusterQueueHelper.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        new (d_clusterStatus.buffer())
            ClusterStatus(bsl::move(original.d_clusterStatus.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        new (d_clusterProxyStatus.buffer()) ClusterProxyStatus(
            bsl::move(original.d_clusterProxyStatus.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ClusterResult& ClusterResult::operator=(const ClusterResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_SUCCESS: {
            makeSuccess(rhs.d_success.object());
        } break;
        case SELECTION_ID_ELECTOR_RESULT: {
            makeElectorResult(rhs.d_electorResult.object());
        } break;
        case SELECTION_ID_STORAGE_RESULT: {
            makeStorageResult(rhs.d_storageResult.object());
        } break;
        case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
            makeClusterQueueHelper(rhs.d_clusterQueueHelper.object());
        } break;
        case SELECTION_ID_CLUSTER_STATUS: {
            makeClusterStatus(rhs.d_clusterStatus.object());
        } break;
        case SELECTION_ID_CLUSTER_PROXY_STATUS: {
            makeClusterProxyStatus(rhs.d_clusterProxyStatus.object());
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
ClusterResult& ClusterResult::operator=(ClusterResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_SUCCESS: {
            makeSuccess(bsl::move(rhs.d_success.object()));
        } break;
        case SELECTION_ID_ELECTOR_RESULT: {
            makeElectorResult(bsl::move(rhs.d_electorResult.object()));
        } break;
        case SELECTION_ID_STORAGE_RESULT: {
            makeStorageResult(bsl::move(rhs.d_storageResult.object()));
        } break;
        case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
            makeClusterQueueHelper(
                bsl::move(rhs.d_clusterQueueHelper.object()));
        } break;
        case SELECTION_ID_CLUSTER_STATUS: {
            makeClusterStatus(bsl::move(rhs.d_clusterStatus.object()));
        } break;
        case SELECTION_ID_CLUSTER_PROXY_STATUS: {
            makeClusterProxyStatus(
                bsl::move(rhs.d_clusterProxyStatus.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ClusterResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_SUCCESS: {
        d_success.object().~Void();
    } break;
    case SELECTION_ID_ELECTOR_RESULT: {
        d_electorResult.object().~ElectorResult();
    } break;
    case SELECTION_ID_STORAGE_RESULT: {
        d_storageResult.object().~StorageResult();
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        d_clusterQueueHelper.object().~ClusterQueueHelper();
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        d_clusterStatus.object().~ClusterStatus();
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        d_clusterProxyStatus.object().~ClusterProxyStatus();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ClusterResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_SUCCESS: {
        makeSuccess();
    } break;
    case SELECTION_ID_ELECTOR_RESULT: {
        makeElectorResult();
    } break;
    case SELECTION_ID_STORAGE_RESULT: {
        makeStorageResult();
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        makeClusterQueueHelper();
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        makeClusterStatus();
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        makeClusterProxyStatus();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ClusterResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Error& ClusterResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& ClusterResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& ClusterResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

Void& ClusterResult::makeSuccess()
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_success.object());
    }
    else {
        reset();
        new (d_success.buffer()) Void();
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

Void& ClusterResult::makeSuccess(const Void& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = value;
    }
    else {
        reset();
        new (d_success.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& ClusterResult::makeSuccess(Void&& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_success.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}
#endif

ElectorResult& ClusterResult::makeElectorResult()
{
    if (SELECTION_ID_ELECTOR_RESULT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_electorResult.object());
    }
    else {
        reset();
        new (d_electorResult.buffer()) ElectorResult(d_allocator_p);
        d_selectionId = SELECTION_ID_ELECTOR_RESULT;
    }

    return d_electorResult.object();
}

ElectorResult& ClusterResult::makeElectorResult(const ElectorResult& value)
{
    if (SELECTION_ID_ELECTOR_RESULT == d_selectionId) {
        d_electorResult.object() = value;
    }
    else {
        reset();
        new (d_electorResult.buffer()) ElectorResult(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ELECTOR_RESULT;
    }

    return d_electorResult.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectorResult& ClusterResult::makeElectorResult(ElectorResult&& value)
{
    if (SELECTION_ID_ELECTOR_RESULT == d_selectionId) {
        d_electorResult.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_electorResult.buffer())
            ElectorResult(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ELECTOR_RESULT;
    }

    return d_electorResult.object();
}
#endif

StorageResult& ClusterResult::makeStorageResult()
{
    if (SELECTION_ID_STORAGE_RESULT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_storageResult.object());
    }
    else {
        reset();
        new (d_storageResult.buffer()) StorageResult(d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE_RESULT;
    }

    return d_storageResult.object();
}

StorageResult& ClusterResult::makeStorageResult(const StorageResult& value)
{
    if (SELECTION_ID_STORAGE_RESULT == d_selectionId) {
        d_storageResult.object() = value;
    }
    else {
        reset();
        new (d_storageResult.buffer()) StorageResult(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE_RESULT;
    }

    return d_storageResult.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageResult& ClusterResult::makeStorageResult(StorageResult&& value)
{
    if (SELECTION_ID_STORAGE_RESULT == d_selectionId) {
        d_storageResult.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_storageResult.buffer())
            StorageResult(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE_RESULT;
    }

    return d_storageResult.object();
}
#endif

ClusterQueueHelper& ClusterResult::makeClusterQueueHelper()
{
    if (SELECTION_ID_CLUSTER_QUEUE_HELPER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterQueueHelper.object());
    }
    else {
        reset();
        new (d_clusterQueueHelper.buffer()) ClusterQueueHelper(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_QUEUE_HELPER;
    }

    return d_clusterQueueHelper.object();
}

ClusterQueueHelper&
ClusterResult::makeClusterQueueHelper(const ClusterQueueHelper& value)
{
    if (SELECTION_ID_CLUSTER_QUEUE_HELPER == d_selectionId) {
        d_clusterQueueHelper.object() = value;
    }
    else {
        reset();
        new (d_clusterQueueHelper.buffer())
            ClusterQueueHelper(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_QUEUE_HELPER;
    }

    return d_clusterQueueHelper.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterQueueHelper&
ClusterResult::makeClusterQueueHelper(ClusterQueueHelper&& value)
{
    if (SELECTION_ID_CLUSTER_QUEUE_HELPER == d_selectionId) {
        d_clusterQueueHelper.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterQueueHelper.buffer())
            ClusterQueueHelper(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_QUEUE_HELPER;
    }

    return d_clusterQueueHelper.object();
}
#endif

ClusterStatus& ClusterResult::makeClusterStatus()
{
    if (SELECTION_ID_CLUSTER_STATUS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterStatus.object());
    }
    else {
        reset();
        new (d_clusterStatus.buffer()) ClusterStatus(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STATUS;
    }

    return d_clusterStatus.object();
}

ClusterStatus& ClusterResult::makeClusterStatus(const ClusterStatus& value)
{
    if (SELECTION_ID_CLUSTER_STATUS == d_selectionId) {
        d_clusterStatus.object() = value;
    }
    else {
        reset();
        new (d_clusterStatus.buffer()) ClusterStatus(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STATUS;
    }

    return d_clusterStatus.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStatus& ClusterResult::makeClusterStatus(ClusterStatus&& value)
{
    if (SELECTION_ID_CLUSTER_STATUS == d_selectionId) {
        d_clusterStatus.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterStatus.buffer())
            ClusterStatus(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STATUS;
    }

    return d_clusterStatus.object();
}
#endif

ClusterProxyStatus& ClusterResult::makeClusterProxyStatus()
{
    if (SELECTION_ID_CLUSTER_PROXY_STATUS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterProxyStatus.object());
    }
    else {
        reset();
        new (d_clusterProxyStatus.buffer()) ClusterProxyStatus(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_PROXY_STATUS;
    }

    return d_clusterProxyStatus.object();
}

ClusterProxyStatus&
ClusterResult::makeClusterProxyStatus(const ClusterProxyStatus& value)
{
    if (SELECTION_ID_CLUSTER_PROXY_STATUS == d_selectionId) {
        d_clusterProxyStatus.object() = value;
    }
    else {
        reset();
        new (d_clusterProxyStatus.buffer())
            ClusterProxyStatus(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_PROXY_STATUS;
    }

    return d_clusterProxyStatus.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterProxyStatus&
ClusterResult::makeClusterProxyStatus(ClusterProxyStatus&& value)
{
    if (SELECTION_ID_CLUSTER_PROXY_STATUS == d_selectionId) {
        d_clusterProxyStatus.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterProxyStatus.buffer())
            ClusterProxyStatus(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_PROXY_STATUS;
    }

    return d_clusterProxyStatus.object();
}
#endif

// ACCESSORS

bsl::ostream&
ClusterResult::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_SUCCESS: {
        printer.printAttribute("success", d_success.object());
    } break;
    case SELECTION_ID_ELECTOR_RESULT: {
        printer.printAttribute("electorResult", d_electorResult.object());
    } break;
    case SELECTION_ID_STORAGE_RESULT: {
        printer.printAttribute("storageResult", d_storageResult.object());
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        printer.printAttribute("clusterQueueHelper",
                               d_clusterQueueHelper.object());
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        printer.printAttribute("clusterStatus", d_clusterStatus.object());
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        printer.printAttribute("clusterProxyStatus",
                               d_clusterProxyStatus.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ClusterResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_SUCCESS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS].name();
    case SELECTION_ID_ELECTOR_RESULT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTOR_RESULT].name();
    case SELECTION_ID_STORAGE_RESULT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_RESULT].name();
    case SELECTION_ID_CLUSTER_QUEUE_HELPER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_QUEUE_HELPER]
            .name();
    case SELECTION_ID_CLUSTER_STATUS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STATUS].name();
    case SELECTION_ID_CLUSTER_PROXY_STATUS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_PROXY_STATUS]
            .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ---------------------
// class ClustersCommand
// ---------------------

// CONSTANTS

const char ClustersCommand::CLASS_NAME[] = "ClustersCommand";

const bdlat_SelectionInfo ClustersCommand::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_LIST,
     "list",
     sizeof("list") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ADD_REVERSE_PROXY,
     "addReverseProxy",
     sizeof("addReverseProxy") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER,
     "cluster",
     sizeof("cluster") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ClustersCommand::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ClustersCommand::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ClustersCommand::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_LIST: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST];
    case SELECTION_ID_ADD_REVERSE_PROXY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ADD_REVERSE_PROXY];
    case SELECTION_ID_CLUSTER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER];
    default: return 0;
    }
}

// CREATORS

ClustersCommand::ClustersCommand(const ClustersCommand& original,
                                 bslma::Allocator*      basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) Void(original.d_list.object());
    } break;
    case SELECTION_ID_ADD_REVERSE_PROXY: {
        new (d_addReverseProxy.buffer())
            AddReverseProxy(original.d_addReverseProxy.object(),
                            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER: {
        new (d_cluster.buffer())
            Cluster(original.d_cluster.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClustersCommand::ClustersCommand(ClustersCommand&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) Void(bsl::move(original.d_list.object()));
    } break;
    case SELECTION_ID_ADD_REVERSE_PROXY: {
        new (d_addReverseProxy.buffer())
            AddReverseProxy(bsl::move(original.d_addReverseProxy.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER: {
        new (d_cluster.buffer())
            Cluster(bsl::move(original.d_cluster.object()), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ClustersCommand::ClustersCommand(ClustersCommand&& original,
                                 bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_LIST: {
        new (d_list.buffer()) Void(bsl::move(original.d_list.object()));
    } break;
    case SELECTION_ID_ADD_REVERSE_PROXY: {
        new (d_addReverseProxy.buffer())
            AddReverseProxy(bsl::move(original.d_addReverseProxy.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER: {
        new (d_cluster.buffer())
            Cluster(bsl::move(original.d_cluster.object()), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ClustersCommand& ClustersCommand::operator=(const ClustersCommand& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_LIST: {
            makeList(rhs.d_list.object());
        } break;
        case SELECTION_ID_ADD_REVERSE_PROXY: {
            makeAddReverseProxy(rhs.d_addReverseProxy.object());
        } break;
        case SELECTION_ID_CLUSTER: {
            makeCluster(rhs.d_cluster.object());
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
ClustersCommand& ClustersCommand::operator=(ClustersCommand&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_LIST: {
            makeList(bsl::move(rhs.d_list.object()));
        } break;
        case SELECTION_ID_ADD_REVERSE_PROXY: {
            makeAddReverseProxy(bsl::move(rhs.d_addReverseProxy.object()));
        } break;
        case SELECTION_ID_CLUSTER: {
            makeCluster(bsl::move(rhs.d_cluster.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ClustersCommand::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_LIST: {
        d_list.object().~Void();
    } break;
    case SELECTION_ID_ADD_REVERSE_PROXY: {
        d_addReverseProxy.object().~AddReverseProxy();
    } break;
    case SELECTION_ID_CLUSTER: {
        d_cluster.object().~Cluster();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ClustersCommand::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_LIST: {
        makeList();
    } break;
    case SELECTION_ID_ADD_REVERSE_PROXY: {
        makeAddReverseProxy();
    } break;
    case SELECTION_ID_CLUSTER: {
        makeCluster();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ClustersCommand::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& ClustersCommand::makeList()
{
    if (SELECTION_ID_LIST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_list.object());
    }
    else {
        reset();
        new (d_list.buffer()) Void();
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

Void& ClustersCommand::makeList(const Void& value)
{
    if (SELECTION_ID_LIST == d_selectionId) {
        d_list.object() = value;
    }
    else {
        reset();
        new (d_list.buffer()) Void(value);
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& ClustersCommand::makeList(Void&& value)
{
    if (SELECTION_ID_LIST == d_selectionId) {
        d_list.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_list.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_LIST;
    }

    return d_list.object();
}
#endif

AddReverseProxy& ClustersCommand::makeAddReverseProxy()
{
    if (SELECTION_ID_ADD_REVERSE_PROXY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_addReverseProxy.object());
    }
    else {
        reset();
        new (d_addReverseProxy.buffer()) AddReverseProxy(d_allocator_p);
        d_selectionId = SELECTION_ID_ADD_REVERSE_PROXY;
    }

    return d_addReverseProxy.object();
}

AddReverseProxy&
ClustersCommand::makeAddReverseProxy(const AddReverseProxy& value)
{
    if (SELECTION_ID_ADD_REVERSE_PROXY == d_selectionId) {
        d_addReverseProxy.object() = value;
    }
    else {
        reset();
        new (d_addReverseProxy.buffer()) AddReverseProxy(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ADD_REVERSE_PROXY;
    }

    return d_addReverseProxy.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AddReverseProxy& ClustersCommand::makeAddReverseProxy(AddReverseProxy&& value)
{
    if (SELECTION_ID_ADD_REVERSE_PROXY == d_selectionId) {
        d_addReverseProxy.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_addReverseProxy.buffer())
            AddReverseProxy(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ADD_REVERSE_PROXY;
    }

    return d_addReverseProxy.object();
}
#endif

Cluster& ClustersCommand::makeCluster()
{
    if (SELECTION_ID_CLUSTER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_cluster.object());
    }
    else {
        reset();
        new (d_cluster.buffer()) Cluster(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER;
    }

    return d_cluster.object();
}

Cluster& ClustersCommand::makeCluster(const Cluster& value)
{
    if (SELECTION_ID_CLUSTER == d_selectionId) {
        d_cluster.object() = value;
    }
    else {
        reset();
        new (d_cluster.buffer()) Cluster(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER;
    }

    return d_cluster.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Cluster& ClustersCommand::makeCluster(Cluster&& value)
{
    if (SELECTION_ID_CLUSTER == d_selectionId) {
        d_cluster.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_cluster.buffer()) Cluster(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER;
    }

    return d_cluster.object();
}
#endif

// ACCESSORS

bsl::ostream& ClustersCommand::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_LIST: {
        printer.printAttribute("list", d_list.object());
    } break;
    case SELECTION_ID_ADD_REVERSE_PROXY: {
        printer.printAttribute("addReverseProxy", d_addReverseProxy.object());
    } break;
    case SELECTION_ID_CLUSTER: {
        printer.printAttribute("cluster", d_cluster.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ClustersCommand::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_LIST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST].name();
    case SELECTION_ID_ADD_REVERSE_PROXY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ADD_REVERSE_PROXY].name();
    case SELECTION_ID_CLUSTER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------------
// class ClustersResult
// --------------------

// CONSTANTS

const char ClustersResult::CLASS_NAME[] = "ClustersResult";

const bdlat_SelectionInfo ClustersResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SUCCESS,
     "success",
     sizeof("success") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_LIST,
     "clusterList",
     sizeof("clusterList") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_RESULT,
     "clusterResult",
     sizeof("clusterResult") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ClustersResult::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ClustersResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ClustersResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_SUCCESS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS];
    case SELECTION_ID_CLUSTER_LIST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_LIST];
    case SELECTION_ID_CLUSTER_RESULT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_RESULT];
    default: return 0;
    }
}

// CREATORS

ClustersResult::ClustersResult(const ClustersResult& original,
                               bslma::Allocator*     basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(original.d_success.object());
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        new (d_clusterList.buffer())
            ClusterList(original.d_clusterList.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_RESULT: {
        new (d_clusterResult.buffer())
            ClusterResult(original.d_clusterResult.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClustersResult::ClustersResult(ClustersResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        new (d_clusterList.buffer())
            ClusterList(bsl::move(original.d_clusterList.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_RESULT: {
        new (d_clusterResult.buffer())
            ClusterResult(bsl::move(original.d_clusterResult.object()),
                          d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ClustersResult::ClustersResult(ClustersResult&&  original,
                               bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        new (d_clusterList.buffer())
            ClusterList(bsl::move(original.d_clusterList.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_RESULT: {
        new (d_clusterResult.buffer())
            ClusterResult(bsl::move(original.d_clusterResult.object()),
                          d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ClustersResult& ClustersResult::operator=(const ClustersResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_SUCCESS: {
            makeSuccess(rhs.d_success.object());
        } break;
        case SELECTION_ID_CLUSTER_LIST: {
            makeClusterList(rhs.d_clusterList.object());
        } break;
        case SELECTION_ID_CLUSTER_RESULT: {
            makeClusterResult(rhs.d_clusterResult.object());
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
ClustersResult& ClustersResult::operator=(ClustersResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_SUCCESS: {
            makeSuccess(bsl::move(rhs.d_success.object()));
        } break;
        case SELECTION_ID_CLUSTER_LIST: {
            makeClusterList(bsl::move(rhs.d_clusterList.object()));
        } break;
        case SELECTION_ID_CLUSTER_RESULT: {
            makeClusterResult(bsl::move(rhs.d_clusterResult.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ClustersResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_SUCCESS: {
        d_success.object().~Void();
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        d_clusterList.object().~ClusterList();
    } break;
    case SELECTION_ID_CLUSTER_RESULT: {
        d_clusterResult.object().~ClusterResult();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ClustersResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_SUCCESS: {
        makeSuccess();
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        makeClusterList();
    } break;
    case SELECTION_ID_CLUSTER_RESULT: {
        makeClusterResult();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ClustersResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Error& ClustersResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& ClustersResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& ClustersResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

Void& ClustersResult::makeSuccess()
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_success.object());
    }
    else {
        reset();
        new (d_success.buffer()) Void();
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

Void& ClustersResult::makeSuccess(const Void& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = value;
    }
    else {
        reset();
        new (d_success.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& ClustersResult::makeSuccess(Void&& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_success.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}
#endif

ClusterList& ClustersResult::makeClusterList()
{
    if (SELECTION_ID_CLUSTER_LIST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterList.object());
    }
    else {
        reset();
        new (d_clusterList.buffer()) ClusterList(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_LIST;
    }

    return d_clusterList.object();
}

ClusterList& ClustersResult::makeClusterList(const ClusterList& value)
{
    if (SELECTION_ID_CLUSTER_LIST == d_selectionId) {
        d_clusterList.object() = value;
    }
    else {
        reset();
        new (d_clusterList.buffer()) ClusterList(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_LIST;
    }

    return d_clusterList.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterList& ClustersResult::makeClusterList(ClusterList&& value)
{
    if (SELECTION_ID_CLUSTER_LIST == d_selectionId) {
        d_clusterList.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterList.buffer())
            ClusterList(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_LIST;
    }

    return d_clusterList.object();
}
#endif

ClusterResult& ClustersResult::makeClusterResult()
{
    if (SELECTION_ID_CLUSTER_RESULT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterResult.object());
    }
    else {
        reset();
        new (d_clusterResult.buffer()) ClusterResult(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_RESULT;
    }

    return d_clusterResult.object();
}

ClusterResult& ClustersResult::makeClusterResult(const ClusterResult& value)
{
    if (SELECTION_ID_CLUSTER_RESULT == d_selectionId) {
        d_clusterResult.object() = value;
    }
    else {
        reset();
        new (d_clusterResult.buffer()) ClusterResult(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_RESULT;
    }

    return d_clusterResult.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterResult& ClustersResult::makeClusterResult(ClusterResult&& value)
{
    if (SELECTION_ID_CLUSTER_RESULT == d_selectionId) {
        d_clusterResult.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterResult.buffer())
            ClusterResult(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_RESULT;
    }

    return d_clusterResult.object();
}
#endif

// ACCESSORS

bsl::ostream& ClustersResult::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_SUCCESS: {
        printer.printAttribute("success", d_success.object());
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        printer.printAttribute("clusterList", d_clusterList.object());
    } break;
    case SELECTION_ID_CLUSTER_RESULT: {
        printer.printAttribute("clusterResult", d_clusterResult.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ClustersResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_SUCCESS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS].name();
    case SELECTION_ID_CLUSTER_LIST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_LIST].name();
    case SELECTION_ID_CLUSTER_RESULT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_RESULT].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -------------------
// class ConsumerState
// -------------------

// CONSTANTS

const char ConsumerState::CLASS_NAME[] = "ConsumerState";

const bdlat_AttributeInfo ConsumerState::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_STATUS,
     "status",
     sizeof("status") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_IS_AT_END_OF_STORAGE,
     "isAtEndOfStorage",
     sizeof("isAtEndOfStorage") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_APP_STATE,
     "appState",
     sizeof("appState") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* ConsumerState::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ConsumerState::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ConsumerState::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_STATUS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATUS];
    case ATTRIBUTE_ID_IS_AT_END_OF_STORAGE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_AT_END_OF_STORAGE];
    case ATTRIBUTE_ID_APP_STATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_STATE];
    default: return 0;
    }
}

// CREATORS

ConsumerState::ConsumerState(bslma::Allocator* basicAllocator)
: d_appId(basicAllocator)
, d_appState(basicAllocator)
, d_status(static_cast<ConsumerStatus::Value>(0))
, d_isAtEndOfStorage()
{
}

ConsumerState::ConsumerState(const ConsumerState& original,
                             bslma::Allocator*    basicAllocator)
: d_appId(original.d_appId, basicAllocator)
, d_appState(original.d_appState, basicAllocator)
, d_status(original.d_status)
, d_isAtEndOfStorage(original.d_isAtEndOfStorage)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConsumerState::ConsumerState(ConsumerState&& original) noexcept
: d_appId(bsl::move(original.d_appId)),
  d_appState(bsl::move(original.d_appState)),
  d_status(bsl::move(original.d_status)),
  d_isAtEndOfStorage(bsl::move(original.d_isAtEndOfStorage))
{
}

ConsumerState::ConsumerState(ConsumerState&&   original,
                             bslma::Allocator* basicAllocator)
: d_appId(bsl::move(original.d_appId), basicAllocator)
, d_appState(bsl::move(original.d_appState), basicAllocator)
, d_status(bsl::move(original.d_status))
, d_isAtEndOfStorage(bsl::move(original.d_isAtEndOfStorage))
{
}
#endif

ConsumerState::~ConsumerState()
{
}

// MANIPULATORS

ConsumerState& ConsumerState::operator=(const ConsumerState& rhs)
{
    if (this != &rhs) {
        d_appId            = rhs.d_appId;
        d_status           = rhs.d_status;
        d_isAtEndOfStorage = rhs.d_isAtEndOfStorage;
        d_appState         = rhs.d_appState;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConsumerState& ConsumerState::operator=(ConsumerState&& rhs)
{
    if (this != &rhs) {
        d_appId            = bsl::move(rhs.d_appId);
        d_status           = bsl::move(rhs.d_status);
        d_isAtEndOfStorage = bsl::move(rhs.d_isAtEndOfStorage);
        d_appState         = bsl::move(rhs.d_appState);
    }

    return *this;
}
#endif

void ConsumerState::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_appId);
    bdlat_ValueTypeFunctions::reset(&d_status);
    bdlat_ValueTypeFunctions::reset(&d_isAtEndOfStorage);
    bdlat_ValueTypeFunctions::reset(&d_appState);
}

// ACCESSORS

bsl::ostream&
ConsumerState::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("appId", this->appId());
    printer.printAttribute("status", this->status());
    printer.printAttribute("isAtEndOfStorage", this->isAtEndOfStorage());
    printer.printAttribute("appState", this->appState());
    printer.end();
    return stream;
}

// ----------------------
// class RelayQueueEngine
// ----------------------

// CONSTANTS

const char RelayQueueEngine::CLASS_NAME[] = "RelayQueueEngine";

const bdlat_AttributeInfo RelayQueueEngine::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NUM_SUBSTREAMS,
     "numSubstreams",
     sizeof("numSubstreams") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SUB_STREAMS,
     "subStreams",
     sizeof("subStreams") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_APP_STATES,
     "appStates",
     sizeof("appStates") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_ROUTING,
     "Routing",
     sizeof("Routing") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
RelayQueueEngine::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RelayQueueEngine::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RelayQueueEngine::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NUM_SUBSTREAMS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_SUBSTREAMS];
    case ATTRIBUTE_ID_SUB_STREAMS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUB_STREAMS];
    case ATTRIBUTE_ID_APP_STATES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_STATES];
    case ATTRIBUTE_ID_ROUTING:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROUTING];
    default: return 0;
    }
}

// CREATORS

RelayQueueEngine::RelayQueueEngine(bslma::Allocator* basicAllocator)
: d_subStreams(basicAllocator)
, d_appStates(basicAllocator)
, d_routing(basicAllocator)
, d_numSubstreams()
{
}

RelayQueueEngine::RelayQueueEngine(const RelayQueueEngine& original,
                                   bslma::Allocator*       basicAllocator)
: d_subStreams(original.d_subStreams, basicAllocator)
, d_appStates(original.d_appStates, basicAllocator)
, d_routing(original.d_routing, basicAllocator)
, d_numSubstreams(original.d_numSubstreams)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RelayQueueEngine::RelayQueueEngine(RelayQueueEngine&& original) noexcept
: d_subStreams(bsl::move(original.d_subStreams)),
  d_appStates(bsl::move(original.d_appStates)),
  d_routing(bsl::move(original.d_routing)),
  d_numSubstreams(bsl::move(original.d_numSubstreams))
{
}

RelayQueueEngine::RelayQueueEngine(RelayQueueEngine&& original,
                                   bslma::Allocator*  basicAllocator)
: d_subStreams(bsl::move(original.d_subStreams), basicAllocator)
, d_appStates(bsl::move(original.d_appStates), basicAllocator)
, d_routing(bsl::move(original.d_routing), basicAllocator)
, d_numSubstreams(bsl::move(original.d_numSubstreams))
{
}
#endif

RelayQueueEngine::~RelayQueueEngine()
{
}

// MANIPULATORS

RelayQueueEngine& RelayQueueEngine::operator=(const RelayQueueEngine& rhs)
{
    if (this != &rhs) {
        d_numSubstreams = rhs.d_numSubstreams;
        d_subStreams    = rhs.d_subStreams;
        d_appStates     = rhs.d_appStates;
        d_routing       = rhs.d_routing;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RelayQueueEngine& RelayQueueEngine::operator=(RelayQueueEngine&& rhs)
{
    if (this != &rhs) {
        d_numSubstreams = bsl::move(rhs.d_numSubstreams);
        d_subStreams    = bsl::move(rhs.d_subStreams);
        d_appStates     = bsl::move(rhs.d_appStates);
        d_routing       = bsl::move(rhs.d_routing);
    }

    return *this;
}
#endif

void RelayQueueEngine::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_numSubstreams);
    bdlat_ValueTypeFunctions::reset(&d_subStreams);
    bdlat_ValueTypeFunctions::reset(&d_appStates);
    bdlat_ValueTypeFunctions::reset(&d_routing);
}

// ACCESSORS

bsl::ostream& RelayQueueEngine::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("numSubstreams", this->numSubstreams());
    printer.printAttribute("subStreams", this->subStreams());
    printer.printAttribute("appStates", this->appStates());
    printer.printAttribute("routing", this->routing());
    printer.end();
    return stream;
}

// -------------------
// class CommandChoice
// -------------------

// CONSTANTS

const char CommandChoice::CLASS_NAME[] = "CommandChoice";

const bdlat_SelectionInfo CommandChoice::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_HELP,
     "help",
     sizeof("help") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DOMAINS,
     "domains",
     sizeof("domains") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONFIG_PROVIDER,
     "configProvider",
     sizeof("configProvider") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STAT,
     "stat",
     sizeof("stat") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTERS,
     "clusters",
     sizeof("clusters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DANGER,
     "danger",
     sizeof("danger") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_BROKER_CONFIG,
     "brokerConfig",
     sizeof("brokerConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* CommandChoice::lookupSelectionInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 7; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            CommandChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* CommandChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_HELP: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_HELP];
    case SELECTION_ID_DOMAINS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAINS];
    case SELECTION_ID_CONFIG_PROVIDER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIG_PROVIDER];
    case SELECTION_ID_STAT: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STAT];
    case SELECTION_ID_CLUSTERS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTERS];
    case SELECTION_ID_DANGER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DANGER];
    case SELECTION_ID_BROKER_CONFIG:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_BROKER_CONFIG];
    default: return 0;
    }
}

// CREATORS

CommandChoice::CommandChoice(const CommandChoice& original,
                             bslma::Allocator*    basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_HELP: {
        new (d_help.buffer()) HelpCommand(original.d_help.object());
    } break;
    case SELECTION_ID_DOMAINS: {
        new (d_domains.buffer())
            DomainsCommand(original.d_domains.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CONFIG_PROVIDER: {
        new (d_configProvider.buffer())
            ConfigProviderCommand(original.d_configProvider.object(),
                                  d_allocator_p);
    } break;
    case SELECTION_ID_STAT: {
        new (d_stat.buffer())
            StatCommand(original.d_stat.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTERS: {
        new (d_clusters.buffer())
            ClustersCommand(original.d_clusters.object(), d_allocator_p);
    } break;
    case SELECTION_ID_DANGER: {
        new (d_danger.buffer()) DangerCommand(original.d_danger.object());
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        new (d_brokerConfig.buffer())
            BrokerConfigCommand(original.d_brokerConfig.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CommandChoice::CommandChoice(CommandChoice&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_HELP: {
        new (d_help.buffer()) HelpCommand(bsl::move(original.d_help.object()));
    } break;
    case SELECTION_ID_DOMAINS: {
        new (d_domains.buffer())
            DomainsCommand(bsl::move(original.d_domains.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_CONFIG_PROVIDER: {
        new (d_configProvider.buffer()) ConfigProviderCommand(
            bsl::move(original.d_configProvider.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_STAT: {
        new (d_stat.buffer())
            StatCommand(bsl::move(original.d_stat.object()), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTERS: {
        new (d_clusters.buffer())
            ClustersCommand(bsl::move(original.d_clusters.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_DANGER: {
        new (d_danger.buffer())
            DangerCommand(bsl::move(original.d_danger.object()));
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        new (d_brokerConfig.buffer())
            BrokerConfigCommand(bsl::move(original.d_brokerConfig.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

CommandChoice::CommandChoice(CommandChoice&&   original,
                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_HELP: {
        new (d_help.buffer()) HelpCommand(bsl::move(original.d_help.object()));
    } break;
    case SELECTION_ID_DOMAINS: {
        new (d_domains.buffer())
            DomainsCommand(bsl::move(original.d_domains.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_CONFIG_PROVIDER: {
        new (d_configProvider.buffer()) ConfigProviderCommand(
            bsl::move(original.d_configProvider.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_STAT: {
        new (d_stat.buffer())
            StatCommand(bsl::move(original.d_stat.object()), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTERS: {
        new (d_clusters.buffer())
            ClustersCommand(bsl::move(original.d_clusters.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_DANGER: {
        new (d_danger.buffer())
            DangerCommand(bsl::move(original.d_danger.object()));
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        new (d_brokerConfig.buffer())
            BrokerConfigCommand(bsl::move(original.d_brokerConfig.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

CommandChoice& CommandChoice::operator=(const CommandChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_HELP: {
            makeHelp(rhs.d_help.object());
        } break;
        case SELECTION_ID_DOMAINS: {
            makeDomains(rhs.d_domains.object());
        } break;
        case SELECTION_ID_CONFIG_PROVIDER: {
            makeConfigProvider(rhs.d_configProvider.object());
        } break;
        case SELECTION_ID_STAT: {
            makeStat(rhs.d_stat.object());
        } break;
        case SELECTION_ID_CLUSTERS: {
            makeClusters(rhs.d_clusters.object());
        } break;
        case SELECTION_ID_DANGER: {
            makeDanger(rhs.d_danger.object());
        } break;
        case SELECTION_ID_BROKER_CONFIG: {
            makeBrokerConfig(rhs.d_brokerConfig.object());
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
CommandChoice& CommandChoice::operator=(CommandChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_HELP: {
            makeHelp(bsl::move(rhs.d_help.object()));
        } break;
        case SELECTION_ID_DOMAINS: {
            makeDomains(bsl::move(rhs.d_domains.object()));
        } break;
        case SELECTION_ID_CONFIG_PROVIDER: {
            makeConfigProvider(bsl::move(rhs.d_configProvider.object()));
        } break;
        case SELECTION_ID_STAT: {
            makeStat(bsl::move(rhs.d_stat.object()));
        } break;
        case SELECTION_ID_CLUSTERS: {
            makeClusters(bsl::move(rhs.d_clusters.object()));
        } break;
        case SELECTION_ID_DANGER: {
            makeDanger(bsl::move(rhs.d_danger.object()));
        } break;
        case SELECTION_ID_BROKER_CONFIG: {
            makeBrokerConfig(bsl::move(rhs.d_brokerConfig.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void CommandChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_HELP: {
        d_help.object().~HelpCommand();
    } break;
    case SELECTION_ID_DOMAINS: {
        d_domains.object().~DomainsCommand();
    } break;
    case SELECTION_ID_CONFIG_PROVIDER: {
        d_configProvider.object().~ConfigProviderCommand();
    } break;
    case SELECTION_ID_STAT: {
        d_stat.object().~StatCommand();
    } break;
    case SELECTION_ID_CLUSTERS: {
        d_clusters.object().~ClustersCommand();
    } break;
    case SELECTION_ID_DANGER: {
        d_danger.object().~DangerCommand();
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        d_brokerConfig.object().~BrokerConfigCommand();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int CommandChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_HELP: {
        makeHelp();
    } break;
    case SELECTION_ID_DOMAINS: {
        makeDomains();
    } break;
    case SELECTION_ID_CONFIG_PROVIDER: {
        makeConfigProvider();
    } break;
    case SELECTION_ID_STAT: {
        makeStat();
    } break;
    case SELECTION_ID_CLUSTERS: {
        makeClusters();
    } break;
    case SELECTION_ID_DANGER: {
        makeDanger();
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        makeBrokerConfig();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int CommandChoice::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

HelpCommand& CommandChoice::makeHelp()
{
    if (SELECTION_ID_HELP == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_help.object());
    }
    else {
        reset();
        new (d_help.buffer()) HelpCommand();
        d_selectionId = SELECTION_ID_HELP;
    }

    return d_help.object();
}

HelpCommand& CommandChoice::makeHelp(const HelpCommand& value)
{
    if (SELECTION_ID_HELP == d_selectionId) {
        d_help.object() = value;
    }
    else {
        reset();
        new (d_help.buffer()) HelpCommand(value);
        d_selectionId = SELECTION_ID_HELP;
    }

    return d_help.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
HelpCommand& CommandChoice::makeHelp(HelpCommand&& value)
{
    if (SELECTION_ID_HELP == d_selectionId) {
        d_help.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_help.buffer()) HelpCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_HELP;
    }

    return d_help.object();
}
#endif

DomainsCommand& CommandChoice::makeDomains()
{
    if (SELECTION_ID_DOMAINS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domains.object());
    }
    else {
        reset();
        new (d_domains.buffer()) DomainsCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAINS;
    }

    return d_domains.object();
}

DomainsCommand& CommandChoice::makeDomains(const DomainsCommand& value)
{
    if (SELECTION_ID_DOMAINS == d_selectionId) {
        d_domains.object() = value;
    }
    else {
        reset();
        new (d_domains.buffer()) DomainsCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAINS;
    }

    return d_domains.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainsCommand& CommandChoice::makeDomains(DomainsCommand&& value)
{
    if (SELECTION_ID_DOMAINS == d_selectionId) {
        d_domains.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domains.buffer())
            DomainsCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAINS;
    }

    return d_domains.object();
}
#endif

ConfigProviderCommand& CommandChoice::makeConfigProvider()
{
    if (SELECTION_ID_CONFIG_PROVIDER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_configProvider.object());
    }
    else {
        reset();
        new (d_configProvider.buffer()) ConfigProviderCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIG_PROVIDER;
    }

    return d_configProvider.object();
}

ConfigProviderCommand&
CommandChoice::makeConfigProvider(const ConfigProviderCommand& value)
{
    if (SELECTION_ID_CONFIG_PROVIDER == d_selectionId) {
        d_configProvider.object() = value;
    }
    else {
        reset();
        new (d_configProvider.buffer())
            ConfigProviderCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIG_PROVIDER;
    }

    return d_configProvider.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConfigProviderCommand&
CommandChoice::makeConfigProvider(ConfigProviderCommand&& value)
{
    if (SELECTION_ID_CONFIG_PROVIDER == d_selectionId) {
        d_configProvider.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_configProvider.buffer())
            ConfigProviderCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CONFIG_PROVIDER;
    }

    return d_configProvider.object();
}
#endif

StatCommand& CommandChoice::makeStat()
{
    if (SELECTION_ID_STAT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_stat.object());
    }
    else {
        reset();
        new (d_stat.buffer()) StatCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_STAT;
    }

    return d_stat.object();
}

StatCommand& CommandChoice::makeStat(const StatCommand& value)
{
    if (SELECTION_ID_STAT == d_selectionId) {
        d_stat.object() = value;
    }
    else {
        reset();
        new (d_stat.buffer()) StatCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STAT;
    }

    return d_stat.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatCommand& CommandChoice::makeStat(StatCommand&& value)
{
    if (SELECTION_ID_STAT == d_selectionId) {
        d_stat.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_stat.buffer()) StatCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STAT;
    }

    return d_stat.object();
}
#endif

ClustersCommand& CommandChoice::makeClusters()
{
    if (SELECTION_ID_CLUSTERS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusters.object());
    }
    else {
        reset();
        new (d_clusters.buffer()) ClustersCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTERS;
    }

    return d_clusters.object();
}

ClustersCommand& CommandChoice::makeClusters(const ClustersCommand& value)
{
    if (SELECTION_ID_CLUSTERS == d_selectionId) {
        d_clusters.object() = value;
    }
    else {
        reset();
        new (d_clusters.buffer()) ClustersCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTERS;
    }

    return d_clusters.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClustersCommand& CommandChoice::makeClusters(ClustersCommand&& value)
{
    if (SELECTION_ID_CLUSTERS == d_selectionId) {
        d_clusters.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusters.buffer())
            ClustersCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTERS;
    }

    return d_clusters.object();
}
#endif

DangerCommand& CommandChoice::makeDanger()
{
    if (SELECTION_ID_DANGER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_danger.object());
    }
    else {
        reset();
        new (d_danger.buffer()) DangerCommand();
        d_selectionId = SELECTION_ID_DANGER;
    }

    return d_danger.object();
}

DangerCommand& CommandChoice::makeDanger(const DangerCommand& value)
{
    if (SELECTION_ID_DANGER == d_selectionId) {
        d_danger.object() = value;
    }
    else {
        reset();
        new (d_danger.buffer()) DangerCommand(value);
        d_selectionId = SELECTION_ID_DANGER;
    }

    return d_danger.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DangerCommand& CommandChoice::makeDanger(DangerCommand&& value)
{
    if (SELECTION_ID_DANGER == d_selectionId) {
        d_danger.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_danger.buffer()) DangerCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_DANGER;
    }

    return d_danger.object();
}
#endif

BrokerConfigCommand& CommandChoice::makeBrokerConfig()
{
    if (SELECTION_ID_BROKER_CONFIG == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_brokerConfig.object());
    }
    else {
        reset();
        new (d_brokerConfig.buffer()) BrokerConfigCommand();
        d_selectionId = SELECTION_ID_BROKER_CONFIG;
    }

    return d_brokerConfig.object();
}

BrokerConfigCommand&
CommandChoice::makeBrokerConfig(const BrokerConfigCommand& value)
{
    if (SELECTION_ID_BROKER_CONFIG == d_selectionId) {
        d_brokerConfig.object() = value;
    }
    else {
        reset();
        new (d_brokerConfig.buffer()) BrokerConfigCommand(value);
        d_selectionId = SELECTION_ID_BROKER_CONFIG;
    }

    return d_brokerConfig.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerConfigCommand&
CommandChoice::makeBrokerConfig(BrokerConfigCommand&& value)
{
    if (SELECTION_ID_BROKER_CONFIG == d_selectionId) {
        d_brokerConfig.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_brokerConfig.buffer()) BrokerConfigCommand(bsl::move(value));
        d_selectionId = SELECTION_ID_BROKER_CONFIG;
    }

    return d_brokerConfig.object();
}
#endif

// ACCESSORS

bsl::ostream&
CommandChoice::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_HELP: {
        printer.printAttribute("help", d_help.object());
    } break;
    case SELECTION_ID_DOMAINS: {
        printer.printAttribute("domains", d_domains.object());
    } break;
    case SELECTION_ID_CONFIG_PROVIDER: {
        printer.printAttribute("configProvider", d_configProvider.object());
    } break;
    case SELECTION_ID_STAT: {
        printer.printAttribute("stat", d_stat.object());
    } break;
    case SELECTION_ID_CLUSTERS: {
        printer.printAttribute("clusters", d_clusters.object());
    } break;
    case SELECTION_ID_DANGER: {
        printer.printAttribute("danger", d_danger.object());
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        printer.printAttribute("brokerConfig", d_brokerConfig.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* CommandChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_HELP:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_HELP].name();
    case SELECTION_ID_DOMAINS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAINS].name();
    case SELECTION_ID_CONFIG_PROVIDER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIG_PROVIDER].name();
    case SELECTION_ID_STAT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STAT].name();
    case SELECTION_ID_CLUSTERS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTERS].name();
    case SELECTION_ID_DANGER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DANGER].name();
    case SELECTION_ID_BROKER_CONFIG:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_BROKER_CONFIG].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------------
// class FanoutQueueEngine
// -----------------------

// CONSTANTS

const char FanoutQueueEngine::CLASS_NAME[] = "FanoutQueueEngine";

const bdlat_AttributeInfo FanoutQueueEngine::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_MAX_CONSUMERS,
     "maxConsumers",
     sizeof("maxConsumers") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MODE,
     "mode",
     sizeof("mode") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CONSUMER_STATES,
     "consumerStates",
     sizeof("consumerStates") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_ROUTING,
     "Routing",
     sizeof("Routing") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
FanoutQueueEngine::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            FanoutQueueEngine::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* FanoutQueueEngine::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MAX_CONSUMERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONSUMERS];
    case ATTRIBUTE_ID_MODE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE];
    case ATTRIBUTE_ID_CONSUMER_STATES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_STATES];
    case ATTRIBUTE_ID_ROUTING:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROUTING];
    default: return 0;
    }
}

// CREATORS

FanoutQueueEngine::FanoutQueueEngine(bslma::Allocator* basicAllocator)
: d_consumerStates(basicAllocator)
, d_mode(basicAllocator)
, d_routing(basicAllocator)
, d_maxConsumers()
{
}

FanoutQueueEngine::FanoutQueueEngine(const FanoutQueueEngine& original,
                                     bslma::Allocator*        basicAllocator)
: d_consumerStates(original.d_consumerStates, basicAllocator)
, d_mode(original.d_mode, basicAllocator)
, d_routing(original.d_routing, basicAllocator)
, d_maxConsumers(original.d_maxConsumers)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FanoutQueueEngine::FanoutQueueEngine(FanoutQueueEngine&& original) noexcept
: d_consumerStates(bsl::move(original.d_consumerStates)),
  d_mode(bsl::move(original.d_mode)),
  d_routing(bsl::move(original.d_routing)),
  d_maxConsumers(bsl::move(original.d_maxConsumers))
{
}

FanoutQueueEngine::FanoutQueueEngine(FanoutQueueEngine&& original,
                                     bslma::Allocator*   basicAllocator)
: d_consumerStates(bsl::move(original.d_consumerStates), basicAllocator)
, d_mode(bsl::move(original.d_mode), basicAllocator)
, d_routing(bsl::move(original.d_routing), basicAllocator)
, d_maxConsumers(bsl::move(original.d_maxConsumers))
{
}
#endif

FanoutQueueEngine::~FanoutQueueEngine()
{
}

// MANIPULATORS

FanoutQueueEngine& FanoutQueueEngine::operator=(const FanoutQueueEngine& rhs)
{
    if (this != &rhs) {
        d_maxConsumers   = rhs.d_maxConsumers;
        d_mode           = rhs.d_mode;
        d_consumerStates = rhs.d_consumerStates;
        d_routing        = rhs.d_routing;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FanoutQueueEngine& FanoutQueueEngine::operator=(FanoutQueueEngine&& rhs)
{
    if (this != &rhs) {
        d_maxConsumers   = bsl::move(rhs.d_maxConsumers);
        d_mode           = bsl::move(rhs.d_mode);
        d_consumerStates = bsl::move(rhs.d_consumerStates);
        d_routing        = bsl::move(rhs.d_routing);
    }

    return *this;
}
#endif

void FanoutQueueEngine::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_maxConsumers);
    bdlat_ValueTypeFunctions::reset(&d_mode);
    bdlat_ValueTypeFunctions::reset(&d_consumerStates);
    bdlat_ValueTypeFunctions::reset(&d_routing);
}

// ACCESSORS

bsl::ostream& FanoutQueueEngine::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("maxConsumers", this->maxConsumers());
    printer.printAttribute("mode", this->mode());
    printer.printAttribute("consumerStates", this->consumerStates());
    printer.printAttribute("routing", this->routing());
    printer.end();
    return stream;
}

// -------------
// class Command
// -------------

// CONSTANTS

const char Command::CLASS_NAME[] = "Command";

const EncodingFormat::Value Command::DEFAULT_INITIALIZER_ENCODING =
    EncodingFormat::TEXT;

const bdlat_AttributeInfo Command::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED},
    {ATTRIBUTE_ID_ENCODING,
     "encoding",
     sizeof("encoding") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Command::lookupAttributeInfo(const char* name,
                                                        int         nameLength)
{
    if (bdlb::String::areEqualCaseless("help", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("domains", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("configProvider", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("stat", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("clusters", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("danger", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("brokerConfig", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Command::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Command::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    case ATTRIBUTE_ID_ENCODING:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENCODING];
    default: return 0;
    }
}

// CREATORS

Command::Command(bslma::Allocator* basicAllocator)
: d_choice(basicAllocator)
, d_encoding(DEFAULT_INITIALIZER_ENCODING)
{
}

Command::Command(const Command& original, bslma::Allocator* basicAllocator)
: d_choice(original.d_choice, basicAllocator)
, d_encoding(original.d_encoding)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Command::Command(Command&& original) noexcept
: d_choice(bsl::move(original.d_choice)),
  d_encoding(bsl::move(original.d_encoding))
{
}

Command::Command(Command&& original, bslma::Allocator* basicAllocator)
: d_choice(bsl::move(original.d_choice), basicAllocator)
, d_encoding(bsl::move(original.d_encoding))
{
}
#endif

Command::~Command()
{
}

// MANIPULATORS

Command& Command::operator=(const Command& rhs)
{
    if (this != &rhs) {
        d_choice   = rhs.d_choice;
        d_encoding = rhs.d_encoding;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Command& Command::operator=(Command&& rhs)
{
    if (this != &rhs) {
        d_choice   = bsl::move(rhs.d_choice);
        d_encoding = bsl::move(rhs.d_encoding);
    }

    return *this;
}
#endif

void Command::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_choice);
    d_encoding = DEFAULT_INITIALIZER_ENCODING;
}

// ACCESSORS

bsl::ostream&
Command::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("choice", this->choice());
    printer.printAttribute("encoding", this->encoding());
    printer.end();
    return stream;
}

// -----------------
// class QueueEngine
// -----------------

// CONSTANTS

const char QueueEngine::CLASS_NAME[] = "QueueEngine";

const bdlat_SelectionInfo QueueEngine::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_FANOUT,
     "fanout",
     sizeof("fanout") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_RELAY,
     "relay",
     sizeof("relay") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* QueueEngine::lookupSelectionInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            QueueEngine::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* QueueEngine::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_FANOUT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_FANOUT];
    case SELECTION_ID_RELAY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_RELAY];
    default: return 0;
    }
}

// CREATORS

QueueEngine::QueueEngine(const QueueEngine& original,
                         bslma::Allocator*  basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        new (d_fanout.buffer())
            FanoutQueueEngine(original.d_fanout.object(), d_allocator_p);
    } break;
    case SELECTION_ID_RELAY: {
        new (d_relay.buffer())
            RelayQueueEngine(original.d_relay.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueEngine::QueueEngine(QueueEngine&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        new (d_fanout.buffer())
            FanoutQueueEngine(bsl::move(original.d_fanout.object()),
                              d_allocator_p);
    } break;
    case SELECTION_ID_RELAY: {
        new (d_relay.buffer())
            RelayQueueEngine(bsl::move(original.d_relay.object()),
                             d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

QueueEngine::QueueEngine(QueueEngine&&     original,
                         bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        new (d_fanout.buffer())
            FanoutQueueEngine(bsl::move(original.d_fanout.object()),
                              d_allocator_p);
    } break;
    case SELECTION_ID_RELAY: {
        new (d_relay.buffer())
            RelayQueueEngine(bsl::move(original.d_relay.object()),
                             d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

QueueEngine& QueueEngine::operator=(const QueueEngine& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_FANOUT: {
            makeFanout(rhs.d_fanout.object());
        } break;
        case SELECTION_ID_RELAY: {
            makeRelay(rhs.d_relay.object());
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
QueueEngine& QueueEngine::operator=(QueueEngine&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_FANOUT: {
            makeFanout(bsl::move(rhs.d_fanout.object()));
        } break;
        case SELECTION_ID_RELAY: {
            makeRelay(bsl::move(rhs.d_relay.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void QueueEngine::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        d_fanout.object().~FanoutQueueEngine();
    } break;
    case SELECTION_ID_RELAY: {
        d_relay.object().~RelayQueueEngine();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int QueueEngine::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_FANOUT: {
        makeFanout();
    } break;
    case SELECTION_ID_RELAY: {
        makeRelay();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int QueueEngine::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

FanoutQueueEngine& QueueEngine::makeFanout()
{
    if (SELECTION_ID_FANOUT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_fanout.object());
    }
    else {
        reset();
        new (d_fanout.buffer()) FanoutQueueEngine(d_allocator_p);
        d_selectionId = SELECTION_ID_FANOUT;
    }

    return d_fanout.object();
}

FanoutQueueEngine& QueueEngine::makeFanout(const FanoutQueueEngine& value)
{
    if (SELECTION_ID_FANOUT == d_selectionId) {
        d_fanout.object() = value;
    }
    else {
        reset();
        new (d_fanout.buffer()) FanoutQueueEngine(value, d_allocator_p);
        d_selectionId = SELECTION_ID_FANOUT;
    }

    return d_fanout.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FanoutQueueEngine& QueueEngine::makeFanout(FanoutQueueEngine&& value)
{
    if (SELECTION_ID_FANOUT == d_selectionId) {
        d_fanout.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_fanout.buffer())
            FanoutQueueEngine(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_FANOUT;
    }

    return d_fanout.object();
}
#endif

RelayQueueEngine& QueueEngine::makeRelay()
{
    if (SELECTION_ID_RELAY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_relay.object());
    }
    else {
        reset();
        new (d_relay.buffer()) RelayQueueEngine(d_allocator_p);
        d_selectionId = SELECTION_ID_RELAY;
    }

    return d_relay.object();
}

RelayQueueEngine& QueueEngine::makeRelay(const RelayQueueEngine& value)
{
    if (SELECTION_ID_RELAY == d_selectionId) {
        d_relay.object() = value;
    }
    else {
        reset();
        new (d_relay.buffer()) RelayQueueEngine(value, d_allocator_p);
        d_selectionId = SELECTION_ID_RELAY;
    }

    return d_relay.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RelayQueueEngine& QueueEngine::makeRelay(RelayQueueEngine&& value)
{
    if (SELECTION_ID_RELAY == d_selectionId) {
        d_relay.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_relay.buffer())
            RelayQueueEngine(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_RELAY;
    }

    return d_relay.object();
}
#endif

// ACCESSORS

bsl::ostream&
QueueEngine::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        printer.printAttribute("fanout", d_fanout.object());
    } break;
    case SELECTION_ID_RELAY: {
        printer.printAttribute("relay", d_relay.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* QueueEngine::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_FANOUT].name();
    case SELECTION_ID_RELAY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_RELAY].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ----------------
// class LocalQueue
// ----------------

// CONSTANTS

const char LocalQueue::CLASS_NAME[] = "LocalQueue";

const bdlat_AttributeInfo LocalQueue::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_QUEUE_ENGINE,
     "queueEngine",
     sizeof("queueEngine") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* LocalQueue::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LocalQueue::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* LocalQueue::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_QUEUE_ENGINE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_ENGINE];
    default: return 0;
    }
}

// CREATORS

LocalQueue::LocalQueue(bslma::Allocator* basicAllocator)
: d_queueEngine(basicAllocator)
{
}

LocalQueue::LocalQueue(const LocalQueue& original,
                       bslma::Allocator* basicAllocator)
: d_queueEngine(original.d_queueEngine, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LocalQueue::LocalQueue(LocalQueue&& original) noexcept
: d_queueEngine(bsl::move(original.d_queueEngine))
{
}

LocalQueue::LocalQueue(LocalQueue&& original, bslma::Allocator* basicAllocator)
: d_queueEngine(bsl::move(original.d_queueEngine), basicAllocator)
{
}
#endif

LocalQueue::~LocalQueue()
{
}

// MANIPULATORS

LocalQueue& LocalQueue::operator=(const LocalQueue& rhs)
{
    if (this != &rhs) {
        d_queueEngine = rhs.d_queueEngine;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LocalQueue& LocalQueue::operator=(LocalQueue&& rhs)
{
    if (this != &rhs) {
        d_queueEngine = bsl::move(rhs.d_queueEngine);
    }

    return *this;
}
#endif

void LocalQueue::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_queueEngine);
}

// ACCESSORS

bsl::ostream&
LocalQueue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queueEngine", this->queueEngine());
    printer.end();
    return stream;
}

// -----------------
// class RemoteQueue
// -----------------

// CONSTANTS

const char RemoteQueue::CLASS_NAME[] = "RemoteQueue";

const bdlat_AttributeInfo RemoteQueue::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NUM_PENDING_PUTS,
     "numPendingPuts",
     sizeof("numPendingPuts") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NUM_PENDING_CONFIRMS,
     "numPendingConfirms",
     sizeof("numPendingConfirms") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_IS_PUSH_EXPIRATION_TIMER_SCHEDULED,
     "isPushExpirationTimerScheduled",
     sizeof("isPushExpirationTimerScheduled") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NUM_UPSTREAM_GENERATION,
     "numUpstreamGeneration",
     sizeof("numUpstreamGeneration") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STREAMS,
     "streams",
     sizeof("streams") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUE_ENGINE,
     "queueEngine",
     sizeof("queueEngine") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* RemoteQueue::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 6; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RemoteQueue::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RemoteQueue::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NUM_PENDING_PUTS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PENDING_PUTS];
    case ATTRIBUTE_ID_NUM_PENDING_CONFIRMS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PENDING_CONFIRMS];
    case ATTRIBUTE_ID_IS_PUSH_EXPIRATION_TIMER_SCHEDULED:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_IS_PUSH_EXPIRATION_TIMER_SCHEDULED];
    case ATTRIBUTE_ID_NUM_UPSTREAM_GENERATION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_UPSTREAM_GENERATION];
    case ATTRIBUTE_ID_STREAMS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STREAMS];
    case ATTRIBUTE_ID_QUEUE_ENGINE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_ENGINE];
    default: return 0;
    }
}

// CREATORS

RemoteQueue::RemoteQueue(bslma::Allocator* basicAllocator)
: d_numPendingPuts()
, d_numPendingConfirms()
, d_streams(basicAllocator)
, d_queueEngine(basicAllocator)
, d_numUpstreamGeneration()
, d_isPushExpirationTimerScheduled()
{
}

RemoteQueue::RemoteQueue(const RemoteQueue& original,
                         bslma::Allocator*  basicAllocator)
: d_numPendingPuts(original.d_numPendingPuts)
, d_numPendingConfirms(original.d_numPendingConfirms)
, d_streams(original.d_streams, basicAllocator)
, d_queueEngine(original.d_queueEngine, basicAllocator)
, d_numUpstreamGeneration(original.d_numUpstreamGeneration)
, d_isPushExpirationTimerScheduled(original.d_isPushExpirationTimerScheduled)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RemoteQueue::RemoteQueue(RemoteQueue&& original) noexcept
: d_numPendingPuts(bsl::move(original.d_numPendingPuts)),
  d_numPendingConfirms(bsl::move(original.d_numPendingConfirms)),
  d_streams(bsl::move(original.d_streams)),
  d_queueEngine(bsl::move(original.d_queueEngine)),
  d_numUpstreamGeneration(bsl::move(original.d_numUpstreamGeneration)),
  d_isPushExpirationTimerScheduled(
      bsl::move(original.d_isPushExpirationTimerScheduled))
{
}

RemoteQueue::RemoteQueue(RemoteQueue&&     original,
                         bslma::Allocator* basicAllocator)
: d_numPendingPuts(bsl::move(original.d_numPendingPuts))
, d_numPendingConfirms(bsl::move(original.d_numPendingConfirms))
, d_streams(bsl::move(original.d_streams), basicAllocator)
, d_queueEngine(bsl::move(original.d_queueEngine), basicAllocator)
, d_numUpstreamGeneration(bsl::move(original.d_numUpstreamGeneration))
, d_isPushExpirationTimerScheduled(
      bsl::move(original.d_isPushExpirationTimerScheduled))
{
}
#endif

RemoteQueue::~RemoteQueue()
{
}

// MANIPULATORS

RemoteQueue& RemoteQueue::operator=(const RemoteQueue& rhs)
{
    if (this != &rhs) {
        d_numPendingPuts     = rhs.d_numPendingPuts;
        d_numPendingConfirms = rhs.d_numPendingConfirms;
        d_isPushExpirationTimerScheduled =
            rhs.d_isPushExpirationTimerScheduled;
        d_numUpstreamGeneration = rhs.d_numUpstreamGeneration;
        d_streams               = rhs.d_streams;
        d_queueEngine           = rhs.d_queueEngine;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RemoteQueue& RemoteQueue::operator=(RemoteQueue&& rhs)
{
    if (this != &rhs) {
        d_numPendingPuts                 = bsl::move(rhs.d_numPendingPuts);
        d_numPendingConfirms             = bsl::move(rhs.d_numPendingConfirms);
        d_isPushExpirationTimerScheduled = bsl::move(
            rhs.d_isPushExpirationTimerScheduled);
        d_numUpstreamGeneration = bsl::move(rhs.d_numUpstreamGeneration);
        d_streams               = bsl::move(rhs.d_streams);
        d_queueEngine           = bsl::move(rhs.d_queueEngine);
    }

    return *this;
}
#endif

void RemoteQueue::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_numPendingPuts);
    bdlat_ValueTypeFunctions::reset(&d_numPendingConfirms);
    bdlat_ValueTypeFunctions::reset(&d_isPushExpirationTimerScheduled);
    bdlat_ValueTypeFunctions::reset(&d_numUpstreamGeneration);
    bdlat_ValueTypeFunctions::reset(&d_streams);
    bdlat_ValueTypeFunctions::reset(&d_queueEngine);
}

// ACCESSORS

bsl::ostream&
RemoteQueue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("numPendingPuts", this->numPendingPuts());
    printer.printAttribute("numPendingConfirms", this->numPendingConfirms());
    printer.printAttribute("isPushExpirationTimerScheduled",
                           this->isPushExpirationTimerScheduled());
    printer.printAttribute("numUpstreamGeneration",
                           this->numUpstreamGeneration());
    printer.printAttribute("streams", this->streams());
    printer.printAttribute("queueEngine", this->queueEngine());
    printer.end();
    return stream;
}

// -----------
// class Queue
// -----------

// CONSTANTS

const char Queue::CLASS_NAME[] = "Queue";

const bdlat_SelectionInfo Queue::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_LOCAL_QUEUE,
     "localQueue",
     sizeof("localQueue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_REMOTE_QUEUE,
     "remoteQueue",
     sizeof("remoteQueue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_UNINITIALIZED_QUEUE,
     "uninitializedQueue",
     sizeof("uninitializedQueue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* Queue::lookupSelectionInfo(const char* name,
                                                      int         nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            Queue::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* Queue::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_LOCAL_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_LOCAL_QUEUE];
    case SELECTION_ID_REMOTE_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REMOTE_QUEUE];
    case SELECTION_ID_UNINITIALIZED_QUEUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_UNINITIALIZED_QUEUE];
    default: return 0;
    }
}

// CREATORS

Queue::Queue(const Queue& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_LOCAL_QUEUE: {
        new (d_localQueue.buffer())
            LocalQueue(original.d_localQueue.object(), d_allocator_p);
    } break;
    case SELECTION_ID_REMOTE_QUEUE: {
        new (d_remoteQueue.buffer())
            RemoteQueue(original.d_remoteQueue.object(), d_allocator_p);
    } break;
    case SELECTION_ID_UNINITIALIZED_QUEUE: {
        new (d_uninitializedQueue.buffer())
            UninitializedQueue(original.d_uninitializedQueue.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Queue::Queue(Queue&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_LOCAL_QUEUE: {
        new (d_localQueue.buffer())
            LocalQueue(bsl::move(original.d_localQueue.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_REMOTE_QUEUE: {
        new (d_remoteQueue.buffer())
            RemoteQueue(bsl::move(original.d_remoteQueue.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_UNINITIALIZED_QUEUE: {
        new (d_uninitializedQueue.buffer()) UninitializedQueue(
            bsl::move(original.d_uninitializedQueue.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

Queue::Queue(Queue&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_LOCAL_QUEUE: {
        new (d_localQueue.buffer())
            LocalQueue(bsl::move(original.d_localQueue.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_REMOTE_QUEUE: {
        new (d_remoteQueue.buffer())
            RemoteQueue(bsl::move(original.d_remoteQueue.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_UNINITIALIZED_QUEUE: {
        new (d_uninitializedQueue.buffer()) UninitializedQueue(
            bsl::move(original.d_uninitializedQueue.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

Queue& Queue::operator=(const Queue& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_LOCAL_QUEUE: {
            makeLocalQueue(rhs.d_localQueue.object());
        } break;
        case SELECTION_ID_REMOTE_QUEUE: {
            makeRemoteQueue(rhs.d_remoteQueue.object());
        } break;
        case SELECTION_ID_UNINITIALIZED_QUEUE: {
            makeUninitializedQueue(rhs.d_uninitializedQueue.object());
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
Queue& Queue::operator=(Queue&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_LOCAL_QUEUE: {
            makeLocalQueue(bsl::move(rhs.d_localQueue.object()));
        } break;
        case SELECTION_ID_REMOTE_QUEUE: {
            makeRemoteQueue(bsl::move(rhs.d_remoteQueue.object()));
        } break;
        case SELECTION_ID_UNINITIALIZED_QUEUE: {
            makeUninitializedQueue(
                bsl::move(rhs.d_uninitializedQueue.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void Queue::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_LOCAL_QUEUE: {
        d_localQueue.object().~LocalQueue();
    } break;
    case SELECTION_ID_REMOTE_QUEUE: {
        d_remoteQueue.object().~RemoteQueue();
    } break;
    case SELECTION_ID_UNINITIALIZED_QUEUE: {
        d_uninitializedQueue.object().~UninitializedQueue();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int Queue::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_LOCAL_QUEUE: {
        makeLocalQueue();
    } break;
    case SELECTION_ID_REMOTE_QUEUE: {
        makeRemoteQueue();
    } break;
    case SELECTION_ID_UNINITIALIZED_QUEUE: {
        makeUninitializedQueue();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int Queue::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

LocalQueue& Queue::makeLocalQueue()
{
    if (SELECTION_ID_LOCAL_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_localQueue.object());
    }
    else {
        reset();
        new (d_localQueue.buffer()) LocalQueue(d_allocator_p);
        d_selectionId = SELECTION_ID_LOCAL_QUEUE;
    }

    return d_localQueue.object();
}

LocalQueue& Queue::makeLocalQueue(const LocalQueue& value)
{
    if (SELECTION_ID_LOCAL_QUEUE == d_selectionId) {
        d_localQueue.object() = value;
    }
    else {
        reset();
        new (d_localQueue.buffer()) LocalQueue(value, d_allocator_p);
        d_selectionId = SELECTION_ID_LOCAL_QUEUE;
    }

    return d_localQueue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LocalQueue& Queue::makeLocalQueue(LocalQueue&& value)
{
    if (SELECTION_ID_LOCAL_QUEUE == d_selectionId) {
        d_localQueue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_localQueue.buffer())
            LocalQueue(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_LOCAL_QUEUE;
    }

    return d_localQueue.object();
}
#endif

RemoteQueue& Queue::makeRemoteQueue()
{
    if (SELECTION_ID_REMOTE_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_remoteQueue.object());
    }
    else {
        reset();
        new (d_remoteQueue.buffer()) RemoteQueue(d_allocator_p);
        d_selectionId = SELECTION_ID_REMOTE_QUEUE;
    }

    return d_remoteQueue.object();
}

RemoteQueue& Queue::makeRemoteQueue(const RemoteQueue& value)
{
    if (SELECTION_ID_REMOTE_QUEUE == d_selectionId) {
        d_remoteQueue.object() = value;
    }
    else {
        reset();
        new (d_remoteQueue.buffer()) RemoteQueue(value, d_allocator_p);
        d_selectionId = SELECTION_ID_REMOTE_QUEUE;
    }

    return d_remoteQueue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RemoteQueue& Queue::makeRemoteQueue(RemoteQueue&& value)
{
    if (SELECTION_ID_REMOTE_QUEUE == d_selectionId) {
        d_remoteQueue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_remoteQueue.buffer())
            RemoteQueue(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_REMOTE_QUEUE;
    }

    return d_remoteQueue.object();
}
#endif

UninitializedQueue& Queue::makeUninitializedQueue()
{
    if (SELECTION_ID_UNINITIALIZED_QUEUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_uninitializedQueue.object());
    }
    else {
        reset();
        new (d_uninitializedQueue.buffer()) UninitializedQueue();
        d_selectionId = SELECTION_ID_UNINITIALIZED_QUEUE;
    }

    return d_uninitializedQueue.object();
}

UninitializedQueue&
Queue::makeUninitializedQueue(const UninitializedQueue& value)
{
    if (SELECTION_ID_UNINITIALIZED_QUEUE == d_selectionId) {
        d_uninitializedQueue.object() = value;
    }
    else {
        reset();
        new (d_uninitializedQueue.buffer()) UninitializedQueue(value);
        d_selectionId = SELECTION_ID_UNINITIALIZED_QUEUE;
    }

    return d_uninitializedQueue.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
UninitializedQueue& Queue::makeUninitializedQueue(UninitializedQueue&& value)
{
    if (SELECTION_ID_UNINITIALIZED_QUEUE == d_selectionId) {
        d_uninitializedQueue.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_uninitializedQueue.buffer())
            UninitializedQueue(bsl::move(value));
        d_selectionId = SELECTION_ID_UNINITIALIZED_QUEUE;
    }

    return d_uninitializedQueue.object();
}
#endif

// ACCESSORS

bsl::ostream&
Queue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_LOCAL_QUEUE: {
        printer.printAttribute("localQueue", d_localQueue.object());
    } break;
    case SELECTION_ID_REMOTE_QUEUE: {
        printer.printAttribute("remoteQueue", d_remoteQueue.object());
    } break;
    case SELECTION_ID_UNINITIALIZED_QUEUE: {
        printer.printAttribute("uninitializedQueue",
                               d_uninitializedQueue.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* Queue::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_LOCAL_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_LOCAL_QUEUE].name();
    case SELECTION_ID_REMOTE_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REMOTE_QUEUE].name();
    case SELECTION_ID_UNINITIALIZED_QUEUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_UNINITIALIZED_QUEUE]
            .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------------
// class QueueInternals
// --------------------

// CONSTANTS

const char QueueInternals::CLASS_NAME[] = "QueueInternals";

const bdlat_AttributeInfo QueueInternals::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_STATE,
     "state",
     sizeof("state") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUE,
     "queue",
     sizeof("queue") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueInternals::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueInternals::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueInternals::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_STATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATE];
    case ATTRIBUTE_ID_QUEUE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE];
    default: return 0;
    }
}

// CREATORS

QueueInternals::QueueInternals(bslma::Allocator* basicAllocator)
: d_state(basicAllocator)
, d_queue(basicAllocator)
{
}

QueueInternals::QueueInternals(const QueueInternals& original,
                               bslma::Allocator*     basicAllocator)
: d_state(original.d_state, basicAllocator)
, d_queue(original.d_queue, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueInternals::QueueInternals(QueueInternals&& original) noexcept
: d_state(bsl::move(original.d_state)),
  d_queue(bsl::move(original.d_queue))
{
}

QueueInternals::QueueInternals(QueueInternals&&  original,
                               bslma::Allocator* basicAllocator)
: d_state(bsl::move(original.d_state), basicAllocator)
, d_queue(bsl::move(original.d_queue), basicAllocator)
{
}
#endif

QueueInternals::~QueueInternals()
{
}

// MANIPULATORS

QueueInternals& QueueInternals::operator=(const QueueInternals& rhs)
{
    if (this != &rhs) {
        d_state = rhs.d_state;
        d_queue = rhs.d_queue;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueInternals& QueueInternals::operator=(QueueInternals&& rhs)
{
    if (this != &rhs) {
        d_state = bsl::move(rhs.d_state);
        d_queue = bsl::move(rhs.d_queue);
    }

    return *this;
}
#endif

void QueueInternals::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_state);
    bdlat_ValueTypeFunctions::reset(&d_queue);
}

// ACCESSORS

bsl::ostream& QueueInternals::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("state", this->state());
    printer.printAttribute("queue", this->queue());
    printer.end();
    return stream;
}

// -----------------
// class QueueResult
// -----------------

// CONSTANTS

const char QueueResult::CLASS_NAME[] = "QueueResult";

const bdlat_SelectionInfo QueueResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PURGED_QUEUES,
     "purgedQueues",
     sizeof("purgedQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_CONTENTS,
     "queueContents",
     sizeof("queueContents") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_INTERNALS,
     "queueInternals",
     sizeof("queueInternals") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* QueueResult::lookupSelectionInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            QueueResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* QueueResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_PURGED_QUEUES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGED_QUEUES];
    case SELECTION_ID_QUEUE_CONTENTS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_CONTENTS];
    case SELECTION_ID_QUEUE_INTERNALS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_INTERNALS];
    default: return 0;
    }
}

// CREATORS

QueueResult::QueueResult(const QueueResult& original,
                         bslma::Allocator*  basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(original.d_purgedQueues.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        new (d_queueContents.buffer())
            QueueContents(original.d_queueContents.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        new (d_queueInternals.buffer())
            QueueInternals(original.d_queueInternals.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueResult::QueueResult(QueueResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(original.d_purgedQueues.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        new (d_queueContents.buffer())
            QueueContents(bsl::move(original.d_queueContents.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        new (d_queueInternals.buffer())
            QueueInternals(bsl::move(original.d_queueInternals.object()),
                           d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

QueueResult::QueueResult(QueueResult&&     original,
                         bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(original.d_purgedQueues.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        new (d_queueContents.buffer())
            QueueContents(bsl::move(original.d_queueContents.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        new (d_queueInternals.buffer())
            QueueInternals(bsl::move(original.d_queueInternals.object()),
                           d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

QueueResult& QueueResult::operator=(const QueueResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_PURGED_QUEUES: {
            makePurgedQueues(rhs.d_purgedQueues.object());
        } break;
        case SELECTION_ID_QUEUE_CONTENTS: {
            makeQueueContents(rhs.d_queueContents.object());
        } break;
        case SELECTION_ID_QUEUE_INTERNALS: {
            makeQueueInternals(rhs.d_queueInternals.object());
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
QueueResult& QueueResult::operator=(QueueResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_PURGED_QUEUES: {
            makePurgedQueues(bsl::move(rhs.d_purgedQueues.object()));
        } break;
        case SELECTION_ID_QUEUE_CONTENTS: {
            makeQueueContents(bsl::move(rhs.d_queueContents.object()));
        } break;
        case SELECTION_ID_QUEUE_INTERNALS: {
            makeQueueInternals(bsl::move(rhs.d_queueInternals.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void QueueResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        d_purgedQueues.object().~PurgedQueues();
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        d_queueContents.object().~QueueContents();
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        d_queueInternals.object().~QueueInternals();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int QueueResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        makePurgedQueues();
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        makeQueueContents();
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        makeQueueInternals();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int QueueResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Error& QueueResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& QueueResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& QueueResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

PurgedQueues& QueueResult::makePurgedQueues()
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_purgedQueues.object());
    }
    else {
        reset();
        new (d_purgedQueues.buffer()) PurgedQueues(d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}

PurgedQueues& QueueResult::makePurgedQueues(const PurgedQueues& value)
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        d_purgedQueues.object() = value;
    }
    else {
        reset();
        new (d_purgedQueues.buffer()) PurgedQueues(value, d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgedQueues& QueueResult::makePurgedQueues(PurgedQueues&& value)
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        d_purgedQueues.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}
#endif

QueueContents& QueueResult::makeQueueContents()
{
    if (SELECTION_ID_QUEUE_CONTENTS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueContents.object());
    }
    else {
        reset();
        new (d_queueContents.buffer()) QueueContents(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_CONTENTS;
    }

    return d_queueContents.object();
}

QueueContents& QueueResult::makeQueueContents(const QueueContents& value)
{
    if (SELECTION_ID_QUEUE_CONTENTS == d_selectionId) {
        d_queueContents.object() = value;
    }
    else {
        reset();
        new (d_queueContents.buffer()) QueueContents(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_CONTENTS;
    }

    return d_queueContents.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueContents& QueueResult::makeQueueContents(QueueContents&& value)
{
    if (SELECTION_ID_QUEUE_CONTENTS == d_selectionId) {
        d_queueContents.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueContents.buffer())
            QueueContents(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_CONTENTS;
    }

    return d_queueContents.object();
}
#endif

QueueInternals& QueueResult::makeQueueInternals()
{
    if (SELECTION_ID_QUEUE_INTERNALS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueInternals.object());
    }
    else {
        reset();
        new (d_queueInternals.buffer()) QueueInternals(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_INTERNALS;
    }

    return d_queueInternals.object();
}

QueueInternals& QueueResult::makeQueueInternals(const QueueInternals& value)
{
    if (SELECTION_ID_QUEUE_INTERNALS == d_selectionId) {
        d_queueInternals.object() = value;
    }
    else {
        reset();
        new (d_queueInternals.buffer()) QueueInternals(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_INTERNALS;
    }

    return d_queueInternals.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueInternals& QueueResult::makeQueueInternals(QueueInternals&& value)
{
    if (SELECTION_ID_QUEUE_INTERNALS == d_selectionId) {
        d_queueInternals.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueInternals.buffer())
            QueueInternals(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_INTERNALS;
    }

    return d_queueInternals.object();
}
#endif

// ACCESSORS

bsl::ostream&
QueueResult::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        printer.printAttribute("purgedQueues", d_purgedQueues.object());
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        printer.printAttribute("queueContents", d_queueContents.object());
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        printer.printAttribute("queueInternals", d_queueInternals.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* QueueResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_PURGED_QUEUES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGED_QUEUES].name();
    case SELECTION_ID_QUEUE_CONTENTS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_CONTENTS].name();
    case SELECTION_ID_QUEUE_INTERNALS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_INTERNALS].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ------------
// class Result
// ------------

// CONSTANTS

const char Result::CLASS_NAME[] = "Result";

const bdlat_SelectionInfo Result::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SUCCESS,
     "success",
     sizeof("success") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_VALUE,
     "value",
     sizeof("value") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLE,
     "tunable",
     sizeof("tunable") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLES,
     "tunables",
     sizeof("tunables") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_TUNABLE_CONFIRMATION,
     "tunableConfirmation",
     sizeof("tunableConfirmation") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_HELP,
     "help",
     sizeof("help") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DOMAIN_INFO,
     "domainInfo",
     sizeof("domainInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PURGED_QUEUES,
     "purgedQueues",
     sizeof("purgedQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_INTERNALS,
     "queueInternals",
     sizeof("queueInternals") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_MESSAGE_GROUP_ID_HELPER,
     "messageGroupIdHelper",
     sizeof("messageGroupIdHelper") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_CONTENTS,
     "queueContents",
     sizeof("queueContents") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_MESSAGE,
     "message",
     sizeof("message") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STATS,
     "stats",
     sizeof("stats") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {SELECTION_ID_CLUSTER_LIST,
     "clusterList",
     sizeof("clusterList") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_STATUS,
     "clusterStatus",
     sizeof("clusterStatus") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_PROXY_STATUS,
     "clusterProxyStatus",
     sizeof("clusterProxyStatus") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_NODE_STATUSES,
     "nodeStatuses",
     sizeof("nodeStatuses") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ELECTOR_INFO,
     "electorInfo",
     sizeof("electorInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PARTITIONS_INFO,
     "partitionsInfo",
     sizeof("partitionsInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_QUEUE_HELPER,
     "clusterQueueHelper",
     sizeof("clusterQueueHelper") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STORAGE_CONTENT,
     "storageContent",
     sizeof("storageContent") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_STORAGE_SUMMARY,
     "clusterStorageSummary",
     sizeof("clusterStorageSummary") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES,
     "clusterDomainQueueStatuses",
     sizeof("clusterDomainQueueStatuses") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_BROKER_CONFIG,
     "brokerConfig",
     sizeof("brokerConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* Result::lookupSelectionInfo(const char* name,
                                                       int         nameLength)
{
    for (int i = 0; i < 25; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            Result::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* Result::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_SUCCESS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS];
    case SELECTION_ID_VALUE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_VALUE];
    case SELECTION_ID_TUNABLE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE];
    case SELECTION_ID_TUNABLES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLES];
    case SELECTION_ID_TUNABLE_CONFIRMATION:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE_CONFIRMATION];
    case SELECTION_ID_HELP: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_HELP];
    case SELECTION_ID_DOMAIN_INFO:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_INFO];
    case SELECTION_ID_PURGED_QUEUES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGED_QUEUES];
    case SELECTION_ID_QUEUE_INTERNALS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_INTERNALS];
    case SELECTION_ID_MESSAGE_GROUP_ID_HELPER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_MESSAGE_GROUP_ID_HELPER];
    case SELECTION_ID_QUEUE_CONTENTS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_CONTENTS];
    case SELECTION_ID_MESSAGE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_MESSAGE];
    case SELECTION_ID_STATS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STATS];
    case SELECTION_ID_CLUSTER_LIST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_LIST];
    case SELECTION_ID_CLUSTER_STATUS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STATUS];
    case SELECTION_ID_CLUSTER_PROXY_STATUS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_PROXY_STATUS];
    case SELECTION_ID_NODE_STATUSES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_NODE_STATUSES];
    case SELECTION_ID_ELECTOR_INFO:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTOR_INFO];
    case SELECTION_ID_PARTITIONS_INFO:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PARTITIONS_INFO];
    case SELECTION_ID_CLUSTER_QUEUE_HELPER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_QUEUE_HELPER];
    case SELECTION_ID_STORAGE_CONTENT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_CONTENT];
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STORAGE_SUMMARY];
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_CLUSTER_DOMAIN_QUEUE_STATUSES];
    case SELECTION_ID_BROKER_CONFIG:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_BROKER_CONFIG];
    default: return 0;
    }
}

// CREATORS

Result::Result(const Result& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(original.d_success.object());
    } break;
    case SELECTION_ID_VALUE: {
        new (d_value.buffer()) Value(original.d_value.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(original.d_tunable.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(original.d_tunables.object(), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(original.d_tunableConfirmation.object(),
                                d_allocator_p);
    } break;
    case SELECTION_ID_HELP: {
        new (d_help.buffer()) Help(original.d_help.object(), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        new (d_domainInfo.buffer())
            DomainInfo(original.d_domainInfo.object(), d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(original.d_purgedQueues.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        new (d_queueInternals.buffer())
            QueueInternals(original.d_queueInternals.object(), d_allocator_p);
    } break;
    case SELECTION_ID_MESSAGE_GROUP_ID_HELPER: {
        new (d_messageGroupIdHelper.buffer())
            MessageGroupIdHelper(original.d_messageGroupIdHelper.object(),
                                 d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        new (d_queueContents.buffer())
            QueueContents(original.d_queueContents.object(), d_allocator_p);
    } break;
    case SELECTION_ID_MESSAGE: {
        new (d_message.buffer())
            Message(original.d_message.object(), d_allocator_p);
    } break;
    case SELECTION_ID_STATS: {
        new (d_stats.buffer())
            bsl::string(original.d_stats.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        new (d_clusterList.buffer())
            ClusterList(original.d_clusterList.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        new (d_clusterStatus.buffer())
            ClusterStatus(original.d_clusterStatus.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        new (d_clusterProxyStatus.buffer())
            ClusterProxyStatus(original.d_clusterProxyStatus.object(),
                               d_allocator_p);
    } break;
    case SELECTION_ID_NODE_STATUSES: {
        new (d_nodeStatuses.buffer())
            NodeStatuses(original.d_nodeStatuses.object(), d_allocator_p);
    } break;
    case SELECTION_ID_ELECTOR_INFO: {
        new (d_electorInfo.buffer())
            ElectorInfo(original.d_electorInfo.object(), d_allocator_p);
    } break;
    case SELECTION_ID_PARTITIONS_INFO: {
        new (d_partitionsInfo.buffer())
            PartitionsInfo(original.d_partitionsInfo.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        new (d_clusterQueueHelper.buffer())
            ClusterQueueHelper(original.d_clusterQueueHelper.object(),
                               d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        new (d_storageContent.buffer())
            StorageContent(original.d_storageContent.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(original.d_clusterStorageSummary.object(),
                                  d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        new (d_clusterDomainQueueStatuses.buffer()) ClusterDomainQueueStatuses(
            original.d_clusterDomainQueueStatuses.object(),
            d_allocator_p);
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        new (d_brokerConfig.buffer())
            BrokerConfig(original.d_brokerConfig.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Result::Result(Result&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_VALUE: {
        new (d_value.buffer())
            Value(bsl::move(original.d_value.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(bsl::move(original.d_tunable.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(bsl::move(original.d_tunables.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer()) TunableConfirmation(
            bsl::move(original.d_tunableConfirmation.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_HELP: {
        new (d_help.buffer())
            Help(bsl::move(original.d_help.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        new (d_domainInfo.buffer())
            DomainInfo(bsl::move(original.d_domainInfo.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(original.d_purgedQueues.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        new (d_queueInternals.buffer())
            QueueInternals(bsl::move(original.d_queueInternals.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_MESSAGE_GROUP_ID_HELPER: {
        new (d_messageGroupIdHelper.buffer()) MessageGroupIdHelper(
            bsl::move(original.d_messageGroupIdHelper.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        new (d_queueContents.buffer())
            QueueContents(bsl::move(original.d_queueContents.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_MESSAGE: {
        new (d_message.buffer())
            Message(bsl::move(original.d_message.object()), d_allocator_p);
    } break;
    case SELECTION_ID_STATS: {
        new (d_stats.buffer())
            bsl::string(bsl::move(original.d_stats.object()), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        new (d_clusterList.buffer())
            ClusterList(bsl::move(original.d_clusterList.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        new (d_clusterStatus.buffer())
            ClusterStatus(bsl::move(original.d_clusterStatus.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        new (d_clusterProxyStatus.buffer()) ClusterProxyStatus(
            bsl::move(original.d_clusterProxyStatus.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_NODE_STATUSES: {
        new (d_nodeStatuses.buffer())
            NodeStatuses(bsl::move(original.d_nodeStatuses.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_ELECTOR_INFO: {
        new (d_electorInfo.buffer())
            ElectorInfo(bsl::move(original.d_electorInfo.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_PARTITIONS_INFO: {
        new (d_partitionsInfo.buffer())
            PartitionsInfo(bsl::move(original.d_partitionsInfo.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        new (d_clusterQueueHelper.buffer()) ClusterQueueHelper(
            bsl::move(original.d_clusterQueueHelper.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        new (d_storageContent.buffer())
            StorageContent(bsl::move(original.d_storageContent.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        new (d_clusterStorageSummary.buffer()) ClusterStorageSummary(
            bsl::move(original.d_clusterStorageSummary.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        new (d_clusterDomainQueueStatuses.buffer()) ClusterDomainQueueStatuses(
            bsl::move(original.d_clusterDomainQueueStatuses.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        new (d_brokerConfig.buffer())
            BrokerConfig(bsl::move(original.d_brokerConfig.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

Result::Result(Result&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_VALUE: {
        new (d_value.buffer())
            Value(bsl::move(original.d_value.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE: {
        new (d_tunable.buffer())
            Tunable(bsl::move(original.d_tunable.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLES: {
        new (d_tunables.buffer())
            Tunables(bsl::move(original.d_tunables.object()), d_allocator_p);
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        new (d_tunableConfirmation.buffer()) TunableConfirmation(
            bsl::move(original.d_tunableConfirmation.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_HELP: {
        new (d_help.buffer())
            Help(bsl::move(original.d_help.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        new (d_domainInfo.buffer())
            DomainInfo(bsl::move(original.d_domainInfo.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(original.d_purgedQueues.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        new (d_queueInternals.buffer())
            QueueInternals(bsl::move(original.d_queueInternals.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_MESSAGE_GROUP_ID_HELPER: {
        new (d_messageGroupIdHelper.buffer()) MessageGroupIdHelper(
            bsl::move(original.d_messageGroupIdHelper.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        new (d_queueContents.buffer())
            QueueContents(bsl::move(original.d_queueContents.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_MESSAGE: {
        new (d_message.buffer())
            Message(bsl::move(original.d_message.object()), d_allocator_p);
    } break;
    case SELECTION_ID_STATS: {
        new (d_stats.buffer())
            bsl::string(bsl::move(original.d_stats.object()), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        new (d_clusterList.buffer())
            ClusterList(bsl::move(original.d_clusterList.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        new (d_clusterStatus.buffer())
            ClusterStatus(bsl::move(original.d_clusterStatus.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        new (d_clusterProxyStatus.buffer()) ClusterProxyStatus(
            bsl::move(original.d_clusterProxyStatus.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_NODE_STATUSES: {
        new (d_nodeStatuses.buffer())
            NodeStatuses(bsl::move(original.d_nodeStatuses.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_ELECTOR_INFO: {
        new (d_electorInfo.buffer())
            ElectorInfo(bsl::move(original.d_electorInfo.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_PARTITIONS_INFO: {
        new (d_partitionsInfo.buffer())
            PartitionsInfo(bsl::move(original.d_partitionsInfo.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        new (d_clusterQueueHelper.buffer()) ClusterQueueHelper(
            bsl::move(original.d_clusterQueueHelper.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        new (d_storageContent.buffer())
            StorageContent(bsl::move(original.d_storageContent.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        new (d_clusterStorageSummary.buffer()) ClusterStorageSummary(
            bsl::move(original.d_clusterStorageSummary.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        new (d_clusterDomainQueueStatuses.buffer()) ClusterDomainQueueStatuses(
            bsl::move(original.d_clusterDomainQueueStatuses.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        new (d_brokerConfig.buffer())
            BrokerConfig(bsl::move(original.d_brokerConfig.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

Result& Result::operator=(const Result& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_SUCCESS: {
            makeSuccess(rhs.d_success.object());
        } break;
        case SELECTION_ID_VALUE: {
            makeValue(rhs.d_value.object());
        } break;
        case SELECTION_ID_TUNABLE: {
            makeTunable(rhs.d_tunable.object());
        } break;
        case SELECTION_ID_TUNABLES: {
            makeTunables(rhs.d_tunables.object());
        } break;
        case SELECTION_ID_TUNABLE_CONFIRMATION: {
            makeTunableConfirmation(rhs.d_tunableConfirmation.object());
        } break;
        case SELECTION_ID_HELP: {
            makeHelp(rhs.d_help.object());
        } break;
        case SELECTION_ID_DOMAIN_INFO: {
            makeDomainInfo(rhs.d_domainInfo.object());
        } break;
        case SELECTION_ID_PURGED_QUEUES: {
            makePurgedQueues(rhs.d_purgedQueues.object());
        } break;
        case SELECTION_ID_QUEUE_INTERNALS: {
            makeQueueInternals(rhs.d_queueInternals.object());
        } break;
        case SELECTION_ID_MESSAGE_GROUP_ID_HELPER: {
            makeMessageGroupIdHelper(rhs.d_messageGroupIdHelper.object());
        } break;
        case SELECTION_ID_QUEUE_CONTENTS: {
            makeQueueContents(rhs.d_queueContents.object());
        } break;
        case SELECTION_ID_MESSAGE: {
            makeMessage(rhs.d_message.object());
        } break;
        case SELECTION_ID_STATS: {
            makeStats(rhs.d_stats.object());
        } break;
        case SELECTION_ID_CLUSTER_LIST: {
            makeClusterList(rhs.d_clusterList.object());
        } break;
        case SELECTION_ID_CLUSTER_STATUS: {
            makeClusterStatus(rhs.d_clusterStatus.object());
        } break;
        case SELECTION_ID_CLUSTER_PROXY_STATUS: {
            makeClusterProxyStatus(rhs.d_clusterProxyStatus.object());
        } break;
        case SELECTION_ID_NODE_STATUSES: {
            makeNodeStatuses(rhs.d_nodeStatuses.object());
        } break;
        case SELECTION_ID_ELECTOR_INFO: {
            makeElectorInfo(rhs.d_electorInfo.object());
        } break;
        case SELECTION_ID_PARTITIONS_INFO: {
            makePartitionsInfo(rhs.d_partitionsInfo.object());
        } break;
        case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
            makeClusterQueueHelper(rhs.d_clusterQueueHelper.object());
        } break;
        case SELECTION_ID_STORAGE_CONTENT: {
            makeStorageContent(rhs.d_storageContent.object());
        } break;
        case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
            makeClusterStorageSummary(rhs.d_clusterStorageSummary.object());
        } break;
        case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
            makeClusterDomainQueueStatuses(
                rhs.d_clusterDomainQueueStatuses.object());
        } break;
        case SELECTION_ID_BROKER_CONFIG: {
            makeBrokerConfig(rhs.d_brokerConfig.object());
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
Result& Result::operator=(Result&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_SUCCESS: {
            makeSuccess(bsl::move(rhs.d_success.object()));
        } break;
        case SELECTION_ID_VALUE: {
            makeValue(bsl::move(rhs.d_value.object()));
        } break;
        case SELECTION_ID_TUNABLE: {
            makeTunable(bsl::move(rhs.d_tunable.object()));
        } break;
        case SELECTION_ID_TUNABLES: {
            makeTunables(bsl::move(rhs.d_tunables.object()));
        } break;
        case SELECTION_ID_TUNABLE_CONFIRMATION: {
            makeTunableConfirmation(
                bsl::move(rhs.d_tunableConfirmation.object()));
        } break;
        case SELECTION_ID_HELP: {
            makeHelp(bsl::move(rhs.d_help.object()));
        } break;
        case SELECTION_ID_DOMAIN_INFO: {
            makeDomainInfo(bsl::move(rhs.d_domainInfo.object()));
        } break;
        case SELECTION_ID_PURGED_QUEUES: {
            makePurgedQueues(bsl::move(rhs.d_purgedQueues.object()));
        } break;
        case SELECTION_ID_QUEUE_INTERNALS: {
            makeQueueInternals(bsl::move(rhs.d_queueInternals.object()));
        } break;
        case SELECTION_ID_MESSAGE_GROUP_ID_HELPER: {
            makeMessageGroupIdHelper(
                bsl::move(rhs.d_messageGroupIdHelper.object()));
        } break;
        case SELECTION_ID_QUEUE_CONTENTS: {
            makeQueueContents(bsl::move(rhs.d_queueContents.object()));
        } break;
        case SELECTION_ID_MESSAGE: {
            makeMessage(bsl::move(rhs.d_message.object()));
        } break;
        case SELECTION_ID_STATS: {
            makeStats(bsl::move(rhs.d_stats.object()));
        } break;
        case SELECTION_ID_CLUSTER_LIST: {
            makeClusterList(bsl::move(rhs.d_clusterList.object()));
        } break;
        case SELECTION_ID_CLUSTER_STATUS: {
            makeClusterStatus(bsl::move(rhs.d_clusterStatus.object()));
        } break;
        case SELECTION_ID_CLUSTER_PROXY_STATUS: {
            makeClusterProxyStatus(
                bsl::move(rhs.d_clusterProxyStatus.object()));
        } break;
        case SELECTION_ID_NODE_STATUSES: {
            makeNodeStatuses(bsl::move(rhs.d_nodeStatuses.object()));
        } break;
        case SELECTION_ID_ELECTOR_INFO: {
            makeElectorInfo(bsl::move(rhs.d_electorInfo.object()));
        } break;
        case SELECTION_ID_PARTITIONS_INFO: {
            makePartitionsInfo(bsl::move(rhs.d_partitionsInfo.object()));
        } break;
        case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
            makeClusterQueueHelper(
                bsl::move(rhs.d_clusterQueueHelper.object()));
        } break;
        case SELECTION_ID_STORAGE_CONTENT: {
            makeStorageContent(bsl::move(rhs.d_storageContent.object()));
        } break;
        case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
            makeClusterStorageSummary(
                bsl::move(rhs.d_clusterStorageSummary.object()));
        } break;
        case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
            makeClusterDomainQueueStatuses(
                bsl::move(rhs.d_clusterDomainQueueStatuses.object()));
        } break;
        case SELECTION_ID_BROKER_CONFIG: {
            makeBrokerConfig(bsl::move(rhs.d_brokerConfig.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void Result::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_SUCCESS: {
        d_success.object().~Void();
    } break;
    case SELECTION_ID_VALUE: {
        d_value.object().~Value();
    } break;
    case SELECTION_ID_TUNABLE: {
        d_tunable.object().~Tunable();
    } break;
    case SELECTION_ID_TUNABLES: {
        d_tunables.object().~Tunables();
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        d_tunableConfirmation.object().~TunableConfirmation();
    } break;
    case SELECTION_ID_HELP: {
        d_help.object().~Help();
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        d_domainInfo.object().~DomainInfo();
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        d_purgedQueues.object().~PurgedQueues();
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        d_queueInternals.object().~QueueInternals();
    } break;
    case SELECTION_ID_MESSAGE_GROUP_ID_HELPER: {
        d_messageGroupIdHelper.object().~MessageGroupIdHelper();
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        d_queueContents.object().~QueueContents();
    } break;
    case SELECTION_ID_MESSAGE: {
        d_message.object().~Message();
    } break;
    case SELECTION_ID_STATS: {
        typedef bsl::string Type;
        d_stats.object().~Type();
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        d_clusterList.object().~ClusterList();
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        d_clusterStatus.object().~ClusterStatus();
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        d_clusterProxyStatus.object().~ClusterProxyStatus();
    } break;
    case SELECTION_ID_NODE_STATUSES: {
        d_nodeStatuses.object().~NodeStatuses();
    } break;
    case SELECTION_ID_ELECTOR_INFO: {
        d_electorInfo.object().~ElectorInfo();
    } break;
    case SELECTION_ID_PARTITIONS_INFO: {
        d_partitionsInfo.object().~PartitionsInfo();
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        d_clusterQueueHelper.object().~ClusterQueueHelper();
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        d_storageContent.object().~StorageContent();
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        d_clusterStorageSummary.object().~ClusterStorageSummary();
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        d_clusterDomainQueueStatuses.object().~ClusterDomainQueueStatuses();
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        d_brokerConfig.object().~BrokerConfig();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int Result::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_SUCCESS: {
        makeSuccess();
    } break;
    case SELECTION_ID_VALUE: {
        makeValue();
    } break;
    case SELECTION_ID_TUNABLE: {
        makeTunable();
    } break;
    case SELECTION_ID_TUNABLES: {
        makeTunables();
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        makeTunableConfirmation();
    } break;
    case SELECTION_ID_HELP: {
        makeHelp();
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        makeDomainInfo();
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        makePurgedQueues();
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        makeQueueInternals();
    } break;
    case SELECTION_ID_MESSAGE_GROUP_ID_HELPER: {
        makeMessageGroupIdHelper();
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        makeQueueContents();
    } break;
    case SELECTION_ID_MESSAGE: {
        makeMessage();
    } break;
    case SELECTION_ID_STATS: {
        makeStats();
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        makeClusterList();
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        makeClusterStatus();
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        makeClusterProxyStatus();
    } break;
    case SELECTION_ID_NODE_STATUSES: {
        makeNodeStatuses();
    } break;
    case SELECTION_ID_ELECTOR_INFO: {
        makeElectorInfo();
    } break;
    case SELECTION_ID_PARTITIONS_INFO: {
        makePartitionsInfo();
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        makeClusterQueueHelper();
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        makeStorageContent();
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        makeClusterStorageSummary();
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        makeClusterDomainQueueStatuses();
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        makeBrokerConfig();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int Result::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Error& Result::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& Result::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& Result::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

Void& Result::makeSuccess()
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_success.object());
    }
    else {
        reset();
        new (d_success.buffer()) Void();
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

Void& Result::makeSuccess(const Void& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = value;
    }
    else {
        reset();
        new (d_success.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& Result::makeSuccess(Void&& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_success.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}
#endif

Value& Result::makeValue()
{
    if (SELECTION_ID_VALUE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_value.object());
    }
    else {
        reset();
        new (d_value.buffer()) Value(d_allocator_p);
        d_selectionId = SELECTION_ID_VALUE;
    }

    return d_value.object();
}

Value& Result::makeValue(const Value& value)
{
    if (SELECTION_ID_VALUE == d_selectionId) {
        d_value.object() = value;
    }
    else {
        reset();
        new (d_value.buffer()) Value(value, d_allocator_p);
        d_selectionId = SELECTION_ID_VALUE;
    }

    return d_value.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Value& Result::makeValue(Value&& value)
{
    if (SELECTION_ID_VALUE == d_selectionId) {
        d_value.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_value.buffer()) Value(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_VALUE;
    }

    return d_value.object();
}
#endif

Tunable& Result::makeTunable()
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunable.object());
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}

Tunable& Result::makeTunable(const Tunable& value)
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        d_tunable.object() = value;
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunable& Result::makeTunable(Tunable&& value)
{
    if (SELECTION_ID_TUNABLE == d_selectionId) {
        d_tunable.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunable.buffer()) Tunable(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE;
    }

    return d_tunable.object();
}
#endif

Tunables& Result::makeTunables()
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunables.object());
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}

Tunables& Result::makeTunables(const Tunables& value)
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        d_tunables.object() = value;
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Tunables& Result::makeTunables(Tunables&& value)
{
    if (SELECTION_ID_TUNABLES == d_selectionId) {
        d_tunables.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunables.buffer()) Tunables(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLES;
    }

    return d_tunables.object();
}
#endif

TunableConfirmation& Result::makeTunableConfirmation()
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tunableConfirmation.object());
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}

TunableConfirmation&
Result::makeTunableConfirmation(const TunableConfirmation& value)
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        d_tunableConfirmation.object() = value;
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TunableConfirmation&
Result::makeTunableConfirmation(TunableConfirmation&& value)
{
    if (SELECTION_ID_TUNABLE_CONFIRMATION == d_selectionId) {
        d_tunableConfirmation.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tunableConfirmation.buffer())
            TunableConfirmation(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TUNABLE_CONFIRMATION;
    }

    return d_tunableConfirmation.object();
}
#endif

Help& Result::makeHelp()
{
    if (SELECTION_ID_HELP == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_help.object());
    }
    else {
        reset();
        new (d_help.buffer()) Help(d_allocator_p);
        d_selectionId = SELECTION_ID_HELP;
    }

    return d_help.object();
}

Help& Result::makeHelp(const Help& value)
{
    if (SELECTION_ID_HELP == d_selectionId) {
        d_help.object() = value;
    }
    else {
        reset();
        new (d_help.buffer()) Help(value, d_allocator_p);
        d_selectionId = SELECTION_ID_HELP;
    }

    return d_help.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Help& Result::makeHelp(Help&& value)
{
    if (SELECTION_ID_HELP == d_selectionId) {
        d_help.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_help.buffer()) Help(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_HELP;
    }

    return d_help.object();
}
#endif

DomainInfo& Result::makeDomainInfo()
{
    if (SELECTION_ID_DOMAIN_INFO == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domainInfo.object());
    }
    else {
        reset();
        new (d_domainInfo.buffer()) DomainInfo(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_INFO;
    }

    return d_domainInfo.object();
}

DomainInfo& Result::makeDomainInfo(const DomainInfo& value)
{
    if (SELECTION_ID_DOMAIN_INFO == d_selectionId) {
        d_domainInfo.object() = value;
    }
    else {
        reset();
        new (d_domainInfo.buffer()) DomainInfo(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_INFO;
    }

    return d_domainInfo.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainInfo& Result::makeDomainInfo(DomainInfo&& value)
{
    if (SELECTION_ID_DOMAIN_INFO == d_selectionId) {
        d_domainInfo.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domainInfo.buffer())
            DomainInfo(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_INFO;
    }

    return d_domainInfo.object();
}
#endif

PurgedQueues& Result::makePurgedQueues()
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_purgedQueues.object());
    }
    else {
        reset();
        new (d_purgedQueues.buffer()) PurgedQueues(d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}

PurgedQueues& Result::makePurgedQueues(const PurgedQueues& value)
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        d_purgedQueues.object() = value;
    }
    else {
        reset();
        new (d_purgedQueues.buffer()) PurgedQueues(value, d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgedQueues& Result::makePurgedQueues(PurgedQueues&& value)
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        d_purgedQueues.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}
#endif

QueueInternals& Result::makeQueueInternals()
{
    if (SELECTION_ID_QUEUE_INTERNALS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueInternals.object());
    }
    else {
        reset();
        new (d_queueInternals.buffer()) QueueInternals(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_INTERNALS;
    }

    return d_queueInternals.object();
}

QueueInternals& Result::makeQueueInternals(const QueueInternals& value)
{
    if (SELECTION_ID_QUEUE_INTERNALS == d_selectionId) {
        d_queueInternals.object() = value;
    }
    else {
        reset();
        new (d_queueInternals.buffer()) QueueInternals(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_INTERNALS;
    }

    return d_queueInternals.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueInternals& Result::makeQueueInternals(QueueInternals&& value)
{
    if (SELECTION_ID_QUEUE_INTERNALS == d_selectionId) {
        d_queueInternals.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueInternals.buffer())
            QueueInternals(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_INTERNALS;
    }

    return d_queueInternals.object();
}
#endif

MessageGroupIdHelper& Result::makeMessageGroupIdHelper()
{
    if (SELECTION_ID_MESSAGE_GROUP_ID_HELPER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_messageGroupIdHelper.object());
    }
    else {
        reset();
        new (d_messageGroupIdHelper.buffer())
            MessageGroupIdHelper(d_allocator_p);
        d_selectionId = SELECTION_ID_MESSAGE_GROUP_ID_HELPER;
    }

    return d_messageGroupIdHelper.object();
}

MessageGroupIdHelper&
Result::makeMessageGroupIdHelper(const MessageGroupIdHelper& value)
{
    if (SELECTION_ID_MESSAGE_GROUP_ID_HELPER == d_selectionId) {
        d_messageGroupIdHelper.object() = value;
    }
    else {
        reset();
        new (d_messageGroupIdHelper.buffer())
            MessageGroupIdHelper(value, d_allocator_p);
        d_selectionId = SELECTION_ID_MESSAGE_GROUP_ID_HELPER;
    }

    return d_messageGroupIdHelper.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
MessageGroupIdHelper&
Result::makeMessageGroupIdHelper(MessageGroupIdHelper&& value)
{
    if (SELECTION_ID_MESSAGE_GROUP_ID_HELPER == d_selectionId) {
        d_messageGroupIdHelper.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_messageGroupIdHelper.buffer())
            MessageGroupIdHelper(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_MESSAGE_GROUP_ID_HELPER;
    }

    return d_messageGroupIdHelper.object();
}
#endif

QueueContents& Result::makeQueueContents()
{
    if (SELECTION_ID_QUEUE_CONTENTS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueContents.object());
    }
    else {
        reset();
        new (d_queueContents.buffer()) QueueContents(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_CONTENTS;
    }

    return d_queueContents.object();
}

QueueContents& Result::makeQueueContents(const QueueContents& value)
{
    if (SELECTION_ID_QUEUE_CONTENTS == d_selectionId) {
        d_queueContents.object() = value;
    }
    else {
        reset();
        new (d_queueContents.buffer()) QueueContents(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_CONTENTS;
    }

    return d_queueContents.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueContents& Result::makeQueueContents(QueueContents&& value)
{
    if (SELECTION_ID_QUEUE_CONTENTS == d_selectionId) {
        d_queueContents.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueContents.buffer())
            QueueContents(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_CONTENTS;
    }

    return d_queueContents.object();
}
#endif

Message& Result::makeMessage()
{
    if (SELECTION_ID_MESSAGE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_message.object());
    }
    else {
        reset();
        new (d_message.buffer()) Message(d_allocator_p);
        d_selectionId = SELECTION_ID_MESSAGE;
    }

    return d_message.object();
}

Message& Result::makeMessage(const Message& value)
{
    if (SELECTION_ID_MESSAGE == d_selectionId) {
        d_message.object() = value;
    }
    else {
        reset();
        new (d_message.buffer()) Message(value, d_allocator_p);
        d_selectionId = SELECTION_ID_MESSAGE;
    }

    return d_message.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Message& Result::makeMessage(Message&& value)
{
    if (SELECTION_ID_MESSAGE == d_selectionId) {
        d_message.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_message.buffer()) Message(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_MESSAGE;
    }

    return d_message.object();
}
#endif

bsl::string& Result::makeStats()
{
    if (SELECTION_ID_STATS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_stats.object());
    }
    else {
        reset();
        new (d_stats.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_STATS;
    }

    return d_stats.object();
}

bsl::string& Result::makeStats(const bsl::string& value)
{
    if (SELECTION_ID_STATS == d_selectionId) {
        d_stats.object() = value;
    }
    else {
        reset();
        new (d_stats.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STATS;
    }

    return d_stats.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& Result::makeStats(bsl::string&& value)
{
    if (SELECTION_ID_STATS == d_selectionId) {
        d_stats.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_stats.buffer()) bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STATS;
    }

    return d_stats.object();
}
#endif

ClusterList& Result::makeClusterList()
{
    if (SELECTION_ID_CLUSTER_LIST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterList.object());
    }
    else {
        reset();
        new (d_clusterList.buffer()) ClusterList(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_LIST;
    }

    return d_clusterList.object();
}

ClusterList& Result::makeClusterList(const ClusterList& value)
{
    if (SELECTION_ID_CLUSTER_LIST == d_selectionId) {
        d_clusterList.object() = value;
    }
    else {
        reset();
        new (d_clusterList.buffer()) ClusterList(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_LIST;
    }

    return d_clusterList.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterList& Result::makeClusterList(ClusterList&& value)
{
    if (SELECTION_ID_CLUSTER_LIST == d_selectionId) {
        d_clusterList.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterList.buffer())
            ClusterList(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_LIST;
    }

    return d_clusterList.object();
}
#endif

ClusterStatus& Result::makeClusterStatus()
{
    if (SELECTION_ID_CLUSTER_STATUS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterStatus.object());
    }
    else {
        reset();
        new (d_clusterStatus.buffer()) ClusterStatus(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STATUS;
    }

    return d_clusterStatus.object();
}

ClusterStatus& Result::makeClusterStatus(const ClusterStatus& value)
{
    if (SELECTION_ID_CLUSTER_STATUS == d_selectionId) {
        d_clusterStatus.object() = value;
    }
    else {
        reset();
        new (d_clusterStatus.buffer()) ClusterStatus(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STATUS;
    }

    return d_clusterStatus.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStatus& Result::makeClusterStatus(ClusterStatus&& value)
{
    if (SELECTION_ID_CLUSTER_STATUS == d_selectionId) {
        d_clusterStatus.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterStatus.buffer())
            ClusterStatus(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STATUS;
    }

    return d_clusterStatus.object();
}
#endif

ClusterProxyStatus& Result::makeClusterProxyStatus()
{
    if (SELECTION_ID_CLUSTER_PROXY_STATUS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterProxyStatus.object());
    }
    else {
        reset();
        new (d_clusterProxyStatus.buffer()) ClusterProxyStatus(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_PROXY_STATUS;
    }

    return d_clusterProxyStatus.object();
}

ClusterProxyStatus&
Result::makeClusterProxyStatus(const ClusterProxyStatus& value)
{
    if (SELECTION_ID_CLUSTER_PROXY_STATUS == d_selectionId) {
        d_clusterProxyStatus.object() = value;
    }
    else {
        reset();
        new (d_clusterProxyStatus.buffer())
            ClusterProxyStatus(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_PROXY_STATUS;
    }

    return d_clusterProxyStatus.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterProxyStatus& Result::makeClusterProxyStatus(ClusterProxyStatus&& value)
{
    if (SELECTION_ID_CLUSTER_PROXY_STATUS == d_selectionId) {
        d_clusterProxyStatus.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterProxyStatus.buffer())
            ClusterProxyStatus(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_PROXY_STATUS;
    }

    return d_clusterProxyStatus.object();
}
#endif

NodeStatuses& Result::makeNodeStatuses()
{
    if (SELECTION_ID_NODE_STATUSES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_nodeStatuses.object());
    }
    else {
        reset();
        new (d_nodeStatuses.buffer()) NodeStatuses(d_allocator_p);
        d_selectionId = SELECTION_ID_NODE_STATUSES;
    }

    return d_nodeStatuses.object();
}

NodeStatuses& Result::makeNodeStatuses(const NodeStatuses& value)
{
    if (SELECTION_ID_NODE_STATUSES == d_selectionId) {
        d_nodeStatuses.object() = value;
    }
    else {
        reset();
        new (d_nodeStatuses.buffer()) NodeStatuses(value, d_allocator_p);
        d_selectionId = SELECTION_ID_NODE_STATUSES;
    }

    return d_nodeStatuses.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
NodeStatuses& Result::makeNodeStatuses(NodeStatuses&& value)
{
    if (SELECTION_ID_NODE_STATUSES == d_selectionId) {
        d_nodeStatuses.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_nodeStatuses.buffer())
            NodeStatuses(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_NODE_STATUSES;
    }

    return d_nodeStatuses.object();
}
#endif

ElectorInfo& Result::makeElectorInfo()
{
    if (SELECTION_ID_ELECTOR_INFO == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_electorInfo.object());
    }
    else {
        reset();
        new (d_electorInfo.buffer()) ElectorInfo(d_allocator_p);
        d_selectionId = SELECTION_ID_ELECTOR_INFO;
    }

    return d_electorInfo.object();
}

ElectorInfo& Result::makeElectorInfo(const ElectorInfo& value)
{
    if (SELECTION_ID_ELECTOR_INFO == d_selectionId) {
        d_electorInfo.object() = value;
    }
    else {
        reset();
        new (d_electorInfo.buffer()) ElectorInfo(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ELECTOR_INFO;
    }

    return d_electorInfo.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ElectorInfo& Result::makeElectorInfo(ElectorInfo&& value)
{
    if (SELECTION_ID_ELECTOR_INFO == d_selectionId) {
        d_electorInfo.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_electorInfo.buffer())
            ElectorInfo(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ELECTOR_INFO;
    }

    return d_electorInfo.object();
}
#endif

PartitionsInfo& Result::makePartitionsInfo()
{
    if (SELECTION_ID_PARTITIONS_INFO == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_partitionsInfo.object());
    }
    else {
        reset();
        new (d_partitionsInfo.buffer()) PartitionsInfo(d_allocator_p);
        d_selectionId = SELECTION_ID_PARTITIONS_INFO;
    }

    return d_partitionsInfo.object();
}

PartitionsInfo& Result::makePartitionsInfo(const PartitionsInfo& value)
{
    if (SELECTION_ID_PARTITIONS_INFO == d_selectionId) {
        d_partitionsInfo.object() = value;
    }
    else {
        reset();
        new (d_partitionsInfo.buffer()) PartitionsInfo(value, d_allocator_p);
        d_selectionId = SELECTION_ID_PARTITIONS_INFO;
    }

    return d_partitionsInfo.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionsInfo& Result::makePartitionsInfo(PartitionsInfo&& value)
{
    if (SELECTION_ID_PARTITIONS_INFO == d_selectionId) {
        d_partitionsInfo.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_partitionsInfo.buffer())
            PartitionsInfo(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_PARTITIONS_INFO;
    }

    return d_partitionsInfo.object();
}
#endif

ClusterQueueHelper& Result::makeClusterQueueHelper()
{
    if (SELECTION_ID_CLUSTER_QUEUE_HELPER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterQueueHelper.object());
    }
    else {
        reset();
        new (d_clusterQueueHelper.buffer()) ClusterQueueHelper(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_QUEUE_HELPER;
    }

    return d_clusterQueueHelper.object();
}

ClusterQueueHelper&
Result::makeClusterQueueHelper(const ClusterQueueHelper& value)
{
    if (SELECTION_ID_CLUSTER_QUEUE_HELPER == d_selectionId) {
        d_clusterQueueHelper.object() = value;
    }
    else {
        reset();
        new (d_clusterQueueHelper.buffer())
            ClusterQueueHelper(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_QUEUE_HELPER;
    }

    return d_clusterQueueHelper.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterQueueHelper& Result::makeClusterQueueHelper(ClusterQueueHelper&& value)
{
    if (SELECTION_ID_CLUSTER_QUEUE_HELPER == d_selectionId) {
        d_clusterQueueHelper.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterQueueHelper.buffer())
            ClusterQueueHelper(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_QUEUE_HELPER;
    }

    return d_clusterQueueHelper.object();
}
#endif

StorageContent& Result::makeStorageContent()
{
    if (SELECTION_ID_STORAGE_CONTENT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_storageContent.object());
    }
    else {
        reset();
        new (d_storageContent.buffer()) StorageContent(d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE_CONTENT;
    }

    return d_storageContent.object();
}

StorageContent& Result::makeStorageContent(const StorageContent& value)
{
    if (SELECTION_ID_STORAGE_CONTENT == d_selectionId) {
        d_storageContent.object() = value;
    }
    else {
        reset();
        new (d_storageContent.buffer()) StorageContent(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE_CONTENT;
    }

    return d_storageContent.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageContent& Result::makeStorageContent(StorageContent&& value)
{
    if (SELECTION_ID_STORAGE_CONTENT == d_selectionId) {
        d_storageContent.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_storageContent.buffer())
            StorageContent(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STORAGE_CONTENT;
    }

    return d_storageContent.object();
}
#endif

ClusterStorageSummary& Result::makeClusterStorageSummary()
{
    if (SELECTION_ID_CLUSTER_STORAGE_SUMMARY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterStorageSummary.object());
    }
    else {
        reset();
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STORAGE_SUMMARY;
    }

    return d_clusterStorageSummary.object();
}

ClusterStorageSummary&
Result::makeClusterStorageSummary(const ClusterStorageSummary& value)
{
    if (SELECTION_ID_CLUSTER_STORAGE_SUMMARY == d_selectionId) {
        d_clusterStorageSummary.object() = value;
    }
    else {
        reset();
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STORAGE_SUMMARY;
    }

    return d_clusterStorageSummary.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStorageSummary&
Result::makeClusterStorageSummary(ClusterStorageSummary&& value)
{
    if (SELECTION_ID_CLUSTER_STORAGE_SUMMARY == d_selectionId) {
        d_clusterStorageSummary.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STORAGE_SUMMARY;
    }

    return d_clusterStorageSummary.object();
}
#endif

ClusterDomainQueueStatuses& Result::makeClusterDomainQueueStatuses()
{
    if (SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_clusterDomainQueueStatuses.object());
    }
    else {
        reset();
        new (d_clusterDomainQueueStatuses.buffer())
            ClusterDomainQueueStatuses(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES;
    }

    return d_clusterDomainQueueStatuses.object();
}

ClusterDomainQueueStatuses&
Result::makeClusterDomainQueueStatuses(const ClusterDomainQueueStatuses& value)
{
    if (SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES == d_selectionId) {
        d_clusterDomainQueueStatuses.object() = value;
    }
    else {
        reset();
        new (d_clusterDomainQueueStatuses.buffer())
            ClusterDomainQueueStatuses(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES;
    }

    return d_clusterDomainQueueStatuses.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterDomainQueueStatuses&
Result::makeClusterDomainQueueStatuses(ClusterDomainQueueStatuses&& value)
{
    if (SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES == d_selectionId) {
        d_clusterDomainQueueStatuses.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterDomainQueueStatuses.buffer())
            ClusterDomainQueueStatuses(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES;
    }

    return d_clusterDomainQueueStatuses.object();
}
#endif

BrokerConfig& Result::makeBrokerConfig()
{
    if (SELECTION_ID_BROKER_CONFIG == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_brokerConfig.object());
    }
    else {
        reset();
        new (d_brokerConfig.buffer()) BrokerConfig(d_allocator_p);
        d_selectionId = SELECTION_ID_BROKER_CONFIG;
    }

    return d_brokerConfig.object();
}

BrokerConfig& Result::makeBrokerConfig(const BrokerConfig& value)
{
    if (SELECTION_ID_BROKER_CONFIG == d_selectionId) {
        d_brokerConfig.object() = value;
    }
    else {
        reset();
        new (d_brokerConfig.buffer()) BrokerConfig(value, d_allocator_p);
        d_selectionId = SELECTION_ID_BROKER_CONFIG;
    }

    return d_brokerConfig.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerConfig& Result::makeBrokerConfig(BrokerConfig&& value)
{
    if (SELECTION_ID_BROKER_CONFIG == d_selectionId) {
        d_brokerConfig.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_brokerConfig.buffer())
            BrokerConfig(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_BROKER_CONFIG;
    }

    return d_brokerConfig.object();
}
#endif

// ACCESSORS

bsl::ostream&
Result::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_SUCCESS: {
        printer.printAttribute("success", d_success.object());
    } break;
    case SELECTION_ID_VALUE: {
        printer.printAttribute("value", d_value.object());
    } break;
    case SELECTION_ID_TUNABLE: {
        printer.printAttribute("tunable", d_tunable.object());
    } break;
    case SELECTION_ID_TUNABLES: {
        printer.printAttribute("tunables", d_tunables.object());
    } break;
    case SELECTION_ID_TUNABLE_CONFIRMATION: {
        printer.printAttribute("tunableConfirmation",
                               d_tunableConfirmation.object());
    } break;
    case SELECTION_ID_HELP: {
        printer.printAttribute("help", d_help.object());
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        printer.printAttribute("domainInfo", d_domainInfo.object());
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        printer.printAttribute("purgedQueues", d_purgedQueues.object());
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        printer.printAttribute("queueInternals", d_queueInternals.object());
    } break;
    case SELECTION_ID_MESSAGE_GROUP_ID_HELPER: {
        printer.printAttribute("messageGroupIdHelper",
                               d_messageGroupIdHelper.object());
    } break;
    case SELECTION_ID_QUEUE_CONTENTS: {
        printer.printAttribute("queueContents", d_queueContents.object());
    } break;
    case SELECTION_ID_MESSAGE: {
        printer.printAttribute("message", d_message.object());
    } break;
    case SELECTION_ID_STATS: {
        printer.printAttribute("stats", d_stats.object());
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        printer.printAttribute("clusterList", d_clusterList.object());
    } break;
    case SELECTION_ID_CLUSTER_STATUS: {
        printer.printAttribute("clusterStatus", d_clusterStatus.object());
    } break;
    case SELECTION_ID_CLUSTER_PROXY_STATUS: {
        printer.printAttribute("clusterProxyStatus",
                               d_clusterProxyStatus.object());
    } break;
    case SELECTION_ID_NODE_STATUSES: {
        printer.printAttribute("nodeStatuses", d_nodeStatuses.object());
    } break;
    case SELECTION_ID_ELECTOR_INFO: {
        printer.printAttribute("electorInfo", d_electorInfo.object());
    } break;
    case SELECTION_ID_PARTITIONS_INFO: {
        printer.printAttribute("partitionsInfo", d_partitionsInfo.object());
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        printer.printAttribute("clusterQueueHelper",
                               d_clusterQueueHelper.object());
    } break;
    case SELECTION_ID_STORAGE_CONTENT: {
        printer.printAttribute("storageContent", d_storageContent.object());
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        printer.printAttribute("clusterStorageSummary",
                               d_clusterStorageSummary.object());
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        printer.printAttribute("clusterDomainQueueStatuses",
                               d_clusterDomainQueueStatuses.object());
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        printer.printAttribute("brokerConfig", d_brokerConfig.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* Result::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_SUCCESS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS].name();
    case SELECTION_ID_VALUE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_VALUE].name();
    case SELECTION_ID_TUNABLE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE].name();
    case SELECTION_ID_TUNABLES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLES].name();
    case SELECTION_ID_TUNABLE_CONFIRMATION:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TUNABLE_CONFIRMATION]
            .name();
    case SELECTION_ID_HELP:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_HELP].name();
    case SELECTION_ID_DOMAIN_INFO:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_INFO].name();
    case SELECTION_ID_PURGED_QUEUES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGED_QUEUES].name();
    case SELECTION_ID_QUEUE_INTERNALS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_INTERNALS].name();
    case SELECTION_ID_MESSAGE_GROUP_ID_HELPER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_MESSAGE_GROUP_ID_HELPER]
            .name();
    case SELECTION_ID_QUEUE_CONTENTS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_CONTENTS].name();
    case SELECTION_ID_MESSAGE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_MESSAGE].name();
    case SELECTION_ID_STATS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STATS].name();
    case SELECTION_ID_CLUSTER_LIST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_LIST].name();
    case SELECTION_ID_CLUSTER_STATUS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STATUS].name();
    case SELECTION_ID_CLUSTER_PROXY_STATUS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_PROXY_STATUS]
            .name();
    case SELECTION_ID_NODE_STATUSES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_NODE_STATUSES].name();
    case SELECTION_ID_ELECTOR_INFO:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ELECTOR_INFO].name();
    case SELECTION_ID_PARTITIONS_INFO:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PARTITIONS_INFO].name();
    case SELECTION_ID_CLUSTER_QUEUE_HELPER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_QUEUE_HELPER]
            .name();
    case SELECTION_ID_STORAGE_CONTENT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STORAGE_CONTENT].name();
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STORAGE_SUMMARY]
            .name();
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_CLUSTER_DOMAIN_QUEUE_STATUSES]
                .name();
    case SELECTION_ID_BROKER_CONFIG:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_BROKER_CONFIG].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// ------------------
// class DomainResult
// ------------------

// CONSTANTS

const char DomainResult::CLASS_NAME[] = "DomainResult";

const bdlat_SelectionInfo DomainResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_SUCCESS,
     "success",
     sizeof("success") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DOMAIN_INFO,
     "domainInfo",
     sizeof("domainInfo") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PURGED_QUEUES,
     "purgedQueues",
     sizeof("purgedQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_RESULT,
     "queueResult",
     sizeof("queueResult") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* DomainResult::lookupSelectionInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            DomainResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* DomainResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_SUCCESS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS];
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_DOMAIN_INFO:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_INFO];
    case SELECTION_ID_PURGED_QUEUES:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGED_QUEUES];
    case SELECTION_ID_QUEUE_RESULT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_RESULT];
    default: return 0;
    }
}

// CREATORS

DomainResult::DomainResult(const DomainResult& original,
                           bslma::Allocator*   basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(original.d_success.object());
    } break;
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        new (d_domainInfo.buffer())
            DomainInfo(original.d_domainInfo.object(), d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(original.d_purgedQueues.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_RESULT: {
        new (d_queueResult.buffer())
            QueueResult(original.d_queueResult.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainResult::DomainResult(DomainResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        new (d_domainInfo.buffer())
            DomainInfo(bsl::move(original.d_domainInfo.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(original.d_purgedQueues.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_RESULT: {
        new (d_queueResult.buffer())
            QueueResult(bsl::move(original.d_queueResult.object()),
                        d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

DomainResult::DomainResult(DomainResult&&    original,
                           bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        new (d_domainInfo.buffer())
            DomainInfo(bsl::move(original.d_domainInfo.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(original.d_purgedQueues.object()),
                         d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_RESULT: {
        new (d_queueResult.buffer())
            QueueResult(bsl::move(original.d_queueResult.object()),
                        d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

DomainResult& DomainResult::operator=(const DomainResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SUCCESS: {
            makeSuccess(rhs.d_success.object());
        } break;
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_DOMAIN_INFO: {
            makeDomainInfo(rhs.d_domainInfo.object());
        } break;
        case SELECTION_ID_PURGED_QUEUES: {
            makePurgedQueues(rhs.d_purgedQueues.object());
        } break;
        case SELECTION_ID_QUEUE_RESULT: {
            makeQueueResult(rhs.d_queueResult.object());
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
DomainResult& DomainResult::operator=(DomainResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SUCCESS: {
            makeSuccess(bsl::move(rhs.d_success.object()));
        } break;
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_DOMAIN_INFO: {
            makeDomainInfo(bsl::move(rhs.d_domainInfo.object()));
        } break;
        case SELECTION_ID_PURGED_QUEUES: {
            makePurgedQueues(bsl::move(rhs.d_purgedQueues.object()));
        } break;
        case SELECTION_ID_QUEUE_RESULT: {
            makeQueueResult(bsl::move(rhs.d_queueResult.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void DomainResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        d_success.object().~Void();
    } break;
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        d_domainInfo.object().~DomainInfo();
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        d_purgedQueues.object().~PurgedQueues();
    } break;
    case SELECTION_ID_QUEUE_RESULT: {
        d_queueResult.object().~QueueResult();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int DomainResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_SUCCESS: {
        makeSuccess();
    } break;
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        makeDomainInfo();
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        makePurgedQueues();
    } break;
    case SELECTION_ID_QUEUE_RESULT: {
        makeQueueResult();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int DomainResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& DomainResult::makeSuccess()
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_success.object());
    }
    else {
        reset();
        new (d_success.buffer()) Void();
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

Void& DomainResult::makeSuccess(const Void& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = value;
    }
    else {
        reset();
        new (d_success.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& DomainResult::makeSuccess(Void&& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_success.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}
#endif

Error& DomainResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& DomainResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& DomainResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

DomainInfo& DomainResult::makeDomainInfo()
{
    if (SELECTION_ID_DOMAIN_INFO == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domainInfo.object());
    }
    else {
        reset();
        new (d_domainInfo.buffer()) DomainInfo(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_INFO;
    }

    return d_domainInfo.object();
}

DomainInfo& DomainResult::makeDomainInfo(const DomainInfo& value)
{
    if (SELECTION_ID_DOMAIN_INFO == d_selectionId) {
        d_domainInfo.object() = value;
    }
    else {
        reset();
        new (d_domainInfo.buffer()) DomainInfo(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_INFO;
    }

    return d_domainInfo.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainInfo& DomainResult::makeDomainInfo(DomainInfo&& value)
{
    if (SELECTION_ID_DOMAIN_INFO == d_selectionId) {
        d_domainInfo.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domainInfo.buffer())
            DomainInfo(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_INFO;
    }

    return d_domainInfo.object();
}
#endif

PurgedQueues& DomainResult::makePurgedQueues()
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_purgedQueues.object());
    }
    else {
        reset();
        new (d_purgedQueues.buffer()) PurgedQueues(d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}

PurgedQueues& DomainResult::makePurgedQueues(const PurgedQueues& value)
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        d_purgedQueues.object() = value;
    }
    else {
        reset();
        new (d_purgedQueues.buffer()) PurgedQueues(value, d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PurgedQueues& DomainResult::makePurgedQueues(PurgedQueues&& value)
{
    if (SELECTION_ID_PURGED_QUEUES == d_selectionId) {
        d_purgedQueues.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_purgedQueues.buffer())
            PurgedQueues(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_PURGED_QUEUES;
    }

    return d_purgedQueues.object();
}
#endif

QueueResult& DomainResult::makeQueueResult()
{
    if (SELECTION_ID_QUEUE_RESULT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueResult.object());
    }
    else {
        reset();
        new (d_queueResult.buffer()) QueueResult(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_RESULT;
    }

    return d_queueResult.object();
}

QueueResult& DomainResult::makeQueueResult(const QueueResult& value)
{
    if (SELECTION_ID_QUEUE_RESULT == d_selectionId) {
        d_queueResult.object() = value;
    }
    else {
        reset();
        new (d_queueResult.buffer()) QueueResult(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_RESULT;
    }

    return d_queueResult.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueResult& DomainResult::makeQueueResult(QueueResult&& value)
{
    if (SELECTION_ID_QUEUE_RESULT == d_selectionId) {
        d_queueResult.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueResult.buffer())
            QueueResult(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_RESULT;
    }

    return d_queueResult.object();
}
#endif

// ACCESSORS

bsl::ostream&
DomainResult::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        printer.printAttribute("success", d_success.object());
    } break;
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_DOMAIN_INFO: {
        printer.printAttribute("domainInfo", d_domainInfo.object());
    } break;
    case SELECTION_ID_PURGED_QUEUES: {
        printer.printAttribute("purgedQueues", d_purgedQueues.object());
    } break;
    case SELECTION_ID_QUEUE_RESULT: {
        printer.printAttribute("queueResult", d_queueResult.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* DomainResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS].name();
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_DOMAIN_INFO:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_INFO].name();
    case SELECTION_ID_PURGED_QUEUES:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PURGED_QUEUES].name();
    case SELECTION_ID_QUEUE_RESULT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_RESULT].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -------------------------
// class RouteResponseResult
// -------------------------

// CONSTANTS

const char RouteResponseResult::CLASS_NAME[] = "RouteResponseResult";

const bdlat_AttributeInfo RouteResponseResult::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SOURCE_NODE_DESCRIPTION,
     "sourceNodeDescription",
     sizeof("sourceNodeDescription") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_RESULT,
     "result",
     sizeof("result") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
RouteResponseResult::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RouteResponseResult::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RouteResponseResult::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SOURCE_NODE_DESCRIPTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SOURCE_NODE_DESCRIPTION];
    case ATTRIBUTE_ID_RESULT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESULT];
    default: return 0;
    }
}

// CREATORS

RouteResponseResult::RouteResponseResult(bslma::Allocator* basicAllocator)
: d_sourceNodeDescription(basicAllocator)
, d_result(basicAllocator)
{
}

RouteResponseResult::RouteResponseResult(const RouteResponseResult& original,
                                         bslma::Allocator* basicAllocator)
: d_sourceNodeDescription(original.d_sourceNodeDescription, basicAllocator)
, d_result(original.d_result, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouteResponseResult::RouteResponseResult(
    RouteResponseResult&& original) noexcept
: d_sourceNodeDescription(bsl::move(original.d_sourceNodeDescription)),
  d_result(bsl::move(original.d_result))
{
}

RouteResponseResult::RouteResponseResult(RouteResponseResult&& original,
                                         bslma::Allocator*     basicAllocator)
: d_sourceNodeDescription(bsl::move(original.d_sourceNodeDescription),
                          basicAllocator)
, d_result(bsl::move(original.d_result), basicAllocator)
{
}
#endif

RouteResponseResult::~RouteResponseResult()
{
}

// MANIPULATORS

RouteResponseResult&
RouteResponseResult::operator=(const RouteResponseResult& rhs)
{
    if (this != &rhs) {
        d_sourceNodeDescription = rhs.d_sourceNodeDescription;
        d_result                = rhs.d_result;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouteResponseResult& RouteResponseResult::operator=(RouteResponseResult&& rhs)
{
    if (this != &rhs) {
        d_sourceNodeDescription = bsl::move(rhs.d_sourceNodeDescription);
        d_result                = bsl::move(rhs.d_result);
    }

    return *this;
}
#endif

void RouteResponseResult::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sourceNodeDescription);
    bdlat_ValueTypeFunctions::reset(&d_result);
}

// ACCESSORS

bsl::ostream& RouteResponseResult::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sourceNodeDescription",
                           this->sourceNodeDescription());
    printer.printAttribute("result", this->result());
    printer.end();
    return stream;
}

// -------------------
// class DomainsResult
// -------------------

// CONSTANTS

const char DomainsResult::CLASS_NAME[] = "DomainsResult";

const bdlat_SelectionInfo DomainsResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_SUCCESS,
     "success",
     sizeof("success") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DOMAIN_RESULT,
     "domainResult",
     sizeof("domainResult") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* DomainsResult::lookupSelectionInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            DomainsResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* DomainsResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_SUCCESS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS];
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_DOMAIN_RESULT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_RESULT];
    default: return 0;
    }
}

// CREATORS

DomainsResult::DomainsResult(const DomainsResult& original,
                             bslma::Allocator*    basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(original.d_success.object());
    } break;
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_RESULT: {
        new (d_domainResult.buffer())
            DomainResult(original.d_domainResult.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainsResult::DomainsResult(DomainsResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_RESULT: {
        new (d_domainResult.buffer())
            DomainResult(bsl::move(original.d_domainResult.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

DomainsResult::DomainsResult(DomainsResult&&   original,
                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_RESULT: {
        new (d_domainResult.buffer())
            DomainResult(bsl::move(original.d_domainResult.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

DomainsResult& DomainsResult::operator=(const DomainsResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SUCCESS: {
            makeSuccess(rhs.d_success.object());
        } break;
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_DOMAIN_RESULT: {
            makeDomainResult(rhs.d_domainResult.object());
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
DomainsResult& DomainsResult::operator=(DomainsResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_SUCCESS: {
            makeSuccess(bsl::move(rhs.d_success.object()));
        } break;
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_DOMAIN_RESULT: {
            makeDomainResult(bsl::move(rhs.d_domainResult.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void DomainsResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        d_success.object().~Void();
    } break;
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_DOMAIN_RESULT: {
        d_domainResult.object().~DomainResult();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int DomainsResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_SUCCESS: {
        makeSuccess();
    } break;
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_DOMAIN_RESULT: {
        makeDomainResult();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int DomainsResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Void& DomainsResult::makeSuccess()
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_success.object());
    }
    else {
        reset();
        new (d_success.buffer()) Void();
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

Void& DomainsResult::makeSuccess(const Void& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = value;
    }
    else {
        reset();
        new (d_success.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& DomainsResult::makeSuccess(Void&& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_success.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}
#endif

Error& DomainsResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& DomainsResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& DomainsResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

DomainResult& DomainsResult::makeDomainResult()
{
    if (SELECTION_ID_DOMAIN_RESULT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domainResult.object());
    }
    else {
        reset();
        new (d_domainResult.buffer()) DomainResult(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_RESULT;
    }

    return d_domainResult.object();
}

DomainResult& DomainsResult::makeDomainResult(const DomainResult& value)
{
    if (SELECTION_ID_DOMAIN_RESULT == d_selectionId) {
        d_domainResult.object() = value;
    }
    else {
        reset();
        new (d_domainResult.buffer()) DomainResult(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_RESULT;
    }

    return d_domainResult.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainResult& DomainsResult::makeDomainResult(DomainResult&& value)
{
    if (SELECTION_ID_DOMAIN_RESULT == d_selectionId) {
        d_domainResult.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domainResult.buffer())
            DomainResult(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_RESULT;
    }

    return d_domainResult.object();
}
#endif

// ACCESSORS

bsl::ostream&
DomainsResult::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS: {
        printer.printAttribute("success", d_success.object());
    } break;
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_DOMAIN_RESULT: {
        printer.printAttribute("domainResult", d_domainResult.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* DomainsResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_SUCCESS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS].name();
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_DOMAIN_RESULT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_RESULT].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------------------
// class RouteResponseResultList
// -----------------------------

// CONSTANTS

const char RouteResponseResultList::CLASS_NAME[] = "RouteResponseResultList";

const bdlat_AttributeInfo RouteResponseResultList::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_RESPONSES,
     "responses",
     sizeof("responses") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
RouteResponseResultList::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            RouteResponseResultList::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* RouteResponseResultList::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_RESPONSES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESPONSES];
    default: return 0;
    }
}

// CREATORS

RouteResponseResultList::RouteResponseResultList(
    bslma::Allocator* basicAllocator)
: d_responses(basicAllocator)
{
}

RouteResponseResultList::RouteResponseResultList(
    const RouteResponseResultList& original,
    bslma::Allocator*              basicAllocator)
: d_responses(original.d_responses, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouteResponseResultList::RouteResponseResultList(
    RouteResponseResultList&& original) noexcept
: d_responses(bsl::move(original.d_responses))
{
}

RouteResponseResultList::RouteResponseResultList(
    RouteResponseResultList&& original,
    bslma::Allocator*         basicAllocator)
: d_responses(bsl::move(original.d_responses), basicAllocator)
{
}
#endif

RouteResponseResultList::~RouteResponseResultList()
{
}

// MANIPULATORS

RouteResponseResultList&
RouteResponseResultList::operator=(const RouteResponseResultList& rhs)
{
    if (this != &rhs) {
        d_responses = rhs.d_responses;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
RouteResponseResultList&
RouteResponseResultList::operator=(RouteResponseResultList&& rhs)
{
    if (this != &rhs) {
        d_responses = bsl::move(rhs.d_responses);
    }

    return *this;
}
#endif

void RouteResponseResultList::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_responses);
}

// ACCESSORS

bsl::ostream& RouteResponseResultList::print(bsl::ostream& stream,
                                             int           level,
                                             int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("responses", this->responses());
    printer.end();
    return stream;
}

// --------------------
// class InternalResult
// --------------------

// CONSTANTS

const char InternalResult::CLASS_NAME[] = "InternalResult";

const bdlat_SelectionInfo InternalResult::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_ERROR,
     "error",
     sizeof("error") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_SUCCESS,
     "success",
     sizeof("success") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DOMAINS_RESULT,
     "domainsResult",
     sizeof("domainsResult") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTERS_RESULT,
     "clustersResult",
     sizeof("clustersResult") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_HELP,
     "help",
     sizeof("help") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_INTERNALS,
     "queueInternals",
     sizeof("queueInternals") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STAT_RESULT,
     "statResult",
     sizeof("statResult") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_LIST,
     "clusterList",
     sizeof("clusterList") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_QUEUE_HELPER,
     "clusterQueueHelper",
     sizeof("clusterQueueHelper") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_STORAGE_SUMMARY,
     "clusterStorageSummary",
     sizeof("clusterStorageSummary") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES,
     "clusterDomainQueueStatuses",
     sizeof("clusterDomainQueueStatuses") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_BROKER_CONFIG,
     "brokerConfig",
     sizeof("brokerConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
InternalResult::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 12; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            InternalResult::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* InternalResult::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ERROR:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR];
    case SELECTION_ID_SUCCESS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS];
    case SELECTION_ID_DOMAINS_RESULT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAINS_RESULT];
    case SELECTION_ID_CLUSTERS_RESULT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTERS_RESULT];
    case SELECTION_ID_HELP: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_HELP];
    case SELECTION_ID_QUEUE_INTERNALS:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_INTERNALS];
    case SELECTION_ID_STAT_RESULT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STAT_RESULT];
    case SELECTION_ID_CLUSTER_LIST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_LIST];
    case SELECTION_ID_CLUSTER_QUEUE_HELPER:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_QUEUE_HELPER];
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STORAGE_SUMMARY];
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES:
        return &SELECTION_INFO_ARRAY
            [SELECTION_INDEX_CLUSTER_DOMAIN_QUEUE_STATUSES];
    case SELECTION_ID_BROKER_CONFIG:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_BROKER_CONFIG];
    default: return 0;
    }
}

// CREATORS

InternalResult::InternalResult(const InternalResult& original,
                               bslma::Allocator*     basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer()) Error(original.d_error.object(), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(original.d_success.object());
    } break;
    case SELECTION_ID_DOMAINS_RESULT: {
        new (d_domainsResult.buffer())
            DomainsResult(original.d_domainsResult.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTERS_RESULT: {
        new (d_clustersResult.buffer())
            ClustersResult(original.d_clustersResult.object(), d_allocator_p);
    } break;
    case SELECTION_ID_HELP: {
        new (d_help.buffer()) Help(original.d_help.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        new (d_queueInternals.buffer())
            QueueInternals(original.d_queueInternals.object(), d_allocator_p);
    } break;
    case SELECTION_ID_STAT_RESULT: {
        new (d_statResult.buffer())
            StatResult(original.d_statResult.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        new (d_clusterList.buffer())
            ClusterList(original.d_clusterList.object(), d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        new (d_clusterQueueHelper.buffer())
            ClusterQueueHelper(original.d_clusterQueueHelper.object(),
                               d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(original.d_clusterStorageSummary.object(),
                                  d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        new (d_clusterDomainQueueStatuses.buffer()) ClusterDomainQueueStatuses(
            original.d_clusterDomainQueueStatuses.object(),
            d_allocator_p);
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        new (d_brokerConfig.buffer())
            BrokerConfig(original.d_brokerConfig.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
InternalResult::InternalResult(InternalResult&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_DOMAINS_RESULT: {
        new (d_domainsResult.buffer())
            DomainsResult(bsl::move(original.d_domainsResult.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTERS_RESULT: {
        new (d_clustersResult.buffer())
            ClustersResult(bsl::move(original.d_clustersResult.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_HELP: {
        new (d_help.buffer())
            Help(bsl::move(original.d_help.object()), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        new (d_queueInternals.buffer())
            QueueInternals(bsl::move(original.d_queueInternals.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_STAT_RESULT: {
        new (d_statResult.buffer())
            StatResult(bsl::move(original.d_statResult.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        new (d_clusterList.buffer())
            ClusterList(bsl::move(original.d_clusterList.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        new (d_clusterQueueHelper.buffer()) ClusterQueueHelper(
            bsl::move(original.d_clusterQueueHelper.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        new (d_clusterStorageSummary.buffer()) ClusterStorageSummary(
            bsl::move(original.d_clusterStorageSummary.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        new (d_clusterDomainQueueStatuses.buffer()) ClusterDomainQueueStatuses(
            bsl::move(original.d_clusterDomainQueueStatuses.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        new (d_brokerConfig.buffer())
            BrokerConfig(bsl::move(original.d_brokerConfig.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

InternalResult::InternalResult(InternalResult&&  original,
                               bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        new (d_error.buffer())
            Error(bsl::move(original.d_error.object()), d_allocator_p);
    } break;
    case SELECTION_ID_SUCCESS: {
        new (d_success.buffer()) Void(bsl::move(original.d_success.object()));
    } break;
    case SELECTION_ID_DOMAINS_RESULT: {
        new (d_domainsResult.buffer())
            DomainsResult(bsl::move(original.d_domainsResult.object()),
                          d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTERS_RESULT: {
        new (d_clustersResult.buffer())
            ClustersResult(bsl::move(original.d_clustersResult.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_HELP: {
        new (d_help.buffer())
            Help(bsl::move(original.d_help.object()), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        new (d_queueInternals.buffer())
            QueueInternals(bsl::move(original.d_queueInternals.object()),
                           d_allocator_p);
    } break;
    case SELECTION_ID_STAT_RESULT: {
        new (d_statResult.buffer())
            StatResult(bsl::move(original.d_statResult.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        new (d_clusterList.buffer())
            ClusterList(bsl::move(original.d_clusterList.object()),
                        d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        new (d_clusterQueueHelper.buffer()) ClusterQueueHelper(
            bsl::move(original.d_clusterQueueHelper.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        new (d_clusterStorageSummary.buffer()) ClusterStorageSummary(
            bsl::move(original.d_clusterStorageSummary.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        new (d_clusterDomainQueueStatuses.buffer()) ClusterDomainQueueStatuses(
            bsl::move(original.d_clusterDomainQueueStatuses.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        new (d_brokerConfig.buffer())
            BrokerConfig(bsl::move(original.d_brokerConfig.object()),
                         d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

InternalResult& InternalResult::operator=(const InternalResult& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(rhs.d_error.object());
        } break;
        case SELECTION_ID_SUCCESS: {
            makeSuccess(rhs.d_success.object());
        } break;
        case SELECTION_ID_DOMAINS_RESULT: {
            makeDomainsResult(rhs.d_domainsResult.object());
        } break;
        case SELECTION_ID_CLUSTERS_RESULT: {
            makeClustersResult(rhs.d_clustersResult.object());
        } break;
        case SELECTION_ID_HELP: {
            makeHelp(rhs.d_help.object());
        } break;
        case SELECTION_ID_QUEUE_INTERNALS: {
            makeQueueInternals(rhs.d_queueInternals.object());
        } break;
        case SELECTION_ID_STAT_RESULT: {
            makeStatResult(rhs.d_statResult.object());
        } break;
        case SELECTION_ID_CLUSTER_LIST: {
            makeClusterList(rhs.d_clusterList.object());
        } break;
        case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
            makeClusterQueueHelper(rhs.d_clusterQueueHelper.object());
        } break;
        case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
            makeClusterStorageSummary(rhs.d_clusterStorageSummary.object());
        } break;
        case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
            makeClusterDomainQueueStatuses(
                rhs.d_clusterDomainQueueStatuses.object());
        } break;
        case SELECTION_ID_BROKER_CONFIG: {
            makeBrokerConfig(rhs.d_brokerConfig.object());
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
InternalResult& InternalResult::operator=(InternalResult&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ERROR: {
            makeError(bsl::move(rhs.d_error.object()));
        } break;
        case SELECTION_ID_SUCCESS: {
            makeSuccess(bsl::move(rhs.d_success.object()));
        } break;
        case SELECTION_ID_DOMAINS_RESULT: {
            makeDomainsResult(bsl::move(rhs.d_domainsResult.object()));
        } break;
        case SELECTION_ID_CLUSTERS_RESULT: {
            makeClustersResult(bsl::move(rhs.d_clustersResult.object()));
        } break;
        case SELECTION_ID_HELP: {
            makeHelp(bsl::move(rhs.d_help.object()));
        } break;
        case SELECTION_ID_QUEUE_INTERNALS: {
            makeQueueInternals(bsl::move(rhs.d_queueInternals.object()));
        } break;
        case SELECTION_ID_STAT_RESULT: {
            makeStatResult(bsl::move(rhs.d_statResult.object()));
        } break;
        case SELECTION_ID_CLUSTER_LIST: {
            makeClusterList(bsl::move(rhs.d_clusterList.object()));
        } break;
        case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
            makeClusterQueueHelper(
                bsl::move(rhs.d_clusterQueueHelper.object()));
        } break;
        case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
            makeClusterStorageSummary(
                bsl::move(rhs.d_clusterStorageSummary.object()));
        } break;
        case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
            makeClusterDomainQueueStatuses(
                bsl::move(rhs.d_clusterDomainQueueStatuses.object()));
        } break;
        case SELECTION_ID_BROKER_CONFIG: {
            makeBrokerConfig(bsl::move(rhs.d_brokerConfig.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void InternalResult::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        d_error.object().~Error();
    } break;
    case SELECTION_ID_SUCCESS: {
        d_success.object().~Void();
    } break;
    case SELECTION_ID_DOMAINS_RESULT: {
        d_domainsResult.object().~DomainsResult();
    } break;
    case SELECTION_ID_CLUSTERS_RESULT: {
        d_clustersResult.object().~ClustersResult();
    } break;
    case SELECTION_ID_HELP: {
        d_help.object().~Help();
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        d_queueInternals.object().~QueueInternals();
    } break;
    case SELECTION_ID_STAT_RESULT: {
        d_statResult.object().~StatResult();
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        d_clusterList.object().~ClusterList();
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        d_clusterQueueHelper.object().~ClusterQueueHelper();
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        d_clusterStorageSummary.object().~ClusterStorageSummary();
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        d_clusterDomainQueueStatuses.object().~ClusterDomainQueueStatuses();
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        d_brokerConfig.object().~BrokerConfig();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int InternalResult::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ERROR: {
        makeError();
    } break;
    case SELECTION_ID_SUCCESS: {
        makeSuccess();
    } break;
    case SELECTION_ID_DOMAINS_RESULT: {
        makeDomainsResult();
    } break;
    case SELECTION_ID_CLUSTERS_RESULT: {
        makeClustersResult();
    } break;
    case SELECTION_ID_HELP: {
        makeHelp();
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        makeQueueInternals();
    } break;
    case SELECTION_ID_STAT_RESULT: {
        makeStatResult();
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        makeClusterList();
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        makeClusterQueueHelper();
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        makeClusterStorageSummary();
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        makeClusterDomainQueueStatuses();
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        makeBrokerConfig();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int InternalResult::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Error& InternalResult::makeError()
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_error.object());
    }
    else {
        reset();
        new (d_error.buffer()) Error(d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

Error& InternalResult::makeError(const Error& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = value;
    }
    else {
        reset();
        new (d_error.buffer()) Error(value, d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Error& InternalResult::makeError(Error&& value)
{
    if (SELECTION_ID_ERROR == d_selectionId) {
        d_error.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_error.buffer()) Error(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_ERROR;
    }

    return d_error.object();
}
#endif

Void& InternalResult::makeSuccess()
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_success.object());
    }
    else {
        reset();
        new (d_success.buffer()) Void();
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

Void& InternalResult::makeSuccess(const Void& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = value;
    }
    else {
        reset();
        new (d_success.buffer()) Void(value);
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Void& InternalResult::makeSuccess(Void&& value)
{
    if (SELECTION_ID_SUCCESS == d_selectionId) {
        d_success.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_success.buffer()) Void(bsl::move(value));
        d_selectionId = SELECTION_ID_SUCCESS;
    }

    return d_success.object();
}
#endif

DomainsResult& InternalResult::makeDomainsResult()
{
    if (SELECTION_ID_DOMAINS_RESULT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domainsResult.object());
    }
    else {
        reset();
        new (d_domainsResult.buffer()) DomainsResult(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAINS_RESULT;
    }

    return d_domainsResult.object();
}

DomainsResult& InternalResult::makeDomainsResult(const DomainsResult& value)
{
    if (SELECTION_ID_DOMAINS_RESULT == d_selectionId) {
        d_domainsResult.object() = value;
    }
    else {
        reset();
        new (d_domainsResult.buffer()) DomainsResult(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAINS_RESULT;
    }

    return d_domainsResult.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainsResult& InternalResult::makeDomainsResult(DomainsResult&& value)
{
    if (SELECTION_ID_DOMAINS_RESULT == d_selectionId) {
        d_domainsResult.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domainsResult.buffer())
            DomainsResult(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAINS_RESULT;
    }

    return d_domainsResult.object();
}
#endif

ClustersResult& InternalResult::makeClustersResult()
{
    if (SELECTION_ID_CLUSTERS_RESULT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clustersResult.object());
    }
    else {
        reset();
        new (d_clustersResult.buffer()) ClustersResult(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTERS_RESULT;
    }

    return d_clustersResult.object();
}

ClustersResult& InternalResult::makeClustersResult(const ClustersResult& value)
{
    if (SELECTION_ID_CLUSTERS_RESULT == d_selectionId) {
        d_clustersResult.object() = value;
    }
    else {
        reset();
        new (d_clustersResult.buffer()) ClustersResult(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTERS_RESULT;
    }

    return d_clustersResult.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClustersResult& InternalResult::makeClustersResult(ClustersResult&& value)
{
    if (SELECTION_ID_CLUSTERS_RESULT == d_selectionId) {
        d_clustersResult.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clustersResult.buffer())
            ClustersResult(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTERS_RESULT;
    }

    return d_clustersResult.object();
}
#endif

Help& InternalResult::makeHelp()
{
    if (SELECTION_ID_HELP == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_help.object());
    }
    else {
        reset();
        new (d_help.buffer()) Help(d_allocator_p);
        d_selectionId = SELECTION_ID_HELP;
    }

    return d_help.object();
}

Help& InternalResult::makeHelp(const Help& value)
{
    if (SELECTION_ID_HELP == d_selectionId) {
        d_help.object() = value;
    }
    else {
        reset();
        new (d_help.buffer()) Help(value, d_allocator_p);
        d_selectionId = SELECTION_ID_HELP;
    }

    return d_help.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Help& InternalResult::makeHelp(Help&& value)
{
    if (SELECTION_ID_HELP == d_selectionId) {
        d_help.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_help.buffer()) Help(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_HELP;
    }

    return d_help.object();
}
#endif

QueueInternals& InternalResult::makeQueueInternals()
{
    if (SELECTION_ID_QUEUE_INTERNALS == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueInternals.object());
    }
    else {
        reset();
        new (d_queueInternals.buffer()) QueueInternals(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_INTERNALS;
    }

    return d_queueInternals.object();
}

QueueInternals& InternalResult::makeQueueInternals(const QueueInternals& value)
{
    if (SELECTION_ID_QUEUE_INTERNALS == d_selectionId) {
        d_queueInternals.object() = value;
    }
    else {
        reset();
        new (d_queueInternals.buffer()) QueueInternals(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_INTERNALS;
    }

    return d_queueInternals.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueInternals& InternalResult::makeQueueInternals(QueueInternals&& value)
{
    if (SELECTION_ID_QUEUE_INTERNALS == d_selectionId) {
        d_queueInternals.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueInternals.buffer())
            QueueInternals(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_INTERNALS;
    }

    return d_queueInternals.object();
}
#endif

StatResult& InternalResult::makeStatResult()
{
    if (SELECTION_ID_STAT_RESULT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_statResult.object());
    }
    else {
        reset();
        new (d_statResult.buffer()) StatResult(d_allocator_p);
        d_selectionId = SELECTION_ID_STAT_RESULT;
    }

    return d_statResult.object();
}

StatResult& InternalResult::makeStatResult(const StatResult& value)
{
    if (SELECTION_ID_STAT_RESULT == d_selectionId) {
        d_statResult.object() = value;
    }
    else {
        reset();
        new (d_statResult.buffer()) StatResult(value, d_allocator_p);
        d_selectionId = SELECTION_ID_STAT_RESULT;
    }

    return d_statResult.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatResult& InternalResult::makeStatResult(StatResult&& value)
{
    if (SELECTION_ID_STAT_RESULT == d_selectionId) {
        d_statResult.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_statResult.buffer())
            StatResult(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_STAT_RESULT;
    }

    return d_statResult.object();
}
#endif

ClusterList& InternalResult::makeClusterList()
{
    if (SELECTION_ID_CLUSTER_LIST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterList.object());
    }
    else {
        reset();
        new (d_clusterList.buffer()) ClusterList(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_LIST;
    }

    return d_clusterList.object();
}

ClusterList& InternalResult::makeClusterList(const ClusterList& value)
{
    if (SELECTION_ID_CLUSTER_LIST == d_selectionId) {
        d_clusterList.object() = value;
    }
    else {
        reset();
        new (d_clusterList.buffer()) ClusterList(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_LIST;
    }

    return d_clusterList.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterList& InternalResult::makeClusterList(ClusterList&& value)
{
    if (SELECTION_ID_CLUSTER_LIST == d_selectionId) {
        d_clusterList.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterList.buffer())
            ClusterList(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_LIST;
    }

    return d_clusterList.object();
}
#endif

ClusterQueueHelper& InternalResult::makeClusterQueueHelper()
{
    if (SELECTION_ID_CLUSTER_QUEUE_HELPER == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterQueueHelper.object());
    }
    else {
        reset();
        new (d_clusterQueueHelper.buffer()) ClusterQueueHelper(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_QUEUE_HELPER;
    }

    return d_clusterQueueHelper.object();
}

ClusterQueueHelper&
InternalResult::makeClusterQueueHelper(const ClusterQueueHelper& value)
{
    if (SELECTION_ID_CLUSTER_QUEUE_HELPER == d_selectionId) {
        d_clusterQueueHelper.object() = value;
    }
    else {
        reset();
        new (d_clusterQueueHelper.buffer())
            ClusterQueueHelper(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_QUEUE_HELPER;
    }

    return d_clusterQueueHelper.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterQueueHelper&
InternalResult::makeClusterQueueHelper(ClusterQueueHelper&& value)
{
    if (SELECTION_ID_CLUSTER_QUEUE_HELPER == d_selectionId) {
        d_clusterQueueHelper.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterQueueHelper.buffer())
            ClusterQueueHelper(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_QUEUE_HELPER;
    }

    return d_clusterQueueHelper.object();
}
#endif

ClusterStorageSummary& InternalResult::makeClusterStorageSummary()
{
    if (SELECTION_ID_CLUSTER_STORAGE_SUMMARY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_clusterStorageSummary.object());
    }
    else {
        reset();
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STORAGE_SUMMARY;
    }

    return d_clusterStorageSummary.object();
}

ClusterStorageSummary&
InternalResult::makeClusterStorageSummary(const ClusterStorageSummary& value)
{
    if (SELECTION_ID_CLUSTER_STORAGE_SUMMARY == d_selectionId) {
        d_clusterStorageSummary.object() = value;
    }
    else {
        reset();
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STORAGE_SUMMARY;
    }

    return d_clusterStorageSummary.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterStorageSummary&
InternalResult::makeClusterStorageSummary(ClusterStorageSummary&& value)
{
    if (SELECTION_ID_CLUSTER_STORAGE_SUMMARY == d_selectionId) {
        d_clusterStorageSummary.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterStorageSummary.buffer())
            ClusterStorageSummary(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_STORAGE_SUMMARY;
    }

    return d_clusterStorageSummary.object();
}
#endif

ClusterDomainQueueStatuses& InternalResult::makeClusterDomainQueueStatuses()
{
    if (SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(
            &d_clusterDomainQueueStatuses.object());
    }
    else {
        reset();
        new (d_clusterDomainQueueStatuses.buffer())
            ClusterDomainQueueStatuses(d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES;
    }

    return d_clusterDomainQueueStatuses.object();
}

ClusterDomainQueueStatuses& InternalResult::makeClusterDomainQueueStatuses(
    const ClusterDomainQueueStatuses& value)
{
    if (SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES == d_selectionId) {
        d_clusterDomainQueueStatuses.object() = value;
    }
    else {
        reset();
        new (d_clusterDomainQueueStatuses.buffer())
            ClusterDomainQueueStatuses(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES;
    }

    return d_clusterDomainQueueStatuses.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterDomainQueueStatuses& InternalResult::makeClusterDomainQueueStatuses(
    ClusterDomainQueueStatuses&& value)
{
    if (SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES == d_selectionId) {
        d_clusterDomainQueueStatuses.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_clusterDomainQueueStatuses.buffer())
            ClusterDomainQueueStatuses(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES;
    }

    return d_clusterDomainQueueStatuses.object();
}
#endif

BrokerConfig& InternalResult::makeBrokerConfig()
{
    if (SELECTION_ID_BROKER_CONFIG == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_brokerConfig.object());
    }
    else {
        reset();
        new (d_brokerConfig.buffer()) BrokerConfig(d_allocator_p);
        d_selectionId = SELECTION_ID_BROKER_CONFIG;
    }

    return d_brokerConfig.object();
}

BrokerConfig& InternalResult::makeBrokerConfig(const BrokerConfig& value)
{
    if (SELECTION_ID_BROKER_CONFIG == d_selectionId) {
        d_brokerConfig.object() = value;
    }
    else {
        reset();
        new (d_brokerConfig.buffer()) BrokerConfig(value, d_allocator_p);
        d_selectionId = SELECTION_ID_BROKER_CONFIG;
    }

    return d_brokerConfig.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerConfig& InternalResult::makeBrokerConfig(BrokerConfig&& value)
{
    if (SELECTION_ID_BROKER_CONFIG == d_selectionId) {
        d_brokerConfig.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_brokerConfig.buffer())
            BrokerConfig(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_BROKER_CONFIG;
    }

    return d_brokerConfig.object();
}
#endif

// ACCESSORS

bsl::ostream& InternalResult::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ERROR: {
        printer.printAttribute("error", d_error.object());
    } break;
    case SELECTION_ID_SUCCESS: {
        printer.printAttribute("success", d_success.object());
    } break;
    case SELECTION_ID_DOMAINS_RESULT: {
        printer.printAttribute("domainsResult", d_domainsResult.object());
    } break;
    case SELECTION_ID_CLUSTERS_RESULT: {
        printer.printAttribute("clustersResult", d_clustersResult.object());
    } break;
    case SELECTION_ID_HELP: {
        printer.printAttribute("help", d_help.object());
    } break;
    case SELECTION_ID_QUEUE_INTERNALS: {
        printer.printAttribute("queueInternals", d_queueInternals.object());
    } break;
    case SELECTION_ID_STAT_RESULT: {
        printer.printAttribute("statResult", d_statResult.object());
    } break;
    case SELECTION_ID_CLUSTER_LIST: {
        printer.printAttribute("clusterList", d_clusterList.object());
    } break;
    case SELECTION_ID_CLUSTER_QUEUE_HELPER: {
        printer.printAttribute("clusterQueueHelper",
                               d_clusterQueueHelper.object());
    } break;
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY: {
        printer.printAttribute("clusterStorageSummary",
                               d_clusterStorageSummary.object());
    } break;
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES: {
        printer.printAttribute("clusterDomainQueueStatuses",
                               d_clusterDomainQueueStatuses.object());
    } break;
    case SELECTION_ID_BROKER_CONFIG: {
        printer.printAttribute("brokerConfig", d_brokerConfig.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* InternalResult::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ERROR:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ERROR].name();
    case SELECTION_ID_SUCCESS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_SUCCESS].name();
    case SELECTION_ID_DOMAINS_RESULT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAINS_RESULT].name();
    case SELECTION_ID_CLUSTERS_RESULT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTERS_RESULT].name();
    case SELECTION_ID_HELP:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_HELP].name();
    case SELECTION_ID_QUEUE_INTERNALS:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_INTERNALS].name();
    case SELECTION_ID_STAT_RESULT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STAT_RESULT].name();
    case SELECTION_ID_CLUSTER_LIST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_LIST].name();
    case SELECTION_ID_CLUSTER_QUEUE_HELPER:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_QUEUE_HELPER]
            .name();
    case SELECTION_ID_CLUSTER_STORAGE_SUMMARY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CLUSTER_STORAGE_SUMMARY]
            .name();
    case SELECTION_ID_CLUSTER_DOMAIN_QUEUE_STATUSES:
        return SELECTION_INFO_ARRAY
            [SELECTION_INDEX_CLUSTER_DOMAIN_QUEUE_STATUSES]
                .name();
    case SELECTION_ID_BROKER_CONFIG:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_BROKER_CONFIG].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}
}  // close package namespace
}  // close enterprise namespace

// GENERATED BY @BLP_BAS_CODEGEN_VERSION@
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbcmd --msgComponent messages mqbcmd.xsd
// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright 2024 Bloomberg Finance L.P. All rights reserved.
//      Property of Bloomberg Finance L.P. (BFLP)
//      This software is made available solely pursuant to the
//      terms of a BFLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
