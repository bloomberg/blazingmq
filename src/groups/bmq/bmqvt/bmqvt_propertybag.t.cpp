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

// bmqvt_propertybag.t.cpp -*-C++-*-
#include <bmqvt_propertybag.h>

#include <bdlma_localsequentialallocator.h>
#include <bsl_sstream.h>
#include <bslma_testallocator.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace bmqvt;

// ========================================================================
//                                   HELPERS
// ------------------------------------------------------------------------

/// Validates that the value at the specified `key` from the specified `bag`
/// is of type `T` having the specified `expected` value.  Use the specified
/// `line` to provide meaningful information in case it is not.
template <typename T>
void checkValue(int                      line,
                const PropertyBag&       bag,
                const bslstl::StringRef& key,
                const T&                 expected)
{
    T bagValue;

    bool ret = bag.load(&bagValue, key);
    if (!ret) {
        ASSERT_D("line " << line, false);
        return;  // RETURN
    }

    ASSERT_EQ_D("line " << line, bagValue, expected);
}

/// Validates that the value at the specified `key` from the specified `bag`
/// is of pointer type having the specified `expected` value.  Use the
/// specified `line` to provide meaningful information in case it is not.
static void checkValueSp(int                          line,
                         const PropertyBag&           bag,
                         const bslstl::StringRef&     key,
                         const bsl::shared_ptr<void>& expected)
{
    bslma::ManagedPtr<PropertyBagValue> bagValue;

    bool ret = bag.load(&bagValue, key);
    if (!ret) {
        ASSERT_D("line " << line, false);
        return;  // RETURN
    }

    if (!bagValue->isPtr()) {
        PRINT(*bagValue);
        ASSERT_D("line " << line << ": Not a pointer", false);
        return;  // RETURN
    }

    ASSERT_EQ_D("lines " << line, bagValue->thePtr(), expected);
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("Breathing Test");

    const bslstl::StringRef k_strVal("theQuickBrownFoxJumped");
    bsl::shared_ptr<int>    intSp;
    intSp.createInplace(bmqtst::TestHelperUtil::allocator(), 1);

    PropertyBag obj(bmqtst::TestHelperUtil::allocator());

    ASSERT_EQ(obj.allocator(), bmqtst::TestHelperUtil::allocator());

    // Set a value and read it back
    obj.set("intVal", 1)
        .set("int64Val", bsls::Types::Int64(123))
        .set("strVal", k_strVal)
        .set("ptrVal", intSp);

    checkValue(L_, obj, "intVal", 1);
    checkValue(L_, obj, "int64Val", bsls::Types::Int64(123));
    checkValue(L_, obj, "strVal", k_strVal);
    checkValueSp(L_, obj, "ptrVal", intSp);

    // Set a second value and check both
    obj.set("intVal2", 2);
    checkValue(L_, obj, "intVal", 1);
    checkValue(L_, obj, "intVal2", 2);

    // Change value of first property
    obj.set("intVal", 3);
    checkValue(L_, obj, "intVal", 3);

    // Check accessing an unknown property
    ASSERT(!obj.load(static_cast<int*>(0), "dummy"));
    ASSERT(!obj.load(static_cast<bslstl::StringRef*>(0), "dummy"));
    {
        bslma::ManagedPtr<PropertyBagValue> value;
        ASSERT(!obj.load(&value, "dummy"));
    }

    // Check unset property
    obj.unset("intVal");
    ASSERT(!obj.load(static_cast<int*>(0), "intVal"));

    // Unset unknown property
    obj.unset("intVal3");

    // Not able to load a string val into an int
    ASSERT(!obj.load(static_cast<int*>(0), "strVal"));

    // Verify PropertyBagValue accessors
    {
        bslma::ManagedPtr<PropertyBagValue> bagValue;

        bool loaded = obj.load(&bagValue, "intVal2");
        ASSERT(loaded);
        ASSERT_EQ(bagValue->name(), "intVal2");
        ASSERT(bagValue->isDatum());
        ASSERT(!bagValue->isPtr());

        loaded = obj.load(&bagValue, "ptrVal");
        ASSERT(loaded);
        ASSERT_EQ(bagValue->name(), "ptrVal");
        ASSERT(!bagValue->isDatum());
        ASSERT(bagValue->isPtr());
    }
}

static void test2_copyAndAssignment()
{
    bmqtst::TestHelper::printTestName("Copy and Assignment");

    const bslstl::StringRef k_strVal("theQuickBrownFoxJumped");
    bsl::shared_ptr<int>    intSp;
    intSp.createInplace(bmqtst::TestHelperUtil::allocator(), 1);

    PropertyBag obj1(bmqtst::TestHelperUtil::allocator());
    obj1.set("intVal", 5).set("strVal", k_strVal).set("ptrVal", intSp);

    {
        PV("Copy PropertyBag and check values in copied object");
        PropertyBag obj2(obj1, bmqtst::TestHelperUtil::allocator());
        checkValue(L_, obj2, "intVal", 5);
        checkValue(L_, obj2, "strVal", k_strVal);
        checkValueSp(L_, obj2, "ptrVal", intSp);

        // Update a value in obj2, ensure no change in obj1
        obj2.set("intVal", 13);
        checkValue(L_, obj1, "intVal", 5);
        checkValue(L_, obj2, "intVal", 13);
    }

    {
        PV("Assignment");
        PropertyBag obj3(bmqtst::TestHelperUtil::allocator());
        obj3.set("someIntVal", 3);

        obj3 = obj1;
        checkValue(L_, obj3, "intVal", 5);
        checkValueSp(L_, obj3, "ptrVal", intSp);
        checkValue(L_, obj3, "strVal", k_strVal);

        // Check "someIntVal" property got removed as part of assignment
        // operations
        ASSERT(!obj3.load(static_cast<int*>(0), "someIntVal"));
    }
}

static void test3_print()
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // We can't use 'bmqu::MemOutStream' because bmqu is above bmqvt, so use
    // a 'ostringstream', which allocates its string using the default
    // allocator.

    bmqtst::TestHelper::printTestName("Print");

    const bslstl::StringRef k_strVal("theQuickBrownFox");

    PropertyBag obj(bmqtst::TestHelperUtil::allocator());
    obj.set("IntVal", 5).set("strVal", k_strVal);

    {
        PV("Testing PropertyBagValue::print and <<(PropertyBagValue)");

        bsl::ostringstream out;
        bsl::string        expected("[ strVal = \"theQuickBrownFox\" ]",
                             bmqtst::TestHelperUtil::allocator());

        bslma::ManagedPtr<PropertyBagValue> bagValue;
        obj.load(&bagValue, "strVal");

        out.setstate(bsl::ios_base::badbit);
        bagValue->print(out, 0, -1);
        ASSERT_EQ(out.str(), "");

        out.clear();
        out.str("");
        bagValue->print(out, 0, -1);
        ASSERT_EQ(out.str(), expected);

        out.clear();
        out.str("");
        out << *bagValue;
        ASSERT_EQ(out.str(), expected);
    }

    {
        PV("Testing PropertyBag::print and <<(PropertyBag)");

        bsl::ostringstream out;

        bsl::string expected("[ strVal = \"theQuickBrownFox\" IntVal = 5 ]",
                             bmqtst::TestHelperUtil::allocator());

        out.setstate(bsl::ios_base::badbit);
        obj.print(out, 0, -1);
        ASSERT_EQ(out.str(), "");

        out.clear();
        out.str("");
        obj.print(out, 0, -1);
        ASSERT_EQ(out.str(), expected);

        out.clear();
        out.str("");
        out << obj;
        ASSERT_EQ(out.str(), expected);
    }
}

static void test4_loadAllImport()
{
    bmqtst::TestHelper::printTestName("loadAll/import");

    PropertyBag obj(bmqtst::TestHelperUtil::allocator());
    PropertyBag obj2(bmqtst::TestHelperUtil::allocator());

    obj.set("int1", 1);
    obj.set("int2", 2);

    bsl::vector<bslma::ManagedPtr<PropertyBagValue> > values(
        bmqtst::TestHelperUtil::allocator());
    obj.loadAll(&values);
    ASSERT_EQ(static_cast<int>(values.size()), 2);

    obj2.import(values);
    checkValue(L_, obj2, "int1", 1);
    checkValue(L_, obj2, "int2", 2);

    PropertyBag obj3(bmqtst::TestHelperUtil::allocator());
    obj3.import(*values[0]);
    obj3.import(*values[1]);
    checkValue(L_, obj3, "int1", 1);
    checkValue(L_, obj3, "int2", 2);

    obj3.set("intCpy", *values[0]);
    checkValue(L_, obj3, "intCpy", values[0]->theDatum().theInteger());
}

static void test5_propertyBagUtil()
{
    bmqtst::TestHelper::printTestName("PropertyBagUtil");

    bslstl::StringRef longStr(
        "theQuickBrownFoxALongStringThatWillForceAllocation");

    PropertyBag obj(bmqtst::TestHelperUtil::allocator());
    obj.set("int1", 1).set("int2", 2).set("str1", "abc").set("str2", longStr);

    bdlma::LocalSequentialAllocator<10 * 1024> arena;
    PropertyBag                                obj2(&arena);
    obj2.set("int3", 3).set("int1", 7);

    PropertyBagUtil::update(&obj2, obj);

    checkValue(L_, obj2, "int1", 1);
    checkValue(L_, obj2, "int2", 2);
    checkValue(L_, obj2, "int3", 3);
    checkValue(L_, obj2, "str1", bslstl::StringRef("abc"));
    checkValue(L_, obj2, "str2", longStr);

    // Make sure 'obj2's 'str2' is within 'arena's footprint, proving it was
    // copied
    bslstl::StringRef str2;
    obj2.load(&str2, "str2");

    char* arenaPtr = reinterpret_cast<char*>(&arena);
    ASSERT(str2.data() >= arenaPtr && str2.data() < arenaPtr + sizeof(arena));
}

static void test6_valueOverwrite()
{
    bmqtst::TestHelper::printTestName("ValueOverwrite");

    // In this test, we ensure that the underlying container used properly
    // handles inserting a new value for an existing key (see comment in
    // 'insertValueImp' in the component.

    // For that purpose, we force usage of a 'TestAllocator', which scribbles
    // deleted arena, implying that upon insertion of a new value, the previous
    // one (especially the buffer containing the string used as key) gets
    // overwritten.

    bslma::TestAllocator allocator;
    PropertyBag          obj(&allocator);

    bslstl::StringRef longStr(
        "theQuickBrownFoxALongStringThatWillForceAllocation");

    obj.set(longStr, 1);
    obj.set(longStr, 2);

    // If the map didn't handle the key correctly, upon insertion of the '2'
    // value, then the lookup will fail because the 'stringref' key now won't
    // point to the expected key string buffer.
    checkValue(L_, obj, longStr, 2);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 6: test6_valueOverwrite(); break;
    case 5: test5_propertyBagUtil(); break;
    case 4: test4_loadAllImport(); break;
    case 3: test3_print(); break;
    case 2: test2_copyAndAssignment(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
