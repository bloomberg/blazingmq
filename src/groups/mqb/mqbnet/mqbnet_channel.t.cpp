// Copyright 2023 Bloomberg Finance L.P.
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

// mqbnet_channel.t.cpp                                               -*-C++-*-
#include <mqbnet_channel.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_event.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqt_messageguid.h>

#include <bmqio_testchannel.h>
#include <bmqu_atomicstate.h>

// BDE
#include <bdlb_random.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_deque.h>
#include <bslmt_barrier.h>
#include <bslmt_readerwritermutex.h>
#include <bsls_annotation.h>
#include <bsls_atomic.h>
#include <bsls_platform.h>
#include <bsls_protocoltest.h>
#include <bsls_systemtime.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace BloombergLP {
namespace bmqio {

class TestChannelEx : public TestChannel {
  private:
    size_t                   d_limit;
    mqbnet::Channel&         d_channel;
    bool                     d_isInHWM;
    bslmt::ReaderWriterMutex d_mutex;
    bdlbb::Blob              d_eof;

  public:
    TestChannelEx(mqbnet::Channel&          channel,
                  bdlbb::BlobBufferFactory* factory,
                  bslma::Allocator*         basicAllocator);
    ~TestChannelEx() BSLS_KEYWORD_OVERRIDE;

    void write(bmqio::Status*     status,
               const bdlbb::Blob& blob,
               bsls::Types::Int64 watermark = bsl::numeric_limits<int>::max())
        BSLS_KEYWORD_OVERRIDE;

    void setWriteStatus(const bmqio::Status& status);
    void setLimit(size_t limit);
    bool waitForChannel(const bsls::TimeInterval& interval);
    void lowWatermark();
};

}
}

const char   k_CONTENT[]   = "Being is always the Being of a being";
const size_t k_BUFFER_SIZE = sizeof(k_CONTENT) * 100;

struct PseudoBuilder {
    bdlbb::Blob d_payload;
    PseudoBuilder(bdlbb::PooledBlobBufferFactory* bufferFactory,
                  bslma::Allocator*               allocator_p)
    : d_payload(bufferFactory, allocator_p)
    {
        // NOTHING
    }
    int  messageCount() const { return d_payload.length() ? 1 : 0; }
    void reset() { d_payload.removeAll(); }
    const bdlbb::Blob& blob() const { return d_payload; }
};
template <class Builder>
struct Iterator {
    typedef void Type;
    static void  load(Type* iterator, bmqp::Event& event);
    // static bool isEqual(const Type& o1, const Type& o2);
};

template <class Builder>
class Tester {
  private:
    Builder                         d_builder;
    bdlbb::PooledBlobBufferFactory& d_bufferFactory;
    mqbnet::Channel&                d_channel;
    bsl::deque<bdlbb::Blob>         d_history;
    bslmt::ThreadUtil::Handle       d_threadHandle;
    bsls::AtomicBool                d_stop;
    bslma::Allocator*               d_allocator_p;

  private:
    bmqt::EventBuilderResult::Enum build();
    void threadFn(bslmt::Barrier* phase1, bslmt::Barrier* phase2);

  public:
    Tester();

    Tester(mqbnet::Channel&                channel,
           bdlbb::PooledBlobBufferFactory& bufferFactory,
           bslma::Allocator*               allocator_p);

    void   test();
    size_t verify(const bsl::shared_ptr<bmqio::TestChannelEx>& testChannel);

    void createThread(bslmt::Barrier* phase1, bslmt::Barrier* phase2);
    void stop();
    void join();
};

template <>
struct Iterator<bmqp::PutEventBuilder> : public bmqp::PutMessageIterator {
    bslma::Allocator* d_allocator_p;

    Iterator(bdlbb::BlobBufferFactory* bufferFactory,
             bslma::Allocator*         allocator)
    : bmqp::PutMessageIterator(bufferFactory, allocator)
    , d_allocator_p(allocator)
    {
        // NOTHING
    }
    void load(bmqp::Event& event)
    {
        event.loadPutMessageIterator(this, false);
    }

    bool isEqual(const Iterator<bmqp::PutEventBuilder>& other) const
    {
        // mqbnet::Channel packs raw
        const_cast<bmqp::PutHeader&>(header()).setCrc32c(0);

        bdlbb::Blob blob(d_allocator_p);
        bdlbb::Blob otherBlob(d_allocator_p);
        loadApplicationData(&blob);
        other.loadApplicationData(&otherBlob);

        ASSERT_EQ(header().queueId(), other.header().queueId());

        return memcmp(&header(), &other.header(), sizeof(header())) == 0 &&
               bdlbb::BlobUtil::compare(blob, otherBlob) == 0;
    }
};

template <>
struct Iterator<bmqp::PushEventBuilder> : bmqp::PushMessageIterator {
    Iterator(bdlbb::BlobBufferFactory* bufferFactory,
             bslma::Allocator*         allocator)
    : bmqp::PushMessageIterator(bufferFactory, allocator)
    {
        // NOTHING
    }
    void load(bmqp::Event& event)
    {
        event.loadPushMessageIterator(this, false);
    }
    bool isEqual(const Iterator<bmqp::PushEventBuilder>& other)
    {
        return memcmp(&header(), &other.header(), sizeof(header())) == 0;
    }
};

template <>
struct Iterator<bmqp::AckEventBuilder> : bmqp::AckMessageIterator {
    Iterator(BSLS_ANNOTATION_UNUSED bdlbb::BlobBufferFactory* bufferFactory,
             BSLS_ANNOTATION_UNUSED bslma::Allocator* allocator)
    {
        // NOTHING
    }
    void load(bmqp::Event& event) { event.loadAckMessageIterator(this); }
    bool isEqual(const Iterator<bmqp::AckEventBuilder>& other)
    {
        return memcmp(&header(), &other.header(), sizeof(header())) == 0;
    }
};

template <>
struct Iterator<bmqp::ConfirmEventBuilder> : bmqp::ConfirmMessageIterator {
    Iterator(BSLS_ANNOTATION_UNUSED bdlbb::BlobBufferFactory* bufferFactory,
             BSLS_ANNOTATION_UNUSED bslma::Allocator* allocator)
    {
        // NOTHING
    }
    void load(bmqp::Event& event) { event.loadConfirmMessageIterator(this); }
    bool isEqual(const Iterator<bmqp::ConfirmEventBuilder>& other)
    {
        return memcmp(&header(), &other.header(), sizeof(header())) == 0;
    }
};

template <>
struct Iterator<bmqp::RejectEventBuilder> : bmqp::RejectMessageIterator {
    Iterator(BSLS_ANNOTATION_UNUSED bdlbb::BlobBufferFactory* bufferFactory,
             BSLS_ANNOTATION_UNUSED bslma::Allocator* allocator)
    {
        // NOTHING
    }
    void load(bmqp::Event& event) { event.loadRejectMessageIterator(this); }
    bool isEqual(const Iterator<bmqp::RejectEventBuilder>& other)
    {
        return memcmp(&header(), &other.header(), sizeof(header())) == 0;
    }
};

template <>
struct Iterator<PseudoBuilder> {
    const bdlbb::Blob* d_blob;
    int                d_next;

    Iterator(BSLS_ANNOTATION_UNUSED bdlbb::BlobBufferFactory* bufferFactory,
             BSLS_ANNOTATION_UNUSED bslma::Allocator* allocator)
    : d_blob(0)
    , d_next(0)
    {
        // NOTHING
    }
    void load(bmqp::Event& event)
    {
        d_blob = event.blob();
        d_next = 1;
    }
    bool isValid() const { return d_blob; }
    int  next()
    {
        int next = d_next;
        d_next   = 0;
        return next;
    }

    bool isEqual(const Iterator<PseudoBuilder>& other) const
    {
        BSLS_ASSERT_OPT(d_blob);
        BSLS_ASSERT_OPT(other.d_blob);

        return bdlbb::BlobUtil::compare(*d_blob, *other.d_blob) == 0;
    }
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline void setContent(bdlbb::BlobBuffer* buffer)
{
    static bsls::AtomicInt s_seed(0x01020304);
    int                    seed = s_seed;
    int i1 = bdlb::Random::generate15(&seed) % sizeof(k_CONTENT);
    int i2 = bdlb::Random::generate15(&seed) % sizeof(k_CONTENT);

    s_seed += seed;

    buffer->setSize(k_BUFFER_SIZE);
    for (size_t length = 0; length < k_BUFFER_SIZE;
         length += sizeof(k_CONTENT)) {
        bsl::memcpy(buffer->data() + length, k_CONTENT, sizeof(k_CONTENT));
    }

    const char temp    = buffer->data()[i2];
    buffer->data()[i2] = buffer->data()[i1];
    buffer->data()[i1] = temp;
}

// -----------------
// class TestChannel
// -----------------

namespace BloombergLP {
namespace bmqio {

TestChannelEx::TestChannelEx(mqbnet::Channel&          channel,
                             bdlbb::BlobBufferFactory* factory,
                             bslma::Allocator*         basicAllocator)
: TestChannel(basicAllocator)
, d_limit(0)
, d_channel(channel)
, d_isInHWM(false)
, d_eof(factory, basicAllocator)
{
    static const char signature[] = "12345";
    bdlbb::BlobBuffer blobBuffer;
    factory->allocate(&blobBuffer);

    blobBuffer.setSize(sizeof(signature));

    bsl::memcpy(blobBuffer.data(), signature, sizeof(signature));

    d_eof.appendDataBuffer(blobBuffer);
}

TestChannelEx::~TestChannelEx()
{
    // NOTHING
}

void TestChannelEx::setWriteStatus(const bmqio::Status& status)
{
    bslmt::WriteLockGuard<bslmt::ReaderWriterMutex> guard(&d_mutex);
    // WRITE-LOCK
    TestChannel::setWriteStatus(status);
}

void TestChannelEx::setLimit(size_t limit)
{
    bslmt::WriteLockGuard<bslmt::ReaderWriterMutex> guard(&d_mutex);
    // WRITE-LOCK

    if (d_isInHWM) {
        if (limit == 0 || writeCalls().size() < limit) {
            d_isInHWM = false;
            d_channel.onWatermark(
                bmqio::ChannelWatermarkType::e_LOW_WATERMARK);
        }
    }
    else if (writeCalls().size() >= limit) {
        d_isInHWM = true;
        d_channel.onWatermark(bmqio::ChannelWatermarkType::e_HIGH_WATERMARK);
    }
    d_limit = limit;
}

void TestChannelEx::lowWatermark()
{
    bslmt::WriteLockGuard<bslmt::ReaderWriterMutex> guard(&d_mutex);
    // WRITE-LOCK

    if (d_isInHWM) {
        d_isInHWM = false;
        d_channel.onWatermark(bmqio::ChannelWatermarkType::e_LOW_WATERMARK);
    }
}

void TestChannelEx::write(bmqio::Status*     status,
                          const bdlbb::Blob& blob,
                          bsls::Types::Int64 watermark)
{
    bslmt::ReadLockGuard<bslmt::ReaderWriterMutex> guard(
        &d_mutex);  // READ-LOCK

    if (writeStatus().category() != bmqio::StatusCategory::e_SUCCESS) {
        *status = writeStatus();
        return;  // RETURN
    }

    if (d_isInHWM) {
        status->setCategory(bmqio::StatusCategory::e_LIMIT);
        return;  // RETURN
    }

    if (d_limit && writeCalls().size() >= d_limit) {
        d_isInHWM = true;
        status->setCategory(bmqio::StatusCategory::e_LIMIT);
        d_channel.onWatermark(bmqio::ChannelWatermarkType::e_HIGH_WATERMARK);
    }

    TestChannel::write(status, blob, watermark);
}

bool TestChannelEx::waitForChannel(const bsls::TimeInterval& interval)
{
    ASSERT_EQ(d_channel.writeBlob(d_eof, bmqp::EventType::e_CONTROL),
              bmqt::GenericResult::e_SUCCESS);

    return waitFor(d_eof, interval);
}

}
}

// --------------------
// class ester<Builder>
// --------------------

template <class Builder>
inline Tester<Builder>::Tester(mqbnet::Channel&                channel,
                               bdlbb::PooledBlobBufferFactory& bufferFactory,
                               bslma::Allocator*               allocator_p)
: d_builder(&bufferFactory, allocator_p)
, d_bufferFactory(bufferFactory)
, d_channel(channel)
, d_history(allocator_p)
, d_threadHandle()
, d_stop(false)
, d_allocator_p(allocator_p)
{
}

template <class Builder>
inline void Tester<Builder>::test()
{
    bmqt::EventBuilderResult::Enum rc = build();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqt::EventBuilderResult::e_EVENT_TOO_BIG == rc ||
            bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG == rc ||
            bmqt::EventBuilderResult::e_OPTION_TOO_BIG == rc)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        d_history.push_back(d_builder.blob());
        d_builder.reset();

        rc = build();
    }
    BSLS_ASSERT_OPT(rc == bmqt::EventBuilderResult::e_SUCCESS);
}

template <>
inline bmqt::EventBuilderResult::Enum Tester<bmqp::PutEventBuilder>::build()
{
    static int                   id      = 0;
    static int                   queueId = 0;
    bmqp::PutHeader              ph;
    const int                    flags = 0;
    bsl::shared_ptr<bdlbb::Blob> payload(
        new (*d_allocator_p) bdlbb::Blob(&d_bufferFactory, d_allocator_p),
        d_allocator_p);
    bdlbb::BlobBuffer                  blobBuffer;
    bsl::shared_ptr<bmqu::AtomicState> state(new (*d_allocator_p)
                                                 bmqu::AtomicState,
                                             d_allocator_p);

    d_bufferFactory.allocate(&blobBuffer);

    setContent(&blobBuffer);

    payload->appendDataBuffer(blobBuffer);

    ph.setCorrelationId(++id);
    ph.setMessageGUID(bmqp::MessageGUIDGenerator::testGUID());
    ph.setFlags(flags);
    ph.setQueueId(++queueId);
    ph.setCrc32c(0);

    d_builder.startMessage();

    d_builder.setMessageGUID(ph.messageGUID())
        .setFlags(ph.flags())
        .setMessagePayload(payload.get())
        .setCompressionAlgorithmType(ph.compressionAlgorithmType())
        .setCrc32c(ph.crc32c());

    bmqt::EventBuilderResult::Enum rc = d_builder.packMessage(queueId);

    if (rc == bmqt::EventBuilderResult::e_SUCCESS) {
        d_channel.writePut(ph, payload, state);
    }
    return rc;
}

template <>
inline bmqt::EventBuilderResult::Enum Tester<bmqp::PushEventBuilder>::build()
{
    static int        queueId = 0;
    const int         flags   = 0;
    bmqt::MessageGUID guid    = bmqp::MessageGUIDGenerator::testGUID();

    bmqt::EventBuilderResult::Enum rc;
    static int                     flip = 0;

    bmqp::Protocol::SubQueueInfosArray subQueueInfos(d_allocator_p);
    for (unsigned int subQueueId = 0; subQueueId < 10; ++subQueueId) {
        subQueueInfos.push_back(bmqp::SubQueueInfo(subQueueId));
    }
    rc = d_builder.addSubQueueInfosOption(subQueueInfos);

    if (++flip & 1) {
        if (rc == bmqt::EventBuilderResult::e_SUCCESS) {
            rc = d_builder.packMessage(queueId,
                                       guid,
                                       flags,
                                       bmqt::CompressionAlgorithmType::e_NONE);
            if (rc == bmqt::EventBuilderResult::e_SUCCESS) {
                d_channel.writePush(queueId,
                                    guid,
                                    flags,
                                    bmqt::CompressionAlgorithmType::e_NONE,
                                    bmqp::MessagePropertiesInfo(),
                                    subQueueInfos);
            }
        }
    }
    else {
        bsl::shared_ptr<bdlbb::Blob> payload(
            new (*d_allocator_p) bdlbb::Blob(&d_bufferFactory, d_allocator_p),
            d_allocator_p);
        bdlbb::BlobBuffer blobBuffer;

        d_bufferFactory.allocate(&blobBuffer);

        setContent(&blobBuffer);

        payload->appendDataBuffer(blobBuffer);

        rc = d_builder.packMessage(*payload,
                                   queueId,
                                   guid,
                                   flags,
                                   bmqt::CompressionAlgorithmType::e_NONE);
        if (rc == bmqt::EventBuilderResult::e_SUCCESS) {
            d_channel.writePush(payload,
                                queueId,
                                guid,
                                flags,
                                bmqt::CompressionAlgorithmType::e_NONE,
                                bmqp::MessagePropertiesInfo(),
                                subQueueInfos);
        }
    }

    return rc;
}

template <>
inline bmqt::EventBuilderResult::Enum
Tester<bmqp::ConfirmEventBuilder>::build()
{
    static int        queueId    = 0;
    const int         subQueueId = 0;
    bmqt::MessageGUID guid       = bmqp::MessageGUIDGenerator::testGUID();

    bmqt::EventBuilderResult::Enum rc;

    rc = d_builder.appendMessage(queueId, subQueueId, guid);

    if (rc == bmqt::EventBuilderResult::e_SUCCESS) {
        d_channel.writeConfirm(queueId, subQueueId, guid);
    }

    return rc;
}

template <>
inline bmqt::EventBuilderResult::Enum Tester<bmqp::RejectEventBuilder>::build()
{
    static int        queueId    = 0;
    const int         subQueueId = 0;
    bmqt::MessageGUID guid       = bmqp::MessageGUIDGenerator::testGUID();

    bmqt::EventBuilderResult::Enum rc;

    rc = d_builder.appendMessage(queueId, subQueueId, guid);

    if (rc == bmqt::EventBuilderResult::e_SUCCESS) {
        d_channel.writeReject(queueId, subQueueId, guid);
    }

    return rc;
}

template <>
inline bmqt::EventBuilderResult::Enum Tester<bmqp::AckEventBuilder>::build()
{
    static int        id      = 0;
    static int        queueId = 0;
    const int         status  = 0;
    bmqt::MessageGUID guid    = bmqp::MessageGUIDGenerator::testGUID();

    bmqt::EventBuilderResult::Enum rc;

    rc = d_builder.appendMessage(status, ++id, guid, queueId);

    if (rc == bmqt::EventBuilderResult::e_SUCCESS) {
        d_channel.writeAck(status, id, guid, queueId);
    }

    return rc;
}

template <>
inline bmqt::EventBuilderResult::Enum Tester<PseudoBuilder>::build()
{
    d_builder.d_payload.setLength(sizeof(bmqp::EventHeader));

    bmqp::EventHeader* eventHeader = new (d_builder.d_payload.buffer(0).data())
        bmqp::EventHeader(bmqp::EventType::e_CONTROL);

    bdlbb::BlobBuffer blobBuffer;

    d_bufferFactory.allocate(&blobBuffer);
    setContent(&blobBuffer);
    d_builder.d_payload.appendDataBuffer(blobBuffer);

    eventHeader->setLength(d_builder.d_payload.length());

    d_channel.writeBlob(d_builder.d_payload, bmqp::EventType::e_CONTROL);

    // never return e_EVENT_TOO_BIG
    d_history.push_back(d_builder.d_payload);
    d_builder.reset();

    return bmqt::EventBuilderResult::e_SUCCESS;
}

template <class Builder>
inline size_t Tester<Builder>::verify(
    const bsl::shared_ptr<bmqio::TestChannelEx>& testChannel)
{
    if (d_builder.messageCount()) {
        d_history.push_back(d_builder.blob());
        d_builder.reset();
    }

    typedef bsl::deque<bmqio::TestChannel::WriteCall>::const_iterator Writes;

    Writes            writes = testChannel->writeCalls().begin();
    Iterator<Builder> itEvents(&d_bufferFactory, d_allocator_p);
    size_t            counter    = 0;
    size_t            writeBlobs = 0;

    for (bsl::deque<bdlbb::Blob>::iterator itHistory = d_history.begin();
         itHistory != d_history.end();
         ++itHistory) {
        const bdlbb::Blob& blob = *itHistory;
        bmqp::Event        eventHistory(&blob, d_allocator_p);
        Iterator<Builder>  itHistoryEvents(&d_bufferFactory, d_allocator_p);

        itHistoryEvents.load(eventHistory);

        ASSERT(itHistoryEvents.isValid());

        while (itHistoryEvents.next() == 1) {
            if (itEvents.next() != 1) {
                bool isFound = false;

                for (; writes != testChannel->writeCalls().end() && !isFound;
                     ++writes) {
                    // This assumes that the scope of iterator can be greater
                    // than the scope of the event
                    bmqp::Event event(&writes->d_blob, d_allocator_p);

                    if (event.type() == eventHistory.type()) {
                        itEvents.load(event);
                        ASSERT_EQ(itEvents.next(), 1);

                        ++writeBlobs;
                        isFound = true;
                    }
                }
                ASSERT_EQ_D("# " << counter, isFound, true);
            }

            ASSERT_EQ_D("# " << counter, itEvents.isValid(), true);
            ASSERT_EQ_D("# " << counter,
                        itHistoryEvents.isEqual(itEvents),
                        true);
            ++counter;
        }
    }
    return writeBlobs;
}

template <class Builder>
void Tester<Builder>::createThread(bslmt::Barrier* phase1,
                                   bslmt::Barrier* phase2)
{
    bslmt::ThreadUtil::createWithAllocator(
        &d_threadHandle,
        bdlf::BindUtil::bind(&Tester<Builder>::threadFn, this, phase1, phase2),
        d_allocator_p);
}

template <class Builder>
void Tester<Builder>::stop()
{
    d_stop = true;
}

template <class Builder>
void Tester<Builder>::join()
{
    bslmt::ThreadUtil::join(d_threadHandle);
}

template <class Builder>
void Tester<Builder>::threadFn(bslmt::Barrier* phase1, bslmt::Barrier* phase2)
{
    d_stop = false;

    phase1->wait();
    size_t i = 0;
    for (; i < 3000 || !d_stop; ++i) {
        test();
    }
    d_stop = false;

    phase2->wait();

    for (i = 0; i < 3000 || !d_stop; ++i) {
        test();
    }
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_write()
// ------------------------------------------------------------------------
//
// Call writePut, writePush, writeAck, writeConfirm repeatedly and verify
// that the mqbnet::Channel output is identical to corresponding builders
// output.
//
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(k_BUFFER_SIZE, s_allocator_p);
    mqbnet::Channel channel(&bufferFactory, "test", s_allocator_p);

    bsl::shared_ptr<bmqio::TestChannelEx> testChannel(
        new (*s_allocator_p)
            bmqio::TestChannelEx(channel, &bufferFactory, s_allocator_p),
        s_allocator_p);

    Tester<bmqp::PutEventBuilder>  put(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::PushEventBuilder> push(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::AckEventBuilder>  ack(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::ConfirmEventBuilder> confirm(channel,
                                              bufferFactory,
                                              s_allocator_p);
    Tester<bmqp::RejectEventBuilder>  reject(channel,
                                            bufferFactory,
                                            s_allocator_p);

    channel.setChannel(bsl::weak_ptr<bmqio::TestChannel>(testChannel));

    for (size_t i = 0; i < 5000; i++) {
        put.test();
        push.test();
        ack.test();
        confirm.test();
        reject.test();
    }

    testChannel->setWriteStatus(bmqio::StatusCategory::e_LIMIT);

    for (size_t i = 0; i < 5000; i++) {
        put.test();
        push.test();
        ack.test();
        confirm.test();
        reject.test();
    }

    testChannel->setWriteStatus(bmqio::StatusCategory::e_SUCCESS);
    channel.onWatermark(bmqio::ChannelWatermarkType::e_LOW_WATERMARK);

    size_t writeBlobs = 0;

    // Flush ACKs which are secondary
    channel.flush();

    ASSERT_EQ(testChannel->waitForChannel(bsls::TimeInterval(3)), true);

    writeBlobs += put.verify(testChannel);
    writeBlobs += push.verify(testChannel);
    writeBlobs += ack.verify(testChannel);
    writeBlobs += confirm.verify(testChannel);
    writeBlobs += reject.verify(testChannel);

    ASSERT_EQ(testChannel->writeCalls().size(), writeBlobs);
}

static void test2_highWatermark()
// ------------------------------------------------------------------------
//
// Concurrently call writePut, writePush, writeAck, writeConfirm from
// different threads.  Simulate HWM half way.  Verify that the
// mqbnet::Channel output is identical to corresponding builders output.
//
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(k_BUFFER_SIZE, s_allocator_p);
    mqbnet::Channel channel(&bufferFactory, "test", s_allocator_p);

    bsl::shared_ptr<bmqio::TestChannelEx> testChannel(
        new (*s_allocator_p)
            bmqio::TestChannelEx(channel, &bufferFactory, s_allocator_p),
        s_allocator_p);

    Tester<bmqp::PutEventBuilder>  put(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::PushEventBuilder> push(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::AckEventBuilder>  ack(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::ConfirmEventBuilder> confirm(channel,
                                              bufferFactory,
                                              s_allocator_p);
    Tester<PseudoBuilder> control(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::RejectEventBuilder> reject(channel,
                                            bufferFactory,
                                            s_allocator_p);

    bslmt::Barrier phase1(6 + 1);
    bslmt::Barrier phase2(6 + 1);

    channel.setChannel(bsl::weak_ptr<bmqio::TestChannel>(testChannel));

    confirm.createThread(&phase1, &phase2);
    put.createThread(&phase1, &phase2);
    ack.createThread(&phase1, &phase2);
    push.createThread(&phase1, &phase2);
    control.createThread(&phase1, &phase2);
    reject.createThread(&phase1, &phase2);

    phase1.wait();
    // start concurrently writing in LWM

    confirm.stop();
    put.stop();
    ack.stop();
    push.stop();
    control.stop();
    reject.stop();

    testChannel->setWriteStatus(bmqio::StatusCategory::e_LIMIT);
    channel.onWatermark(bmqio::ChannelWatermarkType::e_HIGH_WATERMARK);

    phase2.wait();

    confirm.stop();
    put.stop();
    ack.stop();
    push.stop();
    control.stop();
    reject.stop();

    confirm.join();
    put.join();
    ack.join();
    push.join();
    control.join();
    reject.join();

    testChannel->setWriteStatus(bmqio::StatusCategory::e_SUCCESS);
    channel.onWatermark(bmqio::ChannelWatermarkType::e_LOW_WATERMARK);

    size_t writeBlobs = 0;

    // Flush ACKs which are secondary
    channel.flush();

    ASSERT_EQ(testChannel->waitForChannel(bsls::TimeInterval(1)), true);

    writeBlobs += put.verify(testChannel);
    writeBlobs += push.verify(testChannel);
    writeBlobs += ack.verify(testChannel);
    writeBlobs += confirm.verify(testChannel);
    writeBlobs += control.verify(testChannel);
    writeBlobs += reject.verify(testChannel);

    ASSERT_EQ(testChannel->writeCalls().size(), writeBlobs);
}

static void test3_highWatermarkInWriteCb()
// ------------------------------------------------------------------------
//
// Concurrently call writePut, writePush, writeAck, writeConfirm from
// different threads.  Simulate HWM while writing and while processing LWM.
// Verify that the mqbnet::Channel output is identical to corresponding
// builders output.
//
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(k_BUFFER_SIZE, s_allocator_p);
    mqbnet::Channel channel(&bufferFactory, "test", s_allocator_p);

    bsl::shared_ptr<bmqio::TestChannelEx> testChannel(
        new (*s_allocator_p)
            bmqio::TestChannelEx(channel, &bufferFactory, s_allocator_p),
        s_allocator_p);

    Tester<bmqp::PutEventBuilder>  put(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::PushEventBuilder> push(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::AckEventBuilder>  ack(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::ConfirmEventBuilder> confirm(channel,
                                              bufferFactory,
                                              s_allocator_p);
    Tester<bmqp::RejectEventBuilder>  reject(channel,
                                            bufferFactory,
                                            s_allocator_p);

    bslmt::Barrier phase1(5 + 1);
    bslmt::Barrier phase2(5 + 1);

    channel.setChannel(bsl::weak_ptr<bmqio::TestChannelEx>(testChannel));

    confirm.createThread(&phase1, &phase2);
    put.createThread(&phase1, &phase2);
    ack.createThread(&phase1, &phase2);
    push.createThread(&phase1, &phase2);
    reject.createThread(&phase1, &phase2);

    testChannel->setLimit(1);
    // trigger HWM after 1 message

    phase1.wait();
    // start concurrently writing in LWM

    confirm.stop();
    put.stop();
    ack.stop();
    push.stop();
    reject.stop();

    phase2.wait();

    // Wait for at least 2 'write' calls (the second triggers HWM)
    ASSERT_EQ(testChannel->waitFor(2, false, bsls::TimeInterval(3)), true);

    // trigger LWM during which the limit gets hit and trigger HWM
    testChannel->lowWatermark();

    // Wait for at least 1 'write' call to trigger HWM
    ASSERT_EQ(testChannel->waitFor(3, false, bsls::TimeInterval(3)), true);

    confirm.stop();
    put.stop();
    ack.stop();
    push.stop();
    reject.stop();

    confirm.join();
    put.join();
    ack.join();
    push.join();
    reject.join();

    testChannel->setLimit(0);

    // Flush ACKs which are secondary
    channel.flush();
    ASSERT_EQ(testChannel->waitForChannel(bsls::TimeInterval(10)), true);

    size_t writeBlobs = 0;
    writeBlobs += put.verify(testChannel);
    writeBlobs += push.verify(testChannel);
    writeBlobs += ack.verify(testChannel);
    writeBlobs += confirm.verify(testChannel);
    writeBlobs += reject.verify(testChannel);

    ASSERT_EQ(testChannel->writeCalls().size(), writeBlobs);
}

static void test4_controlBlob()
// ------------------------------------------------------------------------
//
// Call writePut, writePush, writeAck, writeConfirm once just to touch the
// builders.  Call writeBlob.  Verify that the mqbnet::Channel output is
// identical to corresponding builders output and that the last write
// flushes all previous ones.
//
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(k_BUFFER_SIZE, s_allocator_p);
    mqbnet::Channel channel(&bufferFactory, "test", s_allocator_p);

    bsl::shared_ptr<bmqio::TestChannelEx> testChannel(
        new (*s_allocator_p)
            bmqio::TestChannelEx(channel, &bufferFactory, s_allocator_p),
        s_allocator_p);

    Tester<bmqp::PutEventBuilder>  put(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::PushEventBuilder> push(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::AckEventBuilder>  ack(channel, bufferFactory, s_allocator_p);
    Tester<bmqp::ConfirmEventBuilder> confirm(channel,
                                              bufferFactory,
                                              s_allocator_p);
    Tester<bmqp::RejectEventBuilder>  reject(channel,
                                            bufferFactory,
                                            s_allocator_p);

    channel.setChannel(bsl::weak_ptr<bmqio::TestChannelEx>(testChannel));

    put.test();
    push.test();
    ack.test();
    confirm.test();
    reject.test();

    // cannot assert 'writeCalls().size() == 0' because of auto-flushing

    bdlbb::Blob       payload = bdlbb::Blob(&bufferFactory, s_allocator_p);
    bdlbb::BlobBuffer blobBuffer;

    bufferFactory.allocate(&blobBuffer);
    bsl::memset(blobBuffer.data(), 0, blobBuffer.size());

    payload.appendDataBuffer(blobBuffer);

    // Flush ACKs which are secondary
    channel.flush();

    ASSERT_EQ(channel.writeBlob(payload, bmqp::EventType::e_CONTROL),
              bmqt::GenericResult::e_SUCCESS);

    ASSERT_EQ(testChannel->waitForChannel(bsls::TimeInterval(1)), true);

    size_t writeBlobs = 0;

    writeBlobs += put.verify(testChannel);
    writeBlobs += push.verify(testChannel);
    writeBlobs += ack.verify(testChannel);
    writeBlobs += confirm.verify(testChannel);
    writeBlobs += reject.verify(testChannel);

    ASSERT_EQ(testChannel->writeCalls().size(), writeBlobs + 1);

    const bdlbb::Blob& lastWrite = (--testChannel->writeCalls().end())->d_blob;

    // make sure the control is the last
    ASSERT_EQ(bdlbb::BlobUtil::compare(payload, lastWrite), 0);
}

static void test5_reconnect()
// ------------------------------------------------------------------------
//
// Call writeBlob, simulate disconnect, call writeBlob, verify return code,
// simulate connection, call writeBlob.  Verify that the mqbnet::Channel
// output is the 1st and the 3rd blobs.
//
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(k_BUFFER_SIZE, s_allocator_p);
    mqbnet::Channel channel(&bufferFactory, "test", s_allocator_p);

    bsl::shared_ptr<bmqio::TestChannelEx> testChannel(
        new (*s_allocator_p)
            bmqio::TestChannelEx(channel, &bufferFactory, s_allocator_p),
        s_allocator_p);

    channel.setChannel(bsl::weak_ptr<bmqio::TestChannelEx>(testChannel));

    {
        bdlbb::Blob       payload = bdlbb::Blob(&bufferFactory, s_allocator_p);
        bdlbb::BlobBuffer blobBuffer;

        bufferFactory.allocate(&blobBuffer);
        setContent(&blobBuffer);
        payload.appendDataBuffer(blobBuffer);

        ASSERT_EQ(channel.writeBlob(payload, bmqp::EventType::e_CONTROL),
                  bmqt::GenericResult::e_SUCCESS);

        ASSERT_EQ(testChannel->waitForChannel(bsls::TimeInterval(1)), true);
        const bdlbb::Blob& write = testChannel->writeCalls().begin()->d_blob;

        ASSERT_EQ(bdlbb::BlobUtil::compare(payload, write), 0);
    }
    ASSERT_EQ(testChannel->writeCalls().size(), 1U);

    testChannel->setWriteStatus(bmqio::StatusCategory::e_CONNECTION);

    {
        bdlbb::Blob       payload = bdlbb::Blob(&bufferFactory, s_allocator_p);
        bdlbb::BlobBuffer blobBuffer;

        bufferFactory.allocate(&blobBuffer);
        setContent(&blobBuffer);
        payload.appendDataBuffer(blobBuffer);

        ASSERT_EQ(channel.writeBlob(payload, bmqp::EventType::e_CONTROL),
                  bmqt::GenericResult::e_SUCCESS);
    }
    ASSERT_EQ(testChannel->writeCalls().size(), 1U);

    // simulate reconnection
    channel.resetChannel();
    channel.setChannel(bsl::weak_ptr<bmqio::TestChannelEx>(testChannel));

    testChannel->setWriteStatus(bmqio::StatusCategory::e_SUCCESS);

    {
        bdlbb::Blob       payload = bdlbb::Blob(&bufferFactory, s_allocator_p);
        bdlbb::BlobBuffer blobBuffer;

        bufferFactory.allocate(&blobBuffer);
        setContent(&blobBuffer);
        payload.appendDataBuffer(blobBuffer);

        ASSERT_EQ(channel.writeBlob(payload, bmqp::EventType::e_CONTROL),
                  bmqt::GenericResult::e_SUCCESS);

        ASSERT_EQ(testChannel->waitForChannel(bsls::TimeInterval(1)), true);
        const bdlbb::Blob& write =
            (++testChannel->writeCalls().begin())->d_blob;

        ASSERT_EQ(bdlbb::BlobUtil::compare(payload, write), 0);
    }

    ASSERT_EQ(testChannel->writeCalls().size(), 2U);
}

static void test6_weakData()
// ------------------------------------------------------------------------
//
// Call writePut which takes weak_ptr under HWM causing the channel to
// buffer data.  Release the data, simulate LWM, and observe negative
// return code,
//
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(k_BUFFER_SIZE, s_allocator_p);
    mqbnet::Channel channel(&bufferFactory, "test", s_allocator_p);

    bsl::shared_ptr<bmqio::TestChannelEx> testChannel(
        new (*s_allocator_p)
            bmqio::TestChannelEx(channel, &bufferFactory, s_allocator_p),
        s_allocator_p);

    channel.setChannel(bsl::weak_ptr<bmqio::TestChannelEx>(testChannel));

    // Saturate the channel causing it to buffer next write
    channel.onWatermark(bmqio::ChannelWatermarkType::e_HIGH_WATERMARK);

    {
        bsl::shared_ptr<bdlbb::Blob> payload(
            new (*s_allocator_p) bdlbb::Blob(&bufferFactory, s_allocator_p),
            s_allocator_p);
        bdlbb::BlobBuffer                  blobBuffer;
        bsl::shared_ptr<bmqu::AtomicState> state(new (*s_allocator_p)
                                                     bmqu::AtomicState,
                                                 s_allocator_p);
        bmqp::PutHeader                    ph;

        bufferFactory.allocate(&blobBuffer);

        payload->appendDataBuffer(blobBuffer);
        ph.setMessageGUID(bmqp::MessageGUIDGenerator::testGUID());

        // This write ends up in the builder and cannot be canceled.
        ASSERT_EQ(channel.writePut(ph, payload, state, false),
                  bmqt::GenericResult::e_SUCCESS);
        // After 'onWatermark', the channel starts buffering.
    }
    ASSERT_EQ(testChannel->writeCalls().size(), 0U);

    // Next write
    {
        bsl::shared_ptr<bdlbb::Blob> payload(
            new (*s_allocator_p) bdlbb::Blob(&bufferFactory, s_allocator_p),
            s_allocator_p);
        bdlbb::BlobBuffer                  blobBuffer;
        bsl::shared_ptr<bmqu::AtomicState> state(new (*s_allocator_p)
                                                     bmqu::AtomicState,
                                                 s_allocator_p);
        bmqp::PutHeader                    ph;

        bufferFactory.allocate(&blobBuffer);

        payload->appendDataBuffer(blobBuffer);
        ph.setMessageGUID(bmqp::MessageGUIDGenerator::testGUID());

        ASSERT_EQ(channel.writePut(ph, payload, state, true),
                  bmqt::GenericResult::e_SUCCESS);
    }

    channel.onWatermark(bmqio::ChannelWatermarkType::e_LOW_WATERMARK);

    ASSERT_EQ(testChannel->waitForChannel(bsls::TimeInterval(1)), true);

    // Only the first write makes it to IO.
    ASSERT_EQ(testChannel->writeCalls().size(), 1U);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Initialize Crc32c
    bmqp::Crc32c::initialize();

    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(s_allocator_p);
    // expect BALL_LOG_ERROR
    switch (_testCase) {
    case 0:
    case 1: test1_write(); break;
    case 2: test2_highWatermark(); break;
    case 3: test3_highWatermarkInWriteCb(); break;
    case 4: test4_controlBlob(); break;
    case 5: test5_reconnect(); break;
    case 6: test6_weakData(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
