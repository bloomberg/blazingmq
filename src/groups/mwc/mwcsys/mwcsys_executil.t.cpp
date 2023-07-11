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

// mwcsys_executil.t.cpp                                              -*-C++-*-
#include <mwcsys_executil.h>

// BDE
#include <bdls_filesystemutil.h>

// SYSTEM
#include <sys/resource.h>  // for setrlimit
#include <sys/stat.h>      // for chmod

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                   HELPER
// ----------------------------------------------------------------------------

/// Create a file at the specified `path` and write the specified `content`
/// into it; ensure the file has the specified `mode` flags set.
static void createFile(const char* path, const char* content, mode_t mode)
{
    bdls::FilesystemUtil::FileDescriptor fd = bdls::FilesystemUtil::open(
        path,
        bdls::FilesystemUtil::e_OPEN_OR_CREATE,
        bdls::FilesystemUtil::e_WRITE_ONLY,
        bdls::FilesystemUtil::e_TRUNCATE);
    ASSERT_NE(fd, bdls::FilesystemUtil::k_INVALID_FD);
    bdls::FilesystemUtil::write(fd, content, bsl::strlen(content));
    bdls::FilesystemUtil::close(fd);

    chmod(path, mode);
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_execute()
// ------------------------------------------------------------------------
// EXECUTE
//
// Concerns:
//   - ensure the correct return value of the 'execute()' method for the
//     success or proper (non-zero) exit of the command
//   - ensure the proper command output is returned
//
// Plan:
//   Execute a few commands and compare the rc and output values.
//
// Testing:
//   Proper behavior of the 'execute()' method.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("execute");

    struct Test {
        int         d_line;            // Line
        const char* d_command;         // Command to execute
        int         d_expectedRc;      // Expected rc
        const char* d_expectedOutput;  // Expected output
    } k_DATA[] = {
        // Success commands, check output
        {L_, "bash -c \"echo 'success'\"", 0, "success\n"},
        {L_, "bash -c \"echo -n 'noNewLine'\"", 0, "noNewLine"},
        {L_, "bash -c 'echo \"Multi\nLine\nOut\"'", 0, "Multi\nLine\nOut\n"},

        // Failure commands, check valid rc
        {L_, "bash -c \"exit 3\"", 3, ""},
        {L_, "bash -c \"echo 'I failed' && exit 5\"", 5, "I failed\n"},
        {L_, "sOmENoNExIsTiNgCmD", 127, ""},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": executing command '" << test.d_command << "'");

        bsl::string output(s_allocator_p);
        int         rc = mwcsys::ExecUtil::execute(&output, test.d_command);
        ASSERT_EQ_D("line " << test.d_line, rc, test.d_expectedRc);
        ASSERT_EQ_D("line " << test.d_line, output, test.d_expectedOutput);
    }
}

static void test2_executeSystemFailure()
// ------------------------------------------------------------------------
// EXECUTE-SYSTEM_FAILURE
//
// Concerns:
//   - ensure proper return code is returned when the command exits
//     abnormally (signaled) or fails to execute due to resource
//     limitations.
//
// Plan:
//   - Execute a suicidal command that terminates by sending itself a
//     SIGINT.
//   - Use 'setrlimit()' [http://linux.die.net/man/2/setrlimit] to change
//     the maximum number of file descriptor for the current process, so
//     that the command will fail to execute.
//
// Testing:
//   Proper behavior of the 'execute()' method in case of system failures.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("execute (system failure)");

    bsl::string output(s_allocator_p);
    int         rc;

    PVV("Testing abnormal exit of the command");
    rc = mwcsys::ExecUtil::execute(&output,
                                   "python -c 'import os,signal; "
                                   "os.kill(os.getpid(), signal.SIGKILL)'");
    ASSERT_EQ(rc, -2);
    ASSERT_EQ(output, "");

    PVV("Test resource limits");
    rlimit limit = {4, 4};  // stdin, stdout, stderr
    rc           = setrlimit(RLIMIT_NOFILE, &limit);
    ASSERT_EQ(rc, 0);

    rc = mwcsys::ExecUtil::execute(&output, "bash -c \"/bin/echo 'test'\"");
    // NOTE: We must force use '/bin/echo' and not just 'echo' to make sure
    //       it will fork and not just run it as a shell built-in.
    ASSERT_EQ(rc, -1);
    ASSERT_EQ(output, "");
}

static void test3_outputFromFileNoGen()
// ------------------------------------------------------------------------
// OUTPUTFROMFILE-NOGEN
//
// Concerns:
//   - ensure 'outputFromFile' returns the input file content when it's not
//     an executable script
//
// Plan:
//   - Generate a temporary file, and invoke 'outputFromFile', and compare
//     the return status and content.
//
// Testing:
//   Proper behavior of the 'outputFromFile()' method in case the input is
//   not a script.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("outputFromFile (noGen)");

    const char k_TESTFILE[] = "./mwcsys_executil.t.in";
    const char k_CONTENT[]  = "Test file\ngenerated by mwcsys::ExecUtil.t";

    struct Test {
        int         d_line;  // Line
        const char* d_content;
        const char* d_arguments;
        const char* d_expectedOutput;
    } k_DATA[] = {
        {L_, "", "", ""},
        {L_, "a", "", "a"},
        {L_, "", "b", ""},
        {L_, "", "b", ""},
        {L_, k_CONTENT, "", k_CONTENT},
        {L_, k_CONTENT, "UnusedArguments", k_CONTENT},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": invoking with content '" << test.d_content
                        << "' and arguments '" << test.d_arguments << "'");

        createFile(k_TESTFILE, test.d_content, S_IRWXU);

        bsl::string output(s_allocator_p);
        int         rc = mwcsys::ExecUtil::outputFromFile(&output,
                                                  k_TESTFILE,
                                                  test.d_arguments);
        ASSERT_EQ_D("line " << test.d_line, rc, 0);
        ASSERT_EQ_D("line " << test.d_line, output, test.d_expectedOutput);

        bdls::FilesystemUtil::remove(k_TESTFILE);
    }
}

static void test4_outputFromFileGen()
// ------------------------------------------------------------------------
// OUTPUTFROMFILE-GEN
//
// Concerns:
//   - ensure 'outputFromFile' properly detects the input file is a script,
//     executes it with the parameters and returns the proper output.  Also
//     ensure that a script looking file, which doesn't have the exec
//     permission, will properly return an error.
//
// Plan:
//   - Generate a temporary file, containing a simple shell script that
//     echo to stdout all its passed arguments and invoke 'outputFromFile',
//     and compare the return status and content.  First test when the file
//     doesn't have exec permission, then with.
//
// Testing:
//   Proper behavior of the 'outputFromFile()' method in case the input is
//   a script.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("outputFromFile (gen)");

    const char k_TESTFILE[] = "./mwcsys_executil.t.in";

    struct Test {
        int         d_line;
        const char* d_content;
        const char* d_arguments;
        bool        d_expectSuccess;
        const char* d_expectedOutput;
    } k_DATA[] = {
        {L_, "#! /bin/bash\necho -n $@", "", true, ""},
        {L_, "#! /bin/bash\necho -n $@", "a", true, "a"},
        {L_, "#! /bin/bash\necho -n $@\nexit 2", "b", false, "b"},

    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": invoking with content '" << test.d_content
                        << "' and arguments '" << test.d_arguments << "'");

        createFile(k_TESTFILE, test.d_content, S_IRWXU);

        bsl::string output(s_allocator_p);
        int         rc = mwcsys::ExecUtil::outputFromFile(&output,
                                                  k_TESTFILE,
                                                  test.d_arguments);
        ASSERT_EQ_D("line " << test.d_line, (rc == 0), test.d_expectSuccess);
        ASSERT_EQ_D("line " << test.d_line, output, test.d_expectedOutput);

        bdls::FilesystemUtil::remove(k_TESTFILE);
    }
}

static void test5_outputFromFileError()
// ------------------------------------------------------------------------
// OUTPUTFROMFILE-GEN
//
// Concerns:
//   - ensure 'outputFromFile' properly handles various system errors:
//     - the file doesn't exist
//     - the file is not executable
//     - the shebang of the file references an invalid interpreter
//
// Testing:
//   Proper behavior of the 'outputFromFile()' method in case the input is
//   invalid.
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("outputFromFile (error)");

    const char k_TESTFILE[] = "./mwcsys_executil.t.in";

    {
        PV("Non-existent input file");

        if (bdls::FilesystemUtil::exists(k_TESTFILE)) {
            bdls::FilesystemUtil::remove(k_TESTFILE);
        }

        bsl::string output(s_allocator_p);
        int         rc = mwcsys::ExecUtil::outputFromFile(&output,
                                                  k_TESTFILE,
                                                  "dummyArgs");
        ASSERT_NE(rc, 0);
    }

    {
        PV("Not a regular input file");

        bsl::string output(s_allocator_p);
        int         rc = mwcsys::ExecUtil::outputFromFile(&output,
                                                  "./",  // a directory
                                                  "dummyArgs");
        ASSERT_NE(rc, 0);
    }

    {
        PV("Non-executable input file");

        createFile(k_TESTFILE, "#! /bin/bash\necho -n 'yes'", S_IRUSR);

        bsl::string output(s_allocator_p);
        int         rc = mwcsys::ExecUtil::outputFromFile(&output,
                                                  k_TESTFILE,
                                                  "dummyArgs");
        ASSERT_NE(rc, 0);

        bdls::FilesystemUtil::remove(k_TESTFILE);
    }

    {
        PV("Non-existent interpreter");

        createFile(k_TESTFILE,
                   "#! /InVaLiDInTeRpReTeR\necho -n 'yes'",
                   S_IRWXU);

        bsl::string output(s_allocator_p);
        int         rc = mwcsys::ExecUtil::outputFromFile(&output,
                                                  k_TESTFILE,
                                                  "dummyArgs");
        ASSERT_NE(rc, 0);

        bdls::FilesystemUtil::remove(k_TESTFILE);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 5: test5_outputFromFileError(); break;
    case 4: test4_outputFromFileGen(); break;
    case 3: test3_outputFromFileNoGen(); break;
    case 2: test2_executeSystemFailure(); break;
    case 1: test1_execute(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
