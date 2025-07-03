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

// bmqu_objectplaceholder.t.cpp                                       -*-C++-*-
#include <bmqu_objectplaceholder.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bslma_testallocator.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

// ====================
// struct TestException
// ====================

/// An exception to be thrown by the `TestObject` on construction.
struct TestException {};

// ================
// class TestObject
// ================

/// A object for to be held by the placeholder.
template <size_t PADDING>
class TestObject {
  private:
    // PRIVATE DATA
    bool* d_initializationFlag_p;

    char d_padding[PADDING];

  private:
    // NOT IMPLEMENTED
    TestObject(const TestObject&) BSLS_KEYWORD_DELETED;
    TestObject& operator=(const TestObject&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS
    explicit TestObject(bool* initializationFlag,
                        bool  throwOnConstruction = false)
    : d_initializationFlag_p(initializationFlag)
    {
        // PRECONDITIONS
        BSLS_ASSERT(initializationFlag);

        if (throwOnConstruction) {
            throw TestException();  // THROW
        }

        *initializationFlag = true;
    }

    ~TestObject() { *d_initializationFlag_p = false; }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_createDeleteObject()
// ------------------------------------------------------------------------
// CREATE DELETE OBJECT
//
// Concerns:
//   Ensure proper behavior of the 'createObject' and 'deleteObject'
//   functions.
//
// Plan:
//   1. Ensure that 'createObject' correctly creates objects placing them
//      either in the internal or the external buffer, depending on the
//      object size, and that 'deleteObject' correctly deletes previously
//      created objects.
//
//   2. Ensure that 'createObject' provides strong exception guarantee.
//
// Testing:
//   bmqu::ObjectPlaceHolder::createObject
//   bmqu::ObjectPlaceHolder::deleteObject
// ------------------------------------------------------------------------
{
    static const int         k_BUFFER_SIZE = 64;
    static const void* const k_ZERO_P      = static_cast<const void*>(0);

    typedef TestObject<1>                 SmallObject;
    typedef TestObject<k_BUFFER_SIZE + 1> LargeObject;

    // 1. General use-case
    {
        // allocator for external storage
        bslma::TestAllocator storageAllocator;

        // create placeholder
        bmqu::ObjectPlaceHolder<k_BUFFER_SIZE> placeHolder;

        // placeholder is empty
        BMQTST_ASSERT_EQ(placeHolder.objectAddress(), k_ZERO_P);

        // object initialization flag
        bool objectInitialized = false;

        // create a "small" object
        placeHolder.createObject<SmallObject>(&storageAllocator,
                                              &objectInitialized);

        // placeholder is not empty and the object is located internally
        BMQTST_ASSERT_NE(placeHolder.objectAddress(), k_ZERO_P);
        BMQTST_ASSERT_EQ(placeHolder.objectAddress(),
                         static_cast<void*>(&placeHolder));

        // the object was properly constructed
        BMQTST_ASSERT_EQ(objectInitialized, true);

        // no allocation did occur
        BMQTST_ASSERT_EQ(storageAllocator.numAllocation(), 0);

        // delete the object
        placeHolder.deleteObject<SmallObject>();

        // placeholder is empty
        BMQTST_ASSERT_EQ(placeHolder.objectAddress(), k_ZERO_P);

        // the object was properly destructed
        BMQTST_ASSERT_EQ(objectInitialized, false);

        // create a "large" object
        placeHolder.createObject<LargeObject>(&storageAllocator,
                                              &objectInitialized);

        // placeholder is not empty and the object is located externally
        BMQTST_ASSERT_NE(placeHolder.objectAddress(), k_ZERO_P);
        BMQTST_ASSERT_NE(placeHolder.objectAddress(),
                         static_cast<const void*>(&placeHolder));

        // the object was properly constructed
        BMQTST_ASSERT_EQ(objectInitialized, true);

        // one allocation occurred
        BMQTST_ASSERT_EQ(storageAllocator.numAllocation(), 1);

        // delete the object
        placeHolder.deleteObject<LargeObject>();

        // placeholder is empty
        BMQTST_ASSERT_EQ(placeHolder.objectAddress(), k_ZERO_P);

        // the object was properly destructed
        BMQTST_ASSERT_EQ(objectInitialized, false);

        // one deallocation occurred
        BMQTST_ASSERT_EQ(storageAllocator.numDeallocation(), 1);
    }

    // 2. Exception-safety
    {
        // allocator for external storage
        bslma::TestAllocator storageAllocator;

        // create placeholder
        bmqu::ObjectPlaceHolder<k_BUFFER_SIZE> placeHolder;

        bool objectInitialized = false;
        bool exceptionThrown   = false;

        // create a "small" object that throws on construction
        try {
            placeHolder.createObject<SmallObject>(
                &storageAllocator,
                &objectInitialized,
                true);  // throwOnConstruction
        }
        catch (const TestException&) {
            exceptionThrown = true;
        }

        // exception thrown
        BMQTST_ASSERT_EQ(exceptionThrown, true);

        // placeholder is empty
        BMQTST_ASSERT_EQ(placeHolder.objectAddress(), k_ZERO_P);

        // reset exception flag
        exceptionThrown = false;

        // create a "large" object that throws on construction
        try {
            placeHolder.createObject<LargeObject>(
                &storageAllocator,
                &objectInitialized,
                true);  // throwOnConstruction
        }
        catch (const TestException&) {
            exceptionThrown = true;
        }

        // exception thrown
        BMQTST_ASSERT_EQ(exceptionThrown, true);

        // placeholder is empty
        BMQTST_ASSERT_EQ(placeHolder.objectAddress(), k_ZERO_P);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_createDeleteObject(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
