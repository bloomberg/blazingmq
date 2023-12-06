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
#include <mqbs_filestoreprotocol.h>

// MWC
#include <mwcu_stringutil.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// ================
// class Parameters
// ================

/// Holds command line parameters for bmqstoragetool.
class Parameters {
  public:
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

  public:
    // CREATORS
    /// Default constructor
    Parameters(bslma::Allocator* allocator);

    // MANIPULATORS
    bsls::Types::Int64& setTimestampGt();
    bsls::Types::Int64& setTimestampLt();
    bsl::string& setPath();
    bsl::string& setJournalFile();
    bsl::string& setDataFile();
    bsl::string& setCslFile();
    bsl::vector<bsl::string>& setGuid();
    bsl::vector<bsl::string>& setQueueKey();
    bsl::vector<bsl::string>& setQueueName();
    int& setDumpLimit();
    bool& setDetails();
    bool& setDumpPayload();
    bool& setSummary();
    bool& setOutstanding();
    bool& setConfirmed();
    bool& setPartiallyConfirmed();

    /// Validate the consistency of all settings.
    bool validate(bsl::string* error);

    // ACCESSORS
    /// Print all the parameters
    void print(bsl::ostream& ss) const;
    bsls::Types::Int64 timestampGt() const;
    bsls::Types::Int64 timestampLt() const;
    bsl::string path() const;
    bsl::string journalFile() const;
    bsl::string dataFile() const;
    bsl::string cslFile() const;
    bsl::vector<bsl::string> guid() const;
    bsl::vector<bsl::string> queueKey() const;
    bsl::vector<bsl::string> queueName() const;
    int dumpLimit() const;
    bool details() const;
    bool dumpPayload() const;
    bool summary() const;
    bool outstanding() const;
    bool confirmed() const;
    bool partiallyConfirmed() const;
};

}  // close package namespace

}  // close enterprise namespace

#endif
