// Copyright 2014-2024 Bloomberg Finance L.P.
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

// mqba_application.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBA_APPLICATION
#define INCLUDED_MQBA_APPLICATION

//@PURPOSE: Provide an Application class to control object lifetime/creation.
//
//@CLASSES:
//  mqba::Application: BlazingMQ broker application top level component
//
//@DESCRIPTION: This component defines a mechanism, 'mqba::Application',
// responsible for instantiating and destroying the top-level BlazingMQ objects
// used by the BlazingMQ broker.

// MQB
#include <mqba_commandrouter.h>
#include <mqbcmd_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>

// MWC
#include <mwcma_countingallocatorstore.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_threadpool.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace mqbblp {
class ClusterCatalog;
}
namespace mqbnet {
class TransportManager;
}
namespace mqbplug {
class PluginManager;
}
namespace mqbstat {
class StatController;
}
namespace mwcst {
class StatContext;
}

namespace mqba {

// FORWARD DECLARATION
class ConfigProvider;
class Dispatcher;
class DomainManager;

// =================
// class Application
// =================

/// BMQBRKR application top level component
class Application {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBA.APPLICATION");

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Application, bslma::UsesBslmaAllocator)

  private:
    // PRIVATE TYPES
    typedef bslma::ManagedPtr<mqbplug::PluginManager>   PluginManagerMp;
    typedef bslma::ManagedPtr<mqbblp::ClusterCatalog>   ClusterCatalogMp;
    typedef bslma::ManagedPtr<ConfigProvider>           ConfigProviderMp;
    typedef bslma::ManagedPtr<Dispatcher>               DispatcherMp;
    typedef bslma::ManagedPtr<DomainManager>            DomainManagerMp;
    typedef bslma::ManagedPtr<mqbstat::StatController>  StatControllerMp;
    typedef bslma::ManagedPtr<mqbnet::TransportManager> TransportManagerMp;
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

    // Data members
    mwcma::CountingAllocatorStore d_allocators;
    // Allocator store to spawn new allocators
    // for sub-components

    bdlmt::EventScheduler* d_scheduler_p;
    // Event scheduler (held not owned) to use,
    // shared with all interested components.

    bdlmt::ThreadPool d_adminExecutionPool;
    // Thread pool for admin commands
    // execution.

    bdlmt::ThreadPool d_adminRerouteExecutionPool;
    // Thread pool for routed admin commands execution.
    // Ensuring rerouted commands always execute on their
    // own dedicated thread prevents a case where two nodes
    // are simultaneously waiting for each other to process
    // a routed command, but cannot make process because
    // the calling thread is blocked ("deadlock").
    // Note that rerouted commands never route again.

    bdlbb::PooledBlobBufferFactory d_bufferFactory;

    BlobSpPool d_blobSpPool;

    mwcst::StatContext* d_allocatorsStatContext_p;
    // Stat context of the counting allocators,
    // if used

    PluginManagerMp d_pluginManager_mp;

    StatControllerMp d_statController_mp;
    // Statistics controller component

    ConfigProviderMp d_configProvider_mp;

    DispatcherMp d_dispatcher_mp;

    TransportManagerMp d_transportManager_mp;

    ClusterCatalogMp d_clusterCatalog_mp;

    DomainManagerMp d_domainManager_mp;

    bslma::Allocator* d_allocator_p;
    // Allocator to use

  private:
    // PRIVATE MANIPULATORS

    /// Perform any initialization that needs to be done one time only at
    /// task startup (to enable thread-safety of some component, or
    /// initialize some statics, ...).
    void oneTimeInit();

    /// Pendant operation of the `oneTimeInit` one.
    void oneTimeShutdown();

  private:
    // NOT IMPLEMENTED
    Application(const Application& other) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented
    Application& operator=(const Application& other) BSLS_CPP11_DELETED;

  public:
    // CREATORS

    /// Create a new `Application` object, using the specified `scheduler`
    /// and the specified `allocatorsStatContext` and `allocator`.
    Application(bdlmt::EventScheduler* scheduler,
                mwcst::StatContext*    allocatorsStatContext,
                bslma::Allocator*      allocator);

    /// Destructor.
    ~Application();

    // ACCESSORS

    /// Load into the specified `contexts` all root top level stat contexts
    /// (allocators, systems, domains, clients, ...).
    void loadStatContexts(
        bsl::unordered_map<bsl::string, mwcst::StatContext*>* contexts) const;

    // MANIPULATORS

    /// Start the application. Return 0 on success, or a non-zero return
    /// code on error and fill in the specified `errorDescription` stream
    /// with the description of the error.
    int start(bsl::ostream& errorDescription);

    /// Stop the application.
    void stop();

    /// Process the command `cmd` coming from the specified `source`, and write
    /// the result of the command in the given output stream, `os`.
    /// Mark `fromReroute` as true if executing the command from a reroute to
    /// ensure proper routing logic. Returns 0 on success, -1 on early exit,
    /// -2 on error, and some non-zero error code on parse failure.
    int processCommand(const bslstl::StringRef& source,
                       const bsl::string&       cmd,
                       bsl::ostream&            os,
                       bool                     fromReroute = false);

    /// Process the command `cmd` coming from the specified `source` node, and
    /// send the result of the command in the given `onProcessedCb`. Mark
    /// `fromReroute` as true if executing command from a reroute to ensure
    /// proper routing logic. Returns the error code of calling
    /// `processCommand` with the given `cmd`, `source`, and `fromReroute`.
    int processCommandCb(
        const bslstl::StringRef&                            source,
        const bsl::string&                                  cmd,
        const bsl::function<void(int, const bsl::string&)>& onProcessedCb,
        bool fromReroute = false);

    /// Enqueue for execution the command in the specified `cmd` coming from
    /// the specified `source`.  The specified `onProcessedCb` callback is
    /// used to send result of the command after execution. Mark `fromReroute`
    /// as true if executing command from a reroute to ensure proper routing
    /// logic.
    int enqueueCommand(
        const bslstl::StringRef&                            source,
        const bsl::string&                                  cmd,
        const bsl::function<void(int, const bsl::string&)>& onProcessedCb,
        bool fromReroute = false);

  private:
    /// Returns a pointer to the cluster instance that the given `command`
    /// needs to execute for. Fails when the given command does not have a
    /// cluster associated with it or the cluster cannot be found. On failure,
    /// this function returns a nullptr and populates `errorDescription` with
    /// a reason.
    mqbi::Cluster* getRelevantCluster(bsl::ostream&          errorDescription,
                                      const mqbcmd::Command& command) const;

    /// Executes the logic of the given `command` and outputs the result in
    /// `cmdResult`. Returns 0 on success and -1 on early exit
    int executeCommand(const mqbcmd::Command&  command,
                       mqbcmd::InternalResult* cmdResult);
};

}  // close package namespace
}  // close enterprise namespace

#endif
