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

// m_bmqstoragetool_commandprocessor.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_COMMANDPROCESSOR
#define INCLUDED_M_BMQSTORAGETOOL_COMMANDPROCESSOR

//@PURPOSE: Provide a command processor interface to process commands.
//
//@CLASSES:
//  m_bmqstoragetool::CommandProcessor: command processor interface.
//
//@DESCRIPTION: 'CommandProcessor' provides interface
// to process commands.

// bmqstoragetool
#include <m_bmqstoragetool_filemanager.h>
#include <m_bmqstoragetool_parameters.h>

// BDE
#include <bsl_ostream.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ======================
// class CommandProcessor
// ======================

class CommandProcessor {
  protected:
    // PRIVATE DATA
    const bsl::shared_ptr<Parameters>  d_parameters;
    const bsl::shared_ptr<FileManager> d_fileManager;
    bsl::ostream&                      d_ostream;

  public:
    // CREATORS

    /// Constructor using the specified `params` and 'ostream'.
    explicit CommandProcessor(const bsl::shared_ptr<Parameters>&  params,
                              const bsl::shared_ptr<FileManager>& fileManager,
                              bsl::ostream&                       ostream);

    virtual ~CommandProcessor() = default;

    // MANIPULATORS

    /// Process command.
    virtual void process() = 0;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline CommandProcessor::CommandProcessor(
    const bsl::shared_ptr<Parameters>&  params,
    const bsl::shared_ptr<FileManager>& fileManager,
    bsl::ostream&                       ostream)
: d_parameters(params)
, d_fileManager(fileManager)
, d_ostream(ostream)
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace

#endif
