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

// bmqa_session.cpp                                                   -*-C++-*-
#include <bmqa_session.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqa_event.h>
#include <bmqa_message.h>
#include <bmqa_messageevent.h>
#include <bmqa_messageproperties.h>
#include <bmqa_queueid.h>
#include <bmqa_sessionevent.h>
#include <bmqimp_application.h>
#include <bmqimp_event.h>
#include <bmqimp_eventqueue.h>
#include <bmqimp_messagedumper.h>
#include <bmqimp_queue.h>
#include <bmqp_confirmeventbuilder.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqt_correlationid.h>
#include <bmqt_queueflags.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_string.h>
#include <bdlbb_blob.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_processutil.h>
#include <bdlt_timeunitratio.h>
#include <bsl_cstdio.h>
#include <bsl_iostream.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>
#include <bslmf_assert.h>
#include <bslmf_ispolymorphic.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqa {

namespace {

// Do some compile time validation
BSLMF_ASSERT(sizeof(MessageEventBuilder) == sizeof(MessageEventBuilderImpl));
BSLMF_ASSERT(false == bsl::is_polymorphic<MessageEventBuilder>::value);

BSLMF_ASSERT(sizeof(ConfirmEventBuilder) == sizeof(ConfirmEventBuilderImpl));
BSLMF_ASSERT(false == bsl::is_polymorphic<ConfirmEventBuilder>::value);

BSLMF_ASSERT(sizeof(QueueId) == sizeof(bsl::shared_ptr<bmqimp::Queue>));
BSLMF_ASSERT(false == bsl::is_polymorphic<QueueId>::value);

BSLMF_ASSERT(sizeof(Event) == sizeof(bsl::shared_ptr<bmqimp::Event>));
BSLMF_ASSERT(false == bsl::is_polymorphic<Event>::value);

BSLMF_ASSERT(sizeof(MessageEvent) == sizeof(bsl::shared_ptr<bmqimp::Event>));
BSLMF_ASSERT(false == bsl::is_polymorphic<MessageEvent>::value);

BSLMF_ASSERT(sizeof(SessionEvent) == sizeof(bsl::shared_ptr<bmqimp::Event>));
BSLMF_ASSERT(false == bsl::is_polymorphic<SessionEvent>::value);

BSLMF_ASSERT(false == bsl::is_polymorphic<MessageProperties>::value);

// CONSTANTS
const int k_CHANNEL_WRITE_TIMEOUT = 5;  // sec

// LOG CONFIGURATION
BALL_LOG_SET_NAMESPACE_CATEGORY("BMQA.SESSION");

// FUNCTIONS

/// Validate the specified `options` and log an error description to the
/// specified `out` in case of error.  Return 0 on success, and non-zero
/// otherwise.
int validateQueueOptions(bsl::ostream& out, const bmqt::QueueOptions& options)
{
    if (options.maxUnconfirmedMessages() < 0) {
        out << "queueOptions.maxUnconfirmedMessages() must be >= 0 when queue"
               " is configured with READ flag";
        return -1;  // RETURN
    }
    if (options.maxUnconfirmedBytes() < 0) {
        out << "queueOptions.maxUnconfirmedBytes() must be >= 0 when queue is"
               " configured with READ flag";
        return -1;  // RETURN
    }
    if (options.consumerPriority() <
        bmqt::QueueOptions::k_CONSUMER_PRIORITY_MIN) {
        out << "queueOptions.consumerPriority() must be >="
               " bmqt::QueueOptions::k_CONSUMER_PRIORITY_MIN when queue is"
               " configured with READ flag";
        return -1;  // RETURN
    }
    if (options.consumerPriority() >
        bmqt::QueueOptions::k_CONSUMER_PRIORITY_MAX) {
        out << "queueOptions.consumerPriority() must be <="
               " bmqt::QueueOptions::k_CONSUMER_PRIORITY_MAX when queue is"
               " configured with READ flag";
        return -1;  // RETURN
    }

    return 0;
}

// CLASSES

// ===========
// SessionUtil
// ===========

/// This struct provides a collection of functions encapsulating logic that
/// is reused across the implementation of a session with the BlazingMQ
/// broker.
struct SessionUtil {
    // TYPES
    typedef Session::OpenQueueCallback      OpenQueueCallback;
    typedef Session::ConfigureQueueCallback ConfigureQueueCallback;
    typedef Session::CloseQueueCallback     CloseQueueCallback;

    // CLASS METHODS

    // Session management helpers
    // --------------------------

    /// Create the application contained in the specified `sessionImpl` and
    /// return 0 on success, or a non-zero value otherwise.  The behavior is
    /// undefined if this method is called more than once for the same
    /// SessionImpl object.
    static int createApplication(SessionImpl* sessionImpl);

    /// Validate the specified `options` and return 0 if successful and
    /// non-zero otherwise.
    static int validateOptions(const bmqt::SessionOptions& options);

    // Event handler helpers
    // ------------------------

    /// Callback method invoked by the `bmqimp::BrokerSession` to notify of
    /// a new event in the specified `eventImpl`.  This method converts the
    /// event, from the internal `bmqimp::Event` type, to the public
    /// `bmqa::SessionEvent` or `bmqa::MessageEvent` types and forwards it
    /// to the user-provided eventHandler contained in the specified
    /// `sessionImpl` .
    static void eventHandlerCB(const bsl::shared_ptr<bmqimp::Event>& eventImpl,
                               SessionImpl* sessionImpl);

    // Factories and converters
    // ------------------------

    /// Load into the specified `event` encapsulation the "pimpl"
    /// representation provided by the specified `eventImpl`.
    static void
    makeSessionEventFromImpl(SessionEvent*                         event,
                             const bsl::shared_ptr<bmqimp::Event>& eventImpl);

    /// Load into the specified `result` an object of type
    /// `OPERATION_RESULT_TYPE` using the specified `event`.
    template <typename OPERATION_RESULT_TYPE, typename OPERATION_RESULT_ENUM>
    static void createOperationResult(OPERATION_RESULT_TYPE* status,
                                      const SessionEvent&    event);

    // Queue management helpers
    // ------------------------

    /// Invoked when the specified `eventImpl` is emitted from the session
    /// in response to a `...Sync(QueueId *queueId)` operation with the BMQ
    /// broker.  This method populates the optionally specified `result`.
    template <typename OPERATION_RESULT_TYPE, typename OPERATION_RESULT_ENUM>
    static void operationResultSyncWrapper(
        OPERATION_RESULT_TYPE*                result,
        const bsl::shared_ptr<bmqimp::Event>& eventImpl);

    /// Invoked when the specified `eventImpl` is emitted from the session
    /// in response to an asynchronous operation with the BlazingMQ broker
    /// and ready to be communicated to the specified `CALLBACK` with the
    /// specified `OPERATION_RESULT_TYPE` providing the result and context
    /// of the requested operation, using the specified to supply memory in
    /// the result.
    template <typename OPERATION_RESULT_TYPE,
              typename OPERATION_RESULT_ENUM,
              typename CALLBACK>
    static void operationResultCallbackWrapper(
        const bsl::shared_ptr<bmqimp::Event>& eventImpl,
        const CALLBACK&                       callback);

    static bmqt::OpenQueueResult::Enum
    validateAndSetOpenQueueParameters(bmqu::MemOutStream* errorDescription,
                                      QueueId*            queueId,
                                      SessionImpl*        sessionImpl,
                                      const bmqt::Uri&    uri,
                                      const bsls::Types::Uint64 flags,
                                      const bmqt::QueueOptions& options,
                                      const bsls::TimeInterval& timeout);

    static bmqt::ConfigureQueueResult::Enum
    validateAndSetConfigureQueueParameters(
        bmqu::MemOutStream*       errorDescription,
        const QueueId*            queueId,
        const SessionImpl*        sessionImpl,
        const bmqt::QueueOptions& options,
        const bsls::TimeInterval& timeout);

    static bmqt::CloseQueueResult::Enum
    validateAndSetCloseQueueParameters(bmqu::MemOutStream* errorDescription,
                                       const QueueId*      queueId,
                                       const SessionImpl*  sessionImpl,
                                       const bsls::TimeInterval& timeout);
};

// -----------
// SessionUtil
// -----------

// Session management helpers
// --------------------------
int SessionUtil::createApplication(SessionImpl* sessionImpl)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(sessionImpl);
    BSLS_ASSERT_SAFE(!sessionImpl->d_application_mp &&
                     "'createApplication' should only be called once for "
                     "the same session");
    // Should only be called once..

    static bsls::AtomicInt s_sessionInstanceCount(0);

    // Negotiation Message
    bmqp_ctrlmsg::NegotiationMessage negotiationMessage;
    bmqp_ctrlmsg::ClientIdentity& ci = negotiationMessage.makeClientIdentity();
    bsl::string                   features;
    features.append(bmqp::EncodingFeature::k_FIELD_NAME)
        .append(":")
        .append(bmqp::EncodingFeature::k_ENCODING_BER)
        .append(",")
        .append(bmqp::EncodingFeature::k_ENCODING_JSON)
        .append(";")
        .append(bmqp::MessagePropertiesFeatures::k_FIELD_NAME)
        .append(":")
        .append(bmqp::MessagePropertiesFeatures::k_MESSAGE_PROPERTIES_EX);

    ci.protocolVersion() = bmqp::Protocol::k_VERSION;
    ci.sdkVersion()      = bmqscm::Version::versionAsInt();
    ci.clientType()      = bmqp_ctrlmsg::ClientType::E_TCPCLIENT;
    ci.pid()             = bdls::ProcessUtil::getProcessId();
    ci.sessionId()       = ++s_sessionInstanceCount;
    ci.hostName() = "";  // The broker will resolve this from the channel
    ci.features() = features;
    if (!sessionImpl->d_sessionOptions.processNameOverride().empty()) {
        ci.processName() = sessionImpl->d_sessionOptions.processNameOverride();
    }
    else {
        if (bdls::ProcessUtil::getProcessName(&ci.processName()) != 0) {
            ci.processName() = "* unknown *";
        }
    }
    ci.sdkLanguage() = bmqp_ctrlmsg::ClientLanguage::E_CPP;

    // Create the GUID generator
    sessionImpl->d_guidGenerator_sp.createInplace(sessionImpl->d_allocator_p,
                                                  ci.sessionId());

    ci.guidInfo() = sessionImpl->d_guidGenerator_sp->guidInfo();

    // EventHandlerCB
    bmqimp::EventQueue::EventHandlerCallback eventHandler;
    if (sessionImpl->d_eventHandler_mp.get() != 0) {
        eventHandler = bdlf::BindUtil::bind(
            &SessionUtil::eventHandlerCB,
            bdlf::PlaceHolders::_1,  // eventImpl,
            sessionImpl);
    }

    // Override session options from the environment.
    // Currently supported:
    //   - BMQ_BROKER_URI
    bmqt::SessionOptions options(sessionImpl->d_sessionOptions);
    if (const char* brokerUriEnvVar = bsl::getenv("BMQ_BROKER_URI")) {
        if (options.brokerUri().compare(
                bmqt::SessionOptions::k_BROKER_DEFAULT_URI) != 0) {
            BALL_LOG_WARN << "Overriding 'brokerUri' from session options "
                          << "with environment variable 'BMQ_BROKER_URI' "
                          << "[previous: '" << options.brokerUri()
                          << "', override: '" << brokerUriEnvVar << "']";
        }
        options.setBrokerUri(brokerUriEnvVar);
    }

    // Create the application
    sessionImpl->d_application_mp.load(
        new (*(sessionImpl->d_allocator_p))
            bmqimp::Application(options,
                                negotiationMessage,
                                eventHandler,
                                sessionImpl->d_allocator_p),
        sessionImpl->d_allocator_p);

    return 0;
}

int SessionUtil::validateOptions(const bmqt::SessionOptions& options)
{
#define VALIDATE_TIMEOUT(OP)                                                  \
    do {                                                                      \
        const bsls::TimeInterval timeout        = options.OP##Timeout();      \
        const bsls::TimeInterval minRecommended = bsls::TimeInterval(         \
            bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT);         \
        if (timeout < minRecommended) {                                       \
            BALL_LOG_WARN                                                     \
                << "The default " << #OP << " timeout '" << timeout << "' "   \
                << "is lower than the minimum recommended value ("            \
                << bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT    \
                << ")!";                                                      \
            rc = -1;                                                          \
        }                                                                     \
    } while (0)

    int rc = 0;
    VALIDATE_TIMEOUT(openQueue);
    VALIDATE_TIMEOUT(configureQueue);
    VALIDATE_TIMEOUT(closeQueue);

#undef VALIDATE_QUEUE_TIMEOUT

    return rc;
}

// Event handler helpers
// ------------------------
void SessionUtil::eventHandlerCB(
    const bsl::shared_ptr<bmqimp::Event>& eventImpl,
    SessionImpl*                          sessionImpl)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(sessionImpl);
    BSLS_ASSERT_SAFE(sessionImpl->d_eventHandler_mp &&
                     "Must setup to use eventHandler");
    // We should only be called if we had been setup to use eventHandler

    switch (eventImpl->type()) {
    case bmqimp::Event::EventType::e_SESSION: {
        SessionEvent                    event;
        bsl::shared_ptr<bmqimp::Event>& implRef =
            reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(event);
        implRef = eventImpl;
        sessionImpl->d_eventHandler_mp->onSessionEvent(event);
    } break;
    case bmqimp::Event::EventType::e_MESSAGE: {
        MessageEvent                    event;
        bsl::shared_ptr<bmqimp::Event>& implRef =
            reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(event);
        implRef = eventImpl;
        sessionImpl->d_eventHandler_mp->onMessageEvent(event);
    } break;
    case bmqimp::Event::EventType::e_UNINITIALIZED:
    case bmqimp::Event::EventType::e_RAW:
    case bmqimp::Event::EventType::e_REQUEST:
    default: {
        BALL_LOG_ERROR << "Received an unknown event type: " << *(eventImpl);
        BSLS_ASSERT_OPT(false && "Unknown event type");
    }
    }
}

// Factories and converters
// ------------------------
inline void SessionUtil::makeSessionEventFromImpl(
    SessionEvent*                         event,
    const bsl::shared_ptr<bmqimp::Event>& eventImpl)
{
    bsl::shared_ptr<bmqimp::Event>& implRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(*event);
    implRef = eventImpl;
}

template <typename OPERATION_RESULT_TYPE, typename OPERATION_RESULT_ENUM>
void SessionUtil::createOperationResult(OPERATION_RESULT_TYPE* status,
                                        const SessionEvent&    event)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(status);

    *status = OPERATION_RESULT_TYPE(
        event.queueId(),
        static_cast<OPERATION_RESULT_ENUM>(event.statusCode()),
        event.errorDescription());
}

// Queue management helpers
// ------------------------
template <typename OPERATION_RESULT_TYPE, typename OPERATION_RESULT_ENUM>
void SessionUtil::operationResultSyncWrapper(
    OPERATION_RESULT_TYPE*                result,
    const bsl::shared_ptr<bmqimp::Event>& eventImpl)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(result);
    BSLS_ASSERT_SAFE(eventImpl->type() == bmqimp::Event::EventType::e_SESSION);

    SessionEvent event;
    SessionUtil::makeSessionEventFromImpl(&event, eventImpl);
    SessionUtil::createOperationResult<OPERATION_RESULT_TYPE,
                                       OPERATION_RESULT_ENUM>(result, event);
}

template <typename OPERATION_RESULT_TYPE,
          typename OPERATION_RESULT_ENUM,
          typename CALLBACK>
void SessionUtil::operationResultCallbackWrapper(
    const bsl::shared_ptr<bmqimp::Event>& eventImpl,
    const CALLBACK&                       callback)

{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventImpl->type() == bmqimp::Event::EventType::e_SESSION);

    SessionEvent event;
    SessionUtil::makeSessionEventFromImpl(&event, eventImpl);

    OPERATION_RESULT_TYPE result;
    SessionUtil::createOperationResult<OPERATION_RESULT_TYPE,
                                       OPERATION_RESULT_ENUM>(&result, event);

    callback(result);
}

bmqt::OpenQueueResult::Enum SessionUtil::validateAndSetOpenQueueParameters(
    bmqu::MemOutStream*       errorDescription,
    QueueId*                  queueId,
    SessionImpl*              sessionImpl,
    const bmqt::Uri&          uri,
    const bsls::Types::Uint64 flags,
    const bmqt::QueueOptions& options,
    const bsls::TimeInterval& timeout)
{
    BSLS_ASSERT_SAFE(sessionImpl && "Must provide 'sessionImpl'");
    BSLS_ASSERT_SAFE(errorDescription && "Must provide 'errorDescription'");
    BSLS_ASSERT_SAFE(queueId && "Must provide 'queueId'");
    BSLS_ASSERT_SAFE(!queueId->correlationId().isUnset() &&
                     "CorrelationId must be set");

    if (!sessionImpl->d_application_mp ||
        !sessionImpl->d_application_mp->isStarted()) {
        (*errorDescription) << "Not connected";
        return bmqt::OpenQueueResult::e_NOT_CONNECTED;  // RETURN
    }

    if (queueId->correlationId().isUnset()) {
        // While all queueId constructors are setting the correlationId, the
        // user could still use the 'QueueId(const bmqt::CorrelationId&)'
        // constructor and supply an unset 'correlationId'.  Because we use the
        // queue's correlationId as key in a TwoKeyHashMap (refer to the
        // 'QueueMap' inside 'bmqimp::QueueManager'), then user should not use
        // an unset correlationId which would only work for one queue; but for
        // backward compatibility, we can only warn and not reject this
        // request.
        BALL_LOG_ERROR << "CorrelationId unset for queue '" << uri << "' !";
    }

    // If user-provided timeout is less then default
    if (timeout != bsls::TimeInterval() &&
        timeout <
            bsls::TimeInterval(
                bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT)) {
        BALL_LOG_WARN
            << "The specified openQueue timeout '" << timeout << "' is lower "
            << "than the minimum recommended value ("
            << bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT << ")!";
    }

    // Validate the URI
    if (!uri.isValid()) {
        (*errorDescription) << "Invalid URI";
        return bmqt::OpenQueueResult::e_INVALID_URI;  // RETURN
    }

    // Validate the queue flags
    bdlma::LocalSequentialAllocator<1024> localAllocator(
        sessionImpl->d_allocator_p);
    bmqu::MemOutStream error(&localAllocator);
    if (!bmqt::QueueFlagsUtil::isValid(error, flags)) {
        (*errorDescription)
            << "Invalid openQueue flags: '" << error.str() << "'";
        return bmqt::OpenQueueResult::e_INVALID_FLAGS;  // RETURN
    }

    const bool isReader = bmqt::QueueFlagsUtil::isReader(flags);

    // queue 'appId' should only be set if queue is being opened with the read
    // flag only (i.e. no other flag(s))
    if (!uri.id().empty() &&
        (!isReader ||
         bmqt::QueueFlagsUtil::removals(flags, bmqt::QueueFlags::e_READ) !=
             0)) {
        (*errorDescription)
            << "Invalid argument - Queue.id should not be set "
            << "if the queue is opened in write mode [uri: '" << uri << "']";
        return bmqt::OpenQueueResult::e_INVALID_ARGUMENT;  // RETURN
    }

    // Convert to bmqimp::Queue and forward the openQueue to impl
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    bmqt::QueueOptions optionsCopy(options, sessionImpl->d_allocator_p);
    if (isReader) {
        // Validate Queue Options
        bmqu::MemOutStream errorDesc(sessionImpl->d_allocator_p);
        int                rc = validateQueueOptions(errorDesc, optionsCopy);
        if (rc) {
            (*errorDescription) << "Invalid argument - " << errorDesc.str();
            return bmqt::OpenQueueResult::e_INVALID_ARGUMENT;  // RETURN
        }
    }
    else {
        // QueueOptions has default values for maxUnconfirmedMessages and
        // bytes, zero them out in case the queue is not opened with READ flag
        // and set the consumer priority to the invalid priority.
        optionsCopy.setMaxUnconfirmedMessages(0)
            .setMaxUnconfirmedBytes(0)
            .setConsumerPriority(bmqp::Protocol ::k_CONSUMER_PRIORITY_INVALID);
    }

    (*queue).setUri(uri).setFlags(flags).setOptions(optionsCopy);

    return bmqt::OpenQueueResult::e_SUCCESS;
}

bmqt::ConfigureQueueResult::Enum
SessionUtil::validateAndSetConfigureQueueParameters(
    bmqu::MemOutStream*       errorDescription,
    const QueueId*            queueId,
    const SessionImpl*        sessionImpl,
    const bmqt::QueueOptions& options,
    const bsls::TimeInterval& timeout)
{
    BSLS_ASSERT_SAFE(sessionImpl && "Must provide 'sessionImpl'");
    BSLS_ASSERT_SAFE(errorDescription && "Must provide 'errorDescription'");
    BSLS_ASSERT_SAFE(queueId && "Must provide 'queueId'");
    BSLS_ASSERT_SAFE(!queueId->correlationId().isUnset() &&
                     "CorrelationId must be set");

    if (!sessionImpl->d_application_mp ||
        !sessionImpl->d_application_mp->isStarted()) {
        (*errorDescription) << "Not connected";
        return bmqt::ConfigureQueueResult::e_NOT_CONNECTED;  // RETURN
    }

    // If user-provided timeout is less then default
    if (timeout != bsls::TimeInterval() &&
        timeout <
            bsls::TimeInterval(
                bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT)) {
        BALL_LOG_WARN
            << "The specified configureQueue timeout '" << timeout
            << "' is lower than the minimum recommended value ("
            << bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT << ")!";
    }

    // Make sure the provided QueueId is valid
    if (!queueId->isValid()) {
        (*errorDescription) << "Invalid QueueId";
        return bmqt::ConfigureQueueResult::e_INVALID_QUEUE;  // RETURN
    }

    // Validate Options
    bmqu::MemOutStream errorDesc(sessionImpl->d_allocator_p);
    int                rc = validateQueueOptions(errorDesc, options);
    if (rc) {
        (*errorDescription) << "Invalid argument - " << errorDesc.str();
        return bmqt::ConfigureQueueResult::e_INVALID_ARGUMENT;  // RETURN
    }

    return bmqt::ConfigureQueueResult::e_SUCCESS;
}

bmqt::CloseQueueResult::Enum SessionUtil::validateAndSetCloseQueueParameters(
    bmqu::MemOutStream*       errorDescription,
    const QueueId*            queueId,
    const SessionImpl*        sessionImpl,
    const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(sessionImpl && "Must provide 'sessionImpl'");
    BSLS_ASSERT_SAFE(errorDescription && "Must provide 'errorDescription'");
    BSLS_ASSERT_SAFE(queueId && "Must provide 'queueId'");
    BSLS_ASSERT_SAFE(!queueId->correlationId().isUnset() &&
                     "CorrelationId must be set");

    if (!sessionImpl->d_application_mp ||
        !sessionImpl->d_application_mp->isStarted()) {
        (*errorDescription) << "Not connected";
        return bmqt::CloseQueueResult::e_NOT_CONNECTED;  // RETURN
    }

    (void)sessionImpl;  // Compiler happiness in OPT build

    // Make sure the provided QueueId is valid
    if (!queueId->isValid()) {
        (*errorDescription) << "Invalid QueueId";
        return bmqt::CloseQueueResult::e_INVALID_QUEUE;  // RETURN
    }

    // If user-provided timeout is less then default
    if (timeout != bsls::TimeInterval() &&
        timeout <
            bsls::TimeInterval(
                bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT)) {
        BALL_LOG_WARN
            << "The specified closeQueue timeout '" << timeout << "' is lower "
            << "than the minimum recommended value ("
            << bmqt::SessionOptions::k_QUEUE_OPERATION_DEFAULT_TIMEOUT << ")!";
    }

    return bmqt::CloseQueueResult::e_SUCCESS;
}

}  // close unnamed namespace

// -------------------------
// class SessionEventHandler
// -------------------------

SessionEventHandler::~SessionEventHandler()
{
    // NOTHING (required because of virtual class)
}

// ------------------
// struct SessionImpl
// ------------------

SessionImpl::SessionImpl(const bmqt::SessionOptions&            options,
                         bslma::ManagedPtr<SessionEventHandler> eventHandler,
                         bslma::Allocator*                      allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_sessionOptions(options, d_allocator_p)
, d_eventHandler_mp(eventHandler)
, d_guidGenerator_sp()
, d_application_mp(0)
{
    // NOTHING
}

// -------------
// class Session
// -------------

// CREATORS
Session::Session(const bmqt::SessionOptions& options,
                 bslma::Allocator*           allocator)
: d_impl(options, bslma::ManagedPtr<SessionEventHandler>(), allocator)
{
    SessionUtil::validateOptions(options);
}

Session::Session(bslma::ManagedPtr<SessionEventHandler> eventHandler,
                 const bmqt::SessionOptions&            options,
                 bslma::Allocator*                      allocator)
: d_impl(options, eventHandler, allocator)
{
    SessionUtil::validateOptions(options);
}

Session::~Session()
{
    if (d_impl.d_application_mp) {
        d_impl.d_application_mp.clear();
    }
}

// MANIPULATORS
int Session::start(const bsls::TimeInterval& timeout)
{
    int rc = 0;

    // Create application only once
    if (!d_impl.d_application_mp) {
        rc = SessionUtil::createApplication(&d_impl);
        if (rc != 0) {
            return rc;  // RETURN
        }
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().connectTimeout();
    }

    // Start the application
    rc = d_impl.d_application_mp->start(time);
    if (rc != 0) {
        return rc;  // RETURN
    }

    return 0;
}

int Session::startAsync(const bsls::TimeInterval& timeout)
{
    int rc = 0;

    // Create application only once
    if (!d_impl.d_application_mp) {
        rc = SessionUtil::createApplication(&d_impl);
        if (rc != 0) {
            return rc;  // RETURN
        }
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().connectTimeout();
    }

    // Start the application
    rc = d_impl.d_application_mp->startAsync(time);
    if (rc != 0) {
        return rc;  // RETURN
    }

    return 0;
}

void Session::stop()
{
    if (d_impl.d_application_mp) {
        d_impl.d_application_mp->stop();
    }
}

void Session::stopAsync()
{
    if (d_impl.d_application_mp) {
        d_impl.d_application_mp->stopAsync();
    }
}

void Session::finalizeStop()
{
    // Deprecated
    BALL_LOG_WARN << "bmqa::Session::finalizeStop is now deprecated and a "
                     "no-op.";
}

MessageEventBuilder Session::createMessageEventBuilder()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl.d_application_mp && "The session was not started");

    MessageEventBuilder builder;

    // Get MessageEventBuilderImpl from MessageEventBuilder
    MessageEventBuilderImpl& builderRef =
        reinterpret_cast<MessageEventBuilderImpl&>(builder);

    builderRef.d_guidGenerator_sp = d_impl.d_guidGenerator_sp;

    // Get bmqimp::Event sharedptr from MessageEventBuilderImpl
    bsl::shared_ptr<bmqimp::Event>& eventImplSpRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(
            builderRef.d_msgEvent);

    eventImplSpRef = d_impl.d_application_mp->brokerSession().createEvent();

    eventImplSpRef->configureAsMessageEvent(
        d_impl.d_application_mp->blobSpPool());

    return builder;
}

void Session::loadMessageEventBuilder(MessageEventBuilder* builder)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl.d_application_mp && "The session was not started");
    BSLS_ASSERT(builder);

    MessageEventBuilder& builderRef = *builder;

    // Get MessageEventBuilderImpl from MessageEventBuilder
    MessageEventBuilderImpl& builderImplRef =
        reinterpret_cast<MessageEventBuilderImpl&>(builderRef);

    builderImplRef.d_guidGenerator_sp = d_impl.d_guidGenerator_sp;

    // Get bmqimp::Event sharedptr from MessageEventBuilderImpl
    bsl::shared_ptr<bmqimp::Event>& eventImplSpRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(
            builderImplRef.d_msgEvent);

    eventImplSpRef = d_impl.d_application_mp->brokerSession().createEvent();

    eventImplSpRef->configureAsMessageEvent(
        d_impl.d_application_mp->blobSpPool());
}

void Session::loadConfirmEventBuilder(ConfirmEventBuilder* builder)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl.d_application_mp && "This session was not started");
    BSLS_ASSERT(builder);

    ConfirmEventBuilder& builderRef = *builder;

    // Get ConfirmEventBuilderImpl from ConfirmEventBuilder
    ConfirmEventBuilderImpl& builderImplRef =
        reinterpret_cast<ConfirmEventBuilderImpl&>(builderRef);

    if (builderImplRef.d_builder_p) {
        // The load is being invoked on an already initialized confirm event
        // builder.
        builderRef.reset();
        return;  // RETURN
    }

    new (builderImplRef.d_buffer.buffer())
        bmqp::ConfirmEventBuilder(d_impl.d_application_mp->blobSpPool(),
                                  d_impl.d_allocator_p);

    builderImplRef.d_builder_p = reinterpret_cast<bmqp::ConfirmEventBuilder*>(
        builderImplRef.d_buffer.buffer());
}

void Session::loadMessageProperties(MessageProperties* buffer)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl.d_application_mp && "This session was not started");
    BSLS_ASSERT(buffer);

    *buffer = MessageProperties();
}

int Session::getQueueId(QueueId* queueId, const bmqt::Uri& uri)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueId);

    if (!d_impl.d_application_mp || !d_impl.d_application_mp->isStarted()) {
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    bsl::shared_ptr<bmqimp::Queue> queue =
        d_impl.d_application_mp->brokerSession().lookupQueue(uri);

    if (!queue) {
        // Not found ..
        return bmqt::GenericResult::e_UNKNOWN;  // RETURN
    }

    bsl::shared_ptr<bmqimp::Queue>& queueImplSpRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);
    queueImplSpRef = queue;

    return 0;
}

int Session::getQueueId(QueueId*                   queueId,
                        const bmqt::CorrelationId& correlationId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueId);

    if (!d_impl.d_application_mp || !d_impl.d_application_mp->isStarted()) {
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    bsl::shared_ptr<bmqimp::Queue> queue =
        d_impl.d_application_mp->brokerSession().lookupQueue(correlationId);

    if (!queue) {
        // Not found ..
        return bmqt::GenericResult::e_UNKNOWN;  // RETURN
    }

    bsl::shared_ptr<bmqimp::Queue>& queueImplSpRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);
    queueImplSpRef = queue;

    return 0;
}

int Session::openQueue(QueueId*                  queueId,
                       const bmqt::Uri&          uri,
                       bsls::Types::Uint64       flags,
                       const bmqt::QueueOptions& options,
                       const bsls::TimeInterval& timeout)
{
    // Validate parameters
    bmqu::MemOutStream          error;
    bmqt::OpenQueueResult::Enum validationRc =
        SessionUtil::validateAndSetOpenQueueParameters(&error,
                                                       queueId,
                                                       &d_impl,
                                                       uri,
                                                       flags,
                                                       options,
                                                       timeout);
    if (validationRc != bmqt::OpenQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and return error code
        BALL_LOG_ERROR << error.str();
        return static_cast<int>(validationRc);  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().openQueueTimeout();
    }

    // Convert to bmqimp::Queue and forward the openQueue to impl
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    BALL_LOG_INFO << "Open queue (SYNC DEPRECATED)"
                  << " [queue: " << *queue << ", options: " << options
                  << ", timeout: " << time << "]";

    int rc = d_impl.d_application_mp->brokerSession().openQueue(queue, time);

    return rc;
}

OpenQueueStatus Session::openQueueSync(QueueId*                  queueId,
                                       const bmqt::Uri&          uri,
                                       bsls::Types::Uint64       flags,
                                       const bmqt::QueueOptions& options,
                                       const bsls::TimeInterval& timeout)
{
    // Validate parameters
    bmqu::MemOutStream          error;
    bmqt::OpenQueueResult::Enum validationRc =
        SessionUtil::validateAndSetOpenQueueParameters(&error,
                                                       queueId,
                                                       &d_impl,
                                                       uri,
                                                       flags,
                                                       options,
                                                       timeout);
    if (validationRc != bmqt::OpenQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and return error code
        BALL_LOG_ERROR << error.str();
        return OpenQueueStatus(*queueId, validationRc,
                               error.str());  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().openQueueTimeout();
    }

    // Convert to 'bmqimp::Queue' and forward the openQueue to impl (the result
    // of the operation is serialized through the queue of incoming events).
    bmqa::OpenQueueStatus           status;
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    BALL_LOG_INFO << "Open queue (SYNC)"
                  << " [queue: " << *queue << ", options: " << options
                  << ", timeout: " << time << "]";

    d_impl.d_application_mp->brokerSession().openQueueSync(
        queue,
        time,
        bdlf::BindUtil::bind(&SessionUtil::operationResultSyncWrapper<
                                 bmqa::OpenQueueStatus,
                                 bmqt::OpenQueueResult::Enum>,
                             &status,
                             bdlf::PlaceHolders::_1));  // eventImpl

    return status;
}

int Session::openQueue(const QueueId&            queueId,
                       const bmqt::Uri&          uri,
                       bsls::Types::Uint64       flags,
                       const bmqt::QueueOptions& options,
                       const bsls::TimeInterval& timeout)
{
    QueueId* queue = const_cast<QueueId*>(&queueId);
    return openQueue(queue, uri, flags, options, timeout);
}

int Session::openQueueAsync(QueueId*                  queueId,
                            const bmqt::Uri&          uri,
                            bsls::Types::Uint64       flags,
                            const bmqt::QueueOptions& options,
                            const bsls::TimeInterval& timeout)
{
    // Validate parameters
    bmqu::MemOutStream          error;
    bmqt::OpenQueueResult::Enum validationRc =
        SessionUtil::validateAndSetOpenQueueParameters(&error,
                                                       queueId,
                                                       &d_impl,
                                                       uri,
                                                       flags,
                                                       options,
                                                       timeout);
    if (validationRc != bmqt::OpenQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and return error code
        BALL_LOG_ERROR << error.str();
        return static_cast<int>(validationRc);  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().openQueueTimeout();
    }

    // Convert to bmqimp::Queue and forward the openQueue to impl
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    BALL_LOG_INFO << "Open queue (ASYNC)"
                  << " [queue: " << *queue << ", options: " << options
                  << ", timeout: " << time << "]";

    int rc = d_impl.d_application_mp->brokerSession().openQueueAsync(queue,
                                                                     time);

    return rc;
}

void Session::openQueueAsync(bmqa::QueueId*            queueId,
                             const bmqt::Uri&          uri,
                             bsls::Types::Uint64       flags,
                             const OpenQueueCallback&  callback,
                             const bmqt::QueueOptions& options,
                             const bsls::TimeInterval& timeout)
{
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    // Wrap the user-specified callback
    const bmqimp::BrokerSession::EventCallback eventCallback =
        bdlf::BindUtil::bind(&SessionUtil::operationResultCallbackWrapper<
                                 bmqa::OpenQueueStatus,
                                 bmqt::OpenQueueResult::Enum,
                                 OpenQueueCallback>,
                             bdlf::PlaceHolders::_1,  // eventImpl
                             callback);

    // Validate parameters
    bmqu::MemOutStream          error;
    bmqt::OpenQueueResult::Enum validationRc =
        SessionUtil::validateAndSetOpenQueueParameters(&error,
                                                       queueId,
                                                       &d_impl,
                                                       uri,
                                                       flags,
                                                       options,
                                                       timeout);
    if (validationRc != bmqt::OpenQueueResult::e_SUCCESS) {
        // Validation failed - enqueue callback invocation with error result
        BALL_LOG_ERROR << error.str();
        d_impl.d_application_mp->brokerSession().enqueueSessionEvent(
            bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
            static_cast<int>(validationRc),
            error.str(),
            queue->correlationId(),
            queue,
            eventCallback);
        return;  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().openQueueTimeout();
    }

    BALL_LOG_INFO << "Open queue (ASYNC)"
                  << " [queue: " << *queue << ", options: " << options
                  << ", timeout: " << time << "]";

    // Forward the openQueue to impl
    d_impl.d_application_mp->brokerSession().openQueueAsync(queue,
                                                            time,
                                                            eventCallback);
}

int Session::configureQueue(QueueId*                  queueId,
                            const bmqt::QueueOptions& options,
                            const bsls::TimeInterval& timeout)
{
    // Validate parameters
    bmqu::MemOutStream               error;
    bmqt::ConfigureQueueResult::Enum validationRc =
        SessionUtil::validateAndSetConfigureQueueParameters(&error,
                                                            queueId,
                                                            &d_impl,
                                                            options,
                                                            timeout);
    if (validationRc != bmqt::ConfigureQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and return error code
        BALL_LOG_ERROR << error.str();
        return static_cast<int>(validationRc);  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time =
            d_impl.d_application_mp->sessionOptions().configureQueueTimeout();
    }

    // Convert to bmqimp::Queue and forward the configureQueue to impl
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    BALL_LOG_INFO << "Configure queue (SYNC DEPRECATED)"
                  << " [queue: " << *queue << ", options: " << options
                  << ", timeout: " << time << "]";

    int rc = d_impl.d_application_mp->brokerSession().configureQueue(queue,
                                                                     options,
                                                                     time);

    return rc;
}

ConfigureQueueStatus
Session::configureQueueSync(QueueId*                  queueId,
                            const bmqt::QueueOptions& options,
                            const bsls::TimeInterval& timeout)
{
    // Validate parameters
    bmqu::MemOutStream               error;
    bmqt::ConfigureQueueResult::Enum validationRc =
        SessionUtil::validateAndSetConfigureQueueParameters(&error,
                                                            queueId,
                                                            &d_impl,
                                                            options,
                                                            timeout);
    if (validationRc != bmqt::ConfigureQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and return error code
        BALL_LOG_ERROR << error.str();
        return ConfigureQueueStatus(*queueId,
                                    validationRc,
                                    error.str());  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time =
            d_impl.d_application_mp->sessionOptions().configureQueueTimeout();
    }

    // Convert to bmqimp::Queue and forward the configureQueue to impl (the
    // result of the operation is serialized through the queue of incoming
    // events).
    bmqa::ConfigureQueueStatus            status;
    const bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<const bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    BALL_LOG_INFO << "Configure queue (SYNC)"
                  << " [queue: " << *queue << ", options: " << options
                  << ", timeout: " << time << "]";

    d_impl.d_application_mp->brokerSession().configureQueueSync(
        queue,
        options,
        time,
        bdlf::BindUtil::bind(&SessionUtil::operationResultSyncWrapper<
                                 bmqa::ConfigureQueueStatus,
                                 bmqt::ConfigureQueueResult::Enum>,
                             &status,
                             bdlf::PlaceHolders::_1));  // eventImpl

    return status;
}

int Session::configureQueueAsync(QueueId*                  queueId,
                                 const bmqt::QueueOptions& options,
                                 const bsls::TimeInterval& timeout)
{
    // Validate parameters
    bmqu::MemOutStream               error;
    bmqt::ConfigureQueueResult::Enum validationRc =
        SessionUtil::validateAndSetConfigureQueueParameters(&error,
                                                            queueId,
                                                            &d_impl,
                                                            options,
                                                            timeout);
    if (validationRc != bmqt::ConfigureQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and return error code
        BALL_LOG_ERROR << error.str();
        return static_cast<int>(validationRc);  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time =
            d_impl.d_application_mp->sessionOptions().configureQueueTimeout();
    }

    // Convert to bmqimp::Queue and forward the configureQueueAsync to impl
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    BALL_LOG_INFO << "Configure queue (ASYNC)"
                  << " [queue: " << *queue << ", options: " << options
                  << ", timeout: " << time << "]";

    int rc = d_impl.d_application_mp->brokerSession().configureQueueAsync(
        queue,
        options,
        time);

    return rc;
}

void Session::configureQueueAsync(QueueId*                      queueId,
                                  const bmqt::QueueOptions&     options,
                                  const ConfigureQueueCallback& callback,
                                  const bsls::TimeInterval&     timeout)
{
    const bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<const bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    // Wrap the user-specified callback
    const bmqimp::BrokerSession::EventCallback eventCallback =
        bdlf::BindUtil::bind(&SessionUtil::operationResultCallbackWrapper<
                                 bmqa::ConfigureQueueStatus,
                                 bmqt::ConfigureQueueResult::Enum,
                                 ConfigureQueueCallback>,
                             bdlf::PlaceHolders::_1,  // eventImpl
                             callback);

    // Validate parameters
    bmqu::MemOutStream               error;
    bmqt::ConfigureQueueResult::Enum validationRc =
        SessionUtil::validateAndSetConfigureQueueParameters(&error,
                                                            queueId,
                                                            &d_impl,
                                                            options,
                                                            timeout);
    if (validationRc != bmqt::ConfigureQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and invoke the callback with
        // error result
        BALL_LOG_ERROR << error.str();
        d_impl.d_application_mp->brokerSession().enqueueSessionEvent(
            bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
            static_cast<int>(validationRc),
            error.str(),
            queue->correlationId(),
            queue,
            eventCallback);
        return;  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time =
            d_impl.d_application_mp->sessionOptions().configureQueueTimeout();
    }

    BALL_LOG_INFO << "Configure queue (ASYNC)"
                  << " [queue: " << *queue << ", options: " << options
                  << ", timeout: " << time << "]";

    d_impl.d_application_mp->brokerSession()
        .configureQueueAsync(queue, options, time, eventCallback);
}

int Session::closeQueue(QueueId* queueId, const bsls::TimeInterval& timeout)
{
    // Validate parameters
    bmqu::MemOutStream           error;
    bmqt::CloseQueueResult::Enum validationRc =
        SessionUtil::validateAndSetCloseQueueParameters(&error,
                                                        queueId,
                                                        &d_impl,
                                                        timeout);
    if (validationRc != bmqt::CloseQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and return error code
        BALL_LOG_ERROR << error.str();
        return static_cast<int>(validationRc);  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().closeQueueTimeout();
    }

    // Convert to bmqimp::Queue and forward the configureQueue to impl
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    BALL_LOG_INFO << "Close queue (SYNC DEPRECATED)"
                  << " [queue: " << *queue << ", timeout: " << time << "]";

    int rc = d_impl.d_application_mp->brokerSession().closeQueue(queue, time);

    return rc;
}

CloseQueueStatus Session::closeQueueSync(QueueId*                  queueId,
                                         const bsls::TimeInterval& timeout)
{
    // Validate parameters
    bmqu::MemOutStream           error;
    bmqt::CloseQueueResult::Enum validationRc =
        SessionUtil::validateAndSetCloseQueueParameters(&error,
                                                        queueId,
                                                        &d_impl,
                                                        timeout);
    if (validationRc != bmqt::CloseQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and return error code
        BALL_LOG_ERROR << error.str();
        return CloseQueueStatus(*queueId,
                                validationRc,
                                error.str());  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().closeQueueTimeout();
    }

    // Convert to bmqimp::Queue and forward the configureQueue to impl (the
    // result of the operation is serialized through the queue of incoming
    // events).
    bmqa::CloseQueueStatus                status;
    const bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<const bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    BALL_LOG_INFO << "Close queue (SYNC)"
                  << " [queue: " << *queue << ", timeout: " << time << "]";

    d_impl.d_application_mp->brokerSession().closeQueueSync(
        queue,
        time,
        bdlf::BindUtil::bind(&SessionUtil::operationResultSyncWrapper<
                                 bmqa::CloseQueueStatus,
                                 bmqt::CloseQueueResult::Enum>,
                             &status,
                             bdlf::PlaceHolders::_1));  // eventImpl

    return status;
}

int Session::closeQueue(const QueueId&            queueId,
                        const bsls::TimeInterval& timeout)
{
    QueueId* queue = const_cast<QueueId*>(&queueId);
    return closeQueue(queue, timeout);
}

int Session::closeQueueAsync(QueueId*                  queueId,
                             const bsls::TimeInterval& timeout)
{
    // Validate parameters
    bmqu::MemOutStream           error;
    bmqt::CloseQueueResult::Enum validationRc =
        SessionUtil::validateAndSetCloseQueueParameters(&error,
                                                        queueId,
                                                        &d_impl,
                                                        timeout);
    if (validationRc != bmqt::CloseQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and return error code
        BALL_LOG_ERROR << error.str();
        return static_cast<int>(validationRc);  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().closeQueueTimeout();
    }

    // Convert to bmqimp::Queue and forward the configureQueue to impl
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    BALL_LOG_INFO << "Close queue (ASYNC)"
                  << " [queue: " << *queue << ", timeout: " << time << "]";

    int rc = d_impl.d_application_mp->brokerSession().closeQueueAsync(queue,
                                                                      time);

    return rc;
}

void Session::closeQueueAsync(QueueId*                  queueId,
                              const CloseQueueCallback& callback,
                              const bsls::TimeInterval& timeout)
{
    const bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<const bsl::shared_ptr<bmqimp::Queue>&>(*queueId);

    // Wrap the user-specified callback
    const bmqimp::BrokerSession::EventCallback eventCallback =
        bdlf::BindUtil::bind(&SessionUtil::operationResultCallbackWrapper<
                                 bmqa::CloseQueueStatus,
                                 bmqt::CloseQueueResult::Enum,
                                 CloseQueueCallback>,
                             bdlf::PlaceHolders::_1,  // eventImpl
                             callback);

    // Validate parameters
    bmqu::MemOutStream           error;
    bmqt::CloseQueueResult::Enum validationRc =
        SessionUtil::validateAndSetCloseQueueParameters(&error,
                                                        queueId,
                                                        &d_impl,
                                                        timeout);
    if (validationRc != bmqt::CloseQueueResult::e_SUCCESS) {
        // Validation failed - abort operation and invoke the callback with
        // error result
        BALL_LOG_ERROR << error.str();
        d_impl.d_application_mp->brokerSession().enqueueSessionEvent(
            bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT,
            static_cast<int>(validationRc),
            error.str(),
            queue->correlationId(),
            queue,
            eventCallback);
        return;  // RETURN
    }

    // Use default timeout (from SessionOptions) if none was provided
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time = d_impl.d_application_mp->sessionOptions().closeQueueTimeout();
    }

    BALL_LOG_INFO << "Close queue (ASYNC)"
                  << " [queue: " << *queue << ", timeout: " << time << "]";

    d_impl.d_application_mp->brokerSession().closeQueueAsync(queue,
                                                             time,
                                                             eventCallback);
}

int Session::closeQueueAsync(const QueueId&            queueId,
                             const bsls::TimeInterval& timeout)
{
    QueueId* queue = const_cast<QueueId*>(&queueId);
    return closeQueueAsync(queue, timeout);
}

Event Session::nextEvent(const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl.d_application_mp && "The session was not started");

    Event                           event;
    bsl::shared_ptr<bmqimp::Event>& eventImplSpRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(event);

    // If no timeout was specified, use a long timeout to simulate a 'no
    // timeout' behavior
    bsls::TimeInterval time = timeout;
    if (time == bsls::TimeInterval()) {
        time.addDays(365);
    }

    eventImplSpRef = d_impl.d_application_mp->brokerSession().nextEvent(time);
    return event;
}

int Session::post(const MessageEvent& event)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !d_impl.d_application_mp ||
            !d_impl.d_application_mp->isStarted())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    const bsl::shared_ptr<bmqimp::Event>& eventSpRef =
        reinterpret_cast<const bsl::shared_ptr<bmqimp::Event>&>(event);

    BSLS_ASSERT_SAFE(0 != eventSpRef.get());

    return d_impl.d_application_mp->brokerSession().post(
        *(eventSpRef->rawEvent().blob()),
        bsls::TimeInterval(k_CHANNEL_WRITE_TIMEOUT));
}

int Session::confirmMessage(const MessageConfirmationCookie& cookie)
{
    // Check there is a connection with broker. Return error if there is no
    // connection (e.g. the session is not started or reconnecting) because
    // CONFIRM messages are not buffered and retransmitted.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !d_impl.d_application_mp ||
            d_impl.d_application_mp->brokerSession().state() !=
                bmqimp::BrokerSession::State::e_STARTED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    const bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<const bsl::shared_ptr<bmqimp::Queue>&>(
            cookie.queueId());

    return d_impl.d_application_mp->brokerSession().confirmMessage(
        queue,
        cookie.messageGUID(),
        bsls::TimeInterval(k_CHANNEL_WRITE_TIMEOUT));
}

int Session::confirmMessages(ConfirmEventBuilder* builder)
{
    // Check there is a connection with broker. Return error if there is no
    // connection (e.g. the session is not started or reconnecting) because
    // CONFIRM messages are not buffered and retransmitted.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !d_impl.d_application_mp ||
            d_impl.d_application_mp->brokerSession().state() !=
                bmqimp::BrokerSession::State::e_STARTED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    if (builder->messageCount() == 0) {
        return bmqt::GenericResult::e_SUCCESS;  // RETURN
    }

    const int rc = d_impl.d_application_mp->brokerSession().confirmMessages(
        *(&builder->blob()),
        bsls::TimeInterval(k_CHANNEL_WRITE_TIMEOUT));

    if (bmqt::GenericResult::e_SUCCESS == rc) {
        builder->reset();
    }

    return rc;
}

int Session::configureMessageDumping(const bslstl::StringRef& command)
{
    if (!d_impl.d_application_mp || !d_impl.d_application_mp->isStarted()) {
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS       = 0,
        rc_PARSE_FAILURE = -1
    };

    bmqp_ctrlmsg::DumpMessages cmd;

    int rc = bmqimp::MessageDumper::parseCommand(&cmd, command);
    if (rc != 0) {
        return 10 * rc + rc_PARSE_FAILURE;  // RETURN
    }

    d_impl.d_application_mp->brokerSession().processDumpCommand(cmd);

    return rc_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
