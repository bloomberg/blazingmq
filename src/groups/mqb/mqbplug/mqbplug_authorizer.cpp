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

#include <mqbplug_authorizer.h>

#include <mqbscm_version.h>

// MQB
#include <mqbcfg_messages.h>

// BDE
#include <bsl_string_view.h>
#include <bsl_vector.h>

namespace BloombergLP {
namespace mqbplug {

// -------------------------------
// class Action_ConnectClusterNode
// -------------------------------

Action_ConnectClusterNode::Action_ConnectClusterNode(
    const bsl::string&    clusterName,
    const allocator_type& allocator)
: d_clusterName(clusterName, allocator)
{
    // NOTHING
}

Action_ConnectClusterNode::Action_ConnectClusterNode(
    const Action_ConnectClusterNode& other,
    const allocator_type&            allocator)
: d_clusterName(other.d_clusterName, allocator)
{
    // NOTHING
}

Action_ConnectClusterNode::Action_ConnectClusterNode(
    bslmf::MovableRef<Action_ConnectClusterNode> other) BSLS_KEYWORD_NOEXCEPT
: d_clusterName(MoveUtil::move(MoveUtil::access(other).d_clusterName))
{
    // NOTHING
}

Action_ConnectClusterNode::Action_ConnectClusterNode(
    bslmf::MovableRef<Action_ConnectClusterNode> other,
    const allocator_type&                        allocator)
: d_clusterName(MoveUtil::move(MoveUtil::access(other).d_clusterName),
                allocator)
{
    // NOTHING
}

Action_ConnectClusterNode::~Action_ConnectClusterNode()
{
    // NOTHING
}

Action_ConnectClusterNode&
Action_ConnectClusterNode::operator=(const Action_ConnectClusterNode& other)
{
    if (this == &other) {
        return *this;
    }

    d_clusterName = other.d_clusterName;

    return *this;
}

Action_ConnectClusterNode& Action_ConnectClusterNode::operator=(
    bslmf::MovableRef<Action_ConnectClusterNode> other)
{
    Action_ConnectClusterNode& otherRef = other;
    if (get_allocator() != otherRef.get_allocator()) {
        operator=(other);
    }
    else if (this != &otherRef) {
        d_clusterName = MoveUtil::move(otherRef.d_clusterName);
    }

    return *this;
}

Action_ConnectClusterNode::allocator_type
Action_ConnectClusterNode::get_allocator() const
{
    return d_clusterName.get_allocator();
}

const bsl::string& Action_ConnectClusterNode::clusterName() const
{
    return d_clusterName;
}

// ----------------------
// class Action_QueueRead
// ----------------------

Action_QueueRead::Action_QueueRead(const bsl::string&    uri,
                                   const allocator_type& allocator)
: d_uri(uri, allocator)
{
}

Action_QueueRead::Action_QueueRead(const Action_QueueRead& other,
                                   const allocator_type&   allocator)
: d_uri(other.d_uri, allocator)
{
}

Action_QueueRead::Action_QueueRead(bslmf::MovableRef<Action_QueueRead> other)
    BSLS_KEYWORD_NOEXCEPT
: d_uri(MoveUtil::move(MoveUtil::access(other).d_uri))
{
}

Action_QueueRead::Action_QueueRead(bslmf::MovableRef<Action_QueueRead> other,
                                   const allocator_type& allocator)
: d_uri(MoveUtil::move(MoveUtil::access(other).d_uri), allocator)
{
}

Action_QueueRead::~Action_QueueRead()
{
    // NOTHING
}

Action_QueueRead& Action_QueueRead::operator=(const Action_QueueRead& other)
{
    if (this == &other) {
        return *this;
    }

    d_uri = other.d_uri;

    return *this;
}

Action_QueueRead&
Action_QueueRead::operator=(bslmf::MovableRef<Action_QueueRead> other)
{
    Action_QueueRead& otherRef = other;
    if (get_allocator() != otherRef.get_allocator()) {
        operator=(other);
    }
    else if (this != &otherRef) {
        d_uri = MoveUtil::move(otherRef.d_uri);
    }

    return *this;
}

Action_QueueRead::allocator_type Action_QueueRead::get_allocator() const
{
    return d_uri.get_allocator();
}

const bsl::string& Action_QueueRead::uri() const
{
    return d_uri;
}

// -----------------------
// class Action_QueueWrite
// -----------------------

Action_QueueWrite::Action_QueueWrite(const bsl::string&    uri,
                                     const allocator_type& allocator)
: d_uri(uri, allocator)
{
    // NOTHING
}

Action_QueueWrite::Action_QueueWrite(const Action_QueueWrite& other,
                                     const allocator_type&    allocator)
: d_uri(other.d_uri, allocator)
{
    // NOTHING
}

Action_QueueWrite::Action_QueueWrite(
    bslmf::MovableRef<Action_QueueWrite> other) BSLS_KEYWORD_NOEXCEPT
: d_uri(MoveUtil::move(MoveUtil::access(other).d_uri))
{
    // NOTHING
}

Action_QueueWrite::Action_QueueWrite(
    bslmf::MovableRef<Action_QueueWrite> other,
    const allocator_type&                allocator)
: d_uri(MoveUtil::move(MoveUtil::access(other).d_uri), allocator)
{
}

Action_QueueWrite::~Action_QueueWrite()
{
    // NOTHING
}

Action_QueueWrite& Action_QueueWrite::operator=(const Action_QueueWrite& other)
{
    if (this == &other) {
        return *this;
    }

    d_uri = other.d_uri;

    return *this;
}

Action_QueueWrite&
Action_QueueWrite::operator=(bslmf::MovableRef<Action_QueueWrite> other)
{
    Action_QueueWrite& otherRef = other;
    if (get_allocator() != otherRef.get_allocator()) {
        operator=(other);
    }
    else if (this != &otherRef) {
        d_uri = MoveUtil::move(otherRef.d_uri);
    }

    return *this;
}

Action_QueueWrite::allocator_type Action_QueueWrite::get_allocator() const
{
    return d_uri.get_allocator();
}

const bsl::string& Action_QueueWrite::uri() const
{
    return d_uri;
}

// --------------------------------
// class Action_ExecuteAdminCommand
// --------------------------------

Action_ExecuteAdminCommand::Action_ExecuteAdminCommand(
    const bsl::string&    command,
    const allocator_type& allocator)
: d_command(command, allocator)
{
    // NOTHING
}

Action_ExecuteAdminCommand::Action_ExecuteAdminCommand(
    const Action_ExecuteAdminCommand& other,
    const allocator_type&             allocator)
: d_command(other.d_command, allocator)
{
    // NOTHING
}

Action_ExecuteAdminCommand::Action_ExecuteAdminCommand(
    bslmf::MovableRef<Action_ExecuteAdminCommand> other) BSLS_KEYWORD_NOEXCEPT
: d_command(MoveUtil::move(MoveUtil::access(other).d_command))
{
    // NOTHING
}

Action_ExecuteAdminCommand::Action_ExecuteAdminCommand(
    bslmf::MovableRef<Action_ExecuteAdminCommand> other,
    const allocator_type&                         allocator)
: d_command(MoveUtil::move(MoveUtil::access(other).d_command), allocator)
{
    // NOTHING
}

Action_ExecuteAdminCommand::~Action_ExecuteAdminCommand()
{
    // NOTHING
}

Action_ExecuteAdminCommand&
Action_ExecuteAdminCommand::operator=(const Action_ExecuteAdminCommand& other)
{
    if (this == &other) {
        return *this;
    }

    d_command = other.d_command;

    return *this;
}

Action_ExecuteAdminCommand& Action_ExecuteAdminCommand::operator=(
    bslmf::MovableRef<Action_ExecuteAdminCommand> other)
{
    Action_ExecuteAdminCommand& otherRef = other;
    if (get_allocator() != otherRef.get_allocator()) {
        operator=(other);
    }
    else if (this != &otherRef) {
        d_command = MoveUtil::move(otherRef.d_command);
    }

    return *this;
}

Action_ExecuteAdminCommand::allocator_type
Action_ExecuteAdminCommand::get_allocator() const
{
    return d_command.get_allocator();
}

const bsl::string& Action_ExecuteAdminCommand::command() const
{
    return d_command;
}

// ------------
// class Aciton
// ------------

Action::Action(const Action& other, const allocator_type& allocator)
: d_action(bsl::allocator_arg, allocator, other.d_action)
{
    // NOTHING
}

Action::Action(bslmf::MovableRef<Action> other) BSLS_KEYWORD_NOEXCEPT
: d_action(MoveUtil::move(MoveUtil::access(other).d_action))
{
    // NOTHING
}

Action::Action(bslmf::MovableRef<Action> other,
               const allocator_type&     allocator)
: d_action(bsl::allocator_arg,
           allocator,
           MoveUtil::move(MoveUtil::access(other).d_action))
{
    // NOTHING
}

Action::~Action()
{
    // NOTHING
}

Action& Action::operator=(const Action& other)
{
    if (this == &other) {
        return *this;
    }
    d_action = other.d_action;

    return *this;
}

Action& Action::operator=(bslmf::MovableRef<Action> other)
{
    Action& otherRef = other;
    if (get_allocator() != otherRef.get_allocator()) {
        operator=(other);
    }
    else if (this != &otherRef) {
        d_action = MoveUtil::move(otherRef.d_action);
    }

    return *this;
}

Action::allocator_type Action::get_allocator() const
{
    return d_action.get_allocator();
}

// -------------------
// class Authorizer
// -------------------

Authorizer::~Authorizer()
{
    // NOTHING
}

// --------------------------------
// class AuthorizerPluginFactory
// --------------------------------

AuthorizerPluginFactory::AuthorizerPluginFactory()
{
    // NOTHING
}

AuthorizerPluginFactory::~AuthorizerPluginFactory()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
