// Copyright 2026 Bloomberg Finance L.P.
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

#include <bslma_testallocatormonitor.h>
#include <mqbauthz_authorizationcontroller.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbplug_pluginmanager.h>

// BMQ
#include <bmqtst_testhelper.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bslma_managedptr.h>
#include <bslma_testallocatormonitor.h>

// gtest
#include <gtest/gtest.h>

using namespace BloombergLP;

TEST(AuthorizationController, breathingTest)
{
    bslma::TestAllocatorMonitor tam(
        &bmqtst::TestHelperUtil::defaultAllocator());

    bsl::allocator<> alloc = bmqtst::TestHelperUtil::allocator();
    bslma::ManagedPtr<mqbauthz::AuthorizationController> controller_mp;
    mqbplug::PluginManager   pluginManager(alloc.mechanism());
    mqbcfg::AuthorizerConfig authzConfig;
    bmqu::MemOutStream       errDesc;

    int rc = mqbauthz::AuthorizationController::allocateManaged(&controller_mp,
                                                                errDesc,
                                                                authzConfig,
                                                                pluginManager,
                                                                alloc);

    EXPECT_EQ(0, rc);
    EXPECT_TRUE(tam.isInUseSame());
}

// ========================================================================
//                                  MAIN
// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    ::testing::InitGoogleTest(&argc, argv);

    bmqtst::TestHelperUtil::testStatus() = RUN_ALL_TESTS();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
