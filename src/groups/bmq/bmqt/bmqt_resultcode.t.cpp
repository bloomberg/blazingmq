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

// bmqt_resultcode.t.cpp                                              -*-C++-*-
#include <bmqt_resultcode.h>

#include <../bmqp/bmqp_ctrlmsg_messages.h>  // Violating dependency.. used to
                                            // ensure synchronization between
                                            // two enums.

// BDE
#include <bsl_ios.h>
#include <bsl_string.h>

// MWC
#include <mwcu_memoutstream.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

const int k_MAX_ENUM_NAME_LENGTH = 100;

struct PrintTestData {
    int         d_line;
    int         d_type;
    const char* d_expected;
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    PV("Testing GenericResult");
    bmqt::GenericResult::Enum obj;
    const char*               str;
    bool                      res;

    PV("Testing toAscii");
    str = bmqt::GenericResult::toAscii(bmqt::GenericResult::e_TIMEOUT);
    ASSERT_EQ(bsl::strncmp(str, "TIMEOUT", k_MAX_ENUM_NAME_LENGTH), 0);

    PV("Testing fromAscii");
    res = bmqt::GenericResult::fromAscii(&obj, "NOT_CONNECTED");
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, bmqt::GenericResult::e_NOT_CONNECTED);
    res = bmqt::GenericResult::fromAscii(&obj, "invalid");
    ASSERT_EQ(res, false);

    PV("Testing: fromAscii(toAscii(value)) = value");
    res = bmqt::GenericResult::fromAscii(
        &obj,
        bmqt::GenericResult::toAscii(bmqt::GenericResult::e_REFUSED));
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, bmqt::GenericResult::e_REFUSED);

    PV("Testing: toAscii(fromAscii(value)) = value");
    res = bmqt::GenericResult::fromAscii(&obj, "CANCELED");
    ASSERT_EQ(res, true);
    str = bmqt::GenericResult::toAscii(obj);
    ASSERT_EQ(bsl::strncmp(str, "CANCELED", k_MAX_ENUM_NAME_LENGTH), 0);
}

static void test2_schemaConsistency()
{
    mwctst::TestHelper::printTestName("SCHEMA CONSISTENCY");

    PV("Ensuring GenericResult and bmqp_ctrlmsg::StatusCategory in sync");

    // We need to ensure that e_LAST == e_NOT_READY, to make sure that no new
    // values have been added..
    ASSERT_EQ(bmqt::GenericResult::e_LAST, bmqt::GenericResult::e_NOT_READY);

    // Validate each values to be equal
    ASSERT_EQ(static_cast<int>(bmqt::GenericResult::e_SUCCESS),
              static_cast<int>(bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    ASSERT_EQ(static_cast<int>(bmqt::GenericResult::e_UNKNOWN),
              static_cast<int>(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN));
    ASSERT_EQ(static_cast<int>(bmqt::GenericResult::e_TIMEOUT),
              static_cast<int>(bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));
    ASSERT_EQ(static_cast<int>(bmqt::GenericResult::e_NOT_CONNECTED),
              static_cast<int>(bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED));
    ASSERT_EQ(static_cast<int>(bmqt::GenericResult::e_CANCELED),
              static_cast<int>(bmqp_ctrlmsg::StatusCategory::E_CANCELED));
    ASSERT_EQ(static_cast<int>(bmqt::GenericResult::e_NOT_SUPPORTED),
              static_cast<int>(bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED));
    ASSERT_EQ(static_cast<int>(bmqt::GenericResult::e_REFUSED),
              static_cast<int>(bmqp_ctrlmsg::StatusCategory::E_REFUSED));
    ASSERT_EQ(
        static_cast<int>(bmqt::GenericResult::e_INVALID_ARGUMENT),
        static_cast<int>(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT));
    ASSERT_EQ(static_cast<int>(bmqt::GenericResult::e_NOT_READY),
              static_cast<int>(bmqp_ctrlmsg::StatusCategory::E_NOT_READY));
}

template <typename ENUM_TYPE, typename ARRAY, int SIZE>
static void printEnumHelper(ARRAY (&data)[SIZE])
{
    for (size_t idx = 0; idx < SIZE; ++idx) {
        const PrintTestData& test = data[idx];

        PVVV("Line [" << test.d_line << "]");

        mwcu::MemOutStream out(s_allocator_p);
        mwcu::MemOutStream expected(s_allocator_p);

        typedef typename ENUM_TYPE::Enum T;

        T obj = static_cast<T>(test.d_type);

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        ENUM_TYPE::print(out, obj, 0, -1);

        ASSERT_EQ(out.str(), "");

        out.clear();
        ENUM_TYPE::print(out, obj, 0, -1);

        ASSERT_EQ(out.str(), expected.str());

        out.reset();
        out << obj;

        ASSERT_EQ(out.str(), expected.str());
    }
}

static void test3_printTest()
{
    mwctst::TestHelper::printTestName("PRINT");

    PV("Testing bmqt::GenericResult print");
    {
        PrintTestData k_DATA[] = {
            {L_, bmqt::GenericResult::e_SUCCESS, "SUCCESS"},
            {L_, bmqt::GenericResult::e_UNKNOWN, "UNKNOWN"},
            {L_, bmqt::GenericResult::e_TIMEOUT, "TIMEOUT"},
            {L_, bmqt::GenericResult::e_NOT_CONNECTED, "NOT_CONNECTED"},
            {L_, bmqt::GenericResult::e_CANCELED, "CANCELED"},
            {L_, bmqt::GenericResult::e_NOT_SUPPORTED, "NOT_SUPPORTED"},
            {L_, bmqt::GenericResult::e_REFUSED, "REFUSED"},
            {L_, bmqt::GenericResult::e_INVALID_ARGUMENT, "INVALID_ARGUMENT"},
            {L_, bmqt::GenericResult::e_NOT_READY, "NOT_READY"},
            {L_, bmqt::GenericResult::e_LAST, "NOT_READY"},
            {L_, bmqt::GenericResult::e_LAST - 1, "(* UNKNOWN *)"}};

        printEnumHelper<bmqt::GenericResult>(k_DATA);
    }

    PV("Testing bmqt::OpenQueueResult print");
    {
        PrintTestData k_DATA[] = {
            {L_, bmqt::OpenQueueResult::e_SUCCESS, "SUCCESS"},
            {L_, bmqt::OpenQueueResult::e_UNKNOWN, "UNKNOWN"},
            {L_, bmqt::OpenQueueResult::e_TIMEOUT, "TIMEOUT"},
            {L_, bmqt::OpenQueueResult::e_NOT_CONNECTED, "NOT_CONNECTED"},
            {L_, bmqt::OpenQueueResult::e_CANCELED, "CANCELED"},
            {L_, bmqt::OpenQueueResult::e_NOT_SUPPORTED, "NOT_SUPPORTED"},
            {L_, bmqt::OpenQueueResult::e_REFUSED, "REFUSED"},
            {L_,
             bmqt::OpenQueueResult::e_INVALID_ARGUMENT,
             "INVALID_ARGUMENT"},
            {L_, bmqt::OpenQueueResult::e_NOT_READY, "NOT_READY"},
            {L_, bmqt::OpenQueueResult::e_ALREADY_OPENED, "ALREADY_OPENED"},
            {L_,
             bmqt::OpenQueueResult::e_ALREADY_IN_PROGRESS,
             "ALREADY_IN_PROGRESS"},
            {L_, bmqt::OpenQueueResult::e_INVALID_URI, "INVALID_URI"},
            {L_, bmqt::OpenQueueResult::e_INVALID_FLAGS, "INVALID_FLAGS"},
            {L_,
             bmqt::OpenQueueResult::e_CORRELATIONID_NOT_UNIQUE,
             "CORRELATIONID_NOT_UNIQUE"},
            {L_, -1234, "(* UNKNOWN *)"}};

        printEnumHelper<bmqt::OpenQueueResult>(k_DATA);
    }

    PV("Testing bmqt::ConfigureQueueResult print");
    {
        PrintTestData k_DATA[] = {
            {L_, bmqt::ConfigureQueueResult::e_SUCCESS, "SUCCESS"},
            {L_, bmqt::ConfigureQueueResult::e_UNKNOWN, "UNKNOWN"},
            {L_, bmqt::ConfigureQueueResult::e_TIMEOUT, "TIMEOUT"},
            {L_, bmqt::ConfigureQueueResult::e_NOT_CONNECTED, "NOT_CONNECTED"},
            {L_, bmqt::ConfigureQueueResult::e_CANCELED, "CANCELED"},
            {L_, bmqt::ConfigureQueueResult::e_NOT_SUPPORTED, "NOT_SUPPORTED"},
            {L_, bmqt::ConfigureQueueResult::e_REFUSED, "REFUSED"},
            {L_,
             bmqt::ConfigureQueueResult::e_INVALID_ARGUMENT,
             "INVALID_ARGUMENT"},
            {L_, bmqt::ConfigureQueueResult::e_NOT_READY, "NOT_READY"},
            {L_,
             bmqt::ConfigureQueueResult::e_ALREADY_IN_PROGRESS,
             "ALREADY_IN_PROGRESS"},
            {L_, bmqt::ConfigureQueueResult::e_INVALID_QUEUE, "INVALID_QUEUE"},
            {L_, -1234, "(* UNKNOWN *)"}};

        printEnumHelper<bmqt::ConfigureQueueResult>(k_DATA);
    }

    PV("Testing bmqt::CloseQueueResult print");
    {
        PrintTestData k_DATA[] = {
            {L_, bmqt::CloseQueueResult::e_SUCCESS, "SUCCESS"},
            {L_, bmqt::CloseQueueResult::e_UNKNOWN, "UNKNOWN"},
            {L_, bmqt::CloseQueueResult::e_TIMEOUT, "TIMEOUT"},
            {L_, bmqt::CloseQueueResult::e_NOT_CONNECTED, "NOT_CONNECTED"},
            {L_, bmqt::CloseQueueResult::e_CANCELED, "CANCELED"},
            {L_, bmqt::CloseQueueResult::e_NOT_SUPPORTED, "NOT_SUPPORTED"},
            {L_, bmqt::CloseQueueResult::e_REFUSED, "REFUSED"},
            {L_,
             bmqt::CloseQueueResult::e_INVALID_ARGUMENT,
             "INVALID_ARGUMENT"},
            {L_, bmqt::CloseQueueResult::e_NOT_READY, "NOT_READY"},
            {L_, bmqt::CloseQueueResult::e_ALREADY_CLOSED, "ALREADY_CLOSED"},
            {L_,
             bmqt::CloseQueueResult::e_ALREADY_IN_PROGRESS,
             "ALREADY_IN_PROGRESS"},
            {L_, bmqt::CloseQueueResult::e_UNKNOWN_QUEUE, "UNKNOWN_QUEUE"},
            {L_, bmqt::CloseQueueResult::e_INVALID_QUEUE, "INVALID_QUEUE"},
            {L_, -1234, "(* UNKNOWN *)"}};

        printEnumHelper<bmqt::CloseQueueResult>(k_DATA);
    }

    PV("Testing bmqt::EventBuilderResult print");
    {
        PrintTestData k_DATA[] = {
            {L_, bmqt::EventBuilderResult::e_SUCCESS, "SUCCESS"},
            {L_, bmqt::EventBuilderResult::e_UNKNOWN, "UNKNOWN"},
            {L_, bmqt::EventBuilderResult::e_QUEUE_INVALID, "QUEUE_INVALID"},
            {L_, bmqt::EventBuilderResult::e_QUEUE_READONLY, "QUEUE_READONLY"},
            {L_,
             bmqt::EventBuilderResult::e_MISSING_CORRELATION_ID,
             "MISSING_CORRELATION_ID"},
            {L_, bmqt::EventBuilderResult::e_EVENT_TOO_BIG, "EVENT_TOO_BIG"},
            {L_,
             bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG,
             "PAYLOAD_TOO_BIG"},
            {L_, bmqt::EventBuilderResult::e_PAYLOAD_EMPTY, "PAYLOAD_EMPTY"},
            {L_, bmqt::EventBuilderResult::e_OPTION_TOO_BIG, "OPTION_TOO_BIG"},
#ifdef BMQ_ENABLE_MSG_GROUPID
            {L_,
             bmqt::EventBuilderResult::e_INVALID_MSG_GROUP_ID,
             "INVALID_MSG_GROUP_ID"},
#endif
            {L_, -1234, "(* UNKNOWN *)"}};

        printEnumHelper<bmqt::EventBuilderResult>(k_DATA);
    }

    PV("Testing bmqt::AckResult print");
    {
        PrintTestData k_DATA[] = {
            {L_, bmqt::AckResult::e_SUCCESS, "SUCCESS"},
            {L_, bmqt::AckResult::e_UNKNOWN, "UNKNOWN"},
            {L_, bmqt::AckResult::e_TIMEOUT, "TIMEOUT"},
            {L_, bmqt::AckResult::e_NOT_CONNECTED, "NOT_CONNECTED"},
            {L_, bmqt::AckResult::e_CANCELED, "CANCELED"},
            {L_, bmqt::AckResult::e_NOT_SUPPORTED, "NOT_SUPPORTED"},
            {L_, bmqt::AckResult::e_REFUSED, "REFUSED"},
            {L_, bmqt::AckResult::e_INVALID_ARGUMENT, "INVALID_ARGUMENT"},
            {L_, bmqt::AckResult::e_NOT_READY, "NOT_READY"},
            {L_, bmqt::AckResult::e_LIMIT_MESSAGES, "LIMIT_MESSAGES"},
            {L_, bmqt::AckResult::e_LIMIT_BYTES, "LIMIT_BYTES"},
            {L_, bmqt::AckResult::e_LIMIT_DOMAIN_MESSAGES, "LIMIT_MESSAGES"},
            {L_, bmqt::AckResult::e_LIMIT_DOMAIN_BYTES, "LIMIT_BYTES"},
            {L_,
             bmqt::AckResult::e_LIMIT_QUEUE_MESSAGES,
             "LIMIT_QUEUE_MESSAGES"},
            {L_, bmqt::AckResult::e_LIMIT_QUEUE_BYTES, "LIMIT_QUEUE_BYTES"},
            {L_, bmqt::AckResult::e_STORAGE_FAILURE, "STORAGE_FAILURE"},
            {L_, -1234, "(* UNKNOWN *)"}};

        printEnumHelper<bmqt::AckResult>(k_DATA);
    }

    PV("Testing bmqt::PostResult print");
    {
        PrintTestData k_DATA[] = {
            {L_, bmqt::PostResult::e_SUCCESS, "SUCCESS"},
            {L_, bmqt::PostResult::e_UNKNOWN, "UNKNOWN"},
            {L_, bmqt::PostResult::e_TIMEOUT, "TIMEOUT"},
            {L_, bmqt::PostResult::e_NOT_CONNECTED, "NOT_CONNECTED"},
            {L_, bmqt::PostResult::e_CANCELED, "CANCELED"},
            {L_, bmqt::PostResult::e_NOT_SUPPORTED, "NOT_SUPPORTED"},
            {L_, bmqt::PostResult::e_REFUSED, "REFUSED"},
            {L_, bmqt::PostResult::e_INVALID_ARGUMENT, "INVALID_ARGUMENT"},
            {L_, bmqt::PostResult::e_NOT_READY, "NOT_READY"},
            {L_, bmqt::PostResult::e_BW_LIMIT, "BW_LIMIT"},
            {L_, -1234, "(* UNKNOWN *)"}};

        printEnumHelper<bmqt::PostResult>(k_DATA);
    }
}

template <typename RESULT_TYPE>
static void idempotenceHelper(typename RESULT_TYPE::Enum enumVal,
                              const bslstl::StringRef&   enumValName)
{
    typename RESULT_TYPE::Enum obj;
    const char*                str;
    bool                       res;
    res = RESULT_TYPE::fromAscii(&obj, enumValName);
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, enumVal);
    str = RESULT_TYPE::toAscii(obj);
    ASSERT_EQ(str, enumValName);
}

static void test4_idempotence()
{
    mwctst::TestHelper::printTestName("IDEMPOTENCE");
#define TEST_IDEMPOTENCE(ENUM_TYPE, ENUM_VAL)                                 \
    {                                                                         \
        PV("Testing idempotence for " #ENUM_VAL " value of " #ENUM_TYPE       \
           " enum");                                                          \
        typedef bmqt::ENUM_TYPE T;                                            \
        idempotenceHelper<T>(T::e_##ENUM_VAL, #ENUM_VAL);                     \
    }

#define TEST_IDEMPOTENCE_GENERIC_STATES(ENUM_TYPE)                            \
    TEST_IDEMPOTENCE(ENUM_TYPE, SUCCESS)                                      \
    TEST_IDEMPOTENCE(ENUM_TYPE, UNKNOWN)                                      \
    TEST_IDEMPOTENCE(ENUM_TYPE, TIMEOUT)                                      \
    TEST_IDEMPOTENCE(ENUM_TYPE, NOT_CONNECTED)                                \
    TEST_IDEMPOTENCE(ENUM_TYPE, CANCELED)                                     \
    TEST_IDEMPOTENCE(ENUM_TYPE, NOT_SUPPORTED)                                \
    TEST_IDEMPOTENCE(ENUM_TYPE, REFUSED)                                      \
    TEST_IDEMPOTENCE(ENUM_TYPE, INVALID_ARGUMENT)                             \
    TEST_IDEMPOTENCE(ENUM_TYPE, NOT_READY)

    // Test GenericResult enum idempotence
    TEST_IDEMPOTENCE_GENERIC_STATES(GenericResult)

    // Test OpenQueueResult enum idempotence
    TEST_IDEMPOTENCE_GENERIC_STATES(OpenQueueResult)
    TEST_IDEMPOTENCE(OpenQueueResult, ALREADY_OPENED)
    TEST_IDEMPOTENCE(OpenQueueResult, ALREADY_IN_PROGRESS)
    TEST_IDEMPOTENCE(OpenQueueResult, INVALID_URI)
    TEST_IDEMPOTENCE(OpenQueueResult, INVALID_FLAGS)
    TEST_IDEMPOTENCE(OpenQueueResult, CORRELATIONID_NOT_UNIQUE)

    // Test PostResult enum idempotence
    TEST_IDEMPOTENCE_GENERIC_STATES(PostResult)
    TEST_IDEMPOTENCE(PostResult, BW_LIMIT)

    // Test ConfigureQueueResult enum idempotence
    TEST_IDEMPOTENCE_GENERIC_STATES(ConfigureQueueResult)
    TEST_IDEMPOTENCE(ConfigureQueueResult, ALREADY_IN_PROGRESS)
    TEST_IDEMPOTENCE(ConfigureQueueResult, INVALID_QUEUE)

    // Test EventBuilderResult enum idempotence
    TEST_IDEMPOTENCE(EventBuilderResult, SUCCESS)
    TEST_IDEMPOTENCE(EventBuilderResult, UNKNOWN)
    TEST_IDEMPOTENCE(EventBuilderResult, QUEUE_INVALID)
    TEST_IDEMPOTENCE(EventBuilderResult, QUEUE_READONLY)
    TEST_IDEMPOTENCE(EventBuilderResult, MISSING_CORRELATION_ID)
    TEST_IDEMPOTENCE(EventBuilderResult, EVENT_TOO_BIG)
    TEST_IDEMPOTENCE(EventBuilderResult, PAYLOAD_TOO_BIG)
    TEST_IDEMPOTENCE(EventBuilderResult, PAYLOAD_EMPTY)
    TEST_IDEMPOTENCE(EventBuilderResult, OPTION_TOO_BIG)
#ifdef BMQ_ENABLE_MSG_GROUPID
    TEST_IDEMPOTENCE(EventBuilderResult, INVALID_MSG_GROUP_ID)
#endif

    // Test CloseQueueResult enum idempotence
    TEST_IDEMPOTENCE_GENERIC_STATES(CloseQueueResult)
    TEST_IDEMPOTENCE(CloseQueueResult, ALREADY_CLOSED)
    TEST_IDEMPOTENCE(CloseQueueResult, ALREADY_IN_PROGRESS)
    TEST_IDEMPOTENCE(CloseQueueResult, UNKNOWN_QUEUE)
    TEST_IDEMPOTENCE(CloseQueueResult, INVALID_QUEUE)

    // Test AckResult enum idempotence
    TEST_IDEMPOTENCE_GENERIC_STATES(AckResult)
    TEST_IDEMPOTENCE(AckResult, LIMIT_MESSAGES)
    TEST_IDEMPOTENCE(AckResult, LIMIT_BYTES)
    TEST_IDEMPOTENCE(AckResult, LIMIT_QUEUE_MESSAGES)
    TEST_IDEMPOTENCE(AckResult, LIMIT_QUEUE_BYTES)
    TEST_IDEMPOTENCE(AckResult, STORAGE_FAILURE)

#undef TEST_IDEMPOTENCE_GENERIC_STATES
#undef TEST_IDEMPOTENCE
}

template <typename RESULT_TYPE>
static void invalidValueFromAsciiHelper()
{
    typename RESULT_TYPE::Enum obj;
    bool                       res;
    PVV("Invalid enum value for fromAscii method")
    res = RESULT_TYPE::fromAscii(&obj, "IMPOSSIBLE_RETURN_CODE");
    ASSERT_EQ(res, false);
}

static void test5_invalidValueFromAscii()
{
    mwctst::TestHelper::printTestName("INVALID VALUES FOR 'fromAscii'");
    invalidValueFromAsciiHelper<bmqt::GenericResult>();
    invalidValueFromAsciiHelper<bmqt::OpenQueueResult>();
    invalidValueFromAsciiHelper<bmqt::PostResult>();
    invalidValueFromAsciiHelper<bmqt::ConfigureQueueResult>();
    invalidValueFromAsciiHelper<bmqt::EventBuilderResult>();
    invalidValueFromAsciiHelper<bmqt::CloseQueueResult>();
    invalidValueFromAsciiHelper<bmqt::AckResult>();
}
// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 5: test5_invalidValueFromAscii(); break;
    case 4: test4_idempotence(); break;
    case 3: test3_printTest(); break;
    case 2: test2_schemaConsistency(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
