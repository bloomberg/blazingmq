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

// mqbi_queueengine.t.cpp                                             -*-C++-*-
#include <mqbi_queueengine.h>

// MQB
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>

// BDE
#include <bsl_string.h>

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
// Plan:
//  1) Consistency check
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bsl::string emptyString("", bmqtst::TestHelperUtil::allocator());

    // App id
    ASSERT(emptyString == bmqp::ProtocolUtil::k_NULL_APP_ID);
    ASSERT(emptyString !=
           bmqp_ctrlmsg::SubQueueIdInfo::DEFAULT_INITIALIZER_APP_ID);

    ASSERT_NE(bsl::string(bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
                          bmqtst::TestHelperUtil::allocator()),
              bsl::string(bmqp::ProtocolUtil::k_NULL_APP_ID,
                          bmqtst::TestHelperUtil::allocator()));
    ASSERT_EQ(
        bsl::string(bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
                    bmqtst::TestHelperUtil::allocator()),
        bsl::string(bmqp_ctrlmsg::SubQueueIdInfo ::DEFAULT_INITIALIZER_APP_ID,
                    bmqtst::TestHelperUtil::allocator()));

    // App key
    ASSERT_EQ(mqbi::QueueEngine::k_DEFAULT_APP_KEY,
              mqbu::StorageKey(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID));
    ASSERT_NE(mqbu::StorageKey::k_NULL_KEY,
              mqbi::QueueEngine::k_DEFAULT_APP_KEY);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
