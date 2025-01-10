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

// bmqio_ntcchannel.t.cpp                                             -*-C++-*-
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
#include <bsls_annotation.h>
#include <bsls_types.h>

#include <bmqtst_testhelper.h>

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

void executeOnClosedChannelFunc(bmqio::NtcChannel* channel,
                                const Status&      status)
{
    // PRECONDITIONS
    BMQTST_ASSERT(channel);

    BSLA_MAYBE_UNUSED bslma::Allocator* alloc     = channel->allocator();
    BSLA_MAYBE_UNUSED int               id        = channel->channelId();
    BSLA_MAYBE_UNUSED ntsa::Endpoint peerEndpoint = channel->peerEndpoint();
    BSLA_MAYBE_UNUSED ntsa::Endpoint sourceEndpoint =
        channel->sourceEndpoint();
    BSLA_MAYBE_UNUSED const bsl::string& peerUri     = channel->peerUri();
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
    bmqio::ChannelFactory::ResultCallback           d_listenResultCallback;
    bmqio::ChannelFactory::ResultCallback           d_connectResultCallback;
    bsl::vector<bsl::shared_ptr<bmqio::Channel> >   d_listenChannels;
    bsl::vector<bsl::shared_ptr<bmqio::Channel> >   d_connectChannels;
    bslmt::Semaphore                                d_semaphore;
    bslmt::Mutex                                    d_mutex;

    // NOT IMPLEMENTED
    Tester(const Tester&);
    Tester& operator=(const Tester&);

    // PRIVATE MANIPULATORS
    void destroy();

    void
    onAcceptConnection(const bsl::shared_ptr<ntci::Acceptor>&     acceptor,
                       const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
                       const ntca::AcceptEvent&                   event);

    void onChannelResult(ChannelFactoryEvent::Enum       event,
                         const Status&                   status,
                         const bsl::shared_ptr<Channel>& channel);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Tester, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit Tester(bslma::Allocator* basicAllocator = 0);
    ~Tester();

    // MANIPULATORS

    /// (Re-)create the object being tested and reset the state of any
    /// supporting objects.
    void init();

    bsl::shared_ptr<bmqio::NtcChannel> connect();
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
, d_listenResultCallback(
      bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                            &Tester::onChannelResult,
                            this,
                            bdlf::PlaceHolders::_1,
                            bdlf::PlaceHolders::_2,
                            bdlf::PlaceHolders::_3))
, d_connectResultCallback(
      bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                            &Tester::onChannelResult,
                            this,
                            bdlf::PlaceHolders::_1,
                            bdlf::PlaceHolders::_2,
                            bdlf::PlaceHolders::_3))
, d_listenChannels(d_allocator_p)
, d_connectChannels(d_allocator_p)
, d_semaphore()
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
    const bsl::shared_ptr<ntci::Acceptor>&     acceptor,
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
    const ntca::AcceptEvent&                   event)
{
    if (event.isError()) {
        return;
    }

    bsl::shared_ptr<bmqio::NtcChannel> channel;
    channel.createInplace(d_allocator_p,
                          d_interface_sp,
                          d_listenResultCallback,
                          d_allocator_p);

    channel->import(streamSocket);

    ntsa::Error error = acceptor->accept(
        ntca::AcceptOptions(),
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &Tester::onAcceptConnection,
                              this,
                              bdlf::PlaceHolders::_1,
                              bdlf::PlaceHolders::_2,
                              bdlf::PlaceHolders::_3));
    BMQTST_ASSERT_EQ(error, ntsa::Error::e_OK);

    d_listenChannels.push_back(channel);
    d_semaphore.post();
}

void Tester::onChannelResult(ChannelFactoryEvent::Enum       event,
                             const Status&                   status,
                             const bsl::shared_ptr<Channel>& channel)
{
    d_connectChannels.push_back(channel);
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
    listenerSocketOptions.setKeepHalfOpen(true);
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
    channel.createInplace(d_allocator_p,
                          d_interface_sp,
                          d_connectResultCallback,
                          d_allocator_p);

    bmqio::Status         status(d_allocator_p);
    bmqio::ConnectOptions options(d_allocator_p);
    options.setEndpoint(d_listener_sp->sourceEndpoint().text())
        .setNumAttempts(1)
        .setAttemptInterval(bsls::TimeInterval(1));
    const int rc = channel->connect(&status, options);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(status.category(), bmqio::StatusCategory::e_SUCCESS);

    ntci::AcceptCallback acceptCallback = d_listener_sp->createAcceptCallback(
        bdlf::BindUtil::bindS(d_allocator_p,
                              &Tester::onAcceptConnection,
                              this,
                              bdlf::PlaceHolders::_1,
                              bdlf::PlaceHolders::_2,
                              bdlf::PlaceHolders::_3),
        d_allocator_p);

    ntsa::Error error = d_listener_sp->accept(ntca::AcceptOptions(),
                                              acceptCallback);
    BMQTST_ASSERT_EQ(error, ntsa::Error::e_OK);

    d_semaphore.wait();

    return channel;
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

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(0);
}
