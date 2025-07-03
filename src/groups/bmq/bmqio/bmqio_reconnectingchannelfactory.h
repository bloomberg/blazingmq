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

// bmqio_reconnectingchannelfactory.h                                 -*-C++-*-
#ifndef INCLUDED_BMQIO_RECONNECTINGCHANNELFACTORY
#define INCLUDED_BMQIO_RECONNECTINGCHANNELFACTORY

//@PURPOSE: Provide an auto-reconnecting ChannelFactory decorator
//
//@CLASSES:
// bmqio::ReconnectingChannelFactory
// bmqio::ReconnectingChannelFactoryConfig
// bmqio::ReconnectingChannelFactoryUtil
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a mechanism,
// 'bmqio::ReconnectingChannelFactory', which is a decorator of a base
// 'bmqio::ChannelFactory' that gives it the ability to automatically retry a
// connection if it goes down, using a pool of many available endpoints which
// have been resolved.
//
// When the endoint resolves to multiple hosts, the factory considers them all
// as 'one unit', meaning it will try to connect to each of them in a row
// (i.e., without any in-between sleep), and emit a single
// 'CONNECT_ATTEMPT_FAILED' event after failing to connect to all of them and
// decrementing the maxConnectAttempt by one.

#include <bmqio_channel.h>
#include <bmqio_channelfactory.h>
#include <bmqio_connectoptions.h>
#include <bmqu_atomicvalidator.h>

// BDE
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqio {

// FORWARD DECLARATIONS
class ReconnectingChannelFactory;
struct ReconnectingChannelFactory_ConnectHandle;

// ======================================
// class ReconnectingChannelFactoryConfig
// ======================================

/// Configuration for a `ReconnectingChannelFactory`
class ReconnectingChannelFactoryConfig {
  public:
    // TYPES
    typedef bsl::function<void(bsl::vector<bsl::string>* out,
                               const ConnectOptions&     options)>
        EndpointResolveFn;
    // Signature of the endpoint resolver method.  Load into the specified
    // 'out' the (sorted) lists of endpoints to try to connect to, from the
    // endpoint in the specified 'options'.  The factory will try to connect
    // to the endpoints from the 'out' list, one-by-one in order until a
    // successfull connection happens.  If all attempts fail, the factory
    // will sleep for some time (as computed by the 'ReconnectIntervalFn' and
    // will then re-resolve the endpoints and retry connecting.

    typedef bsl::function<void(bsls::TimeInterval*       interval,
                               const ConnectOptions&     options,
                               const bsls::TimeInterval& timeSinceLastAttempt)>
        ReconnectIntervalFn;
    // Signature of the method used to compute how long to wait before trying
    // to connect out.  The specified 'interval' is populated with the
    // 'interval' used for the last attempt (or 0 if that is the first time),
    // and the method should load into 'interval' the new time to use.  The
    // specified 'options' correspond to the options passed to the
    // corresponding call to 'connect', and the specified
    // 'timeSinceLastAttempt' contains the amount of time that has elapsed
    // since the last connection attempt (or set to Int64 max nanoseconds if
    // no connection ever succeeded yet; this value can be used to detect if
    // long enough time has elapsed since the last successfull connection,
    // and therefore the 'interval' could be reset.

  private:
    // DATA
    ChannelFactory* d_base_p;  // underlying ChannelFactory

    bdlmt::EventScheduler* d_scheduler_p;
    // injected scheduler to use for
    // scheduling reconnections

    ReconnectIntervalFn d_reconnectIntervalFn;
    // function used to determine the
    // interval to use for the next
    // connection attempt for a connection

    EndpointResolveFn d_endpointResolveFn;
    // function to use to translate the
    // 'endpoint' of the connect to an actual
    // list of hosts to pass to the
    // underlying base 'ChannelFactory's
    // 'connect'

    // FRIENDS
    friend class ReconnectingChannelFactory;
    friend struct ReconnectingChannelFactory_ConnectHandle;

    // NOT IMPLEMENTED
    ReconnectingChannelFactoryConfig&
    operator=(const ReconnectingChannelFactoryConfig&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ReconnectingChannelFactoryConfig,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit ReconnectingChannelFactoryConfig(
        ChannelFactory*        base,
        bdlmt::EventScheduler* scheduler,
        bslma::Allocator*      basicAllocator = 0);
    ReconnectingChannelFactoryConfig(
        const ReconnectingChannelFactoryConfig& original,
        bslma::Allocator*                       basicAllocator = 0);

    // MANIPULATORS

    /// Set the function to be invoked when the interval for a reconnect
    /// attempt must be determined to the specified `value` and return a
    /// reference providing modifiable access to this object.  Refer to the
    /// `ReconnectIntervalFn` documentation for this methods contract.  By
    /// default, this will be
    /// ```
    /// ReconnectingChannelFactoryUtil::defaultConnectIntervalFn(
    ///    <interval>,
    ///    <options>,
    ///    <timeSinceLastSuccessfullConnect>,
    ///    bsls::TimeInterval(120.0), // minimumUpTimeBeforeReset
    ///    bsls::TimeInterval(60.0)); // at most 1min interval
    /// ```
    ReconnectingChannelFactoryConfig&
    setReconnectIntervalFn(const ReconnectIntervalFn& value);

    /// Set the function to use to turn the `endpoint` provided to `connect`
    /// into a list of endpoint suitable to pass to the `base`
    /// `ChannelFactory`s `connect` to the specified `value` and return a
    /// reference providing modifiable access to this object.  By default,
    /// this is `ReconnectingChannelFactoryUtil::resolveEndpointFn`, using
    /// `;` as the separator.
    ReconnectingChannelFactoryConfig&
    setEndpointResolveFn(const EndpointResolveFn& value);
};

// ===============================================
// struct ReconnectingChannelFactory_ConnectHandle
// ===============================================

/// Handle to an outstanding `connect` operation.
struct ReconnectingChannelFactory_ConnectHandle
: public ChannelFactory::OpHandle {
  public:
    // PUBLIC DATA
    bmqu::AtomicValidator d_validator;
    // The validator is used to prevent calling the
    // 'd_resultCallback' if the user has canceled the
    // connect.  This is also used to prevent earlier
    // some operations in case the user has canceled
    // the connect.

    bsls::AtomicBool                            d_canceled;
    ReconnectingChannelFactory*                 d_factory_p;
    ChannelFactory::ResultCallback              d_resultCallback;
    ConnectOptions                              d_options;
    bsl::vector<bsl::string>                    d_endpoints;
    int                                         d_currentAttemptCounter;
    bslma::ManagedPtr<ChannelFactory::OpHandle> d_baseConnectHandle;
    bdlmt::SignalerConnection                   d_closeFnConnection;
    bdlmt::EventScheduler::EventHandle          d_reconnectHandle;
    bsls::Types::Int64                          d_lastConnectAttemptTime;
    bsls::TimeInterval                          d_lastConnectInterval;

    // NOT IMPLEMENTED
    ReconnectingChannelFactory_ConnectHandle(
        const ReconnectingChannelFactory_ConnectHandle&) BSLS_KEYWORD_DELETED;
    ReconnectingChannelFactory_ConnectHandle& operator=(
        const ReconnectingChannelFactory_ConnectHandle&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ReconnectingChannelFactory_ConnectHandle,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit ReconnectingChannelFactory_ConnectHandle(
        bslma::Allocator* basicAllocator = 0);
    ~ReconnectingChannelFactory_ConnectHandle() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Cancel any current and future connect attempts (but do not close any
    /// existing Channel).
    void cancel() BSLS_KEYWORD_OVERRIDE;

    bmqvt::PropertyBag& properties() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    const bmqvt::PropertyBag& properties() const BSLS_KEYWORD_OVERRIDE;
};

// ================================
// class ReconnectingChannelFactory
// ================================

/// A ChannelFactory decorator that will automatically try reconnecting if
/// a Channel produced by a `connect` goes down.
class ReconnectingChannelFactory : public ChannelFactory {
  public:
    // TYPES
    typedef ReconnectingChannelFactoryConfig Config;
    typedef ChannelFactory::OpHandle         OpHandle;
    typedef ChannelFactory::ResultCallback   ResultCallback;

  private:
    // PRIVATE TYPES
    typedef ReconnectingChannelFactory_ConnectHandle ConnectHandle;
    typedef bsl::shared_ptr<ConnectHandle>           ConnectHandlePtr;
    typedef bsl::unordered_map<ConnectHandle*, ConnectHandlePtr>
        ConnectHandleMap;

    // DATA
    bmqu::AtomicValidator d_validator;
    Config                d_config;
    ConnectHandleMap      d_handles;
    bslmt::Mutex          d_mutex;

    // PRIVATE MANIPULATORS

    /// Schedule a connect attempt for the specified `handle`.
    void scheduleConnect(const ConnectHandlePtr& handle);

    /// Attempt to connect to one of the endpoints of the connection
    /// defined by the specified `handle`, and load into the specified
    /// `status` the result of the attempt.
    void doConnect(Status* status, const ConnectHandlePtr& handle);

    /// Callback invoked by the EventScheduler to try connecting the
    /// specified `handle` after the connection retry time has expired.
    void connectTimerCb(const ConnectHandlePtr& handle);

    /// Handle the result of a `connect` attempt for the specified `handle`
    /// with our base ChannelFactory of the specified `event` type with the
    /// specified `status` and `channel`.
    void connectResultCb(const ConnectHandlePtr&         handle,
                         ChannelFactoryEvent::Enum       event,
                         const Status&                   status,
                         const bsl::shared_ptr<Channel>& channel);

    /// Callback invoked when a Channel produced by a `connect` goes down
    /// for the specified `handle`.  This will attempt to reconnect if the
    /// request hasn't been canceled.
    void onChannelDown(const ConnectHandlePtr& handle);

    /// Remove the specified `handle` from `d_handles`.
    void removeConnectHandle(ConnectHandle* handle);

    // FRIENDS
    friend struct ReconnectingChannelFactory_ConnectHandle;

    // NOT IMPLEMENTED
    ReconnectingChannelFactory(const ReconnectingChannelFactory&);
    ReconnectingChannelFactory& operator=(const ReconnectingChannelFactory&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ReconnectingChannelFactory,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit ReconnectingChannelFactory(const Config&     config,
                                        bslma::Allocator* basicAllocator = 0);
    ~ReconnectingChannelFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    int start();

    /// Cancel any pending reconnect attempts.
    void stop();

    // ChannelFactory

    /// Forward to the base `ChannelFactory`s listen.
    void listen(Status*                      status,
                bslma::ManagedPtr<OpHandle>* handle,
                const ListenOptions&         options,
                const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;

    /// Connect (using the base `ChannelFactory`) to one of the endpoints
    /// produced by our Config's `resolveEndpointFn`, and if that connection
    /// fails/goes down, continuously attempt to connect to another endpoint
    /// if `options.autoReconnect()` is `true`.  Note that if this call
    /// fails synchronously (possibly because the `connect` call to the
    /// underlying ChannelFactory' failed synchronously), then the
    /// connection will *not* be automatically retried, except for the case
    /// when the resolution returned 0 endpoints in which situation the
    /// factory will keep trying to resolve and connect.
    void connect(Status*                      status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const ConnectOptions&        options,
                 const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;
};

// =====================================
// struct ReconnectingChannelFactoryUtil
// =====================================

/// Utilities useful when working with a ReconnectingChannelFactory
struct ReconnectingChannelFactoryUtil {
    // CLASS METHODS

    /// Resolve the endpoint from the specified `options` formatted as
    /// defined below and load the resulting endpoints into the specified
    /// `out`.  The factory will then try to connect to each endpoint in the
    /// order.
    /// ```
    /// <host>:<port>[;<host>:<port>;...]
    /// ```
    /// Where each `host` will be resolved and expanded to all host it
    /// potentially maps to.
    static void resolveEndpointFn(bsl::vector<bsl::string>* out,
                                  const ConnectOptions&     options,
                                  char                      separator = ';');

    /// Update the reconnect interval in the specified `interval` with the
    /// amount of time to wait until the next connection attempt, taking
    /// into account the remaining arguments.
    /// * If `timeSinceLastAttempt >= resetReconnectTime`, resets
    ///   `interval` back to the options attempt interval time.
    /// * Otherwise, double `interval`, capping it to `maxInterval`, adding
    ///   some jitter randomness
    /// Note that this method uses `bsl::rand`, and therefore `bsl::srand`
    /// should be called to initialize the random generator.
    static void
    defaultConnectIntervalFn(bsls::TimeInterval*       interval,
                             const ConnectOptions&     options,
                             const bsls::TimeInterval& timeSinceLastAttempt,
                             const bsls::TimeInterval& resetReconnectTime,
                             const bsls::TimeInterval& maxInterval);
};

}  // close package namespace
}  // close enterprise namespace

#endif
