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
#include <bmqio_testchannelfactory.h>

// BDE
#include <bslmf_movableref.h>

// TEST_DRIVER
#include <bmqtst_testhelper.h>
#include <bsls_assert.h>
#include <bsls_asserttestexception.h>
#include <bslstl_sharedptr.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// CONVENIENCE
using namespace BloombergLP;

namespace {

class MockChannelFactory : public bmqio::ChannelFactory {
  public:
    MOCK_METHOD(void,
                listen,
                (bmqio::Status * status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const bmqio::ListenOptions&  options,
                 const ResultCallback&        cb),
                (BSLS_KEYWORD_OVERRIDE));
    MOCK_METHOD(void,
                connect,
                (bmqio::Status * status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const bmqio::ConnectOptions& options,
                 const ResultCallback&        cb),
                (BSLS_KEYWORD_OVERRIDE));
    MOCK_METHOD(int, start, (), (BSLS_KEYWORD_OVERRIDE));
    MOCK_METHOD(void, stop, (), (BSLS_KEYWORD_OVERRIDE));
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

class ChannelFactoryPipelineBuilderTest : public ::testing::Test {
  protected:
    ChannelFactoryPipelineBuilderTest() {}

    typedef bsl::allocator<unsigned char> allocator_type;

    allocator_type get_allocator() const
    {
        return bmqtst::TestHelperUtil::allocator();
    }
};

TEST_F(ChannelFactoryPipelineBuilderTest, breathingTest)
{
    bmqio::ChannelFactoryPipeline::Builder builder(get_allocator());
}

TEST_F(ChannelFactoryPipelineBuilderTest, add)
{
    bmqio::ChannelFactoryPipeline::Builder builder(get_allocator());
    builder.add(
        bsl::allocate_shared<bmqio::TestChannelFactory>(get_allocator()));

    EXPECT_EQ(1, builder.size());
}

TEST_F(ChannelFactoryPipelineBuilderTest, addWith)
{
    bmqio::ChannelFactoryPipeline::Builder builder(get_allocator());

    struct FactoryFunctor {
        allocator_type allocator;

        bsl::shared_ptr<bmqio::ChannelFactory>
        operator()(bsl::shared_ptr<bmqio::ChannelFactory> prev) const
        {
            EXPECT_TRUE(NULL == prev);
            return bsl::allocate_shared<bmqio::TestChannelFactory>(allocator);
        }
    };

    FactoryFunctor func{get_allocator()};

    builder.addWith(func);

    EXPECT_EQ(1, builder.size());
}

TEST_F(ChannelFactoryPipelineBuilderTest, addWithFails)
{
    bmqio::ChannelFactoryPipeline::Builder builder(get_allocator());

    struct FactoryFunctor {
        bsl::shared_ptr<bmqio::ChannelFactory>
        operator()(bsl::shared_ptr<bmqio::ChannelFactory> prev) const
        {
            return NULL;
        }
    };

    FactoryFunctor func;

    // This is a contract failure.
    EXPECT_THROW(builder.addWith(func), bsls::AssertTestException);
}

TEST_F(ChannelFactoryPipelineBuilderTest, addWithPrev)
{
    bmqio::ChannelFactoryPipeline::Builder builder(get_allocator());

    struct FactoryFunctor {
        allocator_type allocator;

        bsl::shared_ptr<bmqio::ChannelFactory>
        operator()(bsl::shared_ptr<bmqio::ChannelFactory> prev) const
        {
            return bsl::allocate_shared<bmqio::TestChannelFactory>(allocator);
        }
    };

    FactoryFunctor func{get_allocator()};

    builder.addWith(func);

    ASSERT_EQ(1, builder.size());
}

TEST_F(ChannelFactoryPipelineBuilderTest, canMove)
{
    bmqio::ChannelFactoryPipeline::Builder builder(
        (bmqtst::TestHelperUtil::allocator()));
    bmqio::ChannelFactoryPipeline::Builder builder2 =
        bslmf::MovableRefUtil::move(builder);
}

class ChannelFactoryPipelineTest : public ::testing::Test {
  protected:
    ChannelFactoryPipelineTest()
    : d_builder(get_allocator())
    {
    }

    typedef bsl::allocator<unsigned char> allocator_type;

    allocator_type get_allocator() const
    {
        return bmqtst::TestHelperUtil::allocator();
    }

    bmqio::ChannelFactoryPipeline::Builder::ChannelFactoryBuilder
    wrappedFactoryBuilder() const
    {
        struct Builder {
            allocator_type allocator;

            bsl::shared_ptr<bmqio::ChannelFactory>
            operator()(bsl::shared_ptr<bmqio::ChannelFactory> prev) const
            {
                return bsl::allocate_shared<WrappedChannelFactory>(allocator,
                                                                   prev.get());
            }
        };

        return Builder{get_allocator()};
    }

    bmqio::ChannelFactoryPipeline::Builder d_builder;
};

TEST_F(ChannelFactoryPipelineTest, breathingTest)
{
    bmqio::ChannelFactoryPipeline pipeline = d_builder.build();
}

TEST_F(ChannelFactoryPipelineTest, listenFollowsChain)
{
    bsl::shared_ptr<MockChannelFactory> mockFactory =
        bsl::allocate_shared<MockChannelFactory>(get_allocator());
    bmqio::Status                                      status;
    bslma::ManagedPtr<bmqio::ChannelFactory::OpHandle> handle;
    bmqio::ListenOptions                               options;
    const bmqio::ChannelFactory::ResultCallback        cb;

    using ::testing::_;
    using ::testing::Address;

    EXPECT_CALL(*mockFactory,
                listen(&status, &handle, Address(&options), Address(&cb)));

    bmqio::ChannelFactoryPipeline pipeline =
        d_builder.add(mockFactory).addWith(wrappedFactoryBuilder()).build();

    pipeline.listen(&status, &handle, options, cb);
}

TEST_F(ChannelFactoryPipelineTest, connectFollowsChain)
{
    bsl::shared_ptr<MockChannelFactory> mockFactory =
        bsl::allocate_shared<MockChannelFactory>(get_allocator());
    bmqio::Status                                      status;
    bslma::ManagedPtr<bmqio::ChannelFactory::OpHandle> handle;
    bmqio::ConnectOptions                              options;
    const bmqio::ChannelFactory::ResultCallback        cb;

    using ::testing::_;
    using ::testing::Address;

    EXPECT_CALL(*mockFactory,
                connect(&status, &handle, Address(&options), Address(&cb)));

    bmqio::ChannelFactoryPipeline pipeline =
        d_builder.add(mockFactory).addWith(wrappedFactoryBuilder()).build();

    pipeline.connect(&status, &handle, options, cb);
}

TEST_F(ChannelFactoryPipelineTest, startGoesFromTopToBottom)
{
    bsl::shared_ptr<MockChannelFactory> mockFactory1 =
        bsl::allocate_shared<MockChannelFactory>(get_allocator());
    bsl::shared_ptr<MockChannelFactory> mockFactory2 =
        bsl::allocate_shared<MockChannelFactory>(get_allocator());

    using ::testing::InSequence;
    using ::testing::Invoke;

    {
        InSequence seq;
        EXPECT_CALL(*mockFactory2, start())
            .WillRepeatedly(
                Invoke(mockFactory1.get(), &MockChannelFactory::start));
        EXPECT_CALL(*mockFactory1, start());
    }
    bmqio::ChannelFactoryPipeline pipeline =
        d_builder.add(mockFactory1).add(mockFactory2).build();

    pipeline.start();
}

TEST_F(ChannelFactoryPipelineTest, stopGoesFromTopToBottom)
{
    bsl::shared_ptr<MockChannelFactory> mockFactory1 =
        bsl::allocate_shared<MockChannelFactory>(get_allocator());
    bsl::shared_ptr<MockChannelFactory> mockFactory2 =
        bsl::allocate_shared<MockChannelFactory>(get_allocator());

    using ::testing::InSequence;
    using ::testing::Invoke;

    {
        InSequence seq;
        EXPECT_CALL(*mockFactory2, stop())
            .WillRepeatedly(
                Invoke(mockFactory1.get(), &MockChannelFactory::stop));
        EXPECT_CALL(*mockFactory1, stop());
    }
    bmqio::ChannelFactoryPipeline pipeline =
        d_builder.add(mockFactory1).add(mockFactory2).build();

    pipeline.stop();
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
