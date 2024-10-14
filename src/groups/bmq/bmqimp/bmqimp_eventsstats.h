// Copyright 2015-2023 Bloomberg Finance L.P.
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

// bmqimp_eventsstats.h                                               -*-C++-*-
#ifndef INCLUDED_BMQIMP_EVENTSSTATS
#define INCLUDED_BMQIMP_EVENTSSTATS

//@PURPOSE: Provide a mechanism to keep track of Events statistics.
//
//@CLASSES:
//  bmqimp::EventsStatsEventType: Enum of the various events types
//  bmqimp::EventsStats         : Events statistics manipulator
//
//@DESCRIPTION: 'bmqimp::EventsStats' allows to keep track of statistics for
// the various events, represented by the 'bmqimp::EventsStatsEventType'.
//
/// Usage
///-----
// The 'initializeStats' method must be called once on the EventsStats, before
// being able to report statistics update using the 'onEvent' method.

// BMQ

#include <bmqimp_stat.h>

#include <bmqst_statcontext.h>
#include <bmqst_statvalue.h>

// BDE
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace bmqimp {

// ==========================
// struct EventsStatsEvenType
// ==========================

/// Enum representing the various types of Events being tracked.
///
/// NOTE: This enum is voluntarily a `fork` of `bmqp::EventType` because the
///       latter contains more events with the Broker<->Broker specific
///       events.  If bmqp::EventType is updated, this enumeration must also
///       be updated.
struct EventsStatsEventType {
    // TYPES
    enum Enum {
        e_ACK     = 0,
        e_CONFIRM = 1,
        e_CONTROL = 2,
        e_PUSH    = 3,
        e_PUT     = 4,
        e_LAST    = 5  // enum item count
    };

    // MANIPULATORS

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(EventsStatsEventType::Enum value);
};

// =================
// class EventsStats
// =================

/// Events statistics manipulator
class EventsStats {
  private:
    // PRIVATE TYPES
    typedef bslma::ManagedPtr<bmqst::StatContext> StatContextMp;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    Stat d_stat;
    // Stat holder

    StatContextMp d_statContexts_mp[EventsStatsEventType::e_LAST];
    // SubContext for each of the various event's type

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented
    EventsStats(const EventsStats& other, bslma::Allocator* allocator = 0);
    EventsStats& operator=(const EventsStats& other);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(EventsStats, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `EventsStats`, using the specified `allocator`.
    explicit EventsStats(bslma::Allocator* allocator);

    // MANIPULATORS

    /// Create the various stat context, table and tips for stats related to
    /// the events.  This will create a subcontext of the specified
    /// `rootStatContext`, and one subcontext for each of the EventsTypes.
    /// The `stat` contains delta and noDelta version of stats.  Delta ones
    /// correspond to stats between the specified `start` and `end` snapshot
    /// location.  The behavior is undefined is this method is called more
    /// than once on the same `EventsStats` object.
    void initializeStats(bmqst::StatContext* rootStatContext,
                         const bmqst::StatValue::SnapshotLocation& start,
                         const bmqst::StatValue::SnapshotLocation& end);

    /// Reset all statistics (used when restarting the session).
    void resetStats();

    /// Update stats associated to the event of the specified `type` to
    /// indicate that one event containing the specified `messageCount`
    /// messages with a total bytes size of the specified `eventSize`.  The
    /// behavior is undefined unless `initializeStats` has been called on
    /// the object prior to `onEvent`.
    void
    onEvent(EventsStatsEventType::Enum type, int eventSize, int messageCount);

    // ACCESSORS

    /// Print the stats to the specified `stream`; print the `delta` stats
    /// column if the specified `includeDelta` is true.
    void printStats(bsl::ostream& stream, bool includeDelta) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class EventsStats
// -----------------

inline void EventsStats::printStats(bsl::ostream& stream,
                                    bool          includeDelta) const
{
    d_stat.printStats(stream, includeDelta);
}

}  // close package namespace
}  // close enterprise namespace

#endif
