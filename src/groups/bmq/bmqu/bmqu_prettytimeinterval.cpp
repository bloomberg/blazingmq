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

#include <bmqu_prettytimeinterval.h>

#include <bmqscm_version.h>

#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>

// BDE
#include <bdlb_print.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_cmath.h>
#include <bsl_iomanip.h>
#include <bsl_ios.h>
#include <bsl_limits.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqu {

namespace {

/// Unit names, from the smallest to the largest
// TODO: use 'constexpr bsl::string_view' once C++03 support is dropped.
const char* const k_UNITS[] = {"ns", "us", "ms", "s", "m", "h", "d", "w"};

/// Number of 'k_UNITS[i]' in one 'k_UNITS[i + 1]' (e.g. 1000 ns in 1 us)
const int k_UNIT_SIZES[] = {1000, 1000, 1000, 60, 60, 24, 7};

const int k_UNIT_SIZES_COUNT = sizeof(k_UNIT_SIZES) / sizeof(*k_UNIT_SIZES);

/// Powers of 10 as integers, so the carry check is exact
const bsls::Types::Int64 k_POWERS_OF_10[] = {
    1LL,                     // [0] 10^0
    10LL,                    // [1] 10^1
    100LL,                   // [2] 10^2
    1000LL,                  // [3] 10^3
    10000LL,                 // [4] 10^4
    100000LL,                // [5] 10^5
    1000000LL,               // [6] 10^6
    10000000LL,              // [7] 10^7
    100000000LL,             // [8] 10^8
    1000000000LL,            // [9] 10^9
    10000000000LL,           // [10] 10^10
    100000000000LL,          // [11] 10^11
    1000000000000LL,         // [12] 10^12
    10000000000000LL,        // [13] 10^13
    100000000000000LL,       // [14] 10^14
    1000000000000000LL,      // [15] 10^15
    10000000000000000LL,     // [16] 10^16
    100000000000000000LL,    // [17] 10^17
    1000000000000000000LL};  // [18] 10^18

}  // close unnamed namespace

// ------------------------
// class PrettyTimeInterval
// ------------------------

// ACCESSORS
bsl::ostream& PrettyTimeInterval::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << *this;

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, const PrettyTimeInterval& value)
{
    bsls::Types::Int64 timeNs    = value.timeNs();
    const int          precision = value.precision();

    // PRECONDITIONS
    // '10^18' is the largest power of 10 that fits in 'Int64'.  Note that the
    // decimal part is computed in 'double', so digits beyond the 15th are not
    // accurate.
    BSLS_ASSERT_SAFE(0 <= precision && precision <= 18);

    bdlma::LocalSequentialAllocator<1024> localAllocator(
        bslma::Default::allocator());
    bmqu::MemOutStream temp(&localAllocator);

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
    bsls::Types::Int64 unitNs  = 1;
    for (; unitIdx < k_UNIT_SIZES_COUNT; ++unitIdx) {
        const int unitSize = k_UNIT_SIZES[unitIdx];
        if (timeNs < unitNs * unitSize) {
            break;  // BREAK
        }
        unitNs *= unitSize;
    }

    // Nanoseconds are always printed without decimals
    const int fractionDigits = (unitIdx == 0) ? 0 : precision;

    const bsls::Types::Int64 fractionScale = k_POWERS_OF_10[fractionDigits];

    // Split the value into the whole part and the decimal part, rounded to
    // 'fractionDigits' digits
    bsls::Types::Int64 wholePart    = timeNs / unitNs;
    bsls::Types::Int64 fractionPart = lround(
        (static_cast<double>(timeNs - wholePart * unitNs) /
         static_cast<double>(unitNs)) *
        static_cast<double>(fractionScale));

    // The decimal part rounded up to a whole unit: carry it into the whole
    // part (e.g. '1.999' at precision 2 is '2.00')
    if (fractionPart == fractionScale) {
        ++wholePart;
        fractionPart = 0;
    }

    // The whole part reached the size of the unit: move to the next unit
    // (e.g. '1000.00 us' is printed as '1.00 ms')
    if (unitIdx < k_UNIT_SIZES_COUNT && wholePart == k_UNIT_SIZES[unitIdx]) {
        ++unitIdx;
        wholePart = 1;
    }

    // Print the whole part, then the decimal part padded with leading zeros
    temp << wholePart;
    if (fractionDigits > 0) {
        temp << "." << bsl::setw(fractionDigits) << bsl::setfill('0')
             << fractionPart;
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

}  // close package namespace
}  // close enterprise namespace
