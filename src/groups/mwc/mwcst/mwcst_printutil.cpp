// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwcst_printutil.cpp -*-C++-*-
#include <mwcst_printutil.h>

#include <bdlb_bitutil.h>
#include <bdlb_print.h>
#include <bdlma_localsequentialallocator.h>
#include <bslmf_assert.h>
#include <bsls_alignedbuffer.h>
#include <bsls_platform.h>
#include <bsls_stopwatch.h>
#include <mwcscm_version.h>

#include <bsl_cmath.h>
#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_ios.h>

namespace BloombergLP {
namespace mwcstu {

namespace {

// MACROS
#ifdef BSLS_PLATFORM_OS_WINDOWS
#define snprintf _snprintf
#endif

// CONSTANTS
static const char* TIME_INTERVAL_NS_UNITS[] =
    {"ns", "us", "ms", "s", "m", "h", "d", "w"};
static int       TIME_INTERVAL_NS_SIZES[] = {1000, 1000, 1000, 60, 60, 24, 7};
static const int NUM_TIME_INTERVAL_NS_UNITS = sizeof(TIME_INTERVAL_NS_UNITS) /
                                              sizeof(*TIME_INTERVAL_NS_UNITS);
BSLMF_ASSERT(NUM_TIME_INTERVAL_NS_UNITS - 1 ==
             (sizeof(TIME_INTERVAL_NS_SIZES) /
              sizeof(*TIME_INTERVAL_NS_SIZES)));

static const bsls::Types::Int64 k_NANOSECS_PER_SEC = 1000000000L;  // 1e9

static const char* MEMORY_UNITS[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
static const int   NUM_MEMORY_UNITS = sizeof(MEMORY_UNITS) /
                                    sizeof(*MEMORY_UNITS);

/// From the specified `ns` return the appropriate TIME_INTERVAL_NS_UNITS
/// element to use, and in the specified `num` and `remainder` return the
/// number of units of the returned level and the remainder.
const char* timeIntervalNsHelper(bsls::Types::Int64* num,
                                 int*                remainder,
                                 bsls::Types::Int64  ns,
                                 int                 precision)
{
    bsls::Types::Int64 div   = 1;
    int                level = 0;
    for (; level < NUM_TIME_INTERVAL_NS_UNITS - 1; ++level) {
        int unitSize = TIME_INTERVAL_NS_SIZES[level];
        if (ns < div * unitSize) {
            break;
        }

        div *= unitSize;
    }

    *num       = ns / div;
    *remainder = (int)bsl::floor(((double)(ns - *num * div) / static_cast<double>(div)) *
                                 bsl::pow(10.0, precision));
    return TIME_INTERVAL_NS_UNITS[level];
}

/// From the specified `bytes` return the appropriate MEMORY_UNITS
/// element to use, and in the specified `num` and `remainder` return the
/// number of units of the returned unit and the remainder.
const char* memoryHelper(bsls::Types::Int64* num,
                         int*                remainder,
                         bsls::Types::Int64  bytes,
                         int                 precision)
{
    if (bytes == 0) {
        *num       = 0;
        *remainder = 0;
        return MEMORY_UNITS[0];
    }

    bool negative = false;
    if (bytes < 0) {
        negative = true;
        bytes    = -bytes;
    }

    static const int sizeInBits     = 64;
    static const int higestBitIndex = sizeInBits - 1;
    int              highestBitSet  = higestBitIndex -
                        bdlb::BitUtil::numLeadingUnsetBits(
                            static_cast<bsl::uint64_t>(bytes));
    int level = highestBitSet / 10;

    if (level >= NUM_MEMORY_UNITS) {
        level = NUM_MEMORY_UNITS - 1;
    }

    int shift = level * 10;

    bsls::Types::Int64 div = (((bsls::Types::Int64)1) << shift);
    *num                   = bytes >> shift;
    *remainder = (int)bsl::floor((((double)(bytes - (*num << shift)) / static_cast<double>(div)) *
                                  bsl::pow(10.0, precision)));

    if (negative) {
        *num = -*num;
    }

    return MEMORY_UNITS[level];
}

char* printValueWithSeparatorImp(char*              buf,
                                 bsls::Types::Int64 value,
                                 int                groupSize,
                                 char               separator)
{
    bool negative = value < 0;

    int processedDigits = 0;
    while (value) {
        if (processedDigits && (processedDigits % groupSize == 0)) {
            *(--buf) = separator;
        }

        bsls::Types::Int64 digit = value % 10;
        if (digit < 0) {
            digit *= -1;
        }

        *(--buf) = '0' + (char)digit;
        value /= 10;

        ++processedDigits;
    }

    if (processedDigits == 0) {
        *(--buf) = '0';
    }

    if (negative) {
        *(--buf) = '-';
    }

    return buf;
}

}  // close anonymous namespace

// ---------------
// struct PrintUtil
// ---------------

// CLASS METHODS
int PrintUtil::printedValueLength(bsls::Types::Int64 value)
{
    int len = 0;

    if (value < 0) {
        len   = 1;
        value = -value;
    }

    while (value > 1000) {
        len += 3;
        value /= 1000;
    }

    while (value) {
        len += 1;
        value /= 10;
    }

    return len > 0 ? len : 1;
}

int PrintUtil::printedValueLength(double value, int precision)
{
    return mwcstu::PrintUtil::printedValueLength((bsls::Types::Int64)value) +
           precision + 1;
}

int PrintUtil::printedValueLengthWithSeparator(bsls::Types::Int64 value,
                                               int                groupSize)
{
    int len = printedValueLength(value);
    if (value < 0) {
        len--;
    }

    bool divisible = (len % groupSize) == 0;

    len += len / groupSize;
    if (divisible) {
        len--;
    }

    if (value < 0) {
        ++len;
    }

    return len;
}

int PrintUtil::printedValueLengthWithSeparator(double value,
                                               int    precision,
                                               int    groupSize)
{
    return mwcstu::PrintUtil::printedValueLengthWithSeparator(
               (bsls::Types::Int64)value,
               groupSize) +
           precision + (precision > 0 ? 1 : 0);
}

bsl::ostream& PrintUtil::printValueWithSeparator(bsl::ostream&      stream,
                                                 bsls::Types::Int64 value,
                                                 int                groupSize,
                                                 char               separator)
{
    char  buf[64];
    char* pos = buf + 63;
    *pos      = '\0';

    return stream << printValueWithSeparatorImp(pos,
                                                value,
                                                groupSize,
                                                separator);
}

bsl::ostream& PrintUtil::printValueWithSeparator(bsl::ostream& stream,
                                                 double        value,
                                                 int           precision,
                                                 int           groupSize,
                                                 char          separator)
{
    char  buf[128];
    char* pos = buf + 127;
    *pos      = '\0';

    if (precision > 0) {
        double absValue  = bsl::abs(value);
        int    remainder = (int)bsl::floor((absValue - bsl::floor(absValue)) *
                                        bsl::pow(10.0, precision));

        snprintf(pos - precision - 2,
                 precision + 2,
                 ".%.*d",
                 precision,
                 remainder);

        pos -= precision + 2;
    }

    return stream << printValueWithSeparatorImp(pos,
                                                (bsls::Types::Int64)value,
                                                groupSize,
                                                separator);
}

bsl::ostream& PrintUtil::printStringCentered(bsl::ostream&            stream,
                                             const bslstl::StringRef& str)
{
    bsl::streamsize width = stream.width();
    if (width > (int)str.length()) {
        bsl::streamsize left = (width + str.length()) / 2;
        stream.width(left);
        stream << str;
        stream.width(width - left);
        stream << "";
    }
    else {
        stream << str;
    }
    return stream;
}

bsl::ostream& PrintUtil::printMemory(bsl::ostream&      stream,
                                     bsls::Types::Int64 bytes,
                                     int                precision)
{
    bsls::Types::Int64 value;
    int                remainder = 0;
    const char* unit = memoryHelper(&value, &remainder, bytes, precision);

    char buf[64];
    int  ret = snprintf(buf, sizeof(buf), "%lld", value);

    char* end = buf + ret;
    if (unit != MEMORY_UNITS[0] && precision > 0) {
        ret = snprintf(end, sizeof(buf) - ret, ".%.*d", precision, remainder);
        end = end + ret;
    }

    *end++ = ' ';
    for (; *unit; ++unit) {
        *end++ = *unit;
    }
    *end++ = '\0';

    return stream << buf;
}

int PrintUtil::printedMemoryLength(bsls::Types::Int64 bytes, int precision)
{
    bsls::Types::Int64 value;
    int                remainder = 0;
    const char* unit = memoryHelper(&value, &remainder, bytes, precision);

    int ret = printedValueLength(value) + static_cast<int>(bsl::strlen(unit)) + 1;
    if (unit != MEMORY_UNITS[0] && precision > 0) {
        ret += 1 + precision;
    }

    return ret;
}

bsl::ostream& PrintUtil::printTimeIntervalNs(bsl::ostream&      stream,
                                             bsls::Types::Int64 timeIntervalNs,
                                             int                precision)
{
    bsls::Types::Int64 value;
    int                remainder = 0;
    const char*        unit =
        timeIntervalNsHelper(&value, &remainder, timeIntervalNs, precision);

    char buf[64];
    int  ret = snprintf(buf, sizeof(buf), "%lld", value);

    // 'end' points to the terminating NUL
    char* end = buf + ret;

    if (unit != TIME_INTERVAL_NS_UNITS[0]) {
        ret = snprintf(end, sizeof(buf) - ret, ".%.*d", precision, remainder);
        end = end + ret;
    }

    *end++ = ' ';
    for (; *unit; ++unit) {
        *end++ = *unit;
    }
    *end++ = '\0';

    return stream << buf;
}

int PrintUtil::printedTimeIntervalNsLength(bsls::Types::Int64 timeIntervalNs,
                                           int                precision)
{
    bsls::Types::Int64 value;
    int                remainder = 0;
    const char*        unit =
        timeIntervalNsHelper(&value, &remainder, timeIntervalNs, precision);

    int ret = printedValueLength(value) + static_cast<int>(bsl::strlen(unit)) + 1;
    if (unit != TIME_INTERVAL_NS_UNITS[0]) {
        ret += 1 + precision;
    }

    return ret;
}

bsl::ostream& PrintUtil::printElapsedTime(bsl::ostream&          stream,
                                          const bsls::Stopwatch& stopwatch,
                                          int                    precision)
{
    return printTimeIntervalNs(
        stream,
        static_cast<bsls::Types::Int64>(k_NANOSECS_PER_SEC *
                                        stopwatch.elapsedTime()),
        precision);
}

bsl::ostream& PrintUtil::printOrdinal(bsl::ostream&      stream,
                                      bsls::Types::Int64 num)
{
    stream << num;

    int mod100 = (int)(num % 100);
    int mod10  = (int)(num % 10);

    if (mod100 == 11 || mod100 == 12 || mod100 == 13) {
        return stream << "th";
    }
    else if (mod10 == 1) {
        return stream << "st";
    }
    else if (mod10 == 2) {
        return stream << "nd";
    }
    else if (mod10 == 3) {
        return stream << "rd";
    }
    else {
        return stream << "th";
    }
}

bsl::ostream& PrintUtil::stringRefPrint(bsl::ostream&            stream,
                                        const bslstl::StringRef& stringRef,
                                        int                      level,
                                        int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);

    stream << stringRef;

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
