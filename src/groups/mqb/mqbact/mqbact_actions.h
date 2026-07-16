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

// mqbact_actions.h             *DO NOT EDIT*              @generated -*-C++-*-
#ifndef INCLUDED_MQBACT_ACTIONS
#define INCLUDED_MQBACT_ACTIONS

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

#include <bsl_iosfwd.h>
#include <bsl_limits.h>
#include <bsl_type_traits.h>

namespace BloombergLP {

namespace bslma {
class Allocator;
}

namespace mqbact {
class ConnectAdmin;
}
namespace mqbact {
class ConnectClient;
}
namespace mqbact {
class ConnectClusterNode;
}
namespace mqbact {
class ConnectProxy;
}
namespace mqbact {
class ExecuteAdminCommand;
}
namespace mqbact {
class QueueRead;
}
namespace mqbact {
class QueueWrite;
}
namespace mqbact {
class Action;
}
namespace mqbact {

// ==================
// class ConnectAdmin
// ==================

class ConnectAdmin {
    // INSTANCE DATA

  public:
    // TYPES

    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS

    static const char CLASS_NAME[];

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

    // MANIPULATORS

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

    // HIDDEN FRIENDS

    /// Returns `true` as this type has no attributes and so all objects of
    /// this type are considered equal.
    friend bool operator==(const ConnectAdmin&, const ConnectAdmin&)
    {
        return true;
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ConnectAdmin& lhs, const ConnectAdmin& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&       stream,
                                    const ConnectAdmin& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ConnectAdmin`.
    friend void hashAppend(t_HASH_ALGORITHM&, const ConnectAdmin&)
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbact::ConnectAdmin);
template <>
struct bdlat_UsesDefaultValueFlag<mqbact::ConnectAdmin> : bsl::true_type {};

namespace mqbact {

// ===================
// class ConnectClient
// ===================

class ConnectClient {
    // INSTANCE DATA

  public:
    // TYPES

    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS

    static const char CLASS_NAME[];

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

    // MANIPULATORS

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

    // HIDDEN FRIENDS

    /// Returns `true` as this type has no attributes and so all objects of
    /// this type are considered equal.
    friend bool operator==(const ConnectClient&, const ConnectClient&)
    {
        return true;
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ConnectClient& lhs, const ConnectClient& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const ConnectClient& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ConnectClient`.
    friend void hashAppend(t_HASH_ALGORITHM&, const ConnectClient&)
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbact::ConnectClient);
template <>
struct bdlat_UsesDefaultValueFlag<mqbact::ConnectClient> : bsl::true_type {};

namespace mqbact {

// ========================
// class ConnectClusterNode
// ========================

class ConnectClusterNode {
    // INSTANCE DATA

    bsl::string d_clusterName;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_CLUSTER_NAME = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_CLUSTER_NAME = 0 };

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

    /// Create an object of type `ConnectClusterNode` having the default value.
    ///  Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit ConnectClusterNode(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `ConnectClusterNode` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    ConnectClusterNode(const ConnectClusterNode& original,
                       bslma::Allocator*         basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `ConnectClusterNode` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    ConnectClusterNode(ConnectClusterNode&& original) noexcept;

    /// Create an object of type `ConnectClusterNode` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    ConnectClusterNode(ConnectClusterNode&& original,
                       bslma::Allocator*    basicAllocator);
#endif

    /// Destroy this object.
    ~ConnectClusterNode();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ConnectClusterNode& operator=(const ConnectClusterNode& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    ConnectClusterNode& operator=(ConnectClusterNode&& rhs);
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

    /// Return a reference to the modifiable "ClusterName" attribute of this
    /// object.
    bsl::string& clusterName();

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

    /// Return a reference offering non-modifiable access to the "ClusterName"
    /// attribute of this object.
    const bsl::string& clusterName() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ConnectClusterNode& lhs,
                           const ConnectClusterNode& rhs)
    {
        return lhs.clusterName() == rhs.clusterName();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ConnectClusterNode& lhs,
                           const ConnectClusterNode& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const ConnectClusterNode& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ConnectClusterNode`.
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const ConnectClusterNode& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.clusterName());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbact::ConnectClusterNode);
template <>
struct bdlat_UsesDefaultValueFlag<mqbact::ConnectClusterNode>
: bsl::true_type {};

namespace mqbact {

// ==================
// class ConnectProxy
// ==================

class ConnectProxy {
    // INSTANCE DATA

  public:
    // TYPES

    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS

    static const char CLASS_NAME[];

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

    // MANIPULATORS

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

    // HIDDEN FRIENDS

    /// Returns `true` as this type has no attributes and so all objects of
    /// this type are considered equal.
    friend bool operator==(const ConnectProxy&, const ConnectProxy&)
    {
        return true;
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ConnectProxy& lhs, const ConnectProxy& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&       stream,
                                    const ConnectProxy& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ConnectProxy`.
    friend void hashAppend(t_HASH_ALGORITHM&, const ConnectProxy&)
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbact::ConnectProxy);
template <>
struct bdlat_UsesDefaultValueFlag<mqbact::ConnectProxy> : bsl::true_type {};

namespace mqbact {

// =========================
// class ExecuteAdminCommand
// =========================

class ExecuteAdminCommand {
    // INSTANCE DATA

    bsl::string d_command;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_COMMAND = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_COMMAND = 0 };

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

    /// Create an object of type `ExecuteAdminCommand` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit ExecuteAdminCommand(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `ExecuteAdminCommand` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    ExecuteAdminCommand(const ExecuteAdminCommand& original,
                        bslma::Allocator*          basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `ExecuteAdminCommand` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    ExecuteAdminCommand(ExecuteAdminCommand&& original) noexcept;

    /// Create an object of type `ExecuteAdminCommand` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    ExecuteAdminCommand(ExecuteAdminCommand&& original,
                        bslma::Allocator*     basicAllocator);
#endif

    /// Destroy this object.
    ~ExecuteAdminCommand();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ExecuteAdminCommand& operator=(const ExecuteAdminCommand& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    ExecuteAdminCommand& operator=(ExecuteAdminCommand&& rhs);
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

    /// Return a reference to the modifiable "Command" attribute of this
    /// object.
    bsl::string& command();

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

    /// Return a reference offering non-modifiable access to the "Command"
    /// attribute of this object.
    const bsl::string& command() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ExecuteAdminCommand& lhs,
                           const ExecuteAdminCommand& rhs)
    {
        return lhs.command() == rhs.command();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ExecuteAdminCommand& lhs,
                           const ExecuteAdminCommand& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&              stream,
                                    const ExecuteAdminCommand& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ExecuteAdminCommand`.
    friend void hashAppend(t_HASH_ALGORITHM&          hashAlg,
                           const ExecuteAdminCommand& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.command());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbact::ExecuteAdminCommand);
template <>
struct bdlat_UsesDefaultValueFlag<mqbact::ExecuteAdminCommand>
: bsl::true_type {};

namespace mqbact {

// ===============
// class QueueRead
// ===============

class QueueRead {
    // INSTANCE DATA

    bsl::string d_uri;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_URI = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_URI = 0 };

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

    /// Create an object of type `QueueRead` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit QueueRead(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `QueueRead` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    QueueRead(const QueueRead& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `QueueRead` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    QueueRead(QueueRead&& original) noexcept;

    /// Create an object of type `QueueRead` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    QueueRead(QueueRead&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~QueueRead();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    QueueRead& operator=(const QueueRead& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    QueueRead& operator=(QueueRead&& rhs);
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

    /// Return a reference to the modifiable "Uri" attribute of this object.
    bsl::string& uri();

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

    /// Return a reference offering non-modifiable access to the "Uri"
    /// attribute of this object.
    const bsl::string& uri() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const QueueRead& lhs, const QueueRead& rhs)
    {
        return lhs.uri() == rhs.uri();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const QueueRead& lhs, const QueueRead& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const QueueRead& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `QueueRead`.
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const QueueRead& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.uri());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbact::QueueRead);
template <>
struct bdlat_UsesDefaultValueFlag<mqbact::QueueRead> : bsl::true_type {};

namespace mqbact {

// ================
// class QueueWrite
// ================

class QueueWrite {
    // INSTANCE DATA

    bsl::string d_uri;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_URI = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_URI = 0 };

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

    /// Create an object of type `QueueWrite` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit QueueWrite(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `QueueWrite` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    QueueWrite(const QueueWrite& original,
               bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `QueueWrite` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    QueueWrite(QueueWrite&& original) noexcept;

    /// Create an object of type `QueueWrite` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    QueueWrite(QueueWrite&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~QueueWrite();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    QueueWrite& operator=(const QueueWrite& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    QueueWrite& operator=(QueueWrite&& rhs);
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

    /// Return a reference to the modifiable "Uri" attribute of this object.
    bsl::string& uri();

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

    /// Return a reference offering non-modifiable access to the "Uri"
    /// attribute of this object.
    const bsl::string& uri() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const QueueWrite& lhs, const QueueWrite& rhs)
    {
        return lhs.uri() == rhs.uri();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const QueueWrite& lhs, const QueueWrite& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&     stream,
                                    const QueueWrite& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `QueueWrite`.
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const QueueWrite& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.uri());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbact::QueueWrite);
template <>
struct bdlat_UsesDefaultValueFlag<mqbact::QueueWrite> : bsl::true_type {};

namespace mqbact {

// ============
// class Action
// ============

/// This type is used to represent possible cluster actions.
class Action {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<ConnectClient>       d_connectClient;
        bsls::ObjectBuffer<ConnectProxy>        d_connectProxy;
        bsls::ObjectBuffer<ConnectAdmin>        d_connectAdmin;
        bsls::ObjectBuffer<ConnectClusterNode>  d_connectClusterNode;
        bsls::ObjectBuffer<QueueRead>           d_queueRead;
        bsls::ObjectBuffer<QueueWrite>          d_queueWrite;
        bsls::ObjectBuffer<ExecuteAdminCommand> d_executeAdminCommand;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Action& rhs) const;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED             = -1,
        SELECTION_ID_CONNECT_CLIENT        = 0,
        SELECTION_ID_CONNECT_PROXY         = 1,
        SELECTION_ID_CONNECT_ADMIN         = 2,
        SELECTION_ID_CONNECT_CLUSTER_NODE  = 3,
        SELECTION_ID_QUEUE_READ            = 4,
        SELECTION_ID_QUEUE_WRITE           = 5,
        SELECTION_ID_EXECUTE_ADMIN_COMMAND = 6
    };

    enum { NUM_SELECTIONS = 7 };

    enum {
        SELECTION_INDEX_CONNECT_CLIENT        = 0,
        SELECTION_INDEX_CONNECT_PROXY         = 1,
        SELECTION_INDEX_CONNECT_ADMIN         = 2,
        SELECTION_INDEX_CONNECT_CLUSTER_NODE  = 3,
        SELECTION_INDEX_QUEUE_READ            = 4,
        SELECTION_INDEX_QUEUE_WRITE           = 5,
        SELECTION_INDEX_EXECUTE_ADMIN_COMMAND = 6
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_SelectionInfo SELECTION_INFO_ARRAY[];

    // CLASS METHODS

    /// Return selection information for the selection indicated by the
    /// specified `id` if the selection exists, and 0 otherwise.
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);

    /// Return selection information for the selection indicated by the
    /// specified `name` of the specified `nameLength` if the selection
    /// exists, and 0 otherwise.
    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `Action` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Action(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Action` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Action(const Action& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Action` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    Action(Action&& original) noexcept;

    /// Create an object of type `Action` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    Action(Action&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Action();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Action& operator=(const Action& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    Action& operator=(Action&& rhs);
#endif

    /// Reset this object to the default value (i.e., its value upon default
    /// construction).
    void reset();

    /// Set the value of this object to be the default for the selection
    /// indicated by the specified `selectionId`.  Return 0 on success, and
    /// non-zero value otherwise (i.e., the selection is not found).
    int makeSelection(int selectionId);

    /// Set the value of this object to be the default for the selection
    /// indicated by the specified `name` of the specified `nameLength`.
    /// Return 0 on success, and non-zero value otherwise (i.e., the
    /// selection is not found).
    int makeSelection(const char* name, int nameLength);

    /// Set the value of this object to be a `ConnectClient` value.  Optionally
    /// specify the `value` of the `ConnectClient`.  If `value` is not
    /// specified, the default `ConnectClient` value is used.
    ConnectClient& makeConnectClient();
    ConnectClient& makeConnectClient(const ConnectClient& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConnectClient& makeConnectClient(ConnectClient&& value);
#endif

    /// Set the value of this object to be a `ConnectProxy` value.  Optionally
    /// specify the `value` of the `ConnectProxy`.  If `value` is not
    /// specified, the default `ConnectProxy` value is used.
    ConnectProxy& makeConnectProxy();
    ConnectProxy& makeConnectProxy(const ConnectProxy& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConnectProxy& makeConnectProxy(ConnectProxy&& value);
#endif

    /// Set the value of this object to be a `ConnectAdmin` value.  Optionally
    /// specify the `value` of the `ConnectAdmin`.  If `value` is not
    /// specified, the default `ConnectAdmin` value is used.
    ConnectAdmin& makeConnectAdmin();
    ConnectAdmin& makeConnectAdmin(const ConnectAdmin& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConnectAdmin& makeConnectAdmin(ConnectAdmin&& value);
#endif

    /// Set the value of this object to be a `ConnectClusterNode` value.
    /// Optionally specify the `value` of the `ConnectClusterNode`.  If `value`
    /// is not specified, the default `ConnectClusterNode` value is used.
    ConnectClusterNode& makeConnectClusterNode();
    ConnectClusterNode&
    makeConnectClusterNode(const ConnectClusterNode& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConnectClusterNode& makeConnectClusterNode(ConnectClusterNode&& value);
#endif

    /// Set the value of this object to be a `QueueRead` value.  Optionally
    /// specify the `value` of the `QueueRead`.  If `value` is not specified,
    /// the default `QueueRead` value is used.
    QueueRead& makeQueueRead();
    QueueRead& makeQueueRead(const QueueRead& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueRead& makeQueueRead(QueueRead&& value);
#endif

    /// Set the value of this object to be a `QueueWrite` value.  Optionally
    /// specify the `value` of the `QueueWrite`.  If `value` is not specified,
    /// the default `QueueWrite` value is used.
    QueueWrite& makeQueueWrite();
    QueueWrite& makeQueueWrite(const QueueWrite& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueWrite& makeQueueWrite(QueueWrite&& value);
#endif

    /// Set the value of this object to be a `ExecuteAdminCommand` value.
    /// Optionally specify the `value` of the `ExecuteAdminCommand`.  If
    /// `value` is not specified, the default `ExecuteAdminCommand` value is
    /// used.
    ExecuteAdminCommand& makeExecuteAdminCommand();
    ExecuteAdminCommand&
    makeExecuteAdminCommand(const ExecuteAdminCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ExecuteAdminCommand& makeExecuteAdminCommand(ExecuteAdminCommand&& value);
#endif

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable `ConnectClient` selection of this
    /// object if `ConnectClient` is the current selection.  The behavior is
    /// undefined unless `ConnectClient` is the selection of this object.
    ConnectClient& connectClient();

    /// Return a reference to the modifiable `ConnectProxy` selection of this
    /// object if `ConnectProxy` is the current selection.  The behavior is
    /// undefined unless `ConnectProxy` is the selection of this object.
    ConnectProxy& connectProxy();

    /// Return a reference to the modifiable `ConnectAdmin` selection of this
    /// object if `ConnectAdmin` is the current selection.  The behavior is
    /// undefined unless `ConnectAdmin` is the selection of this object.
    ConnectAdmin& connectAdmin();

    /// Return a reference to the modifiable `ConnectClusterNode` selection of
    /// this object if `ConnectClusterNode` is the current selection.  The
    /// behavior is undefined unless `ConnectClusterNode` is the selection of
    /// this object.
    ConnectClusterNode& connectClusterNode();

    /// Return a reference to the modifiable `QueueRead` selection of this
    /// object if `QueueRead` is the current selection.  The behavior is
    /// undefined unless `QueueRead` is the selection of this object.
    QueueRead& queueRead();

    /// Return a reference to the modifiable `QueueWrite` selection of this
    /// object if `QueueWrite` is the current selection.  The behavior is
    /// undefined unless `QueueWrite` is the selection of this object.
    QueueWrite& queueWrite();

    /// Return a reference to the modifiable `ExecuteAdminCommand` selection of
    /// this object if `ExecuteAdminCommand` is the current selection.  The
    /// behavior is undefined unless `ExecuteAdminCommand` is the selection of
    /// this object.
    ExecuteAdminCommand& executeAdminCommand();

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

    /// Return the id of the current selection if the selection is defined,
    /// and -1 otherwise.
    int selectionId() const;

    /// Invoke the specified `accessor` on the non-modifiable selection,
    /// supplying `accessor` with the corresponding selection information
    /// structure.  Return the value returned from the invocation of
    /// `accessor` if this object has a defined selection, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;

    /// Return a reference to the non-modifiable `ConnectClient` selection of
    /// this object if `ConnectClient` is the current selection.  The behavior
    /// is undefined unless `ConnectClient` is the selection of this object.
    const ConnectClient& connectClient() const;

    /// Return a reference to the non-modifiable `ConnectProxy` selection of
    /// this object if `ConnectProxy` is the current selection.  The behavior
    /// is undefined unless `ConnectProxy` is the selection of this object.
    const ConnectProxy& connectProxy() const;

    /// Return a reference to the non-modifiable `ConnectAdmin` selection of
    /// this object if `ConnectAdmin` is the current selection.  The behavior
    /// is undefined unless `ConnectAdmin` is the selection of this object.
    const ConnectAdmin& connectAdmin() const;

    /// Return a reference to the non-modifiable `ConnectClusterNode` selection
    /// of this object if `ConnectClusterNode` is the current selection.  The
    /// behavior is undefined unless `ConnectClusterNode` is the selection of
    /// this object.
    const ConnectClusterNode& connectClusterNode() const;

    /// Return a reference to the non-modifiable `QueueRead` selection of this
    /// object if `QueueRead` is the current selection.  The behavior is
    /// undefined unless `QueueRead` is the selection of this object.
    const QueueRead& queueRead() const;

    /// Return a reference to the non-modifiable `QueueWrite` selection of this
    /// object if `QueueWrite` is the current selection.  The behavior is
    /// undefined unless `QueueWrite` is the selection of this object.
    const QueueWrite& queueWrite() const;

    /// Return a reference to the non-modifiable `ExecuteAdminCommand`
    /// selection of this object if `ExecuteAdminCommand` is the current
    /// selection.  The behavior is undefined unless `ExecuteAdminCommand` is
    /// the selection of this object.
    const ExecuteAdminCommand& executeAdminCommand() const;

    /// Return `true` if the value of this object is a `ConnectClient` value,
    /// and return `false` otherwise.
    bool isConnectClientValue() const;

    /// Return `true` if the value of this object is a `ConnectProxy` value,
    /// and return `false` otherwise.
    bool isConnectProxyValue() const;

    /// Return `true` if the value of this object is a `ConnectAdmin` value,
    /// and return `false` otherwise.
    bool isConnectAdminValue() const;

    /// Return `true` if the value of this object is a `ConnectClusterNode`
    /// value, and return `false` otherwise.
    bool isConnectClusterNodeValue() const;

    /// Return `true` if the value of this object is a `QueueRead` value, and
    /// return `false` otherwise.
    bool isQueueReadValue() const;

    /// Return `true` if the value of this object is a `QueueWrite` value, and
    /// return `false` otherwise.
    bool isQueueWriteValue() const;

    /// Return `true` if the value of this object is a `ExecuteAdminCommand`
    /// value, and return `false` otherwise.
    bool isExecuteAdminCommandValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` objects have the same
    /// value, and `false` otherwise.  Two `Action` objects have the same value
    /// if either the selections in both objects have the same ids and the same
    /// values, or both selections are undefined.
    friend bool operator==(const Action& lhs, const Action& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Action& lhs, const Action& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const Action& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Action`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Action& object)
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbact::Action);

// ============================================================================
//                          INLINE DEFINITIONS
// ============================================================================

namespace mqbact {

// ------------------
// class ConnectAdmin
// ------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ConnectAdmin::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int ConnectAdmin::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ConnectAdmin::manipulateAttribute(t_MANIPULATOR& manipulator,
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

// ACCESSORS
template <typename t_ACCESSOR>
int ConnectAdmin::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int ConnectAdmin::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ConnectAdmin::accessAttribute(t_ACCESSOR& accessor,
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

// -------------------
// class ConnectClient
// -------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ConnectClient::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int ConnectClient::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ConnectClient::manipulateAttribute(t_MANIPULATOR& manipulator,
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

// ACCESSORS
template <typename t_ACCESSOR>
int ConnectClient::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int ConnectClient::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ConnectClient::accessAttribute(t_ACCESSOR& accessor,
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

// ------------------------
// class ConnectClusterNode
// ------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ConnectClusterNode::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_clusterName,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ConnectClusterNode::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CLUSTER_NAME: {
        return manipulator(&d_clusterName,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ConnectClusterNode::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& ConnectClusterNode::clusterName()
{
    return d_clusterName;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ConnectClusterNode::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_clusterName,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ConnectClusterNode::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CLUSTER_NAME: {
        return accessor(d_clusterName,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ConnectClusterNode::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& ConnectClusterNode::clusterName() const
{
    return d_clusterName;
}

// ------------------
// class ConnectProxy
// ------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ConnectProxy::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int ConnectProxy::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ConnectProxy::manipulateAttribute(t_MANIPULATOR& manipulator,
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

// ACCESSORS
template <typename t_ACCESSOR>
int ConnectProxy::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int ConnectProxy::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ConnectProxy::accessAttribute(t_ACCESSOR& accessor,
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

// -------------------------
// class ExecuteAdminCommand
// -------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ExecuteAdminCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_command,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ExecuteAdminCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                             int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_COMMAND: {
        return manipulator(&d_command,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ExecuteAdminCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& ExecuteAdminCommand::command()
{
    return d_command;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ExecuteAdminCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_command, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ExecuteAdminCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_COMMAND: {
        return accessor(d_command,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMMAND]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ExecuteAdminCommand::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& ExecuteAdminCommand::command() const
{
    return d_command;
}

// ---------------
// class QueueRead
// ---------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int QueueRead::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int QueueRead::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int QueueRead::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& QueueRead::uri()
{
    return d_uri;
}

// ACCESSORS
template <typename t_ACCESSOR>
int QueueRead::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int QueueRead::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int QueueRead::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& QueueRead::uri() const
{
    return d_uri;
}

// ----------------
// class QueueWrite
// ----------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int QueueWrite::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int QueueWrite::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int QueueWrite::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& QueueWrite::uri()
{
    return d_uri;
}

// ACCESSORS
template <typename t_ACCESSOR>
int QueueWrite::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int QueueWrite::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int QueueWrite::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& QueueWrite::uri() const
{
    return d_uri;
}

// ------------
// class Action
// ------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Action::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef Action Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_CONNECT_CLIENT:
        hashAppend(hashAlgorithm, this->connectClient());
        break;
    case Class::SELECTION_ID_CONNECT_PROXY:
        hashAppend(hashAlgorithm, this->connectProxy());
        break;
    case Class::SELECTION_ID_CONNECT_ADMIN:
        hashAppend(hashAlgorithm, this->connectAdmin());
        break;
    case Class::SELECTION_ID_CONNECT_CLUSTER_NODE:
        hashAppend(hashAlgorithm, this->connectClusterNode());
        break;
    case Class::SELECTION_ID_QUEUE_READ:
        hashAppend(hashAlgorithm, this->queueRead());
        break;
    case Class::SELECTION_ID_QUEUE_WRITE:
        hashAppend(hashAlgorithm, this->queueWrite());
        break;
    case Class::SELECTION_ID_EXECUTE_ADMIN_COMMAND:
        hashAppend(hashAlgorithm, this->executeAdminCommand());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool Action::isEqualTo(const Action& rhs) const
{
    typedef Action Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_CONNECT_CLIENT:
            return this->connectClient() == rhs.connectClient();
        case Class::SELECTION_ID_CONNECT_PROXY:
            return this->connectProxy() == rhs.connectProxy();
        case Class::SELECTION_ID_CONNECT_ADMIN:
            return this->connectAdmin() == rhs.connectAdmin();
        case Class::SELECTION_ID_CONNECT_CLUSTER_NODE:
            return this->connectClusterNode() == rhs.connectClusterNode();
        case Class::SELECTION_ID_QUEUE_READ:
            return this->queueRead() == rhs.queueRead();
        case Class::SELECTION_ID_QUEUE_WRITE:
            return this->queueWrite() == rhs.queueWrite();
        case Class::SELECTION_ID_EXECUTE_ADMIN_COMMAND:
            return this->executeAdminCommand() == rhs.executeAdminCommand();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

// CREATORS
inline Action::Action(bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline Action::~Action()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int Action::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case Action::SELECTION_ID_CONNECT_CLIENT:
        return manipulator(
            &d_connectClient.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_CLIENT]);
    case Action::SELECTION_ID_CONNECT_PROXY:
        return manipulator(
            &d_connectProxy.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_PROXY]);
    case Action::SELECTION_ID_CONNECT_ADMIN:
        return manipulator(
            &d_connectAdmin.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_ADMIN]);
    case Action::SELECTION_ID_CONNECT_CLUSTER_NODE:
        return manipulator(
            &d_connectClusterNode.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_CLUSTER_NODE]);
    case Action::SELECTION_ID_QUEUE_READ:
        return manipulator(&d_queueRead.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_READ]);
    case Action::SELECTION_ID_QUEUE_WRITE:
        return manipulator(&d_queueWrite.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_WRITE]);
    case Action::SELECTION_ID_EXECUTE_ADMIN_COMMAND:
        return manipulator(
            &d_executeAdminCommand.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_EXECUTE_ADMIN_COMMAND]);
    default:
        BSLS_ASSERT(Action::SELECTION_ID_UNDEFINED == d_selectionId);
        return -1;
    }
}

inline ConnectClient& Action::connectClient()
{
    BSLS_ASSERT(SELECTION_ID_CONNECT_CLIENT == d_selectionId);
    return d_connectClient.object();
}

inline ConnectProxy& Action::connectProxy()
{
    BSLS_ASSERT(SELECTION_ID_CONNECT_PROXY == d_selectionId);
    return d_connectProxy.object();
}

inline ConnectAdmin& Action::connectAdmin()
{
    BSLS_ASSERT(SELECTION_ID_CONNECT_ADMIN == d_selectionId);
    return d_connectAdmin.object();
}

inline ConnectClusterNode& Action::connectClusterNode()
{
    BSLS_ASSERT(SELECTION_ID_CONNECT_CLUSTER_NODE == d_selectionId);
    return d_connectClusterNode.object();
}

inline QueueRead& Action::queueRead()
{
    BSLS_ASSERT(SELECTION_ID_QUEUE_READ == d_selectionId);
    return d_queueRead.object();
}

inline QueueWrite& Action::queueWrite()
{
    BSLS_ASSERT(SELECTION_ID_QUEUE_WRITE == d_selectionId);
    return d_queueWrite.object();
}

inline ExecuteAdminCommand& Action::executeAdminCommand()
{
    BSLS_ASSERT(SELECTION_ID_EXECUTE_ADMIN_COMMAND == d_selectionId);
    return d_executeAdminCommand.object();
}

// ACCESSORS
inline int Action::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int Action::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_CONNECT_CLIENT:
        return accessor(d_connectClient.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_CLIENT]);
    case SELECTION_ID_CONNECT_PROXY:
        return accessor(d_connectProxy.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_PROXY]);
    case SELECTION_ID_CONNECT_ADMIN:
        return accessor(d_connectAdmin.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_ADMIN]);
    case SELECTION_ID_CONNECT_CLUSTER_NODE:
        return accessor(
            d_connectClusterNode.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_CONNECT_CLUSTER_NODE]);
    case SELECTION_ID_QUEUE_READ:
        return accessor(d_queueRead.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_READ]);
    case SELECTION_ID_QUEUE_WRITE:
        return accessor(d_queueWrite.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_QUEUE_WRITE]);
    case SELECTION_ID_EXECUTE_ADMIN_COMMAND:
        return accessor(
            d_executeAdminCommand.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_EXECUTE_ADMIN_COMMAND]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const ConnectClient& Action::connectClient() const
{
    BSLS_ASSERT(SELECTION_ID_CONNECT_CLIENT == d_selectionId);
    return d_connectClient.object();
}

inline const ConnectProxy& Action::connectProxy() const
{
    BSLS_ASSERT(SELECTION_ID_CONNECT_PROXY == d_selectionId);
    return d_connectProxy.object();
}

inline const ConnectAdmin& Action::connectAdmin() const
{
    BSLS_ASSERT(SELECTION_ID_CONNECT_ADMIN == d_selectionId);
    return d_connectAdmin.object();
}

inline const ConnectClusterNode& Action::connectClusterNode() const
{
    BSLS_ASSERT(SELECTION_ID_CONNECT_CLUSTER_NODE == d_selectionId);
    return d_connectClusterNode.object();
}

inline const QueueRead& Action::queueRead() const
{
    BSLS_ASSERT(SELECTION_ID_QUEUE_READ == d_selectionId);
    return d_queueRead.object();
}

inline const QueueWrite& Action::queueWrite() const
{
    BSLS_ASSERT(SELECTION_ID_QUEUE_WRITE == d_selectionId);
    return d_queueWrite.object();
}

inline const ExecuteAdminCommand& Action::executeAdminCommand() const
{
    BSLS_ASSERT(SELECTION_ID_EXECUTE_ADMIN_COMMAND == d_selectionId);
    return d_executeAdminCommand.object();
}

inline bool Action::isConnectClientValue() const
{
    return SELECTION_ID_CONNECT_CLIENT == d_selectionId;
}

inline bool Action::isConnectProxyValue() const
{
    return SELECTION_ID_CONNECT_PROXY == d_selectionId;
}

inline bool Action::isConnectAdminValue() const
{
    return SELECTION_ID_CONNECT_ADMIN == d_selectionId;
}

inline bool Action::isConnectClusterNodeValue() const
{
    return SELECTION_ID_CONNECT_CLUSTER_NODE == d_selectionId;
}

inline bool Action::isQueueReadValue() const
{
    return SELECTION_ID_QUEUE_READ == d_selectionId;
}

inline bool Action::isQueueWriteValue() const
{
    return SELECTION_ID_QUEUE_WRITE == d_selectionId;
}

inline bool Action::isExecuteAdminCommandValue() const
{
    return SELECTION_ID_EXECUTE_ADMIN_COMMAND == d_selectionId;
}

inline bool Action::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}
}  // close package namespace

// FREE FUNCTIONS

}  // close enterprise namespace
#endif

// GENERATED BY BLP_BAS_CODEGEN_2026.05.28
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbact --msgComponent actions mqbact.xsd
