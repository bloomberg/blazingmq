// Copyright 2014-2025 Bloomberg Finance L.P.
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

// bmqsys_operationlogger.h                                           -*-C++-*-
#ifndef INCLUDED_BMQSYS_OPERATIONLOGGER
#define INCLUDED_BMQSYS_OPERATIONLOGGER

//@PURPOSE: Provide utility to measure and log operation time.
//
//@CLASSES:
// bmqsys::OperationLogger
//
//@DESCRIPTION:
// This component provides an utility class `bmqsys::OperationLogger` that can
// be used to measure and log time spent to perform operation.
// There are several benefits of using this class:
//: o Ability to perform measures and to log them in an uniform style.
//: o Ability to pass operation loggers between different threads to perform
//:   measures end-to-end.
//: o Simplify classes that measure operation times by removing their internal
//:   states used to keep track of time and operation name.
//: o Allow to simply keep track of several async operations at the same time
//:   by using different instances of `bmqsys::OperationLogger`.
//: o Flexibility in usage pattern: operation loggers might be used as fully
//:   disposable objects and as reusable objects (object pool compatible).
//
/// Usage
///-----
// 1. Simple scoped operation logger
//..
//  {
//      bmqsys::OperationLogger opLogger(allocator);
//      opLogger.start() << "Scoped operation";
//
//      doSomething1();
//      doSomething2();
//      // `opLogger` is destructed here and it will log the total time passed
//      // since its creation.
//  }
//..
//
// 2. Pass operation logger to another function
//..
//  bsl::shared_ptr<bmqsys::OperationLogger> opLogger =
//      bsl::allocate_shared<bmqsys::OperationLogger>(d_state.d_allocator_p);
//  opLogger.start() << "Pass operation logger";
//  // Make sure to `move` so we don't have a copy of this shared pointer in
//  // this scope for accurate results.
//  doSomething(bslmf::MovableRefUtil::move(opLogger));
//..
//
// 3. Use pool for operation loggers
//..
//  typedef bdlcc::SharedObjectPool<
//      bmqsys::OperationLogger,
//      bdlcc::ObjectPoolFunctors::DefaultCreator,
//      bdlcc::ObjectPoolFunctors::Clear<bmqsys::OperationLogger> >
//      OperationLoggerPool;
//
//  OperationLoggerPool pool(-1, allocator);
//  for (int i = 0; i < 100; i++) {
//      bsl::shared_ptr<bmqsys::OperationLogger> opLogger = pool.getObject();
//      opLogger.start() << "Dynamic operation " << i;
//      thread.doSomethingAsync(opLogger);
//  }
//  // Finish all tasks in the worker thread and ensure that `pool` outlives
//  // shared pointers that have to return to this pool.
//  thread.join();
//..

// BDE
#include <ball_log.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslmf_nestedtraitdeclaration.h>

// BMQ
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

namespace BloombergLP {
namespace bmqsys {

// =====================
// class OperationLogger
// =====================

/// Operarion execution and logging context holding the operation description
/// and start time.
class OperationLogger BSLS_KEYWORD_FINAL {
  private:
    // PRIVATE DATA

    /// High resolution time of the current operation start.
    bsls::Types::Int64 d_beginTimestamp;

    /// Stream holding current operation description.
    bmqu::MemOutStream d_currentOpDescription;

    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQSYS.OPERATIONLOGGER");

    // NOT IMPLEMENTED
    /// Copy and assignment are disabled to prevent misuse of this class.
    OperationLogger(const OperationLogger&) BSLS_KEYWORD_DELETED;
    OperationLogger& operator=(const OperationLogger&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OperationLogger, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create this object using the optionally specified `allocator`.
    explicit OperationLogger(bslma::Allocator* allocator = 0);

    // Destroy this object.  If this object has an ongoing measurement for any
    // operation, stop and log it.
    ~OperationLogger();

    // MANIPULATORS
    /// Start the current operation by saving the current timestamp and return
    /// the modifiable output stream to set up the operation name.
    bsl::ostream& start();

    /// Return the modifiable stream to the stream holding the current
    /// operation name.  This call does not reset the stored start timestamp
    /// and can be used between `start()` and `stop()` calls to append or
    /// update the operation name.
    bsl::ostream& operation();

    /// Stop the current operation and log the measured execution time.
    void stop();

    /// The same as `stop()`, defined to be used with
    /// `bdlcc::ObjectPoolFunctors::Clear<OperationLogger>`.
    void clear();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class OperationLogger
// ---------------------

inline bsl::ostream& OperationLogger::start()
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_beginTimestamp != 0 || !d_currentOpDescription.isEmpty())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_WARN << "Trying to start OperationLogger with non-stopped "
                      << "operation: d_beginTimestamp = " << d_beginTimestamp
                      << ", d_currentOpDescription = \""
                      << d_currentOpDescription.str() << "\"";
        d_currentOpDescription.reset();
    }

    d_beginTimestamp = bmqsys::Time::highResolutionTimer();
    return d_currentOpDescription;
}

inline bsl::ostream& OperationLogger::operation()
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_beginTimestamp == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_WARN << "Trying to access the name of an operation that was "
                      << "not started";
    }
    return d_currentOpDescription;
}

inline void OperationLogger::stop()
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_beginTimestamp == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_WARN << "Calling `OperationLogger::stop` multiple times";
        return;  // RETURN
    }
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_currentOpDescription.isEmpty())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_WARN << "Empty operation description in OperationLogger: "
                      << "operation was not set on `start` call.";
        return;  // RETURN
    }

    BALL_LOG_INFO_BLOCK
    {
        const bsls::Types::Int64 elapsed =
            bmqsys::Time::highResolutionTimer() - d_beginTimestamp;
        BALL_LOG_OUTPUT_STREAM
            << d_currentOpDescription.str()
            << " took: " << bmqu::PrintUtil::prettyTimeInterval(elapsed)
            << " (" << elapsed << " nanoseconds)";
    }

    d_beginTimestamp = 0;
    d_currentOpDescription.reset();
}

inline void OperationLogger::clear()
{
    stop();
}

}  // close package namespace
}  // close enterprise namespace

#endif
