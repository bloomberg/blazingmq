// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqbrkr.m.cpp                                                      -*-C++-*-

// Application
#include <bmqbrkrscm_version.h>
#include <m_bmqbrkr_task.h>

// MQB
#include <mqba_application.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbu_exit.h>
#include <mqbu_messageguidutil.h>

// MWC
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <balcl_commandline.h>
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <ball_log.h>
#include <balst_stacktraceprintutil.h>
#include <bdlb_string.h>
#include <bdld_datum.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>
#include <bdls_processutil.h>
#include <bdlsb_fixedmeminstreambuf.h>
#include <bdlt_currenttime.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_cstdio.h>
#include <bsl_cstdlib.h>
#include <bsl_cstring.h>
#include <bsl_exception.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_list.h>
#include <bsl_stdexcept.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_managedptr.h>
#include <bslmt_semaphore.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_assertimputil.h>
#include <bsls_objectbuffer.h>
#include <bsls_platform.h>
#include <bsls_types.h>

// SYSTEM
#include <unistd.h>  // for isatty

#if defined(BSLS_PLATFORM_OS_UNIX)
#include <signal.h>
#endif

using namespace BloombergLP;

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQBRKR");

/// Struct holding various `global` parameters representing the Context of
/// this process.
struct TaskEnvironment {
    bsl::string d_instanceId;
    // BMQ_INSTANCE if provided ('default'
    // otherwise)

    bsl::string d_bmqPrefix;
    // BMQ_PREFIX directory path

    bsl::string d_configJson;
    // JSON content ouput of the generated
    // config

    mqbcfg::Configuration d_config;
    // De-serialized config

    bsls::ObjectBuffer<m_bmqbrkr::Task> d_task;
    // The Task

    bsls::ObjectBuffer<mqba::Application> d_app;
    // The Application

    bslmt::Semaphore d_shutdownSemaphore;
    // Used to keep the program alive until
    // shutdown either from an EXIT M-Trap
    // command, or a ctrl-c (when running on a
    // tty).  Note that after posting to this
    // semaphore, it is undefined behavior to
    // use any methods on the Application of the
    // Task as those are getting shutdown and
    // destroyed.
};

/// Raw pointer to the TaskEnvironment, used so that the signal handler, and
/// the assert handler can access the needed data.
static TaskEnvironment* s_taskEnv_p = 0;

}  // close unnamed namespace

extern "C" {
/// Callback method for signal handler; called when the specified `signal`
/// was sent to the application.
static void appShutdownSignal(int signal)
{
    bsl::cout << "Shutting down [received signal '" << strsignal(signal)
              << "' (" << signal << ")]\n"
              << bsl::flush;
    if (s_taskEnv_p) {
        s_taskEnv_p->d_shutdownSemaphore.post();
    }
}
}  // close 'C' namespace

/// Since we use an async file observer for BALL logging, there is a
/// possibility upon assert that the log file would be missing the last few
/// log records, which would be the most important ones in order to debug
/// the cause of the assertion.  This assert handler will ensure the BALL
/// file observer has flushed every pending record to disk, including a
/// record describing the assertion failure, along with a stack trace, then
/// it will cause the task to abort.  Note that this assert handler is setup
/// *after* the logController has been correctly configured, and removed
/// just *before* it is being torn down, so we have the guarantee in here
/// that all the objects hierarchy leading to the fileObserver are in valid
/// state.
BSLS_ANNOTATION_NORETURN
static void bmqAssertHandler(const char* comment, const char* file, int line)
{
    // The following code is copied verbatim from 'printError' in
    // bsls_assert.cpp
    if (!comment) {
        comment = "(* Unspecified Comment Text *)";
    }
    else if (!*comment) {
        comment = "(* Empty Comment Text *)";
    }

    if (!file) {
        file = "(* Unspecified File Name *)";
    }
    else if (!*file) {
        file = "(* Empty File Name *)";
    }

    mwcu::MemOutStream stackTrace;
    balst::StackTracePrintUtil::printStackTrace(stackTrace);
    stackTrace << bsl::ends;

    bsls::Log::logFormattedMessage(bsls::LogSeverity::e_FATAL,
                                   file,
                                   line,
                                   "Assertion failed: %s, stack trace:\n%s",
                                   comment,
                                   stackTrace.str().data());

    s_taskEnv_p->d_task.object()
        .logController()
        .fileObserver()
        .stopPublicationThread();

    bsls::AssertImpUtil::failByAbort();
}

// ====
// main
// ====

/// On Unix only, ignore the SIGPIPE & SIGXFSZ signal.
static void ignoreSigpipe()
{
#if defined(BSLS_PLATFORM_OS_UNIX)
    // Ignore SIGPIPE & SIGXFSZ on Unix platforms.  By default, SIGXFSZ
    // terminates the process.  However, if caught/ignored, the relevant system
    // call (write, truncate) fails with the error EFBIG.

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags   = 0;
    sa.sa_handler = SIG_IGN;

    int rc = 0;

    rc = ::sigaction(SIGPIPE, &sa, NULL);
    if (rc != 0) {
        bsl::cerr << "Failed to ignore SIGPIPE (rc: " << rc << ")\n"
                  << bsl::flush;
    }

    rc = ::sigaction(SIGXFSZ, &sa, NULL);
    if (rc != 0) {
        bsl::cerr << "Failed to ignore SIGXFSZ (rc: " << rc << ")\n"
                  << bsl::flush;
    }

#endif
}

/// Print the start/stop trace for the specified `action` (starting,
/// started, stopping, stopped, ...) for the specified `instance` (could be
/// empty).
static void printStartStopTrace(const bslstl::StringRef& action,
                                const bsl::string&       instance = "")
{
    bsl::cout << action << " bmqbrkr [";
    if (!instance.empty() && instance != "default") {
        bsl::cout << "instance: '" << instance << "', ";
    }
    bsl::cout << "pid: " << bdls::ProcessUtil::getProcessId() << "]\n"
              << bsl::flush;
}

/// Retrieve the config from the `bmqbrkrcfg.json` file in the specified
/// `configDir` file.
///
/// Return 0 on success and store the JSON content, at well as its
/// associated de-serialized object part in the corresponding field of the
/// specified `taskEnv`, or return a non-zero error code and populate the
/// specified `errorDescription` with a description of the error.
static int getConfig(bsl::ostream&      errorDescription,
                     TaskEnvironment*   taskEnv,
                     const bsl::string& configDir)
{
    enum RcEnum {
        // Enum for the various RC error categories
        rc_SUCCESS           = 0,
        rc_GENERATION_FAILED = -1,
        rc_DECODE_FAILED     = -2
    };

    int rc = rc_SUCCESS;

    // Generate the config
    bsl::string configFilename = configDir;

    if (bdls::PathUtil::appendIfValid(&configFilename, "bmqbrkrcfg.json")) {
        bsl::cerr << "PANIC [STARTUP]: invalid configuration path "
                  << configDir << "\n"
                  << bsl::flush;
        return mqbu::ExitCode::e_CONFIG_GENERATION;  // RETURN
    }

    bsl::cout << "Reading broker configuration from " << configFilename
              << "\n";
    bsl::ifstream      configStream(configFilename.c_str());
    mwcu::MemOutStream configParameters;
    configParameters << configStream.rdbuf();
    taskEnv->d_configJson = configParameters.str();

    if (!configStream || !configParameters) {
        errorDescription << "Failed to read the config "
                         << "[file: " << configFilename << "]";
        return rc_GENERATION_FAILED;  // RETURN
    }

    // ----------------------------------
    // 2. JSON parse the config to object
    baljsn::Decoder        decoder;
    baljsn::DecoderOptions options;
    options.setSkipUnknownElements(true);

    bdlsb::FixedMemInStreamBuf jsonStreamBuf(taskEnv->d_configJson.data(),
                                             taskEnv->d_configJson.length());

    rc = decoder.decode(&jsonStreamBuf, &taskEnv->d_config, options);
    if (rc != 0) {
        errorDescription << "Error decoding appConfig (JSON->Object) "
                         << "[rc: " << rc
                         << ", error: " << decoder.loggedMessages() << "], "
                         << "generated config file (first 1024 characters):\n"
                         << taskEnv->d_configJson.substr(
                                0,
                                bsl::min(1024,
                                         static_cast<int>(
                                             taskEnv->d_configJson.length())));
        return rc_DECODE_FAILED;  // RETURN
    }

    taskEnv->d_config.appConfig().etcDir() = configDir;
    taskEnv->d_config.appConfig().brokerVersion() =
        bmqbrkrscm::Version::versionAsInt();
    mqbcfg::BrokerConfig::set(taskEnv->d_config.appConfig());

    return rc_SUCCESS;
}

/// Callback called upon execution of the specified admin command `command`
/// with the specified `prefix`.  The specified `start` is a time when
/// command have been received and scheduled to execution.  The specified
/// `rc` is a return code after execution of the command.  The specified
/// `results` is a command execution results.
static void onProcessedAdminCommand(const bsl::string&       prefix,
                                    const bsl::string&       command,
                                    const bsls::Types::Int64 start,
                                    int                      rc,
                                    const bsl::string&       results)
{
    const bsls::Types::Int64 end = mwcsys::Time::highResolutionTimer();

    if (rc != 0) {
        BALL_LOG_ERROR << "Error processing command [rc: " << rc << "] "
                       << results << "\n"
                       << "Send 'HELP' to see a list of commands supported by "
                       << "bmqbrkr.";
    }
    else {
        BALL_LOG_INFO << "Command '" << prefix << " " << command
                      << "' processed successfully in "
                      << mwcu::PrintUtil::prettyTimeInterval(end - start)
                      << ":\n"
                      << results;
    }
}

/// Callback when an M-Trap command is received for the specified `taskEnv`,
/// with the specified `input` command having the specified `prefix` (being
/// the command type).
static void onMTrap(TaskEnvironment*   taskEnv,
                    const bsl::string& prefix,
                    bsl::istream&      input)
{
    BSLS_ASSERT_SAFE(taskEnv);

    bsl::string cmd;

    bsl::getline(input, cmd);
    cmd.erase(0, 1);  // cmd starts by a space, remove it

    if (bdlb::String::areEqualCaseless(prefix, "EXIT")) {
        taskEnv->d_shutdownSemaphore.post();
        // Return now because as a consequence of the Task::shutdown() call,
        // the BALL log system has been torn down.
        return;  // RETURN
    }
    else if (bdlb::String::areEqualCaseless(prefix, "CMD")) {
        const bsls::Types::Int64 start = mwcsys::Time::highResolutionTimer();
        taskEnv->d_app.object().enqueueCommand(
            "MTRAP",
            cmd,
            bdlf::BindUtil::bind(
                onProcessedAdminCommand,
                prefix,
                cmd,
                start,
                bdlf::PlaceHolders::_1,    // rc
                bdlf::PlaceHolders::_2));  // commandExecResults
        return;                            // RETURN
    }

    BALL_LOG_ERROR << "Unknown command '" << prefix << "'\n"
                   << "Send 'HELP' to see a list of commands supported by "
                   << "bmqbrkr.";
}

/// Create and initialize the Task object from the specified `taskEnv`.
/// Return 0 on success, or a non-zero error code and populate the specified
/// `errorDescription` with a description of the error otherwise.
static int initializeTask(bsl::ostream&    errorDescription,
                          TaskEnvironment* taskEnv)
{
    enum RcEnum {
        // Enum for the various RC error categories
        rc_SUCCESS           = 0,
        rc_INITIALIZE_FAILED = -1
    };

    // Create the Task
    new (taskEnv->d_task.buffer())
        m_bmqbrkr::Task(taskEnv->d_bmqPrefix, taskEnv->d_config.taskConfig());

    mwcu::MemOutStream localError;
    const int          rc = taskEnv->d_task.object().initialize(localError);
    if (rc != 0) {
        errorDescription << "Failed to initialize task "
                         << "[rc: " << rc << ", reason: '" << localError.str()
                         << "']";

        // Destroy the task
        taskEnv->d_task.object().m_bmqbrkr::Task::~Task();
        return rc_INITIALIZE_FAILED;  // RETURN
    }

    // Register 'EXIT' M-Trap
    taskEnv->d_task.object().registerMTrapHandler(
        "EXIT",
        "",
        "Gracefully terminate bmqbrkr",
        bdlf::BindUtil::bind(onMTrap,
                             taskEnv,
                             bdlf::PlaceHolders::_1,    // prefix
                             bdlf::PlaceHolders::_2));  // istream

    // Save the PID of the process in the '${BMQ_PREFIX}/bmqbrkr.pid' file
    const bsl::string pidFile = taskEnv->d_bmqPrefix + "/bmqbrkr.pid";
    bsl::ofstream     pidFd(pidFile.c_str());
    if (!pidFd) {
        MWCTSK_ALARMLOG_ALARM("STARTUP")
            << "Failed to create pid file [" << pidFile << "]."
            << " This is not fatal." << MWCTSK_ALARMLOG_END;
    }
    else {
        pidFd << bdls::ProcessUtil::getProcessId() << "\n";
    }
    pidFd.close();

    // Setup the assert handler (refer to the comments in 'bmqAssertHandler' as
    // to why this is done at this point and not earlier.
    bsls::Assert::setFailureHandler(&bmqAssertHandler);

    return 0;
}

/// Shutdown and destroy the Task from the specified `taskEnv`.
static void shutdownTask(TaskEnvironment* taskEnv)
{
    // Remove our custom assert handler (refer to the comments in
    // 'bmqAssertHandler' as to why this is done at this point and not later.
    bsls::Assert::setFailureHandler(&bsls::Assert::failAbort);

    // Shutdown Task
    taskEnv->d_task.object().shutdown();

    // Destroy Task
    taskEnv->d_task.object().m_bmqbrkr::Task::~Task();

    // Delete the PID file
    const bsl::string pidFile = taskEnv->d_bmqPrefix + "/bmqbrkr.pid";
    bdls::FilesystemUtil::remove(pidFile);
}

/// Create and initialize the Application object from the specified
/// `taskEnv`.  Return 0 on success, or a non-zero error code and populate
/// the specified `errorDescription` with a description of the error
/// otherwise.
static int
initializeApplication(BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
                      TaskEnvironment*                     taskEnv)
{
    // Dump the generated config file
    BALL_LOG_INFO << "Configuration: " << '\n' << taskEnv->d_configJson;

    m_bmqbrkr::Task& task = taskEnv->d_task.object();

    // Create the Application
    new (taskEnv->d_app.buffer())
        mqba::Application(task.scheduler(),
                          task.allocatorStatContext(),
                          task.allocatorStore().get("Application"));

    // Register 'CMD' M-Trap
    taskEnv->d_task.object().registerMTrapHandler(
        "CMD",
        "",
        "bmqbrkr specific commands",
        bdlf::BindUtil::bind(onMTrap,
                             taskEnv,
                             bdlf::PlaceHolders::_1,    // prefix
                             bdlf::PlaceHolders::_2));  // istream
    return 0;
}

/// Shutdown and destroy the Application from the specified `taskEnv`.
static void shutdownApplication(TaskEnvironment* taskEnv)
{
    m_bmqbrkr::Task&   task = taskEnv->d_task.object();
    mqba::Application& app  = taskEnv->d_app.object();

    // DeRegister M-Trap handler
    task.deregisterMTrapHandler("CMD");

    // Stop Application
    app.stop();

    // Destroy Application
    app.mqba::Application::~Application();
}

/// Update the `bmqbrkr.hist` file (in the BMQ_PREFIX directory) using the
/// specified `taskEnv`.  This file contains information about the last `n`
/// successfull start of the broker, in reverse time order.
/// Each line entry has the following format:
///    <currentTime_UTC>|<brokerVersion>|<configVersion>|<brokerId>
///
/// The purpose of this file is to provide information about the last
/// executions of the broker: some important details (such as the broker
/// version) are only printed once at startup, so can get lost after log
/// rotation and cleanup; especially on machines with infrequent turn
/// schedule (less than once a week).  On such machines, in case of crash,
/// it is difficult to know which version was running at the time of the
/// crash because the logs have been rotated and cleaned up, and many new
/// versions may have been deployed since the last start, overriding it.
static void updateHistFile(const TaskEnvironment* taskEnv)
{
    static const int k_MAX_ENTRIES = 90;  // Maximum number of entries to keep

    const bsl::string      histFile = taskEnv->d_bmqPrefix + "/bmqbrkr.hist";
    bsl::list<bsl::string> entries;

    // First, read existing entries from the file (if it exists)
    if (bdls::FilesystemUtil::exists(histFile)) {
        bsl::ifstream input(histFile.c_str());
        if (input) {
            bsl::string line;
            while (bsl::getline(input, line)) {
                entries.push_back(line);
            }
        }
        input.close();
    }

    // Make space if reached max capacity
    if (entries.size() == k_MAX_ENTRIES) {
        entries.pop_back();
    }

    // Generate the new entry
    mwcu::MemOutStream os;
    os << bdlt::CurrentTime::utc() << "|"
       << taskEnv->d_config.appConfig().brokerVersion() << "|"
       << taskEnv->d_config.appConfig().configVersion() << "|"
       << mqbu::MessageGUIDUtil::brokerIdHex();

    entries.push_front(bsl::string(os.str().data(), os.str().length()));

    // Write back all entries to file
    bsl::ofstream output(histFile.c_str());
    if (!output) {
        MWCTSK_ALARMLOG_ALARM("STARTUP")
            << "Failed to create vers file [" << histFile << "]. "
            << "This is not fatal." << MWCTSK_ALARMLOG_END;
    }
    else {
        for (bsl::list<bsl::string>::const_iterator it = entries.begin();
             it != entries.end();
             ++it) {
            output << *it << "\n";
        }
    }
    output.close();
}

/// Start the Application in the specified `taskEnv` and wait until
/// termination (if the optionally specified `wait` is true).  Return 0 on
/// success, or a non-zero error code and populate the specified
/// `errorDescription` with a description of the error otherwise.
static int
run(bsl::ostream& errorDescription, TaskEnvironment* taskEnv, bool wait = true)
{
    // Start the application
    mwcu::MemOutStream localError;

    int rc = taskEnv->d_app.object().start(localError);
    if (rc != 0) {
        errorDescription << "Failed to start application [rc: " << rc
                         << ", error: " << localError.str() << "]";
        return 1;  // RETURN
    }

    // Broker successfully started, prepend entry in the hist file.
    updateHistFile(taskEnv);

    printStartStopTrace("STARTED", taskEnv->d_instanceId);

    // Wait till termination of the program (ctrl-c, exit command, ...)
    if (wait) {
        taskEnv->d_shutdownSemaphore.wait();
        printStartStopTrace("STOPPING", taskEnv->d_instanceId);
    }

    return 0;
}

int main(int argc, const char* argv[])
{

    // Parse command line parameters
    bsl::string configDir;
    bsl::string instanceId = "default";
    bsl::string hostName;
    bsl::string hostTags;
    bsl::string hostDataCenter;
    int         port    = 0;
    bool        version = false;

    balcl::OptionInfo specTable[] = {
        {"",
         "config",
         "Path to the configuration directory",
         balcl::TypeInfo(&configDir),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"i|instanceId",
         "instanceId",
         "The instance ID ('default' if not provided)",
         balcl::TypeInfo(&instanceId),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"h|hostName",
         "hostName",
         "Override host name",
         balcl::TypeInfo(&hostName),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"t|hostTags",
         "hostTags",
         "Override host tags",
         balcl::TypeInfo(&hostTags),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"d|hostDataCenter",
         "hostDataCenter",
         "Override host data center",
         balcl::TypeInfo(&hostDataCenter),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"p|port",
         "port",
         "Override port",
         balcl::TypeInfo(&port),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"v|version",
         "version",
         "Show version number",
         balcl::TypeInfo(&version),
         balcl::OccurrenceInfo::e_OPTIONAL},
    };

    balcl::CommandLine commandLine(specTable);

    if (commandLine.parse(argc, argv)) {
        bsl::cerr << "PANIC [STARTUP] Failed to parse command line\n"
                  << bsl::flush;
        commandLine.printUsage();
        return mqbu::ExitCode::e_COMMAND_LINE;  // RETURN
    }

    if (version) {
        bsl::cout << bmqbrkrscm::Version::version() << "\n";
        return 0;
    }

    if (configDir.empty()) {
        bsl::cerr << "Error: No value supplied for the non-option argument "
                     "\"config\".\n";
    }

    printStartStopTrace("STARTING");

    ignoreSigpipe();

    // Register a SIGINT handler to allow graceful shutdown of the broker from
    // lower layers.
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags   = 0;
    sa.sa_handler = appShutdownSignal;

    int rc = ::sigaction(SIGINT, &sa, NULL);
    if (rc != 0) {
        // Note that this is not fatal, as the primary way to stop the broker
        // is not through a 'kill -s INT', but via issueing an 'exit' command
        // through the control pipe.
        bsl::cerr << "Failed to install SIGINT handler  (rc: " << rc << ")\n"
                  << bsl::flush;
    }

    // If running in a TTY, enable signal handler so that user signals can be
    // used to stop the task.
    if (isatty(fileno(stdout))) {
        ::sigaction(SIGQUIT, &sa, NULL);
        ::sigaction(SIGTERM, &sa, NULL);
    }

    TaskEnvironment taskEnv;
    s_taskEnv_p = &taskEnv;

    const char* prefixEnvVar = bsl::getenv("BMQ_PREFIX");
    taskEnv.d_bmqPrefix      = (prefixEnvVar != 0 ? prefixEnvVar : "./");
    taskEnv.d_instanceId     = instanceId;

    mwcu::MemOutStream errorDescription;

    rc = getConfig(errorDescription, &taskEnv, configDir);
    if (rc != 0) {
        bsl::cerr << "PANIC [STARTUP] (" << rc
                  << "): " << errorDescription.str() << "\n"
                  << bsl::flush;
        return mqbu::ExitCode::e_CONFIG_GENERATION;  // RETURN
    }

    if (!hostName.empty()) {
        taskEnv.d_config.appConfig().hostName() = hostName;
    }

    if (!hostTags.empty()) {
        taskEnv.d_config.appConfig().hostTags() = hostTags;
    }

    if (!hostDataCenter.empty()) {
        taskEnv.d_config.appConfig().hostDataCenter() = hostDataCenter;
    }

    if (port != 0 && !taskEnv.d_config.appConfig()
                          .networkInterfaces()
                          .tcpInterface()
                          .isNull()) {
        taskEnv.d_config.appConfig()
            .networkInterfaces()
            .tcpInterface()
            .value()
            .port() = port;
    }

    // Create and initialize the task
    rc = initializeTask(errorDescription, &taskEnv);
    if (rc != 0) {
        bsl::cerr << "PANIC [STARTUP] (" << rc
                  << "): " << errorDescription.str() << "\n"
                  << bsl::flush;
        return mqbu::ExitCode::e_TASK_INITIALIZE;  // RETURN
    }

    // Initialize the Application
    rc = initializeApplication(errorDescription, &taskEnv);
    if (rc != 0) {
        bsl::cerr << "PANIC [STARTUP] (" << rc
                  << "): " << errorDescription.str() << "\n"
                  << bsl::flush;
        shutdownTask(&taskEnv);
        return mqbu::ExitCode::e_APP_INITIALIZE;  // RETURN
    }

    // Run
    rc = run(errorDescription, &taskEnv);
    if (rc != 0) {
        MWCTSK_ALARMLOG_PANIC("STARTUP")
            << "(" << rc << "): " << errorDescription.str()
            << MWCTSK_ALARMLOG_END;
        shutdownApplication(&taskEnv);
        shutdownTask(&taskEnv);
        return mqbu::ExitCode::e_RUN;  // RETURN
    }

    // Clean shutdown
    shutdownApplication(&taskEnv);
    shutdownTask(&taskEnv);

    printStartStopTrace("STOPPED", taskEnv.d_instanceId);

    return mqbu::ExitCode::e_SUCCESS;
}
