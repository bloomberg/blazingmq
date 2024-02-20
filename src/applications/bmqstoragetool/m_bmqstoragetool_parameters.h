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

// m_bmqstoragetool_parameters.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_PARAMETERS
#define INCLUDED_M_BMQSTORAGETOOL_PARAMETERS

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

// bmqstoragetool
#include <m_bmqstoragetool_queuemap.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>

// MWC
#include <mwcu_stringutil.h>

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
    bsls::Types::Int64 d_timestampGt;
    // Filter messages by minimum timestamp
    bsls::Types::Int64 d_timestampLt;
    // Filter messages by maximum timestamp
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

struct Parameters {
    // PUBLIC DATA
    bsls::Types::Int64 d_timestampGt;
    // Filter messages by minimum timestamp
    bsls::Types::Int64 d_timestampLt;
    // Filter messages by maximum timestamp
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
    // Queue map containing uri to key and key to info mappings

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
