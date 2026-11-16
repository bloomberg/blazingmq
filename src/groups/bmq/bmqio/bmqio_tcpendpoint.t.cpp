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

#include <bmqio_tcpendpoint.h>

#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bsl_string.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Verify that the specified `obj` is in the invalid, empty state: it
/// converts to `false`, and its `uri()`, `host()` and `port()` are all empty
/// or zero.  Use the specified `line` to identify the failing test row.
void verifyInvalid(int line, const bmqio::TCPEndpoint& obj)
{
    BMQTST_ASSERT_D(line, !obj);
    BMQTST_ASSERT_D(line, obj.uri().empty());
    BMQTST_ASSERT_EQ_D(line, obj.host(), "");
    BMQTST_ASSERT_EQ_D(line, obj.port(), 0);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

/// Exercise the basic functionality of the component: default construction,
/// construction from a valid URI, and construction from a host and port.
static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("Default constructor");

        bmqio::TCPEndpoint obj(bmqtst::TestHelperUtil::allocator());
        verifyInvalid(L_, obj);
    }

    {
        PV("Constructor from a valid URI");

        const bsl::string  uri("tcp://localhost:30114",
                              bmqtst::TestHelperUtil::allocator());
        bmqio::TCPEndpoint obj(uri, bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT(obj);
        BMQTST_ASSERT_EQ(obj.uri(), uri);
        BMQTST_ASSERT_EQ(obj.host(), "localhost");
        BMQTST_ASSERT_EQ(obj.port(), 30114);
    }

    {
        PV("Constructor from a host and a port");

        bmqio::TCPEndpoint obj("localhost",
                               30114,
                               bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT(obj);
        BMQTST_ASSERT_EQ(obj.uri(), "tcp://localhost:30114");
        BMQTST_ASSERT_EQ(obj.host(), "localhost");
        BMQTST_ASSERT_EQ(obj.port(), 30114);
    }
}

/// Verify that `fromUri` accepts well-formed URIs and splits them into the
/// expected host and port, keeping the original URI.
static void test2_validUri()
{
    bmqtst::TestHelper::printTestName("VALID URI");

    const struct {
        int         d_line;
        const char* d_uri;
        const char* d_host;
        int         d_port;
    } k_DATA[] = {
        // uri                                 host                 port
        // ----------------------------------- -------------------- -----
        {L_, "tcp://localhost:30114", "localhost", 30114},
        {L_, "tcp://127.0.0.1:8080", "127.0.0.1", 8080},
        {L_, "tcp://my-host.example.com:1234", "my-host.example.com", 1234},
        {L_, "tcp://h:9", "h", 9},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const bsl::string uri(k_DATA[idx].d_uri,
                              bmqtst::TestHelperUtil::allocator());

        PVV(k_DATA[idx].d_line << ": '" << uri << "'");

        bmqio::TCPEndpoint obj(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_D(k_DATA[idx].d_line, obj.fromUri(uri));
        BMQTST_ASSERT_D(k_DATA[idx].d_line, obj);
        BMQTST_ASSERT_EQ_D(k_DATA[idx].d_line, obj.uri(), uri);
        BMQTST_ASSERT_EQ_D(k_DATA[idx].d_line, obj.host(), k_DATA[idx].d_host);
        BMQTST_ASSERT_EQ_D(k_DATA[idx].d_line, obj.port(), k_DATA[idx].d_port);
    }
}

/// Verify that `fromUri` rejects malformed URIs, and that a rejected URI
/// leaves the object in the invalid, empty state.
static void test3_invalidUri()
{
    bmqtst::TestHelper::printTestName("INVALID URI");

    const struct {
        int         d_line;
        const char* d_uri;
    } k_DATA[] = {
        // Scheme
        {L_, ""},
        {L_, "localhost:30114"},
        {L_, "http://localhost:30114"},
        {L_, "tcp:/localhost:30114"},

        // Missing or empty port
        {L_, "tcp://localhost"},
        {L_, "tcp://localhost:"},

        // Empty host
        {L_, "tcp://:30114"},

        // Junk in the port
        {L_, "tcp://localhost:301 14"},
        {L_, "tcp://localhost:30114abc"},
        {L_, "tcp://localhost:30114 "},
        {L_, "tcp://localhost: 30114"},
        {L_, "tcp://localhost:+30114"},
        {L_, "tcp://localhost:-30114"},
        {L_, "tcp://localhost:abc"},
        {L_, "tcp://localhost:0x10"},

        // More than one ':' after the scheme
        {L_, "tcp://local:host:30114"},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const bsl::string uri(k_DATA[idx].d_uri,
                              bmqtst::TestHelperUtil::allocator());

        PVV(k_DATA[idx].d_line << ": '" << uri << "'");

        bmqio::TCPEndpoint obj(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_D(k_DATA[idx].d_line, !obj.fromUri(uri));
        verifyInvalid(k_DATA[idx].d_line, obj);
    }
}

/// Verify the lowest and highest accepted port values, and that the values
/// just outside of that range are rejected.
static void test4_portBoundaries()
{
    bmqtst::TestHelper::printTestName("PORT BOUNDARIES");

    const struct {
        int         d_line;
        const char* d_uri;
        bool        d_isValid;
        int         d_port;
    } k_DATA[] = {
        // uri                                         valid  port
        // ------------------------------------------- ----- -----
        {L_, "tcp://localhost:0", false, 0},
        {L_, "tcp://localhost:1", true, 1},
        {L_, "tcp://localhost:65535", true, 65535},
        {L_, "tcp://localhost:65536", false, 0},
        {L_, "tcp://localhost:99999999999999999999999", false, 0},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const bsl::string uri(k_DATA[idx].d_uri,
                              bmqtst::TestHelperUtil::allocator());

        PVV(k_DATA[idx].d_line << ": '" << uri << "'");

        bmqio::TCPEndpoint obj(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ_D(k_DATA[idx].d_line,
                           obj.fromUri(uri),
                           k_DATA[idx].d_isValid);

        if (k_DATA[idx].d_isValid) {
            BMQTST_ASSERT_D(k_DATA[idx].d_line, obj);
            BMQTST_ASSERT_EQ_D(k_DATA[idx].d_line, obj.host(), "localhost");
            BMQTST_ASSERT_EQ_D(k_DATA[idx].d_line,
                               obj.port(),
                               k_DATA[idx].d_port);
        }
        else {
            verifyInvalid(k_DATA[idx].d_line, obj);
        }
    }
}

/// Verify that a failed `fromUri` call on an object holding a valid endpoint
/// does not leave the previous host and port behind.
static void test5_failedParseResetsState()
{
    bmqtst::TestHelper::printTestName("FAILED PARSE RESETS STATE");

    // Each of these fails at a different step of the parsing.
    const struct {
        int         d_line;
        const char* d_uri;
    } k_DATA[] = {
        {L_, "udp://other-host:4321"},
        {L_, "tcp://other-host"},
        {L_, "tcp://other-host:99999"},
        {L_, "tcp://:4321"},
        {L_, "tcp://other-host:43 21"},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    const bsl::string goodUri("tcp://good-host:1234",
                              bmqtst::TestHelperUtil::allocator());

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const bsl::string badUri(k_DATA[idx].d_uri,
                                 bmqtst::TestHelperUtil::allocator());

        PVV(k_DATA[idx].d_line << ": '" << badUri << "'");

        bmqio::TCPEndpoint obj(goodUri, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_D(k_DATA[idx].d_line, obj);
        BMQTST_ASSERT_EQ_D(k_DATA[idx].d_line, obj.host(), "good-host");
        BMQTST_ASSERT_EQ_D(k_DATA[idx].d_line, obj.port(), 1234);

        BMQTST_ASSERT_D(k_DATA[idx].d_line, !obj.fromUri(badUri));
        verifyInvalid(k_DATA[idx].d_line, obj);
    }
}

/// Verify the output of `print` and `operator<<` for both a valid and an
/// invalid endpoint.
static void test6_print()
{
    bmqtst::TestHelper::printTestName("PRINT");

    bmqu::MemOutStream stream(bmqtst::TestHelperUtil::allocator());

    {
        PV("Valid endpoint");

        bmqio::TCPEndpoint obj("localhost",
                               30114,
                               bmqtst::TestHelperUtil::allocator());

        // 'operator<<' prints on a single line, without a line feed
        stream << obj;
        BMQTST_ASSERT_EQ(stream.str(), "tcp://localhost:30114");
        stream.reset();

        // 'print' with a non-negative 'spacesPerLevel' adds a line feed
        obj.print(stream, 0, 4);
        BMQTST_ASSERT_EQ(stream.str(), "tcp://localhost:30114\n");
        stream.reset();

        // 'level' indents by 'level * spacesPerLevel' spaces
        obj.print(stream, 1, 2);
        BMQTST_ASSERT_EQ(stream.str(), "  tcp://localhost:30114\n");
        stream.reset();
    }

    {
        PV("Invalid endpoint");

        bmqio::TCPEndpoint obj(bmqtst::TestHelperUtil::allocator());

        stream << obj;
        BMQTST_ASSERT_EQ(stream.str(), "** invalid endpoint **");
        stream.reset();

        obj.print(stream, 0, 4);
        BMQTST_ASSERT_EQ(stream.str(), "** invalid endpoint **\n");
        stream.reset();
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 6: test6_print(); break;
    case 5: test5_failedParseResetsState(); break;
    case 4: test4_portBoundaries(); break;
    case 3: test3_invalidUri(); break;
    case 2: test2_validUri(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    // 'TCPEndpoint::assign' (used by the '(host, port)' constructor) builds
    // the URI using the default allocator, so we can check here only for
    // global allocations.
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
