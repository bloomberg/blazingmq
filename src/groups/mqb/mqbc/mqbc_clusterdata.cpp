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

// mqbc_clusterdata.cpp                                               -*-C++-*-
#include <mqbc_clusterdata.h>

// MQB
#include <mqbnet_cluster.h>
#include <mqbnet_elector.h>
#include <mqbscm_version.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqscm_version.h>

// MWC
#include <mwcsys_threadutil.h>

// BDE
#include <bdls_processutil.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbc {

namespace {

mqbc::ClusterDataIdentity clusterIdentity(const bslstl::StringRef& name,
                                          const mqbnet::Cluster*   netCluster)
{
    // Create client identity
    bmqp_ctrlmsg::ClientIdentity identity;
    identity.protocolVersion() = bmqp::Protocol::k_VERSION;
    identity.sdkVersion()      = bmqscm::Version::versionAsInt();
    identity.clientType()      = bmqp_ctrlmsg::ClientType::E_TCPBROKER;
    identity.pid()             = bdls::ProcessUtil::getProcessId();
    identity.sessionId()       = 1;
    identity.clusterName()     = name;
    identity.clusterNodeId()   = netCluster->selfNodeId();
    identity.hostName()        = netCluster->selfNode()->nodeDescription();
    if (bdls::ProcessUtil::getProcessName(&(identity.processName())) != 0) {
        identity.processName() = "** UNKNOWN **";
    }
    identity.sdkLanguage() = bmqp_ctrlmsg::ClientLanguage::E_CPP;

    // Create cluster identity
    mqbc::ClusterDataIdentity clusterIdentity(name, identity);

    // Create and set description
    bdlma::LocalSequentialAllocator<256> localAllocator;
    mwcu::MemOutStream                   os(&localAllocator);
    os << "Cluster (" << name << ")";

    bsl::string description;
    description.assign(os.str().data(), os.str().length());

    clusterIdentity.setDescription(description);

    return clusterIdentity;
}

}  // close unnamed namespace

// -----------------
// class ClusterData
// -----------------

// CREATORS
ClusterData::ClusterData(const bslstl::StringRef&           name,
                         bdlmt::EventScheduler*             scheduler,
                         bdlbb::BlobBufferFactory*          bufferFactory,
                         BlobSpPool*                        blobSpPool,
                         const mqbcfg::ClusterDefinition&   clusterConfig,
                         bslma::ManagedPtr<mqbnet::Cluster> netCluster,
                         mqbi::Cluster*                     cluster,
                         mqbi::DomainFactory*               domainFactory,
                         mqbnet::TransportManager*          transportManager,
                         mwcst::StatContext*    clustersStatContext,
                         const StatContextsMap& statContexts,
                         bslma::Allocator*      allocator)
: d_allocator_p(allocator)
, d_scheduler_p(scheduler)
, d_bufferFactory_p(bufferFactory)
, d_blobSpPool_p(blobSpPool)
, d_dispatcherClientData()
, d_clusterConfig(clusterConfig)
, d_electorInfo(cluster)
, d_membership(netCluster, allocator)
, d_identity(cluster->isRemote()
                 ? ClusterDataIdentity(name,
                                       bmqp_ctrlmsg::ClientIdentity(allocator),
                                       allocator)
                 : clusterIdentity(name, d_membership.netCluster()))
, d_cluster_p(cluster)
, d_messageTransmitter(bufferFactory, cluster, transportManager, allocator)
, d_requestManager(bmqp::EventType::e_CONTROL,
                   bufferFactory,
                   scheduler,
                   false,  // lateResponseMode
                   allocator)
, d_multiRequestManager(&d_requestManager, allocator)
, d_domainFactory_p(domainFactory)
, d_transportManager_p(transportManager)
, d_stats(allocator)
, d_clusterNodesStatContext_mp(
      statContexts.find("clusterNodes")
          ->second->addSubcontext(
              mwcst::StatContextConfiguration(d_identity.name(),
                                              d_allocator_p)))
, d_stateSpPool(8192, allocator)
, d_miscWorkThreadPool(
      mwcsys::ThreadUtil::defaultAttributes().setThreadName("bmqMiscWorkTP"),
      3,      // numThreads
      10000,  // maxNumPendingJobs
      allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_allocator_p);
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT(scheduler->clockType() == bsls::SystemClockType::e_MONOTONIC);

    // Initialize the clusterStats object - under the hood this creates a new
    // subcontext to be held by this object to be used by all lower level
    // components created here.
    d_stats.initialize(cluster->name(),
                       clusterConfig.partitionConfig().numPartitions(),
                       clustersStatContext,
                       d_allocator_p);
}

}  // close package namespace
}  // close enterprise namespace
