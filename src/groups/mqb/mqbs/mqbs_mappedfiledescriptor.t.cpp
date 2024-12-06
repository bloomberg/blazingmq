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

// mqbs_mappedfiledescriptor.t.cpp                                    -*-C++-*-
#include <mqbs_mappedfiledescriptor.h>

// MQB
#include <mqbs_memoryblock.h>

// BDE
#include <bsls_alignedbuffer.h>
#include <bsls_alignmentutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Testing:
//   Verifies the default constructor of 'mqbs::MappedFileDescriptor'.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Breathing Test");

    // Default constructor
    mqbs::MappedFileDescriptor obj;

    BMQTST_ASSERT_EQ(obj.fd(),
                     static_cast<int>(obj.k_INVALID_FILE_DESCRIPTOR));
    BMQTST_ASSERT_EQ(obj.fileSize(), 0U);
    BMQTST_ASSERT_EQ(
        obj.mappingSize(),
        static_cast<bsls::Types::Uint64>(obj.k_INVALID_MAPPING_SIZE));
    BMQTST_ASSERT_EQ(obj.mapping(), obj.k_INVALID_MAPPING);
    BMQTST_ASSERT(!obj.isValid());
}

static void test2_operations()
// ------------------------------------------------------------------------
// Operations Test
//
// Testing:
//   Verifies the manipulators and accessors for the Value fields
//   in a 'mqbs::MappedFileDescriptor'.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Operations Test");

    const bsl::size_t         bufferLength         = 64;
    char                      buffer[bufferLength] = {0};
    const int                 fileDescriptor       = 10;
    const bsls::Types::Uint64 fileSize             = 512;

    mqbs::MemoryBlock block(buffer, bufferLength);

    mqbs::MappedFileDescriptor obj;

    // Verify Manipulators
    obj.setFd(fileDescriptor)
        .setFileSize(fileSize)
        .setBlock(block)
        .setMapping(block.base())
        .setMappingSize(block.size());

    BMQTST_ASSERT(obj.isValid());

    // Verify Manipulators- Accessors
    BMQTST_ASSERT_EQ(obj.fd(), fileDescriptor);
    BMQTST_ASSERT_EQ(obj.fileSize(), fileSize);
    BMQTST_ASSERT_EQ(obj.mapping(), block.base());
    BMQTST_ASSERT_EQ(obj.mappingSize(), block.size());
}

static void test3_reset()
// ------------------------------------------------------------------------
// Reset Test
//
// Testing:
//   Verifies the manupulator 'clear()' of a 'mqbs::MappedFileDescriptor'.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Reset Test");

    const bsl::size_t         bufferLength         = 32;
    char                      buffer[bufferLength] = {0};
    const int                 fileDescriptor       = 7;
    const bsls::Types::Uint64 fileSize             = 128;

    mqbs::MemoryBlock block(buffer, bufferLength);

    mqbs::MappedFileDescriptor obj;

    // Set Fields
    obj.setFd(fileDescriptor)
        .setFileSize(fileSize)
        .setBlock(block)
        .setMapping(block.base())
        .setMappingSize(block.size());

    BMQTST_ASSERT(obj.isValid());

    // Verify Clear()
    obj.reset();

    BMQTST_ASSERT_EQ(obj.fd(),
                     static_cast<int>(obj.k_INVALID_FILE_DESCRIPTOR));
    BMQTST_ASSERT_EQ(obj.fileSize(), 0U);
    BMQTST_ASSERT_EQ(
        obj.mappingSize(),
        static_cast<bsls::Types::Uint64>(obj.k_INVALID_MAPPING_SIZE));
    BMQTST_ASSERT_EQ(obj.mapping(), obj.k_INVALID_MAPPING);
    BMQTST_ASSERT(!obj.isValid());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_reset(); break;
    case 2: test2_operations(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
