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

// bmqio_ntcchannel.cpp                                               -*-C++-*-
#include <bmqio_ntcchannel.h>

#include <bmqscm_version.h>

#include <bmqu_blob.h>

// NTF
#include <ntsf_system.h>

// BDE
#include <ball_log.h>
#include <bdlb_stringrefutil.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlf_placeholder.h>
#include <bsl_cstddef.h>
#include <bsl_iomanip.h>
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslmt_lockguard.h>
#include <bsls_assert.h>
#include <bsls_platform.h>

namespace BloombergLP {
namespace bmqio {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQIO.NTCCHANNEL");

/// Maximum number of bytes to dump when in read/write.
const int k_MAX_BYTES_DUMP = 512;

#if defined(BSLS_PLATFORM_CPU_64_BIT)
#define BMQIO_ADDRESS_WIDTH 16
#else
#define BMQIO_ADDRESS_WIDTH 8
#endif

#define BMQIO_UNUSED(parameter) (void)(parameter)

// Define and set to 1 to enable resolution of a listener's source endpoint
// through the asynchronous bind operation, but note that this implementation
// must artificially block until the asynchronous bind operation completes,
// because the contract of bmqio::ChannelFactory::listen is expected to
// bind and listen synchronously. This configuration option should be set to
// 0 until the channel factory supports some notion of a listener starting
// asynchronously.
#define BMQIO_NTCLISTENER_BIND_ASYNC 0

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

#define BMQIO_NTCCHANNEL_LOG_CONNECT_START(address,                           \
                                           streamSocket,                      \
                                           remoteEndpoint)                    \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " to " << (remoteEndpoint)                          \
                       << " connection initiated" << BALL_LOG_END;            \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_CONNECT_COMPLETE(address, streamSocket, event)   \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " connection complete: " << (event)                 \
                       << BALL_LOG_END;                                       \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_CONNECT_FAILED(address,                          \
                                            streamSocket,                     \
                                            remoteEndpoint,                   \
                                            event)                            \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " to " << (remoteEndpoint)                          \
                       << " connection failed: " << (event) << BALL_LOG_END;  \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_RECEIVE_WOULD_BLOCK(address, streamSocket)       \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " receive WOULD_BLOCK" << BALL_LOG_END;             \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_RECEIVE_EOF(address, streamSocket)               \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " receive EOF" << BALL_LOG_END;                     \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_RECEIVE_FAILED(address, streamSocket, error)     \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " receive failed: " << (event) << BALL_LOG_END;     \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_READ_CACHE_FILLED(address,                       \
                                               streamSocket,                  \
                                               readCache)                     \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " read cache filled to " << (readCache).length()    \
                       << ((readCache).length() == 1 ? " byte" : " bytes")    \
                       << ":\n"                                               \
                       << bmqu::BlobStartHexDumper(&(readCache),              \
                                                   k_MAX_BYTES_DUMP)          \
                       << BALL_LOG_END;                                       \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_READ_CACHE_DRAINED(address,                      \
                                                streamSocket,                 \
                                                readCache)                    \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " read cache drained to " << (readCache).length()   \
                       << ((readCache).length() == 1 ? " byte" : " bytes")    \
                       << ":\n"                                               \
                       << bmqu::BlobStartHexDumper(&(readCache),              \
                                                   k_MAX_BYTES_DUMP)          \
                       << BALL_LOG_END;                                       \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_READ_START(address,                              \
                                        streamSocket,                         \
                                        read,                                 \
                                        numBytes)                             \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " read " << AddressFormatter((read).get())          \
                       << " of " << (numBytes)                                \
                       << ((numBytes) == 1 ? " byte" : " bytes")              \
                       << " started" << BALL_LOG_END;                         \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_READ_SATISFIED(address,                          \
                                            streamSocket,                     \
                                            read,                             \
                                            readCache)                        \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " read " << AddressFormatter((read).get())          \
                       << " of " << (read)->numNeeded()                       \
                       << ((read)->numNeeded() == 1 ? " byte" : " bytes")     \
                       << " satisfied by a read cache of "                    \
                       << (readCache).length()                                \
                       << ((readCache).length() == 1 ? " byte" : " bytes")    \
                       << BALL_LOG_END;                                       \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_READ_COMPLETE(address, streamSocket, read)       \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " read " << AddressFormatter((read).get())          \
                       << " is complete" << BALL_LOG_END;                     \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_READ_MORE(address,                               \
                                       streamSocket,                          \
                                       read,                                  \
                                       numNeeded)                             \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " read " << AddressFormatter((read).get())          \
                       << " now needs " << (numNeeded)                        \
                       << ((numNeeded) == 1 ? " byte" : " bytes")             \
                       << BALL_LOG_END;                                       \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_READ_CAUSED_CLOSE(address, streamSocket, read)   \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " read " << AddressFormatter((read).get())          \
                       << " has indicated the connection should be closed"    \
                       << BALL_LOG_END;                                       \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_WRITE(address, streamSocket, blob)               \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " write of " << (blob).length()                     \
                       << ((blob.length()) == 1 ? " byte" : " bytes")         \
                       << " started:\n"                                       \
                       << bmqu::BlobStartHexDumper(&(blob), k_MAX_BYTES_DUMP) \
                       << BALL_LOG_END;                                       \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_WRITE_WOULD_BLOCK(address, streamSocket, blob)   \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " write of " << (blob).length()                     \
                       << ((blob.length()) == 1 ? " byte" : " bytes")         \
                       << " WOULD_BLOCK" << BALL_LOG_END;                     \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_WRITE_FAILED(address, streamSocket, blob, error) \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " write of " << (blob).length()                     \
                       << ((blob.length()) == 1 ? " byte" : " bytes")         \
                       << " failed: " << (error) << BALL_LOG_END;             \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_EVENT(address, streamSocket, type, event)        \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint() << " "   \
                       << (type) << " event: " << (event) << BALL_LOG_END;    \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_CLOSING(address, streamSocket)                   \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " closing" << BALL_LOG_END;                         \
    } while (false)

#define BMQIO_NTCCHANNEL_LOG_CLOSED(address, streamSocket, status)            \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC channel " << AddressFormatter(address)         \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " closed: " << (status) << BALL_LOG_END;            \
    } while (false)

#if BMQIO_NTCLISTENER_BIND_ASYNC

#define BMQIO_NTCLISTENER_LOG_BIND_START(address,                             \
                                         listenerSocket,                      \
                                         sourceEndpoint)                      \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(address)        \
                       << " at " << (sourceEndpoint) << " bind initiated"     \
                       << BALL_LOG_END;                                       \
    } while (false)

#define BMQIO_NTCLISTENER_LOG_BIND_COMPLETE(address, listenerSocket, event)   \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(address)        \
                       << " at " << (listenerSocket)->sourceEndpoint()        \
                       << " bind complete: " << (event) << BALL_LOG_END;      \
    } while (false)

#define BMQIO_NTCLISTENER_LOG_BIND_FAILED(address,                            \
                                          listenerSocket,                     \
                                          sourceEndpoint,                     \
                                          event)                              \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(address)        \
                       << " at " << (sourceEndpoint)                          \
                       << " bind failed: " << (event) << BALL_LOG_END;        \
    } while (false)

#else

#define BMQIO_NTCLISTENER_LOG_RESOLVE_FAILED(address, endpointString, error)  \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(address)        \
                       << " resolution of '" << (endpointString)              \
                       << " failed: " << (error) << BALL_LOG_END;             \
    } while (false)

#endif

#define BMQIO_NTCLISTENER_LOG_ACCEPT_COMPLETE(address, streamSocket, event)   \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(address)        \
                       << " at " << (streamSocket)->sourceEndpoint()          \
                       << " to " << (streamSocket)->remoteEndpoint()          \
                       << " accept complete: " << (event) << BALL_LOG_END;    \
    } while (false)

#define BMQIO_NTCLISTENER_LOG_ACCEPT_FAILED(address, listenerSocket, event)   \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(address)        \
                       << " at " << (listenerSocket)->sourceEndpoint()        \
                       << " accept failed: " << (event) << BALL_LOG_END;      \
    } while (false)

#define BMQIO_NTCLISTENER_LOG_CLOSING(address, listenerSocket)                \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(address)        \
                       << " at " << (listenerSocket)->sourceEndpoint()        \
                       << " closing" << BALL_LOG_END;                         \
    } while (false)

#define BMQIO_NTCLISTENER_LOG_CLOSED(address, listenerSocket, status)         \
    do {                                                                      \
        BALL_LOG_TRACE << "NTC listener " << AddressFormatter(address)        \
                       << " at " << (listenerSocket)->sourceEndpoint()        \
                       << " closed: " << (status) << BALL_LOG_END;            \
    } while (false)

/// Load into the specified `host` and `port` the hostname and port parsed
/// from the specified `str`, which is of the form [<host>:]port.  Return
/// `0` on success or a negative value if the `str` couldn't be parsed.
int parseEndpoint(bsl::string*             host,
                  bsl::string*             port,
                  const bslstl::StringRef& str)
{
    bdlma::LocalSequentialAllocator<128> arena;

    // Check for a ':'
    bslstl::StringRef foundColon    = bdlb::StringRefUtil::strstr(str, ":");
    bslstl::StringRef portStringRef = str;
    if (!foundColon.isEmpty()) {
        // We have an ip address
        *host = bslstl::StringRef(str.begin(), foundColon.begin());
        portStringRef.assign(foundColon.end(), str.end());
    }

    // Parse the port number
    *port = portStringRef;
    return 0;
}

}  // close unnamed namespace

// -------------
// class NtcRead
// -------------

// CREATORS
NtcRead::NtcRead(const bmqio::Channel::ReadCallback& callback,
                 int                                 numNeeded,
                 bslma::Allocator*                   basicAllocator)
: d_callback(bsl::allocator_arg, basicAllocator, callback)
, d_timer_sp()
, d_numNeeded(numNeeded)
, d_complete(false)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

NtcRead::~NtcRead()
{
    BSLS_ASSERT_OPT(d_numNeeded == 0);
    BSLS_ASSERT_OPT(d_complete);
    BSLS_ASSERT_OPT(!d_timer_sp);
}

// MANIPULATORS
void NtcRead::setNumNeeded(int numNeeded)
{
    if (!d_complete) {
        d_numNeeded = numNeeded;
    }
}

void NtcRead::setTimer(const bsl::shared_ptr<ntci::Timer>& timer)
{
    if (d_timer_sp) {
        d_timer_sp->close();
        d_timer_sp.reset();
    }

    d_timer_sp = timer;
}

void NtcRead::clear()
{
    if (d_timer_sp) {
        d_timer_sp->close();
        d_timer_sp.reset();
    }

    d_numNeeded = 0;
    d_complete  = true;
}

// ACCESSORS
int NtcRead::numNeeded() const
{
    return d_numNeeded;
}

bool NtcRead::isComplete() const
{
    return d_complete;
}

const bmqio::Channel::ReadCallback& NtcRead::callback() const
{
    return d_callback;
}

bslma::Allocator* NtcRead::allocator() const
{
    return d_allocator_p;
}

// ------------------
// class NtcReadQueue
// ------------------

// CREATORS
NtcReadQueue::NtcReadQueue(bslma::Allocator* basicAllocator)
: d_list(basicAllocator)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

NtcReadQueue::~NtcReadQueue()
{
    BSLS_ASSERT_OPT(d_list.empty());
}

// MANIPULATORS
void NtcReadQueue::append(const bsl::shared_ptr<bmqio::NtcRead>& operation)
{
    d_list.push_back(operation);
}

void NtcReadQueue::remove(const bsl::shared_ptr<bmqio::NtcRead>& operation)
{
    for (List::iterator it = d_list.begin(); it != d_list.end(); ++it) {
        if (*it == operation) {
            d_list.erase(it);
            break;
        }
    }
}

void NtcReadQueue::pop()
{
    d_list.pop_front();
}

void NtcReadQueue::pop(bsl::shared_ptr<bmqio::NtcRead>* operation)
{
    *operation = d_list.front();
    d_list.pop_front();
}

bsl::shared_ptr<bmqio::NtcRead> NtcReadQueue::front()
{
    return d_list.front();
}

// ACCESSORS
bsl::size_t NtcReadQueue::size() const
{
    return d_list.size();
}

bool NtcReadQueue::empty() const
{
    return d_list.empty();
}

// ----------------
// class NtcChannel
// ----------------

// PRIVATE MANIPULATORS
void NtcChannel::processConnect(
    const bsl::shared_ptr<ntci::Connector>& connector,
    const ntca::ConnectEvent&               event)
{
    BMQIO_UNUSED(connector);

    bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    if (d_state != e_STATE_OPEN) {
        return;
    }

    if (!event.context().error()) {
        BMQIO_NTCCHANNEL_LOG_CONNECT_COMPLETE(this, d_streamSocket_sp, event);

        d_streamSocket_sp->registerSession(self);

        bmqio::ChannelFactory::ResultCallback resultCallback(
            bsl::allocator_arg,
            d_allocator_p);
        resultCallback.swap(d_resultCallback);

        d_peerUri = d_streamSocket_sp->remoteEndpoint().text();

        lock.release()->unlock();

        if (resultCallback) {
            resultCallback(bmqio::ChannelFactoryEvent::e_CHANNEL_UP,
                           bmqio::Status(),
                           self);
        }
    }
    else {
        BMQIO_NTCCHANNEL_LOG_CONNECT_FAILED(this,
                                            d_streamSocket_sp,
                                            d_options.endpoint(),
                                            event);

        bmqio::Status status;
        NtcChannelUtil::fail(&status,
                             bmqio::StatusCategory::e_CONNECTION,
                             "connect",
                             event.context().error());

        if (event.context().attemptsRemaining() > 0) {
            bmqio::ChannelFactory::ResultCallback resultCallback =
                d_resultCallback;

            if (resultCallback) {
                lock.release()->unlock();
                resultCallback(
                    bmqio::ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED,
                    status,
                    bsl::shared_ptr<bmqio::Channel>());
            }
        }
        else {
            bmqio::ChannelFactory::ResultCallback resultCallback(
                bsl::allocator_arg,
                d_allocator_p);

            resultCallback.swap(d_resultCallback);

            if (resultCallback) {
                bslmt::UnLockGuard<bslmt::Mutex> unlock(&d_mutex);
                resultCallback(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED,
                               status,
                               bsl::shared_ptr<bmqio::Channel>());
            }

            if (d_state != e_STATE_OPEN) {
                return;
            }

            BMQIO_NTCCHANNEL_LOG_CLOSING(this, d_streamSocket_sp);

            d_state = e_STATE_CLOSING;

            d_streamSocket_sp->close(
                bdlf::BindUtil::bind(&NtcChannel::processClose, self, status));
        }
    }
}

void NtcChannel::processReadTimeout(
    const bsl::shared_ptr<bmqio::NtcRead>& read,
    const bsl::shared_ptr<ntci::Timer>&    timer,
    const ntca::TimerEvent&                event)
{
    BMQIO_UNUSED(timer);
    BMQIO_UNUSED(event);

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    d_readQueue.remove(read);

    if (d_state == e_STATE_CLOSED) {
        read->clear();
        return;
    }

    if (read->isComplete()) {
        return;
    }

    bmqio::Channel::ReadCallback readCallback = read->callback();
    read->clear();

    lock.release()->unlock();

    int         numNeeded = 0;
    bdlbb::Blob blob;

    readCallback(bmqio::Status(bmqio::StatusCategory::e_TIMEOUT),
                 &numNeeded,
                 &blob);
}

void NtcChannel::processReadCancelled(
    const bsl::shared_ptr<bmqio::NtcRead>& read)
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    d_readQueue.remove(read);

    if (d_state == e_STATE_CLOSED) {
        read->clear();
        return;
    }

    if (read->isComplete()) {
        return;
    }

    bmqio::Channel::ReadCallback readCallback = read->callback();
    read->clear();

    lock.release()->unlock();

    int         numNeeded = 0;
    bdlbb::Blob blob;

    readCallback(bmqio::Status(bmqio::StatusCategory::e_CANCELED),
                 &numNeeded,
                 &blob);
}

void NtcChannel::processReadQueueLowWatermark(
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
    const ntca::ReadQueueEvent&                event)
{
    BMQIO_UNUSED(streamSocket);

    ntsa::Error error;

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    BMQIO_NTCCHANNEL_LOG_EVENT(this, d_streamSocket_sp, "read queue", event);

    bool close = false;

    while (true) {
        if (d_readQueue.empty()) {
            break;
        }

        bsl::shared_ptr<bmqio::NtcRead> read = d_readQueue.front();

        if (read->numNeeded() == 0 || read->isComplete()) {
            read->clear();
            d_readQueue.pop();
            continue;
        }

        if (read->numNeeded() > d_readCache.length()) {
            ntca::ReceiveContext receiveContext;
            ntca::ReceiveOptions receiveOptions;

            error = d_streamSocket_sp->receive(&receiveContext,
                                               &d_readCache,
                                               receiveOptions);
            if (error) {
                if (error == ntsa::Error(ntsa::Error::e_WOULD_BLOCK)) {
                    BMQIO_NTCCHANNEL_LOG_RECEIVE_WOULD_BLOCK(
                        this,
                        d_streamSocket_sp);
                    break;
                }
                else if (error == ntsa::Error(ntsa::Error::e_EOF)) {
                    BMQIO_NTCCHANNEL_LOG_RECEIVE_EOF(this, d_streamSocket_sp);
                    close = true;
                    break;
                }
                else {
                    BMQIO_NTCCHANNEL_LOG_RECEIVE_FAILED(this,
                                                        d_streamSocket_sp,
                                                        error);
                    close = true;
                    break;
                }
            }

            BMQIO_NTCCHANNEL_LOG_READ_CACHE_FILLED(this,
                                                   d_streamSocket_sp,
                                                   d_readCache);
        }

        if (read->numNeeded() <= d_readCache.length()) {
            BMQIO_NTCCHANNEL_LOG_READ_SATISFIED(this,
                                                d_streamSocket_sp,
                                                read,
                                                d_readCache);

            int numNeeded = 0;
            {
                const bmqio::Channel::ReadCallback& readCallback =
                    read->callback();

                bslmt::UnLockGuard<bslmt::Mutex> unlock(&d_mutex);
                readCallback(bmqio::Status(), &numNeeded, &d_readCache);
            }
            if (read->isComplete()) {
                // It's possible that we encountered canceled or timeout event
                // when we unlocked `d_mutex`, so the `read` pointer that we
                // hold now might be pointing to NtcRead already removed from
                // `d_readQueue`.  There is nothing we can do.
                continue;
            }

            BMQIO_NTCCHANNEL_LOG_READ_CACHE_DRAINED(this,
                                                    d_streamSocket_sp,
                                                    d_readCache);

            if (numNeeded == 0) {
                BMQIO_NTCCHANNEL_LOG_READ_COMPLETE(this,
                                                   d_streamSocket_sp,
                                                   read);
                read->clear();
                d_readQueue.remove(read);
                continue;
            }
            else if (numNeeded < 0) {
                BMQIO_NTCCHANNEL_LOG_READ_CAUSED_CLOSE(this,
                                                       d_streamSocket_sp,
                                                       read);
                close = true;
                break;
            }
            else {
                BMQIO_NTCCHANNEL_LOG_READ_MORE(this,
                                               d_streamSocket_sp,
                                               read,
                                               numNeeded);

                read->setNumNeeded(numNeeded);
            }
        }
    }

    if (close) {
        bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

        bmqio::Status status(bmqio::StatusCategory::e_CANCELED);

        drainReaders(status);

        if (d_state != e_STATE_OPEN) {
            return;
        }

        BMQIO_NTCCHANNEL_LOG_CLOSING(this, d_streamSocket_sp);

        d_state = e_STATE_CLOSING;

        d_streamSocket_sp->close(
            bdlf::BindUtil::bind(&NtcChannel::processClose,
                                 self,
                                 bmqio::Status()));
    }
}

void NtcChannel::processWriteQueueLowWatermark(
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
    const ntca::WriteQueueEvent&               event)
{
    BMQIO_UNUSED(streamSocket);

    BMQIO_NTCCHANNEL_LOG_EVENT(this, d_streamSocket_sp, "write queue", event);

    d_watermarkSignaler(bmqio::ChannelWatermarkType::e_LOW_WATERMARK);
}

void NtcChannel::processWriteQueueHighWatermark(
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
    const ntca::WriteQueueEvent&               event)
{
    BMQIO_UNUSED(streamSocket);

    BMQIO_NTCCHANNEL_LOG_EVENT(this, d_streamSocket_sp, "write queue", event);

    d_watermarkSignaler(bmqio::ChannelWatermarkType::e_HIGH_WATERMARK);
}

void NtcChannel::processShutdownInitiated(
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
    const ntca::ShutdownEvent&                 event)
{
    BMQIO_UNUSED(streamSocket);

    BMQIO_NTCCHANNEL_LOG_EVENT(this, d_streamSocket_sp, "shutdown", event);
}

void NtcChannel::processShutdownReceive(
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
    const ntca::ShutdownEvent&                 event)
{
    BMQIO_UNUSED(streamSocket);

    BMQIO_NTCCHANNEL_LOG_EVENT(this, d_streamSocket_sp, "shutdown", event);

    // Keep the readers until 'processShutdownComplete'.
}

void NtcChannel::processShutdownSend(
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
    const ntca::ShutdownEvent&                 event)
{
    BMQIO_UNUSED(streamSocket);

    BMQIO_NTCCHANNEL_LOG_EVENT(this, d_streamSocket_sp, "shutdown", event);
}

void NtcChannel::processShutdownComplete(
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
    const ntca::ShutdownEvent&                 event)
{
    BMQIO_UNUSED(streamSocket);

    BMQIO_NTCCHANNEL_LOG_EVENT(this, d_streamSocket_sp, "shutdown", event);

    bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

    bmqio::Status status;
    NtcChannelUtil::fail(&status,
                         bmqio::StatusCategory::e_CONNECTION,
                         "shutdown",
                         ntsa::Error(ntsa::Error::e_EOF));

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    drainReaders(status);

    if (d_state != e_STATE_OPEN) {
        return;
    }

    BMQIO_NTCCHANNEL_LOG_CLOSING(this, d_streamSocket_sp);

    d_state = e_STATE_CLOSING;

    d_streamSocket_sp->close(bdlf::BindUtil::bind(&NtcChannel::processClose,
                                                  self,
                                                  bmqio::Status()));
}

void NtcChannel::processClose(const bmqio::Status& status)
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    if (d_state != e_STATE_CLOSING) {
        return;
    }

    BMQIO_NTCCHANNEL_LOG_CLOSED(this, d_streamSocket_sp, status);

    d_state = e_STATE_CLOSED;

    d_streamSocket_sp.reset();
    d_interface_sp.reset();

    d_resultCallback = bmqio::ChannelFactory::ResultCallback();

    lock.release()->unlock();

    d_closeSignaler(status);

    d_watermarkSignaler.disconnectAllSlots();
    d_closeSignaler.disconnectAllSlots();
}

void NtcChannel::drainReaders(const bmqio::Status& status)
{
    while (!d_readQueue.empty()) {
        bsl::shared_ptr<bmqio::NtcRead> read;
        d_readQueue.pop(&read);

        if (!read->isComplete()) {
            bmqio::Channel::ReadCallback readCallback = read->callback();
            read->clear();

            bslmt::UnLockGuard<bslmt::Mutex> unlock(&d_mutex);

            int         numNeeded = 0;
            bdlbb::Blob blob;

            readCallback(status, &numNeeded, &blob);
        }
    }
}

// CREATORS
NtcChannel::NtcChannel(
    const bsl::shared_ptr<ntci::Interface>&      interface,
    const bmqio::ChannelFactory::ResultCallback& resultCallback,
    bslma::Allocator*                            basicAllocator)
: d_mutex()
, d_interface_sp(interface)
, d_streamSocket_sp()
, d_readQueue(basicAllocator)
, d_readCache(basicAllocator)
, d_channelId(0)
, d_peerUri(basicAllocator)
, d_state(e_STATE_DEFAULT)
, d_options(basicAllocator)
, d_properties(basicAllocator)
, d_watermarkSignaler(basicAllocator)
, d_closeSignaler(basicAllocator)
, d_resultCallback(bsl::allocator_arg, basicAllocator, resultCallback)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

NtcChannel::~NtcChannel()
{
    BSLS_ASSERT_OPT(d_state == e_STATE_DEFAULT || d_state == e_STATE_CLOSED);
}

// MANIPULATORS
int NtcChannel::connect(bmqio::Status*               status,
                        const bmqio::ConnectOptions& options)
{
    ntsa::Error error;

    if (status) {
        status->reset();
    }

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

    if (d_state != e_STATE_DEFAULT) {
        bmqio::NtcChannelUtil::fail(status,
                                    bmqio::StatusCategory::e_GENERIC_ERROR,
                                    "state",
                                    ntsa::Error(ntsa::Error::e_INVALID));
        return 1;
    }

    if (options.autoReconnect()) {
        bmqio::NtcChannelUtil::fail(status,
                                    bmqio::StatusCategory::e_GENERIC_ERROR,
                                    "autoReconnectNotSupported",
                                    ntsa::Error(ntsa::Error::e_INVALID));
        return 2;
    }

    ntca::StreamSocketOptions streamSocketOptions;
    streamSocketOptions.setTransport(ntsa::Transport::e_TCP_IPV4_STREAM);
    streamSocketOptions.setKeepHalfOpen(false);

    bsl::shared_ptr<ntci::StreamSocket> streamSocket =
        d_interface_sp->createStreamSocket(streamSocketOptions, d_allocator_p);

    ntci::StreamSocketCloseGuard streamSocketGuard(streamSocket);

    ntca::ConnectOptions connectOptions;
    connectOptions.setIpAddressType(ntsa::IpAddressType::e_V4);

    if (options.numAttempts() > 1) {
        connectOptions.setRetryCount(options.numAttempts() - 1);

        bsls::TimeInterval minRetryInterval;
        minRetryInterval.setTotalMilliseconds(10);

        if (options.attemptInterval() >= minRetryInterval) {
            connectOptions.setRetryInterval(options.attemptInterval());
        }
        else {
            connectOptions.setRetryInterval(minRetryInterval);
        }
    }
    else {
        bsls::TimeInterval minRetryInterval;
        minRetryInterval.setTotalMilliseconds(10);

        if (options.attemptInterval() >= minRetryInterval) {
            connectOptions.setDeadline(streamSocket->currentTime() +
                                       options.attemptInterval());
        }
        else {
            connectOptions.setRetryInterval(streamSocket->currentTime() +
                                            minRetryInterval);
        }
    }

    bsl::string endpointString;
    {
        BSLS_ASSERT_OPT(!options.endpoint().empty());

        bsl::string host;
        bsl::string port;
        int         rc = parseEndpoint(&host, &port, options.endpoint());
        if (rc != 0) {
            bmqio::NtcChannelUtil::fail(status,
                                        bmqio::StatusCategory::e_GENERIC_ERROR,
                                        "connect",
                                        ntsa::Error(ntsa::Error::e_INVALID));
            return 2;
        }

        if (host.empty()) {
            host = "127.0.0.1";
        }

        BSLS_ASSERT_OPT(!port.empty());

        endpointString.assign(host);
        endpointString.append(1, ':');
        endpointString.append(port);
    }

    ntci::ConnectFunction connectCallback = bdlf::BindUtil::bind(
        &NtcChannel::processConnect,
        self,
        bdlf::PlaceHolders::_1,
        bdlf::PlaceHolders::_2);

    BMQIO_NTCCHANNEL_LOG_CONNECT_START(this, streamSocket, endpointString);

    error = streamSocket->connect(endpointString,
                                  connectOptions,
                                  connectCallback);
    if (error) {
        bmqio::NtcChannelUtil::fail(status,
                                    bmqio::StatusCategory::e_CONNECTION,
                                    "connect",
                                    error);
        return 3;
    }

    d_streamSocket_sp = streamSocket;
    d_state           = e_STATE_OPEN;
    d_peerUri         = endpointString;
    d_options         = options;

    streamSocketGuard.release();

    return 0;
}

void NtcChannel::import(
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket)
{
    bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

    d_streamSocket_sp = streamSocket;
    d_state           = e_STATE_OPEN;
    d_peerUri         = d_streamSocket_sp->remoteEndpoint().text();

    d_streamSocket_sp->registerSession(self);
}

void NtcChannel::read(Status*                   status,
                      int                       numBytes,
                      const ReadCallback&       readCallback,
                      const bsls::TimeInterval& timeout)
{
    ntsa::Error error;

    if (status) {
        status->reset();
    }

    bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    if (d_state != e_STATE_OPEN) {
        bmqio::NtcChannelUtil::fail(status,
                                    bmqio::StatusCategory::e_GENERIC_ERROR,
                                    "state",
                                    ntsa::Error(ntsa::Error::e_INVALID));
        return;
    }

    bsl::shared_ptr<bmqio::NtcRead> read;
    read.createInplace(d_allocator_p, readCallback, numBytes, d_allocator_p);

    BMQIO_NTCCHANNEL_LOG_READ_START(this, d_streamSocket_sp, read, numBytes);

    bsl::shared_ptr<ntci::Timer> timer;
    if (timeout != bsls::TimeInterval()) {
        ntca::TimerOptions timerOptions;
        timerOptions.setOneShot(true);
        timerOptions.showEvent(ntca::TimerEventType::e_DEADLINE);
        timerOptions.hideEvent(ntca::TimerEventType::e_CANCELED);
        timerOptions.hideEvent(ntca::TimerEventType::e_CLOSED);

        ntci::TimerCallback timerCallback =
            d_streamSocket_sp->createTimerCallback(
                bdlf::BindUtil::bind(&NtcChannel::processReadTimeout,
                                     self,
                                     read,
                                     bdlf::PlaceHolders::_1,
                                     bdlf::PlaceHolders::_2),
                d_allocator_p);

        timer = d_streamSocket_sp->createTimer(timerOptions,
                                               timerCallback,
                                               d_allocator_p);

        read->setTimer(timer);
    }

    bool enableRead = false;
    if (d_readQueue.empty()) {
        enableRead = true;
    }

    if (enableRead) {
        error = d_streamSocket_sp->relaxFlowControl(
            ntca::FlowControlType::e_RECEIVE);
        if (error) {
            bmqio::NtcChannelUtil::fail(status,
                                        bmqio::StatusCategory::e_GENERIC_ERROR,
                                        "relaxFlowControl",
                                        error);

            return;
        }
        // Move the read queue low watermark so not to miss already received
        // data.
        d_streamSocket_sp->setReadQueueLowWatermark(1);
    }

    if (timer) {
        error = timer->schedule(timer->currentTime() + timeout);
        if (error) {
            bmqio::NtcChannelUtil::fail(status,
                                        bmqio::StatusCategory::e_GENERIC_ERROR,
                                        "schedule",
                                        error);

            return;
        }
    }

    d_readQueue.append(read);
}

void NtcChannel::write(Status*            status,
                       const bdlbb::Blob& blob,
                       bsls::Types::Int64 watermark)
{
    ntsa::Error error;

    if (status) {
        status->reset();
    }

    bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    if (d_state != e_STATE_OPEN) {
        bmqio::NtcChannelUtil::fail(status,
                                    bmqio::StatusCategory::e_GENERIC_ERROR,
                                    "state",
                                    ntsa::Error(ntsa::Error::e_INVALID));
        return;
    }

    BMQIO_NTCCHANNEL_LOG_WRITE(this, d_streamSocket_sp, blob);

    ntca::SendOptions sendOptions;
    if (watermark != bsl::numeric_limits<int>::max()) {
        sendOptions.setHighWatermark(watermark);
    }

    error = d_streamSocket_sp->send(blob, sendOptions);
    if (error) {
        if (error == ntsa::Error::e_WOULD_BLOCK) {
            BMQIO_NTCCHANNEL_LOG_WRITE_WOULD_BLOCK(this,
                                                   d_streamSocket_sp,
                                                   blob);
            NtcChannelUtil::fail(status,
                                 bmqio::StatusCategory::e_LIMIT,
                                 "send",
                                 error);
        }
        else {
            BMQIO_NTCCHANNEL_LOG_WRITE_FAILED(this,
                                              d_streamSocket_sp,
                                              blob,
                                              error);
            NtcChannelUtil::fail(status,
                                 bmqio::StatusCategory::e_CONNECTION,
                                 "send",
                                 error);
        }
    }
}

void NtcChannel::cancel()
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

    if (d_state != e_STATE_OPEN) {
        return;
    }

    BMQIO_NTCCHANNEL_LOG_CLOSING(this, d_streamSocket_sp);

    d_state = e_STATE_CLOSING;

    bmqio::Status status(bmqio::StatusCategory::e_CANCELED);
    d_streamSocket_sp->close(
        bdlf::BindUtil::bind(&NtcChannel::processClose, self, status));
}

void NtcChannel::cancelRead()
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

    if (d_state != e_STATE_OPEN) {
        return;
    }

    while (!d_readQueue.empty()) {
        bsl::shared_ptr<bmqio::NtcRead> read;
        d_readQueue.pop(&read);

        read->setTimer(bsl::shared_ptr<ntci::Timer>());

        d_streamSocket_sp->execute(
            bdlf::BindUtil::bind(&NtcChannel::processReadCancelled,
                                 self,
                                 read));
    }
}

void NtcChannel::close(const Status& status)
{
    // Executed from *ANY* thread

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    bsl::shared_ptr<NtcChannel> self = this->shared_from_this();

    if (d_state != e_STATE_OPEN) {
        return;
    }

    while (!d_readQueue.empty()) {
        bsl::shared_ptr<bmqio::NtcRead> read;
        d_readQueue.pop(&read);

        // This code assumes thread-safety of `ntci::Timer::close` because we
        // are not in IO thread.
        // `read->d_callback` can still be executed concurrently from IO
        // thread.
        read->clear();
    }

    BMQIO_NTCCHANNEL_LOG_CLOSING(this, d_streamSocket_sp);

    d_state = e_STATE_CLOSING;

    d_streamSocket_sp->close(
        bdlf::BindUtil::bind(&NtcChannel::processClose, self, status));
}

int NtcChannel::execute(const ExecuteCb& cb)
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);
    if (d_streamSocket_sp) {
        d_streamSocket_sp->execute(cb);
        return 0;  // RETURN
    }
    return -1;
}

bdlmt::SignalerConnection NtcChannel::onClose(const CloseFn& cb)
{
    return d_closeSignaler.connect(cb);
}

bdlmt::SignalerConnection NtcChannel::onClose(const CloseFn& cb, int group)
{
    return d_closeSignaler.connect(cb, group);
}

bdlmt::SignalerConnection NtcChannel::onWatermark(const WatermarkFn& cb)
{
    return d_watermarkSignaler.connect(cb);
}

bmqvt::PropertyBag& NtcChannel::properties()
{
    return d_properties;
}

void NtcChannel::setChannelId(int channelId)
{
    d_channelId = channelId;
}

void NtcChannel::setWriteQueueLowWatermark(int lowWatermark)
{
    if (d_streamSocket_sp) {
        d_streamSocket_sp->setWriteQueueLowWatermark(lowWatermark);
    }
}

void NtcChannel::setWriteQueueHighWatermark(int highWatermark)
{
    if (d_streamSocket_sp) {
        d_streamSocket_sp->setWriteQueueHighWatermark(highWatermark);
    }
}

// ACCESSORS
int NtcChannel::channelId() const
{
    return d_channelId;
}

ntsa::Endpoint NtcChannel::peerEndpoint() const
{
    return d_streamSocket_sp ? d_streamSocket_sp->remoteEndpoint()
                             : ntsa::Endpoint();
}

ntsa::Endpoint NtcChannel::sourceEndpoint() const
{
    return d_streamSocket_sp ? d_streamSocket_sp->sourceEndpoint()
                             : ntsa::Endpoint();
}

const bsl::string& NtcChannel::peerUri() const
{
    return d_peerUri;
}

const bmqvt::PropertyBag& NtcChannel::properties() const
{
    return d_properties;
}

bslma::Allocator* NtcChannel::allocator() const
{
    return d_allocator_p;
}

const ntci::StreamSocket& NtcChannel::streamSocket() const
{
    BSLS_ASSERT(d_streamSocket_sp);
    return *d_streamSocket_sp;
}

// ---------------------
// struct NtcChannelUtil
// ---------------------

// CLASS METHODS
void NtcChannelUtil::fail(Status*                     status,
                          bmqio::StatusCategory::Enum category,
                          const bslstl::StringRef&    operation,
                          const ntsa::Error&          error)
{
    BSLS_ASSERT_OPT(error);

    if (status) {
        status->reset(category);
        status->properties().set("ntfOperation", operation);
        status->properties().set("tcpPlatformError", error.number());
    }
}

// -----------------
// class NtcListener
// -----------------

void NtcListener::processAccept(
    const bsl::shared_ptr<ntci::Acceptor>&     acceptor,
    const bsl::shared_ptr<ntci::StreamSocket>& streamSocket,
    const ntca::AcceptEvent&                   event)
{
    ntsa::Error error;

    bsl::shared_ptr<NtcListener> self = this->shared_from_this();

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    if (d_state != e_STATE_OPEN) {
        if (streamSocket) {
            streamSocket->close();
        }
        return;
    }

    if (!event.context().error()) {
        bsl::shared_ptr<bmqio::NtcChannel> channel;
        channel.createInplace(d_allocator_p,
                              d_interface_sp,
                              d_resultCallback,
                              d_allocator_p);

        BMQIO_NTCLISTENER_LOG_ACCEPT_COMPLETE(channel.get(),
                                              streamSocket,
                                              event);

        channel->import(streamSocket);

        {
            bslmt::UnLockGuard<bslmt::Mutex> unlock(&d_mutex);
            d_resultCallback(bmqio::ChannelFactoryEvent::e_CHANNEL_UP,
                             bmqio::Status(),
                             channel);
        }

        if (d_state != e_STATE_OPEN) {
            return;
        }

        error = acceptor->accept(
            ntca::AcceptOptions(),
            bdlf::BindUtil::bind(&NtcListener::processAccept,
                                 self,
                                 bdlf::PlaceHolders::_1,
                                 bdlf::PlaceHolders::_2,
                                 bdlf::PlaceHolders::_3));
        if (error) {
            bmqio::Status status;
            NtcChannelUtil::fail(&status,
                                 bmqio::StatusCategory::e_GENERIC_ERROR,
                                 "accept",
                                 error);

            BMQIO_NTCLISTENER_LOG_CLOSING(this, d_listenerSocket_sp);

            d_state = e_STATE_CLOSING;

            d_listenerSocket_sp->close(
                bdlf::BindUtil::bind(&NtcListener::processClose,
                                     self,
                                     status));
        }
    }
    else {
        BMQIO_NTCLISTENER_LOG_ACCEPT_FAILED(this, d_listenerSocket_sp, event);

        bmqio::Status status;
        NtcChannelUtil::fail(&status,
                             bmqio::StatusCategory::e_CONNECTION,
                             "accept",
                             event.context().error());

        BMQIO_NTCLISTENER_LOG_CLOSING(this, d_listenerSocket_sp);

        d_state = e_STATE_CLOSING;

        d_listenerSocket_sp->close(
            bdlf::BindUtil::bind(&NtcListener::processClose, self, status));
    }
}

void NtcListener::processClose(const bmqio::Status& status)
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    if (d_state != e_STATE_CLOSING) {
        return;
    }

    BMQIO_NTCLISTENER_LOG_CLOSED(this, d_listenerSocket_sp, status);

    d_state = e_STATE_CLOSED;

    d_listenerSocket_sp.reset();
    d_interface_sp.reset();

    d_resultCallback = bmqio::ChannelFactory::ResultCallback();

    lock.release()->unlock();

    d_closeSignaler(status);

    d_closeSignaler.disconnectAllSlots();
}

// CREATORS
NtcListener::NtcListener(
    const bsl::shared_ptr<ntci::Interface>&      interface,
    const bmqio::ChannelFactory::ResultCallback& resultCallback,
    bslma::Allocator*                            basicAllocator)
: d_mutex()
, d_interface_sp(interface)
, d_listenerSocket_sp()
, d_state(e_STATE_DEFAULT)
, d_options(basicAllocator)
, d_properties(basicAllocator)
, d_closeSignaler(basicAllocator)
, d_resultCallback(bsl::allocator_arg, basicAllocator, resultCallback)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

NtcListener::~NtcListener()
{
    BSLS_ASSERT_OPT(d_state == e_STATE_DEFAULT || d_state == e_STATE_CLOSED);
}

// MANIPULATORS
int NtcListener::listen(bmqio::Status*              status,
                        const bmqio::ListenOptions& options)
{
    ntsa::Error error;

    if (status) {
        status->reset();
    }

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    bsl::shared_ptr<NtcListener> self = this->shared_from_this();

    if (d_state != e_STATE_DEFAULT) {
        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "state",
                                     ntsa::Error(ntsa::Error::e_INVALID));
        return 1;
    }

    int backlog;
    if (!options.properties().load(&backlog,
                                   NtcListenerUtil::listenBacklogProperty())) {
        backlog = 10;
    }

    ntsa::Endpoint endpoint;
    bsl::string    endpointString;
    {
        BSLS_ASSERT_OPT(!options.endpoint().empty());

        bsl::string host;
        bsl::string port;
        int         rc = parseEndpoint(&host, &port, options.endpoint());
        if (rc != 0) {
            bmqio::NtcListenerUtil::fail(
                status,
                bmqio::StatusCategory::e_GENERIC_ERROR,
                "bind",
                ntsa::Error(ntsa::Error::e_INVALID));
            return 2;
        }

        if (host.empty()) {
            host = "0.0.0.0";
        }

        BSLS_ASSERT_OPT(!port.empty());

        endpointString.assign(host);
        endpointString.append(1, ':');
        endpointString.append(port);
    }

    ntca::ListenerSocketOptions listenerSocketOptions;
    listenerSocketOptions.setTransport(ntsa::Transport::e_TCP_IPV4_STREAM);
    listenerSocketOptions.setReuseAddress(true);
    listenerSocketOptions.setKeepHalfOpen(false);
    listenerSocketOptions.setBacklog(backlog);

#if BMQIO_NTCLISTENER_BIND_ASYNC == 0

    bsl::shared_ptr<ntsi::Resolver> resolver = ntsf::System::createResolver(
        d_allocator_p);

    ntsa::EndpointOptions endpointOptions;
    endpointOptions.setTransport(ntsa::Transport::e_TCP_IPV4_STREAM);

    error = resolver->getEndpoint(&endpoint, endpointString, endpointOptions);
    if (error) {
        BMQIO_NTCLISTENER_LOG_RESOLVE_FAILED(this, options.endpoint(), error);

        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "resolve",
                                     error);
        return 3;
    }

    listenerSocketOptions.setSourceEndpoint(endpoint);

#endif

    bsl::shared_ptr<ntci::ListenerSocket> listenerSocket =
        d_interface_sp->createListenerSocket(listenerSocketOptions,
                                             d_allocator_p);

    ntci::ListenerSocketCloseGuard listenerSocketGuard(listenerSocket);

    error = listenerSocket->open();
    if (error) {
        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "open",
                                     error);
        return 4;
    }

#if BMQIO_NTCLISTENER_BIND_ASYNC

    BMQIO_NTCLISTENER_LOG_BIND_START(this, listenerSocket, endpointString);

    ntca::BindOptions bindOptions;
    bindOptions.setIpAddressType(ntsa::IpAddressType::e_V4);

    ntci::BindFuture bindFuture;

    error = listenerSocket->bind(endpointString, bindOptions, bindFuture);
    if (error) {
        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "bind",
                                     error);
        return 5;
    }

    ntci::BindResult bindResult;
    error = bindFuture.wait(&bindResult);
    if (error) {
        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "bind",
                                     error);
        return 6;
    }

    if (bindResult.event().context().error()) {
        BMQIO_NTCLISTENER_LOG_BIND_FAILED(this,
                                          listenerSocket,
                                          options.endpoint(),
                                          bindResult.event());

        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "bind",
                                     bindResult.event().context().error());
        return 7;
    }

    BMQIO_NTCLISTENER_LOG_BIND_COMPLETE(this,
                                        listenerSocket,
                                        bindResult.event());

#endif

    error = listenerSocket->listen(backlog);
    if (error) {
        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "listen",
                                     error);
        return 8;
    }

    endpoint = listenerSocket->sourceEndpoint();
    if (!endpoint.isIp() && !endpoint.ip().host().isV4()) {
        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "bind",
                                     ntsa::Error(ntsa::Error::e_INVALID));
        return 9;
    }

    d_properties.set(bmqio::NtcListenerUtil::listenPortProperty(),
                     static_cast<int>(endpoint.ip().port()));

    ntci::AcceptFunction acceptCallback = bdlf::BindUtil::bind(
        &NtcListener::processAccept,
        self,
        bdlf::PlaceHolders::_1,
        bdlf::PlaceHolders::_2,
        bdlf::PlaceHolders::_3);

    error = listenerSocket->accept(ntca::AcceptOptions(), acceptCallback);
    if (error) {
        bmqio::NtcListenerUtil::fail(status,
                                     bmqio::StatusCategory::e_GENERIC_ERROR,
                                     "accept",
                                     ntsa::Error(ntsa::Error::e_INVALID));
        return 10;
    }

    d_listenerSocket_sp = listenerSocket;
    d_state             = e_STATE_OPEN;
    d_localUri          = listenerSocket->sourceEndpoint().text();
    d_options           = options;

    listenerSocketGuard.release();

    return 0;
}

void NtcListener::cancel()
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);

    bsl::shared_ptr<NtcListener> self = this->shared_from_this();

    if (d_state != e_STATE_OPEN) {
        return;
    }

    BMQIO_NTCLISTENER_LOG_CLOSING(this, d_listenerSocket_sp);

    d_state = e_STATE_CLOSING;

    bmqio::Status status(bmqio::StatusCategory::e_CANCELED);
    d_listenerSocket_sp->close(
        bdlf::BindUtil::bind(&NtcListener::processClose, self, status));
}

bdlmt::SignalerConnection NtcListener::onClose(const CloseFn& cb)
{
    return d_closeSignaler.connect(cb);
}

bdlmt::SignalerConnection NtcListener::onClose(const CloseFn& cb, int group)
{
    return d_closeSignaler.connect(cb, group);
}

bmqvt::PropertyBag& NtcListener::properties()
{
    return d_properties;
}

// ACCESSORS
const bsl::string& NtcListener::localUri() const
{
    return d_localUri;
}

const bmqvt::PropertyBag& NtcListener::properties() const
{
    return d_properties;
}

bslma::Allocator* NtcListener::allocator() const
{
    return d_allocator_p;
}

// ----------------------
// struct NtcListenerUtil
// ----------------------

// CLASS METHODS
bslstl::StringRef NtcListenerUtil::listenBacklogProperty()
{
    return bslstl::StringRef("tcp.listen.backlog", 18);
}

bslstl::StringRef NtcListenerUtil::listenPortProperty()
{
    return bslstl::StringRef("tcp.listen.port", 15);
}

void NtcListenerUtil::fail(Status*                     status,
                           bmqio::StatusCategory::Enum category,
                           const bslstl::StringRef&    operation,
                           const ntsa::Error&          error)
{
    BSLS_ASSERT_OPT(error);

    if (status) {
        status->reset(category);
        status->properties().set("ntfOperation", operation);
        status->properties().set("tcpPlatformError", error.number());
    }
}

}  // close package namespace
}  // close enterprise namespace
