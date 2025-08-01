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

// mqbblp_rootqueueengine.t.cpp                                       -*-C++-*-
#include <mqbblp_rootqueueengine.h>

// MQB
#include <mqbblp_queueenginetester.h>
#include <mqbblp_queueengineutil.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>
#include <mqbmock_queuehandle.h>
#include <mqbstat_brokerstats.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_routingconfigurationutils.h>

// BDE
#include <bdlb_bitutil.h>
#include <bdlb_tokenizer.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmf_assert.h>
#include <bslmt_semaphore.h>
#include <bsls_timeinterval.h>

#include <bmqtst_scopedlogobserver.h>
#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_algorithm.h>
#include <bsl_functional.h>
#include <bsl_ios.h>
#include <bsl_limits.h>
#include <bsl_map.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

const mqbmock::QueueHandle* k_nullMockHandle_p = 0;

mqbconfm::Domain broadcastConfig()
{
    mqbconfm::Domain domainConfig(bmqtst::TestHelperUtil::allocator());
    domainConfig.mode().makeBroadcast();
    domainConfig.storage().config().makeInMemory();
    return domainConfig;
}

/// Represents the expected consumer status at any given point in time
/// according to our model.
struct ConsumerStatus {
    /// This holds the handle for this consumer.  It will be null before a
    /// subscription starts.
    mqbmock::QueueHandle* handle;

    /// It keeps track of weather the consumer is able to consume messages
    /// or not (e.g. in case of high watermark).
    bool active;

    /// It keeps track of the priority of the consumer.
    int priority;

    /// It keeps track of the expected number of messages we should have for
    /// this consumer at any given point in time.
    int expectedCount;

    /// Creates a new `ConsumerStatus` object with default null values.  The
    /// consumer is Active by default.
    ConsumerStatus()
    : handle(0)
    , active(true)
    , priority(1)
    , expectedCount(0)
    {
    }
};

/// This is the main model for our queueing system.  Tracks the status of
/// each consumer while operations get applied to the device under tests.
typedef bsl::map<bsl::string, ConsumerStatus> ActiveConsumersModel;

/// Null on success or error string on error.
typedef bdlb::NullableValue<bsl::string> MaybeError;

/// A class that specifies an invariant that should apply while operations
/// are applied to both the model and the device under test.
struct Invariant {
    /// The type of invariant clause.  Should return null `MaybeError` if
    /// the invariant holds true or a `MaybeError` with a descriptive error
    /// message if the invariant fails.
    typedef bsl::function<MaybeError(const ActiveConsumersModel&      model,
                                     const mqbblp::QueueEngineTester& tester)>
        Clause;

    /// Creates a new `Invariant` with the specified `name` and `clause`.
    Invariant(const bsl::string& name, const Clause& clause)
    : d_name(name)
    , d_clause(clause)
    {
    }

    /// The name of this invariant.  Used to form error messages if it
    /// fails.
    bsl::string d_name;

    /// A function that expresses the invariant clause.
    Clause d_clause;
};

/// A vector of `Invariant`s.
typedef bsl::vector<Invariant> Invariants;

/// A base-class for operations that apply to the model and the device under
/// test.
struct Operation {
    /// Applies the actual operation using the specified `model` and
    /// `tester`.
    virtual MaybeError operator()(ActiveConsumersModel*      model,
                                  mqbblp::QueueEngineTester* tester) const = 0;

    virtual ~Operation() {}

    /// Prints this object to the specified `os` stream.
    virtual void print(bsl::ostream& os) const = 0;

    /// Streams the specified `me` object to the specified `os` stream by
    /// using the `virtual` `print()` method.
    friend bsl::ostream& operator<<(bsl::ostream& os, const Operation& me)
    {
        me.print(os);
        return os;
    }
};

/// An `Operation` shared pointer.
typedef bsl::shared_ptr<Operation> OperationSp;

/// A vector with `Operation` shared pointers.
typedef bsl::vector<OperationSp> Operations;

/// Prints the specified `p` operation pointer to the specified `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const OperationSp& p)
{
    if (p) {
        return stream << *p;
    }
    else {
        return stream << "NULL";
    }
}

/// An operation that checks the specified `invariants` against the
/// specified `model` and `tester`.  In case of error, the specified `op` is
/// used along with other data to form a descriptive error message.  Returns
/// null `MaybeError` on success or one holding an error message on failure.
MaybeError checkInvariants(const Invariants&                invariants,
                           const ActiveConsumersModel&      model,
                           const mqbblp::QueueEngineTester& tester,
                           const Operation&                 op)
{
    for (Invariants::const_iterator i = invariants.begin();
         i != invariants.end();
         ++i) {
        const MaybeError rv = i->d_clause(model, tester);
        if (!rv.isNull()) {
            bmqu::MemOutStream out;
            out << "invariant \"" << i->d_name
                << "\" failed after operation \"" << op << "\", "
                << rv.value();
            return MaybeError(out.str());
        }
    }
    return MaybeError();
}

// ============================================================================
//                    IMPLEMENTATION OF OPERATIONS HIERARCHY
// ----------------------------------------------------------------------------

/// An empty default no-op `Operation`. Helps with printouts.
struct Start : public Operation {
    /// Applies the actual operation using the specified `model` and
    /// `tester`.
    MaybeError operator()(BSLA_UNUSED ActiveConsumersModel* model,
                          BSLA_UNUSED mqbblp::QueueEngineTester* tester) const
        BSLS_KEYWORD_OVERRIDE
    {
        return MaybeError();
    }

    // ACCESSORS

    /// Prints this object to the specified `os` stream.
    void print(bsl::ostream& os) const BSLS_KEYWORD_OVERRIDE { os << "Start"; }
};

/// An operation that targets a specific client.
struct ClientOperation : public Operation {
    // TYPES

    /// An iterator type.
    typedef ActiveConsumersModel::iterator Iterator;

    // DATA

    /// The client for this operation.
    const char* d_client;

    // CREATORS

    /// Creates a new `ClientOperation` targeting the specified `client`.
    ClientOperation(const char* client)
    : d_client(client)
    {
    }

    // ACCESSORS

    /// Queries the specified `model` for the `client` of this operation.
    /// Will return `model->end()` on failure or valid `Iterator` on
    /// success.
    Iterator findConsumer(ActiveConsumersModel* model) const
    {
        return model->find(d_client);
    }
};

/// Models the subscription operation.
struct Subscribe : public ClientOperation {
    // DATA

    /// Argument passed to `getHandle()`
    const int d_readCount;

    /// Argument passed to `configureHandle()`
    const int d_consumerPriority;

    /// Argument passed to `configureHandle()`
    const int d_consumerPriorityCount;

    // CREATORS

    /// Creates a new `Subscribe` targeting the specified `client` and
    /// using the specified `readCount` while opening.  Then, configure
    /// the client handle with the specified `consumerPriority` and
    /// `consumerPriorityCount`.  If `consumerPriorityCount` == -1, it is
    /// set to be equal to `readCount`.
    Subscribe(const char* client,
              const int   readCount,
              const int   consumerPriority      = 1,
              const int   consumerPriorityCount = -1)
    : ClientOperation(client)
    , d_readCount(readCount)
    , d_consumerPriority(consumerPriority)
    , d_consumerPriorityCount(
          consumerPriorityCount == -1 ? readCount : consumerPriorityCount)
    {
        BSLS_ASSERT_OPT(d_consumerPriorityCount <= d_readCount);
    }

    // MANIPULATORS
    MaybeError
    operator()(ActiveConsumersModel*      model,
               mqbblp::QueueEngineTester* tester) const BSLS_KEYWORD_OVERRIDE
    {
        // Applies the actual operation using the specified 'model' and
        // 'tester'.
        {
            bmqu::MemOutStream out;
            out << d_client << " readCount=" << d_readCount;
            ConsumerStatus& status = (*model)[d_client];
            status.handle          = tester->getHandle(out.str());
            status.priority        = d_consumerPriority;
        }
        {
            bmqu::MemOutStream out;
            out << d_client << " consumerPriority=" << d_consumerPriority
                << " consumerPriorityCount=" << d_consumerPriorityCount;
            tester->configureHandle(out.str());
        }
        return MaybeError();
    }

    // ACCESSORS

    /// Prints this object to the specified `os` stream.
    void print(bsl::ostream& os) const BSLS_KEYWORD_OVERRIDE
    {
        os << "Subscribe(" << d_client << ", " << d_readCount << ")";
    }
};

/// Models the unsubscription operation.
struct UnSubscribe : public ClientOperation {
    // CREATORS

    /// Creates a new `UnSubscribe`.
    UnSubscribe(const char* client)
    : ClientOperation(client)
    {
    }

    // MANIPULATORS
    MaybeError
    operator()(ActiveConsumersModel*      model,
               mqbblp::QueueEngineTester* tester) const BSLS_KEYWORD_OVERRIDE
    {
        // Applies the actual operation using the specified 'model' and
        // 'tester'.

        const Iterator iter = findConsumer(model);
        if (iter != model->end()) {
            // Clear any unconfirmed messages for this handle.  This isn't
            // necessary with the current queue implementation, but given that
            // our 'MockQueueHandle's store unconfirmed messages, a call to
            //
            ConsumerStatus& status = iter->second;
            BMQTST_ASSERT(status.handle);
            status.handle->_resetUnconfirmed();

            // Don't unsubscribe someone you haven't subscribed yet,
            // because it will segv due to assertion in
            // 'QueueEngineTester::dropHandle()'
            tester->dropHandle(d_client);
            model->erase(iter);
        }
        return MaybeError();
    }

    // ACCESSORS

    /// Prints this object to the specified `os` stream.
    void print(bsl::ostream& os) const BSLS_KEYWORD_OVERRIDE
    {
        os << "UnSubscribe(" << d_client << ")";
    }
};

/// Models the `_setCanDeliver()` (high watermark) operation.
struct SetCanDeliver : public ClientOperation {
    // DATA

    /// The status to set.
    const bool d_status;

    // CREATORS

    /// Creates a new `SetCanDeliver` targeting the specified `client` and
    /// setting the value to the specified `status`.
    SetCanDeliver(const char* client, const bool status)
    : ClientOperation(client)
    , d_status(status)
    {
    }

    // MANIPULATORS
    MaybeError operator()(ActiveConsumersModel* model,
                          BSLA_UNUSED mqbblp::QueueEngineTester* tester) const
        BSLS_KEYWORD_OVERRIDE
    {
        // Applies the actual operation using the specified 'model' and
        // 'tester'.

        const Iterator iter = findConsumer(model);
        if (iter != model->end()) {
            ConsumerStatus& status = iter->second;
            BMQTST_ASSERT(status.handle);

            // If we run without being subscribed, it will segv in
            // '_setCanDeliver()' where it tries to reference 'queueEngine()'.
            status.handle->_setCanDeliver(d_status);
            status.active = d_status;
        }
        return MaybeError();
    }

    // ACCESSORS

    /// Prints this object to the specified `os` stream.
    void print(bsl::ostream& os) const BSLS_KEYWORD_OVERRIDE
    {
        os << "SetCanDeliver(" << d_client << ", " << bsl::boolalpha
           << d_status << ")";
    }
};

/// Models a post message to the queue operation.
struct Post : public Operation {
    // DATA

    /// The value to post.
    const int d_value;

    // CREATORS

    /// Creates a new `Post`, posting the specified `value` to the queue.
    Post(const int value)
    : d_value(value)
    {
    }

    // MANIPULATORS
    MaybeError
    operator()(ActiveConsumersModel*      model,
               mqbblp::QueueEngineTester* tester) const BSLS_KEYWORD_OVERRIDE
    {
        // Applies the actual operation using the specified 'model' and
        // 'tester'.

        // Does post and wait.
        bmqu::MemOutStream out;
        out << d_value;
        tester->post(out.str());
        tester->afterNewMessage(1);

        // Compute the current highest priority
        int highestPriority = bsl::numeric_limits<int>::min();
        for (ActiveConsumersModel::iterator iter = model->begin();
             iter != model->end();
             ++iter) {
            ConsumerStatus& status = iter->second;

            if (status.priority > highestPriority) {
                highestPriority = status.priority;
            }
        }

        // Update the expected counters
        for (ActiveConsumersModel::iterator iter = model->begin();
             iter != model->end();
             ++iter) {
            ConsumerStatus& status = iter->second;

            if (status.active && status.priority == highestPriority) {
                ++status.expectedCount;
            }
        }
        return MaybeError();
    }

    // ACCESSORS

    /// Prints this object to the specified `os` stream.
    void print(bsl::ostream& os) const BSLS_KEYWORD_OVERRIDE
    {
        os << "Post(" << d_value << ")";
    }
};

MaybeError
checkReceivedForConsumers(const ActiveConsumersModel& model,
                          BSLA_UNUSED const mqbblp::QueueEngineTester& tester)
{
    // Check, using the specified model and tester, that everyone who is
    // currently active, has the expected number of messages.  Returns 'null'
    // 'MaybeError' for success or a 'MaybeError' containint the error message
    // otherwise.

    for (ActiveConsumersModel::const_iterator iter = model.begin();
         iter != model.end();
         ++iter) {
        const ConsumerStatus& status = iter->second;
        mqbmock::QueueHandle* handle = status.handle;
        BMQTST_ASSERT(handle);
        if (handle->_numMessages() != status.expectedCount) {
            bmqu::MemOutStream out;
            out << "actual messages (" << handle->_numMessages()
                << ") != expected messages(" << status.expectedCount << ")"
                << " for client \"" << iter->first << "\"";
            if (handle->_numMessages()) {
                out << " pending messages: [" << handle->_messages() << "]";
            }
            return MaybeError(out.str());
        }
    }
    return MaybeError();
}

/// Creates a testbench and a model and applies the specified `operations`
/// in any possible permuted order, while checking the specified
/// `invariants`.  It can jump directly to the optionally specified `skipTo`
/// permutation and limit the number of permutations it check up to the
/// optional specified `limit`.
void regress(Operations*       operations,
             const Invariants& invariants,
             const int         skipTo = -1,
             const int         limit  = INT_MAX)
{
    int count = 0;
    while (count <= skipTo - 1) {
        // If 'skipTo' is set, then create permutations till you reach the
        // desired state.
        bsl::next_permutation(operations->begin(), operations->end());
        ++count;
    }

    do {
        // Print the iteration so that you can quickly 'skipTo' if you need
        // to debug somethign.
        PVV(L_ << ": Iteration: " << count);

        // Setup testbench with proper tester infrastructure and a model.
        mqbblp::QueueEngineTester tester(broadcastConfig(),
                                         false,  // start scheduler
                                         bmqtst::TestHelperUtil::allocator());

        mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

        ActiveConsumersModel model;

        // Do the first check of invariants.
        MaybeError rv = checkInvariants(invariants, model, tester, Start());
        if (rv.isNull()) {
            // For each specified operation...
            for (Operations::const_iterator iter = operations->begin();
                 iter != operations->end();
                 ++iter) {
                const Operation& op = **iter;

                // Apply the operation...
                rv = op(&model, &tester);
                if (!rv.isNull()) {
                    break;
                }

                // Check the invariants again...
                rv = checkInvariants(invariants, model, tester, op);
                if (!rv.isNull()) {
                    break;
                }
            }
        }

        // If there was an error, print the error message with debug
        // information.
        if (!rv.isNull()) {
            PVV(L_ << ": Failed on sequence: "
                   << bmqu::PrintUtil::printer(*operations) << " "
                   << rv.value());
            BMQTST_ASSERT(false);
            return;  // RETURN
        }

        // Is this the last iteration according to the 'limit'?
        if (++count >= (skipTo + limit)) {
            break;
        }

        // Permute 'operations' and if we aren't back in the first one, repeat
        // another experiment.
    } while (bsl::next_permutation(operations->begin(), operations->end()));
}

/// Populate the specified `strings` with messages parsed from the
/// specified `str`.  The format of `str` must be:
///   `[s_1],[s_2],...,[s_N]`
///
/// The behavior is undefined unless `str` is formatted as above.  Note that
/// `strings` will be cleared.
static void parseStrings(bsl::vector<bsl::string>* strings, bsl::string str)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(strings);

    strings->clear();

    bdlb::Tokenizer tokenizer(str, " ", ",");

    BSLS_ASSERT_OPT(tokenizer.isValid() && "Format error in 'str'");

    while (tokenizer.isValid()) {
        strings->push_back(tokenizer.token());
        ++tokenizer;
    }
}

/// Return a fanout domain configured with appIds from the specified
/// `appIds`.  The format of `appIdsStr` must be:
///  `[appId_1],[appId_2],...,[appIdN]`
///
/// The behavior is undefined unless `appIdsStr` is formatted as above.
mqbconfm::Domain fanoutConfig(const bsl::string& appIdsStr)
{
    mqbconfm::Domain domainConfig;
    domainConfig.mode().makeFanout();
    bsl::vector<bsl::string>& appIDs = domainConfig.mode().fanout().appIDs();
    parseStrings(&appIDs, appIdsStr);

    return domainConfig;
}

mqbconfm::Domain priorityDomainConfig()
{
    mqbconfm::Domain domainConfig;
    domainConfig.mode().makePriority();

    return domainConfig;
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                             BROADCAST TESTS
// ----------------------------------------------------------------------------

static void test1_broadcastBreathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//  1) 3 consumers
//  2) Post 3 messages to the queue, and invoke the engine to broadcast
//     them
//  3) Verify that every consumer received all messages
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    mqbblp::QueueEngineTester tester(broadcastConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1) 3 consumers
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");

    tester.configureHandle("C1 consumerPriority=-1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=-1 consumerPriorityCount=1");
    tester.configureHandle("C3 consumerPriority=-1 consumerPriorityCount=1");

    C2 = tester.getHandle("C2 readCount=2");
    tester.configureHandle("C2 consumerPriority=-1 consumerPriorityCount=2");

    // 2) Post 3 messages to the queue, and invoke the engine to broadcast
    //     them
    tester.post("1");
    tester.post("2");
    tester.post("3");

    tester.afterNewMessage(3);

    // 3) Verify that every consumer received all messages
    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    PVV(L_ << ": C3 Messages: " << C3->_messages());

    BMQTST_ASSERT_EQ(C1->_messages(), "1,2,3");
    BMQTST_ASSERT_EQ(C2->_messages(), "1,2,3");
    BMQTST_ASSERT_EQ(C3->_messages(), "1,2,3");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();
    C3->_resetUnconfirmed();

    // No confirm required. Actually confirm would assert fail.
}

static void test2_broadcastConfirmAssertFails()
// ------------------------------------------------------------------------
// CONFIRM ASSERT FAILS TEST
//
// Concerns:
//   Makes sure that an assertion fails if a message confirmation is
//   attempted.
//
// Plan:
//  1) 1 consumer
//  2) Post 1 message to the queue, and invoke the engine to broadcast it
//  3) Try to confirm the message
//  4) Assert that an assert failed
//
// Testing:
//   Calling confirm assert fails.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("CONFIRM ASSERT FAILS TEST");

    mqbblp::QueueEngineTester tester(broadcastConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1) 1 consumer
    BSLA_MAYBE_UNUSED mqbmock::QueueHandle* C1 = tester.getHandle(
        "C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=-1 consumerPriorityCount=1");

    // 2) Post 1 message to the queue, and invoke the engine to broadcast it
    tester.post("1");
    tester.afterNewMessage(1);

    // NOTE: Even if our 'onConfirmMessage()' doesn't abort, the mock
    //       infrastructure is.  Thus, some inspection on the cause of abort
    //       would also be useful.
    BMQTST_ASSERT_SAFE_FAIL(tester.confirm(
        "C1",
        mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(), "0")));
}

static void test3_broadcastCannotDeliver()
// ------------------------------------------------------------------------
// CANNOT DELIVER
//
// Concerns:
//   a) If it is not possible to deliver a message to a client, then it
//      gets ignored. There's no flow-control in broadcasting mode.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the same priority. Post 2
//      messages, deliver, and verify.
//   2) Disable delivery for C1.  Post 2 messages, deliver, and verify
//      that C2 received both messages (and C1 received none).
//   3) Disable delivery for C2.  Post 1 message, deliver, and verify that
//      neither C2 nor C1 received any messages.
//   4) Enable delivery for C1.  Verify that C1 did not receive the
//      message from step 3 above (and C2 received none).
//   5) Enable delivery for C2.  Verify that neither C1 nor C2 received any
//      messages.  Post 4 messages, deliver, and verify that both C1 and
//      C2 received all 4 messages.
//
// Testing:
//   Queue Engine delivery when it is not possible to deliver to one or
//   more highest priority consumers.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("CANNOT CONSUMERS");

    mqbblp::QueueEngineTester tester(broadcastConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    tester.configureHandle("C1 consumerPriority=0 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=0 consumerPriorityCount=1");

    // C1: free
    // C2: free
    tester.post("1,2");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_messages(), "1,2");
    BMQTST_ASSERT_EQ(C2->_messages(), "1,2");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // No confirm required. Actually confirm would assert fail.

    // 2) C1: Can't deliver
    C1->_setCanDeliver(false);

    // C1: busy
    // C2: free
    tester.post("3,4");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_messages(), "");
    BMQTST_ASSERT_EQ(C2->_messages(), "3,4");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // No confirm required. Actually confirm would assert fail.

    // 3) C2: Can't deliver
    C2->_setCanDeliver(false);

    // C1: busy
    // C2: busy
    tester.post("5");
    tester.afterNewMessage(1);

    BMQTST_ASSERT_EQ(C1->_messages(), "");
    BMQTST_ASSERT_EQ(C2->_messages(), "");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // 4) C1: Can deliver
    C1->_setCanDeliver(true);

    // C1: free
    // C2: busy
    BMQTST_ASSERT_EQ(C1->_messages(), "");
    BMQTST_ASSERT_EQ(C2->_messages(), "");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // No confirm required. Actually confirm would assert fail.

    // 5) C2: Can deliver
    C2->_setCanDeliver(true);

    BMQTST_ASSERT_EQ(C1->_messages(), "");
    BMQTST_ASSERT_EQ(C2->_messages(), "");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // C1: free
    // C2: free
    tester.post("6,7,8,9");
    tester.afterNewMessage(4);

    BMQTST_ASSERT_EQ(C1->_messages(), "6,7,8,9");
    BMQTST_ASSERT_EQ(C2->_messages(), "6,7,8,9");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // No confirm required. Actually confirm would assert fail.
}

static void test4_broadcastPostAfterResubscribe()
// ------------------------------------------------------------------------
// POST AFTER RESUBSCRIBE
//
// Concerns:
//   a) It tests that posts during non-subscription are discarded for
//      handler
//
// Plan:
//   1) Perform [Subscribe(C1, 1), UnSubscribe(C1), Post(1),
//      Subscribe(C2, 1), Post(2)]
//   2) At the end of this sequence, there should be only one message
//
// Testing:
//   Queue Engine delivery when we have re-subscription
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("POST AFTER RESUBSCRIBE");

    mqbblp::QueueEngineTester tester(broadcastConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // Subscribe(C1, 1)
    tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=0 consumerPriorityCount=1");

    // UnSubscribe(C1, 1)
    tester.dropHandle("C1");

    // Post(1) - Should be dropped
    tester.post("1");
    tester.afterNewMessage(1);

    // Subscribe(C1, 1)
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=0 consumerPriorityCount=1");

    // Post(1) - Should be dropped
    tester.post("2");
    tester.afterNewMessage(1);

    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);

    BMQTST_ASSERT_EQ(C2->_messages(), "2");
    C2->_resetUnconfirmed();

    // No confirm required. Actually confirm would assert fail.
}

static void test5_broadcastReleaseHandle_isDeletedFlag()
// ------------------------------------------------------------------------
// RELEASE HANDLE - IS-DELETED FLAG
//
// Concerns:
//   1. Ensure that releasing a handle fully (i.e., all counts go to zero
//      'isFinal=true' is explicitly specified) results in the handle being
//      deleted, as indicated by the 'isDeleted' flag propagated via the
//      'releaseHandle' callback invoation.
//
// Plan:
//   1. Bring up:
//      - C1 with writeCount=1
//      - C2 with readCount=2
//      - C3 with readCount=2
//   2. Release from C2 one reader but pass 'isFinal=true' and verify that
//      C2 was fully deleted.
//   3. Release from C1 one writer but pass 'isFinal=false' and verify that
//      C1 was fully deleted.
//   4. Release from C3 two readers and pass 'isFinal=true' (i.e. correct
//      scenario) and verify that C3 was fully deleted.
//
// Testing:
//  void releaseHandle(
//      mqbi::QueueHandle                                *handle,
//      const bmqp_ctrlmsg::QueueHandleParameters&        handleParameters,
//      bool                                              isFinal,
//      const mqbi::QueueHandle::HandleReleasedCallback&  releasedCb)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RELEASE HANDLE - IS-DELETED FLAG");

    mqbconfm::Domain domainConfig(bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTester tester(broadcastConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1. Bring up a consumer C1 with writeCount=1 and a consumer C2 with
    //    readCount=2.
    tester.getHandle("C1 writeCount=1");
    tester.getHandle("C2 readCount=2");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=2");
    tester.getHandle("C3 readCount=2");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=2");

    bool isDeleted = false;

    // 2. Release from C2 one reader but pass 'isFinal=true' and verify that
    //    C2 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C2 readCount=1 isFinal=true",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);

    // 3. Release from C1 one writer but pass 'isFinal=false' and verify that
    //    C1 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C1 writeCount=1 isFinal=false",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);

    // 4. Release from C3 two readers and pass 'isFinal=true' (i.e. correct
    //    scenario) and verify that C3 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C3 readCount=2 isFinal=true",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);
}

static void test6_broadcastDynamicPriorities()
// ------------------------------------------------------------------------
// DYNAMIC PRIORITIES
//
// Concerns:
//   a) Only the highest priority consumers receive messages when the
//      priorities are dynamically changing.
//
// Plan:
//   1) Configure 3 handles, C1, C2 and C3, with the same priority. Post 2
//      messages, deliver, and verify.
//   2) Lower the priority of C2.  Post 2 messages, deliver, and verify
//      that C1 and C3 received both messages (and C2 received none).
//   3) Configure C1 to be the single highest priority consumer.  Post 2
//      messages, deliver, and verify that only C1 received any messages.
//   4) Increases C2's priority to be the same as C1.  Post 4 messages,
//      deliver, and verify that C1 and C2 received all 4 messages (and C3
//      received none).
//
// Testing:
//   Queue Engine delivery when consumer priorities can change.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("DYNAMIC PRIORITIES");

    mqbblp::QueueEngineTester tester(broadcastConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");

    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=2 consumerPriorityCount=1");
    tester.configureHandle("C3 consumerPriority=2 consumerPriorityCount=1");

    tester.post("1,2");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_messages(), "1,2");
    BMQTST_ASSERT_EQ(C2->_messages(), "1,2");
    BMQTST_ASSERT_EQ(C3->_messages(), "1,2");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();
    C3->_resetUnconfirmed();

    // No confirm required. Actually confirm would assert fail.

    // 2) Lower the priority of C2.
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    tester.post("3,4");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_messages(), "3,4");
    BMQTST_ASSERT_EQ(C2->_messages(), "");
    BMQTST_ASSERT_EQ(C3->_messages(), "3,4");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();
    C3->_resetUnconfirmed();

    // 3) Configure C1 to be the single highest priority consumer.
    tester.configureHandle("C1 consumerPriority=99 consumerPriorityCount=1");

    tester.post("5,6");
    tester.afterNewMessage(1);

    BMQTST_ASSERT_EQ(C1->_messages(), "5,6");
    BMQTST_ASSERT_EQ(C2->_messages(), "");
    BMQTST_ASSERT_EQ(C3->_messages(), "");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();
    C3->_resetUnconfirmed();

    // 4) Increases C2's priority to be the same as C1.
    tester.configureHandle("C2 consumerPriority=99 consumerPriorityCount=1");

    tester.post("7,8,9,10");
    tester.afterNewMessage(4);

    BMQTST_ASSERT_EQ(C1->_messages(), "7,8,9,10");
    BMQTST_ASSERT_EQ(C2->_messages(), "7,8,9,10");
    BMQTST_ASSERT_EQ(C3->_messages(), "");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();
    C3->_resetUnconfirmed();
}

static void test7_broadcastPriorityFailover()
// ------------------------------------------------------------------------
// PRIORITY FAILOVER
//
// Concerns:
//   a) Only the new highest priority consumers might receive messages when
//      the current highest priority consumers either unsubscribe or become
//      busy.
//   b) If all the highest priority consumers are busy, no one will receive
//      messages.
//
// Plan:
//   1) Configure 3 handles, C1, C2 and C3, with increasing priority.  Post
//      2 messages, deliver, and verify that only C3 received the messages.
//   2) C3 becomes busy.  Post 1 message, deliver, and verify that only no
//      one received the message.
//   3) C3 unsubscribes.  Post 2 messages, deliver, and verify that only C2
//      received the messages.
//   4) C2 becomes busy.  Post 1 message, deliver, and verify that only no
//      one received the message.
//   5) C2 unsubscribes.  Post 2 messages, deliver, and verify that only C1
//      received the messages.
//
// Testing:
//   Queue Engine delivery when highest priority consumers either
//   unsubscribe or become busy gradually.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("PRIORITY FAILOVER");

    mqbblp::QueueEngineTester tester(broadcastConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=2 consumerPriorityCount=1");
    tester.configureHandle("C3 consumerPriority=3 consumerPriorityCount=1");

    tester.post("1,2");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_messages(), "");
    BMQTST_ASSERT_EQ(C2->_messages(), "");
    BMQTST_ASSERT_EQ(C3->_messages(), "1,2");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();
    C3->_resetUnconfirmed();

    // No confirm required. Actually confirm would assert fail.

    // 2) C3 becomes busy.
    C3->_setCanDeliver(false);

    tester.post("3");
    tester.afterNewMessage(1);

    BMQTST_ASSERT_EQ(C1->_messages(), "");
    BMQTST_ASSERT_EQ(C2->_messages(), "");
    BMQTST_ASSERT_EQ(C3->_messages(), "");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();
    C3->_resetUnconfirmed();

    // 3) C3 unsubscribes.
    tester.dropHandle("C3");

    tester.post("4,5");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_messages(), "");
    BMQTST_ASSERT_EQ(C2->_messages(), "4,5");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // 4) C2 becomes busy.
    C2->_setCanDeliver(false);

    tester.post("6");
    tester.afterNewMessage(1);

    BMQTST_ASSERT_EQ(C1->_messages(), "");
    BMQTST_ASSERT_EQ(C2->_messages(), "");
    C1->_resetUnconfirmed();
    C2->_resetUnconfirmed();

    // 5) C2 unsubscribes.
    tester.dropHandle("C2");

    tester.post("7,8");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_messages(), "7,8");
    C1->_resetUnconfirmed();
}

static void testN1_broadcastExhaustiveSubscriptions()
// ------------------------------------------------------------------------
// EXHAUSTIVE SUBSCRIPTIONS TEST
//
// Concerns:
//   Exercise all combinations that exercise this queue.
//
// Testing:
//   That the queue works with any permutation of basic operations.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("EXHAUSTIVE SUBSCRIPTIONS TEST");

    Operations operations;

    // Two subscriptions for C2, one for C1
    operations.push_back(OperationSp(new Subscribe("C1", 1)));
    operations.push_back(OperationSp(new Subscribe("C2", 1)));
    operations.push_back(OperationSp(new Subscribe("C2", 2)));

    // Post 1 message
    operations.push_back(OperationSp(new Post(1)));
    operations.push_back(OperationSp(new Post(2)));

    // One un-subscription per consumer
    operations.push_back(OperationSp(new UnSubscribe("C1")));
    operations.push_back(OperationSp(new UnSubscribe("C2")));

    // Setup invariants
    Invariants invariants;
    invariants.push_back(
        Invariant("expected messages", &checkReceivedForConsumers));

    // Run the tests.
    regress(&operations, invariants);
}

static void testN2_broadcastExhaustiveCanDeliver()
// ------------------------------------------------------------------------
// EXHAUSTIVE CAN DELIVER TEST
//
// Concerns:
//   Exercise all combinations that exercise this queue.
//
// Testing:
//   That the queue works with any permutation of operations including
//   clients being unable to process data (e.g. due to high watermark).
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("EXHAUSTIVE CAN DELIVER TEST");

    Operations operations;

    // Two subscriptions for C2, one for C1
    operations.push_back(OperationSp(new Subscribe("C1", 1)));
    operations.push_back(OperationSp(new Subscribe("C2", 2)));

    // One round of delivering on and one of off for one consumer
    operations.push_back(OperationSp(new SetCanDeliver("C1", false)));

    // Post a message
    operations.push_back(OperationSp(new Post(1)));

    // Enable subscription
    operations.push_back(OperationSp(new SetCanDeliver("C1", true)));

    // One un-subscription per consumer
    operations.push_back(OperationSp(new UnSubscribe("C1")));
    // One less unsubscribe (i.e. not unsubscribing C2) doesn't reduce
    // validation strength but makes 10x difference in speed.

    // Setup invariants
    Invariants invariants;
    invariants.push_back(
        Invariant("expected messages", &checkReceivedForConsumers));

    // Run the tests.
    regress(&operations, invariants);
}

static void testN3_broadcastExhaustiveConsumerPriority()
// ------------------------------------------------------------------------
// EXHAUSTIVE CONSUMER PRIORITY TEST
//
// Concerns:
//   Exercise all combinations that exercise this queue.
//
// Testing:
//   That the queue works with any permutation of basic operations when
//   consumers of various priorities are being added.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("EXHAUSTIVE CONSUMER PRIORITY TEST");

    Operations operations;

    // Subscriptions for 4 consumers with various priorities
    operations.push_back(OperationSp(new Subscribe("C1", 1, 1)));
    operations.push_back(OperationSp(new Subscribe("C2", 2, 2)));
    operations.push_back(OperationSp(new Subscribe("C3", 5, 3, 3)));

    // Post 2 messages
    operations.push_back(OperationSp(new Post(1)));
    operations.push_back(OperationSp(new Post(2)));

    // One un-subscription per consumer
    operations.push_back(OperationSp(new UnSubscribe("C1")));
    operations.push_back(OperationSp(new UnSubscribe("C2")));
    operations.push_back(OperationSp(new UnSubscribe("C3")));

    // Setup invariants
    Invariants invariants;
    invariants.push_back(
        Invariant("expected messages", &checkReceivedForConsumers));

    // Run the tests.
    regress(&operations, invariants);
}

// ----------------------------------------------------------------------------
//                            PRIORITY TESTS
// ----------------------------------------------------------------------------

static void test8_priorityBreathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//  1) 3 consumers with the same priority
//  2) Post 3 messages to the queue, and invoke the engine to deliver them
//     to the highest priority consumers
//  3) Verify that each consumer received 1 message
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1) 3 consumers with the same priority
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=1");

    // 2) Post 3 messages to the queue, and invoke the engine to deliver them
    //    to the highest priority consumers
    tester.post("1,2,3");

    tester.afterNewMessage(3);

    // 3) Verify that each consumer received 1 message
    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    PVV(L_ << ": C3 Messages: " << C3->_messages());

    // TODO: For each message, verify that it was delivered once to exactly
    //       one handle (and one handle only!)
    // BMQTST_ASSERT(tester.wasDeliveredOnce("a,b,c"));
    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C3->_numMessages(), 1);

    // Confirm
    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0"));
}

static void test9_priorityCreateAndConfigure()
// ------------------------------------------------------------------------
// CREATE AND CONFIGURE
//
// Concerns:
//   Ensure that creating the queue engine results in a functional queue
//   engine that can be configured successfully.
//
// Plan:
//  1. Create PriorityQueueEngine
//  2. Verify that the created PriorityQueueEngine is functional by
//     configuring it successfully
//
// Testing:
//   create
//   configure
//   messageReferenceCount
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("CREATE AND CONFIGURE");

    class PriorityQueueEngineTester : public mqbblp::QueueEngineTester {
      public:
        // CREATORS
        PriorityQueueEngineTester(const mqbconfm::Domain& domainConfig,
                                  bslma::Allocator*       allocator)
        : mqbblp::QueueEngineTester(domainConfig, true, allocator)
        {
        }

        // ACCESSORS
        void create(bslma::ManagedPtr<mqbi::QueueEngine>* queueEngine,
                    bslma::Allocator*                     allocator) const
        {
            mqbblp::RootQueueEngine::create(
                queueEngine,
                d_queueState_mp.get(),
                d_queueState_mp->domain()->config(),
                allocator);
        }

        ~PriorityQueueEngineTester() {}
    };

    mqbconfm::Domain domainConfig;
    domainConfig.mode().makePriority();

    PriorityQueueEngineTester            tester(domainConfig,
                                     bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<mqbi::QueueEngine> queueEngineMp;
    BSLS_ASSERT_OPT(!queueEngineMp);

    // 1. Create PriorityQueueEngine
    tester.create(&queueEngineMp, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT(queueEngineMp.get() != 0);

    // 2. Verify that the created PriorityQueueEngine is functional by
    //    configuring it successfully
    bmqu::MemOutStream errorDescription(bmqtst::TestHelperUtil::allocator());
    errorDescription.reset();

    int rc = queueEngineMp->configure(errorDescription, false);

    BMQTST_ASSERT_EQ(errorDescription.length(), 0U);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(queueEngineMp->messageReferenceCount(), 1U);
}

static void test10_priorityAggregateDownstream()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//  a) If a client has multiple highest priority consumers, then the queue
//     engine will deliver to that client one message per highest priority
//     consumer before moving on to the next client
//
// Plan:
//  1) Configure 3 handles with the same priority, C2 has 2 highest
//     priority consumers, C1 and C3 have 1 highest priority consumer.
//     Post 4 messages, deliver, and verify.
//  2) Configure C3 to have 3 highest priority consumers.  Post 6 messages,
//     deliver, and verify.
//  3) Configure C3 to have 2 highest priority consumers.  Post 5 messages,
//     deliver, and verify.
//  4) Configure C2 to have 1 highest priority consumer.  Post 4 messages,
//     deliver, and verify.
//  5) Configure C2 to have 0 highest priority consumer.  Post 4 messages,
//     deliver, and verify.
//
// Testing:
//   'afterNewMessage()' with client having multiple highest priority
//   consumers
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("AGGREGATE DOWNSTREAM");

    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=1");

    // 1) C2: 2 highest priority consumers
    tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=2");

    // C1: 1
    // C2: 2
    // C3: 1
    tester.post("1,2,3,4");
    tester.afterNewMessage(4);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C3->_numMessages(), 1);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0"));

    // 2) C3: 3 highest priority consumers
    tester.getHandle("C3 readCount=1");
    tester.getHandle("C3 readCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=2");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=3");

    // C1: 1
    // C2: 2
    // C3: 3
    tester.post("5,6,7,8,9,10");
    tester.afterNewMessage(6);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C3->_numMessages(), 3);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1,2"));

    // 3) C3: 2 highest priority consumers
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=2");
    tester.releaseHandle("C3 readCount=1");

    // C1: 1
    // C2: 2
    // C3: 2
    tester.post("11,12,13,14,15");
    tester.afterNewMessage(5);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C3->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1"));

    // 4) C2: 1 highest priority consumer
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");
    tester.releaseHandle("C2 readCount=1");

    // C1: 1
    // C2: 1
    // C3: 2
    tester.post("16,17,18,19");
    tester.afterNewMessage(4);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C3->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1"));

    // 5) C2: No highest priority consumers
    tester.configureHandle("C2 consumerPriority=0 consumerPriorityCount=1");

    // C1: 1
    // C2: 0
    // C3: 2
    tester.post("20,21,22,23,24,25");
    tester.afterNewMessage(6);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C3->_numMessages(), 4);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1,2,3"));
}

static void test11_priorityReconfigure()
// ------------------------------------------------------------------------
// RECONFIGURE
//
// Concerns:
//   a) If a handle is reconfigured for a client, then the queue engine
//      correctly accounts for the reconfigure with respect to distributing
//      messages.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the same priority. Post 2
//      messages, deliver, and verify.
//   2) Reconfigure C1 to a lower priority.  Post 2 messages, deliver, and
//      verify that C2 received both messages (and C1 received none).
//   3) Reconfigure C2 to C1's priority.  Post 4 messages, deliver, and
//      verify that both C1 and C2 received 2 messages each.
//   4) Bring up a new handle, C3, and configure it with a new highest
//      priority.  Post 2 messages, deliver, and verify that C3 received
//      both messages (and C1 and C2 received none).
//   5) Reconfigure C3 to C1 and C2's priority, and reconfigure C1 to have
//      a 'consumerPriorityCount' of 2.  Post 8 messages, deliver, and
//      verify that C1 received 4 messages, and C2 and C3 received 2
//      messages each.
//
// Testing:
//   Queue Engine delivery across handle reconfigure.
//     - 'configureHandle()'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks
    // from 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RECONFIGURE");

    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    // 1)
    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("1,2");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0"));

    // 2) C1: Lower priority
    tester.configureHandle("C1 consumerPriority=0 consumerPriorityCount=1");

    // C1: 0
    // C2: 1
    tester.post("3,4");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));

    // 3) C2: Lower priority
    tester.configureHandle("C2 consumerPriority=0 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("5,6,7,8");
    tester.afterNewMessage(4);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0,1"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));

    // 4) C3: New highest priority
    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=1");

    // C1: 0
    // C2: 0
    // C3: 1
    tester.post("9,10");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C3->_numMessages(), 2);

    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1"));

    // 5) C3: Lower priority, C1: Increase priority count
    tester.configureHandle("C3 consumerPriority=0 consumerPriorityCount=1");
    tester.configureHandle("C1 consumerPriority=0 consumerPriorityCount=2");

    // C1: 2
    // C2: 1
    // C3: 1
    tester.post("11,12,13,14,15,16,17,18");
    tester.afterNewMessage(8);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 4);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C3->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0,1,2,3"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
    tester.confirm("C3",
                   mqbblp::QueueEngineTestUtil::getMessages(C3->_messages(),
                                                            "0,1"));
}

static void test12_priorityCannotDeliver()
// ------------------------------------------------------------------------
// CANNOT DELIVER
//
// Concerns:
//   a) If it is not possible to deliver a message to a client, then the
//      queue engine attempts to deliver messages to other clients.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the same priority. Post 2
//      messages, deliver, and verify.
//   2) Disable delivery for C1.  Post 2 messages, deliver, and verify
//      that C2 received both messages (and C2 received none).
//   3) Disable delivery for C2.  Post 1 message, deliver, and verify that
//      neither C2 nor C1 received any messages.
//   4) Enable delivery for C1.  Verify that C1 received the "pending"
//      message from step 3 above (and C2 received none).
//   5) Enable delivery for C2.  Verify that neither C1 nor C2 received any
//      messages.  Post 4 messages, deliver, and verify that both C1 and
//      C2 received 2 messages each.
//
// Testing:
//   Queue Engine delivery when it is not possible to deliver to one or
//   more highest priority consumers.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("CANNOT CONSUMERS");

    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("1,2");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));
    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0"));

    // 2) C1: Can't deliver
    C1->_setCanDeliver(false);

    // C1: N.A.
    // C2: 1
    tester.post("3,4");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));

    // 3) C2: Can't deliver
    C2->_setCanDeliver(false);

    // C1: N.A.
    // C2: N.A.
    tester.post("5");
    tester.afterNewMessage(1);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 0);

    // 4) C1: Can deliver
    C1->_setCanDeliver(true);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 0);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));

    // 5) C2: Can deliver
    C2->_setCanDeliver(true);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 0);

    // C1: 1
    // C2: 1
    tester.post("6,7,8,9");
    tester.afterNewMessage(4);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0,1"));

    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1"));
}

static void test13_priorityRedeliverToFirstConsumerUp()
// ------------------------------------------------------------------------
// REDELIVERY TO FIRST CONSUMER UP
//
// Concerns:
//   a) If a last consumer goes down without confirming messages, then when
//      the first next consumer comes up the queue engine attempts to
//      deliver messages that were previously sent but not confirmed.
//
// Plan:
//   1) Configure 1 handle, C1.  Post 2 messages, deliver, and verify.
//      Confirm the 1st message but not the 2nd.
//   2) Bring C1 down and then back up.  Verify that the 2nd message was
//      indeed "redelivered" to C1.
//   3) Bring C1 down, then configure 1 handle, C2.  Verify that the 2nd
//      message was indeed "redelivered" to C2.
// Testing:
//   Queue Engine redelivery of messages that were sent but not confirmed
//   before the first next consumer comes up.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("REDELIVERY TO FIRST CONSUMER UP");

    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");

    // C1: 2
    tester.post("1,2");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C1->_messages(), "1,2");

    tester.confirm("C1", "1");

    // 2)
    tester.dropHandle("C1");

    C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");

    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C1->_messages(), "2");

    // 3)
    tester.dropHandle("C1");

    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_messages(), "2");

    tester.confirm("C2", "2");
}

static void test14_priorityRedeliverToOtherConsumers()
// ------------------------------------------------------------------------
// REDELIVERY TO OTHER CONSUMERS
//
// Concerns:
//   a) If a consumer goes down without confirming messages, then the queue
//      engine attempts to deliver messages that were previously sent but
//      not confirmed to other consumers.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the same priority. Post 4
//      messages, deliver, and verify that C1 and C2 each got 2 messages.
//      C1 confirms the 1st message but not the 2nd.
//   2) Bring C1 down.  Verify that C1's 2nd message was indeed
//      "redelivered" to C2.
// Testing:
//   Queue Engine redelivery of messages that were sent but not confirmed
//   when other consumers are available and able to receive messages.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("REDELIVERY TO OTHER CONSUMERS");

    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("1,2,3,4");
    tester.afterNewMessage(4);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);

    tester.confirm("C1",
                   mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(),
                                                            "0"));

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());

    const bsl::string unconfirmedMessage =
        mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(), "0");

    PVV(L_ << ": unconfirmedMessage: " << unconfirmedMessage);

    // 2)
    tester.dropHandle("C1");

    PVV(L_ << ": C2 Messages: " << C2->_messages());

    BMQTST_ASSERT_EQ(C2->_numMessages(), 3);
    BMQTST_ASSERT_EQ(unconfirmedMessage,
                     mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                              "2"));

    tester.confirm("C2",
                   mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                            "0,1,2"));
}

static void test15_priorityReleaseActiveConsumerWithoutNullReconfigure()
// ------------------------------------------------------------------------
// RELEASE ACTIVE CONSUMER WITHOUT NULL RECONFIGURE
//
// Concerns:
//   a) If an active consumer loses read capacity without a preceding
//      configureQueue with null parameters, then the queue engine should
//      gracefully handle this case by manually setting null stream
//      parameters on the handle and rebuilding the set of active
//      consumers.
//   b) An alarm is generated in the case described above.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the same priority. Post 4
//      messages, deliver, and verify that C1 and C2 each got 2 messages.
//   2) Release the consumer portion of C1 without a preceding
//      'configureQueue' with null stream parameters.  Verify that C1's
//      stream parameters have been set to null, that it was removed from
//      the set of active consumers, that its messages were redelivered
//      to C2, and that an alarm was generated.
//
// Testing:
//   Queue Engine gracefully handling of relasing the consumer portion of
//   a handle without a preceding configureQueue with null stream
//   parameters.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RELEASE ACTIVE CONSUMER WITHOUT NULL"
                                      " RECONFIGURE");

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_ERROR,
                                          bmqtst::TestHelperUtil::allocator());
    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1) Configure 2 handles, C1 and C2, with the same priority. Post 4
    //    messages, deliver, and verify that C1 and C2 each got 2 messages.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("1,2,3,4");
    tester.afterNewMessage(4);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());

    // 2) Release the consumer portion of C1 without a preceding
    //    'configureQueue' with null stream parameters.  Verify that C1's
    //    stream parameters have been set to null, that it was removed from the
    //    set of active consumers, and that its messages were "redelivered" to
    //    C2.
    const bsl::string unconfirmedMessages =
        mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(), "0,1");

    BMQTST_ASSERT(logObserver.records().empty());

    tester.releaseHandle("C1 readCount=1");

    PV(L_ << ": C2 Messages: " << C2->_messages());

    // C1 has no active subStreams
    BMQTST_ASSERT_EQ(C1->_numActiveSubstreams(), 0U);

    // Messages were redelivered
    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 4);
    BMQTST_ASSERT_EQ(mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                              "2,3"),
                     unconfirmedMessages);

    // C1 not among the set of active consumers
    tester.post("5");
    tester.afterNewMessage(1);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 5);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());

    BMQTST_ASSERT_EQ(mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                              "4"),
                     "5");
}

static void test16_priorityReleaseDormantConsumerWithoutNullReconfigure()
// ------------------------------------------------------------------------
// RELEASE DORMANT CONSUMER WITHOUT NULL RECONFIGURE
//
// Concerns:
//   a) If a dormant consumer (i.e. not having the highest consumer
//      priority) loses read capacity without a preceding configureQueue
//      with null parameters, then the queue engine should gracefully
//      handle this case by manually setting null stream parameters on the
//      handle and rebuilding the set of active consumers.
//   b) An alarm is generated in the case described above.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the same priority. Post 2
//      messages, deliver, and verify that only each got one message.
//   2) Lower the priority of C1, such that only C2 is an active consumer.
//   3) Release the consumer portion of C1 without a preceding
//      'configureQueue' with null stream parameters.  Verify that C1's
//      stream parameters have been set to null, that its messages were
//      "redelivered" to C2, and that an alarm was generated.
//
// Testing:
//   Queue Engine gracefully handling of relasing the dormant consumer
//   portion of a handle without a preceding configureQueue with null
//   stream parameters.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RELEASE DORMANT CONSUMER WITHOUT NULL"
                                      " RECONFIGURE");

    bmqtst::ScopedLogObserver logObserver(ball::Severity::e_ERROR,
                                          bmqtst::TestHelperUtil::allocator());
    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1) Configure 2 handles, C1 and C2, with the same priority. Post 4
    //    messages, deliver, and verify that C1 and C2 each got 2 messages.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    // C1: 1
    // C2: 1
    tester.post("1,2");
    tester.afterNewMessage(2);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());

    // 2)
    tester.configureHandle("C1 consumerPriority=0 consumerPriorityCount=1");

    // 3) Release the consumer portion of C1 without a preceding
    //    'configureQueue' with null stream parameters.  Verify that C1's
    //    stream parameters have been set to null, that its messages were
    //    redelivered to C2, and that an alarm was generated.
    const bsl::string unconfirmedMessages =
        mqbblp::QueueEngineTestUtil::getMessages(C1->_messages(), "0");

    BMQTST_ASSERT(logObserver.records().empty());

    BMQTST_ASSERT_EQ(C1->_numActiveSubstreams(), 1U);
    tester.releaseHandle("C1 readCount=1");

    PV(L_ << ": C2 Messages: " << C2->_messages());

    // C1 has no active subStreams
    BMQTST_ASSERT_EQ(C1->_numActiveSubstreams(), 0U);

    // Messages were redelivered
    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);
    BMQTST_ASSERT_EQ(mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                              "1"),
                     unconfirmedMessages);

    // C1 not among the set of active consumers
    tester.post("3");
    tester.afterNewMessage(1);

    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 3);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());

    BMQTST_ASSERT_EQ(mqbblp::QueueEngineTestUtil::getMessages(C2->_messages(),
                                                              "2"),
                     "3");
}

static void test17_priorityBeforeMessageRemoved_garbageCollection()
// ------------------------------------------------------------------------
// BEFORE MESSAGE REMOVED - GARBAGE COLLECTION
//
// Concerns:
//   a) If the queue engine is notified that a message is about to be
//      removed from the queue (e.g. it's TTL expired, it was deleted by
//      admin task, confirmed by all recipients, etc.), then it updates its
//      position in the stream of the queue to advance past the message
//      accordingly (if needed).
//
// Plan:
//   1) Bring up a consumer C1 and post 4 messages.
//   2) Simulate queue garbage collection (e.g., TTL expiration) of the
//      first 2 messages (this involves invoking
//      'beforeMessageRemoved(msgGUID)' followed by removing the message
//      from storage -- for each message in turn).
//   3) Invoke message delivery in the engine and verify that only the last
//      2 messages were delivered to C1.
//
// Testing:
//   Queue Engine adjusting its position in the stream of the queue upon
//   receiving notification prior to a message being removed.
//   - 'beforeMessageRemoved()'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BEFORE MESSAGE REMOVED - GARBAGE "
                                      "COLLECTION");

    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1) Bring up a consumer C1 and post 4 messages.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");

    PVV(L_ << ": post ['1','2','3','4']");
    tester.post("1,2,3,4");

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);

    // 2) Simulate queue garbage collection (e.g., TTL expiration) of the first
    //    2 messages (this involves invoking 'beforeMessageRemoved(msgGUID)'
    //    followed by removing the message from storage -- for each message in
    //    turn).
    PVV(L_ << ": Garbage collect (TTL expiration) ['1','2']");
    tester.garbageCollectMessages(2);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);

    // 3) Invoke message delivery in the engine and verify that only the last 2
    //    messages were delivered to C1.
    PVV(L_ << ": afterNewMessage ['3','4']");
    tester.afterNewMessage(2);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    BMQTST_ASSERT_EQ(C1->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C1->_messages(), "3,4");
}

static void test18_priorityAfterQueuePurged_queueStreamResets()
// ------------------------------------------------------------------------
// AFTER QUEUE PURGED - QUEUE STREAM RESETS
//
// Concerns:
//   If the Queue Engine is notified that the queue *was* purged (i.e.,
//   after it occurred), then:
//     a) Undelivered messages from prior to the purge are not delivered to
//        any consumer.
//
// Plan:
//   1) Post two messages, attempt to deliver, and then purge the queue and
//      notify the engine.
//   2) Bring up a consumer C1 configured with 'maxUnconfirmedMessages=2'
//      and verify that it did not receive any messages.
//   3) Post 4 messages, deliver, and verify that C1 received 2 messages.
//   4) Purge the queue, then notify the queue engine that the queue was
//      purged.
//   5) Confirm C1's messages (thus making it unsaturated and capable of
//      receiving messages) and verify that it did not receive any
//      messages (purge should have wiped them out).
//   6) Post 2 messages, deliver, and verify that C1 received both
//      messages.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("AFTER QUEUE PURGED - QUEUE STREAM"
                                      " RESETS");

    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1) Post two messages, attempt to deliver, and then purge the queue and
    //    notify the engine.
    PVV(L_ << ": post ['1','2']");
    tester.post("1,2");

    PVV(L_ << ": afterNewMessage ['1','2']");
    tester.afterNewMessage(2);

    PVV(L_ << ": --> purgeQueue <--");
    tester.purgeQueue();
    PVV(L_ << ": afterQueuePurged ... ");

    // 2) Bring up a consumer C1 configured with 'maxUnconfirmedMessages=2' and
    //    verify that it did not receive any messages.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1"
                           " maxUnconfirmedMessages=2");

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C1->_messages(), "");

    // 3) Post 4 messages, deliver, and verify that C1 received 2 messages.
    PVV(L_ << ": post ['11','12','13','14']");
    PVV(L_ << ": afterNewMessage ['11','12','13','14']");
    tester.post("11,12");
    tester.afterNewMessage(2);

    C1->_setCanDeliver(false);

    tester.post("13,14");
    tester.afterNewMessage(2);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    BMQTST_ASSERT_EQ(C1->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C1->_messages(), "11,12");

    // 4) Purge the queue, then notify the queue engine that the queue was
    //    purged.
    PVV(L_ << ": --> purgeQueue <--");
    tester.purgeQueue();
    PVV(L_ << ": afterQueuePurged ... ");

    // 5) Confirm C1's messages (thus making it unsaturated and capable of
    //    receiving messages) and verify that it did not receive any messages
    //    (purge should have wiped them out).
    tester.confirm("C1", "11,12");
    C1->_setCanDeliver(true);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    BMQTST_ASSERT_EQ(C1->_numMessages(), 0);
    BMQTST_ASSERT_EQ(C1->_messages(), "");

    // 6) Post 2 messages, deliver, and verify that C1 received both
    //      messages.
    PVV(L_ << ": post ['21','22']");
    tester.post("21,22");

    PVV(L_ << ": afterNewMessage ['21','22']");
    tester.afterNewMessage(2);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    BMQTST_ASSERT_EQ(C1->_numMessages(), 2);
    BMQTST_ASSERT_EQ(C1->_messages(), "21,22");
}

static void test19_priorityReleaseHandle_isDeletedFlag()
// ------------------------------------------------------------------------
// RELEASE HANDLE - IS-DELETED FLAG
//
// Concerns:
//   1. Ensure that releasing a handle fully (i.e., all counts go to zero
//      'isFinal=true' is explicitly specified) results in the handle being
//      deleted, as indicated by the 'isDeleted' flag propagated via the
//      'releaseHandle' callback invoation.
//
// Plan:
//   1. Bring up:
//      - C1 with writeCount=1
//      - C2 with readCount=2.
//      - C3 with readCount=2
//   2. Release from C2 one reader but pass 'isFinal=true' and verify that
//      C2 was fully deleted.
//   3. Release from C1 one writer but pass 'isFinal=false' and verify that
//      C1 was fully deleted.
//   4. Release from C3 two readers and pass 'isFinal=true' (i.e. correct
//      scenario) and verify that C3 was fully deleted.
//
// Testing:
//  void releaseHandle(
//      mqbi::QueueHandle                                *handle,
//      const bmqp_ctrlmsg::QueueHandleParameters&        handleParameters,
//      bool                                              isFinal,
//      const mqbi::QueueHandle::HandleReleasedCallback&  releasedCb)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RELEASE HANDLE - IS-DELETED FLAG");

    mqbconfm::Domain domainConfig(bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTester tester(domainConfig,
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1. Bring up a producer C1 with writeCount=1, consumer C2 with
    //    readCount=2, and consumer C3 with readCount=2.
    tester.getHandle("C1 writeCount=1");
    tester.getHandle("C2 readCount=2");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=2");
    tester.getHandle("C3 readCount=2");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=2");

    bool isDeleted = false;

    // 2. Release from C2 one reader but pass 'isFinal=true' and verify that
    //    C2 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C2 readCount=1 isFinal=true",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);

    // 3. Release from C1 one writer but pass 'isFinal=false' and verify that
    //    C1 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C1 writeCount=1 isFinal=false",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);

    //   4. Release from C3 two readers and pass 'isFinal=true' (i.e. correct
    //      scenario) and verify that C3 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C3 readCount=2 isFinal=true",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);
}

static void test20_priorityRedeliverAfterGc()
// ------------------------------------------------------------------------
// REDELIVERY AFTER GC
//
// Concerns:
//   'beforeMessageRemoved' should remove message from the redeliveryList
//   (as well as from the storage).  This case triggers
//   'beforeMessageRemoved' by calling GC.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1.
//      Disable delivery to C2. Post 4 messages, verify that C1 got 4
//      messages and C2 got none.
//   2) Bring C1 down.  Verify C2 still has no messages.  The queue now has
//      all 4 messages in its redeliveryList
//   3) GC one (first) message.  That should remove it from the storage as
//      well as from the redelivery list.
//   4) Now enable and trigger delivery on C2.  Verify that C2 got 3
//      messages.
// Testing:
//   Queue Engine redelivery of unconfirmed messages after removing some of
//   them.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("REDELIVERY AFTER GC");

    mqbblp::QueueEngineTester tester(priorityDomainConfig(),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");

    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1"
                           " maxUnconfirmedMessages=0");

    tester.post("1,2,3,4");
    tester.afterNewMessage(4);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    PVV(L_ << ": C2 Messages: " << C2->_messages());

    BMQTST_ASSERT_EQ(C1->_numMessages(), 4);
    BMQTST_ASSERT_EQ(C2->_numMessages(), 0);

    BMQTST_ASSERT_EQ(C1->_messages(), "1,2,3,4");

    // 2)
    tester.dropHandle("C1");
    BMQTST_ASSERT_EQ(C2->_numMessages(), 0);

    // 3)
    tester.garbageCollectMessages(1);

    // 4)
    C2->_setCanDeliver(true);

    PVV(L_ << ": C2 Messages: " << C2->_messages());

    BMQTST_ASSERT_EQ(C2->_numMessages(), 3);
    BMQTST_ASSERT_EQ(C2->_messages(), "2,3,4");

    tester.confirm("C2", "2,3,4");
}

// ----------------------------------------------------------------------------
//                             FANOUT TESTS
// ----------------------------------------------------------------------------

static void test21_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//  1. FanoutQueueEngine_ConsumerState breathing test
//  2. FanoutQueueEngine breathing test
//    1. 3 fanout consumers
//    2. Post 3 messages to the queue, and invoke the engine to deliver
//       them
//    3. Verify that each consumer received all 3 messages
//    4. In turn, confirm messages of each consumer, and verify that others
//       were unaffected.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    {
        PV("FanoutQueueEngine breathing test");
        // 1. 3 fanout consumers
        mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
        mqbmock::QueueHandle* C2 = tester.getHandle("C2@b readCount=1");
        mqbmock::QueueHandle* C3 = tester.getHandle("C3@c readCount=1");

        BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
        BMQTST_ASSERT_NE(C1, C2);
        BMQTST_ASSERT_NE(C1, C3);
        BMQTST_ASSERT_NE(C2, k_nullMockHandle_p);
        BMQTST_ASSERT_NE(C2, C3);
        BMQTST_ASSERT_NE(C3, k_nullMockHandle_p);

        BMQTST_ASSERT_EQ(C1->_appIds(), "a");
        BMQTST_ASSERT_EQ(C2->_appIds(), "b");
        BMQTST_ASSERT_EQ(C3->_appIds(), "c");

        // Configure
        BMQTST_ASSERT_EQ(tester.configureHandle(
                             "C1@a maxUnconfirmedMessages=11"
                             " consumerPriority=1 consumerPriorityCount=1"),
                         0);
        BMQTST_ASSERT_EQ(tester.configureHandle(
                             "C2@b maxUnconfirmedMessages=12"
                             " consumerPriority=1 consumerPriorityCount=1"),
                         0);
        BMQTST_ASSERT_EQ(tester.configureHandle(
                             "C3@c maxUnconfirmedMessages=13"
                             " consumerPriority=1 consumerPriorityCount=1"),
                         0);

        PV(L_ << ": C1@a stream parameters:" << C1->_streamParameters("a"));
        PV(L_ << ": C2@b stream parameters:" << C2->_streamParameters("b"));
        PV(L_ << ": C3@c stream parameters:" << C3->_streamParameters("c"));

        BMQTST_ASSERT_EQ(
            bmqp::ProtocolUtil::consumerInfo(C1->_streamParameters("a"))
                .maxUnconfirmedMessages(),
            11);
        BMQTST_ASSERT_EQ(
            bmqp::ProtocolUtil::consumerInfo(C2->_streamParameters("b"))
                .maxUnconfirmedMessages(),
            12);
        BMQTST_ASSERT_EQ(
            bmqp::ProtocolUtil::consumerInfo(C3->_streamParameters("c"))
                .maxUnconfirmedMessages(),
            13);

        // 2. Post 3 messages to the queue, and invoke the engine to deliver
        //    them
        tester.post("1,2");
        tester.post("3");
        tester.afterNewMessage(3);

        // 3. Verify that each consumer received all 3 messages
        PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
        PVV(L_ << ": C2@b Messages: " << C2->_messages("b"));
        PVV(L_ << ": C3@c Messages: " << C3->_messages("c"));

        BMQTST_ASSERT_EQ(C1->_numMessages("a"), 3);
        BMQTST_ASSERT_EQ(C2->_numMessages("b"), 3);
        BMQTST_ASSERT_EQ(C3->_numMessages("c"), 3);

        BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2,3");
        BMQTST_ASSERT_EQ(C2->_messages("b"), "1,2,3");
        BMQTST_ASSERT_EQ(C3->_messages("c"), "1,2,3");

        // 4. In turn, confirm messages of each consumer, and verify that
        //    others were unaffected.
        // C1
        PVV(L_ << ": C1@a confirming ['1','2','3']");

        tester.confirm("C1@a", "1,2,3");

        PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
        PVV(L_ << ": C2@b Messages: " << C2->_messages("b"));
        PVV(L_ << ": C3@c Messages: " << C3->_messages("c"));

        BMQTST_ASSERT_EQ(C1->_messages("a"), "");
        BMQTST_ASSERT_EQ(C2->_messages("b"), "1,2,3");
        BMQTST_ASSERT_EQ(C3->_messages("c"), "1,2,3");

        // C2
        PVV(L_ << ": C2@b confirming ['1','2','3']");

        tester.confirm("C2@b", "1,2,3");

        PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
        PVV(L_ << ": C2@b Messages: " << C2->_messages("b"));
        PVV(L_ << ": C3@c Messages: " << C3->_messages("c"));

        BMQTST_ASSERT_EQ(C1->_messages("a"), "");
        BMQTST_ASSERT_EQ(C2->_messages("b"), "");
        BMQTST_ASSERT_EQ(C3->_messages("c"), "1,2,3");

        // C3
        PVV(L_ << ": C3@c confirming ['1','3']");

        tester.confirm("C3@c", "1,3");

        PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
        PVV(L_ << ": C2@b Messages: " << C2->_messages("b"));
        PVV(L_ << ": C3@c Messages: " << C3->_messages("c"));

        BMQTST_ASSERT_EQ(C1->_messages("a"), "");
        BMQTST_ASSERT_EQ(C2->_messages("b"), "");
        BMQTST_ASSERT_EQ(C3->_messages("c"), "2");

        PVV(L_ << ": C3@c confirming ['2']");

        tester.confirm("C3@c", "2");

        BMQTST_ASSERT_EQ(C1->_messages("a"), "");
        BMQTST_ASSERT_EQ(C2->_messages("b"), "");
        BMQTST_ASSERT_EQ(C3->_messages("c"), "");

        PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
        PVV(L_ << ": C2@b Messages: " << C2->_messages("b"));
        PVV(L_ << ": C3@c Messages: " << C3->_messages("c"));

        BMQTST_ASSERT_EQ(tester.releaseHandle("C1@a readCount=1"), 0);
        BMQTST_ASSERT_EQ(tester.releaseHandle("C2@b readCount=1"), 0);
        BMQTST_ASSERT_EQ(tester.releaseHandle("C3@c readCount=1"), 0);

        BMQTST_ASSERT_EQ(C1->_appIds(), "");
        BMQTST_ASSERT_EQ(C2->_appIds(), "");
        BMQTST_ASSERT_EQ(C3->_appIds(), "");
    }
}

static void test22_createAndConfigure()
// ------------------------------------------------------------------------
// CREATE AND CONFIGURE
//
// Concerns:
//   Ensure that creating the queue engine results in a functional queue
//   engine that can be configured successfully.
//
// Plan:
//  1. Create FanoutQueueEngine
//  2. Verify that the created FanoutQueueEngine is functional by
//     configuring it successfully
//
// Testing:
//   create
//   configure
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("CREATE AND CONFIGURE");

    class FanoutQueueEngineTester : public mqbblp::QueueEngineTester {
      public:
        // CREATORS
        FanoutQueueEngineTester(const mqbconfm::Domain& domainConfig,
                                bslma::Allocator*       allocator)
        : mqbblp::QueueEngineTester(domainConfig, true, allocator)
        {
        }

        // ACCESSORS
        void create(bslma::ManagedPtr<mqbi::QueueEngine>* queueEngine,
                    bslma::Allocator*                     allocator) const
        {
            mqbblp::RootQueueEngine::create(
                queueEngine,
                d_queueState_mp.get(),
                d_queueState_mp->domain()->config(),
                allocator);
        }

        ~FanoutQueueEngineTester() {}
    };

    FanoutQueueEngineTester              tester(fanoutConfig("a,b,c,d,e"),
                                   bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<mqbi::QueueEngine> queueEngineMp;
    BSLS_ASSERT_OPT(!queueEngineMp);

    // 1. Create FanoutQueueEngine
    tester.create(&queueEngineMp, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT(queueEngineMp.get() != 0);

    // 2. Verify that the created FanoutQueueEngine is functional by
    //    configuring it successfully
    bmqu::MemOutStream errorDescription(bmqtst::TestHelperUtil::allocator());
    errorDescription.reset();
    int rc = queueEngineMp->configure(errorDescription, false);

    BMQTST_ASSERT_EQ(errorDescription.length(), 0U);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(queueEngineMp->messageReferenceCount(), 5U);
}

static void test23_loadRoutingConfiguration()
// ------------------------------------------------------------------------
// ROUTING CONFIGURATION
//
// Concerns:
//   Ensure that the correct routing configuration is loaded.
//
// Plan:
//  1. Load routing configuration and verify that it corresponds to
//     multiple substreams.
//
// Testing:
//   loadRoutingConfiguration
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("ROUTING CONFIGURATION");

    mqbblp::QueueEngineTester tester(fanoutConfig("a"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());
    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1.
    {
        bmqp_ctrlmsg::RoutingConfiguration config;
        BMQTST_ASSERT(bmqp::RoutingConfigurationUtils::isClear(config));

        mqbblp::RootQueueEngine::FanoutConfiguration::loadRoutingConfiguration(
            &config);

        bdlb::BitUtil::uint64_t flags       = config.flags();
        int                     numFlagsSet = bdlb::BitUtil::numBitsSet(flags);
        BMQTST_ASSERT_EQ(numFlagsSet, 2);
        BMQTST_ASSERT(
            bmqp::RoutingConfigurationUtils::hasMultipleSubStreams(config));
        BMQTST_ASSERT(
            bmqp::RoutingConfigurationUtils::isDeliverConsumerPriority(
                config));
    }

    // 2.
    {
        bmqp_ctrlmsg::RoutingConfiguration config;
        BMQTST_ASSERT(bmqp::RoutingConfigurationUtils::isClear(config));

        mqbblp::RootQueueEngine::PriorityConfiguration::
            loadRoutingConfiguration(&config);

        bdlb::BitUtil::uint64_t flags       = config.flags();
        int                     numFlagsSet = bdlb::BitUtil::numBitsSet(flags);
        BMQTST_ASSERT_EQ(numFlagsSet, 1);
        BMQTST_ASSERT(
            !bmqp::RoutingConfigurationUtils::hasMultipleSubStreams(config));
        BMQTST_ASSERT(
            bmqp::RoutingConfigurationUtils::isDeliverConsumerPriority(
                config));
    }
}

static void test24_getHandleDuplicateAppId()
// ------------------------------------------------------------------------
// GET HANDLE - DUPLICATE APP ID
//
// Concerns:
//   Ensure that getting a handle for an appId that was already opened
//   fails.
//
// Plan:
//  1) Bring up a consumer C1 with appId 'a'.
//  2) Attempt to get handle for C1 with appId 'a' and verify it fails.
//  3) Attempt bring up a consumer C2 with appId 'a' and verify it fails.
//
// Testing:
//   'getHandle' for an appId that was already opened.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("GET HANDLE - DUPLICATE APP ID");

    mqbblp::QueueEngineTester tester(fanoutConfig("a"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1) Bring up a consumer C1 with appId 'a'.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1->_appIds(), "a");

    // 2) Attempt to get handle for C1 with appId 'a' and verify it succeeds.
    mqbmock::QueueHandle* TMP = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_NE(TMP, k_nullMockHandle_p);

    // 3) Attempt bring up a consumer C2 with appId 'a' and verify it succeeds.
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@a readCount=1");
    BMQTST_ASSERT_NE(C2, k_nullMockHandle_p);

    // Finally, C1 still has only appId 'a'
    BMQTST_ASSERT_EQ(C1->_appIds(), "a");
}

static void test25_getHandleSameHandleMultipleAppIds()
// ------------------------------------------------------------------------
// GET HANDLE - SAME HANDLE, MULTIPLE APP IDS
//
// Concerns:
//   Ensure that getting the same handle with multiple distinct appIds
//   succeeds.
//
// Plan:
//  1) Bring up a consumer C1 with appId 'a'.
//  2) Get handle for C1 with appId 'b' and verify it succeeds.
//  3) Get handle for C1 with appId 'c' and verify it succeeds.
//
// Testing:
//   'getHandle' for multiple distinct appIds succeeds.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("GET HANDLE - MULTIPLE APP IDS");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1) Bring up a consumer C1 with appId 'a'.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1->_appIds(), "a");

    // 2) Get handle for C1 with appId 'b' and verify it succeeds.
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@b readCount=1"));
    BMQTST_ASSERT_EQ(C1->_appIds(), "a,b");

    // 3) Get handle for C1 with appId 'c' and verify it succeeds.
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@c readCount=1"));
    BMQTST_ASSERT_EQ(C1->_appIds(), "a,b,c");
}

static void test26_getHandleUnauthorizedAppId()
// ------------------------------------------------------------------------
// GET HANDLE - UNAUTHORIZED APP ID
//
// Concerns:
//   Ensure that getting a handle for an appId that was already opened
//   fails.
//
// Plan:
//  1. Bring up a consumer C1 with appId 'a'.
//  2. Attempt to get handle for C1 with unauthorized appId 'pikachu'
//     and verify it succeeds.
//  3. Attempt to bring up a consumer C2 with unauthorized appId 'pikachu'
//     and verify it fails.
//
// Testing:
//   getHandle for an unauthorized appId
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("GET HANDLE - UNAUTHORIZED APP ID");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    //  1. Bring up a consumer C1 with appId 'a'.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1->_appIds(), "a");

    //  2. Attempt to get handle for C1 with unauthorized appId 'pikachu'
    //     and verify it succeeds.
    mqbmock::QueueHandle* tmp = tester.getHandle("C1@pikachu readCount=1");
    BMQTST_ASSERT_NE(tmp, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1->_appIds(), "a,pikachu");

    //  3. Attempt to bring up a consumer C2 with same unauthorized appId and
    //     verify it succeeds.
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@pikachu readCount=1");
    BMQTST_ASSERT_NE(C2, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C2->_appIds(), "pikachu");
}

static void test27_configureHandleMultipleAppIds()
// ------------------------------------------------------------------------
// CONFIGURE HANDLE - MULTIPLE APP IDS
//
// Concerns:
//   1. Configuring the same handle with multiple distinct appIds succeeds.
//
// Plan:
//  1. Bring up a consumer C1 with appIds "a,b,c".
//  2. Configure C1 for appId 'a' and verify it succeeds.
//  3. Configure C1 for appId 'b' and verify it succeeds.
//  4. Configure C1 for appId 'c' and verify it succeeds.
//  5. Verify C1 has expected stream parameters for each of the appIds.
//  6.
//
//
// Testing:
//   'configureHandle' for multiple distinct appIds.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("CONFIGURE HANDLE - MULTIPLE APP IDS");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1. Bring up a consumer C1 with appId 'a'.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@b readCount=1"));
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@c readCount=1"));

    BMQTST_ASSERT_EQ(C1->_appIds(), "a,b,c");

    // 2. Configure C1 for appId 'a' and verify it succeeds.
    BMQTST_ASSERT_EQ(
        tester.configureHandle(
            "C1@a maxUnconfirmedMessages=11 maxUnconfirmedBytes=111"
            " consumerPriority=1 consumerPriorityCount=1"),
        0);

    // 3. Configure C1 for appId 'b' and verify it succeeds.
    BMQTST_ASSERT_EQ(
        tester.configureHandle(
            "C1@b maxUnconfirmedMessages=22 maxUnconfirmedBytes=222"
            " consumerPriority=1 consumerPriorityCount=1"),
        0);

    // 4. Configure C1 for appId 'c' and verify it succeeds.
    BMQTST_ASSERT_EQ(
        tester.configureHandle(
            "C1@c maxUnconfirmedMessages=33 maxUnconfirmedBytes=333"
            " consumerPriority=1 consumerPriorityCount=1"),
        0);

    // Verify expected
    PV(L_ << ": C1@a stream parameters:" << C1->_streamParameters("a"));
    PV(L_ << ": C1@b stream parameters:" << C1->_streamParameters("b"));
    PV(L_ << ": C1@c stream parameters:" << C1->_streamParameters("c"));

    BMQTST_ASSERT_EQ(
        bmqp::ProtocolUtil::consumerInfo(C1->_streamParameters("a"))
            .maxUnconfirmedMessages(),
        11);
    BMQTST_ASSERT_EQ(
        bmqp::ProtocolUtil::consumerInfo(C1->_streamParameters("a"))
            .maxUnconfirmedBytes(),
        111);

    BMQTST_ASSERT_EQ(
        bmqp::ProtocolUtil::consumerInfo(C1->_streamParameters("b"))
            .maxUnconfirmedMessages(),
        22);
    BMQTST_ASSERT_EQ(
        bmqp::ProtocolUtil::consumerInfo(C1->_streamParameters("b"))
            .maxUnconfirmedBytes(),
        222);

    BMQTST_ASSERT_EQ(
        bmqp::ProtocolUtil::consumerInfo(C1->_streamParameters("c"))
            .maxUnconfirmedMessages(),
        33);
    BMQTST_ASSERT_EQ(
        bmqp::ProtocolUtil::consumerInfo(C1->_streamParameters("c"))
            .maxUnconfirmedBytes(),
        333);
}

static void test28_releaseHandle()
// ------------------------------------------------------------------------
// RELEASE HANDLE
//
// Concerns:
//  1. Ensure that releasing a consumer with appId succeeds.
//  2. Ensure that releasing a producer succeeds.
//
// Plan:
//  1. Get handle for C1 with appId 'a' and verify it succeeds.
//  2. Get handle for C1 with appId 'b' and verify it succeeds.
//  3. Get handle for C2 with appId 'c' and verify it succeeds.
//  4. Get handle for C2 for a producer (i.e. no appId) and verify it
//     succeeds.
//  5. Release 'b' from C1 and verify it succeeds
//  6. Release 'c' from C2 and verify it succeeds
//  7. Get handle for C1 with appId 'c' and verify it succeeds
//  8. Get handle for C2 with appId 'b' and verify it succeeds
//  9. Release one writer from C2 and verify it succeeds
//  10. Release 'a' and 'c' from C1, 'b' from C2 and verify these succeed
//
// Testing:
//  void releaseHandle(
//      mqbi::QueueHandle                                *handle,
//      const bmqp_ctrlmsg::QueueHandleParameters&        handleParameters,
//      bool                                              isFinal,
//      const mqbi::QueueHandle::HandleReleasedCallback&  releasedCb)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RELEASE HANDLE");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1. Bring up a consumer C1 with appId 'a'.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1->_appIds(), "a");

    // 2. Get handle for C1 with appId 'b' and verify it succeeds.
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@b readCount=1"));
    BMQTST_ASSERT_EQ(C1->_appIds(), "a,b");

    // 3. Get handle for C2 with appId 'c' and verify it succeeds.
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@c readCount=1");
    BMQTST_ASSERT_NE(C2, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C2->_appIds(), "c");

    // 4. Get handle for C2 for a producer (i.e. no appId) and verify it
    //    succeeds.
    BMQTST_ASSERT_EQ(C2, tester.getHandle("C2 writeCount=1"));
    BMQTST_ASSERT_EQ(C2->_appIds(), "-,c");

    // 5. Release 'b' from C1 and verify it succeeds
    BMQTST_ASSERT_EQ(tester.releaseHandle("C1@b readCount=1"), 0);
    BMQTST_ASSERT_EQ(C1->_appIds(), "a");

    // 6. Release 'c' from C2 and verify it succeeds
    BMQTST_ASSERT_EQ(tester.releaseHandle("C2@c readCount=1"), 0);
    BMQTST_ASSERT_EQ(C2->_appIds(), "-");

    // 7. Get handle for C1 with appId 'c' and verify it succeeds
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@c readCount=1"));
    BMQTST_ASSERT_EQ(C1->_appIds(), "a,c");

    // 8. Get handle for C2 with appId 'b' and verify it succeeds
    BMQTST_ASSERT_EQ(C2, tester.getHandle("C2@b readCount=1"));
    BMQTST_ASSERT_EQ(C2->_appIds(), "-,b");

    // 9. Release one writer from C2 and verify it succeeds
    BMQTST_ASSERT_EQ(tester.releaseHandle("C2 writeCount=1"), 0);
    BMQTST_ASSERT_EQ(C2->_appIds(), "b");

    // 10. Release 'a' and 'c' from C1, 'b' from C2 and verify these succeed
    BMQTST_ASSERT_EQ(tester.releaseHandle("C1@a readCount=1"), 0);
    BMQTST_ASSERT_EQ(tester.releaseHandle("C1@c readCount=1"), 0);
    BMQTST_ASSERT_EQ(tester.releaseHandle("C2@b readCount=1"), 0);

    BMQTST_ASSERT_EQ(C1->_appIds(), "");
    BMQTST_ASSERT_EQ(C2->_appIds(), "");
}

static void test29_releaseHandleMultipleProducers()
// ------------------------------------------------------------------------
// RELEASE HANDLE - MULTIPLE PRODUCERS
//
// Concerns:
//  1. Ensure that releasing a handle having multiple producers succeeds,
//     in that write capacity is maintained on the handle so longs as not
//     all producers have been released (unlike consumer subStreams which
//     are limited to a readCount of 1).
//
// Plan:
//  1. Get a handle C1 with 2 producers.
//  2. Release one writer from C1, verify it succeeds, and verify that
//     the default subStream (i.e. producer subStream) still exists.
//  3. Release one writer from C1, verify it succeeds, and verify that the
//     default subStream does not exist.
//
// Testing:
//  void releaseHandle(
//      mqbi::QueueHandle                                *handle,
//      const bmqp_ctrlmsg::QueueHandleParameters&        handleParameters,
//      bool                                              isFinal,
//      const mqbi::QueueHandle::HandleReleasedCallback&  releasedCb)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RELEASE HANDLE - MULTIPLE PRODUCERS");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    //  1. Get a handle C1 with 2 producers.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 writeCount=1");
    BMQTST_ASSERT_EQ(tester.getHandle("C1 writeCount=1"), C1);
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1->_numActiveSubstreams(), 1U);
    BMQTST_ASSERT_EQ(C1->_appIds(), "-");
    BMQTST_ASSERT_EQ(C1->handleParameters().writeCount(), 2);

    // 2. Release one writer from C1, verify it succeeds, and verify that
    //    the default subStream (i.e. producer subStream) still exists.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C1 writeCount=1"), 0);
    BMQTST_ASSERT_EQ(C1->_numActiveSubstreams(), 1U);
    BMQTST_ASSERT_EQ(C1->_appIds(), "-");
    BMQTST_ASSERT_EQ(C1->handleParameters().writeCount(), 1);

    // 3. Release one writer from C1, verify it succeeds, and verify that the
    //    default subStream does not exist.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C1 writeCount=1"), 0);
    BMQTST_ASSERT_EQ(C1->_numActiveSubstreams(), 0U);
    BMQTST_ASSERT_EQ(C1->_appIds(), "");
    BMQTST_ASSERT_EQ(C1->handleParameters().writeCount(), 0);
}

static void test30_releaseHandle_isDeletedFlag()
// ------------------------------------------------------------------------
// RELEASE HANDLE - IS-DELETED FLAG
//
// Concerns:
//   1. Ensure that releasing a handle fully (i.e., all counts go to zero
//      'isFinal=true' is explicitly specified) results in the handle being
//      deleted, as indicated by the 'isDeleted' flag propagated via the
//      'releaseHandle' callback invoation.
//
// Plan:
//   1. Bring up consumers:
//      - C1 with writeCount=1 having appId 'a' (with readCount=1)
//      - C2 with writeCount=1 having appId 'b' (with readCount=1)
//      - C3@c (with readCount=1)
//      - C4@d (with readCount=1)
//   2. Release from C1 one reader but pass 'isFinal=true' and verify that
//      C1 was fully deleted.
//   3. Release from C2 one reader and verify that it *WAS NOT* deleted.
//   4. Release from C2 one writer but pass 'isFinal=false' and verify that
//      C2 was fully deleted.
//   5. Release from C3@c one reader but pass 'isFinal=false' and verify
//      that C3 was fully deleted.
//   6. Release from C4@d one reader, pass 'isFinal=true' (i.e., the
//      correct scenario), and verify that C4 was fully deleted.
//
// Testing:
//  void releaseHandle(
//      mqbi::QueueHandle                                *handle,
//      const bmqp_ctrlmsg::QueueHandleParameters&        handleParameters,
//      bool                                              isFinal,
//      const mqbi::QueueHandle::HandleReleasedCallback&  releasedCb)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("RELEASE HANDLE - IS-DELETED FLAG");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c,d"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    mqbmock::QueueHandle *C1, *C2, *C3, *C4;
    // 1. Bring up consumers:
    //    - C1 with writeCount=1 having appId 'a' (with readCount=1)
    //    - C2 with writeCount=1 having appId 'b' (with readCount=1)
    //    - C3 with writeCount=1 having appId 'c' (with readCount=1)
    C1 = tester.getHandle("C1 writeCount=1");
    C1 = tester.getHandle("C1@a readCount=1");

    C2 = tester.getHandle("C2 writeCount=1");
    C2 = tester.getHandle("C2@b readCount=1");

    C3 = tester.getHandle("C3@c readCount=1");

    C4 = tester.getHandle("C4@d readCount=1");

    // TEST PRECONDITIONS
    BSLS_ASSERT_OPT(C1->_numActiveSubstreams() == 2U);
    BSLS_ASSERT_OPT(C2->_numActiveSubstreams() == 2U);
    BSLS_ASSERT_OPT(C3->_numActiveSubstreams() == 1U);

    BSLS_ASSERT_OPT(C1->_appIds() == "-,a");
    BSLS_ASSERT_OPT(C2->_appIds() == "-,b");
    BSLS_ASSERT_OPT(C3->_appIds() == "c");

    bool isDeleted = false;
    // 2. Release from C1 one reader but pass 'isFinal=true' and verify that
    //    C1 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C1@a readCount=1 isFinal=true",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);
    BMQTST_ASSERT_EQ(C1->_numActiveSubstreams(), 0U);
    BMQTST_ASSERT_EQ(C1->_appIds(), "");

    // 3. Release from C2 one reader and verify that C2 *WAS NOT* deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C2@b readCount=1", &isDeleted), 0);
    BMQTST_ASSERT_EQ(isDeleted, false);
    BMQTST_ASSERT_EQ(C2->_numActiveSubstreams(), 1U);
    BMQTST_ASSERT_EQ(C2->_appIds(), "-");

    // 4. Release from C2 one writer but pass 'isFinal=false' and verify that
    //    C2 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C2 writeCount=1 isFinal=false",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);
    BMQTST_ASSERT_EQ(C2->_numActiveSubstreams(), 0U);
    BMQTST_ASSERT_EQ(C2->_appIds(), "");

    // 5. Release from C3@c one reader but pass 'isFinal=false' and verify
    //    that C3 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C3@c readCount=1 isFinal=true",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);
    BMQTST_ASSERT_EQ(C1->_numActiveSubstreams(), 0U);
    BMQTST_ASSERT_EQ(C1->_appIds(), "");

    // 6. Release from C4@d one reader, pass 'isFinal=true' (i.e., the correct
    //    scenario), and verify that C4 was fully deleted.
    BMQTST_ASSERT_EQ(tester.releaseHandle("C4@d readCount=1 isFinal=true",
                                          &isDeleted),
                     0);
    BMQTST_ASSERT_EQ(isDeleted, true);
    BMQTST_ASSERT_EQ(C4->_numActiveSubstreams(), 0U);
    BMQTST_ASSERT_EQ(C4->_appIds(), "");
}

static void test31_afterNewMessageDeliverToAllActiveConsumers()
// ------------------------------------------------------------------------
// AFTER NEW MESSAGE - DELIVER TO ALL ACTIVE CONSUMERS
//
// Concerns:
//  1. FanoutQueueEngine delivers a new message to all active consumers.
//  2. Producers do not receive messages.
//
// Plan:
//  1. Bring up 5 active consumers and 1 producer.
//      - handle C1 has 2 consumers ("a,b")
//      - handle C2 has 2 consumers ("c,d") and 1 producer
//      - handle C3 has 1 consumer ("e")
//  2. Post 3 messages to the queue and invoked the engine to deliver them.
//  3. Verify that every active consumer received all 3 messages, and that
//     no producer received any message.
//
// Testing:
//  virtual void afterNewMessage(const bmqt::MessageGUID&  msgGUID,
//                               mqbi::QueueHandle        *source)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName(
        "AFTER NEW MESSAGE - DELIVER TO ALL ACTIVE CONSUMERS");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c,d,e"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    mqbmock::QueueHandle* C1 = static_cast<mqbmock::QueueHandle*>(0);
    mqbmock::QueueHandle* C2 = static_cast<mqbmock::QueueHandle*>(0);
    mqbmock::QueueHandle* C3 = static_cast<mqbmock::QueueHandle*>(0);

    // 1. Bring up 5 active consumers and 1 producer.
    //     - handle C1 has 2 consumers ("a,b")
    //     - handle C2 has 2 consumers ("c,d") and 1 producer
    //     - handle C3 has 1 consumer ("e")
    // C1
    C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_EQ(tester.getHandle("C1@b readCount=1"), C1);
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1->_appIds(), "a,b");

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@a maxUnconfirmedMessages=10"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@b maxUnconfirmedMessages=10"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // C2
    C2 = tester.getHandle("C2@c readCount=1");
    BMQTST_ASSERT_EQ(tester.getHandle("C2@d readCount=1"), C2);
    BMQTST_ASSERT_EQ(tester.getHandle("C2   writeCount=1"), C2);
    BMQTST_ASSERT_NE(C2, k_nullMockHandle_p);
    BMQTST_ASSERT_NE(C2, C1)
    BMQTST_ASSERT_EQ(C2->_appIds(), "-,c,d");

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@c maxUnconfirmedMessages=10"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@d maxUnconfirmedMessages=10"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // C3
    C3 = tester.getHandle("C3@e readCount=1");
    BMQTST_ASSERT_NE(C3, k_nullMockHandle_p);
    BMQTST_ASSERT_NE(C3, C1);
    BMQTST_ASSERT_NE(C3, C2);
    BMQTST_ASSERT_EQ(C3->_appIds(), "e");

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C3@e maxUnconfirmedMessages=10"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // 2. Post 3 messages to the queue and invoked the engine to deliver them.
    tester.post("1");
    tester.post("2");
    tester.post("3");

    // 3. Verify that every active consumer received all 3 messages, and that
    //    no producer received any message.
    PVV(L_ << ": afterNewMessage ['1','2','3']");

    tester.afterNewMessage(3);

    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));
    PVV(L_ << ": C2@- Messages: " << C2->_messages(""));
    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));
    PVV(L_ << ": C2@d Messages: " << C2->_messages("d"));
    PVV(L_ << ": C3@e Messages: " << C3->_messages("e"));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 3);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 3);
    BMQTST_ASSERT_EQ(C2->_numMessages(""), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 3);
    BMQTST_ASSERT_EQ(C2->_numMessages("d"), 3);
    BMQTST_ASSERT_EQ(C3->_numMessages("e"), 3);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2,3");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "1,2,3");
    BMQTST_ASSERT_EQ(C2->_messages(""), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "1,2,3");
    BMQTST_ASSERT_EQ(C2->_messages("d"), "1,2,3");
    BMQTST_ASSERT_EQ(C3->_messages("e"), "1,2,3");
}

static void test32_afterNewMessageRespectsFlowControl()
// ------------------------------------------------------------------------
// AFTER NEW MESSAGE - RESPECT FLOW CONTROL
//
// Concerns:
//  1. After a new message comes in, if it is not possible to deliver the
//     message to a consumer, then the engine does not attempt to deliver
//     the message to the consumer at that time.
//  2. When it becomes possible to deliver to a consumer , the engine
//     delivers to it as many messages as possible starting at the first
//     message that the consumer missed when delivery was not possible for
//     it.
//
// Plan:
//  1. Bring up 3 consumers and 1 producer.
//      1. handle C1 has 1 active consumer ("a") and 1 inactive consumer
//        ("b")
//      2. handle C2 has 1 active consumer ("c") and 1 producer
//  2. Post 2 message to the queue and invoke the engine to deliver them.
//  3. Verify that C1@a and C2@c received the messages but C1@b did not.
//  4. Reconfigure C1@b such that it becomes possible to deliver to it.
//     Verify that the engine delivers to it the messages that it missed.
//  5. Post 2 messages to the queue and invoke the engine to deliver them.
//     Verify that C1@a, C1@b, and C2@c all receive those messages.
//  6. Reconfigure C2@c with maxUnconfirmed == 0, such that it is no longer
//     possible to deliver messages to it.
//  7. Post 2 message to the queue and invoke the engine to deliver them.
//     Verify that C1@a and C1@b received the messages but C2@c did not.
//  8. Reconfigure C2@c such that it becomes possible to deliver to it.
//     Verify that the engine delivers to it the messages that it missed.
//  9. Post 2 messages to the queue and invoke the engine to deliver them.
//     Verify that every consumer received all messages.
//  10. Disable delivery for C1@a and C1@b.
//  11. Post 2 messages to the queue and invoke the engine to deliver them.
//  12  Verify that C2@c received the messages but C1@a and C1@b did not.
//      Confirm the messages for C2@c.
//  13. Enable delivery for C1@a and C1@b.  Verify that they received the
//      messages that they missed.  Confirm all messages for C1@a and C1@b.
//  14. Bring up 2 consumers: handle C3 has 2 active consumers ("d,e").
//      Verify that they received all the messages that they missed.
//
// Testing:
//  virtual void configureHandle
//  virtual void afterNewMessage
//  virtual void onHandleUsable
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName(
        "AFTER NEW MESSAGE - RESPECT FLOW CONTROL");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c,d,e"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    mqbmock::QueueHandle* C1 = static_cast<mqbmock::QueueHandle*>(0);
    mqbmock::QueueHandle* C2 = static_cast<mqbmock::QueueHandle*>(0);
    mqbmock::QueueHandle* C3 = static_cast<mqbmock::QueueHandle*>(0);

    // 1. Bring up 3 consumers and 1 producer.
    //     1. handle C1 has 1 active consumer ("a") and 1 inactive consumer
    //       ("b")
    //     2. handle C2 has 1 active consumer ("c") and 1 producer
    //
    // C1
    C1 = tester.getHandle("C1@a readCount=1");
    C1 = tester.getHandle("C1@b readCount=1");
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@a maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@b maxUnconfirmedMessages=0"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);
    BMQTST_ASSERT_EQ(C1->_appIds(), "a,b");

    // C2
    C2 = tester.getHandle("C2@c readCount=1");
    C2 = tester.getHandle("C2 writeCount=1");
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@c maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);
    BMQTST_ASSERT_EQ(C2->_appIds(), "-,c");

    //  2. Post 2 message to the queue and invoke the engine to deliver them.
    tester.post("1,2");
    tester.afterNewMessage(2);

    //  3. Verify that C1@a and C2@c received the messages but C1@b did not.
    PVV(L_ << ": afterNewMessage ['1','2']");

    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));
    PVV(L_ << ": C2@- Messages: " << C2->_messages(""));
    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages(""), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 2);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "");
    BMQTST_ASSERT_EQ(C2->_messages(""), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "1,2");

    //  4. Reconfigure C1@b such that it becomes possible to deliver to it.
    //     Verify that the engine delivers to it the messages that it missed.
    PVV(L_ << ": configureHandle(\"C1@b maxUnconfirmedMessages=100\")");

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@b maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));

    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 2);
    BMQTST_ASSERT_EQ(C1->_messages("b"), "1,2");

    //  5. Post 2 messages to the queue and invoke the engine to deliver them.
    //     Verify that C1@a, C1@b, and C2@c all receive those messages.
    tester.post("3,4");

    PVV(L_ << ": afterNewMessage ['3','4']");

    tester.afterNewMessage(2);

    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));
    PVV(L_ << ": C2@- Messages: " << C2->_messages(""));
    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 4);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 4);
    BMQTST_ASSERT_EQ(C2->_numMessages(""), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 4);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2,3,4");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "1,2,3,4");
    BMQTST_ASSERT_EQ(C2->_messages(""), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "1,2,3,4");

    //  6. Reconfigure C2@c with maxUnconfirmed == 0, such that it is no longer
    //     possible to deliver messages to it.
    PVV(L_ << ": configureHandle(\"C2@c maxUnconfirmedMessages=0\")");

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@c maxUnconfirmedMessages=0"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // 7. Post 2 message to the queue and invoke the engine to deliver them.
    //   Verify that C1@a and C1@b received the messages but C2@c did not.
    tester.post("5,6");

    PVV(L_ << ": afterNewMessage ['5','6']");

    tester.afterNewMessage(2);

    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));
    PVV(L_ << ": C2@- Messages: " << C2->_messages(""));
    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 6);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 6);
    BMQTST_ASSERT_EQ(C2->_numMessages(""), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 4);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2,3,4,5,6");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "1,2,3,4,5,6");
    BMQTST_ASSERT_EQ(C2->_messages(""), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "1,2,3,4");

    // 8. Reconfigure C2@c such that it becomes possible to deliver to it.
    //    Verify that the engine delivers to it the messages that it missed.
    PVV(L_ << ": configureHandle(\"C2@c maxUnconfirmedMessages=100\")");

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@c maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 6);
    BMQTST_ASSERT_EQ(C1->_messages("b"), "1,2,3,4,5,6");

    // 9. Post 2 messages to the queue and invoke the engine to deliver them.
    //    Verify that every consumer received all messages.
    tester.post("7,8");

    PVV(L_ << ": afterNewMessage ['7','8']");

    tester.afterNewMessage(2);

    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));
    PVV(L_ << ": C2@- Messages: " << C2->_messages(""));
    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 8);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 8);
    BMQTST_ASSERT_EQ(C2->_numMessages(""), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 8);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2,3,4,5,6,7,8");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "1,2,3,4,5,6,7,8");
    BMQTST_ASSERT_EQ(C2->_messages(""), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "1,2,3,4,5,6,7,8");

    // 10. Disable delivery for C1@a and C1@b.
    PVV(L_ << ": C1@a canDeliver = false");
    PVV(L_ << ": C1@b canDeliver = false");

    C1->_setCanDeliver("a", false)._setCanDeliver("b", false);

    // 11. Post 2 messages to the queue and invoke the engine to deliver them.
    tester.post("9, 10");

    PVV(L_ << ": afterNewMessage ['9','10']");

    tester.afterNewMessage(2);

    // 12. Verify that C2@c received the messages but C1@a and C1@b did not.
    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));
    PVV(L_ << ": C2@- Messages: " << C2->_messages(""));
    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 8);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 8);
    BMQTST_ASSERT_EQ(C2->_numMessages(""), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 10);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2,3,4,5,6,7,8");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "1,2,3,4,5,6,7,8");
    BMQTST_ASSERT_EQ(C2->_messages(""), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "1,2,3,4,5,6,7,8,9,10");

    // Confirm the messages for C2@c
    PVV(L_ << ": C2@c confirm ['1','2','3','4','5','6','7','8','9','10']");

    tester.confirm("C2@c", "1,2,3,4,5,6,7,8,9,10");

    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 0);
    BMQTST_ASSERT_EQ(C2->_messages("c"), "");

    // 13. Enable delivery for C1@a and C1@b.  Verify that they received the
    //     messages that they missed.
    PVV(L_ << ": C1@a canDeliver = true");
    PVV(L_ << ": C1@b canDeliver = true");

    C1->_setCanDeliver("a", true)._setCanDeliver("b", true);

    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));
    PVV(L_ << ": C2@- Messages: " << C2->_messages(""));
    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 10);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 10);
    BMQTST_ASSERT_EQ(C2->_numMessages(""), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 0);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2,3,4,5,6,7,8,9,10");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "1,2,3,4,5,6,7,8,9,10");
    BMQTST_ASSERT_EQ(C2->_messages(""), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "");

    // Confirm all messages for C1@a and C2@b
    PVV(L_ << ": C1@a confirm ['1','2','3','4','5','6','7','8','9','10']");
    PVV(L_ << ": C1@b confirm ['1','2','3','4','5','6','7','8','9','10']");

    tester.confirm("C1@a", "1,2,3,4,5,6,7,8,9,10");
    tester.confirm("C1@b", "1,2,3,4,5,6,7,8,9,10");

    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 0);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 0);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "");

    // 14. Bring up 2 consumers: handle C3 has 2 active consumers ("d,e").
    C3 = tester.getHandle("C3@d readCount=1");
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C3@d maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);
    BMQTST_ASSERT_EQ(tester.getHandle("C3@e readCount=1"), C3);
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C3@e maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);
    BMQTST_ASSERT_EQ(C3->_appIds(), "d,e");

    // Verify that they received all the messages that they missed.
    PVV(L_ << ": C3@d Messages: " << C3->_messages("d"));
    PVV(L_ << ": C3@e Messages: " << C3->_messages("e"));

    BMQTST_ASSERT_EQ(C3->_numMessages("d"), 10);
    BMQTST_ASSERT_EQ(C3->_numMessages("e"), 10);

    BMQTST_ASSERT_EQ(C3->_messages("d"), "1,2,3,4,5,6,7,8,9,10");
    BMQTST_ASSERT_EQ(C3->_messages("e"), "1,2,3,4,5,6,7,8,9,10");
}

static void test33_consumerUpAfterMessagePosted()
// ------------------------------------------------------------------------
// HANDLE CONSUMER SUBSCRIPTION AFTER POST
//
// Concerns:
//  1. If messages are posted when there are no active consumers, then they
//     are not discarded - they will be delivered to each configured
//     consumer as it comes up (unless TTL kicks in, etc.).
//
// Plan:
//  1. Post 3 messages to the queue and invoke the engine to deliver them.
//  2. Bring up 2 active consumers C1@a and C2@b.  Verify that each
//     receives the previously posted messages.  For C1@a, confirm the
//     messages.
//  3. Drop C1@a and C2@b.
//  4. Bring up active consumer C1@a.
//  5. Post 2 messages to the queue and invoke the engine to deliver them.
//     Verify that C1@a received the messages.  For C1@a, confirm the
//     messages.
//  6. Bring up active consumer C2@b.  Verify that C2@b received *all*
//     previously posted messages.  For C2@b, confirm all messages.
//
// Testing:
//  configureHandle
//  onConfirmMessage
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName(
        "HANDLE CONSUMER SUBSCRIPTION AFTER POST");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c,d,e"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    mqbmock::QueueHandle* C1 = static_cast<mqbmock::QueueHandle*>(0);
    mqbmock::QueueHandle* C2 = static_cast<mqbmock::QueueHandle*>(0);

    // 1. Post 3 messages to the queue and invoke the engine to deliver them.
    tester.post("1,2,3");

    PVV(L_ << ": afterNewMessage ['1','2','3']");
    tester.afterNewMessage(3);

    // 2. Bring up 2 active consumers C1@a and C2@b.
    C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@a maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    C2 = tester.getHandle("C2@b readCount=1");
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@b maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // Verify that each receives the previously posted messages.
    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C2@b Messages: [" << C2->_messages("b") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 3);
    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 3);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2,3");
    BMQTST_ASSERT_EQ(C2->_messages("b"), "1,2,3");

    // For C1@a, confirm the messages.
    PVV(L_ << ": C1@a confirm ['1','2','3']");

    tester.confirm("C1@a", "1,2,3");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 0);
    BMQTST_ASSERT_EQ(C1->_messages("a"), "");

    // 3. Drop C1@a and C2@b.
    tester.dropHandle("C1");
    tester.dropHandle("C2");

    // 4. Bring up active consumer C1@a.
    C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@a maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // 5. Post 2 messages to the queue and invoke the engine to deliver them.
    tester.post("4,5");

    PVV(L_ << ": afterNewMessage ['4','5']");
    tester.afterNewMessage(2);

    // Verify that C1@a received the messages.
    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    BMQTST_ASSERT_EQ(C1->_messages("a"), "4,5");

    //  For C1@a, confirm the messages.
    PVV(L_ << ": C1@a confirm ['4','5']");

    tester.confirm("C1@a", "4,5");

    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 0);
    BMQTST_ASSERT_EQ(C1->_messages("a"), "");

    // 6. Bring up active consumer C2@b.
    C2 = tester.getHandle("C2@b readCount=1");
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@b maxUnconfirmedMessages=100"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // Verify that C2@b received *all* previously posted messages.
    PVV(L_ << ": C2@b Messages: " << C2->_messages("b"));

    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 5);
    BMQTST_ASSERT_EQ(C2->_messages("b"), "1,2,3,4,5");

    // For C2@b, confirm all messages.
    PVV(L_ << ": C2@b confirm ['1','2','3','4','5']");

    tester.confirm("C2@b", "1,2,3,4,5");

    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 0);
    BMQTST_ASSERT_EQ(C2->_messages("b"), "");

    PVV(L_ << ": C2@b Messages: [" << C2->_messages("b") << "]");
}

static void test34_beforeMessageRemoved_deadConsumers()
// ------------------------------------------------------------------------
// BEFORE MESSAGE REMOVED - DEAD CONSUMERS
//
// Concerns:
//   1. If the queue engine is notified that a message is about to be
//      removed from the queue (e.g. it's TTL expired, it was deleted by
//      admin task, confirmed by all recipients, etc.), then:
//      a) It updates its position in the stream of the queue to advance
//         past the message accordingly (if needed).
//      b) It updates the positions of all configured consumers to advance
//         past the message accordingly (if needed).
//
// Plan:
//   1. Post 3 messages to the queue.
//   2. Simulate queue garbage collection (e.g., TTL expiration) of the
//      first 2 messages (this involves invoking
//      'beforeMessageRemoved(msgGUID)' followed by removing the message
//      from storage -- for each message in turn).
//   4. Bring up 3 active consumers and configure them.
//      - handle C1 has 2 consumers ("a,b")
//      - handle C2 has 1 consumers ("c")
//   5. Verify that *only* the last message was delivered to all consumers.
//
// Testing:
//   Queue Engine adjusting its position in the stream of the queue upon
//   receiving notification prior to a message being removed, specifically
//   with regards to configured consumers that are not alive at the time.
//   - 'beforeMessageRemoved()'
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BEFORE MESSAGE REMOVED - DEAD"
                                      " CONSUMERS");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    tester.getHandle("Temp@a readCount=1");

    // 1. Post 3 messages to the queue.
    PVV(L_ << ": post ['1','2','3']");
    tester.post("1,2,3");

    // 2. Simulate queue garbage collection (e.g., TTL expiration) of the
    //    first 2 messages (this involves invoking
    //    'beforeMessageRemoved(msgGUID)' followed by removing the message
    //    from storage -- for each message in turn).
    PVV(L_ << ": Garbage collect (TTL expiration) ['1','2']");
    tester.garbageCollectMessages(2);

    tester.dropHandle("Temp");

    // 4. Bring up 3 active consumers and configure them.
    //    - handle C1 has 2 consumers ("a,b")
    //    - handle C2 has 1 consumers ("c")
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    C1                       = tester.getHandle("C1@b readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@c readCount=1");

    tester.configureHandle("C1@a consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C1@b consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2@c consumerPriority=1 consumerPriorityCount=1");

    // 5. Verify that *only* the last message was delivered to all consumers.
    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 1);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 1);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "3");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "3");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "3");
}

static void test35_beforeMessageRemoved_withActiveConsumers()
// ------------------------------------------------------------------------
// BEFORE MESSAGE REMOVED - WITH ALIVE CONSUMERS
//
// Concerns:
//   1. If the queue engine is notified that a message is about to be
//      removed from the queue (e.g. it's TTL expired, it was deleted by
//      admin task, confirmed by all recipients, etc.), then:
//      a) It updates its position in the stream of the queue to advance
//         past the message accordingly (if needed).
//      b) It updates the positions of all configured consumers to advance
//         past the message accordingly (if needed)
//
// Plan:
//   1. Bring up 3 active consumers and configure them.
//      - handle C1 has 2 consumers ("a,b")
//      - handle C2 has 1 consumers ("c")
//  2. Post 3 messages to the queue (without yet invoking the engine to
//     deliver them).
//  3. Simulate queue garbage collection (e.g., TTL expiration) of the
//     first 2 messages (this involves invoking
//     'beforeMessageRemoved(msgGUID)' followed by removing the message
//     from storage -- for each message in turn).
//  4. Invoke message delivery in the engine and verify that *only* the
//     last message was delivered to all consumers.
//
// Testing:
//   Queue Engine adjusting its position in the stream of the queue upon
//   receiving notification prior to a message being removed.
//   - 'beforeMessageRemoved()'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BEFORE MESSAGE REMOVED - WITH ALIVE"
                                      " CONSUMERS");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1. Bring up 3 active consumers and configure them.
    //    - handle C1 has 2 consumers ("a,b")
    //    - handle C2 has 1 consumers ("c")
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    C1                       = tester.getHandle("C1@b readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@c readCount=1");

    tester.configureHandle("C1@a consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C1@b consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2@c consumerPriority=1 consumerPriorityCount=1");

    // 2. Post 3 messages to the queue (without yet invoking the engine to
    //    deliver them).
    PVV(L_ << ": post ['1','2','3']");
    tester.post("1,2,3");

    // 3. Simulate queue garbage collection (e.g., TTL expiration) of the
    //    first 2 messages (this involves invoking
    //    'beforeMessageRemoved(msgGUID)' followed by removing the message
    //    from storage -- for each message in turn).
    PVV(L_ << ": Garbage collect (TTL expiration) ['1','2']");
    tester.garbageCollectMessages(2);

    //  3. Invoke message delivery in the engine and verify that only the last
    //     message was delivered to all consumers.

    // 4. Invoke message delivery in the engine and verify that *only* the last
    //    message was delivered to all consumers.
    tester.afterNewMessage(0);

    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 1);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 1);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "3");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "3");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "3");
}

static void test36_afterQueuePurged_queueStreamResets()
// ------------------------------------------------------------------------
// AFTER QUEUE PURGED - QUEUE STREAM RESETS
//
// Concerns:
//   If the Queue Engine is notified that the queue *was* purged (i.e.,
//   after it occurred), then undelivered messages from prior to the purge
//   are not delivered to any consumer.
//
// Plan:
//   1. Post two messages, attempt to deliver, and then purge the queue and
//      notify the engine.
//   2. Bring up 3 active consumers and configure them with
//      'maxUnconfirmedMessages=2':
//      - handle C1 has 2 consumers ("a,b")
//      - handle C2 has 1 consumers ("c")
//      Verify that neither received any messages.
//   3. Post 4 messages, deliver, and verify that every consumer received
//      the first 2 messages.
//   4. Purge the queue, then notify the queue engine that the queue was
//      purged.
//   5. For every consumer, confirm its messages (thus making it
//      unsaturated and capable of receiving messages) and verify that it
//      did not receive any additional messages (purge should have wiped
//      them out).
//   6. Post 2 messages, deliver, and verify that every consumer received
//      both messages.
//
// Testing:
//   Queue Engine resetting the stream of the queue upon receiving
//   notification that the queue was purged.
//   - 'afterQueuePurged()'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BEFORE MESSAGE REMOVED - WITH ALIVE"
                                      " CONSUMERS");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1. Post two messages, attempt to deliver, and then purge the queue and
    //    notify the engine.
    PVV(L_ << ": post ['1','2']");
    tester.post("1,2");

    PVV(L_ << ": afterNewMessage ['1','2']");
    tester.afterNewMessage(2);

    PVV(L_ << ": --> purgeQueue <--");
    tester.purgeQueue();
    PVV(L_ << ": afterQueuePurged ... ");

    // 2. Bring up 3 active consumers and configure them with
    //    'maxUnconfirmedMessages=2':
    //    - handle C1 has 2 consumers ("a,b")
    //    - handle C2 has 1 consumers ("c")
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    C1                       = tester.getHandle("C1@b readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@c readCount=1");

    tester.configureHandle("C1@a maxUnconfirmedMessages=2"
                           " consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C1@b maxUnconfirmedMessages=2"
                           " consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2@c maxUnconfirmedMessages=2"
                           " consumerPriority=1 consumerPriorityCount=1");

    //    Verify that neither received any messages.
    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 0);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 0);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "");

    // 3. Post 4 messages, deliver, and verify that every consumer received
    //    the first 2 messages.
    PVV(L_ << ": post ['11','12','13','14']");
    PVV(L_ << ": afterNewMessage ['11','12','13','14']");
    tester.post("11,12");
    tester.afterNewMessage(2);

    C1->_setCanDeliver("a", false)._setCanDeliver("b", false);
    C2->_setCanDeliver("c", false);

    tester.post("13,14");
    tester.afterNewMessage(2);

    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 2);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "11,12");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "11,12");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "11,12");

    // 5. Purge the queue, then notify the queue engine that the queue was
    //    purged.
    PVV(L_ << ": --> purgeQueue <--");
    tester.purgeQueue();
    PVV(L_ << ": afterQueuePurged ... ");

    // 6. For every consumer, confirm its messages (thus making it unsaturated
    //    and capable of receiving messages) and verify that it did not receive
    //    any additional messages (purge should have wiped them out).
    tester.confirm("C1@a", "11,12");
    tester.confirm("C1@b", "11,12");
    tester.confirm("C2@c", "11,12");

    C1->_setCanDeliver("a", true)._setCanDeliver("b", true);
    C2->_setCanDeliver("c", true);

    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 0);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 0);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "");

    // 7. Post 2 messages, deliver, and verify that every consumer received
    //    both messages.
    PVV(L_ << ": post ['21','22']");
    tester.post("21,22");

    PVV(L_ << ": afterNewMessage ['21','22']");
    tester.afterNewMessage(2);

    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 2);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "21,22");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "21,22");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "21,22");
}

static void test37_afterQueuePurged_specificSubStreamResets()
// ------------------------------------------------------------------------
// AFTER QUEUE PURGED - SPECIFIC SUB STREAM RESETS
//
// Concerns:
//   If the Queue Engine is notified that a specific subStream of the
//   queue *was* purged (i.e., after it occurred), then undelivered
//   messages from prior to the purge are not to be delivered to the
//   consumer of that subStream.
//
// Plan:
//   1. Post 2 messages, attempt to deliver, and then purge the "b"
//      subStream of the queue and notify the engine.
//   2. Bring up 3 active consumers and configure them:
//      - handle C1 has 2 consumers ("a,b")
//      - handle C2 has 1 consumers ("c")
//      Verify that C1@a and C2@c both received 2 messages and C1@b did not
//      receive any messages.
//   3. Post 2 messages, then purge the "a" and "c" subStreams of the queue
//      and notify the engine.
//   4. Attempt to deliver new messages and verify that C1@b received the
//      2 new messages while C1@a and C2@c did not receive any new
//      messages.
//   5. For every consumer, confirm its messages and verify that it did not
//      receive any additional messages (purge should have wiped them out)
//      as well as that confirmation was completed gracefully (it is
//      *valid* to attempt to confirm messages after they were purged, and
//      the queue engine *must* handle that gracefully).
//   6. Post 2 messages, deliver, and verify that every consumer received
//      both messages.
//
// Testing:
//   Queue Engine resetting a specific subStream of the queue upon
//   receiving notification that the subStream of the queue was purged.
//   - 'afterQueuePurged(<appKey>)', where '<appKey>' corresponds to a
//     specific subStream of the queue.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("BEFORE MESSAGE REMOVED - WITH ALIVE"
                                      " CONSUMERS");

    mqbblp::QueueEngineTester tester(fanoutConfig("a,b,c"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1. Post 2 messages, attempt to deliver, and then purge the "b" subStream
    //    of the queue and notify the engine.
    PVV(L_ << ": post ['1','2']");
    tester.post("1,2");

    PVV(L_ << ": afterNewMessage ['1','2']");
    tester.afterNewMessage(2);

    PVV(L_ << ": --> purgeQueue('b') <--");
    tester.purgeQueue("b");
    PVV(L_ << ": afterQueuePurged('b) ... ");

    // 2. Bring up 3 active consumers and configure them:
    //    - handle C1 has 2 consumers ("a,b")
    //    - handle C2 has 1 consumers ("c")
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    C1                       = tester.getHandle("C1@b readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@c readCount=1");

    tester.configureHandle("C1@a consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C1@b consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2@c consumerPriority=1 consumerPriorityCount=1");

    // Verify that C1@a and C2@c both received 2 messages and C1@b did not
    // receive any messages.
    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 2);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "1,2");

    // 3. Post 2 messages, then purge the "a" and "c" subStreams of the queue
    //    and notify the engine.
    PVV(L_ << ": post ['11','12']");
    tester.post("11,12");

    PVV(L_ << ": --> purgeQueue('a') <--");
    tester.purgeQueue("a");
    PVV(L_ << ": afterQueuePurged('a') ... ");

    PVV(L_ << ": --> purgeQueue('c') <--");
    tester.purgeQueue("c");
    PVV(L_ << ": afterQueuePurged('c') ... ");

    // 4. Attempt to deliver new messages and verify that C1@b received the 2
    //    new messages while C1@a and C2@c did not receive any new messages.
    PVV(L_ << ": afterNewMessage ['11','12']");
    tester.afterNewMessage(2);

    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 2);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "11,12");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "1,2");

    // 5. For every consumer, confirm its messages and verify that it did not
    //    receive any additional messages (purge should have wiped them out) as
    //    well as that confirmation was completed gracefully (it is *valid* to
    //    attempt to confirm messages after they were purged, and the queue
    //    engine *must* handle that gracefully).
    PVV(L_ << ": C1@a confirm ['1', '2']");
    tester.confirm("C1@a", "1,2");

    PVV(L_ << ": C1@a confirm ['11', '12']");
    tester.confirm("C1@b", "11,12");

    PVV(L_ << ": C2@c confirm ['1', '2']");
    tester.confirm("C2@c", "1,2");

    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 0);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 0);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "");

    // 6. Post 2 messages, deliver, and verify that every consumer received
    //    both messages.
    PVV(L_ << ": post ['21','22']");
    tester.post("21,22");

    PVV(L_ << ": afterNewMessage ['21','22']");
    tester.afterNewMessage(2);

    PVV(L_ << ": C1@a Messages: [" << C1->_messages("a") << "]");
    PVV(L_ << ": C1@b Messages: [" << C1->_messages("b") << "]");
    PVV(L_ << ": C2@c Messages: [" << C2->_messages("c") << "]");

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 2);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "21,22");
    BMQTST_ASSERT_EQ(C1->_messages("b"), "21,22");
    BMQTST_ASSERT_EQ(C2->_messages("c"), "21,22");
}

static void test38_unauthorizedAppIds()
// ------------------------------------------------------------------------
// UNAUTHORIZED APP IDS
//
// Concerns:
//  1. FanoutQueueEngine delivers a new message to all active consumers.
//  2. Consumers using unauthorized appIds do not receive messages.
//  3. Messages confirmed by consumers using authorized appIds are removed
//     from queue.
//
// Plan:
//  1. Bring up 1 consumer that opens a queue with an authorized appId,
//     and 1 consumer that opens a queue with an unauthorized appId.
//      - handle C1 has 1 consumers ("a")
//      - handle C2 has 1 consumer ("pikachu")
//  2. Post 3 messages to the queue and invoked the engine to deliver them.
//  3. Verify that C2 receives no message.
//  4. Verify that no message remains in queue after C1 confirms messages.
//
// Testing:
//  virtual void afterNewMessage()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("UNAUTHORIZED APP IDS - DELIVER ONLY TO "
                                      "CONSUMERS USING AUTHORIZED APPID");

    mqbblp::QueueEngineTester tester(fanoutConfig("a"),
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    mqbmock::QueueHandle* C1 = static_cast<mqbmock::QueueHandle*>(0);
    mqbmock::QueueHandle* C2 = static_cast<mqbmock::QueueHandle*>(0);

    // 1. Bring up 2 consumers.
    //     - handle C1 has 1 consumer using authorized appId ("a")
    //     - handle C2 has 1 consumer using unauthorized appId  ("pikachu")
    // C1
    C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@a maxUnconfirmedMessages=3"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // C2
    C2 = tester.getHandle("C2@pikachu readCount=1");
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@pikachu maxUnconfirmedMessages=3"
                         " consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // 2. Post 3 messages to the queue and invoked the engine to deliver them.
    tester.post("1");
    tester.post("2");
    tester.post("3");

    // 3. Verify that C1 received all 3 messages, and that C2 received none.
    PVV(L_ << ": afterNewMessage ['1','2','3']");

    tester.afterNewMessage(3);

    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C2@pikachu Messages: " << C2->_messages(""));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 3);
    BMQTST_ASSERT_EQ(C2->_numMessages(""), 0);
    BMQTST_ASSERT_EQ(C2->_numMessages("pikachu"), 0);

    // 4. Have C1 confirm all messages and check that the queue is empty.
    PVV(L_ << ": C1@a confirming ['1','2','3']");
    tester.confirm("C1@a", "1,2,3");
    BMQTST_ASSERT_EQ(C1->queue()->capacityMeter()->messages(), 0);
}

static void test39_maxConsumersProducers()
// ------------------------------------------------------------------------
// CONSUMERS/PRODUCERS LIMITS PER APP
//
// Concerns:
//   1. Enforcing maxConsumers/maxProducers configuration per App.
//
// Plan:
//  1. Set maxConsumers and maxProducers limits to the value (2) less than
//     number of applications (3).
//  2. Bring up a consumer C1 with appId 'a', 'b', and 'c'.
//  3. Bring up a consumer C2 with appId 'a'
//  4. Verify that attempt to bring up another consumer with appId 'a'
//     fails
//  5. Bring up a consumer C2 with appId 'b'
//  6. Verify that attempt to bring up another consumer with appId 'b' fails
//  7. Bring up a producer C1 with writeCount == 2
//  8. Verify that attempt to bring up another producer with fails
//
// Testing:
//   'getHandle' for multiple distinct appIds.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("CONSUMERS/PRODUCERS LIMITS PER APP");

    // 1. Set maxConsumers and maxProducers limits to the value (2) less than
    //    number of applications (3).
    mqbconfm::Domain config = fanoutConfig("a,b,c");
    config.maxProducers()   = 2;
    config.maxConsumers()   = 2;

    mqbblp::QueueEngineTester tester(config,
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 2. Bring up a consumer C1 with appId 'a', 'b', and 'c'.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@b readCount=1"));
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@c readCount=1"));

    // 3. Bring up a consumer C2 with appId 'a'
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@a readCount=1");
    BMQTST_ASSERT_NE(C2, k_nullMockHandle_p);

    // 4. Verify that attempt to bring up another consumer with appId 'a' fails
    BMQTST_ASSERT_EQ(k_nullMockHandle_p, tester.getHandle("C2@a readCount=1"));

    // 5. Bring up a consumer C2 with appId 'b'
    BMQTST_ASSERT_EQ(C2, tester.getHandle("C2@b readCount=1"));

    // 6. Verify that attempt to bring up another consumer with appId 'b' fails
    BMQTST_ASSERT_EQ(k_nullMockHandle_p, tester.getHandle("C2@b readCount=1"));

    // 7. Bring up a producer C1 with writeCount == 2
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1 writeCount=2"));

    // 8. Verify that attempt to bring up another producer with fails
    BMQTST_ASSERT_EQ(k_nullMockHandle_p, tester.getHandle("C1 writeCount=1"));
}

static void test40_roundRobinAndRedelivery()
// ------------------------------------------------------------------------
// ROUND-ROBIN AND REDELIVERY
//
// Concerns:
//   1. Verifying redelivery per fanout appId..
//
// Plan:
//  1. Bring up a consumer C1 with appId 'a', 'b', and 'c'.
//  2. Post 3 messages (2 + 1)
//  3. Verify that each consumer received messages according to
//      consumerPriorityCount and consumerPriority
//  4. Close higher priority handle and verify messages redelivered to
//      lower priority handle.
//
// Testing:
//   'getHandle' and 'configureHandle' for multiple distinct appIds.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("ROUND-ROBIN AND REDELIVERY");

    mqbconfm::Domain config = fanoutConfig("a,b,c");

    mqbblp::QueueEngineTester tester(config,
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1. Bring up a consumer C1 with appId 'a', 'b', and 'c'.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@b readCount=2"));
    BMQTST_ASSERT_EQ(C1, tester.getHandle("C1@c readCount=1"));

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@a consumerPriority=2 consumerPriorityCount=2"),
                     0);
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@b consumerPriority=2 consumerPriorityCount=1"),
                     0);
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@c consumerPriority=2 consumerPriorityCount=1"),
                     0);

    mqbmock::QueueHandle* C2 = tester.getHandle("C2@a readCount=1");
    BMQTST_ASSERT_NE(C2, k_nullMockHandle_p);
    BMQTST_ASSERT_EQ(C2, tester.getHandle("C2@b readCount=2"));
    BMQTST_ASSERT_EQ(C2, tester.getHandle("C2@c readCount=1"));

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@a consumerPriority=2 consumerPriorityCount=1"),
                     0);
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@b consumerPriority=2 consumerPriorityCount=2"),
                     0);
    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@c consumerPriority=1 consumerPriorityCount=1"),
                     0);

    // 2. Post 3 messages (2 + 1)
    tester.post("1,2,3");
    tester.afterNewMessage(3);

    // 3. Verify that each consumer received messages according to
    //      consumerPriorityCount and consumerPriority
    PVV(L_ << ": C1@a Messages: " << C1->_messages("a"));
    PVV(L_ << ": C1@b Messages: " << C1->_messages("b"));
    PVV(L_ << ": C1@c Messages: " << C1->_messages("c"));

    PVV(L_ << ": C2@a Messages: " << C2->_messages("a"));
    PVV(L_ << ": C2@b Messages: " << C2->_messages("b"));
    PVV(L_ << ": C2@c Messages: " << C2->_messages("c"));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    BMQTST_ASSERT_EQ(C1->_numMessages("b"), 1);
    BMQTST_ASSERT_EQ(C1->_numMessages("c"), 3);

    BMQTST_ASSERT_EQ(C2->_numMessages("a"), 1);
    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 2);
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 0);

    BMQTST_ASSERT_EQ(C1->_messages("c"), "1,2,3");

    // 4. Close higher priority handle and verify messages redelivered to
    //      lower priority handle

    tester.releaseHandle("C1@c readCount=1");
    BMQTST_ASSERT_EQ(C2->_numMessages("c"), 3);
    BMQTST_ASSERT_EQ(C2->_messages("c"), "1,2,3");
}

static void test41_redeliverAfterGc()
// ------------------------------------------------------------------------
// REDELIVERY AFTER GC
//
// Concerns:
//   'beforeMessageRemoved' should remove message from the redeliveryList
//   (as well as from the storage).  This case triggers
//   'beforeMessageRemoved' by calling GC.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1.
//      Disable delivery to C2. Post 4 messages, verify that C1 got 4
//      messages and C2 got none.
//   2) Bring C1 down.  Verify C2 still has no messages.  The queue now has
//      all 4 messages in its redeliveryList
//   3) GC one (first) message.  That should remove it from the storage as
//      well as from the redelivery list.
//   4) Now enable and trigger delivery on C2.  Verify that C2 got 3
//      messages.
// Testing:
//   Queue Engine redelivery of unconfirmed messages after removing some of
//   them.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("REDELIVERY AFTER GC");

    mqbconfm::Domain config = fanoutConfig("a");

    mqbblp::QueueEngineTester tester(config,
                                     false,  // start scheduler
                                     bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1. Bring up a consumer C1 with appId 'a'.
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    BMQTST_ASSERT_NE(C1, k_nullMockHandle_p);

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C1@a consumerPriority=2 consumerPriorityCount=1"),
                     0);

    mqbmock::QueueHandle* C2 = tester.getHandle("C2@a readCount=1");
    BMQTST_ASSERT_NE(C2, k_nullMockHandle_p);

    BMQTST_ASSERT_EQ(tester.configureHandle(
                         "C2@a consumerPriority=1 consumerPriorityCount=1"
                         " maxUnconfirmedMessages=0"),
                     0);

    tester.post("1,2,3,4");
    tester.afterNewMessage(4);

    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    PVV(L_ << ": C2 Messages: " << C2->_messages("a"));

    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 4);
    BMQTST_ASSERT_EQ(C2->_numMessages("a"), 0);

    BMQTST_ASSERT_EQ(C1->_messages("a"), "1,2,3,4");

    // 2)
    tester.dropHandle("C1");
    BMQTST_ASSERT_EQ(C2->_numMessages("a"), 0);

    // 3)
    tester.garbageCollectMessages(1);

    // 4)
    C2->_setCanDeliver("a", true);

    PVV(L_ << ": C2 Messages: " << C2->_messages("a"));

    BMQTST_ASSERT_EQ(C2->_numMessages("a"), 3);
    BMQTST_ASSERT_EQ(C2->_messages("a"), "2,3,4");
}

static void test42_throttleRedeliveryPriority()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY PRIORITY
//
// Concerns:
//   After a consumer disconnects ungracefully, the unconfirmed messages
//   should be redelivered to the remaining consumer in a throttled manner.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1.
//      Post 4 messages, verify that C1 got 4 messages and C2 got none.
//   2) Bring C1 down.  Verify C2 receives only the first message in the
//      redelivery list.
//   3) Advance the system clock by the value of the throttling delay and
//      verify C2 only received one additonal message.
//   4) Verify only one message comes in each time the system clock is
//      advanced by the throttling delay.
// Testing:
//   Queue Engine throttled redelivery of unconfirmed messages after the
//   rda reaches 2.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY PRIORITY");

    mqbconfm::Domain config      = priorityDomainConfig();
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(
        config,
        bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // Set up for the expected message delay
    bmqp::RdaInfo rdaInfo = bmqp::RdaInfo()
                                .setCounter(config.maxDeliveryAttempts() - 1)
                                .setPotentiallyPoisonous(true);
    bsls::TimeInterval expectedDelay;
    // The message throttle config used from the MockQueue is default
    // constructed so it will have the same threshold and interval values as
    // this one.
    mqbcfg::MessageThrottleConfig messageThrottleConfig;
    mqbblp::QueueEngineUtil::loadMessageDelay(rdaInfo,
                                              messageThrottleConfig,
                                              &expectedDelay);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2,3,4");
    tester.afterNewMessage(4);

    PVV(L_ << ": C1 Messages: " << C1->_messages());
    BMQTST_ASSERT_EQ(C1->_numMessages(), 4);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 0);

    // 2)
    tester.dropHandle("C1");

    // Only one message should be redelivered here since the other messages are
    // being throttled.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);

    // 3)
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);

    // 4)
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 3);

    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 4);
}

static void test43_throttleRedeliveryFanout()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY FANOUT
//
// Concerns:
//   After a consumer disconnects ungracefully, the unconfirmed messages
//   should be redelivered to the remaining consumer in a throttled manner
//   for a particular app id. The other app ids shouldn't be affected.
//
// Plan:
//   1) Configure 3 handles, C1, C2, C3  each with distinct app ids.
//      Configure handle C4 with the same app id as C3 but with a lower
//      priority.
//      Post 4 messages, verify that C1, C2, C3 got 4 messages and C4 got
//      none.
//   2) Bring C3 down.  Verify C4 receives only the first message in the
//      redelivery list.  Verify C1 and C2 are unaffected.
//   3) Confirm the first two messages for C1 and C2.  Verify that C4 is
//      unaffected and no more messages come in for C1 and C2.
//   4) Advance the system clock by the value of the throttling delay and
//      verify C4 only received one additonal message.
//   5) Verify only one message comes in for C4 each time the system clock
//      is advanced by the throttling delay.  Verify C1 and C2 remain
//      unaffected.
// Testing:
//   Queue Engine throttled redelivery of unconfirmed messages after the
//   rda reaches 2.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY FANOUT");

    mqbconfm::Domain config      = fanoutConfig("a,b,c");
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(
        config,
        bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // Set up for the expected message delay
    bmqp::RdaInfo rdaInfo = bmqp::RdaInfo()
                                .setCounter(config.maxDeliveryAttempts() - 1)
                                .setPotentiallyPoisonous(true);
    bsls::TimeInterval expectedDelay;
    // The message throttle config used from the MockQueue is default
    // constructed so it will have the same threshold and interval values as
    // this one.
    mqbcfg::MessageThrottleConfig messageThrottleConfig;
    mqbblp::QueueEngineUtil::loadMessageDelay(rdaInfo,
                                              messageThrottleConfig,
                                              &expectedDelay);

    // 1)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1@a readCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2@b readCount=1");
    mqbmock::QueueHandle* C3 = tester.getHandle("C3@c readCount=1");
    mqbmock::QueueHandle* C4 = tester.getHandle("C4@c readCount=1");

    tester.configureHandle("C1@a maxUnconfirmedMessages=4"
                           " consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C2@b maxUnconfirmedMessages=4"
                           " consumerPriority=1 consumerPriorityCount=1");
    tester.configureHandle("C3@c maxUnconfirmedMessages=4"
                           " consumerPriority=2 consumerPriorityCount=1");
    tester.configureHandle("C4@c maxUnconfirmedMessages=4"
                           " consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2,3,4");
    tester.afterNewMessage(4);

    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 4);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 4);
    PVV(L_ << ": C3 Messages: " << C3->_messages("c"));
    BMQTST_ASSERT_EQ(C3->_numMessages("c"), 4);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    BMQTST_ASSERT_EQ(C4->_numMessages("c"), 0);

    // 2)
    tester.dropHandle("C3");

    // Only one message should be redelivered to C4 since the other messages
    // are being throttled.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 4);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 4);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    BMQTST_ASSERT_EQ(C4->_numMessages("c"), 1);

    // 3)
    tester.confirm("C1@a", "1,2");
    tester.confirm("C2@b", "1,2");
    // The messages being confirmed for C1 and C2 shouldn't have any effect on
    // C4. No messages should be delivered to C1 and C2.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 2);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    BMQTST_ASSERT_EQ(C4->_numMessages("c"), 1);

    // 4)
    tester.advanceTime(expectedDelay);
    // C4 should receive the next throttled message. C1 and C2 should be
    // unaffected.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 2);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    BMQTST_ASSERT_EQ(C4->_numMessages("c"), 2);

    // 5)
    tester.advanceTime(expectedDelay);
    // C4 should receive the next throttled message. C1 and C2 should be
    // unaffected.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 2);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    BMQTST_ASSERT_EQ(C4->_numMessages("c"), 3);

    tester.advanceTime(expectedDelay);
    // C4 should receive the next throttled message. C1 and C2 should be
    // unaffected.
    PVV(L_ << ": C1 Messages: " << C1->_messages("a"));
    BMQTST_ASSERT_EQ(C1->_numMessages("a"), 2);
    PVV(L_ << ": C2 Messages: " << C2->_messages("b"));
    BMQTST_ASSERT_EQ(C2->_numMessages("b"), 2);
    PVV(L_ << ": C4 Messages: " << C4->_messages("c"));
    BMQTST_ASSERT_EQ(C4->_numMessages("c"), 4);
}

static void test44_throttleRedeliveryCancelledDelay()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY CANCELLED DELAY
//
// Concerns:
//   If the message preceding a delayed message is confirmed, the delay on
//   the current message should be cancelled and sent right away.  A
//   confirmed message which was sent before the delayed and preceding
//   shouldn't have an effect on the current delay.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1.
//      Post 4 messages.
//   2) Bring C1 down.  Verify C2 receives only the first message in the
//      redelivery list.
//   3) Advance the system clock by the value of the throttling delay and
//      verify C2 only received one additonal message.
//   4) Confirm only the 1st message sent and verify the 3rd message still
//      being throttled and hasn't been delivered
//   5) Advance the system clock by the value of the throttling delay and
//      verify C2 received only the 3rd message.
//   6) Confirm the 2nd and 3rd message and verify the throttle on the 4th
//      message is cancelled and has come in.
// Testing:
//   mqbblp::QueueEngine cancelThrottle on a delayed message.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY CANCELLED DELAY");

    mqbconfm::Domain config      = priorityDomainConfig();
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(
        config,
        bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // Set up for the expected message delay
    bmqp::RdaInfo rdaInfo = bmqp::RdaInfo()
                                .setCounter(config.maxDeliveryAttempts() - 1)
                                .setPotentiallyPoisonous(true);
    bsls::TimeInterval expectedDelay;
    // The message throttle config used from the MockQueue is default
    // constructed so it will have the same threshold and interval values as
    // this one.
    mqbcfg::MessageThrottleConfig messageThrottleConfig;
    mqbblp::QueueEngineUtil::loadMessageDelay(rdaInfo,
                                              messageThrottleConfig,
                                              &expectedDelay);

    // 1)
    tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2,3,4");
    tester.afterNewMessage(4);

    // 2)
    tester.dropHandle("C1");

    // Only one message should be redelivered here since the other messages are
    // being throttled.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);

    // Timer is set for 'expectedDelay'.  Pending "2,3,4".

    // 3)
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);

    // Timer is set for 'expectedDelay'.  Pending "3,4".

    // 4)
    // Confirming the first message should still keep the delay between the
    // second and third message intact (ie. no new messages should come in).
    tester.confirm("C2", "1");
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);

    // Timer is set for 'expectedDelay'.  Pending "3,4".

    // 5)
    // Another message should come in after the delay
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 2);

    // Timer is set for 'expectedDelay'.  Pending "4".

    // 6)
    // Since the message before the currently delayed message was confirmed,
    // the delay of the current message should be cut short.
    tester.confirm("C2", "2");

    // Confirming "2" does not cancel throttling since it is not the last one.
    // Confirming "3" does cancel throttling since it is the last one pending.
    // Canceling throttling is done by rescheduling at 'bsls::TimeInterval()'
    // _before_ removing the message so there is thread contention in the
    // mock dispatcher environment.  'synchronizeScheduler()' is too late.

    {
        bslmt::LockGuard<bslmt::Mutex> lock(&tester.dispatcher()->mutex());
        tester.QueueEngineTester::confirm("C2", "3");
    }
    tester.synchronizeScheduler();

    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);
}

static void test45_throttleRedeliveryNewHandle()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY NEW HANDLE
//
// Concerns:
//   If a message is currently being delayed and a new handle shows up, the
//   delay should be cancelled and the message sent right away.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1. Post 2
//      messages.
//   2) Bring C1 down.  Verify C2 receives only the first message in the
//      redelivery list (the second messages should be throttled).
//   3) Bring up a new handle (C1) and configure it. Verify the second
//      message is delivered to C1 without delay.
// Testing:
//   mqbblp::QueueEngine a newly configured handle should end the delay for
//   for the current message.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY NEW HANDLE");

    mqbconfm::Domain config      = priorityDomainConfig();
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(
        config,
        bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // 1)
    tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2");
    tester.afterNewMessage(2);

    // 2)
    tester.dropHandle("C1");

    // Only one message should be redelivered here since the other message is
    // being throttled.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);

    // 3)
    mqbmock::QueueHandle* C1 = tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    BMQTST_ASSERT_EQ(C1->_numMessages(), 1);
}

static void test46_throttleRedeliveryNoMoreHandles()
// ------------------------------------------------------------------------
// THROTTLED REDELIVERY NO MORE HANDLES
//
// Concerns:
//   If a message is currently being delayed and the last handle
//   disappears, the delay on the message should be cancelled.
//
// Plan:
//   1) Configure 2 handles, C1 and C2, with the priorities 2 and 1. Post 2
//      messages.
//   2) Bring C1 down.  Verify C2 receives only the first message in the
//      redelivery list (the second messages should be throttled).
//   3) Advance the time by expectedDelay-1 then bring C2 down. Bring up C3
//      then advance the time by 1.  Verify C3 has only received the first
//      message (if the throttle wasn't cancelled properly when C2 dropped,
//      the call to deliverMessages() would happen at this point and the
//      second message would be delivered).
//   4) Advance the time by the throttling delay and verify the second C3
//      received the second message.
// Testing:
//   mqbblp::QueueEngine the last handle disappearing for a particular app
//   should end the delay for the current message.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'mqbblp::QueueEngine' and mocks from
    // 'mqbi' methods print with ball, which allocates.

    bmqtst::TestHelper::printTestName("THROTTLED REDELIVERY NO MORE HANDLES");

    mqbconfm::Domain config      = priorityDomainConfig();
    config.maxDeliveryAttempts() = 5;

    mqbblp::TimeControlledQueueEngineTester tester(
        config,
        bmqtst::TestHelperUtil::allocator());

    mqbblp::QueueEngineTesterGuard<mqbblp::RootQueueEngine> guard(&tester);

    // Set up for the expected message delay
    bmqp::RdaInfo rdaInfo = bmqp::RdaInfo()
                                .setCounter(config.maxDeliveryAttempts() - 1)
                                .setPotentiallyPoisonous(true);
    bsls::TimeInterval expectedDelay;
    // The message throttle config used from the MockQueue is default
    // constructed so it will have the same threshold and interval values as
    // this one.
    mqbcfg::MessageThrottleConfig messageThrottleConfig;
    mqbblp::QueueEngineUtil::loadMessageDelay(rdaInfo,
                                              messageThrottleConfig,
                                              &expectedDelay);

    // 1)
    tester.getHandle("C1 readCount=1");
    tester.configureHandle("C1 consumerPriority=2 consumerPriorityCount=1");
    mqbmock::QueueHandle* C2 = tester.getHandle("C2 readCount=1");
    tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");

    tester.post("1,2");
    tester.afterNewMessage(2);

    // 2)
    tester.dropHandle("C1");

    // Only one message should be redelivered here since the other message is
    // being throttled.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);

    // 3)
    tester.advanceTime(expectedDelay -
                       bsls::TimeInterval().addMilliseconds(1));
    // We still should not have received the throttled message.
    PVV(L_ << ": C2 Messages: " << C2->_messages());
    BMQTST_ASSERT_EQ(C2->_numMessages(), 1);
    tester.dropHandle("C2");

    mqbmock::QueueHandle* C3 = tester.getHandle("C3 readCount=1");
    tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=1");
    // If the throttled message was cancelled correctly, C3 will not receive
    // the 2nd message. If the throttled message wasn't cancelled, C3 will
    // receive the message since we advanced the time expectedDelay-1 before
    // C2 dropped and we're advancing the time by 1 here.
    tester.advanceTime(bsls::TimeInterval().addMilliseconds(1));
    PVV(L_ << ": C3 Messages: " << C3->_messages());
    BMQTST_ASSERT_EQ(C3->_numMessages(), 1);

    // 4)
    tester.advanceTime(expectedDelay);
    PVV(L_ << ": C3 Messages: " << C3->_messages());
    BMQTST_ASSERT_EQ(C3->_numMessages(), 2);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());
    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    mqbcfg::AppConfig brokerConfig(bmqtst::TestHelperUtil::allocator());
    mqbcfg::BrokerConfig::set(brokerConfig);

    {
        bsl::shared_ptr<bmqst::StatContext> statContext =
            mqbstat::BrokerStatsUtil::initializeStatContext(
                30,
                bmqtst::TestHelperUtil::allocator());

        switch (_testCase) {
        case 0:
        case 46: test46_throttleRedeliveryNoMoreHandles(); break;
        case 45: test45_throttleRedeliveryNewHandle(); break;
        case 44: test44_throttleRedeliveryCancelledDelay(); break;
        case 43: test43_throttleRedeliveryFanout(); break;
        case 42: test42_throttleRedeliveryPriority(); break;
        case 41: test41_redeliverAfterGc(); break;
        case 40: test40_roundRobinAndRedelivery(); break;
        case 39: test39_maxConsumersProducers(); break;
        case 38: test38_unauthorizedAppIds(); break;
        case 37: test37_afterQueuePurged_specificSubStreamResets(); break;
        case 36: test36_afterQueuePurged_queueStreamResets(); break;
        case 35: test35_beforeMessageRemoved_withActiveConsumers(); break;
        case 34: test34_beforeMessageRemoved_deadConsumers(); break;
        case 33: test33_consumerUpAfterMessagePosted(); break;
        case 32: test32_afterNewMessageRespectsFlowControl(); break;
        case 31: test31_afterNewMessageDeliverToAllActiveConsumers(); break;
        case 30: test30_releaseHandle_isDeletedFlag(); break;
        case 29: test29_releaseHandleMultipleProducers(); break;
        case 28: test28_releaseHandle(); break;
        case 27: test27_configureHandleMultipleAppIds(); break;
        case 26: test26_getHandleUnauthorizedAppId(); break;
        case 25: test25_getHandleSameHandleMultipleAppIds(); break;
        case 24: test24_getHandleDuplicateAppId(); break;
        case 23: test23_loadRoutingConfiguration(); break;
        case 22: test22_createAndConfigure(); break;
        case 21: test21_breathingTest(); break;
        case 20: test20_priorityRedeliverAfterGc(); break;
        case 19: test19_priorityReleaseHandle_isDeletedFlag(); break;
        case 18: test18_priorityAfterQueuePurged_queueStreamResets(); break;
        case 17:
            test17_priorityBeforeMessageRemoved_garbageCollection();
            break;
        case 16:
            test16_priorityReleaseDormantConsumerWithoutNullReconfigure();
            break;
        case 15:
            test15_priorityReleaseActiveConsumerWithoutNullReconfigure();
            break;
        case 14: test14_priorityRedeliverToOtherConsumers(); break;
        case 13: test13_priorityRedeliverToFirstConsumerUp(); break;
        case 12: test12_priorityCannotDeliver(); break;
        case 11: test11_priorityReconfigure(); break;
        case 10: test10_priorityAggregateDownstream(); break;
        case 9: test9_priorityCreateAndConfigure(); break;
        case 8: test8_priorityBreathingTest(); break;
        case 7: test7_broadcastPriorityFailover(); break;
        case 6: test6_broadcastDynamicPriorities(); break;
        case 5: test5_broadcastReleaseHandle_isDeletedFlag(); break;
        case 4: test4_broadcastPostAfterResubscribe(); break;
        case 3: test3_broadcastCannotDeliver(); break;
        case 2: test2_broadcastConfirmAssertFails(); break;
        case 1: test1_broadcastBreathingTest(); break;
        case -1: testN1_broadcastExhaustiveSubscriptions(); break;
        case -2: testN2_broadcastExhaustiveCanDeliver(); break;
        case -3: testN3_broadcastExhaustiveConsumerPriority(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            bmqtst::TestHelperUtil::testStatus() = -1;
        } break;
        }
    }

    bmqp::ProtocolUtil::shutdown();
    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
