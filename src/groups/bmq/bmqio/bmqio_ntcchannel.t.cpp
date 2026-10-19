// Copyright 2024 Bloomberg Finance L.P.
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

#include <bmqio_ntcchannel.h>

// BMQ
#include <bmqu_blob.h>

// NTC
#include <ntca_interfaceconfig.h>
#include <ntcf_system.h>
#include <ntsf_system.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsla_annotations.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadutil.h>
#include <bsls_types.h>

#include <bmqtst_testhelper.h>
#include <bsl_vector.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace bmqio;

namespace {

const size_t k_WRITE_QUEUE_LOW_WM  = 512 * 1024;
const size_t k_WRITE_QUEUE_HIGH_WM = 512 * 1024;

/// Create the ntca::InterfaceConfig to use given the specified
/// `sessionOptions`.  Use the specified `allocator` for any memory
/// allocation.
ntca::InterfaceConfig ntcCreateInterfaceConfig(bslma::Allocator* allocator)
{
    ntca::InterfaceConfig config(allocator);

    config.setThreadName("test");

    config.setMaxThreads(4);
    config.setMaxConnections(128);
    config.setWriteQueueLowWatermark(k_WRITE_QUEUE_LOW_WM);
    config.setWriteQueueHighWatermark(k_WRITE_QUEUE_HIGH_WM);

    config.setDriverMetrics(false);
    config.setDriverMetricsPerWaiter(false);
    config.setSocketMetrics(false);
    config.setSocketMetricsPerHandle(false);

    config.setAcceptGreedily(false);
    config.setSendGreedily(false);
    config.setReceiveGreedily(false);

    config.setNoDelay(true);
    config.setKeepAlive(true);
    config.setKeepHalfOpen(false);

    return config;
}

void doFail()
{
    BMQTST_ASSERT(false && "Must not be invoked");
}

void executeOnClosedChannelFunc(bmqio::NtcChannel*              channel,
                                BSLA_MAYBE_UNUSED const Status& status)
{
    // PRECONDITIONS
    BMQTST_ASSERT(channel);

    BSLA_MAYBE_UNUSED bslma::Allocator* alloc     = channel->allocator();
    BSLA_MAYBE_UNUSED int               id        = channel->channelId();
    BSLA_MAYBE_UNUSED ntsa::Endpoint peerEndpoint = channel->peerEndpoint();
    BSLA_MAYBE_UNUSED ntsa::Endpoint sourceEndpoint =
        channel->sourceEndpoint();
    BSLA_MAYBE_UNUSED bsl::string peerUri            = channel->peerUri();
    BSLA_MAYBE_UNUSED bmqvt::PropertyBag& properties = channel->properties();

    channel->setChannelId(id);
    channel->setWriteQueueLowWatermark(k_WRITE_QUEUE_LOW_WM);
    channel->setWriteQueueHighWatermark(k_WRITE_QUEUE_HIGH_WM);

    // These operations on a closed channel should be no-op
    channel->cancel();
    channel->cancelRead();
    channel->execute(bdlf::BindUtil::bind(doFail));

    // The second `close` should be no-op
    channel->close();
}

/// @brief Save the specified `status` to the specified `result` and post on
///        the specified `semaphore`.
///
/// @param semaphore The semaphore to post on.
/// @param result    The output status.
/// @param channel   The channel kept alive while this callback is stored.
/// @param status    The close status.
void saveCloseStatus(
    bslmt::Semaphore*       semaphore,
    Status*                 result,
    BSLA_MAYBE_UNUSED const bsl::shared_ptr<NtcChannel>& channel,
    const Status&                                        status)
{
    // PRECONDITIONS
    BMQTST_ASSERT(semaphore);
    BMQTST_ASSERT(result);

    *result = status;
    semaphore->post();
}

// ============
// class Tester
// ============

/// Helper class testing a NtcChannel, wrapping its creation and providing
/// convenient wrappers for calling its functions and checking its results.
///
/// Many of the functions take a `int line` argument, which is always
/// passed `L_` and is used in assertion messages to find where an error
/// occurred.
class Tester {
    // DATA
    bslma::Allocator*                               d_allocator_p;
    bsl::shared_ptr<bdlbb::PooledBlobBufferFactory> d_blobBufferFactory_sp;
    bsl::shared_ptr<ntci::Interface>                d_interface_sp;
    bsl::shared_ptr<ntci::ListenerSocket>           d_listener_sp;
    bsl::vector<bsl::shared_ptr<bmqio::Channel> >   d_listenChannels;
    bsl::vector<bsl::shared_ptr<bmqio::Channel> >   d_connectChannels;
    bslmt::Mutex                                    d_mutex;

    // NOT IMPLEMENTED
    Tester(const Tester&);
    Tester& operator=(const Tester&);

    // PRIVATE MANIPULATORS
    void destroy();

    void
    onAcceptConnection(bslmt::Semaphore*                          semaphore_p,
                       const bsl::shared_ptr<ntci::Acceptor>&     acceptor,
                       const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
                       const ntca::AcceptEvent&                   event);

    void onChannelResult(bslmt::Semaphore*               semaphore_p,
                         ChannelFactoryEvent::Enum       event,
                         const Status&                   status,
                         const bsl::shared_ptr<Channel>& channel);

  public:
    // TRAITS
    BSLA_MAYBE_UNUSED
    BSLMF_NESTED_TRAIT_DECLARATION(Tester, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit Tester(bslma::Allocator* basicAllocator = 0);
    ~Tester();

    // MANIPULATORS

    /// (Re-)create the object being tested and reset the state of any
    /// supporting objects.
    void init();

    bsl::shared_ptr<bmqio::NtcChannel> connect();

    /// @brief Release all references held to the connecting channels.
    void releaseConnectChannels();
};

// ------------
// class Tester
// ------------

// CREATORS
Tester::Tester(bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_blobBufferFactory_sp()
, d_interface_sp()
, d_listener_sp()
, d_listenChannels(d_allocator_p)
, d_connectChannels(d_allocator_p)
, d_mutex()
{
}

Tester::~Tester()
{
    destroy();
}

// MANIPULATORS
void Tester::destroy()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    for (size_t i = 0; i < d_listenChannels.size(); i++) {
        d_listenChannels[i]->close();
    }
    for (size_t i = 0; i < d_connectChannels.size(); i++) {
        d_connectChannels[i]->close();
    }
    d_listenChannels.clear();
    d_connectChannels.clear();
    if (d_listener_sp) {
        d_listener_sp->close();
        d_listener_sp->shutdown();
        d_listener_sp.reset();
    }
    if (d_interface_sp) {
        d_interface_sp->shutdown();
        d_interface_sp->linger();
        d_interface_sp.reset();
    }
}

void Tester::onAcceptConnection(
    bslmt::Semaphore*       semaphore_p,
    BSLA_MAYBE_UNUSED const bsl::shared_ptr<ntci::Acceptor>& acceptor,
    const bsl::shared_ptr<ntci::StreamSocket>&               streamSocket,
    const ntca::AcceptEvent&                                 event)
{
    // PRECONDITIONS
    BMQTST_ASSERT(semaphore_p);
    BMQTST_ASSERT(event.isComplete());

    bsl::shared_ptr<bmqio::NtcChannel> channel;
    channel.createInplace(d_allocator_p,
                          d_interface_sp,
                          bmqio::ChannelFactory::ResultCallback(),
                          d_allocator_p);

    channel->import(streamSocket);

    d_listenChannels.push_back(channel);
    semaphore_p->post();
}

void Tester::onChannelResult(bslmt::Semaphore* semaphore_p,
                             BSLA_MAYBE_UNUSED ChannelFactoryEvent::Enum event,
                             BSLA_MAYBE_UNUSED const Status& status,
                             const bsl::shared_ptr<Channel>& channel)
{
    // PRECONDITIONS
    BMQTST_ASSERT(semaphore_p);

    d_connectChannels.push_back(channel);
    semaphore_p->post();
}

void Tester::init()
{
    // 0. Cleanup
    destroy();

    // 1. Create blob buffer factory
    d_blobBufferFactory_sp.createInplace(d_allocator_p, 4096, d_allocator_p);

    // 2. Start ntc intefrace
    ntca::InterfaceConfig config = ntcCreateInterfaceConfig(d_allocator_p);
    config.setThreadName("test");

    // Solaris: disambiguate by using the expected interface type
    bsl::shared_ptr<bdlbb::BlobBufferFactory> bufferFactory_sp =
        bsl::static_pointer_cast<bdlbb::BlobBufferFactory>(
            d_blobBufferFactory_sp);

    d_interface_sp    = ntcf::System::createInterface(config,
                                                   bufferFactory_sp,
                                                   d_allocator_p);
    ntsa::Error error = d_interface_sp->start();
    BMQTST_ASSERT_EQ(error, ntsa::Error::e_OK);

    // 3. Start listener
    const int backlog = 10;

    ntca::ListenerSocketOptions listenerSocketOptions;
    listenerSocketOptions.setTransport(ntsa::Transport::e_TCP_IPV4_STREAM);
    listenerSocketOptions.setReuseAddress(true);
    listenerSocketOptions.setKeepHalfOpen(false);
    listenerSocketOptions.setBacklog(backlog);
    listenerSocketOptions.setSourceEndpoint(
        ntsa::Endpoint(ntsa::Ipv4Address::loopback(), 0));

    d_listener_sp = d_interface_sp->createListenerSocket(listenerSocketOptions,
                                                         d_allocator_p);
    BMQTST_ASSERT(d_listener_sp);

    error = d_listener_sp->open();
    BMQTST_ASSERT_EQ(error, ntsa::Error::e_OK);

    error = d_listener_sp->listen(backlog);
    BMQTST_ASSERT_EQ(error, ntsa::Error::e_OK);

    const ntsa::Endpoint endpoint = d_listener_sp->sourceEndpoint();
    BMQTST_ASSERT(endpoint.isIp());
    BMQTST_ASSERT(endpoint.ip().host().isV4());
}

bsl::shared_ptr<bmqio::NtcChannel> Tester::connect()
{
    bsl::shared_ptr<bmqio::NtcChannel> channel;

    bslmt::Semaphore connectSemaphore, acceptSemaphore;

    bmqio::ChannelFactory::ResultCallback connectResultCb =
        bdlf::BindUtil::bindS(d_allocator_p,
                              &Tester::onChannelResult,
                              this,
                              &connectSemaphore,
                              bdlf::PlaceHolders::_1,
                              bdlf::PlaceHolders::_2,
                              bdlf::PlaceHolders::_3);

    channel.createInplace(d_allocator_p,
                          d_interface_sp,
                          connectResultCb,
                          d_allocator_p);

    bmqio::Status         status(d_allocator_p);
    bmqio::ConnectOptions options(d_allocator_p);
    options.setEndpoint(d_listener_sp->sourceEndpoint().text());
    const int rc = channel->connect(&status, options);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(status.category(), bmqio::StatusCategory::e_SUCCESS);

    ntci::AcceptCallback acceptCallback = d_listener_sp->createAcceptCallback(
        bdlf::BindUtil::bindS(d_allocator_p,
                              &Tester::onAcceptConnection,
                              this,
                              &acceptSemaphore,
                              bdlf::PlaceHolders::_1,
                              bdlf::PlaceHolders::_2,
                              bdlf::PlaceHolders::_3),
        d_allocator_p);

    ntsa::Error error = d_listener_sp->accept(ntca::AcceptOptions(),
                                              acceptCallback);
    BMQTST_ASSERT_EQ(error, ntsa::Error::e_OK);

    acceptSemaphore.wait();
    connectSemaphore.wait();

    return channel;
}

void Tester::releaseConnectChannels()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    d_connectChannels.clear();
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   a) Listening for a connection and connecting to the same port
//      establishes both connections
//   b) Closing a Channel closes both ends
//   c) Operations executed over closed channel are handled gracefully
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Breathing Test");

    Tester tester(bmqtst::TestHelperUtil::allocator());
    tester.init();

    bsl::shared_ptr<bmqio::NtcChannel> channel = tester.connect();
    bdlbb::PooledBlobBufferFactory     blobFactory(
        4096,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob blob(&blobFactory, bmqtst::TestHelperUtil::allocator());

    bsl::string           message("test", bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<char> messagePtr;
    messagePtr.reset(message.data(),
                     bslstl::SharedPtrNilDeleter(),
                     bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer buffer(messagePtr, message.size());

    blob.appendDataBuffer(buffer);
    bmqio::Status status(bmqtst::TestHelperUtil::allocator());
    channel->write(&status, blob);
    BMQTST_ASSERT_EQ(status.category(), bmqio::StatusCategory::e_SUCCESS);

    bmqio::Channel::CloseFn closeCb = bdlf::BindUtil::bindS(
        bmqtst::TestHelperUtil::allocator(),
        executeOnClosedChannelFunc,
        channel.get(),
        bdlf::PlaceHolders::_1);
    channel->onClose(closeCb);

    channel->close();
}

static void test2_onCloseAfterClose()
// ------------------------------------------------------------------------
// ON CLOSE AFTER CLOSE
//
// Concerns:
//   a) A close callback registered on an already closed channel is
//      invoked immediately in the calling thread
//   b) This callback receives the same status as the callbacks invoked
//      on close
//   c) Close callbacks holding a reference to the channel do not keep it
//      alive: the channel is destroyed once all other references are
//      released
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("On Close After Close");

    Tester tester(bmqtst::TestHelperUtil::allocator());
    tester.init();

    bsl::shared_ptr<bmqio::NtcChannel> channel = tester.connect();
    bsl::weak_ptr<bmqio::NtcChannel>   weakChannel(channel);

    bslmt::Semaphore semaphore;
    Status           status1(bmqtst::TestHelperUtil::allocator());
    Status           status2(bmqtst::TestHelperUtil::allocator());

    channel->onClose(bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                           saveCloseStatus,
                                           &semaphore,
                                           &status1,
                                           channel,
                                           bdlf::PlaceHolders::_1));

    channel->close(Status(StatusCategory::e_CANCELED,
                          bmqtst::TestHelperUtil::allocator()));
    semaphore.wait();
    BMQTST_ASSERT_EQ(status1.category(), StatusCategory::e_CANCELED);

    channel->onClose(bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                           saveCloseStatus,
                                           &semaphore,
                                           &status2,
                                           channel,
                                           bdlf::PlaceHolders::_1));

    BMQTST_ASSERT_EQ(semaphore.tryWait(), 0);
    BMQTST_ASSERT_EQ(status2.category(), StatusCategory::e_CANCELED);

    // Wait for the channel to be destroyed, as an owner waiting for all its
    // channels on shutdown would do.  The IO thread may still briefly hold a
    // reference while finishing the close.
    channel.reset();
    tester.releaseConnectChannels();

    const int k_MAX_WAIT_ITERATIONS = 1000;
    for (int i = 0; i < k_MAX_WAIT_ITERATIONS && !weakChannel.expired(); ++i) {
        bslmt::ThreadUtil::microSleep(10 * 1000);  // 10ms
    }
    BMQTST_ASSERT(weakChannel.expired());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: {
        test2_onCloseAfterClose();
    } break;
    case 1: {
        test1_breathingTest();
    } break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(0);
}
