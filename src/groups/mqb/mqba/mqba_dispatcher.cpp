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

// mqba_dispatcher.cpp                                                -*-C++-*-
#include <mqba_dispatcher.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqsys_threadutil.h>
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>

// MQB
#include <mqbstat_dispatcherstats.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_cstddef.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bsla_annotations.h>
#include <bslma_managedptr.h>
#include <bslmt_semaphore.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {
const double k_QUEUE_STUCK_INTERVAL = 3 * 60.0;
const int    k_POOL_GROW_BY         = 1024;
}  // close unnamed namespace

// -------------------------
// class Dispatcher_Executor
// -------------------------

// CREATORS
Dispatcher_Executor::Dispatcher_Executor(const Dispatcher* dispacher,
                                         const mqbi::DispatcherClient* client)
    BSLS_CPP11_NOEXCEPT : d_eventSource_sp(),
                          d_processorPool_p(0),
                          d_statContext_p(0),
                          d_processorHandle()
{
    // PRECONDITIONS
    BSLS_ASSERT(dispacher);
    BSLS_ASSERT(client);
    BSLS_ASSERT(client->dispatcher() == dispacher);
    BSLS_ASSERT(client->dispatcherClientData().clientType() !=
                mqbi::DispatcherClientType::e_UNDEFINED);
    BSLS_ASSERT(client->dispatcherClientData().processorHandle() !=
                mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE);

    d_eventSource_sp = client->getEventSource();

    Dispatcher::DispatcherContext* dispatcherContext_p =
        dispacher->d_contexts.at(client->dispatcherClientData().clientType())
            .get();
    // set processor
    d_processorPool_p = dispatcherContext_p->d_processorPool_mp.get();
    d_processorHandle = client->dispatcherClientData().processorHandle();

    d_statContext_p =
        dispatcherContext_p->d_statContexts.at(d_processorHandle).get();
}

// ACCESSORS
bool Dispatcher_Executor::operator==(const Dispatcher_Executor& rhs) const
    BSLS_CPP11_NOEXCEPT
{
    return d_processorPool_p == rhs.d_processorPool_p &&
           d_processorHandle == rhs.d_processorHandle &&
           d_statContext_p == rhs.d_statContext_p;
}

void Dispatcher_Executor::post(const bsl::function<void()>& f) const
{
    // PRECONDITIONS
    BSLS_ASSERT(f);
    BSLS_ASSERT(d_processorPool_p->isStarted());

    // create an event containing the function to be invoked on the processor
    bsl::shared_ptr<mqbi::DispatcherEvent> event =
        d_eventSource_sp->getEvent();

    (*event)
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
        .setEnqueueTime(bmqsys::Time::highResolutionTimer())
        .callback()
        .set(f);

    // submit the event
    int rc = d_processorPool_p->enqueueEvent(
        bslmf::MovableRefUtil::move(event),
        d_processorHandle);
    BSLS_ASSERT_OPT(rc == 0);

    // Update stats
    mqbstat::DispatcherStats::onEnqueue(d_statContext_p);

    // TODO: We should call 'releaseUnmanagedEvent' on the
    //      'bmqc::MultiQueueThreadPool' in case of exception to prevent the
    //      event from leaking. But somehow this method is declared but not
    //      implemented.
}

void Dispatcher_Executor::dispatch(const bsl::function<void()>& f) const
{
    // PRECONDITIONS
    BSLS_ASSERT(f);
    BSLS_ASSERT(d_processorPool_p->isStarted());

    if (d_processorPool_p->queueThreadId(d_processorHandle) ==
        bslmt::ThreadUtil::selfId()) {
        // This function is called from the processor's thread. Invoke the
        // submitted function object in-place.
        f();
    }
    else {
        // This function is called outside of the processor's thread. Fallback
        // to 'post'.
        post(f);
    }
}

// ---------------------------
// class DispatcherEventSource
// ---------------------------

Dispatcher_EventSource::Dispatcher_EventSource(bslma::Allocator* allocator)
: d_pool(bdlf::BindUtil::bindS(allocator,
                               &Dispatcher_EventSource::eventCreator,
                               bdlf::PlaceHolders::_1,   // arena
                               bdlf::PlaceHolders::_2),  // allocator
         k_POOL_GROW_BY,
         allocator)
{
    // NOTHING
}

Dispatcher_EventSource::~Dispatcher_EventSource()
{
    // Make sure all the events have returned to the pool.
    BSLS_ASSERT(d_pool.numObjects() == d_pool.numAvailableObjects());
}

// ------------------------------------
// struct Dispatcher::DispatcherContext
// ------------------------------------

Dispatcher::DispatcherContext::DispatcherContext(
    const mqbcfg::DispatcherProcessorConfig& config,
    bslma::Allocator*                        allocator)
: d_threadPool_mp()
, d_processorPool_mp()
, d_loadBalancer(config.numProcessors(), allocator)
, d_flushList(config.numProcessors(),
              DispatcherClientPtrVector(allocator),
              allocator)
, d_eventSources(config.numProcessors(), allocator)
, d_clientStatContext_mp()
, d_statContexts(config.numProcessors(), allocator)
{
    typedef bsl::vector<bsl::shared_ptr<mqbi::DispatcherEventSource> >
        EventSources;
    for (EventSources::iterator it = d_eventSources.begin();
         it != d_eventSources.end();
         ++it) {
        *it = bsl::allocate_shared<mqba::Dispatcher_EventSource>(allocator);
    }
}

// ----------------
// class Dispatcher
// ----------------

int Dispatcher::startContext(bsl::ostream&                    errorDescription,
                             mqbi::DispatcherClientType::Enum type,
                             const mqbcfg::DispatcherProcessorConfig& config)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                     = 0,
        rc_THREAD_POOL_START_FAILED    = -1,
        rc_PROCESSOR_POOL_START_FAILED = -2
    };

    int rc = rc_SUCCESS;

    DispatcherContextSp& context = d_contexts[type];

    context.reset(new (*d_allocator_p)
                      DispatcherContext(config, d_allocator_p),
                  d_allocator_p);

    // Create client stat context
    context->d_clientStatContext_mp =
        mqbstat::DispatcherStatsUtil::initializeClientStatContext(
            d_statContext_p,
            mqbi::DispatcherClientType::toAscii(type),
            d_allocator_p);
    // Create and start the threadPool
    context->d_threadPool_mp.load(
        new (*d_allocator_p)
            bdlmt::ThreadPool(bmqsys::ThreadUtil::defaultAttributes(),
                              config.numProcessors(),           // min threads
                              config.numProcessors(),           // max threads
                              bsl::numeric_limits<int>::max(),  // idle time
                              d_allocator_p),
        d_allocator_p);

    rc = context->d_threadPool_mp->start();
    if (rc != 0) {
        context->d_threadPool_mp.clear();
        errorDescription << "Failed to start thread pool for '" << type
                         << "' [rc: " << rc << "]";
        return rc_THREAD_POOL_START_FAILED;  // RETURN
    }

    // Create and start the processorPool
    ProcessorPool::Config processorPoolConfig(
        config.numProcessors(),
        context->d_threadPool_mp.get(),
        bdlf::BindUtil::bind(&Dispatcher::queueEventCb,
                             this,
                             type,
                             bdlf::PlaceHolders::_1,   // processorId
                             bdlf::PlaceHolders::_2),  // event*
        bdlf::BindUtil::bind(&Dispatcher::queueCreator,
                             this,
                             type,
                             config.processorConfig(),
                             bdlf::PlaceHolders::_1,   // processorId
                             bdlf::PlaceHolders::_2),  // allocator*
        d_allocator_p);

    processorPoolConfig.setName(mqbi::DispatcherClientType::toAscii(type))
        .setEventScheduler(d_scheduler_p)
        .setMonitorAlarm("ALARM [DISPATCHER_QUEUE_STUCK] ",
                         bsls::TimeInterval(k_QUEUE_STUCK_INTERVAL));
    // TBD: .statContext(...) / .createSubcontext(true)
    //      We should have subcontext per each type of event (PUSH, PUT,
    //      CALLBACK, ACK, ...)

    context->d_processorPool_mp.load(
        new (*d_allocator_p) ProcessorPool(processorPoolConfig, d_allocator_p),
        d_allocator_p);

    rc = context->d_processorPool_mp->start();

    if (rc != 0) {
        context->d_processorPool_mp.clear();
        context->d_threadPool_mp->stop();
        context->d_threadPool_mp.clear();
        errorDescription << "Failed to start processor pool for '" << type
                         << "' [rc: " << rc << "]";
        return rc_PROCESSOR_POOL_START_FAILED;  // RETURN
    }

    return rc_SUCCESS;
}

Dispatcher::ProcessorPool::Queue*
Dispatcher::queueCreator(mqbi::DispatcherClientType::Enum             type,
                         const mqbcfg::DispatcherProcessorParameters& config,
                         int               processorId,
                         bslma::Allocator* allocator)
{
    bmqu::MemOutStream os;
    os << "ProcessorQueue " << processorId << " for '" << type << "'";
    bsl::string queueName(os.str().data(), os.str().length());

    ProcessorPool::Queue* queue = new (*allocator)
        ProcessorPool::Queue(config.queueSizeLowWatermark(), allocator);

    queue->setWatermarks(config.queueSizeLowWatermark(),
                         config.queueSizeHighWatermark());
    queue->setStateCallback(
        bdlf::BindUtil::bind(&bmqc::MonitoredQueueUtil::stateLogCallback,
                             queueName,
                             "ALARM [DISPATCHER]",  // warning string
                             config.queueSizeLowWatermark(),
                             config.queueSizeHighWatermark(),
                             config.queueSize(),
                             config.queueSize(),
                             bdlf::PlaceHolders::_1));  // state

    // Create stat context for the client's queue
    DispatcherContext* context = d_contexts[type].get();
    context->d_statContexts.at(processorId) =
        mqbstat::DispatcherStatsUtil::initializeQueueStatContext(
            context->d_clientStatContext_mp.get(),
            queueName,
            mqbi::DispatcherClientType::toAscii(type),
            processorId,
            d_allocator_p);
    return queue;
}

void Dispatcher::queueEventCb(mqbi::DispatcherClientType::Enum type,
                              int                              processorId,
                              const ProcessorPool::EventSp&    event)
{
    if (event) {
        BALL_LOG_TRACE << "Dispatching Event to queue " << processorId
                       << " of " << type << " dispatcher: " << *event;

        const bsls::Types::Int64 queuedTime =
            bmqsys::Time::highResolutionTimer() - event->enqueueTime();

        DispatcherContext& dispatcherContext = *(d_contexts[type]);

        if (event->type() == mqbi::DispatcherEventType::e_DISPATCHER) {
            const mqbi::DispatcherDispatcherEvent* realEvent =
                event->asDispatcherEvent();

            // We must flush now (and irrespective of a callback actually being
            // set on the event) to ensure the flushList is empty before
            // executing the callback: this dispatcher event may correspond to
            // the destruction of the Client, and guaranteeing this client is
            // not (and will not be added) to the flushList is actually the
            // whole purpose of the 'e_DISPATCHER' event type.
            flushClients(type, processorId);

            if (!realEvent->callback().empty()) {
                // A callback may not have been set if all we wanted was to
                // execute the 'finalizeCallback' of the event.
                realEvent->callback()();
            }
        }
        else {
            event->destination()->onDispatcherEvent(*event.get());
            if (!event->destination()
                     ->dispatcherClientData()
                     .addedToFlushList()) {
                dispatcherContext.d_flushList[processorId].emplace_back(
                    event->destination());
                event->destination()
                    ->dispatcherClientData()
                    .setAddedToFlushList(true);
            }
        }

        // Update stats
        mqbstat::DispatcherStats::onDequeue(
            dispatcherContext.d_statContexts[processorId].get(),
            queuedTime);
    }
    else {
        // Empty `event` means queue is empty
        flushClients(type, processorId);
    }
}

void Dispatcher::flushClients(mqbi::DispatcherClientType::Enum type,
                              int                              processorId)
{
    // executed by the *DISPATCHER* thread
    bmqu::GateKeeper::Status status(d_flushClientsGate);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!status.isOpen())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    DispatcherContext& context = *(d_contexts[type]);
    DispatcherClientPtrVector& flushList = context.d_flushList[processorId];
    for (size_t i = 0; i < flushList.size(); ++i) {
        flushList[i]->flush();
        flushList[i]->dispatcherClientData().setAddedToFlushList(false);
    }
    flushList.clear();
}

Dispatcher::Dispatcher(const mqbcfg::DispatcherConfig& config,
                       bmqst::StatContext*             statContext,
                       bdlmt::EventScheduler*          scheduler,
                       bslma::Allocator*               allocator)
: d_allocator_p(allocator)
, d_isStarted(false)
, d_config(config)
, d_scheduler_p(scheduler)
, d_contexts(allocator)
, d_statContext_p(statContext)
, d_defaultEventSource_sp(
      bsl::allocate_shared<mqba::Dispatcher_EventSource>(allocator))
, d_customEventSources(allocator)
, d_customEventSources_mtx()
, d_flushClientsGate()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(scheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);
    BSLS_ASSERT_SAFE(statContext);

    d_flushClientsGate.open();
}

Dispatcher::~Dispatcher()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

int Dispatcher::start(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "start() can only be called once on this object");

    BALL_LOG_INFO << "Starting dispatcher " << d_config;

    int rc = 0;

    d_contexts.resize(mqbi::DispatcherClientType::k_COUNT);

    // SESSION
    rc = startContext(errorDescription,
                      mqbi::DispatcherClientType::e_SESSION,
                      d_config.sessions());
    if (rc != 0) {
        return rc;  // RETURN
    }

    // QUEUE
    rc = startContext(errorDescription,
                      mqbi::DispatcherClientType::e_QUEUE,
                      d_config.queues());
    if (rc != 0) {
        return rc;  // RETURN
    }

    // CLUSTER
    rc = startContext(errorDescription,
                      mqbi::DispatcherClientType::e_CLUSTER,
                      d_config.clusters());
    if (rc != 0) {
        return rc;  // RETURN
    }

    executeOnAllQueues(
        bdlf::BindUtil::bind(&bmqsys::ThreadUtil::setCurrentThreadName,
                             "bmqDispSession"),
        mqbi::DispatcherClientType::e_SESSION);
    executeOnAllQueues(
        bdlf::BindUtil::bind(&bmqsys::ThreadUtil::setCurrentThreadName,
                             "bmqDispQueue"),
        mqbi::DispatcherClientType::e_QUEUE);
    executeOnAllQueues(
        bdlf::BindUtil::bind(&bmqsys::ThreadUtil::setCurrentThreadName,
                             "bmqDispCluster"),
        mqbi::DispatcherClientType::e_CLUSTER);

    d_isStarted = true;

    return 0;
}

void Dispatcher::disableFlushClients()
{
    d_flushClientsGate.close();
}

void Dispatcher::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

#define STOP_AND_CLEAR(OBJ)                                                   \
    if (OBJ) {                                                                \
        OBJ->stop();                                                          \
        OBJ.clear();                                                          \
    }

    DispatcherContext* context = 0;

    // Shutdown the queue dispatcher before the session one
    // After the application stops and invalidates sessions, they are not
    // source of events for queues anymore.  The reverse is not true, a queue
    // can generate session event (tearDownAllQueuesDone or countUnconfirmed).
    context = d_contexts[mqbi::DispatcherClientType::e_QUEUE].get();
    STOP_AND_CLEAR(context->d_processorPool_mp);
    STOP_AND_CLEAR(context->d_threadPool_mp);

    // Shutdown the  dispatcher
    context = d_contexts[mqbi::DispatcherClientType::e_SESSION].get();
    STOP_AND_CLEAR(context->d_processorPool_mp);
    STOP_AND_CLEAR(context->d_threadPool_mp);

    // Shutdown the cluster dispatcher
    context = d_contexts[mqbi::DispatcherClientType::e_CLUSTER].get();
    STOP_AND_CLEAR(context->d_processorPool_mp);
    STOP_AND_CLEAR(context->d_threadPool_mp);

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_customEventSources_mtx);
        d_customEventSources.clear();
    }

#undef STOP_AND_CLEAR

    // Clear all stat sub contexts
    d_statContext_p->clearSubcontexts();
}

mqbi::Dispatcher::ProcessorHandle
Dispatcher::registerClient(mqbi::DispatcherClient*           client,
                           mqbi::DispatcherClientType::Enum  type,
                           mqbi::Dispatcher::ProcessorHandle handle)
{
    switch (type) {
    case mqbi::DispatcherClientType::e_SESSION:
    case mqbi::DispatcherClientType::e_QUEUE:
    case mqbi::DispatcherClientType::e_CLUSTER: {
        DispatcherContext& context = *(d_contexts[type]);

        int processor = static_cast<int>(handle);
        if (handle == mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE) {
            processor = context.d_loadBalancer.getProcessorForClient(client);
        }
        else {
            context.d_loadBalancer.setProcessorForClient(client, processor);
        }
        client->dispatcherClientData()
            .setDispatcher(this)
            .setClientType(type)
            .setProcessorHandle(processor);
        client->setThreadId(
            context.d_processorPool_mp->queueThreadId(processor));
        client->setEventSource(context.d_eventSources[processor]);

        BALL_LOG_DEBUG << "Registered a new client to the dispatcher "
                       << "[Client: " << client->description()
                       << ", type: " << type << ", processor: " << processor
                       << "]";

        return processor;  // RETURN
    }  // break;
    case mqbi::DispatcherClientType::e_UNDEFINED:
    default: {
        BALL_LOG_ERROR << "#DISPATCHER_INVALID_CLIENT "
                       << "Registering client of invalid type [type: "
                       << client->dispatcherClientData().clientType()
                       << ", client: '" << client->description() << "']";
        BSLS_ASSERT_OPT(false && "Invalid client type");
    }
    }

    return mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE;
}

void Dispatcher::unregisterClient(mqbi::DispatcherClient* client)
{
    mqbi::DispatcherClientType::Enum type =
        client->dispatcherClientData().clientType();
    switch (type) {
    case mqbi::DispatcherClientType::e_SESSION:
    case mqbi::DispatcherClientType::e_QUEUE:
    case mqbi::DispatcherClientType::e_CLUSTER: {
        d_contexts[type]->d_loadBalancer.removeClient(client);
    } break;
    case mqbi::DispatcherClientType::e_UNDEFINED:
    default: {
        BALL_LOG_ERROR << "#DISPATCHER_INVALID_CLIENT "
                       << "UnRegistering client of invalid type [type: "
                       << client->dispatcherClientData().clientType()
                       << ", client: '" << client->description() << "']";
        BSLS_ASSERT_SAFE(false && "Invalid dispatcher client type");
        return;  // RETURN
    }
    }

    BALL_LOG_DEBUG << "UnRegistered client from the dispatcher "
                   << "[Client: '" << client->description() << "'"
                   << ", type: " << type << ", processor: "
                   << client->dispatcherClientData().processorHandle() << "]";

    // Invalidate the client's processor handle
    client->dispatcherClientData().setProcessorHandle(
        mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE);
}

void Dispatcher::dispatchEvent(mqbi::Dispatcher::DispatcherEventRvRef event,
                               mqbi::DispatcherClient* destination)
{
    BALL_LOG_TRACE << "Enqueuing Event to '" << destination->description()
                   << "': " << *bslmf::MovableRefUtil::access(event);

    bslmf::MovableRefUtil::access(event)->setDestination(destination);

    dispatchEvent(bslmf::MovableRefUtil::move(event),
                  destination->dispatcherClientData().clientType(),
                  destination->dispatcherClientData().processorHandle());
}

void Dispatcher::dispatchEvent(mqbi::Dispatcher::DispatcherEventRvRef event,
                               mqbi::DispatcherClientType::Enum       type,
                               mqbi::Dispatcher::ProcessorHandle      handle)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle != mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE);

    BALL_LOG_TRACE << "Enqueuing Event to processor " << handle << " of "
                   << type << ": " << *bslmf::MovableRefUtil::access(event);

    switch (type) {
    case mqbi::DispatcherClientType::e_SESSION:
    case mqbi::DispatcherClientType::e_QUEUE:
    case mqbi::DispatcherClientType::e_CLUSTER: {
        DispatcherContext* dispatcherContext = d_contexts[type].get();

        bslmf::MovableRefUtil::access(event)->setEnqueueTime(
            bmqsys::Time::highResolutionTimer());

        dispatcherContext->d_processorPool_mp->enqueueEvent(
            bslmf::MovableRefUtil::move(event),
            handle);

        // Update stats
        mqbstat::DispatcherStats::onEnqueue(
            dispatcherContext->d_statContexts[handle].get());

    } break;
    case mqbi::DispatcherClientType::e_UNDEFINED:
    default: {
        BSLS_ASSERT_OPT(false && "Invalid destination type");
    }
    }
}

void Dispatcher::executeOnAllQueues(
    const mqbi::Dispatcher::VoidFunctor& functor,
    mqbi::DispatcherClientType::Enum     type,
    const mqbi::Dispatcher::VoidFunctor& doneCallback)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type != mqbi::DispatcherClientType::e_UNDEFINED);

    DispatcherContext* context_p = d_contexts[type].get();

    // Pointers to the pool to enqueue the event to.
    ProcessorPool* processorPool = context_p->d_processorPool_mp.get();
    BSLS_ASSERT_SAFE(processorPool);

    BALL_LOG_TRACE << "Enqueuing Event to ALL '" << type << "' dispatcher "
                   << "queues [hasFinalizeCallback: "
                   << (doneCallback ? "yes" : "no") << "]";

    bsl::shared_ptr<mqbi::DispatcherEvent> qEvent =
        d_defaultEventSource_sp->getEvent();
    qEvent->setType(mqbi::DispatcherEventType::e_DISPATCHER)
        .setEnqueueTime(bmqsys::Time::highResolutionTimer());
    qEvent->callback().set(functor);
    qEvent->finalizeCallback().set(doneCallback);
    processorPool->enqueueEventOnAllQueues(
        bslmf::MovableRefUtil::move(qEvent));

    // Update stats for all queues
    for (size_t i = 0; i < context_p->d_statContexts.size(); ++i) {
        mqbstat::DispatcherStats::onEnqueue(
            context_p->d_statContexts[i].get());
    }
}

void Dispatcher::execute(const mqbi::Dispatcher::VoidFunctor& functor,
                         mqbi::DispatcherClient*              client,
                         mqbi::DispatcherEventType::Enum      type)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(client);
    BSLS_ASSERT_SAFE(type == mqbi::DispatcherEventType::e_CALLBACK ||
                     type == mqbi::DispatcherEventType::e_DISPATCHER);
    BSLS_ASSERT_SAFE(functor);

    bsl::shared_ptr<mqbi::DispatcherEvent> event =
        d_defaultEventSource_sp->getEvent();
    (*event)
        .setType(type)
        .setEnqueueTime(bmqsys::Time::highResolutionTimer())
        .callback()
        .set(functor);

    dispatchEvent(bslmf::MovableRefUtil::move(event), client);
}

void Dispatcher::execute(const mqbi::Dispatcher::VoidFunctor& functor,
                         const mqbi::DispatcherClientData&    client)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(functor);

    bsl::shared_ptr<mqbi::DispatcherEvent> event =
        d_defaultEventSource_sp->getEvent();
    (*event)
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
        .setEnqueueTime(bmqsys::Time::highResolutionTimer())
        .callback()
        .set(functor);

    dispatchEvent(bslmf::MovableRefUtil::move(event),
                  client.clientType(),
                  client.processorHandle());
}

void Dispatcher::synchronize(mqbi::DispatcherClient* client)
{
    synchronize(client->dispatcherClientData().clientType(),
                client->dispatcherClientData().processorHandle());
}

void Dispatcher::synchronize(mqbi::DispatcherClientType::Enum  type,
                             mqbi::Dispatcher::ProcessorHandle handle)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_contexts[type]->d_processorPool_mp->queueThreadId(handle) !=
        bslmt::ThreadUtil::selfId());  // Deadlock detection

    typedef void (bslmt::Semaphore::*PostFn)();

    bslmt::Semaphore                       semaphore;
    bsl::shared_ptr<mqbi::DispatcherEvent> event =
        d_defaultEventSource_sp->getEvent();
    (*event)
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
        .setCallback(
            bdlf::BindUtil::bind(static_cast<PostFn>(&bslmt::Semaphore::post),
                                 &semaphore));
    dispatchEvent(bslmf::MovableRefUtil::move(event), type, handle);
    semaphore.wait();
}

bmqex::Executor
Dispatcher::executor(const mqbi::DispatcherClient* client) const
{
    // PRECONDITIONS
    BSLS_ASSERT(client);
    BSLS_ASSERT(client->dispatcher() == this);
    BSLS_ASSERT(client->dispatcherClientData().clientType() !=
                mqbi::DispatcherClientType::e_UNDEFINED);

    return Dispatcher_Executor(this, client);
}

bsl::shared_ptr<mqbi::DispatcherEventSource> Dispatcher::createEventSource()
{
    bsl::shared_ptr<mqbi::DispatcherEventSource> res =
        bsl::allocate_shared<mqba::Dispatcher_EventSource>(d_allocator_p);
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_customEventSources_mtx);
        d_customEventSources.push_back(res);
    }
    return res;
}

}  // close package namespace
}  // close enterprise namespace
