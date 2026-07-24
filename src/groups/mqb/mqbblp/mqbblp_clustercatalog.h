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

#ifndef INCLUDED_MQBBLP_CLUSTERCATALOG
#define INCLUDED_MQBBLP_CLUSTERCATALOG

/// @file mqbblp_clustercatalog.h
///
/// @brief Provide a catalog for building and retrieving `Cluster` objects.
///
/// @bbref{mqbblp::ClusterCatalog} is a mechanism to manage all the cluster
/// components (implementing the @bbref{mqbi::Cluster} interface).  It is in
/// charge of loading the clusters' definition and creating the cluster object
/// this broker is part of.  Clusters are reused when queried, and lazily
/// constructed if not yet created.
///
/// Thread-safety                               {#mqbblp_clustercatalog_thread}
/// =============
///
/// This object is *thread* *enabled*, meaning that two threads can safely call
/// any methods on the *same* *instance* without external synchronization.

// MQB
#include <mqbcfg_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_domain.h>
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_multirequestmanager.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqma_countingallocatorstore.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace mqbcmd {
class ClustersCommand;
}
namespace mqbcmd {
class ClustersResult;
}
namespace mqbi {
class Dispatcher;
}
namespace mqbnet {
class Cluster;
}
namespace mqbnet {
class InitialConnectionContext;
}
namespace mqbnet {
class TransportManager;
}
namespace bmqst {
class StatContext;
}

namespace mqbblp {

// ====================
// class ClusterCatalog
// ====================

/// Mechanism to manage a catalog of cluster objects.
class ClusterCatalog {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.CLUSTERCATALOG");

  public:
    // TYPES

    /// Class holding some context state used during negotiation: refer to the
    /// @bbref{ mqba::SessionNegotiator} for usage of that class.  This
    /// `userData` is created here, passed to `mqbnet` to hold on to it and
    /// deliver it back to the `Negotiator`, which recovers the concrete type
    /// by downcasting.  Implementing the @bbref{mqbnet::NegotiationUserData}
    /// protocol keeps the `mqbnet` layer abstracted away from this logic and
    /// avoids dependency cycles.
    class NegotiationUserData : public mqbnet::NegotiationUserData {
      private:
        // DATA

        /// Name of the cluster to negotiate the connection with.
        const bsl::string d_clusterName;

      public:
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(NegotiationUserData,
                                       bslma::UsesBslmaAllocator)

        // CREATORS

        /// Create a `NegotiationUserData` holding the specified `clusterName`.
        /// Use the optionally specified `allocator` to supply memory; if
        /// `allocator` is 0, use the currently installed default allocator.
        explicit NegotiationUserData(bsl::string_view  clusterName,
                                     bslma::Allocator* allocator = 0);

        /// Destroy this object.
        ~NegotiationUserData() BSLS_KEYWORD_OVERRIDE;

        // ACCESSORS

        /// Return the name of the cluster to negotiate the connection with.
        const bsl::string& clusterName() const;
    };

    typedef bmqp::RequestManager RequestManagerType;

    typedef mqbnet::MultiRequestManager<bsl::shared_ptr<mqbnet::Session> >
        StopRequestManagerType;

  private:
    // PRIVATE TYPES

    /// Struct containing meta information associated to a created cluster.
    struct ClusterInfo {
        // PUBLIC DATA

        /// Cluster created, owned by this struct.
        bsl::shared_ptr<mqbi::Cluster> d_cluster_sp;

        /// Event processor associated to this cluster.
        mqbnet::SessionEventProcessor* d_eventProcessor_p;
    };

    /// Map of `Cluster name` to `ClusterInfo` object.
    typedef bsl::unordered_map<bsl::string, ClusterInfo> ClustersMap;
    typedef ClustersMap::iterator                        ClustersMapIter;
    typedef ClustersMap::const_iterator                  ClustersMapConstIter;

    typedef bsl::vector<mqbcfg::ClusterDefinition>::const_iterator
        ClusterDefinitionConstIter;

    typedef bsl::vector<mqbcfg::ClusterProxyDefinition>::const_iterator
        ClusterProxyDefinitionConstIter;

    /// Map of `Cluster name` to `self nodeId`, of virtual clusters only.
    typedef bsl::unordered_map<bsl::string, int> VirtualClustersMap;

    /// Map of stat context names to StatContext pointers
    typedef bsl::unordered_map<bsl::string, bmqst::StatContext*>
        StatContextsMap;

  private:
    // DATA

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    /// Allocator store to spawn new allocators for sub-components.
    bmqma::CountingAllocatorStore d_allocators;

    /// True if this component is started.
    bool d_isStarted;

    /// Dispatcher to use.
    mqbi::Dispatcher* d_dispatcher_p;

    /// TransportManager for creating @bbref{mqbnet::Cluster}.
    mqbnet::TransportManager* d_transportManager_p;

    /// The domain factory to use, held not owned.
    mqbi::DomainFactory* d_domainFactory_p;

    /// Mutex for synchronizing usage of this component.
    mutable bslmt::Mutex d_mutex;

    mqbcfg::ClustersDefinition d_clustersDefinition;

    /// Cluster this machine belongs to (or empty if not part of any).
    bsl::unordered_set<bsl::string> d_myClusters;

    /// This map contains the list of all virtual clusters this machine belongs
    /// to (if any) as well as its nodeId in that virtual cluster.  While a
    /// virtual cluster is only meaningful at the downstream client, as a means
    /// to establish a multi-hop chained connection path to the upstream, the
    /// "virtual cluster member" needs to be aware of the nodeId the downstream
    /// clients refer it to, so that upon reception of the negotiation response
    /// they can map it back to their internal `ClusterNode` object.
    VirtualClustersMap d_myVirtualClusters;

    /// Container for the @bbref{mqbi::Cluster}s that have been created.
    ClustersMap d_clusters;

    /// Map of stat contexts.
    StatContextsMap d_statContexts;

    const mqbi::ClusterResources d_resources;

    /// Callback function to enqueue admin commands.
    mqbnet::Session::AdminCommandEnqueueCb d_adminCb;

    /// Request manager to use.
    RequestManagerType d_requestManager;

    /// Request manager to send stop requests to connected brokers.
    ///
    /// @note Should be part of `ClusterResources`.
    StopRequestManagerType d_stopRequestsManager;

  private:
    // NOT IMPLEMENTED
    ClusterCatalog(const ClusterCatalog&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    ClusterCatalog& operator=(const ClusterCatalog&) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Create a new net cluster object for the cluster with the specified
    /// `name` and `nodes`, and store the result in the specified `out`.
    /// Return 0 on success or a non-zero value and populate the specified
    /// `errorDescription` with a description of the error otherwise.  Note
    /// that the `d_mutex` must be locked prior to calling this method.
    int createNetCluster(bsl::ostream&                       errorDescription,
                         bslma::ManagedPtr<mqbnet::Cluster>* out,
                         const bsl::string&                  name,
                         const bsl::vector<mqbcfg::ClusterNode>& nodes);

    /// Create a new cluster object for the cluster with the specified
    /// `name` and store the result in the specified `out`.  Return 0 on
    /// success or a non-zero value and populate the specified
    /// `errorDescription` with a description of the error otherwise.  Note
    /// that the `d_mutex` must be locked prior to calling this method.
    /// Note also that if the cluster was already created, this method will
    /// return an error.
    int createCluster(bsl::ostream&                   errorDescription,
                      bsl::shared_ptr<mqbi::Cluster>* out,
                      const bsl::string&              name);

    /// Start the specified `cluster` and return 0 on success or a non-zero
    /// value otherwise, populating the specified `errorDescription` with a
    /// description of the error.  This method must *NOT* be called from
    /// within the object's mutex locked (see implementation for details).
    int startCluster(bsl::ostream& errorDescription, mqbi::Cluster* cluster);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterCatalog, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object using the specified `dispatcher`,
    /// `transportManager`, `statContexts`, `resources`, and the specified
    /// `allocator`.
    ClusterCatalog(mqbi::Dispatcher*             dispatcher,
                   mqbnet::TransportManager*     transportManager,
                   const StatContextsMap&        statContexts,
                   const mqbi::ClusterResources& resources,
                   bslma::Allocator*             allocator);

    /// Destructor.
    ~ClusterCatalog();

    // MANIPULATORS

    /// Retrieve the cluster config of this broker.  Return 0 on success or a
    /// non-zero value and populate the specified `errorDescription` with a
    /// description of the error otherwise.
    int loadBrokerClusterConfig(bsl::ostream& errorDescription);

    /// Start this component, which implies executing the script to retrieve
    /// the clusters' information as well as creating any cluster this
    /// broker is member of.  Return 0 on success or a non-zero value and
    /// populate the specified `errorDescription` with a description of the
    /// error otherwise.
    int start(bsl::ostream& errorDescription);

    /// Stop this component.
    void stop();

    /// Set the specified `domainFactory` on this instance.  Behavior is
    /// undefined unless `domainFactory` is non-zero.
    void setDomainFactory(mqbi::DomainFactory* domainFactory);

    /// Get (or create) the cluster with the specified `name` and load it in
    /// the specified `out`.  Return the appropriate status.
    bmqp_ctrlmsg::Status getCluster(bsl::shared_ptr<mqbi::Cluster>* out,
                                    const bslstl::StringRef&        name);

    /// Find cluster with the specified `name` and load it in the specified
    /// `out`.  Return `true` on success, `false` otherwise.
    bool findCluster(bsl::shared_ptr<mqbi::Cluster>* out,
                     const bslstl::StringRef&        name);

    /// Return number of clusters in the catalog.
    int count();

    /// Method invoked by the session negotiator when a new session,
    /// corresponding to the specified `nodeId` in the specified
    /// `clusterName` is being negotiated.  The specified `context`
    /// correspond to the negotiator context associated to the session, and
    /// some of its fields may be populated by this method (such as the
    /// `eventProcessor`, or the `resultState`).  Return a pointer to the
    /// corresponding `mqbnet::ClusterNode` on success, meaning this cluster
    /// session is legit, or 0 and populate the specified `errorDescription`
    /// in case this session was not expected and should be failed to
    /// negotiate.
    mqbnet::ClusterNode*
    onNegotiationForClusterSession(bsl::ostream& errorDescription,
                                   mqbnet::InitialConnectionContext* context,
                                   const bslstl::StringRef& clusterName,
                                   int                      nodeId);

    /// Process the specified `command`, and load the result in the
    /// specified `result`.  Return zero on success or a nonzero value
    /// otherwise.
    int processCommand(mqbcmd::ClustersResult*        result,
                       const mqbcmd::ClustersCommand& command);

    StopRequestManagerType& stopRequestManger();
    void processStopResponse(const bmqp_ctrlmsg::ControlMessage& message);

    /// Sets the callback, `value`, to pass to created clusters in this catalog
    /// that runs when an admin command is received by the cluster.
    void setAdminCommandEnqueueCallback(
        const mqbnet::Session::AdminCommandEnqueueCb& value);

    // ACCESSORS

    /// Return the node Id of this host in the cluster identified by the
    /// specified `name` if that cluster exist, has been created and this
    /// host is a member of it.  Return `mqbnet::Cluster::k_INVALID_NODE_ID`
    /// otherwise.
    int selfNodeIdInCluster(const bslstl::StringRef& name) const;

    /// Return `true` if the cluster identified by the specified `name` is
    /// configured as virtual.
    bool isClusterVirtual(const bslstl::StringRef& name) const;

    /// Returns `true` if this node is member of the specified `clusterName`
    /// and `false` otherwise.  If `clusterName` is empty, return true if
    /// this node is member of *any* cluster.
    bool isMemberOf(const bsl::string& clusterName) const;

    /// Load into the specified `out` a snapshot of all clusters currently in
    /// the catalog.
    void getClusters(bsl::vector<bsl::shared_ptr<mqbi::Cluster> >* out) const;
};

// ============================

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------------------------
// class ClusterCatalog::NegotiationUserData
// ----------------------------------------

// ACCESSORS
inline const bsl::string&
ClusterCatalog::NegotiationUserData::clusterName() const
{
    return d_clusterName;
}

// --------------------
// class ClusterCatalog
// --------------------

// ACCESSORS
inline bool ClusterCatalog::isMemberOf(const bsl::string& clusterName) const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED
    if (clusterName.empty()) {
        return !d_myClusters.empty();  // RETURN
    }

    return (d_myClusters.find(clusterName) != d_myClusters.end());
}

inline ClusterCatalog::StopRequestManagerType&
ClusterCatalog::stopRequestManger()
{
    return d_stopRequestsManager;
}

inline void ClusterCatalog::processStopResponse(
    const bmqp_ctrlmsg::ControlMessage& message)
{
    d_requestManager.processResponse(message);
}

inline void ClusterCatalog::setAdminCommandEnqueueCallback(
    const mqbnet::Session::AdminCommandEnqueueCb& value)
{
    d_adminCb = value;
}

}  // close package namespace
}  // close enterprise namespace

#endif
