// Copyright 2020-2023 Bloomberg Finance L.P.
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

// mqbcmd_humanprinter.cpp                                            -*-C++-*-
#include <mqbcmd_humanprinter.h>

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

void printCapacityMeter(bsl::ostream&        os,
                        const CapacityMeter& capacityMeter,
                        int                  level,
                        int                  spacesPerLevel)
{
    bdlb::Print::newlineAndIndent(os, level, spacesPerLevel);

    if (capacityMeter.isDisabled()) {
        os << "[ ** DISABLED ** ]";
        return;  // RETURN
    }

    os << capacityMeter.name() << ":"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Messages: [current: "
       << mwcu::PrintUtil::prettyNumber(capacityMeter.numMessages())
       << ", limit: "
       << mwcu::PrintUtil::prettyNumber(capacityMeter.messageCapacity());
    if (capacityMeter.numMessagesReserved() != 0) {
        os << ", reserved: "
           << mwcu::PrintUtil::prettyNumber(
                  capacityMeter.numMessagesReserved());
    }
    os << "]" << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Bytes   : [current: "
       << mwcu::PrintUtil::prettyBytes(capacityMeter.numBytes()) << ", limit: "
       << mwcu::PrintUtil::prettyBytes(capacityMeter.byteCapacity());
    if (capacityMeter.numBytesReserved() != 0) {
        os << ", reserved: "
           << mwcu::PrintUtil::prettyBytes(capacityMeter.numBytesReserved());
    }
    os << "]";

    if (!capacityMeter.parent().isNull()) {
        printCapacityMeter(os,
                           capacityMeter.parent().value(),
                           level,
                           spacesPerLevel);
    }
}

void printQueueStatus(bsl::ostream&         os,
                      const StorageContent& storageContent,
                      int                   level,
                      int                   spacesPerLevel)
{
    if (storageContent.storages().empty()) {
        return;  // RETURN
    }

    os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << " QueueKey    Partition    NumMsgs    NumBytes"
       << "                QueueUri";

    typedef bsl::vector<StorageQueueInfo> Storages;
    const Storages&                       storages = storageContent.storages();
    mwcu::OutStreamFormatSaver            fmtSaver(os);

    for (Storages::const_iterator cit = storages.cbegin();
         cit != storages.cend();
         ++cit) {
        os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
           << cit->queueKey() << " " << bsl::setfill(' ') << bsl::setw(7)
           << cit->partitionId() << "      " << bsl::setfill(' ')
           << bsl::setw(8) << mwcu::PrintUtil::prettyNumber(cit->numMessages())
           << "  " << bsl::setfill(' ') << bsl::setw(10)
           << mwcu::PrintUtil::prettyBytes(cit->numBytes()) << "    "
           << cit->queueUri()
           << (cit->isPersistent() ? "" : "  (non-persistent)");
    }
}

void printDomainInfo(bsl::ostream&     os,
                     const DomainInfo& domainInfo,
                     int               level,
                     int               spacesPerLevel)
{
    bdlb::Print::indent(os, level, spacesPerLevel);
    os << "Domain:" << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "-------"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Name ............: " << domainInfo.name()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Config ..........:\n"
       << domainInfo.configJson()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Cluster .........: " << domainInfo.clusterName()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Capacity Meter ..: ";
    printCapacityMeter(os,
                       domainInfo.capacityMeter(),
                       level + 2,
                       spacesPerLevel);
    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Queues ..........: "
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "------------------"
       << mwcu::PrintUtil::newlineAndIndent(level + 2, spacesPerLevel)
       << "ActiveQueues ..: " << domainInfo.queueUris().size();
    {
        mwcu::OutStreamFormatSaver fmtSaver(os);
        for (size_t i = 0; i < domainInfo.queueUris().size(); ++i) {
            const bsl::string& queueUri = domainInfo.queueUris()[i];
            os << mwcu::PrintUtil::newlineAndIndent(level + 2, spacesPerLevel)
               << " " << bsl::setfill(' ') << bsl::setw(3) << i << ". "
               << queueUri;
        }
    }
    os << "\n\n";

    bdlb::Print::indent(os, level, spacesPerLevel);
    os << "Storage ......."
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Number of assigned queue-storages: "
       << domainInfo.storageContent().storages().size();

    printQueueStatus(os,
                     domainInfo.storageContent(),
                     level + 2,
                     spacesPerLevel);
}

void printClusterList(bsl::ostream& os, const ClusterList& clusterList)
{
    mwcu::OutStreamFormatSaver fmtSaver(os);
    os << "The following clusters are active:";

    for (bsl::vector<ClusterInfo>::const_iterator itCluster =
             clusterList.clusters().cbegin();
         itCluster != clusterList.clusters().end();
         ++itCluster) {
        os << "\n    ";

        bsl::string locality(Locality::toString(itCluster->locality()));
        bdlb::String::toUpper(&locality);
        os << "[" << bsl::left << bsl::setw(6) << locality << "] "
           << itCluster->name() << ":";

        for (bsl::vector<ClusterNode>::const_iterator itNode =
                 itCluster->nodes().cbegin();
             itNode != itCluster->nodes().cend();
             ++itNode) {
            os << "\n        " << bsl::left << bsl::setw(20)
               << itNode->hostName() << bsl::setw(7) << itNode->nodeId()
               << bsl::setw(14) << itNode->dataCenter();
        }
        os << "\n";
    }
}

void printBrokerConfig(bsl::ostream& os, const BrokerConfig& brokerConfig)
{
    os << brokerConfig.asJSON();
}

void printValue(bsl::ostream& os, const Value& value)
{
    switch (value.selectionId()) {
    case Value::SELECTION_ID_THE_INTEGER: {
        os << value.theInteger();
    } break;
    case Value::SELECTION_ID_THE_BOOL: {
        os << value.theBool();
    } break;
    case Value::SELECTION_ID_THE_DOUBLE: {
        os << value.theDouble();
    } break;
    case Value::SELECTION_ID_THE_STRING: {
        os << value.theString();
    } break;
    case Value::SELECTION_ID_THE_DATE: {
        os << value.theDate();
    } break;
    case Value::SELECTION_ID_THE_TIME: {
        os << value.theTime();
    } break;
    case Value::SELECTION_ID_THE_DATETIME: {
        os << value.theDatetime();
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unsupported value");
        os << "Unsupported value: " << value;
    }
    }
}

void printTunable(bsl::ostream& os, const Tunable& tunable)
{
    os << tunable.name() << " = ";
    printValue(os, tunable.value());
}

void printTunableList(bsl::ostream& os, const Tunables& tunables)
{
    mwcu::OutStreamFormatSaver fmtSaver(os);

    // Compute the longest tunable name, for pretty-alignment
    bsl::size_t longestName = 0;
    for (bsl::vector<Tunable>::const_iterator it =
             tunables.tunables().cbegin();
         it != tunables.tunables().cend();
         ++it) {
        longestName = bsl::max(longestName, it->name().length());
    }

    // Print all tunables
    for (bsl::vector<Tunable>::const_iterator it =
             tunables.tunables().cbegin();
         it != tunables.tunables().cend();
         ++it) {
        os << it->name() << bsl::setw(longestName - it->name().length() + 4)
           << bsl::setfill('.') << ": ";
        // NOTE: '+ 4' in order to have at least 2 '.' (and then the ": ")
        printValue(os, it->value());
        os << "\n    " << it->description() << "\n";
    }
}

void printTunableConfirmation(bsl::ostream&              os,
                              const TunableConfirmation& tunableConfirmation)
{
    os << tunableConfirmation.name() << " set to ";
    printValue(os, tunableConfirmation.newValue());
    os << " from ";
    printValue(os, tunableConfirmation.oldValue());
}

void printPurgedQueues(bsl::ostream& os, const PurgedQueues& purgedQueueList)
{
    if (purgedQueueList.queues().empty()) {
        os << "No queue purged.";
        return;  // RETURN
    }

    // const_cast so we can inline sort without the need to make a deep
    // copy of the vector
    bsl::vector<PurgeQueueResult>& queues =
        const_cast<bsl::vector<PurgeQueueResult>&>(purgedQueueList.queues());

    bsl::sort(queues.begin(), queues.end(), SortPurgeQueueResultByBytesDesc());

    mwcu::MemOutStream stream;
    mwcu::MemOutStream errorStream;
    bsls::Types::Int64 totalBytes    = 0;
    bsls::Types::Int64 totalMessages = 0;

    for (bsl::vector<PurgeQueueResult>::const_iterator it = queues.begin();
         it != queues.end();
         ++it) {
        if (it->isErrorValue()) {
            errorStream << "\n" << it->error().message();
            continue;  // CONTINUE
        }

        const PurgedQueueDetails& queue = it->queue();
        stream << bsl::endl
               << "  " << bsl::setfill(' ') << bsl::setw(10) << bsl::right
               << mwcu::PrintUtil::prettyBytes(queue.numBytesPurged()) << "  "
               << bsl::setfill(' ') << bsl::setw(10) << bsl::right
               << mwcu::PrintUtil::prettyNumber(queue.numMessagesPurged())
               << bsl::left << "  " << queue.appKey() << "  "
               << bsl::setfill(' ') << bsl::setw(16) << bsl::right
               << queue.appId() << bsl::left << "  " << queue.queueUri();
        totalBytes += queue.numBytesPurged();
        totalMessages += queue.numMessagesPurged();
    }

    os << "Purged " << mwcu::PrintUtil::prettyNumber(totalMessages)
       << " message(s) for a total of "
       << mwcu::PrintUtil::prettyBytes(totalBytes) << " from " << queues.size()
       << " queue(s):\n"
       << "  "
       << "  NumBytes"
       << "  "
       << "   NumMsgs"
       << "  "
       << "    AppKey"
       << "  "
       << "           AppId"
       << "  "
       << "QueueName" << stream.str();

    if (!errorStream.str().empty()) {
        os << "\nErrors encountered: " << errorStream.str();
    }
}

void printQueueContents(bsl::ostream& os, const QueueContents& queueContents)
{
    mwcu::MemOutStream msgListing;
    bsls::Types::Int64 totalBytes = 0;

    bsl::vector<Message>::const_iterator it =
        queueContents.messages().cbegin();
    for (bsls::Types::Int64 sequenceNo = queueContents.offset();
         it != queueContents.messages().cend();
         ++sequenceNo, ++it) {
        totalBytes += it->sizeBytes();
        msgListing << bsl::setw(5) << sequenceNo << ": " << bsl::setw(32)
                   << it->guid() << "  " << bsl::setw(10)
                   << mwcu::PrintUtil::prettyBytes(it->sizeBytes()) << "  "
                   << it->arrivalTimestamp() << "\n";
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

    os << "Printing " << numMessages << " message(s) ["
       << queueContents.offset() << "-" << endInterval << " / "
       << mwcu::PrintUtil::prettyNumber(queueContents.totalQueueMessages())
       << "] (total: " << mwcu::PrintUtil::prettyBytes(totalBytes) << ")\n"
       << "       GUID                              Size        Timestamp "
          "(UTC)\n"
       << msgListing.str();
}

void printMessage(bsl::ostream& os, const Message& message)
{
    os << "GUID                              Size        Timestamp (UTC)\n"
       << bsl::setw(32) << message.guid() << "  " << bsl::setw(10)
       << mwcu::PrintUtil::prettyBytes(message.sizeBytes()) << "  "
       << message.arrivalTimestamp();
}

void printFileStoreSummary(bsl::ostream&           os,
                           const FileStoreSummary& summary,
                           int                     partitionId,
                           int                     level,
                           int                     spacesPerLevel)
{
    using namespace mwcu::PrintUtil;

    os << indent(level, spacesPerLevel) << "PartitionId [" << partitionId
       << "]:" << newlineAndIndent(level, spacesPerLevel) << "----------------"
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "Primary Node: " << summary.primaryNodeDescription()
       << ", Sequence Number: (" << summary.primaryLeaseId() << ", "
       << summary.sequenceNum() << ")"
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "IsAvailable: " << bsl::boolalpha << summary.isAvailable()
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "Total maintained file sets: " << summary.fileSets().size() << "\n"
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "Aliased payload count: ";

    typedef bsl::vector<FileSet> FileSets;
    const FileSets&              fileSets = summary.fileSets();
    for (FileSets::const_iterator cit = fileSets.cbegin();
         cit != fileSets.cend();
         ++cit) {
        os << newlineAndIndent(level + 2, spacesPerLevel)
           << cit->dataFileName() << " : "
           << prettyNumber(static_cast<bsls::Types::Int64>(
                  cit->aliasedBlobBufferCount()));
    }

    const ActiveFileSet& activeFileSet = summary.activeFileSet();
    BSLS_ASSERT_SAFE(activeFileSet.dataFile().sizeBytes() > 0);
    BSLS_ASSERT_SAFE(activeFileSet.journalFile().sizeBytes() > 0);
    os << '\n'
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "Active file set details: "
       << newlineAndIndent(level + 2, spacesPerLevel)
       << "Data file size    (used/total): "
       << prettyBytes(activeFileSet.dataFile().positionBytes()) << " / "
       << prettyBytes(activeFileSet.dataFile().sizeBytes()) << "  ("
       << ((activeFileSet.dataFile().positionBytes() * 100) /
           activeFileSet.dataFile().sizeBytes())
       << " %). Outstanding bytes: "
       << prettyNumber(static_cast<bsls::Types::Int64>(
              activeFileSet.dataFile().outstandingBytes()))
       << newlineAndIndent(level + 2, spacesPerLevel)
       << "Journal file size (used/total): "
       << prettyBytes(activeFileSet.journalFile().positionBytes()) << " / "
       << prettyBytes(activeFileSet.journalFile().sizeBytes()) << "  ("
       << ((activeFileSet.journalFile().positionBytes() * 100) /
           activeFileSet.journalFile().sizeBytes())
       << " %). Outstanding bytes: "
       << prettyNumber(static_cast<bsls::Types::Int64>(
              activeFileSet.journalFile().outstandingBytes()))
       << newlineAndIndent(level + 2, spacesPerLevel);

    // QList will be deprecated when CSL mode with FSM workflow is enabled.
    //
    // TODO CSL Remove this code block
    if (activeFileSet.qlistFile().sizeBytes() > 0) {
        os << "Qlist file size   (used/total): "
           << prettyBytes(activeFileSet.qlistFile().positionBytes()) << " / "
           << prettyBytes(activeFileSet.qlistFile().sizeBytes()) << "  ("
           << ((activeFileSet.qlistFile().positionBytes() * 100) /
               activeFileSet.qlistFile().sizeBytes())
           << " %). Outstanding bytes: "
           << prettyNumber(static_cast<bsls::Types::Int64>(
                  activeFileSet.qlistFile().outstandingBytes()));
    }

    os << '\n'
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "Total mapped size      : "
       << prettyBytes(summary.totalMappedBytes())
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "Num outstanding records: "
       << prettyNumber(
              static_cast<bsls::Types::Int64>(summary.numOutstandingRecords()))
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "Num unreceipted messages: "
       << prettyNumber(static_cast<bsls::Types::Int64>(
              summary.numUnreceiptedMessages()))
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "Current Nagle count: "
       << prettyNumber(
              static_cast<bsls::Types::Int64>(summary.naglePacketCount()))
       << '\n'
       << newlineAndIndent(level + 1, spacesPerLevel)
       << "Number of assigned queue-storages: "
       << prettyNumber(static_cast<bsls::Types::Int64>(
              summary.storageContent().storages().size()));

    printQueueStatus(os, summary.storageContent(), level + 2, spacesPerLevel);
}

void printClusterStorageSummary(bsl::ostream&                os,
                                const ClusterStorageSummary& summary,
                                int                          level,
                                int                          spacesPerLevel)
{
    bdlb::Print::indent(os, level, spacesPerLevel);
    os << "Storage Summary:"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "----------------"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Storage location: " << summary.clusterFileStoreLocation() << "\n\n";

    typedef bsl::vector<FileStore> FileStores;
    const FileStores&              fileStores = summary.fileStores();
    for (FileStores::const_iterator cit = fileStores.cbegin();
         cit != fileStores.cend();
         ++cit) {
        if (cit->state() == FileStoreState::CLOSED) {
            os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
               << "PartitionId [" << cit->partitionId() << "]: NOT OPEN.";
            continue;  // CONTINUE
        }
        else if (cit->state() == FileStoreState::STOPPING) {
            os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
               << "PartitionId [" << cit->partitionId() << "]: STOPPING.";
            continue;  // CONTINUE
        }

        printFileStoreSummary(os,
                              cit->summary(),
                              cit->partitionId(),
                              level + 1,
                              spacesPerLevel);
        os << "\n";
    }
}

void printClusterQueueHelper(bsl::ostream&             os,
                             const ClusterQueueHelper& clusterQueueHelper,
                             int                       level,
                             int                       spacesPerLevel)
{
    os << "ClusterQueueHelper :\n"
       << "--------------------"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "cluster......................: " << clusterQueueHelper.clusterName()
       << " (" << Locality::toString(clusterQueueHelper.locality()) << ")"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "nbQueues.....................: " << clusterQueueHelper.numQueues()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "nbQueueKeys..................: "
       << clusterQueueHelper.numQueueKeys()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "nbPendingReopenQueueRequests.: "
       << clusterQueueHelper.numPendingReopenQueueRequests() << "\n";

    // Domains
    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Domains :"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "---------";

    bsl::size_t                        maxDomainLength = 0;
    typedef bsl::vector<ClusterDomain> ClusterDomains;
    const ClusterDomains& clusterDomains = clusterQueueHelper.domains();
    for (ClusterDomains::const_iterator cit = clusterDomains.cbegin();
         cit != clusterDomains.cend();
         ++cit) {
        maxDomainLength = bsl::max(maxDomainLength, cit->name().length());
    }

    for (ClusterDomains::const_iterator cit = clusterDomains.cbegin();
         cit != clusterDomains.cend();
         ++cit) {
        os << mwcu::PrintUtil::newlineAndIndent(level + 2, spacesPerLevel)
           << cit->name() << bsl::setw(maxDomainLength - cit->name().length())
           << bsl::setfill('.')
           << ": numAssignedQueues=" << cit->numAssignedQueues();
        if (!cit->loaded()) {
            os << " (domain not yet loaded)";
        }
        os << "\n";
    }

    // Queues
    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Queues :"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "--------";

    typedef bsl::vector<ClusterQueue> ClusterQueues;
    const ClusterQueues& clusterQueues = clusterQueueHelper.queues();
    for (ClusterQueues::const_iterator cit = clusterQueues.cbegin();
         cit != clusterQueues.cend();
         ++cit) {
        // Queue URI
        os << mwcu::PrintUtil::newlineAndIndent(level + 2, spacesPerLevel)
           << cit->uri();

        // Info
        os << mwcu::PrintUtil::newlineAndIndent(level + 3, spacesPerLevel)
           << "inFlightContexts.: " << cit->numInFlightContexts()
           << mwcu::PrintUtil::newlineAndIndent(level + 3, spacesPerLevel)
           << "assigned.........: " << bsl::boolalpha << cit->isAssigned()
           << mwcu::PrintUtil::newlineAndIndent(level + 3, spacesPerLevel)
           << "primaryAvailable..: " << bsl::boolalpha
           << cit->isPrimaryAvailable()
           << mwcu::PrintUtil::newlineAndIndent(level + 3, spacesPerLevel)
           << "id...............: " << bmqp::QueueId::QueueIdInt(cit->id())
           << mwcu::PrintUtil::newlineAndIndent(level + 3, spacesPerLevel)
           << "subIds...........: ";

        typedef bsl::vector<SubId> SubIds;
        const SubIds&              subIds = cit->subIds();
        for (SubIds::const_iterator citer = subIds.cbegin();
             citer != subIds.cend();
             ++citer) {
            os << "(" << citer->subId() << ", " << citer->appId() << ")";

            SubIds::const_iterator citerCopy(citer);
            if ((++citerCopy) != subIds.end()) {
                os << ", ";
            }
        }

        os << mwcu::PrintUtil::newlineAndIndent(level + 3, spacesPerLevel)
           << "partitionId......: " << cit->partitionId();
        if (!cit->primaryNodeDescription().isNull()) {
            os << cit->primaryNodeDescription().value();
        }
        os << mwcu::PrintUtil::newlineAndIndent(level + 3, spacesPerLevel)
           << "key..............: " << cit->key()
           << mwcu::PrintUtil::newlineAndIndent(level + 3, spacesPerLevel)
           << "queue............: "
           << (cit->isCreated() ? "created" : "*NOT* created");

        // Contexts
        os << mwcu::PrintUtil::newlineAndIndent(level + 3, spacesPerLevel)
           << "nbContext........: " << cit->contexts().size();
        for (size_t ctxId = 0; ctxId != cit->contexts().size(); ++ctxId) {
            const Context& context = cit->contexts()[ctxId];
            os << mwcu::PrintUtil::newlineAndIndent(level + 4, spacesPerLevel)
               << "#" << (ctxId + 1) << ": "
               << "[handle parameters: " << context.queueHandleParametersJson()
               << "]";
        }
    }
}

void printResourceUsageMonitor(bsl::ostream&               os,
                               const ResourceUsageMonitor& usageMonitor,
                               int                         level,
                               int                         spacesPerLevel)
{
    os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << usageMonitor.state() << " "
       << "[Messages (" << usageMonitor.messagesState()
       << "): " << mwcu::PrintUtil::prettyNumber(usageMonitor.numMessages())
       << " ("
       << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
              usageMonitor.messagesLowWatermarkRatio() *
              usageMonitor.messagesCapacity()))
       << " - "
       << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
              usageMonitor.messagesHighWatermarkRatio() *
              usageMonitor.messagesCapacity()))
       << " - "
       << mwcu::PrintUtil::prettyNumber(usageMonitor.messagesCapacity())
       << "), "
       << "Bytes (" << usageMonitor.bytesState()
       << "): " << mwcu::PrintUtil::prettyBytes(usageMonitor.numBytes())
       << " ("
       << mwcu::PrintUtil::prettyBytes(usageMonitor.bytesLowWatermarkRatio() *
                                           usageMonitor.bytesCapacity(),
                                       2)
       << " - "
       << mwcu::PrintUtil::prettyBytes(usageMonitor.bytesHighWatermarkRatio() *
                                           usageMonitor.bytesCapacity(),
                                       2)
       << " - " << mwcu::PrintUtil::prettyBytes(usageMonitor.bytesCapacity())
       << ")] ";
}

void printQueueHandleSubStream(bsl::ostream&               os,
                               const QueueHandleSubStream& subStream,
                               int                         level,
                               int                         spacesPerLevel)
{
    os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "subId .................: " << subStream.subId();
    if (!subStream.appId().isNull()) {
        os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
           << "appId .................: " << subStream.appId().value();
    }

    os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "Stream Parameters .....: " << subStream.parametersJson()
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "UnconfirmedMonitors ...:"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "--------";

    for (size_t i = 0; i != subStream.unconfirmedMonitors().size(); ++i) {
        printResourceUsageMonitor(os,
                                  subStream.unconfirmedMonitors()[i],
                                  0,
                                  -1);
    }

    if (!subStream.numUnconfirmedMessages().isNull()) {
        os << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
           << "UnconfirmedMessages ...: "
           << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                  subStream.numUnconfirmedMessages().value()));
    }

    os << '\n';
}

void printQueueHandle(bsl::ostream&      os,
                      const QueueHandle& handle,
                      int                level,
                      int                spacesPerLevel)
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel)
       << "Handle Parameters .....: " << handle.parametersJson()
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "IsClientClusterMember .: " << bsl::boolalpha
       << handle.isClientClusterMember();

    typedef bsl::vector<QueueHandleSubStream> SubStreams;
    const SubStreams&                         subStreams = handle.subStreams();
    for (SubStreams::const_iterator cit = subStreams.begin();
         cit != subStreams.end();
         ++cit) {
        printQueueHandleSubStream(os, *cit, level, spacesPerLevel);
    }
}

void printQueueHandles(bsl::ostream&                   os,
                       const bsl::vector<QueueHandle>& handles,
                       int                             level,
                       int                             spacesPerLevel)
{
    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << handles.size() << " handles:\n";
    for (unsigned int idx = 1; idx <= handles.size(); ++idx) {
        os << mwcu::PrintUtil::indent(level + 2, spacesPerLevel) << idx << ". "
           << handles[idx - 1].clientDescription() << "\n";
        printQueueHandle(os, handles[idx - 1], level + 3, spacesPerLevel);
    }
}

void printQueueState(bsl::ostream&     os,
                     const QueueState& state,
                     int               level,
                     int               spacesPerLevel)
{
    os << mwcu::PrintUtil::indent(level, spacesPerLevel)
       << "State:" << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "------"
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "URI ...............: " << state.uri()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Handle Parameters .: " << state.handleParametersJson()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Stream Parameters .: " << state.streamParametersJson()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Id/Key ............: ";

    if (state.id() == bmqp::QueueId::k_UNASSIGNED_QUEUE_ID) {
        os << "unassigned";
    }
    else if (state.id() == bmqp::QueueId::k_PRIMARY_QUEUE_ID) {
        os << "primary";
    }
    else {
        os << state.id();
    }

    os << " ~ " << state.key()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "PartitionId .......: " << state.partitionId()
       << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Storage ...........: ";

    if (!state.storage().isNull()) {
        const QueueStorage& storage = state.storage().value();
        os << mwcu::PrintUtil::prettyNumber(
                  static_cast<bsls::Types::Int64>(storage.numMessages()))
           << " messages, " << mwcu::PrintUtil::prettyBytes(storage.numBytes())
           << mwcu::PrintUtil::newlineAndIndent(level + 2, spacesPerLevel)
           << "Num virtual storages: " << storage.virtualStorages().size();

        if (storage.virtualStorages().size()) {
            typedef bsl::vector<VirtualStorage> VirtualStorages;
            const VirtualStorages& virtualStorages = storage.virtualStorages();
            for (VirtualStorages::const_iterator cit =
                     virtualStorages.cbegin();
                 cit != virtualStorages.cend();
                 ++cit) {
                bdlb::Print::newlineAndIndent(os, level + 2, spacesPerLevel);
                os << cit->appId() << " : [appKey: " << cit->appKey()
                   << ", numMessages: "
                   << mwcu::PrintUtil::prettyNumber(
                          static_cast<bsls::Types::Int64>(cit->numMessages()))
                   << "]";
            }
        }
    }
    else {
        os << "none";
    }

    os << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
       << "Capacity Meter ....:";
    if (!state.capacityMeter().isNull()) {
        printCapacityMeter(os,
                           state.capacityMeter().value(),
                           level + 2,
                           spacesPerLevel);
    }
    else {
        os << "none";
    }

    os << "\n"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "QueueHandleCatalog:"
       << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
       << "-------------------";
    printQueueHandles(os, state.handles(), level + 1, spacesPerLevel);
    os << "\n";
}

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

void printQueueInternals(bsl::ostream&         os,
                         const QueueInternals& queueInternals,
                         int                   level,
                         int                   spacesPerLevel)
{
    os << "Queue Internals\n"
       << "---------------\n";

    // Print State Object
    printQueueState(os, queueInternals.state(), level + 1, spacesPerLevel);

    // Print Queue Object
    const Queue& queue = queueInternals.queue();
    if (queue.isLocalQueueValue()) {
        printLocalQueue(os, queue.localQueue(), level + 1, spacesPerLevel);
    }
    else if (queue.isRemoteQueueValue()) {
        printRemoteQueue(os, queue.remoteQueue(), level + 1, spacesPerLevel);
    }
    else {
        os << "Uninitialized queue !!\n";
    }
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
    printClusterStorageSummary(os,
                               clusterStatus.clusterStorageSummary(),
                               level + 1,
                               spacesPerLevel);
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

void printHelpCommands(bsl::ostream& os, const Help& helpCommands)
{
    typedef bsl::vector<CommandSpec> Commands;
    const Commands&                  commands = helpCommands.commands();
    bsl::string                      separator1;
    bsl::string                      separator2;

    if (helpCommands.isPlumbing()) {
        separator1 = "";
        separator2 = " - ";
    }
    else {
        separator1 = "    ";
        separator2 = "\n        ";
        os << "This process responds to the following CMD subcommands:\n";
    }

    for (Commands::const_iterator cit = commands.cbegin();
         cit != commands.cend();
         ++cit) {
        os << separator1 << cit->command() << separator2 << cit->description()
           << "\n";
    }
}

}  // close anonymous namespace

// -------------------
// struct HumanPrinter
// -------------------

bsl::ostream& HumanPrinter::print(bsl::ostream& os,
                                  const Result& result,
                                  int           level,
                                  int           spacesPerLevel)
{
    if (result.isErrorValue()) {
        os << result.error().message();
    }
    else if (result.isSuccessValue()) {
        os << "SUCCESS";
    }
    else if (result.isHelpValue()) {
        printHelpCommands(os, result.help());
    }
    else if (result.isDomainInfoValue()) {
        printDomainInfo(os, result.domainInfo(), level, spacesPerLevel);
    }
    else if (result.isClusterListValue()) {
        printClusterList(os, result.clusterList());
    }
    else if (result.isBrokerConfigValue()) {
        printBrokerConfig(os, result.brokerConfig());
    }
    else if (result.isTunableValue()) {
        printTunable(os, result.tunable());
    }
    else if (result.isTunablesValue()) {
        printTunableList(os, result.tunables());
    }
    else if (result.isTunableConfirmationValue()) {
        printTunableConfirmation(os, result.tunableConfirmation());
    }
    else if (result.isStatsValue()) {
        os << result.stats();
    }
    else if (result.isPurgedQueuesValue()) {
        printPurgedQueues(os, result.purgedQueues());
    }
    else if (result.isQueueContentsValue()) {
        printQueueContents(os, result.queueContents());
    }
    else if (result.isMessageValue()) {
        printMessage(os, result.message());
    }
    else if (result.isClusterStorageSummaryValue()) {
        printClusterStorageSummary(os,
                                   result.clusterStorageSummary(),
                                   level,
                                   spacesPerLevel);
    }
    else if (result.isStorageContentValue()) {
        printQueueStatus(os, result.storageContent(), level, spacesPerLevel);
    }
    else if (result.isClusterQueueHelperValue()) {
        printClusterQueueHelper(os,
                                result.clusterQueueHelper(),
                                level,
                                spacesPerLevel);
    }
    else if (result.isQueueInternalsValue()) {
        printQueueInternals(os,
                            result.queueInternals(),
                            level,
                            spacesPerLevel);
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
    return os;
}

bsl::ostream&
HumanPrinter::printResponses(bsl::ostream&            os,
                             const RouteResponseList& responseList)
{
    typedef bsl::vector<BloombergLP::mqbcmd::RouteResponse>
        RouteResponseVector;

    RouteResponseVector responses = responseList.responses();

    for (RouteResponseVector::const_iterator respIt = responses.begin();
         respIt != responses.end();
         ++respIt) {
        os << "Response from node \"" << respIt->source() << "\":";
        os << bsl::endl;
        os << respIt->response();
        os << bsl::endl;
    }
    return os;
}

}  // close package namespace
}  // close enterprise namespace
