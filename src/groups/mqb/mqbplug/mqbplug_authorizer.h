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

#ifndef INCLUDED_MQBPLUG_AUTHORIZER
#define INCLUDED_MQBPLUG_AUTHORIZER

/// @file mqbplug_authorizer.h
///
/// @brief Provide base classes for the 'Authorizer' plugin.

// MQB
#include <mqbplug_authenticator.h>
#include <mqbplug_pluginfactory.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_variant.h>
#include <bslma_bslallocator.h>
#include <bslmf_iscopyconstructible.h>
#include <bslmf_movableref.h>
#include <bsls_compilerfeatures.h>
#include <bslstl_inplace.h>

namespace BloombergLP {

// FORWARD DECLARATION

namespace mqbcfg {
class AuthorizerPluginConfig;
}

namespace mqbplug {

class Action_ConnectClient {};

// INLINE DEFINITIONS

inline bool operator==(const Action_ConnectClient& lhs,
                       const Action_ConnectClient& rhs)
{
    return true;
}

inline bool operator!=(const Action_ConnectClient& lhs,
                       const Action_ConnectClient& rhs)
{
    return !(lhs == rhs);
}

class Action_ConnectProxy {};

// INLINE DEFINITIONS

inline bool operator==(const Action_ConnectProxy& lhs,
                       const Action_ConnectProxy& rhs)
{
    return true;
}

inline bool operator!=(const Action_ConnectProxy& lhs,
                       const Action_ConnectProxy& rhs)
{
    return !(lhs == rhs);
}

class Action_ConnectAdmin {};

// INLINE DEFINITIONS

inline bool operator==(const Action_ConnectAdmin& lhs,
                       const Action_ConnectAdmin& rhs)
{
    return true;
}

inline bool operator!=(const Action_ConnectAdmin& lhs,
                       const Action_ConnectAdmin& rhs)
{
    return !(lhs == rhs);
}

class Action_ConnectClusterNode {
  private:
    // PRIVATE TYPES
    typedef bslmf::MovableRefUtil MoveUtil;

    // DATA
    bsl::string d_clusterName;

  public:
    // TYPES
    typedef bsl::allocator<> allocator_type;

    // CREATORS

    /// Construct a value representing an action to connect to a cluster node.
    ///
    /// @param clusterName The name of the cluster
    /// @param allocator User supplied allocator
    explicit Action_ConnectClusterNode(
        const bsl::string&    clusterName,
        const allocator_type& allocator = allocator_type());

    /// Create a connect cluster action having the same value as `other`.
    ///
    /// @param allocator User supplied allocator
    Action_ConnectClusterNode(
        const Action_ConnectClusterNode& other,
        const allocator_type&            allocator = allocator_type());

    /// Create a connect cluster action having the same value as `other` by
    /// moving the contents of `other` to the newly created object.
    Action_ConnectClusterNode(bslmf::MovableRef<Action_ConnectClusterNode>
                                  other) BSLS_KEYWORD_NOEXCEPT;

    /// Create a connect cluster action having the same value as `other` by
    /// moving the contents of `other` to the newly created object.
    ///
    /// @param allocator User supplied allocator
    Action_ConnectClusterNode(
        bslmf::MovableRef<Action_ConnectClusterNode> other,
        const allocator_type&                        allocator);

    /// Destruct this object.
    ~Action_ConnectClusterNode();

    /// Assign to this object the value of `other`.
    Action_ConnectClusterNode&
    operator=(const Action_ConnectClusterNode& other);

    /// Assign to this object the value of `other` by moving the contents of
    /// `other` into this object.
    Action_ConnectClusterNode&
    operator=(bslmf::MovableRef<Action_ConnectClusterNode> other);

    // ACCESSORS

    /// Get the allocator for this object.
    allocator_type get_allocator() const;

    /// Get the cluster name targeted by this action.
    const bsl::string& clusterName() const;

    // FRIENDS

    friend bool operator==(const Action_ConnectClusterNode& lhs,
                           const Action_ConnectClusterNode& rhs);
};

// INLINE DEFINITIONS

inline bool operator==(const Action_ConnectClusterNode& lhs,
                       const Action_ConnectClusterNode& rhs)
{
    return lhs.d_clusterName == rhs.d_clusterName;
}

inline bool operator!=(const Action_ConnectClusterNode& lhs,
                       const Action_ConnectClusterNode& rhs)
{
    return !(lhs == rhs);
}

class Action_QueueRead {
  private:
    // PRIVATE TYPES
    typedef bslmf::MovableRefUtil MoveUtil;

    // DATA
    bsl::string d_uri;

  public:
    // TYPES
    typedef bsl::allocator<> allocator_type;

    // CREATORS

    /// Construct a value representing an action that reads from a queue.
    ///
    /// @param uri The name of the queue
    /// @param allocator User supplied allocator
    explicit Action_QueueRead(
        const bsl::string&    uri,
        const allocator_type& allocator = allocator_type());

    /// Create a read queue action having the same value as `other`.
    ///
    /// @param allocator User supplied allocator
    Action_QueueRead(const Action_QueueRead& other,
                     const allocator_type&   allocator = allocator_type());

    /// Create a queue read action having the same value as `other` by
    /// moving the contents of `other` to the newly created object.
    Action_QueueRead(bslmf::MovableRef<Action_QueueRead> other)
        BSLS_KEYWORD_NOEXCEPT;

    /// Create a queue read action having the same value as `other` by
    /// moving the contents of `other` to the newly created object.
    ///
    /// @param allocator User supplied allocator
    Action_QueueRead(bslmf::MovableRef<Action_QueueRead> other,
                     const allocator_type&               allocator);

    /// Destruct this object.
    ~Action_QueueRead();

    /// Assign to this object the value of `other`.
    Action_QueueRead& operator=(const Action_QueueRead& other);

    /// Assign to this object the value of `other` by moving the contents of
    /// `other` into this object.
    Action_QueueRead& operator=(bslmf::MovableRef<Action_QueueRead> other);

    // ACCESSORS

    /// Get the allocator for this object.
    allocator_type get_allocator() const;

    /// Get the URI for the queue targeted by this action.
    const bsl::string& uri() const;

    friend bool operator==(const Action_QueueRead& lhs,
                           const Action_QueueRead& rhs);
};

// INLINE DEFINITIONS

inline bool operator==(const Action_QueueRead& lhs,
                       const Action_QueueRead& rhs)
{
    return lhs.d_uri == rhs.d_uri;
}

inline bool operator!=(const Action_QueueRead& lhs,
                       const Action_QueueRead& rhs)
{
    return !(lhs == rhs);
}

class Action_QueueWrite {
  private:
    // PRIVATE TYPES
    typedef bslmf::MovableRefUtil MoveUtil;

    // DATA
    bsl::string d_uri;

  public:
    // TYPES
    typedef bsl::allocator<> allocator_type;

    // CREATORS

    /// Construct a value representing an action that write to a queue.
    ///
    /// @param uri The name of the queue
    /// @param allocator User supplied allocator
    explicit Action_QueueWrite(
        const bsl::string&    uri,
        const allocator_type& allocator = allocator_type());

    /// Create a write queue action having the same value as `other`.
    ///
    /// @param allocator User supplied allocator
    Action_QueueWrite(const Action_QueueWrite& other,
                      const allocator_type&    allocator = allocator_type());

    /// Create a queue write action having the same value as `other` by
    /// moving the contents of `other` to the newly created object.
    Action_QueueWrite(bslmf::MovableRef<Action_QueueWrite> other)
        BSLS_KEYWORD_NOEXCEPT;

    /// Create a queue write action having the same value as `other` by
    /// moving the contents of `other` to the newly created object.
    ///
    /// @param allocator User supplied allocator
    Action_QueueWrite(bslmf::MovableRef<Action_QueueWrite> other,
                      const allocator_type&                allocator);

    /// Destruct this object.
    ~Action_QueueWrite();

    /// Assign to this object the value of `other`.
    Action_QueueWrite& operator=(const Action_QueueWrite& other);

    /// Assign to this object the value of `other` by moving the contents of
    /// `other` into this object.
    Action_QueueWrite& operator=(bslmf::MovableRef<Action_QueueWrite> other);

    // ACCESSORS

    /// Get the allocator for this object.
    allocator_type get_allocator() const;

    /// Get the URI for the queue targeted by this action.
    const bsl::string& uri() const;

    friend bool operator==(const Action_QueueWrite& lhs,
                           const Action_QueueWrite& rhs);
};

// INLINE DEFINITIONS

inline bool operator==(const Action_QueueWrite& lhs,
                       const Action_QueueWrite& rhs)
{
    return lhs.d_uri == rhs.d_uri;
}

inline bool operator!=(const Action_QueueWrite& lhs,
                       const Action_QueueWrite& rhs)
{
    return !(lhs == rhs);
}

class Action_ExecuteAdminCommand {
  private:
    // PRIVATE TYPES
    typedef bslmf::MovableRefUtil MoveUtil;

    // DATA
    bsl::string d_command;

  public:
    // TYPES
    typedef bsl::allocator<> allocator_type;

    // CREATORS

    /// Construct a value representing an action that executes and admin
    /// command.
    ///
    /// @param command The admin command
    /// @param allocator User supplied allocator
    explicit Action_ExecuteAdminCommand(
        const bsl::string&    command,
        const allocator_type& allocator = allocator_type());

    /// Create an execute admin command action having the same value as
    /// `other`.
    ///
    /// @param allocator User supplied allocator
    Action_ExecuteAdminCommand(
        const Action_ExecuteAdminCommand& other,
        const allocator_type&             allocator = allocator_type());

    /// Create an execute admin command action having the same value as `other`
    /// by moving the contents of `other` to the newly created object.
    Action_ExecuteAdminCommand(bslmf::MovableRef<Action_ExecuteAdminCommand>
                                   other) BSLS_KEYWORD_NOEXCEPT;

    /// Create an execute admin action having the same value as `other` by
    /// moving the contents of `other` to the newly created object.
    ///
    /// @param allocator User supplied allocator
    Action_ExecuteAdminCommand(
        bslmf::MovableRef<Action_ExecuteAdminCommand> other,
        const allocator_type&                         allocator);

    /// Destruct this object.
    ~Action_ExecuteAdminCommand();

    /// Assign to this object the value of `other`.
    Action_ExecuteAdminCommand&
    operator=(const Action_ExecuteAdminCommand& other);

    /// Assign to this object the value of `other` by moving the contents of
    /// `other` into this object.
    Action_ExecuteAdminCommand&
    operator=(bslmf::MovableRef<Action_ExecuteAdminCommand> other);

    // ACCESSORS

    /// Get the allocator for this object.
    allocator_type get_allocator() const;

    /// Get the admin command targeted by this action.
    const bsl::string& command() const;

    friend bool operator==(const Action_ExecuteAdminCommand& lhs,
                           const Action_ExecuteAdminCommand& rhs);
};

// INLINE DEFINITIONS

inline bool operator==(const Action_ExecuteAdminCommand& lhs,
                       const Action_ExecuteAdminCommand& rhs)
{
    return lhs.d_command == rhs.d_command;
}

inline bool operator!=(const Action_ExecuteAdminCommand& lhs,
                       const Action_ExecuteAdminCommand& rhs)
{
    return !(lhs == rhs);
}

// ===================
// class Action
// ===================

class Action {
  private:
    // PRIVATE TYPES
    typedef bslmf::MovableRefUtil MoveUtil;

  public:
    // TYPES
    typedef bsl::allocator<> allocator_type;

    typedef Action_ConnectClient       ConnectClient;
    typedef Action_ConnectProxy        ConnectProxy;
    typedef Action_ConnectAdmin        ConnectAdmin;
    typedef Action_ConnectClusterNode  ConnectClusterNode;
    typedef Action_QueueRead           QueueRead;
    typedef Action_QueueWrite          QueueWrite;
    typedef Action_ExecuteAdminCommand ExecuteAdminCommand;

    typedef bsl::variant<ConnectClient,
                         ConnectProxy,
                         ConnectAdmin,
                         ConnectClusterNode,
                         QueueRead,
                         QueueWrite,
                         ExecuteAdminCommand>
        Type;

  private:
    // DATA
    Type d_action;

  public:
    // CREATORS

    /// Create an action having the same value as `other`.
    ///
    /// @param allocator User supplied allocator
    Action(const Action&         other,
           const allocator_type& allocator = allocator_type());

    /// Create an action having the same value as `other` by
    /// moving the contents of `other` to the newly created object.
    Action(bslmf::MovableRef<Action> other) BSLS_KEYWORD_NOEXCEPT;

    /// Create an action having the same value as `other` by
    /// moving the contents of `other` to the newly created object.
    ///
    /// @param allocator User supplied allocator
    Action(bslmf::MovableRef<Action> other, const allocator_type& allocator);

    /// Construct a value representing an action
    ///
    /// @param action The choice of action
    /// @param allocator User supplied allocator
    template <typename T>
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    explicit Action(BSLS_COMPILERFEATURES_FORWARD_REF(T) action,
                    const allocator_type& allocator = allocator_type());

    /// Destruct this object.
    ~Action();

    /// Assign to this object the value of `other`.
    Action& operator=(const Action& other);

    /// Assign to this object the value of `other` by moving the contents of
    /// `other` into this object.
    Action& operator=(bslmf::MovableRef<Action> other);

    // ACCESSORS

    /// Get the allocator for this object.
    allocator_type get_allocator() const;

    friend bool operator==(const Action& lhs, const Action& rhs);
};

// INLINE DEFINITIONS

template <typename T>
// NOLINTBEGIN(bugprone-forwarding-reference-overload)
// NOLINTBEGIN(cppcoreguidelines-missing-std-forward)
inline Action::Action(BSLS_COMPILERFEATURES_FORWARD_REF(T) action,
                      const allocator_type& allocator)
// NOLINTEND(bugprone-forwarding-reference-overload)
// NOLINTEND(cppcoreguidelines-missing-std-forward)
: d_action(bsl::allocator_arg,
           allocator,
           BSLS_COMPILERFEATURES_FORWARD(T, action))
{
}

inline bool operator==(const Action& lhs, const Action& rhs)
{
    return lhs.d_action == rhs.d_action;
}

inline bool operator!=(const Action& lhs, const Action& rhs)
{
    return !(lhs == rhs);
}

// ===================
// class Authorizer
// ===================

/// Interface for an Authorizer.
class Authorizer {
  public:
    // CREATORS

    /// Destroy this object.
    virtual ~Authorizer();

    // ACESSORS

    /// Return the name of the plugin.
    ///
    /// This method is valid to call at any time after construction, including
    /// before `start()` is called.
    virtual bsl::string_view name() const = 0;

    // TODO(tfoxhall) authorize API
    virtual bool authorize(const Action&               action,
                           const AuthenticationResult& authnResult) = 0;

    // MANIPULATORS
};

// ================================
// class AuthorizerPluginFactory
// ================================

/// This is the base class for the factory of plugins of type `Authorizer`.
/// All it does is allows to instantiate a concrete object of the
/// `Authorizer` interface, taking any required (plugin specific) arguments.
class AuthorizerPluginFactory : public PluginFactory {
  public:
    // CREATORS
    AuthorizerPluginFactory();

    ~AuthorizerPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    virtual bslma::ManagedPtr<Authorizer>
    create(bslma::Allocator* allocator) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
