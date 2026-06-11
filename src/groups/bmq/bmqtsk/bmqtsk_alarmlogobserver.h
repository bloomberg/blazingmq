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

#ifndef INCLUDED_BMQTSK_ALARMLOGOBSERVER
#define INCLUDED_BMQTSK_ALARMLOGOBSERVER

//@PURPOSE: Provide a BALL observer for ALARMS with rate-controlled output.
//
//@CLASSES:
//  bmqtsk::AlarmLogObserver: BALL observer emitting alarms to stderr
//
//@DESCRIPTION: 'bmqtsk::AlarmLogObserver' implements the
// 'ball::ObserverAdapter' protocol to monitor ball logs.  If a log record
// matches specific criteria, it will be dumped to 'stderr' so that some
// external monitoring systems can trigger system alerts about those events.
// The criteria are set by the macros defined in 'bmqtsk_alarmlog'.
//
/// Thread-safety
///-------------
// This object is *thread* *enabled*, meaning that two threads can safely call
// any methods on the *same* *instance* without external synchronization.
//
/// Implementation Notes
///--------------------
// The implementation leverages the 'customFields' on a ball record: the
// 'BMQTSK_ALARMLOG_ALARM' and 'BMQTSK_ALARMLOG_PANIC' macros (defined in
// 'bmqtsk_alarmlog') create a 'ballRecord' and set its first customField to
// the corresponding string.
//
// The alarms are rate-controlled, by authorizing at most one alarm per source,
// the source being identified by the file and line producing the log.
//

// BDE
#include <ball_observeradapter.h>
#include <ball_record.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqtsk {

// ======================
// class AlarmLogObserver
// ======================

/// BALL observer emitting rate-controlled alarms to stderr
// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
class AlarmLogObserver : public ball::ObserverAdapter {
  private:
    // PRIVATE TYPES

    /// Map of log source identity to last alarm emitting time.
    typedef bsl::unordered_map<bsl::string, bsls::Types::Int64>
        CategoryTimeMap;

    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use, held not owned.

    CategoryTimeMap d_lastAlarmTime;
    // Map of last emit time of all encountered alarms.

    bslmt::Mutex d_mutex;
    // Mutex for thread-safety of this component.

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    AlarmLogObserver(const AlarmLogObserver&);             // = delete;
    AlarmLogObserver& operator=(const AlarmLogObserver&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AlarmLogObserver, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object, using the specified `allocator`.
    explicit AlarmLogObserver(bslma::Allocator* allocator);

    /// Destroy this object.
    ~AlarmLogObserver() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //  (virtual: ball::ObserverAdapter)

    /// Process the specified log `record` having the specified publishing
    /// `context`.  If `record` correspond to an alarm record, its message
    /// will be dumped to stderr with respects to rate-controlled logic.
    void publish(const ball::Record&  record,
                 const ball::Context& context) BSLS_KEYWORD_OVERRIDE;

    /// Note: this member is overridden to get rid of the "hides the virtual
    ///       function" warning.
    inline void publish(const bsl::shared_ptr<const ball::Record>& record,
                        const ball::Context& context) BSLS_KEYWORD_OVERRIDE
    {
        publish(*record, context);
    }
};
// NOLINTEND(cppcoreguidelines-special-member-functions)

}  // close package namespace
}  // close enterprise namespace

#endif
