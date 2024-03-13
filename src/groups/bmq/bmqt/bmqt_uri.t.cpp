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

// MWC
#include <mwctst_scopedlogobserver.h>
#include <mwcu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslh_defaulthashalgorithm.h>
#include <bslh_hash.h>
#include <bslmt_barrier.h>
#include <bslmt_threadgroup.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Thread function: wait on the specified `barrier` and then generate the
/// specified `numGUIDs` (in a tight loop) and store them in the specified
/// `out`.
static void threadFunction(int                                      threadId,
                           bsl::vector<bsl::pair<bmqt::Uri, int> >* out,
                           bslmt::Barrier*                          barrier,
                           int numIterations)
{
    out->reserve(numIterations);
    barrier->wait();

    for (int i = 0; i < numIterations; ++i) {
        mwcu::MemOutStream osstrDomain(s_allocator_p);
        mwcu::MemOutStream osstrQueue(s_allocator_p);
        osstrDomain << "my.domain." << threadId << "." << i;
        osstrQueue << "queue-foo-bar-" << threadId << "-" << i;

        bmqt::Uri        qualifiedUri(s_allocator_p);
        bmqt::UriBuilder builder(s_allocator_p);
        builder.setDomain(osstrDomain.str());
        builder.setQueue(osstrQueue.str());

        int rc = builder.uri(&qualifiedUri);
        out->push_back(bsl::make_pair(qualifiedUri, rc));
    }
}

}  // close unnamed namespace

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
    mwctst::TestHelper::printTestName("BREATHING TEST");

    bmqt::UriParser::initialize(s_allocator_p);

    int         rc = 0;
    bsl::string error(s_allocator_p);

    PV("Test constructors");
    {
        bsl::string uriStr("bmq://my.domain/queue-foo-bar", s_allocator_p);
        bmqt::Uri   obj1(uriStr, s_allocator_p);

        ASSERT_EQ(obj1.isValid(), true);

        const bslstl::StringRef& uriStrRef = uriStr;
        bmqt::Uri                obj2(uriStrRef, s_allocator_p);

        ASSERT_EQ(obj2.isValid(), true);
        ASSERT_EQ(obj1, obj2);

        const char* uriRaw = uriStr.data();
        bmqt::Uri   obj3(uriRaw, s_allocator_p);

        ASSERT_EQ(obj3.isValid(), true);
        ASSERT_EQ(obj2, obj3);

        bmqt::Uri obj4(obj3, s_allocator_p);

        ASSERT_EQ(obj4.isValid(), true);
        ASSERT_EQ(obj3, obj4);
    }

    PV("Test basic parsing");
    {
        bmqt::Uri obj(s_allocator_p);
        ASSERT_EQ(obj.isValid(), false);
        {
            // Scope input to ensure it gets deleted after (and so validate the
            // string ref are correctly pointing to the right values).
            bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
            rc = bmqt::UriParser::parse(&obj, &error, uri);
            ASSERT_EQ(rc, 0);
            ASSERT_EQ(error, "");
            ASSERT_EQ(obj.asString(), uri);
            uri = "deadbeef";  // put 'garbage' in input
        }

        ASSERT_EQ(obj.isValid(), true);
        ASSERT_EQ(obj.scheme(), "bmq");
        ASSERT_EQ(obj.authority(), "my.domain");
        ASSERT_EQ(obj.path(), "queue-foo-bar");
        ASSERT_EQ(obj.domain(), "my.domain");
        ASSERT_EQ(obj.queue(), "queue-foo-bar");
        ASSERT_EQ(obj.id(), "");
        ASSERT_EQ(obj.canonical(), "bmq://my.domain/queue-foo-bar");
    }

    PV("Test assignment operator")
    {
        const char k_URI[] = "bmq://my.domain.~tst/queue";
        bmqt::Uri  objCopy(s_allocator_p);

        {
            // Scope the original URI, so that it will be destroyed before we
            // check the copy.
            bmqt::Uri objOriginal(s_allocator_p);
            rc = bmqt::UriParser::parse(&objOriginal, &error, k_URI);
            ASSERT_EQ(rc, 0);
            ASSERT_EQ(error, "");
            objCopy = objOriginal;
        }

        ASSERT_EQ(objCopy.isValid(), true);
        ASSERT_EQ(objCopy.asString(), k_URI);
        ASSERT_EQ(objCopy.scheme(), "bmq");
        ASSERT_EQ(objCopy.authority(), "my.domain.~tst");
        ASSERT_EQ(objCopy.domain(), "my.domain");
        ASSERT_EQ(objCopy.tier(), "tst");
        ASSERT_EQ(objCopy.path(), "queue");
        ASSERT_EQ(objCopy.queue(), "queue");
        ASSERT_EQ(objCopy.id(), "");
        ASSERT_EQ(objCopy.canonical(), "bmq://my.domain.~tst/queue");
    }

    PV("Testing comparison operators")
    {
        const char k_URI[] = "bmq://my.domain/queue";
        bmqt::Uri  obj1(s_allocator_p);

        bmqt::UriParser::parse(&obj1, &error, k_URI);

        bmqt::Uri obj2(obj1, s_allocator_p);
        ASSERT_EQ(obj1, obj2);

        bmqt::Uri obj3(s_allocator_p);
        ASSERT_NE(obj1, obj3);

        obj3 = obj2;
        ASSERT_EQ(obj3, obj2);
    }

    PV("Testing valid URI parsing")
    {
        {
            const char k_URI[] = "bmq://ts.trades.myapp/my.queue";
            bmqt::Uri  obj(s_allocator_p);
            rc = bmqt::UriParser::parse(&obj, &error, k_URI);
            ASSERT_EQ(rc, 0);
            ASSERT_EQ(error, "");
            ASSERT_EQ(obj.isValid(), true);
            ASSERT_EQ(obj.scheme(), "bmq");
            ASSERT_EQ(obj.authority(), "ts.trades.myapp");
            ASSERT_EQ(obj.domain(), "ts.trades.myapp");
            ASSERT_EQ(obj.path(), "my.queue");
            ASSERT_EQ(obj.queue(), "my.queue");
            ASSERT_EQ(obj.id(), "");
            ASSERT_EQ(obj.canonical(), "bmq://ts.trades.myapp/my.queue");
        }
        {
            const char k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";
            bmqt::Uri  obj(s_allocator_p);
            rc = bmqt::UriParser::parse(&obj, &error, k_URI);
            ASSERT_EQ(rc, 0);
            ASSERT_EQ(error, "");
            ASSERT_EQ(obj.isValid(), true);
            ASSERT_EQ(obj.scheme(), "bmq");
            ASSERT_EQ(obj.authority(), "ts.trades.myapp");
            ASSERT_EQ(obj.domain(), "ts.trades.myapp");
            ASSERT_EQ(obj.queue(), "my.queue");
            ASSERT_EQ(obj.id(), "my.app");
            ASSERT_EQ(obj.canonical(), "bmq://ts.trades.myapp/my.queue");
        }
        {
            const char k_URI[] = "bmq://ts.trades.myapp.~tst/my.queue";
            bmqt::Uri  obj(s_allocator_p);
            rc = bmqt::UriParser::parse(&obj, &error, k_URI);
            ASSERT_EQ(rc, 0);
            ASSERT_EQ(error, "");
            ASSERT_EQ(obj.isValid(), true);
            ASSERT_EQ(obj.scheme(), "bmq");
            ASSERT_EQ(obj.authority(), "ts.trades.myapp.~tst");
            ASSERT_EQ(obj.domain(), "ts.trades.myapp");
            ASSERT_EQ(obj.qualifiedDomain(), "ts.trades.myapp.~tst");
            ASSERT_EQ(obj.tier(), "tst");
            ASSERT_EQ(obj.queue(), "my.queue");
            ASSERT_EQ(obj.canonical(),
                      "bmq://ts.trades.myapp.~tst"
                      "/my.queue");
        }
        {
            const char k_URI[] = "bmq://ts.trades.myapp.~lcl-fooBar/my.queue";
            bmqt::Uri  obj(s_allocator_p);
            rc = bmqt::UriParser::parse(&obj, &error, k_URI);
            ASSERT_EQ(rc, 0);
            ASSERT_EQ(error, "");
            ASSERT_EQ(obj.isValid(), true);
            ASSERT_EQ(obj.scheme(), "bmq");
            ASSERT_EQ(obj.authority(), "ts.trades.myapp.~lcl-fooBar");
            ASSERT_EQ(obj.domain(), "ts.trades.myapp");
            ASSERT_EQ(obj.qualifiedDomain(), "ts.trades.myapp.~lcl-fooBar");
            ASSERT_EQ(obj.tier(), "lcl-fooBar");
            ASSERT_EQ(obj.queue(), "my.queue");
            ASSERT_EQ(obj.canonical(),
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
            {L_, "", -1},
            {L_, "foobar", -1},
            {L_, "bb://", -1},
            {L_, "bmq://", -1},
            {L_, "bmq://a/", -4},
            {L_, "bmq://$%@/ts.trades.myapp/queue@sss", -1},
            {L_, "bb:///ts.trades.myapp/myqueue", -1},
            {L_, "bmq://ts.trades.myapp/", -4},
            {L_, "bmq://ts.trades.myapp/queue?id=", -1},
            {L_, "bmq://ts.trades.myapp/queue?bs=a", -1},
            {L_, "bmq://ts.trades.myapp/queue?", -1},
            {L_, "bmq://ts.trades.myapp/queue?id=", -1},
            {L_, "bmq://ts.trades.myapp/queue&id==", -1},
            {L_, "bmq://ts.trades.myapp/queue&id=foo", -1},
            {L_, "bmq://ts.trades.myapp/queue?id=foo&", -1},
            {L_, "bmq://ts.trades.myapp/queue?pid=foo", -1},
            {L_, "bmq://ts.trades.myapp.~/queue", -5},
            {L_, "bmq://ts.trades~myapp/queue", -1},
            {L_, "bmq://ts.trades.myapp.~a_b/queue", -1},
        };

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];

            bmqt::Uri obj(s_allocator_p);
            rc = bmqt::UriParser::parse(&obj, &error, test.d_input);
            ASSERT_EQ_D(test.d_line, rc, test.d_rc);
            ASSERT_EQ_D(test.d_line, obj.isValid(), false);

            // Retest without specifying the optional error strings
            rc = bmqt::UriParser::parse(&obj, 0, test.d_input);
            ASSERT_EQ_D(test.d_line, rc, test.d_rc);
            ASSERT_EQ_D(test.d_line, obj.isValid(), false);
        }
    }

    bmqt::UriParser::shutdown();
}

static void test2_URIBuilder()
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    bmqt::UriParser::initialize(s_allocator_p);

    bmqt::UriBuilder builder(s_allocator_p);

    {
        bmqt::Uri uri(s_allocator_p);
        builder.reset();

        builder.setDomain("si.uics.tester").setQueue("siqueue");
        ASSERT_EQ(builder.uri(&uri, 0), 0);
        ASSERT_EQ(uri.asString(), "bmq://si.uics.tester/siqueue");
        ASSERT_EQ(uri.isValid(), true);
    }

    {
        bmqt::Uri uri(s_allocator_p);
        builder.reset();

        builder.setDomain("bmq.tutorial").setQueue("worker").setId("myApp");
        ASSERT_EQ(builder.uri(&uri, 0), 0);
        ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial/worker?id=myApp");
        ASSERT_EQ(uri.authority(), "bmq.tutorial");
        ASSERT_EQ(uri.domain(), "bmq.tutorial");
        ASSERT_EQ(uri.tier(), "");
        ASSERT_EQ(uri.path(), "worker");
        ASSERT_EQ(uri.id(), "myApp");
        ASSERT_EQ(uri.isValid(), true);
    }

    {
        bmqt::Uri uri(s_allocator_p);
        builder.reset();

        builder.setDomain("bmq.tutorial").setTier("tst").setQueue("myQueue");
        ASSERT_EQ(builder.uri(&uri, 0), 0);
        ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial.~tst/myQueue");
        ASSERT_EQ(uri.authority(), "bmq.tutorial.~tst");
        ASSERT_EQ(uri.domain(), "bmq.tutorial");
        ASSERT_EQ(uri.tier(), "tst");
        ASSERT_EQ(uri.path(), "myQueue");
        ASSERT_EQ(uri.isValid(), true);
    }

    PV("domain/tier/qualifiedDomain correlation")
    {
        bmqt::Uri uri(s_allocator_p);
        builder.reset();

        builder.setQualifiedDomain("bmq.tutorial.~lcl").setQueue("myQueue");
        ASSERT_EQ(builder.uri(&uri, 0), 0);
        ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial.~lcl/myQueue");
        ASSERT_EQ(uri.authority(), "bmq.tutorial.~lcl");
        ASSERT_EQ(uri.domain(), "bmq.tutorial");
        ASSERT_EQ(uri.tier(), "lcl");
        ASSERT_EQ(uri.path(), "myQueue");
        ASSERT_EQ(uri.isValid(), true);

        builder.setTier("tst");
        ASSERT_EQ(builder.uri(&uri, 0), 0);
        ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial.~tst/myQueue");
        ASSERT_EQ(uri.authority(), "bmq.tutorial.~tst");
        ASSERT_EQ(uri.domain(), "bmq.tutorial");
        ASSERT_EQ(uri.tier(), "tst");

        builder.setDomain("bmq.test");
        ASSERT_EQ(builder.uri(&uri, 0), 0);
        ASSERT_EQ(uri.asString(), "bmq://bmq.test.~tst/myQueue");
        ASSERT_EQ(builder.uri(&uri, 0), 0);
        ASSERT_EQ(uri.authority(), "bmq.test.~tst");
        ASSERT_EQ(uri.domain(), "bmq.test");
        ASSERT_EQ(uri.tier(), "tst");

        builder.setQualifiedDomain("bmq.tutorial.~lcl");
        ASSERT_EQ(builder.uri(&uri, 0), 0);
        ASSERT_EQ(uri.asString(), "bmq://bmq.tutorial.~lcl/myQueue");
        ASSERT_EQ(uri.authority(), "bmq.tutorial.~lcl");
        ASSERT_EQ(uri.domain(), "bmq.tutorial");
        ASSERT_EQ(uri.tier(), "lcl");
        ASSERT_EQ(uri.isValid(), true);
    }

    PV("Test error case");
    {
        bmqt::Uri   uri(s_allocator_p);
        bsl::string errorMessage(s_allocator_p);
        builder.reset();

        ASSERT_EQ(builder.uri(&uri, &errorMessage), -3);  // -3: MISSING DOMAIN
        ASSERT_EQ(errorMessage, "missing domain");
        builder.setDomain("my.domain");
        ASSERT_EQ(uri.isValid(), false);

        ASSERT_EQ(builder.uri(&uri, &errorMessage), -4);  // -4: MISSING QUEUE
        ASSERT_EQ(errorMessage, "missing queue");
        builder.setQueue("myQueue");
        ASSERT_EQ(builder.uri(&uri, 0), 0);
        ASSERT_EQ(uri.asString(), "bmq://my.domain/myQueue");
        ASSERT_EQ(uri.isValid(), true);
    }

    PV("Test creating a builder by copy of a Uri");
    {
        bmqt::Uri        uri("bmq://my.domain/myQueue", s_allocator_p);
        bmqt::Uri        tmpUri(s_allocator_p);
        bmqt::UriBuilder uriBuilder(uri, s_allocator_p);

        // Validate the uri in builder is the same
        ASSERT_EQ(uriBuilder.uri(&tmpUri, 0), 0);
        ASSERT_EQ(tmpUri.asString(), "bmq://my.domain/myQueue");

        // Update URI in builder
        uriBuilder.setQueue("yourQueue");

        // Ensure original URI is unchanged
        ASSERT_EQ(uri.asString(), "bmq://my.domain/myQueue");

        // Verify the built URI has the change
        ASSERT_EQ(uriBuilder.uri(&tmpUri, 0), 0);
        ASSERT_EQ(tmpUri.asString(), "bmq://my.domain/yourQueue");
    }

    bmqt::UriParser::shutdown();
}

/// Test same `UriBuilder` object to match various patterns concurrently
/// from multiple threads.
static void test3_URIBuilderMultiThreaded()
{
    s_ignoreCheckGblAlloc = true;
    s_ignoreCheckDefAlloc = true;
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator to allocate
    // memory.

    mwctst::TestHelper::printTestName("MULTI-THREADED URI BUILDER TEST");

    bmqt::UriParser::initialize(s_allocator_p);

    const int          k_NUM_THREADS    = 6;
    const int          k_NUM_ITERATIONS = 10000;
    bslmt::ThreadGroup threadGroup(s_allocator_p);

    // Barrier to get each thread to start at the same time; `+1` for this
    // (main) thread.
    bslmt::Barrier barrier(k_NUM_THREADS + 1);

    typedef bsl::pair<bmqt::Uri, int> Result;

    bsl::vector<bsl::vector<Result> > threadsData(s_allocator_p);
    threadsData.resize(k_NUM_THREADS);

    for (int i = 0; i < k_NUM_THREADS; ++i) {
        int rc = threadGroup.addThread(bdlf::BindUtil::bind(&threadFunction,
                                                            i,
                                                            &threadsData[i],
                                                            &barrier,
                                                            k_NUM_ITERATIONS));
        ASSERT_EQ_D(i, rc, 0);
    }

    barrier.wait();
    threadGroup.joinAll();

    // Validate for each thread.

    for (int i = 0; i < k_NUM_THREADS; ++i) {
        const bsl::vector<Result>& threadResults = threadsData[i];

        BSLS_ASSERT_OPT(threadResults.size() ==
                        static_cast<size_t>(k_NUM_ITERATIONS));

        for (int j = 0; j < k_NUM_ITERATIONS; ++j) {
            ASSERT_EQ_D(i << ", " << j,
                        threadResults[j].second,
                        0);  // builder rc

            mwcu::MemOutStream expectedUriStr(s_allocator_p);
            expectedUriStr << "bmq://my.domain." << i << "." << j
                           << "/queue-foo-bar-" << i << "-" << j;
            bmqt::Uri expectedUri(expectedUriStr.str(), s_allocator_p);

            ASSERT_EQ_D(i << ", " << j, threadResults[j].first, expectedUri);
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
    mwctst::TestHelper::printTestName("INITIALIZE / SHUTDOWN");

    // Initialize the 'UriParser'.
    bmqt::UriParser::initialize(s_allocator_p);

    // Initialize should be a no-op.
    bmqt::UriParser::initialize(s_allocator_p);

    // Shutdown the parser is a no-op.
    bmqt::UriParser::shutdown();

    // Shut down the parser is a no-op.
    bmqt::UriParser::shutdown();

    // Shutdown again should assert
    ASSERT_SAFE_FAIL(bmqt::UriParser::shutdown());
}

/// Test Uri print method.
static void test5_testPrint()
{
    mwctst::TestHelper::printTestName("PRINT");

    bmqt::UriParser::initialize(s_allocator_p);

    PV("Testing print");

    mwcu::MemOutStream stream(s_allocator_p);
    bmqt::Uri          obj("bmq://my.domain/myQueue", s_allocator_p);

    // Test stream output without line feed
    stream << obj;
    ASSERT_EQ(stream.str(), "bmq://my.domain/myQueue");
    stream.reset();
    // Test print method with a line feed
    obj.print(stream, 0, 0);
    ASSERT_EQ(stream.str(), "bmq://my.domain/myQueue\n");
    stream.reset();

    PV("Bad stream test");
    stream << "NO LAYOUT";
    stream.clear(bsl::ios_base::badbit);
    stream << obj;
    ASSERT_EQ(stream.str(), "NO LAYOUT");

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
    mwctst::TestHelper::printTestName("HASH APPEND");

    bmqt::UriParser::initialize(s_allocator_p);

    PV("HASH FUNCTION DETERMINISTIC");

    int         rc = 0;
    bsl::string error(s_allocator_p);
    bsl::string uri("bmq://my.domain/queue-foo-bar", s_allocator_p);
    bmqt::Uri   obj(s_allocator_p);

    rc = bmqt::UriParser::parse(&obj, &error, uri);

    ASSERT_EQ(rc, 0);
    ASSERT_EQ(obj.isValid(), true);

    const size_t                      k_NUM_ITERATIONS = 1000;
    bsl::hash<bmqt::Uri>              hasher;
    bsl::hash<bmqt::Uri>::result_type firstHash = hasher(obj);

    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        bslh::DefaultHashAlgorithm algo;
        hashAppend(algo, obj);
        bsl::hash<bmqt::Uri>::result_type currHash = algo.computeHash();
        PVVV("[" << i << "] hash: " << currHash);
        ASSERT_EQ_D(i, currHash, firstHash);
    }

    bmqt::UriParser::shutdown();
}

static void test7_testLongUri()
{
    s_ignoreCheckDefAlloc = true;
    // Disable the default allocator check. When 'bmqt::Uri'
    // is created from a long uri string error message is
    // logged via 'BALL_LOG_ERROR' which allocates using
    // the default allocator.

    mwctst::TestHelper::printTestName("LONG URI TEST");

    bmqt::UriParser::initialize(s_allocator_p);

    mwctst::ScopedLogObserver observer(ball::Severity::WARN, s_allocator_p);

    bsl::string domainStr("bmq://my.domain/", s_allocator_p);
    bsl::string pathStr(bmqt::Uri::k_QUEUENAME_MAX_LENGTH + 1,
                        'q',
                        s_allocator_p);

    mwcu::MemOutStream stream(s_allocator_p);
    stream << domainStr << pathStr;

    bmqt::Uri obj(stream.str(), s_allocator_p);

    ASSERT_EQ(observer.records().size(), 1U);

    ASSERT_EQ(observer.records()[0].fixedFields().severity(),
              ball::Severity::ERROR);

    ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
        observer.records()[0],
        pathStr.data(),
        s_allocator_p));

    ASSERT_EQ(obj.isValid(), true);

    bmqt::UriParser::shutdown();
}

static void test8_testUriParserOverflow()
{
    s_ignoreCheckDefAlloc = true;
    // Disable the default allocator check. When 'bmqt::Uri'
    // is created from a long uri string error message is
    // logged via 'BALL_LOG_ERROR' which allocates using
    // the default allocator.

    mwctst::TestHelper::printTestName("URI PARSER OVERFLOW TEST");

    // Initialize bmqt::UriParser many times, and then deinitialize
    // it the same number of times according to the usage contract.
    const bsls::Types::Int64 k_INIT_NUM = 2200000000LL;
    for (bsls::Types::Int64 i = 0; i < k_INIT_NUM; i++) {
        bmqt::UriParser::initialize(s_allocator_p);
    }

    for (bsls::Types::Int64 i = 0; i < k_INIT_NUM; i++) {
        bmqt::UriParser::shutdown();
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 8: test8_testUriParserOverflow(); break;
    case 7: test7_testLongUri(); break;
    case 6: test6_hashAppend(); break;
    case 5: test5_testPrint(); break;
    case 4: test4_initializeShutdown(); break;
    case 3: test3_URIBuilderMultiThreaded(); break;
    case 2: test2_URIBuilder(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
