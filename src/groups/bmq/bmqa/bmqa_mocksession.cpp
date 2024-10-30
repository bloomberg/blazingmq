// Copyright 2016-2023 Bloomberg Finance L.P.
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

// bmqa_mocksession.cpp                                               -*-C++-*-
#include <bmqa_mocksession.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqa_confirmeventbuilder.h>
#include <bmqa_message.h>
#include <bmqa_messageiterator.h>
#include <bmqc_twokeyhashmap.h>
#include <bmqimp_event.h>
#include <bmqimp_eventqueue.h>
#include <bmqimp_messagecorrelationidcontainer.h>
#include <bmqimp_queue.h>
#include <bmqimp_stat.h>
#include <bmqp_ackeventbuilder.h>
#include <bmqp_confirmeventbuilder.h>
#include <bmqp_confirmmessageiterator.h>
#include <bmqp_crc32c.h>
#include <bmqp_event.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_pusheventbuilder.h>
#include <bmqst_statcontext.h>
#include <bmqsys_time.h>
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlf_placeholder.h>
#include <bsl_iostream.h>
#include <bslma_default.h>
#include <bslmf_allocatorargt.h>
#include <bslmf_assert.h>
#include <bslmt_lockguard.h>
#include <bslmt_qlock.h>
#include <bsls_annotation.h>
#include <bsls_platform.h>

namespace BloombergLP {
namespace bmqa {

namespace {
const char k_LOG_CATEGORY[] = "BMQA.MOCKSESSION";

/// Two key hash map of uri and correlationIds to QueueId.
typedef bmqc::TwoKeyHashMap<bmqt::Uri,
                            bmqt::CorrelationId,
                            QueueId,
                            bsl::hash<bmqt::Uri>,
                            bsl::hash<bmqt::CorrelationId> >
    UriCorrIdToQueueMap;

/// Integer to keep track of the number of calls to `initialize` for the
/// `MockSession`.  If the value is non-zero, then it has already been
/// initialized, otherwise it can be initialized.  Each call to `initialize`
/// increments the value of this integer by one.  Each call to `shutdown`
/// decrements the value of this integer by one.  If the decremented value
/// is zero, then the `shutdown` on all required components is called.
int g_utilInitialized = 0;

/// Lock used to provide thread-safe protection for accessing the
/// `g_initialized` counter.
bslmt::QLock g_initLock = BSLMT_QLOCK_INITIALIZER;

/// First call to `initialize` assigns a BlobBufferFactory created on heap
/// to this `g_BufferFactory_p` which remains unchanged till destroyed.
bdlbb::BlobBufferFactory* g_bufferFactory_p = 0;

#ifdef BSLS_PLATFORM_CMP_CLANG
// Suppress "exit-time-destructor" warning on Clang by qualifying the
// static variable 'g_guidGenerator_sp' with Clang-specific attribute.
[[clang::no_destroy]]
#endif
bsl::shared_ptr<bmqp::MessageGUIDGenerator> g_guidGenerator_sp;
// First call to 'initialize' allocates a MessageGUIDGenerator created on
// heap and managed by this 'g_BufferFactory_sp' till destroyed.

/// First call to `initialize` assigns a Allocator to this `g_alloc_p`.
bslma::Allocator* g_alloc_p = 0;

BSLMF_ASSERT(sizeof(bsl::shared_ptr<bmqimp::Event>) == sizeof(Event));
// Ensure that that the types are matching

/// Default installed callback on the mocksession: log the specified
/// `description`, from the specified `file` and `line`, and then assert.
void defaultFailureCallback(const bslstl::StringRef& description,
                            const bslstl::StringRef& file,
                            int                      line)
{
    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);
    BALL_LOG_ERROR_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << "[ Description: '" << description << "'";
        if (file.length() != 0) {
            BALL_LOG_OUTPUT_STREAM << ", File: '" << file << ":" << line
                                   << "'";
        }
        BALL_LOG_OUTPUT_STREAM << " ]";
    }

    BSLS_ASSERT_OPT(false);
}

/// Utility method to cast the `UriCorrIdToQueueMap` held by the
/// `bsls::AlignedBuffer` (represented by the type `B`).
template <class B>
UriCorrIdToQueueMap& uriCorrIdToQueues(B& buffer)
{
    return reinterpret_cast<UriCorrIdToQueueMap&>(*(buffer.buffer()));
}

/// Utility method to cast the `UriCorrIdToQueueMap` held by the
/// `bsls::AlignedBuffer` (represented by the type `B`).
template <class B>
const UriCorrIdToQueueMap& uriCorrIdToQueues(const B& buffer)
{
    return reinterpret_cast<const UriCorrIdToQueueMap&>(*(buffer.buffer()));
}
}  // close unnamed namespace

#define BMQA_CHECK_ARG(METHOD, ARGNAME, EXPECTED, ACTUAL, CALL)               \
    do {                                                                      \
        if ((EXPECTED) != (ACTUAL)) {                                         \
            assertWrongArg((EXPECTED),                                        \
                           (ACTUAL),                                          \
                           (METHOD),                                          \
                           (ARGNAME),                                         \
                           (CALL));                                           \
        }                                                                     \
    } while (0)

// The following macro does not implement the standard 'do, while', used with
// regular macros because we expect the scope of the 'const Call&' to leak into
// the function calling it.  This allows for us to write the functions more
// cleanly and succinctly.
#define BMQA_CHECK_CALL(METHOD, RETURNBLOCK)                                  \
    if (d_calls.empty()) {                                                    \
        assertWrongCall((METHOD));                                            \
        RETURNBLOCK;                                                          \
    }                                                                         \
    const Call& call = d_calls.front();                                       \
    if (call.d_method != (METHOD)) {                                          \
        assertWrongCall((METHOD), call);                                      \
        RETURNBLOCK;                                                          \
    }

#define BMQA_ASSERT_AND_POP_FRONT()                                           \
    do {                                                                      \
        BSLS_ASSERT_OPT(!d_calls.empty());                                    \
        d_calls.pop_front();                                                  \
    } while (0)

#define BMQA_RETURN_ON_RC()                                                   \
    do {                                                                      \
        const int _rc = call.d_rc;                                            \
        if (_rc != 0) {                                                       \
            BSLS_ASSERT_OPT(!d_calls.empty());                                \
            d_calls.pop_front();                                              \
            return _rc;                                                       \
        }                                                                     \
    } while (0)

// --------------------------------
// class MockSessionUtil::AckParams
// --------------------------------

MockSessionUtil::AckParams::AckParams(const bmqt::AckResult::Enum status,
                                      const bmqt::CorrelationId& correlationId,
                                      const bmqt::MessageGUID&   guid,
                                      const bmqa::QueueId&       queueId)
: d_status(status)
, d_correlationId(correlationId)
, d_guid(guid)
, d_queueId(queueId)
{
    // NOTHING
}

// ----------------------------------------
// class MockSessionUtil::PushMessageParams
// ----------------------------------------

MockSessionUtil::PushMessageParams::PushMessageParams(
    const bdlbb::Blob&       payload,
    const bmqa::QueueId&     queueId,
    const bmqt::MessageGUID& guid,
    const MessageProperties& properties)
: d_payload(payload)
, d_queueId(queueId)
, d_guid(guid)
, d_properties(properties)
{
    // NOTHING
}

// ---------------------
// class MockSessionUtil
// ---------------------

// CLASS METHODS
Event MockSessionUtil::createSessionEvent(
    bmqt::SessionEventType::Enum sessionEventType,
    const bmqt::CorrelationId&   correlationId,
    const int                    errorCode,
    const bslstl::StringRef&     errorDescription,
    bslma::Allocator*            allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        sessionEventType != bmqt::SessionEventType::e_QUEUE_OPEN_RESULT &&
        sessionEventType != bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT &&
        sessionEventType != bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT &&
        sessionEventType != bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT);

    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    Event        event;
    EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(
        static_cast<Event&>(event));
    implPtr = EventImplSp(new (*alloc) bmqimp::Event(g_bufferFactory_p, alloc),
                          alloc);

    implPtr->configureAsSessionEvent(sessionEventType,
                                     errorCode,
                                     correlationId,
                                     errorDescription);
    return event;
}

Event MockSessionUtil::createQueueSessionEvent(
    bmqt::SessionEventType::Enum sessionEventType,
    QueueId*                     queueId,
    const bmqt::CorrelationId&   correlationId,
    int                          errorCode,
    const bslstl::StringRef&     errorDescription,
    bslma::Allocator*            allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        sessionEventType == bmqt::SessionEventType::e_QUEUE_OPEN_RESULT ||
        sessionEventType == bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT ||
        sessionEventType == bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT ||
        sessionEventType == bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT);

    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    Event        event;
    EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(
        static_cast<Event&>(event));
    implPtr = EventImplSp(new (*alloc) bmqimp::Event(g_bufferFactory_p, alloc),
                          alloc);

    implPtr->configureAsSessionEvent(sessionEventType,
                                     errorCode,
                                     correlationId,
                                     errorDescription);

    QueueImplSp& impQueue = reinterpret_cast<QueueImplSp&>(*queueId);

    implPtr->insertQueue(impQueue);

    return event;
}

Event MockSessionUtil::createAckEvent(const bsl::vector<AckParams>& acks,
                                      bdlbb::BlobBufferFactory* bufferFactory,
                                      bslma::Allocator*         allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!acks.empty());
    BSLS_ASSERT_SAFE(bufferFactory);

    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    Event        event;
    EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(
        static_cast<Event&>(event));
    implPtr = EventImplSp(new (*alloc) bmqimp::Event(g_bufferFactory_p, alloc),
                          alloc);

    // TODO: deprecate `createAckEvent` with bufferFactory arg and introduce
    // another function with BlobSpPool arg.
    bmqa::Session::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(bufferFactory, allocator));

    bmqp::AckEventBuilder ackBuilder(&blobSpPool, alloc);
    for (size_t i = 0; i != acks.size(); ++i) {
        const AckParams&   params   = acks[i];
        const QueueImplSp& impQueue = reinterpret_cast<const QueueImplSp&>(
            params.d_queueId);

        ackBuilder.appendMessage(
            bmqp::ProtocolUtil::ackResultToCode(params.d_status),
            0,  // Don't care about corrId in raw event
            params.d_guid,
            impQueue->id());
        implPtr->insertQueue(impQueue);
    }

    implPtr->configureAsMessageEvent(
        bmqp::Event(&ackBuilder.blob(), alloc, true));
    for (size_t i = 0; i != acks.size(); ++i) {
        implPtr->addCorrelationId(acks[i].d_correlationId);
    }

    return event;
}

Event MockSessionUtil::createPushEvent(
    const bsl::vector<PushMessageParams>& pushEventParams,
    bdlbb::BlobBufferFactory*             bufferFactory,
    bslma::Allocator*                     allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!pushEventParams.empty());

    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    Event        event;
    EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(event);
    implPtr = EventImplSp(new (*alloc) bmqimp::Event(g_bufferFactory_p, alloc),
                          alloc);

    // TODO: deprecate `createPushEvent` with bufferFactory arg and introduce
    // another function with BlobSpPool arg.
    bmqa::Session::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(bufferFactory, allocator));

    bmqp::PushEventBuilder pushBuilder(&blobSpPool, alloc);

    for (size_t i = 0; i != pushEventParams.size(); ++i) {
        const QueueImplSp& queueImplPtr = reinterpret_cast<const QueueImplSp&>(
            static_cast<const QueueId&>(pushEventParams[i].d_queueId));

        const PushMessageParams& pushMessageParams = pushEventParams[i];

        // By default, no flags are set; i.e. 0.

        bdlbb::Blob                 combinedBlob;
        bmqp::MessagePropertiesInfo logic;

        if (pushMessageParams.d_properties.numProperties() > 0) {
            combinedBlob = pushMessageParams.d_properties.streamOut(
                bufferFactory);
            logic = bmqp::MessagePropertiesInfo::makeNoSchema();
            // Use the same old style as the 'streamOut' above.
        }

        bdlbb::BlobUtil::append(&combinedBlob, pushMessageParams.d_payload);

        pushBuilder.packMessage(combinedBlob,
                                queueImplPtr->id(),
                                pushMessageParams.d_guid,
                                0,
                                bmqt::CompressionAlgorithmType::e_NONE,
                                logic);
        implPtr->insertQueue(bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID,
                             queueImplPtr);
        implPtr->addCorrelationId(bmqt::CorrelationId());
    }

    bmqp::Event bmqpEvent(&pushBuilder.blob(), alloc, true);
    implPtr->configureAsMessageEvent(bmqpEvent);

    return event;
}

OpenQueueStatus
MockSessionUtil::createOpenQueueStatus(const QueueId&              queueId,
                                       bmqt::OpenQueueResult::Enum statusCode,
                                       const bsl::string& errorDescription,
                                       bslma::Allocator*  allocator)
{
    return OpenQueueStatus(queueId,
                           statusCode,
                           errorDescription,
                           bslma::Default::allocator(allocator));
}

ConfigureQueueStatus MockSessionUtil::createConfigureQueueStatus(
    const QueueId&                   queueId,
    bmqt::ConfigureQueueResult::Enum statusCode,
    const bsl::string&               errorDescription,
    bslma::Allocator*                allocator)
{
    return ConfigureQueueStatus(queueId,
                                statusCode,
                                errorDescription,
                                bslma::Default::allocator(allocator));
}

CloseQueueStatus MockSessionUtil::createCloseQueueStatus(
    const QueueId&               queueId,
    bmqt::CloseQueueResult::Enum statusCode,
    const bsl::string&           errorDescription,
    bslma::Allocator*            allocator)
{
    return CloseQueueStatus(queueId,
                            statusCode,
                            errorDescription,
                            bslma::Default::allocator(allocator));
}

// -----------------------
// class MockSession::Call
// -----------------------

MockSession::Call::Call(Method method, bslma::Allocator* allocator)
: d_rc(0)
, d_method(method)
, d_line(-1)
, d_file(allocator)
, d_uri(allocator)
, d_flags(0)
, d_queueOptions(allocator)
, d_timeout()
, d_openQueueCallback(bsl::allocator_arg, allocator)
, d_configureQueueCallback(bsl::allocator_arg, allocator)
, d_closeQueueCallback(bsl::allocator_arg, allocator)
, d_openQueueResult(allocator)
, d_configureQueueResult(allocator)
, d_closeQueueResult(allocator)
, d_emittedEvents(allocator)
, d_returnEvent()
, d_messageEvent()
, d_cookie()
, d_allocator_p(allocator)
{
    // NOTHING
}

MockSession::Call::Call(const Call& other, bslma::Allocator* allocator)
: d_rc(other.d_rc)
, d_method(other.d_method)
, d_line(other.d_line)
, d_file(other.d_file, allocator)
, d_uri(other.d_uri, allocator)
, d_flags(other.d_flags)
, d_queueOptions(other.d_queueOptions, allocator)
, d_timeout(other.d_timeout)
, d_openQueueCallback(bsl::allocator_arg, allocator, other.d_openQueueCallback)
, d_configureQueueCallback(bsl::allocator_arg,
                           allocator,
                           other.d_configureQueueCallback)
, d_closeQueueCallback(bsl::allocator_arg,
                       allocator,
                       other.d_closeQueueCallback)
, d_openQueueResult(other.d_openQueueResult)
, d_configureQueueResult(other.d_configureQueueResult, allocator)
, d_closeQueueResult(other.d_closeQueueResult, allocator)
, d_emittedEvents(other.d_emittedEvents, allocator)
, d_returnEvent(other.d_returnEvent)
, d_messageEvent(other.d_messageEvent)
, d_cookie(other.d_cookie)
, d_allocator_p(allocator)
{
    // NOTHING
}

MockSession::Call& MockSession::Call::fromLocation(const char* file, int line)
{
    d_file.assign(file);
    d_line = line;
    return *this;
}

MockSession::Call& MockSession::Call::returning(int rc)
{
    d_rc = rc;
    return *this;
}

MockSession::Call&
MockSession::Call::returning(const bmqa::OpenQueueStatus& result)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_method = e_OPEN_QUEUE_SYNC);

    d_openQueueResult = result;
    return *this;
}

MockSession::Call&
MockSession::Call::returning(const bmqa::ConfigureQueueStatus& result)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_method = e_CONFIGURE_QUEUE_SYNC);

    d_configureQueueResult = result;
    return *this;
}

MockSession::Call&
MockSession::Call::returning(const bmqa::CloseQueueStatus& result)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_method = e_CLOSE_QUEUE_SYNC);

    d_closeQueueResult = result;
    return *this;
}

MockSession::Call& MockSession::Call::returning(const Event& event)
{
    d_returnEvent = event;
    return *this;
}

MockSession::Call& MockSession::Call::emitting(const Event& event)
{
    EventOrJob eventOrJob(event, d_allocator_p);
    d_emittedEvents.push_back(eventOrJob);
    return *this;
}

MockSession::Call&
MockSession::Call::emitting(const OpenQueueStatus& openQueueResult)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_method = e_OPEN_QUEUE_ASYNC_CALLBACK);
    BSLS_ASSERT_SAFE(d_openQueueCallback);

    d_openQueueResult = openQueueResult;

    const CallbackFn callbackFn = bdlf::BindUtil::bindS(d_allocator_p,
                                                        d_openQueueCallback,
                                                        openQueueResult);
    bmqa::QueueId    queueId    = openQueueResult.queueId();
    QueueImplSp      queue = reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(
        queueId);

    Job job;
    job.d_callback = callbackFn;
    job.d_queue    = queue;
    job.d_type     = bmqt::SessionEventType::e_QUEUE_OPEN_RESULT;
    job.d_status   = openQueueResult.result();

    EventOrJob eventOrJob(job, d_allocator_p);
    d_emittedEvents.push_back(eventOrJob);

    return *this;
}

MockSession::Call&
MockSession::Call::emitting(const ConfigureQueueStatus& configureQueueResult)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_method = e_CONFIGURE_QUEUE_ASYNC_CALLBACK);
    BSLS_ASSERT_SAFE(d_configureQueueCallback);

    d_configureQueueResult = configureQueueResult;

    const CallbackFn callbackFn = bdlf::BindUtil::bindS(
        d_allocator_p,
        d_configureQueueCallback,
        configureQueueResult);

    bmqa::QueueId queueId = configureQueueResult.queueId();
    QueueImplSp   queue   = reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(
        queueId);

    Job job;
    job.d_callback = callbackFn;
    job.d_queue    = queue;
    job.d_type     = bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT;
    job.d_status   = configureQueueResult.result();

    EventOrJob eventOrJob(job, d_allocator_p);
    d_emittedEvents.push_back(eventOrJob);

    return *this;
}

MockSession::Call&
MockSession::Call::emitting(const CloseQueueStatus& closeQueueResult)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_method = e_CLOSE_QUEUE_ASYNC_CALLBACK);
    BSLS_ASSERT_SAFE(d_closeQueueCallback);

    d_closeQueueResult = closeQueueResult;

    const CallbackFn callbackFn = bdlf::BindUtil::bindS(d_allocator_p,
                                                        d_closeQueueCallback,
                                                        closeQueueResult);

    bmqa::QueueId queueId = closeQueueResult.queueId();
    QueueImplSp   queue   = reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(
        queueId);

    Job job;
    job.d_callback = callbackFn;
    job.d_queue    = queue;
    job.d_type     = bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT;
    job.d_status   = closeQueueResult.result();

    EventOrJob eventOrJob(job, d_allocator_p);
    d_emittedEvents.push_back(eventOrJob);

    return *this;
}

const char* MockSession::Call::methodName() const
{
    return MockSession::toAscii(d_method);
}

// -----------------
// class MockSession
// -----------------

void MockSession::initialize(bslma::Allocator* allocator)
{
    bslmt::QLockGuard qlockGuard(&g_initLock);  // LOCK

    ++g_utilInitialized;
    if (g_utilInitialized > 1) {
        return;  // RETURN
    }

    g_alloc_p = bslma::Default::globalAllocator(allocator);

    bmqsys::Time::initialize(g_alloc_p);
    bmqp::Crc32c::initialize();
    bmqp::ProtocolUtil::initialize(g_alloc_p);
    bmqt::UriParser::initialize(g_alloc_p);

    g_bufferFactory_p = new (*g_alloc_p)
        bdlbb::PooledBlobBufferFactory(1024, g_alloc_p);
    g_guidGenerator_sp.createInplace(g_alloc_p, 0);
}

void MockSession::shutdown()
{
    bslmt::QLockGuard qlockGuard(&g_initLock);  // LOCK

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_utilInitialized > 0 && "Not initialized");

    if (--g_utilInitialized != 0) {
        return;  // RETURN
    }

    bmqt::UriParser::shutdown();
    bmqp::ProtocolUtil::shutdown();
    bmqsys::Time::shutdown();
    g_alloc_p->deleteObject(g_bufferFactory_p);
    g_guidGenerator_sp.reset();
}

const char* MockSession::toAscii(const Method method)
{
    switch (method) {
    case e_START: return "start()";
    case e_START_ASYNC: return "startAsync()";
    case e_STOP: return "stop()";
    case e_STOP_ASYNC: return "stopAsync()";
    case e_FINALIZE_STOP: return "finalizeStop()";
    case e_OPEN_QUEUE:
        return "int openQueue(QueueId                   *queueId,"
               "const bmqt::Uri&           uri,"
               "bsls::Types::Uint64        flags,"
               "const bmqt::QueueOptions&  options,"
               "const bsls::TimeInterval&  timeout)";
    case e_OPEN_QUEUE_SYNC:
        return "bmqa::OpenQueueStatus openQueueSync("
               "QueueId                   *queueId,"
               "const bmqt::Uri&           uri,"
               "bsls::Types::Uint64        flags,"
               "const bmqt::QueueOptions&  options,"
               "const bsls::TimeInterval&  timeout)";
    case e_OPEN_QUEUE_ASYNC:
        return "int openQueueAsync(QueueId                   *queueId,"
               "const bmqt::Uri&           uri,"
               "bsls::Types::Uint64        flags,"
               "const bmqt::QueueOptions&  options,"
               "const bsls::TimeInterval&  timeout)";
    case e_OPEN_QUEUE_ASYNC_CALLBACK:
        return "void openQueueAsync(const bmqt::Uri&          uri,"
               "bsls::Types::Uint64       flags,"
               "const OpenQueueCallback&  callback,"
               "const bmqt::QueueOptions& options,"
               "const bsls::TimeInterval& timeout)";
    case e_CONFIGURE_QUEUE:
        return "int openQueue(QueueId                   *queueId,"
               "const bmqt::QueueOptions&  options,"
               "const bsls::TimeInterval&  timeout)";
    case e_CONFIGURE_QUEUE_SYNC:
        return "bmqa::ConfigureQueueStatus configureQueueSync("
               "QueueId                   *queueId,"
               "const bmqt::QueueOptions&  options,"
               "const bsls::TimeInterval&  timeout)";
    case e_CONFIGURE_QUEUE_ASYNC:
        return "int configureQueueAsync(QueueId                   *queueId,"
               "const bmqt::QueueOptions&  options,"
               "const bsls::TimeInterval&  timeout)";
    case e_CONFIGURE_QUEUE_ASYNC_CALLBACK:
        return "void configureQueueAsync(QueueId                       "
               "*queueId,"
               "const bmqt::QueueOptions&      options,"
               "const ConfigureQueueCallback&  callback,"
               "const bsls::TimeInterval&      timeout)";
    case e_CLOSE_QUEUE:
        return "int closeQueue(QueueId                   *queueId,"
               "const bsls::TimeInterval&  timeout)";
    case e_CLOSE_QUEUE_SYNC:
        return "bmqa::CloseQueueStatus closeQueueSync("
               "QueueId                   *queueId,"
               "const bsls::TimeInterval&  timeout)";
    case e_CLOSE_QUEUE_ASYNC:
        return "int closeQueueAsync(QueueId                   *queueId,"
               "const bsls::TimeInterval&  timeout)";
    case e_CLOSE_QUEUE_ASYNC_CALLBACK:
        return "void closeQueueAsync(QueueId                   *queueId,"
               "const CloseQueueCallback&  callback,"
               "const bsls::TimeInterval&  timeout)";
    case e_NEXT_EVENT:
        return "Event nextEvent(const bsls::TimeInterval& timeout)";
    case e_POST: return "int post(const MessageEvent& messageEvent)";
    case e_CONFIRM_MESSAGE:
        return "int confirmMessage(const MessageConfirmationCookie& cookie)";
    case e_CONFIRM_MESSAGES:
        return "int confirmMessages(ConfirmEventBuilder *builder)";
    }

    return "UNKNOWN";
}

void MockSession::initializeStats()
{
    bmqst::StatValue::SnapshotLocation start;
    bmqst::StatValue::SnapshotLocation end;
    start.setLevel(0).setIndex(0);
    end.setLevel(0).setIndex(1);
    bmqimp::QueueStatsUtil::initializeStats(d_queuesStats_sp.get(),
                                            d_rootStatContext_mp.get(),
                                            start,
                                            end,
                                            d_allocator_p);
}

void MockSession::openQueueImp(QueueId*                  queueId,
                               const bmqt::QueueOptions& options,
                               const bmqt::Uri&          uri,
                               bsls::Types::Uint64       flags,
                               bool                      async)
{
    // Cast the supplied queue
    QueueImplSp& queueImpl = reinterpret_cast<QueueImplSp&>(*queueId);

    // In case queueId was uninitialized the impl ptr will be 0x0
    BSLS_ASSERT_SAFE(queueImpl && "QueueId not set");

    if (queueImpl->state() == bmqimp::QueueState::e_OPENED) {
        return;  // RETURN
    }

    queueImpl->setId(++d_lastQueueId);
    queueImpl->setUri(uri);
    queueImpl->setFlags(flags);
    queueImpl->setOptions(options);

    if (async) {
        queueImpl->setState(bmqimp::QueueState::e_OPENING_OPN);
    }
    else {
        queueImpl->setState(bmqimp::QueueState::e_OPENED);
        queueImpl->registerStatContext(
            d_queuesStats_sp->d_statContext_mp.get());
    }

    // Finally queue is set up so add it to internal map, ignoring error code.
    bmqt::CorrelationId corrId = queueId->correlationId();
    uriCorrIdToQueues(d_twoKeyHashMapBuffer).insert(uri, corrId, *queueId);
}

void MockSession::processIfQueueEvent(Event* event)
{
    const bmqt::SessionEventType::Enum eventType =
        event->sessionEvent().type();
    if ((eventType != bmqt::SessionEventType::e_QUEUE_OPEN_RESULT) &&
        (eventType != bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT)) {
        return;  // RETURN
    }

    EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(*event);

    const bmqimp::Event::QueuesMap& queues = implPtr->queues();
    BSLS_ASSERT_SAFE(queues.size() == 1U);

    // At this point, openQueue should have been called, so we can look for the
    // queue in the TwoKey hashmap.
    QueueImplSp queueImpl = queues.begin()->second;

    UriCorrIdToQueueMap::iterator qit =
        uriCorrIdToQueues(d_twoKeyHashMapBuffer).findByKey1(queueImpl->uri());

    QueueId   internalQueue = qit->value();
    const int status        = event->sessionEvent().statusCode();
    queueImpl->setCorrelationId(internalQueue.correlationId());
    if (eventType == bmqt::SessionEventType::e_QUEUE_OPEN_RESULT) {
        if (status == 0) {
            queueImpl->setState(bmqimp::QueueState::e_OPENED);
            queueImpl->registerStatContext(
                d_queuesStats_sp->d_statContext_mp.get());
        }
        else {
            // We should erase the queue from our internal map since it is
            // marked closed.
            queueImpl->setState(bmqimp::QueueState::e_CLOSED);
            uriCorrIdToQueues(d_twoKeyHashMapBuffer)
                .eraseByKey1(queueImpl->uri());
        }
    }
    else if (eventType == bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT) {
        queueImpl->setState(bmqimp::QueueState::e_CLOSED);

        // At this point we should be removing the queue from our internal map
        // as well since we have closed it.  This will enable subsequent calls
        // for this 'uri' to 'openQueue' to succeed and 'getQueueId' to fail.
        uriCorrIdToQueues(d_twoKeyHashMapBuffer).eraseByKey1(queueImpl->uri());
    }
}

void MockSession::processIfQueueJob(Job* job)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(job && "'job' must be specified.");

    const bmqt::SessionEventType::Enum eventType = job->d_type;
    if ((eventType != bmqt::SessionEventType::e_QUEUE_OPEN_RESULT) &&
        (eventType != bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT)) {
        return;  // RETURN
    }

    // At this point, openQueue should have been called, so we can look for the
    // queue in the TwoKey hashmap.
    QueueImplSp queueImpl = job->d_queue;

    UriCorrIdToQueueMap::iterator qit =
        uriCorrIdToQueues(d_twoKeyHashMapBuffer).findByKey1(queueImpl->uri());

    QueueId   internalQueue = qit->value();
    const int status        = job->d_status;
    queueImpl->setCorrelationId(internalQueue.correlationId());
    if (eventType == bmqt::SessionEventType::e_QUEUE_OPEN_RESULT) {
        if (status == bmqt::OpenQueueResult::e_SUCCESS) {
            queueImpl->setState(bmqimp::QueueState::e_OPENED);
        }
        else {
            // We should erase the queue from our internal map since it is
            // marked closed.
            queueImpl->setState(bmqimp::QueueState::e_CLOSED);
            uriCorrIdToQueues(d_twoKeyHashMapBuffer)
                .eraseByKey1(queueImpl->uri());
        }
    }
    else if (eventType == bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT) {
        queueImpl->setState(bmqimp::QueueState::e_CLOSED);

        // At this point we should be removing the queue from our internal map
        // as well since we have closed it.  This will enable subsequent calls
        // for this 'uri' to 'openQueue' to succeed and 'getQueueId' to fail.
        uriCorrIdToQueues(d_twoKeyHashMapBuffer).eraseByKey1(queueImpl->uri());
    }
}

void MockSession::processIfPushEvent(const Event& event)
{
    if (!event.isMessageEvent() ||
        event.messageEvent().type() != bmqt::MessageEventType::e_PUSH) {
        return;  // RETURN
    }

    // In case of a push message, build internal list of unconfirmed messages.
    bmqa::MessageIterator mIter = event.messageEvent().messageIterator();
    while (mIter.nextMessage()) {
        d_unconfirmedGUIDs.insert(mIter.message().messageGUID());
    }
}

void MockSession::assertWrongCall(const Method method) const
{
    bmqu::MemOutStream mos(d_allocator_p);
    mos << "No expected calls but received call to '"
        << MockSession::toAscii(method) << "'" << bsl::ends;
    d_failureCb(mos.str().data(), "", 0);
}

void MockSession::assertWrongCall(const Method method,
                                  const Call&  expectedCall) const
{
    bmqu::MemOutStream mos(d_allocator_p);
    mos << "Expected call to '" << expectedCall.methodName() << "' but got '"
        << MockSession::toAscii(method) << "' (" << expectedCall.d_file << ':'
        << expectedCall.d_line << ')' << bsl::ends;
    d_failureCb(mos.str().data(), "", 0);
}

template <class T, class U>
void MockSession::assertWrongArg(const T&     expected,
                                 const U&     actual,
                                 const Method method,
                                 const char*  arg,
                                 const Call&  call) const
{
    bmqu::MemOutStream mos(d_allocator_p);
    mos << "Bad value for argument '" << arg << "' in call to '"
        << toAscii(method) << "': expected '" << expected << "', got '"
        << actual << "'";

    if (call.d_file.length() != 0) {
        mos << " (" << call.d_file << ':' << call.d_line << ')';
    }

    mos << bsl::ends;
    d_failureCb(mos.str().data(), call.d_file.data(), call.d_line);
}

int MockSession::start(const bsls::TimeInterval& timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_START, { return 0; });
    BMQA_CHECK_ARG(e_START, "timeout", call.d_timeout, timeout, call);

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    const int rc = call.d_rc;
    BMQA_ASSERT_AND_POP_FRONT();
    return rc;
}

MockSession::MockSession(const bmqt::SessionOptions& options,
                         bslma::Allocator*           allocator)
: d_blobBufferFactory(1024, allocator)
, d_blobSpPool(
      bmqp::BlobPoolUtil::createBlobPool(&d_blobBufferFactory, allocator))
, d_eventHandler_mp(0)
, d_calls(allocator)
, d_eventsAndJobs(allocator)
, d_unconfirmedGUIDs(allocator)
, d_twoKeyHashMapBuffer()
, d_failureCb(bdlf::BindUtil::bindS(bslma::Default::allocator(allocator),
                                    &defaultFailureCallback,
                                    bdlf::PlaceHolders::_1,
                                    bdlf::PlaceHolders::_2,
                                    bdlf::PlaceHolders::_3))
, d_lastQueueId(0)
, d_corrIdContainer_sp(new(*bslma::Default::allocator(allocator))
                           bmqimp::MessageCorrelationIdContainer(
                               bslma::Default::allocator(allocator)),
                       bslma::Default::allocator(allocator))
, d_postedEvents(bslma::Default::allocator(allocator))
, d_rootStatContext_mp(bslma::ManagedPtrUtil::makeManaged<bmqst::StatContext>(
      bmqst::StatContextConfiguration("MockSession", allocator),
      allocator))
, d_queuesStats_sp(new(*bslma::Default::allocator(allocator))
                       bmqimp::Stat(bslma::Default::allocator(allocator)),
                   bslma::Default::allocator(allocator))
, d_sessionOptions(options, allocator)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    BSLMF_ASSERT(k_MAX_SIZEOF_BMQC_TWOKEYHASHMAP >=
                 sizeof(UriCorrIdToQueueMap));
    // Compile-time assert to keep the hardcoded value of size of an object of
    // type 'bmqc::TwoKeyHashMap' in sync with its actual size.  We need to
    // hard code the size in 'bmqa_mocksession.h' because none of the 'bmqc'
    // headers can be included in 'bmqa' headers.  Note that we don't check
    // exact size, but 'enough' size, see comment of the constant in header.

    // Inplace-create the TwoKeyHashMap in the buffer
    new (d_twoKeyHashMapBuffer.buffer()) UriCorrIdToQueueMap(d_allocator_p);

    initialize(d_allocator_p);
    initializeStats();
}

MockSession::MockSession(bslma::ManagedPtr<SessionEventHandler> eventHandler,
                         const bmqt::SessionOptions&            options,
                         bslma::Allocator*                      allocator)
: d_blobBufferFactory(1024, allocator)
, d_blobSpPool(
      bmqp::BlobPoolUtil::createBlobPool(&d_blobBufferFactory, allocator))
, d_eventHandler_mp(eventHandler)
, d_calls(bslma::Default::allocator(allocator))
, d_eventsAndJobs(bslma::Default::allocator(allocator))
, d_unconfirmedGUIDs(bslma::Default::allocator(allocator))
, d_twoKeyHashMapBuffer()
, d_failureCb(bdlf::BindUtil::bindS(bslma::Default::allocator(allocator),
                                    &defaultFailureCallback,
                                    bdlf::PlaceHolders::_1,
                                    bdlf::PlaceHolders::_2,
                                    bdlf::PlaceHolders::_3))
, d_lastQueueId(0)
, d_corrIdContainer_sp(new(*bslma::Default::allocator(allocator))
                           bmqimp::MessageCorrelationIdContainer(
                               bslma::Default::allocator(allocator)),
                       bslma::Default::allocator(allocator))
, d_postedEvents(bslma::Default::allocator(allocator))
, d_rootStatContext_mp(bslma::ManagedPtrUtil::makeManaged<bmqst::StatContext>(
      bmqst::StatContextConfiguration("MockSession", allocator),
      allocator))
, d_queuesStats_sp(new(*bslma::Default::allocator(allocator))
                       bmqimp::Stat(bslma::Default::allocator(allocator)),
                   bslma::Default::allocator(allocator))
, d_sessionOptions(options, allocator)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    // Inplace-create the TwoKeyHashMap in the buffer
    new (d_twoKeyHashMapBuffer.buffer()) UriCorrIdToQueueMap(d_allocator_p);

    initialize(d_allocator_p);
    initializeStats();
}

MockSession::~MockSession()
{
    if (!d_calls.empty()) {
        bmqu::MemOutStream stream(d_allocator_p);
        stream << "Expected calls [";

        CallQueue::iterator cit = d_calls.begin();
        stream << cit->methodName();
        ++cit;
        for (; cit != d_calls.end(); ++cit) {
            stream << ", " << cit->methodName();
        }
        stream << "]";

        BALL_LOG_ERROR << stream.str();
        d_failureCb(stream.str().data(), "", 0);
    }

    uriCorrIdToQueues(d_twoKeyHashMapBuffer).clear();
    uriCorrIdToQueues(d_twoKeyHashMapBuffer)
        .bmqc::TwoKeyHashMap<
            bmqt::Uri,
            bmqt::CorrelationId,
            QueueId,
            bsl::hash<bmqt::Uri>,
            bsl::hash<bmqt::CorrelationId> >::TwoKeyHashMap ::~TwoKeyHashMap();
    // Above expression, particularly the 'TwoKeyHashMap::' before
    // '~TwoKeyHashMap()' is required if passing '-Wpedantic' flag in our
    // build, which is what we are doing when building with clang.

    shutdown();
}

MockSession::Call& MockSession::expect_start(const bsls::TimeInterval& timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_START);
    Call& call     = d_calls.back();
    call.d_timeout = timeout;

    return call;
}

MockSession::Call&
MockSession::expect_startAsync(const bsls::TimeInterval& timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_START_ASYNC);
    Call& call     = d_calls.back();
    call.d_timeout = timeout;

    return call;
}

MockSession::Call& MockSession::expect_stop()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_STOP);
    return d_calls.back();
}

MockSession::Call& MockSession::expect_stopAsync()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_STOP_ASYNC);
    return d_calls.back();
}

MockSession::Call& MockSession::expect_finalizeStop()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_FINALIZE_STOP);
    return d_calls.back();
}

MockSession::Call&
MockSession::expect_openQueue(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                              const bmqt::Uri&                uri,
                              bsls::Types::Uint64             flags,
                              const bmqt::QueueOptions&       options,
                              const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_OPEN_QUEUE);
    Call& call          = d_calls.back();
    call.d_uri          = uri;
    call.d_flags        = flags;
    call.d_queueOptions = options;
    call.d_timeout      = timeout;

    return call;
}

MockSession::Call&
MockSession::expect_openQueueSync(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                                  const bmqt::Uri&                uri,
                                  bsls::Types::Uint64             flags,
                                  const bmqt::QueueOptions&       options,
                                  const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_OPEN_QUEUE_SYNC);
    Call& call          = d_calls.back();
    call.d_uri          = uri;
    call.d_flags        = flags;
    call.d_queueOptions = options;
    call.d_timeout      = timeout;
    call.d_allocator_p  = d_allocator_p;

    return call;
}

MockSession::Call&
MockSession::expect_openQueueAsync(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                                   const bmqt::Uri&                uri,
                                   bsls::Types::Uint64             flags,
                                   const bmqt::QueueOptions&       options,
                                   const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_OPEN_QUEUE_ASYNC);
    Call& call          = d_calls.back();
    call.d_uri          = uri;
    call.d_flags        = flags;
    call.d_queueOptions = options;
    call.d_timeout      = timeout;

    return call;
}

MockSession::Call&
MockSession::expect_openQueueAsync(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                                   const bmqt::Uri&                uri,
                                   bsls::Types::Uint64             flags,
                                   const OpenQueueCallback&        callback,
                                   const bmqt::QueueOptions&       options,
                                   const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);

    d_calls.emplace_back(e_OPEN_QUEUE_ASYNC_CALLBACK);
    Call& call               = d_calls.back();
    call.d_uri               = uri;
    call.d_flags             = flags;
    call.d_queueOptions      = options;
    call.d_openQueueCallback = callback;
    call.d_timeout           = timeout;
    call.d_allocator_p       = d_allocator_p;

    return call;
}

MockSession::Call&
MockSession::expect_configureQueue(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                                   const bmqt::QueueOptions&       options,
                                   const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_CONFIGURE_QUEUE);
    Call& call          = d_calls.back();
    call.d_queueOptions = options;
    call.d_timeout      = timeout;

    return call;
}

MockSession::Call&
MockSession::expect_configureQueueSync(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                                       const bmqt::QueueOptions&       options,
                                       const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_CONFIGURE_QUEUE_SYNC);
    Call& call          = d_calls.back();
    call.d_queueOptions = options;
    call.d_timeout      = timeout;
    call.d_allocator_p  = d_allocator_p;

    return call;
}

MockSession::Call& MockSession::expect_configureQueueAsync(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    const bmqt::QueueOptions&       options,
    const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_CONFIGURE_QUEUE_ASYNC);
    Call& call          = d_calls.back();
    call.d_queueOptions = options;
    call.d_timeout      = timeout;

    return call;
}

MockSession::Call& MockSession::expect_configureQueueAsync(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    const bmqt::QueueOptions&       options,
    const ConfigureQueueCallback&   callback,
    const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);

    d_calls.emplace_back(e_CONFIGURE_QUEUE_ASYNC_CALLBACK);
    Call& call                    = d_calls.back();
    call.d_queueOptions           = options;
    call.d_configureQueueCallback = callback;
    call.d_timeout                = timeout;
    call.d_allocator_p            = d_allocator_p;

    return call;
}

MockSession::Call&
MockSession::expect_closeQueue(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                               const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_CLOSE_QUEUE);
    Call& call     = d_calls.back();
    call.d_timeout = timeout;

    return call;
}

MockSession::Call&
MockSession::expect_closeQueueSync(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                                   const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_CLOSE_QUEUE_SYNC);
    Call& call         = d_calls.back();
    call.d_timeout     = timeout;
    call.d_allocator_p = d_allocator_p;

    return call;
}

MockSession::Call&
MockSession::expect_closeQueueAsync(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                                    const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_CLOSE_QUEUE_ASYNC);
    Call& call     = d_calls.back();
    call.d_timeout = timeout;

    return d_calls.back();
}

MockSession::Call&
MockSession::expect_closeQueueAsync(BSLS_ANNOTATION_UNUSED QueueId* queueId,
                                    const CloseQueueCallback&       callback,
                                    const bsls::TimeInterval&       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);

    d_calls.emplace_back(e_CLOSE_QUEUE_ASYNC_CALLBACK);
    Call& call                = d_calls.back();
    call.d_closeQueueCallback = callback;
    call.d_timeout            = timeout;
    call.d_allocator_p        = d_allocator_p;

    return call;
}

MockSession::Call&
MockSession::expect_nextEvent(const bsls::TimeInterval& timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_NEXT_EVENT);
    Call& call     = d_calls.back();
    call.d_timeout = timeout;

    return call;
}

MockSession::Call& MockSession::expect_post(const MessageEvent& messageEvent)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_POST);
    Call& call          = d_calls.back();
    call.d_messageEvent = messageEvent;

    return call;
}

MockSession::Call&
MockSession::expect_confirmMessage(const bmqa::Message& message)
{
    return expect_confirmMessage(message.confirmationCookie());
}

MockSession::Call& MockSession::expect_confirmMessage(
    const bmqa::MessageConfirmationCookie& cookie)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_CONFIRM_MESSAGE);
    Call& call    = d_calls.back();
    call.d_cookie = cookie;

    return call;
}

MockSession::Call& MockSession::expect_confirmMessages(
    BSLS_ANNOTATION_UNUSED ConfirmEventBuilder* builder)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    d_calls.emplace_back(e_CONFIRM_MESSAGES);
    // We don't compare confirmEventBuilder ever so no need to add to call

    return d_calls.back();
}

void MockSession::enqueueEvent(const bmqa::Event& event)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    EventOrJob eventOrJob(event, d_allocator_p);
    d_eventsAndJobs.push_back(eventOrJob);
}

bool MockSession::emitEvent(int numEvents)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_eventHandler_mp &&
                    "MockSession was not created in asynchronous mode");
    BSLS_ASSERT_OPT(numEvents > 0 && "Bad parameter 'numEvents'");

    // Cannot acquire lock AND call the event handler.  This is because the
    // event handler could potentially call a method on 'MockSession' which
    // would acquire the already acquired lock and lead to a deadlock.  To
    // mitigate this we make a copy of the existing events deque up to
    // 'numEvents' and erase under lock.  This will allow multi-threaded unit
    // tests to append to the events deque while emitting events.

    bsl::deque<EventOrJob> eventsAndJobsCopy(d_allocator_p);
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED
        eventsAndJobsCopy.insert(eventsAndJobsCopy.begin(),
                                 d_eventsAndJobs.begin(),
                                 d_eventsAndJobs.begin() + numEvents);
        d_eventsAndJobs.erase(d_eventsAndJobs.begin(),
                              d_eventsAndJobs.begin() + numEvents);

        if (eventsAndJobsCopy.empty()) {
            return false;  // RETURN
        }
    }  // UNLOCKED

    while (!eventsAndJobsCopy.empty()) {
        EventOrJob& eventOrJob = eventsAndJobsCopy.front();
        if (eventOrJob.is<Event>()) {
            Event& event = eventOrJob.the<Event>();
            if (event.isSessionEvent()) {
                processIfQueueEvent(&event);
                d_eventHandler_mp->onSessionEvent(event.sessionEvent());
            }
            else if (event.isMessageEvent()) {
                processIfPushEvent(event);
                d_eventHandler_mp->onMessageEvent(event.messageEvent());
            }
            else {
                BSLS_ASSERT(false && "Unknown event type");
            }
        }
        else if (eventOrJob.is<Job>()) {
            // We have a job that is an async callback with a bound result
            // object
            Job& job = eventOrJob.the<Job>();
            processIfQueueJob(&job);
            job.d_callback();
        }
        eventsAndJobsCopy.pop_front();
    }

    return true;
}

int MockSession::startAsync(const bsls::TimeInterval& timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_START_ASYNC, { return 0; });
    BMQA_CHECK_ARG(e_START, "timeout", call.d_timeout, timeout, call);

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    const int rc = call.d_rc;
    BMQA_ASSERT_AND_POP_FRONT();
    return rc;
}

void MockSession::stop()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_STOP, {});

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    // Reset all queue's state to 'closed' to mimic the real implementation
    UriCorrIdToQueueMap& queueMap = uriCorrIdToQueues(d_twoKeyHashMapBuffer);
    UriCorrIdToQueueMap::iterator qIt = queueMap.begin();
    while (qIt != queueMap.end()) {
        QueueImplSp& queueImpl = reinterpret_cast<QueueImplSp&>(qIt->value());
        queueImpl->setState(bmqimp::QueueState::e_CLOSED);
        ++qIt;
    }

    BMQA_ASSERT_AND_POP_FRONT();
}

void MockSession::stopAsync()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_STOP_ASYNC, {});

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    // Reset all queue's state to 'closed' to mimic the real implementation
    UriCorrIdToQueueMap& queueMap = uriCorrIdToQueues(d_twoKeyHashMapBuffer);
    UriCorrIdToQueueMap::iterator qIt = queueMap.begin();
    while (qIt != queueMap.end()) {
        QueueImplSp& queueImpl = reinterpret_cast<QueueImplSp&>(qIt->value());
        queueImpl->setState(bmqimp::QueueState::e_CLOSED);
        ++qIt;
    }

    BMQA_ASSERT_AND_POP_FRONT();
}

void MockSession::finalizeStop()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_FINALIZE_STOP, {});

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
}

void MockSession::loadMessageEventBuilder(MessageEventBuilder* builder)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(builder);

    MessageEventBuilder& builderRef = *builder;

    // Get MessageEventBuilderImpl from MessageEventBuilder
    MessageEventBuilderImpl& builderImplRef =
        reinterpret_cast<MessageEventBuilderImpl&>(builderRef);

    builderImplRef.d_guidGenerator_sp = g_guidGenerator_sp;

    // Get bmqimp::Event sharedptr from MessageEventBuilderImpl
    bsl::shared_ptr<bmqimp::Event>& eventImplSpRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(
            builderImplRef.d_msgEvent);

    eventImplSpRef.createInplace(d_allocator_p,
                                 g_bufferFactory_p,
                                 d_allocator_p);

    eventImplSpRef->configureAsMessageEvent(&d_blobSpPool);
    eventImplSpRef->setMessageCorrelationIdContainer(
        d_corrIdContainer_sp.get());
}

void MockSession::loadConfirmEventBuilder(ConfirmEventBuilder* builder)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(builder);

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
        bmqp::ConfirmEventBuilder(&d_blobSpPool, d_allocator_p);

    builderImplRef.d_builder_p = reinterpret_cast<bmqp::ConfirmEventBuilder*>(
        builderImplRef.d_buffer.buffer());
}

void MockSession::loadMessageProperties(MessageProperties* buffer)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(buffer);

    *buffer = MessageProperties();
}

int MockSession::getQueueId(QueueId* queueId, const bmqt::Uri& uri)
{
    UriCorrIdToQueueMap::const_iterator iter =
        uriCorrIdToQueues(d_twoKeyHashMapBuffer).findByKey1(uri);

    if (iter == uriCorrIdToQueues(d_twoKeyHashMapBuffer).end()) {
        return bmqt::GenericResult::e_UNKNOWN;  // RETURN
    }

    const bsl::shared_ptr<bmqimp::Queue>& queueImplSpRef =
        reinterpret_cast<const bsl::shared_ptr<bmqimp::Queue>&>(iter->value());
    if (queueImplSpRef->state() != bmqimp::QueueState::e_OPENED) {
        return bmqt::GenericResult::e_REFUSED;  // RETURN
    }

    *queueId = iter->value();

    return 0;
}

int MockSession::getQueueId(QueueId*                   queueId,
                            const bmqt::CorrelationId& correlationId)
{
    UriCorrIdToQueueMap::const_iterator iter =
        uriCorrIdToQueues(d_twoKeyHashMapBuffer).findByKey2(correlationId);

    if (iter == uriCorrIdToQueues(d_twoKeyHashMapBuffer).end()) {
        return bmqt::GenericResult::e_UNKNOWN;  // RETURN
    }

    *queueId = iter->value();
    return 0;
}

int MockSession::openQueue(QueueId*                  queueId,
                           const bmqt::Uri&          uri,
                           bsls::Types::Uint64       flags,
                           const bmqt::QueueOptions& options,
                           const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(queueId);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_OPEN_QUEUE, { return 0; });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_OPEN_QUEUE, "uri", call.d_uri, uri, call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE, "flags", call.d_flags, flags, call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE,
                   "options",
                   call.d_queueOptions,
                   options,
                   call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE, "timeout", call.d_timeout, timeout, call);
    BMQA_RETURN_ON_RC();

    openQueueImp(queueId, options, uri, flags, false);  // openqueue is sync

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
    return 0;
}

OpenQueueStatus MockSession::openQueueSync(QueueId*                  queueId,
                                           const bmqt::Uri&          uri,
                                           bsls::Types::Uint64       flags,
                                           const bmqt::QueueOptions& options,
                                           const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(queueId && "'queueId' not provided");

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_OPEN_QUEUE_SYNC, {
        return OpenQueueStatus(bmqa::QueueId(),
                               bmqt::OpenQueueResult::e_INVALID_ARGUMENT,
                               "Call to 'openQueueSync' was not expected.");
    });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_OPEN_QUEUE_SYNC, "uri", call.d_uri, uri, call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE_SYNC, "flags", call.d_flags, flags, call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE_SYNC,
                   "options",
                   call.d_queueOptions,
                   options,
                   call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE_SYNC,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);

    // Check and return on error rc
    const bmqa::OpenQueueStatus _result = call.d_openQueueResult;
    if (!_result) {
        BSLS_ASSERT_OPT(!d_calls.empty());
        d_calls.pop_front();
        return _result;  // RETURN
    }

    openQueueImp(queueId, options, uri, flags, false);  // openqueue is sync

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
    return _result;
}

int MockSession::openQueueAsync(QueueId*                  queueId,
                                const bmqt::Uri&          uri,
                                bsls::Types::Uint64       flags,
                                const bmqt::QueueOptions& options,
                                const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(queueId);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_OPEN_QUEUE_ASYNC, { return 0; });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_OPEN_QUEUE_ASYNC, "uri", call.d_uri, uri, call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE_ASYNC, "flags", call.d_flags, flags, call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE_ASYNC,
                   "options",
                   call.d_queueOptions,
                   options,
                   call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE_ASYNC,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);
    BMQA_RETURN_ON_RC();

    openQueueImp(queueId, options, uri, flags, true);  // openqueue is async

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
    return 0;
}

void MockSession::openQueueAsync(
    BSLS_ANNOTATION_UNUSED QueueId*                 queueId,
    const bmqt::Uri&                                uri,
    bsls::Types::Uint64                             flags,
    BSLS_ANNOTATION_UNUSED const OpenQueueCallback& callback,
    const bmqt::QueueOptions&                       options,
    const bsls::TimeInterval&                       timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_OPEN_QUEUE_ASYNC_CALLBACK, { return; });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_OPEN_QUEUE_ASYNC_CALLBACK, "uri", uri, call.d_uri, call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE_ASYNC_CALLBACK,
                   "flags",
                   flags,
                   call.d_flags,
                   call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE_ASYNC_CALLBACK,
                   "options",
                   call.d_queueOptions,
                   options,
                   call);
    BMQA_CHECK_ARG(e_OPEN_QUEUE_ASYNC_CALLBACK,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    bmqa::QueueId& queueIdRef = const_cast<bmqa::QueueId&>(
        call.d_openQueueResult.queueId());

    openQueueImp(&queueIdRef,
                 options,
                 uri,
                 flags,
                 true);  // openqueue is async

    BMQA_ASSERT_AND_POP_FRONT();
}

int MockSession::configureQueue(QueueId*                  queueId,
                                const bmqt::QueueOptions& options,
                                const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(queueId);

    (void)queueId;  // Compiler happiness in 'opt' build

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CONFIGURE_QUEUE, { return 0; });

    BMQA_CHECK_ARG(e_CONFIGURE_QUEUE,
                   "options",
                   call.d_queueOptions,
                   options,
                   call);
    BMQA_CHECK_ARG(e_CONFIGURE_QUEUE,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);
    BMQA_RETURN_ON_RC();

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
    return 0;
}

ConfigureQueueStatus
MockSession::configureQueueSync(QueueId*                  queueId,
                                const bmqt::QueueOptions& options,
                                const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(queueId && "'queueId' not provided");

    (void)queueId;  // Compiler happiness in 'opt' build

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CONFIGURE_QUEUE_SYNC, {
        return ConfigureQueueStatus(
            bmqa::QueueId(),
            bmqt::ConfigureQueueResult::e_INVALID_ARGUMENT,
            "Call to 'configureQueueSync' was not expected");
    });

    BMQA_CHECK_ARG(e_CONFIGURE_QUEUE_SYNC,
                   "options",
                   call.d_queueOptions,
                   options,
                   call);
    BMQA_CHECK_ARG(e_CONFIGURE_QUEUE_SYNC,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);

    // Check and return on error rc
    const bmqa::ConfigureQueueStatus _result = call.d_configureQueueResult;
    if (!_result) {
        BSLS_ASSERT_OPT(!d_calls.empty());
        d_calls.pop_front();
        return _result;  // RETURN
    }

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
    return _result;
}

int MockSession::configureQueueAsync(QueueId*                  queueId,
                                     const bmqt::QueueOptions& options,
                                     const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(queueId);

    (void)queueId;  // Compiler happiness in 'opt' build

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CONFIGURE_QUEUE_ASYNC, { return 0; });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_CONFIGURE_QUEUE_ASYNC,
                   "options",
                   call.d_queueOptions,
                   options,
                   call);
    BMQA_CHECK_ARG(e_CONFIGURE_QUEUE_ASYNC,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);
    BMQA_RETURN_ON_RC();

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();

    return 0;
}

void MockSession::configureQueueAsync(
    QueueId*                                             queueId,
    const bmqt::QueueOptions&                            options,
    BSLS_ANNOTATION_UNUSED const ConfigureQueueCallback& callback,
    const bsls::TimeInterval&                            timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueId && "'queueId' not provided");

    (void)queueId;  // Compiler happiness in 'opt' build

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CONFIGURE_QUEUE_ASYNC_CALLBACK, { return; });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_CONFIGURE_QUEUE_ASYNC_CALLBACK,
                   "options",
                   call.d_queueOptions,
                   options,
                   call);
    BMQA_CHECK_ARG(e_CONFIGURE_QUEUE_ASYNC_CALLBACK,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
}

int MockSession::closeQueue(QueueId*                  queueId,
                            const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(queueId);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CLOSE_QUEUE, { return 0; });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_CLOSE_QUEUE, "timeout", call.d_timeout, timeout, call);
    BMQA_RETURN_ON_RC();

    QueueImplSp& queueImpl = reinterpret_cast<QueueImplSp&>(*queueId);

    // In case queueId was uninitialized the impl ptr will be 0x0
    BSLS_ASSERT(queueImpl && "QueueId not set");

    queueImpl->setState(bmqimp::QueueState::e_CLOSED);
    uriCorrIdToQueues(d_twoKeyHashMapBuffer).eraseByKey1(queueId->uri());

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
    return 0;
}

CloseQueueStatus MockSession::closeQueueSync(QueueId*                  queueId,
                                             const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(queueId && "'queueId' not provided");

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CLOSE_QUEUE_SYNC, {
        return CloseQueueStatus(bmqa::QueueId(),
                                bmqt::CloseQueueResult::e_INVALID_ARGUMENT,
                                "Call to 'closeQueueSync' was not expected.");
    });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_CLOSE_QUEUE_SYNC,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);

    // Check and return on error rc
    const bmqa::CloseQueueStatus _result = call.d_closeQueueResult;
    if (!_result) {
        BSLS_ASSERT_OPT(!d_calls.empty());
        d_calls.pop_front();
        return _result;  // RETURN
    }

    const QueueImplSp& queueImpl = reinterpret_cast<const QueueImplSp&>(
        *queueId);

    // In case queueId was uninitialized the impl ptr will be 0x0
    BSLS_ASSERT(queueImpl && "QueueId not set");

    queueImpl->setState(bmqimp::QueueState::e_CLOSED);
    uriCorrIdToQueues(d_twoKeyHashMapBuffer).eraseByKey1(queueId->uri());

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
    return _result;
}

int MockSession::closeQueueAsync(QueueId*                  queueId,
                                 const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT(queueId);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CLOSE_QUEUE_ASYNC, { return 0; });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_CLOSE_QUEUE_ASYNC,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);
    BMQA_RETURN_ON_RC();

    QueueImplSp& queueImpl = reinterpret_cast<QueueImplSp&>(*queueId);

    // In case queueId was uninitialized the impl ptr will be 0x0
    BSLS_ASSERT(queueImpl && "QueueId not set");

    queueImpl->setState(bmqimp::QueueState::e_CLOSED);

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
    return 0;
}

void MockSession::closeQueueAsync(
    QueueId*                                         queueId,
    BSLS_ANNOTATION_UNUSED const CloseQueueCallback& callback,
    const bsls::TimeInterval&                        timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueId && "'queueId' not provided");

    (void)queueId;  // Compiler happiness in 'opt' build

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CLOSE_QUEUE_ASYNC_CALLBACK, { return; });

    // No need to check output parameter QueueId
    BMQA_CHECK_ARG(e_CLOSE_QUEUE_ASYNC_CALLBACK,
                   "timeout",
                   call.d_timeout,
                   timeout,
                   call);

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
}

Event MockSession::nextEvent(const bsls::TimeInterval& timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    // PRECONDITIONS
    BMQA_CHECK_CALL(e_NEXT_EVENT, { return Event(); });
    BSLS_ASSERT_OPT(call.d_emittedEvents.empty() &&
                    "'nextEvent' cannot emit events");

    BMQA_CHECK_ARG(e_NEXT_EVENT, "timeout", call.d_timeout, timeout, call);

    Event event = call.d_returnEvent;

    if (event.isSessionEvent()) {
        processIfQueueEvent(&event);
    }
    else if (event.isMessageEvent()) {
        processIfPushEvent(event);
    }

    BMQA_ASSERT_AND_POP_FRONT();
    return event;
}

int MockSession::post(const MessageEvent& messageEvent)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_POST, { return 0; });

    // TODO: Cannot validate messageEvent for now.
    BMQA_RETURN_ON_RC();

    d_eventsAndJobs.insert(d_eventsAndJobs.end(),
                           call.d_emittedEvents.begin(),
                           call.d_emittedEvents.end());

    BMQA_ASSERT_AND_POP_FRONT();
    d_postedEvents.push_back(messageEvent);
    return 0;
}

int MockSession::confirmMessage(const Message& message)
{
    return confirmMessage(message.confirmationCookie());
}

int MockSession::confirmMessage(const MessageConfirmationCookie& cookie)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CONFIRM_MESSAGE, { return 0; });

    BMQA_CHECK_ARG(e_CONFIRM_MESSAGE,
                   "cookie.queueId())",
                   call.d_cookie.queueId(),
                   cookie.queueId(),
                   call);

    BMQA_CHECK_ARG(e_CONFIRM_MESSAGE,
                   "cookie.messageGUID()",
                   call.d_cookie.messageGUID(),
                   cookie.messageGUID(),
                   call);
    BMQA_RETURN_ON_RC();

    d_unconfirmedGUIDs.erase(cookie.messageGUID());

    BMQA_ASSERT_AND_POP_FRONT();
    return 0;
}

int MockSession::confirmMessages(ConfirmEventBuilder* builder)
{
    // PRECONDITIONS
    BSLS_ASSERT(builder->blob().length() && "Invalid builder");

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    BMQA_CHECK_CALL(e_CONFIRM_MESSAGES, { return 0; });

    // No need to compare pointer of confirmEventBuilder.
    BMQA_RETURN_ON_RC();

    bmqp::Event                  bmqpEvent(&builder->blob(), d_allocator_p);
    bmqp::ConfirmMessageIterator cIter;

    bmqpEvent.loadConfirmMessageIterator(&cIter);

    while (cIter.next()) {
        d_unconfirmedGUIDs.erase(cIter.message().messageGUID());
    }

    BMQA_ASSERT_AND_POP_FRONT();
    return 0;
}

int MockSession::configureMessageDumping(
    BSLS_ANNOTATION_UNUSED const bslstl::StringRef& command)
{
    return 0;
}

// These macros are only used within the component.
#undef BMQA_CHECK_ARG
#undef BMQA_CHECK_CALL
#undef BMQA_ASSERT_AND_POP_FRONT
#undef BMQA_RETURN_ON_RC

}  // close package namespace
}  // close enterprise namespace
