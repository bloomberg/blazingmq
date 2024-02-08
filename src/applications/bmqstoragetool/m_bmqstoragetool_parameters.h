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

// bmqstoragetool
#include <m_bmqstoragetool_queuemap.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filesystemutil.h>
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

class Parameters {
  public:
    // MANIPULATORS
    virtual mqbs::JournalFileIterator* journalFileIterator() = 0;
    virtual mqbs::DataFileIterator*    dataFileIterator()    = 0;

    // ACCESSORS
    virtual bsls::Types::Int64       timestampGt() const        = 0;
    virtual bsls::Types::Int64       timestampLt() const        = 0;
    virtual bsl::vector<bsl::string> guid() const               = 0;
    virtual bsl::vector<bsl::string> queueKey() const           = 0;
    virtual bsl::vector<bsl::string> queueName() const          = 0;
    virtual unsigned int             dumpLimit() const          = 0;
    virtual bool                     details() const            = 0;
    virtual bool                     dumpPayload() const        = 0;
    virtual bool                     summary() const            = 0;
    virtual bool                     outstanding() const        = 0;
    virtual bool                     confirmed() const          = 0;
    virtual bool                     partiallyConfirmed() const = 0;
    virtual const QueueMap&          queueMap() const           = 0;

    // MEMBER FUNCTIONS
    /// Print all the parameters
    virtual void print(bsl::ostream& ss) const = 0;
};

class ParametersReal : public Parameters {
  private:
    // PRIVATE TYPES
    template <typename ITER>
    class FileHandler {
      public:
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

        /// Mapped file iterator
        ITER* iterator();

        /// Mapped file descriptor
        mqbs::MappedFileDescriptor& mappedFileDescriptor();
    };

    // PRIVATE DATA
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
    // Queue map containing uri to key and key to info mappings

    // MANIPULATORS
    bool buildQueueMap(bsl::ostream& ss, bslma::Allocator* allocator);
    // Build queue map from csl file.

  public:
    // CREATORS
    /// Default constructor
    explicit ParametersReal(const CommandLineArguments& aruments,
                            bslma::Allocator*           allocator = 0);

    // MANIPULATORS
    mqbs::JournalFileIterator* journalFileIterator() BSLS_KEYWORD_OVERRIDE;
    mqbs::DataFileIterator*    dataFileIterator() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    bsls::Types::Int64       timestampGt() const BSLS_KEYWORD_OVERRIDE;
    bsls::Types::Int64       timestampLt() const BSLS_KEYWORD_OVERRIDE;
    bsl::vector<bsl::string> guid() const BSLS_KEYWORD_OVERRIDE;
    bsl::vector<bsl::string> queueKey() const BSLS_KEYWORD_OVERRIDE;
    bsl::vector<bsl::string> queueName() const BSLS_KEYWORD_OVERRIDE;
    unsigned int             dumpLimit() const BSLS_KEYWORD_OVERRIDE;
    bool                     details() const BSLS_KEYWORD_OVERRIDE;
    bool                     dumpPayload() const BSLS_KEYWORD_OVERRIDE;
    bool                     summary() const BSLS_KEYWORD_OVERRIDE;
    bool                     outstanding() const BSLS_KEYWORD_OVERRIDE;
    bool                     confirmed() const BSLS_KEYWORD_OVERRIDE;
    bool                     partiallyConfirmed() const BSLS_KEYWORD_OVERRIDE;
    const QueueMap&          queueMap() const BSLS_KEYWORD_OVERRIDE;

    // MEMBER FUNCTIONS
    /// Print all the parameters
    void print(bsl::ostream& ss) const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

template <typename ITER>
inline ParametersReal::FileHandler<ITER>::FileHandler(
    const bsl::string& path,
    bslma::Allocator*  allocator)
: d_path(path, allocator)
{
    // NOTHING
}

template <typename ITER>
inline ParametersReal::FileHandler<ITER>::~FileHandler()
{
    d_iter.clear();
    if (d_mfd.isValid()) {
        mqbs::FileSystemUtil::close(&d_mfd);
    }
}

template <typename ITER>
inline bsl::string ParametersReal::FileHandler<ITER>::path() const
{
    return d_path;
}

template <typename ITER>
inline ITER* ParametersReal::FileHandler<ITER>::iterator()
{
    return &d_iter;
}

template <typename ITER>
inline mqbs::MappedFileDescriptor&
ParametersReal::FileHandler<ITER>::mappedFileDescriptor()
{
    return d_mfd;
}

}  // close package namespace

}  // close enterprise namespace

#endif