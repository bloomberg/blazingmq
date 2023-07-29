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

// mwctst_testhelper.h                                                -*-C++-*-
#ifndef INCLUDED_MWCTST_TESTHELPER
#define INCLUDED_MWCTST_TESTHELPER

//@PURPOSE: Provide macros and utilities to assist in writting test drivers.
//
//@CLASSES:
//  MACROS:             see macros definitions and descriptions below
//  mwctst::TestHelper: namespace for a set of utilities
//
//@DESCRIPTION:
// This component provides a set of macros and utilities to assist in
// writing test drivers.
//
/// TEST SHELL
///----------
// Two macros 'TEST_PROLOG' and 'TEST_EPILOG' are provided, and should
// respectively be called first and last in main.  They take care of all the
// boilerplate (such as parsing arguments, initializing ball logging,
// allocators, ensuring no default or global memory was used, ...).  Each of
// them takes a bitmask flag value (which is a combination of the flags defined
// in the 'mwctst::TestHelper::e_FLAGS' enum to control and alter what gets
// initialized and checked.
//
//: o Prolog flags:
//:   o !e_USE_STACKTRACE_ALLOCATOR!:
//:     use a 'balst::StackTraceTestAllocator' instead of a
//:     'bslma::TestAllocator' to report stacks of the memory leaks.
//:
//: o Epilog flags:
//:   o !e_CHECK_DEF_ALLOC!:
//:     verify that no memory was allocated from the default allocator during
//:     the test.
//:   o !e_CHECK_GBL_ALLOC!:
//:     verify that no memory was allocated from the global allocator during
//:     the test.
//:   o !e_CHECK_DEF_GBL_ALLOC!:
//:     verify that no memory was allocated from either the default or the
//:     global allocator during the test.
//
// NOTE: Because the Epilog flags are global and apply to all the tests in
//       the test driver, it is not always possible to enable them if one of
//       the test cases may make usage of default or global allocator.  For
//       that matter, the global variables 's_ignoreCheckDefAlloc' and
//       's_ignoreCheckGblAlloc' can be set to 'true' in the test of a given
//       case to selectively disable those checks for that test case only.
//
/// ASSERTIONS
///----------
// The following macros are provided to perform checks:
//: o Comparison macros
//:   o !ASSERT(X)!:       'X'
//:   o !ASSERT_EQ(X, Y)!: 'X == Y'
//:   o !ASSERT_NE(X, Y)!: 'X != Y'
//:   o !ASSERT_LT(X, Y)!: 'X <  Y'
//:   o !ASSERT_LE(X, Y)!: 'X <= Y'
//:   o !ASSERT_GT(X, Y)!: 'X  > Y'
//:   o !ASSERT_GE(X, Y)!: 'X >= Y'
//:   o !ASSERT_EQF(X, Y)!: 'X == Y' (fuzzy)
//:   o !ASSERT_NEF(X, Y)!: 'X != Y' (fuzzy)
//
// Each of the above macros have a '_D' variant (for example 'ASSERT_EQ_D')
// which takes an additional first parameter, 'description', that will be
// printed in case the assertion fails (but always evaluated upfront).
//
//: o Negative testing macros
//:   o !ASSERT_SAFE_PASS(E)!: no assertion in safe mode is expected
//:   o !ASSERT_SAFE_FAIL(E)!: an assertion in safe mode is expected
//:   o !ASSERT_PASS(E)!:      no assertion in default mode is expected
//:   o !ASSERT_FAIL(E)!:      an assertion in default mode is expected
//:   o !ASSERT_OPT_PASS(E)!:  no assertion in opt mode is expected
//:   o !ASSERT_OPT_FAIL(E)!:  an assertion in opt mode is expected
//
/// PRINTING
///--------
// The following macros are providing to simplify printing for tracing the
// execution of the test driver:
//: o !PRINT(X)!:
//:   print the specified expression to stdout
//: o !PRINT_(X)!:
//:   print the specified expression to stdout without '\n'
//: o !PRINT_SAFE(X)!:
//:   print the specified expression to stdout, in a thread safe manner
//: o !PRINT_SAFE_(X)!:
//:   print the specified expression to stdout, in a thread safe manner without
//:   '\n'
//: o !PV(X)!, !PVV(X)!, !PVVV(X)!:
//:   print the specified expression to stdout if the verbosity is at least
//:   set to the corresponding level
//: o !PV_SAFE(X)!, !PVV_SAFE(X)!, !PVVV_SAFE(X)!:
//:   print the specified expression to stdout, in a thread safe manner, if the
//:   verbosity is at least set to the corresponding level
//:
//: o !Q(X)!:  Quote identifier literally
//: o !P(X)!:  Print identifier and value
//: o !P_(X)!: 'P(X) without '\n'
//: o !T_!:    Print a tab (w/o newline)
//: o !L_!:    Current line number
//
/// ALLOCATORS
///----------
// 'TEST_PROLOG' registers a 'bslma::TestAllocator' for the default and global
// allocators.  During 'TEST_EPILOG', unless requested not to, the allocators
// are checked to ensure the execution of the test did not make any usage of
// either the default or the global allocators. 'TEST_PROLOG' also creates a
// 'bslma::TestAllocator' (by default, or a 'balst::StackTraceTestAllocator' if
// the 'e_USE_STACKTRACE_ALLOCATOR' flag is set) and makes it available as the
// global 's_allocator_p' pointer.  This allocator is the one that should be
// used and passed to all objects under test.  'TEST_EPILOG' does verify that
// no memory was leaked from that allocator as well.
//
/// BALL LOGGING
///------------
// The BALL infrastructure is initialized during 'TEST_PROLOG', to provide ball
// logging to stdout.  Note that the logging is done asynchronously, thus a
// separate thread is created by the test framework.
//
/// TEST REGISTRATION
///-----------------
// This component includes basic support for creating test suites in a manner
// similar to Google Test or Boost Test.  This does away with the need to
// assign indices to tests, and to write and maintain a switch/case statement
// in 'main'.
//
// Two macros are provided for creating tests:
//: o TEST(TEST_NAME)             { TEST_CODE }
//: o TEST_F(FIXTURE, TEST_NAME)  { TEST_CODE }
// Both macros register the code in the following block as a test.  'TEST_F'
// takes an extra argument, a "fixture", which is a class that provides an
// environment in which one or several tests can be conveniently executed.
// Most tests require a context for execution: objects and mocks (and sometimes
// directories or other resources) need to be created and initialized.  This is
// the role of a fixture class: it is instantiated and the test code is run as
// an instance member function of the fixture class.  Thus all the objects and
// methods supplied by the fixture class are available in the test code.  This
// approach has two advantages over simply writing the setup code at the
// beginning of the test (and cleanup at the end):
//: o If the test exits prematurely (for example on a test failure), the
//: cleanup
//:   code is executed as per the RAII idiom.
//: o The same fixture can be reused for multiple tests.
//
// Function 'mwctst::runTest()' replaces the traditional BDE switch
// statement. It runs the i-th test in order of appearance, as specified by the
// first command-line argument (extracted by 'TEST_PROLOG' into '_testCase').
// Running a test consists of the following steps:
//: o If no fixture is specified (i.e. test was created with 'TEST'):
//:   o Execute the test code
//: o If an explicit fixture is specified (i.e. test was created with
//: 'TEST_F'):
//:   o Instantiate a fixture object (execute the ctor)
//:   o Call the SetUp() method on the fixture object
//:   o Execute the test code as a (non-static) member function of the fixture
//:     class
//:   o Call the TearDown() method on the fixture
//:   o Destroy the fixture object (execute the dtor)
//
/// Usage Example
///-------------
// This section illustrates intended use of this component.
//
/// Example 1: BDE style
///- - - - - - - - - -
// Typical test driver skeleton using this 'mwctst::TestHelper' would look like
// the following:
//..
//  // grppkg_mycomponent.t.cpp                                       -*-C++-*-
//  #include <grppkg_mycomponent.h>
//
//  // TEST_DRIVER
//  #include <mwctst_testhelper.h>
//
//  // Any additional includes required by the test driver
//
//  // CONVENIENCE
//  using namespace BloombergLP;
//
//  // ========================================================================
//  //                                  TESTS
//  // ------------------------------------------------------------------------
//
//  static void test1_breathingTest() {
//      mwctst::TestHelper::printTestName("BREATHING TEST");
//
//      grppkg::MyComponent myComponent("name", s_allocator_p);
//      ASSERT(myComponent.isValid());
//      ASSERT_EQ(myComponent.name(), "name");
//  }
//
//  // ========================================================================
//  //                                  MAIN
//  // ------------------------------------------------------------------------
//
//  int main(int argc, char *argv[])
//  {
//      TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);
//
//      switch (_testCase) { case 0:
//        case 1: test1_breathingTest(); break;
//        default: {
//          bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
//                    << bsl::endl;
//          s_testStatus = -1;
//        } break;
//      }
//
//      TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
//  }
//..
//
/// Example 2: Google Test style
///- - - - - - - - - - - - - -
// Typical test driver skeleton using this 'mwctst::TestHelper' and the 'TEST'
// macros would look like this:
//..
//  // grppkg_mycomponent.t.cpp                                       -*-C++-*-
//  #include <grppkg_mycomponent.h>
//
//  // TEST_DRIVER
//  #include <mwctst_testhelper.h>
//
//  // Any additional includes required by the test driver
//
//  // CONVENIENCE
//  using namespace BloombergLP;
//
//  // ========================================================================
//  //                                  TESTS
//  // ------------------------------------------------------------------------
//
//  struct FunctionalTest : mwctst::Test {
//      bdld::Datum   config;
//      MockComponent mock;
//
//      FunctionalTest()
//      {
//        // fill config
//        // init mock
//      }
//  };
//
//  TEST_F(FunctionalTest, breathingTest)
//  {
//      grppkg::MyComponent myComponent("name", config, mock, s_allocator_p);
//      ASSERT(myComponent.isValid());
//      ASSERT_EQ(myComponent.name(), "name");
//  }
//
//  // ========================================================================
//  //                                  MAIN
//  // ------------------------------------------------------------------------
//
//  int main(int argc, char *argv[])
//  {
//      TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);
//
//      mwctst::runTest(_testCase);
//
//      TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
//  }
//..

// MWC

// BDE
#include <ball_attributecontext.h>
#include <ball_fileobserver.h>
#include <ball_loggermanager.h>
#include <ball_loggermanagerconfiguration.h>
#include <ball_multiplexobserver.h>
#include <ball_severity.h>
#include <balst_stacktracetestallocator.h>
#include <bdlsb_memoutstreambuf.h>
#include <bsl_cstddef.h>
#include <bsl_cstdio.h>
#include <bsl_cstdlib.h>
#include <bsl_iostream.h>
#include <bsl_map.h>
#include <bsl_set.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bslma_defaultallocatorguard.h>
#include <bslma_testallocator.h>
#include <bslmt_qlock.h>
#include <bsls_assert.h>
#include <bsls_asserttest.h>
#include <bsls_bsltestutil.h>
#include <bsls_platform.h>

// ============================================================================
//                       STANDARD BDE ASSERT TEST MACROS
// ----------------------------------------------------------------------------
#define ASSERT(X)                                                             \
    {                                                                         \
        ::_assert((X), #X, __FILE__, __LINE__);                               \
    }

// ============================================================================
//                       STANDARD BDE TEST DRIVER MACROS
// ----------------------------------------------------------------------------
#define Q BSLS_BSLTESTUTIL_Q    // Quote identifier literally.
#define P BSLS_BSLTESTUTIL_P    // Print identifier and value.
#define P_ BSLS_BSLTESTUTIL_P_  // 'P(X)' without '\n'.
#define T_ BSLS_BSLTESTUTIL_T_  // Print a tab (w/o newline).
#define L_ BSLS_BSLTESTUTIL_L_  // Current line number

// ============================================================================
//                      ADDITIONAL COMPARISON TEST MACROS
// ----------------------------------------------------------------------------
// Set of additional useful and more powerful test macros that check for
// equality (or other common comparison operation), and print the expression as
// well as their evaluated value in case of negative result.  The template
// method is used so that we don't double evaluate the test operands (once for
// checking whether the assertion was success and once for printing the
// values).

#define ASSERT_COMPARE_DECLARE(NAME, OP)                                      \
    template <typename P, typename T, typename U>                             \
    void _assertCompare##NAME(const P&    prefix,                             \
                              const T&    xResult,                            \
                              const U&    yResult,                            \
                              const char* xStr,                               \
                              const char* yStr,                               \
                              const char* file,                               \
                              int         line)                               \
    {                                                                         \
        if (!(xResult OP yResult)) {                                          \
            bdlsb::MemOutStreamBuf buffer(s_allocator_p);                     \
            bsl::ostream           os(&buffer);                               \
            os << prefix << "'" << xStr << "' "                               \
               << "(" << mwctst::printer(xResult) << ")"                      \
               << " " #OP " "                                                 \
               << "'" << yStr << "' "                                         \
               << "(" << mwctst::printer(yResult) << ")" << bsl::ends;        \
            bslstl::StringRef str(buffer.data(), buffer.length());            \
            BloombergLP::_assert(false, str.data(), file, line);              \
        }                                                                     \
    }

#define ASSERT_EQ(X, Y)                                                       \
    {                                                                         \
        _assertCompareEquals("", X, Y, #X, #Y, __FILE__, __LINE__);           \
    }
#define ASSERT_NE(X, Y)                                                       \
    {                                                                         \
        _assertCompareNotEquals("", X, Y, #X, #Y, __FILE__, __LINE__);        \
    }
#define ASSERT_LT(X, Y)                                                       \
    {                                                                         \
        _assertCompareLess("", X, Y, #X, #Y, __FILE__, __LINE__);             \
    }
#define ASSERT_LE(X, Y)                                                       \
    {                                                                         \
        _assertCompareLessEquals("", X, Y, #X, #Y, __FILE__, __LINE__);       \
    }
#define ASSERT_GT(X, Y)                                                       \
    {                                                                         \
        _assertCompareGreater("", X, Y, #X, #Y, __FILE__, __LINE__);          \
    }
#define ASSERT_GE(X, Y)                                                       \
    {                                                                         \
        _assertCompareGreaterEquals("", X, Y, #X, #Y, __FILE__, __LINE__);    \
    }

// '_D' variants, allowing to specify a 'description' that will be printed in
// case of failure.
#define ASSERT_D(D, X)                                                        \
    {                                                                         \
        bdlsb::MemOutStreamBuf _buf(s_allocator_p);                           \
        bsl::ostream           _os(&_buf);                                    \
        _os << D << '\0';                                                     \
        bslstl::StringRef _osStr(_buf.data(), _buf.length());                 \
        if (!X) {                                                             \
            BloombergLP::_assert(false, _osStr.data(), __FILE__, __LINE__);   \
        }                                                                     \
    }
#define ASSERT_EQ_D(D, X, Y)                                                  \
    {                                                                         \
        bdlsb::MemOutStreamBuf _buf(s_allocator_p);                           \
        bsl::ostream           _os(&_buf);                                    \
        _os << D << ": ";                                                     \
        bslstl::StringRef _osStr(_buf.data(), _buf.length());                 \
        _assertCompareEquals(_osStr, X, Y, #X, #Y, __FILE__, __LINE__);       \
    }
#define ASSERT_NE_D(D, X, Y)                                                  \
    {                                                                         \
        bdlsb::MemOutStreamBuf _buf(s_allocator_p);                           \
        bsl::ostream           _os(&_buf);                                    \
        _os << D << ": ";                                                     \
        bslstl::StringRef _osStr(_buf.data(), _buf.length());                 \
        _assertCompareNotEquals(_osStr, X, Y, #X, #Y, __FILE__, __LINE__);    \
    }
#define ASSERT_LT_D(D, X, Y)                                                  \
    {                                                                         \
        bdlsb::MemOutStreamBuf _buf(s_allocator_p);                           \
        bsl::ostream           _os(&_buf);                                    \
        _os << D << ": ";                                                     \
        bslstl::StringRef _osStr(_buf.data(), _buf.length());                 \
        _assertCompareLess(_osStr, X, Y, #X, #Y, __FILE__, __LINE__);         \
    }
#define ASSERT_LE_D(D, X, Y)                                                  \
    {                                                                         \
        bdlsb::MemOutStreamBuf _buf(s_allocator_p);                           \
        bsl::ostream           _os(&_buf);                                    \
        _os << D << ": ";                                                     \
        bslstl::StringRef _osStr(_buf.data(), _buf.length());                 \
        _assertCompareLessEquals(_osStr, X, Y, #X, #Y, __FILE__, __LINE__);   \
    }
#define ASSERT_GT_D(D, X, Y)                                                  \
    {                                                                         \
        bdlsb::MemOutStreamBuf _buf(s_allocator_p);                           \
        bsl::ostream           _os(&_buf);                                    \
        _os << D << ": ";                                                     \
        bslstl::StringRef _osStr(_buf.data(), _buf.length());                 \
        _assertCompareGreater(_osStr, X, Y, #X, #Y, __FILE__, __LINE__);      \
    }
#define ASSERT_GE_D(D, X, Y)                                                  \
    {                                                                         \
        bdlsb::MemOutStreamBuf _buf(s_allocator_p);                           \
        bsl::ostream           _os(&_buf);                                    \
        _os << D << ": ";                                                     \
        bslstl::StringRef _osStr(_buf.data(), _buf.length());                 \
        _assertCompareGreaterEquals(_osStr,                                   \
                                    X,                                        \
                                    Y,                                        \
                                    #X,                                       \
                                    #Y,                                       \
                                    __FILE__,                                 \
                                    __LINE__);                                \
    }

// Assertions using fuzzy comparisons
#define ASSERT_EQF(X, Y)                                                      \
    {                                                                         \
        if (!(mwctst::TestHelper::areFuzzyEqual((X), (Y)))) {                 \
            bsl::cout << "Error " << __FILE__ << "(" << __LINE__              \
                      << "): " << #X << " (" << mwctst::printer(X)            \
                      << ") ~= " << #Y << " (" << mwctst::printer(Y) << ")"   \
                      << "    (failed)" << bsl::endl;                         \
            if (s_testStatus >= 0 && s_testStatus <= 100)                     \
                ++s_testStatus;                                               \
        }                                                                     \
    }

#define ASSERT_NEF(X, Y)                                                      \
    {                                                                         \
        if (mwctst::TestHelper : areFuzzyEqual((X), (Y))) {                   \
            bsl::cout << "Error " << __FILE__ << "(" << __LINE__              \
                      << "): " << #X << " (" << mwctst::printer(X)            \
                      << ") !~= " << #Y << " (" << mwctst::printer(Y) << ")"  \
                      << "    (failed)" << bsl::endl;                         \
            if (s_testStatus >= 0 && s_testStatus <= 100)                     \
                ++s_testStatus;                                               \
        }                                                                     \
    }

// ============================================================================
//                     NEGATIVE-TEST MACROS ABBREVIATIONS
// ----------------------------------------------------------------------------
#define ASSERT_SAFE_PASS(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_PASS(EXPR)
#define ASSERT_SAFE_FAIL(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_FAIL(EXPR)
#define ASSERT_PASS(EXPR) BSLS_ASSERTTEST_ASSERT_PASS(EXPR)
#define ASSERT_FAIL(EXPR) BSLS_ASSERTTEST_ASSERT_FAIL(EXPR)
#define ASSERT_OPT_PASS(EXPR) BSLS_ASSERTTEST_ASSERT_OPT_PASS(EXPR)
#define ASSERT_OPT_FAIL(EXPR) BSLS_ASSERTTEST_ASSERT_OPT_FAIL(EXPR)

// ============================================================================
//                                PRINT MACROS
// ----------------------------------------------------------------------------
// The following macros may be used to print an expression 'X' at different
// levels of verbosity.  Note that 'X' is not surrounded with parentheses so
// that expressions containing output stream operations can be supported.  The
// '_SAFE' versions use a mutex to make sure concurrent output doesn't get
// interleaved.
#define PRINT(X)                                                              \
    {                                                                         \
        bsl::cout << X << bsl::endl;                                          \
    }

#define PRINT_(X)                                                             \
    {                                                                         \
        bsl::cout << X;                                                       \
    }

#define PRINT_SAFE(X)                                                         \
    {                                                                         \
        bslmt::QLockGuard qGuard(&s_serializePrintLock);                      \
        PRINT(X);                                                             \
    }

#define PRINT_SAFE_(X)                                                        \
    {                                                                         \
        bslmt::QLockGuard qGuard(&s_serializePrintLock);                      \
        PRINT_(X);                                                            \
    }

#define PV(X)                                                                 \
    if (s_verbosityLevel >= 1)                                                \
        PRINT(X);
#define PVV(X)                                                                \
    if (s_verbosityLevel >= 2)                                                \
        PRINT(X);
#define PVVV(X)                                                               \
    if (s_verbosityLevel >= 3)                                                \
        PRINT(X);

#define PV_SAFE(X)                                                            \
    if (s_verbosityLevel >= 1) {                                              \
        PRINT_SAFE(X);                                                        \
    };
#define PVV_SAFE(X)                                                           \
    if (s_verbosityLevel >= 2) {                                              \
        PRINT_SAFE(X);                                                        \
    };
#define PVVV_SAFE(X)                                                          \
    if (s_verbosityLevel >= 3) {                                              \
        PRINT_SAFE(X);                                                        \
    };

// ============================================================================
//                                 TEST SHELL
// ----------------------------------------------------------------------------
#define TEST_PROLOG(F)                                                        \
    const int _testCase = argc > 1 ? atoi(argv[1]) : 0;                       \
    s_verbosityLevel    = argc - 2;                                           \
                                                                              \
    /* Install an assert handler to gracefully mark the test as failure */    \
    /* in case of assert.                                               */    \
    bsls::AssertFailureHandlerGuard _assertGuard(::_assertViolationHandler);  \
                                                                              \
    /* Initialize BALL */                                                     \
    /* NOTE: BALL mechanisms use the default allocator, that is why */        \
    /*       logging is initialized before the allocators.          */        \
    INIT_BALL_LOGGING();                                                      \
                                                                              \
    /* Initialize allocators */                                               \
    INIT_ALLOCATORS(F);                                                       \
                                                                              \
    bsl::cout << "TEST " << __FILE__ << " CASE " << _testCase << bsl::endl;   \
                                                                              \
    /* Create a scope for all code in between TEST_PROLOG and TEST_EPILOG */  \
    {
#define INIT_BALL_LOGGING()                                                   \
    /* create logger configuration */                                         \
    ball::LoggerManagerConfiguration _logConfig;                              \
    ball::Severity::Level            _logSeverity = ball::Severity::WARN;     \
    {                                                                         \
        if (s_verbosityLevel == 1) {                                          \
            _logSeverity = ball::Severity::INFO;                              \
        }                                                                     \
        else if (s_verbosityLevel == 2) {                                     \
            _logSeverity = ball::Severity::DEBUG;                             \
        }                                                                     \
        else if (s_verbosityLevel >= 3) {                                     \
            _logSeverity = ball::Severity::TRACE;                             \
        }                                                                     \
        _logConfig.setDefaultThresholdLevelsIfValid(ball::Severity::OFF,      \
                                                    _logSeverity,             \
                                                    ball::Severity::OFF,      \
                                                    ball::Severity::OFF);     \
    }                                                                         \
                                                                              \
    ball::AttributeContextProctor _attributeContextProctor;                   \
                                                                              \
    /* create a multiplex observer to register other observers */             \
    ball::MultiplexObserver _logMultiplexObserver;                            \
                                                                              \
    /* create and register an observer that writes to stdout */               \
    ball::FileObserver _logStdoutObserver(_logSeverity);                      \
    _logMultiplexObserver.registerObserver(&_logStdoutObserver);              \
                                                                              \
    /* initialize logger manager */                                           \
    ball::LoggerManagerScopedGuard _logManagerGuard(&_logMultiplexObserver,   \
                                                    _logConfig);

#define INIT_GLOBAL_ALLOCATOR_INTERNAL()                                      \
    /* Global Allocator */                                                    \
    /* NOTE: The global allocator has a static storage duration to outlive */ \
    /*       all static objects using that allocator.                      */ \
    static bslma::TestAllocator _gblAlloc("global", (s_verbosityLevel >= 4)); \
    bslma::Default::setGlobalAllocator(&_gblAlloc);

#ifdef BSLS_PLATFORM_CMP_CLANG
/* Suppress "exit-time-destructor" warning on Clang by qualifying the */
/* static variable '_gblAlloc' with Clang-specific attribute.         */
#define INIT_GLOBAL_ALLOCATOR()                                               \
    [[clang::no_destroy]] INIT_GLOBAL_ALLOCATOR_INTERNAL()
#else
#define INIT_GLOBAL_ALLOCATOR() INIT_GLOBAL_ALLOCATOR_INTERNAL()
#endif

#define INIT_ALLOCATORS(F)                                                      \
    INIT_GLOBAL_ALLOCATOR();                                                    \
                                                                                \
    /* Default allocator */                                                     \
    bslma::TestAllocator         _defAlloc("default", (s_verbosityLevel >= 4)); \
    bslma::DefaultAllocatorGuard _defAllocGuard(&_defAlloc);                    \
                                                                                \
    /* Test driver allocator */                                                 \
    bslma::TestAllocator _testAlloc("test", (s_verbosityLevel >= 4));           \
    _testAlloc.setNoAbort(true);                                                \
                                                                                \
    balst::StackTraceTestAllocator _stTestAlloc;                                \
    _stTestAlloc.setName("test");                                               \
                                                                                \
    if ((F)&mwctst::TestHelper::e_USE_STACKTRACE_ALLOCATOR) {                   \
        s_allocator_p = &_stTestAlloc;                                          \
    }                                                                           \
    else {                                                                      \
        s_allocator_p = &_testAlloc;                                            \
    }

#define TEST_EPILOG(F)                                                        \
    /* Close the scope for all code in between TEST_PROLOG and TEST_EPILOG */ \
    }                                                                         \
                                                                              \
    /* Ensure no memory leak from the component under test */                 \
    ASSERT_EQ(_testAlloc.numBlocksInUse(), 0);                                \
                                                                              \
    /* Verify no default allocator usage */                                   \
    if (F & mwctst::TestHelper::e_CHECK_DEF_ALLOC &&                          \
        !s_ignoreCheckDefAlloc) {                                             \
        ASSERT_EQ(_defAlloc.numBlocksTotal(), 0);                             \
    }                                                                         \
                                                                              \
    /* Verify no global allocator usage */                                    \
    if (F & mwctst::TestHelper::e_CHECK_GBL_ALLOC &&                          \
        !s_ignoreCheckGblAlloc) {                                             \
        ASSERT_EQ(_gblAlloc.numBlocksTotal(), 0);                             \
    }                                                                         \
                                                                              \
    /* shutdown log observers */                                              \
    _logMultiplexObserver.deregisterObserver(&_logStdoutObserver);            \
                                                                              \
    /* Check test result */                                                   \
    if (s_testStatus > 0) {                                                   \
        bsl::cerr << "Error, non-zero test status: " << s_testStatus << "."   \
                  << bsl::endl;                                               \
    }                                                                         \
                                                                              \
    s_allocator_p = 0; /* clang-tidy warning silencing */                     \
    return s_testStatus;

#define TEST_F(FIXTURE, NAME)                                                 \
    struct FIXTURE##NAME : FIXTURE {                                          \
        void body() BSLS_KEYWORD_OVERRIDE;                                    \
                                                                              \
        static void run()                                                     \
        {                                                                     \
            FIXTURE##NAME test;                                               \
            mwctst::TestHelper::printTestName(#NAME);                         \
            test.SetUp();                                                     \
            test.body();                                                      \
            test.TearDown();                                                  \
        }                                                                     \
                                                                              \
        static ::BloombergLP::mwctst::TestHelper_Test s_testItem;             \
    };                                                                        \
                                                                              \
    ::BloombergLP::mwctst::TestHelper_Test FIXTURE##NAME ::s_testItem(        \
        FIXTURE##NAME ::run);                                                 \
    void FIXTURE##NAME ::body()

#define TEST(NAME)                                                            \
    struct Test##NAME : ::BloombergLP::mwctst::Test {                         \
        void body() BSLS_KEYWORD_OVERRIDE;                                    \
                                                                              \
        static void run()                                                     \
        {                                                                     \
            Test##NAME test;                                                  \
            mwctst::TestHelper::printTestName(#NAME);                         \
            test.SetUp();                                                     \
            test.body();                                                      \
            test.TearDown();                                                  \
        }                                                                     \
                                                                              \
        static ::BloombergLP::mwctst::TestHelper_Test s_testItem;             \
    };                                                                        \
                                                                              \
    ::BloombergLP::mwctst::TestHelper_Test Test##NAME ::s_testItem(           \
        Test##NAME ::run);                                                    \
    void Test##NAME ::body()

/*Define benchmarking macros*/
#ifdef BSLS_PLATFORM_OS_LINUX
#define MWC_BENCHMARK_WITH_ARGS(BM_NAME, ARGS)                                \
    BENCHMARK(BM_NAME##_GoogleBenchmark)->ARGS;
#define MWC_BENCHMARK(BM_NAME) BENCHMARK(BM_NAME##_GoogleBenchmark);
#else  // !BSLS_PLATFORM_OS_LINUX
#define MWC_BENCHMARK(BM_NAME) BM_NAME();
#define MWC_BENCHMARK_WITH_ARGS(BM_NAME, ARGS) BM_NAME();

#endif  // BSLS_PLATFORM_OS_LINUX

namespace BloombergLP {
namespace mwctst {

// FORWARD DECLARATION
template <typename TYPE>
class TestHelper_Printer;

template <typename TYPE>
TestHelper_Printer<TYPE> printer(const TYPE& obj);

}

// ============================================================================
//                              GLOBAL VARIABLES
// ----------------------------------------------------------------------------

/// Result of the test:  0: success
///                     >0: number of errors
///                     -1: no such test
extern int s_testStatus;

/// Verbosity to use ([0..4], the higher the more verbose).
extern int s_verbosityLevel;

/// Global flag which can be set to ignore checking the default allocator
/// usage for a specific test case.
extern bool s_ignoreCheckDefAlloc;

/// Global flag which can be set to ignore checking the global allocator
/// usage for a specific test case.
extern bool s_ignoreCheckGblAlloc;

/// Lock mechanism to serialize output in.
extern bslmt::QLock s_serializePrintLock;

/// Allocator to use by the components under test.
extern bslma::Allocator* s_allocator_p;

// ============================================================================
//                              GLOBAL FUNCTIONS
// ----------------------------------------------------------------------------

/// "TestDriver" version of an assert method for the specified `result`,
/// with the specified `expression` from the specified `file` and `line`.
static inline void
_assert(bool result, const char* expression, const char* file, int line)
{
    if (!result) {
        printf("Error %s(%d): %s    (failed)\n", file, line, expression);
        if (s_testStatus >= 0 && s_testStatus <= 100) {
            ++s_testStatus;
        }
    }
}

/// A handler to be invoked on BDE assertion violation (see `bsls_assert`).
/// Prints the error and calls
/// `bsls::AssertTest::failTestDriver(violation)`.
BSLS_ANNOTATION_NORETURN
static inline void
_assertViolationHandler(const bsls::AssertViolation& violation)
{
    printf("Error %s(%d): %s    (failed)\n",
           violation.fileName(),
           violation.lineNumber(),
           violation.comment());

    bsls::AssertTest::failTestDriver(violation);
}

// Create a definition of the assert template method for each of the 6 common
// comparison operators.
ASSERT_COMPARE_DECLARE(Equals, ==)
ASSERT_COMPARE_DECLARE(NotEquals, !=)
ASSERT_COMPARE_DECLARE(Less, <)
ASSERT_COMPARE_DECLARE(LessEquals, <=)
ASSERT_COMPARE_DECLARE(Greater, >)
ASSERT_COMPARE_DECLARE(GreaterEquals, >=)

namespace mwctst {

// ======================
// struct TestHelper_Test
// ======================

/// Class for registering tests.
struct TestHelper_Test {
    // TYPES
    typedef void (*TestFn)();

    // PUBLIC CLASS DATA
    static const int k_MAX_TESTS = 512;

    static TestFn s_tests[k_MAX_TESTS];

    static int s_numTests;

    // CREATORS
    TestHelper_Test(const TestFn& test);
};

// ========================
// class TestHelper_Printer
// ========================

/// Printer for arbitrary types, with output operator specialized for some
/// types without an output operator
template <typename TYPE>
class TestHelper_Printer {
  private:
    // PRIVATE DATA
    const TYPE* d_obj_p;

  public:
    // CREATORS
    explicit TestHelper_Printer(const TYPE* obj)
    : d_obj_p(obj)
    {
        // NOTHING
    }

  public:
    // ACCESSORS
    const TYPE& obj() const;
};

// =================
// struct TestHelper
// =================

/// Namespace for a set of utilities.
struct TestHelper {
    // TYPES
    enum e_FLAGS {
        /// Flags to provide to `TEST_PROLOG` and `TEST_EPILOG` macros.
        e_DEFAULT = 0

        // PROLOG FLAGS
        ,
        e_USE_STACKTRACE_ALLOCATOR = 1 << 0

        // EPILOG FLAGS
        ,
        e_CHECK_DEF_ALLOC     = 1 << 0,
        e_CHECK_GBL_ALLOC     = 1 << 1,
        e_CHECK_DEF_GBL_ALLOC = e_CHECK_DEF_ALLOC | e_CHECK_GBL_ALLOC
    };

    // CLASS METHODS

    /// Print the banner of the name of the test, in the specified `value`.
    static void printTestName(bslstl::StringRef value);

    /// Return true if the specified `x` and `y` should be considered equal,
    /// with respect to floating numerics imprecision.
    static bool areFuzzyEqual(double x, double y);
};

// ==========
// class Test
// ==========

/// Default class for tests, and base class for tests that require a
/// `fixture`, i.e. a context for their execution.
class Test {
  public:
    // CREATORS
    virtual ~Test();

    // MANIPULATORS
    virtual void body() = 0;

    /// Perform initializations prior to running the test.
    virtual void SetUp();

    /// Perform cleanup after running the test.
    virtual void TearDown();
};

// FREE FUNCTIONS

/// Run the index-th test.
void runTest(int index);

/// Return a printer for the specified `obj`.
template <typename TYPE>
TestHelper_Printer<TYPE> printer(const TYPE& obj);

/// Print the contents of the specified `printer` into the specified output
/// `stream`.
template <typename TYPE>
bsl::ostream& operator<<(bsl::ostream&                   stream,
                         const TestHelper_Printer<TYPE>& printer);
template <typename TYPE>
bsl::ostream&
operator<<(bsl::ostream&                                 stream,
           const TestHelper_Printer<bsl::vector<TYPE> >& printer);
template <typename TYPE1, typename TYPE2>
bsl::ostream&
operator<<(bsl::ostream&                                      stream,
           const TestHelper_Printer<bsl::map<TYPE1, TYPE2> >& printer);
template <typename TYPE>
bsl::ostream& operator<<(bsl::ostream&                              stream,
                         const TestHelper_Printer<bsl::set<TYPE> >& printer);
template <typename TYPE1, typename TYPE2>
bsl::ostream& operator<<(
    bsl::ostream&                                                stream,
    const TestHelper_Printer<bsl::unordered_map<TYPE1, TYPE2> >& printer);
template <typename TYPE>
bsl::ostream&
operator<<(bsl::ostream&                                        stream,
           const TestHelper_Printer<bsl::unordered_set<TYPE> >& printer);
template <typename TYPE1, typename TYPE2>
bsl::ostream&
operator<<(bsl::ostream&                                       stream,
           const TestHelper_Printer<bsl::pair<TYPE1, TYPE2> >& printer);

// ============================================================================
//                      INLINE FUNCTION IMPLEMENTATIONS
// ============================================================================

// ------------------------
// class TestHelper_Printer
// ------------------------

// ACCESSORS
template <typename TYPE>
inline const TYPE& TestHelper_Printer<TYPE>::obj() const
{
    return *d_obj_p;
}

}  // close package namespace

// FREE FUNCTIONS
template <typename TYPE>
inline mwctst::TestHelper_Printer<TYPE> mwctst::printer(const TYPE& obj)
{
    return TestHelper_Printer<TYPE>(&obj);
}

template <typename TYPE>
inline bsl::ostream&
mwctst::operator<<(bsl::ostream&                   stream,
                   const TestHelper_Printer<TYPE>& printer)
{
    return stream << printer.obj();
}

template <typename TYPE>
inline bsl::ostream&
mwctst::operator<<(bsl::ostream&                                 stream,
                   const TestHelper_Printer<bsl::vector<TYPE> >& printer)
{
    stream << "[";

    if (!printer.obj().empty()) {
        for (size_t i = 0; i < printer.obj().size() - 1; ++i) {
            stream << TestHelper_Printer<TYPE>(&printer.obj()[i]) << ", ";
        }

        stream << TestHelper_Printer<TYPE>(&printer.obj().back());
    }

    stream << "]";
    return stream;
}

template <typename TYPE1, typename TYPE2>
inline bsl::ostream&
mwctst::operator<<(bsl::ostream&                                      stream,
                   const TestHelper_Printer<bsl::map<TYPE1, TYPE2> >& printer)
{
    stream << "{";

    if (!printer.obj().empty()) {
        typedef typename bsl::map<TYPE1, TYPE2>::const_iterator Iter;
        Iter iter     = printer.obj().begin();
        Iter lastElem = --printer.obj().end();
        for (; iter != lastElem; ++iter) {
            stream << TestHelper_Printer<TYPE1>(&iter->first) << ":"
                   << TestHelper_Printer<TYPE2>(&iter->second) << ", ";
        }

        stream << TestHelper_Printer<TYPE1>(&iter->first) << ":"
               << TestHelper_Printer<TYPE2>(&iter->second);
    }

    stream << "}";
    return stream;
}

template <typename TYPE>
inline bsl::ostream&
mwctst::operator<<(bsl::ostream&                              stream,
                   const TestHelper_Printer<bsl::set<TYPE> >& printer)
{
    stream << "{";

    if (!printer.obj().empty()) {
        typedef typename bsl::set<TYPE>::const_iterator Iter;
        Iter iter     = printer.obj().begin();
        Iter lastElem = --printer.obj().end();
        for (; iter != lastElem; ++iter) {
            stream << TestHelper_Printer<TYPE>(&(*iter)) << ", ";
        }

        stream << TestHelper_Printer<TYPE>(&(*iter));
    }

    stream << "}";
    return stream;
}

template <typename TYPE>
inline bsl::ostream& mwctst::operator<<(
    bsl::ostream&                                        stream,
    const TestHelper_Printer<bsl::unordered_set<TYPE> >& printer)
{
    stream << "{";

    if (!printer.obj().empty()) {
        typedef typename bsl::unordered_set<TYPE>::const_iterator Iter;
        Iter begin = printer.obj().begin();
        Iter end   = printer.obj().end();
        for (Iter iter = begin; iter != end; ++iter) {
            if (iter != begin) {
                stream << ", ";
            }
            stream << TestHelper_Printer<TYPE>(&(*iter));
        }
    }

    stream << "}";
    return stream;
}

template <typename TYPE1, typename TYPE2>
inline bsl::ostream& mwctst::operator<<(
    bsl::ostream&                                                stream,
    const TestHelper_Printer<bsl::unordered_map<TYPE1, TYPE2> >& printer)
{
    stream << "{";

    typedef typename bsl::unordered_map<TYPE1, TYPE2>::const_iterator Iter;
    Iter begin = printer.obj().begin();
    Iter end   = printer.obj().end();
    for (Iter iter = begin; iter != end; ++iter) {
        if (iter != begin) {
            stream << ", ";
        }

        stream << TestHelper_Printer<TYPE1>(&iter->first) << ":"
               << TestHelper_Printer<TYPE2>(&iter->second);
    }

    stream << "}";
    return stream;
}

template <typename TYPE1, typename TYPE2>
inline bsl::ostream&
mwctst::operator<<(bsl::ostream&                                       stream,
                   const TestHelper_Printer<bsl::pair<TYPE1, TYPE2> >& printer)
{
    return stream << '<' << printer.obj().first << ", " << printer.obj().second
                  << '>';
}

}  // close enterprise namespace

#endif
