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

#include <mqbauthz_defaultauthorizer.h>

// MQB
#include <mqbact_actions.h>
#include <mqbauthz_policy.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_authenticator.h>
#include <mqbplug_authorizer.h>
#include <mqbpoly_policies.h>

// BDE
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsla_nodiscard.h>
#include <bslma_managedptr.h>
#include <bslmf_movableref.h>

// TEST_DRIVER
#include <bmqtst_testhelper.h>
#include <gtest/gtest.h>

// CONVENIENCE
using namespace BloombergLP;

namespace {

class TestAuthenticationResult : public mqbplug::AuthenticationResult {
    static const bsl::optional<bsls::Types::Uint64> s_LIFETIME_MS;

  public:
    /// Return the principal in human-readable format.
    bsl::string_view principal() const BSLS_KEYWORD_OVERRIDE
    {
        return "anonymous";
    }

    /// Return the remaining lifetime of an authenticated session.
    const bsl::optional<bsls::Types::Uint64>&
    lifetimeMs() const BSLS_KEYWORD_OVERRIDE
    {
        return s_LIFETIME_MS;
    }
};

const bsl::optional<bsls::Types::Uint64>
    TestAuthenticationResult::s_LIFETIME_MS = bsl::make_optional(60);

}

class DefaultAuthorizerTest : public ::testing::Test {
  protected:
    DefaultAuthorizerTest() {}

    BSLA_NODISCARD static int
    makePolicy(bslma::ManagedPtr<mqbauthz::Policy>* res)
    {
        mqbpoly::Policy policy(bmqtst::TestHelperUtil::allocator());
        bsl::vector<mqbpoly::Role>& roles = policy.roles();

        mqbpoly::Role& role = roles.emplace_back();
        role.id().name()    = "anonymous";

        bsl::vector<mqbpoly::Permission>& permissions = role.permissions();
        mqbpoly::Permission& permission = permissions.emplace_back();
        permission.action()             = "connectClient";

        *res = bslma::ManagedPtrUtil::allocateManaged<mqbauthz::Policy>(
            bmqtst::TestHelperUtil::allocator());
        return mqbauthz::Policy::parse(res->get(),
                                       policy,
                                       bmqtst::TestHelperUtil::allocator());
    }

    ~DefaultAuthorizerTest() BSLS_KEYWORD_OVERRIDE;
};

DefaultAuthorizerTest::~DefaultAuthorizerTest()
{
    // NOTHING
}

TEST_F(DefaultAuthorizerTest, breathingTest)
{
    mqbauthz::DefaultAuthorizer authorizer;
    bsl::string                 name(authorizer.name());
    EXPECT_STREQ("DefaultAuthorizer", name.c_str());
}

TEST_F(DefaultAuthorizerTest, acceptsPolicy)
{
    bslma::ManagedPtr<mqbauthz::Policy> policy;
    ASSERT_EQ(0, makePolicy(&policy));
    mqbauthz::DefaultAuthorizer authorizer(
        bslmf::MovableRefUtil::move(policy));

    bsl::string name(authorizer.name());
    EXPECT_STREQ("DefaultAuthorizer", name.c_str());

    mqbact::Action connectClient;
    connectClient.makeConnectClient();
    TestAuthenticationResult authnResult;
    EXPECT_TRUE(authorizer.authorize(connectClient, authnResult));
}

class DefaultAuthorizerAccessTest : public DefaultAuthorizerTest {
  protected:
    typedef bsl::vector<mqbact::Action> TestCases;
    TestCases                           d_cases;

    DefaultAuthorizerAccessTest()
    : d_cases(bmqtst::TestHelperUtil::allocator())
    {
        bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

        // Build the test data
        bsl::string testNode("testNode", alloc);
        bsl::string testQueue("testQueue", alloc);
        bsl::string testCommand("TEST", alloc);

        mqbact::Action& connectClient = d_cases.emplace_back();
        connectClient.makeConnectClient();

        mqbact::Action& connectProxy = d_cases.emplace_back();
        connectProxy.makeConnectProxy();

        mqbact::Action& connectAdmin = d_cases.emplace_back();
        connectAdmin.makeConnectAdmin();

        mqbact::Action& connectClusterNode = d_cases.emplace_back();
        mqbact::ConnectClusterNode& clusterNode =
            connectClusterNode.makeConnectClusterNode();
        clusterNode.clusterName() = testNode;

        mqbact::Action&    actQueueRead = d_cases.emplace_back();
        mqbact::QueueRead& queueRead    = actQueueRead.makeQueueRead();
        queueRead.uri()                 = testQueue;

        mqbact::Action&     actQueueWrite = d_cases.emplace_back();
        mqbact::QueueWrite& queueWrite    = actQueueWrite.makeQueueWrite();
        queueWrite.uri()                  = testQueue;

        mqbact::Action& actExecuteAdminCommand = d_cases.emplace_back();
        mqbact::ExecuteAdminCommand& executeAdminCommand =
            actExecuteAdminCommand.makeExecuteAdminCommand();
        executeAdminCommand.command() = testCommand;
    }

    ~DefaultAuthorizerAccessTest() BSLS_KEYWORD_OVERRIDE;
};

DefaultAuthorizerAccessTest::~DefaultAuthorizerAccessTest()
{
    // NOTHING
}

TEST_F(DefaultAuthorizerAccessTest, allActionsAreAllowedWithDefaultConstructor)
{
    TestAuthenticationResult authnResult;

    const TestCases& cases = d_cases;

    mqbauthz::DefaultAuthorizer authorizer;
    for (TestCases::const_iterator it = cases.cbegin(), end = cases.cend();
         it != end;
         ++it) {
        EXPECT_TRUE(authorizer.authorize(*it, authnResult));
    }
}

TEST_F(DefaultAuthorizerAccessTest, someActionsAreAllowedWithPolicy)
{
    TestAuthenticationResult authnResult;

    const TestCases& cases = d_cases;

    bslma::ManagedPtr<mqbauthz::Policy> policy;
    ASSERT_EQ(0, makePolicy(&policy));
    mqbauthz::DefaultAuthorizer authorizer(
        bslmf::MovableRefUtil::move(policy));

    for (TestCases::const_iterator it = cases.cbegin(), end = cases.cend();
         it != end;
         ++it) {
        bool isAuthorized = authorizer.authorize(*it, authnResult);
        if (it->isConnectClientValue()) {
            EXPECT_TRUE(isAuthorized);
        }
        else {
            EXPECT_FALSE(isAuthorized);
        }
    }
}

TEST(DefaultAuthorizerFactory, factoryBreathingTest)
{
    mqbauthz::DefaultAuthorizerPluginFactory factory;
    bslma::ManagedPtr<mqbplug::Authorizer>   authorizer = factory.create(
        bmqtst::TestHelperUtil::allocator());
}

// ========================================================================
//                                  MAIN
// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    // 'DefaultAuthorizerPluginFactory::create()' looks up its settings from
    // the broker's global authorization configuration, so it must be set
    // before any test constructs an authorizer.
    mqbcfg::AppConfig brokerConfig(bmqtst::TestHelperUtil::allocator());
    mqbcfg::BrokerConfig::set(brokerConfig);

    ::testing::InitGoogleTest(&argc, argv);

    bmqtst::TestHelperUtil::testStatus() = RUN_ALL_TESTS();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
