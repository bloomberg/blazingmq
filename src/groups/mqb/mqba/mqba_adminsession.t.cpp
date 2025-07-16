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

// mqba_adminsession.t.cpp                                            -*-C++-*-
#include <mqba_adminsession.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbmock_dispatcher.h>
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>

#include <bmqio_channel.h>
#include <bmqio_testchannel.h>
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
#include <bsla_annotations.h>

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

mqbmock::Dispatcher* setInDispatcherThread(mqbmock::Dispatcher* mockDispatcher)
// Utility method.  Sets 'MockDispatcher' attribute.
{
    mockDispatcher->_setInDispatcherThread(true);
    return mockDispatcher;
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
        BSLA_UNUSED const bslstl::StringRef&            source,
        const bsl::string&                              cmd,
        const mqbnet::Session::AdminCommandProcessedCb& onProcessedCb)
    {
        int rc = 0;
        onProcessedCb(rc, cmd);
        return rc;
    }
};

/// The `TestBench` holds system components together.
class TestBench {
  public:
    // DATA
    bdlbb::PooledBlobBufferFactory      d_bufferFactory;
    BlobSpPool                          d_blobSpPool;
    bsl::shared_ptr<bmqio::TestChannel> d_channel;
    mqbmock::Dispatcher                 d_mockDispatcher;
    bdlmt::EventScheduler               d_scheduler;
    TestClock                           d_testClock;
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
    , d_channel(new bmqio::TestChannel(allocator))
    , d_mockDispatcher(allocator)
    , d_scheduler(bsls::SystemClockType::e_MONOTONIC, allocator)
    , d_testClock(d_scheduler)
    , d_as(d_channel,
           negotiationMessage,
           "sessionDescription",
           bsl::allocate_shared<mqbnet::AuthenticationContext>(
               allocator,
               bsl::nullptr_t(),
               bmqp_ctrlmsg::AuthenticationMessage(allocator),
               bmqp::EncodingType::e_UNKNOWN,
               mqbnet::AuthenticationContext::ReauthenticateCb(),
               mqbnet::AuthenticationContext::e_AUTHENTICATING,
               mqbnet::ConnectionType::e_UNKNOWN),
           setInDispatcherThread(&d_mockDispatcher),
           &d_blobSpPool,
           &d_scheduler,
           adminEnqueueCb,
           allocator)
    , d_allocator_p(allocator)
    {
        // Typically done during 'Dispatcher::registerClient()'.
        d_as.dispatcherClientData().setDispatcher(&d_mockDispatcher);
        d_mockDispatcher._setInDispatcherThread(true);

        // Setup test time source
        bmqsys::Time::shutdown();
        bmqsys::Time::initialize(
            bdlf::BindUtil::bind(&TestClock::realtimeClock, &d_testClock),
            bdlf::BindUtil::bind(&TestClock::monotonicClock, &d_testClock),
            bdlf::BindUtil::bind(&TestClock::highResTimer, &d_testClock),
            d_allocator_p);

        int rc = d_scheduler.start();
        BMQTST_ASSERT_EQ(rc, 0);
    }

    /// Destructor
    ~TestBench()
    {
        d_as.tearDown(bsl::shared_ptr<void>(), true);
        d_scheduler.cancelAllEventsAndWait();
        d_scheduler.stop();
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
    tb.d_channel->setWriteStatus(status);

    // Send the sample admin event multiple times to the admin session
    for (size_t i = 0; i < numMessages; i++) {
        tb.d_as.processEvent(adminEvent);
        BSLS_ASSERT(tb.d_channel->waitFor(1, false));
    }

    // Check if callback loop delivered admin commands execution results back
    // to the admin session and it writes the needed number of responses to the
    // test channel
    BMQTST_ASSERT_EQ(tb.d_channel->writeCalls().size(), numMessages);

    // Sanity check for the first admin response
    bmqp::Event adminResponseEvent(&tb.d_channel->writeCalls().at(0).d_blob,
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

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    {
        bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());
        bmqsys::Time::initialize(bmqtst::TestHelperUtil::allocator());

        mqbcfg::AppConfig brokerConfig(bmqtst::TestHelperUtil::allocator());
        mqbcfg::BrokerConfig::set(brokerConfig);

        mqbu::MessageGUIDUtil::initialize();

        switch (_testCase) {
        case 0:
        case 1: test1_watermark(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            bmqtst::TestHelperUtil::testStatus() = -1;
        } break;
        }

        bmqsys::Time::shutdown();
        bmqp::ProtocolUtil::shutdown();
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
    // Do not check for default/global allocator usage.
}
