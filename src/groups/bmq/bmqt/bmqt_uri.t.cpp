// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_uri.t.cpp                                                     -*-C++-*-
#include <bmqt_uri.h>

#include <bmqtst_scopedlogobserver.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslh_defaulthashalgorithm.h>
#include <bslh_hash.h>
#include <bslmt_barrier.h>
#include <bslmt_latch.h>
#include <bslmt_threadgroup.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BENCHMARKING LIBRARY
#ifdef BMQTST_BENCHMARK_ENABLED
#include <benchmark/benchmark.h>
#endif

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Testing:
//   Exercise a broad cross-section of functionality before beginning
//   testing in earnest.  Probe that functionality systematically and
//   incrementally to discover basic errors in isolation.
//
// Plan:
//   Create a series of good and bad URIs and parse them, and compare with
//   the expected result.  Note that this test merely checks the validity
//   of the regular expression used by 'UriParser'.
//
// Testing:
//   class Uri
//   int Uri::parseUri
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    int         rc = 0;
    bsl::string error(bmqtst::TestHelperUtil::allocator());

    PV("Test constructors");
    {
        bsl::string uriStr("bmq://my.domain/queue-foo-bar",
                           bmqtst::TestHelperUtil::allocator());
        bmqt::Uri   obj1(uriStr, bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(obj1.isValid(), true);

        const bslstl::StringRef& uriStrRef = uriStr;
        bmqt::Uri obj2(uriStrRef, bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(obj2.isValid(), true);
        BMQTST_ASSERT_EQ(obj1, obj2);

        const char* uriRaw = uriStr.data();
        bmqt::Uri   obj3(uriRaw, bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(obj3.isValid(), true);
        BMQTST_ASSERT_EQ(obj2, obj3);

        bmqt::Uri obj4(obj3, bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(obj4.isValid(), true);
        BMQTST_ASSERT_EQ(obj3, obj4);
    }

    PV("Test basic parsing");
    {
        bmqt::Uri obj(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.isValid(), false);
        {
            // Scope input to ensure it gets deleted after (and so validate the
            // string ref are correctly pointing to the right values).
            bsl::string uri("bmq://my.domain/queue-foo-bar",
                            bmqtst::TestHelperUtil::allocator());
            rc = bmqt::UriParser::parse(&obj, &error, uri);
            BMQTST_ASSERT_EQ(rc, 0);
            BMQTST_ASSERT_EQ(error, "");
            BMQTST_ASSERT_EQ(obj.asString(), uri);
            uri = "deadbeef";  // put 'garbage' in input
        }

        BMQTST_ASSERT_EQ(obj.isValid(), true);
        BMQTST_ASSERT_EQ(obj.scheme(), "bmq");
        BMQTST_ASSERT_EQ(obj.authority(), "my.domain");
        BMQTST_ASSERT_EQ(obj.path(), "queue-foo-bar");
        BMQTST_ASSERT_EQ(obj.domain(), "my.domain");
        BMQTST_ASSERT_EQ(obj.queue(), "queue-foo-bar");
        BMQTST_ASSERT_EQ(obj.id(), "");
        BMQTST_ASSERT_EQ(obj.canonical(), "bmq://my.domain/queue-foo-bar");
    }

    PV("Test assignment operator")
    {
        const char k_URI[] = "bmq://my.domain.~tst/queue";
        bmqt::Uri  objCopy(bmqtst::TestHelperUtil::allocator());

        {
            // Scope the original URI, so that it will be destroyed before we
            // check the copy.
            bmqt::Uri objOriginal(bmqtst::TestHelperUtil::allocator());
            rc = bmqt::UriParser::parse(&objOriginal, &error, k_URI);
            BMQTST_ASSERT_EQ(rc, 0);
            BMQTST_ASSERT_EQ(error, "");
            objCopy = objOriginal;
        }

        BMQTST_ASSERT_EQ(objCopy.isValid(), true);
        BMQTST_ASSERT_EQ(objCopy.asString(), k_URI);
        BMQTST_ASSERT_EQ(objCopy.scheme(), "bmq");
        BMQTST_ASSERT_EQ(objCopy.authority(), "my.domain.~tst");
        BMQTST_ASSERT_EQ(objCopy.domain(), "my.domain");
        BMQTST_ASSERT_EQ(objCopy.tier(), "tst");
        BMQTST_ASSERT_EQ(objCopy.path(), "queue");
        BMQTST_ASSERT_EQ(objCopy.queue(), "queue");
        BMQTST_ASSERT_EQ(objCopy.id(), "");
        BMQTST_ASSERT_EQ(objCopy.canonical(), "bmq://my.domain.~tst/queue");
    }

    PV("Testing comparison operators")
    {
        const char k_URI[] = "bmq://my.domain/queue";
        bmqt::Uri  obj1(bmqtst::TestHelperUtil::allocator());

        bmqt::UriParser::parse(&obj1, &error, k_URI);

        bmqt::Uri obj2(obj1, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj1, obj2);

        bmqt::Uri obj3(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_NE(obj1, obj3);

        obj3 = obj2;
        BMQTST_ASSERT_EQ(obj3, obj2);
    }

    PV("Testing valid URI parsing")
    {
        {
            const char k_URI[] = "bmq://ts.trades.myapp/my.queue";
            bmqt::Uri  obj(bmqtst::TestHelperUtil::allocator());
            rc = bmqt::UriParser::parse(&obj, &error, k_URI);
            BMQTST_ASSERT_EQ(rc, 0);
            BMQTST_ASSERT_EQ(error, "");
            BMQTST_ASSERT_EQ(obj.isValid(), true);
            BMQTST_ASSERT_EQ(obj.scheme(), "bmq");
            BMQTST_ASSERT_EQ(obj.authority(), "ts.trades.myapp");
            BMQTST_ASSERT_EQ(obj.domain(), "ts.trades.myapp");
            BMQTST_ASSERT_EQ(obj.path(), "my.queue");
            BMQTST_ASSERT_EQ(obj.queue(), "my.queue");
            BMQTST_ASSERT_EQ(obj.id(), "");
            BMQTST_ASSERT_EQ(obj.canonical(),
                             "bmq://ts.trades.myapp/my.queue");
        }
        {
            const char k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";
            bmqt::Uri  obj(bmqtst::TestHelperUtil::allocator());
            rc = bmqt::UriParser::parse(&obj, &error, k_URI);
            BMQTST_ASSERT_EQ(rc, 0);
            BMQTST_ASSERT_EQ(error, "");
            BMQTST_ASSERT_EQ(obj.isValid(), true);
            BMQTST_ASSERT_EQ(obj.scheme(), "bmq");
            BMQTST_ASSERT_EQ(obj.authority(), "ts.trades.myapp");
            BMQTST_ASSERT_EQ(obj.domain(), "ts.trades.myapp");
            BMQTST_ASSERT_EQ(obj.queue(), "my.queue");
            BMQTST_ASSERT_EQ(obj.id(), "my.app");
            BMQTST_ASSERT_EQ(obj.canonical(),
                             "bmq://ts.trades.myapp/my.queue");
        }
        {
            const char k_URI[] = "bmq://ts.trades.myapp.~tst/my.queue";
            bmqt::Uri  obj(bmqtst::TestHelperUtil::allocator());
            rc = bmqt::UriParser::parse(&obj, &error, k_URI);
            BMQTST_ASSERT_EQ(rc, 0);
            BMQTST_ASSERT_EQ(error, "");
            BMQTST_ASSERT_EQ(obj.isValid(), true);
            BMQTST_ASSERT_EQ(obj.scheme(), "bmq");
            BMQTST_ASSERT_EQ(obj.authority(), "ts.trades.myapp.~tst");
            BMQTST_ASSERT_EQ(obj.domain(), "ts.trades.myapp");
            BMQTST_ASSERT_EQ(obj.qualifiedDomain(), "ts.trades.myapp.~tst");
            BMQTST_ASSERT_EQ(obj.tier(), "tst");
            BMQTST_ASSERT_EQ(obj.queue(), "my.queue");
            BMQTST_ASSERT_EQ(obj.canonical(),
                             "bmq://ts.trades.myapp.~tst"
                             "/my.queue");
        }
        {
            const char k_URI[] = "bmq://ts.trades.myapp.~lcl-fooBar/my.queue";
            bmqt::Uri  obj(bmqtst::TestHelperUtil::allocator());
            rc = bmqt::UriParser::parse(&obj, &error, k_URI);
            BMQTST_ASSERT_EQ(rc, 0);
            BMQTST_ASSERT_EQ(error, "");
            BMQTST_ASSERT_EQ(obj.isValid(), true);
            BMQTST_ASSERT_EQ(obj.scheme(), "bmq");
            BMQTST_ASSERT_EQ(obj.authority(), "ts.trades.myapp.~lcl-fooBar");
            BMQTST_ASSERT_EQ(obj.domain(), "ts.trades.myapp");
            BMQTST_ASSERT_EQ(obj.qualifiedDomain(),
                             "ts.trades.myapp.~lcl-fooBar");
            BMQTST_ASSERT_EQ(obj.tier(), "lcl-fooBar");
            BMQTST_ASSERT_EQ(obj.queue(), "my.queue");
            BMQTST_ASSERT_EQ(obj.canonical(),
                             "bmq://ts.trades.myapp"
                             ".~lcl-fooBar/my.queue");
        }
    }

    PV("Testing invalid URI parsing");
    {
        struct Test {
            int         d_line;
            const char* d_input;
            int         d_rc;
        } k_DATA[] = {
            // input                                     rc
            // ----------------------------------------- --
            {L_, "", bmqt::UriParser::UriParseResult::e_INVALID_SCHEME},
            {L_, "foobar", bmqt::UriParser::UriParseResult::e_INVALID_SCHEME},
            {L_, "bb://", bmqt::UriParser::UriParseResult::e_INVALID_SCHEME},
            {L_, "bmq://", bmqt::UriParser::UriParseResult::e_MISSING_DOMAIN},
            {L_, "bmq://a/", bmqt::UriParser::UriParseResult::e_MISSING_QUEUE},
            {L_,
             "bmq://$%@/ts.trades.myapp/queue@sss",
             bmqt::UriParser::UriParseResult::e_UNSUPPORTED_CHAR},
            {L_,
             "bb:///ts.trades.myapp/myqueue",
             bmqt::UriParser::UriParseResult::e_INVALID_SCHEME},
            {L_,
             "bmq://ts.trades.myapp/",
             bmqt::UriParser::UriParseResult::e_MISSING_QUEUE},
            {L_,
             "bmq://ts.trades.myapp/queue?id=",
             bmqt::UriParser::UriParseResult::e_BAD_QUERY},
            {L_,
             "bmq://ts.trades.myapp/queue?bs=a",
             bmqt::UriParser::UriParseResult::e_BAD_QUERY},
            {L_,
             "bmq://ts.trades.myapp/queue?",
             bmqt::UriParser::UriParseResult::e_BAD_QUERY},
            {L_,
             "bmq://ts.trades.myapp/queue?id=",
             bmqt::UriParser::UriParseResult::e_BAD_QUERY},
            {L_,
             "bmq://ts.trades.myapp/queue&id==",
             bmqt::UriParser::UriParseResult::e_UNSUPPORTED_CHAR},
            {L_,
             "bmq://ts.trades.myapp/queue&id=foo",
             bmqt::UriParser::UriParseResult::e_UNSUPPORTED_CHAR},
            {L_,
             "bmq://ts.trades.myapp/queue?id=foo&",
             bmqt::UriParser::UriParseResult::e_UNSUPPORTED_CHAR},
            {L_,
             "bmq://ts.trades.myapp/queue?pid=foo",
             bmqt::UriParser::UriParseResult::e_BAD_QUERY},
            {L_,
             "bmq://ts.trades.myapp.~/queue",
             bmqt::UriParser::UriParseResult::e_EMPTY_TIER},
            {L_,
             "bmq://ts.trades~myapp/queue",
             bmqt::UriParser::UriParseResult::e_UNSUPPORTED_CHAR},
            {L_,
             "bmq://ts.trades.myapp.~a_b/queue",
             bmqt::UriParser::UriParseResult::e_UNSUPPORTED_CHAR},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            bmqt::Uri obj(bmqtst::TestHelperUtil::allocator());
            rc = bmqt::UriParser::parse(&obj, &error, test.d_input);
            BMQTST_ASSERT_EQ_D(test.d_line, rc, test.d_rc);
            BMQTST_ASSERT_EQ_D(test.d_line, obj.isValid(), false);

            // Retest without specifying the optional error strings
            rc = bmqt::UriParser::parse(&obj, 0, test.d_input);
            BMQTST_ASSERT_EQ_D(test.d_line, rc, test.d_rc);
            BMQTST_ASSERT_EQ_D(test.d_line, obj.isValid(), false);
        }
    }

    bmqt::UriParser::shutdown();
}

static void test2_URIBuilder()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    bmqt::UriBuilder builder(bmqtst::TestHelperUtil::allocator());

    {
        bmqt::Uri uri(bmqtst::TestHelperUtil::allocator());
        builder.reset();

        builder.setDomain("si.uics.tester").setQueue("siqueue");
        BMQTST_ASSERT_EQ(builder.uri(&uri, 0), 0);
        BMQTST_ASSERT_EQ(uri.asString(), "bmq://si.uics.tester/siqueue");
        BMQTST_ASSERT_EQ(uri.isValid(), true);
    }

    {
        bmqt::Uri uri(bmqtst::TestHelperUtil::allocator());
        builder.reset();

        builder.setDomain("bmq.tutorial").setQueue("worker").setId("myApp");
        BMQTST_ASSERT_EQ(builder.uri(&uri, 0), 0);
        BMQTST_ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial/worker?id=myApp");
        BMQTST_ASSERT_EQ(uri.authority(), "bmq.tutorial");
        BMQTST_ASSERT_EQ(uri.domain(), "bmq.tutorial");
        BMQTST_ASSERT_EQ(uri.tier(), "");
        BMQTST_ASSERT_EQ(uri.path(), "worker");
        BMQTST_ASSERT_EQ(uri.id(), "myApp");
        BMQTST_ASSERT_EQ(uri.isValid(), true);
    }

    {
        bmqt::Uri uri(bmqtst::TestHelperUtil::allocator());
        builder.reset();

        builder.setDomain("bmq.tutorial").setTier("tst").setQueue("myQueue");
        BMQTST_ASSERT_EQ(builder.uri(&uri, 0), 0);
        BMQTST_ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial.~tst/myQueue");
        BMQTST_ASSERT_EQ(uri.authority(), "bmq.tutorial.~tst");
        BMQTST_ASSERT_EQ(uri.domain(), "bmq.tutorial");
        BMQTST_ASSERT_EQ(uri.tier(), "tst");
        BMQTST_ASSERT_EQ(uri.path(), "myQueue");
        BMQTST_ASSERT_EQ(uri.isValid(), true);
    }

    PV("domain/tier/qualifiedDomain correlation")
    {
        bmqt::Uri uri(bmqtst::TestHelperUtil::allocator());
        builder.reset();

        builder.setQualifiedDomain("bmq.tutorial.~lcl").setQueue("myQueue");
        BMQTST_ASSERT_EQ(builder.uri(&uri, 0), 0);
        BMQTST_ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial.~lcl/myQueue");
        BMQTST_ASSERT_EQ(uri.authority(), "bmq.tutorial.~lcl");
        BMQTST_ASSERT_EQ(uri.domain(), "bmq.tutorial");
        BMQTST_ASSERT_EQ(uri.tier(), "lcl");
        BMQTST_ASSERT_EQ(uri.path(), "myQueue");
        BMQTST_ASSERT_EQ(uri.isValid(), true);

        builder.setTier("tst");
        BMQTST_ASSERT_EQ(builder.uri(&uri, 0), 0);
        BMQTST_ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial.~tst/myQueue");
        BMQTST_ASSERT_EQ(uri.authority(), "bmq.tutorial.~tst");
        BMQTST_ASSERT_EQ(uri.domain(), "bmq.tutorial");
        BMQTST_ASSERT_EQ(uri.tier(), "tst");

        builder.setDomain("bmq.test");
        BMQTST_ASSERT_EQ(builder.uri(&uri, 0), 0);
        BMQTST_ASSERT_EQ(uri.asString(), "bmq://bmq.test.~tst/myQueue");
        BMQTST_ASSERT_EQ(builder.uri(&uri, 0), 0);
        BMQTST_ASSERT_EQ(uri.authority(), "bmq.test.~tst");
        BMQTST_ASSERT_EQ(uri.domain(), "bmq.test");
        BMQTST_ASSERT_EQ(uri.tier(), "tst");

        builder.setQualifiedDomain("bmq.tutorial.~lcl");
        BMQTST_ASSERT_EQ(builder.uri(&uri, 0), 0);
        BMQTST_ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial.~lcl/myQueue");
        BMQTST_ASSERT_EQ(uri.authority(), "bmq.tutorial.~lcl");
        BMQTST_ASSERT_EQ(uri.domain(), "bmq.tutorial");
        BMQTST_ASSERT_EQ(uri.tier(), "lcl");
        BMQTST_ASSERT_EQ(uri.isValid(), true);
    }

    PV("Test error case");
    {
        bmqt::Uri   uri(bmqtst::TestHelperUtil::allocator());
        bsl::string errorMessage(bmqtst::TestHelperUtil::allocator());
        builder.reset();

        BMQTST_ASSERT_EQ(builder.uri(&uri, &errorMessage),
                         bmqt::UriParser::UriParseResult::e_MISSING_DOMAIN);
        BMQTST_ASSERT_EQ(errorMessage, "Missing domain");
        builder.setDomain("my.domain");
        BMQTST_ASSERT_EQ(uri.isValid(), false);

        BMQTST_ASSERT_EQ(builder.uri(&uri, &errorMessage),
                         bmqt::UriParser::UriParseResult::e_MISSING_QUEUE);
        BMQTST_ASSERT_EQ(errorMessage, "Missing queue");
        builder.setQueue("myQueue");
        BMQTST_ASSERT_EQ(builder.uri(&uri, 0), 0);
        BMQTST_ASSERT_EQ(uri.asString(), "bmq://my.domain/myQueue");
        BMQTST_ASSERT_EQ(uri.isValid(), true);
    }

    PV("Test creating a builder by copy of a Uri");
    {
        bmqt::Uri        uri("bmq://my.domain/myQueue",
                      bmqtst::TestHelperUtil::allocator());
        bmqt::Uri        tmpUri(bmqtst::TestHelperUtil::allocator());
        bmqt::UriBuilder uriBuilder(uri, bmqtst::TestHelperUtil::allocator());

        // Validate the uri in builder is the same
        BMQTST_ASSERT_EQ(uriBuilder.uri(&tmpUri, 0), 0);
        BMQTST_ASSERT_EQ(tmpUri.asString(), "bmq://my.domain/myQueue");

        // Update URI in builder
        uriBuilder.setQueue("yourQueue");

        // Ensure original URI is unchanged
        BMQTST_ASSERT_EQ(uri.asString(), "bmq://my.domain/myQueue");

        // Verify the built URI has the change
        BMQTST_ASSERT_EQ(uriBuilder.uri(&tmpUri, 0), 0);
        BMQTST_ASSERT_EQ(tmpUri.asString(), "bmq://my.domain/yourQueue");
    }

    bmqt::UriParser::shutdown();
}

/// Test same `UriBuilder` object to match various patterns concurrently
/// from multiple threads.
static void test3_URIBuilderMultiThreaded()
{
    bmqtst::TestHelper::printTestName("MULTI-THREADED URI BUILDER TEST");

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    const size_t k_NUM_THREADS    = 6;
    const size_t k_NUM_ITERATIONS = 10000;

    struct Local {
        static void threadFunction(size_t                  threadId,
                                   bsl::vector<bmqt::Uri>* out,
                                   bslmt::Barrier*         barrier)
        {
            out->reserve(k_NUM_ITERATIONS);
            barrier->wait();

            for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
                bmqu::MemOutStream osstrDomain(
                    bmqtst::TestHelperUtil::allocator());
                bmqu::MemOutStream osstrQueue(
                    bmqtst::TestHelperUtil::allocator());
                osstrDomain << "my.domain." << threadId << "." << i;
                osstrQueue << "queue-foo-bar-" << threadId << "-" << i;

                bmqt::Uri qualifiedUri(bmqtst::TestHelperUtil::allocator());
                bmqt::UriBuilder builder(bmqtst::TestHelperUtil::allocator());
                builder.setDomain(osstrDomain.str());
                builder.setQueue(osstrQueue.str());

                const int rc = builder.uri(&qualifiedUri);
                BMQTST_ASSERT_EQ_D("Failed to build bmqt::Uri, i="
                                       << i << ", rc=" << rc,
                                   rc,
                                   0);
                out->emplace_back(bslmf::MovableRefUtil::move(qualifiedUri));
            }
        }
    };

    // Barrier to get each thread to start at the same time; `+1` for this
    // (main) thread.
    bslmt::Barrier barrier(k_NUM_THREADS + 1);

    bsl::vector<bsl::vector<bmqt::Uri> > threadsData(
        bmqtst::TestHelperUtil::allocator());
    threadsData.resize(k_NUM_THREADS);

    bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());
    for (size_t i = 0; i < k_NUM_THREADS; ++i) {
        const int rc = threadGroup.addThread(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &Local::threadFunction,
                                  i,
                                  &threadsData[i],
                                  &barrier));
        BMQTST_ASSERT_EQ_D(i, rc, 0);
    }

    barrier.wait();
    threadGroup.joinAll();

    // Validate for each thread.

    for (size_t i = 0; i < k_NUM_THREADS; ++i) {
        const bsl::vector<bmqt::Uri>& uris = threadsData[i];

        BMQTST_ASSERT_EQ(uris.size(), k_NUM_ITERATIONS);

        for (size_t j = 0; j < k_NUM_ITERATIONS; ++j) {
            bmqu::MemOutStream expectedUriStr(
                bmqtst::TestHelperUtil::allocator());
            expectedUriStr << "bmq://my.domain." << i << "." << j
                           << "/queue-foo-bar-" << i << "-" << j;
            bmqt::Uri expectedUri(expectedUriStr.str(),
                                  bmqtst::TestHelperUtil::allocator());

            BMQTST_ASSERT_EQ_D(i << ", " << j, uris[j], expectedUri);
        }
    }

    bmqt::UriParser::shutdown();
}

static void test4_initializeShutdown()
// ------------------------------------------------------------------------
// Testing:
//   'UriParser' initialize and shutdown.  Should be able to call
//   '.initialize()' and '.shutdown()' after the instance has already
//   started or shutdown, and have no effect.  Shutting down the
//   'UriParser' without a call to 'initialize' should assert.
//
// Plan:
//   Initialize the 'UriParser' again.  Initialize the 'UriParser' again.
//   Shutdown the 'UriParser'.  Shutdown the 'UriParser' again.
//   Shutdown the 'UriParser', destroying it.  Shutdown the 'UriParser'
//   again.
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("INITIALIZE / SHUTDOWN");

    // Initialize the 'UriParser'.
    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    // Initialize should be a no-op.
    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    // Shutdown the parser is a no-op.
    bmqt::UriParser::shutdown();

    // Shut down the parser is a no-op.
    bmqt::UriParser::shutdown();
}

/// Test Uri print method.
static void test5_testPrint()
{
    bmqtst::TestHelper::printTestName("PRINT");

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    PV("Testing print");

    bmqu::MemOutStream stream(bmqtst::TestHelperUtil::allocator());
    bmqt::Uri          obj("bmq://my.domain/myQueue",
                  bmqtst::TestHelperUtil::allocator());

    // Test stream output without line feed
    stream << obj;
    BMQTST_ASSERT_EQ(stream.str(), "bmq://my.domain/myQueue");
    stream.reset();
    // Test print method with a line feed
    obj.print(stream, 0, 0);
    BMQTST_ASSERT_EQ(stream.str(), "bmq://my.domain/myQueue\n");
    stream.reset();

    PV("Bad stream test");
    stream << "NO LAYOUT";
    stream.clear(bsl::ios_base::badbit);
    stream << obj;
    BMQTST_ASSERT_EQ(stream.str(), "NO LAYOUT");

    bmqt::UriParser::shutdown();
}

static void test6_hashAppend()
// ------------------------------------------------------------------------
// TEST HASH APPEND
//
// Concerns:
//   Ensure that 'hashAppend' on 'bmqt::Uri' is functional.
//
// Plan:
//  1) Generate a 'bmqt::Uri' object, compute its hash using the default
//     'bslh' hashing algorithm and verify that 'hashAppend' on this
//     object is deterministic by comparing the hash value over many
//     iterations.
//
// Testing:
//   template <class HASH_ALGORITHM>
//   void
//   hashAppend(HASH_ALGORITHM&  hashAlgo, const bmqt::Uri& uri)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HASH APPEND");

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    PV("HASH FUNCTION DETERMINISTIC");

    int         rc = 0;
    bsl::string error(bmqtst::TestHelperUtil::allocator());
    bsl::string uri("bmq://my.domain/queue-foo-bar",
                    bmqtst::TestHelperUtil::allocator());
    bmqt::Uri   obj(bmqtst::TestHelperUtil::allocator());

    rc = bmqt::UriParser::parse(&obj, &error, uri);

    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(obj.isValid(), true);

    const size_t                      k_NUM_ITERATIONS = 1000;
    bsl::hash<bmqt::Uri>              hasher;
    bsl::hash<bmqt::Uri>::result_type firstHash = hasher(obj);

    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        bslh::DefaultHashAlgorithm algo;
        hashAppend(algo, obj);
        bsl::hash<bmqt::Uri>::result_type currHash = algo.computeHash();
        PVVV("[" << i << "] hash: " << currHash);
        BMQTST_ASSERT_EQ_D(i, currHash, firstHash);
    }

    bmqt::UriParser::shutdown();
}

static void test7_testLongUri()
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Disable the default allocator check. When 'bmqt::Uri'
    // is created from a long uri string error message is
    // logged via 'BALL_LOG_ERROR' which allocates using
    // the default allocator.

    bmqtst::TestHelper::printTestName("LONG URI TEST");

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    bmqtst::ScopedLogObserver observer(ball::Severity::e_WARN,
                                       bmqtst::TestHelperUtil::allocator());

    bsl::string domainStr("bmq://my.domain/",
                          bmqtst::TestHelperUtil::allocator());
    bsl::string pathStr(bmqt::Uri::k_QUEUENAME_MAX_LENGTH + 1,
                        'q',
                        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream stream(bmqtst::TestHelperUtil::allocator());
    stream << domainStr << pathStr;

    bmqt::Uri obj(stream.str(), bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(obj.isValid(), false);

    bmqt::UriParser::shutdown();
}

#ifdef BMQTST_BENCHMARK_ENABLED

struct UriParserBenchmark {
    static void bench(bslmt::Latch*   initLatch_p,
                      bslmt::Barrier* startBarrier_p,
                      bslmt::Latch*   finishLatch_p)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(initLatch_p);
        BSLS_ASSERT_OPT(startBarrier_p);
        BSLS_ASSERT_OPT(finishLatch_p);

        const size_t      k_NUM_ITERATIONS = 100000;
        const bsl::string k_SAMPLE_URI(
            "bmq://my.sample.domain.~dev/my-queue-name?id=consumer123",
            bmqtst::TestHelperUtil::allocator());

        bmqt::Uri   uri(bmqtst::TestHelperUtil::allocator());
        bsl::string error(bmqtst::TestHelperUtil::allocator());

        initLatch_p->arrive();
        startBarrier_p->wait();

        for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
            bmqt::UriParser::parse(&uri, &error, k_SAMPLE_URI);
        }

        finishLatch_p->arrive();
    }
};

struct UriConstructorBenchmark {
    static void bench(bslmt::Latch*   initLatch_p,
                      bslmt::Barrier* startBarrier_p,
                      bslmt::Latch*   finishLatch_p)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(initLatch_p);
        BSLS_ASSERT_OPT(startBarrier_p);
        BSLS_ASSERT_OPT(finishLatch_p);

        const size_t      k_NUM_ITERATIONS = 100000;
        const bsl::string k_SAMPLE_URI(
            "bmq://my.sample.domain.~dev/my-queue-name?id=consumer123",
            bmqtst::TestHelperUtil::allocator());

        // Test allocator is slow and might skew the benchmarks
        bdlma::LocalSequentialAllocator<256> lsa(
            bmqtst::TestHelperUtil::allocator());

        initLatch_p->arrive();
        startBarrier_p->wait();

        for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
            bmqt::Uri uri(k_SAMPLE_URI, &lsa);
            (void)uri;
        }

        finishLatch_p->arrive();
    }
};

template <size_t NUM_THREADS, typename BENCHMARK>
static void testN1_benchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// URI PERFORMANCE TEST
//
// Plan: spawn NUM_THREADS and measure the time taken for BENCHMARK::bench
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("URI PERFORMANCE TEST");

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    bslmt::Latch   initThreadLatch(NUM_THREADS);
    bslmt::Barrier startBenchmarkBarrier(NUM_THREADS + 1);
    bslmt::Latch   finishBenchmarkLatch(NUM_THREADS);

    bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        const int rc = threadGroup.addThread(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &(BENCHMARK::bench),
                                  &initThreadLatch,
                                  &startBenchmarkBarrier,
                                  &finishBenchmarkLatch));
        BMQTST_ASSERT_EQ_D(i, rc, 0);
    }

    initThreadLatch.wait();

    size_t iter = 0;
    for (auto _ : state) {
        // Benchmark time start

        // We don't support running multi-iteration benchmarks because we
        // prepare and start complex tasks in separate threads.
        // Once these tasks are finished, we cannot simply re-run them without
        // reinitialization, and it goes against benchmark library design.
        // Make sure we run this only once.
        BSLS_ASSERT_OPT(0 == iter++ && "Must be run only once");

        startBenchmarkBarrier.wait();
        finishBenchmarkLatch.wait();

        // Benchmark time end
    }

    threadGroup.joinAll();
    bmqt::UriParser::shutdown();
}

#endif  // BMQTST_BENCHMARK_ENABLED

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 7: test7_testLongUri(); break;
    case 6: test6_hashAppend(); break;
    case 5: test5_testPrint(); break;
    case 4: test4_initializeShutdown(); break;
    case 3: test3_URIBuilderMultiThreaded(); break;
    case 2: test2_URIBuilder(); break;
    case 1: test1_breathingTest(); break;
    case -1: {
#ifdef BMQTST_BENCHMARK_ENABLED
        BENCHMARK(testN1_benchmark<1, UriParserBenchmark>)
            ->Name("bmqt::UriParser::parse threads=1")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_benchmark<2, UriParserBenchmark>)
            ->Name("bmqt::UriParser::parse threads=2")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_benchmark<4, UriParserBenchmark>)
            ->Name("bmqt::UriParser::parse threads=4")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_benchmark<8, UriParserBenchmark>)
            ->Name("bmqt::UriParser::parse threads=8")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);

        BENCHMARK(testN1_benchmark<1, UriConstructorBenchmark>)
            ->Name("bmqt::Uri::Uri         threads=1")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_benchmark<2, UriConstructorBenchmark>)
            ->Name("bmqt::Uri::Uri         threads=2")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_benchmark<4, UriConstructorBenchmark>)
            ->Name("bmqt::Uri::Uri         threads=4")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_benchmark<8, UriConstructorBenchmark>)
            ->Name("bmqt::Uri::Uri         threads=8")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);

        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
#else
        cerr << "WARNING: BENCHMARK '" << _testCase
             << "' IS NOT SUPPORTED ON THIS PLATFORM." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
#endif  // BMQTST_BENCHMARK_ENABLED
    } break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
