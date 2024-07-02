// Copyright 2017-2023 Bloomberg Finance L.P.
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
//@CLASSES:
//  mqbcmd::CommandRouter: Manages routing admin commands
//
//@DESCRIPTION:
//

// BSL
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
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
    class Router {
      private:
        CommandRouter* d_router;

      public:
        Router(CommandRouter* router);
        virtual ~Router() = 0;

        virtual bool   routeCommand() = 0;
        CommandRouter* router();
    };
    class AllPartitionPrimariesRouter : public Router {
      public:
        AllPartitionPrimariesRouter(CommandRouter* router);

        bool routeCommand() BSLS_KEYWORD_OVERRIDE;
    };
    class SinglePartitionPrimaryRouter : public Router {
      private:
        int d_partitionId;

      public:
        SinglePartitionPrimaryRouter(CommandRouter* router);

        void setPartitionID(int id);
        bool routeCommand() BSLS_KEYWORD_OVERRIDE;
    };
    class ClusterRouter : public Router {
      public:
        ClusterRouter(CommandRouter* router);

        bool routeCommand() BSLS_KEYWORD_OVERRIDE;
    };
  public:
    typedef bsl::shared_ptr<
        mqbnet::MultiRequestManagerRequestContext<bmqp_ctrlmsg::ControlMessage,
                                                  bmqp_ctrlmsg::ControlMessage,
                                                  mqbnet::ClusterNode*> >
        MultiRequestContextSp;
    typedef bsl::vector<bsl::pair<mqbnet::ClusterNode*, bsl::string> >
            ResponseMessages;
    typedef bsl::vector<mqbnet::ClusterNode*> NodesVector;
  private:
    // store an instance of each type of router
    AllPartitionPrimariesRouter  d_allPartitionPrimariesRouter;
    SinglePartitionPrimaryRouter d_singlePartitionPrimaryRouter;
    ClusterRouter                d_clusterRouter;

    bslmt::Latch d_latch;

    const bsl::string&           d_commandString;
    const mqbcmd::Command&       d_commandWithOptions;
    const mqbcmd::CommandChoice& d_command;

    ResponseMessages d_responses;

    Router* d_router;

    mqbi::Cluster* d_cluster;
  public:
    /// Sets up a command router with the given command string and parsed
    /// command object. This will  
    CommandRouter(const bsl::string& commandString, const mqbcmd::Command& command);

    /// Returns true if this command router is necessary to route the command
    /// that it was set up with. If the command does not require routing, then
    /// this function returns false.
    bool isRoutingNeeded() const;

    /// Performs any routing on the command and returns true if the caller
    /// should also execute the command.
    bool processCommand(mqbi::Cluster* cluster);

    /// Waits on a latch that triggers when the responses have been received.
    void waitForResponses();

    /// Returns a reference to the collected responses from routing.
    ResponseMessages& responses();

    /// Returns a pointer to the relevant cluster for this 
    /// command. The pointer can be guaranteed to be non-null.
    mqbi::Cluster* cluster() const;

  private:
    Router* getCommandRouter();

    void countDownLatch();

    void onRouteCommandResponse(const MultiRequestContextSp& requestContext);

    void routeCommand(const NodesVector& nodes);
};

inline CommandRouter* CommandRouter::Router::router() {
    return d_router;
}

inline void CommandRouter::SinglePartitionPrimaryRouter::setPartitionID(int id)
{
    d_partitionId = id;
}

inline CommandRouter::ResponseMessages& CommandRouter::responses() {
    return d_responses;
}

inline void CommandRouter::waitForResponses() {
    d_latch.wait();
}

inline mqbi::Cluster* CommandRouter::cluster() const {
    BSLS_ASSERT_SAFE(d_cluster);

    return d_cluster;
}

inline void CommandRouter::countDownLatch() {
    d_latch.countDown(1);
}

}  // close package namespace
}  // close enterprise namespace

#endif
