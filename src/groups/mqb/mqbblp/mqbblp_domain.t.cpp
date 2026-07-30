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

// mqbblp_domain.t.cpp                                                -*-C++-*-
#include <mqbblp_domain.h>

// MQB
#include <mqbc_clusterutil.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbcmd_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbmock_cluster.h>
#include <mqbmock_dispatcher.h>
#include <mqbmock_queue.h>
#include <mqbscm_version.h>
#include <mqbstat_domainstats.h>

// BMQ
#include <bmqst_statcontext.h>
#include <bmqt_uri.h>
#include <bmqu_memoutstream.h>
#include <bmqu_tempdirectory.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_barrier.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_timeinterval.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// A minimal `mqbi::Queue` (built on `mqbmock::Queue`) that reports a
/// per-instance URI, so many of them can be registered under distinct names
/// in a single domain.  `mqbmock::Queue` itself hard-codes a single URI,
/// which is insufficient for this test.
class TestQueue : public mqbmock::Queue {
  private:
    // DATA
    bmqt::Uri d_uri;

    /// Incremented on every call to `configure`.  Lets a test observe
    /// whether the reconfigure functor actually reached this queue.
    bsls::AtomicInt d_configureCalls;

  private:
    // NOT IMPLEMENTED
    TestQueue(const TestQueue&) BSLS_KEYWORD_DELETED;
    TestQueue& operator=(const TestQueue&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TestQueue, bslma::UsesBslmaAllocator)

    // CREATORS

    /// @brief Create a `TestQueue` reporting the specified `uri`.
    ///
    /// @param uri       The URI this queue reports from `uri()`.
    /// @param allocator The allocator to supply memory.
    explicit TestQueue(bsl::string_view uri, bslma::Allocator* allocator)
    : mqbmock::Queue(static_cast<mqbi::Domain*>(0), allocator)
    , d_uri(uri, allocator)
    , d_configureCalls(0)
    {
        BSLS_ASSERT_OPT(d_uri.isValid());
    }

    // MANIPULATORS

    /// @brief Record the reconfigure and return success.
    ///
    /// @return 0 always.
    int configure(BSLA_MAYBE_UNUSED bsl::ostream* errorDescription_p,
                  BSLA_MAYBE_UNUSED bool          isReconfigure,
                  BSLA_MAYBE_UNUSED bool          wait) BSLS_KEYWORD_OVERRIDE
    {
        ++d_configureCalls;
        return 0;
    }

    // ACCESSORS

    /// @brief Return the number of times `configure` has been called.
    ///
    /// @return The reconfigure call count.
    int configureCounter() const { return d_configureCalls.load(); }

    /// @brief Return the URI of this queue.
    ///
    /// @return The URI supplied at construction.
    const bmqt::Uri& uri() const BSLS_KEYWORD_OVERRIDE { return d_uri; }
};

/// @brief Build a valid priority / in-memory domain configuration.
///
/// Varying `messagesLimit` yields a configuration that differs from a
/// previous one, so `Domain::configure` does not early-return.
///
/// @param name          The domain name.
/// @param messagesLimit The domain-wide message limit.
/// @param allocator     The allocator to supply memory.
///
/// @return The constructed configuration.
mqbconfm::Domain makeConfig(const bsl::string& name,
                            bsls::Types::Int64 messagesLimit,
                            bslma::Allocator*  allocator)
{
    mqbconfm::Domain config(allocator);
    config.name() = name;
    config.mode().makePriority();
    config.consistency().makeEventual();
    config.storage().config().makeInMemory();

    mqbconfm::Limits& limits = config.storage().domainLimits();
    limits.messages()        = messagesLimit;
    limits.bytes()           = messagesLimit * 1024;
    // Watermark ratios default to 0.8, which is valid.

    return config;
}

/// @brief No-op teardown callback for `Domain::teardown`.
///
/// @param domainName The name of the domain being torn down (unused).
void teardownCb(BSLA_MAYBE_UNUSED const bsl::string& domainName)
{
    // NOTHING
}

/// Test fixture owning a fully-constructed `mqbblp::Domain` and its
/// dependencies, mirroring the wiring done by `mqba::DomainManager`.
struct DomainTester {
    // DATA
    bslma::Allocator*                     d_allocator_p;
    bmqu::TempDirectory                   d_tempDir;
    bdlbb::PooledBlobBufferFactory        d_bufferFactory;
    mqbmock::Dispatcher                   d_dispatcher;
    bsl::shared_ptr<mqbmock::Cluster>     d_cluster_sp;
    bsl::shared_ptr<bmqst::StatContext>   d_domainsStatContext_sp;
    bslma::ManagedPtr<bmqst::StatContext> d_queuesStatContext_mp;
    bslma::ManagedPtr<mqbblp::Domain>     d_domain_mp;

    // CREATORS

    /// @brief Build a `Domain` and its dependencies for testing.
    ///
    /// @param allocator The allocator to supply memory.
    explicit DomainTester(bslma::Allocator* allocator)
    : d_allocator_p(allocator)
    , d_tempDir(allocator)
    , d_bufferFactory(1024, allocator)
    , d_dispatcher(allocator)
    , d_cluster_sp(0)
    , d_domainsStatContext_sp(0)
    , d_queuesStatContext_mp(0)
    , d_domain_mp(0)
    {
        // The Domain's dispatcher only needs to accept the reconfigure
        // functors 'Domain::configure' posts; setting 'enqueueOnly' prevents
        // it from ever invoking 'Queue::configure' on the mock queues.
        d_dispatcher.setEnqueueOnly(true);

        // Build a *member* cluster so that 'Domain' does not treat itself as
        // remote (a remote domain skips the reconfigure path entirely).
        mqbmock::Cluster::ClusterNodeDefs nodeDefs(d_allocator_p);
        mqbc::ClusterUtil::appendClusterNode(
            &nodeDefs,
            "testNode1",
            "US-EAST",
            41234,
            mqbmock::Cluster::k_LEADER_NODE_ID,
            d_allocator_p);
        mqbc::ClusterUtil::appendClusterNode(
            &nodeDefs,
            "testNode2",
            "US-EAST",
            41235,
            mqbmock::Cluster::k_LEADER_NODE_ID + 1,
            d_allocator_p);

        d_cluster_sp.createInplace(d_allocator_p,
                                   d_allocator_p,
                                   true,   // isClusterMember
                                   false,  // isLeader
                                   false,  // isFSMWorkflow
                                   false,  // doesFSMwriteQLIST
                                   nodeDefs,
                                   "testCluster",
                                   d_tempDir.path());

        // Allow dispatcher-thread checks (e.g. in 'registerQueue') to pass
        // from any thread.
        d_cluster_sp->setThreadId(mqbi::DispatcherClient::k_ANY_THREAD_ID);

        d_domainsStatContext_sp =
            mqbstat::DomainStatsUtil::initializeStatContext(1, d_allocator_p);

        bmqst::StatContextConfiguration statCfg("domain-test", d_allocator_p);
        d_queuesStatContext_mp = d_domainsStatContext_sp->addSubcontext(
            statCfg);

        bsl::shared_ptr<mqbi::Cluster> clusterBase = d_cluster_sp;

        d_domain_mp.load(new (*d_allocator_p)
                             mqbblp::Domain("domain-test",
                                            &d_dispatcher,
                                            &d_bufferFactory,
                                            clusterBase,
                                            d_domainsStatContext_sp.get(),
                                            d_queuesStatContext_mp,
                                            d_allocator_p),
                         d_allocator_p);
    }

    ~DomainTester()
    {
        // Satisfy 'Domain::~Domain' precondition (must be torn down first).
        d_domain_mp->teardown(&teardownCb);
    }

    /// @brief Return the domain under test.
    ///
    /// @return A reference to the owned `Domain`.
    mqbblp::Domain& domain() { return *d_domain_mp; }

    /// @brief Return the dispatcher wired to the domain under test.
    ///
    /// @return A reference to the owned mock dispatcher.
    mqbmock::Dispatcher& dispatcher() { return d_dispatcher; }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic construction / configuration / teardown of a Domain.
//
// Plan:
//   Construct a Domain, configure it once, reconfigure it, and let the
//   fixture tear it down.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    DomainTester tester(alloc);

    bmqu::MemOutStream errorDescription(alloc);

    // First-time configure.
    int rc = tester.domain().configure(errorDescription,
                                       makeConfig("domain-test", 1000, alloc));
    BMQTST_ASSERT_EQ(rc, 0);

    // Reconfigure with a different config (takes the reconfigure path).
    rc = tester.domain().configure(errorDescription,
                                   makeConfig("domain-test", 2000, alloc));
    BMQTST_ASSERT_EQ(rc, 0);
}

namespace {

/// @brief Repeatedly reconfigure the domain until the stop flag is set.
///
/// Exercises `Domain::configure` concurrently with queue registration.
/// Runs on a spawned thread.
///
/// @param domain    The domain to reconfigure.
/// @param barrier   Barrier used to start in lock-step with the main thread.
/// @param stop      Flag signalling the loop to terminate.
/// @param allocator The allocator to supply memory.
void reconfigureThread(mqbblp::Domain*   domain,
                       bslmt::Barrier*   barrier,
                       bsls::AtomicBool* stop,
                       bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(domain);
    BSLS_ASSERT_OPT(barrier);
    BSLS_ASSERT_OPT(stop);

    const mqbconfm::Domain cfgA = makeConfig("domain-test", 1000, allocator);
    const mqbconfm::Domain cfgB = makeConfig("domain-test", 2000, allocator);

    bmqu::MemOutStream err(allocator);

    barrier->wait();

    bool toggle = false;
    while (!stop->loadRelaxed()) {
        domain->configure(err, toggle ? cfgA : cfgB);
        toggle = !toggle;
    }
}

}  // close unnamed namespace

static void test2_concurrentConfigureAndRegisterQueue()
// ------------------------------------------------------------------------
// CONCURRENT CONFIGURE AND REGISTER-QUEUE
//
// Concerns:
//   'Domain::configure' and 'Domain::registerQueue' must be safe to call
//   concurrently: a 'DOMAINS RECONFIGURE' running on the admin thread may
//   overlap queue open/close activity driving 'registerQueue' on the
//   cluster dispatcher thread.  Concurrent access to the domain's internal
//   queue map must be properly synchronized.
//
// Plan:
//   Spawn one thread that continuously reconfigures the domain while the
//   main thread continuously registers distinct queues.  The test must
//   complete without data races or crashes.  Run under ThreadSanitizer to
//   assert the concurrent accesses are correctly synchronized.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CONCURRENT CONFIGURE AND REGISTER-QUEUE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    DomainTester tester(alloc);

    bmqu::MemOutStream errorDescription(alloc);

    // Establish an initial config so subsequent 'configure' calls take the
    // reconfigure path.
    int rc = tester.domain().configure(errorDescription,
                                       makeConfig("domain-test", 500, alloc));
    BMQTST_ASSERT_EQ(rc, 0);

    // Pre-create a large number of distinct queues to register concurrently
    // with the reconfigure loop.
    const int                                k_NUM_QUEUES = 4000;
    bsl::vector<bsl::shared_ptr<TestQueue> > queues(alloc);
    queues.reserve(k_NUM_QUEUES);
    for (int i = 0; i < k_NUM_QUEUES; ++i) {
        bmqu::MemOutStream uri(alloc);
        uri << "bmq://bmq.test.local/q" << i;
        queues.push_back(bsl::allocate_shared<TestQueue>(alloc, uri.str()));
    }

    bslmt::Barrier   barrier(2);
    bsls::AtomicBool stop(false);

    bslmt::ThreadUtil::Handle handle;
    rc = bslmt::ThreadUtil::create(&handle,
                                   bdlf::BindUtil::bind(&reconfigureThread,
                                                        &tester.domain(),
                                                        &barrier,
                                                        &stop,
                                                        alloc));
    BMQTST_ASSERT_EQ(rc, 0);

    // Start both threads roughly simultaneously, then hammer registrations.
    barrier.wait();
    for (int i = 0; i < k_NUM_QUEUES; ++i) {
        bsl::shared_ptr<mqbi::Queue> queueBase = queues[i];
        tester.domain().registerQueue(queueBase);
    }

    stop.storeRelaxed(true);
    bslmt::ThreadUtil::join(handle);

    // If we get here without a crash / sanitizer abort, the run completed.
    BMQTST_ASSERT_EQ(static_cast<int>(queues.size()), k_NUM_QUEUES);
}

static void test3_reconfigureSkipsUnregisteredQueue()
// ------------------------------------------------------------------------
// RECONFIGURE SKIPS UNREGISTERED QUEUE
//
// Concerns:
//   The queue reconfiguration that 'Domain::configure' dispatches must not
//   access a queue that has been unregistered (and destroyed) after the
//   reconfigure was posted but before it runs.  The dispatched work must
//   hold no owning-or-raw reference that can outlive the queue: a queue
//   gone by execution time must be safely skipped, while queues still
//   alive must still be reconfigured.
//
// Plan:
//   Drive the exact ordering deterministically using the mock dispatcher's
//   'enqueueOnly' mode:
//     1. Register two queues: one to survive, one to be destroyed.
//     2. Reconfigure the domain.  This posts the reconfigure work but, in
//        'enqueueOnly' mode, does not run it yet.
//     3. Unregister the doomed queue and drop every strong reference to it,
//        so it is actually destroyed while the reconfigure work is still
//        queued.  Assert (via a 'weak_ptr') that it is gone.
//     4. Run the queued work with 'processQueue'.
//   The surviving queue must observe exactly one reconfigure and the run
//   must complete cleanly.  Were the dispatched work holding a raw queue
//   pointer, step 4 would dereference freed memory (a use-after-free that
//   ThreadSanitizer / AddressSanitizer flags).
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("RECONFIGURE SKIPS UNREGISTERED QUEUE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    DomainTester tester(alloc);

    bmqu::MemOutStream errorDescription(alloc);

    // Establish an initial config so subsequent 'configure' calls take the
    // reconfigure path.
    int rc = tester.domain().configure(errorDescription,
                                       makeConfig("domain-test", 500, alloc));
    BMQTST_ASSERT_EQ(rc, 0);

    // A queue that will remain registered across the reconfigure.
    bsl::shared_ptr<TestQueue> survivor = bsl::allocate_shared<TestQueue>(
        alloc,
        "bmq://bmq.test.local/survivor");

    // A queue that will be unregistered and destroyed before the reconfigure
    // work runs.
    bsl::shared_ptr<TestQueue> doomed =
        bsl::allocate_shared<TestQueue>(alloc, "bmq://bmq.test.local/doomed");

    // Watch the doomed queue's lifetime without keeping it alive.
    bsl::weak_ptr<TestQueue> doomedWatch = doomed;

    {
        bsl::shared_ptr<mqbi::Queue> survivorBase = survivor;
        bsl::shared_ptr<mqbi::Queue> doomedBase   = doomed;
        tester.domain().registerQueue(survivorBase);
        tester.domain().registerQueue(doomedBase);
    }

    // Reconfigure: this snapshots the queues and *posts* the reconfigure work,
    // but 'enqueueOnly' keeps it queued rather than running it now.
    rc = tester.domain().configure(errorDescription,
                                   makeConfig("domain-test", 1000, alloc));
    BMQTST_ASSERT_EQ(rc, 0);

    // Nothing has run yet.
    BMQTST_ASSERT_EQ(survivor->configureCounter(), 0);

    // Destroy the doomed queue while the reconfigure work is still queued.
    tester.domain().unregisterQueue(doomed.get());
    doomed.reset();

    // The doomed queue is truly gone; any raw pointer to it now dangles.
    BMQTST_ASSERT(doomedWatch.expired());

    // Run the queued reconfigure work.  The destroyed queue must be skipped
    // and the survivor must be reconfigured exactly once.
    tester.dispatcher().processQueue();

    BMQTST_ASSERT_EQ(survivor->configureCounter(), 1);

    // Unregister the survivor so the fixture tears down cleanly.
    tester.domain().unregisterQueue(survivor.get());
}

namespace {

/// @brief Repeatedly issue a domain `INFO` command until the stop flag is set.
///
/// Exercises `Domain::processCommand` (which iterates the domain's queue map)
/// concurrently with queue registration.  Runs on a spawned thread.
///
/// @param domain    The domain to query.
/// @param barrier   Barrier used to start in lock-step with the main thread.
/// @param stop      Flag signalling the loop to terminate.
/// @param allocator The allocator to supply memory.
void processInfoThread(mqbblp::Domain*   domain,
                       bslmt::Barrier*   barrier,
                       bsls::AtomicBool* stop,
                       bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(domain);
    BSLS_ASSERT_OPT(barrier);
    BSLS_ASSERT_OPT(stop);

    mqbcmd::DomainCommand command(allocator);
    command.makeInfo();

    barrier->wait();

    while (!stop->loadRelaxed()) {
        // The mock cluster returns an error for the nested cluster command,
        // but the racy queue-map iteration in 'processCommand' happens before
        // that, which is what this test exercises.
        mqbcmd::DomainResult result(allocator);
        domain->processCommand(&result, command);
    }
}

}  // close unnamed namespace

static void test4_concurrentProcessCommandAndRegisterQueue()
// ------------------------------------------------------------------------
// CONCURRENT PROCESS-COMMAND AND REGISTER-QUEUE
//
// Concerns:
//   'Domain::processCommand' (handling a 'DOMAINS DOMAIN <name> INFOS'
//   command) iterates the domain's internal queue map.  It runs on an admin
//   ('ANY') thread and may overlap queue open activity driving
//   'registerQueue' on the cluster dispatcher thread.  Access to the queue
//   map must be properly synchronized.
//
// Plan:
//   Spawn one thread that continuously issues 'INFO' commands while the main
//   thread continuously registers distinct queues.  The test must complete
//   without data races or crashes.  Run under ThreadSanitizer to assert the
//   concurrent accesses are correctly synchronized.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CONCURRENT PROCESS-COMMAND AND REGISTER-QUEUE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    DomainTester tester(alloc);

    bmqu::MemOutStream errorDescription(alloc);

    int rc = tester.domain().configure(errorDescription,
                                       makeConfig("domain-test", 500, alloc));
    BMQTST_ASSERT_EQ(rc, 0);

    // Pre-create a large number of distinct queues to register concurrently
    // with the 'INFO' loop.
    const int                                k_NUM_QUEUES = 4000;
    bsl::vector<bsl::shared_ptr<TestQueue> > queues(alloc);
    queues.reserve(k_NUM_QUEUES);
    for (int i = 0; i < k_NUM_QUEUES; ++i) {
        bmqu::MemOutStream uri(alloc);
        uri << "bmq://bmq.test.local/q" << i;
        queues.push_back(bsl::allocate_shared<TestQueue>(alloc, uri.str()));
    }

    bslmt::Barrier   barrier(2);
    bsls::AtomicBool stop(false);

    bslmt::ThreadUtil::Handle handle;
    rc = bslmt::ThreadUtil::create(&handle,
                                   bdlf::BindUtil::bind(&processInfoThread,
                                                        &tester.domain(),
                                                        &barrier,
                                                        &stop,
                                                        alloc));
    BMQTST_ASSERT_EQ(rc, 0);

    // Start both threads roughly simultaneously, then hammer registrations.
    barrier.wait();
    for (int i = 0; i < k_NUM_QUEUES; ++i) {
        bsl::shared_ptr<mqbi::Queue> queueBase = queues[i];
        tester.domain().registerQueue(queueBase);
    }

    stop.storeRelaxed(true);
    bslmt::ThreadUtil::join(handle);

    // If we get here without a crash / sanitizer abort, the run completed.
    BMQTST_ASSERT_EQ(static_cast<int>(queues.size()), k_NUM_QUEUES);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    {
        mqbcfg::AppConfig brokerConfig(bmqtst::TestHelperUtil::allocator());
        mqbcfg::BrokerConfig::set(brokerConfig);

        switch (_testCase) {
        case 4: test4_concurrentProcessCommandAndRegisterQueue(); break;
        case 3: test3_reconfigureSkipsUnregisteredQueue(); break;
        case 2: test2_concurrentConfigureAndRegisterQueue(); break;
        case 1: test1_breathingTest(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            bmqtst::TestHelperUtil::testStatus() = -1;
        } break;
        }
    }

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
    // Can't ensure no global memory is allocated because
    // 'bslmt::ThreadUtil::create()' uses the global allocator.
}
