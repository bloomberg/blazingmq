// Copyright 2014-2023 Bloomberg Finance L.P.
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

// m_bmqtool_application.h                                            -*-C++-*-
#ifndef INCLUDED_M_BMQTOOL_APPLICATION
#define INCLUDED_M_BMQTOOL_APPLICATION

//@PURPOSE: Provide the main class for the 'bmqtool' application.
//
//@CLASSES:
//  m_bmqtool::Application: the class.
//
//@DESCRIPTION: This component provides the main class for the 'bmqtool'
// application.

// BMQTOOL
#include <m_bmqtool_filelogger.h>
#include <m_bmqtool_interactive.h>
#include <m_bmqtool_messages.h>
#include <m_bmqtool_poster.h>
#include <m_bmqtool_storageinspector.h>

// MQB
#include <mqbs_journalfileiterator.h>

// BMQ
#include <bmqa_messageeventbuilder.h>
#include <bmqa_queueid.h>
#include <bmqa_session.h>

// BMQ
#include <bmqst_statcontext.h>
#include <bmqtsk_consoleobserver.h>

// BDE
#include <ball_multiplexobserver.h>
#include <bdlbb_blob.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_throttle.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmt_threadutil.h>
#include <bsls_atomic.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATIONS
namespace bslmt {
class Semaphore;
}
namespace m_bmqtool {
class Parameters;
}

namespace m_bmqtool {

// =================
// class Application
// =================

/// Main application class for `bmqtool`.
class Application : public bmqa::SessionEventHandler {
  private:
    // CLASS METHODS
    static bsl::shared_ptr<bmqst::StatContext>
    createStatContext(int historySize, bslma::Allocator* allocator);

    // DATA
    bslma::Allocator* d_allocator_p;
    // Held, not owned

    /// Run parameters
    /// Copy is made to ensure lifetime of the Parameters object used by this
    /// Application
    const Parameters d_parameters;

    bslmt::Semaphore* d_shutdownSemaphore_p;
    // Semaphore holding the main thread
    // alive

    bslmt::ThreadUtil::Handle d_runningThread;
    // Handle on the running thread
    // (producer mode)

    bmqa::QueueId d_queueId;
    // Queue to send/receive messages

    bsl::shared_ptr<bmqst::StatContext> d_statContext_sp;
    // StatContext for msg/event stats

    bdlmt::EventScheduler d_scheduler;
    // Used to schedule stat snapshots

    bsls::AtomicBool d_isConnected;
    // Are we connected to bmqbrkr?  (to
    // know whether to dump stats/publish
    // message or not)

    bsls::AtomicBool d_isRunning;
    // Are we running ? To know when to stop
    // the processing thread

    ball::MultiplexObserver d_multiplexObserver;

    bmqtsk::ConsoleObserver d_consoleObserver;

    bslma::ManagedPtr<bmqa::Session> d_session_mp;
    // Session with the BlazingMQ broker.

    StorageInspector d_storageInspector;

    FileLogger d_fileLogger;
    // Logger to use in case events logging
    // to file has been enabled.

    Poster d_poster;
    // A factory for posting series of messages.

    Interactive d_interactive;
    // CLI handler.

    /// A throttle object used to control confirm latency logging on a consumer
    bdlmt::Throttle d_confirmLatencyThrottle;

    /// List of all confirm message latencies (in ns).
    /// Confirm message latency is the end-to-end time to deliver a message,
    /// starting from producer post and ending on a consumer.
    /// Only populated when requested to generate a latency report (with
    /// --latency-report).
    bsl::list<bsls::Types::Int64> d_confirmLatencies;

    /// A throttle object used to control ack latency logging on a consumer
    bdlmt::Throttle d_ackLatencyThrottle;

    /// List of all ack message latencies (in ns).
    /// Ack message latency is the time between posting a message and getting
    /// an ACK for it, meaning that the message was at least replicated with
    /// a needed quorum (delivery might not have happened yet).
    /// Only populated when requested to generate a latency report (with
    /// --latency-report).
    bsl::list<bsls::Types::Int64> d_ackLatencies;

    bsls::AtomicBool d_autoReadInProgress;
    // Auto-consume mode only.  True if a
    // message has already been seen.

    bsls::AtomicBool d_autoReadActivity;
    // Auto-consume mode only.  True if a
    // message was seen during the current
    // grace period.

    bsl::uint64_t d_numExpectedAcks;
    // Auto-produce mode only. The total number of messages
    // the tool will send. After posting is finished
    // the tool will be waiting for this number of ACK
    // messages, after which the shutdown semaphore will
    // be posted.

    bsl::uint64_t d_numAcknowledged;
    // Auto-produce mode only. The number of acknowledged
    // messages. When the value of this field becomes equal
    // to d_numExpectedAcks, the shutdown semaphore will be
    // posted.

    // PRIVATE MANIPULATORS
    //   (virtual: bmqa::SessionEventHandler)

    /// Process the specified session `event` (connected, disconnected,
    /// queue opened, queue closed, etc.).
    void onSessionEvent(const bmqa::SessionEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified message `event` containing one or more
    /// messages.
    void onMessageEvent(const bmqa::MessageEvent& event) BSLS_KEYWORD_OVERRIDE;

    // PRIVATE MANIPULATORS
    void setUpLog();

    void tearDownLog();

    /// Initialize the statContext, using the specified `historySize` to
    /// configure each stat Value
    void initializeStatContext(int historySize);

    /// Snapshot the stats.
    void snapshotStats();

    /// Check if no message was received within the grace period in auto
    /// read mode.
    void autoReadShutdown();

    /// Print column headers for stats to the standard output, if it was not
    /// already printed.
    void printStatHeader() const;

    /// Print one new line with the stats corresponding to the specified
    /// `interval` time elapsed to the standard output.
    void printStats(int interval) const;

    /// Print the final stats to the standard output, at exit time.
    void printFinalStats();

    /// Generate the latency report from the specified `latencies`.
    /// The specified `name` represents the origin of the latencies, it is
    /// either end-to-end latency (producer->consumer) or ack latency
    /// (producer->cluster->producer-ack).
    void generateLatencyReport(const bsl::list<bsls::Types::Int64>& latencies,
                               const bslstl::StringRef&             name);

    /// Do any `pre` run initialization, such as connecting to bmqbrkr,
    /// opening a queue, preparing the blob to publish, ...  Return 0 on
    /// success.
    int initialize();

    /// Thread to process the publish.
    void producerThread();

  public:
    // CLASS METHODS

    /// Check if we can connect to the specified `broker`.  Used by sysqc
    /// script(s).  Returns 0 on success or non-zero number on failure.
    static int syschk(const m_bmqtool::Parameters& parameters);

    // CREATORS

    /// Constructor
    Application(const m_bmqtool::Parameters& parameters,
                bslmt::Semaphore*            shutdownSemaphore,
                bslma::Allocator*            allocator);

    ~Application() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    int start();

    int run();

    void stop();
};

}  // close package namespace
}  // close enterprise namespace

#endif
