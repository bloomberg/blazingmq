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

// mwcio_reconnectingchannelfactory.cpp                               -*-C++-*-
#include <mwcio_reconnectingchannelfactory.h>

#include <mwcscm_version.h>
// MWC
#include <mwcio_resolveutil.h>
#include <mwcsys_time.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <ball_log.h>
#include <bdlb_random.h>
#include <bdlb_randomdevice.h>
#include <bdlb_stringrefutil.h>
#include <bdlb_tokenizer.h>
#include <bdlf_bind.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_algorithm.h>
#include <bsl_cstdlib.h>
#include <bsl_limits.h>
#include <bsls_libraryfeatures.h>

// NTC
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_BASELINE_LIBRARY
#include <bsl_random.h>
#endif

namespace BloombergLP {
namespace mwcio {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MWCIO.RECONNECTINGCHANNELFACTORY");

}  // close unnamed namespace

// --------------------------------------
// class ReconnectingChannelFactoryConfig
// --------------------------------------

ReconnectingChannelFactoryConfig::ReconnectingChannelFactoryConfig(
    ChannelFactory*        base,
    bdlmt::EventScheduler* scheduler,
    bslma::Allocator*      basicAllocator)
: d_base_p(base)
, d_scheduler_p(scheduler)
, d_reconnectIntervalFn(
      bsl::allocator_arg,
      basicAllocator,
      bdlf::BindUtil::bind(
          &ReconnectingChannelFactoryUtil::defaultConnectIntervalFn,
          bdlf::PlaceHolders::_1,
          bdlf::PlaceHolders::_2,
          bdlf::PlaceHolders::_3,
          bsls::TimeInterval(120.0),
          bsls::TimeInterval(60.0)))
, d_endpointResolveFn(
      bsl::allocator_arg,
      basicAllocator,
      bdlf::BindUtil::bind(&ReconnectingChannelFactoryUtil::resolveEndpointFn,
                           bdlf::PlaceHolders::_1,
                           bdlf::PlaceHolders::_2,
                           ';'))
{
    // NOTHING
}

ReconnectingChannelFactoryConfig::ReconnectingChannelFactoryConfig(
    const ReconnectingChannelFactoryConfig& original,
    bslma::Allocator*                       basicAllocator)
: d_base_p(original.d_base_p)
, d_scheduler_p(original.d_scheduler_p)
, d_reconnectIntervalFn(bsl::allocator_arg,
                        basicAllocator,
                        original.d_reconnectIntervalFn)
, d_endpointResolveFn(bsl::allocator_arg,
                      basicAllocator,
                      original.d_endpointResolveFn)
{
    // NOTHING
}

ReconnectingChannelFactoryConfig&
ReconnectingChannelFactoryConfig::setReconnectIntervalFn(
    const ReconnectIntervalFn& value)
{
    d_reconnectIntervalFn = value;
    return *this;
}

ReconnectingChannelFactoryConfig&
ReconnectingChannelFactoryConfig::setEndpointResolveFn(
    const EndpointResolveFn& value)
{
    d_endpointResolveFn = value;
    return *this;
}

// -----------------------------------------------
// struct ReconnectingChannelFactory_ConnectHandle
// -----------------------------------------------

ReconnectingChannelFactory_ConnectHandle::
    ReconnectingChannelFactory_ConnectHandle(bslma::Allocator* basicAllocator)
: d_validator()
, d_canceled(false)
, d_factory_p(0)
, d_resultCallback(bsl::allocator_arg, basicAllocator)
, d_options(basicAllocator)
, d_endpoints(basicAllocator)
, d_currentAttemptCounter(0)
, d_baseConnectHandle()
, d_closeFnConnection()
, d_reconnectHandle()
, d_lastConnectAttemptTime(-1)
, d_lastConnectInterval()
{
    // NOTHING
}

ReconnectingChannelFactory_ConnectHandle::
    ~ReconnectingChannelFactory_ConnectHandle()
{
    // NOTHING
}

// MANIPULATORS
void ReconnectingChannelFactory_ConnectHandle::cancel()
{
    if (d_canceled.testAndSwap(false, true) == true) {
        // Seamlessly handle multiple cancel
        return;  // RETURN
    }

    d_validator.invalidate();

    if (d_reconnectHandle) {
        d_factory_p->d_config.d_scheduler_p->cancelEventAndWait(
            &d_reconnectHandle);
    }

    d_factory_p->removeConnectHandle(this);

    if (d_baseConnectHandle) {
        d_baseConnectHandle->cancel();
    }
}

mwct::PropertyBag& ReconnectingChannelFactory_ConnectHandle::properties()
{
    return d_baseConnectHandle->properties();
}

const mwct::PropertyBag&
ReconnectingChannelFactory_ConnectHandle::properties() const
{
    return d_baseConnectHandle->properties();
}

// --------------------------------
// class ReconnectingChannelFactory
// --------------------------------

// PRIVATE MANIPULATORS
void ReconnectingChannelFactory::scheduleConnect(
    const ConnectHandlePtr& handle)
{
    if (!handle->d_endpoints.empty()) {
        // If we haven't yet exhausted all endpoints, retry immediately without
        // waiting.
        bdlma::LocalSequentialAllocator<512> arena;

        Status status(&arena);
        doConnect(&status, handle);
        if (!status) {
            // If we are here, this means the initial connect call succeeded,
            // therefore this failure is unexpected, so log it.  Also, keep
            // retrying because this implies failure was due to external
            // changes, which might eventually get resolved.
            BALL_LOG_ERROR << "Unexpected internal connection attempt failure"
                           << " with '" << handle->d_options.endpoint()
                           << "': " << status;

            // 'simulate' a CONNECT_FAILED from the base factory so that the
            // default reconnect logic scheduling will kick in.
            connectResultCb(handle,
                            ChannelFactoryEvent::e_CONNECT_FAILED,
                            status,
                            bsl::shared_ptr<Channel>());
        }

        return;  // RETURN
    }

    bsls::TimeInterval timeSinceLastConnect;
    if (handle->d_lastConnectAttemptTime == -1) {
        // We never successfully connect, initialize time to the max possible.
        timeSinceLastConnect.setTotalNanoseconds(
            bsl::numeric_limits<bsls::Types::Int64>::max());
    }
    else {
        timeSinceLastConnect.setTotalNanoseconds(
            mwcsys::Time::highResolutionTimer() -
            handle->d_lastConnectAttemptTime);
    }

    // Query for how long to wait before the new connect attempt
    d_config.d_reconnectIntervalFn(&handle->d_lastConnectInterval,
                                   handle->d_options,
                                   timeSinceLastConnect);

    BALL_LOG_TRACE << "Scheduling a connect with '"
                   << handle->d_options.endpoint() << "' in "
                   << handle->d_lastConnectInterval;

    d_config.d_scheduler_p->scheduleEvent(
        &handle->d_reconnectHandle,
        d_config.d_scheduler_p->now() + handle->d_lastConnectInterval,
        bdlf::BindUtil::bind(&ReconnectingChannelFactory::connectTimerCb,
                             this,
                             handle));
}

void ReconnectingChannelFactory::doConnect(Status*                 status,
                                           const ConnectHandlePtr& handle)
{
    handle->d_lastConnectAttemptTime = mwcsys::Time::highResolutionTimer();

    if (handle->d_endpoints.empty()) {
        // We haven't yet resolved endpoints, or we tried them all already.

        d_config.d_endpointResolveFn(&handle->d_endpoints, handle->d_options);
        // Note that this method will be executed by either the scheduler
        // or the IO thread.  The resolver could potentially be 'slow'
        // therefore we may eventually need to offload it to a dedicated
        // thread.

        if (handle->d_endpoints.size() > 1) {
            // Logging the hosts we resolved to is important, however, we
            // resolve between each attempt (and the results are expected to
            // remain stable), therefore only log when the resolved list has
            // more than one host - the expectation being that when resolving
            // to multiple hosts, at least one should succeed at first attempt.
            BALL_LOG_INFO << "Resolved endpoint '"
                          << handle->d_options.endpoint() << "' to: "
                          << mwcu::PrintUtil::printer(handle->d_endpoints);
        }

        bsl::reverse(handle->d_endpoints.begin(), handle->d_endpoints.end());
        // Reverse the resolved hosts because we will 'pop' them one by one
        // as they are being tried (and on a vector, it's more efficient to
        // pop_back), therefore reverse it in order ot keep the returning
        // order of hosts to try.

        ++handle->d_currentAttemptCounter;
    }

    if (handle->d_endpoints.empty()) {
        // If the endpoints is empty at this point, this implies that the
        // 'endpointResolverFn' returned an empty list; but they may resolve
        // successfully in the next ones, therefore immediately 'simulate' a
        // CONNECT_FAILED from the base factory so that the default reconnect
        // logic scheduling will kick in.
        bdlma::LocalSequentialAllocator<512> arena;
        Status                               localStatus(&arena);
        localStatus.reset(StatusCategory::e_GENERIC_ERROR,
                          "ReconnectingChannelFactory::endpointResolution",
                          "Empty resolved hosts list");
        connectResultCb(handle,
                        ChannelFactoryEvent::e_CONNECT_FAILED,
                        localStatus,
                        bsl::shared_ptr<Channel>());

        if (status) {
            // But indicate 'success' to the caller (the reconnecting logic
            // will be kicked in from the above 'connectResultCb' call).
            status->reset(StatusCategory::e_SUCCESS);
        }

        return;  // RETURN
    }

    // Time to initiate the connect to the next host in the list
    ConnectOptions options(handle->d_options);
    options.setEndpoint(handle->d_endpoints.back())
        .setAutoReconnect(false)
        .setNumAttempts(1);
    // In order to implement the failover endpoints, override 'numAttempts'
    // to 1; this factory takes care of the 'real' numAttempts as requested
    // by the user.

    handle->d_endpoints.pop_back();

    BALL_LOG_TRACE << "Connecting to '" << options.endpoint()
                   << "', which was resolved from '"
                   << handle->d_options.endpoint() << "'";

    d_config.d_base_p->connect(
        status,
        &handle->d_baseConnectHandle,
        options,
        bdlf::BindUtil::bind(&ReconnectingChannelFactory::connectResultCb,
                             this,
                             handle,
                             bdlf::PlaceHolders::_1,
                             bdlf::PlaceHolders::_2,
                             bdlf::PlaceHolders::_3));
}

void ReconnectingChannelFactory::connectTimerCb(const ConnectHandlePtr& handle)
{
    mwcu::AtomicValidatorGuard guard(&handle->d_validator);
    if (!guard.isValid()) {
        return;  // RETURN
    }

    handle->d_reconnectHandle.release();

    bdlma::LocalSequentialAllocator<512> arena;
    Status                               status(&arena);
    doConnect(&status, handle);

    if (!status) {
        // 'simulate' a CONNECT_FAILED from the base factory so that the
        // default reconnect logic scheduling will kick in.
        connectResultCb(handle,
                        ChannelFactoryEvent::e_CONNECT_FAILED,
                        status,
                        bsl::shared_ptr<Channel>());
    }
}

void ReconnectingChannelFactory::connectResultCb(
    const ConnectHandlePtr&         handle,
    ChannelFactoryEvent::Enum       event,
    const Status&                   status,
    const bsl::shared_ptr<Channel>& channel)
{
    mwcu::AtomicValidatorGuard guard(&handle->d_validator);

    if (!guard.isValid()) {
        return;  // RETURN
    }

    switch (event) {
    case ChannelFactoryEvent::e_CHANNEL_UP: {
        handle->d_baseConnectHandle.clear();

        if (handle->d_options.autoReconnect()) {
            // Monitor this channel, so we can schedule a reconnect when it
            // gets disconnected.
            handle->d_closeFnConnection = channel->onClose(
                bdlf::BindUtil::bind(
                    &ReconnectingChannelFactory::onChannelDown,
                    this,
                    handle));
        }

        handle->d_endpoints.clear();
        // We successfully connected, so clear out the resolved endpoints
        // list, which will force trigger a new resolution at next connect,
        // restarting from the beginning of the full list.

        handle->d_currentAttemptCounter = 0;
        handle->d_resultCallback(event, status, channel);

        if (!handle->d_options.autoReconnect()) {
            // The handle is not for an auto-reconnecting connecting, therefore
            // forget about it now that it's been successfully connected.
            removeConnectHandle(handle.get());
        }
    } break;  // BREAK
    case ChannelFactoryEvent::e_CONNECT_FAILED: {
        // Schedule another connect if more attempts left
        handle->d_baseConnectHandle.clear();

        if (handle->d_currentAttemptCounter ==
                handle->d_options.numAttempts() &&
            handle->d_endpoints.empty()) {
            BALL_LOG_WARN << "Reached maximum number of connection attempts ("
                          << handle->d_options.numAttempts() << ") with"
                          << handle->d_options.endpoint();
            handle->d_resultCallback(event, status, channel);
            removeConnectHandle(handle.get());
            return;  // RETURN
        }

        if (handle->d_endpoints.empty()) {
            // We exhausted all endpoints from the resolved list, time to
            // notify the caller of a connection attempt failure.
            handle->d_resultCallback(
                ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED,
                status,
                channel);
        }

        // Schedule the next connection attempt
        scheduleConnect(handle);
    } break;  //  BREAK
    case ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED: {
        // Since this factory always overrides the numAttempt to 1, we should
        // never get such event.
        BSLS_ASSERT_SAFE(false && "Unexpected 'CONNECT_ATTEMPT_FAILED' event");
        BALL_LOG_ERROR << "Unexpected 'CONNECT_ATTEMPT_FAILED' event.";
    } break;  // BREAK
    default: {
        handle->d_resultCallback(event, status, channel);
    } break;
    }
}

void ReconnectingChannelFactory::onChannelDown(const ConnectHandlePtr& handle)
{
    mwcu::AtomicValidatorGuard guard(&handle->d_validator);
    if (!guard.isValid()) {
        return;  // RETURN
    }

    handle->d_closeFnConnection.reset();

    // We only monitor autoReconnect channel for the down event
    BSLS_ASSERT_SAFE(handle->d_options.autoReconnect());

    scheduleConnect(handle);
}

void ReconnectingChannelFactory::removeConnectHandle(ConnectHandle* handle)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    handle->d_resultCallback = ChannelFactory::ResultCallback();
    // Clear the result callback to free up resources (and avoid circular
    // dependency depending on what state would be binded into the
    // callback by the user).

    d_handles.erase(handle);
}

ReconnectingChannelFactory::ReconnectingChannelFactory(
    const Config&     config,
    bslma::Allocator* basicAllocator)
: d_validator(false)
, d_config(config, basicAllocator)
, d_handles(basicAllocator)
, d_mutex()
{
    // NOTHING
}

ReconnectingChannelFactory::~ReconnectingChannelFactory()
{
    // NOTHING
}

int ReconnectingChannelFactory::start()
{
    d_validator.reset();  // Support multiple 'start'
    return 0;
}

void ReconnectingChannelFactory::stop()
{
    d_validator.invalidate();

    // Canceling a handle involves acquiring the factory's lock, so need to
    // call 'cancel' on the handles outside of the lock. This swap enables
    // doing that in a thread-safe manner.
    ConnectHandleMap handlesCopy(d_handles.get_allocator().mechanism());
    {  //   LOCK
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
        handlesCopy.swap(d_handles);
    }  // UNLOCK

    for (ConnectHandleMap::iterator iter = handlesCopy.begin();
         iter != handlesCopy.end();
         ++iter) {
        iter->second->cancel();
    }
}

void ReconnectingChannelFactory::listen(Status*                      status,
                                        bslma::ManagedPtr<OpHandle>* handle,
                                        const ListenOptions&         options,
                                        const ResultCallback&        cb)
{
    mwcu::AtomicValidatorGuard valGuard(&d_validator);
    if (!valGuard.isValid()) {
        if (status) {
            status->reset(StatusCategory::e_CANCELED);
        }

        return;  // RETURN
    }

    d_config.d_base_p->listen(status, handle, options, cb);
}

void ReconnectingChannelFactory::connect(Status*                      status,
                                         bslma::ManagedPtr<OpHandle>* handle,
                                         const ConnectOptions&        options,
                                         const ResultCallback&        cb)
{
    mwcu::AtomicValidatorGuard valGuard(&d_validator);
    if (!valGuard.isValid()) {
        if (status) {
            status->reset(StatusCategory::e_CANCELED);
        }

        return;  // RETURN
    }

    bslma::Allocator* alloc = d_handles.get_allocator().mechanism();

    ConnectHandlePtr connHandle;
    connHandle.createInplace(alloc, alloc);
    connHandle->d_factory_p      = this;
    connHandle->d_resultCallback = cb;
    connHandle->d_options        = options;

    {  //   LOCK
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
        d_handles[connHandle.get()] = connHandle;
    }  // UNLOCK

    if (handle) {
        bslma::ManagedPtr<ConnectHandle> handleMp(connHandle.managedPtr());
        handle->loadAlias(handleMp, connHandle.get());
    }

    bdlma::LocalSequentialAllocator<512> arena;
    Status                               dummyStatus(&arena);
    if (!status) {
        status = &dummyStatus;
    }

    doConnect(status, connHandle);

    if (!*status) {
        removeConnectHandle(connHandle.get());
        if (handle) {
            handle->reset();
        }
    }
}

// -------------------------------------
// struct ReconnectingChannelFactoryUtil
// -------------------------------------

void ReconnectingChannelFactoryUtil::resolveEndpointFn(
    bsl::vector<bsl::string>* out,
    const ConnectOptions&     options,
    char                      separator)
{
    const char k_HOSTPORT_SEPARATOR = ':';

    // Clear 'out'
    out->clear();

    bdlma::LocalSequentialAllocator<1024> arena;

    bdlb::Tokenizer endpoints(options.endpoint().c_str(),
                              bslstl::StringRef(&separator, 1));
    for (bdlb::TokenizerIterator endpointsIter = endpoints.begin();
         endpointsIter != endpoints.end();
         ++endpointsIter) {
        const size_t colonOffset = bdlb::StringRefUtil::findFirstOf(
            *endpointsIter,
            bslstl::StringRef(&k_HOSTPORT_SEPARATOR, 1));
        if (colonOffset == bdlb::StringRefUtil::k_NPOS) {
            BALL_LOG_ERROR << "Invalid endpoint format '" << *endpointsIter
                           << "' (missing ':port' part)";
            continue;  // CONTINUE
        }
        const bsl::string host(endpointsIter->begin(), colonOffset, &arena);
        const bsl::string port(bdlb::StringRefUtil::substr(*endpointsIter,
                                                           colonOffset + 1),
                               &arena);

        bsl::vector<ntsa::IpAddress> addresses(&arena);
        ntsa::Error error = mwcio::ResolveUtil::getIpAddress(&addresses, host);
        if (error.code() != ntsa::Error::e_OK) {
            BALL_LOG_ERROR << "Unable to resolve addresses from host '" << host
                           << "' (error: " << error << ")";
            continue;  // CONTINUE
        }

        for (bsl::vector<ntsa::IpAddress>::const_iterator it =
                 addresses.begin();
             it != addresses.end();
             ++it) {
            bsl::string hostname(&arena);
            error = mwcio::ResolveUtil::getDomainName(&hostname, *it);
            if (error.code() != ntsa::Error::e_OK) {
                // If we fail to resolve, log an error and skip this entry
                BALL_LOG_ERROR << "Unable to resolve hostname of address "
                               << *it << " from host '" << host
                               << "' (error: " << error << ")";
                continue;  // CONTINUE
            }

            mwcu::MemOutStream os(&arena);
            os << hostname << ":" << port;
            out->push_back(os.str());
        }
    }

    if (out->empty()) {
        BALL_LOG_ERROR << "Resolving hosts lists from endpoint(s) '"
                       << options.endpoint()
                       << "' yielded an empty list of host.";
        return;  // RETURN
    }

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_BASELINE_LIBRARY
    bsl::random_device rd;
    bsl::mt19937       generator(rd());
    bsl::shuffle(out->begin(), out->end(), generator);
#else
    bsl::random_shuffle(out->begin(), out->end());
#endif
    // After resolving the hosts from the endpoints, we then randomize the
    // vector of hosts, in order to help getting a better load distribution
    // from all the clients using the same list of hosts.
}

void ReconnectingChannelFactoryUtil::defaultConnectIntervalFn(
    bsls::TimeInterval*       interval,
    const ConnectOptions&     options,
    const bsls::TimeInterval& timeSinceLastAttempt,
    const bsls::TimeInterval& resetReconnectTime,
    const bsls::TimeInterval& maxInterval)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(maxInterval < resetReconnectTime);

    if (timeSinceLastAttempt >= resetReconnectTime) {
        // If the last attempt happened a 'while back', then we can assume the
        // connection was successfull, and when it goes down, we should
        // immediately retry, without initial delay (this will especially be
        // usefull for connections to multi-hosts to have a faster failover).
        interval->setTotalNanoseconds(0);
        return;  // RETURN
    }

    if (*interval == bsls::TimeInterval()) {
        // This is the very first connect call, return the initial connect
        // interval with some jitter, from the range
        //   '[interval * 0.5; interval * 1.5]'
        interval->setTotalNanoseconds(
            options.attemptInterval().totalNanoseconds() / 2);
        *interval += (static_cast<double>(bsl::rand()) / RAND_MAX) *
                     options.attemptInterval().totalSecondsAsDouble();
    }
    else {
        *interval += (static_cast<double>(bsl::rand()) / RAND_MAX) *
                     interval->totalSecondsAsDouble();
    }

    if (*interval > maxInterval) {
        *interval = maxInterval;
    }
}

}  // close package namespace
}  // close enterprise namespace
