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

// m_bmqbrkr_task.cpp                                                 -*-C++-*-
#include <m_bmqbrkr_task.h>

// MQB
#include <mqbu_exit.h>

// MWC
#include <mwcma_countingallocator.h>
#include <mwcma_countingallocatorutil.h>
#include <mwcst_statcontext.h>
#include <mwcsys_threadutil.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlb_string.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_pipeutil.h>
#include <bsl_c_stdlib.h>
#include <bsl_cstddef.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bslma_default.h>
#include <bslma_newdeleteallocator.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace m_bmqbrkr {

namespace {
const char k_MTRAP_SET_THREADNAME[] = "__setThreadName";

/// Invoked by the top level CountingAllocator when its cumulated allocation
/// has crossed the configured specified `limit`.
void onAllocationLimit(bsls::Types::Uint64 limit)
{
    // This function will execute in 'arbitrary' thread (whichever one
    // triggered the allocation to go beyond the configured limit).  Inside
    // here, we explicitly snapshot and query the allocators stat context,
    // which is not thread-safe unless invoked from the stat context's snapshot
    // scheduler thread.  However, we don't have an (easy) access to that
    // scheduler from here, and anyway the broker is going down due to
    // unexpected high memory allocation; therefore we recognize but accept the
    // potential 'non-thread-safetyness' of stat context manipulation happening
    // here.

    bdlma::LocalSequentialAllocator<2048> localAllocator;
    mwcu::MemOutStream                    os(&localAllocator);

    os << "Memory allocation limit of " << mwcu::PrintUtil::prettyBytes(limit)
       << " has been reached.";

    mwcst::StatContext* sc = mwcma::CountingAllocatorUtil::globalStatContext();
    sc->snapshot();  // Snapshot to ensure we'll print the latest values

    // The 'sc' is the top stat context, we need access to its only child,
    // corresponding to the top level CountingAllocator in order to print it.
    BSLS_ASSERT_SAFE(sc->numSubcontexts() == 1);
    const mwcst::StatContext& context = *(sc->subcontextIterator());

    // Print all allocations from this top allocator and its children
    mwcma::CountingAllocatorUtil::printAllocations(os, context);

    os << "\nThe broker will now gracefully shutdown!";

    // Log a PANIC alarm
    BALL_LOG_SET_CATEGORY("BMQBRKR.TASK");
    MWCTSK_ALARMLOG_PANIC("MEMORY_LIMIT") << os.str() << MWCTSK_ALARMLOG_END;

    // Initiate a graceful shutdown of the broker
    mqbu::ExitUtil::shutdown(mqbu::ExitCode::e_MEMORY_LIMIT);
}

}  // close unnamed namespace

// ---------------------------
// class Task_AllocatorManager
// ---------------------------

Task_AllocatorManager::Task_AllocatorManager(mqbcfg::AllocatorType::Value type)
: d_type(type)
, d_store()
, d_stackTraceTestAllocator(bslma::Default::allocator(0))
, d_store_p(0)
, d_statContext_p(0)
{
    switch (d_type) {
    case mqbcfg::AllocatorType::NEWDELETE: {
        d_statContext_p = 0;

        new (d_store.buffer()) mwcma::CountingAllocatorStore(
            &(bslma::NewDeleteAllocator::singleton()));
        d_store_p = reinterpret_cast<mwcma::CountingAllocatorStore*>(
            d_store.buffer());

        bsl::cout << "\n"
                  << "   ##############################\n"
                  << "   # USING NEW/DELETE ALLOCATOR #\n"
                  << "   ##############################\n"
                  << "\n"
                  << bsl::flush;
    } break;
    case mqbcfg::AllocatorType::STACKTRACETEST: {
        d_statContext_p = 0;

        new (d_store.buffer())
            mwcma::CountingAllocatorStore(&d_stackTraceTestAllocator);
        d_store_p = reinterpret_cast<mwcma::CountingAllocatorStore*>(
            d_store.buffer());
        d_stackTraceTestAllocator.setName("BMQBRKR");
        d_stackTraceTestAllocator.setFailureHandler(
            &balst::StackTraceTestAllocator::failAbort);

        bsl::cerr << "\n"
                  << "   ####################################\n"
                  << "   # USING STACK TRACE TEST ALLOCATOR #\n"
                  << "   ####################################\n"
                  << "\n"
                  << bsl::flush;
    } break;
    case mqbcfg::AllocatorType::COUNTING: {
        mwcma::CountingAllocatorUtil::initGlobalAllocators(
            mwcst::StatContextConfiguration("task"),
            "allocators");

        d_statContext_p = mwcma::CountingAllocatorUtil::globalStatContext();
        d_store_p       = &mwcma::CountingAllocatorUtil::topAllocatorStore();
    } break;
    default: {
        bsl::cerr << "PANIC [STARTUP] Invalid allocator type '" << type << "'"
                  << "\n"
                  << bsl::flush;
        BSLS_ASSERT_OPT(false && "Unknown allocator type");
    }
    }
}

Task_AllocatorManager::~Task_AllocatorManager()
{
    if (d_type == mqbcfg::AllocatorType::NEWDELETE) {
        // Properly destroy the object buffer object.
        d_store.object()
            .mwcma::CountingAllocatorStore ::~CountingAllocatorStore();
    }
    else if (d_type == mqbcfg::AllocatorType::STACKTRACETEST) {
        // Ensure no memory leak
        const bsl::size_t blocksInUse =
            d_stackTraceTestAllocator.numBlocksInUse();
        if (blocksInUse > 0) {
            bsl::cout << "\n"
                      << "******************************\n"
                      << "/!\\ Memory leak detected: " << blocksInUse
                      << " blocks in use\n"
                      << bsl::flush;
            // No need to report blocks in use (i.e printing their stacks), it
            // will be done automatically when the allocator goes out of scope.
        }
        else {
            bsl::cout << "Test allocator OK.\n" << bsl::flush;
        }

        // Properly destroy the object buffer object.
        d_store.object()
            .mwcma::CountingAllocatorStore ::~CountingAllocatorStore();
    }
}

// ----------
// class Task
// ----------

int Task::onControlMessage(const bsl::string& message)
{
    // executes on the PIPE CONTROL CHANNEL thread

    // Intercept the M-Trap to set the name of this PIPE CONTROL CHANNEL thread
    if (bdlb::String::areEqualCaseless(message, k_MTRAP_SET_THREADNAME)) {
        mwcsys::ThreadUtil::setCurrentThreadName("bmqPipeCtrl");
        return 0;  // RETURN
    }

    const int rc = d_controlManager.dispatchMessage(message);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to dispatch M-Trap message '" << message
                       << "' [rc: " << rc << "]";
    }

    return rc;
}

void Task::onLogCommand(const bsl::string& prefix, bsl::istream& input)
{
    // executes on the PIPE CONTROL CHANNEL thread

    bsl::string        cmd;
    mwcu::MemOutStream ss;

    bsl::getline(input, cmd);
    cmd.erase(0, 1);  // cmd starts by a space, remove it

    const int rc = d_logController.processCommand(cmd, ss);
    if (rc != 0) {
        BALL_LOG_ERROR << "Error processing command '" << prefix << " " << cmd
                       << "' [rc: " << rc << "]:\n"
                       << ss.str();
    }
    else {
        BALL_LOG_INFO << "Command '" << prefix << " " << cmd
                      << "' successfully processed:\n"
                      << ss.str();
    }
}

Task::Task(const bsl::string& bmqPrefix, const mqbcfg::TaskConfig& config)
: d_allocatorManager(config.allocatorType())
, d_config(config)
, d_isInitialized(false)
, d_bmqPrefix(bmqPrefix, d_allocatorManager.store().get("Task"))
, d_scheduler(bsls::SystemClockType::e_MONOTONIC,
              d_allocatorManager.store().get("EventScheduler"))
, d_logController(&d_scheduler, d_allocatorManager.store().get("Task"))
, d_controlManager(d_allocatorManager.store().get("Task"))
, d_controlChannel(bdlf::BindUtil::bind(&Task::onControlMessage,
                                        this,
                                        bdlf::PlaceHolders::_1),  // message
                   d_allocatorManager.store().get("Task"))
{
    // NOTHING
}

Task::~Task()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isInitialized &&
                    "shutdown() must be called before destroying this object");
}

int Task::initialize(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isInitialized &&
                    "initialize() can only be called once on this object");

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                     = 0,
        rc_SCHEDULER_START_FAILED      = -1,
        rc_LOGCONTROLLER_CONFIG_FAILED = -2,
        rc_LOGCONTROLLER_INIT_FAILED   = -3,
        rc_CONTROLCHANNEL_START_FAILED = -4
    };

    int                rc = rc_SUCCESS;
    mwcu::MemOutStream localError;

    // ---------
    // Scheduler
    rc = d_scheduler.start();
    if (rc != 0) {
        errorDescription << "failed to start scheduler [rc: " << rc << "]";
        return rc_SCHEDULER_START_FAILED;  // RETURN
    }

    if (mwcsys::ThreadUtil::k_SUPPORT_THREAD_NAME) {
        d_scheduler.scheduleEvent(
            bsls::TimeInterval(0, 0),  // now
            bdlf::BindUtil::bind(&mwcsys::ThreadUtil::setCurrentThreadName,
                                 "bmqSchedTask"));
    }

    // -------------
    // LogController
    mwctsk::LogControllerConfig logConfig;
    rc = logConfig.fromObj(localError, d_config.logController());
    if (rc != 0) {
        d_scheduler.stop();
        errorDescription << "failed to initialize LogControllerConfig from "
                         << "config (error: '" << localError.str() << "')";
        return (rc * 10) + rc_LOGCONTROLLER_CONFIG_FAILED;  // RETURN
    }

    rc = d_logController.initialize(localError, logConfig);
    if (rc != 0) {
        d_scheduler.stop();
        errorDescription << "failed to initialize LogController (error: '"
                         << localError.str() << "')";
        return (rc * 10) + rc_LOGCONTROLLER_INIT_FAILED;  // RETURN
    }

    // ----------------
    // Allocation limit
    // Set allocation limit if enabled and using counting allocator
    if (d_allocatorManager.type() == mqbcfg::AllocatorType::COUNTING) {
        if (d_config.allocationLimit() == 0) {
            BALL_LOG_INFO << "No memory allocation limit set!";
        }
        else {
            BALL_LOG_INFO << "Memory allocation limit set to "
                          << mwcu::PrintUtil::prettyBytes(
                                 d_config.allocationLimit());

            mwcma::CountingAllocator* topAllocator =
                dynamic_cast<mwcma::CountingAllocator*>(
                    mwcma::CountingAllocatorUtil::topAllocatorStore()
                        .baseAllocator());

            BSLS_ASSERT_OPT(topAllocator);
            topAllocator->setAllocationLimit(
                d_config.allocationLimit(),
                bdlf::BindUtil::bind(&onAllocationLimit,
                                     d_config.allocationLimit()));
        }
    }

    // ---------------
    // Control channel
    const bsl::string pipePath(d_bmqPrefix + "/bmqbrkr.ctl");
    rc = d_controlChannel.start(pipePath);
    if (rc != 0) {
        d_logController.shutdown();
        d_scheduler.stop();
        errorDescription << "failed to start pipe control channel "
                         << "(rc: " << rc << ")";
        // NOTE: Can't use the (10 * rc) + errorCode trick because
        //       'controlChannel.start()' may return positive error codes.
        return rc_CONTROLCHANNEL_START_FAILED;  // RETURN
    }

    if (mwcsys::ThreadUtil::k_SUPPORT_THREAD_NAME) {
        bdls::PipeUtil::send(pipePath,
                             bsl::string(k_MTRAP_SET_THREADNAME) + "\n");
    }

    // -------------------
    // M-Trap registration
    registerMTrapHandler(
        "HELP",
        "",
        "Display this message",
        bdlf::BindUtil::bind(
            &balb::ControlManager::printUsageHelper,
            &d_controlManager,
            &bsl::cout,
            "This process responds to the following messages:"));

    registerMTrapHandler(
        "LOG",
        "",
        "Log controlling commands",
        bdlf::BindUtil::bind(&Task::onLogCommand,
                             this,
                             bdlf::PlaceHolders::_1,    // prefix
                             bdlf::PlaceHolders::_2));  // istream

    d_isInitialized = true;
    return rc_SUCCESS;
}

void Task::shutdown()
{
    if (!d_isInitialized) {
        return;  // RETURN
    }

    // Stop the control channel
    d_controlChannel.shutdown();
    d_controlChannel.stop();

    // Shutdown the log controller
    d_logController.shutdown();

    // Stop the scheduler
    d_scheduler.stop();

    d_isInitialized = false;
}

int Task::registerMTrapHandler(const bsl::string&  prefix,
                               const bsl::string&  arguments,
                               const bsl::string&  description,
                               const MTrapHandler& handler)
{
    const int rc = d_controlManager.registerHandler(prefix,
                                                    arguments,
                                                    description,
                                                    handler);
    if (rc < 0) {
        BALL_LOG_ERROR << "Failed to register M-Trap handler for prefix "
                       << "'" << prefix << "' [rc: " << rc << "]";
    }

    return rc;
}

int Task::deregisterMTrapHandler(const bsl::string& prefix)
{
    return d_controlManager.deregisterHandler(prefix);
}

}  // close package namespace
}  // close enterprise namespace
