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

// m_bmqstoragetool_commandprocessorfactory.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_COMMANDPROCESSORFACTORY
#define INCLUDED_M_BMQSTORAGETOOL_COMMANDPROCESSORFACTORY

//@PURPOSE: Provide a factory to create corresponding command processor.
//
//@CLASSES:
//  m_bmqstoragetool::CommandProcessorFactory: command processor factory.
//
//@DESCRIPTION: 'CommandProcessorFactory' provides a factory to create
// corresponding command processor.

// bmqstoragetool
#include <m_bmqstoragetool_journalfileprocessor.h>
#include <m_bmqstoragetool_parameters.h>

// BDE
#include <bsl_memory.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =============================
// class CommandProcessorFactory
// =============================

class CommandProcessorFactory {
  public:
    // MANIPULATORS

    /// Create command processor for specified 'params' using specified
    /// 'allocator'.
    static bsl::unique_ptr<CommandProcessor>
    createCommandProcessor(bsl::unique_ptr<Parameters> params,
                           bsl::ostream&               ostream,
                           bslma::Allocator*           allocator);
};

}  // close package namespace
}  // close enterprise namespace

#endif
