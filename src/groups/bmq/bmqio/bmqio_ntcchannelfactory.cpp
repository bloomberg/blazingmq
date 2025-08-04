// Copyright 2022-2023 Bloomberg Finance L.P.
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

// bmqio_ntcchannelfactory.cpp                                        -*-C++-*-
#include <bmqio_ntcchannelfactory.h>

#include <bmqscm_version.h>
// NTF
#include <ntcf_system.h>
#include <ntsf_system.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlf_placeholder.h>
#include <bsl_iomanip.h>
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bsls_assert.h>
#include <bsls_platform.h>

namespace BloombergLP {
namespace bmqio {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQIO.NTCCHANNELFACTORY");

#if defined(BSLS_PLATFORM_CPU_64_BIT)
#define BMQIO_ADDRESS_WIDTH 16
#else
#define BMQIO_ADDRESS_WIDTH 8
#endif

const int k_CLOSE_GROUP = bsl::numeric_limits<int>::max();

struct AddressFormatter {
    void* d_address_p;
    explicit AddressFormatter(void* address)
    : d_address_p(address)
    {
    }
    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const AddressFormatter& object)
    {
        bsl::ios_base::fmtflags flags     = stream.flags();
        bsl::streamsize         precision = stream.precision();
        char                    fill      = stream.fill();

        stream << bsl::hex << bsl::showbase << bsl::internal
               << bsl::setfill('0') << bsl::setw(BMQIO_ADDRESS_WIDTH)
               << object.d_address_p;

        stream.flags(flags);
        stream.precision(precision);
        stream.fill(fill);

        return stream;
    }
};

}  // close unnamed namespace

// PRIVATE MANIPULATORS
void NtcChannelFactory::processListenerResult(
    bmqio::ChannelFactoryEvent::Enum             event,
    const bmqio::Status&                         status,
    const bsl::shared_ptr<bmqio::Channel>&       channel,
    const bmqio::ChannelFactory::ResultCallback& callback)
{
    BALL_LOG_TRACE << "NTC factory event " << event << " status " << status
                   << BALL_LOG_END;

    if (event == bmqio::ChannelFactoryEvent::e_CHANNEL_UP) {
        if (channel) {
            bsl::shared_ptr<bmqio::NtcChannel> alias;
            bslstl::SharedPtrUtil::dynamicCast(&alias, channel);
            if (alias) {
                int catalogHandle = d_channels.add(alias);

                // Increment resource usage count for new channel
                d_resourceMonitor.acquire();

                alias->setChannelId(catalogHandle);
                alias->onClose(bdlf::BindUtil::bind(
                                   &NtcChannelFactory::processChannelClosed,
                                   this,
                                   catalogHandle),
                               k_CLOSE_GROUP);

                d_createSignaler(alias, alias);

                BALL_LOG_TRACE << "NTC channel "
                               << AddressFormatter(alias.get()) << " to "
                               << alias->peerUri() << " registered"
                               << BALL_LOG_END;
            }
        }
    }

    callback(event, status, channel);
}

void NtcChannelFactory::processListenerClosed(int handle)
{
    bsl::shared_ptr<bmqio::NtcListener> listener;
    int rc = d_listeners.remove(handle, &listener);
    if (rc == 0) {
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(listener.get())
                       << " at " << listener->localUri() << " deregistered"
                       << BALL_LOG_END;
    }

    d_resourceMonitor.release();  // Decrement resource usage count
}

void NtcChannelFactory::processChannelResult(
    bmqio::ChannelFactoryEvent::Enum             event,
    const bmqio::Status&                         status,
    const bsl::shared_ptr<bmqio::Channel>&       channel,
    const bmqio::ChannelFactory::ResultCallback& callback)
{
    BALL_LOG_TRACE << "NTC factory event " << event << " status " << status
                   << BALL_LOG_END;

    if (event == bmqio::ChannelFactoryEvent::e_CHANNEL_UP) {
        if (channel) {
            bsl::shared_ptr<bmqio::NtcChannel> alias;
            bslstl::SharedPtrUtil::dynamicCast(&alias, channel);
            if (alias) {
                d_createSignaler(alias, alias);
            }
        }
    }

    callback(event, status, channel);
}

void NtcChannelFactory::processChannelClosed(int handle)
{
    bsl::shared_ptr<bmqio::NtcChannel> channel;
    int rc = d_channels.remove(handle, &channel);
    if (rc == 0) {
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(channel.get())
                       << " to " << channel->peerUri() << " deregistered"
                       << BALL_LOG_END;
    }

    d_resourceMonitor.release();  // Decrement resource usage count
}

// CREATORS

NtcChannelFactory::NtcChannelFactory(
    const bsl::shared_ptr<ntci::Interface>& interface,
    bslma::Allocator*                       basicAllocator)
: d_interface_sp(interface)
, d_listeners(basicAllocator)
, d_channels(basicAllocator)
, d_createSignaler(basicAllocator)
, d_limitSignaler(basicAllocator)
, d_owned(false)
, d_validator(false)
, d_resourceMonitor(false)
, d_isInterfaceStarted(false)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

NtcChannelFactory::NtcChannelFactory(
    const ntca::InterfaceConfig& interfaceConfig,
    bdlbb::BlobBufferFactory*    blobBufferFactory,
    bslma::Allocator*            basicAllocator)
: d_interface_sp()
, d_listeners(basicAllocator)
, d_channels(basicAllocator)
, d_createSignaler(basicAllocator)
, d_limitSignaler(basicAllocator)
, d_owned(true)
, d_validator(false)
, d_resourceMonitor(false)
, d_isInterfaceStarted(false)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    bsl::shared_ptr<bdlbb::BlobBufferFactory> blobBufferFactory_sp(
        blobBufferFactory,
        bslstl::SharedPtrNilDeleter(),
        d_allocator_p);

    d_interface_sp = ntcf::System::createInterface(interfaceConfig,
                                                   blobBufferFactory_sp,
                                                   d_allocator_p);
}

NtcChannelFactory::~NtcChannelFactory()
{
    this->stop();

    if (d_owned) {
        d_interface_sp->closeAll();
        d_interface_sp->shutdown();
        d_interface_sp->linger();
        d_interface_sp.reset();
    }

    BSLS_ASSERT_OPT(d_listeners.length() == 0);
    BSLS_ASSERT_OPT(d_channels.length() == 0);
    BSLS_ASSERT_OPT(d_createSignaler.slotCount() == 0);
    BSLS_ASSERT_OPT(d_limitSignaler.slotCount() == 0);
    BSLS_ASSERT_OPT(!d_interface_sp);
}

// MANIPULATORS
int NtcChannelFactory::start()
{
    bmqu::AtomicValidatorGuard valGuard(&d_validator);

    if (valGuard.isValid()) {
        // Already started.
        return 1;  // RETURN
    }

    if (!d_isInterfaceStarted) {
        // Make sure we don't restart the same interface if we have
        // `start()`, `stop()`, `start()` sequence.
        d_isInterfaceStarted    = true;
        const ntsa::Error error = d_interface_sp->start();
        if (error) {
            return error.number();  // RETURN
        }
    }

    d_resourceMonitor.reset();
    d_validator.reset();

    return 0;
}

void NtcChannelFactory::stop()
{
    bmqu::AtomicValidatorGuard valGuard(&d_validator);

    if (!valGuard.isValid()) {
        return;  // RETURN
    }

    valGuard.release()->release();
    d_validator.invalidate();  // Disallow new listen/connect

    BALL_LOG_TRACE << "NTC factory is stopping" << BALL_LOG_END;

    {
        ChannelIterator iterator(d_channels);
        while (iterator) {
            iterator.value()->close(bmqio::Status());
            ++iterator;
        }
    }

    {
        ListenerIterator iterator(d_listeners);
        while (iterator) {
            iterator.value()->cancel();
            ++iterator;
        }
    }

    // Wait until all channels and listeners are finished
    d_resourceMonitor.invalidate();

    d_createSignaler.disconnectAllSlots();
    d_limitSignaler.disconnectAllSlots();

    BALL_LOG_TRACE << "NTC factory has stopped" << BALL_LOG_END;
}

void NtcChannelFactory::listen(Status*                      status,
                               bslma::ManagedPtr<OpHandle>* handle,
                               const ListenOptions&         options,
                               const ResultCallback&        cb)
{
    ntsa::Error error;
    int         rc;

    if (status) {
        status->reset();
    }

    if (handle) {
        handle->reset();
    }

    bmqu::AtomicValidatorGuard valGuard(&d_validator);

    if (!valGuard.isValid()) {
        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "state",
                                     ntsa::Error(ntsa::Error::e_INVALID));
        return;  // RETURN
    }

    ResultCallback resultCallbackProxy = bdlf::BindUtil::bind(
        &NtcChannelFactory::processListenerResult,
        this,
        bdlf::PlaceHolders::_1,
        bdlf::PlaceHolders::_2,
        bdlf::PlaceHolders::_3,
        cb);

    bsl::shared_ptr<bmqio::NtcListener> listener;
    listener.createInplace(d_allocator_p,
                           d_interface_sp,
                           resultCallbackProxy,
                           d_allocator_p);

    int catalogHandle = d_listeners.add(listener);

    listener->onClose(
        bdlf::BindUtil::bind(&NtcChannelFactory::processListenerClosed,
                             this,
                             catalogHandle),
        k_CLOSE_GROUP);

    rc = listener->listen(status, options);
    if (rc != 0) {
        d_listeners.remove(catalogHandle);
        return;  // RETURN
    }

    // Increment resource usage count for new listener
    d_resourceMonitor.acquire();

    if (handle) {
        bslma::ManagedPtr<bmqio::NtcListener> alias(listener.managedPtr());
        handle->loadAlias(alias, listener.get());
    }

    BALL_LOG_TRACE << "NTC listener " << AddressFormatter(listener.get())
                   << " at " << listener->localUri() << " registered"
                   << BALL_LOG_END;
}

void NtcChannelFactory::connect(Status*                      status,
                                bslma::ManagedPtr<OpHandle>* handle,
                                const ConnectOptions&        options,
                                const ResultCallback&        cb)
{
    ntsa::Error error;
    int         rc;

    if (status) {
        status->reset();
    }

    if (handle) {
        handle->reset();
    }

    bmqu::AtomicValidatorGuard valGuard(&d_validator);

    if (!valGuard.isValid()) {
        bmqio::NtcChannelUtil::fail(status,
                                    bmqio::StatusCategory::e_GENERIC_ERROR,
                                    "state",
                                    ntsa::Error(ntsa::Error::e_INVALID));
        return;  // RETURN
    }

    ResultCallback resultCallbackProxy = bdlf::BindUtil::bind(
        &NtcChannelFactory::processChannelResult,
        this,
        bdlf::PlaceHolders::_1,
        bdlf::PlaceHolders::_2,
        bdlf::PlaceHolders::_3,
        cb);

    bsl::shared_ptr<bmqio::NtcChannel> channel;
    channel.createInplace(d_allocator_p,
                          d_interface_sp,
                          resultCallbackProxy,
                          d_allocator_p);

    int catalogHandle = d_channels.add(channel);

    channel->setChannelId(catalogHandle);

    channel->onClose(
        bdlf::BindUtil::bind(&NtcChannelFactory::processChannelClosed,
                             this,
                             catalogHandle),
        k_CLOSE_GROUP);

    rc = channel->connect(status, options);
    if (rc != 0) {
        d_channels.remove(catalogHandle);
        return;  // RETURN
    }

    if (handle) {
        bslma::ManagedPtr<bmqio::NtcChannel> alias(channel.managedPtr());
        handle->loadAlias(alias, channel.get());
    }

    // Increment resource usage count for new channel
    d_resourceMonitor.acquire();

    BALL_LOG_TRACE << "NTC channel " << AddressFormatter(channel.get())
                   << " to " << channel->peerUri() << " registered"
                   << BALL_LOG_END;
}

bdlmt::SignalerConnection NtcChannelFactory::onCreate(const CreateFn& cb)
{
    return d_createSignaler.connect(cb);
}

bdlmt::SignalerConnection NtcChannelFactory::onLimit(const LimitFn& cb)
{
    return d_limitSignaler.connect(cb);
}

int NtcChannelFactory::lookupChannel(
    bsl::shared_ptr<bmqio::NtcChannel>* result,
    int                                 channelId)
{
    return d_channels.find(channelId, result);
}

// ----------------------------
// struct NtcChannelFactoryUtil
// ----------------------------

// CLASS METHODS
bslstl::StringRef NtcChannelFactoryUtil::listenBacklogProperty()
{
    return bmqio::NtcListenerUtil::listenBacklogProperty();
}

bslstl::StringRef NtcChannelFactoryUtil::listenPortProperty()
{
    return bmqio::NtcListenerUtil::listenPortProperty();
}

}  // close package namespace
}  // close enterprise namespace
