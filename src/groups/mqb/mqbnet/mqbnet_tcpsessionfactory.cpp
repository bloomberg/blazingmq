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
// When a channel is being created, the following methods are always called, in
// order, regardless of the success or failure of the negotiation:
//: o !channelStateCallback!
//: o !negotiate!
//: o !negotiationComplete!
//
// When a channel goes down, 'onClose()' is the only method being invoked.

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbnet_cluster.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>

// MWC
#include <mwcex_executionutil.h>
#include <mwcex_systemexecutor.h>
#include <mwcio_channelutil.h>
#include <mwcio_connectoptions.h>
#include <mwcio_ntcchannel.h>
#include <mwcio_ntcchannelfactory.h>
#include <mwcio_resolveutil.h>
#include <mwcio_tcpendpoint.h>
#include <mwcsys_threadutil.h>
#include <mwcsys_time.h>
#include <mwcu_blob.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

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
#include <bslalg_swaputil.h>
#include <bslmt_lockguard.h>
#include <bslmt_once.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>
#include <bsls_platform.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

// NTC
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

namespace BloombergLP {
namespace mqbnet {

const char* TCPSessionFactory::k_CHANNEL_PROPERTY_PEER_IP = "tcp.peer.ip";
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

char calculateInitialMissedHbCounter(const mqbcfg::TcpInterfaceConfig& config)
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

    const char retVal = bsl::min(
        bsl::numeric_limits<char>::max(),
        static_cast<char>((3 * bdlt::TimeUnitRatio::k_MS_PER_M) /
                          config.heartbeatIntervalMs()));

    return -retVal;
}

bsl::ostream& operator<<(bsl::ostream& os, const mwcio::Channel* channel)
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
    const bsl::shared_ptr<mwcio::NtcChannel>& channel,
    BSLS_ANNOTATION_UNUSED const
        bsl::shared_ptr<mwcio::ChannelFactory::OpHandle>& operationHandle)
{
    ntsa::Endpoint peerEndpoint = channel->peerEndpoint();

    if (peerEndpoint.isIp() && peerEndpoint.ip().host().isV4()) {
        channel->properties().set(
            TCPSessionFactory::k_CHANNEL_PROPERTY_PEER_IP,
            static_cast<int>(peerEndpoint.ip().host().v4().value()));
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

#ifdef BSLS_PLATFORM_OS_AIX
    // Set stack size to 1Mb for IBM.
    config.setThreadStackSize(1024 * 1024);
#endif

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
/// `mwcio::ResolvingChannelFactoryUtil` that just adds final resolution
/// logging with time instrumentation.
void monitoredDNSResolution(bsl::string*          resolvedUri,
                            const mwcio::Channel& baseChannel)
{
    const bsls::Types::Int64 start = mwcsys::Time::highResolutionTimer();

    mwcio::ResolvingChannelFactoryUtil::defaultResolutionFn(
        resolvedUri,
        baseChannel,
        &mwcio::ResolveUtil::getDomainName,
        true);

    const bsls::Types::Int64 end = mwcsys::Time::highResolutionTimer();

    BALL_LOG_INFO << "Channel " << static_cast<const void*>(&baseChannel)
                  << " with remote peer " << baseChannel.peerUri()
                  << " resolved to '" << *resolvedUri << "' (took: "
                  << mwcu::PrintUtil::prettyTimeInterval(end - start) << ", "
                  << (end - start) << " nanoseconds)";
    // NOTE: cast the channel to actually just print the address and not the
    //       overload << operator.  The channel's address printed here is that
    //       one of the 'mwcio::TcpChannel', while application will actually
    //       only see the 'mwcio::ResolvingChannelFactory_Channel'.
}

bool isClientOrProxy(const mqbnet::Session* session)
{
    return mqbnet::ClusterUtil::isClientOrProxy(session->negotiationMessage());
}

void stopChannelFactory(mwcio::ChannelFactory* channelFactory)
{
    mwcio::NtcChannelFactory* factory =
        dynamic_cast<mwcio::NtcChannelFactory*>(channelFactory);
    BSLS_ASSERT_SAFE(factory);
    factory->stop();
}

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
    // 'NegotiatorContext::resultState' passed to the
    // 'Negotiator::negotiate'.
};

// -----------------------
// class TCPSessionFactory
// -----------------------

bslma::ManagedPtr<mwcst::StatContext>
TCPSessionFactory::channelStatContextCreator(
    const bsl::shared_ptr<mwcio::Channel>&                  channel,
    const bsl::shared_ptr<mwcio::StatChannelFactoryHandle>& handle)
{
    mwcst::StatContext* parent = 0;

    int peerAddress;
    channel->properties().load(&peerAddress, k_CHANNEL_PROPERTY_PEER_IP);

    ntsa::Ipv4Address ipv4Address(static_cast<bsl::uint32_t>(peerAddress));
    ntsa::IpAddress   ipAddress(ipv4Address);
    if (!mwcio::ChannelUtil::isLocalHost(ipAddress)) {
        parent = d_statController_p->channelsStatContext(
            mqbstat::StatController::ChannelSelector::e_LOCAL);
    }
    else {
        parent = d_statController_p->channelsStatContext(
            mqbstat::StatController::ChannelSelector::e_REMOTE);
    }

    BSLS_ASSERT_SAFE(parent);

    bsl::string name;
    if (handle->options().is<mwcio::ConnectOptions>()) {
        name = handle->options().the<mwcio::ConnectOptions>().endpoint();
    }
    else {
        name = channel->peerUri();
    }

    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);
    mwcst::StatContextConfiguration       statConfig(name, &localAllocator);

    return parent->addSubcontext(statConfig);
}

void TCPSessionFactory::negotiate(
    const bsl::shared_ptr<mwcio::Channel>&   channel,
    const bsl::shared_ptr<OperationContext>& context)
{
    // executed by one of the *IO* threads

    BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name()
                  << "': allocating a channel with '" << channel.get() << "' ["
                  << d_nbActiveChannels << " active channels]";

    // Create a unique NegotiatorContext for the channel, from the
    // OperationContext.  This shared_ptr is bound to the 'negotiationComplete'
    // callback below, which is what scopes its lifetime.
    bsl::shared_ptr<NegotiatorContext> negotiatorContextSp;
    negotiatorContextSp.createInplace(d_allocator_p, context->d_isIncoming);
    (*negotiatorContextSp)
        .setUserData(context->d_negotiationUserData_sp.get())
        .setResultState(context->d_resultState_p);

    // NOTE: we must ensure the 'negotiationCb' can be invoked from the
    //       'negotiate()' call as specified on the 'Negotiator::negotiate'
    //       method contract (this means we can't have mutex lock around the
    //       call to 'negotiate').
    d_negotiator_p->negotiate(
        negotiatorContextSp.get(),
        channel,
        bdlf::BindUtil::bind(&TCPSessionFactory::negotiationComplete,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // errorDescription
                             bdlf::PlaceHolders::_3,  // session
                             channel,
                             context,
                             negotiatorContextSp));
}

void TCPSessionFactory::readCallback(const mwcio::Status& status,
                                     int*                 numNeeded,
                                     bdlbb::Blob*         blob,
                                     ChannelInfo*         channelInfo)
{
    // executed by one of the *IO* threads

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            status.category() == mwcio::StatusCategory::e_CANCELED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // There is nothing to do in the event of a 'e_CANCELED' event, so
        // simply return.

        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            status.category() == mwcio::StatusCategory::e_CONNECTION)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // There is a slight difference in behavior between BTE and NTZ when
        // the peer shuts down the connection. BTE implicitly calls channel
        // close(), and vast majority of the time does not trigger a read
        // callback with CLOSED event (translated to a mwcio e_CONNECTION event
        // by mwcio::Channel); OTH, NTZ always trigger a CLOSED event, but
        // doesn't call close().  We explicitly call close() on the channel
        // here to preserve the same behavior in NTZ as BTE and prevent a
        // warning from being logged.

        channelInfo->d_channel_p->close();
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
                       << ", channel: '" << channelInfo->d_channel_p << "']";

        // Nothing much we can do, close the channel
        channelInfo->d_channel_p->close();
        return;  // RETURN
    }

    bdlma::LocalSequentialAllocator<32 * sizeof(bdlbb::Blob) +
                                    sizeof(bsl::vector<bdlbb::Blob>)>
        lsa(d_allocator_p);

    bsl::vector<bdlbb::Blob> readBlobs(&lsa);
    readBlobs.reserve(32);

    const int rc = mwcio::ChannelUtil::handleRead(&readBlobs, numNeeded, blob);
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
                       << channelInfo->d_channel_p << "']:\n"
                       << mwcu::BlobStartHexDumper(blob);

        // Nothing much we can do, close the channel
        channelInfo->d_channel_p->close();
        return;  // RETURN
    }

    if (channelInfo->d_maxMissedHeartbeat != 0) {
        // Heartbeat is enabled on this channel, record incoming packet
        channelInfo->d_packetReceived.storeRelaxed(1);
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(readBlobs.empty())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Don't yet have a full blob
        return;  // RETURN
    }

    for (size_t i = 0; i < readBlobs.size(); ++i) {
        const bdlbb::Blob& readBlob = readBlobs[i];

        BALL_LOG_TRACE << channelInfo->d_session_sp->description()
                       << ": ReadCallback got a blob\n"
                       << mwcu::BlobStartHexDumper(&readBlob);

        bmqp::Event event(&readBlob, d_allocator_p);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!event.isValid())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BALL_LOG_ERROR << "#TCP_INVALID_PACKET "
                           << channelInfo->d_session_sp->description()
                           << ": Received an invalid packet:\n"
                           << mwcu::BlobStartHexDumper(&readBlob);
            continue;  // CONTINUE
        }

        // Process heartbeat: if we receive a heartbeat request, simply reply
        // with a heartbeat response.
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                event.isHeartbeatReqEvent())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            channelInfo->d_channel_p->write(
                0,  // status
                bmqp::ProtocolUtil::heartbeatRspBlob());
            // We explicitly ignore any failure as failure implies issues with
            // the channel, which is what the heartbeat is trying to expose.
            continue;  // CONTINUE
        }
        else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                     event.isHeartbeatRspEvent())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // Nothing to be done, we already updated the packet's counter
            // above, just 'drop' that event now.
            continue;  // CONTINUE
        }

        channelInfo->d_eventProcessor_p->processEvent(
            event,
            channelInfo->d_session_sp->clusterNode());
    }
}

void TCPSessionFactory::negotiationComplete(
    int                                       statusCode,
    const bsl::string&                        errorDescription,
    const bsl::shared_ptr<Session>&           session,
    const bsl::shared_ptr<mwcio::Channel>&    channel,
    const bsl::shared_ptr<OperationContext>&  context,
    const bsl::shared_ptr<NegotiatorContext>& negotiatorContext)
{
    if (statusCode != 0) {
        // Failed to negotiate
        BALL_LOG_WARN << "#SESSION_NEGOTIATION "
                      << "TCPSessionFactory '" << d_config.name() << "' "
                      << "failed to negotiate a session "
                      << "[channel: '" << channel.get()
                      << "', status: " << statusCode << ", error: '"
                      << errorDescription << "']";

        mwcio::Status status(mwcio::StatusCategory::e_GENERIC_ERROR,
                             "negotiationError",
                             statusCode,
                             d_allocator_p);
        channel->close(status);
        return;  // RETURN
    }

    // Successful negotiation
    BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name()
                  << "' successfully negotiated a session "
                  << "[session: '" << session->description() << "', channel: '"
                  << channel.get() << "'"
                  << ", maxMissedHeartbeat: "
                  << static_cast<int>(negotiatorContext->maxMissedHeartbeat())
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

        info.createInplace(d_allocator_p);
        info->d_channel_p        = channel.get();
        info->d_session_sp       = monitoredSession;
        info->d_eventProcessor_p = negotiatorContext->eventProcessor();
        if (!info->d_eventProcessor_p) {
            // No eventProcessor was provided default to the negotiated session
            info->d_eventProcessor_p = monitoredSession.get();
        }

        info->d_packetReceived     = 0;
        info->d_maxMissedHeartbeat = negotiatorContext->maxMissedHeartbeat();
        info->d_missedHeartbeatCounter = d_initialMissedHeartbeatCounter;
        // See comments in 'calculateInitialMissedHbCounter'.

        bsl::pair<mwcio::Channel*, ChannelInfoSp> toInsert(channel.get(),
                                                           info);
        inserted = d_channels.insert(toInsert);
        info     = inserted.first->second;

        if (isClientOrProxy(info->d_session_sp.get())) {
            ++d_nbOpenClients;
        }
    }  // close mutex lock guard                                      // UNLOCK

    // Do not initiate reading from the channel.  Transport observer(s) will
    // enable the read when they are ready.
    bool result = context->d_resultCb(
        mwcio::ChannelFactoryEvent::e_CHANNEL_UP,
        mwcio::Status(),
        monitoredSession,
        negotiatorContext->cluster(),
        negotiatorContext->resultState(),
        bdlf::BindUtil::bind(&TCPSessionFactory::readCallback,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // numNeeded
                             bdlf::PlaceHolders::_3,  // blob
                             info.get()));

    if (!result || !d_isListening) {
        // TODO: Revisit if still needed, following move to mwcio.
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
        return;  // RETURN
    }

    if (info->d_maxMissedHeartbeat != 0) {
        // Enable heartbeating
        d_scheduler_p->scheduleEvent(
            bsls::TimeInterval(0),
            bdlf::BindUtil::bind(&TCPSessionFactory::enableHeartbeat,
                                 this,
                                 info.get()));
    }
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
    mwcio::ChannelFactoryEvent::Enum         event,
    const mwcio::Status&                     status,
    const bsl::shared_ptr<mwcio::Channel>&   channel,
    const bsl::shared_ptr<OperationContext>& context)
{
    // This function (over time) will be executed by each of the IO threads.
    // This is an infrequent enough operation (compared to a 'readCb') that it
    // is fine to do this here (since we have no other ways to
    // proactively-execute code in the IO threads created by the channelPool).
    if (mwcsys::ThreadUtil::k_SUPPORT_THREAD_NAME) {
        mwcsys::ThreadUtil::setCurrentThreadNameOnce(d_threadName);
    }

    BALL_LOG_TRACE << "TCPSessionFactory '" << d_config.name()
                   << "': channelStateCallback [event: " << event
                   << ", status: " << status << ", channel: '" << channel.get()
                   << "', " << d_nbActiveChannels << " active channels]";

    switch (event) {
    case mwcio::ChannelFactoryEvent::e_CHANNEL_UP: {
        BSLS_ASSERT_SAFE(status);  // got a channel up, it must be success
        BSLS_ASSERT_SAFE(channel);

        if (channel->peerUri().empty()) {
            BALL_LOG_ERROR << "#SESSION_NEGOTIATION "
                           << "TCPSessionFactory '" << d_config.name() << "' "
                           << "rejecting empty peer URI: '" << channel.get()
                           << "'";

            mwcio::Status closeStatus(mwcio::StatusCategory::e_GENERIC_ERROR,
                                      d_allocator_p);
            channel->close(closeStatus);
        }
        else {
            // Keep track of active channels, for logging purposes
            ++d_nbActiveChannels;

            // Register as observer of the channel to get the 'onClose'
            channel->onClose(bdlf::BindUtil::bind(
                &TCPSessionFactory::onClose,
                this,
                channel,
                bdlf::PlaceHolders::_1 /* mwcio::Status */));

            negotiate(channel, context);
        }
    } break;
    case mwcio::ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED: {
        // Nothing
    } break;
    case mwcio::ChannelFactoryEvent::e_CONNECT_FAILED: {
        // This means the session in 'listen' or 'connect' failed to
        // negotiate (maybe rejected by the remote peer..)
        context->d_resultCb(event,
                            status,
                            bsl::shared_ptr<Session>(),
                            0,  // Cluster*
                            context->d_resultState_p,
                            mwcio::Channel::ReadCallback());
    } break;
    }
}

void TCPSessionFactory::onClose(const bsl::shared_ptr<mwcio::Channel>& channel,
                                const mwcio::Status&                   status)
{
    --d_nbActiveChannels;

    ChannelInfoSp channelInfo;
    {
        // Lookup the session and remove it from internal map
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        ChannelMap::const_iterator it = d_channels.find(channel.get());
        if (it != d_channels.end()) {
            channelInfo = it->second;
            d_channels.erase(it);
        }
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

        // Synchronously remove from heartbeat monitored channels
        if (channelInfo->d_maxMissedHeartbeat != 0 &&
            d_heartbeatSchedulerActive) {
            // NOTE: When shutting down, we don't care about heartbeat
            //       verifying the channel, therefore, as an optimization to
            //       avoid the one-by-one disable for each channel (as they all
            //       will get closed at this time), the 'stop()' sequence
            //       cancels the recurring event and wait before closing the
            //       channels, so we don't need to 'disableHeartbeat' in this
            //       case.
            d_scheduler_p->scheduleEvent(
                bsls::TimeInterval(0),
                bdlf::BindUtil::bind(&TCPSessionFactory::disableHeartbeat,
                                     this,
                                     channelInfo));
        }

        // TearDown the session
        int isBrokerShutdown = false;
        if (status.category() == mwcio::StatusCategory::e_SUCCESS) {
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

    bsl::unordered_map<mwcio::Channel*, ChannelInfo*>::const_iterator it;
    for (it = d_heartbeatChannels.begin(); it != d_heartbeatChannels.end();
         ++it) {
        ChannelInfo* info = it->second;

        // Always proactively send a sporadic heartbeat response message to
        // notify remote peer of the 'good functioning' of that unidirectional
        // part of the channel.
        //
        /// NOTE
        ///----
        //  - this is necessary in the scenario where broker (A) is sending a
        //    huge amount of data to its peer (B), and (B) is just reading, not
        //    sending anything; therefore (A) will try to send heartbeat
        //    requests, which will be queued behind the data, and not being
        //    delivered in time.
        //  - sending a 'heartbeatRsp' unconditionally make it sound
        //    superfluous to also do the remaining of this method (i.e.,
        //    sending 'heartbeatReq' in case we haven't received any data from
        //    the remote peer), but we still do it as it's a very low
        //    insignificant overhead that can be helpful to ensure good
        //    detection of any one-way TCP issue.
        //
        /// TBD
        ///---
        //  - ideally, we should be 'smart' here too, and only send heartbeats
        //    if we detect we haven't written anything to the channel recently
        //    (similar to the current smart heartbeat logic which monitors
        //    'incoming' traffic).  But this can't be done with current design
        //    as we don't have access to the lower level channel 'write'
        //    wrapper, allowing us to capture all sent out traffic; once
        //    dmcsbte is forked into mwc, smart-heartbeat technology can be
        //    embedded into it: the new channel will keep track of its own
        //    metric (in/out bytes and packets) which can be leveraged to
        //    detect if the channel is idle.
        info->d_channel_p->write(0,  // status
                                 bmqp::ProtocolUtil::heartbeatRspBlob());

        // Perform 'incoming' traffic channel monitoring
        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
                info->d_packetReceived.loadRelaxed() != 0)) {
            // A packet was received on the channel since the last heartbeat
            // check, simply reset the associated counters.
            info->d_packetReceived.storeRelaxed(0);
            info->d_missedHeartbeatCounter = 0;
            continue;  // CONTINUE
        }

        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        if (++info->d_missedHeartbeatCounter == info->d_maxMissedHeartbeat) {
            BALL_LOG_WARN << "#TCP_DEAD_CHANNEL "
                          << "TCPSessionFactory '" << d_config.name() << "'"
                          << ": Closing unresponsive channel after "
                          << static_cast<int>(info->d_maxMissedHeartbeat)
                          << " missed heartbeats [session: '"
                          << info->d_session_sp->description()
                          << "', channel: '" << info->d_channel_p << "']";

            info->d_channel_p->close();
        }
        else {
            // Send heartbeat
            info->d_channel_p->write(0,  // status
                                     bmqp::ProtocolUtil::heartbeatReqBlob());
            // We explicitly ignore any failure as failure implies issues with
            // the channel, which is what the heartbeat is trying to expose.
        }
    }
}

void TCPSessionFactory::enableHeartbeat(ChannelInfo* channelInfo)
{
    // executed by the *SCHEDULER* thread

    d_heartbeatChannels[channelInfo->d_channel_p] = channelInfo;
}

void TCPSessionFactory::disableHeartbeat(
    const bsl::shared_ptr<ChannelInfo>& channelInfo)
{
    // executed by the *SCHEDULER* thread

    BALL_LOG_INFO << "Disabling TCPSessionFactory '" << d_config.name()
                  << "' Heartbeat";

    d_heartbeatChannels.erase(channelInfo->d_channel_p);
}

TCPSessionFactory::TCPSessionFactory(
    const mqbcfg::TcpInterfaceConfig& config,
    bdlmt::EventScheduler*            scheduler,
    bdlbb::BlobBufferFactory*         blobBufferFactory,
    Negotiator*                       negotiator,
    mqbstat::StatController*          statController,
    bslma::Allocator*                 allocator)
: d_self(this)  // use default allocator
, d_isStarted(false)
, d_config(config, allocator)
, d_scheduler_p(scheduler)
, d_blobBufferFactory_p(blobBufferFactory)
, d_negotiator_p(negotiator)
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
, d_heartbeatSchedulerActive(false)
, d_heartbeatChannels(allocator)
, d_initialMissedHeartbeatCounter(calculateInitialMissedHbCounter(config))
, d_isListening(false)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(scheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);

    BALL_LOG_INFO << "Creating TcpSessionFactory: " << config;

    // Resolve the default address of this host
    bsl::string hostname;
    ntsa::Error error = mwcio::ResolveUtil::getHostname(&hostname);
    if (error.code() != ntsa::Error::e_OK) {
        BALL_LOG_ERROR << "Failed to get local hostname, error: " << error;
        BSLS_ASSERT_OPT(false && "Failed to get local host name");
        return;  // RETURN
    }

    ntsa::Ipv4Address defaultIP;
    error = mwcio::ResolveUtil::getIpAddress(&defaultIP, hostname);
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

int TCPSessionFactory::start(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "start() can only be called once on this object");

    BALL_LOG_INFO << "Starting TCPSessionFactory '" << d_config.name() << "'";

    int rc = 0;

    ntca::InterfaceConfig interfaceConfig = ntcCreateInterfaceConfig(d_config);

    bslma::ManagedPtr<mwcio::NtcChannelFactory> channelFactory;
    channelFactory.load(new (*d_allocator_p)
                            mwcio::NtcChannelFactory(interfaceConfig,
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
        mwcsys::ThreadUtil::defaultAttributes();
    attributes.setThreadName("bmqDNSResolver");
    rc = d_resolutionContext.start(attributes);
    BSLS_ASSERT_SAFE(rc == 0);

    d_resolvingChannelFactory_mp.load(
        new (*d_allocator_p) mwcio::ResolvingChannelFactory(
            mwcio::ResolvingChannelFactoryConfig(
                d_tcpChannelFactory_mp.get(),
                mwcex::ExecutionPolicyUtil::oneWay()
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
        new (*d_allocator_p) mwcio::ReconnectingChannelFactory(
            mwcio::ReconnectingChannelFactoryConfig(
                d_resolvingChannelFactory_mp.get(),
                d_scheduler_p,
                d_allocator_p)
                .setReconnectIntervalFn(bdlf::BindUtil::bind(
                    &mwcio::ReconnectingChannelFactoryUtil ::
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
        bdlf::BindUtil::bind(&mwcio::ReconnectingChannelFactory::stop,
                             d_reconnectingChannelFactory_mp.get()));

    d_statChannelFactory_mp.load(
        new (*d_allocator_p) mwcio::StatChannelFactory(
            mwcio::StatChannelFactoryConfig(
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
            << mwcu::PrintUtil::prettyTimeInterval(
                   d_config.heartbeatIntervalMs() *
                   bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND)
            << ")";

        bsls::TimeInterval interval;
        interval.addMilliseconds(d_config.heartbeatIntervalMs());

        d_scheduler_p->scheduleRecurringEvent(
            &d_heartbeatSchedulerHandle,
            interval,
            bdlf::BindUtil::bind(&TCPSessionFactory::onHeartbeatSchedulerEvent,
                                 this));
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

    int rc = listen(d_config.port(), resultCallback);
    if (rc != 0) {
        errorDescription << "Failed listening to port '" << d_config.port()
                         << "' for TCPSessionFactory '" << d_config.name()
                         << "' [rc: " << rc << "]";
        return rc;  // RETURN
    }

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

    BSLS_ASSERT_SAFE(d_listeningHandle_mp);
    d_listeningHandle_mp->cancel();
    d_listeningHandle_mp.reset();

    // NOTE: This is done here as a temporary workaround until channels are
    //       properly stopped (see 'mqba::Application::stop'), because in the
    //       current shutdown sequence, we 'stopListening()' and then
    //       explicitly close each channel one by one in application layer,
    //       instead of calling 'stop()'; therefore this would not allow the
    //       optimization to 'bypass' the one-by-one disablement.
    if (d_heartbeatSchedulerActive) {
        d_heartbeatSchedulerActive = false;
        d_scheduler_p->cancelEventAndWait(&d_heartbeatSchedulerHandle);
        d_heartbeatChannels.clear();
    }
}

void TCPSessionFactory::closeClients()
{
    bsl::vector<bsl::weak_ptr<Session> > clients(d_allocator_p);
    bslmt::LockGuard<bslmt::Mutex>       guard(&d_mutex);  // LOCK

    BALL_LOG_INFO << "TCPSessionFactory '" << d_config.name() << "' closing "
                  << d_nbOpenClients << " open client(s)";

    mwcio::Status status(mwcio::StatusCategory::e_SUCCESS,
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

    // Cancel the heartbeat scheduler event; note that
    // 'd_heartbeatSchedulerActive' must be set to false prior to this cancel
    // event, so that 'onClose' of the channels will not try to uselessly
    // 'disableHeartbeat' on each channel, one-by-one.
    if (d_heartbeatSchedulerActive) {
        d_heartbeatSchedulerActive = false;
        d_scheduler_p->cancelEventAndWait(&d_heartbeatSchedulerHandle);
        d_heartbeatChannels.clear();
    }

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
            // We timedout
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

int TCPSessionFactory::listen(int port, const ResultCallback& resultCallback)
{
    // Supporting only one 'listen' call.
    BSLS_ASSERT_SAFE(!d_isListening);
    BSLS_ASSERT_SAFE(!d_listenContext_mp);

    // Maintain ownership of 'OperationContext' instead of passing it to
    // 'ChannelFactory::listen' because it may delete the context
    // (on stopListening) while operation (readCallback / negotiation) is in
    // progress.
    OperationContext* context = new (*d_allocator_p) OperationContext();
    d_listenContext_mp.load(context, d_allocator_p);
    bsl::shared_ptr<OperationContext> contextSp;
    contextSp.reset(context, bslstl::SharedPtrNilDeleter());

    context->d_resultCb      = resultCallback;
    context->d_isIncoming    = true;
    context->d_resultState_p = 0;

    bdlma::LocalSequentialAllocator<64> localAlloc(d_allocator_p);
    mwcu::MemOutStream                  endpoint(&localAlloc);
    endpoint << ":" << port;  // Empty hostname, listen from all interfaces
    mwcio::ListenOptions listenOptions;
    listenOptions.setEndpoint(endpoint.str());

    d_isListening = true;

    mwcio::Status status;
    d_statChannelFactory_mp->listen(
        &status,
        &d_listeningHandle_mp,
        listenOptions,
        bdlf::BindUtil::bind(&TCPSessionFactory::channelStateCallback,
                             this,
                             bdlf::PlaceHolders::_1,  // event
                             bdlf::PlaceHolders::_2,  // status
                             bdlf::PlaceHolders::_3,  // channel
                             contextSp));
    if (!status) {
        BALL_LOG_ERROR << "#TCP_LISTEN_FAILED "
                       << "TCPSessionFactory '" << d_config.name() << "' "
                       << "failed listening to '" << endpoint.str()
                       << "' [status: " << status << "]";
        d_isListening = false;
        return status.category();  // RETURN
    }

    BSLS_ASSERT_SAFE(d_listeningHandle_mp);
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

    mwcio::TCPEndpoint                  tcpEndpoint(endpoint);
    bdlma::LocalSequentialAllocator<64> localAlloc(d_allocator_p);
    mwcu::MemOutStream                  endpointStream(&localAlloc);
    endpointStream << tcpEndpoint.host() << ":" << tcpEndpoint.port();

    mwcio::ConnectOptions options;
    options.setNumAttempts(bsl::numeric_limits<int>::max())
        .setAttemptInterval(bsls::TimeInterval(k_CONNECT_INTERVAL))
        .setEndpoint(endpointStream.str())
        .setAutoReconnect(shouldAutoReconnect);

    mwcio::Status status;
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

    mwcio::NtcChannelFactory* factory =
        dynamic_cast<mwcio::NtcChannelFactory*>(d_tcpChannelFactory_mp.get());
    BSLS_ASSERT_SAFE(factory);

    bsl::shared_ptr<mwcio::NtcChannel> ntcChannel;
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
    mwcio::TCPEndpoint endpoint(uri);

    return (endpoint.port() == d_config.port()) &&
           mwcio::ChannelUtil::isLocalHost(endpoint.host());
}

}  // close package namespace
}  // close enterprise namespace
