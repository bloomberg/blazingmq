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

// mwcst_value.cpp -*-C++-*-
#include <mwcst_value.h>

#include <bdlb_hashutil.h>
#include <bdlt_dateutil.h>
#include <bdlt_timeutil.h>
#include <mwcscm_version.h>

#include <bsl_cstring.h>

namespace BloombergLP {
namespace mwcst {

enum {
    VALUE_NULLTYPE,
    VALUE_BOOLTYPE,
    VALUE_INTTYPE,
    VALUE_INT64TYPE,
    VALUE_DOUBLETYPE,
    VALUE_DATETZTYPE,
    VALUE_TIMETZTYPE,
    VALUE_DATETIMETZTYPE,
    VALUE_STRINGTYPE,
    VALUE_BADTYPE
};

// -----------
// class Value
// -----------

// MANIPUALATORS
void Value::ownValue()
{
    if (!d_owned && d_value.is<bslstl::StringRef>() &&
        !d_value.the<bslstl::StringRef>().isEmpty()) {
        bslstl::StringRef& ref = d_value.the<bslstl::StringRef>();
        void*              buf = d_allocator_p->allocate(ref.length());
        bsl::memcpy(buf, ref.data(), ref.length());
        ref.assign((char*)buf, ref.length());
        d_owned = true;
    }
}

// ACCESSORS
size_t Value::hash() const
{
    static bsl::hash<int>                 intHasher;
    static bsl::hash<bsls::Types::Int64>  int64Hasher;
    static bsl::hash<bsls::Types::Uint64> uint64Hasher;

    if (d_hash != 0) {
        return d_hash;
    }

    switch (d_value.typeIndex()) {
    case VALUE_NULLTYPE: {
        d_hash = 1;
    } break;
    case VALUE_BOOLTYPE: {
        d_hash = (d_value.the<bool>() ? 2 : 3);
    } break;
    case VALUE_INTTYPE: {
        d_hash = intHasher(d_value.the<int>());
    } break;
    case VALUE_INT64TYPE: {
        d_hash = int64Hasher(d_value.the<bsls::Types::Int64>());
    } break;
    case VALUE_DOUBLETYPE: {
        const double&              d = d_value.the<double>();
        const bsls::Types::Uint64* u =
            reinterpret_cast<const bsls::Types::Uint64*>(&d);
        d_hash = uint64Hasher(*u);
    } break;
    case VALUE_DATETZTYPE: {
        bsls::Types::Uint64 uint = bdlt::DateUtil::convertToYYYYMMDD(
            d_value.the<bdlt::DateTz>().localDate());
        uint <<= 32;
        uint += d_value.the<bdlt::DateTz>().offset();
        d_hash = uint64Hasher(uint);
    } break;
    case VALUE_TIMETZTYPE: {
        bsls::Types::Uint64 uint = bdlt::TimeUtil::convertToHHMMSSmmm(
            d_value.the<bdlt::TimeTz>().localTime());
        uint <<= 32;
        uint += d_value.the<bdlt::TimeTz>().offset();
        d_hash = uint64Hasher(uint);
    } break;
    case VALUE_DATETIMETZTYPE: {
        const bdlt::DatetimeTz& dt      = d_value.the<bdlt::DatetimeTz>();
        bsls::Types::Int64      hashInt = bdlt::DateUtil::convertToYYYYMMDD(
            dt.localDatetime().date());
        hashInt <<= 32;
        hashInt += bdlt::TimeUtil::convertToHHMMSSmmm(
            dt.localDatetime().time());

        d_hash = int64Hasher(hashInt);
        d_hash += intHasher(dt.offset());
    } break;
    case VALUE_STRINGTYPE: {
        d_hash = bdlb::HashUtil::hash1(
            d_value.the<bslstl::StringRef>().data(),
            static_cast<int>(d_value.the<bslstl::StringRef>().length()));
    } break;
    default: {
        d_hash = (size_t)-1;
    } break;
    }

    return d_hash;
}

}  // close package namespace

// FREE OPERATORS
bool mwcst::operator<(const mwcst::Value& lhs, const mwcst::Value& rhs)
{
    // give a partial order between types and withint types

    if (lhs.d_value.typeIndex() < rhs.d_value.typeIndex()) {
        return true;
    }

    if (lhs.d_value.typeIndex() > rhs.d_value.typeIndex()) {
        return false;
    }

    // same type comparison here
    switch (lhs.d_value.typeIndex()) {
    case VALUE_NULLTYPE: {
        return false;
    }
    case VALUE_BOOLTYPE: {
        return (lhs.the<bool>() < rhs.the<bool>());
    }
    case VALUE_INTTYPE: {
        return (lhs.the<int>() < rhs.the<int>());
    }
    case VALUE_INT64TYPE: {
        return (lhs.the<bsls::Types::Int64>() < rhs.the<bsls::Types::Int64>());
    }
    case VALUE_DOUBLETYPE: {
        return (lhs.the<double>() < rhs.the<double>());
    }
    case VALUE_DATETZTYPE: {
        const bdlt::DateTz& lhsDate = lhs.the<bdlt::DateTz>();
        const bdlt::DateTz& rhsDate = rhs.the<bdlt::DateTz>();
        if (lhsDate.localDate() == rhsDate.localDate()) {
            return lhsDate.offset() < rhsDate.offset();
        }
        else {
            return lhsDate.localDate() < rhsDate.localDate();
        }
    }
    case VALUE_TIMETZTYPE: {
        const bdlt::TimeTz& lhsTime = lhs.the<bdlt::TimeTz>();
        const bdlt::TimeTz& rhsTime = rhs.the<bdlt::TimeTz>();
        if (lhsTime.localTime() == rhsTime.localTime()) {
            return lhsTime.offset() < rhsTime.offset();
        }
        else {
            return lhsTime.localTime() < rhsTime.localTime();
        }
    }
    case VALUE_DATETIMETZTYPE: {
        const bdlt::DatetimeTz& lhsDatetime = lhs.the<bdlt::DatetimeTz>();
        const bdlt::DatetimeTz& rhsDatetime = rhs.the<bdlt::DatetimeTz>();
        if (lhsDatetime.localDatetime() == rhsDatetime.localDatetime()) {
            return lhsDatetime.offset() < rhsDatetime.offset();
        }
        else {
            return lhsDatetime.localDatetime() < rhsDatetime.localDatetime();
        }
    }
    case VALUE_STRINGTYPE: {
        return (lhs.the<bslstl::StringRef>() < rhs.the<bslstl::StringRef>());
    }
    default: {
        return false;
    }
    }
}

bsl::ostream& mwcst::operator<<(bsl::ostream&       stream,
                                const mwcst::Value& value)
{
    return stream << value.d_value;
}

}  // close enterprise namespace
