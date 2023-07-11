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

// mwcio_ntcchannelfactory.cpp                                        -*-C++-*-
#include <mwcio_ntcchannelfactory.h>

#include <mwcscm_version.h>
// NTF
#include <ntcf_system.h>
#include <ntsf_system.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlf_placeholder.h>
#include <bsl_iomanip.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bsls_assert.h>
#include <bsls_platform.h>

namespace BloombergLP {
namespace mwcio {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MWCIO.NTCCHANNELFACTORY");

#if defined(BSLS_PLATFORM_CPU_64_BIT)
#define MWCIO_ADDRESS_WIDTH 16
#else
#define MWCIO_ADDRESS_WIDTH 8
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
               << bsl::setfill('0') << bsl::setw(MWCIO_ADDRESS_WIDTH)
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
    mwcio::ChannelFactoryEvent::Enum             event,
    const mwcio::Status&                         status,
    const bsl::shared_ptr<mwcio::Channel>&       channel,
    const mwcio::ChannelFactory::ResultCallback& callback)
{
    BALL_LOG_TRACE << "NTC factory event " << event << " status " << status
                   << BALL_LOG_END;

    if (event == mwcio::ChannelFactoryEvent::e_CHANNEL_UP) {
        if (channel) {
            bsl::shared_ptr<mwcio::NtcChannel> alias;
            bslstl::SharedPtrUtil::dynamicCast(&alias, channel);
            if (alias) {
                int catalogHandle = d_channels.add(alias);
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
    bsl::shared_ptr<mwcio::NtcListener> listener;
    int rc = d_listeners.remove(handle, &listener);
    if (rc == 0) {
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(listener.get())
                       << " at " << listener->localUri() << " deregistered"
                       << BALL_LOG_END;
    }

    bslmt::LockGuard<bslmt::Mutex> lock(&d_stateMutex);  // LOCKED
    if (d_state == e_STATE_STOPPING) {
        if (d_channels.length() == 0 && d_listeners.length() == 0) {
            BALL_LOG_TRACE << "NTC factory channels and listeners have closed"
                           << BALL_LOG_END;

            d_state = e_STATE_STOPPED;
            d_stateCondition.signal();
        }
    }
}

void NtcChannelFactory::processChannelResult(
    mwcio::ChannelFactoryEvent::Enum             event,
    const mwcio::Status&                         status,
    const bsl::shared_ptr<mwcio::Channel>&       channel,
    const mwcio::ChannelFactory::ResultCallback& callback)
{
    BALL_LOG_TRACE << "NTC factory event " << event << " status " << status
                   << BALL_LOG_END;

    if (event == mwcio::ChannelFactoryEvent::e_CHANNEL_UP) {
        if (channel) {
            bsl::shared_ptr<mwcio::NtcChannel> alias;
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
    bsl::shared_ptr<mwcio::NtcChannel> channel;
    int rc = d_channels.remove(handle, &channel);
    if (rc == 0) {
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(channel.get())
                       << " to " << channel->peerUri() << " deregistered"
                       << BALL_LOG_END;
    }

    bslmt::LockGuard<bslmt::Mutex> lock(&d_stateMutex);  // LOCKED
    if (d_state == e_STATE_STOPPING) {
        if (d_channels.length() == 0 && d_listeners.length() == 0) {
            BALL_LOG_TRACE << "NTC factory channels and listeners have closed"
                           << BALL_LOG_END;

            d_state = e_STATE_STOPPED;
            d_stateCondition.signal();
        }
    }
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
, d_stateMutex()
, d_stateCondition()
, d_state(e_STATE_DEFAULT)
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
, d_stateMutex()
, d_stateCondition()
, d_state(e_STATE_DEFAULT)
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

    BSLS_ASSERT_OPT(d_state == e_STATE_DEFAULT || d_state == e_STATE_STOPPED);
    BSLS_ASSERT_OPT(d_listeners.length() == 0);
    BSLS_ASSERT_OPT(d_channels.length() == 0);
    BSLS_ASSERT_OPT(d_createSignaler.slotCount() == 0);
    BSLS_ASSERT_OPT(d_limitSignaler.slotCount() == 0);
    BSLS_ASSERT_OPT(!d_interface_sp);
}

// MANIPULATORS
int NtcChannelFactory::start()
{
    ntsa::Error error;

    bslmt::LockGuard<bslmt::Mutex> lock(&d_stateMutex);  // LOCKED

    switch (d_state) {
    case e_STATE_DEFAULT:
        error = d_interface_sp->start();
        if (error) {
            return error.number();  // RETURN
        }
        d_state = e_STATE_STARTED;
        return 0;                                               // RETURN
    case e_STATE_STOPPED: d_state = e_STATE_STARTED; return 0;  // RETURN
    case e_STATE_STARTED: return 0;                             // RETURN
    case e_STATE_STOPPING: return 1;                            // RETURN
    default: return 1;                                          // RETURN
    }
}

void NtcChannelFactory::stop()
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_stateMutex);  // LOCKED

    if (d_state != e_STATE_STARTED) {
        return;  // RETURN
    }

    d_state = e_STATE_STOPPING;

    BALL_LOG_TRACE << "NTC factory is stopping" << BALL_LOG_END;

    if (d_channels.length() == 0 && d_listeners.length() == 0) {
        d_state = e_STATE_STOPPED;
    }
    else {
        {
            ChannelIterator iterator(d_channels);
            while (iterator) {
                iterator.value()->close(mwcio::Status());
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

        while (d_state != e_STATE_STOPPED) {
            d_stateCondition.wait(&d_stateMutex);
        }
    }

    BSLS_ASSERT_OPT(d_state == e_STATE_STOPPED);

    lock.release()->unlock();

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

    bslmt::LockGuard<bslmt::Mutex> lock(&d_stateMutex);  // LOCKED

    if (d_state != e_STATE_STARTED) {
        mwcio::NtcListenerUtil::fail(status,
                                     mwcio::StatusCategory::e_GENERIC_ERROR,
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

    bsl::shared_ptr<mwcio::NtcListener> listener;
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

    if (handle) {
        bslma::ManagedPtr<mwcio::NtcListener> alias(listener.managedPtr());
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

    bslmt::LockGuard<bslmt::Mutex> lock(&d_stateMutex);  // LOCKED

    if (d_state != e_STATE_STARTED) {
        mwcio::NtcChannelUtil::fail(status,
                                    mwcio::StatusCategory::e_GENERIC_ERROR,
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

    bsl::shared_ptr<mwcio::NtcChannel> channel;
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
        bslma::ManagedPtr<mwcio::NtcChannel> alias(channel.managedPtr());
        handle->loadAlias(alias, channel.get());
    }

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
    bsl::shared_ptr<mwcio::NtcChannel>* result,
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
    return mwcio::NtcListenerUtil::listenBacklogProperty();
}

bslstl::StringRef NtcChannelFactoryUtil::listenPortProperty()
{
    return mwcio::NtcListenerUtil::listenPortProperty();
}

}  // close package namespace
}  // close enterprise namespace
