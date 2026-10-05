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

namespace BloombergLP {
namespace mqbu {

namespace {

bool isAlnumOrDash(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') || ch == '-';
}

bool isValidName(bsl::string_view name)
{
    if (name.empty()) {
        return false;
    }

    for (size_t pos = 0; pos < name.length(); ++pos) {
        const char ch = name[pos];
        if (isAlnumOrDash(ch) || ch == '_') {
            continue;
        }
        if (ch == '.') {
            if (pos > 0 && name[pos - 1] == '.') {
                return false;
            }
            continue;
        }
        return false;
    }

    return true;
}

bool isValidTier(bsl::string_view tier)
{
    if (tier.empty()) {
        return false;
    }

    for (size_t pos = 0; pos < tier.length(); ++pos) {
        if (!isAlnumOrDash(tier[pos])) {
            return false;
        }
    }

    return true;
}

}  // close unnamed namespace

bool DomainUtil::isValidDomain(bsl::string_view domain)
{
    const size_t tierPos = domain.find(".~");
    if (tierPos == bsl::string_view::npos) {
        return isValidName(domain);
    }

    const bsl::string_view name = domain.substr(0, tierPos);
    return isValidName(name) && name[name.length() - 1] != '.' &&
           isValidTier(domain.substr(tierPos + 2));
}

}  // close package namespace
}  // close enterprise namespace
