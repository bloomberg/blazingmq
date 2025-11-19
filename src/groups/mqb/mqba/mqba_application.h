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

/// @file mqba_application.h
///
/// @brief Provide an `Application` class to control object lifetime/creation.
///
/// This component defines a mechanism, @bbref{mqba::Application}, responsible
/// for instantiating and destroying the top-level BlazingMQ objects used by
/// the BlazingMQ broker.

// MQB
#include <mqba_commandrouter.h>
#include <mqbauthn_authenticationcontroller.h>
#include <mqbcmd_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>

// BMQ
#include <bmqma_countingallocatorstore.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_threadpool.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
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
class Session;
}
namespace mqbplug {
class PluginManager;
}
namespace mqbstat {
class StatController;
}
namespace bmqst {
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
    typedef bslma::ManagedPtr<mqbplug::PluginManager>  PluginManagerMp;
    typedef bslma::ManagedPtr<mqbblp::ClusterCatalog>  ClusterCatalogMp;
    typedef bslma::ManagedPtr<ConfigProvider>          ConfigProviderMp;
    typedef bslma::ManagedPtr<Dispatcher>              DispatcherMp;
    typedef bslma::ManagedPtr<DomainManager>           DomainManagerMp;
    typedef bslma::ManagedPtr<mqbstat::StatController> StatControllerMp;
    typedef bslma::ManagedPtr<mqbauthn::AuthenticationController>
        AuthenticationControllerMp;
    typedef bslma::ManagedPtr<mqbnet::TransportManager> TransportManagerMp;
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
                                                           BlobSpPool;
    typedef bsl::vector<bsl::shared_ptr<mqbnet::Session> > Sessions;

    // Data members

    /// Allocator store to spawn new allocators for sub components.
    bmqma::CountingAllocatorStore d_allocators;

    /// Event scheduler (held not owned to use, sharde with all interested
    /// components.
    bdlmt::EventScheduler* d_scheduler_p;

    /// Thread pool for admin commands execution.
    bdlmt::ThreadPool d_adminExecutionPool;

    /// Thread pool for routed admin commands execution.  Ensuring rerouted
    /// commands always execute on their own dedicated thread prevents a case
    /// where two nodes are simultaneously waiting for each other to process a
    /// routed command, but cannot make progress because the calling thread is
    /// blocked ("deadlock").  Note that rerouted commands never route again.
    bdlmt::ThreadPool d_adminRerouteExecutionPool;

    bdlbb::PooledBlobBufferFactory d_bufferFactory;

    BlobSpPool d_blobSpPool;

    bsl::shared_ptr<bdlma::ConcurrentPool> d_pushElementsPool_sp;

    /// Stat context of the counting allocators, if used.
    bmqst::StatContext* d_allocatorsStatContext_p;

    PluginManagerMp d_pluginManager_mp;

    /// Statistics controller component.
    StatControllerMp d_statController_mp;

    /// Authentication controller component.
    AuthenticationControllerMp d_authenticationController_mp;

    ConfigProviderMp d_configProvider_mp;

    DispatcherMp d_dispatcher_mp;

    TransportManagerMp d_transportManager_mp;

    ClusterCatalogMp d_clusterCatalog_mp;

    DomainManagerMp d_domainManager_mp;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE MANIPULATORS

    /// Perform any initialization that needs to be done one time only at
    /// task startup (to enable thread-safety of some component, or
    /// initialize some statics, ...).
    void oneTimeInit();

    /// Pendant operation of the `oneTimeInit` one.
    void oneTimeShutdown();

    /// Attempt to execute graceful shutdown logic v2.
    ///
    /// If any node or proxy does not support the v2 graceful shutdown logic,
    /// do not perform any shutdown actions and return `false`.  Otherwise,
    /// send v2 shutdown requests to all nodes, shutdown clients and proxies,
    /// and return `true`.
    bool initiateShutdown();

    void sendStopRequests(bslmt::Latch*   latch,
                          const Sessions& brokers,
                          int             version);

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
                bmqst::StatContext*    allocatorsStatContext,
                bslma::Allocator*      allocator);

    /// Destructor.
    ~Application();

    // ACCESSORS

    /// Load into the specified `contexts` all root top level stat contexts
    /// (allocators, systems, domains, clients, ...).
    void loadStatContexts(
        bsl::unordered_map<bsl::string, bmqst::StatContext*>* contexts) const;

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
