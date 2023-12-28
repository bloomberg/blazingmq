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

// m_bmqtool_parameters.h                                             -*-C++-*-
#ifndef INCLUDED_M_BMQTOOL_PARAMETERS
#define INCLUDED_M_BMQTOOL_PARAMETERS

//@PURPOSE: Provide a class holding command line parameters for 'bmqtool'.
//
//@CLASSES:
//  m_bmqtool::ParametersVerbosity: enum for verbosity mode.
//  m_bmqtool::ParametersMode     : enum for the tool mode.
//  m_bmqtool::ParametersLatency  : enum for latency mode.
//  m_bmqtool::Parameters         : holds all parameter values
//
//@DESCRIPTION: This component provides a value-semantic type holding the
// command-line parameters for the 'bmqtool' program.

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bsls_types.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MWC
#include <mwcu_stringutil.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// ================
// class Parameters
// ================

struct CommandLineArguments {
    // PUBLIC DATA
    bsls::Types::Int64 d_timestampGt;
    // Filter messages by minimum timestamp
    bsls::Types::Int64 d_timestampLt;
    // Filter messages by maximum timestamp
    bsl::string d_path;
    // Path to find all files from
    bsl::string d_journalFile;
    // Path to read journal files from
    bsl::string d_dataFile;
    // Path to read data files from
    bsl::string d_cslFile;
    // Path to read CSL files from
    bsl::vector<bsl::string> d_guid;
    // Filter messages by message guids
    bsl::vector<bsl::string> d_queueKey;
    // Filter messages by queue keys
    bsl::vector<bsl::string> d_queueName;
    // Filter messages by queue names
    int d_dumpLimit;
    // Limit number of bytes to
    bool d_details;
    // Print message details
    bool d_dumpPayload;
    // Print message payload
    bool d_summary;
    // Print summary of messages
    bool d_outstanding;
    // Show only outstanding messages (not deleted)
    bool d_confirmed;
    // Show only messages, confirmed by all the appId's
    bool d_partiallyConfirmed;
    // Show only messages, confirmed by some of the appId's

    // CREATORS
    explicit CommandLineArguments(bslma::Allocator* allocator = 0);

    // MANIPULATORS
    /// Validate the consistency of all settings.
    bool validate(bsl::string* error);
};

/// Holds command line parameters for bmqstoragetool.
class Parameters {
  public:
    // PUBLIC TYPES

    template <typename ITER>
    class FileHandler {
        const bsl::string          d_path;
        ITER                       d_iter;
        mqbs::MappedFileDescriptor d_mfd;

      public:
        // CREATORS
        explicit FileHandler(const bsl::string& path,
                             bslma::Allocator*  allocator = 0);

        ~FileHandler();

        // ACCESSORS
        /// File path
        bsl::string path() const;

        // MANIPULATORS
        /// iterator resetter
        bool resetIterator(bsl::ostream& errorDescription);

        // TODO: used for testing, consider better way
        void setIterator(ITER* iter);

        /// Mapped file iterator
        ITER* iterator();
    };

    typedef bsl::unordered_map<mqbu::StorageKey, bmqp_ctrlmsg::QueueInfo>
        QueueKeyToInfoMap;
    typedef bsl::unordered_map<bsl::string, mqbu::StorageKey> QueueUriToKeyMap;

    class QueueMap {
        QueueKeyToInfoMap d_queueKeyToInfoMap;
        QueueUriToKeyMap  d_queueUriToKeyMap;

      public:
        // CREATORS
        explicit QueueMap(bslma::Allocator* allocator);

        // MANIPULATORS
        QueueKeyToInfoMap& queueKeyToInfoMap();
        // Return reference to modifiable key to info map

        QueueUriToKeyMap& queueUriToKeyMap();
        // Return reference to modifiable Uri to key map

        // ACCESSORS
        const QueueKeyToInfoMap& queueKeyToInfoMap() const;
        // Return reference to non-modifiable key to info map

        const QueueUriToKeyMap& queueUriToKeyMap() const;
        // Return reference to non-modifiable Uri to key map
    };

  private:
    bsls::Types::Int64 d_timestampGt;
    // Filter messages by minimum timestamp
    bsls::Types::Int64 d_timestampLt;
    // Filter messages by maximum timestamp
    FileHandler<mqbs::JournalFileIterator> d_journalFile;
    // Handler of journal file
    FileHandler<mqbs::DataFileIterator> d_dataFile;
    // Handler of data file
    bsl::string d_cslFile;
    // CSL file path
    bsl::vector<bsl::string> d_guid;
    // Filter messages by message guids
    bsl::vector<bsl::string> d_queueKey;
    // Filter messages by queue keys
    bsl::vector<bsl::string> d_queueName;
    // Filter messages by queue names
    unsigned int d_dumpLimit;
    // Limit number of bytes to dump
    bool d_details;
    // Print message details
    bool d_dumpPayload;
    // Print message payload
    bool d_summary;
    // Print summary of messages
    bool d_outstanding;
    // Show only outstanding messages (not deleted)
    bool d_confirmed;
    // Show only messages, confirmed by all the appId's
    bool d_partiallyConfirmed;
    // Show only messages, confirmed by some of the appId's
    QueueMap d_queueMap;
    // Queue info

    // MANIPULATORS
    bool buildQueueMap(bsl::ostream& ss, bslma::Allocator* allocator);
    // Build queue map from csl file.

  public:
    // CREATORS
    /// Default constructor
    explicit Parameters(const CommandLineArguments& aruments,
                        bslma::Allocator*           allocator = 0);

    // MANIPULATORS
    FileHandler<mqbs::JournalFileIterator>* journalFile();
    FileHandler<mqbs::DataFileIterator>*    dataFile();
    // TODO: used for testing, find better way
    QueueMap& queueMap();

    // ACCESSORS
    bsls::Types::Int64       timestampGt() const;
    bsls::Types::Int64       timestampLt() const;
    bsl::vector<bsl::string> guid() const;
    bsl::vector<bsl::string> queueKey() const;
    bsl::vector<bsl::string> queueName() const;
    unsigned int             dumpLimit() const;
    bool                     details() const;
    bool                     dumpPayload() const;
    bool                     summary() const;
    bool                     outstanding() const;
    bool                     confirmed() const;
    bool                     partiallyConfirmed() const;
    const QueueMap&          queueMap() const;

    // MEMBER FUNCTIONS
    /// Print all the parameters
    void print(bsl::ostream& ss) const;
};

}  // close package namespace

}  // close enterprise namespace

#endif
