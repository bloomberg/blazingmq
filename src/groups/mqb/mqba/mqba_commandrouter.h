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
// * Cluster-wide commands
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
//@CLASSES:
//  mqbcmd::CommandRouter: Manages routing a single admin command. This class
//    is designed to be used once per command and should be destructed after
//    the command has been processed.

// BDE
#include <ball_log.h>
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
  private:
    // PRIVATE TYPES

    /// Shared pointer to the correct multirequest context for routing control
    /// messages to `mqbnet::ClusterNode`s
    typedef bsl::shared_ptr<
        mqbnet::MultiRequestManagerRequestContext<bmqp_ctrlmsg::ControlMessage,
                                                  bmqp_ctrlmsg::ControlMessage,
                                                  mqbnet::ClusterNode*> >
        MultiRequestContextSp;

    /// Vector of `mqbnet::ClusterNode` pointers used for routing
    typedef bsl::vector<mqbnet::ClusterNode*> NodesVector;

  private:
    /// Struct representing which nodes a command should be routed to. Contains
    /// both a list of external nodes to route to and a flag indicating whether
    /// the self node should execute the command.
    struct RouteTargets {
        NodesVector d_nodes;  // Proxy nodes and the self node should never be
                              // route members.
        bool d_self;  // True if the command should execute on the self node.
    };

    // ==================
    // class Routing Mode
    // ==================

    /// Private interface to implement various methods of choosing routing
    /// targets.
    class RoutingMode {
      public:
        RoutingMode();
        virtual ~RoutingMode() = 0;

        /// Populates the given `routeMembers` struct with the proper nodes to
        /// route to from the given `cluster`. Returns 0 on success or a
        /// non-zero error code on failure. Populates the given
        /// `errorDescription` output stream.
        virtual int getRouteTargets(bsl::ostream&  errorDescription,
                                    RouteTargets*  routeMembers,
                                    mqbi::Cluster* cluster) = 0;
    };
    class AllPartitionPrimariesRoutingMode : public RoutingMode {
      public:
        /// Used to route the command to all nodes which are a primary of any
        /// partition
        AllPartitionPrimariesRoutingMode();

        /// Collects all nodes which are a primary for some partition of the
        /// given `cluster`.
        int getRouteTargets(bsl::ostream&  errorDescription,
                            RouteTargets*  routeMembers,
                            mqbi::Cluster* cluster) BSLS_KEYWORD_OVERRIDE;
    };
    class SinglePartitionPrimaryRoutingMode : public RoutingMode {
      private:
        /// The id of the partition's primary to route to.
        int d_partitionId;

      public:
        /// Used to route the command to the primary of the given
        /// `partitionId`.
        SinglePartitionPrimaryRoutingMode(int partitionId);

        /// Collects the node which is the primary of the partition this
        /// routing mode was constructed with on the given `cluster`.
        int getRouteTargets(bsl::ostream&  errorDescription,
                            RouteTargets*  routeMembers,
                            mqbi::Cluster* cluster) BSLS_KEYWORD_OVERRIDE;
    };
    class ClusterWideRoutingMode : public RoutingMode {
      public:
        /// Used to route the command to all nodes in a cluster.
        ClusterWideRoutingMode();

        /// Collects all nodes in the given `cluster`.
        int getRouteTargets(bsl::ostream&  errorDescription,
                            RouteTargets*  routeMembers,
                            mqbi::Cluster* cluster) BSLS_KEYWORD_OVERRIDE;
    };

  private:
    // PRIVATE TYPES
    typedef bslma::ManagedPtr<RoutingMode> RoutingModeMp;

    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBA.COMMANDROUTER");

    // DATA

    /// The raw string representation of the command, which is what would be
    /// routed to other nodes.
    const bsl::string& d_commandString;

    /// The command object we are routing
    const mqbcmd::Command& d_command;

    /// All collected responses from routing.
    mqbcmd::RouteResponseList d_responses;

    /// A managed pointer to the routing mode instance to use on this command.
    RoutingModeMp d_routingModeMp;

    /// Synchronization mechanism used to wait for all responses to come back.
    bslmt::Latch d_latch;

    // MANIPULATORS

    /// Sets `d_routingModeMp` to the proper routing mode instance for the
    /// command associated with this command router. If this command should
    /// not be routed, then `d_routingModeMp` is kept as a nullptr and this
    /// function returns false. Otherwise, on success, it returns true.
    void setCommandRoutingMode();

    /// Counts down the latch associated with this command route by one,
    /// effectively releasing the latch since it is always initialized with
    /// a value of 1. This should be called either when routing isn't needed
    /// or when all responses from routed nodes have been received.
    void releaseLatch();

    /// Callback function that runs when all responses to routed nodes have
    /// been received.
    void onRouteCommandResponse(const MultiRequestContextSp& requestContext);

    /// Routes the command associated with this command router to the given
    /// nodes. Each of these nodes should be external (i.e. not the self node).
    void routeCommand(const NodesVector& nodes);

  public:
    // CREATORS

    /// Sets up a command router with the given `commandString` and parsed
    /// `command` object.
    CommandRouter(const bsl::string&     commandString,
                  const mqbcmd::Command& command);

    // MANIPULATORS

    /// Performs any routing on the command using the given `relevantCluster`
    /// and returns true if the caller should also execute the command.
    int route(bsl::ostream&  errorDescription,
              bool*          selfShouldExecute,
              mqbi::Cluster* relevantCluster);

    /// Waits on a latch that triggers when the responses have been received.
    void waitForResponses();

    /// Returns a reference to the collected responses from routing.
    mqbcmd::RouteResponseList& responses();

    // ACCESSORS

    /// Returns true if this command needs to be routed, i.e. if
    /// `d_routingModeMp` has been populated with a routing mode instance.
    bool isRoutingNeeded() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline void CommandRouter::waitForResponses()
{
    d_latch.wait();
}

inline mqbcmd::RouteResponseList& CommandRouter::responses()
{
    return d_responses;
}

inline void CommandRouter::releaseLatch()
{
    d_latch.countDown(1);
}

inline bool CommandRouter::isRoutingNeeded() const
{
    return d_routingModeMp;
}

}  // close package namespace
}  // close enterprise namespace

#endif
