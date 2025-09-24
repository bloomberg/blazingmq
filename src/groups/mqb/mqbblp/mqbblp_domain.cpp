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

// mqbblp_domain.cpp                                                  -*-C++-*-
#include <bsls_nullptr.h>
#include <mqbblp_domain.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_rootqueueengine.h>
#include <mqbc_storageutil.h>
#include <mqbcmd_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>
#include <bmqp_routingconfigurationutils.h>

#include <bmqtsk_alarmlog.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BDE
#include <baljsn_encoder.h>
#include <baljsn_encoderoptions.h>
#include <bdlb_string.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_map.h>
#include <bslma_allocator.h>
#include <bslmt_lockguard.h>
#include <bslmt_semaphore.h>

namespace BloombergLP {
namespace mqbblp {

namespace {
const char k_LOG_CATEGORY[] = "MQBBLP.DOMAIN";

const char k_NODE_IS_STOPPING[]              = "Node is stopping";
const char k_DOMAIN_IS_REMOVING_OR_REMOVED[] = "Domain is removing or removed";

/// This method does nothing.. it's just used so that we can control the
/// destruction of the specified `queue` to happen once we guarantee the
/// associated Dispatcher's queue has been drained and flushed.
void queueHolderDummy(const bsl::shared_ptr<mqbi::Queue>& queue)
{
    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    BALL_LOG_INFO << "Deleted queue '" << queue->uri().canonical() << "'";
}

/// Validates an application subscription.
bool validateSubscriptionExpression(bsl::ostream& errorDescription,
                                    const mqbconfm::Expression& expression,
                                    bslma::Allocator*           allocator)
{
    if (mqbconfm::ExpressionVersion::E_VERSION_1 == expression.version()) {
        if (!expression.text().empty()) {
            bmqeval::CompilationContext context(allocator);

            if (!bmqeval::SimpleEvaluator::validate(expression.text(),
                                                    context)) {
                errorDescription
                    << "Expression validation failed: [ expression: "
                    << expression << ", rc: " << context.lastError()
                    << ", reason: \"" << context.lastErrorMessage() << "\" ]";
                return false;  // RETURN
            }
        }
    }
    else {
        errorDescription << "Unsupported version: [ expression: " << expression
                         << " ]";
        return false;  // RETURN
    }

    return true;
}

/// Validates a domain configuration. If `previousDefn` is provided, also
/// checks that the implied reconfiguration is also valid.
int validateConfig(bsl::ostream& errorDescription,
                   const bdlb::NullableValue<mqbconfm::Domain>& previousDefn,
                   const mqbconfm::Domain&                      newConfig,
                   bslma::Allocator*                            allocator)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS               = 0,
        rc_WRONG_WATERMARK_RATIO = -1,
        rc_CHANGED_DOMAIN_MODE   = -2,
        rc_CHANGED_STORAGE_TYPE  = -3,
        rc_INVALID_SUBSCRIPTION  = -4
    };

    // 1. Validate new configuration only

    // Check watermark ratios
    {
        bool failed = false;

        const mqbconfm::Limits& limits = newConfig.storage().domainLimits();

        if (!(0 <= limits.messagesWatermarkRatio() &&
              limits.messagesWatermarkRatio() <= 1.0)) {
            errorDescription
                << "'messagesWatermarkRatio' must be in range [0; 1], got: "
                << limits.messagesWatermarkRatio();
            failed = true;
        }
        if (!(0 <= limits.bytesWatermarkRatio() &&
              limits.bytesWatermarkRatio() <= 1.0)) {
            if (failed)
                errorDescription << bsl::endl;
            errorDescription
                << "'bytesWatermarkRatio' must be in range [0; 1], got: "
                << limits.bytesWatermarkRatio();
            failed = true;
        }

        if (failed) {
            return rc_WRONG_WATERMARK_RATIO;  // RETURN
        }
    }

    // Validate newConfig.subscriptions()
    bsl::size_t numSubscriptions         = newConfig.subscriptions().size();
    bool        allSubscriptionsAreValid = true;

    for (bsl::size_t i = 0; i < numSubscriptions; ++i) {
        if (!validateSubscriptionExpression(
                errorDescription,
                newConfig.subscriptions()[i].expression(),
                allocator)) {
            allSubscriptionsAreValid = false;
        }
    }

    if (!allSubscriptionsAreValid) {
        return rc_INVALID_SUBSCRIPTION;  // RETURN
    }

    // 2. Check compatibility between old/new configurations,
    // if old configuration exists
    if (previousDefn.isNull()) {
        // First time configure, nothing more to validate
        return rc_SUCCESS;  // RETURN
    }

    // Validate properties of new configurations relative to old ones.
    const mqbconfm::Domain& previousCfg = previousDefn.value();

    // Reconfiguring the routing mode is not allowed.
    if (previousCfg.mode().selectionId() != newConfig.mode().selectionId()) {
        errorDescription << "Reconfiguration of domain routing mode is not "
                            "allowed (was '"
                         << previousCfg.mode() << "', changed to '"
                         << newConfig.mode() << "')";
        return rc_CHANGED_DOMAIN_MODE;  // RETURN
    }

    // Reconfiguring the storage mode is not allowed.
    if (previousCfg.storage().config().selectionId() !=
        newConfig.storage().config().selectionId()) {
        errorDescription << "Reconfiguration of storage type is not allowed "
                            "(was '"
                         << previousCfg.storage().config() << "', changed to '"
                         << newConfig.storage().config() << ")";
        return rc_CHANGED_STORAGE_TYPE;  // RETURN
    }

    return rc_SUCCESS;
}

/// Given a definition `defn` for `domain`, ensures that the values provided
/// by `defn` are suitable and consistent for configuring `domain`. For any
/// issues detected, a value within `defn` will be modified and a suitable
/// error-message will be written to `errorDescription`. Returns the number
/// of updates made to `defn`: a return value of zero indicates that `defn`
/// was not modified.
int normalizeConfig(mqbconfm::Domain* defn,
                    bsl::ostream&     errorDescription,
                    const Domain&     domain)
{
    int updatedValues = 0;

    if (defn->mode().isBroadcastValue() &&
        defn->consistency().selectionId() ==
            mqbconfm::Consistency::SELECTION_ID_STRONG) {
        errorDescription << domain.cluster()->name() << ", " << domain.name()
                         << ": A broadcast domain cannot be of strong "
                         << "consistency. Updated this domain's consistency to"
                         << " eventual. Please update this domain's config "
                         << "to not be of strong consistency.\n";

        defn->consistency().makeEventual();
        ++updatedValues;
    }

    return updatedValues;
}

}  // close unnamed namespace

// ------------
// class Domain
// ------------

void Domain::onOpenQueueResponse(
    const bmqp_ctrlmsg::Status&                       status,
    mqbi::QueueHandle*                                queuehandle,
    const bmqp_ctrlmsg::OpenQueueResponse&            openQueueResponse,
    const mqbi::Cluster::OpenQueueConfirmationCookie& confirmationCookie,
    const mqbi::Domain::OpenQueueCallback&            callback)
{
    // executed by *ANY* thread

    --d_pendingRequests;
    if (status.category() == bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        // VALIDATION: The queue must exist at this point, i.e., have been
        //             registered.
        BSLS_ASSERT_SAFE(queuehandle);
        BSLS_ASSERT_SAFE(queuehandle->queue());
        BSLS_ASSERT_SAFE(lookupQueue(0, queuehandle->queue()->uri()) == 0);
    }
    else {
        queuehandle = 0;
    }

    callback(status, queuehandle, openQueueResponse, confirmationCookie);
}

Domain::Domain(const bsl::string&                     name,
               mqbi::Dispatcher*                      dispatcher,
               bdlbb::BlobBufferFactory*              blobBufferFactory,
               const bsl::shared_ptr<mqbi::Cluster>&  cluster,
               bmqst::StatContext*                    domainsStatContext,
               bslma::ManagedPtr<bmqst::StatContext>& queuesStatContext,
               bslma::Allocator*                      allocator)
: d_allocator_p(allocator)
, d_state(e_STOPPED)
, d_name(name, d_allocator_p)
, d_config(d_allocator_p)
, d_cluster_sp(cluster)
, d_dispatcher_p(dispatcher)
, d_blobBufferFactory_p(blobBufferFactory)
, d_domainsStatContext_p(domainsStatContext)
, d_queuesStatContext_mp(queuesStatContext)
, d_capacityMeter(bsl::string("domain [bmq://", d_allocator_p) + d_name + "]",
                  0,
                  d_allocator_p)
, d_queues(d_allocator_p)
, d_pendingRequests(0)
, d_teardownCb()
, d_teardownRemoveCb()
, d_mutex()
{
    if (d_cluster_sp->isRemote()) {
        // In a remote domain, we don't care about monitoring, so disable it
        // for performance efficiency.
        d_capacityMeter.disable();
    }

    // Initialize stats
    d_domainsStats.initialize(this, d_domainsStatContext_p, d_allocator_p);
}

Domain::~Domain()
{
    BSLS_ASSERT_SAFE((e_STOPPING == d_state || e_STOPPED == d_state) &&
                     "'teardown' must be called before the destructor");
}

int Domain::configure(bsl::ostream&           errorDescription,
                      const mqbconfm::Domain& config)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_VALIDATION_FAILED        = -1,
        rc_NOT_IMPLEMENTED          = -2,
        rc_QUEUE_RECONFIGURE_FAILED = -3,
        rc_APPID_RECONFIGURE_FAILED = -4
    };

    // Store a copy of the old configuration.
    bdlb::NullableValue<mqbconfm::Domain> oldConfig(d_config);
    const bool                            isReconfigure(oldConfig.has_value());

    // Certain invalid values might need to be updated in the configuration.
    mqbconfm::Domain finalConfig(config);
    {
        bmqu::MemOutStream err;
        if (normalizeConfig(&finalConfig, err, *this)) {
            BMQTSK_ALARMLOG_ALARM("DOMAIN")
                << err.str() << BMQTSK_ALARMLOG_END;
        }
    }

    // Return early if there are no changes.
    if (oldConfig && (finalConfig == oldConfig.value())) {
        return rc_SUCCESS;  // RETURN
    }

    // No configuration is required outside of the cluster.
    if (d_cluster_sp->isRemote()) {
        d_state = e_STARTED;
        return rc_SUCCESS;  // RETURN
    }

    // Validate config. Return early if the configuration is not valid.
    if (const int rc = validateConfig(errorDescription,
                                      d_config,
                                      finalConfig,
                                      d_allocator_p)) {
        return (rc * 10 + rc_VALIDATION_FAILED);  // RETURN
    }

    // Adopt the updated domain configuration.
    d_config.makeValue(finalConfig);

    // Configure domain limits.
    const mqbconfm::Limits& limits = d_config.value().storage().domainLimits();
    d_capacityMeter.setLimits(limits.messages(), limits.bytes())
        .setWatermarkThresholds(limits.messagesWatermarkRatio(),
                                limits.bytesWatermarkRatio());
    d_domainsStats.onEvent<mqbstat::DomainStats::EventType::e_CFG_MSGS>(
        limits.messages());
    d_domainsStats.onEvent<mqbstat::DomainStats::EventType::e_CFG_BYTES>(
        limits.bytes());

    if (isReconfigure) {
        BSLS_ASSERT_OPT(oldConfig.has_value());
        BSLS_ASSERT_OPT(d_config.has_value());

        // Notify the 'cluster' of the updated configuration, so it can write
        // any needed update-advisories to the CSL.
        d_cluster_sp->onDomainReconfigured(*this,
                                           oldConfig.value(),
                                           d_config.value());

        // Note: Queues must only be reconfigured AFTER ensuring that virtual
        // storage has been created for any new AppIds. This is done by the
        // 'QueueEngine::afterAppIdRegistered' method, which is invoked on the
        // Queue dispatcher thread either:
        //  1) In response to a 'QueueUpdateAdvisory' published to the cluster
        //     by 'onDomainReconfigured' (in CSL mode), or
        //  2) By directly dispatching this method above (non-CSL mode).
        //
        //  Running 'Queue:configure' on the Cluster dispatcher thread ensures
        //  that it happens after 'onDomainReconfigured'; since implementation
        //  of 'Queue::configure' dispatches to the Queue dispatcher thread, it
        //  will also happen after completion of 'afterAppIdRegistered' above.
        BALL_LOG_INFO << "Reconfiguring " << d_queues.size()
                      << " queues from "
                         "domain "
                      << d_name;

        QueueMap::iterator it = d_queues.begin();
        for (; it != d_queues.end(); it++) {
            bsl::function<int()> reconfigureQueueFn = bdlf::BindUtil::bind(
                &mqbi::Queue::configure,
                it->second.get(),
                static_cast<bsl::ostream*>(0),  // errorDescription_p
                true,                           // isReconfigure
                false);                         // wait
            d_dispatcher_p->execute(reconfigureQueueFn, cluster());
        }
    }
    // 'wait==false', so the result of reconfiguration is not known

    d_state = e_STARTED;
    return rc_SUCCESS;
}

void Domain::teardown(const mqbi::Domain::TeardownCb& teardownCb)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state != e_STOPPING);
    BSLS_ASSERT_SAFE(!d_teardownCb);
    BSLS_ASSERT_SAFE(teardownCb);

    // Note that 'd_state' variable is atomic, but it is still accessed after
    // acquiring 'd_mutex'.  This is needed to ensure that the execution of
    // business logic which transitions 'd_state' from e_STOPPING -> e_STOPPED
    // is atomic.  Otherwise, there is a chance due to race that 'teardownCb'
    // can be executed more than once.

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCKED

    BALL_LOG_INFO << "Stopping domain '" << d_name << "' having "
                  << d_queues.size() << " registered queues.";

    d_teardownCb = teardownCb;
    d_state      = e_STOPPING;

    if (d_queues.empty()) {
        d_teardownCb(d_name);
        d_teardownCb = bsl::nullptr_t();
        d_state      = e_STOPPED;
        return;  // RETURN
    }

    for (QueueMap::iterator it = d_queues.begin(); it != d_queues.end();
         ++it) {
        it->second->close();
    }
}

void Domain::teardownRemove(const TeardownCb& teardownCb)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_teardownRemoveCb);
    BSLS_ASSERT_SAFE(teardownCb);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCKED

    BALL_LOG_INFO << "Removing domain '" << d_name << "' having "
                  << d_queues.size() << " registered queues.";

    d_teardownRemoveCb = teardownCb;

    if (d_queues.empty()) {
        d_teardownRemoveCb(d_name);
        d_teardownRemoveCb = bsl::nullptr_t();
        d_state            = e_STOPPED;
        return;  // RETURN
    }

    for (QueueMap::iterator it = d_queues.begin(); it != d_queues.end();
         ++it) {
        it->second->close();
    }
}

void Domain::openQueue(
    const bmqt::Uri&                                          uri,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    const mqbi::Domain::OpenQueueCallback&                    callback)
{
    // will execute in DomainManager's IO requester thread
    // (TBD: for now, in client-session or cluster dispatcher thread)

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(uri.asString() == handleParameters.uri());

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        if (d_state != e_STARTED) {
            // Reject this open-queue request with a soft failure status.

            bmqp_ctrlmsg::Status status;

            if (d_state == e_REMOVING || d_state == e_STOPPED) {
                status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
                status.code()     = mqbi::ClusterErrorCode::e_UNKNOWN;
                status.message()  = k_DOMAIN_IS_REMOVING_OR_REMOVED;
            }
            else {
                status.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
                status.code()     = mqbi::ClusterErrorCode::e_STOPPING;
                status.message()  = k_NODE_IS_STOPPING;
            }

            callback(status,
                     static_cast<mqbi::QueueHandle*>(0),
                     bmqp_ctrlmsg::OpenQueueResponse(),
                     mqbi::Cluster::OpenQueueConfirmationCookie());
            return;  // RETURN
        }

        ++d_pendingRequests;
    }

    d_cluster_sp->openQueue(
        uri,
        this,
        handleParameters,
        clientContext,
        bdlf::BindUtil::bind(&Domain::onOpenQueueResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // queue
                             bdlf::PlaceHolders::_3,  // openQueueResponse
                             bdlf::PlaceHolders::_4,  // confirmationCookie
                             callback));
}

int Domain::registerQueue(const bsl::shared_ptr<mqbi::Queue>& queueSp)
{
    // executed by the associated CLUSTER's DISPATCHER thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_sp->dispatcher()->inDispatcherThread(d_cluster_sp.get()));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS            = 0,
        rc_ALREADY_REGISTERED = -1
    };

    // As part of registering the 'queue' with the domain, 'queue' will also be
    // configured.  But 'Queue.configure' could be expensive, and more
    // importantly, synchronizing on the queue-dispatcher thread.  But
    // queue-dispatcher thread could invoke 'Domain::lookupQueue', which
    // attempts to acquire 'd_lock' as well.  Thus, in order to avoid deadlock
    // b/w this (cluster-dispatcher) thread and queue-dispatcher thread, we
    // invoke 'Queue.configure' outside of the lock scope, and in case it
    // fails, we rollback.

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        // PRECONDITIONS: The queue must NOT have been registered at this point
        QueueMap::const_iterator it = d_queues.find(queueSp->uri().queue());
        if (it != d_queues.end()) {
            BALL_LOG_ERROR
                << "#DOMAIN_QUEUE_REGISTRATION_FAILURE "
                << "A queue has already been registered with the domain '"
                << d_name << "' [queue: '" << queueSp->uri().queue() << "']";
            BSLS_ASSERT_SAFE(false && "Queue already registered with domain");
            return rc_ALREADY_REGISTERED;  // RETURN
        }

        // Optimistically add queue to this domain's catalog of queues
        // (assuming Queue.configure() will succeed).

        d_queues[queueSp->uri().queue()] = queueSp;
    }

    BALL_LOG_INFO << "Registered queue to domain '" << d_name << "' "
                  << "[canonicalURI: " << queueSp->uri().canonical()
                  << ", qId: " << bmqp::QueueId::QueueIdInt(queueSp->id())
                  << "]. Total number of registered queues in the domain: "
                  << d_queues.size() << ".";

    return rc_SUCCESS;
}

void Domain::unregisterQueue(mqbi::Queue* queue)
{
    // executed by the associated CLUSTER's DISPATCHER thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_sp->dispatcher()->inDispatcherThread(d_cluster_sp.get()));

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    QueueMap::const_iterator it = d_queues.find(queue->uri().queue());
    BSLS_ASSERT_SAFE(it != d_queues.end() &&
                     "Queue was not registered with domain");

    if (it == d_queues.end()) {
        BALL_LOG_ERROR << "#DOMAIN_QUEUE_REGISTRATION_FAILURE "
                       << "Unable to unregister queue '"
                       << queue->uri().queue() << "' "
                       << "from domain '" << d_name
                       << "' [reason: queue was not "
                       << "registered]";
        return;  // RETURN
    }

    bsl::shared_ptr<mqbi::Queue> queueSp(it->second);
    d_queues.erase(it);

    // Close the queue before we schedule Dummy event so that we can clear
    // any pending messages before the destruction of the queue
    queueSp->close();

    // We need to make sure the 'queue' is not in its associated dispatcher's
    // thread (flush list or 'expirePendingMessagesDispatched': for that
    // purpose, enqueue an 'e_DISPATCHER' type event with the shared_ptr to the
    // queue; once it gets processed, we have guarantees that the dispatcher's
    // work is done and therefore can then safely delete the queue.
    //
    // This code relies on queue being in an 'empty' state meaning it will NOT
    // schedule any new dispatcher work.
    d_dispatcher_p->execute(bdlf::BindUtil::bind(&queueHolderDummy, queueSp),
                            queueSp.get(),
                            mqbi::DispatcherEventType::e_DISPATCHER);

    BALL_LOG_INFO << "Unregistered queue from domain '" << d_name
                  << "' [canonicalURI: " << queueSp->uri().canonical() << "]. "
                  << "Total number of registered queues in the domain: "
                  << d_queues.size();

    // Refer to note in 'teardown' routine to see why 'd_state' is updated
    // while 'd_mutex' is acquired.
    if (d_queues.empty()) {
        if (d_teardownCb) {
            d_teardownCb(d_name);
            d_teardownCb = bsl::nullptr_t();
            d_state      = e_STOPPED;
        }
        if (d_teardownRemoveCb) {
            d_teardownRemoveCb(d_name);
            d_teardownRemoveCb = bsl::nullptr_t();
            d_state            = e_STOPPED;
        }
    }
}

int Domain::processCommand(mqbcmd::DomainResult*        result,
                           const mqbcmd::DomainCommand& command)
{
    // executed by *any* thread

    if (command.isPurgeValue()) {
        // Some queues might be inactive.  They don't have associated
        // mqbi::Queue objects registered in Domain.  To purge these queues, we
        // need to send purge command to the storage level.
        mqbcmd::ClusterCommand clusterCommand;
        mqbcmd::StorageDomain& domain =
            clusterCommand.makeStorage().makeDomain();
        domain.name() = d_name;
        domain.command().makePurge();

        mqbcmd::ClusterResult clusterResult;
        const int             rc = d_cluster_sp->processCommand(&clusterResult,
                                                    clusterCommand);

        if (clusterResult.isErrorValue()) {
            result->makeError(clusterResult.error());
            return rc;  // RETURN
        }

        BSLS_ASSERT_SAFE(clusterResult.isStorageResultValue());
        BSLS_ASSERT_SAFE(clusterResult.storageResult().isPurgedQueuesValue());

        mqbcmd::PurgedQueues& purgedQueues = result->makePurgedQueues();
        purgedQueues.queues() =
            clusterResult.storageResult().purgedQueues().queues();

        return 0;  // RETURN
    }
    else if (command.isInfoValue()) {
        mqbcmd::DomainInfo& domainInfo = result->makeDomainInfo();

        domainInfo.name()        = d_name;
        domainInfo.clusterName() = d_cluster_sp->name();
        if (!d_config.isNull()) {
            baljsn::Encoder                       encoder;
            bdlma::LocalSequentialAllocator<1024> localAllocator(
                d_allocator_p);
            bmqu::MemOutStream out(&localAllocator);

            baljsn::EncoderOptions options;
            options.setEncodingStyle(baljsn::EncoderOptions::e_PRETTY);
            options.setSpacesPerLevel(2);

            BSLA_MAYBE_UNUSED const int rc = encoder.encode(out,
                                                            d_config.value(),
                                                            options);
            BSLS_ASSERT_SAFE(rc == 0);
            domainInfo.configJson() = out.str();
        }

        mqbcmd::CapacityMeter& capacityMeter = domainInfo.capacityMeter();
        mqbu::CapacityMeterUtil::loadState(&capacityMeter, d_capacityMeter);

        typedef bsl::map<bsl::string, bsl::shared_ptr<mqbi::Queue> >
                                                OrderedQueueMap;
        typedef OrderedQueueMap::const_iterator OrderedQueueMapCIter;

        // sort by queue name
        OrderedQueueMap      map(d_queues.cbegin(), d_queues.cend());
        OrderedQueueMapCIter cit;
        domainInfo.queueUris().reserve(d_queues.size());
        for (cit = map.cbegin(); cit != map.cend(); ++cit) {
            domainInfo.queueUris().push_back(cit->second->uri().asString());
        }

        mqbcmd::ClusterCommand clusterCommand;
        mqbcmd::StorageDomain& domain =
            clusterCommand.makeStorage().makeDomain();
        domain.name() = d_name;
        domain.command().makeQueueStatus();

        mqbcmd::ClusterResult clusterResult;
        const int             rc = d_cluster_sp->processCommand(&clusterResult,
                                                    clusterCommand);
        if (clusterResult.isErrorValue()) {
            result->makeError(clusterResult.error());
            return rc;  // RETURN
        }

        // The clusterResult is guaranteed to be a storage result since we set
        // the clusterCommand above.
        domainInfo.storageContent() =
            clusterResult.storageResult().storageContent();
        return rc;  // RETURN
    }
    else if (command.isQueueValue()) {
        bmqt::UriBuilder uriBuilder(d_allocator_p);
        uriBuilder.setQualifiedDomain(name()).setQueue(command.queue().name());

        bmqt::Uri   uri;
        bsl::string uriError;
        int         rc = uriBuilder.uri(&uri, &uriError);

        if (rc != 0) {
            bmqu::MemOutStream os;
            os << "Unable to build queue uri with error '" << uriError << "'";
            result->makeError().message() = os.str();
            return -1;  // RETURN
        }

        if (command.queue().command().isPurgeAppIdValue()) {
            const bsl::string& purgeAppId =
                command.queue().command().purgeAppId();

            if (purgeAppId.empty()) {
                mqbcmd::Error& error = result->makeError();
                error.message() = "Queue Purge requires a non-empty appId ("
                                  "Specify '*' to purge the entire queue).";
                return -1;  // RETURN
            }

            // Some queues might be inactive.  They don't have associated
            // mqbi::Queue objects registered in Domain.  The only way to purge
            // both active/inactive queues is to execute purge on the storage
            // level.
            mqbcmd::ClusterCommand clusterCommand;
            mqbcmd::StorageQueue&  queue =
                clusterCommand.makeStorage().makeQueue();
            queue.canonicalUri()             = uri.canonical();
            queue.command().makePurgeAppId() = purgeAppId;

            mqbcmd::ClusterResult clusterResult;
            rc = d_cluster_sp->processCommand(&clusterResult, clusterCommand);
            if (clusterResult.isErrorValue()) {
                result->makeError(clusterResult.error());
                return rc;  // RETURN
            }

            BSLS_ASSERT_SAFE(clusterResult.isStorageResultValue());
            BSLS_ASSERT_SAFE(
                clusterResult.storageResult().isPurgedQueuesValue());

            result->makeQueueResult().makePurgedQueues().queues() =
                clusterResult.storageResult().purgedQueues().queues();
            return rc;  // RETURN
        }

        bsl::shared_ptr<mqbi::Queue> queue;
        rc = lookupQueue(&queue, uri);

        if (rc != 0) {
            bmqu::MemOutStream os;
            os << "Queue '" << command.queue().name() << "'"
               << " was not found on domain '" << name() << "'";
            result->makeError().message() = os.str();
            return -1;  // RETURN
        }

        mqbcmd::QueueResult queueResult;
        rc = queue->processCommand(&queueResult, command.queue().command());
        if (queueResult.isErrorValue()) {
            result->makeError(queueResult.error());
            return rc;  // RETURN
        }

        result->makeQueueResult(queueResult);
        return rc;  // RETURN
    }

    bdlma::LocalSequentialAllocator<256> localAllocator(d_allocator_p);
    bmqu::MemOutStream                   os(&localAllocator);
    os << "Unknown command '" << command << "'";
    result->makeError().message() = os.str();
    return -1;
}

// ACCESSORS
int Domain::lookupQueue(bsl::shared_ptr<mqbi::Queue>* out,
                        const bmqt::Uri&              uri) const
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS   = 0,
        rc_NOT_FOUND = -1
    };

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    QueueMap::const_iterator it = d_queues.find(uri.queue());
    if (it == d_queues.end()) {
        return rc_NOT_FOUND;  // RETURN
    }

    if (out) {
        // Some callers may just want to know if the queue exist and don't need
        // it returned
        *out = it->second;
    }

    return rc_SUCCESS;
}

void Domain::loadAllQueues(
    bsl::vector<bsl::shared_ptr<mqbi::Queue> >* out) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    for (QueueMapCIter cit = d_queues.cbegin(); cit != d_queues.cend();
         ++cit) {
        out->push_back(cit->second);
    }
}

void Domain::loadAllQueues(bsl::vector<bmqt::Uri>* out) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    for (QueueMapCIter cit = d_queues.cbegin(); cit != d_queues.cend();
         ++cit) {
        out->push_back(cit->second->uri());
    }
}

void Domain::loadRoutingConfiguration(
    bmqp_ctrlmsg::RoutingConfiguration* config) const
{
    // executed by the associated CLUSTER's DISPATCHER thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_sp->dispatcher()->inDispatcherThread(d_cluster_sp.get()));
    BSLS_ASSERT_SAFE(config);

    bmqp::RoutingConfigurationUtils::clear(config);

    if (d_config.isNull()) {
        BALL_LOG_ERROR << "#DOMAIN_INVALID_CONFIG "
                       << "Uninitialized config for domain '" << d_name
                       << "'.";
        return;  // RETURN
    }

    switch (d_config.value().mode().selectionId()) {
    case mqbconfm::QueueMode::SELECTION_ID_FANOUT: {
        RootQueueEngine::FanoutConfiguration::loadRoutingConfiguration(config);
    } break;
    case mqbconfm::QueueMode::SELECTION_ID_PRIORITY: {
        RootQueueEngine::PriorityConfiguration::loadRoutingConfiguration(
            config);
    } break;
    case mqbconfm::QueueMode::SELECTION_ID_BROADCAST: {
        RootQueueEngine::BroadcastConfiguration::loadRoutingConfiguration(
            config);
    } break;
    case mqbconfm::QueueMode::SELECTION_ID_UNDEFINED:
    default: {
        BSLS_ASSERT_SAFE(false && "Invalid domain routing mode");
        BALL_LOG_ERROR << "#DOMAIN_INVALID_CONFIG "
                       << "Invalid or undefined mode '"
                       << d_config.value().mode() << "' for domain '" << d_name
                       << "'.";
    }
    }
}

bool Domain::tryRemove()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    if (d_state == e_STOPPING) {
        return false;
    }

    if (d_pendingRequests != 0) {
        return false;
    }

    // Reset d_teardownRemoveCb in case the first round of
    // DOMAINS REMOVE fails and we want to call it again
    d_state            = e_REMOVING;
    d_teardownRemoveCb = bsl::nullptr_t();

    return true;
}

bool Domain::isRemoveComplete() const
{
    return d_state == e_STOPPED;
}

}  // close package namespace
}  // close enterprise namespace
