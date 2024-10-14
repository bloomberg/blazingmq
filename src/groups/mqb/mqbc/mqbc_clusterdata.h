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

// mqbc_clusterdata.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERDATA
#define INCLUDED_MQBC_CLUSTERDATA

//@PURPOSE: Provide a VST representing the non-persistent state of a cluster.
//
//@CLASSES:
//  mqbc::ClusterDataIdentity: VST for the identity of a cluster
//  mqbc::ClusterData:         VST for non-persistent state of a cluster
//
//@DESCRIPTION: 'mqbc::ClusterData' is a value-semantic type representing the
// non-persistent state of a cluster.

// MQB

#include <mqbc_clustermembership.h>
#include <mqbc_controlmessagetransmitter.h>
#include <mqbc_electorinfo.h>
#include <mqbcfg_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbnet_cluster.h>
#include <mqbnet_elector.h>
#include <mqbnet_multirequestmanager.h>
#include <mqbnet_transportmanager.h>
#include <mqbstat_clusterstats.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_requestmanager.h>

// MWC
#include <mwcst_statcontext.h>
#include <mwcu_atomicstate.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_fixedthreadpool.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbc {

// =========================
// class ClusterDataIdentity
// =========================

/// This class provides a VST representing the identity of a cluster.
class ClusterDataIdentity {
  private:
    // DATA

    bsl::string d_name;
    // Name of the cluster

    bsl::string d_description;
    // Description of the cluster

    bmqp_ctrlmsg::ClientIdentity d_identity;
    // Information sent to the primary node of
    // a queue while sending a clusterOpenQueue
    // request to that node

  public:
    // CREATORS

    /// Create a `mqbc::ClusterDataIdentity` with the specified `name`,
    /// `description` and `identity`.
    ClusterDataIdentity(const bsl::string&                  name,
                        const bsl::string&                  description,
                        const bmqp_ctrlmsg::ClientIdentity& identity);

    // ACCESSORS

    /// Return the value of the corresponding member of this object.
    const bsl::string&                  name() const;
    const bsl::string&                  description() const;
    const bmqp_ctrlmsg::ClientIdentity& identity() const;
};

// =================
// class ClusterData
// =================

/// This class provides a VST representing the non-persistent state of a
/// cluster.
class ClusterData {
  public:
    // TYPES

    /// Pool of shared pointers to Blobs
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

    typedef bdlcc::SharedObjectPool<
        mwcu::AtomicState,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::Reset<mwcu::AtomicState> >
        StateSpPool;

    /// Type of the RequestManager used by the cluster.
    typedef bmqp::RequestManager<bmqp_ctrlmsg::ControlMessage,
                                 bmqp_ctrlmsg::ControlMessage>
        RequestManagerType;

    /// Type of the MultiRequestManager used by the cluster.
    typedef mqbnet::MultiRequestManager<bmqp_ctrlmsg::ControlMessage,
                                        bmqp_ctrlmsg::ControlMessage>
        MultiRequestManagerType;

    typedef bslma::ManagedPtr<mwcst::StatContext> StatContextMp;

    /// Map of stat context names to StatContext pointers
    typedef bsl::unordered_map<bsl::string, mwcst::StatContext*>
        StatContextsMap;

  private:
    // DATA

    bslma::Allocator* d_allocator_p;
    // Allocator to use

    const mqbi::ClusterResources d_resources;

    mqbi::DispatcherClientData d_dispatcherClientData;
    // Dispatcher client data associated to this
    // session

    mqbcfg::ClusterDefinition d_clusterConfig;
    // Cluster configuration to use

    mqbcfg::ClusterProxyDefinition d_clusterProxyConfig;
    // Cluster proxy configuration to use

    ElectorInfo d_electorInfo;
    // Elector information

    ClusterMembership d_membership;
    // The membership information of the cluster

    const ClusterDataIdentity d_identity;
    // The identity of the cluster

    mqbi::Cluster* d_cluster_p;
    // Associated cluster

    ControlMessageTransmitter d_messageTransmitter;
    // Control message transmitter to use

    RequestManagerType d_requestManager;
    // Request manager to use

    MultiRequestManagerType d_multiRequestManager;
    // MultiRequest manager to use

    mqbi::DomainFactory* d_domainFactory_p;  // from mqbblp::Cluster
                                             // Domain factory to use

    mqbnet::TransportManager* d_transportManager_p;

    mqbstat::ClusterStats d_stats;
    // Object encapsulating the statistics
    // recorded for this cluster

    StatContextMp d_clusterNodesStatContext_mp;
    // Top level StatContext pointer for all
    // nodes of this cluster

    StateSpPool d_stateSpPool;

    bdlmt::FixedThreadPool d_miscWorkThreadPool;
    // Thread pool used for any standalone
    // work that can be offloaded to any
    // non-dispatcher threads.

  private:
    // NOT IMPLEMENTED
    ClusterData(const ClusterData&) BSLS_KEYWORD_DELETED;
    ClusterData& operator=(const ClusterData&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterData, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbc::ClusterData` with the specified `name`, `resources`,
    /// `clusterConfig`, `clusterProxyConfig`, `netCluster`, `cluster`,
    /// `clustersStatContext` and `statContexts` values.  Use the specified
    /// `allocator` for any memory allocation.
    ClusterData(const bslstl::StringRef&              name,
                const mqbi::ClusterResources&         resources,
                const mqbcfg::ClusterDefinition&      clusterConfig,
                const mqbcfg::ClusterProxyDefinition& clusterProxyConfig,
                bslma::ManagedPtr<mqbnet::Cluster>    netCluster,
                mqbi::Cluster*                        cluster,
                mqbi::DomainFactory*                  domainFactory,
                mqbnet::TransportManager*             transportManager,
                mwcst::StatContext*                   clustersStatContext,
                const StatContextsMap&                statContexts,
                bslma::Allocator*                     allocator);

    // MANIPULATORS

    /// Get a modifiable reference to this object's event scheduler.
    bdlmt::EventScheduler& scheduler();

    /// Get a modifiable reference to this object's buffer factory.
    bdlbb::BlobBufferFactory& bufferFactory();

    /// Get a modifiable reference to this object's blobSpPool.
    BlobSpPool& blobSpPool();

    /// Get a modifiable reference to this object's dispatcherClientData.
    mqbi::DispatcherClientData& dispatcherClientData();

    /// Get a modifiable reference to this object's cluster config.
    mqbcfg::ClusterDefinition& clusterConfig();

    /// Get a modifiable reference to this object's cluster proxy config.
    mqbcfg::ClusterProxyDefinition& clusterProxyConfig();

    /// Get a modifiable reference to this object's elector information.
    ElectorInfo& electorInfo();

    /// Get a modifiable reference to this object's cluster membership.
    ClusterMembership& membership();

    /// Get a modifiable reference to this object's cluster.
    mqbi::Cluster& cluster();

    /// Get a modifiable reference to this object's messageTransmitter.
    ControlMessageTransmitter& messageTransmitter();

    /// Get a modifiable reference to this object's requestManager.
    RequestManagerType& requestManager();

    /// Get a modifiable reference to this object's multiRequestManager.
    MultiRequestManagerType& multiRequestManager();

    /// Get a modifiable reference to this object's domainFactory.
    mqbi::DomainFactory* domainFactory();

    /// Get a modifiable reference to this object's transportManager.
    mqbnet::TransportManager& transportManager();

    /// Get a modifiable reference to this object's cluster stats.
    mqbstat::ClusterStats& stats();

    /// Get a modifiable reference to this object's clusterNodesStatContext.
    StatContextMp& clusterNodesStatContext();

    /// Get a modifiable reference to this object's stateSpPool.
    StateSpPool& stateSpPool();

    /// Get a modifiable reference to this object's miscWorkThreadPool.
    bdlmt::FixedThreadPool& miscWorkThreadPool();

    // ACCESSORS

    /// Return the value of the corresponding member of this object.
    const mqbi::ClusterResources&         resources() const;
    const mqbi::DispatcherClientData&     dispatcherClientData() const;
    const mqbcfg::ClusterDefinition&      clusterConfig() const;
    const mqbcfg::ClusterProxyDefinition& clusterProxyConfig() const;
    const ElectorInfo&                    electorInfo() const;
    const ClusterMembership&              membership() const;
    const ClusterDataIdentity&            identity() const;
    const mqbi::Cluster&                  cluster() const;
    const StatContextMp&                  clusterNodesStatContext() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class ClusterDataIdentity
// -------------------------

// CREATORS
inline ClusterDataIdentity::ClusterDataIdentity(
    const bsl::string&                  name,
    const bsl::string&                  description,
    const bmqp_ctrlmsg::ClientIdentity& identity)
: d_name(name)
, d_description(description)
, d_identity(identity)
{
    // NOTHING
}

// ACCESSORS
inline const bsl::string& ClusterDataIdentity::name() const
{
    return d_name;
}

inline const bsl::string& ClusterDataIdentity::description() const
{
    return d_description;
}

inline const bmqp_ctrlmsg::ClientIdentity&
ClusterDataIdentity::identity() const
{
    return d_identity;
}

// -----------------
// class ClusterData
// -----------------

// MANIPULATORS
inline bdlmt::EventScheduler& ClusterData::scheduler()
{
    return *d_resources.scheduler();
}

inline bdlbb::BlobBufferFactory& ClusterData::bufferFactory()
{
    return *d_resources.bufferFactory();
}

inline ClusterData::BlobSpPool& ClusterData::blobSpPool()
{
    return *d_resources.blobSpPool();
}

inline mqbi::DispatcherClientData& ClusterData::dispatcherClientData()
{
    return d_dispatcherClientData;
}

inline mqbcfg::ClusterDefinition& ClusterData::clusterConfig()
{
    return d_clusterConfig;
}

inline mqbcfg::ClusterProxyDefinition& ClusterData::clusterProxyConfig()
{
    return d_clusterProxyConfig;
}

inline ElectorInfo& ClusterData::electorInfo()
{
    return d_electorInfo;
}

inline ClusterMembership& ClusterData::membership()
{
    return d_membership;
}

inline mqbi::Cluster& ClusterData::cluster()
{
    return *d_cluster_p;
}

inline ControlMessageTransmitter& ClusterData::messageTransmitter()
{
    return d_messageTransmitter;
}

inline ClusterData::RequestManagerType& ClusterData::requestManager()
{
    return d_requestManager;
}

inline ClusterData::MultiRequestManagerType& ClusterData::multiRequestManager()
{
    return d_multiRequestManager;
}

inline mqbi::DomainFactory* ClusterData::domainFactory()
{
    return d_domainFactory_p;
}

inline mqbnet::TransportManager& ClusterData::transportManager()
{
    return *d_transportManager_p;
}

inline mqbstat::ClusterStats& ClusterData::stats()
{
    return d_stats;
}

inline ClusterData::StatContextMp& ClusterData::clusterNodesStatContext()
{
    return d_clusterNodesStatContext_mp;
}

inline ClusterData::StateSpPool& ClusterData::stateSpPool()
{
    return d_stateSpPool;
}

inline bdlmt::FixedThreadPool& ClusterData::miscWorkThreadPool()
{
    return d_miscWorkThreadPool;
}

// ACCESSORS
inline const mqbi::ClusterResources& ClusterData::resources() const
{
    return d_resources;
}

// ACCESSORS
inline const mqbi::DispatcherClientData&
ClusterData::dispatcherClientData() const
{
    return d_dispatcherClientData;
}

inline const mqbcfg::ClusterDefinition& ClusterData::clusterConfig() const
{
    return d_clusterConfig;
}

inline const mqbcfg::ClusterProxyDefinition&
ClusterData::clusterProxyConfig() const
{
    return d_clusterProxyConfig;
}

inline const ElectorInfo& ClusterData::electorInfo() const
{
    return d_electorInfo;
}

inline const ClusterMembership& ClusterData::membership() const
{
    return d_membership;
}

inline const ClusterDataIdentity& ClusterData::identity() const
{
    return d_identity;
}

inline const mqbi::Cluster& ClusterData::cluster() const
{
    return *d_cluster_p;
}

inline const ClusterData::StatContextMp&
ClusterData::clusterNodesStatContext() const
{
    return d_clusterNodesStatContext_mp;
}

}  // close package namespace
}  // close enterprise namespace

#endif
