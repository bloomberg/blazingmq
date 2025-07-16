// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqio_testchannelfactory.cpp                                       -*-C++-*-
#include <bmqio_testchannelfactory.h>

#include <bmqscm_version.h>
#include <bsl_memory.h>
namespace BloombergLP {
namespace bmqio {

// --------------------------------
// class TestChannelFactoryOpHandle
// --------------------------------

// CREATORS
TestChannelFactoryOpHandle::TestChannelFactoryOpHandle(
    TestChannelFactory* factory,
    bslma::Allocator*   basicAllocator)
: d_factory_p(factory)
, d_cancelToken()
, d_properties(basicAllocator)
{
    d_cancelToken.createInplace(basicAllocator, basicAllocator);
}

bsl::shared_ptr<bsl::string>& TestChannelFactoryOpHandle::token()
{
    return d_cancelToken;
}

void TestChannelFactoryOpHandle::cancel()
{
    d_factory_p->d_handleCancelCalls.emplace_back(d_cancelToken);
}

bmqvt::PropertyBag& TestChannelFactoryOpHandle::properties()
{
    return d_properties;
}

const bmqvt::PropertyBag& TestChannelFactoryOpHandle::properties() const
{
    return d_properties;
}

// ------------------------
// class TestChannelFactory
// ------------------------

TestChannelFactory::TestChannelFactory(bslma::Allocator* basicAllocator)
: d_listenStatus(basicAllocator)
, d_connectStatus(basicAllocator)
, d_newHandleProperties(basicAllocator)
, d_listenCalls(basicAllocator)
, d_connectCalls(basicAllocator)
, d_handleCancelCalls(basicAllocator)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

void TestChannelFactory::reset()
{
    d_listenStatus.reset();
    d_connectStatus.reset();
    d_listenCalls.clear();
    d_connectCalls.clear();
    d_handleCancelCalls.clear();
}

void TestChannelFactory::setListenStatus(const Status& status)
{
    d_listenStatus = status;
}

void TestChannelFactory::setConnectStatus(const Status& status)
{
    d_connectStatus = status;
}

bmqvt::PropertyBag& TestChannelFactory::newHandleProperties()
{
    return d_newHandleProperties;
}

bsl::deque<TestChannelFactory::ListenCall>& TestChannelFactory::listenCalls()
{
    return d_listenCalls;
}

bsl::deque<TestChannelFactory::ConnectCall>& TestChannelFactory::connectCalls()
{
    return d_connectCalls;
}

bsl::deque<TestChannelFactory::HandleCancelCall>&
TestChannelFactory::handleCancelCalls()
{
    return d_handleCancelCalls;
}

void TestChannelFactory::listen(Status*                      status,
                                bslma::ManagedPtr<OpHandle>* handle,
                                const ListenOptions&         options,
                                const ResultCallback&        cb)
{
    bsl::shared_ptr<TestOpHandle> opHandle;
    if (handle) {
        opHandle.createInplace(d_allocator_p, this, d_allocator_p);
        opHandle->properties() = d_newHandleProperties;
    }

    d_listenCalls.emplace_back(opHandle, options, cb);

    if (status) {
        *status = d_listenStatus;
    }
}

void TestChannelFactory::connect(Status*                      status,
                                 bslma::ManagedPtr<OpHandle>* handle,
                                 const ConnectOptions&        options,
                                 const ResultCallback&        cb)
{
    bsl::shared_ptr<TestOpHandle> opHandle;
    if (handle) {
        opHandle.createInplace(d_allocator_p, this, d_allocator_p);
        opHandle->properties() = d_newHandleProperties;
    }

    d_connectCalls.emplace_back(opHandle, options, cb);

    if (status) {
        *status = d_connectStatus;
    }
}

}  // close package namespace
}  // close enterprise namespace
