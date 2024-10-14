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

// bmqu_blobobjectproxy.t.cpp -*-C++-*-
#include <bmqu_blobobjectproxy.h>

#include <bmqtst_blobtestutil.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bslim_printer.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

// =================
// struct TestHeader
// =================

/// Binary protocol header structure used in tests
struct TestHeader {
    // DATA
    unsigned char         d_length;
    unsigned char         d_reserved[3];
    bdlb::BigEndianUint32 d_member1;
    bdlb::BigEndianUint32 d_member2;

    // MANIPULATORS
    void reset(int length, int member1, int member2);
};

// -----------------
// struct TestHeader
// -----------------

// MANIPULATORS
void TestHeader::reset(int length, int member1, int member2)
{
    d_length = length;
    bsl::memset(d_reserved, 0, sizeof(d_reserved));
    d_member1 = member1;
    d_member2 = member2;
}

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//    a) Reading an object from a blob that is correctly aligned and in
//       1 buffer return a pointer into the blob.
//    b) Reading an incomplete object from a blob correctly copies out
//       the relevant section, zeros out the remainder, and correctly
//       reports which fields are present.
//    c) 'resize' correctly adjusts the size of the object.
//    d) When created in write mode, changes are written out to the blob
//       if necessary upon destruction or reset
//    e) Reading from a blob shorter than specified TYPE's length
//    f) Specifying minimum bytes to read
//    g) 'loadEndPosition' returns the correct position
//
// Plan:
//    a) Test reading a complete, aligned object from blob
//    b) Test reading an incomplete object from a blob
//    c) Test 'resize'
//    d) Test writing out changes to the blob
//    e) Test 'reset' with a blob shorter than specified TYPE's length
//    f) Test 'reset' with speicfying minimum length to read
//    g) Test 'loadEndPosition'
//
// Testing:
//    everything
{
    bmqtst::TestHelper::printTestName("Breathing Test");

    bdlbb::PooledBlobBufferFactory smallFactory(2, s_allocator_p);
    bdlbb::PooledBlobBufferFactory bigFactory(128, s_allocator_p);

    TestHeader        hdr;
    const char* const ptr = reinterpret_cast<const char*>(&hdr);

    {
        PVV("Test reading a complete, aligned object from blob");

        bdlbb::Blob blob(&bigFactory, s_allocator_p);
        hdr.reset(sizeof(hdr), 1, 2);
        bdlbb::BlobUtil::append(&blob, ptr, sizeof(hdr));

        bmqu::BlobObjectProxy<TestHeader> pxy(&blob);
        ASSERT(pxy.isInBlob());
        ASSERT_EQ(pxy->d_member1, 1u);
        ASSERT_EQ(pxy->d_member2, 2u);
        ASSERT(pxy.hasMember(&TestHeader::d_length));
        ASSERT(pxy.hasMember(&TestHeader::d_member2));
    }

    {
        PVV("Test reading an incomplete object from a blob");

        bdlbb::Blob blob(&bigFactory, s_allocator_p);
        hdr.reset(9, 1, 2);
        bdlbb::BlobUtil::append(&blob, ptr, sizeof(hdr));

        bmqu::BlobObjectProxy<TestHeader> pxy(&blob, 9);
        ASSERT(!pxy.isInBlob());
        ASSERT_EQ(pxy->d_member1, 1u);
        ASSERT_EQ(pxy->d_member2, 0u);
        ASSERT(pxy.hasMember(&TestHeader::d_member1));
        ASSERT(!pxy.hasMember(&TestHeader::d_member2));
    }

    {
        PVV("Test 'resize'");

        bdlbb::Blob blob(&bigFactory, s_allocator_p);
        hdr.reset(sizeof(hdr), 1, 2);
        bdlbb::BlobUtil::append(&blob, ptr, sizeof(hdr));

        bmqu::BlobObjectProxy<TestHeader> pxy(&blob, -1);
        ASSERT(pxy.isInBlob());

        pxy.resize(128);
        ASSERT(pxy.isInBlob());
        ASSERT(pxy.hasMember(&TestHeader::d_member2));

        pxy.resize(8);
        ASSERT(!pxy.isInBlob());
        ASSERT(pxy.hasMember(&TestHeader::d_member1));
        ASSERT(!pxy.hasMember(&TestHeader::d_member2));
        ASSERT_EQ(pxy->d_member1, 1u);
    }

    {
        PVV("Test writing out changes to the blob");

        bdlbb::Blob blob(&bigFactory, s_allocator_p);
        hdr.reset(sizeof(hdr), 1, 2);
        bdlbb::BlobUtil::append(&blob, ptr, sizeof(hdr));

        // Modify without 'write' mode
        bmqu::BlobObjectProxy<TestHeader> pxy(&blob, 8);
        ASSERT(!pxy.isInBlob());
        ASSERT(pxy.hasMember(&TestHeader::d_member1));
        ASSERT(!pxy.hasMember(&TestHeader::d_member2));

        pxy->reset(8, 11, 22);
        pxy.reset();
        ASSERT(!pxy.isSet());

        // Modify with 'write' mode
        pxy.reset(&blob, 8, true, true);
        ASSERT(pxy.isSet());
        ASSERT(!pxy.isInBlob());
        ASSERT_EQ(size_t(pxy->d_length), sizeof(hdr));
        ASSERT_EQ(pxy->d_member1, 1u);
        ASSERT_EQ(pxy->d_member2, 0u);

        pxy->reset(17, 11, 22);
        pxy.reset();
        ASSERT(!pxy.isSet());

        // Verify that only the proxied fields were written out
        pxy.reset(&blob);
        ASSERT(pxy.isSet());
        ASSERT(pxy.isInBlob());
        ASSERT_EQ(pxy->d_length, 17);
        ASSERT_EQ(pxy->d_member1, 11u);
        ASSERT_EQ(pxy->d_member2, 2u);
    }

    {
        PVV("Test 'reset' with a blob shorter than specified TYPE's length");

        // Create a blob length 1
        bdlbb::Blob blob(&bigFactory, s_allocator_p);
        bdlbb::BlobUtil::append(&blob, "a", 1);
        ASSERT_EQ(1, blob.length());

        bmqu::BlobObjectProxy<TestHeader> pxy;
        pxy.reset(&blob,
                  bmqu::BlobPosition(),
                  -1,      // read as many bytes as possible
                  true,    // read
                  false);  // write

        ASSERT_EQ(1, pxy.length());
        ASSERT_EQ(true, pxy.isSet());
    }

    {
        PVV("Test 'reset' with speicfying minimum length to read");

        // Create blob containing complete TestHeader
        bdlbb::Blob blob(&bigFactory, s_allocator_p);
        bdlbb::BlobUtil::append(&blob, ptr, sizeof(hdr));

        bmqu::BlobObjectProxy<TestHeader> pxy;
        pxy.reset(&blob,
                  bmqu::BlobPosition(),
                  -4,      // read at least 4 bytes
                  true,    // read
                  false);  // write

        ASSERT_EQ(true, pxy.isSet());
        ASSERT_EQ(sizeof(hdr), size_t(pxy.length()));

        // Create blob containing partial TestHeader
        blob.removeAll();
        bdlbb::BlobUtil::append(&blob, ptr, 4);  // only 4 bytes

        // Reset proxy to read more bytes than present in the blob
        pxy.reset(&blob,
                  bmqu::BlobPosition(),
                  -8,      // read at least 8 bytes
                  true,    // read
                  false);  // write

        ASSERT_EQ(false, pxy.isSet());
    }

    {
        PVV("Test 'loadEndPosition'");

        // Create blob containing complete TestHeader
        bdlbb::Blob blob(&bigFactory, s_allocator_p);
        hdr.reset(1, 2, 3);
        bdlbb::BlobUtil::append(&blob, ptr, sizeof(hdr));
        hdr.reset(4, 5, 6);
        bdlbb::BlobUtil::append(&blob, ptr, sizeof(hdr));

        bmqu::BlobObjectProxy<TestHeader> pxy;
        bmqu::BlobPosition                pos;

        // Looking at whole object
        pxy.reset(&blob);
        pxy.loadEndPosition(&pos);
        ASSERT_EQ(pos, bmqu::BlobPosition(0, sizeof(TestHeader)));

        // Looking at part of an object
        pxy.reset(&blob, 10);
        pxy.loadEndPosition(&pos);
        ASSERT_EQ(pos, bmqu::BlobPosition(0, 10));

        // Looking at an object at the end of the blob
        pxy.reset(&blob, bmqu::BlobPosition(0, sizeof(TestHeader)));
        pxy.loadEndPosition(&pos);
        ASSERT_EQ(pos, bmqu::BlobPosition(1, 0));
    }
}

static void test2_usageExample1()
// ------------------------------------------------------------------------
// USAGE EXAMPLE 1
//
// Concerns:
//  a) Usage example 1 compiles and runs as expected.
//
// Plan:
//  1) Copy the usage example from the header.
//
// Testing:
//  Usage Example 1
//
{
    bmqtst::TestHelper::printTestName("Usage Example 1 Test");

    /// Example 1: Reading objects from blobs
    ///- - - - - - - - - - - - - - - - - - -
    // We demonstrate using a 'BlobObjectProxy' to read the fields of a proxied
    // object in a blob.
    //
    // First, we initialize our blob by writing a single 'TestHeader' object to
    // it with some test values.
    //..
    bdlbb::PooledBlobBufferFactory factory(0xFF, s_allocator_p);
    bdlbb::Blob                    blob(&factory, s_allocator_p);

    TestHeader hdr;
    bsl::memset(&hdr, 0, sizeof(hdr));
    hdr.d_length  = sizeof(hdr);
    hdr.d_member1 = 1;
    hdr.d_member2 = 2;

    const char* const ptr = reinterpret_cast<const char*>(&hdr);
    bdlbb::BlobUtil::append(&blob, ptr, sizeof(hdr));
    //..
    // Next, we can construct a 'BlobObjectProxy' for a 'TestHeader' object
    // contained at the beginning of 'blob' and access its fields.
    //..
    bmqu::BlobObjectProxy<TestHeader> pxy(&blob);
    ASSERT(pxy.isSet());
    ASSERT(pxy->d_length == sizeof(hdr));
    ASSERT(pxy->d_member1 == 1);
    ASSERT(pxy->d_member2 == 2);
    //..
    // Now, suppose we know that our blob only contains up to 'd_member1' of
    // our 'TestHeader'.  We can reset (or construct) a 'BlobObjectProxy' with
    // this limit, and it will correctly expose only the fields present in the
    // blob.
    //
    // To do this, we can reset our proxy with a length of '8', which will
    // allow us to read up to 'd_member1' of our structure.
    //..
    pxy.reset(&blob, 8);
    ASSERT(pxy->d_length == sizeof(hdr));
    ASSERT(pxy->d_member1 == 1);
    //..
    // When configured this way, the 'BlobObjectProxy' will zero out the
    // remaining fields in its internal buffer (the 'blob' is not modified in
    // any way).
    //..
    ASSERT(pxy->d_member2 == 0);
    //..
    // In most cases, having the extraneous members return a value of '0'
    // should be enough, however in some cases it might be desirable to
    // explicitly check whether a field is actually present in the proxied
    // object.  This can be accomplished using 'hasMember' as follows:
    //..
    ASSERT(pxy.hasMember(&TestHeader::d_length));
    ASSERT(pxy.hasMember(&TestHeader::d_member1));
    ASSERT(!pxy.hasMember(&TestHeader::d_member2));
    //..
    // Finally, we will demonstrate the proper way of reading a blob object
    // whose length is one of its members and can't be known until the object
    // is already proxied.
    //
    // By configuring the 'BlobObjectProxy' with a length of '-1', we instruct
    // it to read as much of our object from the blob as possible, without any
    // knowledge (yet) of whether all the data is actually a part of our
    // object.
    //..
    pxy.reset(&blob, -1);
    //..
    // At this point, 'pxy' is proxying as much of 'TestHeader' as possible,
    // but we still need to ensure that 'd_length' is one of the proxied fields
    // (for example, if the blob was only 2 bytes long).
    //..
    ASSERT(pxy.hasMember(&TestHeader::d_length));
    //..
    // Now we can safely access 'd_length' in our object, and update the
    // proxied object's size appropriately.
    //..
    pxy.resize(pxy->d_length);
    //..
    // Now, we can safely access any desired fields of our proxied object.
    // Note that accessing fields that were not actually present in the message
    // using a 'BlobObjectProxy' before its length is set correctly will result
    // in *undefined behavior*.
}

static void test3_usageExample2()
// --------------------------------------------------------------------
// USAGE EXAMPLE 2
//
// Concerns:
//  a) Usage example 2 compiles and runs as expected.
//
// Plan:
//  1) Copy the usage example from the header.
//
// Testing:
//  Usage Example 2
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Usage Example 2 Test");

    /// Example 2: Modify blob objects
    ///  - - - - - - - - - - - - - - -
    // We demonstrate using a 'BlobObjectProxy' to modify a proxied blob object
    // efficiently.  To do this, we need to create our blob with enough room to
    // store an instance of our 'TestHeader'.
    //..
    bdlbb::PooledBlobBufferFactory factory(0xFF, s_allocator_p);
    bdlbb::Blob                    blob(&factory, s_allocator_p);

    TestHeader        hdr;
    const char* const ptr = reinterpret_cast<const char*>(&hdr);
    bdlbb::BlobUtil::append(&blob, ptr, sizeof(hdr));
    //..
    // With the blob in this uninitialized state, first, we must create a
    // 'BlobObjectProxy' for writing.
    //..
    bmqu::BlobObjectProxy<TestHeader> pxy(
        &blob,
        false,  // don't read current contents
        true);  // write out changes upon
    //..
    // Configured this way, the 'BlobObjectProxy' will write any changes made
    // to the proxied object back into its blob if necessary when it is
    // destroyed or reset.
    //
    // Now we can proceed to fill our object with values.
    //..
    bsl::memset(pxy.object(), 0, sizeof(TestHeader));
    pxy->d_length  = sizeof(TestHeader);
    pxy->d_member1 = 17;
    pxy->d_member2 = 32;
    pxy.reset();
    //..
    // Finally, we verify that the blob contains our changes.
    //..
    bdlbb::Blob expectedBlob(&factory, s_allocator_p);

    bsl::memset(&hdr, 0, sizeof(hdr));
    hdr.d_length  = sizeof(TestHeader);
    hdr.d_member1 = 17;
    hdr.d_member2 = 32;
    bdlbb::BlobUtil::append(&expectedBlob, ptr, sizeof(hdr));
    ASSERT(!bdlbb::BlobUtil::compare(blob, expectedBlob));
    //..
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_usageExample2(); break;
    case 2: test2_usageExample1(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
