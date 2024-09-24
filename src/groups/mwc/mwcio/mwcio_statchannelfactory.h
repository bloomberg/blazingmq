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

// mwcio_statchannelfactory.h                                         -*-C++-*-
#ifndef INCLUDED_MWCIO_STATCHANNELFACTORY
#define INCLUDED_MWCIO_STATCHANNELFACTORY

//@PURPOSE: Provide a ChannelFactory decorator for channels collecting stats
//
//@CLASSES:
// mwcio::StatChannelFactory
// mwcio::StatChannelFactoryConfig
// mwcio::StatChannelFactoryHandle
// mwcio::StatChannelFactoryUtil
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a mechanism,
// 'mwcio::StatChannelFactory', which is a decorator of a base
// 'mwcio::ChannelFactory' that gives it the ability to create channels that
// collect stats.  A stat context is associated with every channel.  If the
// channel was obtained through a call to a reconnecting connect, its
// associated stat context is preserved on connection lost and restored upon
// reconnection.

// MWC

#include <mwcio_channel.h>
#include <mwcio_channelfactory.h>
#include <mwcio_connectoptions.h>
#include <mwcio_listenoptions.h>
#include <mwcst_basictableinfoprovider.h>
#include <mwcst_statcontext.h>
#include <mwcst_table.h>
#include <mwcst_tableutil.h>

// BDE
#include <bdlb_variant.h>
#include <bsl_functional.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>

namespace BloombergLP {
namespace mwcio {

// FORWARD DECLARATIONS
class StatChannelFactory;
struct StatChannelFactoryHandle;

// ========================
// class StatChannelFactory
// ========================

/// Configuration for a `StatChannelFactory`
class StatChannelFactoryConfig {
  public:
    // PUBLIC TYPES

    /// Signature of the callback to create and return the statContext to be
    /// used for tracking stats of the specified `channel` obtained from the
    /// specified `handle`.
    typedef bsl::function<bslma::ManagedPtr<mwcst::StatContext>(
        const bsl::shared_ptr<Channel>&                  channel,
        const bsl::shared_ptr<StatChannelFactoryHandle>& handle)>
        StatContextCreatorFn;

  private:
    // DATA
    ChannelFactory* d_baseFactory_p;
    // underlying ChannelFactory

    StatContextCreatorFn d_statContextCreator;

    bslma::Allocator* d_allocator_p;

    // FRIENDS
    friend class StatChannelFactory;

    // NOT IMPLEMENTED
    StatChannelFactoryConfig& operator=(const StatChannelFactoryConfig&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatChannelFactoryConfig,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `StatChannelFactoryConfig` using the specified `base` and
    /// `statContextCreator` and the optionally specified `basicAllocator`.
    explicit StatChannelFactoryConfig(
        ChannelFactory*             base,
        const StatContextCreatorFn& statContextCreator,
        bslma::Allocator*           basicAllocator = 0);

    StatChannelFactoryConfig(const StatChannelFactoryConfig& original,
                             bslma::Allocator* basicAllocator = 0);
};

// ===============================
// struct StatChannelFactoryHandle
// ===============================

/// Handle to an outstanding `connect` or `listen`
struct StatChannelFactoryHandle : public ChannelFactory::OpHandle {
  public:
    // PUBLIC TYPES
    typedef bdlb::Variant2<ConnectOptions, ListenOptions> OptionsVariant;

  private:
    // PUBLIC DATA
    StatChannelFactory*                         d_factory_p;
    ChannelFactory::ResultCallback              d_resultCallback;
    OptionsVariant                              d_options;
    bslma::ManagedPtr<ChannelFactory::OpHandle> d_baseConnectHandle;
    bslma::Allocator*                           d_allocator_p;

    // FRIENDS
    friend class StatChannelFactory;

    // NOT IMPLEMENTED
    StatChannelFactoryHandle(const StatChannelFactoryHandle&)
        BSLS_CPP11_DELETED;
    StatChannelFactoryHandle&
    operator=(const StatChannelFactoryHandle&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatChannelFactoryHandle,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit StatChannelFactoryHandle(bslma::Allocator* basicAllocator = 0);
    ~StatChannelFactoryHandle() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Cancel any current and future connect attempts (but do not close any
    /// existing Channel).
    void cancel() BSLS_KEYWORD_OVERRIDE;

    mwct::PropertyBag& properties() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    const mwct::PropertyBag& properties() const BSLS_KEYWORD_OVERRIDE;

    const OptionsVariant& options() const;
};

// ========================
// class StatChannelFactory
// ========================

/// A ChannelFactory decorator that creates channels that collect
/// statistics.
class StatChannelFactory : public ChannelFactory {
  public:
    // TYPES
    typedef StatChannelFactoryConfig       Config;
    typedef ChannelFactory::OpHandle       OpHandle;
    typedef ChannelFactory::ResultCallback ResultCallback;

  private:
    // PRIVATE TYPES
    typedef StatChannelFactoryHandle Handle;
    typedef bsl::shared_ptr<Handle>  HandleSp;

  private:
    // DATA
    Config d_config;

    // PRIVATE MANIPULATORS

    /// Handle an event from our base ChannelFactory.
    void baseResultCallback(const HandleSp&                 handleSp,
                            ChannelFactoryEvent::Enum       event,
                            const Status&                   status,
                            const bsl::shared_ptr<Channel>& channel);

    // NOT IMPLEMENTED
    StatChannelFactory(const StatChannelFactory&);
    StatChannelFactory& operator=(const StatChannelFactory&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatChannelFactory,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit StatChannelFactory(const Config&     config,
                                bslma::Allocator* basicAllocator = 0);
    ~StatChannelFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    // ChannelFactory

    /// Forward to the base `ChannelFactory`s listen.
    void listen(Status*                      status,
                bslma::ManagedPtr<OpHandle>* handle,
                const ListenOptions&         options,
                const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;

    /// Connect (using the base `ChannelFactory`) to one of the endpoints
    /// produced by our Config's `endpointEvalFn`, and if that connection
    /// fails/goes down, continuously attempt to connect to another
    /// endpoint.  Note that if this call fails, it only indicates that the
    /// first connect attempt failed. The `StatChannelFactory` will
    /// continue trying to establish the connection.
    void connect(Status*                      status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const ConnectOptions&        options,
                 const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE;
};

// =============================
// struct StatChannelFactoryUtil
// =============================

/// Utilities useful when working with a StatChannelFactory.
struct StatChannelFactoryUtil {
  public:
    // PUBLIC TYPES

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum {
            e_BYTES_IN_DELTA,
            e_BYTES_IN_ABS,
            e_BYTES_OUT_DELTA,
            e_BYTES_OUT_ABS,
            e_CONNECTIONS_DELTA,
            e_CONNECTIONS_ABS
        };
    };

  public:
    // CLASS METHODS

    /// Return the stat context configuration to use for the stats managed
    /// by the factory.
    static mwcst::StatContextConfiguration
    statContextConfiguration(const bsl::string& name,
                             int                historySize = -1,
                             bslma::Allocator*  allocator   = 0);

    /// Create and return a root "channels" stat context with the specified
    /// `name` and `historySize` using the specified `allocator`.
    static bslma::ManagedPtr<mwcst::StatContext>
    createStatContext(const bsl::string& name,
                      int                historySize,
                      bslma::Allocator*  allocator = 0);

    /// Configure the specified `table` and `tip` for printing the channels
    /// statistics from the specified `rootStatContext.  If `end' does not
    /// representent the default snapshot location, then delta statistics
    /// between the specified `stat` and `end` are printed, otherwise only
    /// the current ones are.
    static void
    initializeStatsTable(mwcst::Table*                  table,
                         mwcst::BasicTableInfoProvider* tip,
                         mwcst::StatContext*            rootStatContext,
                         const mwcst::StatValue::SnapshotLocation& start =
                             mwcst::StatValue::SnapshotLocation(),
                         const mwcst::StatValue::SnapshotLocation& end =
                             mwcst::StatValue::SnapshotLocation());

    /// Get the value of the specified `stat` reported to the channel
    /// represented by its associated specified `context` as the
    /// difference between the latest snapshot-ed value (i.e., 'snapshotId
    /// == 0') and the value that was recorded at the specified `snapshotId`
    /// snapshots ago.
    ///
    /// THREAD: This method can only be invoked from the `snapshot` thread.
    static bsls::Types::Int64 getValue(const mwcst::StatContext& context,
                                       int                       snapshotId,
                                       const Stat::Enum&         stat);
};

}  // close package namespace
}  // close enterprise namespace

#endif
