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

// z_bmqt_correlationid.t.cpp -*-C++-*-
#include <bmqt_correlationid.h>
#include <z_bmqt_correlationid.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_ios.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslh_defaulthashalgorithm.h>
#include <bslh_hash.h>
#include <bsls_types.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static const bmqt::CorrelationId&
toCppType(const z_bmqt_CorrelationId* correlationId)
{
    return *(reinterpret_cast<const bmqt::CorrelationId*>(correlationId));
}

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
    mwctst::TestHelper::printTestName("BREATHING TEST");

    // An object for testing pointers
    bsl::string  foo("foo", s_allocator_p);
    bsl::string* fooPtr = &foo;

    PV("Test unset correlationId");
    z_bmqt_CorrelationId* unset;
    z_bmqt_CorrelationId__create(&unset);
    ASSERT_EQ(z_bmqt_CorrelationId__isUnset(unset), true);
    z_bmqt_CorrelationId__delete(&unset);

    PV("Test initialization by Int64");
    z_bmqt_CorrelationId* id;
    z_bmqt_CorrelationId__createFromNumeric(&id, -1);
    ASSERT_EQ(z_bmqt_CorrelationId__isNumeric(id), true);
    ASSERT_EQ(z_bmqt_CorrelationId__isPointer(id), false);
    ASSERT_EQ(z_bmqt_CorrelationId__isUnset(id), false);
    ASSERT_EQ(-1, z_bmqt_CorrelationId__theNumeric(id));

    PV("Test initialization by integer");
    bsls::Types::Int64    numeric = 0x5a5a5a5a5a5a5a5a;
    z_bmqt_CorrelationId* numericId;
    z_bmqt_CorrelationId__createFromNumeric(&numericId, numeric);
    ASSERT_EQ(z_bmqt_CorrelationId__isPointer(numericId), false);
    ASSERT_EQ(numeric, z_bmqt_CorrelationId__theNumeric(numericId));

    PV("Test copy constructor");
    z_bmqt_CorrelationId* newNumericId;
    z_bmqt_CorrelationId__createCopy(&newNumericId, numericId);
    ASSERT_EQ(z_bmqt_CorrelationId__isPointer(newNumericId), false);
    ASSERT_EQ(numeric, z_bmqt_CorrelationId__theNumeric(newNumericId));

    PV("Test initialization by pointer");
    bmqt::CorrelationId ptrId(fooPtr);
    ASSERT_EQ(ptrId.isPointer(), true);
    ASSERT_EQ(fooPtr, ptrId.thePointer());

    PV("Test copy constructor");
    bmqt::CorrelationId newPtrId(ptrId);
    ASSERT_EQ(newPtrId.isPointer(), true);
    ASSERT_EQ(fooPtr, newPtrId.thePointer());

    PV("Test 'Set' methods");
    z_bmqt_CorrelationId__setPointer(id, fooPtr);
    ASSERT_EQ(toCppType(id), bmqt::CorrelationId(fooPtr));

    z_bmqt_CorrelationId__setNumeric(id, numeric);
    ASSERT_EQ(toCppType(id), bmqt::CorrelationId(numeric));

    PV("Test 'makeUnset' method");
    z_bmqt_CorrelationId__makeUnset(id);
    ASSERT_EQ(toCppType(id), bmqt::CorrelationId());
    ASSERT_EQ(z_bmqt_CorrelationId__isUnset(id), true);

    z_bmqt_CorrelationId__delete(&id);
    z_bmqt_CorrelationId__delete(&newNumericId);
    z_bmqt_CorrelationId__delete(&numericId);
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
    mwctst::TestHelper::printTestName("COPY AND ASSIGN");

    // An object for testing pointers
    char fooPtr[4] = {'f', 'o', 'o', '\0'};

    // conversions to bmqt::CorrelationId and back
    bsls::Types::Int64 numeric = 0xA5A5A5A5A5A5A5A5;
    void*              ptr     = fooPtr + 1;

    z_bmqt_CorrelationId* numericId = NULL;
    z_bmqt_CorrelationId* ptrId     = NULL;
    z_bmqt_CorrelationId__createFromNumeric(&numericId, numeric);
    z_bmqt_CorrelationId__createFromPointer(&ptrId, ptr);

    PV("Test copy")
    {
        z_bmqt_CorrelationId* numericIdCopy = NULL;
        z_bmqt_CorrelationId__createCopy(&numericIdCopy, numericId);
        ASSERT_EQ(z_bmqt_CorrelationId__isPointer(numericIdCopy), false);
        ASSERT_EQ(z_bmqt_CorrelationId__theNumeric(numericIdCopy), numeric);

        z_bmqt_CorrelationId* ptrIdCopy = NULL;
        z_bmqt_CorrelationId__createCopy(&ptrIdCopy, ptrId);
        ASSERT_EQ(z_bmqt_CorrelationId__isPointer(ptrIdCopy), true);
        ASSERT_EQ(z_bmqt_CorrelationId__thePointer(ptrIdCopy), ptr);

        z_bmqt_CorrelationId__delete(&numericIdCopy);
        z_bmqt_CorrelationId__delete(&ptrIdCopy);
    }

    PV("Test assignment");
    {
        z_bmqt_CorrelationId* numericIdCopy = NULL;
        z_bmqt_CorrelationId__create(&numericIdCopy);
        z_bmqt_CorrelationId__assign(&numericIdCopy, numericId);
        ASSERT_EQ(z_bmqt_CorrelationId__isPointer(numericIdCopy), false);
        ASSERT_EQ(z_bmqt_CorrelationId__theNumeric(numericIdCopy), numeric);

        z_bmqt_CorrelationId* ptrIdCopy = NULL;
        z_bmqt_CorrelationId__create(&ptrIdCopy);
        z_bmqt_CorrelationId__assign(&ptrIdCopy, ptrId);
        ASSERT_EQ(z_bmqt_CorrelationId__isPointer(ptrIdCopy), true);
        ASSERT_EQ(z_bmqt_CorrelationId__thePointer(ptrIdCopy), ptr);

        z_bmqt_CorrelationId__delete(&numericIdCopy);
        z_bmqt_CorrelationId__delete(&ptrIdCopy);
    }

    z_bmqt_CorrelationId__delete(&numericId);
    z_bmqt_CorrelationId__delete(&ptrId);
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
    mwctst::TestHelper::printTestName("COMPARE");

    // Test compare operator
    bmqt::CorrelationIdLess less;
    const int               val1    = 1;
    const int               val4    = 4;
    int*                    intPtr1 = new int(val1);
    int*                    intPtr4 = new int(val4);

    z_bmqt_CorrelationId* numericId1;
    z_bmqt_CorrelationId* numericId4;
    z_bmqt_CorrelationId* ptrId1;
    z_bmqt_CorrelationId* ptrId4;
    z_bmqt_CorrelationId* autoId1;
    z_bmqt_CorrelationId* autoId2;

    z_bmqt_CorrelationId__createFromNumeric(&numericId1, val1);
    z_bmqt_CorrelationId__createFromNumeric(&numericId4, val4);
    z_bmqt_CorrelationId__createFromPointer(&ptrId1, intPtr1);
    z_bmqt_CorrelationId__createFromPointer(&ptrId4, intPtr4);
    z_bmqt_CorrelationId__autoValue(&autoId1);
    z_bmqt_CorrelationId__autoValue(&autoId2);

    ASSERT_LT(z_bmqt_CorrelationId__compare(numericId1, numericId4), 0);
    ASSERT_GT(z_bmqt_CorrelationId__compare(numericId4, numericId1), 0);

    ASSERT_LT(z_bmqt_CorrelationId__compare(ptrId1, ptrId4), 0);
    ASSERT_GT(z_bmqt_CorrelationId__compare(ptrId4, ptrId1), 0);

    ASSERT_LT(z_bmqt_CorrelationId__compare(autoId1, autoId2), 0);
    ASSERT_GT(z_bmqt_CorrelationId__compare(autoId2, autoId1), 0);

    ASSERT_LT(z_bmqt_CorrelationId__compare(numericId1, ptrId1), 0);
    ASSERT_GT(z_bmqt_CorrelationId__compare(ptrId1, numericId1), 0);

    ASSERT_LT(z_bmqt_CorrelationId__compare(numericId4, ptrId1), 0);
    ASSERT_GT(z_bmqt_CorrelationId__compare(ptrId4, numericId1), 0);

    ASSERT_LT(z_bmqt_CorrelationId__compare(numericId1, autoId1), 0);
    ASSERT_GT(z_bmqt_CorrelationId__compare(autoId1, numericId1), 0);

    z_bmqt_CorrelationId__delete(&numericId1);
    z_bmqt_CorrelationId__delete(&numericId4);
    z_bmqt_CorrelationId__delete(&ptrId1);
    z_bmqt_CorrelationId__delete(&ptrId4);
    z_bmqt_CorrelationId__delete(&autoId1);
    z_bmqt_CorrelationId__delete(&autoId2);

    delete intPtr1;
    delete intPtr4;
}

static void test4_autoValue()
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
    mwctst::TestHelper::printTestName("AUTO VALUE");

    z_bmqt_CorrelationId* auto1 = NULL;
    z_bmqt_CorrelationId* auto2 = NULL;

    z_bmqt_CorrelationId__autoValue(&auto1);
    z_bmqt_CorrelationId__autoValue(&auto2);

    ASSERT_EQ(z_bmqt_CorrelationId__isAutoValue(auto1), true);

    ASSERT_EQ(z_bmqt_CorrelationId__compare(auto1, auto1), 0);
    ASSERT_NE(z_bmqt_CorrelationId__compare(auto1, auto2), 0);

    z_bmqt_CorrelationId__delete(&auto1);
    z_bmqt_CorrelationId__delete(&auto2);
}

static void test5_toStringTest()
{
    mwctst::TestHelper::printTestName("TO STRING");

    PV("Testing print");
    char    fooPtr[4] = {'f', 'o', 'o', '\0'};
    int64_t numeric   = 0x5a5a5a5a5a5a5a5a;
    size_t  t         = bmqt::CorrelationId::e_NUMERIC;

    for (; t < z_bmqt_CorrelationId::ec_UNSET + 1; ++t) {
        if (t == z_bmqt_CorrelationId::ec_SHARED_PTR) {
            // No shared ptr for C
            continue;
        }
        z_bmqt_CorrelationId* obj = NULL;
        char*                 out;
        mwcu::MemOutStream    patStream(s_allocator_p);
        mwcu::MemOutStream    objStream(s_allocator_p);
        switch (t) {
        case z_bmqt_CorrelationId::ec_NUMERIC: {
            z_bmqt_CorrelationId__createFromNumeric(&obj, numeric);
            patStream << "[ numeric = " << numeric << " ]";
        } break;
        case z_bmqt_CorrelationId::ec_POINTER: {
            z_bmqt_CorrelationId__createFromPointer(&obj, fooPtr);
            patStream << bsl::noshowbase << std::hex;
            patStream << "[ pointer = 0x" << reinterpret_cast<size_t>(fooPtr)
                      << " ]";
        } break;
        case z_bmqt_CorrelationId::ec_AUTO_VALUE: {
            z_bmqt_CorrelationId__autoValue(&obj);
            patStream << "[ autoValue = 1 ]";
        } break;
        case z_bmqt_CorrelationId::ec_UNSET: {
            // obj has e_UNSET type
            z_bmqt_CorrelationId__create(&obj);
            patStream << "[ \"* unset *\" ]";
        } break;
        default: {
            BSLS_ASSERT_OPT(false && "Unknown correlationId type");
        }
        }

        z_bmqt_CorrelationId__toString(obj, &out);

        objStream.write(out, strlen(out));
        ASSERT_EQ(objStream.str(), patStream.str());
        delete[] out;

        z_bmqt_CorrelationId__delete(&obj);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 5: test5_toStringTest(); break;
    case 4: test4_autoValue(); break;
    case 3: test3_compare(); break;
    case 2: test2_copyAndAssign(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_DEFAULT);

    return 0;
}
