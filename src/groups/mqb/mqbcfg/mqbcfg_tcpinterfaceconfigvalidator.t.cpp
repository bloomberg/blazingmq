// Copyright 2024 Bloomberg Finance L.P.
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

// mqbcfg_tcpinterfaceconfigvalidator.t.cpp                           -*-C++-*-
#include <mqbcfg_tcpinterfaceconfigvalidator.h>

// MQB
#include <mqbcfg_messages.h>

// BMQ
#include <bmqtst_testhelper.h>

// BDE
#include <bsl_cstdlib.h>
#include <bsl_iostream.h>
#include <bsls_asserttest.h>

using namespace BloombergLP;

struct TcpInterfaceConfigValidatorTest : bmqtst::Test {};

BMQTST_TEST_F(TcpInterfaceConfigValidatorTest, breathingTest)
{
    BMQTST_ASSERT_EQ(mqbcfg::TcpInterfaceConfigValidator::k_OK, 0);
}

BMQTST_TEST_F(TcpInterfaceConfigValidatorTest, emptyConfigIsValid)
{
    mqbcfg::TcpInterfaceConfigValidator validator;
    mqbcfg::TcpInterfaceConfig          config;

    BMQTST_ASSERT_EQ(mqbcfg::TcpInterfaceConfigValidator::k_OK,
                     validator(config));
}

BMQTST_TEST_F(TcpInterfaceConfigValidatorTest, nonUniqueNamesAreInvalid)
{
    mqbcfg::TcpInterfaceConfigValidator validator;
    mqbcfg::TcpInterfaceConfig          config;

    {
        mqbcfg::TcpInterfaceListener& listener =
            config.listeners().emplace_back();
        listener.name() = "Test";
    }

    {
        mqbcfg::TcpInterfaceListener& listener =
            config.listeners().emplace_back();
        listener.name() = "Test";
    }

    BMQTST_ASSERT_EQ(mqbcfg::TcpInterfaceConfigValidator::k_DUPLICATE_NAME,
                     validator(config));
}

BMQTST_TEST_F(TcpInterfaceConfigValidatorTest, nonUniquePortsAreInvalid)
{
    mqbcfg::TcpInterfaceConfigValidator validator;
    mqbcfg::TcpInterfaceConfig          config;

    {
        mqbcfg::TcpInterfaceListener& listener =
            config.listeners().emplace_back();
        listener.name() = "listener0";
        listener.port() = 8000;
    }

    {
        mqbcfg::TcpInterfaceListener& listener =
            config.listeners().emplace_back();
        listener.name() = "listener1";
        listener.port() = 8000;
    }

    BMQTST_ASSERT_EQ(mqbcfg::TcpInterfaceConfigValidator::k_DUPLICATE_PORT,
                     validator(config));
}

BMQTST_TEST_F(TcpInterfaceConfigValidatorTest, outOfRangePortsAreInvalid)
{
    mqbcfg::TcpInterfaceConfigValidator validator;

    {
        mqbcfg::TcpInterfaceConfig    config;
        mqbcfg::TcpInterfaceListener& listener =
            config.listeners().emplace_back();
        listener.port() = 0x10000;
        BMQTST_ASSERT_EQ(mqbcfg::TcpInterfaceConfigValidator::k_PORT_RANGE,
                         validator(config));
    }

    {
        mqbcfg::TcpInterfaceConfig    config;
        mqbcfg::TcpInterfaceListener& listener =
            config.listeners().emplace_back();
        listener.port() = -1;
        BMQTST_ASSERT_EQ(mqbcfg::TcpInterfaceConfigValidator::k_PORT_RANGE,
                         validator(config));
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqtst::runTest(_testCase);

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
