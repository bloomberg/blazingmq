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

// mqbnet_multirequestmanager.t.cpp                                   -*-C++-*-
#include <mqbnet_multirequestmanager.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_messageproperties.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_queueid.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>

// MQB
#include <mqbc_clusterutil.h>
#include <mqbmock_cluster.h>
#include <mqbnet_mockcluster.h>

#include <bmqu_tempdirectory.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdld_datum.h>
#include <bsl_cstddef.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_queue.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bslmt_barrier.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bslmt_threadgroup.h>
#include <bsls_atomic.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqio_testchannel.h>
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
// - creatorsTest
// - setExecutorTest
// - createRequestTest
// - sendRequestTest
// - processResponseTest
// - cancelAllRequestsTest
//-----------------------------------------------------------------------------
// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef bmqp_ctrlmsg::ControlMessage               Mes;
typedef bsl::queue<Mes>                            MesQue;
typedef bmqp::RequestManager<Mes, Mes>             ReqManagerType;
typedef bsl::shared_ptr<ReqManagerType>            ReqManagerTypeSp;
typedef mqbnet::MultiRequestManager<Mes, Mes>      MultiReqManagerType;
typedef bsl::shared_ptr<MultiReqManagerType>       MultiReqManagerTypeSp;
typedef MultiReqManagerType::RequestContextType    ReqContextType;
typedef MultiReqManagerType::RequestContextSp      ReqContextSp;
typedef ReqContextType::NodeResponsePair           NodeResponse;
typedef ReqContextType::NodeResponsePairs          NodeResponses;
typedef ReqContextType::NodeResponsePairsConstIter NodeResponsesIt;
typedef ReqContextType::ResponseCb                 ResponseCb;
typedef ReqContextType::Nodes                      Nodes;
typedef Nodes::iterator                            NodesIt;
typedef ReqManagerType::RequestType                Req;
typedef ReqManagerType::RequestSp                  ReqSp;
typedef bsl::vector<ReqSp>                         ReqVec;
typedef bmqp_ctrlmsg::ControlMessageChoice         ReqChoice;
typedef bsl::unordered_set<ReqChoice>              ReqChoiceSet;
typedef bsl::shared_ptr<bmqio::TestChannel>        ChannelSp;

const bsls::TimeInterval SEND_REQUEST_TIMEOUT(30);
const bsls::Types::Int64 WATERMARK = 64 * 1024 * 1024;

}  // close unnamed namespace

/// Set the specified `called` flag to true and check that it has not
/// been set before.  Also check that the specified `context` is not
/// null
struct Caller {
    static void callback(bool* called, const ReqContextSp& context)
    {
        BMQTST_ASSERT(!*called);
        *called = true;
        BMQTST_ASSERT(context);
    }
};

class TestContext {
  private:
    bdlbb::PooledBlobBufferFactory d_blobBufferFactory;
    // Buffer factory provided to the various builders

    /// Blob shared pointer pool used in event builders.
    bmqp::BlobPoolUtil::BlobSpPoolSp d_blobSpPool_sp;

    ReqManagerTypeSp d_requestManager;
    // RequestManager object under testing

    MultiReqManagerTypeSp d_multiRequestManager;
    // MultiRequestManager object under testing

    ReqContextSp d_requestContextSp;
    // MultiRequestManagerRequestContext object used to collect responses from
    // the nodes

    bslma::ManagedPtr<mqbmock::Cluster> d_cluster_mp;
    // Cluster object used for generating mock nodes

    Nodes d_nodes;
    // Cluster nodes used to send them request by MultiRequestManager

    bmqu::TempDirectory d_tempDir;
    // Temp directory needed for Cluster object

    bslma::Allocator* d_allocator_p;
    // Allocator to use

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TestContext, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct TestContext object with the specified `nodesCount` and the
    /// optionally specified `allocator`.
    explicit TestContext(int nodesCount, bslma::Allocator* allocator = 0);

    /// Destructor of TestContext object.  Stop the scheduler and cancel all
    /// the requests.
    ~TestContext();

    // ACCESSORS

    /// Return pointer to the used allocator.
    bslma::Allocator* allocator() const;

    /// Return shared pointer to the MultiRequestManagerRequestContext
    /// object.
    ReqContextSp& context();

    /// Return reference to the buffer factory
    bdlbb::PooledBlobBufferFactory& factory();

    /// Return shared pointer to the RequestManager object
    ReqManagerTypeSp manager();

    /// Return cluster nodes.
    Nodes& nodes();

    // MANIPULATORS

    /// Advance time with the specified `amount`
    void advanceTime(bsls::TimeInterval amount);

    /// Cancel all pending requests
    void cancelRequests();

    /// Create new request object and return shared pointer to it.
    ReqSp createRequest();

    /// Create new response object with the specified `id` and return it.
    Mes createResponse(int id);

    /// Create new response object with CANCEL status and return it.
    Mes createResponseCancel();

    /// Return last message, sent by RequestManager, stored in the specified
    /// `channel`.
    Mes getNextRequest(const ChannelSp& channel);

    /// Send the specified `request` to the tested MultiRequestManager
    /// object.
    void sendRequest(const ReqSp& request);

    /// Set the specified `cb` as callback, that will be called after
    /// collecting responses from all nodes.
    void setResponseCb(const ResponseCb& cb);

    // STATIC CLASS METHODS

    /// Return vector of nodes of the specified `cluster`. Use the specified
    /// `allocator` to create the vector.
    static Nodes clusterNodes(mqbmock::Cluster* cluster,
                              bslma::Allocator* allocator);

    /// Create vector of node configs, containing the specified `nodeCount`
    /// number of nodes with the specified `allocator`.
    static mqbmock::Cluster::ClusterNodeDefs
    generateNodeDefs(int nodesCount, bslma::Allocator* allocator);

    /// Set custom values to some fields of the specified `request` object
    /// from the optionally specified `uri`, the optionally specified
    /// `flags`, the optionally specified `qId`, the optionally specified
    /// `readCount`, the optionally specified `writeCount` and the
    /// optionally specified `adminCount`
    static void populateRequest(const ReqSp&        request,
                                const bsl::string&  uri   = "bmq://foo.bar",
                                bsls::Types::Uint64 flags = 0,
                                unsigned int        qId   = 0,
                                int                 readCount  = 0,
                                int                 writeCount = 1,
                                int                 adminCount = 0);
};

TestContext::TestContext(int nodesCount, bslma::Allocator* allocator)
: d_blobBufferFactory(1024, allocator)
, d_blobSpPool_sp(
      bmqp::BlobPoolUtil::createBlobPool(&d_blobBufferFactory,
                                         bmqtst::TestHelperUtil::allocator()))
, d_requestManager(0)
, d_multiRequestManager(0)
, d_requestContextSp(0)
, d_cluster_mp(0)
, d_nodes(allocator)
, d_tempDir(allocator)
, d_allocator_p(allocator)
{
    mqbmock::Cluster::ClusterNodeDefs defs = generateNodeDefs(nodesCount,
                                                              d_allocator_p);

    d_cluster_mp.load(new (*d_allocator_p)
                          mqbmock::Cluster(&d_blobBufferFactory,
                                           d_allocator_p,
                                           true,   // isClusterMember
                                           false,  // isLeader
                                           false,  // isCSLMode
                                           false,  // isFSMWorkflow
                                           false,  // doesFSMwriteQLIST
                                           defs,
                                           "testCluster",
                                           d_tempDir.path()),
                      d_allocator_p);

    d_nodes = clusterNodes(d_cluster_mp.get(), d_allocator_p);

    d_requestManager = bsl::make_shared<ReqManagerType>(
        bmqp::EventType::e_CONTROL,
        d_blobSpPool_sp.get(),
        &d_cluster_mp->_scheduler(),
        false,  // lateResponseMode
        d_allocator_p);

    d_multiRequestManager = bsl::make_shared<MultiReqManagerType>(
        d_requestManager.get(),
        d_allocator_p);

    d_requestContextSp = d_multiRequestManager->createRequestContext();
    d_requestContextSp->setDestinationNodes(d_nodes);

    bmqsys::Time::shutdown();
    bmqsys::Time::initialize(
        bdlf::BindUtil::bind(&mqbmock::Cluster::getTime, d_cluster_mp.get()),
        bdlf::BindUtil::bind(&mqbmock::Cluster::getTime, d_cluster_mp.get()),
        bdlf::BindUtil::bind(&mqbmock::Cluster::getTimeInt64,
                             d_cluster_mp.get()));

    int rc = d_cluster_mp->_scheduler().start();
    BMQTST_ASSERT_EQ(rc, 0);
}

/// Set dummy callback to prevent bad_function_call for case if it has not
/// been set before.
TestContext::~TestContext()
{
    bool called = false;
    d_requestContextSp->setResponseCb(
        bdlf::BindUtil::bind(&Caller::callback,
                             &called,
                             bdlf::PlaceHolders::_1));
    cancelRequests();
    d_cluster_mp->_scheduler().stop();
}

// ACCESSORS
bslma::Allocator* TestContext::allocator() const
{
    return d_allocator_p;
}

ReqContextSp& TestContext::context()
{
    return d_requestContextSp;
}

bdlbb::PooledBlobBufferFactory& TestContext::factory()
{
    return d_blobBufferFactory;
}

ReqManagerTypeSp TestContext::manager()
{
    return d_requestManager;
}

Nodes& TestContext::nodes()
{
    return d_nodes;
}

// MANIPULATORS
void TestContext::advanceTime(bsls::TimeInterval amount)
{
    d_cluster_mp->advanceTime(amount.seconds());
}

void TestContext::cancelRequests()
{
    Mes reason = createResponseCancel();
    d_requestManager->cancelAllRequests(reason);
}

ReqSp TestContext::createRequest()
{
    return d_requestManager->createRequest();
}

Mes TestContext::createResponse(int id)
{
    Mes                   response(d_allocator_p);
    bmqp_ctrlmsg::Status& status = response.choice().makeStatus();
    status.category()            = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    status.code()                = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    status.message()             = bsl::string("Test", d_allocator_p);
    response.rId()               = id;

    return response;
}

Mes TestContext::createResponseCancel()
{
    Mes                   reason(d_allocator_p);
    bmqp_ctrlmsg::Status& status = reason.choice().makeStatus();
    status.category()            = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
    status.code()                = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
    status.message()             = bsl::string("Test", d_allocator_p);

    return reason;
}

Mes TestContext::getNextRequest(const ChannelSp& channel)
{
    BMQTST_ASSERT(channel->waitFor(1, true, bsls::TimeInterval(1)));
    bmqio::TestChannel::WriteCall wc = channel->popWriteCall();
    bmqp::Event                   ev(&wc.d_blob, d_allocator_p);
    BMQTST_ASSERT(ev.isControlEvent());
    Mes controlMessage(d_allocator_p);
    BMQTST_ASSERT_EQ(0, ev.loadControlEvent(&controlMessage));
    return controlMessage;
}

void TestContext::sendRequest(const ReqSp& request)
{
    d_requestContextSp->request() = request->request();
    d_multiRequestManager->sendRequest(d_requestContextSp,
                                       SEND_REQUEST_TIMEOUT);
}

void TestContext::setResponseCb(const ResponseCb& cb)
{
    d_requestContextSp->setResponseCb(cb);
}

// STATIC CLASS METHODS
Nodes TestContext::clusterNodes(mqbmock::Cluster* cluster,
                                bslma::Allocator* allocator)
{
    Nodes nodes(allocator);
    nodes.reserve(cluster->netCluster().nodes().size());
    for (mqbnet::Cluster::NodesList::iterator iter =
             cluster->netCluster().nodes().begin();
         iter != cluster->netCluster().nodes().end();
         ++iter) {
        nodes.push_back(*iter);
    }
    return nodes;
}

mqbmock::Cluster::ClusterNodeDefs
TestContext::generateNodeDefs(int nodesCount, bslma::Allocator* allocator)
{
    BMQTST_ASSERT_GT(nodesCount, 0);
    mqbmock::Cluster::ClusterNodeDefs clusterNodeDefs(allocator);
    for (int i = 0; i < nodesCount; ++i) {
        bsl::ostringstream nodeName(bmqtst::TestHelperUtil::allocator());
        nodeName << "testNode" << i;
        mqbc::ClusterUtil::appendClusterNode(
            &clusterNodeDefs,
            nodeName.str(),
            "US-EAST",
            41234 + i,
            mqbmock::Cluster::k_LEADER_NODE_ID + i,
            allocator);
    }
    return clusterNodeDefs;
}

void TestContext::populateRequest(const ReqSp&        request,
                                  const bsl::string&  uri,
                                  bsls::Types::Uint64 flags,
                                  unsigned int        qId,
                                  int                 readCount,
                                  int                 writeCount,
                                  int                 adminCount)
{
    bmqp_ctrlmsg::OpenQueue& req = request->request().choice().makeOpenQueue();

    bmqp_ctrlmsg::QueueHandleParameters params(
        bmqtst::TestHelperUtil::allocator());

    bmqt::QueueFlagsUtil::setWriter(&flags);
    bmqt::QueueFlagsUtil::setAck(&flags);

    params.uri()        = uri;
    params.flags()      = flags;
    params.qId()        = qId;
    params.readCount()  = readCount;
    params.writeCount() = writeCount;
    params.adminCount() = adminCount;

    req.handleParameters() = params;
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_contextTest()
// ------------------------------------------------------------------------
// Testing:
//    MultiRequestManagerRequestContext
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CONTEXT TEST");

    {
        ReqContextSp reqContext;
        BMQTST_ASSERT_PASS(reqContext = bsl::make_shared<ReqContextType>(
                               bmqtst::TestHelperUtil::allocator()));

        ReqSp request = bsl::make_shared<Req>(
            bmqtst::TestHelperUtil::allocator());
        TestContext::populateRequest(request);
        BMQTST_ASSERT_PASS(reqContext->request() = request->request());

        mqbmock::Cluster::ClusterNodeDefs defs = TestContext::generateNodeDefs(
            5,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::PooledBlobBufferFactory blobBufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bmqu::TempDirectory tempDir(bmqtst::TestHelperUtil::allocator());
        mqbmock::Cluster    cluster(&blobBufferFactory,
                                 bmqtst::TestHelperUtil::allocator(),
                                 true,   // isClusterMember
                                 false,  // isLeader
                                 false,  // isCSLMode
                                 false,  // isFSMWorkflow
                                 false,  // doesFSMwriteQLIST
                                 defs,
                                 "testCluster",
                                 tempDir.path());
        Nodes               nodes = TestContext::clusterNodes(
            &cluster,
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_PASS(reqContext->setDestinationNodes(nodes));
        NodeResponses responses = reqContext->response();
        BMQTST_ASSERT_EQ(nodes.size(), responses.size());
        NodesIt         nodesIt     = nodes.begin();
        NodeResponsesIt responsesIt = responses.begin();
        for (; nodesIt != nodes.end() && responsesIt != responses.end();
             ++nodesIt, ++responsesIt) {
            BMQTST_ASSERT_EQ(*nodesIt, responsesIt->first);
        }

        reqContext->clear();
        // Check that the MultiRequestManagerRequestContext object returned to
        // the default state
        BMQTST_ASSERT_EQ(reqContext->request(),
                         Mes(bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT(reqContext->response().empty());
    }
}

static void test2_creatorsTest()
// ------------------------------------------------------------------------
// Testing:
//    MultiRequestManager::MultiRequestManager()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CREATORS TEST");

    {
        // Null RequestManager pointer
        BMQTST_ASSERT_SAFE_FAIL(
            MultiReqManagerType(NULL, bmqtst::TestHelperUtil::allocator()));
    }

    {
        // Success creation
        TestContext context(5, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_PASS(
            MultiReqManagerType(context.manager().get(),
                                bmqtst::TestHelperUtil::allocator()));
    }
}

static void test3_sendRequestTest()
// ------------------------------------------------------------------------
// Testing:
//    MultiRequestManager::sendRequest()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEND REQUEST TEST");

    {
        // Send one request and check it was delivered
        TestContext context(5, bmqtst::TestHelperUtil::allocator());

        ReqSp req = context.createRequest();
        context.populateRequest(req);
        context.sendRequest(req);

        NodesIt it = context.nodes().begin();
        for (; it != context.nodes().end(); ++it) {
            ChannelSp ch = bsl::dynamic_pointer_cast<bmqio::TestChannel>(
                (*it)->channel().channel());
            BMQTST_ASSERT(ch);
            // checking that MultiRequestManager has really sent the requests
            Mes controlMessage = context.getNextRequest(ch);
            BMQTST_ASSERT_EQ(req->request().choice(), controlMessage.choice());
        }
    }
}

static void test4_handleResponseTest()
// ------------------------------------------------------------------------
// Testing:
//    MultiRequestManager::response()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HANDLE RESPONSE TEST");

    {
        // Successfully receive responses from all the nodes
        TestContext context(5, bmqtst::TestHelperUtil::allocator());
        ReqSp       req = context.createRequest();
        context.populateRequest(req);
        bool called = false;
        context.setResponseCb(bdlf::BindUtil::bind(&Caller::callback,
                                                   &called,
                                                   bdlf::PlaceHolders::_1));
        context.sendRequest(req);

        bsl::vector<Mes> responses(bmqtst::TestHelperUtil::allocator());
        responses.reserve(context.nodes().size());
        NodesIt it = context.nodes().begin();
        for (; it != context.nodes().end(); ++it) {
            ChannelSp ch = bsl::dynamic_pointer_cast<bmqio::TestChannel>(
                (*it)->channel().channel());
            BMQTST_ASSERT(ch);
            // checking that MultiRequestManager has really sent the requests
            Mes controlMessage = context.getNextRequest(ch);
            responses.emplace_back(
                context.createResponse(controlMessage.rId().value()));
        }

        BMQTST_ASSERT(!called);
        bsl::vector<Mes>::const_iterator rIt = responses.begin();
        for (; rIt != responses.end(); ++rIt) {
            context.manager()->processResponse(*rIt);
        }
        BMQTST_ASSERT(called);

        const NodeResponses& nodeResponses = context.context()->response();
        NodeResponsesIt      nodeIt        = nodeResponses.begin();
        rIt                                = responses.begin();
        for (; nodeIt != nodeResponses.end() && rIt != responses.end();
             ++nodeIt, ++rIt) {
            BMQTST_ASSERT_EQ(nodeIt->second, *rIt);
        }
    }

    {
        // No responses from the nodes
        TestContext context(5, bmqtst::TestHelperUtil::allocator());
        ReqSp       req = context.createRequest();
        context.populateRequest(req);
        bool called = false;
        context.setResponseCb(bdlf::BindUtil::bind(&Caller::callback,
                                                   &called,
                                                   bdlf::PlaceHolders::_1));
        context.sendRequest(req);

        BMQTST_ASSERT(!called);
        context.advanceTime(SEND_REQUEST_TIMEOUT);
        BMQTST_ASSERT(called);
    }

    {
        // Concurrency test.  Create a lot of nodes and process their responses
        // from different threads.
        bslmt::Mutex       responsesLock;
        MesQue             responses(bmqtst::TestHelperUtil::allocator());
        const int          nodesCount   = 500;
        const int          threadsCount = 10;
        bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());
        bslmt::Barrier     barrier(threadsCount + 1);

        /// Take one response from the specified `responses` queue and
        /// processes it by the specified `context`.  The specified
        /// `responsesLock` is used to sync access to queue from
        /// different threads.  The specified `barrier` is used to start
        /// execution of all the threads simultaneously.
        struct ConcurrentCaller {
            static void processResponsesJob(MesQue*         responses,
                                            bslmt::Mutex*   responsesLock,
                                            bslmt::Barrier* barrier,
                                            TestContext*    context)
            {
                barrier->wait();
                while (true) {
                    bsl::optional<Mes> response;
                    {
                        bslmt::LockGuard<bslmt::Mutex> lock(responsesLock);
                        if (!responses->empty()) {
                            response.emplace(responses->front());
                            responses->pop();
                        }
                    }
                    if (response.has_value()) {
                        int rc = context->manager()->processResponse(
                            response.value());
                        BMQTST_ASSERT_EQ(rc, 0);
                    }
                    else {
                        break;
                    }
                }
            }
        };

        TestContext context(nodesCount, bmqtst::TestHelperUtil::allocator());
        ReqSp       req = context.createRequest();
        context.populateRequest(req);
        bool called = false;
        context.setResponseCb(bdlf::BindUtil::bind(&Caller::callback,
                                                   &called,
                                                   bdlf::PlaceHolders::_1));
        context.sendRequest(req);

        NodesIt it = context.nodes().begin();
        for (; it != context.nodes().end(); ++it) {
            ChannelSp ch = bsl::dynamic_pointer_cast<bmqio::TestChannel>(
                (*it)->channel().channel());
            BMQTST_ASSERT(ch);
            // checking that MultiRequestManager has really sent the requests
            Mes controlMessage = context.getNextRequest(ch);
            responses.emplace(
                context.createResponse(controlMessage.rId().value()));
        }

        // Start the threads to process responses from queue
        for (bsl::size_t i = 0; i < threadsCount; ++i) {
            int rc = threadGroup.addThread(
                bdlf::BindUtil::bind(&ConcurrentCaller::processResponsesJob,
                                     &responses,
                                     &responsesLock,
                                     &barrier,
                                     &context));
            BMQTST_ASSERT_EQ(0, rc);
        }

        BMQTST_ASSERT(!called);
        barrier.wait();
        threadGroup.joinAll();
        BMQTST_ASSERT(called);
    }
}

// ============================================================================
//                                MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize(bmqtst::TestHelperUtil::allocator());
    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 4: test4_handleResponseTest(); break;
    case 3: test3_sendRequestTest(); break;
    case 2: test2_creatorsTest(); break;
    case 1: test1_contextTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqsys::Time::shutdown();
    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);  // RETURN
    // Default: EventQueue uses bmqex::BindUtil::bindExecute(), which uses
    //          default allocator.
}
