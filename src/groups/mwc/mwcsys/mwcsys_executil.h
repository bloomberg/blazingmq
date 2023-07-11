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

// mwcsys_executil.h                                                  -*-C++-*-
#ifndef INCLUDED_MWCSYS_EXECUTIL
#define INCLUDED_MWCSYS_EXECUTIL

//@PURPOSE: Provide utilities to execute commands on the system.
//
//@CLASSES:
//  mwcsys::ExecUtil: Utility to execute commands on the system
//
//@DESCRIPTION: 'mwcsys::ExecUtil' provides a utility namespace for executing
// commands on the system and retrieving the status code as well as the output.
//
/// Usage Example
///-------------
// The following example illustrates typical intended usage of this component.
//
//..
//  const char k_SCRIPT_PATH = "path/to/my/script.py";
//
//  // Prepare the command to execute
//  bsl::ostringstream command;
//  command << k_SCRIPT_PATH << " --myArg=Value";
//
//  // Execute the command
//  bsl::string output;
//  int rc = mwcsys::ExecUtil::execute(&output, command.str().c_str());
//  if (rc != 0) {
//      // Command failed to execute
//      BALL_LOG_ERROR << "Error while executing command '" << command.str()
//                     << "' [rc: " << rc << ", output: '" << output << "']";
//      return -1;                                                    // RETURN
//  }
//
//  // Command successfully executed, do something with its 'output'.
//..

// MWC

// BDE
#include <bsl_string.h>

namespace BloombergLP {
namespace mwcsys {

// ===============
// struct ExecUtil
// ===============

/// Utility namespace for executing commands on the system.
struct ExecUtil {
    // CLASS METHODS

    /// Execute the specified `command` and store its resulting stdout in
    /// the specified `output`.  Return the `>= 0` exit status of the
    /// command when successfully executed it, or a negative code on system
    /// failure (such as failure to fork, command interrupted, ...).
    static int execute(bsl::string* output, const char* command);

    /// If the file at the specified `srcFile` is a script (starts with a
    /// shebang), populate the specified `output` with the stdout of its
    /// execution, otherwise populate `output` with the content of the
    /// `srcFile`.  Note that when executed, the specified `arguments` are
    /// provided to the script.  Return 0 on success, and a non-zero return
    /// code on any error (such as the file doesn't exist, it failed to
    /// execute, ...).
    static int outputFromFile(bsl::string* output,
                              const char*  srcFile,
                              const char*  arguments);
};

}  // close package namespace
}  // close enterprise namespace

#endif
