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

// bmqio_statchannelfactory.cpp                                       -*-C++-*-
#include <bmqio_statchannelfactory.h>

#include <bmqscm_version.h>

#include <bmqio_statchannel.h>
#include <bmqst_statutil.h>
#include <bmqst_statvalue.h>
#include <bmqst_tablerecords.h>
#include <bmqst_tableschema.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_utility.h>
#include <bslmt_lockguard.h>
#include <bsls_assert.h>
#include <bsls_timeutil.h>

namespace BloombergLP {
namespace bmqio {

// ------------------------------
// class StatChannelFactoryConfig
// ------------------------------

StatChannelFactoryConfig::StatChannelFactoryConfig(
    ChannelFactory*             base,
    const StatContextCreatorFn& statContextCreator,
    bslma::Allocator*           basicAllocator)
: d_baseFactory_p(base)
, d_statContextCreator(statContextCreator)
, d_allocator_p(basicAllocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_statContextCreator);
}

StatChannelFactoryConfig::StatChannelFactoryConfig(
    const StatChannelFactoryConfig& original,
    bslma::Allocator*               basicAllocator)
: d_baseFactory_p(original.d_baseFactory_p)
, d_statContextCreator(original.d_statContextCreator)
, d_allocator_p(basicAllocator)
{
    // NOTHING
}

// ------------------------------
// class StatChannelFactoryHandle
// ------------------------------

StatChannelFactoryHandle::StatChannelFactoryHandle(
    bslma::Allocator* basicAllocator)
: d_factory_p(0)
, d_resultCallback(bsl::allocator_arg_t(), basicAllocator)
, d_options(basicAllocator)
, d_baseConnectHandle()
{
    // NOTHING
}

StatChannelFactoryHandle::~StatChannelFactoryHandle()
{
    // NOTHING
}

void StatChannelFactoryHandle::cancel()
{
    if (d_baseConnectHandle) {
        d_baseConnectHandle->cancel();
    }
}

bmqvt::PropertyBag& StatChannelFactoryHandle::properties()
{
    return d_baseConnectHandle->properties();
}

const StatChannelFactoryHandle::OptionsVariant&
StatChannelFactoryHandle::options() const
{
    return d_options;
}

// ACCESSORS
const bmqvt::PropertyBag& StatChannelFactoryHandle::properties() const
{
    return d_baseConnectHandle->properties();
}

// ------------------------
// class StatChannelFactory
// ------------------------

// PRIVATE MANIPULATORS
void StatChannelFactory::baseResultCallback(
    const HandleSp&                 handleSp,
    ChannelFactoryEvent::Enum       event,
    const Status&                   status,
    const bsl::shared_ptr<Channel>& channel)
{
    // executed by the *IO* thread

    if (event != ChannelFactoryEvent::e_CHANNEL_UP) {
        handleSp->d_resultCallback(event, status, channel);
        return;  // RETURN
    }

    // Create a StatContext for this channel
    bsl::shared_ptr<bmqst::StatContext> statContext(
        d_config.d_statContextCreator(channel, handleSp));
    // Create the channel and notify user
    bsl::shared_ptr<StatChannel> newChannel;
    newChannel.createInplace(
        handleSp->d_allocator_p,
        StatChannelConfig(channel, statContext, handleSp->d_allocator_p),
        handleSp->d_allocator_p);

    handleSp->d_resultCallback(event, status, newChannel);
}

// CREATORS
StatChannelFactory::StatChannelFactory(const Config&     config,
                                       bslma::Allocator* basicAllocator)
: d_config(config, basicAllocator)
{
    // NOTHING
}

StatChannelFactory::~StatChannelFactory()
{
    // NOTHING
}

// MANIPULATORS
void StatChannelFactory::listen(Status*                      status,
                                bslma::ManagedPtr<OpHandle>* handle,
                                const ListenOptions&         options,
                                const ResultCallback&        cb)
{
    HandleSp handleSp;
    handleSp.createInplace(d_config.d_allocator_p, d_config.d_allocator_p);
    handleSp->d_allocator_p    = d_config.d_allocator_p;
    handleSp->d_factory_p      = this;
    handleSp->d_resultCallback = cb;
    handleSp->d_options.assign<ListenOptions>(options);

    if (handle) {
        bslma::ManagedPtr<Handle> handleMp(handleSp.managedPtr());
        handle->loadAlias(handleMp, handleSp.get());
    }

    d_config.d_baseFactory_p->listen(
        status,
        &handleSp->d_baseConnectHandle,
        options,
        bdlf::BindUtil::bind(&StatChannelFactory::baseResultCallback,
                             this,
                             handleSp,
                             bdlf::PlaceHolders::_1,    // event
                             bdlf::PlaceHolders::_2,    // status
                             bdlf::PlaceHolders::_3));  // channel
}

void StatChannelFactory::connect(Status*                      status,
                                 bslma::ManagedPtr<OpHandle>* handle,
                                 const ConnectOptions&        options,
                                 const ResultCallback&        cb)
{
    HandleSp handleSp;
    handleSp.createInplace(d_config.d_allocator_p, d_config.d_allocator_p);
    handleSp->d_allocator_p    = d_config.d_allocator_p;
    handleSp->d_factory_p      = this;
    handleSp->d_resultCallback = cb;
    handleSp->d_options.assign<ConnectOptions>(options);

    if (handle) {
        bslma::ManagedPtr<Handle> handleMp(handleSp.managedPtr());
        handle->loadAlias(handleMp, handleSp.get());
    }

    d_config.d_baseFactory_p->connect(
        status,
        &handleSp->d_baseConnectHandle,
        options,
        bdlf::BindUtil::bind(&StatChannelFactory::baseResultCallback,
                             this,
                             handleSp,
                             bdlf::PlaceHolders::_1,    // event
                             bdlf::PlaceHolders::_2,    // status
                             bdlf::PlaceHolders::_3));  // channel
}

// -----------------------------
// struct StatChannelFactoryUtil
// -----------------------------

bmqst::StatContextConfiguration
StatChannelFactoryUtil::statContextConfiguration(const bsl::string& name,
                                                 int               historySize,
                                                 bslma::Allocator* allocator)
{
    bmqst::StatContextConfiguration config(name, allocator);
    config.isTable(true);
    config.value("in_bytes")
        .value("out_bytes")
        .value("connections")
        .storeExpiredSubcontextValues(true);

    if (historySize != -1) {
        config.defaultHistorySize(historySize);
    }

    return config;
}

bslma::ManagedPtr<bmqst::StatContext>
StatChannelFactoryUtil::createStatContext(const bsl::string& name,
                                          int                historySize,
                                          bslma::Allocator*  allocator)
{
    bslma::ManagedPtr<bmqst::StatContext> rootStatContext;
    rootStatContext.load(
        new (*allocator) bmqst::StatContext(
            statContextConfiguration(name, historySize, allocator),
            allocator),
        allocator);

    return rootStatContext;
}

void StatChannelFactoryUtil::initializeStatsTable(
    bmqst::Table*                             table,
    bmqst::BasicTableInfoProvider*            tip,
    bmqst::StatContext*                       rootStatContext,
    const bmqst::StatValue::SnapshotLocation& start,
    const bmqst::StatValue::SnapshotLocation& end)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(table);
    BSLS_ASSERT_SAFE(tip);
    BSLS_ASSERT_SAFE(rootStatContext);

    // Schema
    bmqst::TableSchema& schema = table->schema();
    schema.addDefaultIdColumn("id");

    schema.addColumn("in_bytes",
                     StatChannel::Stat::e_BYTES_IN,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("out_bytes",
                     StatChannel::Stat::e_BYTES_OUT,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("connections",
                     StatChannel::Stat::e_CONNECTIONS,
                     bmqst::StatUtil::value,
                     start);

    if (!(end == bmqst::StatValue::SnapshotLocation())) {
        schema.addColumn("in_bytes_delta",
                         StatChannel::Stat::e_BYTES_IN,
                         bmqst::StatUtil::valueDifference,
                         start,
                         end);
        schema.addColumn("out_bytes_delta",
                         StatChannel::Stat::e_BYTES_OUT,
                         bmqst::StatUtil::valueDifference,
                         start,
                         end);
        schema.addColumn("connections_delta",
                         StatChannel::Stat::e_CONNECTIONS,
                         bmqst::StatUtil::valueDifference,
                         start,
                         end);
    }

    // Configure records
    bmqst::TableRecords& records = table->records();
    records.setContext(rootStatContext);

    struct local {
        static bool filterDirect(const bmqst::TableRecords::Record& record)
        {
            return record.type() == bmqst::StatContext::e_TOTAL_VALUE;
        }
    };

    records.setFilter(&local::filterDirect);
    records.considerChildrenOfFilteredContexts(true);

    // Create the tip
    tip->setTable(table);
    tip->setColumnGroup("");
    tip->addColumn("id", "").justifyLeft();

    tip->setColumnGroup("In");
    if (!(end == bmqst::StatValue::SnapshotLocation())) {
        tip->addColumn("in_bytes_delta", "delta")
            .zeroString("")
            .printAsMemory();
    }
    tip->addColumn("in_bytes", "total").zeroString("").printAsMemory();

    tip->setColumnGroup("Out");
    if (!(end == bmqst::StatValue::SnapshotLocation())) {
        tip->addColumn("out_bytes_delta", "delta")
            .zeroString("")
            .printAsMemory();
    }
    tip->addColumn("out_bytes", "total").zeroString("").printAsMemory();

    tip->setColumnGroup("Connections");
    if (!(end == bmqst::StatValue::SnapshotLocation())) {
        tip->addColumn("connections_delta", "delta")
            .zeroString("")
            .setPrecision(0);
    }
    tip->addColumn("connections", "total").setPrecision(0);
}

bsls::Types::Int64
StatChannelFactoryUtil::getValue(const bmqst::StatContext& context,
                                 int                       snapshotId,
                                 const Stat::Enum&         stat)
{
    // invoked from the SNAPSHOT thread

    const bmqst::StatValue::SnapshotLocation latestSnapshot(0, 0);
    const bmqst::StatValue::SnapshotLocation oldestSnapshot(0, snapshotId);

#define STAT_SINGLE(OPERATION, STAT)                                          \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_TOTAL_VALUE, STAT),               \
        latestSnapshot)

#define STAT_RANGE(OPERATION, STAT)                                           \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_TOTAL_VALUE, STAT),               \
        latestSnapshot,                                                       \
        oldestSnapshot)

    switch (stat) {
    case Stat::e_BYTES_IN_DELTA: {
        return STAT_RANGE(valueDifference, StatChannel::Stat::e_BYTES_IN);
    }
    case Stat::e_BYTES_IN_ABS: {
        return STAT_SINGLE(value, StatChannel::Stat::e_BYTES_IN);
    }
    case Stat::e_BYTES_OUT_DELTA: {
        return STAT_RANGE(valueDifference, StatChannel::Stat::e_BYTES_OUT);
    }
    case Stat::e_BYTES_OUT_ABS: {
        return STAT_SINGLE(value, StatChannel::Stat::e_BYTES_OUT);
    }
    case Stat::e_CONNECTIONS_DELTA: {
        return STAT_RANGE(valueDifference, StatChannel::Stat::e_CONNECTIONS);
    }
    case Stat::e_CONNECTIONS_ABS: {
        return STAT_SINGLE(value, StatChannel::Stat::e_CONNECTIONS);
    }
    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;

#undef STAT_RANGE
#undef STAT_SINGLE
}

}  // close package namespace
}  // close enterprise namespace
