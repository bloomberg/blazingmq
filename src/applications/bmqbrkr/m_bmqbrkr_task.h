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

// m_bmqbrkr_task.h                                                   -*-C++-*-
#ifndef INCLUDED_M_BMQBRKR_TASK
#define INCLUDED_M_BMQBRKR_TASK

//@PURPOSE: Provide base infrastructure for a task.
//
//@CLASSES:
//  m_bmqbrkr::Task:                  base infrastructure of a task
//  m_bmqbrkr::Task_AllocatorManager: guard-like mechanism to manage allocator
//
//@SEE_ALSO:
//  mwctsk::LogController: BALL logging management
//
//@DESCRIPTION: 'm_bmqbrkr::Task_AllocatorManager' is an internal mechanism
// allowing to setup different kind of allocators to use through the entire
// task.  'm_bmqbrkr::Task' is a generic mechanism providing the base
// infrastructure for a task, controlling the following aspects of it:
//: o *memory*: memory allocators are managed using the
//:   'm_bmqbrkr::Task_AllocatorManager' component,
//: o *logging*: an 'mwctsk::LogController' is used to initialize and
//:   configure logging to file and console,
//: o *control channel command*: a 'balb::PipeControlChannel' component is used
//:   to create a pipe allowing to send IPC command (called M-Trap) to the task
//
/// Allocators
///----------
// The following allocator models are supported:
//: o *NEWDELETE*:      using 'bslma:NewDeleteAllocator'
//: o *STACKTRACETEST*: using 'balst::StackTraceTestAllocator'
//: o *COUNTING*:       using 'mwcma::CountingAllocator'
// Note that when using the 'COUNTING' allocator, the 'default' and 'global'
// allocators are changed to be counting allocators too.
//
/// Lifecycle
///---------
// Since this object is in charge of initializing the logging system, and
// setting up the appropriate allocators, typical usage is to create it as
// early as possible in main, and destroy it last.
//
/// Pipe Names
///----------
// The pipe name supplied by the caller is assumed to be an unqualified name
// (i.e., having no root or path elements).  The specified pipe name is coerced
// to lower case and then used to construct an implementation-defined pipe
// name, which can be accessed using the 'pipeName' method.  The caller is
// responsible for ensuring that the supplied name is unique on a given system;
// otherwise, 'initialize' will fail.

// MWC
#include <mwcma_countingallocatorstore.h>
#include <mwctsk_logcontroller.h>

// MQB
#include <mqbcfg_messages.h>

// BDE
#include <balb_controlmanager.h>
#include <balb_pipecontrolchannel.h>
#include <ball_log.h>
#include <balst_stacktracetestallocator.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
#include <bsls_objectbuffer.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdld {
class Datum;
}
namespace mwcst {
class StatContext;
}

namespace m_bmqbrkr {

// ===========================
// class Task_AllocatorManager
// ===========================

/// Guard-like mechanism to manage allocators for use by the entire process.
class Task_AllocatorManager {
  private:
    // DATA
    mqbcfg::AllocatorType::Value d_type;
    // Type of allocator in use.

    bsls::ObjectBuffer<mwcma::CountingAllocatorStore> d_store;
    // Allocator store, to dispence allocators
    // based on the configured type; used if
    // type is 'e_NEWDELETE' or
    // 'e_STACKTRACETEST'.

    balst::StackTraceTestAllocator d_stackTraceTestAllocator;
    // The stack trace test allocator, if type
    // is 'e_STACKTRACETEST'.

    mwcma::CountingAllocatorStore* d_store_p;
    // Raw pointer to the allocator store to
    // use; regardless of the configured type.

    mwcst::StatContext* d_statContext_p;
    // The StatContext of the counting
    // allocators, if type is 'e_COUNTING', or
    // null otherwise.

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    Task_AllocatorManager(const Task_AllocatorManager&);  // = delete;
    Task_AllocatorManager&
    operator=(const Task_AllocatorManager&);  // = delete;

  public:
    // CREATORS

    /// Create a new object, configured to use the specified `type`
    /// allocator'.  Note that when using the `COUNTING` allocator, the
    /// `default` and `global` allocators are changed to be counting
    /// allocators too.
    explicit Task_AllocatorManager(mqbcfg::AllocatorType::Value type);

    /// Destroy this object.  If the allocator is of type `STACKTRACETEST`,
    /// any detected memory leak will be reported to stdout.
    ~Task_AllocatorManager();

    // MANIPULATORS

    /// Return the stat context keeping track of the allocations, if this
    /// object has been initialized with the `COUNTING` allocator, or null
    /// otherwise.
    mwcst::StatContext* statContext();

    /// Return a reference offering modifiable access to the allocator store
    /// to use.
    mwcma::CountingAllocatorStore& store();

    // ACCESSORS

    /// Return the type of allocator used.
    mqbcfg::AllocatorType::Value type() const;
};

//===========
// class Task
//===========

/// This class provides the base infrastructure for a task.
class Task {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQBRKR.TASK");

  public:
    // TYPES

    /// Defines a type alias for the function called to handle control
    /// messages (aka M-Trap message).  The specified `prefix` argument is
    /// the first space-delimited word read from the message, and the
    /// specified `stream` argument contains the remainder of the message.
    typedef bsl::function<void(const bsl::string& prefix,
                               bsl::istream&      stream)>
        MTrapHandler;

  private:
    // DATA
    Task_AllocatorManager d_allocatorManager;
    // Allocator manager object, must be the first
    // member so that other members can be
    // initialized with a proper allocator from it.

    const mqbcfg::TaskConfig& d_config;
    // Configuration to use, const reference to the
    // one held in main.

    bool d_isInitialized;
    // True is this object has been initialized.

    bsl::string d_bmqPrefix;
    // BMQ_PREFIX directory

    bdlmt::EventScheduler d_scheduler;
    // EventScheduler

    mwctsk::LogController d_logController;
    // Log controller for log file and console
    // observer

    balb::ControlManager d_controlManager;
    // M-Trap callback registry

    balb::PipeControlChannel d_controlChannel;
    // M-Trap IPC mechanism

  private:
    // PRIVATE MANIPULATORS

    /// Callback invoked when an M-Trap command, in the specified `message`,
    /// has been received; in charge of dispatching it to the appropriate
    /// registered handler.  Return 0 on success or a non-zero value
    /// otherwise.
    int onControlMessage(const bsl::string& message);

    /// M-Trap handler for the `LOG` command.  The specified `prefix`
    /// contains the command (`LOG` in this case), and the specified `input`
    /// contains the stream of arguments for the command.
    void onLogCommand(const bsl::string& prefix, bsl::istream& input);

  private:
    // NOT IMPLEMENTED
    Task(const Task&) BSLS_KEYWORD_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    Task& operator=(const Task&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a new task, creating the pipe control channel in the
    /// specified `bmqPrefix` directory and using the specified `config`.
    Task(const bsl::string& bmqPrefix, const mqbcfg::TaskConfig& config);

    /// Destroy this object.  If the task was successfully initialized,
    /// `shutdown()` must be called prior to destruction of this object.
    ~Task();

    // MANIPULATORS

    /// Initialize this object.  Return 0 on success and a non-zero value
    /// otherwise, populating the specified `errorDescription` with a
    /// description of the error.  The initialization leverages the
    /// properties from the `config` parameter supplied at construction of
    /// this object.
    int initialize(bsl::ostream& errorDescription);

    /// Shutdown the object, effectively closing the pipe control channel
    /// and doing a teardown of the logging system.
    void shutdown();

    // M-Trap Registration

    /// Register the specified `handler` to be invoked whenever a control
    /// message (aka M-Trap) having the specified `prefix`, is received by
    /// this task.  The specified `arguments` can be used to describe the
    /// arguments accepted by the M-Trap and the specified `description` to
    /// describe its operation; these are printed as part of this task's
    /// HELP output.  Return a positive value if an existing callback was
    /// replaced, return 0 if no replacement occurred, and return a negative
    /// value otherwise.
    int registerMTrapHandler(const bsl::string&  prefix,
                             const bsl::string&  arguments,
                             const bsl::string&  description,
                             const MTrapHandler& handler);

    /// De-Register the M-Trap handler associated with the specified
    /// `prefix`.  Return 0 on success, and a non-zero value otherwise.
    int deregisterMTrapHandler(const bsl::string& prefix);

    /// Return the stat context keeping track of the allocations, if this
    /// object has been initialized with the `COUNTING` allocator, or null
    /// otherwise.
    mwcst::StatContext* allocatorStatContext();

    /// Return a reference offering modifiable access to the allocator store
    /// to use.
    mwcma::CountingAllocatorStore& allocatorStore();

    /// Return a reference offering modifiable access to the log controller
    /// object used by this task.
    mwctsk::LogController& logController();

    /// Return a pointer to the event scheduler.
    bdlmt::EventScheduler* scheduler();

    // ACCESSORS

    /// Return the path of the named pipe used for the control channel.  The
    /// behavior is undefined unless the task has been initialized.
    const bsl::string& pipeName() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class Task_AllocatorManager
// ---------------------------

inline mwcst::StatContext* Task_AllocatorManager::statContext()
{
    return d_statContext_p;
}

inline mwcma::CountingAllocatorStore& Task_AllocatorManager::store()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_store_p);

    return *d_store_p;
}

inline mqbcfg::AllocatorType::Value Task_AllocatorManager::type() const
{
    return d_type;
}

// ----------
// class Task
// ----------

inline mwcst::StatContext* Task::allocatorStatContext()
{
    return d_allocatorManager.statContext();
}

inline mwcma::CountingAllocatorStore& Task::allocatorStore()
{
    return d_allocatorManager.store();
}

inline mwctsk::LogController& Task::logController()
{
    return d_logController;
}

inline bdlmt::EventScheduler* Task::scheduler()
{
    return &d_scheduler;
}

inline const bsl::string& Task::pipeName() const
{
    return d_controlChannel.pipeName();
}

}  // close package namespace
}  // close enterprise namespace

#endif
