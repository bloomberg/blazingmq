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

// mqbmock_dispatcher.cpp                                             -*-C++-*-
#include <mqbmock_dispatcher.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bsl_ostream.h>
#include <bsla_annotations.h>
#include <bslim_printer.h>
#include <bslmt_lockguard.h>

namespace BloombergLP {
namespace mqbmock {

// ----------------
// class Dispatcher
// ----------------

// CREATORS
Dispatcher::Dispatcher(bslma::Allocator* allocator)
: d_eventsForClients(allocator)
, d_mutex()
, d_queue(allocator)
, d_allocator_p(allocator)

{
    // NOTHING
}

Dispatcher::~Dispatcher()
{
    // NOTHING
}

// MANIPULATORS
//   (virtual: mqbi::Dispatcher)
Dispatcher::ProcessorHandle Dispatcher::registerClient(
    mqbi::DispatcherClient*          client,
    mqbi::DispatcherClientType::Enum type,
    BSLA_UNUSED mqbi::Dispatcher::ProcessorHandle handle)
{
    client->dispatcherClientData()
        .setDispatcher(this)
        .setClientType(type)
        .setThreadId(bslmt::ThreadUtil::selfId());

    return Dispatcher::k_INVALID_PROCESSOR_HANDLE;
}

void Dispatcher::unregisterClient(BSLA_UNUSED mqbi::DispatcherClient* client)
{
    // NOTHING
}

mqbi::Dispatcher::DispatcherEventSp
Dispatcher::getEvent(const mqbi::DispatcherClient* client)
{
    EventMap::iterator iter = d_eventsForClients.find(client);
    BSLS_ASSERT_SAFE(iter != d_eventsForClients.end());
    return iter->second;
}

mqbi::Dispatcher::DispatcherEventSp
Dispatcher::getEvent(BSLA_UNUSED mqbi::DispatcherClientType::Enum type)
{
    return mqbi::Dispatcher::DispatcherEventSp();
}

void Dispatcher::dispatchEvent(mqbi::Dispatcher::DispatcherEventRvRef event,
                               mqbi::DispatcherClient* destination)
{
    destination->onDispatcherEvent(*bslmf::MovableRefUtil::access(event));
}

void Dispatcher::dispatchEvent(
    BSLA_UNUSED mqbi::Dispatcher::DispatcherEventRvRef event,
    BSLA_UNUSED mqbi::DispatcherClientType::Enum type,
    BSLA_UNUSED mqbi::Dispatcher::ProcessorHandle handle)
{
    // NOTHING
}

void Dispatcher::execute(const mqbi::Dispatcher::VoidFunctor& functor,
                         BSLA_UNUSED mqbi::DispatcherClient* client,
                         BSLA_UNUSED mqbi::DispatcherEventType::Enum type)
{
    _execute(functor);
}

void Dispatcher::execute(const mqbi::Dispatcher::VoidFunctor& functor,
                         BSLA_UNUSED const mqbi::DispatcherClientData& client)
{
    _execute(functor);
}

void Dispatcher::executeOnAllQueues(
    const mqbi::Dispatcher::VoidFunctor& functor,
    BSLA_UNUSED mqbi::DispatcherClientType::Enum type,
    const mqbi::Dispatcher::VoidFunctor&         doneCallback)
{
    if (functor) {
        _execute(functor);
    }

    if (doneCallback) {
        _execute(doneCallback);
    }
}

void Dispatcher::_execute(const mqbi::Dispatcher::VoidFunctor& functor)
{
    bool run = false;

    {
        bslmt::LockGuard<bslmt::Mutex> lock(&mutex());

        if (d_queue.empty()) {
            // The thread that pushes the first functor to the queue
            // will try to process this queue.
            run = true;
        }
        d_queue.push(functor);
    }

    while (run) {
        mqbi::Dispatcher::VoidFunctor next;
        {
            bslmt::LockGuard<bslmt::Mutex> lock(&mutex());

            next = d_queue.front();

            d_queue.pop();
            if (d_queue.empty()) {
                // There is nothing more to process, this thread can stop
                // after calling the last functor.
                run = false;
            }
        }
        next();
    }
}

void Dispatcher::synchronize(BSLA_UNUSED mqbi::DispatcherClient* client)
{
    // NOTHING
}

void Dispatcher::synchronize(
    BSLA_UNUSED mqbi::DispatcherClientType::Enum type,
    BSLA_UNUSED mqbi::Dispatcher::ProcessorHandle handle)
{
    // NOTHING
}

// ACCESSORS
//   (virtual: mqbi::Dispatcher)
int Dispatcher::numProcessors(
    BSLA_UNUSED mqbi::DispatcherClientType::Enum type) const
{
    return 1;  // placeholder value for number of processors
}

bmqex::Executor
Dispatcher::executor(BSLA_UNUSED const mqbi::DispatcherClient* client) const
{
    BSLS_ASSERT(false && "Not yet implemented");
    return bmqex::Executor();
}

// ---------------------------------
// class Dispatcher::InnerEventGuard
// ---------------------------------

/// A guard that ensures the event is removed when it goes out of scope.
class Dispatcher::InnerEventGuard {
  private:
    // DATA
    Dispatcher* d_dispatcher;
    // The dispatcher this points to.
    const mqbi::DispatcherClient* d_client;
    // The client to disassociate.

  public:
    // CREATORS

    /// Create an `InnerEventGuard` object by using the specified
    /// `dispatcher`, `client` and `event`.
    InnerEventGuard(Dispatcher*                         dispatcher,
                    const mqbi::DispatcherClient*       client,
                    mqbi::Dispatcher::DispatcherEventSp event)
    : d_dispatcher(dispatcher)
    , d_client(client)
    {
        d_dispatcher->d_eventsForClients[client] = event;
    }

    /// Destructor of this object.
    ~InnerEventGuard() { d_dispatcher->d_eventsForClients.erase(d_client); }
};

Dispatcher::EventGuard
Dispatcher::_withEvent(const mqbi::DispatcherClient*       client,
                       mqbi::Dispatcher::DispatcherEventSp event)
{
    EventGuard eventGuard;
    eventGuard.createInplace(d_allocator_p, this, client, event);
    return eventGuard;
}

bslmt::Mutex& Dispatcher::mutex()
{
    return d_mutex;
}

// ----------------------
// class DispatcherClient
// ----------------------

// CREATORS
DispatcherClient::DispatcherClient(bslma::Allocator* allocator)
: d_dispatcherClientData()
, d_description(allocator)
{
    // NOTHING
}

// MANIPULATORS
mqbi::Dispatcher* DispatcherClient::dispatcher()
{
    return d_dispatcherClientData.dispatcher();
}

mqbi::DispatcherClientData& DispatcherClient::dispatcherClientData()
{
    return d_dispatcherClientData;
}

void DispatcherClient::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    if (event.type() == mqbi::DispatcherEventType::e_CALLBACK) {
        BSLS_ASSERT_SAFE(!event.asCallbackEvent()->callback().empty());
        event.asCallbackEvent()->callback()();
    }
}

void DispatcherClient::flush()
{
}

DispatcherClient&
DispatcherClient::_setDescription(const bslstl::StringRef& value)
{
    d_description = value;
    return *this;
}

// ACCESSORS
const mqbi::Dispatcher* DispatcherClient::dispatcher() const
{
    return d_dispatcherClientData.dispatcher();
}

const mqbi::DispatcherClientData&
DispatcherClient::dispatcherClientData() const
{
    return d_dispatcherClientData;
}

const bsl::string& DispatcherClient::description() const
{
    return d_description;
}

}  // close package namespace
}  // close enterprise namespace
