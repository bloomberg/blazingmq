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

// m_bmqtool_storageinspector.h                                       -*-C++-*-
#ifndef INCLUDED_M_BMQTOOL_STORAGEINSPECTOR
#define INCLUDED_M_BMQTOOL_STORAGEINSPECTOR

//@PURPOSE: Provide a command line interface to inspect BlazingMQ storage.
//
//@CLASSES:
//  m_bmqtool::StorageInspector: CLI to inspect BlazingMQ storage.
//
//@DESCRIPTION: 'm_bmqtool_storageinspector' provides a command line interface
// to inspect BlazingMQ storage.

// BMQTOOL
#include <m_bmqtool_messages.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_qlistfileiterator.h>
#include <mqbu_storagekey.h>

// BDE
#include <ball_log.h>
#include <bsl_map.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>

namespace BloombergLP {

namespace m_bmqtool {

// ===================================
// struct StorageInspector_AppIdRecord
// ===================================

struct StorageInspector_AppIdRecord {
    // DATA
    bsl::string      d_appId;
    mqbu::StorageKey d_appKey;

    // CREATORS
    StorageInspector_AppIdRecord()
    : d_appId()
    , d_appKey()
    {
    }

    StorageInspector_AppIdRecord(const bsl::string&      appId,
                                 const mqbu::StorageKey& appKey)
    : d_appId(appId)
    , d_appKey(appKey)
    {
    }
};

// ===================================
// struct StorageInspector_QueueRecord
// ===================================

struct StorageInspector_QueueRecord {
    // TYPES
    typedef StorageInspector_AppIdRecord AppIdRecord;

    // DATA
    mqbu::StorageKey         d_queueKey;
    bsl::vector<AppIdRecord> d_appIds;
};

// ======================
// class StorageInspector
// ======================

/// This component provides a command line interface to inspect BlazingMQ
/// storage.
class StorageInspector {
  private:
    // PRIVATE TYPES
    BALL_LOG_SET_CLASS_CATEGORY("BMQTOOL.STORAGEINSPECTOR");

  private:
    // PRIVATE TYPES
    typedef StorageInspector_AppIdRecord AppIdRecord;
    typedef StorageInspector_QueueRecord QueueRecord;

    /// QLIST file may contain multiple CREATION/ADDITION records for the
    /// same queue uri, hence a multimap instead of just a map.  The 'queue
    /// key' field is used to differentiate among various records having
    /// same queue uri.
    typedef bsl::multimap<bsl::string, QueueRecord> QueuesMap;
    typedef QueuesMap::iterator                     QueuesMapIter;
    typedef QueuesMap::const_iterator               QueuesMapConstIter;
    typedef bsl::pair<QueuesMapIter, QueuesMapIter> IterPair;

  private:
    // DATA
    bool d_isOpen;

    QueuesMap d_queues;

    bool d_qlistFileRead;

    bsl::string d_dataFile;

    bsl::string d_qlistFile;

    bsl::string d_journalFile;

    mqbs::MappedFileDescriptor d_dataFd;

    mqbs::MappedFileDescriptor d_journalFd;

    mqbs::MappedFileDescriptor d_qlistFd;

    mqbs::DataFileIterator d_dataFileIter;

    mqbs::JournalFileIterator d_journalFileIter;

    mqbs::QlistFileIterator d_qlistFileIter;

  private:
    // PRIVATE ACCESSORS
    void printHelp() const;

    // PRIVATE MANIPULATORS

    /// Process the specified `command`.
    void processCommand(const OpenStorageCommand& command);
    void processCommand(const CloseStorageCommand& command);
    void processCommand(const MetadataCommand& command);
    void processCommand(const ListQueuesCommand& command);
    void processCommand(const DumpQueueCommand& command);
    void processCommand(const DataCommand& command);
    void processCommand(const QlistCommand& command);
    void processCommand(JournalCommand& command);

    void readQueuesIfNeeded();

  public:
    // CREATORS

    /// Constructor using the specified `parameters` and `allocator`.
    StorageInspector(bslma::Allocator* allocator);

    ~StorageInspector();

    // MANIPULATORS

    /// Initialize this instance.
    int initialize();

    /// main loop; read from stdin, parse JSON commands and executes them.
    int mainLoop();
};

}  // close package namespace
}  // close enterprise namespace

#endif
