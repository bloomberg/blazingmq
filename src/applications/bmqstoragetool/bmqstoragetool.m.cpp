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

// bmqstoragetool
#include <m_bmqstoragetool_commandprocessorfactory.h>
#include <m_bmqstoragetool_parameters.h>

// BDE
#include <balcl_commandline.h>
#include <bsl_iostream.h>

using namespace BloombergLP;
using namespace m_bmqstoragetool;

static bool
parseArgs(CommandLineArguments& arguments, int argc, const char* argv[])
{
    bool showHelp = false;

    balcl::OptionInfo specTable[] = {
        {"path",
         "path",
         "'*'-ended file path pattern, where the tool will try to find "
         "journal, data and csl files",
         balcl::TypeInfo(&arguments.d_path),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"journal-file",
         "journal file",
         "path to a .bmq_journal file",
         balcl::TypeInfo(&arguments.d_journalFile),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"data-file",
         "data file",
         "path to a .bmq_data file",
         balcl::TypeInfo(&arguments.d_dataFile),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"csl-file",
         "csl file",
         "path to a .bmq_csl file",
         balcl::TypeInfo(&arguments.d_cslFile),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"guid",
         "guid",
         "message guid",
         balcl::TypeInfo(&arguments.d_guid),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"queue-name",
         "queue name",
         "message queue name",
         balcl::TypeInfo(&arguments.d_queueName),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"queue-key",
         "queue key",
         "message queue key",
         balcl::TypeInfo(&arguments.d_queueKey),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"timestamp-gt",
         "timestamp grater then",
         "lower timestamp bound",
         balcl::TypeInfo(&arguments.d_timestampGt),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"timestamp-lt",
         "timestamp less then",
         "lower timestamp bound",
         balcl::TypeInfo(&arguments.d_timestampLt),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"outstanding",
         "only outstanding",
         "show only outstanding (not deleted) messages",
         balcl::TypeInfo(&arguments.d_outstanding),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"confirmed",
         "only confirmed",
         "show only messages, confirmed by all the appId's",
         balcl::TypeInfo(&arguments.d_confirmed),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"partially-confirmed",
         "only partially confirmed",
         "show only messages, confirmed by some of the appId's",
         balcl::TypeInfo(&arguments.d_partiallyConfirmed),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"details",
         "details",
         "specify if you need message details",
         balcl::TypeInfo(&arguments.d_details),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"dump-payload",
         "dump payload",
         "specify if you need message payload",
         balcl::TypeInfo(&arguments.d_dumpPayload),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"dump-limit",
         "dump limit",
         "limit of payload output",
         balcl::TypeInfo(&arguments.d_dumpLimit),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"summary",
         "summary",
         "summary of all matching messages (number of outstanding messages "
         "and other statistics)",
         balcl::TypeInfo(&arguments.d_summary),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"h|help",
         "help",
         "print usage)",
         balcl::TypeInfo(&showHelp),
         balcl::OccurrenceInfo::e_OPTIONAL}};

    balcl::CommandLine commandLine(specTable);
    if (commandLine.parse(argc, argv) != 0 || showHelp) {
        commandLine.printUsage();
        return false;  // RETURN
    }

    bsl::string error;
    if (!arguments.validate(&error)) {
        bsl::cerr << "Arguments validation failed:\n" << error;
        return false;  // RETURN
    }

    return true;
}

// ====
// main
// ====

int main(int argc, const char* argv[])
{
    // Arguments parsing
    CommandLineArguments arguments;
    if (!parseArgs(arguments, argc, argv)) {
        return 1;  // RETURN
    }

    bsl::unique_ptr<Parameters> parameters;
    try {
        parameters = bsl::make_unique<Parameters>(arguments,
                                                  bslma::Default::allocator());
    }
    catch (const bsl::exception& e) {
        bsl::cerr << e.what();
        return 2;  // RETURN
    }

    bsl::unique_ptr<CommandProcessor> processor =
        CommandProcessorFactory::createCommandProcessor(
            bsl::move(parameters),
            bslma::Default::allocator());

    if (!processor) {
        bsl::cerr << "Failed to create processor";
        return 3;  // RETURN
    }

    processor->process(bsl::cout);

    return 0;
}
