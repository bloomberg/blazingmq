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

#include <bmqst_printutil.h>

#include <bdlb_bitutil.h>
#include <bdlb_print.h>
#include <bdlma_localsequentialallocator.h>
#include <bmqscm_version.h>
#include <bslmf_assert.h>
#include <bsls_alignedbuffer.h>
#include <bsls_platform.h>
#include <bsls_stopwatch.h>

#include <bsl_cmath.h>
#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_ios.h>

namespace BloombergLP {
namespace bmqst {

namespace {

// MACROS
#ifdef BSLS_PLATFORM_OS_WINDOWS
#define snprintf _snprintf
#endif

// CONSTANTS
// NOLINTBEGIN(*-avoid-c-arrays,cppcoreguidelines-avoid-non-const-global-variables)
static const char* TIME_INTERVAL_NS_UNITS[] =
    {"ns", "us", "ms", "s", "m", "h", "d", "w"};
// NOLINTEND(*-avoid-c-arrays,cppcoreguidelines-avoid-non-const-global-variables)
// NOLINTNEXTLINE(*-avoid-c-arrays,*-magic-numbers,cppcoreguidelines-avoid-non-const-global-variables)
static int TIME_INTERVAL_NS_SIZES[] = {1000, 1000, 1000, 60, 60, 24, 7};
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
static const int NUM_TIME_INTERVAL_NS_UNITS = sizeof(TIME_INTERVAL_NS_UNITS) /
                                              sizeof(*TIME_INTERVAL_NS_UNITS);
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
BSLMF_ASSERT(NUM_TIME_INTERVAL_NS_UNITS - 1 ==
             (sizeof(TIME_INTERVAL_NS_SIZES) /
              sizeof(*TIME_INTERVAL_NS_SIZES)));
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

static const bsls::Types::Int64 k_NANOSECS_PER_SEC = 1000000000L;  // 1e9

// NOLINTNEXTLINE(*-avoid-c-arrays,cppcoreguidelines-avoid-non-const-global-variables)
static const char* MEMORY_UNITS[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
static const int NUM_MEMORY_UNITS = sizeof(MEMORY_UNITS) /
                                    sizeof(*MEMORY_UNITS);
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

/// From the specified `ns` return the appropriate TIME_INTERVAL_NS_UNITS
/// element to use, and in the specified `num` and `remainder` return the
/// number of units of the returned level and the remainder.
const char* timeIntervalNsHelper(bsls::Types::Int64* num,
                                 int*                remainder,
                                 bsls::Types::Int64  ns,
                                 int                 precision)
// NOLINTBEGIN(*-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)
{
    bsls::Types::Int64 div   = 1;
    int                level = 0;
    for (; level < NUM_TIME_INTERVAL_NS_UNITS - 1; ++level) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        int unitSize = TIME_INTERVAL_NS_SIZES[level];
        if (ns < div * unitSize) {
            break;
        }

        div *= unitSize;
    }

    *num       = ns / div;
    *remainder = static_cast<int>(bsl::floor(
        (static_cast<double>(ns - *num * div) / static_cast<double>(div)) *
        bsl::pow(10.0, precision)));
    return TIME_INTERVAL_NS_UNITS[level];
}
// NOLINTEND(*-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)

/// From the specified `bytes` return the appropriate MEMORY_UNITS
/// element to use, and in the specified `num` and `remainder` return the
/// number of units of the returned unit and the remainder.
const char* memoryHelper(bsls::Types::Int64* num,
                         int*                remainder,
                         bsls::Types::Int64  bytes,
                         int                 precision)
// NOLINTBEGIN(*-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)
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
    // NOLINTNEXTLINE(*-magic-numbers)
    bsls::Types::Int64 level = highestBitSet / 10;

    if (level >= NUM_MEMORY_UNITS) {
        level = NUM_MEMORY_UNITS - 1;
    }

    // NOLINTNEXTLINE(*-magic-numbers)
    bsls::Types::Int64 shift = level * 10;

    bsls::Types::Int64 div = 1LL << shift;
    *num                   = bytes >> shift;
    *remainder             = static_cast<int>(
        bsl::floor(((static_cast<double>(bytes - (*num << shift)) /
                     static_cast<double>(div)) *
                    bsl::pow(10.0, precision))));

    if (negative) {
        *num = -*num;
    }

    return MEMORY_UNITS[level];
}
// NOLINTEND(*-magic-numbers,cppcoreguidelines-pro-bounds-constant-array-index)

char* printValueWithSeparatorImp(char*              buf,
                                 bsls::Types::Int64 value,
                                 int                groupSize,
                                 char               separator)
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
{
    bool negative = value < 0;

    int processedDigits = 0;
    // NOLINTBEGIN(*-magic-numbers,*-narrowing-conversions,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    while (value) {
        if (processedDigits && (processedDigits % groupSize == 0)) {
            *(--buf) = separator;
        }

        // NOLINTNEXTLINE(*-magic-numbers)
        bsls::Types::Int64 digit = value % 10;
        if (digit < 0) {
            digit *= -1;
        }

        // 0 <= digit < 10, so this static cast is safe.
        *(--buf) = '0' + static_cast<char>(digit);
        value /= 10;

        ++processedDigits;
    }
    // NOLINTEND(*-magic-numbers,*-narrowing-conversions,cppcoreguidelines-pro-bounds-pointer-arithmetic)

    if (processedDigits == 0) {
        *(--buf) = '0';
    }

    if (negative) {
        *(--buf) = '-';
    }

    return buf;
}
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

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

    // NOLINTBEGIN(*-magic-numbers)
    while (value > 1000) {
        len += 3;
        value /= 1000;
    }
    // NOLINTEND(*-magic-numbers)

    // NOLINTBEGIN(*-magic-numbers)
    while (value) {
        len += 1;
        value /= 10;
    }
    // NOLINTEND(*-magic-numbers)

    return len > 0 ? len : 1;
}

int PrintUtil::printedValueLength(double value, int precision)
{
    // static cast to find the length of the non-decimal portion.
    return bmqst::PrintUtil::printedValueLength(
               static_cast<bsls::Types::Int64>(value)) +
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
    // static cast to find the length of the non-decimal portion.
    return bmqst::PrintUtil::printedValueLengthWithSeparator(
               static_cast<bsls::Types::Int64>(value),
               groupSize) +
           precision + (precision > 0 ? 1 : 0);
}

bsl::ostream& PrintUtil::printValueWithSeparator(bsl::ostream&      stream,
                                                 bsls::Types::Int64 value,
                                                 int                groupSize,
                                                 char               separator)
// NOLINTBEGIN(*-avoid-c-arrays,*-magic-numbers)
{
    char buf[64];
    // NOLINTNEXTLINE(*-magic-numbers,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    char* pos = buf + 63;
    *pos      = '\0';

    return stream << printValueWithSeparatorImp(pos,
                                                value,
                                                groupSize,
                                                separator);
}
// NOLINTEND(*-avoid-c-arrays,*-magic-numbers)

bsl::ostream& PrintUtil::printValueWithSeparator(bsl::ostream& stream,
                                                 double        value,
                                                 int           precision,
                                                 int           groupSize,
                                                 char          separator)
// NOLINTBEGIN(*-avoid-c-arrays,*-magic-numbers,cert-err33-c,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-vararg)
{
    char buf[128];
    // NOLINTNEXTLINE(*-magic-numbers,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    char* pos = buf + 127;
    *pos      = '\0';

    if (precision > 0) {
        double absValue = bsl::abs(value);
        // NOLINTBEGIN(*-magic-numbers)
        int remainder = static_cast<int>(bsl::floor(
            (absValue - bsl::floor(absValue)) * bsl::pow(10.0, precision)));
        // NOLINTEND(*-magic-numbers)

        snprintf(pos - precision - 2,
                 precision + 2,
                 ".%.*d",
                 precision,
                 remainder);

        pos -= precision + 2;
    }

    // Static cast to only print the non-decimal portion of value.
    return stream << printValueWithSeparatorImp(
               pos,
               static_cast<bsls::Types::Int64>(value),
               groupSize,
               separator);
}
// NOLINTEND(*-avoid-c-arrays,*-magic-numbers,cert-err33-c,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-vararg)

bsl::ostream& PrintUtil::printStringCentered(bsl::ostream&            stream,
                                             const bslstl::StringRef& str)
{
    bsl::streamsize width = stream.width();
    if (width > static_cast<int>(str.length())) {
        // NOLINTNEXTLINE(*-narrowing-conversions)
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
// NOLINTBEGIN(*-avoid-c-arrays,*-magic-numbers,cppcoreguidelines-init-variables,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-vararg)
{
    bsls::Types::Int64 value;
    int                remainder = 0;
    const char* unit = memoryHelper(&value, &remainder, bytes, precision);

    char buf[64];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-type-vararg)
    int ret = snprintf(buf, sizeof(buf), "%lld", value);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    char* end = buf + ret;
    if (unit != MEMORY_UNITS[0] && precision > 0) {
        ret = snprintf(end, sizeof(buf) - ret, ".%.*d", precision, remainder);
        end = end + ret;
    }

    *end++ = ' ';
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (; *unit; ++unit) {
        *end++ = *unit;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    *end++ = '\0';

    return stream << buf;
}
// NOLINTEND(*-avoid-c-arrays,*-magic-numbers,cppcoreguidelines-init-variables,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-vararg)

int PrintUtil::printedMemoryLength(bsls::Types::Int64 bytes, int precision)
// NOLINTBEGIN(cppcoreguidelines-init-variables)
{
    bsls::Types::Int64 value;
    int                remainder = 0;
    const char* unit = memoryHelper(&value, &remainder, bytes, precision);

    int ret = printedValueLength(value) + static_cast<int>(bsl::strlen(unit)) +
              1;
    if (unit != MEMORY_UNITS[0] && precision > 0) {
        ret += 1 + precision;
    }

    return ret;
}
// NOLINTEND(cppcoreguidelines-init-variables)

bsl::ostream& PrintUtil::printTimeIntervalNs(bsl::ostream&      stream,
                                             bsls::Types::Int64 timeIntervalNs,
                                             int                precision)
// NOLINTBEGIN(*-avoid-c-arrays,*-magic-numbers,cppcoreguidelines-init-variables,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-vararg)
{
    bsls::Types::Int64 value;
    int                remainder = 0;
    const char*        unit =
        timeIntervalNsHelper(&value, &remainder, timeIntervalNs, precision);

    char buf[64];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-type-vararg)
    int ret = snprintf(buf, sizeof(buf), "%lld", value);

    // 'end' points to the terminating NUL
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    char* end = buf + ret;

    if (unit != TIME_INTERVAL_NS_UNITS[0]) {
        ret = snprintf(end, sizeof(buf) - ret, ".%.*d", precision, remainder);
        end = end + ret;
    }

    *end++ = ' ';
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (; *unit; ++unit) {
        *end++ = *unit;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    *end++ = '\0';

    return stream << buf;
}
// NOLINTEND(*-avoid-c-arrays,*-magic-numbers,cppcoreguidelines-init-variables,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-vararg)

int PrintUtil::printedTimeIntervalNsLength(bsls::Types::Int64 timeIntervalNs,
                                           int                precision)
// NOLINTBEGIN(cppcoreguidelines-init-variables)
{
    bsls::Types::Int64 value;
    int                remainder = 0;
    const char*        unit =
        timeIntervalNsHelper(&value, &remainder, timeIntervalNs, precision);

    int ret = printedValueLength(value) + static_cast<int>(bsl::strlen(unit)) +
              1;
    if (unit != TIME_INTERVAL_NS_UNITS[0]) {
        ret += 1 + precision;
    }

    return ret;
}
// NOLINTEND(cppcoreguidelines-init-variables)

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
// NOLINTBEGIN(*-magic-numbers,bugprone-branch-clone)
{
    stream << num;

    // NOLINTNEXTLINE(*-magic-numbers)
    int mod100 = static_cast<int>(num % 100);
    // NOLINTNEXTLINE(*-magic-numbers)
    int mod10 = static_cast<int>(num % 10);

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
// NOLINTEND(*-magic-numbers,bugprone-branch-clone)

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
