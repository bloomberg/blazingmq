// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbmock_dispatcher.h                                               -*-C++-*-
#ifndef INCLUDED_MQBMOCK_DISPATCHER
#define INCLUDED_MQBMOCK_DISPATCHER

//@PURPOSE: Provide a mock implementation of the 'mqbi::Dispatcher' interface.
//
//@CLASSES:
//  mqbmock::Dispatcher:       mock dispatcher implementation
//  mqbmock::DispatcherClient: mock client implementation
//
//@DESCRIPTION: This component provides a mock implementation,
// 'mqbmock::Dispatcher', of the 'mqbi::Dispatcher' interface, and a mock
// implementation, 'mqbmock::DispatcherClient', of the 'mqbi::DispatcherClient'
// interface, that are used to emulate a real dispatcher and a real client for
// testing purposes.
//
/// Notes
///------
// At the time of this writing, this component implements only those methods
// of the 'mqbi::Dispatcher' protocol that are needed for testing
// 'mqbblp::QueueEngine'.  Additionally, the set of methods that are specific
// to this component is the minimal set required for testing
// 'mqbblp::QueueEngine'.  These methods are denoted with a leading underscore
// ('_').

// MQB

#include <mqbi_dispatcher.h>

#include <bmqex_executor.h>

// BDE
#include <bsl_unordered_map.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>

namespace BloombergLP {
namespace mqbmock {

// ================
// class Dispatcher
// ================

/// Mock dispatcher implementation of the `mqbi::Dispatcher` protocol.
class Dispatcher : public mqbi::Dispatcher {
  private:
    // TYPES
    typedef bsl::unordered_map<const mqbi::DispatcherClient*,
                               mqbi::DispatcherEvent*>
        EventMap;
    // A map from clients to events.
  private:
    // DATA
    bool d_inDispatcherThread;
    // A flag indicating whether the
    // current thread is in the dispatcher
    // thread with respect to any client

    EventMap d_eventsForClients;
    // Maps clients to currently processed
    // events;

    bslma::Allocator* d_allocator_p;  // Allocator to use

  private:
    // NOT IMPLEMENTED
    Dispatcher(const Dispatcher&) BSLS_CPP11_DELETED;
    Dispatcher& operator=(const Dispatcher&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Dispatcher, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `mqbmock::Dispather` object.  Use the specified `allocator`
    /// for any memory allocation.
    explicit Dispatcher(bslma::Allocator* allocator);

    /// Destructor of this object.
    ~Dispatcher() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::Dispatcher)

    /// Associate the specified `client` to one of the dispatcher's
    /// processors in charge of clients of the specified `type`.  Use the
    /// processor provided in the optionally specified `handle` if it is
    /// valid, or let the dispatcher automatically affect a processor to the
    /// `client` (by using some internal load-balancing mechanism for
    /// example) if `handle` represents the `k_INVALID_PROCESSOR_HANDLE`.
    /// This operation is a no-op if the `client` is already associated with
    /// a processor *and* `handle` is invalid.  If `handle` if valid, the
    /// behavior is undefined unless `client` is not yet associated with any
    /// processor.  As of result of this operation, the
    /// `dispatcherClientData` of `client` will be populated.  Return the
    /// processor handle associated to `client`.
    ///
    /// NOTE: specifying a valid `handle` is useful when BlazingMQ broker
    ///       requires a client to be associated to the same processor
    ///       across brokers' instantiations.
    Dispatcher::ProcessorHandle
    registerClient(mqbi::DispatcherClient*           client,
                   mqbi::DispatcherClientType::Enum  type,
                   mqbi::Dispatcher::ProcessorHandle handle =
                       mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE)
        BSLS_KEYWORD_OVERRIDE;

    /// Remove the association of the specified `client` from its processor,
    /// and mark as invalid the `processorHandle` from the client's
    /// `dispatcherClientData` member.  This operation is a no-op if the
    /// `client` is not associated with any processor.
    void
    unregisterClient(mqbi::DispatcherClient* client) BSLS_KEYWORD_OVERRIDE;

    /// Retrieve an event from the event pool to send to the specified
    /// `client`.  Once populated, the returned event *must* be enqueued for
    /// processing by calling `dispatchEvent` otherwise it will be leaked.
    mqbi::DispatcherEvent*
    getEvent(const mqbi::DispatcherClient* client) BSLS_KEYWORD_OVERRIDE;

    /// Retrieve an event from the event pool to send to a client of the
    /// specified `type`.  Once populated, the returned event *must* be
    /// enqueued for processing by calling `dispatchEvent` otherwise it will
    /// be leaked.
    mqbi::DispatcherEvent*
    getEvent(mqbi::DispatcherClientType::Enum type) BSLS_KEYWORD_OVERRIDE;

    /// Dispatch the specified `event` to the specified `destination`.  The
    /// behavior is undefined unless `event` was obtained by a call to
    /// `getEvent` with a type matching the one of `destination`.
    void
    dispatchEvent(mqbi::DispatcherEvent*  event,
                  mqbi::DispatcherClient* destination) BSLS_KEYWORD_OVERRIDE;

    /// Dispatch the specified `event` to the processor in charge of clients
    /// of the specified `type` and associated with the specified `handle`.
    /// The behavior is undefined unless `event` was obtained by a call to
    /// `getEvent` with a matching `type`..
    void dispatchEvent(mqbi::DispatcherEvent*            event,
                       mqbi::DispatcherClientType::Enum  type,
                       mqbi::Dispatcher::ProcessorHandle handle)
        BSLS_KEYWORD_OVERRIDE;

    /// Execute the specified `functor`, using the optionally specified
    /// dispatcher `type`, in the processor associated to the specified
    /// `client`.  The behavior is undefined unless `type` is `e_DISPATCHER`
    /// or `e_CALLBACK`.
    void execute(const mqbi::Dispatcher::VoidFunctor& functor,
                 mqbi::DispatcherClient*              client,
                 mqbi::DispatcherEventType::Enum type) BSLS_KEYWORD_OVERRIDE;

    /// Execute the specified `functor`, using the `e_DISPATCHER` event
    /// type, in the processor associated to the specified `client`.
    void
    execute(const mqbi::Dispatcher::VoidFunctor& functor,
            const mqbi::DispatcherClientData&    client) BSLS_KEYWORD_OVERRIDE;

    /// Execute the specified `functor` in the processors in charge of
    /// clients of the specified `type`, and invoke the specified
    /// `doneCallback` (if any) when all the relevant processors are done
    /// executing the `functor`.
    void execute(const mqbi::Dispatcher::VoidFunctor& functor,
                 mqbi::DispatcherClientType::Enum     type,
                 const mqbi::Dispatcher::VoidFunctor& doneCallback)
        BSLS_KEYWORD_OVERRIDE;

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

    // MANIPULATORS
    //   (specific to mqbmock::Dispatcher)

    /// Set the value returned by this dispatcher when calling
    /// `inDispatcherThread()` and return a reference offering modifiable
    /// access to this object.
    Dispatcher& _setInDispatcherThread(bool value);

    // ACCESSORS
    //   (virtual: mqbi::Dispatcher)

    /// Return the number of processors dedicated for dispatching clients of
    /// the specified `type`.
    int numProcessors(mqbi::DispatcherClientType::Enum type) const
        BSLS_KEYWORD_OVERRIDE;

    /// Return whether the current thread is the dispatcher thread
    /// associated to the specified `client`.  This is useful for
    /// preconditions assert validation.
    bool inDispatcherThread(const mqbi::DispatcherClient* client) const
        BSLS_KEYWORD_OVERRIDE;

    /// Return whether the current thread is the dispatcher thread
    /// associated to the specified dispatcher client `data`.  This is
    /// useful for preconditions assert validation.
    bool inDispatcherThread(const mqbi::DispatcherClientData* data) const
        BSLS_KEYWORD_OVERRIDE;

    /// Not implemented.
    bmqex::Executor
    executor(const mqbi::DispatcherClient* client) const BSLS_KEYWORD_OVERRIDE;

    /// Not implemented.
    bmqex::Executor clientExecutor(const mqbi::DispatcherClient* client) const
        BSLS_KEYWORD_OVERRIDE;

    class InnerEventGuard;
    friend class InnerEventGuard;
    // A guard class that releases the even association when it goes out of
    // scope.

    /// A shared pointer that defines the lifecycle of an `InnerEventGuard`.
    typedef bsl::shared_ptr<InnerEventGuard> EventGuard;

    /// Associates a specified `event` with a specified `client` while the
    /// returned `EventGuard` doesn't go out of scope.
    EventGuard _withEvent(const mqbi::DispatcherClient* client,
                          mqbi::DispatcherEvent*        event);
};

// ======================
// class DispatcherClient
// ======================

/// Interface for a client of the Dispatcher.
class DispatcherClient : public mqbi::DispatcherClient {
    // DATA
    mqbi::DispatcherClientData d_dispatcherClientData;
    bsl::string                d_description;

    // NOT IMPLEMENTED
    DispatcherClient(const DispatcherClient&);
    DispatcherClient& operator=(const DispatcherClient&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(DispatcherClient, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `MockDispatcherClient` object.
    explicit DispatcherClient(bslma::Allocator* allocator);

    // MANIPULATORS

    /// Return a pointer to the dispatcher this client is associated with.
    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering modifiable access to the
    /// DispatcherClientData of this client.
    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE;

    /// Called by the `Dispatcher` when it has the specified `event` to
    /// deliver to the client.
    void onDispatcherEvent(const mqbi::DispatcherEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Called by the dispatcher to flush any pending operation; mainly used
    /// to provide batch and nagling mechanism.
    void flush() BSLS_KEYWORD_OVERRIDE;

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    DispatcherClient& _setDescription(const bslstl::StringRef& value);

    // ACCESSORS

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference not offering modifiable access to the
    /// DispatcherClientData of this client.
    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;

    /// Return a printable description of the client (e.g., for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
