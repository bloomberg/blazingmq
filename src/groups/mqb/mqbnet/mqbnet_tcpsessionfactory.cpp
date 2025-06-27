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

// mqbnet_tcpsessionfactory.cpp                                       -*-C++-*-
#include <mqbnet_tcpsessionfactory.h>

#include <mqbscm_version.h>
/// Implementation Notes
///====================
/// When a channel is being created, the following methods are always called,
/// in order, regardless of the success or failure of the negotiation:
/// - `channelStateCallback`
/// - `negotiate`
/// - `initialConnectionComplete`
///
/// When a channel goes down, `onClose()` is the only method being invoked.

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbcfg_tcpinterfaceconfigvalidator.h>
#include <mqbnet_cluster.h>
#include <mqbnet_negotiationcontext.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqex_executionutil.h>
#include <bmqex_systemexecutor.h>
#include <bmqio_channelutil.h>
#include <bmqio_connectoptions.h>
#include <bmqio_ntcchannel.h>
#include <bmqio_ntcchannelfactory.h>
#include <bmqio_resolveutil.h>
#include <bmqio_statchannel.h>
#include <bmqio_tcpendpoint.h>
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqsys_threadutil.h>
#include <bmqsys_time.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>
#include <bmqu_weakmemfn.h>

// BDE
#include <ball_log.h>
#include <bdlb_scopeexit.h>
#include <bdlb_string.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_cstdlib.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>
#include <bslalg_swaputil.h>
#include <bslmf_movableref.h>
#include <bslmt_lockguard.h>
#include <bslmt_once.h>
#include <bsls_performancehint.h>
#include <bsls_platform.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

// NTC
#include <bsl_vector.h>
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

namespace BloombergLP {
namespace mqbnet {

const char* TCPSessionFactory::k_CHANNEL_PROPERTY_PEER_IP = "tcp.peer.ip";
const char* TCPSessionFactory::k_CHANNEL_PROPERTY_LOCAL_PORT =
    "tcp.local.port";
const char* TCPSessionFactory::k_CHANNEL_PROPERTY_CHANNEL_ID =
    "channelpool.channel.id";
const char* TCPSessionFactory::k_CHANNEL_STATUS_CLOSE_REASON =
    "reason.brokershutdown";

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBNET.TCPSESSIONFACTORY");

const int k_CONNECT_INTERVAL     = 2;
const int k_SESSION_DESTROY_WAIT = 20;
// Maximum time to wait (in seconds) for all session to be destroyed
// during stop sequence.
const int k_CLIENT_CLOSE_WAIT = 20;
// Time to wait incrementally (in seconds) for all clients and
// proxies to be destroyed during stop sequence.

int calculateInitialMissedHbCounter(const mqbcfg::TcpInterfaceConfig& config)
{
    // Calculate the value with which 'ChannelInfo.d_missedHeartbeatCounter'
    // should be initialized when a channel is established.  We want to give
    // the peer a grace of 3 minutes before we take into account peer's
    // heartbeats.  This is needed so that if the peer is just starting up and
    // is doing heavy lifting (connecting and negotiating with 100s of other
    // peers, syncing storage files, etc), it has enough time to initiate the
    // logic of periodic heartbeats.  Specifically, if the peer connected to
    // self node, it will schedule periodic heartbeat event only after it has
    // received and processed negotiation response from self node (the
    // 'server').  This can take several seconds, specially if peer's IO
    // threads are busy with other connections as well.

    // Based on this formula:
    //..
    //  MaxInactivityIntervalSec = MissedHbCount * HbIntervalSec
    //..
    // we calculate initial value of 'MissedHbCount' like so (taking into
    // account the possibility of overflow):

    const int retVal = bsl::min(
        bsl::numeric_limits<int>::max(),
        static_cast<int>((3 * bdlt::TimeUnitRatio::k_MS_PER_M) /
                         config.heartbeatIntervalMs()));

    return -retVal;
}

bsl::ostream& operator<<(bsl::ostream& os, const bmqio::Channel* channel)
{
    // 'pretty-print' the specified 'channel' to the specified 'os'.  The
    // printed channel from that function includes the address of the channel
    // for easy tracking and matching of logs.

    if (channel) {
        os << channel->peerUri() << "#" << static_cast<const void*>(channel);
    }
    else {
        os << "*null*";
    }

    return os;
}

/// Callback invoked when the specified `channel` is created, as a result of
/// the operation with the specified `operationHandle`.  This is used to set
/// a property on the channel, that higher levels (such as the
/// `SessionNegotiator` can extract and leverage).
void ntcChannelPreCreation(
    const bsl::shared_ptr<bmqio::NtcChannel>& channel,
    BSLA_UNUSED const bsl::shared_ptr<bmqio::ChannelFactory::OpHandle>&
                      operationHandle)
{
    ntsa::Endpoint peerEndpoint   = channel->peerEndpoint();
    ntsa::Endpoint sourceEndpoint = channel->sourceEndpoint();

    if (peerEndpoint.isIp() && peerEndpoint.ip().host().isV4()) {
        channel->properties().set(
            TCPSessionFactory::k_CHANNEL_PROPERTY_PEER_IP,
            static_cast<int>(peerEndpoint.ip().host().v4().value()));
    }

    if (sourceEndpoint.isIp()) {
        channel->properties().set(
            TCPSessionFactory::k_CHANNEL_PROPERTY_LOCAL_PORT,
            static_cast<int>(sourceEndpoint.ip().port()));
    }

    channel->properties().set(TCPSessionFactory::k_CHANNEL_PROPERTY_CHANNEL_ID,
                              channel->channelId());
}

/// Create the ntca::InterfaceConfig to use given the specified
/// `tcpConfig`
ntca::InterfaceConfig
ntcCreateInterfaceConfig(const mqbcfg::TcpInterfaceConfig& tcpConfig)
{
    ntca::InterfaceConfig config;

    config.setThreadName("mqbnet");

    config.setMinThreads(tcpConfig.ioThreads());
    config.setMaxThreads(tcpConfig.ioThreads());
    config.setMaxConnections(tcpConfig.maxConnections());

    config.setWriteQueueLowWatermark(tcpConfig.lowWatermark());
    config.setWriteQueueHighWatermark(tcpConfig.highWatermark());

    config.setAcceptGreedily(false);
    config.setSendGreedily(false);
    config.setReceiveGreedily(false);

    config.setNoDelay(true);
    config.setKeepAlive(true);
    config.setKeepHalfOpen(false);

    return config;
}

/// Load into the specified `resolvedUri` the reverse-DNS resolved URI of
/// the remote peer represented by the specified `baseChannel`.  This is a
/// thin wrapper around the default DNS resolution from
/// `bmqio::ResolvingChannelFactoryUtil` that just adds final resolution
/// logging with time instrumentation.
void monitoredDNSResolution(bsl::string*          resolvedUri,
                            const bmqio::Channel& baseChannel)
{
    const bsls::Types::Int64 start = bmqsys::Time::highResolutionTimer();

    bmqio::ResolvingChannelFactoryUtil::defaultResolutionFn(
        resolvedUri,
        baseChannel,
        &bmqio::ResolveUtil::getDomainName,
        true);

    const bsls::Types::Int64 end = bmqsys::Time::highResolutionTimer();

    BALL_LOG_INFO << "Channel " << static_cast<const void*>(&baseChannel)
                  << " with remote peer " << baseChannel.peerUri()
                  << " resolved to '" << *resolvedUri << "' (took: "
                  << bmqu::PrintUtil::prettyTimeInterval(end - start) << ", "
                  << (end - start) << " nanoseconds)";
    // NOTE: cast the channel to actually just print the address and not the
    //       overload << operator.  The channel's address printed here is that
    //       one of the 'bmqio::TcpChannel', while application will actually
    //       only see the 'bmqio::ResolvingChannelFactory_Channel'.
}

bool isClientOrProxy(const mqbnet::Session* session)
{
    return mqbnet::ClusterUtil::isClientOrProxy(session->negotiationMessage());
}

void stopChannelFactory(bmqio::ChannelFactory* channelFactory)
{
    bmqio::NtcChannelFactory* factory =
        dynamic_cast<bmqio::NtcChannelFactory*>(channelFactory);
    BSLS_ASSERT_SAFE(factory);
    factory->stop();
}

/// A predicate functor for comparing a [mqbcfg::TcpInterfaceListener] by their
/// `port()` member.
struct PortMatcher {
    int d_port;

    PortMatcher(int port)
    : d_port(port)
    {
    }

    bool operator()(const mqbcfg::TcpInterfaceListener& listener)
    {
        return listener.port() == d_port;
    }
};
}  // close unnamed namespace

// -----------------------------------------
// struct TCPSessionFactory_OperationContext
// -----------------------------------------

/// Structure holding a context associated to each individual call to either
/// `listen` or `connect`.
struct TCPSessionFactory_OperationContext {
    TCPSessionFactory::ResultCallback d_resultCb;
    // Callback to invoke when a session is
    // created/failed.

    bool d_isIncoming;
    // True if for incoming session (i.e., associated to
    // a 'listen' operation); false for an outgoing
    // session (i.e., associated to a 'connect'
    // operation).

    bsl::shared_ptr<void> d_negotiationUserData_sp;
    // The negotiation user data, if any, provided by
    // the caller (for the 'connect' operation); unused
    // for a 'listen' operation.  This is the user data
    // that will be passed to the
    // 'Negotiator::negotiate'.

    void* d_resultState_p;
    // The result state cookie, if any, provided by the
    // caller (for the 'connect' operation); unused for
    // a 'listen' operation.  This is the initial value
    // that will be set for the
    // 'InitialConnectionContext::resultState' passed to the
    // 'Negotiator::negotiate'.
};

// -----------------------
// class TCPSessionFactory
// -----------------------

bslma::ManagedPtr<bmqst::StatContext>
TCPSessionFactory::channelStatContextCreator(
    const bsl::shared_ptr<bmqio::Channel>&                  channel,
    const bsl::shared_ptr<bmqio::StatChannelFactoryHandle>& handle)
{
    int peerAddress;
    channel->properties().load(&peerAddress, k_CHANNEL_PROPERTY_PEER_IP);

    ntsa::Ipv4Address   ipv4Address(static_cast<bsl::uint32_t>(peerAddress));
    ntsa::IpAddress     ipAddress(ipv4Address);
    bmqst::StatContext* parent = d_statController_p->channelsStatContext(
        bmqio::ChannelUtil::isLocalHost(ipAddress)
            ? mqbstat::StatController::ChannelSelector::e_LOCAL
            : mqbstat::StatController::ChannelSelector::e_REMOTE);
    BSLS_ASSERT_SAFE(parent);

    bsl::string endpoint =
        handle->options().is<bmqio::ConnectOptions>()
            ? handle->options().the<bmqio::ConnectOptions>().endpoint()
            : channel->peerUri();

    int localPort;
    channel->properties().load(
        &localPort,
        TCPSessionFactory::k_CHANNEL_PROPERTY_LOCAL_PORT);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    return d_ports.addChannelContext(parent,
                                     endpoint,
                                     static_cast<bsl::uint16_t>(localPort));
}

void TCPSessionFactory::handleInitialConnection(
    const bsl::shared_ptr<bmqio::Channel>&   channel,
    const bsl::shared_ptr<OperationContext>& context)
{
    // executed by one of the *IO* threads

    BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name()
                  << "': allocating a channel with '" << channel.get() << "' ["
                  << d_nbActiveChannels << " active channels]";

    // Create a unique InitialConnectionContext for the channel, from
    // the OperationContext.  This shared_ptr is bound to the
    // 'initialConnectionComplete' callback below, which is what scopes its
    // lifetime.
    bsl::shared_ptr<InitialConnectionContext> initialConnectionContext;
    initialConnectionContext.createInplace(d_allocator_p,
                                           context->d_isIncoming);
    (*initialConnectionContext)
        .setUserData(context->d_negotiationUserData_sp.get())
        .setResultState(context->d_resultState_p)
        .setChannel(channel)
        .setCompleteCb(bdlf::BindUtil::bind(
            &TCPSessionFactory::initialConnectionComplete,
            this,
            bdlf::PlaceHolders::_1,  // status
            bdlf::PlaceHolders::_2,  // errorDescription
            bdlf::PlaceHolders::_3,  // session
            bdlf::PlaceHolders::_4,  // channel
            bdlf::PlaceHolders::_5,  // initialConnectionContext
            context));

    // Register as observer of the channel to get the 'onClose'
    channel->onClose(
        bdlf::BindUtil::bindS(d_allocator_p,
                              &TCPSessionFactory::onClose,
                              this,
                              initialConnectionContext,
                              bdlf::PlaceHolders::_1 /* bmqio::Status */));

    // NOTE: we must ensure the 'initialConnectionCompleteCb' can be invoked
    // from the
    //       'handleInitialConnection()' call as specified on the
    //       'InitialConnectionHandler::handleInitialConnection' method
    //       contract (this means we can't have mutex lock around the call to
    //       'handleInitialConnection').
    d_initialConnectionHandler_p->handleInitialConnection(
        initialConnectionContext);
}

void TCPSessionFactory::readCallback(const bmqio::Status& status,
                                     int*                 numNeeded,
                                     bdlbb::Blob*         blob,
                                     ChannelInfo*         channelInfo)
{
    // executed by one of the *IO* threads

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            status.category() == bmqio::StatusCategory::e_CANCELED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // There is nothing to do in the event of a 'e_CANCELED' event, so
        // simply return.

        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            status.category() == bmqio::StatusCategory::e_CONNECTION)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // There is a slight difference in behavior between BTE and NTZ when
        // the peer shuts down the connection. BTE implicitly calls channel
        // close(), and vast majority of the time does not trigger a read
        // callback with CLOSED event (translated to a bmqio e_CONNECTION event
        // by bmqio::Channel); OTH, NTZ always trigger a CLOSED event, but
        // doesn't call close().  We explicitly call close() on the channel
        // here to preserve the same behavior in NTZ as BTE and prevent a
        // warning from being logged.

        channelInfo->d_channel_sp->close();
        return;  // RETURN
    }

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(channelInfo->d_eventProcessor_p &&
                     "EventProcessor must be set at this point");

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!status)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "#TCP_READ_ERROR "
                       << channelInfo->d_session_sp->description()
                       << ": ReadCallback error [status: " << status
                       << ", channel: '" << channelInfo->d_channel_sp.get()
                       << "']";

        // Nothing much we can do, close the channel
        channelInfo->d_channel_sp->close();
        return;  // RETURN
    }

    bdlma::LocalSequentialAllocator<32 * sizeof(bdlbb::Blob) +
                                    sizeof(bsl::vector<bdlbb::Blob>)>
        lsa(d_allocator_p);

    bsl::vector<bdlbb::Blob> readBlobs(&lsa);
    readBlobs.reserve(32);

    const int rc = bmqio::ChannelUtil::handleRead(&readBlobs, numNeeded, blob);
    // NOTE: The blobs in readBlobs will be created using the vector's
    //       allocator, which is LSA, but that is ok because the blobs at the
    //       end are passed as pointer (through bmqp::Event) to the
    //       'eventProcess::processEvent' which makes a full copy if it needs
    //       to async process the blob.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "#TCP_READ_ERROR "
                       << channelInfo->d_session_sp->description()
                       << ": ReadCallback unrecoverable error "
                       << "[status: " << status << ", channel: '"
                       << channelInfo->d_channel_sp.get() << "']:\n"
                       << bmqu::BlobStartHexDumper(blob);

        // Nothing much we can do, close the channel
        channelInfo->d_channel_sp->close();
        return;  // RETURN
    }

    // Not updating d_heartbeatMonitor until there is a valid event

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(readBlobs.empty())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Don't yet have a full blob
        return;  // RETURN
    }

    for (size_t i = 0; i < readBlobs.size(); ++i) {
        const bdlbb::Blob& readBlob = readBlobs[i];

        BALL_LOG_TRACE << channelInfo->d_session_sp->description()
                       << ": ReadCallback got a blob\n"
                       << bmqu::BlobStartHexDumper(&readBlob);

        bmqp::Event event(&readBlob, d_allocator_p);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!event.isValid())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BALL_LOG_ERROR << "#TCP_INVALID_PACKET "
                           << channelInfo->d_session_sp->description()
                           << ": Received an invalid packet:\n"
                           << bmqu::BlobStartHexDumper(&readBlob);
            continue;  // CONTINUE
        }

        if (channelInfo->d_monitor.checkData(channelInfo->d_channel_sp.get(),
                                             event)) {
            channelInfo->d_eventProcessor_p->processEvent(
                event,
                channelInfo->d_session_sp->clusterNode());
        }
    }
}

void TCPSessionFactory::initialConnectionComplete(
    int                                      statusCode,
    const bsl::string&                       errorDescription,
    const bsl::shared_ptr<Session>&          session,
    const bsl::shared_ptr<bmqio::Channel>&   channel,
    const InitialConnectionContext*          initialConnectionContext_p,
    const bsl::shared_ptr<OperationContext>& operationContext)
{
    // executed by one of the *IO* threads

    if (statusCode != 0) {
        // Failed to negotiate
        BALL_LOG_WARN << "#INITIAL_CONNECTION TCPSessionFactory '"
                      << d_config.name() << "' "
                      << "failed to authenticate and negotiate a session "
                      << "[channel: '" << channel.get()
                      << "', status: " << statusCode << ", error: '"
                      << errorDescription << "']";

        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "initialconnectionError",
                             statusCode,
                             d_allocator_p);
        channel->close(status);

        bdlma::LocalSequentialAllocator<64> localAlloc(d_allocator_p);
        bmqu::MemOutStream                  logStream(&localAlloc);
        logStream << "[channel: '" << channel.get() << "]";
        logOpenSessionTime(logStream.str(), channel);
        return;  // RETURN
    }

    // Successful negotiation
    BSLS_ASSERT_SAFE(initialConnectionContext_p);
    BSLS_ASSERT_SAFE(initialConnectionContext_p->negotiationContext());

    BALL_LOG_INFO
        << "TCPSessionFactory '" << d_config.name()
        << "' successfully authenticated and negotiated a session [session: '"
        << session->description() << "', channel: '" << channel.get()
        << "', maxMissedHeartbeat: "
        << initialConnectionContext_p->negotiationContext()
               ->maxMissedHeartbeats()
        << "]";

    // Session is established; keep a hold to it.

    // First, 'decorate' the session shared_ptr's destructor so that we can
    // get a notification upon its destruction.

    // We could have const_cast the supplied 'session', but the below release
    // would then have potential side-effect on the caller if it wanted to
    // still use the object after invoking the negotiation callback.
    bsl::shared_ptr<Session>                  tmpSession = session;
    bsl::pair<Session*, bslma::SharedPtrRep*> rawSession =
        tmpSession.release();
    bsl::shared_ptr<Session> monitoredSession(
        rawSession.first,
        bdlf::BindUtil::bind(&TCPSessionFactory::onSessionDestroyed,
                             this,
                             d_self.acquireWeak(),
                             bdlf::PlaceHolders::_1,  // ptr
                             rawSession.second));     // rep

    ChannelInfoSp                         info;
    bsl::pair<ChannelMap::iterator, bool> inserted;

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        ++d_nbSessions;

        if (isClientOrProxy(session.get())) {
            ++d_nbOpenClients;
        }

        // check if the channel is not closed (we can be in authentication
        // thread)

        if (initialConnectionContext_p->isClosed()) {
            BALL_LOG_WARN
                << "#TCP_UNEXPECTED_STATE TCPSessionFactory '"
                << d_config.name()
                << "' got an already closed channel after negotiation.";

            // Since the 'session' has missed 'onClose', call 'tearDown' here.
            session->tearDown(session, false);
            return;  // RETURN
        }

        info.createInplace(
            d_allocator_p,
            channel,
            initialConnectionContext_p->authenticationContext(),
            monitoredSession,
            initialConnectionContext_p->negotiationContext()->eventProcessor(),
            initialConnectionContext_p->negotiationContext()
                ->maxMissedHeartbeats(),
            d_initialMissedHeartbeatCounter);
        // See comments in 'calculateInitialMissedHbCounter'.

        bsl::pair<bmqio::Channel*, ChannelInfoSp> toInsert(channel.get(),
                                                           info);
        inserted = d_channels.insert(toInsert);
        info     = inserted.first->second;

        if (info->d_monitor.isHearbeatEnabled() &&
            d_heartbeatSchedulerActive) {
            // Enable/Disable heartbeating under the lock
            // If the 'result' below is 'false' and the channel gets closed,
            // then 'onClose' must be called and since the session is inserted
            // into 'd_channels', 'onClose' will disable heartbeat.
            d_scheduler_p->scheduleEvent(
                bsls::TimeInterval(0),
                bdlf::BindUtil::bind(&TCPSessionFactory::enableHeartbeat,
                                     this,
                                     info));
        }
    }  // close mutex lock guard                                      // UNLOCK

    // Do not initiate reading from the channel.  Transport observer(s) will
    // enable the read when they are ready.
    const bool result = operationContext->d_resultCb(
        bmqio::ChannelFactoryEvent::e_CHANNEL_UP,
        bmqio::Status(),
        monitoredSession,
        initialConnectionContext_p->negotiationContext()->cluster(),
        initialConnectionContext_p->resultState(),
        bdlf::BindUtil::bind(&TCPSessionFactory::readCallback,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // numNeeded
                             bdlf::PlaceHolders::_3,  // blob
                             info.get()));

    if (!result || !d_isListening) {
        // TODO: Revisit if still needed, following move to bmqio.
        //
        //       If 'stopListening' have been called, 'tearDown' may or may not
        //       have been called, depending whether the 'callback' has been
        //       called before or after 'stopListening'.  Invoke 'tearDown'
        //       explicitly (it supports subsequent calls).

        BALL_LOG_WARN << "#TCP_UNEXPECTED_STATE "
                      << "TCPSessionFactory '" << d_config.name()
                      << (result ? "' has initiated shutdown "
                                 : "' has encountered an error ")
                      << "while negotiating a session [session: '"
                      << monitoredSession->description() << "', channel: '"
                      << channel.get() << "']";

        // This will eventually call 'btemt_ChannelPool::shutdown' which will
        // schedule channelStateCb/poolSessionStateCb/onClose/tearDown
        channel->close();

        logOpenSessionTime(session->description(), channel);
        return;  // RETURN
    }

    logOpenSessionTime(session->description(), channel);
}

void TCPSessionFactory::onSessionDestroyed(
    const bsl::weak_ptr<TCPSessionFactory>& self,
    void*                                   session,
    void*                                   sprep)
{
    // Delete the session object by releasing its associated rep.
    bslma::SharedPtrRep* rep = static_cast<bslma::SharedPtrRep*>(sprep);
    bool isClient            = isClientOrProxy(static_cast<Session*>(session));
    rep->releaseRef();

    bsl::shared_ptr<TCPSessionFactory> strongSelf = self.lock();
    if (!strongSelf) {
        // The TCPSessionFactory object was destroyed: this could happen
        // because in stop, we timeWait on all sessions to have been destroyed,
        // so if a session takes longer to be destroyed, this method could be
        // invoked after the factory has been deleted, in this case, nothing
        // more to do here.
        return;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> counterGuard(&d_mutex);  // LOCK

    if (isClient) {
        if (--d_nbOpenClients == 0) {
            d_noClientCondition.signal();
        }
    }

    if (--d_nbSessions == 0) {
        d_noSessionCondition.signal();
    }
}

void TCPSessionFactory::channelStateCallback(
    bmqio::ChannelFactoryEvent::Enum         event,
    const bmqio::Status&                     status,
    const bsl::shared_ptr<bmqio::Channel>&   channel,
    const bsl::shared_ptr<OperationContext>& context)
{
    // This function (over time) will be executed by each of the IO threads.
    // This is an infrequent enough operation (compared to a 'readCb') that it
    // is fine to do this here (since we have no other ways to
    // proactively-execute code in the IO threads created by the channelPool).
    bmqsys::ThreadUtil::setCurrentThreadNameOnce(d_threadName);

    BALL_LOG_TRACE << "TCPSessionFactory '" << d_config.name()
                   << "': channelStateCallback [event: " << event
                   << ", status: " << status << ", channel: '" << channel.get()
                   << "', " << d_nbActiveChannels << " active channels]";

    switch (event) {
    case bmqio::ChannelFactoryEvent::e_CHANNEL_UP: {
        BSLS_ASSERT_SAFE(status);  // got a channel up, it must be success
        BSLS_ASSERT_SAFE(channel);

        if (channel->peerUri().empty()) {
            BALL_LOG_ERROR << "#SESSION_NEGOTIATION "
                           << "TCPSessionFactory '" << d_config.name() << "' "
                           << "rejecting empty peer URI: '" << channel.get()
                           << "'";

            bmqio::Status closeStatus(bmqio::StatusCategory::e_GENERIC_ERROR,
                                      d_allocator_p);
            channel->close(closeStatus);
        }
        else {
            {  // Save begin session timestamp
               // TODO: it's possible to store this timestamp directly in one
               // of the bmqio::Channel implementations, so we don't need a
               // mutex synchronization for them at all.
                bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
                d_timestampMap[channel.get()] =
                    bmqsys::Time::highResolutionTimer();
            }  // close mutex lock guard // UNLOCK

            // Keep track of active channels, for logging purposes
            ++d_nbActiveChannels;

            handleInitialConnection(channel, context);
        }
    } break;
    case bmqio::ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED: {
        // Nothing
    } break;
    case bmqio::ChannelFactoryEvent::e_CONNECT_FAILED: {
        // This means the session in 'listen' or 'connect' failed to
        // negotiate (maybe rejected by the remote peer..)
        context->d_resultCb(event,
                            status,
                            bsl::shared_ptr<Session>(),
                            0,  // Cluster*
                            context->d_resultState_p,
                            bmqio::Channel::ReadCallback());
    } break;
    default: {
        BSLS_ASSERT(false &&
                    "Unexpected ChannelFactorEvent; unreachable by design.");
        BSLA_UNREACHABLE;
    }
    }
}

void TCPSessionFactory::onClose(
    const bsl::shared_ptr<InitialConnectionContext>& initialConnectionContext,
    const bmqio::Status&                             status)
{
    // Executed by one of the IO threads.

    --d_nbActiveChannels;

    const bsl::shared_ptr<bmqio::Channel>& channel =
        initialConnectionContext->channel();

    int port;
    channel->properties().load(
        &port,
        TCPSessionFactory::k_CHANNEL_PROPERTY_LOCAL_PORT);

    ChannelInfoSp channelInfo;
    {
        // Lookup the session and remove it from internal map
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        // set the 'isClosed' flag under lock to be checked under lock in
        // 'negotiationComplete'.
        initialConnectionContext->onClose();

        ChannelMap::const_iterator it = d_channels.find(channel.get());
        if (it != d_channels.end()) {
            channelInfo = it->second;
            d_channels.erase(it);

            // Synchronously remove from heartbeat monitored channels
            if (channelInfo->d_monitor.isHearbeatEnabled() &&
                d_heartbeatSchedulerActive) {
                // NOTE: When shutting down, we don't care about heartbeat
                //       verifying the channel, therefore, as an optimization
                //       to avoid the one-by-one disable for each channel (as
                //       they all will get closed at this time), the 'stop()'
                //       sequence cancels the recurring event and wait before
                //       closing the channels, so we don't need to
                //       'disableHeartbeat' in this case.
                d_scheduler_p->scheduleEvent(
                    bsls::TimeInterval(0),
                    bdlf::BindUtil::bind(&TCPSessionFactory::disableHeartbeat,
                                         this,
                                         channel.get()));
            }
        }

        d_ports.onDeleteChannelContext(port);
    }  // close mutex lock guard                                      // UNLOCK

    if (!channelInfo) {
        // We register to the close event as soon as the channel is up;
        // however, we insert in the d_channels only upon successful
        // negotiation; therefore a failed to negotiate channel (like during
        // intrusion testing) would trigger this trace.
        BALL_LOG_INFO << "#TCP_UNEXPECTED_STATE "
                      << "TCPSessionFactory '" << d_config.name()
                      << "': OnClose channel for an unknown channel '"
                      << channel.get() << "', " << d_nbActiveChannels
                      << " active channels, status: " << status;
    }
    else {
        BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name()
                      << "': OnClose channel [session: '"
                      << channelInfo->d_session_sp->description()
                      << "', channel: '" << channel.get() << "', "
                      << d_nbActiveChannels << " active channels"
                      << ", status: " << status << "]";

        // TearDown the session
        int isBrokerShutdown = false;
        if (status.category() == bmqio::StatusCategory::e_SUCCESS) {
            status.properties().load(&isBrokerShutdown,
                                     k_CHANNEL_STATUS_CLOSE_REASON);
        }
        channelInfo->d_session_sp->tearDown(channelInfo->d_session_sp,
                                            isBrokerShutdown);
    }
}

void TCPSessionFactory::onHeartbeatSchedulerEvent()
{
    // executed by the *SCHEDULER* thread

    for (ChannelMap::const_iterator it = d_heartbeatChannels.begin();
         it != d_heartbeatChannels.end();) {
        ChannelInfo* info = it->second.get();
        if (!info->d_monitor.checkHeartbeat(info->d_channel_sp.get())) {
            const Session* session = info->d_session_sp.get();
            BSLS_ASSERT_SAFE(session);
            const ClusterNode* node = session->clusterNode();

            BALL_LOG_WARN << "#TCP_DEAD_CHANNEL TCPSessionFactory '"
                          << d_config.name() << "'"
                          << ": Closing unresponsive channel after "
                          << info->d_monitor.maxMissedHeartbeats()
                          << " missed heartbeats [session: '"
                          << session->description() << "', channel: '"
                          << info->d_channel_sp.get() << "', node: '"
                          << (node ? node->nodeDescription() : "") << "' ]";

            info->d_channel_sp->close();
            // Avoid interference with new connection on the channel
            it = d_heartbeatChannels.erase(it);
        }
        else {
            ++it;
        }
    }
}

void TCPSessionFactory::enableHeartbeat(
    const bsl::shared_ptr<ChannelInfo>& channelInfo_sp)
{
    // executed by the *SCHEDULER* thread

    d_heartbeatChannels[channelInfo_sp->d_channel_sp.get()] = channelInfo_sp;
}

void TCPSessionFactory::disableHeartbeat(const bmqio::Channel* channel_p)
{
    // executed by the *SCHEDULER* thread

    ChannelMap::const_iterator cit = d_heartbeatChannels.find(channel_p);

    if (cit == d_heartbeatChannels.end()) {
        // The `channel_p` have been removed as DEAD
        return;  // RETURN
    }
    const bsl::shared_ptr<ChannelInfo>& channelInfo_sp = cit->second;

    BSLS_ASSERT_SAFE(channelInfo_sp);
    BSLS_ASSERT_SAFE(channelInfo_sp->d_session_sp);

    BALL_LOG_INFO << "Disabling TCPSessionFactory '" << d_config.name()
                  << "' Heartbeat for [session: '"
                  << channelInfo_sp->d_session_sp->description()
                  << "', channel: '" << channelInfo_sp->d_channel_sp.get()
                  << "' ]";

    d_heartbeatChannels.erase(cit);
}

void TCPSessionFactory::logOpenSessionTime(
    const bsl::string&                     sessionDescription,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    bsls::Types::Int64 begin = 0;
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        TimestampMap::const_iterator it = d_timestampMap.find(channel.get());
        if (it != d_timestampMap.end()) {
            begin = it->second;
            d_timestampMap.erase(it);
        }

    }  // close mutex lock guard // UNLOCK

    if (begin) {
        BALL_LOG_INFO_BLOCK
        {
            const bsls::Types::Int64 elapsed =
                bmqsys::Time::highResolutionTimer() - begin;
            BALL_LOG_OUTPUT_STREAM
                << "Open session '" << sessionDescription
                << "' took: " << bmqu::PrintUtil::prettyTimeInterval(elapsed)
                << " (" << elapsed << " nanoseconds)";
        }
    }
}

void TCPSessionFactory::stopHeartbeats()
{
    d_heartbeatChannels.clear();
}

TCPSessionFactory::TCPSessionFactory(
    const mqbcfg::TcpInterfaceConfig& config,
    bdlmt::EventScheduler*            scheduler,
    bdlbb::BlobBufferFactory*         blobBufferFactory,
    InitialConnectionHandler*         initialConnectionHandler,
    mqbstat::StatController*          statController,
    bslma::Allocator*                 allocator)
: d_self(this)  // use default allocator
, d_isStarted(false)
, d_config(config, allocator)
, d_scheduler_p(scheduler)
, d_blobBufferFactory_p(blobBufferFactory)
, d_initialConnectionHandler_p(initialConnectionHandler)
, d_statController_p(statController)
, d_tcpChannelFactory_mp()
, d_resolutionContext(allocator)
, d_resolvingChannelFactory_mp()
, d_reconnectingChannelFactory_mp()
, d_statChannelFactory_mp()
, d_threadName(allocator)
, d_nbActiveChannels(0)
, d_nbOpenClients(0)
, d_nbSessions(0)
, d_noSessionCondition(bsls::SystemClockType::e_MONOTONIC)
, d_noClientCondition(bsls::SystemClockType::e_MONOTONIC)
, d_channels(allocator)
, d_ports(allocator)
, d_heartbeatSchedulerActive(false)
, d_heartbeatChannels(allocator)
, d_initialMissedHeartbeatCounter(calculateInitialMissedHbCounter(config))
, d_listeningHandles(allocator)
, d_isListening(false)
, d_listenContexts(allocator)
, d_timestampMap(allocator)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(scheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);

    // Resolve the default address of this host
    bsl::string hostname;
    ntsa::Error error = bmqio::ResolveUtil::getHostname(&hostname);
    if (error.code() != ntsa::Error::e_OK) {
        BALL_LOG_ERROR << "Failed to get local hostname, error: " << error;
        BSLS_ASSERT_OPT(false && "Failed to get local host name");
        return;  // RETURN
    }

    ntsa::Ipv4Address defaultIP;
    error = bmqio::ResolveUtil::getIpAddress(&defaultIP, hostname);
    if (error.code() != ntsa::Error::e_OK) {
        BALL_LOG_ERROR << "Failed to get IP address of the host '" << hostname
                       << "' error: " << error;
        BSLS_ASSERT_OPT(false && "Failed to get IP address of the host.");
        return;  // RETURN
    }

    BALL_LOG_INFO << "TcpSessionFactory '" << d_config.name() << "' "
                  << "[Hostname: " << hostname << ", ipAddress: " << defaultIP
                  << "]";

    // Thread name
    d_threadName = "bmqIO_" + d_config.name().substr(0, 15 - 6);
    // on Linux, a thread name is limited to 16 characters,
    // including the \0.
}

TCPSessionFactory::~TCPSessionFactory()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");

    BALL_LOG_INFO << "Destructing TCPSessionFactory '" << d_config.name()
                  << "'";

    d_self.invalidate();
}

int TCPSessionFactory::validateTcpInterfaces() const
{
    mqbcfg::TcpInterfaceConfigValidator validator;
    return validator(d_config);
}

void TCPSessionFactory::cancelListeners()
{
    for (ListeningHandleMap::iterator it  = d_listeningHandles.begin(),
                                      end = d_listeningHandles.end();
         it != end;
         ++it) {
        BSLS_ASSERT_SAFE(it->second);
        it->second->cancel();
        it->second.reset();
    }

    d_listeningHandles.clear();
    d_listenContexts.clear();
}

int TCPSessionFactory::start(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "start() can only be called once on this object");

    BALL_LOG_INFO << "Starting TCPSessionFactory '" << d_config.name() << "'";

    int rc = 0;

    rc = validateTcpInterfaces();

    if (rc != 0) {
        errorDescription << "Failed to validate the TCP interface config for "
                         << "TCPSessionFactory '" << d_config.name()
                         << "' [rc: " << rc << "]";
        return rc;  // RETURN
    }

    ntca::InterfaceConfig interfaceConfig = ntcCreateInterfaceConfig(d_config);

    bslma::ManagedPtr<bmqio::NtcChannelFactory> channelFactory;
    channelFactory.load(new (*d_allocator_p)
                            bmqio::NtcChannelFactory(interfaceConfig,
                                                     d_blobBufferFactory_p,
                                                     d_allocator_p),
                        d_allocator_p);

    channelFactory->onCreate(bdlf::BindUtil::bind(&ntcChannelPreCreation,
                                                  bdlf::PlaceHolders::_1,
                                                  bdlf::PlaceHolders::_2));

    rc = channelFactory->start();
    if (rc != 0) {
        errorDescription << "Failed starting channel pool for "
                         << "TCPSessionFactory '" << d_config.name()
                         << "' [rc: " << rc << "]";
        return rc;  // RETURN
    }

    d_tcpChannelFactory_mp = channelFactory;

    bdlb::ScopeExitAny tcpScopeGuard(
        bdlf::BindUtil::bind(&stopChannelFactory,
                             d_tcpChannelFactory_mp.get()));

    bslmt::ThreadAttributes attributes =
        bmqsys::ThreadUtil::defaultAttributes();
    attributes.setThreadName("bmqDNSResolver");
    rc = d_resolutionContext.start(attributes);
    BSLS_ASSERT_SAFE(rc == 0);

    d_resolvingChannelFactory_mp.load(
        new (*d_allocator_p) bmqio::ResolvingChannelFactory(
            bmqio::ResolvingChannelFactoryConfig(
                d_tcpChannelFactory_mp.get(),
                bmqex::ExecutionPolicyUtil::oneWay()
                    .neverBlocking()
                    .useExecutor(d_resolutionContext.executor()),
                d_allocator_p)
                .resolutionFn(bdlf::BindUtil::bind(
                    &monitoredDNSResolution,
                    bdlf::PlaceHolders::_1,    // resolvedUri
                    bdlf::PlaceHolders::_2)),  // channel
            d_allocator_p),
        d_allocator_p);

    d_reconnectingChannelFactory_mp.load(
        new (*d_allocator_p) bmqio::ReconnectingChannelFactory(
            bmqio::ReconnectingChannelFactoryConfig(
                d_resolvingChannelFactory_mp.get(),
                d_scheduler_p,
                d_allocator_p)
                .setReconnectIntervalFn(bdlf::BindUtil::bind(
                    &bmqio::ReconnectingChannelFactoryUtil ::
                        defaultConnectIntervalFn,
                    bdlf::PlaceHolders::_1,        // interval
                    bdlf::PlaceHolders::_2,        // options
                    bdlf::PlaceHolders::_3,        // timeSinceLastAttempt
                    bsls::TimeInterval(3 * 60.0),  // resetReconnectTime
                    bsls::TimeInterval(30.0))),    // maxInterval
            d_allocator_p),
        d_allocator_p);

    rc = d_reconnectingChannelFactory_mp->start();
    if (rc != 0) {
        errorDescription << "Failed starting reconnecting channel factory for "
                         << "TCPSessionFactory '" << d_config.name()
                         << "' [rc: " << rc << "]";
        return rc;  // RETURN
    }

    bdlb::ScopeExitAny reconnectingScopeGuard(
        bdlf::BindUtil::bind(&bmqio::ReconnectingChannelFactory::stop,
                             d_reconnectingChannelFactory_mp.get()));

    d_statChannelFactory_mp.load(
        new (*d_allocator_p) bmqio::StatChannelFactory(
            bmqio::StatChannelFactoryConfig(
                d_reconnectingChannelFactory_mp.get(),
                bdlf::BindUtil::bind(
                    &TCPSessionFactory::channelStatContextCreator,
                    this,
                    bdlf::PlaceHolders::_1,   // channel
                    bdlf::PlaceHolders::_2),  // handle
                d_allocator_p),
            d_allocator_p),
        d_allocator_p);

    if (d_config.heartbeatIntervalMs() != 0) {
        BALL_LOG_INFO
            << "TCPSessionFactory '" << d_config.name()
            << "' heartbeat enabled (interval: "
            << bmqu::PrintUtil::prettyTimeInterval(
                   d_config.heartbeatIntervalMs() *
                   bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND)
            << ")";

        bsls::TimeInterval interval;
        interval.addMilliseconds(d_config.heartbeatIntervalMs());

        d_scheduler_p->scheduleRecurringEvent(
            &d_heartbeatSchedulerHandle,
            interval,
            bmqu::WeakMemFnUtil::weakMemFn(
                &TCPSessionFactory::onHeartbeatSchedulerEvent,
                d_self.acquireWeak()));
        d_heartbeatSchedulerActive = true;
    }
    else {
        BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name()
                      << "' heartbeat globally disabled by config.";
    }

    BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name() << "' "
                  << "successfully started";

    d_isStarted = true;

    reconnectingScopeGuard.release();
    tcpScopeGuard.release();

    return 0;
}

int TCPSessionFactory::startListening(bsl::ostream&         errorDescription,
                                      const ResultCallback& resultCallback)
{
    BSLS_ASSERT_SAFE(d_isStarted && "TCPSessionFactory must be started first");

    // Adapt a legacy listener configuration
    if (d_config.listeners().empty()) {
        mqbcfg::TcpInterfaceListener listener;
        listener.name() = d_config.name();
        listener.port() = d_config.port();
        int rc          = listen(listener, resultCallback);
        if (rc != 0) {
            errorDescription << "Failed listening to port '" << d_config.port()
                             << "' for TCPSessionFactory '" << d_config.name()
                             << "' [rc: " << rc << "]";
            cancelListeners();
            return rc;  // RETURN
        }
    }
    else {
        for (bsl::vector<mqbcfg::TcpInterfaceListener>::const_iterator
                 it  = d_config.listeners().cbegin(),
                 end = d_config.listeners().cend();
             it != end;
             ++it) {
            int rc = listen(*it, resultCallback);
            if (rc != 0) {
                errorDescription << "Failed listening to port '" << it->port()
                                 << "' for TCPSessionFactory '"
                                 << d_config.name() << "' [rc: " << rc << "]";
                cancelListeners();
                return rc;  // RETURN
            }
        }
    }

    d_isListening = true;
    return 0;
}

void TCPSessionFactory::stopListening()
{
    if (!d_isListening) {
        BALL_LOG_WARN << "#TCP_UNEXPECTED_STATE "
                      << "TCPSessionFactory '" << d_config.name()
                      << "' is not listening";
        return;  // RETURN
    }
    d_isListening = false;

    cancelListeners();
}

void TCPSessionFactory::closeClients()
{
    bsl::vector<bsl::weak_ptr<Session> > clients(d_allocator_p);
    bslmt::LockGuard<bslmt::Mutex>       guard(&d_mutex);  // LOCK

    BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name() << "' closing "
                  << d_nbOpenClients << " open client(s)";

    bmqio::Status status(bmqio::StatusCategory::e_SUCCESS,
                         k_CHANNEL_STATUS_CLOSE_REASON,
                         true,
                         d_allocator_p);
    for (ChannelMap::iterator it = d_channels.begin(); it != d_channels.end();
         ++it) {
        if (isClientOrProxy(it->second->d_session_sp.get())) {
            // SDK client or proxy
            clients.push_back(it->second->d_session_sp);
            it->second->d_session_sp->channel()->close(status);
        }
    }

    // Wait for 'onClose' for all sessions.
    while (d_nbOpenClients) {
        bsls::TimeInterval timeout = bsls::SystemTime::nowMonotonicClock();
        timeout.addSeconds(k_CLIENT_CLOSE_WAIT);
        BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name() << "' "
                      << "waiting up to " << k_CLIENT_CLOSE_WAIT << "s for "
                      << d_nbOpenClients << " clients to close";
        const int rc = d_noClientCondition.timedWait(&d_mutex, timeout);
        if (rc == -1) {  // timeout
            break;       // BREAK
        }
    }

    if (d_nbOpenClients) {
        // We timed out
        BALL_LOG_ERROR << "TCPSessionFactory '" << d_config.name() << "' "
                       << "timed out while waiting for clients to close"
                       << ", remaining clients: " << d_nbOpenClients;

        // Invalidate the remaining sessions before stopping all Dispatchers.

        for (size_t i = 0; i < clients.size(); ++i) {
            bsl::shared_ptr<Session> session = clients[i].lock();
            if (session) {
                BALL_LOG_ERROR << "TCPSessionFactory '" << d_config.name()
                               << "' invalidating '" << session->description()
                               << "'";

                session->invalidate();
            }
        }
    }
}

void TCPSessionFactory::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }
    d_isStarted = false;

    BALL_LOG_INFO << "Stopping TCPSessionFactory '" << d_config.name() << "'"
                  << " [" << d_nbActiveChannels << " active channels, "
                  << d_nbSessions << " alive sessions]";

    // NOTE: We don't need to manually call 'teardown' on any active session in
    //       the 'd_channels' map: calling 'stop' on the channel factory will
    //       invoke the 'onClose' for every sessions.

    // STOP
    d_resolutionContext.stop();
    d_resolutionContext.join();

    if (d_reconnectingChannelFactory_mp) {
        d_reconnectingChannelFactory_mp->stop();
    }

    if (d_tcpChannelFactory_mp) {
        stopChannelFactory(d_tcpChannelFactory_mp.get());
    }

    // Wait for all sessions to have been destroyed
    d_mutex.lock();

    if (d_heartbeatSchedulerActive) {
        d_heartbeatSchedulerActive = false;

        // Cancel the scheduled heartbeat event
        d_scheduler_p->cancelEventAndWait(&d_heartbeatSchedulerHandle);

        d_scheduler_p->scheduleEvent(
            bsls::TimeInterval(0),
            bmqu::WeakMemFnUtil::weakMemFn(&TCPSessionFactory::stopHeartbeats,
                                           d_self.acquireWeak()));

        // No need to wait for 'stopHeartbeats' because 'd_heartbeatChannels'
        // keep counted reference to sessions ('ChannelInfo::d_session_sp') and
        // the code below waits for sessions destruction.
    }

    if (d_nbSessions != 0) {
        BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name() << "' "
                      << "waiting up to " << k_SESSION_DESTROY_WAIT << "s "
                      << "for " << d_nbSessions << " sessions to be destroyed";
        while (d_nbSessions != 0) {
            bsls::TimeInterval timeout = bsls::SystemTime::nowMonotonicClock();
            timeout.addSeconds(k_SESSION_DESTROY_WAIT);
            const int rc = d_noSessionCondition.timedWait(&d_mutex, timeout);
            if (rc == -1) {  // timeout
                break;       // BREAK
            }
        }

        if (d_nbSessions != 0) {
            // We timed out
            BALL_LOG_ERROR << "TCPSessionFactory '" << d_config.name() << "' "
                           << "timedout while waiting for sessions to be "
                           << "destroyed, remaining sessions: "
                           << d_nbSessions;
        }
    }
    d_mutex.unlock();

    // DESTROY
    // We destroy the channel factories here for symmetry since it's created in
    // 'start'.
    if (d_statChannelFactory_mp) {
        d_statChannelFactory_mp.clear();
    }
    if (d_reconnectingChannelFactory_mp) {
        d_reconnectingChannelFactory_mp.clear();
    }
    if (d_resolvingChannelFactory_mp) {
        d_resolvingChannelFactory_mp.clear();
    }
    if (d_tcpChannelFactory_mp) {
        d_tcpChannelFactory_mp.clear();
    }

    BALL_LOG_INFO << "Stopped TCPSessionFactory '" << d_config.name() << "'";
}

int TCPSessionFactory::listen(const mqbcfg::TcpInterfaceListener& listener,
                              const ResultCallback& resultCallback)
{
    const int port = listener.port();

    BSLS_ASSERT_SAFE(d_listenContexts.find(port) == d_listenContexts.cend());
    BSLS_ASSERT_SAFE(d_listeningHandles.find(port) ==
                     d_listeningHandles.cend());

    // Maintain ownership of 'OperationContext' instead of passing it to
    // 'ChannelFactory::listen' because it may delete the context
    // (on stopListening) while operation (readCallback / negotiation) is in
    // progress.
    bsl::shared_ptr<OperationContext> context =
        bsl::allocate_shared<OperationContext>(d_allocator_p);
    d_listenContexts.emplace(port, context);

    context->d_resultCb      = resultCallback;
    context->d_isIncoming    = true;
    context->d_resultState_p = 0;

    bdlma::LocalSequentialAllocator<64> localAlloc(d_allocator_p);
    bmqu::MemOutStream                  endpoint(&localAlloc);
    endpoint << ":" << port;  // Empty hostname, listen from all interfaces
    bmqio::ListenOptions listenOptions;
    listenOptions.setEndpoint(endpoint.str());

    bslma::ManagedPtr<bmqio::ChannelFactory::OpHandle> listeningHandle_mp;
    bmqio::Status                                      status;
    d_statChannelFactory_mp->listen(
        &status,
        &listeningHandle_mp,
        listenOptions,
        bdlf::BindUtil::bind(&TCPSessionFactory::channelStateCallback,
                             this,
                             bdlf::PlaceHolders::_1,  // event
                             bdlf::PlaceHolders::_2,  // status
                             bdlf::PlaceHolders::_3,  // channel
                             context));
    if (!status) {
        BALL_LOG_ERROR << "#TCP_LISTEN_FAILED "
                       << "TCPSessionFactory '" << d_config.name() << "' "
                       << "failed listening to '" << endpoint.str()
                       << "' [status: " << status << "]";
        d_isListening = false;
        return status.category();  // RETURN
    }

    BSLS_ASSERT_SAFE(listeningHandle_mp);

    OpHandleSp listeningHandle_sp(listeningHandle_mp, d_allocator_p);
    d_listeningHandles.emplace(port, listeningHandle_sp);

    BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name() << "' "
                  << "successfully listening to '" << endpoint.str() << "'";

    return 0;
}

int TCPSessionFactory::connect(const bslstl::StringRef& endpoint,
                               const ResultCallback&    resultCallback,
                               bslma::ManagedPtr<void>* negotiationUserData,
                               void*                    resultState,
                               bool                     shouldAutoReconnect)
{
    bsl::shared_ptr<OperationContext> context;
    context.createInplace(d_allocator_p);
    context->d_resultCb      = resultCallback;
    context->d_isIncoming    = false;
    context->d_resultState_p = resultState;

    if (negotiationUserData) {
        context->d_negotiationUserData_sp = *negotiationUserData;
    }

    bmqio::TCPEndpoint                  tcpEndpoint(endpoint);
    bdlma::LocalSequentialAllocator<64> localAlloc(d_allocator_p);
    bmqu::MemOutStream                  endpointStream(&localAlloc);
    endpointStream << tcpEndpoint.host() << ":" << tcpEndpoint.port();

    bmqio::ConnectOptions options;
    options.setNumAttempts(bsl::numeric_limits<int>::max())
        .setAttemptInterval(bsls::TimeInterval(k_CONNECT_INTERVAL))
        .setEndpoint(endpointStream.str())
        .setAutoReconnect(shouldAutoReconnect);

    bmqio::Status status;
    d_statChannelFactory_mp->connect(
        &status,
        0,  // no handle ..
        options,
        bdlf::BindUtil::bind(&TCPSessionFactory::channelStateCallback,
                             this,
                             bdlf::PlaceHolders::_1,  // event
                             bdlf::PlaceHolders::_2,  // status
                             bdlf::PlaceHolders::_3,  // channel
                             context));

    if (!status) {
        BALL_LOG_ERROR << "#TCP_CONNECT_FAILED "
                       << "TCPSessionFactory '" << d_config.name() << "' "
                       << "failed connecting to '" << endpointStream.str()
                       << "' [status: " << status << "]";
    }
    else {
        BALL_LOG_DEBUG << "TCPSessionFactory '" << d_config.name() << "' "
                       << "successfully initiated connection to '"
                       << endpointStream.str() << "'";
    }

    return status.category();
}

bool TCPSessionFactory::setNodeWriteQueueWatermarks(const Session& session)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_tcpChannelFactory_mp);
    BSLS_ASSERT_SAFE(d_config.nodeLowWatermark() > 0);
    BSLS_ASSERT_SAFE(d_config.nodeLowWatermark() <=
                     d_config.nodeHighWatermark());

    int channelId;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !session.channel()->properties().load(
                &channelId,
                k_CHANNEL_PROPERTY_CHANNEL_ID))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "TCPSessionFactory '" << d_config.name() << "' "
                       << "failed to get channel id out of '"
                       << session.description() << "'";
        return false;  // RETURN
    }

    bmqio::NtcChannelFactory* factory =
        dynamic_cast<bmqio::NtcChannelFactory*>(d_tcpChannelFactory_mp.get());
    BSLS_ASSERT_SAFE(factory);

    bsl::shared_ptr<bmqio::NtcChannel> ntcChannel;
    int rc = factory->lookupChannel(&ntcChannel, channelId);
    if (rc != 0) {
        BALL_LOG_ERROR << "TCPSessionFactory '" << d_config.name() << "' "
                       << "failed to set watermarks for '"
                       << session.description() << "' [rc: " << rc << "]";
        return false;  // RETURN
    }

    ntcChannel->setWriteQueueLowWatermark(d_config.nodeLowWatermark());
    ntcChannel->setWriteQueueHighWatermark(d_config.nodeHighWatermark());

    return true;
}

// ACCESSORS
bool TCPSessionFactory::isEndpointLoopback(const bslstl::StringRef& uri) const
{
    bmqio::TCPEndpoint endpoint(uri);

    // Use the original port specification method
    if (d_config.listeners().empty()) {
        return (endpoint.port() == d_config.port()) &&
               bmqio::ChannelUtil::isLocalHost(endpoint.host());
    }

    PortMatcher portMatcher(endpoint.port());
    return bsl::any_of(d_config.listeners().cbegin(),
                       d_config.listeners().cend(),
                       portMatcher);
}

// ------------------------------------
// class TCPSessionFactory::ChannelInfo
// ------------------------------------

TCPSessionFactory::ChannelInfo::ChannelInfo(
    const bsl::shared_ptr<bmqio::Channel>&        channel_sp,
    const bsl::shared_ptr<AuthenticationContext>& authenticationContext,
    const bsl::shared_ptr<Session>&               monitoredSession,
    SessionEventProcessor*                        eventProcessor,
    int                                           maxMissedHeartbeats,
    int initialMissedHeartbeatCounter)
: d_channel_sp(channel_sp)
, d_authenticationCtx_sp(authenticationContext)
, d_session_sp(monitoredSession)
, d_eventProcessor_p(eventProcessor)
, d_monitor(maxMissedHeartbeats, initialMissedHeartbeatCounter)
{
    if (!d_eventProcessor_p) {
        // No eventProcessor was provided default to the negotiated session
        d_eventProcessor_p = monitoredSession.get();
    }
}

// ------------------------------------
// class TCPSessionFactory::PortManager
// ------------------------------------

TCPSessionFactory::PortManager::PortManager(bslma::Allocator* allocator)
: d_portMap(allocator)
, d_allocator_p(allocator)
{
}

bslma::ManagedPtr<bmqst::StatContext>
TCPSessionFactory::PortManager::addChannelContext(bmqst::StatContext* parent,
                                                  const bsl::string&  endpoint,
                                                  bsl::uint16_t       port)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);
    bmqst::StatContextConfiguration statConfig(endpoint, &localAllocator);

    bslma::ManagedPtr<bmqst::StatContext> channelStatContext;

    PortMap::iterator portIt = d_portMap.find(port);

    if (portIt != d_portMap.end()) {
        channelStatContext = portIt->second.d_portContext->addSubcontext(
            statConfig);
        ++portIt->second.d_numChannels;
    }
    else {
        bmqst::StatContextConfiguration portConfig(
            static_cast<bsls::Types::Int64>(port),
            &localAllocator);
        bsl::shared_ptr<bmqst::StatContext> portStatContext =
            parent->addSubcontext(
                portConfig.storeExpiredSubcontextValues(true));
        channelStatContext      = portStatContext->addSubcontext(statConfig);
        PortContext portContext = {portStatContext, 1};
        d_portMap.emplace(port, portContext);
    }

    return channelStatContext;
}

void TCPSessionFactory::PortManager::onDeleteChannelContext(bsl::uint16_t port)
{
    // Lookup the port's StatContext and remove it from the internal containers
    PortMap::iterator it = d_portMap.find(port);
    if (it != d_portMap.end() && --it->second.d_numChannels == 0) {
        d_portMap.erase(it);
    }
}

}  // close package namespace
}  // close enterprise namespace
