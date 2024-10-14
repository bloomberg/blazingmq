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

// bmqio_testchannelfactory.h                                         -*-C++-*-
#ifndef INCLUDED_BMQIO_TESTCHANNELFACTORY
#define INCLUDED_BMQIO_TESTCHANNELFACTORY

//@PURPOSE: Provide a test imp of the 'ChannelFactory' protocol
//
//@CLASSES:
// bmqio::TestChannelFactory
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a mechanism, 'TestChannelFactory',
// which is an implementation of the 'ChannelFactory' protocol for use in test
// drivers.  It stores the calls made to it, and allows the user to inspect
// them.

#include <bmqio_channelfactory.h>
#include <bmqio_connectoptions.h>
#include <bmqio_listenoptions.h>

// BDE
#include <bsl_deque.h>

namespace BloombergLP {
namespace bmqio {

class TestChannelFactory;

// ================================
// class TestChannelFactoryOpHandle
// ================================

/// The OperationHandle returned by a `TestChannelFactory`.
class TestChannelFactoryOpHandle : public ChannelFactoryOperationHandle {
  private:
    // DATA
    TestChannelFactory*          d_factory_p;
    bsl::shared_ptr<bsl::string> d_cancelToken;
    bmqvt::PropertyBag           d_properties;

    // NOT IMPLEMENTED
    TestChannelFactoryOpHandle(const TestChannelFactoryOpHandle&);
    TestChannelFactoryOpHandle& operator=(const TestChannelFactoryOpHandle&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TestChannelFactoryOpHandle,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit TestChannelFactoryOpHandle(TestChannelFactory* factory,
                                        bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS

    /// Return a reference providing modifiable access to this object's
    /// `cancel` token.  This is the token that will be included in the
    /// `HandleCancelCall` objects created as a result of calls to `cancel`
    /// on this object in its owning `TestChannelFactory`.
    bsl::shared_ptr<bsl::string>& token();

    // ChannelFactoryOperationHandle
    void                cancel() BSLS_KEYWORD_OVERRIDE;
    bmqvt::PropertyBag& properties() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    // ChannelFactoryOperationHandle
    const bmqvt::PropertyBag& properties() const BSLS_KEYWORD_OVERRIDE;
};

// ========================
// class TestChannelFactory
// ========================

/// Test implementation of the `ChannelFactory` protocol.
class TestChannelFactory : public ChannelFactory {
  public:
    // TYPES
    typedef TestChannelFactoryOpHandle TestOpHandle;

    struct ListenCall {
        // DATA
        bsl::shared_ptr<TestOpHandle> d_handle;
        ListenOptions                 d_options;
        ResultCallback                d_cb;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(ListenCall, bslma::UsesBslmaAllocator)

        // CREATORS
        ListenCall(const bsl::shared_ptr<TestOpHandle>& handle,
                   const ListenOptions&                 options,
                   const ResultCallback&                cb,
                   bslma::Allocator*                    basicAllocator = 0)
        : d_handle(handle)
        , d_options(options, basicAllocator)
        , d_cb(bsl::allocator_arg_t(), basicAllocator, cb)
        {
            // NOTHING
        }
    };

    struct ConnectCall {
        // DATA
        bsl::shared_ptr<TestOpHandle> d_handle;
        ConnectOptions                d_options;
        ResultCallback                d_cb;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(ConnectCall, bslma::UsesBslmaAllocator)

        // CREATORS
        ConnectCall(const bsl::shared_ptr<TestOpHandle>& handle,
                    const ConnectOptions&                options,
                    const ResultCallback&                cb,
                    bslma::Allocator*                    basicAllocator = 0)
        : d_handle(handle)
        , d_options(options, basicAllocator)
        , d_cb(bsl::allocator_arg_t(), basicAllocator, cb)
        {
            // NOTHING
        }
    };

    struct HandleCancelCall {
        // DATA
        bsl::shared_ptr<bsl::string> d_token;

        // CREATORS
        HandleCancelCall(const bsl::shared_ptr<bsl::string>& token)
        : d_token(token)
        {
            // NOTHING
        }
    };

  private:
    // DATA
    Status                       d_listenStatus;
    Status                       d_connectStatus;
    bmqvt::PropertyBag           d_newHandleProperties;
    bsl::deque<ListenCall>       d_listenCalls;
    bsl::deque<ConnectCall>      d_connectCalls;
    bsl::deque<HandleCancelCall> d_handleCancelCalls;
    bslma::Allocator*            d_allocator_p;

    // FRIENDS
    friend class TestChannelFactoryOpHandle;

    // NOT IMPLEMENTED
    TestChannelFactory(const TestChannelFactory&);
    TestChannelFactory& operator=(const TestChannelFactory&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TestChannelFactory,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit TestChannelFactory(bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS

    /// Reset this object to its default-constructed state.
    void reset();

    /// Set the status returned by calls to `listen`.
    void setListenStatus(const Status& status);

    /// Set the status returned by calls to `connect`.
    void setConnectStatus(const Status& status);

    /// Return a reference providing modifiable access to the new Handle
    /// properties of this object.  These are the properties that will be
    /// set on new `OpHandles` produced by this object.
    bmqvt::PropertyBag& newHandleProperties();

    bsl::deque<ListenCall>&       listenCalls();
    bsl::deque<ConnectCall>&      connectCalls();
    bsl::deque<HandleCancelCall>& handleCancelCalls();

    // ChannelFactory
    void listen(Status*                      status,
                bslma::ManagedPtr<OpHandle>* handle,
                const ListenOptions&         options,
                const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;
    void connect(Status*                      status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const ConnectOptions&        options,
                 const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
