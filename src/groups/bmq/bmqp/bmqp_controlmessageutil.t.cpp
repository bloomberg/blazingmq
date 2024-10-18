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

// bmqp_controlmessageutil.t.cpp                                      -*-C++-*-
#include <bmqp_controlmessageutil.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_validate()
// ------------------------------------------------------------------------
// VALIDATE
//
// Concerns:
//   Proper behavior of the 'validate' method.
//
// Plan:
//   Verify that the 'validate' method returns the correct return code for
//   every applicable scenario.
//     1. Invalid id (rc != 0)
//     2. Undefined selection (rc != 0)
//     3. Valid control message (rc == 0)
//
// Testing:
//   validate
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("VALIDATE");

    struct Test {
        int d_line;
        int d_id;
        int d_choiceSelection;
        int d_expectedRc;
    } k_DATA[] = {
        {L_, -1, bmqp_ctrlmsg::ControlMessageChoice::SELECTION_ID_STATUS, -1},
        {L_,
         1,
         bmqp_ctrlmsg::ControlMessageChoice::SELECTION_ID_UNDEFINED,
         -2},
        {L_, 1, bmqp_ctrlmsg::ControlMessageChoice::SELECTION_ID_STATUS, 0},
        {L_,
         2,
         bmqp_ctrlmsg::ControlMessageChoice::SELECTION_ID_OPEN_QUEUE,
         0}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing:"
                        << " bmqp::ControlMessageUtil::validate({id: "
                        << test.d_id
                        << ", choiceSelection: " << test.d_choiceSelection
                        << "}) == " << test.d_expectedRc);

        bmqp_ctrlmsg::ControlMessage controlMessage(s_allocator_p);
        controlMessage.rId().makeValue(test.d_id);
        controlMessage.choice().makeSelection(test.d_choiceSelection);

        ASSERT_EQ_D(test.d_line,
                    bmqp::ControlMessageUtil::validate(controlMessage),
                    test.d_expectedRc);
    }

    // Edge case #1: 'id.isNull()'
    bmqp_ctrlmsg::ControlMessage controlMessage(s_allocator_p);
    ASSERT_EQ(bmqp::ControlMessageUtil::validate(controlMessage), -1);
    // rc_INVALID_ID

    // Edge case #2: 'id.isNull()' in a ClusterMessage
    controlMessage.choice().makeClusterMessage();
    ASSERT_EQ(bmqp::ControlMessageUtil::validate(controlMessage), 0);
    // rc_SUCCESS
}

static void test2_makeStatusControlMessage()
// ------------------------------------------------------------------------
// MAKE STATUS CONTROL MESSAGE
//
// Concerns:
//   Proper behavior of the 'makeStatusControlMessage' method.
//
// Plan:
//   Verify that the 'makeStatusControlMessage' method makes the proper
//   status message.
//
// Testing:
//   makeStatusControlMessage
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MAKE STATUS CONTROL MESSAGE");

    struct Test {
        int                                 d_line;
        bmqp_ctrlmsg::StatusCategory::Value d_category;
        int                                 d_code;
        const char*                         d_message;
    } k_DATA[] = {{L_, bmqp_ctrlmsg::StatusCategory::E_SUCCESS, 0, "Success!"},
                  {L_, bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED, -1, ""}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line
            << ": Testing:"
            << " bmqp::ControlMessageUtil::makeStatusControlMessage("
            << "{category: " << test.d_category << ", code: " << test.d_code
            << ", message: " << test.d_message << "});");

        bmqp_ctrlmsg::ControlMessage expected(s_allocator_p);
        expected.choice().makeStatus();
        expected.choice().status().category() = test.d_category;
        expected.choice().status().code()     = test.d_code;
        expected.choice().status().message()  = test.d_message;

        bmqp_ctrlmsg::ControlMessage obj(s_allocator_p);
        bmqp::ControlMessageUtil::makeStatus(&obj,
                                             test.d_category,
                                             test.d_code,
                                             test.d_message);

        ASSERT_EQ_D(test.d_line, obj, expected);
    }
}

// ============================================================================
//                                MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_makeStatusControlMessage(); break;
    case 1: test1_validate(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
