#include <mqbauthz_policy.h>

// MQB
#include <mqbcmd_messages.h>
#include <mqbcmd_parseutil.h>
#include <mqbpoly_policies.h>

// BMQ
#include <bmqt_uri.h>

// BDE
#include <bdlb_pairutil.h>
#include <bdlpcre_regex.h>
#include <bsl_algorithm.h>
#include <bsl_optional.h>
#include <bsl_string_view.h>
#include <bsl_unordered_map.h>
#include <bsl_variant.h>
#include <bsl_vector.h>
#include <bsla_maybeunused.h>
#include <bsla_nodiscard.h>
#include <bslma_constructionutil.h>
#include <bslmf_movableref.h>

namespace BloombergLP {
namespace mqbauthz {

namespace {

typedef bsl::unordered_map<BloombergLP::mqbpoly::Identity,
                           BloombergLP::mqbauthz::Policy_Permission>
    RoleMap;

struct ContainsRole {
    bsl::string_view d_role;

    bool operator()(const RoleMap::value_type& entry)
    {
        return entry.first.name() == d_role;
    }
};

}  // anonymous namespace

// ==================
// Policy_UriResource
// ==================

const char* Policy_UriResource::k_PATTERN =
    "^(?P<allResources>\\*)|"
    "^(?P<domainResource>[a-zA-Z0-9-_.]+)(/"
    "(?P<queueResource>[a-zA-Z0-9-_.]+))?";

Policy_UriResourceDomain::Policy_UriResourceDomain(bsl::string_view domainName)
: d_domainName(domainName)
{
}

Policy_UriResourceQueue::Policy_UriResourceQueue(bsl::string_view domainName,
                                                 bsl::string_view queueName)
: d_domainName(domainName)
, d_queueName(queueName)
{
}

Policy_UriResource::Policy_UriResource()
{
}

BSLA_NODISCARD
int Policy_UriResource::parse(Policy_UriResource* result,
                              bsl::string_view    resourcePattern)
{
    enum { k_OK = 0, k_NO_MATCH };

    bdlpcre::RegEx uriResourcePattern;
    uriResourcePattern.prepare(NULL, NULL, k_PATTERN);

    typedef bsl::vector<bsl::string_view> PatternMatches;
    PatternMatches                        matches;
    if (bdlpcre::RegEx::k_STATUS_SUCCESS !=
        uriResourcePattern.match(&matches, resourcePattern)) {
        return k_NO_MATCH;
    }

    *result = Policy_UriResource();

    // Empty strings in matches indicate the pattern was not found.

    if (!matches[uriResourcePattern.subpatternIndex("allResources")].empty()) {
        result->d_resource.emplace<Policy_UriResourceAll>();
    }
    else {
        bsl::string_view domainName =
            matches[uriResourcePattern.subpatternIndex("domainResource")];
        bsl::string_view queueName =
            matches[uriResourcePattern.subpatternIndex("queueResource")];

        if (queueName.empty()) {
            result->d_resource.emplace<Policy_UriResourceDomain>(domainName);
        }
        else if (!domainName.empty()) {
            result->d_resource.emplace<Policy_UriResourceQueue>(domainName,
                                                                queueName);
        }
        else {
            // This shouldn't be possible but if the regex is wrong it
            // might be possible to get here.
            return k_NO_MATCH;  // RETURN
        }
    }

    return k_OK;
}

bool Policy_UriResource::matches(const bmqt::Uri& uri) const
{
    struct Matcher {
        const bmqt::Uri* d_uri;

        Matcher(const bmqt::Uri* uri)
        : d_uri(uri)
        {
        }

        bool operator()(bsl::monostate) { return false; }
        bool operator()(Policy_UriResourceAll) { return true; }
        bool operator()(const Policy_UriResourceDomain& domain)
        {
            return d_uri->domain() == domain.d_domainName;
        }
        bool operator()(const Policy_UriResourceQueue& queue)
        {
            return d_uri->domain() == queue.d_domainName &&
                   d_uri->queue() == queue.d_queueName;
        }
    };

    Matcher matcher(&uri);
    return bsl::visit(matcher, d_resource);
}

// =================
// Policy_Permission
// =================

Policy_Permission::Policy_Permission()
: d_id()
, d_connectClient(false)
, d_connectProxy(false)
, d_connectAdmin(false)
, d_connectClusterNode(false)
, d_queueRead()
, d_queueWrite()
, d_executeAdminCommand(false)
{
}

Policy_Permission::Policy_Permission(const allocator_type& allocator)
: d_id(allocator.mechanism())
, d_connectClient(false)
, d_connectProxy(false)
, d_connectAdmin(false)
, d_connectClusterNode(false)
, d_queueRead(allocator)
, d_queueWrite(allocator)
, d_executeAdminCommand(false)
{
}

Policy_Permission::Policy_Permission(const Policy_Permission& other,
                                     const allocator_type&    allocator)
: d_id(other.d_id, allocator.mechanism())
, d_connectClient(other.d_connectClient)
, d_connectProxy(other.d_connectClient)
, d_connectAdmin(other.d_connectAdmin)
, d_connectClusterNode(other.d_connectClusterNode)
, d_queueRead(other.d_queueRead, allocator)
, d_queueWrite(other.d_queueWrite, allocator)
, d_executeAdminCommand(other.d_executeAdminCommand)
{
}

Policy_Permission& Policy_Permission::operator=(const Policy_Permission& other)
{
    if (this == &other) {
        return *this;
    }

    d_id                  = other.d_id;
    d_connectClient       = other.d_connectClient;
    d_connectProxy        = other.d_connectProxy;
    d_connectAdmin        = other.d_connectAdmin;
    d_connectClusterNode  = other.d_connectClusterNode;
    d_queueRead           = other.d_queueRead;
    d_queueWrite          = other.d_queueWrite;
    d_executeAdminCommand = other.d_executeAdminCommand;

    return *this;
}

#define MOVE_FIELD(OBJ, FIELD)                                                \
    bslmf::MovableRefUtil::move(bslmf::MovableRefUtil::access(OBJ).FIELD)
#define MOVE_INIT(OBJ, FIELD) FIELD(MOVE_FIELD(OBJ, FIELD))
#define MOVE_INIT_ALLOC(OBJ, FIELD, ALLOC) FIELD(MOVE_FIELD(OBJ, FIELD), ALLOC)

Policy_Permission::Policy_Permission(
    bslmf::MovableRef<Policy_Permission> other) BSLS_KEYWORD_NOEXCEPT
: MOVE_INIT(other, d_id),
  MOVE_INIT(other, d_connectClient),
  MOVE_INIT(other, d_connectProxy),
  MOVE_INIT(other, d_connectAdmin),
  MOVE_INIT(other, d_connectClusterNode),
  MOVE_INIT(other, d_queueRead),
  MOVE_INIT(other, d_queueWrite),
  MOVE_INIT(other, d_executeAdminCommand)
{
}

Policy_Permission::Policy_Permission(
    bslmf::MovableRef<Policy_Permission> other,
    const allocator_type&                allocator)
: MOVE_INIT_ALLOC(other, d_id, allocator.mechanism())
, MOVE_INIT(other, d_connectClient)
, MOVE_INIT(other, d_connectProxy)
, MOVE_INIT(other, d_connectAdmin)
, MOVE_INIT(other, d_connectClusterNode)
, MOVE_INIT_ALLOC(other, d_queueRead, allocator)
, MOVE_INIT_ALLOC(other, d_queueWrite, allocator)
, MOVE_INIT(other, d_executeAdminCommand)
{
}

#undef MOVE_INIT_ALLOC
#undef MOVE_INIT
#undef MOVE_FIELD

Policy_Permission&
Policy_Permission::operator=(bslmf::MovableRef<Policy_Permission> other)
{
    Policy_Permission& otherRef = other;
    if (this == &otherRef) {
        return *this;
    }

    if (get_allocator() != other.get_allocator()) {
        *this = otherRef;
    }
    else {
        typedef bslmf::MovableRefUtil Move;
        d_id                  = Move::move(otherRef.d_id);
        d_connectClient       = otherRef.d_connectClient;
        d_connectProxy        = otherRef.d_connectProxy;
        d_connectAdmin        = otherRef.d_connectAdmin;
        d_connectClusterNode  = otherRef.d_connectClusterNode;
        d_queueRead           = Move::move(otherRef.d_queueRead);
        d_queueWrite          = Move::move(otherRef.d_queueWrite);
        d_executeAdminCommand = otherRef.d_executeAdminCommand;
    }

    return *this;
}

Policy_Permission::allocator_type Policy_Permission::get_allocator() const
{
    return d_queueRead.get_allocator();
}

int Policy_Permission::updateAction(const bsl::string&  action,
                                    const ResourceList& resources)
{
    enum { k_OK = 0, k_INVALID_ACTION };

    // TODO(tfoxhall): Convert resources into a more type-specific value,
    // depending on the action
    if (action == "connectClient") {
        d_connectClient = true;
    }
    else if (action == "connectProxy") {
        d_connectProxy = true;
    }
    else if (action == "connectAdmin") {
        d_connectAdmin = true;
    }
    else if (action == "connectClusterNode") {
        d_connectClusterNode = true;
    }
    else if (action == "queueRead") {
        d_queueRead = resources;
    }
    else if (action == "queueWrite") {
        d_queueWrite = resources;
    }
    else if (action == "executeAdminCommand") {
        d_executeAdminCommand = true;
    }
    else {
        return k_INVALID_ACTION;
    }

    return k_OK;
}

bool Policy_Permission::matchUri(bsl::string_view    uriStr,
                                 const ResourceList& resources)
{
    /*
    Ok so we need to define the syntax of patterns on URI matching
    It seems like we could define 3-ish categories:

    1. Domain-level
    "bmq-domain-name"

    2. Queue-level
    "bmq-domain-name/queue-name"

    3. Allow-all
    "*"

    */
    bmqt::Uri uri;
    int       rc = bmqt::UriParser::parse(&uri, NULL, uriStr);
    if (rc != 0) {
        // Invalid URIs don't match
        return false;
    }

    for (ResourceList::const_iterator it  = resources.cbegin(),
                                      end = resources.cend();
         it != end;
         ++it) {
        Policy_UriResource uriResource;
        rc = Policy_UriResource::parse(&uriResource, it->id());

        if (rc != 0) {
            // TODO(tfoxhall): Maybe we should parse the resource as a URI
            // matcher earlier?
            return false;  // RETURN
        }

        if (uriResource.matches(uri)) {
            return true;
        }
    }

    return false;
}

BSLA_NODISCARD
int Policy_Permission::parse(Policy_Permission*     result,
                             const mqbpoly::Role&   role,
                             const bsl::allocator<> allocator)
{
    enum {
        k_OK = 0,
        k_NON_UNIQUE_ACTIONS,
    };

    *result      = Policy_Permission(allocator);
    result->d_id = role.id();

    // Validate uniqueness of actions
    bsl::set<bsl::string> actions;

    for (bsl::vector<mqbpoly::Permission>::const_iterator
             it  = role.permissions().cbegin(),
             end = role.permissions().cend();
         it != end;
         ++it) {
        // Validate action uniqueness
        bsl::set<bsl::string>::const_iterator pos        = actions.cend();
        bool                                  isInserted = false;
        bdlb::PairUtil::tie(pos, isInserted) = actions.emplace(it->action());
        if (!isInserted) {
            return k_NON_UNIQUE_ACTIONS;  // RETURN
        }

        // Parse the action
        int rc = result->updateAction(it->action(), it->resources());
        if (rc != 0) {
            return rc;  // RETURN
        }
    }

    return k_OK;
}

bool Policy_Permission::isConnectClientAllowed() const
{
    return d_connectClient;
}

bool Policy_Permission::isConnctProxyAllowed() const
{
    return d_connectProxy;
}

bool Policy_Permission::isConnctAdminAllowed() const
{
    return d_connectAdmin;
}

bool Policy_Permission::isConnctClusterNodeAllowed() const
{
    return d_connectClusterNode;
}

bool Policy_Permission::isQueueReadAllowed(bsl::string_view uri) const
{
    return matchUri(uri, d_queueRead);
}

bool Policy_Permission::isQueueWriteAllowed(bsl::string_view uri) const
{
    return matchUri(uri, d_queueWrite);
}

bool Policy_Permission::isExecuteAdminCommandAllowed(
    BSLA_MAYBE_UNUSED bsl::string_view command) const
{
    return d_executeAdminCommand;
}

// ======
// Policy
// ======

Policy::Policy()
{
}

int Policy::parse(Policy*                 result,
                  const mqbpoly::Policy&  policy,
                  const bsl::allocator<>& allocator)
{
    enum {
        k_OK = 0,
        k_NON_UNQIUE_IDENTITIES,
        k_INVALID_PERMISSION,
    };

    // TODO(tfoxhall): Validate the policies described by the policy VST
    // 1. each role is unique
    // 2. each resource within a permission has a unique action
    // 3. resource identifiers are valid
    RoleMap roles(allocator);
    for (bsl::vector<mqbpoly::Role>::const_iterator
             it  = policy.roles().cbegin(),
             end = policy.roles().cend();
         it != end;
         ++it) {
        // Validate the permission
        Permission permission;
        int        rc = Permission::parse(&permission, *it, allocator);
        if (rc != 0) {
            return k_INVALID_PERMISSION;
        }

        // Insert the validated permission into the roles map
        RoleMap::const_iterator position   = roles.cend();
        bool                    isInserted = false;
        bdlb::PairUtil::tie(position, isInserted) =
            roles.emplace(it->id(), bslmf::MovableRefUtil::move(permission));
        if (!isInserted) {
            return k_NON_UNQIUE_IDENTITIES;  // RETURN
        }
    }

    bslma::ConstructionUtil::construct(result, allocator);
    result->d_rolePermissions = bslmf::MovableRefUtil::move(roles);

    return k_OK;
}

bsl::optional<const Policy::Permission*> Policy::get(bsl::string_view role)
{
    bsl::optional<const Permission*> result;
    ContainsRole                     containsRole = {role};
    RoleMap::const_iterator permIt = bsl::find_if(d_rolePermissions.cbegin(),
                                                  d_rolePermissions.cend(),
                                                  containsRole);
    if (permIt != d_rolePermissions.cend()) {
        result = &permIt->second;
    }
    return result;
}

}  // namespace mqbauthz
}  // namespace BloombergLP
