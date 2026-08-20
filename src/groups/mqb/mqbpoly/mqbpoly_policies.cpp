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

// mqbpoly_policies.cpp           *DO NOT EDIT*            @generated -*-C++-*-

#include <mqbpoly_policies.h>

#include <bdlat_formattingmode.h>
#include <bdlat_valuetypefunctions.h>
#include <bdlb_print.h>
#include <bdlb_printmethods.h>
#include <bdlb_string.h>

#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslim_printer.h>
#include <bsls_assert.h>

#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>

namespace BloombergLP {
namespace mqbpoly {

// --------------
// class Identity
// --------------

// CONSTANTS

const char Identity::CLASS_NAME[] = "Identity";

const bdlat_AttributeInfo Identity::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Identity::lookupAttributeInfo(const char* name,
                                                         int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Identity::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Identity::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    default: return 0;
    }
}

// CREATORS

Identity::Identity(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
{
}

Identity::Identity(const Identity& original, bslma::Allocator* basicAllocator)
: d_name(original.d_name, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Identity::Identity(Identity&& original) noexcept
: d_name(bsl::move(original.d_name))
{
}

Identity::Identity(Identity&& original, bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
{
}
#endif

Identity::~Identity()
{
}

// MANIPULATORS

Identity& Identity::operator=(const Identity& rhs)
{
    if (this != &rhs) {
        d_name = rhs.d_name;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Identity& Identity::operator=(Identity&& rhs)
{
    if (this != &rhs) {
        d_name = bsl::move(rhs.d_name);
    }

    return *this;
}
#endif

void Identity::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
}

// ACCESSORS

bsl::ostream&
Identity::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.end();
    return stream;
}

// --------------
// class Resource
// --------------

// CONSTANTS

const char Resource::CLASS_NAME[] = "Resource";

const bdlat_AttributeInfo Resource::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ID,
     "id",
     sizeof("id") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Resource::lookupAttributeInfo(const char* name,
                                                         int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Resource::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Resource::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID];
    default: return 0;
    }
}

// CREATORS

Resource::Resource(bslma::Allocator* basicAllocator)
: d_id(basicAllocator)
{
}

Resource::Resource(const Resource& original, bslma::Allocator* basicAllocator)
: d_id(original.d_id, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Resource::Resource(Resource&& original) noexcept
: d_id(bsl::move(original.d_id))
{
}

Resource::Resource(Resource&& original, bslma::Allocator* basicAllocator)
: d_id(bsl::move(original.d_id), basicAllocator)
{
}
#endif

Resource::~Resource()
{
}

// MANIPULATORS

Resource& Resource::operator=(const Resource& rhs)
{
    if (this != &rhs) {
        d_id = rhs.d_id;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Resource& Resource::operator=(Resource&& rhs)
{
    if (this != &rhs) {
        d_id = bsl::move(rhs.d_id);
    }

    return *this;
}
#endif

void Resource::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_id);
}

// ACCESSORS

bsl::ostream&
Resource::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("id", this->id());
    printer.end();
    return stream;
}

// ----------------
// class Permission
// ----------------

// CONSTANTS

const char Permission::CLASS_NAME[] = "Permission";

const bdlat_AttributeInfo Permission::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ACTION,
     "action",
     sizeof("action") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_RESOURCES,
     "resources",
     sizeof("resources") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Permission::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Permission::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Permission::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ACTION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACTION];
    case ATTRIBUTE_ID_RESOURCES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOURCES];
    default: return 0;
    }
}

// CREATORS

Permission::Permission(bslma::Allocator* basicAllocator)
: d_resources(basicAllocator)
, d_action(basicAllocator)
{
}

Permission::Permission(const Permission& original,
                       bslma::Allocator* basicAllocator)
: d_resources(original.d_resources, basicAllocator)
, d_action(original.d_action, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Permission::Permission(Permission&& original) noexcept
: d_resources(bsl::move(original.d_resources)),
  d_action(bsl::move(original.d_action))
{
}

Permission::Permission(Permission&& original, bslma::Allocator* basicAllocator)
: d_resources(bsl::move(original.d_resources), basicAllocator)
, d_action(bsl::move(original.d_action), basicAllocator)
{
}
#endif

Permission::~Permission()
{
}

// MANIPULATORS

Permission& Permission::operator=(const Permission& rhs)
{
    if (this != &rhs) {
        d_action    = rhs.d_action;
        d_resources = rhs.d_resources;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Permission& Permission::operator=(Permission&& rhs)
{
    if (this != &rhs) {
        d_action    = bsl::move(rhs.d_action);
        d_resources = bsl::move(rhs.d_resources);
    }

    return *this;
}
#endif

void Permission::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_action);
    bdlat_ValueTypeFunctions::reset(&d_resources);
}

// ACCESSORS

bsl::ostream&
Permission::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("action", this->action());
    printer.printAttribute("resources", this->resources());
    printer.end();
    return stream;
}

// ----------
// class Role
// ----------

// CONSTANTS

const char Role::CLASS_NAME[] = "Role";

const bdlat_AttributeInfo Role::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ID,
     "id",
     sizeof("id") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PERMISSIONS,
     "permissions",
     sizeof("permissions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Role::lookupAttributeInfo(const char* name,
                                                     int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Role::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Role::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID];
    case ATTRIBUTE_ID_PERMISSIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PERMISSIONS];
    default: return 0;
    }
}

// CREATORS

Role::Role(bslma::Allocator* basicAllocator)
: d_permissions(basicAllocator)
, d_id(basicAllocator)
{
}

Role::Role(const Role& original, bslma::Allocator* basicAllocator)
: d_permissions(original.d_permissions, basicAllocator)
, d_id(original.d_id, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Role::Role(Role&& original) noexcept
: d_permissions(bsl::move(original.d_permissions)),
  d_id(bsl::move(original.d_id))
{
}

Role::Role(Role&& original, bslma::Allocator* basicAllocator)
: d_permissions(bsl::move(original.d_permissions), basicAllocator)
, d_id(bsl::move(original.d_id), basicAllocator)
{
}
#endif

Role::~Role()
{
}

// MANIPULATORS

Role& Role::operator=(const Role& rhs)
{
    if (this != &rhs) {
        d_id          = rhs.d_id;
        d_permissions = rhs.d_permissions;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Role& Role::operator=(Role&& rhs)
{
    if (this != &rhs) {
        d_id          = bsl::move(rhs.d_id);
        d_permissions = bsl::move(rhs.d_permissions);
    }

    return *this;
}
#endif

void Role::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_id);
    bdlat_ValueTypeFunctions::reset(&d_permissions);
}

// ACCESSORS

bsl::ostream&
Role::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("id", this->id());
    printer.printAttribute("permissions", this->permissions());
    printer.end();
    return stream;
}

// ------------
// class Policy
// ------------

// CONSTANTS

const char Policy::CLASS_NAME[] = "Policy";

const bdlat_AttributeInfo Policy::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ROLES,
     "roles",
     sizeof("roles") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Policy::lookupAttributeInfo(const char* name,
                                                       int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Policy::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Policy::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ROLES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROLES];
    default: return 0;
    }
}

// CREATORS

Policy::Policy(bslma::Allocator* basicAllocator)
: d_roles(basicAllocator)
{
}

Policy::Policy(const Policy& original, bslma::Allocator* basicAllocator)
: d_roles(original.d_roles, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Policy::Policy(Policy&& original) noexcept
: d_roles(bsl::move(original.d_roles))
{
}

Policy::Policy(Policy&& original, bslma::Allocator* basicAllocator)
: d_roles(bsl::move(original.d_roles), basicAllocator)
{
}
#endif

Policy::~Policy()
{
}

// MANIPULATORS

Policy& Policy::operator=(const Policy& rhs)
{
    if (this != &rhs) {
        d_roles = rhs.d_roles;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Policy& Policy::operator=(Policy&& rhs)
{
    if (this != &rhs) {
        d_roles = bsl::move(rhs.d_roles);
    }

    return *this;
}
#endif

void Policy::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_roles);
}

// ACCESSORS

bsl::ostream&
Policy::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("roles", this->roles());
    printer.end();
    return stream;
}

}  // close package namespace
}  // close enterprise namespace

// GENERATED BY BLP_BAS_CODEGEN_2026.08.06
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbpoly --msgComponent policies mqbpoly.xsd
