// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbblp_queuestate.h                                                -*-C++-*-
#ifndef INCLUDED_MQBBLP_QUEUESTATE
#define INCLUDED_MQBBLP_QUEUESTATE

/// @file mqbblp_queuestate.h
///
/// @brief Provide a value-semantic type holding the state of a queue.
///
/// @todo Document component.

// MQB

#include <mqbblp_queueengineutil.h>
#include <mqbblp_queuehandlecatalog.h>
#include <mqbcfg_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbstat_queuestats.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_cstring.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace bdlmt {
class FixedThreadPool;
}
namespace mqbcmd {
class QueueState;
}
namespace mqbi {
class Storage;
}
namespace mqbi {
class StorageManager;
}
namespace mqbi {
class Queue;
}

namespace mqbblp {
// ================
// class QueueState
// ================

/// Value-semantic type holding the state of a queue.
class QueueState {
  public:
    // TYPES

    /// `SubQueuesParameters` is an alias for a map of QueueStreamParameters
    /// (subQueueId) -> queueStreamParameters
    ///
    /// * `subQueueId`           : upstream subQueueId
    /// * `queueStreamParameters`: QueueStreamParameters to send upstream
    typedef bsl::unordered_map<unsigned int, bmqp_ctrlmsg::StreamParameters>
        SubQueuesParameters;

    /// `SubQueuesHandleParameters` is an alias for a map of
    /// QueueHandleParameters (appId) -> queueHandleParameters
    ///
    /// * `appId`                : upstream appId
    /// * `queueHandleParameters`: cumulative QueueHandleParameters
    typedef bsl::unordered_map<bsl::string,
                               bmqp_ctrlmsg::QueueHandleParameters>
        SubQueuesHandleParameters;

    typedef bmqc::Array<bsl::shared_ptr<QueueEngineUtil_AppState>,
                        bmqp::Protocol::k_SUBID_ARRAY_STATIC_LEN>
        SubQueues;

  private:
    // DATA

    /// The queue associated to this state.
    mqbi::Queue* d_queue_p;

    /// The URI of the queue associated to this state.
    bmqt::Uri d_uri;

    /// A description of the queue associated to this state.
    bsl::string d_description;

    /// Upstream id of the queue associated to this state.
    unsigned int d_id;

    /// QueueKey of the queue associated to this state.
    mqbu::StorageKey d_key;

    /// Aggregated parameters of all currently opened queueHandles to the queue
    /// associated to this state.
    bmqp_ctrlmsg::QueueHandleParameters d_handleParameters;

    SubQueuesParameters d_subQueuesParametersMap;

    /// Cumulative values per AppId.
    SubQueuesHandleParameters d_subQueuesHandleParameters;

    /// PartitionId affected to the queue associated to this state.
    int d_partitionId;

    /// Domain the queue associated to this state belongs to.
    mqbi::Domain* d_domain_p;

    /// Storage manager to use.
    mqbi::StorageManager* d_storageManager_p;

    const mqbi::ClusterResources d_resources;

    /// Thread pool used for any standalone work that can be offloaded to any
    /// non-dispatcher threads.
    bdlmt::FixedThreadPool* d_miscWorkThreadPool_p;

    /// Storage used by the queue associated to this state.
    bsl::shared_ptr<mqbi::Storage> d_storage_sp;

    /// Dispatcher Client Data of the queue associated to this state.
    mqbi::DispatcherClientData d_dispatcherClientData;

    /// Statistics of the queue associated to this state.
    bsl::shared_ptr<mqbstat::QueueStatsDomain> d_stats_sp;

    /// The routing configuration for this queue.
    bmqp_ctrlmsg::RoutingConfiguration d_routingConfig;

    /// The throttling thresholds and delay values for poison messages.
    mqbcfg::MessageThrottleConfig d_messageThrottleConfig;

    QueueHandleCatalog d_handleCatalog;

    Routers::QueueRoutingContext d_context;

    SubQueues d_subStreams;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    QueueState(const QueueState&);             // = delete;
    QueueState& operator=(const QueueState&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueState, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new 'QueueState' associated to the specified 'queue' and
    /// having the specified 'uri', 'id', 'key', 'partitionId', 'domain', and
    /// 'resources'.  Use the specified 'allocator' for any memory allocations.
    QueueState(mqbi::Queue*                 queue,
               const bmqt::Uri&             uri,
               unsigned int                 id,
               const mqbu::StorageKey&      key,
               int                          partitionId,
               mqbi::Domain*                domain,
               const mqbi::ClusterResources resources,
               bslma::Allocator*            allocator);

    /// Destructor
    ~QueueState();

    // MANIPULATORS
    QueueState& setMiscWorkThreadPool(bdlmt::FixedThreadPool* threadPool);
    QueueState& setDescription(const bslstl::StringRef& value);
    QueueState& setDomain(mqbi::Domain* value);
    QueueState& setId(unsigned int value);
    QueueState& setKey(const mqbu::StorageKey& key);
    QueueState& setPartitionId(int value);
    QueueState& setStorage(const bsl::shared_ptr<mqbi::Storage>& value);
    QueueState& setStorageManager(mqbi::StorageManager* value);
    QueueState&
    setRoutingConfig(const bmqp_ctrlmsg::RoutingConfiguration& routingConfig);
    QueueState& setMessageThrottleConfig(
        const mqbcfg::MessageThrottleConfig& messageThrottleConfig);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    QueueState& setUri(const bmqt::Uri& value);

    mqbi::DispatcherClientData& dispatcherClientData();
    QueueHandleCatalog&         handleCatalog();

    /// Store the specified value in the subQueuesParametersMap under the
    /// specified `subQueueId` key.
    void setUpstreamParameters(
        const bmqp_ctrlmsg::StreamParameters& value,
        unsigned int subQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    /// Remove the upstream parameters associated with the specified
    /// `subQueueId` and return true if they exist.  Return false otherwise.
    bool removeUpstreamParameters(
        unsigned int subQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    /// Set the corresponding attribute to the specified `stats`.
    void setStats(const bsl::shared_ptr<mqbstat::QueueStatsDomain>& stats);

    /// Add read, write, and admin counters from the specified `params` to
    /// cumulative values per queue and per appId.
    void add(const bmqp_ctrlmsg::QueueHandleParameters& params);

    /// Subtract read, write, and admin counters from the specified `params`
    /// from cumulative values per queue and per appId.  Return resulting
    /// counts.
    mqbi::QueueCounts
    subtract(const bmqp_ctrlmsg::QueueHandleParameters& params);

    /// Return readers and writers counts associated with the specified
    /// `handleParameters`.  If the `handleParameters` are unknown, return
    /// {0, 0}
    mqbi::QueueCounts consumerAndProducerCounts(
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters) const;

    /// Cache the reference to subStream of the specified `app`.  If the
    /// `app` does not have assigned upstreamSubQueueId, generate and assign
    /// one.
    void adopt(const bsl::shared_ptr<QueueEngineUtil_AppState>& app);

    /// Clear previously cached reference to the subStream identified by the
    /// specified `upstreamSubQueueId`.
    void abandon(unsigned int upstreamSubQueueId);

    /// Return reference to the structures for the queue engine routing.
    Routers::QueueRoutingContext& routingContext();

    /// Update the stats to the current values in the handleParamaters
    void updateStats();

    // ACCESSORS

    /// Return true if the queue has upstream parameters for the specified
    /// `upstreamSubQueueId` in which case load the parameters into the
    /// specified `value`.  Return false otherwise.
    bool getUpstreamParameters(
        bmqp_ctrlmsg::StreamParameters* value,
        unsigned int subQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) const;

    bdlbb::BlobBufferFactory*                    blobBufferFactory() const;
    bdlmt::EventScheduler*                       scheduler() const;
    mqbi::ClusterResources::BlobSpPool*          blobSpPool() const;
    const bsl::shared_ptr<bdlma::ConcurrentPool>& pushElementsPool() const;
    bdlmt::FixedThreadPool*                      miscWorkThreadPool() const;
    const bsl::string&                           description() const;
    const mqbi::DispatcherClientData&            dispatcherClientData() const;
    mqbi::Domain*                                domain() const;
    unsigned int                                 id() const;
    const mqbu::StorageKey&                      key() const;
    const QueueHandleCatalog&                    handleCatalog() const;
    const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters() const;

    int                                       partitionId() const;
    mqbi::Queue*                              queue() const;
    mqbi::Storage*                            storage() const;
    mqbi::StorageManager*                     storageManager() const;
    const bmqp_ctrlmsg::RoutingConfiguration& routingConfig() const;
    const mqbcfg::MessageThrottleConfig&      messageThrottleConfig() const;
    const bmqt::Uri&                          uri() const;

    /// Return a reference offering non-modifiable access to the shared pointer
    /// to the QueueStatsDomain.
    const bsl::shared_ptr<mqbstat::QueueStatsDomain>& stats() const;

    /// Print to the specified `out` object the internal details about this
    /// queue state.
    void loadInternals(mqbcmd::QueueState* out) const;

    /// Return a reference not offering modifiable access to the collection
    /// of QueueStreamParameters for all subQueues
    const SubQueuesParameters& subQueuesParameters() const;

    /// Return `true` if the specified `storage` is compatible with the
    /// current configuration, or `false` otherwise.
    bool
    isStorageCompatible(const bsl::shared_ptr<mqbi::Storage>& storageSp) const;

    /// Return `true` if the configuration for this queue requires
    /// at-most-once semantics or `false` otherwise.
    bool isAtMostOnce() const;

    /// Return `true` if the configuration for this queue requires
    /// deliver-to-all semantics or `false` otherwise.
    bool isDeliverAll() const;

    /// Return `true` if the configuration for this queue requires
    /// priority-consumers semantics or `false` otherwise.
    bool isDeliverConsumerPriority() const;

    /// Return `true` if the configuration for this queue requires
    /// has-multiple-sub-streams semantics or `false` otherwise.
    bool hasMultipleSubStreams() const;

    /// Return non-modifiable access reference to the collection of cached
    /// references to subStreams.
    const SubQueues& subQueues() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class QueueState
// ----------------

// MANIPULATORS

inline QueueState&
QueueState::setMiscWorkThreadPool(bdlmt::FixedThreadPool* threadPool)
{
    d_miscWorkThreadPool_p = threadPool;
    return *this;
}

inline QueueState& QueueState::setDescription(const bslstl::StringRef& value)
{
    d_description.assign(value.data(), value.length());
    return *this;
}

inline QueueState& QueueState::setDomain(mqbi::Domain* value)
{
    d_domain_p = value;
    return *this;
}

inline QueueState& QueueState::setId(unsigned int value)
{
    d_id = value;
    return *this;
}

inline QueueState& QueueState::setKey(const mqbu::StorageKey& key)
{
    d_key = key;
    return *this;
}

inline QueueState& QueueState::setPartitionId(int value)
{
    d_partitionId = value;
    return *this;
}

inline QueueState&
QueueState::setStorage(const bsl::shared_ptr<mqbi::Storage>& value)
{
    d_storage_sp = value;
    return *this;
}

inline QueueState& QueueState::setStorageManager(mqbi::StorageManager* value)
{
    d_storageManager_p = value;
    return *this;
}

inline QueueState& QueueState::setRoutingConfig(
    const bmqp_ctrlmsg::RoutingConfiguration& routingConfig)
{
    d_routingConfig = routingConfig;
    return *this;
}

inline QueueState& QueueState::setMessageThrottleConfig(
    const mqbcfg::MessageThrottleConfig& messageThrottleConfig)
{
    d_messageThrottleConfig = messageThrottleConfig;
    return *this;
}

inline QueueState& QueueState::setUri(const bmqt::Uri& value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value.isCanonical() &&
                     "Queue 'uri' must always be canonical");

    d_uri = value;
    return *this;
}

inline mqbi::DispatcherClientData& QueueState::dispatcherClientData()
{
    return d_dispatcherClientData;
}

inline QueueHandleCatalog& QueueState::handleCatalog()
{
    return d_handleCatalog;
}

inline void
QueueState::setUpstreamParameters(const bmqp_ctrlmsg::StreamParameters& value,
                                  unsigned int subQueueId)
{
    bsl::pair<SubQueuesParameters::iterator, bool> result =
        d_subQueuesParametersMap.emplace(subQueueId, value);

    if (!result.second) {
        result.first->second = value;
    }
}

inline bool QueueState::removeUpstreamParameters(unsigned int subQueueId)
{
    return d_subQueuesParametersMap.erase(subQueueId) == 1;
}

inline bool
QueueState::getUpstreamParameters(bmqp_ctrlmsg::StreamParameters* value,
                                  unsigned int subQueueId) const
{
    SubQueuesParameters::const_iterator it = d_subQueuesParametersMap.find(
        subQueueId);
    if (it == d_subQueuesParametersMap.end()) {
        return false;  // RETURN
    }
    *value = it->second;
    return true;
}

inline void
QueueState::setStats(const bsl::shared_ptr<mqbstat::QueueStatsDomain>& stats)
{
    d_stats_sp = stats;
}

inline void
QueueState::adopt(const bsl::shared_ptr<QueueEngineUtil_AppState>& app)
{
    unsigned int upstreamSubQueueId = app->upstreamSubQueueId();

    if (upstreamSubQueueId == bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID) {
        upstreamSubQueueId = static_cast<unsigned int>(d_subStreams.size());
        app->setUpstreamSubQueueId(upstreamSubQueueId);
    }

    if (upstreamSubQueueId >= d_subStreams.size()) {
        d_subStreams.resize(upstreamSubQueueId + 1);
    }

    d_subStreams[upstreamSubQueueId] = app;
}

inline void QueueState::abandon(unsigned int upstreamSubQueueId)
{
    BSLS_ASSERT_SAFE(upstreamSubQueueId < d_subStreams.size());

    d_subStreams[upstreamSubQueueId].reset();
}

inline Routers::QueueRoutingContext& QueueState::routingContext()
{
    return d_context;
}

// ACCESSORS
inline bdlbb::BlobBufferFactory* QueueState::blobBufferFactory() const
{
    return d_resources.bufferFactory();
}

inline bdlmt::EventScheduler* QueueState::scheduler() const
{
    return d_resources.scheduler();
}

inline mqbi::ClusterResources::BlobSpPool* QueueState::blobSpPool() const
{
    return d_resources.blobSpPool();
}

inline const bsl::shared_ptr<bdlma::ConcurrentPool>&
QueueState::pushElementsPool() const
{
    return d_resources.pushElementsPool();
}

inline bdlmt::FixedThreadPool* QueueState::miscWorkThreadPool() const
{
    return d_miscWorkThreadPool_p;
}

inline const bsl::string& QueueState::description() const
{
    return d_description;
}

inline const mqbi::DispatcherClientData&
QueueState::dispatcherClientData() const
{
    return d_dispatcherClientData;
}

inline mqbi::Domain* QueueState::domain() const
{
    return d_domain_p;
}

inline unsigned int QueueState::id() const
{
    return d_id;
}

inline const mqbu::StorageKey& QueueState::key() const
{
    return d_key;
}

inline const QueueHandleCatalog& QueueState::handleCatalog() const
{
    return d_handleCatalog;
}

inline const bmqp_ctrlmsg::QueueHandleParameters&
QueueState::handleParameters() const
{
    return d_handleParameters;
}

inline const QueueState::SubQueuesParameters&
QueueState::subQueuesParameters() const
{
    return d_subQueuesParametersMap;
}

inline int QueueState::partitionId() const
{
    return d_partitionId;
}

inline mqbi::Queue* QueueState::queue() const
{
    return d_queue_p;
}

inline mqbi::Storage* QueueState::storage() const
{
    return d_storage_sp.get();
}

inline mqbi::StorageManager* QueueState::storageManager() const
{
    return d_storageManager_p;
}

inline const bmqp_ctrlmsg::RoutingConfiguration&
QueueState::routingConfig() const
{
    return d_routingConfig;
}

inline const mqbcfg::MessageThrottleConfig&
QueueState::messageThrottleConfig() const
{
    return d_messageThrottleConfig;
}

inline const bmqt::Uri& QueueState::uri() const
{
    return d_uri;
}

inline const bsl::shared_ptr<mqbstat::QueueStatsDomain>&
QueueState::stats() const
{
    BSLS_ASSERT_SAFE(d_stats_sp);
    return d_stats_sp;
}

inline const QueueState::SubQueues& QueueState::subQueues() const
{
    return d_subStreams;
}

/// Format the specified `rhs` to the specified output `os` and return a
/// reference to the modifiable `os`.
bsl::ostream& operator<<(bsl::ostream&                          os,
                         const QueueState::SubQueuesParameters& rhs);

}  // close package namespace
}  // close enterprise namespace

#endif
