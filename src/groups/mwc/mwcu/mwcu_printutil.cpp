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

// mwcu_printutil.cpp                                                 -*-C++-*-
#include <mwcu_printutil.h>

#include <mwcscm_version.h>
// MWC
#include <mwcu_memoutstream.h>
#include <mwcu_outstreamformatsaver.h>

// BDE
#include <bdlb_bitutil.h>
#include <bdlb_print.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_cmath.h>
#include <bsl_cstdio.h>
#include <bsl_iomanip.h>
#include <bsl_ios.h>
#include <bsl_limits.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mwcu {

namespace {

// UTILITY FUNCTIONS

/// Workhorse for printing the specified integer `value`, split into groups
/// of specified `groupSize` digits, separated by the specified `separator`.
char* prettyNumberImp(char*              buf,
                      bsls::Types::Int64 value,
                      int                groupSize,
                      char               separator)
{
    const bool isNegative = (value < 0);

    int processedDigits = 0;
    while (value) {
        if (processedDigits && (processedDigits % groupSize == 0)) {
            *(--buf) = separator;
        }

        bsls::Types::Int64 digit = value % 10;
        if (digit < 0) {
            digit *= -1;
        }

        *(--buf) = '0' + static_cast<char>(digit);
        value /= 10;

        ++processedDigits;
    }

    if (processedDigits == 0) {
        *(--buf) = '0';
    }

    if (isNegative) {
        *(--buf) = '-';
    }

    return buf;
}

}  // close unnamed namespace

// -------------------
// namespace PrintUtil
// -------------------

namespace PrintUtil {

bsl::ostream&
prettyNumber(bsl::ostream& stream, int value, int groupSize, char separator)
{
    return prettyNumber(stream,
                        static_cast<bsls::Types::Int64>(value),
                        groupSize,
                        separator);
}

bsl::ostream& prettyNumber(bsl::ostream&      stream,
                           bsls::Types::Int64 value,
                           int                groupSize,
                           char               separator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(groupSize > 0);

    char  buf[64];
    char* pos = buf + 63;
    *pos      = '\0';

    return stream << prettyNumberImp(pos, value, groupSize, separator);
}

bsl::ostream& prettyNumber(bsl::ostream& stream,
                           double        value,
                           int           precision,
                           int           groupSize,
                           char          separator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(groupSize > 0);

    char  buf[128];
    char* pos = buf + 127;
    *pos      = '\0';

    if (precision > 0) {
        const double absValue  = bsl::abs(value);
        const int    remainder = static_cast<int>(
            bsl::floor((absValue - bsl::floor(absValue)) *
                       bsl::pow(static_cast<double>(10.),
                                static_cast<double>(precision))));

        snprintf(pos - precision - 2,
                 precision + 2,
                 ".%.*d",
                 precision,
                 remainder);

        pos -= precision + 2;
    }

    return stream << prettyNumberImp(pos,
                                     static_cast<bsls::Types::Int64>(value),
                                     groupSize,
                                     separator);
}

bsl::ostream&
prettyBytes(bsl::ostream& stream, bsls::Types::Int64 bytes, int precision)
{
    static const char* k_UNITS[]     = {" B", "KB", "MB", "GB", "TB", "PB"};
    static const int   k_UNITS_COUNT = sizeof(k_UNITS) / sizeof(*k_UNITS);

    bdlma::LocalSequentialAllocator<1024> localAllocator(
        bslma::Default::allocator());
    mwcu::MemOutStream temp(&localAllocator);

    // Handle negative 'bytes'
    if (bytes < 0) {
        if (bytes == bsl::numeric_limits<bsls::Types::Int64>::min()) {
            // Special case: '-intMin > intMax', therefore sign inversion
            // ('*= -1') will overflow.  Since we are going to print that
            // number in the 'PB' unit scale, there is no problem to be off by
            // one byte, and therefore just reverse the value to IntMax.
            bytes = bsl::numeric_limits<bsls::Types::Int64>::max();
        }
        else {
            bytes *= -1;
        }

        // Print the '-' sign
        temp << '-';
    }

    int unit = (bdlb::BitUtil::sizeInBits(bytes) -
                bdlb::BitUtil::numLeadingUnsetBits(
                    static_cast<uint64_t>(bytes)) -
                1) /
               10;

    if (unit >= k_UNITS_COUNT) {
        unit = k_UNITS_COUNT - 1;
    }

    if (precision == 0 || unit == 0) {
        // When no decimal part is required, we round up the value and print it
        bsls::Types::Int64 quot = lround(
            bytes / bsl::pow(1024., static_cast<double>(unit)));
        if (quot == 1024 && unit != k_UNITS_COUNT - 1) {
            // This is a special case when the round up leads to the next unit
            quot = 1;
            ++unit;
        }
        temp << quot;
    }
    else {
        int shift   = unit * 10;
        int scaling = 1;

        for (int mult = precision; mult; --mult) {
            scaling *= 10;
        }

        if (unit == k_UNITS_COUNT - 1) {
            shift -= 10;
            bytes >>= 10;
        }

        bsls::Types::Int64 scaledValue = (bytes * scaling * 10) /
                                             (1LL << shift) +
                                         5;

        temp << (scaledValue / scaling / 10) << "." << bsl::setw(precision)
             << bsl::setfill('0') << (scaledValue / 10 % scaling);
    }

    // Print the unit scale
    temp << ' ' << k_UNITS[unit];

    {
        OutStreamFormatSaver streamFmtSaver(stream);
        stream << temp.str();
    }

    stream << bsl::setw(0);
    // By the standard, setw should be reset after writing to the stream,
    // however since we use the StreamFormatSaver it will get restored, so
    // explicitly reset it.

    return stream;
}

bsl::ostream& prettyTimeInterval(bsl::ostream&      stream,
                                 bsls::Types::Int64 timeNs,
                                 int                precision)
{
    static const char* k_UNITS[] = {"ns", "us", "ms", "s", "m", "h", "d", "w"};
    static const int   k_SIZES[] = {1000, 1000, 1000, 60, 60, 24, 7};
    static const int   k_SIZES_COUNT = sizeof(k_SIZES) / sizeof(*k_SIZES);

    bdlma::LocalSequentialAllocator<1024> localAllocator(
        bslma::Default::allocator());
    mwcu::MemOutStream temp(&localAllocator);

    // Handle negative 'timeNs'
    if (timeNs < 0) {
        if (timeNs == bsl::numeric_limits<bsls::Types::Int64>::min()) {
            // Special case: '-intMin > intMax', therefore sign inversion
            // ('*= -1') will overflow.  Since we are going to print that
            // number in the 'weeks' unit scale, there is no problem to be off
            // by one nanosecond, and therefore just reverse the value to
            // IntMax.
            timeNs = bsl::numeric_limits<bsls::Types::Int64>::max();
        }
        else {
            timeNs *= -1;
        }

        // Print the '-' sign
        temp << '-';
    }

    // Find the right unit scale
    int                unitIdx = 0;
    bsls::Types::Int64 div     = 1;
    for (; unitIdx < k_SIZES_COUNT; ++unitIdx) {
        const int unitSize = k_SIZES[unitIdx];
        if (timeNs < div * unitSize) {
            break;  // BREAK
        }
        div *= unitSize;
    }

    // Compute and print the quotient and remainder
    if (precision == 0 || unitIdx == 0) {
        // When no decimal part is required, we round up the value and print it
        const bsls::Types::Int64 quot = lround(timeNs /
                                               static_cast<double>(div));
        temp << quot;
    }
    else {
        // Decimal part needed, compute the remainder and print the required
        // precision digits
        const bsls::Types::Int64 quot      = timeNs / div;
        const long               remainder = lround(
            (static_cast<double>(timeNs - quot * div) / div) *
            bsl::pow(10., static_cast<double>(precision)));
        temp << quot << "." << bsl::setw(precision) << bsl::setfill('0')
             << remainder;
    }

    // Print the unit scale
    temp << ' ' << k_UNITS[unitIdx];

    {
        OutStreamFormatSaver streamFmtSaver(stream);
        stream << temp.str();
    }

    stream << bsl::setw(0);
    // By the standard, setw should be reset after writing to the stream,
    // however since we use the StreamFormatSaver it will get restored, so
    // explicitly reset it.

    return stream;
}

}  // close PrintUtil namespace

}  // close package namespace
}  // close enterprise namespace
