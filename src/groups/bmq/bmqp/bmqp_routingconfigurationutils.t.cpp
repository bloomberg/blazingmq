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

// bmqp_routingconfigurationutils.t.cpp                               -*-C++-*-
#include <bmqp_routingconfigurationutils.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlb_bitutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Ensure the flags member of the specified `config` are all set.
void setAllFlags(bmqp_ctrlmsg::RoutingConfiguration* config)
{
    config->flags() = 0;

    for (int flag = 0;
         flag < bmqp_ctrlmsg::RoutingConfigurationFlags::NUM_ENUMERATORS;
         ++flag) {
        config->flags() |= (1 << flag);
    }
}

/// Return the number of flags set in the flags member of the specified
/// `config`.
int numFlagsSet(const bmqp_ctrlmsg::RoutingConfiguration& config)
{
    bdlb::BitUtil::uint64_t flags = config.flags();
    return bdlb::BitUtil::numBitsSet(flags);
}

/// Return the number of flags not set in the flags member of the specified
/// `config`.
int numFlagsUnset(const bmqp_ctrlmsg::RoutingConfiguration& config)
{
    bdlb::BitUtil::uint64_t flags = config.flags();
    return bmqp_ctrlmsg::RoutingConfigurationFlags::NUM_ENUMERATORS -
           bdlb::BitUtil::numBitsSet(flags);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    typedef bmqp::RoutingConfigurationUtils obj;

    {
        PV("clear / isClear");
        bmqp_ctrlmsg::RoutingConfiguration config;

        BMQTST_ASSERT_EQ(obj::isClear(config), true);

        setAllFlags(&config);
        BMQTST_ASSERT_EQ(obj::isClear(config), false);

        obj::clear(&config);
        BMQTST_ASSERT_EQ(obj::isClear(config), true);
    }

    {
        PV("atMostOnce (isSet, set, unset)");
        bmqp_ctrlmsg::RoutingConfiguration config;

        BMQTST_ASSERT_EQ(obj::isAtMostOnce(config), false);

        obj::setAtMostOnce(&config);
        BMQTST_ASSERT_EQ(obj::isAtMostOnce(config), true);
        BMQTST_ASSERT_EQ(numFlagsSet(config), 1);

        obj::unsetAtMostOnce(&config);
        BMQTST_ASSERT_EQ(obj::isAtMostOnce(config), false);

        setAllFlags(&config);
        BMQTST_ASSERT_EQ(obj::isAtMostOnce(config), true);

        obj::unsetAtMostOnce(&config);
        BMQTST_ASSERT_EQ(numFlagsUnset(config), 1);
    }

    {
        PV("deliverAll (isSet, set, unset)");
        bmqp_ctrlmsg::RoutingConfiguration config;

        BMQTST_ASSERT_EQ(obj::isDeliverAll(config), false);

        obj::setDeliverAll(&config);
        BMQTST_ASSERT_EQ(obj::isDeliverAll(config), true);
        BMQTST_ASSERT_EQ(numFlagsSet(config), 1);

        obj::unsetDeliverAll(&config);
        BMQTST_ASSERT_EQ(obj::isDeliverAll(config), false);

        setAllFlags(&config);
        BMQTST_ASSERT_EQ(obj::isDeliverAll(config), true);

        obj::unsetDeliverAll(&config);
        BMQTST_ASSERT_EQ(numFlagsUnset(config), 1);
    }

    {
        PV("deliverConsumerPriority (isSet, set, unset)");
        bmqp_ctrlmsg::RoutingConfiguration config;

        BMQTST_ASSERT_EQ(obj::isDeliverConsumerPriority(config), false);

        obj::setDeliverConsumerPriority(&config);
        BMQTST_ASSERT_EQ(obj::isDeliverConsumerPriority(config), true);
        BMQTST_ASSERT_EQ(numFlagsSet(config), 1);

        obj::unsetDeliverConsumerPriority(&config);
        BMQTST_ASSERT_EQ(obj::isDeliverConsumerPriority(config), false);

        setAllFlags(&config);
        BMQTST_ASSERT_EQ(obj::isDeliverConsumerPriority(config), true);

        obj::unsetDeliverConsumerPriority(&config);
        BMQTST_ASSERT_EQ(numFlagsUnset(config), 1);
    }

    {
        PV("hasMultipleSubStreams (isSet, set, unset)");
        bmqp_ctrlmsg::RoutingConfiguration config;

        BMQTST_ASSERT_EQ(obj::hasMultipleSubStreams(config), false);

        obj::setHasMultipleSubStreams(&config);
        BMQTST_ASSERT_EQ(obj::hasMultipleSubStreams(config), true);
        BMQTST_ASSERT_EQ(numFlagsSet(config), 1);

        obj::unsetHasMultipleSubStreams(&config);
        BMQTST_ASSERT_EQ(obj::hasMultipleSubStreams(config), false);

        setAllFlags(&config);
        BMQTST_ASSERT_EQ(obj::hasMultipleSubStreams(config), true);

        obj::unsetHasMultipleSubStreams(&config);
        BMQTST_ASSERT_EQ(numFlagsUnset(config), 1);
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
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
