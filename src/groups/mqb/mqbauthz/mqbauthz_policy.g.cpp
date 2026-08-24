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

#include <mqbauthz_policy.h>

// MQB
#include <mqbact_actions.h>
#include <mqbpoly_policies.h>

// BMQ
#include <bmqt_uri.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_bslallocator.h>
#include <bslma_managedptr.h>
#include <bslma_testallocatormonitor.h>

// TEST_DRIVER
#include <bmqtst_testhelper.h>
#include <gtest/gtest.h>

// CONVENIENCE
using namespace BloombergLP;

namespace {

class BaseTest : public ::testing::Test {
    bslma::TestAllocatorMonitor d_tam;

  protected:
    BaseTest()
    : d_tam(&bmqtst::TestHelperUtil::defaultAllocator())
    {
    }

    ~BaseTest() BSLS_KEYWORD_OVERRIDE { EXPECT_TRUE(d_tam.isInUseSame()); }
};

}

class TestPolicy_UriResource : public BaseTest {};

TEST_F(TestPolicy_UriResource, defaultMatchesNone)
{
    bsl::allocator<>         alloc = bmqtst::TestHelperUtil::allocator();
    bsl::vector<bsl::string> testCases(alloc);
    testCases.emplace_back("bmq://bmq-test.uri-1/q123");
    testCases.emplace_back("bmq://BMQ-TEST.URI-1/q123");
    testCases.emplace_back("bmq://bmq-test.uri-1.~pd/q123");

    mqbauthz::Policy_UriResource uriResource;

    bmqt::Uri uri(alloc.mechanism());
    for (bsl::vector<bsl::string>::const_iterator it  = testCases.cbegin(),
                                                  end = testCases.cend();
         it != end;
         ++it) {
        ASSERT_EQ(0, bmqt::UriParser::parse(&uri, NULL, *it));
        EXPECT_FALSE(uriResource.matches(uri));
    }
}

TEST_F(TestPolicy_UriResource, allowAllMatchesAll)
{
    bsl::allocator<>              alloc = bmqtst::TestHelperUtil::allocator();
    bsl::vector<bsl::string_view> testCases(alloc);
    testCases.emplace_back("bmq://bmq-test.uri-1/q123");
    testCases.emplace_back("bmq://BMQ-TEST.URI-1/q123");
    testCases.emplace_back("bmq://bmq-test.uri-1.~pd/q123");

    mqbauthz::Policy_UriResource uriResource;
    ASSERT_EQ(0, mqbauthz::Policy_UriResource::parse(&uriResource, "*"));

    bmqt::Uri uri(alloc.mechanism());
    for (bsl::vector<bsl::string_view>::const_iterator it = testCases.cbegin(),
                                                       end = testCases.cend();
         it != end;
         ++it) {
        ASSERT_EQ(0, bmqt::UriParser::parse(&uri, NULL, *it));
        EXPECT_TRUE(uriResource.matches(uri));
    }
}

TEST_F(TestPolicy_UriResource, domainPatternMatchesAllQueuesInDomain)
{
    struct TestCase {
        bsl::string_view uri;
        bool             shouldMatch;
    };

    bsl::allocator<>      alloc = bmqtst::TestHelperUtil::allocator();
    bsl::vector<TestCase> testCases(alloc);
    testCases.push_back({"bmq://bmq-test-uri-1/q123", true});
    testCases.push_back({"bmq://bmq-test-uri-1.~pd/q123", true});
    testCases.push_back({"bmq://bmq-test-uri-1.~pd/q123?id=myapp123", true});
    testCases.push_back({"bmq://bmq-test-uri-2/q123", false});
    testCases.push_back({"bmq://BMQ-TEST-URI-1/q123", false});

    mqbauthz::Policy_UriResource uriResource;
    ASSERT_EQ(0,
              mqbauthz::Policy_UriResource::parse(&uriResource,
                                                  "bmq-test-uri-1"));

    bmqt::Uri uri(alloc.mechanism());
    for (bsl::vector<TestCase>::const_iterator it  = testCases.cbegin(),
                                               end = testCases.cend();
         it != end;
         ++it) {
        ASSERT_EQ(0, bmqt::UriParser::parse(&uri, NULL, it->uri));
        EXPECT_EQ(it->shouldMatch, uriResource.matches(uri));
    }
}

TEST_F(TestPolicy_UriResource, queuePatternMatchesQueue)
{
    struct TestCase {
        bsl::string_view uri;
        bool             shouldMatch;
    };

    bsl::allocator<>      alloc = bmqtst::TestHelperUtil::allocator();
    bsl::vector<TestCase> testCases(alloc);
    testCases.push_back({"bmq://bmq-test-uri-1/q123", true});
    testCases.push_back({"bmq://bmq-test-uri-1.~pd/q123", true});
    testCases.push_back({"bmq://bmq-test-uri-1.~pd/q123?id=myapp123", true});
    testCases.push_back({"bmq://bmq-test-uri-1/q456", false});
    testCases.push_back({"bmq://bmq-test-uri-2/q123", false});
    testCases.push_back({"bmq://BMQ-TEST-URI-1/q123", false});

    mqbauthz::Policy_UriResource uriResource;
    ASSERT_EQ(0,
              mqbauthz::Policy_UriResource::parse(&uriResource,
                                                  "bmq-test-uri-1/q123"));

    bmqt::Uri uri(alloc.mechanism());
    for (bsl::vector<TestCase>::const_iterator it  = testCases.cbegin(),
                                               end = testCases.cend();
         it != end;
         ++it) {
        ASSERT_EQ(0, bmqt::UriParser::parse(&uri, NULL, it->uri));
        EXPECT_EQ(it->shouldMatch, uriResource.matches(uri));
    }
}

class TestPolicy_Permission : public BaseTest {
  protected:
    bsl::allocator<> get_allocator() const
    {
        return bmqtst::TestHelperUtil::allocator();
    }

    int decodeRoleJson(mqbpoly::Role* role, bsl::string_view roleJson)
    {
        bsl::allocator<>       alloc = get_allocator();
        bsl::istringstream     roleStr(roleJson);
        baljsn::Decoder        decoder(alloc.mechanism());
        baljsn::DecoderOptions decoderOptions;
        decoderOptions.setSkipUnknownElements(false);
        return decoder.decode(roleStr, role, decoderOptions);
    }
};

TEST_F(TestPolicy_Permission, emptyPermissionDenysAll)
{
    mqbauthz::Policy_Permission permission;

    EXPECT_FALSE(permission.isConnectClientAllowed());
    EXPECT_FALSE(permission.isConnctProxyAllowed());
    EXPECT_FALSE(permission.isConnctAdminAllowed());
    EXPECT_FALSE(permission.isConnctClusterNodeAllowed());

    bsl::allocator<> alloc = bmqtst::TestHelperUtil::allocator();

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://bmq-test.uri-1/q123");
        testCases.emplace_back("bmq://BMQ-TEST.URI-1/q123");
        testCases.emplace_back("bmq://bmq-test.uri-1.~pd/q123");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isQueueReadAllowed(*it));
            EXPECT_FALSE(permission.isQueueWriteAllowed(*it));
        }
    }

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("HELP");
        testCases.emplace_back("DOMAINS DOMAIN foo");
        testCases.emplace_back("CONFIGPROVIDER CACHE_CLEAR ALL");
        testCases.emplace_back("CLUSTERS LIST");
        testCases.emplace_back("BROKERCONFIG DUMP");
        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isExecuteAdminCommandAllowed(*it));
        }
    }
}

TEST_F(TestPolicy_Permission, consumerClient)
{
    bsl::allocator<> alloc    = bmqtst::TestHelperUtil::allocator();
    bsl::string_view roleJson = "{"
                                "   \"id\": { \"name\": \"testUser\" },"
                                "   \"permissions\": ["
                                "       {"
                                "           \"action\": \"connectClient\""
                                "       },"
                                "       {"
                                "           \"action\": \"queueRead\","
                                "           \"resources\": ["
                                "               { \"id\": \"*\" }"
                                "           ]"
                                "       }"
                                "   ]"
                                "}";
    mqbpoly::Role    role(alloc.mechanism());
    ASSERT_EQ(0, decodeRoleJson(&role, roleJson));

    mqbauthz::Policy_Permission permission;
    ASSERT_EQ(0, mqbauthz::Policy_Permission::parse(&permission, role, alloc));

    EXPECT_TRUE(permission.isConnectClientAllowed());
    EXPECT_FALSE(permission.isConnctProxyAllowed());
    EXPECT_FALSE(permission.isConnctAdminAllowed());
    EXPECT_FALSE(permission.isConnctClusterNodeAllowed());

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://bmq-test.uri-1/q123");
        testCases.emplace_back("bmq://BMQ-TEST.URI-1/q123");
        testCases.emplace_back("bmq://bmq-test.uri-1.~pd/q123");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_TRUE(permission.isQueueReadAllowed(*it));
            EXPECT_FALSE(permission.isQueueWriteAllowed(*it));
        }
    }

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("HELP");
        testCases.emplace_back("DOMAINS DOMAIN foo");
        testCases.emplace_back("CONFIGPROVIDER CACHE_CLEAR ALL");
        testCases.emplace_back("CLUSTERS LIST");
        testCases.emplace_back("BROKERCONFIG DUMP");
        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isExecuteAdminCommandAllowed(*it));
        }
    }
}

TEST_F(TestPolicy_Permission, producerClient)
{
    bsl::allocator<> alloc    = bmqtst::TestHelperUtil::allocator();
    bsl::string_view roleJson = "{"
                                "   \"id\": { \"name\": \"testUser\" },"
                                "   \"permissions\": ["
                                "       {"
                                "           \"action\": \"connectClient\""
                                "       },"
                                "       {"
                                "           \"action\": \"queueWrite\","
                                "           \"resources\": ["
                                "               { \"id\": \"*\" }"
                                "           ]"
                                "       }"
                                "   ]"
                                "}";
    mqbpoly::Role    role(alloc.mechanism());
    ASSERT_EQ(0, decodeRoleJson(&role, roleJson));

    mqbauthz::Policy_Permission permission;
    ASSERT_EQ(0, mqbauthz::Policy_Permission::parse(&permission, role, alloc));

    EXPECT_TRUE(permission.isConnectClientAllowed());
    EXPECT_FALSE(permission.isConnctProxyAllowed());
    EXPECT_FALSE(permission.isConnctAdminAllowed());
    EXPECT_FALSE(permission.isConnctClusterNodeAllowed());

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://bmq-test.uri-1/q123");
        testCases.emplace_back("bmq://BMQ-TEST.URI-1/q123");
        testCases.emplace_back("bmq://bmq-test.uri-1.~pd/q123");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isQueueReadAllowed(*it));
            EXPECT_TRUE(permission.isQueueWriteAllowed(*it));
        }
    }

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("HELP");
        testCases.emplace_back("DOMAINS DOMAIN foo");
        testCases.emplace_back("CONFIGPROVIDER CACHE_CLEAR ALL");
        testCases.emplace_back("CLUSTERS LIST");
        testCases.emplace_back("BROKERCONFIG DUMP");
        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isExecuteAdminCommandAllowed(*it));
        }
    }
}

TEST_F(TestPolicy_Permission, adminClient)
{
    bsl::allocator<> alloc = bmqtst::TestHelperUtil::allocator();
    bsl::string_view roleJson =
        "{"
        "   \"id\": { \"name\": \"testUser\" },"
        "   \"permissions\": ["
        "       {"
        "           \"action\": \"connectAdmin\""
        "       },"
        "       {"
        "           \"action\": \"executeAdminCommand\""
        "       }"
        "   ]"
        "}";
    mqbpoly::Role role(alloc.mechanism());
    ASSERT_EQ(0, decodeRoleJson(&role, roleJson));

    mqbauthz::Policy_Permission permission;
    ASSERT_EQ(0, mqbauthz::Policy_Permission::parse(&permission, role, alloc));

    EXPECT_FALSE(permission.isConnectClientAllowed());
    EXPECT_FALSE(permission.isConnctProxyAllowed());
    EXPECT_TRUE(permission.isConnctAdminAllowed());
    EXPECT_FALSE(permission.isConnctClusterNodeAllowed());

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://bmq-test.uri-1/q123");
        testCases.emplace_back("bmq://BMQ-TEST.URI-1/q123");
        testCases.emplace_back("bmq://bmq-test.uri-1.~pd/q123");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isQueueReadAllowed(*it));
            EXPECT_FALSE(permission.isQueueWriteAllowed(*it));
        }
    }

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("HELP");
        testCases.emplace_back("DOMAINS DOMAIN foo");
        testCases.emplace_back("CONFIGPROVIDER CACHE_CLEAR ALL");
        testCases.emplace_back("CLUSTERS LIST");
        testCases.emplace_back("BROKERCONFIG DUMP");
        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_TRUE(permission.isExecuteAdminCommandAllowed(*it));
        }
    }
}

TEST_F(TestPolicy_Permission, proxyBroker)
{
    bsl::allocator<> alloc    = bmqtst::TestHelperUtil::allocator();
    bsl::string_view roleJson = "{"
                                "   \"id\": { \"name\": \"testUser\" },"
                                "   \"permissions\": ["
                                "       {"
                                "           \"action\": \"connectProxy\""
                                "       },"
                                "       {"
                                "           \"action\": \"queueRead\","
                                "           \"resources\": ["
                                "               { \"id\": \"*\" }"
                                "           ]"
                                "       },"
                                "       {"
                                "           \"action\": \"queueWrite\","
                                "           \"resources\": ["
                                "               { \"id\": \"*\" }"
                                "           ]"
                                "       }"
                                "   ]"
                                "}";
    mqbpoly::Role    role(alloc.mechanism());
    ASSERT_EQ(0, decodeRoleJson(&role, roleJson));

    mqbauthz::Policy_Permission permission;
    ASSERT_EQ(0, mqbauthz::Policy_Permission::parse(&permission, role, alloc));

    EXPECT_FALSE(permission.isConnectClientAllowed());
    EXPECT_TRUE(permission.isConnctProxyAllowed());
    EXPECT_FALSE(permission.isConnctAdminAllowed());
    EXPECT_FALSE(permission.isConnctClusterNodeAllowed());

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://bmq-test.uri-1/q123");
        testCases.emplace_back("bmq://BMQ-TEST.URI-1/q123");
        testCases.emplace_back("bmq://bmq-test.uri-1.~pd/q123");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_TRUE(permission.isQueueReadAllowed(*it));
            EXPECT_TRUE(permission.isQueueWriteAllowed(*it));
        }
    }

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("HELP");
        testCases.emplace_back("DOMAINS DOMAIN foo");
        testCases.emplace_back("CONFIGPROVIDER CACHE_CLEAR ALL");
        testCases.emplace_back("CLUSTERS LIST");
        testCases.emplace_back("BROKERCONFIG DUMP");
        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isExecuteAdminCommandAllowed(*it));
        }
    }
}

TEST_F(TestPolicy_Permission, clusterNode)
{
    bsl::allocator<> alloc    = bmqtst::TestHelperUtil::allocator();
    bsl::string_view roleJson = "{"
                                "   \"id\": { \"name\": \"testUser\" },"
                                "   \"permissions\": ["
                                "       {"
                                "           \"action\": \"connectClusterNode\""
                                "       },"
                                "       {"
                                "           \"action\": \"queueRead\","
                                "           \"resources\": ["
                                "               { \"id\": \"*\" }"
                                "           ]"
                                "       },"
                                "       {"
                                "           \"action\": \"queueWrite\","
                                "           \"resources\": ["
                                "               { \"id\": \"*\" }"
                                "           ]"
                                "       }"
                                "   ]"
                                "}";
    mqbpoly::Role    role(alloc.mechanism());
    ASSERT_EQ(0, decodeRoleJson(&role, roleJson));

    mqbauthz::Policy_Permission permission;
    ASSERT_EQ(0, mqbauthz::Policy_Permission::parse(&permission, role, alloc));

    EXPECT_FALSE(permission.isConnectClientAllowed());
    EXPECT_FALSE(permission.isConnctProxyAllowed());
    EXPECT_FALSE(permission.isConnctAdminAllowed());
    EXPECT_TRUE(permission.isConnctClusterNodeAllowed());

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://bmq-test.uri-1/q123");
        testCases.emplace_back("bmq://BMQ-TEST.URI-1/q123");
        testCases.emplace_back("bmq://bmq-test.uri-1.~pd/q123");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_TRUE(permission.isQueueReadAllowed(*it));
            EXPECT_TRUE(permission.isQueueWriteAllowed(*it));
        }
    }

    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("HELP");
        testCases.emplace_back("DOMAINS DOMAIN foo");
        testCases.emplace_back("CONFIGPROVIDER CACHE_CLEAR ALL");
        testCases.emplace_back("CLUSTERS LIST");
        testCases.emplace_back("BROKERCONFIG DUMP");
        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isExecuteAdminCommandAllowed(*it));
        }
    }
}

TEST_F(TestPolicy_Permission, queueAccessOnDomain)
{
    bsl::allocator<> alloc = bmqtst::TestHelperUtil::allocator();
    bsl::string_view roleJson =
        "{"
        "   \"id\": { \"name\": \"testUser\" },"
        "   \"permissions\": ["
        "       {"
        "           \"action\": \"queueRead\","
        "           \"resources\": ["
        "               { \"id\": \"bmq-test-domain\" }"
        "           ]"
        "       },"
        "       {"
        "           \"action\": \"queueWrite\","
        "           \"resources\": ["
        "               { \"id\": \"bmq-test-domain\" }"
        "           ]"
        "       }"
        "   ]"
        "}";
    mqbpoly::Role role(alloc.mechanism());
    ASSERT_EQ(0, decodeRoleJson(&role, roleJson));

    mqbauthz::Policy_Permission permission;
    ASSERT_EQ(0, mqbauthz::Policy_Permission::parse(&permission, role, alloc));

    // Positive cases
    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://bmq-test-domain/q123");
        testCases.emplace_back("bmq://bmq-test-domain/q456");
        testCases.emplace_back("bmq://bmq-test-domain/q123?id=foo");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_TRUE(permission.isQueueReadAllowed(*it));
            EXPECT_TRUE(permission.isQueueWriteAllowed(*it));
        }
    }

    // Negative cases
    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://BMQ-TEST-DOMAIN/q123");
        testCases.emplace_back("bmq://bmq-test-domain-1/q123");
        testCases.emplace_back("bmq://b/q");
        testCases.emplace_back("");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isQueueReadAllowed(*it));
            EXPECT_FALSE(permission.isQueueWriteAllowed(*it));
        }
    }
}

TEST_F(TestPolicy_Permission, queueAccessOnQueue)
{
    bsl::allocator<> alloc = bmqtst::TestHelperUtil::allocator();
    bsl::string_view roleJson =
        "{"
        "   \"id\": { \"name\": \"testUser\" },"
        "   \"permissions\": ["
        "       {"
        "           \"action\": \"queueRead\","
        "           \"resources\": ["
        "               { \"id\": \"bmq-test-domain/test-queue\" }"
        "           ]"
        "       },"
        "       {"
        "           \"action\": \"queueWrite\","
        "           \"resources\": ["
        "               { \"id\": \"bmq-test-domain/test-queue\" }"
        "           ]"
        "       }"
        "   ]"
        "}";
    mqbpoly::Role role(alloc.mechanism());
    ASSERT_EQ(0, decodeRoleJson(&role, roleJson));

    mqbauthz::Policy_Permission permission;
    ASSERT_EQ(0, mqbauthz::Policy_Permission::parse(&permission, role, alloc));

    // Positive cases
    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://bmq-test-domain/test-queue");
        testCases.emplace_back("bmq://bmq-test-domain/test-queue?id=foo");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_TRUE(permission.isQueueReadAllowed(*it));
            EXPECT_TRUE(permission.isQueueWriteAllowed(*it));
        }
    }

    // Negative cases
    {
        bsl::vector<bsl::string_view> testCases(alloc);
        testCases.emplace_back("bmq://bmq-test-domain/test-queue-1");
        testCases.emplace_back("bmq://bmq-test-domain/TEST-QUEUE");
        testCases.emplace_back("bmq://bmq-test-domain-2/test-queue");
        testCases.emplace_back("");

        for (bsl::vector<bsl::string_view>::const_iterator
                 it  = testCases.cbegin(),
                 end = testCases.cend();
             it != end;
             ++it) {
            EXPECT_FALSE(permission.isQueueReadAllowed(*it));
            EXPECT_FALSE(permission.isQueueWriteAllowed(*it));
        }
    }
}

TEST_F(TestPolicy_Permission, duplicateActionsFail)
{
    bsl::allocator<> alloc    = bmqtst::TestHelperUtil::allocator();
    bsl::string_view roleJson = "{"
                                "   \"id\": { \"name\": \"testUser\" },"
                                "   \"permissions\": ["
                                "       {"
                                "           \"action\": \"connectClient\""
                                "       },"
                                "       {"
                                "           \"action\": \"connectClient\""
                                "       }"
                                "   ]"
                                "}";
    mqbpoly::Role    role(alloc.mechanism());
    ASSERT_EQ(0, decodeRoleJson(&role, roleJson));

    mqbauthz::Policy_Permission permission;
    EXPECT_NE(0, mqbauthz::Policy_Permission::parse(&permission, role, alloc));
}

TEST_F(TestPolicy_Permission, invalidActionNameFails)
{
    bsl::allocator<> alloc = bmqtst::TestHelperUtil::allocator();
    bsl::string_view roleJson =
        "{"
        "   \"id\": { \"name\": \"testUser\" },"
        "   \"permissions\": ["
        "       {"
        "           \"action\": \"immanentizeEschaton\""
        "       }"
        "   ]"
        "}";
    mqbpoly::Role role(alloc.mechanism());
    ASSERT_EQ(0, decodeRoleJson(&role, roleJson));

    mqbauthz::Policy_Permission permission;
    EXPECT_NE(0, mqbauthz::Policy_Permission::parse(&permission, role, alloc));
}

class TestPolicy : public BaseTest {
  protected:
    bsl::allocator<> get_allocator() const
    {
        return bmqtst::TestHelperUtil::allocator();
    }

    int decodePolicyJson(mqbpoly::Policy* policy, bsl::string_view policyJson)
    {
        bsl::allocator<>       alloc = get_allocator();
        bsl::istringstream     policyStr(policyJson);
        baljsn::Decoder        decoder(alloc.mechanism());
        baljsn::DecoderOptions decoderOptions;
        decoderOptions.setSkipUnknownElements(false);
        return decoder.decode(policyStr, policy, decoderOptions);
    }
};

TEST_F(TestPolicy, doctestPolicyDefinition)
{
    bsl::allocator<> alloc = bmqtst::TestHelperUtil::allocator();
    bsl::string_view policyJson =
        "{"
        "    \"roles\": ["
        "        {"
        "            \"id\": \"anonymous\","
        "            \"permissions\": ["
        "                {"
        "                    \"action\": \"connectClient\","
        "                },"
        "                {"
        "                    \"action\": \"queueRead\","
        "                    \"resources\": ["
        "                        {"
        "                            \"id\": \"*\","
        "                        }"
        "                    ]"
        "                },"
        "                {"
        "                    \"action\": \"queueWrite\","
        "                    \"resources\": ["
        "                        {"
        "                            \"id\": \"*\","
        "                        }"
        "                    ]"
        "                }"
        "            ]"
        "        }"
        "    ]"
        "}";
    mqbpoly::Policy policy(alloc.mechanism());
    ASSERT_EQ(0, decodePolicyJson(&policy, policyJson));

    mqbauthz::Policy policyDef;
    EXPECT_NE(0, mqbauthz::Policy::parse(&policyDef, policy, alloc));
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
