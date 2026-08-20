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

// mqbpoly_policies.h            *DO NOT EDIT*             @generated -*-C++-*-
#ifndef INCLUDED_MQBPOLY_POLICIES
#define INCLUDED_MQBPOLY_POLICIES

//@PURPOSE: Provide value-semantic attribute classes

#include <bslalg_typetraits.h>

#include <bdlat_attributeinfo.h>

#include <bdlat_selectioninfo.h>

#include <bdlat_typetraits.h>

#include <bslh_hash.h>
#include <bsls_objectbuffer.h>

#include <bslma_default.h>

#include <bsls_assert.h>

#include <bsl_string.h>

#include <bsl_vector.h>

#include <bsl_iosfwd.h>
#include <bsl_limits.h>
#include <bsl_type_traits.h>

namespace BloombergLP {

namespace bslma {
class Allocator;
}

namespace mqbpoly {
class Identity;
}
namespace mqbpoly {
class Resource;
}
namespace mqbpoly {
class Permission;
}
namespace mqbpoly {
class Role;
}
namespace mqbpoly {
class Policy;
}
namespace mqbpoly {

// ==============
// class Identity
// ==============

/// This type represents an authenticated identity.
class Identity {
    // INSTANCE DATA

    bsl::string d_name;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_NAME = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_NAME = 0 };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `Identity` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Identity(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Identity` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Identity(const Identity& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Identity` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    Identity(Identity&& original) noexcept;

    /// Create an object of type `Identity` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    Identity(Identity&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Identity();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Identity& operator=(const Identity& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    Identity& operator=(Identity&& rhs);
#endif

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "Name" attribute of this object.
    bsl::string& name();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return a reference offering non-modifiable access to the "Name"
    /// attribute of this object.
    const bsl::string& name() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const Identity& lhs, const Identity& rhs)
    {
        return lhs.name() == rhs.name();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Identity& lhs, const Identity& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const Identity& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Identity`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Identity& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.name());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbpoly::Identity);
template <>
struct bdlat_UsesDefaultValueFlag<mqbpoly::Identity> : bsl::true_type {};

namespace mqbpoly {

// ==============
// class Resource
// ==============

/// This type represents a resource.  The meaning of the identifier depends on
/// the context of the action the resource is associated with.
class Resource {
    // INSTANCE DATA

    bsl::string d_id;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_ID = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_ID = 0 };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `Resource` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Resource(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Resource` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Resource(const Resource& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Resource` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    Resource(Resource&& original) noexcept;

    /// Create an object of type `Resource` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    Resource(Resource&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Resource();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Resource& operator=(const Resource& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    Resource& operator=(Resource&& rhs);
#endif

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "Id" attribute of this object.
    bsl::string& id();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return a reference offering non-modifiable access to the "Id" attribute
    /// of this object.
    const bsl::string& id() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const Resource& lhs, const Resource& rhs)
    {
        return lhs.id() == rhs.id();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Resource& lhs, const Resource& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const Resource& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Resource`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Resource& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.id());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbpoly::Resource);
template <>
struct bdlat_UsesDefaultValueFlag<mqbpoly::Resource> : bsl::true_type {};

namespace mqbpoly {

// ================
// class Permission
// ================

/// This type represents a permission to access a resource.  A permission is a
/// simple association of an action identifier with a set of resources related
/// to that action.
/// The list of supported actions are: - connectClient - connectProxy -
/// connectAdmin - connectClusterNode - queueRead - queueWrite -
/// executeAdminCommand
class Permission {
    // INSTANCE DATA

    bsl::vector<Resource> d_resources;
    bsl::string           d_action;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_ACTION = 0, ATTRIBUTE_ID_RESOURCES = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_ACTION = 0, ATTRIBUTE_INDEX_RESOURCES = 1 };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `Permission` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Permission(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Permission` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Permission(const Permission& original,
               bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Permission` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    Permission(Permission&& original) noexcept;

    /// Create an object of type `Permission` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    Permission(Permission&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Permission();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Permission& operator=(const Permission& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    Permission& operator=(Permission&& rhs);
#endif

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "Action" attribute of this object.
    bsl::string& action();

    /// Return a reference to the modifiable "Resources" attribute of this
    /// object.
    bsl::vector<Resource>& resources();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return a reference offering non-modifiable access to the "Action"
    /// attribute of this object.
    const bsl::string& action() const;

    /// Return a reference offering non-modifiable access to the "Resources"
    /// attribute of this object.
    const bsl::vector<Resource>& resources() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const Permission& lhs, const Permission& rhs)
    {
        return lhs.action() == rhs.action() &&
               lhs.resources() == rhs.resources();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Permission& lhs, const Permission& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&     stream,
                                    const Permission& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Permission`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Permission& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.action());
        hashAppend(hashAlg, object.resources());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbpoly::Permission);
template <>
struct bdlat_UsesDefaultValueFlag<mqbpoly::Permission> : bsl::true_type {};

namespace mqbpoly {

// ==========
// class Role
// ==========

/// This type represents a role that describes access levels on the cluster.  A
/// role is described by two fields: 1.  id :: The identity (principle)
/// associated with this role 2.  permissions :: The list of actions allowed
/// for this identity
class Role {
    // INSTANCE DATA

    bsl::vector<Permission> d_permissions;
    Identity                d_id;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_ID = 0, ATTRIBUTE_ID_PERMISSIONS = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_ID = 0, ATTRIBUTE_INDEX_PERMISSIONS = 1 };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `Role` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Role(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Role` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Role(const Role& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Role` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    Role(Role&& original) noexcept;

    /// Create an object of type `Role` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    Role(Role&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Role();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Role& operator=(const Role& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    Role& operator=(Role&& rhs);
#endif

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "Id" attribute of this object.
    Identity& id();

    /// Return a reference to the modifiable "Permissions" attribute of this
    /// object.
    bsl::vector<Permission>& permissions();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return a reference offering non-modifiable access to the "Id" attribute
    /// of this object.
    const Identity& id() const;

    /// Return a reference offering non-modifiable access to the "Permissions"
    /// attribute of this object.
    const bsl::vector<Permission>& permissions() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const Role& lhs, const Role& rhs)
    {
        return lhs.id() == rhs.id() && lhs.permissions() == rhs.permissions();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Role& lhs, const Role& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const Role& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Role`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Role& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.id());
        hashAppend(hashAlg, object.permissions());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbpoly::Role);
template <>
struct bdlat_UsesDefaultValueFlag<mqbpoly::Role> : bsl::true_type {};

namespace mqbpoly {

// ============
// class Policy
// ============

/// This type is used by the default authorizer to describe cluster access
/// policies.
class Policy {
    // INSTANCE DATA

    bsl::vector<Role> d_roles;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_ROLES = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_ROLES = 0 };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `Policy` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Policy(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Policy` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Policy(const Policy& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Policy` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    Policy(Policy&& original) noexcept;

    /// Create an object of type `Policy` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    Policy(Policy&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Policy();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Policy& operator=(const Policy& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    Policy& operator=(Policy&& rhs);
#endif

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "Roles" attribute of this object.
    bsl::vector<Role>& roles();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return a reference offering non-modifiable access to the "Roles"
    /// attribute of this object.
    const bsl::vector<Role>& roles() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const Policy& lhs, const Policy& rhs)
    {
        return lhs.roles() == rhs.roles();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Policy& lhs, const Policy& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const Policy& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Policy`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Policy& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.roles());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbpoly::Policy);
template <>
struct bdlat_UsesDefaultValueFlag<mqbpoly::Policy> : bsl::true_type {};

// ============================================================================
//                          INLINE DEFINITIONS
// ============================================================================

namespace mqbpoly {

// --------------
// class Identity
// --------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Identity::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Identity::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Identity::manipulateAttribute(t_MANIPULATOR& manipulator,
                                  const char*    name,
                                  int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& Identity::name()
{
    return d_name;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Identity::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Identity::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Identity::accessAttribute(t_ACCESSOR& accessor,
                              const char* name,
                              int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& Identity::name() const
{
    return d_name;
}

// --------------
// class Resource
// --------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Resource::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Resource::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ID: {
        return manipulator(&d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Resource::manipulateAttribute(t_MANIPULATOR& manipulator,
                                  const char*    name,
                                  int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& Resource::id()
{
    return d_id;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Resource::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Resource::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ID: {
        return accessor(d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Resource::accessAttribute(t_ACCESSOR& accessor,
                              const char* name,
                              int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& Resource::id() const
{
    return d_id;
}

// ----------------
// class Permission
// ----------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Permission::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_action, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACTION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_resources,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOURCES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Permission::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ACTION: {
        return manipulator(&d_action,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACTION]);
    }
    case ATTRIBUTE_ID_RESOURCES: {
        return manipulator(&d_resources,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOURCES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Permission::manipulateAttribute(t_MANIPULATOR& manipulator,
                                    const char*    name,
                                    int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& Permission::action()
{
    return d_action;
}

inline bsl::vector<Resource>& Permission::resources()
{
    return d_resources;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Permission::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_action, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACTION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_resources,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOURCES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Permission::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ACTION: {
        return accessor(d_action,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACTION]);
    }
    case ATTRIBUTE_ID_RESOURCES: {
        return accessor(d_resources,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOURCES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Permission::accessAttribute(t_ACCESSOR& accessor,
                                const char* name,
                                int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& Permission::action() const
{
    return d_action;
}

inline const bsl::vector<Resource>& Permission::resources() const
{
    return d_resources;
}

// ----------
// class Role
// ----------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Role::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_permissions,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PERMISSIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Role::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ID: {
        return manipulator(&d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    }
    case ATTRIBUTE_ID_PERMISSIONS: {
        return manipulator(&d_permissions,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PERMISSIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Role::manipulateAttribute(t_MANIPULATOR& manipulator,
                              const char*    name,
                              int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline Identity& Role::id()
{
    return d_id;
}

inline bsl::vector<Permission>& Role::permissions()
{
    return d_permissions;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Role::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_permissions,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PERMISSIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Role::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ID: {
        return accessor(d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    }
    case ATTRIBUTE_ID_PERMISSIONS: {
        return accessor(d_permissions,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PERMISSIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Role::accessAttribute(t_ACCESSOR& accessor,
                          const char* name,
                          int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const Identity& Role::id() const
{
    return d_id;
}

inline const bsl::vector<Permission>& Role::permissions() const
{
    return d_permissions;
}

// ------------
// class Policy
// ------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Policy::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_roles, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROLES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Policy::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ROLES: {
        return manipulator(&d_roles,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROLES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Policy::manipulateAttribute(t_MANIPULATOR& manipulator,
                                const char*    name,
                                int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::vector<Role>& Policy::roles()
{
    return d_roles;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Policy::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_roles, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROLES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Policy::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ROLES: {
        return accessor(d_roles, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROLES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Policy::accessAttribute(t_ACCESSOR& accessor,
                            const char* name,
                            int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::vector<Role>& Policy::roles() const
{
    return d_roles;
}

}  // close package namespace

// FREE FUNCTIONS

}  // close enterprise namespace
#endif

// GENERATED BY BLP_BAS_CODEGEN_2026.08.06
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbpoly --msgComponent policies mqbpoly.xsd
