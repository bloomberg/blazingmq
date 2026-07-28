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
#include <bsl_type_traits.h>

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
class Credential;
}
namespace mqbcfg {
class Disallow;
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
class LogDumpConfig;
}
namespace mqbcfg {
class MessagePropertiesV2;
}
namespace mqbcfg {
class MessageThrottleConfig;
}
namespace mqbcfg {
class PluginSettingValue;
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
class StorageSyncConfig;
}
namespace mqbcfg {
class SyslogConfig;
}
namespace mqbcfg {
class TcpClusterNodeConnection;
}
namespace mqbcfg {
class TcpInterfaceListener;
}
namespace mqbcfg {
class TlsConfig;
}
namespace mqbcfg {
class VirtualClusterInformation;
}
namespace mqbcfg {
class AnonymousCredential;
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
class PartitionConfig;
}
namespace mqbcfg {
class PluginSettingKeyValue;
}
namespace mqbcfg {
class StatPluginConfigPrometheus;
}
namespace mqbcfg {
class StatsPrinterConfig;
}
namespace mqbcfg {
class TcpInterfaceConfig;
}
namespace mqbcfg {
class AuthenticatorPluginConfig;
}
namespace mqbcfg {
class AuthorizerPluginConfig;
}
namespace mqbcfg {
class ClusterNode;
}
namespace mqbcfg {
class CredentialProviderConfig;
}
namespace mqbcfg {
class DispatcherConfig;
}
namespace mqbcfg {
class NetworkInterfaces;
}
namespace mqbcfg {
class StatPluginConfig;
}
namespace mqbcfg {
class TaskConfig;
}
namespace mqbcfg {
class AuthenticatorConfig;
}
namespace mqbcfg {
class AuthorizerConfig;
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
    enum Value {
        e_NEWDELETE      = 0,
        e_COUNTING       = 1,
        e_STACKTRACETEST = 2,

        NEWDELETE      = e_NEWDELETE,
        COUNTING       = e_COUNTING,
        STACKTRACETEST = e_STACKTRACETEST
    };

    enum { k_NUM_ENUMERATORS = 3, NUM_ENUMERATORS = k_NUM_ENUMERATORS };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    /// Return the string representation exactly matching the enumerator
    /// name corresponding to the specified enumeration `value`.
    static const char* toString(Value value);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string` of the specified `stringLength`.  Return 0 on
    /// success, and a non-zero value with no effect on `result` otherwise
    /// (i.e., `string` does not match any enumerator).
    static int fromString(Value* result, const char* string, int stringLength);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `string` does not match any
    /// enumerator).
    static int fromString(Value* result, const bsl::string& string);

    /// Load into the specified `result` the enumerator matching the
    /// specified `number`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `number` does not match any
    /// enumerator).
    static int fromInt(Value* result, int number);

    /// Write to the specified `stream` the string representation of
    /// the specified enumeration `value`.  Return a reference to
    /// the modifiable `stream`.
    static bsl::ostream& print(bsl::ostream& stream, Value value);

    // HIDDEN FRIENDS
    /// Format the specified `rhs` to the specified output `stream` and
    /// return a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    {
        return AllocatorType::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mqbcfg::AllocatorType);

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `BmqconfConfig` having the default value.
    BmqconfConfig();

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

    /// Return a reference to the modifiable "CacheTTLSeconds" attribute of
    /// this object.
    int& cacheTTLSeconds();

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

    /// Return the value of the "CacheTTLSeconds" attribute of this object.
    int cacheTTLSeconds() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const BmqconfConfig& lhs, const BmqconfConfig& rhs)
    {
        return lhs.cacheTTLSeconds() == rhs.cacheTTLSeconds();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const BmqconfConfig& lhs, const BmqconfConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const BmqconfConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `BmqconfConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const BmqconfConfig& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.cacheTTLSeconds());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::BmqconfConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::BmqconfConfig> : bsl::true_type {};

namespace mqbcfg {

// =======================
// class ClusterAttributes
// =======================

/// Type representing the attributes specific to a cluster.
/// isCSLModeEnabled...............: indicates if CSL is enabled for this
/// cluster isFSMWorkflow..................: indicates if CSL FSM workflow is
/// enabled for this cluster.  This flag *must* be false if 'isCSLModeEnabled'
/// is false.  doesFSMwriteQLIST..............: indicates whether the broker
/// still writes to the to-be-deprecated QLIST file when FSM workflow is
/// enabled.  If above 'isFSMWorkflow' flag is false, this flag is ignored.
/// clusterFsmWatchdogTimeoutSec...: timeout duration in seconds for Cluster
/// FSM watchdog.  Only applies when 'isFSMWorkflow' is true.
/// clusterFsmWatchdogNumRetries...: number of retries for Cluster FSM watchdog
/// before we give up and terminate the broker.  Only applies when
/// 'isFSMWorkflow' is true.  partitionFsmWatchdogTimeoutSec.: timeout duration
/// in seconds for Partition FSM watchdog.  Only applies when 'isFSMWorkflow'
/// is true.  partitionFsmWatchdogNumRetries.: number of retries for Partition
/// FSM watchdog before we give up and terminate the broker.  Only applies when
/// 'isFSMWorkflow' is true.
class ClusterAttributes {
    // INSTANCE DATA

    int  d_clusterFsmWatchdogTimeoutSec;
    int  d_clusterFsmWatchdogNumRetries;
    int  d_partitionFsmWatchdogTimeoutSec;
    int  d_partitionFsmWatchdogNumRetries;
    bool d_isCSLModeEnabled;
    bool d_isFSMWorkflow;
    bool d_doesFSMwriteQLIST;

    // PRIVATE ACCESSORS

    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const ClusterAttributes& rhs) const;

  public:
    // TYPES

    enum {
        ATTRIBUTE_ID_IS_C_S_L_MODE_ENABLED              = 0,
        ATTRIBUTE_ID_IS_F_S_M_WORKFLOW                  = 1,
        ATTRIBUTE_ID_DOES_F_S_MWRITE_Q_L_I_S_T          = 2,
        ATTRIBUTE_ID_CLUSTER_FSM_WATCHDOG_TIMEOUT_SEC   = 3,
        ATTRIBUTE_ID_CLUSTER_FSM_WATCHDOG_NUM_RETRIES   = 4,
        ATTRIBUTE_ID_PARTITION_FSM_WATCHDOG_TIMEOUT_SEC = 5,
        ATTRIBUTE_ID_PARTITION_FSM_WATCHDOG_NUM_RETRIES = 6
    };

    enum { NUM_ATTRIBUTES = 7 };

    enum {
        ATTRIBUTE_INDEX_IS_C_S_L_MODE_ENABLED              = 0,
        ATTRIBUTE_INDEX_IS_F_S_M_WORKFLOW                  = 1,
        ATTRIBUTE_INDEX_DOES_F_S_MWRITE_Q_L_I_S_T          = 2,
        ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_TIMEOUT_SEC   = 3,
        ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_NUM_RETRIES   = 4,
        ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_TIMEOUT_SEC = 5,
        ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_NUM_RETRIES = 6
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_IS_C_S_L_MODE_ENABLED;

    static const bool DEFAULT_INITIALIZER_IS_F_S_M_WORKFLOW;

    static const bool DEFAULT_INITIALIZER_DOES_F_S_MWRITE_Q_L_I_S_T;

    static const int DEFAULT_INITIALIZER_CLUSTER_FSM_WATCHDOG_TIMEOUT_SEC;

    static const int DEFAULT_INITIALIZER_CLUSTER_FSM_WATCHDOG_NUM_RETRIES;

    static const int DEFAULT_INITIALIZER_PARTITION_FSM_WATCHDOG_TIMEOUT_SEC;

    static const int DEFAULT_INITIALIZER_PARTITION_FSM_WATCHDOG_NUM_RETRIES;

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

    /// Create an object of type `ClusterAttributes` having the default value.
    ClusterAttributes();

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

    /// Return a reference to the modifiable "IsCSLModeEnabled" attribute of
    /// this object.
    bool& isCSLModeEnabled();

    /// Return a reference to the modifiable "IsFSMWorkflow" attribute of this
    /// object.
    bool& isFSMWorkflow();

    /// Return a reference to the modifiable "DoesFSMwriteQLIST" attribute of
    /// this object.
    bool& doesFSMwriteQLIST();

    /// Return a reference to the modifiable "ClusterFsmWatchdogTimeoutSec"
    /// attribute of this object.
    int& clusterFsmWatchdogTimeoutSec();

    /// Return a reference to the modifiable "ClusterFsmWatchdogNumRetries"
    /// attribute of this object.
    int& clusterFsmWatchdogNumRetries();

    /// Return a reference to the modifiable "PartitionFsmWatchdogTimeoutSec"
    /// attribute of this object.
    int& partitionFsmWatchdogTimeoutSec();

    /// Return a reference to the modifiable "PartitionFsmWatchdogNumRetries"
    /// attribute of this object.
    int& partitionFsmWatchdogNumRetries();

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

    /// Return the value of the "IsCSLModeEnabled" attribute of this object.
    bool isCSLModeEnabled() const;

    /// Return the value of the "IsFSMWorkflow" attribute of this object.
    bool isFSMWorkflow() const;

    /// Return the value of the "DoesFSMwriteQLIST" attribute of this object.
    bool doesFSMwriteQLIST() const;

    /// Return the value of the "ClusterFsmWatchdogTimeoutSec" attribute of
    /// this object.
    int clusterFsmWatchdogTimeoutSec() const;

    /// Return the value of the "ClusterFsmWatchdogNumRetries" attribute of
    /// this object.
    int clusterFsmWatchdogNumRetries() const;

    /// Return the value of the "PartitionFsmWatchdogTimeoutSec" attribute of
    /// this object.
    int partitionFsmWatchdogTimeoutSec() const;

    /// Return the value of the "PartitionFsmWatchdogNumRetries" attribute of
    /// this object.
    int partitionFsmWatchdogNumRetries() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ClusterAttributes& lhs,
                           const ClusterAttributes& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ClusterAttributes& lhs,
                           const ClusterAttributes& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const ClusterAttributes& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ClusterAttributes`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const ClusterAttributes& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::ClusterAttributes);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::ClusterAttributes> : bsl::true_type {
};

namespace mqbcfg {

// ==========================
// class ClusterMonitorConfig
// ==========================

/// Type representing the configuration for cluster state monitor.
/// maxTimeLeader......: Time (in seconds) before alarming that the cluster's
/// leader is not 'active' maxTimeMaster......: Time (in seconds) before
/// alarming that a partition's master is not 'active' maxTimeNode........:
/// Time (in seconds) before alarming that a node is not 'available'
/// maxTimeFailover..: Time (in seconds) before alarming that failover hasn't
/// completed thresholdLeader....: Time (in seconds) before first notifying
/// observers that cluster's leader is not 'active'.  This time interval is
/// smaller than 'maxTimeLeader' because observing components may attempt to
/// heal the cluster state before an alarm is raised.  thresholdMaster....:
/// Time (in seconds) before notifying observers that a partition's master is
/// not 'active'.  This time interval is smaller than 'maxTimeMaster' because
/// observing components may attempt to heal the cluster state before an alarm
/// is raised.  thresholdNode......: Time (in seconds) before notifying
/// observers that a node is not 'available'.  This time interval is smaller
/// than 'maxTimeNode' because observing components may attempt to heal the
/// cluster state before an alarm is raised.  thresholdFailover..: Time (in
/// seconds) before notifying observers that failover has not completed.  This
/// time interval is smaller than 'maxTimeFailover' because observing
/// components may attempt to fix the issue before an alarm is raised.
class ClusterMonitorConfig {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `ClusterMonitorConfig` having the default
    /// value.
    ClusterMonitorConfig();

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

    /// Return a reference to the modifiable "MaxTimeLeader" attribute of this
    /// object.
    int& maxTimeLeader();

    /// Return a reference to the modifiable "MaxTimeMaster" attribute of this
    /// object.
    int& maxTimeMaster();

    /// Return a reference to the modifiable "MaxTimeNode" attribute of this
    /// object.
    int& maxTimeNode();

    /// Return a reference to the modifiable "MaxTimeFailover" attribute of
    /// this object.
    int& maxTimeFailover();

    /// Return a reference to the modifiable "ThresholdLeader" attribute of
    /// this object.
    int& thresholdLeader();

    /// Return a reference to the modifiable "ThresholdMaster" attribute of
    /// this object.
    int& thresholdMaster();

    /// Return a reference to the modifiable "ThresholdNode" attribute of this
    /// object.
    int& thresholdNode();

    /// Return a reference to the modifiable "ThresholdFailover" attribute of
    /// this object.
    int& thresholdFailover();

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

    /// Return the value of the "MaxTimeLeader" attribute of this object.
    int maxTimeLeader() const;

    /// Return the value of the "MaxTimeMaster" attribute of this object.
    int maxTimeMaster() const;

    /// Return the value of the "MaxTimeNode" attribute of this object.
    int maxTimeNode() const;

    /// Return the value of the "MaxTimeFailover" attribute of this object.
    int maxTimeFailover() const;

    /// Return the value of the "ThresholdLeader" attribute of this object.
    int thresholdLeader() const;

    /// Return the value of the "ThresholdMaster" attribute of this object.
    int thresholdMaster() const;

    /// Return the value of the "ThresholdNode" attribute of this object.
    int thresholdNode() const;

    /// Return the value of the "ThresholdFailover" attribute of this object.
    int thresholdFailover() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ClusterMonitorConfig& lhs,
                           const ClusterMonitorConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ClusterMonitorConfig& lhs,
                           const ClusterMonitorConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&               stream,
                                    const ClusterMonitorConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ClusterMonitorConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&           hashAlg,
                           const ClusterMonitorConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::ClusterMonitorConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::ClusterMonitorConfig>
: bsl::true_type {};

namespace mqbcfg {

// ================
// class Credential
// ================

/// Type representing a credential used for authentication.
/// This type is used to represent a credential that can be used for
/// authentication.  It contains an authentication mechanism and an identity.
class Credential {
    // INSTANCE DATA

    bsl::string d_mechanism;
    bsl::string d_identity;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_MECHANISM = 0, ATTRIBUTE_ID_IDENTITY = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_MECHANISM = 0, ATTRIBUTE_INDEX_IDENTITY = 1 };

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

    /// Create an object of type `Credential` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Credential(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Credential` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Credential(const Credential& original,
               bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Credential` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    Credential(Credential&& original) noexcept;

    /// Create an object of type `Credential` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    Credential(Credential&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Credential();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Credential& operator=(const Credential& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    Credential& operator=(Credential&& rhs);
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

    /// Return a reference to the modifiable "Mechanism" attribute of this
    /// object.
    bsl::string& mechanism();

    /// Return a reference to the modifiable "Identity" attribute of this
    /// object.
    bsl::string& identity();

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

    /// Return a reference offering non-modifiable access to the "Mechanism"
    /// attribute of this object.
    const bsl::string& mechanism() const;

    /// Return a reference offering non-modifiable access to the "Identity"
    /// attribute of this object.
    const bsl::string& identity() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const Credential& lhs, const Credential& rhs)
    {
        return lhs.mechanism() == rhs.mechanism() &&
               lhs.identity() == rhs.identity();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Credential& lhs, const Credential& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&     stream,
                                    const Credential& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Credential`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Credential& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.mechanism());
        hashAppend(hashAlg, object.identity());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::Credential);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::Credential> : bsl::true_type {};

namespace mqbcfg {

// ==============
// class Disallow
// ==============

/// Type representing the disallow anonymous credential configuration.
/// This type is used to indicate that anonymous authentication is not allowed
/// on the broker.  If this is set, the broker will not use the anonymous
/// authenticator plugin.  Authentication is required and clients which cannot
/// or do not authenticate will be rejected.
class Disallow {
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
    friend bool operator==(const Disallow&, const Disallow&) { return true; }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Disallow& lhs, const Disallow& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const Disallow& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Disallow`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const Disallow&)
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::Disallow);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::Disallow> : bsl::true_type {};

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `DispatcherProcessorParameters` having the
    /// default value.
    DispatcherProcessorParameters();

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

    /// Return a reference to the modifiable "QueueSize" attribute of this
    /// object.
    int& queueSize();

    /// Return a reference to the modifiable "QueueSizeLowWatermark" attribute
    /// of this object.
    int& queueSizeLowWatermark();

    /// Return a reference to the modifiable "QueueSizeHighWatermark" attribute
    /// of this object.
    int& queueSizeHighWatermark();

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

    /// Return the value of the "QueueSize" attribute of this object.
    int queueSize() const;

    /// Return the value of the "QueueSizeLowWatermark" attribute of this
    /// object.
    int queueSizeLowWatermark() const;

    /// Return the value of the "QueueSizeHighWatermark" attribute of this
    /// object.
    int queueSizeHighWatermark() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const DispatcherProcessorParameters& lhs,
                           const DispatcherProcessorParameters& rhs)
    {
        return lhs.queueSize() == rhs.queueSize() &&
               lhs.queueSizeLowWatermark() == rhs.queueSizeLowWatermark() &&
               lhs.queueSizeHighWatermark() == rhs.queueSizeHighWatermark();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const DispatcherProcessorParameters& lhs,
                           const DispatcherProcessorParameters& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream,
                                    const DispatcherProcessorParameters& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for
    /// `DispatcherProcessorParameters`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                    hashAlg,
                           const DispatcherProcessorParameters& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(
    mqbcfg::DispatcherProcessorParameters);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::DispatcherProcessorParameters>
: bsl::true_type {};

namespace mqbcfg {

// ===================
// class ElectorConfig
// ===================

/// Type representing the configuration for leader election amongst a cluster
/// of nodes.
/// initialWaitTimeoutMs.......: initial wait timeout, in milliseconds, of a
/// follower for leader heartbeat before initiating election, as per the *Raft*
/// Algorithm.  Note that `initialWaitTimeoutMs` should be larger than
/// `maxRandomWaitTimeoutMs` maxRandomWaitTimeoutMs.....: maximum random wait
/// timeout, in milliseconds, of a follower for leader heartbeat before
/// initiating election, as per the *Raft* Algorithm
/// scoutingResultTimeoutMs....: timeout, in milliseconds, of a follower for
/// awaiting scouting responses from all nodes after sending scouting request.
/// electionResultTimeoutMs....: timeout, in milliseconds, of a candidate for
/// awaiting quorum to be reached after proposing election, as per the *Raft*
/// Algorithm heartbeatBroadcastPeriodMs.: frequency, in milliseconds, in which
/// the leader broadcasts a heartbeat signal, as per the *Raft* Algorithm,
/// heartbeatCheckPeriodMs.....: frequency, in milliseconds, in which a
/// follower checks for heartbeat signals from the leader, as per the *Raft*
/// Algorithm heartbeatMissCount.........: the number of missed heartbeat
/// signals required before a follower marks the current leader as inactive, as
/// per the *Raft* Algorithm quorum.....................: the minimum number of
/// votes required for a candidate to transition to the leader.  If zero,
/// dynamically set to half the number of nodes plus one
/// leaderSyncDelayMs..........: delay, in milliseconds, after a leader has
/// been elected before initiating leader sync, in order to give a chance to
/// all nodes to come up and declare themselves AVAILABLE.  Note that this
/// should be done only in case of cluster of size > 1
class ElectorConfig {
    // INSTANCE DATA

    unsigned int d_quorum;
    int          d_initialWaitTimeoutMs;
    int          d_maxRandomWaitTimeoutMs;
    int          d_scoutingResultTimeoutMs;
    int          d_electionResultTimeoutMs;
    int          d_heartbeatBroadcastPeriodMs;
    int          d_heartbeatCheckPeriodMs;
    int          d_heartbeatMissCount;
    int          d_leaderSyncDelayMs;

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

    static const unsigned int DEFAULT_INITIALIZER_QUORUM;

    static const int DEFAULT_INITIALIZER_LEADER_SYNC_DELAY_MS;

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

    /// Create an object of type `ElectorConfig` having the default value.
    ElectorConfig();

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

    /// Return a reference to the modifiable "InitialWaitTimeoutMs" attribute
    /// of this object.
    int& initialWaitTimeoutMs();

    /// Return a reference to the modifiable "MaxRandomWaitTimeoutMs" attribute
    /// of this object.
    int& maxRandomWaitTimeoutMs();

    /// Return a reference to the modifiable "ScoutingResultTimeoutMs"
    /// attribute of this object.
    int& scoutingResultTimeoutMs();

    /// Return a reference to the modifiable "ElectionResultTimeoutMs"
    /// attribute of this object.
    int& electionResultTimeoutMs();

    /// Return a reference to the modifiable "HeartbeatBroadcastPeriodMs"
    /// attribute of this object.
    int& heartbeatBroadcastPeriodMs();

    /// Return a reference to the modifiable "HeartbeatCheckPeriodMs" attribute
    /// of this object.
    int& heartbeatCheckPeriodMs();

    /// Return a reference to the modifiable "HeartbeatMissCount" attribute of
    /// this object.
    int& heartbeatMissCount();

    /// Return a reference to the modifiable "Quorum" attribute of this object.
    unsigned int& quorum();

    /// Return a reference to the modifiable "LeaderSyncDelayMs" attribute of
    /// this object.
    int& leaderSyncDelayMs();

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

    /// Return the value of the "InitialWaitTimeoutMs" attribute of this
    /// object.
    int initialWaitTimeoutMs() const;

    /// Return the value of the "MaxRandomWaitTimeoutMs" attribute of this
    /// object.
    int maxRandomWaitTimeoutMs() const;

    /// Return the value of the "ScoutingResultTimeoutMs" attribute of this
    /// object.
    int scoutingResultTimeoutMs() const;

    /// Return the value of the "ElectionResultTimeoutMs" attribute of this
    /// object.
    int electionResultTimeoutMs() const;

    /// Return the value of the "HeartbeatBroadcastPeriodMs" attribute of this
    /// object.
    int heartbeatBroadcastPeriodMs() const;

    /// Return the value of the "HeartbeatCheckPeriodMs" attribute of this
    /// object.
    int heartbeatCheckPeriodMs() const;

    /// Return the value of the "HeartbeatMissCount" attribute of this object.
    int heartbeatMissCount() const;

    /// Return the value of the "Quorum" attribute of this object.
    unsigned int quorum() const;

    /// Return the value of the "LeaderSyncDelayMs" attribute of this object.
    int leaderSyncDelayMs() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ElectorConfig& lhs, const ElectorConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ElectorConfig& lhs, const ElectorConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const ElectorConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ElectorConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const ElectorConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::ElectorConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::ElectorConfig> : bsl::true_type {};

namespace mqbcfg {

// ================
// class ExportMode
// ================

struct ExportMode {
  public:
    // TYPES
    enum Value {
        e_E_PUSH = 0,
        e_E_PULL = 1,

        E_PUSH = e_E_PUSH,
        E_PULL = e_E_PULL
    };

    enum { k_NUM_ENUMERATORS = 2, NUM_ENUMERATORS = k_NUM_ENUMERATORS };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    /// Return the string representation exactly matching the enumerator
    /// name corresponding to the specified enumeration `value`.
    static const char* toString(Value value);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string` of the specified `stringLength`.  Return 0 on
    /// success, and a non-zero value with no effect on `result` otherwise
    /// (i.e., `string` does not match any enumerator).
    static int fromString(Value* result, const char* string, int stringLength);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `string` does not match any
    /// enumerator).
    static int fromString(Value* result, const bsl::string& string);

    /// Load into the specified `result` the enumerator matching the
    /// specified `number`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `number` does not match any
    /// enumerator).
    static int fromInt(Value* result, int number);

    /// Write to the specified `stream` the string representation of
    /// the specified enumeration `value`.  Return a reference to
    /// the modifiable `stream`.
    static bsl::ostream& print(bsl::ostream& stream, Value value);

    // HIDDEN FRIENDS
    /// Format the specified `rhs` to the specified output `stream` and
    /// return a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    {
        return ExportMode::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mqbcfg::ExportMode);

namespace mqbcfg {

// ===============
// class Heartbeat
// ===============

/// The following parameters define, for the various connection types, after
/// how many missed heartbeats the connection should be proactively resetted.
/// Note that a value of 0 means that smart-heartbeat is entirely disabled for
/// this kind of connection (i.e., it will not periodically emit heatbeats in
/// case no traffic is received, and will therefore not quickly detect stale
/// remote peer).  Each value is in multiple of the
/// 'NetworkInterfaces/TCPInterfaceConfig/heartIntervalMs'.
/// client............: The channel represents a client connected to the broker
/// downstreamBroker..: The channel represents a downstream broker connected to
/// this broker, i.e.  a proxy.  upstreamBroker....: The channel represents an
/// upstream broker connection from this broker, i.e.  a cluster proxy
/// connection.  clusterPeer.......: The channel represents a connection with a
/// peer node in the cluster this broker is part of.
class Heartbeat {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `Heartbeat` having the default value.
    Heartbeat();

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

    /// Return a reference to the modifiable "Client" attribute of this object.
    int& client();

    /// Return a reference to the modifiable "DownstreamBroker" attribute of
    /// this object.
    int& downstreamBroker();

    /// Return a reference to the modifiable "UpstreamBroker" attribute of this
    /// object.
    int& upstreamBroker();

    /// Return a reference to the modifiable "ClusterPeer" attribute of this
    /// object.
    int& clusterPeer();

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

    /// Return the value of the "Client" attribute of this object.
    int client() const;

    /// Return the value of the "DownstreamBroker" attribute of this object.
    int downstreamBroker() const;

    /// Return the value of the "UpstreamBroker" attribute of this object.
    int upstreamBroker() const;

    /// Return the value of the "ClusterPeer" attribute of this object.
    int clusterPeer() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const Heartbeat& lhs, const Heartbeat& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Heartbeat& lhs, const Heartbeat& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const Heartbeat& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Heartbeat`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Heartbeat& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::Heartbeat);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::Heartbeat> : bsl::true_type {};

namespace mqbcfg {

// ===================
// class LogDumpConfig
// ===================

class LogDumpConfig {
    // INSTANCE DATA

    bsl::string d_recordingLevel;
    bsl::string d_triggerLevel;
    int         d_recordBufferSize;

    // PRIVATE ACCESSORS

    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES

    enum {
        ATTRIBUTE_ID_RECORD_BUFFER_SIZE = 0,
        ATTRIBUTE_ID_RECORDING_LEVEL    = 1,
        ATTRIBUTE_ID_TRIGGER_LEVEL      = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_RECORD_BUFFER_SIZE = 0,
        ATTRIBUTE_INDEX_RECORDING_LEVEL    = 1,
        ATTRIBUTE_INDEX_TRIGGER_LEVEL      = 2
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_RECORD_BUFFER_SIZE;

    static const char DEFAULT_INITIALIZER_RECORDING_LEVEL[];

    static const char DEFAULT_INITIALIZER_TRIGGER_LEVEL[];

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

    /// Create an object of type `LogDumpConfig` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit LogDumpConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `LogDumpConfig` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    LogDumpConfig(const LogDumpConfig& original,
                  bslma::Allocator*    basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `LogDumpConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    LogDumpConfig(LogDumpConfig&& original) noexcept;

    /// Create an object of type `LogDumpConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    LogDumpConfig(LogDumpConfig&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~LogDumpConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    LogDumpConfig& operator=(const LogDumpConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    LogDumpConfig& operator=(LogDumpConfig&& rhs);
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

    /// Return a reference to the modifiable "RecordBufferSize" attribute of
    /// this object.
    int& recordBufferSize();

    /// Return a reference to the modifiable "RecordingLevel" attribute of this
    /// object.
    bsl::string& recordingLevel();

    /// Return a reference to the modifiable "TriggerLevel" attribute of this
    /// object.
    bsl::string& triggerLevel();

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

    /// Return the value of the "RecordBufferSize" attribute of this object.
    int recordBufferSize() const;

    /// Return a reference offering non-modifiable access to the
    /// "RecordingLevel" attribute of this object.
    const bsl::string& recordingLevel() const;

    /// Return a reference offering non-modifiable access to the "TriggerLevel"
    /// attribute of this object.
    const bsl::string& triggerLevel() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const LogDumpConfig& lhs, const LogDumpConfig& rhs)
    {
        return lhs.recordBufferSize() == rhs.recordBufferSize() &&
               lhs.recordingLevel() == rhs.recordingLevel() &&
               lhs.triggerLevel() == rhs.triggerLevel();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const LogDumpConfig& lhs, const LogDumpConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const LogDumpConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `LogDumpConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const LogDumpConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::LogDumpConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::LogDumpConfig> : bsl::true_type {};

namespace mqbcfg {

// ===============================
// class MasterAssignmentAlgorithm
// ===============================

/// Enumeration of the various algorithm's used for assigning a master to a
/// partition: - E_LEADER_IS_MASTER_ALL: the leader is master for all
/// partitions - E_LEAST_ASSIGNED:       the active node with the least number
/// of partitions assigned is used
struct MasterAssignmentAlgorithm {
  public:
    // TYPES
    enum Value {
        e_E_LEADER_IS_MASTER_ALL = 0,
        e_E_LEAST_ASSIGNED       = 1,

        E_LEADER_IS_MASTER_ALL = e_E_LEADER_IS_MASTER_ALL,
        E_LEAST_ASSIGNED       = e_E_LEAST_ASSIGNED
    };

    enum { k_NUM_ENUMERATORS = 2, NUM_ENUMERATORS = k_NUM_ENUMERATORS };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    /// Return the string representation exactly matching the enumerator
    /// name corresponding to the specified enumeration `value`.
    static const char* toString(Value value);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string` of the specified `stringLength`.  Return 0 on
    /// success, and a non-zero value with no effect on `result` otherwise
    /// (i.e., `string` does not match any enumerator).
    static int fromString(Value* result, const char* string, int stringLength);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `string` does not match any
    /// enumerator).
    static int fromString(Value* result, const bsl::string& string);

    /// Load into the specified `result` the enumerator matching the
    /// specified `number`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `number` does not match any
    /// enumerator).
    static int fromInt(Value* result, int number);

    /// Write to the specified `stream` the string representation of
    /// the specified enumeration `value`.  Return a reference to
    /// the modifiable `stream`.
    static bsl::ostream& print(bsl::ostream& stream, Value value);

    // HIDDEN FRIENDS
    /// Format the specified `rhs` to the specified output `stream` and
    /// return a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    {
        return MasterAssignmentAlgorithm::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mqbcfg::MasterAssignmentAlgorithm);

namespace mqbcfg {

// =========================
// class MessagePropertiesV2
// =========================

/// This complex type captures information which can be used to tell a broker
/// if it should advertise support for message properties v2 format (also
/// knownn as extended or 'EX' message properties at some places).
/// Additionally, broker can be configured to advertise this feature only to
/// those C++ and Java clients which match a certain minimum SDK version.
class MessagePropertiesV2 {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `MessagePropertiesV2` having the default
    /// value.
    MessagePropertiesV2();

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

    /// Return a reference to the modifiable "AdvertiseV2Support" attribute of
    /// this object.
    bool& advertiseV2Support();

    /// Return a reference to the modifiable "MinCppSdkVersion" attribute of
    /// this object.
    int& minCppSdkVersion();

    /// Return a reference to the modifiable "MinJavaSdkVersion" attribute of
    /// this object.
    int& minJavaSdkVersion();

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

    /// Return the value of the "AdvertiseV2Support" attribute of this object.
    bool advertiseV2Support() const;

    /// Return the value of the "MinCppSdkVersion" attribute of this object.
    int minCppSdkVersion() const;

    /// Return the value of the "MinJavaSdkVersion" attribute of this object.
    int minJavaSdkVersion() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const MessagePropertiesV2& lhs,
                           const MessagePropertiesV2& rhs)
    {
        return lhs.advertiseV2Support() == rhs.advertiseV2Support() &&
               lhs.minCppSdkVersion() == rhs.minCppSdkVersion() &&
               lhs.minJavaSdkVersion() == rhs.minJavaSdkVersion();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const MessagePropertiesV2& lhs,
                           const MessagePropertiesV2& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&              stream,
                                    const MessagePropertiesV2& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `MessagePropertiesV2`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&          hashAlg,
                           const MessagePropertiesV2& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::MessagePropertiesV2);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::MessagePropertiesV2>
: bsl::true_type {};

namespace mqbcfg {

// ===========================
// class MessageThrottleConfig
// ===========================

/// Configuration values for message throttling intervals and thresholds.
/// lowInterval...: time in milliseconds.
/// highInterval..: time in milliseconds.
/// lowThreshold..: indicates the rda counter value at which we start
/// throttlling for time equal to 'lowInterval'.
/// highThreshold.: indicates the rda counter value at which we start
/// throttlling for time equal to 'highInterval'.
/// Note: lowInterval should be less than/equal to highInterval, lowThreshold
/// should be less than highThreshold.
class MessageThrottleConfig {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `MessageThrottleConfig` having the default
    /// value.
    MessageThrottleConfig();

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

    /// Return a reference to the modifiable "LowThreshold" attribute of this
    /// object.
    unsigned int& lowThreshold();

    /// Return a reference to the modifiable "HighThreshold" attribute of this
    /// object.
    unsigned int& highThreshold();

    /// Return a reference to the modifiable "LowInterval" attribute of this
    /// object.
    unsigned int& lowInterval();

    /// Return a reference to the modifiable "HighInterval" attribute of this
    /// object.
    unsigned int& highInterval();

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

    /// Return the value of the "LowThreshold" attribute of this object.
    unsigned int lowThreshold() const;

    /// Return the value of the "HighThreshold" attribute of this object.
    unsigned int highThreshold() const;

    /// Return the value of the "LowInterval" attribute of this object.
    unsigned int lowInterval() const;

    /// Return the value of the "HighInterval" attribute of this object.
    unsigned int highInterval() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const MessageThrottleConfig& lhs,
                           const MessageThrottleConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const MessageThrottleConfig& lhs,
                           const MessageThrottleConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const MessageThrottleConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `MessageThrottleConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const MessageThrottleConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::MessageThrottleConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::MessageThrottleConfig>
: bsl::true_type {};

namespace mqbcfg {

// ========================
// class PluginSettingValue
// ========================

class PluginSettingValue {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<bool>               d_boolVal;
        bsls::ObjectBuffer<int>                d_intVal;
        bsls::ObjectBuffer<bsls::Types::Int64> d_longVal;
        bsls::ObjectBuffer<double>             d_doubleVal;
        bsls::ObjectBuffer<bsl::string>        d_stringVal;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const PluginSettingValue& rhs) const;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED  = -1,
        SELECTION_ID_BOOL_VAL   = 0,
        SELECTION_ID_INT_VAL    = 1,
        SELECTION_ID_LONG_VAL   = 2,
        SELECTION_ID_DOUBLE_VAL = 3,
        SELECTION_ID_STRING_VAL = 4
    };

    enum { NUM_SELECTIONS = 5 };

    enum {
        SELECTION_INDEX_BOOL_VAL   = 0,
        SELECTION_INDEX_INT_VAL    = 1,
        SELECTION_INDEX_LONG_VAL   = 2,
        SELECTION_INDEX_DOUBLE_VAL = 3,
        SELECTION_INDEX_STRING_VAL = 4
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

    /// Create an object of type `PluginSettingValue` having the default value.
    ///  Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit PluginSettingValue(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `PluginSettingValue` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    PluginSettingValue(const PluginSettingValue& original,
                       bslma::Allocator*         basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `PluginSettingValue` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    PluginSettingValue(PluginSettingValue&& original) noexcept;

    /// Create an object of type `PluginSettingValue` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    PluginSettingValue(PluginSettingValue&& original,
                       bslma::Allocator*    basicAllocator);
#endif

    /// Destroy this object.
    ~PluginSettingValue();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    PluginSettingValue& operator=(const PluginSettingValue& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    PluginSettingValue& operator=(PluginSettingValue&& rhs);
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

    /// Set the value of this object to be a `BoolVal` value.  Optionally
    /// specify the `value` of the `BoolVal`.  If `value` is not specified, the
    /// default `BoolVal` value is used.
    bool& makeBoolVal();
    bool& makeBoolVal(bool value);

    /// Set the value of this object to be a `IntVal` value.  Optionally
    /// specify the `value` of the `IntVal`.  If `value` is not specified, the
    /// default `IntVal` value is used.
    int& makeIntVal();
    int& makeIntVal(int value);

    /// Set the value of this object to be a `LongVal` value.  Optionally
    /// specify the `value` of the `LongVal`.  If `value` is not specified, the
    /// default `LongVal` value is used.
    bsls::Types::Int64& makeLongVal();
    bsls::Types::Int64& makeLongVal(bsls::Types::Int64 value);

    /// Set the value of this object to be a `DoubleVal` value.  Optionally
    /// specify the `value` of the `DoubleVal`.  If `value` is not specified,
    /// the default `DoubleVal` value is used.
    double& makeDoubleVal();
    double& makeDoubleVal(double value);

    /// Set the value of this object to be a `StringVal` value.  Optionally
    /// specify the `value` of the `StringVal`.  If `value` is not specified,
    /// the default `StringVal` value is used.
    bsl::string& makeStringVal();
    bsl::string& makeStringVal(const bsl::string& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    bsl::string& makeStringVal(bsl::string&& value);
#endif

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable `BoolVal` selection of this object
    /// if `BoolVal` is the current selection.  The behavior is undefined
    /// unless `BoolVal` is the selection of this object.
    bool& boolVal();

    /// Return a reference to the modifiable `IntVal` selection of this object
    /// if `IntVal` is the current selection.  The behavior is undefined unless
    /// `IntVal` is the selection of this object.
    int& intVal();

    /// Return a reference to the modifiable `LongVal` selection of this object
    /// if `LongVal` is the current selection.  The behavior is undefined
    /// unless `LongVal` is the selection of this object.
    bsls::Types::Int64& longVal();

    /// Return a reference to the modifiable `DoubleVal` selection of this
    /// object if `DoubleVal` is the current selection.  The behavior is
    /// undefined unless `DoubleVal` is the selection of this object.
    double& doubleVal();

    /// Return a reference to the modifiable `StringVal` selection of this
    /// object if `StringVal` is the current selection.  The behavior is
    /// undefined unless `StringVal` is the selection of this object.
    bsl::string& stringVal();

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

    /// Return a reference to the non-modifiable `BoolVal` selection of this
    /// object if `BoolVal` is the current selection.  The behavior is
    /// undefined unless `BoolVal` is the selection of this object.
    const bool& boolVal() const;

    /// Return a reference to the non-modifiable `IntVal` selection of this
    /// object if `IntVal` is the current selection.  The behavior is undefined
    /// unless `IntVal` is the selection of this object.
    const int& intVal() const;

    /// Return a reference to the non-modifiable `LongVal` selection of this
    /// object if `LongVal` is the current selection.  The behavior is
    /// undefined unless `LongVal` is the selection of this object.
    const bsls::Types::Int64& longVal() const;

    /// Return a reference to the non-modifiable `DoubleVal` selection of this
    /// object if `DoubleVal` is the current selection.  The behavior is
    /// undefined unless `DoubleVal` is the selection of this object.
    const double& doubleVal() const;

    /// Return a reference to the non-modifiable `StringVal` selection of this
    /// object if `StringVal` is the current selection.  The behavior is
    /// undefined unless `StringVal` is the selection of this object.
    const bsl::string& stringVal() const;

    /// Return `true` if the value of this object is a `BoolVal` value, and
    /// return `false` otherwise.
    bool isBoolValValue() const;

    /// Return `true` if the value of this object is a `IntVal` value, and
    /// return `false` otherwise.
    bool isIntValValue() const;

    /// Return `true` if the value of this object is a `LongVal` value, and
    /// return `false` otherwise.
    bool isLongValValue() const;

    /// Return `true` if the value of this object is a `DoubleVal` value, and
    /// return `false` otherwise.
    bool isDoubleValValue() const;

    /// Return `true` if the value of this object is a `StringVal` value, and
    /// return `false` otherwise.
    bool isStringValValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` objects have the same
    /// value, and `false` otherwise.  Two `PluginSettingValue` objects have
    /// the same value if either the selections in both objects have the same
    /// ids and the same values, or both selections are undefined.
    friend bool operator==(const PluginSettingValue& lhs,
                           const PluginSettingValue& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const PluginSettingValue& lhs,
                           const PluginSettingValue& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const PluginSettingValue& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `PluginSettingValue`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const PluginSettingValue& object)
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::PluginSettingValue);

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `Plugins` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Plugins(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Plugins` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    Plugins(const Plugins& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Plugins` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    Plugins(Plugins&& original) noexcept;

    /// Create an object of type `Plugins` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    Plugins(Plugins&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Plugins();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Plugins& operator=(const Plugins& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    Plugins& operator=(Plugins&& rhs);
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

    /// Return a reference to the modifiable "Libraries" attribute of this
    /// object.
    bsl::vector<bsl::string>& libraries();

    /// Return a reference to the modifiable "Enabled" attribute of this
    /// object.
    bsl::vector<bsl::string>& enabled();

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

    /// Return a reference offering non-modifiable access to the "Libraries"
    /// attribute of this object.
    const bsl::vector<bsl::string>& libraries() const;

    /// Return a reference offering non-modifiable access to the "Enabled"
    /// attribute of this object.
    const bsl::vector<bsl::string>& enabled() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const Plugins& lhs, const Plugins& rhs)
    {
        return lhs.libraries() == rhs.libraries() &&
               lhs.enabled() == rhs.enabled();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Plugins& lhs, const Plugins& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const Plugins& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Plugins`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Plugins& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.libraries());
        hashAppend(hashAlg, object.enabled());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::Plugins);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::Plugins> : bsl::true_type {};

namespace mqbcfg {

// ===========================
// class QueueOperationsConfig
// ===========================

/// Type representing the configuration for queue operations on a cluster.
/// openTimeoutMs..............: timeout, in milliseconds, to use when opening
/// a queue.  An open request requires some co-ordination among the nodes
/// (queue assignment request/response, followed by queue open
/// request/response, etc) configureTimeoutMs.........: timeout, in
/// milliseconds, to use for a configure queue request.  Note that
/// `configureTimeoutMs` must be less than or equal to `closeTimeoutMs` to
/// prevent out-of-order processing of closeQueue (e.g.  closeQueue sent after
/// configureQueue but timeout response processed first for the closeQueue)
/// closeTimeoutMs.............: timeout, in milliseconds, to use for a close
/// queue request reopenTimeoutMs............: timeout, in milliseconds, to use
/// when sending a reopen-queue request.  Ideally, we should use same value as
/// `openTimeoutMs`, but we are using a very large value as a workaround:
/// during network outages, a proxy or a replica may failover to a new upstream
/// node, which itself may be out of sync or not yet ready.  Eventually, the
/// reopen-queue requests sent during failover may timeout, and will never be
/// retried, leading to a 'permanent' failure (client consumer app stops
/// receiving messages; PUT messages from client producer app starts getting
/// NAK'd or buffered).  Using such a large timeout value helps in a situation
/// when network outages or its after-effects are fixed after a few hours).
/// reopenRetryIntervalMs......: duration, in milliseconds, after which a retry
/// attempt should be made to reopen the queue reopenMaxAttempts..........:
/// maximum number of attempts to reopen a queue when restoring the state in a
/// proxy upon getting a new active node notification
/// assignmentTimeoutMs........: timeout, in milliseconds, to use for a queue
/// assignment request keepaliveDurationMs........: duration, in milliseconds,
/// to keep a queue alive after it has met the criteria for deletion
/// consumptionMonitorPeriodMs.: frequency, in milliseconds, in which the
/// consumption monitor checks queue consumption statistics on the cluster
/// stopTimeoutMs..............: timeout, in milliseconds, to use in
/// StopRequest between deconfiguring and closing each affected queue.  This is
/// primarily to give a chance for pending PUSH mesages to be CONFIRMed.
/// shutdownTimeoutMs..........: timeout, in milliseconds, to use when node
/// stops for shutdown or maintenance mode.  This timeout should be greater
/// than the 'stopTimeoutMs'.  This is to handle misbehaving downstream which
/// may not reply to stopRequest (otherwise, this timeout is not expected to be
/// reached).  ackWindowSize..............: number of PUTs without ACK
/// requested after which we request an ACK.  This is to remove pending
/// broadcast PUTs.
class QueueOperationsConfig {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `QueueOperationsConfig` having the default
    /// value.
    QueueOperationsConfig();

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

    /// Return a reference to the modifiable "OpenTimeoutMs" attribute of this
    /// object.
    int& openTimeoutMs();

    /// Return a reference to the modifiable "ConfigureTimeoutMs" attribute of
    /// this object.
    int& configureTimeoutMs();

    /// Return a reference to the modifiable "CloseTimeoutMs" attribute of this
    /// object.
    int& closeTimeoutMs();

    /// Return a reference to the modifiable "ReopenTimeoutMs" attribute of
    /// this object.
    int& reopenTimeoutMs();

    /// Return a reference to the modifiable "ReopenRetryIntervalMs" attribute
    /// of this object.
    int& reopenRetryIntervalMs();

    /// Return a reference to the modifiable "ReopenMaxAttempts" attribute of
    /// this object.
    int& reopenMaxAttempts();

    /// Return a reference to the modifiable "AssignmentTimeoutMs" attribute of
    /// this object.
    int& assignmentTimeoutMs();

    /// Return a reference to the modifiable "KeepaliveDurationMs" attribute of
    /// this object.
    int& keepaliveDurationMs();

    /// Return a reference to the modifiable "ConsumptionMonitorPeriodMs"
    /// attribute of this object.
    int& consumptionMonitorPeriodMs();

    /// Return a reference to the modifiable "StopTimeoutMs" attribute of this
    /// object.
    int& stopTimeoutMs();

    /// Return a reference to the modifiable "ShutdownTimeoutMs" attribute of
    /// this object.
    int& shutdownTimeoutMs();

    /// Return a reference to the modifiable "AckWindowSize" attribute of this
    /// object.
    int& ackWindowSize();

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

    /// Return the value of the "OpenTimeoutMs" attribute of this object.
    int openTimeoutMs() const;

    /// Return the value of the "ConfigureTimeoutMs" attribute of this object.
    int configureTimeoutMs() const;

    /// Return the value of the "CloseTimeoutMs" attribute of this object.
    int closeTimeoutMs() const;

    /// Return the value of the "ReopenTimeoutMs" attribute of this object.
    int reopenTimeoutMs() const;

    /// Return the value of the "ReopenRetryIntervalMs" attribute of this
    /// object.
    int reopenRetryIntervalMs() const;

    /// Return the value of the "ReopenMaxAttempts" attribute of this object.
    int reopenMaxAttempts() const;

    /// Return the value of the "AssignmentTimeoutMs" attribute of this object.
    int assignmentTimeoutMs() const;

    /// Return the value of the "KeepaliveDurationMs" attribute of this object.
    int keepaliveDurationMs() const;

    /// Return the value of the "ConsumptionMonitorPeriodMs" attribute of this
    /// object.
    int consumptionMonitorPeriodMs() const;

    /// Return the value of the "StopTimeoutMs" attribute of this object.
    int stopTimeoutMs() const;

    /// Return the value of the "ShutdownTimeoutMs" attribute of this object.
    int shutdownTimeoutMs() const;

    /// Return the value of the "AckWindowSize" attribute of this object.
    int ackWindowSize() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const QueueOperationsConfig& lhs,
                           const QueueOperationsConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const QueueOperationsConfig& lhs,
                           const QueueOperationsConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const QueueOperationsConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `QueueOperationsConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const QueueOperationsConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::QueueOperationsConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::QueueOperationsConfig>
: bsl::true_type {};

namespace mqbcfg {

// ====================
// class ResolvedDomain
// ====================

/// Top level type representing the information retrieved when resolving a
/// domain.
/// resolvedName.: Resolved name of the domain clusterName..: Name of the
/// cluster where this domain exists
class ResolvedDomain {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `ResolvedDomain` having the default value.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit ResolvedDomain(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `ResolvedDomain` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    ResolvedDomain(const ResolvedDomain& original,
                   bslma::Allocator*     basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `ResolvedDomain` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    ResolvedDomain(ResolvedDomain&& original) noexcept;

    /// Create an object of type `ResolvedDomain` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    ResolvedDomain(ResolvedDomain&&  original,
                   bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~ResolvedDomain();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ResolvedDomain& operator=(const ResolvedDomain& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    ResolvedDomain& operator=(ResolvedDomain&& rhs);
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

    /// Return a reference to the modifiable "ResolvedName" attribute of this
    /// object.
    bsl::string& resolvedName();

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

    /// Return a reference offering non-modifiable access to the "ResolvedName"
    /// attribute of this object.
    const bsl::string& resolvedName() const;

    /// Return a reference offering non-modifiable access to the "ClusterName"
    /// attribute of this object.
    const bsl::string& clusterName() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ResolvedDomain& lhs,
                           const ResolvedDomain& rhs)
    {
        return lhs.resolvedName() == rhs.resolvedName() &&
               lhs.clusterName() == rhs.clusterName();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ResolvedDomain& lhs,
                           const ResolvedDomain& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&         stream,
                                    const ResolvedDomain& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ResolvedDomain`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&     hashAlg,
                           const ResolvedDomain& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.resolvedName());
        hashAppend(hashAlg, object.clusterName());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ResolvedDomain);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::ResolvedDomain> : bsl::true_type {};

namespace mqbcfg {

// ================================
// class StatsPrinterEncodingFormat
// ================================

/// Bitmask of stats output encoding formats: - NONE:           no stats output
/// (0) - TABLE:          human-readable table format (1) - JSON:
/// machine-readable JSON format (2) - TABLE_AND_JSON: both formats (TABLE |
/// JSON = 3)
struct StatsPrinterEncodingFormat {
  public:
    // TYPES
    enum Value {
        e_NONE           = 0,
        e_TABLE          = 1,
        e_JSON           = 2,
        e_TABLE_AND_JSON = 3,

        NONE           = e_NONE,
        TABLE          = e_TABLE,
        JSON           = e_JSON,
        TABLE_AND_JSON = e_TABLE_AND_JSON
    };

    enum { k_NUM_ENUMERATORS = 4, NUM_ENUMERATORS = k_NUM_ENUMERATORS };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    /// Return the string representation exactly matching the enumerator
    /// name corresponding to the specified enumeration `value`.
    static const char* toString(Value value);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string` of the specified `stringLength`.  Return 0 on
    /// success, and a non-zero value with no effect on `result` otherwise
    /// (i.e., `string` does not match any enumerator).
    static int fromString(Value* result, const char* string, int stringLength);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `string` does not match any
    /// enumerator).
    static int fromString(Value* result, const bsl::string& string);

    /// Load into the specified `result` the enumerator matching the
    /// specified `number`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `number` does not match any
    /// enumerator).
    static int fromInt(Value* result, int number);

    /// Write to the specified `stream` the string representation of
    /// the specified enumeration `value`.  Return a reference to
    /// the modifiable `stream`.
    static bsl::ostream& print(bsl::ostream& stream, Value value);

    // HIDDEN FRIENDS
    /// Format the specified `rhs` to the specified output `stream` and
    /// return a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    {
        return StatsPrinterEncodingFormat::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mqbcfg::StatsPrinterEncodingFormat);

namespace mqbcfg {

// =======================
// class StorageSyncConfig
// =======================

/// Type representing the configuration for storage synchronization and
/// recovery of a cluster.
/// startupRecoveryMaxDurationMs...: maximum amount of time, in milliseconds,
/// in which recovery for a partition at node startup must complete.  This
/// interval captures the time taken to receive storage-sync response, retry
/// attempts for failed storage-sync request, as well as the time taken by the
/// peer node to send partition files to the starting (i.e.  requester) node
/// maxAttemptsStorageSync.........: maximum number of attempts that a node
/// makes for storage-sync requests when it comes up (this value includes the
/// 1st attempt) storageSyncReqTimeoutMs........: timeout, in milliseconds, for
/// the storage-sync request.  A bigger value is recommended because peer node
/// could be busy serving storage-sync request for other partition(s) assigned
/// to the same partition-dispatcher thread, etc.  This timeout does *not*
/// capture the time taken by the peer to send partition files
/// masterSyncMaxDurationMs........: maximum amount of time, in milliseconds,
/// in which master sync for a partition must complete.  This interval includes
/// the time taken by replica node (the one with advanced view of the
/// partition) to send the file chunks, as well as time taken to receive
/// partition-sync state and data responses partitionSyncStateReqTimeoutMs.:
/// timeout, in milliseconds, for partition-sync-state-query request.  This
/// request is sent by a new master node to all replica nodes to query their
/// view of the partition partitionSyncDataReqTimeoutMs..: timeout, in
/// milliseconds, for partition-sync-data-query request.  This request is sent
/// by a new master node to a replica node (which has an advanced view of the
/// partition) to initiate partition sync.  This duration does *not* capture
/// the amount of time which replica might take to send the partition file
/// startupWaitDurationMs..........: duration, in milliseconds, for which
/// recovery manager waits for a sync point for a partition.  If no sync point
/// is received from a peer for a partition during this time, it is assumed
/// that there is no master for that partition, and recovery manager randomly
/// picks up a node from all the available ones with send a sync request.  If
/// no peers are available at this time, it is assumed that entire cluster is
/// coming up together, and proceeds with local recovery for that partition.
/// Note that this value should be less than the duration for which a node
/// waits if its elected a leader and there are no AVAILABLE nodes.  This is
/// important so that if all nodes in the cluster are starting, they have a
/// chance to wait for 'startupWaitDurationMs' milliseconds, find out that none
/// of the partitions have any master, go ahead with local recovery and declare
/// themselves as AVAILABLE.  This will give the new leader node a chance to
/// make each node a master for a given partition.  Moreover, this value should
/// be greater than the duration for which a peer waits before attempting to
/// reconnect to the node in the cluster, so that peer has a chance to connect
/// to this node, get notified (via ClusterObserver), and send sync point if
/// its a master for any partition fileChunkSize..................: chunk size,
/// in bytes, to send in one go to the peer when serving a storage sync request
/// from it partitionSyncEventSize.........: maximum size, in bytes, of
/// bmqp::EventType::PARTITION_SYNC before we send it to the peer
class StorageSyncConfig {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `StorageSyncConfig` having the default value.
    StorageSyncConfig();

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

    /// Return a reference to the modifiable "StartupRecoveryMaxDurationMs"
    /// attribute of this object.
    int& startupRecoveryMaxDurationMs();

    /// Return a reference to the modifiable "MaxAttemptsStorageSync" attribute
    /// of this object.
    int& maxAttemptsStorageSync();

    /// Return a reference to the modifiable "StorageSyncReqTimeoutMs"
    /// attribute of this object.
    int& storageSyncReqTimeoutMs();

    /// Return a reference to the modifiable "MasterSyncMaxDurationMs"
    /// attribute of this object.
    int& masterSyncMaxDurationMs();

    /// Return a reference to the modifiable "PartitionSyncStateReqTimeoutMs"
    /// attribute of this object.
    int& partitionSyncStateReqTimeoutMs();

    /// Return a reference to the modifiable "PartitionSyncDataReqTimeoutMs"
    /// attribute of this object.
    int& partitionSyncDataReqTimeoutMs();

    /// Return a reference to the modifiable "StartupWaitDurationMs" attribute
    /// of this object.
    int& startupWaitDurationMs();

    /// Return a reference to the modifiable "FileChunkSize" attribute of this
    /// object.
    int& fileChunkSize();

    /// Return a reference to the modifiable "PartitionSyncEventSize" attribute
    /// of this object.
    int& partitionSyncEventSize();

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

    /// Return the value of the "StartupRecoveryMaxDurationMs" attribute of
    /// this object.
    int startupRecoveryMaxDurationMs() const;

    /// Return the value of the "MaxAttemptsStorageSync" attribute of this
    /// object.
    int maxAttemptsStorageSync() const;

    /// Return the value of the "StorageSyncReqTimeoutMs" attribute of this
    /// object.
    int storageSyncReqTimeoutMs() const;

    /// Return the value of the "MasterSyncMaxDurationMs" attribute of this
    /// object.
    int masterSyncMaxDurationMs() const;

    /// Return the value of the "PartitionSyncStateReqTimeoutMs" attribute of
    /// this object.
    int partitionSyncStateReqTimeoutMs() const;

    /// Return the value of the "PartitionSyncDataReqTimeoutMs" attribute of
    /// this object.
    int partitionSyncDataReqTimeoutMs() const;

    /// Return the value of the "StartupWaitDurationMs" attribute of this
    /// object.
    int startupWaitDurationMs() const;

    /// Return the value of the "FileChunkSize" attribute of this object.
    int fileChunkSize() const;

    /// Return the value of the "PartitionSyncEventSize" attribute of this
    /// object.
    int partitionSyncEventSize() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const StorageSyncConfig& lhs,
                           const StorageSyncConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const StorageSyncConfig& lhs,
                           const StorageSyncConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const StorageSyncConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `StorageSyncConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const StorageSyncConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::StorageSyncConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::StorageSyncConfig> : bsl::true_type {
};

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `SyslogConfig` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit SyslogConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `SyslogConfig` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    SyslogConfig(const SyslogConfig& original,
                 bslma::Allocator*   basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `SyslogConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    SyslogConfig(SyslogConfig&& original) noexcept;

    /// Create an object of type `SyslogConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    SyslogConfig(SyslogConfig&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~SyslogConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    SyslogConfig& operator=(const SyslogConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    SyslogConfig& operator=(SyslogConfig&& rhs);
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

    /// Return a reference to the modifiable "Enabled" attribute of this
    /// object.
    bool& enabled();

    /// Return a reference to the modifiable "AppName" attribute of this
    /// object.
    bsl::string& appName();

    /// Return a reference to the modifiable "LogFormat" attribute of this
    /// object.
    bsl::string& logFormat();

    /// Return a reference to the modifiable "Verbosity" attribute of this
    /// object.
    bsl::string& verbosity();

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

    /// Return the value of the "Enabled" attribute of this object.
    bool enabled() const;

    /// Return a reference offering non-modifiable access to the "AppName"
    /// attribute of this object.
    const bsl::string& appName() const;

    /// Return a reference offering non-modifiable access to the "LogFormat"
    /// attribute of this object.
    const bsl::string& logFormat() const;

    /// Return a reference offering non-modifiable access to the "Verbosity"
    /// attribute of this object.
    const bsl::string& verbosity() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const SyslogConfig& lhs, const SyslogConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const SyslogConfig& lhs, const SyslogConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&       stream,
                                    const SyslogConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `SyslogConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&   hashAlg,
                           const SyslogConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::SyslogConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::SyslogConfig> : bsl::true_type {};

namespace mqbcfg {

// ==============================
// class TcpClusterNodeConnection
// ==============================

/// Configuration of a TCP based cluster node connectivity.
/// endpoint.: endpoint URI of the node address
class TcpClusterNodeConnection {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `TcpClusterNodeConnection` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit TcpClusterNodeConnection(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `TcpClusterNodeConnection` having the value of
    /// the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    TcpClusterNodeConnection(const TcpClusterNodeConnection& original,
                             bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `TcpClusterNodeConnection` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    TcpClusterNodeConnection(TcpClusterNodeConnection&& original) noexcept;

    /// Create an object of type `TcpClusterNodeConnection` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    TcpClusterNodeConnection(TcpClusterNodeConnection&& original,
                             bslma::Allocator*          basicAllocator);
#endif

    /// Destroy this object.
    ~TcpClusterNodeConnection();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    TcpClusterNodeConnection& operator=(const TcpClusterNodeConnection& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    TcpClusterNodeConnection& operator=(TcpClusterNodeConnection&& rhs);
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

    /// Return a reference to the modifiable "Endpoint" attribute of this
    /// object.
    bsl::string& endpoint();

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

    /// Return a reference offering non-modifiable access to the "Endpoint"
    /// attribute of this object.
    const bsl::string& endpoint() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const TcpClusterNodeConnection& lhs,
                           const TcpClusterNodeConnection& rhs)
    {
        return lhs.endpoint() == rhs.endpoint();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const TcpClusterNodeConnection& lhs,
                           const TcpClusterNodeConnection& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                   stream,
                                    const TcpClusterNodeConnection& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `TcpClusterNodeConnection`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&               hashAlg,
                           const TcpClusterNodeConnection& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.endpoint());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::TcpClusterNodeConnection);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::TcpClusterNodeConnection>
: bsl::true_type {};

namespace mqbcfg {

// ==========================
// class TcpInterfaceListener
// ==========================

/// This type describes the information needed for the broker to open a TCP
/// listener.
/// name.................: A name to associate this listener to.
/// address..............: The IPv4 address this listener will accept
/// connections on.  port.................: The port this listener will accept
/// connections on.  tls..................: Use TLS on this interface.
class TcpInterfaceListener {
    // INSTANCE DATA

    bsl::string d_name;
    bsl::string d_address;
    int         d_port;
    bool        d_tls;

    // PRIVATE ACCESSORS

    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const TcpInterfaceListener& rhs) const;

  public:
    // TYPES

    enum {
        ATTRIBUTE_ID_NAME    = 0,
        ATTRIBUTE_ID_ADDRESS = 1,
        ATTRIBUTE_ID_PORT    = 2,
        ATTRIBUTE_ID_TLS     = 3
    };

    enum { NUM_ATTRIBUTES = 4 };

    enum {
        ATTRIBUTE_INDEX_NAME    = 0,
        ATTRIBUTE_INDEX_ADDRESS = 1,
        ATTRIBUTE_INDEX_PORT    = 2,
        ATTRIBUTE_INDEX_TLS     = 3
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_ADDRESS[];

    static const bool DEFAULT_INITIALIZER_TLS;

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

    /// Create an object of type `TcpInterfaceListener` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit TcpInterfaceListener(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `TcpInterfaceListener` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    TcpInterfaceListener(const TcpInterfaceListener& original,
                         bslma::Allocator*           basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `TcpInterfaceListener` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    TcpInterfaceListener(TcpInterfaceListener&& original) noexcept;

    /// Create an object of type `TcpInterfaceListener` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    TcpInterfaceListener(TcpInterfaceListener&& original,
                         bslma::Allocator*      basicAllocator);
#endif

    /// Destroy this object.
    ~TcpInterfaceListener();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    TcpInterfaceListener& operator=(const TcpInterfaceListener& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    TcpInterfaceListener& operator=(TcpInterfaceListener&& rhs);
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

    /// Return a reference to the modifiable "Address" attribute of this
    /// object.
    bsl::string& address();

    /// Return a reference to the modifiable "Port" attribute of this object.
    int& port();

    /// Return a reference to the modifiable "Tls" attribute of this object.
    bool& tls();

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

    /// Return a reference offering non-modifiable access to the "Address"
    /// attribute of this object.
    const bsl::string& address() const;

    /// Return the value of the "Port" attribute of this object.
    int port() const;

    /// Return the value of the "Tls" attribute of this object.
    bool tls() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const TcpInterfaceListener& lhs,
                           const TcpInterfaceListener& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const TcpInterfaceListener& lhs,
                           const TcpInterfaceListener& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&               stream,
                                    const TcpInterfaceListener& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `TcpInterfaceListener`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&           hashAlg,
                           const TcpInterfaceListener& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::TcpInterfaceListener);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::TcpInterfaceListener>
: bsl::true_type {};

namespace mqbcfg {

// ===============
// class TlsConfig
// ===============

/// certificateAuthority.: A path to the FILE, containing concatenation of
/// known certificates the server can use to reference as its certificate
/// store.  certificate..........: A path to the FILE, containing the
/// certificate the broker will use to identify itself to other clients.
/// key..................: A path to the FILE, containing the private key that
/// the broker uses to read the certificate.  versions.............: A string
/// with a comma-separated list of supported protocol versions.
class TlsConfig {
    // INSTANCE DATA

    bsl::string d_certificateAuthority;
    bsl::string d_certificate;
    bsl::string d_key;
    bsl::string d_versions;

    // PRIVATE ACCESSORS

    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const TlsConfig& rhs) const;

  public:
    // TYPES

    enum {
        ATTRIBUTE_ID_CERTIFICATE_AUTHORITY = 0,
        ATTRIBUTE_ID_CERTIFICATE           = 1,
        ATTRIBUTE_ID_KEY                   = 2,
        ATTRIBUTE_ID_VERSIONS              = 3
    };

    enum { NUM_ATTRIBUTES = 4 };

    enum {
        ATTRIBUTE_INDEX_CERTIFICATE_AUTHORITY = 0,
        ATTRIBUTE_INDEX_CERTIFICATE           = 1,
        ATTRIBUTE_INDEX_KEY                   = 2,
        ATTRIBUTE_INDEX_VERSIONS              = 3
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_VERSIONS[];

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

    /// Create an object of type `TlsConfig` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit TlsConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `TlsConfig` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    TlsConfig(const TlsConfig& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `TlsConfig` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    TlsConfig(TlsConfig&& original) noexcept;

    /// Create an object of type `TlsConfig` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    TlsConfig(TlsConfig&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~TlsConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    TlsConfig& operator=(const TlsConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    TlsConfig& operator=(TlsConfig&& rhs);
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

    /// Return a reference to the modifiable "CertificateAuthority" attribute
    /// of this object.
    bsl::string& certificateAuthority();

    /// Return a reference to the modifiable "Certificate" attribute of this
    /// object.
    bsl::string& certificate();

    /// Return a reference to the modifiable "Key" attribute of this object.
    bsl::string& key();

    /// Return a reference to the modifiable "Versions" attribute of this
    /// object.
    bsl::string& versions();

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
    /// "CertificateAuthority" attribute of this object.
    const bsl::string& certificateAuthority() const;

    /// Return a reference offering non-modifiable access to the "Certificate"
    /// attribute of this object.
    const bsl::string& certificate() const;

    /// Return a reference offering non-modifiable access to the "Key"
    /// attribute of this object.
    const bsl::string& key() const;

    /// Return a reference offering non-modifiable access to the "Versions"
    /// attribute of this object.
    const bsl::string& versions() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const TlsConfig& lhs, const TlsConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const TlsConfig& lhs, const TlsConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const TlsConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `TlsConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const TlsConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::TlsConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::TlsConfig> : bsl::true_type {};

namespace mqbcfg {

// ===============================
// class VirtualClusterInformation
// ===============================

/// Type representing the information about the current node with regards to
/// virtual cluster.
/// name.............: name of the cluster selfNodeId.......: id of the current
/// node in that virtual cluster
class VirtualClusterInformation {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `VirtualClusterInformation` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit VirtualClusterInformation(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `VirtualClusterInformation` having the value
    /// of the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    VirtualClusterInformation(const VirtualClusterInformation& original,
                              bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `VirtualClusterInformation` having the value
    /// of the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    VirtualClusterInformation(VirtualClusterInformation&& original) noexcept;

    /// Create an object of type `VirtualClusterInformation` having the value
    /// of the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    VirtualClusterInformation(VirtualClusterInformation&& original,
                              bslma::Allocator*           basicAllocator);
#endif

    /// Destroy this object.
    ~VirtualClusterInformation();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    VirtualClusterInformation& operator=(const VirtualClusterInformation& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    VirtualClusterInformation& operator=(VirtualClusterInformation&& rhs);
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

    /// Return a reference to the modifiable "SelfNodeId" attribute of this
    /// object.
    int& selfNodeId();

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

    /// Return the value of the "SelfNodeId" attribute of this object.
    int selfNodeId() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const VirtualClusterInformation& lhs,
                           const VirtualClusterInformation& rhs)
    {
        return lhs.name() == rhs.name() &&
               lhs.selfNodeId() == rhs.selfNodeId();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const VirtualClusterInformation& lhs,
                           const VirtualClusterInformation& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                    stream,
                                    const VirtualClusterInformation& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `VirtualClusterInformation`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                hashAlg,
                           const VirtualClusterInformation& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.name());
        hashAppend(hashAlg, object.selfNodeId());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::VirtualClusterInformation);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::VirtualClusterInformation>
: bsl::true_type {};

namespace mqbcfg {

// =========================
// class AnonymousCredential
// =========================

/// Type representing the anonymous credential configuration.
/// disallow...: If set, the anonymous credential is not allowed.
/// Authentication is required and clients which cannot or do not authenticate
/// will be rejected.  credential.: If set, the credential is used for
/// anonymous authentication in case the client does not support authentication
/// or has not been configured to authenticate.
class AnonymousCredential {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<Disallow>   d_disallow;
        bsls::ObjectBuffer<Credential> d_credential;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const AnonymousCredential& rhs) const;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED  = -1,
        SELECTION_ID_DISALLOW   = 0,
        SELECTION_ID_CREDENTIAL = 1
    };

    enum { NUM_SELECTIONS = 2 };

    enum { SELECTION_INDEX_DISALLOW = 0, SELECTION_INDEX_CREDENTIAL = 1 };

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

    /// Create an object of type `AnonymousCredential` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit AnonymousCredential(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `AnonymousCredential` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    AnonymousCredential(const AnonymousCredential& original,
                        bslma::Allocator*          basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `AnonymousCredential` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    AnonymousCredential(AnonymousCredential&& original) noexcept;

    /// Create an object of type `AnonymousCredential` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    AnonymousCredential(AnonymousCredential&& original,
                        bslma::Allocator*     basicAllocator);
#endif

    /// Destroy this object.
    ~AnonymousCredential();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    AnonymousCredential& operator=(const AnonymousCredential& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    AnonymousCredential& operator=(AnonymousCredential&& rhs);
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

    /// Set the value of this object to be a `Disallow` value.  Optionally
    /// specify the `value` of the `Disallow`.  If `value` is not specified,
    /// the default `Disallow` value is used.
    Disallow& makeDisallow();
    Disallow& makeDisallow(const Disallow& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Disallow& makeDisallow(Disallow&& value);
#endif

    /// Set the value of this object to be a `Credential` value.  Optionally
    /// specify the `value` of the `Credential`.  If `value` is not specified,
    /// the default `Credential` value is used.
    Credential& makeCredential();
    Credential& makeCredential(const Credential& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Credential& makeCredential(Credential&& value);
#endif

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable `Disallow` selection of this
    /// object if `Disallow` is the current selection.  The behavior is
    /// undefined unless `Disallow` is the selection of this object.
    Disallow& disallow();

    /// Return a reference to the modifiable `Credential` selection of this
    /// object if `Credential` is the current selection.  The behavior is
    /// undefined unless `Credential` is the selection of this object.
    Credential& credential();

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

    /// Return a reference to the non-modifiable `Disallow` selection of this
    /// object if `Disallow` is the current selection.  The behavior is
    /// undefined unless `Disallow` is the selection of this object.
    const Disallow& disallow() const;

    /// Return a reference to the non-modifiable `Credential` selection of this
    /// object if `Credential` is the current selection.  The behavior is
    /// undefined unless `Credential` is the selection of this object.
    const Credential& credential() const;

    /// Return `true` if the value of this object is a `Disallow` value, and
    /// return `false` otherwise.
    bool isDisallowValue() const;

    /// Return `true` if the value of this object is a `Credential` value, and
    /// return `false` otherwise.
    bool isCredentialValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` objects have the same
    /// value, and `false` otherwise.  Two `AnonymousCredential` objects have
    /// the same value if either the selections in both objects have the same
    /// ids and the same values, or both selections are undefined.
    friend bool operator==(const AnonymousCredential& lhs,
                           const AnonymousCredential& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const AnonymousCredential& lhs,
                           const AnonymousCredential& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&              stream,
                                    const AnonymousCredential& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `AnonymousCredential`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&          hashAlg,
                           const AnonymousCredential& object)
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::AnonymousCredential);

namespace mqbcfg {

// ===========================
// class ClusterNodeConnection
// ===========================

/// Choice of all the various transport mode available to establish
/// connectivity with a node.
/// tcp.: TCP connectivity
class ClusterNodeConnection {
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

    /// Return selection information for the selection indicated by the
    /// specified `id` if the selection exists, and 0 otherwise.
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);

    /// Return selection information for the selection indicated by the
    /// specified `name` of the specified `nameLength` if the selection
    /// exists, and 0 otherwise.
    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `ClusterNodeConnection` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit ClusterNodeConnection(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `ClusterNodeConnection` having the value of
    /// the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    ClusterNodeConnection(const ClusterNodeConnection& original,
                          bslma::Allocator*            basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `ClusterNodeConnection` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    ClusterNodeConnection(ClusterNodeConnection&& original) noexcept;

    /// Create an object of type `ClusterNodeConnection` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    ClusterNodeConnection(ClusterNodeConnection&& original,
                          bslma::Allocator*       basicAllocator);
#endif

    /// Destroy this object.
    ~ClusterNodeConnection();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ClusterNodeConnection& operator=(const ClusterNodeConnection& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    ClusterNodeConnection& operator=(ClusterNodeConnection&& rhs);
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

    /// Set the value of this object to be a `Tcp` value.  Optionally specify
    /// the `value` of the `Tcp`.  If `value` is not specified, the default
    /// `Tcp` value is used.
    TcpClusterNodeConnection& makeTcp();
    TcpClusterNodeConnection& makeTcp(const TcpClusterNodeConnection& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    TcpClusterNodeConnection& makeTcp(TcpClusterNodeConnection&& value);
#endif

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable `Tcp` selection of this object if
    /// `Tcp` is the current selection.  The behavior is undefined unless `Tcp`
    /// is the selection of this object.
    TcpClusterNodeConnection& tcp();

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

    /// Return a reference to the non-modifiable `Tcp` selection of this object
    /// if `Tcp` is the current selection.  The behavior is undefined unless
    /// `Tcp` is the selection of this object.
    const TcpClusterNodeConnection& tcp() const;

    /// Return `true` if the value of this object is a `Tcp` value, and return
    /// `false` otherwise.
    bool isTcpValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` objects have the same
    /// value, and `false` otherwise.  Two `ClusterNodeConnection` objects have
    /// the same value if either the selections in both objects have the same
    /// ids and the same values, or both selections are undefined.
    friend bool operator==(const ClusterNodeConnection& lhs,
                           const ClusterNodeConnection& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ClusterNodeConnection& lhs,
                           const ClusterNodeConnection& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const ClusterNodeConnection& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ClusterNodeConnection`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const ClusterNodeConnection& object)
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ClusterNodeConnection);

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `DispatcherProcessorConfig` having the default
    /// value.
    DispatcherProcessorConfig();

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

    /// Return a reference to the modifiable "NumProcessors" attribute of this
    /// object.
    int& numProcessors();

    /// Return a reference to the modifiable "ProcessorConfig" attribute of
    /// this object.
    DispatcherProcessorParameters& processorConfig();

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

    /// Return the value of the "NumProcessors" attribute of this object.
    int numProcessors() const;

    /// Return a reference offering non-modifiable access to the
    /// "ProcessorConfig" attribute of this object.
    const DispatcherProcessorParameters& processorConfig() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const DispatcherProcessorConfig& lhs,
                           const DispatcherProcessorConfig& rhs)
    {
        return lhs.numProcessors() == rhs.numProcessors() &&
               lhs.processorConfig() == rhs.processorConfig();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const DispatcherProcessorConfig& lhs,
                           const DispatcherProcessorConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                    stream,
                                    const DispatcherProcessorConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `DispatcherProcessorConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                hashAlg,
                           const DispatcherProcessorConfig& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.numProcessors());
        hashAppend(hashAlg, object.processorConfig());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(
    mqbcfg::DispatcherProcessorConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::DispatcherProcessorConfig>
: bsl::true_type {};

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
    LogDumpConfig            d_logDump;
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
        ATTRIBUTE_ID_SYSLOG                      = 9,
        ATTRIBUTE_ID_LOG_DUMP                    = 10
    };

    enum { NUM_ATTRIBUTES = 11 };

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
        ATTRIBUTE_INDEX_SYSLOG                      = 9,
        ATTRIBUTE_INDEX_LOG_DUMP                    = 10
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_BSLS_LOG_SEVERITY_THRESHOLD[];

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

    /// Create an object of type `LogController` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit LogController(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `LogController` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    LogController(const LogController& original,
                  bslma::Allocator*    basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `LogController` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    LogController(LogController&& original) noexcept;

    /// Create an object of type `LogController` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    LogController(LogController&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~LogController();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    LogController& operator=(const LogController& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    LogController& operator=(LogController&& rhs);
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

    /// Return a reference to the modifiable "FileName" attribute of this
    /// object.
    bsl::string& fileName();

    /// Return a reference to the modifiable "FileMaxAgeDays" attribute of this
    /// object.
    int& fileMaxAgeDays();

    /// Return a reference to the modifiable "RotationBytes" attribute of this
    /// object.
    int& rotationBytes();

    /// Return a reference to the modifiable "LogfileFormat" attribute of this
    /// object.
    bsl::string& logfileFormat();

    /// Return a reference to the modifiable "ConsoleFormat" attribute of this
    /// object.
    bsl::string& consoleFormat();

    /// Return a reference to the modifiable "LoggingVerbosity" attribute of
    /// this object.
    bsl::string& loggingVerbosity();

    /// Return a reference to the modifiable "BslsLogSeverityThreshold"
    /// attribute of this object.
    bsl::string& bslsLogSeverityThreshold();

    /// Return a reference to the modifiable "ConsoleSeverityThreshold"
    /// attribute of this object.
    bsl::string& consoleSeverityThreshold();

    /// Return a reference to the modifiable "Categories" attribute of this
    /// object.
    bsl::vector<bsl::string>& categories();

    /// Return a reference to the modifiable "Syslog" attribute of this object.
    SyslogConfig& syslog();

    /// Return a reference to the modifiable "LogDump" attribute of this
    /// object.
    LogDumpConfig& logDump();

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

    /// Return a reference offering non-modifiable access to the "FileName"
    /// attribute of this object.
    const bsl::string& fileName() const;

    /// Return the value of the "FileMaxAgeDays" attribute of this object.
    int fileMaxAgeDays() const;

    /// Return the value of the "RotationBytes" attribute of this object.
    int rotationBytes() const;

    /// Return a reference offering non-modifiable access to the
    /// "LogfileFormat" attribute of this object.
    const bsl::string& logfileFormat() const;

    /// Return a reference offering non-modifiable access to the
    /// "ConsoleFormat" attribute of this object.
    const bsl::string& consoleFormat() const;

    /// Return a reference offering non-modifiable access to the
    /// "LoggingVerbosity" attribute of this object.
    const bsl::string& loggingVerbosity() const;

    /// Return a reference offering non-modifiable access to the
    /// "BslsLogSeverityThreshold" attribute of this object.
    const bsl::string& bslsLogSeverityThreshold() const;

    /// Return a reference offering non-modifiable access to the
    /// "ConsoleSeverityThreshold" attribute of this object.
    const bsl::string& consoleSeverityThreshold() const;

    /// Return a reference offering non-modifiable access to the "Categories"
    /// attribute of this object.
    const bsl::vector<bsl::string>& categories() const;

    /// Return a reference offering non-modifiable access to the "Syslog"
    /// attribute of this object.
    const SyslogConfig& syslog() const;

    /// Return a reference offering non-modifiable access to the "LogDump"
    /// attribute of this object.
    const LogDumpConfig& logDump() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const LogController& lhs, const LogController& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const LogController& lhs, const LogController& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const LogController& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `LogController`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const LogController& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::LogController);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::LogController> : bsl::true_type {};

namespace mqbcfg {

// =====================
// class PartitionConfig
// =====================

/// Type representing the configuration for the storage layer of a cluster.
/// numPartitions........: number of partitions at each node in the cluster
/// location.............: location of active files for a partition
/// archiveLocation......: location of archive files for a partition
/// maxDataFileSize......: maximum size of partitions' data file
/// maxJournalFileSize...: maximum size of partitions' journal file
/// maxQlistFileSize.....: maximum size of partitions' qlist file
/// maxCSLFileSize.......: maximum size of partitions' CSL file
/// preallocate..........: flag to indicate whether files should be
/// preallocated on disk maxArchivedFileSets..: maximum number of archived file
/// sets per partition to keep prefaultPages........: flag to indicate whether
/// to populate (prefault) page tables for a mapping.  flushAtShutdown......:
/// flag to indicate whether broker should flush storage files to disk at
/// shutdown syncConfig...........: configuration for storage synchronization
/// and recovery
class PartitionConfig {
    // INSTANCE DATA

    bsls::Types::Uint64 d_maxDataFileSize;
    bsls::Types::Uint64 d_maxJournalFileSize;
    bsls::Types::Uint64 d_maxQlistFileSize;
    bsls::Types::Uint64 d_maxCSLFileSize;
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
        ATTRIBUTE_ID_MAX_C_S_L_FILE_SIZE    = 6,
        ATTRIBUTE_ID_PREALLOCATE            = 7,
        ATTRIBUTE_ID_MAX_ARCHIVED_FILE_SETS = 8,
        ATTRIBUTE_ID_PREFAULT_PAGES         = 9,
        ATTRIBUTE_ID_FLUSH_AT_SHUTDOWN      = 10,
        ATTRIBUTE_ID_SYNC_CONFIG            = 11
    };

    enum { NUM_ATTRIBUTES = 12 };

    enum {
        ATTRIBUTE_INDEX_NUM_PARTITIONS         = 0,
        ATTRIBUTE_INDEX_LOCATION               = 1,
        ATTRIBUTE_INDEX_ARCHIVE_LOCATION       = 2,
        ATTRIBUTE_INDEX_MAX_DATA_FILE_SIZE     = 3,
        ATTRIBUTE_INDEX_MAX_JOURNAL_FILE_SIZE  = 4,
        ATTRIBUTE_INDEX_MAX_QLIST_FILE_SIZE    = 5,
        ATTRIBUTE_INDEX_MAX_C_S_L_FILE_SIZE    = 6,
        ATTRIBUTE_INDEX_PREALLOCATE            = 7,
        ATTRIBUTE_INDEX_MAX_ARCHIVED_FILE_SETS = 8,
        ATTRIBUTE_INDEX_PREFAULT_PAGES         = 9,
        ATTRIBUTE_INDEX_FLUSH_AT_SHUTDOWN      = 10,
        ATTRIBUTE_INDEX_SYNC_CONFIG            = 11
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const bsls::Types::Uint64 DEFAULT_INITIALIZER_MAX_C_S_L_FILE_SIZE;

    static const bool DEFAULT_INITIALIZER_PREALLOCATE;

    static const bool DEFAULT_INITIALIZER_PREFAULT_PAGES;

    static const bool DEFAULT_INITIALIZER_FLUSH_AT_SHUTDOWN;

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

    /// Create an object of type `PartitionConfig` having the default value.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit PartitionConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `PartitionConfig` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    PartitionConfig(const PartitionConfig& original,
                    bslma::Allocator*      basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `PartitionConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    PartitionConfig(PartitionConfig&& original) noexcept;

    /// Create an object of type `PartitionConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    PartitionConfig(PartitionConfig&& original,
                    bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~PartitionConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    PartitionConfig& operator=(const PartitionConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    PartitionConfig& operator=(PartitionConfig&& rhs);
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

    /// Return a reference to the modifiable "NumPartitions" attribute of this
    /// object.
    int& numPartitions();

    /// Return a reference to the modifiable "Location" attribute of this
    /// object.
    bsl::string& location();

    /// Return a reference to the modifiable "ArchiveLocation" attribute of
    /// this object.
    bsl::string& archiveLocation();

    /// Return a reference to the modifiable "MaxDataFileSize" attribute of
    /// this object.
    bsls::Types::Uint64& maxDataFileSize();

    /// Return a reference to the modifiable "MaxJournalFileSize" attribute of
    /// this object.
    bsls::Types::Uint64& maxJournalFileSize();

    /// Return a reference to the modifiable "MaxQlistFileSize" attribute of
    /// this object.
    bsls::Types::Uint64& maxQlistFileSize();

    /// Return a reference to the modifiable "MaxCSLFileSize" attribute of this
    /// object.
    bsls::Types::Uint64& maxCSLFileSize();

    /// Return a reference to the modifiable "Preallocate" attribute of this
    /// object.
    bool& preallocate();

    /// Return a reference to the modifiable "MaxArchivedFileSets" attribute of
    /// this object.
    int& maxArchivedFileSets();

    /// Return a reference to the modifiable "PrefaultPages" attribute of this
    /// object.
    bool& prefaultPages();

    /// Return a reference to the modifiable "FlushAtShutdown" attribute of
    /// this object.
    bool& flushAtShutdown();

    /// Return a reference to the modifiable "SyncConfig" attribute of this
    /// object.
    StorageSyncConfig& syncConfig();

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

    /// Return the value of the "NumPartitions" attribute of this object.
    int numPartitions() const;

    /// Return a reference offering non-modifiable access to the "Location"
    /// attribute of this object.
    const bsl::string& location() const;

    /// Return a reference offering non-modifiable access to the
    /// "ArchiveLocation" attribute of this object.
    const bsl::string& archiveLocation() const;

    /// Return the value of the "MaxDataFileSize" attribute of this object.
    bsls::Types::Uint64 maxDataFileSize() const;

    /// Return the value of the "MaxJournalFileSize" attribute of this object.
    bsls::Types::Uint64 maxJournalFileSize() const;

    /// Return the value of the "MaxQlistFileSize" attribute of this object.
    bsls::Types::Uint64 maxQlistFileSize() const;

    /// Return the value of the "MaxCSLFileSize" attribute of this object.
    bsls::Types::Uint64 maxCSLFileSize() const;

    /// Return the value of the "Preallocate" attribute of this object.
    bool preallocate() const;

    /// Return the value of the "MaxArchivedFileSets" attribute of this object.
    int maxArchivedFileSets() const;

    /// Return the value of the "PrefaultPages" attribute of this object.
    bool prefaultPages() const;

    /// Return the value of the "FlushAtShutdown" attribute of this object.
    bool flushAtShutdown() const;

    /// Return a reference offering non-modifiable access to the "SyncConfig"
    /// attribute of this object.
    const StorageSyncConfig& syncConfig() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const PartitionConfig& lhs,
                           const PartitionConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const PartitionConfig& lhs,
                           const PartitionConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&          stream,
                                    const PartitionConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `PartitionConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&      hashAlg,
                           const PartitionConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::PartitionConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::PartitionConfig> : bsl::true_type {};

namespace mqbcfg {

// ===========================
// class PluginSettingKeyValue
// ===========================

/// The key-value pair used for plugin settings.
/// key...: setting key/name value.: setting value
class PluginSettingKeyValue {
    // INSTANCE DATA

    bsl::string        d_key;
    PluginSettingValue d_value;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_KEY = 0, ATTRIBUTE_ID_VALUE = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_KEY = 0, ATTRIBUTE_INDEX_VALUE = 1 };

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

    /// Create an object of type `PluginSettingKeyValue` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit PluginSettingKeyValue(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `PluginSettingKeyValue` having the value of
    /// the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    PluginSettingKeyValue(const PluginSettingKeyValue& original,
                          bslma::Allocator*            basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `PluginSettingKeyValue` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    PluginSettingKeyValue(PluginSettingKeyValue&& original) noexcept;

    /// Create an object of type `PluginSettingKeyValue` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    PluginSettingKeyValue(PluginSettingKeyValue&& original,
                          bslma::Allocator*       basicAllocator);
#endif

    /// Destroy this object.
    ~PluginSettingKeyValue();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    PluginSettingKeyValue& operator=(const PluginSettingKeyValue& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    PluginSettingKeyValue& operator=(PluginSettingKeyValue&& rhs);
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

    /// Return a reference to the modifiable "Key" attribute of this object.
    bsl::string& key();

    /// Return a reference to the modifiable "Value" attribute of this object.
    PluginSettingValue& value();

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

    /// Return a reference offering non-modifiable access to the "Key"
    /// attribute of this object.
    const bsl::string& key() const;

    /// Return a reference offering non-modifiable access to the "Value"
    /// attribute of this object.
    const PluginSettingValue& value() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const PluginSettingKeyValue& lhs,
                           const PluginSettingKeyValue& rhs)
    {
        return lhs.key() == rhs.key() && lhs.value() == rhs.value();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const PluginSettingKeyValue& lhs,
                           const PluginSettingKeyValue& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const PluginSettingKeyValue& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `PluginSettingKeyValue`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const PluginSettingKeyValue& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.key());
        hashAppend(hashAlg, object.value());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::PluginSettingKeyValue);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::PluginSettingKeyValue>
: bsl::true_type {};

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `StatPluginConfigPrometheus` having the
    /// default value.  Use the optionally specified `basicAllocator` to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    explicit StatPluginConfigPrometheus(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatPluginConfigPrometheus` having the value
    /// of the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    StatPluginConfigPrometheus(const StatPluginConfigPrometheus& original,
                               bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatPluginConfigPrometheus` having the value
    /// of the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    StatPluginConfigPrometheus(StatPluginConfigPrometheus&& original) noexcept;

    /// Create an object of type `StatPluginConfigPrometheus` having the value
    /// of the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    StatPluginConfigPrometheus(StatPluginConfigPrometheus&& original,
                               bslma::Allocator*            basicAllocator);
#endif

    /// Destroy this object.
    ~StatPluginConfigPrometheus();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatPluginConfigPrometheus&
    operator=(const StatPluginConfigPrometheus& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    StatPluginConfigPrometheus& operator=(StatPluginConfigPrometheus&& rhs);
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

    /// Return a reference to the modifiable "Mode" attribute of this object.
    ExportMode::Value& mode();

    /// Return a reference to the modifiable "Host" attribute of this object.
    bsl::string& host();

    /// Return a reference to the modifiable "Port" attribute of this object.
    int& port();

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

    /// Return the value of the "Mode" attribute of this object.
    ExportMode::Value mode() const;

    /// Return a reference offering non-modifiable access to the "Host"
    /// attribute of this object.
    const bsl::string& host() const;

    /// Return the value of the "Port" attribute of this object.
    int port() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const StatPluginConfigPrometheus& lhs,
                           const StatPluginConfigPrometheus& rhs)
    {
        return lhs.mode() == rhs.mode() && lhs.host() == rhs.host() &&
               lhs.port() == rhs.port();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const StatPluginConfigPrometheus& lhs,
                           const StatPluginConfigPrometheus& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                     stream,
                                    const StatPluginConfigPrometheus& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `StatPluginConfigPrometheus`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                 hashAlg,
                           const StatPluginConfigPrometheus& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::StatPluginConfigPrometheus);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::StatPluginConfigPrometheus>
: bsl::true_type {};

namespace mqbcfg {

// ========================
// class StatsPrinterConfig
// ========================

class StatsPrinterConfig {
    // INSTANCE DATA

    bsl::string                       d_file;
    int                               d_printInterval;
    int                               d_maxAgeDays;
    int                               d_rotateBytes;
    int                               d_rotateDays;
    StatsPrinterEncodingFormat::Value d_encoding;

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
        ATTRIBUTE_ID_ROTATE_DAYS    = 4,
        ATTRIBUTE_ID_ENCODING       = 5
    };

    enum { NUM_ATTRIBUTES = 6 };

    enum {
        ATTRIBUTE_INDEX_PRINT_INTERVAL = 0,
        ATTRIBUTE_INDEX_FILE           = 1,
        ATTRIBUTE_INDEX_MAX_AGE_DAYS   = 2,
        ATTRIBUTE_INDEX_ROTATE_BYTES   = 3,
        ATTRIBUTE_INDEX_ROTATE_DAYS    = 4,
        ATTRIBUTE_INDEX_ENCODING       = 5
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_PRINT_INTERVAL;

    static const int DEFAULT_INITIALIZER_ROTATE_BYTES;

    static const int DEFAULT_INITIALIZER_ROTATE_DAYS;

    static const StatsPrinterEncodingFormat::Value
        DEFAULT_INITIALIZER_ENCODING;

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

    /// Create an object of type `StatsPrinterConfig` having the default value.
    ///  Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit StatsPrinterConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatsPrinterConfig` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    StatsPrinterConfig(const StatsPrinterConfig& original,
                       bslma::Allocator*         basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatsPrinterConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    StatsPrinterConfig(StatsPrinterConfig&& original) noexcept;

    /// Create an object of type `StatsPrinterConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    StatsPrinterConfig(StatsPrinterConfig&& original,
                       bslma::Allocator*    basicAllocator);
#endif

    /// Destroy this object.
    ~StatsPrinterConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatsPrinterConfig& operator=(const StatsPrinterConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    StatsPrinterConfig& operator=(StatsPrinterConfig&& rhs);
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

    /// Return a reference to the modifiable "PrintInterval" attribute of this
    /// object.
    int& printInterval();

    /// Return a reference to the modifiable "File" attribute of this object.
    bsl::string& file();

    /// Return a reference to the modifiable "MaxAgeDays" attribute of this
    /// object.
    int& maxAgeDays();

    /// Return a reference to the modifiable "RotateBytes" attribute of this
    /// object.
    int& rotateBytes();

    /// Return a reference to the modifiable "RotateDays" attribute of this
    /// object.
    int& rotateDays();

    /// Return a reference to the modifiable "Encoding" attribute of this
    /// object.
    StatsPrinterEncodingFormat::Value& encoding();

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

    /// Return the value of the "PrintInterval" attribute of this object.
    int printInterval() const;

    /// Return a reference offering non-modifiable access to the "File"
    /// attribute of this object.
    const bsl::string& file() const;

    /// Return the value of the "MaxAgeDays" attribute of this object.
    int maxAgeDays() const;

    /// Return the value of the "RotateBytes" attribute of this object.
    int rotateBytes() const;

    /// Return the value of the "RotateDays" attribute of this object.
    int rotateDays() const;

    /// Return the value of the "Encoding" attribute of this object.
    StatsPrinterEncodingFormat::Value encoding() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const StatsPrinterConfig& lhs,
                           const StatsPrinterConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const StatsPrinterConfig& lhs,
                           const StatsPrinterConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const StatsPrinterConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `StatsPrinterConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const StatsPrinterConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::StatsPrinterConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::StatsPrinterConfig>
: bsl::true_type {};

namespace mqbcfg {

// ========================
// class TcpInterfaceConfig
// ========================

/// name.................: The name of the TCP session manager.
/// port.................: (Deprecated) The port to receive connections.
/// lowWatermark.........: highWatermark........: Watermarks used for channels
/// with a client or proxy.  nodeLowWatermark.....: nodeHighWatermark....:
/// Reduced watermarks for communication between cluster nodes where BlazingMQ
/// maintains its own cache.  heartbeatIntervalMs..: How often (in
/// milliseconds) to check if the channel received data, and emit heartbeat.  0
/// to globally disable.  listeners: A list of listener interfaces to receive
/// TCP connections from.  When non-empty this option overrides the listener
/// specified by port.
class TcpInterfaceConfig {
    // INSTANCE DATA

    bsls::Types::Int64                d_lowWatermark;
    bsls::Types::Int64                d_highWatermark;
    bsls::Types::Int64                d_nodeLowWatermark;
    bsls::Types::Int64                d_nodeHighWatermark;
    bsl::vector<TcpInterfaceListener> d_listeners;
    bsl::string                       d_name;
    int                               d_port;
    int                               d_ioThreads;
    int                               d_maxConnections;
    int                               d_heartbeatIntervalMs;

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
        ATTRIBUTE_ID_HEARTBEAT_INTERVAL_MS = 8,
        ATTRIBUTE_ID_LISTENERS             = 9
    };

    enum { NUM_ATTRIBUTES = 10 };

    enum {
        ATTRIBUTE_INDEX_NAME                  = 0,
        ATTRIBUTE_INDEX_PORT                  = 1,
        ATTRIBUTE_INDEX_IO_THREADS            = 2,
        ATTRIBUTE_INDEX_MAX_CONNECTIONS       = 3,
        ATTRIBUTE_INDEX_LOW_WATERMARK         = 4,
        ATTRIBUTE_INDEX_HIGH_WATERMARK        = 5,
        ATTRIBUTE_INDEX_NODE_LOW_WATERMARK    = 6,
        ATTRIBUTE_INDEX_NODE_HIGH_WATERMARK   = 7,
        ATTRIBUTE_INDEX_HEARTBEAT_INTERVAL_MS = 8,
        ATTRIBUTE_INDEX_LISTENERS             = 9
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `TcpInterfaceConfig` having the default value.
    ///  Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit TcpInterfaceConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `TcpInterfaceConfig` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    TcpInterfaceConfig(const TcpInterfaceConfig& original,
                       bslma::Allocator*         basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `TcpInterfaceConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    TcpInterfaceConfig(TcpInterfaceConfig&& original) noexcept;

    /// Create an object of type `TcpInterfaceConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    TcpInterfaceConfig(TcpInterfaceConfig&& original,
                       bslma::Allocator*    basicAllocator);
#endif

    /// Destroy this object.
    ~TcpInterfaceConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    TcpInterfaceConfig& operator=(const TcpInterfaceConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    TcpInterfaceConfig& operator=(TcpInterfaceConfig&& rhs);
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

    /// Return a reference to the modifiable "Port" attribute of this object.
    int& port();

    /// Return a reference to the modifiable "IoThreads" attribute of this
    /// object.
    int& ioThreads();

    /// Return a reference to the modifiable "MaxConnections" attribute of this
    /// object.
    int& maxConnections();

    /// Return a reference to the modifiable "LowWatermark" attribute of this
    /// object.
    bsls::Types::Int64& lowWatermark();

    /// Return a reference to the modifiable "HighWatermark" attribute of this
    /// object.
    bsls::Types::Int64& highWatermark();

    /// Return a reference to the modifiable "NodeLowWatermark" attribute of
    /// this object.
    bsls::Types::Int64& nodeLowWatermark();

    /// Return a reference to the modifiable "NodeHighWatermark" attribute of
    /// this object.
    bsls::Types::Int64& nodeHighWatermark();

    /// Return a reference to the modifiable "HeartbeatIntervalMs" attribute of
    /// this object.
    int& heartbeatIntervalMs();

    /// Return a reference to the modifiable "Listeners" attribute of this
    /// object.
    bsl::vector<TcpInterfaceListener>& listeners();

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

    /// Return the value of the "Port" attribute of this object.
    int port() const;

    /// Return the value of the "IoThreads" attribute of this object.
    int ioThreads() const;

    /// Return the value of the "MaxConnections" attribute of this object.
    int maxConnections() const;

    /// Return the value of the "LowWatermark" attribute of this object.
    bsls::Types::Int64 lowWatermark() const;

    /// Return the value of the "HighWatermark" attribute of this object.
    bsls::Types::Int64 highWatermark() const;

    /// Return the value of the "NodeLowWatermark" attribute of this object.
    bsls::Types::Int64 nodeLowWatermark() const;

    /// Return the value of the "NodeHighWatermark" attribute of this object.
    bsls::Types::Int64 nodeHighWatermark() const;

    /// Return the value of the "HeartbeatIntervalMs" attribute of this object.
    int heartbeatIntervalMs() const;

    /// Return a reference offering non-modifiable access to the "Listeners"
    /// attribute of this object.
    const bsl::vector<TcpInterfaceListener>& listeners() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const TcpInterfaceConfig& lhs,
                           const TcpInterfaceConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const TcpInterfaceConfig& lhs,
                           const TcpInterfaceConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const TcpInterfaceConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `TcpInterfaceConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const TcpInterfaceConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::TcpInterfaceConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::TcpInterfaceConfig>
: bsl::true_type {};

namespace mqbcfg {

// ===============================
// class AuthenticatorPluginConfig
// ===============================

/// The configuration for an authenticator plugin.
/// name.....: The name of the authenticator plugin.  settings.:
/// Plugin-specific settings.
class AuthenticatorPluginConfig {
    // INSTANCE DATA

    bsl::vector<PluginSettingKeyValue> d_settings;
    bsl::string                        d_name;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_NAME = 0, ATTRIBUTE_ID_SETTINGS = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_NAME = 0, ATTRIBUTE_INDEX_SETTINGS = 1 };

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

    /// Create an object of type `AuthenticatorPluginConfig` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit AuthenticatorPluginConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `AuthenticatorPluginConfig` having the value
    /// of the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    AuthenticatorPluginConfig(const AuthenticatorPluginConfig& original,
                              bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `AuthenticatorPluginConfig` having the value
    /// of the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    AuthenticatorPluginConfig(AuthenticatorPluginConfig&& original) noexcept;

    /// Create an object of type `AuthenticatorPluginConfig` having the value
    /// of the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    AuthenticatorPluginConfig(AuthenticatorPluginConfig&& original,
                              bslma::Allocator*           basicAllocator);
#endif

    /// Destroy this object.
    ~AuthenticatorPluginConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    AuthenticatorPluginConfig& operator=(const AuthenticatorPluginConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    AuthenticatorPluginConfig& operator=(AuthenticatorPluginConfig&& rhs);
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

    /// Return a reference to the modifiable "Settings" attribute of this
    /// object.
    bsl::vector<PluginSettingKeyValue>& settings();

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

    /// Return a reference offering non-modifiable access to the "Settings"
    /// attribute of this object.
    const bsl::vector<PluginSettingKeyValue>& settings() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const AuthenticatorPluginConfig& lhs,
                           const AuthenticatorPluginConfig& rhs)
    {
        return lhs.name() == rhs.name() && lhs.settings() == rhs.settings();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const AuthenticatorPluginConfig& lhs,
                           const AuthenticatorPluginConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                    stream,
                                    const AuthenticatorPluginConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `AuthenticatorPluginConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                hashAlg,
                           const AuthenticatorPluginConfig& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.name());
        hashAppend(hashAlg, object.settings());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::AuthenticatorPluginConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::AuthenticatorPluginConfig>
: bsl::true_type {};

namespace mqbcfg {

// ============================
// class AuthorizerPluginConfig
// ============================

/// The configuration for an authorizer plugin.
/// name.....: The name of the authenticator plugin.  settings.:
/// Plugin-specific settings.
class AuthorizerPluginConfig {
    // INSTANCE DATA

    bsl::vector<PluginSettingKeyValue> d_settings;
    bsl::string                        d_name;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_NAME = 0, ATTRIBUTE_ID_SETTINGS = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_NAME = 0, ATTRIBUTE_INDEX_SETTINGS = 1 };

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

    /// Create an object of type `AuthorizerPluginConfig` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit AuthorizerPluginConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `AuthorizerPluginConfig` having the value of
    /// the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    AuthorizerPluginConfig(const AuthorizerPluginConfig& original,
                           bslma::Allocator*             basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `AuthorizerPluginConfig` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    AuthorizerPluginConfig(AuthorizerPluginConfig&& original) noexcept;

    /// Create an object of type `AuthorizerPluginConfig` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    AuthorizerPluginConfig(AuthorizerPluginConfig&& original,
                           bslma::Allocator*        basicAllocator);
#endif

    /// Destroy this object.
    ~AuthorizerPluginConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    AuthorizerPluginConfig& operator=(const AuthorizerPluginConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    AuthorizerPluginConfig& operator=(AuthorizerPluginConfig&& rhs);
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

    /// Return a reference to the modifiable "Settings" attribute of this
    /// object.
    bsl::vector<PluginSettingKeyValue>& settings();

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

    /// Return a reference offering non-modifiable access to the "Settings"
    /// attribute of this object.
    const bsl::vector<PluginSettingKeyValue>& settings() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const AuthorizerPluginConfig& lhs,
                           const AuthorizerPluginConfig& rhs)
    {
        return lhs.name() == rhs.name() && lhs.settings() == rhs.settings();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const AuthorizerPluginConfig& lhs,
                           const AuthorizerPluginConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                 stream,
                                    const AuthorizerPluginConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `AuthorizerPluginConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&             hashAlg,
                           const AuthorizerPluginConfig& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.name());
        hashAppend(hashAlg, object.settings());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::AuthorizerPluginConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::AuthorizerPluginConfig>
: bsl::true_type {};

namespace mqbcfg {

// =================
// class ClusterNode
// =================

/// Type representing the configuration of a node in a cluster.
/// id.........: the unique ID of that node in the cluster; must be a > 0 value
/// name.......: name of this node datacenter.: the datacenter of that node
/// transport..: the transport configuration for establishing connectivity with
/// the node
class ClusterNode {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `ClusterNode` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit ClusterNode(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `ClusterNode` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    ClusterNode(const ClusterNode& original,
                bslma::Allocator*  basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `ClusterNode` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    ClusterNode(ClusterNode&& original) noexcept;

    /// Create an object of type `ClusterNode` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    ClusterNode(ClusterNode&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~ClusterNode();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ClusterNode& operator=(const ClusterNode& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    ClusterNode& operator=(ClusterNode&& rhs);
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
    int& id();

    /// Return a reference to the modifiable "Name" attribute of this object.
    bsl::string& name();

    /// Return a reference to the modifiable "DataCenter" attribute of this
    /// object.
    bsl::string& dataCenter();

    /// Return a reference to the modifiable "Transport" attribute of this
    /// object.
    ClusterNodeConnection& transport();

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

    /// Return the value of the "Id" attribute of this object.
    int id() const;

    /// Return a reference offering non-modifiable access to the "Name"
    /// attribute of this object.
    const bsl::string& name() const;

    /// Return a reference offering non-modifiable access to the "DataCenter"
    /// attribute of this object.
    const bsl::string& dataCenter() const;

    /// Return a reference offering non-modifiable access to the "Transport"
    /// attribute of this object.
    const ClusterNodeConnection& transport() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ClusterNode& lhs, const ClusterNode& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ClusterNode& lhs, const ClusterNode& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&      stream,
                                    const ClusterNode& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ClusterNode`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&  hashAlg,
                           const ClusterNode& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::ClusterNode);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::ClusterNode> : bsl::true_type {};

namespace mqbcfg {

// ==============================
// class CredentialProviderConfig
// ==============================

/// The configuration for the broker's credential provider.
/// name.....: The name of the credential provider.  settings.: Plugin-specific
/// settings.
class CredentialProviderConfig {
    // INSTANCE DATA

    bsl::vector<PluginSettingKeyValue> d_settings;
    bsl::string                        d_name;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_NAME = 0, ATTRIBUTE_ID_SETTINGS = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_NAME = 0, ATTRIBUTE_INDEX_SETTINGS = 1 };

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

    /// Create an object of type `CredentialProviderConfig` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit CredentialProviderConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `CredentialProviderConfig` having the value of
    /// the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    CredentialProviderConfig(const CredentialProviderConfig& original,
                             bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `CredentialProviderConfig` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    CredentialProviderConfig(CredentialProviderConfig&& original) noexcept;

    /// Create an object of type `CredentialProviderConfig` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    CredentialProviderConfig(CredentialProviderConfig&& original,
                             bslma::Allocator*          basicAllocator);
#endif

    /// Destroy this object.
    ~CredentialProviderConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    CredentialProviderConfig& operator=(const CredentialProviderConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    CredentialProviderConfig& operator=(CredentialProviderConfig&& rhs);
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

    /// Return a reference to the modifiable "Settings" attribute of this
    /// object.
    bsl::vector<PluginSettingKeyValue>& settings();

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

    /// Return a reference offering non-modifiable access to the "Settings"
    /// attribute of this object.
    const bsl::vector<PluginSettingKeyValue>& settings() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const CredentialProviderConfig& lhs,
                           const CredentialProviderConfig& rhs)
    {
        return lhs.name() == rhs.name() && lhs.settings() == rhs.settings();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const CredentialProviderConfig& lhs,
                           const CredentialProviderConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                   stream,
                                    const CredentialProviderConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `CredentialProviderConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&               hashAlg,
                           const CredentialProviderConfig& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.name());
        hashAppend(hashAlg, object.settings());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::CredentialProviderConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::CredentialProviderConfig>
: bsl::true_type {};

namespace mqbcfg {

// ======================
// class DispatcherConfig
// ======================

class DispatcherConfig {
    // INSTANCE DATA

    DispatcherProcessorConfig d_sessions;
    DispatcherProcessorConfig d_queues;
    DispatcherProcessorConfig d_clusters;
    int                       d_alarmTimeoutMs;
    int                       d_warningTimeoutMs;

    // PRIVATE ACCESSORS

    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const DispatcherConfig& rhs) const;

  public:
    // TYPES

    enum {
        ATTRIBUTE_ID_SESSIONS           = 0,
        ATTRIBUTE_ID_QUEUES             = 1,
        ATTRIBUTE_ID_CLUSTERS           = 2,
        ATTRIBUTE_ID_ALARM_TIMEOUT_MS   = 3,
        ATTRIBUTE_ID_WARNING_TIMEOUT_MS = 4
    };

    enum { NUM_ATTRIBUTES = 5 };

    enum {
        ATTRIBUTE_INDEX_SESSIONS           = 0,
        ATTRIBUTE_INDEX_QUEUES             = 1,
        ATTRIBUTE_INDEX_CLUSTERS           = 2,
        ATTRIBUTE_INDEX_ALARM_TIMEOUT_MS   = 3,
        ATTRIBUTE_INDEX_WARNING_TIMEOUT_MS = 4
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_ALARM_TIMEOUT_MS;

    static const int DEFAULT_INITIALIZER_WARNING_TIMEOUT_MS;

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

    /// Create an object of type `DispatcherConfig` having the default value.
    DispatcherConfig();

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

    /// Return a reference to the modifiable "Sessions" attribute of this
    /// object.
    DispatcherProcessorConfig& sessions();

    /// Return a reference to the modifiable "Queues" attribute of this object.
    DispatcherProcessorConfig& queues();

    /// Return a reference to the modifiable "Clusters" attribute of this
    /// object.
    DispatcherProcessorConfig& clusters();

    /// Return a reference to the modifiable "AlarmTimeoutMs" attribute of this
    /// object.
    int& alarmTimeoutMs();

    /// Return a reference to the modifiable "WarningTimeoutMs" attribute of
    /// this object.
    int& warningTimeoutMs();

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

    /// Return a reference offering non-modifiable access to the "Sessions"
    /// attribute of this object.
    const DispatcherProcessorConfig& sessions() const;

    /// Return a reference offering non-modifiable access to the "Queues"
    /// attribute of this object.
    const DispatcherProcessorConfig& queues() const;

    /// Return a reference offering non-modifiable access to the "Clusters"
    /// attribute of this object.
    const DispatcherProcessorConfig& clusters() const;

    /// Return the value of the "AlarmTimeoutMs" attribute of this object.
    int alarmTimeoutMs() const;

    /// Return the value of the "WarningTimeoutMs" attribute of this object.
    int warningTimeoutMs() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const DispatcherConfig& lhs,
                           const DispatcherConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const DispatcherConfig& lhs,
                           const DispatcherConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const DispatcherConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `DispatcherConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const DispatcherConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(mqbcfg::DispatcherConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::DispatcherConfig> : bsl::true_type {
};

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `NetworkInterfaces` having the default value.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit NetworkInterfaces(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `NetworkInterfaces` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    NetworkInterfaces(const NetworkInterfaces& original,
                      bslma::Allocator*        basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `NetworkInterfaces` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    NetworkInterfaces(NetworkInterfaces&& original) noexcept;

    /// Create an object of type `NetworkInterfaces` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    NetworkInterfaces(NetworkInterfaces&& original,
                      bslma::Allocator*   basicAllocator);
#endif

    /// Destroy this object.
    ~NetworkInterfaces();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    NetworkInterfaces& operator=(const NetworkInterfaces& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    NetworkInterfaces& operator=(NetworkInterfaces&& rhs);
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

    /// Return a reference to the modifiable "Heartbeats" attribute of this
    /// object.
    Heartbeat& heartbeats();

    /// Return a reference to the modifiable "TcpInterface" attribute of this
    /// object.
    bdlb::NullableValue<TcpInterfaceConfig>& tcpInterface();

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

    /// Return a reference offering non-modifiable access to the "Heartbeats"
    /// attribute of this object.
    const Heartbeat& heartbeats() const;

    /// Return a reference offering non-modifiable access to the "TcpInterface"
    /// attribute of this object.
    const bdlb::NullableValue<TcpInterfaceConfig>& tcpInterface() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const NetworkInterfaces& lhs,
                           const NetworkInterfaces& rhs)
    {
        return lhs.heartbeats() == rhs.heartbeats() &&
               lhs.tcpInterface() == rhs.tcpInterface();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const NetworkInterfaces& lhs,
                           const NetworkInterfaces& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const NetworkInterfaces& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `NetworkInterfaces`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const NetworkInterfaces& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.heartbeats());
        hashAppend(hashAlg, object.tcpInterface());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::NetworkInterfaces);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::NetworkInterfaces> : bsl::true_type {
};

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `StatPluginConfig` having the default value.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit StatPluginConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatPluginConfig` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    StatPluginConfig(const StatPluginConfig& original,
                     bslma::Allocator*       basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatPluginConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    StatPluginConfig(StatPluginConfig&& original) noexcept;

    /// Create an object of type `StatPluginConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    StatPluginConfig(StatPluginConfig&& original,
                     bslma::Allocator*  basicAllocator);
#endif

    /// Destroy this object.
    ~StatPluginConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatPluginConfig& operator=(const StatPluginConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    StatPluginConfig& operator=(StatPluginConfig&& rhs);
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

    /// Return a reference to the modifiable "QueueSize" attribute of this
    /// object.
    int& queueSize();

    /// Return a reference to the modifiable "QueueHighWatermark" attribute of
    /// this object.
    int& queueHighWatermark();

    /// Return a reference to the modifiable "QueueLowWatermark" attribute of
    /// this object.
    int& queueLowWatermark();

    /// Return a reference to the modifiable "PublishInterval" attribute of
    /// this object.
    int& publishInterval();

    /// Return a reference to the modifiable "NamespacePrefix" attribute of
    /// this object.
    bsl::string& namespacePrefix();

    /// Return a reference to the modifiable "Hosts" attribute of this object.
    bsl::vector<bsl::string>& hosts();

    /// Return a reference to the modifiable "InstanceId" attribute of this
    /// object.
    bsl::string& instanceId();

    /// Return a reference to the modifiable "PrometheusSpecific" attribute of
    /// this object.
    bdlb::NullableValue<StatPluginConfigPrometheus>& prometheusSpecific();

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

    /// Return the value of the "QueueSize" attribute of this object.
    int queueSize() const;

    /// Return the value of the "QueueHighWatermark" attribute of this object.
    int queueHighWatermark() const;

    /// Return the value of the "QueueLowWatermark" attribute of this object.
    int queueLowWatermark() const;

    /// Return the value of the "PublishInterval" attribute of this object.
    int publishInterval() const;

    /// Return a reference offering non-modifiable access to the
    /// "NamespacePrefix" attribute of this object.
    const bsl::string& namespacePrefix() const;

    /// Return a reference offering non-modifiable access to the "Hosts"
    /// attribute of this object.
    const bsl::vector<bsl::string>& hosts() const;

    /// Return a reference offering non-modifiable access to the "InstanceId"
    /// attribute of this object.
    const bsl::string& instanceId() const;

    /// Return a reference offering non-modifiable access to the
    /// "PrometheusSpecific" attribute of this object.
    const bdlb::NullableValue<StatPluginConfigPrometheus>&
    prometheusSpecific() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const StatPluginConfig& lhs,
                           const StatPluginConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const StatPluginConfig& lhs,
                           const StatPluginConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const StatPluginConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `StatPluginConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const StatPluginConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::StatPluginConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::StatPluginConfig> : bsl::true_type {
};

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `TaskConfig` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit TaskConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `TaskConfig` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    TaskConfig(const TaskConfig& original,
               bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `TaskConfig` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    TaskConfig(TaskConfig&& original) noexcept;

    /// Create an object of type `TaskConfig` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    TaskConfig(TaskConfig&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~TaskConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    TaskConfig& operator=(const TaskConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    TaskConfig& operator=(TaskConfig&& rhs);
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

    /// Return a reference to the modifiable "AllocatorType" attribute of this
    /// object.
    AllocatorType::Value& allocatorType();

    /// Return a reference to the modifiable "AllocationLimit" attribute of
    /// this object.
    bsls::Types::Uint64& allocationLimit();

    /// Return a reference to the modifiable "LogController" attribute of this
    /// object.
    LogController& logController();

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

    /// Return the value of the "AllocatorType" attribute of this object.
    AllocatorType::Value allocatorType() const;

    /// Return the value of the "AllocationLimit" attribute of this object.
    bsls::Types::Uint64 allocationLimit() const;

    /// Return a reference offering non-modifiable access to the
    /// "LogController" attribute of this object.
    const LogController& logController() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const TaskConfig& lhs, const TaskConfig& rhs)
    {
        return lhs.allocatorType() == rhs.allocatorType() &&
               lhs.allocationLimit() == rhs.allocationLimit() &&
               lhs.logController() == rhs.logController();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const TaskConfig& lhs, const TaskConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&     stream,
                                    const TaskConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `TaskConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const TaskConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::TaskConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::TaskConfig> : bsl::true_type {};

namespace mqbcfg {

// =========================
// class AuthenticatorConfig
// =========================

/// Top level type for the broker's authentication configurations.
/// authenticators...........: Configuration entries for authenticator plugins
/// (built-in or external).  Each entry defines settings for a specific plugin.
///  All plugins must have unique authentication mechanisms.
/// anonymousCredential.: Controls anonymous authentication behavior.  When
/// specified, the broker uses the provided credential with a matching plugin
/// from `authenticators`.  When omitted, the broker defaults to
/// AnonAuthenticator and always passes for anonymous authentication.
/// minThreads..............: Minimum number of threads in the authentication
/// thread pool.  maxThreads..............: Maximum number of threads in the
/// authentication thread pool.  credentialProvider...: Optional credential
/// provider.  When specified, the broker uses credentials provided by the
/// credential provider to authenticate with other brokers.  If not specified,
/// the broker will attempt to directly negotiate sessions with other brokers
/// without explicitly authenticating.
class AuthenticatorConfig {
    // INSTANCE DATA

    bsl::vector<AuthenticatorPluginConfig>        d_authenticators;
    bdlb::NullableValue<CredentialProviderConfig> d_credentialProvider;
    bdlb::NullableValue<AnonymousCredential>      d_anonymousCredential;
    int                                           d_minThreads;
    int                                           d_maxThreads;

    // PRIVATE ACCESSORS

    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const AuthenticatorConfig& rhs) const;

  public:
    // TYPES

    enum {
        ATTRIBUTE_ID_AUTHENTICATORS       = 0,
        ATTRIBUTE_ID_ANONYMOUS_CREDENTIAL = 1,
        ATTRIBUTE_ID_MIN_THREADS          = 2,
        ATTRIBUTE_ID_MAX_THREADS          = 3,
        ATTRIBUTE_ID_CREDENTIAL_PROVIDER  = 4
    };

    enum { NUM_ATTRIBUTES = 5 };

    enum {
        ATTRIBUTE_INDEX_AUTHENTICATORS       = 0,
        ATTRIBUTE_INDEX_ANONYMOUS_CREDENTIAL = 1,
        ATTRIBUTE_INDEX_MIN_THREADS          = 2,
        ATTRIBUTE_INDEX_MAX_THREADS          = 3,
        ATTRIBUTE_INDEX_CREDENTIAL_PROVIDER  = 4
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_MIN_THREADS;

    static const int DEFAULT_INITIALIZER_MAX_THREADS;

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

    /// Create an object of type `AuthenticatorConfig` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit AuthenticatorConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `AuthenticatorConfig` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    AuthenticatorConfig(const AuthenticatorConfig& original,
                        bslma::Allocator*          basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `AuthenticatorConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    AuthenticatorConfig(AuthenticatorConfig&& original) noexcept;

    /// Create an object of type `AuthenticatorConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    AuthenticatorConfig(AuthenticatorConfig&& original,
                        bslma::Allocator*     basicAllocator);
#endif

    /// Destroy this object.
    ~AuthenticatorConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    AuthenticatorConfig& operator=(const AuthenticatorConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    AuthenticatorConfig& operator=(AuthenticatorConfig&& rhs);
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

    /// Return a reference to the modifiable "Authenticators" attribute of this
    /// object.
    bsl::vector<AuthenticatorPluginConfig>& authenticators();

    /// Return a reference to the modifiable "AnonymousCredential" attribute of
    /// this object.
    bdlb::NullableValue<AnonymousCredential>& anonymousCredential();

    /// Return a reference to the modifiable "MinThreads" attribute of this
    /// object.
    int& minThreads();

    /// Return a reference to the modifiable "MaxThreads" attribute of this
    /// object.
    int& maxThreads();

    /// Return a reference to the modifiable "CredentialProvider" attribute of
    /// this object.
    bdlb::NullableValue<CredentialProviderConfig>& credentialProvider();

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
    /// "Authenticators" attribute of this object.
    const bsl::vector<AuthenticatorPluginConfig>& authenticators() const;

    /// Return a reference offering non-modifiable access to the
    /// "AnonymousCredential" attribute of this object.
    const bdlb::NullableValue<AnonymousCredential>&
    anonymousCredential() const;

    /// Return the value of the "MinThreads" attribute of this object.
    int minThreads() const;

    /// Return the value of the "MaxThreads" attribute of this object.
    int maxThreads() const;

    /// Return a reference offering non-modifiable access to the
    /// "CredentialProvider" attribute of this object.
    const bdlb::NullableValue<CredentialProviderConfig>&
    credentialProvider() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const AuthenticatorConfig& lhs,
                           const AuthenticatorConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const AuthenticatorConfig& lhs,
                           const AuthenticatorConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&              stream,
                                    const AuthenticatorConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `AuthenticatorConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&          hashAlg,
                           const AuthenticatorConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::AuthenticatorConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::AuthenticatorConfig>
: bsl::true_type {};

namespace mqbcfg {

// ======================
// class AuthorizerConfig
// ======================

/// Top level type for the broker's authorization configurations.
/// authorizer...........: Configuration entries for the authorizer plugin
/// (built-in or external).
class AuthorizerConfig {
    // INSTANCE DATA

    bdlb::NullableValue<AuthorizerPluginConfig> d_authorizer;

  public:
    // TYPES

    enum { ATTRIBUTE_ID_AUTHORIZER = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_AUTHORIZER = 0 };

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

    /// Create an object of type `AuthorizerConfig` having the default value.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit AuthorizerConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `AuthorizerConfig` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    AuthorizerConfig(const AuthorizerConfig& original,
                     bslma::Allocator*       basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `AuthorizerConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    AuthorizerConfig(AuthorizerConfig&& original) noexcept;

    /// Create an object of type `AuthorizerConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    AuthorizerConfig(AuthorizerConfig&& original,
                     bslma::Allocator*  basicAllocator);
#endif

    /// Destroy this object.
    ~AuthorizerConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    AuthorizerConfig& operator=(const AuthorizerConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    AuthorizerConfig& operator=(AuthorizerConfig&& rhs);
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

    /// Return a reference to the modifiable "Authorizer" attribute of this
    /// object.
    bdlb::NullableValue<AuthorizerPluginConfig>& authorizer();

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

    /// Return a reference offering non-modifiable access to the "Authorizer"
    /// attribute of this object.
    const bdlb::NullableValue<AuthorizerPluginConfig>& authorizer() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const AuthorizerConfig& lhs,
                           const AuthorizerConfig& rhs)
    {
        return lhs.authorizer() == rhs.authorizer();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const AuthorizerConfig& lhs,
                           const AuthorizerConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const AuthorizerConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `AuthorizerConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const AuthorizerConfig& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.authorizer());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::AuthorizerConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::AuthorizerConfig> : bsl::true_type {
};

namespace mqbcfg {

// =======================
// class ClusterDefinition
// =======================

/// Type representing the configuration for a cluster.
/// name..................: name of the cluster nodes.................: list of
/// nodes in the cluster partitionConfig.......: configuration for the storage
/// masterAssignment......: algorithm to use for partition's master assignment
/// elector...............: configuration for leader election amongst the nodes
/// queueOperations.......: configuration for queue operations on the cluster
/// clusterAttributes.....: attributes specific to this cluster
/// clusterMonitorConfig..: configuration for cluster state monitor
/// messageThrottleConfig.: configuration for message throttling intervals and
/// thresholds.
class ClusterDefinition {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `ClusterDefinition` having the default value.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit ClusterDefinition(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `ClusterDefinition` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    ClusterDefinition(const ClusterDefinition& original,
                      bslma::Allocator*        basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `ClusterDefinition` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    ClusterDefinition(ClusterDefinition&& original) noexcept;

    /// Create an object of type `ClusterDefinition` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    ClusterDefinition(ClusterDefinition&& original,
                      bslma::Allocator*   basicAllocator);
#endif

    /// Destroy this object.
    ~ClusterDefinition();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ClusterDefinition& operator=(const ClusterDefinition& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    ClusterDefinition& operator=(ClusterDefinition&& rhs);
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

    /// Return a reference to the modifiable "Nodes" attribute of this object.
    bsl::vector<ClusterNode>& nodes();

    /// Return a reference to the modifiable "PartitionConfig" attribute of
    /// this object.
    PartitionConfig& partitionConfig();

    /// Return a reference to the modifiable "MasterAssignment" attribute of
    /// this object.
    MasterAssignmentAlgorithm::Value& masterAssignment();

    /// Return a reference to the modifiable "Elector" attribute of this
    /// object.
    ElectorConfig& elector();

    /// Return a reference to the modifiable "QueueOperations" attribute of
    /// this object.
    QueueOperationsConfig& queueOperations();

    /// Return a reference to the modifiable "ClusterAttributes" attribute of
    /// this object.
    ClusterAttributes& clusterAttributes();

    /// Return a reference to the modifiable "ClusterMonitorConfig" attribute
    /// of this object.
    ClusterMonitorConfig& clusterMonitorConfig();

    /// Return a reference to the modifiable "MessageThrottleConfig" attribute
    /// of this object.
    MessageThrottleConfig& messageThrottleConfig();

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

    /// Return a reference offering non-modifiable access to the "Nodes"
    /// attribute of this object.
    const bsl::vector<ClusterNode>& nodes() const;

    /// Return a reference offering non-modifiable access to the
    /// "PartitionConfig" attribute of this object.
    const PartitionConfig& partitionConfig() const;

    /// Return the value of the "MasterAssignment" attribute of this object.
    MasterAssignmentAlgorithm::Value masterAssignment() const;

    /// Return a reference offering non-modifiable access to the "Elector"
    /// attribute of this object.
    const ElectorConfig& elector() const;

    /// Return a reference offering non-modifiable access to the
    /// "QueueOperations" attribute of this object.
    const QueueOperationsConfig& queueOperations() const;

    /// Return a reference offering non-modifiable access to the
    /// "ClusterAttributes" attribute of this object.
    const ClusterAttributes& clusterAttributes() const;

    /// Return a reference offering non-modifiable access to the
    /// "ClusterMonitorConfig" attribute of this object.
    const ClusterMonitorConfig& clusterMonitorConfig() const;

    /// Return a reference offering non-modifiable access to the
    /// "MessageThrottleConfig" attribute of this object.
    const MessageThrottleConfig& messageThrottleConfig() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ClusterDefinition& lhs,
                           const ClusterDefinition& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ClusterDefinition& lhs,
                           const ClusterDefinition& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const ClusterDefinition& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ClusterDefinition`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const ClusterDefinition& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ClusterDefinition);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::ClusterDefinition> : bsl::true_type {
};

namespace mqbcfg {

// ============================
// class ClusterProxyDefinition
// ============================

/// Type representing the configuration for a cluster proxy.
/// name..................: name of the cluster nodes.................: list of
/// nodes in the cluster queueOperations.......: configuration for queue
/// operations with the cluster clusterMonitorConfig..: configuration for
/// cluster state monitor messageThrottleConfig.: configuration for message
/// throttling intervals and thresholds.
class ClusterProxyDefinition {
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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `ClusterProxyDefinition` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply memory.
    ///  If `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit ClusterProxyDefinition(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `ClusterProxyDefinition` having the value of
    /// the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    ClusterProxyDefinition(const ClusterProxyDefinition& original,
                           bslma::Allocator*             basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `ClusterProxyDefinition` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    ClusterProxyDefinition(ClusterProxyDefinition&& original) noexcept;

    /// Create an object of type `ClusterProxyDefinition` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    ClusterProxyDefinition(ClusterProxyDefinition&& original,
                           bslma::Allocator*        basicAllocator);
#endif

    /// Destroy this object.
    ~ClusterProxyDefinition();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ClusterProxyDefinition& operator=(const ClusterProxyDefinition& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    ClusterProxyDefinition& operator=(ClusterProxyDefinition&& rhs);
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

    /// Return a reference to the modifiable "Nodes" attribute of this object.
    bsl::vector<ClusterNode>& nodes();

    /// Return a reference to the modifiable "QueueOperations" attribute of
    /// this object.
    QueueOperationsConfig& queueOperations();

    /// Return a reference to the modifiable "ClusterMonitorConfig" attribute
    /// of this object.
    ClusterMonitorConfig& clusterMonitorConfig();

    /// Return a reference to the modifiable "MessageThrottleConfig" attribute
    /// of this object.
    MessageThrottleConfig& messageThrottleConfig();

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

    /// Return a reference offering non-modifiable access to the "Nodes"
    /// attribute of this object.
    const bsl::vector<ClusterNode>& nodes() const;

    /// Return a reference offering non-modifiable access to the
    /// "QueueOperations" attribute of this object.
    const QueueOperationsConfig& queueOperations() const;

    /// Return a reference offering non-modifiable access to the
    /// "ClusterMonitorConfig" attribute of this object.
    const ClusterMonitorConfig& clusterMonitorConfig() const;

    /// Return a reference offering non-modifiable access to the
    /// "MessageThrottleConfig" attribute of this object.
    const MessageThrottleConfig& messageThrottleConfig() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ClusterProxyDefinition& lhs,
                           const ClusterProxyDefinition& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ClusterProxyDefinition& lhs,
                           const ClusterProxyDefinition& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&                 stream,
                                    const ClusterProxyDefinition& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ClusterProxyDefinition`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&             hashAlg,
                           const ClusterProxyDefinition& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ClusterProxyDefinition);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::ClusterProxyDefinition>
: bsl::true_type {};

namespace mqbcfg {

// =================
// class StatsConfig
// =================

class StatsConfig {
    // INSTANCE DATA

    bsl::vector<StatPluginConfig> d_plugins;
    StatsPrinterConfig            d_printer;
    int                           d_snapshotInterval;

    // PRIVATE ACCESSORS

    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES

    enum {
        ATTRIBUTE_ID_SNAPSHOT_INTERVAL = 0,
        ATTRIBUTE_ID_PLUGINS           = 1,
        ATTRIBUTE_ID_PRINTER           = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_SNAPSHOT_INTERVAL = 0,
        ATTRIBUTE_INDEX_PLUGINS           = 1,
        ATTRIBUTE_INDEX_PRINTER           = 2
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_SNAPSHOT_INTERVAL;

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

    /// Create an object of type `StatsConfig` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit StatsConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatsConfig` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    StatsConfig(const StatsConfig& original,
                bslma::Allocator*  basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatsConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    StatsConfig(StatsConfig&& original) noexcept;

    /// Create an object of type `StatsConfig` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    StatsConfig(StatsConfig&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~StatsConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatsConfig& operator=(const StatsConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    StatsConfig& operator=(StatsConfig&& rhs);
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

    /// Return a reference to the modifiable "SnapshotInterval" attribute of
    /// this object.
    int& snapshotInterval();

    /// Return a reference to the modifiable "Plugins" attribute of this
    /// object.
    bsl::vector<StatPluginConfig>& plugins();

    /// Return a reference to the modifiable "Printer" attribute of this
    /// object.
    StatsPrinterConfig& printer();

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

    /// Return the value of the "SnapshotInterval" attribute of this object.
    int snapshotInterval() const;

    /// Return a reference offering non-modifiable access to the "Plugins"
    /// attribute of this object.
    const bsl::vector<StatPluginConfig>& plugins() const;

    /// Return a reference offering non-modifiable access to the "Printer"
    /// attribute of this object.
    const StatsPrinterConfig& printer() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const StatsConfig& lhs, const StatsConfig& rhs)
    {
        return lhs.snapshotInterval() == rhs.snapshotInterval() &&
               lhs.plugins() == rhs.plugins() &&
               lhs.printer() == rhs.printer();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const StatsConfig& lhs, const StatsConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&      stream,
                                    const StatsConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `StatsConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&  hashAlg,
                           const StatsConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::StatsConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::StatsConfig> : bsl::true_type {};

namespace mqbcfg {

// ===============
// class AppConfig
// ===============

/// Top level type for the broker's configuration.
/// brokerInstanceName...: name of the broker instance brokerVersion........:
/// version of the broker configVersion........: version of the bmqbrkr.cfg
/// config etcDir...............: directory containing the json config files
/// hostName.............: name of the current host hostTags.............: tags
/// of the current host hostDataCenter.......: datacenter the current host
/// resides in logsObserverMaxSize..: maximum number of log records to keep
/// latencyMonitorDomain.: common prefix of all latemon domains
/// dispatcherConfig.....: configuration for the dispatcher
/// stats................: configuration for the stats networkInterfaces....:
/// configuration for the network interfaces bmqconfConfig........:
/// configuration for bmqconf plugins..............: configuration for the
/// plugins msgPropertiesSupport.: information about if/how to advertise
/// support for v2 message properties configureStream......: send new
/// ConfigureStream instead of old ConfigureQueue advertiseSubscriptions.:
/// temporarily control use of ConfigureStream in SDK routeCommandTimeoutMs:
/// maximum amount of time to wait for a routed command's response
/// authentication.......: configuration for authentication
/// tlsConfig............: optional configuration for TLS
class AppConfig {
    // INSTANCE DATA

    bsl::string                    d_brokerInstanceName;
    bsl::string                    d_etcDir;
    bsl::string                    d_hostName;
    bsl::string                    d_hostTags;
    bsl::string                    d_hostDataCenter;
    bsl::string                    d_latencyMonitorDomain;
    bdlb::NullableValue<TlsConfig> d_tlsConfig;
    StatsConfig                    d_stats;
    Plugins                        d_plugins;
    NetworkInterfaces              d_networkInterfaces;
    MessagePropertiesV2            d_messagePropertiesV2;
    DispatcherConfig               d_dispatcherConfig;
    BmqconfConfig                  d_bmqconfConfig;
    AuthorizerConfig               d_authorization;
    AuthenticatorConfig            d_authentication;
    int                            d_brokerVersion;
    int                            d_configVersion;
    int                            d_logsObserverMaxSize;
    int                            d_routeCommandTimeoutMs;
    bool                           d_configureStream;
    bool                           d_advertiseSubscriptions;

    // PRIVATE ACCESSORS

    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const AppConfig& rhs) const;

  public:
    // TYPES

    enum {
        ATTRIBUTE_ID_BROKER_INSTANCE_NAME     = 0,
        ATTRIBUTE_ID_BROKER_VERSION           = 1,
        ATTRIBUTE_ID_CONFIG_VERSION           = 2,
        ATTRIBUTE_ID_ETC_DIR                  = 3,
        ATTRIBUTE_ID_HOST_NAME                = 4,
        ATTRIBUTE_ID_HOST_TAGS                = 5,
        ATTRIBUTE_ID_HOST_DATA_CENTER         = 6,
        ATTRIBUTE_ID_LOGS_OBSERVER_MAX_SIZE   = 7,
        ATTRIBUTE_ID_LATENCY_MONITOR_DOMAIN   = 8,
        ATTRIBUTE_ID_DISPATCHER_CONFIG        = 9,
        ATTRIBUTE_ID_STATS                    = 10,
        ATTRIBUTE_ID_NETWORK_INTERFACES       = 11,
        ATTRIBUTE_ID_BMQCONF_CONFIG           = 12,
        ATTRIBUTE_ID_PLUGINS                  = 13,
        ATTRIBUTE_ID_MESSAGE_PROPERTIES_V2    = 14,
        ATTRIBUTE_ID_CONFIGURE_STREAM         = 15,
        ATTRIBUTE_ID_ADVERTISE_SUBSCRIPTIONS  = 16,
        ATTRIBUTE_ID_ROUTE_COMMAND_TIMEOUT_MS = 17,
        ATTRIBUTE_ID_AUTHENTICATION           = 18,
        ATTRIBUTE_ID_AUTHORIZATION            = 19,
        ATTRIBUTE_ID_TLS_CONFIG               = 20
    };

    enum { NUM_ATTRIBUTES = 21 };

    enum {
        ATTRIBUTE_INDEX_BROKER_INSTANCE_NAME     = 0,
        ATTRIBUTE_INDEX_BROKER_VERSION           = 1,
        ATTRIBUTE_INDEX_CONFIG_VERSION           = 2,
        ATTRIBUTE_INDEX_ETC_DIR                  = 3,
        ATTRIBUTE_INDEX_HOST_NAME                = 4,
        ATTRIBUTE_INDEX_HOST_TAGS                = 5,
        ATTRIBUTE_INDEX_HOST_DATA_CENTER         = 6,
        ATTRIBUTE_INDEX_LOGS_OBSERVER_MAX_SIZE   = 7,
        ATTRIBUTE_INDEX_LATENCY_MONITOR_DOMAIN   = 8,
        ATTRIBUTE_INDEX_DISPATCHER_CONFIG        = 9,
        ATTRIBUTE_INDEX_STATS                    = 10,
        ATTRIBUTE_INDEX_NETWORK_INTERFACES       = 11,
        ATTRIBUTE_INDEX_BMQCONF_CONFIG           = 12,
        ATTRIBUTE_INDEX_PLUGINS                  = 13,
        ATTRIBUTE_INDEX_MESSAGE_PROPERTIES_V2    = 14,
        ATTRIBUTE_INDEX_CONFIGURE_STREAM         = 15,
        ATTRIBUTE_INDEX_ADVERTISE_SUBSCRIPTIONS  = 16,
        ATTRIBUTE_INDEX_ROUTE_COMMAND_TIMEOUT_MS = 17,
        ATTRIBUTE_INDEX_AUTHENTICATION           = 18,
        ATTRIBUTE_INDEX_AUTHORIZATION            = 19,
        ATTRIBUTE_INDEX_TLS_CONFIG               = 20
    };

    // CONSTANTS

    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_LATENCY_MONITOR_DOMAIN[];

    static const bool DEFAULT_INITIALIZER_CONFIGURE_STREAM;

    static const bool DEFAULT_INITIALIZER_ADVERTISE_SUBSCRIPTIONS;

    static const int DEFAULT_INITIALIZER_ROUTE_COMMAND_TIMEOUT_MS;

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

    /// Create an object of type `AppConfig` having the default value.  Use the
    /// optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit AppConfig(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `AppConfig` having the value of the specified
    /// `original` object.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    AppConfig(const AppConfig& original, bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `AppConfig` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.
    AppConfig(AppConfig&& original) noexcept;

    /// Create an object of type `AppConfig` having the value of the specified
    /// `original` object.  After performing this action, the `original` object
    /// will be left in a valid, but unspecified state.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    AppConfig(AppConfig&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~AppConfig();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    AppConfig& operator=(const AppConfig& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    AppConfig& operator=(AppConfig&& rhs);
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

    /// Return a reference to the modifiable "BrokerInstanceName" attribute of
    /// this object.
    bsl::string& brokerInstanceName();

    /// Return a reference to the modifiable "BrokerVersion" attribute of this
    /// object.
    int& brokerVersion();

    /// Return a reference to the modifiable "ConfigVersion" attribute of this
    /// object.
    int& configVersion();

    /// Return a reference to the modifiable "EtcDir" attribute of this object.
    bsl::string& etcDir();

    /// Return a reference to the modifiable "HostName" attribute of this
    /// object.
    bsl::string& hostName();

    /// Return a reference to the modifiable "HostTags" attribute of this
    /// object.
    bsl::string& hostTags();

    /// Return a reference to the modifiable "HostDataCenter" attribute of this
    /// object.
    bsl::string& hostDataCenter();

    /// Return a reference to the modifiable "LogsObserverMaxSize" attribute of
    /// this object.
    int& logsObserverMaxSize();

    /// Return a reference to the modifiable "LatencyMonitorDomain" attribute
    /// of this object.
    bsl::string& latencyMonitorDomain();

    /// Return a reference to the modifiable "DispatcherConfig" attribute of
    /// this object.
    DispatcherConfig& dispatcherConfig();

    /// Return a reference to the modifiable "Stats" attribute of this object.
    StatsConfig& stats();

    /// Return a reference to the modifiable "NetworkInterfaces" attribute of
    /// this object.
    NetworkInterfaces& networkInterfaces();

    /// Return a reference to the modifiable "BmqconfConfig" attribute of this
    /// object.
    BmqconfConfig& bmqconfConfig();

    /// Return a reference to the modifiable "Plugins" attribute of this
    /// object.
    Plugins& plugins();

    /// Return a reference to the modifiable "MessagePropertiesV2" attribute of
    /// this object.
    MessagePropertiesV2& messagePropertiesV2();

    /// Return a reference to the modifiable "ConfigureStream" attribute of
    /// this object.
    bool& configureStream();

    /// Return a reference to the modifiable "AdvertiseSubscriptions" attribute
    /// of this object.
    bool& advertiseSubscriptions();

    /// Return a reference to the modifiable "RouteCommandTimeoutMs" attribute
    /// of this object.
    int& routeCommandTimeoutMs();

    /// Return a reference to the modifiable "Authentication" attribute of this
    /// object.
    AuthenticatorConfig& authentication();

    /// Return a reference to the modifiable "Authorization" attribute of this
    /// object.
    AuthorizerConfig& authorization();

    /// Return a reference to the modifiable "TlsConfig" attribute of this
    /// object.
    bdlb::NullableValue<TlsConfig>& tlsConfig();

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
    /// "BrokerInstanceName" attribute of this object.
    const bsl::string& brokerInstanceName() const;

    /// Return the value of the "BrokerVersion" attribute of this object.
    int brokerVersion() const;

    /// Return the value of the "ConfigVersion" attribute of this object.
    int configVersion() const;

    /// Return a reference offering non-modifiable access to the "EtcDir"
    /// attribute of this object.
    const bsl::string& etcDir() const;

    /// Return a reference offering non-modifiable access to the "HostName"
    /// attribute of this object.
    const bsl::string& hostName() const;

    /// Return a reference offering non-modifiable access to the "HostTags"
    /// attribute of this object.
    const bsl::string& hostTags() const;

    /// Return a reference offering non-modifiable access to the
    /// "HostDataCenter" attribute of this object.
    const bsl::string& hostDataCenter() const;

    /// Return the value of the "LogsObserverMaxSize" attribute of this object.
    int logsObserverMaxSize() const;

    /// Return a reference offering non-modifiable access to the
    /// "LatencyMonitorDomain" attribute of this object.
    const bsl::string& latencyMonitorDomain() const;

    /// Return a reference offering non-modifiable access to the
    /// "DispatcherConfig" attribute of this object.
    const DispatcherConfig& dispatcherConfig() const;

    /// Return a reference offering non-modifiable access to the "Stats"
    /// attribute of this object.
    const StatsConfig& stats() const;

    /// Return a reference offering non-modifiable access to the
    /// "NetworkInterfaces" attribute of this object.
    const NetworkInterfaces& networkInterfaces() const;

    /// Return a reference offering non-modifiable access to the
    /// "BmqconfConfig" attribute of this object.
    const BmqconfConfig& bmqconfConfig() const;

    /// Return a reference offering non-modifiable access to the "Plugins"
    /// attribute of this object.
    const Plugins& plugins() const;

    /// Return a reference offering non-modifiable access to the
    /// "MessagePropertiesV2" attribute of this object.
    const MessagePropertiesV2& messagePropertiesV2() const;

    /// Return the value of the "ConfigureStream" attribute of this object.
    bool configureStream() const;

    /// Return the value of the "AdvertiseSubscriptions" attribute of this
    /// object.
    bool advertiseSubscriptions() const;

    /// Return the value of the "RouteCommandTimeoutMs" attribute of this
    /// object.
    int routeCommandTimeoutMs() const;

    /// Return a reference offering non-modifiable access to the
    /// "Authentication" attribute of this object.
    const AuthenticatorConfig& authentication() const;

    /// Return a reference offering non-modifiable access to the
    /// "Authorization" attribute of this object.
    const AuthorizerConfig& authorization() const;

    /// Return a reference offering non-modifiable access to the "TlsConfig"
    /// attribute of this object.
    const bdlb::NullableValue<TlsConfig>& tlsConfig() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const AppConfig& lhs, const AppConfig& rhs)
    {
        return lhs.isEqualTo(rhs);
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const AppConfig& lhs, const AppConfig& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream& stream, const AppConfig& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `AppConfig`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const AppConfig& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(mqbcfg::AppConfig);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::AppConfig> : bsl::true_type {};

namespace mqbcfg {

// ========================
// class ClustersDefinition
// ========================

/// Top level type representing the configuration for all clusters.
/// myClusters.................: definition of the clusters the current machine
/// is part of (if any); empty means this broker does not belong to any cluster
/// myVirtualClusters..........: information about all the virtual clusters the
/// current machine is considered to belong to (if any)
/// proxyClusters..............: array of cluster proxy definition
class ClustersDefinition {
    // INSTANCE DATA

    bsl::vector<VirtualClusterInformation> d_myVirtualClusters;
    bsl::vector<ClusterProxyDefinition>    d_proxyClusters;
    bsl::vector<ClusterDefinition>         d_myClusters;

    // PRIVATE ACCESSORS

    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES

    enum {
        ATTRIBUTE_ID_MY_CLUSTERS         = 0,
        ATTRIBUTE_ID_MY_VIRTUAL_CLUSTERS = 1,
        ATTRIBUTE_ID_PROXY_CLUSTERS      = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_MY_CLUSTERS         = 0,
        ATTRIBUTE_INDEX_MY_VIRTUAL_CLUSTERS = 1,
        ATTRIBUTE_INDEX_PROXY_CLUSTERS      = 2
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

    /// Create an object of type `ClustersDefinition` having the default value.
    ///  Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit ClustersDefinition(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `ClustersDefinition` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    ClustersDefinition(const ClustersDefinition& original,
                       bslma::Allocator*         basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `ClustersDefinition` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    ClustersDefinition(ClustersDefinition&& original) noexcept;

    /// Create an object of type `ClustersDefinition` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    ClustersDefinition(ClustersDefinition&& original,
                       bslma::Allocator*    basicAllocator);
#endif

    /// Destroy this object.
    ~ClustersDefinition();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ClustersDefinition& operator=(const ClustersDefinition& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    ClustersDefinition& operator=(ClustersDefinition&& rhs);
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

    /// Return a reference to the modifiable "MyClusters" attribute of this
    /// object.
    bsl::vector<ClusterDefinition>& myClusters();

    /// Return a reference to the modifiable "MyVirtualClusters" attribute of
    /// this object.
    bsl::vector<VirtualClusterInformation>& myVirtualClusters();

    /// Return a reference to the modifiable "ProxyClusters" attribute of this
    /// object.
    bsl::vector<ClusterProxyDefinition>& proxyClusters();

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

    /// Return a reference offering non-modifiable access to the "MyClusters"
    /// attribute of this object.
    const bsl::vector<ClusterDefinition>& myClusters() const;

    /// Return a reference offering non-modifiable access to the
    /// "MyVirtualClusters" attribute of this object.
    const bsl::vector<VirtualClusterInformation>& myVirtualClusters() const;

    /// Return a reference offering non-modifiable access to the
    /// "ProxyClusters" attribute of this object.
    const bsl::vector<ClusterProxyDefinition>& proxyClusters() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const ClustersDefinition& lhs,
                           const ClustersDefinition& rhs)
    {
        return lhs.myClusters() == rhs.myClusters() &&
               lhs.myVirtualClusters() == rhs.myVirtualClusters() &&
               lhs.proxyClusters() == rhs.proxyClusters();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const ClustersDefinition& lhs,
                           const ClustersDefinition& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const ClustersDefinition& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `ClustersDefinition`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const ClustersDefinition& object)
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::ClustersDefinition);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::ClustersDefinition>
: bsl::true_type {};

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

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `Configuration` having the default value.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit Configuration(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `Configuration` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    Configuration(const Configuration& original,
                  bslma::Allocator*    basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `Configuration` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    Configuration(Configuration&& original) noexcept;

    /// Create an object of type `Configuration` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.  Use
    /// the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    Configuration(Configuration&& original, bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~Configuration();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    Configuration& operator=(const Configuration& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.  After
    /// performing this action, the `rhs` object will be left in a valid, but
    /// unspecified state.
    Configuration& operator=(Configuration&& rhs);
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

    /// Return a reference to the modifiable "TaskConfig" attribute of this
    /// object.
    TaskConfig& taskConfig();

    /// Return a reference to the modifiable "AppConfig" attribute of this
    /// object.
    AppConfig& appConfig();

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

    /// Return a reference offering non-modifiable access to the "TaskConfig"
    /// attribute of this object.
    const TaskConfig& taskConfig() const;

    /// Return a reference offering non-modifiable access to the "AppConfig"
    /// attribute of this object.
    const AppConfig& appConfig() const;

    // HIDDEN FRIENDS

    /// Return `true` if the specified `lhs` and `rhs` attribute objects have
    /// the same value, and `false` otherwise.  Two attribute objects have the
    /// same value if each respective attribute has the same value.
    friend bool operator==(const Configuration& lhs, const Configuration& rhs)
    {
        return lhs.taskConfig() == rhs.taskConfig() &&
               lhs.appConfig() == rhs.appConfig();
    }

    /// Return `true` if the specified `lhs` and `rhs` objects do not have the
    /// same values, as determined by `operator==`, and `false` otherwise.
    friend bool operator!=(const Configuration& lhs, const Configuration& rhs)
    {
        return !(lhs == rhs);
    }

    /// Format the specified `rhs` to the specified output `stream` and return
    /// a reference to the modifiable `stream`.
    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const Configuration& rhs)
    {
        return rhs.print(stream, 0, -1);
    }

    /// Pass the specified `object` to the specified `hashAlg`.  This function
    /// integrates with the `bslh` modular hashing system and effectively
    /// provides a `bsl::hash` specialization for `Configuration`.
    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&    hashAlg,
                           const Configuration& object)
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.taskConfig());
        hashAppend(hashAlg, object.appConfig());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mqbcfg::Configuration);
template <>
struct bdlat_UsesDefaultValueFlag<mqbcfg::Configuration> : bsl::true_type {};

// ============================================================================
//                          INLINE DEFINITIONS
// ============================================================================

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

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void ClusterAttributes::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->isCSLModeEnabled());
    hashAppend(hashAlgorithm, this->isFSMWorkflow());
    hashAppend(hashAlgorithm, this->doesFSMwriteQLIST());
    hashAppend(hashAlgorithm, this->clusterFsmWatchdogTimeoutSec());
    hashAppend(hashAlgorithm, this->clusterFsmWatchdogNumRetries());
    hashAppend(hashAlgorithm, this->partitionFsmWatchdogTimeoutSec());
    hashAppend(hashAlgorithm, this->partitionFsmWatchdogNumRetries());
}

inline bool ClusterAttributes::isEqualTo(const ClusterAttributes& rhs) const
{
    return this->isCSLModeEnabled() == rhs.isCSLModeEnabled() &&
           this->isFSMWorkflow() == rhs.isFSMWorkflow() &&
           this->doesFSMwriteQLIST() == rhs.doesFSMwriteQLIST() &&
           this->clusterFsmWatchdogTimeoutSec() ==
               rhs.clusterFsmWatchdogTimeoutSec() &&
           this->clusterFsmWatchdogNumRetries() ==
               rhs.clusterFsmWatchdogNumRetries() &&
           this->partitionFsmWatchdogTimeoutSec() ==
               rhs.partitionFsmWatchdogTimeoutSec() &&
           this->partitionFsmWatchdogNumRetries() ==
               rhs.partitionFsmWatchdogNumRetries();
}

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

    ret = manipulator(
        &d_doesFSMwriteQLIST,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOES_F_S_MWRITE_Q_L_I_S_T]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_clusterFsmWatchdogTimeoutSec,
                      ATTRIBUTE_INFO_ARRAY
                          [ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_TIMEOUT_SEC]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_clusterFsmWatchdogNumRetries,
                      ATTRIBUTE_INFO_ARRAY
                          [ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_NUM_RETRIES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_partitionFsmWatchdogTimeoutSec,
        ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_TIMEOUT_SEC]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_partitionFsmWatchdogNumRetries,
        ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_NUM_RETRIES]);
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
    case ATTRIBUTE_ID_DOES_F_S_MWRITE_Q_L_I_S_T: {
        return manipulator(
            &d_doesFSMwriteQLIST,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOES_F_S_MWRITE_Q_L_I_S_T]);
    }
    case ATTRIBUTE_ID_CLUSTER_FSM_WATCHDOG_TIMEOUT_SEC: {
        return manipulator(
            &d_clusterFsmWatchdogTimeoutSec,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_TIMEOUT_SEC]);
    }
    case ATTRIBUTE_ID_CLUSTER_FSM_WATCHDOG_NUM_RETRIES: {
        return manipulator(
            &d_clusterFsmWatchdogNumRetries,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_NUM_RETRIES]);
    }
    case ATTRIBUTE_ID_PARTITION_FSM_WATCHDOG_TIMEOUT_SEC: {
        return manipulator(
            &d_partitionFsmWatchdogTimeoutSec,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_TIMEOUT_SEC]);
    }
    case ATTRIBUTE_ID_PARTITION_FSM_WATCHDOG_NUM_RETRIES: {
        return manipulator(
            &d_partitionFsmWatchdogNumRetries,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_NUM_RETRIES]);
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

inline bool& ClusterAttributes::doesFSMwriteQLIST()
{
    return d_doesFSMwriteQLIST;
}

inline int& ClusterAttributes::clusterFsmWatchdogTimeoutSec()
{
    return d_clusterFsmWatchdogTimeoutSec;
}

inline int& ClusterAttributes::clusterFsmWatchdogNumRetries()
{
    return d_clusterFsmWatchdogNumRetries;
}

inline int& ClusterAttributes::partitionFsmWatchdogTimeoutSec()
{
    return d_partitionFsmWatchdogTimeoutSec;
}

inline int& ClusterAttributes::partitionFsmWatchdogNumRetries()
{
    return d_partitionFsmWatchdogNumRetries;
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

    ret = accessor(
        d_doesFSMwriteQLIST,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOES_F_S_MWRITE_Q_L_I_S_T]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_clusterFsmWatchdogTimeoutSec,
                   ATTRIBUTE_INFO_ARRAY
                       [ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_TIMEOUT_SEC]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_clusterFsmWatchdogNumRetries,
                   ATTRIBUTE_INFO_ARRAY
                       [ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_NUM_RETRIES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_partitionFsmWatchdogTimeoutSec,
                   ATTRIBUTE_INFO_ARRAY
                       [ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_TIMEOUT_SEC]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_partitionFsmWatchdogNumRetries,
                   ATTRIBUTE_INFO_ARRAY
                       [ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_NUM_RETRIES]);
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
    case ATTRIBUTE_ID_DOES_F_S_MWRITE_Q_L_I_S_T: {
        return accessor(
            d_doesFSMwriteQLIST,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOES_F_S_MWRITE_Q_L_I_S_T]);
    }
    case ATTRIBUTE_ID_CLUSTER_FSM_WATCHDOG_TIMEOUT_SEC: {
        return accessor(
            d_clusterFsmWatchdogTimeoutSec,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_TIMEOUT_SEC]);
    }
    case ATTRIBUTE_ID_CLUSTER_FSM_WATCHDOG_NUM_RETRIES: {
        return accessor(
            d_clusterFsmWatchdogNumRetries,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_CLUSTER_FSM_WATCHDOG_NUM_RETRIES]);
    }
    case ATTRIBUTE_ID_PARTITION_FSM_WATCHDOG_TIMEOUT_SEC: {
        return accessor(
            d_partitionFsmWatchdogTimeoutSec,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_TIMEOUT_SEC]);
    }
    case ATTRIBUTE_ID_PARTITION_FSM_WATCHDOG_NUM_RETRIES: {
        return accessor(
            d_partitionFsmWatchdogNumRetries,
            ATTRIBUTE_INFO_ARRAY
                [ATTRIBUTE_INDEX_PARTITION_FSM_WATCHDOG_NUM_RETRIES]);
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

inline bool ClusterAttributes::doesFSMwriteQLIST() const
{
    return d_doesFSMwriteQLIST;
}

inline int ClusterAttributes::clusterFsmWatchdogTimeoutSec() const
{
    return d_clusterFsmWatchdogTimeoutSec;
}

inline int ClusterAttributes::clusterFsmWatchdogNumRetries() const
{
    return d_clusterFsmWatchdogNumRetries;
}

inline int ClusterAttributes::partitionFsmWatchdogTimeoutSec() const
{
    return d_partitionFsmWatchdogTimeoutSec;
}

inline int ClusterAttributes::partitionFsmWatchdogNumRetries() const
{
    return d_partitionFsmWatchdogNumRetries;
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

// ----------------
// class Credential
// ----------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Credential::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_mechanism,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MECHANISM]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_identity,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IDENTITY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Credential::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MECHANISM: {
        return manipulator(&d_mechanism,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MECHANISM]);
    }
    case ATTRIBUTE_ID_IDENTITY: {
        return manipulator(&d_identity,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IDENTITY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Credential::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& Credential::mechanism()
{
    return d_mechanism;
}

inline bsl::string& Credential::identity()
{
    return d_identity;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Credential::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_mechanism,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MECHANISM]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_identity, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IDENTITY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Credential::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MECHANISM: {
        return accessor(d_mechanism,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MECHANISM]);
    }
    case ATTRIBUTE_ID_IDENTITY: {
        return accessor(d_identity,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IDENTITY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Credential::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& Credential::mechanism() const
{
    return d_mechanism;
}

inline const bsl::string& Credential::identity() const
{
    return d_identity;
}

// --------------
// class Disallow
// --------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Disallow::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int Disallow::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Disallow::manipulateAttribute(t_MANIPULATOR& manipulator,
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
int Disallow::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int Disallow::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Disallow::accessAttribute(t_ACCESSOR& accessor,
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

inline unsigned int& ElectorConfig::quorum()
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

inline unsigned int ElectorConfig::quorum() const
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

// -------------------
// class LogDumpConfig
// -------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void LogDumpConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->recordBufferSize());
    hashAppend(hashAlgorithm, this->recordingLevel());
    hashAppend(hashAlgorithm, this->triggerLevel());
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int LogDumpConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(
        &d_recordBufferSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RECORD_BUFFER_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_recordingLevel,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RECORDING_LEVEL]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_triggerLevel,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TRIGGER_LEVEL]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int LogDumpConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_RECORD_BUFFER_SIZE: {
        return manipulator(
            &d_recordBufferSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RECORD_BUFFER_SIZE]);
    }
    case ATTRIBUTE_ID_RECORDING_LEVEL: {
        return manipulator(
            &d_recordingLevel,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RECORDING_LEVEL]);
    }
    case ATTRIBUTE_ID_TRIGGER_LEVEL: {
        return manipulator(
            &d_triggerLevel,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TRIGGER_LEVEL]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int LogDumpConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline int& LogDumpConfig::recordBufferSize()
{
    return d_recordBufferSize;
}

inline bsl::string& LogDumpConfig::recordingLevel()
{
    return d_recordingLevel;
}

inline bsl::string& LogDumpConfig::triggerLevel()
{
    return d_triggerLevel;
}

// ACCESSORS
template <typename t_ACCESSOR>
int LogDumpConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_recordBufferSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RECORD_BUFFER_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_recordingLevel,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RECORDING_LEVEL]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_triggerLevel,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TRIGGER_LEVEL]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int LogDumpConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_RECORD_BUFFER_SIZE: {
        return accessor(
            d_recordBufferSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RECORD_BUFFER_SIZE]);
    }
    case ATTRIBUTE_ID_RECORDING_LEVEL: {
        return accessor(d_recordingLevel,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RECORDING_LEVEL]);
    }
    case ATTRIBUTE_ID_TRIGGER_LEVEL: {
        return accessor(d_triggerLevel,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TRIGGER_LEVEL]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int LogDumpConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline int LogDumpConfig::recordBufferSize() const
{
    return d_recordBufferSize;
}

inline const bsl::string& LogDumpConfig::recordingLevel() const
{
    return d_recordingLevel;
}

inline const bsl::string& LogDumpConfig::triggerLevel() const
{
    return d_triggerLevel;
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

// ------------------------
// class PluginSettingValue
// ------------------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void PluginSettingValue::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef PluginSettingValue Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_BOOL_VAL:
        hashAppend(hashAlgorithm, this->boolVal());
        break;
    case Class::SELECTION_ID_INT_VAL:
        hashAppend(hashAlgorithm, this->intVal());
        break;
    case Class::SELECTION_ID_LONG_VAL:
        hashAppend(hashAlgorithm, this->longVal());
        break;
    case Class::SELECTION_ID_DOUBLE_VAL:
        hashAppend(hashAlgorithm, this->doubleVal());
        break;
    case Class::SELECTION_ID_STRING_VAL:
        hashAppend(hashAlgorithm, this->stringVal());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool PluginSettingValue::isEqualTo(const PluginSettingValue& rhs) const
{
    typedef PluginSettingValue Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_BOOL_VAL:
            return this->boolVal() == rhs.boolVal();
        case Class::SELECTION_ID_INT_VAL:
            return this->intVal() == rhs.intVal();
        case Class::SELECTION_ID_LONG_VAL:
            return this->longVal() == rhs.longVal();
        case Class::SELECTION_ID_DOUBLE_VAL:
            return this->doubleVal() == rhs.doubleVal();
        case Class::SELECTION_ID_STRING_VAL:
            return this->stringVal() == rhs.stringVal();
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
inline PluginSettingValue::PluginSettingValue(bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline PluginSettingValue::~PluginSettingValue()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int PluginSettingValue::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case PluginSettingValue::SELECTION_ID_BOOL_VAL:
        return manipulator(&d_boolVal.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_BOOL_VAL]);
    case PluginSettingValue::SELECTION_ID_INT_VAL:
        return manipulator(&d_intVal.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_INT_VAL]);
    case PluginSettingValue::SELECTION_ID_LONG_VAL:
        return manipulator(&d_longVal.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_LONG_VAL]);
    case PluginSettingValue::SELECTION_ID_DOUBLE_VAL:
        return manipulator(&d_doubleVal.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_DOUBLE_VAL]);
    case PluginSettingValue::SELECTION_ID_STRING_VAL:
        return manipulator(&d_stringVal.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_STRING_VAL]);
    default:
        BSLS_ASSERT(PluginSettingValue::SELECTION_ID_UNDEFINED ==
                    d_selectionId);
        return -1;
    }
}

inline bool& PluginSettingValue::boolVal()
{
    BSLS_ASSERT(SELECTION_ID_BOOL_VAL == d_selectionId);
    return d_boolVal.object();
}

inline int& PluginSettingValue::intVal()
{
    BSLS_ASSERT(SELECTION_ID_INT_VAL == d_selectionId);
    return d_intVal.object();
}

inline bsls::Types::Int64& PluginSettingValue::longVal()
{
    BSLS_ASSERT(SELECTION_ID_LONG_VAL == d_selectionId);
    return d_longVal.object();
}

inline double& PluginSettingValue::doubleVal()
{
    BSLS_ASSERT(SELECTION_ID_DOUBLE_VAL == d_selectionId);
    return d_doubleVal.object();
}

inline bsl::string& PluginSettingValue::stringVal()
{
    BSLS_ASSERT(SELECTION_ID_STRING_VAL == d_selectionId);
    return d_stringVal.object();
}

// ACCESSORS
inline int PluginSettingValue::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int PluginSettingValue::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_BOOL_VAL:
        return accessor(d_boolVal.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_BOOL_VAL]);
    case SELECTION_ID_INT_VAL:
        return accessor(d_intVal.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_INT_VAL]);
    case SELECTION_ID_LONG_VAL:
        return accessor(d_longVal.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_LONG_VAL]);
    case SELECTION_ID_DOUBLE_VAL:
        return accessor(d_doubleVal.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_DOUBLE_VAL]);
    case SELECTION_ID_STRING_VAL:
        return accessor(d_stringVal.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_STRING_VAL]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const bool& PluginSettingValue::boolVal() const
{
    BSLS_ASSERT(SELECTION_ID_BOOL_VAL == d_selectionId);
    return d_boolVal.object();
}

inline const int& PluginSettingValue::intVal() const
{
    BSLS_ASSERT(SELECTION_ID_INT_VAL == d_selectionId);
    return d_intVal.object();
}

inline const bsls::Types::Int64& PluginSettingValue::longVal() const
{
    BSLS_ASSERT(SELECTION_ID_LONG_VAL == d_selectionId);
    return d_longVal.object();
}

inline const double& PluginSettingValue::doubleVal() const
{
    BSLS_ASSERT(SELECTION_ID_DOUBLE_VAL == d_selectionId);
    return d_doubleVal.object();
}

inline const bsl::string& PluginSettingValue::stringVal() const
{
    BSLS_ASSERT(SELECTION_ID_STRING_VAL == d_selectionId);
    return d_stringVal.object();
}

inline bool PluginSettingValue::isBoolValValue() const
{
    return SELECTION_ID_BOOL_VAL == d_selectionId;
}

inline bool PluginSettingValue::isIntValValue() const
{
    return SELECTION_ID_INT_VAL == d_selectionId;
}

inline bool PluginSettingValue::isLongValValue() const
{
    return SELECTION_ID_LONG_VAL == d_selectionId;
}

inline bool PluginSettingValue::isDoubleValValue() const
{
    return SELECTION_ID_DOUBLE_VAL == d_selectionId;
}

inline bool PluginSettingValue::isStringValValue() const
{
    return SELECTION_ID_STRING_VAL == d_selectionId;
}

inline bool PluginSettingValue::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
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

// --------------------------------
// class StatsPrinterEncodingFormat
// --------------------------------

// CLASS METHODS
inline int StatsPrinterEncodingFormat::fromString(Value*             result,
                                                  const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream&
StatsPrinterEncodingFormat::print(bsl::ostream&                     stream,
                                  StatsPrinterEncodingFormat::Value value)
{
    return stream << toString(value);
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

// --------------------------
// class TcpInterfaceListener
// --------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void TcpInterfaceListener::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->name());
    hashAppend(hashAlgorithm, this->address());
    hashAppend(hashAlgorithm, this->port());
    hashAppend(hashAlgorithm, this->tls());
}

inline bool
TcpInterfaceListener::isEqualTo(const TcpInterfaceListener& rhs) const
{
    return this->name() == rhs.name() && this->address() == rhs.address() &&
           this->port() == rhs.port() && this->tls() == rhs.tls();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int TcpInterfaceListener::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_address,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADDRESS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_port, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_tls, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TLS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int TcpInterfaceListener::manipulateAttribute(t_MANIPULATOR& manipulator,
                                              int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_ADDRESS: {
        return manipulator(&d_address,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADDRESS]);
    }
    case ATTRIBUTE_ID_PORT: {
        return manipulator(&d_port,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    }
    case ATTRIBUTE_ID_TLS: {
        return manipulator(&d_tls, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TLS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int TcpInterfaceListener::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& TcpInterfaceListener::name()
{
    return d_name;
}

inline bsl::string& TcpInterfaceListener::address()
{
    return d_address;
}

inline int& TcpInterfaceListener::port()
{
    return d_port;
}

inline bool& TcpInterfaceListener::tls()
{
    return d_tls;
}

// ACCESSORS
template <typename t_ACCESSOR>
int TcpInterfaceListener::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_address, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADDRESS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_port, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_tls, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TLS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int TcpInterfaceListener::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_ADDRESS: {
        return accessor(d_address,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADDRESS]);
    }
    case ATTRIBUTE_ID_PORT: {
        return accessor(d_port, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT]);
    }
    case ATTRIBUTE_ID_TLS: {
        return accessor(d_tls, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TLS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int TcpInterfaceListener::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& TcpInterfaceListener::name() const
{
    return d_name;
}

inline const bsl::string& TcpInterfaceListener::address() const
{
    return d_address;
}

inline int TcpInterfaceListener::port() const
{
    return d_port;
}

inline bool TcpInterfaceListener::tls() const
{
    return d_tls;
}

// ---------------
// class TlsConfig
// ---------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void TlsConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->certificateAuthority());
    hashAppend(hashAlgorithm, this->certificate());
    hashAppend(hashAlgorithm, this->key());
    hashAppend(hashAlgorithm, this->versions());
}

inline bool TlsConfig::isEqualTo(const TlsConfig& rhs) const
{
    return this->certificateAuthority() == rhs.certificateAuthority() &&
           this->certificate() == rhs.certificate() &&
           this->key() == rhs.key() && this->versions() == rhs.versions();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int TlsConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(
        &d_certificateAuthority,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CERTIFICATE_AUTHORITY]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_certificate,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CERTIFICATE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_versions,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int TlsConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CERTIFICATE_AUTHORITY: {
        return manipulator(
            &d_certificateAuthority,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CERTIFICATE_AUTHORITY]);
    }
    case ATTRIBUTE_ID_CERTIFICATE: {
        return manipulator(&d_certificate,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CERTIFICATE]);
    }
    case ATTRIBUTE_ID_KEY: {
        return manipulator(&d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    }
    case ATTRIBUTE_ID_VERSIONS: {
        return manipulator(&d_versions,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int TlsConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& TlsConfig::certificateAuthority()
{
    return d_certificateAuthority;
}

inline bsl::string& TlsConfig::certificate()
{
    return d_certificate;
}

inline bsl::string& TlsConfig::key()
{
    return d_key;
}

inline bsl::string& TlsConfig::versions()
{
    return d_versions;
}

// ACCESSORS
template <typename t_ACCESSOR>
int TlsConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(
        d_certificateAuthority,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CERTIFICATE_AUTHORITY]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_certificate,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CERTIFICATE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_versions, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int TlsConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CERTIFICATE_AUTHORITY: {
        return accessor(
            d_certificateAuthority,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CERTIFICATE_AUTHORITY]);
    }
    case ATTRIBUTE_ID_CERTIFICATE: {
        return accessor(d_certificate,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CERTIFICATE]);
    }
    case ATTRIBUTE_ID_KEY: {
        return accessor(d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    }
    case ATTRIBUTE_ID_VERSIONS: {
        return accessor(d_versions,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int TlsConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& TlsConfig::certificateAuthority() const
{
    return d_certificateAuthority;
}

inline const bsl::string& TlsConfig::certificate() const
{
    return d_certificate;
}

inline const bsl::string& TlsConfig::key() const
{
    return d_key;
}

inline const bsl::string& TlsConfig::versions() const
{
    return d_versions;
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

// -------------------------
// class AnonymousCredential
// -------------------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void AnonymousCredential::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef AnonymousCredential Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_DISALLOW:
        hashAppend(hashAlgorithm, this->disallow());
        break;
    case Class::SELECTION_ID_CREDENTIAL:
        hashAppend(hashAlgorithm, this->credential());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool
AnonymousCredential::isEqualTo(const AnonymousCredential& rhs) const
{
    typedef AnonymousCredential Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_DISALLOW:
            return this->disallow() == rhs.disallow();
        case Class::SELECTION_ID_CREDENTIAL:
            return this->credential() == rhs.credential();
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
inline AnonymousCredential::AnonymousCredential(
    bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline AnonymousCredential::~AnonymousCredential()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int AnonymousCredential::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case AnonymousCredential::SELECTION_ID_DISALLOW:
        return manipulator(&d_disallow.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_DISALLOW]);
    case AnonymousCredential::SELECTION_ID_CREDENTIAL:
        return manipulator(&d_credential.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_CREDENTIAL]);
    default:
        BSLS_ASSERT(AnonymousCredential::SELECTION_ID_UNDEFINED ==
                    d_selectionId);
        return -1;
    }
}

inline Disallow& AnonymousCredential::disallow()
{
    BSLS_ASSERT(SELECTION_ID_DISALLOW == d_selectionId);
    return d_disallow.object();
}

inline Credential& AnonymousCredential::credential()
{
    BSLS_ASSERT(SELECTION_ID_CREDENTIAL == d_selectionId);
    return d_credential.object();
}

// ACCESSORS
inline int AnonymousCredential::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int AnonymousCredential::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_DISALLOW:
        return accessor(d_disallow.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_DISALLOW]);
    case SELECTION_ID_CREDENTIAL:
        return accessor(d_credential.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_CREDENTIAL]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const Disallow& AnonymousCredential::disallow() const
{
    BSLS_ASSERT(SELECTION_ID_DISALLOW == d_selectionId);
    return d_disallow.object();
}

inline const Credential& AnonymousCredential::credential() const
{
    BSLS_ASSERT(SELECTION_ID_CREDENTIAL == d_selectionId);
    return d_credential.object();
}

inline bool AnonymousCredential::isDisallowValue() const
{
    return SELECTION_ID_DISALLOW == d_selectionId;
}

inline bool AnonymousCredential::isCredentialValue() const
{
    return SELECTION_ID_CREDENTIAL == d_selectionId;
}

inline bool AnonymousCredential::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
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
    hashAppend(hashAlgorithm, this->logDump());
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
           this->syslog() == rhs.syslog() && this->logDump() == rhs.logDump();
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

    ret = manipulator(&d_logDump,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_DUMP]);
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
    case ATTRIBUTE_ID_LOG_DUMP: {
        return manipulator(&d_logDump,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_DUMP]);
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

inline LogDumpConfig& LogController::logDump()
{
    return d_logDump;
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

    ret = accessor(d_logDump, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_DUMP]);
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
    case ATTRIBUTE_ID_LOG_DUMP: {
        return accessor(d_logDump,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_DUMP]);
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

inline const LogDumpConfig& LogController::logDump() const
{
    return d_logDump;
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
    hashAppend(hashAlgorithm, this->maxCSLFileSize());
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
           this->maxCSLFileSize() == rhs.maxCSLFileSize() &&
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

    ret = manipulator(
        &d_maxCSLFileSize,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_C_S_L_FILE_SIZE]);
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
    case ATTRIBUTE_ID_MAX_C_S_L_FILE_SIZE: {
        return manipulator(
            &d_maxCSLFileSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_C_S_L_FILE_SIZE]);
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

inline bsls::Types::Uint64& PartitionConfig::maxCSLFileSize()
{
    return d_maxCSLFileSize;
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

    ret = accessor(d_maxCSLFileSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_C_S_L_FILE_SIZE]);
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
    case ATTRIBUTE_ID_MAX_C_S_L_FILE_SIZE: {
        return accessor(
            d_maxCSLFileSize,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_C_S_L_FILE_SIZE]);
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

inline bsls::Types::Uint64 PartitionConfig::maxCSLFileSize() const
{
    return d_maxCSLFileSize;
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

// ---------------------------
// class PluginSettingKeyValue
// ---------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int PluginSettingKeyValue::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_value, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int PluginSettingKeyValue::manipulateAttribute(t_MANIPULATOR& manipulator,
                                               int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_KEY: {
        return manipulator(&d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    }
    case ATTRIBUTE_ID_VALUE: {
        return manipulator(&d_value,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int PluginSettingKeyValue::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& PluginSettingKeyValue::key()
{
    return d_key;
}

inline PluginSettingValue& PluginSettingKeyValue::value()
{
    return d_value;
}

// ACCESSORS
template <typename t_ACCESSOR>
int PluginSettingKeyValue::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_value, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int PluginSettingKeyValue::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_KEY: {
        return accessor(d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    }
    case ATTRIBUTE_ID_VALUE: {
        return accessor(d_value, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int PluginSettingKeyValue::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& PluginSettingKeyValue::key() const
{
    return d_key;
}

inline const PluginSettingValue& PluginSettingKeyValue::value() const
{
    return d_value;
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
    hashAppend(hashAlgorithm, this->encoding());
}

inline bool StatsPrinterConfig::isEqualTo(const StatsPrinterConfig& rhs) const
{
    return this->printInterval() == rhs.printInterval() &&
           this->file() == rhs.file() &&
           this->maxAgeDays() == rhs.maxAgeDays() &&
           this->rotateBytes() == rhs.rotateBytes() &&
           this->rotateDays() == rhs.rotateDays() &&
           this->encoding() == rhs.encoding();
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

    ret = manipulator(&d_encoding,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENCODING]);
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
    case ATTRIBUTE_ID_ENCODING: {
        return manipulator(&d_encoding,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENCODING]);
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

inline StatsPrinterEncodingFormat::Value& StatsPrinterConfig::encoding()
{
    return d_encoding;
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

    ret = accessor(d_encoding, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENCODING]);
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
    case ATTRIBUTE_ID_ENCODING: {
        return accessor(d_encoding,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENCODING]);
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

inline StatsPrinterEncodingFormat::Value StatsPrinterConfig::encoding() const
{
    return d_encoding;
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
    hashAppend(hashAlgorithm, this->listeners());
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
           this->heartbeatIntervalMs() == rhs.heartbeatIntervalMs() &&
           this->listeners() == rhs.listeners();
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

    ret = manipulator(&d_listeners,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LISTENERS]);
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
    case ATTRIBUTE_ID_LISTENERS: {
        return manipulator(&d_listeners,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LISTENERS]);
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

inline bsl::vector<TcpInterfaceListener>& TcpInterfaceConfig::listeners()
{
    return d_listeners;
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

    ret = accessor(d_listeners,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LISTENERS]);
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
    case ATTRIBUTE_ID_LISTENERS: {
        return accessor(d_listeners,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LISTENERS]);
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

inline const bsl::vector<TcpInterfaceListener>&
TcpInterfaceConfig::listeners() const
{
    return d_listeners;
}

// -------------------------------
// class AuthenticatorPluginConfig
// -------------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int AuthenticatorPluginConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_settings,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int AuthenticatorPluginConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                   int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_SETTINGS: {
        return manipulator(&d_settings,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int AuthenticatorPluginConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& AuthenticatorPluginConfig::name()
{
    return d_name;
}

inline bsl::vector<PluginSettingKeyValue>&
AuthenticatorPluginConfig::settings()
{
    return d_settings;
}

// ACCESSORS
template <typename t_ACCESSOR>
int AuthenticatorPluginConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_settings, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int AuthenticatorPluginConfig::accessAttribute(t_ACCESSOR& accessor,
                                               int         id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_SETTINGS: {
        return accessor(d_settings,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int AuthenticatorPluginConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& AuthenticatorPluginConfig::name() const
{
    return d_name;
}

inline const bsl::vector<PluginSettingKeyValue>&
AuthenticatorPluginConfig::settings() const
{
    return d_settings;
}

// ----------------------------
// class AuthorizerPluginConfig
// ----------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int AuthorizerPluginConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_settings,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int AuthorizerPluginConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_SETTINGS: {
        return manipulator(&d_settings,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int AuthorizerPluginConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& AuthorizerPluginConfig::name()
{
    return d_name;
}

inline bsl::vector<PluginSettingKeyValue>& AuthorizerPluginConfig::settings()
{
    return d_settings;
}

// ACCESSORS
template <typename t_ACCESSOR>
int AuthorizerPluginConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_settings, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int AuthorizerPluginConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_SETTINGS: {
        return accessor(d_settings,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int AuthorizerPluginConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& AuthorizerPluginConfig::name() const
{
    return d_name;
}

inline const bsl::vector<PluginSettingKeyValue>&
AuthorizerPluginConfig::settings() const
{
    return d_settings;
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

// ------------------------------
// class CredentialProviderConfig
// ------------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int CredentialProviderConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_settings,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int CredentialProviderConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                  int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_SETTINGS: {
        return manipulator(&d_settings,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int CredentialProviderConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::string& CredentialProviderConfig::name()
{
    return d_name;
}

inline bsl::vector<PluginSettingKeyValue>& CredentialProviderConfig::settings()
{
    return d_settings;
}

// ACCESSORS
template <typename t_ACCESSOR>
int CredentialProviderConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_settings, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int CredentialProviderConfig::accessAttribute(t_ACCESSOR& accessor,
                                              int         id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_SETTINGS: {
        return accessor(d_settings,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SETTINGS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int CredentialProviderConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::string& CredentialProviderConfig::name() const
{
    return d_name;
}

inline const bsl::vector<PluginSettingKeyValue>&
CredentialProviderConfig::settings() const
{
    return d_settings;
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
    hashAppend(hashAlgorithm, this->alarmTimeoutMs());
    hashAppend(hashAlgorithm, this->warningTimeoutMs());
}

inline bool DispatcherConfig::isEqualTo(const DispatcherConfig& rhs) const
{
    return this->sessions() == rhs.sessions() &&
           this->queues() == rhs.queues() &&
           this->clusters() == rhs.clusters() &&
           this->alarmTimeoutMs() == rhs.alarmTimeoutMs() &&
           this->warningTimeoutMs() == rhs.warningTimeoutMs();
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

    ret = manipulator(&d_alarmTimeoutMs,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALARM_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_warningTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_WARNING_TIMEOUT_MS]);
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
    case ATTRIBUTE_ID_ALARM_TIMEOUT_MS: {
        return manipulator(
            &d_alarmTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALARM_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_WARNING_TIMEOUT_MS: {
        return manipulator(
            &d_warningTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_WARNING_TIMEOUT_MS]);
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

inline int& DispatcherConfig::alarmTimeoutMs()
{
    return d_alarmTimeoutMs;
}

inline int& DispatcherConfig::warningTimeoutMs()
{
    return d_warningTimeoutMs;
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

    ret = accessor(d_alarmTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALARM_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_warningTimeoutMs,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_WARNING_TIMEOUT_MS]);
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
    case ATTRIBUTE_ID_ALARM_TIMEOUT_MS: {
        return accessor(
            d_alarmTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALARM_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_WARNING_TIMEOUT_MS: {
        return accessor(
            d_warningTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_WARNING_TIMEOUT_MS]);
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

inline int DispatcherConfig::alarmTimeoutMs() const
{
    return d_alarmTimeoutMs;
}

inline int DispatcherConfig::warningTimeoutMs() const
{
    return d_warningTimeoutMs;
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

// -------------------------
// class AuthenticatorConfig
// -------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void AuthenticatorConfig::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->authenticators());
    hashAppend(hashAlgorithm, this->anonymousCredential());
    hashAppend(hashAlgorithm, this->minThreads());
    hashAppend(hashAlgorithm, this->maxThreads());
    hashAppend(hashAlgorithm, this->credentialProvider());
}

inline bool
AuthenticatorConfig::isEqualTo(const AuthenticatorConfig& rhs) const
{
    return this->authenticators() == rhs.authenticators() &&
           this->anonymousCredential() == rhs.anonymousCredential() &&
           this->minThreads() == rhs.minThreads() &&
           this->maxThreads() == rhs.maxThreads() &&
           this->credentialProvider() == rhs.credentialProvider();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int AuthenticatorConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_authenticators,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHENTICATORS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_anonymousCredential,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ANONYMOUS_CREDENTIAL]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_minThreads,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_THREADS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxThreads,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_THREADS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_credentialProvider,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CREDENTIAL_PROVIDER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int AuthenticatorConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
                                             int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_AUTHENTICATORS: {
        return manipulator(
            &d_authenticators,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHENTICATORS]);
    }
    case ATTRIBUTE_ID_ANONYMOUS_CREDENTIAL: {
        return manipulator(
            &d_anonymousCredential,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ANONYMOUS_CREDENTIAL]);
    }
    case ATTRIBUTE_ID_MIN_THREADS: {
        return manipulator(&d_minThreads,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_THREADS]);
    }
    case ATTRIBUTE_ID_MAX_THREADS: {
        return manipulator(&d_maxThreads,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_THREADS]);
    }
    case ATTRIBUTE_ID_CREDENTIAL_PROVIDER: {
        return manipulator(
            &d_credentialProvider,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CREDENTIAL_PROVIDER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int AuthenticatorConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bsl::vector<AuthenticatorPluginConfig>&
AuthenticatorConfig::authenticators()
{
    return d_authenticators;
}

inline bdlb::NullableValue<AnonymousCredential>&
AuthenticatorConfig::anonymousCredential()
{
    return d_anonymousCredential;
}

inline int& AuthenticatorConfig::minThreads()
{
    return d_minThreads;
}

inline int& AuthenticatorConfig::maxThreads()
{
    return d_maxThreads;
}

inline bdlb::NullableValue<CredentialProviderConfig>&
AuthenticatorConfig::credentialProvider()
{
    return d_credentialProvider;
}

// ACCESSORS
template <typename t_ACCESSOR>
int AuthenticatorConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_authenticators,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHENTICATORS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_anonymousCredential,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ANONYMOUS_CREDENTIAL]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_minThreads,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_THREADS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxThreads,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_THREADS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_credentialProvider,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CREDENTIAL_PROVIDER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int AuthenticatorConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_AUTHENTICATORS: {
        return accessor(d_authenticators,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHENTICATORS]);
    }
    case ATTRIBUTE_ID_ANONYMOUS_CREDENTIAL: {
        return accessor(
            d_anonymousCredential,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ANONYMOUS_CREDENTIAL]);
    }
    case ATTRIBUTE_ID_MIN_THREADS: {
        return accessor(d_minThreads,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_THREADS]);
    }
    case ATTRIBUTE_ID_MAX_THREADS: {
        return accessor(d_maxThreads,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_THREADS]);
    }
    case ATTRIBUTE_ID_CREDENTIAL_PROVIDER: {
        return accessor(
            d_credentialProvider,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CREDENTIAL_PROVIDER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int AuthenticatorConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const bsl::vector<AuthenticatorPluginConfig>&
AuthenticatorConfig::authenticators() const
{
    return d_authenticators;
}

inline const bdlb::NullableValue<AnonymousCredential>&
AuthenticatorConfig::anonymousCredential() const
{
    return d_anonymousCredential;
}

inline int AuthenticatorConfig::minThreads() const
{
    return d_minThreads;
}

inline int AuthenticatorConfig::maxThreads() const
{
    return d_maxThreads;
}

inline const bdlb::NullableValue<CredentialProviderConfig>&
AuthenticatorConfig::credentialProvider() const
{
    return d_credentialProvider;
}

// ----------------------
// class AuthorizerConfig
// ----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int AuthorizerConfig::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_authorizer,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHORIZER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int AuthorizerConfig::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_AUTHORIZER: {
        return manipulator(&d_authorizer,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHORIZER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int AuthorizerConfig::manipulateAttribute(t_MANIPULATOR& manipulator,
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

inline bdlb::NullableValue<AuthorizerPluginConfig>&
AuthorizerConfig::authorizer()
{
    return d_authorizer;
}

// ACCESSORS
template <typename t_ACCESSOR>
int AuthorizerConfig::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_authorizer,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHORIZER]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int AuthorizerConfig::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_AUTHORIZER: {
        return accessor(d_authorizer,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHORIZER]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int AuthorizerConfig::accessAttribute(t_ACCESSOR& accessor,
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

inline const bdlb::NullableValue<AuthorizerPluginConfig>&
AuthorizerConfig::authorizer() const
{
    return d_authorizer;
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
    hashAppend(hashAlgorithm, this->plugins());
    hashAppend(hashAlgorithm, this->printer());
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
    hashAppend(hashAlgorithm, this->routeCommandTimeoutMs());
    hashAppend(hashAlgorithm, this->authentication());
    hashAppend(hashAlgorithm, this->authorization());
    hashAppend(hashAlgorithm, this->tlsConfig());
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
           this->logsObserverMaxSize() == rhs.logsObserverMaxSize() &&
           this->latencyMonitorDomain() == rhs.latencyMonitorDomain() &&
           this->dispatcherConfig() == rhs.dispatcherConfig() &&
           this->stats() == rhs.stats() &&
           this->networkInterfaces() == rhs.networkInterfaces() &&
           this->bmqconfConfig() == rhs.bmqconfConfig() &&
           this->plugins() == rhs.plugins() &&
           this->messagePropertiesV2() == rhs.messagePropertiesV2() &&
           this->configureStream() == rhs.configureStream() &&
           this->advertiseSubscriptions() == rhs.advertiseSubscriptions() &&
           this->routeCommandTimeoutMs() == rhs.routeCommandTimeoutMs() &&
           this->authentication() == rhs.authentication() &&
           this->authorization() == rhs.authorization() &&
           this->tlsConfig() == rhs.tlsConfig();
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

    ret = manipulator(
        &d_routeCommandTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROUTE_COMMAND_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_authentication,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHENTICATION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_authorization,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHORIZATION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_tlsConfig,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TLS_CONFIG]);
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
    case ATTRIBUTE_ID_ROUTE_COMMAND_TIMEOUT_MS: {
        return manipulator(
            &d_routeCommandTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROUTE_COMMAND_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_AUTHENTICATION: {
        return manipulator(
            &d_authentication,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHENTICATION]);
    }
    case ATTRIBUTE_ID_AUTHORIZATION: {
        return manipulator(
            &d_authorization,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHORIZATION]);
    }
    case ATTRIBUTE_ID_TLS_CONFIG: {
        return manipulator(&d_tlsConfig,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TLS_CONFIG]);
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

inline int& AppConfig::routeCommandTimeoutMs()
{
    return d_routeCommandTimeoutMs;
}

inline AuthenticatorConfig& AppConfig::authentication()
{
    return d_authentication;
}

inline AuthorizerConfig& AppConfig::authorization()
{
    return d_authorization;
}

inline bdlb::NullableValue<TlsConfig>& AppConfig::tlsConfig()
{
    return d_tlsConfig;
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

    ret = accessor(
        d_routeCommandTimeoutMs,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROUTE_COMMAND_TIMEOUT_MS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_authentication,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHENTICATION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_authorization,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHORIZATION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_tlsConfig,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TLS_CONFIG]);
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
    case ATTRIBUTE_ID_ROUTE_COMMAND_TIMEOUT_MS: {
        return accessor(
            d_routeCommandTimeoutMs,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROUTE_COMMAND_TIMEOUT_MS]);
    }
    case ATTRIBUTE_ID_AUTHENTICATION: {
        return accessor(d_authentication,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHENTICATION]);
    }
    case ATTRIBUTE_ID_AUTHORIZATION: {
        return accessor(d_authorization,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_AUTHORIZATION]);
    }
    case ATTRIBUTE_ID_TLS_CONFIG: {
        return accessor(d_tlsConfig,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TLS_CONFIG]);
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

inline int AppConfig::routeCommandTimeoutMs() const
{
    return d_routeCommandTimeoutMs;
}

inline const AuthenticatorConfig& AppConfig::authentication() const
{
    return d_authentication;
}

inline const AuthorizerConfig& AppConfig::authorization() const
{
    return d_authorization;
}

inline const bdlb::NullableValue<TlsConfig>& AppConfig::tlsConfig() const
{
    return d_tlsConfig;
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
    hashAppend(hashAlgorithm, this->myVirtualClusters());
    hashAppend(hashAlgorithm, this->proxyClusters());
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

inline bsl::vector<VirtualClusterInformation>&
ClustersDefinition::myVirtualClusters()
{
    return d_myVirtualClusters;
}

inline bsl::vector<ClusterProxyDefinition>& ClustersDefinition::proxyClusters()
{
    return d_proxyClusters;
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
    case ATTRIBUTE_ID_MY_VIRTUAL_CLUSTERS: {
        return accessor(
            d_myVirtualClusters,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_VIRTUAL_CLUSTERS]);
    }
    case ATTRIBUTE_ID_PROXY_CLUSTERS: {
        return accessor(d_proxyClusters,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROXY_CLUSTERS]);
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

// GENERATED BY BLP_BAS_CODEGEN_2026.06.18
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbcfg --msgComponent messages mqbcfg.xsd
