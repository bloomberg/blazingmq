// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbs_datastore.cpp                                                 -*-C++-*-
#include <mqbs_datastore.h>

#include <mqbscm_version.h>
// BDE
#include <bsl_cstddef.h>
#include <bsl_ostream.h>
#include <bslim_printer.h>
#include <bslmf_assert.h>

namespace BloombergLP {
namespace mqbs {

namespace {
// Compile-time assertions for size of 'mqbs::DataStoreRecordHandle'.
BSLMF_ASSERT(sizeof(DataStoreRecordHandle) ==
             sizeof(DataStoreConfig::RecordIterator));
}  // close unnamed namespace

// ---------------------
// class DataStoreConfig
// ---------------------

// CREATORS
DataStoreConfig::DataStoreConfig()
: d_bufferFactory_p(0)
, d_scheduler_p(0)
, d_preallocate(false)
, d_prefaultPages(false)
, d_location()
, d_archiveLocation()
, d_nodeId(-1)
, d_partitionId(-1)
, d_maxDataFileSize(0)
, d_maxJournalFileSize(0)
, d_maxQlistFileSize(0)
, d_maxArchivedFileSets(0)
{
    // NOTHING
}

// ACCESSORS
bsl::ostream& DataStoreConfig::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();

    printer.printAttribute("nodeId", nodeId());
    printer.printAttribute("partitionId", partitionId());
    printer.printAttribute("location", location());
    printer.printAttribute("archiveLocation", archiveLocation());
    printer.printAttribute("clusterName", clusterName());
    printer.printAttribute("preallocate",
                           (hasPreallocate() ? "true" : "false"));
    printer.printAttribute("prefaultPages",
                           (hasPrefaultPages() ? "true" : "false"));
    printer.printAttribute("maxDataFileSize", maxDataFileSize());
    printer.printAttribute("maxQlistFileSize", maxQlistFileSize());
    printer.printAttribute("maxJournalFileSize", maxJournalFileSize());
    printer.printAttribute("hasRecoveredQueuesCb",
                           (recoveredQueuesCb() ? "yes" : "no"));
    printer.printAttribute("maxArchiveFileSets", maxArchivedFileSets());
    printer.end();
    return stream;
}

// ---------------
// class DataStore
// ---------------

/// Force variable/symbol definition so that it can be used in other files
const int DataStore::k_INVALID_PARTITION_ID;

DataStore::~DataStore()
{
    // NOTHING
}

bsl::ostream& DataStoreRecordKey::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sequenceNum", d_sequenceNum);
    printer.printAttribute("primaryLeaseId", d_primaryLeaseId);
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
