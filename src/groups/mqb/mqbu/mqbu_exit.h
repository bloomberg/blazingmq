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

// mqbu_exit.h                                                        -*-C++-*-
#ifndef INCLUDED_MQBU_EXIT
#define INCLUDED_MQBU_EXIT

//@PURPOSE: Provide enumerations and methods needed for exiting bmqbrkr.
//
//@CLASSES:
//  mqbu::ExitCode: various bmqbrkr exit code values
//  mqbu::ExitUtil: helper functions to shutdown bmqbrkr
//
//@DESCRIPTION: This file contains an enum, 'mqbu::ExitCode', representing the
// list of all possible exit codes from bmqbrkr; whether returned directly in
// 'main()' or through a call to 'bsl::exit(n)'.  It also contains a utility
// namespace, 'mqbu::ExitUtil' allowing graceful and ungraceful shutdown of the
// broker.
//

// MQB

// BDE
#include <ball_log.h>
#include <bsl_iosfwd.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace mqbu {

// ===============
// struct ExitCode
// ===============

/// This enum represents various exit code of bmqbrkr
struct ExitCode {
  public:
    // TYPES
    enum Enum {
        e_SUCCESS = 0  // Clean exit

        ,
        e_COMMAND_LINE = 1  // Error while parsing the command line
                            // parameters, this typically mean that an
                            // invalid argument was provided.

        ,
        e_CONFIG_GENERATION = 2  // Error while generating the
                                 // configuration, or parsing the generated
                                 // configuration: this could mean the
                                 // python failed to execute, the generated
                                 // output was invalid JSON syntax, or
                                 // didn't match the expected schema.

        ,
        e_TASK_INITIALIZE = 3  // Failed to initialize the task.  Possible
                               // cause are invalid configuration, failed
                               // to create threads, unable to open the
                               // pipe control channel, ..

        ,
        e_BENCH_START = 4  // Unable to start the bench application.

        ,
        e_APP_INITIALIZE = 5  // Failed to initialize the application.

        ,
        e_RUN = 6  // Failed to start the application (many
                   // reasons such as failed to listen to TCP
                   // port, ...).

        ,
        e_QUEUEID_FULL = 7  // The upstream queueId counter has reached
                            // capacity limit.

        ,
        e_SUBQUEUEID_FULL = 8  // The upstream subQueueId counter has
                               // reached capacity limit for a particular
                               // queue

        ,
        e_RECOVERY_FAILURE = 9  // A fatal error was encountered while
                                // attempting storage recovery.

        ,
        e_STORAGE_OUT_OF_SYNC = 10  // Storage on this node has gone out of
                                    // sync.

        ,
        e_UNSUPPORTED_SCENARIO = 11  // An supported scenario was encountered.

        ,
        e_MEMORY_LIMIT = 12  // The configured maximum memory allocation
                             // has been reached.

        ,
        e_REQUESTED = 13  // The broker was requested, through a
                          // command, to stop.
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `GenericResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&  stream,
                               ExitCode::Enum value,
                               int            level          = 0,
                               int            spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ExitCode::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ExitCode::Enum* out, const bslstl::StringRef& str);
};

// --------------
// class ExitCode
// --------------

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, ExitCode::Enum value);

// ===============
// struct ExitUtil
// ===============

/// Provide helper functions to shutdown the bmqbrkr
struct ExitUtil {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBU.EXIT");

  public:
    /// Exit the bmqbrkr by calling `bsl::exit` with the specified
    /// `reason`. Note that this is an ungraceful shutdown of the broker.
    static void terminate(ExitCode::Enum reason);

    /// Gracefully shutdown the broker, for the specified `reason`, by
    /// sending to itself the `SIGINT` signal, that will be caught by the
    /// signal handler setup in main.
    static void shutdown(ExitCode::Enum reason);
};

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------
// struct ExitCode
// ---------------

inline bsl::ostream& mqbu::operator<<(bsl::ostream&        stream,
                                      mqbu::ExitCode::Enum value)
{
    return mqbu::ExitCode::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
