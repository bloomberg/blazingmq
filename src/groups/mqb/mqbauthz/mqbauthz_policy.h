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

#ifndef INCLUDED_MQBAUTHZ_POLICY
#define INCLUDED_MQBAUTHZ_POLICY

/// @file mqbauthz_policy.h
///
/// @brief Provide the built-in default authorizer plugin.
///
/// @bbref{mqbauthz::Policy} provides a collection for associating policies
/// with roles. See also: @bbref{mqbauthz::DefaultAuthorizer} See also:
/// @bbref{mqbpoly::Policy}
///
/// Purpose
/// =======
/// This component is used to define a simple policy evaluation language for
/// the DefaultAuthorizer plugin.
///
/// Usage
/// =====
/// Given a policy definition like so:
///
/// ```json
/// {
///     "roles": [
///         {
///             "id": "anonymous",
///             "permissions": [
///                 {
///                     "action": "connectClient",
///                 },
///                 {
///                     "action": "queueRead",
///                     "resources": [
///                         {
///                             "id": "*",
///                         }
///                     ]
///                 },
///                 {
///                     "action": "queueWrite",
///                     "resources": [
///                         {
///                             "id": "*",
///                         }
///                     ]
///                 }
///             ]
///         }
///     ]
/// }
/// ```
///
/// We can validate and load this policy definition
///
/// ```c++
/// mqbpoly::Policy policyDefinition;
/// ASSERT(0 == fromJson(&policyDefinition, json));
///
/// Policy policy;  // create an empty policy set
/// ASSERT(0 == Policy::load(&policy, policyDefinition));
///
/// // Now check user "anonymous"'s permissions
/// const Policy::Permission& anonPermission = policy.get("anonymous");
/// ASSERT(anonPermission.isConnectClientAllowed());
/// ASSERT(!anonPermission.isAdminAllowed());
/// ASSERT(anonPermission.isQueueReadAllowed());
/// ASSERT(anonPermission.isQueueWriteAllowed());
/// ````
///
/// Policy Language
/// ===============
/// The policy language structure is mostly defined by the `mqbpoly::Policy`
/// type. A *policy* consists of a list of *roles*, which are a mapping of a
/// *role identifier* to a list of *permissions*.
///
/// Roles
/// -----
/// Roles are a simple way to describe what actions and resources a user has
/// access to. The role identifier as specified by the `id` property is the
/// principal used during authentication. The role identifier must exactly
/// match the authenticated principal for the `permissions` property to apply
/// to a connection.
///
/// Permissions
/// -----------
/// The permission object is a pair of `(action, resources)`, where `action`
/// represents an authorized action and `resources` represent an optional list
/// of objects that the action operates on. The default authorizer interprets
/// the existance of a `permission` object as allowing access to the specified
/// `action` on the specified `resources` for the role specified by `id`.
///
/// There are six actions that can be specified. They must be unique per
/// permission list.
///
/// - `connectClient`
/// - `connectAdmin`
/// - `connectClusterNode`
/// - `queueRead`
/// - `queueWrite`
/// - `executeAdminCommand`
///
/// ### connectClient
/// Specifies that this role can connect as a client. This action has no
/// resources
///
/// ### connectAdmin
/// Specifies that this role can connect as an admin. This action has no
/// resources
///
/// ### connectClusterNode
/// Specifies that this role can connect as a member of a cluster. This action
/// has no resources
///
/// ### queueRead
/// Specifies that this role can open queues for reading/consuming. This action
/// requires specifying at least one resource, which denotes which domains and
/// queues it may open.
///
/// ### queueWrite
/// Specifies that this role can open queues for writing/producing. This action
/// requires specifying at least one resource, which denotes which domains and
/// queues it may open.
///
/// ### Queue Resource Syntax
/// The syntax for specifing queue resources is:
/// - "*" => Allow all
/// - "valid-domain-name" => Allow opening any queue in the domain
/// "valid-domain-name"
/// - "valid-domain-name/valid-queue-name" => Allow opening the queue
/// "valid-queue-name" in the domain "valid-domain-name"

// MQB
#include <mqbpoly_policies.h>

// BMQ

// BDE
#include <ball_log.h>
#include <bsl_optional.h>
#include <bsl_string_view.h>
#include <bsl_unordered_map.h>
#include <bsl_variant.h>
#include <bsl_vector.h>
#include <bsla_nodiscard.h>
#include <bslmf_movableref.h>

namespace BloombergLP {

namespace bmqt {
class Uri;
}

namespace mqbauthz {

// =======================
// class Policy
// =======================

struct Policy_UriResourceAll {};

struct Policy_UriResourceDomain {
    bsl::string_view d_domainName;

    Policy_UriResourceDomain(bsl::string_view domainName);
};

struct Policy_UriResourceQueue {
    bsl::string_view d_domainName;
    bsl::string_view d_queueName;

    Policy_UriResourceQueue(bsl::string_view domainName,
                            bsl::string_view queueName);
};

class Policy_UriResource {
  private:
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHZ.POLICY_URIRESOURCE");

    static const char* k_PATTERN;

    bsl::variant<bsl::monostate,
                 Policy_UriResourceAll,
                 Policy_UriResourceDomain,
                 Policy_UriResourceQueue>
        d_resource;

  public:
    Policy_UriResource();

    BSLA_NODISCARD
    static int parse(Policy_UriResource* result,
                     bsl::string_view    resourcePattern);

    // ACCESSORS

    bool matches(const bmqt::Uri& uri) const;
};

class Policy_Permission {
  private:
    typedef bsl::vector<mqbpoly::Resource> ResourceList;

    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHZ.POLICY_URIRESOURCE");

    mqbpoly::Identity d_id;
    bool              d_connectClient;
    bool              d_connectProxy;
    bool              d_connectAdmin;
    bool              d_connectClusterNode;
    ResourceList      d_queueRead;
    ResourceList      d_queueWrite;
    bool              d_executeAdminCommand;

  public:
    typedef bsl::allocator<> allocator_type;

    // CREATORS

    /// Construct a permission allowing no access for identity.
    Policy_Permission();

    explicit Policy_Permission(const allocator_type& allocator);

    Policy_Permission(const Policy_Permission& other,
                      const allocator_type&    allocator = allocator_type());

    Policy_Permission& operator=(const Policy_Permission& other);

    Policy_Permission(bslmf::MovableRef<Policy_Permission> other)
        BSLS_KEYWORD_NOEXCEPT;

    Policy_Permission(bslmf::MovableRef<Policy_Permission> other,
                      const allocator_type&                allocator);

    Policy_Permission& operator=(bslmf::MovableRef<Policy_Permission> other);

    allocator_type get_allocator() const;

  private:
    int updateAction(const bsl::string& action, const ResourceList& resources);

    static bool matchUri(bsl::string_view    uriStr,
                         const ResourceList& resources);

  public:
    BSLA_NODISCARD
    static int parse(Policy_Permission*     result,
                     const mqbpoly::Role&   role,
                     const bsl::allocator<> allocator);

    // ACCESSORS
    bool isConnectClientAllowed() const;

    bool isConnctProxyAllowed() const;

    bool isConnctAdminAllowed() const;

    bool isConnctClusterNodeAllowed() const;

    bool isQueueReadAllowed(bsl::string_view uri) const;

    bool isQueueWriteAllowed(bsl::string_view uri) const;

    bool isExecuteAdminCommandAllowed(bsl::string_view command) const;
};

class Policy {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHZ.POLICY");

  public:
    // TYPES
    typedef Policy_Permission Permission;

  private:
    // PRIVATE TYPES
    typedef bsl::unordered_map<mqbpoly::Identity, Permission> RoleMap;

    // DATA
    RoleMap d_rolePermissions;

  public:
    // CREATORS

    // TODO(tfoxhall): Figure out what to pass here
    Policy();

    static int parse(Policy*                 result,
                     const mqbpoly::Policy&  policy,
                     const bsl::allocator<>& allocator);

    // ACCESSORS

    /// Get a handle to the permissions for the role specified by `role`, or
    /// none if none were defined.
    bsl::optional<const Permission*> get(bsl::string_view role);
};

}  // namespace mqbauthz
}  // namespace BloombergLP

#endif  // INCLUDED_MQBAUTHZ_POLICY
