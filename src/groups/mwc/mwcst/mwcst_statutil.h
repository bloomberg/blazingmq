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

// mwcst_statutil.h                                                   -*-C++-*-
#ifndef INCLUDED_MWCST_STATUTIL
#define INCLUDED_MWCST_STATUTIL

//@PURPOSE: Provide utility functions operating on a 'mwcst::StatValue'.
//
//@DEPRECATED: Use 'mwcst::MetricUtil' instead.
//
//@CLASSES:
// mwcst::StatUtil : container for functions operating on a 'mwcst::StatValue'.
//
//@SEE_ALSO: mwcst_metricutil, mwcst_table
//
//@DESCRIPTION: This component defines a utility, 'mwcst::StatUtil' containing
// functions for calculating statistics of a provided 'mwcst::StatValue'.
//
// Applications should probably not be using these functions directly with a
// given 'mwcst::StatValue' except for specific cases.  Most likely this
// utility will instead be used to create a 'mwcst::Table'.
//
// Refer to the documentation of 'mwcst::Table' for a usage example.

#include <mwcst_statvalue.h>

#ifndef INCLUDED_BSLA_ANNOTATIONS
#include <bsla_annotations.h>
#endif

#ifndef INCLUDED_BSLS_TYPES
#include <bsls_types.h>
#endif

namespace BloombergLP {
namespace mwcst {

// ===============
// struct StatUtil
// ===============

/// Utilities for calculating statistics.  In all functions taking
/// `firstSnapshot` and `secondSnapshot`, the behavior is undefined unless
/// `firstSnapshot > secondSnapshot` and
/// `firstSnapshot.level() == secondSnapshot.level()`
///
/// @deprecated Use `mwcst::MetricUtil` instead.
struct BSLA_DEPRECATED StatUtil {
    // CLASS METHODS

    /// Return the value of the specified `snapshot` of the specified
    /// `value`.
    static bsls::Types::Int64
    value(const StatValue& value, const StatValue::SnapshotLocation& snapshot);

    /// Return the difference of the value of the specified `firstSnapshot`
    /// and the specified `secondSnapshot` of the specified `value`.
    static bsls::Types::Int64
    valueDifference(const StatValue&                   value,
                    const StatValue::SnapshotLocation& firstSnapshot,
                    const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the total number of increments recorded by the
    /// specified `value` up to the specified `snapshot`.
    static bsls::Types::Int64
    increments(const StatValue&                   value,
               const StatValue::SnapshotLocation& snapshot);

    /// Return the difference in the number of increments recorded by the
    /// specified `value` between the specified `firstSnapshot` and the
    /// specified `secondSnapshot`.
    static bsls::Types::Int64
    incrementsDifference(const StatValue&                   value,
                         const StatValue::SnapshotLocation& firstSnapshot,
                         const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the average number of increments of the specified `value`
    /// per snapshot from the specified `firstSnapshot` to the specified
    /// `secondSnapshot`
    static double
    incrementRate(const StatValue&                   value,
                  const StatValue::SnapshotLocation& firstSnapshot,
                  const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the average number of increments of the specified `value`
    /// per second from the specified `firstSnapshot` to the specified
    /// `secondSnapshot`
    static double
    incrementsPerSecond(const StatValue&                   value,
                        const StatValue::SnapshotLocation& firstSnapshot,
                        const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the total number of decrements recorded by the
    /// specified `value` up to the specified `snapshot`.
    static bsls::Types::Int64
    decrements(const StatValue&                   value,
               const StatValue::SnapshotLocation& snapshot);

    /// Return the difference in the number of decrements recorded by the
    /// specified `value` between the specified `firstSnapshot` and the
    /// specified `secondSnapshot`.
    static bsls::Types::Int64
    decrementsDifference(const StatValue&                   value,
                         const StatValue::SnapshotLocation& firstSnapshot,
                         const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the average number of decrements of the specified `value`
    /// per snapshot from the specified `firstSnapshot` to the specified
    /// `secondSnapshot`.
    static double
    decrementRate(const StatValue&                   value,
                  const StatValue::SnapshotLocation& firstSnapshot,
                  const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the average number of decrements of the specified `value`
    /// per second from the specified `firstSnapshot` to the specified
    /// `secondSnapshot`.
    static double
    decrementsPerSecond(const StatValue&                   value,
                        const StatValue::SnapshotLocation& firstSnapshot,
                        const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the average value per snapshot of the specified `value` from
    /// the specified `firstSnapshot` to the specified `secondSnapshot`.
    /// The behavior is undefined unless `firstSnapshot > secondSnapshot`.
    static double average(const StatValue&                   value,
                          const StatValue::SnapshotLocation& firstSnapshot,
                          const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the average change in the value of the specified `value` per
    /// snapshot from the specified `firstSnapshot` to the specified
    /// `secondSnapshot`.
    static double rate(const StatValue&                   value,
                       const StatValue::SnapshotLocation& firstSnapshot,
                       const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the average change in the value of the specified `value` per
    /// second from the specified `firstSnapshot` to the specified
    /// `secondSnapshot`.
    static double
    ratePerSecond(const StatValue&                   value,
                  const StatValue::SnapshotLocation& firstSnapshot,
                  const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the minimum value reported to the specified `value` for the
    /// specified `snapshot`.  If nothing was reported during this
    /// `snapshot`, the maximum Int64 is returned.
    static bsls::Types::Int64 min(const StatValue&                   value,
                                  const StatValue::SnapshotLocation& snapshot);

    /// Return the minimum value reported to the specified `value` between
    /// the specified `firstSnapshot` and `secondSnapshot` inclusive.
    static bsls::Types::Int64
    rangeMin(const StatValue&                   value,
             const StatValue::SnapshotLocation& firstSnapshot,
             const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the all-time minimum value observed by the specified
    /// `value`.  If nothing was ever reported, the maximum Int64 is
    /// returned.
    static bsls::Types::Int64 absoluteMin(const StatValue& value);

    /// Return the maximum value reported to the specified `value` for the
    /// specified `snapshot`.  If nothing was reported during this
    /// `snapshot`, the minumim Int64 is returned.
    static bsls::Types::Int64 max(const StatValue&                   value,
                                  const StatValue::SnapshotLocation& snapshot);

    /// Return the maximum value reported to the specified `value` between
    /// the specified `firstSnapshot` and `secondSnapshot` inclusive.
    static bsls::Types::Int64
    rangeMax(const StatValue&                   value,
             const StatValue::SnapshotLocation& firstSnapshot,
             const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the maximum value difference reported to the specified
    /// `value` between the specified `firstSnapshot` and `secondSnapshot`
    /// inclusive.
    static bsls::Types::Int64
    rangeMaxValueDifference(const StatValue&                   value,
                            const StatValue::SnapshotLocation& firstSnapshot,
                            const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the maximum increments difference reported to the specified
    /// `value` between the specified `firstSnapshot` and `secondSnapshot`
    /// inclusive.
    static bsls::Types::Int64 rangeMaxIncrementsDifference(
        const StatValue&                   value,
        const StatValue::SnapshotLocation& firstSnapshot,
        const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the maximum decrements difference reported to the specified
    /// `value` between the specified `firstSnapshot` and `secondSnapshot`
    /// inclusive.
    static bsls::Types::Int64 rangeMaxDecrementsDifference(
        const StatValue&                   value,
        const StatValue::SnapshotLocation& firstSnapshot,
        const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the all-time maximum value observed by thie specified
    /// `value.` If nothing was ever reported, the minimum Int64 is
    /// returned.
    static bsls::Types::Int64 absoluteMax(const StatValue& value);

    // ** Discrete StatValue functions only           **
    // ** The behavior is undefined unless            **
    // ** 'value.type() == StatValue::DMCST_DISCRETE' **

    /// Return the total number of events recorded by the
    /// specified `value` up to the specified `snapshot`.
    static bsls::Types::Int64
    events(const StatValue&                   value,
           const StatValue::SnapshotLocation& snapshot);

    /// Return the difference in the number of events recorded by the
    /// specified `value` between the specified `firstSnapshot` and the
    /// specified `secondSnapshot`.
    static bsls::Types::Int64
    eventsDifference(const StatValue&                   value,
                     const StatValue::SnapshotLocation& firstSnapshot,
                     const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the total sum of events recorded by the
    /// specified `value` up to the specified `snapshot`.
    static bsls::Types::Int64 sum(const StatValue&                   value,
                                  const StatValue::SnapshotLocation& snapshot);

    /// Return the difference in the sum of all events recorded by the
    /// specified `value` between the specified `firstSnapshot` and the
    /// specified `secondSnapshot`.
    static bsls::Types::Int64
    sumDifference(const StatValue&                   value,
                  const StatValue::SnapshotLocation& firstSnapshot,
                  const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the average value reported to the specified `value` between
    /// the specified `firstSnapshot` and the specified `secondSnapshot`.
    /// If nothing was ever reported, the maximum Int64 is returned.
    static bsls::Types::Int64
    averagePerEvent(const StatValue&                   value,
                    const StatValue::SnapshotLocation& firstSnapshot,
                    const StatValue::SnapshotLocation& secondSnapshot);

    /// Return the average value reported to the specified `value` between
    /// the specified `firstSnapshot` and the specified `secondSnapshot`.
    /// If nothing was ever reported, `NaN` is returned.
    static double
    averagePerEventReal(const StatValue&                   value,
                        const StatValue::SnapshotLocation& firstSnapshot,
                        const StatValue::SnapshotLocation& secondSnapshot);
};

}  // close package namespace
}  // close enterprise namespace

#endif
