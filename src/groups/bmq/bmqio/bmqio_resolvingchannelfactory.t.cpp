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

// bmqio_resolvingchannelfactory.t.cpp                                -*-C++-*-
#include <bmqio_resolvingchannelfactory.h>

#include <bdlf_bind.h>
#include <bdlma_localsequentialallocator.h>

#include <bmqio_testchannel.h>
#include <bmqio_testchannelfactory.h>
#include <bmqtst_testhelper.h>

#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace bmqio;

// FUNCTIONS
static ntsa::Error
testResolveFn(bsl::string*                  domainName,
              const ntsa::IpAddress&        address,
              bsl::vector<bsl::string>*     retHosts,
              bsl::vector<ntsa::IpAddress>* expectedAddresses)
{
    ASSERT(!expectedAddresses->empty());

    if (address != (*expectedAddresses)[0]) {
        bsl::ostringstream ss;
        ss << address;
        ASSERT_D(ss.str(), false);
    }
    expectedAddresses->erase(expectedAddresses->begin());

    ASSERT(!retHosts->empty());
    *domainName = (*retHosts)[0];
    retHosts->erase(retHosts->begin());
    if (domainName->length() == 0) {
        return ntsa::Error(ntsa::Error::e_UNKNOWN);  // RETURN
    }

    return ntsa::Error();
}

static void testResultCallback(bsl::deque<bsl::shared_ptr<Channel> >* store,
                               ChannelFactoryEvent::Enum              event,
                               BSLS_ANNOTATION_UNUSED const Status&   status,
                               const bsl::shared_ptr<Channel>&        channel)
{
    ASSERT_EQ(event, ChannelFactoryEvent::e_CHANNEL_UP);
    store->push_back(channel);
}

// ==================
// class TestExecutor
// ==================

/// An executor that stores the functions it was used to dispatch.
class TestExecutor {
  public:
    // TYPES
    typedef bsl::deque<bsl::function<void()> > Store;

  private:
    // PRIVATE DATA
    Store* d_store_p;

  public:
    // CREATORS
    explicit TestExecutor(Store* store)
    : d_store_p(store)
    {
        // PRECONDITIONS
        BSLS_ASSERT(store);
    }

    // MANIPULATORS
    template <class FUNCTION>
    void post(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const
    {
        d_store_p->emplace_back(BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
    }

    // ACCESSORS
    bool operator==(const TestExecutor& rhs) const
    {
        // NOTE: The executor needs to be equality-comparable to satisfy the
        //       Executor concept as defined in 'bmqex' package documentation.
        return d_store_p == rhs.d_store_p;
    }
};

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_defaultResolutionFn()
// ------------------------------------------------------------------------
// DEFAULT RESOLUTION FN TEST
//
// Concerns:
//  a) If the channel being resolved has a badly formatted peerUri,
//     nothing is resolved
//  b) If resolution succeeds, the correct string is returned
// ------------------------------------------------------------------------
{
    bsl::vector<bsl::string>     retHosts(s_allocator_p);
    bsl::vector<ntsa::IpAddress> expectedAddresses(s_allocator_p);

    using namespace bdlf::PlaceHolders;
    ResolvingChannelFactoryUtil::ResolveFn resolveFn = bdlf::BindUtil::bind(
        &testResolveFn,
        _1,
        _2,
        &retHosts,
        &expectedAddresses);

    bmqio::TestChannel base(s_allocator_p);
    bsl::string        ret(s_allocator_p);

    // No ':'
    base.setPeerUri("127.0.0.1");
    ResolvingChannelFactoryUtil::defaultResolutionFn(&ret,
                                                     base,
                                                     resolveFn,
                                                     false);
    ASSERT_EQ(ret, "");

    // Can't parse ip address
    base.setPeerUri("127.0.s:123");
    ResolvingChannelFactoryUtil::defaultResolutionFn(&ret,
                                                     base,
                                                     resolveFn,
                                                     false);
    ASSERT_EQ(ret, "");

    // Resolve fails
    expectedAddresses.emplace_back("1.2.3.4");
    retHosts.emplace_back("");
    base.setPeerUri("1.2.3.4:75");
    ResolvingChannelFactoryUtil::defaultResolutionFn(&ret,
                                                     base,
                                                     resolveFn,
                                                     false);
    ASSERT_EQ(ret, "");

    // Resolve succeeds
    expectedAddresses.emplace_back("1.2.3.4");
    retHosts.emplace_back("testHost");
    base.setPeerUri("1.2.3.4:75");
    ResolvingChannelFactoryUtil::defaultResolutionFn(&ret,
                                                     base,
                                                     resolveFn,
                                                     false);
    ASSERT_EQ(ret, "1.2.3.4~testHost:75");
}

static void test2_channelFactory()
// ------------------------------------------------------------------------
// CHANNEL FACTORY
//
// Concerns:
//  a) Before the resolution succeeds, the channel returns its base's
//     peerUri.
//  b) Once resolution completes, 'peerUri' returns the updated uri.
//  c) If resolution doesn't update the uri, keep returning the base's
//     uri.
//  d) Any Executor jobs still outstanding when the factory is destroyed
//     can safely execute.
// ------------------------------------------------------------------------
{
    TestChannelFactory baseFactory(s_allocator_p);

    bsl::vector<bsl::string>     retHosts(s_allocator_p);
    bsl::vector<ntsa::IpAddress> expectedAddresses(s_allocator_p);

    using namespace bdlf::PlaceHolders;
    ResolvingChannelFactoryUtil::ResolveFn resolveFn = bdlf::BindUtil::bind(
        &testResolveFn,
        _1,
        _2,
        &retHosts,
        &expectedAddresses);

    TestExecutor::Store execStore(s_allocator_p);

    ResolvingChannelFactoryConfig cfg(
        &baseFactory,
        bmqex::ExecutionPolicyUtil::oneWay().neverBlocking().useExecutor(
            TestExecutor(&execStore)),
        s_allocator_p);
    cfg.resolutionFn(
        bdlf::BindUtil::bind(&ResolvingChannelFactoryUtil::defaultResolutionFn,
                             _1,
                             _2,
                             resolveFn,
                             false));
    bsl::shared_ptr<ResolvingChannelFactory> obj;
    obj.createInplace(s_allocator_p, cfg, s_allocator_p);

    bsl::deque<bsl::shared_ptr<Channel> > channels;

    obj->listen(
        0,
        0,
        ListenOptions(),
        bdlf::BindUtil::bind(&testResultCallback, &channels, _1, _2, _3));

    bsl::shared_ptr<TestChannel> channel;
    channel.createInplace(s_allocator_p, s_allocator_p);

    // Create a Channel and observe its peerUri get updated
    channel->setPeerUri("1.2.3.4:567");
    baseFactory.listenCalls()[0].d_cb(ChannelFactoryEvent::e_CHANNEL_UP,
                                      Status(),
                                      channel);

    ASSERT_EQ(channels[0]->peerUri(), "1.2.3.4:567");

    expectedAddresses.emplace_back("1.2.3.4");
    retHosts.emplace_back("testHost");

    execStore[0]();

    ASSERT(expectedAddresses.empty());

    ASSERT_EQ(channels[0]->peerUri(), "1.2.3.4~testHost:567");

    // Resolution fails
    baseFactory.listenCalls()[0].d_cb(ChannelFactoryEvent::e_CHANNEL_UP,
                                      Status(),
                                      channel);

    ASSERT_EQ(channels[1]->peerUri(), "1.2.3.4:567");

    expectedAddresses.emplace_back("1.2.3.4");
    retHosts.emplace_back("");

    execStore[1]();

    ASSERT(expectedAddresses.empty());

    ASSERT_EQ(channels[1]->peerUri(), "1.2.3.4:567");

    // Destroy the factory and make sure executor cbs can still be safely
    // executed
    obj.clear();

    execStore[0]();
    execStore[1]();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_channelFactory(); break;
    case 1: test1_defaultResolutionFn(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
