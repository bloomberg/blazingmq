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
#include <m_bmqstoragetool_messages.h>

// BDE
#include <balcl_commandline.h>
#include <bsl_iostream.h>

using namespace BloombergLP;
using namespace m_bmqstoragetool;

static bool parseArgs(int argc, const char* argv[])
{
    // Parameters is default initialized, get all default values ...
    CommandLineParameters params(bslma::Default::allocator());
    bsl::string           command = "E_SEARCH";

    balcl::OptionInfo specTable[] = {
        {"path",
         "path",
         "file path pattern, where the tool will try to find journal, data "
         "and queue files. Can be a directory or a '*'-ended pattern",
         balcl::TypeInfo(&params.path()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"journal-file",
         "journal file",
         "path to a .bmq_journal file",
         balcl::TypeInfo(&params.journalFile()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"data-file",
         "data file",
         "path to a .bmq_data file",
         balcl::TypeInfo(&params.dataFile()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"qlist-file",
         "qlist file",
         "path to a .bmq_qlist file",
         balcl::TypeInfo(&params.qlistFile()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"guid",
         "guid",
         "message guid",
         balcl::TypeInfo(&params.guid()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"queue",
         "queue",
         "message queue name",
         balcl::TypeInfo(&params.queue()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"timestamp-gt",
         "timestamp grater then",
         "lower timestamp bound",
         balcl::TypeInfo(&params.timestampGt()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"timestamp-lt",
         "timestamp less then",
         "lower timestamp bound",
         balcl::TypeInfo(&params.timestampLt()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"details",
         "details",
         "specify if you need message details",
         balcl::TypeInfo(&params.details()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"dump-payload",
         "dump payload",
         "specify if you need message payload",
         balcl::TypeInfo(&params.dumpPayload()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"dump-limit",
         "dump limit",
         "limit of payload output",
         balcl::TypeInfo(&params.dumpLimit()),
         balcl::OccurrenceInfo::e_OPTIONAL},
        {"",
         "command",
         "command to execute [E_SEARCH, E_METADATA, E_RATIO]",
         balcl::TypeInfo(&command),
         balcl::OccurrenceInfo::e_OPTIONAL}};

    balcl::CommandLine commandLine(specTable);
    if (commandLine.parse(argc, argv) != 0 /*|| showHelp*/) {
        commandLine.printUsage();
        return false;  // RETURN
    }
    bsl::cout << "path :\t" << params.path() << bsl::endl;
    bsl::cout << "journal-file :\t" << params.journalFile() << bsl::endl;
    bsl::cout << "data-file :\t" << params.dataFile() << bsl::endl;
    bsl::cout << "qlist-file :\t" << params.qlistFile() << bsl::endl;
    bsl::cout << "guids :\n";
    for (int i = 0; i < params.guid().size(); ++i) {
        bsl::cout << "[" << i << "] :\t" << params.guid()[i] << bsl::endl;
    }
    bsl::cout << "queue :\n";
    for (int i = 0; i < params.queue().size(); ++i) {
        bsl::cout << "[" << i << "] :\t" << params.queue()[i] << bsl::endl;
    }
    bsl::cout << "timestamp-gt :\t" << params.timestampGt() << bsl::endl;
    bsl::cout << "timestamp-lt :\t" << params.timestampLt() << bsl::endl;
    bsl::cout << "details :\t" << params.details() << bsl::endl;
    bsl::cout << "dump-payload :\t" << params.dumpPayload() << bsl::endl;
    bsl::cout << "dump-limit :\t" << params.dumpLimit() << bsl::endl;

    return true;
}

// ====
// main
// ====

int main(int argc, const char* argv[])
{
    // Parameters parsing
    if (!parseArgs(argc, argv)) {
        return 1;  // RETURN
    }

    return 0;
}
