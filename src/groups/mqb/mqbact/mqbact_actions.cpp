// Copyright 2026 Bloomberg Finance L.P.
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

// mqbact_actions.cpp            *DO NOT EDIT*             @generated -*-C++-*-

#include <mqbact_actions.h>

#include <bdlat_formattingmode.h>
#include <bdlat_valuetypefunctions.h>
#include <bdlb_print.h>
#include <bdlb_printmethods.h>
#include <bdlb_string.h>

#include <bsl_string.h>
#include <bslim_printer.h>
#include <bsls_assert.h>

#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>

namespace BloombergLP {
namespace mqbact {

// ------------------
// class ConnectAdmin
// ------------------

// CONSTANTS

const char ConnectAdmin::CLASS_NAME[] = "ConnectAdmin";

// CLASS METHODS

const bdlat_AttributeInfo* ConnectAdmin::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* ConnectAdmin::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void ConnectAdmin::reset()
{
}

// ACCESSORS

bsl::ostream& ConnectAdmin::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// -------------------
// class ConnectClient
// -------------------

// CONSTANTS

const char ConnectClient::CLASS_NAME[] = "ConnectClient";

// CLASS METHODS

const bdlat_AttributeInfo* ConnectClient::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* ConnectClient::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void ConnectClient::reset()
{
}

// ACCESSORS

bsl::ostream& ConnectClient::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// ------------------------
// class ConnectClusterNode
// ------------------------

// CONSTANTS

const char ConnectClusterNode::CLASS_NAME[] = "ConnectClusterNode";

const bdlat_AttributeInfo ConnectClusterNode::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLUSTER_NAME,
     "clusterName",
     sizeof("clusterName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ConnectClusterNode::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ConnectClusterNode::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ConnectClusterNode::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLUSTER_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME];
    default: return 0;
    }
}

// CREATORS

ConnectClusterNode::ConnectClusterNode(bslma::Allocator* basicAllocator)
: d_clusterName(basicAllocator)
{
}

ConnectClusterNode::ConnectClusterNode(const ConnectClusterNode& original,
                                       bslma::Allocator* basicAllocator)
: d_clusterName(original.d_clusterName, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConnectClusterNode::ConnectClusterNode(ConnectClusterNode&& original) noexcept
: d_clusterName(bsl::move(original.d_clusterName))
{
}

ConnectClusterNode::ConnectClusterNode(ConnectClusterNode&& original,
                                       bslma::Allocator*    basicAllocator)
: d_clusterName(bsl::move(original.d_clusterName), basicAllocator)
{
}
#endif

ConnectClusterNode::~ConnectClusterNode()
{
}

// MANIPULATORS

ConnectClusterNode&
ConnectClusterNode::operator=(const ConnectClusterNode& rhs)
{
    if (this != &rhs) {
        d_clusterName = rhs.d_clusterName;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConnectClusterNode& ConnectClusterNode::operator=(ConnectClusterNode&& rhs)
{
    if (this != &rhs) {
        d_clusterName = bsl::move(rhs.d_clusterName);
    }

    return *this;
}
#endif

void ConnectClusterNode::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_clusterName);
}

// ACCESSORS

bsl::ostream& ConnectClusterNode::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("clusterName", this->clusterName());
    printer.end();
    return stream;
}

// ------------------
// class ConnectProxy
// ------------------

// CONSTANTS

const char ConnectProxy::CLASS_NAME[] = "ConnectProxy";

// CLASS METHODS

const bdlat_AttributeInfo* ConnectProxy::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* ConnectProxy::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

// MANIPULATORS

void ConnectProxy::reset()
{
}

// ACCESSORS

bsl::ostream& ConnectProxy::print(bsl::ostream& stream, int, int) const
{
    return stream;
}

// -------------------------
// class ExecuteAdminCommand
// -------------------------

// CONSTANTS

const char ExecuteAdminCommand::CLASS_NAME[] = "ExecuteAdminCommand";

const bdlat_AttributeInfo ExecuteAdminCommand::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_COMMAND,
     "command",
     sizeof("command") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ExecuteAdminCommand::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ExecuteAdminCommand::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ExecuteAdminCommand::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_COMMAND:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND];
    default: return 0;
    }
}

// CREATORS

ExecuteAdminCommand::ExecuteAdminCommand(bslma::Allocator* basicAllocator)
: d_command(basicAllocator)
{
}

ExecuteAdminCommand::ExecuteAdminCommand(const ExecuteAdminCommand& original,
                                         bslma::Allocator* basicAllocator)
: d_command(original.d_command, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ExecuteAdminCommand::ExecuteAdminCommand(ExecuteAdminCommand&& original)
    noexcept : d_command(bsl::move(original.d_command))
{
}

ExecuteAdminCommand::ExecuteAdminCommand(ExecuteAdminCommand&& original,
                                         bslma::Allocator*     basicAllocator)
: d_command(bsl::move(original.d_command), basicAllocator)
{
}
#endif

ExecuteAdminCommand::~ExecuteAdminCommand()
{
}

// MANIPULATORS

ExecuteAdminCommand&
ExecuteAdminCommand::operator=(const ExecuteAdminCommand& rhs)
{
    if (this != &rhs) {
        d_command = rhs.d_command;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ExecuteAdminCommand& ExecuteAdminCommand::operator=(ExecuteAdminCommand&& rhs)
{
    if (this != &rhs) {
        d_command = bsl::move(rhs.d_command);
    }

    return *this;
}
#endif

void ExecuteAdminCommand::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_command);
}

// ACCESSORS

bsl::ostream& ExecuteAdminCommand::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("command", this->command());
    printer.end();
    return stream;
}

// ---------------
// class QueueRead
// ---------------

// CONSTANTS

const char QueueRead::CLASS_NAME[] = "QueueRead";

const bdlat_AttributeInfo QueueRead::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* QueueRead::lookupAttributeInfo(const char* name,
                                                          int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueRead::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueRead::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    default: return 0;
    }
}

// CREATORS

QueueRead::QueueRead(bslma::Allocator* basicAllocator)
: d_uri(basicAllocator)
{
}

QueueRead::QueueRead(const QueueRead&  original,
                     bslma::Allocator* basicAllocator)
: d_uri(original.d_uri, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueRead::QueueRead(QueueRead&& original) noexcept
: d_uri(bsl::move(original.d_uri))
{
}

QueueRead::QueueRead(QueueRead&& original, bslma::Allocator* basicAllocator)
: d_uri(bsl::move(original.d_uri), basicAllocator)
{
}
#endif

QueueRead::~QueueRead()
{
}

// MANIPULATORS

QueueRead& QueueRead::operator=(const QueueRead& rhs)
{
    if (this != &rhs) {
        d_uri = rhs.d_uri;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueRead& QueueRead::operator=(QueueRead&& rhs)
{
    if (this != &rhs) {
        d_uri = bsl::move(rhs.d_uri);
    }

    return *this;
}
#endif

void QueueRead::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
}

// ACCESSORS

bsl::ostream&
QueueRead::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.end();
    return stream;
}

// ----------------
// class QueueWrite
// ----------------

// CONSTANTS

const char QueueWrite::CLASS_NAME[] = "QueueWrite";

const bdlat_AttributeInfo QueueWrite::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_URI,
     "uri",
     sizeof("uri") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* QueueWrite::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueWrite::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueWrite::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_URI: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI];
    default: return 0;
    }
}

// CREATORS

QueueWrite::QueueWrite(bslma::Allocator* basicAllocator)
: d_uri(basicAllocator)
{
}

QueueWrite::QueueWrite(const QueueWrite& original,
                       bslma::Allocator* basicAllocator)
: d_uri(original.d_uri, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueWrite::QueueWrite(QueueWrite&& original) noexcept
: d_uri(bsl::move(original.d_uri))
{
}

QueueWrite::QueueWrite(QueueWrite&& original, bslma::Allocator* basicAllocator)
: d_uri(bsl::move(original.d_uri), basicAllocator)
{
}
#endif

QueueWrite::~QueueWrite()
{
}

// MANIPULATORS

QueueWrite& QueueWrite::operator=(const QueueWrite& rhs)
{
    if (this != &rhs) {
        d_uri = rhs.d_uri;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueWrite& QueueWrite::operator=(QueueWrite&& rhs)
{
    if (this != &rhs) {
        d_uri = bsl::move(rhs.d_uri);
    }

    return *this;
}
#endif

void QueueWrite::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_uri);
}

// ACCESSORS

bsl::ostream&
QueueWrite::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", this->uri());
    printer.end();
    return stream;
}

// ------------
// class Action
// ------------

// CONSTANTS

const char Action::CLASS_NAME[] = "Action";

const bdlat_SelectionInfo Action::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_CONNECT_CLIENT,
     "connectClient",
     sizeof("connectClient") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONNECT_PROXY,
     "connectProxy",
     sizeof("connectProxy") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONNECT_ADMIN,
     "connectAdmin",
     sizeof("connectAdmin") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_CONNECT_CLUSTER_NODE,
     "connectClusterNode",
     sizeof("connectClusterNode") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_READ,
     "queueRead",
     sizeof("queueRead") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_QUEUE_WRITE,
     "queueWrite",
     sizeof("queueWrite") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_EXECUTE_ADMIN_COMMAND,
     "executeAdminCommand",
     sizeof("executeAdminCommand") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* Action::lookupSelectionInfo(const char* name,
                                                       int         nameLength)
{
    for (int i = 0; i < 7; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            Action::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* Action::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_CONNECT_CLIENT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_CLIENT];
    case SELECTION_ID_CONNECT_PROXY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_PROXY];
    case SELECTION_ID_CONNECT_ADMIN:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_ADMIN];
    case SELECTION_ID_CONNECT_CLUSTER_NODE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_CLUSTER_NODE];
    case SELECTION_ID_QUEUE_READ:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_READ];
    case SELECTION_ID_QUEUE_WRITE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_WRITE];
    case SELECTION_ID_EXECUTE_ADMIN_COMMAND:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_EXECUTE_ADMIN_COMMAND];
    default: return 0;
    }
}

// CREATORS

Action::Action(const Action& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_CONNECT_CLIENT: {
        new (d_connectClient.buffer())
            ConnectClient(original.d_connectClient.object());
    } break;
    case SELECTION_ID_CONNECT_PROXY: {
        new (d_connectProxy.buffer())
            ConnectProxy(original.d_connectProxy.object());
    } break;
    case SELECTION_ID_CONNECT_ADMIN: {
        new (d_connectAdmin.buffer())
            ConnectAdmin(original.d_connectAdmin.object());
    } break;
    case SELECTION_ID_CONNECT_CLUSTER_NODE: {
        new (d_connectClusterNode.buffer())
            ConnectClusterNode(original.d_connectClusterNode.object(),
                               d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_READ: {
        new (d_queueRead.buffer())
            QueueRead(original.d_queueRead.object(), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_WRITE: {
        new (d_queueWrite.buffer())
            QueueWrite(original.d_queueWrite.object(), d_allocator_p);
    } break;
    case SELECTION_ID_EXECUTE_ADMIN_COMMAND: {
        new (d_executeAdminCommand.buffer())
            ExecuteAdminCommand(original.d_executeAdminCommand.object(),
                                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Action::Action(Action&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_CONNECT_CLIENT: {
        new (d_connectClient.buffer())
            ConnectClient(bsl::move(original.d_connectClient.object()));
    } break;
    case SELECTION_ID_CONNECT_PROXY: {
        new (d_connectProxy.buffer())
            ConnectProxy(bsl::move(original.d_connectProxy.object()));
    } break;
    case SELECTION_ID_CONNECT_ADMIN: {
        new (d_connectAdmin.buffer())
            ConnectAdmin(bsl::move(original.d_connectAdmin.object()));
    } break;
    case SELECTION_ID_CONNECT_CLUSTER_NODE: {
        new (d_connectClusterNode.buffer()) ConnectClusterNode(
            bsl::move(original.d_connectClusterNode.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_READ: {
        new (d_queueRead.buffer())
            QueueRead(bsl::move(original.d_queueRead.object()), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_WRITE: {
        new (d_queueWrite.buffer())
            QueueWrite(bsl::move(original.d_queueWrite.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_EXECUTE_ADMIN_COMMAND: {
        new (d_executeAdminCommand.buffer()) ExecuteAdminCommand(
            bsl::move(original.d_executeAdminCommand.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

Action::Action(Action&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_CONNECT_CLIENT: {
        new (d_connectClient.buffer())
            ConnectClient(bsl::move(original.d_connectClient.object()));
    } break;
    case SELECTION_ID_CONNECT_PROXY: {
        new (d_connectProxy.buffer())
            ConnectProxy(bsl::move(original.d_connectProxy.object()));
    } break;
    case SELECTION_ID_CONNECT_ADMIN: {
        new (d_connectAdmin.buffer())
            ConnectAdmin(bsl::move(original.d_connectAdmin.object()));
    } break;
    case SELECTION_ID_CONNECT_CLUSTER_NODE: {
        new (d_connectClusterNode.buffer()) ConnectClusterNode(
            bsl::move(original.d_connectClusterNode.object()),
            d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_READ: {
        new (d_queueRead.buffer())
            QueueRead(bsl::move(original.d_queueRead.object()), d_allocator_p);
    } break;
    case SELECTION_ID_QUEUE_WRITE: {
        new (d_queueWrite.buffer())
            QueueWrite(bsl::move(original.d_queueWrite.object()),
                       d_allocator_p);
    } break;
    case SELECTION_ID_EXECUTE_ADMIN_COMMAND: {
        new (d_executeAdminCommand.buffer()) ExecuteAdminCommand(
            bsl::move(original.d_executeAdminCommand.object()),
            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

Action& Action::operator=(const Action& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_CONNECT_CLIENT: {
            makeConnectClient(rhs.d_connectClient.object());
        } break;
        case SELECTION_ID_CONNECT_PROXY: {
            makeConnectProxy(rhs.d_connectProxy.object());
        } break;
        case SELECTION_ID_CONNECT_ADMIN: {
            makeConnectAdmin(rhs.d_connectAdmin.object());
        } break;
        case SELECTION_ID_CONNECT_CLUSTER_NODE: {
            makeConnectClusterNode(rhs.d_connectClusterNode.object());
        } break;
        case SELECTION_ID_QUEUE_READ: {
            makeQueueRead(rhs.d_queueRead.object());
        } break;
        case SELECTION_ID_QUEUE_WRITE: {
            makeQueueWrite(rhs.d_queueWrite.object());
        } break;
        case SELECTION_ID_EXECUTE_ADMIN_COMMAND: {
            makeExecuteAdminCommand(rhs.d_executeAdminCommand.object());
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
Action& Action::operator=(Action&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_CONNECT_CLIENT: {
            makeConnectClient(bsl::move(rhs.d_connectClient.object()));
        } break;
        case SELECTION_ID_CONNECT_PROXY: {
            makeConnectProxy(bsl::move(rhs.d_connectProxy.object()));
        } break;
        case SELECTION_ID_CONNECT_ADMIN: {
            makeConnectAdmin(bsl::move(rhs.d_connectAdmin.object()));
        } break;
        case SELECTION_ID_CONNECT_CLUSTER_NODE: {
            makeConnectClusterNode(
                bsl::move(rhs.d_connectClusterNode.object()));
        } break;
        case SELECTION_ID_QUEUE_READ: {
            makeQueueRead(bsl::move(rhs.d_queueRead.object()));
        } break;
        case SELECTION_ID_QUEUE_WRITE: {
            makeQueueWrite(bsl::move(rhs.d_queueWrite.object()));
        } break;
        case SELECTION_ID_EXECUTE_ADMIN_COMMAND: {
            makeExecuteAdminCommand(
                bsl::move(rhs.d_executeAdminCommand.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void Action::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_CONNECT_CLIENT: {
        d_connectClient.object().~ConnectClient();
    } break;
    case SELECTION_ID_CONNECT_PROXY: {
        d_connectProxy.object().~ConnectProxy();
    } break;
    case SELECTION_ID_CONNECT_ADMIN: {
        d_connectAdmin.object().~ConnectAdmin();
    } break;
    case SELECTION_ID_CONNECT_CLUSTER_NODE: {
        d_connectClusterNode.object().~ConnectClusterNode();
    } break;
    case SELECTION_ID_QUEUE_READ: {
        d_queueRead.object().~QueueRead();
    } break;
    case SELECTION_ID_QUEUE_WRITE: {
        d_queueWrite.object().~QueueWrite();
    } break;
    case SELECTION_ID_EXECUTE_ADMIN_COMMAND: {
        d_executeAdminCommand.object().~ExecuteAdminCommand();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int Action::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_CONNECT_CLIENT: {
        makeConnectClient();
    } break;
    case SELECTION_ID_CONNECT_PROXY: {
        makeConnectProxy();
    } break;
    case SELECTION_ID_CONNECT_ADMIN: {
        makeConnectAdmin();
    } break;
    case SELECTION_ID_CONNECT_CLUSTER_NODE: {
        makeConnectClusterNode();
    } break;
    case SELECTION_ID_QUEUE_READ: {
        makeQueueRead();
    } break;
    case SELECTION_ID_QUEUE_WRITE: {
        makeQueueWrite();
    } break;
    case SELECTION_ID_EXECUTE_ADMIN_COMMAND: {
        makeExecuteAdminCommand();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int Action::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

ConnectClient& Action::makeConnectClient()
{
    if (SELECTION_ID_CONNECT_CLIENT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_connectClient.object());
    }
    else {
        reset();
        new (d_connectClient.buffer()) ConnectClient();
        d_selectionId = SELECTION_ID_CONNECT_CLIENT;
    }

    return d_connectClient.object();
}

ConnectClient& Action::makeConnectClient(const ConnectClient& value)
{
    if (SELECTION_ID_CONNECT_CLIENT == d_selectionId) {
        d_connectClient.object() = value;
    }
    else {
        reset();
        new (d_connectClient.buffer()) ConnectClient(value);
        d_selectionId = SELECTION_ID_CONNECT_CLIENT;
    }

    return d_connectClient.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConnectClient& Action::makeConnectClient(ConnectClient&& value)
{
    if (SELECTION_ID_CONNECT_CLIENT == d_selectionId) {
        d_connectClient.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_connectClient.buffer()) ConnectClient(bsl::move(value));
        d_selectionId = SELECTION_ID_CONNECT_CLIENT;
    }

    return d_connectClient.object();
}
#endif

ConnectProxy& Action::makeConnectProxy()
{
    if (SELECTION_ID_CONNECT_PROXY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_connectProxy.object());
    }
    else {
        reset();
        new (d_connectProxy.buffer()) ConnectProxy();
        d_selectionId = SELECTION_ID_CONNECT_PROXY;
    }

    return d_connectProxy.object();
}

ConnectProxy& Action::makeConnectProxy(const ConnectProxy& value)
{
    if (SELECTION_ID_CONNECT_PROXY == d_selectionId) {
        d_connectProxy.object() = value;
    }
    else {
        reset();
        new (d_connectProxy.buffer()) ConnectProxy(value);
        d_selectionId = SELECTION_ID_CONNECT_PROXY;
    }

    return d_connectProxy.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConnectProxy& Action::makeConnectProxy(ConnectProxy&& value)
{
    if (SELECTION_ID_CONNECT_PROXY == d_selectionId) {
        d_connectProxy.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_connectProxy.buffer()) ConnectProxy(bsl::move(value));
        d_selectionId = SELECTION_ID_CONNECT_PROXY;
    }

    return d_connectProxy.object();
}
#endif

ConnectAdmin& Action::makeConnectAdmin()
{
    if (SELECTION_ID_CONNECT_ADMIN == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_connectAdmin.object());
    }
    else {
        reset();
        new (d_connectAdmin.buffer()) ConnectAdmin();
        d_selectionId = SELECTION_ID_CONNECT_ADMIN;
    }

    return d_connectAdmin.object();
}

ConnectAdmin& Action::makeConnectAdmin(const ConnectAdmin& value)
{
    if (SELECTION_ID_CONNECT_ADMIN == d_selectionId) {
        d_connectAdmin.object() = value;
    }
    else {
        reset();
        new (d_connectAdmin.buffer()) ConnectAdmin(value);
        d_selectionId = SELECTION_ID_CONNECT_ADMIN;
    }

    return d_connectAdmin.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConnectAdmin& Action::makeConnectAdmin(ConnectAdmin&& value)
{
    if (SELECTION_ID_CONNECT_ADMIN == d_selectionId) {
        d_connectAdmin.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_connectAdmin.buffer()) ConnectAdmin(bsl::move(value));
        d_selectionId = SELECTION_ID_CONNECT_ADMIN;
    }

    return d_connectAdmin.object();
}
#endif

ConnectClusterNode& Action::makeConnectClusterNode()
{
    if (SELECTION_ID_CONNECT_CLUSTER_NODE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_connectClusterNode.object());
    }
    else {
        reset();
        new (d_connectClusterNode.buffer()) ConnectClusterNode(d_allocator_p);
        d_selectionId = SELECTION_ID_CONNECT_CLUSTER_NODE;
    }

    return d_connectClusterNode.object();
}

ConnectClusterNode&
Action::makeConnectClusterNode(const ConnectClusterNode& value)
{
    if (SELECTION_ID_CONNECT_CLUSTER_NODE == d_selectionId) {
        d_connectClusterNode.object() = value;
    }
    else {
        reset();
        new (d_connectClusterNode.buffer())
            ConnectClusterNode(value, d_allocator_p);
        d_selectionId = SELECTION_ID_CONNECT_CLUSTER_NODE;
    }

    return d_connectClusterNode.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ConnectClusterNode& Action::makeConnectClusterNode(ConnectClusterNode&& value)
{
    if (SELECTION_ID_CONNECT_CLUSTER_NODE == d_selectionId) {
        d_connectClusterNode.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_connectClusterNode.buffer())
            ConnectClusterNode(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_CONNECT_CLUSTER_NODE;
    }

    return d_connectClusterNode.object();
}
#endif

QueueRead& Action::makeQueueRead()
{
    if (SELECTION_ID_QUEUE_READ == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueRead.object());
    }
    else {
        reset();
        new (d_queueRead.buffer()) QueueRead(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_READ;
    }

    return d_queueRead.object();
}

QueueRead& Action::makeQueueRead(const QueueRead& value)
{
    if (SELECTION_ID_QUEUE_READ == d_selectionId) {
        d_queueRead.object() = value;
    }
    else {
        reset();
        new (d_queueRead.buffer()) QueueRead(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_READ;
    }

    return d_queueRead.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueRead& Action::makeQueueRead(QueueRead&& value)
{
    if (SELECTION_ID_QUEUE_READ == d_selectionId) {
        d_queueRead.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueRead.buffer()) QueueRead(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_READ;
    }

    return d_queueRead.object();
}
#endif

QueueWrite& Action::makeQueueWrite()
{
    if (SELECTION_ID_QUEUE_WRITE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_queueWrite.object());
    }
    else {
        reset();
        new (d_queueWrite.buffer()) QueueWrite(d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_WRITE;
    }

    return d_queueWrite.object();
}

QueueWrite& Action::makeQueueWrite(const QueueWrite& value)
{
    if (SELECTION_ID_QUEUE_WRITE == d_selectionId) {
        d_queueWrite.object() = value;
    }
    else {
        reset();
        new (d_queueWrite.buffer()) QueueWrite(value, d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_WRITE;
    }

    return d_queueWrite.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueWrite& Action::makeQueueWrite(QueueWrite&& value)
{
    if (SELECTION_ID_QUEUE_WRITE == d_selectionId) {
        d_queueWrite.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_queueWrite.buffer())
            QueueWrite(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_QUEUE_WRITE;
    }

    return d_queueWrite.object();
}
#endif

ExecuteAdminCommand& Action::makeExecuteAdminCommand()
{
    if (SELECTION_ID_EXECUTE_ADMIN_COMMAND == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_executeAdminCommand.object());
    }
    else {
        reset();
        new (d_executeAdminCommand.buffer())
            ExecuteAdminCommand(d_allocator_p);
        d_selectionId = SELECTION_ID_EXECUTE_ADMIN_COMMAND;
    }

    return d_executeAdminCommand.object();
}

ExecuteAdminCommand&
Action::makeExecuteAdminCommand(const ExecuteAdminCommand& value)
{
    if (SELECTION_ID_EXECUTE_ADMIN_COMMAND == d_selectionId) {
        d_executeAdminCommand.object() = value;
    }
    else {
        reset();
        new (d_executeAdminCommand.buffer())
            ExecuteAdminCommand(value, d_allocator_p);
        d_selectionId = SELECTION_ID_EXECUTE_ADMIN_COMMAND;
    }

    return d_executeAdminCommand.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ExecuteAdminCommand&
Action::makeExecuteAdminCommand(ExecuteAdminCommand&& value)
{
    if (SELECTION_ID_EXECUTE_ADMIN_COMMAND == d_selectionId) {
        d_executeAdminCommand.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_executeAdminCommand.buffer())
            ExecuteAdminCommand(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_EXECUTE_ADMIN_COMMAND;
    }

    return d_executeAdminCommand.object();
}
#endif

// ACCESSORS

bsl::ostream&
Action::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_CONNECT_CLIENT: {
        printer.printAttribute("connectClient", d_connectClient.object());
    } break;
    case SELECTION_ID_CONNECT_PROXY: {
        printer.printAttribute("connectProxy", d_connectProxy.object());
    } break;
    case SELECTION_ID_CONNECT_ADMIN: {
        printer.printAttribute("connectAdmin", d_connectAdmin.object());
    } break;
    case SELECTION_ID_CONNECT_CLUSTER_NODE: {
        printer.printAttribute("connectClusterNode",
                               d_connectClusterNode.object());
    } break;
    case SELECTION_ID_QUEUE_READ: {
        printer.printAttribute("queueRead", d_queueRead.object());
    } break;
    case SELECTION_ID_QUEUE_WRITE: {
        printer.printAttribute("queueWrite", d_queueWrite.object());
    } break;
    case SELECTION_ID_EXECUTE_ADMIN_COMMAND: {
        printer.printAttribute("executeAdminCommand",
                               d_executeAdminCommand.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* Action::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_CONNECT_CLIENT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_CLIENT].name();
    case SELECTION_ID_CONNECT_PROXY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_PROXY].name();
    case SELECTION_ID_CONNECT_ADMIN:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_ADMIN].name();
    case SELECTION_ID_CONNECT_CLUSTER_NODE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_CLUSTER_NODE]
            .name();
    case SELECTION_ID_QUEUE_READ:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_READ].name();
    case SELECTION_ID_QUEUE_WRITE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_WRITE].name();
    case SELECTION_ID_EXECUTE_ADMIN_COMMAND:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_EXECUTE_ADMIN_COMMAND]
            .name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}
}  // close package namespace
}  // close enterprise namespace

// GENERATED BY BLP_BAS_CODEGEN_2026.05.28
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbact --msgComponent actions mqbact.xsd
