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

// mqbblp_queuestate.cpp                                              -*-C++-*-
#include <mqbblp_queuestate.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_messages.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbi_storage.h>
#include <mqbstat_brokerstats.h>
#include <mqbu_capacitymeter.h>

// BMQ
#include <bmqp_queueutil.h>
#include <bmqp_routingconfigurationutils.h>

#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbblp {

// ----------------
// class QueueState
// ----------------

QueueState::QueueState(mqbi::Queue*                 queue,
                       const bmqt::Uri&             uri,
                       unsigned int                 id,
                       const mqbu::StorageKey&      key,
                       int                          partitionId,
                       mqbi::Domain*                domain,
                       const mqbi::ClusterResources resources,
                       bslma::Allocator*            allocator)
: d_queue_p(queue)
, d_uri(uri, allocator)
, d_description(allocator)
, d_id(id)
, d_key()
, d_handleParameters(allocator)
, d_subQueuesParametersMap(allocator)
, d_subQueuesHandleParameters(allocator)
, d_partitionId(partitionId)
, d_domain_p(domain)
, d_storageManager_p(0)
, d_resources(resources)
, d_miscWorkThreadPool_p(0)
, d_storage_mp(0)
, d_stats(allocator)
, d_messageThrottleConfig()
, d_handleCatalog(queue, allocator)
, d_context(queue->schemaLearner(), allocator)
, d_subStreams(allocator)
{
    setKey(key);

    // Initialize d_parameters
    d_handleParameters.uri() = d_uri.canonical();
    d_handleParameters.qId() = d_id;

    // Initialize stats
    d_stats.initialize(d_uri, d_domain_p);

    // NOTE: The 'description' will be set by the owner of this object.

    mqbstat::BrokerStats::instance()
        .onEvent<mqbstat::BrokerStats::EventType::e_QUEUE_CREATED>();
}

QueueState::~QueueState()
{
    mqbstat::BrokerStats::instance()
        .onEvent<mqbstat::BrokerStats::EventType::e_QUEUE_DESTROYED>();
}

void QueueState::add(const bmqp_ctrlmsg::QueueHandleParameters& params)
{
    // cumulative across all handles in all subStreams
    bmqp::QueueUtil::mergeHandleParameters(&d_handleParameters, params);

    // cumulative across all handles in one subStream
    const char* appId = bmqp::QueueUtil::extractAppId(params);

    SubQueuesHandleParameters::iterator it = d_subQueuesHandleParameters.find(
        appId);

    if (it == d_subQueuesHandleParameters.end()) {
        d_subQueuesHandleParameters.emplace(appId, params);
    }
    else {
        bmqp::QueueUtil::mergeHandleParameters(&it->second, params);
    }
}

mqbi::QueueCounts
QueueState::subtract(const bmqp_ctrlmsg::QueueHandleParameters& params)
{
    // cumulative across all handles in all subStreams
    bmqp::QueueUtil::subtractHandleParameters(&d_handleParameters, params);

    // cumulative across all handles in one subStream
    const char* appId = bmqp::QueueUtil::extractAppId(params);

    SubQueuesHandleParameters::iterator it = d_subQueuesHandleParameters.find(
        appId);

    BSLS_ASSERT_SAFE(it != d_subQueuesHandleParameters.end());

    bmqp::QueueUtil::subtractHandleParameters(&it->second, params);

    return mqbi::QueueCounts(it->second.readCount(), it->second.writeCount());
}

void QueueState::updateStats()
{
    stats()
        .setReaderCount(handleParameters().readCount())
        .setWriterCount(handleParameters().writeCount());
}

mqbi::QueueCounts QueueState::consumerAndProducerCounts(
    const bmqp_ctrlmsg::QueueHandleParameters& params) const
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue()->dispatcher()->inDispatcherThread(queue()));

    const char* appId = bmqp::QueueUtil::extractAppId(params);

    SubQueuesHandleParameters::const_iterator it =
        d_subQueuesHandleParameters.find(appId);
    if (it != d_subQueuesHandleParameters.end()) {
        return mqbi::QueueCounts(it->second.readCount(),
                                 it->second.writeCount());  // RETURN
    }
    return mqbi::QueueCounts(0, 0);
}

bool QueueState::isStorageCompatible(const StorageMp& storageMp) const
{
    // executed by either the *QUEUE* or *CLUSTER* thread

    // If persistent, then there are no validation rules at the moment.
    // If no persistence, then the storage should be in-memory.
    return !isAtMostOnce() || !storageMp->isPersistent();
}

bool QueueState::isAtMostOnce() const
{
    // executed by either the *QUEUE* or *CLUSTER* thread

    return bmqp::RoutingConfigurationUtils::isAtMostOnce(routingConfig());
}

bool QueueState::isDeliverAll() const
{
    // executed by either the *QUEUE* or *CLUSTER* thread

    return bmqp::RoutingConfigurationUtils::isDeliverAll(routingConfig());
}

bool QueueState::isDeliverConsumerPriority() const
{
    // executed by either the *QUEUE* or *CLUSTER* thread

    return bmqp::RoutingConfigurationUtils::isDeliverConsumerPriority(
        routingConfig());
}

bool QueueState::hasMultipleSubStreams() const
{
    // executed by either the *QUEUE* or *CLUSTER* thread

    return bmqp::RoutingConfigurationUtils::hasMultipleSubStreams(
        routingConfig());
}

void QueueState::loadInternals(mqbcmd::QueueState* out) const
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_p->dispatcher()->inDispatcherThread(d_queue_p));

    bmqu::MemOutStream os;

    out->uri() = d_uri.asString();
    os << d_handleParameters;
    out->handleParametersJson() = os.str();
    os.reset();
    os << d_subQueuesParametersMap;
    out->streamParametersJson() = os.str();
    out->id()                   = d_id;
    os.reset();
    os << d_key;
    out->key()         = os.str();
    out->partitionId() = d_partitionId;

    if (d_storage_mp) {
        mqbcmd::QueueStorage& queueStorage = out->storage().makeValue();
        queueStorage.numMessages()         = d_storage_mp->numMessages(
            mqbu::StorageKey::k_NULL_KEY);
        queueStorage.numBytes() = d_storage_mp->numBytes(
            mqbu::StorageKey::k_NULL_KEY);
        if (d_storage_mp->numVirtualStorages()) {
            mqbi::Storage::AppInfos appIdKeyPairs;
            d_storage_mp->loadVirtualStorageDetails(&appIdKeyPairs);
            BSLS_ASSERT_SAFE(
                appIdKeyPairs.size() ==
                static_cast<size_t>(d_storage_mp->numVirtualStorages()));

            bsl::vector<mqbcmd::VirtualStorage>& virtualStorages =
                queueStorage.virtualStorages();
            virtualStorages.resize(appIdKeyPairs.size());

            size_t i = 0;
            for (mqbi::Storage::AppInfos::const_iterator cit =
                     appIdKeyPairs.cbegin();
                 cit != appIdKeyPairs.cend();
                 ++cit, ++i) {
                const mqbi::Storage::AppInfo& p      = *cit;
                virtualStorages[i].appId()           = p.first;
                os.reset();
                os << p.second;
                virtualStorages[i].appKey()      = os.str();
                virtualStorages[i].numMessages() = d_storage_mp->numMessages(
                    p.second);
            }
        }
    }

    if (d_storage_mp) {
        mqbcmd::CapacityMeter& capacityMeter =
            out->capacityMeter().makeValue();
        mqbu::CapacityMeterUtil::loadState(&capacityMeter,
                                           *d_storage_mp->capacityMeter());
    }

    d_handleCatalog.loadInternals(&out->handles());
}

bsl::ostream& operator<<(bsl::ostream&                          os,
                         const QueueState::SubQueuesParameters& rhs)
{
    bslim::Printer printer(&os, 0, -1);  // one line

    printer.printValue(rhs.begin(), rhs.end());

    return os;
}

}  // close package namespace
}  // close enterprise namespace
