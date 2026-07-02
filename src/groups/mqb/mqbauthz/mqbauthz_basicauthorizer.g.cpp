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

#include <bslstl_optional.h>
#include <mqbauthz_basicauthorizer.h>

// MQB
#include <mqbplug_authenticator.h>
#include <mqbplug_authorizer.h>

// BDE
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>

// TEST_DRIVER
#include <bmqtst_testhelper.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// CONVENIENCE
using namespace BloombergLP;

namespace {

class MockAuthorizer : public mqbplug::Authorizer {
  public:
    MOCK_METHOD2(authorizer,
                 bool(const mqbplug::Action&               action,
                      const mqbplug::AuthenticationResult& authnResult));
};

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
    ~BasicAuthorizerTest() BSLS_KEYWORD_OVERRIDE {}

    mqbauthz::BasicAuthorizer& authorizer() { return d_authorizer; }
};

TEST_F(BasicAuthorizerTest, breathingTest)
{
    // NOTHING
}

TEST_F(BasicAuthorizerTest, name)
{
    bsl::string name(authorizer().name());
    EXPECT_STREQ("BasicAuthorizer", name.c_str());
}

TEST_F(BasicAuthorizerTest, actionsAreAllowed)
{
    bslma::Allocator*        alloc = bmqtst::TestHelperUtil::allocator();
    TestAuthenticationResult authnResult;

    typedef bsl::vector<mqbplug::Action> TestCases;

    TestCases cases(alloc);
    {
        // Build the test data
        bsl::string testNode("testNode", alloc);
        bsl::string testQueue("testQueue", alloc);
        bsl::string testCommand("TEST", alloc);
        cases.emplace_back(mqbplug::Action::ConnectClient());
        cases.emplace_back(mqbplug::Action::ConnectProxy());
        cases.emplace_back(mqbplug::Action::ConnectAdmin());
        cases.emplace_back(mqbplug::Action::ConnectClusterNode(testNode));
        cases.emplace_back(mqbplug::Action::QueueRead(testQueue));
        cases.emplace_back(mqbplug::Action::QueueWrite(testQueue));
        cases.emplace_back(mqbplug::Action::ExecuteAdminCommand(testCommand));
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
