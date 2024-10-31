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
#include <bmqu_memoutstream.h>

#include <bmqsys_threadutil.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_cstddef.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_managedptr.h>
#include <bslmt_semaphore.h>
#include <bsls_annotation.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {
const double k_QUEUE_STUCK_INTERVAL = 3 * 60.0;
}  // close unnamed namespace

// -------------------------
// class Dispatcher_Executor
// -------------------------

// CREATORS
Dispatcher_Executor::Dispatcher_Executor(const Dispatcher* dispacher,
                                         const mqbi::DispatcherClient* client)
    BSLS_CPP11_NOEXCEPT : d_processorPool_p(0),
                          d_processorHandle()
{
    // PRECONDITIONS
    BSLS_ASSERT(dispacher);
    BSLS_ASSERT(client);
    BSLS_ASSERT(client->dispatcher() == dispacher);
    BSLS_ASSERT(client->dispatcherClientData().clientType() !=
                    mqbi::DispatcherClientType::e_UNDEFINED &&
                client->dispatcherClientData().clientType() !=
                    mqbi::DispatcherClientType::e_ALL);
    BSLS_ASSERT(client->dispatcherClientData().processorHandle() !=
                mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE);

    // set processor
    d_processorPool_p = dispacher->d_contexts
                            .at(client->dispatcherClientData().clientType())
                            ->d_processorPool_mp.get();
    d_processorHandle = client->dispatcherClientData().processorHandle();
}

// ACCESSORS
bool Dispatcher_Executor::operator==(const Dispatcher_Executor& rhs) const
    BSLS_CPP11_NOEXCEPT
{
    return d_processorPool_p == rhs.d_processorPool_p &&
           d_processorHandle == rhs.d_processorHandle;
}

void Dispatcher_Executor::post(const bsl::function<void()>& f) const
{
    // PRECONDITIONS
    BSLS_ASSERT(f);
    BSLS_ASSERT(d_processorPool_p->isStarted());

    // create an event containing the function to be invoked on the processor
    bmqc::MultiQueueThreadPool<mqbi::DispatcherEvent>::Event* event =
        d_processorPool_p->getUnmanagedEvent();

    event->object()
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
        .callback()
        .setCallback(f);

    // submit the event
    int rc = d_processorPool_p->enqueueEvent(event, d_processorHandle);
    BSLS_ASSERT_OPT(rc == 0);

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

    if (d_processorPool_p->queueThreadHandle(d_processorHandle) ==
        bslmt::ThreadUtil::self()) {
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

// -------------------------------
// class Dispatcher_ClientExecutor
// -------------------------------

// PRIVATE ACCESSORS
bmqc::MultiQueueThreadPool<mqbi::DispatcherEvent>*
Dispatcher_ClientExecutor::processorPool() const BSLS_CPP11_NOEXCEPT
{
    const mqba::Dispatcher* dispatcher = static_cast<const mqba::Dispatcher*>(
        d_client_p->dispatcher());

    return dispatcher->d_contexts
        .at(d_client_p->dispatcherClientData().clientType())
        ->d_processorPool_mp.get();
}

mqbi::Dispatcher::ProcessorHandle
Dispatcher_ClientExecutor::processorHandle() const BSLS_CPP11_NOEXCEPT
{
    // PRECONFITIONS
    BSLS_ASSERT(d_client_p->dispatcherClientData().processorHandle() !=
                mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE);

    return d_client_p->dispatcherClientData().processorHandle();
}

// CREATORS
Dispatcher_ClientExecutor::Dispatcher_ClientExecutor(
    const Dispatcher*             dispacher,
    const mqbi::DispatcherClient* client) BSLS_CPP11_NOEXCEPT
: d_client_p(client)
{
    // PRECONDITIONS
    BSLS_ASSERT(dispacher);
    BSLS_ASSERT(client);
    BSLS_ASSERT(client->dispatcher() == dispacher);
    BSLS_ASSERT(client->dispatcherClientData().clientType() !=
                    mqbi::DispatcherClientType::e_UNDEFINED &&
                client->dispatcherClientData().clientType() !=
                    mqbi::DispatcherClientType::e_ALL);
}

// ACCESSORS
bool Dispatcher_ClientExecutor::operator==(
    const Dispatcher_ClientExecutor& rhs) const BSLS_CPP11_NOEXCEPT
{
    return d_client_p == rhs.d_client_p;
}

void Dispatcher_ClientExecutor::post(const bsl::function<void()>& f) const
{
    // PRECONDITIONS
    BSLS_ASSERT(f);
    BSLS_ASSERT(processorPool()->isStarted());

    // create an event containing the function to be invoked on the processor
    bmqc::MultiQueueThreadPool<mqbi::DispatcherEvent>::Event* event =
        processorPool()->getUnmanagedEvent();

    event->object()
        .setType(mqbi::DispatcherEventType::e_CALLBACK)
        .setDestination(const_cast<mqbi::DispatcherClient*>(d_client_p))
        .callback()
        .setCallback(f);

    // submit the event
    int rc = processorPool()->enqueueEvent(event, processorHandle());
    BSLS_ASSERT_OPT(rc == 0);

    // TODO: We should call 'releaseUnmanagedEvent' on the
    //      'bmqc::MultiQueueThreadPool' in case of exception to prevent the
    //      event from leaking. But somehow this method is declared but not
    //      implemented.
}

void Dispatcher_ClientExecutor::dispatch(const bsl::function<void()>& f) const
{
    // PRECONDITIONS
    BSLS_ASSERT(f);
    BSLS_ASSERT(processorPool()->isStarted());

    if (processorPool()->queueThreadHandle(processorHandle()) ==
        bslmt::ThreadUtil::self()) {
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
{
    // NOTHING
}

// ------------------------------------
// class Dispatcher::OnNewClientFunctor
// ------------------------------------

Dispatcher::OnNewClientFunctor::OnNewClientFunctor(
    Dispatcher*                      owner_p,
    mqbi::DispatcherClientType::Enum type,
    int                              processorId)
: d_owner_p(owner_p)
, d_type(type)
, d_processorId(processorId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_owner_p);
}

// ACCESSORS
void Dispatcher::OnNewClientFunctor::operator()() const
{
    // executed by the *DISPATCHER* thread

    // Resize the 'd_flushList' vector for that specified 'processorId', if
    // needed, to ensure it has enough space to hold all clients associated to
    // that processorId.
    DispatcherContext& context = *(d_owner_p->d_contexts[d_type]);

    int count = context.d_loadBalancer.clientsCountForProcessor(d_processorId);
    if (static_cast<int>(context.d_flushList[d_processorId].capacity()) <
        count) {
        context.d_flushList[d_processorId].reserve(count);
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
                             bdlf::PlaceHolders::_2,   // context*
                             bdlf::PlaceHolders::_3),  // event*
        bdlf::BindUtil::bind(&Dispatcher::queueCreator,
                             this,
                             type,
                             config.processorConfig(),
                             bdlf::PlaceHolders::_1,   // qCreatorRet*
                             bdlf::PlaceHolders::_2,   // processorId
                             bdlf::PlaceHolders::_3),  // allocator*
        d_allocator_p);

    processorPoolConfig.setName(mqbi::DispatcherClientType::toAscii(type))
        .setEventScheduler(d_scheduler_p)
        .setFinalizeEvents(ProcessorPool::Config::BMQC_FINALIZE_MULTI_QUEUE)
        .setMonitorAlarm("ALARM [DISPATCHER_QUEUE_STUCK] ",
                         bsls::TimeInterval(k_QUEUE_STUCK_INTERVAL));
    // TBD: .statContext(...) / .createSubcontext(true)
    //      We should have subcontext per each type of event (PUSH, PUT,
    //      CALLBACK, ACK, ...)

    processorPoolConfig.setGrowBy(64 * 1024);

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

Dispatcher::ProcessorPool::Queue* Dispatcher::queueCreator(
    mqbi::DispatcherClientType::Enum             type,
    const mqbcfg::DispatcherProcessorParameters& config,
    BSLS_ANNOTATION_UNUSED ProcessorPool::QueueCreatorRet* ret,
    int                                                    processorId,
    bslma::Allocator*                                      allocator)
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

    return queue;
}

void Dispatcher::queueEventCb(mqbi::DispatcherClientType::Enum type,
                              int                              processorId,
                              BSLS_ANNOTATION_UNUSED void*     context,
                              const ProcessorPool::Event*      event)
{
    switch (event->type()) {
    case ProcessorPool::Event::BMQC_USER: {
        BALL_LOG_TRACE << "Dispatching Event to queue " << processorId
                       << " of " << type << " dispatcher: " << event->object();
        if (event->object().type() ==
            mqbi::DispatcherEventType::e_DISPATCHER) {
            const mqbi::DispatcherDispatcherEvent* realEvent =
                event->object().asDispatcherEvent();

            // We must flush now (and irrespective of a callback actually being
            // set on the event) to ensure the flushList is empty before
            // executing the callback: this dispatcher event may correspond to
            // the destruction of the Client, and guaranteeing this client is
            // not (and will not be added) to the flushList is actually the
            // whole purpose of the 'e_DISPATCHER' event type.
            flushClients(type, processorId);

            if (realEvent->callback().hasCallback()) {
                // A callback may not have been set if all we wanted was to
                // execute the 'finalizeCallback' of the event.
                realEvent->callback()();
            }
        }
        else {
            DispatcherContext& dispatcherContext = *(d_contexts[type]);
            event->object().destination()->onDispatcherEvent(event->object());
            if (!event->object()
                     .destination()
                     ->dispatcherClientData()
                     .addedToFlushList()) {
                dispatcherContext.d_flushList[processorId].emplace_back(
                    event->object().destination());
                event->object()
                    .destination()
                    ->dispatcherClientData()
                    .setAddedToFlushList(true);
            }
        }
    } break;
    case ProcessorPool::Event::BMQC_QUEUE_EMPTY: {
        flushClients(type, processorId);
    } break;
    case ProcessorPool::Event::BMQC_FINALIZE_EVENT: {
        // We only set finalizeCallback on e_DISPATCHER events
        if (event->object().type() ==
            mqbi::DispatcherEventType::e_DISPATCHER) {
            const mqbi::DispatcherDispatcherEvent* realEvent =
                event->object().asDispatcherEvent();

            if (realEvent->finalizeCallback().hasCallback()) {
                BALL_LOG_TRACE << "Calling finalizeCallback on queue "
                               << processorId << " of " << type
                               << " dispatcher: " << event->object();
                realEvent->finalizeCallback()();
            }
        }
    } break;
    }
}

void Dispatcher::flushClients(mqbi::DispatcherClientType::Enum type,
                              int                              processorId)
{
    // executed by the *DISPATCHER* thread

    DispatcherContext& context = *(d_contexts[type]);
    for (size_t i = 0; i < context.d_flushList[processorId].size(); ++i) {
        context.d_flushList[processorId][i]->flush();
        context.d_flushList[processorId][i]
            ->dispatcherClientData()
            .setAddedToFlushList(false);
    }
    context.d_flushList[processorId].clear();
}

Dispatcher::Dispatcher(const mqbcfg::DispatcherConfig& config,
                       bdlmt::EventScheduler*          scheduler,
                       bslma::Allocator*               allocator)
: d_allocator_p(allocator)
, d_isStarted(false)
, d_config(config)
, d_scheduler_p(scheduler)
, d_contexts(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(scheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);
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

    if (bmqsys::ThreadUtil::k_SUPPORT_THREAD_NAME) {
        execute(bdlf::BindUtil::bind(&bmqsys::ThreadUtil::setCurrentThreadName,
                                     "bmqDispSession"),
                mqbi::DispatcherClientType::e_SESSION);
        execute(bdlf::BindUtil::bind(&bmqsys::ThreadUtil::setCurrentThreadName,
                                     "bmqDispQueue"),
                mqbi::DispatcherClientType::e_QUEUE);
        execute(bdlf::BindUtil::bind(&bmqsys::ThreadUtil::setCurrentThreadName,
                                     "bmqDispCluster"),
                mqbi::DispatcherClientType::e_CLUSTER);
    }

    d_isStarted = true;

    return 0;
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

#undef STOP_AND_CLEAR
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

        BALL_LOG_DEBUG << "Registered a new client to the dispatcher "
                       << "[Client: " << client->description()
                       << ", type: " << type << ", processor: " << processor
                       << "]";

        // Enqueue an event to resize (if needed) the flush vector to
        // accommodate for a new client.  This has to execute on the processor
        // thread, because the vector is not thread safe; and this must be done
        // before any event is being dispatched to this client (since that
        // would cause it to be added to the flush list).
        mqbi::DispatcherEvent* event =
            &context.d_processorPool_mp->getUnmanagedEvent()->object();
        (*event)
            .setType(mqbi::DispatcherEventType::e_DISPATCHER)
            .setDestination(client);  // TODO: not needed?

        // Build callback functor in-place.
        // The destructor for functor is called in `reset`.
        new (event->callback().place<OnNewClientFunctor>())
            OnNewClientFunctor(this, type, processor);

        context.d_processorPool_mp->enqueueEvent(event, processor);
        return processor;  // RETURN
    }  // break;
    case mqbi::DispatcherClientType::e_UNDEFINED:
    case mqbi::DispatcherClientType::e_ALL:
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
    case mqbi::DispatcherClientType::e_ALL:
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

void Dispatcher::execute(const mqbi::Dispatcher::VoidFunctor& functor,
                         mqbi::DispatcherClientType::Enum     type,
                         const mqbi::Dispatcher::VoidFunctor& doneCallback)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type != mqbi::DispatcherClientType::e_UNDEFINED);

    // Pointers to the pool to enqueue the event to.
    ProcessorPool* processorPool[mqbi::DispatcherClientType::k_COUNT];

    for (size_t i = 0; i < mqbi::DispatcherClientType::k_COUNT; ++i) {
        processorPool[i] = 0;
    }

    if (type == mqbi::DispatcherClientType::e_SESSION ||
        type == mqbi::DispatcherClientType::e_ALL) {
        processorPool[mqbi::DispatcherClientType::e_SESSION] =
            d_contexts[mqbi::DispatcherClientType::e_SESSION]
                ->d_processorPool_mp.get();
    }
    if (type == mqbi::DispatcherClientType::e_QUEUE ||
        type == mqbi::DispatcherClientType::e_ALL) {
        processorPool[mqbi::DispatcherClientType::e_QUEUE] =
            d_contexts[mqbi::DispatcherClientType::e_QUEUE]
                ->d_processorPool_mp.get();
    }
    if (type == mqbi::DispatcherClientType::e_CLUSTER ||
        type == mqbi::DispatcherClientType::e_ALL) {
        processorPool[mqbi::DispatcherClientType::e_CLUSTER] =
            d_contexts[mqbi::DispatcherClientType::e_CLUSTER]
                ->d_processorPool_mp.get();
    }

    BALL_LOG_TRACE << "Enqueuing Event to ALL '" << type << "' dispatcher "
                   << "queues [hasAFinalizeCallback: "
                   << (doneCallback ? "yes" : "no") << "]";

    for (size_t i = 0; i < mqbi::DispatcherClientType::k_COUNT; ++i) {
        if (processorPool[i] != 0) {
            mqbi::DispatcherEvent* qEvent =
                &processorPool[i]->getUnmanagedEvent()->object();
            qEvent->setType(mqbi::DispatcherEventType::e_DISPATCHER);
            qEvent->callback().setCallback(functor);
            qEvent->finalizeCallback().setCallback(doneCallback);
            processorPool[i]->enqueueEventOnAllQueues(qEvent);
        }
    }
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
        d_contexts[type]->d_processorPool_mp->queueThreadHandle(handle) !=
        bslmt::ThreadUtil::self());  // Deadlock detection

    typedef void (bslmt::Semaphore::*PostFn)();

    bslmt::Semaphore       semaphore;
    mqbi::DispatcherEvent* event = getEvent(type);
    (*event)
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
        .setCallback(
            bdlf::BindUtil::bind(static_cast<PostFn>(&bslmt::Semaphore::post),
                                 &semaphore));
    dispatchEvent(event, type, handle);
    semaphore.wait();
}

bmqex::Executor
Dispatcher::executor(const mqbi::DispatcherClient* client) const
{
    // PRECONDITIONS
    BSLS_ASSERT(client);
    BSLS_ASSERT(client->dispatcher() == this);
    BSLS_ASSERT(client->dispatcherClientData().clientType() !=
                    mqbi::DispatcherClientType::e_UNDEFINED &&
                client->dispatcherClientData().clientType() !=
                    mqbi::DispatcherClientType::e_ALL);

    return Dispatcher_Executor(this, client);
}

bmqex::Executor
Dispatcher::clientExecutor(const mqbi::DispatcherClient* client) const
{
    // PRECONDITIONS
    BSLS_ASSERT(client);
    BSLS_ASSERT(client->dispatcher() == this);
    BSLS_ASSERT(client->dispatcherClientData().clientType() !=
                    mqbi::DispatcherClientType::e_UNDEFINED &&
                client->dispatcherClientData().clientType() !=
                    mqbi::DispatcherClientType::e_ALL);

    return Dispatcher_ClientExecutor(this, client);
}

}  // close package namespace
}  // close enterprise namespace
