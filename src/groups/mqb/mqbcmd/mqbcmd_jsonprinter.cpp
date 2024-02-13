// Copyright 2020-2024 Bloomberg Finance L.P.
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

// mqbcmd_jsonprinter.cpp                                            -*-C++-*-
#include <mqbcmd_jsonprinter.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_messages.h>

// BMQ
#include <bmqp_queueid.h>

// MWC
#include <mwcu_memoutstream.h>
#include <mwcu_outstreamformatsaver.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdld_datumarraybuilder.h>
#include <bdld_datummapbuilder.h>
#include <bdljsn_json.h>
#include <bdljsn_jsonutil.h>
#include <bsl_algorithm.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslim_printer.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbcmd {

namespace {

template <class _Tp>
inline void makeJson(bdljsn::Json& res, const _Tp& value);

template <class _Tp>
inline void makeJson(bdljsn::Json& res, const bsl::vector<_Tp>& vec)
{
    bdljsn::JsonArray& array = res.makeArray();
    array.resize(vec.size());
    for (size_t i = 0; i < vec.size(); ++i) {
        makeJson(array[i], vec[i]);
    }
}

/// Functor to sort a `PurgeQueueResult` object by it's number of bytes, in
/// descending order.
struct SortPurgeQueueResultByBytesDesc {
    bool operator()(const PurgeQueueResult& lhs,
                    const PurgeQueueResult& rhs) const
    {
        return (lhs.isQueueValue() ? lhs.queue().numBytesPurged() : 0) >
               (rhs.isQueueValue() ? rhs.queue().numBytesPurged() : 0);
    }
};

void printMessageGroupIdManagerIndex(
    bsl::ostream&                     os,
    const MessageGroupIdManagerIndex& messageGroupIdManagerIndex,
    int                               level,
    int                               spacesPerLevel)
{
    os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel) << "Lru:"
       << "\n";

    {
        bslim::Printer printer(&os, level, spacesPerLevel);
        printer.start();

        typedef bsl::vector<LeastRecentlyUsedGroupId> LruGroupIds;
        const LruGroupIds&                            lruGroupIds =
            messageGroupIdManagerIndex.leastRecentlyUsedGroupIds();
        for (LruGroupIds::const_iterator cit = lruGroupIds.begin();
             cit != lruGroupIds.end();
             ++cit) {
            bsl::ostringstream key;
            bsl::ostringstream timeDelta;
            key << cit->clientDescription() << "#" << cit->msgGroupId();
            mwcu::PrintUtil::prettyTimeInterval(
                timeDelta,
                cit->lastSeenDeltaNanoseconds());
            printer.printAttribute(key.str().c_str(), timeDelta.str());
        }
        printer.end();
    }

    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "Handles:"
       << "\n";
    {
        bslim::Printer printer(&os, level, spacesPerLevel);
        printer.start();

        typedef bsl::vector<ClientMsgGroupsCount> NumMsgGroupsPerClient;
        const NumMsgGroupsPerClient&              numMsgGroupsPerClient =
            messageGroupIdManagerIndex.numMsgGroupsPerClient();
        for (NumMsgGroupsPerClient::const_iterator cit =
                 numMsgGroupsPerClient.begin();
             cit != numMsgGroupsPerClient.end();
             ++cit) {
            printer.printAttribute(cit->clientDescription().c_str(),
                                   cit->numMsgGroupIds());
        }
        printer.end();
    }
}

void printMessageGroupIdHelper(
    bsl::ostream&               os,
    const MessageGroupIdHelper& messageGroupIdHelper,
    int                         level,
    int                         spacesPerLevel)
{
    mwcu::OutStreamFormatSaver fmtSaver(os);

    os << mwcu::PrintUtil::indent(level, spacesPerLevel)
       << "MessageGroupIdManager:"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "timeout........: " << messageGroupIdHelper.timeoutNanoseconds()
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "maxMsgGroupIds.: " << messageGroupIdHelper.maxMsgGroupIds()
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "rebalance......: " << bsl::boolalpha
       << messageGroupIdHelper.isRebalanceOn()
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "status.........: [";

    printMessageGroupIdManagerIndex(os,
                                    messageGroupIdHelper.status(),
                                    level + 1,
                                    spacesPerLevel);

    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "]";
}

void printAppState(bsl::ostream&   os,
                   const AppState& appState,
                   int             level,
                   int             spacesPerLevel)
{
    os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "Consumers .........: " << appState.numConsumers()
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "RedeliveryList ....: " << appState.redeliveryListLength()
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "RoundRobinRouter ..:\n";

    appState.roundRobinRouter().print(os, level + 1, spacesPerLevel);
}

void printFanoutQueueEngine(bsl::ostream&            os,
                            const FanoutQueueEngine& queueEngine,
                            int                      level,
                            int                      spacesPerLevel)
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel)
       << "FanoutQueueEngine (" << queueEngine.mode()
       << "):" << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "-----------------";

    if (queueEngine.maxConsumers() != 0) {
        os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
           << " (maxConsumers: " << queueEngine.maxConsumers() << ")";
    }

    bsl::size_t                        maxAppIdLength = 0;
    typedef bsl::vector<ConsumerState> ConsumerStates;
    const ConsumerStates& consumerStates = queueEngine.consumerStates();
    for (ConsumerStates::const_iterator cit = consumerStates.begin();
         cit != consumerStates.end();
         ++cit) {
        maxAppIdLength = bsl::max(maxAppIdLength, cit->appId().length());
    }

    for (ConsumerStates::const_iterator cit = consumerStates.begin();
         cit != consumerStates.end();
         ++cit) {
        os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
           << bsl::setw(maxAppIdLength) << bsl::setfill(' ') << bsl::left
           << cit->appId() << ": status=" << cit->status();

        if (!cit->isAtEndOfStorage().isNull()) {
            os << ", StorageIter.atEnd=" << bsl::boolalpha
               << cit->isAtEndOfStorage().value();
        }

        os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
           << bsl::setw(maxAppIdLength) << bsl::setfill('-') << "-";

        printAppState(os, cit->appState(), level + 2, spacesPerLevel);
    }
    os << '\n';
}

void printRelayQueueEngine(bsl::ostream&           os,
                           const RelayQueueEngine& queueEngine,
                           int                     level,
                           int                     spacesPerLevel)
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "RelayQueueEngine:"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "-----------------";

    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Num SubStreams ....: " << queueEngine.numSubstreams();

    if (queueEngine.numSubstreams()) {
        typedef bsl::vector<RelayQueueEngineSubStream> SubStreams;
        const SubStreams& subStreams = queueEngine.subStreams();
        for (SubStreams::const_iterator cit = subStreams.cbegin();
             cit != subStreams.cend();
             ++cit) {
            os << mwcu::PrintUtil::newlineAndIndent(level + 2, spacesPerLevel)
               << "[" << cit->appId() << "] - [" << cit->appKey()
               << "] : " << cit->numMessages();
        }
    }

    typedef bsl::vector<AppState> AppStates;
    const AppStates&              appStates = queueEngine.appStates();
    for (AppStates::const_iterator cit = appStates.begin();
         cit != appStates.end();
         ++cit) {
        os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
           << "App: " << cit->appId()
           << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
           << "-----------------";
        printAppState(os, *cit, level + 2, spacesPerLevel);
    }

    os << "\n";
}

void printQueueEngine(bsl::ostream&      os,
                      const QueueEngine& queueEngine,
                      int                level,
                      int                spacesPerLevel)
{
    if (queueEngine.isFanoutValue()) {
        printFanoutQueueEngine(os,
                               queueEngine.fanout(),
                               level,
                               spacesPerLevel);
    }
    else if (queueEngine.isRelayValue()) {
        printRelayQueueEngine(os, queueEngine.relay(), level, spacesPerLevel);
    }
    else {
        os << "Unknown queue engine type: " << queueEngine;
    }
}

void printLocalQueue(bsl::ostream&     os,
                     const LocalQueue& localQueue,
                     int               level,
                     int               spacesPerLevel)
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "LocalQueue:"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "-----------\n";
    printQueueEngine(os, localQueue.queueEngine(), level + 1, spacesPerLevel);
}

void printRemoteStream(bsl::ostream&           os,
                       const RemoteStreamInfo& info,
                       int                     level,
                       int                     spacesPerLevel)
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel)
       << "[Id: " << info.id() << ", state: " << info.state()
       << ", genCount: " << info.genCount() << "]\n";
}
void printRemoteQueue(bsl::ostream&      os,
                      const RemoteQueue& remoteQueue,
                      int                level,
                      int                spacesPerLevel)
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "RemoteQueue:"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "------------\n";

    os << mwcu::PrintUtil::indent(level + 1, spacesPerLevel)
       << "Pending PUTs .......................: "
       << remoteQueue.numPendingPuts() << " items\n";
    os << mwcu::PrintUtil::indent(level + 1, spacesPerLevel)
       << "Pending CONFIRMs ...................: "
       << remoteQueue.numPendingConfirms() << " items\n";
    os << mwcu::PrintUtil::indent(level + 1, spacesPerLevel)
       << "Has PUSH expiration timer scheduled : "
       << (remoteQueue.isPushExpirationTimerScheduled() ? "yes" : "no")
       << "\n";

    os << mwcu::PrintUtil::indent(level + 1, spacesPerLevel)
       << "Upstream generation count ..........: "
       << remoteQueue.numUpstreamGeneration() << "\n";

    size_t num = remoteQueue.streams().size();
    os << mwcu::PrintUtil::indent(level + 1, spacesPerLevel)
       << "Streams ............................: "
       << "\n";

    typedef bsl::vector<RemoteStreamInfo> Streams;
    const Streams&                        streams = remoteQueue.streams();
    for (Streams::const_iterator cit = streams.begin(); cit != streams.end();
         ++cit, --num) {
        printRemoteStream(os, *cit, level + 2, spacesPerLevel);
    }

    os << "\n";
    printQueueEngine(os, remoteQueue.queueEngine(), level + 1, spacesPerLevel);
}

void printQueuesInfo(bsl::ostream&         os,
                     const StorageContent& queuesInfo,
                     int                   level,
                     int                   spacesPerLevel)
{
    mwcu::OutStreamFormatSaver fmtSaver(os);

    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "Queues"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel) << "------"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel);

    typedef bsl::vector<StorageQueueInfo> QueuesInfo;
    const QueuesInfo&                     queues = queuesInfo.storages();
    os << "Num Queues: " << queues.size();
    if (!queues.empty()) {
        os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
           << " QueueKey    Partition    Internal QueueId          QueueUri";
    }

    for (QueuesInfo::const_iterator cit = queues.begin(); cit != queues.end();
         ++cit) {
        os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
           << cit->queueKey() << "       " << cit->partitionId()
           << "           " << bsl::setfill(' ') << bsl::setw(10)
           << bmqp::QueueId::QueueIdInt(cit->internalQueueId()) << "       "
           << cit->queueUri();
    }
}

void printPartitionsInfo(bsl::ostream&         os,
                         const PartitionsInfo& partitionsInfo,
                         int                   level,
                         int                   spacesPerLevel)
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "Partitions"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "----------";

    const bsl::vector<PartitionInfo>& partitions = partitionsInfo.partitions();
    for (unsigned int i = 0; i < partitions.size(); ++i) {
        os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
           << "PartitionId: " << i
           << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
           << "Num Queues    :"
           << " mapped (" << partitions[i].numQueuesMapped() << "),"
           << " active (" << partitions[i].numActiveQueues() << ")";

        os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel);
        if (!partitions[i].primaryNode().isNull()) {
            os << "Primary Node   : " << partitions[i].primaryNode().value();
        }
        else {
            os << "Primary Node   : "
               << "[ ** NONE ** ]";
        }
        os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
           << "Primary LeaseId: " << partitions[i].primaryLeaseId()
           << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
           << "Primary Status : " << partitions[i].primaryStatus();
    }
}

void printElectorInfo(bsl::ostream&      os,
                      const ElectorInfo& electorInfo,
                      int                level,
                      int                spacesPerLevel)
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "Elector"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "-------\n"
       << mwcu::PrintUtil::indent(level, spacesPerLevel)
       << "Self State            : " << electorInfo.electorState()
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "Leader Node           : " << electorInfo.leaderNode()
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "Leader Sequence Number: [ electorTerm = "
       << electorInfo.leaderMessageSequence().electorTerm()
       << " sequenceNumber = "
       << electorInfo.leaderMessageSequence().sequenceNumber() << " ]"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "Leader Status         : " << electorInfo.leaderStatus();
}

void printNodeStatuses(bsl::ostream&       os,
                       const NodeStatuses& nodeStatuses,
                       int                 level,
                       int                 spacesPerLevel)
{
    mwcu::OutStreamFormatSaver fmtSaver(os);

    os << mwcu::PrintUtil::indent(level, spacesPerLevel) << "Nodes";
    os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel) << "-----";
    typedef bsl::vector<ClusterNodeInfo> Nodes;
    const Nodes&                         nodes = nodeStatuses.nodes();
    for (Nodes::const_iterator cit = nodes.cbegin(); cit != nodes.cend();
         ++cit) {
        os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
           << cit->description();
        os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel);
        if (cit->isAvailable().isNull()) {
            os << "IsConnected: self";
        }
        else {
            os << "IsConnected: " << bsl::boolalpha
               << cit->isAvailable().value();
        }
        os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
           << "Node Status: " << cit->status();

        if (cit->primaryForPartitionIds().size() > 0) {
            os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
               << "Primary for partitions: [ ";
            for (unsigned int i = 0; i < cit->primaryForPartitionIds().size();
                 ++i) {
                os << cit->primaryForPartitionIds()[i];
                if (i != (cit->primaryForPartitionIds().size() - 1)) {
                    os << ", ";
                }
            }
            os << " ]";
        }
    }
}

void printClusterStatus(bsl::ostream&        os,
                        const ClusterStatus& clusterStatus,
                        int                  level,
                        int                  spacesPerLevel)
{
    os << "Cluster ('" << clusterStatus.name() << "') :\n";
    os << "---------";
    // Add extra '-' padding
    for (size_t i = 0; i < clusterStatus.name().size() + 5; ++i) {
        os << "-";
    }
    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Cluster name  : " << clusterStatus.description();
    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Cluster size  : " << clusterStatus.nodeStatuses().nodes().size()
       << " node(s)";
    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Self Node     : " << clusterStatus.selfNodeDescription();
    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Is Healthy    : " << (clusterStatus.isHealthy() ? "Yes" : "No");

    os << "\n\n";
    printNodeStatuses(os,
                      clusterStatus.nodeStatuses(),
                      level + 1,
                      spacesPerLevel);
    os << "\n\n";
    printElectorInfo(os,
                     clusterStatus.electorInfo(),
                     level + 1,
                     spacesPerLevel);
    os << "\n\n";
    printPartitionsInfo(os,
                        clusterStatus.partitionsInfo(),
                        level + 1,
                        spacesPerLevel);
    os << "\n\n";
    printQueuesInfo(os, clusterStatus.queuesInfo(), level + 1, spacesPerLevel);
    os << "\n\n";
    //    printClusterStorageSummary(os,
    //                               clusterStatus.clusterStorageSummary(),
    //                               level + 1,
    //                               spacesPerLevel);
}

void printClusterProxyStatus(bsl::ostream&             os,
                             const ClusterProxyStatus& clusterProxyStatus,
                             int                       level,
                             int                       spacesPerLevel)
{
    mwcu::OutStreamFormatSaver fmtSaver(os);

    os << "Cluster\n"
       << "-------"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Cluster name: " << clusterProxyStatus.description()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Active node : ";

    if (!clusterProxyStatus.activeNodeDescription().isNull()) {
        os << clusterProxyStatus.activeNodeDescription().value();
    }
    else {
        os << "* none *";
    }

    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Is Healthy  : " << (clusterProxyStatus.isHealthy() ? "Yes" : "No")
       << "\n";

    // Print nodes status
    os << "\n";
    printNodeStatuses(os,
                      clusterProxyStatus.nodeStatuses(),
                      level,
                      spacesPerLevel);

    // Print queue status
    os << "\n";
    os << "\n";
    printQueuesInfo(os,
                    clusterProxyStatus.queuesInfo(),
                    level,
                    spacesPerLevel);
}

template <>
inline void makeJson(bdljsn::Json& res, const mqbcmd::Error& error)
{
    res.makeObject()["error"].makeObject()["message"] = error.message();
}

template <>
inline void makeJson(bdljsn::Json& res, const mqbcmd::Void& success)
{
    (void)success;
    res.makeObject()["success"].makeObject();
}

template <>
inline void makeJson(bdljsn::Json& res, const CommandSpec& spec)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["command"]          = spec.command();
    obj["description"]      = spec.description();
}

template <>
inline void makeJson(bdljsn::Json& res, const Help& helpCommands)
{
    bdljsn::JsonObject& obj = res.makeObject();
    makeJson(obj["help"], helpCommands.commands());
}

template <>
inline void makeJson(bdljsn::Json& res, const CapacityMeter& capacityMeter)
{
    bdljsn::JsonObject& obj = res.makeObject();

    if (capacityMeter.isDisabled()) {
        obj["disabled"] = true;
        return;  // RETURN
    }

    bdljsn::JsonObject& messages = obj["messages"].makeObject();
    messages["current"]          = capacityMeter.numMessages();
    messages["limit"]            = capacityMeter.messageCapacity();
    messages["reserved"]         = capacityMeter.numMessagesReserved();

    bdljsn::JsonObject& bytes = obj["bytes"].makeObject();
    bytes["current"]          = capacityMeter.numBytes();
    bytes["limit"]            = capacityMeter.byteCapacity();
    bytes["reserved"]         = capacityMeter.numBytesReserved();

    if (!capacityMeter.parent().isNull()) {
        makeJson(obj["parent"], capacityMeter.parent().value());
    }
}

template <>
inline void makeJson(bdljsn::Json& res, const StorageQueueInfo& info)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["queueUri"]         = info.queueUri();
    obj["queueKey"]         = info.queueKey();
    obj["partition"]        = info.partitionId();
    obj["numMessages"]      = info.numMessages();
    obj["numBytes"]         = info.numBytes();
}

template <>
inline void makeJson(bdljsn::Json& res, const StorageContent& storageContent)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["num"] = static_cast<int>(storageContent.storages().size());

    makeJson(obj["queueStorages"], storageContent.storages());
}

template <>
inline void makeJson(bdljsn::Json& res, const bsl::string& str)
{
    res = str;
}

template <>
inline void makeJson(bdljsn::Json& res, const DomainInfo& domainInfo)
{
    bdljsn::JsonObject& obj = res.makeObject()["domainInfo"].makeObject();

    obj["name"]    = domainInfo.name();
    obj["cluster"] = domainInfo.clusterName();

    const int rc = bdljsn::JsonUtil::read(&obj["config"],
                                          domainInfo.configJson());
    if (0 != rc) {
        // BALL_LOG_ERROR << "";
    }

    makeJson(obj["capacityMeter"], domainInfo.capacityMeter());
    makeJson(obj["storages"], domainInfo.storageContent());

    bdljsn::JsonObject& activeQueues = obj["activeQueues"].makeObject();
    activeQueues["num"] = static_cast<int>(domainInfo.queueUris().size());
    makeJson(activeQueues["uris"], domainInfo.queueUris());
}

template <>
inline void makeJson(bdljsn::Json& res, const ClusterNode& node)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["nodeId"]           = node.nodeId();
    obj["hostName"]         = node.hostName();
    obj["dataCenter"]       = node.dataCenter();
}

template <>
inline void makeJson(bdljsn::Json& res, const ClusterInfo& info)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["name"]             = info.name();
    obj["locality"]         = Locality::toString(info.locality());

    makeJson(obj["nodes"], info.nodes());
}

template <>
inline void makeJson(bdljsn::Json& res, const ClusterList& clusterList)
{
    makeJson(res.makeObject()["clusters"], clusterList.clusters());
}

template <>
inline void makeJson(bdljsn::Json& res, const BrokerConfig& brokerConfig)
{
    const int rc = bdljsn::JsonUtil::read(&res, brokerConfig.asJSON());
    if (0 != rc) {
        // BALL_LOG_ERROR << "";
    }
}

template <>
inline void makeJson(bdljsn::Json& res, const Value& value)
{
    switch (value.selectionId()) {
    case Value::SELECTION_ID_THE_INTEGER: {
        res = value.theInteger();
    } break;  // BREAK
    case Value::SELECTION_ID_THE_BOOL: {
        res = value.theBool();
    } break;  // BREAK
    case Value::SELECTION_ID_THE_DOUBLE: {
        res = value.theDouble();
    } break;  // BREAK
    case Value::SELECTION_ID_THE_STRING: {
        res = value.theString();
    } break;  // BREAK
    case Value::SELECTION_ID_THE_DATE: {
        mwcu::MemOutStream os(res.allocator());
        os << value.theDate();
        res = os.str();
    } break;  // BREAK
    case Value::SELECTION_ID_THE_TIME: {
        mwcu::MemOutStream os(res.allocator());
        os << value.theTime();
        res = os.str();
    } break;  // BREAK
    case Value::SELECTION_ID_THE_DATETIME: {
        mwcu::MemOutStream os(res.allocator());
        os << value.theDatetime();
        res = os.str();
    } break;  // BREAK
    default: {
        BSLS_ASSERT_SAFE(false && "Unsupported value");
    }
    }
}

template <>
inline void makeJson(bdljsn::Json& res, const Tunable& tunable)
{
    makeJson(res.makeObject()[tunable.name()], tunable.value());
}

template <>
inline void makeJson(bdljsn::Json& res, const Tunables& tunables)
{
    makeJson(res.makeObject()["tunables"], tunables.tunables());
}

template <>
inline void makeJson(bdljsn::Json&              res,
                     const TunableConfirmation& tunableConfirmation)
{
    bdljsn::JsonObject& obj =
        res.makeObject()["tunableConfirmation"].makeObject();
    obj["name"] = tunableConfirmation.name();
    makeJson(obj["newValue"], tunableConfirmation.newValue());
    makeJson(obj["oldValue"], tunableConfirmation.oldValue());
}

template <>
inline void makeJson(bdljsn::Json& res, const PurgedQueues& purgedQueueList)
{
    bdljsn::JsonObject& obj = res.makeObject()["purgedQueues"].makeObject();
    bdljsn::JsonArray&  queuesJson = obj["queues"].makeArray();
    bdljsn::JsonArray&  errorsJson = obj["errors"].makeArray();
    bdljsn::JsonObject& totalJson  = obj["total"].makeObject();
    if (purgedQueueList.queues().empty()) {
        totalJson["bytesPurged"]    = 0;
        totalJson["messagesPurged"] = 0;
        return;  // RETURN
    }

    // const_cast so we can inline sort without the need to make a deep
    // copy of the vector
    bsl::vector<PurgeQueueResult>& queues =
        const_cast<bsl::vector<PurgeQueueResult>&>(purgedQueueList.queues());

    bsl::sort(queues.begin(), queues.end(), SortPurgeQueueResultByBytesDesc());

    size_t numErrors = 0;
    for (bsl::vector<PurgeQueueResult>::const_iterator it = queues.begin();
         it != queues.end();
         ++it) {
        if (it->isErrorValue()) {
            numErrors++;
        }
    }

    bsls::Types::Int64 totalBytes    = 0;
    bsls::Types::Int64 totalMessages = 0;

    queuesJson.resize(queues.size() - numErrors);
    errorsJson.resize(numErrors);

    size_t nextQueueIndex = 0;
    size_t nextErrorIndex = 0;
    for (bsl::vector<PurgeQueueResult>::const_iterator it = queues.begin();
         it != queues.end();
         ++it) {
        if (it->isErrorValue()) {
            bdljsn::JsonObject& errorJson =
                errorsJson[nextErrorIndex++].makeObject();
            errorJson["message"] = it->error().message();
            continue;  // CONTINUE
        }

        const PurgedQueueDetails& queue = it->queue();

        bdljsn::JsonObject& queueJson =
            queuesJson[nextQueueIndex++].makeObject();
        queueJson["uri"]            = queue.queueUri();
        queueJson["appId"]          = queue.appId();
        queueJson["appKey"]         = queue.appKey();
        queueJson["bytesPurged"]    = queue.numBytesPurged();
        queueJson["messagesPurged"] = queue.numMessagesPurged();

        totalBytes += queue.numBytesPurged();
        totalMessages += queue.numMessagesPurged();
    }

    totalJson["bytesPurged"]    = totalBytes;
    totalJson["messagesPurged"] = totalMessages;
}

template <>
inline void makeJson(bdljsn::Json& res, const Message& message)
{
    bdljsn::JsonObject& messageJson = res.makeObject();
    messageJson["GUID"]             = message.guid();
    messageJson["sizeBytes"]        = message.sizeBytes();

    mwcu::MemOutStream os(res.allocator());
    os << message.arrivalTimestamp();

    messageJson["arrivalTimestamp"] = os.str();
}

template <>
inline void makeJson(bdljsn::Json& res, const QueueContents& queueContents)
{
    bdljsn::JsonObject& obj = res.makeObject()["queueContents"].makeObject();
    obj["numMessages"] = static_cast<int>(queueContents.messages().size());

    bdljsn::JsonArray& messagesJson = obj["messages"].makeArray();
    messagesJson.resize(queueContents.messages().size());

    bsls::Types::Int64 totalBytes = 0;
    bsls::Types::Int64 sequenceNo = queueContents.offset();

    for (size_t i = 0; i < queueContents.messages().size();
         i++, sequenceNo++) {
        const mqbcmd::Message& message = queueContents.messages()[i];

        totalBytes += message.sizeBytes();

        makeJson(messagesJson[i], message);

        messagesJson[i].theObject()["sequenceNo"] = sequenceNo;
    }

    bsls::Types::Int64 numMessages = queueContents.messages().size();
    bsls::Types::Int64 endInterval = 0;
    if (queueContents.offset() + numMessages >=
        queueContents.totalQueueMessages()) {
        endInterval = queueContents.totalQueueMessages() - 1;
    }
    else {
        endInterval = queueContents.offset() + numMessages - 1;
    }

    obj["offset"]        = queueContents.offset();
    obj["endInterval"]   = endInterval;
    obj["totalMessages"] = queueContents.totalQueueMessages();
    obj["totalBytes"]    = totalBytes;
}

template <>
inline void makeJson(bdljsn::Json& res, const FileSet& fset)
{
    bdljsn::JsonObject& obj       = res.makeObject();
    obj["dataFileName"]           = fset.dataFileName();
    obj["aliasedBlobBufferCount"] = static_cast<bsls::Types::Int64>(
        fset.aliasedBlobBufferCount());
}

template <>
inline void makeJson(bdljsn::Json& res, const FileInfo& info)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["usedBytes"]        = info.positionBytes();
    obj["totalBytes"]       = info.sizeBytes();
    obj["outstandingBytes"] = static_cast<bsls::Types::Int64>(
        info.outstandingBytes());
}

template <>
inline void makeJson(bdljsn::Json& res, const FileStore& fileStore)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["partitionId"]      = fileStore.partitionId();
    obj["state"] = mqbcmd::FileStoreState::toString(fileStore.state());

    if (fileStore.state() != mqbcmd::FileStoreState::OPEN) {
        return;  // RETURN
    }

    const FileStoreSummary& summary = fileStore.summary();
    obj["primaryNode"]              = summary.primaryNodeDescription();
    obj["primaryLeaseId"]           = summary.primaryLeaseId();
    obj["sequenceNumber"]           = summary.sequenceNum();
    obj["isAvailable"]              = summary.isAvailable();
    obj["totalMappedBytes"]         = summary.totalMappedBytes();
    obj["outstandingRecordsNum"]    = static_cast<bsls::Types::Int64>(
        summary.numOutstandingRecords());
    obj["unreceiptedMessagesNum"] = static_cast<bsls::Types::Int64>(
        summary.numUnreceiptedMessages());
    obj["naglePacketCount"] = static_cast<bsls::Types::Int64>(
        summary.naglePacketCount());

    makeJson(obj["fileSets"], summary.fileSets());

    const ActiveFileSet& activeFileSet     = summary.activeFileSet();
    bdljsn::JsonObject&  activeFileSetJson = obj["activeFileSet"].makeObject();
    makeJson(activeFileSetJson["dataFile"], activeFileSet.dataFile());
    makeJson(activeFileSetJson["journalFile"], activeFileSet.journalFile());
    makeJson(activeFileSetJson["qlistFile"], activeFileSet.qlistFile());

    makeJson(obj["storages"], summary.storageContent());
}

template <>
inline void makeJson(bdljsn::Json& res, const ClusterStorageSummary& summary)
{
    bdljsn::JsonObject& obj = res.makeObject()["clusterStorage"].makeObject();

    obj["location"] = summary.clusterFileStoreLocation();
    makeJson(obj["fileStores"], summary.fileStores());
}

template <>
inline void makeJson(bdljsn::Json& res, const SubId& subId)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["subId"]            = static_cast<bsls::Types::Int64>(subId.subId());
    obj["appId"]            = subId.appId();
}

template <>
inline void makeJson(bdljsn::Json& res, const Context& context)
{
    const int rc = bdljsn::JsonUtil::read(&res,
                                          context.queueHandleParametersJson());
    if (0 != rc) {
        // BALL_LOG_ERROR
    }
}

template <>
inline void makeJson(bdljsn::Json& res, const ClusterQueue& clusterQueue)
{
    bdljsn::JsonObject& obj    = res.makeObject();
    obj["uri"]                 = clusterQueue.uri();
    obj["key"]                 = clusterQueue.key();
    obj["isCreated"]           = clusterQueue.isCreated();
    obj["inFlightContextsNum"] = clusterQueue.numInFlightContexts();
    obj["isAssigned"]          = clusterQueue.isAssigned();
    obj["isPrimaryAvailable"]  = clusterQueue.isPrimaryAvailable();
    obj["id"] = static_cast<bsls::Types::Int64>(clusterQueue.id());
    // #review pass id here as int64 or convert it to string?
    // with values like UNASSIGNED ... etc
    // bmqp::QueueId::QueueIdInt(clusterQueue.id());

    obj["partitionId"] = clusterQueue.partitionId();

    if (!clusterQueue.primaryNodeDescription().isNull()) {
        obj["primaryNode"] = clusterQueue.primaryNodeDescription().value();
        // #review empty value for Null primary description
        // or do not make this field at all?
    }

    makeJson(obj["subIds"], clusterQueue.subIds());
    makeJson(obj["contexts"], clusterQueue.contexts());
}

template <>
inline void makeJson(bdljsn::Json& res, const ClusterDomain& domain)
{
    bdljsn::JsonObject& obj  = res.makeObject();
    obj["name"]              = domain.name();
    obj["assignedQueuesNum"] = domain.numAssignedQueues();
    obj["isLoaded"]          = domain.loaded();
}

template <>
inline void makeJson(bdljsn::Json&             res,
                     const ClusterQueueHelper& clusterQueueHelper)
{
    bdljsn::JsonObject& obj =
        res.makeObject()["clusterQueueHelper"].makeObject();
    obj["clusterName"]  = clusterQueueHelper.clusterName();
    obj["locality"]     = Locality::toString(clusterQueueHelper.locality());
    obj["queuesNum"]    = clusterQueueHelper.numQueues();
    obj["queueKeysNum"] = clusterQueueHelper.numQueueKeys();
    obj["pendingReopenQueueRequestsNum"] =
        clusterQueueHelper.numPendingReopenQueueRequests();

    makeJson(obj["domains"], clusterQueueHelper.domains());
    makeJson(obj["queues"], clusterQueueHelper.queues());
}

template <>
inline void makeJson(bdljsn::Json& res, const VirtualStorage& storage)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["appId"]            = storage.appId();
    obj["appKey"]           = storage.appKey();
    obj["numMessages"]      = static_cast<bsls::Types::Int64>(
        storage.numMessages());
}

template <>
inline void makeJson(bdljsn::Json& res, const QueueStorage& storage)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["numMessages"]      = static_cast<bsls::Types::Int64>(
        storage.numMessages());
    obj["numBytes"] = static_cast<bsls::Types::Int64>(storage.numBytes());

    makeJson(obj["virtualStorages"], storage.virtualStorages());
}

template<>
inline void makeJson(bdljsn::Json &res, const ResourceUsageMonitorState::Value &state) {
    // #review collapse inline
    res = ResourceUsageMonitorState::toString(state);
}

template <>
inline void makeJson(bdljsn::Json&               res,
                     const ResourceUsageMonitor& usageMonitor)
{
    bdljsn::JsonObject &obj = res.makeObject();
    makeJson(obj["state"], usageMonitor.state());

    makeJson(obj["messages"], usageMonitor.messagesState());

    obj["numMessages"] = usageMonitor.numMessages();
    obj["lowWatermarkMessagesNum"] = static_cast<bsls::Types::Int64>(
            usageMonitor.messagesLowWatermarkRatio() *
            usageMonitor.messagesCapacity());
    obj["highWatermarkMessagesNum"] = static_cast<bsls::Types::Int64>(
            usageMonitor.messagesHighWatermarkRatio() *
            usageMonitor.messagesCapacity());
    obj["messagesCapacity"] = usageMonitor.messagesCapacity();


    makeJson(obj["bytes"], usageMonitor.bytesState());

    obj["numBytes"] = usageMonitor.numBytes();
    obj["lowWatermarkBytesNum"] = static_cast<bsls::Types::Int64>(
            usageMonitor.bytesLowWatermarkRatio() *
            usageMonitor.bytesCapacity());
    obj["highWatermarkBytesNum"] = static_cast<bsls::Types::Int64>(
            usageMonitor.bytesHighWatermarkRatio() *
            usageMonitor.bytesCapacity());
    obj["bytesCapacity"] = usageMonitor.bytesCapacity();
}

template <>
inline void makeJson(bdljsn::Json& res, const QueueHandleSubStream& subStream)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["subId"] = static_cast<bsls::Types::Int64>(subStream.subId());

    if (!subStream.appId().isNull()) {
        obj["appId"] = subStream.appId().value();
        // #review empty value for Null primary description
        // or do not make this field at all?
    }

    if (!subStream.numUnconfirmedMessages().isNull()) {
        obj["numUnconfirmedMessages"] = static_cast<bsls::Types::Int64>(
            subStream.numUnconfirmedMessages().value());
        // #review empty value for Null primary description
        // or do not make this field at all?
    }

    const int rc = bdljsn::JsonUtil::read(&obj["streamParameters"],
                                          subStream.parametersJson());
    if (0 != rc) {
        // BALL_LOG_ERROR
    }

    makeJson(obj["unconfirmedMonitors"], subStream.unconfirmedMonitors());
}

template <>
inline void makeJson(bdljsn::Json& res, const QueueHandle& handle)
{
    bdljsn::JsonObject& obj = res.makeObject();

    int rc = bdljsn::JsonUtil::read(&obj["handleParameters"],
                                    handle.parametersJson());
    if (0 != rc) {
        // BALL_LOG_ERROR
    }

    obj["isClientClusterMember"] = handle.isClientClusterMember();

    makeJson(obj["subStreams"], handle.subStreams());
}

template <>
inline void makeJson(bdljsn::Json& res, const QueueState& state)
{
    bdljsn::JsonObject& obj = res.makeObject();
    obj["uri"]              = state.uri();

    int rc = bdljsn::JsonUtil::read(&obj["handleParameters"],
                                    state.handleParametersJson());
    if (0 != rc) {
        // BALL_LOG_ERROR
    }

    rc = bdljsn::JsonUtil::read(&obj["streamParameters"],
                                state.streamParametersJson());
    if (0 != rc) {
        // BALL_LOG_ERROR
    }

    obj["id"] = static_cast<bsls::Types::Int64>(state.id());
    // #review
    //    if (state.id() == bmqp::QueueId::k_UNASSIGNED_QUEUE_ID) {
    //        os << "unassigned";
    //    }
    //    else if (state.id() == bmqp::QueueId::k_PRIMARY_QUEUE_ID) {
    //        os << "primary";
    //    }
    //    else {
    //        os << state.id();
    //    }
    obj["key"]         = state.key();
    obj["partitionId"] = state.partitionId();

    if (!state.storage().isNull()) {
        makeJson(obj["storage"], state.storage().value());
        // #review Null storage - empty field in json?
    }

    if (!state.capacityMeter().isNull()) {
        makeJson(obj["capacityMeter"], state.capacityMeter().value());
        // #review Null storage - empty field in json?
    }

    makeJson(obj["queueHandles"], state.handles());
}

template <>
inline void makeJson(bdljsn::Json& res, const QueueInternals& queueInternals)
{
    bdljsn::JsonObject& obj = res.makeObject()["queueInternals"].makeObject();

    // Print State Object
    makeJson(obj["state"], queueInternals.state());

    // Print Queue Object
    const Queue& queue = queueInternals.queue();
    if (queue.isLocalQueueValue()) {
        makeJson(obj["localQueue"], queue.localQueue());
    }
    else if (queue.isRemoteQueueValue()) {
        makeJson(obj["remoteQueue"], queue.remoteQueue());
    }
    else {
        obj["error"] = "Uninitialized queue";
        // #review assert?
    }
}

}  // close anonymous namespace

// ------------------
// struct JsonPrinter
// ------------------

bsl::ostream& JsonPrinter::print(bsl::ostream& os,
                                 const Result& result,
                                 int           level,
                                 int           spacesPerLevel)
{
    bslma::Allocator* alloc = bslma::Default::allocator(0);

    baljsn::Encoder encoder;
    baljsn::EncoderOptions options;
    options.setEncodingStyle(baljsn::EncoderOptions::e_PRETTY);
    options.setSpacesPerLevel(4);

    const int rc = encoder.encode(os, result, options);
    return os;


    bdljsn::Json      json(alloc);

    if (result.isErrorValue()) {
        makeJson(json, result.error());
    }
    else if (result.isSuccessValue()) {
        makeJson(json, result.success());
    }
    else if (result.isHelpValue()) {
        makeJson(json, result.help());
    }
    else if (result.isDomainInfoValue()) {
        makeJson(json, result.domainInfo());
    }
    else if (result.isClusterListValue()) {
        makeJson(json, result.clusterList());
    }
    else if (result.isBrokerConfigValue()) {
        makeJson(json, result.brokerConfig());
    }
    else if (result.isTunableValue()) {
        makeJson(json, result.tunable());
    }
    else if (result.isTunablesValue()) {
        makeJson(json, result.tunables());
    }
    else if (result.isTunableConfirmationValue()) {
        makeJson(json, result.tunableConfirmation());
    }
    else if (result.isStatsValue()) {
        os << result.stats();
    }
    else if (result.isPurgedQueuesValue()) {
        makeJson(json, result.purgedQueues());
    }
    else if (result.isQueueContentsValue()) {
        makeJson(json, result.queueContents());
    }
    else if (result.isMessageValue()) {
        makeJson(json, result.message());
    }
    else if (result.isClusterStorageSummaryValue()) {
        makeJson(json, result.clusterStorageSummary());
    }
    else if (result.isStorageContentValue()) {
        makeJson(json, result.storageContent());
    }
    else if (result.isClusterQueueHelperValue()) {
        makeJson(json, result.clusterQueueHelper());
    }
    else if (result.isQueueInternalsValue()) {
        makeJson(json, result.queueInternals());
    }
    else if (result.isMessageGroupIdHelperValue()) {
        printMessageGroupIdHelper(os,
                                  result.messageGroupIdHelper(),
                                  level,
                                  spacesPerLevel);
    }
    else if (result.isClusterStatusValue()) {
        printClusterStatus(os, result.clusterStatus(), level, spacesPerLevel);
    }
    else if (result.isClusterProxyStatusValue()) {
        printClusterProxyStatus(os,
                                result.clusterProxyStatus(),
                                level,
                                spacesPerLevel);
    }
    else if (result.isNodeStatusesValue()) {
        printNodeStatuses(os, result.nodeStatuses(), level, spacesPerLevel);
    }
    else if (result.isElectorInfoValue()) {
        printElectorInfo(os, result.electorInfo(), level, spacesPerLevel);
    }
    else if (result.isPartitionsInfoValue()) {
        printPartitionsInfo(os,
                            result.partitionsInfo(),
                            level,
                            spacesPerLevel);
    }
    else {
        BSLS_ASSERT_SAFE(false && "Unsupported result");
    }

    if (json.isObject()) {
        bdljsn::WriteOptions options;
        options.setInitialIndentLevel(level);
        options.setSpacesPerLevel(spacesPerLevel);
        options.setStyle(bdljsn::WriteStyle::e_PRETTY);

        int rc = bdljsn::JsonUtil::write(os, json, options);
        if (0 != rc) {
            // BALL_LOG_ERROR
        }
    }
    return os;
}

}  // close package namespace
}  // close enterprise namespace
