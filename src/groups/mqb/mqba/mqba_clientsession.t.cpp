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

// mqba_clientsession.t.cpp                                           -*-C++-*-
#include <mqba_clientsession.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbi_queue.h>
#include <mqbmock_cluster.h>
#include <mqbmock_dispatcher.h>
#include <mqbmock_domain.h>
#include <mqbmock_queue.h>
#include <mqbmock_queueengine.h>
#include <mqbmock_queuehandle.h>
#include <mqbstat_brokerstats.h>
#include <mqbstat_queuestats.h>
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqp_puteventbuilder.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_puttester.h>
#include <bmqt_queueflags.h>
#include <bmqu_memoutstream.h>

#include <bmqio_channel.h>
#include <bmqio_testchannel.h>
#include <bmqst_statcontext.h>
#include <bmqsys_time.h>
#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlf_bind.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bslmt_timedsemaphore.h>
#include <bsls_annotation.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

typedef bdlcc::SharedObjectPool<
    bdlbb::Blob,
    bdlcc::ObjectPoolFunctors::DefaultCreator,
    bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
    BlobSpPool;

/// Struct to initialize system time component
struct TestClock {
    // DATA
    bdlmt::EventScheduler& d_scheduler;

    bdlmt::EventSchedulerTestTimeSource d_timeSource;

    // CREATORS
    TestClock(bdlmt::EventScheduler& scheduler)
    : d_scheduler(scheduler)
    , d_timeSource(&scheduler)
    {
        // NOTHING
    }

    // MANIPULATORS
    bsls::TimeInterval realtimeClock() { return d_timeSource.now(); }

    bsls::TimeInterval monotonicClock() { return d_timeSource.now(); }

    bsls::Types::Int64 highResTimer()
    {
        return d_timeSource.now().totalNanoseconds();
    }
};

enum Any { e_Any, e_Specific };

template <typename T>
struct TypeOrAny {
    T   d_value;
    Any d_any;

    TypeOrAny(const T value)
    : d_value(value)
    , d_any(e_Specific)
    {
    }

    TypeOrAny(const Any any)
    : d_value()
    , d_any(any)
    {
        ASSERT(any == e_Any);
    }

    bool operator==(const T& rhs) const
    {
        return (d_any == e_Any) || (d_value == rhs);
    }
};

enum ClientType {
    e_FirstHop,
    e_FirstHopCorrelationIds,
    e_MiddleHop,
    e_Broker
};
enum AtMostOnce { e_AtLeastOnce, e_AtMostOnce };
enum PutAction { e_DoNotPut, e_DoPut };
enum AckRequested { e_AckNotRequested, e_AckRequested };
enum AckSuccess { e_AckFailure, e_AckSuccess };
enum AckResult { e_AckResultNone, e_AckResultSuccess, e_AckResultUnknown };

enum InvalidPut {
    e_InvalidPutUnknownQueue,

    // Not covered yet
    // e_InvalidPutEvent,
    // e_InvalidPutNext,
    // e_InvalidPutAppData,
    // e_InvalidPutOptions,

    e_InvalidPutClosedQueue
};

typedef TypeOrAny<ClientType>   ClientTypeOrAny;
typedef TypeOrAny<AtMostOnce>   AtMostOnceOrAny;
typedef TypeOrAny<PutAction>    PutActionOrAny;
typedef TypeOrAny<AckRequested> AckRequestedOrAny;
typedef TypeOrAny<AckSuccess>   AckSuccessOrAny;

struct Spec {
    ClientTypeOrAny   clientType;
    AtMostOnceOrAny   atMostOnce;
    PutActionOrAny    putAction;
    AckRequestedOrAny ackRequested;
    AckSuccessOrAny   ackSuccess;
    AckResult         ackResult;
};

struct InvalidPutSpec {
    ClientTypeOrAny   clientType;
    AtMostOnceOrAny   atMostOnce;
    AckRequestedOrAny ackRequested;
    InvalidPut        invalidPut;
    AckResult         ackResult;
};

struct GuidCollisionSpec {
    ClientTypeOrAny   clientType;
    AtMostOnceOrAny   atMostOnce;
    AckRequestedOrAny ackRequested;
    AckResult         ackResult1;
    AckResult         ackResult2;
};

// ----------------------------------------------------------------------------
//                    This specifies the expected behavior
// ----------------------------------------------------------------------------

// ACK propagation general use cases
const Spec s_spec[] = {
    // When we have at-least once mode on the first hop, then, an ack is
    // propagated if we get a failure _or_ if the GUID has been previously
    // 'PUT' and ack was requested.
    {e_FirstHop,
     e_AtLeastOnce,
     e_Any,
     e_Any,
     e_AckFailure,
     e_AckResultUnknown},
    {e_FirstHop,
     e_AtLeastOnce,
     e_DoNotPut,
     e_Any,
     e_AckSuccess,
     e_AckResultNone},
    {e_FirstHop,
     e_AtLeastOnce,
     e_DoPut,
     e_AckNotRequested,
     e_AckSuccess,
     e_AckResultNone},
    {e_FirstHop,
     e_AtLeastOnce,
     e_DoPut,
     e_AckRequested,
     e_AckSuccess,
     e_AckResultSuccess},

    {e_FirstHopCorrelationIds,
     e_AtLeastOnce,
     e_Any,
     e_Any,
     e_AckFailure,
     e_AckResultUnknown},
    {e_FirstHopCorrelationIds,
     e_AtLeastOnce,
     e_DoNotPut,
     e_Any,
     e_AckSuccess,
     e_AckResultNone},
    {e_FirstHopCorrelationIds,
     e_AtLeastOnce,
     e_DoPut,
     e_AckNotRequested,
     e_AckSuccess,
     e_AckResultNone},
    {e_FirstHopCorrelationIds,
     e_AtLeastOnce,
     e_DoPut,
     e_AckRequested,
     e_AckSuccess,
     e_AckResultSuccess},

    // When we have a client and we aren't on the first hop or we are a broker,
    // we propagate success and failures.
    {e_MiddleHop,
     e_AtLeastOnce,
     e_Any,
     e_Any,
     e_AckFailure,
     e_AckResultUnknown},
    {e_MiddleHop,
     e_AtLeastOnce,
     e_Any,
     e_Any,
     e_AckSuccess,
     e_AckResultSuccess},
    {e_Broker, e_AtLeastOnce, e_Any, e_Any, e_AckFailure, e_AckResultUnknown},
    {e_Broker, e_AtLeastOnce, e_Any, e_Any, e_AckSuccess, e_AckResultSuccess},

    // When we have at-most once mode, then, we drop ack regardless of whether
    // this is first hop or not.
    {e_Any, e_AtMostOnce, e_DoNotPut, e_Any, e_Any, e_AckResultNone},
    {e_Any, e_AtMostOnce, e_DoPut, e_AckNotRequested, e_Any, e_AckResultNone},
    {e_Any, e_AtMostOnce, e_DoPut, e_AckRequested, e_Any, e_AckResultSuccess}};

// Invalid event use cases
const InvalidPutSpec s_invalidPutSpec[] = {
    {e_Any, e_Any, e_Any, e_InvalidPutUnknownQueue, e_AckResultUnknown},

    // Not covered yet
    // {e_Any, e_Any, e_Any, e_InvalidPutEvent,        e_AckResultNone   },
    // {e_Any, e_Any, e_Any, e_InvalidPutNext,         e_AckResultNone   },
    // {e_Any, e_Any, e_Any, e_InvalidPutAppData,      e_AckResultUnknown},
    // {e_Any, e_Any, e_Any, e_InvalidPutOptions,      e_AckResultUnknown},

    {e_Any, e_Any, e_Any, e_InvalidPutClosedQueue, e_AckResultUnknown}};

// GUID collision use cases
const GuidCollisionSpec s_guidCollisionSpec[] = {
    // For each case, we send two PUT messages, two success ACK messages
    // and check the result.

    // In case this is first hop, ack is requested and there is a guid
    // collision, we log an error and do send the second PUT message upstream
    // but only the first ACK message is propagated.
    {e_FirstHop,
     e_AtLeastOnce,
     e_AckNotRequested,
     e_AckResultNone,
     e_AckResultNone},
    {e_FirstHop,
     e_AtLeastOnce,
     e_AckRequested,
     e_AckResultSuccess,
     e_AckResultNone},

    // In case this is first hop and SDK sends PUTs with correlation ids,
    // corresponsing GUIDs are generated by client session. These GUIDs are
    // very unlikely to be the same so there should be no guid collision and
    // both PUT messages are sent upstream. If ack is requested, then both
    // ACK messages are propagated.
    {e_FirstHopCorrelationIds,
     e_AtLeastOnce,
     e_AckNotRequested,
     e_AckResultNone,
     e_AckResultNone},
    {e_FirstHopCorrelationIds,
     e_AtLeastOnce,
     e_AckRequested,
     e_AckResultSuccess,
     e_AckResultSuccess},

    // When we have a client and we aren't on the first hop or we are a broker,
    // both messages are sent upstream. Regardless of ack requested flag, both
    // ACK messages are propagated.
    {e_MiddleHop,
     e_AtLeastOnce,
     e_Any,
     e_AckResultSuccess,
     e_AckResultSuccess},
    {e_Broker, e_AtLeastOnce, e_Any, e_AckResultSuccess, e_AckResultSuccess},

    // When we have at-most once mode, then, we always receive generated ACK
    // messages if ack requested flag is set regardless of guid uniqueness.
    {e_Any, e_AtMostOnce, e_AckNotRequested, e_AckResultNone, e_AckResultNone},
    {e_Any,
     e_AtMostOnce,
     e_AckRequested,
     e_AckResultSuccess,
     e_AckResultSuccess}};

/// Attempts to find a `spec` that matches the specified `clientType`,
/// `atMostOnce`, `putAction`, `ackRequested` and `ackSuccess`.
Spec findSpec(ClientType   clientType,
              AtMostOnce   atMostOnce,
              PutAction    putAction,
              AckRequested ackRequested,
              AckSuccess   ackSuccess)
{
    int matchIndex = -1;
    for (unsigned i = 0; i < sizeof(s_spec) / sizeof(s_spec[0]); ++i) {
        const Spec& current = s_spec[i];
        if (current.clientType == clientType &&
            current.atMostOnce == atMostOnce &&
            current.putAction == putAction &&
            current.ackRequested == ackRequested &&
            current.ackSuccess == ackSuccess) {
            ASSERT(matchIndex == -1);  // Duplicate match on specs.
            matchIndex = i;
        }
    }
    ASSERT(matchIndex != -1);  // Exactly one should match
    return s_spec[matchIndex];
}

/// Attempts to find a `spec` that matches the specified `clientType`,
/// `atMostOnce`, `ackRequested` and `invalidPut`.
InvalidPutSpec findInvalidPutSpec(ClientType   clientType,
                                  AtMostOnce   atMostOnce,
                                  AckRequested ackRequested,
                                  InvalidPut   invalidPut)
{
    int      matchIndex = -1;
    unsigned n = sizeof(s_invalidPutSpec) / sizeof(s_invalidPutSpec[0]);
    for (unsigned i = 0; i < n; ++i) {
        const InvalidPutSpec& current = s_invalidPutSpec[i];
        if (current.clientType == clientType &&
            current.atMostOnce == atMostOnce &&
            current.ackRequested == ackRequested &&
            current.invalidPut == invalidPut) {
            ASSERT(matchIndex == -1);  // Duplicate match on specs.
            matchIndex = i;
        }
    }
    ASSERT(matchIndex != -1);  // Exactly one should match
    return s_invalidPutSpec[matchIndex];
}

/// Attempts to find a `spec` that matches the specified `clientType`,
/// `ackRequested` and `atMostOnce`.
GuidCollisionSpec findGuidCollisionSpec(ClientType   clientType,
                                        AtMostOnce   atMostOnce,
                                        AckRequested ackRequested)
{
    int      matchIndex = -1;
    unsigned n = sizeof(s_guidCollisionSpec) / sizeof(s_guidCollisionSpec[0]);
    for (unsigned i = 0; i < n; ++i) {
        const GuidCollisionSpec& current = s_guidCollisionSpec[i];
        if (current.clientType == clientType &&
            current.atMostOnce == atMostOnce &&
            current.ackRequested == ackRequested) {
            ASSERT(matchIndex == -1);  // Duplicate match on specs.
            matchIndex = i;
        }
    }
    ASSERT(matchIndex != -1);  // Exactly one should match
    return s_guidCollisionSpec[matchIndex];
}

bmqp_ctrlmsg::NegotiationMessage client(const ClientType clientType)
// Create a 'NegotiationMessage' that represents a client configuration for
// the specified 'clientType'.
{
    bmqp_ctrlmsg::NegotiationMessage negotiationMessage;
    if (clientType == e_Broker) {
        negotiationMessage.makeBrokerResponse();
    }
    else {
        bmqp_ctrlmsg::ClientIdentity& clientIdentity =
            negotiationMessage.makeClientIdentity();
        clientIdentity.clientType() =
            clientType == e_MiddleHop ? bmqp_ctrlmsg::ClientType::E_TCPBROKER
                                      : bmqp_ctrlmsg::ClientType::E_TCPCLIENT;

        if (clientType == e_FirstHop) {
            clientIdentity.guidInfo().clientId()             = "0A0B0C0D0E0F";
            clientIdentity.guidInfo().nanoSecondsFromEpoch() = 1261440000;
        }
    }
    return negotiationMessage;
}

mqbmock::Dispatcher* setInDispatcherThread(mqbmock::Dispatcher* mockDispatcher)
// Utility method.  Sets 'MockDispatcher' attribute.
{
    mockDispatcher->_setInDispatcherThread(true);
    return mockDispatcher;
}

/// Overrides default `MockQueueHandle` behavior with extra functionality.
class MyMockQueueHandle : public mqbmock::QueueHandle {
  public:
    // PUBLIC TYPES
    struct Post {
        bmqp::PutHeader              d_putHeader;
        bsl::shared_ptr<bdlbb::Blob> d_appData;
        bsl::shared_ptr<bdlbb::Blob> d_options;
    };

  private:
    // PRIVATE DATA
    bmqst::StatContext d_statContext;
    // The 'bmqst::StatContext' for this 'QueueHandle'.

    bsl::vector<Post> d_postedMessages;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MyMockQueueHandle,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Constructor
    MyMockQueueHandle(
        const bsl::shared_ptr<mqbi::Queue>& queue,
        const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                   clientContext,
        mqbstat::QueueStatsDomain*                 domainStats,
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
        bslma::Allocator*                          allocator)
    : mqbmock::QueueHandle(queue,
                           clientContext,
                           domainStats,
                           handleParameters,
                           allocator)
    , d_statContext(bmqst::StatContextConfiguration("Test Stat Context")
                        .value("In", 11)
                        .value("Out", 11),
                    allocator)
    {
        // Not used upstreamSubQueueId;
        unsigned int upstreamSubQueueId = 1;
        registerSubStream(bmqp_ctrlmsg::SubQueueIdInfo(),
                          upstreamSubQueueId,
                          mqbi::QueueCounts(1, 0));
    }

    /// Destructor
    ~MyMockQueueHandle() BSLS_KEYWORD_OVERRIDE
    {
        // Reset those.  Typically it should be done while closing queues.
        setStreamParameters(bmqp_ctrlmsg::StreamParameters());
        setHandleParameters(bmqp_ctrlmsg::QueueHandleParameters());
        unregisterSubStream(bmqp_ctrlmsg::SubQueueIdInfo(),
                            mqbi::QueueCounts(1, 0),
                            false);
    }

    // ACCESSORS

    /// Returns the captured messages.
    const bsl::vector<Post>& postedMessages() const
    {
        return d_postedMessages;
    }

    // MANIPULATORS

    /// Called by the framework when a new message with the specified
    /// `putHeader`, `appData` and `options` is sent upstream.  We capture
    /// the message.
    virtual void postMessage(const bmqp::PutHeader&              putHeader,
                             const bsl::shared_ptr<bdlbb::Blob>& appData,
                             const bsl::shared_ptr<bdlbb::Blob>& options)
        BSLS_KEYWORD_OVERRIDE
    {
        Post& p = d_postedMessages.emplace_back();

        p.d_putHeader = putHeader;
        p.d_appData   = appData;
        p.d_options   = options;
    }
};

class MyQueueEngine : public mqbmock::QueueEngine {
  private:
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS
    MyQueueEngine(bslma::Allocator* allocator)
    : mqbmock::QueueEngine(allocator)
    , d_allocator_p(allocator)
    {
    }

    ~MyQueueEngine() BSLS_KEYWORD_OVERRIDE {}

    // MANIPULATORS
    void configureHandle(
        BSLS_ANNOTATION_UNUSED mqbi::QueueHandle*          handle,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
        BSLS_KEYWORD_OVERRIDE
    {
        bmqp_ctrlmsg::Status success(d_allocator_p);
        success.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        if (configuredCb) {
            configuredCb(success, streamParameters);
        }
    }
};

/// Overrides default `MockDomain` behavior with extra functionality.
class MyMockDomain : public mqbmock::Domain {
  public:
    mqbmock::Dispatcher*               d_mockDispatcher;
    bmqp_ctrlmsg::RoutingConfiguration d_routingConfiguration;
    bsl::shared_ptr<MyMockQueueHandle> d_queueHandle;
    MyQueueEngine                      d_mockQueueEngine;
    const bool                         d_atMostOnce;
    bslma::Allocator*                  d_allocator_p;

    // CREATORS

    /// Constructor
    MyMockDomain(mqbmock::Dispatcher* dispatcher,
                 mqbi::Cluster*       cluster,
                 bool                 atMostOnce,
                 bslma::Allocator*    allocator)
    : mqbmock::Domain(cluster, allocator)
    , d_mockDispatcher(dispatcher)
    , d_queueHandle()
    , d_mockQueueEngine(allocator)
    , d_atMostOnce(atMostOnce)
    , d_allocator_p(allocator)
    {
    }

    /// Destructor
    ~MyMockDomain() BSLS_KEYWORD_OVERRIDE {}

    // MANIPULATORS

    /// Implements the required for this test behavior for `OpenQueue`.  It
    /// calls the specified `callback` with a new queue handle created
    /// using the specified `handleParameters`.  The specified `uri` and
    /// `clientContext` are ignored.
    void openQueue(BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
                   const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                              clientContext,
                   const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                   const OpenQueueCallback& callback) BSLS_KEYWORD_OVERRIDE
    {
        mqbstat::QueueStatsDomain*      domainStats = 0;
        bsl::shared_ptr<mqbmock::Queue> queue;
        queue.createInplace(d_allocator_p, this, d_allocator_p);
        queue->_setQueueEngine(&d_mockQueueEngine);
        queue->_setAtMostOnce(d_atMostOnce);
        queue->_setDispatcher(d_mockDispatcher);

        d_queueHandle.createInplace(d_allocator_p,
                                    queue,
                                    clientContext,
                                    domainStats,
                                    handleParameters,
                                    d_allocator_p);

        OpenQueueConfirmationCookie confirmationCookie;
        confirmationCookie.createInplace(d_allocator_p, d_queueHandle.get());

        bmqp_ctrlmsg::Status status(d_allocator_p);
        status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;

        bmqp_ctrlmsg::OpenQueueResponse openQueueResponse(s_allocator_p);
        openQueueResponse.routingConfiguration() = d_routingConfiguration;

        callback(status,
                 d_queueHandle.get(),
                 openQueueResponse,
                 confirmationCookie);
    }
};

/// Create a new blob at the specified `arena` address, using the specified
/// `bufferFactory` and `allocator`.
void createBlob(bdlbb::BlobBufferFactory* bufferFactory,
                void*                     arena,
                bslma::Allocator*         allocator)
{
    new (arena) bdlbb::Blob(bufferFactory, allocator);
}

/// Utility function that assert-fails.
template <typename T>
T assertFail()
{
    ASSERT(false);
    return T();
}

/// The `TestBench` holds system components together.
class TestBench {
  private:
    // PRIVATE TYPES
    typedef mqbmock::Dispatcher::EventGuard     EventGuard;
    typedef const bmqio::TestChannel::WriteCall ConstWriteCall;

  public:
    // DATA
    bdlbb::PooledBlobBufferFactory        d_bufferFactory;
    BlobSpPool                            d_blobSpPool;
    bsl::shared_ptr<bmqio::TestChannel>   d_channel;
    mqbmock::Cluster                      d_cluster;
    mqbmock::Dispatcher                   d_mockDispatcher;
    MyMockDomain                          d_domain;
    mqbmock::DomainFactory                d_mockDomainFactory;
    bslma::ManagedPtr<bmqst::StatContext> d_clientStatContext_mp;
    bdlmt::EventScheduler                 d_scheduler;
    TestClock                             d_testClock;
    mqba::ClientSession                   d_cs;
    bslma::Allocator*                     d_allocator_p;

    static const int k_PAYLOAD_LENGTH = 36;

    // CREATORS

    /// Constructor. Creates a `TestBench` using the specified
    /// `negotiationMessage`, `atMostOnce` and `allocator`.
    TestBench(const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
              const bool                              atMostOnce,
              bslma::Allocator*                       allocator)
    : d_bufferFactory(256, allocator)
    , d_blobSpPool(bdlf::BindUtil::bind(&createBlob,
                                        &d_bufferFactory,
                                        bdlf::PlaceHolders::_1,   // arena
                                        bdlf::PlaceHolders::_2),  // alloc
                   1024,  // blob pool growth strategy
                   allocator)
    , d_channel(new bmqio::TestChannel(allocator))
    , d_cluster(&d_bufferFactory, allocator)
    , d_mockDispatcher(allocator)
    , d_domain(&d_mockDispatcher, &d_cluster, atMostOnce, allocator)
    , d_mockDomainFactory(d_domain, allocator)
    , d_clientStatContext_mp(
          mqbstat::QueueStatsUtil::initializeStatContextClients(10, allocator)
              .managedPtr())
    , d_scheduler(bsls::SystemClockType::e_MONOTONIC, allocator)
    , d_testClock(d_scheduler)
    , d_cs(d_channel,
           negotiationMessage,
           "sessionDescription",
           setInDispatcherThread(&d_mockDispatcher),
           0,  // ClusterCatalog
           &d_mockDomainFactory,
           d_clientStatContext_mp,
           &d_blobSpPool,
           &d_bufferFactory,
           &d_scheduler,
           allocator)
    , d_allocator_p(allocator)
    {
        {
            // Temporary workaround to suppress the 'unused operator
            // NestedTraitDeclaration' warning/error generated by clang.
            //
            // TBD: figure out the right way to "fix" this.

            bmqp_ctrlmsg::QueueHandleParameters dummyParameters(s_allocator_p);
            bsl::shared_ptr<mqbi::QueueHandleRequesterContext> clientContext(
                new (*s_allocator_p)
                    mqbi::QueueHandleRequesterContext(s_allocator_p),
                s_allocator_p);
            clientContext->setClient(0);

            bsl::shared_ptr<mqbi::Queue> queue(
                new (*d_allocator_p) mqbmock::Queue(&d_domain, d_allocator_p),
                d_allocator_p);

            MyMockQueueHandle dummy(queue,
                                    clientContext,
                                    static_cast<mqbstat::QueueStatsDomain*>(0),
                                    dummyParameters,
                                    s_allocator_p);
            // NOTE: use '1' to fool the assert of d_queue_sp in
            //       MockQueueHandle constructor.
            static_cast<void>(
                static_cast<
                    bslmf::NestedTraitDeclaration<MyMockQueueHandle,
                                                  bslma::UsesBslmaAllocator> >(
                    dummy));
        }

        // Typically done during 'Dispatcher::registerClient()'.
        d_cs.dispatcherClientData().setDispatcher(&d_mockDispatcher);
        d_mockDispatcher._setInDispatcherThread(true);

        // Setup test time source
        bmqsys::Time::shutdown();
        bmqsys::Time::initialize(
            bdlf::BindUtil::bind(&TestClock::realtimeClock, &d_testClock),
            bdlf::BindUtil::bind(&TestClock::monotonicClock, &d_testClock),
            bdlf::BindUtil::bind(&TestClock::highResTimer, &d_testClock),
            d_allocator_p);

        int rc = d_scheduler.start();
        ASSERT_EQ(rc, 0);
    }

    /// Destructor
    ~TestBench()
    {
        d_cs.tearDown(bsl::shared_ptr<void>(), true);
        d_scheduler.cancelAllEventsAndWait();
        d_scheduler.stop();
    }

    // MANIPULATORS

    /// Sends an `OpenQueue` event for the specified `uri` and `queuId`, to
    /// this testbench's `ClientSession`.
    void openQueue(const bsl::string& uri, const int queueId)
    {
        bmqp::SchemaEventBuilder     obj(&d_bufferFactory, d_allocator_p);
        bmqp_ctrlmsg::ControlMessage controlMessage(d_allocator_p);
        bmqp_ctrlmsg::OpenQueue&     openQueue =
            controlMessage.choice().makeOpenQueue();
        openQueue.handleParameters().uri()        = uri;
        openQueue.handleParameters().readCount()  = 1;
        openQueue.handleParameters().writeCount() = 1;
        openQueue.handleParameters().flags()      = bmqt::QueueFlags::e_WRITE |
                                               bmqt::QueueFlags::e_READ;
        openQueue.handleParameters().qId() = queueId;
        controlMessage.rId().makeValue(queueId);

        // Encode the message
        int rc = obj.setMessage(controlMessage, bmqp::EventType::e_CONTROL);
        ASSERT_EQ(rc, 0);

        bmqp::Event event(&obj.blob(), d_allocator_p);

        d_cs.processEvent(event);
    }

    /// Sends an `CloseQueue` event for the specified `uri` and `queuId`, to
    /// this testbench's `ClientSession`.
    void closeQueue(const bsl::string& uri, const int queueId)
    {
        bmqp::SchemaEventBuilder     obj(&d_bufferFactory, d_allocator_p);
        bmqp_ctrlmsg::ControlMessage controlMessage(d_allocator_p);
        bmqp_ctrlmsg::CloseQueue&    closeQueue =
            controlMessage.choice().makeCloseQueue();
        closeQueue.isFinal()                       = true;
        closeQueue.handleParameters().uri()        = uri;
        closeQueue.handleParameters().readCount()  = 1;
        closeQueue.handleParameters().writeCount() = 1;
        closeQueue.handleParameters().flags() = bmqt::QueueFlags::e_WRITE |
                                                bmqt::QueueFlags::e_READ;
        closeQueue.handleParameters().qId() = queueId;
        controlMessage.rId().makeValue(queueId);

        // Encode the message
        int rc = obj.setMessage(controlMessage, bmqp::EventType::e_CONTROL);
        ASSERT_EQ(rc, 0);

        bmqp::Event event(&obj.blob(), d_allocator_p);

        d_cs.processEvent(event);
    }

    /// Sends an `Ack` event for the specified `queueId`, `guid` and
    /// `ackResult` to this testbench's `ClientSession`.
    void sendAck(const int                   queueId,
                 const bmqt::MessageGUID&    guid,
                 const bmqt::AckResult::Enum ackResult)
    {
        bmqp::AckMessage ackMessage(
            bmqp::ProtocolUtil::ackResultToCode(ackResult),
            bmqp::AckMessage::k_NULL_CORRELATION_ID,
            guid,
            queueId);

        mqbi::DispatcherEvent event(d_allocator_p);
        event.makeAckEvent().setAckMessage(ackMessage);

        dispatch(event);
    }

    /// Sends a `Put` event for the specified `queueId`, `msgGUID` and
    /// `correlationId` with `isAckRequested` flag
    /// to this testbench's `ClientSession`.
    void sendOldPut(
        const int                      queueId,
        const bmqt::MessageGUID&       msgGUID,
        const int                      correlationId,
        const bool                     isAckRequested,
        const bmqp::MessageProperties& properties = bmqp::MessageProperties(),
        bmqt::CompressionAlgorithmType::Enum cat =
            bmqt::CompressionAlgorithmType::e_NONE)
    {
        PVV("Sending PUT with queueId="
            << queueId << ", guid=" << msgGUID << ", correlationId="
            << correlationId << ", isAckRequested=" << isAckRequested);

        bsl::shared_ptr<bdlbb::Blob> eventBlob;
        eventBlob.createInplace(d_allocator_p,
                                &d_bufferFactory,
                                d_allocator_p);

        // Populate blob
        bmqp::EventHeader eventHeader;
        bmqp::PutHeader   putHeader;

        bmqp::PutTester::populateBlob(eventBlob.get(),
                                      &eventHeader,
                                      &putHeader,
                                      k_PAYLOAD_LENGTH,
                                      queueId,
                                      msgGUID,
                                      correlationId,
                                      properties,
                                      isAckRequested,
                                      &d_bufferFactory,
                                      cat);

        mqbi::DispatcherEvent event(d_allocator_p);
        event
            .setSource(&d_cs)  // DispatcherClient *value
            .makePutEvent()
            .setIsRelay(true)  // Relay message
            .setPutHeader(putHeader)
            .setPartitionId(1)    // d_state_p->partitionId()) // int value
            .setBlob(eventBlob);  // const bsl::shared_ptr<bdlbb::Blob>& value

        // Internal-ticket D167598037.
        // Verify that PutMessageIterator does not change the input.
        bmqp::Event rawEvent(eventBlob.get(), s_allocator_p);

        BSLS_ASSERT(rawEvent.isValid());
        BSLS_ASSERT(rawEvent.isPutEvent());

        bmqp::PutMessageIterator pIt1(&d_bufferFactory, d_allocator_p, false);
        rawEvent.loadPutMessageIterator(&pIt1, false);
        bmqp::PutMessageIterator pIt2(pIt1, d_allocator_p);
        bmqp::PutMessageIterator pIt3(&d_bufferFactory, d_allocator_p, true);
        rawEvent.loadPutMessageIterator(&pIt3, false);

        BSLS_ASSERT(pIt1.next());
        BSLS_ASSERT(pIt2.next());
        BSLS_ASSERT(pIt3.next());

        dispatch(event);
    }

    /// Sends a `Push` event for the specified `queueId`, `msgGUID` and
    /// `correlationId` with `isAckRequested` flag
    /// to this testbench's `ClientSession`.
    void sendPush(const int                            queueId,
                  const bmqt::MessageGUID&             msgGUID,
                  const bsl::shared_ptr<bdlbb::Blob>&  blob,
                  bmqt::CompressionAlgorithmType::Enum cat,
                  const bmqp::MessagePropertiesInfo&   logic)
    {
        PVV("Sending PUSH with queueId=" << queueId << ", guid=" << msgGUID);

        mqbi::DispatcherEvent event(d_allocator_p);

        event
            .setSource(&d_cs)  // DispatcherClient *value
            .makePushEvent()
            .setBlob(blob)
            .setGuid(msgGUID)
            .setMessagePropertiesInfo(logic)
            .setQueueId(queueId)
            .setCompressionAlgorithmType(cat);

        dispatch(event);
    }
    bool validateData(const bdlbb::Blob& blob,
                      int                offset,
                      int                length = k_PAYLOAD_LENGTH)
    {
        bdlbb::Blob in(&d_bufferFactory, d_allocator_p);
        bdlbb::Blob out(&d_bufferFactory, d_allocator_p);

        bmqp::PutTester::populateBlob(&in, length);
        bdlbb::BlobUtil::append(&out, blob, offset, d_allocator_p);
        return 0 == bdlbb::BlobUtil::compare(in, out);
    }

    /// Asserts that the first event written is an Open Queue Control Event.
    void assertOpenQueueResponse()
    {
        ASSERT(d_channel->waitFor(1, false));  // isFinal = false
        ConstWriteCall& openQueueCall = d_channel->writeCalls()[0];
        bmqp::Event     openQueueEvent(&openQueueCall.d_blob, s_allocator_p);
        PVV("Event 1: " << openQueueEvent);
        ASSERT(openQueueEvent.isControlEvent());
    }

    /// Depending on the specified `ackResult` it might assert that an ack
    /// message for the specified `queueId`, `msgGUID` and `correlationId`
    /// is sent downstream in the event specified by `eventIndex`.  Also
    /// check that the event is final or not.
    void assertAckIsSentIfExpected(const AckResult&         ackResult,
                                   const int                queueId,
                                   const bmqt::MessageGUID& msgGUID,
                                   const int                correlationId,
                                   const int                eventIndex = 1,
                                   const bool               isFinal    = true)
    {
        if (ackResult == e_AckResultNone) {
            // If no ack expected we shouldn't have more events than
            // 'eventIndex'.
            ASSERT(d_channel->waitFor(eventIndex));
            ASSERT(d_channel->hasNoMoreWriteCalls());

            return;  // RETURN
        }

        const bmqt::AckResult::Enum expectedStatus =
            ackResult == e_AckResultSuccess ? bmqt::AckResult::e_SUCCESS
            : ackResult == e_AckResultUnknown
                ? bmqt::AckResult::e_UNKNOWN
                : assertFail<bmqt::AckResult::Enum>();

        // If we expect acks, then the event with corresponding number
        // should exist and be of type Ack.
        ASSERT(d_channel->waitFor(eventIndex + 1, isFinal));

        ConstWriteCall& ackCall = d_channel->writeCalls()[eventIndex];
        bmqp::Event     ackEvent(&ackCall.d_blob, s_allocator_p);
        PVV("Event " << eventIndex + 1 << ": " << ackEvent);
        ASSERT(ackEvent.isAckEvent());

        // The event should have one (Ack) Message in it and it should be
        // about the queue with the given queue ID.
        bmqp::AckMessageIterator iter;
        ackEvent.loadAckMessageIterator(&iter);
        ASSERT(iter.next());
        ASSERT(iter.isValid());

        const bmqp::AckMessage& ack = iter.message();
        ASSERT_EQ(ack.queueId(), queueId);
        ASSERT_EQ(ack.messageGUID(), msgGUID);
        ASSERT_EQ(ack.correlationId(), correlationId);

        const bmqt::AckResult::Enum status =
            bmqp::ProtocolUtil::ackResultFromCode(ack.status());
        ASSERT_EQ(expectedStatus, status);
        ASSERT(!iter.next());
        ASSERT(!iter.isValid());

        if (isFinal) {
            ASSERT(d_channel->hasNoMoreWriteCalls());
        }
    }

    /// A method that prepares and calls ClientSession's `dispatch()`
    /// using the specified `event`.
    void dispatch(mqbi::DispatcherEvent& event)
    {
        EventGuard guard(d_mockDispatcher._withEvent(&d_cs, &event));

        d_cs.onDispatcherEvent(event);
    }

    void verifyPush(bmqp::MessageProperties* properties,
                    int                      length = k_PAYLOAD_LENGTH)
    {
        int last = d_channel->writeCalls().size();
        d_cs.flush();

        ASSERT(d_channel->waitFor(last + 1, false));

        bmqp::Event pushEvent(&d_channel->writeCalls()[last].d_blob,
                              s_allocator_p);

        ASSERT(pushEvent.isPushEvent());

        bmqp::PushMessageIterator pushIt(&d_bufferFactory, d_allocator_p);
        pushEvent.loadPushMessageIterator(&pushIt, true);

        ASSERT(pushIt.next());
        ASSERT_EQ(pushIt.loadMessageProperties(properties), 0);

        ASSERT_EQ(pushIt.header().compressionAlgorithmType(),
                  bmqt::CompressionAlgorithmType::e_NONE);
        // Any PUSH with MessageProperties comes out decompressed.
        // In ClientSession:
        //  Old style, uncompressed PUT gets converted to the new style.
        //  Old style, compressed PUT gets de-compressed and converted to
        //                                                  the new style.
        //  New style, uncompressed PUT gets passed through.
        //  New style, compressed PUT gets passed through.
        // So, inside Cluster/ClusterProxy, the style is always new.
        //  New style, uncompressed PUSH gets converted to the old style.
        //  New style, compressed PUSH gets de-compressed and converted to
        //                                                  the old style.
        // This will change once SDK starts advertising new feature.  Then
        // ClientSession will send PUSH in the new style to new client.

        ASSERT(bmqp::PushHeaderFlagUtil::isSet(
            pushIt.header().flags(),
            bmqp::PushHeaderFlags::e_MESSAGE_PROPERTIES));
        ASSERT(!bmqp::MessagePropertiesInfo::hasSchema(pushIt.header()));

        bdlbb::Blob payloadBlob(&d_bufferFactory, d_allocator_p);
        pushIt.loadApplicationData(&payloadBlob);

        bmqu::BlobObjectProxy<bmqp::MessagePropertiesHeader> mpsh(
            &payloadBlob,
            true,    // read
            false);  // write
        const int msgPropsAreaSize = mpsh->messagePropertiesAreaWords() *
                                     bmqp::Protocol::k_WORD_SIZE;

        ASSERT(validateData(payloadBlob, msgPropsAreaSize, length));
    }
};

/// This implements the core testing functionality.  Implements a test run
/// for the configuration defined by the specified `clientType`,
/// `atMostOnce`, `putAction`, `ackRequested` and `ackSuccess`.
void test(ClientType   clientType,
          AtMostOnce   atMostOnce,
          PutAction    putAction,
          AckRequested ackRequested,
          AckSuccess   ackSuccess)
{
    PV("Running test with configuration"
       << " clientType: " << clientType << " and atMostOnce: " << atMostOnce
       << " and putAction: " << putAction << " and ackRequested: "
       << ackRequested << " and ackSuccess: " << ackSuccess);

    const Spec spec =
        findSpec(clientType, atMostOnce, putAction, ackRequested, ackSuccess);

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    const int         queueId       = 4;  // A queue number
    int               correlationId = 0;  // A correlation id
    bmqt::MessageGUID guid;               // Unset

    TestBench tb(client(clientType),
                 atMostOnce == e_AtMostOnce,
                 s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    // If required by the spec, send a `PUT` message.
    const bool sendPut        = (putAction == e_DoPut);
    bool       isAckRequested = (ackRequested == e_AckRequested);

    if (sendPut) {
        // Set correlationId or generate GUID
        if (clientType == e_FirstHopCorrelationIds) {
            // Set correlationId if ack is requested
            if (isAckRequested) {
                correlationId = 2;
            }
        }
        else {
            // Generate GUID v1
            guid = bmqp::MessageGUIDGenerator::testGUID();

            PVV("Generated GUID v1: " << guid);
        }

        // Send PUT.  If GUID is unset, correlation ID will be used and
        // the GUID will be generated by client session.
        tb.sendOldPut(queueId, guid, correlationId, isAckRequested);
    }

    // Confirm that the OpenQueue response has been sent downstream.
    tb.d_cs.flush();
    tb.assertOpenQueueResponse();

    // Check if a message was sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT_EQ(sendPut, postMessages.size() == 1u);
    if (sendPut) {
        // If a message with correlation id was sent upstream, grab the GUID to
        // be used in the subsequent ack.
        if (clientType == e_FirstHopCorrelationIds) {
            // If PUT message was sent with correlation ID, when GUID was
            // generated by client session
            ASSERT(guid.isUnset());
            guid = postMessages[0].d_putHeader.messageGUID();
        }

        // If a message was sent upstream to 'at-most-once' queue,
        // ACK_REQUESTED flag should be unset, otherwise it should be set.
        isAckRequested = bmqp::PutHeaderFlagUtil::isSet(
            postMessages[0].d_putHeader.flags(),
            bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        ASSERT_EQ(atMostOnce == e_AtMostOnce, !isAckRequested);
    }

    // If a message was sent upstream, GUID should be set.
    // Otherwise GUID must be unset.
    ASSERT_EQ(sendPut, !guid.isUnset());

    // Send an 'Ack' message.
    tb.sendAck(queueId,
               guid,
               ackSuccess == e_AckSuccess
                   ? bmqt::AckResult::e_SUCCESS
                   : bmqt::AckResult::e_INVALID_ARGUMENT);

    // Confirm that (if expected) the ack has been sent downstream.
    tb.d_cs.flush();
    tb.assertAckIsSentIfExpected(spec.ackResult, queueId, guid, correlationId);
}

/// This implements the core testing functionality.  Implements a test run
/// for the configuration defined by the specified `clientType`,
/// `atMostOnce`, `ackRequested` and `invalidPut`.
void testInvalidPut(ClientType   clientType,
                    AtMostOnce   atMostOnce,
                    AckRequested ackRequested,
                    InvalidPut   invalidPut)
{
    PV("Running test with configuration"
       << " clientType: " << clientType << " and atMostOnce: " << atMostOnce
       << " and ackRequested: " << ackRequested
       << " and invalidPut: " << invalidPut);

    const InvalidPutSpec spec =
        findInvalidPutSpec(clientType, atMostOnce, ackRequested, invalidPut);

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    int               queueId       = 4;  // A queue number
    int               correlationId = 0;  // A correlation id
    bmqt::MessageGUID guid;               // Unset

    TestBench tb(client(clientType),
                 atMostOnce == e_AtMostOnce,
                 s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    const bool isAckRequested = (ackRequested == e_AckRequested);

    // Set correlationId or generate GUID
    if (clientType == e_FirstHopCorrelationIds) {
        // Set correlationId if ack is requested
        if (isAckRequested) {
            correlationId = 2;
        }
    }
    else {
        // Generate GUID v1
        guid = bmqp::MessageGUIDGenerator::testGUID();

        PVV("Generated GUID v1: " << guid);
    }

    if (invalidPut == e_InvalidPutUnknownQueue) {
        // Send PUT with modified queue id
        queueId++;
    }
    else if (invalidPut == e_InvalidPutClosedQueue) {
        // Close queue before sending PUT
        tb.closeQueue(uri, queueId);
    }
    else {
        // Other cases are not covered by this method yet.
        PV("NOT COVERED YET");
        return;  // RETURN
    }

    // Send PUT.  If GUID is unset, correlation ID will be used and
    // the GUID will be generated by client session.
    tb.sendOldPut(queueId, guid, correlationId, isAckRequested);

    // Confirm that the OpenQueue response has been sent downstream.
    tb.d_cs.flush();
    tb.assertOpenQueueResponse();

    // Check no messages were sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT(postMessages.empty());

    // Confirm that (if expected) the ack has been sent downstream.
    tb.assertAckIsSentIfExpected(spec.ackResult, queueId, guid, correlationId);
}

/// This implements the core testing functionality.  Implements a test run
/// for the configuration defined by the specified `clientType`,
/// `atMostOnce` and `ackRequested`.
void testGuidCollision(ClientType   clientType,
                       AtMostOnce   atMostOnce,
                       AckRequested ackRequested)
{
    PV("Running test with configuration"
       << " clientType: " << clientType << " and atMostOnce: " << atMostOnce
       << " and ackRequested: " << ackRequested);

    const GuidCollisionSpec spec = findGuidCollisionSpec(clientType,
                                                         atMostOnce,
                                                         ackRequested);

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    const int         queueId       = 4;  // A queue number
    int               correlationId = 0;  // A correlation id
    bmqt::MessageGUID guid1;              // Unset
    bmqt::MessageGUID guid2;              // Unset

    TestBench tb(client(clientType),
                 atMostOnce == e_AtMostOnce,
                 s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    const bool isAckRequested = (ackRequested == e_AckRequested);

    // Set correlationId or generate GUID
    if (clientType == e_FirstHopCorrelationIds) {
        // Set correlationId if ack is requested
        if (isAckRequested) {
            correlationId = 2;
        }
    }
    else {
        // Generate GUID v1
        guid1 = bmqp::MessageGUIDGenerator::testGUID();

        PVV("Generated GUID v1: " << guid1);

        // Set guid2 to the same value.
        guid2 = guid1;
        ASSERT_EQ(guid1, guid2);
    }

    // Send PUTs.  If GUID is unset, correlation ID will be used and
    // the GUIDs will be generated by client session.
    // In order to have one ACK message per on ACK event,
    // flush after sending each PUT message.
    tb.sendOldPut(queueId, guid1, correlationId, isAckRequested);
    tb.d_cs.flush();
    tb.sendOldPut(queueId, guid2, correlationId, isAckRequested);
    tb.d_cs.flush();

    // Confirm that the OpenQueue response has been sent downstream.
    tb.assertOpenQueueResponse();

    // Check if messages were sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT_EQ(2u, postMessages.size());

    // If a message was sent upstream to 'at-most-once' queue,
    // ACK_REQUESTED flag should be unset, otherwise it should be set.
    const bool isAckRequestedSet1 = bmqp::PutHeaderFlagUtil::isSet(
        postMessages[0].d_putHeader.flags(),
        bmqp::PutHeaderFlags::e_ACK_REQUESTED);
    ASSERT_EQ(atMostOnce == e_AtMostOnce, !isAckRequestedSet1);
    const bool isAckRequestedSet2 = bmqp::PutHeaderFlagUtil::isSet(
        postMessages[1].d_putHeader.flags(),
        bmqp::PutHeaderFlags::e_ACK_REQUESTED);
    ASSERT_EQ(atMostOnce == e_AtMostOnce, !isAckRequestedSet2);

    // Grab GUIDs generated by client session
    if (clientType == e_FirstHopCorrelationIds) {
        // If PUT message was sent with correlation ID, when GUID was
        // generated by client session.
        guid1 = postMessages[0].d_putHeader.messageGUID();
        guid2 = postMessages[1].d_putHeader.messageGUID();

        ASSERT_NE(guid1, guid2);
    }

    // Send ACK messages.
    // In order to have one ACK message per on ACK event,
    // flush after sending each PUT message.
    tb.sendAck(queueId, guid1, bmqt::AckResult::e_SUCCESS);
    tb.d_cs.flush();
    tb.sendAck(queueId, guid2, bmqt::AckResult::e_SUCCESS);
    tb.d_cs.flush();

    // Confirm that (if expected) the first ack has been sent downstream.
    int  eventIndex = 1;
    bool isFinal    = spec.ackResult1 == e_AckResultNone;

    tb.assertAckIsSentIfExpected(spec.ackResult1,
                                 queueId,
                                 guid1,
                                 correlationId,
                                 eventIndex,
                                 isFinal);

    // Confirm that (if expected) the second ack has been sent downstream.
    if (spec.ackResult1 != e_AckResultNone) {
        eventIndex++;
    }
    isFinal = true;
    tb.assertAckIsSentIfExpected(spec.ackResult2,
                                 queueId,
                                 guid2,
                                 correlationId,
                                 eventIndex,
                                 isFinal);
}

/// This implements the core testing functionality.  Implements a test run
/// for the configuration defined by the specified `atMostOnce` and
/// `ackRequested`.
void testFirstHopUnsetGUID(AtMostOnce atMostOnce, AckRequested ackRequested)
{
    PV("Running test with configuration"
       << " atMostOnce: " << atMostOnce
       << " and ackRequested: " << ackRequested);

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    const int         correlationId = 0;  // A correlation id
    const int         queueId       = 4;  // A queue number
    bmqt::MessageGUID guid;               // Unset

    TestBench tb(client(e_FirstHop),
                 atMostOnce == e_AtMostOnce,
                 s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    const bool isAckRequested = (ackRequested == e_AckRequested);

    // Send PUT with unset GUID.
    tb.sendOldPut(queueId, guid, correlationId, isAckRequested);
    tb.d_cs.flush();

    // Confirm that the OpenQueue response has been sent downstream.
    tb.assertOpenQueueResponse();

    // Check if a message was sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT(postMessages.empty());

    // Check if a NACK was sent
    tb.assertAckIsSentIfExpected(e_AckResultUnknown,
                                 queueId,
                                 guid,
                                 correlationId);
}

void encode(bmqp::MessageProperties* properties)
{
    ASSERT_EQ(0, properties->setPropertyAsInt32("encoding", 3));
    ASSERT_EQ(0, properties->setPropertyAsString("id", "3"));
    ASSERT_EQ(0, properties->setPropertyAsInt64("timestamp", 3LL));
    ASSERT_EQ(3, properties->numProperties());
}

void verify(const bmqp::MessageProperties& properties)
{
    ASSERT_EQ(3, properties.numProperties());
    ASSERT_EQ(properties.getPropertyAsInt32("encoding"), 3);
    ASSERT_EQ(properties.getPropertyAsString("id"), "3");
    ASSERT_EQ(properties.getPropertyAsInt64("timestamp"), 3LL);
}

void onShutdownComplete(bsls::AtomicInt*       callbackCounter,
                        bslmt::TimedSemaphore* sem)
{
    ++(*callbackCounter);
    sem->post();
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_ackConfigurations()
// ------------------------------------------------------------------------
// TESTS ACK CONFIGURATIONS FOR CLIENT SESSION
//
// Concerns:
//   - Tests that acks get propagated as expected under different
//     'ClientSession' configurations.
//
// Plan:
//   Instantiate a testbench, open a queue, send puts and acks under
//   different configurations.
//
// Testing:
//   That acks are propagated as expected.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS ACK CONFIGURATIONS FOR CLIENT SESSION");

#define SCENARIO_RUN(var, from, to) for (int var = from; var <= to; ++var) {
#define END_SCENARIO }

    SCENARIO_RUN(clientType, e_FirstHop, e_Broker)
    SCENARIO_RUN(atMostOnce, e_AtLeastOnce, e_AtMostOnce)
    SCENARIO_RUN(putAction, e_DoNotPut, e_DoPut)
    SCENARIO_RUN(ackRequested, e_AckNotRequested, e_AckRequested)
    SCENARIO_RUN(ackSuccess, e_AckFailure, e_AckSuccess)

    test(static_cast<ClientType>(clientType),
         static_cast<AtMostOnce>(atMostOnce),
         static_cast<PutAction>(putAction),
         static_cast<AckRequested>(ackRequested),
         static_cast<AckSuccess>(ackSuccess));

    END_SCENARIO
    END_SCENARIO
    END_SCENARIO
    END_SCENARIO
    END_SCENARIO

#undef SCENARIO_RUN
#undef END_SCENARIO
}

static void test2_invalidPutConfigurations()
// ------------------------------------------------------------------------
// TESTS INVALID PUT CONFIGURATIONS FOR CLIENT SESSION
//
// Concerns:
//   - Tests that acks get propagated as expected under different
//     'ClientSession' configurations when invalid PUT message is sent.
//
// Plan:
//   Instantiate a testbench, open a queue and send invalid PUT under
//   different configurations.
//
// Testing:
//   That acks are propagated as expected.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS INVALID PUT CONFIGURATIONS FOR CLIENT SESSION");

#define SCENARIO_RUN(var, from, to) for (int var = from; var <= to; ++var) {
#define END_SCENARIO }

    SCENARIO_RUN(clientType, e_FirstHop, e_Broker)
    SCENARIO_RUN(atMostOnce, e_AtLeastOnce, e_AtMostOnce)
    SCENARIO_RUN(ackRequested, e_AckNotRequested, e_AckRequested)
    SCENARIO_RUN(invalidPut, e_InvalidPutUnknownQueue, e_InvalidPutClosedQueue)

    testInvalidPut(static_cast<ClientType>(clientType),
                   static_cast<AtMostOnce>(atMostOnce),
                   static_cast<AckRequested>(ackRequested),
                   static_cast<InvalidPut>(invalidPut));

    END_SCENARIO
    END_SCENARIO
    END_SCENARIO
    END_SCENARIO

#undef SCENARIO_RUN
#undef END_SCENARIO
}

static void test3_guidCollisionConfigurations()
// ------------------------------------------------------------------------
// TESTS GUID COLLISION CONFIGURATIONS FOR CLIENT SESSION
//
// Concerns:
//   - Tests that acks get propagated or not as expected under different
//     'ClientSession' configurations when PUT message with non-unique GUID
//     is sent.
//
// Plan:
//   Instantiate a testbench, open a queue and send two PUTs with the same
//   GUID under different configurations.
//
// Testing:
//   That acks are propagated or not as expected.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS GUID COLLISION CONFIGURATIONS FOR CLIENT SESSION");

#define SCENARIO_RUN(var, from, to) for (int var = from; var <= to; ++var) {
#define END_SCENARIO }

    SCENARIO_RUN(clientType, e_FirstHop, e_Broker)
    SCENARIO_RUN(atMostOnce, e_AtLeastOnce, e_AtMostOnce)
    SCENARIO_RUN(ackRequested, e_AckNotRequested, e_AckRequested)

    testGuidCollision(static_cast<ClientType>(clientType),
                      static_cast<AtMostOnce>(atMostOnce),
                      static_cast<AckRequested>(ackRequested));

    END_SCENARIO
    END_SCENARIO
    END_SCENARIO

#undef SCENARIO_RUN
#undef END_SCENARIO
}

static void test4_ackRequestedNullCorrelationId()
// ------------------------------------------------------------------------
// TESTS ACK REQUESTED NULL CORRELATION ID
//
// Concerns:
//   - Tests that success ack doesn't get propagated as expected when it is
//     requested but PUT message contains null correlation id.
//
// Plan:
//   Instantiate a testbench, open a queue, send put and ack.
//
// Testing:
//   That ack is not propagated as expected.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS ACK REQUESTED NULL CORRELATION ID");

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    const int         queueId        = 4;  // A queue number
    const int         correlationId  = 0;  // A correlation id
    const bool        isAtMostOnce   = false;
    const bool        isAckRequested = true;
    bmqt::MessageGUID guid;  // Unset

    TestBench tb(client(e_FirstHopCorrelationIds),
                 isAtMostOnce,
                 s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    // Send PUT.
    tb.sendOldPut(queueId, guid, correlationId, isAckRequested);

    // Confirm that the OpenQueue response has been sent downstream.
    tb.d_cs.flush();
    tb.assertOpenQueueResponse();

    // Check if a message was sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT_EQ(1u, postMessages.size());
    // If a message with correlation id was sent upstream, grab the GUID.
    guid = postMessages[0].d_putHeader.messageGUID();
    ASSERT(!guid.isUnset());

    // ACK_REQUESTED flag should be set.
    const bool isAckRequestedSet = bmqp::PutHeaderFlagUtil::isSet(
        postMessages[0].d_putHeader.flags(),
        bmqp::PutHeaderFlags::e_ACK_REQUESTED);
    ASSERT(isAckRequestedSet);

    // Send an 'Ack' message.
    tb.sendAck(queueId, guid, bmqt::AckResult::e_SUCCESS);

    // Confirm that (if expected) the ack has been sent downstream.
    tb.d_cs.flush();
    tb.assertAckIsSentIfExpected(e_AckResultNone,
                                 queueId,
                                 guid,
                                 correlationId);
}

static void test5_ackNotRequestedNotNullCorrelationId()
// ------------------------------------------------------------------------
// TESTS ACK NOT REQUESTED NOT NULL CORRELATION ID
//
// Concerns:
//   - Tests that success ack gets propagated as expected when it is not
//     requested but PUT message contains not null correlation id.
//
// Plan:
//   Instantiate a testbench, open a queue, send put and ack.
//
// Testing:
//   That ack is propagated as expected.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS ACK NOT REQUESTED NOT NULL CORRELATION ID");

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    const int         queueId        = 4;  // A queue number
    const int         correlationId  = 2;  // A correlation id
    const bool        isAtMostOnce   = false;
    const bool        isAckRequested = false;
    bmqt::MessageGUID guid;  // Unset

    TestBench tb(client(e_FirstHopCorrelationIds),
                 isAtMostOnce,
                 s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    // Send PUT.
    tb.sendOldPut(queueId, guid, correlationId, isAckRequested);

    // Confirm that the OpenQueue response has been sent downstream.
    tb.d_cs.flush();
    tb.assertOpenQueueResponse();

    // Check if a message was sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT_EQ(1u, postMessages.size());
    // If a message with correlation id was sent upstream, grab the GUID.
    guid = postMessages[0].d_putHeader.messageGUID();
    ASSERT(!guid.isUnset());

    // ACK_REQUESTED flag should be set.
    const bool isAckRequestedSet = bmqp::PutHeaderFlagUtil::isSet(
        postMessages[0].d_putHeader.flags(),
        bmqp::PutHeaderFlags::e_ACK_REQUESTED);
    ASSERT(isAckRequestedSet);

    // Send an 'Ack' message.
    tb.sendAck(queueId, guid, bmqt::AckResult::e_SUCCESS);

    // Confirm that (if expected) the ack has been sent downstream.
    tb.d_cs.flush();
    tb.assertAckIsSentIfExpected(e_AckResultSuccess,
                                 queueId,
                                 guid,
                                 correlationId);
}

static void test6_firstHopUnsetGUID()
// ------------------------------------------------------------------------
// TESTS FIRST HOP UNSET GUID
//
// Concerns:
//   - Tests that PUT messages with unset GUIDs sent by SDK which generates
//     GUIDs are rejected under different configurations.
//
// Plan:
//   Instantiate a testbench, open a queue, send put.
//
// Testing:
//   That nack is propagated as expected.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("TESTS FIRST HOP UNSET GUID");

#define SCENARIO_RUN(var, from, to) for (int var = from; var <= to; ++var) {
#define END_SCENARIO }

    SCENARIO_RUN(atMostOnce, e_AtLeastOnce, e_AtMostOnce)
    SCENARIO_RUN(ackRequested, e_AckNotRequested, e_AckRequested)

    testFirstHopUnsetGUID(static_cast<AtMostOnce>(atMostOnce),
                          static_cast<AckRequested>(ackRequested));

    END_SCENARIO
    END_SCENARIO

#undef SCENARIO_RUN
#undef END_SCENARIO
}

static void test7_oldStylePut()
// ------------------------------------------------------------------------
// TESTS CONVERSION FROM OLD STYLE PUT TO THE NEW STYLE
//
// Concerns:
//   - Tests that old style PUT gets re-encoded MessageProperties.
//
// Plan:
//   Instantiate a testbench, open a queue, send put and observe Post.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("TESTS CONVERSION FROM OLD STYLE PUT");

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    const int         queueId        = 4;  // A queue number
    const int         correlationId  = 2;  // A correlation id
    const bool        isAtMostOnce   = false;
    const bool        isAckRequested = false;
    bmqt::MessageGUID guid;  // Unset

    TestBench tb(client(e_FirstHop), isAtMostOnce, s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    // Confirm that the OpenQueue response has been sent downstream.
    tb.d_cs.flush();
    tb.assertOpenQueueResponse();

    // Send PUT.
    bmqp::MessageProperties in(s_allocator_p);
    encode(&in);

    tb.sendOldPut(queueId, guid, correlationId, isAckRequested, in);

    // Check if a message was sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT_EQ(1u, postMessages.size());
    ASSERT(bmqp::PutHeaderFlagUtil::isSet(
        postMessages[0].d_putHeader.flags(),
        bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES));

    bmqp::MessageProperties out(s_allocator_p);
    const bdlbb::Blob*      payloadBlob = postMessages[0].d_appData.get();
    const bmqp::MessagePropertiesInfo& logic(postMessages[0].d_putHeader);
    ASSERT_EQ(0, out.streamIn(*payloadBlob, logic.isExtended()));
    ASSERT(!logic.isExtended());

    verify(out);

    // MessageProperties re-encoding happens in 'loadMesageProperties'
    // so no schema change

    int msgPropsAreaSize;
    bmqp::ProtocolUtil::readPropertiesSize(&msgPropsAreaSize,
                                           *payloadBlob,
                                           bmqu::BlobPosition());

    ASSERT(tb.validateData(*postMessages[0].d_appData, msgPropsAreaSize));

    // Turn around and send PUSH
    tb.sendPush(queueId,
                guid,
                postMessages[0].d_appData,
                bmqt::CompressionAlgorithmType::e_NONE,
                logic);
    tb.verifyPush(&out);

    verify(out);
}

static void test8_oldStyleCompressedPut()
// ------------------------------------------------------------------------
// TESTS CONVERSION FROM OLD STYLE COMPRESSED PUT TO THE NEW STYLE
//
// Concerns:
//   - Tests that old style PUT gets decompressed.
//
// Plan:
//   Instantiate a testbench, open a queue, send put and observe Post.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS OLD STYLE COMPRESSED PUT CONVERSION");

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    const int         queueId        = 4;  // A queue number
    const int         correlationId  = 2;  // A correlation id
    const bool        isAtMostOnce   = false;
    const bool        isAckRequested = false;
    bmqt::MessageGUID guid;  // Unset

    TestBench tb(client(e_FirstHop), isAtMostOnce, s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    // Confirm that the OpenQueue response has been sent downstream.
    tb.d_cs.flush();
    tb.assertOpenQueueResponse();

    // Send PUT.
    bmqp::MessageProperties in(s_allocator_p);
    encode(&in);

    tb.sendOldPut(queueId,
                  guid,
                  correlationId,
                  isAckRequested,
                  in,
                  bmqt::CompressionAlgorithmType::e_ZLIB);

    // Check if a message was sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT_EQ(1u, postMessages.size());
    ASSERT(bmqp::PutHeaderFlagUtil::isSet(
        postMessages[0].d_putHeader.flags(),
        bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES));

    ASSERT_EQ(postMessages[0].d_putHeader.compressionAlgorithmType(),
              bmqt::CompressionAlgorithmType::e_NONE);

    bmqp::MessageProperties out(s_allocator_p);
    const bdlbb::Blob*      payloadBlob = postMessages[0].d_appData.get();
    const bmqp::MessagePropertiesInfo& logic(postMessages[0].d_putHeader);

    ASSERT(!logic.isExtended());
    ASSERT_EQ(0, out.streamIn(*payloadBlob, logic.isExtended()));
    verify(out);

    int msgPropsAreaSize;
    bmqp::ProtocolUtil::readPropertiesSize(&msgPropsAreaSize,
                                           *payloadBlob,
                                           bmqu::BlobPosition());

    ASSERT(tb.validateData(*postMessages[0].d_appData, msgPropsAreaSize));

    // Turn around and send PUSH
    tb.sendPush(queueId,
                guid,
                postMessages[0].d_appData,
                bmqt::CompressionAlgorithmType::e_NONE,
                logic);

    tb.verifyPush(&out);

    verify(out);
}

static void test9_newStylePush()
// ------------------------------------------------------------------------
// TESTS CONVERSION FROM NEW STYLE PUSH TO THE OLD STYLE
//
// Concerns:
//   - Tests that new style PUSH gets re-encoded.
//
// Plan:
//   Instantiate a testbench, open a queue, send put, push and observe Push.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS OLD STYLE COMPRESSED PUT CONVERSION");

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    const int         queueId      = 4;  // A queue number
    const bool        isAtMostOnce = false;
    bmqt::MessageGUID guid         = bmqp::MessageGUIDGenerator::testGUID();

    TestBench tb(client(e_FirstHop), isAtMostOnce, s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    // Confirm that the OpenQueue response has been sent downstream.
    tb.d_cs.flush();
    tb.assertOpenQueueResponse();

    // Send PUT.
    bmqp::MessageProperties in(s_allocator_p);
    encode(&in);

    bmqp::PutEventBuilder peb(&tb.d_bufferFactory, s_allocator_p);
    bdlbb::Blob           payload(&tb.d_bufferFactory, s_allocator_p);

    bmqp::PutTester::populateBlob(&payload, 99);

    peb.startMessage();
    peb.setMessagePayload(&payload);
    peb.setMessageProperties(&in);
    peb.setMessageGUID(guid);

    peb.setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE);

    bmqt::EventBuilderResult::Enum rc = peb.packMessage(queueId);

    ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);

    mqbi::DispatcherEvent putEvent(s_allocator_p);
    bmqp::Event           rawEvent(&peb.blob(), s_allocator_p);

    BSLS_ASSERT(rawEvent.isValid());
    BSLS_ASSERT(rawEvent.isPutEvent());

    bmqp::PutMessageIterator putIt(&tb.d_bufferFactory, s_allocator_p);
    rawEvent.loadPutMessageIterator(&putIt, false);
    BSLS_ASSERT(putIt.next());

    bsl::shared_ptr<bdlbb::Blob> blobSp;
    blobSp.createInplace(s_allocator_p, &tb.d_bufferFactory, s_allocator_p);
    *blobSp = peb.blob();

    putEvent
        .setSource(&tb.d_cs)  // DispatcherClient *value
        .makePutEvent()
        .setIsRelay(true)  // Relay message
        .setPutHeader(putIt.header())
        .setBlob(blobSp);  // const bsl::shared_ptr<bdlbb::Blob>& value

    tb.dispatch(putEvent);

    // Check if a message was sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT_EQ(1u, postMessages.size());
    ASSERT(bmqp::PutHeaderFlagUtil::isSet(
        postMessages[0].d_putHeader.flags(),
        bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES));
    ASSERT(
        bmqp::MessagePropertiesInfo::hasSchema(postMessages[0].d_putHeader));
    ASSERT_EQ(postMessages[0].d_putHeader.compressionAlgorithmType(),
              bmqt::CompressionAlgorithmType::e_NONE);

    bmqp::MessageProperties out(s_allocator_p);
    const bdlbb::Blob*      payloadBlob = postMessages[0].d_appData.get();
    const bmqp::MessagePropertiesInfo& logic(postMessages[0].d_putHeader);
    ASSERT_EQ(0, out.streamIn(*payloadBlob, logic.isExtended()));
    verify(out);

    int msgPropsAreaSize;
    bmqp::ProtocolUtil::readPropertiesSize(&msgPropsAreaSize,
                                           *payloadBlob,
                                           bmqu::BlobPosition());

    ASSERT(tb.validateData(*postMessages[0].d_appData, msgPropsAreaSize, 99));

    // Turn around and send PUSH
    tb.sendPush(queueId,
                guid,
                postMessages[0].d_appData,
                bmqt::CompressionAlgorithmType::e_NONE,
                logic);
    tb.verifyPush(&out, 99);

    verify(out);
}

static void test10_newStyleCompressedPush()
// ------------------------------------------------------------------------
// TESTS CONVERSION FROM NEW STYLE PUSH TO THE OLD STYLE
//
// Concerns:
//   - Tests that new style PUSH gets re-encoded.
//
// Plan:
//   Instantiate a testbench, open a queue, send put, push and observe Push.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS OLD STYLE COMPRESSED PUT CONVERSION");

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    const int         queueId      = 4;  // A queue number
    const bool        isAtMostOnce = false;
    bmqt::MessageGUID guid         = bmqp::MessageGUIDGenerator::testGUID();

    TestBench tb(client(e_FirstHop), isAtMostOnce, s_allocator_p);

    // Send an 'OpenQueue` request.
    tb.openQueue(uri, queueId);

    // Confirm that the OpenQueue response has been sent downstream.
    tb.d_cs.flush();
    tb.assertOpenQueueResponse();

    // Send PUT.
    bmqp::MessageProperties in(s_allocator_p);
    encode(&in);

    bmqp::PutEventBuilder peb(&tb.d_bufferFactory, s_allocator_p);
    bdlbb::Blob           payload(&tb.d_bufferFactory, s_allocator_p);

    bmqp::PutTester::populateBlob(
        &payload,
        2 * bmqp::Protocol::k_COMPRESSION_MIN_APPDATA_SIZE);

    peb.startMessage();
    peb.setMessagePayload(&payload);
    peb.setMessageProperties(&in);
    peb.setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::e_ZLIB);
    peb.setMessageGUID(guid);

    bmqt::EventBuilderResult::Enum rc = peb.packMessage(queueId);

    ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);

    mqbi::DispatcherEvent putEvent(s_allocator_p);
    bmqp::Event           rawEvent(&peb.blob(), s_allocator_p);

    BSLS_ASSERT(rawEvent.isValid());
    BSLS_ASSERT(rawEvent.isPutEvent());

    bmqp::PutMessageIterator putIt(&tb.d_bufferFactory, s_allocator_p);
    rawEvent.loadPutMessageIterator(&putIt, false);
    BSLS_ASSERT(putIt.next());

    bsl::shared_ptr<bdlbb::Blob> blobSp;
    blobSp.createInplace(s_allocator_p, &tb.d_bufferFactory, s_allocator_p);
    *blobSp = peb.blob();

    putEvent
        .setSource(&tb.d_cs)  // DispatcherClient *value
        .makePutEvent()
        .setIsRelay(true)  // Relay message
        .setPutHeader(putIt.header())
        .setBlob(blobSp);  // const bsl::shared_ptr<bdlbb::Blob>& value

    tb.dispatch(putEvent);

    // Check if a message was sent
    const bsl::vector<MyMockQueueHandle::Post>& postMessages =
        tb.d_domain.d_queueHandle->postedMessages();
    ASSERT_EQ(1u, postMessages.size());
    ASSERT(bmqp::PutHeaderFlagUtil::isSet(
        postMessages[0].d_putHeader.flags(),
        bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES));
    ASSERT(
        bmqp::MessagePropertiesInfo::hasSchema(postMessages[0].d_putHeader));
    ASSERT_EQ(postMessages[0].d_putHeader.compressionAlgorithmType(),
              bmqt::CompressionAlgorithmType::e_ZLIB);

    bmqp::MessageProperties out(s_allocator_p);
    const bdlbb::Blob*      payloadBlob = postMessages[0].d_appData.get();
    const bmqp::MessagePropertiesInfo& logic(postMessages[0].d_putHeader);

    ASSERT_EQ(0, out.streamIn(*payloadBlob, logic.isExtended()));
    verify(out);

    // No payload validation, it is compressed.

    // Turn around and send PUSH
    tb.sendPush(queueId,
                guid,
                postMessages[0].d_appData,
                bmqt::CompressionAlgorithmType::e_ZLIB,
                logic);
    tb.verifyPush(&out, 2 * bmqp::Protocol::k_COMPRESSION_MIN_APPDATA_SIZE);

    verify(out);
}

static void test11_initiateShutdown()
// ------------------------------------------------------------------------
// TESTS SHUTTING DOWN
//
// Concerns:
//   - Verify that client session with and without unconfirmed messages can
//     be shut down within the specified timeout.
//
// Plan:
//   Instantiate a testbench, open a queue, check that a shutdown callback
//   is invoked when the session has or has no unconfirmed messages.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("TESTS SHUTTING DOWN");

    const bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);

    const int                queueId          = 4;  // A queue number
    const bool               isAtMostOnce     = false;
    const bsls::TimeInterval timeout          = bsls::TimeInterval(10);
    const bsls::TimeInterval semaphoreTimeout = bsls::TimeInterval(
        0,
        100000000);  // 100 ms
    bslmt::TimedSemaphore semaphore;
    bmqt::MessageGUID     guid = bmqp::MessageGUIDGenerator::testGUID();
    mqbi::StorageMessageAttributes messageAttributes;
    bmqp::Protocol::MsgGroupId     msgGroupId(s_allocator_p);
    const unsigned int             subscriptionId =
        bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;
    const unsigned int subQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;

    bmqp::Protocol::SubQueueInfosArray subQueueInfos(
        1,
        bmqp::SubQueueInfo(subscriptionId),
        s_allocator_p);
    bsl::shared_ptr<bdlbb::Blob> blob;

    PV("Shutdown without unconfirmed messages");
    {
        TestBench       tb(client(e_FirstHop), isAtMostOnce, s_allocator_p);
        bsls::AtomicInt callbackCounter(0);

        // Send an 'OpenQueue` request.
        tb.openQueue(uri, queueId);

        // Confirm that the OpenQueue response has been sent downstream.
        tb.d_cs.flush();
        tb.assertOpenQueueResponse();

        // Verify there are no unconfirmed messages in the handle
        ASSERT_EQ(0, tb.d_domain.d_queueHandle->countUnconfirmed());

        // Initiate client session shutdown
        tb.d_cs.initiateShutdown(bdlf::BindUtil::bind(&onShutdownComplete,
                                                      &callbackCounter,
                                                      &semaphore),
                                 timeout);

        // Verify 'Channel::close' call
        ASSERT_EQ(tb.d_channel->closeCalls().size(), 1UL);

        // Imitate close operation
        tb.d_cs.tearDown(bsl::shared_ptr<void>(), true);

        // Verify that the shutdown callback gets called
        semaphore.wait();

        ASSERT_EQ(callbackCounter, 1);
    }

    PV("Shutdown with unconfirmed messages and timeout");
    {
        TestBench       tb(client(e_FirstHop), isAtMostOnce, s_allocator_p);
        bsls::AtomicInt callbackCounter(0);

        // Send an 'OpenQueue` request.
        tb.openQueue(uri, queueId);

        // Confirm that the OpenQueue response has been sent downstream.
        tb.d_cs.flush();
        tb.assertOpenQueueResponse();

        // Emulate sending PUSH.
        tb.d_domain.d_queueHandle->deliverMessage(blob,
                                                  guid,
                                                  messageAttributes,
                                                  msgGroupId,
                                                  subQueueInfos,
                                                  false);

        // Verify there are unconfirmed messages in the handle
        ASSERT_EQ(1, tb.d_domain.d_queueHandle->countUnconfirmed());

        // Initiate client session shutdown
        tb.d_cs.initiateShutdown(bdlf::BindUtil::bind(&onShutdownComplete,
                                                      &callbackCounter,
                                                      &semaphore),
                                 timeout);

        // No 'Channel::close' call
        ASSERT_EQ(tb.d_channel->closeCalls().size(), 0UL);

        // Verify that the shutdown callback hasn't been called
        int rc = semaphore.timedWait(
            bsls::SystemTime::now(bsls::SystemClockType::e_REALTIME) +
            semaphoreTimeout);

        ASSERT_NE(rc, 0);
        ASSERT_EQ(callbackCounter, 0);

        // Advance time to reach the shutdown timeout
        tb.d_testClock.d_timeSource.advanceTime(timeout);

        // Verify 'Channel::close' call
        ASSERT_EQ(tb.d_channel->closeCalls().size(), 1UL);

        // Imitate close operation
        tb.d_cs.tearDown(bsl::shared_ptr<void>(), true);

        // Verify that the shutdown callback gets called
        semaphore.wait();

        ASSERT_EQ(callbackCounter, 1);
    }

    PV("Confirm a messsage while shutting down");
    {
        TestBench       tb(client(e_FirstHop), isAtMostOnce, s_allocator_p);
        bsls::AtomicInt callbackCounter(0);

        // Send an 'OpenQueue` request.
        tb.openQueue(uri, queueId);

        // Confirm that the OpenQueue response has been sent downstream.
        tb.d_cs.flush();
        tb.assertOpenQueueResponse();

        // Emulate sending PUSH.
        tb.d_domain.d_queueHandle->deliverMessage(blob,
                                                  guid,
                                                  messageAttributes,
                                                  msgGroupId,
                                                  subQueueInfos,
                                                  false);

        // Verify there are unconfirmed messages in the handle
        ASSERT_EQ(1, tb.d_domain.d_queueHandle->countUnconfirmed());

        // Initiate client session shutdown
        tb.d_cs.initiateShutdown(bdlf::BindUtil::bind(&onShutdownComplete,
                                                      &callbackCounter,
                                                      &semaphore),
                                 timeout + timeout);
        // Long shutdown timeout

        // No 'Channel::close' call
        ASSERT_EQ(tb.d_channel->closeCalls().size(), 0UL);

        // Verify that the shutdown callback hasn't been called
        int rc = semaphore.timedWait(
            bsls::SystemTime::now(bsls::SystemClockType::e_REALTIME) +
            semaphoreTimeout);

        ASSERT_NE(rc, 0);
        ASSERT_EQ(callbackCounter, 0);

        tb.d_domain.d_queueHandle->confirmMessage(guid, subQueueId);

        // Verify there are no unconfirmed messages in the handle
        ASSERT_EQ(0, tb.d_domain.d_queueHandle->countUnconfirmed());

        // Advance time less than the shutdown timeout
        tb.d_testClock.d_timeSource.advanceTime(timeout);

        // Verify 'Channel::close' call
        ASSERT_EQ(tb.d_channel->closeCalls().size(), 1UL);

        // Imitate close operation
        tb.d_cs.tearDown(bsl::shared_ptr<void>(), true);

        // Verify that the shutdown callback gets called
        semaphore.wait();

        ASSERT_EQ(callbackCounter, 1);
    }

    PV("Confirm multiple messsages while shutting down");
    {
        const int       NUM_MESSAGES = 5;
        TestBench       tb(client(e_FirstHop), isAtMostOnce, s_allocator_p);
        bsls::AtomicInt callbackCounter(0);
        bsl::vector<bmqt::MessageGUID> guids(s_allocator_p);
        guids.reserve(NUM_MESSAGES);

        // Send an 'OpenQueue` request.
        tb.openQueue(uri, queueId);

        // Confirm that the OpenQueue response has been sent downstream.
        tb.d_cs.flush();
        tb.assertOpenQueueResponse();

        // Emulate sending PUSHs.
        for (int i = 0; i < NUM_MESSAGES; ++i) {
            guid = bmqp::MessageGUIDGenerator::testGUID();
            tb.d_domain.d_queueHandle->deliverMessage(blob,
                                                      guid,
                                                      messageAttributes,
                                                      msgGroupId,
                                                      subQueueInfos,
                                                      false);
            guids.push_back(guid);
        }
        // Verify there are unconfirmed messages in the handle
        ASSERT_EQ(NUM_MESSAGES, tb.d_domain.d_queueHandle->countUnconfirmed());

        // Initiate client session shutdown
        tb.d_cs.initiateShutdown(bdlf::BindUtil::bind(&onShutdownComplete,
                                                      &callbackCounter,
                                                      &semaphore),
                                 timeout + timeout + timeout);
        // Long shutdown timeout

        // No 'Channel::close' call
        ASSERT_EQ(tb.d_channel->closeCalls().size(), 0UL);

        // Verify that the shutdown callback hasn't been called
        int rc = semaphore.timedWait(
            bsls::SystemTime::now(bsls::SystemClockType::e_REALTIME) +
            semaphoreTimeout);

        ASSERT_NE(rc, 0);
        ASSERT_EQ(callbackCounter, 0);

        // Confirm NUM_MESSAGES - 1 messages
        for (int i = 0; i < NUM_MESSAGES - 1; ++i) {
            tb.d_domain.d_queueHandle->confirmMessage(guids[i], subQueueId);
        }

        // Verify there is one unconfirmed message in the handle
        ASSERT_EQ(1, tb.d_domain.d_queueHandle->countUnconfirmed());

        // Advance time less than the shutdown timeout
        tb.d_testClock.d_timeSource.advanceTime(timeout);

        // No 'Channel::close' call
        ASSERT_EQ(tb.d_channel->closeCalls().size(), 0UL);

        // Still no callback
        rc = semaphore.timedWait(
            bsls::SystemTime::now(bsls::SystemClockType::e_REALTIME) +
            semaphoreTimeout);

        ASSERT_NE(rc, 0);
        ASSERT_EQ(callbackCounter, 0);

        // Confirm the last message
        tb.d_domain.d_queueHandle->confirmMessage(guids[NUM_MESSAGES - 1],
                                                  subQueueId);

        ASSERT_EQ(0, tb.d_domain.d_queueHandle->countUnconfirmed());

        // Advance time less than the shutdown timeout
        tb.d_testClock.d_timeSource.advanceTime(timeout);

        // Verify 'Channel::close' call
        ASSERT_EQ(tb.d_channel->closeCalls().size(), 1UL);

        // Imitate close operation
        tb.d_cs.tearDown(bsl::shared_ptr<void>(), true);

        // Verify that the shutdown callback gets called
        semaphore.wait();

        ASSERT_EQ(callbackCounter, 1);
    }
}

static void testN1_ackConfiguration()
// ------------------------------------------------------------------------
// TESTS ACK CONFIGURATION FOR CLIENT SESSION
//
// Concerns:
//   - Tests that acks get propagated as expected under specific
//     'ClientSession' configuration.
//
// Plan:
//   Instantiate a testbench, open a queue, send put and ack under specific
//   configuration.
//
// Testing:
//   That acks are propagated as expected.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS ACK CONFIGURATION FOR CLIENT SESSION");

    cout << "Please enter the configuration to test followed by\n"
         << "<enter> when done:\n"
         << "  clientType atMostOnce putAction ackRequested ackSuccess\n\n"
         << "clientType  : from " << e_FirstHop << " to " << e_Broker << "\n"
         << "atMostOnce  : from " << e_AtLeastOnce << " to " << e_AtMostOnce
         << "\n"
         << "putAction   : from " << e_DoNotPut << " to " << e_DoPut << "\n"
         << "ackRequested: from " << e_AckNotRequested << " to "
         << e_AckRequested << "\n"
         << "ackSuccess  : from " << e_AckFailure << " to " << e_AckSuccess
         << endl;

    // Read from stdin
    char buffer[10];
    bsl::cin.getline(buffer, 10, '\n');

    bsl::istringstream is(buffer);

    int clientType   = 0;
    int atMostOnce   = 0;
    int putAction    = 0;
    int ackRequested = 0;
    int ackSuccess   = 0;

    is >> clientType;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    is >> atMostOnce;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    is >> putAction;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    is >> ackRequested;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    is >> ackSuccess;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    test(static_cast<ClientType>(clientType),
         static_cast<AtMostOnce>(atMostOnce),
         static_cast<PutAction>(putAction),
         static_cast<AckRequested>(ackRequested),
         static_cast<AckSuccess>(ackSuccess));
}

static void testN2_invalidPutConfiguration()
// ------------------------------------------------------------------------
// TESTS INVALID PUT CONFIGURATION FOR CLIENT SESSION
//
// Concerns:
//   - Tests that acks get propagated as expected under specific
//     'ClientSession' configuration.
//
// Plan:
//   Instantiate a testbench, open a queue and send invalid put under
//   specific configuration.
//
// Testing:
//   That acks are propagated as expected.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS INVALID PUT CONFIGURATION FOR CLIENT SESSION");

    cout << "Please enter the configuration to test followed by\n"
         << "<enter> when done:\n"
         << "  clientType atMostOnce ackRequested invalidPut\n\n"
         << "clientType  : from " << e_FirstHop << " to " << e_Broker << "\n"
         << "atMostOnce  : from " << e_AtLeastOnce << " to " << e_AtMostOnce
         << "\n"
         << "ackRequested: from " << e_AckNotRequested << " to "
         << e_AckRequested << "\n"
         << "invalidPut  : from " << e_InvalidPutUnknownQueue << " to "
         << e_InvalidPutClosedQueue << "\n"
         << endl;

    // Read from stdin
    char buffer[8];
    bsl::cin.getline(buffer, 8, '\n');

    bsl::istringstream is(buffer);

    int clientType   = 0;
    int atMostOnce   = 0;
    int ackRequested = 0;
    int invalidPut   = 0;

    is >> clientType;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    is >> atMostOnce;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    is >> ackRequested;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    is >> invalidPut;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    testInvalidPut(static_cast<ClientType>(clientType),
                   static_cast<AtMostOnce>(atMostOnce),
                   static_cast<AckRequested>(ackRequested),
                   static_cast<InvalidPut>(invalidPut));
}

static void testN3_guidCollisionConfiguration()
// TESTS GUID COLLISION CONFIGURATION FOR CLIENT SESSION
//
// Concerns:
//   - Tests that acks get propagated or not as expected under specific
//     'ClientSession' configuration when PUT message with non-unique GUID
//     is sent.
//
// Plan:
//   Instantiate a testbench, open a queue and send two PUTs with the same
//   GUID under specific configuration.
//
// Testing:
//   That acks are propagated or not as expected.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "TESTS GUID COLLISION CONFIGURATION FOR CLIENT SESSION");

    cout << "Please enter the configuration to test followed by\n"
         << "<enter> when done:\n"
         << "  clientType atMostOnce ackRequested\n\n"
         << "clientType  : from " << e_FirstHop << " to " << e_Broker << "\n"
         << "atMostOnce  : from " << e_AtLeastOnce << " to " << e_AtMostOnce
         << "\n"
         << "ackRequested: from " << e_AckNotRequested << " to "
         << e_AckRequested << "\n"
         << endl;

    // Read from stdin
    char buffer[8];
    bsl::cin.getline(buffer, 8, '\n');

    bsl::istringstream is(buffer);

    int clientType   = 0;
    int atMostOnce   = 0;
    int ackRequested = 0;

    is >> clientType;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    is >> atMostOnce;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    is >> ackRequested;
    if (is.fail()) {
        cout << "The input '" << buffer << "' is not properly formatted."
             << endl;
        return;  // RETURN
    }

    testGuidCollision(static_cast<ClientType>(clientType),
                      static_cast<AtMostOnce>(atMostOnce),
                      static_cast<AckRequested>(ackRequested));
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(s_allocator_p);
    bmqp::Crc32c::initialize();

    {
        bmqp::ProtocolUtil::initialize(s_allocator_p);
        bmqsys::Time::initialize(s_allocator_p);

        mqbcfg::AppConfig brokerConfig(s_allocator_p);
        brokerConfig.brokerVersion() = 999999;  // required for test case 8
                                                // to convert msg properties
                                                // from v1 to v2
        mqbcfg::BrokerConfig::set(brokerConfig);

        bsl::shared_ptr<bmqst::StatContext> statContext =
            mqbstat::BrokerStatsUtil::initializeStatContext(30, s_allocator_p);

        mqbu::MessageGUIDUtil::initialize();

        switch (_testCase) {
        case 0:
        case 11: test11_initiateShutdown(); break;
        case 10: test10_newStyleCompressedPush(); break;
        case 9: test9_newStylePush(); break;
        case 8: test8_oldStyleCompressedPut(); break;
        case 7: test7_oldStylePut(); break;
        case 6: test6_firstHopUnsetGUID(); break;
        case 5: test5_ackNotRequestedNotNullCorrelationId(); break;
        case 4: test4_ackRequestedNullCorrelationId(); break;
        case 3: test3_guidCollisionConfigurations(); break;
        case 2: test2_invalidPutConfigurations(); break;
        case 1: test1_ackConfigurations(); break;
        case -1: testN1_ackConfiguration(); break;
        case -2: testN2_invalidPutConfiguration(); break;
        case -3: testN3_guidCollisionConfiguration(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            s_testStatus = -1;
        } break;
        }

        bmqsys::Time::shutdown();
        bmqp::ProtocolUtil::shutdown();
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
    // Do not check for default/global allocator usage.
}
