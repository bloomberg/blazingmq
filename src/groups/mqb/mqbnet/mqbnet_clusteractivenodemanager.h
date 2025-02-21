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

// mqbnet_clusteractivenodemanager.h                                  -*-C++-*-
#ifndef INCLUDED_MQBNET_CLUSTERACTIVENODEMANAGER
#define INCLUDED_MQBNET_CLUSTERACTIVENODEMANAGER

//@PURPOSE: Provide a mechanism to manage an active node from a cluster.
//
//@CLASSES:
//  mqbnet::ClusterActiveNodeManager: mechanism to manage an active node
//
//@DESCRIPTION: 'mqbnet::ClusterActiveNodeManager' is a mechanism to manage a
// single active node from an associated 'cluster' object.
//
/// Active node selection
///---------------------
// A cluster is a pool of nodes ('mqbnet::ClusterNode') which are always
// connected; however in some situations ('mqbblp::ClusterProxy' for example),
// it is desirable to only use one node at a time for communication.  We favour
// a node from the same data center as the current machine.  The nodes'
// connectivity is dynamic and they can go up and down.  When the currently
// active node goes down, a new one is immediately selected.  Transitioning
// from one node to another can potentially introduce some temporary hiccup and
// glitches such as latency added, and therefore once a node has been selected,
// we never switch until it goes down.
//
// All the active node selection logic is in the 'findNewActiveNode()' method.
// There is a special handling for the very beginning: upon initialization of
// the 'ClusterActiveNodeManager', the associated 'mqbnet::Cluster' has just
// been created and the connections may not yet have been (all) established.
// We can't simply use the first node up as the active one, because it may not
// be one in the same data center; and we also must not assume there will
// always be at least one node from the same data center.  The solution is that
// during initialization of the object an event is scheduled.  If a node comes
// up in the same data center before the event expires (likely situation), this
// node is picked as the active and the event is canceled; otherwise, if the
// event fires, we assume we waited long enough and must pick a node regardless
// of the data center.
//
// Note that while looking for a candidate for the active node, we use a
// randomized pick within the candidates so that, hopefully, all brokers will
// not choose the same node in the cluster, and those will have an evenly
// distributed connection from the remotes.
//
/// Thread Safety
///-------------
// The 'mqbnet::ClusterActiveNodeManager' object is not thread safe.  It
// expects to be called in the same thread.
//
/// Usage Example
///-------------
// This example shows typical usage of the 'ClusterActiveNodeManager' object.
//
//..
//  class ClusterProxy {
//    private:
//      TransportManager                 *d_transportManager_p;
//      mqbnet::ClusterActiveNodeManager  d_activeNodeManager;
//
//    private:
//      // PRIVATE MANIPULATORS
//      void startDispatched(bsl::ostream *errorDescription,
//                           int          *rc);
//      void onNodeStateChange(
//                     mqbnet::ClusterNode                           *node,
//                     const bsl::shared_ptr<const mqbnet::Session>&  session);
//
//      void refreshActiveNodeManager();
//
//      void processActiveNodeManagerResult(int result);
//
//    public:
//      void onNodeStateChange(
//                    mqbnet::ClusterNode                           *node,
//                    const bsl::shared_ptr<const mqbnet::Session>&  session);
//      int sendData(const bdlbb::Blob& blob);
//  };
//
//  void
//  ClusterProxy::startDispatched(bsl::ostream *errorDescription,
//                                int          *rc)
//  {
//                                       // executed by the *DISPATCHER* thread
//
//    // PRECONDITIONS
//    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
//
//    d_clusterData.membership().netCluster()->registerObserver(this);
//
//    d_activeNodeManager.initialize(
//                            d_clusterData.membership().netCluster()->nodes(),
//                            d_transportManager_p);
//
//    dispatcher()->execute(
//              bdlf::BindUtil::bind(&ClusterProxy::refreshActiveNodeManager,
//                                   this),
//              this);
//  }
//
//  void
//  ClusterProxy::refreshActiveNodeManager()
//  {
//                                       // executed by the *DISPATCHER* thread
//
//    // PRECONDITIONS
//    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
//
//    int result = d_activeNodeManager.refresh();
//
//    processActiveNodeManagerResult(result);
//  }
//
//  void
//  ClusterProxy::onNodeStateChange(
//                      mqbnet::ClusterNode                           *node,
//                      const bsl::shared_ptr<const mqbnet::Session>&  session)
//  {
//    dispatcher()->execute(
//            bdlf::BindUtil::bind(&ClusterProxy::onNodeStateChange,
//                                 this,
//                                 node,
//                                 session),
//            this);
//  }
//
//  void
//  ClusterProxy::onNodeStateChange(
//                      mqbnet::ClusterNode                           *node,
//                      const bsl::shared_ptr<const mqbnet::Session>&  session)
//  {
//                                       // executed by the *DISPATCHER* thread
//
//    // PRECONDITIONS
//    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
//
//    int result = d_activeNodeManager.onNodeStateChange(node, session);
//
//    processActiveNodeManagerResult(result);
//  }
//
//  void
//  ClusterProxy::processActiveNodeManagerResult(int result)
//  {
//                                       // executed by the *DISPATCHER* thread
//
//    // PRECONDITIONS
//    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
//
//    if (result & mqbnet::ClusterActiveNodeManager::e_LOST_ACTIVE) {
//        // we lost our active node
//        onActiveNodeDown();
//    }
//    if (result & mqbnet::ClusterActiveNodeManager::e_NEW_ACTIVE) {
//        // Cancel the scheduler event, if any.
//        if (d_activeNodeLookupEventHandle) {
//            d_clusterData.scheduler()->cancelEvent(
//                                            &d_activeNodeLookupEventHandle);
//        }
//        // new active node
//        onActiveNodeUp(d_activeNodeManager.activeNode());
//    }
//
//  }
//
//  bmqio::StatusCategory::Enum
//  ClusterProxy::sendData(const bdlbb::Blob& blob)
//  {
//    return d_activeNodeManager.write(0, blob, 64 * 1024 * 1024);
//  }
//..
//

// MQB

#include <mqbi_dispatcher.h>
#include <mqbnet_cluster.h>
#include <mqbnet_transportmanager.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

#include <bmqio_channel.h>
#include <bmqio_status.h>

// BDE
#include <ball_log.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsls_assert.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class NodeStatuses;
}

namespace mqbnet {

// ==============================
// class ClusterActiveNodeManager
// ==============================

/// Mechanism to manage an active node from an associated cluster.
class ClusterActiveNodeManager {
  public:
    // public TYPES

    enum eResult {
        e_NO_CHANGE   = 0,
        e_LOST_ACTIVE = (1 << 0),
        e_NEW_ACTIVE  = (1 << 1)
    };

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.CLUSTERACTIVENODEMANAGER");

    // PRIVATE TYPES

    struct NodeContext {
        bmqp_ctrlmsg::NodeStatus::Value d_status;

        bsl::string d_sessionDescription;
        // Current session description
    };

    typedef bsl::map<mqbnet::ClusterNode*, NodeContext> NodesMap;

  private:
    // DATA
    bsl::string d_description;
    // A description string, used while
    // printing logs.

    bsl::string d_dataCenter;
    // The dataCenter the current machine
    // resides in.

    NodesMap d_nodes;
    // Cached nodes status and sessions.

    NodesMap::const_iterator d_activeNodeIt;
    // Pointer to the currently active node
    // and its context.

    bool d_ignoreDataCenter;
    // If true, remove the data center
    // requirement when selecting active
    // node. Set to true when the cluster
    // does not have any nodes in the
    // current machine's data center.

    bool d_useExtendedSelection;
    // If true, drop the same data center
    // requirement when selecting active
    // node.

  private:
    // PRIVATE MANIPULATORS

    /// Find the most suitable node to use as the active one, privileging
    /// same data center as the current machine, and taking into account an
    /// optional initial delay for connection to establish.
    bool findNewActiveNode();

    /// Method invoked when the active node has been switched to the
    /// specified `node`.
    void onNewActiveNode(ClusterNode* node);

    /// Process status change of the specified `context` associated with the
    /// specified `node`.  The specified `oldStatus` indicates previous
    /// status.  This can trigger searching for active node.  Return bitmask
    /// e_NO_CHANGE/e_LOST_ACTIVE/e_NEW_ACTIVE.
    int processNodeStatus(const ClusterNode*              node,
                          const NodeContext&              context,
                          bmqp_ctrlmsg::NodeStatus::Value oldStatus);

  public:
    // CREATORS

    /// Create a new object managing an active for the specified `nodes`.
    /// Consider the machine to belong to the specified `dataCenter` when
    /// selecting an active, and use the specified `description`.
    ClusterActiveNodeManager(const Cluster::NodesList& nodes,
                             const bslstl::StringRef&  description,
                             const bsl::string&        dataCenter);

    /// Destructor.
    virtual ~ClusterActiveNodeManager();

    // MANIPULATORS

    /// drop the `same DataCenter` requirement for all subsequent active
    /// node selections.
    void enableExtendedSelection();

    /// Initialize this object.  Iterate sessions from the specified
    /// `transportManager` and update corresponding nodes status.
    void initialize(TransportManager* transportManager);

    /// Find active node if none is available.
    /// Return bitmask e_NO_CHANGE / e_LOST_ACTIVE / e_NEW_ACTIVE.
    int refresh();

    int onNodeUp(ClusterNode*                        node,
                 const bmqp_ctrlmsg::ClientIdentity& identity);

    /// Notification methods to indicate change in connectivity for the
    /// specified `node`.  The specified `channel` and the specified
    /// `identity` indicate new connection properties in case the `node` is
    /// connected.  Return bitmask e_NO_CHANGE/e_LOST_ACTIVE/e_NEW_ACTIVE.
    int onNodeDown(ClusterNode* node);

    /// Notification methods to indicate status change for the specified
    /// `node` to the specified `status`.
    /// Return bitmask e_NO_CHANGE/e_LOST_ACTIVE/e_NEW_ACTIVE.
    int onNodeStatusChange(ClusterNode*                    node,
                           bmqp_ctrlmsg::NodeStatus::Value status);

    // ACCESSORS

    /// Return the currently active node, or a null pointer if there are no
    /// node currently active.
    ClusterNode* activeNode() const;

    /// Load connectivity state and node status for each cluster node to the
    /// `out` object.
    void loadNodesInfo(mqbcmd::NodeStatuses* out) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------------
// class ClusterActiveNodeManager
// ------------------------------

inline void ClusterActiveNodeManager::enableExtendedSelection()
{
    d_useExtendedSelection = true;
}

inline mqbnet::ClusterNode* ClusterActiveNodeManager::activeNode() const
{
    return d_activeNodeIt == d_nodes.end() ? 0 : d_activeNodeIt->first;
}

}  // close package namespace
}  // close enterprise namespace

#endif
