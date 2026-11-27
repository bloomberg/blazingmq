// Copyright 2026 Bloomberg Finance L.P.
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

// bmqp_fixedsizemessageiterator.t.cpp                                -*-C++-*-
#include <bmqp_fixedsizemessageiterator.h>

// BMQ
#include <bmqp_protocol.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_cstring.h>

#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Populate the specified `blob` with an event made of an `EventHeader`, a
/// `HEADER` and the specified `numMsgs` default-constructed `MESSAGE`s, and
/// load the corresponding event header into the specified `eh`.
template <class HEADER, class MESSAGE>
void populateBlob(bdlbb::Blob* blob, bmqp::EventHeader* eh, int numMsgs)
{
    const int eventLength = static_cast<int>(sizeof(bmqp::EventHeader) +
                                             sizeof(HEADER) +
                                             numMsgs * sizeof(MESSAGE));

    (*eh)
        .setLength(eventLength)
        .setHeaderWords(sizeof(bmqp::EventHeader) /
                        bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            sizeof(bmqp::EventHeader));

    HEADER header;
    header.setHeaderWords(sizeof(HEADER) / bmqp::Protocol::k_WORD_SIZE);
    header.setPerMessageWords(sizeof(MESSAGE) / bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(&header),
                            sizeof(HEADER));

    const MESSAGE message = MESSAGE();  // value-initialized: zero bytes
    for (int i = 0; i < numMsgs; ++i) {
        bdlbb::BlobUtil::append(blob,
                                reinterpret_cast<const char*>(&message),
                                sizeof(MESSAGE));
    }
}

/// Return a pointer to the `HEADER` written by `populateBlob` in the
/// specified `blob`.
template <class HEADER>
HEADER* headerOf(bdlbb::Blob* blob)
{
    return reinterpret_cast<HEADER*>(blob->buffer(0).data() +
                                     sizeof(bmqp::EventHeader));
}

/// Check iteration, copy and `clear` for `FixedSizeMessageIterator<HEADER,
/// MESSAGE>` over an event of the specified `numMsgs` messages.
template <class HEADER, class MESSAGE>
void verifyIteration(int numMsgs)
{
    typedef bmqp::FixedSizeMessageIterator<HEADER, MESSAGE> Iterator;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;
    populateBlob<HEADER, MESSAGE>(&blob, &eventHeader, numMsgs);

    Iterator iter(&blob, eventHeader);
    BMQTST_ASSERT(iter.isValid());
    BMQTST_ASSERT_EQ(static_cast<size_t>(iter.header().perMessageWords() *
                                         bmqp::Protocol::k_WORD_SIZE),
                     sizeof(MESSAGE));

    // Copy before iterating; the copy must iterate independently
    Iterator copy(iter);
    BMQTST_ASSERT(copy.isValid());

    int count = 0;
    int rc    = 0;
    while ((rc = iter.next()) == 1) {
        ++count;
        BMQTST_ASSERT(iter.isValid());

        // Assignment in the middle of the iteration keeps the position
        Iterator assigned;
        assigned = iter;
        BMQTST_ASSERT(assigned.isValid());
        BMQTST_ASSERT_EQ(
            bsl::memcmp(&assigned.message(), &iter.message(), sizeof(MESSAGE)),
            0);
    }
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(count, numMsgs);
    BMQTST_ASSERT(!iter.isValid());
    BMQTST_ASSERT_LT(iter.next(), 0);  // rc_INVALID

    int copyCount = 0;
    while (copy.next() == 1) {
        ++copyCount;
    }
    BMQTST_ASSERT_EQ(copyCount, numMsgs);

    // 'reset(blob, other)' continues from the position of 'other' on another
    // blob holding the same data
    {
        Iterator first(&blob, eventHeader);
        if (numMsgs > 0) {
            BMQTST_ASSERT_EQ(first.next(), 1);
        }

        bdlbb::Blob copiedBlob(blob, bmqtst::TestHelperUtil::allocator());
        Iterator    moved;
        BMQTST_ASSERT_EQ(moved.reset(&copiedBlob, first), 0);
        BMQTST_ASSERT(moved.isValid());

        int remaining = 0;
        while (moved.next() == 1) {
            ++remaining;
        }
        BMQTST_ASSERT_EQ(remaining, numMsgs > 0 ? numMsgs - 1 : 0);
    }

    // 'clear' makes the iterator invalid, and 'reset' makes it usable again
    iter.clear();
    BMQTST_ASSERT(!iter.isValid());
    BMQTST_ASSERT_EQ(iter.reset(&blob, eventHeader), 0);
    BMQTST_ASSERT(iter.isValid());
}

/// Check that `reset` rejects a malformed `HEADER` for
/// `FixedSizeMessageIterator<HEADER, MESSAGE>`.
template <class HEADER, class MESSAGE>
void verifyMalformedHeader()
{
    typedef bmqp::FixedSizeMessageIterator<HEADER, MESSAGE> Iterator;

    // Zero header words
    {
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bmqp::EventHeader eventHeader;
        populateBlob<HEADER, MESSAGE>(&blob, &eventHeader, 2);
        headerOf<HEADER>(&blob)->setHeaderWords(0);

        Iterator iter;
        BMQTST_ASSERT_LT(iter.reset(&blob, eventHeader), 0);
        BMQTST_ASSERT(!iter.isValid());
        BMQTST_ASSERT_LT(iter.next(), 0);  // rc_INVALID
    }

    // Zero per-message words: rejected in 'reset', so 'next' never sees a
    // zero advance length
    {
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bmqp::EventHeader eventHeader;
        populateBlob<HEADER, MESSAGE>(&blob, &eventHeader, 2);
        headerOf<HEADER>(&blob)->setPerMessageWords(0);

        Iterator iter;
        BMQTST_ASSERT_LT(iter.reset(&blob, eventHeader), 0);
        BMQTST_ASSERT(!iter.isValid());
        BMQTST_ASSERT_LT(iter.next(), 0);  // rc_INVALID
    }

    // Header declaring more bytes than the blob holds
    {
        bdlbb::PooledBlobBufferFactory bufferFactory(
            1024,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob blob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        bmqp::EventHeader eventHeader;
        populateBlob<HEADER, MESSAGE>(&blob, &eventHeader, 0);
        blob.setLength(blob.length() - 1);

        Iterator iter;
        BMQTST_ASSERT_LT(iter.reset(&blob, eventHeader), 0);
        BMQTST_ASSERT(!iter.isValid());
    }
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    // --------------------------------------------------------------------
    // BREATHING TEST
    //
    // Concerns:
    //   A default-constructed iterator is invalid, and dumps '/no blob/'.
    //
    // Testing:
    //   FixedSizeMessageIterator();
    //   bool isValid() const;
    //   void dumpBlob(bsl::ostream& stream);
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqp::FixedSizeMessageIterator<bmqp::AckHeader, bmqp::AckMessage> iter;
    BMQTST_ASSERT(!iter.isValid());
    BMQTST_ASSERT_LT(iter.next(), 0);  // rc_INVALID

    bmqu::MemOutStream stream(bmqtst::TestHelperUtil::allocator());
    iter.dumpBlob(stream);
    BMQTST_ASSERT_EQ(stream.str(), "/no blob/");
}

static void test2_iteration()
{
    // --------------------------------------------------------------------
    // ITERATION
    //
    // Concerns:
    //   For each instantiation used by the package (Ack, Confirm, Reject):
    //     1. 'next' visits exactly the messages of the event, then returns 0
    //        and leaves the iterator invalid.
    //     2. Copy construction and assignment produce independent
    //        iterators positioned on the same message.
    //     3. 'reset(blob, other)' resumes from the position of 'other'.
    //     4. 'clear' invalidates the iterator, and 'reset' revalidates it.
    //
    // Testing:
    //   FixedSizeMessageIterator(const bdlbb::Blob*, const EventHeader&);
    //   FixedSizeMessageIterator(const FixedSizeMessageIterator&);
    //   operator=(const FixedSizeMessageIterator&);
    //   int next();
    //   int reset(const bdlbb::Blob*, const FixedSizeMessageIterator&);
    //   void clear();
    //   const HEADER& header() const;
    //   const MESSAGE& message() const;
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("ITERATION");

    const int k_NUM_MSGS[] = {0, 1, 7};
    for (size_t i = 0; i < sizeof(k_NUM_MSGS) / sizeof(*k_NUM_MSGS); ++i) {
        verifyIteration<bmqp::AckHeader, bmqp::AckMessage>(k_NUM_MSGS[i]);
        verifyIteration<bmqp::ConfirmHeader, bmqp::ConfirmMessage>(
            k_NUM_MSGS[i]);
        verifyIteration<bmqp::RejectHeader, bmqp::RejectMessage>(
            k_NUM_MSGS[i]);
    }
}

static void test3_malformedHeader()
{
    // --------------------------------------------------------------------
    // MALFORMED HEADER
    //
    // Concerns:
    //   For each instantiation used by the package, 'reset' fails and
    //   leaves the iterator invalid if the header:
    //     1. declares zero header words,
    //     2. declares zero per-message words,
    //     3. declares more bytes than the blob holds.
    //
    // Testing:
    //   int reset(const bdlbb::Blob*, const EventHeader&);
    // --------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("MALFORMED HEADER");

    verifyMalformedHeader<bmqp::AckHeader, bmqp::AckMessage>();
    verifyMalformedHeader<bmqp::ConfirmHeader, bmqp::ConfirmMessage>();
    verifyMalformedHeader<bmqp::RejectHeader, bmqp::RejectMessage>();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    bmqtst::TestGuard guard(argc, argv);

    switch (guard.testCase()) {
    case 0:
    case 3: test3_malformedHeader(); break;
    case 2: test2_iteration(); break;
    case 1: test1_breathingTest(); break;
    default: guard.caseNotFound(); break;
    }

    return guard.finish();
}
