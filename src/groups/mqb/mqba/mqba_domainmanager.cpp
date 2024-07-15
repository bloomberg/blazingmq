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

// mqba_domainmanager.cpp                                             -*-C++-*-
#include <mqba_domainmanager.h>

#include <mqbscm_version.h>
// MQB
#include <mqba_configprovider.h>
#include <mqba_domainresolver.h>
#include <mqbblp_cluster.h>
#include <mqbblp_clustercatalog.h>
#include <mqbblp_domain.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbcmd_messages.h>
#include <mqbi_domain.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MWC
#include <mwcst_statcontext.h>
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>
#include <mwcu_sharedresource.h>
#include <mwcu_stringutil.h>
#include <mwcu_weakmemfn.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_cstddef.h>
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bslmt_latch.h>
#include <bslmt_lockguard.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {
const int k_MAX_WAIT_SECONDS_AT_SHUTDOWN = 40;

/// This function is a callback passed to domain manager for
/// synchronization.  The specified 'status' is a return status
/// of a function which called this callback.  The specified
/// 'domain' is optional Domain returned from a caller.
/// The specified 'latch' is used for synchronization with
/// external source and called after all the interesting
/// processing is done.
void onDomain(const bmqp_ctrlmsg::Status& status,
              mqbi::Domain*               domain,
              bslmt::Latch*               latch)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(latch);

    if (bmqp_ctrlmsg::StatusCategory::E_SUCCESS != status.category()) {
        BSLS_ASSERT_SAFE(0 == domain);
    }
    else {
        BSLS_ASSERT_SAFE(domain);
    }

    latch->arrive();
}

}  // close unnamed namespace

// ===========================
// struct DomainManager::Error
// ===========================

struct DomainManager::Error {
    // CREATORS

    /// Error constructor.  Creates a default error representation.
    Error()
    : d_status()
    , d_details()
    , d_isFatal(false)
    {
    }

    /// `Error` constructor.  Uses the specified `category`, `code`,
    /// `message`, `details` and optionally specified `isFatal` to create an
    /// `Error`.
    Error(const bmqp_ctrlmsg::StatusCategory::Value category,
          int                                       code,
          const bsl::string&                        message,
          const bsl::string&                        details,
          const bool                                isFatal = false)
    : d_status()
    , d_details(details)
    , d_isFatal(isFatal)
    {
        d_status.category() = category;
        d_status.code()     = code;
        d_status.message()  = message;
    }

    /// `Error` constructor.  Uses the specified `status`, `details` and and
    /// optionally specified `isFatal` to create an `Error`.
    Error(const bmqp_ctrlmsg::Status& status,
          const bsl::string&          details,
          const bool                  isFatal = false)
    : d_status(status)
    , d_details(details)
    , d_isFatal(isFatal)
    {
    }

    // DATA
    bmqp_ctrlmsg::Status d_status;  // Status capturing category, code,
                                    // etc.

    bsl::string d_details;  // Long string with details that might
                            // help with debug.

    bool d_isFatal;  // If this is true don't try to load
                     // configuration from backup.

    // FRIENDS

    /// Writes output to the specified `stream`.
    friend inline bsl::ostream& operator<<(bsl::ostream&               stream,
                                           const DomainManager::Error& error)
    {
        stream << "[ status = " << error.d_status << " fatal "
               << bsl::boolalpha << error.d_isFatal << " details = [ "
               << error.d_details << " ] ]";
        return stream;
    }
};

// -------------------
// class DomainManager
// -------------------

void DomainManager::onDomainResolverDomainLocatedCb(
    const bmqp_ctrlmsg::Status&                status,
    const bsl::string&                         domainLocation,
    const bsl::string&                         domain,
    const mqbi::DomainFactory::CreateDomainCb& callback)
{
    //             executes on *ANY* thread
    // (in reality the domain resolver one)

    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        callback(status, 0);
        return;  // RETURN
    }

    BALL_LOG_INFO << "Domain '" << domain << "' located at " << domainLocation;

    if (!isClusterMember(domainLocation)) {
        initializeNonClusterMemberDomain(domain, domainLocation, callback);
        return;  // RETURN
    }

    DomainSp domainSp;
    if (locateDomain(&domainSp, domain) == 0) {
        // Domain found and currently in use, can't reload the config, so
        // simply return that domain.
        BALL_LOG_INFO << "createDomain('" << domain << "'): "
                      << "reusing existing domain";
        bmqp_ctrlmsg::Status success;
        success.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        success.code()     = 0;
        success.message()  = "";

        callback(success, domainSp.get());
        return;  // RETURN
    }

    // Domain not found, or not active, we should fetch its config (again)
    BALL_LOG_INFO << "createDomain('" << domain << "'): "
                  << "querying config provider";
    d_configProvider_p->getDomainConfig(
        domain,
        bdlf::BindUtil::bind(&DomainManager::onConfigProviderDomainConfigCb,
                             this,
                             bdlf::PlaceHolders::_1,  // configProviderStatus
                             bdlf::PlaceHolders::_2,  // configProviderResult
                             domain,
                             domainLocation,
                             callback));
}

void DomainManager::onConfigProviderDomainConfigCb(
    int                                        configProviderStatus,
    const bsl::string&                         configProviderResult,
    const bsl::string&                         domain,
    const bsl::string&                         domainLocation,
    const mqbi::DomainFactory::CreateDomainCb& callback)
{
    // NOTE: We don't log many errors here, the caller (who provided the
    //       'callback') will get the full error string and will decide on how
    //       to deal with it. We only log if the primary (configProvider)
    //       failed, but the secondary (backup) succeeded, because in this
    //       case, callback will be called with a success result.

    DecodeAndUpsertValue configProviderUpsertResponse;
    decodeAndUpsert(&configProviderUpsertResponse,
                    configProviderStatus,
                    configProviderResult,
                    domain,
                    domainLocation);

    if (configProviderUpsertResponse.isValue() ||
        configProviderUpsertResponse.error().d_isFatal) {
        // If it's a success or a hopeless error case, send immediately.
        invokeCallback(configProviderUpsertResponse, callback);
        return;  // RETURN
    }

    // ConfigProvider returned an error.
    Error configProviderError = configProviderUpsertResponse.error();
    // This will print `d_message`, `d_rc`, `d_details` and `d_isFatal` for
    // errors from the config provider.
    BALL_LOG_ERROR << "#DOMAINMANAGER_INVALID_CONFIG "
                   << "ConfigProvider failed to load domain config "
                   << "[ domain = '" << domain
                   << "' configProviderError = " << configProviderError
                   << " ]";

    // This compiles the message that will be sent to the client. This contains
    // only `d_message` for brevity.
    mwcu::MemOutStream err;
    err << "ConfigProvider failed to load domain config "
        << "[ domain = '" << domain << "'"
        << " configProviderError = '" << configProviderError.d_status.message()
        << "'"
        << "' ]";

    // Modify 'message' field, which goes all the way to the client.  Detailed
    // message is already captured in 'err'.
    configProviderError.d_status.message() = "Attempt failed";

    DecodeAndUpsertValue result;
    result.makeError(Error(configProviderError.d_status, err.str()));
    invokeCallback(result, callback);
}

DomainManager::DecodeAndUpsertValue&
DomainManager::decodeAndUpsert(DecodeAndUpsertValue* out,
                               const int             status,
                               const bsl::string&    result,
                               const bsl::string&    domain,
                               const bsl::string&    clusterName)
{
    if (status != 0) {
        mwcu::MemOutStream err;
        err << "rc = " << status << " message = '" << result << "'";
        out->makeError(Error(bmqp_ctrlmsg::StatusCategory::E_REFUSED,
                             mqbi::ClusterErrorCode::e_UNKNOWN,
                             "Failed to retrieve domain config",
                             err.str()));
        return *out;  // RETURN
    }

    // Convert from JSON to 'DomainDefinition' schema object
    // - - - - - - - - - - - - - - - - - - - - - - - - - - -
    mqbconfm::DomainVariant domainVariant;
    baljsn::Decoder         decoder;
    baljsn::DecoderOptions  options;
    bsl::istringstream      jsonStream(result);

    options.setSkipUnknownElements(true);

    int rc = decoder.decode(jsonStream, &domainVariant, options);
    if (rc != 0) {
        mwcu::MemOutStream err;
        err << "rc = " << rc << " error = '" << decoder.loggedMessages() << "'"
            << " from content = '" << jsonStream.str() << "'";
        out->makeError(Error(bmqp_ctrlmsg::StatusCategory::E_REFUSED,
                             mqbi::ClusterErrorCode::e_UNKNOWN,
                             "Error decoding configuration",
                             err.str()));
        return *out;  // RETURN
    }

    if (!domainVariant.isDefinitionValue()) {
        mwcu::MemOutStream err;
        err << "invalid domain configuration for domain '" << domain
            << "' (not a domain definition)";
        out->makeError(Error(bmqp_ctrlmsg::StatusCategory::E_REFUSED,
                             mqbi::ClusterErrorCode::e_UNKNOWN,
                             "Error decoding configuration",
                             err.str()));
        return *out;  // RETURN
    }

    UpsertDomainValue upsertDomainValue;
    upsertDomain(&upsertDomainValue,
                 domainVariant.definition().parameters(),
                 domain,
                 clusterName);

    // Pass-through the value or the error.
    return upsertDomainValue.mapValue<DomainSp>(out, &toDomainSp);
}

void DomainManager::initializeNonClusterMemberDomain(
    const bsl::string&                         domain,
    const bsl::string&                         domainLocation,
    const mqbi::DomainFactory::CreateDomainCb& callback)
{
    UpsertDomainValue upsertDomainValue;
    mqbconfm::Domain  definition;
    upsertDomain(&upsertDomainValue, definition, domain, domainLocation);

    DecodeAndUpsertValue decodeAndUpsertValue;
    invokeCallback(upsertDomainValue.mapValue<DomainSp>(&decodeAndUpsertValue,
                                                        &toDomainSp),
                   callback);
}

DomainManager::UpsertDomainValue&
DomainManager::upsertDomain(UpsertDomainValue*             out,
                            const mqbconfm::Domain&        definition,
                            const bsl::string&             domain,
                            const bsl::string&             clusterName,
                            bsl::shared_ptr<mqbi::Cluster> cluster)
{
    // Enter 'single-threaded' configuration part
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // mutex LOCKED

    // Make sure the domain is not already configured
    // - - - - - - - - - - - - - - - - - - - - - - -
    // Check that the domain was not already added (while waiting on the
    // response, or the mutex, ...)
    DomainSpMap::const_iterator it = d_domains.find(domain);

    DomainSp domainSp;

    // If domain is found, we should reuse and reconfigure it
    // - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if (it != d_domains.end()) {
        domainSp = it->second;
    }
    else {
        if (cluster == 0) {
            // Release lock before creating the cluster which involves
            // synchronizing with cluster dispatcher thread using mutex.
            guard.release()->unlock();  // UNLOCK

            // Get or create the cluster
            bmqp_ctrlmsg::Status status =
                d_clusterCatalog_p->getCluster(&cluster, clusterName);

            if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
                // This is a fatal error (backup won't be tried).  Since the
                // cluster description is not backed up and not coming from the
                // domain configuration, there is no need to try to fall back
                // if this fails..
                mwcu::MemOutStream err;
                err << "cluster = " << clusterName << " status = " << status;
                out->makeError(Error(status, err.str(), true));
                return *out;  // RETURN
            }

            BSLS_ASSERT_OPT(cluster);

            return upsertDomain(out, definition, domain, clusterName, cluster);
            // RETURN
        }

        // Create a dedicated stats subcontext for this domain
        mwcst::StatContextConfiguration statContextCfg(domain);
        statContextCfg.storeExpiredSubcontextValues(true);
        bslma::ManagedPtr<mwcst::StatContext> queuesStatContext =
            d_queuesStatContext_p->addSubcontext(statContextCfg);

        bslma::ManagedPtr<mqbi::Domain> domainMp(
            new (*d_allocator_p) mqbblp::Domain(domain,
                                                d_dispatcher_p,
                                                d_blobBufferFactory_p,
                                                cluster,
                                                d_domainsStatContext_p,
                                                queuesStatContext,
                                                d_allocator_p),
            d_allocator_p);
        domainSp = DomainSp(domainMp, d_allocator_p);
    }

    // Configure the domain
    // - - - - - - - - - -
    mwcu::MemOutStream configureErrorStream;
    int rc = domainSp->configure(configureErrorStream, definition);
    if (rc != 0) {
        mwcu::MemOutStream err;
        err << "error = '" << configureErrorStream.str() << "'"
            << " config = '" << definition << "'";
        out->makeError(
            Error(bmqp_ctrlmsg::StatusCategory::E_REFUSED,
                  mqbi::ClusterErrorCode::e_UNKNOWN,  // Non-retryable
                  "Failed configuring domain",
                  err.str()));
        return *out;  // RETURN
    }

    // Insert the domain in the map
    // - - - - - - - - - - - - - -
    d_domains[domain] = domainSp;

    // Domain successfully inserted
    out->makeValue(UpsertDomainSuccess(domainSp, true));

    return *out;
}

void DomainManager::onDomainClosed(const bsl::string& domainName,
                                   bslmt::Latch*      latch)
{
    BSLS_ASSERT_SAFE(latch);

    BALL_LOG_INFO << "Domain [" << domainName << "] successfully closed.";

    latch->arrive();
}

bool DomainManager::isClusterMember(const bsl::string& domainLocation) const
{
    return d_clusterCatalog_p->isMemberOf(domainLocation);
}

void DomainManager::invokeCallback(
    const DecodeAndUpsertValue&                response,
    const mqbi::DomainFactory::CreateDomainCb& callback)
{
    bmqp_ctrlmsg::Status status;
    if (response.isError()) {
        status = response.error().d_status;

        // Rework the 'message' field of 'status' for improved format.
        mwcu::MemOutStream err;
        err << status.message() << ". Details: " << response.error().d_details;
        status.message().assign(err.str().data(), err.str().length());
        callback(status, 0);
    }
    else {
        status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        status.code()     = 0;
        status.message()  = "";
        callback(status, response.value().get());
    }
}

DomainManager::DomainSp
DomainManager::toDomainSp(const UpsertDomainSuccess& value)
{
    return value.first;
}

DomainManager::DomainManager(ConfigProvider*           configProvider,
                             bdlbb::BlobBufferFactory* blobBufferFactory,
                             mqbblp::ClusterCatalog*   clusterCatalog,
                             mqbi::Dispatcher*         dispatcher,
                             mwcst::StatContext*       domainsStatContext,
                             mwcst::StatContext*       queuesStatContext,
                             bslma::Allocator*         allocator)
: d_configProvider_p(configProvider)
, d_blobBufferFactory_p(blobBufferFactory)
, d_domainResolver_mp()
, d_clusterCatalog_p(clusterCatalog)
, d_domainsStatContext_p(domainsStatContext)
, d_queuesStatContext_p(queuesStatContext)
, d_dispatcher_p(dispatcher)
, d_domains(allocator)
, d_isStarted(false)
, d_allocator_p(allocator)
{
    // NOTHING
}

DomainManager::~DomainManager()
{
    BSLS_ASSERT_SAFE(!d_isStarted &&
                     "'stop()' must be called before the destructor");
}

int DomainManager::start(bsl::ostream& errorDescription)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS         = 0,
        rc_DOMAIN_RESOLVER = -1
    };

    BALL_LOG_INFO << "Starting DomainManager";

    int ret = rc_SUCCESS;

    // Create the DomainResolver
    d_domainResolver_mp.load(new (*d_allocator_p)
                                 DomainResolver(d_allocator_p),
                             d_allocator_p);
    ret = d_domainResolver_mp->start(errorDescription);
    if (ret != 0) {
        return (ret * 10) + rc_DOMAIN_RESOLVER;  // RETURN
    }
    d_isStarted = true;

    return rc_SUCCESS;
}

void DomainManager::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // mutex LOCKED

    BALL_LOG_INFO << "Stopping " << d_domains.size() << " domains";

    if (d_domains.empty()) {
        return;  // RETURN
    }

    // Stop all domains (and queues).
    mwcu::SharedResource<DomainManager> self(this);
    bslmt::Latch latch(d_domains.size(), bsls::SystemClockType::e_MONOTONIC);

    for (DomainSpMap::iterator it = d_domains.begin(); it != d_domains.end();
         ++it) {
        it->second->teardown(bdlf::BindUtil::bind(
            mwcu::WeakMemFnUtil::weakMemFn(&DomainManager::onDomainClosed,
                                           self.acquireWeak()),
            bdlf::PlaceHolders::_1,  // Domain Name
            &latch));
    }

    bsls::TimeInterval timeout = mwcsys::Time::nowMonotonicClock().addSeconds(
        k_MAX_WAIT_SECONDS_AT_SHUTDOWN);
    int rc = latch.timedWait(timeout);
    if (0 != rc) {
        BALL_LOG_ERROR << "#DOMAINMANAGER_STOP_FAILURE "
                       << "DomainManager failed to stop in "
                       << k_MAX_WAIT_SECONDS_AT_SHUTDOWN
                       << " seconds while shutting down"
                       << " bmqbrkr. rc:  " << rc << ".";

        // Note that 'self' variable will get invalidated when this function
        // returns, which will ensure that any pending 'onDomainClosed'
        // callbacks are not invoked.  So there is no need to explicitly call
        // 'self.invalidate()' here.
    }

    if (d_domainResolver_mp) {
        d_domainResolver_mp->stop();
        d_domainResolver_mp.clear();
    }
}

int DomainManager::locateDomain(DomainSp*          domain,
                                const bsl::string& domainName)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS          = 0,
        rc_DOMAIN_NOT_FOUND = -1
    };

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // mutex LOCKED

    DomainSpMap::const_iterator it = d_domains.find(domainName);
    if (it == d_domains.end()) {
        return rc_DOMAIN_NOT_FOUND;  // RETURN
    }

    *domain = it->second;
    return rc_SUCCESS;
}

int DomainManager::locateOrCreateDomain(DomainSp*          domain,
                                        const bsl::string& domainName)
{
    if (0 != locateDomain(domain, domainName)) {
        BALL_LOG_WARN << "Domain '" << domainName << "' is not opened,"
                      << " trying to initialize from configuration";

        bslmt::Latch latch(1);
        createDomain(domainName,
                     bdlf::BindUtil::bind(&onDomain,
                                          bdlf::PlaceHolders::_1,  // status
                                          bdlf::PlaceHolders::_2,  // domain*
                                          &latch));
        // To return a result from command execution, we need to
        // synchronize with the domain creation attempt
        latch.wait();

        if (0 != locateDomain(domain, domainName)) {
            return -1;  // RETURN
        }
    }

    return 0;  // RETURN
}

int DomainManager::locateOrCreateDomain(DomainSp*          domain,
                                        const bsl::string& domainName)
{
    if (0 != locateDomain(domain, domainName)) {
        BALL_LOG_WARN
            << "Domain '" << domainName
            << "' is not opened, trying to initialize from configuration";

        bslmt::Latch latch(1);
        createDomain(domainName,
                     bdlf::BindUtil::bind(&onDomain,
                                          bdlf::PlaceHolders::_1,  // status
                                          bdlf::PlaceHolders::_2,  // domain*
                                          &latch));

        // To return a result from command execution, we need to
        // synchronize with the domain creation attempt
        latch.wait();

        if (0 != locateDomain(domain, domainName)) {
            return -1;  // RETURN
        }
    }

    return 0;  // RETURN
}

int DomainManager::processCommand(mqbcmd::DomainsResult*        result,
                                  const mqbcmd::DomainsCommand& command)
{
    if (command.isResolverValue()) {
        mqbcmd::Error error;
        int rc = d_domainResolver_mp->processCommand(command.resolver(),
                                                     &error);
        if (0 == rc) {
            result->makeSuccess();
        }
        else {
            result->makeError(error);
        }

        return rc;  // RETURN
    }
    else if (command.isDomainValue()) {
        const bsl::string& name = command.domain().name();

        DomainSp domainSp;

        if (0 != locateOrCreateDomain(&domainSp, name)) {
            mwcu::MemOutStream os;
            os << "Domain '" << name << "' doesn't exist";
            result->makeError().message() = os.str();
            return -1;  // RETURN
        }

        mqbcmd::DomainResult domainResult;
        const int            rc = domainSp->processCommand(&domainResult,
                                                command.domain().command());

        if (domainResult.isErrorValue()) {
            result->makeError(domainResult.error());
            return rc;  // RETURN
        }
        else if (domainResult.isSuccessValue()) {
            result->makeSuccess(domainResult.success());
            return rc;  // RETURN
        }
        result->makeDomainResult(domainResult);
        return rc;  // RETURN
    }
    else if (command.isReconfigureValue()) {
        const bsl::string& name = command.reconfigure().domain();

        DomainSp domainSp;

        if (0 != locateOrCreateDomain(&domainSp, name)) {
            mwcu::MemOutStream os;
            os << "Domain '" << name << "' doesn't exist";
            result->makeError().message() = os.str();
            return -1;  // RETURN
        }

        DecodeAndUpsertValue configureResult;
        d_configProvider_p->clearCache(domainSp->name());
        d_configProvider_p->getDomainConfig(
            domainSp->name(),
            bdlf::BindUtil::bind(
                &DomainManager::decodeAndUpsert,
                this,
                &configureResult,
                bdlf::PlaceHolders::_1,  // configProviderStatus
                bdlf::PlaceHolders::_2,  // configProviderResult
                domainSp->name(),
                domainSp->cluster()->name()));

        if (configureResult.isError()) {
            result->makeError().message() = configureResult.error().d_details;
            return -1;  // RETURN
        }
        else {
            result->makeSuccess();
            return 0;  // RETURN
        }
    }

    mwcu::MemOutStream os;
    os << "Unknown command '" << command << "'";
    result->makeError().message() = os.str();
    return -1;
}

void DomainManager::qualifyDomain(
    const bslstl::StringRef&                      name,
    const mqbi::DomainFactory::QualifiedDomainCb& callback)
{
    d_domainResolver_mp->qualifyDomain(name, callback);
}

void DomainManager::createDomain(
    const bsl::string&                         name,
    const mqbi::DomainFactory::CreateDomainCb& callback)
{
    // executed by *ANY* thread

    if (!d_isStarted) {
        BALL_LOG_INFO << "Not creating domain [" << name << "] at this time "
                      << "because self is stopping.";
        bmqp_ctrlmsg::Status status;
        status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()     = mqbi::ClusterErrorCode::e_STOPPING;  // Retryable
        status.message()  = "Self node is stopping.";
        callback(status, 0);
        return;  // RETURN
    }

    d_domainResolver_mp->locateDomain(
        name,
        bdlf::BindUtil::bind(&DomainManager::onDomainResolverDomainLocatedCb,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // result
                             bsl::string(name),
                             callback));
}

mqbi::Domain* DomainManager::getDomain(const bsl::string& name) const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // mutex LOCKED
    DomainSpMap::const_iterator    it = d_domains.find(name);
    return it == d_domains.end() ? 0 : it->second.get();
}

}  // close package namespace
}  // close enterprise namespace
