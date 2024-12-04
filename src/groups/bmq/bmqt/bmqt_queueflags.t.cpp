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

// bmqt_queueflags.t.cpp                                              -*-C++-*-
#include <bmqt_queueflags.h>

// BMQ
#include <bmqu_memoutstream.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_ios.h>
#include <bsl_string.h>
#include <bslmf_assert.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {
const bsls::Types::Uint64 k_ALL_FLAGS_SETTED = bmqt::QueueFlags::e_ACK |
                                               bmqt::QueueFlags::e_READ |
                                               bmqt::QueueFlags::e_WRITE |
                                               bmqt::QueueFlags::e_ADMIN;

BSLMF_ASSERT(k_ALL_FLAGS_SETTED < 1 << 10);
// Compile-time assert. Check if k_ALL_FLAGS_SETTED is too big. Given
// constant variable defines number of iterations needed to test all
// possible input test vectors. '1 << 10' is arbitrary big enough value.
// (Note that this value should be at least bigger then sum of all flags).
// If you want to modify it, please take to the account hazard of hanging
// in cycle.
}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqu::MemOutStream  errorOs(bmqtst::TestHelperUtil::allocator());
    int                 rc;
    bsls::Types::Uint64 flags1 = 0;
    bsls::Types::Uint64 flags2 = 0;

    PV("Testing QueueFlagsUtil::isValid");
    ASSERT_EQ(bmqt::QueueFlagsUtil::isValid(errorOs, flags1), false);
    ASSERT_EQ(errorOs.str(),
              "At least one of 'READ' or 'WRITE' mode must be set.");
    errorOs.reset();

    flags1 |= bmqt::QueueFlags::e_WRITE;
    flags1 |= bmqt::QueueFlags::e_ACK;
    ASSERT_EQ(bmqt::QueueFlagsUtil::isValid(errorOs, flags1), true);

    errorOs.reset();
    flags1 |= bmqt::QueueFlags::e_ADMIN;
    ASSERT_EQ(bmqt::QueueFlagsUtil::isValid(errorOs, flags1), false);
    ASSERT_EQ(errorOs.str(),
              "'ADMIN' mode is valid only for BlazingMQ admin tasks.");
    flags1 &= ~bmqt::QueueFlags::e_ADMIN;
    errorOs.reset();

    PV("Testing prettyPrint");
    bmqu::MemOutStream osPrint(bmqtst::TestHelperUtil::allocator());
    bmqt::QueueFlagsUtil::prettyPrint(osPrint, flags2);
    ASSERT_EQ(osPrint.str(), "");

    bmqt::QueueFlagsUtil::prettyPrint(osPrint, flags1);
    ASSERT_EQ(osPrint.str(), "WRITE,ACK");

    PV("Testing fromString");
    rc = bmqt::QueueFlagsUtil::fromString(errorOs, &flags2, osPrint.str());
    PV("Error out: '" << errorOs.str() << "'");
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(flags2, flags1);

    flags2 = 0;
    rc     = bmqt::QueueFlagsUtil::fromString(errorOs, &flags2, "READ");
    ASSERT_EQ(rc, 0);
    ASSERT(bmqt::QueueFlagsUtil::isReader(flags2));
    ASSERT(!bmqt::QueueFlagsUtil::isWriter(flags2));

    bsl::string flagsStr("WRITE,INVALID,READ,INVALID2",
                         bmqtst::TestHelperUtil::allocator());
    rc = bmqt::QueueFlagsUtil::fromString(errorOs, &flags2, flagsStr);
    ASSERT_NE(rc, 0);
    ASSERT_EQ(errorOs.str(), "Invalid flag(s) 'INVALID','INVALID2'");
}

static void test2_additionsRemovals()
{
    bmqtst::TestHelper::printTestName("ADDITIONS / REMOVALS");

    // Short alias
    const int k_ADMIN = bmqt::QueueFlags::e_ADMIN;
    const int k_READ  = bmqt::QueueFlags::e_READ;
    const int k_WRITE = bmqt::QueueFlags::e_WRITE;
    const int k_ACK   = bmqt::QueueFlags::e_ACK;

    const struct TestData {
        int                 d_line;
        bool                d_isAdditions;
        bsls::Types::Uint64 d_oldFlags;
        bsls::Types::Uint64 d_newFlags;
        bsls::Types::Uint64 d_expected;
    } k_DATA[] = {
        {L_, true, 0, 0, 0},
        {L_, true, k_ADMIN, 0, 0},
        {L_, true, 0, k_ACK, k_ACK},
        {L_, true, k_ADMIN | k_WRITE, k_ADMIN, 0},
        {L_, true, k_ADMIN | k_WRITE, k_ADMIN | k_READ, k_READ},
        {L_, true, k_ADMIN | k_WRITE, k_READ, k_READ},
        {L_, true, 0, k_ADMIN | k_WRITE, k_ADMIN | k_WRITE},
        {L_, false, 0, 0, 0},
        {L_, false, 0, k_ACK, 0},
        {L_, false, k_ADMIN, 0, k_ADMIN},
        {L_, false, k_ADMIN, k_ADMIN | k_WRITE, 0},
        {L_, false, k_ADMIN | k_WRITE, k_ADMIN | k_READ, k_WRITE},
        {L_, false, k_ADMIN | k_WRITE, k_READ, k_ADMIN | k_WRITE},
        {L_, false, 0, k_ADMIN | k_WRITE, 0},
        {L_, false, k_ADMIN | k_WRITE, 0, k_ADMIN | k_WRITE},
    };
    const int k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (int idx = 0; idx < k_NUM_DATA; ++idx) {
        const TestData& test = k_DATA[idx];

        bsls::Types::Uint64 diffFlags = 0;
        if (test.d_isAdditions) {
            diffFlags = bmqt::QueueFlagsUtil::additions(test.d_oldFlags,
                                                        test.d_newFlags);
        }
        else {
            diffFlags = bmqt::QueueFlagsUtil::removals(test.d_oldFlags,
                                                       test.d_newFlags);
        }
        PVV("Line: " << test.d_line << ", Old: " << test.d_oldFlags
                     << ", New: " << test.d_newFlags << ", Diff: " << diffFlags
                     << ", Expected: " << test.d_expected);
        ASSERT_EQ(diffFlags, test.d_expected);
    }
}

static void test3_printTest()
{
    bmqtst::TestHelper::printTestName("PRINT");

    PV("Testing print");

    BSLMF_ASSERT(bmqt::QueueFlags::e_ADMIN ==
                 bmqt::QueueFlags::k_LOWEST_SUPPORTED_QUEUE_FLAG);

    BSLMF_ASSERT(bmqt::QueueFlags::e_ACK ==
                 bmqt::QueueFlags::k_HIGHEST_SUPPORTED_QUEUE_FLAG);

    struct Test {
        bmqt::QueueFlags::Enum d_type;
        const char*            d_expected;
    } k_DATA[] = {{bmqt::QueueFlags::e_ADMIN, "ADMIN"},
                  {bmqt::QueueFlags::e_READ, "READ"},
                  {bmqt::QueueFlags::e_WRITE, "WRITE"},
                  {bmqt::QueueFlags::e_ACK, "ACK"},
                  {static_cast<bmqt::QueueFlags::Enum>(-1), "(* UNKNOWN *)"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&        test = k_DATA[idx];
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator());

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        bmqt::QueueFlags::print(out, test.d_type, 0, -1);

        ASSERT_EQ(out.str(), "");

        out.clear();
        bmqt::QueueFlags::print(out, test.d_type, 0, -1);

        ASSERT_EQ(out.str(), expected.str());

        out.reset();
        out << test.d_type;

        ASSERT_EQ(out.str(), expected.str());
    }
}

static void test4_empty()
{
    bmqtst::TestHelper::printTestName("EMPTY");

    bsls::Types::Uint64 flags = 0;

    // EMPTY
    PV("Testing empty state getters")
    flags = bmqt::QueueFlagsUtil::empty();
    ASSERT_EQ(flags, 0UL);
    ASSERT_EQ(bmqt::QueueFlagsUtil::isEmpty(flags), true);

    // NOT EMPTY
    flags = 0xffffffff;
    ASSERT_EQ(bmqt::QueueFlagsUtil::isEmpty(flags), false);
}

static void test5_setFlag()
{
    bmqtst::TestHelper::printTestName("SET FLAG");

    PV("Test flag setter")
    for (bsls::Types::Uint64 flags = 0; flags <= k_ALL_FLAGS_SETTED; flags++) {
        // ACK flag
        bsls::Types::Uint64 flags0 = flags;
        bmqt::QueueFlagsUtil::setAck(&flags0);
        ASSERT_EQ_D("Test set ACK", flags | bmqt::QueueFlags::e_ACK, flags0)

        // READ flag
        flags0 = flags;
        bmqt::QueueFlagsUtil::setReader(&flags0);
        ASSERT_EQ_D("Test set READ", flags | bmqt::QueueFlags::e_READ, flags0)

        // WRITE flag
        flags0 = flags;
        bmqt::QueueFlagsUtil::setWriter(&flags0);
        ASSERT_EQ_D("Test set WRITE",
                    flags | bmqt::QueueFlags::e_WRITE,
                    flags0)

        // ADMIN flag
        flags0 = flags;
        bmqt::QueueFlagsUtil::setAdmin(&flags0);
        ASSERT_EQ_D("Test set ADMIN",
                    flags | bmqt::QueueFlags::e_ADMIN,
                    flags0)
    }
}

static void test6_unsetFlag()
{
    bmqtst::TestHelper::printTestName("UNSET FLAG");

    PV("Test flag unsetter")
    for (bsls::Types::Uint64 flags = 0; flags <= k_ALL_FLAGS_SETTED; flags++) {
        // ACK flag
        bsls::Types::Uint64 flags0 = flags;
        bmqt::QueueFlagsUtil::unsetAck(&flags0);
        ASSERT_EQ_D("Test unset ACK", flags & ~bmqt::QueueFlags::e_ACK, flags0)

        // READ flag
        flags0 = flags;
        bmqt::QueueFlagsUtil::unsetReader(&flags0);
        ASSERT_EQ_D("Test unset READ",
                    flags & ~bmqt::QueueFlags::e_READ,
                    flags0)

        // WRITE flag
        flags0 = flags;
        bmqt::QueueFlagsUtil::unsetWriter(&flags0);
        ASSERT_EQ_D("Test unset WRITE",
                    flags & ~bmqt::QueueFlags::e_WRITE,
                    flags0)

        // ADMIN flag
        flags0 = flags;
        bmqt::QueueFlagsUtil::unsetAdmin(&flags0);
        ASSERT_EQ_D("Test unset ADMIN",
                    flags & ~bmqt::QueueFlags::e_ADMIN,
                    flags0)
    }
}

static void test7_getFlag()
{
    bmqtst::TestHelper::printTestName("GET FLAG");

    PV("Test flag getter")
    for (bsls::Types::Uint64 flags = 0; flags <= k_ALL_FLAGS_SETTED; flags++) {
        // ACK flag
        bsls::Types::Uint64 flags0 = flags;
        ASSERT_EQ_D("Test ACK flag check",
                    bmqt::QueueFlagsUtil::isAck(flags0),
                    static_cast<bool>(flags & bmqt::QueueFlags::e_ACK))

        // READ flag
        flags0 = flags;
        ASSERT_EQ_D("Test READ flag check",
                    bmqt::QueueFlagsUtil::isReader(flags0),
                    static_cast<bool>(flags & bmqt::QueueFlags::e_READ))

        // WRITE flag
        flags0 = flags;
        ASSERT_EQ_D("Test WRITE flag check",
                    bmqt::QueueFlagsUtil::isWriter(flags0),
                    static_cast<bool>(flags & bmqt::QueueFlags::e_WRITE))

        // ADMIN flag
        flags0 = flags;
        ASSERT_EQ_D("Test ADMIN flag check",
                    bmqt::QueueFlagsUtil::isAdmin(flags0),
                    static_cast<bool>(flags & bmqt::QueueFlags::e_ADMIN))
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 7: test7_getFlag(); break;
    case 6: test6_unsetFlag(); break;
    case 5: test5_setFlag(); break;
    case 4: test4_empty(); break;
    case 3: test3_printTest(); break;
    case 2: test2_additionsRemovals(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
