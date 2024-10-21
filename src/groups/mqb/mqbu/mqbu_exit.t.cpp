// Copyright 2023 Bloomberg Finance L.P.
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

// mqbu_exit.t.cpp                                                    -*-C++-*-
#include <mqbu_exit.h>

#include <bmqu_memoutstream.h>

// SYS
#include <sys/wait.h>
#include <unistd.h>

#if defined(BSLS_PLATFORM_OS_UNIX)
#include <signal.h>
#endif

// BDE
#include <bsl_cerrno.h>
#include <bsl_charconv.h>
#include <bsl_csignal.h>
#include <bsl_functional.h>
#include <bsl_string.h>
#include <bslmt_threadutil.h>
#include <bslmt_timedsemaphore.h>
#include <bsls_atomic.h>
#include <bsls_objectbuffer.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

// NOTE: An object buffer is needed to avoid exit time destructors because
//       this is a non-POD type.

/// Semaphore with static storage on which the installed signal handler
/// posts upon invocation, defined here because `signal` accepts a signal
/// handler function pointer that only accepts an int and fails to accept
/// a `bsl::function` to which an output variable would be bound.
static bsls::ObjectBuffer<bslmt::TimedSemaphore> g_semaphore;

/// Incremented everytime the signal handler is being invoked.  Variable
/// with static storage populated by the installed signal handler, defined
/// here because `signal` accepts a signal handler function pointer that
/// only accepts an int and fails to accept a `bsl::function` to which an
/// output variable would be bound.
static bsls::AtomicInt g_signalCount(0);

/// Callback method for signal handling, this will increment the
/// `g_signalCount` and post of the `g_semaphore`.
static void dummySignalHandler(int signal)
{
    PVV_SAFE("received signal '" << strsignal(signal) << "' (" << signal
                                 << ")]\n");

    // We only expect 'SIGINT' signal
    ASSERT_EQ(signal, SIGINT);

    ++g_signalCount;
    g_semaphore.object().post();
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_exitCode_toAscii()
// ------------------------------------------------------------------------
// EXIT CODE - TO ASCII
//
// Concerns:
//   Proper behavior of the 'ExitCode::toAscii' method.
//
// Plan:
//   Verify that the 'toAscii' method returns the string representation of
//   every enum value of 'ExitCode'.
//
// Testing:
//   'ExitCode::toAscii'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EXIT CODE - TO ASCII");

    struct Test {
        int         d_line;
        int         d_value;
        const char* d_expected;
    } k_DATA[] = {{L_, 0, "SUCCESS"},
                  {L_, 1, "COMMAND_LINE"},
                  {L_, 2, "CONFIG_GENERATION"},
                  {L_, 3, "TASK_INITIALIZE"},
                  {L_, 4, "BENCH_START"},
                  {L_, 5, "APP_INITIALIZE"},
                  {L_, 6, "RUN"},
                  {L_, 7, "QUEUEID_FULL"},
                  {L_, 8, "SUBQUEUEID_FULL"},
                  {L_, 9, "RECOVERY_FAILURE"},
                  {L_, 10, "STORAGE_OUT_OF_SYNC"},
                  {L_, 11, "UNSUPPORTED_SCENARIO"},
                  {L_, 12, "MEMORY_LIMIT"},
                  {L_, 13, "REQUESTED"},
                  {L_, -1, "(* UNKNOWN *)"}};
    // NOTE: Using the 'integer' value instead of the enum to ensure the
    //       numeric values are *never* changed.

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: toAscii(" << test.d_value
                        << ") == " << test.d_expected);

        bsl::string ascii(s_allocator_p);
        ascii = mqbu::ExitCode::toAscii(mqbu::ExitCode::Enum(test.d_value));

        ASSERT_EQ_D(test.d_line, ascii, test.d_expected);
    }
}

static void test2_exitCode_fromAscii()
// ------------------------------------------------------------------------
// EXIT CODE - FROM ASCII
//
// Concerns:
//   Proper behavior of the 'ExitCode::fromAscii' method.
//
// Plan:
//   Verify that the 'fromAscii' method returns the string representation
//   of every enum value of 'ExitCode'.
//
// Testing:
//   'ExitCode::fromAscii'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EXIT CODE - FROM ASCII");

    struct Test {
        int         d_line;
        const char* d_input;
        bool        d_isValid;
        int         d_expected;
    } k_DATA[] = {{L_, "SUCCESS", true, 0},
                  {L_, "COMMAND_LINE", true, 1},
                  {L_, "CONFIG_GENERATION", true, 2},
                  {L_, "TASK_INITIALIZE", true, 3},
                  {L_, "BENCH_START", true, 4},
                  {L_, "APP_INITIALIZE", true, 5},
                  {L_, "RUN", true, 6},
                  {L_, "QUEUEID_FULL", true, 7},
                  {L_, "SUBQUEUEID_FULL", true, 8},
                  {L_, "RECOVERY_FAILURE", true, 9},
                  {L_, "STORAGE_OUT_OF_SYNC", true, 10},
                  {L_, "UNSUPPORTED_SCENARIO", true, 11},
                  {L_, "MEMORY_LIMIT", true, 12},
                  {L_, "REQUESTED", true, 13},
                  {L_, "invalid", false, -1}};
    // NOTE: Using the 'integer' value instead of the enum to ensure the
    //       numeric values are *never* changed.

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: fromAscii(" << test.d_input
                        << ") == " << test.d_expected);

        mqbu::ExitCode::Enum obj;
        ASSERT_EQ_D(test.d_line,
                    mqbu::ExitCode::fromAscii(&obj, test.d_input),
                    test.d_isValid);
        if (test.d_isValid) {
            ASSERT_EQ_D(test.d_line, static_cast<int>(obj), test.d_expected);
        }
    }
}

static void test3_exitCode_print()
// ------------------------------------------------------------------------
// EXIT CODE - PRINT
//
// Concerns:
//   Proper behavior of the 'ExitCode::print' method.
//
// Plan:
//   1. Verify that the 'print' and 'operator<<' methods output the
//      expected string representation of every enum value of 'ExitCode'.
//   2. Verify that the 'print' method outputs nothing when the stream has
//      the bad bit set.
//
// Testing:
//   'ExitCode::print'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EXIT CODE - PRINT");

    // 1.
    struct Test {
        int         d_line;
        int         d_value;
        const char* d_expected;
    } k_DATA[] = {{L_, 0, "SUCCESS"},
                  {L_, 1, "COMMAND_LINE"},
                  {L_, 2, "CONFIG_GENERATION"},
                  {L_, 3, "TASK_INITIALIZE"},
                  {L_, 4, "BENCH_START"},
                  {L_, 5, "APP_INITIALIZE"},
                  {L_, 6, "RUN"},
                  {L_, 7, "QUEUEID_FULL"},
                  {L_, 8, "SUBQUEUEID_FULL"},
                  {L_, 9, "RECOVERY_FAILURE"},
                  {L_, 10, "STORAGE_OUT_OF_SYNC"},
                  {L_, 11, "UNSUPPORTED_SCENARIO"},
                  {L_, 12, "MEMORY_LIMIT"},
                  {L_, 13, "REQUESTED"},
                  {L_, -1, "(* UNKNOWN *)"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: print(" << test.d_value
                        << ") == " << test.d_expected);

        // 1.
        bmqu::MemOutStream   out(s_allocator_p);
        mqbu::ExitCode::Enum obj(
            static_cast<mqbu::ExitCode::Enum>(test.d_value));

        // print
        mqbu::ExitCode::print(out, obj, 0, 0);

        PVV(test.d_line << ": '" << out.str());

        bsl::string expected(s_allocator_p);
        expected.assign(test.d_expected);
        expected.append("\n");
        ASSERT_EQ_D(test.d_line, out.str(), expected);

        // operator<<
        out.reset();
        out << obj;

        ASSERT_EQ_D(test.d_line, out.str(), test.d_expected);

        // 2. 'badbit' set
        out.reset();
        out.setstate(bsl::ios_base::badbit);
        mqbu::ExitCode::print(out, obj, 0, -1);

        ASSERT_EQ_D(test.d_line, out.str(), "");
    }
}

static void test4_exit_terminate(int argc, char* argv[])
// ------------------------------------------------------------------------
// EXIT UTIL - TERMINATE
//
// Concerns:
//   'terminate' calls 'bsl::exit' with the specified exit code.
//
// Plan:
//   Create a new process and wait for it to terminate.  Verify that its
//   exit status is the specified exit code.
//
// Testing:
//   'ExitUtil::terminate'
// ------------------------------------------------------------------------
{
    // PRECONDITIONS
    BSLS_ASSERT(argc >= 2);
    BSLS_ASSERT(argv);

    if (argc >= 3 && bsl::strcmp(argv[2], "--fork") == 0) {
        // This is a fork.

        // Terminate with a reason.
        const int terminateReason = argc >= 4 ? bsl::atoi(argv[3]) : 0;
        mqbu::ExitUtil::terminate(
            static_cast<mqbu::ExitCode::Enum>(terminateReason));  // EXIT
    }

    bmqtst::TestHelper::printTestName("EXIT UTIL - TERMINATE");

    struct Test {
        int                  d_line;
        mqbu::ExitCode::Enum d_reason;
    } k_DATA[] = {{L_, mqbu::ExitCode::e_SUCCESS},
                  {L_, mqbu::ExitCode::e_COMMAND_LINE},
                  {L_, mqbu::ExitCode::e_CONFIG_GENERATION},
                  {L_, mqbu::ExitCode::e_TASK_INITIALIZE},
                  {L_, mqbu::ExitCode::e_BENCH_START},
                  {L_, mqbu::ExitCode::e_APP_INITIALIZE},
                  {L_, mqbu::ExitCode::e_RUN},
                  {L_, mqbu::ExitCode::e_QUEUEID_FULL},
                  {L_, mqbu::ExitCode::e_SUBQUEUEID_FULL},
                  {L_, mqbu::ExitCode::e_RECOVERY_FAILURE},
                  {L_, mqbu::ExitCode::e_STORAGE_OUT_OF_SYNC},
                  {L_, mqbu::ExitCode::e_UNSUPPORTED_SCENARIO},
                  {L_, mqbu::ExitCode::e_MEMORY_LIMIT},
                  {L_, mqbu::ExitCode::e_REQUESTED}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": terminating with reason '" << test.d_reason
                        << "'");

        pid_t pid = fork();

        if (pid == -1) {
            // 'Fork' failed.
            PRINT("Error in 'fork' [errno: " << strerror(errno) << "]");
            BSLS_ASSERT_OPT(false);
        }
        else if (pid == 0) {
            // Child process.

            // run self binary with a special "--fork" argument that will let
            // the test know it is a forked process
            char reasonStr[16] = {};
            bsl::to_chars(reasonStr,
                          reasonStr + 16,
                          static_cast<int>(test.d_reason),
                          10);  // base

            execl(argv[0],  // self binary path
                  argv[0],  // self binary path
                  argv[1],  // test case number
                  "--fork",
                  reasonStr,
                  static_cast<char*>(0));
        }
        else {
            // Parent process, wait for child to terminate
            PVV("child pid: " << pid);

            int status = -1;
            ASSERT_EQ(pid, waitpid(pid, &status, 0));

            ASSERT(WIFEXITED(status));
            ASSERT_EQ(static_cast<int>(test.d_reason), WEXITSTATUS(status));
        }
    }
}

static void test5_exit_shutdown()
// ------------------------------------------------------------------------
// EXIT UTIL - SHUTDOWN
//
// Concerns:
//   'shutdown' notifies the registered signal handler with SIGINT when
//   invoked from the current (main) thread or a subthread.
//
// Plan:
//   Install a signal handler asserting that the passed signal is SIGINT
//   and invoke 'ExitUtil::shutdown()'.
//   Create a new Thread, and execute 'ExitUtil::shutdown' from it.
//
// Testing:
//   'ExitUtil::shutdown'
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    s_ignoreCheckGblAlloc = true;
    // For an unknown reason, this test case fails the default allocator
    // check, and whenever main sets '_da.setAllocationLimit(0)', it
    // suddenly passes the default allocator check.
    // Ignore global allocation because we create a thread, which allocates
    // from global.

    bmqtst::TestHelper::printTestName("EXIT UTIL - SHUTDOWN");

    const int k_MAX_WAIT_SECONDS_AT_SHUTDOWN = 3;

    new (g_semaphore.buffer())
        bslmt::TimedSemaphore(bsls::SystemClockType::e_MONOTONIC);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags   = 0;
    sa.sa_handler = dummySignalHandler;

    int rc = ::sigaction(SIGINT, &sa, NULL);
    if (rc != 0) {
        PRINT("Can't catch SIGINT [rc: " << rc << "]");
        BSLS_ASSERT_OPT(false);
    }

    ASSERT_EQ(g_signalCount, 0);

    {
        PV("Calling 'ExitUtil::shutdown(" << mqbu::ExitCode::e_SUCCESS << ")'"
                                          << " from main-thread");

        mqbu::ExitUtil::shutdown(mqbu::ExitCode::e_SUCCESS);

        const bsls::TimeInterval timeout =
            bsls::SystemTime::nowMonotonicClock().addSeconds(
                k_MAX_WAIT_SECONDS_AT_SHUTDOWN);

        rc = g_semaphore.object().timedWait(timeout);
        if (rc != 0) {
            PRINT("'ExitUtil::shutdown' failed to invoke the signal handler in"
                  << k_MAX_WAIT_SECONDS_AT_SHUTDOWN << " seconds");
        }

        ASSERT_EQ(g_signalCount, 1);
    }

    {
        PV("Calling 'ExitUtil::shutdown(" << mqbu::ExitCode::e_SUCCESS << ")'"
                                          << " from sub-thread");

        struct local {
            static void threadFn()
            // Thread function which simply invokes 'ExitUtil::shutdown'
            {
                PVV_SAFE("Invoking shutdown");
                mqbu::ExitUtil::shutdown(mqbu::ExitCode::e_SUCCESS);
            }
        };

        bslmt::ThreadUtil::Handle threadHandle;
        bslmt::ThreadUtil::create(&threadHandle, &local::threadFn);

        const bsls::TimeInterval timeout =
            bsls::SystemTime::nowMonotonicClock().addSeconds(
                k_MAX_WAIT_SECONDS_AT_SHUTDOWN);

        rc = g_semaphore.object().timedWait(timeout);
        if (rc != 0) {
            PRINT("'ExitUtil::shutdown' failed to invoke the signal handler in"
                  << k_MAX_WAIT_SECONDS_AT_SHUTDOWN << " seconds");
        }

        ASSERT_EQ(g_signalCount, 2);
        bslmt::ThreadUtil::join(threadHandle);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 5: test5_exit_shutdown(); break;
    case 4: test4_exit_terminate(argc, argv); break;
    case 3: test3_exitCode_print(); break;
    case 2: test2_exitCode_fromAscii(); break;
    case 1: test1_exitCode_toAscii(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
