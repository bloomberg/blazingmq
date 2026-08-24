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

#include <bsla_maybeunused.h>
#include <mqba_adminsession.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbi_authorizer.h>
#include <mqbi_dispatcher.h>
#include <mqbmock_dispatcher.h>
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_protocol.h>

#include <bmqio_channel.h>
#include <bmqio_testchannel.h>
#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_time.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlf_bind.h>
#include <bdlmt_threadpool.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadutil.h>
#include <bsls_atomic.h>
#include <bsls_timeinterval.h>

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

bmqp_ctrlmsg::NegotiationMessage client()
// Create a 'NegotiationMessage' that represents a client configuration for
// the specified 'clientType'.
{
    bmqp_ctrlmsg::NegotiationMessage negotiationMessage;
    bmqp_ctrlmsg::ClientIdentity&    clientIdentity =
        negotiationMessage.makeClientIdentity();
    clientIdentity.clientType() = bmqp_ctrlmsg::ClientType::E_TCPADMIN;
    clientIdentity.guidInfo().clientId()             = "0A0B0C0D0E0F";
    clientIdentity.guidInfo().nanoSecondsFromEpoch() = 1261440000;

    return negotiationMessage;
}

/// Create a new blob at the specified `arena` address, using the specified
/// `bufferFactory` and `allocator`.
void createBlob(bdlbb::BlobBufferFactory* bufferFactory,
                void*                     arena,
                bslma::Allocator*         allocator)
{
    new (arena) bdlbb::Blob(bufferFactory, allocator);
}

/// Struct to return back incoming admin commands
struct TestAdminRetranslator {
    TestAdminRetranslator() {}

    int enqueueCommand(
        BSLA_MAYBE_UNUSED const bsl::string&            source,
        const bsl::string&                              cmd,
        const mqbnet::Session::AdminCommandProcessedCb& onProcessedCb)
    {
        int rc = 0;
        onProcessedCb(rc, cmd);
        return rc;
    }
};

/// Authorizer for testing that allows all
class TestAuthorizer : public mqbi::Authorizer {
  public:
    TestAuthorizer() {}

    bool
    authorize(BSLA_UNUSED const mqbact::Action& action,
              BSLA_UNUSED const mqbplug::AuthenticationResult& authnResult)
        BSLS_KEYWORD_OVERRIDE
    {
        return true;
    }
};

/// The `TestBench` holds system components together.
class TestBench {
  public:
    // DATA
    bdlbb::PooledBlobBufferFactory      d_bufferFactory;
    BlobSpPool                          d_blobSpPool;
    bsl::shared_ptr<bmqio::TestChannel> d_channel_sp;
    mqbmock::Dispatcher                 d_mockDispatcher;
    bsl::shared_ptr<TestAuthorizer>     d_authorizer_sp;
    mqba::AdminSession                  d_as;
    bslma::Allocator*                   d_allocator_p;

    // CREATORS

    /// Constructor. Creates a `TestBench` using the specified
    /// `negotiationMessage`, `atMostOnce` and `allocator`.
    TestBench(const bmqp_ctrlmsg::NegotiationMessage&       negotiationMessage,
              const mqbnet::Session::AdminCommandEnqueueCb& adminEnqueueCb,
              bslma::Allocator*                             allocator)
    : d_bufferFactory(256, allocator)
    , d_blobSpPool(bdlf::BindUtil::bind(&createBlob,
                                        &d_bufferFactory,
                                        bdlf::PlaceHolders::_1,   // arena
                                        bdlf::PlaceHolders::_2),  // alloc
                   1024,  // blob pool growth strategy
                   allocator)
    , d_channel_sp(new bmqio::TestChannel(allocator))
    , d_mockDispatcher(allocator)
    , d_authorizer_sp(bsl::allocate_shared<TestAuthorizer>(allocator))
    , d_as(d_channel_sp,
           negotiationMessage,
           "sessionDescription",
           &d_mockDispatcher,
           &d_blobSpPool,
           adminEnqueueCb,
           d_authorizer_sp,
           allocator)
    , d_allocator_p(allocator)
    {
        // Typically done during 'Dispatcher::registerClient()'.
        d_as.dispatcherClientData().setDispatcher(&d_mockDispatcher);
        d_as.setThreadId(bslmt::ThreadUtil::selfId());
    }

    /// Destructor
    ~TestBench() { d_as.tearDown(bsl::shared_ptr<void>(), true); }
};

// ============================================================================
//              HELPERS FOR THE CONCURRENT TEARDOWN TEST
// ----------------------------------------------------------------------------

/// Drives a `mqbmock::Dispatcher` (configured in `enqueueOnly` mode) from a
/// dedicated thread, emulating the broker's single-threaded dispatcher
/// processor.
/// This ensures that dispatcher callbacks (such as `finalizeAdminCommand`,
/// enqueued by `onProcessedAdminCommand` from the admin execution pool) run on
/// a thread distinct from the one that tears down and destroys the
/// `AdminSession`, as they do in the broker.
class QueueProcessor {
  private:
    // DATA
    mqbmock::Dispatcher*      d_dispatcher_p;
    bsls::AtomicBool          d_running;
    bslmt::ThreadUtil::Handle d_handle;

    // NOT IMPLEMENTED
    QueueProcessor(const QueueProcessor&);
    QueueProcessor& operator=(const QueueProcessor&);

  public:
    // CREATORS

    /// Create a processor draining the specified `dispatcher`.
    explicit QueueProcessor(mqbmock::Dispatcher* dispatcher)
    : d_dispatcher_p(dispatcher)
    , d_running(true)
    , d_handle(bslmt::ThreadUtil::invalidHandle())
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(d_dispatcher_p);
    }

    // MANIPULATORS

    /// Spawn the processor thread.
    void start()
    {
        int rc = bslmt::ThreadUtil::create(
            &d_handle,
            bdlf::BindUtil::bind(&QueueProcessor::run, this));
        BMQTST_ASSERT_EQ(rc, 0);
    }

    /// Body of the processor thread: repeatedly drain the dispatcher queue
    /// until stopped, then perform one final drain.
    void run()
    {
        while (d_running.load()) {
            d_dispatcher_p->processQueue();
            bslmt::ThreadUtil::yield();
        }
        d_dispatcher_p->processQueue();
    }

    /// Signal the processor thread to stop and join it.
    void stop()
    {
        d_running.store(false);
        if (d_handle != bslmt::ThreadUtil::invalidHandle()) {
            bslmt::ThreadUtil::join(d_handle);
            d_handle = bslmt::ThreadUtil::invalidHandle();
        }
    }
};

/// Admin command enqueue callback that completes commands asynchronously on a
/// separate thread pool.
class AsyncAdminExecutor {
  private:
    // DATA
    bdlmt::ThreadPool* d_pool_p;

  public:
    // CREATORS
    explicit AsyncAdminExecutor(bdlmt::ThreadPool* pool)
    : d_pool_p(pool)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(d_pool_p);
    }

    // MANIPULATORS

    /// Post the completion of the specified `cmd` (with the specified
    /// `onProcessedCb`) to the thread pool.
    void enqueueCommand(
        BSLA_MAYBE_UNUSED const bsl::string&            source,
        const bsl::string&                              cmd,
        const mqbnet::Session::AdminCommandProcessedCb& onProcessedCb)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(onProcessedCb);

        d_pool_p->enqueueJob(bdlf::BindUtil::bind(&AsyncAdminExecutor::invoke,
                                                  onProcessedCb,
                                                  cmd));
    }

  private:
    /// Invoke the specified `onProcessedCb` with the specified `cmd`.
    static void
    invoke(const mqbnet::Session::AdminCommandProcessedCb& onProcessedCb,
           const bsl::string&                              cmd)
    {
        onProcessedCb(0, cmd);
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_watermark()
// ------------------------------------------------------------------------
// TESTS ADMIN SESSION CONTINUES WORKING ON HIGH WATERMARK
//
// Concerns:
//   - Callback loop works for admin session commands/responses.
//   - High watermark status is not causing crash in admin session.
//   - Admin command response corresponds with the initial admin command.
//
// Plan:
//   Instantiate a testbench and admin command retranslator, set the high
//   watermark status for the test channel, send multiple admin commands
//   and check that all admin responses are written to the channel.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ADMIN SESSION HIGH WATERMARK");

    const bsl::string command("sample command",
                              bmqtst::TestHelperUtil::allocator());
    const size_t      numMessages = 64;
    const int         rId         = 678098;

    // Prepare test bench and admin commands retranslator
    TestAdminRetranslator retranslator;
    TestBench             tb(
        client(),
        bdlf::BindUtil::bind(&TestAdminRetranslator::enqueueCommand,
                             &retranslator,
                             bdlf::PlaceHolders::_1,  // source
                             bdlf::PlaceHolders::_2,  // cmd
                             bdlf::PlaceHolders::_3),  // onProcessedCb
        bmqtst::TestHelperUtil::allocator());

    // Prepare sample admin command control message event
    bdlma::LocalSequentialAllocator<2048> localAllocator(
        bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::ControlMessage admin(&localAllocator);

    admin.rId() = rId;
    admin.choice().makeAdminCommand();
    admin.choice().adminCommand().command() = command;

    bmqp::SchemaEventBuilder builder(&tb.d_blobSpPool,
                                     bmqp::EncodingType::e_JSON,
                                     tb.d_allocator_p);

    int rc = builder.setMessage(admin, bmqp::EventType::e_CONTROL);
    BMQTST_ASSERT_EQ(rc, 0);

    bmqp::Event adminEvent(builder.blob().get(),
                           bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT(adminEvent.isValid());
    BSLS_ASSERT(adminEvent.isControlEvent());

    // Set high watermark status for the test channel
    bmqio::Status status;
    status.setCategory(bmqio::StatusCategory::e_LIMIT);
    tb.d_channel_sp->setWriteStatus(status);

    // Send the sample admin event multiple times to the admin session
    for (size_t i = 0; i < numMessages; i++) {
        tb.d_as.processEvent(adminEvent);
        BSLS_ASSERT(tb.d_channel_sp->waitFor(i + 1));
    }

    // Check that we have the needed number of write calls after all admin
    // commands were sent.
    BMQTST_ASSERT(tb.d_channel_sp->waitFor(numMessages));

    bmqio::TestChannel::WriteCall writeCall;
    BMQTST_ASSERT(tb.d_channel_sp->getWriteCall(&writeCall, 0));

    // Sanity check for the first admin response
    bmqp::Event adminResponseEvent(&writeCall.d_blob,
                                   bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT(adminResponseEvent.isValid());
    BSLS_ASSERT(adminResponseEvent.isControlEvent());

    bmqp_ctrlmsg::ControlMessage response(&localAllocator);
    rc = adminResponseEvent.loadControlEvent(&response);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(response.rId(), rId);
    BSLS_ASSERT(response.choice().isAdminCommandResponseValue());
    BMQTST_ASSERT_EQ(response.choice().adminCommandResponse().text(), command);
}

static void test2_safeConcurrentTeardown()
// ------------------------------------------------------------------------
// SAFE CONCURRENT TEARDOWN
//
// Concerns:
//   - An admin command submitted to an `AdminSession` is executed
//     asynchronously (on the broker's admin execution pool), and its
//     completion callback is delivered on a separate thread.  Tearing down
//     and destroying the session concurrently with such an in-flight
//     completion must be safe: the session must remain valid until every
//     callback it has dispatched to the client dispatcher has been processed,
//     so that no callback ever runs on a destroyed session.
//
// Plan:
//   Faithfully emulate the broker's threading model:
//     - a `mqbmock::Dispatcher` in `enqueueOnly` mode drained by a dedicated
//       `QueueProcessor` thread (so dispatched callbacks run on a thread
//       distinct from the destroying thread, as in the broker);
//     - an `AsyncAdminExecutor` that completes admin commands on a thread pool
//       (as `mqba::Application`'s admin execution pool does).
//   For a number of iterations: create an `AdminSession`, submit an admin
//   command (whose completion is delivered asynchronously), then tear down and
//   release the session while the completion may still be in flight.  The
//   session must be destroyed safely in every iteration.  Running under
//   ThreadSanitizer or AddressSanitizer additionally verifies the absence of
//   data races and invalid memory accesses.
//
// Testing:
//   AdminSession::tearDown
//   AdminSession::onProcessedAdminCommand
//   AdminSession::finalizeAdminCommand
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SAFE CONCURRENT TEARDOWN");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    // Number of teardown iterations to exercise.
    const int k_NUM_ITERATIONS = 1000;

    // Shared blob infrastructure.
    bdlbb::PooledBlobBufferFactory bufferFactory(256, alloc);
    BlobSpPool                     blobSpPool(bdlf::BindUtil::bind(&createBlob,
                                               &bufferFactory,
                                               bdlf::PlaceHolders::_1,
                                               bdlf::PlaceHolders::_2),
                          1024,
                          alloc);

    // Dispatcher driven by a dedicated processor thread.
    mqbmock::Dispatcher dispatcher(alloc);
    dispatcher.setEnqueueOnly(true);
    QueueProcessor processor(&dispatcher);
    processor.start();

    // Thread pool emulating the asynchronous admin execution pool.
    bdlmt::ThreadPool pool(bslmt::ThreadAttributes(),
                           1,  // minThreads
                           2,  // maxThreads
                           bsls::TimeInterval(120).totalMilliseconds(),
                           alloc);
    int               rc = pool.start();
    BMQTST_ASSERT_EQ(rc, 0);

    AsyncAdminExecutor                     executor(&pool);
    mqbnet::Session::AdminCommandEnqueueCb adminCb = bdlf::BindUtil::bind(
        &AsyncAdminExecutor::enqueueCommand,
        &executor,
        bdlf::PlaceHolders::_1,   // source
        bdlf::PlaceHolders::_2,   // cmd
        bdlf::PlaceHolders::_3);  // onProcessedCb

    // Build a single admin command event, reused across iterations.  The
    // builder is kept alive for the whole loop so that `adminEvent`'s
    // underlying blob remains valid.
    bmqp_ctrlmsg::ControlMessage admin(alloc);
    admin.rId() = 1;
    admin.choice().makeAdminCommand();
    admin.choice().adminCommand().command() = "sample command";

    bmqp::SchemaEventBuilder builder(&blobSpPool,
                                     bmqp::EncodingType::e_BER,
                                     alloc);
    rc = builder.setMessage(admin, bmqp::EventType::e_CONTROL);
    BMQTST_ASSERT_EQ(rc, 0);

    bmqp::Event adminEvent(builder.blob().get(), alloc);
    const bmqp_ctrlmsg::NegotiationMessage negotiationMessage = client();
    const bsl::string                      description("adminSession", alloc);
    bsl::shared_ptr<TestAuthorizer>        authorizer =
        bsl::allocate_shared<TestAuthorizer>(alloc);

    for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
        bsl::shared_ptr<bmqio::TestChannel> channel;
        channel.createInplace(alloc, alloc);

        bsl::shared_ptr<mqba::AdminSession> session =
            bsl::allocate_shared<mqba::AdminSession>(alloc,
                                                     channel,
                                                     negotiationMessage,
                                                     description,
                                                     &dispatcher,
                                                     &blobSpPool,
                                                     adminCb,
                                                     authorizer);

        // All dispatched callbacks may run on the processor thread; allow
        // `inDispatcherThread()` preconditions to pass regardless of thread.
        session->setThreadId(mqbi::DispatcherClient::k_ANY_THREAD_ID);
        session->dispatcherClientData().setDispatcher(&dispatcher);

        // Feed an admin command.  This enqueues `enqueueAdminCommand`, which
        // the processor thread runs, invoking `adminCb`, which posts the
        // completion to the pool.  The completion will call
        // `onProcessedAdminCommand`, potentially concurrently with the
        // teardown below.
        session->processEvent(adminEvent);

        // Tear the session down and release it while the asynchronous
        // completion may still be in flight (or a callback it dispatched may
        // still be pending on the processor).  This must be safe.
        bsl::shared_ptr<void> handle = session;
        session->tearDown(handle, false);
        handle.reset();
        session.reset();  // destroy the session
    }

    // Ensure no more completions are posted, then drain everything still in
    // flight before tearing down the shared infrastructure.
    pool.drain();
    pool.stop();
    processor.stop();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    {
        bmqu::Time::initialize(bmqtst::TestHelperUtil::allocator());

        mqbcfg::AppConfig brokerConfig(bmqtst::TestHelperUtil::allocator());
        mqbcfg::BrokerConfig::set(brokerConfig);

        switch (_testCase) {
        case 0:
        case 1: test1_watermark(); break;
        case 2: test2_safeConcurrentTeardown(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            bmqtst::TestHelperUtil::testStatus() = -1;
        } break;
        }

        bmqu::Time::shutdown();
    }

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
    // Do not check for default/global allocator usage.
}
