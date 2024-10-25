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

// mqba_application.cpp                                               -*-C++-*-
#include <mqba_application.h>

#include <mqbscm_version.h>
// MQB
#include <mqba_configprovider.h>
#include <mqba_dispatcher.h>
#include <mqba_domainmanager.h>
#include <mqba_sessionnegotiator.h>
#include <mqbblp_clustercatalog.h>
#include <mqbblp_relayqueueengine.h>
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

#include <bmqscm_version.h>
#include <bmqst_statcontext.h>
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_operationchain.h>

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
const int                k_BLOBBUFFER_SIZE           = 4 * 1024;
const int                k_BLOB_POOL_GROWTH_STRATEGY = 1024;
const bsls::Types::Int64 k_STOP_REQUEST_TIMEOUT_MS   = 5000;

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
        bmqsys::Time::initialize();

        // Initialize pseudo-random number generator.  We add some
        // machine-specific (high resolution timer) and task-specific (pid)
        // information to take care of scenarios when nodes are started at the
        // same time or on the same physical host.
        unsigned int seed =
            bsl::time(NULL) +
            static_cast<unsigned int>(bdls::ProcessUtil::getProcessId()) +
            static_cast<unsigned int>(bmqsys::Time::highResolutionTimer() &
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
        bmqsys::Time::shutdown();
    }
}

// CREATORS
Application::Application(bdlmt::EventScheduler* scheduler,
                         bmqst::StatContext*    allocatorsStatContext,
                         bslma::Allocator*      allocator)
: d_allocators(allocator)
, d_scheduler_p(scheduler)
, d_adminExecutionPool(bmqsys::ThreadUtil::defaultAttributes(),
                       0,
                       1,
                       bsls::TimeInterval(120).totalMilliseconds(),
                       allocator)
, d_adminRerouteExecutionPool(bmqsys::ThreadUtil::defaultAttributes(),
                              0,
                              1,
                              bsls::TimeInterval(120).totalMilliseconds(),
                              allocator)
, d_bufferFactory(k_BLOBBUFFER_SIZE,
                  bsls::BlockGrowth::BSLS_CONSTANT,
                  d_allocators.get("BufferFactory"))

, d_blobSpPool(bdlf::BindUtil::bind(&createBlob,
                                    &d_bufferFactory,
                                    bdlf::PlaceHolders::_1,   // arena
                                    bdlf::PlaceHolders::_2),  // allocator
               k_BLOB_POOL_GROWTH_STRATEGY,
               d_allocators.get("BlobSpPool"))
, d_pushElementsPool(sizeof(mqbblp::PushStream::Element),
                     d_allocators.get("PushElementsPool"))
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
        << "Starting" << "\n   ____  __  __  ___  _               _"
        << "\n  | __ )|  \\/  |/ _ \\| |__  _ __ ___ | | _____ _ __"
        << "\n  |  _ \\| |\\/| | | | | '_ \\| '__/ _ \\| |/ / _ \\ '__|"
        << "\n  | |_) | |  | | |_| | |_) | | | (_) |   <  __/ |"
        << "\n  |____/|_|  |_|\\__\\_\\_.__/|_|  \\___/|_|\\_\\___|_|" << "\n"
        << "\n    Instance..............: " << brkrCfg.brokerInstanceName()
        << "\n    Version...............: " << brkrCfg.brokerVersion()
        << "\n    Build Type............: " << MQBA_STRINGIFY(BMQ_BUILD_TYPE)
        << "\n    Assert Level..........: " << assertLevel
        << "\n    Config version........: " << brkrCfg.configVersion()
        << "\n    BMQ version...........: " << bmqscm::Version::version()
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
    bsl::unordered_map<bsl::string, bmqst::StatContext*>* contexts) const
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

    mqbi::ClusterResources resources(d_scheduler_p,
                                     &d_bufferFactory,
                                     &d_blobSpPool,
                                     &d_pushElementsPool);

    // Start the StatController
    d_statController_mp.load(
        new (*d_allocator_p) mqbstat::StatController(
            bdlf::BindUtil::bind(&Application::processCommand,
                                 this,
                                 bdlf::PlaceHolders::_1,  // source
                                 bdlf::PlaceHolders::_2,  // cmd
                                 bdlf::PlaceHolders::_3,  // os
                                 false),                  // fromReroute
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
                                 bdlf::PlaceHolders::_3,    // onProcessedCb
                                 bdlf::PlaceHolders::_4));  // fromReroute

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

    bsl::unordered_map<bsl::string, bmqst::StatContext*> statContextsMap(
        d_allocator_p);
    d_statController_mp->loadStatContexts(&statContextsMap);

    // Start the ClusterCatalog
    d_clusterCatalog_mp.load(new (*d_allocator_p) mqbblp::ClusterCatalog(
                                 d_dispatcher_mp.get(),
                                 d_transportManager_mp.get(),
                                 statContextsMap,
                                 resources,
                                 d_allocators.get("ClusterCatalog")),
                             d_allocator_p);

    d_clusterCatalog_mp->setAdminCommandEnqueueCallback(
        bdlf::BindUtil::bind(&Application::enqueueCommand,
                             this,
                             bdlf::PlaceHolders::_1,  // source
                             bdlf::PlaceHolders::_2,  // cmd
                             bdlf::PlaceHolders::_3,  // onProcessedCb
                             bdlf::PlaceHolders::_4   // fromReroute
                             ));

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

    rc = d_adminRerouteExecutionPool.start();
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

    bool supportShutdownV2 = initiateShutdown();

    if (supportShutdownV2) {
        BALL_LOG_INFO << ": Executing GRACEFUL_SHUTDOWN_V2";
    }
    else {
        BALL_LOG_INFO << ": Peers do not support "
                      << "GRACEFUL_SHUTDOWN_V2. Retreat to V1";
    }

    // For each cluster in cluster catalog, inform peers about this shutdown.
    int          count = d_clusterCatalog_mp->count();
    bslmt::Latch latch(count);

    BALL_LOG_INFO << "Initiating " << count << " cluster(s) shutdown...";

    for (mqbblp::ClusterCatalogIterator clusterIt(d_clusterCatalog_mp.get());
         count > 0;
         ++clusterIt, --count) {
        clusterIt.cluster()->initiateShutdown(
            bdlf::BindUtil::bind(&bslmt::Latch::arrive, &latch),
            supportShutdownV2);
    }
    latch.wait();

    // Close each SDK and proxy session in transport manager.
    BALL_LOG_INFO << "Closing client and proxy sessions...";
    d_transportManager_mp->closeClients();

    BALL_LOG_INFO << "Stopping admin thread pool...";
    d_adminExecutionPool.stop();

    BALL_LOG_INFO << "Stopping admin reroute thread pool...";
    d_adminRerouteExecutionPool.stop();

    // NOTE: Once we no longer call 'channel->close()' here,
    //       'mqbnet::TCPSessionFactory::stopListening' should be revisited
    //       with regards to its cancel of the heartbeat scheduler event.

    // STOP everything.

    // Note that clusterCatalog must be stopped before transport manager
    // because transportManager.stop() blocks until all sessions have been
    // destroyed, and above code proactively closes only the clientOrProxy
    // sessions; clusterNode ones are being destroyed by the clusterCatalog
    // calling stop on each cluster.

    STOP_OBJ(d_clusterCatalog_mp, "ClusterCatalog");
    STOP_OBJ(d_transportManager_mp, "TransportManager");
    STOP_OBJ(d_domainManager_mp, "DomainManager");
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

bool Application::initiateShutdown()
{
    typedef bsl::vector<bsl::shared_ptr<mqbnet::Session> > Sessions;

    // Send a StopRequest to all connected cluster nodes and brokers
    Sessions brokers(d_allocator_p);
    Sessions clients(d_allocator_p);

    for (mqbnet::TransportManagerIterator sessIt(d_transportManager_mp.get());
         sessIt;
         ++sessIt) {
        bsl::shared_ptr<mqbnet::Session> sessionSp = sessIt.session().lock();
        if (!sessionSp) {
            continue;  // CONTINUE
        }

        const bmqp_ctrlmsg::NegotiationMessage& negoMsg =
            sessionSp->negotiationMessage();

        const bmqp_ctrlmsg::ClientIdentity& peerIdentity =
            negoMsg.isClientIdentityValue()
                ? negoMsg.clientIdentity()
                : negoMsg.brokerResponse().brokerIdentity();

        bool isBroker = false;
        if (mqbnet::ClusterUtil::isClientOrProxy(negoMsg)) {
            clients.push_back(sessionSp);
            if (!negoMsg.clientIdentity().clusterName().empty()) {
                isBroker = true;
            }
        }
        else {
            isBroker = true;
        }
        if (isBroker) {
            // Node or Proxy
            // Expect all proxies and nodes support this feature.
            if (!bmqp::ProtocolUtil::hasFeature(
                    bmqp::HighAvailabilityFeatures::k_FIELD_NAME,
                    bmqp::HighAvailabilityFeatures::k_GRACEFUL_SHUTDOWN,
                    peerIdentity.features())) {
                BALL_LOG_ERROR << ": Peer doesn't support "
                               << "GRACEFUL_SHUTDOWN. Skip sending stopRequest"
                               << " to [" << peerIdentity << "]";
                continue;  // CONTINUE
            }
            if (!bmqp::ProtocolUtil::hasFeature(
                    bmqp::HighAvailabilityFeatures::k_FIELD_NAME,
                    bmqp::HighAvailabilityFeatures::k_GRACEFUL_SHUTDOWN_V2,
                    peerIdentity.features())) {
                // Abandon the attempt to shutdown V2
                return false;  // RETURN
            }
            brokers.push_back(sessionSp);
        }
    }

    bslmt::Latch latch(clients.size() + 1);
    // The 'StopRequestManagerType::sendRequest' always calls 'd_responseCb'.

    mqbblp::ClusterCatalog::StopRequestManagerType::RequestContextSp
        contextSp =
            d_clusterCatalog_mp->stopRequestManger().createRequestContext();

    bmqp_ctrlmsg::StopRequest& request = contextSp->request()
                                             .choice()
                                             .makeClusterMessage()
                                             .choice()
                                             .makeStopRequest();

    request.version() = 2;

    bsls::TimeInterval shutdownTimeout;

    shutdownTimeout.setTotalMilliseconds(k_STOP_REQUEST_TIMEOUT_MS);

    contextSp->setDestinationNodes(brokers);

    contextSp->setResponseCb(
        bdlf::BindUtil::bind(&bslmt::Latch::arrive, &latch));

    BALL_LOG_INFO << "Sending StopRequest V2 to " << brokers.size()
                  << " brokers; timeout is " << shutdownTimeout << " ms";

    d_clusterCatalog_mp->stopRequestManger().sendRequest(contextSp,
                                                         shutdownTimeout);

    BALL_LOG_INFO << "Shutting down " << clients.size()
                  << " clients; timeout is " << shutdownTimeout << " ms";

    for (Sessions::const_iterator cit = clients.begin(); cit != clients.end();
         ++cit) {
        (*cit)->initiateShutdown(bdlf::BindUtil::bind(&bslmt::Latch::arrive,
                                                      &latch),
                                 shutdownTimeout,
                                 true);
    }

    // Need to wait for peers to update this node status to guarantee no new
    // clusters.
    latch.wait();

    return true;
}

mqbi::Cluster*
Application::getRelevantCluster(bsl::ostream&          errorDescription,
                                const mqbcmd::Command& command) const
{
    const mqbcmd::CommandChoice& commandChoice = command.choice();

    if (commandChoice.isDomainsValue()) {
        const mqbcmd::DomainsCommand& domains = commandChoice.domains();

        bsl::string domainName;
        if (domains.isDomainValue()) {
            domainName = domains.domain().name();
        }
        else if (domains.isReconfigureValue()) {
            domainName = domains.reconfigure().domain();
        }
        else {
            errorDescription << "Cannot extract cluster for that command";
            return NULL;  // RETURN
        }

        // Attempt to locate the domain
        bsl::shared_ptr<mqbi::Domain> domainSp;
        if (0 !=
            d_domainManager_mp->locateOrCreateDomain(&domainSp, domainName)) {
            errorDescription << "Domain '" << domainName << "' doesn't exist";
            return NULL;  // RETURN
        }

        return domainSp->cluster();  // RETURN
    }
    else if (commandChoice.isClustersValue()) {
        const bsl::string& clusterName =
            commandChoice.clusters().cluster().name();
        bsl::shared_ptr<mqbi::Cluster> clusterOut;
        if (!d_clusterCatalog_mp->findCluster(&clusterOut, clusterName)) {
            errorDescription << "Cluster '" << clusterName
                             << "' doesn't exist";
            return NULL;  // RETURN
        }
        return clusterOut.get();  // RETURN
    }

    errorDescription << "Cannot extract cluster for that command";
    return NULL;  // RETURN
}

int Application::executeCommand(const mqbcmd::Command&  command,
                                mqbcmd::InternalResult* cmdResult)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cmdResult);

    enum RcEnum {
        rc_SUCCESS    = 0,
        rc_EARLY_EXIT = -1,
    };

    const mqbcmd::CommandChoice& commandChoice = command.choice();

    int rc;
    if (commandChoice.isHelpValue()) {
        const bool isPlumbing = commandChoice.help().plumbing();

        mqbcmd::Help help;
        mqbcmd::CommandList::loadCommands(&help, isPlumbing);
        cmdResult->makeHelp(help);
    }
    else if (commandChoice.isDomainsValue()) {
        mqbcmd::DomainsResult domainsResult;
        d_domainManager_mp->processCommand(&domainsResult,
                                           commandChoice.domains());
        if (domainsResult.isErrorValue()) {
            cmdResult->makeError(domainsResult.error());
        }
        else if (domainsResult.isSuccessValue()) {
            cmdResult->makeSuccess();
        }
        else {
            cmdResult->makeDomainsResult(domainsResult);
        }
    }
    else if (commandChoice.isConfigProviderValue()) {
        mqbcmd::Error error;
        rc = d_configProvider_mp->processCommand(
            commandChoice.configProvider(),
            &error);
        if (rc == 0) {
            cmdResult->makeSuccess();
        }
        else {
            cmdResult->makeError(error);
        }
    }
    else if (commandChoice.isStatValue()) {
        mqbcmd::StatResult statResult;
        d_statController_mp->processCommand(&statResult,
                                            commandChoice.stat(),
                                            command.encoding());
        if (statResult.isErrorValue()) {
            cmdResult->makeError(statResult.error());
        }
        else {
            cmdResult->makeStatResult(statResult);
        }
    }
    else if (commandChoice.isClustersValue()) {
        mqbcmd::ClustersResult clustersResult;
        d_clusterCatalog_mp->processCommand(&clustersResult,
                                            commandChoice.clusters());
        if (clustersResult.isErrorValue()) {
            cmdResult->makeError(clustersResult.error());
        }
        else if (clustersResult.isSuccessValue()) {
            cmdResult->makeSuccess(clustersResult.success());
        }
        else {
            cmdResult->makeClustersResult(clustersResult);
        }
    }
    else if (commandChoice.isDangerValue()) {
        // Intentially _undocumented_ *DANGEROUS* commands!!
        if (commandChoice.danger().isShutdownValue()) {
            mqbu::ExitUtil::shutdown(mqbu::ExitCode::e_REQUESTED);
            return rc_EARLY_EXIT;  // RETURN
        }
        else if (commandChoice.danger().isTerminateValue()) {
            mqbu::ExitUtil::terminate(mqbu::ExitCode::e_REQUESTED);
            // See the implementation of 'mqbu::ExitUtil::terminate'.  It might
            // return.
            return rc_EARLY_EXIT;  // RETURN
        }
    }
    else if (commandChoice.isBrokerConfigValue()) {
        if (commandChoice.brokerConfig().isDumpValue()) {
            baljsn::Encoder        encoder;
            baljsn::EncoderOptions options;
            options.setEncodingStyle(baljsn::EncoderOptions::e_PRETTY);
            options.setInitialIndentLevel(1);
            options.setSpacesPerLevel(4);

            bmqu::MemOutStream brokerConfigOs;
            rc = encoder.encode(brokerConfigOs,
                                mqbcfg::BrokerConfig::get(),
                                options);
            if (rc != 0) {
                cmdResult->makeError().message() = brokerConfigOs.str();
            }
            else {
                cmdResult->makeBrokerConfig();
                cmdResult->brokerConfig().asJSON() = brokerConfigOs.str();
            }
        }
    }
    else {
        bmqu::MemOutStream errorOs;
        errorOs << "Unknown command '" << commandChoice << "'";
        cmdResult->makeError().message() = errorOs.str();
    }

    return rc_SUCCESS;
}

int Application::processCommand(const bslstl::StringRef& source,
                                const bsl::string&       cmd,
                                bsl::ostream&            os,
                                bool                     fromReroute)
{
    enum RcEnum {
        rc_SUCCESS     = 0,
        rc_EARLY_EXIT  = -1,
        rc_ERROR       = -2,
        rc_PARSE_ERROR = -3,
    };

    BALL_LOG_INFO << "Received command '" << cmd << "' "
                  << "[source: " << source << "]";

    mqbcmd::Command command;
    bsl::string     parseError;
    if (const int rc = mqbcmd::ParseUtil::parse(&command, &parseError, cmd)) {
        os << "Unable to decode command " << "(rc: " << rc << ", error: '"
           << parseError << "')";
        return rc + 10 * rc_PARSE_ERROR;  // RETURN
    }

    mqbcmd::InternalResult cmdResult;

    // Note that routed commands should never route again to another node.
    // This should always be the "end of the road" for a command.
    // Note that this logic is important to prevent a "deadlock" scenario
    // where two nodes are waiting on a response from each other to continue.
    // Currently commands from reroutes are executed on their own dedicated
    // thread.
    if (fromReroute) {
        if (0 != executeCommand(command, &cmdResult)) {
            // early exit (caused by "dangerous" command)
            return rc_EARLY_EXIT;  // RETURN
        }
        mqbcmd::Util::printCommandResult(cmdResult, command.encoding(), os);
        return cmdResult.isErrorValue() ? rc_ERROR : rc_SUCCESS;  // RETURN
    }

    // Otherwise, this is an original call. Utilize router if necessary
    mqba::CommandRouter routeCommandManager(cmd, command);

    bool        shouldSelfExecute = true;
    bsl::string selfName;

    if (routeCommandManager.isRoutingNeeded()) {
        bmqu::MemOutStream errorDescription;

        mqbi::Cluster* cluster = getRelevantCluster(errorDescription, command);
        if (cluster == NULL) {  // Error occurred getting cluster
            cmdResult.makeError().message() = errorDescription.str();
            mqbcmd::Util::printCommandResult(cmdResult,
                                             command.encoding(),
                                             os);
            return rc_ERROR;  // RETURN
        }

        if (const int rc = routeCommandManager.route(errorDescription,
                                                     &shouldSelfExecute,
                                                     cluster)) {
            BALL_LOG_ERROR << "Failed to route command (rc: " << rc
                           << ", error: '" << errorDescription.str() << "')";
            cmdResult.makeError().message() = errorDescription.str();
            mqbcmd::Util::printCommandResult(cmdResult,
                                             command.encoding(),
                                             os);
            return rc_ERROR;  // RETURN
        }

        selfName = cluster->netCluster().selfNode()->hostName();
    }

    if (shouldSelfExecute) {
        if (0 != executeCommand(command, &cmdResult)) {
            // early exit (caused by "dangerous" command)
            return rc_EARLY_EXIT;  // RETURN
        }
    }

    // While we wait we are blocking the execution of any subsequent commands.
    routeCommandManager.waitForResponses();

    mqbcmd::RouteResponseList& responses = routeCommandManager.responses();

    if (shouldSelfExecute) {
        // Add self response (executed earlier)
        bmqu::MemOutStream cmdOs;
        mqbcmd::Util::printCommandResult(cmdResult, command.encoding(), cmdOs);
        mqbcmd::RouteResponse routeResponse;
        routeResponse.response()              = cmdOs.str();
        routeResponse.sourceNodeDescription() = selfName;
        responses.responses().push_back(routeResponse);
    }

    mqbcmd::Util::printCommandResponses(responses, command.encoding(), os);

    return cmdResult.isErrorValue() ? rc_ERROR : rc_SUCCESS;  // RETURN
}

int Application::processCommandCb(
    const bslstl::StringRef&                            source,
    const bsl::string&                                  cmd,
    const bsl::function<void(int, const bsl::string&)>& onProcessedCb,
    bool                                                fromReroute)
{
    bmqu::MemOutStream os;
    int                rc = processCommand(source, cmd, os, fromReroute);

    onProcessedCb(rc, os.str());

    return rc;  // RETURN
}

int Application::enqueueCommand(
    const bslstl::StringRef&                            source,
    const bsl::string&                                  cmd,
    const bsl::function<void(int, const bsl::string&)>& onProcessedCb,
    bool                                                fromReroute)
{
    bdlmt::ThreadPool* threadPool = fromReroute ? &d_adminRerouteExecutionPool
                                                : &d_adminExecutionPool;

    BALL_LOG_TRACE << "Enqueuing admin command '" << cmd
                   << "' [source: " << source
                   << "] to the execution pool [numPendingJobs: "
                   << threadPool->numPendingJobs() << "]";

    return threadPool->enqueueJob(
        bdlf::BindUtil::bind(&Application::processCommandCb,
                             this,
                             source,
                             cmd,
                             onProcessedCb,
                             fromReroute));
}

}  // close package namespace
}  // close enterprise namespace
