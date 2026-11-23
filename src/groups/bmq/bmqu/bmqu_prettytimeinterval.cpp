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

namespace BloombergLP {
namespace bmqu {

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

    static const char* k_UNITS[] = {"ns", "us", "ms", "s", "m", "h", "d", "w"};
    static const int   k_SIZES[] = {1000, 1000, 1000, 60, 60, 24, 7};
    static const int   k_SIZES_COUNT = sizeof(k_SIZES) / sizeof(*k_SIZES);

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
        const bsls::Types::Int64 quot = lround(static_cast<double>(timeNs) /
                                               static_cast<double>(div));
        temp << quot;
    }
    else {
        // Decimal part needed, compute the remainder and print the required
        // precision digits
        const bsls::Types::Int64 quot      = timeNs / div;
        const long               remainder = lround(
            (static_cast<double>(timeNs - quot * div) /
             static_cast<double>(div)) *
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

}  // close package namespace
}  // close enterprise namespace
