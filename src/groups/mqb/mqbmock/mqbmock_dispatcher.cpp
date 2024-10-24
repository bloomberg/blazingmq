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
#include <bslim_printer.h>
#include <bsls_annotation.h>

namespace BloombergLP {
namespace mqbmock {

// ----------------
// class Dispatcher
// ----------------

// CREATORS
Dispatcher::Dispatcher(bslma::Allocator* allocator)
: d_inDispatcherThread(false)
, d_eventsForClients(allocator)
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
    BSLS_ANNOTATION_UNUSED mqbi::Dispatcher::ProcessorHandle handle)
{
    client->dispatcherClientData().setDispatcher(this).setClientType(type);

    return Dispatcher::k_INVALID_PROCESSOR_HANDLE;
}

void Dispatcher::unregisterClient(
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherClient* client)
{
    // NOTHING
}

mqbi::DispatcherEvent*
Dispatcher::getEvent(const mqbi::DispatcherClient* client)
{
    EventMap::iterator iter = d_eventsForClients.find(client);
    BSLS_ASSERT_SAFE(iter != d_eventsForClients.end());
    return iter->second;
}

mqbi::DispatcherEvent* Dispatcher::getEvent(
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherClientType::Enum type)
{
    return 0;
}

void Dispatcher::dispatchEvent(mqbi::DispatcherEvent*  event,
                               mqbi::DispatcherClient* destination)
{
    destination->onDispatcherEvent(*event);
}

void Dispatcher::dispatchEvent(
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherEvent* event,
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherClientType::Enum type,
    BSLS_ANNOTATION_UNUSED mqbi::Dispatcher::ProcessorHandle handle)
{
    // NOTHING
}

void Dispatcher::execute(
    const mqbi::Dispatcher::VoidFunctor& functor,
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherClient* client,
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherEventType::Enum type)
{
    functor();
}

void Dispatcher::execute(
    const mqbi::Dispatcher::VoidFunctor& functor,
    BSLS_ANNOTATION_UNUSED const mqbi::DispatcherClientData& client)
{
    functor();
}

void Dispatcher::execute(
    const mqbi::Dispatcher::ProcessorFunctor& functor,
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherClientType::Enum type,
    const mqbi::Dispatcher::VoidFunctor&                    doneCallback)
{
    if (functor) {
        const ProcessorHandle dummy = Dispatcher::k_INVALID_PROCESSOR_HANDLE;
        functor(dummy);
    }

    if (doneCallback) {
        doneCallback();
    }
}

void Dispatcher::synchronize(
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherClient* client)
{
    // NOTHING
}

void Dispatcher::synchronize(
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherClientType::Enum type,
    BSLS_ANNOTATION_UNUSED mqbi::Dispatcher::ProcessorHandle handle)
{
    // NOTHING
}

// MANIPULATORS
//   (specific to mqbmock::Dispatcher)
Dispatcher& Dispatcher::_setInDispatcherThread(bool value)
{
    d_inDispatcherThread = value;
    return *this;
}

// ACCESSORS
//   (virtual: mqbi::Dispatcher)
int Dispatcher::numProcessors(
    BSLS_ANNOTATION_UNUSED mqbi::DispatcherClientType::Enum type) const
{
    return 1;  // placeholder value for number of processors
}

bool Dispatcher::inDispatcherThread(
    BSLS_ANNOTATION_UNUSED const mqbi::DispatcherClient* client) const
{
    return d_inDispatcherThread;
}

bool Dispatcher::inDispatcherThread(
    BSLS_ANNOTATION_UNUSED const mqbi::DispatcherClientData* data) const
{
    return d_inDispatcherThread;
}

bmqex::Executor Dispatcher::executor(
    BSLS_ANNOTATION_UNUSED const mqbi::DispatcherClient* client) const
{
    BSLS_ASSERT(false && "Not yet implemented");
    return bmqex::Executor();
}

bmqex::Executor Dispatcher::clientExecutor(
    BSLS_ANNOTATION_UNUSED const mqbi::DispatcherClient* client) const
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
    InnerEventGuard(Dispatcher*                   dispatcher,
                    const mqbi::DispatcherClient* client,
                    mqbi::DispatcherEvent*        event)
    : d_dispatcher(dispatcher)
    , d_client(client)
    {
        d_dispatcher->d_eventsForClients[client] = event;
    }

    /// Destructor of this object.
    ~InnerEventGuard() { d_dispatcher->d_eventsForClients.erase(d_client); }
};

Dispatcher::EventGuard
Dispatcher::_withEvent(const mqbi::DispatcherClient* client,
                       mqbi::DispatcherEvent*        event)
{
    EventGuard eventGuard;
    eventGuard.createInplace(d_allocator_p, this, client, event);
    return eventGuard;
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
        event.getAs<mqbi::DispatcherCallbackEvent>().callback()(0);
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
