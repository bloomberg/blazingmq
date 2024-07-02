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

// mwcst_statvalue.h                                                  -*-C++-*-
#ifndef INCLUDED_MWCST_STATVALUE
#define INCLUDED_MWCST_STATVALUE

//@PURPOSE: Provide a statistic-collecting value.
//
//@CLASSES:
// mwcst::StatValue     : value (or variable) able to collect statistics.
// mwcst::StatValueUtil : non-primitive operations on a 'StatValue'.
//
//@SEE_ALSO:
//  mwcst_statcontext
//  mwcst_statutil
//
//@DESCRIPTION: This component defines a mechanism, 'mwcst::StatValue', which
// maintains a value and collects statistics about it and changes to it.  It
// can be asked to calculate a number of statistics over its history.
//
// You probably should not use this class directly.  Instead, you should use
// the 'mwcst::StatContext' component.  Refer to the usage examples in the
// documentation of that component.
//
/// Thread Safety
///-------------
// 'adjustValue' and 'setValue' are thread-safe.  All other functions are not.

#ifndef INCLUDED_BSLIM_PRINTER
#include <bslim_printer.h>
#endif

#ifndef INCLUDED_BSLMA_USESBSLMAALLOCATOR
#include <bslma_usesbslmaallocator.h>
#endif

#ifndef INCLUDED_BSLMF_NESTEDTRAITDECLARATION
#include <bslmf_nestedtraitdeclaration.h>
#endif

#ifndef INCLUDED_BSLS_ATOMIC
#include <bsls_atomic.h>
#endif

#ifndef INCLUDED_BSL_ALGORITHM
#include <bsl_algorithm.h>
#endif

#ifndef INCLUDED_BSL_LIMITS
#include <bsl_limits.h>
#endif

#ifndef INCLUDED_BSLS_TYPES
#include <bsls_types.h>
#endif

#ifndef INCLUDED_BSL_CSTDINT
#include <bsl_cstdint.h>
#endif

#ifndef INCLUDED_BSL_VECTOR
#include <bsl_vector.h>
#endif

namespace BloombergLP {

namespace mwcstm {
class StatValueUpdate;
}

namespace mwcst {

// FORWARD DECLARATIONS
class StatValue;

// =====================
// class StatValue_Value
// =====================

template <typename T, typename NON_ATOMIC = T>
class StatValue_Value {
  private:
    // DATA
    T d_value;
    T d_min;
    T d_max;

    // # of increments if continuous value, # of events if discrete value
    T d_incrementsOrEvents;

    // # of decrements if continuous value, sum of all reported value if
    // discrete value
    T d_decrementsOrSum;

    bsls::Types::Int64 d_snapshotTime;

    // FRIENDS
    friend class StatValue;

  public:
    // CREATORS
    StatValue_Value();
    StatValue_Value(const StatValue_Value& other);

    // MANIPULATORS
    StatValue_Value& operator=(const StatValue_Value& rhs);
    void             reset(bool extremeMinMax, bsls::Types::Int64 resetTime);

    // ACCESSORS
    NON_ATOMIC min() const;
    NON_ATOMIC max() const;

    // Only for continuous values
    NON_ATOMIC value() const;
    NON_ATOMIC increments() const;
    NON_ATOMIC decrements() const;

    // Only for discrete values
    NON_ATOMIC events() const;
    NON_ATOMIC sum() const;

    /// Return the time this snapshot was made as returned by
    /// `bsls::TimeUtil::getTimer()`
    bsls::Types::Int64 snapshotTime() const;

    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// ================================
// class StatValue_SnapshotLocation
// ================================

/// A value-semantic type defining the location of a snapshot in a
/// StatValue.  The location of a snapshot is defined as an index into a
/// particular level of a StatValue.  Both the index and level must be
/// non-negative.
class StatValue_SnapshotLocation {
  private:
    // DATA
    int d_level;
    int d_index;

  public:
    // CREATORS

    /// Create a SnapshotLocation referring to the `0`th index of the `0`th
    /// level (the most current snapshot).
    StatValue_SnapshotLocation();

    /// Create a SnapshotLocation referring to the specified `index` of the
    /// `0`th level.
    StatValue_SnapshotLocation(int index);

    /// Create a SnapshotLocation referring to the specified `index` of the
    /// specified `level`.
    StatValue_SnapshotLocation(int level, int index);

    // MANIPULATORS

    /// Set the level of this `SnapshotLocation` to the specified `level`.
    StatValue_SnapshotLocation& setLevel(int level);

    /// Set the index of this `SnapshotLocation` to the specified `index`.
    StatValue_SnapshotLocation& setIndex(int index);

    // ACCESSORS

    /// Return the level.
    int level() const;

    /// Return the index.
    int index() const;

    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS
bool          operator==(const StatValue_SnapshotLocation& lhs,
                const StatValue_SnapshotLocation& rhs);
bool          operator<(const StatValue_SnapshotLocation& lhs,
               const StatValue_SnapshotLocation& rhs);
bool          operator>(const StatValue_SnapshotLocation& lhs,
               const StatValue_SnapshotLocation& rhs);
bsl::ostream& operator<<(bsl::ostream&                     stream,
                         const StatValue_SnapshotLocation& location);

// ===============
// class StatValue
// ===============

/// Statistic-collecting value
class StatValue {
  public:
    // PUBLIC TYPES
    typedef StatValue_Value<bsls::Types::Int64> Snapshot;
    typedef StatValue_SnapshotLocation          SnapshotLocation;

    enum Type {
        /// A continuous value logically represents a curve that is moved
        /// with `adjustValue` and `setValue`.  When adding two continuous
        /// StatValues, imagine them being stacked into a single continuous
        /// value.  For example the max of the added value will be the sum
        /// of the maxes of the values being added.
        e_CONTINUOUS = 0,

        /// A discrete value logically represents a number of discrete
        /// events reported with `reportEvent`.  When two discrete values
        /// are added, their set of reported events is simply considered as
        /// a single stream of events.  For example, the max of two added
        /// discrete values will be the max of all the individual maxes.
        e_DISCRETE = 1
    };

  private:
    // PRIVATE TYPES
    typedef StatValue_Value<bsls::AtomicInt64, bsls::Types::Int64>
        AtomicValueStats;

    // DATA
    Type d_type;

    AtomicValueStats d_currentStats;

    bsl::vector<Snapshot> d_history;  // snapshots

    bsl::vector<int> d_levelStartIndices;
    // i'th value is the first index
    // in 'd_history' for the i'th
    // aggregation level.  Last
    // element holds
    // 'd_history.size()' to simplify
    // code

    bsl::vector<int> d_curSnapshotIndices;
    // Last snapshot index for each
    // aggregation level

    bsls::Types::Int64 d_min;  // min value since creation

    bsls::Types::Int64 d_max;  // max value since creation

    // PRIVATE MANIPULATORS
    void updateMinMax(bsls::Types::Int64 value);

    /// Aggregate the specified aggregation `level` if there is an
    /// aggregation level above it using the specified `snapshotTime`
    void aggregateLevel(int level, bsls::Types::Int64 snapshotTime);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatValue, bslma::UsesBslmaAllocator)

    // CREATORS
    StatValue(bslma::Allocator* basicAllocator = 0);
    StatValue(const bsl::vector<int>& sizes,
              Type                    type,
              bsls::Types::Int64      initTime,
              bslma::Allocator*       basicAllocator = 0);
    StatValue(const StatValue& other, bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS
    StatValue& operator=(const StatValue& rhs);

    /// Adjust the value of this StatValue by the specified `delta`.  The
    /// behavior is undefined unless this is a continuous StatValue.
    void adjustValue(bsls::Types::Int64 delta);

    /// Set the value of this StatValue to the specified `value`.  The
    /// behavior is undefined unless this is a continuous StatValue.
    void setValue(bsls::Types::Int64 value);

    /// Report the specified `value` to this StatValue.  The behavior is
    /// undefined unless this is a discrete StatValue.
    void reportValue(bsls::Types::Int64 value);

    /// Add the snapshot of the specified `other` StatValue to the current
    /// value of this `StatValue`.
    void addSnapshot(const StatValue& other);

    /// Set the values of this `StatValue` from the field values within the
    /// specified `update`.  Note that any value changes made since the last
    /// snapshot may be lost.
    void setFromUpdate(const mwcstm::StatValueUpdate& update);

    void takeSnapshot(bsls::Types::Int64 snapshotTime);

    /// Clear all history and reset all snapshot's snapshotTime with the
    /// specified `clearTime`
    void clear(bsls::Types::Int64 clearTime);

    /// Reset current stats only, leaving history and last snapshot intact
    void clearCurrentStats();

    /// (Re)initialize this StatValue to be of the specified `type` with
    /// the specified history `sizes` using the specified `initTime` to
    /// initialize each snapshot's `snapshotTime`.  The current state is
    /// lost.
    void init(const bsl::vector<int>& sizes,
              Type                    type,
              bsls::Types::Int64      initTime);

    /// Sync this StatValue's snapshot schedule with that of the specified
    /// `other` StatValue.  This means that all level 1 and above snapshots
    /// will occur at the same time, and all snapshots will have the same
    /// snapshotTimes.  The behavior is undefined unless `takeSnapshot`
    /// hasn't been called since this StatValue was created, cleared, or
    /// inited and this `StatValue` and `other` both have the same history
    /// structure.
    void syncSnapshotSchedule(const StatValue& other);

    // ACCESSORS

    /// Return the type of this StatValue.
    Type type() const;

    /// Return the number of snapshot levels specified at construction.
    int numLevels() const;

    /// Return the history size specified at construction for the specified
    /// `level`.  This is the maximum `index` argument that can be passed
    /// to `snapshot` for this `level`.
    int historySize(int level) const;

    /// Return the snapshot referred to by the specified `location`.  The
    /// behavior is undefined unless
    /// `location.level() < numLevels()` and
    /// `location.index() < historySize(location.level())`
    const Snapshot& snapshot(const SnapshotLocation& location) const;

    /// Return the minimum value of this StatValue since creation.
    bsls::Types::Int64 min() const;

    /// Return the maximum value of this StatValue since creation.
    bsls::Types::Int64 max() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// ====================
// struct StatValueUtil
// ====================

/// This `struct` provides a namespace for a suite of functions that provide
/// non-primitive operations on a `StatValue`.
struct StatValueUtil {
  private:
    /// Load into the specified `update` the values of fields in the
    /// specified `value` that have a corresponding flag set in the
    /// specified `valueFieldMask`.  If the specified `fullUpdate` is true,
    /// `update` is initialized with the latest snapshot of `value`,
    /// otherwise only values that have changed between the last two
    /// snapshots are loaded.
    static void loadUpdateImp(mwcstm::StatValueUpdate* update,
                              const StatValue&         value,
                              bsl::uint32_t            valueFieldMask,
                              bool                     fullUpdate);

  public:
    /// Load into the specified `update` the values of fields in the
    /// specified `value` that have changed between the last two snapshots.
    /// If this value only records one snapshot, load all of the field
    /// values of that snapshot.  Optionally specify a `valueFieldMask` to
    /// control which attributes of this value are loaded into `update`.  If
    /// `valueFieldMask` is not specified, all attributes are saved.
    static void loadUpdate(mwcstm::StatValueUpdate* update,
                           const StatValue&         value,
                           int valueFieldMask = 0xFFFFFFFF);

    /// Load into the specified `update` the latest snapshotted values of
    /// all fields in the specified `value`.  Optionally specify a
    /// `valueFieldMask` to control which attributes of this value are
    /// loaded into `update`.  If `valueFieldMask` is not specified, all
    /// attributes are saved.
    static void loadFullUpdate(mwcstm::StatValueUpdate* update,
                               const StatValue&         value,
                               int valueFieldMask = 0xFFFFFFFF);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class StatValue_Value
// ---------------------

// CREATORS
template <typename T, typename NON_ATOMIC>
StatValue_Value<T, NON_ATOMIC>::StatValue_Value()
: d_value(0)
, d_min(0)
, d_max(0)
, d_incrementsOrEvents(0)
, d_decrementsOrSum(0)
, d_snapshotTime(0)
{
}

template <typename T, typename NON_ATOMIC>
StatValue_Value<T, NON_ATOMIC>::StatValue_Value(
    const StatValue_Value<T, NON_ATOMIC>& other)
: d_value(static_cast<NON_ATOMIC>(other.d_value))
, d_min(static_cast<NON_ATOMIC>(other.d_min))
, d_max(static_cast<NON_ATOMIC>(other.d_max))
, d_incrementsOrEvents(static_cast<NON_ATOMIC>(other.d_incrementsOrEvents))
, d_decrementsOrSum(static_cast<NON_ATOMIC>(other.d_decrementsOrSum))
, d_snapshotTime(static_cast<NON_ATOMIC>(other.d_snapshotTime))
{
}

// MANIPULATORS
template <typename T, typename NON_ATOMIC>
StatValue_Value<T, NON_ATOMIC>& StatValue_Value<T, NON_ATOMIC>::operator=(
    const StatValue_Value<T, NON_ATOMIC>& rhs)
{
    d_value              = static_cast<NON_ATOMIC>(rhs.d_value);
    d_min                = static_cast<NON_ATOMIC>(rhs.d_min);
    d_max                = static_cast<NON_ATOMIC>(rhs.d_max);
    d_incrementsOrEvents = static_cast<NON_ATOMIC>(rhs.d_incrementsOrEvents);
    d_decrementsOrSum    = static_cast<NON_ATOMIC>(rhs.d_decrementsOrSum);
    d_snapshotTime       = static_cast<NON_ATOMIC>(rhs.d_snapshotTime);

    return *this;
}

template <typename T, typename NON_ATOMIC>
void StatValue_Value<T, NON_ATOMIC>::reset(bool               extremeMinMax,
                                           bsls::Types::Int64 resetTime)
{
    d_value = 0;
    if (extremeMinMax) {
        d_min = bsl::numeric_limits<NON_ATOMIC>::max();
        d_max = bsl::numeric_limits<NON_ATOMIC>::min();
    }
    else {
        d_min = 0;
        d_max = 0;
    }
    d_incrementsOrEvents = 0;
    d_decrementsOrSum    = 0;
    d_snapshotTime       = resetTime;
}

// ACCESSORS
template <typename T, typename NON_ATOMIC>
inline NON_ATOMIC StatValue_Value<T, NON_ATOMIC>::min() const
{
    return d_min;
}

template <typename T, typename NON_ATOMIC>
inline NON_ATOMIC StatValue_Value<T, NON_ATOMIC>::max() const
{
    return d_max;
}

template <typename T, typename NON_ATOMIC>
inline NON_ATOMIC StatValue_Value<T, NON_ATOMIC>::value() const
{
    return d_value;
}

template <typename T, typename NON_ATOMIC>
inline NON_ATOMIC StatValue_Value<T, NON_ATOMIC>::increments() const
{
    return d_incrementsOrEvents;
}

template <typename T, typename NON_ATOMIC>
inline NON_ATOMIC StatValue_Value<T, NON_ATOMIC>::decrements() const
{
    return d_decrementsOrSum;
}

template <typename T, typename NON_ATOMIC>
inline NON_ATOMIC StatValue_Value<T, NON_ATOMIC>::events() const
{
    return d_incrementsOrEvents;
}

template <typename T, typename NON_ATOMIC>
inline NON_ATOMIC StatValue_Value<T, NON_ATOMIC>::sum() const
{
    return d_decrementsOrSum;
}

template <typename T, typename NON_ATOMIC>
inline bsls::Types::Int64 StatValue_Value<T, NON_ATOMIC>::snapshotTime() const
{
    return d_snapshotTime;
}

template <typename T, typename NON_ATOMIC>
bsl::ostream& StatValue_Value<T, NON_ATOMIC>::print(bsl::ostream& stream,
                                                    int           level,
                                                    int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("Value", static_cast<NON_ATOMIC>(d_value));
    printer.printAttribute("Min", static_cast<NON_ATOMIC>(d_min));
    printer.printAttribute("Max", static_cast<NON_ATOMIC>(d_max));
    printer.printAttribute("IncrementsOrEvents",
                           static_cast<NON_ATOMIC>(d_incrementsOrEvents));
    printer.printAttribute("DecrementsOrSum",
                           static_cast<NON_ATOMIC>(d_decrementsOrSum));
    printer.end();

    return stream;
}

// --------------------------------
// class StatValue_SnapshotLocation
// --------------------------------

// CREATORS
inline StatValue_SnapshotLocation::StatValue_SnapshotLocation()
: d_level(0)
, d_index(0)
{
}

inline StatValue_SnapshotLocation::StatValue_SnapshotLocation(int index)
: d_level(0)
, d_index(index)
{
}

inline StatValue_SnapshotLocation::StatValue_SnapshotLocation(int level,
                                                              int index)
: d_level(level)
, d_index(index)
{
}

// MANIPULATORS
inline StatValue_SnapshotLocation&
StatValue_SnapshotLocation::setLevel(int level)
{
    d_level = level;
    return *this;
}

inline StatValue_SnapshotLocation&
StatValue_SnapshotLocation::setIndex(int index)
{
    d_index = index;
    return *this;
}

// ACCESSORS
inline int StatValue_SnapshotLocation::level() const
{
    return d_level;
}

inline int StatValue_SnapshotLocation::index() const
{
    return d_index;
}

inline bsl::ostream&
StatValue_SnapshotLocation::print(bsl::ostream& stream,
                                  int           level,
                                  int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("level", d_level);
    printer.printAttribute("index", d_index);
    printer.end();

    return stream;
}

// ---------------
// class StatValue
// ---------------

// PRIVATE MANIPULATORS
inline void StatValue::updateMinMax(bsls::Types::Int64 value)
{
    bsls::Types::Int64 min = d_currentStats.d_min;
    while (min > value) {
        d_currentStats.d_min.testAndSwap(min, value);
        min = d_currentStats.d_min;
    }

    bsls::Types::Int64 max = d_currentStats.d_max;
    while (max < value) {
        d_currentStats.d_max.testAndSwap(max, value);
        max = d_currentStats.d_max;
    }
}

// MANIPULATORS
inline void StatValue::adjustValue(bsls::Types::Int64 delta)
{
    BSLS_ASSERT(d_type == e_CONTINUOUS);

    bsls::Types::Int64 newValue = (d_currentStats.d_value += delta);

    updateMinMax(newValue);

    if (delta > 0) {
        d_currentStats.d_incrementsOrEvents++;
    }
    else if (delta < 0) {
        d_currentStats.d_decrementsOrSum++;
    }
}

inline void StatValue::setValue(bsls::Types::Int64 value)
{
    BSLS_ASSERT(d_type == e_CONTINUOUS);

    bsls::Types::Int64 oldValue = d_currentStats.d_value.swap(value);
    updateMinMax(value);

    if (value > oldValue) {
        d_currentStats.d_incrementsOrEvents++;
    }
    else if (value < oldValue) {
        d_currentStats.d_decrementsOrSum++;
    }
}

inline void StatValue::reportValue(bsls::Types::Int64 value)
{
    BSLS_ASSERT(d_type == e_DISCRETE);

    d_currentStats.d_decrementsOrSum += value;
    d_currentStats.d_incrementsOrEvents++;

    updateMinMax(value);
}

inline void StatValue::clearCurrentStats()
{
    d_currentStats.reset(d_type == e_DISCRETE, 0);
}

// ACCESSORS
inline StatValue::Type StatValue::type() const
{
    return d_type;
}

inline int StatValue::numLevels() const
{
    return static_cast<int>(d_curSnapshotIndices.size());
}

inline int StatValue::historySize(int aggregationlevel) const
{
    BSLS_ASSERT(aggregationlevel <
                (static_cast<int>(d_levelStartIndices.size()) - 1));

    return d_levelStartIndices[aggregationlevel + 1] -
           d_levelStartIndices[aggregationlevel];
}

inline const StatValue::Snapshot&
StatValue::snapshot(const SnapshotLocation& location) const
{
    BSLS_ASSERT(location.level() < numLevels());
    BSLS_ASSERT(location.index() < historySize(location.level()));

    int snapshotIndex = d_curSnapshotIndices[location.level()];

    int historyIndex = snapshotIndex - location.index();
    if (historyIndex < 0) {
        historyIndex += historySize(location.level());
    }

    return d_history[historyIndex + d_levelStartIndices[location.level()]];
}

inline bsls::Types::Int64 StatValue::min() const
{
    return d_min;
}

inline bsls::Types::Int64 StatValue::max() const
{
    return d_max;
}

// --------------------
// struct StatValueUtil
// --------------------

inline void StatValueUtil::loadUpdate(mwcstm::StatValueUpdate* update,
                                      const StatValue&         value,
                                      int                      valueFieldMask)
{
    loadUpdateImp(update,
                  value,
                  static_cast<bsl::uint32_t>(valueFieldMask),
                  false);
}

inline void StatValueUtil::loadFullUpdate(mwcstm::StatValueUpdate* update,
                                          const StatValue&         value,
                                          int valueFieldMask)
{
    loadUpdateImp(update,
                  value,
                  static_cast<bsl::uint32_t>(valueFieldMask),
                  true);
}

}  // close package namespace

// FREE OPERATORS
inline bool mwcst::operator==(const StatValue_SnapshotLocation& lhs,
                              const StatValue_SnapshotLocation& rhs)
{
    return lhs.level() == rhs.level() && lhs.index() == rhs.index();
}

inline bool mwcst::operator<(const StatValue_SnapshotLocation& lhs,
                             const StatValue_SnapshotLocation& rhs)
{
    if (lhs.level() == rhs.level()) {
        return lhs.index() < rhs.index();
    }
    else {
        return lhs.level() < rhs.level();
    }
}

inline bool mwcst::operator>(const StatValue_SnapshotLocation& lhs,
                             const StatValue_SnapshotLocation& rhs)
{
    if (lhs.level() == rhs.level()) {
        return lhs.index() > rhs.index();
    }
    else {
        return lhs.level() > rhs.level();
    }
}

inline bsl::ostream&
mwcst::operator<<(bsl::ostream&                     stream,
                  const StatValue_SnapshotLocation& location)
{
    return location.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
