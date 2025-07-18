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

#ifndef INCLUDED_M_BMQSTORAGETOOL_PARAMETERS
#define INCLUDED_M_BMQSTORAGETOOL_PARAMETERS

//@PURPOSE: Provide a class holding command line parameters for
//'bmqstoragetool'.
//
//@CLASSES:
// m_bmqtool::Parameters: holds all parameter values.
//
//@DESCRIPTION: This component provides a value-semantic type holding the
// command-line parameters for the 'bmqtool' program.

// bmqstoragetool
#include <m_bmqstoragetool_compositesequencenumber.h>
#include <m_bmqstoragetool_queuemap.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>

// BMQ
#include <bmqu_stringutil.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// ==========================
// class CommandLineArguments
// ==========================

class CommandLineArguments {
  public:
    // PUBLIC DATA

    /// Record types constants
    static const char* k_ALL_TYPE;
    static const char* k_MESSAGE_TYPE;
    static const char* k_QUEUEOP_TYPE;
    static const char* k_JOURNALOP_TYPE;
    static const char* k_CSL_ALL_TYPE;
    static const char* k_CSL_SNAPSHOT_TYPE;
    static const char* k_CSL_UPDATE_TYPE;
    static const char* k_CSL_COMMIT_TYPE;
    static const char* k_CSL_ACK_TYPE;
    /// Print modes constants
    static const char* k_HUMAN_MODE;
    static const char* k_JSON_PRETTY_MODE;
    static const char* k_JSON_LINE_MODE;
    /// List of record types to process (message, journalOp, queueOp)
    bsl::vector<bsl::string> d_recordType;
    /// List of CSL record types to process (snapshot, update, commit, ack)
    bsl::vector<bsl::string> d_cslRecordType;
    /// Filter messages by minimum timestamp
    bsls::Types::Int64 d_timestampGt;
    /// Filter messages by maximum timestamp
    bsls::Types::Int64 d_timestampLt;
    /// Filter messages by minimum record composite sequence number
    bsl::string d_seqNumGt;
    /// Filter messages by maximum record composite sequence number
    bsl::string d_seqNumLt;
    /// Filter messages by minimum record offset
    bsls::Types::Int64 d_offsetGt;
    /// Filter messages by maximum record offset
    bsls::Types::Int64 d_offsetLt;
    /// Path to find all files from
    bsl::string d_journalPath;
    /// Path to read journal files from
    bsl::string d_journalFile;
    /// Path to read data files from
    bsl::string d_dataFile;
    /// Path to read CSL files from
    bsl::string d_cslFile;
    /// If true force to iterate CSL file from the beginning, otherwise iterate
    /// from the latest snapshot
    bool d_cslFromBegin;
    /// Print mode
    bsl::string d_printMode;
    /// Filter messages by message guids
    bsl::vector<bsl::string> d_guid;
    /// Filter messages by record composite sequence numbers
    bsl::vector<bsl::string> d_seqNum;
    /// Filter messages by record offsets
    bsl::vector<bsls::Types::Int64> d_offset;
    /// Filter messages by queue keys
    bsl::vector<bsl::string> d_queueKey;
    /// Filter messages by queue names
    bsl::vector<bsl::string> d_queueName;
    /// Limit number of bytes to
    int d_dumpLimit;
    /// Print message details
    bool d_details;
    /// Print message payload
    bool d_dumpPayload;
    /// Print summary of messages
    bool d_summary;
    /// Show only outstanding messages (not deleted)
    bool d_outstanding;
    /// Show only messages, confirmed by all the appId's
    bool d_confirmed;
    /// Show only messages, confirmed by some of the appId's
    bool d_partiallyConfirmed;
    /// Min number of records per queue for detailed info to be displayed
    bsls::Types::Int64 d_minRecordsPerQueue;
    /// Limit number of queues to display in CSL file summary
    int d_cslSummaryQueuesLimit;

    // CREATORS
    explicit CommandLineArguments(bslma::Allocator* allocator = 0);

    // MANIPULATORS
    /// Validate the consistency of all settings.
    bool validate(bsl::string* error, bslma::Allocator* allocator = 0);

  private:
    // PRIVATE MANIPULATORS

    /// Validate journal mode arguments. Write validation error into the
    /// specified `stream`.
    void validateJournalModeArgs(bsl::ostream&     stream,
                                 bslma::Allocator* allocator = 0);

    // PRIVATE ACCESSORS

    /// Validate CSL mode arguments. Write validation error into the specified
    /// `stream`.
    void validateCslModeArgs(bsl::ostream&     stream,
                             bslma::Allocator* allocator = 0);
    /// Validate range args. Return true if at least one range argument passed,
    /// false otherwise.
    bool validateRangeArgs(bsl::ostream&     error,
                           bslma::Allocator* allocator) const;

  public:
    // CLASS METHODS
    /// Return true if the specified `recordType` is valid, false otherwise.
    /// Error message is written into the specified `stream` if `recordType` is
    /// invalid.
    static bool isValidRecordType(const bsl::string* recordType,
                                  bsl::ostream&      stream);
    /// Return true if the specified `cslRecordType` is valid, false otherwise.
    /// Error message is written into the specified `stream` if `cslRecordType`
    /// is invalid.
    static bool isValidCslRecordType(const bsl::string* cslRecordType,
                                     bsl::ostream&      stream);
    /// Return true if the specified `printMode` is valid, false otherwise.
    /// Error message is written into the specified `stream` if `printMode` is
    /// invalid.
    static bool isValidPrintMode(const bsl::string* printMode,
                                 bsl::ostream&      stream);
    /// Return true if the specified `fileName` is valid (file exists), false
    /// otherwise. Error message is written into the specified `stream` if
    /// `fileName` is invalid.
    static bool isValidFileName(const bsl::string* fileName,
                                bsl::ostream&      stream);
};

// =================
// struct Parameters
// =================

struct Parameters {
    // PUBLIC TYPES

    /// Enum with available printing modes
    enum PrintMode { e_HUMAN, e_JSON_PRETTY, e_JSON_LINE };

    /// VST representing search range parameters
    struct Range {
        // PUBLIC DATA

        /// Filter messages greater than timestamp value
        bsl::optional<bsls::Types::Uint64> d_timestampGt;
        /// Filter messages less than timestamp value
        bsl::optional<bsls::Types::Uint64> d_timestampLt;
        /// Filter messages greater than offset value
        bsl::optional<bsls::Types::Uint64> d_offsetGt;
        /// Filter messages less than offset value
        bsl::optional<bsls::Types::Uint64> d_offsetLt;
        /// Filter messages greater than sequence number
        bsl::optional<CompositeSequenceNumber> d_seqNumGt;
        /// Filter messages less than sequence number
        bsl::optional<CompositeSequenceNumber> d_seqNumLt;

        // CREATORS
        /// Default constructor
        explicit Range();
    };

    // VST representing record types to process
    struct ProcessRecordTypes {
        // PUBLIC DATA

        /// Flag to process records of type message
        bool d_message;
        /// Flag to process records of type queueOp
        bool d_queueOp;
        /// Flag to process records of type journalOp
        bool d_journalOp;

        // CREATORS
        explicit ProcessRecordTypes();

        // MANIPULATORS
        /// Set all record types to process
        void setAll();

        bool operator==(ProcessRecordTypes const& other) const;
    };

    // VST representing CSL record types to process
    struct ProcessCslRecordTypes {
        // PUBLIC DATA

        /// Flag to process CSL records of type snapshot
        bool d_snapshot;
        /// Flag to process CSL records of type d_update
        bool d_update;
        /// Flag to process CSL records of type commit
        bool d_commit;
        /// Flag to process CSL records of type ack
        bool d_ack;

        // CREATORS
        explicit ProcessCslRecordTypes();

        // MANIPULATORS
        /// Set all CSL record types to process
        void setAll();

        bool operator==(ProcessCslRecordTypes const& other) const;
    };

    // PUBLIC DATA

    /// Flag to process CSL file instead of Journal one
    bool d_cslMode;
    /// Record types to process
    ProcessRecordTypes d_processRecordTypes;
    /// CSL record types to process
    ProcessCslRecordTypes d_processCslRecordTypes;
    /// Queue map containing uri to key and key to info mappings
    QueueMap d_queueMap;
    /// Print mode
    PrintMode d_printMode;
    /// Range parameters for filtering
    Range d_range;
    /// Filter messages by message guids
    bsl::vector<bsl::string> d_guid;
    /// Filter messages by message sequence number
    bsl::vector<CompositeSequenceNumber> d_seqNum;
    /// Filter messages by message offsets
    bsl::vector<bsls::Types::Int64> d_offset;
    /// Filter messages by queue keys
    bsl::vector<bsl::string> d_queueKey;
    /// Filter messages by queue names
    bsl::vector<bsl::string> d_queueName;
    /// Limit number of bytes to dump
    unsigned int d_dumpLimit;
    /// Print message details
    bool d_details;
    /// Print message payload
    bool d_dumpPayload;
    /// Print summary of messages
    bool d_summary;
    /// Show only outstanding messages (not deleted)
    bool d_outstanding;
    /// Show only messages, confirmed by all the appId's
    bool d_confirmed;
    /// Show only messages, confirmed by some of the appId's
    bool d_partiallyConfirmed;
    /// Min number of records per queue for detailed info to be displayed
    bsl::optional<bsls::Types::Uint64> d_minRecordsPerQueue;
    /// Limit number of queues to display in CSL file summary
    unsigned int d_cslSummaryQueuesLimit;

    // CREATORS
    /// Constructor from the specified 'aruments'
    explicit Parameters(const CommandLineArguments& aruments,
                        bslma::Allocator*           allocator = 0);

    void validateQueueNames(bslma::Allocator* allocator = 0) const;
};

}  // close package namespace

}  // close enterprise namespace

#endif
