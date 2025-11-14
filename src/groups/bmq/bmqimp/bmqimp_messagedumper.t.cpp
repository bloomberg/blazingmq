// Copyright 2023 Bloomberg Finance L.P.
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

// bmqimp_messagedumper.t.cpp                                         -*-C++-*-
#include <bmqimp_messagedumper.h>

// BMQ
#include <bmqimp_messagecorrelationidcontainer.h>
#include <bmqimp_queue.h>
#include <bmqimp_queuemanager.h>
#include <bmqp_ackeventbuilder.h>
#include <bmqp_confirmeventbuilder.h>
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_pusheventbuilder.h>
#include <bmqp_puteventbuilder.h>
#include <bmqp_queueid.h>
#include <bmqt_correlationid.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_stringutil.h>

// BDE
#include <bdlb_tokenizer.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bdlpcre_regex.h>
#include <bsl_functional.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Match the specified `str` against the specified `pattern` using the
/// specified `allocator` and return true on success, false otherwise.  The
/// matching is performed with the flags `k_FLAGS_CASELESS` and
/// `k_FLAG_DOTMATCHESALL` (as described in `bdlpcre_regex.h`).  The
/// behavior is undefined unless `pattern` represents a valid pattern.
static bool regexMatch(const bslstl::StringRef& str,
                       const char*              pattern,
                       bslma::Allocator*        allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(pattern);
    BSLS_ASSERT_SAFE(allocator);

    bsl::string    error(allocator);
    size_t         errorOffset;
    bdlpcre::RegEx regex(allocator);

    int rc = regex.prepare(&error,
                           &errorOffset,
                           pattern,
                           bdlpcre::RegEx::k_FLAG_CASELESS |
                               bdlpcre::RegEx::k_FLAG_DOTMATCHESALL);
    BSLS_ASSERT_SAFE(rc == 0);
    BSLS_ASSERT_SAFE(regex.isPrepared() == true);

    rc = regex.match(str.data(), str.length());

    return rc == 0;
}

/// This class provides a wrapper on top of the MessageDumper under test and
/// implements a few mechanisms to help testing the object.
struct Tester BSLS_CPP11_FINAL {
    /// full URI -> QueueId (id, subId)
    typedef bsl::unordered_map<bsl::string, bmqp::QueueId> QueueIdsMap;

  public:
    // PUBLIC DATA
    bmqimp::QueueManager d_queueManager;
    // Queue manager

    bmqimp::MessageCorrelationIdContainer d_messageCorrelationIdContainer;
    // Container for message correlationIds

    bmqimp::MessageDumper d_messageDumper;
    // MessageDumper (i.e. the object under
    // test)

    QueueIdsMap d_queueIdsByUri;
    // Map of queueIds by full URI;
    // populated upon inserting queues into
    // the Queue manager

    bsls::Types::Int64 d_nextCorrelationId;
    // Next correlationId for a new
    // queue. This value is always
    // incremented and never decremented.

    bsls::TimeInterval d_time;
    // Test time source that is used by the
    // MessageDumper under test to
    // determine at which point to cease
    // dumping event for dump actions that
    // are of type 'E_TIME_IN_SECONDS'

    bdlbb::PooledBlobBufferFactory d_bufferFactory;
    // Buffer factory provided to the
    // various builders

    /// Blob shared pointer pool used in event builders.
    bmqp::BlobPoolUtil::BlobSpPoolSp d_blobSpPool_sp;

    bmqp::PushEventBuilder d_pushEventBuilder;
    // PUSH event builder

    bmqp::AckEventBuilder d_ackEventBuilder;
    // ACK event builder

    bmqp::PutEventBuilder d_putEventBuilder;
    // PUT event builder

    bmqp::ConfirmEventBuilder d_confirmEventBuilder;
    // CONFIRM event builder

    bslma::Allocator* d_allocator_p;
    // Allocator to use

  private:
    Tester(const Tester& other) BSLS_CPP11_DELETED;
    Tester& operator=(const Tester& other) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Return the next correlationId.  This method always returns a value
    /// greater than or equal to 0 and never returns the same value twice.
    /// Note that this method is useful when setting the correlationId for a
    /// new queue.
    bsls::Types::Int64 generateNextCorrelationId();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Tester, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `Tester` object using the specified `allocator` to supply
    /// memory and perform any needed utilites initialization (e.g. time).
    explicit Tester(bslma::Allocator* allocator);

    ~Tester();

    // MANIPULATORS
    int processDumpCommand(const bslstl::StringRef& command);
    // Parse and process the specified dump 'command'.  Update the state
    // of the MessageDumper under test and return 0 if successful,
    // otherwise do nothing and return non-zero error code.  The behavior
    // is undefined unless 'command' is a valid dump command (as indicated
    // by the return code of 'MessageDumper::parseCommand').

    int processDumpCommand(const bmqp_ctrlmsg::DumpMessages& command);
    // Process the specified dump 'command'.  Update the state of the
    // MessageDumper under test and return 0 if successful, otherwise do
    // nothing and return non-zero error code.

    void reset();
    // Reset the state of the MessageDumper under test to its default
    // state, thus nullifying all previously processed dump commands.

    /// Create and insert a queue having the specified `uri` (full URI) to
    /// the queues recognized by this tester, henceforth associating the
    /// `uri` with the created queue in the various `packMessage` and/or
    /// `appendMessage` methods.
    void insertQueue(const bslstl::StringRef& uri);

    /// Add to the PUSH event being built a PUSH message having the
    /// specified `payload` and the optionally specified `msgGroupId` and
    /// associated with the queue corresponding to the specified `uri`. The
    /// behavior is undefined unless a queue with the `uri` was created and
    /// inserted using a call to `insertQueue`, or if packing the message is
    /// unsuccessful.
    void packPushMessage(const bslstl::StringRef& uri,
                         const bslstl::StringRef& payload,
                         unsigned int             subscriptionId);

    /// Append to the ACK event being built an ACK message having the
    /// specified `ackResult` status and associated with the queue
    /// corresponding to the specified `uri`.  The behavior is undefined
    /// unless a queue with the `uri` was created and inserted using a call
    /// to `insertQueue`, or if appending the message is unsuccessful.
    void appendAckMessage(const bslstl::StringRef& uri,
                          bmqt::AckResult::Enum    ackResult);

    /// Add to the PUT event being built a PUT message having the specified
    /// `payload` and the optionally specified `msgGroupId` and associated
    /// with the queue corresponding to the specified `uri`.  The behavior
    /// is undefined unless a queue with the `uri` was created and inserted
    /// using a call to `insertQueue`, or if packing the message is
    /// unsuccessful.
#ifdef BMQ_ENABLE_MSG_GROUPID
    void packPutMessage(const bslstl::StringRef& uri,
                        const bslstl::StringRef& payload,
                        const bslstl::StringRef& msgGroupId = "");
#else
    void packPutMessage(const bslstl::StringRef& uri,
                        const bslstl::StringRef& payload);
#endif

    /// Append to the CONFIRM event being built a CONFIRM message associated
    /// with the queue corresponding to the specified `uri`.  The behavior
    /// is undefined unless a queue with the `uri` was created and inserted
    /// using a call to `insertQueue`, or if appending the message is
    /// unsuccessful.
    void appendConfirmMessage(const bslstl::StringRef& uri);

    /// Print to the specified `out` the specified `event` of PUSH messages
    /// and reset the PUSH event currently being built by this object.  The
    /// behavior is undefined unless `event` is a valid PUSH event and PUSH
    /// event dump is enabled as indicated by `isEventDumpEnabled`.
    /// Additionally, the behavior is undefined unless each PUSH message in
    /// the `event` has at most one SubQueueId.
    void dumpPushEvent(bsl::ostream& out, const bmqp::Event& event);

    /// Print to the specified `out` the specified `event` of ACK messages
    /// and reset the ACK event currently being built by this object.  The
    /// behavior is undefined unless `event` is a valid ACK event and ACK
    /// event dump is enabled as indicated by `isEventDumpEnabled`.
    void dumpAckEvent(bsl::ostream& out, const bmqp::Event& event);

    /// Print to the specified `out` the specified `event` of PUT messages
    /// and reset the PUT event currently being built by this object.  The
    /// behavior is undefined unless `event` is a valid PUT event and PUT
    /// event dump is enabled as indicated by `isEventDumpEnabled`.
    void dumpPutEvent(bsl::ostream& out, const bmqp::Event& event);

    /// Print to the specified `out` the specified `event` of CONFIRM
    /// messages and reset the CONFIRM event currently being built by this
    /// object.  The behavior is undefined unless `event` is a valid CONFIRM
    /// event and CONFIRM event dump is enabled as indicated by
    /// `isEventDumpEnabled`.
    void dumpConfirmEvent(bsl::ostream& out, const bmqp::Event& event);

    /// Advance time by the specified `seconds`.
    void advanceTime(bsls::Types::Int64 seconds);

    void registerSubscription(const bslstl::StringRef&   uri,
                              unsigned int               subscriptionId,
                              const bmqt::CorrelationId& correlationId);

    void updateSubscriptions(const bslstl::StringRef&              uri,
                             const bmqp_ctrlmsg::StreamParameters& config);

    // ACCESSORS

    /// Return true if dumping is enabled for the next specified `type` of
    /// event, and false otherwise.  Note that event dumping could only be
    /// enabled for `PUSH`, `ACK`, `PUT`, and `CONFIRM` events, if
    /// configured appropriately and per the configuration specified in
    /// `processDumpCommand`.
    bool isEventDumpEnabled(const bmqp::EventType::Enum& type) const;

    /// Populate the specified `event` with the PUSH event currently being
    /// built by this object and return a reference to `event`.
    bmqp::Event& pushEvent(bmqp::Event* event) const;

    /// Populate the specified `event` with the ACK event currently being
    /// built by this object and return a reference to `event`.
    bmqp::Event& ackEvent(bmqp::Event* event) const;

    /// Populate the specified `event` with the PUT event currently being
    /// built by this object and return a reference to `event`.
    bmqp::Event& putEvent(bmqp::Event* event) const;

    /// Populate the specified `event` with the CONFIRM event currently
    /// being built by this object and return a reference to `event`.
    bmqp::Event& confirmEvent(bmqp::Event* event) const;

    /// Return the current time.
    bsls::TimeInterval now() const;

    /// Return the current time in total nanoseconds.
    bsls::Types::Int64 highResolutionTimer() const;
};

// ------
// Tester
// ------

// PRIVATE MANIPULATORS
bsls::Types::Int64 Tester::generateNextCorrelationId()
{
    return d_nextCorrelationId++;
}

// CREATORS
Tester::Tester(bslma::Allocator* allocator)
: d_queueManager(allocator)
, d_messageCorrelationIdContainer(allocator)
, d_messageDumper(&d_queueManager,
                  &d_messageCorrelationIdContainer,
                  &d_bufferFactory,
                  allocator)
, d_nextCorrelationId(0)
, d_time(0, 0)
, d_bufferFactory(1024, allocator)
, d_blobSpPool_sp(
      bmqp::BlobPoolUtil::createBlobPool(&d_bufferFactory, allocator))
, d_pushEventBuilder(d_blobSpPool_sp.get(), allocator)
, d_ackEventBuilder(d_blobSpPool_sp.get(), allocator)
, d_putEventBuilder(d_blobSpPool_sp.get(), allocator)
, d_confirmEventBuilder(d_blobSpPool_sp.get(), allocator)
, d_allocator_p(allocator)
{
    bmqsys::Time::initialize(
        bdlf::BindUtil::bindS(d_allocator_p, &Tester::now, this),
        bdlf::BindUtil::bindS(d_allocator_p, &Tester::now, this),
        bdlf::BindUtil::bindS(d_allocator_p,
                              &Tester::highResolutionTimer,
                              this),
        allocator);
}

Tester::~Tester()
{
    bmqsys::Time::shutdown();
}

// MANIPULATORS
int Tester::processDumpCommand(const bslstl::StringRef& command)
{
    bmqp_ctrlmsg::DumpMessages dumpMessagesCommand;

    int rc = bmqimp::MessageDumper::parseCommand(&dumpMessagesCommand,
                                                 command);
    BSLS_ASSERT_SAFE(rc == 0);
    static_cast<void>(rc);  // suppress 'unused variable' warning

    return d_messageDumper.processDumpCommand(dumpMessagesCommand);
}

int Tester::processDumpCommand(const bmqp_ctrlmsg::DumpMessages& command)
{
    return d_messageDumper.processDumpCommand(command);
}

void Tester::reset()
{
    d_messageDumper.reset();
}

void Tester::insertQueue(const bslstl::StringRef& uri)
{
    bmqimp::QueueManager::QueueSp queueSp;
    queueSp.createInplace(d_allocator_p, d_allocator_p);

    bmqt::Uri           fullUri(uri, d_allocator_p);
    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);

    bmqp::QueueId queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    d_queueManager.generateQueueAndSubQueueId(&queueId, fullUri, flags);
    (*queueSp)
        .setUri(fullUri)
        .setId(queueId.id())
        .setFlags(flags)
        .setCorrelationId(bmqt::CorrelationId(generateNextCorrelationId()));

    if (!fullUri.id().empty()) {
        // Fanout consumer
        (*queueSp).setSubQueueId(queueId.subId());
    }

    d_queueManager.insertQueue(queueSp);

    const bsl::string fullUriStr(uri, d_allocator_p);
    d_queueIdsByUri.emplace(fullUriStr, queueId);
}

void Tester::packPushMessage(const bslstl::StringRef& uri,
                             const bslstl::StringRef& payload,
                             unsigned int             subscriptionId)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueIdsByUri.find(uri) != d_queueIdsByUri.end() &&
                    "queue with 'uri' was not previously inserted ");

    bmqp::QueueId     queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    bdlbb::Blob       msgPayload(&d_bufferFactory, d_allocator_p);
    bmqt::MessageGUID msgGUID;
    int               rc = 0;

    queueId = d_queueIdsByUri.find(uri)->second;
    if (queueId.subId() != bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) {
        // Non-default subQueueId
        bmqp::Protocol::SubQueueInfosArray subQueueInfos(
            1,
            bmqp::SubQueueInfo(subscriptionId),
            d_allocator_p);
        rc = d_pushEventBuilder.addSubQueueInfosOption(subQueueInfos);
        BSLS_ASSERT_OPT(rc == 0);
    }

    bdlbb::BlobUtil::append(&msgPayload, payload.data(), payload.length());

    rc = d_pushEventBuilder.packMessage(
        msgPayload,
        queueId.id(),
        msgGUID,
        0,  // flags
        bmqt::CompressionAlgorithmType::e_NONE);
    BSLS_ASSERT_OPT(rc == 0);
}

void Tester::appendAckMessage(const bslstl::StringRef& uri,
                              bmqt::AckResult::Enum    ackResult)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueIdsByUri.find(uri) != d_queueIdsByUri.end() &&
                    "queue with 'uri' was not previously inserted ");

    bmqp::QueueId     queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    bmqt::MessageGUID msgGUID = bmqp::MessageGUIDGenerator::testGUID();

    bsls::Types::Int64 cid = generateNextCorrelationId();
    d_messageCorrelationIdContainer.add(msgGUID,
                                        bmqt::CorrelationId(cid),
                                        queueId);

    queueId = d_queueIdsByUri.find(uri)->second;

    int rc = d_ackEventBuilder.appendMessage(
        bmqp::ProtocolUtil::ackResultToCode(ackResult),
        bmqp::AckMessage::k_NULL_CORRELATION_ID,
        msgGUID,
        queueId.id());
    BSLS_ASSERT_OPT(rc == 0);
}

#ifdef BMQ_ENABLE_MSG_GROUPID
void Tester::packPutMessage(const bslstl::StringRef& uri,
                            const bslstl::StringRef& payload,
                            const bslstl::StringRef& msgGroupId)
#else
void Tester::packPutMessage(const bslstl::StringRef& uri,
                            const bslstl::StringRef& payload)
#endif
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueIdsByUri.find(uri) != d_queueIdsByUri.end() &&
                    "queue with 'uri' was not previously inserted ");

    bmqp::QueueId      queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    bdlbb::Blob        msgPayload(&d_bufferFactory, d_allocator_p);
    bmqt::MessageGUID  guid = bmqp::MessageGUIDGenerator::testGUID();
    bsls::Types::Int64 cid  = generateNextCorrelationId();
    d_messageCorrelationIdContainer.add(guid,
                                        bmqt::CorrelationId(cid),
                                        queueId);

    queueId = d_queueIdsByUri.find(uri)->second;
    bdlbb::BlobUtil::append(&msgPayload, payload.data(), payload.length());

    d_putEventBuilder.startMessage();
    d_putEventBuilder.setMessageGUID(guid).setMessagePayload(&msgPayload);
#ifdef BMQ_ENABLE_MSG_GROUPID
    if (!msgGroupId.empty()) {
        bmqp::Protocol::MsgGroupId groupId(msgGroupId, d_allocator_p);
        d_putEventBuilder.setMsgGroupId(msgGroupId);
    }
#endif

    int rc = d_putEventBuilder.packMessage(queueId.id());
    BSLS_ASSERT_OPT(rc == 0);
}

void Tester::appendConfirmMessage(const bslstl::StringRef& uri)

{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueIdsByUri.find(uri) != d_queueIdsByUri.end() &&
                    "queue with 'uri' was not previously inserted ");

    bmqp::QueueId     queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);
    bmqt::MessageGUID msgGUID;

    queueId = d_queueIdsByUri.find(uri)->second;

    int rc = d_confirmEventBuilder.appendMessage(queueId.id(),
                                                 queueId.subId(),
                                                 msgGUID);
    BSLS_ASSERT_OPT(rc == 0);
}

void Tester::dumpPushEvent(bsl::ostream& out, const bmqp::Event& event)
{
    d_messageDumper.dumpPushEvent(out, event);

    d_pushEventBuilder.reset();
}

void Tester::dumpAckEvent(bsl::ostream& out, const bmqp::Event& event)
{
    d_messageDumper.dumpAckEvent(out, event);

    d_ackEventBuilder.reset();
}

void Tester::dumpPutEvent(bsl::ostream& out, const bmqp::Event& event)
{
    d_messageDumper.dumpPutEvent(out, event, &d_bufferFactory);

    d_putEventBuilder.reset();
}

void Tester::dumpConfirmEvent(bsl::ostream& out, const bmqp::Event& event)
{
    d_messageDumper.dumpConfirmEvent(out, event);

    d_confirmEventBuilder.reset();
}

void Tester::advanceTime(bsls::Types::Int64 seconds)
{
    d_time.addSeconds(seconds);
}

void Tester::registerSubscription(const bslstl::StringRef&   uri,
                                  unsigned int               subscriptionId,
                                  const bmqt::CorrelationId& correlationId)
{
    const bmqimp::QueueManager::QueueSp& queue = d_queueManager.lookupQueue(
        uri);

    BSLS_ASSERT_SAFE(queue);

    queue->registerInternalSubscriptionId(subscriptionId,
                                          subscriptionId,
                                          correlationId);
}

void Tester::updateSubscriptions(const bslstl::StringRef&              uri,
                                 const bmqp_ctrlmsg::StreamParameters& config)
{
    const bmqimp::QueueManager::QueueSp& queue = d_queueManager.lookupQueue(
        uri);

    BSLS_ASSERT_SAFE(queue);

    d_queueManager.updateSubscriptions(queue, config);
}

// ACCESSORS
bool Tester::isEventDumpEnabled(const bmqp::EventType::Enum& type) const
{
    bool result = false;

    switch (type) {
    case bmqp::EventType::e_CONTROL: {
        result =
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_CONTROL>();
    } break;
    case bmqp::EventType::e_PUT: {
        result = d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_PUT>();
    } break;
    case bmqp::EventType::e_CONFIRM: {
        result =
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_CONFIRM>();
    } break;
    case bmqp::EventType::e_REJECT: {
        result =
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_REJECT>();
    } break;
    case bmqp::EventType::e_PUSH: {
        result = d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_PUSH>();
    } break;
    case bmqp::EventType::e_ACK: {
        result = d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_ACK>();
    } break;
    case bmqp::EventType::e_CLUSTER_STATE: {
        result = d_messageDumper
                     .isEventDumpEnabled<bmqp::EventType::e_CLUSTER_STATE>();
    } break;
    case bmqp::EventType::e_ELECTOR: {
        result =
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_ELECTOR>();
    } break;
    case bmqp::EventType::e_STORAGE: {
        result =
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_STORAGE>();
    } break;
    case bmqp::EventType::e_RECOVERY: {
        result =
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_RECOVERY>();
    } break;
    case bmqp::EventType::e_PARTITION_SYNC: {
        result = d_messageDumper
                     .isEventDumpEnabled<bmqp::EventType::e_PARTITION_SYNC>();
    } break;
    case bmqp::EventType::e_HEARTBEAT_REQ: {
        result = d_messageDumper
                     .isEventDumpEnabled<bmqp::EventType::e_HEARTBEAT_REQ>();

    } break;
    case bmqp::EventType::e_HEARTBEAT_RSP: {
        result = d_messageDumper
                     .isEventDumpEnabled<bmqp::EventType::e_HEARTBEAT_RSP>();
    } break;
    case bmqp::EventType::e_REPLICATION_RECEIPT: {
        result =
            d_messageDumper
                .isEventDumpEnabled<bmqp::EventType::e_REPLICATION_RECEIPT>();
    } break;
    case bmqp::EventType::e_UNDEFINED:
    default: {
        result =
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_UNDEFINED>();
    } break;
    }

    return result;
}

bmqp::Event& Tester::pushEvent(bmqp::Event* event) const
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(event && "'event' must be provided");

    event->reset(d_pushEventBuilder.blob().get(), true);

    return *event;
}

bmqp::Event& Tester::ackEvent(bmqp::Event* event) const
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(event && "'event' must be provided");

    event->reset(d_ackEventBuilder.blob().get(), true);

    return *event;
}

bmqp::Event& Tester::putEvent(bmqp::Event* event) const
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(event && "'event' must be provided");

    event->reset(d_putEventBuilder.blob().get(), true);

    return *event;
}

bmqp::Event& Tester::confirmEvent(bmqp::Event* event) const
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(event && "'event' must be provided");

    event->reset(d_confirmEventBuilder.blob().get(), true);

    return *event;
}

bsls::TimeInterval Tester::now() const
{
    return d_time;
}

bsls::Types::Int64 Tester::highResolutionTimer() const
{
    return now().totalNanoseconds();
}

}  // close unnamed namespace

//=============================================================================
//                              TEST CASES
//-----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    Tester tester(bmqtst::TestHelperUtil::allocator());

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.  TBD:
    // figure out the right way to "fix" this.
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Tester, bslma::UsesBslmaAllocator> >(
            tester));

    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUSH),
                     false);
    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_ACK), false);
    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUT), false);
    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_CONFIRM),
                     false);
    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_UNDEFINED),
                     false);
}

static void test2_parseCommand()
// ------------------------------------------------------------------------
// PARSE COMMAND
//
// Concerns:
//   1. Ensure that can parse a DumpMessages command from a valid string
//      per the format specified in the contract.
//   2. Ensure that if unable to parse a DumpMessages command from a string
//      due to invalid formatting of the string, a non-zero error code is
//      returned.
//
// Plan:
//   1. Attempt to parse a DumpMessages command from various valid and
//      invalid string representations and verify expected return code as
//      well as DumpMessages output.
//
// Testing:
//   MessageDumper::parseCommand
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("PARSE COMMAND");

    struct Test {
        int                                 d_line;
        const char*                         d_command;
        int                                 d_expectedRc;
        bmqp_ctrlmsg::DumpMsgType::Value    d_expectedMsgTypeToDump;
        bmqp_ctrlmsg::DumpActionType::Value d_expectedDumpActionType;
        int                                 d_expectedDumpActionValue;
    } k_DATA[] = {

        // IN
        {L_,
         "in on",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_INCOMING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "IN ON",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_INCOMING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "IN OFF",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_INCOMING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_OFF  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "IN 10",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_INCOMING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT  // dumpActionType
         ,
         10},  // dumpActionValue
        {L_,
         "IN 100s",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_INCOMING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS
         // dumpActionType
         ,
         100},  // dumpActionValue

        // OUT
        {L_,
         "OUT ON",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_OUTGOING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "OUT OFF",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_OUTGOING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_OFF  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "OUT 10",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_OUTGOING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT  // dumpActionType
         ,
         10},  // dumpActionValue
        {L_,
         "OUT 100s",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_OUTGOING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS
         // dumpActionType
         ,
         100},  // dumpActionValue

        // PUSH
        {L_,
         "push on",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUSH  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "PUSH ON",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUSH  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "PUSH OFF",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUSH  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_OFF  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "PUSH 10",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUSH  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT  // dumpActionType
         ,
         10},  // dumpActionValue
        {L_,
         "PUSH 100s",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUSH  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS
         // dumpActionType
         ,
         100},  // dumpActionValue

        // ACK
        {L_,
         "ack on",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_ACK  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "ACK ON",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_ACK  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "ACK OFF",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_ACK  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_OFF  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "ACK 10",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_ACK  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT  // dumpActionType
         ,
         10},  // dumpActionValue
        {L_,
         "ACK 100s",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_ACK  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS
         // dumpActionType
         ,
         100},  // dumpActionValue

        // PUT
        {L_,
         "put on",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUT  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "PUT ON",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUT  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "PUT OFF",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUT  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_OFF  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "PUT 10",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUT  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT  // dumpActionType
         ,
         10},  // dumpActionValue
        {L_,
         "PUT 100s",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUT  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS
         // dumpActionType
         ,
         100},  // dumpActionValue

        // CONFIRM
        {L_,
         "confirm on",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_CONFIRM  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "CONFIRM ON",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_CONFIRM  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_ON  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "CONFIRM OFF",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_CONFIRM  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_OFF  // dumpActionType
         ,
         0},  // dumpActionValue
        {L_,
         "CONFIRM 10",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_CONFIRM  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT  // dumpActionType
         ,
         10},  // dumpActionValue
        {L_,
         "CONFIRM 100s",
         0  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_CONFIRM  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_TIME_IN_SECONDS
         // dumpActionType
         ,
         100},  // dumpActionValue

        // ERROR CASES
        // Invalid message type
        {L_,
         "InVaLiD MesSsaGe TyPe",
         -1  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_INCOMING  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_OFF  // dumpActionType
         ,
         0},  // dumpActionValue
        // Missing action type
        {L_,
         "PUSH",
         -2  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_PUSH  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_OFF  // dumpActionType
         ,
         0},  // dumpActionValue
        // Invalid action type
        {L_,
         "CONFIRM 0",
         -3  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_CONFIRM  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT  // dumpActionType
         ,
         10},  // dumpActionValue
        // Invalid action type value
        {L_,
         "CONFIRM 10sec",
         -4  // rc
         ,
         bmqp_ctrlmsg::DumpMsgType::E_CONFIRM  // msgTypeToDump
         ,
         bmqp_ctrlmsg::DumpActionType::E_MESSAGE_COUNT  // dumpActionType
         ,
         0}  // dumpActionValue
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": parsing dump command: [command: "
                        << test.d_command
                        << "], expecting return code: " << test.d_expectedRc);

        bmqp_ctrlmsg::DumpMessages dumpMessagesCommand;

        int rc = bmqimp::MessageDumper::parseCommand(&dumpMessagesCommand,
                                                     test.d_command);
        BMQTST_ASSERT_EQ(rc, test.d_expectedRc);

        if (rc != 0) {
            // The specified 'command' is invalid and 'dumpMessagesCommand' has
            // not been populated, so we're done. Continue to the next test
            // datum.
            continue;  // CONTINUE
        }

        PVV(test.d_line << ": Expecting DumpMessages:"
                        << " [msgTypeToDump: " << test.d_expectedMsgTypeToDump
                        << ", dumpActionType: "
                        << test.d_expectedDumpActionType
                        << ", dumpActionValue: "
                        << test.d_expectedDumpActionValue);

        // 1. Attempt to parse a DumpMessages command from various valid and
        //    invalid string representations and verify expected return code as
        //    well as DumpMessages output.
        BMQTST_ASSERT_EQ(dumpMessagesCommand.msgTypeToDump(),
                         test.d_expectedMsgTypeToDump);
        BMQTST_ASSERT_EQ(dumpMessagesCommand.dumpActionType(),
                         test.d_expectedDumpActionType);
        BMQTST_ASSERT_EQ(dumpMessagesCommand.dumpActionValue(),
                         test.d_expectedDumpActionValue);
    }
}

static void test3_processDumpCommand()
// ------------------------------------------------------------------------
// PROCESS DUMP COMMAND
//
// Concerns:
//   1. Ensure that can process a DumpMessages command enabling the event
//      dumping of the corresponding types.
//
// Plan:
//   1. Process various DumpMessages commands and verify that dumping of
//      corresponding events is enabled.
//
// Testing:
//   processDumpCommand
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PROCESS DUMP COMMAND");

    struct Test {
        int         d_line;
        const char* d_command;
        bool        d_isPushEnabled;
        bool        d_isAckEnabled;
        bool        d_isPutEnabled;
        bool        d_isConfirmEnabled;
    } k_DATA[] = {{L_, "PUSH 10", true, false, false, false},
                  {L_, "ACK 100s", false, true, false, false},
                  {L_, "PUT ON", false, false, true, false},
                  {L_, "CONFIRM 1", false, false, false, true},
                  {L_, "IN ON", true, true, false, false},
                  {L_, "OUT 1s", false, false, true, true},
                  {L_, "IN OFF", false, false, false, false}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        Tester      tester(bmqtst::TestHelperUtil::allocator());
        const Test& test = k_DATA[idx];

        // 1. Process various DumpMessages commands and verify that dumping of
        //    corresponding events is enabled.
        PVV(test.d_line << ": processing dump command: [command: "
                        << test.d_command << "], expecting event dumping "
                        << "enabled as follows: "
                        << "[PUSH: " << test.d_isPushEnabled
                        << ", ACK: " << test.d_isAckEnabled
                        << ", PUT: " << test.d_isPutEnabled
                        << ", CONFIRM: " << test.d_isConfirmEnabled << "]");

        BMQTST_ASSERT_EQ(tester.processDumpCommand(test.d_command), 0);

        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUSH),
                         test.d_isPushEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_ACK),
                         test.d_isAckEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUT),
                         test.d_isPutEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_CONFIRM),
                         test.d_isConfirmEnabled);
    }
}

static void test4_processDumpCommand_invalidDumpMessage()
// ------------------------------------------------------------------------
// PROCESS DUMP COMMAND - INVALID DUMP MESSAGE
//
// Concerns:
//   1. Attempting to process an invalid dump command does not impact the
//      state of the MessageDumper object.
//   2. An appropriate return code is returned, indicating whether
//      processing the specified dump command was successful.
//
// Plan:
//   1. For various *valid* DumpMessages commands:
//     a. Process the command and verify that dumping of the corresponding
//       events is enabled.
//     b. Attempt to process further an *invalid* dump command and verify
//        that it does not impact the state of the MessageDumper object as
//        well as that a non-zero error code is returned.
//
// Testing:
//   processDumpCommand
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("PROCESS DUMP COMMAND");

    struct Test {
        int         d_line;
        const char* d_command;
        bool        d_isPushEnabled;
        bool        d_isAckEnabled;
        bool        d_isPutEnabled;
        bool        d_isConfirmEnabled;
    } k_DATA[] = {{L_, "PUSH 10", true, false, false, false},
                  {L_, "ACK 100s", false, true, false, false},
                  {L_, "PUT ON", false, false, true, false},
                  {L_, "CONFIRM 1", false, false, false, true},
                  {L_, "IN ON", true, true, false, false},
                  {L_, "OUT 1s", false, false, true, true},
                  {L_, "IN OFF", false, false, false, false}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        Tester      tester(bmqtst::TestHelperUtil::allocator());
        const Test& test = k_DATA[idx];

        // a. Process the command and verify that dumping of the corresponding
        //    events is enabled.
        PVV(test.d_line << ": processing dump command: [command: "
                        << test.d_command << "], expecting event dumping "
                        << "enabled as follows: "
                        << "[PUSH: " << test.d_isPushEnabled
                        << ", ACK: " << test.d_isAckEnabled
                        << ", PUT: " << test.d_isPutEnabled
                        << ", CONFIRM: " << test.d_isConfirmEnabled << "]");

        BMQTST_ASSERT_EQ(tester.processDumpCommand(test.d_command), 0);

        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUSH),
                         test.d_isPushEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_ACK),
                         test.d_isAckEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUT),
                         test.d_isPutEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_CONFIRM),
                         test.d_isConfirmEnabled);

        // b. Attempt to process further an *invalid* dump command and verify
        //    that it does not impact the state of the MessageDumper object as
        //    well as that a non-zero error code is returned.
        bmqp_ctrlmsg::DumpMessages invalidDumpMessagesCommand;
        invalidDumpMessagesCommand.msgTypeToDump() =
            static_cast<bmqp_ctrlmsg::DumpMsgType::Value>(-1);

        PVV(test.d_line << ": Attempting to process an invalid dump command");

        // Non-zero error code is returned
        BMQTST_ASSERT_NE(tester.processDumpCommand(invalidDumpMessagesCommand),
                         0);

        // No impact on the state of the MessageDumper object
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUSH),
                         test.d_isPushEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_ACK),
                         test.d_isAckEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUT),
                         test.d_isPutEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_CONFIRM),
                         test.d_isConfirmEnabled);
    }
}

static void test5_reset()
// ------------------------------------------------------------------------
// RESET
//
// Concerns:
//   1. Resetting the MessageDumper sets the object in a default state,
//      thus nullifying all previously processed dump commands.
//
// Plan:
//   1. Process various DumpMessages commands and verify that dumping of
//      corresponding events is enabled.  Then reset the MessageDumper and
//      verify that no event dumping is enabled.
//
// Testing:
//   reset
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("PROCESS DUMP COMMAND");

    struct Test {
        int         d_line;
        const char* d_command;
        bool        d_isPushEnabled;
        bool        d_isAckEnabled;
        bool        d_isPutEnabled;
        bool        d_isConfirmEnabled;
    } k_DATA[] = {{L_, "PUSH 10", true, false, false, false},
                  {L_, "ACK 100s", false, true, false, false},
                  {L_, "PUT ON", false, false, true, false},
                  {L_, "CONFIRM 1", false, false, false, true},
                  {L_, "IN ON", true, true, false, false},
                  {L_, "OUT 1s", false, false, true, true},
                  {L_, "IN OFF", false, false, false, false}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        Tester      tester(bmqtst::TestHelperUtil::allocator());
        const Test& test = k_DATA[idx];

        // 1. Process various DumpMessages commands and verify that dumping of
        //    corresponding events is enabled.  Then reset the MessageDumper
        //    and verify that no event dumping is enabled.
        PVV(test.d_line << ": processing dump command: [command: "
                        << test.d_command << "], expecting event dumping "
                        << "enabled as follows: "
                        << "[PUSH: " << test.d_isPushEnabled
                        << ", ACK: " << test.d_isAckEnabled
                        << ", PUT: " << test.d_isPutEnabled
                        << ", CONFIRM: " << test.d_isConfirmEnabled << "]");

        BMQTST_ASSERT_EQ(tester.processDumpCommand(test.d_command), 0);

        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUSH),
                         test.d_isPushEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_ACK),
                         test.d_isAckEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUT),
                         test.d_isPutEnabled);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_CONFIRM),
                         test.d_isConfirmEnabled);

        // Reset the MessageDumper and verify that no event dumping is enabled
        tester.reset();

        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUSH),
                         false);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_ACK),
                         false);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUT),
                         false);
        BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_CONFIRM),
                         false);
    }
}

static void test6_dumpPushEvent()
// ------------------------------------------------------------------------
// DUMP PUSH EVENT
//
// Concerns:
//   1. Dumping a PUSH event with an action type restricting the dumping
//      to a specific number of messages outputs no more than the specified
//      number of messages.
//   2. Upon exhausting the specified number of messages to dump, dumping
//      of PUSH events is no longer enabled, and attempts to do so fail
//      appropriately.
//
// Plan:
//   1. Process a dump command of 'PUSH 5' and build an event with six
//      messages.
//   2. Dump the PUSH event and verify that the first five messages were
//      dumped, and that no additional messages were dumped.
//   3. Ensure that upon exhausting the number of messages to dump, dumping
//      of PUSH events is no longer enabled, and attempts to do so fail
//      appropriately.
//
// Testing:
//   dumpPushEvent
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // QueueManager's 'generateQueueAndSubQueueId' method creates a
    // temporary string using the default allocator when performing queue
    // lookup by canonical URI

    bmqtst::TestHelper::printTestName("DUMP PUSH EVENT");

    Tester tester(bmqtst::TestHelperUtil::allocator());

    // Insert queues
    tester.insertQueue("bmq://bmq.test.mmap.fanout/q1?id=foo");
    tester.insertQueue("bmq://bmq.test.mmap.fanout/q1?id=bar");
    tester.insertQueue("bmq://bmq.test.mmap.priority/q1");

    // 1. Process a dump command of 'PUSH 5' and build an event with six
    //    messages.
    tester.processDumpCommand("PUSH 5");

    unsigned int                   subscriptionId = 1;
    bmqp_ctrlmsg::StreamParameters config(bmqtst::TestHelperUtil::allocator());

    config.subscriptions().resize(1);
    config.subscriptions()[0].sId() = subscriptionId;

    tester.registerSubscription("bmq://bmq.test.mmap.fanout/q1?id=foo",
                                subscriptionId,
                                bmqt::CorrelationId());

    tester.updateSubscriptions("bmq://bmq.test.mmap.fanout/q1?id=foo", config);

    config.subscriptions()[0].sId() = subscriptionId + 1;
    // subscriptionId must be unique across all subQueues

    tester.registerSubscription("bmq://bmq.test.mmap.fanout/q1?id=bar",
                                subscriptionId + 1,
                                bmqt::CorrelationId());
    tester.updateSubscriptions("bmq://bmq.test.mmap.fanout/q1?id=bar", config);

    config.subscriptions()[0].sId() = subscriptionId;

    tester.registerSubscription("bmq://bmq.test.mmap.priority/q1",
                                subscriptionId,
                                bmqt::CorrelationId());
    tester.updateSubscriptions("bmq://bmq.test.mmap.priority/q1", config);

    tester.packPushMessage("bmq://bmq.test.mmap.fanout/q1?id=foo",
                           "abcd",
                           subscriptionId);
    tester.packPushMessage("bmq://bmq.test.mmap.fanout/q1?id=foo",
                           "efgh",
                           subscriptionId);
    tester.packPushMessage("bmq://bmq.test.mmap.fanout/q1?id=bar",
                           "ijkl",
                           subscriptionId + 1);
    tester.packPushMessage("bmq://bmq.test.mmap.priority/q1",
                           "mnop",
                           subscriptionId);
    tester.packPushMessage("bmq://bmq.test.mmap.priority/q1",
                           "mnop",
                           subscriptionId);
    tester.packPushMessage("bmq://bmq.test.mmap.priority/q1",
                           "qrst",
                           subscriptionId);

    // 2. Dump the PUSH event and verify that the first five messages were
    //    dumped, and that no additional messages were dumped.
    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    bmqp::Event        event(bmqtst::TestHelperUtil::allocator());

    tester.pushEvent(&event);
    tester.dumpPushEvent(out, event);

    PVV(L_ << ": PUSH event dump: " << out.str());

    BMQTST_ASSERT_EQ(
        regexMatch(out.str(),
                   "PUSH Message #1:.*"
                   "queue: bmq://bmq.test.mmap.fanout/q1\\?id=foo.*"
                   "abcd.*",
                   bmqtst::TestHelperUtil::allocator()),
        true);
    BMQTST_ASSERT_EQ(
        regexMatch(out.str(),
                   "PUSH Message #2:.*"
                   "queue: bmq://bmq.test.mmap.fanout/q1\\?id=foo.*"
                   "efgh.*",
                   bmqtst::TestHelperUtil::allocator()),
        true);
    BMQTST_ASSERT_EQ(
        regexMatch(out.str(),
                   "PUSH Message #3:.*"
                   "queue: bmq://bmq.test.mmap.fanout/q1\\?id=bar.*"
                   "ijkl.*",
                   bmqtst::TestHelperUtil::allocator()),
        true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUSH Message #4:.*"
                                "queue: bmq://bmq.test.mmap.priority/q1.*"
                                "mnop.*",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUSH Message #5:.*"
                                "queue: bmq://bmq.test.mmap.priority/q1.*"
                                "mnop.*",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUSH Message #6:.*",
                                bmqtst::TestHelperUtil::allocator()),
                     false);

    out.reset();

    // 3. Ensure that upon exhausting the number of messages to dump, dumping
    //    of PUSH events is no longer enabled, and attempts to do so fail
    //    appropriately.
    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUSH),
                     false);
}

static void test7_dumpAckEvent()
// ------------------------------------------------------------------------
// DUMP ACK EVENT
//
// Concerns:
//   1. Dumping an ACK event with an action type restricting the number of
//      seconds during which to dump messages results in dumping of ACK
//      messages only during that time period and no dumping after.
//   2. Upon exhausting the specified number of seconds during which to
//      dump, further dumping of ACK messages is no longer enabled, and
//      attempts to do so fail appropriately.
//
// Plan:
//   1. Process a dump command of 'ACK 100s' and build an event with ACK
//      messages.
//   2. Dump the ACK event and verify that the messages in the event were
//      dumped.
//   3. Advance the time by an amount falling within the specified
//      duration of time to dump messages and verify that dumping of ACK
//      events is still enabled and working as expected.
//   4. Advance the time by an amount falling *outside* the specified
//      duration of time to dump messages and verify that dumping of ACK
//      events is no longer enabled and that attempts to do so fail
//      appropriately.
//
// Testing:
//   dumpAckEvent
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // QueueManager's 'generateQueueAndSubQueueId' method creates a
    // temporary string using the default allocator when performing queue
    // lookup by canonical URI

    bmqtst::TestHelper::printTestName("DUMP ACK EVENT");

    Tester tester(bmqtst::TestHelperUtil::allocator());

    // Insert queues
    tester.insertQueue("bmq://bmq.test.mmap.fanout/q1?id=foo");
    tester.insertQueue("bmq://bmq.test.mmap.fanout/q1?id=bar");
    tester.insertQueue("bmq://bmq.test.mmap.priority/q1");

    // 1. Process a dump command of 'ACK 100s' and build an event with ACK
    //    messages.
    tester.processDumpCommand("ACK 100s");

    tester.appendAckMessage("bmq://bmq.test.mmap.fanout/q1?id=foo",
                            bmqt::AckResult::e_SUCCESS);
    tester.appendAckMessage("bmq://bmq.test.mmap.fanout/q1?id=foo",
                            bmqt::AckResult::e_LIMIT_MESSAGES);
    tester.appendAckMessage("bmq://bmq.test.mmap.fanout/q1?id=bar",
                            bmqt::AckResult::e_LIMIT_BYTES);
    tester.appendAckMessage("bmq://bmq.test.mmap.priority/q1",
                            bmqt::AckResult::e_STORAGE_FAILURE);
    tester.appendAckMessage("bmq://bmq.test.mmap.priority/q1",
                            bmqt::AckResult::e_UNKNOWN);

    // 2. Dump the ACK event and verify that the messages in the event were
    //    dumped.
    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    bmqp::Event        event(bmqtst::TestHelperUtil::allocator());

    tester.ackEvent(&event);
    tester.dumpAckEvent(out, event);

    PVV(L_ << ": ACK event dump: " << out.str());

    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "ACK Message #1:.* status: SUCCESS",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "ACK Message #2:.* status: LIMIT_MESSAGES",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "ACK Message #3:.* status: LIMIT_BYTES",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "ACK Message #4:.* status: STORAGE_FAILURE",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "ACK Message #5:.* status: UNKNOWN",
                                bmqtst::TestHelperUtil::allocator()),
                     true);

    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "ACK Message #6",
                                bmqtst::TestHelperUtil::allocator()),
                     false);

    out.reset();

    // 3. Advance the time by an amount falling within the specified duration
    //    of time to dump messages and verify that dumping of ACK events is
    //    still enabled and working as expected.
    PVV(L_ << ": advancing time by 99 seconds: ");
    tester.advanceTime(99);
    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_ACK), true);

    tester.appendAckMessage("bmq://bmq.test.mmap.priority/q1",
                            bmqt::AckResult::e_SUCCESS);

    tester.ackEvent(&event);
    tester.dumpAckEvent(out, event);

    PVV(L_ << ": ACK event dump: " << out.str());

    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "ACK Message #1:.* status: SUCCESS",
                                bmqtst::TestHelperUtil::allocator()),
                     true);

    out.reset();

    // 4. Advance the time by an amount falling *outside* the specified
    //    duration of time to dump messages and verify that dumping of ACK
    //    events is no longer enabled and that attempts to do so fail
    //    appropriately.
    PVV(L_ << ": advancing time by 2 seconds: ");
    tester.advanceTime(2);

    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_ACK), false);
}

static void test8_dumpPutEvent()
// ------------------------------------------------------------------------
// DUMP PUT EVENT
//
// Concerns:
//   1. Dumping a PUT event when dumping is 'ON' outputs all PUT messages.
//   2. Upon turning dumping of PUT events 'OFF', dumping of PUT events is
//      no longer enabled, and attempts to do so fail appropriately.
//
// Plan:
//   1. Process a dump command of 'PUT ON' and build an event with four
//      PUT messages.
//   2. Dump the PUT event and verify that the PUT messages were dumped,
//      and that no additional messages were dumped.
//   3. Turn PUT event dumping off and ensure PUT event dump is no longer
//      enabled, and that attempts to do so fail appropriately.
//
// Testing:
//   dumpPushEvent
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // QueueManager's 'generateQueueAndSubQueueId' method creates a
    // temporary string using the default allocator when performing queue
    // lookup by canonical URI

    bmqtst::TestHelper::printTestName("DUMP PUT EVENT");

    Tester tester(bmqtst::TestHelperUtil::allocator());

    // Insert queues
    tester.insertQueue("bmq://bmq.test.mmap.fanout/q1");
    tester.insertQueue("bmq://bmq.test.mmap.priority/q1");
    tester.insertQueue("bmq://bmq.test.mmap.priority/q2");

    // 1. Process a dump command of 'PUT ON' and build an event with four PUT
    //    messages.
    tester.processDumpCommand("PUT ON");

#ifdef BMQ_ENABLE_MSG_GROUPID
    tester.packPutMessage("bmq://bmq.test.mmap.fanout/q1", "abcd", "Group 1");
    tester.packPutMessage("bmq://bmq.test.mmap.fanout/q1", "abcd", "Group 2");
#else
    tester.packPutMessage("bmq://bmq.test.mmap.fanout/q1", "abcd");
    tester.packPutMessage("bmq://bmq.test.mmap.fanout/q1", "abcd");
#endif
    tester.packPutMessage("bmq://bmq.test.mmap.priority/q1", "efgh");
    tester.packPutMessage("bmq://bmq.test.mmap.priority/q2", "ijkl");

    // 2. Dump the PUT event and verify that the PUT messages were dumped, and
    //    that no additional messages were dumped.
    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    bmqp::Event        event(bmqtst::TestHelperUtil::allocator());

    tester.putEvent(&event);
    tester.dumpPutEvent(out, event);

    PVV(L_ << ": PUT event dump: " << out.str());

#ifdef BMQ_ENABLE_MSG_GROUPID
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUT Message #1:.*"
                                "queue: bmq://bmq.test.mmap.fanout/q1.*"
                                "msgGroupId: \"Group 1\".*"
                                "abcd.*",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUT Message #2:.*"
                                "queue: bmq://bmq.test.mmap.fanout/q1.*"
                                "msgGroupId: \"Group 2\".*"
                                "abcd.*",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
#else
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUT Message #1:.*"
                                "queue: bmq://bmq.test.mmap.fanout/q1.*"
                                "abcd.*",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUT Message #2:.*"
                                "queue: bmq://bmq.test.mmap.fanout/q1.*"
                                "abcd.*",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
#endif
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUT Message #3:.*"
                                "queue: bmq://bmq.test.mmap.priority/q1.*"
                                "efgh.*",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUT Message #4:.*"
                                "queue: bmq://bmq.test.mmap.priority/q2.*"
                                "ijkl.*",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "PUT Message #5:.*",
                                bmqtst::TestHelperUtil::allocator()),
                     false);

    out.reset();

    // 3. Turn PUT event dumping off and ensure PUT event dump is no longer
    //    enabled, and that attempts to do so fail appropriately.
    tester.processDumpCommand("PUT OFF");

    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_PUT), false);
}

static void test9_dumpConfirmEvent()
// ------------------------------------------------------------------------
// DUMP CONFIRM EVENT
//
// Concerns:
//   1. Dumping a CONFIRM event with an action type restricting the dumping
//      to a specific number of messages outputs no more than the specified
//      number of messages.
//   2. Upon exhausting the specified number of messages to dump, dumping
//      of CONFIRM events is no longer enabled, and attempts to do so fail
//      appropriately.
//   3. Re-enabling dumping of CONFIRM events resumes dumping of CONFIRM
//      events per the dump command specified.
//
// Plan:
//   1. Process a dump command of 'CONFIRM 4' and build an event with five
//      messages.
//   2. Dump the CONFIRM event and verify that the first four messages were
//      dumped, and that no additional messages were dumped.
//   3. Ensure that upon exhausting the number of messages to dump, dumping
//      of CONFIRM events is no longer enabled, and attempts to do so fail
//      appropriately.
//   4. Process a dump command of 'CONFIRM 2' and build an event with three
//      CONFIRM messages.
//   5. Dump the CONFIRM event and verify the first two messages were
//      dumped, and that no additional messages were dumped.
//   6. Ensure that upon exhausting the number of messages to dump, dumping
//      of CONFIRM events is no longer enabled, and attempts to do so fail
//      appropriately.
//
// Testing:
//   dumpConfirmEvent
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // QueueManager's 'generateQueueAndSubQueueId' method creates a
    // temporary string using the default allocator when performing queue
    // lookup by canonical URI

    bmqtst::TestHelper::printTestName("DUMP CONFIRM EVENT");

    Tester tester(bmqtst::TestHelperUtil::allocator());

    // CONSTANTS

    // Insert queues
    tester.insertQueue("bmq://bmq.test.mmap.fanout/q1?id=foo");
    tester.insertQueue("bmq://bmq.test.mmap.fanout/q1?id=bar");
    tester.insertQueue("bmq://bmq.test.mmap.priority/q1");
    tester.insertQueue("bmq://bmq.test.mmap.priority/q2");

    // 1. Process a dump command of 'CONFIRM 4' and build an event with five
    //    messages.
    tester.processDumpCommand("CONFIRM 4");

    tester.appendConfirmMessage("bmq://bmq.test.mmap.fanout/q1?id=foo");
    tester.appendConfirmMessage("bmq://bmq.test.mmap.fanout/q1?id=foo");
    tester.appendConfirmMessage("bmq://bmq.test.mmap.fanout/q1?id=bar");
    tester.appendConfirmMessage("bmq://bmq.test.mmap.priority/q2");
    tester.appendConfirmMessage("bmq://bmq.test.mmap.priority/q1");

    // 2. Dump the CONFIRM event and verify that the first four messages were
    //    dumped, and that no additional messages were dumped.
    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    bmqp::Event        event(bmqtst::TestHelperUtil::allocator());

    tester.confirmEvent(&event);
    tester.dumpConfirmEvent(out, event);

    PVV(L_ << ": CONFIRM event dump: " << out.str());

    BMQTST_ASSERT_EQ(
        regexMatch(out.str(),
                   "CONFIRM Message #1:.*"
                   "queue: bmq://bmq.test.mmap.fanout/q1\\?id=foo",
                   bmqtst::TestHelperUtil::allocator()),
        true);
    BMQTST_ASSERT_EQ(
        regexMatch(out.str(),
                   "CONFIRM Message #2:.*"
                   "queue: bmq://bmq.test.mmap.fanout/q1\\?id=foo",
                   bmqtst::TestHelperUtil::allocator()),
        true);
    BMQTST_ASSERT_EQ(
        regexMatch(out.str(),
                   "CONFIRM Message #3:.*"
                   "queue: bmq://bmq.test.mmap.fanout/q1\\?id=bar",
                   bmqtst::TestHelperUtil::allocator()),
        true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "CONFIRM Message #4:.*"
                                "queue: bmq://bmq.test.mmap.priority/q2",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "CONFIRM Message #5:.*",
                                bmqtst::TestHelperUtil::allocator()),
                     false);

    out.reset();

    // 3. Ensure that upon exhausting the number of messages to dump, dumping
    //    of CONFIRM events is no longer enabled, and attempts to do so fail
    //    appropriately.
    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_CONFIRM),
                     false);

    // 4. Process a dump command of 'CONFIRM 2' and build an event with three
    //    CONFIRM messages.
    tester.processDumpCommand("CONFIRM 2");

    tester.appendConfirmMessage("bmq://bmq.test.mmap.priority/q1");
    tester.appendConfirmMessage("bmq://bmq.test.mmap.priority/q1");
    tester.appendConfirmMessage("bmq://bmq.test.mmap.priority/q2");

    // 5. Dump the CONFIRM event and verify the first two messages were dumped
    //    and that no additional messages were dumped.
    tester.confirmEvent(&event);
    tester.dumpConfirmEvent(out, event);

    PVV(L_ << ": CONFIRM event dump: " << out.str());

    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "CONFIRM Message #1:.*"
                                "queue: bmq://bmq.test.mmap.priority/q1",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "CONFIRM Message #2:.*"
                                "queue: bmq://bmq.test.mmap.priority/q1",
                                bmqtst::TestHelperUtil::allocator()),
                     true);
    BMQTST_ASSERT_EQ(regexMatch(out.str(),
                                "CONFIRM Message #3:.*",
                                bmqtst::TestHelperUtil::allocator()),
                     false);

    out.reset();

    // 6. Ensure that upon exhausting the number of messages to dump, dumping
    //    of CONFIRM events is no longer enabled, and attempts to do so fail
    //    appropriately.
    BMQTST_ASSERT_EQ(tester.isEventDumpEnabled(bmqp::EventType::e_CONFIRM),
                     false);
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 9: test9_dumpConfirmEvent(); break;
    case 8: test8_dumpPutEvent(); break;
    case 7: test7_dumpAckEvent(); break;
    case 6: test6_dumpPushEvent(); break;
    case 5: test5_reset(); break;
    case 4: test4_processDumpCommand_invalidDumpMessage(); break;
    case 3: test3_processDumpCommand(); break;
    case 2: test2_parseCommand(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
