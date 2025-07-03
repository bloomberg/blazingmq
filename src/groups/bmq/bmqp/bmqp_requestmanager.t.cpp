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

// bmqp_requestmanager.t.cpp                                          -*-C++-*-
#include <bmqp_requestmanager.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_messageproperties.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_queueid.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdld_datum.h>
#include <bsl_queue.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslmt_barrier.h>
#include <bslmt_threadgroup.h>
#include <bsls_atomic.h>

// TEST DRIVER
#include <bmqio_testchannel.h>
#include <bmqtst_testhelper.h>
#include <bsl_cstddef.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
// - creatorsTest
// - setExecutorTest
// - createRequestTest
// - sendRequestTest
// - processResponseTest
// - cancelAllRequestsTest
//-----------------------------------------------------------------------------
// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef bmqp_ctrlmsg::ControlMessage       Mes;
typedef bsl::queue<Mes>                    MesQue;
typedef bmqp::RequestManager<Mes, Mes>     ReqManagerType;
typedef ReqManagerType::RequestType        Req;
typedef ReqManagerType::RequestSp          ReqSp;
typedef bsl::vector<ReqSp>                 ReqVec;
typedef bmqp_ctrlmsg::ControlMessageChoice ReqChoice;
typedef bsl::unordered_set<ReqChoice>      ReqChoiceSet;

const bsls::TimeInterval SEND_REQUEST_TIMEOUT(30);
const bsls::Types::Int64 WATERMARK = 64 * 1024 * 1024;

}  // close unnamed namespace

struct TestClock {
    // DATA
    bdlmt::EventScheduler d_scheduler;

    bdlmt::EventSchedulerTestTimeSource d_timeSource;

    // CREATORS

    /// Constructs TestClock object with the optionally specified
    /// `allocator`.
    explicit TestClock(bslma::Allocator* allocator = 0)
    : d_scheduler(bsls::SystemClockType::e_MONOTONIC, allocator)
    , d_timeSource(&d_scheduler)
    {
    }

    // MANIPULATORS

    /// Return the value of this time interval as an integral number of
    /// nanoseconds
    bsls::Types::Int64 highResTimer()
    {
        return d_timeSource.now().totalNanoseconds();
    }

    /// Return the value of this time interval.
    bsls::TimeInterval monotonicClock() { return d_timeSource.now(); }

    /// Return the value of this time interval.
    bsls::TimeInterval realtimeClock() { return d_timeSource.now(); }
};

/// Buffer factory provided to the various builders
class TestContext {
    bdlbb::PooledBlobBufferFactory d_blobBufferFactory;

    /// Blob pool used to provide blobs to event builders.
    bmqp::BlobPoolUtil::BlobSpPoolSp d_blobSpPool_sp;

    TestClock d_testClock;
    // Pointer to struct to initialize system time

    // Event scheduler used in the request manager
    bdlmt::EventScheduler& d_scheduler;

    // Mocked network channel object to be used in the request manager
    bmqio::TestChannel d_testChannel;

    // RequestManager object under testing
    ReqManagerType d_requestManager;

    // Allocator to use
    bslma::Allocator* d_allocator_p;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TestContext, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct TestContext object with the specified `lateResponseMode`
    /// and the optionally specified `allocator`.
    explicit TestContext(bool              lateResponseMode,
                         bslma::Allocator* allocator = 0);

    /// Destructor of TestContext object.  Stop the scheduler and cancel
    /// all the requests.
    ~TestContext();

    // ACCESSORS

    /// Return pointer to the used allocator
    bslma::Allocator* allocator() const;

    /// Return reference to the mocked network channel object.
    bmqio::TestChannel& channel();

    /// Return reference to the buffer factory
    bdlbb::PooledBlobBufferFactory& factory();

    /// Return reference to the RequestManager object.
    ReqManagerType& manager();

    /// Return reference to the scheduler object.
    bdlmt::EventScheduler& scheduler();

    // MANIPULATORS

    /// Advance time with the specified `amount`
    void advanceTime(bsls::TimeInterval amount);

    /// Cancel all pending requests
    void cancelRequests();

    /// Create new request object and return shared pointer to it.
    ReqSp createRequest();

    /// Create new response object with the specified `id` and return it.
    Mes createResponse(int id);

    /// Create new response object with CANCEL status and return it.
    Mes createResponseCancel();

    /// Return last message, sent by RequestManager.
    Mes getNextRequest();

    /// Set custom values to some fields of the specified `request` object
    /// from the optionally specified `uri`, the optionally specified
    /// `flags`, the optionally specified `qId`, the optionally specified
    /// `readCount`, the optionally specified `writeCount` and the
    /// optionally specified `adminCount`
    static void populateRequest(const ReqSp&        request,
                                const bsl::string&  uri   = "bmq://foo.bar",
                                bsls::Types::Uint64 flags = 0,
                                unsigned int        qId   = 0,
                                int                 readCount  = 0,
                                int                 writeCount = 1,
                                int                 adminCount = 0);

    /// Send the specified `request` and the specified `sendFn`.
    void sendCallbackRequest(const ReqSp&                  request,
                             const ReqManagerType::SendFn& sendFn);

    /// Send the specified `request` to the hardcoded TestChannel object.
    void sendChannelRequest(const ReqSp& request);
};

TestContext::TestContext(bool lateResponseMode, bslma::Allocator* allocator)
: d_blobBufferFactory(1024, allocator)
, d_blobSpPool_sp(
      bmqp::BlobPoolUtil::createBlobPool(&d_blobBufferFactory, allocator))
, d_testClock(allocator)
, d_scheduler(d_testClock.d_scheduler)
, d_testChannel(allocator)
, d_requestManager(bmqp::EventType::e_CONTROL,
                   d_blobSpPool_sp.get(),
                   &d_scheduler,
                   lateResponseMode,
                   allocator)
, d_allocator_p(allocator)
{
    bmqsys::Time::shutdown();
    bmqsys::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, &d_testClock),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, &d_testClock),
        bdlf::BindUtil::bind(&TestClock::highResTimer, &d_testClock),
        d_allocator_p);

    int rc = d_scheduler.start();
    BMQTST_ASSERT_EQ(rc, 0);
}

TestContext::~TestContext()
{
    d_scheduler.cancelAllEventsAndWait();
    d_scheduler.stop();
    cancelRequests();
}

// ACCESSORS
bslma::Allocator* TestContext::allocator() const
{
    return d_allocator_p;
}

bmqio::TestChannel& TestContext::channel()
{
    return d_testChannel;
}

bdlbb::PooledBlobBufferFactory& TestContext::factory()
{
    return d_blobBufferFactory;
}

ReqManagerType& TestContext::manager()
{
    return d_requestManager;
}

bdlmt::EventScheduler& TestContext::scheduler()
{
    return d_scheduler;
}

// MANIPULATORS
void TestContext::advanceTime(bsls::TimeInterval amount)
{
    d_testClock.d_timeSource.advanceTime(amount);
}

void TestContext::cancelRequests()
{
    Mes reason = createResponseCancel();
    d_requestManager.cancelAllRequests(reason);
}

ReqSp TestContext::createRequest()
{
    return d_requestManager.createRequest();
}

Mes TestContext::createResponse(int id)
{
    Mes                   response(d_allocator_p);
    bmqp_ctrlmsg::Status& status = response.choice().makeStatus();
    status.category()            = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    status.code()                = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    status.message()             = bsl::string("Test", d_allocator_p);
    response.rId()               = id;

    return response;
}

Mes TestContext::createResponseCancel()
{
    Mes                   reason(d_allocator_p);
    bmqp_ctrlmsg::Status& status = reason.choice().makeStatus();
    status.category()            = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
    status.code()                = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
    status.message()             = bsl::string("Test", d_allocator_p);

    return reason;
}

Mes TestContext::getNextRequest()
{
    BMQTST_ASSERT(d_testChannel.waitFor(1, true, bsls::TimeInterval(1)));
    bmqio::TestChannel::WriteCall wc = d_testChannel.popWriteCall();
    bmqp::Event                   ev(&wc.d_blob, d_allocator_p);
    BMQTST_ASSERT(ev.isControlEvent());
    Mes controlMessage(d_allocator_p);
    BMQTST_ASSERT_EQ(0, ev.loadControlEvent(&controlMessage));
    return controlMessage;
}

void TestContext::populateRequest(const ReqSp&        request,
                                  const bsl::string&  uri,
                                  bsls::Types::Uint64 flags,
                                  unsigned int        qId,
                                  int                 readCount,
                                  int                 writeCount,
                                  int                 adminCount)
{
    bmqp_ctrlmsg::OpenQueue& req = request->request().choice().makeOpenQueue();

    bmqp_ctrlmsg::QueueHandleParameters params(
        bmqtst::TestHelperUtil::allocator());

    bmqt::QueueFlagsUtil::setWriter(&flags);
    bmqt::QueueFlagsUtil::setAck(&flags);

    params.uri()        = uri;
    params.flags()      = flags;
    params.qId()        = qId;
    params.readCount()  = readCount;
    params.writeCount() = writeCount;
    params.adminCount() = adminCount;

    req.handleParameters() = params;
}

void TestContext::sendCallbackRequest(const ReqSp&                  request,
                                      const ReqManagerType::SendFn& sendFn)
{
    bmqt::GenericResult::Enum rc = manager().sendRequest(
        request,
        sendFn,
        bsl::string("foo", d_allocator_p),
        SEND_REQUEST_TIMEOUT);
    BMQTST_ASSERT_EQ(rc, bmqt::GenericResult::e_SUCCESS);
}

void TestContext::sendChannelRequest(const ReqSp& request)
{
    bmqt::GenericResult::Enum rc = manager().sendRequest(
        request,
        &d_testChannel,
        bsl::string("foo", d_allocator_p),
        SEND_REQUEST_TIMEOUT,
        WATERMARK);
    BMQTST_ASSERT_EQ(rc, bmqt::GenericResult::e_SUCCESS);
}

/// Check that the specified `request` is the same request as the
/// specified `expected` one, and set the specified `called` flag to
/// true.
struct Caller {
    static void
    callback(bool* called, const ReqSp& request, const ReqSp* expected)
    {
        *called = true;
        BMQTST_ASSERT_EQ(request->request(), (*expected)->request());
    }
};

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_creatorsTest()
// ------------------------------------------------------------------------
// Testing:
//    RequestManager::RequestManager()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CREATORS TEST");

    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        4096,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));

    {
        // Wrong clock type
        bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_REALTIME,
                                        bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_SAFE_FAIL(
            ReqManagerType(bmqp::EventType::e_CONTROL,
                           blobSpPool.get(),
                           &scheduler,
                           false,  // late response mode is off
                           bmqtst::TestHelperUtil::allocator()));
    }

    {
        // Success creation
        bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                        bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_PASS(
            ReqManagerType(bmqp::EventType::e_CONTROL,
                           blobSpPool.get(),
                           &scheduler,
                           false,  // late response mode is off
                           bmqtst::TestHelperUtil::allocator()));

        BMQTST_ASSERT_PASS(
            ReqManagerType(bmqp::EventType::e_CONTROL,
                           blobSpPool.get(),
                           &scheduler,
                           false,  // late response mode is off
                           bmqex::SystemExecutor(),
                           bmqtst::TestHelperUtil::allocator()));

        BMQTST_ASSERT_PASS(
            ReqManagerType(bmqp::EventType::e_CONTROL,
                           blobSpPool.get(),
                           &scheduler,
                           false,  // late response mode is off
                           bmqex::SystemExecutor(),
                           ReqManagerType::DTContextSp(),
                           bmqtst::TestHelperUtil::allocator()));
    }
}

static void test2_setExecutorTest()
// ------------------------------------------------------------------------
// Testing:
//    void RequestManager::setExecutor()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SET EXECUTOR TEST");

    {
        // set SystemExecutor
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_PASS(
            context.manager().setExecutor(bmqex::SystemExecutor()));
    }
}

static void test3_createRequestTest()
// ------------------------------------------------------------------------
// Testing:
//    RequestSp RequestManager::createRequest();
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CREATE REQUEST TEST");

    {
        // Check that RequestManager really creates request
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        BMQTST_ASSERT(request);
    }
}

static void test4_sendRequestTest()
// ------------------------------------------------------------------------
// Testing:
//    void RequestManager::sendRequest();
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEND REQUEST TEST");

    /// Check that the specified `blob` contains the same request as the
    /// specified `request`, and set the specified `called` flag to true
    struct Caller {
        static bmqt::GenericResult::Enum
        sendFn(bool*                               called,
               const bsl::shared_ptr<bdlbb::Blob>& blob_sp,
               const ReqSp&                        request)
        {
            *called = true;
            bmqp::Event ev(blob_sp.get(), bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT(ev.isControlEvent());
            Mes controlMessage(bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_EQ(0, ev.loadControlEvent(&controlMessage));
            BMQTST_ASSERT_EQ(request->request(), controlMessage);
            return bmqt::GenericResult::e_SUCCESS;
        }
    };

    {
        // Check one request sent to channel
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        context.populateRequest(request);
        context.sendChannelRequest(request);

        // checking, RequestManager has really sent the request to testChannel
        BMQTST_ASSERT(request->request().rId().has_value());
        Mes controlMessage = context.getNextRequest();
        BMQTST_ASSERT_EQ(request->request(), controlMessage);
    }

    {
        // Check several requests sent to channel in correct order
        TestContext       context(false, bmqtst::TestHelperUtil::allocator());
        const bsl::size_t num_requests = 5;

        ReqVec requests(bmqtst::TestHelperUtil::allocator());
        requests.reserve(num_requests);
        for (bsl::size_t i = 0; i < num_requests; ++i) {
            ReqSp              request = context.createRequest();
            bsl::ostringstream os(bmqtst::TestHelperUtil::allocator());
            os << "bmq://foo.bar" << i;
            context.populateRequest(request,
                                    os.str(),
                                    i + 1,
                                    i + 2,
                                    i + 3,
                                    i + 4);
            context.sendChannelRequest(request);
            requests.emplace_back(request);
        }

        for (bsl::size_t i = 0; i < num_requests; ++i) {
            // checking, that RequestManager has really sent all the requests
            // to testChannel
            const ReqSp& request        = requests[i];
            Mes          controlMessage = context.getNextRequest();
            BMQTST_ASSERT_EQ(request->request().rId(), controlMessage.rId());
            BMQTST_ASSERT_EQ(request->request(), controlMessage);
        }
    }

    {
        // Check one request sent with provided SendFunction
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        context.populateRequest(request);
        bool called = false;

        context.sendCallbackRequest(
            request,
            bdlf::BindUtil::bind(Caller::sendFn,
                                 &called,
                                 bdlf::PlaceHolders::_1,
                                 request));
        BMQTST_ASSERT(called);
    }

    {
        // Concurrency
        TestContext  context(false, bmqtst::TestHelperUtil::allocator());
        ReqChoiceSet requestsWithoutId;
        // We store only 'choices' because request obtains id only after it has
        // been sent
        const bsl::size_t  numThreads  = 10;
        const bsl::size_t  numRequests = 100;
        bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());
        bslmt::Barrier     barrier(numThreads + 1);

        struct Caller {
            static void requestSenderJob(ReqVec          requests,
                                         bslmt::Barrier* barrier,
                                         TestContext*    context)
            // ------------------------------------------------------------
            // Wait on the specified 'barrier' and then send the specified
            // 'requests' to the channel in the specified 'context'
            // ------------------------------------------------------------
            {
                barrier->wait();
                for (ReqVec::const_iterator it = requests.begin();
                     it != requests.end();
                     ++it) {
                    context->sendChannelRequest(*it);
                }
            }
        };

        for (bsl::size_t i = 0; i < numThreads; ++i) {
            ReqVec requestsGroup(bmqtst::TestHelperUtil::allocator());
            requestsGroup.reserve(numRequests);
            for (bsl::size_t j = 0; j < numRequests; ++j) {
                ReqSp              req = context.createRequest();
                bsl::ostringstream os(bmqtst::TestHelperUtil::allocator());
                os << "bmq://foo.bar_" << i << "_" << j;
                context.populateRequest(req,
                                        os.str(),
                                        i + 1,
                                        i + 2,
                                        i + 3,
                                        i + 4);
                requestsGroup.emplace_back(req);
                requestsWithoutId.emplace(req->request().choice());
            }

            int rc = threadGroup.addThread(
                bdlf::BindUtil::bind(&Caller::requestSenderJob,
                                     requestsGroup,
                                     &barrier,
                                     &context));
            BMQTST_ASSERT_EQ(0, rc);
        }

        barrier.wait();
        threadGroup.joinAll();

        // checking if RequestManager has really sent all the requests to
        // testChannel
        BMQTST_ASSERT_EQ(context.channel().writeCalls().size(),
                         requestsWithoutId.size());
        while (!context.channel().writeCalls().empty()) {
            Mes                    controlMes = context.getNextRequest();
            ReqChoice&             choice     = controlMes.choice();
            ReqChoiceSet::iterator it         = requestsWithoutId.find(choice);
            BMQTST_ASSERT(it != requestsWithoutId.end());
            if (it != requestsWithoutId.end()) {
                BMQTST_ASSERT_EQ(choice, *it);
                requestsWithoutId.erase(it);
            }
        }

        BMQTST_ASSERT(requestsWithoutId.empty());
    }
}

static void test5_processResponseTest()
// ------------------------------------------------------------------------
// Testing:
//   int RequestManager::processResponse()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PROCESS RESPONSE TEST");

    {
        // Correct response
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        context.populateRequest(request);
        context.sendChannelRequest(request);

        Mes response = context.createResponse(
            request->request().rId().value());

        BMQTST_ASSERT_EQ(context.manager().processResponse(response), 0);
        request->wait();

        BMQTST_ASSERT_EQ(request->result(), bmqt::GenericResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(request->response().rId(), request->request().rId());
        BMQTST_ASSERT_EQ(request->response(), response);
    }

    {
        // Response with invalid ID
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        context.populateRequest(request);
        request->request().rId() = 111;
        context.sendChannelRequest(request);

        Mes response = context.createResponse(222);

        BMQTST_ASSERT_NE(context.manager().processResponse(response), 0);
        context.advanceTime(SEND_REQUEST_TIMEOUT);
        request->wait();

        BMQTST_ASSERT_EQ(request->result(), bmqt::GenericResult::e_TIMEOUT);
    }

    {
        // Callback
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        context.populateRequest(request);

        bool called = false;
        request->setResponseCb(bdlf::BindUtil::bind(&Caller::callback,
                                                    &called,
                                                    bdlf::PlaceHolders::_1,
                                                    &request));
        context.sendChannelRequest(request);
        Mes response = context.createResponse(
            request->request().rId().value());
        BMQTST_ASSERT_EQ(context.manager().processResponse(response), 0);
        context.advanceTime(SEND_REQUEST_TIMEOUT);
        BMQTST_ASSERT_EQ(request->result(), bmqt::GenericResult::e_SUCCESS);

        BMQTST_ASSERT(called);
    }

    {
        // Timeout
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        context.populateRequest(request);

        bool called = false;
        request->setResponseCb(bdlf::BindUtil::bind(&Caller::callback,
                                                    &called,
                                                    bdlf::PlaceHolders::_1,
                                                    &request));

        context.sendChannelRequest(request);
        context.advanceTime(SEND_REQUEST_TIMEOUT);

        Mes response = context.createResponse(
            request->request().rId().value());

        BMQTST_ASSERT_NE(context.manager().processResponse(response), 0);
        BMQTST_ASSERT_EQ(request->result(), bmqt::GenericResult::e_TIMEOUT);
        BMQTST_ASSERT(called);
    }

    {
        // Late Mes mode
        TestContext context(true, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        context.populateRequest(request);

        bool called = false;
        request->setResponseCb(bdlf::BindUtil::bind(&Caller::callback,
                                                    &called,
                                                    bdlf::PlaceHolders::_1,
                                                    &request));

        context.sendChannelRequest(request);
        context.advanceTime(SEND_REQUEST_TIMEOUT);

        Mes response = context.createResponse(
            request->request().rId().value());

        BMQTST_ASSERT_EQ(context.manager().processResponse(response), 0);
        BMQTST_ASSERT_EQ(request->result(), bmqt::GenericResult::e_SUCCESS);
        BMQTST_ASSERT(called);
    }

    {
        // Concurrency
        TestContext        context(false, bmqtst::TestHelperUtil::allocator());
        bslmt::Mutex       responsesLock;
        MesQue             responses(bmqtst::TestHelperUtil::allocator());
        bsls::AtomicUint   callsCounter = 0;
        const bsl::size_t  numThreads   = 10;
        const bsl::size_t  numRequests  = 500;
        bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());
        bslmt::Barrier     barrier(numThreads + 1);

        /// Check that the specified `request` is the same request as
        /// the specified `expected` one, and increment the specified
        /// `callsCounter`
        struct ConcurrentCaller {
            static void callback(bsls::AtomicUint* callsCounter,
                                 const ReqSp&      request,
                                 const ReqSp*      expected)
            {
                BMQTST_ASSERT_EQ((*expected)->request(), request->request());
                ++(*callsCounter);
            }

            /// Take one response from the specified `responses` queue and
            /// processes it by the specified `context`.  The specified
            /// `responsesLock` is used to sync access to queue from
            /// different threads.  The specified `barrier` is used to start
            /// execution of all the threads simultaneously.
            static void processResponsesJob(MesQue*         responses,
                                            bslmt::Mutex*   responsesLock,
                                            bslmt::Barrier* barrier,
                                            TestContext*    context)
            {
                barrier->wait();
                while (true) {
                    bsl::optional<Mes> response;
                    {
                        bslmt::LockGuard<bslmt::Mutex> lock(responsesLock);
                        if (!responses->empty()) {
                            response.emplace(responses->front());
                            responses->pop();
                        }
                    }
                    if (response.has_value()) {
                        int rc = context->manager().processResponse(
                            response.value());
                        BMQTST_ASSERT_EQ(rc, 0);
                    }
                    else {
                        break;
                    }
                }
            }
        };

        ReqVec requests;
        requests.reserve(numRequests);

        // Prepare requests to process them in parallel
        for (bsl::size_t i = 0; i < numRequests; ++i) {
            ReqSp& request = requests.emplace_back(context.createRequest());

            bsl::ostringstream os(bmqtst::TestHelperUtil::allocator());
            os << "bmq://foo.bar" << i;
            context.populateRequest(request,
                                    os.str(),
                                    i + 1,
                                    i + 2,
                                    i + 3,
                                    i + 4);

            request->setResponseCb(
                bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                      &ConcurrentCaller::callback,
                                      &callsCounter,
                                      bdlf::PlaceHolders::_1,
                                      &request));
            context.sendChannelRequest(request);
            Mes response = context.createResponse(
                request->request().rId().value());
            responses.emplace(response);
        }

        // Start the threads to process responses from queue
        for (bsl::size_t i = 0; i < numThreads; ++i) {
            int rc = threadGroup.addThread(
                bdlf::BindUtil::bind(&ConcurrentCaller::processResponsesJob,
                                     &responses,
                                     &responsesLock,
                                     &barrier,
                                     &context));
            BMQTST_ASSERT_EQ(0, rc);
        }

        barrier.wait();
        threadGroup.joinAll();

        BMQTST_ASSERT_EQ(callsCounter, numRequests);
    }
}

static void test6_cancelAllRequestsTest()
// ------------------------------------------------------------------------
// Testing:
//   void RequestManager::cancelAllRequests()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CANCEL ALL REQUESTS TEST");

    {
        // Cancel one event
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        context.populateRequest(request);
        context.sendChannelRequest(request);

        Mes reason = context.createResponseCancel();
        context.manager().cancelAllRequests(reason);
        BMQTST_ASSERT_EQ(request->result(), bmqt::GenericResult::e_CANCELED);
        BMQTST_ASSERT_EQ(request->response().choice().status(),
                         reason.choice().status());
    }

    {
        // Cancel all events
        TestContext       context(false, bmqtst::TestHelperUtil::allocator());
        const bsl::size_t numRequests = 10;
        ReqVec            requests(bmqtst::TestHelperUtil::allocator());
        requests.reserve(numRequests);
        for (bsl::size_t i = 0; i < numRequests; ++i) {
            ReqSp              request = context.createRequest();
            bsl::ostringstream os(bmqtst::TestHelperUtil::allocator());
            os << "bmq://foo.bar" << i;
            context.populateRequest(request,
                                    os.str(),
                                    i + 1,
                                    i + 2,
                                    i + 3,
                                    i + 4);
            context.sendChannelRequest(request);
            requests.emplace_back(request);
        }

        Mes reason = context.createResponseCancel();
        context.manager().cancelAllRequests(reason);

        for (bsl::size_t i = 0; i < numRequests; ++i) {
            ReqSp request = requests[i];
            BMQTST_ASSERT_EQ(request->result(),
                             bmqt::GenericResult::e_CANCELED);
            BMQTST_ASSERT_EQ(request->response().choice().status(),
                             reason.choice().status());
        }
    }

    {
        // Cancel group of events
        TestContext       context(false, bmqtst::TestHelperUtil::allocator());
        const bsl::size_t numRequests = 10;
        ReqVec            requests(bmqtst::TestHelperUtil::allocator());
        requests.reserve(numRequests);
        bsl::vector<int> groupIds(2, bmqtst::TestHelperUtil::allocator());
        groupIds[0] = 1;
        groupIds[1] = 2;
        for (bsl::size_t i = 0; i < numRequests; ++i) {
            ReqSp request = context.createRequest();
            context.populateRequest(request);
            request->setGroupId(groupIds[i % groupIds.size()]);
            context.sendChannelRequest(request);
            requests.emplace_back(request);
        }

        Mes reason = context.createResponseCancel();
        context.manager().cancelAllRequests(reason, 1);

        for (bsl::size_t i = 0; i < numRequests; ++i) {
            ReqSp request = requests[i];
            if (request->groupId() == 1) {
                BMQTST_ASSERT_EQ(request->result(),
                                 bmqt::GenericResult::e_CANCELED);
                BMQTST_ASSERT_EQ(request->response().choice().status(),
                                 reason.choice().status());
            }
            else {
                BMQTST_ASSERT_EQ(request->result(),
                                 bmqt::GenericResult::e_SUCCESS);
            }
        }
    }
}

static void test7_requestBreathingTest()
// ------------------------------------------------------------------------
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("REQUEST BREATHING TEST");

    {
        // Create request and check its initial state
        ReqSp request;
        BMQTST_ASSERT_PASS(
            request.createInplace(bmqtst::TestHelperUtil::allocator(),
                                  bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT(request);
        BMQTST_ASSERT(!request->isLateResponse());
        BMQTST_ASSERT(!request->isLocalTimeout());
        BMQTST_ASSERT(!request->isError());
        BMQTST_ASSERT(typeid(request->request()) == typeid(Mes));
        Mes& req = request->request();
        BMQTST_ASSERT(!req.rId().has_value());
        BMQTST_ASSERT(typeid(request->response()) == typeid(Mes));
        Mes& res = request->response();
        BMQTST_ASSERT(!res.rId().has_value());
        BMQTST_ASSERT(!static_cast<bool>(request->responseCb()));
        BMQTST_ASSERT_EQ(request->result(), bmqt::GenericResult::e_SUCCESS);
        BMQTST_ASSERT(request->nodeDescription().empty());
        BMQTST_ASSERT_EQ(request->groupId(), -1);  // Req::k_NO_GROUP_ID
        BMQTST_ASSERT(request->userData().isNull());
    }

    {
        // set responseCb and call it
        ReqSp request;
        request.createInplace(bmqtst::TestHelperUtil::allocator(),
                              bmqtst::TestHelperUtil::allocator());
        bool called = false;
        request->setResponseCb(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &Caller::callback,
                                  &called,
                                  bdlf::PlaceHolders::_1,
                                  &request));
        request->responseCb()(request);

        BMQTST_ASSERT(called);
    }

    {
        // set asyncNotifierCb and check if it is called
        TestContext context(false, bmqtst::TestHelperUtil::allocator());
        ReqSp       request = context.createRequest();
        bool        called  = false;
        request->setAsyncNotifierCb(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &Caller::callback,
                                  &called,
                                  bdlf::PlaceHolders::_1,
                                  &request));
        request->signal();
        BMQTST_ASSERT(called);
    }

    {
        // set groupId and check it
        ReqSp request;
        request.createInplace(bmqtst::TestHelperUtil::allocator(),
                              bmqtst::TestHelperUtil::allocator());
        int expected = 666;
        request->setGroupId(expected);
        BMQTST_ASSERT_EQ(request->groupId(), expected);
    }

    {
        // set user data and check it
        bsl::string test("test string", bmqtst::TestHelperUtil::allocator());
        bdld::Datum datum = bdld::Datum::createStringRef(
            test.c_str(),
            bmqtst::TestHelperUtil::allocator());
        ReqSp request;
        request.createInplace(bmqtst::TestHelperUtil::allocator(),
                              bmqtst::TestHelperUtil::allocator());
        request->adoptUserData(datum);
        const bdld::Datum& new_datum = request->userData();
        BMQTST_ASSERT(new_datum.isString());
        BMQTST_ASSERT_EQ(test, new_datum.theString());
    }
}

static void test8_requestSignalWaitTest()
// ------------------------------------------------------------------------
// Testing:
//   void RequestManagerRequest::signal()
//   void RequestManagerRequest::wait()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("REQUEST SIGNAL WAIT TEST");

    /// Call wait function of the specified `request`, and then set the
    /// specified `worked` flag to true.
    struct RequestKeeper {
        static void waiter(bool* worked, const ReqSp* request)
        {
            (*request)->wait();
            *worked = true;
        }
    };

    {
        // Wait for signal in separate thread
        bool  worked = false;
        ReqSp request;
        BMQTST_ASSERT_PASS(
            request.createInplace(bmqtst::TestHelperUtil::allocator(),
                                  bmqtst::TestHelperUtil::allocator()));

        bslmt::ThreadUtil::Handle handle;

        int rc = bslmt::ThreadUtil::createWithAllocator(
            &handle,
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &RequestKeeper::waiter,
                                  &worked,
                                  &request),
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(rc, 0);
        bslmt::ThreadUtil::yield();
        BMQTST_ASSERT(!worked);
        request->signal();
        // Here we don't know if 'wait()' is already executing in the separate
        // thread or not.  But we can guarantee that it is not finished yet.
        rc = bslmt::ThreadUtil::join(handle);
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT(worked);
    }
}

// ============================================================================
//                                MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize(bmqtst::TestHelperUtil::allocator());
    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 8: test8_requestSignalWaitTest(); break;
    case 7: test7_requestBreathingTest(); break;
    case 6: test6_cancelAllRequestsTest(); break;
    case 5: test5_processResponseTest(); break;
    case 4: test4_sendRequestTest(); break;
    case 3: test3_createRequestTest(); break;
    case 2: test2_setExecutorTest(); break;
    case 1: test1_creatorsTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqsys::Time::shutdown();
    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);  // RETURN
    // Default: EventQueue uses bmqex::BindUtil::bindExecute(), which uses
    //          default allocator.
}
