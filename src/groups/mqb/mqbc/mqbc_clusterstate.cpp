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

// BMQ
#include <bmqp_protocolutil.h>
#include <bmqu_printutil.h>

// BDE
#include <bsl_vector.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace mqbc {

// ---------------------------
// class ClusterStateQueueInfo
// ---------------------------

bool ClusterStateQueueInfo::containsDefaultAppIdOnly(const AppInfos& appInfos)
{
    if (appInfos.empty()) {
        return true;  // RETURN
    }

    if (appInfos.size() == 1 &&
        appInfos.count(bmqp::ProtocolUtil::k_DEFAULT_APP_ID) == 1) {
        return true;  // RETURN
    }

    return false;
}

bool ClusterStateQueueInfo::hasTheSameAppIds(const AppInfos& appInfos) const
{
    if (containsDefaultAppIdOnly(d_appInfos) &&
        containsDefaultAppIdOnly(appInfos)) {
        return true;  // RETURN
    }

    // This ignores the order

    if (d_appInfos.size() != appInfos.size()) {
        return false;  // RETURN
    }

    for (AppInfos::const_iterator cit = d_appInfos.cbegin();
         cit != d_appInfos.cend();
         ++cit) {
        if (appInfos.count(cit->first) != 1) {
            return false;  // RETURN
        }
    }

    return true;
}

void ClusterStateQueueInfo::setApps(const bmqp_ctrlmsg::QueueInfo& advisory)
{
    BSLS_ASSERT_SAFE(uri() == advisory.uri());

    d_appInfos.clear();

    for (bsl::vector<bmqp_ctrlmsg::AppIdInfo>::const_iterator cit =
             advisory.appIds().cbegin();
         cit != advisory.appIds().cend();
         ++cit) {
        BSLS_ASSERT_SAFE(!cit->appId().empty());
        BSLS_ASSERT_SAFE(!cit->appKey().empty());

        d_appInfos.insert(mqbi::ClusterStateManager::AppInfo(
            bsl::string(cit->appId(), d_allocator_p),
            mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                             cit->appKey().data())));
    }
}

bool ClusterStateQueueInfo::equal(
    const bmqp_ctrlmsg::QueueInfo& advisory) const
{
    BSLS_ASSERT_SAFE(uri() == advisory.uri());

    if (partitionId() != advisory.partitionId()) {
        return false;
    }
    if (key() != mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                  advisory.key().data())) {
        return false;
    }

    if (advisory.appIds().size() != appInfos().size()) {
        return false;
    }

    for (bsl::vector<bmqp_ctrlmsg::AppIdInfo>::const_iterator cit =
             advisory.appIds().cbegin();
         cit != advisory.appIds().cend();
         ++cit) {
        if (appInfos().count(cit->appId()) == 0) {
            return false;
        }
    }
    return true;
}

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
    printer.printAttribute("stateOfAssignment", state());
    printer.end();

    return stream;
}

bsl::ostream&
ClusterStateQueueInfo::State::print(bsl::ostream&                      stream,
                                    ClusterStateQueueInfo::State::Enum value,
                                    int                                level,
                                    int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ClusterStateQueueInfo::State::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
ClusterStateQueueInfo::State::toAscii(ClusterStateQueueInfo::State::Enum value)
{
#define CASE(X)                                                               \
    case k_##X: return #X;

    switch (value) {
        CASE(NONE)
        CASE(ASSIGNING)
        CASE(ASSIGNED)
        CASE(UNASSIGNING)
    default: return "(* NONE *)";
    }

#undef CASE
}

bool ClusterStateQueueInfo::State::fromAscii(
    ClusterStateQueueInfo::State::Enum* out,
    const bslstl::StringRef&            str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(                                       \
            toAscii(ClusterStateQueueInfo::State::k_##M),                     \
            str.data(),                                                       \
            static_cast<int>(str.length()))) {                                \
        *out = ClusterStateQueueInfo::State::k_##M;                           \
        return true;                                                          \
    }

    CHECKVALUE(NONE)
    CHECKVALUE(ASSIGNING)
    CHECKVALUE(ASSIGNED)
    CHECKVALUE(UNASSIGNING)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// --------------------------
// class ClusterStateObserver
// --------------------------

ClusterStateObserver::~ClusterStateObserver()
{
    // NOTHING
}

void ClusterStateObserver::onPartitionPrimaryAssignment(
    BSLA_UNUSED int partitionId,
    BSLA_UNUSED mqbnet::ClusterNode* primary,
    BSLA_UNUSED unsigned int         leaseId,
    BSLA_UNUSED bmqp_ctrlmsg::PrimaryStatus::Value status,
    BSLA_UNUSED mqbnet::ClusterNode* oldPrimary,
    BSLA_UNUSED unsigned int         oldLeaseId)
{
    // NOTHING
}

void ClusterStateObserver::onQueueAssigned(
    BSLA_UNUSED const bsl::shared_ptr<ClusterStateQueueInfo>& info)
{
    // NOTHING
}

void ClusterStateObserver::onQueueUnassigned(
    BSLA_UNUSED const bsl::shared_ptr<ClusterStateQueueInfo>& info)
{
    // NOTHING
}

void ClusterStateObserver::onQueueUpdated(
    BSLA_UNUSED const bmqt::Uri& uri,
    BSLA_UNUSED const bsl::string& domain,
    BSLA_UNUSED const AppInfos&    addedAppIds,
    BSLA_UNUSED const AppInfos&    removedAppIds)
{
    // NOTHING
}

void ClusterStateObserver::onPartitionOrphanThreshold(
    BSLA_UNUSED size_t partitionId)
{
    // NOTHING
}

void ClusterStateObserver::onNodeUnavailableThreshold(
    BSLA_UNUSED mqbnet::ClusterNode* node)
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
    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());

    BALL_LOG_INFO << "Cluster [" << name()
                  << "]: " << "Registered 1 new state observer.";

    d_observers.insert(observer);
    return *this;
}

ClusterState& ClusterState::unregisterObserver(ClusterStateObserver* observer)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());

    BALL_LOG_INFO << "Cluster [" << name()
                  << "]: " << "Unregistered 1 state observer.";

    d_observers.erase(observer);
    return *this;
}

ClusterState& ClusterState::setPartitionPrimary(int          partitionId,
                                                unsigned int leaseId,
                                                ClusterNodeSession* ns)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());
    BSLS_ASSERT_SAFE(partitionId >= 0);
    BSLS_ASSERT_SAFE(partitionId < static_cast<int>(d_partitionsInfo.size()));

    ClusterStatePartitionInfo& pinfo      = d_partitionsInfo[partitionId];
    ClusterNodeSession*        oldPrimary = pinfo.primaryNodeSession();
    const unsigned int         oldLeaseId = pinfo.primaryLeaseId();

    BSLS_ASSERT_SAFE(leaseId >= oldLeaseId);

    mqbnet::ClusterNode* node           = 0;
    mqbnet::ClusterNode* oldPrimaryNode = oldPrimary
                                              ? oldPrimary->clusterNode()
                                              : 0;
    if (ns) {
        node = ns->clusterNode();
        BSLS_ASSERT_SAFE(node);
        pinfo.setPrimaryNodeId(node->nodeId());
    }
    else {
        pinfo.setPrimaryNodeId(mqbnet::Cluster::k_INVALID_NODE_ID);
    }

    if (ns == oldPrimary) {
        // We are being notified about the same primary.  Check leaseId.  Note
        // that leader can bump up just the leaseId while keeping the primary
        // node unchanged.

        if (leaseId == oldLeaseId) {
            // Nothing's changed.  We leave the primary status unchanged.  No
            // need to notify observers.

            return *this;  // RETURN
        }
    }

    BALL_LOG_INFO << "Cluster [" << d_cluster_p->name()
                  << "]: closing the gate " << partitionId;
    d_gatePrimary[partitionId].close();

    pinfo.setPrimaryNodeSession(ns);

    pinfo.setPrimaryLeaseId(leaseId);

    bmqp_ctrlmsg::PrimaryStatus::Value primaryStatus =
        bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED;
    if (ns) {
        // By default, a new primary is PASSIVE.
        primaryStatus = bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE;
    }
    pinfo.setPrimaryStatus(primaryStatus);

    BALL_LOG_INFO << "Cluster [" << name()
                  << "]: " << "Setting primary of Partition [" << partitionId
                  << "] to " << "["
                  << (node ? node->nodeDescription() : "** NULL **")
                  << "], leaseId: [" << leaseId << "], primaryStatus: ["
                  << primaryStatus << "], oldPrimary: ["
                  << (oldPrimaryNode ? oldPrimaryNode->nodeDescription()
                                     : "** NULL **")
                  << "], oldLeaseId: [" << oldLeaseId << "].";

    for (ObserversSetIter it = d_observers.begin(); it != d_observers.end();
         ++it) {
        (*it)->onPartitionPrimaryAssignment(partitionId,
                                            node,
                                            leaseId,
                                            pinfo.primaryStatus(),
                                            oldPrimaryNode,
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
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());
    BSLS_ASSERT_SAFE(partitionId >= 0);
    BSLS_ASSERT_SAFE(partitionId < static_cast<int>(d_partitionsInfo.size()));

    ClusterStatePartitionInfo& pinfo = d_partitionsInfo[partitionId];
    if (0 == pinfo.primaryNodeSession()) {
        BALL_LOG_ERROR << "Cluster [" << name() << "]: "
                       << "Failed to set the primary status of Partition ["
                       << partitionId << "] to [" << value
                       << "], reason: primary node is ** NULL **.";

        return *this;  // RETURN
    }

    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED !=
                     pinfo.primaryStatus());

    mqbnet::ClusterNode* node = pinfo.primaryNode();

    BSLS_ASSERT_SAFE(node);

    bmqp_ctrlmsg::PrimaryStatus::Value oldStatus = pinfo.primaryStatus();
    pinfo.setPrimaryStatus(value);

    BALL_LOG_INFO << "Cluster [" << name()
                  << "]: " << "Setting status of primary ["
                  << node->nodeDescription() << "] of Partition ["
                  << partitionId << "] to [" << value
                  << "], oldPrimaryStatus: [" << oldStatus << "], leaseId: ["
                  << pinfo.primaryLeaseId() << "].";

    if (oldStatus != value) {
        // Notify observers if primary is transitioning to another state, and
        // only the first time this occurs.

        for (ObserversSetIter it = d_observers.begin();
             it != d_observers.end();
             ++it) {
            (*it)->onPartitionPrimaryAssignment(partitionId,
                                                node,
                                                pinfo.primaryLeaseId(),
                                                value,
                                                node,
                                                pinfo.primaryLeaseId());
        }
    }

    // TODO: this code assumes that it is safe to send PUTs and CONFIRMS at
    // this point if the status is E_ACTIVE or close the gate otherwise.
    // May need to open the gate later/close earlier by a separate call.

    if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE == value) {
        BALL_LOG_INFO << "Cluster [" << d_cluster_p->name()
                      << "]: opening the gate " << partitionId;
        d_gatePrimary[partitionId].open();
    }
    else {
        BALL_LOG_INFO << "Cluster [" << d_cluster_p->name()
                      << "]: closing the gate " << partitionId;
        d_gatePrimary[partitionId].close();
    }

    return *this;
}

ClusterState& ClusterState::updatePartitionQueueMapped(int partitionId,
                                                       int delta)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());
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
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());
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

ClusterState::DomainState&
ClusterState::getDomainState(const bsl::string& domain)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());

    DomainStatesIter domIt = d_domainStates.find(domain);
    if (domIt == d_domainStates.end()) {
        DomainStateSp domainState;
        domainState.createInplace(d_allocator_p, d_allocator_p);
        domIt = d_domainStates.emplace(domain, domainState).first;
    }
    return *domIt->second;
}

void ClusterState::onDomainsCreated(const DomainMap& domains)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());

    DomainMap::const_iterator it = domains.cbegin();
    for (; it != domains.cend(); ++it) {
        if (it->second != 0) {
            DomainState& domState = getDomainState(it->first);
            domState.setDomain(it->second);
            domState.adjustQueueCount(0);
        }
    }
}

void ClusterState::assignQueue(const bmqp_ctrlmsg::QueueInfo& advisory)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());

    const bmqt::Uri&      uri         = advisory.uri();
    const int             partitionId = advisory.partitionId();
    DomainState&          domState    = getDomainState(uri.qualifiedDomain());
    UriToQueueInfoMapIter queueIt     = domState.queuesInfo().find(uri);
    QueueInfoSp           newQueueInfo;
    QueueInfoSp&          queue = newQueueInfo;

    if (queueIt == domState.queuesInfo().end()) {
        newQueueInfo.createInplace(d_allocator_p, advisory, d_allocator_p);
        queueIt = domState.queuesInfo().emplace(uri, queue).first;
    }
    else {
        queue = queueIt->second;

        if (queue->state() == ClusterStateQueueInfo::State::k_ASSIGNED) {
            ClusterStateQueueInfo fromAdvisory(advisory, d_allocator_p);

            if (queue->isEquivalent(fromAdvisory)) {
                // If queue info is unchanged, can simply return
                return;  // RETURN
            }
        }

        if (queue->partitionId() != mqbi::Storage::k_INVALID_PARTITION_ID) {
            updatePartitionQueueMapped(queue->partitionId(), -1);
        }

        queue->setKey(advisory).setPartitionId(partitionId);
        queue->setApps(advisory);
    }

    // Set the queue as assigned
    queue->setState(ClusterStateQueueInfo::State::k_ASSIGNED);

    updatePartitionQueueMapped(partitionId, 1);

    bmqu::Printer<bsl::vector<bmqp_ctrlmsg::AppIdInfo> > printer(
        &advisory.appIds());
    BALL_LOG_INFO << "Cluster [" << name() << "]: Assigning queue [" << uri
                  << "], queueKey: ["
                  << mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                      advisory.key().data())
                  << "] to Partition [" << partitionId
                  << "] with appIdInfos: [" << printer << "].";

    for (ObserversSetIter it = d_observers.begin(); it != d_observers.end();
         ++it) {
        (*it)->onQueueAssigned(queue);
    }

    // POSTCONDITIONS
    //
    // Note: This assert needs to be here since onQueueAssigned() may change
    // 'numActiveQueues'.
    BSLS_ASSERT_SAFE(d_partitionsInfo[partitionId].numQueuesMapped() >=
                     d_partitionsInfo[partitionId].numActiveQueues());
}

bool ClusterState::unassignQueue(const bmqt::Uri& uri)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());

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

    BALL_LOG_INFO << "Cluster [" << name() << "]: " << "Unassigning queue ["
                  << uri << "], queueKey: [" << key << "] from Partition ["
                  << partitionId << "].";

    for (ObserversSetIter it = d_observers.begin(); it != d_observers.end();
         ++it) {
        (*it)->onQueueUnassigned(cit->second);
    }

    domIt->second->queuesInfo().erase(cit);

    if (domIt->second->queuesInfo().empty()) {
        d_domainStates.erase(domIt);
    }

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
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());

    BALL_LOG_INFO << "Cluster [" << name() << "]: " << "Clearing all "
                  << d_domainStates.size() << " domain states from state.";

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

int ClusterState::updateQueue(const bmqp_ctrlmsg::QueueInfoUpdate& update)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster()->inDispatcherThread());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS              = 0,
        rc_QUEUE_NOT_FOUND      = -1,
        rc_APPID_ALREADY_EXISTS = -2,
        rc_APPID_NOT_FOUND      = -3
    };

    const bmqt::Uri&   uri    = update.uri();
    const bsl::string& domain = update.domain();

    // TODO: avoid this extra copy
    AppInfos addedAppIds(d_allocator_p);
    AppInfos removedAppIds(d_allocator_p);

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
        AppInfos& appInfos = iter->second->appInfos();

        for (bsl::vector<bmqp_ctrlmsg::AppIdInfo>::const_iterator citer =
                 update.addedAppIds().cbegin();
             citer != update.addedAppIds().cend();
             ++citer) {
            const AppInfo appInfo = bsl::make_pair(
                citer->appId(),
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 citer->appKey().data()));

            if (!appInfos.insert(appInfo).second) {
                return rc_APPID_ALREADY_EXISTS;  // RETURN
            }
            addedAppIds.insert(appInfo);
        }

        for (bsl::vector<bmqp_ctrlmsg::AppIdInfo>::const_iterator citer =
                 update.removedAppIds().cbegin();
             citer != update.removedAppIds().cend();
             ++citer) {
            const AppInfo appInfo = bsl::make_pair(
                citer->appId(),
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 citer->appKey().data()));

            if (appInfos.erase(citer->appId()) == 0) {
                return rc_APPID_NOT_FOUND;  // RETURN
            }

            removedAppIds.insert(appInfo);
        }

        bmqu::Printer<bsl::vector<bmqp_ctrlmsg::AppIdInfo> > printer1(
            &update.addedAppIds());
        bmqu::Printer<bsl::vector<bmqp_ctrlmsg::AppIdInfo> > printer2(
            &update.removedAppIds());
        BALL_LOG_INFO << "Cluster [" << name() << "]: " << "Updating queue ["
                      << uri << "], queueKey: [" << iter->second->key()
                      << "], partitionId: [" << iter->second->partitionId()
                      << "], addedAppIds: " << printer1
                      << ", removedAppIds: " << printer2 << ".";
    }
    else {
        // This update is for the entire domain, instead of any individual
        // queue.
        bmqu::Printer<bsl::vector<bmqp_ctrlmsg::AppIdInfo> > printer1(
            &update.addedAppIds());
        bmqu::Printer<bsl::vector<bmqp_ctrlmsg::AppIdInfo> > printer2(
            &update.removedAppIds());
        BALL_LOG_INFO << "Cluster [" << name() << "]: " << "Updating domain: ["
                      << domain << "], addedAppIds: " << printer1
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

/// TODO (FSM); remove after switching to FSM
bool ClusterState::cacheDoubleAssignment(const bmqt::Uri& uri, int partitionId)
{
    BSLS_ASSERT_SAFE(0 <= partitionId);
    return d_doubleAssignments[partitionId].emplace(uri).second;
}

/// TODO (FSM); remove after switching to FSM
void ClusterState::iterateDoubleAssignments(int                partitionId,
                                            AssignmentVisitor& visitor)
{
    if (mqbi::Storage::k_ANY_PARTITION_ID == partitionId) {
        for (Assignments::const_iterator cit = d_doubleAssignments.cbegin();
             cit != d_doubleAssignments.cend();
             ++cit) {
            iterateDoubleAssignments(cit, visitor);
        }
        d_doubleAssignments.clear();
    }
    else {
        Assignments::const_iterator cit = d_doubleAssignments.find(
            partitionId);
        if (cit != d_doubleAssignments.cend()) {
            iterateDoubleAssignments(cit, visitor);
            d_doubleAssignments.erase(cit);
        }
    }
}

void ClusterState::iterateDoubleAssignments(
    const Assignments::const_iterator& partitionAssignments,
    AssignmentVisitor&                 visitor) const
{
    const bsl::unordered_set<bmqt::Uri>& uris = partitionAssignments->second;

    for (bsl::unordered_set<bmqt::Uri>::const_iterator cit = uris.cbegin();
         cit != uris.cend();
         ++cit) {
        const bmqt::Uri& problematicUri   = *cit;
        const int        wrongPartitionId = partitionAssignments->first;

        BALL_LOG_WARN << "Cluster [" << name()
                      << "]: attempting to repair double assignment of queue '"
                      << problematicUri
                      << "' by unregistering it from the partition ["
                      << wrongPartitionId << "].";

        visitor(*cit, wrongPartitionId);
    }
}

// --------------------------------
// struct ClusterState::DomainState
// --------------------------------

void ClusterState::DomainState::adjustQueueCount(int by)
{
    // executed by the cluster *DISPATCHER* thread
    d_numAssignedQueues += by;

    if (d_domain_p != 0) {
        d_domain_p->domainStats()
            ->onEvent<mqbstat::DomainStats::EventType::e_QUEUE_COUNT>(
                d_numAssignedQueues);
    }
}

void ClusterState::DomainState::adjustOpenedQueueCount(int by)
{
    // executed by the cluster *DISPATCHER* thread
    d_numOpenedQueues += by;

    if (d_domain_p != 0) {
        d_domain_p->domainStats()
            ->onEvent<mqbstat::DomainStats::EventType::e_QUEUE_COUNT_OPEN>(
                d_numOpenedQueues);
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
    // Enable JIT compilation, unless running under MemorySanitizer.
    // Low-level assembler instructions used by sljit causes sanitizer issues.
    // See the internal ticket 177953779.
    int regexOptions = bdlpcre::RegEx::k_FLAG_JIT;
#if defined(__has_feature)  // Clang-supported method for checking sanitizers.
#if __has_feature(memory_sanitizer)
    regexOptions &= ~bdlpcre::RegEx::k_FLAG_JIT;
#endif
#elif defined(__SANITIZE_MEMORY__)  // GCC-supported macros for checking MSAN.
    regexOptions &= ~bdlpcre::RegEx::k_FLAG_JIT;
#endif

    const char                  pattern[] = "^\\S+\\.([0-9]+)\\.\\S+\\.\\S+$";
    bsl::string                 error(d_allocator_p);
    size_t                      errorOffset;
    BSLA_MAYBE_UNUSED const int rc =
        d_regex.prepare(&error, &errorOffset, pattern, regexOptions);
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
