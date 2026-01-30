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

// bmqio_testchannel.cpp                                              -*-C++-*-
#include <bmqio_testchannel.h>

#include <bmqscm_version.h>
// BDE
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>

namespace BloombergLP {
namespace bmqio {

// -----------------
// class TestChannel
// -----------------

// CREATORS
TestChannel::TestChannel(bslma::Allocator* basicAllocator)
: d_readStatus(basicAllocator)
, d_writeStatus(basicAllocator)
, d_executeRet(0)
, d_closeSignaler(basicAllocator)
, d_watermarkSignaler(basicAllocator)
, d_readCalls(basicAllocator)
, d_writeCalls(basicAllocator)
, d_cancelReadCalls(basicAllocator)
, d_closeCalls(basicAllocator)
, d_executeCalls(basicAllocator)
, d_onCloseCalls(basicAllocator)
, d_onWatermarkCalls(basicAllocator)
, d_properties(basicAllocator)
, d_peerUri(basicAllocator)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

TestChannel::~TestChannel()
{
    // NOTHING
}

void TestChannel::reset()
{
    d_readStatus.reset();
    d_writeStatus.reset();
    d_executeRet = 0;
    d_closeSignaler.disconnectAllSlots();
    d_watermarkSignaler.disconnectAllSlots();
    d_readCalls.clear();
    d_writeCalls.clear();
    d_cancelReadCalls.clear();
    d_closeCalls.clear();
    d_executeCalls.clear();
    d_onCloseCalls.clear();
    d_onWatermarkCalls.clear();
    d_peerUri.clear();
}

void TestChannel::setReadStatus(const Status& status)
{
    d_readStatus = status;
}

void TestChannel::setWriteStatus(const Status& status)
{
    d_writeStatus = status;
}

void TestChannel::setExecuteRet(int ret)
{
    d_executeRet = ret;
}

void TestChannel::setPeerUri(const bslstl::StringRef& value)
{
    d_peerUri = value;
}

void TestChannel::read(Status*                   status,
                       int                       numBytes,
                       const ReadCallback&       readCallback,
                       const bsls::TimeInterval& timeout)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_readCalls.emplace_back(numBytes, readCallback, timeout);
    *status = d_readStatus;
}

void TestChannel::write(Status*            status,
                        const bdlbb::Blob& blob,
                        bsls::Types::Int64 watermark)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    d_writeCalls.emplace_back(blob, watermark);
    *status = d_writeStatus;

    d_condition.signal();
}

void TestChannel::cancelRead()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_cancelReadCalls.emplace_back();
}

void TestChannel::close(const Status& status)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_closeCalls.emplace_back(status);
}

int TestChannel::execute(const ExecuteCb& cb)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    d_executeCalls.emplace_back(cb);
    return d_executeRet;
}

bdlmt::SignalerConnection TestChannel::onClose(const CloseFn& cb)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    bdlmt::SignalerConnection conn = d_closeSignaler.connect(cb);
    d_onCloseCalls.emplace_back(cb, conn);
    return conn;
}

bdlmt::SignalerConnection TestChannel::onWatermark(const WatermarkFn& cb)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    bdlmt::SignalerConnection conn = d_watermarkSignaler.connect(cb);
    d_onWatermarkCalls.emplace_back(cb, conn);
    return conn;
}

bmqvt::PropertyBag& TestChannel::properties()
{
    return d_properties;
}

const bsl::string& TestChannel::peerUri() const
{
    return d_peerUri;
}

const bmqvt::PropertyBag& TestChannel::properties() const
{
    return d_properties;
}

bool TestChannel::waitFor(size_t                    size,
                          const bsls::TimeInterval& interval) const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    while (d_writeCalls.size() < size) {
        bsls::TimeInterval when = bsls::SystemTime::nowRealtimeClock() +
                                  interval;
        if (d_condition.timedWait(&d_mutex, when)) {
            return false;
        }
    }
    return true;
}

bool TestChannel::waitFor(const bdlbb::Blob&        blob,
                          const bsls::TimeInterval& interval,
                          bool                      pop)
{
    if (!waitFor(1, interval)) {
        return false;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    while (bdlbb::BlobUtil::compare(d_writeCalls.back().d_blob, blob)) {
        bsls::TimeInterval when = bsls::SystemTime::nowRealtimeClock() +
                                  interval;
        if (d_condition.timedWait(&d_mutex, when)) {
            return false;
        }
    }
    if (pop) {
        d_writeCalls.pop_back();
    }

    return true;
}

TestChannel::WriteCall TestChannel::popWriteCall()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    WriteCall                      result = d_writeCalls.front();
    d_writeCalls.pop_front();
    return result;
}

bool TestChannel::getWriteCall(TestChannel::WriteCall*   call,
                               size_t                    index,
                               const bsls::TimeInterval& interval) const
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(call);

    if (!waitFor(index + 1 /* size = index + 1 */, interval)) {
        return false;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    *call = d_writeCalls[index];
    return true;
}

TestChannel::CloseCall TestChannel::popCloseCall()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    CloseCall                      result = d_closeCalls.front();
    d_closeCalls.pop_front();
    return result;
}

TestChannel::OnCloseCall TestChannel::popOnCloseCall()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    OnCloseCall                    result = d_onCloseCalls.front();
    d_onCloseCalls.pop_front();
    return result;
}

TestChannel::CloseSignaler& TestChannel::closeSignaler()
{
    return d_closeSignaler;
}

TestChannel::WatermarkSignaler& TestChannel::watermarkSignaler()
{
    return d_watermarkSignaler;
}

// ACCESSORS
const Status& TestChannel::writeStatus() const
{
    return d_writeStatus;
}

size_t TestChannel::numWriteCalls() const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    return d_writeCalls.size();
}

size_t TestChannel::numCloseCalls() const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    return d_closeCalls.size();
}

size_t TestChannel::numOnCloseCalls() const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    return d_onCloseCalls.size();
}

}  // close package namespace
}  // close enterprise namespace
