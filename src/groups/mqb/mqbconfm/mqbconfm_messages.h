// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqbconfm_messages.h            *DO NOT EDIT*            @generated -*-C++-*-
#ifndef INCLUDED_MQBCONFM_MESSAGES
#define INCLUDED_MQBCONFM_MESSAGES

//@PURPOSE: Provide value-semantic attribute classes

#include <bslalg_typetraits.h>

#include <bdlat_attributeinfo.h>

#include <bdlat_selectioninfo.h>

#include <bdlat_typetraits.h>

#include <bslh_hash.h>
#include <bsls_objectbuffer.h>

#include <bslma_default.h>

#include <bsls_assert.h>

#include <bdlb_nullablevalue.h>

#include <bsl_string.h>

#include <bsl_vector.h>

#include <bsls_types.h>

#include <bsl_iosfwd.h>
#include <bsl_limits.h>

namespace BloombergLP {

namespace bslma {
class Allocator;
}

namespace mqbconfm {
class BrokerIdentity;
}
namespace mqbconfm {
class DomainConfigRaw;
}
namespace mqbconfm {
class DomainResolver;
}
namespace mqbconfm {
class Failure;
}
namespace mqbconfm {
class FileBackedStorage;
}
namespace mqbconfm {
class InMemoryStorage;
}
namespace mqbconfm {
class Limits;
}
namespace mqbconfm {
class MsgGroupIdConfig;
}
namespace mqbconfm {
class QueueConsistencyEventual;
}
namespace mqbconfm {
class QueueConsistencyStrong;
}
namespace mqbconfm {
class QueueModeBroadcast;
}
namespace mqbconfm {
class QueueModeFanout;
}
namespace mqbconfm {
class QueueModePriority;
}
namespace mqbconfm {
class Consistency;
}
namespace mqbconfm {
class DomainConfigRequest;
}
namespace mqbconfm {
class QueueMode;
}
namespace mqbconfm {
class Response;
}
namespace mqbconfm {
class Storage;
}
namespace mqbconfm {
class Request;
}
namespace mqbconfm {
class StorageDefinition;
}
namespace mqbconfm {
class Domain;
}
namespace mqbconfm {
class DomainDefinition;
}
namespace mqbconfm {
class DomainVariant;
}
namespace mqbconfm {

// ====================
// class BrokerIdentity
// ====================

/// Generic type to hold identification of a broker.
/// hostName......: machine name hostTags......: machine tags
/// brokerVersion.: version of the broker
class BrokerIdentity {
    // INSTANCE DATA
    bsl::string d_hostName;
    bsl::string d_hostTags;
    bsl::string d_brokerVersion;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_HOST_NAME      = 0,
        ATTRIBUTE_ID_HOST_TAGS      = 1,
        ATTRIBUTE_ID_BROKER_VERSION = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_HOST_NAME      = 0,
        ATTRIBUTE_INDEX_HOST_TAGS      = 1,
        ATTRIBUTE_INDEX_BROKER_VERSION = 2
    };

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

    /// Create an object of type `BrokerIdentity` having the default value.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit BrokerIdentity(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `BrokerIdentity` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    BrokerIdentity(const BrokerIdentity& original,
                   bslma::Allocator*     basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `BrokerIdentity` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    BrokerIdentity(BrokerIdentity&& original) noexcept;

    /// Create an object of type `BrokerIdentity` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    BrokerIdentity(BrokerIdentity&&  original,
                   bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~BrokerIdentity();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    BrokerIdentity& operator=(const BrokerIdentity& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    BrokerIdentity& operator=(BrokerIdentity&& rhs);
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

    /// Return a reference to the modifiable "HostName" attribute of this
    /// object.
    bsl::string& hostName();

    /// Return a reference to the modifiable "HostTags" attribute of this
    /// object.
    bsl::string& hostTags();

    /// Return a reference to the modifiable "BrokerVersion" attribute of
    /// this object.
    bsl::string& brokerVersion();

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

    /// Return a reference offering non-modifiable access to the "HostName"
    /// attribute of this object.
    const bsl::string& hostName() const;

    /// Return a reference offering non-modifiable access to the "HostTags"
    /// attribute of this object.
    const bsl::string& hostTags() const;

    /// Return a reference offering non-modifiable access to the
    /// "BrokerVersion" attribute of this object.
    const bsl::string& brokerVersion() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const BrokerIdentity& lhs, const BrokerIdentity& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const BrokerIdentity& lhs, const BrokerIdentity& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&         stream,
                                const BrokerIdentity& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `BrokerIdentity`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const BrokerIdentity& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::BrokerIdentity)

namespace mqbconfm {

// =====================
// class DomainConfigRaw
// =====================

/// Response of a get domain config request.
class DomainConfigRaw {
    // INSTANCE DATA
    bsl::string d_domainName;
    bsl::string d_config;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_DOMAIN_NAME = 0, ATTRIBUTE_ID_CONFIG = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_DOMAIN_NAME = 0, ATTRIBUTE_INDEX_CONFIG = 1 };

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

    /// Create an object of type `DomainConfigRaw` having the default value.
    ///  Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit DomainConfigRaw(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `DomainConfigRaw` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    DomainConfigRaw(const DomainConfigRaw& original,
                    bslma::Allocator*      basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `DomainConfigRaw` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    DomainConfigRaw(DomainConfigRaw&& original) noexcept;

    /// Create an object of type `DomainConfigRaw` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    DomainConfigRaw(DomainConfigRaw&& original,
                    bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~DomainConfigRaw();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    DomainConfigRaw& operator=(const DomainConfigRaw& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    DomainConfigRaw& operator=(DomainConfigRaw&& rhs);
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

    /// Return a reference to the modifiable "DomainName" attribute of this
    /// object.
    bsl::string& domainName();

    /// Return a reference to the modifiable "Config" attribute of this
    /// object.
    bsl::string& config();

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

    /// Return a reference offering non-modifiable access to the
    /// "DomainName" attribute of this object.
    const bsl::string& domainName() const;

    /// Return a reference offering non-modifiable access to the "Config"
    /// attribute of this object.
    const bsl::string& config() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const DomainConfigRaw& lhs, const DomainConfigRaw& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const DomainConfigRaw& lhs, const DomainConfigRaw& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&          stream,
                                const DomainConfigRaw& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `DomainConfigRaw`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const DomainConfigRaw& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainConfigRaw)

namespace mqbconfm {

// ====================
// class DomainResolver
// ====================

/// Top level type representing the information retrieved when resolving a
/// domain.  Review: Keep this? Why not just store the cluster name?
/// name....: Domain name cluster.: Cluster name
class DomainResolver {
    // INSTANCE DATA
    bsl::string d_name;
    bsl::string d_cluster;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_NAME = 0, ATTRIBUTE_ID_CLUSTER = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_NAME = 0, ATTRIBUTE_INDEX_CLUSTER = 1 };

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

    /// Create an object of type `DomainResolver` having the default value.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit DomainResolver(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `DomainResolver` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    DomainResolver(const DomainResolver& original,
                   bslma::Allocator*     basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `DomainResolver` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    DomainResolver(DomainResolver&& original) noexcept;

    /// Create an object of type `DomainResolver` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    DomainResolver(DomainResolver&&  original,
                   bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~DomainResolver();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    DomainResolver& operator=(const DomainResolver& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    DomainResolver& operator=(DomainResolver&& rhs);
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

    /// Return a reference to the modifiable "Name" attribute of this
    /// object.
    bsl::string& name();

    /// Return a reference to the modifiable "Cluster" attribute of this
    /// object.
    bsl::string& cluster();

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

    /// Return a reference offering non-modifiable access to the "Cluster"
    /// attribute of this object.
    const bsl::string& cluster() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const DomainResolver& lhs, const DomainResolver& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const DomainResolver& lhs, const DomainResolver& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&         stream,
                                const DomainResolver& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `DomainResolver`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const DomainResolver& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainResolver)

namespace mqbconfm {

// =============
// class Failure
// =============

/// Generic type to represent an error.
/// code.....: an integer value representing the error message..: an
/// optional string describing the error
class Failure {
    // INSTANCE DATA
    bsl::string d_message;
    int         d_code;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_CODE = 0, ATTRIBUTE_ID_MESSAGE = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_CODE = 0, ATTRIBUTE_INDEX_MESSAGE = 1 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_MESSAGE[];

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

    /// Create an object of type `Failure` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Failure(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Failure` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Failure(const Failure& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Failure` having the value of the specified
    /// `original` object.  After performing this action, the `original`
    /// object will be left in a valid, but unspecified state.
    Failure(Failure&& original) noexcept;

    /// Create an object of type `Failure` having the value of the specified
    /// `original` object.  After performing this action, the `original`
    /// object will be left in a valid, but unspecified state.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    Failure(Failure&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Failure();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Failure& operator=(const Failure& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    Failure& operator=(Failure&& rhs);
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

    /// Return a reference to the modifiable "Code" attribute of this
    /// object.
    int& code();

    /// Return a reference to the modifiable "Message" attribute of this
    /// object.
    bsl::string& message();

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

    /// Return the value of the "Code" attribute of this object.
    int code() const;

    /// Return a reference offering non-modifiable access to the "Message"
    /// attribute of this object.
    const bsl::string& message() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const Failure& lhs, const Failure& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const Failure& lhs, const Failure& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream& stream, const Failure& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `Failure`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const Failure& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::Failure)

namespace mqbconfm {

// =======================
// class FileBackedStorage
// =======================

/// Configuration for storage using a file on disk.
class FileBackedStorage {
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

    /// Create an object of type `FileBackedStorage` having the default
    /// value.
    FileBackedStorage();

    /// Create an object of type `FileBackedStorage` having the value of the
    /// specified `original` object.
    FileBackedStorage(const FileBackedStorage& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `FileBackedStorage` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    FileBackedStorage(FileBackedStorage&& original) = default;
#endif

    /// Destroy this object.
    ~FileBackedStorage();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    FileBackedStorage& operator=(const FileBackedStorage& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    FileBackedStorage& operator=(FileBackedStorage&& rhs);
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
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const FileBackedStorage& lhs,
                       const FileBackedStorage& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const FileBackedStorage& lhs,
                       const FileBackedStorage& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&            stream,
                                const FileBackedStorage& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `FileBackedStorage`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const FileBackedStorage& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::FileBackedStorage)

namespace mqbconfm {

// =====================
// class InMemoryStorage
// =====================

/// Configuration for storage using an in-memory map.
class InMemoryStorage {
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

    /// Create an object of type `InMemoryStorage` having the default value.
    InMemoryStorage();

    /// Create an object of type `InMemoryStorage` having the value of the
    /// specified `original` object.
    InMemoryStorage(const InMemoryStorage& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `InMemoryStorage` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    InMemoryStorage(InMemoryStorage&& original) = default;
#endif

    /// Destroy this object.
    ~InMemoryStorage();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    InMemoryStorage& operator=(const InMemoryStorage& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    InMemoryStorage& operator=(InMemoryStorage&& rhs);
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
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const InMemoryStorage& lhs, const InMemoryStorage& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const InMemoryStorage& lhs, const InMemoryStorage& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&          stream,
                                const InMemoryStorage& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `InMemoryStorage`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const InMemoryStorage& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::InMemoryStorage)

namespace mqbconfm {

// ============
// class Limits
// ============

/// Represent the various limitations to apply to either a `domain` or an
/// individual `queue`.
/// message................: maximum number of messages
/// messagesWatermarkRatio.: threshold ratio to the maximum number of
/// messages for which a high watermark alarm will trigger
/// byte...................: maximum cumulated number of bytes
/// bytesWatermarkRatio....: threshold ratio to the maximum cumulated number
/// of bytes for which a high watermark alarm will trigger
class Limits {
    // INSTANCE DATA
    double             d_messagesWatermarkRatio;
    double             d_bytesWatermarkRatio;
    bsls::Types::Int64 d_messages;
    bsls::Types::Int64 d_bytes;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_MESSAGES                 = 0,
        ATTRIBUTE_ID_MESSAGES_WATERMARK_RATIO = 1,
        ATTRIBUTE_ID_BYTES                    = 2,
        ATTRIBUTE_ID_BYTES_WATERMARK_RATIO    = 3
    };

    enum { NUM_ATTRIBUTES = 4 };

    enum {
        ATTRIBUTE_INDEX_MESSAGES                 = 0,
        ATTRIBUTE_INDEX_MESSAGES_WATERMARK_RATIO = 1,
        ATTRIBUTE_INDEX_BYTES                    = 2,
        ATTRIBUTE_INDEX_BYTES_WATERMARK_RATIO    = 3
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const double DEFAULT_INITIALIZER_MESSAGES_WATERMARK_RATIO;

    static const double DEFAULT_INITIALIZER_BYTES_WATERMARK_RATIO;

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

    /// Create an object of type `Limits` having the default value.
    Limits();

    /// Create an object of type `Limits` having the value of the specified
    /// `original` object.
    Limits(const Limits& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Limits` having the value of the specified
    /// `original` object.  After performing this action, the `original`
    /// object will be left in a valid, but unspecified state.
    Limits(Limits&& original) = default;
#endif

    /// Destroy this object.
    ~Limits();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Limits& operator=(const Limits& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    Limits& operator=(Limits&& rhs);
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

    /// Return a reference to the modifiable "Messages" attribute of this
    /// object.
    bsls::Types::Int64& messages();

    /// Return a reference to the modifiable "MessagesWatermarkRatio"
    /// attribute of this object.
    double& messagesWatermarkRatio();

    /// Return a reference to the modifiable "Bytes" attribute of this
    /// object.
    bsls::Types::Int64& bytes();

    /// Return a reference to the modifiable "BytesWatermarkRatio" attribute
    /// of this object.
    double& bytesWatermarkRatio();

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

    /// Return the value of the "Messages" attribute of this object.
    bsls::Types::Int64 messages() const;

    /// Return the value of the "MessagesWatermarkRatio" attribute of this
    /// object.
    double messagesWatermarkRatio() const;

    /// Return the value of the "Bytes" attribute of this object.
    bsls::Types::Int64 bytes() const;

    /// Return the value of the "BytesWatermarkRatio" attribute of this
    /// object.
    double bytesWatermarkRatio() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const Limits& lhs, const Limits& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const Limits& lhs, const Limits& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream& stream, const Limits& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `Limits`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const Limits& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::Limits)

namespace mqbconfm {

// ======================
// class MsgGroupIdConfig
// ======================

/// Configuration for the use of Group Ids for routing.  The garbage
/// collection arguments could be assigned manually or get calculated out of
/// statistics on the streams.  They are considered internal and our
/// intentions is <u>not</u> to give customers full control over those numbers.
/// Their role is to protect BMQ from abuse i.e.  cases of infinite Group
/// Ids being stored.  Another assumption is that 'maxGroups >> number of
/// consumers'.
/// rebalance..: groups will be dynamically rebalanced in way such that all
/// consumers have equal share of Group Ids assigned to them maxGroups..:
/// Maximum number of groups.  If the number of groups gets larger than
/// this, the least recently used one is evicted.  This is a "garbage
/// collection" parameter ttlSeconds.: minimum time of inactivity (no
/// messages for a Group Id), in seconds, before a group becomes available
/// for "garbage collection".  0 (the default) means unlimited
class MsgGroupIdConfig {
    // INSTANCE DATA
    bsls::Types::Int64 d_ttlSeconds;
    int                d_maxGroups;
    bool               d_rebalance;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_REBALANCE   = 0,
        ATTRIBUTE_ID_MAX_GROUPS  = 1,
        ATTRIBUTE_ID_TTL_SECONDS = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_REBALANCE   = 0,
        ATTRIBUTE_INDEX_MAX_GROUPS  = 1,
        ATTRIBUTE_INDEX_TTL_SECONDS = 2
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_REBALANCE;

    static const int DEFAULT_INITIALIZER_MAX_GROUPS;

    static const bsls::Types::Int64 DEFAULT_INITIALIZER_TTL_SECONDS;

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

    /// Create an object of type `MsgGroupIdConfig` having the default
    /// value.
    MsgGroupIdConfig();

    /// Create an object of type `MsgGroupIdConfig` having the value of the
    /// specified `original` object.
    MsgGroupIdConfig(const MsgGroupIdConfig& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `MsgGroupIdConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    MsgGroupIdConfig(MsgGroupIdConfig&& original) = default;
#endif

    /// Destroy this object.
    ~MsgGroupIdConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    MsgGroupIdConfig& operator=(const MsgGroupIdConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    MsgGroupIdConfig& operator=(MsgGroupIdConfig&& rhs);
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

    /// Return a reference to the modifiable "Rebalance" attribute of this
    /// object.
    bool& rebalance();

    /// Return a reference to the modifiable "MaxGroups" attribute of this
    /// object.
    int& maxGroups();

    /// Return a reference to the modifiable "TtlSeconds" attribute of this
    /// object.
    bsls::Types::Int64& ttlSeconds();

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

    /// Return the value of the "Rebalance" attribute of this object.
    bool rebalance() const;

    /// Return the value of the "MaxGroups" attribute of this object.
    int maxGroups() const;

    /// Return the value of the "TtlSeconds" attribute of this object.
    bsls::Types::Int64 ttlSeconds() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const MsgGroupIdConfig& lhs,
                       const MsgGroupIdConfig& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const MsgGroupIdConfig& lhs,
                       const MsgGroupIdConfig& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&           stream,
                                const MsgGroupIdConfig& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `MsgGroupIdConfig`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const MsgGroupIdConfig& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::MsgGroupIdConfig)

namespace mqbconfm {

// ==============================
// class QueueConsistencyEventual
// ==============================

/// Configuration for eventual consistency.
class QueueConsistencyEventual {
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

    /// Create an object of type `QueueConsistencyEventual` having the
    /// default value.
    QueueConsistencyEventual();

    /// Create an object of type `QueueConsistencyEventual` having the value
    /// of the specified `original` object.
    QueueConsistencyEventual(const QueueConsistencyEventual& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `QueueConsistencyEventual` having the value
    /// of the specified `original` object.  After performing this action,
    /// the `original` object will be left in a valid, but unspecified
    /// state.
    QueueConsistencyEventual(QueueConsistencyEventual&& original) = default;
#endif

    /// Destroy this object.
    ~QueueConsistencyEventual();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    QueueConsistencyEventual& operator=(const QueueConsistencyEventual& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    QueueConsistencyEventual& operator=(QueueConsistencyEventual&& rhs);
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
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const QueueConsistencyEventual& lhs,
                       const QueueConsistencyEventual& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const QueueConsistencyEventual& lhs,
                       const QueueConsistencyEventual& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&                   stream,
                                const QueueConsistencyEventual& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `QueueConsistencyEventual`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM&               hashAlg,
                const QueueConsistencyEventual& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(
    mqbconfm::QueueConsistencyEventual)

namespace mqbconfm {

// ============================
// class QueueConsistencyStrong
// ============================

/// Configuration for strong consistency.
class QueueConsistencyStrong {
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

    /// Create an object of type `QueueConsistencyStrong` having the default
    /// value.
    QueueConsistencyStrong();

    /// Create an object of type `QueueConsistencyStrong` having the value
    /// of the specified `original` object.
    QueueConsistencyStrong(const QueueConsistencyStrong& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `QueueConsistencyStrong` having the value
    /// of the specified `original` object.  After performing this action,
    /// the `original` object will be left in a valid, but unspecified
    /// state.
    QueueConsistencyStrong(QueueConsistencyStrong&& original) = default;
#endif

    /// Destroy this object.
    ~QueueConsistencyStrong();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    QueueConsistencyStrong& operator=(const QueueConsistencyStrong& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    QueueConsistencyStrong& operator=(QueueConsistencyStrong&& rhs);
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
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const QueueConsistencyStrong& lhs,
                       const QueueConsistencyStrong& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const QueueConsistencyStrong& lhs,
                       const QueueConsistencyStrong& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&                 stream,
                                const QueueConsistencyStrong& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `QueueConsistencyStrong`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM&             hashAlg,
                const QueueConsistencyStrong& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(
    mqbconfm::QueueConsistencyStrong)

namespace mqbconfm {

// ========================
// class QueueModeBroadcast
// ========================

/// Configuration for a broadcast queue.
class QueueModeBroadcast {
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

    /// Create an object of type `QueueModeBroadcast` having the default
    /// value.
    QueueModeBroadcast();

    /// Create an object of type `QueueModeBroadcast` having the value of
    /// the specified `original` object.
    QueueModeBroadcast(const QueueModeBroadcast& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `QueueModeBroadcast` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    QueueModeBroadcast(QueueModeBroadcast&& original) = default;
#endif

    /// Destroy this object.
    ~QueueModeBroadcast();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    QueueModeBroadcast& operator=(const QueueModeBroadcast& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    QueueModeBroadcast& operator=(QueueModeBroadcast&& rhs);
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
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const QueueModeBroadcast& lhs,
                       const QueueModeBroadcast& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const QueueModeBroadcast& lhs,
                       const QueueModeBroadcast& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&             stream,
                                const QueueModeBroadcast& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `QueueModeBroadcast`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const QueueModeBroadcast& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::QueueModeBroadcast)

namespace mqbconfm {

// =====================
// class QueueModeFanout
// =====================

/// Configuration for a fanout queue.
/// appIDs.: List of appIDs authorized to consume from the queue.
class QueueModeFanout {
    // INSTANCE DATA
    bsl::vector<bsl::string> d_appIDs;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_APP_I_DS = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_APP_I_DS = 0 };

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

    /// Create an object of type `QueueModeFanout` having the default value.
    ///  Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit QueueModeFanout(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `QueueModeFanout` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    QueueModeFanout(const QueueModeFanout& original,
                    bslma::Allocator*      basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `QueueModeFanout` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    QueueModeFanout(QueueModeFanout&& original) noexcept;

    /// Create an object of type `QueueModeFanout` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    QueueModeFanout(QueueModeFanout&& original,
                    bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~QueueModeFanout();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    QueueModeFanout& operator=(const QueueModeFanout& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    QueueModeFanout& operator=(QueueModeFanout&& rhs);
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

    /// Return a reference to the modifiable "AppIDs" attribute of this
    /// object.
    bsl::vector<bsl::string>& appIDs();

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

    /// Return a reference offering non-modifiable access to the "AppIDs"
    /// attribute of this object.
    const bsl::vector<bsl::string>& appIDs() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const QueueModeFanout& lhs, const QueueModeFanout& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const QueueModeFanout& lhs, const QueueModeFanout& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&          stream,
                                const QueueModeFanout& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `QueueModeFanout`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const QueueModeFanout& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::QueueModeFanout)

namespace mqbconfm {

// =======================
// class QueueModePriority
// =======================

/// Configuration for a priority queue.
class QueueModePriority {
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

    /// Create an object of type `QueueModePriority` having the default
    /// value.
    QueueModePriority();

    /// Create an object of type `QueueModePriority` having the value of the
    /// specified `original` object.
    QueueModePriority(const QueueModePriority& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `QueueModePriority` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    QueueModePriority(QueueModePriority&& original) = default;
#endif

    /// Destroy this object.
    ~QueueModePriority();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    QueueModePriority& operator=(const QueueModePriority& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    QueueModePriority& operator=(QueueModePriority&& rhs);
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
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const QueueModePriority& lhs,
                       const QueueModePriority& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const QueueModePriority& lhs,
                       const QueueModePriority& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&            stream,
                                const QueueModePriority& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `QueueModePriority`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const QueueModePriority& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::QueueModePriority)

namespace mqbconfm {

// =================
// class Consistency
// =================

/// Consistency choices to configure a queue.
/// eventual........: no Replication Receipt is required.  strong..........:
/// require Replication Receipt before ACK/PUSH
class Consistency {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<QueueConsistencyEventual> d_eventual;
        bsls::ObjectBuffer<QueueConsistencyStrong>   d_strong;
    };

    int d_selectionId;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED = -1,
        SELECTION_ID_EVENTUAL  = 0,
        SELECTION_ID_STRONG    = 1
    };

    enum { NUM_SELECTIONS = 2 };

    enum { SELECTION_INDEX_EVENTUAL = 0, SELECTION_INDEX_STRONG = 1 };

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

    /// Create an object of type `Consistency` having the default value.
    Consistency();

    /// Create an object of type `Consistency` having the value of the
    /// specified `original` object.
    Consistency(const Consistency& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Consistency` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    Consistency(Consistency&& original) noexcept;
#endif

    /// Destroy this object.
    ~Consistency();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Consistency& operator=(const Consistency& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    Consistency& operator=(Consistency&& rhs);
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

    QueueConsistencyEventual& makeEventual();
    QueueConsistencyEventual&
    makeEventual(const QueueConsistencyEventual& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueConsistencyEventual& makeEventual(QueueConsistencyEventual&& value);
#endif
    // Set the value of this object to be a "Eventual" value.  Optionally
    // specify the 'value' of the "Eventual".  If 'value' is not specified,
    // the default "Eventual" value is used.

    QueueConsistencyStrong& makeStrong();
    QueueConsistencyStrong& makeStrong(const QueueConsistencyStrong& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueConsistencyStrong& makeStrong(QueueConsistencyStrong&& value);
#endif
    // Set the value of this object to be a "Strong" value.  Optionally
    // specify the 'value' of the "Strong".  If 'value' is not specified,
    // the default "Strong" value is used.

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable "Eventual" selection of this
    /// object if "Eventual" is the current selection.  The behavior is
    /// undefined unless "Eventual" is the selection of this object.
    QueueConsistencyEventual& eventual();

    /// Return a reference to the modifiable "Strong" selection of this
    /// object if "Strong" is the current selection.  The behavior is
    /// undefined unless "Strong" is the selection of this object.
    QueueConsistencyStrong& strong();

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

    /// Return a reference to the non-modifiable "Eventual" selection of
    /// this object if "Eventual" is the current selection.  The behavior is
    /// undefined unless "Eventual" is the selection of this object.
    const QueueConsistencyEventual& eventual() const;

    /// Return a reference to the non-modifiable "Strong" selection of this
    /// object if "Strong" is the current selection.  The behavior is
    /// undefined unless "Strong" is the selection of this object.
    const QueueConsistencyStrong& strong() const;

    /// Return `true` if the value of this object is a "Eventual" value, and
    /// return `false` otherwise.
    bool isEventualValue() const;

    /// Return `true` if the value of this object is a "Strong" value, and
    /// return `false` otherwise.
    bool isStrongValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` objects have the same
/// value, and `false` otherwise.  Two `Consistency` objects have the same
/// value if either the selections in both objects have the same ids and
/// the same values, or both selections are undefined.
inline bool operator==(const Consistency& lhs, const Consistency& rhs);

/// Return `true` if the specified `lhs` and `rhs` objects do not have the
/// same values, as determined by `operator==`, and `false` otherwise.
inline bool operator!=(const Consistency& lhs, const Consistency& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream& stream, const Consistency& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `Consistency`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const Consistency& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::Consistency)

namespace mqbconfm {

// =========================
// class DomainConfigRequest
// =========================

/// Request to get a domain config.
class DomainConfigRequest {
    // INSTANCE DATA
    bsl::string    d_domainName;
    BrokerIdentity d_brokerIdentity;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_BROKER_IDENTITY = 0, ATTRIBUTE_ID_DOMAIN_NAME = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum {
        ATTRIBUTE_INDEX_BROKER_IDENTITY = 0,
        ATTRIBUTE_INDEX_DOMAIN_NAME     = 1
    };

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

    /// Create an object of type `DomainConfigRequest` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    explicit DomainConfigRequest(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `DomainConfigRequest` having the value of
    /// the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    DomainConfigRequest(const DomainConfigRequest& original,
                        bslma::Allocator*          basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `DomainConfigRequest` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    DomainConfigRequest(DomainConfigRequest&& original) noexcept;

    /// Create an object of type `DomainConfigRequest` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    DomainConfigRequest(DomainConfigRequest&& original,
                        bslma::Allocator*     basicAllocator);
#endif

    /// Destroy this object.
    ~DomainConfigRequest();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    DomainConfigRequest& operator=(const DomainConfigRequest& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    DomainConfigRequest& operator=(DomainConfigRequest&& rhs);
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

    /// Return a reference to the modifiable "BrokerIdentity" attribute of
    /// this object.
    BrokerIdentity& brokerIdentity();

    /// Return a reference to the modifiable "DomainName" attribute of this
    /// object.
    bsl::string& domainName();

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

    /// Return a reference offering non-modifiable access to the
    /// "BrokerIdentity" attribute of this object.
    const BrokerIdentity& brokerIdentity() const;

    /// Return a reference offering non-modifiable access to the
    /// "DomainName" attribute of this object.
    const bsl::string& domainName() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const DomainConfigRequest& lhs,
                       const DomainConfigRequest& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const DomainConfigRequest& lhs,
                       const DomainConfigRequest& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&              stream,
                                const DomainConfigRequest& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `DomainConfigRequest`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const DomainConfigRequest& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainConfigRequest)

namespace mqbconfm {

// ===============
// class QueueMode
// ===============

/// Choice of all the various mode a queue can be configured in.
/// fanout.........: multiple consumers are each getting all messages
/// priority.......: consumers with highest priority are sharing load in
/// round robin way broadcast......: send to all available consumers on a
/// best-effort basis
class QueueMode {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<QueueModeFanout>    d_fanout;
        bsls::ObjectBuffer<QueueModePriority>  d_priority;
        bsls::ObjectBuffer<QueueModeBroadcast> d_broadcast;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED = -1,
        SELECTION_ID_FANOUT    = 0,
        SELECTION_ID_PRIORITY  = 1,
        SELECTION_ID_BROADCAST = 2
    };

    enum { NUM_SELECTIONS = 3 };

    enum {
        SELECTION_INDEX_FANOUT    = 0,
        SELECTION_INDEX_PRIORITY  = 1,
        SELECTION_INDEX_BROADCAST = 2
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

    /// Create an object of type `QueueMode` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit QueueMode(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `QueueMode` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    QueueMode(const QueueMode& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `QueueMode` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    QueueMode(QueueMode&& original) noexcept;

    /// Create an object of type `QueueMode` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    QueueMode(QueueMode&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~QueueMode();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    QueueMode& operator=(const QueueMode& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    QueueMode& operator=(QueueMode&& rhs);
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

    QueueModeFanout& makeFanout();
    QueueModeFanout& makeFanout(const QueueModeFanout& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueModeFanout& makeFanout(QueueModeFanout&& value);
#endif
    // Set the value of this object to be a "Fanout" value.  Optionally
    // specify the 'value' of the "Fanout".  If 'value' is not specified,
    // the default "Fanout" value is used.

    QueueModePriority& makePriority();
    QueueModePriority& makePriority(const QueueModePriority& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueModePriority& makePriority(QueueModePriority&& value);
#endif
    // Set the value of this object to be a "Priority" value.  Optionally
    // specify the 'value' of the "Priority".  If 'value' is not specified,
    // the default "Priority" value is used.

    QueueModeBroadcast& makeBroadcast();
    QueueModeBroadcast& makeBroadcast(const QueueModeBroadcast& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueModeBroadcast& makeBroadcast(QueueModeBroadcast&& value);
#endif
    // Set the value of this object to be a "Broadcast" value.  Optionally
    // specify the 'value' of the "Broadcast".  If 'value' is not
    // specified, the default "Broadcast" value is used.

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable "Fanout" selection of this
    /// object if "Fanout" is the current selection.  The behavior is
    /// undefined unless "Fanout" is the selection of this object.
    QueueModeFanout& fanout();

    /// Return a reference to the modifiable "Priority" selection of this
    /// object if "Priority" is the current selection.  The behavior is
    /// undefined unless "Priority" is the selection of this object.
    QueueModePriority& priority();

    /// Return a reference to the modifiable "Broadcast" selection of this
    /// object if "Broadcast" is the current selection.  The behavior is
    /// undefined unless "Broadcast" is the selection of this object.
    QueueModeBroadcast& broadcast();

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

    /// Return a reference to the non-modifiable "Fanout" selection of this
    /// object if "Fanout" is the current selection.  The behavior is
    /// undefined unless "Fanout" is the selection of this object.
    const QueueModeFanout& fanout() const;

    /// Return a reference to the non-modifiable "Priority" selection of
    /// this object if "Priority" is the current selection.  The behavior is
    /// undefined unless "Priority" is the selection of this object.
    const QueueModePriority& priority() const;

    /// Return a reference to the non-modifiable "Broadcast" selection of
    /// this object if "Broadcast" is the current selection.  The behavior
    /// is undefined unless "Broadcast" is the selection of this object.
    const QueueModeBroadcast& broadcast() const;

    /// Return `true` if the value of this object is a "Fanout" value, and
    /// return `false` otherwise.
    bool isFanoutValue() const;

    /// Return `true` if the value of this object is a "Priority" value, and
    /// return `false` otherwise.
    bool isPriorityValue() const;

    /// Return `true` if the value of this object is a "Broadcast" value,
    /// and return `false` otherwise.
    bool isBroadcastValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` objects have the same
/// value, and `false` otherwise.  Two `QueueMode` objects have the same
/// value if either the selections in both objects have the same ids and
/// the same values, or both selections are undefined.
inline bool operator==(const QueueMode& lhs, const QueueMode& rhs);

/// Return `true` if the specified `lhs` and `rhs` objects do not have the
/// same values, as determined by `operator==`, and `false` otherwise.
inline bool operator!=(const QueueMode& lhs, const QueueMode& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream& stream, const QueueMode& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `QueueMode`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const QueueMode& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::QueueMode)

namespace mqbconfm {

// ==============
// class Response
// ==============

/// The choice between all possible response from the bmqconf task.
class Response {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<Failure>         d_failure;
        bsls::ObjectBuffer<DomainConfigRaw> d_domainConfig;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED     = -1,
        SELECTION_ID_FAILURE       = 0,
        SELECTION_ID_DOMAIN_CONFIG = 1
    };

    enum { NUM_SELECTIONS = 2 };

    enum { SELECTION_INDEX_FAILURE = 0, SELECTION_INDEX_DOMAIN_CONFIG = 1 };

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

    /// Create an object of type `Response` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Response(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Response` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    Response(const Response& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Response` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    Response(Response&& original) noexcept;

    /// Create an object of type `Response` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    Response(Response&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Response();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Response& operator=(const Response& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    Response& operator=(Response&& rhs);
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

    Failure& makeFailure();
    Failure& makeFailure(const Failure& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Failure& makeFailure(Failure&& value);
#endif
    // Set the value of this object to be a "Failure" value.  Optionally
    // specify the 'value' of the "Failure".  If 'value' is not specified,
    // the default "Failure" value is used.

    DomainConfigRaw& makeDomainConfig();
    DomainConfigRaw& makeDomainConfig(const DomainConfigRaw& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainConfigRaw& makeDomainConfig(DomainConfigRaw&& value);
#endif
    // Set the value of this object to be a "DomainConfig" value.
    // Optionally specify the 'value' of the "DomainConfig".  If 'value' is
    // not specified, the default "DomainConfig" value is used.

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable "Failure" selection of this
    /// object if "Failure" is the current selection.  The behavior is
    /// undefined unless "Failure" is the selection of this object.
    Failure& failure();

    /// Return a reference to the modifiable "DomainConfig" selection of
    /// this object if "DomainConfig" is the current selection.  The
    /// behavior is undefined unless "DomainConfig" is the selection of this
    /// object.
    DomainConfigRaw& domainConfig();

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

    /// Return a reference to the non-modifiable "Failure" selection of this
    /// object if "Failure" is the current selection.  The behavior is
    /// undefined unless "Failure" is the selection of this object.
    const Failure& failure() const;

    /// Return a reference to the non-modifiable "DomainConfig" selection of
    /// this object if "DomainConfig" is the current selection.  The
    /// behavior is undefined unless "DomainConfig" is the selection of this
    /// object.
    const DomainConfigRaw& domainConfig() const;

    /// Return `true` if the value of this object is a "Failure" value, and
    /// return `false` otherwise.
    bool isFailureValue() const;

    /// Return `true` if the value of this object is a "DomainConfig" value,
    /// and return `false` otherwise.
    bool isDomainConfigValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` objects have the same
/// value, and `false` otherwise.  Two `Response` objects have the same
/// value if either the selections in both objects have the same ids and
/// the same values, or both selections are undefined.
inline bool operator==(const Response& lhs, const Response& rhs);

/// Return `true` if the specified `lhs` and `rhs` objects do not have the
/// same values, as determined by `operator==`, and `false` otherwise.
inline bool operator!=(const Response& lhs, const Response& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream& stream, const Response& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `Response`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const Response& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::Response)

namespace mqbconfm {

// =============
// class Storage
// =============

/// Choice of all the various Storage backend
/// inMemory....: store data in memory fileBacked..: store data in a file on
/// disk
class Storage {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<InMemoryStorage>   d_inMemory;
        bsls::ObjectBuffer<FileBackedStorage> d_fileBacked;
    };

    int d_selectionId;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED   = -1,
        SELECTION_ID_IN_MEMORY   = 0,
        SELECTION_ID_FILE_BACKED = 1
    };

    enum { NUM_SELECTIONS = 2 };

    enum { SELECTION_INDEX_IN_MEMORY = 0, SELECTION_INDEX_FILE_BACKED = 1 };

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

    /// Create an object of type `Storage` having the default value.
    Storage();

    /// Create an object of type `Storage` having the value of the specified
    /// `original` object.
    Storage(const Storage& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Storage` having the value of the specified
    /// `original` object.  After performing this action, the `original`
    /// object will be left in a valid, but unspecified state.
    Storage(Storage&& original) noexcept;
#endif

    /// Destroy this object.
    ~Storage();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Storage& operator=(const Storage& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    Storage& operator=(Storage&& rhs);
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

    InMemoryStorage& makeInMemory();
    InMemoryStorage& makeInMemory(const InMemoryStorage& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    InMemoryStorage& makeInMemory(InMemoryStorage&& value);
#endif
    // Set the value of this object to be a "InMemory" value.  Optionally
    // specify the 'value' of the "InMemory".  If 'value' is not specified,
    // the default "InMemory" value is used.

    FileBackedStorage& makeFileBacked();
    FileBackedStorage& makeFileBacked(const FileBackedStorage& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    FileBackedStorage& makeFileBacked(FileBackedStorage&& value);
#endif
    // Set the value of this object to be a "FileBacked" value.  Optionally
    // specify the 'value' of the "FileBacked".  If 'value' is not
    // specified, the default "FileBacked" value is used.

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable "InMemory" selection of this
    /// object if "InMemory" is the current selection.  The behavior is
    /// undefined unless "InMemory" is the selection of this object.
    InMemoryStorage& inMemory();

    /// Return a reference to the modifiable "FileBacked" selection of this
    /// object if "FileBacked" is the current selection.  The behavior is
    /// undefined unless "FileBacked" is the selection of this object.
    FileBackedStorage& fileBacked();

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

    /// Return a reference to the non-modifiable "InMemory" selection of
    /// this object if "InMemory" is the current selection.  The behavior is
    /// undefined unless "InMemory" is the selection of this object.
    const InMemoryStorage& inMemory() const;

    /// Return a reference to the non-modifiable "FileBacked" selection of
    /// this object if "FileBacked" is the current selection.  The behavior
    /// is undefined unless "FileBacked" is the selection of this object.
    const FileBackedStorage& fileBacked() const;

    /// Return `true` if the value of this object is a "InMemory" value, and
    /// return `false` otherwise.
    bool isInMemoryValue() const;

    /// Return `true` if the value of this object is a "FileBacked" value,
    /// and return `false` otherwise.
    bool isFileBackedValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` objects have the same
/// value, and `false` otherwise.  Two `Storage` objects have the same
/// value if either the selections in both objects have the same ids and
/// the same values, or both selections are undefined.
inline bool operator==(const Storage& lhs, const Storage& rhs);

/// Return `true` if the specified `lhs` and `rhs` objects do not have the
/// same values, as determined by `operator==`, and `false` otherwise.
inline bool operator!=(const Storage& lhs, const Storage& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream& stream, const Storage& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `Storage`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const Storage& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::Storage)

namespace mqbconfm {

// =============
// class Request
// =============

/// The choice between all possible request to the bmqconf task.
class Request {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<DomainConfigRequest> d_domainConfig;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

  public:
    // TYPES

    enum { SELECTION_ID_UNDEFINED = -1, SELECTION_ID_DOMAIN_CONFIG = 0 };

    enum { NUM_SELECTIONS = 1 };

    enum { SELECTION_INDEX_DOMAIN_CONFIG = 0 };

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

    /// Create an object of type `Request` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Request(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Request` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Request(const Request& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Request` having the value of the specified
    /// `original` object.  After performing this action, the `original`
    /// object will be left in a valid, but unspecified state.
    Request(Request&& original) noexcept;

    /// Create an object of type `Request` having the value of the specified
    /// `original` object.  After performing this action, the `original`
    /// object will be left in a valid, but unspecified state.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    Request(Request&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Request();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Request& operator=(const Request& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    Request& operator=(Request&& rhs);
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

    DomainConfigRequest& makeDomainConfig();
    DomainConfigRequest& makeDomainConfig(const DomainConfigRequest& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainConfigRequest& makeDomainConfig(DomainConfigRequest&& value);
#endif
    // Set the value of this object to be a "DomainConfig" value.
    // Optionally specify the 'value' of the "DomainConfig".  If 'value' is
    // not specified, the default "DomainConfig" value is used.

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable "DomainConfig" selection of
    /// this object if "DomainConfig" is the current selection.  The
    /// behavior is undefined unless "DomainConfig" is the selection of this
    /// object.
    DomainConfigRequest& domainConfig();

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

    /// Return a reference to the non-modifiable "DomainConfig" selection of
    /// this object if "DomainConfig" is the current selection.  The
    /// behavior is undefined unless "DomainConfig" is the selection of this
    /// object.
    const DomainConfigRequest& domainConfig() const;

    /// Return `true` if the value of this object is a "DomainConfig" value,
    /// and return `false` otherwise.
    bool isDomainConfigValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` objects have the same
/// value, and `false` otherwise.  Two `Request` objects have the same
/// value if either the selections in both objects have the same ids and
/// the same values, or both selections are undefined.
inline bool operator==(const Request& lhs, const Request& rhs);

/// Return `true` if the specified `lhs` and `rhs` objects do not have the
/// same values, as determined by `operator==`, and `false` otherwise.
inline bool operator!=(const Request& lhs, const Request& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream& stream, const Request& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `Request`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const Request& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::Request)

namespace mqbconfm {

// =======================
// class StorageDefinition
// =======================

/// Type representing the configuration for a Storage.
/// config........: configuration for the type of storage to use
/// domainLimits..: global limits to apply to the entire domain, cumulated
/// for all queues in the domain queueLimits...: individual limits (as a
/// subset of the global limits) to apply to each queue of the domain
class StorageDefinition {
    // INSTANCE DATA
    Storage d_config;
    Limits  d_domainLimits;
    Limits  d_queueLimits;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_DOMAIN_LIMITS = 0,
        ATTRIBUTE_ID_QUEUE_LIMITS  = 1,
        ATTRIBUTE_ID_CONFIG        = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_DOMAIN_LIMITS = 0,
        ATTRIBUTE_INDEX_QUEUE_LIMITS  = 1,
        ATTRIBUTE_INDEX_CONFIG        = 2
    };

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

    /// Create an object of type `StorageDefinition` having the default
    /// value.
    StorageDefinition();

    /// Create an object of type `StorageDefinition` having the value of the
    /// specified `original` object.
    StorageDefinition(const StorageDefinition& original);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StorageDefinition` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    StorageDefinition(StorageDefinition&& original) = default;
#endif

    /// Destroy this object.
    ~StorageDefinition();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StorageDefinition& operator=(const StorageDefinition& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    StorageDefinition& operator=(StorageDefinition&& rhs);
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

    /// Return a reference to the modifiable "DomainLimits" attribute of
    /// this object.
    Limits& domainLimits();

    /// Return a reference to the modifiable "QueueLimits" attribute of this
    /// object.
    Limits& queueLimits();

    /// Return a reference to the modifiable "Config" attribute of this
    /// object.
    Storage& config();

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

    /// Return a reference offering non-modifiable access to the
    /// "DomainLimits" attribute of this object.
    const Limits& domainLimits() const;

    /// Return a reference offering non-modifiable access to the
    /// "QueueLimits" attribute of this object.
    const Limits& queueLimits() const;

    /// Return a reference offering non-modifiable access to the "Config"
    /// attribute of this object.
    const Storage& config() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const StorageDefinition& lhs,
                       const StorageDefinition& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const StorageDefinition& lhs,
                       const StorageDefinition& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&            stream,
                                const StorageDefinition& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `StorageDefinition`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const StorageDefinition& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::StorageDefinition)

namespace mqbconfm {

// ============
// class Domain
// ============

/// Configuration for a Domain using the custom Bloomberg Domain.
/// mode................: mode of the queues in this domain
/// storage.............: storage to use by queues in this domain
/// maxConsumers........: will reject if more than this number of consumers
/// tries to connect.  0 (the default) means unlimited maxProducers........:
/// will reject if more than this number of producers tries to connect.  0
/// (the default) means unlimited maxQueues...........: will reject creating
/// more than this number of queues.  0 (the default) means unlimited
/// msgGroupIdConfig....: optional configuration for Group Id routing
/// features maxIdleTime.........: (seconds) time queue can be inactive
/// before alarming.  0 (the default) means no monitoring and alarming
/// messageTtl..........: (seconds) minimum time before which a message can
/// be discarded (i.e., it's not guaranteed that the message will be
/// discarded exactly after `ttlSeconds` time, but it is guaranteed that it
/// will not be discarded before at least `ttlSeconds` time
/// maxDeliveryAttempts.: maximum number of times BMQ framework will attempt
/// to deliver a message to consumers before purging it from the queue.
/// Zero (the default) means unlimited deduplicationTimeMs.: timeout, in
/// milliseconds, to keep GUID of PUT message for the purpose of detecting
/// duplicate PUTs.  consistency.........: optional consistency mode.
class Domain {
    // INSTANCE DATA
    bsls::Types::Int64                    d_messageTtl;
    bsl::string                           d_name;
    bdlb::NullableValue<MsgGroupIdConfig> d_msgGroupIdConfig;
    StorageDefinition                     d_storage;
    QueueMode                             d_mode;
    Consistency                           d_consistency;
    int                                   d_maxConsumers;
    int                                   d_maxProducers;
    int                                   d_maxQueues;
    int                                   d_maxIdleTime;
    int                                   d_maxDeliveryAttempts;
    int                                   d_deduplicationTimeMs;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_NAME                  = 0,
        ATTRIBUTE_ID_MODE                  = 1,
        ATTRIBUTE_ID_STORAGE               = 2,
        ATTRIBUTE_ID_MAX_CONSUMERS         = 3,
        ATTRIBUTE_ID_MAX_PRODUCERS         = 4,
        ATTRIBUTE_ID_MAX_QUEUES            = 5,
        ATTRIBUTE_ID_MSG_GROUP_ID_CONFIG   = 6,
        ATTRIBUTE_ID_MAX_IDLE_TIME         = 7,
        ATTRIBUTE_ID_MESSAGE_TTL           = 8,
        ATTRIBUTE_ID_MAX_DELIVERY_ATTEMPTS = 9,
        ATTRIBUTE_ID_DEDUPLICATION_TIME_MS = 10,
        ATTRIBUTE_ID_CONSISTENCY           = 11
    };

    enum { NUM_ATTRIBUTES = 12 };

    enum {
        ATTRIBUTE_INDEX_NAME                  = 0,
        ATTRIBUTE_INDEX_MODE                  = 1,
        ATTRIBUTE_INDEX_STORAGE               = 2,
        ATTRIBUTE_INDEX_MAX_CONSUMERS         = 3,
        ATTRIBUTE_INDEX_MAX_PRODUCERS         = 4,
        ATTRIBUTE_INDEX_MAX_QUEUES            = 5,
        ATTRIBUTE_INDEX_MSG_GROUP_ID_CONFIG   = 6,
        ATTRIBUTE_INDEX_MAX_IDLE_TIME         = 7,
        ATTRIBUTE_INDEX_MESSAGE_TTL           = 8,
        ATTRIBUTE_INDEX_MAX_DELIVERY_ATTEMPTS = 9,
        ATTRIBUTE_INDEX_DEDUPLICATION_TIME_MS = 10,
        ATTRIBUTE_INDEX_CONSISTENCY           = 11
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_MAX_CONSUMERS;

    static const int DEFAULT_INITIALIZER_MAX_PRODUCERS;

    static const int DEFAULT_INITIALIZER_MAX_QUEUES;

    static const int DEFAULT_INITIALIZER_MAX_IDLE_TIME;

    static const int DEFAULT_INITIALIZER_MAX_DELIVERY_ATTEMPTS;

    static const int DEFAULT_INITIALIZER_DEDUPLICATION_TIME_MS;

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

    /// Create an object of type `Domain` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Domain(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Domain` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Domain(const Domain& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Domain` having the value of the specified
    /// `original` object.  After performing this action, the `original`
    /// object will be left in a valid, but unspecified state.
    Domain(Domain&& original) noexcept;

    /// Create an object of type `Domain` having the value of the specified
    /// `original` object.  After performing this action, the `original`
    /// object will be left in a valid, but unspecified state.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    Domain(Domain&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Domain();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Domain& operator=(const Domain& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    Domain& operator=(Domain&& rhs);
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

    /// Return a reference to the modifiable "Name" attribute of this
    /// object.
    bsl::string& name();

    /// Return a reference to the modifiable "Mode" attribute of this
    /// object.
    QueueMode& mode();

    /// Return a reference to the modifiable "Storage" attribute of this
    /// object.
    StorageDefinition& storage();

    /// Return a reference to the modifiable "MaxConsumers" attribute of
    /// this object.
    int& maxConsumers();

    /// Return a reference to the modifiable "MaxProducers" attribute of
    /// this object.
    int& maxProducers();

    /// Return a reference to the modifiable "MaxQueues" attribute of this
    /// object.
    int& maxQueues();

    /// Return a reference to the modifiable "MsgGroupIdConfig" attribute of
    /// this object.
    bdlb::NullableValue<MsgGroupIdConfig>& msgGroupIdConfig();

    /// Return a reference to the modifiable "MaxIdleTime" attribute of this
    /// object.
    int& maxIdleTime();

    /// Return a reference to the modifiable "MessageTtl" attribute of this
    /// object.
    bsls::Types::Int64& messageTtl();

    /// Return a reference to the modifiable "MaxDeliveryAttempts" attribute
    /// of this object.
    int& maxDeliveryAttempts();

    /// Return a reference to the modifiable "DeduplicationTimeMs" attribute
    /// of this object.
    int& deduplicationTimeMs();

    /// Return a reference to the modifiable "Consistency" attribute of this
    /// object.
    Consistency& consistency();

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

    /// Return a reference offering non-modifiable access to the "Mode"
    /// attribute of this object.
    const QueueMode& mode() const;

    /// Return a reference offering non-modifiable access to the "Storage"
    /// attribute of this object.
    const StorageDefinition& storage() const;

    /// Return the value of the "MaxConsumers" attribute of this object.
    int maxConsumers() const;

    /// Return the value of the "MaxProducers" attribute of this object.
    int maxProducers() const;

    /// Return the value of the "MaxQueues" attribute of this object.
    int maxQueues() const;

    /// Return a reference offering non-modifiable access to the
    /// "MsgGroupIdConfig" attribute of this object.
    const bdlb::NullableValue<MsgGroupIdConfig>& msgGroupIdConfig() const;

    /// Return the value of the "MaxIdleTime" attribute of this object.
    int maxIdleTime() const;

    /// Return the value of the "MessageTtl" attribute of this object.
    bsls::Types::Int64 messageTtl() const;

    /// Return the value of the "MaxDeliveryAttempts" attribute of this
    /// object.
    int maxDeliveryAttempts() const;

    /// Return the value of the "DeduplicationTimeMs" attribute of this
    /// object.
    int deduplicationTimeMs() const;

    /// Return a reference offering non-modifiable access to the
    /// "Consistency" attribute of this object.
    const Consistency& consistency() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const Domain& lhs, const Domain& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const Domain& lhs, const Domain& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream& stream, const Domain& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `Domain`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const Domain& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::Domain)

namespace mqbconfm {

// ======================
// class DomainDefinition
// ======================

/// Top level type representing the information retrieved when resolving a
/// domain.
/// location..: Domain location (i.e., cluster name)  REVIEW: consider:
/// s/location/cluster/ parameters: Domain parameters
/// REVIEW: consider merging Domain into DomainDefinition
class DomainDefinition {
    // INSTANCE DATA
    bsl::string d_location;
    Domain      d_parameters;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_LOCATION = 0, ATTRIBUTE_ID_PARAMETERS = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_LOCATION = 0, ATTRIBUTE_INDEX_PARAMETERS = 1 };

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

    /// Create an object of type `DomainDefinition` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    explicit DomainDefinition(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `DomainDefinition` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    DomainDefinition(const DomainDefinition& original,
                     bslma::Allocator*       basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `DomainDefinition` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    DomainDefinition(DomainDefinition&& original) noexcept;

    /// Create an object of type `DomainDefinition` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    DomainDefinition(DomainDefinition&& original,
                     bslma::Allocator*  basicAllocator);
#endif

    /// Destroy this object.
    ~DomainDefinition();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    DomainDefinition& operator=(const DomainDefinition& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    DomainDefinition& operator=(DomainDefinition&& rhs);
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

    /// Return a reference to the modifiable "Location" attribute of this
    /// object.
    bsl::string& location();

    /// Return a reference to the modifiable "Parameters" attribute of this
    /// object.
    Domain& parameters();

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

    /// Return a reference offering non-modifiable access to the "Location"
    /// attribute of this object.
    const bsl::string& location() const;

    /// Return a reference offering non-modifiable access to the
    /// "Parameters" attribute of this object.
    const Domain& parameters() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const DomainDefinition& lhs,
                       const DomainDefinition& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const DomainDefinition& lhs,
                       const DomainDefinition& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&           stream,
                                const DomainDefinition& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `DomainDefinition`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const DomainDefinition& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainDefinition)

namespace mqbconfm {

// ===================
// class DomainVariant
// ===================

/// Either a Domain or a DomainRedirection.
/// definition..: The full definition of a domain redirection.: The name of
/// the domain to redirect to
class DomainVariant {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<DomainDefinition> d_definition;
        bsls::ObjectBuffer<bsl::string>      d_redirect;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED  = -1,
        SELECTION_ID_DEFINITION = 0,
        SELECTION_ID_REDIRECT   = 1
    };

    enum { NUM_SELECTIONS = 2 };

    enum { SELECTION_INDEX_DEFINITION = 0, SELECTION_INDEX_REDIRECT = 1 };

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

    /// Create an object of type `DomainVariant` having the default value.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit DomainVariant(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `DomainVariant` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    DomainVariant(const DomainVariant& original,
                  bslma::Allocator*    basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `DomainVariant` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    DomainVariant(DomainVariant&& original) noexcept;

    /// Create an object of type `DomainVariant` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    DomainVariant(DomainVariant&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~DomainVariant();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    DomainVariant& operator=(const DomainVariant& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    DomainVariant& operator=(DomainVariant&& rhs);
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

    DomainDefinition& makeDefinition();
    DomainDefinition& makeDefinition(const DomainDefinition& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainDefinition& makeDefinition(DomainDefinition&& value);
#endif
    // Set the value of this object to be a "Definition" value.  Optionally
    // specify the 'value' of the "Definition".  If 'value' is not
    // specified, the default "Definition" value is used.

    bsl::string& makeRedirect();
    bsl::string& makeRedirect(const bsl::string& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    bsl::string& makeRedirect(bsl::string&& value);
#endif
    // Set the value of this object to be a "Redirect" value.  Optionally
    // specify the 'value' of the "Redirect".  If 'value' is not specified,
    // the default "Redirect" value is used.

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable "Definition" selection of this
    /// object if "Definition" is the current selection.  The behavior is
    /// undefined unless "Definition" is the selection of this object.
    DomainDefinition& definition();

    /// Return a reference to the modifiable "Redirect" selection of this
    /// object if "Redirect" is the current selection.  The behavior is
    /// undefined unless "Redirect" is the selection of this object.
    bsl::string& redirect();

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

    /// Return a reference to the non-modifiable "Definition" selection of
    /// this object if "Definition" is the current selection.  The behavior
    /// is undefined unless "Definition" is the selection of this object.
    const DomainDefinition& definition() const;

    /// Return a reference to the non-modifiable "Redirect" selection of
    /// this object if "Redirect" is the current selection.  The behavior is
    /// undefined unless "Redirect" is the selection of this object.
    const bsl::string& redirect() const;

    /// Return `true` if the value of this object is a "Definition" value,
    /// and return `false` otherwise.
    bool isDefinitionValue() const;

    /// Return `true` if the value of this object is a "Redirect" value, and
    /// return `false` otherwise.
    bool isRedirectValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` objects have the same
/// value, and `false` otherwise.  Two `DomainVariant` objects have the same
/// value if either the selections in both objects have the same ids and
/// the same values, or both selections are undefined.
inline bool operator==(const DomainVariant& lhs, const DomainVariant& rhs);

/// Return `true` if the specified `lhs` and `rhs` objects do not have the
/// same values, as determined by `operator==`, and `false` otherwise.
inline bool operator!=(const DomainVariant& lhs, const DomainVariant& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&        stream,
                                const DomainVariant& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `DomainVariant`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const DomainVariant& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainVariant)

// ============================================================================
//                         INLINE FUNCTION DEFINITIONS
// ============================================================================

namespace mqbconfm {

// --------------------
// class BrokerIdentity
// --------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int BrokerIdentity::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_hostName,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_hostTags,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_TAGS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_brokerVersion,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int BrokerIdentity::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_HOST_NAME: {
        return manipulator(&d_hostName,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME]);
    }
    case ATTRIBUTE_ID_HOST_TAGS: {
        return manipulator(&d_hostTags,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_TAGS]);
    }
    case ATTRIBUTE_ID_BROKER_VERSION: {
        return manipulator(
            &d_brokerVersion,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int BrokerIdentity::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& BrokerIdentity::hostName()
{
    return d_hostName;
}

inline bsl::string& BrokerIdentity::hostTags()
{
    return d_hostTags;
}

inline bsl::string& BrokerIdentity::brokerVersion()
{
    return d_brokerVersion;
}

// ACCESSORS
template <typename t_ACCESSOR>
int BrokerIdentity::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_hostName,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_hostTags,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_TAGS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_brokerVersion,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int BrokerIdentity::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_HOST_NAME: {
        return accessor(d_hostName,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME]);
    }
    case ATTRIBUTE_ID_HOST_TAGS: {
        return accessor(d_hostTags,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_TAGS]);
    }
    case ATTRIBUTE_ID_BROKER_VERSION: {
        return accessor(d_brokerVersion,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int BrokerIdentity::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& BrokerIdentity::hostName() const
{
    return d_hostName;
}

inline const bsl::string& BrokerIdentity::hostTags() const
{
    return d_hostTags;
}

inline const bsl::string& BrokerIdentity::brokerVersion() const
{
    return d_brokerVersion;
}

// ---------------------
// class DomainConfigRaw
// ---------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int DomainConfigRaw::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_domainName,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_config, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int DomainConfigRaw::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_DOMAIN_NAME: {
        return manipulator(&d_domainName,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME]);
    }
    case ATTRIBUTE_ID_CONFIG: {
        return manipulator(&d_config,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int DomainConfigRaw::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& DomainConfigRaw::domainName()
{
    return d_domainName;
}

inline bsl::string& DomainConfigRaw::config()
{
    return d_config;
}

// ACCESSORS
template <typename t_ACCESSOR>
int DomainConfigRaw::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_domainName,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_config, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int DomainConfigRaw::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_DOMAIN_NAME: {
        return accessor(d_domainName,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME]);
    }
    case ATTRIBUTE_ID_CONFIG: {
        return accessor(d_config,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int DomainConfigRaw::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& DomainConfigRaw::domainName() const
{
    return d_domainName;
}

inline const bsl::string& DomainConfigRaw::config() const
{
    return d_config;
}

// --------------------
// class DomainResolver
// --------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int DomainResolver::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_cluster,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int DomainResolver::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_CLUSTER: {
        return manipulator(&d_cluster,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int DomainResolver::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& DomainResolver::name()
{
    return d_name;
}

inline bsl::string& DomainResolver::cluster()
{
    return d_cluster;
}

// ACCESSORS
template <typename t_ACCESSOR>
int DomainResolver::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_cluster, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int DomainResolver::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_CLUSTER: {
        return accessor(d_cluster,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int DomainResolver::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& DomainResolver::name() const
{
    return d_name;
}

inline const bsl::string& DomainResolver::cluster() const
{
    return d_cluster;
}

// -------------
// class Failure
// -------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Failure::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_code, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CODE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_message,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Failure::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CODE: {
        return manipulator(&d_code,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CODE]);
    }
    case ATTRIBUTE_ID_MESSAGE: {
        return manipulator(&d_message,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Failure::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& Failure::code()
{
    return d_code;
}

inline bsl::string& Failure::message()
{
    return d_message;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Failure::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_code, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CODE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_message, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Failure::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CODE: {
        return accessor(d_code, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CODE]);
    }
    case ATTRIBUTE_ID_MESSAGE: {
        return accessor(d_message,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Failure::accessAttribute(t_ACCESSOR& accessor,
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

inline int Failure::code() const
{
    return d_code;
}

inline const bsl::string& Failure::message() const
{
    return d_message;
}

// -----------------------
// class FileBackedStorage
// -----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int FileBackedStorage::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int FileBackedStorage::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int FileBackedStorage::manipulateAttribute(t_MANIPULATOR& manipulator,
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
int FileBackedStorage::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int FileBackedStorage::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int FileBackedStorage::accessAttribute(t_ACCESSOR& accessor,
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

// ---------------------
// class InMemoryStorage
// ---------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int InMemoryStorage::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int InMemoryStorage::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int InMemoryStorage::manipulateAttribute(t_MANIPULATOR& manipulator,
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
int InMemoryStorage::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int InMemoryStorage::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int InMemoryStorage::accessAttribute(t_ACCESSOR& accessor,
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

// ------------
// class Limits
// ------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Limits::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_messages,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_messagesWatermarkRatio,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES_WATERMARK_RATIO]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_bytes, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_bytesWatermarkRatio,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES_WATERMARK_RATIO]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Limits::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MESSAGES: {
        return manipulator(&d_messages,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES]);
    }
    case ATTRIBUTE_ID_MESSAGES_WATERMARK_RATIO: {
        return manipulator(
            &d_messagesWatermarkRatio,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES_WATERMARK_RATIO]);
    }
    case ATTRIBUTE_ID_BYTES: {
        return manipulator(&d_bytes,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES]);
    }
    case ATTRIBUTE_ID_BYTES_WATERMARK_RATIO: {
        return manipulator(
            &d_bytesWatermarkRatio,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES_WATERMARK_RATIO]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Limits::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsls::Types::Int64& Limits::messages()
{
    return d_messages;
}

inline double& Limits::messagesWatermarkRatio()
{
    return d_messagesWatermarkRatio;
}

inline bsls::Types::Int64& Limits::bytes()
{
    return d_bytes;
}

inline double& Limits::bytesWatermarkRatio()
{
    return d_bytesWatermarkRatio;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Limits::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_messages, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_messagesWatermarkRatio,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES_WATERMARK_RATIO]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_bytes, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_bytesWatermarkRatio,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES_WATERMARK_RATIO]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Limits::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MESSAGES: {
        return accessor(d_messages,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES]);
    }
    case ATTRIBUTE_ID_MESSAGES_WATERMARK_RATIO: {
        return accessor(
            d_messagesWatermarkRatio,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES_WATERMARK_RATIO]);
    }
    case ATTRIBUTE_ID_BYTES: {
        return accessor(d_bytes, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES]);
    }
    case ATTRIBUTE_ID_BYTES_WATERMARK_RATIO: {
        return accessor(
            d_bytesWatermarkRatio,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES_WATERMARK_RATIO]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Limits::accessAttribute(t_ACCESSOR& accessor,
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

inline bsls::Types::Int64 Limits::messages() const
{
    return d_messages;
}

inline double Limits::messagesWatermarkRatio() const
{
    return d_messagesWatermarkRatio;
}

inline bsls::Types::Int64 Limits::bytes() const
{
    return d_bytes;
}

inline double Limits::bytesWatermarkRatio() const
{
    return d_bytesWatermarkRatio;
}

// ----------------------
// class MsgGroupIdConfig
// ----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int MsgGroupIdConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_rebalance,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REBALANCE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxGroups,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_GROUPS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_ttlSeconds,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TTL_SECONDS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int MsgGroupIdConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_REBALANCE: {
        return manipulator(&d_rebalance,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REBALANCE]);
    }
    case ATTRIBUTE_ID_MAX_GROUPS: {
        return manipulator(&d_maxGroups,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_GROUPS]);
    }
    case ATTRIBUTE_ID_TTL_SECONDS: {
        return manipulator(&d_ttlSeconds,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TTL_SECONDS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int MsgGroupIdConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bool& MsgGroupIdConfig::rebalance()
{
    return d_rebalance;
}

inline int& MsgGroupIdConfig::maxGroups()
{
    return d_maxGroups;
}

inline bsls::Types::Int64& MsgGroupIdConfig::ttlSeconds()
{
    return d_ttlSeconds;
}

// ACCESSORS
template <typename t_ACCESSOR>
int MsgGroupIdConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_rebalance,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REBALANCE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxGroups,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_GROUPS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_ttlSeconds,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TTL_SECONDS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int MsgGroupIdConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_REBALANCE: {
        return accessor(d_rebalance,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REBALANCE]);
    }
    case ATTRIBUTE_ID_MAX_GROUPS: {
        return accessor(d_maxGroups,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_GROUPS]);
    }
    case ATTRIBUTE_ID_TTL_SECONDS: {
        return accessor(d_ttlSeconds,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TTL_SECONDS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int MsgGroupIdConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline bool MsgGroupIdConfig::rebalance() const
{
    return d_rebalance;
}

inline int MsgGroupIdConfig::maxGroups() const
{
    return d_maxGroups;
}

inline bsls::Types::Int64 MsgGroupIdConfig::ttlSeconds() const
{
    return d_ttlSeconds;
}

// ------------------------------
// class QueueConsistencyEventual
// ------------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int QueueConsistencyEventual::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int QueueConsistencyEventual::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                  int            id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int QueueConsistencyEventual::manipulateAttribute(t_MANIPULATOR& manipulator,
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
int QueueConsistencyEventual::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int QueueConsistencyEventual::accessAttribute(t_ACCESSOR& accessor,
                                              int         id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int QueueConsistencyEventual::accessAttribute(t_ACCESSOR& accessor,
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

// ----------------------------
// class QueueConsistencyStrong
// ----------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int QueueConsistencyStrong::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int QueueConsistencyStrong::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                int            id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int QueueConsistencyStrong::manipulateAttribute(t_MANIPULATOR& manipulator,
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
int QueueConsistencyStrong::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int QueueConsistencyStrong::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int QueueConsistencyStrong::accessAttribute(t_ACCESSOR& accessor,
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
// class QueueModeBroadcast
// ------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int QueueModeBroadcast::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int QueueModeBroadcast::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int QueueModeBroadcast::manipulateAttribute(t_MANIPULATOR& manipulator,
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
int QueueModeBroadcast::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int QueueModeBroadcast::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int QueueModeBroadcast::accessAttribute(t_ACCESSOR& accessor,
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

// ---------------------
// class QueueModeFanout
// ---------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int QueueModeFanout::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_appIDs,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_I_DS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int QueueModeFanout::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_APP_I_DS: {
        return manipulator(&d_appIDs,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_I_DS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int QueueModeFanout::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::vector<bsl::string>& QueueModeFanout::appIDs()
{
    return d_appIDs;
}

// ACCESSORS
template <typename t_ACCESSOR>
int QueueModeFanout::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_appIDs, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_I_DS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int QueueModeFanout::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_APP_I_DS: {
        return accessor(d_appIDs,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_I_DS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int QueueModeFanout::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::vector<bsl::string>& QueueModeFanout::appIDs() const
{
    return d_appIDs;
}

// -----------------------
// class QueueModePriority
// -----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int QueueModePriority::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int QueueModePriority::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int QueueModePriority::manipulateAttribute(t_MANIPULATOR& manipulator,
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
int QueueModePriority::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int QueueModePriority::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int QueueModePriority::accessAttribute(t_ACCESSOR& accessor,
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

// -----------------
// class Consistency
// -----------------

// CLASS METHODS
// CREATORS
inline Consistency::Consistency()
: d_selectionId(SELECTION_ID_UNDEFINED)
{
}

inline Consistency::~Consistency()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int Consistency::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case Consistency::SELECTION_ID_EVENTUAL:
        return manipulator(&d_eventual.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_EVENTUAL]);
    case Consistency::SELECTION_ID_STRONG:
        return manipulator(&d_strong.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_STRONG]);
    default:
        BSLS_ASSERT(Consistency::SELECTION_ID_UNDEFINED == d_selectionId);
        return -1;
    }
}

inline QueueConsistencyEventual& Consistency::eventual()
{
    BSLS_ASSERT(SELECTION_ID_EVENTUAL == d_selectionId);
    return d_eventual.object();
}

inline QueueConsistencyStrong& Consistency::strong()
{
    BSLS_ASSERT(SELECTION_ID_STRONG == d_selectionId);
    return d_strong.object();
}

// ACCESSORS
inline int Consistency::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int Consistency::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_EVENTUAL:
        return accessor(d_eventual.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_EVENTUAL]);
    case SELECTION_ID_STRONG:
        return accessor(d_strong.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_STRONG]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const QueueConsistencyEventual& Consistency::eventual() const
{
    BSLS_ASSERT(SELECTION_ID_EVENTUAL == d_selectionId);
    return d_eventual.object();
}

inline const QueueConsistencyStrong& Consistency::strong() const
{
    BSLS_ASSERT(SELECTION_ID_STRONG == d_selectionId);
    return d_strong.object();
}

inline bool Consistency::isEventualValue() const
{
    return SELECTION_ID_EVENTUAL == d_selectionId;
}

inline bool Consistency::isStrongValue() const
{
    return SELECTION_ID_STRONG == d_selectionId;
}

inline bool Consistency::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// -------------------------
// class DomainConfigRequest
// -------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int DomainConfigRequest::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_brokerIdentity,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_IDENTITY]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_domainName,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int DomainConfigRequest::manipulateAttribute(t_MANIPULATOR& manipulator,
                                             int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_BROKER_IDENTITY: {
        return manipulator(
            &d_brokerIdentity,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_IDENTITY]);
    }
    case ATTRIBUTE_ID_DOMAIN_NAME: {
        return manipulator(&d_domainName,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int DomainConfigRequest::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline BrokerIdentity& DomainConfigRequest::brokerIdentity()
{
    return d_brokerIdentity;
}

inline bsl::string& DomainConfigRequest::domainName()
{
    return d_domainName;
}

// ACCESSORS
template <typename t_ACCESSOR>
int DomainConfigRequest::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_brokerIdentity,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_IDENTITY]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_domainName,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int DomainConfigRequest::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_BROKER_IDENTITY: {
        return accessor(d_brokerIdentity,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_IDENTITY]);
    }
    case ATTRIBUTE_ID_DOMAIN_NAME: {
        return accessor(d_domainName,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int DomainConfigRequest::accessAttribute(t_ACCESSOR& accessor,
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

inline const BrokerIdentity& DomainConfigRequest::brokerIdentity() const
{
    return d_brokerIdentity;
}

inline const bsl::string& DomainConfigRequest::domainName() const
{
    return d_domainName;
}

// ---------------
// class QueueMode
// ---------------

// CLASS METHODS
// CREATORS
inline QueueMode::QueueMode(bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline QueueMode::~QueueMode()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int QueueMode::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case QueueMode::SELECTION_ID_FANOUT:
        return manipulator(&d_fanout.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_FANOUT]);
    case QueueMode::SELECTION_ID_PRIORITY:
        return manipulator(&d_priority.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIORITY]);
    case QueueMode::SELECTION_ID_BROADCAST:
        return manipulator(&d_broadcast.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_BROADCAST]);
    default:
        BSLS_ASSERT(QueueMode::SELECTION_ID_UNDEFINED == d_selectionId);
        return -1;
    }
}

inline QueueModeFanout& QueueMode::fanout()
{
    BSLS_ASSERT(SELECTION_ID_FANOUT == d_selectionId);
    return d_fanout.object();
}

inline QueueModePriority& QueueMode::priority()
{
    BSLS_ASSERT(SELECTION_ID_PRIORITY == d_selectionId);
    return d_priority.object();
}

inline QueueModeBroadcast& QueueMode::broadcast()
{
    BSLS_ASSERT(SELECTION_ID_BROADCAST == d_selectionId);
    return d_broadcast.object();
}

// ACCESSORS
inline int QueueMode::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int QueueMode::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT:
        return accessor(d_fanout.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_FANOUT]);
    case SELECTION_ID_PRIORITY:
        return accessor(d_priority.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIORITY]);
    case SELECTION_ID_BROADCAST:
        return accessor(d_broadcast.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_BROADCAST]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const QueueModeFanout& QueueMode::fanout() const
{
    BSLS_ASSERT(SELECTION_ID_FANOUT == d_selectionId);
    return d_fanout.object();
}

inline const QueueModePriority& QueueMode::priority() const
{
    BSLS_ASSERT(SELECTION_ID_PRIORITY == d_selectionId);
    return d_priority.object();
}

inline const QueueModeBroadcast& QueueMode::broadcast() const
{
    BSLS_ASSERT(SELECTION_ID_BROADCAST == d_selectionId);
    return d_broadcast.object();
}

inline bool QueueMode::isFanoutValue() const
{
    return SELECTION_ID_FANOUT == d_selectionId;
}

inline bool QueueMode::isPriorityValue() const
{
    return SELECTION_ID_PRIORITY == d_selectionId;
}

inline bool QueueMode::isBroadcastValue() const
{
    return SELECTION_ID_BROADCAST == d_selectionId;
}

inline bool QueueMode::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// --------------
// class Response
// --------------

// CLASS METHODS
// CREATORS
inline Response::Response(bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline Response::~Response()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int Response::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case Response::SELECTION_ID_FAILURE:
        return manipulator(&d_failure.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_FAILURE]);
    case Response::SELECTION_ID_DOMAIN_CONFIG:
        return manipulator(
            &d_domainConfig.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_CONFIG]);
    default:
        BSLS_ASSERT(Response::SELECTION_ID_UNDEFINED == d_selectionId);
        return -1;
    }
}

inline Failure& Response::failure()
{
    BSLS_ASSERT(SELECTION_ID_FAILURE == d_selectionId);
    return d_failure.object();
}

inline DomainConfigRaw& Response::domainConfig()
{
    BSLS_ASSERT(SELECTION_ID_DOMAIN_CONFIG == d_selectionId);
    return d_domainConfig.object();
}

// ACCESSORS
inline int Response::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int Response::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_FAILURE:
        return accessor(d_failure.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_FAILURE]);
    case SELECTION_ID_DOMAIN_CONFIG:
        return accessor(d_domainConfig.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_CONFIG]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const Failure& Response::failure() const
{
    BSLS_ASSERT(SELECTION_ID_FAILURE == d_selectionId);
    return d_failure.object();
}

inline const DomainConfigRaw& Response::domainConfig() const
{
    BSLS_ASSERT(SELECTION_ID_DOMAIN_CONFIG == d_selectionId);
    return d_domainConfig.object();
}

inline bool Response::isFailureValue() const
{
    return SELECTION_ID_FAILURE == d_selectionId;
}

inline bool Response::isDomainConfigValue() const
{
    return SELECTION_ID_DOMAIN_CONFIG == d_selectionId;
}

inline bool Response::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// -------------
// class Storage
// -------------

// CLASS METHODS
// CREATORS
inline Storage::Storage()
: d_selectionId(SELECTION_ID_UNDEFINED)
{
}

inline Storage::~Storage()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int Storage::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case Storage::SELECTION_ID_IN_MEMORY:
        return manipulator(&d_inMemory.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_IN_MEMORY]);
    case Storage::SELECTION_ID_FILE_BACKED:
        return manipulator(&d_fileBacked.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_FILE_BACKED]);
    default:
        BSLS_ASSERT(Storage::SELECTION_ID_UNDEFINED == d_selectionId);
        return -1;
    }
}

inline InMemoryStorage& Storage::inMemory()
{
    BSLS_ASSERT(SELECTION_ID_IN_MEMORY == d_selectionId);
    return d_inMemory.object();
}

inline FileBackedStorage& Storage::fileBacked()
{
    BSLS_ASSERT(SELECTION_ID_FILE_BACKED == d_selectionId);
    return d_fileBacked.object();
}

// ACCESSORS
inline int Storage::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int Storage::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_IN_MEMORY:
        return accessor(d_inMemory.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_IN_MEMORY]);
    case SELECTION_ID_FILE_BACKED:
        return accessor(d_fileBacked.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_FILE_BACKED]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const InMemoryStorage& Storage::inMemory() const
{
    BSLS_ASSERT(SELECTION_ID_IN_MEMORY == d_selectionId);
    return d_inMemory.object();
}

inline const FileBackedStorage& Storage::fileBacked() const
{
    BSLS_ASSERT(SELECTION_ID_FILE_BACKED == d_selectionId);
    return d_fileBacked.object();
}

inline bool Storage::isInMemoryValue() const
{
    return SELECTION_ID_IN_MEMORY == d_selectionId;
}

inline bool Storage::isFileBackedValue() const
{
    return SELECTION_ID_FILE_BACKED == d_selectionId;
}

inline bool Storage::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// -------------
// class Request
// -------------

// CLASS METHODS
// CREATORS
inline Request::Request(bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline Request::~Request()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int Request::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case Request::SELECTION_ID_DOMAIN_CONFIG:
        return manipulator(
            &d_domainConfig.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_CONFIG]);
    default:
        BSLS_ASSERT(Request::SELECTION_ID_UNDEFINED == d_selectionId);
        return -1;
    }
}

inline DomainConfigRequest& Request::domainConfig()
{
    BSLS_ASSERT(SELECTION_ID_DOMAIN_CONFIG == d_selectionId);
    return d_domainConfig.object();
}

// ACCESSORS
inline int Request::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int Request::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN_CONFIG:
        return accessor(d_domainConfig.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_CONFIG]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const DomainConfigRequest& Request::domainConfig() const
{
    BSLS_ASSERT(SELECTION_ID_DOMAIN_CONFIG == d_selectionId);
    return d_domainConfig.object();
}

inline bool Request::isDomainConfigValue() const
{
    return SELECTION_ID_DOMAIN_CONFIG == d_selectionId;
}

inline bool Request::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// -----------------------
// class StorageDefinition
// -----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StorageDefinition::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_domainLimits,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_LIMITS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_queueLimits,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LIMITS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_config, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StorageDefinition::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_DOMAIN_LIMITS: {
        return manipulator(
            &d_domainLimits,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_LIMITS]);
    }
    case ATTRIBUTE_ID_QUEUE_LIMITS: {
        return manipulator(&d_queueLimits,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LIMITS]);
    }
    case ATTRIBUTE_ID_CONFIG: {
        return manipulator(&d_config,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StorageDefinition::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline Limits& StorageDefinition::domainLimits()
{
    return d_domainLimits;
}

inline Limits& StorageDefinition::queueLimits()
{
    return d_queueLimits;
}

inline Storage& StorageDefinition::config()
{
    return d_config;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StorageDefinition::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_domainLimits,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_LIMITS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queueLimits,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LIMITS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_config, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StorageDefinition::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_DOMAIN_LIMITS: {
        return accessor(d_domainLimits,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_LIMITS]);
    }
    case ATTRIBUTE_ID_QUEUE_LIMITS: {
        return accessor(d_queueLimits,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LIMITS]);
    }
    case ATTRIBUTE_ID_CONFIG: {
        return accessor(d_config,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StorageDefinition::accessAttribute(t_ACCESSOR& accessor,
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

inline const Limits& StorageDefinition::domainLimits() const
{
    return d_domainLimits;
}

inline const Limits& StorageDefinition::queueLimits() const
{
    return d_queueLimits;
}

inline const Storage& StorageDefinition::config() const
{
    return d_config;
}

// ------------
// class Domain
// ------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Domain::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_mode, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_storage,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxConsumers,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONSUMERS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxProducers,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_PRODUCERS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxQueues,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QUEUES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_msgGroupIdConfig,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_GROUP_ID_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxIdleTime,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_IDLE_TIME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_messageTtl,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_TTL]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxDeliveryAttempts,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DELIVERY_ATTEMPTS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_deduplicationTimeMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DEDUPLICATION_TIME_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_consistency,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSISTENCY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Domain::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_MODE: {
        return manipulator(&d_mode,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    }
    case ATTRIBUTE_ID_STORAGE: {
        return manipulator(&d_storage,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE]);
    }
    case ATTRIBUTE_ID_MAX_CONSUMERS: {
        return manipulator(
            &d_maxConsumers,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONSUMERS]);
    }
    case ATTRIBUTE_ID_MAX_PRODUCERS: {
        return manipulator(
            &d_maxProducers,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_PRODUCERS]);
    }
    case ATTRIBUTE_ID_MAX_QUEUES: {
        return manipulator(&d_maxQueues,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QUEUES]);
    }
    case ATTRIBUTE_ID_MSG_GROUP_ID_CONFIG: {
        return manipulator(
            &d_msgGroupIdConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_GROUP_ID_CONFIG]);
    }
    case ATTRIBUTE_ID_MAX_IDLE_TIME: {
        return manipulator(
            &d_maxIdleTime,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_IDLE_TIME]);
    }
    case ATTRIBUTE_ID_MESSAGE_TTL: {
        return manipulator(&d_messageTtl,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_TTL]);
    }
    case ATTRIBUTE_ID_MAX_DELIVERY_ATTEMPTS: {
        return manipulator(
            &d_maxDeliveryAttempts,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DELIVERY_ATTEMPTS]);
    }
    case ATTRIBUTE_ID_DEDUPLICATION_TIME_MS: {
        return manipulator(
            &d_deduplicationTimeMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DEDUPLICATION_TIME_MS]);
    }
    case ATTRIBUTE_ID_CONSISTENCY: {
        return manipulator(&d_consistency,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSISTENCY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Domain::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& Domain::name()
{
    return d_name;
}

inline QueueMode& Domain::mode()
{
    return d_mode;
}

inline StorageDefinition& Domain::storage()
{
    return d_storage;
}

inline int& Domain::maxConsumers()
{
    return d_maxConsumers;
}

inline int& Domain::maxProducers()
{
    return d_maxProducers;
}

inline int& Domain::maxQueues()
{
    return d_maxQueues;
}

inline bdlb::NullableValue<MsgGroupIdConfig>& Domain::msgGroupIdConfig()
{
    return d_msgGroupIdConfig;
}

inline int& Domain::maxIdleTime()
{
    return d_maxIdleTime;
}

inline bsls::Types::Int64& Domain::messageTtl()
{
    return d_messageTtl;
}

inline int& Domain::maxDeliveryAttempts()
{
    return d_maxDeliveryAttempts;
}

inline int& Domain::deduplicationTimeMs()
{
    return d_deduplicationTimeMs;
}

inline Consistency& Domain::consistency()
{
    return d_consistency;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Domain::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_mode, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_storage, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxConsumers,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONSUMERS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxProducers,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_PRODUCERS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxQueues,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QUEUES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_msgGroupIdConfig,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_GROUP_ID_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxIdleTime,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_IDLE_TIME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_messageTtl,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_TTL]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxDeliveryAttempts,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DELIVERY_ATTEMPTS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_deduplicationTimeMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DEDUPLICATION_TIME_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_consistency,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSISTENCY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Domain::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_MODE: {
        return accessor(d_mode, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    }
    case ATTRIBUTE_ID_STORAGE: {
        return accessor(d_storage,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE]);
    }
    case ATTRIBUTE_ID_MAX_CONSUMERS: {
        return accessor(d_maxConsumers,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONSUMERS]);
    }
    case ATTRIBUTE_ID_MAX_PRODUCERS: {
        return accessor(d_maxProducers,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_PRODUCERS]);
    }
    case ATTRIBUTE_ID_MAX_QUEUES: {
        return accessor(d_maxQueues,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QUEUES]);
    }
    case ATTRIBUTE_ID_MSG_GROUP_ID_CONFIG: {
        return accessor(
            d_msgGroupIdConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_GROUP_ID_CONFIG]);
    }
    case ATTRIBUTE_ID_MAX_IDLE_TIME: {
        return accessor(d_maxIdleTime,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_IDLE_TIME]);
    }
    case ATTRIBUTE_ID_MESSAGE_TTL: {
        return accessor(d_messageTtl,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_TTL]);
    }
    case ATTRIBUTE_ID_MAX_DELIVERY_ATTEMPTS: {
        return accessor(
            d_maxDeliveryAttempts,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DELIVERY_ATTEMPTS]);
    }
    case ATTRIBUTE_ID_DEDUPLICATION_TIME_MS: {
        return accessor(
            d_deduplicationTimeMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DEDUPLICATION_TIME_MS]);
    }
    case ATTRIBUTE_ID_CONSISTENCY: {
        return accessor(d_consistency,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSISTENCY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Domain::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& Domain::name() const
{
    return d_name;
}

inline const QueueMode& Domain::mode() const
{
    return d_mode;
}

inline const StorageDefinition& Domain::storage() const
{
    return d_storage;
}

inline int Domain::maxConsumers() const
{
    return d_maxConsumers;
}

inline int Domain::maxProducers() const
{
    return d_maxProducers;
}

inline int Domain::maxQueues() const
{
    return d_maxQueues;
}

inline const bdlb::NullableValue<MsgGroupIdConfig>&
Domain::msgGroupIdConfig() const
{
    return d_msgGroupIdConfig;
}

inline int Domain::maxIdleTime() const
{
    return d_maxIdleTime;
}

inline bsls::Types::Int64 Domain::messageTtl() const
{
    return d_messageTtl;
}

inline int Domain::maxDeliveryAttempts() const
{
    return d_maxDeliveryAttempts;
}

inline int Domain::deduplicationTimeMs() const
{
    return d_deduplicationTimeMs;
}

inline const Consistency& Domain::consistency() const
{
    return d_consistency;
}

// ----------------------
// class DomainDefinition
// ----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int DomainDefinition::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_location,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_parameters,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARAMETERS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int DomainDefinition::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_LOCATION: {
        return manipulator(&d_location,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION]);
    }
    case ATTRIBUTE_ID_PARAMETERS: {
        return manipulator(&d_parameters,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARAMETERS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int DomainDefinition::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& DomainDefinition::location()
{
    return d_location;
}

inline Domain& DomainDefinition::parameters()
{
    return d_parameters;
}

// ACCESSORS
template <typename t_ACCESSOR>
int DomainDefinition::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_location, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_parameters,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARAMETERS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int DomainDefinition::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_LOCATION: {
        return accessor(d_location,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION]);
    }
    case ATTRIBUTE_ID_PARAMETERS: {
        return accessor(d_parameters,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARAMETERS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int DomainDefinition::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& DomainDefinition::location() const
{
    return d_location;
}

inline const Domain& DomainDefinition::parameters() const
{
    return d_parameters;
}

// -------------------
// class DomainVariant
// -------------------

// CLASS METHODS
// CREATORS
inline DomainVariant::DomainVariant(bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline DomainVariant::~DomainVariant()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int DomainVariant::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case DomainVariant::SELECTION_ID_DEFINITION:
        return manipulator(&d_definition.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_DEFINITION]);
    case DomainVariant::SELECTION_ID_REDIRECT:
        return manipulator(&d_redirect.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_REDIRECT]);
    default:
        BSLS_ASSERT(DomainVariant::SELECTION_ID_UNDEFINED == d_selectionId);
        return -1;
    }
}

inline DomainDefinition& DomainVariant::definition()
{
    BSLS_ASSERT(SELECTION_ID_DEFINITION == d_selectionId);
    return d_definition.object();
}

inline bsl::string& DomainVariant::redirect()
{
    BSLS_ASSERT(SELECTION_ID_REDIRECT == d_selectionId);
    return d_redirect.object();
}

// ACCESSORS
inline int DomainVariant::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int DomainVariant::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_DEFINITION:
        return accessor(d_definition.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_DEFINITION]);
    case SELECTION_ID_REDIRECT:
        return accessor(d_redirect.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_REDIRECT]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const DomainDefinition& DomainVariant::definition() const
{
    BSLS_ASSERT(SELECTION_ID_DEFINITION == d_selectionId);
    return d_definition.object();
}

inline const bsl::string& DomainVariant::redirect() const
{
    BSLS_ASSERT(SELECTION_ID_REDIRECT == d_selectionId);
    return d_redirect.object();
}

inline bool DomainVariant::isDefinitionValue() const
{
    return SELECTION_ID_DEFINITION == d_selectionId;
}

inline bool DomainVariant::isRedirectValue() const
{
    return SELECTION_ID_REDIRECT == d_selectionId;
}

inline bool DomainVariant::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}
}  // close package namespace

// FREE FUNCTIONS

inline bool mqbconfm::operator==(const mqbconfm::BrokerIdentity& lhs,
                                 const mqbconfm::BrokerIdentity& rhs)
{
    return lhs.hostName() == rhs.hostName() &&
           lhs.hostTags() == rhs.hostTags() &&
           lhs.brokerVersion() == rhs.brokerVersion();
}

inline bool mqbconfm::operator!=(const mqbconfm::BrokerIdentity& lhs,
                                 const mqbconfm::BrokerIdentity& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream& stream,
                                          const mqbconfm::BrokerIdentity& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&               hashAlg,
                          const mqbconfm::BrokerIdentity& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.hostName());
    hashAppend(hashAlg, object.hostTags());
    hashAppend(hashAlg, object.brokerVersion());
}

inline bool mqbconfm::operator==(const mqbconfm::DomainConfigRaw& lhs,
                                 const mqbconfm::DomainConfigRaw& rhs)
{
    return lhs.domainName() == rhs.domainName() &&
           lhs.config() == rhs.config();
}

inline bool mqbconfm::operator!=(const mqbconfm::DomainConfigRaw& lhs,
                                 const mqbconfm::DomainConfigRaw& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream& stream,
                                          const mqbconfm::DomainConfigRaw& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                hashAlg,
                          const mqbconfm::DomainConfigRaw& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.domainName());
    hashAppend(hashAlg, object.config());
}

inline bool mqbconfm::operator==(const mqbconfm::DomainResolver& lhs,
                                 const mqbconfm::DomainResolver& rhs)
{
    return lhs.name() == rhs.name() && lhs.cluster() == rhs.cluster();
}

inline bool mqbconfm::operator!=(const mqbconfm::DomainResolver& lhs,
                                 const mqbconfm::DomainResolver& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream& stream,
                                          const mqbconfm::DomainResolver& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&               hashAlg,
                          const mqbconfm::DomainResolver& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.name());
    hashAppend(hashAlg, object.cluster());
}

inline bool mqbconfm::operator==(const mqbconfm::Failure& lhs,
                                 const mqbconfm::Failure& rhs)
{
    return lhs.code() == rhs.code() && lhs.message() == rhs.message();
}

inline bool mqbconfm::operator!=(const mqbconfm::Failure& lhs,
                                 const mqbconfm::Failure& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream&            stream,
                                          const mqbconfm::Failure& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&        hashAlg,
                          const mqbconfm::Failure& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.code());
    hashAppend(hashAlg, object.message());
}

inline bool mqbconfm::operator==(const mqbconfm::FileBackedStorage&,
                                 const mqbconfm::FileBackedStorage&)
{
    return true;
}

inline bool mqbconfm::operator!=(const mqbconfm::FileBackedStorage&,
                                 const mqbconfm::FileBackedStorage&)
{
    return false;
}

inline bsl::ostream&
mqbconfm::operator<<(bsl::ostream&                      stream,
                     const mqbconfm::FileBackedStorage& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                  hashAlg,
                          const mqbconfm::FileBackedStorage& object)
{
    (void)hashAlg;
    (void)object;
}

inline bool mqbconfm::operator==(const mqbconfm::InMemoryStorage&,
                                 const mqbconfm::InMemoryStorage&)
{
    return true;
}

inline bool mqbconfm::operator!=(const mqbconfm::InMemoryStorage&,
                                 const mqbconfm::InMemoryStorage&)
{
    return false;
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream& stream,
                                          const mqbconfm::InMemoryStorage& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                hashAlg,
                          const mqbconfm::InMemoryStorage& object)
{
    (void)hashAlg;
    (void)object;
}

inline bool mqbconfm::operator==(const mqbconfm::Limits& lhs,
                                 const mqbconfm::Limits& rhs)
{
    return lhs.messages() == rhs.messages() &&
           lhs.messagesWatermarkRatio() == rhs.messagesWatermarkRatio() &&
           lhs.bytes() == rhs.bytes() &&
           lhs.bytesWatermarkRatio() == rhs.bytesWatermarkRatio();
}

inline bool mqbconfm::operator!=(const mqbconfm::Limits& lhs,
                                 const mqbconfm::Limits& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream&           stream,
                                          const mqbconfm::Limits& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&       hashAlg,
                          const mqbconfm::Limits& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.messages());
    hashAppend(hashAlg, object.messagesWatermarkRatio());
    hashAppend(hashAlg, object.bytes());
    hashAppend(hashAlg, object.bytesWatermarkRatio());
}

inline bool mqbconfm::operator==(const mqbconfm::MsgGroupIdConfig& lhs,
                                 const mqbconfm::MsgGroupIdConfig& rhs)
{
    return lhs.rebalance() == rhs.rebalance() &&
           lhs.maxGroups() == rhs.maxGroups() &&
           lhs.ttlSeconds() == rhs.ttlSeconds();
}

inline bool mqbconfm::operator!=(const mqbconfm::MsgGroupIdConfig& lhs,
                                 const mqbconfm::MsgGroupIdConfig& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream&
mqbconfm::operator<<(bsl::ostream&                     stream,
                     const mqbconfm::MsgGroupIdConfig& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                 hashAlg,
                          const mqbconfm::MsgGroupIdConfig& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.rebalance());
    hashAppend(hashAlg, object.maxGroups());
    hashAppend(hashAlg, object.ttlSeconds());
}

inline bool mqbconfm::operator==(const mqbconfm::QueueConsistencyEventual&,
                                 const mqbconfm::QueueConsistencyEventual&)
{
    return true;
}

inline bool mqbconfm::operator!=(const mqbconfm::QueueConsistencyEventual&,
                                 const mqbconfm::QueueConsistencyEventual&)
{
    return false;
}

inline bsl::ostream&
mqbconfm::operator<<(bsl::ostream&                             stream,
                     const mqbconfm::QueueConsistencyEventual& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                         hashAlg,
                          const mqbconfm::QueueConsistencyEventual& object)
{
    (void)hashAlg;
    (void)object;
}

inline bool mqbconfm::operator==(const mqbconfm::QueueConsistencyStrong&,
                                 const mqbconfm::QueueConsistencyStrong&)
{
    return true;
}

inline bool mqbconfm::operator!=(const mqbconfm::QueueConsistencyStrong&,
                                 const mqbconfm::QueueConsistencyStrong&)
{
    return false;
}

inline bsl::ostream&
mqbconfm::operator<<(bsl::ostream&                           stream,
                     const mqbconfm::QueueConsistencyStrong& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                       hashAlg,
                          const mqbconfm::QueueConsistencyStrong& object)
{
    (void)hashAlg;
    (void)object;
}

inline bool mqbconfm::operator==(const mqbconfm::QueueModeBroadcast&,
                                 const mqbconfm::QueueModeBroadcast&)
{
    return true;
}

inline bool mqbconfm::operator!=(const mqbconfm::QueueModeBroadcast&,
                                 const mqbconfm::QueueModeBroadcast&)
{
    return false;
}

inline bsl::ostream&
mqbconfm::operator<<(bsl::ostream&                       stream,
                     const mqbconfm::QueueModeBroadcast& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                   hashAlg,
                          const mqbconfm::QueueModeBroadcast& object)
{
    (void)hashAlg;
    (void)object;
}

inline bool mqbconfm::operator==(const mqbconfm::QueueModeFanout& lhs,
                                 const mqbconfm::QueueModeFanout& rhs)
{
    return lhs.appIDs() == rhs.appIDs();
}

inline bool mqbconfm::operator!=(const mqbconfm::QueueModeFanout& lhs,
                                 const mqbconfm::QueueModeFanout& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream& stream,
                                          const mqbconfm::QueueModeFanout& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                hashAlg,
                          const mqbconfm::QueueModeFanout& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.appIDs());
}

inline bool mqbconfm::operator==(const mqbconfm::QueueModePriority&,
                                 const mqbconfm::QueueModePriority&)
{
    return true;
}

inline bool mqbconfm::operator!=(const mqbconfm::QueueModePriority&,
                                 const mqbconfm::QueueModePriority&)
{
    return false;
}

inline bsl::ostream&
mqbconfm::operator<<(bsl::ostream&                      stream,
                     const mqbconfm::QueueModePriority& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                  hashAlg,
                          const mqbconfm::QueueModePriority& object)
{
    (void)hashAlg;
    (void)object;
}

inline bool mqbconfm::operator==(const mqbconfm::Consistency& lhs,
                                 const mqbconfm::Consistency& rhs)
{
    typedef mqbconfm::Consistency Class;
    if (lhs.selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_EVENTUAL:
            return lhs.eventual() == rhs.eventual();
        case Class::SELECTION_ID_STRONG: return lhs.strong() == rhs.strong();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

inline bool mqbconfm::operator!=(const mqbconfm::Consistency& lhs,
                                 const mqbconfm::Consistency& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream&                stream,
                                          const mqbconfm::Consistency& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&            hashAlg,
                          const mqbconfm::Consistency& object)
{
    typedef mqbconfm::Consistency Class;
    using bslh::hashAppend;
    hashAppend(hashAlg, object.selectionId());
    switch (object.selectionId()) {
    case Class::SELECTION_ID_EVENTUAL:
        hashAppend(hashAlg, object.eventual());
        break;
    case Class::SELECTION_ID_STRONG:
        hashAppend(hashAlg, object.strong());
        break;
    default:
        BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == object.selectionId());
    }
}

inline bool mqbconfm::operator==(const mqbconfm::DomainConfigRequest& lhs,
                                 const mqbconfm::DomainConfigRequest& rhs)
{
    return lhs.brokerIdentity() == rhs.brokerIdentity() &&
           lhs.domainName() == rhs.domainName();
}

inline bool mqbconfm::operator!=(const mqbconfm::DomainConfigRequest& lhs,
                                 const mqbconfm::DomainConfigRequest& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream&
mqbconfm::operator<<(bsl::ostream&                        stream,
                     const mqbconfm::DomainConfigRequest& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                    hashAlg,
                          const mqbconfm::DomainConfigRequest& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.brokerIdentity());
    hashAppend(hashAlg, object.domainName());
}

inline bool mqbconfm::operator==(const mqbconfm::QueueMode& lhs,
                                 const mqbconfm::QueueMode& rhs)
{
    typedef mqbconfm::QueueMode Class;
    if (lhs.selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_FANOUT: return lhs.fanout() == rhs.fanout();
        case Class::SELECTION_ID_PRIORITY:
            return lhs.priority() == rhs.priority();
        case Class::SELECTION_ID_BROADCAST:
            return lhs.broadcast() == rhs.broadcast();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

inline bool mqbconfm::operator!=(const mqbconfm::QueueMode& lhs,
                                 const mqbconfm::QueueMode& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream&              stream,
                                          const mqbconfm::QueueMode& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&          hashAlg,
                          const mqbconfm::QueueMode& object)
{
    typedef mqbconfm::QueueMode Class;
    using bslh::hashAppend;
    hashAppend(hashAlg, object.selectionId());
    switch (object.selectionId()) {
    case Class::SELECTION_ID_FANOUT:
        hashAppend(hashAlg, object.fanout());
        break;
    case Class::SELECTION_ID_PRIORITY:
        hashAppend(hashAlg, object.priority());
        break;
    case Class::SELECTION_ID_BROADCAST:
        hashAppend(hashAlg, object.broadcast());
        break;
    default:
        BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == object.selectionId());
    }
}

inline bool mqbconfm::operator==(const mqbconfm::Response& lhs,
                                 const mqbconfm::Response& rhs)
{
    typedef mqbconfm::Response Class;
    if (lhs.selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_FAILURE:
            return lhs.failure() == rhs.failure();
        case Class::SELECTION_ID_DOMAIN_CONFIG:
            return lhs.domainConfig() == rhs.domainConfig();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

inline bool mqbconfm::operator!=(const mqbconfm::Response& lhs,
                                 const mqbconfm::Response& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream&             stream,
                                          const mqbconfm::Response& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&         hashAlg,
                          const mqbconfm::Response& object)
{
    typedef mqbconfm::Response Class;
    using bslh::hashAppend;
    hashAppend(hashAlg, object.selectionId());
    switch (object.selectionId()) {
    case Class::SELECTION_ID_FAILURE:
        hashAppend(hashAlg, object.failure());
        break;
    case Class::SELECTION_ID_DOMAIN_CONFIG:
        hashAppend(hashAlg, object.domainConfig());
        break;
    default:
        BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == object.selectionId());
    }
}

inline bool mqbconfm::operator==(const mqbconfm::Storage& lhs,
                                 const mqbconfm::Storage& rhs)
{
    typedef mqbconfm::Storage Class;
    if (lhs.selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_IN_MEMORY:
            return lhs.inMemory() == rhs.inMemory();
        case Class::SELECTION_ID_FILE_BACKED:
            return lhs.fileBacked() == rhs.fileBacked();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

inline bool mqbconfm::operator!=(const mqbconfm::Storage& lhs,
                                 const mqbconfm::Storage& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream&            stream,
                                          const mqbconfm::Storage& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&        hashAlg,
                          const mqbconfm::Storage& object)
{
    typedef mqbconfm::Storage Class;
    using bslh::hashAppend;
    hashAppend(hashAlg, object.selectionId());
    switch (object.selectionId()) {
    case Class::SELECTION_ID_IN_MEMORY:
        hashAppend(hashAlg, object.inMemory());
        break;
    case Class::SELECTION_ID_FILE_BACKED:
        hashAppend(hashAlg, object.fileBacked());
        break;
    default:
        BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == object.selectionId());
    }
}

inline bool mqbconfm::operator==(const mqbconfm::Request& lhs,
                                 const mqbconfm::Request& rhs)
{
    typedef mqbconfm::Request Class;
    if (lhs.selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_DOMAIN_CONFIG:
            return lhs.domainConfig() == rhs.domainConfig();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

inline bool mqbconfm::operator!=(const mqbconfm::Request& lhs,
                                 const mqbconfm::Request& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream&            stream,
                                          const mqbconfm::Request& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&        hashAlg,
                          const mqbconfm::Request& object)
{
    typedef mqbconfm::Request Class;
    using bslh::hashAppend;
    hashAppend(hashAlg, object.selectionId());
    switch (object.selectionId()) {
    case Class::SELECTION_ID_DOMAIN_CONFIG:
        hashAppend(hashAlg, object.domainConfig());
        break;
    default:
        BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == object.selectionId());
    }
}

inline bool mqbconfm::operator==(const mqbconfm::StorageDefinition& lhs,
                                 const mqbconfm::StorageDefinition& rhs)
{
    return lhs.domainLimits() == rhs.domainLimits() &&
           lhs.queueLimits() == rhs.queueLimits() &&
           lhs.config() == rhs.config();
}

inline bool mqbconfm::operator!=(const mqbconfm::StorageDefinition& lhs,
                                 const mqbconfm::StorageDefinition& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream&
mqbconfm::operator<<(bsl::ostream&                      stream,
                     const mqbconfm::StorageDefinition& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                  hashAlg,
                          const mqbconfm::StorageDefinition& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.domainLimits());
    hashAppend(hashAlg, object.queueLimits());
    hashAppend(hashAlg, object.config());
}

inline bool mqbconfm::operator==(const mqbconfm::Domain& lhs,
                                 const mqbconfm::Domain& rhs)
{
    return lhs.name() == rhs.name() && lhs.mode() == rhs.mode() &&
           lhs.storage() == rhs.storage() &&
           lhs.maxConsumers() == rhs.maxConsumers() &&
           lhs.maxProducers() == rhs.maxProducers() &&
           lhs.maxQueues() == rhs.maxQueues() &&
           lhs.msgGroupIdConfig() == rhs.msgGroupIdConfig() &&
           lhs.maxIdleTime() == rhs.maxIdleTime() &&
           lhs.messageTtl() == rhs.messageTtl() &&
           lhs.maxDeliveryAttempts() == rhs.maxDeliveryAttempts() &&
           lhs.deduplicationTimeMs() == rhs.deduplicationTimeMs() &&
           lhs.consistency() == rhs.consistency();
}

inline bool mqbconfm::operator!=(const mqbconfm::Domain& lhs,
                                 const mqbconfm::Domain& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream&           stream,
                                          const mqbconfm::Domain& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&       hashAlg,
                          const mqbconfm::Domain& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.name());
    hashAppend(hashAlg, object.mode());
    hashAppend(hashAlg, object.storage());
    hashAppend(hashAlg, object.maxConsumers());
    hashAppend(hashAlg, object.maxProducers());
    hashAppend(hashAlg, object.maxQueues());
    hashAppend(hashAlg, object.msgGroupIdConfig());
    hashAppend(hashAlg, object.maxIdleTime());
    hashAppend(hashAlg, object.messageTtl());
    hashAppend(hashAlg, object.maxDeliveryAttempts());
    hashAppend(hashAlg, object.deduplicationTimeMs());
    hashAppend(hashAlg, object.consistency());
}

inline bool mqbconfm::operator==(const mqbconfm::DomainDefinition& lhs,
                                 const mqbconfm::DomainDefinition& rhs)
{
    return lhs.location() == rhs.location() &&
           lhs.parameters() == rhs.parameters();
}

inline bool mqbconfm::operator!=(const mqbconfm::DomainDefinition& lhs,
                                 const mqbconfm::DomainDefinition& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream&
mqbconfm::operator<<(bsl::ostream&                     stream,
                     const mqbconfm::DomainDefinition& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&                 hashAlg,
                          const mqbconfm::DomainDefinition& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.location());
    hashAppend(hashAlg, object.parameters());
}

inline bool mqbconfm::operator==(const mqbconfm::DomainVariant& lhs,
                                 const mqbconfm::DomainVariant& rhs)
{
    typedef mqbconfm::DomainVariant Class;
    if (lhs.selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_DEFINITION:
            return lhs.definition() == rhs.definition();
        case Class::SELECTION_ID_REDIRECT:
            return lhs.redirect() == rhs.redirect();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

inline bool mqbconfm::operator!=(const mqbconfm::DomainVariant& lhs,
                                 const mqbconfm::DomainVariant& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mqbconfm::operator<<(bsl::ostream& stream,
                                          const mqbconfm::DomainVariant& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mqbconfm::hashAppend(t_HASH_ALGORITHM&              hashAlg,
                          const mqbconfm::DomainVariant& object)
{
    typedef mqbconfm::DomainVariant Class;
    using bslh::hashAppend;
    hashAppend(hashAlg, object.selectionId());
    switch (object.selectionId()) {
    case Class::SELECTION_ID_DEFINITION:
        hashAppend(hashAlg, object.definition());
        break;
    case Class::SELECTION_ID_REDIRECT:
        hashAppend(hashAlg, object.redirect());
        break;
    default:
        BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == object.selectionId());
    }
}

}  // close enterprise namespace
#endif

// GENERATED BY BLP_BAS_CODEGEN_2023.05.27
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbconfm --msgComponent messages mqbconf.xsd