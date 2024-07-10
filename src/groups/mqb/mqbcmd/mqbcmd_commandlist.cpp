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

// mqbcmd_commandlist.cpp                                             -*-C++-*-
#include <mqbcmd_commandlist.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_messages.h>

// BDE
#include <bdlb_arrayutil.h>

namespace BloombergLP {
namespace mqbcmd {

namespace {

struct CommandDefinition {
    const char* d_command;      // Command name and argument syntax
    const char* d_summary;      // Short description (one line command summary)
    const char* d_description;  // Long description.
} k_COMMANDS[] = {
    // Help
    {"HELP",
     "Show list of supported commands",
     "Show list of supported commands"},
    // BrokerConfig
    {"BROKERCONFIG DUMP",
     "Dump the broker's configuration",
     "Dump the broker's configuration"},
    // DomainManager
    {"DOMAINS DOMAIN <name> PURGE",
     "Purge all queues in domain 'name'",
     "Purge all queues in domain 'name'"},
    {"DOMAINS DOMAIN <name> INFOS",
     "Show information about domain 'name' and its queues",
     "Show information about domain 'name' and its queues"},
    {"DOMAINS DOMAIN <name> QUEUE <queue_name> PURGE <appId>",
     "Purge the 'appId' in the 'queue_name' belonging to domain 'name'.",
     "Purge the 'appId' in the 'queue_name' belonging to domain 'name'. "
     "Specify '*' for 'appId' for entire queue"},
    {"DOMAINS DOMAIN <name> QUEUE <queue_name> INTERNALS",
     "internals of 'queue_name' belonging to domain 'name'",
     "Dump the internals of 'queue_name' belonging to domain 'name'"},
    {"DOMAINS DOMAIN <name> QUEUE <queue_name> LIST [<appId>] <offset> "
     "<count>",
     "List 'count' messages pending in 'queue_name' for  (optional) 'appId'"
     " starting at 'offset'",
     "List information for 'count' messages starting at 'offset' from "
     "'queue_name' for 'appId' in domain 'name'.  If 'offset' is negative, "
     "it is relative to the position just past the *last* message.  If "
     "'count' is 'negative', print '-count' messages *preceding* the "
     "specified starting position.  If 'count' is 'UNLIMITED', print all "
     "the messages starting at and after the specified position."},
    {"DOMAINS RECONFIGURE <domain>",
     "Reconfigure 'domain' by reloading its configuration from disk",
     "Reconfigure 'domain' by reloading its configuration from disk"},
    {"DOMAINS RESOLVER CACHE_CLEAR (<domain>|ALL)",
     "Clear the domain's cached resolution.",
     "Clear the domain resolution cache entry of the optionally specified "
     "'domain', or clear all domain resolution cache entries if 'ALL' is "
     "specified."},
    // ConfigProvider
    {"CONFIGPROVIDER CACHE_CLEAR (<domain>|ALL)",
     "Clear domain's cached configuration.",
     "Clear the cached configuration entry of the optionally specified "
     "'domain' domain, or clear all cached configuration entries if 'ALL' "
     "is specified."},
    // StatController
    {"STAT SHOW", "Show statistics", "Show statistics"},
    {"STAT SET <parameter> <value>",
     "Set the 'parameter' of the stat controller to 'value'",
     "Set the 'parameter' of the stat controller to 'value'"},
    {"STAT GET <parameter>",
     "Get the 'parameter' of the stat controller",
     "Get the 'parameter' of the stat controller"},
    {"STAT LIST_TUNABLES",
     "Get the supported settable parameters for the stat controller",
     "Get the supported settable parameters for the stat controller"},
    // ClusterCatalog
    {"CLUSTERS LIST", "List all active clusters", "List all active clusters"},
    {"CLUSTERS ADDREVERSE <clusterName> <remotePeer>",
     "Create a new reverse connection to 'remotePeer' about 'clusterName'",
     "Create a new reverse connection to 'remotePeer' about 'clusterName'"},
    {
        "CLUSTERS CLUSTER <name> STATUS",
        "Show status of cluster 'name'",
        "Show status of cluster 'name'",
    },
    {"CLUSTERS CLUSTER <name> QUEUEHELPER",
     "Show queueHelper's internal state of cluster 'name'",
     "Show queueHelper's internal state of cluster 'name'"},
    {"CLUSTERS CLUSTER <name> FORCE_GC_QUEUES",
     "Force GC all queues matching GC criteria",
     "Force GC all queues matching GC criteria"},
    {"CLUSTERS CLUSTER <name> STORAGE SUMMARY",
     "Show storage summary of cluster 'name'",
     "Show storage summary of cluster 'name'"},
    {"CLUSTERS CLUSTER <name> STORAGE PARTITION <partitionId> "
     "[ENABLE|DISABLE]",
     "Enable/disable the 'partitionId' of cluster 'name'",
     "Enable/disable the 'partitionId' of cluster 'name'"},
    {"CLUSTERS CLUSTER <name> STORAGE PARTITION <partitionId> SUMMARY",
     "Show summary of the 'partitionId' of cluster 'name'",
     "Show summary of the 'partitionId' of cluster 'name'"},
    {"CLUSTERS CLUSTER <name> STORAGE DOMAIN <domain_name> QUEUE_STATUS",
     "Show status of queues belonging to 'domain_name' in the storage of "
     "cluster 'name'",
     "Show status of queues belonging to 'domain_name' in the storage of "
     "cluster 'name'"},
    {
        "CLUSTERS CLUSTER <name> STORAGE REPLICATION [SET|SET_ALL] <parameter> <value>",
        "Set the value of the replication 'parameter' of cluster 'name'",
        "Set the value of the replication 'parameter' of cluster 'name'. If SET_ALL is used then set the parameter for all nodes in the cluster.",
    },
    {"CLUSTERS CLUSTER <name> STORAGE REPLICATION [GET|GET_ALL] <parameter>",
     "Get the value of the replication 'parameter' of cluster 'name'",
     "Get the value of the replication 'parameter' of cluster 'name'. If GET_ALL is used then get the paramter value for all nodes in the cluster."},
    {"CLUSTERS CLUSTER <name> STORAGE REPLICATION LIST_TUNABLES",
     "Get the supported settable parameters for the replication of cluster "
     "'name'",
     "Get the supported settable parameters for the replication of cluster "
     "'name'"},
    {"CLUSTERS CLUSTER <name> STATE ELECTOR [SET|SET_ALL] <parameter> <value>",
     "Set the 'parameter' of the elector of cluster 'name' to 'value'",
     "Set the 'parameter' of the elector of cluster 'name' to 'value'. If SET_ALL is used then set the paramter for all nodes in the cluster."},
    {"CLUSTERS CLUSTER <name> STATE ELECTOR [GET|GET_ALL] <parameter>",
     "Get the 'parameter' of the elector of cluster 'name'",
     "Get the 'parameter' of the elector of cluster 'name'. If GET_ALL is used then get the parameter value for all nodes in the cluster."},
    {"CLUSTERS CLUSTER <name> STATE ELECTOR LIST_TUNABLES",
     "Get the supported settable parameters for the elector of cluster "
     "'name'",
     "Get the supported settable parameters for the elector of cluster "
     "'name'"}};

}  // close anonymous namespace

// -----------------
// class CommandList
// -----------------

void CommandList::loadCommands(mqbcmd::Help* out, const bool isPlumbing)
{
    bsl::vector<CommandSpec>& commands = out->commands();
    commands.reserve(BDLB_ARRAYUTIL_SIZE(k_COMMANDS));
    out->isPlumbing() = isPlumbing;

    for (const CommandDefinition* it = k_COMMANDS;
         it != bdlb::ArrayUtil::end(k_COMMANDS);
         ++it) {
        commands.resize(commands.size() + 1);

        CommandSpec& command  = commands.back();
        command.command()     = it->d_command;
        command.description() = isPlumbing ? it->d_summary : it->d_description;
    }
}

}  // close package namespace
}  // close enterprise namespace
