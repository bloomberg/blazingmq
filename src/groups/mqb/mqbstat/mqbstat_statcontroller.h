// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbstat_statcontroller.h                                           -*-C++-*-
#ifndef INCLUDED_MQBSTAT_STATCONTROLLER
#define INCLUDED_MQBSTAT_STATCONTROLLER

//@PURPOSE: Provide a processor for statistics collected by the bmqbrkr.
//
//@CLASSES:
//  mqbstat::StatController: bmqbrkr statistics processor
//
//@DESCRIPTION: 'mqbstat::StatController' handles all the statistics.  It holds
// the top level StatContext, from which all subcontexts are created, and is
// responsible from calling snapshot on them as well as regularly (if enable in
// config) dumping the stats to a dedicated log file.

// MQB

#include <mqbcmd_messages.h>
#include <mqbstat_jsonprinter.h>
#include <mqbstat_printer.h>

#include <bmqma_countingallocatorstore.h>
#include <bmqst_statcontext.h>
#include <bmqsys_statmonitor.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_throttle.h>
#include <bdlmt_timereventscheduler.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace bslmt {
class Semaphore;
}
namespace mqbcmd {
class StatCommand;
}
namespace mqbcmd {
class StatResult;
}
namespace mqbplug {
class PluginManager;
}
namespace mqbplug {
class StatPublisher;
}
namespace mqbplug {
class StatConsumer;
}
namespace bmqst {
class StatContext;
}
namespace bmqst {
class Table;
}
namespace bmqu {
class BasicTableInfoProvider;
}

namespace mqbstat {

// ====================
// class StatController
// ====================

class StatController {
  public:
    // PUBLIC TYPES

    /// Enum representing the available types of stat context selections for
    /// the `channels` stats.
    struct ChannelSelector {
        // TYPES
        enum Enum { e_ALL, e_LOCAL, e_REMOTE };
    };

    /// Signature of a method for processing the command in the specified
    /// `cmd` coming from the specified `source`, and writing the result of
    /// the command in the specified `os`.
    typedef bsl::function<int(const bslstl::StringRef& source,
                              const bsl::string&       cmd,
                              bsl::ostream&            os)>
        CommandProcessorFn;

    /// Map of StatContext names to StatContext pointers
    typedef bsl::unordered_map<bsl::string, bmqst::StatContext*> StatContexts;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSTAT.STATCONTROLLER");

  private:
    // PRIVATE TYPES
    typedef bslma::ManagedPtr<bdlmt::TimerEventScheduler> SchedulerMp;
    typedef bslma::ManagedPtr<bmqst::StatContext>         StatContextMp;
    typedef bsl::shared_ptr<bmqst::StatContext>           StatContextSp;
    typedef bslma::ManagedPtr<bmqsys::StatMonitor>        SystemStatMonitorMp;
    typedef bslma::ManagedPtr<Printer>                    PrinterMp;
    typedef bslma::ManagedPtr<JsonPrinter>                JsonPrinterMp;
    typedef bslma::ManagedPtr<mqbplug::StatPublisher>     StatPublisherMp;
    typedef bslma::ManagedPtr<mqbplug::StatConsumer>      StatConsumerMp;

    /// Struct containing a statcontext and bool specifying if the
    /// statcontext is managed.
    struct StatContextDetails {
        StatContextSp d_statContext_sp;  // Stat Context

        bool d_managed;  // Bool to specify a managed
                         // statcontext. A managed statcontext
                         // is never snapshotted.

        StatContextDetails();

        StatContextDetails(const StatContextSp& statContext, bool managed);

        StatContextDetails(const StatContextDetails& rhs,
                           bslma::Allocator*         allocator_p = 0);
    };

    /// Map of StatContext name to StatContextDetail object
    typedef bsl::unordered_map<bsl::string, StatContextDetails>
        StatContextDetailsMap;

    // DATA
    /// Allocator store to spawn new allocators
    /// for sub-components.
    bmqma::CountingAllocatorStore d_allocators;

    /// This component should use it's own
    /// scheduler to not have stats interfere
    /// with critical other parts.
    SchedulerMp d_scheduler_mp;

    /// Time at which snapshot was last called.
    bsls::Types::Int64 d_lastSnapshotTime;

    /// Throttler for safeguarding against too often snapshot invocations.
    /// Note that snapshots are done periodically via EventScheduler, but
    /// also it is possible to make urgent out-of-order snapshot with
    /// admin command.
    bdlmt::Throttle d_snapshotThrottle;

    /// Stat context of the counting allocators,
    /// if used.
    bmqst::StatContext* d_allocatorsStatContext_p;

    /// Map holding all the stat contexts
    StatContextDetailsMap d_statContextsMap;

    /// 'local' child stat context of the
    /// 'channels' stat context
    StatContextMp d_statContextChannelsLocal_mp;

    /// 'remote' child stat context of the
    /// 'channels' stat context
    StatContextMp d_statContextChannelsRemote_mp;

    /// System stat monitor (for cpu and
    /// memory).
    SystemStatMonitorMp d_systemStatMonitor_mp;

    /// Used to instantiate 'StatConsumer'
    /// plugins at start-time.
    mqbplug::PluginManager* d_pluginManager_p;

    /// Buffer factory for a StatsProvider if
    /// provided as a plugin.
    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    /// Function to invoke when receiving a command
    /// from a command processor plugin.
    CommandProcessorFn d_commandProcessorFn;

    /// Console and log file stats printer
    PrinterMp d_printer_mp;

    /// JsonPrinter used for admin commands processing
    JsonPrinterMp d_jsonPrinter_mp;

    /// Registered stat consumers
    bsl::vector<StatConsumerMp> d_statConsumers;

    /// StatConsumer max publish interval
    int d_statConsumerMaxPublishInterval;

    /// Event scheduler passed in from application
    bdlmt::EventScheduler* d_eventScheduler_p;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE MANIPULATORS

    /// Initialize all the stat contexts and associated Tables and TIPs.
    void initializeStats();

    /// Capture the stats to the specified `result`' object and post on the
    /// specified `semaphore` once done.  The specified `encoding` parameter
    /// controls whether the result should be in a text format or in a JSON.
    /// Note that the JSON format is typically used within integration tests
    /// so we make an out of order stats snapshot when compact or pretty JSON
    /// encoding is passed.
    void captureStatsAndSemaphorePost(
        mqbcmd::StatResult*                  result,
        bslmt::Semaphore*                    semaphore,
        const mqbcmd::EncodingFormat::Value& encoding);

    /// Process specified `tunable` subcommand and load the result into the
    /// specified `result` and post on the optionally specified `semaphore`
    /// once done.
    void setTunable(mqbcmd::StatResult*       result,
                    const mqbcmd::SetTunable& tunable,
                    bslmt::Semaphore*         semaphore = 0);

    /// Get the value of specified `tunable` parameter and load the result
    /// into the specified `result`. Post on the optionally specified
    /// `semaphore` once done.
    void getTunable(mqbcmd::StatResult* result,
                    const bsl::string&  tunable,
                    bslmt::Semaphore*   semaphore = 0);

    /// Get the list of available parameters and load the result into the
    /// specified `result` and post on the optionally specified `semaphore`
    /// once done.
    void listTunables(mqbcmd::StatResult* result,
                      bslmt::Semaphore*   semaphore = 0);

    /// Try to snapshot the stats.
    /// Return `true` upon success and `false` otherwise.  The attempt to make
    /// a snapshot might fail if we try to call `snapshot()` too often.
    bool snapshot();

    /// Try to snapshot the stats and notify all the registered stat consumers
    /// upon success.
    void snapshotAndNotify();

    // PRIVATE ACCESSORS

    /// Validate that the statistics parameter from the config are valid,
    /// and return 0 on success; or a non-zero value on error and fill the
    /// specified `errorDescription` stream with the description of the
    /// error.
    int validateConfig(bsl::ostream& errorDescription) const;

  private:
    // NOT IMPLEMENTED
    StatController(const StatController& other) BSLS_CPP11_DELETED;
    StatController& operator=(const StatController& other) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatController, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `StatController` object, using the specified
    /// `commandProcessor`, `pluginManager`, `bufferFactory`,
    /// `allocatorsStatContext`, `eventScheduler` and the specified
    /// `allocator` for memory allocation.
    StatController(const CommandProcessorFn& commandProcessor,
                   mqbplug::PluginManager*   pluginManager,
                   bdlbb::BlobBufferFactory* bufferFactory,
                   bmqst::StatContext*       allocatorsStatContext,
                   bdlmt::EventScheduler*    eventScheduler,
                   bslma::Allocator*         allocator);

    // MANIPULATORS

    /// Start the StatController.  Return 0 on success, or a non-zero return
    /// code on error and fill in the specified `errorDescription` stream
    /// with the description of the error.
    int start(bsl::ostream& errorDescription);

    /// Stop the statController.
    void stop();

    /// Load into the specified `contexts` all root top level stat contexts
    /// (allocators, systems, domainQueues, clients, ...).
    void loadStatContexts(StatContexts* contexts);

    /// Process the specified `command`, and write the result to the `result`
    /// object.  Return zero on success or a nonzero value otherwise.
    /// The specified `encoding` parameter controls whether the result should
    /// be in a text format or in a JSON.
    int processCommand(mqbcmd::StatResult*                  result,
                       const mqbcmd::StatCommand&           command,
                       const mqbcmd::EncodingFormat::Value& encoding);

    /// Retrieve the domains top-level stat context.
    bmqst::StatContext* domainsStatContext();

    /// Retrieve the domainQueues top-level stat context.
    bmqst::StatContext* domainQueuesStatContext();

    /// Retrieve the clients top-level stat context.
    bmqst::StatContext* clientsStatContext();

    /// Retrieve the clusterNodes top-level stat context.
    bmqst::StatContext* clusterNodesStatContext();

    /// Retrieve the clusters top-level stat context.
    bmqst::StatContext* clustersStatContext();

    /// Retrieve the channels stat context corresponding to the specified
    /// `selector`.
    bmqst::StatContext* channelsStatContext(ChannelSelector::Enum selector);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// class StatController
// --------------------

inline bmqst::StatContext* StatController::domainsStatContext()
{
    return d_statContextsMap["domains"].d_statContext_sp.get();
}

inline bmqst::StatContext* StatController::domainQueuesStatContext()
{
    return d_statContextsMap["domainQueues"].d_statContext_sp.get();
}

inline bmqst::StatContext* StatController::clientsStatContext()
{
    return d_statContextsMap["clients"].d_statContext_sp.get();
}

inline bmqst::StatContext* StatController::clusterNodesStatContext()
{
    return d_statContextsMap["clusterNodes"].d_statContext_sp.get();
}

inline bmqst::StatContext* StatController::clustersStatContext()
{
    return d_statContextsMap["clusters"].d_statContext_sp.get();
}

inline bmqst::StatContext*
StatController::channelsStatContext(ChannelSelector::Enum selector)
{
    switch (selector) {
    case ChannelSelector::e_ALL:
        return d_statContextsMap["channels"].d_statContext_sp.get();
    case ChannelSelector::e_LOCAL: return d_statContextChannelsLocal_mp.get();
    case ChannelSelector::e_REMOTE:
        return d_statContextChannelsRemote_mp.get();
    default: BSLS_ASSERT_OPT(false && "unknown channel selector");
    }

    return 0;  // compiler happiness
}

}  // close package namespace
}  // close enterprise namespace

#endif
