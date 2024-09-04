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

// mwcio_testchannel.cpp                                              -*-C++-*-
#include <mwcio_testchannel.h>

#include <mwcscm_version.h>
// BDE
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>

namespace BloombergLP {
namespace mwcio {

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
, d_isFinal(false)
, d_hasNoMoreWriteCalls(true)
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

bsl::deque<TestChannel::ReadCall>& TestChannel::readCalls()
{
    return d_readCalls;
}

bsl::deque<TestChannel::WriteCall>& TestChannel::writeCalls()
{
    return d_writeCalls;
}

bsl::deque<TestChannel::CancelReadCall>& TestChannel::cancelReadCalls()
{
    return d_cancelReadCalls;
}

bsl::deque<TestChannel::CloseCall>& TestChannel::closeCalls()
{
    return d_closeCalls;
}

bsl::deque<TestChannel::ExecuteCall>& TestChannel::executeCalls()
{
    return d_executeCalls;
}

bsl::deque<TestChannel::OnCloseCall>& TestChannel::onCloseCalls()
{
    return d_onCloseCalls;
}

bsl::deque<TestChannel::OnWatermarkCall>& TestChannel::onWatermarkCalls()
{
    return d_onWatermarkCalls;
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

    if (d_isFinal) {
        d_hasNoMoreWriteCalls = false;
    }

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

mwct::PropertyBag& TestChannel::properties()
{
    return d_properties;
}

const bsl::string& TestChannel::peerUri() const
{
    return d_peerUri;
}

const mwct::PropertyBag& TestChannel::properties() const
{
    return d_properties;
}

bool TestChannel::waitFor(int                       size,
                          bool                      isFinal,
                          const bsls::TimeInterval& interval)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    while (writeCalls().size() < size_t(size)) {
        bsls::TimeInterval when = bsls::SystemTime::nowRealtimeClock() +
                                  interval;
        if (d_condition.timedWait(&d_mutex, when)) {
            return false;
        }
    }
    d_isFinal = isFinal;
    if (isFinal && writeCalls().size() > size_t(size)) {
        d_hasNoMoreWriteCalls = false;
    }

    return true;
}

bool TestChannel::waitFor(const bdlbb::Blob&        blob,
                          const bsls::TimeInterval& interval,
                          bool                      pop)
{
    if (!waitFor(1, false, interval)) {
        return false;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    while (
        bdlbb::BlobUtil::compare(writeCalls()[writeCalls().size() - 1].d_blob,
                                 blob)) {
        bsls::TimeInterval when = bsls::SystemTime::nowRealtimeClock() +
                                  interval;
        if (d_condition.timedWait(&d_mutex, when)) {
            return false;
        }
    }
    if (pop) {
        writeCalls().pop_back();
    }

    return true;
}

TestChannel::WriteCall TestChannel::popWriteCall()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    WriteCall                      result = writeCalls().front();

    writeCalls().pop_front();
    return result;
}

TestChannel::CloseCall TestChannel::popCloseCall()
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    CloseCall                      result = closeCalls().front();

    closeCalls().pop_front();
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

bool TestChannel::hasNoMoreWriteCalls() const
{
    return d_hasNoMoreWriteCalls;
}

bool TestChannel::closeCallsEmpty() const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    return d_closeCalls.empty();
}

}  // close package namespace
}  // close enterprise namespace
