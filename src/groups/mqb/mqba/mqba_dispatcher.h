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

// mqba_dispatcher.h                                                  -*-C++-*-
#ifndef INCLUDED_MQBA_DISPATCHER
#define INCLUDED_MQBA_DISPATCHER

//@PURPOSE: Provide an event dispatcher at the core of BlazingMQ broker.
//
//@CLASSES:
//  mqba::Dispatcher: Event dispatcher.
//
//@SEE_ALSO:
//  mqbi::Dispatcher: Protocol implemented by this dispatcher
//
//@DESCRIPTION: 'mqba::Dispatcher' is an implementation of the
// 'mqbi::Dispatcher' protocol, using the bmqc::MultiQueueThreadPool. This
// dispatcher supports three types of isolated independent pools of threads and
// queues: one for the client sessions, one for the queues, and one for
// clusters.
//
/// Thread Safety
///-------------
// 'mqba::Dispatcher' is thread safe.
//
/// Executors support
///-----------------
// As required by the 'mqbi::Dispatcher' protocol, this implementation provides
// two types of executors, each available through the dispatcher's 'executor'
// and 'clientExecutor' member functions respectively.  Provided executors
// compares equal only if they refer to the same processor (for executors
// returned by 'executor'), or if they refer to the same client (for executors
// returned by 'clientExecutor').  A call to 'dispatch' on such executors
// performed from within the executor's associated processor thread results in
// the submitted functor to be executed in-place.  A call to 'dispatch' from
// outside of the executor's associated processor thread is equivalent to a
// call to 'post'.

// MQB

#include <mqbcfg_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbu_loadbalancer.h>

#include <bmqc_multiqueuethreadpool.h>
#include <bmqex_executor.h>

// BDE
#include <ball_log.h>
#include <bdlmt_threadpool.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}

namespace mqba {

// FORWARD DECLARATION
class Dispatcher;

// =========================
// class Dispatcher_Executor
// =========================

/// Provides an executor suitable for submitting function objects on an
/// dispatcher's processor.
///
/// Note that this class conforms to the Executor concept as defined in
/// the `bmqex` package documentation.
///
/// Note also that it is undefined behavior to submit work on this
/// executor unless its associated dispatcher is started.
///
/// Note also that this executor can be used to submit work event after the
/// dispatcher client used to initialize the executor has been unregistered
/// from the executor's associated dispatcher.
class Dispatcher_Executor {
  private:
    // PRIVATE DATA
    bmqc::MultiQueueThreadPool<mqbi::DispatcherEvent>* d_processorPool_p;

    mqbi::Dispatcher::ProcessorHandle d_processorHandle;

  public:
    // CREATORS

    /// Create a `Dispatcher_Executor` object for executing function objects
    /// on a processor owned by the specified `dispacher` and in charge of
    /// the specified `client`.  The behavior is undefined unless the
    /// specified `client` is registered on the specified `dispacher` and
    /// the client type is not `e_UNDEFINED` or `e_ALL`.
    Dispatcher_Executor(const Dispatcher*             dispacher,
                        const mqbi::DispatcherClient* client)
        BSLS_CPP11_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return `true` if `*this` refer to the same processor as `rhs`, and
    /// `false` otherwise.
    bool operator==(const Dispatcher_Executor& rhs) const BSLS_CPP11_NOEXCEPT;

    /// Submit the specified function object `f` to be executed on the
    /// executor's associated processor.  Return immediately without waiting
    /// for the submitted function object to complete.
    void post(const bsl::function<void()>& f) const;

    /// If this function is called from the thread owned by the executor's
    /// associated processor, invoke the specified function object `f`
    /// in-place as if by `f()`.  Otherwise, submit the function object for
    /// execution as if by `post(f)`.
    void dispatch(const bsl::function<void()>& f) const;
};

// ===============================
// class Dispatcher_ClientExecutor
// ===============================

/// Provides an executor suitable for submitting function objects on an
/// dispatcher's processor to be executed by a dispatcher's client.
///
/// Note that this class conforms to the Executor concept as defined in
/// the `bmqex` package documentation.
///
/// Note also that it is undefined behavior to submit work on this
/// executor unless its associated dispatcher is started and the
/// dispatcher's client used to initialize the executor has not been
/// unregistered from the executor's associated dispatcher.
class Dispatcher_ClientExecutor {
  private:
    // PRIVATE DATA
    const mqbi::DispatcherClient* d_client_p;

  private:
    // PRIVATE ACCESSORS

    /// Return a pointer to the processor pool used to submit work.
    bmqc::MultiQueueThreadPool<mqbi::DispatcherEvent>*
    processorPool() const BSLS_CPP11_NOEXCEPT;

    /// Return the handle of the associated processor.
    mqbi::Dispatcher::ProcessorHandle
    processorHandle() const BSLS_CPP11_NOEXCEPT;

  public:
    // CREATORS

    /// Create a `Dispatcher_ClientExecutor` object for executing function
    /// objects by the specified `client` on a processor in charge of that
    /// client owned by the specified `dispacher`.  The behavior is
    /// undefined unless the specified `client` is registered on the
    /// specified `dispacher` and the client type is not `e_UNDEFINED` or
    /// `e_ALL`.
    Dispatcher_ClientExecutor(const Dispatcher*             dispacher,
                              const mqbi::DispatcherClient* client)
        BSLS_CPP11_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return `true` if `*this` refer to the same client as `rhs`, and
    /// `false` otherwise.
    bool
    operator==(const Dispatcher_ClientExecutor& rhs) const BSLS_CPP11_NOEXCEPT;

    /// Submit the specified function object `f` to be executed by the
    /// executor's associated client on the executor's associated processor.
    /// Return immediately without waiting for the submitted function object
    /// to complete.
    void post(const bsl::function<void()>& f) const;

    /// If this function is called from the thread owned by the executor's
    /// associated processor, invoke the specified function object `f`
    /// in-place as if by `f()`.  Otherwise, submit the function object for
    /// execution as if by `post(f)`.
    void dispatch(const bsl::function<void()>& f) const;
};

// ================
// class Dispatcher
// ================

/// This class provides an implementation of the `mqbi::Dispatcher`
/// protocol, using the bmqc::MultiQueueThreadPool
class Dispatcher BSLS_CPP11_FINAL : public mqbi::Dispatcher {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBA.DISPATCHER");

  private:
    // PRIVATE TYPES
    typedef bmqc::MultiQueueThreadPool<mqbi::DispatcherEvent> ProcessorPool;

    typedef bslma::ManagedPtr<bdlmt::ThreadPool> ThreadPoolMp;

    typedef bslma::ManagedPtr<ProcessorPool> ProcessorPoolMp;

    typedef bsl::vector<mqbi::DispatcherClient*> DispatcherClientPtrVector;

    /// Context for a dispatcher, with threads and pools
    struct DispatcherContext {
      private:
        // NOT IMPLEMENTED
        DispatcherContext(const DispatcherContext&) BSLS_CPP11_DELETED;

        /// Copy constructor and assignment operator are not implemented.
        DispatcherContext&
        operator=(const DispatcherContext&) BSLS_CPP11_DELETED;

      public:
        // PUBLIC DATA
        ThreadPoolMp d_threadPool_mp;
        // Thread Pool to use

        ProcessorPoolMp d_processorPool_mp;
        // Processor Pool to use

        mqbu::LoadBalancer<mqbi::DispatcherClient> d_loadBalancer;
        // The object responsible
        // for distributing clients
        // across processors

        bsl::vector<DispatcherClientPtrVector> d_flushList;
        // Vector of vector of
        // pointers to
        // DispatcherClients, with
        // the clients for which a
        // flush needs to be
        // called.  The first index
        // of the vector
        // corresponds to the
        // processor.

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(DispatcherContext,
                                       bslma::UsesBslmaAllocator)

        // CREATORS

        /// Create a new object with the specified `config` and using the
        /// specified `allocator`.
        DispatcherContext(const mqbcfg::DispatcherProcessorConfig& config,
                          bslma::Allocator*                        allocator);
    };

    typedef bsl::shared_ptr<DispatcherContext> DispatcherContextSp;

  private:
    // NOT IMPLEMENTED
    Dispatcher(const Dispatcher&) BSLS_CPP11_DELETED;
    Dispatcher& operator=(const Dispatcher&) BSLS_CPP11_DELETED;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    bool d_isStarted;
    // True if this component is started

    mqbcfg::DispatcherConfig d_config;
    // Configuration for the dispatcher

    bdlmt::EventScheduler* d_scheduler_p;
    // Event scheduler to use

    bsl::vector<DispatcherContextSp> d_contexts;
    // The various context, one for each
    // ClientType

    // FRIENDS
    friend class Dispatcher_ClientExecutor;
    friend class Dispatcher_Executor;

  private:
    // PRIVATE MANIPULATORS

    /// Start the dispatcher context associated to clients of the specified
    /// `type`, using the specified `config` and return 0 on success, or
    /// return a non-zero value and populate the specified
    /// `errorDescription` on error.
    int startContext(bsl::ostream&                            errorDescription,
                     mqbi::DispatcherClientType::Enum         type,
                     const mqbcfg::DispatcherProcessorConfig& config);

    /// Create a queue for the multi-fixed queue thread pool in charge of
    /// dispatcher client of the specified `type` and using the specified
    /// `config`.  This queue corresponds to the specified `processorId` and
    /// the specified `allocator` should be used to create it.  The
    /// specified `ret` can be used to set a context for the queue.
    ProcessorPool::Queue*
    queueCreator(mqbi::DispatcherClientType::Enum             type,
                 const mqbcfg::DispatcherProcessorParameters& config,
                 ProcessorPool::QueueCreatorRet*              ret,
                 int                                          processorId,
                 bslma::Allocator*                            allocator);

    /// Callback when a new object in the specified `event` and having the
    /// associated specified `context` is dispatched for the queue in charge
    /// of dispatcher client of the specified `type`, having the specified
    /// `processorId`.
    void queueEventCb(mqbi::DispatcherClientType::Enum type,
                      int                              processorId,
                      void*                            context,
                      const ProcessorPool::Event*      event);

    /// Flush clients of the specified `type` for the specified
    /// `processorId`.
    void flushClients(mqbi::DispatcherClientType::Enum type, int processorId);

    /// This method is invoked when a new client of the specified `type` is
    /// registered to the dispatcher, from the thread associated to that new
    /// client that is mapped to the specified `processorId`.
    void onNewClient(mqbi::DispatcherClientType::Enum type, int processorId);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Dispatcher, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a dispatcher using the specified `config` and `scheduler`.
    /// All memory allocation will be performed using the specified
    /// `allocator`.
    Dispatcher(const mqbcfg::DispatcherConfig& config,
               bdlmt::EventScheduler*          scheduler,
               bslma::Allocator*               allocator);

    /// Destructor
    ~Dispatcher() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start the `Dispatcher`.  Return 0 on success and non-zero otherwise
    /// populating the specified `errorDescription` with the reason of the
    /// error.
    int start(bsl::ostream& errorDescription);

    /// Stop the `Dispatcher`.
    void stop();

    /// Based on the specified `type`, associate the specified `client` to
    /// one of the processors of the dispatcher if the optionally specified
    /// `handle` is invalid, or to the provided `handle` if it is valid, and
    /// fill in the `processorHandle` and `dispatcherClientType` in the
    /// client's `dispatcherClientData` member.  This operation is a no-op
    /// if the `client` is already associated with a processor *and*
    /// `handle` is invalid.  If `handle` is valid, behavior is undefined
    /// unless `client` is not associated with any processor.  Note that
    /// specifying a valid `handle` is useful when BlazingMQ broker requires
    /// a client to be associated to same processor across it's (broker's)
    /// instantiations.
    mqbi::Dispatcher::ProcessorHandle
    registerClient(mqbi::DispatcherClient*           client,
                   mqbi::DispatcherClientType::Enum  type,
                   mqbi::Dispatcher::ProcessorHandle handle =
                       mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE)
        BSLS_KEYWORD_OVERRIDE;

    /// Remove the association of the specified `client` from its
    /// processor.  If the `client` is not associated with any processor,
    /// this method has no effect.
    void
    unregisterClient(mqbi::DispatcherClient* client) BSLS_KEYWORD_OVERRIDE;

    mqbi::DispatcherEvent*
    getEvent(mqbi::DispatcherClientType::Enum type) BSLS_KEYWORD_OVERRIDE;
    // Retrieve an event from the pool to send to a client of the specified
    // 'type'.  This event *must* be enqueued by calling 'dispatchEvent',
    // otherwise it will be leaked.

    mqbi::DispatcherEvent*
    getEvent(const mqbi::DispatcherClient* client) BSLS_KEYWORD_OVERRIDE;
    // Retrieve an event from the pool to send to the specified 'client'.
    // This event *must* be enqueued by calling 'dispatchEvent' otherwise
    // it will be leaked.

    void
    dispatchEvent(mqbi::DispatcherEvent*  event,
                  mqbi::DispatcherClient* destination) BSLS_KEYWORD_OVERRIDE;
    // Dispatch the specified 'event' to the specified 'destination'.

    void dispatchEvent(mqbi::DispatcherEvent*            event,
                       mqbi::DispatcherClientType::Enum  type,
                       mqbi::Dispatcher::ProcessorHandle handle)
        BSLS_KEYWORD_OVERRIDE;
    // Dispatch the specified 'event' to the queue associated with the
    // specified 'type' and 'handle'.  The behavior is undefined unless the
    // 'event' was obtained by a call to 'getEvent'.

    /// Execute the specified `functor` in the processors in charge of
    /// clients of the specified `type`, and invoke the optionally specified
    /// `doneCallback` (if any) when all the relevant processors are done
    /// executing the `functor`.
    void execute(const mqbi::Dispatcher::ProcessorFunctor& functor,
                 mqbi::DispatcherClientType::Enum          type,
                 const mqbi::Dispatcher::VoidFunctor&      doneCallback =
                     mqbi::Dispatcher::VoidFunctor()) BSLS_KEYWORD_OVERRIDE;

    void execute(const mqbi::Dispatcher::VoidFunctor& functor,
                 mqbi::DispatcherClient*              client,
                 mqbi::DispatcherEventType::Enum type) BSLS_KEYWORD_OVERRIDE;
    // Execute the specified 'functor', using the specified dispatcher
    // 'type', in the processor associated to the specified 'client'.  The
    // behavior is undefined unless 'type' is 'e_DISPATCHER' or
    // 'e_CALLBACK'.

    void
    execute(const mqbi::Dispatcher::VoidFunctor& functor,
            const mqbi::DispatcherClientData&    client) BSLS_KEYWORD_OVERRIDE;
    // Execute the specified 'functor', using the 'e_DISPATCHER' event
    // type, in the processor associated to the specified 'client'.

    void synchronize(mqbi::DispatcherClient* client) BSLS_KEYWORD_OVERRIDE;

    /// Enqueue an event to the processor associated to the specified
    /// `client` or pair of the specified `type` and `handle` and block
    /// until this event gets dequeued.  This is typically used by a
    /// `dispatcherClient`, in its destructor, to drain the dispatcher's
    /// queue and ensure no more events are to be expected for that
    /// `client`.  The behavior is undefined if `synchronize` is being
    /// invoked from the `client`s thread.
    void synchronize(mqbi::DispatcherClientType::Enum  type,
                     mqbi::Dispatcher::ProcessorHandle handle)
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    int numProcessors(mqbi::DispatcherClientType::Enum type) const
        BSLS_KEYWORD_OVERRIDE;
    // Return number of processors dedicated for dispatching clients of the
    // specified 'type'.

    bool inDispatcherThread(const mqbi::DispatcherClient* client) const
        BSLS_KEYWORD_OVERRIDE;
    // Return whether the current thread is the dispatcher thread
    // associated to the specified 'client'.  This is usefull for
    // preconditions assert validation.

    bool inDispatcherThread(const mqbi::DispatcherClientData* data) const
        BSLS_KEYWORD_OVERRIDE;
    // Return whether the current thread is the dispatcher thread
    // associated to the specified dispatcher client 'data'.  This is
    // useful for preconditions assert validation.

    /// Return an executor object suitable for executing function objects on
    /// the processor in charge of the specified `client`.  The behavior is
    /// undefined unless the specified `client` is registered on this
    /// dispatcher and the client type is not `e_UNDEFINED` or `e_ALL`.
    ///
    /// Note that submitting work on the returned executor is undefined
    /// behavior unless this dispatcher is started.
    ///
    /// Note also that the returned executor can be used to submit work even
    /// after the specified `client` has been unregistered from this
    /// dispatcher.
    bmqex::Executor
    executor(const mqbi::DispatcherClient* client) const BSLS_KEYWORD_OVERRIDE;

    /// Return an executor object suitable for executing function objects by
    /// the specified `client` on the processor in charge of that client.
    /// The behavior is undefined unless the specified `client` is
    /// registered on this dispatcher and the client type is not
    /// `e_UNDEFINED` or `e_ALL`.
    ///
    /// Note that submitting work on the returned executor is undefined
    /// behavior unless this dispatcher is started or if the specified
    /// `client` was unregistered from this dispatcher.
    bmqex::Executor clientExecutor(const mqbi::DispatcherClient* client) const
        BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class Dispatcher
// ----------------

// MANIPULATORS
inline mqbi::DispatcherEvent*
Dispatcher::getEvent(mqbi::DispatcherClientType::Enum type)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(type != mqbi::DispatcherClientType::e_UNDEFINED &&
                     type != mqbi::DispatcherClientType::e_ALL);

    return &d_contexts[type]
                ->d_processorPool_mp->getUnmanagedEvent()
                ->object();
}

inline mqbi::DispatcherEvent*
Dispatcher::getEvent(const mqbi::DispatcherClient* client)
{
    return getEvent(client->dispatcherClientData().clientType());
}

inline void Dispatcher::dispatchEvent(mqbi::DispatcherEvent*  event,
                                      mqbi::DispatcherClient* destination)
{
    BALL_LOG_TRACE << "Enqueuing Event to '" << destination->description()
                   << "': " << *event;

    event->setDestination(destination);

    dispatchEvent(event,
                  destination->dispatcherClientData().clientType(),
                  destination->dispatcherClientData().processorHandle());
}

inline void Dispatcher::dispatchEvent(mqbi::DispatcherEvent*            event,
                                      mqbi::DispatcherClientType::Enum  type,
                                      mqbi::Dispatcher::ProcessorHandle handle)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle != mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE);

    BALL_LOG_TRACE << "Enqueuing Event to processor " << handle << " of "
                   << type << ": " << *event;

    switch (type) {
    case mqbi::DispatcherClientType::e_SESSION:
    case mqbi::DispatcherClientType::e_QUEUE:
    case mqbi::DispatcherClientType::e_CLUSTER: {
        d_contexts[type]->d_processorPool_mp->enqueueEvent(event, handle);
    } break;
    case mqbi::DispatcherClientType::e_UNDEFINED:
    case mqbi::DispatcherClientType::e_ALL:
    default: {
        BSLS_ASSERT_OPT(false && "Invalid destination type");
    }
    }
}

inline void Dispatcher::execute(const mqbi::Dispatcher::VoidFunctor& functor,
                                mqbi::DispatcherClient*              client,
                                mqbi::DispatcherEventType::Enum      type)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(client);
    BSLS_ASSERT_SAFE(type == mqbi::DispatcherEventType::e_CALLBACK ||
                     type == mqbi::DispatcherEventType::e_DISPATCHER);
    BSLS_ASSERT_SAFE(functor);

    mqbi::DispatcherEvent* event = getEvent(client);
    if (type == mqbi::DispatcherEventType::e_DISPATCHER) {
        (*event).makeDispatcherEvent().setCallback(
            mqbi::Dispatcher::voidToProcessorFunctor(functor));
    }
    else {
        (*event).makeCallbackEvent().setCallback(
            mqbi::Dispatcher::voidToProcessorFunctor(functor));
    }

    dispatchEvent(event, client);
}

inline void Dispatcher::execute(const mqbi::Dispatcher::VoidFunctor& functor,
                                const mqbi::DispatcherClientData&    client)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(functor);

    mqbi::DispatcherEvent* event = getEvent(client.clientType());

    (*event).makeDispatcherEvent().setCallback(
        mqbi::Dispatcher::voidToProcessorFunctor(functor));

    dispatchEvent(event, client.clientType(), client.processorHandle());
}

// ACCESSORS
inline int
Dispatcher::numProcessors(mqbi::DispatcherClientType::Enum type) const
{
    switch (type) {
    case mqbi::DispatcherClientType::e_SESSION: {
        return d_config.sessions().numProcessors();  // RETURN
    }  // break;
    case mqbi::DispatcherClientType::e_QUEUE: {
        return d_config.queues().numProcessors();  // RETURN
    }  // break;
    case mqbi::DispatcherClientType::e_CLUSTER: {
        return d_config.clusters().numProcessors();  // RETURN
    }  // break;
    case mqbi::DispatcherClientType::e_ALL: {
        return d_config.sessions().numProcessors() +
               d_config.queues().numProcessors() +
               d_config.clusters().numProcessors();  // RETURN
    }  // break;
    case mqbi::DispatcherClientType::e_UNDEFINED: {
        BSLS_ASSERT_OPT(false && "Invalid type");
        return -1;  // RETURN
    }  // break;
    }

    return 0;
}

inline bool
Dispatcher::inDispatcherThread(const mqbi::DispatcherClient* client) const
{
    return inDispatcherThread(&(client->dispatcherClientData()));
}

inline bool
Dispatcher::inDispatcherThread(const mqbi::DispatcherClientData* data) const

{
    mqbi::DispatcherClientType::Enum type = data->clientType();
    int                              proc = data->processorHandle();

    return (d_contexts[type]->d_processorPool_mp->queueThreadHandle(proc) ==
            bslmt::ThreadUtil::self());
}

}  // close package namespace
}  // close enterprise namespace

#endif
