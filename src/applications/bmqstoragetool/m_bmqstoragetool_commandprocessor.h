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

// bmqstoragetool
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
    /// PRIVATE DATA
    const bsl::shared_ptr<Parameters> d_parameters;

  public:
    /// CREATORS
    explicit CommandProcessor(const bsl::shared_ptr<Parameters>& params);

    virtual ~CommandProcessor() = default;

    virtual void process(bsl::ostream& ostream) = 0;
};

inline CommandProcessor::CommandProcessor(
    const bsl::shared_ptr<Parameters>& params)
: d_parameters(params)
{
}

}  // close package namespace
}  // close enterprise namespace

#endif