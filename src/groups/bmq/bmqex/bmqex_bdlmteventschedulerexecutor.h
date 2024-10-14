// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqex_bdlmteventschedulerexecutor.h                                -*-C++-*-
#ifndef INCLUDED_BMQEX_BDLMTEVENTSCHEDULEREXECUTOR
#define INCLUDED_BMQEX_BDLMTEVENTSCHEDULEREXECUTOR

//@PURPOSE: Provides an executor adapter for 'bdlmt::EventScheduler'.
//
//@CLASSES:
//  BdlmtEventSchedulerExecutor: an executor adapter for the bdlmt scheduler
//
//@DESCRIPTION:
// This component provides a class, 'bmqex::BdlmtEventSchedulerExecutor', that
// is an executor adapter for 'bdlmt::EventScheduler'.
//
/// Thread safety
///-------------
// With the exception of assignment operators, as well as the 'swap' member
// function, 'bmqex::BdlmtEventSchedulerExecutor' is fully thread-safe, meaning
// that multiple threads may use their own instances of the class or use a
// shared instance without further synchronization.
//
/// Usage
///-----
// Lets assume that you already have an instance of 'bdlmt::EventScheduler',
// 'd_eventScheduler'. Then, you can create an executor adapter on top of the
// scheduler, and use the adapter with the 'bmqex' package.  For example:
//..
//  // create an executor
//  bmqex::BdlmtEventSchedulerExecutor ex(&d_eventScheduler);
//
//  // use it
//  bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::oneWay()
//                                                           .neverBlocking()
//                                                           .useExecutor(ex),
//                                [](){ bsl::cout << "Hello World!\n"; });
//..
//
// By default, the job is executed as soon as possible.  You can also specify a
// point in time when you want the job to be executed:
//..
//  // create an executor with a time point of 1 second from now
//  bmqex::BdlmtEventSchedulerExecutor ex(
//                       &d_eventScheduler,
//                       bsls::SystemTime::now(d_eventScheduler.clockType())
//                                                             .addSeconds(1));
//
//  // use it ...
//..
// Note that the time point is relative to the clock used by the event
// scheduler.
//
// 'bmqex::BdlmtEventSchedulerExecutor' also provides a convenient
// 'rebindTimePoint' function that returns a new executor associated with the
// same event scheduler, but having a different time point:
//..
//  bmqex::BdlmtEventSchedulerExecutor ex2 =
//                            ex.rebindTimePoint(ex.timePoint().addSeconds(1));
//..

// BDE
#include <bsl_algorithm.h>  // bsl::swap
#include <bsl_functional.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}

namespace bmqex {

// =================================
// class BdlmtEventSchedulerExecutor
// =================================

/// Provides an executor adapter for `bdlmt::EventScheduler`.
class BdlmtEventSchedulerExecutor {
  public:
    // TYPES

    /// Defines the type of the associated execution context.
    typedef bdlmt::EventScheduler ContextType;

  private:
    // PRIVATE DATA
    ContextType* d_context_p;

    bsls::TimeInterval d_timePoint;

  public:
    // PUBLIC CLASS DATA

    /// The default time point used by this executor.  The time point is
    /// guaranteed to be in the past relative to any clock.
    static const bsls::TimeInterval k_DEFAULT_TIME_POINT;

  public:
    // CREATORS
    BdlmtEventSchedulerExecutor(
        ContextType*              context,
        const bsls::TimeInterval& timePoint = k_DEFAULT_TIME_POINT)
        BSLS_KEYWORD_NOEXCEPT;
    // IMPLICIT
    // Create a 'BdlmtEventSchedulerExecutor' object having the specified
    // 'context' as its associated execution context.  Optionally specify
    // the 'timePoint' used when submitting function objects to the
    // associated 'bdlmt::EventScheduler'.

  public:
    // MANIPULATORS

    /// Submit the specified function object `f` to the associated
    /// `bdlmt::EventScheduler` as if by
    /// `context().scheduleEvent(timePoint(), f)`.
    void post(const bsl::function<void()>& f) const;

    /// Swap the contents of `*this` and the specified `other`.
    void swap(BdlmtEventSchedulerExecutor& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return a reference to the associated `bdlmt::EventScheduler` object.
    ContextType& context() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the associated time point.
    const bsls::TimeInterval& timePoint() const BSLS_KEYWORD_NOEXCEPT;

    /// Return a `BdlmtEventSchedulerExecutor` object having the same
    /// associated execution context as `*this` and the specified
    /// `timePoint`, as if by
    /// `return BdlmtEventSchedulerExecutor(&context(), timePoint)`.
    BdlmtEventSchedulerExecutor rebindTimePoint(
        const bsls::TimeInterval& timePoint) const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BdlmtEventSchedulerExecutor,
                                   bsl::is_trivially_copyable)
};

// FREE OPERATORS

/// Return '&lhs.context()   == &rhs.context() &&
///          lhs.timePoint() ==  rhs.timePoint()'.
bool operator==(const BdlmtEventSchedulerExecutor& lhs,
                const BdlmtEventSchedulerExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Return `!(lhs == rhs)`.
bool operator!=(const BdlmtEventSchedulerExecutor& lhs,
                const BdlmtEventSchedulerExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Swap the contents of the specified `lhs` and `rhs`.
void swap(BdlmtEventSchedulerExecutor& lhs,
          BdlmtEventSchedulerExecutor& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ---------------------------------
// class BdlmtEventSchedulerExecutor
// ---------------------------------

// CREATORS
inline BdlmtEventSchedulerExecutor::BdlmtEventSchedulerExecutor(
    ContextType*              context,
    const bsls::TimeInterval& timePoint) BSLS_KEYWORD_NOEXCEPT
: d_context_p(context),
  d_timePoint(timePoint)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);
}

// MANIPULATORS
inline void BdlmtEventSchedulerExecutor::swap(
    BdlmtEventSchedulerExecutor& other) BSLS_KEYWORD_NOEXCEPT
{
    using bsl::swap;

    swap(d_context_p, other.d_context_p);
    swap(d_timePoint, other.d_timePoint);
}

// ACCESSORS
inline BdlmtEventSchedulerExecutor::ContextType&
BdlmtEventSchedulerExecutor::context() const BSLS_KEYWORD_NOEXCEPT
{
    return *d_context_p;
}

inline const bsls::TimeInterval&
BdlmtEventSchedulerExecutor::timePoint() const BSLS_KEYWORD_NOEXCEPT
{
    return d_timePoint;
}

inline BdlmtEventSchedulerExecutor
BdlmtEventSchedulerExecutor::rebindTimePoint(
    const bsls::TimeInterval& timePoint) const BSLS_KEYWORD_NOEXCEPT
{
    return BdlmtEventSchedulerExecutor(d_context_p, timePoint);
}

}  // close package namespace

// FREE OPERATORS
inline bool
bmqex::operator==(const BdlmtEventSchedulerExecutor& lhs,
                  const BdlmtEventSchedulerExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    return &lhs.context() == &rhs.context() &&
           lhs.timePoint() == rhs.timePoint();
}

inline bool
bmqex::operator!=(const BdlmtEventSchedulerExecutor& lhs,
                  const BdlmtEventSchedulerExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    return !(lhs == rhs);
}

inline void bmqex::swap(BdlmtEventSchedulerExecutor& lhs,
                        BdlmtEventSchedulerExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif
