// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqu_temputil.cpp                                                  -*-C++-*-
#include <bmqu_temputil.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_cstdlib.h>
#include <bsl_cstring.h>
#include <bsls_assert.h>
#include <bsls_platform.h>

#if defined(BSLS_PLATFORM_OS_WINDOWS)
#include <windows.h>
#endif

namespace BloombergLP {
namespace bmqu {

// ---------------
// struct TempUtil
// ---------------

bsl::string TempUtil::tempDir()
{
#if defined(BSLS_PLATFORM_OS_UNIX)

    const char* tmp;
    tmp = std::getenv("TMPDIR");
    if (tmp == 0) {
        tmp = std::getenv("TMP");
        if (tmp == 0) {
            tmp = std::getenv("TEMP");
            if (tmp == 0) {
                tmp = "/tmp/";
            }
        }
    }

    char buffer[PATH_MAX + 1];

    std::size_t length = std::strlen(tmp);
    BSLS_ASSERT_OPT(sizeof buffer >= length + 1);

    std::memcpy(buffer, tmp, length);
    buffer[length] = 0;

    bsl::string result(buffer);

    if (result.empty()) {
        result = ".";
    }

    if (result[result.size() - 1] != '/') {
        result.push_back('/');
    }

    return result;

#elif defined(BSLS_PLATFORM_OS_WINDOWS)

    char buffer[MAX_PATH + 1];

    const char* tmp;

    tmp = std::getenv("TMPDIR");
    if (tmp != 0) {
        std::size_t length = std::strlen(tmp);
        BSLS_ASSERT_OPT(sizeof buffer >= length + 1);

        std::memcpy(buffer, tmp, length);
        buffer[length] = 0;
    }
    else {
        DWORD rc = GetTempPathA(static_cast<DWORD>(sizeof buffer), buffer);
        BSLS_ASSERT_OPT(0 != rc);
    }

    bsl::string result(buffer);

    if (result.empty()) {
        result = ".";
    }

    if (result[result.size() - 1] != '/' &&
        result[result.size() - 1] != '\\') {
        result.push_back('\\');
    }

    return result;

#else
#error The platform is not supported
#endif
}

bsl::string TempUtil::tempDirDefault()
{
#if defined(BSLS_PLATFORM_OS_UNIX)

    return bsl::string("/tmp/");

#elif defined(BSLS_PLATFORM_OS_WINDOWS)

    char  buffer[MAX_PATH + 1];
    DWORD rc = GetWindowsDirectoryA(buffer, static_cast<UINT>(sizeof buffer));
    BSLS_ASSERT_OPT(0 != rc);

    bsl::string result(buffer);

    if (result.empty()) {
        result = ".";
    }

    if (result[result.size() - 1] != '/' &&
        result[result.size() - 1] != '\\') {
        result.push_back('\\');
    }

    return result;

#else
#error The platform is not supported
#endif
}

}  // close package namespace
}  // close enterprise namespace
