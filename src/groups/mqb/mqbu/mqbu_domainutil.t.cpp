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

// BMQ
#include <bmqt_uri.h>

// BDE
#include <bsl_cstddef.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_string_view.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace {

/// Return `true` if `bmqt::UriParser` accepts the specified `domain` as the
/// authority of a URI, and `false` otherwise.
bool isValidUriAuthority(const bsl::string_view& domain)
{
    bsl::string uriString("bmq://", bmqtst::TestHelperUtil::allocator());
    uriString.append(domain.data(), domain.length());
    uriString.append("/q");

    bmqt::Uri   uri(bmqtst::TestHelperUtil::allocator());
    bsl::string error(bmqtst::TestHelperUtil::allocator());
    if (bmqt::UriParser::parse(&uri, &error, uriString) != 0) {
        return false;
    }

    return bsl::string_view(uri.qualifiedDomain().data(),
                            uri.qualifiedDomain().length()) == domain;
}

}  // close unnamed namespace

static void test1_validDomains()
{
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my.domain-name_1"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my.domain."));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain(".my.domain"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("a"));

    // Tiered domains, as returned by 'bmqt::Uri::qualifiedDomain()'
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("a.~b"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my.domain.~tier"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my.domain.~tier-1"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my.domain.~tier-1-2-3"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my_domain.~TST"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain(".my.domain.~tier"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my.domain.~1"));
    BMQTST_ASSERT(mqbu::DomainUtil::isValidDomain("my.domain.~-"));
}

static void test2_invalidDomains()
{
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain(""));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my..domain"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.."));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("..my.domain"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my/domain"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my~domain"));

    // Malformed tiers
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain(".~tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("~tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain~tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain..~tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~~tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~tier~tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~tier.~tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~tier.x"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~tier_x"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~tier/x"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~.tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~../x"));

    // Invalid name before a valid tier
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my~domain.~tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my/domain.~tier"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("..~tier"));

    // Embedded NUL; literals would be truncated, so pass explicit lengths
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain(
        bsl::string_view("my.domain.~tier\0/x", 18)));
    BMQTST_ASSERT(
        !mqbu::DomainUtil::isValidDomain(bsl::string_view("my\0.domain", 10)));
    BMQTST_ASSERT(
        !mqbu::DomainUtil::isValidDomain(bsl::string_view("my.domain\0", 10)));

    // Non-ASCII
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.dom\xC3\xA9in"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~ti\xC3\xA9r"));

    // Whitespace
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my domain"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain(" my.domain"));
    BMQTST_ASSERT(!mqbu::DomainUtil::isValidDomain("my.domain.~tier "));
}

static void test3_matchesUriGrammar()
{
    // 'isValidDomain' must accept exactly the authorities (domain plus
    // optional tier) that 'bmqt::Uri' accepts.

    const bsl::string_view k_DOMAINS[] = {
        "",
        "a",
        ".",
        "my.domain-name_1",
        "my.domain.",
        ".my.domain",
        "my..domain",
        "my.domain..",
        "..my.domain",
        "my/domain",
        "my~domain",
        "my.domain?x",
        "my.domain.~tier",
        "my.domain.~tier-1",
        "my.domain.~tier-1-2-3",
        "my_domain.~TST",
        ".my.domain.~tier",
        "a.~b",
        "my.domain.~1",
        "my.domain.~-",
        ".~tier",
        "~tier",
        "..~tier",
        "my.domain.~",
        "my.domain~tier",
        "my.domain..~tier",
        "my.domain.~~tier",
        "my.domain.~tier~tier",
        "my.domain.~tier.~tier",
        "my.domain.~tier.x",
        "my.domain.~tier_x",
        "my.domain.~tier/x",
        "my.domain.~.tier",
        "my.domain.~../x",
        "my~domain.~tier",
        "my/domain.~tier",
        bsl::string_view("my.domain.~tier\0/x", 18),
        bsl::string_view("my\0.domain", 10),
        bsl::string_view("my.domain\0", 10),
        "my.dom\xC3\xA9in",
        "my.domain.~ti\xC3\xA9r",
        " my.domain",
        "my.domain.~tier ",
    };

    const bsl::size_t k_NUM_DOMAINS = sizeof(k_DOMAINS) / sizeof(*k_DOMAINS);

    for (bsl::size_t i = 0; i < k_NUM_DOMAINS; ++i) {
        const bsl::string_view& domain = k_DOMAINS[i];

        BMQTST_ASSERT_EQ_D("'" << domain << "'",
                           mqbu::DomainUtil::isValidDomain(domain),
                           isValidUriAuthority(domain));
    }
}

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_matchesUriGrammar(); break;
    case 2: test2_invalidDomains(); break;
    case 1: test1_validDomains(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND.\n";
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
