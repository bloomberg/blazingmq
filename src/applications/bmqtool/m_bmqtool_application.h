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
#include <m_bmqtool_storageinspector.h>

// MQB
#include <mqbs_journalfileiterator.h>

// BMQ
#include <bmqa_messageeventbuilder.h>
#include <bmqa_queueid.h>
#include <bmqa_session.h>

// MWC
#include <mwcst_statcontext.h>
#include <mwctsk_consoleobserver.h>

// BDE
#include <ball_multiplexobserver.h>
#include <bdlbb_blob.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_list.h>
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
    // TYPES
    typedef bslma::ManagedPtr<mwcst::StatContext> StatContextMP;

    /// This structure holds context created by `bmqa::Session`.
    /// It must be destructed before the `d_session_mp`.
    struct SessionContext {
        bmqa::QueueId d_queueId;

        bmqa::MessageEventBuilder d_eventBuilder;
        // Message event builder.

        SessionContext(bslma::Allocator* d_allocator);
    };

    // DATA
    bslma::Allocator* d_allocator_p;
    // Held, not owned

    Parameters* d_parameters_p;
    // Command-line parameters.  Held, not
    // owned

    bslmt::Semaphore* d_shutdownSemaphore_p;
    // Semaphore holding the main thread
    // alive

    bslmt::ThreadUtil::Handle d_runningThread;
    // Handle on the running thread
    // (producer mode)

    StatContextMP d_statContext_mp;
    // StatContext for msg/event stats

    bdlmt::EventScheduler d_scheduler;
    // Used to schedule stat snapshots

    bool d_isConnected;
    // Are we connected to bmqbrkr?  (to
    // know whether to dump stats/publish
    // message or not)

    bsls::AtomicBool d_isRunning;
    // Are we running ? To know when to stop
    // the processing thread

    ball::MultiplexObserver d_multiplexObserver;

    mwctsk::ConsoleObserver d_consoleObserver;

    bdlbb::PooledBlobBufferFactory d_bufferFactory;
    // Buffer factory for the payload of the
    // published message

    bdlbb::PooledBlobBufferFactory d_timeBufferFactory;
    // Small buffer factory for the first
    // blob of the published message, to
    // hold the timestamp information

    bdlbb::Blob d_blob;
    // Blob to post

    bslma::ManagedPtr<SessionContext> d_sessionContext_mp;

    bslma::ManagedPtr<bmqa::Session> d_session_mp;
    // Session with the BlazingMQ broker.

    int d_msgUntilNextTimestamp;
    // Number of messages remaining to send
    // until stamping one with latency.

    Interactive d_interactive;

    StorageInspector d_storageInspector;

    FileLogger d_fileLogger;
    // Logger to use in case events logging
    // to file has been enabled.

    bsl::list<bsls::Types::Int64> d_latencies;
    // List of all message latencies (in
    // ns).  Only populated when requested
    // to generate a latency report (with
    // --latency-report).

    bsls::AtomicBool d_autoReadInProgress;
    // Auto-consume mode only.  True if a
    // message has already been seen.

    bsls::AtomicBool d_autoReadActivity;
    // Auto-consume mode only.  True if a
    // message was seen during the current
    // grace period.

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

    /// Generate the latency report.
    void generateLatencyReport();

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
    Application(m_bmqtool::Parameters* parameters,
                bslmt::Semaphore*      shutdownSemaphore,
                bslma::Allocator*      allocator);

    ~Application() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    int start();

    int run();

    void stop();
};

}  // close package namespace
}  // close enterprise namespace

#endif
