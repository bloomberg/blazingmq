// Copyright 2018-2024 Bloomberg Finance L.P.
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

// mqbcfg_messages.h             *DO NOT EDIT*             @generated -*-C++-*-
#ifndef INCLUDED_MQBCFG_MESSAGES
#define INCLUDED_MQBCFG_MESSAGES

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

namespace mqbcfg {
class BmqconfConfig;
}
namespace mqbcfg {
class ClusterAttributes;
}
namespace mqbcfg {
class ClusterMonitorConfig;
}
namespace mqbcfg {
class DispatcherProcessorParameters;
}
namespace mqbcfg {
class ElectorConfig;
}
namespace mqbcfg {
class Heartbeat;
}
namespace mqbcfg {
class MessagePropertiesV2;
}
namespace mqbcfg {
class MessageThrottleConfig;
}
namespace mqbcfg {
class Plugins;
}
namespace mqbcfg {
class QueueOperationsConfig;
}
namespace mqbcfg {
class ResolvedDomain;
}
namespace mqbcfg {
class StatsPrinterConfig;
}
namespace mqbcfg {
class StorageSyncConfig;
}
namespace mqbcfg {
class SyslogConfig;
}
namespace mqbcfg {
class TcpClusterNodeConnection;
}
namespace mqbcfg {
class TcpInterfaceConfig;
}
namespace mqbcfg {
class VirtualClusterInformation;
}
namespace mqbcfg {
class ClusterNodeConnection;
}
namespace mqbcfg {
class DispatcherProcessorConfig;
}
namespace mqbcfg {
class LogController;
}
namespace mqbcfg {
class NetworkInterfaces;
}
namespace mqbcfg {
class PartitionConfig;
}
namespace mqbcfg {
class StatPluginConfigPrometheus;
}
namespace mqbcfg {
class ClusterNode;
}
namespace mqbcfg {
class DispatcherConfig;
}
namespace mqbcfg {
class ReversedClusterConnection;
}
namespace mqbcfg {
class StatPluginConfig;
}
namespace mqbcfg {
class TaskConfig;
}
namespace mqbcfg {
class ClusterDefinition;
}
namespace mqbcfg {
class ClusterProxyDefinition;
}
namespace mqbcfg {
class StatsConfig;
}
namespace mqbcfg {
class AppConfig;
}
namespace mqbcfg {
class ClustersDefinition;
}
namespace mqbcfg {
class Configuration;
}
namespace mqbcfg {

// ===================
// class AllocatorType
// ===================

struct AllocatorType {
  public:
    // TYPES
    enum Value { NEWDELETE = 0, COUNTING = 1, STACKTRACETEST = 2 };

    enum { NUM_ENUMERATORS = 3 };

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
        return AllocatorType::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mqbcfg::AllocatorType)

namespace mqbcfg {

// ===================
// class BmqconfConfig
// ===================

class BmqconfConfig {
    // INSTANCE DATA
    int d_cacheTTLSeconds;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_CACHE_T_T_L_SECONDS = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_CACHE_T_T_L_SECONDS = 0 };

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
    BmqconfConfig();
    // Create an object of type 'BmqconfConfig' having the default value.

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

    int& cacheTTLSeconds();
    // Return a reference to the modifiable "CacheTTLSeconds" attribute of
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

    int cacheTTLSeconds() const;
    // Return the value of the "CacheTTLSeconds" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const BmqconfConfig& lhs, const BmqconfConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.cacheTTLSeconds() == rhs.cacheTTLSeconds();
    }

    friend bool operator!=(const BmqconfConfig& lhs, const BmqconfConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const BmqconfConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const BmqconfConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'BmqconfConfig'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.cacheTTLSeconds());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::BmqconfConfig)

namespace mqbcfg {

// =======================
// class ClusterAttributes
// =======================

class ClusterAttributes {
    // Type representing the attributes specific to a cluster.
    // isCSLModeEnabled.: indicates if CSL is enabled for this cluster
    // isFSMWorkflow....: indicates if CSL FSM workflow is enabled for this
    // cluster.  This flag *must* be false if 'isCSLModeEnabled' is false.

    // INSTANCE DATA
    bool d_isCSLModeEnabled;
    bool d_isFSMWorkflow;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_IS_C_S_L_MODE_ENABLED = 0,
        ATTRIBUTE_ID_IS_F_S_M_WORKFLOW     = 1
    };

    enum { NUM_ATTRIBUTES = 2 };

    enum {
        ATTRIBUTE_INDEX_IS_C_S_L_MODE_ENABLED = 0,
        ATTRIBUTE_INDEX_IS_F_S_M_WORKFLOW     = 1
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_IS_C_S_L_MODE_ENABLED;

    static const bool DEFAULT_INITIALIZER_IS_F_S_M_WORKFLOW;

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
    ClusterAttributes();
    // Create an object of type 'ClusterAttributes' having the default
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

    bool& isCSLModeEnabled();
    // Return a reference to the modifiable "IsCSLModeEnabled" attribute of
    // this object.

    bool& isFSMWorkflow();
    // Return a reference to the modifiable "IsFSMWorkflow" attribute of
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

    bool isCSLModeEnabled() const;
    // Return the value of the "IsCSLModeEnabled" attribute of this object.

    bool isFSMWorkflow() const;
    // Return the value of the "IsFSMWorkflow" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ClusterAttributes& lhs,
                           const ClusterAttributes& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isCSLModeEnabled() == rhs.isCSLModeEnabled() &&
               lhs.isFSMWorkflow() == rhs.isFSMWorkflow();
    }

    friend bool operator!=(const ClusterAttributes& lhs,
                           const ClusterAttributes& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const ClusterAttributes& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const ClusterAttributes& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ClusterAttributes'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.isCSLModeEnabled());
        hashAppend(hashAlg, object.isFSMWorkflow());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::ClusterAttributes)

namespace mqbcfg {

// ==========================
// class ClusterMonitorConfig
// ==========================

class ClusterMonitorConfig {
    // Type representing the configuration for cluster state monitor.
    // maxTimeLeader......: Time (in seconds) before alarming that the
    // cluster's leader is not 'active' maxTimeMaster......: Time (in seconds)
    // before alarming that a partition's master is not 'active'
    // maxTimeNode........: Time (in seconds) before alarming that a node is
    // not 'available' maxTimeFailover..: Time (in seconds) before alarming
    // that failover hasn't completed thresholdLeader....: Time (in seconds)
    // before first notifying observers that cluster's leader is not 'active'.
    // This time interval is smaller than 'maxTimeLeader' because observing
    // components may attempt to heal the cluster state before an alarm is
    // raised.  thresholdMaster....: Time (in seconds) before notifying
    // observers that a partition's master is not 'active'.  This time interval
    // is smaller than 'maxTimeMaster' because observing components may attempt
    // to heal the cluster state before an alarm is raised.
    // thresholdNode......: Time (in seconds) before notifying observers that a
    // node is not 'available'.  This time interval is smaller than
    // 'maxTimeNode' because observing components may attempt to heal the
    // cluster state before an alarm is raised.  thresholdFailover..: Time (in
    // seconds) before notifying observers that failover has not completed.
    // This time interval is smaller than 'maxTimeFailover' because observing
    // components may attempt to fix the issue before an alarm is raised.

    // INSTANCE DATA
    int d_maxTimeLeader;
    int d_maxTimeMaster;
    int d_maxTimeNode;
    int d_maxTimeFailover;
    int d_thresholdLeader;
    int d_thresholdMaster;
    int d_thresholdNode;
    int d_thresholdFailover;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const ClusterMonitorConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_MAX_TIME_LEADER    = 0,
        ATTRIBUTE_ID_MAX_TIME_MASTER    = 1,
        ATTRIBUTE_ID_MAX_TIME_NODE      = 2,
        ATTRIBUTE_ID_MAX_TIME_FAILOVER  = 3,
        ATTRIBUTE_ID_THRESHOLD_LEADER   = 4,
        ATTRIBUTE_ID_THRESHOLD_MASTER   = 5,
        ATTRIBUTE_ID_THRESHOLD_NODE     = 6,
        ATTRIBUTE_ID_THRESHOLD_FAILOVER = 7
    };

    enum { NUM_ATTRIBUTES = 8 };

    enum {
        ATTRIBUTE_INDEX_MAX_TIME_LEADER    = 0,
        ATTRIBUTE_INDEX_MAX_TIME_MASTER    = 1,
        ATTRIBUTE_INDEX_MAX_TIME_NODE      = 2,
        ATTRIBUTE_INDEX_MAX_TIME_FAILOVER  = 3,
        ATTRIBUTE_INDEX_THRESHOLD_LEADER   = 4,
        ATTRIBUTE_INDEX_THRESHOLD_MASTER   = 5,
        ATTRIBUTE_INDEX_THRESHOLD_NODE     = 6,
        ATTRIBUTE_INDEX_THRESHOLD_FAILOVER = 7
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_MAX_TIME_LEADER;

    static const int DEFAULT_INITIALIZER_MAX_TIME_MASTER;

    static const int DEFAULT_INITIALIZER_MAX_TIME_NODE;

    static const int DEFAULT_INITIALIZER_MAX_TIME_FAILOVER;

    static const int DEFAULT_INITIALIZER_THRESHOLD_LEADER;

    static const int DEFAULT_INITIALIZER_THRESHOLD_MASTER;

    static const int DEFAULT_INITIALIZER_THRESHOLD_NODE;

    static const int DEFAULT_INITIALIZER_THRESHOLD_FAILOVER;

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
    ClusterMonitorConfig();
    // Create an object of type 'ClusterMonitorConfig' having the default
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

    int& maxTimeLeader();
    // Return a reference to the modifiable "MaxTimeLeader" attribute of
    // this object.

    int& maxTimeMaster();
    // Return a reference to the modifiable "MaxTimeMaster" attribute of
    // this object.

    int& maxTimeNode();
    // Return a reference to the modifiable "MaxTimeNode" attribute of this
    // object.

    int& maxTimeFailover();
    // Return a reference to the modifiable "MaxTimeFailover" attribute of
    // this object.

    int& thresholdLeader();
    // Return a reference to the modifiable "ThresholdLeader" attribute of
    // this object.

    int& thresholdMaster();
    // Return a reference to the modifiable "ThresholdMaster" attribute of
    // this object.

    int& thresholdNode();
    // Return a reference to the modifiable "ThresholdNode" attribute of
    // this object.

    int& thresholdFailover();
    // Return a reference to the modifiable "ThresholdFailover" attribute
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

    int maxTimeLeader() const;
    // Return the value of the "MaxTimeLeader" attribute of this object.

    int maxTimeMaster() const;
    // Return the value of the "MaxTimeMaster" attribute of this object.

    int maxTimeNode() const;
    // Return the value of the "MaxTimeNode" attribute of this object.

    int maxTimeFailover() const;
    // Return the value of the "MaxTimeFailover" attribute of this object.

    int thresholdLeader() const;
    // Return the value of the "ThresholdLeader" attribute of this object.

    int thresholdMaster() const;
    // Return the value of the "ThresholdMaster" attribute of this object.

    int thresholdNode() const;
    // Return the value of the "ThresholdNode" attribute of this object.

    int thresholdFailover() const;
    // Return the value of the "ThresholdFailover" attribute of this
    // object.

    // HIDDEN FRIENDS
    friend bool operator==(const ClusterMonitorConfig& lhs,
                           const ClusterMonitorConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const ClusterMonitorConfig& lhs,
                           const ClusterMonitorConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&               stream,
                                    const ClusterMonitorConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&           hashAlg,
                           const ClusterMonitorConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ClusterMonitorConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::ClusterMonitorConfig)

namespace mqbcfg {

// ===================================
// class DispatcherProcessorParameters
// ===================================

class DispatcherProcessorParameters {
    // INSTANCE DATA
    int d_queueSize;
    int d_queueSizeLowWatermark;
    int d_queueSizeHighWatermark;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_QUEUE_SIZE                = 0,
        ATTRIBUTE_ID_QUEUE_SIZE_LOW_WATERMARK  = 1,
        ATTRIBUTE_ID_QUEUE_SIZE_HIGH_WATERMARK = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_QUEUE_SIZE                = 0,
        ATTRIBUTE_INDEX_QUEUE_SIZE_LOW_WATERMARK  = 1,
        ATTRIBUTE_INDEX_QUEUE_SIZE_HIGH_WATERMARK = 2
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
    DispatcherProcessorParameters();
    // Create an object of type 'DispatcherProcessorParameters' having the
    // default value.

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

    int& queueSize();
    // Return a reference to the modifiable "QueueSize" attribute of this
    // object.

    int& queueSizeLowWatermark();
    // Return a reference to the modifiable "QueueSizeLowWatermark"
    // attribute of this object.

    int& queueSizeHighWatermark();
    // Return a reference to the modifiable "QueueSizeHighWatermark"
    // attribute of this object.

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

    int queueSize() const;
    // Return the value of the "QueueSize" attribute of this object.

    int queueSizeLowWatermark() const;
    // Return the value of the "QueueSizeLowWatermark" attribute of this
    // object.

    int queueSizeHighWatermark() const;
    // Return the value of the "QueueSizeHighWatermark" attribute of this
    // object.

    // HIDDEN FRIENDS
    friend bool operator==(const DispatcherProcessorParameters& lhs,
                           const DispatcherProcessorParameters& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.queueSize() == rhs.queueSize() &&
               lhs.queueSizeLowWatermark() == rhs.queueSizeLowWatermark() &&
               lhs.queueSizeHighWatermark() == rhs.queueSizeHighWatermark();
    }

    friend bool operator!=(const DispatcherProcessorParameters& lhs,
                           const DispatcherProcessorParameters& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream,
                                    const DispatcherProcessorParameters& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                    hashAlg,
                           const DispatcherProcessorParameters& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DispatcherProcessorParameters'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(
    mqbcfg::DispatcherProcessorParameters)

namespace mqbcfg {

// ===================
// class ElectorConfig
// ===================

class ElectorConfig {
    // Type representing the configuration for leader election amongst a
    // cluster of nodes.
    // initialWaitTimeoutMs.......: initial wait timeout, in milliseconds, of a
    // follower for leader heartbeat before initiating election, as per the
    // *Raft* Algorithm.  Note that `initialWaitTimeoutMs` should be larger
    // than `maxRandomWaitTimeoutMs` maxRandomWaitTimeoutMs.....: maximum
    // random wait timeout, in milliseconds, of a follower for leader heartbeat
    // before initiating election, as per the *Raft* Algorithm
    // scoutingResultTimeoutMs....: timeout, in milliseconds, of a follower for
    // awaiting scouting responses from all nodes after sending scouting
    // request.  electionResultTimeoutMs....: timeout, in milliseconds, of a
    // candidate for awaiting quorum to be reached after proposing election, as
    // per the *Raft* Algorithm heartbeatBroadcastPeriodMs.: frequency, in
    // milliseconds, in which the leader broadcasts a heartbeat signal, as per
    // the *Raft* Algorithm, heartbeatCheckPeriodMs.....: frequency, in
    // milliseconds, in which a follower checks for heartbeat signals from the
    // leader, as per the *Raft* Algorithm heartbeatMissCount.........: the
    // number of missed heartbeat signals required before a follower marks the
    // current leader as inactive, as per the *Raft* Algorithm
    // quorum.....................: the minimum number of votes required for a
    // candidate to transition to the leader.  If zero, dynamically set to half
    // the number of nodes plus one leaderSyncDelayMs..........: delay, in
    // milliseconds, after a leader has been elected before initiating leader
    // sync, in order to give a chance to all nodes to come up and declare
    // themselves AVAILABLE.  Note that this should be done only in case of
    // cluster of size > 1

    // INSTANCE DATA
    int d_initialWaitTimeoutMs;
    int d_maxRandomWaitTimeoutMs;
    int d_scoutingResultTimeoutMs;
    int d_electionResultTimeoutMs;
    int d_heartbeatBroadcastPeriodMs;
    int d_heartbeatCheckPeriodMs;
    int d_heartbeatMissCount;
    int d_quorum;
    int d_leaderSyncDelayMs;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const ElectorConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_INITIAL_WAIT_TIMEOUT_MS       = 0,
        ATTRIBUTE_ID_MAX_RANDOM_WAIT_TIMEOUT_MS    = 1,
        ATTRIBUTE_ID_SCOUTING_RESULT_TIMEOUT_MS    = 2,
        ATTRIBUTE_ID_ELECTION_RESULT_TIMEOUT_MS    = 3,
        ATTRIBUTE_ID_HEARTBEAT_BROADCAST_PERIOD_MS = 4,
        ATTRIBUTE_ID_HEARTBEAT_CHECK_PERIOD_MS     = 5,
        ATTRIBUTE_ID_HEARTBEAT_MISS_COUNT          = 6,
        ATTRIBUTE_ID_QUORUM                        = 7,
        ATTRIBUTE_ID_LEADER_SYNC_DELAY_MS          = 8
    };

    enum { NUM_ATTRIBUTES = 9 };

    enum {
        ATTRIBUTE_INDEX_INITIAL_WAIT_TIMEOUT_MS       = 0,
        ATTRIBUTE_INDEX_MAX_RANDOM_WAIT_TIMEOUT_MS    = 1,
        ATTRIBUTE_INDEX_SCOUTING_RESULT_TIMEOUT_MS    = 2,
        ATTRIBUTE_INDEX_ELECTION_RESULT_TIMEOUT_MS    = 3,
        ATTRIBUTE_INDEX_HEARTBEAT_BROADCAST_PERIOD_MS = 4,
        ATTRIBUTE_INDEX_HEARTBEAT_CHECK_PERIOD_MS     = 5,
        ATTRIBUTE_INDEX_HEARTBEAT_MISS_COUNT          = 6,
        ATTRIBUTE_INDEX_QUORUM                        = 7,
        ATTRIBUTE_INDEX_LEADER_SYNC_DELAY_MS          = 8
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_INITIAL_WAIT_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_MAX_RANDOM_WAIT_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_SCOUTING_RESULT_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_ELECTION_RESULT_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_HEARTBEAT_BROADCAST_PERIOD_MS;

    static const int DEFAULT_INITIALIZER_HEARTBEAT_CHECK_PERIOD_MS;

    static const int DEFAULT_INITIALIZER_HEARTBEAT_MISS_COUNT;

    static const int DEFAULT_INITIALIZER_QUORUM;

    static const int DEFAULT_INITIALIZER_LEADER_SYNC_DELAY_MS;

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
    ElectorConfig();
    // Create an object of type 'ElectorConfig' having the default value.

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

    int& initialWaitTimeoutMs();
    // Return a reference to the modifiable "InitialWaitTimeoutMs"
    // attribute of this object.

    int& maxRandomWaitTimeoutMs();
    // Return a reference to the modifiable "MaxRandomWaitTimeoutMs"
    // attribute of this object.

    int& scoutingResultTimeoutMs();
    // Return a reference to the modifiable "ScoutingResultTimeoutMs"
    // attribute of this object.

    int& electionResultTimeoutMs();
    // Return a reference to the modifiable "ElectionResultTimeoutMs"
    // attribute of this object.

    int& heartbeatBroadcastPeriodMs();
    // Return a reference to the modifiable "HeartbeatBroadcastPeriodMs"
    // attribute of this object.

    int& heartbeatCheckPeriodMs();
    // Return a reference to the modifiable "HeartbeatCheckPeriodMs"
    // attribute of this object.

    int& heartbeatMissCount();
    // Return a reference to the modifiable "HeartbeatMissCount" attribute
    // of this object.

    int& quorum();
    // Return a reference to the modifiable "Quorum" attribute of this
    // object.

    int& leaderSyncDelayMs();
    // Return a reference to the modifiable "LeaderSyncDelayMs" attribute
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

    int initialWaitTimeoutMs() const;
    // Return the value of the "InitialWaitTimeoutMs" attribute of this
    // object.

    int maxRandomWaitTimeoutMs() const;
    // Return the value of the "MaxRandomWaitTimeoutMs" attribute of this
    // object.

    int scoutingResultTimeoutMs() const;
    // Return the value of the "ScoutingResultTimeoutMs" attribute of this
    // object.

    int electionResultTimeoutMs() const;
    // Return the value of the "ElectionResultTimeoutMs" attribute of this
    // object.

    int heartbeatBroadcastPeriodMs() const;
    // Return the value of the "HeartbeatBroadcastPeriodMs" attribute of
    // this object.

    int heartbeatCheckPeriodMs() const;
    // Return the value of the "HeartbeatCheckPeriodMs" attribute of this
    // object.

    int heartbeatMissCount() const;
    // Return the value of the "HeartbeatMissCount" attribute of this
    // object.

    int quorum() const;
    // Return the value of the "Quorum" attribute of this object.

    int leaderSyncDelayMs() const;
    // Return the value of the "LeaderSyncDelayMs" attribute of this
    // object.

    // HIDDEN FRIENDS
    friend bool operator==(const ElectorConfig& lhs, const ElectorConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const ElectorConfig& lhs, const ElectorConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const ElectorConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const ElectorConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ElectorConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::ElectorConfig)

namespace mqbcfg {

// ================
// class ExportMode
// ================

struct ExportMode {
  public:
    // TYPES
    enum Value { E_PUSH = 0, E_PULL = 1 };

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
        return ExportMode::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mqbcfg::ExportMode)

namespace mqbcfg {

// ===============
// class Heartbeat
// ===============

class Heartbeat {
    // The following parameters define, for the various connection types, after
    // how many missed heartbeats the connection should be proactively
    // resetted.  Note that a value of 0 means that smart-heartbeat is entirely
    // disabled for this kind of connection (i.e., it will not periodically
    // emit heatbeats in case no traffic is received, and will therefore not
    // quickly detect stale remote peer).  Each value is in multiple of the
    // 'NetworkInterfaces/TCPInterfaceConfig/heartIntervalMs'.
    // client............: The channel represents a client connected to the
    // broker downstreamBroker..: The channel represents a downstream broker
    // connected to this broker, i.e.  a proxy.  upstreamBroker....: The
    // channel represents an upstream broker connection from this broker, i.e.
    // a cluster proxy connection.  clusterPeer.......: The channel represents
    // a connection with a peer node in the cluster this broker is part of.

    // INSTANCE DATA
    int d_client;
    int d_downstreamBroker;
    int d_upstreamBroker;
    int d_clusterPeer;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Heartbeat& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_CLIENT            = 0,
        ATTRIBUTE_ID_DOWNSTREAM_BROKER = 1,
        ATTRIBUTE_ID_UPSTREAM_BROKER   = 2,
        ATTRIBUTE_ID_CLUSTER_PEER      = 3
    };

    enum { NUM_ATTRIBUTES = 4 };

    enum {
        ATTRIBUTE_INDEX_CLIENT            = 0,
        ATTRIBUTE_INDEX_DOWNSTREAM_BROKER = 1,
        ATTRIBUTE_INDEX_UPSTREAM_BROKER   = 2,
        ATTRIBUTE_INDEX_CLUSTER_PEER      = 3
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_CLIENT;

    static const int DEFAULT_INITIALIZER_DOWNSTREAM_BROKER;

    static const int DEFAULT_INITIALIZER_UPSTREAM_BROKER;

    static const int DEFAULT_INITIALIZER_CLUSTER_PEER;

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
    Heartbeat();
    // Create an object of type 'Heartbeat' having the default value.

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

    int& client();
    // Return a reference to the modifiable "Client" attribute of this
    // object.

    int& downstreamBroker();
    // Return a reference to the modifiable "DownstreamBroker" attribute of
    // this object.

    int& upstreamBroker();
    // Return a reference to the modifiable "UpstreamBroker" attribute of
    // this object.

    int& clusterPeer();
    // Return a reference to the modifiable "ClusterPeer" attribute of this
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

    int client() const;
    // Return the value of the "Client" attribute of this object.

    int downstreamBroker() const;
    // Return the value of the "DownstreamBroker" attribute of this object.

    int upstreamBroker() const;
    // Return the value of the "UpstreamBroker" attribute of this object.

    int clusterPeer() const;
    // Return the value of the "ClusterPeer" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Heartbeat& lhs, const Heartbeat& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const Heartbeat& lhs, const Heartbeat& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const Heartbeat& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Heartbeat& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Heartbeat'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::Heartbeat)

namespace mqbcfg {

// ===============================
// class MasterAssignmentAlgorithm
// ===============================

struct MasterAssignmentAlgorithm {
    // Enumeration of the various algorithm's used for assigning a master to a
    // partition: - E_LEADER_IS_MASTER_ALL: the leader is master for all
    // partitions - E_LEAST_ASSIGNED:       the active node with the least
    // number of partitions assigned is used

  public:
    // TYPES
    enum Value { E_LEADER_IS_MASTER_ALL = 0, E_LEAST_ASSIGNED = 1 };

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
        return MasterAssignmentAlgorithm::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mqbcfg::MasterAssignmentAlgorithm)

namespace mqbcfg {

// =========================
// class MessagePropertiesV2
// =========================

class MessagePropertiesV2 {
    // This complex type captures information which can be used to tell a
    // broker if it should advertise support for message properties v2 format
    // (also knownn as extended or 'EX' message properties at some places).
    // Additionally, broker can be configured to advertise this feature only to
    // those C++ and Java clients which match a certain minimum SDK version.

    // INSTANCE DATA
    int  d_minCppSdkVersion;
    int  d_minJavaSdkVersion;
    bool d_advertiseV2Support;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_ADVERTISE_V2_SUPPORT = 0,
        ATTRIBUTE_ID_MIN_CPP_SDK_VERSION  = 1,
        ATTRIBUTE_ID_MIN_JAVA_SDK_VERSION = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_ADVERTISE_V2_SUPPORT = 0,
        ATTRIBUTE_INDEX_MIN_CPP_SDK_VERSION  = 1,
        ATTRIBUTE_INDEX_MIN_JAVA_SDK_VERSION = 2
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_ADVERTISE_V2_SUPPORT;

    static const int DEFAULT_INITIALIZER_MIN_CPP_SDK_VERSION;

    static const int DEFAULT_INITIALIZER_MIN_JAVA_SDK_VERSION;

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
    MessagePropertiesV2();
    // Create an object of type 'MessagePropertiesV2' having the default
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

    bool& advertiseV2Support();
    // Return a reference to the modifiable "AdvertiseV2Support" attribute
    // of this object.

    int& minCppSdkVersion();
    // Return a reference to the modifiable "MinCppSdkVersion" attribute of
    // this object.

    int& minJavaSdkVersion();
    // Return a reference to the modifiable "MinJavaSdkVersion" attribute
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

    bool advertiseV2Support() const;
    // Return the value of the "AdvertiseV2Support" attribute of this
    // object.

    int minCppSdkVersion() const;
    // Return the value of the "MinCppSdkVersion" attribute of this object.

    int minJavaSdkVersion() const;
    // Return the value of the "MinJavaSdkVersion" attribute of this
    // object.

    // HIDDEN FRIENDS
    friend bool operator==(const MessagePropertiesV2& lhs,
                           const MessagePropertiesV2& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.advertiseV2Support() == rhs.advertiseV2Support() &&
               lhs.minCppSdkVersion() == rhs.minCppSdkVersion() &&
               lhs.minJavaSdkVersion() == rhs.minJavaSdkVersion();
    }

    friend bool operator!=(const MessagePropertiesV2& lhs,
                           const MessagePropertiesV2& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&              stream,
                                    const MessagePropertiesV2& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&          hashAlg,
                           const MessagePropertiesV2& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'MessagePropertiesV2'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::MessagePropertiesV2)

namespace mqbcfg {

// ===========================
// class MessageThrottleConfig
// ===========================

class MessageThrottleConfig {
    // Configuration values for message throttling intervals and thresholds.
    // lowInterval...: time in milliseconds.
    // highInterval..: time in milliseconds.
    // lowThreshold..: indicates the rda counter value at which we start
    // throttlling for time equal to 'lowInterval'.
    // highThreshold.: indicates the rda counter value at which we start
    // throttlling for time equal to 'highInterval'.
    // Note: lowInterval should be less than/equal to highInterval,
    // lowThreshold should be less than highThreshold.

    // INSTANCE DATA
    unsigned int d_lowThreshold;
    unsigned int d_highThreshold;
    unsigned int d_lowInterval;
    unsigned int d_highInterval;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const MessageThrottleConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_LOW_THRESHOLD  = 0,
        ATTRIBUTE_ID_HIGH_THRESHOLD = 1,
        ATTRIBUTE_ID_LOW_INTERVAL   = 2,
        ATTRIBUTE_ID_HIGH_INTERVAL  = 3
    };

    enum { NUM_ATTRIBUTES = 4 };

    enum {
        ATTRIBUTE_INDEX_LOW_THRESHOLD  = 0,
        ATTRIBUTE_INDEX_HIGH_THRESHOLD = 1,
        ATTRIBUTE_INDEX_LOW_INTERVAL   = 2,
        ATTRIBUTE_INDEX_HIGH_INTERVAL  = 3
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const unsigned int DEFAULT_INITIALIZER_LOW_THRESHOLD;

    static const unsigned int DEFAULT_INITIALIZER_HIGH_THRESHOLD;

    static const unsigned int DEFAULT_INITIALIZER_LOW_INTERVAL;

    static const unsigned int DEFAULT_INITIALIZER_HIGH_INTERVAL;

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
    MessageThrottleConfig();
    // Create an object of type 'MessageThrottleConfig' having the default
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

    unsigned int& lowThreshold();
    // Return a reference to the modifiable "LowThreshold" attribute of
    // this object.

    unsigned int& highThreshold();
    // Return a reference to the modifiable "HighThreshold" attribute of
    // this object.

    unsigned int& lowInterval();
    // Return a reference to the modifiable "LowInterval" attribute of this
    // object.

    unsigned int& highInterval();
    // Return a reference to the modifiable "HighInterval" attribute of
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

    unsigned int lowThreshold() const;
    // Return the value of the "LowThreshold" attribute of this object.

    unsigned int highThreshold() const;
    // Return the value of the "HighThreshold" attribute of this object.

    unsigned int lowInterval() const;
    // Return the value of the "LowInterval" attribute of this object.

    unsigned int highInterval() const;
    // Return the value of the "HighInterval" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const MessageThrottleConfig& lhs,
                           const MessageThrottleConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const MessageThrottleConfig& lhs,
                           const MessageThrottleConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const MessageThrottleConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const MessageThrottleConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'MessageThrottleConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::MessageThrottleConfig)

namespace mqbcfg {

// =============
// class Plugins
// =============

class Plugins {
    // INSTANCE DATA
    bsl::vector<bsl::string> d_libraries;
    bsl::vector<bsl::string> d_enabled;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_LIBRARIES = 0, ATTRIBUTE_ID_ENABLED = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_LIBRARIES = 0, ATTRIBUTE_INDEX_ENABLED = 1 };

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
    explicit Plugins(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Plugins' having the default value.  Use
    // the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Plugins(const Plugins& original, bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Plugins' having the value of the specified
    // 'original' object.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Plugins(Plugins&& original) noexcept;
    // Create an object of type 'Plugins' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.

    Plugins(Plugins&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Plugins' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.  Use the
    // optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Plugins();
    // Destroy this object.

    // MANIPULATORS
    Plugins& operator=(const Plugins& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Plugins& operator=(Plugins&& rhs);
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

    bsl::vector<bsl::string>& libraries();
    // Return a reference to the modifiable "Libraries" attribute of this
    // object.

    bsl::vector<bsl::string>& enabled();
    // Return a reference to the modifiable "Enabled" attribute of this
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

    const bsl::vector<bsl::string>& libraries() const;
    // Return a reference offering non-modifiable access to the "Libraries"
    // attribute of this object.

    const bsl::vector<bsl::string>& enabled() const;
    // Return a reference offering non-modifiable access to the "Enabled"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Plugins& lhs, const Plugins& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.libraries() == rhs.libraries() &&
               lhs.enabled() == rhs.enabled();
    }

    friend bool operator!=(const Plugins& lhs, const Plugins& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const Plugins& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Plugins& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Plugins'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.libraries());
        hashAppend(hashAlg, object.enabled());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::Plugins)

namespace mqbcfg {

// ===========================
// class QueueOperationsConfig
// ===========================

class QueueOperationsConfig {
    // Type representing the configuration for queue operations on a cluster.
    // openTimeoutMs..............: timeout, in milliseconds, to use when
    // opening a queue.  An open request requires some co-ordination among the
    // nodes (queue assignment request/response, followed by queue open
    // request/response, etc) configureTimeoutMs.........: timeout, in
    // milliseconds, to use for a configure queue request.  Note that
    // `configureTimeoutMs` must be less than or equal to `closeTimeoutMs` to
    // prevent out-of-order processing of closeQueue (e.g.  closeQueue sent
    // after configureQueue but timeout response processed first for the
    // closeQueue) closeTimeoutMs.............: timeout, in milliseconds, to
    // use for a close queue request reopenTimeoutMs............: timeout, in
    // milliseconds, to use when sending a reopen-queue request.  Ideally, we
    // should use same value as `openTimeoutMs`, but we are using a very large
    // value as a workaround: during network outages, a proxy or a replica may
    // failover to a new upstream node, which itself may be out of sync or not
    // yet ready.  Eventually, the reopen-queue requests sent during failover
    // may timeout, and will never be retried, leading to a 'permanent' failure
    // (client consumer app stops receiving messages; PUT messages from client
    // producer app starts getting NAK'd or buffered).  Using such a large
    // timeout value helps in a situation when network outages or its
    // after-effects are fixed after a few hours).
    // reopenRetryIntervalMs......: duration, in milliseconds, after which a
    // retry attempt should be made to reopen the queue
    // reopenMaxAttempts..........: maximum number of attempts to reopen a
    // queue when restoring the state in a proxy upon getting a new active node
    // notification assignmentTimeoutMs........: timeout, in milliseconds, to
    // use for a queue assignment request keepaliveDurationMs........:
    // duration, in milliseconds, to keep a queue alive after it has met the
    // criteria for deletion consumptionMonitorPeriodMs.: frequency, in
    // milliseconds, in which the consumption monitor checks queue consumption
    // statistics on the cluster stopTimeoutMs..............: timeout, in
    // milliseconds, to use in StopRequest between deconfiguring and closing
    // each affected queue.  This is primarily to give a chance for pending
    // PUSH mesages to be CONFIRMed.  shutdownTimeoutMs..........: timeout, in
    // milliseconds, to use when node stops for shutdown or maintenance mode.
    // This timeout should be greater than the 'stopTimeoutMs'.  This is to
    // handle misbehaving downstream which may not reply to stopRequest
    // (otherwise, this timeout is not expected to be reached).
    // ackWindowSize..............: number of PUTs without ACK requested after
    // which we request an ACK.  This is to remove pending broadcast PUTs.

    // INSTANCE DATA
    int d_openTimeoutMs;
    int d_configureTimeoutMs;
    int d_closeTimeoutMs;
    int d_reopenTimeoutMs;
    int d_reopenRetryIntervalMs;
    int d_reopenMaxAttempts;
    int d_assignmentTimeoutMs;
    int d_keepaliveDurationMs;
    int d_consumptionMonitorPeriodMs;
    int d_stopTimeoutMs;
    int d_shutdownTimeoutMs;
    int d_ackWindowSize;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const QueueOperationsConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_OPEN_TIMEOUT_MS               = 0,
        ATTRIBUTE_ID_CONFIGURE_TIMEOUT_MS          = 1,
        ATTRIBUTE_ID_CLOSE_TIMEOUT_MS              = 2,
        ATTRIBUTE_ID_REOPEN_TIMEOUT_MS             = 3,
        ATTRIBUTE_ID_REOPEN_RETRY_INTERVAL_MS      = 4,
        ATTRIBUTE_ID_REOPEN_MAX_ATTEMPTS           = 5,
        ATTRIBUTE_ID_ASSIGNMENT_TIMEOUT_MS         = 6,
        ATTRIBUTE_ID_KEEPALIVE_DURATION_MS         = 7,
        ATTRIBUTE_ID_CONSUMPTION_MONITOR_PERIOD_MS = 8,
        ATTRIBUTE_ID_STOP_TIMEOUT_MS               = 9,
        ATTRIBUTE_ID_SHUTDOWN_TIMEOUT_MS           = 10,
        ATTRIBUTE_ID_ACK_WINDOW_SIZE               = 11
    };

    enum { NUM_ATTRIBUTES = 12 };

    enum {
        ATTRIBUTE_INDEX_OPEN_TIMEOUT_MS               = 0,
        ATTRIBUTE_INDEX_CONFIGURE_TIMEOUT_MS          = 1,
        ATTRIBUTE_INDEX_CLOSE_TIMEOUT_MS              = 2,
        ATTRIBUTE_INDEX_REOPEN_TIMEOUT_MS             = 3,
        ATTRIBUTE_INDEX_REOPEN_RETRY_INTERVAL_MS      = 4,
        ATTRIBUTE_INDEX_REOPEN_MAX_ATTEMPTS           = 5,
        ATTRIBUTE_INDEX_ASSIGNMENT_TIMEOUT_MS         = 6,
        ATTRIBUTE_INDEX_KEEPALIVE_DURATION_MS         = 7,
        ATTRIBUTE_INDEX_CONSUMPTION_MONITOR_PERIOD_MS = 8,
        ATTRIBUTE_INDEX_STOP_TIMEOUT_MS               = 9,
        ATTRIBUTE_INDEX_SHUTDOWN_TIMEOUT_MS           = 10,
        ATTRIBUTE_INDEX_ACK_WINDOW_SIZE               = 11
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_OPEN_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_CONFIGURE_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_CLOSE_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_REOPEN_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_REOPEN_RETRY_INTERVAL_MS;

    static const int DEFAULT_INITIALIZER_REOPEN_MAX_ATTEMPTS;

    static const int DEFAULT_INITIALIZER_ASSIGNMENT_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_KEEPALIVE_DURATION_MS;

    static const int DEFAULT_INITIALIZER_CONSUMPTION_MONITOR_PERIOD_MS;

    static const int DEFAULT_INITIALIZER_STOP_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_SHUTDOWN_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_ACK_WINDOW_SIZE;

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
    QueueOperationsConfig();
    // Create an object of type 'QueueOperationsConfig' having the default
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

    int& openTimeoutMs();
    // Return a reference to the modifiable "OpenTimeoutMs" attribute of
    // this object.

    int& configureTimeoutMs();
    // Return a reference to the modifiable "ConfigureTimeoutMs" attribute
    // of this object.

    int& closeTimeoutMs();
    // Return a reference to the modifiable "CloseTimeoutMs" attribute of
    // this object.

    int& reopenTimeoutMs();
    // Return a reference to the modifiable "ReopenTimeoutMs" attribute of
    // this object.

    int& reopenRetryIntervalMs();
    // Return a reference to the modifiable "ReopenRetryIntervalMs"
    // attribute of this object.

    int& reopenMaxAttempts();
    // Return a reference to the modifiable "ReopenMaxAttempts" attribute
    // of this object.

    int& assignmentTimeoutMs();
    // Return a reference to the modifiable "AssignmentTimeoutMs" attribute
    // of this object.

    int& keepaliveDurationMs();
    // Return a reference to the modifiable "KeepaliveDurationMs" attribute
    // of this object.

    int& consumptionMonitorPeriodMs();
    // Return a reference to the modifiable "ConsumptionMonitorPeriodMs"
    // attribute of this object.

    int& stopTimeoutMs();
    // Return a reference to the modifiable "StopTimeoutMs" attribute of
    // this object.

    int& shutdownTimeoutMs();
    // Return a reference to the modifiable "ShutdownTimeoutMs" attribute
    // of this object.

    int& ackWindowSize();
    // Return a reference to the modifiable "AckWindowSize" attribute of
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

    int openTimeoutMs() const;
    // Return the value of the "OpenTimeoutMs" attribute of this object.

    int configureTimeoutMs() const;
    // Return the value of the "ConfigureTimeoutMs" attribute of this
    // object.

    int closeTimeoutMs() const;
    // Return the value of the "CloseTimeoutMs" attribute of this object.

    int reopenTimeoutMs() const;
    // Return the value of the "ReopenTimeoutMs" attribute of this object.

    int reopenRetryIntervalMs() const;
    // Return the value of the "ReopenRetryIntervalMs" attribute of this
    // object.

    int reopenMaxAttempts() const;
    // Return the value of the "ReopenMaxAttempts" attribute of this
    // object.

    int assignmentTimeoutMs() const;
    // Return the value of the "AssignmentTimeoutMs" attribute of this
    // object.

    int keepaliveDurationMs() const;
    // Return the value of the "KeepaliveDurationMs" attribute of this
    // object.

    int consumptionMonitorPeriodMs() const;
    // Return the value of the "ConsumptionMonitorPeriodMs" attribute of
    // this object.

    int stopTimeoutMs() const;
    // Return the value of the "StopTimeoutMs" attribute of this object.

    int shutdownTimeoutMs() const;
    // Return the value of the "ShutdownTimeoutMs" attribute of this
    // object.

    int ackWindowSize() const;
    // Return the value of the "AckWindowSize" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const QueueOperationsConfig& lhs,
                           const QueueOperationsConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const QueueOperationsConfig& lhs,
                           const QueueOperationsConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const QueueOperationsConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const QueueOperationsConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'QueueOperationsConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::QueueOperationsConfig)

namespace mqbcfg {

// ====================
// class ResolvedDomain
// ====================

class ResolvedDomain {
    // Top level type representing the information retrieved when resolving a
    // domain.
    // resolvedName.: Resolved name of the domain clusterName..: Name of the
    // cluster where this domain exists

    // INSTANCE DATA
    bsl::string d_resolvedName;
    bsl::string d_clusterName;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_RESOLVED_NAME = 0, ATTRIBUTE_ID_CLUSTER_NAME = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum {
        ATTRIBUTE_INDEX_RESOLVED_NAME = 0,
        ATTRIBUTE_INDEX_CLUSTER_NAME  = 1
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
    explicit ResolvedDomain(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ResolvedDomain' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    ResolvedDomain(const ResolvedDomain& original,
                   bslma::Allocator*     basicAllocator = 0);
    // Create an object of type 'ResolvedDomain' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ResolvedDomain(ResolvedDomain&& original) noexcept;
    // Create an object of type 'ResolvedDomain' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    ResolvedDomain(ResolvedDomain&&  original,
                   bslma::Allocator* basicAllocator);
    // Create an object of type 'ResolvedDomain' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~ResolvedDomain();
    // Destroy this object.

    // MANIPULATORS
    ResolvedDomain& operator=(const ResolvedDomain& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ResolvedDomain& operator=(ResolvedDomain&& rhs);
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

    bsl::string& resolvedName();
    // Return a reference to the modifiable "ResolvedName" attribute of
    // this object.

    bsl::string& clusterName();
    // Return a reference to the modifiable "ClusterName" attribute of this
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

    const bsl::string& resolvedName() const;
    // Return a reference offering non-modifiable access to the
    // "ResolvedName" attribute of this object.

    const bsl::string& clusterName() const;
    // Return a reference offering non-modifiable access to the
    // "ClusterName" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ResolvedDomain& lhs,
                           const ResolvedDomain& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.resolvedName() == rhs.resolvedName() &&
               lhs.clusterName() == rhs.clusterName();
    }

    friend bool operator!=(const ResolvedDomain& lhs,
                           const ResolvedDomain& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&         stream,
                                    const ResolvedDomain& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&     hashAlg,
                           const ResolvedDomain& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ResolvedDomain'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.resolvedName());
        hashAppend(hashAlg, object.clusterName());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ResolvedDomain)

namespace mqbcfg {

// ========================
// class StatsPrinterConfig
// ========================

class StatsPrinterConfig {
    // INSTANCE DATA
    bsl::string d_file;
    int         d_printInterval;
    int         d_maxAgeDays;
    int         d_rotateBytes;
    int         d_rotateDays;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const StatsPrinterConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_PRINT_INTERVAL = 0,
        ATTRIBUTE_ID_FILE           = 1,
        ATTRIBUTE_ID_MAX_AGE_DAYS   = 2,
        ATTRIBUTE_ID_ROTATE_BYTES   = 3,
        ATTRIBUTE_ID_ROTATE_DAYS    = 4
    };

    enum { NUM_ATTRIBUTES = 5 };

    enum {
        ATTRIBUTE_INDEX_PRINT_INTERVAL = 0,
        ATTRIBUTE_INDEX_FILE           = 1,
        ATTRIBUTE_INDEX_MAX_AGE_DAYS   = 2,
        ATTRIBUTE_INDEX_ROTATE_BYTES   = 3,
        ATTRIBUTE_INDEX_ROTATE_DAYS    = 4
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_PRINT_INTERVAL;

    static const int DEFAULT_INITIALIZER_ROTATE_BYTES;

    static const int DEFAULT_INITIALIZER_ROTATE_DAYS;

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
    explicit StatsPrinterConfig(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatsPrinterConfig' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    StatsPrinterConfig(const StatsPrinterConfig& original,
                       bslma::Allocator*         basicAllocator = 0);
    // Create an object of type 'StatsPrinterConfig' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatsPrinterConfig(StatsPrinterConfig&& original) noexcept;
    // Create an object of type 'StatsPrinterConfig' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    StatsPrinterConfig(StatsPrinterConfig&& original,
                       bslma::Allocator*    basicAllocator);
    // Create an object of type 'StatsPrinterConfig' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~StatsPrinterConfig();
    // Destroy this object.

    // MANIPULATORS
    StatsPrinterConfig& operator=(const StatsPrinterConfig& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatsPrinterConfig& operator=(StatsPrinterConfig&& rhs);
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

    int& printInterval();
    // Return a reference to the modifiable "PrintInterval" attribute of
    // this object.

    bsl::string& file();
    // Return a reference to the modifiable "File" attribute of this
    // object.

    int& maxAgeDays();
    // Return a reference to the modifiable "MaxAgeDays" attribute of this
    // object.

    int& rotateBytes();
    // Return a reference to the modifiable "RotateBytes" attribute of this
    // object.

    int& rotateDays();
    // Return a reference to the modifiable "RotateDays" attribute of this
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

    int printInterval() const;
    // Return the value of the "PrintInterval" attribute of this object.

    const bsl::string& file() const;
    // Return a reference offering non-modifiable access to the "File"
    // attribute of this object.

    int maxAgeDays() const;
    // Return the value of the "MaxAgeDays" attribute of this object.

    int rotateBytes() const;
    // Return the value of the "RotateBytes" attribute of this object.

    int rotateDays() const;
    // Return the value of the "RotateDays" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatsPrinterConfig& lhs,
                           const StatsPrinterConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const StatsPrinterConfig& lhs,
                           const StatsPrinterConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const StatsPrinterConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const StatsPrinterConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StatsPrinterConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::StatsPrinterConfig)

namespace mqbcfg {

// =======================
// class StorageSyncConfig
// =======================

class StorageSyncConfig {
    // Type representing the configuration for storage synchronization and
    // recovery of a cluster.
    // startupRecoveryMaxDurationMs...: maximum amount of time, in
    // milliseconds, in which recovery for a partition at node startup must
    // complete.  This interval captures the time taken to receive storage-sync
    // response, retry attempts for failed storage-sync request, as well as the
    // time taken by the peer node to send partition files to the starting
    // (i.e.  requester) node maxAttemptsStorageSync.........: maximum number
    // of attempts that a node makes for storage-sync requests when it comes up
    // (this value includes the 1st attempt) storageSyncReqTimeoutMs........:
    // timeout, in milliseconds, for the storage-sync request.  A bigger value
    // is recommended because peer node could be busy serving storage-sync
    // request for other partition(s) assigned to the same partition-dispatcher
    // thread, etc.  This timeout does *not* capture the time taken by the peer
    // to send partition files masterSyncMaxDurationMs........: maximum amount
    // of time, in milliseconds, in which master sync for a partition must
    // complete.  This interval includes the time taken by replica node (the
    // one with advanced view of the partition) to send the file chunks, as
    // well as time taken to receive partition-sync state and data responses
    // partitionSyncStateReqTimeoutMs.: timeout, in milliseconds, for
    // partition-sync-state-query request.  This request is sent by a new
    // master node to all replica nodes to query their view of the partition
    // partitionSyncDataReqTimeoutMs..: timeout, in milliseconds, for
    // partition-sync-data-query request.  This request is sent by a new master
    // node to a replica node (which has an advanced view of the partition) to
    // initiate partition sync.  This duration does *not* capture the amount of
    // time which replica might take to send the partition file
    // startupWaitDurationMs..........: duration, in milliseconds, for which
    // recovery manager waits for a sync point for a partition.  If no sync
    // point is received from a peer for a partition during this time, it is
    // assumed that there is no master for that partition, and recovery manager
    // randomly picks up a node from all the available ones with send a sync
    // request.  If no peers are available at this time, it is assumed that
    // entire cluster is coming up together, and proceeds with local recovery
    // for that partition.  Note that this value should be less than the
    // duration for which a node waits if its elected a leader and there are no
    // AVAILABLE nodes.  This is important so that if all nodes in the cluster
    // are starting, they have a chance to wait for 'startupWaitDurationMs'
    // milliseconds, find out that none of the partitions have any master, go
    // ahead with local recovery and declare themselves as AVAILABLE.  This
    // will give the new leader node a chance to make each node a master for a
    // given partition.  Moreover, this value should be greater than the
    // duration for which a peer waits before attempting to reconnect to the
    // node in the cluster, so that peer has a chance to connect to this node,
    // get notified (via ClusterObserver), and send sync point if its a master
    // for any partition fileChunkSize..................: chunk size, in bytes,
    // to send in one go to the peer when serving a storage sync request from
    // it partitionSyncEventSize.........: maximum size, in bytes, of
    // bmqp::EventType::PARTITION_SYNC before we send it to the peer

    // INSTANCE DATA
    int d_startupRecoveryMaxDurationMs;
    int d_maxAttemptsStorageSync;
    int d_storageSyncReqTimeoutMs;
    int d_masterSyncMaxDurationMs;
    int d_partitionSyncStateReqTimeoutMs;
    int d_partitionSyncDataReqTimeoutMs;
    int d_startupWaitDurationMs;
    int d_fileChunkSize;
    int d_partitionSyncEventSize;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const StorageSyncConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_STARTUP_RECOVERY_MAX_DURATION_MS    = 0,
        ATTRIBUTE_ID_MAX_ATTEMPTS_STORAGE_SYNC           = 1,
        ATTRIBUTE_ID_STORAGE_SYNC_REQ_TIMEOUT_MS         = 2,
        ATTRIBUTE_ID_MASTER_SYNC_MAX_DURATION_MS         = 3,
        ATTRIBUTE_ID_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS = 4,
        ATTRIBUTE_ID_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS  = 5,
        ATTRIBUTE_ID_STARTUP_WAIT_DURATION_MS            = 6,
        ATTRIBUTE_ID_FILE_CHUNK_SIZE                     = 7,
        ATTRIBUTE_ID_PARTITION_SYNC_EVENT_SIZE           = 8
    };

    enum { NUM_ATTRIBUTES = 9 };

    enum {
        ATTRIBUTE_INDEX_STARTUP_RECOVERY_MAX_DURATION_MS    = 0,
        ATTRIBUTE_INDEX_MAX_ATTEMPTS_STORAGE_SYNC           = 1,
        ATTRIBUTE_INDEX_STORAGE_SYNC_REQ_TIMEOUT_MS         = 2,
        ATTRIBUTE_INDEX_MASTER_SYNC_MAX_DURATION_MS         = 3,
        ATTRIBUTE_INDEX_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS = 4,
        ATTRIBUTE_INDEX_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS  = 5,
        ATTRIBUTE_INDEX_STARTUP_WAIT_DURATION_MS            = 6,
        ATTRIBUTE_INDEX_FILE_CHUNK_SIZE                     = 7,
        ATTRIBUTE_INDEX_PARTITION_SYNC_EVENT_SIZE           = 8
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_STARTUP_RECOVERY_MAX_DURATION_MS;

    static const int DEFAULT_INITIALIZER_MAX_ATTEMPTS_STORAGE_SYNC;

    static const int DEFAULT_INITIALIZER_STORAGE_SYNC_REQ_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_MASTER_SYNC_MAX_DURATION_MS;

    static const int DEFAULT_INITIALIZER_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_STARTUP_WAIT_DURATION_MS;

    static const int DEFAULT_INITIALIZER_FILE_CHUNK_SIZE;

    static const int DEFAULT_INITIALIZER_PARTITION_SYNC_EVENT_SIZE;

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
    StorageSyncConfig();
    // Create an object of type 'StorageSyncConfig' having the default
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

    int& startupRecoveryMaxDurationMs();
    // Return a reference to the modifiable "StartupRecoveryMaxDurationMs"
    // attribute of this object.

    int& maxAttemptsStorageSync();
    // Return a reference to the modifiable "MaxAttemptsStorageSync"
    // attribute of this object.

    int& storageSyncReqTimeoutMs();
    // Return a reference to the modifiable "StorageSyncReqTimeoutMs"
    // attribute of this object.

    int& masterSyncMaxDurationMs();
    // Return a reference to the modifiable "MasterSyncMaxDurationMs"
    // attribute of this object.

    int& partitionSyncStateReqTimeoutMs();
    // Return a reference to the modifiable
    // "PartitionSyncStateReqTimeoutMs" attribute of this object.

    int& partitionSyncDataReqTimeoutMs();
    // Return a reference to the modifiable "PartitionSyncDataReqTimeoutMs"
    // attribute of this object.

    int& startupWaitDurationMs();
    // Return a reference to the modifiable "StartupWaitDurationMs"
    // attribute of this object.

    int& fileChunkSize();
    // Return a reference to the modifiable "FileChunkSize" attribute of
    // this object.

    int& partitionSyncEventSize();
    // Return a reference to the modifiable "PartitionSyncEventSize"
    // attribute of this object.

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

    int startupRecoveryMaxDurationMs() const;
    // Return the value of the "StartupRecoveryMaxDurationMs" attribute of
    // this object.

    int maxAttemptsStorageSync() const;
    // Return the value of the "MaxAttemptsStorageSync" attribute of this
    // object.

    int storageSyncReqTimeoutMs() const;
    // Return the value of the "StorageSyncReqTimeoutMs" attribute of this
    // object.

    int masterSyncMaxDurationMs() const;
    // Return the value of the "MasterSyncMaxDurationMs" attribute of this
    // object.

    int partitionSyncStateReqTimeoutMs() const;
    // Return the value of the "PartitionSyncStateReqTimeoutMs" attribute
    // of this object.

    int partitionSyncDataReqTimeoutMs() const;
    // Return the value of the "PartitionSyncDataReqTimeoutMs" attribute of
    // this object.

    int startupWaitDurationMs() const;
    // Return the value of the "StartupWaitDurationMs" attribute of this
    // object.

    int fileChunkSize() const;
    // Return the value of the "FileChunkSize" attribute of this object.

    int partitionSyncEventSize() const;
    // Return the value of the "PartitionSyncEventSize" attribute of this
    // object.

    // HIDDEN FRIENDS
    friend bool operator==(const StorageSyncConfig& lhs,
                           const StorageSyncConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const StorageSyncConfig& lhs,
                           const StorageSyncConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const StorageSyncConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const StorageSyncConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StorageSyncConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::StorageSyncConfig)

namespace mqbcfg {

// ==================
// class SyslogConfig
// ==================

class SyslogConfig {
    // INSTANCE DATA
    bsl::string d_appName;
    bsl::string d_logFormat;
    bsl::string d_verbosity;
    bool        d_enabled;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const SyslogConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_ENABLED    = 0,
        ATTRIBUTE_ID_APP_NAME   = 1,
        ATTRIBUTE_ID_LOG_FORMAT = 2,
        ATTRIBUTE_ID_VERBOSITY  = 3
    };

    enum { NUM_ATTRIBUTES = 4 };

    enum {
        ATTRIBUTE_INDEX_ENABLED    = 0,
        ATTRIBUTE_INDEX_APP_NAME   = 1,
        ATTRIBUTE_INDEX_LOG_FORMAT = 2,
        ATTRIBUTE_INDEX_VERBOSITY  = 3
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_ENABLED;

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
    explicit SyslogConfig(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'SyslogConfig' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    SyslogConfig(const SyslogConfig& original,
                 bslma::Allocator*   basicAllocator = 0);
    // Create an object of type 'SyslogConfig' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    SyslogConfig(SyslogConfig&& original) noexcept;
    // Create an object of type 'SyslogConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    SyslogConfig(SyslogConfig&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'SyslogConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~SyslogConfig();
    // Destroy this object.

    // MANIPULATORS
    SyslogConfig& operator=(const SyslogConfig& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    SyslogConfig& operator=(SyslogConfig&& rhs);
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

    bool& enabled();
    // Return a reference to the modifiable "Enabled" attribute of this
    // object.

    bsl::string& appName();
    // Return a reference to the modifiable "AppName" attribute of this
    // object.

    bsl::string& logFormat();
    // Return a reference to the modifiable "LogFormat" attribute of this
    // object.

    bsl::string& verbosity();
    // Return a reference to the modifiable "Verbosity" attribute of this
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

    bool enabled() const;
    // Return the value of the "Enabled" attribute of this object.

    const bsl::string& appName() const;
    // Return a reference offering non-modifiable access to the "AppName"
    // attribute of this object.

    const bsl::string& logFormat() const;
    // Return a reference offering non-modifiable access to the "LogFormat"
    // attribute of this object.

    const bsl::string& verbosity() const;
    // Return a reference offering non-modifiable access to the "Verbosity"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const SyslogConfig& lhs, const SyslogConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const SyslogConfig& lhs, const SyslogConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&       stream,
                                    const SyslogConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&   hashAlg,
                           const SyslogConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'SyslogConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::SyslogConfig)

namespace mqbcfg {

// ==============================
// class TcpClusterNodeConnection
// ==============================

class TcpClusterNodeConnection {
    // Configuration of a TCP based cluster node connectivity.
    // endpoint.: endpoint URI of the node address

    // INSTANCE DATA
    bsl::string d_endpoint;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_ENDPOINT = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_ENDPOINT = 0 };

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
    explicit TcpClusterNodeConnection(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'TcpClusterNodeConnection' having the
    // default value.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

    TcpClusterNodeConnection(const TcpClusterNodeConnection& original,
                             bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'TcpClusterNodeConnection' having the value
    // of the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    TcpClusterNodeConnection(TcpClusterNodeConnection&& original) noexcept;
    // Create an object of type 'TcpClusterNodeConnection' having the value
    // of the specified 'original' object.  After performing this action,
    // the 'original' object will be left in a valid, but unspecified
    // state.

    TcpClusterNodeConnection(TcpClusterNodeConnection&& original,
                             bslma::Allocator*          basicAllocator);
    // Create an object of type 'TcpClusterNodeConnection' having the value
    // of the specified 'original' object.  After performing this action,
    // the 'original' object will be left in a valid, but unspecified
    // state.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.
#endif

    ~TcpClusterNodeConnection();
    // Destroy this object.

    // MANIPULATORS
    TcpClusterNodeConnection& operator=(const TcpClusterNodeConnection& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    TcpClusterNodeConnection& operator=(TcpClusterNodeConnection&& rhs);
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

    bsl::string& endpoint();
    // Return a reference to the modifiable "Endpoint" attribute of this
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

    const bsl::string& endpoint() const;
    // Return a reference offering non-modifiable access to the "Endpoint"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const TcpClusterNodeConnection& lhs,
                           const TcpClusterNodeConnection& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.endpoint() == rhs.endpoint();
    }

    friend bool operator!=(const TcpClusterNodeConnection& lhs,
                           const TcpClusterNodeConnection& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                   stream,
                                    const TcpClusterNodeConnection& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&               hashAlg,
                           const TcpClusterNodeConnection& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'TcpClusterNodeConnection'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.endpoint());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::TcpClusterNodeConnection)

namespace mqbcfg {

// ========================
// class TcpInterfaceConfig
// ========================

class TcpInterfaceConfig {
    // lowWatermark.........: highWatermark........: Watermarks used for
    // channels with a client or proxy.  nodeLowWatermark.....:
    // nodeHighWatermark....: Reduced watermarks for communication between
    // cluster nodes where BlazingMQ maintains its own cache.
    // heartbeatIntervalMs..: How often (in milliseconds) to check if the
    // channel received data, and emit heartbeat.  0 to globally disable.

    // INSTANCE DATA
    bsls::Types::Int64 d_lowWatermark;
    bsls::Types::Int64 d_highWatermark;
    bsls::Types::Int64 d_nodeLowWatermark;
    bsls::Types::Int64 d_nodeHighWatermark;
    bsl::string        d_name;
    int                d_port;
    int                d_ioThreads;
    int                d_maxConnections;
    int                d_heartbeatIntervalMs;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const TcpInterfaceConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_NAME                  = 0,
        ATTRIBUTE_ID_PORT                  = 1,
        ATTRIBUTE_ID_IO_THREADS            = 2,
        ATTRIBUTE_ID_MAX_CONNECTIONS       = 3,
        ATTRIBUTE_ID_LOW_WATERMARK         = 4,
        ATTRIBUTE_ID_HIGH_WATERMARK        = 5,
        ATTRIBUTE_ID_NODE_LOW_WATERMARK    = 6,
        ATTRIBUTE_ID_NODE_HIGH_WATERMARK   = 7,
        ATTRIBUTE_ID_HEARTBEAT_INTERVAL_MS = 8
    };

    enum { NUM_ATTRIBUTES = 9 };

    enum {
        ATTRIBUTE_INDEX_NAME                  = 0,
        ATTRIBUTE_INDEX_PORT                  = 1,
        ATTRIBUTE_INDEX_IO_THREADS            = 2,
        ATTRIBUTE_INDEX_MAX_CONNECTIONS       = 3,
        ATTRIBUTE_INDEX_LOW_WATERMARK         = 4,
        ATTRIBUTE_INDEX_HIGH_WATERMARK        = 5,
        ATTRIBUTE_INDEX_NODE_LOW_WATERMARK    = 6,
        ATTRIBUTE_INDEX_NODE_HIGH_WATERMARK   = 7,
        ATTRIBUTE_INDEX_HEARTBEAT_INTERVAL_MS = 8
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_MAX_CONNECTIONS;

    static const bsls::Types::Int64 DEFAULT_INITIALIZER_NODE_LOW_WATERMARK;

    static const bsls::Types::Int64 DEFAULT_INITIALIZER_NODE_HIGH_WATERMARK;

    static const int DEFAULT_INITIALIZER_HEARTBEAT_INTERVAL_MS;

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
    explicit TcpInterfaceConfig(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'TcpInterfaceConfig' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    TcpInterfaceConfig(const TcpInterfaceConfig& original,
                       bslma::Allocator*         basicAllocator = 0);
    // Create an object of type 'TcpInterfaceConfig' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    TcpInterfaceConfig(TcpInterfaceConfig&& original) noexcept;
    // Create an object of type 'TcpInterfaceConfig' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    TcpInterfaceConfig(TcpInterfaceConfig&& original,
                       bslma::Allocator*    basicAllocator);
    // Create an object of type 'TcpInterfaceConfig' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~TcpInterfaceConfig();
    // Destroy this object.

    // MANIPULATORS
    TcpInterfaceConfig& operator=(const TcpInterfaceConfig& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    TcpInterfaceConfig& operator=(TcpInterfaceConfig&& rhs);
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

    int& port();
    // Return a reference to the modifiable "Port" attribute of this
    // object.

    int& ioThreads();
    // Return a reference to the modifiable "IoThreads" attribute of this
    // object.

    int& maxConnections();
    // Return a reference to the modifiable "MaxConnections" attribute of
    // this object.

    bsls::Types::Int64& lowWatermark();
    // Return a reference to the modifiable "LowWatermark" attribute of
    // this object.

    bsls::Types::Int64& highWatermark();
    // Return a reference to the modifiable "HighWatermark" attribute of
    // this object.

    bsls::Types::Int64& nodeLowWatermark();
    // Return a reference to the modifiable "NodeLowWatermark" attribute of
    // this object.

    bsls::Types::Int64& nodeHighWatermark();
    // Return a reference to the modifiable "NodeHighWatermark" attribute
    // of this object.

    int& heartbeatIntervalMs();
    // Return a reference to the modifiable "HeartbeatIntervalMs" attribute
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

    const bsl::string& name() const;
    // Return a reference offering non-modifiable access to the "Name"
    // attribute of this object.

    int port() const;
    // Return the value of the "Port" attribute of this object.

    int ioThreads() const;
    // Return the value of the "IoThreads" attribute of this object.

    int maxConnections() const;
    // Return the value of the "MaxConnections" attribute of this object.

    bsls::Types::Int64 lowWatermark() const;
    // Return the value of the "LowWatermark" attribute of this object.

    bsls::Types::Int64 highWatermark() const;
    // Return the value of the "HighWatermark" attribute of this object.

    bsls::Types::Int64 nodeLowWatermark() const;
    // Return the value of the "NodeLowWatermark" attribute of this object.

    bsls::Types::Int64 nodeHighWatermark() const;
    // Return the value of the "NodeHighWatermark" attribute of this
    // object.

    int heartbeatIntervalMs() const;
    // Return the value of the "HeartbeatIntervalMs" attribute of this
    // object.

    // HIDDEN FRIENDS
    friend bool operator==(const TcpInterfaceConfig& lhs,
                           const TcpInterfaceConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const TcpInterfaceConfig& lhs,
                           const TcpInterfaceConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const TcpInterfaceConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const TcpInterfaceConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'TcpInterfaceConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::TcpInterfaceConfig)

namespace mqbcfg {

// ===============================
// class VirtualClusterInformation
// ===============================

class VirtualClusterInformation {
    // Type representing the information about the current node with regards to
    // virtual cluster.
    // name.............: name of the cluster selfNodeId.......: id of the
    // current node in that virtual cluster

    // INSTANCE DATA
    bsl::string d_name;
    int         d_selfNodeId;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_NAME = 0, ATTRIBUTE_ID_SELF_NODE_ID = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_NAME = 0, ATTRIBUTE_INDEX_SELF_NODE_ID = 1 };

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
    explicit VirtualClusterInformation(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'VirtualClusterInformation' having the
    // default value.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

    VirtualClusterInformation(const VirtualClusterInformation& original,
                              bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'VirtualClusterInformation' having the
    // value of the specified 'original' object.  Use the optionally
    // specified 'basicAllocator' to supply memory.  If 'basicAllocator' is
    // 0, the currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    VirtualClusterInformation(VirtualClusterInformation&& original) noexcept;
    // Create an object of type 'VirtualClusterInformation' having the
    // value of the specified 'original' object.  After performing this
    // action, the 'original' object will be left in a valid, but
    // unspecified state.

    VirtualClusterInformation(VirtualClusterInformation&& original,
                              bslma::Allocator*           basicAllocator);
    // Create an object of type 'VirtualClusterInformation' having the
    // value of the specified 'original' object.  After performing this
    // action, the 'original' object will be left in a valid, but
    // unspecified state.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.
#endif

    ~VirtualClusterInformation();
    // Destroy this object.

    // MANIPULATORS
    VirtualClusterInformation& operator=(const VirtualClusterInformation& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    VirtualClusterInformation& operator=(VirtualClusterInformation&& rhs);
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

    int& selfNodeId();
    // Return a reference to the modifiable "SelfNodeId" attribute of this
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

    int selfNodeId() const;
    // Return the value of the "SelfNodeId" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const VirtualClusterInformation& lhs,
                           const VirtualClusterInformation& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.name() == rhs.name() &&
               lhs.selfNodeId() == rhs.selfNodeId();
    }

    friend bool operator!=(const VirtualClusterInformation& lhs,
                           const VirtualClusterInformation& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                    stream,
                                    const VirtualClusterInformation& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                hashAlg,
                           const VirtualClusterInformation& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'VirtualClusterInformation'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.name());
        hashAppend(hashAlg, object.selfNodeId());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::VirtualClusterInformation)

namespace mqbcfg {

// ===========================
// class ClusterNodeConnection
// ===========================

class ClusterNodeConnection {
    // Choice of all the various transport mode available to establish
    // connectivity with a node.
    // tcp.: TCP connectivity

    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<TcpClusterNodeConnection> d_tcp;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const ClusterNodeConnection& rhs) const;

  public:
    // TYPES

    enum { SELECTION_ID_UNDEFINED = -1, SELECTION_ID_TCP = 0 };

    enum { NUM_SELECTIONS = 1 };

    enum { SELECTION_INDEX_TCP = 0 };

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
    explicit ClusterNodeConnection(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ClusterNodeConnection' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    ClusterNodeConnection(const ClusterNodeConnection& original,
                          bslma::Allocator*            basicAllocator = 0);
    // Create an object of type 'ClusterNodeConnection' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClusterNodeConnection(ClusterNodeConnection&& original) noexcept;
    // Create an object of type 'ClusterNodeConnection' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    ClusterNodeConnection(ClusterNodeConnection&& original,
                          bslma::Allocator*       basicAllocator);
    // Create an object of type 'ClusterNodeConnection' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~ClusterNodeConnection();
    // Destroy this object.

    // MANIPULATORS
    ClusterNodeConnection& operator=(const ClusterNodeConnection& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClusterNodeConnection& operator=(ClusterNodeConnection&& rhs);
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

    TcpClusterNodeConnection& makeTcp();
    TcpClusterNodeConnection& makeTcp(const TcpClusterNodeConnection& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    TcpClusterNodeConnection& makeTcp(TcpClusterNodeConnection&& value);
#endif
    // Set the value of this object to be a "Tcp" value.  Optionally
    // specify the 'value' of the "Tcp".  If 'value' is not specified, the
    // default "Tcp" value is used.

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    TcpClusterNodeConnection& tcp();
    // Return a reference to the modifiable "Tcp" selection of this object
    // if "Tcp" is the current selection.  The behavior is undefined unless
    // "Tcp" is the selection of this object.

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

    const TcpClusterNodeConnection& tcp() const;
    // Return a reference to the non-modifiable "Tcp" selection of this
    // object if "Tcp" is the current selection.  The behavior is undefined
    // unless "Tcp" is the selection of this object.

    bool isTcpValue() const;
    // Return 'true' if the value of this object is a "Tcp" value, and
    // return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ClusterNodeConnection& lhs,
                           const ClusterNodeConnection& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'ClusterNodeConnection' objects
    // have the same value if either the selections in both objects have
    // the same ids and the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const ClusterNodeConnection& lhs,
                           const ClusterNodeConnection& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const ClusterNodeConnection& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const ClusterNodeConnection& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ClusterNodeConnection'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ClusterNodeConnection)

namespace mqbcfg {

// ===============================
// class DispatcherProcessorConfig
// ===============================

class DispatcherProcessorConfig {
    // INSTANCE DATA
    DispatcherProcessorParameters d_processorConfig;
    int                           d_numProcessors;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_NUM_PROCESSORS   = 0,
        ATTRIBUTE_ID_PROCESSOR_CONFIG = 1
    };

    enum { NUM_ATTRIBUTES = 2 };

    enum {
        ATTRIBUTE_INDEX_NUM_PROCESSORS   = 0,
        ATTRIBUTE_INDEX_PROCESSOR_CONFIG = 1
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
    DispatcherProcessorConfig();
    // Create an object of type 'DispatcherProcessorConfig' having the
    // default value.

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

    int& numProcessors();
    // Return a reference to the modifiable "NumProcessors" attribute of
    // this object.

    DispatcherProcessorParameters& processorConfig();
    // Return a reference to the modifiable "ProcessorConfig" attribute of
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

    int numProcessors() const;
    // Return the value of the "NumProcessors" attribute of this object.

    const DispatcherProcessorParameters& processorConfig() const;
    // Return a reference offering non-modifiable access to the
    // "ProcessorConfig" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DispatcherProcessorConfig& lhs,
                           const DispatcherProcessorConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.numProcessors() == rhs.numProcessors() &&
               lhs.processorConfig() == rhs.processorConfig();
    }

    friend bool operator!=(const DispatcherProcessorConfig& lhs,
                           const DispatcherProcessorConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                    stream,
                                    const DispatcherProcessorConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                hashAlg,
                           const DispatcherProcessorConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DispatcherProcessorConfig'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.numProcessors());
        hashAppend(hashAlg, object.processorConfig());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(
    mqbcfg::DispatcherProcessorConfig)

namespace mqbcfg {

// ===================
// class LogController
// ===================

class LogController {
    // INSTANCE DATA
    bsl::vector<bsl::string> d_categories;
    bsl::string              d_fileName;
    bsl::string              d_logfileFormat;
    bsl::string              d_consoleFormat;
    bsl::string              d_loggingVerbosity;
    bsl::string              d_bslsLogSeverityThreshold;
    bsl::string              d_consoleSeverityThreshold;
    SyslogConfig             d_syslog;
    int                      d_fileMaxAgeDays;
    int                      d_rotationBytes;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const LogController& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_FILE_NAME                   = 0,
        ATTRIBUTE_ID_FILE_MAX_AGE_DAYS           = 1,
        ATTRIBUTE_ID_ROTATION_BYTES              = 2,
        ATTRIBUTE_ID_LOGFILE_FORMAT              = 3,
        ATTRIBUTE_ID_CONSOLE_FORMAT              = 4,
        ATTRIBUTE_ID_LOGGING_VERBOSITY           = 5,
        ATTRIBUTE_ID_BSLS_LOG_SEVERITY_THRESHOLD = 6,
        ATTRIBUTE_ID_CONSOLE_SEVERITY_THRESHOLD  = 7,
        ATTRIBUTE_ID_CATEGORIES                  = 8,
        ATTRIBUTE_ID_SYSLOG                      = 9
    };

    enum { NUM_ATTRIBUTES = 10 };

    enum {
        ATTRIBUTE_INDEX_FILE_NAME                   = 0,
        ATTRIBUTE_INDEX_FILE_MAX_AGE_DAYS           = 1,
        ATTRIBUTE_INDEX_ROTATION_BYTES              = 2,
        ATTRIBUTE_INDEX_LOGFILE_FORMAT              = 3,
        ATTRIBUTE_INDEX_CONSOLE_FORMAT              = 4,
        ATTRIBUTE_INDEX_LOGGING_VERBOSITY           = 5,
        ATTRIBUTE_INDEX_BSLS_LOG_SEVERITY_THRESHOLD = 6,
        ATTRIBUTE_INDEX_CONSOLE_SEVERITY_THRESHOLD  = 7,
        ATTRIBUTE_INDEX_CATEGORIES                  = 8,
        ATTRIBUTE_INDEX_SYSLOG                      = 9
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_BSLS_LOG_SEVERITY_THRESHOLD[];

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
    explicit LogController(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'LogController' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    LogController(const LogController& original,
                  bslma::Allocator*    basicAllocator = 0);
    // Create an object of type 'LogController' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    LogController(LogController&& original) noexcept;
    // Create an object of type 'LogController' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    LogController(LogController&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'LogController' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~LogController();
    // Destroy this object.

    // MANIPULATORS
    LogController& operator=(const LogController& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    LogController& operator=(LogController&& rhs);
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

    bsl::string& fileName();
    // Return a reference to the modifiable "FileName" attribute of this
    // object.

    int& fileMaxAgeDays();
    // Return a reference to the modifiable "FileMaxAgeDays" attribute of
    // this object.

    int& rotationBytes();
    // Return a reference to the modifiable "RotationBytes" attribute of
    // this object.

    bsl::string& logfileFormat();
    // Return a reference to the modifiable "LogfileFormat" attribute of
    // this object.

    bsl::string& consoleFormat();
    // Return a reference to the modifiable "ConsoleFormat" attribute of
    // this object.

    bsl::string& loggingVerbosity();
    // Return a reference to the modifiable "LoggingVerbosity" attribute of
    // this object.

    bsl::string& bslsLogSeverityThreshold();
    // Return a reference to the modifiable "BslsLogSeverityThreshold"
    // attribute of this object.

    bsl::string& consoleSeverityThreshold();
    // Return a reference to the modifiable "ConsoleSeverityThreshold"
    // attribute of this object.

    bsl::vector<bsl::string>& categories();
    // Return a reference to the modifiable "Categories" attribute of this
    // object.

    SyslogConfig& syslog();
    // Return a reference to the modifiable "Syslog" attribute of this
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

    const bsl::string& fileName() const;
    // Return a reference offering non-modifiable access to the "FileName"
    // attribute of this object.

    int fileMaxAgeDays() const;
    // Return the value of the "FileMaxAgeDays" attribute of this object.

    int rotationBytes() const;
    // Return the value of the "RotationBytes" attribute of this object.

    const bsl::string& logfileFormat() const;
    // Return a reference offering non-modifiable access to the
    // "LogfileFormat" attribute of this object.

    const bsl::string& consoleFormat() const;
    // Return a reference offering non-modifiable access to the
    // "ConsoleFormat" attribute of this object.

    const bsl::string& loggingVerbosity() const;
    // Return a reference offering non-modifiable access to the
    // "LoggingVerbosity" attribute of this object.

    const bsl::string& bslsLogSeverityThreshold() const;
    // Return a reference offering non-modifiable access to the
    // "BslsLogSeverityThreshold" attribute of this object.

    const bsl::string& consoleSeverityThreshold() const;
    // Return a reference offering non-modifiable access to the
    // "ConsoleSeverityThreshold" attribute of this object.

    const bsl::vector<bsl::string>& categories() const;
    // Return a reference offering non-modifiable access to the
    // "Categories" attribute of this object.

    const SyslogConfig& syslog() const;
    // Return a reference offering non-modifiable access to the "Syslog"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const LogController& lhs, const LogController& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const LogController& lhs, const LogController& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const LogController& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const LogController& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'LogController'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::LogController)

namespace mqbcfg {

// =======================
// class NetworkInterfaces
// =======================

class NetworkInterfaces {
    // INSTANCE DATA
    bdlb::NullableValue<TcpInterfaceConfig> d_tcpInterface;
    Heartbeat                               d_heartbeats;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_HEARTBEATS = 0, ATTRIBUTE_ID_TCP_INTERFACE = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_HEARTBEATS = 0, ATTRIBUTE_INDEX_TCP_INTERFACE = 1 };

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
    explicit NetworkInterfaces(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'NetworkInterfaces' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    NetworkInterfaces(const NetworkInterfaces& original,
                      bslma::Allocator*        basicAllocator = 0);
    // Create an object of type 'NetworkInterfaces' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    NetworkInterfaces(NetworkInterfaces&& original) noexcept;
    // Create an object of type 'NetworkInterfaces' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    NetworkInterfaces(NetworkInterfaces&& original,
                      bslma::Allocator*   basicAllocator);
    // Create an object of type 'NetworkInterfaces' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~NetworkInterfaces();
    // Destroy this object.

    // MANIPULATORS
    NetworkInterfaces& operator=(const NetworkInterfaces& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    NetworkInterfaces& operator=(NetworkInterfaces&& rhs);
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

    Heartbeat& heartbeats();
    // Return a reference to the modifiable "Heartbeats" attribute of this
    // object.

    bdlb::NullableValue<TcpInterfaceConfig>& tcpInterface();
    // Return a reference to the modifiable "TcpInterface" attribute of
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

    const Heartbeat& heartbeats() const;
    // Return a reference offering non-modifiable access to the
    // "Heartbeats" attribute of this object.

    const bdlb::NullableValue<TcpInterfaceConfig>& tcpInterface() const;
    // Return a reference offering non-modifiable access to the
    // "TcpInterface" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const NetworkInterfaces& lhs,
                           const NetworkInterfaces& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.heartbeats() == rhs.heartbeats() &&
               lhs.tcpInterface() == rhs.tcpInterface();
    }

    friend bool operator!=(const NetworkInterfaces& lhs,
                           const NetworkInterfaces& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const NetworkInterfaces& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const NetworkInterfaces& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'NetworkInterfaces'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.heartbeats());
        hashAppend(hashAlg, object.tcpInterface());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::NetworkInterfaces)

namespace mqbcfg {

// =====================
// class PartitionConfig
// =====================

class PartitionConfig {
    // Type representing the configuration for the storage layer of a cluster.
    //
    // numPartitions........: number of partitions at each node in the cluster
    // location.............: location of active files for a partition
    // archiveLocation......: location of archive files for a partition
    // maxDataFileSize......: maximum size of partitions' data file
    // maxJournalFileSize...: maximum size of partitions' journal file
    // maxQlistFileSize.....: maximum size of partitions' qlist file
    // preallocate..........: flag to indicate whether files should be
    // preallocated on disk maxArchivedFileSets..: maximum number of archived
    // file sets per partition to keep prefaultPages........: flag to indicate
    // whether to populate (prefault) page tables for a mapping.
    // flushAtShutdown......: flag to indicate whether broker should flush
    // storage files to disk at shutdown syncConfig...........: configuration
    // for storage synchronization and recovery

    // INSTANCE DATA
    bsls::Types::Uint64 d_maxDataFileSize;
    bsls::Types::Uint64 d_maxJournalFileSize;
    bsls::Types::Uint64 d_maxQlistFileSize;
    bsl::string         d_location;
    bsl::string         d_archiveLocation;
    StorageSyncConfig   d_syncConfig;
    int                 d_numPartitions;
    int                 d_maxArchivedFileSets;
    bool                d_preallocate;
    bool                d_prefaultPages;
    bool                d_flushAtShutdown;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const PartitionConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_NUM_PARTITIONS         = 0,
        ATTRIBUTE_ID_LOCATION               = 1,
        ATTRIBUTE_ID_ARCHIVE_LOCATION       = 2,
        ATTRIBUTE_ID_MAX_DATA_FILE_SIZE     = 3,
        ATTRIBUTE_ID_MAX_JOURNAL_FILE_SIZE  = 4,
        ATTRIBUTE_ID_MAX_QLIST_FILE_SIZE    = 5,
        ATTRIBUTE_ID_PREALLOCATE            = 6,
        ATTRIBUTE_ID_MAX_ARCHIVED_FILE_SETS = 7,
        ATTRIBUTE_ID_PREFAULT_PAGES         = 8,
        ATTRIBUTE_ID_FLUSH_AT_SHUTDOWN      = 9,
        ATTRIBUTE_ID_SYNC_CONFIG            = 10
    };

    enum { NUM_ATTRIBUTES = 11 };

    enum {
        ATTRIBUTE_INDEX_NUM_PARTITIONS         = 0,
        ATTRIBUTE_INDEX_LOCATION               = 1,
        ATTRIBUTE_INDEX_ARCHIVE_LOCATION       = 2,
        ATTRIBUTE_INDEX_MAX_DATA_FILE_SIZE     = 3,
        ATTRIBUTE_INDEX_MAX_JOURNAL_FILE_SIZE  = 4,
        ATTRIBUTE_INDEX_MAX_QLIST_FILE_SIZE    = 5,
        ATTRIBUTE_INDEX_PREALLOCATE            = 6,
        ATTRIBUTE_INDEX_MAX_ARCHIVED_FILE_SETS = 7,
        ATTRIBUTE_INDEX_PREFAULT_PAGES         = 8,
        ATTRIBUTE_INDEX_FLUSH_AT_SHUTDOWN      = 9,
        ATTRIBUTE_INDEX_SYNC_CONFIG            = 10
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_PREALLOCATE;

    static const bool DEFAULT_INITIALIZER_PREFAULT_PAGES;

    static const bool DEFAULT_INITIALIZER_FLUSH_AT_SHUTDOWN;

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
    explicit PartitionConfig(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'PartitionConfig' having the default value.
    //  Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    PartitionConfig(const PartitionConfig& original,
                    bslma::Allocator*      basicAllocator = 0);
    // Create an object of type 'PartitionConfig' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    PartitionConfig(PartitionConfig&& original) noexcept;
    // Create an object of type 'PartitionConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    PartitionConfig(PartitionConfig&& original,
                    bslma::Allocator* basicAllocator);
    // Create an object of type 'PartitionConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~PartitionConfig();
    // Destroy this object.

    // MANIPULATORS
    PartitionConfig& operator=(const PartitionConfig& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    PartitionConfig& operator=(PartitionConfig&& rhs);
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

    int& numPartitions();
    // Return a reference to the modifiable "NumPartitions" attribute of
    // this object.

    bsl::string& location();
    // Return a reference to the modifiable "Location" attribute of this
    // object.

    bsl::string& archiveLocation();
    // Return a reference to the modifiable "ArchiveLocation" attribute of
    // this object.

    bsls::Types::Uint64& maxDataFileSize();
    // Return a reference to the modifiable "MaxDataFileSize" attribute of
    // this object.

    bsls::Types::Uint64& maxJournalFileSize();
    // Return a reference to the modifiable "MaxJournalFileSize" attribute
    // of this object.

    bsls::Types::Uint64& maxQlistFileSize();
    // Return a reference to the modifiable "MaxQlistFileSize" attribute of
    // this object.

    bool& preallocate();
    // Return a reference to the modifiable "Preallocate" attribute of this
    // object.

    int& maxArchivedFileSets();
    // Return a reference to the modifiable "MaxArchivedFileSets" attribute
    // of this object.

    bool& prefaultPages();
    // Return a reference to the modifiable "PrefaultPages" attribute of
    // this object.

    bool& flushAtShutdown();
    // Return a reference to the modifiable "FlushAtShutdown" attribute of
    // this object.

    StorageSyncConfig& syncConfig();
    // Return a reference to the modifiable "SyncConfig" attribute of this
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

    int numPartitions() const;
    // Return the value of the "NumPartitions" attribute of this object.

    const bsl::string& location() const;
    // Return a reference offering non-modifiable access to the "Location"
    // attribute of this object.

    const bsl::string& archiveLocation() const;
    // Return a reference offering non-modifiable access to the
    // "ArchiveLocation" attribute of this object.

    bsls::Types::Uint64 maxDataFileSize() const;
    // Return the value of the "MaxDataFileSize" attribute of this object.

    bsls::Types::Uint64 maxJournalFileSize() const;
    // Return the value of the "MaxJournalFileSize" attribute of this
    // object.

    bsls::Types::Uint64 maxQlistFileSize() const;
    // Return the value of the "MaxQlistFileSize" attribute of this object.

    bool preallocate() const;
    // Return the value of the "Preallocate" attribute of this object.

    int maxArchivedFileSets() const;
    // Return the value of the "MaxArchivedFileSets" attribute of this
    // object.

    bool prefaultPages() const;
    // Return the value of the "PrefaultPages" attribute of this object.

    bool flushAtShutdown() const;
    // Return the value of the "FlushAtShutdown" attribute of this object.

    const StorageSyncConfig& syncConfig() const;
    // Return a reference offering non-modifiable access to the
    // "SyncConfig" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const PartitionConfig& lhs,
                           const PartitionConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const PartitionConfig& lhs,
                           const PartitionConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&          stream,
                                    const PartitionConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&      hashAlg,
                           const PartitionConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'PartitionConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::PartitionConfig)

namespace mqbcfg {

// ================================
// class StatPluginConfigPrometheus
// ================================

class StatPluginConfigPrometheus {
    // INSTANCE DATA
    bsl::string       d_host;
    int               d_port;
    ExportMode::Value d_mode;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_MODE = 0,
        ATTRIBUTE_ID_HOST = 1,
        ATTRIBUTE_ID_PORT = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_MODE = 0,
        ATTRIBUTE_INDEX_HOST = 1,
        ATTRIBUTE_INDEX_PORT = 2
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const ExportMode::Value DEFAULT_INITIALIZER_MODE;

    static const char DEFAULT_INITIALIZER_HOST[];

    static const int DEFAULT_INITIALIZER_PORT;

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
    explicit StatPluginConfigPrometheus(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatPluginConfigPrometheus' having the
    // default value.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

    StatPluginConfigPrometheus(const StatPluginConfigPrometheus& original,
                               bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatPluginConfigPrometheus' having the
    // value of the specified 'original' object.  Use the optionally
    // specified 'basicAllocator' to supply memory.  If 'basicAllocator' is
    // 0, the currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatPluginConfigPrometheus(StatPluginConfigPrometheus&& original) noexcept;
    // Create an object of type 'StatPluginConfigPrometheus' having the
    // value of the specified 'original' object.  After performing this
    // action, the 'original' object will be left in a valid, but
    // unspecified state.

    StatPluginConfigPrometheus(StatPluginConfigPrometheus&& original,
                               bslma::Allocator*            basicAllocator);
    // Create an object of type 'StatPluginConfigPrometheus' having the
    // value of the specified 'original' object.  After performing this
    // action, the 'original' object will be left in a valid, but
    // unspecified state.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.
#endif

    ~StatPluginConfigPrometheus();
    // Destroy this object.

    // MANIPULATORS
    StatPluginConfigPrometheus&
    operator=(const StatPluginConfigPrometheus& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatPluginConfigPrometheus& operator=(StatPluginConfigPrometheus&& rhs);
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

    ExportMode::Value& mode();
    // Return a reference to the modifiable "Mode" attribute of this
    // object.

    bsl::string& host();
    // Return a reference to the modifiable "Host" attribute of this
    // object.

    int& port();
    // Return a reference to the modifiable "Port" attribute of this
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

    ExportMode::Value mode() const;
    // Return the value of the "Mode" attribute of this object.

    const bsl::string& host() const;
    // Return a reference offering non-modifiable access to the "Host"
    // attribute of this object.

    int port() const;
    // Return the value of the "Port" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatPluginConfigPrometheus& lhs,
                           const StatPluginConfigPrometheus& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.mode() == rhs.mode() && lhs.host() == rhs.host() &&
               lhs.port() == rhs.port();
    }

    friend bool operator!=(const StatPluginConfigPrometheus& lhs,
                           const StatPluginConfigPrometheus& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                     stream,
                                    const StatPluginConfigPrometheus& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                 hashAlg,
                           const StatPluginConfigPrometheus& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StatPluginConfigPrometheus'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::StatPluginConfigPrometheus)

namespace mqbcfg {

// =================
// class ClusterNode
// =================

class ClusterNode {
    // Type representing the configuration of a node in a cluster.
    // id.........: the unique ID of that node in the cluster; must be a > 0
    // value name.......: name of this node datacenter.: the datacenter of that
    // node transport..: the transport configuration for establishing
    // connectivity with the node

    // INSTANCE DATA
    bsl::string           d_name;
    bsl::string           d_dataCenter;
    ClusterNodeConnection d_transport;
    int                   d_id;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const ClusterNode& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_ID          = 0,
        ATTRIBUTE_ID_NAME        = 1,
        ATTRIBUTE_ID_DATA_CENTER = 2,
        ATTRIBUTE_ID_TRANSPORT   = 3
    };

    enum { NUM_ATTRIBUTES = 4 };

    enum {
        ATTRIBUTE_INDEX_ID          = 0,
        ATTRIBUTE_INDEX_NAME        = 1,
        ATTRIBUTE_INDEX_DATA_CENTER = 2,
        ATTRIBUTE_INDEX_TRANSPORT   = 3
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
    explicit ClusterNode(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ClusterNode' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    ClusterNode(const ClusterNode& original,
                bslma::Allocator*  basicAllocator = 0);
    // Create an object of type 'ClusterNode' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClusterNode(ClusterNode&& original) noexcept;
    // Create an object of type 'ClusterNode' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    ClusterNode(ClusterNode&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'ClusterNode' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~ClusterNode();
    // Destroy this object.

    // MANIPULATORS
    ClusterNode& operator=(const ClusterNode& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClusterNode& operator=(ClusterNode&& rhs);
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

    int& id();
    // Return a reference to the modifiable "Id" attribute of this object.

    bsl::string& name();
    // Return a reference to the modifiable "Name" attribute of this
    // object.

    bsl::string& dataCenter();
    // Return a reference to the modifiable "DataCenter" attribute of this
    // object.

    ClusterNodeConnection& transport();
    // Return a reference to the modifiable "Transport" attribute of this
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

    int id() const;
    // Return the value of the "Id" attribute of this object.

    const bsl::string& name() const;
    // Return a reference offering non-modifiable access to the "Name"
    // attribute of this object.

    const bsl::string& dataCenter() const;
    // Return a reference offering non-modifiable access to the
    // "DataCenter" attribute of this object.

    const ClusterNodeConnection& transport() const;
    // Return a reference offering non-modifiable access to the "Transport"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ClusterNode& lhs, const ClusterNode& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const ClusterNode& lhs, const ClusterNode& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&      stream,
                                    const ClusterNode& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&  hashAlg,
                           const ClusterNode& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'ClusterNode'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::ClusterNode)

namespace mqbcfg {

// ======================
// class DispatcherConfig
// ======================

class DispatcherConfig {
    // INSTANCE DATA
    DispatcherProcessorConfig d_sessions;
    DispatcherProcessorConfig d_queues;
    DispatcherProcessorConfig d_clusters;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_SESSIONS = 0,
        ATTRIBUTE_ID_QUEUES   = 1,
        ATTRIBUTE_ID_CLUSTERS = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_SESSIONS = 0,
        ATTRIBUTE_INDEX_QUEUES   = 1,
        ATTRIBUTE_INDEX_CLUSTERS = 2
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
    DispatcherConfig();
    // Create an object of type 'DispatcherConfig' having the default
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

    DispatcherProcessorConfig& sessions();
    // Return a reference to the modifiable "Sessions" attribute of this
    // object.

    DispatcherProcessorConfig& queues();
    // Return a reference to the modifiable "Queues" attribute of this
    // object.

    DispatcherProcessorConfig& clusters();
    // Return a reference to the modifiable "Clusters" attribute of this
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

    const DispatcherProcessorConfig& sessions() const;
    // Return a reference offering non-modifiable access to the "Sessions"
    // attribute of this object.

    const DispatcherProcessorConfig& queues() const;
    // Return a reference offering non-modifiable access to the "Queues"
    // attribute of this object.

    const DispatcherProcessorConfig& clusters() const;
    // Return a reference offering non-modifiable access to the "Clusters"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DispatcherConfig& lhs,
                           const DispatcherConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.sessions() == rhs.sessions() &&
               lhs.queues() == rhs.queues() &&
               lhs.clusters() == rhs.clusters();
    }

    friend bool operator!=(const DispatcherConfig& lhs,
                           const DispatcherConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const DispatcherConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const DispatcherConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DispatcherConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::DispatcherConfig)

namespace mqbcfg {

// ===============================
// class ReversedClusterConnection
// ===============================

class ReversedClusterConnection {
    // Type representing the configuration for remote cluster connections..
    // name.............: name of the cluster connections......: list of
    // connections to establish

    // INSTANCE DATA
    bsl::vector<ClusterNodeConnection> d_connections;
    bsl::string                        d_name;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_NAME = 0, ATTRIBUTE_ID_CONNECTIONS = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_NAME = 0, ATTRIBUTE_INDEX_CONNECTIONS = 1 };

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
    explicit ReversedClusterConnection(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ReversedClusterConnection' having the
    // default value.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

    ReversedClusterConnection(const ReversedClusterConnection& original,
                              bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ReversedClusterConnection' having the
    // value of the specified 'original' object.  Use the optionally
    // specified 'basicAllocator' to supply memory.  If 'basicAllocator' is
    // 0, the currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ReversedClusterConnection(ReversedClusterConnection&& original) noexcept;
    // Create an object of type 'ReversedClusterConnection' having the
    // value of the specified 'original' object.  After performing this
    // action, the 'original' object will be left in a valid, but
    // unspecified state.

    ReversedClusterConnection(ReversedClusterConnection&& original,
                              bslma::Allocator*           basicAllocator);
    // Create an object of type 'ReversedClusterConnection' having the
    // value of the specified 'original' object.  After performing this
    // action, the 'original' object will be left in a valid, but
    // unspecified state.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.
#endif

    ~ReversedClusterConnection();
    // Destroy this object.

    // MANIPULATORS
    ReversedClusterConnection& operator=(const ReversedClusterConnection& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ReversedClusterConnection& operator=(ReversedClusterConnection&& rhs);
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

    bsl::vector<ClusterNodeConnection>& connections();
    // Return a reference to the modifiable "Connections" attribute of this
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

    const bsl::vector<ClusterNodeConnection>& connections() const;
    // Return a reference offering non-modifiable access to the
    // "Connections" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ReversedClusterConnection& lhs,
                           const ReversedClusterConnection& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.name() == rhs.name() &&
               lhs.connections() == rhs.connections();
    }

    friend bool operator!=(const ReversedClusterConnection& lhs,
                           const ReversedClusterConnection& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                    stream,
                                    const ReversedClusterConnection& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                hashAlg,
                           const ReversedClusterConnection& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ReversedClusterConnection'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.name());
        hashAppend(hashAlg, object.connections());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ReversedClusterConnection)

namespace mqbcfg {

// ======================
// class StatPluginConfig
// ======================

class StatPluginConfig {
    // INSTANCE DATA
    bsl::vector<bsl::string>                        d_hosts;
    bsl::string                                     d_name;
    bsl::string                                     d_namespacePrefix;
    bsl::string                                     d_instanceId;
    bdlb::NullableValue<StatPluginConfigPrometheus> d_prometheusSpecific;
    int                                             d_queueSize;
    int                                             d_queueHighWatermark;
    int                                             d_queueLowWatermark;
    int                                             d_publishInterval;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const StatPluginConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_NAME                 = 0,
        ATTRIBUTE_ID_QUEUE_SIZE           = 1,
        ATTRIBUTE_ID_QUEUE_HIGH_WATERMARK = 2,
        ATTRIBUTE_ID_QUEUE_LOW_WATERMARK  = 3,
        ATTRIBUTE_ID_PUBLISH_INTERVAL     = 4,
        ATTRIBUTE_ID_NAMESPACE_PREFIX     = 5,
        ATTRIBUTE_ID_HOSTS                = 6,
        ATTRIBUTE_ID_INSTANCE_ID          = 7,
        ATTRIBUTE_ID_PROMETHEUS_SPECIFIC  = 8
    };

    enum { NUM_ATTRIBUTES = 9 };

    enum {
        ATTRIBUTE_INDEX_NAME                 = 0,
        ATTRIBUTE_INDEX_QUEUE_SIZE           = 1,
        ATTRIBUTE_INDEX_QUEUE_HIGH_WATERMARK = 2,
        ATTRIBUTE_INDEX_QUEUE_LOW_WATERMARK  = 3,
        ATTRIBUTE_INDEX_PUBLISH_INTERVAL     = 4,
        ATTRIBUTE_INDEX_NAMESPACE_PREFIX     = 5,
        ATTRIBUTE_INDEX_HOSTS                = 6,
        ATTRIBUTE_INDEX_INSTANCE_ID          = 7,
        ATTRIBUTE_INDEX_PROMETHEUS_SPECIFIC  = 8
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_NAME[];

    static const int DEFAULT_INITIALIZER_QUEUE_SIZE;

    static const int DEFAULT_INITIALIZER_QUEUE_HIGH_WATERMARK;

    static const int DEFAULT_INITIALIZER_QUEUE_LOW_WATERMARK;

    static const int DEFAULT_INITIALIZER_PUBLISH_INTERVAL;

    static const char DEFAULT_INITIALIZER_NAMESPACE_PREFIX[];

    static const char DEFAULT_INITIALIZER_INSTANCE_ID[];

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
    explicit StatPluginConfig(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatPluginConfig' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    StatPluginConfig(const StatPluginConfig& original,
                     bslma::Allocator*       basicAllocator = 0);
    // Create an object of type 'StatPluginConfig' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatPluginConfig(StatPluginConfig&& original) noexcept;
    // Create an object of type 'StatPluginConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    StatPluginConfig(StatPluginConfig&& original,
                     bslma::Allocator*  basicAllocator);
    // Create an object of type 'StatPluginConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~StatPluginConfig();
    // Destroy this object.

    // MANIPULATORS
    StatPluginConfig& operator=(const StatPluginConfig& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatPluginConfig& operator=(StatPluginConfig&& rhs);
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

    int& queueSize();
    // Return a reference to the modifiable "QueueSize" attribute of this
    // object.

    int& queueHighWatermark();
    // Return a reference to the modifiable "QueueHighWatermark" attribute
    // of this object.

    int& queueLowWatermark();
    // Return a reference to the modifiable "QueueLowWatermark" attribute
    // of this object.

    int& publishInterval();
    // Return a reference to the modifiable "PublishInterval" attribute of
    // this object.

    bsl::string& namespacePrefix();
    // Return a reference to the modifiable "NamespacePrefix" attribute of
    // this object.

    bsl::vector<bsl::string>& hosts();
    // Return a reference to the modifiable "Hosts" attribute of this
    // object.

    bsl::string& instanceId();
    // Return a reference to the modifiable "InstanceId" attribute of this
    // object.

    bdlb::NullableValue<StatPluginConfigPrometheus>& prometheusSpecific();
    // Return a reference to the modifiable "PrometheusSpecific" attribute
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

    const bsl::string& name() const;
    // Return a reference offering non-modifiable access to the "Name"
    // attribute of this object.

    int queueSize() const;
    // Return the value of the "QueueSize" attribute of this object.

    int queueHighWatermark() const;
    // Return the value of the "QueueHighWatermark" attribute of this
    // object.

    int queueLowWatermark() const;
    // Return the value of the "QueueLowWatermark" attribute of this
    // object.

    int publishInterval() const;
    // Return the value of the "PublishInterval" attribute of this object.

    const bsl::string& namespacePrefix() const;
    // Return a reference offering non-modifiable access to the
    // "NamespacePrefix" attribute of this object.

    const bsl::vector<bsl::string>& hosts() const;
    // Return a reference offering non-modifiable access to the "Hosts"
    // attribute of this object.

    const bsl::string& instanceId() const;
    // Return a reference offering non-modifiable access to the
    // "InstanceId" attribute of this object.

    const bdlb::NullableValue<StatPluginConfigPrometheus>&
    prometheusSpecific() const;
    // Return a reference offering non-modifiable access to the
    // "PrometheusSpecific" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatPluginConfig& lhs,
                           const StatPluginConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const StatPluginConfig& lhs,
                           const StatPluginConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const StatPluginConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const StatPluginConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StatPluginConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::StatPluginConfig)

namespace mqbcfg {

// ================
// class TaskConfig
// ================

class TaskConfig {
    // INSTANCE DATA
    bsls::Types::Uint64  d_allocationLimit;
    LogController        d_logController;
    AllocatorType::Value d_allocatorType;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_ALLOCATOR_TYPE   = 0,
        ATTRIBUTE_ID_ALLOCATION_LIMIT = 1,
        ATTRIBUTE_ID_LOG_CONTROLLER   = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_ALLOCATOR_TYPE   = 0,
        ATTRIBUTE_INDEX_ALLOCATION_LIMIT = 1,
        ATTRIBUTE_INDEX_LOG_CONTROLLER   = 2
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
    explicit TaskConfig(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'TaskConfig' having the default value.  Use
    // the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    TaskConfig(const TaskConfig& original,
               bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'TaskConfig' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    TaskConfig(TaskConfig&& original) noexcept;
    // Create an object of type 'TaskConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    TaskConfig(TaskConfig&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'TaskConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~TaskConfig();
    // Destroy this object.

    // MANIPULATORS
    TaskConfig& operator=(const TaskConfig& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    TaskConfig& operator=(TaskConfig&& rhs);
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

    AllocatorType::Value& allocatorType();
    // Return a reference to the modifiable "AllocatorType" attribute of
    // this object.

    bsls::Types::Uint64& allocationLimit();
    // Return a reference to the modifiable "AllocationLimit" attribute of
    // this object.

    LogController& logController();
    // Return a reference to the modifiable "LogController" attribute of
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

    AllocatorType::Value allocatorType() const;
    // Return the value of the "AllocatorType" attribute of this object.

    bsls::Types::Uint64 allocationLimit() const;
    // Return the value of the "AllocationLimit" attribute of this object.

    const LogController& logController() const;
    // Return a reference offering non-modifiable access to the
    // "LogController" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const TaskConfig& lhs, const TaskConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.allocatorType() == rhs.allocatorType() &&
               lhs.allocationLimit() == rhs.allocationLimit() &&
               lhs.logController() == rhs.logController();
    }

    friend bool operator!=(const TaskConfig& lhs, const TaskConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&     stream,
                                    const TaskConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const TaskConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'TaskConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::TaskConfig)

namespace mqbcfg {

// =======================
// class ClusterDefinition
// =======================

class ClusterDefinition {
    // Type representing the configuration for a cluster.
    // name..................: name of the cluster nodes.................: list
    // of nodes in the cluster partitionConfig.......: configuration for the
    // storage masterAssignment......: algorithm to use for partition's master
    // assignment elector...............: configuration for leader election
    // amongst the nodes queueOperations.......: configuration for queue
    // operations on the cluster clusterAttributes.....: attributes specific to
    // this cluster clusterMonitorConfig..: configuration for cluster state
    // monitor messageThrottleConfig.: configuration for message throttling
    // intervals and thresholds.

    // INSTANCE DATA
    bsl::vector<ClusterNode>         d_nodes;
    bsl::string                      d_name;
    QueueOperationsConfig            d_queueOperations;
    PartitionConfig                  d_partitionConfig;
    MessageThrottleConfig            d_messageThrottleConfig;
    ElectorConfig                    d_elector;
    ClusterMonitorConfig             d_clusterMonitorConfig;
    MasterAssignmentAlgorithm::Value d_masterAssignment;
    ClusterAttributes                d_clusterAttributes;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const ClusterDefinition& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_NAME                    = 0,
        ATTRIBUTE_ID_NODES                   = 1,
        ATTRIBUTE_ID_PARTITION_CONFIG        = 2,
        ATTRIBUTE_ID_MASTER_ASSIGNMENT       = 3,
        ATTRIBUTE_ID_ELECTOR                 = 4,
        ATTRIBUTE_ID_QUEUE_OPERATIONS        = 5,
        ATTRIBUTE_ID_CLUSTER_ATTRIBUTES      = 6,
        ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG  = 7,
        ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG = 8
    };

    enum { NUM_ATTRIBUTES = 9 };

    enum {
        ATTRIBUTE_INDEX_NAME                    = 0,
        ATTRIBUTE_INDEX_NODES                   = 1,
        ATTRIBUTE_INDEX_PARTITION_CONFIG        = 2,
        ATTRIBUTE_INDEX_MASTER_ASSIGNMENT       = 3,
        ATTRIBUTE_INDEX_ELECTOR                 = 4,
        ATTRIBUTE_INDEX_QUEUE_OPERATIONS        = 5,
        ATTRIBUTE_INDEX_CLUSTER_ATTRIBUTES      = 6,
        ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG  = 7,
        ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG = 8
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
    explicit ClusterDefinition(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ClusterDefinition' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    ClusterDefinition(const ClusterDefinition& original,
                      bslma::Allocator*        basicAllocator = 0);
    // Create an object of type 'ClusterDefinition' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClusterDefinition(ClusterDefinition&& original) noexcept;
    // Create an object of type 'ClusterDefinition' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    ClusterDefinition(ClusterDefinition&& original,
                      bslma::Allocator*   basicAllocator);
    // Create an object of type 'ClusterDefinition' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~ClusterDefinition();
    // Destroy this object.

    // MANIPULATORS
    ClusterDefinition& operator=(const ClusterDefinition& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClusterDefinition& operator=(ClusterDefinition&& rhs);
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

    bsl::vector<ClusterNode>& nodes();
    // Return a reference to the modifiable "Nodes" attribute of this
    // object.

    PartitionConfig& partitionConfig();
    // Return a reference to the modifiable "PartitionConfig" attribute of
    // this object.

    MasterAssignmentAlgorithm::Value& masterAssignment();
    // Return a reference to the modifiable "MasterAssignment" attribute of
    // this object.

    ElectorConfig& elector();
    // Return a reference to the modifiable "Elector" attribute of this
    // object.

    QueueOperationsConfig& queueOperations();
    // Return a reference to the modifiable "QueueOperations" attribute of
    // this object.

    ClusterAttributes& clusterAttributes();
    // Return a reference to the modifiable "ClusterAttributes" attribute
    // of this object.

    ClusterMonitorConfig& clusterMonitorConfig();
    // Return a reference to the modifiable "ClusterMonitorConfig"
    // attribute of this object.

    MessageThrottleConfig& messageThrottleConfig();
    // Return a reference to the modifiable "MessageThrottleConfig"
    // attribute of this object.

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

    const bsl::vector<ClusterNode>& nodes() const;
    // Return a reference offering non-modifiable access to the "Nodes"
    // attribute of this object.

    const PartitionConfig& partitionConfig() const;
    // Return a reference offering non-modifiable access to the
    // "PartitionConfig" attribute of this object.

    MasterAssignmentAlgorithm::Value masterAssignment() const;
    // Return the value of the "MasterAssignment" attribute of this object.

    const ElectorConfig& elector() const;
    // Return a reference offering non-modifiable access to the "Elector"
    // attribute of this object.

    const QueueOperationsConfig& queueOperations() const;
    // Return a reference offering non-modifiable access to the
    // "QueueOperations" attribute of this object.

    const ClusterAttributes& clusterAttributes() const;
    // Return a reference offering non-modifiable access to the
    // "ClusterAttributes" attribute of this object.

    const ClusterMonitorConfig& clusterMonitorConfig() const;
    // Return a reference offering non-modifiable access to the
    // "ClusterMonitorConfig" attribute of this object.

    const MessageThrottleConfig& messageThrottleConfig() const;
    // Return a reference offering non-modifiable access to the
    // "MessageThrottleConfig" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ClusterDefinition& lhs,
                           const ClusterDefinition& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const ClusterDefinition& lhs,
                           const ClusterDefinition& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const ClusterDefinition& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const ClusterDefinition& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ClusterDefinition'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ClusterDefinition)

namespace mqbcfg {

// ============================
// class ClusterProxyDefinition
// ============================

class ClusterProxyDefinition {
    // Type representing the configuration for a cluster proxy.
    // name..................: name of the cluster nodes.................: list
    // of nodes in the cluster queueOperations.......: configuration for queue
    // operations with the cluster clusterMonitorConfig..: configuration for
    // cluster state monitor messageThrottleConfig.: configuration for message
    // throttling intervals and thresholds.

    // INSTANCE DATA
    bsl::vector<ClusterNode> d_nodes;
    bsl::string              d_name;
    QueueOperationsConfig    d_queueOperations;
    MessageThrottleConfig    d_messageThrottleConfig;
    ClusterMonitorConfig     d_clusterMonitorConfig;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const ClusterProxyDefinition& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_NAME                    = 0,
        ATTRIBUTE_ID_NODES                   = 1,
        ATTRIBUTE_ID_QUEUE_OPERATIONS        = 2,
        ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG  = 3,
        ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG = 4
    };

    enum { NUM_ATTRIBUTES = 5 };

    enum {
        ATTRIBUTE_INDEX_NAME                    = 0,
        ATTRIBUTE_INDEX_NODES                   = 1,
        ATTRIBUTE_INDEX_QUEUE_OPERATIONS        = 2,
        ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG  = 3,
        ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG = 4
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
    explicit ClusterProxyDefinition(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ClusterProxyDefinition' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    ClusterProxyDefinition(const ClusterProxyDefinition& original,
                           bslma::Allocator*             basicAllocator = 0);
    // Create an object of type 'ClusterProxyDefinition' having the value
    // of the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClusterProxyDefinition(ClusterProxyDefinition&& original) noexcept;
    // Create an object of type 'ClusterProxyDefinition' having the value
    // of the specified 'original' object.  After performing this action,
    // the 'original' object will be left in a valid, but unspecified
    // state.

    ClusterProxyDefinition(ClusterProxyDefinition&& original,
                           bslma::Allocator*        basicAllocator);
    // Create an object of type 'ClusterProxyDefinition' having the value
    // of the specified 'original' object.  After performing this action,
    // the 'original' object will be left in a valid, but unspecified
    // state.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.
#endif

    ~ClusterProxyDefinition();
    // Destroy this object.

    // MANIPULATORS
    ClusterProxyDefinition& operator=(const ClusterProxyDefinition& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClusterProxyDefinition& operator=(ClusterProxyDefinition&& rhs);
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

    bsl::vector<ClusterNode>& nodes();
    // Return a reference to the modifiable "Nodes" attribute of this
    // object.

    QueueOperationsConfig& queueOperations();
    // Return a reference to the modifiable "QueueOperations" attribute of
    // this object.

    ClusterMonitorConfig& clusterMonitorConfig();
    // Return a reference to the modifiable "ClusterMonitorConfig"
    // attribute of this object.

    MessageThrottleConfig& messageThrottleConfig();
    // Return a reference to the modifiable "MessageThrottleConfig"
    // attribute of this object.

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

    const bsl::vector<ClusterNode>& nodes() const;
    // Return a reference offering non-modifiable access to the "Nodes"
    // attribute of this object.

    const QueueOperationsConfig& queueOperations() const;
    // Return a reference offering non-modifiable access to the
    // "QueueOperations" attribute of this object.

    const ClusterMonitorConfig& clusterMonitorConfig() const;
    // Return a reference offering non-modifiable access to the
    // "ClusterMonitorConfig" attribute of this object.

    const MessageThrottleConfig& messageThrottleConfig() const;
    // Return a reference offering non-modifiable access to the
    // "MessageThrottleConfig" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ClusterProxyDefinition& lhs,
                           const ClusterProxyDefinition& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const ClusterProxyDefinition& lhs,
                           const ClusterProxyDefinition& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                 stream,
                                    const ClusterProxyDefinition& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&             hashAlg,
                           const ClusterProxyDefinition& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ClusterProxyDefinition'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ClusterProxyDefinition)

namespace mqbcfg {

// =================
// class StatsConfig
// =================

class StatsConfig {
    // INSTANCE DATA
    bsl::vector<bsl::string>      d_appIdTagDomains;
    bsl::vector<StatPluginConfig> d_plugins;
    StatsPrinterConfig            d_printer;
    int                           d_snapshotInterval;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const StatsConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_SNAPSHOT_INTERVAL  = 0,
        ATTRIBUTE_ID_APP_ID_TAG_DOMAINS = 1,
        ATTRIBUTE_ID_PLUGINS            = 2,
        ATTRIBUTE_ID_PRINTER            = 3
    };

    enum { NUM_ATTRIBUTES = 4 };

    enum {
        ATTRIBUTE_INDEX_SNAPSHOT_INTERVAL  = 0,
        ATTRIBUTE_INDEX_APP_ID_TAG_DOMAINS = 1,
        ATTRIBUTE_INDEX_PLUGINS            = 2,
        ATTRIBUTE_INDEX_PRINTER            = 3
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_SNAPSHOT_INTERVAL;

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
    explicit StatsConfig(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatsConfig' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    StatsConfig(const StatsConfig& original,
                bslma::Allocator*  basicAllocator = 0);
    // Create an object of type 'StatsConfig' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatsConfig(StatsConfig&& original) noexcept;
    // Create an object of type 'StatsConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    StatsConfig(StatsConfig&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'StatsConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~StatsConfig();
    // Destroy this object.

    // MANIPULATORS
    StatsConfig& operator=(const StatsConfig& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatsConfig& operator=(StatsConfig&& rhs);
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

    int& snapshotInterval();
    // Return a reference to the modifiable "SnapshotInterval" attribute of
    // this object.

    bsl::vector<bsl::string>& appIdTagDomains();
    // Return a reference to the modifiable "AppIdTagDomains" attribute of
    // this object.

    bsl::vector<StatPluginConfig>& plugins();
    // Return a reference to the modifiable "Plugins" attribute of this
    // object.

    StatsPrinterConfig& printer();
    // Return a reference to the modifiable "Printer" attribute of this
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

    int snapshotInterval() const;
    // Return the value of the "SnapshotInterval" attribute of this object.

    const bsl::vector<bsl::string>& appIdTagDomains() const;
    // Return a reference offering non-modifiable access to the
    // "AppIdTagDomains" attribute of this object.

    const bsl::vector<StatPluginConfig>& plugins() const;
    // Return a reference offering non-modifiable access to the "Plugins"
    // attribute of this object.

    const StatsPrinterConfig& printer() const;
    // Return a reference offering non-modifiable access to the "Printer"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatsConfig& lhs, const StatsConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const StatsConfig& lhs, const StatsConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&      stream,
                                    const StatsConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&  hashAlg,
                           const StatsConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'StatsConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::StatsConfig)

namespace mqbcfg {

// ===============
// class AppConfig
// ===============

class AppConfig {
    // Top level typ for the broker's configuration.
    // brokerInstanceName...: name of the broker instance
    // brokerVersion........: version of the broker configVersion........:
    // version of the bmqbrkr.cfg config etcDir...............: directory
    // containing the json config files hostName.............: name of the
    // current host hostTags.............: tags of the current host
    // hostDataCenter.......: datacenter the current host resides in
    // isRunningOnDev.......: true if running on dev logsObserverMaxSize..:
    // maximum number of log records to keep latencyMonitorDomain.: common part
    // of all latemon domains dispatcherConfig.....: configuration for the
    // dispatcher stats................: configuration for the stats
    // networkInterfaces....: configuration for the network interfaces
    // bmqconfConfig........: configuration for bmqconf plugins..............:
    // configuration for the plugins msgPropertiesSupport.: information about
    // if/how to advertise support for v2 message properties
    // configureStream......: send new ConfigureStream instead of old
    // ConfigureQueue advertiseSubscriptions.: temporarily control use of
    // ConfigureStream in SDK/>

    // INSTANCE DATA
    bsl::string         d_brokerInstanceName;
    bsl::string         d_etcDir;
    bsl::string         d_hostName;
    bsl::string         d_hostTags;
    bsl::string         d_hostDataCenter;
    bsl::string         d_latencyMonitorDomain;
    StatsConfig         d_stats;
    Plugins             d_plugins;
    NetworkInterfaces   d_networkInterfaces;
    MessagePropertiesV2 d_messagePropertiesV2;
    DispatcherConfig    d_dispatcherConfig;
    BmqconfConfig       d_bmqconfConfig;
    int                 d_brokerVersion;
    int                 d_configVersion;
    int                 d_logsObserverMaxSize;
    bool                d_isRunningOnDev;
    bool                d_configureStream;
    bool                d_advertiseSubscriptions;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const AppConfig& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_BROKER_INSTANCE_NAME    = 0,
        ATTRIBUTE_ID_BROKER_VERSION          = 1,
        ATTRIBUTE_ID_CONFIG_VERSION          = 2,
        ATTRIBUTE_ID_ETC_DIR                 = 3,
        ATTRIBUTE_ID_HOST_NAME               = 4,
        ATTRIBUTE_ID_HOST_TAGS               = 5,
        ATTRIBUTE_ID_HOST_DATA_CENTER        = 6,
        ATTRIBUTE_ID_IS_RUNNING_ON_DEV       = 7,
        ATTRIBUTE_ID_LOGS_OBSERVER_MAX_SIZE  = 8,
        ATTRIBUTE_ID_LATENCY_MONITOR_DOMAIN  = 9,
        ATTRIBUTE_ID_DISPATCHER_CONFIG       = 10,
        ATTRIBUTE_ID_STATS                   = 11,
        ATTRIBUTE_ID_NETWORK_INTERFACES      = 12,
        ATTRIBUTE_ID_BMQCONF_CONFIG          = 13,
        ATTRIBUTE_ID_PLUGINS                 = 14,
        ATTRIBUTE_ID_MESSAGE_PROPERTIES_V2   = 15,
        ATTRIBUTE_ID_CONFIGURE_STREAM        = 16,
        ATTRIBUTE_ID_ADVERTISE_SUBSCRIPTIONS = 17
    };

    enum { NUM_ATTRIBUTES = 18 };

    enum {
        ATTRIBUTE_INDEX_BROKER_INSTANCE_NAME    = 0,
        ATTRIBUTE_INDEX_BROKER_VERSION          = 1,
        ATTRIBUTE_INDEX_CONFIG_VERSION          = 2,
        ATTRIBUTE_INDEX_ETC_DIR                 = 3,
        ATTRIBUTE_INDEX_HOST_NAME               = 4,
        ATTRIBUTE_INDEX_HOST_TAGS               = 5,
        ATTRIBUTE_INDEX_HOST_DATA_CENTER        = 6,
        ATTRIBUTE_INDEX_IS_RUNNING_ON_DEV       = 7,
        ATTRIBUTE_INDEX_LOGS_OBSERVER_MAX_SIZE  = 8,
        ATTRIBUTE_INDEX_LATENCY_MONITOR_DOMAIN  = 9,
        ATTRIBUTE_INDEX_DISPATCHER_CONFIG       = 10,
        ATTRIBUTE_INDEX_STATS                   = 11,
        ATTRIBUTE_INDEX_NETWORK_INTERFACES      = 12,
        ATTRIBUTE_INDEX_BMQCONF_CONFIG          = 13,
        ATTRIBUTE_INDEX_PLUGINS                 = 14,
        ATTRIBUTE_INDEX_MESSAGE_PROPERTIES_V2   = 15,
        ATTRIBUTE_INDEX_CONFIGURE_STREAM        = 16,
        ATTRIBUTE_INDEX_ADVERTISE_SUBSCRIPTIONS = 17
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_LATENCY_MONITOR_DOMAIN[];

    static const bool DEFAULT_INITIALIZER_CONFIGURE_STREAM;

    static const bool DEFAULT_INITIALIZER_ADVERTISE_SUBSCRIPTIONS;

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
    explicit AppConfig(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'AppConfig' having the default value.  Use
    // the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    AppConfig(const AppConfig& original, bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'AppConfig' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    AppConfig(AppConfig&& original) noexcept;
    // Create an object of type 'AppConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    AppConfig(AppConfig&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'AppConfig' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~AppConfig();
    // Destroy this object.

    // MANIPULATORS
    AppConfig& operator=(const AppConfig& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    AppConfig& operator=(AppConfig&& rhs);
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

    bsl::string& brokerInstanceName();
    // Return a reference to the modifiable "BrokerInstanceName" attribute
    // of this object.

    int& brokerVersion();
    // Return a reference to the modifiable "BrokerVersion" attribute of
    // this object.

    int& configVersion();
    // Return a reference to the modifiable "ConfigVersion" attribute of
    // this object.

    bsl::string& etcDir();
    // Return a reference to the modifiable "EtcDir" attribute of this
    // object.

    bsl::string& hostName();
    // Return a reference to the modifiable "HostName" attribute of this
    // object.

    bsl::string& hostTags();
    // Return a reference to the modifiable "HostTags" attribute of this
    // object.

    bsl::string& hostDataCenter();
    // Return a reference to the modifiable "HostDataCenter" attribute of
    // this object.

    bool& isRunningOnDev();
    // Return a reference to the modifiable "IsRunningOnDev" attribute of
    // this object.

    int& logsObserverMaxSize();
    // Return a reference to the modifiable "LogsObserverMaxSize" attribute
    // of this object.

    bsl::string& latencyMonitorDomain();
    // Return a reference to the modifiable "LatencyMonitorDomain"
    // attribute of this object.

    DispatcherConfig& dispatcherConfig();
    // Return a reference to the modifiable "DispatcherConfig" attribute of
    // this object.

    StatsConfig& stats();
    // Return a reference to the modifiable "Stats" attribute of this
    // object.

    NetworkInterfaces& networkInterfaces();
    // Return a reference to the modifiable "NetworkInterfaces" attribute
    // of this object.

    BmqconfConfig& bmqconfConfig();
    // Return a reference to the modifiable "BmqconfConfig" attribute of
    // this object.

    Plugins& plugins();
    // Return a reference to the modifiable "Plugins" attribute of this
    // object.

    MessagePropertiesV2& messagePropertiesV2();
    // Return a reference to the modifiable "MessagePropertiesV2" attribute
    // of this object.

    bool& configureStream();
    // Return a reference to the modifiable "ConfigureStream" attribute of
    // this object.

    bool& advertiseSubscriptions();
    // Return a reference to the modifiable "AdvertiseSubscriptions"
    // attribute of this object.

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

    const bsl::string& brokerInstanceName() const;
    // Return a reference offering non-modifiable access to the
    // "BrokerInstanceName" attribute of this object.

    int brokerVersion() const;
    // Return the value of the "BrokerVersion" attribute of this object.

    int configVersion() const;
    // Return the value of the "ConfigVersion" attribute of this object.

    const bsl::string& etcDir() const;
    // Return a reference offering non-modifiable access to the "EtcDir"
    // attribute of this object.

    const bsl::string& hostName() const;
    // Return a reference offering non-modifiable access to the "HostName"
    // attribute of this object.

    const bsl::string& hostTags() const;
    // Return a reference offering non-modifiable access to the "HostTags"
    // attribute of this object.

    const bsl::string& hostDataCenter() const;
    // Return a reference offering non-modifiable access to the
    // "HostDataCenter" attribute of this object.

    bool isRunningOnDev() const;
    // Return the value of the "IsRunningOnDev" attribute of this object.

    int logsObserverMaxSize() const;
    // Return the value of the "LogsObserverMaxSize" attribute of this
    // object.

    const bsl::string& latencyMonitorDomain() const;
    // Return a reference offering non-modifiable access to the
    // "LatencyMonitorDomain" attribute of this object.

    const DispatcherConfig& dispatcherConfig() const;
    // Return a reference offering non-modifiable access to the
    // "DispatcherConfig" attribute of this object.

    const StatsConfig& stats() const;
    // Return a reference offering non-modifiable access to the "Stats"
    // attribute of this object.

    const NetworkInterfaces& networkInterfaces() const;
    // Return a reference offering non-modifiable access to the
    // "NetworkInterfaces" attribute of this object.

    const BmqconfConfig& bmqconfConfig() const;
    // Return a reference offering non-modifiable access to the
    // "BmqconfConfig" attribute of this object.

    const Plugins& plugins() const;
    // Return a reference offering non-modifiable access to the "Plugins"
    // attribute of this object.

    const MessagePropertiesV2& messagePropertiesV2() const;
    // Return a reference offering non-modifiable access to the
    // "MessagePropertiesV2" attribute of this object.

    bool configureStream() const;
    // Return the value of the "ConfigureStream" attribute of this object.

    bool advertiseSubscriptions() const;
    // Return the value of the "AdvertiseSubscriptions" attribute of this
    // object.

    // HIDDEN FRIENDS
    friend bool operator==(const AppConfig& lhs, const AppConfig& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const AppConfig& lhs, const AppConfig& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const AppConfig& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const AppConfig& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'AppConfig'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::AppConfig)

namespace mqbcfg {

// ========================
// class ClustersDefinition
// ========================

class ClustersDefinition {
    // Top level type representing the configuration for all clusters.
    // myClusters.................: definition of the clusters the current
    // machine is part of (if any); empty means this broker does not belong to
    // any cluster myReverseClusters..........: name of the clusters (if any)
    // the current machine is expected to receive inbound connections about and
    // therefore should pro-actively create a proxy cluster at startup
    // myVirtualClusters..........: information about all the virtual clusters
    // the current machine is considered to belong to (if any)
    // clusters...................: array of cluster definition
    // reversedClusterConnections.: cluster and associated remote connections
    // that should be established

    // INSTANCE DATA
    bsl::vector<bsl::string>               d_myReverseClusters;
    bsl::vector<VirtualClusterInformation> d_myVirtualClusters;
    bsl::vector<ReversedClusterConnection> d_reversedClusterConnections;
    bsl::vector<ClusterProxyDefinition>    d_proxyClusters;
    bsl::vector<ClusterDefinition>         d_myClusters;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const ClustersDefinition& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_MY_CLUSTERS                  = 0,
        ATTRIBUTE_ID_MY_REVERSE_CLUSTERS          = 1,
        ATTRIBUTE_ID_MY_VIRTUAL_CLUSTERS          = 2,
        ATTRIBUTE_ID_PROXY_CLUSTERS               = 3,
        ATTRIBUTE_ID_REVERSED_CLUSTER_CONNECTIONS = 4
    };

    enum { NUM_ATTRIBUTES = 5 };

    enum {
        ATTRIBUTE_INDEX_MY_CLUSTERS                  = 0,
        ATTRIBUTE_INDEX_MY_REVERSE_CLUSTERS          = 1,
        ATTRIBUTE_INDEX_MY_VIRTUAL_CLUSTERS          = 2,
        ATTRIBUTE_INDEX_PROXY_CLUSTERS               = 3,
        ATTRIBUTE_INDEX_REVERSED_CLUSTER_CONNECTIONS = 4
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
    explicit ClustersDefinition(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ClustersDefinition' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    ClustersDefinition(const ClustersDefinition& original,
                       bslma::Allocator*         basicAllocator = 0);
    // Create an object of type 'ClustersDefinition' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClustersDefinition(ClustersDefinition&& original) noexcept;
    // Create an object of type 'ClustersDefinition' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    ClustersDefinition(ClustersDefinition&& original,
                       bslma::Allocator*    basicAllocator);
    // Create an object of type 'ClustersDefinition' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~ClustersDefinition();
    // Destroy this object.

    // MANIPULATORS
    ClustersDefinition& operator=(const ClustersDefinition& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ClustersDefinition& operator=(ClustersDefinition&& rhs);
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

    bsl::vector<ClusterDefinition>& myClusters();
    // Return a reference to the modifiable "MyClusters" attribute of this
    // object.

    bsl::vector<bsl::string>& myReverseClusters();
    // Return a reference to the modifiable "MyReverseClusters" attribute
    // of this object.

    bsl::vector<VirtualClusterInformation>& myVirtualClusters();
    // Return a reference to the modifiable "MyVirtualClusters" attribute
    // of this object.

    bsl::vector<ClusterProxyDefinition>& proxyClusters();
    // Return a reference to the modifiable "ProxyClusters" attribute of
    // this object.

    bsl::vector<ReversedClusterConnection>& reversedClusterConnections();
    // Return a reference to the modifiable "ReversedClusterConnections"
    // attribute of this object.

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

    const bsl::vector<ClusterDefinition>& myClusters() const;
    // Return a reference offering non-modifiable access to the
    // "MyClusters" attribute of this object.

    const bsl::vector<bsl::string>& myReverseClusters() const;
    // Return a reference offering non-modifiable access to the
    // "MyReverseClusters" attribute of this object.

    const bsl::vector<VirtualClusterInformation>& myVirtualClusters() const;
    // Return a reference offering non-modifiable access to the
    // "MyVirtualClusters" attribute of this object.

    const bsl::vector<ClusterProxyDefinition>& proxyClusters() const;
    // Return a reference offering non-modifiable access to the
    // "ProxyClusters" attribute of this object.

    const bsl::vector<ReversedClusterConnection>&
    reversedClusterConnections() const;
    // Return a reference offering non-modifiable access to the
    // "ReversedClusterConnections" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ClustersDefinition& lhs,
                           const ClustersDefinition& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const ClustersDefinition& lhs,
                           const ClustersDefinition& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const ClustersDefinition& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const ClustersDefinition& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ClustersDefinition'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ClustersDefinition)

namespace mqbcfg {

// ===================
// class Configuration
// ===================

class Configuration {
    // INSTANCE DATA
    TaskConfig d_taskConfig;
    AppConfig  d_appConfig;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_TASK_CONFIG = 0, ATTRIBUTE_ID_APP_CONFIG = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_TASK_CONFIG = 0, ATTRIBUTE_INDEX_APP_CONFIG = 1 };

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
    explicit Configuration(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Configuration' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Configuration(const Configuration& original,
                  bslma::Allocator*    basicAllocator = 0);
    // Create an object of type 'Configuration' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Configuration(Configuration&& original) noexcept;
    // Create an object of type 'Configuration' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    Configuration(Configuration&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Configuration' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Configuration();
    // Destroy this object.

    // MANIPULATORS
    Configuration& operator=(const Configuration& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Configuration& operator=(Configuration&& rhs);
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

    TaskConfig& taskConfig();
    // Return a reference to the modifiable "TaskConfig" attribute of this
    // object.

    AppConfig& appConfig();
    // Return a reference to the modifiable "AppConfig" attribute of this
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

    const TaskConfig& taskConfig() const;
    // Return a reference offering non-modifiable access to the
    // "TaskConfig" attribute of this object.

    const AppConfig& appConfig() const;
    // Return a reference offering non-modifiable access to the "AppConfig"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Configuration& lhs, const Configuration& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.taskConfig() == rhs.taskConfig() &&
               lhs.appConfig() == rhs.appConfig();
    }

    friend bool operator!=(const Configuration& lhs, const Configuration& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const Configuration& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const Configuration& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'Configuration'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.taskConfig());
        hashAppend(hashAlg, object.appConfig());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::Configuration)

//=============================================================================
//                          INLINE DEFINITIONS
//=============================================================================

namespace mqbcfg {

// -------------------
// class AllocatorType
// -------------------

// CLASS METHODS
inline int AllocatorType::fromString(Value* result, const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream& AllocatorType::print(bsl::ostream&        stream,
                                          AllocatorType::Value value)
{
    return stream << toString(value);
}

// -------------------
// class BmqconfConfig
// -------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int BmqconfConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(
        &d_cacheTTLSeconds,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CACHE_T_T_L_SECONDS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int BmqconfConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CACHE_T_T_L_SECONDS: {
        return manipulator(
            &d_cacheTTLSeconds,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CACHE_T_T_L_SECONDS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int BmqconfConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& BmqconfConfig::cacheTTLSeconds()
{
    return d_cacheTTLSeconds;
}

// ACCESSORS
template <typename t_ACCESSOR>
int BmqconfConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_cacheTTLSeconds,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CACHE_T_T_L_SECONDS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int BmqconfConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CACHE_T_T_L_SECONDS: {
        return accessor(
            d_cacheTTLSeconds,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CACHE_T_T_L_SECONDS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int BmqconfConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int BmqconfConfig::cacheTTLSeconds() const
{
    return d_cacheTTLSeconds;
}

// -----------------------
// class ClusterAttributes
// -----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ClusterAttributes::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(
        &d_isCSLModeEnabled,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_C_S_L_MODE_ENABLED]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_isFSMWorkflow,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_F_S_M_WORKFLOW]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ClusterAttributes::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_IS_C_S_L_MODE_ENABLED: {
        return manipulator(
            &d_isCSLModeEnabled,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_C_S_L_MODE_ENABLED]);
    }
    case ATTRIBUTE_ID_IS_F_S_M_WORKFLOW: {
        return manipulator(
            &d_isFSMWorkflow,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_F_S_M_WORKFLOW]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ClusterAttributes::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bool& ClusterAttributes::isCSLModeEnabled()
{
    return d_isCSLModeEnabled;
}

inline bool& ClusterAttributes::isFSMWorkflow()
{
    return d_isFSMWorkflow;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ClusterAttributes::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(
        d_isCSLModeEnabled,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_C_S_L_MODE_ENABLED]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_isFSMWorkflow,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_F_S_M_WORKFLOW]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ClusterAttributes::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_IS_C_S_L_MODE_ENABLED: {
        return accessor(
            d_isCSLModeEnabled,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_C_S_L_MODE_ENABLED]);
    }
    case ATTRIBUTE_ID_IS_F_S_M_WORKFLOW: {
        return accessor(
            d_isFSMWorkflow,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_F_S_M_WORKFLOW]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ClusterAttributes::accessAttribute(t_ACCESSOR& accessor,
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

inline bool ClusterAttributes::isCSLModeEnabled() const
{
    return d_isCSLModeEnabled;
}

inline bool ClusterAttributes::isFSMWorkflow() const
{
    return d_isFSMWorkflow;
}

// --------------------------
// class ClusterMonitorConfig
// --------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void ClusterMonitorConfig::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->maxTimeLeader());
    hashAppend(hashAlgorithm, this->maxTimeMaster());
    hashAppend(hashAlgorithm, this->maxTimeNode());
    hashAppend(hashAlgorithm, this->maxTimeFailover());
    hashAppend(hashAlgorithm, this->thresholdLeader());
    hashAppend(hashAlgorithm, this->thresholdMaster());
    hashAppend(hashAlgorithm, this->thresholdNode());
    hashAppend(hashAlgorithm, this->thresholdFailover());
}

inline bool
ClusterMonitorConfig::isEqualTo(const ClusterMonitorConfig& rhs) const
{
    return this->maxTimeLeader() == rhs.maxTimeLeader() &&
           this->maxTimeMaster() == rhs.maxTimeMaster() &&
           this->maxTimeNode() == rhs.maxTimeNode() &&
           this->maxTimeFailover() == rhs.maxTimeFailover() &&
           this->thresholdLeader() == rhs.thresholdLeader() &&
           this->thresholdMaster() == rhs.thresholdMaster() &&
           this->thresholdNode() == rhs.thresholdNode() &&
           this->thresholdFailover() == rhs.thresholdFailover();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ClusterMonitorConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_maxTimeLeader,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_LEADER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxTimeMaster,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_MASTER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxTimeNode,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_NODE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxTimeFailover,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_FAILOVER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_thresholdLeader,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_LEADER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_thresholdMaster,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_MASTER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_thresholdNode,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_NODE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_thresholdFailover,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_FAILOVER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ClusterMonitorConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
                                              int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MAX_TIME_LEADER: {
        return manipulator(
            &d_maxTimeLeader,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_LEADER]);
    }
    case ATTRIBUTE_ID_MAX_TIME_MASTER: {
        return manipulator(
            &d_maxTimeMaster,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_MASTER]);
    }
    case ATTRIBUTE_ID_MAX_TIME_NODE: {
        return manipulator(
            &d_maxTimeNode,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_NODE]);
    }
    case ATTRIBUTE_ID_MAX_TIME_FAILOVER: {
        return manipulator(
            &d_maxTimeFailover,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_FAILOVER]);
    }
    case ATTRIBUTE_ID_THRESHOLD_LEADER: {
        return manipulator(
            &d_thresholdLeader,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_LEADER]);
    }
    case ATTRIBUTE_ID_THRESHOLD_MASTER: {
        return manipulator(
            &d_thresholdMaster,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_MASTER]);
    }
    case ATTRIBUTE_ID_THRESHOLD_NODE: {
        return manipulator(
            &d_thresholdNode,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_NODE]);
    }
    case ATTRIBUTE_ID_THRESHOLD_FAILOVER: {
        return manipulator(
            &d_thresholdFailover,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_FAILOVER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ClusterMonitorConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& ClusterMonitorConfig::maxTimeLeader()
{
    return d_maxTimeLeader;
}

inline int& ClusterMonitorConfig::maxTimeMaster()
{
    return d_maxTimeMaster;
}

inline int& ClusterMonitorConfig::maxTimeNode()
{
    return d_maxTimeNode;
}

inline int& ClusterMonitorConfig::maxTimeFailover()
{
    return d_maxTimeFailover;
}

inline int& ClusterMonitorConfig::thresholdLeader()
{
    return d_thresholdLeader;
}

inline int& ClusterMonitorConfig::thresholdMaster()
{
    return d_thresholdMaster;
}

inline int& ClusterMonitorConfig::thresholdNode()
{
    return d_thresholdNode;
}

inline int& ClusterMonitorConfig::thresholdFailover()
{
    return d_thresholdFailover;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ClusterMonitorConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_maxTimeLeader,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_LEADER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxTimeMaster,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_MASTER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxTimeNode,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_NODE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxTimeFailover,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_FAILOVER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_thresholdLeader,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_LEADER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_thresholdMaster,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_MASTER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_thresholdNode,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_NODE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_thresholdFailover,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_FAILOVER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ClusterMonitorConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MAX_TIME_LEADER: {
        return accessor(d_maxTimeLeader,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_LEADER]);
    }
    case ATTRIBUTE_ID_MAX_TIME_MASTER: {
        return accessor(d_maxTimeMaster,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_MASTER]);
    }
    case ATTRIBUTE_ID_MAX_TIME_NODE: {
        return accessor(d_maxTimeNode,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_NODE]);
    }
    case ATTRIBUTE_ID_MAX_TIME_FAILOVER: {
        return accessor(
            d_maxTimeFailover,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_FAILOVER]);
    }
    case ATTRIBUTE_ID_THRESHOLD_LEADER: {
        return accessor(
            d_thresholdLeader,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_LEADER]);
    }
    case ATTRIBUTE_ID_THRESHOLD_MASTER: {
        return accessor(
            d_thresholdMaster,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_MASTER]);
    }
    case ATTRIBUTE_ID_THRESHOLD_NODE: {
        return accessor(d_thresholdNode,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_NODE]);
    }
    case ATTRIBUTE_ID_THRESHOLD_FAILOVER: {
        return accessor(
            d_thresholdFailover,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_FAILOVER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ClusterMonitorConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int ClusterMonitorConfig::maxTimeLeader() const
{
    return d_maxTimeLeader;
}

inline int ClusterMonitorConfig::maxTimeMaster() const
{
    return d_maxTimeMaster;
}

inline int ClusterMonitorConfig::maxTimeNode() const
{
    return d_maxTimeNode;
}

inline int ClusterMonitorConfig::maxTimeFailover() const
{
    return d_maxTimeFailover;
}

inline int ClusterMonitorConfig::thresholdLeader() const
{
    return d_thresholdLeader;
}

inline int ClusterMonitorConfig::thresholdMaster() const
{
    return d_thresholdMaster;
}

inline int ClusterMonitorConfig::thresholdNode() const
{
    return d_thresholdNode;
}

inline int ClusterMonitorConfig::thresholdFailover() const
{
    return d_thresholdFailover;
}

// -----------------------------------
// class DispatcherProcessorParameters
// -----------------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void DispatcherProcessorParameters::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->queueSize());
    hashAppend(hashAlgorithm, this->queueSizeLowWatermark());
    hashAppend(hashAlgorithm, this->queueSizeHighWatermark());
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int DispatcherProcessorParameters::manipulateAttributes(
    t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_queueSize,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_queueSizeLowWatermark,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE_LOW_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_queueSizeHighWatermark,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE_HIGH_WATERMARK]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int DispatcherProcessorParameters::manipulateAttribute(
    t_MANIPULATOR& manipulator,
    int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_QUEUE_SIZE: {
        return manipulator(&d_queueSize,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE]);
    }
    case ATTRIBUTE_ID_QUEUE_SIZE_LOW_WATERMARK: {
        return manipulator(
            &d_queueSizeLowWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE_LOW_WATERMARK]);
    }
    case ATTRIBUTE_ID_QUEUE_SIZE_HIGH_WATERMARK: {
        return manipulator(
            &d_queueSizeHighWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE_HIGH_WATERMARK]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int DispatcherProcessorParameters::manipulateAttribute(
    t_MANIPULATOR& manipulator,
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

inline int& DispatcherProcessorParameters::queueSize()
{
    return d_queueSize;
}

inline int& DispatcherProcessorParameters::queueSizeLowWatermark()
{
    return d_queueSizeLowWatermark;
}

inline int& DispatcherProcessorParameters::queueSizeHighWatermark()
{
    return d_queueSizeHighWatermark;
}

// ACCESSORS
template <typename t_ACCESSOR>
int DispatcherProcessorParameters::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_queueSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_queueSizeLowWatermark,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE_LOW_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_queueSizeHighWatermark,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE_HIGH_WATERMARK]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int DispatcherProcessorParameters::accessAttribute(t_ACCESSOR& accessor,
                                                   int         id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_QUEUE_SIZE: {
        return accessor(d_queueSize,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE]);
    }
    case ATTRIBUTE_ID_QUEUE_SIZE_LOW_WATERMARK: {
        return accessor(
            d_queueSizeLowWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE_LOW_WATERMARK]);
    }
    case ATTRIBUTE_ID_QUEUE_SIZE_HIGH_WATERMARK: {
        return accessor(
            d_queueSizeHighWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE_HIGH_WATERMARK]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int DispatcherProcessorParameters::accessAttribute(t_ACCESSOR& accessor,
                                                   const char* name,
                                                   int nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline int DispatcherProcessorParameters::queueSize() const
{
    return d_queueSize;
}

inline int DispatcherProcessorParameters::queueSizeLowWatermark() const
{
    return d_queueSizeLowWatermark;
}

inline int DispatcherProcessorParameters::queueSizeHighWatermark() const
{
    return d_queueSizeHighWatermark;
}

// -------------------
// class ElectorConfig
// -------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void ElectorConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->initialWaitTimeoutMs());
    hashAppend(hashAlgorithm, this->maxRandomWaitTimeoutMs());
    hashAppend(hashAlgorithm, this->scoutingResultTimeoutMs());
    hashAppend(hashAlgorithm, this->electionResultTimeoutMs());
    hashAppend(hashAlgorithm, this->heartbeatBroadcastPeriodMs());
    hashAppend(hashAlgorithm, this->heartbeatCheckPeriodMs());
    hashAppend(hashAlgorithm, this->heartbeatMissCount());
    hashAppend(hashAlgorithm, this->quorum());
    hashAppend(hashAlgorithm, this->leaderSyncDelayMs());
}

inline bool ElectorConfig::isEqualTo(const ElectorConfig& rhs) const
{
    return this->initialWaitTimeoutMs() == rhs.initialWaitTimeoutMs() &&
           this->maxRandomWaitTimeoutMs() == rhs.maxRandomWaitTimeoutMs() &&
           this->scoutingResultTimeoutMs() == rhs.scoutingResultTimeoutMs() &&
           this->electionResultTimeoutMs() == rhs.electionResultTimeoutMs() &&
           this->heartbeatBroadcastPeriodMs() ==
               rhs.heartbeatBroadcastPeriodMs() &&
           this->heartbeatCheckPeriodMs() == rhs.heartbeatCheckPeriodMs() &&
           this->heartbeatMissCount() == rhs.heartbeatMissCount() &&
           this->quorum() == rhs.quorum() &&
           this->leaderSyncDelayMs() == rhs.leaderSyncDelayMs();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ElectorConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(
        &d_initialWaitTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INITIAL_WAIT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxRandomWaitTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_RANDOM_WAIT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_scoutingResultTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SCOUTING_RESULT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_electionResultTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTION_RESULT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_heartbeatBroadcastPeriodMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_BROADCAST_PERIOD_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_heartbeatCheckPeriodMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_CHECK_PERIOD_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_heartbeatMissCount,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_MISS_COUNT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_quorum, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUORUM]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_leaderSyncDelayMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_SYNC_DELAY_MS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ElectorConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_INITIAL_WAIT_TIMEOUT_MS: {
        return manipulator(
            &d_initialWaitTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INITIAL_WAIT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_MAX_RANDOM_WAIT_TIMEOUT_MS: {
        return manipulator(
            &d_maxRandomWaitTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_RANDOM_WAIT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_SCOUTING_RESULT_TIMEOUT_MS: {
        return manipulator(
            &d_scoutingResultTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SCOUTING_RESULT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_ELECTION_RESULT_TIMEOUT_MS: {
        return manipulator(
            &d_electionResultTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTION_RESULT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_HEARTBEAT_BROADCAST_PERIOD_MS: {
        return manipulator(
            &d_heartbeatBroadcastPeriodMs,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_HEARTBEAT_BROADCAST_PERIOD_MS]);
    }
    case ATTRIBUTE_ID_HEARTBEAT_CHECK_PERIOD_MS: {
        return manipulator(
            &d_heartbeatCheckPeriodMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_CHECK_PERIOD_MS]);
    }
    case ATTRIBUTE_ID_HEARTBEAT_MISS_COUNT: {
        return manipulator(
            &d_heartbeatMissCount,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_MISS_COUNT]);
    }
    case ATTRIBUTE_ID_QUORUM: {
        return manipulator(&d_quorum,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUORUM]);
    }
    case ATTRIBUTE_ID_LEADER_SYNC_DELAY_MS: {
        return manipulator(
            &d_leaderSyncDelayMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_SYNC_DELAY_MS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ElectorConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& ElectorConfig::initialWaitTimeoutMs()
{
    return d_initialWaitTimeoutMs;
}

inline int& ElectorConfig::maxRandomWaitTimeoutMs()
{
    return d_maxRandomWaitTimeoutMs;
}

inline int& ElectorConfig::scoutingResultTimeoutMs()
{
    return d_scoutingResultTimeoutMs;
}

inline int& ElectorConfig::electionResultTimeoutMs()
{
    return d_electionResultTimeoutMs;
}

inline int& ElectorConfig::heartbeatBroadcastPeriodMs()
{
    return d_heartbeatBroadcastPeriodMs;
}

inline int& ElectorConfig::heartbeatCheckPeriodMs()
{
    return d_heartbeatCheckPeriodMs;
}

inline int& ElectorConfig::heartbeatMissCount()
{
    return d_heartbeatMissCount;
}

inline int& ElectorConfig::quorum()
{
    return d_quorum;
}

inline int& ElectorConfig::leaderSyncDelayMs()
{
    return d_leaderSyncDelayMs;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ElectorConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(
        d_initialWaitTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INITIAL_WAIT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxRandomWaitTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_RANDOM_WAIT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_scoutingResultTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SCOUTING_RESULT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_electionResultTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTION_RESULT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_heartbeatBroadcastPeriodMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_BROADCAST_PERIOD_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_heartbeatCheckPeriodMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_CHECK_PERIOD_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_heartbeatMissCount,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_MISS_COUNT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_quorum, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUORUM]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_leaderSyncDelayMs,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_SYNC_DELAY_MS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ElectorConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_INITIAL_WAIT_TIMEOUT_MS: {
        return accessor(
            d_initialWaitTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INITIAL_WAIT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_MAX_RANDOM_WAIT_TIMEOUT_MS: {
        return accessor(
            d_maxRandomWaitTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_RANDOM_WAIT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_SCOUTING_RESULT_TIMEOUT_MS: {
        return accessor(
            d_scoutingResultTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SCOUTING_RESULT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_ELECTION_RESULT_TIMEOUT_MS: {
        return accessor(
            d_electionResultTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTION_RESULT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_HEARTBEAT_BROADCAST_PERIOD_MS: {
        return accessor(d_heartbeatBroadcastPeriodMs,
                        ATTRIBUTE_INFO_ARRAY
                            [ATTRIBUTE_INDEX_HEARTBEAT_BROADCAST_PERIOD_MS]);
    }
    case ATTRIBUTE_ID_HEARTBEAT_CHECK_PERIOD_MS: {
        return accessor(
            d_heartbeatCheckPeriodMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_CHECK_PERIOD_MS]);
    }
    case ATTRIBUTE_ID_HEARTBEAT_MISS_COUNT: {
        return accessor(
            d_heartbeatMissCount,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_MISS_COUNT]);
    }
    case ATTRIBUTE_ID_QUORUM: {
        return accessor(d_quorum,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUORUM]);
    }
    case ATTRIBUTE_ID_LEADER_SYNC_DELAY_MS: {
        return accessor(
            d_leaderSyncDelayMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_SYNC_DELAY_MS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ElectorConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int ElectorConfig::initialWaitTimeoutMs() const
{
    return d_initialWaitTimeoutMs;
}

inline int ElectorConfig::maxRandomWaitTimeoutMs() const
{
    return d_maxRandomWaitTimeoutMs;
}

inline int ElectorConfig::scoutingResultTimeoutMs() const
{
    return d_scoutingResultTimeoutMs;
}

inline int ElectorConfig::electionResultTimeoutMs() const
{
    return d_electionResultTimeoutMs;
}

inline int ElectorConfig::heartbeatBroadcastPeriodMs() const
{
    return d_heartbeatBroadcastPeriodMs;
}

inline int ElectorConfig::heartbeatCheckPeriodMs() const
{
    return d_heartbeatCheckPeriodMs;
}

inline int ElectorConfig::heartbeatMissCount() const
{
    return d_heartbeatMissCount;
}

inline int ElectorConfig::quorum() const
{
    return d_quorum;
}

inline int ElectorConfig::leaderSyncDelayMs() const
{
    return d_leaderSyncDelayMs;
}

// ----------------
// class ExportMode
// ----------------

// CLASS METHODS
inline int ExportMode::fromString(Value* result, const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream& ExportMode::print(bsl::ostream&     stream,
                                       ExportMode::Value value)
{
    return stream << toString(value);
}

// ---------------
// class Heartbeat
// ---------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Heartbeat::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->client());
    hashAppend(hashAlgorithm, this->downstreamBroker());
    hashAppend(hashAlgorithm, this->upstreamBroker());
    hashAppend(hashAlgorithm, this->clusterPeer());
}

inline bool Heartbeat::isEqualTo(const Heartbeat& rhs) const
{
    return this->client() == rhs.client() &&
           this->downstreamBroker() == rhs.downstreamBroker() &&
           this->upstreamBroker() == rhs.upstreamBroker() &&
           this->clusterPeer() == rhs.clusterPeer();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Heartbeat::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_client, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_downstreamBroker,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOWNSTREAM_BROKER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_upstreamBroker,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_UPSTREAM_BROKER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_clusterPeer,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_PEER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Heartbeat::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CLIENT: {
        return manipulator(&d_client,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT]);
    }
    case ATTRIBUTE_ID_DOWNSTREAM_BROKER: {
        return manipulator(
            &d_downstreamBroker,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOWNSTREAM_BROKER]);
    }
    case ATTRIBUTE_ID_UPSTREAM_BROKER: {
        return manipulator(
            &d_upstreamBroker,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_UPSTREAM_BROKER]);
    }
    case ATTRIBUTE_ID_CLUSTER_PEER: {
        return manipulator(&d_clusterPeer,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_PEER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Heartbeat::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& Heartbeat::client()
{
    return d_client;
}

inline int& Heartbeat::downstreamBroker()
{
    return d_downstreamBroker;
}

inline int& Heartbeat::upstreamBroker()
{
    return d_upstreamBroker;
}

inline int& Heartbeat::clusterPeer()
{
    return d_clusterPeer;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Heartbeat::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_client, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_downstreamBroker,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOWNSTREAM_BROKER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_upstreamBroker,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_UPSTREAM_BROKER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_clusterPeer,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_PEER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Heartbeat::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CLIENT: {
        return accessor(d_client,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT]);
    }
    case ATTRIBUTE_ID_DOWNSTREAM_BROKER: {
        return accessor(
            d_downstreamBroker,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOWNSTREAM_BROKER]);
    }
    case ATTRIBUTE_ID_UPSTREAM_BROKER: {
        return accessor(d_upstreamBroker,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_UPSTREAM_BROKER]);
    }
    case ATTRIBUTE_ID_CLUSTER_PEER: {
        return accessor(d_clusterPeer,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_PEER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Heartbeat::accessAttribute(t_ACCESSOR& accessor,
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

inline int Heartbeat::client() const
{
    return d_client;
}

inline int Heartbeat::downstreamBroker() const
{
    return d_downstreamBroker;
}

inline int Heartbeat::upstreamBroker() const
{
    return d_upstreamBroker;
}

inline int Heartbeat::clusterPeer() const
{
    return d_clusterPeer;
}

// -------------------------------
// class MasterAssignmentAlgorithm
// -------------------------------

// CLASS METHODS
inline int MasterAssignmentAlgorithm::fromString(Value*             result,
                                                 const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream&
MasterAssignmentAlgorithm::print(bsl::ostream&                    stream,
                                 MasterAssignmentAlgorithm::Value value)
{
    return stream << toString(value);
}

// -------------------------
// class MessagePropertiesV2
// -------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void MessagePropertiesV2::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->advertiseV2Support());
    hashAppend(hashAlgorithm, this->minCppSdkVersion());
    hashAppend(hashAlgorithm, this->minJavaSdkVersion());
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int MessagePropertiesV2::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(
        &d_advertiseV2Support,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_V2_SUPPORT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_minCppSdkVersion,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_CPP_SDK_VERSION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_minJavaSdkVersion,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_JAVA_SDK_VERSION]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int MessagePropertiesV2::manipulateAttribute(t_MANIPULATOR& manipulator,
                                             int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ADVERTISE_V2_SUPPORT: {
        return manipulator(
            &d_advertiseV2Support,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_V2_SUPPORT]);
    }
    case ATTRIBUTE_ID_MIN_CPP_SDK_VERSION: {
        return manipulator(
            &d_minCppSdkVersion,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_CPP_SDK_VERSION]);
    }
    case ATTRIBUTE_ID_MIN_JAVA_SDK_VERSION: {
        return manipulator(
            &d_minJavaSdkVersion,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_JAVA_SDK_VERSION]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int MessagePropertiesV2::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bool& MessagePropertiesV2::advertiseV2Support()
{
    return d_advertiseV2Support;
}

inline int& MessagePropertiesV2::minCppSdkVersion()
{
    return d_minCppSdkVersion;
}

inline int& MessagePropertiesV2::minJavaSdkVersion()
{
    return d_minJavaSdkVersion;
}

// ACCESSORS
template <typename t_ACCESSOR>
int MessagePropertiesV2::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_advertiseV2Support,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_V2_SUPPORT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_minCppSdkVersion,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_CPP_SDK_VERSION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_minJavaSdkVersion,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_JAVA_SDK_VERSION]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int MessagePropertiesV2::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ADVERTISE_V2_SUPPORT: {
        return accessor(
            d_advertiseV2Support,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_V2_SUPPORT]);
    }
    case ATTRIBUTE_ID_MIN_CPP_SDK_VERSION: {
        return accessor(
            d_minCppSdkVersion,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_CPP_SDK_VERSION]);
    }
    case ATTRIBUTE_ID_MIN_JAVA_SDK_VERSION: {
        return accessor(
            d_minJavaSdkVersion,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_JAVA_SDK_VERSION]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int MessagePropertiesV2::accessAttribute(t_ACCESSOR& accessor,
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

inline bool MessagePropertiesV2::advertiseV2Support() const
{
    return d_advertiseV2Support;
}

inline int MessagePropertiesV2::minCppSdkVersion() const
{
    return d_minCppSdkVersion;
}

inline int MessagePropertiesV2::minJavaSdkVersion() const
{
    return d_minJavaSdkVersion;
}

// ---------------------------
// class MessageThrottleConfig
// ---------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void MessageThrottleConfig::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->lowThreshold());
    hashAppend(hashAlgorithm, this->highThreshold());
    hashAppend(hashAlgorithm, this->lowInterval());
    hashAppend(hashAlgorithm, this->highInterval());
}

inline bool
MessageThrottleConfig::isEqualTo(const MessageThrottleConfig& rhs) const
{
    return this->lowThreshold() == rhs.lowThreshold() &&
           this->highThreshold() == rhs.highThreshold() &&
           this->lowInterval() == rhs.lowInterval() &&
           this->highInterval() == rhs.highInterval();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int MessageThrottleConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_lowThreshold,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_THRESHOLD]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_highThreshold,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_THRESHOLD]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_lowInterval,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_highInterval,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_INTERVAL]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int MessageThrottleConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
                                               int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_LOW_THRESHOLD: {
        return manipulator(
            &d_lowThreshold,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_THRESHOLD]);
    }
    case ATTRIBUTE_ID_HIGH_THRESHOLD: {
        return manipulator(
            &d_highThreshold,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_THRESHOLD]);
    }
    case ATTRIBUTE_ID_LOW_INTERVAL: {
        return manipulator(&d_lowInterval,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_INTERVAL]);
    }
    case ATTRIBUTE_ID_HIGH_INTERVAL: {
        return manipulator(
            &d_highInterval,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_INTERVAL]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int MessageThrottleConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline unsigned int& MessageThrottleConfig::lowThreshold()
{
    return d_lowThreshold;
}

inline unsigned int& MessageThrottleConfig::highThreshold()
{
    return d_highThreshold;
}

inline unsigned int& MessageThrottleConfig::lowInterval()
{
    return d_lowInterval;
}

inline unsigned int& MessageThrottleConfig::highInterval()
{
    return d_highInterval;
}

// ACCESSORS
template <typename t_ACCESSOR>
int MessageThrottleConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_lowThreshold,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_THRESHOLD]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_highThreshold,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_THRESHOLD]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_lowInterval,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_highInterval,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_INTERVAL]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int MessageThrottleConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_LOW_THRESHOLD: {
        return accessor(d_lowThreshold,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_THRESHOLD]);
    }
    case ATTRIBUTE_ID_HIGH_THRESHOLD: {
        return accessor(d_highThreshold,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_THRESHOLD]);
    }
    case ATTRIBUTE_ID_LOW_INTERVAL: {
        return accessor(d_lowInterval,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_INTERVAL]);
    }
    case ATTRIBUTE_ID_HIGH_INTERVAL: {
        return accessor(d_highInterval,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_INTERVAL]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int MessageThrottleConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline unsigned int MessageThrottleConfig::lowThreshold() const
{
    return d_lowThreshold;
}

inline unsigned int MessageThrottleConfig::highThreshold() const
{
    return d_highThreshold;
}

inline unsigned int MessageThrottleConfig::lowInterval() const
{
    return d_lowInterval;
}

inline unsigned int MessageThrottleConfig::highInterval() const
{
    return d_highInterval;
}

// -------------
// class Plugins
// -------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Plugins::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_libraries,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LIBRARIES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_enabled,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Plugins::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_LIBRARIES: {
        return manipulator(&d_libraries,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LIBRARIES]);
    }
    case ATTRIBUTE_ID_ENABLED: {
        return manipulator(&d_enabled,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Plugins::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::vector<bsl::string>& Plugins::libraries()
{
    return d_libraries;
}

inline bsl::vector<bsl::string>& Plugins::enabled()
{
    return d_enabled;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Plugins::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_libraries,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LIBRARIES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_enabled, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Plugins::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_LIBRARIES: {
        return accessor(d_libraries,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LIBRARIES]);
    }
    case ATTRIBUTE_ID_ENABLED: {
        return accessor(d_enabled,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Plugins::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::vector<bsl::string>& Plugins::libraries() const
{
    return d_libraries;
}

inline const bsl::vector<bsl::string>& Plugins::enabled() const
{
    return d_enabled;
}

// ---------------------------
// class QueueOperationsConfig
// ---------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void QueueOperationsConfig::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->openTimeoutMs());
    hashAppend(hashAlgorithm, this->configureTimeoutMs());
    hashAppend(hashAlgorithm, this->closeTimeoutMs());
    hashAppend(hashAlgorithm, this->reopenTimeoutMs());
    hashAppend(hashAlgorithm, this->reopenRetryIntervalMs());
    hashAppend(hashAlgorithm, this->reopenMaxAttempts());
    hashAppend(hashAlgorithm, this->assignmentTimeoutMs());
    hashAppend(hashAlgorithm, this->keepaliveDurationMs());
    hashAppend(hashAlgorithm, this->consumptionMonitorPeriodMs());
    hashAppend(hashAlgorithm, this->stopTimeoutMs());
    hashAppend(hashAlgorithm, this->shutdownTimeoutMs());
    hashAppend(hashAlgorithm, this->ackWindowSize());
}

inline bool
QueueOperationsConfig::isEqualTo(const QueueOperationsConfig& rhs) const
{
    return this->openTimeoutMs() == rhs.openTimeoutMs() &&
           this->configureTimeoutMs() == rhs.configureTimeoutMs() &&
           this->closeTimeoutMs() == rhs.closeTimeoutMs() &&
           this->reopenTimeoutMs() == rhs.reopenTimeoutMs() &&
           this->reopenRetryIntervalMs() == rhs.reopenRetryIntervalMs() &&
           this->reopenMaxAttempts() == rhs.reopenMaxAttempts() &&
           this->assignmentTimeoutMs() == rhs.assignmentTimeoutMs() &&
           this->keepaliveDurationMs() == rhs.keepaliveDurationMs() &&
           this->consumptionMonitorPeriodMs() ==
               rhs.consumptionMonitorPeriodMs() &&
           this->stopTimeoutMs() == rhs.stopTimeoutMs() &&
           this->shutdownTimeoutMs() == rhs.shutdownTimeoutMs() &&
           this->ackWindowSize() == rhs.ackWindowSize();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int QueueOperationsConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_openTimeoutMs,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OPEN_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_configureTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_closeTimeoutMs,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLOSE_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_reopenTimeoutMs,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_reopenRetryIntervalMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_RETRY_INTERVAL_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_reopenMaxAttempts,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_MAX_ATTEMPTS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_assignmentTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASSIGNMENT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_keepaliveDurationMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEEPALIVE_DURATION_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_consumptionMonitorPeriodMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMPTION_MONITOR_PERIOD_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_stopTimeoutMs,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STOP_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_shutdownTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_ackWindowSize,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACK_WINDOW_SIZE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int QueueOperationsConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
                                               int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_OPEN_TIMEOUT_MS: {
        return manipulator(
            &d_openTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OPEN_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_CONFIGURE_TIMEOUT_MS: {
        return manipulator(
            &d_configureTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_CLOSE_TIMEOUT_MS: {
        return manipulator(
            &d_closeTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLOSE_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_REOPEN_TIMEOUT_MS: {
        return manipulator(
            &d_reopenTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_REOPEN_RETRY_INTERVAL_MS: {
        return manipulator(
            &d_reopenRetryIntervalMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_RETRY_INTERVAL_MS]);
    }
    case ATTRIBUTE_ID_REOPEN_MAX_ATTEMPTS: {
        return manipulator(
            &d_reopenMaxAttempts,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_MAX_ATTEMPTS]);
    }
    case ATTRIBUTE_ID_ASSIGNMENT_TIMEOUT_MS: {
        return manipulator(
            &d_assignmentTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASSIGNMENT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_KEEPALIVE_DURATION_MS: {
        return manipulator(
            &d_keepaliveDurationMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEEPALIVE_DURATION_MS]);
    }
    case ATTRIBUTE_ID_CONSUMPTION_MONITOR_PERIOD_MS: {
        return manipulator(
            &d_consumptionMonitorPeriodMs,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_CONSUMPTION_MONITOR_PERIOD_MS]);
    }
    case ATTRIBUTE_ID_STOP_TIMEOUT_MS: {
        return manipulator(
            &d_stopTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STOP_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_SHUTDOWN_TIMEOUT_MS: {
        return manipulator(
            &d_shutdownTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_ACK_WINDOW_SIZE: {
        return manipulator(
            &d_ackWindowSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACK_WINDOW_SIZE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int QueueOperationsConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& QueueOperationsConfig::openTimeoutMs()
{
    return d_openTimeoutMs;
}

inline int& QueueOperationsConfig::configureTimeoutMs()
{
    return d_configureTimeoutMs;
}

inline int& QueueOperationsConfig::closeTimeoutMs()
{
    return d_closeTimeoutMs;
}

inline int& QueueOperationsConfig::reopenTimeoutMs()
{
    return d_reopenTimeoutMs;
}

inline int& QueueOperationsConfig::reopenRetryIntervalMs()
{
    return d_reopenRetryIntervalMs;
}

inline int& QueueOperationsConfig::reopenMaxAttempts()
{
    return d_reopenMaxAttempts;
}

inline int& QueueOperationsConfig::assignmentTimeoutMs()
{
    return d_assignmentTimeoutMs;
}

inline int& QueueOperationsConfig::keepaliveDurationMs()
{
    return d_keepaliveDurationMs;
}

inline int& QueueOperationsConfig::consumptionMonitorPeriodMs()
{
    return d_consumptionMonitorPeriodMs;
}

inline int& QueueOperationsConfig::stopTimeoutMs()
{
    return d_stopTimeoutMs;
}

inline int& QueueOperationsConfig::shutdownTimeoutMs()
{
    return d_shutdownTimeoutMs;
}

inline int& QueueOperationsConfig::ackWindowSize()
{
    return d_ackWindowSize;
}

// ACCESSORS
template <typename t_ACCESSOR>
int QueueOperationsConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_openTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OPEN_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_configureTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_closeTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLOSE_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_reopenTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_reopenRetryIntervalMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_RETRY_INTERVAL_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_reopenMaxAttempts,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_MAX_ATTEMPTS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_assignmentTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASSIGNMENT_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_keepaliveDurationMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEEPALIVE_DURATION_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_consumptionMonitorPeriodMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMPTION_MONITOR_PERIOD_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_stopTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STOP_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_shutdownTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_ackWindowSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACK_WINDOW_SIZE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int QueueOperationsConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_OPEN_TIMEOUT_MS: {
        return accessor(d_openTimeoutMs,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OPEN_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_CONFIGURE_TIMEOUT_MS: {
        return accessor(
            d_configureTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_CLOSE_TIMEOUT_MS: {
        return accessor(
            d_closeTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLOSE_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_REOPEN_TIMEOUT_MS: {
        return accessor(
            d_reopenTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_REOPEN_RETRY_INTERVAL_MS: {
        return accessor(
            d_reopenRetryIntervalMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_RETRY_INTERVAL_MS]);
    }
    case ATTRIBUTE_ID_REOPEN_MAX_ATTEMPTS: {
        return accessor(
            d_reopenMaxAttempts,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_MAX_ATTEMPTS]);
    }
    case ATTRIBUTE_ID_ASSIGNMENT_TIMEOUT_MS: {
        return accessor(
            d_assignmentTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASSIGNMENT_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_KEEPALIVE_DURATION_MS: {
        return accessor(
            d_keepaliveDurationMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEEPALIVE_DURATION_MS]);
    }
    case ATTRIBUTE_ID_CONSUMPTION_MONITOR_PERIOD_MS: {
        return accessor(d_consumptionMonitorPeriodMs,
                        ATTRIBUTE_INFO_ARRAY
                            [ATTRIBUTE_INDEX_CONSUMPTION_MONITOR_PERIOD_MS]);
    }
    case ATTRIBUTE_ID_STOP_TIMEOUT_MS: {
        return accessor(d_stopTimeoutMs,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STOP_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_SHUTDOWN_TIMEOUT_MS: {
        return accessor(
            d_shutdownTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_ACK_WINDOW_SIZE: {
        return accessor(d_ackWindowSize,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACK_WINDOW_SIZE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int QueueOperationsConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int QueueOperationsConfig::openTimeoutMs() const
{
    return d_openTimeoutMs;
}

inline int QueueOperationsConfig::configureTimeoutMs() const
{
    return d_configureTimeoutMs;
}

inline int QueueOperationsConfig::closeTimeoutMs() const
{
    return d_closeTimeoutMs;
}

inline int QueueOperationsConfig::reopenTimeoutMs() const
{
    return d_reopenTimeoutMs;
}

inline int QueueOperationsConfig::reopenRetryIntervalMs() const
{
    return d_reopenRetryIntervalMs;
}

inline int QueueOperationsConfig::reopenMaxAttempts() const
{
    return d_reopenMaxAttempts;
}

inline int QueueOperationsConfig::assignmentTimeoutMs() const
{
    return d_assignmentTimeoutMs;
}

inline int QueueOperationsConfig::keepaliveDurationMs() const
{
    return d_keepaliveDurationMs;
}

inline int QueueOperationsConfig::consumptionMonitorPeriodMs() const
{
    return d_consumptionMonitorPeriodMs;
}

inline int QueueOperationsConfig::stopTimeoutMs() const
{
    return d_stopTimeoutMs;
}

inline int QueueOperationsConfig::shutdownTimeoutMs() const
{
    return d_shutdownTimeoutMs;
}

inline int QueueOperationsConfig::ackWindowSize() const
{
    return d_ackWindowSize;
}

// --------------------
// class ResolvedDomain
// --------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ResolvedDomain::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_resolvedName,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOLVED_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_clusterName,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ResolvedDomain::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_RESOLVED_NAME: {
        return manipulator(
            &d_resolvedName,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOLVED_NAME]);
    }
    case ATTRIBUTE_ID_CLUSTER_NAME: {
        return manipulator(&d_clusterName,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ResolvedDomain::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& ResolvedDomain::resolvedName()
{
    return d_resolvedName;
}

inline bsl::string& ResolvedDomain::clusterName()
{
    return d_clusterName;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ResolvedDomain::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_resolvedName,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOLVED_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_clusterName,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ResolvedDomain::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_RESOLVED_NAME: {
        return accessor(d_resolvedName,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOLVED_NAME]);
    }
    case ATTRIBUTE_ID_CLUSTER_NAME: {
        return accessor(d_clusterName,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ResolvedDomain::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& ResolvedDomain::resolvedName() const
{
    return d_resolvedName;
}

inline const bsl::string& ResolvedDomain::clusterName() const
{
    return d_clusterName;
}

// ------------------------
// class StatsPrinterConfig
// ------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StatsPrinterConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->printInterval());
    hashAppend(hashAlgorithm, this->file());
    hashAppend(hashAlgorithm, this->maxAgeDays());
    hashAppend(hashAlgorithm, this->rotateBytes());
    hashAppend(hashAlgorithm, this->rotateDays());
}

inline bool StatsPrinterConfig::isEqualTo(const StatsPrinterConfig& rhs) const
{
    return this->printInterval() == rhs.printInterval() &&
           this->file() == rhs.file() &&
           this->maxAgeDays() == rhs.maxAgeDays() &&
           this->rotateBytes() == rhs.rotateBytes() &&
           this->rotateDays() == rhs.rotateDays();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StatsPrinterConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_printInterval,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINT_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_file, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxAgeDays,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_AGE_DAYS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_rotateBytes,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_BYTES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_rotateDays,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_DAYS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StatsPrinterConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_PRINT_INTERVAL: {
        return manipulator(
            &d_printInterval,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINT_INTERVAL]);
    }
    case ATTRIBUTE_ID_FILE: {
        return manipulator(&d_file,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE]);
    }
    case ATTRIBUTE_ID_MAX_AGE_DAYS: {
        return manipulator(&d_maxAgeDays,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_AGE_DAYS]);
    }
    case ATTRIBUTE_ID_ROTATE_BYTES: {
        return manipulator(&d_rotateBytes,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_BYTES]);
    }
    case ATTRIBUTE_ID_ROTATE_DAYS: {
        return manipulator(&d_rotateDays,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_DAYS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StatsPrinterConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& StatsPrinterConfig::printInterval()
{
    return d_printInterval;
}

inline bsl::string& StatsPrinterConfig::file()
{
    return d_file;
}

inline int& StatsPrinterConfig::maxAgeDays()
{
    return d_maxAgeDays;
}

inline int& StatsPrinterConfig::rotateBytes()
{
    return d_rotateBytes;
}

inline int& StatsPrinterConfig::rotateDays()
{
    return d_rotateDays;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StatsPrinterConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_printInterval,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINT_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_file, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxAgeDays,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_AGE_DAYS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_rotateBytes,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_BYTES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_rotateDays,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_DAYS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StatsPrinterConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_PRINT_INTERVAL: {
        return accessor(d_printInterval,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINT_INTERVAL]);
    }
    case ATTRIBUTE_ID_FILE: {
        return accessor(d_file, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE]);
    }
    case ATTRIBUTE_ID_MAX_AGE_DAYS: {
        return accessor(d_maxAgeDays,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_AGE_DAYS]);
    }
    case ATTRIBUTE_ID_ROTATE_BYTES: {
        return accessor(d_rotateBytes,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_BYTES]);
    }
    case ATTRIBUTE_ID_ROTATE_DAYS: {
        return accessor(d_rotateDays,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_DAYS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StatsPrinterConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int StatsPrinterConfig::printInterval() const
{
    return d_printInterval;
}

inline const bsl::string& StatsPrinterConfig::file() const
{
    return d_file;
}

inline int StatsPrinterConfig::maxAgeDays() const
{
    return d_maxAgeDays;
}

inline int StatsPrinterConfig::rotateBytes() const
{
    return d_rotateBytes;
}

inline int StatsPrinterConfig::rotateDays() const
{
    return d_rotateDays;
}

// -----------------------
// class StorageSyncConfig
// -----------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StorageSyncConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->startupRecoveryMaxDurationMs());
    hashAppend(hashAlgorithm, this->maxAttemptsStorageSync());
    hashAppend(hashAlgorithm, this->storageSyncReqTimeoutMs());
    hashAppend(hashAlgorithm, this->masterSyncMaxDurationMs());
    hashAppend(hashAlgorithm, this->partitionSyncStateReqTimeoutMs());
    hashAppend(hashAlgorithm, this->partitionSyncDataReqTimeoutMs());
    hashAppend(hashAlgorithm, this->startupWaitDurationMs());
    hashAppend(hashAlgorithm, this->fileChunkSize());
    hashAppend(hashAlgorithm, this->partitionSyncEventSize());
}

inline bool StorageSyncConfig::isEqualTo(const StorageSyncConfig& rhs) const
{
    return this->startupRecoveryMaxDurationMs() ==
               rhs.startupRecoveryMaxDurationMs() &&
           this->maxAttemptsStorageSync() == rhs.maxAttemptsStorageSync() &&
           this->storageSyncReqTimeoutMs() == rhs.storageSyncReqTimeoutMs() &&
           this->masterSyncMaxDurationMs() == rhs.masterSyncMaxDurationMs() &&
           this->partitionSyncStateReqTimeoutMs() ==
               rhs.partitionSyncStateReqTimeoutMs() &&
           this->partitionSyncDataReqTimeoutMs() ==
               rhs.partitionSyncDataReqTimeoutMs() &&
           this->startupWaitDurationMs() == rhs.startupWaitDurationMs() &&
           this->fileChunkSize() == rhs.fileChunkSize() &&
           this->partitionSyncEventSize() == rhs.partitionSyncEventSize();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StorageSyncConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_startupRecoveryMaxDurationMs,
                      ATTRIBUTE_INFO_ARRAY
                          [ATTRIBUTE_INDEX_STARTUP_RECOVERY_MAX_DURATION_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxAttemptsStorageSync,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_ATTEMPTS_STORAGE_SYNC]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_storageSyncReqTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE_SYNC_REQ_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_masterSyncMaxDurationMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MASTER_SYNC_MAX_DURATION_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_partitionSyncStateReqTimeoutMs,
        ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_partitionSyncDataReqTimeoutMs,
        ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_startupWaitDurationMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STARTUP_WAIT_DURATION_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_fileChunkSize,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_CHUNK_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_partitionSyncEventSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_SYNC_EVENT_SIZE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StorageSyncConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_STARTUP_RECOVERY_MAX_DURATION_MS: {
        return manipulator(
            &d_startupRecoveryMaxDurationMs,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_STARTUP_RECOVERY_MAX_DURATION_MS]);
    }
    case ATTRIBUTE_ID_MAX_ATTEMPTS_STORAGE_SYNC: {
        return manipulator(
            &d_maxAttemptsStorageSync,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_ATTEMPTS_STORAGE_SYNC]);
    }
    case ATTRIBUTE_ID_STORAGE_SYNC_REQ_TIMEOUT_MS: {
        return manipulator(
            &d_storageSyncReqTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE_SYNC_REQ_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_MASTER_SYNC_MAX_DURATION_MS: {
        return manipulator(
            &d_masterSyncMaxDurationMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MASTER_SYNC_MAX_DURATION_MS]);
    }
    case ATTRIBUTE_ID_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS: {
        return manipulator(
            &d_partitionSyncStateReqTimeoutMs,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS: {
        return manipulator(
            &d_partitionSyncDataReqTimeoutMs,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_STARTUP_WAIT_DURATION_MS: {
        return manipulator(
            &d_startupWaitDurationMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STARTUP_WAIT_DURATION_MS]);
    }
    case ATTRIBUTE_ID_FILE_CHUNK_SIZE: {
        return manipulator(
            &d_fileChunkSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_CHUNK_SIZE]);
    }
    case ATTRIBUTE_ID_PARTITION_SYNC_EVENT_SIZE: {
        return manipulator(
            &d_partitionSyncEventSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_SYNC_EVENT_SIZE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StorageSyncConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& StorageSyncConfig::startupRecoveryMaxDurationMs()
{
    return d_startupRecoveryMaxDurationMs;
}

inline int& StorageSyncConfig::maxAttemptsStorageSync()
{
    return d_maxAttemptsStorageSync;
}

inline int& StorageSyncConfig::storageSyncReqTimeoutMs()
{
    return d_storageSyncReqTimeoutMs;
}

inline int& StorageSyncConfig::masterSyncMaxDurationMs()
{
    return d_masterSyncMaxDurationMs;
}

inline int& StorageSyncConfig::partitionSyncStateReqTimeoutMs()
{
    return d_partitionSyncStateReqTimeoutMs;
}

inline int& StorageSyncConfig::partitionSyncDataReqTimeoutMs()
{
    return d_partitionSyncDataReqTimeoutMs;
}

inline int& StorageSyncConfig::startupWaitDurationMs()
{
    return d_startupWaitDurationMs;
}

inline int& StorageSyncConfig::fileChunkSize()
{
    return d_fileChunkSize;
}

inline int& StorageSyncConfig::partitionSyncEventSize()
{
    return d_partitionSyncEventSize;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StorageSyncConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_startupRecoveryMaxDurationMs,
                   ATTRIBUTE_INFO_ARRAY
                       [ATTRIBUTE_INDEX_STARTUP_RECOVERY_MAX_DURATION_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxAttemptsStorageSync,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_ATTEMPTS_STORAGE_SYNC]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_storageSyncReqTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE_SYNC_REQ_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_masterSyncMaxDurationMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MASTER_SYNC_MAX_DURATION_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_partitionSyncStateReqTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY
                       [ATTRIBUTE_INDEX_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_partitionSyncDataReqTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY
                       [ATTRIBUTE_INDEX_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_startupWaitDurationMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STARTUP_WAIT_DURATION_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_fileChunkSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_CHUNK_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_partitionSyncEventSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_SYNC_EVENT_SIZE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StorageSyncConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_STARTUP_RECOVERY_MAX_DURATION_MS: {
        return accessor(
            d_startupRecoveryMaxDurationMs,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_STARTUP_RECOVERY_MAX_DURATION_MS]);
    }
    case ATTRIBUTE_ID_MAX_ATTEMPTS_STORAGE_SYNC: {
        return accessor(
            d_maxAttemptsStorageSync,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_ATTEMPTS_STORAGE_SYNC]);
    }
    case ATTRIBUTE_ID_STORAGE_SYNC_REQ_TIMEOUT_MS: {
        return accessor(
            d_storageSyncReqTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE_SYNC_REQ_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_MASTER_SYNC_MAX_DURATION_MS: {
        return accessor(
            d_masterSyncMaxDurationMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MASTER_SYNC_MAX_DURATION_MS]);
    }
    case ATTRIBUTE_ID_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS: {
        return accessor(
            d_partitionSyncStateReqTimeoutMs,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS: {
        return accessor(
            d_partitionSyncDataReqTimeoutMs,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_STARTUP_WAIT_DURATION_MS: {
        return accessor(
            d_startupWaitDurationMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STARTUP_WAIT_DURATION_MS]);
    }
    case ATTRIBUTE_ID_FILE_CHUNK_SIZE: {
        return accessor(d_fileChunkSize,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_CHUNK_SIZE]);
    }
    case ATTRIBUTE_ID_PARTITION_SYNC_EVENT_SIZE: {
        return accessor(
            d_partitionSyncEventSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_SYNC_EVENT_SIZE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StorageSyncConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int StorageSyncConfig::startupRecoveryMaxDurationMs() const
{
    return d_startupRecoveryMaxDurationMs;
}

inline int StorageSyncConfig::maxAttemptsStorageSync() const
{
    return d_maxAttemptsStorageSync;
}

inline int StorageSyncConfig::storageSyncReqTimeoutMs() const
{
    return d_storageSyncReqTimeoutMs;
}

inline int StorageSyncConfig::masterSyncMaxDurationMs() const
{
    return d_masterSyncMaxDurationMs;
}

inline int StorageSyncConfig::partitionSyncStateReqTimeoutMs() const
{
    return d_partitionSyncStateReqTimeoutMs;
}

inline int StorageSyncConfig::partitionSyncDataReqTimeoutMs() const
{
    return d_partitionSyncDataReqTimeoutMs;
}

inline int StorageSyncConfig::startupWaitDurationMs() const
{
    return d_startupWaitDurationMs;
}

inline int StorageSyncConfig::fileChunkSize() const
{
    return d_fileChunkSize;
}

inline int StorageSyncConfig::partitionSyncEventSize() const
{
    return d_partitionSyncEventSize;
}

// ------------------
// class SyslogConfig
// ------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void SyslogConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->enabled());
    hashAppend(hashAlgorithm, this->appName());
    hashAppend(hashAlgorithm, this->logFormat());
    hashAppend(hashAlgorithm, this->verbosity());
}

inline bool SyslogConfig::isEqualTo(const SyslogConfig& rhs) const
{
    return this->enabled() == rhs.enabled() &&
           this->appName() == rhs.appName() &&
           this->logFormat() == rhs.logFormat() &&
           this->verbosity() == rhs.verbosity();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int SyslogConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_enabled,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_appName,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_logFormat,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_verbosity,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int SyslogConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ENABLED: {
        return manipulator(&d_enabled,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED]);
    }
    case ATTRIBUTE_ID_APP_NAME: {
        return manipulator(&d_appName,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_NAME]);
    }
    case ATTRIBUTE_ID_LOG_FORMAT: {
        return manipulator(&d_logFormat,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT]);
    }
    case ATTRIBUTE_ID_VERBOSITY: {
        return manipulator(&d_verbosity,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int SyslogConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bool& SyslogConfig::enabled()
{
    return d_enabled;
}

inline bsl::string& SyslogConfig::appName()
{
    return d_appName;
}

inline bsl::string& SyslogConfig::logFormat()
{
    return d_logFormat;
}

inline bsl::string& SyslogConfig::verbosity()
{
    return d_verbosity;
}

// ACCESSORS
template <typename t_ACCESSOR>
int SyslogConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_enabled, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_appName, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_logFormat,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_verbosity,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int SyslogConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ENABLED: {
        return accessor(d_enabled,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED]);
    }
    case ATTRIBUTE_ID_APP_NAME: {
        return accessor(d_appName,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_NAME]);
    }
    case ATTRIBUTE_ID_LOG_FORMAT: {
        return accessor(d_logFormat,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT]);
    }
    case ATTRIBUTE_ID_VERBOSITY: {
        return accessor(d_verbosity,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int SyslogConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline bool SyslogConfig::enabled() const
{
    return d_enabled;
}

inline const bsl::string& SyslogConfig::appName() const
{
    return d_appName;
}

inline const bsl::string& SyslogConfig::logFormat() const
{
    return d_logFormat;
}

inline const bsl::string& SyslogConfig::verbosity() const
{
    return d_verbosity;
}

// ------------------------------
// class TcpClusterNodeConnection
// ------------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int TcpClusterNodeConnection::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_endpoint,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENDPOINT]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int TcpClusterNodeConnection::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                  int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ENDPOINT: {
        return manipulator(&d_endpoint,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENDPOINT]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int TcpClusterNodeConnection::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& TcpClusterNodeConnection::endpoint()
{
    return d_endpoint;
}

// ACCESSORS
template <typename t_ACCESSOR>
int TcpClusterNodeConnection::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_endpoint, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENDPOINT]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int TcpClusterNodeConnection::accessAttribute(t_ACCESSOR& accessor,
                                              int         id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ENDPOINT: {
        return accessor(d_endpoint,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENDPOINT]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int TcpClusterNodeConnection::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& TcpClusterNodeConnection::endpoint() const
{
    return d_endpoint;
}

// ------------------------
// class TcpInterfaceConfig
// ------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void TcpInterfaceConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->name());
    hashAppend(hashAlgorithm, this->port());
    hashAppend(hashAlgorithm, this->ioThreads());
    hashAppend(hashAlgorithm, this->maxConnections());
    hashAppend(hashAlgorithm, this->lowWatermark());
    hashAppend(hashAlgorithm, this->highWatermark());
    hashAppend(hashAlgorithm, this->nodeLowWatermark());
    hashAppend(hashAlgorithm, this->nodeHighWatermark());
    hashAppend(hashAlgorithm, this->heartbeatIntervalMs());
}

inline bool TcpInterfaceConfig::isEqualTo(const TcpInterfaceConfig& rhs) const
{
    return this->name() == rhs.name() && this->port() == rhs.port() &&
           this->ioThreads() == rhs.ioThreads() &&
           this->maxConnections() == rhs.maxConnections() &&
           this->lowWatermark() == rhs.lowWatermark() &&
           this->highWatermark() == rhs.highWatermark() &&
           this->nodeLowWatermark() == rhs.nodeLowWatermark() &&
           this->nodeHighWatermark() == rhs.nodeHighWatermark() &&
           this->heartbeatIntervalMs() == rhs.heartbeatIntervalMs();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int TcpInterfaceConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_port, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_ioThreads,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IO_THREADS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxConnections,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONNECTIONS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_lowWatermark,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_highWatermark,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_nodeLowWatermark,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_LOW_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_nodeHighWatermark,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_HIGH_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_heartbeatIntervalMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_INTERVAL_MS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int TcpInterfaceConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_PORT: {
        return manipulator(&d_port,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    }
    case ATTRIBUTE_ID_IO_THREADS: {
        return manipulator(&d_ioThreads,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IO_THREADS]);
    }
    case ATTRIBUTE_ID_MAX_CONNECTIONS: {
        return manipulator(
            &d_maxConnections,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONNECTIONS]);
    }
    case ATTRIBUTE_ID_LOW_WATERMARK: {
        return manipulator(
            &d_lowWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_WATERMARK]);
    }
    case ATTRIBUTE_ID_HIGH_WATERMARK: {
        return manipulator(
            &d_highWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_WATERMARK]);
    }
    case ATTRIBUTE_ID_NODE_LOW_WATERMARK: {
        return manipulator(
            &d_nodeLowWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_LOW_WATERMARK]);
    }
    case ATTRIBUTE_ID_NODE_HIGH_WATERMARK: {
        return manipulator(
            &d_nodeHighWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_HIGH_WATERMARK]);
    }
    case ATTRIBUTE_ID_HEARTBEAT_INTERVAL_MS: {
        return manipulator(
            &d_heartbeatIntervalMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_INTERVAL_MS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int TcpInterfaceConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& TcpInterfaceConfig::name()
{
    return d_name;
}

inline int& TcpInterfaceConfig::port()
{
    return d_port;
}

inline int& TcpInterfaceConfig::ioThreads()
{
    return d_ioThreads;
}

inline int& TcpInterfaceConfig::maxConnections()
{
    return d_maxConnections;
}

inline bsls::Types::Int64& TcpInterfaceConfig::lowWatermark()
{
    return d_lowWatermark;
}

inline bsls::Types::Int64& TcpInterfaceConfig::highWatermark()
{
    return d_highWatermark;
}

inline bsls::Types::Int64& TcpInterfaceConfig::nodeLowWatermark()
{
    return d_nodeLowWatermark;
}

inline bsls::Types::Int64& TcpInterfaceConfig::nodeHighWatermark()
{
    return d_nodeHighWatermark;
}

inline int& TcpInterfaceConfig::heartbeatIntervalMs()
{
    return d_heartbeatIntervalMs;
}

// ACCESSORS
template <typename t_ACCESSOR>
int TcpInterfaceConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_port, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_ioThreads,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IO_THREADS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxConnections,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONNECTIONS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_lowWatermark,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_highWatermark,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_nodeLowWatermark,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_LOW_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_nodeHighWatermark,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_HIGH_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_heartbeatIntervalMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_INTERVAL_MS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int TcpInterfaceConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_PORT: {
        return accessor(d_port, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    }
    case ATTRIBUTE_ID_IO_THREADS: {
        return accessor(d_ioThreads,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IO_THREADS]);
    }
    case ATTRIBUTE_ID_MAX_CONNECTIONS: {
        return accessor(d_maxConnections,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONNECTIONS]);
    }
    case ATTRIBUTE_ID_LOW_WATERMARK: {
        return accessor(d_lowWatermark,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_WATERMARK]);
    }
    case ATTRIBUTE_ID_HIGH_WATERMARK: {
        return accessor(d_highWatermark,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_WATERMARK]);
    }
    case ATTRIBUTE_ID_NODE_LOW_WATERMARK: {
        return accessor(
            d_nodeLowWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_LOW_WATERMARK]);
    }
    case ATTRIBUTE_ID_NODE_HIGH_WATERMARK: {
        return accessor(
            d_nodeHighWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_HIGH_WATERMARK]);
    }
    case ATTRIBUTE_ID_HEARTBEAT_INTERVAL_MS: {
        return accessor(
            d_heartbeatIntervalMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_INTERVAL_MS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int TcpInterfaceConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& TcpInterfaceConfig::name() const
{
    return d_name;
}

inline int TcpInterfaceConfig::port() const
{
    return d_port;
}

inline int TcpInterfaceConfig::ioThreads() const
{
    return d_ioThreads;
}

inline int TcpInterfaceConfig::maxConnections() const
{
    return d_maxConnections;
}

inline bsls::Types::Int64 TcpInterfaceConfig::lowWatermark() const
{
    return d_lowWatermark;
}

inline bsls::Types::Int64 TcpInterfaceConfig::highWatermark() const
{
    return d_highWatermark;
}

inline bsls::Types::Int64 TcpInterfaceConfig::nodeLowWatermark() const
{
    return d_nodeLowWatermark;
}

inline bsls::Types::Int64 TcpInterfaceConfig::nodeHighWatermark() const
{
    return d_nodeHighWatermark;
}

inline int TcpInterfaceConfig::heartbeatIntervalMs() const
{
    return d_heartbeatIntervalMs;
}

// -------------------------------
// class VirtualClusterInformation
// -------------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int VirtualClusterInformation::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_selfNodeId,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SELF_NODE_ID]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int VirtualClusterInformation::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                   int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_SELF_NODE_ID: {
        return manipulator(&d_selfNodeId,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SELF_NODE_ID]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int VirtualClusterInformation::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& VirtualClusterInformation::name()
{
    return d_name;
}

inline int& VirtualClusterInformation::selfNodeId()
{
    return d_selfNodeId;
}

// ACCESSORS
template <typename t_ACCESSOR>
int VirtualClusterInformation::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_selfNodeId,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SELF_NODE_ID]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int VirtualClusterInformation::accessAttribute(t_ACCESSOR& accessor,
                                               int         id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_SELF_NODE_ID: {
        return accessor(d_selfNodeId,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SELF_NODE_ID]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int VirtualClusterInformation::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& VirtualClusterInformation::name() const
{
    return d_name;
}

inline int VirtualClusterInformation::selfNodeId() const
{
    return d_selfNodeId;
}

// ---------------------------
// class ClusterNodeConnection
// ---------------------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void ClusterNodeConnection::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef ClusterNodeConnection Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_TCP:
        hashAppend(hashAlgorithm, this->tcp());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool
ClusterNodeConnection::isEqualTo(const ClusterNodeConnection& rhs) const
{
    typedef ClusterNodeConnection Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_TCP: return this->tcp() == rhs.tcp();
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
inline ClusterNodeConnection::ClusterNodeConnection(
    bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline ClusterNodeConnection::~ClusterNodeConnection()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int ClusterNodeConnection::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case ClusterNodeConnection::SELECTION_ID_TCP:
        return manipulator(&d_tcp.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_TCP]);
    default:
        BSLS_ASSERT(ClusterNodeConnection::SELECTION_ID_UNDEFINED ==
                    d_selectionId);
        return -1;
    }
}

inline TcpClusterNodeConnection& ClusterNodeConnection::tcp()
{
    BSLS_ASSERT(SELECTION_ID_TCP == d_selectionId);
    return d_tcp.object();
}

// ACCESSORS
inline int ClusterNodeConnection::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int ClusterNodeConnection::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_TCP:
        return accessor(d_tcp.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_TCP]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const TcpClusterNodeConnection& ClusterNodeConnection::tcp() const
{
    BSLS_ASSERT(SELECTION_ID_TCP == d_selectionId);
    return d_tcp.object();
}

inline bool ClusterNodeConnection::isTcpValue() const
{
    return SELECTION_ID_TCP == d_selectionId;
}

inline bool ClusterNodeConnection::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// -------------------------------
// class DispatcherProcessorConfig
// -------------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int DispatcherProcessorConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_numProcessors,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PROCESSORS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_processorConfig,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROCESSOR_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int DispatcherProcessorConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                   int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NUM_PROCESSORS: {
        return manipulator(
            &d_numProcessors,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PROCESSORS]);
    }
    case ATTRIBUTE_ID_PROCESSOR_CONFIG: {
        return manipulator(
            &d_processorConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROCESSOR_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int DispatcherProcessorConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& DispatcherProcessorConfig::numProcessors()
{
    return d_numProcessors;
}

inline DispatcherProcessorParameters&
DispatcherProcessorConfig::processorConfig()
{
    return d_processorConfig;
}

// ACCESSORS
template <typename t_ACCESSOR>
int DispatcherProcessorConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_numProcessors,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PROCESSORS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_processorConfig,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROCESSOR_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int DispatcherProcessorConfig::accessAttribute(t_ACCESSOR& accessor,
                                               int         id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NUM_PROCESSORS: {
        return accessor(d_numProcessors,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PROCESSORS]);
    }
    case ATTRIBUTE_ID_PROCESSOR_CONFIG: {
        return accessor(
            d_processorConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROCESSOR_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int DispatcherProcessorConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int DispatcherProcessorConfig::numProcessors() const
{
    return d_numProcessors;
}

inline const DispatcherProcessorParameters&
DispatcherProcessorConfig::processorConfig() const
{
    return d_processorConfig;
}

// -------------------
// class LogController
// -------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void LogController::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->fileName());
    hashAppend(hashAlgorithm, this->fileMaxAgeDays());
    hashAppend(hashAlgorithm, this->rotationBytes());
    hashAppend(hashAlgorithm, this->logfileFormat());
    hashAppend(hashAlgorithm, this->consoleFormat());
    hashAppend(hashAlgorithm, this->loggingVerbosity());
    hashAppend(hashAlgorithm, this->bslsLogSeverityThreshold());
    hashAppend(hashAlgorithm, this->consoleSeverityThreshold());
    hashAppend(hashAlgorithm, this->categories());
    hashAppend(hashAlgorithm, this->syslog());
}

inline bool LogController::isEqualTo(const LogController& rhs) const
{
    return this->fileName() == rhs.fileName() &&
           this->fileMaxAgeDays() == rhs.fileMaxAgeDays() &&
           this->rotationBytes() == rhs.rotationBytes() &&
           this->logfileFormat() == rhs.logfileFormat() &&
           this->consoleFormat() == rhs.consoleFormat() &&
           this->loggingVerbosity() == rhs.loggingVerbosity() &&
           this->bslsLogSeverityThreshold() ==
               rhs.bslsLogSeverityThreshold() &&
           this->consoleSeverityThreshold() ==
               rhs.consoleSeverityThreshold() &&
           this->categories() == rhs.categories() &&
           this->syslog() == rhs.syslog();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int LogController::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_fileName,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_fileMaxAgeDays,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_MAX_AGE_DAYS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_rotationBytes,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATION_BYTES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_logfileFormat,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGFILE_FORMAT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_consoleFormat,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSOLE_FORMAT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_loggingVerbosity,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGGING_VERBOSITY]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_bslsLogSeverityThreshold,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BSLS_LOG_SEVERITY_THRESHOLD]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_consoleSeverityThreshold,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSOLE_SEVERITY_THRESHOLD]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_categories,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CATEGORIES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_syslog, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYSLOG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int LogController::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_FILE_NAME: {
        return manipulator(&d_fileName,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_NAME]);
    }
    case ATTRIBUTE_ID_FILE_MAX_AGE_DAYS: {
        return manipulator(
            &d_fileMaxAgeDays,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_MAX_AGE_DAYS]);
    }
    case ATTRIBUTE_ID_ROTATION_BYTES: {
        return manipulator(
            &d_rotationBytes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATION_BYTES]);
    }
    case ATTRIBUTE_ID_LOGFILE_FORMAT: {
        return manipulator(
            &d_logfileFormat,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGFILE_FORMAT]);
    }
    case ATTRIBUTE_ID_CONSOLE_FORMAT: {
        return manipulator(
            &d_consoleFormat,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSOLE_FORMAT]);
    }
    case ATTRIBUTE_ID_LOGGING_VERBOSITY: {
        return manipulator(
            &d_loggingVerbosity,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGGING_VERBOSITY]);
    }
    case ATTRIBUTE_ID_BSLS_LOG_SEVERITY_THRESHOLD: {
        return manipulator(
            &d_bslsLogSeverityThreshold,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BSLS_LOG_SEVERITY_THRESHOLD]);
    }
    case ATTRIBUTE_ID_CONSOLE_SEVERITY_THRESHOLD: {
        return manipulator(
            &d_consoleSeverityThreshold,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSOLE_SEVERITY_THRESHOLD]);
    }
    case ATTRIBUTE_ID_CATEGORIES: {
        return manipulator(&d_categories,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CATEGORIES]);
    }
    case ATTRIBUTE_ID_SYSLOG: {
        return manipulator(&d_syslog,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYSLOG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int LogController::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& LogController::fileName()
{
    return d_fileName;
}

inline int& LogController::fileMaxAgeDays()
{
    return d_fileMaxAgeDays;
}

inline int& LogController::rotationBytes()
{
    return d_rotationBytes;
}

inline bsl::string& LogController::logfileFormat()
{
    return d_logfileFormat;
}

inline bsl::string& LogController::consoleFormat()
{
    return d_consoleFormat;
}

inline bsl::string& LogController::loggingVerbosity()
{
    return d_loggingVerbosity;
}

inline bsl::string& LogController::bslsLogSeverityThreshold()
{
    return d_bslsLogSeverityThreshold;
}

inline bsl::string& LogController::consoleSeverityThreshold()
{
    return d_consoleSeverityThreshold;
}

inline bsl::vector<bsl::string>& LogController::categories()
{
    return d_categories;
}

inline SyslogConfig& LogController::syslog()
{
    return d_syslog;
}

// ACCESSORS
template <typename t_ACCESSOR>
int LogController::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_fileName,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_fileMaxAgeDays,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_MAX_AGE_DAYS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_rotationBytes,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATION_BYTES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_logfileFormat,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGFILE_FORMAT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_consoleFormat,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSOLE_FORMAT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_loggingVerbosity,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGGING_VERBOSITY]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_bslsLogSeverityThreshold,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BSLS_LOG_SEVERITY_THRESHOLD]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_consoleSeverityThreshold,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSOLE_SEVERITY_THRESHOLD]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_categories,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CATEGORIES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_syslog, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYSLOG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int LogController::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_FILE_NAME: {
        return accessor(d_fileName,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_NAME]);
    }
    case ATTRIBUTE_ID_FILE_MAX_AGE_DAYS: {
        return accessor(
            d_fileMaxAgeDays,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_MAX_AGE_DAYS]);
    }
    case ATTRIBUTE_ID_ROTATION_BYTES: {
        return accessor(d_rotationBytes,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATION_BYTES]);
    }
    case ATTRIBUTE_ID_LOGFILE_FORMAT: {
        return accessor(d_logfileFormat,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGFILE_FORMAT]);
    }
    case ATTRIBUTE_ID_CONSOLE_FORMAT: {
        return accessor(d_consoleFormat,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSOLE_FORMAT]);
    }
    case ATTRIBUTE_ID_LOGGING_VERBOSITY: {
        return accessor(
            d_loggingVerbosity,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGGING_VERBOSITY]);
    }
    case ATTRIBUTE_ID_BSLS_LOG_SEVERITY_THRESHOLD: {
        return accessor(
            d_bslsLogSeverityThreshold,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BSLS_LOG_SEVERITY_THRESHOLD]);
    }
    case ATTRIBUTE_ID_CONSOLE_SEVERITY_THRESHOLD: {
        return accessor(
            d_consoleSeverityThreshold,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSOLE_SEVERITY_THRESHOLD]);
    }
    case ATTRIBUTE_ID_CATEGORIES: {
        return accessor(d_categories,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CATEGORIES]);
    }
    case ATTRIBUTE_ID_SYSLOG: {
        return accessor(d_syslog,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYSLOG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int LogController::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& LogController::fileName() const
{
    return d_fileName;
}

inline int LogController::fileMaxAgeDays() const
{
    return d_fileMaxAgeDays;
}

inline int LogController::rotationBytes() const
{
    return d_rotationBytes;
}

inline const bsl::string& LogController::logfileFormat() const
{
    return d_logfileFormat;
}

inline const bsl::string& LogController::consoleFormat() const
{
    return d_consoleFormat;
}

inline const bsl::string& LogController::loggingVerbosity() const
{
    return d_loggingVerbosity;
}

inline const bsl::string& LogController::bslsLogSeverityThreshold() const
{
    return d_bslsLogSeverityThreshold;
}

inline const bsl::string& LogController::consoleSeverityThreshold() const
{
    return d_consoleSeverityThreshold;
}

inline const bsl::vector<bsl::string>& LogController::categories() const
{
    return d_categories;
}

inline const SyslogConfig& LogController::syslog() const
{
    return d_syslog;
}

// -----------------------
// class NetworkInterfaces
// -----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int NetworkInterfaces::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_heartbeats,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEATS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_tcpInterface,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TCP_INTERFACE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int NetworkInterfaces::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_HEARTBEATS: {
        return manipulator(&d_heartbeats,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEATS]);
    }
    case ATTRIBUTE_ID_TCP_INTERFACE: {
        return manipulator(
            &d_tcpInterface,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TCP_INTERFACE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int NetworkInterfaces::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline Heartbeat& NetworkInterfaces::heartbeats()
{
    return d_heartbeats;
}

inline bdlb::NullableValue<TcpInterfaceConfig>&
NetworkInterfaces::tcpInterface()
{
    return d_tcpInterface;
}

// ACCESSORS
template <typename t_ACCESSOR>
int NetworkInterfaces::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_heartbeats,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEATS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_tcpInterface,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TCP_INTERFACE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int NetworkInterfaces::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_HEARTBEATS: {
        return accessor(d_heartbeats,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEATS]);
    }
    case ATTRIBUTE_ID_TCP_INTERFACE: {
        return accessor(d_tcpInterface,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TCP_INTERFACE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int NetworkInterfaces::accessAttribute(t_ACCESSOR& accessor,
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

inline const Heartbeat& NetworkInterfaces::heartbeats() const
{
    return d_heartbeats;
}

inline const bdlb::NullableValue<TcpInterfaceConfig>&
NetworkInterfaces::tcpInterface() const
{
    return d_tcpInterface;
}

// ---------------------
// class PartitionConfig
// ---------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void PartitionConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->numPartitions());
    hashAppend(hashAlgorithm, this->location());
    hashAppend(hashAlgorithm, this->archiveLocation());
    hashAppend(hashAlgorithm, this->maxDataFileSize());
    hashAppend(hashAlgorithm, this->maxJournalFileSize());
    hashAppend(hashAlgorithm, this->maxQlistFileSize());
    hashAppend(hashAlgorithm, this->preallocate());
    hashAppend(hashAlgorithm, this->maxArchivedFileSets());
    hashAppend(hashAlgorithm, this->prefaultPages());
    hashAppend(hashAlgorithm, this->flushAtShutdown());
    hashAppend(hashAlgorithm, this->syncConfig());
}

inline bool PartitionConfig::isEqualTo(const PartitionConfig& rhs) const
{
    return this->numPartitions() == rhs.numPartitions() &&
           this->location() == rhs.location() &&
           this->archiveLocation() == rhs.archiveLocation() &&
           this->maxDataFileSize() == rhs.maxDataFileSize() &&
           this->maxJournalFileSize() == rhs.maxJournalFileSize() &&
           this->maxQlistFileSize() == rhs.maxQlistFileSize() &&
           this->preallocate() == rhs.preallocate() &&
           this->maxArchivedFileSets() == rhs.maxArchivedFileSets() &&
           this->prefaultPages() == rhs.prefaultPages() &&
           this->flushAtShutdown() == rhs.flushAtShutdown() &&
           this->syncConfig() == rhs.syncConfig();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int PartitionConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_numPartitions,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PARTITIONS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_location,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_archiveLocation,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ARCHIVE_LOCATION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxDataFileSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DATA_FILE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxJournalFileSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_JOURNAL_FILE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxQlistFileSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QLIST_FILE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_preallocate,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREALLOCATE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxArchivedFileSets,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_ARCHIVED_FILE_SETS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_prefaultPages,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREFAULT_PAGES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_flushAtShutdown,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLUSH_AT_SHUTDOWN]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_syncConfig,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYNC_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int PartitionConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NUM_PARTITIONS: {
        return manipulator(
            &d_numPartitions,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PARTITIONS]);
    }
    case ATTRIBUTE_ID_LOCATION: {
        return manipulator(&d_location,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION]);
    }
    case ATTRIBUTE_ID_ARCHIVE_LOCATION: {
        return manipulator(
            &d_archiveLocation,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ARCHIVE_LOCATION]);
    }
    case ATTRIBUTE_ID_MAX_DATA_FILE_SIZE: {
        return manipulator(
            &d_maxDataFileSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DATA_FILE_SIZE]);
    }
    case ATTRIBUTE_ID_MAX_JOURNAL_FILE_SIZE: {
        return manipulator(
            &d_maxJournalFileSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_JOURNAL_FILE_SIZE]);
    }
    case ATTRIBUTE_ID_MAX_QLIST_FILE_SIZE: {
        return manipulator(
            &d_maxQlistFileSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QLIST_FILE_SIZE]);
    }
    case ATTRIBUTE_ID_PREALLOCATE: {
        return manipulator(&d_preallocate,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREALLOCATE]);
    }
    case ATTRIBUTE_ID_MAX_ARCHIVED_FILE_SETS: {
        return manipulator(
            &d_maxArchivedFileSets,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_ARCHIVED_FILE_SETS]);
    }
    case ATTRIBUTE_ID_PREFAULT_PAGES: {
        return manipulator(
            &d_prefaultPages,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREFAULT_PAGES]);
    }
    case ATTRIBUTE_ID_FLUSH_AT_SHUTDOWN: {
        return manipulator(
            &d_flushAtShutdown,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLUSH_AT_SHUTDOWN]);
    }
    case ATTRIBUTE_ID_SYNC_CONFIG: {
        return manipulator(&d_syncConfig,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYNC_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int PartitionConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& PartitionConfig::numPartitions()
{
    return d_numPartitions;
}

inline bsl::string& PartitionConfig::location()
{
    return d_location;
}

inline bsl::string& PartitionConfig::archiveLocation()
{
    return d_archiveLocation;
}

inline bsls::Types::Uint64& PartitionConfig::maxDataFileSize()
{
    return d_maxDataFileSize;
}

inline bsls::Types::Uint64& PartitionConfig::maxJournalFileSize()
{
    return d_maxJournalFileSize;
}

inline bsls::Types::Uint64& PartitionConfig::maxQlistFileSize()
{
    return d_maxQlistFileSize;
}

inline bool& PartitionConfig::preallocate()
{
    return d_preallocate;
}

inline int& PartitionConfig::maxArchivedFileSets()
{
    return d_maxArchivedFileSets;
}

inline bool& PartitionConfig::prefaultPages()
{
    return d_prefaultPages;
}

inline bool& PartitionConfig::flushAtShutdown()
{
    return d_flushAtShutdown;
}

inline StorageSyncConfig& PartitionConfig::syncConfig()
{
    return d_syncConfig;
}

// ACCESSORS
template <typename t_ACCESSOR>
int PartitionConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_numPartitions,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PARTITIONS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_location, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_archiveLocation,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ARCHIVE_LOCATION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxDataFileSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DATA_FILE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxJournalFileSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_JOURNAL_FILE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxQlistFileSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QLIST_FILE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_preallocate,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREALLOCATE]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxArchivedFileSets,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_ARCHIVED_FILE_SETS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_prefaultPages,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREFAULT_PAGES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_flushAtShutdown,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLUSH_AT_SHUTDOWN]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_syncConfig,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYNC_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int PartitionConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NUM_PARTITIONS: {
        return accessor(d_numPartitions,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PARTITIONS]);
    }
    case ATTRIBUTE_ID_LOCATION: {
        return accessor(d_location,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION]);
    }
    case ATTRIBUTE_ID_ARCHIVE_LOCATION: {
        return accessor(
            d_archiveLocation,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ARCHIVE_LOCATION]);
    }
    case ATTRIBUTE_ID_MAX_DATA_FILE_SIZE: {
        return accessor(
            d_maxDataFileSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DATA_FILE_SIZE]);
    }
    case ATTRIBUTE_ID_MAX_JOURNAL_FILE_SIZE: {
        return accessor(
            d_maxJournalFileSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_JOURNAL_FILE_SIZE]);
    }
    case ATTRIBUTE_ID_MAX_QLIST_FILE_SIZE: {
        return accessor(
            d_maxQlistFileSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QLIST_FILE_SIZE]);
    }
    case ATTRIBUTE_ID_PREALLOCATE: {
        return accessor(d_preallocate,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREALLOCATE]);
    }
    case ATTRIBUTE_ID_MAX_ARCHIVED_FILE_SETS: {
        return accessor(
            d_maxArchivedFileSets,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_ARCHIVED_FILE_SETS]);
    }
    case ATTRIBUTE_ID_PREFAULT_PAGES: {
        return accessor(d_prefaultPages,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREFAULT_PAGES]);
    }
    case ATTRIBUTE_ID_FLUSH_AT_SHUTDOWN: {
        return accessor(
            d_flushAtShutdown,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLUSH_AT_SHUTDOWN]);
    }
    case ATTRIBUTE_ID_SYNC_CONFIG: {
        return accessor(d_syncConfig,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYNC_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int PartitionConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int PartitionConfig::numPartitions() const
{
    return d_numPartitions;
}

inline const bsl::string& PartitionConfig::location() const
{
    return d_location;
}

inline const bsl::string& PartitionConfig::archiveLocation() const
{
    return d_archiveLocation;
}

inline bsls::Types::Uint64 PartitionConfig::maxDataFileSize() const
{
    return d_maxDataFileSize;
}

inline bsls::Types::Uint64 PartitionConfig::maxJournalFileSize() const
{
    return d_maxJournalFileSize;
}

inline bsls::Types::Uint64 PartitionConfig::maxQlistFileSize() const
{
    return d_maxQlistFileSize;
}

inline bool PartitionConfig::preallocate() const
{
    return d_preallocate;
}

inline int PartitionConfig::maxArchivedFileSets() const
{
    return d_maxArchivedFileSets;
}

inline bool PartitionConfig::prefaultPages() const
{
    return d_prefaultPages;
}

inline bool PartitionConfig::flushAtShutdown() const
{
    return d_flushAtShutdown;
}

inline const StorageSyncConfig& PartitionConfig::syncConfig() const
{
    return d_syncConfig;
}

// --------------------------------
// class StatPluginConfigPrometheus
// --------------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StatPluginConfigPrometheus::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->mode());
    hashAppend(hashAlgorithm, this->host());
    hashAppend(hashAlgorithm, this->port());
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StatPluginConfigPrometheus::manipulateAttributes(
    t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_mode, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_host, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_port, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StatPluginConfigPrometheus::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                    int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MODE: {
        return manipulator(&d_mode,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    }
    case ATTRIBUTE_ID_HOST: {
        return manipulator(&d_host,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST]);
    }
    case ATTRIBUTE_ID_PORT: {
        return manipulator(&d_port,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StatPluginConfigPrometheus::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline ExportMode::Value& StatPluginConfigPrometheus::mode()
{
    return d_mode;
}

inline bsl::string& StatPluginConfigPrometheus::host()
{
    return d_host;
}

inline int& StatPluginConfigPrometheus::port()
{
    return d_port;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StatPluginConfigPrometheus::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_mode, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_host, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_port, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StatPluginConfigPrometheus::accessAttribute(t_ACCESSOR& accessor,
                                                int         id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MODE: {
        return accessor(d_mode, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    }
    case ATTRIBUTE_ID_HOST: {
        return accessor(d_host, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST]);
    }
    case ATTRIBUTE_ID_PORT: {
        return accessor(d_port, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StatPluginConfigPrometheus::accessAttribute(t_ACCESSOR& accessor,
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

inline ExportMode::Value StatPluginConfigPrometheus::mode() const
{
    return d_mode;
}

inline const bsl::string& StatPluginConfigPrometheus::host() const
{
    return d_host;
}

inline int StatPluginConfigPrometheus::port() const
{
    return d_port;
}

// -----------------
// class ClusterNode
// -----------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void ClusterNode::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->id());
    hashAppend(hashAlgorithm, this->name());
    hashAppend(hashAlgorithm, this->dataCenter());
    hashAppend(hashAlgorithm, this->transport());
}

inline bool ClusterNode::isEqualTo(const ClusterNode& rhs) const
{
    return this->id() == rhs.id() && this->name() == rhs.name() &&
           this->dataCenter() == rhs.dataCenter() &&
           this->transport() == rhs.transport();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ClusterNode::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_dataCenter,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_CENTER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_transport,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TRANSPORT]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ClusterNode::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ID: {
        return manipulator(&d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    }
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_DATA_CENTER: {
        return manipulator(&d_dataCenter,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_CENTER]);
    }
    case ATTRIBUTE_ID_TRANSPORT: {
        return manipulator(&d_transport,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TRANSPORT]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ClusterNode::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& ClusterNode::id()
{
    return d_id;
}

inline bsl::string& ClusterNode::name()
{
    return d_name;
}

inline bsl::string& ClusterNode::dataCenter()
{
    return d_dataCenter;
}

inline ClusterNodeConnection& ClusterNode::transport()
{
    return d_transport;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ClusterNode::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_dataCenter,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_CENTER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_transport,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TRANSPORT]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ClusterNode::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ID: {
        return accessor(d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    }
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_DATA_CENTER: {
        return accessor(d_dataCenter,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_CENTER]);
    }
    case ATTRIBUTE_ID_TRANSPORT: {
        return accessor(d_transport,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TRANSPORT]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ClusterNode::accessAttribute(t_ACCESSOR& accessor,
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

inline int ClusterNode::id() const
{
    return d_id;
}

inline const bsl::string& ClusterNode::name() const
{
    return d_name;
}

inline const bsl::string& ClusterNode::dataCenter() const
{
    return d_dataCenter;
}

inline const ClusterNodeConnection& ClusterNode::transport() const
{
    return d_transport;
}

// ----------------------
// class DispatcherConfig
// ----------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void DispatcherConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->sessions());
    hashAppend(hashAlgorithm, this->queues());
    hashAppend(hashAlgorithm, this->clusters());
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int DispatcherConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_sessions,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SESSIONS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_queues, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_clusters,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTERS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int DispatcherConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_SESSIONS: {
        return manipulator(&d_sessions,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SESSIONS]);
    }
    case ATTRIBUTE_ID_QUEUES: {
        return manipulator(&d_queues,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES]);
    }
    case ATTRIBUTE_ID_CLUSTERS: {
        return manipulator(&d_clusters,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTERS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int DispatcherConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline DispatcherProcessorConfig& DispatcherConfig::sessions()
{
    return d_sessions;
}

inline DispatcherProcessorConfig& DispatcherConfig::queues()
{
    return d_queues;
}

inline DispatcherProcessorConfig& DispatcherConfig::clusters()
{
    return d_clusters;
}

// ACCESSORS
template <typename t_ACCESSOR>
int DispatcherConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_sessions, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SESSIONS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queues, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_clusters, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTERS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int DispatcherConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_SESSIONS: {
        return accessor(d_sessions,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SESSIONS]);
    }
    case ATTRIBUTE_ID_QUEUES: {
        return accessor(d_queues,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES]);
    }
    case ATTRIBUTE_ID_CLUSTERS: {
        return accessor(d_clusters,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTERS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int DispatcherConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const DispatcherProcessorConfig& DispatcherConfig::sessions() const
{
    return d_sessions;
}

inline const DispatcherProcessorConfig& DispatcherConfig::queues() const
{
    return d_queues;
}

inline const DispatcherProcessorConfig& DispatcherConfig::clusters() const
{
    return d_clusters;
}

// -------------------------------
// class ReversedClusterConnection
// -------------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ReversedClusterConnection::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_connections,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONNECTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ReversedClusterConnection::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                   int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_CONNECTIONS: {
        return manipulator(&d_connections,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONNECTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ReversedClusterConnection::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& ReversedClusterConnection::name()
{
    return d_name;
}

inline bsl::vector<ClusterNodeConnection>&
ReversedClusterConnection::connections()
{
    return d_connections;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ReversedClusterConnection::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_connections,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONNECTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ReversedClusterConnection::accessAttribute(t_ACCESSOR& accessor,
                                               int         id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_CONNECTIONS: {
        return accessor(d_connections,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONNECTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ReversedClusterConnection::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& ReversedClusterConnection::name() const
{
    return d_name;
}

inline const bsl::vector<ClusterNodeConnection>&
ReversedClusterConnection::connections() const
{
    return d_connections;
}

// ----------------------
// class StatPluginConfig
// ----------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StatPluginConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->name());
    hashAppend(hashAlgorithm, this->queueSize());
    hashAppend(hashAlgorithm, this->queueHighWatermark());
    hashAppend(hashAlgorithm, this->queueLowWatermark());
    hashAppend(hashAlgorithm, this->publishInterval());
    hashAppend(hashAlgorithm, this->namespacePrefix());
    hashAppend(hashAlgorithm, this->hosts());
    hashAppend(hashAlgorithm, this->instanceId());
    hashAppend(hashAlgorithm, this->prometheusSpecific());
}

inline bool StatPluginConfig::isEqualTo(const StatPluginConfig& rhs) const
{
    return this->name() == rhs.name() &&
           this->queueSize() == rhs.queueSize() &&
           this->queueHighWatermark() == rhs.queueHighWatermark() &&
           this->queueLowWatermark() == rhs.queueLowWatermark() &&
           this->publishInterval() == rhs.publishInterval() &&
           this->namespacePrefix() == rhs.namespacePrefix() &&
           this->hosts() == rhs.hosts() &&
           this->instanceId() == rhs.instanceId() &&
           this->prometheusSpecific() == rhs.prometheusSpecific();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StatPluginConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_queueSize,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_queueHighWatermark,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_HIGH_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_queueLowWatermark,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LOW_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_publishInterval,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PUBLISH_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_namespacePrefix,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAMESPACE_PREFIX]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_hosts, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOSTS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_instanceId,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INSTANCE_ID]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_prometheusSpecific,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROMETHEUS_SPECIFIC]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StatPluginConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_QUEUE_SIZE: {
        return manipulator(&d_queueSize,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE]);
    }
    case ATTRIBUTE_ID_QUEUE_HIGH_WATERMARK: {
        return manipulator(
            &d_queueHighWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_HIGH_WATERMARK]);
    }
    case ATTRIBUTE_ID_QUEUE_LOW_WATERMARK: {
        return manipulator(
            &d_queueLowWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LOW_WATERMARK]);
    }
    case ATTRIBUTE_ID_PUBLISH_INTERVAL: {
        return manipulator(
            &d_publishInterval,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PUBLISH_INTERVAL]);
    }
    case ATTRIBUTE_ID_NAMESPACE_PREFIX: {
        return manipulator(
            &d_namespacePrefix,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAMESPACE_PREFIX]);
    }
    case ATTRIBUTE_ID_HOSTS: {
        return manipulator(&d_hosts,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOSTS]);
    }
    case ATTRIBUTE_ID_INSTANCE_ID: {
        return manipulator(&d_instanceId,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INSTANCE_ID]);
    }
    case ATTRIBUTE_ID_PROMETHEUS_SPECIFIC: {
        return manipulator(
            &d_prometheusSpecific,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROMETHEUS_SPECIFIC]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StatPluginConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& StatPluginConfig::name()
{
    return d_name;
}

inline int& StatPluginConfig::queueSize()
{
    return d_queueSize;
}

inline int& StatPluginConfig::queueHighWatermark()
{
    return d_queueHighWatermark;
}

inline int& StatPluginConfig::queueLowWatermark()
{
    return d_queueLowWatermark;
}

inline int& StatPluginConfig::publishInterval()
{
    return d_publishInterval;
}

inline bsl::string& StatPluginConfig::namespacePrefix()
{
    return d_namespacePrefix;
}

inline bsl::vector<bsl::string>& StatPluginConfig::hosts()
{
    return d_hosts;
}

inline bsl::string& StatPluginConfig::instanceId()
{
    return d_instanceId;
}

inline bdlb::NullableValue<StatPluginConfigPrometheus>&
StatPluginConfig::prometheusSpecific()
{
    return d_prometheusSpecific;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StatPluginConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queueSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queueHighWatermark,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_HIGH_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queueLowWatermark,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LOW_WATERMARK]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_publishInterval,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PUBLISH_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_namespacePrefix,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAMESPACE_PREFIX]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_hosts, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOSTS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_instanceId,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INSTANCE_ID]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_prometheusSpecific,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROMETHEUS_SPECIFIC]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StatPluginConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_QUEUE_SIZE: {
        return accessor(d_queueSize,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE]);
    }
    case ATTRIBUTE_ID_QUEUE_HIGH_WATERMARK: {
        return accessor(
            d_queueHighWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_HIGH_WATERMARK]);
    }
    case ATTRIBUTE_ID_QUEUE_LOW_WATERMARK: {
        return accessor(
            d_queueLowWatermark,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LOW_WATERMARK]);
    }
    case ATTRIBUTE_ID_PUBLISH_INTERVAL: {
        return accessor(
            d_publishInterval,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PUBLISH_INTERVAL]);
    }
    case ATTRIBUTE_ID_NAMESPACE_PREFIX: {
        return accessor(
            d_namespacePrefix,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAMESPACE_PREFIX]);
    }
    case ATTRIBUTE_ID_HOSTS: {
        return accessor(d_hosts, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOSTS]);
    }
    case ATTRIBUTE_ID_INSTANCE_ID: {
        return accessor(d_instanceId,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INSTANCE_ID]);
    }
    case ATTRIBUTE_ID_PROMETHEUS_SPECIFIC: {
        return accessor(
            d_prometheusSpecific,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROMETHEUS_SPECIFIC]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StatPluginConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& StatPluginConfig::name() const
{
    return d_name;
}

inline int StatPluginConfig::queueSize() const
{
    return d_queueSize;
}

inline int StatPluginConfig::queueHighWatermark() const
{
    return d_queueHighWatermark;
}

inline int StatPluginConfig::queueLowWatermark() const
{
    return d_queueLowWatermark;
}

inline int StatPluginConfig::publishInterval() const
{
    return d_publishInterval;
}

inline const bsl::string& StatPluginConfig::namespacePrefix() const
{
    return d_namespacePrefix;
}

inline const bsl::vector<bsl::string>& StatPluginConfig::hosts() const
{
    return d_hosts;
}

inline const bsl::string& StatPluginConfig::instanceId() const
{
    return d_instanceId;
}

inline const bdlb::NullableValue<StatPluginConfigPrometheus>&
StatPluginConfig::prometheusSpecific() const
{
    return d_prometheusSpecific;
}

// ----------------
// class TaskConfig
// ----------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void TaskConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->allocatorType());
    hashAppend(hashAlgorithm, this->allocationLimit());
    hashAppend(hashAlgorithm, this->logController());
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int TaskConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_allocatorType,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATOR_TYPE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_allocationLimit,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATION_LIMIT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_logController,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_CONTROLLER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int TaskConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ALLOCATOR_TYPE: {
        return manipulator(
            &d_allocatorType,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATOR_TYPE]);
    }
    case ATTRIBUTE_ID_ALLOCATION_LIMIT: {
        return manipulator(
            &d_allocationLimit,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATION_LIMIT]);
    }
    case ATTRIBUTE_ID_LOG_CONTROLLER: {
        return manipulator(
            &d_logController,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_CONTROLLER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int TaskConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline AllocatorType::Value& TaskConfig::allocatorType()
{
    return d_allocatorType;
}

inline bsls::Types::Uint64& TaskConfig::allocationLimit()
{
    return d_allocationLimit;
}

inline LogController& TaskConfig::logController()
{
    return d_logController;
}

// ACCESSORS
template <typename t_ACCESSOR>
int TaskConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_allocatorType,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATOR_TYPE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_allocationLimit,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATION_LIMIT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_logController,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_CONTROLLER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int TaskConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ALLOCATOR_TYPE: {
        return accessor(d_allocatorType,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATOR_TYPE]);
    }
    case ATTRIBUTE_ID_ALLOCATION_LIMIT: {
        return accessor(
            d_allocationLimit,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATION_LIMIT]);
    }
    case ATTRIBUTE_ID_LOG_CONTROLLER: {
        return accessor(d_logController,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_CONTROLLER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int TaskConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline AllocatorType::Value TaskConfig::allocatorType() const
{
    return d_allocatorType;
}

inline bsls::Types::Uint64 TaskConfig::allocationLimit() const
{
    return d_allocationLimit;
}

inline const LogController& TaskConfig::logController() const
{
    return d_logController;
}

// -----------------------
// class ClusterDefinition
// -----------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void ClusterDefinition::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->name());
    hashAppend(hashAlgorithm, this->nodes());
    hashAppend(hashAlgorithm, this->partitionConfig());
    hashAppend(hashAlgorithm, this->masterAssignment());
    hashAppend(hashAlgorithm, this->elector());
    hashAppend(hashAlgorithm, this->queueOperations());
    hashAppend(hashAlgorithm, this->clusterAttributes());
    hashAppend(hashAlgorithm, this->clusterMonitorConfig());
    hashAppend(hashAlgorithm, this->messageThrottleConfig());
}

inline bool ClusterDefinition::isEqualTo(const ClusterDefinition& rhs) const
{
    return this->name() == rhs.name() && this->nodes() == rhs.nodes() &&
           this->partitionConfig() == rhs.partitionConfig() &&
           this->masterAssignment() == rhs.masterAssignment() &&
           this->elector() == rhs.elector() &&
           this->queueOperations() == rhs.queueOperations() &&
           this->clusterAttributes() == rhs.clusterAttributes() &&
           this->clusterMonitorConfig() == rhs.clusterMonitorConfig() &&
           this->messageThrottleConfig() == rhs.messageThrottleConfig();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ClusterDefinition::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_nodes, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_partitionConfig,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_masterAssignment,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MASTER_ASSIGNMENT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_elector,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTOR]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_queueOperations,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_clusterAttributes,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_ATTRIBUTES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_clusterMonitorConfig,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_messageThrottleConfig,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ClusterDefinition::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_NODES: {
        return manipulator(&d_nodes,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES]);
    }
    case ATTRIBUTE_ID_PARTITION_CONFIG: {
        return manipulator(
            &d_partitionConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_CONFIG]);
    }
    case ATTRIBUTE_ID_MASTER_ASSIGNMENT: {
        return manipulator(
            &d_masterAssignment,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MASTER_ASSIGNMENT]);
    }
    case ATTRIBUTE_ID_ELECTOR: {
        return manipulator(&d_elector,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTOR]);
    }
    case ATTRIBUTE_ID_QUEUE_OPERATIONS: {
        return manipulator(
            &d_queueOperations,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS]);
    }
    case ATTRIBUTE_ID_CLUSTER_ATTRIBUTES: {
        return manipulator(
            &d_clusterAttributes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_ATTRIBUTES]);
    }
    case ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG: {
        return manipulator(
            &d_clusterMonitorConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG]);
    }
    case ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG: {
        return manipulator(
            &d_messageThrottleConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ClusterDefinition::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& ClusterDefinition::name()
{
    return d_name;
}

inline bsl::vector<ClusterNode>& ClusterDefinition::nodes()
{
    return d_nodes;
}

inline PartitionConfig& ClusterDefinition::partitionConfig()
{
    return d_partitionConfig;
}

inline MasterAssignmentAlgorithm::Value& ClusterDefinition::masterAssignment()
{
    return d_masterAssignment;
}

inline ElectorConfig& ClusterDefinition::elector()
{
    return d_elector;
}

inline QueueOperationsConfig& ClusterDefinition::queueOperations()
{
    return d_queueOperations;
}

inline ClusterAttributes& ClusterDefinition::clusterAttributes()
{
    return d_clusterAttributes;
}

inline ClusterMonitorConfig& ClusterDefinition::clusterMonitorConfig()
{
    return d_clusterMonitorConfig;
}

inline MessageThrottleConfig& ClusterDefinition::messageThrottleConfig()
{
    return d_messageThrottleConfig;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ClusterDefinition::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_nodes, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_partitionConfig,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_masterAssignment,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MASTER_ASSIGNMENT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_elector, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTOR]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queueOperations,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_clusterAttributes,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_ATTRIBUTES]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_clusterMonitorConfig,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_messageThrottleConfig,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ClusterDefinition::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_NODES: {
        return accessor(d_nodes, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES]);
    }
    case ATTRIBUTE_ID_PARTITION_CONFIG: {
        return accessor(
            d_partitionConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_CONFIG]);
    }
    case ATTRIBUTE_ID_MASTER_ASSIGNMENT: {
        return accessor(
            d_masterAssignment,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MASTER_ASSIGNMENT]);
    }
    case ATTRIBUTE_ID_ELECTOR: {
        return accessor(d_elector,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTOR]);
    }
    case ATTRIBUTE_ID_QUEUE_OPERATIONS: {
        return accessor(
            d_queueOperations,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS]);
    }
    case ATTRIBUTE_ID_CLUSTER_ATTRIBUTES: {
        return accessor(
            d_clusterAttributes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_ATTRIBUTES]);
    }
    case ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG: {
        return accessor(
            d_clusterMonitorConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG]);
    }
    case ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG: {
        return accessor(
            d_messageThrottleConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ClusterDefinition::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& ClusterDefinition::name() const
{
    return d_name;
}

inline const bsl::vector<ClusterNode>& ClusterDefinition::nodes() const
{
    return d_nodes;
}

inline const PartitionConfig& ClusterDefinition::partitionConfig() const
{
    return d_partitionConfig;
}

inline MasterAssignmentAlgorithm::Value
ClusterDefinition::masterAssignment() const
{
    return d_masterAssignment;
}

inline const ElectorConfig& ClusterDefinition::elector() const
{
    return d_elector;
}

inline const QueueOperationsConfig& ClusterDefinition::queueOperations() const
{
    return d_queueOperations;
}

inline const ClusterAttributes& ClusterDefinition::clusterAttributes() const
{
    return d_clusterAttributes;
}

inline const ClusterMonitorConfig&
ClusterDefinition::clusterMonitorConfig() const
{
    return d_clusterMonitorConfig;
}

inline const MessageThrottleConfig&
ClusterDefinition::messageThrottleConfig() const
{
    return d_messageThrottleConfig;
}

// ----------------------------
// class ClusterProxyDefinition
// ----------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void ClusterProxyDefinition::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->name());
    hashAppend(hashAlgorithm, this->nodes());
    hashAppend(hashAlgorithm, this->queueOperations());
    hashAppend(hashAlgorithm, this->clusterMonitorConfig());
    hashAppend(hashAlgorithm, this->messageThrottleConfig());
}

inline bool
ClusterProxyDefinition::isEqualTo(const ClusterProxyDefinition& rhs) const
{
    return this->name() == rhs.name() && this->nodes() == rhs.nodes() &&
           this->queueOperations() == rhs.queueOperations() &&
           this->clusterMonitorConfig() == rhs.clusterMonitorConfig() &&
           this->messageThrottleConfig() == rhs.messageThrottleConfig();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ClusterProxyDefinition::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_nodes, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_queueOperations,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_clusterMonitorConfig,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_messageThrottleConfig,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ClusterProxyDefinition::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_NODES: {
        return manipulator(&d_nodes,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES]);
    }
    case ATTRIBUTE_ID_QUEUE_OPERATIONS: {
        return manipulator(
            &d_queueOperations,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS]);
    }
    case ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG: {
        return manipulator(
            &d_clusterMonitorConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG]);
    }
    case ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG: {
        return manipulator(
            &d_messageThrottleConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ClusterProxyDefinition::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& ClusterProxyDefinition::name()
{
    return d_name;
}

inline bsl::vector<ClusterNode>& ClusterProxyDefinition::nodes()
{
    return d_nodes;
}

inline QueueOperationsConfig& ClusterProxyDefinition::queueOperations()
{
    return d_queueOperations;
}

inline ClusterMonitorConfig& ClusterProxyDefinition::clusterMonitorConfig()
{
    return d_clusterMonitorConfig;
}

inline MessageThrottleConfig& ClusterProxyDefinition::messageThrottleConfig()
{
    return d_messageThrottleConfig;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ClusterProxyDefinition::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_nodes, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queueOperations,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_clusterMonitorConfig,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_messageThrottleConfig,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ClusterProxyDefinition::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_NODES: {
        return accessor(d_nodes, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES]);
    }
    case ATTRIBUTE_ID_QUEUE_OPERATIONS: {
        return accessor(
            d_queueOperations,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS]);
    }
    case ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG: {
        return accessor(
            d_clusterMonitorConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG]);
    }
    case ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG: {
        return accessor(
            d_messageThrottleConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ClusterProxyDefinition::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& ClusterProxyDefinition::name() const
{
    return d_name;
}

inline const bsl::vector<ClusterNode>& ClusterProxyDefinition::nodes() const
{
    return d_nodes;
}

inline const QueueOperationsConfig&
ClusterProxyDefinition::queueOperations() const
{
    return d_queueOperations;
}

inline const ClusterMonitorConfig&
ClusterProxyDefinition::clusterMonitorConfig() const
{
    return d_clusterMonitorConfig;
}

inline const MessageThrottleConfig&
ClusterProxyDefinition::messageThrottleConfig() const
{
    return d_messageThrottleConfig;
}

// -----------------
// class StatsConfig
// -----------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StatsConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->snapshotInterval());
    hashAppend(hashAlgorithm, this->appIdTagDomains());
    hashAppend(hashAlgorithm, this->plugins());
    hashAppend(hashAlgorithm, this->printer());
}

inline bool StatsConfig::isEqualTo(const StatsConfig& rhs) const
{
    return this->snapshotInterval() == rhs.snapshotInterval() &&
           this->appIdTagDomains() == rhs.appIdTagDomains() &&
           this->plugins() == rhs.plugins() &&
           this->printer() == rhs.printer();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StatsConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_snapshotInterval,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SNAPSHOT_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_appIdTagDomains,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID_TAG_DOMAINS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_plugins,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_printer,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINTER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StatsConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_SNAPSHOT_INTERVAL: {
        return manipulator(
            &d_snapshotInterval,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SNAPSHOT_INTERVAL]);
    }
    case ATTRIBUTE_ID_APP_ID_TAG_DOMAINS: {
        return manipulator(
            &d_appIdTagDomains,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID_TAG_DOMAINS]);
    }
    case ATTRIBUTE_ID_PLUGINS: {
        return manipulator(&d_plugins,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS]);
    }
    case ATTRIBUTE_ID_PRINTER: {
        return manipulator(&d_printer,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINTER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StatsConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& StatsConfig::snapshotInterval()
{
    return d_snapshotInterval;
}

inline bsl::vector<bsl::string>& StatsConfig::appIdTagDomains()
{
    return d_appIdTagDomains;
}

inline bsl::vector<StatPluginConfig>& StatsConfig::plugins()
{
    return d_plugins;
}

inline StatsPrinterConfig& StatsConfig::printer()
{
    return d_printer;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StatsConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_snapshotInterval,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SNAPSHOT_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_appIdTagDomains,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID_TAG_DOMAINS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_plugins, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_printer, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINTER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StatsConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_SNAPSHOT_INTERVAL: {
        return accessor(
            d_snapshotInterval,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SNAPSHOT_INTERVAL]);
    }
    case ATTRIBUTE_ID_APP_ID_TAG_DOMAINS: {
        return accessor(
            d_appIdTagDomains,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID_TAG_DOMAINS]);
    }
    case ATTRIBUTE_ID_PLUGINS: {
        return accessor(d_plugins,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS]);
    }
    case ATTRIBUTE_ID_PRINTER: {
        return accessor(d_printer,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINTER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StatsConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int StatsConfig::snapshotInterval() const
{
    return d_snapshotInterval;
}

inline const bsl::vector<bsl::string>& StatsConfig::appIdTagDomains() const
{
    return d_appIdTagDomains;
}

inline const bsl::vector<StatPluginConfig>& StatsConfig::plugins() const
{
    return d_plugins;
}

inline const StatsPrinterConfig& StatsConfig::printer() const
{
    return d_printer;
}

// ---------------
// class AppConfig
// ---------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void AppConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->brokerInstanceName());
    hashAppend(hashAlgorithm, this->brokerVersion());
    hashAppend(hashAlgorithm, this->configVersion());
    hashAppend(hashAlgorithm, this->etcDir());
    hashAppend(hashAlgorithm, this->hostName());
    hashAppend(hashAlgorithm, this->hostTags());
    hashAppend(hashAlgorithm, this->hostDataCenter());
    hashAppend(hashAlgorithm, this->isRunningOnDev());
    hashAppend(hashAlgorithm, this->logsObserverMaxSize());
    hashAppend(hashAlgorithm, this->latencyMonitorDomain());
    hashAppend(hashAlgorithm, this->dispatcherConfig());
    hashAppend(hashAlgorithm, this->stats());
    hashAppend(hashAlgorithm, this->networkInterfaces());
    hashAppend(hashAlgorithm, this->bmqconfConfig());
    hashAppend(hashAlgorithm, this->plugins());
    hashAppend(hashAlgorithm, this->messagePropertiesV2());
    hashAppend(hashAlgorithm, this->configureStream());
    hashAppend(hashAlgorithm, this->advertiseSubscriptions());
}

inline bool AppConfig::isEqualTo(const AppConfig& rhs) const
{
    return this->brokerInstanceName() == rhs.brokerInstanceName() &&
           this->brokerVersion() == rhs.brokerVersion() &&
           this->configVersion() == rhs.configVersion() &&
           this->etcDir() == rhs.etcDir() &&
           this->hostName() == rhs.hostName() &&
           this->hostTags() == rhs.hostTags() &&
           this->hostDataCenter() == rhs.hostDataCenter() &&
           this->isRunningOnDev() == rhs.isRunningOnDev() &&
           this->logsObserverMaxSize() == rhs.logsObserverMaxSize() &&
           this->latencyMonitorDomain() == rhs.latencyMonitorDomain() &&
           this->dispatcherConfig() == rhs.dispatcherConfig() &&
           this->stats() == rhs.stats() &&
           this->networkInterfaces() == rhs.networkInterfaces() &&
           this->bmqconfConfig() == rhs.bmqconfConfig() &&
           this->plugins() == rhs.plugins() &&
           this->messagePropertiesV2() == rhs.messagePropertiesV2() &&
           this->configureStream() == rhs.configureStream() &&
           this->advertiseSubscriptions() == rhs.advertiseSubscriptions();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int AppConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(
        &d_brokerInstanceName,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_INSTANCE_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_brokerVersion,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_configVersion,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG_VERSION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_etcDir,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ETC_DIR]);
    if (ret) {
        return ret;
    }

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

    ret = manipulator(&d_hostDataCenter,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_DATA_CENTER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_isRunningOnDev,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_RUNNING_ON_DEV]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_logsObserverMaxSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGS_OBSERVER_MAX_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_latencyMonitorDomain,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_MONITOR_DOMAIN]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_dispatcherConfig,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DISPATCHER_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_stats, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_networkInterfaces,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NETWORK_INTERFACES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_bmqconfConfig,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BMQCONF_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_plugins,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_messagePropertiesV2,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES_V2]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_configureStream,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_STREAM]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_advertiseSubscriptions,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_SUBSCRIPTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int AppConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_BROKER_INSTANCE_NAME: {
        return manipulator(
            &d_brokerInstanceName,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_INSTANCE_NAME]);
    }
    case ATTRIBUTE_ID_BROKER_VERSION: {
        return manipulator(
            &d_brokerVersion,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION]);
    }
    case ATTRIBUTE_ID_CONFIG_VERSION: {
        return manipulator(
            &d_configVersion,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG_VERSION]);
    }
    case ATTRIBUTE_ID_ETC_DIR: {
        return manipulator(&d_etcDir,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ETC_DIR]);
    }
    case ATTRIBUTE_ID_HOST_NAME: {
        return manipulator(&d_hostName,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME]);
    }
    case ATTRIBUTE_ID_HOST_TAGS: {
        return manipulator(&d_hostTags,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_TAGS]);
    }
    case ATTRIBUTE_ID_HOST_DATA_CENTER: {
        return manipulator(
            &d_hostDataCenter,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_DATA_CENTER]);
    }
    case ATTRIBUTE_ID_IS_RUNNING_ON_DEV: {
        return manipulator(
            &d_isRunningOnDev,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_RUNNING_ON_DEV]);
    }
    case ATTRIBUTE_ID_LOGS_OBSERVER_MAX_SIZE: {
        return manipulator(
            &d_logsObserverMaxSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGS_OBSERVER_MAX_SIZE]);
    }
    case ATTRIBUTE_ID_LATENCY_MONITOR_DOMAIN: {
        return manipulator(
            &d_latencyMonitorDomain,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_MONITOR_DOMAIN]);
    }
    case ATTRIBUTE_ID_DISPATCHER_CONFIG: {
        return manipulator(
            &d_dispatcherConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DISPATCHER_CONFIG]);
    }
    case ATTRIBUTE_ID_STATS: {
        return manipulator(&d_stats,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATS]);
    }
    case ATTRIBUTE_ID_NETWORK_INTERFACES: {
        return manipulator(
            &d_networkInterfaces,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NETWORK_INTERFACES]);
    }
    case ATTRIBUTE_ID_BMQCONF_CONFIG: {
        return manipulator(
            &d_bmqconfConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BMQCONF_CONFIG]);
    }
    case ATTRIBUTE_ID_PLUGINS: {
        return manipulator(&d_plugins,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS]);
    }
    case ATTRIBUTE_ID_MESSAGE_PROPERTIES_V2: {
        return manipulator(
            &d_messagePropertiesV2,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES_V2]);
    }
    case ATTRIBUTE_ID_CONFIGURE_STREAM: {
        return manipulator(
            &d_configureStream,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_STREAM]);
    }
    case ATTRIBUTE_ID_ADVERTISE_SUBSCRIPTIONS: {
        return manipulator(
            &d_advertiseSubscriptions,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_SUBSCRIPTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int AppConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& AppConfig::brokerInstanceName()
{
    return d_brokerInstanceName;
}

inline int& AppConfig::brokerVersion()
{
    return d_brokerVersion;
}

inline int& AppConfig::configVersion()
{
    return d_configVersion;
}

inline bsl::string& AppConfig::etcDir()
{
    return d_etcDir;
}

inline bsl::string& AppConfig::hostName()
{
    return d_hostName;
}

inline bsl::string& AppConfig::hostTags()
{
    return d_hostTags;
}

inline bsl::string& AppConfig::hostDataCenter()
{
    return d_hostDataCenter;
}

inline bool& AppConfig::isRunningOnDev()
{
    return d_isRunningOnDev;
}

inline int& AppConfig::logsObserverMaxSize()
{
    return d_logsObserverMaxSize;
}

inline bsl::string& AppConfig::latencyMonitorDomain()
{
    return d_latencyMonitorDomain;
}

inline DispatcherConfig& AppConfig::dispatcherConfig()
{
    return d_dispatcherConfig;
}

inline StatsConfig& AppConfig::stats()
{
    return d_stats;
}

inline NetworkInterfaces& AppConfig::networkInterfaces()
{
    return d_networkInterfaces;
}

inline BmqconfConfig& AppConfig::bmqconfConfig()
{
    return d_bmqconfConfig;
}

inline Plugins& AppConfig::plugins()
{
    return d_plugins;
}

inline MessagePropertiesV2& AppConfig::messagePropertiesV2()
{
    return d_messagePropertiesV2;
}

inline bool& AppConfig::configureStream()
{
    return d_configureStream;
}

inline bool& AppConfig::advertiseSubscriptions()
{
    return d_advertiseSubscriptions;
}

// ACCESSORS
template <typename t_ACCESSOR>
int AppConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_brokerInstanceName,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_INSTANCE_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_brokerVersion,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_configVersion,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG_VERSION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_etcDir, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ETC_DIR]);
    if (ret) {
        return ret;
    }

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

    ret = accessor(d_hostDataCenter,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_DATA_CENTER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_isRunningOnDev,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_RUNNING_ON_DEV]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_logsObserverMaxSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGS_OBSERVER_MAX_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_latencyMonitorDomain,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_MONITOR_DOMAIN]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_dispatcherConfig,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DISPATCHER_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_stats, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_networkInterfaces,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NETWORK_INTERFACES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_bmqconfConfig,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BMQCONF_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_plugins, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_messagePropertiesV2,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES_V2]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_configureStream,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_STREAM]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_advertiseSubscriptions,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_SUBSCRIPTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int AppConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_BROKER_INSTANCE_NAME: {
        return accessor(
            d_brokerInstanceName,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_INSTANCE_NAME]);
    }
    case ATTRIBUTE_ID_BROKER_VERSION: {
        return accessor(d_brokerVersion,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION]);
    }
    case ATTRIBUTE_ID_CONFIG_VERSION: {
        return accessor(d_configVersion,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG_VERSION]);
    }
    case ATTRIBUTE_ID_ETC_DIR: {
        return accessor(d_etcDir,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ETC_DIR]);
    }
    case ATTRIBUTE_ID_HOST_NAME: {
        return accessor(d_hostName,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME]);
    }
    case ATTRIBUTE_ID_HOST_TAGS: {
        return accessor(d_hostTags,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_TAGS]);
    }
    case ATTRIBUTE_ID_HOST_DATA_CENTER: {
        return accessor(
            d_hostDataCenter,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_DATA_CENTER]);
    }
    case ATTRIBUTE_ID_IS_RUNNING_ON_DEV: {
        return accessor(
            d_isRunningOnDev,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_RUNNING_ON_DEV]);
    }
    case ATTRIBUTE_ID_LOGS_OBSERVER_MAX_SIZE: {
        return accessor(
            d_logsObserverMaxSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGS_OBSERVER_MAX_SIZE]);
    }
    case ATTRIBUTE_ID_LATENCY_MONITOR_DOMAIN: {
        return accessor(
            d_latencyMonitorDomain,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_MONITOR_DOMAIN]);
    }
    case ATTRIBUTE_ID_DISPATCHER_CONFIG: {
        return accessor(
            d_dispatcherConfig,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DISPATCHER_CONFIG]);
    }
    case ATTRIBUTE_ID_STATS: {
        return accessor(d_stats, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATS]);
    }
    case ATTRIBUTE_ID_NETWORK_INTERFACES: {
        return accessor(
            d_networkInterfaces,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NETWORK_INTERFACES]);
    }
    case ATTRIBUTE_ID_BMQCONF_CONFIG: {
        return accessor(d_bmqconfConfig,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BMQCONF_CONFIG]);
    }
    case ATTRIBUTE_ID_PLUGINS: {
        return accessor(d_plugins,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS]);
    }
    case ATTRIBUTE_ID_MESSAGE_PROPERTIES_V2: {
        return accessor(
            d_messagePropertiesV2,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES_V2]);
    }
    case ATTRIBUTE_ID_CONFIGURE_STREAM: {
        return accessor(
            d_configureStream,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_STREAM]);
    }
    case ATTRIBUTE_ID_ADVERTISE_SUBSCRIPTIONS: {
        return accessor(
            d_advertiseSubscriptions,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_SUBSCRIPTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int AppConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& AppConfig::brokerInstanceName() const
{
    return d_brokerInstanceName;
}

inline int AppConfig::brokerVersion() const
{
    return d_brokerVersion;
}

inline int AppConfig::configVersion() const
{
    return d_configVersion;
}

inline const bsl::string& AppConfig::etcDir() const
{
    return d_etcDir;
}

inline const bsl::string& AppConfig::hostName() const
{
    return d_hostName;
}

inline const bsl::string& AppConfig::hostTags() const
{
    return d_hostTags;
}

inline const bsl::string& AppConfig::hostDataCenter() const
{
    return d_hostDataCenter;
}

inline bool AppConfig::isRunningOnDev() const
{
    return d_isRunningOnDev;
}

inline int AppConfig::logsObserverMaxSize() const
{
    return d_logsObserverMaxSize;
}

inline const bsl::string& AppConfig::latencyMonitorDomain() const
{
    return d_latencyMonitorDomain;
}

inline const DispatcherConfig& AppConfig::dispatcherConfig() const
{
    return d_dispatcherConfig;
}

inline const StatsConfig& AppConfig::stats() const
{
    return d_stats;
}

inline const NetworkInterfaces& AppConfig::networkInterfaces() const
{
    return d_networkInterfaces;
}

inline const BmqconfConfig& AppConfig::bmqconfConfig() const
{
    return d_bmqconfConfig;
}

inline const Plugins& AppConfig::plugins() const
{
    return d_plugins;
}

inline const MessagePropertiesV2& AppConfig::messagePropertiesV2() const
{
    return d_messagePropertiesV2;
}

inline bool AppConfig::configureStream() const
{
    return d_configureStream;
}

inline bool AppConfig::advertiseSubscriptions() const
{
    return d_advertiseSubscriptions;
}

// ------------------------
// class ClustersDefinition
// ------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void ClustersDefinition::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->myClusters());
    hashAppend(hashAlgorithm, this->myReverseClusters());
    hashAppend(hashAlgorithm, this->myVirtualClusters());
    hashAppend(hashAlgorithm, this->proxyClusters());
    hashAppend(hashAlgorithm, this->reversedClusterConnections());
}

inline bool ClustersDefinition::isEqualTo(const ClustersDefinition& rhs) const
{
    return this->myClusters() == rhs.myClusters() &&
           this->myReverseClusters() == rhs.myReverseClusters() &&
           this->myVirtualClusters() == rhs.myVirtualClusters() &&
           this->proxyClusters() == rhs.proxyClusters() &&
           this->reversedClusterConnections() ==
               rhs.reversedClusterConnections();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ClustersDefinition::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_myClusters,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_CLUSTERS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_myReverseClusters,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_REVERSE_CLUSTERS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_myVirtualClusters,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_VIRTUAL_CLUSTERS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_proxyClusters,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROXY_CLUSTERS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_reversedClusterConnections,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REVERSED_CLUSTER_CONNECTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ClustersDefinition::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MY_CLUSTERS: {
        return manipulator(&d_myClusters,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_CLUSTERS]);
    }
    case ATTRIBUTE_ID_MY_REVERSE_CLUSTERS: {
        return manipulator(
            &d_myReverseClusters,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_REVERSE_CLUSTERS]);
    }
    case ATTRIBUTE_ID_MY_VIRTUAL_CLUSTERS: {
        return manipulator(
            &d_myVirtualClusters,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_VIRTUAL_CLUSTERS]);
    }
    case ATTRIBUTE_ID_PROXY_CLUSTERS: {
        return manipulator(
            &d_proxyClusters,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROXY_CLUSTERS]);
    }
    case ATTRIBUTE_ID_REVERSED_CLUSTER_CONNECTIONS: {
        return manipulator(&d_reversedClusterConnections,
                           ATTRIBUTE_INFO_ARRAY
                               [ATTRIBUTE_INDEX_REVERSED_CLUSTER_CONNECTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ClustersDefinition::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::vector<ClusterDefinition>& ClustersDefinition::myClusters()
{
    return d_myClusters;
}

inline bsl::vector<bsl::string>& ClustersDefinition::myReverseClusters()
{
    return d_myReverseClusters;
}

inline bsl::vector<VirtualClusterInformation>&
ClustersDefinition::myVirtualClusters()
{
    return d_myVirtualClusters;
}

inline bsl::vector<ClusterProxyDefinition>& ClustersDefinition::proxyClusters()
{
    return d_proxyClusters;
}

inline bsl::vector<ReversedClusterConnection>&
ClustersDefinition::reversedClusterConnections()
{
    return d_reversedClusterConnections;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ClustersDefinition::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_myClusters,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_CLUSTERS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_myReverseClusters,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_REVERSE_CLUSTERS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_myVirtualClusters,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_VIRTUAL_CLUSTERS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_proxyClusters,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROXY_CLUSTERS]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_reversedClusterConnections,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REVERSED_CLUSTER_CONNECTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ClustersDefinition::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MY_CLUSTERS: {
        return accessor(d_myClusters,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_CLUSTERS]);
    }
    case ATTRIBUTE_ID_MY_REVERSE_CLUSTERS: {
        return accessor(
            d_myReverseClusters,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_REVERSE_CLUSTERS]);
    }
    case ATTRIBUTE_ID_MY_VIRTUAL_CLUSTERS: {
        return accessor(
            d_myVirtualClusters,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_VIRTUAL_CLUSTERS]);
    }
    case ATTRIBUTE_ID_PROXY_CLUSTERS: {
        return accessor(d_proxyClusters,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROXY_CLUSTERS]);
    }
    case ATTRIBUTE_ID_REVERSED_CLUSTER_CONNECTIONS: {
        return accessor(d_reversedClusterConnections,
                        ATTRIBUTE_INFO_ARRAY
                            [ATTRIBUTE_INDEX_REVERSED_CLUSTER_CONNECTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ClustersDefinition::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::vector<ClusterDefinition>&
ClustersDefinition::myClusters() const
{
    return d_myClusters;
}

inline const bsl::vector<bsl::string>&
ClustersDefinition::myReverseClusters() const
{
    return d_myReverseClusters;
}

inline const bsl::vector<VirtualClusterInformation>&
ClustersDefinition::myVirtualClusters() const
{
    return d_myVirtualClusters;
}

inline const bsl::vector<ClusterProxyDefinition>&
ClustersDefinition::proxyClusters() const
{
    return d_proxyClusters;
}

inline const bsl::vector<ReversedClusterConnection>&
ClustersDefinition::reversedClusterConnections() const
{
    return d_reversedClusterConnections;
}

// -------------------
// class Configuration
// -------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Configuration::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_taskConfig,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TASK_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_appConfig,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Configuration::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_TASK_CONFIG: {
        return manipulator(&d_taskConfig,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TASK_CONFIG]);
    }
    case ATTRIBUTE_ID_APP_CONFIG: {
        return manipulator(&d_appConfig,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Configuration::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline TaskConfig& Configuration::taskConfig()
{
    return d_taskConfig;
}

inline AppConfig& Configuration::appConfig()
{
    return d_appConfig;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Configuration::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_taskConfig,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TASK_CONFIG]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_appConfig,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_CONFIG]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Configuration::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_TASK_CONFIG: {
        return accessor(d_taskConfig,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TASK_CONFIG]);
    }
    case ATTRIBUTE_ID_APP_CONFIG: {
        return accessor(d_appConfig,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_CONFIG]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Configuration::accessAttribute(t_ACCESSOR& accessor,
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

inline const TaskConfig& Configuration::taskConfig() const
{
    return d_taskConfig;
}

inline const AppConfig& Configuration::appConfig() const
{
    return d_appConfig;
}

}  // close package namespace

// FREE FUNCTIONS

}  // close enterprise namespace
#endif

// GENERATED BY BLP_BAS_CODEGEN_2024.07.04.1
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbcfg --msgComponent messages mqbcfg.xsd
// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright 2024 Bloomberg Finance L.P. All rights reserved.
//      Property of Bloomberg Finance L.P. (BFLP)
//      This software is made available solely pursuant to the
//      terms of a BFLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
