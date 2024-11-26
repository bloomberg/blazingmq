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

// bmqt_messageguid.t.cpp                                             -*-C++-*-
#include <bmqt_messageguid.h>

// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <bsls_alignmentfromtype.h>

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
// Concerns:
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    PV("Test some invalid guids");
    bmqt::MessageGUID obj1;
    ASSERT_EQ(true, obj1.isUnset());

    bmqt::MessageGUID obj2;
    obj1 = obj2;

    ASSERT_EQ(true, obj1.isUnset());
    ASSERT_EQ(true, obj2.isUnset());

    bmqt::MessageGUID obj3(obj1);
    ASSERT_EQ(true, obj3.isUnset());

    PV("Test some valid & invalid hex reps");
    const char k_INVALID_HEX_REP[] = "abcdefghijklmnopqrstuvwxyz012345";
    const char k_VALID_HEX_REP[]   = "ABCDEF0123456789ABCDEF0123456789";

    ASSERT_EQ(false,
              bmqt::MessageGUID::isValidHexRepresentation(k_INVALID_HEX_REP));

    ASSERT_EQ(true,
              bmqt::MessageGUID::isValidHexRepresentation(k_VALID_HEX_REP));

    PV("Create guid g4 from valid hex rep");
    bmqt::MessageGUID obj4;
    obj4.fromHex(k_VALID_HEX_REP);
    ASSERT_EQ(false, obj4.isUnset());

    PV("obj4 -> binary -> obj5");
    unsigned char binObj4[bmqt::MessageGUID::e_SIZE_BINARY];
    obj4.toBinary(binObj4);
    bmqt::MessageGUID obj5;
    obj5.fromBinary(binObj4);
    ASSERT_EQ(obj4, obj5);

    PV("obj4 -> hex -> obj6");
    // (note that here, 'hex' must be equal to VALID_HEX_REP
    char hexObj4[bmqt::MessageGUID::e_SIZE_HEX];
    obj4.toHex(hexObj4);
    bmqt::MessageGUID obj6;
    obj6.fromHex(hexObj4);
    ASSERT_EQ(obj4, obj6);

    PV("Create guid from valid hex rep");
    const char hexObj7[] = "00000000010EA8F9515DCACE04742D2E";
    // For above guid:
    //   Version....: [0]
    //   Counter....: [0]
    //   Timer tick.: [297593876864458]
    //   BrokerId...: [CE04742D2E]

    ASSERT_EQ(true, bmqt::MessageGUID::isValidHexRepresentation(hexObj7));

    bmqt::MessageGUID obj7;
    obj7.fromHex(hexObj7);
    ASSERT_EQ(false, obj7.isUnset());

    unsigned char binObj7[bmqt::MessageGUID::e_SIZE_BINARY];
    obj7.toBinary(binObj7);

    PV("Create guids from valid hex and bin reps");
    bmqt::MessageGUID obj8;

    obj8.fromBinary(binObj7);
    ASSERT_EQ(obj7, obj8);

    bmqt::MessageGUID obj9;
    obj9.fromHex(hexObj7);
    ASSERT_EQ(obj7, obj9);
}

static void test2_streamout()
{
    bmqtst::TestHelper::printTestName("STREAM OUT");

    // Create guid from valid hex rep

    {
        PV("Unset GUID");
        bmqt::MessageGUID obj;

        bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());
        osstr << obj;

        ASSERT_EQ("** UNSET **", osstr.str());
    }

    {
        PV("Valid GUID");
        const char k_HEX_G[] = "0000000000003039CD8101000000270F";

        ASSERT_EQ(true, bmqt::MessageGUID::isValidHexRepresentation(k_HEX_G));

        bmqt::MessageGUID obj;
        obj.fromHex(k_HEX_G);

        bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());
        osstr << obj;

        bsl::string guidStr(k_HEX_G,
                            bmqt::MessageGUID::e_SIZE_HEX,
                            bmqtst::TestHelperUtil::allocator());

        ASSERT_EQ(guidStr, osstr.str());

        PV("  GUID [" << osstr.str() << "]");
    }

    {
        PV("Multiline indented print");
        const char k_HEX_G[]    = "0000000000003039CD8101000000270F";
        const char k_EXPECTED[] = "        0000000000003039CD8101000000270F\n";

        ASSERT_EQ(true, bmqt::MessageGUID::isValidHexRepresentation(k_HEX_G));

        bmqt::MessageGUID obj;
        obj.fromHex(k_HEX_G);

        bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());
        obj.print(osstr, 2, 4);

        ASSERT_EQ(k_EXPECTED, osstr.str());

        PV("  GUID [" << osstr.str() << "]");
    }

    {
        PV("Bad stream");

        bmqt::MessageGUID obj;

        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        out.setstate(bsl::ios_base::badbit);
        obj.print(out, 0, -1);

        ASSERT_EQ(out.str(), "");
    }
}

static void test3_alignment()
// ------------------------------------------------------------------------
// ALIGNMENT TEST
//
// Concerns:
//   Ensure 4 byte alignment of bmqt_messageguid
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALIGNMENT");

    const size_t k_EXPECTED_ALIGNMENT = 1;
    const size_t k_REAL_ALIGNMENT =
        bsls::AlignmentFromType<bmqt::MessageGUID>::VALUE;

    ASSERT_EQ(k_EXPECTED_ALIGNMENT, k_REAL_ALIGNMENT);
}

static void test4_hashAppend()
// ------------------------------------------------------------------------
// TEST HASH APPEND
//
// Concerns:
//   Ensure that 'hashAppend' on 'bmqt::MessageGUID' is functional.
//
// Plan:
//  1) Generate a 'bmqt::MessageGUID' object, compute its hash using
//     'bmqt::MessageGUIDHashAlgo' and verify that 'hashAppend' on this
//     object is deterministic by comparing the hash value over many
//     iterations.
//
// Testing:
//   template <class HASH_ALGORITHM>
//   void
//   hashAppend(HASH_ALGORITHM&          hashAlgo,
//              const bmqt::MessageGUID& mesGuid)
//   bmqt::MessageGUIDHashAlgo
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HASH APPEND");

    PV("HASH FUNCTION DETERMINISTIC");

    // Create guid from valid hex rep
    const char hexObj[] = "00000000010EA8F9515DCACE04742D2E";

    ASSERT_EQ(true, bmqt::MessageGUID::isValidHexRepresentation(hexObj));

    bmqt::MessageGUID obj;
    obj.fromHex(hexObj);
    ASSERT_EQ(false, obj.isUnset());

    const size_t                               k_NUM_ITERATIONS = 1000;
    bslh::Hash<bmqt::MessageGUIDHashAlgo>      hasher;
    bslh::Hash<bmqt::MessageGUID>::result_type firstHash = hasher(obj);

    for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
        bmqt::MessageGUIDHashAlgo algo;
        hashAppend(algo, obj);
        bslh::Hash<bmqt::MessageGUID>::result_type currHash =
            algo.computeHash();
        PVVV("[" << i << "] hash: " << currHash);
        ASSERT_EQ_D(i, currHash, firstHash);
    }
}

static void test5_comparisonOperators()
// ------------------------------------------------------------------------
// COMPARISON OPERATORS TEST
//
// Concerns:
//   Ensure that comparison operators on 'bmqt::MessageGUID' is functional.
//
// Plan:
//   1) Create 3 bmqt::MessageGUID objects.
//   2) Load char string reflecting hex value to given messages in the way
//      that two objects are initialized by the same value and one object
//      is initialized by value lesser then two previous.
//   3) Check that object initialized by different value are not the same
//      via ASSERT_NE.
//   4) Check that object initialized by the same value are reflected as
//      equal via ASSERT_EQ.
//   5) Check that object initialized by the lesser value are reflected as
//      lesser via '<' operator.
//
// Testing:
//   * MessageGUIDLess::operator()
//   * free function bmqt::operator!=(const bmqt::MessageGUID& lhs,
//                                    const bmqt::MessageGUID& rhs)
//   * free function bmqt::operator<(const bmqt::MessageGUID& lhs,
//                                   const bmqt::MessageGUID& rhs)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("COMPARISON OPERATORS");

    // Lesser value
    const char k_HEX_G1[] = "0000000000003039CD8101000000270F";

    // Greater object
    const char k_HEX_G2[] = "0000000000004049CD9202000000380F";

    PV("INIT MessageGUID objects");
    bmqt::MessageGUID obj1, obj2, obj3;

    // One 'lesser' object
    obj1.fromHex(k_HEX_G1);

    // Two 'greater' objects
    obj2.fromHex(k_HEX_G2);
    obj3.fromHex(k_HEX_G2);

    PV("EQUAL TO OPERATOR CHECK");
    ASSERT(obj1 != obj2);

    PV("NOT EQUAL TO OPERATOR CHECK");
    ASSERT(obj2 == obj3);

    PV("LESSER THAN OPERATOR CHECK");
    ASSERT(obj1 < obj2);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 5: test5_comparisonOperators(); break;
    case 4: test4_hashAppend(); break;
    case 3: test3_alignment(); break;
    case 2: test2_streamout(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
