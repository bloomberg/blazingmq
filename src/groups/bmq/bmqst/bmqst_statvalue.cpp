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

// bmqst_statvalue.cpp                                                -*-C++-*-
#include <bmqst_statvalue.h>

#include <bdlb_bitutil.h>
#include <bmqscm_version.h>
#include <bsl_algorithm.h>

#include <bmqstm_values.h>
#include <bsl_ostream.h>

namespace BloombergLP {
namespace bmqst {

namespace {

bsls::Types::Int64 MAX_INT = bsl::numeric_limits<bsls::Types::Int64>::max();
bsls::Types::Int64 MIN_INT = bsl::numeric_limits<bsls::Types::Int64>::min();

}  // close anonymous namespace

// ---------------
// class StatValue
// ---------------

// PRIVATE MANIPULATORS
void StatValue::aggregateLevel(int level, bsls::Types::Int64 snapshotTime)
{
    if (level + 1 >= static_cast<int>(d_levelStartIndices.size()) - 1) {
        // There is no higher aggregation level
        return;
    }

    int levelSize = d_levelStartIndices[level + 2] -
                    d_levelStartIndices[level + 1];
    d_curSnapshotIndices[level + 1] = (d_curSnapshotIndices[level + 1] + 1) %
                                      levelSize;
    Snapshot& aggSnapshot = d_history[d_curSnapshotIndices[level + 1] +
                                      d_levelStartIndices[level + 1]];

    // 'd_levelStartIndices[level]' will be the most recent snapshot of the
    // previous level (being aggregated here) since we call 'aggregateLevel'
    // when that element has been updated.
    const Snapshot& firstSnapshot    = d_history[d_levelStartIndices[level]];
    aggSnapshot.d_value              = firstSnapshot.d_value;
    aggSnapshot.d_min                = firstSnapshot.d_min;
    aggSnapshot.d_max                = firstSnapshot.d_max;
    aggSnapshot.d_incrementsOrEvents = firstSnapshot.d_incrementsOrEvents;
    aggSnapshot.d_decrementsOrSum    = firstSnapshot.d_decrementsOrSum;
    aggSnapshot.d_snapshotTime       = snapshotTime;

    for (int i = d_levelStartIndices[level] + 1;
         i < d_levelStartIndices[level + 1];
         ++i) {
        const Snapshot& snapshot = d_history[i];
        aggSnapshot.d_min        = bsl::min(aggSnapshot.d_min, snapshot.d_min);
        aggSnapshot.d_max        = bsl::max(aggSnapshot.d_max, snapshot.d_max);
    }

    if (d_curSnapshotIndices[level + 1] == 0) {
        // Advance to the next aggregation level
        aggregateLevel(level + 1, snapshotTime);
    }
}

// CREATORS
StatValue::StatValue(bslma::Allocator* basicAllocator)
: d_type(e_CONTINUOUS)
, d_currentStats()
, d_history(basicAllocator)
, d_levelStartIndices(basicAllocator)
, d_curSnapshotIndices(basicAllocator)
, d_min(0)
, d_max(0)
{
}

StatValue::StatValue(const bsl::vector<int>& sizes,
                     Type                    type,
                     bsls::Types::Int64      initTime,
                     bslma::Allocator*       basicAllocator)
: d_type(type)
, d_currentStats()
, d_history(basicAllocator)
, d_levelStartIndices(basicAllocator)
, d_curSnapshotIndices(basicAllocator)
, d_min(0)
, d_max(0)
{
    init(sizes, type, initTime);
}

StatValue::StatValue(const StatValue& other, bslma::Allocator* basicAllocator)
: d_type(other.d_type)
, d_currentStats(other.d_currentStats)
, d_history(other.d_history, basicAllocator)
, d_levelStartIndices(other.d_levelStartIndices, basicAllocator)
, d_curSnapshotIndices(other.d_curSnapshotIndices, basicAllocator)
, d_min(other.d_min)
, d_max(other.d_max)
{
}

// MANIPULATORS
StatValue& StatValue::operator=(const StatValue& rhs)
{
    d_currentStats       = rhs.d_currentStats;
    d_history            = rhs.d_history;
    d_levelStartIndices  = rhs.d_levelStartIndices;
    d_curSnapshotIndices = rhs.d_curSnapshotIndices;
    d_min                = rhs.d_min;
    d_max                = rhs.d_max;

    return *this;
}

void StatValue::addSnapshot(const StatValue& other)
{
    BSLS_ASSERT(d_type == other.d_type);

    const Snapshot& otherSnapshot =
        other.d_history[other.d_curSnapshotIndices[0]];

    if (d_type == e_CONTINUOUS) {
        d_currentStats.d_value += otherSnapshot.d_value;
        d_currentStats.d_min += otherSnapshot.d_min;
        d_currentStats.d_max += otherSnapshot.d_max;
    }
    else {
        if (d_currentStats.d_min == MAX_INT) {
            d_currentStats.d_min = otherSnapshot.d_min;
        }
        else if (otherSnapshot.d_min != MAX_INT) {
            // Static cast to truncate away decimal.
            d_currentStats.d_min = bsl::min(
                static_cast<bsls::Types::Int64>(d_currentStats.d_min),
                otherSnapshot.d_min);
        }

        if (d_currentStats.d_max == MIN_INT) {
            d_currentStats.d_max = otherSnapshot.d_max;
        }
        else if (otherSnapshot.d_max != MIN_INT) {
            // Static cast to truncate away decimal.
            d_currentStats.d_max = bsl::max(
                static_cast<bsls::Types::Int64>(d_currentStats.d_max),
                otherSnapshot.d_max);
        }
    }

    d_currentStats.d_incrementsOrEvents += otherSnapshot.d_incrementsOrEvents;
    d_currentStats.d_decrementsOrSum += otherSnapshot.d_decrementsOrSum;
}

void StatValue::setFromUpdate(const bmqstm::StatValueUpdate& update)
{
    typedef bmqstm::StatValueFields                 Fields;
    bsl::vector<bsls::Types::Int64>::const_iterator f =
        update.fields().begin();
    for (int i = 0; f != update.fields().end() && i < Fields::NUM_ENUMERATORS;
         ++i) {
        if (bdlb::BitUtil::isBitSet(update.fieldMask(), i)) {
            switch (i) {
            case Fields::E_ABSOLUTE_MIN: {
                d_min = *f;
            } break;
            case Fields::E_ABSOLUTE_MAX: {
                d_max = *f;
            } break;
            case Fields::E_MIN: {
                d_currentStats.d_min = *f;
                d_min = bsl::min(d_min, d_currentStats.d_min.load());
            } break;
            case Fields::E_MAX: {
                d_currentStats.d_max = *f;
                d_max = bsl::max(d_max, d_currentStats.d_max.load());
            } break;
            case Fields::E_INCREMENTS:
            case Fields::E_EVENTS: {
                d_currentStats.d_incrementsOrEvents = *f;
            } break;
            case Fields::E_DECREMENTS:
            case Fields::E_SUM: {
                d_currentStats.d_decrementsOrSum = *f;
            } break;
            case Fields::E_VALUE: {
                d_currentStats.d_value = *f;
            } break;
            }
            ++f;
        }
    }
}

void StatValue::takeSnapshot(bsls::Types::Int64 snapshotTime)
{
    bsls::Types::Int64 value = d_currentStats.d_value;
    bsls::Types::Int64 incrementsOrEvents;
    bsls::Types::Int64 decrementsOrSum;
    bsls::Types::Int64 min;
    bsls::Types::Int64 max;

    incrementsOrEvents = d_currentStats.d_incrementsOrEvents;
    decrementsOrSum    = d_currentStats.d_decrementsOrSum;
    if (d_type == e_CONTINUOUS) {
        min = d_currentStats.d_min.swap(value);
        max = d_currentStats.d_max.swap(value);
    }
    else {
        min = d_currentStats.d_min.swap(MAX_INT);
        max = d_currentStats.d_max.swap(MIN_INT);
    }

    // Update values since creation
    d_min = bsl::min(d_min, min);
    d_max = bsl::max(d_max, max);

    int levelSize           = d_levelStartIndices[1] - d_levelStartIndices[0];
    d_curSnapshotIndices[0] = (d_curSnapshotIndices[0] + 1) % levelSize;
    Snapshot& snapshot      = d_history[d_curSnapshotIndices[0]];

    snapshot.d_value              = value;
    snapshot.d_min                = min;
    snapshot.d_max                = max;
    snapshot.d_incrementsOrEvents = incrementsOrEvents;
    snapshot.d_decrementsOrSum    = decrementsOrSum;
    snapshot.d_snapshotTime       = snapshotTime;

    if (d_curSnapshotIndices[0] == 0) {
        // We've performed enough snapshots to advance to the next aggregation
        // level
        aggregateLevel(0, snapshotTime);
    }
}

void StatValue::clear(bsls::Types::Int64 snapshotTime)
{
    d_currentStats.reset(d_type == e_DISCRETE, 0);
    d_curSnapshotIndices.assign(d_curSnapshotIndices.size(), 0);

    for (size_t i = 0; i < d_history.size(); ++i) {
        d_history[i].reset(d_type == e_DISCRETE, snapshotTime);
    }

    if (d_type == e_DISCRETE) {
        d_min = MAX_INT;
        d_max = MIN_INT;
    }
    else {
        d_min = 0;
        d_max = 0;
    }
}

void StatValue::init(const bsl::vector<int>& sizes,
                     Type                    type,
                     bsls::Types::Int64      snapshotTime)
{
    d_type = type;
    d_levelStartIndices.resize(sizes.size() + 1);
    d_curSnapshotIndices.assign(sizes.size(), 0);
    d_min = (d_type == e_DISCRETE ? MAX_INT : 0);
    d_max = (d_type == e_DISCRETE ? MIN_INT : 0);
    d_currentStats.reset(d_type == e_DISCRETE, 0);

    int historySize = 0;
    for (size_t i = 0; i < sizes.size(); ++i) {
        d_levelStartIndices[i] = historySize;
        historySize += sizes[i];
    }
    d_levelStartIndices.back() = historySize;

    d_history.clear();
    d_history.resize(historySize);

    for (size_t i = 0; i < d_history.size(); ++i) {
        d_history[i].reset(d_type == e_DISCRETE, snapshotTime);
    }
}

void StatValue::syncSnapshotSchedule(const StatValue& other)
{
    d_curSnapshotIndices = other.d_curSnapshotIndices;
    BSLS_ASSERT_OPT(d_history.size() == other.d_history.size());
    for (size_t i = 0; i < d_history.size(); ++i) {
        d_history[i].d_snapshotTime = other.d_history[i].d_snapshotTime;
    }
}

// ACCESSORS
bsl::ostream&
StatValue::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("CurrentStats", d_currentStats);
    printer.printAttribute("History", d_history);
    printer.printAttribute("LevelStartIndices", d_levelStartIndices);
    printer.printAttribute("CurSnapshotIndices", d_curSnapshotIndices);
    printer.printAttribute("Min", d_min);
    printer.printAttribute("Max", d_max);
    printer.end();

    return stream;
}

// --------------------
// struct StatValueUtil
// --------------------

void StatValueUtil::loadUpdateImp(bmqstm::StatValueUpdate* update,
                                  const StatValue&         value,
                                  bsl::uint32_t            valueFieldMask,
                                  bool                     fullUpdate)
{
    typedef bmqstm::StatValueFields Fields;
    bsl::uint32_t                   mask    = 0;
    const StatValue::Snapshot&      current = value.snapshot(
        StatValue::SnapshotLocation());

    bool                       full = fullUpdate;
    const StatValue::Snapshot* last = 0;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !full && 1 == value.historySize(0) && 1 == value.numLevels())) {
        full = true;
    }
    else {
        const StatValue::Snapshot& lastSnapshot = value.snapshot(
            BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(1 == value.historySize(0))
                ? StatValue::SnapshotLocation(1, 0)
                : StatValue::SnapshotLocation(0, 1));
        last = &lastSnapshot;
    }

    update->fields().reserve(Fields::NUM_ENUMERATORS);
    for (int i = 0; i < Fields::NUM_ENUMERATORS; ++i) {
        if (bdlb::BitUtil::isBitSet(valueFieldMask, i)) {
            switch (i) {
            case Fields::E_ABSOLUTE_MIN: {
                if (full) {
                    // Only set on full update, as the 'MIN' update will catch
                    // any change here.

                    update->fields().push_back(value.min());
                    mask = bdlb::BitUtil::withBitSet(mask, i);
                }
            } break;
            case Fields::E_ABSOLUTE_MAX: {
                if (full) {
                    // Only set on full update, as the 'MAX' update will catch
                    // any change here.

                    update->fields().push_back(value.max());
                    mask = bdlb::BitUtil::withBitSet(mask, i);
                }
            } break;
            case Fields::E_MIN: {
                if (full || current.min() != last->min()) {
                    update->fields().push_back(current.min());
                    mask = bdlb::BitUtil::withBitSet(mask, i);
                }
            } break;
            case Fields::E_MAX: {
                if (full || current.max() != last->max()) {
                    update->fields().push_back(current.max());
                    mask = bdlb::BitUtil::withBitSet(mask, i);
                }
            } break;
            case Fields::E_VALUE: {
                if (StatValue::e_CONTINUOUS == value.type() &&
                    (full || current.value() != last->value())) {
                    update->fields().push_back(current.value());
                    mask = bdlb::BitUtil::withBitSet(mask, i);
                }
            } break;
            case Fields::E_INCREMENTS: {
                if (StatValue::e_CONTINUOUS == value.type() &&
                    (full || current.increments() != last->increments())) {
                    update->fields().push_back(current.increments());
                    mask = bdlb::BitUtil::withBitSet(mask, i);
                }
            } break;
            case Fields::E_DECREMENTS: {
                if (StatValue::e_CONTINUOUS == value.type() &&
                    (full || current.decrements() != last->decrements())) {
                    update->fields().push_back(current.decrements());
                    mask = bdlb::BitUtil::withBitSet(mask, i);
                }
            } break;
            case Fields::E_EVENTS: {
                if (StatValue::e_DISCRETE == value.type() &&
                    (full || current.events() != last->events())) {
                    update->fields().push_back(current.events());
                    mask = bdlb::BitUtil::withBitSet(mask, i);
                }
            } break;
            case Fields::E_SUM: {
                if (StatValue::e_DISCRETE == value.type() &&
                    (full || current.sum() != last->sum())) {
                    update->fields().push_back(current.sum());
                    mask = bdlb::BitUtil::withBitSet(mask, i);
                }
            } break;
            }
        }
    }
    update->fieldMask() = mask;
}

}  // close package namespace
}  // close enterprise namespace
