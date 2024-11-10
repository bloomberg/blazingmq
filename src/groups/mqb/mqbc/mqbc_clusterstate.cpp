// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbc_clusterstate.cpp                                              -*-C++-*-
#include <mqbc_clusterstate.h>

#include <mqbscm_version.h>
// MQB
#include <mqbi_domain.h>
#include <mqbstat_domainstats.h>

#include <bmqu_printutil.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace mqbc {

// ---------------------------
// class ClusterStateQueueInfo
// ---------------------------

bsl::ostream& ClusterStateQueueInfo::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", uri());
    printer.printAttribute("queueKey", key());
    printer.printAttribute("partitionId", partitionId());
    printer.printAttribute("appIdInfos", appInfos());
    printer.end();

    return stream;
}

// --------------------------
// class ClusterStateObserver
// --------------------------

ClusterStateObserver::~ClusterStateObserver()
{
    // NOTHING
}

void ClusterStateObserver::onPartitionPrimaryAssignment(
    BSLS_ANNOTATION_UNUSED int partitionId,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* primary,
    BSLS_ANNOTATION_UNUSED unsigned int         leaseId,
    BSLS_ANNOTATION_UNUSED bmqp_ctrlmsg::PrimaryStatus::Value status,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* oldPrimary,
    BSLS_ANNOTATION_UNUSED unsigned int         oldLeaseId)
{
    // NOTHING
}

void ClusterStateObserver::onQueueAssigned(
    BSLS_ANNOTATION_UNUSED const ClusterStateQueueInfo& info)
{
    // NOTHING
}

void ClusterStateObserver::onQueueUnassigned(
    BSLS_ANNOTATION_UNUSED const ClusterStateQueueInfo& info)
{
    // NOTHING
}

void ClusterStateObserver::onQueueUpdated(
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED const bsl::string& domain,
    BSLS_ANNOTATION_UNUSED const AppInfos&    addedAppIds,
    BSLS_ANNOTATION_UNUSED const AppInfos&    removedAppIds)
{
    // NOTHING
}

void ClusterStateObserver::onPartitionOrphanThreshold(
    BSLS_ANNOTATION_UNUSED size_t partitiondId)
{
    // NOTHING
}

void ClusterStateObserver::onNodeUnavailableThreshold(
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* node)
{
    // NOTHING
}

void ClusterStateObserver::onLeaderPassiveThreshold()
{
    // NOTHING
}

void ClusterStateObserver::onFailoverThreshold()
{
    // NOTHING
}

// ------------------
// class ClusterState
// ------------------

// MANIPULATORS
ClusterState& ClusterState::registerObserver(ClusterStateObserver* observer)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    BALL_LOG_INFO << "Cluster [" << d_cluster_p->name() << "]: "
                  << "Registered 1 new state observer.";

    d_observers.insert(observer);
    return *this;
}

ClusterState& ClusterState::unregisterObserver(ClusterStateObserver* observer)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));

    BALL_LOG_INFO << "Cluster [" << d_cluster_p->name() << "]: "
                  << "Unregistered 1 state observer.";

    d_observers.erase(observer);
    return *this;
}

ClusterState& ClusterState::setPartitionPrimary(int          partitionId,
                                                unsigned int leaseId,
                                                mqbnet::ClusterNode* node)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));
    BSLS_ASSERT_SAFE(partitionId >= 0);
    BSLS_ASSERT_SAFE(partitionId < static_cast<int>(d_partitionsInfo.size()));

    ClusterStatePartitionInfo& pinfo      = d_partitionsInfo[partitionId];
    mqbnet::ClusterNode*       oldPrimary = pinfo.primaryNode();
    const unsigned int         oldLeaseId = pinfo.primaryLeaseId();

    BSLS_ASSERT_SAFE(leaseId >= oldLeaseId);

    pinfo.setPrimaryNode(node);
    if (node) {
        pinfo.setPrimaryNodeId(node->nodeId());
    }
    else {
        pinfo.setPrimaryNodeId(mqbnet::Cluster::k_INVALID_NODE_ID);
    }
    pinfo.setPrimaryLeaseId(leaseId);

    if (node == oldPrimary) {
        // We are being notified about the same primary.  Check leaseId.  Note
        // that leader can bump up just the leaseId while keeping the primary
        // node unchanged.

        if (leaseId == oldLeaseId) {
            // Nothing's changed.  We leave the primary status unchanged.  No
            // need to notify observers.

            return *this;  // RETURN
        }
    }

    bmqp_ctrlmsg::PrimaryStatus::Value primaryStatus =
        bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED;
    if (node) {
        // By default, a new primary is PASSIVE.
        primaryStatus = bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE;
    }
    pinfo.setPrimaryStatus(primaryStatus);

    BALL_LOG_INFO << "Cluster [" << d_cluster_p->name() << "]: "
                  << "Setting primary of Partition [" << partitionId << "] to "
                  << "[" << (node ? node->nodeDescription() : "** NULL **")
                  << "], leaseId: [" << leaseId << "], primaryStatus: ["
                  << primaryStatus << "], oldPrimary: ["
                  << (oldPrimary ? oldPrimary->nodeDescription()
                                 : "** NULL **")
                  << "], oldLeaseId: [" << oldLeaseId << "].";

    for (ObserversSetIter it = d_observers.begin(); it != d_observers.end();
         ++it) {
        (*it)->onPartitionPrimaryAssignment(partitionId,
                                            node,
                                            leaseId,
                                            pinfo.primaryStatus(),
                                            oldPrimary,
                                            oldLeaseId);
    }

    return *this;
}

ClusterState& ClusterState::setPartitionPrimaryStatus(
    int                                partitionId,
    bmqp_ctrlmsg::PrimaryStatus::Value value)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));
    BSLS_ASSERT_SAFE(partitionId >= 0);
    BSLS_ASSERT_SAFE(partitionId < static_cast<int>(d_partitionsInfo.size()));

    ClusterStatePartitionInfo& pinfo = d_partitionsInfo[partitionId];
    if (0 == pinfo.primaryNode()) {
        BALL_LOG_ERROR << "Cluster [" << d_cluster_p->name() << "]: "
                       << "Failed to set the primary status of Partition ["
                       << partitionId << "] to [" << value
                       << "], reason: primary node is ** NULL **.";

        return *this;  // RETURN
    }

    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED !=
                     pinfo.primaryStatus());

    bmqp_ctrlmsg::PrimaryStatus::Value oldStatus = pinfo.primaryStatus();
    pinfo.setPrimaryStatus(value);

    BALL_LOG_INFO << "Cluster [" << d_cluster_p->name() << "]: "
                  << "Setting status of primary ["
                  << pinfo.primaryNode()->nodeDescription()
                  << "] of Partition [" << partitionId << "] to [" << value
                  << "], oldPrimaryStatus: [" << oldStatus << "], leaseId: ["
                  << pinfo.primaryLeaseId() << "].";

    if (oldStatus != value) {
        // Notify observers if primary is transitioning to another state, and
        // only the first time this occurs.

        for (ObserversSetIter it = d_observers.begin();
             it != d_observers.end();
             ++it) {
            (*it)->onPartitionPrimaryAssignment(partitionId,
                                                pinfo.primaryNode(),
                                                pinfo.primaryLeaseId(),
                                                value,
                                                pinfo.primaryNode(),
                                                pinfo.primaryLeaseId());
        }
    }

    return *this;
}

ClusterState& ClusterState::updatePartitionQueueMapped(int partitionId,
                                                       int delta)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));
    BSLS_ASSERT_SAFE(partitionId >= 0);
    BSLS_ASSERT_SAFE(partitionId < static_cast<int>(d_partitionsInfo.size()));

    ClusterStatePartitionInfo& pinfo = d_partitionsInfo[partitionId];
    BSLS_ASSERT_SAFE(delta > 0 || pinfo.numQueuesMapped() >= -delta);
    // Should never reach negative queue mapped

    pinfo.setNumQueuesMapped(pinfo.numQueuesMapped() + delta);

    return *this;
}

ClusterState& ClusterState::updatePartitionNumActiveQueues(int partitionId,
                                                           int delta)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));
    BSLS_ASSERT_SAFE(partitionId >= 0);
    BSLS_ASSERT_SAFE(partitionId < static_cast<int>(d_partitionsInfo.size()));

    ClusterStatePartitionInfo& pinfo = d_partitionsInfo[partitionId];
    BSLS_ASSERT_SAFE(delta > 0 || pinfo.numActiveQueues() >= -delta);
    // Should never reach negative queues count

    pinfo.setNumActiveQueues(pinfo.numActiveQueues() + delta);

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(pinfo.numActiveQueues() <= pinfo.numQueuesMapped());

    return *this;
}

bool ClusterState::assignQueue(const bmqt::Uri&        uri,
                               const mqbu::StorageKey& key,
                               int                     partitionId,
                               const AppInfos&         appIdInfos)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));

    bool                   isNewAssignment = true;
    const DomainStatesIter domIt = d_domainStates.find(uri.qualifiedDomain());

    if (domIt == d_domainStates.end()) {
        d_domainStates[uri.qualifiedDomain()].createInplace(d_allocator_p,
                                                            d_allocator_p);
        d_domainStates.at(uri.qualifiedDomain())
            ->queuesInfo()[uri]
            .createInplace(d_allocator_p,
                           uri,
                           key,
                           partitionId,
                           appIdInfos,
                           d_allocator_p);
    }
    else {
        const UriToQueueInfoMapIter iter = domIt->second->queuesInfo().find(
            uri);
        if (iter == domIt->second->queuesInfo().end()) {
            domIt->second->queuesInfo()[uri].createInplace(d_allocator_p,
                                                           uri,
                                                           key,
                                                           partitionId,
                                                           appIdInfos,
                                                           d_allocator_p);
        }
        else {
            isNewAssignment = false;

            updatePartitionQueueMapped(iter->second->partitionId(), -1);
            iter->second->setKey(key).setPartitionId(partitionId);
            iter->second->appInfos() = appIdInfos;
            // TODO: in what scenario 'pendingUnassignment() == true'?
            iter->second->setPendingUnassignment(false);
        }
    }

    updatePartitionQueueMapped(partitionId, 1);

    bmqu::Printer<AppInfos> printer(&appIdInfos);
    BALL_LOG_INFO << "Cluster [" << d_cluster_p->name() << "]: "
                  << "Assigning queue [" << uri << "], queueKey: [" << key
                  << "] to Partition [" << partitionId
                  << "] with appIdInfos: [" << printer
                  << "], isNewAssignment: " << isNewAssignment << ".";

    for (ObserversSetIter it = d_observers.begin(); it != d_observers.end();
         ++it) {
        (*it)->onQueueAssigned(ClusterStateQueueInfo(uri,
                                                     key,
                                                     partitionId,
                                                     appIdInfos,
                                                     d_allocator_p));
    }

    // POSTCONDITIONS
    //
    // Note: This assert needs to be here since onQueueAssigned() may change
    // 'numActiveQueues'.
    BSLS_ASSERT_SAFE(d_partitionsInfo[partitionId].numQueuesMapped() >=
                     d_partitionsInfo[partitionId].numActiveQueues());

    return isNewAssignment;
}

bool ClusterState::unassignQueue(const bmqt::Uri& uri)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));

    const DomainStatesIter domIt = d_domainStates.find(uri.qualifiedDomain());
    if (domIt == d_domainStates.end()) {
        return false;  // RETURN
    }

    const UriToQueueInfoMapCIter cit = domIt->second->queuesInfo().find(uri);
    if (cit == domIt->second->queuesInfo().end()) {
        return false;  // RETURN
    }

    const mqbu::StorageKey& key         = cit->second->key();
    const int               partitionId = cit->second->partitionId();
    updatePartitionQueueMapped(partitionId, -1);

    BALL_LOG_INFO << "Cluster [" << d_cluster_p->name() << "]: "
                  << "Unassigning queue [" << uri << "], queueKey: [" << key
                  << "] from Partition [" << partitionId << "].";

    for (ObserversSetIter it = d_observers.begin(); it != d_observers.end();
         ++it) {
        (*it)->onQueueUnassigned(*cit->second);
    }

    domIt->second->queuesInfo().erase(cit);

    // POSTCONDITIONS
    //
    // Note: This assert needs to be here since onQueueUnassigned() may change
    // 'numActiveQueues'.
    BSLS_ASSERT_SAFE(d_partitionsInfo[partitionId].numQueuesMapped() >=
                     d_partitionsInfo[partitionId].numActiveQueues());

    return true;
}

void ClusterState::clearQueues()
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));

    BALL_LOG_INFO << "Cluster [" << d_cluster_p->name() << "]: "
                  << "Clearing all " << d_domainStates.size()
                  << " domain states from state.";

    for (DomainStatesCIter domCit = d_domainStates.cbegin();
         domCit != d_domainStates.cend();
         ++domCit) {
        for (UriToQueueInfoMapCIter cit =
                 domCit->second->queuesInfo().cbegin();
             cit != domCit->second->queuesInfo().cend();) {
            unassignQueue((cit++)->first);
        }
        d_domainStates.erase(domCit);
    }
}

int ClusterState::updateQueue(const bmqt::Uri&   uri,
                              const bsl::string& domain,
                              const AppInfos&    addedAppIds,
                              const AppInfos&    removedAppIds)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->dispatcher()->inDispatcherThread(cluster()));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS              = 0,
        rc_QUEUE_NOT_FOUND      = -1,
        rc_APPID_ALREADY_EXISTS = -2,
        rc_APPID_NOT_FOUND      = -3
    };

    if (uri.isValid()) {
        BSLS_ASSERT_SAFE(uri.qualifiedDomain() == domain);

        const DomainStatesIter domIt = d_domainStates.find(
            uri.qualifiedDomain());
        if (domIt == d_domainStates.end()) {
            return rc_QUEUE_NOT_FOUND;  // RETURN
        }

        const UriToQueueInfoMapIter iter = domIt->second->queuesInfo().find(
            uri);
        if (iter == domIt->second->queuesInfo().end()) {
            return rc_QUEUE_NOT_FOUND;  // RETURN
        }

        AppInfos& appIdInfos = iter->second->appInfos();
        for (AppInfosCIter citer = addedAppIds.cbegin();
             citer != addedAppIds.cend();
             ++citer) {
            if (!appIdInfos.insert(*citer).second) {
                return rc_APPID_ALREADY_EXISTS;  // RETURN
            }
        }

        for (AppInfosCIter citer = removedAppIds.begin();
             citer != removedAppIds.end();
             ++citer) {
            const AppInfosCIter appIdInfoCIter = appIdInfos.find(*citer);
            if (appIdInfoCIter == appIdInfos.cend()) {
                return rc_APPID_NOT_FOUND;  // RETURN
            }
            appIdInfos.erase(appIdInfoCIter);
        }

        bmqu::Printer<AppInfos> printer1(&addedAppIds);
        bmqu::Printer<AppInfos> printer2(&removedAppIds);
        BALL_LOG_INFO << "Cluster [" << d_cluster_p->name() << "]: "
                      << "Updating queue [" << uri << "], queueKey: ["
                      << iter->second->key() << "], partitionId: ["
                      << iter->second->partitionId()
                      << "], addedAppIds: " << printer1
                      << ", removedAppIds: " << printer2 << ".";
    }
    else {
        // This update is for an entire domain, instead of any individual
        // queue.
        bmqu::Printer<AppInfos> printer1(&addedAppIds);
        bmqu::Printer<AppInfos> printer2(&removedAppIds);
        BALL_LOG_INFO << "Cluster [" << d_cluster_p->name() << "]: "
                      << "Updating domain: [" << domain
                      << "], addedAppIds: " << printer1
                      << ", removedAppIds: " << printer2 << ".";
    }

    for (ObserversSetIter it = d_observers.begin(); it != d_observers.end();
         ++it) {
        (*it)->onQueueUpdated(uri, domain, addedAppIds, removedAppIds);
    }

    return rc_SUCCESS;
}

void ClusterState::clear()
{
    d_observers.clear();
    d_queueKeys.clear();
    d_domainStates.clear();
    d_partitionsInfo.clear();
    d_cluster_p = 0;
}

// --------------------------------
// struct ClusterState::DomainState
// --------------------------------

void ClusterState::DomainState::adjustQueueCount(int by)
{
    d_numAssignedQueues += by;

    if (d_domain_p != 0) {
        d_domain_p->domainStats()->onEvent(
            mqbstat::DomainStats::EventType::e_QUEUE_COUNT,
            d_numAssignedQueues);
    }
}

// ----------------------------------------
// class ClusterState::PartitionIdExtractor
// ----------------------------------------

ClusterState::PartitionIdExtractor::PartitionIdExtractor(
    bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_regex(allocator)
{
    const char                  pattern[] = "^\\S+\\.([0-9]+)\\.\\S+\\.\\S+$";
    bsl::string                 error(d_allocator_p);
    size_t                      errorOffset;
    BSLA_MAYBE_UNUSED const int rc = d_regex.prepare(
        &error,
        &errorOffset,
        pattern,
        bdlpcre::RegEx::k_FLAG_JIT);
    BSLS_ASSERT_SAFE(rc == 0);
    BSLS_ASSERT_SAFE(d_regex.isPrepared() == true);
}

int ClusterState::PartitionIdExtractor::extract(
    const bsl::string& queueName) const
{
    bsl::vector<bslstl::StringRef> result(d_allocator_p);
    const int                      rc = d_regex.match(&result,
                                 queueName.data(),
                                 queueName.length());
    if (rc != 0) {
        return -1;  // RETURN
    }
    const int partitionId = bsl::stoi(result[1]);
    return partitionId;
}

}  // close package namespace
}  // close enterprise namespace
