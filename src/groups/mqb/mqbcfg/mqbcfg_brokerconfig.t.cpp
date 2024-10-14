// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbcfg_brokerconfig.t.cpp                                          -*-C++-*-
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>

#include <bmqtst_testhelper.h>

// BDE
#include <bsl_cstdlib.h>
#include <bsl_iostream.h>
#include <bsls_asserttest.h>

using namespace BloombergLP;

// ============================================================================
//                                  UTILITIES
// ----------------------------------------------------------------------------

TEST(breathing)
{
    ASSERT_SAFE_FAIL(mqbcfg::BrokerConfig::get());

    mqbcfg::AppConfig config;
    mqbcfg::BrokerConfig::set(config);
    ASSERT_EQ(&mqbcfg::BrokerConfig::get(), &config);

    ASSERT_SAFE_FAIL(mqbcfg::BrokerConfig::set(config));
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
