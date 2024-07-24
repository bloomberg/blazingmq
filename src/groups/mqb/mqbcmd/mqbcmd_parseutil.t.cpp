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

// mqbcmd_parseutil.t.cpp                                             -*-C++-*-
#include <mqbcmd_parseutil.h>

// MQB
#include <mqbcmd_messages.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <bdlb_arrayutil.h>
#include <bdlsb_fixedmeminstreambuf.h>
#include <bsl_cstddef.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsls_assert.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace {

// ============================================================================
//                                 HELPERS
// ----------------------------------------------------------------------------

struct Test {
    int         d_sourceLine;    // where this 'Test' is defined
    const char* d_description;   // what is being tested
    const char* d_input;         // input to the parser
    const char* d_expectedJson;  // JSON of expected output, or null if
                                 // failure is expected.
};

mqbcmd::Command fromJson(const bslstl::StringRef& json)
{
    mqbcmd::Command            result;
    bdlsb::FixedMemInStreamBuf input(json.data(), json.size());
    baljsn::DecoderOptions     options;

    options.setSkipUnknownElements(false);

    int rc = baljsn::Decoder().decode(&input, &result, options);

    // Failure to decode from the hard-coded JSON is a test _design_ error.
    BSLS_ASSERT_OPT(rc == 0);

    return result;
}

void verifyExpected(const Test& test)
{
    mwctst::TestHelper::printTestName(test.d_description);

    BSLS_ASSERT_OPT(test.d_expectedJson);

    const mqbcmd::Command expected = fromJson(test.d_expectedJson);
    mqbcmd::Command       actual;
    bsl::string           error;
    const int rc = mqbcmd::ParseUtil::parse(&actual, &error, test.d_input);

    ASSERT_EQ_D(test.d_sourceLine << ": " << test.d_input << ": " << error,
                rc,
                0);
    ASSERT_EQ_D(test.d_sourceLine << ": " << test.d_input, actual, expected);
}

void verifyFailure(const Test& test)
{
    mwctst::TestHelper::printTestName(test.d_description);

    BSLS_ASSERT_OPT(test.d_expectedJson == 0);

    mqbcmd::Command actual;
    bsl::string     error;
    const int rc = mqbcmd::ParseUtil::parse(&actual, &error, test.d_input);

    ASSERT_NE_D(test.d_sourceLine << ": parsing should fail", rc, 0);
}

void verifyFailsWithExtra(const Test& test)
{
    BSLS_ASSERT_OPT(test.d_expectedJson);

    const bsl::string description = test.d_description +
                                    bslstl::StringRef(
                                        "; fails with extra input");
    const bsl::string input = test.d_input +
                              bslstl::StringRef(" extra extra extra");
    const char* const expected     = 0;  // now we expect it to fail
    const Test        extendedTest = {test.d_sourceLine,
                                      description.c_str(),
                                      input.c_str(),
                                      expected};

    verifyFailure(extendedTest);
}

// ============================================================================
//                                  TESTS
// ----------------------------------------------------------------------------

const Test k_TESTS[] = {
    {__LINE__, "HELP command", "HELP", "{\"help\": {\"plumbing\": false}}"},
    {__LINE__,
     "HELP PLUMBING command",
     "HELP PLUMBING",
     "{\"help\": {\"plumbing\": true}}"},
    {__LINE__,
     "DOMAINS DOMAIN command requires a domain name",
     "DOMAINS DOMAIN",
     0},
    {__LINE__,
     "domain-specific queue purge",
     "DOMAINS DOMAIN foo PURGE",
     "{\"domains\": {\"domain\": {\"name\": \"foo\", \"command\": {\"purge"
     "\": {}}}}}"},
    {__LINE__,
     "domain-specific information",
     "DOMAINS DOMAIN foo INFOS",
     "{\"domains\": {\"domain\": {\"name\": \"foo\", \"command\": {\"info\""
     ": {}}}}}"},
    {__LINE__,
     "DOMAINS DOMAIN <name> QUEUE command requires queue name",
     "DOMAINS DOMAIN foo QUEUE",
     0},
    {__LINE__,
     "DOMAINS DOMAIN <name> QUEUE <queue> requires subcommand",
     "DOMAINS DOMAIN foo QUEUE bar",
     0},
    {__LINE__,
     "DOMAINS DOMAIN <name> QUEUE <queue> PURGE requires appId",
     "DOMAINS DOMAIN foo QUEUE bar PURGE",
     0},
    {__LINE__,
     "purge an appId from a queue",
     "DOMAINS DOMAIN foo QUEUE bar PURGE epson",
     "{\"domains\": {\"domain\": {\"name\": \"foo\", \"command\": {\"queue"
     "\": {\"name\": \"bar\", \"command\": {\"purgeAppId\": \"epson\"}}}}}}"},
    {__LINE__,
     "purge everything from a queue",
     "DOMAINS DOMAIN foo QUEUE bar PURGE *",
     "{\"domains\": {\"domain\": {\"name\": \"foo\", \"command\": {\"queue"
     "\": {\"name\": \"bar\", \"command\": {\"purgeAppId\": \"*\"}}}}}}"},
    {__LINE__,
     "get queue internals",
     "DOMAINS DOMAIN foo QUEUE bar INTERNALS",
     "{\"domains\": {\"domain\": {\"name\": \"foo\", \"command\": {\"queue"
     "\": {\"name\": \"bar\", \"command\": {\"internals\": {}}}}}}}"},
    {__LINE__,
     "DOMAINS DOMAIN ... LIST accepts optional <appId>",
     "DOMAINS DOMAIN foo QUEUE bar LIST baz -30 10",
     "{\"domains\": {\"domain\": {\"name\": \"foo\", \"command\": {\"queue"
     "\": {\"name\": \"bar\", \"command\": {\"messages\": {\"appId\": \"baz"
     "\", \"offset\": -30, \"count\": 10}}}}}}}"},
    {__LINE__,
     "in DOMAINS DOMAIN ... LIST ... <appId> is optional",
     "DOMAINS DOMAIN foo QUEUE bar LIST -30 10",
     "{\"domains\": {\"domain\": {\"name\": \"foo\", \"command\": {\"queue"
     "\": {\"name\": \"bar\", \"command\": {\"messages\": {\"offset\": -30,"
     " \"count\": 10}}}}}}}"},
    {__LINE__,
     "in DOMAINS DOMAIN ... LIST ... <offset> <count> required",
     "DOMAINS DOMAIN foo QUEUE bar LIST -30",
     0},
    {__LINE__,
     "clear the domain resolver cache for a domain",
     "DOMAINS RESOLVER CACHE_CLEAR foo",
     "{\"domains\": {\"resolver\": {\"clearCache\": {\"domain\": \"foo\"}}}"
     "}"},
    {__LINE__,
     "clear the domain resolver cache for all domains",
     "DOMAINS RESOLVER CACHE_CLEAR ALL",
     "{\"domains\": {\"resolver\": {\"clearCache\": {\"all\": {}}}}}"},
    {__LINE__,
     "clear domain resolver cache requires a domain or \"all\"",
     "DOMAINS RESOLVER CACHE_CLEAR",
     0},
    {__LINE__,
     "clear the configuration provider cache for a domain",
     "CONFIGPROVIDER CACHE_CLEAR foo",
     "{\"configProvider\": {\"clearCache\": {\"domain\": \"foo\"}}}"},
    {__LINE__,
     "clear the configuration provider cache for all domains",
     "CONFIGPROVIDER CACHE_CLEAR ALL",
     "{\"configProvider\": {\"clearCache\": {\"all\": {}}}}"},
    {__LINE__,
     "clear config provider cache requires a domain or \"all\"",
     "CONFIGPROVIDER CACHE_CLEAR",
     0},
    {__LINE__, "show statistics", "STAT SHOW", "{\"stat\": {\"show\": {}}}"},
    {__LINE__,
     "list all active clusters",
     "CLUSTERS LIST",
     "{\"clusters\": {\"list\": {}}}"},
    {__LINE__,
     "establish a reverse connection about a cluster",
     "CLUSTERS ADDREVERSE cloister tcp://123.456.789.012",
     "{\"clusters\": {\"addReverseProxy\": {\"clusterName\": \"cloister\", "
     "\"remotePeer\": \"tcp://123.456.789.012\"}}}"},
    {__LINE__,
     "reverse connection must be about a cluster",
     "CLUSTERS ADDREVERSE",
     0},
    {__LINE__,
     "reverse connection must be to a peer",
     "CLUSTERS ADDREVERSE cloister",
     0},
    {__LINE__,
     "cluster-related command requires a subcommand",
     "CLUSTERS CLUSTER",
     0},
    {__LINE__,
     "get the status of a cluster",
     "CLUSTERS CLUSTER cloister STATUS",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"status\": {}}}}}"},
    {__LINE__,
     "show internal state of cluster's queue helper",
     "CLUSTERS CLUSTER cloister QUEUEHELPER",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"queueHelper\": {}}}}}"},
    {__LINE__,
     "run garbage collection on a cluster's queues",
     "CLUSTERS CLUSTER cloister FORCE_GC_QUEUES",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"forceGcQueues\": {}}}}}"},
    {__LINE__,
     "cluster storage command requires a subcommand",
     "CLUSTERS CLUSTER cloister STORAGE",
     0},
    {__LINE__,
     "show storage summary for a cluster",
     "CLUSTERS CLUSTER cloister STORAGE SUMMARY",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"storage\": { \"summary\": {}}}}}}"},
    {__LINE__,
     "cluster storage partition command requires partitionId",
     "CLUSTERS CLUSTER cloister STORAGE PARTITION",
     0},
    {__LINE__,
     "cluster storage partitionId is an integer",
     "CLUSTERS CLUSTER cloister STORAGE PARTITION party",
     0},
    {__LINE__,
     "cluster storage partition command requires subcommand",
     "CLUSTERS CLUSTER cloister STORAGE PARTITION 123",
     0},
    {__LINE__,
     "enable a partition on a cluster",
     "CLUSTERS CLUSTER cloister STORAGE PARTITION 123 ENABLE",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"storage\": { \"partition\": {\"partitionId\": 123, \"command\": "
     "{\"enable\": {}}}}}}}}"},
    {__LINE__,
     "disable a partition on a cluster",
     "CLUSTERS CLUSTER cloister STORAGE PARTITION 123 DISABLE",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"storage\": { \"partition\": {\"partitionId\": 123, \"command\": "
     "{\"disable\": {}}}}}}}}"},
    {__LINE__,
     "summarize a partition on a cluster",
     "CLUSTERS CLUSTER cloister STORAGE PARTITION 123 SUMMARY",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"storage\": { \"partition\": {\"partitionId\": 123, \"command\": "
     "{\"summary\": {}}}}}}}}"},
    {__LINE__,
     "cluster storage domain command requires domain name",
     "CLUSTERS CLUSTER cloister STORAGE DOMAIN",
     0},
    {__LINE__,
     "cluster storage domain command requires subcommand",
     "CLUSTERS CLUSTER cloister STORAGE DOMAIN domicile",
     0},
    {__LINE__,
     "show status of queues in a domain in a cluster",
     "CLUSTERS CLUSTER cloister STORAGE DOMAIN domicile QUEUE_STATUS",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"storage\": { \"domain\": {\"name\": \"domicile\", \"command\": "
     "{\"queueStatus\": {}}}}}}}}"},

    {__LINE__,
     "cluster state command requires a subcommand",
     "CLUSTERS CLUSTER cloister STATE",
     0},
    {__LINE__,
     "cluster elector state command requires a subcommand",
     "CLUSTERS CLUSTER cloister STATE ELECTOR",
     0},
    {__LINE__,
     "set a tunable on a cluster's elector state",
     "CLUSTERS CLUSTER cloister STATE ELECTOR SET parameter 478",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"state\": { \"elector\": {\"setTunable\": {\"name\": \"parameter\", "
     "\"value\": {\"theInteger\": 478}, \"self\": {}}}}}}}}"},
    {__LINE__,
     "get a tunable on a cluster's elector state",
     "CLUSTERS CLUSTER cloister STATE ELECTOR GET parameter",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"state\": { \"elector\": {\"getTunable\": {\"name\": \"parameter\", "
     "\"self\": {}}}}}}}}"},
    {__LINE__,
     "list supported tunables on a cluster's elector state",
     "CLUSTERS CLUSTER cloister STATE ELECTOR LIST_TUNABLES",
     "{\"clusters\": {\"cluster\": {\"name\": \"cloister\", \"command\": {"
     "\"state\": { \"elector\": {\"listTunables\": {}}}}}}}"},
    {__LINE__, "the danger command requires a subcommand", "DANGER", 0},
    {__LINE__,
     "perform clean shutdown of a broker instance",
     "DANGER SHUTDOWN",
     "{\"danger\": {\"shutdown\": {}}}"},
    {__LINE__,
     "terminate a broker instance",
     "DANGER TERMINATE",
     "{\"danger\": {\"terminate\": {}}}"},
    {__LINE__,
     "Broker Config command",
     "BROKERCONFIG DUMP",
     "{\"brokerConfig\": {\"dump\": {}}}"}};

void test1_parseExpected()
{
    for (const Test* it = k_TESTS; it != bdlb::ArrayUtil::end(k_TESTS); ++it) {
        const Test& test = *it;
        if (test.d_expectedJson) {
            verifyExpected(test);
        }
    }
}

void test2_parseFailure()
{
    for (const Test* it = k_TESTS; it != bdlb::ArrayUtil::end(k_TESTS); ++it) {
        const Test& test = *it;
        if (!test.d_expectedJson) {
            verifyFailure(test);
        }
    }
}

void test3_parseFailsWithExtra()
{
    for (const Test* it = k_TESTS; it != bdlb::ArrayUtil::end(k_TESTS); ++it) {
        const Test& test = *it;
        if (test.d_expectedJson) {
            verifyFailsWithExtra(test);
        }
    }
}

}  // close unnamed namespace

// ============================================================================
//                                  MAIN
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_parseExpected(); break;
    case 2: test2_parseFailure(); break;
    case 3: test3_parseFailsWithExtra(); break;
    default:
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND.\n";
        s_testStatus = -1;
    }

    TEST_EPILOG(mwctst::TestHelper::e_DEFAULT);
}
