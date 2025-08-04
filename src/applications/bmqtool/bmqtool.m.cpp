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

// bmqtool.m.cpp                                                      -*-C++-*-

// bmqtool
#include <m_bmqtool_application.h>
#include <m_bmqtool_inpututil.h>
#include <m_bmqtool_parameters.h>

// BMQ
#include <bmqt_queueflags.h>
#include <bmqt_sessionoptions.h>
#include <bmqt_uri.h>
#include <bmqu_memoutstream.h>

// BDE
#include <balcl_commandline.h>
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <baljsn_encoder.h>
#include <baljsn_encoderoptions.h>
#include <balst_stacktracetestallocator.h>
#include <bdlde_md5.h>
#include <bsl_cstdio.h>   // for bsl::getenv
#include <bsl_cstring.h>  // for bsl::strcmp
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_testallocator.h>
#include <bslma_testallocatormonitor.h>
#include <bslmt_semaphore.h>
#include <bsls_assert.h>
#include <bsls_timeutil.h>

// SYSTEM
#if defined(BSLS_PLATFORM_OS_UNIX)
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#endif
#include <bsl_cstdlib.h>

using namespace BloombergLP;
using namespace m_bmqtool;

// On UNIX only, ignore the SIGPIPE signal.
static void ignoreSigpipe()
{
#ifdef BSLS_PLATFORM_OS_UNIX
    // Ignore SIGPIPE on Unix platforms.
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags   = 0;
    sa.sa_handler = SIG_IGN;
    if (0 != sigaction(SIGPIPE, &sa, NULL)) {
        bsl::cerr << "Failed to ignore SIGPIPE!" << bsl::endl;
    }
#endif
}

class ShutdownContext {
  public:
    // PUBLIC DATA

    /// The semaphore that keeps the application alive.
    bslmt::Semaphore d_appSemaphore;

    /// The number of times a user tried to close bmqtool.
    bsls::AtomicInt d_shutdownCount;

    // CREATORS
    ShutdownContext()
    : d_appSemaphore(0)
    , d_shutdownCount(0){
          // NOTHING
      };
};

static ShutdownContext* s_shutdownContext_p = 0;

extern "C" {
static void shutdownApp(int sig)
{
    // The number of received signals for immediate termination.
    static const int k_NUM_TO_TERMINATE = 5;

    if (s_shutdownContext_p) {
        const int numRetries = s_shutdownContext_p->d_shutdownCount.addRelaxed(
            1);
        if (k_NUM_TO_TERMINATE <= numRetries) {
            bsl::cerr << "Received a signal [" << sig << "] " << numRetries
                      << " times, terminating immediately" << bsl::endl;
            bsl::exit(128 + sig);
        }
        else {
            bsl::cerr << "Received a signal [" << sig << "], shutting down... "
                      << "Please send a signal "
                      << k_NUM_TO_TERMINATE - numRetries
                      << " more times to terminate immediately" << bsl::endl;
            s_shutdownContext_p->d_appSemaphore.post();
        }
    }
    else {
        bsl::cerr << "No shutdown context provided to handle a signal [" << sig
                  << "]" << bsl::endl;
    }
}
}  // close extern "C"

/// Iterate over the specified `argc` arguments in the specified `argv`
/// list, and return the argument of the `--profile` option, if found, or
/// null if not found (or if `--profile` was the last argument; this error
/// will be caught and reported by optparse later.)
static const char* extractProfilePath(int argc, const char* argv[])
{
    for (int i = 0; i != argc; ++i) {
        if (bsl::strcmp(argv[i], "--profile") == 0) {
            return (i + 1 <= argc) ? argv[i + 1] : 0;  // RETURN
        }
    }
    return 0;
}

static bool loadProfile(CommandLineParameters* params, const char* profilePath)
{
    // PRECONDITIONS
    BSLS_ASSERT(params);

    bsl::ifstream ifstream(profilePath);
    if (!ifstream.is_open()) {
        bsl::cerr << "Failed to open profile file: " << profilePath << "\n";
        return false;  // RETURN
    }

    baljsn::DecoderOptions options;
    options.setSkipUnknownElements(true);

    baljsn::Decoder decoder;
    const int       rc = decoder.decode(ifstream, params, options);
    ifstream.close();
    if (rc != 0) {
        bsl::cerr << "Error decoding JSON in " << profilePath << ": "
                  << decoder.loggedMessages() << "\n";
        return false;  // RETURN
    }

    return true;
}

static bool parseArgs(Parameters* parameters, int argc, const char* argv[])
{
    // Parameters is default initialized, get all default values ...
    CommandLineParameters params(bslma::Default::allocator());

    // Set the default broker URI, taking into account a potential BMQ_PORT
    // envvar override.  Note that we don't validate BMQ_PORT represents a
    // valid integer.
    bsl::string brokerURI("tcp://localhost:");
    char*       bmqPort = bsl::getenv("BMQ_PORT");
    if (bmqPort) {
        brokerURI.append(bmqPort);
    }
    else {
        brokerURI.append(
            bsl::to_string(bmqt::SessionOptions::k_BROKER_DEFAULT_PORT));
    }
    params.broker() = brokerURI;

    const char* profilePath = extractProfilePath(argc, argv);
    if (profilePath) {
        if (!loadProfile(&params, profilePath)) {
            return false;  // RETURN
        }
    }

    bsl::string dummyProfile = "";
    bool        showHelp     = false;
    bool        dumpProfile  = false;
    bsl::string jsonMessageProperties;
    bsl::string jsonSubscriptions;

    balcl::OptionInfo specTable[] = {
        {"mode",
         "mode",
         "mode ([<cli>, auto, storage, syschk])",
         balcl::TypeInfo(&params.mode(), &ParametersMode::isValid),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"b|broker",
         "address",
         "address and port of the broker",
         balcl::TypeInfo(&params.broker()),
         balcl::OccurrenceInfo(params.broker())},
        {"q|queueuri",
         "uri",
         "URI of the queue (for auto/syschk modes)",
         balcl::TypeInfo(&params.queueUri()),
         balcl::OccurrenceInfo(params.queueUri())},
        {"f|queueflags",
         "flags",
         "flags of the queue (for auto mode)",
         balcl::TypeInfo(&params.queueFlags()),
         balcl::OccurrenceInfo(params.queueFlags())},
        {"l|latency",
         "latency",
         "method to use for latency computation ([<none>, hires, epoch])",
         balcl::TypeInfo(&params.latency(), &ParametersLatency::isValid),
         balcl::OccurrenceInfo(params.latency())},
        {"latency-report",
         "report.json",
         "where to generate the JSON latency report",
         balcl::TypeInfo(&params.latencyReport()),
         balcl::OccurrenceInfo(params.latencyReport())},
        {"d|dumpmsg",
         "dumpmsg",
         "dump received message content",
         balcl::TypeInfo(&params.dumpMsg()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"c|confirmmsg",
         "confirmmsg",
         "confirm received message",
         balcl::TypeInfo(&params.confirmMsg()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"e|eventsize",
         "evtSize",
         "number of messages per event",
         balcl::TypeInfo(&params.eventSize()),
         balcl::OccurrenceInfo(params.eventSize())},
        {"m|msgsize",
         "msgSize",
         "payload size of posted messages in bytes",
         balcl::TypeInfo(&params.msgSize()),
         balcl::OccurrenceInfo(params.msgSize())},
        {"r|postrate",
         "rate",
         "number of events to post at each 'postinterval'",
         balcl::TypeInfo(&params.postRate()),
         balcl::OccurrenceInfo(params.postRate())},
        {"eventscount",
         "events",
         "If there is an 's' or 'S' at the end, duration in seconds to post; "
         "otherwise, number of events to post (0 for unlimited)",
         balcl::TypeInfo(&params.eventsCount()),
         balcl::OccurrenceInfo(params.eventsCount())},
        {"u|maxunconfirmed",
         "unconfirmed",
         "Maximum unconfirmed msgs and bytes (fmt: 'msgs:bytes')",
         balcl::TypeInfo(&params.maxUnconfirmed()),
         balcl::OccurrenceInfo(params.maxUnconfirmed())},
        {"i|postinterval",
         "interval",
         "interval to wait between each post",
         balcl::TypeInfo(&params.postInterval()),
         balcl::OccurrenceInfo(params.postInterval())},
        {"v|verbosity",
         "verbosity",
         "verbosity ([silent, trace, debug, <info>, warning, error, fatal])",
         balcl::TypeInfo(&params.verbosity(), &ParametersVerbosity::isValid),
         balcl::OccurrenceInfo(params.verbosity())},
        {"logFormat",
         "logFormat",
         "log format",
         balcl::TypeInfo(&params.logFormat()),
         balcl::OccurrenceInfo(params.logFormat())},
        {"D|memorydebug",
         "memdebug",
         "use a test allocator to debug memory",
         balcl::TypeInfo(&params.memoryDebug()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"t|threads",
         "threads",
         "number of processing threads to use",
         balcl::TypeInfo(&params.threads()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"shutdownGrace",
         "shutdownGrace",
         "seconds of inactivity (no message posted in auto producer mode, or"
         " no message received in auto consumer mode) before shutting down",
         balcl::TypeInfo(&params.shutdownGrace()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"nosessioneventhandler",
         "noSessionEventHandler",
         "use custom event handler threads",
         balcl::TypeInfo(&params.noSessionEventHandler()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"s|storage",
         "storage",
         "path to storage files to open",
         balcl::TypeInfo(&params.storage()),
         balcl::OccurrenceInfo(params.storage())},
        {"log",
         "log",
         "path to log file",
         balcl::TypeInfo(&params.log()),
         balcl::OccurrenceInfo(params.log())},
        {"profile",
         "profile",
         "path to profile in JSON format",
         balcl::TypeInfo(&dummyProfile),
         balcl::OccurrenceInfo(dummyProfile)},
        {"h|help",
         "help",
         "show the help message",
         balcl::TypeInfo(&showHelp),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"dumpprofile",
         "dumpprofile",
         "Dump the JSON profile of the parameters and exit",
         balcl::TypeInfo(&dumpProfile),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"messagepattern",
         "sequential message pattern",
         "printf-like format for sequential message payload content",
         balcl::TypeInfo(&params.sequentialMessagePattern()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"messageProperties",
         "MessageProperties",
         "MessageProperties",
         balcl::TypeInfo(&jsonMessageProperties),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"subscriptions",
         "Subscriptions",
         "Subscriptions",
         balcl::TypeInfo(&jsonSubscriptions),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"autoPubSubModulo",
         "autoPubSubModulo",
         "autoPubSubModulo",
         balcl::TypeInfo(&params.autoPubSubModulo()),
         balcl::OccurrenceInfo::e_OPTIONAL}};

    balcl::CommandLine commandLine(specTable);
    if (commandLine.parse(argc, argv) != 0 || showHelp) {
        commandLine.printUsage();
        return false;  // RETURN
    }

    bsl::string error;

    if (jsonMessageProperties.length() > 0) {
        if (!InputUtil::parseCommand(&params.messageProperties(),
                                     &error,
                                     jsonMessageProperties)) {
            bsl::cerr << "Invalid message properties: "
                      << jsonMessageProperties << "\n"
                      << error << "\n";
            return false;  // RETURN
        }
    }

    if (jsonSubscriptions.length() > 0) {
        if (!InputUtil::parseCommand(&params.subscriptions(),
                                     &error,
                                     jsonSubscriptions)) {
            bsl::cerr << "Invalid subscriptions: " << jsonSubscriptions << "\n"
                      << error << "\n";
            return false;  // RETURN
        }
    }

    if (!(parameters->from(bsl::cerr, params))) {
        return false;  // RETURN
    }

    // Post parsing validation
    if (!parameters->validate(&error)) {
        bsl::cerr << "Invalid parameters:\n" << error << "\n";
        return false;  // RETURN
    }

    if (commandLine.isSpecified("sequential message pattern")) {
        if (commandLine.isSpecified("msgSize")) {
            bsl::cerr << "'--msgsize' and '--messagepattern' are mutually "
                         "exclusive parameters!"
                      << bsl::endl;
            return false;  // RETURN
        }
        if (commandLine.isSpecified("latency")) {
            bsl::cerr << "'--latency' and '--messagepattern' are mutually "
                         "exclusive parameters!"
                      << bsl::endl;
            return false;  // RETURN
        }
    }

    if (dumpProfile) {
        // DumpProfile option can be used to quickly see the default, and also
        // as an initial generator for the profile JSON to feed in to the
        // '--profile' argument.

        bmqu::MemOutStream     os;
        baljsn::Encoder        encoder;
        baljsn::EncoderOptions options;

        options.setEncodingStyle(baljsn::EncoderOptions::e_PRETTY);
        options.setInitialIndentLevel(0);
        options.setSpacesPerLevel(2);

        encoder.encode(os, params, options);

        bsl::cout << os.str() << "\n";
        return false;  // RETURN
    }

    return true;
}

// ====
// main
// ====

int main(int argc, const char* argv[])
{
    ignoreSigpipe();

    // Make TimeUtil thread-safe by calling initialize
    bsls::TimeUtil::initialize();

    // Prepare the UriParser regexp
    bmqt::UriParser::initialize();

    // Test allocator
    balst::StackTraceTestAllocator stta;
    stta.setFailureHandler(&stta.failNoop);
    bslma::TestAllocator ta("TestAllocator", &stta);
    ta.setNoAbort(true);
    bslma::TestAllocatorMonitor tam(&ta);

    // Parameters parsing
    Parameters parameters(bslma::Default::allocator());
    if (!parseArgs(&parameters, argc, argv)) {
        return 1;  // RETURN
    }

    if (parameters.mode() == ParametersMode::e_SYSCHK) {
        return Application::syschk(parameters);  // RETURN
    }

    bool isInteractive = parameters.mode() == ParametersMode::e_CLI ||
                         parameters.mode() == ParametersMode::e_STORAGE;

    ShutdownContext shutdownContext;
    s_shutdownContext_p = &shutdownContext;

    // If we are running in interactive mode, we don't want to intercept
    // ctrl-C, the application will exit on ctrl-D from the stdin stream
    if (!isInteractive) {
        // Set up signal handler
        signal(SIGINT, shutdownApp);
        signal(SIGQUIT, shutdownApp);
        signal(SIGTERM, shutdownApp);
    }

    bslma::Allocator* allocator = parameters.memoryDebug()
                                      ? &ta
                                      : bslma::Default::allocator();

    Application app(parameters, &shutdownContext.d_appSemaphore, allocator);
    if (app.start() != 0) {
        return 2;  // RETURN
    }
    shutdownContext.d_appSemaphore.wait();
    app.stop();

    // Memory debugging
    if (parameters.memoryDebug()) {
        int bytesLeaked = ta.numBytesInUse();
        if (bytesLeaked > 0) {
            bsl::cerr << bytesLeaked << " bytes of memory were leaked!"
                      << "\n";
        }
    }

    return 0;
}
