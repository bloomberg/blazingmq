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

// mqbs_filestoreset.cpp                                              -*-C++-*-
#include <mqbs_filestoreset.h>

#include <mqbscm_version.h>
// MWC
#include <mwcu_printutil.h>

// BDE
#include <bslim_printer.h>
#include <bsls_annotation.h>

namespace BloombergLP {
namespace mqbs {

namespace {

/// Pretty print the specificied `size` to the specified `stream` using the
/// optionally specified `level` and `spacesPerLevel`.
static bsl::ostream& prettyPrintSize(bsl::ostream&              stream,
                                     const bsls::Types::Uint64& size,
                                     BSLS_ANNOTATION_UNUSED int level = 0,
                                     int spacesPerLevel               = 4)
{
    stream << mwcu::PrintUtil::prettyNumber(
        static_cast<bsls::Types::Int64>(size));
    if (spacesPerLevel >= 0) {
        stream << "\n";
    }

    return stream;
}

}  // close unnamed namespace

// ------------------
// class FileStoreSet
// ------------------

// MANIPULATORS
void FileStoreSet::reset()
{
    d_dataFile.clear();
    d_dataFileSize = 0;
    d_journalFile.clear();
    d_journalFileSize = 0;
    d_qlistFile.clear();
    d_qlistFileSize = 0;
}

// ACCESSORS
bsl::ostream&
FileStoreSet::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("dataFile", d_dataFile);
    printer.printForeign(d_dataFileSize, &prettyPrintSize, "dataFileSize");
    printer.printAttribute("qlistFile", d_qlistFile);
    printer.printForeign(d_qlistFileSize, &prettyPrintSize, "qlistFileSize");
    printer.printAttribute("journalFile", d_journalFile);
    printer.printForeign(d_journalFileSize,
                         &prettyPrintSize,
                         "journalFileSize");
    printer.end();
    return stream;
}

}  // close package namespace
}  // close enterprise namespace
