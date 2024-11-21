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
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// ================
// class Parameters
// ================

struct CommandLineArguments {
    // PUBLIC DATA
    static const char* k_MESSAGE_TYPE;
    static const char* k_QUEUEOP_TYPE;
    static const char* k_JOURNAL_TYPE;
    // Record types constants
    bsl::vector<bsl::string> d_recordType;
    // List of record types to process (message, journalOp, queueOp)
    bsls::Types::Int64 d_timestampGt;
    // Filter messages by minimum timestamp
    bsls::Types::Int64 d_timestampLt;
    // Filter messages by maximum timestamp
    bsl::string d_seqNumGt;
    // Filter messages by minimum record composite sequence number
    bsl::string d_seqNumLt;
    // Filter messages by maximum record composite sequence number
    bsls::Types::Int64 d_offsetGt;
    // Filter messages by minimum record offset
    bsls::Types::Int64 d_offsetLt;
    // Filter messages by maximum record offset
    bsl::string d_journalPath;
    // Path to find all files from
    bsl::string d_journalFile;
    // Path to read journal files from
    bsl::string d_dataFile;
    // Path to read data files from
    bsl::string d_cslFile;
    // Path to read CSL files from
    bsl::vector<bsl::string> d_guid;
    // Filter messages by message guids
    bsl::vector<bsl::string> d_seqNum;
    // Filter messages by record composite sequence numbers
    bsl::vector<bsls::Types::Int64> d_offset;
    // Filter messages by record offsets
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
    bool validate(bsl::string* error, bslma::Allocator* allocator = 0);
};

struct Parameters {
    // PUBLIC TYPES

    // VST representing search range parameters
    struct Range {
        // PUBLIC TYPES
        enum Type {
            e_NONE         = 0,
            e_TIMESTAMP    = 1,
            e_SEQUENCE_NUM = 2,
            e_OFFSET       = 3
        };

        // PUBLIC DATA
        Type d_type;
        /// Range type
        bsls::Types::Uint64 d_timestampGt;
        // Filter messages greater than timestamp value
        bsls::Types::Uint64 d_timestampLt;
        // Filter messages less than timestamp value
        bsls::Types::Uint64 d_offsetGt;
        // Filter messages greater than offset value
        bsls::Types::Uint64 d_offsetLt;
        // Filter messages less than offset value
        CompositeSequenceNumber d_seqNumGt;
        // Filter messages greater than sequence number
        CompositeSequenceNumber d_seqNumLt;
        // Filter messages less than sequence number

        // CREATORS
        explicit Range();
    };

    // VST representing record types to process
    struct ProcessRecordTypes {
        // PUBLIC DATA
        bool d_message;
        // Flag to process records of type message
        bool d_queueOp;
        // Flag to process records of type queueOp
        bool d_journalOp;
        // Flag to process records of type journalOp

        // CREATORS
        explicit ProcessRecordTypes(bool enableDefault = true);
    };

    // PUBLIC DATA
    ProcessRecordTypes d_processRecordTypes;
    // Record types to process
    QueueMap d_queueMap;
    // Queue map containing uri to key and key to info mappings
    Range d_range;
    // Range parameters for filtering
    bsl::vector<bsl::string> d_guid;
    // Filter messages by message guids
    bsl::vector<CompositeSequenceNumber> d_seqNum;
    // Filter messages by message sequence number
    bsl::vector<bsls::Types::Int64> d_offset;
    // Filter messages by message offsets
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

    // CREATORS
    /// Default constructor
    explicit Parameters(bslma::Allocator* allocator = 0);
    /// Constructor from the specified 'aruments'
    explicit Parameters(const CommandLineArguments& aruments,
                        bslma::Allocator*           allocator = 0);

    void validateQueueNames(bslma::Allocator* allocator = 0) const;
};

}  // close package namespace

}  // close enterprise namespace

#endif
