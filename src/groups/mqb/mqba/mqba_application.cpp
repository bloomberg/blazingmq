// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqba_application.cpp                                               -*-C++-*-
#include <mqba_application.h>

#include <mqbscm_version.h>
// MQB
#include <mqba_configprovider.h>
#include <mqba_dispatcher.h>
#include <mqba_domainmanager.h>
#include <mqba_sessionnegotiator.h>
#include <mqbblp_cluster.h>
#include <mqbblp_clustercatalog.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbcmd_commandlist.h>
#include <mqbcmd_humanprinter.h>
#include <mqbcmd_jsonprinter.h>
#include <mqbcmd_messages.h>
#include <mqbcmd_parseutil.h>
#include <mqbcmd_util.h>
#include <mqbnet_cluster.h>
#include <mqbnet_transportmanager.h>
#include <mqbplug_pluginmanager.h>
#include <mqbstat_statcontroller.h>
#include <mqbu_exit.h>
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqt_uri.h>

// MWC
#include <mwcscm_version.h>
#include <mwcst_statcontext.h>
#include <mwcsys_time.h>
#include <mwcu_memoutstream.h>

// BDE
#include <baljsn_encoder.h>
#include <baljsn_encoderoptions.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bdls_memoryutil.h>
#include <bdls_osutil.h>
#include <bdls_processutil.h>
#include <bsl_cstddef.h>
#include <bsl_cstdlib.h>
#include <bsl_ctime.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bslmt_latch.h>
#include <bslmt_lockguard.h>
#include <bslmt_once.h>
#include <bsls_assert.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {
const int k_BLOBBUFFER_SIZE           = 4 * 1024;
const int k_BLOB_POOL_GROWTH_STRATEGY = 1024;

/// Create a new blob at the specified `arena` address, using the specified
/// `bufferFactory` and `allocator`.
void createBlob(bdlbb::BlobBufferFactory* bufferFactory,
                void*                     arena,
                bslma::Allocator*         allocator)
{
    new (arena) bdlbb::Blob(bufferFactory, allocator);
}

}  // close unnamed namespace

// -----------
// Application
// -----------

// PRIVATE MANIPULATORS
void Application::oneTimeInit()
{
    BSLMT_ONCE_DO
    {
        // Initialize Time with platform-specific clocks/timers
        mwcsys::Time::initialize();

        // Initialize pseudo-random number generator.  We add some
        // machine-specific (high resolution timer) and task-specific (pid)
        // information to take care of scenarios when nodes are started at the
        // same time or on the same physical host.
        unsigned int seed =
            bsl::time(NULL) +
            static_cast<unsigned int>(bdls::ProcessUtil::getProcessId()) +
            static_cast<unsigned int>(mwcsys::Time::highResolutionTimer() &
                                      0xFFFFFFFF);

        bsl::srand(seed);

        // Make MessageGUID generation thread-safe by calling initialize
        mqbu::MessageGUIDUtil::initialize();

        bmqp::Crc32c::initialize();
        bmqt::UriParser::initialize();
        bmqp::ProtocolUtil::initialize();
    }
}

void Application::oneTimeShutdown()
{
    BSLMT_ONCE_DO
    {
        bmqp::ProtocolUtil::shutdown();
        bmqt::UriParser::shutdown();
        mwcsys::Time::shutdown();
    }
}

// CREATORS
Application::Application(bdlmt::EventScheduler* scheduler,
                         mwcst::StatContext*    allocatorsStatContext,
                         bslma::Allocator*      allocator)
: d_allocators(allocator)
, d_scheduler_p(scheduler)
, d_adminExecutionPool(mwcsys::ThreadUtil::defaultAttributes(),
                       0,
                       1,
                       bsls::TimeInterval(120).totalMilliseconds(),
                       allocator)
, d_bufferFactory(k_BLOBBUFFER_SIZE, d_allocators.get("BufferFactory"))
, d_blobSpPool(bdlf::BindUtil::bind(&createBlob,
                                    &d_bufferFactory,
                                    bdlf::PlaceHolders::_1,   // arena
                                    bdlf::PlaceHolders::_2),  // allocator
               k_BLOB_POOL_GROWTH_STRATEGY,
               d_allocators.get("BlobSpPool"))
, d_allocatorsStatContext_p(allocatorsStatContext)
, d_pluginManager_mp()
, d_statController_mp()
, d_configProvider_mp()
, d_dispatcher_mp()
, d_transportManager_mp()
, d_clusterCatalog_mp()
, d_domainManager_mp()
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_scheduler_p->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);

    oneTimeInit();

    bsl::string osName("** NA**");
    bsl::string osVersion("** NA **");
    bsl::string osPatch("** NA **");
    bdls::OsUtil::getOsInfo(&osName, &osVersion, &osPatch);  // ignore rc

    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    const char assertLevel[] = "SAFE";
#elif defined(BSLS_ASSERT_IS_ACTIVE)
    const char assertLevel[] = "ASSERT";
#elif defined(BSLS_ASSERT_OPT_IS_ACTIVE)
    const char assertLevel[] = "OPT";
#else
    const char assertLevel[] = "NONE";
#endif

#define MQBA_STRINGIFY2(s) #s
#define MQBA_STRINGIFY(s) MQBA_STRINGIFY2(s)

    // Print banner
    BALL_LOG_INFO
        << "Starting"
        << "\n   ____  __  __  ___  _               _"
        << "\n  | __ )|  \\/  |/ _ \\| |__  _ __ ___ | | _____ _ __"
        << "\n  |  _ \\| |\\/| | | | | '_ \\| '__/ _ \\| |/ / _ \\ '__|"
        << "\n  | |_) | |  | | |_| | |_) | | | (_) |   <  __/ |"
        << "\n  |____/|_|  |_|\\__\\_\\_.__/|_|  \\___/|_|\\_\\___|_|"
        << "\n"
        << "\n    Instance..............: " << brkrCfg.brokerInstanceName()
        << "\n    Version...............: " << brkrCfg.brokerVersion()
        << "\n    Build Type............: " << MQBA_STRINGIFY(BMQ_BUILD_TYPE)
        << "\n    Assert Level..........: " << assertLevel
        << "\n    Config version........: " << brkrCfg.configVersion()
        << "\n    MWC version...........: " << mwcscm::Version::version()
        << "\n    OS name...............: " << osName
        << "\n    OS version............: " << osVersion
        << "\n    OS patch..............: " << osPatch
        << "\n    OS page size (bytes)..: " << bdls::MemoryUtil::pageSize()
        << "\n    BrokerId..............: "
        << mqbu::MessageGUIDUtil::brokerIdHex() << "\n";

#undef MQBA_STRINGIFY2
#undef MQBA_STRINGIFY
}

Application::~Application()
{
    oneTimeShutdown();
}

void Application::loadStatContexts(
    bsl::unordered_map<bsl::string, mwcst::StatContext*>* contexts) const
{
    if (!d_statController_mp) {
        return;  // RETURN
    }

    d_statController_mp->loadStatContexts(contexts);
}

int Application::start(bsl::ostream& errorDescription)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                            = 0,
        rc_STATCONTROLLER                     = -1,
        rc_CONFIGPROVIDER                     = -2,
        rc_DISPATCHER                         = -3,
        rc_TRANSPORTMANAGER                   = -4,
        rc_BROKER_CLUSTER_CONFIG_LOADFAILURE  = -5,
        rc_INVALID_CONFIGPROVIDER_VERSION     = -6,
        rc_CLUSTERCATALOG                     = -7,
        rc_DOMAINMANAGER                      = -8,
        rc_TRANSPORTMANAGER_LISTEN            = -9,
        rc_CLUSTER_REVERSECONNECTIONS_FAILURE = -10,
        rc_ADMIN_POOL_START_FAILURE           = -11,
        rc_PLUGINMANAGER                      = -12
    };

    int rc = rc_SUCCESS;

    // Start the PluginManager
    {
        d_pluginManager_mp.load(new (*d_allocator_p)
                                    mqbplug::PluginManager(d_allocator_p),
                                d_allocator_p);
        rc = d_pluginManager_mp->start(mqbcfg::BrokerConfig::get().plugins(),
                                       errorDescription);
        if (rc != 0) {
            return (rc * 100) + rc_PLUGINMANAGER;  // RETURN
        }
    }

    // Start the StatController
    d_statController_mp.load(
        new (*d_allocator_p) mqbstat::StatController(
            bdlf::BindUtil::bind(&Application::processCommand,
                                 this,
                                 bdlf::PlaceHolders::_1,   // source
                                 bdlf::PlaceHolders::_2,   // cmd
                                 bdlf::PlaceHolders::_3),  // os
            d_pluginManager_mp.get(),
            &d_bufferFactory,
            d_allocatorsStatContext_p,
            d_scheduler_p,
            d_allocators.get("StatController")),
        d_allocator_p);
    rc = d_statController_mp->start(errorDescription);
    if (rc != 0) {
        return (rc * 100) + rc_STATCONTROLLER;  // RETURN
    }

    // Start the config provider
    d_configProvider_mp.load(new (*d_allocator_p) ConfigProvider(
                                 d_allocators.get("ConfigProvider")),
                             d_allocator_p);
    rc = d_configProvider_mp->start(errorDescription);
    if (rc != 0) {
        return (rc * 100) + rc_CONFIGPROVIDER;  // RETURN
    }

    // Start dispatcher
    d_dispatcher_mp.load(new (*d_allocator_p) Dispatcher(
                             mqbcfg::BrokerConfig::get().dispatcherConfig(),
                             d_scheduler_p,
                             d_allocators.get("Dispatcher")),
                         d_allocator_p);
    rc = d_dispatcher_mp->start(errorDescription);
    if (0 != rc) {
        return (rc * 100) + rc_DISPATCHER;  // RETURN
    }

    // Start the transport manager
    SessionNegotiator* sessionNegotiator = new (*d_allocator_p)
        SessionNegotiator(&d_bufferFactory,
                          d_dispatcher_mp.get(),
                          d_statController_mp->clientsStatContext(),
                          &d_blobSpPool,
                          d_scheduler_p,
                          d_allocators.get("SessionNegotiator"));

    (*sessionNegotiator)
        .setAdminCommandEnqueueCallback(
            bdlf::BindUtil::bind(&Application::enqueueCommand,
                                 this,
                                 bdlf::PlaceHolders::_1,    // source
                                 bdlf::PlaceHolders::_2,    // cmd
                                 bdlf::PlaceHolders::_3));  // onProcessedCb

    bslma::ManagedPtr<mqbnet::Negotiator> negotiatorMp(sessionNegotiator,
                                                       d_allocator_p);

    d_transportManager_mp.load(new (*d_allocator_p) mqbnet::TransportManager(
                                   d_scheduler_p,
                                   &d_bufferFactory,
                                   negotiatorMp,
                                   d_statController_mp.get(),
                                   d_allocators.get("TransportManager")),
                               d_allocator_p);

    rc = d_transportManager_mp->start(errorDescription);
    if (rc != 0) {
        return (rc * 100) + rc_TRANSPORTMANAGER;  // RETURN
    }

    // TBD: Review lifecycle and ordering: ideally, objects should be created
    //      and started one after another.  Currently, because of
    //      inter-dependencies and domainFactory needed to be passed in to
    //      sessionNegotiator and cluster catalog, creation and start order is
    //      less than ideal

    bsl::unordered_map<bsl::string, mwcst::StatContext*> statContextsMap(
        d_allocator_p);
    d_statController_mp->loadStatContexts(&statContextsMap);

    // Start the ClusterCatalog
    d_clusterCatalog_mp.load(new (*d_allocator_p) mqbblp::ClusterCatalog(
                                 d_scheduler_p,
                                 d_dispatcher_mp.get(),
                                 d_transportManager_mp.get(),
                                 statContextsMap,
                                 &d_bufferFactory,
                                 &d_blobSpPool,
                                 d_allocators.get("ClusterCatalog")),
                             d_allocator_p);

    d_clusterCatalog_mp->setAdminCommandEnqueueCallback(
        bdlf::BindUtil::bind(&Application::enqueueCommand,
                             this,
                             bdlf::PlaceHolders::_1,    // source
                             bdlf::PlaceHolders::_2,    // cmd
                             bdlf::PlaceHolders::_3));  // onProcessedCb

    // Register the ClusterCatalog and TransportManager to the
    // SessionNegotiator.  Must be done before starting the ClusterCatalog
    // because in start, it will create clusters which will instantiates
    // mqbnet::Cluster using the negotiator.
    (*sessionNegotiator).setClusterCatalog(d_clusterCatalog_mp.get());

    rc = d_clusterCatalog_mp->loadBrokerClusterConfig(errorDescription);
    if (rc != 0) {
        return (rc * 100) + rc_BROKER_CLUSTER_CONFIG_LOADFAILURE;  // RETURN
    }

    // Start the DomainManager
    d_domainManager_mp.load(new (*d_allocator_p) DomainManager(
                                d_configProvider_mp.get(),
                                &d_bufferFactory,
                                d_clusterCatalog_mp.get(),
                                d_dispatcher_mp.get(),
                                d_statController_mp->domainsStatContext(),
                                d_statController_mp->domainQueuesStatContext(),
                                d_allocators.get("DomainManager")),
                            d_allocator_p);

    sessionNegotiator->setDomainFactory(d_domainManager_mp.get());
    d_clusterCatalog_mp->setDomainFactory(d_domainManager_mp.get());

    rc = d_domainManager_mp->start(errorDescription);
    if (rc != 0) {
        return (rc * 100) + rc_DOMAINMANAGER;  // RETURN
    }

    // Start the clusterCatalog
    rc = d_clusterCatalog_mp->start(errorDescription);
    if (rc != 0) {
        return (rc * 100) + rc_CLUSTERCATALOG;  // RETURN
    }

    // Everything started, start listening
    rc = d_transportManager_mp->startListening(errorDescription);
    if (rc != 0) {
        return (rc * 100) + rc_TRANSPORTMANAGER_LISTEN;  // RETURN
    }

    rc = d_clusterCatalog_mp->initiateReversedClusterConnections(
        errorDescription);
    if (rc != 0) {
        return (rc * 100) + rc_CLUSTER_REVERSECONNECTIONS_FAILURE;  // RETURN
    }

    rc = d_adminExecutionPool.start();
    if (rc != 0) {
        return (rc * 100) + rc_ADMIN_POOL_START_FAILURE;  // RETURN
    }

    BALL_LOG_INFO << "BMQbrkr started successfully";

    return rc_SUCCESS;
}

void Application::stop()
{
    BALL_LOG_INFO << bsl::endl
                  << "========== ============================== =========="
                  << bsl::endl
                  << "                      Stopping                      "
                  << bsl::endl
                  << "========== ============================== ==========";

#define STOP_OBJ(OBJ, NAME)                                                   \
    if (OBJ) {                                                                \
        BALL_LOG_INFO << "Stopping " NAME "...";                              \
        OBJ->stop();                                                          \
    }

#define DESTROY_OBJ(OBJ, NAME)                                                \
    if (OBJ) {                                                                \
        BALL_LOG_INFO << "Destroying " NAME "...";                            \
        OBJ.reset();                                                          \
    }

    // Stop listening so that any new connections are refused.
    BALL_LOG_INFO << "Stopping listening for new connections...";

    d_transportManager_mp->initiateShutdown();
    BALL_LOG_INFO << "Stopped listening for new connections.";

    // For each cluster in cluster catalog, inform peers about this shutdown.
    int          count = d_clusterCatalog_mp->count();
    bslmt::Latch latch(count);

    BALL_LOG_INFO << "Initiating " << count << " cluster(s) shutdown...";

    for (mqbblp::ClusterCatalogIterator clusterIt(d_clusterCatalog_mp.get());
         count > 0;
         ++clusterIt, --count) {
        clusterIt.cluster()->initiateShutdown(
            bdlf::BindUtil::bind(&bslmt::Latch::arrive, &latch));
    }
    latch.wait();

    // Close each SDK and proxy session in transport manager.
    BALL_LOG_INFO << "Closing client and proxy sessions...";
    d_transportManager_mp->closeClients();

    BALL_LOG_INFO << "Stopping admin thread pool...";
    d_adminExecutionPool.stop();

    // NOTE: Once we no longer call 'channel->close()' here,
    //       'mqbnet::TCPSessionFactory::stopListening' should be revisited
    //       with regards to its cancel of the heartbeat scheduler event.

    // STOP everything.

    // Note that we must do an out of order stop/destroy with respect to the
    // 'DomainManager' and 'ClusterCatalog' because it appears that the domain
    // manager contains objects that are held and owned by 'ClusterCatalog', so
    // it is believed that the relationship is not properly done.
    //
    // Note that clusterCatalog must be stopped before transport manager
    // because transportManager.stop() blocks until all sessions have been
    // destroyed, and above code proactively closes only the clientOrProxy
    // sessions; clusterNode ones are being destroyed by the clusterCatalog
    // calling stop on each cluster.
    STOP_OBJ(d_domainManager_mp, "DomainManager");
    STOP_OBJ(d_clusterCatalog_mp, "ClusterCatalog");
    STOP_OBJ(d_transportManager_mp, "TransportManager");
    STOP_OBJ(d_dispatcher_mp, "Dispatcher");
    STOP_OBJ(d_configProvider_mp, "ConfigProvider");
    STOP_OBJ(d_statController_mp, "StatController");
    STOP_OBJ(d_pluginManager_mp, "PluginManager");

    // and now DESTROY everything
    DESTROY_OBJ(d_domainManager_mp, "DomainManager");
    DESTROY_OBJ(d_clusterCatalog_mp, "ClusterCatalog");
    DESTROY_OBJ(d_transportManager_mp, "TransportManager");
    DESTROY_OBJ(d_dispatcher_mp, "Dispatcher");
    DESTROY_OBJ(d_configProvider_mp, "ConfigProvider");
    DESTROY_OBJ(d_statController_mp, "StatController");
    DESTROY_OBJ(d_pluginManager_mp, "PluginManager");

    BALL_LOG_INFO << "BMQbrkr stopped";

#undef DESTROY_OBJ
#undef STOP_OBJ
}

bool Application::isCommandForPrimary(mqbcmd::CommandChoice& command) const 
{
    if (command.isDomainsValue()) {
        mqbcmd::DomainsCommand& domains = command.domains();
        if (domains.isDomainValue()) {
            mqbcmd::DomainCommand& domain = domains.domain().command();
            if (domain.isPurgeValue()) {
                return true;
            }
            else if (domain.isQueueValue()) {
                if (domain.queue().command().isPurgeAppIdValue()) {
                    return true;
                }
            }
        }
    }
    else if (command.isClustersValue()) {
        mqbcmd::ClustersCommand& clusters = command.clusters();
        if (clusters.isClusterValue()) {
            mqbcmd::ClusterCommand& cluster = clusters.cluster().command();
            if (cluster.isForceGcQueuesValue()) {
                return true;
            }
        }
    }

    return false;
}

mqbi::Cluster* Application::getRelevantCluster(mqbcmd::CommandChoice& command, mqbcmd::InternalResult& cmdResult) const {
    if (command.isDomainsValue()) {
        const bsl::string& domainName = command.domains().domain().name();
        bsl::shared_ptr<mqbi::Domain> domainSp;
        // DomainManager::locateDomain acquires a lock internally, so this
        // should be safe to execute from Application
        if (d_domainManager_mp->locateDomain(&domainSp, domainName) != 0) {
            mwcu::MemOutStream os;
            os << "Domain '" << domainName << "' doesn't exist";
            cmdResult.makeError().message() = os.str();
            return nullptr;
        }
        return domainSp->cluster();
    }
    else if (command.isClustersValue()) {
        const bsl::string& clusterName = command.clusters().cluster().name();
        bsl::shared_ptr<mqbi::Cluster> clusterOut;
        // ClusterCatalog::findCluster acquires a lock internally, so this
        // should be safe to execute from Application 
        d_clusterCatalog_mp->findCluster(&clusterOut, clusterName);
        return clusterOut.get();
    }

    return nullptr;
}

void Application::onRerouteCommandResponse(const bsl::shared_ptr<
          mqbnet::MultiRequestManagerRequestContext<
            bmqp_ctrlmsg::ControlMessage, 
            bmqp_ctrlmsg::ControlMessage, 
            mqbnet::ClusterNode*
          >
        >& requestContext, bslmt::Latch& latch, bsl::ostream& os) {
    bsl::vector<bsl::pair<mqbnet::ClusterNode *, bmqp_ctrlmsg::ControlMessage>>  
            responses = requestContext->response();

    for (bsl::vector<bsl::pair<mqbnet::ClusterNode *, bmqp_ctrlmsg::ControlMessage>>::const_iterator 
            pairIt = responses.begin();
            pairIt != responses.end();
            pairIt++)
    {
        bsl::pair<mqbnet::ClusterNode *, bmqp_ctrlmsg::ControlMessage> pair = *pairIt;
        BALL_LOG_INFO << "response from " << pair.first->nodeId();
        pair.second.print(os);
    }

    BALL_LOG_INFO << "counting down latch";

    latch.countDown(1);
}

bool Application::routeCommandToPrimaryNodes(mqbi::Cluster* cluster, const bsl::string& cmd, bsl::ostream& os) {
    bsl::shared_ptr<
        bmqp::RequestManagerRequest<
            bmqp_ctrlmsg::ControlMessage,
            bmqp_ctrlmsg::ControlMessage
        >
    > request = cluster->requestManager().createRequest();

    bmqp_ctrlmsg::AdminCommand& adminCommand = 
                            request->request().choice().makeAdminCommand();
    adminCommand.command() = cmd;

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(1000);

    bsl::list<mqbnet::ClusterNode*> nodes;
    bool isSelfPrimary;
    BALL_LOG_INFO << "dispatching (??????)";
    cluster->dispatcher()->execute(
        bdlf::BindUtil::bind(
            &mqbi::Cluster::getPrimaryNodes,
            cluster,
            bsl::ref(nodes),
            bsl::ref(isSelfPrimary)
        ),
        cluster
    );
    BALL_LOG_INFO << "synchronizing......";
    cluster->dispatcher()->synchronize(cluster);

    BALL_LOG_INFO << "collected " << nodes.size() << " nodes";
    BALL_LOG_INFO << "is self primary? " << (isSelfPrimary ? "YES" : "NO");

    // is use-after-free possible here?
    // hypothetically: we get a pointer to all the cluster nodes,
    // one goes down and needs to be freed (does that happen)
    // then we could possibly crash BlazingMQ with segfault or UB.

    // 1 solution to make sure that never happens is to use a shared_ptr

    // ok, definitely not a use-after-free because ClusterNode instance is 
    // stored on the stack.

    if (nodes.size() > 0) {
        bsl::vector<mqbnet::ClusterNode*> nodesVector{ nodes.begin(), nodes.end() };
        mqbi::Cluster::MultiRequestManagerType::RequestContextSp contextSp =
            cluster->multiRequestManager().createRequestContext();

        contextSp->request()
            .choice()
            .makeAdminCommand()
            .command() = cmd;

        contextSp->setDestinationNodes(nodesVector);

        bslmt::Latch latch{1};

        contextSp->setResponseCb(
            bdlf::BindUtil::bind(&Application::onRerouteCommandResponse,
                                this,
                                bdlf::PlaceHolders::_1,
                                bsl::ref(latch),
                                bsl::ref(os)));

        cluster->multiRequestManager().sendRequest(contextSp, bsls::TimeInterval(3));

        latch.wait();
    }

    return isSelfPrimary;
}

int Application::executeCommand(mqbcmd::CommandChoice& command, 
                                mqbcmd::InternalResult& cmdResult)
{
    int rc;
    if (command.isHelpValue()) {
        const bool isPlumbing = command.help().plumbing();

        mqbcmd::Help help;
        mqbcmd::CommandList::loadCommands(&help, isPlumbing);
        cmdResult.makeHelp(help);
    }
    else if (command.isDomainsValue()) {
        mqbcmd::DomainsResult domainsResult;
        d_domainManager_mp->processCommand(&domainsResult, command.domains());
        if (domainsResult.isErrorValue()) {
            cmdResult.makeError(domainsResult.error());
        }
        else if (domainsResult.isSuccessValue()) {
            cmdResult.makeSuccess();
        }
        else {
            cmdResult.makeDomainsResult(domainsResult);
        }
    }
    else if (command.isConfigProviderValue()) {
        mqbcmd::Error error;
        rc = d_configProvider_mp->processCommand(command.configProvider(),
                                                &error);
        if (rc == 0) {
            cmdResult.makeSuccess();
        }
        else {
            cmdResult.makeError(error);
        }
    }
    else if (command.isStatValue()) {
        mqbcmd::StatResult statResult;
        d_statController_mp->processCommand(&statResult, command.stat());
        if (statResult.isErrorValue()) {
            cmdResult.makeError(statResult.error());
        }
        else {
            cmdResult.makeStatResult(statResult);
        }
    }
    else if (command.isClustersValue()) {
        mqbcmd::ClustersResult clustersResult;
        d_clusterCatalog_mp->processCommand(&clustersResult,
                                            command.clusters());
        if (clustersResult.isErrorValue()) {
            cmdResult.makeError(clustersResult.error());
        }
        else if (clustersResult.isSuccessValue()) {
            cmdResult.makeSuccess(clustersResult.success());
        }
        else {
            cmdResult.makeClustersResult(clustersResult);
        }
    }
    else if (command.isDangerValue()) {
        // Intentially _undocumented_ *DANGEROUS* commands!!
        if (command.danger().isShutdownValue()) {
            mqbu::ExitUtil::shutdown(mqbu::ExitCode::e_REQUESTED);
            return 1;  // RETURN
        }
        else if (command.danger().isTerminateValue()) {
            mqbu::ExitUtil::terminate(mqbu::ExitCode::e_REQUESTED);
            // See the implementation of 'mqbu::ExitUtil::terminate'.  It might
            // return.
            return 1;  // RETURN
        }
    }
    else if (command.isBrokerConfigValue()) {
        if (command.brokerConfig().isDumpValue()) {
            baljsn::Encoder        encoder;
            baljsn::EncoderOptions options;
            options.setEncodingStyle(baljsn::EncoderOptions::e_PRETTY);
            options.setInitialIndentLevel(1);
            options.setSpacesPerLevel(4);

            mwcu::MemOutStream brokerConfigOs;
            rc = encoder.encode(brokerConfigOs,
                                mqbcfg::BrokerConfig::get(),
                                options);
            if (rc != 0) {
                cmdResult.makeError().message() = brokerConfigOs.str();
            }
            else {
                cmdResult.makeBrokerConfig();
                cmdResult.brokerConfig().asJSON() = brokerConfigOs.str();
            }
        }
    }
    else {
        mwcu::MemOutStream errorOs;
        errorOs << "Unknown command '" << command << "'";
        cmdResult.makeError().message() = errorOs.str();
    }

    return 0;
}

int Application::processCommand(const bslstl::StringRef& source,
                                const bsl::string&       cmd,
                                bsl::ostream&            os)
{
    BALL_LOG_INFO << "Received command '" << cmd << "' "
                  << "[source: " << source << "]";

    mqbcmd::Command commandWithOptions;
    bsl::string     parseError;
    if (const int rc = mqbcmd::ParseUtil::parse(&commandWithOptions,
                                                &parseError,
                                                cmd)) {
        os << "Unable to decode command "
           << "(rc: " << rc << ", error: '" << parseError << "')";
        return rc;  // RETURN
    }

    mqbcmd::CommandChoice& command = commandWithOptions.choice();

    mqbcmd::InternalResult cmdResult;
    int                    rc = 0;

    bool isSourceReroute = source == "reroute";
    bool isPrimaryCommand = isCommandForPrimary(command);
    bool isSelfPrimary = false;
    
    if (!isSourceReroute && isPrimaryCommand) {
        mqbi::Cluster* cluster = getRelevantCluster(command, cmdResult);
        if (!cmdResult.isErrorValue()) {
            isSelfPrimary = routeCommandToPrimaryNodes(cluster, cmd, os);
            cmdResult.makeSuccess();
        }
    }
    
    // 3 cases to execute this route
    //  1. the command wasn't for a primary
    //  2. the command was for a primary and we are primary
    //  3. the command was for a primary and we are a reroute 

    // this logic should be cleaned up somehow
    if (!cmdResult.isErrorValue() && (!isPrimaryCommand || isSelfPrimary || isSourceReroute)) { // messy logic currently
        if (executeCommand(command, cmdResult)) {
            return 0; // early exit (caused by "dangerous" command)
        }
    }

    // Flatten into the final result
    mqbcmd::Result result;
    mqbcmd::Util::flatten(&result, cmdResult);

    switch (commandWithOptions.encoding()) {
    case mqbcmd::EncodingFormat::TEXT: {
        // Pretty print
        mqbcmd::HumanPrinter::print(os, result);
    } break;  // BREAK
    case mqbcmd::EncodingFormat::JSON_COMPACT: {
        mqbcmd::JsonPrinter::print(os, result, false);
    } break;  // BREAK
    case mqbcmd::EncodingFormat::JSON_PRETTY: {
        mqbcmd::JsonPrinter::print(os, result, true);
    } break;  // BREAK
    default: BSLS_ASSERT_SAFE(false && "Unsupported encoding");
    }

    return result.isErrorValue() ? -2 : 0;
}

int Application::processCommandCb(
    const bslstl::StringRef&                            source,
    const bsl::string&                                  cmd,
    const bsl::function<void(int, const bsl::string&)>& onProcessedCb)
{
    mwcu::MemOutStream os;
    int                rc = processCommand(source, cmd, os);

    onProcessedCb(rc, os.str());

    return rc;
}

int Application::enqueueCommand(
    const bslstl::StringRef&                            source,
    const bsl::string&                                  cmd,
    const bsl::function<void(int, const bsl::string&)>& onProcessedCb)
{
    BALL_LOG_TRACE << "Enqueuing admin command '" << cmd
                   << "' [source: " << source
                   << "] to the execution pool [numPendingJobs: "
                   << d_adminExecutionPool.numPendingJobs() << "]";

    return d_adminExecutionPool.enqueueJob(
        bdlf::BindUtil::bind(&Application::processCommandCb,
                             this,
                             source,
                             cmd,
                             onProcessedCb));
}

}  // close package namespace
}  // close enterprise namespace
