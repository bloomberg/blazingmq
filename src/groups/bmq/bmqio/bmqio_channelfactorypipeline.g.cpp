// Copyright 2019-2023 Bloomberg Finance L.P.
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

#include <bmqio_channelfactorypipeline.h>

// BMQ
#include <bmqio_channelfactory.h>
#include <bmqio_connectoptions.h>
#include <bmqio_listenoptions.h>
#include <bmqio_ntcchannel.h>
#include <bmqio_ntcchannelfactory.h>
#include <bmqio_testchannelfactory.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_memory.h>
#include <bsla_annotations.h>
#include <bslmf_movableref.h>
#include <bsls_asserttestexception.h>

// TEST_DRIVER
#include <bmqtst_testhelper.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// CONVENIENCE
using namespace BloombergLP;

namespace {

class MockChannelFactory : public bmqio::ChannelFactory {
  public:
    MOCK_METHOD4(listen,
                 void(bmqio::Status*               status,
                      bslma::ManagedPtr<OpHandle>* handle,
                      const bmqio::ListenOptions&  options,
                      const ResultCallback&        cb));
    MOCK_METHOD4(connect,
                 void(bmqio::Status*               status,
                      bslma::ManagedPtr<OpHandle>* handle,
                      const bmqio::ConnectOptions& options,
                      const ResultCallback&        cb));
    MOCK_METHOD0(start, int());
    MOCK_METHOD0(stop, void());
};

class WrappedChannelFactory : public bmqio::ChannelFactory {
  private:
    bmqio::ChannelFactory* d_wrapped;

  public:
    WrappedChannelFactory(bmqio::ChannelFactory* wrapped)
    : d_wrapped(wrapped)
    {
        BSLS_ASSERT(d_wrapped != NULL);
    }

    void listen(bmqio::Status*               status,
                bslma::ManagedPtr<OpHandle>* handle,
                const bmqio::ListenOptions&  options,
                const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE
    {
        d_wrapped->listen(status, handle, options, cb);
    }

    void connect(bmqio::Status*               status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const bmqio::ConnectOptions& options,
                 const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE
    {
        d_wrapped->connect(status, handle, options, cb);
    }

    int start() BSLS_KEYWORD_OVERRIDE { return 0; }

    void stop() BSLS_KEYWORD_OVERRIDE {}
};

}

// ========================================================================
//                                  TESTS
// ------------------------------------------------------------------------

class ChannelFactoryPipelineConfigTest : public ::testing::Test {
  protected:
    ChannelFactoryPipelineConfigTest() {}
    ~ChannelFactoryPipelineConfigTest() BSLS_KEYWORD_OVERRIDE;

    typedef bsl::allocator<unsigned char>         allocator_type;
    typedef bmqio::ChannelFactoryPipeline::Config Config;
    typedef bslmf::MovableRefUtil                 MoveUtil;

    allocator_type get_allocator() const
    {
        return bmqtst::TestHelperUtil::allocator();
    }

    bsl::shared_ptr<bmqio::ChannelFactory> testFactory() const
    {
        return bsl::allocate_shared<bmqio::TestChannelFactory>(
            get_allocator());
    }
};

ChannelFactoryPipelineConfigTest::~ChannelFactoryPipelineConfigTest()
{
    // Here to suppress a -Wweak-vtables warning

    // NOTHING
}

TEST_F(ChannelFactoryPipelineConfigTest, breathingTest)
{
    Config builder(testFactory(), get_allocator());
}

TEST_F(ChannelFactoryPipelineConfigTest, add)
{
    Config builder(testFactory(), get_allocator());
    builder.add(testFactory());

    EXPECT_EQ(2, builder.size());
}

TEST_F(ChannelFactoryPipelineConfigTest, addWith)
{
    Config builder(testFactory(), get_allocator());

    struct FactoryFunctor {
        static bsl::shared_ptr<bmqio::ChannelFactory>
        make(allocator_type allocator,
             BSLA_UNUSED bsl::shared_ptr<bmqio::ChannelFactory>& prev)
        {
            return bsl::allocate_shared<bmqio::TestChannelFactory>(allocator);
        }
    };

    using bdlf::PlaceHolders::_1;
    builder.addWith(
        bdlf::BindUtil::bindS(bslma::AllocatorUtil::adapt(get_allocator()),
                              FactoryFunctor::make,
                              get_allocator(),
                              _1));

    EXPECT_EQ(2, builder.size());
}

TEST_F(ChannelFactoryPipelineConfigTest, addWithFails)
{
    Config builder(testFactory(), get_allocator());

    struct FactoryFunctor {
        static bsl::shared_ptr<bmqio::ChannelFactory>
        make(BSLA_UNUSED bsl::shared_ptr<bmqio::ChannelFactory>& prev)
        {
            // This is a contract failure.
            return NULL;
        }
    };

    using bdlf::PlaceHolders::_1;
    EXPECT_THROW(builder.addWith(bdlf::BindUtil::bindS(
                     bslma::AllocatorUtil::adapt(get_allocator()),
                     FactoryFunctor::make,
                     _1)),
                 bsls::AssertTestException);
}

TEST_F(ChannelFactoryPipelineConfigTest, canMove)
{
    Config builder(testFactory(), get_allocator());
    Config builder2(MoveUtil::move(builder), get_allocator());
}

class ChannelFactoryPipelineTest : public ::testing::Test {
  protected:
    ChannelFactoryPipelineTest() {}
    ~ChannelFactoryPipelineTest() BSLS_KEYWORD_OVERRIDE;

    typedef bsl::allocator<unsigned char>         allocator_type;
    typedef bslmf::MovableRefUtil                 MoveUtil;
    typedef bsl::shared_ptr<MockChannelFactory>   MockFactorySP;
    typedef bmqio::ChannelFactoryPipeline::Config Config;

    allocator_type get_allocator() const
    {
        return bmqtst::TestHelperUtil::allocator();
    }

    MockFactorySP mockFactory() const
    {
        using ::testing::NiceMock;
        MockFactorySP factory =
            bsl::allocate_shared<NiceMock<MockChannelFactory> >(
                get_allocator());

        using ::testing::Return;
        ON_CALL(*factory, start()).WillByDefault(Return(0));
        ON_CALL(*factory, stop()).WillByDefault(Return());

        return factory;
    }

    bmqio::ChannelFactoryPipeline::Config::ChannelFactoryBuilder
    wrappedFactoryConfig() const
    {
        struct Config {
            static bsl::shared_ptr<bmqio::ChannelFactory>
            make(allocator_type                          allocator,
                 bsl::shared_ptr<bmqio::ChannelFactory>& prev)
            {
                return bsl::allocate_shared<WrappedChannelFactory>(allocator,
                                                                   prev.get());
            }
        };

        using bdlf::PlaceHolders::_1;
        return bdlf::BindUtil::bindS(
            bslma::AllocatorUtil::adapt(get_allocator()),
            Config::make,
            get_allocator(),
            _1);
    }
};

ChannelFactoryPipelineTest::~ChannelFactoryPipelineTest()
{
    // Here to suppress a -Wweak-vtables warning

    // NOTHING
}

TEST_F(ChannelFactoryPipelineTest, breathingTest)
{
    Config                        config(mockFactory(), get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());
}

TEST_F(ChannelFactoryPipelineTest, stopOnDestructor)
{
    MockFactorySP factory = mockFactory();
    EXPECT_CALL(*factory, stop());

    Config                        config(factory, get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());
}

TEST_F(ChannelFactoryPipelineTest, listenFollowsChain)
{
    MockFactorySP                                      factory = mockFactory();
    bmqio::Status                                      status;
    bslma::ManagedPtr<bmqio::ChannelFactory::OpHandle> handle;
    bmqio::ListenOptions                               options;
    const bmqio::ChannelFactory::ResultCallback        cb;

    using ::testing::_;
    using ::testing::Ref;

    EXPECT_CALL(*factory, listen(&status, &handle, Ref(options), Ref(cb)));

    Config                        config(factory, get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());

    pipeline.start();
    pipeline.listen(&status, &handle, options, cb);
}

TEST_F(ChannelFactoryPipelineTest, connectFollowsChain)
{
    MockFactorySP                                      factory = mockFactory();
    bmqio::Status                                      status;
    bslma::ManagedPtr<bmqio::ChannelFactory::OpHandle> handle;
    bmqio::ConnectOptions                              options;
    const bmqio::ChannelFactory::ResultCallback        cb;

    using ::testing::_;
    using ::testing::Ref;

    EXPECT_CALL(*factory, connect(&status, &handle, Ref(options), Ref(cb)));

    Config                        config(factory, get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(
        MoveUtil::move(config.addWith(wrappedFactoryConfig())),
        get_allocator());

    pipeline.connect(&status, &handle, options, cb);
}

TEST_F(ChannelFactoryPipelineTest, startIsForwarded)
{
    using ::testing::Return;

    MockFactorySP factory1 = mockFactory();

    EXPECT_CALL(*factory1, start()).WillRepeatedly(Return(0));

    Config                        config(factory1, get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());

    EXPECT_EQ(0, pipeline.start());
}

TEST_F(ChannelFactoryPipelineTest, startFailureIsForwarded)
{
    using ::testing::Return;

    MockFactorySP factory1 = mockFactory();

    EXPECT_CALL(*factory1, start()).WillRepeatedly(Return(1));

    Config                        config(factory1, get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());

    EXPECT_NE(0, pipeline.start());
}

TEST_F(ChannelFactoryPipelineTest, startIsForwardedToLastFactory)
{
    using ::testing::Return;

    MockFactorySP factory1 = mockFactory();
    MockFactorySP factory2 = mockFactory();

    EXPECT_CALL(*factory2, start()).WillRepeatedly(Return(0));

    Config config(factory1, get_allocator());
    config.add(factory2);
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());

    EXPECT_EQ(0, pipeline.start());
}

TEST_F(ChannelFactoryPipelineTest, stopIsForwarded)
{
    MockFactorySP factory1 = mockFactory();

    EXPECT_CALL(*factory1, stop());

    Config                        config(factory1, get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());

    pipeline.stop();
    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(factory1.get()));
}

TEST_F(ChannelFactoryPipelineTest, stopIsForwardedToLastFactory)
{
    using ::testing::AnyNumber;
    using ::testing::Return;

    MockFactorySP factory1 = mockFactory();
    MockFactorySP factory2 = mockFactory();

    EXPECT_CALL(*factory2, stop()).Times(AnyNumber());

    Config config(factory1, get_allocator());
    config.add(factory2);
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());

    pipeline.stop();
}

TEST_F(ChannelFactoryPipelineTest, stopIsCalledOnDestruction)
{
    MockFactorySP factory1 = mockFactory();

    EXPECT_CALL(*factory1, stop());

    Config                        config(factory1, get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());
}

TEST_F(ChannelFactoryPipelineTest, getFindsChannelFactorySubtype)
{
    MockFactorySP factory = mockFactory();

    Config                        config(factory, get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());

    MockChannelFactory* result = pipeline.get<MockChannelFactory>();

    EXPECT_EQ(result, factory.get());
}

TEST_F(ChannelFactoryPipelineTest, getDoesntFindNonexistantSubtype)
{
    MockFactorySP factory = mockFactory();

    Config                        config(factory, get_allocator());
    bmqio::ChannelFactoryPipeline pipeline(MoveUtil::move(config),
                                           get_allocator());

    bmqio::NtcChannelFactory* result =
        pipeline.get<bmqio::NtcChannelFactory>();

    EXPECT_EQ(NULL, result);
}

// ========================================================================
//                                  MAIN
// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    ::testing::InitGoogleTest(&argc, argv);

    bmqtst::TestHelperUtil::testStatus() = RUN_ALL_TESTS();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
