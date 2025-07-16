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

// bmqst_statutil.cpp                                                 -*-C++-*-
#include <bmqst_statutil.h>

#include <bmqscm_version.h>
#include <bsl_algorithm.h>
#include <bsl_limits.h>

namespace BloombergLP {
namespace bmqst {

namespace {

// CONSTANTS
const bsls::Types::Int64 k_NS_PER_SEC = 1000000000;

}  // close anonymous namespace

// ---------------
// struct StatUtil
// ---------------

// CLASS METHODS
bsls::Types::Int64 StatUtil::value(const StatValue&                   value,
                                   const StatValue::SnapshotLocation& snapshot)
{
    return value.snapshot(snapshot).value();
}

bsls::Types::Int64
StatUtil::valueDifference(const StatValue&                   value,
                          const StatValue::SnapshotLocation& firstSnapshot,
                          const StatValue::SnapshotLocation& secondSnapshot)
{
    return value.snapshot(firstSnapshot).value() -
           value.snapshot(secondSnapshot).value();
}

bsls::Types::Int64
StatUtil::increments(const StatValue&                   value,
                     const StatValue::SnapshotLocation& snapshot)
{
    return value.snapshot(snapshot).increments();
}

bsls::Types::Int64 StatUtil::incrementsDifference(
    const StatValue&                   value,
    const StatValue::SnapshotLocation& firstSnapshot,
    const StatValue::SnapshotLocation& secondSnapshot)
{
    return value.snapshot(firstSnapshot).increments() -
           value.snapshot(secondSnapshot).increments();
}

double
StatUtil::incrementRate(const StatValue&                   value,
                        const StatValue::SnapshotLocation& firstSnapshot,
                        const StatValue::SnapshotLocation& secondSnapshot)
{
    double diff = static_cast<double>(
        value.snapshot(secondSnapshot).increments() -
        value.snapshot(firstSnapshot).increments());

    double divisor = firstSnapshot.index() - secondSnapshot.index();
    for (int i = 0; i < firstSnapshot.level(); ++i) {
        divisor *= value.historySize(i);
    }

    return diff / divisor;
}

double StatUtil::incrementsPerSecond(
    const StatValue&                   value,
    const StatValue::SnapshotLocation& firstSnapshot,
    const StatValue::SnapshotLocation& secondSnapshot)
{
    double diff = static_cast<double>(
        value.snapshot(secondSnapshot).increments() -
        value.snapshot(firstSnapshot).increments());

    bsls::Types::Int64 duration =
        value.snapshot(secondSnapshot).snapshotTime() -
        value.snapshot(firstSnapshot).snapshotTime();

    double divisor = static_cast<double>(duration) / k_NS_PER_SEC;
    return diff / divisor;
}

bsls::Types::Int64
StatUtil::decrements(const StatValue&                   value,
                     const StatValue::SnapshotLocation& snapshot)
{
    return value.snapshot(snapshot).decrements();
}

bsls::Types::Int64 StatUtil::decrementsDifference(
    const StatValue&                   value,
    const StatValue::SnapshotLocation& firstSnapshot,
    const StatValue::SnapshotLocation& secondSnapshot)
{
    return value.snapshot(firstSnapshot).decrements() -
           value.snapshot(secondSnapshot).decrements();
}

double
StatUtil::decrementRate(const StatValue&                   value,
                        const StatValue::SnapshotLocation& firstSnapshot,
                        const StatValue::SnapshotLocation& secondSnapshot)
{
    double diff = static_cast<double>(
        value.snapshot(secondSnapshot).decrements() -
        value.snapshot(firstSnapshot).decrements());

    double divisor = firstSnapshot.index() - secondSnapshot.index();
    for (int i = 0; i < firstSnapshot.level(); ++i) {
        divisor *= value.historySize(i);
    }

    return diff / divisor;
}

double StatUtil::decrementsPerSecond(
    const StatValue&                   value,
    const StatValue::SnapshotLocation& firstSnapshot,
    const StatValue::SnapshotLocation& secondSnapshot)
{
    double diff = static_cast<double>(
        value.snapshot(secondSnapshot).decrements() -
        value.snapshot(firstSnapshot).decrements());

    bsls::Types::Int64 duration =
        value.snapshot(secondSnapshot).snapshotTime() -
        value.snapshot(firstSnapshot).snapshotTime();

    double divisor = static_cast<double>(duration) / k_NS_PER_SEC;
    return diff / divisor;
}

double StatUtil::average(const StatValue&                   value,
                         const StatValue::SnapshotLocation& firstSnapshot,
                         const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(firstSnapshot.level() == secondSnapshot.level());
    BSLS_ASSERT(firstSnapshot > secondSnapshot);

    bsls::Types::Int64 sum = 0;
    for (StatValue::SnapshotLocation loc(firstSnapshot);
         loc.index() >= secondSnapshot.index();
         loc.setIndex(loc.index() - 1)) {
        sum += value.snapshot(loc).value();
    }

    double divisor = 1 + firstSnapshot.index() - secondSnapshot.index();
    for (int i = 0; i < firstSnapshot.level(); ++i) {
        divisor *= value.historySize(i);
    }

    return static_cast<double>(sum) / divisor;
}

double StatUtil::rate(const StatValue&                   value,
                      const StatValue::SnapshotLocation& firstSnapshot,
                      const StatValue::SnapshotLocation& secondSnapshot)
{
    double diff = static_cast<double>(value.snapshot(secondSnapshot).value() -
                                      value.snapshot(firstSnapshot).value());

    int divisor = firstSnapshot.index() - secondSnapshot.index();
    for (int i = 0; i < firstSnapshot.level(); ++i) {
        divisor *= value.historySize(i);
    }

    return diff / divisor;
}

double
StatUtil::ratePerSecond(const StatValue&                   value,
                        const StatValue::SnapshotLocation& firstSnapshot,
                        const StatValue::SnapshotLocation& secondSnapshot)
{
    double diff = static_cast<double>(value.snapshot(secondSnapshot).value() -
                                      value.snapshot(firstSnapshot).value());

    bsls::Types::Int64 duration =
        value.snapshot(secondSnapshot).snapshotTime() -
        value.snapshot(firstSnapshot).snapshotTime();

    double divisor = static_cast<double>(duration) / k_NS_PER_SEC;
    return diff / divisor;
}

bsls::Types::Int64 StatUtil::min(const StatValue&                   value,
                                 const StatValue::SnapshotLocation& snapshot)
{
    return value.snapshot(snapshot).min();
}

bsls::Types::Int64
StatUtil::rangeMin(const StatValue&                   value,
                   const StatValue::SnapshotLocation& firstSnapshot,
                   const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(firstSnapshot.level() == secondSnapshot.level());
    int start = bsl::min(firstSnapshot.index(), secondSnapshot.index());
    int end   = bsl::max(firstSnapshot.index(), secondSnapshot.index());

    StatValue::SnapshotLocation startLoc(firstSnapshot.level(), start);
    bsls::Types::Int64          min = value.snapshot(startLoc).min();
    for (int i = start + 1; i <= end; ++i) {
        StatValue::SnapshotLocation loc(secondSnapshot.level(), i);
        min = bsl::min(min, value.snapshot(loc).min());
    }

    return min;
}

bsls::Types::Int64 StatUtil::absoluteMin(const StatValue& value)
{
    return value.min();
}

bsls::Types::Int64 StatUtil::max(const StatValue&                   value,
                                 const StatValue::SnapshotLocation& snapshot)
{
    return value.snapshot(snapshot).max();
}

bsls::Types::Int64
StatUtil::rangeMax(const StatValue&                   value,
                   const StatValue::SnapshotLocation& firstSnapshot,
                   const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(firstSnapshot.level() == secondSnapshot.level());
    int start = bsl::min(firstSnapshot.index(), secondSnapshot.index());
    int end   = bsl::max(firstSnapshot.index(), secondSnapshot.index());

    StatValue::SnapshotLocation startLoc(firstSnapshot.level(), start);
    bsls::Types::Int64          max = value.snapshot(startLoc).max();
    for (int i = start + 1; i <= end; ++i) {
        StatValue::SnapshotLocation loc(secondSnapshot.level(), i);
        max = bsl::max(max, value.snapshot(loc).max());
    }

    return max;
}

bsls::Types::Int64 StatUtil::rangeMaxValueDifference(
    const StatValue&                   value,
    const StatValue::SnapshotLocation& firstSnapshot,
    const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(firstSnapshot.level() == secondSnapshot.level());
    BSLS_ASSERT(firstSnapshot > secondSnapshot);

    bsls::Types::Int64 val_max =
        bsl::numeric_limits<bsls::Types::Int64>::min();

    StatValue::SnapshotLocation loc(firstSnapshot);
    loc.setIndex(loc.index() - 1);

    for (; loc.index() >= secondSnapshot.index();
         loc.setIndex(loc.index() - 1)) {
        val_max = bsl::max(
            val_max,
            valueDifference(value,
                            loc,
                            StatValue::SnapshotLocation(loc.level(),
                                                        loc.index() + 1)));
    }

    return val_max;
}

bsls::Types::Int64 StatUtil::rangeMaxIncrementsDifference(
    const StatValue&                   value,
    const StatValue::SnapshotLocation& firstSnapshot,
    const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(firstSnapshot.level() == secondSnapshot.level());
    BSLS_ASSERT(firstSnapshot > secondSnapshot);

    bsls::Types::Int64 val_max =
        bsl::numeric_limits<bsls::Types::Int64>::min();

    StatValue::SnapshotLocation loc(firstSnapshot);
    loc.setIndex(loc.index() - 1);

    for (; loc.index() >= secondSnapshot.index();
         loc.setIndex(loc.index() - 1)) {
        val_max = bsl::max(
            val_max,
            incrementsDifference(
                value,
                loc,
                StatValue::SnapshotLocation(loc.level(), loc.index() + 1)));
    }

    return val_max;
}

bsls::Types::Int64 StatUtil::rangeMaxDecrementsDifference(
    const StatValue&                   value,
    const StatValue::SnapshotLocation& firstSnapshot,
    const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(firstSnapshot.level() == secondSnapshot.level());
    BSLS_ASSERT(firstSnapshot > secondSnapshot);

    bsls::Types::Int64 val_max =
        bsl::numeric_limits<bsls::Types::Int64>::min();

    StatValue::SnapshotLocation loc(firstSnapshot);
    loc.setIndex(loc.index() - 1);

    for (; loc.index() >= secondSnapshot.index();
         loc.setIndex(loc.index() - 1)) {
        val_max = bsl::max(
            val_max,
            decrementsDifference(
                value,
                loc,
                StatValue::SnapshotLocation(loc.level(), loc.index() + 1)));
    }

    return val_max;
}

bsls::Types::Int64 StatUtil::absoluteMax(const StatValue& value)
{
    return value.max();
}

bsls::Types::Int64
StatUtil::events(const StatValue&                   value,
                 const StatValue::SnapshotLocation& snapshot)
{
    BSLS_ASSERT(value.type() == StatValue::e_DISCRETE);
    return value.snapshot(snapshot).events();
}

bsls::Types::Int64
StatUtil::eventsDifference(const StatValue&                   value,
                           const StatValue::SnapshotLocation& firstSnapshot,
                           const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(value.type() == StatValue::e_DISCRETE);

    return value.snapshot(firstSnapshot).events() -
           value.snapshot(secondSnapshot).events();
}

bsls::Types::Int64 StatUtil::sum(const StatValue&                   value,
                                 const StatValue::SnapshotLocation& snapshot)
{
    BSLS_ASSERT(value.type() == StatValue::e_DISCRETE);
    return value.snapshot(snapshot).sum();
}

bsls::Types::Int64
StatUtil::sumDifference(const StatValue&                   value,
                        const StatValue::SnapshotLocation& firstSnapshot,
                        const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(value.type() == StatValue::e_DISCRETE);

    return value.snapshot(firstSnapshot).sum() -
           value.snapshot(secondSnapshot).sum();
}

bsls::Types::Int64
StatUtil::averagePerEvent(const StatValue&                   value,
                          const StatValue::SnapshotLocation& firstSnapshot,
                          const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(value.type() == StatValue::e_DISCRETE);

    bsls::Types::Int64 events = eventsDifference(value,
                                                 firstSnapshot,
                                                 secondSnapshot);
    if (events == 0) {
        return bsl::numeric_limits<bsls::Types::Int64>::max();
    }
    else {
        bsls::Types::Int64 sum = sumDifference(value,
                                               firstSnapshot,
                                               secondSnapshot);
        return sum / events;
    }
}

double StatUtil::averagePerEventReal(
    const StatValue&                   value,
    const StatValue::SnapshotLocation& firstSnapshot,
    const StatValue::SnapshotLocation& secondSnapshot)
{
    BSLS_ASSERT(value.type() == StatValue::e_DISCRETE);

    bsls::Types::Int64 events = eventsDifference(value,
                                                 firstSnapshot,
                                                 secondSnapshot);
    if (events == 0) {
        return bsl::numeric_limits<double>::quiet_NaN();
    }
    else {
        bsls::Types::Int64 sum = sumDifference(value,
                                               firstSnapshot,
                                               secondSnapshot);
        return static_cast<double>(sum) / static_cast<double>(events);
    }
}

}  // close package namespace
}  // close enterprise namespace
