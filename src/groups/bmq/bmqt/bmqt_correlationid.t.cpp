// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_correlationid.t.cpp                                           -*-C++-*-
#include <bmqt_correlationid.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_ios.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslh_defaulthashalgorithm.h>
#include <bslh_hash.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_functional.h>

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
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // An object for testing pointers
    bsl::string  foo("foo", bmqtst::TestHelperUtil::allocator());
    bsl::string* fooPtr = &foo;

    PV("Test unset correlationId");
    bmqt::CorrelationId unset;
    BMQTST_ASSERT_EQ(unset.isUnset(), true);

    PV("Test initialization by Int64");
    bmqt::CorrelationId id(-1);
    BMQTST_ASSERT_EQ(id.isNumeric(), true);
    BMQTST_ASSERT_EQ(id.isPointer(), false);
    BMQTST_ASSERT_EQ(id.isUnset(), false);
    BMQTST_ASSERT_EQ(-1, id.theNumeric());

    PV("Test initialization by integer");
    bsls::Types::Int64  numeric = 0x5a5a5a5a5a5a5a5a;
    bmqt::CorrelationId numericId(numeric);
    BMQTST_ASSERT_EQ(numericId.isPointer(), false);
    BMQTST_ASSERT_EQ(numeric, numericId.theNumeric());

    PV("Test copy constructor");
    bmqt::CorrelationId newNumericId(numericId);
    BMQTST_ASSERT_EQ(newNumericId.isPointer(), false);
    BMQTST_ASSERT_EQ(numeric, newNumericId.theNumeric());

    PV("Test integer value assignment");
    id = numericId;
    BMQTST_ASSERT_EQ(id.isPointer(), false);
    BMQTST_ASSERT_EQ(numeric, id.theNumeric());

    PV("Test initialization by pointer");
    bmqt::CorrelationId ptrId(fooPtr);
    BMQTST_ASSERT_EQ(ptrId.isPointer(), true);
    BMQTST_ASSERT_EQ(fooPtr, ptrId.thePointer());

    PV("Test copy constructor");
    bmqt::CorrelationId newPtrId(ptrId);
    BMQTST_ASSERT_EQ(newPtrId.isPointer(), true);
    BMQTST_ASSERT_EQ(fooPtr, newPtrId.thePointer());

    PV("Test pointer value assignment");
    id = ptrId;
    BMQTST_ASSERT_EQ(id.isPointer(), true);
    BMQTST_ASSERT_EQ(fooPtr, id.thePointer());

    PV("Test equality operator");
    BMQTST_ASSERT_EQ(id, ptrId);
    BMQTST_ASSERT_EQ(!(id == numericId), true);
    BMQTST_ASSERT_EQ(id != numericId, true);
    BMQTST_ASSERT_EQ(!(id == bmqt::CorrelationId(fooPtr + 1)), true);
    BMQTST_ASSERT_EQ(id != bmqt::CorrelationId(fooPtr + 1), true);

    id = numericId;
    BMQTST_ASSERT_EQ(id, numericId);
    BMQTST_ASSERT_EQ(!(id == ptrId), true);
    BMQTST_ASSERT_EQ(id != ptrId, true);
    BMQTST_ASSERT_EQ(!(id == bmqt::CorrelationId(numeric + 1)), true);
    BMQTST_ASSERT_EQ(id != bmqt::CorrelationId(numeric + 1), true);

    PV("Test 'Set' methods");
    id.setPointer(fooPtr);
    BMQTST_ASSERT_EQ(id, bmqt::CorrelationId(fooPtr));

    id.setNumeric(numeric);
    BMQTST_ASSERT_EQ(id, bmqt::CorrelationId(numeric));

    PV("Test 'makeUnset' method");
    id.makeUnset();
    BMQTST_ASSERT_EQ(id, bmqt::CorrelationId());
    BMQTST_ASSERT_EQ(id.isUnset(), true);
}

static void test2_copyAndAssign()
// ------------------------------------------------------------------------
// TEST COPY AND ASSIGN
//
// Concerns:
//   Test copy and assignment of the object.
//
// Plan:
//
// Testing:
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("COPY AND ASSIGN");

    // An object for testing pointers
    bsl::string  foo("foo", bmqtst::TestHelperUtil::allocator());
    bsl::string* fooPtr = &foo;

    // conversions to bmqt::CorrelationId and back
    bsls::Types::Int64 numeric = 0xA5A5A5A5A5A5A5A5;
    void*              ptr     = fooPtr + 1;

    bmqt::CorrelationId numericId(numeric);
    bmqt::CorrelationId ptrId(ptr);

    PV("Test copy constructor")
    {
        bmqt::CorrelationId numericIdCopy(numericId);
        BMQTST_ASSERT_EQ(numericIdCopy.isPointer(), false);
        BMQTST_ASSERT_EQ(numericIdCopy.theNumeric(), numeric);

        bmqt::CorrelationId ptrIdCopy(ptrId);
        BMQTST_ASSERT_EQ(ptrIdCopy.isPointer(), true);
        BMQTST_ASSERT_EQ(ptrIdCopy.thePointer(), ptr);
    }

    PV("Test assignment operator");
    {
        bmqt::CorrelationId numericIdCopy = numericId;
        BMQTST_ASSERT_EQ(numericIdCopy.isPointer(), false);
        BMQTST_ASSERT_EQ(numericIdCopy.theNumeric(), numeric);

        bmqt::CorrelationId ptrIdCopy = ptrId;
        BMQTST_ASSERT_EQ(ptrIdCopy.isPointer(), true);
        BMQTST_ASSERT_EQ(ptrIdCopy.thePointer(), ptr);
    }

    PV("Test conversion back to bmqt::CorrelationId");
    {
        bmqt::CorrelationId numericIdCopy = bmqt::CorrelationId(numeric);
        BMQTST_ASSERT_EQ(numericIdCopy.isPointer(), false);
        BMQTST_ASSERT_EQ(numericIdCopy.theNumeric(), numeric);

        bmqt::CorrelationId ptrIdCopy = bmqt::CorrelationId(ptr);
        BMQTST_ASSERT_EQ(ptrIdCopy.isPointer(), true);
        BMQTST_ASSERT_EQ(ptrIdCopy.thePointer(), ptr);
    }
}

static void test3_compare()
// ------------------------------------------------------------------------
// TEST COMPARE
//
// Concerns:
//   Test comparing two objects.
//
// Plan:
//
// Testing:
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("COMPARE");

    // Test compare operator
    bmqt::CorrelationIdLess less;
    const int               val1 = 1;
    const int               val4 = 4;
    bsl::shared_ptr<int>    intPtr1;
    bsl::shared_ptr<int>    intPtr4;

    intPtr1.createInplace(bmqtst::TestHelperUtil::allocator(), val1);
    intPtr4.createInplace(bmqtst::TestHelperUtil::allocator(), val4);

    bmqt::CorrelationId sptrId1(intPtr1);
    bmqt::CorrelationId sptrId4(intPtr4);
    bmqt::CorrelationId numericId1(val1);
    bmqt::CorrelationId numericId4(val4);
    bmqt::CorrelationId ptrId1(reinterpret_cast<void*>(val1));
    bmqt::CorrelationId ptrId4(reinterpret_cast<void*>(val4));
    bmqt::CorrelationId autoId1 = bmqt::CorrelationId::autoValue();
    bmqt::CorrelationId autoId2 = bmqt::CorrelationId::autoValue();

    BMQTST_ASSERT_EQ(less(numericId1, numericId4), true);
    BMQTST_ASSERT_EQ(less(numericId4, numericId1), false);

    BMQTST_ASSERT_EQ(less(ptrId1, ptrId4), true);
    BMQTST_ASSERT_EQ(less(ptrId4, ptrId1), false);

    BMQTST_ASSERT_EQ(less(autoId1, autoId2), true);
    BMQTST_ASSERT_EQ(less(autoId2, autoId1), false);

    BMQTST_ASSERT_EQ(less(sptrId1, sptrId4), true);
    BMQTST_ASSERT_EQ(less(sptrId4, sptrId1), false);

    BMQTST_ASSERT_EQ(less(numericId1, ptrId1), true);
    BMQTST_ASSERT_EQ(less(ptrId1, numericId1), false);

    BMQTST_ASSERT_EQ(less(numericId4, ptrId1), true);
    BMQTST_ASSERT_EQ(less(ptrId4, numericId1), false);

    BMQTST_ASSERT_EQ(less(numericId1, sptrId1), true);
    BMQTST_ASSERT_EQ(less(sptrId1, numericId1), false);

    BMQTST_ASSERT_EQ(less(numericId1, autoId1), true);
    BMQTST_ASSERT_EQ(less(autoId1, numericId1), false);

    BMQTST_ASSERT_EQ(less(ptrId1, sptrId1), true);
    BMQTST_ASSERT_EQ(less(sptrId1, ptrId1), false);

    BMQTST_ASSERT_EQ(less(sptrId1, autoId1), true);
    BMQTST_ASSERT_EQ(less(autoId1, sptrId1), false);
}

static void test4_smartPointers()
// ------------------------------------------------------------------------
// TEST SMART POINTERS
//
// Concerns:
//   Test correlation ids wrapping smart pointers.
//
// Plan:
//
// Testing:
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SMART POINTERS");

    int                 value = 153;
    bmqt::CorrelationId smartIntCorrelation;
    {
        bsl::shared_ptr<int> intPtr;
        intPtr.createInplace(bmqtst::TestHelperUtil::allocator(), value);
        bmqt::CorrelationId id(intPtr);
        smartIntCorrelation = id;
        BMQTST_ASSERT_EQ(smartIntCorrelation.isNumeric(), false);
        BMQTST_ASSERT_EQ(smartIntCorrelation.isPointer(), false);
        BMQTST_ASSERT_EQ(smartIntCorrelation.isSharedPtr(), true);
        BMQTST_ASSERT_EQ(smartIntCorrelation.theSharedPtr(), intPtr);
    }
    bsl::shared_ptr<void> resPtr = smartIntCorrelation.theSharedPtr();
    BMQTST_ASSERT_EQ(value, *reinterpret_cast<int*>(resPtr.get()));
}

static void test5_autoValue()
// ------------------------------------------------------------------------
// TEST AUTO VALUE
//
// Concerns:
//   Test correlation ids with automatic value.
//
// Plan:
//
// Testing:
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("AUTO VALUE");

    bmqt::CorrelationId auto1 = bmqt::CorrelationId::autoValue();
    bmqt::CorrelationId auto2 = bmqt::CorrelationId::autoValue();

    BMQTST_ASSERT_EQ(auto1.isAutoValue(), true);

    BMQTST_ASSERT_EQ(auto1, auto1);
    BMQTST_ASSERT_NE(auto1, auto2);
}

static void test6_hashAppend()
// ------------------------------------------------------------------------
// TEST HASH APPEND
//
// Concerns:
//   Ensure that 'hashAppend' on 'bmqt::CorrelationId' is functional.
//
// Plan:
//  1) Generate a 'bmqt::CorrelationId' object, compute its hash, and
//     verify that 'hashAppend' on this object is deterministic by
//     comparing the hash value over many iterations.
//
// Testing:
//   template <class HASH_ALGORITHM>
//   void
//   hashAppend(HASH_ALGORITHM&            hashAlgo,
//              const bmqt::CorrelationId& corrId)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HASH APPEND");

    PV("HASH FUNCTION DETERMINISTIC");

    bsl::string        foo("foo", bmqtst::TestHelperUtil::allocator());
    bsl::string*       fooPtr           = &foo;
    bsls::Types::Int64 numeric          = 0xA5A5A5A5A5A5A5A5;
    const size_t       k_NUM_ITERATIONS = 1000;
    size_t             t                = bmqt::CorrelationId::e_NUMERIC;
    bsl::shared_ptr<bsls::Types::Int64> intPtr;

    intPtr.createInplace(bmqtst::TestHelperUtil::allocator(), numeric);

    for (; t < bmqt::CorrelationId::e_UNSET + 1; ++t) {
        bmqt::CorrelationId obj;
        switch (t) {
        case bmqt::CorrelationId::e_NUMERIC: {
            obj = bmqt::CorrelationId(numeric);
        } break;
        case bmqt::CorrelationId::e_POINTER: {
            obj = bmqt::CorrelationId(fooPtr);
        } break;
        case bmqt::CorrelationId::e_SHARED_PTR: {
            obj = bmqt::CorrelationId(intPtr);
        } break;
        case bmqt::CorrelationId::e_AUTO_VALUE: {
            obj = bmqt::CorrelationId::autoValue();
        } break;
        case bmqt::CorrelationId::e_UNSET: {
            // obj has e_UNSET type
        } break;
        default: {
            BSLS_ASSERT_OPT(false && "Unknown correlationId type");
        }
        }

        bsl::hash<bmqt::CorrelationId>              hasher;
        bsl::hash<bmqt::CorrelationId>::result_type firstHash = hasher(obj);

        for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
            bslh::DefaultHashAlgorithm algo;
            hashAppend(algo, obj);
            bsl::hash<bmqt::CorrelationId>::result_type currHash =
                algo.computeHash();
            PVVV("[" << t << "]"
                     << "[" << i << "] hash: " << currHash);
            BMQTST_ASSERT_EQ_D(i, currHash, firstHash);
        }
    }
}

static void test7_printTest()
{
    bmqtst::TestHelper::printTestName("PRINT");

    PV("Testing print");

    bsl::string        foo("foo", bmqtst::TestHelperUtil::allocator());
    bsl::string*       fooPtr  = &foo;
    bsls::Types::Int64 numeric = 0x5a5a5a5a5a5a5a5a;
    size_t             t       = bmqt::CorrelationId::e_NUMERIC;
    bsl::shared_ptr<bsls::Types::Int64> intPtr;

    intPtr.createInplace(bmqtst::TestHelperUtil::allocator(), numeric);

    for (; t < bmqt::CorrelationId::e_UNSET + 1; ++t) {
        bmqt::CorrelationId obj;
        bmqu::MemOutStream  patStream(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream  objStream(bmqtst::TestHelperUtil::allocator());
        switch (t) {
        case bmqt::CorrelationId::e_NUMERIC: {
            obj = bmqt::CorrelationId(numeric);
            patStream << "[ numeric = " << numeric << " ]";
        } break;
        case bmqt::CorrelationId::e_POINTER: {
            obj = bmqt::CorrelationId(fooPtr);
            patStream << bsl::noshowbase << std::hex;
            patStream << "[ pointer = 0x" << reinterpret_cast<size_t>(fooPtr)
                      << " ]";
        } break;
        case bmqt::CorrelationId::e_SHARED_PTR: {
            obj = bmqt::CorrelationId(intPtr);
            patStream << bsl::noshowbase << std::hex;
            patStream << "[ sharedPtr = 0x"
                      << reinterpret_cast<size_t>(intPtr.get()) << " ]";
        } break;
        case bmqt::CorrelationId::e_AUTO_VALUE: {
            obj = bmqt::CorrelationId::autoValue();
            patStream << "[ autoValue = 1 ]";
        } break;
        case bmqt::CorrelationId::e_UNSET: {
            // obj has e_UNSET type
            patStream << "[ \"* unset *\" ]";
        } break;
        default: {
            BSLS_ASSERT_OPT(false && "Unknown correlationId type");
        }
        }

        objStream << obj;

        BMQTST_ASSERT_EQ(objStream.str(), patStream.str());

        objStream.reset();
        obj.print(objStream, 0, -1);

        BMQTST_ASSERT_EQ(objStream.str(), patStream.str());
    }

    PV("Bad stream test");

    bmqt::CorrelationId obj;
    bmqu::MemOutStream  stream(bmqtst::TestHelperUtil::allocator());

    stream << "BAD STREAM";
    stream.clear(bsl::ios_base::badbit);
    stream << obj;

    BMQTST_ASSERT_EQ(stream.str(), "BAD STREAM");
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 7: test7_printTest(); break;
    case 6: test6_hashAppend(); break;
    case 5: test5_autoValue(); break;
    case 4: test4_smartPointers(); break;
    case 3: test3_compare(); break;
    case 2: test2_copyAndAssign(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
