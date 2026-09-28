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

#include <mqbu_domainutil.h>

// BDE
#include <bsl_iostream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

static void test1_validDomains()
{
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my.domain-name_1"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my.domain."));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain(".my.domain"));
}

static void test2_invalidDomains()
{
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain(""));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my..domain"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.."));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("..my.domain"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my/domain"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my~domain"));
}

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_invalidDomains(); break;
    case 1: test1_validDomains(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND.\n";
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
