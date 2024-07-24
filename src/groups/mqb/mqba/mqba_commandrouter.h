// Copyright 2024 Bloomberg Finance L.P.
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

// mqbblp_messagegroupidmanager.h                                     -*-C++-*-
#ifndef INCLUDED_MQBA_COMMANDROUTER
#define INCLUDED_MQBA_COMMANDROUTER

//@PURPOSE: Provide a class responsible for routing admin commands to the
// subset of cluster nodes that should execute that command.
// 
// This currently only supports routing cluster related commands (i.e. cluster
// commands and domain commands). There are 2 main routing modes supported:
// routing to primary node(s) or to all nodes in the cluster. The following
// commands are supported:
// * Primary commands
//   * DOMAINS DOMAIN <domain> PURGE
//   * DOMAINS DOMAIN <name> QUEUE <queue_name> PURGE <appId>
//   * CLUSTERS CLUSTER <name> FORCE_GC_QUEUES
//   * CLUSTERS CLUSTER <name> STORAGE PARTITION <partitionId> [ENABLE|DISABLE]
// * Cluster commands
//   * DOMAINS RECONFIGURE <domain>
//   * CLUSTERS CLUSTER <name> STORAGE REPLICATION SET_ALL <parameter> <value>
//   * CLUSTERS CLUSTER <name> STORAGE REPLICATION GET_ALL <parameter>
//   * CLUSTERS CLUSTER <name> STATE ELECTOR SET_ALL <parameter> <value>
//   * CLUSTERS CLUSTER <name> STATE ELECTOR GET_ALL <parameter>
//
// Routing operates on mqbnet::ClusterNode pointers, which currently limits our
// ability to route non-cluster commands. In order to support routing non
// cluster related commands, this class would need to be generalized or add
// specialized cases for routing using a more fundamental abstraction (i.e. a
// Session or Channel). This is beyond the scope of current needs at the time
// of writing this feature.
//
// Additionally, be aware of the following non-fatal "deadlock" scenario.
// Currently, if two cluster nodes receive a command simultaneously and both
// of these nodes route to each other, they will both deadlock (because they
// cannot start executing the routed commands until they receive a response).
// This deadlock is short-lived, though, as both commands will just end up
// timing out. This scenario can be mitigated by increasing the number of
// command processing threads from 1.
//
//@CLASSES:
//  mqbcmd::CommandRouter: Manages routing a single admin command. This class
//    is designed to be used once per command and should be destructed after
//    the command has been processed.

// BSL
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bslmt_latch.h>
#include <bslstl_sharedptr.h>

// MQB
#include <mqbcmd_messages.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqp_ctrlmsg {
class ControlMessage;
}
namespace mqbi {
class Cluster;
}
namespace mqbnet {
class ClusterNode;
}
namespace mqbnet {
template <class REQUEST, class RESPONSE, class TARGET>
class MultiRequestManagerRequestContext;
}

namespace mqba {

class CommandRouter {
  public:
    typedef bsl::shared_ptr<
        mqbnet::MultiRequestManagerRequestContext<bmqp_ctrlmsg::ControlMessage,
                                                  bmqp_ctrlmsg::ControlMessage,
                                                  mqbnet::ClusterNode*> >
                                              MultiRequestContextSp;
    typedef bsl::vector<mqbnet::ClusterNode*> NodesVector;

  private:
    struct RouteMembers {
        NodesVector d_nodes;  // Proxy nodes and the self node should never be
                              // route members.
        bool d_self;  // True if the command should execute on this node
    };

    class RoutingMode {
      public:
        RoutingMode();
        virtual ~RoutingMode() = 0;

        virtual RouteMembers getRouteMembers(mqbcmd::InternalResult* result,
                                             mqbi::Cluster* cluster) = 0;
    };
    class AllPartitionPrimariesRoutingMode : public RoutingMode {
      public:
        AllPartitionPrimariesRoutingMode();

        RouteMembers
        getRouteMembers(mqbcmd::InternalResult* result,
                        mqbi::Cluster*          cluster) BSLS_KEYWORD_OVERRIDE;
    };
    class SinglePartitionPrimaryRoutingMode : public RoutingMode {
      private:
        int d_partitionId;

      public:
        SinglePartitionPrimaryRoutingMode(int partitionId);

        RouteMembers
        getRouteMembers(mqbcmd::InternalResult* result,
                        mqbi::Cluster*          cluster) BSLS_KEYWORD_OVERRIDE;
    };
    class ClusterRoutingMode : public RoutingMode {
      public:
        ClusterRoutingMode();

        RouteMembers
        getRouteMembers(mqbcmd::InternalResult* result,
                        mqbi::Cluster*          cluster) BSLS_KEYWORD_OVERRIDE;
    };

  public:
    typedef bslma::ManagedPtr<RoutingMode> RoutingModeMp;

  private:
    const bsl::string&           d_commandString;
    const mqbcmd::Command&       d_commandWithOptions;
    const mqbcmd::CommandChoice& d_command;

    mqbcmd::RouteResponseList d_responses;

    RoutingModeMp d_routingMode;

    bslmt::Latch d_latch;

  public:
    /// Sets up a command router with the given command string and parsed
    /// command object. This will
    CommandRouter(const bsl::string&     commandString,
                  const mqbcmd::Command& commandWithOptions);

    /// Returns true if this command router is necessary to route the command
    /// that it was set up with. If the command does not require routing, then
    /// this function returns false.
    bool isRoutingNeeded() const;

    /// Performs any routing on the command and returns true if the caller
    /// should also execute the command.
    bool route(mqbcmd::InternalResult* result, mqbi::Cluster* relevantCluster);

    /// Waits on a latch that triggers when the responses have been received.
    void waitForResponses();

    /// Returns a reference to the collected responses from routing.
    // ResponseMessages& responses();
    mqbcmd::RouteResponseList& responses();

  private:
    RoutingModeMp getCommandRoutingMode();

    void countDownLatch();

    void onRouteCommandResponse(const MultiRequestContextSp& requestContext);

    void routeCommand(const NodesVector& nodes);
};

inline bool CommandRouter::isRoutingNeeded() const
{
    return d_routingMode.get() != nullptr;
}

inline mqbcmd::RouteResponseList& CommandRouter::responses()
{
    return d_responses;
}

inline void CommandRouter::waitForResponses()
{
    d_latch.wait();
}

inline void CommandRouter::countDownLatch()
{
    d_latch.countDown(1);
}

}  // close package namespace
}  // close enterprise namespace

#endif
