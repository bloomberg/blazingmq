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

#include <mqbauthz_basicauthorizer.h>

// MQB
#include <mqbact_actions.h>
#include <mqbplug_authenticator.h>
#include <mqbplug_authorizer.h>

// BDE
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>

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
        return "test:principal";
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

class BasicAuthorizerTest : public ::testing::Test {
  private:
    mqbauthz::BasicAuthorizer d_authorizer;

  protected:
    BasicAuthorizerTest()
    : d_authorizer()
    {
    }

    ~BasicAuthorizerTest() BSLS_KEYWORD_OVERRIDE;

    mqbauthz::BasicAuthorizer& authorizer() { return d_authorizer; }
};

BasicAuthorizerTest::~BasicAuthorizerTest()
{
}

TEST_F(BasicAuthorizerTest, breathingTest)
{
    bsl::string name(authorizer().name());
    EXPECT_STREQ("BasicAuthorizer", name.c_str());
}

TEST_F(BasicAuthorizerTest, allActionsAreAllowed)
{
    bslma::Allocator*        alloc = bmqtst::TestHelperUtil::allocator();
    TestAuthenticationResult authnResult;

    typedef bsl::vector<mqbact::Action> TestCases;

    TestCases cases(alloc);
    {
        // Build the test data
        bsl::string testNode("testNode", alloc);
        bsl::string testQueue("testQueue", alloc);
        bsl::string testCommand("TEST", alloc);

        mqbact::Action& connectClient = cases.emplace_back();
        connectClient.makeConnectClient();

        mqbact::Action& connectProxy = cases.emplace_back();
        connectProxy.makeConnectProxy();

        mqbact::Action& connectAdmin = cases.emplace_back();
        connectAdmin.makeConnectAdmin();

        mqbact::Action&             connectClusterNode = cases.emplace_back();
        mqbact::ConnectClusterNode& clusterNode =
            connectClusterNode.makeConnectClusterNode();
        clusterNode.clusterName() = testNode;

        mqbact::Action&    actQueueRead = cases.emplace_back();
        mqbact::QueueRead& queueRead    = actQueueRead.makeQueueRead();
        queueRead.uri()                 = testQueue;

        mqbact::Action&     actQueueWrite = cases.emplace_back();
        mqbact::QueueWrite& queueWrite    = actQueueWrite.makeQueueWrite();
        queueWrite.uri()                  = testQueue;

        mqbact::Action& actExecuteAdminCommand = cases.emplace_back();
        mqbact::ExecuteAdminCommand& executeAdminCommand =
            actExecuteAdminCommand.makeExecuteAdminCommand();
        executeAdminCommand.command() = testCommand;
    }

    for (TestCases::const_iterator it = cases.cbegin(), end = cases.cend();
         it != end;
         ++it) {
        EXPECT_TRUE(authorizer().authorize(*it, authnResult));
    }
}

TEST(BasicAuthorizerFactory, factoryBreathingTest)
{
    mqbauthz::BasicAuthorizerPluginFactory factory;
    bslma::ManagedPtr<mqbplug::Authorizer> authorizer = factory.create(
        bmqtst::TestHelperUtil::allocator());
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
