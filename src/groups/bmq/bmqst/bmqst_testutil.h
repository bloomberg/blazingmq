// Copyright 2022-2023 Bloomberg Finance L.P.
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

// bmqst_testutil.h -*-C++-*-
#ifndef INCLUDED_BMQST_TESTUTIL
#define INCLUDED_BMQST_TESTUTIL

//@PURPOSE: Provide non-standard macros and utilities for use in test drivers.
//
//@CLASSES:
// bmqst::TestUtil         : utility funcions for test drivers
// bmqst::BlobDataComparer : compares and pretty-print two blobs
//
//@DESCRIPTION: This component defines macros and utilities useful in test
// drivers.
//
// Note: The assertion macros will only work in a test driver as they assumes
// the presence of a 'testStatus' variable.

#include <ball_fileobserver.h>
#include <ball_loggermanager.h>
#include <ball_loggermanagerconfiguration.h>
#include <bdlbb_blob.h>
#include <bmqu_blob.h>
#include <bmqu_printutil.h>
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bsl_vector.h>
#include <bsls_asserttest.h>
#include <bsls_bsltestutil.h>

//=============================================================================
//                        NON-STANDARD ASSERTION MACROS
//-----------------------------------------------------------------------------

// Asserts like the standard 'ASSERT', but skips instead of failing and 'SKIP'
// is true.
#define ASSERT_SKIP(X, SKIP)                                                  \
    {                                                                         \
        if (!(X)) {                                                           \
            bsl::cout << "Error " << __FILE__ << "(" << __LINE__              \
                      << "): " << #X << "    (failed)" << bsl::endl;          \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }

// Asserts that X == Y
#define ASSERT_EQUALS_SKIP(X, Y, SKIP)                                        \
    {                                                                         \
        if (!((X) == (Y))) {                                                  \
            bsl::cout << "Error " << __FILE__ << "(" << __LINE__              \
                      << "): " << #X << " (" << bmqu::PrintUtil::printer(X)   \
                      << ") == " << #Y << " (" << bmqu::PrintUtil::printer(Y) \
                      << ")" << "    (failed)" << bsl::endl;                  \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define ASSERT_EQUALS(X, Y) ASSERT_EQUALS_SKIP(X, Y, false)

// Asserts that X != Y
#define ASSERT_NOT_EQUALS_SKIP(X, Y, SKIP)                                    \
    {                                                                         \
        if ((X) == (Y)) {                                                     \
            bsl::cout << "Error " << __FILE__ << "(" << __LINE__              \
                      << "): " << #X << " (" << bmqu::PrintUtil::printer(X)   \
                      << ") != " << #Y << " (" << bmqu::PrintUtil::printer(Y) \
                      << ")" << "    (failed)" << bsl::endl;                  \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define ASSERT_NOT_EQUALS(X, Y) ASSERT_NOT_EQUALS_SKIP(X, Y, false)

// Loop version of ASSERT_EQUALS
#define LOOP_ASSERT_EQUALS_SKIP(I, X, Y, SKIP)                                \
    {                                                                         \
        if (!((X) == (Y))) {                                                  \
            cout << #I << ": " << bmqu::PrintUtil::printer(I) << "\n"         \
                 << "Error " << __FILE__ << "(" << __LINE__ << "): " << #X    \
                 << " (" << bmqu::PrintUtil::printer(X) << ") == " << #Y      \
                 << " (" << bmqu::PrintUtil::printer(Y) << ")"                \
                 << "    (failed)" << endl;                                   \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define LOOP_ASSERT_EQUALS(I, X, Y) LOOP_ASSERT_EQUALS_SKIP(I, X, Y, false)

// Loop version of ASSERT_NOT_EQUALS
#define LOOP_ASSERT_NOT_EQUALS(I, X, Y)                                       \
    {                                                                         \
        if ((X) == (Y)) {                                                     \
            cout << #I << ": " << bmqu::PrintUtil::printer(I) << "\n"         \
                 << "Error " << __FILE__ << "(" << __LINE__ << "): " << #X    \
                 << " (" << bmqu::PrintUtil::printer(X) << ") == " << #Y      \
                 << " (" << bmqu::PrintUtil::printer(Y) << ")"                \
                 << "    (failed)" << endl;                                   \
            if (testStatus >= 0 && testStatus <= 100)                         \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define LOOP2_ASSERT_EQUALS(I, J, X, Y)                                       \
    {                                                                         \
        if (!((X) == (Y))) {                                                  \
            cout << #I << ": " << bmqu::PrintUtil::printer(I) << "\n"         \
                 << #J << ": " << bmqu::PrintUtil::printer(J) << "\n"         \
                 << "Error " << __FILE__ << "(" << __LINE__ << "): " << #X    \
                 << " (" << bmqu::PrintUtil::printer(X) << ") == " << #Y      \
                 << " (" << bmqu::PrintUtil::printer(Y) << ")"                \
                 << "    (failed)" << endl;                                   \
            if (testStatus >= 0 && testStatus <= 100)                         \
                ++testStatus;                                                 \
        }                                                                     \
    }

// Asserts that X < Y
#define ASSERT_LESS_SKIP(X, Y, SKIP)                                          \
    {                                                                         \
        if (!((X) < (Y))) {                                                   \
            bsl::cout << "Error " << __FILE__ << "(" << __LINE__              \
                      << "): " << #X << " (" << bmqu::PrintUtil::printer(X)   \
                      << ") < " << #Y << " (" << bmqu::PrintUtil::printer(Y)  \
                      << ")" << "    (failed)" << bsl::endl;                  \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define ASSERT_LESS(X, Y) ASSERT_LESS_SKIP(X, Y, false)

// Loop version of ASSERT_LESS
#define LOOP_ASSERT_LESS_SKIP(I, X, Y, SKIP)                                  \
    {                                                                         \
        if (!((X) < (Y))) {                                                   \
            cout << #I << ": " << bmqu::PrintUtil::printer(I) << "\n"         \
                 << "Error " << __FILE__ << "(" << __LINE__ << "): " << #X    \
                 << " (" << bmqu::PrintUtil::printer(X) << ") < " << #Y       \
                 << " (" << bmqu::PrintUtil::printer(Y) << ")"                \
                 << "    (failed)" << endl;                                   \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define LOOP2_ASSERT_LESS_SKIP(I, J, X, Y, SKIP)                              \
    {                                                                         \
        if (!((X) < (Y))) {                                                   \
            cout << #I << ": " << bmqu::PrintUtil::printer(I) << "\n"         \
                 << #J << ": " << bmqu::PrintUtil::printer(J) << "\n"         \
                 << "Error " << __FILE__ << "(" << __LINE__ << "): " << #X    \
                 << " (" << bmqu::PrintUtil::printer(X) << ") < " << #Y       \
                 << " (" << bmqu::PrintUtil::printer(Y) << ")"                \
                 << "    (failed)" << endl;                                   \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define LOOP_ASSERT_LESS(I, X, Y) LOOP_ASSERT_LESS_SKIP(I, X, Y, false)
#define LOOP2_ASSERT_LESS(I, J, X, Y) LOOP2_ASSERT_LESS_SKIP(I, J, X, Y, false)

// Asserts that X <= Y
#define ASSERT_LE_SKIP(X, Y, SKIP)                                            \
    {                                                                         \
        if (!((X) <= (Y))) {                                                  \
            bsl::cout << "Error " << __FILE__ << "(" << __LINE__              \
                      << "): " << #X << " (" << bmqu::PrintUtil::printer(X)   \
                      << ") <= " << #Y << " (" << bmqu::PrintUtil::printer(Y) \
                      << ")" << "    (failed)" << bsl::endl;                  \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define ASSERT_LE(X, Y) ASSERT_LE_SKIP(X, Y, false)

// Loop version of ASSERT_LE
#define LOOP_ASSERT_LE_SKIP(I, X, Y, SKIP)                                    \
    {                                                                         \
        if (!((X) <= (Y))) {                                                  \
            cout << #I << ": " << bmqu::PrintUtil::printer(I) << "\n"         \
                 << "Error " << __FILE__ << "(" << __LINE__ << "): " << #X    \
                 << " (" << bmqu::PrintUtil::printer(X) << ") <= " << #Y      \
                 << " (" << bmqu::PrintUtil::printer(Y) << ")"                \
                 << "    (failed)" << endl;                                   \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define LOOP2_ASSERT_LE_SKIP(I, J, X, Y, SKIP)                                \
    {                                                                         \
        if (!((X) <= (Y))) {                                                  \
            cout << #I << ": " << bmqu::PrintUtil::printer(I) << "\n"         \
                 << #J << ": " << bmqu::PrintUtil::printer(J) << "\n"         \
                 << "Error " << __FILE__ << "(" << __LINE__ << "): " << #X    \
                 << " (" << bmqu::PrintUtil::printer(X) << ") <= " << #Y      \
                 << " (" << bmqu::PrintUtil::printer(Y) << ")"                \
                 << "    (failed)" << endl;                                   \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define LOOP_ASSERT_LE(I, X, Y) LOOP_ASSERT_LE_SKIP(I, X, Y, false)
#define LOOP2_ASSERT_LE(I, J, X, Y) LOOP2_ASSERT_LE_SKIP(I, J, X, Y, false)

// Asserts that X >= Y
#define ASSERT_GE_SKIP(X, Y, SKIP)                                            \
    {                                                                         \
        if (!((X) >= (Y))) {                                                  \
            bsl::cout << "Error " << __FILE__ << "(" << __LINE__              \
                      << "): " << #X << " (" << bmqu::PrintUtil::printer(X)   \
                      << ") >= " << #Y << " (" << bmqu::PrintUtil::printer(Y) \
                      << ")" << "    (failed)" << bsl::endl;                  \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define ASSERT_GE(X, Y) ASSERT_GE_SKIP(X, Y, false)

// Loop version of ASSERT_LE
#define LOOP_ASSERT_GE_SKIP(I, X, Y, SKIP)                                    \
    {                                                                         \
        if (!((X) >= (Y))) {                                                  \
            cout << #I << ": " << bmqu::PrintUtil::printer(I) << "\n"         \
                 << "Error " << __FILE__ << "(" << __LINE__ << "): " << #X    \
                 << " (" << bmqu::PrintUtil::printer(X) << ") >= " << #Y      \
                 << " (" << bmqu::PrintUtil::printer(Y) << ")"                \
                 << "    (failed)" << endl;                                   \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define LOOP2_ASSERT_GE_SKIP(I, J, X, Y, SKIP)                                \
    {                                                                         \
        if (!((X) >= (Y))) {                                                  \
            cout << #I << ": " << bmqu::PrintUtil::printer(I) << "\n"         \
                 << #J << ": " << bmqu::PrintUtil::printer(J) << "\n"         \
                 << "Error " << __FILE__ << "(" << __LINE__ << "): " << #X    \
                 << " (" << bmqu::PrintUtil::printer(X) << ") >= " << #Y      \
                 << " (" << bmqu::PrintUtil::printer(Y) << ")"                \
                 << "    (failed)" << endl;                                   \
            if ((SKIP))                                                       \
                testStatus = 254;                                             \
            else if (testStatus >= 0 && testStatus <= 100)                    \
                ++testStatus;                                                 \
        }                                                                     \
    }
#define LOOP_ASSERT_GE(I, X, Y) LOOP_ASSERT_GE_SKIP(I, X, Y, false)
#define LOOP2_ASSERT_GE(I, J, X, Y) LOOP2_ASSERT_GE_SKIP(I, J, X, Y, false)

//=============================================================================
//                                OTHER MACROS
//-----------------------------------------------------------------------------

// Initialize BALL logging based on verbosity command line arguments
#define INIT_BALL_LOGGING_VERBOSITY(VERBOSE, VERY_VERBOSE)                    \
    ball::FileObserver observer;                                              \
    observer.disableFileLogging();                                            \
    const bsl::string logFormat = "%d (%t) %c %s [%F:%l] %m\n\n";             \
    observer.setLogFormat(logFormat.c_str(), logFormat.c_str());              \
    ball::Severity::Level ballLevel = ball::Severity::e_WARN;                 \
    if (VERBOSE)                                                              \
        ballLevel = ball::Severity::e_INFO;                                   \
    if (VERY_VERBOSE)                                                         \
        ballLevel = ball::Severity::e_TRACE;                                  \
    observer.setStdoutThreshold(ballLevel);                                   \
    ball::LoggerManagerConfiguration config;                                  \
    config.setDefaultThresholdLevelsIfValid(ball::Severity::e_OFF,            \
                                            ballLevel,                        \
                                            ball::Severity::e_OFF,            \
                                            ball::Severity::e_OFF);           \
    ball::LoggerManagerScopedGuard manager(&observer, config);

//=============================================================================
//                                UTILITIES
//-----------------------------------------------------------------------------

namespace BloombergLP {
namespace bmqst {

// ===============
// struct TestUtil
// ===============

/// Struct containing various functions useful in test drivers.
struct TestUtil {
    // CLASS METHODS
    static bsl::vector<int> intVector(const char* nums);

    /// Return a vector of integers defined by the specified `nums`, which
    /// is a space separated list of either integers, `--`, or `++`.  `--`
    /// will be replaced with the minimum possible value for the type, and
    /// `++` will be replaced with the maximum value.
    static bsl::vector<bsls::Types::Int64> int64Vector(const char* nums);

    /// Return a vector of strings generated by the specified `s`, which is
    /// a space-separated list of strings to include.  An entry of `-` will
    /// produce an empty string.
    static bsl::vector<bsl::string> stringVector(const char* s);

    /// Load the specified `output` string with the content of the specified
    /// `input` containing hexadecimal numbers.  For example "48656C6C6F"
    /// will load `output` with "Hello".  The behavior is undefined unless
    /// `input` is a valid hex string.
    static void hexToString(bsl::string*             output,
                            const bslstl::StringRef& input);

    /// Return true if this test is running in Jenkins, by testing if the
    /// environment variable "JENKINS" is defined.  Return false otherwise.
    static bool testRunningInJenkins();

    /// Print the result of the test case using the specified `testStatus`
    /// and `verbose` flag.  This function prints test passed if
    /// `testStatus` is zero, or test skipped if `testStatus` is 254, or
    /// the number of failed assertions otherwise.  Note that the standard
    /// `testStatus` values are 0 on success, a positive value to indicate
    /// the number of assertion failures, or a negative value to indicate
    /// that the test case was not found.
    static void printTestStatus(int testStatus, int verbose);
};

// ======================
// class BlobDataComparer
// ======================

/// Compares the data in 2 blobs, and pretty-prints the two blobs if they
/// are found not to be equal.
///
/// DEPRECATED. Use `bmqu::BlobComparator` instead.
class BlobDataComparer {
  private:
    // DATA
    const bdlbb::Blob* d_blob_p;  // held, not owned: blob to compare

    int d_offset;  // offset in the blob to compare

    bslma::Allocator* d_allocator_p;  // held, not owned

  public:
    // CREATORS

    /// Create a `BlobDataComparer` initialized with the specfied `blob`.
    /// Optionally specify an `allocator` used to supply memory.  If
    /// `allocator` is null, the default allocator is used.
    explicit BlobDataComparer(const bdlbb::Blob* blob,
                              bslma::Allocator*  allocator = 0);

    /// Create a `BlobDataComparer` initialized with the specfied `blob` and
    /// `start` position within that `blob`.  Optionally specify an
    /// `allocator` used to supply memory.  If `allocator` is null, the
    /// default allocator is used.  The behavior is undefined unless `start`
    /// is a valid position in `blob`.
    BlobDataComparer(const bdlbb::Blob*        blob,
                     const bmqu::BlobPosition& start,
                     bslma::Allocator*         allocator = 0);

    // ACCESSORS

    /// Return a pointer to the blob referenced by this object.
    const bdlbb::Blob* blob() const;

    /// Return the offset in the blob (a number of bytes).
    int offset() const;

    /// Return the allocator held by this object.
    bslma::Allocator* allocator() const;
};

// FREE FUNCTIONS
bool          operator==(const BlobDataComparer&, const BlobDataComparer&);
bsl::ostream& operator<<(bsl::ostream&, const BlobDataComparer&);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

}  // close package namespace
}  // close enterprise namespace

#endif
