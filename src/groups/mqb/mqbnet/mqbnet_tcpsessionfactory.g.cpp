// Copyright 2026 Bloomberg Finance L.P.
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

#include <mqbnet_tcpsessionfactory.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbnet_authenticator.h>
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_negotiator.h>
#include <mqbnet_session.h>
#include <mqbplug_pluginmanager.h>
#include <mqbstat_statcontroller.h>

// BMQ
#include <bmqio_testchannelfactory.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>

// TEST_DRIVER
#include <bmqtst_testhelper.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// CONVENIENCE
using namespace BloombergLP;

namespace {

class MockAuthenticator : public mqbnet::Authenticator {
  public:
    MOCK_METHOD1(start, int(bsl::ostream& errorDescription));

    MOCK_METHOD0(stop, void());

    MOCK_METHOD3(
        handleAuthentication,
        int(bsl::ostream&                              errorDescription,
            mqbnet::InitialConnectionContext*          context_p,
            const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg));

    MOCK_METHOD3(
        handleReauthentication,
        int(bsl::ostream& errorDescription,
            const bsl::shared_ptr<mqbnet::AuthenticationContext>& context_sp,
            const bsl::shared_ptr<bmqio::Channel>&                channel));

    MOCK_METHOD2(
        authenticationOutbound,
        int(bsl::ostream& errorDescription,
            const bsl::shared_ptr<mqbnet::AuthenticationContext>& context_sp));

    MOCK_CONST_METHOD0(anonymousCredential,
                       const bsl::optional<mqbcfg::Credential>&());
};

class MockNegotiator : public mqbnet::Negotiator {
  public:
    MOCK_METHOD3(createSessionOnMsgType,
                 int(bsl::ostream&                     errorDescription,
                     bsl::shared_ptr<mqbnet::Session>* session,
                     mqbnet::InitialConnectionContext* context));

    MOCK_METHOD2(negotiateOutbound,
                 int(bsl::ostream&                     errorDescription,
                     mqbnet::InitialConnectionContext* context));
};

class MockSession : public mqbnet::Session {
  public:
    MOCK_METHOD2(processEvent,
                 void(const bmqp::Event& event, mqbnet::ClusterNode* source));

    MOCK_METHOD2(tearDown,
                 void(const bsl::shared_ptr<void>& handle,
                      bool                         isBrokerShutdown));

    MOCK_METHOD1(initiateShutdown,
                 void(const mqbnet::Session::ShutdownCb& callback));

    MOCK_METHOD0(invalidate, void());

    MOCK_CONST_METHOD0(channel, bsl::shared_ptr<bmqio::Channel>());

    MOCK_CONST_METHOD0(clusterNode, mqbnet::ClusterNode*());

    MOCK_CONST_METHOD0(negotiationMessage,
                       const bmqp_ctrlmsg::NegotiationMessage&());

    MOCK_CONST_METHOD0(description, bsl::string_view());
};

class MockChannel : public bmqio::Channel {
  public:
    MOCK_METHOD4(read,
                 void(bmqio::Status*                      status,
                      int                                 numBytes,
                      const bmqio::Channel::ReadCallback& readCallback,
                      const bsls::TimeInterval&           timeout));

    MOCK_METHOD3(write,
                 void(bmqio::Status*     status,
                      const bdlbb::Blob& blob,
                      bsls::Types::Int64 watermark));

    MOCK_METHOD0(cancelRead, void());

    MOCK_METHOD1(close, void(const bmqio::Status& status));

    MOCK_METHOD1(execute, int(const bmqio::Channel::ExecuteCb& cb));

    MOCK_METHOD1(onClose,
                 bdlmt::SignalerConnection(const bmqio::Channel::CloseFn& cb));

    MOCK_METHOD1(
        onWatermark,
        bdlmt::SignalerConnection(const bmqio::Channel::WatermarkFn& cb));

    MOCK_METHOD0(properties, bmqvt::PropertyBag&());

    MOCK_METHOD1(setWriteQueueLowWatermark, void(int lowWatermark));

    MOCK_METHOD1(setWriteQueueHighWatermark, void(int highWatermark));

    MOCK_CONST_METHOD0(peerUri, const bsl::string&());

    MOCK_CONST_METHOD0(properties, const bmqvt::PropertyBag&());
};

}

class TCPSessionFactoryTest : public ::testing::Test {
  protected:
    bslma::Allocator*              d_allocator;
    mqbcfg::AppConfig              d_appConfig;
    mqbcfg::TcpInterfaceConfig     d_tcpConfig;
    bdlmt::EventScheduler          d_scheduler;
    bdlbb::PooledBlobBufferFactory d_blobBufferFactory;
    MockAuthenticator              d_authenticator;
    MockNegotiator                 d_negotiator;
    mqbplug::PluginManager         d_pluginManager;
    mqbstat::StatController        d_statController;
    mqbnet::TCPSessionFactory      d_tcpSessionFactory;

    /// `fake` command processor to pass to the object under test, print the
    /// specified `cmd` from the specified `source` to stdout, and write
    /// `SUCCESS` to the specified `os`, return 0.
    static int processCommand(const bslstl::StringRef& source,
                              const bsl::string&       cmd,
                              bsl::ostream&            os)
    {
        PRINT_SAFE("Processing command '" << cmd << "' from '" << source
                                          << "'");
        os << "SUCCESS";

        return 0;
    }

    mqbstat::StatController::CommandProcessorFn
    processCommandFn(bslma::Allocator* allocator)
    {
        using namespace bdlf::PlaceHolders;
        return bdlf::BindUtil::bindS(allocator, processCommand, _1, _2, _3);
    }

    TCPSessionFactoryTest()
    : d_allocator(bmqtst::TestHelperUtil::allocator())
    , d_appConfig(d_allocator)
    , d_tcpConfig(d_allocator)
    , d_scheduler(bsls::SystemClockType::e_MONOTONIC, d_allocator)
    , d_blobBufferFactory(1024, d_allocator)
    , d_authenticator()
    , d_negotiator()
    , d_pluginManager(d_allocator)
    , d_statController(processCommandFn(d_allocator),
                       &d_pluginManager,
                       &d_blobBufferFactory,
                       0,  // no allocatorsStatContext
                       &d_scheduler,
                       d_allocator)
    , d_tcpSessionFactory(d_tcpConfig,
                          &d_scheduler,
                          &d_blobBufferFactory,
                          &d_authenticator,
                          &d_negotiator,
                          &d_statController,
                          d_allocator)
    {
        mqbcfg::BrokerConfig::set(d_appConfig);
    }

    ~TCPSessionFactoryTest() BSLS_KEYWORD_OVERRIDE;

    mqbnet::TCPSessionFactory& obj() { return d_tcpSessionFactory; }
};

TCPSessionFactoryTest::~TCPSessionFactoryTest()
{
    // NOTHING
}

TEST_F(TCPSessionFactoryTest, setNodeWriteQueueWatermarksFromTcpConfig)
{
    using ::testing::NiceMock;

    NiceMock<MockSession>                   session;
    bsl::shared_ptr<NiceMock<MockChannel> > channel =
        bsl::allocate_shared<NiceMock<MockChannel> >(d_allocator);

    using ::testing::Return;

    ON_CALL(session, channel()).WillByDefault(Return(channel));
    EXPECT_CALL(*channel,
                setWriteQueueLowWatermark(d_tcpConfig.nodeLowWatermark()));
    EXPECT_CALL(*channel,
                setWriteQueueHighWatermark(d_tcpConfig.nodeHighWatermark()));

    obj().setNodeWriteQueueWatermarks(session);
}

// ========================================================================
//                                  MAIN
// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    ::testing::InitGoogleTest(&argc, argv);

    bmqtst::TestHelperUtil::testStatus() = RUN_ALL_TESTS();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
