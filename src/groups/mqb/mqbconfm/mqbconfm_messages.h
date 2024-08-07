// Copyright 2014-2024 Bloomberg Finance L.P.
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

#include <bdlat_enumeratorinfo.h>

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

#include <bsl_ostream.h>
#include <bsl_string.h>

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
class Expression;
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
class Subscription;
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

class BrokerIdentity {
    // Generic type to hold identification of a broker.
    // hostName......: machine name hostTags......: machine tags
    // brokerVersion.: version of the broker

    // INSTANCE DATA
    bsl::string d_hostName;
    bsl::string d_hostTags;
    bsl::string d_brokerVersion;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit BrokerIdentity(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'BrokerIdentity' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    BrokerIdentity(const BrokerIdentity& original,
                   bslma::Allocator*     basicAllocator = 0);
    // Create an object of type 'BrokerIdentity' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    BrokerIdentity(BrokerIdentity&& original) noexcept;
    // Create an object of type 'BrokerIdentity' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    BrokerIdentity(BrokerIdentity&&  original,
                   bslma::Allocator* basicAllocator);
    // Create an object of type 'BrokerIdentity' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~BrokerIdentity();
    // Destroy this object.

    // MANIPULATORS
    BrokerIdentity& operator=(const BrokerIdentity& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    BrokerIdentity& operator=(BrokerIdentity&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& hostName();
    // Return a reference to the modifiable "HostName" attribute of this
    // object.

    bsl::string& hostTags();
    // Return a reference to the modifiable "HostTags" attribute of this
    // object.

    bsl::string& brokerVersion();
    // Return a reference to the modifiable "BrokerVersion" attribute of
    // this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& hostName() const;
    // Return a reference offering non-modifiable access to the "HostName"
    // attribute of this object.

    const bsl::string& hostTags() const;
    // Return a reference offering non-modifiable access to the "HostTags"
    // attribute of this object.

    const bsl::string& brokerVersion() const;
    // Return a reference offering non-modifiable access to the
    // "BrokerVersion" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const BrokerIdentity& lhs,
                           const BrokerIdentity& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.hostName() == rhs.hostName() &&
               lhs.hostTags() == rhs.hostTags() &&
               lhs.brokerVersion() == rhs.brokerVersion();
    }

    friend bool operator!=(const BrokerIdentity& lhs,
                           const BrokerIdentity& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&         stream,
                                    const BrokerIdentity& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&     hashAlg,
                           const BrokerIdentity& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'BrokerIdentity'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::BrokerIdentity)

namespace mqbconfm {

// =====================
// class DomainConfigRaw
// =====================

class DomainConfigRaw {
    // Response of a get domain config request.

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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit DomainConfigRaw(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'DomainConfigRaw' having the default value.
    //  Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    DomainConfigRaw(const DomainConfigRaw& original,
                    bslma::Allocator*      basicAllocator = 0);
    // Create an object of type 'DomainConfigRaw' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainConfigRaw(DomainConfigRaw&& original) noexcept;
    // Create an object of type 'DomainConfigRaw' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    DomainConfigRaw(DomainConfigRaw&& original,
                    bslma::Allocator* basicAllocator);
    // Create an object of type 'DomainConfigRaw' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~DomainConfigRaw();
    // Destroy this object.

    // MANIPULATORS
    DomainConfigRaw& operator=(const DomainConfigRaw& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainConfigRaw& operator=(DomainConfigRaw&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& domainName();
    // Return a reference to the modifiable "DomainName" attribute of this
    // object.

    bsl::string& config();
    // Return a reference to the modifiable "Config" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& domainName() const;
    // Return a reference offering non-modifiable access to the
    // "DomainName" attribute of this object.

    const bsl::string& config() const;
    // Return a reference offering non-modifiable access to the "Config"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DomainConfigRaw& lhs,
                           const DomainConfigRaw& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.domainName() == rhs.domainName() &&
               lhs.config() == rhs.config();
    }

    friend bool operator!=(const DomainConfigRaw& lhs,
                           const DomainConfigRaw& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&          stream,
                                    const DomainConfigRaw& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&      hashAlg,
                           const DomainConfigRaw& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DomainConfigRaw'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.domainName());
        hashAppend(hashAlg, object.config());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainConfigRaw)

namespace mqbconfm {

// ====================
// class DomainResolver
// ====================

class DomainResolver {
    // Top level type representing the information retrieved when resolving a
    // domain.  Review: Keep this? Why not just store the cluster name?
    // name....: Domain name cluster.: Cluster name

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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit DomainResolver(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'DomainResolver' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    DomainResolver(const DomainResolver& original,
                   bslma::Allocator*     basicAllocator = 0);
    // Create an object of type 'DomainResolver' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainResolver(DomainResolver&& original) noexcept;
    // Create an object of type 'DomainResolver' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    DomainResolver(DomainResolver&&  original,
                   bslma::Allocator* basicAllocator);
    // Create an object of type 'DomainResolver' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~DomainResolver();
    // Destroy this object.

    // MANIPULATORS
    DomainResolver& operator=(const DomainResolver& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainResolver& operator=(DomainResolver&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& name();
    // Return a reference to the modifiable "Name" attribute of this
    // object.

    bsl::string& cluster();
    // Return a reference to the modifiable "Cluster" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& name() const;
    // Return a reference offering non-modifiable access to the "Name"
    // attribute of this object.

    const bsl::string& cluster() const;
    // Return a reference offering non-modifiable access to the "Cluster"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DomainResolver& lhs,
                           const DomainResolver& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.name() == rhs.name() && lhs.cluster() == rhs.cluster();
    }

    friend bool operator!=(const DomainResolver& lhs,
                           const DomainResolver& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&         stream,
                                    const DomainResolver& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&     hashAlg,
                           const DomainResolver& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DomainResolver'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.name());
        hashAppend(hashAlg, object.cluster());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainResolver)

namespace mqbconfm {

// =======================
// class ExpressionVersion
// =======================

struct ExpressionVersion {
    // Enumeration of the various expression versions.

  public:
    // TYPES
    enum Value { E_UNDEFINED = 0, E_VERSION_1 = 1 };

    enum { NUM_ENUMERATORS = 2 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    static const char* toString(Value value);
    // Return the string representation exactly matching the enumerator
    // name corresponding to the specified enumeration 'value'.

    static int fromString(Value* result, const char* string, int stringLength);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string' of the specified 'stringLength'.  Return 0 on
    // success, and a non-zero value with no effect on 'result' otherwise
    // (i.e., 'string' does not match any enumerator).

    static int fromString(Value* result, const bsl::string& string);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'string' does not match any
    // enumerator).

    static int fromInt(Value* result, int number);
    // Load into the specified 'result' the enumerator matching the
    // specified 'number'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'number' does not match any
    // enumerator).

    static bsl::ostream& print(bsl::ostream& stream, Value value);
    // Write to the specified 'stream' the string representation of
    // the specified enumeration 'value'.  Return a reference to
    // the modifiable 'stream'.

    // HIDDEN FRIENDS
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return ExpressionVersion::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mqbconfm::ExpressionVersion)

namespace mqbconfm {

// =============
// class Failure
// =============

class Failure {
    // Generic type to represent an error.
    // code.....: an integer value representing the error message..: an
    // optional string describing the error

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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit Failure(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Failure' having the default value.  Use
    // the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Failure(const Failure& original, bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Failure' having the value of the specified
    // 'original' object.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Failure(Failure&& original) noexcept;
    // Create an object of type 'Failure' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.

    Failure(Failure&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Failure' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.  Use the
    // optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Failure();
    // Destroy this object.

    // MANIPULATORS
    Failure& operator=(const Failure& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Failure& operator=(Failure&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    int& code();
    // Return a reference to the modifiable "Code" attribute of this
    // object.

    bsl::string& message();
    // Return a reference to the modifiable "Message" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    int code() const;
    // Return the value of the "Code" attribute of this object.

    const bsl::string& message() const;
    // Return a reference offering non-modifiable access to the "Message"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Failure& lhs, const Failure& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.code() == rhs.code() && lhs.message() == rhs.message();
    }

    friend bool operator!=(const Failure& lhs, const Failure& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const Failure& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Failure& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Failure'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.code());
        hashAppend(hashAlg, object.message());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::Failure)

namespace mqbconfm {

// =======================
// class FileBackedStorage
// =======================

class FileBackedStorage {
    // Configuration for storage using a file on disk.

    // INSTANCE DATA

  public:
    // TYPES
    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS

    // MANIPULATORS
    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    // HIDDEN FRIENDS
    friend bool operator==(const FileBackedStorage&, const FileBackedStorage&)
    // Returns 'true' as this type has no attributes and so all objects of
    // this type are considered equal.
    {
        return true;
    }

    friend bool operator!=(const FileBackedStorage& lhs,
                           const FileBackedStorage& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const FileBackedStorage& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const FileBackedStorage&)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'FileBackedStorage'.
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::FileBackedStorage)

namespace mqbconfm {

// =====================
// class InMemoryStorage
// =====================

class InMemoryStorage {
    // Configuration for storage using an in-memory map.

    // INSTANCE DATA

  public:
    // TYPES
    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS

    // MANIPULATORS
    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    // HIDDEN FRIENDS
    friend bool operator==(const InMemoryStorage&, const InMemoryStorage&)
    // Returns 'true' as this type has no attributes and so all objects of
    // this type are considered equal.
    {
        return true;
    }

    friend bool operator!=(const InMemoryStorage& lhs,
                           const InMemoryStorage& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&          stream,
                                    const InMemoryStorage& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const InMemoryStorage&)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'InMemoryStorage'.
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::InMemoryStorage)

namespace mqbconfm {

// ============
// class Limits
// ============

class Limits {
    // Represent the various limitations to apply to either a 'domain' or an
    // individual 'queue'.
    // messages...............: maximum number of messages
    // messagesWatermarkRatio.: threshold ratio to the maximum number of
    // messages for which a high watermark alarm will trigger
    // bytes..................: maximum cumulated number of bytes
    // bytesWatermarkRatio....: threshold ratio to the maximum cumulated number
    // of bytes for which a high watermark alarm will trigger

    // INSTANCE DATA
    double             d_messagesWatermarkRatio;
    double             d_bytesWatermarkRatio;
    bsls::Types::Int64 d_messages;
    bsls::Types::Int64 d_bytes;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Limits& rhs) const;

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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    Limits();
    // Create an object of type 'Limits' having the default value.

    // MANIPULATORS
    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsls::Types::Int64& messages();
    // Return a reference to the modifiable "Messages" attribute of this
    // object.

    double& messagesWatermarkRatio();
    // Return a reference to the modifiable "MessagesWatermarkRatio"
    // attribute of this object.

    bsls::Types::Int64& bytes();
    // Return a reference to the modifiable "Bytes" attribute of this
    // object.

    double& bytesWatermarkRatio();
    // Return a reference to the modifiable "BytesWatermarkRatio" attribute
    // of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    bsls::Types::Int64 messages() const;
    // Return the value of the "Messages" attribute of this object.

    double messagesWatermarkRatio() const;
    // Return the value of the "MessagesWatermarkRatio" attribute of this
    // object.

    bsls::Types::Int64 bytes() const;
    // Return the value of the "Bytes" attribute of this object.

    double bytesWatermarkRatio() const;
    // Return the value of the "BytesWatermarkRatio" attribute of this
    // object.

    // HIDDEN FRIENDS
    friend bool operator==(const Limits& lhs, const Limits& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const Limits& lhs, const Limits& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const Limits& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Limits& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Limits'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::Limits)

namespace mqbconfm {

// ======================
// class MsgGroupIdConfig
// ======================

class MsgGroupIdConfig {
    // Configuration for the use of Group Ids for routing.  The garbage
    // collection arguments could be assigned manually or get calculated out of
    // statistics on the streams.  They are considered internal and our
    // intentions is _not_ to give customers full control over those numbers.
    // Their role is to protect BlazingMQ from abuse i.e.  cases of infinite
    // Group Ids being stored.  Another assumption is that 'maxGroups >> number
    // of consumers'.
    // rebalance..: groups will be dynamically rebalanced in way such that all
    // consumers have equal share of Group Ids assigned to them maxGroups..:
    // Maximum number of groups.  If the number of groups gets larger than
    // this, the least recently used one is evicted.  This is a "garbage
    // collection" parameter ttlSeconds.: minimum time of inactivity (no
    // messages for a Group Id), in seconds, before a group becomes available
    // for "garbage collection".  0 (the default) means unlimited

    // INSTANCE DATA
    bsls::Types::Int64 d_ttlSeconds;
    int                d_maxGroups;
    bool               d_rebalance;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    MsgGroupIdConfig();
    // Create an object of type 'MsgGroupIdConfig' having the default
    // value.

    // MANIPULATORS
    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bool& rebalance();
    // Return a reference to the modifiable "Rebalance" attribute of this
    // object.

    int& maxGroups();
    // Return a reference to the modifiable "MaxGroups" attribute of this
    // object.

    bsls::Types::Int64& ttlSeconds();
    // Return a reference to the modifiable "TtlSeconds" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    bool rebalance() const;
    // Return the value of the "Rebalance" attribute of this object.

    int maxGroups() const;
    // Return the value of the "MaxGroups" attribute of this object.

    bsls::Types::Int64 ttlSeconds() const;
    // Return the value of the "TtlSeconds" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const MsgGroupIdConfig& lhs,
                           const MsgGroupIdConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.rebalance() == rhs.rebalance() &&
               lhs.maxGroups() == rhs.maxGroups() &&
               lhs.ttlSeconds() == rhs.ttlSeconds();
    }

    friend bool operator!=(const MsgGroupIdConfig& lhs,
                           const MsgGroupIdConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const MsgGroupIdConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const MsgGroupIdConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'MsgGroupIdConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::MsgGroupIdConfig)

namespace mqbconfm {

// ==============================
// class QueueConsistencyEventual
// ==============================

class QueueConsistencyEventual {
    // Configuration for eventual consistency.

    // INSTANCE DATA

  public:
    // TYPES
    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS

    // MANIPULATORS
    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    // HIDDEN FRIENDS
    friend bool operator==(const QueueConsistencyEventual&,
                           const QueueConsistencyEventual&)
    // Returns 'true' as this type has no attributes and so all objects of
    // this type are considered equal.
    {
        return true;
    }

    friend bool operator!=(const QueueConsistencyEventual& lhs,
                           const QueueConsistencyEventual& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                   stream,
                                    const QueueConsistencyEventual& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const QueueConsistencyEventual&)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'QueueConsistencyEventual'.
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(
    mqbconfm::QueueConsistencyEventual)

namespace mqbconfm {

// ============================
// class QueueConsistencyStrong
// ============================

class QueueConsistencyStrong {
    // Configuration for strong consistency.

    // INSTANCE DATA

  public:
    // TYPES
    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS

    // MANIPULATORS
    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    // HIDDEN FRIENDS
    friend bool operator==(const QueueConsistencyStrong&,
                           const QueueConsistencyStrong&)
    // Returns 'true' as this type has no attributes and so all objects of
    // this type are considered equal.
    {
        return true;
    }

    friend bool operator!=(const QueueConsistencyStrong& lhs,
                           const QueueConsistencyStrong& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                 stream,
                                    const QueueConsistencyStrong& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const QueueConsistencyStrong&)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'QueueConsistencyStrong'.
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(
    mqbconfm::QueueConsistencyStrong)

namespace mqbconfm {

// ========================
// class QueueModeBroadcast
// ========================

class QueueModeBroadcast {
    // Configuration for a broadcast queue.

    // INSTANCE DATA

  public:
    // TYPES
    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS

    // MANIPULATORS
    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    // HIDDEN FRIENDS
    friend bool operator==(const QueueModeBroadcast&,
                           const QueueModeBroadcast&)
    // Returns 'true' as this type has no attributes and so all objects of
    // this type are considered equal.
    {
        return true;
    }

    friend bool operator!=(const QueueModeBroadcast& lhs,
                           const QueueModeBroadcast& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const QueueModeBroadcast& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const QueueModeBroadcast&)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'QueueModeBroadcast'.
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::QueueModeBroadcast)

namespace mqbconfm {

// =====================
// class QueueModeFanout
// =====================

class QueueModeFanout {
    // Configuration for a fanout queue.
    // appIDs.............: List of appIDs authorized to consume from the
    // queue.  publishAppIdMetrics: Whether to publish appId metrics.

    // INSTANCE DATA
    bsl::vector<bsl::string> d_appIDs;
    bool                     d_publishAppIdMetrics;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_APP_I_DS               = 0,
        ATTRIBUTE_ID_PUBLISH_APP_ID_METRICS = 1
    };

    enum { NUM_ATTRIBUTES = 2 };

    enum {
        ATTRIBUTE_INDEX_APP_I_DS               = 0,
        ATTRIBUTE_INDEX_PUBLISH_APP_ID_METRICS = 1
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_PUBLISH_APP_ID_METRICS;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit QueueModeFanout(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'QueueModeFanout' having the default value.
    //  Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    QueueModeFanout(const QueueModeFanout& original,
                    bslma::Allocator*      basicAllocator = 0);
    // Create an object of type 'QueueModeFanout' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueModeFanout(QueueModeFanout&& original) noexcept;
    // Create an object of type 'QueueModeFanout' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    QueueModeFanout(QueueModeFanout&& original,
                    bslma::Allocator* basicAllocator);
    // Create an object of type 'QueueModeFanout' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~QueueModeFanout();
    // Destroy this object.

    // MANIPULATORS
    QueueModeFanout& operator=(const QueueModeFanout& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueModeFanout& operator=(QueueModeFanout&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::vector<bsl::string>& appIDs();
    // Return a reference to the modifiable "AppIDs" attribute of this
    // object.

    bool& publishAppIdMetrics();
    // Return a reference to the modifiable "PublishAppIdMetrics" attribute
    // of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::vector<bsl::string>& appIDs() const;
    // Return a reference offering non-modifiable access to the "AppIDs"
    // attribute of this object.

    bool publishAppIdMetrics() const;
    // Return the value of the "PublishAppIdMetrics" attribute of this
    // object.

    // HIDDEN FRIENDS
    friend bool operator==(const QueueModeFanout& lhs,
                           const QueueModeFanout& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.appIDs() == rhs.appIDs() &&
               lhs.publishAppIdMetrics() == rhs.publishAppIdMetrics();
    }

    friend bool operator!=(const QueueModeFanout& lhs,
                           const QueueModeFanout& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&          stream,
                                    const QueueModeFanout& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&      hashAlg,
                           const QueueModeFanout& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'QueueModeFanout'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.appIDs());
        hashAppend(hashAlg, object.publishAppIdMetrics());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::QueueModeFanout)

namespace mqbconfm {

// =======================
// class QueueModePriority
// =======================

class QueueModePriority {
    // Configuration for a priority queue.

    // INSTANCE DATA

  public:
    // TYPES
    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS

    // MANIPULATORS
    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    // HIDDEN FRIENDS
    friend bool operator==(const QueueModePriority&, const QueueModePriority&)
    // Returns 'true' as this type has no attributes and so all objects of
    // this type are considered equal.
    {
        return true;
    }

    friend bool operator!=(const QueueModePriority& lhs,
                           const QueueModePriority& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const QueueModePriority& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const QueueModePriority&)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'QueueModePriority'.
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::QueueModePriority)

namespace mqbconfm {

// =================
// class Consistency
// =================

class Consistency {
    // Consistency choices to configure a queue.
    // eventual........: no Replication Receipt is required.  strong..........:
    // require Replication Receipt before ACK/PUSH

    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<QueueConsistencyEventual> d_eventual;
        bsls::ObjectBuffer<QueueConsistencyStrong>   d_strong;
    };

    int d_selectionId;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Consistency& rhs) const;

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
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    Consistency();
    // Create an object of type 'Consistency' having the default value.

    Consistency(const Consistency& original);
    // Create an object of type 'Consistency' having the value of the
    // specified 'original' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Consistency(Consistency&& original) noexcept;
    // Create an object of type 'Consistency' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
#endif

    ~Consistency();
    // Destroy this object.

    // MANIPULATORS
    Consistency& operator=(const Consistency& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Consistency& operator=(Consistency&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

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

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    QueueConsistencyEventual& eventual();
    // Return a reference to the modifiable "Eventual" selection of this
    // object if "Eventual" is the current selection.  The behavior is
    // undefined unless "Eventual" is the selection of this object.

    QueueConsistencyStrong& strong();
    // Return a reference to the modifiable "Strong" selection of this
    // object if "Strong" is the current selection.  The behavior is
    // undefined unless "Strong" is the selection of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const QueueConsistencyEventual& eventual() const;
    // Return a reference to the non-modifiable "Eventual" selection of
    // this object if "Eventual" is the current selection.  The behavior is
    // undefined unless "Eventual" is the selection of this object.

    const QueueConsistencyStrong& strong() const;
    // Return a reference to the non-modifiable "Strong" selection of this
    // object if "Strong" is the current selection.  The behavior is
    // undefined unless "Strong" is the selection of this object.

    bool isEventualValue() const;
    // Return 'true' if the value of this object is a "Eventual" value, and
    // return 'false' otherwise.

    bool isStrongValue() const;
    // Return 'true' if the value of this object is a "Strong" value, and
    // return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Consistency& lhs, const Consistency& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'Consistency' objects have the
    // same value if either the selections in both objects have the same
    // ids and the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const Consistency& lhs, const Consistency& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&      stream,
                                    const Consistency& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&  hashAlg,
                           const Consistency& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Consistency'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::Consistency)

namespace mqbconfm {

// =========================
// class DomainConfigRequest
// =========================

class DomainConfigRequest {
    // Request to get a domain config.

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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit DomainConfigRequest(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'DomainConfigRequest' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    DomainConfigRequest(const DomainConfigRequest& original,
                        bslma::Allocator*          basicAllocator = 0);
    // Create an object of type 'DomainConfigRequest' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainConfigRequest(DomainConfigRequest&& original) noexcept;
    // Create an object of type 'DomainConfigRequest' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    DomainConfigRequest(DomainConfigRequest&& original,
                        bslma::Allocator*     basicAllocator);
    // Create an object of type 'DomainConfigRequest' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~DomainConfigRequest();
    // Destroy this object.

    // MANIPULATORS
    DomainConfigRequest& operator=(const DomainConfigRequest& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainConfigRequest& operator=(DomainConfigRequest&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    BrokerIdentity& brokerIdentity();
    // Return a reference to the modifiable "BrokerIdentity" attribute of
    // this object.

    bsl::string& domainName();
    // Return a reference to the modifiable "DomainName" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const BrokerIdentity& brokerIdentity() const;
    // Return a reference offering non-modifiable access to the
    // "BrokerIdentity" attribute of this object.

    const bsl::string& domainName() const;
    // Return a reference offering non-modifiable access to the
    // "DomainName" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DomainConfigRequest& lhs,
                           const DomainConfigRequest& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.brokerIdentity() == rhs.brokerIdentity() &&
               lhs.domainName() == rhs.domainName();
    }

    friend bool operator!=(const DomainConfigRequest& lhs,
                           const DomainConfigRequest& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&              stream,
                                    const DomainConfigRequest& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&          hashAlg,
                           const DomainConfigRequest& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DomainConfigRequest'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.brokerIdentity());
        hashAppend(hashAlg, object.domainName());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainConfigRequest)

namespace mqbconfm {

// ================
// class Expression
// ================

class Expression {
    // This complex type contains expression to evaluate when selecting
    // Subscription for delivery.
    // version................: expression version (default is no expression)
    // text...................: textual representation of the expression

    // INSTANCE DATA
    bsl::string              d_text;
    ExpressionVersion::Value d_version;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_VERSION = 0, ATTRIBUTE_ID_TEXT = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_VERSION = 0, ATTRIBUTE_INDEX_TEXT = 1 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const ExpressionVersion::Value DEFAULT_INITIALIZER_VERSION;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit Expression(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Expression' having the default value.  Use
    // the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Expression(const Expression& original,
               bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Expression' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Expression(Expression&& original) noexcept;
    // Create an object of type 'Expression' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    Expression(Expression&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Expression' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Expression();
    // Destroy this object.

    // MANIPULATORS
    Expression& operator=(const Expression& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Expression& operator=(Expression&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    ExpressionVersion::Value& version();
    // Return a reference to the modifiable "Version" attribute of this
    // object.

    bsl::string& text();
    // Return a reference to the modifiable "Text" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    ExpressionVersion::Value version() const;
    // Return the value of the "Version" attribute of this object.

    const bsl::string& text() const;
    // Return a reference offering non-modifiable access to the "Text"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Expression& lhs, const Expression& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.version() == rhs.version() && lhs.text() == rhs.text();
    }

    friend bool operator!=(const Expression& lhs, const Expression& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&     stream,
                                    const Expression& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Expression& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Expression'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.version());
        hashAppend(hashAlg, object.text());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::Expression)

namespace mqbconfm {

// ===============
// class QueueMode
// ===============

class QueueMode {
    // Choice of all the various modes a queue can be configured in.
    // fanout.........: multiple consumers are each getting all messages
    // priority.......: consumers with highest priority are sharing load in
    // round robin way broadcast......: send to all available consumers on a
    // best-effort basis

    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<QueueModeFanout>    d_fanout;
        bsls::ObjectBuffer<QueueModePriority>  d_priority;
        bsls::ObjectBuffer<QueueModeBroadcast> d_broadcast;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const QueueMode& rhs) const;

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
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    explicit QueueMode(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'QueueMode' having the default value.  Use
    // the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    QueueMode(const QueueMode& original, bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'QueueMode' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueMode(QueueMode&& original) noexcept;
    // Create an object of type 'QueueMode' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    QueueMode(QueueMode&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'QueueMode' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~QueueMode();
    // Destroy this object.

    // MANIPULATORS
    QueueMode& operator=(const QueueMode& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QueueMode& operator=(QueueMode&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

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

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    QueueModeFanout& fanout();
    // Return a reference to the modifiable "Fanout" selection of this
    // object if "Fanout" is the current selection.  The behavior is
    // undefined unless "Fanout" is the selection of this object.

    QueueModePriority& priority();
    // Return a reference to the modifiable "Priority" selection of this
    // object if "Priority" is the current selection.  The behavior is
    // undefined unless "Priority" is the selection of this object.

    QueueModeBroadcast& broadcast();
    // Return a reference to the modifiable "Broadcast" selection of this
    // object if "Broadcast" is the current selection.  The behavior is
    // undefined unless "Broadcast" is the selection of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const QueueModeFanout& fanout() const;
    // Return a reference to the non-modifiable "Fanout" selection of this
    // object if "Fanout" is the current selection.  The behavior is
    // undefined unless "Fanout" is the selection of this object.

    const QueueModePriority& priority() const;
    // Return a reference to the non-modifiable "Priority" selection of
    // this object if "Priority" is the current selection.  The behavior is
    // undefined unless "Priority" is the selection of this object.

    const QueueModeBroadcast& broadcast() const;
    // Return a reference to the non-modifiable "Broadcast" selection of
    // this object if "Broadcast" is the current selection.  The behavior
    // is undefined unless "Broadcast" is the selection of this object.

    bool isFanoutValue() const;
    // Return 'true' if the value of this object is a "Fanout" value, and
    // return 'false' otherwise.

    bool isPriorityValue() const;
    // Return 'true' if the value of this object is a "Priority" value, and
    // return 'false' otherwise.

    bool isBroadcastValue() const;
    // Return 'true' if the value of this object is a "Broadcast" value,
    // and return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const QueueMode& lhs, const QueueMode& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'QueueMode' objects have the same
    // value if either the selections in both objects have the same ids and
    // the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const QueueMode& lhs, const QueueMode& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const QueueMode& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const QueueMode& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'QueueMode'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::QueueMode)

namespace mqbconfm {

// ==============
// class Response
// ==============

class Response {
    // The choice between all the possible responses from the bmqconf task.

    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<Failure>         d_failure;
        bsls::ObjectBuffer<DomainConfigRaw> d_domainConfig;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Response& rhs) const;

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
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    explicit Response(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Response' having the default value.  Use
    // the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Response(const Response& original, bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Response' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Response(Response&& original) noexcept;
    // Create an object of type 'Response' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    Response(Response&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Response' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Response();
    // Destroy this object.

    // MANIPULATORS
    Response& operator=(const Response& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Response& operator=(Response&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

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

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    Failure& failure();
    // Return a reference to the modifiable "Failure" selection of this
    // object if "Failure" is the current selection.  The behavior is
    // undefined unless "Failure" is the selection of this object.

    DomainConfigRaw& domainConfig();
    // Return a reference to the modifiable "DomainConfig" selection of
    // this object if "DomainConfig" is the current selection.  The
    // behavior is undefined unless "DomainConfig" is the selection of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const Failure& failure() const;
    // Return a reference to the non-modifiable "Failure" selection of this
    // object if "Failure" is the current selection.  The behavior is
    // undefined unless "Failure" is the selection of this object.

    const DomainConfigRaw& domainConfig() const;
    // Return a reference to the non-modifiable "DomainConfig" selection of
    // this object if "DomainConfig" is the current selection.  The
    // behavior is undefined unless "DomainConfig" is the selection of this
    // object.

    bool isFailureValue() const;
    // Return 'true' if the value of this object is a "Failure" value, and
    // return 'false' otherwise.

    bool isDomainConfigValue() const;
    // Return 'true' if the value of this object is a "DomainConfig" value,
    // and return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Response& lhs, const Response& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'Response' objects have the same
    // value if either the selections in both objects have the same ids and
    // the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const Response& lhs, const Response& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const Response& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Response& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Response'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::Response)

namespace mqbconfm {

// =============
// class Storage
// =============

class Storage {
    // Choice of all the various Storage backends
    // inMemory....: store data in memory fileBacked..: store data in a file on
    // disk

    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<InMemoryStorage>   d_inMemory;
        bsls::ObjectBuffer<FileBackedStorage> d_fileBacked;
    };

    int d_selectionId;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Storage& rhs) const;

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
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    Storage();
    // Create an object of type 'Storage' having the default value.

    Storage(const Storage& original);
    // Create an object of type 'Storage' having the value of the specified
    // 'original' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Storage(Storage&& original) noexcept;
    // Create an object of type 'Storage' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.
#endif

    ~Storage();
    // Destroy this object.

    // MANIPULATORS
    Storage& operator=(const Storage& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Storage& operator=(Storage&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

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

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    InMemoryStorage& inMemory();
    // Return a reference to the modifiable "InMemory" selection of this
    // object if "InMemory" is the current selection.  The behavior is
    // undefined unless "InMemory" is the selection of this object.

    FileBackedStorage& fileBacked();
    // Return a reference to the modifiable "FileBacked" selection of this
    // object if "FileBacked" is the current selection.  The behavior is
    // undefined unless "FileBacked" is the selection of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const InMemoryStorage& inMemory() const;
    // Return a reference to the non-modifiable "InMemory" selection of
    // this object if "InMemory" is the current selection.  The behavior is
    // undefined unless "InMemory" is the selection of this object.

    const FileBackedStorage& fileBacked() const;
    // Return a reference to the non-modifiable "FileBacked" selection of
    // this object if "FileBacked" is the current selection.  The behavior
    // is undefined unless "FileBacked" is the selection of this object.

    bool isInMemoryValue() const;
    // Return 'true' if the value of this object is a "InMemory" value, and
    // return 'false' otherwise.

    bool isFileBackedValue() const;
    // Return 'true' if the value of this object is a "FileBacked" value,
    // and return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Storage& lhs, const Storage& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'Storage' objects have the same
    // value if either the selections in both objects have the same ids and
    // the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const Storage& lhs, const Storage& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const Storage& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Storage& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Storage'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::Storage)

namespace mqbconfm {

// =============
// class Request
// =============

class Request {
    // The choice between all the possible requests to the bmqconf task.

    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<DomainConfigRequest> d_domainConfig;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Request& rhs) const;

  public:
    // TYPES

    enum { SELECTION_ID_UNDEFINED = -1, SELECTION_ID_DOMAIN_CONFIG = 0 };

    enum { NUM_SELECTIONS = 1 };

    enum { SELECTION_INDEX_DOMAIN_CONFIG = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_SelectionInfo SELECTION_INFO_ARRAY[];

    // CLASS METHODS
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    explicit Request(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Request' having the default value.  Use
    // the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Request(const Request& original, bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Request' having the value of the specified
    // 'original' object.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Request(Request&& original) noexcept;
    // Create an object of type 'Request' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.

    Request(Request&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Request' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.  Use the
    // optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Request();
    // Destroy this object.

    // MANIPULATORS
    Request& operator=(const Request& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Request& operator=(Request&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

    DomainConfigRequest& makeDomainConfig();
    DomainConfigRequest& makeDomainConfig(const DomainConfigRequest& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainConfigRequest& makeDomainConfig(DomainConfigRequest&& value);
#endif
    // Set the value of this object to be a "DomainConfig" value.
    // Optionally specify the 'value' of the "DomainConfig".  If 'value' is
    // not specified, the default "DomainConfig" value is used.

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    DomainConfigRequest& domainConfig();
    // Return a reference to the modifiable "DomainConfig" selection of
    // this object if "DomainConfig" is the current selection.  The
    // behavior is undefined unless "DomainConfig" is the selection of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const DomainConfigRequest& domainConfig() const;
    // Return a reference to the non-modifiable "DomainConfig" selection of
    // this object if "DomainConfig" is the current selection.  The
    // behavior is undefined unless "DomainConfig" is the selection of this
    // object.

    bool isDomainConfigValue() const;
    // Return 'true' if the value of this object is a "DomainConfig" value,
    // and return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Request& lhs, const Request& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'Request' objects have the same
    // value if either the selections in both objects have the same ids and
    // the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const Request& lhs, const Request& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const Request& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Request& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Request'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::Request)

namespace mqbconfm {

// =======================
// class StorageDefinition
// =======================

class StorageDefinition {
    // Type representing the configuration for a Storage.
    // config........: configuration for the type of storage to use
    // domainLimits..: global limits to apply to the entire domain, cumulated
    // for all queues in the domain queueLimits...: individual limits (as a
    // subset of the global limits) to apply to each queue of the domain

    // INSTANCE DATA
    Storage d_config;
    Limits  d_domainLimits;
    Limits  d_queueLimits;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    StorageDefinition();
    // Create an object of type 'StorageDefinition' having the default
    // value.

    // MANIPULATORS
    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    Limits& domainLimits();
    // Return a reference to the modifiable "DomainLimits" attribute of
    // this object.

    Limits& queueLimits();
    // Return a reference to the modifiable "QueueLimits" attribute of this
    // object.

    Storage& config();
    // Return a reference to the modifiable "Config" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const Limits& domainLimits() const;
    // Return a reference offering non-modifiable access to the
    // "DomainLimits" attribute of this object.

    const Limits& queueLimits() const;
    // Return a reference offering non-modifiable access to the
    // "QueueLimits" attribute of this object.

    const Storage& config() const;
    // Return a reference offering non-modifiable access to the "Config"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StorageDefinition& lhs,
                           const StorageDefinition& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.domainLimits() == rhs.domainLimits() &&
               lhs.queueLimits() == rhs.queueLimits() &&
               lhs.config() == rhs.config();
    }

    friend bool operator!=(const StorageDefinition& lhs,
                           const StorageDefinition& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const StorageDefinition& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const StorageDefinition& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StorageDefinition'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbconfm::StorageDefinition)

namespace mqbconfm {

// ==================
// class Subscription
// ==================

class Subscription {
    // This complex type contains various parameters required by an upstream
    // node to configure subscription for an app.
    // appId..................: app identifier expression.............:
    // expression denoting a subscription for the app

    // INSTANCE DATA
    bsl::string d_appId;
    Expression  d_expression;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_APP_ID = 0, ATTRIBUTE_ID_EXPRESSION = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_APP_ID = 0, ATTRIBUTE_INDEX_EXPRESSION = 1 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit Subscription(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Subscription' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Subscription(const Subscription& original,
                 bslma::Allocator*   basicAllocator = 0);
    // Create an object of type 'Subscription' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Subscription(Subscription&& original) noexcept;
    // Create an object of type 'Subscription' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    Subscription(Subscription&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Subscription' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Subscription();
    // Destroy this object.

    // MANIPULATORS
    Subscription& operator=(const Subscription& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Subscription& operator=(Subscription&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& appId();
    // Return a reference to the modifiable "AppId" attribute of this
    // object.

    Expression& expression();
    // Return a reference to the modifiable "Expression" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& appId() const;
    // Return a reference offering non-modifiable access to the "AppId"
    // attribute of this object.

    const Expression& expression() const;
    // Return a reference offering non-modifiable access to the
    // "Expression" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Subscription& lhs, const Subscription& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.appId() == rhs.appId() &&
               lhs.expression() == rhs.expression();
    }

    friend bool operator!=(const Subscription& lhs, const Subscription& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&       stream,
                                    const Subscription& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&   hashAlg,
                           const Subscription& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'Subscription'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.appId());
        hashAppend(hashAlg, object.expression());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::Subscription)

namespace mqbconfm {

// ============
// class Domain
// ============

class Domain {
    // Configuration for a Domain using the custom Bloomberg Domain.
    // name................: name of this domain mode................: mode of
    // the queues in this domain storage.............: storage to use by queues
    // in this domain maxConsumers........: will reject if more than this
    // number of consumers tries to connect.  0 (the default) means unlimited
    // maxProducers........: will reject if more than this number of producers
    // tries to connect.  0 (the default) means unlimited maxQueues...........:
    // will reject creating more than this number of queues.  0 (the default)
    // means unlimited msgGroupIdConfig....: optional configuration for Group
    // Id routing features maxIdleTime.........: (seconds) time queue can be
    // inactive before alarming.  0 (the default) means no monitoring and
    // alarming messageTtl..........: (seconds) minimum time before which a
    // message can be discarded (i.e., it's not guaranteed that the message
    // will be discarded exactly after 'ttlSeconds' time, but it is guaranteed
    // that it will not be discarded before at least 'ttlSeconds' time
    // maxDeliveryAttempts.: maximum number of times BlazingMQ framework will
    // attempt to deliver a message to consumers before purging it from the
    // queue.  Zero (the default) means unlimited deduplicationTimeMs.:
    // timeout, in milliseconds, to keep GUID of PUT message for the purpose of
    // detecting duplicate PUTs.  consistency.........: optional consistency
    // mode.  subscriptions.......: optional Auto (Application) subscriptions

    // INSTANCE DATA
    bsls::Types::Int64                    d_messageTtl;
    bsl::vector<Subscription>             d_subscriptions;
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

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Domain& rhs) const;

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
        ATTRIBUTE_ID_CONSISTENCY           = 11,
        ATTRIBUTE_ID_SUBSCRIPTIONS         = 12
    };

    enum { NUM_ATTRIBUTES = 13 };

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
        ATTRIBUTE_INDEX_CONSISTENCY           = 11,
        ATTRIBUTE_INDEX_SUBSCRIPTIONS         = 12
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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit Domain(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Domain' having the default value.  Use the
    // optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Domain(const Domain& original, bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Domain' having the value of the specified
    // 'original' object.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Domain(Domain&& original) noexcept;
    // Create an object of type 'Domain' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.

    Domain(Domain&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Domain' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.  Use the
    // optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Domain();
    // Destroy this object.

    // MANIPULATORS
    Domain& operator=(const Domain& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Domain& operator=(Domain&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& name();
    // Return a reference to the modifiable "Name" attribute of this
    // object.

    QueueMode& mode();
    // Return a reference to the modifiable "Mode" attribute of this
    // object.

    StorageDefinition& storage();
    // Return a reference to the modifiable "Storage" attribute of this
    // object.

    int& maxConsumers();
    // Return a reference to the modifiable "MaxConsumers" attribute of
    // this object.

    int& maxProducers();
    // Return a reference to the modifiable "MaxProducers" attribute of
    // this object.

    int& maxQueues();
    // Return a reference to the modifiable "MaxQueues" attribute of this
    // object.

    bdlb::NullableValue<MsgGroupIdConfig>& msgGroupIdConfig();
    // Return a reference to the modifiable "MsgGroupIdConfig" attribute of
    // this object.

    int& maxIdleTime();
    // Return a reference to the modifiable "MaxIdleTime" attribute of this
    // object.

    bsls::Types::Int64& messageTtl();
    // Return a reference to the modifiable "MessageTtl" attribute of this
    // object.

    int& maxDeliveryAttempts();
    // Return a reference to the modifiable "MaxDeliveryAttempts" attribute
    // of this object.

    int& deduplicationTimeMs();
    // Return a reference to the modifiable "DeduplicationTimeMs" attribute
    // of this object.

    Consistency& consistency();
    // Return a reference to the modifiable "Consistency" attribute of this
    // object.

    bsl::vector<Subscription>& subscriptions();
    // Return a reference to the modifiable "Subscriptions" attribute of
    // this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& name() const;
    // Return a reference offering non-modifiable access to the "Name"
    // attribute of this object.

    const QueueMode& mode() const;
    // Return a reference offering non-modifiable access to the "Mode"
    // attribute of this object.

    const StorageDefinition& storage() const;
    // Return a reference offering non-modifiable access to the "Storage"
    // attribute of this object.

    int maxConsumers() const;
    // Return the value of the "MaxConsumers" attribute of this object.

    int maxProducers() const;
    // Return the value of the "MaxProducers" attribute of this object.

    int maxQueues() const;
    // Return the value of the "MaxQueues" attribute of this object.

    const bdlb::NullableValue<MsgGroupIdConfig>& msgGroupIdConfig() const;
    // Return a reference offering non-modifiable access to the
    // "MsgGroupIdConfig" attribute of this object.

    int maxIdleTime() const;
    // Return the value of the "MaxIdleTime" attribute of this object.

    bsls::Types::Int64 messageTtl() const;
    // Return the value of the "MessageTtl" attribute of this object.

    int maxDeliveryAttempts() const;
    // Return the value of the "MaxDeliveryAttempts" attribute of this
    // object.

    int deduplicationTimeMs() const;
    // Return the value of the "DeduplicationTimeMs" attribute of this
    // object.

    const Consistency& consistency() const;
    // Return a reference offering non-modifiable access to the
    // "Consistency" attribute of this object.

    const bsl::vector<Subscription>& subscriptions() const;
    // Return a reference offering non-modifiable access to the
    // "Subscriptions" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Domain& lhs, const Domain& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const Domain& lhs, const Domain& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const Domain& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Domain& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Domain'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbconfm::Domain)

namespace mqbconfm {

// ======================
// class DomainDefinition
// ======================

class DomainDefinition {
    // Top level type representing the information retrieved when resolving a
    // domain.
    // location..: Domain location (i.e., cluster name)  REVIEW: consider:
    // s/location/cluster/ parameters: Domain parameters
    // REVIEW: consider merging Domain into DomainDefinition

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
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit DomainDefinition(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'DomainDefinition' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    DomainDefinition(const DomainDefinition& original,
                     bslma::Allocator*       basicAllocator = 0);
    // Create an object of type 'DomainDefinition' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainDefinition(DomainDefinition&& original) noexcept;
    // Create an object of type 'DomainDefinition' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    DomainDefinition(DomainDefinition&& original,
                     bslma::Allocator*  basicAllocator);
    // Create an object of type 'DomainDefinition' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~DomainDefinition();
    // Destroy this object.

    // MANIPULATORS
    DomainDefinition& operator=(const DomainDefinition& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainDefinition& operator=(DomainDefinition&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& location();
    // Return a reference to the modifiable "Location" attribute of this
    // object.

    Domain& parameters();
    // Return a reference to the modifiable "Parameters" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& location() const;
    // Return a reference offering non-modifiable access to the "Location"
    // attribute of this object.

    const Domain& parameters() const;
    // Return a reference offering non-modifiable access to the
    // "Parameters" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DomainDefinition& lhs,
                           const DomainDefinition& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.location() == rhs.location() &&
               lhs.parameters() == rhs.parameters();
    }

    friend bool operator!=(const DomainDefinition& lhs,
                           const DomainDefinition& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const DomainDefinition& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const DomainDefinition& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DomainDefinition'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.location());
        hashAppend(hashAlg, object.parameters());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainDefinition)

namespace mqbconfm {

// ===================
// class DomainVariant
// ===================

class DomainVariant {
    // Either a Domain or a DomainRedirection.
    // definition..: The full definition of a domain redirection.: The name of
    // the domain to redirect to

    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<DomainDefinition> d_definition;
        bsls::ObjectBuffer<bsl::string>      d_redirect;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const DomainVariant& rhs) const;

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
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    explicit DomainVariant(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'DomainVariant' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    DomainVariant(const DomainVariant& original,
                  bslma::Allocator*    basicAllocator = 0);
    // Create an object of type 'DomainVariant' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainVariant(DomainVariant&& original) noexcept;
    // Create an object of type 'DomainVariant' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    DomainVariant(DomainVariant&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'DomainVariant' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~DomainVariant();
    // Destroy this object.

    // MANIPULATORS
    DomainVariant& operator=(const DomainVariant& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DomainVariant& operator=(DomainVariant&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

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

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    DomainDefinition& definition();
    // Return a reference to the modifiable "Definition" selection of this
    // object if "Definition" is the current selection.  The behavior is
    // undefined unless "Definition" is the selection of this object.

    bsl::string& redirect();
    // Return a reference to the modifiable "Redirect" selection of this
    // object if "Redirect" is the current selection.  The behavior is
    // undefined unless "Redirect" is the selection of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const DomainDefinition& definition() const;
    // Return a reference to the non-modifiable "Definition" selection of
    // this object if "Definition" is the current selection.  The behavior
    // is undefined unless "Definition" is the selection of this object.

    const bsl::string& redirect() const;
    // Return a reference to the non-modifiable "Redirect" selection of
    // this object if "Redirect" is the current selection.  The behavior is
    // undefined unless "Redirect" is the selection of this object.

    bool isDefinitionValue() const;
    // Return 'true' if the value of this object is a "Definition" value,
    // and return 'false' otherwise.

    bool isRedirectValue() const;
    // Return 'true' if the value of this object is a "Redirect" value, and
    // return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DomainVariant& lhs, const DomainVariant& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'DomainVariant' objects have the
    // same value if either the selections in both objects have the same
    // ids and the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const DomainVariant& lhs, const DomainVariant& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const DomainVariant& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const DomainVariant& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DomainVariant'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbconfm::DomainVariant)

//=============================================================================
//                          INLINE DEFINITIONS
//=============================================================================

namespace mqbconfm {

// --------------------
// class BrokerIdentity
// --------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void BrokerIdentity::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->hostName());
    hashAppend(hashAlgorithm, this->hostTags());
    hashAppend(hashAlgorithm, this->brokerVersion());
}

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

// -----------------------
// class ExpressionVersion
// -----------------------

// CLASS METHODS
inline int ExpressionVersion::fromString(Value*             result,
                                         const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream& ExpressionVersion::print(bsl::ostream&            stream,
                                              ExpressionVersion::Value value)
{
    return stream << toString(value);
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

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Limits::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->messages());
    hashAppend(hashAlgorithm, this->messagesWatermarkRatio());
    hashAppend(hashAlgorithm, this->bytes());
    hashAppend(hashAlgorithm, this->bytesWatermarkRatio());
}

inline bool Limits::isEqualTo(const Limits& rhs) const
{
    return this->messages() == rhs.messages() &&
           this->messagesWatermarkRatio() == rhs.messagesWatermarkRatio() &&
           this->bytes() == rhs.bytes() &&
           this->bytesWatermarkRatio() == rhs.bytesWatermarkRatio();
}

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

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void MsgGroupIdConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->rebalance());
    hashAppend(hashAlgorithm, this->maxGroups());
    hashAppend(hashAlgorithm, this->ttlSeconds());
}

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

    ret = manipulator(
        &d_publishAppIdMetrics,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PUBLISH_APP_ID_METRICS]);
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
    case ATTRIBUTE_ID_PUBLISH_APP_ID_METRICS: {
        return manipulator(
            &d_publishAppIdMetrics,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PUBLISH_APP_ID_METRICS]);
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

inline bool& QueueModeFanout::publishAppIdMetrics()
{
    return d_publishAppIdMetrics;
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

    ret = accessor(
        d_publishAppIdMetrics,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PUBLISH_APP_ID_METRICS]);
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
    case ATTRIBUTE_ID_PUBLISH_APP_ID_METRICS: {
        return accessor(
            d_publishAppIdMetrics,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PUBLISH_APP_ID_METRICS]);
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

inline bool QueueModeFanout::publishAppIdMetrics() const
{
    return d_publishAppIdMetrics;
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
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Consistency::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef Consistency Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_EVENTUAL:
        hashAppend(hashAlgorithm, this->eventual());
        break;
    case Class::SELECTION_ID_STRONG:
        hashAppend(hashAlgorithm, this->strong());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool Consistency::isEqualTo(const Consistency& rhs) const
{
    typedef Consistency Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_EVENTUAL:
            return this->eventual() == rhs.eventual();
        case Class::SELECTION_ID_STRONG: return this->strong() == rhs.strong();
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

// ----------------
// class Expression
// ----------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Expression::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_version,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_text, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TEXT]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Expression::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_VERSION: {
        return manipulator(&d_version,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSION]);
    }
    case ATTRIBUTE_ID_TEXT: {
        return manipulator(&d_text,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TEXT]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Expression::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline ExpressionVersion::Value& Expression::version()
{
    return d_version;
}

inline bsl::string& Expression::text()
{
    return d_text;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Expression::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_version, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_text, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TEXT]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Expression::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_VERSION: {
        return accessor(d_version,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSION]);
    }
    case ATTRIBUTE_ID_TEXT: {
        return accessor(d_text, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TEXT]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Expression::accessAttribute(t_ACCESSOR& accessor,
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

inline ExpressionVersion::Value Expression::version() const
{
    return d_version;
}

inline const bsl::string& Expression::text() const
{
    return d_text;
}

// ---------------
// class QueueMode
// ---------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void QueueMode::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef QueueMode Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_FANOUT:
        hashAppend(hashAlgorithm, this->fanout());
        break;
    case Class::SELECTION_ID_PRIORITY:
        hashAppend(hashAlgorithm, this->priority());
        break;
    case Class::SELECTION_ID_BROADCAST:
        hashAppend(hashAlgorithm, this->broadcast());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool QueueMode::isEqualTo(const QueueMode& rhs) const
{
    typedef QueueMode Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_FANOUT: return this->fanout() == rhs.fanout();
        case Class::SELECTION_ID_PRIORITY:
            return this->priority() == rhs.priority();
        case Class::SELECTION_ID_BROADCAST:
            return this->broadcast() == rhs.broadcast();
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
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Response::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef Response Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_FAILURE:
        hashAppend(hashAlgorithm, this->failure());
        break;
    case Class::SELECTION_ID_DOMAIN_CONFIG:
        hashAppend(hashAlgorithm, this->domainConfig());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool Response::isEqualTo(const Response& rhs) const
{
    typedef Response Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_FAILURE:
            return this->failure() == rhs.failure();
        case Class::SELECTION_ID_DOMAIN_CONFIG:
            return this->domainConfig() == rhs.domainConfig();
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
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Storage::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef Storage Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_IN_MEMORY:
        hashAppend(hashAlgorithm, this->inMemory());
        break;
    case Class::SELECTION_ID_FILE_BACKED:
        hashAppend(hashAlgorithm, this->fileBacked());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool Storage::isEqualTo(const Storage& rhs) const
{
    typedef Storage Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_IN_MEMORY:
            return this->inMemory() == rhs.inMemory();
        case Class::SELECTION_ID_FILE_BACKED:
            return this->fileBacked() == rhs.fileBacked();
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
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Request::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef Request Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_DOMAIN_CONFIG:
        hashAppend(hashAlgorithm, this->domainConfig());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool Request::isEqualTo(const Request& rhs) const
{
    typedef Request Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_DOMAIN_CONFIG:
            return this->domainConfig() == rhs.domainConfig();
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

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StorageDefinition::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->domainLimits());
    hashAppend(hashAlgorithm, this->queueLimits());
    hashAppend(hashAlgorithm, this->config());
}

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

// ------------------
// class Subscription
// ------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Subscription::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_appId, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_expression,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Subscription::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_APP_ID: {
        return manipulator(&d_appId,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID]);
    }
    case ATTRIBUTE_ID_EXPRESSION: {
        return manipulator(&d_expression,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Subscription::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& Subscription::appId()
{
    return d_appId;
}

inline Expression& Subscription::expression()
{
    return d_expression;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Subscription::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_appId, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_expression,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Subscription::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_APP_ID: {
        return accessor(d_appId, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID]);
    }
    case ATTRIBUTE_ID_EXPRESSION: {
        return accessor(d_expression,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Subscription::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& Subscription::appId() const
{
    return d_appId;
}

inline const Expression& Subscription::expression() const
{
    return d_expression;
}

// ------------
// class Domain
// ------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Domain::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->name());
    hashAppend(hashAlgorithm, this->mode());
    hashAppend(hashAlgorithm, this->storage());
    hashAppend(hashAlgorithm, this->maxConsumers());
    hashAppend(hashAlgorithm, this->maxProducers());
    hashAppend(hashAlgorithm, this->maxQueues());
    hashAppend(hashAlgorithm, this->msgGroupIdConfig());
    hashAppend(hashAlgorithm, this->maxIdleTime());
    hashAppend(hashAlgorithm, this->messageTtl());
    hashAppend(hashAlgorithm, this->maxDeliveryAttempts());
    hashAppend(hashAlgorithm, this->deduplicationTimeMs());
    hashAppend(hashAlgorithm, this->consistency());
    hashAppend(hashAlgorithm, this->subscriptions());
}

inline bool Domain::isEqualTo(const Domain& rhs) const
{
    return this->name() == rhs.name() && this->mode() == rhs.mode() &&
           this->storage() == rhs.storage() &&
           this->maxConsumers() == rhs.maxConsumers() &&
           this->maxProducers() == rhs.maxProducers() &&
           this->maxQueues() == rhs.maxQueues() &&
           this->msgGroupIdConfig() == rhs.msgGroupIdConfig() &&
           this->maxIdleTime() == rhs.maxIdleTime() &&
           this->messageTtl() == rhs.messageTtl() &&
           this->maxDeliveryAttempts() == rhs.maxDeliveryAttempts() &&
           this->deduplicationTimeMs() == rhs.deduplicationTimeMs() &&
           this->consistency() == rhs.consistency() &&
           this->subscriptions() == rhs.subscriptions();
}

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

    ret = manipulator(&d_subscriptions,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
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
    case ATTRIBUTE_ID_SUBSCRIPTIONS: {
        return manipulator(
            &d_subscriptions,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
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

inline bsl::vector<Subscription>& Domain::subscriptions()
{
    return d_subscriptions;
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

    ret = accessor(d_subscriptions,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
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
    case ATTRIBUTE_ID_SUBSCRIPTIONS: {
        return accessor(d_subscriptions,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
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

inline const bsl::vector<Subscription>& Domain::subscriptions() const
{
    return d_subscriptions;
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
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void DomainVariant::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef DomainVariant Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_DEFINITION:
        hashAppend(hashAlgorithm, this->definition());
        break;
    case Class::SELECTION_ID_REDIRECT:
        hashAppend(hashAlgorithm, this->redirect());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool DomainVariant::isEqualTo(const DomainVariant& rhs) const
{
    typedef DomainVariant Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_DEFINITION:
            return this->definition() == rhs.definition();
        case Class::SELECTION_ID_REDIRECT:
            return this->redirect() == rhs.redirect();
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

}  // close enterprise namespace
#endif

// GENERATED BY @BLP_BAS_CODEGEN_VERSION@
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbconfm --msgComponent messages mqbconf.xsd SERVICE
// VERSION bmqconf:183474-1.0
// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright 2024 Bloomberg Finance L.P. All rights reserved.
//      Property of Bloomberg Finance L.P. (BFLP)
//      This software is made available solely pursuant to the
//      terms of a BFLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
