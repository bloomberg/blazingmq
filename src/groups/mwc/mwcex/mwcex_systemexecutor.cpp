// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mwcex_systemexecutor.cpp                                           -*-C++-*-
#include <mwcex_systemexecutor.h>

#include <mwcscm_version.h>
// BDE
#include <bsl_functional.h>
#include <bslma_default.h>
#include <bslmt_lockguard.h>
#include <bslmt_once.h>
#include <bslmt_threadattributes.h>
#include <bsls_objectbuffer.h>
#include <bsls_platform.h>

namespace BloombergLP {
namespace mwcex {

namespace {

// ===================
// struct ContextGuard
// ===================

/// Provides a guard that calls `shutdownSingletonImpl` on destruction.
struct ContextGuard {
    // CREATORS
    ~ContextGuard();
};

// GLOBAL DATA

/// Pointer to the execution context singleton object. The reason this is an
/// atomic is that this pointer might be initialized from one thread, and be
/// read from another.
bsls::AtomicPointer<SystemExecutor_Context> s_context_p(0);

/// Memory buffer used to store the singleton object.
bsls::ObjectBuffer<SystemExecutor_Context> s_contextBuffer;

// UTILITY FUNCTIONS

/// Initialize the singleton object, if not already, with the optionally
/// specified `globalAllocator`. Load into the specified `wasInit` `true` if
/// the singleton object was initialized, and `false` otherwise. If
/// `globalAllocator` is 0, use the currently installed global allocator.
/// Return a reference to the singleton object. The behavior id undefined if
/// the singleton has already been destroyed via a call to
/// `shutdownSingletonImpl`.
SystemExecutor_Context&
initSingletonImpl(bool* wasInit, bslma::Allocator* globalAllocator = 0)
{
    // PRECONDITIONS
    BSLS_ASSERT(wasInit);

    *wasInit = false;

    BSLMT_ONCE_DO
    {
        // obtain allocator
        bslma::Allocator* allocator = bslma::Default::globalAllocator(
            globalAllocator);

        // create context and set singleton pointer
        s_context_p.storeRelease(new (s_contextBuffer.address())
                                     SystemExecutor_Context(allocator));

        // set initialization flag
        *wasInit = true;

#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif
        // NOTE: Suppress Clang annoying warning.

        // This object will destroy the singleton object (if not already) when
        // the application terminates.
        static ContextGuard s_contextGuard;
#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic pop
#endif
    }

    // get singleton pointer
    SystemExecutor_Context* context = s_context_p.loadAcquire();

    BSLS_ASSERT(context && "Singleton not initialized");
    return *context;
}

/// Destroy the singleton object, if not already. Load into the specified
/// `wasShutdown` `true` if the singleton object was destroyed, and `false`
/// otherwise.
void shutdownSingletonImpl(bool* wasShutdown) BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(wasShutdown);

    // reset singleton pointer
    SystemExecutor_Context* context = s_context_p.swapAcqRel(0);

    // set shutdown flag
    *wasShutdown = static_cast<bool>(context);

    if (context) {
        // If the singleton object is initialized, destroy it.
        context->~SystemExecutor_Context();
    }
}

// -------------------
// struct ContextGuard
// -------------------

// CREATORS
ContextGuard::~ContextGuard()
{
    bool wasShutdown;  // unused
    shutdownSingletonImpl(&wasShutdown);
}

}  // close unnamed namespace

// ----------------------------
// class SystemExecutor_Context
// ----------------------------

// PRIVATE MANIPULATORS
void SystemExecutor_Context::finalizeThread(
    const bslmt::ThreadUtil::Handle& thread) BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(
        bslmt::ThreadUtil::areEqual(thread, bslmt::ThreadUtil::self()));

    bslmt::LockGuard<bslmt::Mutex> lock(&d_threadExitMutex);  // LOCK

    unsigned unjoinedThreads = 1;
    if (d_lastCompletedThread != bslmt::ThreadUtil::invalidHandle()) {
        // We have unjoined threads besides this one.

        // join the last completed thread
        int rc = bslmt::ThreadUtil::join(d_lastCompletedThread);
        BSLS_ASSERT_OPT(rc == 0);

        // decrement unjoined threads counter
        unjoinedThreads = d_unjoinedThreads.subtractAcqRel(1);
    }

    // this is the last completed thread now
    d_lastCompletedThread = thread;

    if (unjoinedThreads == 1) {
        // This is the last unjoined thread. Report that.
        d_threadExitCondition.signal();

        // NOTE: This may be a false-positive report, since another thread may
        //       have been spawned right after the 'd_unjoinedThreads' counter
        //       check.
    }
}

// CLASS METHODS
SystemExecutor_Context& SystemExecutor_Context::singleton()
{
    bool wasInit;  // unused
    return initSingletonImpl(&wasInit);
}

SystemExecutor_Context&
SystemExecutor_Context::initSingleton(bslma::Allocator* globalAllocator)
{
    bool                    wasInit;
    SystemExecutor_Context& context = initSingletonImpl(&wasInit,
                                                        globalAllocator);

    BSLS_ASSERT(wasInit && "Singleton already initialized");
    return context;
}

void SystemExecutor_Context::shutdownSingleton() BSLS_KEYWORD_NOEXCEPT
{
    bool wasShutdown;
    shutdownSingletonImpl(&wasShutdown);

    BSLS_ASSERT(wasShutdown && "Singleton not initialized");
}

// CREATORS
SystemExecutor_Context::SystemExecutor_Context(bslma::Allocator* allocator)
: d_threadDataAllocator(
      sizeof(SystemExecutor_ThreadData<bsl::function<void()> >),
      allocator)
, d_threadExitMutex()
, d_threadExitCondition()
, d_unjoinedThreads(0)
, d_lastCompletedThread(bslmt::ThreadUtil::invalidHandle())
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);

    // NOTE: The block size for the thread data allocator is an arbitrary
    //       value, large enough to hold an 'SystemExecutor_ThreadData' object
    //       containing a "small" 'bsl::function'-sized functor.
}

SystemExecutor_Context::~SystemExecutor_Context()
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_threadExitMutex);  // LOCK

    if (d_unjoinedThreads != 0) {
        // We have unjoined threads.

        // wait till all threads complete
        while (d_unjoinedThreads != 1 ||
               d_lastCompletedThread == bslmt::ThreadUtil::invalidHandle()) {
            d_threadExitCondition.wait(&d_threadExitMutex);  // UNLOCK / LOCK
        }

        // join the last thread
        int rc = bslmt::ThreadUtil::join(d_lastCompletedThread);
        BSLS_ASSERT_OPT(rc == 0);
    }
}

// ACCESSORS
bslma::Allocator*
SystemExecutor_Context::allocator() const BSLS_KEYWORD_NOEXCEPT
{
    return d_allocator_p;
}

// --------------------
// class SystemExecutor
// --------------------

// CREATORS
SystemExecutor::SystemExecutor()
: d_threadAttributes()
{
    // Initialize the context singleton if not already. If we don't, the
    // singleton will be initialized on the first call to 'post' / 'dispatch',
    // which is fine, but doing this on construction feels more natural.
    SystemExecutor_Context::singleton();
}

SystemExecutor::SystemExecutor(const bslmt::ThreadAttributes& threadAttributes,
                               bslma::Allocator*              basicAllocator)
: d_threadAttributes(
      bsl::allocate_shared<bslmt::ThreadAttributes>(basicAllocator,
                                                    threadAttributes))
{
    // PRECONDITIONS
    BSLS_ASSERT(threadAttributes.detachedState() ==
                bslmt::ThreadAttributes::e_CREATE_JOINABLE);

    // Initialize the context singleton if not already. If we don't, the
    // singleton will be initialized on the first call to 'post' / 'dispatch',
    // which is fine, but doing this on construction feels more natural.
    SystemExecutor_Context::singleton();
}

// ACCESSORS
const bslmt::ThreadAttributes*
SystemExecutor::threadAttributes() const BSLS_KEYWORD_NOEXCEPT
{
    return d_threadAttributes.get();
}

}  // close package namespace

// FREE OPERATORS
bool mwcex::operator==(const SystemExecutor& lhs,
                       const SystemExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    if (lhs.threadAttributes() == rhs.threadAttributes()) {
        // Both executors share the same 'bslmt::ThreadAttributes' object, or
        // both have no attributes at all. Therefore they are equivalent.
        return true;  // RETURN
    }

    if (!lhs.threadAttributes() || !rhs.threadAttributes()) {
        // One executor does have attributes and the other one doesn't.
        // Therefore they are not equivalent.
        return false;  // RETURN
    }

    // both executors have attributes, compare them
    return *lhs.threadAttributes() == *rhs.threadAttributes();
}

bool mwcex::operator!=(const SystemExecutor& lhs,
                       const SystemExecutor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    return !(lhs == rhs);
}

}  // close enterprise namespace
