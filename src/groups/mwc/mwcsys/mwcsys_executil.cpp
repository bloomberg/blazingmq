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

// mwcsys_executil.cpp                                                -*-C++-*-
#include <mwcsys_executil.h>

#include <mwcscm_version.h>

// BMQ
#include <mwcu_memoutstream.h>

// MWC
#include <mwcu_stringutil.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bdls_filesystemutil.h>
#include <bsl_cstdio.h>
#include <bsl_cstdlib.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bslma_default.h>

// SYSTEM
#include <sys/wait.h>  // for WIFEXITED, WEXITSTATUS

namespace BloombergLP {
namespace mwcsys {

// ---------------
// struct ExecUtil
// ---------------

int ExecUtil::execute(bsl::string* output, const char* command)
{
    enum RcEnum {
        // Value for the various RC error categories

        rc_SUCCESS = 0  // command successfully executed

        ,
        rc_POPEN_FAILED = -1  // system failure (failed to fork, too many
                              // file descriptors, ...)

        ,
        rc_COMMAND_FAILURE = -2  // command exited abnormally (due to receipt
                                 // of a signal, ...)
    };

    // Execute the command
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        return rc_POPEN_FAILED;  // RETURN
    }

    // Read the output
    bdlma::LocalSequentialAllocator<2048> localAllocator(
        bslma::Default::allocator(0));
    mwcu::MemOutStream cmdOutput(&localAllocator);
    char               buffer[1024];

    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != 0) {
            cmdOutput << buffer;
        }
    }

    output->assign(cmdOutput.str().data(), cmdOutput.str().length());

    // Close the pipe, and retrieve the rc code of execution of the command.
    int rc = pclose(pipe);
    if (rc != 0) {
        if (WIFEXITED(rc)) {
            // Process normally exited with an 'exit', extract the lower 8 bits
            // return value.
            return WEXITSTATUS(rc);  // RETURN
        }
        return rc_COMMAND_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

int ExecUtil::outputFromFile(bsl::string* output,
                             const char*  srcFile,
                             const char*  arguments)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS           = 0,
        rc_FILE_NON_EXISTANT = -1,
        rc_EXECUTE_FAIL      = -2
    };

    bdlma::LocalSequentialAllocator<1024> localAllocator(
        bslma::Default::allocator());
    mwcu::MemOutStream os(&localAllocator);

    // Ensure the file exist and is a regular file (not a directory or other)
    if (!bdls::FilesystemUtil::isRegularFile(srcFile, true)) {
        os << "File '" << srcFile << "' is not a regular file";
        output->assign(os.str().data(), os.str().length());
        return rc_FILE_NON_EXISTANT;  // RETURN
    }

    bsl::ifstream stream(srcFile, bsl::ios::in);
    if (!stream.is_open()) {
        os << "Failed to open file '" << srcFile << "'";
        output->assign(os.str().data(), os.str().length());
        return rc_FILE_NON_EXISTANT;  // RETURN
    }

    // Check whether its a script that should be executed, or a file that
    // should be read.  For that, read the first line and test if it represents
    // a shebang.
    bsl::string firstLine(&localAllocator);
    getline(stream, firstLine);

    if (!mwcu::StringUtil::startsWith(firstLine, "#!")) {
        stream.clear();  // Reset the state (error bits) of the stream, in case
                         // the file was empty, the above readline sets the
                         // failbit, which will prevent the 'tellg' from
                         // returning the right size (it returns -1).

        // File is not a script, just read and return its entire content.
        stream.seekg(0, bsl::ios::end);
        output->resize(stream.tellg());
        stream.seekg(0, bsl::ios::beg);
        stream.read(&(*output)[0], output->size());
        stream.close();

        return rc_SUCCESS;  // RETURN
    }

    stream.close();

    // File is a script, execute it.
    os << srcFile << " " << arguments << bsl::ends;
    int rc = execute(output, os.str().data());
    if (rc != 0) {
        return (rc * 10) + rc_EXECUTE_FAIL;  // RETURN
    }

    return rc_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
