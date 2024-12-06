// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbblp_queueenginetester.h                                         -*-C++-*-
#ifndef INCLUDED_MQBBLP_QUEUEENGINETESTER
#define INCLUDED_MQBBLP_QUEUEENGINETESTER

//@PURPOSE: Provide a mechanism to simplify writing Queue Engine test cases.
//
//@CLASSES:
//  mqbblp::QueueEngineTester:      a tester object for Queue Engine
//  mqbblp::QueueEngineTesterGuard: proctor object for QueueEngineTester
//  mqbblp::QueueEngineTestUtil:    helper for testing Queue Engine
//
//@DESCRIPTION: This component provides a mechanism,
//  'mqbblp::QueueEngineTester', for testing an implementation of the
//  'mqbi::QueueEngine' protocol.  It is a wrapper around a 'mqbi::QueueEngine'
//  object and any supporting objects.  It additionally provides a proctor
//  mechanism for acquisition and release of a 'mqbblp::QueueEngineTester'
//  object, handling initialization, creation of the associated Queue Engine,
//  release, and necessary cleanup (e.g., invoking 'dropHandles()' upon
//  destruction) for the QueueEngineTester under management.  Finally, it
//  provides utilities, 'mqbblp::QueueEngineTestUtil', to simplify Queue Engine
//  test cases.
//
/// Thread Safety
///-------------
// NOT Thread-Safe.
//
/// Usage
//------
// This section illustrates intended use of this component.
//
/// Example 1: Testing PriorityQueueEngine with 3 consumers
///- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The following code illustrates how to leverage the functionality of this
// component to perform a basic test of 'mqbblp::PriorityQueueEngine'
// whereby three distinct consumers having the same priority are brought up
// and three messages are distributed across the consumers.
//
// First, we create a 'QueueEngineTester' object and the Priority Queue Engine.
//..
//  mqbblp::QueueEngineTester tester(bmqtst::TestHelperUtil::allocator());
//  tester.createQueueEngine<mqbblp::PriorityQueueEngine>();
//..
// Then, we get handles for three consumers, each with one reader.
//..
//  mqbi::MockQueueHandle *handle1 = tester.getHandle("C1 readCount=1");
//  mqbi::MockQueueHandle *handle2 = tester.getHandle("C2 readCount=1");
//  mqbi::MockQueueHandle *handle3 = tester.getHandle("C3 readCount=1");
//..
// Next, we configure each handle with priority of 1 and a count of 1.
//..
//  tester.configureHandle("C1 consumerPriority=1 consumerPriorityCount=1");
//  tester.configureHandle("C2 consumerPriority=1 consumerPriorityCount=1");
//  tester.configureHandle("C3 consumerPriority=1 consumerPriorityCount=1");
//..
// Then, we post three messages and inform the tester to distribute them.
//..
//  tester.post("a,b,c");
//  tester.afterNewMessage(0);
//..
// Next, we verify the consumers received the corresponding messages.
//..
//  BMQTST_ASSERT_EQ(handle1->_messages(), "a");
//  BMQTST_ASSERT_EQ(handle2->_messages(), "b");
//  BMQTST_ASSERT_EQ(handle3->_messages(), "c");
//..
// Then, for each consumer we confirm the corresponding messages.
//..
//  tester.confirm("C1", "a");
//  tester.confirm("C2", "b");
//  tester.confirm("C3", "c");
//
//  BMQTST_ASSERT_EQ(handle1->_messages(), "");
//  BMQTST_ASSERT_EQ(handle2->_messages(), "");
//  BMQTST_ASSERT_EQ(handle3->_messages(), "");
//..
// Finally, we release all the handles.
//..
//  tester.dropHandles();
//..

// MQB
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_queuestate.h>
#include <mqbblp_relayqueueengine.h>
#include <mqbi_queueengine.h>
#include <mqbmock_cluster.h>
#include <mqbmock_dispatcher.h>
#include <mqbmock_domain.h>
#include <mqbmock_queue.h>
#include <mqbmock_queuehandle.h>
#include <mqbs_virtualstoragecatalog.h>
#include <mqbu_storagekey.h>

#include <bmqc_orderedhashmap.h>

// BMQ
#include <bmqt_messageguid.h>

// BDE
#include <ball_log.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

namespace mqbblp {

// =======================
// class QueueEngineTester
// =======================

/// Wrapper around a `QueueEngine` object and any supporting objects to
/// simplify the writing of test cases.
///
/// Clients are identified by a unique string literal referred to as
/// <clientKey>, which serves as a key in a map of clients.  Messages are
/// identified by their payload, which also serves as a key in a map of
/// payloads (message GUIDs are abstracted away from users of this class).
/// Hence, message payloads are unique across the lifetime of an object of
/// this class.
class QueueEngineTester {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.QUEUEENGINETESTER");

  private:
    // PRIVATE TYPES

    /// clientKey -> MockQueueHandle*
    typedef bsl::unordered_map<bsl::string, mqbmock::QueueHandle*> HandleMap;

    /// appId -> subId
    typedef bsl::unordered_map<bsl::string, unsigned int> SubIdsMap;

    typedef bsl::shared_ptr<mqbi::QueueHandleRequesterContext> ClientContextSp;

    /// clientKey -> mqbi::QueueHandleRequesterContext
    typedef bsl::unordered_map<bsl::string, ClientContextSp> ClientContextMap;

    /// payload (as string) -> msgGUID
    /// Must be a container in which iteration order is same as insertion
    /// order because in `afterNewMessage`, we need to invoke the engine for
    /// newly posted messages in the order that they were posted.
    typedef bmqc::OrderedHashMap<bsl::string, bmqt::MessageGUID> MessagesMap;

    typedef bsl::shared_ptr<mqbi::QueueHandle> QueueHandleSp;

  protected:
    // DATA
    const bmqt::MessageGUID d_invalidGuid;
    // Constant representing a null Message
    // GUID. This value should be used for
    // the guid of a message whose GUID is
    // not important or unknown.

    bdlbb::PooledBlobBufferFactory d_bufferFactory;
    // Buffer factory to use for messages

    bslma::ManagedPtr<mqbmock::Dispatcher> d_mockDispatcher_mp;
    // Mock dispatcher

    bslma::ManagedPtr<mqbmock::DispatcherClient> d_mockDispatcherClient_mp;
    // Mock dispatcher client

    bslma::ManagedPtr<mqbmock::Cluster> d_mockCluster_mp;
    // Mock cluster

    bslma::ManagedPtr<mqbmock::Domain> d_mockDomain_mp;
    // Mock domain

    bsl::shared_ptr<mqbmock::Queue> d_mockQueue_sp;
    // Mock queue

    bslma::ManagedPtr<mqbblp::QueueState> d_queueState_mp;
    // Queue state

    bslma::ManagedPtr<mqbi::QueueEngine> d_queueEngine_mp;
    // Queue Engine being tested

    HandleMap d_handles;
    // Map of all created handles.  Note
    // that a handle in this map is owned
    // by the Queue Handle Catalog and
    // therefore must be 'released'
    // accordingly.

    SubIdsMap d_subIds;
    // Map of 'appId' to 'subId' of all
    // streams of a queue.

    ClientContextMap d_clientContexts;
    // Map of client context for all
    // created handles

    MessagesMap d_postedMessages;
    // Collection of message GUIDs that
    // were posted to the queue.

    MessagesMap d_newMessages;
    // Collection of Message GUIDs that
    // have been newly posted to the queue
    // and for which 'afterNewMessage' of
    // the queue engine under test has not
    // yet been invoked.

    unsigned int d_clientCounter;
    // Counter for distinct clients

    unsigned int d_subQueueIdCounter;
    // Counter for distinct subQueueIds

    bsl::vector<QueueHandleSp> d_deletedHandles;
    // List of handles that were fully
    // dropped (i.e. fully released) but
    // needed to stay alive to correctly
    // test post-drop state

    size_t d_messageCount;

    bslma::Allocator* d_allocator_p;
    // Allocator to use

  private:
    // NOT IMPLEMENTED
    QueueEngineTester(const QueueEngineTester&) BSLS_CPP11_DELETED;
    QueueEngineTester& operator=(const QueueEngineTester&) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Perform any initialization that needs to be done one time only in
    /// the course of a program's execution (to enable thread-safety of some
    /// component, etc.).
    void oneTimeInit();

    /// Reset and recreate all objects using the currently set options and
    /// the specific `domainConfig`.
    void init(const mqbconfm::Domain& domainConfig, bool startScheduler);

    /// Pendant operation of the `oneTimeInit` one.
    void oneTimeShutdown();

    void
    handleReleasedCallback(int*  rc,
                           bool* isDeletedOutput,
                           const bsl::shared_ptr<mqbi::QueueHandle>& handle,
                           const mqbi::QueueHandleReleaseResult&     result,
                           const bsl::string& clientKey);

    /// Callback method invoked when finished releasing the specified
    /// `handle`, with the specified `isDeleted` flag indicating if the
    /// handle was deleted.  Populate the specified `rc` with the result
    /// status code of the releaseHandle operation (zero on success,
    /// non-zero otherwise) and the optionally specified `isDeletedOutput`,
    /// if provided, with the value of the `isDeleted` flag propagated from
    /// the queue engine.  The specified `clientKey` is used when
    /// `isDeleted` is true to erase the handle from the map of active
    /// handles.
    void dummyHandleReleasedCallback(
        int*                                      rc,
        const bsl::shared_ptr<mqbi::QueueHandle>& handle,
        const mqbi::QueueHandleReleaseResult&     result,
        const bsl::string&                        clientKey);

    mqbi::QueueEngine* createQueueEngineHelper(mqbi::QueueEngine* engine);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueEngineTester,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `QueueEngineTester` object, passing the specified
    /// `domainConfig` to the internally created domain.  Use the specified
    /// `startScheduler` to determine whether the scheduler is started on
    /// creation.  Use the specified `allocator` for any memory allocation.
    explicit QueueEngineTester(const mqbconfm::Domain& domainConfig,
                               bool                    startScheduler,
                               bslma::Allocator*       allocator);

    /// Destructor
    ~QueueEngineTester();

    // MANIPULATORS

    /// Create and set the Queue Engine of this object to the concrete
    /// implementation of the `mqbi::QueueEngine` protocol specified by the
    /// parameterized type `T`.  Return a pointer to the Queue Engine.  The
    /// behavior is undefined if this method was previously called.  Note
    /// that `configure()` is called on the engine after it is created.
    /// Finally note that the behavior of all other methods in this class is
    /// undefined unless this method was called exactly once.
    template <typename T>
    T* createQueueEngine();

    /// Obtain and return a handle for the client with handle parameters in
    /// the specified `clientText` per the following format:
    ///   `<clientKey>[@<appId>] [readCount=<N>] [writeCount=<M>]`
    ///
    /// The behavior is undefined unless `clientText` is formatted as above
    /// and includes at least one attribute (e.g. `readCount=1`) other than
    /// an `appId`, or if `createQueueEngine()` was not called.  Note that
    /// if there is a failure in obtaining the handle from the underlying
    /// Queue Engine, this method returns a null pointer.
    mqbmock::QueueHandle* getHandle(const bsl::string& clientText);

    /// Configure the handle for the client with stream parameters in the
    /// specified `clientText` per the following format:
    ///   '<clientKey>[@<appId>] [consumerPriority=<P>]
    ///                          [consumerPriorityCount=<C>]
    ///                          [maxUnconfirmedMessages=<M>]
    ///                          [maxUnconfirmedBytes=<B>]'
    ///
    /// Return the result status code of the configureHandle operation (zero
    /// on success, non-zero otherwise).  The behavior is undefined unless
    /// `clientText` is formatted as above (e.g. 'C1 consumerPriority=1
    /// consumerPriorityCount=2') , and a previous call to `getHandle()`
    /// returned a handle for the `<clientKey>[@appId]`, or if
    /// `createQueueEngine()` was not called.
    int configureHandle(const bsl::string& clientText);

    /// Return the existing handle associated with the client identified
    /// with the specified `clientKey`.  The behavior is undefined unless a
    /// previous call to `getHandle()` returned a handle for the
    /// `clientKey`, or if `createQueueEngine()` was not called.
    mqbmock::QueueHandle* client(const bslstl::StringRef& clientKey);

    /// Post the specified `messages` to the storage of the queue associated
    /// with the queue engine under test.  The format of `messages` must be
    /// as follows:
    ///   `<msg1>,<msg2>,...,<msgN>`
    /// If the optionally specified 'downstream' is not zero, consider it as
    /// representing 'RelayQueueEngine' involved in the data delivery and
    /// invoke its 'push' method for each message.
    /// The order of posting each message is from left to right per the
    /// format above.  The behavior is undefined unless `messages` is
    /// formatted as above and each message is unique (across the lifetime
    /// of this object), or if `createQueueEngine()` was not called.
    void post(const bslstl::StringRef& messages,
              RelayQueueEngine*        downstream = 0);

    /// Invoke the Queue Engine's `afterNewMessage()` method for the
    /// specified `numMessages` newly posted messages if `numMessages > 0`,
    /// or all newly posted messages if `numMessages == 0`.  The behavior is
    /// undefined unless `numMessages >= 0` and 'numMessages <= No. of
    /// messages that were posted but for which afterNewMessage() was not
    /// called', or if `createQueueEngine()` was not called.
    void afterNewMessage(const int numMessages);

    /// Invoke the `confirmMessage()` method on the handle associated with
    /// the client identified with the specified `clientText` for the
    /// specified `messages`.  The format of `clientText` must be as
    /// follows:
    ///   `<clientKey>[@<appId>]`
    ///
    /// The format of `messages` must be as follows:
    ///   `<msg1>,<msg2>,...,<msgN>`
    ///
    /// The order of confirming each message is from left to right per the
    /// format above.  The behavior is undefined unless `messages` is
    /// formatted as above, and a previous call to `getHandle()` returned a
    /// handle for the `<clientKey>[@appId]`, or if `createQueueEngine()`
    /// was not called.  Note that it is "legal" for a client that is a
    /// reader to confirm a message that was not posted or that is already
    /// confirmed.
    void confirm(const bsl::string&       clientText,
                 const bslstl::StringRef& messages);

    /// Invoke the `rejectMessage()` method on the handle associated with
    /// the client identified with the specified `clientText` for the
    /// specified `messages`.  The format of `clientText` must be as
    /// follows:
    ///   `<clientKey>[@<appId>]`
    ///
    /// The format of `messages` must be as follows:
    ///   `<msg1>,<msg2>,...,<msgN>`
    ///
    /// The order of rejecting each message is from left to right per the
    /// format above.  The behavior is undefined unless `messages` is
    /// formatted as above, and a previous call to `getHandle()` returned a
    /// handle for the `<clientKey>[@appId]`, or if `createQueueEngine()`
    /// was not called.
    void reject(const bsl::string&       clientText,
                const bslstl::StringRef& messages);

    /// Invoke the Queue Engine's `beforeMessageRemoved()` method for the
    /// specified `numMessages` newly posted (yet to be delivered) messages
    /// and remove the messages from the storage of the queue associated
    /// with the queue engine under test.  If `numMessages == 0`, apply
    /// these operations to all newly posted (yet to be delivered) messages.
    /// The behavior is undefined unless `numMessages >= 0` and 'numMessages
    /// <= No. of messages that were posted but for which afterNewMessage()
    /// was not called', or if `createQueueEngine()` was not called.
    void garbageCollectMessages(const int numMessages);

    /// Remove all outstanding messages from the subStream identified by the
    /// optionally specified `appId` in the queue associated with the Queue
    /// Engine under test and subsequently invoke the Queue Engine's
    /// `afterQueuePurged(<appKey>)` method (so that it may do post-purge
    /// adjustments).  If `appId` is not specified, then all subStreams are
    /// assumed.  The behavior is undefined unless `createQueueEngine()` was
    /// called.
    void purgeQueue(const bslstl::StringRef& appId = "");

    /// Release the parameters in the handle of the client in the specified
    /// `clientText` per the following format:
    ///   `<clientKey>[@appId] [readCount=<N>] [writeCount=<M>]`
    ///
    /// Return the result status code of the releaseHandle operation (zero
    /// on success, non-zero otherwise). The behavior is undefined unless
    /// `clientText` is formatted as above and includes at least one option
    /// (e.g. `readCount=1`), and a previous call to `getHandle()` returned
    /// a handle for the `clientKey`, or if `createQueueEngine()` was not
    /// called.
    int releaseHandle(const bsl::string& clientText);

    /// Release the parameters in the handle of the client in the specified
    /// `clientText` per the following format:
    ///   '<clientKey>[@appId] [readCount=<N>] [writeCount=<M>]
    ///                        [isFinal=(true|false)]'
    ///
    /// Return the result status code of the releaseHandle operation (zero
    /// on success, non-zero otherwise) and populate the specified
    /// `isDeleted` flag with the `isDeleted` flag propagated from the queue
    /// engine under test (via callback invocation at completion of
    /// releasing a handle).  The behavior is undefined unless `clientText`
    /// is formatted as above and includes at least one option other than
    /// `isFinal` (e.g. `readCount=1`), and a previous call to `getHandle()`
    /// returned a handle for the `clientKey`, or if `createQueueEngine()`
    /// was not called.
    int releaseHandle(const bsl::string& clientText, bool* isDeleted);

    /// Drop the handle of the client identified with the specified
    /// `clientKey`.  The behavior is undefined unless a previous call to
    /// `getHandle()` returned a handle for the `clientKey`, or if
    /// `createQueueEngine()` was not called.
    void dropHandle(const bslstl::StringRef& clientKey);

    /// Drop all handles obtained through previous calls to `getHandle()`.
    /// The behavior is undefined if `createQueueEngine()` was not called.
    /// Note that pointers to these handles may be left dangling.
    void dropHandles();

    /// Load into the specified `value` previously cached parameters sent
    /// upstream for the specified `appId`.
    bool getUpstreamParameters(bmqp_ctrlmsg::StreamParameters* value,
                               const bslstl::StringRef&        appId) const;

    void synchronizeScheduler();
};

// ============================
// class QueueEngineTesterGuard
// ============================

/// This class implements a proctor mechanism for acquisition and release of
/// a `mqbblp::QueueEngineTester` object.  It handles initialization,
/// creation of the associated Queue Engine of type `QUEUE_ENGINE_TYPE`,
/// release, and necessary cleanup (e.g. invoking `dropHandles()` upon
/// destruction) for the QueueEngineTester under management.
template <class QUEUE_ENGINE_TYPE>
class QueueEngineTesterGuard {
  private:
    // DATA
    QueueEngineTester* d_tester_p;
    QUEUE_ENGINE_TYPE* d_engine_p;  // an alias

  private:
    // NOT IMPLEMENTED
    QueueEngineTesterGuard(const QueueEngineTesterGuard&) BSLS_CPP11_DELETED;
    QueueEngineTesterGuard<QUEUE_ENGINE_TYPE>& operator=(
        const QueueEngineTesterGuard<QUEUE_ENGINE_TYPE>&) BSLS_CPP11_DELETED;

  public:
    // CREATORS

    /// Create a proctor object that manages the specified
    /// `queueEngineTester` (if non-zero) and invokes the
    /// `createQueueEngine` method on `queueEngineTester` with the
    /// parameterized `QUEUE_ENGINE_TYPE`.
    explicit QueueEngineTesterGuard(QueueEngineTester* queueEngineTester);

    /// Destroy this proctor object and invoke the `dropHandles()` on the
    /// QueueEngineTester under management, if any.
    ~QueueEngineTesterGuard();

  public:
    // MANIPULATORS

    /// Return the address of the modifiable QueueEngine object under
    /// management by this proctor, and release the tester from further
    /// management by this proctor.  If no tester is currently being
    /// managed, return 0 with no other effect.  Note that this operation
    /// does *not* perform cleanup (e.g. `dropHandles()`) on the tester
    /// object (if any) that was under management.
    QueueEngineTester* release();

    // ACCESSORS

    /// Return the address of the modifiable QueueEngine object under
    /// management by this proctor, or 0 if no QueueEngine is currently being
    /// managed.
    QUEUE_ENGINE_TYPE* engine() const;
};

// ==========================
// struct QueueEngineTestUtil
// ==========================

/// Utilities to simplify writing Queue Engine test cases.
struct QueueEngineTestUtil {
  public:
    // CLASS METHODS

    /// Return a string containing those messages that are at the specified
    /// `indices` in the specified `messages`.  The format of `messages`:
    ///   `<msg1>,<msg2>,...,<msgN>`
    ///
    /// The format of `indices`:
    ///   `<index1>,<index2>,...,<indexK>`
    ///
    /// The behavior is undefined unless `messages` and `indices` are
    /// formatted as above, and each index in `indices` is a valid index for
    /// `messages`.
    static bsl::string getMessages(bsl::string messages, bsl::string indices);
};

// ================
// struct TestClock
// ================

struct TestClock {
    // DATA
    bdlmt::EventSchedulerTestTimeSource& d_timeSource;

    // CREATORS
    TestClock(bdlmt::EventSchedulerTestTimeSource& timeSource)
    : d_timeSource(timeSource)
    {
        // NOTHING
    }

    bsls::TimeInterval realtimeClock() { return d_timeSource.now(); }

    bsls::TimeInterval monotonicClock() { return d_timeSource.now(); }

    bsls::Types::Int64 highResTimer()
    {
        return d_timeSource.now().totalNanoseconds();
    }
};

// =====================================
// class TimeControlledQueueEngineTester
// =====================================

class TimeControlledQueueEngineTester : public mqbblp::QueueEngineTester {
  private:
    TestClock d_testClock;

  public:
    // CREATORS
    TimeControlledQueueEngineTester(const mqbconfm::Domain& domainConfig,
                                    bslma::Allocator*       allocator)
    : mqbblp::QueueEngineTester(domainConfig, true, allocator)
    , d_testClock(d_mockCluster_mp->_timeSource())
    {
        bmqsys::Time::shutdown();
        bmqsys::Time::initialize(
            bdlf::BindUtil::bind(&TestClock::realtimeClock, &d_testClock),
            bdlf::BindUtil::bind(&TestClock::monotonicClock, &d_testClock),
            bdlf::BindUtil::bind(&TestClock::highResTimer, &d_testClock));
    }

    // MANIPULATORS
    void advanceTime(const bsls::TimeInterval& step);

    void confirm(const bsl::string&       clientText,
                 const bslstl::StringRef& messages);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// QueueEngineTester
// -----------------

// PRIVATE MANIPULATORS
inline mqbi::QueueEngine*
QueueEngineTester::createQueueEngineHelper(mqbi::QueueEngine* engine)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(engine);
    BSLS_ASSERT_OPT(d_mockQueue_sp);

    bmqu::MemOutStream errorDescription(d_allocator_p);
    int                rc = engine->configure(errorDescription, false);
    BSLS_ASSERT_OPT(rc == 0);

    // Set the engine on the Queue
    d_mockQueue_sp->_setQueueEngine(engine);

    return engine;
}

// MANIPULATORS
template <typename T>
inline T* QueueEngineTester::createQueueEngine()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_queueEngine_mp && "'createQueueEngine()' was called");
    T* result = new (*d_allocator_p)
        T(d_queueState_mp.get(),
          d_queueState_mp->queue()->domain()->config(),
          d_allocator_p);
    // Create and configure Queue Engine
    d_queueEngine_mp.load(result, d_allocator_p);

    createQueueEngineHelper(d_queueEngine_mp.get());

    return result;
}

inline void QueueEngineTester::synchronizeScheduler()
{
    d_mockCluster_mp->waitForScheduler();
}
// ----------------------
// QueueEngineTesterGuard
// ----------------------

// CREATORS
template <class QUEUE_ENGINE_TYPE>
inline QueueEngineTesterGuard<QUEUE_ENGINE_TYPE>::QueueEngineTesterGuard(
    QueueEngineTester* queueEngineTester)
: d_tester_p(queueEngineTester)
, d_engine_p(0)
{
    if (d_tester_p) {
        d_engine_p = d_tester_p->createQueueEngine<QUEUE_ENGINE_TYPE>();
    }
}

template <class QUEUE_ENGINE_TYPE>
inline QueueEngineTesterGuard<QUEUE_ENGINE_TYPE>::~QueueEngineTesterGuard()
{
    if (d_tester_p) {
        d_tester_p->dropHandles();
    }
    d_engine_p = 0;
}

// MANIPULATORS
template <class QUEUE_ENGINE_TYPE>
inline QueueEngineTester* QueueEngineTesterGuard<QUEUE_ENGINE_TYPE>::release()
{
    QueueEngineTester* tester = d_tester_p;
    d_tester_p                = 0;
    d_engine_p                = 0;
    return tester;
}

// ACCESSORS
template <class QUEUE_ENGINE_TYPE>
inline QUEUE_ENGINE_TYPE*
QueueEngineTesterGuard<QUEUE_ENGINE_TYPE>::engine() const
{
    return d_engine_p;
}

// -------------------------------
// TimeControlledQueueEngineTester
// -------------------------------

inline void
TimeControlledQueueEngineTester::advanceTime(const bsls::TimeInterval& step)
{
    d_mockCluster_mp->advanceTime(step);

    synchronizeScheduler();
}

inline void
TimeControlledQueueEngineTester::confirm(const bsl::string&       clientText,
                                         const bslstl::StringRef& messages)
{
    mqbblp::QueueEngineTester::confirm(clientText, messages);
    synchronizeScheduler();
}

}  // close namespace mqbblp
}  // close namespace BloombergLP

#endif
