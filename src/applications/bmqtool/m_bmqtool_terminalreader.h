// Copyright 2026 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQTOOL_TERMINALREADER
#define INCLUDED_M_BMQTOOL_TERMINALREADER

//@PURPOSE: Provide terminal input with command history for bmqtool.
//
//@CLASSES:
//  m_bmqtool::TerminalReader: Terminal input with history support.
//
//@DESCRIPTION: 'm_bmqtool::TerminalReader' provides a 'getLine' method that
// reads a line from stdin using termios raw mode to intercept keystrokes and
// support command history navigation via arrow keys.  When stdin is not a TTY,
// it falls back to standard 'getline'.

// BDE
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace m_bmqtool {

// ====================
// class TerminalReader
// ====================

/// @brief Terminal input with command history support.
class TerminalReader {
  private:
    // DATA
    bslma::Allocator*        d_allocator_p;
    bsl::vector<bsl::string> d_history;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TerminalReader, bslma::UsesBslmaAllocator)

    // CREATORS

    /// @brief Create a TerminalReader.
    /// @param allocator optional allocator for memory management
    explicit TerminalReader(bslma::Allocator* allocator = 0);

    // MANIPULATORS

    /// @brief Read a line from stdin with history support.
    ///
    /// Print the specified @p prompt, handle arrow keys for history
    /// navigation, and store the result in the specified @p out.
    /// Non-empty submitted lines are appended to the internal history.
    /// When stdin is not a TTY, fall back to standard 'getline'.
    ///
    /// @param out    output string to store the entered line
    /// @param prompt the prompt string displayed before input
    /// @return 'true' on success, 'false' on EOF (Ctrl-D on empty line)
    bool getLine(bsl::string* out, bsl::string_view prompt = "> ");
};

}  // close package namespace
}  // close enterprise namespace

#endif
